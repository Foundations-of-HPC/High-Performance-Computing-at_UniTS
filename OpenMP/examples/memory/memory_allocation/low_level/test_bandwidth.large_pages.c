
/* ────────────────────────────────────────────────────────────────────────── *
 │                                                                            │
 │ This file is part of the exercises for the Lectures on                     │
 │   "Foundations of High Performance Computing"                              │
 │ given at                                                                   │
 │   Master in HPC and                                                        │
 │   Master in Data Science and Scientific Computing                          │
 │ @ SISSA, ICTP and University of Trieste                                    │
 │                                                                            │
 │ contact: luca.tornatore@inaf.it                                            │
 │                                                                            │
 │     This is free software; you can redistribute it and/or modify           │
 │     it under the terms of the GNU General Public License as published by   │
 │     the Free Software Foundation; either version 3 of the License, or      │
 │     (at your option) any later version.                                    │
 │     This code is distributed in the hope that it will be useful,           │
 │     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
 │     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
 │     GNU General Public License for more details.                           │
 │                                                                            │
 │     You should have received a copy of the GNU General Public License      │
 │     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
 │                                                                            │
 * ────────────────────────────────────────────────────────────────────────── */

// ················································
//
#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif

#if !defined(_OPENMP)
#error "OpenMP support is mandatory for this code"
#endif

// ················································
//

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>

// needed for numalib interface
#include <numa.h>
#include <numaif.h>

// needed for large pages stuff
#include <sys/mman.h>

#include <omp.h>

#define CPU_TIME_T ({ struct timespec myts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), \
(double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })

// ················································
//

double stream            ( const double * restrict array, unsigned int N, double *, double * );
void   probe_memory_alloc( uint64_t  );
  
// ················································
//

  
int main( int argc, char **argv )
{
  int bind_memory            = 0;
  int large_pages            = 0;
  int numa_alloc             = 0;
  int probe_memory_placement = 0;
  
  {
    int opt;
    while ((opt = getopt(argc, argv, "hbnp")) != -1)
      {
	switch(opt)
	  {
	  case 'h':
	    printf("possible options:\n"
		   "-b explicitly bind memory\n"
		   "-l allocate large pages\n"
		   "-n allocate with explicit numa placement\n"
		   "-p probe where mem pages are\n");
	    return 0;
	  case 'b': bind_memory = 1; break;
	  case 'l': large_pages = 1; break;
	  case 'n': numa_alloc = 1; break;
	  case 'p': probe_memory_placement = 1; break;
	  case '?': printf("invalid option\n"); return 1;
	  }
      }
  }

  
  char *places = getenv("OMP_PLACES");
  char *bind   = getenv("OMP_PROC_BIND");

  if ( (places == NULL) ||
       ( (places!= NULL) && (strcasecmp(places, "sockets")) ) ) {
    printf("It is mandatory to set OMP_PLACES=sockets to run this code\n");
    return 1; }
  
  if ( bind != NULL )
    printf("OMP_PROC_BINDING is set to %s\n", bind);

  printf("Explicit memory binding: %s\n", (bind_memory?"on":"off"));
  
  int nsockets;
 #pragma omp parallel
 #pragma omp single
  nsockets = omp_get_num_places();

  
  double *data[nsockets];
  int     thread_to_socket[nsockets];
 #define DOUBLE_DATASIZE (1 << 26)   // get beyond the L3
  
 #pragma omp parallel num_threads(nsockets) proc_bind(spread)
  {   
    int me       = omp_get_thread_num();
    int mysocket = omp_get_place_num();  // get on what socket this thread is running on

    thread_to_socket[me] = mysocket;

    if ( numa_alloc )
      {
	// use libnuma to allocate memory on the nearest NUMA node
	data[me] = (double*)numa_alloc_onnode(DOUBLE_DATASIZE * sizeof(double), mysocket);
      }
    else
      {
	data[me] = MAP_FAILED;
	if ( large_pages )
	  {
	    // ask for large memory pages
	    //
	    size_t size = DOUBLE_DATASIZE * sizeof(double);
	    data[me] = mmap(NULL, size, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
			    -1, 0);
	    if ( data[me] == MAP_FAILED )
	      printf("   > mmap hugetlb failed, fallback to standard allocation\n");
	  }
	if ( data[me] == MAP_FAILED )
	  {
	    data[me] = (double*)aligned_alloc( 64, DOUBLE_DATASIZE*sizeof(double) );

	    if ( large_pages )
	      // ask the kernel to merge small pages into large ones	      
	      madvise(data[me], DOUBLE_DATASIZE * sizeof(double), MADV_HUGEPAGE);
	  }
      }
    
    // initialize, touch, drag through cache and TLB
    for ( int i = 0; i < DOUBLE_DATASIZE; i++ )
      data[me][i] = (double)i;

    if ( bind_memory )
      {
	// after the first touch, bind to the local node,
	// beyond the current memory policy
	// is numa allocation was used, this is not necessary
	unsigned long nodemask = 1UL << mysocket;
	mbind(data[me], DOUBLE_DATASIZE*sizeof(double),
	      MPOL_BIND, &nodemask, sizeof(nodemask)*8, 0);
      }
    
   #pragma omp barrier

   #pragma omp single
    {      
      if ( probe_memory_placement )
	// we want to show where exactly the memory pages
	// are mapped
	probe_memory_alloc( DOUBLE_DATASIZE * sizeof(double) );
      printf( "running the experiment for %d sockets..\n\n", nsockets );  
    }


    double bw[nsockets];   // nsockets is expected to be just a few

    /* ******************************************************************* *
     *  R U N  the   E X P E R I M E N T                                   *     
     * ******************************************************************* */
    
    for ( int runner = 0; runner < nsockets; runner ++ )
      {
	if ( me == runner )
	  {
	    printf( "SOCKET %d\n", runner );	    
	    
	    for ( int j = 0; j < nsockets; j++ ) {
	      printf( "\trunning the stream for S%d <-> S%d : ",
		      mysocket, thread_to_socket[j]);
	      double foo1, foo2;
	      bw[j] = stream ( data[j], DOUBLE_DATASIZE, &foo1, &foo2 );
	      printf( "BW: %4.2g +- %5.3f GB/s\n",
		      bw[j], foo1 );
	      if ( isnan(foo2) )
		printf("This will never be printed, but convince the "
		       "optimizer we really need the loop in strem()\n");
	    }	    
	    
	    printf("----------------------------------------\n\n");
	  }
       #pragma omp barrier
       #pragma omp single
	if ( probe_memory_placement )
	  probe_memory_alloc( DOUBLE_DATASIZE*sizeof(double) );
      }

    // ----------------------------------------
    //  OUTPUT the Bandwidth matrix
    // ----------------------------------------
    
   #pragma omp single
    {
      printf("\nBandwidth matrix (GB/s)\n\n");
      printf("%6s ", "");
      for (int i = 0; i < nsockets; i++)
	printf("S%-6d ", i);
      printf("\n-------");
      for (int i = 0; i < nsockets; i++)
	printf("--------");
      printf("\n");
    }
    
   #pragma omp for ordered
    for ( int runner = 0; runner < nsockets; runner++ )
     #pragma omp ordered
      {
	printf("S%-3d | ", runner);
	for (int i = 0; i < nsockets; i++)
	  printf("%-6.2g  ", bw[i]);
	printf("\n");
      }
   #pragma omp single
    printf("\n");
    

    // ----------------------------------------
    //  Release the memory
    // ----------------------------------------
    
   #pragma omp barrier

    if ( numa_alloc )
       numa_free( data[me], DOUBLE_DATASIZE * sizeof(double) );
    else
       free( data[me] );
   
  }

  return 0;
}


