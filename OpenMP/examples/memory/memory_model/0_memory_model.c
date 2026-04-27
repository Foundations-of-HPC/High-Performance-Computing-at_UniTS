
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


/*
 * 0_memory_model.c
 * 
 * This routines are used to illustrate the OpenMP memory model,
 * i.e. to answer the questions: when are writes visible to other threads?
 * 
 * Remind about key concepts:
 * - Each thread has a "temporary view" that may contain stale values
 *   -> in the OMP mem model this "view" accounts for whatever component
 *      of the memory hierarchy that is not the DRAM, which is the ground truth
 * - Synchronization points (barriers, critical, etc.) include implicit flushes
 * - Without synchronization, visibility is NOT guaranteed
 *
 */

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


#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>

/* ··························································
 * Case 1: implicit synchronization at barrier (barrier)  
 * The barrier includes an implicit flush of the entire view,
 * so thread 1 sees thread 0's write.
 *
 * ·························································· */

void example_with_barrier(void)
{
    int shared_value = 0;
    
    printf("\n··· case 1: WITH barrier (correct) ···\n");
    
   #pragma omp parallel num_threads(2)
    {
      int tid = omp_get_thread_num();
        
      if (tid == 0)
	{
	  shared_value = 42;
	  printf("Thread 0: wrote shared_value = 42\n");
        }
      
        #pragma omp barrier  // Implicit flush here - all threads synchronize
        
        if (tid == 1)
	  printf("Thread 1: read shared_value = %d (should be 42)\n", shared_value);
    }
}

/* ··························································
 * Case 2: Visibility with critical regions
 * Critical regions include implicit flushes at entry and exit
 *
 * ·························································· */

void example_with_critical(void)
{
    int counter = 0;
    int iterations = 1000;
    
    printf("\n··· case 2: WITH critical (correct) ···\n");
    
    #pragma omp parallel num_threads(4)
    {
      for (int i = 0; i < iterations; i++)
	{
	 #pragma omp critical  // Implicit flush at entry and exit
	  {
	    counter++;  // Safe: only one thread at a time, with flush
	  }
	}
    }
    
    printf("Counter = %d (should be %d)\n", counter, 4 * iterations);
}

/* ··························································
 * Case 3: Visibility with atomic  
 * Atomic operations flush only the specific targete memory.
 * => lighter weight than critical sections.
 *
 * ·························································· */

void example_with_atomic(void)
{
    int counter = 0;
    int iterations = 1000;
    
    printf("\n··· Case 3: WITH atomic (correct) ···\n");
    
    #pragma omp parallel num_threads(4)
    {
        for (int i = 0; i < iterations; i++) {
            #pragma omp atomic  // Atomically update counter
            counter++;
        }
    }
    
    printf("Counter = %d (should be %d)\n", counter, 4 * iterations);
}

/* ··························································
 * Case 4: no synchronization
 * Without synchronization, updates may be lost due to race conditions.
 * Each thread may have a stale copy of 'counter' in registers or cache.
 * ==> see the_problem.v2.c example
 *
 * ·························································· */

void example_broken_no_sync( in nthreads )
{
    int counter = 0;
    int iterations = 1000;
    
    printf("\n··· case 4: no synchronization ···\n"
	   "              %d threads\n", nthreads);
    
    #pragma omp parallel num_threads(nthreads)
    {
        for (int i = 0; i < iterations; i++)
	  {
            // NO synchronization - race condition!
            // Multiple threads read counter, increment locally, write back
            // Some increments will be lost
            counter++;
	  }
    }
    
    printf("Counter = %d (should be %d)\n"
	   "likely - and more so with higher number of threads - it might be less due to races\n", 
           counter, 4 * iterations);
}

/* ··························································
 * Case 5: Demonstrating flush behavior 
 * flush is a point-to-point operation: it synchronizes the
 * calling-thread's view with shared memory, but doesn't wait for other threads.
 *
 * ·························································· */

void example_flush_behavior(void)
{
    int data = 0;
    int ready = 0;
    
    printf("\n··· Case 5: explicit Flush  ···\n");
    
    #pragma omp parallel num_threads(2)
    {
        int tid = omp_get_thread_num();
        
        if (tid == 0)
	  {
            // Producer
            data = 123;
	   #pragma omp flush(data)  // Push data to shared memory
	    
            ready = 1;
	   #pragma omp flush(ready) // Push ready flag to shared memory
	    
            printf("Thread 0: data=%d written, ready=%d set\n", data, ready);
	  }
        else
	  {
            // Consumer - wait for ready flag
            int local_ready = 0;
            while (local_ready == 0)
	      {
	       #pragma omp flush(ready)  // Pull fresh value from shared memory
		local_ready = ready;
		// Small delay to avoid hammering memory
		for (volatile int i = 0; i < 1000; i++);
	      }
            
	   #pragma omp flush(data)  // Pull fresh data from shared memory
            printf("Thread 1: saw ready=%d, data=%d\n", ready, data);
	  }
    }
}

/* ··························································
 * Case 6: Implicit flush at parallel region boundaries
 * entering/exiting a parallel region, all threads are synchronized.
 *
 * ·························································· */

void example_parallel_boundaries(int nthreads)
{
    int result = 0;
    
    printf("\n··· Case 6: Implicit flush at parallel region boundaries ···\n"
	   "              %d threads\n", nthreads);
    
    #pragma omp parallel num_threads(nthreads) reduction(+:result)
    {
        result += omp_get_thread_num() + 1;  // 1 + 2 + 3 + 4 = 10
    }
    // --> Implicit barrier + flush at end of parallel region
    // result value is now visible to main thread
    // and all the subsequent parallel regions
    
    printf("After first region: result = %d (expected: %d)\n",
	   result, nthreads*(nthreads-1)/2);
    
    #pragma omp parallel num_threads(2)
    {
      // All threads see result (from the implicit flush at region entry)
      printf("Thread %d sees result = %d\n",
	     omp_get_thread_num(), result);
    }
}

int main(int argc, char *argv[])
{
  int nthreads = (argc>1 ? atoi(*(argv+1): 4));
  
  printf("[ --------------------------------- ]\n");
  printf("Examples about OpenMP Memory Model\n");
  printf("Number of available threads: %d\n", omp_get_max_threads());
  printf("[ --------------------------------- ]\n");
    
    example_with_barrier();
    
    example_with_critical();
    
    example_with_atomic();
    
    example_flush_behavior();
    
    example_parallel_boundaries(nthreads);

    example_broken_no_sync(nthreads);
        
    return 0;
}
