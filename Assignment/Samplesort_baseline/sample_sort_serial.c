#define _POSIX_C_SOURCE 200809L

/*
  Serial sample-sort baseline for the HPC final-exam exercise.

  This program is intentionally serial.  It mirrors the structure of the
  distributed sample-sort algorithm using a configurable number of "virtual
  ranks": each virtual rank owns one chunk, sorts it, selects regular samples,
  partitions the sorted chunk into buckets, and contributes those buckets to a
  final k-way merge.

  The goal is not to provide the fastest possible serial integer sorter, but just 
  a readable, correct starting point that already has the same algorithmic checkpoints
  as the later MPI + OpenMP code:

    1. local sort;
    2. regular sampling;
    3. pivot selection;
    4. bucket partitioning;
    5. exchange-like bucket redistribution;
    6. final merge of sorted incoming streams.

  The local sort is a straightforward bottom-up merge sort.  The final k-way
  merge is deliberately naive: it scans the head of every incoming stream to
  find the next key.
  What an be done there? What different data structures/algorithm that you have seen
 in other coursed? what about memory traffic and impact of branching?
*/

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_NKEYS          (1000000ULL)
#define DEFAULT_NBUCKETS       (8u)
#define DEFAULT_OVERSAMPLE     (1ULL)
#define DEFAULT_SEED           (1ULL)
#define DEFAULT_DISTRIBUTION   "uniform"
#define DEFAULT_PRINT_LIMIT    (0ULL)


/* ========================================================================================

   : ------------------------------------------------------ :
   :  DATA TYPES &                                          :
   :  DATA STRUCTURES                                       :
   : ------------------------------------------------------ :
 */ 

typedef uint64_t sort_key_t;

typedef enum
{
  DISTRIBUTION_UNIFORM,
  DISTRIBUTION_SKEWED,
  DISTRIBUTION_FEW_UNIQUE,
  DISTRIBUTION_SORTED,
  DISTRIBUTION_REVERSE,
  DISTRIBUTION_ALMOST_SORTED
} distribution_t;

/*
  Runtime options for the serial baseline. Guess what?  nbuckets plays the role of the
  number of MPI processes in the future distributed implementation, but here it
  only controls how the single array is split into virtual chunks.
*/
typedef struct
{
  size_t            nkeys;
  unsigned int      nbuckets;
  size_t            oversample;
  uint64_t          seed;
  distribution_t    distribution;
  char             *distribution_name;
  size_t            print_limit;
} options_t;

/*
  Lightweight phase timings. 
*/
typedef struct
{
  double generation;
  double signature;
  double local_sort;
  double sampling;
  double partitioning;
  double merging;
  double sort_verification;
  double signature_verification;
  double total;
} timing_t;

/*
  Order-independent diagnostic signature.  Sortedness is the main correctness
  check used by this baseline; the signature is only an additional guard against
  accidental loss or duplication of keys while experimenting with the bucket
  code.
*/
typedef struct
{
  uint64_t          sum;
  uint64_t          xor_value;
} signature_t;


/* ========================================================================================
   
   : ------------------------------------------------------ :
   :  UTILITIES                                             :
   : ------------------------------------------------------ :
 */ 


/*
  Print command-line usage.
*/
static void
print_usage ( char     *program_name   // executable name from argv[0]
	      )
{
  fprintf (stderr,
           "Usage: %s [options]\n"
           "\n"
           "Options:\n"
           "  --n VALUE              number of keys to sort                    (%llu)\n"
           "  --nbuckets VALUE       number of virtual ranks / buckets          (%u)\n"
           "  --oversample VALUE     regular samples per virtual rank multiplier (%llu)\n"
           "  --seed VALUE           random seed for generated inputs            (%llu)\n"
           "  --distribution NAME    uniform | skewed | few-unique | sorted | reverse | almost-sorted (%s)\n"
           "  --print-limit VALUE    print the first VALUE sorted keys           (%llu)\n"
           "  --help                 show this help message\n"
           "\n"
           "Examples:\n"
           "  %s --n 1000000 --nbuckets 8 --distribution uniform\n"
           "  %s --n 1000000 --nbuckets 8 --distribution skewed --oversample 4\n",
           program_name,
           (unsigned long long) DEFAULT_NKEYS,
           DEFAULT_NBUCKETS,
           (unsigned long long) DEFAULT_OVERSAMPLE,
           (unsigned long long) DEFAULT_SEED,
           DEFAULT_DISTRIBUTION,
           (unsigned long long) DEFAULT_PRINT_LIMIT,
           program_name,
           program_name);
}

/*
  Fill the option structure with defaults.
*/
static void
set_default_options ( options_t   *options   // output options structure
		      )
{
  options->nkeys             = (size_t) DEFAULT_NKEYS;
  options->nbuckets          = DEFAULT_NBUCKETS;
  options->oversample        = (size_t) DEFAULT_OVERSAMPLE;
  options->seed              = DEFAULT_SEED;
  options->distribution      = DISTRIBUTION_UNIFORM;
  options->distribution_name = DEFAULT_DISTRIBUTION;
  options->print_limit       = (size_t) DEFAULT_PRINT_LIMIT;
}