int cmp( const void *a, const void *b )
{
  const double A = *((double*)a);
  const double B = *((double*)b);
  return (A>B) - (A<B);
}

double stream( const double * restrict array, unsigned int N, double *stddev, double *result )
{
 #if !defined(UNROLL_FACTOR)
 #define UNROLL_FACTOR 8
 #endif
 #if !defined(NREPS)
 #define NREPS 1
 #warning "using just 1 repetition for the stream: compile with -DNREPS=N to have N repetitions (advice: N>~10)" 
 #endif


  //unsigned int _N  = N&0xFFFFFF8;  // renders clear that _N is a multiple of 8
  unsigned int _N  = N&(~((UNROLL_FACTOR)-1));  // renders clear that _N is a multiple of 8
  const double *_a = (const double*) __builtin_assume_aligned( array, 64 );

 
  double timings[NREPS] = {0};

  for ( int r = 0; r < NREPS; r++ )
    {
      double S[UNROLL_FACTOR] = {0};
      
      double timing = CPU_TIME_T;
     #pragma GCC ivdep
      for ( unsigned int i = 0; i < _N; i += UNROLL_FACTOR )
	{
	  for ( int j = 0; j < UNROLL_FACTOR; j++)
	    S[j] += _a[i+j]*2.0 + 1.0;
	}      
      timing = CPU_TIME_T - timing;

      timings[r] = timing;
      
      // induce the compiler not to optimize out
      for ( int j = 1; j < UNROLL_FACTOR; j++ )
	S[0] += S[j];
      *result += S[0];
    }

  double avgtime;
  double std_dev = 0;
  
  // sort the timings
  if (NREPS > 3)
    {
      qsort( timings, NREPS, sizeof(double), cmp );
      avgtime = timings[0];
      
      for ( int j = 1; j < NREPS-2; j++ )
	avgtime += timings[j];
      avgtime /= (NREPS-2);

      for ( int j = 0; j < NREPS-2; j++ )
	std_dev += (timings[j] - avgtime)*(timings[j] - avgtime);
      std_dev = sqrt(std_dev / (NREPS-2));
    }
  else
    avgtime = timings[0];

  *stddev = std_dev;
  
  double bandwidth = ( ((double)_N * sizeof(double))/(1<<30) / avgtime );   // calculate the bandwitdh in GB/s

  return bandwidth;
}


void probe_memory_alloc( uint64_t msize )
{
  // get the page size
  size_t pagesize = sysconf(_SC_PAGESIZE);

  // get how many pages we allcoated
  size_t npages = msize / pagesize;

  printf("···········································\n"
	 "probing memory placement on Numa Nodes:\n\n" );
  // visualize where the memory pages are allocated
  char cmd[256];
  snprintf(cmd, sizeof(cmd),
	   "awk '/anon=/ { for(i=1;i<=NF;i++) if($i ~ /^anon=/) { "
	   "split($i,a,\"=\"); if(a[2] > %ld ) print } }' /proc/%d/numa_maps",
	   npages, getpid());
  system(cmd);
  //
  printf("\n···········································\n\n");
  fflush(stdout);
}
