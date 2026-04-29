
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

#define BUFFER_SIZE (256 * 1024 * 1024) // larger than L3
#define CLS 64                          // Cache Line Size
#define STRIDE_ELEMENTS (CLS / sizeof(uint64_t))  // how many uint64_t elements fit in one cache line
#define NUM_LINES (BUFFER_SIZE / CLS) // how many L1 lines are needed for the buffer size


void probe_memory_alloc( uint64_t );

  
// ················································
//

  
int main( int argc, char **argv )
{
  int bind_memory            = 0;
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
		   "-n allocate with explicit numa placement\n"
		   "-p probe where mem pages are\n");
	    return 0;
	  case 'b': bind_memory = 1; break;
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

  
  uint64_t *data[nsockets];
  int     thread_to_socket[nsockets];
 #define UINT64_DATASIZE (1 << 26)   // get beyond the L3
  
 #pragma omp parallel num_threads(nsockets) proc_bind(spread)
  {

    // --------------------------------------------------------------------------------------
    // 0. setting up
    // --------------------------------------------------------------------------------------
    
    int me       = omp_get_thread_num();
    int mysocket = omp_get_place_num();  // get on what socket this thread is running on
    uint64_t *indexes;
    
    thread_to_socket[me] = mysocket;
   if( numa_alloc ) 
     {
       data[me] = (uint64_t*)numa_alloc_onnode(BUFFER_SIZE * sizeof(uint64_t), mysocket);
       indexes  = (uint64_t*)numa_alloc_onnode(NUM_LINES * sizeof(uint64_t), mysocket);
     }
   else
     {
       data[me] = (uint64_t*)aligned_alloc( 64, BUFFER_SIZE );
       indexes  = (uint64_t*)aligned_alloc( 64, NUM_LINES*sizeof(uint64_t) );
     }


   #pragma omp single
    printf("\n[o] setting up random walk for %d cache lines..\n", NUM_LINES);

    
    // --------------------------------------------------------------------------------------
    // 1. initialize a regular path of cache lines indexes
    // --------------------------------------------------------------------------------------
    
    // note: this also amounts to a first-touch and should force the memory
    //       allocation to the nearest possible NUMA node 
    for ( uint64_t i = 0; i < NUM_LINES; i++ )
      indexes[i] = i;

    
    // --------------------------------------------------------------------------------------
    // 2. randomize the path
    // --------------------------------------------------------------------------------------
    
    unsigned short myseeds[3] = { 1234*me, 567^me, 8919+(1<<me) };
    //unsigned short myseeds[3] = { time(NULL)*me, time(NULL)^me, time(NULL)+(1<<me) };
    
    for ( uint64_t i = NUM_LINES-1; i > 0; i-- )
      {
	uint64_t j    = nrand48( myseeds ) % (i+1);
	uint64_t temp = indexes[i];
	indexes[i]    = indexes[j];
	indexes[j]    = temp;
      }

    for ( uint64_t i = 0; i < NUM_LINES; i++ )
      if ( indexes[i] > NUM_LINES )
	printf("idx[%lu] = %lu\n", i, indexes[i]);
    
   #pragma omp single
    printf("[o] building the dependency chain..\n");

    
    // --------------------------------------------------------------------------------------
    // 3. Build the Dependent Chain
    // array[current_random_line] = next_random_line
    // --------------------------------------------------------------------------------------
    
    for (uint64_t i = 0; i < NUM_LINES - 1; i++)
      {
	if ( indexes[i] * STRIDE_ELEMENTS >= BUFFER_SIZE/sizeof(uint64_t))
	  printf("out-of-bound for i=%lu : %lu %lu\n", i, indexes[i] * STRIDE_ELEMENTS, BUFFER_SIZE/sizeof(uint64_t));
	data[me][indexes[i] * STRIDE_ELEMENTS] = indexes[i + 1] * STRIDE_ELEMENTS;
      }
    
    // Close the loop so it can run infinitely
    data[me][indexes[NUM_LINES - 1] * STRIDE_ELEMENTS] = indexes[0] * STRIDE_ELEMENTS;

    if ( bind_memory )
      {
	// after the first touch, bind to the local node,
	// beyond the current memory policy
	// is numa allocation was used, this is not necessary
	unsigned long nodemask = 1UL << mysocket;
	mbind(data[me], UINT64_DATASIZE*sizeof(uint64_t),
	      MPOL_BIND, &nodemask, sizeof(nodemask)*8, 0);
	mbind(indexes, NUM_LINES*sizeof(uint64_t),
	      MPOL_BIND, &nodemask, sizeof(nodemask)*8, 0);
      }

   #pragma omp barrier

    
    // --------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------
    
   #pragma omp single
    {
      if ( probe_memory_placement )
	// we want to show where exactly the memory pages
	// are mapped
	probe_memory_alloc( UINT64_DATASIZE * sizeof(double) );
      printf( "[o] running the experiment for %d sockets..\n\n", nsockets );
    }

    double L[nsockets];   // nsockets is expected to be just a few
    
    for ( int runner = 0; runner < nsockets; runner ++ )
      {
	if ( me == runner )
	  {
	    printf( "SOCKET %d\n", thread_to_socket[me] );	    
	    
	    for ( int j = 0; j < nsockets; j++ )
	      {
		// -------------------------------------------------------------------------
		// 4. Measure the Latency
		// -------------------------------------------------------------------------
		
		uint64_t current_index = indexes[0] * STRIDE_ELEMENTS;
		uint64_t iterations = 10000000; // 10 million reads
		
		struct timespec start, end;

		double timing = CPU_TIME_T;
		// THE HOT LOOP: The CPU cannot execute the next read until the current one finishes
		for (uint64_t i = 0; i < iterations; i++)
		  current_index = data[j][current_index];
		timing = CPU_TIME_T - timing;
		timing /= iterations;

		L[j] = timing*1000000000ULL;
		
		printf("\t\tAverage Memory Latency S%d <-> S%d: %7.2f ns\n",
		       thread_to_socket[me], thread_to_socket[j], L[j]);


		// 5. cheat to the Compiler: 
		// Use the final result so -O3 doesn't delete the entire loop
		if (current_index == 0xffffffffffffffff)
		  printf("This will never print, but keeps the optimizer honest.\n");
		
	      }
	  }
       #pragma omp barrier
      }


    // ----------------------------------------
    //  OUTPUT the Latency matrix
    // ----------------------------------------

   #pragma omp single
    {
      printf("\nLatency matrix (ns)\n\n");
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
	  printf("%-6d  ", (unsigned int)L[i]);
	printf("\n");
      }
   #pragma omp single
    printf("\n");


    // ----------------------------------------
    //  Release the memory
    // ----------------------------------------

   #pragma omp barrier
    free( data[me] );
    free( indexes );
  }

  return 0;
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