/*
  Convert a distribution name into the internal enum.  
  Different types of data distribution are useful for a load-imbalance discussion:
  uniform is the clean case, skewed and few-unique stress the pivot selection,
  and sorted/reverse are edge cases for the local sort.
*/
static int
parse_distribution_name (char           *name,          // user-provided distribution name
                         distribution_t *distribution   // parsed distribution enum
			 )
{
  if (strcmp (name, "uniform") == 0)
    {
      *distribution = DISTRIBUTION_UNIFORM;
      return 0;
    }

  if (strcmp (name, "skewed") == 0)
    {
      *distribution = DISTRIBUTION_SKEWED;
      return 0;
    }

  if (strcmp (name, "few-unique") == 0 || strcmp (name, "fewunique") == 0)
    {
      *distribution = DISTRIBUTION_FEW_UNIQUE;
      return 0;
    }

  if (strcmp (name, "sorted") == 0)
    {
      *distribution = DISTRIBUTION_SORTED;
      return 0;
    }

  if (strcmp (name, "reverse") == 0)
    {
      *distribution = DISTRIBUTION_REVERSE;
      return 0;
    }

  if (strcmp (name, "almost-sorted") == 0 || strcmp (name, "almostsorted") == 0)
    {
      *distribution = DISTRIBUTION_ALMOST_SORTED;
      return 0;
    }

  fprintf (stderr, "Unknown distribution '%s'\n", name);
  return -1;
}

/*
  Parse an unsigned integer option into size_t.
  This helper keeps the command line parsing in main-style code readable
  and centralises overflow checks.
*/
static int
parse_size_option ( int      argc,    // number of command-line tokens
                    char   **argv,    // command-line token vector
                    int     *i,       // index of the option being parsed
                    size_t  *value    // parsed output value
		    )
{
  char             *endptr;
  unsigned long long parsed;

  if (*i + 1 >= argc)
    {
      fprintf (stderr, "Missing value after %s\n", argv[*i]);
      return -1;
    }

  errno = 0;
  endptr = NULL;
  parsed = strtoull (argv[*i + 1], &endptr, 10);

  if (errno != 0 || endptr == argv[*i + 1] || *endptr != '\0')
    {
      fprintf (stderr, "Invalid unsigned integer for %s: %s\n", argv[*i], argv[*i + 1]);
      return -1;
    }

  if (parsed > (unsigned long long) SIZE_MAX)
    {
      fprintf (stderr, "Value too large for this platform: %s\n", argv[*i + 1]);
      return -1;
    }

  *value = (size_t) parsed;
  *i += 1;
  return 0;
}

/*
  Parse an unsigned integer option into unsigned int.
*/
static int
parse_uint_option ( int            argc,    // number of command-line tokens
                    char         **argv,    // command-line token vector
                    int           *i,       // index of the option being parsed
                    unsigned int  *value    // parsed output value
		    )
{
  size_t            parsed;

  if (parse_size_option (argc, argv, i, &parsed) != 0)
    return -1;

  if (parsed > (size_t) UINT_MAX)
    {
      fprintf (stderr, "Value too large for unsigned int: %zu\n", parsed);
      return -1;
    }

  *value = (unsigned int) parsed;
  return 0;
}

/*
  Parse an unsigned integer option into uint64_t.
*/
static int
parse_u64_option ( int        argc,    // number of command-line tokens
                   char     **argv,    // command-line token vector
                   int       *i,       // index of the option being parsed
                   uint64_t  *value    // parsed output value
		   )
{
  char             *endptr;
  unsigned long long parsed;

  if (*i + 1 >= argc)
    {
      fprintf (stderr, "Missing value after %s\n", argv[*i]);
      return -1;
    }

  errno = 0;
  endptr = NULL;
  parsed = strtoull (argv[*i + 1], &endptr, 10);

  if (errno != 0 || endptr == argv[*i + 1] || *endptr != '\0')
    {
      fprintf (stderr, "Invalid unsigned integer for %s: %s\n", argv[*i], argv[*i + 1]);
      return -1;
    }

  *value = (uint64_t) parsed;
  *i += 1;
  return 0;
}

/*
  Parse all command-line options.
  Unknown flags are treated as errors so that mistakes in job scripts are caught
*/
static int
parse_options ( int          argc,      // number of command-line tokens
                char       **argv,      // command-line token vector
                options_t   *options    // output options structure
	      )
{
  int               i;

  set_default_options (options);

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--help") == 0)
        {
          print_usage (argv[0]);
          exit (EXIT_SUCCESS);
        }
      else if (strcmp (argv[i], "--n") == 0)
        {
          if (parse_size_option (argc, argv, &i, &options->nkeys) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--nbuckets") == 0)
        {
          if (parse_uint_option (argc, argv, &i, &options->nbuckets) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--oversample") == 0)
        {
          if (parse_size_option (argc, argv, &i, &options->oversample) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--seed") == 0)
        {
          if (parse_u64_option (argc, argv, &i, &options->seed) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--distribution") == 0)
        {
          if (i + 1 >= argc)
            {
              fprintf (stderr, "Missing value after %s\n", argv[i]);
              return -1;
            }

          if (parse_distribution_name (argv[i + 1], &options->distribution) != 0)
            return -1;

          options->distribution_name = argv[i + 1];
          i += 1;
        }
      else if (strcmp (argv[i], "--print-limit") == 0)
        {
          if (parse_size_option (argc, argv, &i, &options->print_limit) != 0)
            return -1;
        }
      else
        {
          fprintf (stderr, "Unknown option: %s\n", argv[i]);
          print_usage (argv[0]);
          return -1;
        }
    }

  return 0;
}

/*
  Validate the option values after parsing.
  In real codes you should always validate the input, t avoid wasting time
  with runs that either crash or are non-sense
*/
static int
validate_options ( options_t   *options   // parsed options to validate
		   )
{
  size_t            samples_per_chunk;

  if (options->nkeys == 0)
    {
      fprintf (stderr, "The number of keys must be positive\n");
      return -1;
    }

  if (options->nbuckets == 0)
    {
      fprintf (stderr, "The number of buckets must be positive\n");
      return -1;
    }

  if ((size_t) options->nbuckets > options->nkeys)
    {
      fprintf (stderr,
               "This teaching baseline requires nbuckets <= n so that each virtual chunk is non-empty\n");
      return -1;
    }

  if (options->oversample == 0)
    {
      fprintf (stderr, "The oversampling factor must be positive\n");
      return -1;
    }

  if (options->nbuckets > 1)
    {
      if (options->oversample > SIZE_MAX / (size_t) (options->nbuckets - 1))
        {
          fprintf (stderr, "The requested oversampling factor is too large\n");
          return -1;
        }

      samples_per_chunk = options->oversample * (size_t) (options->nbuckets - 1);

      if (samples_per_chunk > SIZE_MAX / (size_t) options->nbuckets)
        {
          fprintf (stderr, "The requested sample set is too large\n");
          return -1;
        }
    }

  return 0;
}

/*
  Return the current monotonic time in seconds.
*/
static double
wall_seconds (void)
{
  struct timespec   now;
  /*
  if (clock_gettime (CLOCK_MONOTONIC, &now) != 0)
    {
      perror ("clock_gettime");
      exit (EXIT_FAILURE);
    } */

  clock_gettime (CLOCK_MONOTONIC, &now);
  return (double) now.tv_sec + 1.0e-9 * (double) now.tv_nsec;
}

/*
  Allocate an array with size checking.
*/
static void *
malloc_array ( size_t count,        // number of array elements
                size_t element_size   // size of one element in bytes
		)
{
  void *ptr;

  if (count != 0 && element_size > SIZE_MAX / count)
    {
      fprintf (stderr, "Allocation size overflow: %zu elements of %zu bytes\n",
               count, element_size);
      exit (EXIT_FAILURE);
    }

  if (count == 0)
    return NULL;

  ptr = malloc (count * element_size);
  if (ptr == NULL)
    {
      fprintf (stderr, "Failed to allocate %zu bytes\n", count * element_size);
      exit (EXIT_FAILURE);
    }

  return ptr;
}

/*
  A small SplitMix64 generator.  It is fast, reproducible, and reasonable for
  input generation in this baseline.
*/
static uint64_t
splitmix64_next ( uint64_t * restrict state   // mutable generator state
		  )
{
  uint64_t z = (*state += UINT64_C (0x9e3779b97f4a7c15));
  
  z = (z ^ (z >> 30)) * UINT64_C (0xbf58476d1ce4e5b9);
  z = (z ^ (z >> 27)) * UINT64_C (0x94d049bb133111eb);
  z = z ^ (z >> 31);

  return z;
}

/*
  Mix a key into a pseudo-random-looking 64-bit value.
  The function is used for order-independent signatures so that mistakaes in
  accidental bucket-copy mistakes can be detected
*/
static uint64_t
mix_key ( sort_key_t key   // key value to mix
	  )
{
  uint64_t x;

  x = (uint64_t) key + UINT64_C (0x9e3779b97f4a7c15);
  x = (x ^ (x >> 30)) * UINT64_C (0xbf58476d1ce4e5b9);
  x = (x ^ (x >> 27)) * UINT64_C (0x94d049bb133111eb);
  x = x ^ (x >> 31);

  return x;
}

/*
  Generate an input array.
  The skewed distribution deliberately creates many keys in a small low-value range,
  which is useful to introduce bucket imbalance and test whether regular sampling
  is enough effective 
*/
static void
generate_keys (       sort_key_t * restrict keys,      // output key array
                const size_t                nkeys,     // number of keys to generate
                const options_t  * restrict options    // runtime options
	      )
{
  uint64_t    state;
  uint64_t    r;
  uint64_t    small_range;
  size_t      i;
  size_t      nswaps;
  size_t      a;
  size_t      b;
  sort_key_t  tmp;

  state = options->seed;

  if (options->distribution == DISTRIBUTION_SORTED)
    {
      for (i = 0; i < nkeys; i++)
        keys[i] = (sort_key_t) i;
      return;
    }

  if (options->distribution == DISTRIBUTION_REVERSE)
    {
      for (i = 0; i < nkeys; i++)
        keys[i] = (sort_key_t) (nkeys - i);
      return;
    }

  if (options->distribution == DISTRIBUTION_ALMOST_SORTED)
    {
      for (i = 0; i < nkeys; i++)
        keys[i] = (sort_key_t) i;

      nswaps = nkeys / 100;
      if (nswaps == 0)
        nswaps = 1;

      for (i = 0; i < nswaps; i++)
        {
          a = (size_t) (splitmix64_next (&state) % (uint64_t) nkeys);
          b = (size_t) (splitmix64_next (&state) % (uint64_t) nkeys);
          tmp = keys[a];
          keys[a] = keys[b];
          keys[b] = tmp;
        }
      return;
    }

  if (options->distribution == DISTRIBUTION_FEW_UNIQUE)
    {
      for (i = 0; i < nkeys; i++)
        keys[i] = (sort_key_t) (splitmix64_next (&state) % UINT64_C (1024));
      return;
    }

  if (options->distribution == DISTRIBUTION_SKEWED)
    {
      small_range = (uint64_t) (nkeys / 16 + 1);

      for (i = 0; i < nkeys; i++)
        {
          r = splitmix64_next (&state);

          // About 90 percent of the keys fall in a small interval.  This is a
          // simple stress case for pivot balance, not a faithful Zipf model.
          if ((r & UINT64_C (255)) < UINT64_C (230))
            keys[i] = (sort_key_t) (splitmix64_next (&state) % small_range);
          else
            keys[i] = (sort_key_t) splitmix64_next (&state);
        }
      return;
    }

  for (i = 0; i < nkeys; i++)
    keys[i] = (sort_key_t) splitmix64_next (&state);
}




/* ========================================================================================

   : ------------------------------------------------------ :
   :  SORTING FUNCTIONS                                     :
   : ------------------------------------------------------ :
 */ 




/*
  Compute the inclusive begin index of a virtual chunk.
  The first remainder chunks receive one extra element.
*/
static size_t
chunk_begin ( const size_t       nkeys,      // total number of keys
              const unsigned int nchunks,    // number of virtual chunks
              const unsigned int chunk_id    // chunk index
	    )
{
  size_t base;
  size_t remainder;

  base = nkeys / (size_t) nchunks;
  remainder = nkeys % (size_t) nchunks;

  if ((size_t) chunk_id < remainder)
    return (size_t) chunk_id * (base + 1);

  return remainder * (base + 1) + ((size_t) chunk_id - remainder) * base;
}

/*
  Compute the exclusive end index of a "virtual chunk".
*/
static size_t
chunk_end ( const size_t       nkeys,      // total number of keys
            const unsigned int nchunks,    // number of virtual chunks
            const unsigned int chunk_id    // chunk index
	  )
{
  return chunk_begin (nkeys, nchunks, chunk_id + 1);
}

/*
  Merge two adjacent sorted runs from data into scratch and copy the result back
  in the caller's pass.
  This is the primitive used both by local chunk sorting
  and by sorting the global sample array.
*/
static void
merge_runs ( sort_key_t *data,       // array containing the input runs
             sort_key_t *scratch,    // temporary array receiving the merged run
             size_t      left,       // first index of the left run
             size_t      middle,     // first index of the right run
             size_t      right       // one-past-last index of the right run 
           )
{
  size_t i;
  size_t j;
  size_t k;

  i = left;
  j = middle;
  k = left;

  while (i < middle && j < right)
    {
      if (data[i] <= data[j])
        scratch[k++] = data[i++];
      else
        scratch[k++] = data[j++];
    }

  while (i < middle)
    scratch[k++] = data[i++];

  while (j < right)
    scratch[k++] = data[j++];
}

/*
  Sort data[begin:end) with a bottom-up merge sort.
  The implementation is straightforward: every pass writes merged runs to scratch and
  copies the pass back to data.
*/
static void
merge_sort_range ( sort_key_t *data,      // array containing the range to sort
                   sort_key_t *scratch,   // temporary array with at least end elements
                   size_t      begin,     // first index of the sorted range
                   size_t      end        // one-past-last index of the sorted range
                 )
{
  size_t n;
  size_t width;
  size_t left;
  size_t middle;
  size_t right;

  if (end <= begin + 1)
    return;

  n = end - begin;

  for (width = 1; width < n; width *= 2)
    {
      for (left = begin; left < end; left += 2 * width)
        {
          middle = left + width;
          if (middle > end)
            middle = end;

          right = left + 2 * width;
          if (right > end)
            right = end;

          merge_runs (data, scratch, left, middle, right);
        }

      memcpy (data + begin, scratch + begin, n * sizeof (sort_key_t));

      if (width > n / 2)
        break;
    }
}

/*
  Sort each virtual rank's local chunk independently.
  In the parallel version, this is rank-local and is the obvious natural place
  for optimization ( OpenMP or different sorting algorithms )
*/
static void
sort_virtual_chunks ( sort_key_t    *keys,       // key array split into virtual chunks
                      sort_key_t    *scratch,    // temporary array for merge sort
                      size_t         nkeys,      // total number of keys
                      unsigned int   nchunks     // number of virtual chunks
		    )
{
  unsigned int rank;
  size_t       begin;
  size_t       end;

  for (rank = 0; rank < nchunks; rank++)
    {
      begin = chunk_begin (nkeys, nchunks, rank);
      end = chunk_end (nkeys, nchunks, rank);
      merge_sort_range (keys, scratch, begin, end);
    }
}

/*
  Select regular samples from every sorted virtual chunk.
  Regular sampling is later implemented before the MPI_ gatehring of samples; this
  serial function stores the gathered samples directly in one array.
*/
static void
select_regular_samples ( sort_key_t    *keys,                // locally sorted chunks
                         size_t         nkeys,               // total number of keys
                         unsigned int   nchunks,             // number of virtual chunks
                         size_t         samples_per_chunk,   // samples selected from each chunk
                         sort_key_t    *samples              // gathered sample array
		       )
{
  unsigned int rank;
  size_t       begin;
  size_t       end;
  size_t       chunk_size;
  size_t       s;
  size_t       local_index;
  size_t       sample_id;

  sample_id = 0;

  for (rank = 0; rank < nchunks; rank++)
    {
      begin = chunk_begin (nkeys, nchunks, rank);
      end = chunk_end (nkeys, nchunks, rank);
      chunk_size = end - begin;

      for (s = 1; s <= samples_per_chunk; s++)
        {
          local_index = (s * chunk_size) / (samples_per_chunk + 1);
          if (local_index >= chunk_size)
            local_index = chunk_size - 1;

          samples[sample_id++] = keys[begin + local_index];
        }
    }
}

/*
  Choose global pivots from the sorted sample array.
  With nbuckets = P, the pivots split the key space into P buckets.
  The pivoting is implemented simply.
  Experiment with stronger oversampling or different pivot positioning
*/
static void
choose_global_pivots ( sort_key_t    *samples,             // sorted gathered samples
                       size_t         samples_per_chunk,   // samples contributed by each virtual rank
                       unsigned int   nbuckets,            // number of output buckets
                       sort_key_t    *pivots               // output array of nbuckets - 1 pivots
		     )
{
  unsigned int bucket;
  size_t       nsamples;
  size_t       pivot_index;

  if (nbuckets <= 1)
    return;

  nsamples = (size_t) nbuckets * samples_per_chunk;

  for (bucket = 1; bucket < nbuckets; bucket++)
    {
      pivot_index = (size_t) bucket * samples_per_chunk;

      if (pivot_index >= nsamples)
        pivot_index = nsamples - 1;

      pivots[bucket - 1] = samples[pivot_index];
    }
}

/*
  Find the first position in data[begin:end) with value greater than key.
  Using upper_bound means that keys equal to a pivot stay in the lower bucket.
  If there are many data with the same value, this could create imbalance.
*/
static size_t
upper_bound_key ( sort_key_t   *data,    // sorted array
                  size_t        begin,   // first search index
                  size_t        end,     // one-past-last search index
                  sort_key_t    key      // pivot key
		)
{
  size_t left;
  size_t right;
  size_t middle;

  left = begin;
  right = end;

  while (left < right)
    {
      middle = left + (right - left) / 2;

      if (data[middle] <= key)
        left = middle + 1;
      else
        right = middle;
    }

  return left;
}

/*
  Partition every sorted virtual chunk into nbuckets sorted subranges.
  In MPI this becomes the send counter and displacement arrays used by (possibly)
  MPI_Alltoall and MPI_Alltoallv.
*/
static void
build_bucket_bounds ( sort_key_t    *keys,       // locally sorted chunks
                      size_t         nkeys,      // total number of keys
                      unsigned int   nchunks,    // number of virtual chunks
                      sort_key_t    *pivots,     // global pivots
                      size_t        *bounds      // output matrix nchunks x (nchunks + 1)
		    )
{
  unsigned int rank;
  unsigned int bucket;
  size_t       begin;
  size_t       end;
  size_t       cursor;
  size_t       pos;
  size_t       row;

  for (rank = 0; rank < nchunks; rank++)
    {
      begin = chunk_begin (nkeys, nchunks, rank);
      end = chunk_end (nkeys, nchunks, rank);
      cursor = begin;
      row = (size_t) rank * ((size_t) nchunks + 1);

      bounds[row] = begin;

      for (bucket = 0; bucket + 1 < nchunks; bucket++)
        {
          pos = upper_bound_key (keys, cursor, end, pivots[bucket]);
          bounds[row + (size_t) bucket + 1] = pos;
          cursor = pos;
        }

      bounds[row + (size_t) nchunks] = end;
    }
}

/*
  Compute the output offset of every destination bucket.
  In the parallel code, these sizes are the receive counters after MPI_All...
  In this serial code,
  they tell where each final bucket starts in the globally sorted output.
*/
static void
compute_bucket_starts ( size_t        *bounds,          // source/destination boundaries
                        unsigned int   nchunks,         // number of virtual chunks and buckets
                        size_t        *bucket_starts    // output prefix sum, length nchunks + 1
		      )
{
  unsigned int      destination;
  unsigned int      source;
  size_t            total;
  size_t            row;
  size_t            count;

  bucket_starts[0] = 0;

  for (destination = 0; destination < nchunks; destination++)
    {
      total = 0;

      for (source = 0; source < nchunks; source++)
        {
          row = (size_t) source * ((size_t) nchunks + 1);
          count = bounds[row + (size_t) destination + 1] - bounds[row + (size_t) destination];
          total += count;
        }

      bucket_starts[(size_t) destination + 1] = bucket_starts[destination] + total;
    }
}

/*
  Merge all sorted incoming streams for one destination bucket.
  The code scans all current stream heads to find the next key.
  This is clear but maybe not optimal.
  This is the place to possible compare different marging strategies
*/
static void
merge_destination_bucket ( sort_key_t    *keys,          // source sorted chunks
                           size_t        *bounds,        // source/destination boundaries
                           unsigned int   nchunks,       // number of incoming streams
                           unsigned int   destination,   // destination bucket to merge
                           sort_key_t    *output,        // full output array
                           size_t         out_begin      // first output index for this bucket
			 )
{
  size_t      *current;
  size_t      *end;
  unsigned int source;
  unsigned int best_source;
  int          have_best;
  sort_key_t   best_key;
  size_t       row;
  size_t       out;
  size_t       out_end;

  current = malloc_array ((size_t) nchunks, sizeof (size_t));
  end = malloc_array ((size_t) nchunks, sizeof (size_t));

  out = out_begin;
  out_end = out_begin;

  for (source = 0; source < nchunks; source++)
    {
      row = (size_t) source * ((size_t) nchunks + 1);
      current[source] = bounds[row + (size_t) destination];
      end[source] = bounds[row + (size_t) destination + 1];
      out_end += end[source] - current[source];
    }

  while (out < out_end)
    {
      have_best = 0;
      best_source = 0;
      best_key = 0;

      for (source = 0; source < nchunks; source++)
        {
          if (current[source] < end[source])
            {
              if (!have_best || keys[current[source]] < best_key)
                {
                  have_best = 1;
                  best_source = source;
                  best_key = keys[current[source]];
                }
            }
        }

      // have_best must be true while out < out_end.  If it is not, the bucket
      // boundary arithmetic above is inconsistent.
      if (!have_best)
        {
          fprintf (stderr, "Internal error during k-way merge\n");
          free (current);
          free (end);
          exit (EXIT_FAILURE);
        }

      output[out++] = best_key;
      current[best_source] += 1;
    }

  free (current);
  free (end);
}

/*
  Merge every destination bucket in increasing pivot order.
  Concatenating these merged buckets gives the final globally sorted array because all keys in
  bucket b are less than or equal to all keys in bucket b + 1 by construction.
*/
static void
merge_all_buckets ( sort_key_t    *keys,            // locally sorted source chunks
                    size_t        *bounds,          // source/destination boundaries
                    size_t        *bucket_starts,   // output prefix sum by destination bucket
                    unsigned int   nchunks,         // number of virtual chunks and buckets
                    sort_key_t    *output           // globally sorted output array
		  )
{
  unsigned int destination;

  for (destination = 0; destination < nchunks; destination++)
    merge_destination_bucket (keys, bounds, nchunks, destination, output,
                              bucket_starts[destination]);
}



/* ========================================================================================

   : ------------------------------------------------------ :
   :  VERIFICATION UTILITIES                                :
   : ------------------------------------------------------ :
 */ 


/*
  Compute an order-independent signature of the array.
  Note: this does not prove that the output is a permutation of the input, but it is a cheap diagnostic
  to assess that the elements are the same
*/
static signature_t
compute_signature ( sort_key_t   *keys,    // array to inspect
                    size_t        nkeys    // number of keys
		  )
{
  signature_t sig;
  uint64_t    mixed;
  size_t      i;

  sig.sum = 0;
  sig.xor_value = 0;

  for (i = 0; i < nkeys; i++)
    {
      mixed = mix_key (keys[i]);
      sig.sum += mixed;
      sig.xor_value ^= mixed;
    }

  return sig;
}

/*
  Compare two signatures.  Kept as a helper so the final verification printout
  reads cleanly.
*/
static int
same_signature ( signature_t a,    // first signature
                 signature_t b     // second signature
)
{
  return a.sum == b.sum && a.xor_value == b.xor_value;
}

/*
  Check that the array is globally non-decreasing.
  This is the main, perhaps obvious, correctness test requested for the serial baseline
  You should map to the parallel case
  
*/
static int
verify_sorted ( sort_key_t *keys,       // array to verify
                 size_t     nkeys,      // number of keys
                 size_t    *bad_index   // first failing index, if any
	      )
{
  size_t i;

  for (i = 1; i < nkeys; i++)
    {
      if (keys[i - 1] > keys[i])
        {
          *bad_index = i;
          return 0;
        }
    }

  *bad_index = 0;
  return 1;
}

/*
  Print a short prefix of the sorted array.
  This is useful for small examples.
*/
static void
print_key_prefix ( sort_key_t   *keys,        // sorted key array
                   size_t        nkeys,       // number of keys
                   size_t        print_limit  // number of keys to print
		 )
{
  size_t i;
  size_t nprint;

  if (print_limit == 0)
    return;

  nprint = print_limit;
  if (nprint > nkeys)
    nprint = nkeys;

  printf ("first_keys");
  for (i = 0; i < nprint; i++)
    printf (" %" PRIu64, (uint64_t) keys[i]);
  printf ("\n");
}



/* ========================================================================================

   : ------------------------------------------------------ :
   :  SORTING DRIVER                                     :
   : ------------------------------------------------------ :
 */ 



/*
  Run the serial sample-sort-like algorithm.
  The input array is modified during the local sort phase;
  the final globally sorted sequence is written to output.
*/
static void
sample_sort_serial ( sort_key_t    *keys,          // input keys, modified by local sorts
                     sort_key_t    *output,        // globally sorted output keys
                     size_t         nkeys,         // number of keys
                     options_t     *options,       // runtime options
                     timing_t      *timing         // phase timings to fill
		   )
{
  sort_key_t *scratch;
  sort_key_t *samples;
  sort_key_t *sample_scratch;
  sort_key_t *pivots;
  size_t     *bounds;
  size_t     *bucket_starts;
  size_t      samples_per_chunk;
  size_t      nsamples;
  double      t0;
  double      t1;

  scratch = malloc_array (nkeys, sizeof (sort_key_t));

  // ··············································
  // sort your local chunk
  
  t0 = wall_seconds ();
  sort_virtual_chunks (keys, scratch, nkeys, options->nbuckets);
  t1 = wall_seconds ();
  timing->local_sort = t1 - t0;

  if (options->nbuckets == 1)
    {
      memcpy (output, keys, nkeys * sizeof (sort_key_t));
      free (scratch);
      timing->sampling = 0.0;
      timing->partitioning = 0.0;
      timing->merging = 0.0;
      return;
    }

  // ··············································
  // sample
  
  samples_per_chunk = options->oversample * (size_t) (options->nbuckets - 1);
  nsamples          = samples_per_chunk * (size_t) options->nbuckets;

  samples        = malloc_array (nsamples, sizeof (sort_key_t));
  sample_scratch = malloc_array (nsamples, sizeof (sort_key_t));
  pivots         = malloc_array ((size_t) options->nbuckets - 1, sizeof (sort_key_t));
  bounds         = malloc_array ((size_t) options->nbuckets * ((size_t) options->nbuckets + 1), sizeof (size_t));
  bucket_starts  = malloc_array ((size_t) options->nbuckets + 1, sizeof (size_t));

  t0 = wall_seconds ();
  select_regular_samples (keys, nkeys, options->nbuckets, samples_per_chunk, samples);
  merge_sort_range       (samples, sample_scratch, 0, nsamples);
  choose_global_pivots   (samples, samples_per_chunk, options->nbuckets, pivots);
  t1 = wall_seconds ();
  timing->sampling = t1 - t0;

  // ··············································
  // have a global view
  
  t0 = wall_seconds ();
  build_bucket_bounds   (keys, nkeys, options->nbuckets, pivots, bounds);
  compute_bucket_starts (bounds, options->nbuckets, bucket_starts);
  t1 = wall_seconds ();
  timing->partitioning = t1 - t0;

  // ··············································
  // merge (here of course we lack exchanging data.. )
  
  t0 = wall_seconds ();
  merge_all_buckets (keys, bounds, bucket_starts, options->nbuckets, output);
  t1 = wall_seconds ();
  timing->merging = t1 - t0;

  free (bucket_starts);
  free (bounds);
  free (pivots);
  free (sample_scratch);
  free (samples);
  free (scratch);
}



/* ========================================================================================

   : ------------------------------------------------------ :
   :  SUMMARY PRINTING AND MAIN                             :
   : ------------------------------------------------------ :
 */ 


/*
  Print a compact summary of the run.
  This can be parse by a script when collecting repeated runs.
*/
static void
print_summary ( options_t     *options,       // runtime options
                timing_t      *timing,        // measured phase timings
                signature_t    before_sig,    // input signature
                signature_t    after_sig,     // output signature
                int            sorted_ok,     // result of sortedness check
                int            signature_ok,  // result of signature comparison
                size_t         bad_index      // first sortedness failure, if any
	      )
{
  printf ("nkeys                    %zu\n", options->nkeys);
  printf ("virtual_ranks            %u\n", options->nbuckets);
  printf ("oversample               %zu\n", options->oversample);
  printf ("distribution             %s\n", options->distribution_name);
  printf ("seed                     %" PRIu64 "\n", options->seed);
  printf ("sorted_ok                %s\n", sorted_ok ? "yes" : "no");
  printf ("multiset_signature_ok    %s\n", signature_ok ? "yes" : "no");
  printf ("input_signature_sum      %" PRIu64 "\n", before_sig.sum);
  printf ("input_signature_xor      %" PRIu64 "\n", before_sig.xor_value);
  printf ("output_signature_sum     %" PRIu64 "\n", after_sig.sum);
  printf ("output_signature_xor     %" PRIu64 "\n", after_sig.xor_value);

  if (!sorted_ok)
    printf ("first_bad_index          %zu\n", bad_index);

  printf ("time_generation_seconds  %.9f\n", timing->generation);
  printf ("time_local_sort_seconds  %.9f\n", timing->local_sort);
  printf ("time_sampling_seconds    %.9f\n", timing->sampling);
  printf ("time_partition_seconds   %.9f\n", timing->partitioning);
  printf ("time_merge_seconds       %.9f\n", timing->merging);
  printf ("time_verify_seconds      %.9f\n", timing->verification);
  printf ("time_total_seconds       %.9f\n", timing->total);
}



/* ======================================================================================== */

/*
  Program entry point.  The workflow is deliberately close to the exercise
  statement: generate input, compute a pre-sort diagnostic, run sample sort,
  verify global non-decreasing order, and print reproducible diagnostics.
*/
int
main ( int      argc,    // number of command-line tokens
       char   **argv     // command-line token vector
     )
{
  options_t    options;
  timing_t     timing;
  sort_key_t  *keys;
  sort_key_t  *output;
  signature_t  before_sig;
  signature_t  after_sig;
  double       t_start;
  double       t0;
  double       t1;
  size_t       bad_index;
  int          sorted_ok;
  int          signature_ok;

  if (parse_options (argc, argv, &options) != 0)
    return EXIT_FAILURE;

  if (validate_options (&options) != 0)
    return EXIT_FAILURE;

  memset (&timing, 0, sizeof (timing));

  keys = malloc_array (options.nkeys, sizeof (sort_key_t));
  output = malloc_array (options.nkeys, sizeof (sort_key_t));

  t_start = wall_seconds ();

  // ·······························································
  // generate keys
  
  t0 = wall_seconds ();
  generate_keys (keys, options.nkeys, &options);
  t1 = wall_seconds ();
  timing.generation = t1 - t0;

  // ·······························································
  // compute signature

  t0 = wall_seconds ();
  before_sig = compute_signature (keys, options.nkeys);
  t1 = wall_seconds ();
  timing.signature = t1 - t0;
    
  // ·······························································
  // sort

  sample_sort_serial (keys, output, options.nkeys, &options, &timing);

  // ·······························································
  // verify sort

  t0 = wall_seconds ();
  sorted_ok = verify_sorted (output, options.nkeys, &bad_index);
  t1 = wall_seconds ();
  timing.sort_verification = t1 - t0;
  
  // ·······························································
  // verify signature
  t0 = wall_seconds ();
  after_sig = compute_signature (output, options.nkeys);
  signature_ok = same_signature (before_sig, after_sig);
  t1 = wall_seconds ();
  timing.signature_verification = t1 - t0;
  
  timing.total = t1 - t_start;

  // ·······························································
  // print infos
  print_summary (&options, &timing, before_sig, after_sig,
                 sorted_ok, signature_ok, bad_index);
  print_key_prefix (output, options.nkeys, options.print_limit);

  free (output);
  free (keys);

  if (!sorted_ok || !signature_ok)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
