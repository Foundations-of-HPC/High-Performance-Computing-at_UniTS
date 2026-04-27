
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


/*
 * 1_spinwait_flush.c
 * 
 * This code is used to illustrate the spin-wait (busy-wait)
 * synchronization using explicit flush and what happens when
 * the synch is not used
 * 
 * Key message 1 : When you're implementing your own synchronization
 * without using OpenMP's built-in constructs (barrier, critical, etc.),
 * explicit flush is necessary.
 *
 * Key message 2: BOTH threads must flush.
 *
 * ** > IN PRACTICE DO NOT USE THE FLUSH SEMANTICS, SINCE IT IS MORE
 * ** > A MEMORY FENCE THAN A PROPER SYNCHRONIZATION (ALTHOUGH THE
 * ** > FLUSH(LIST) IS IN PRINCIPLE EQUIVALENT TO ATOMICS).
 * ** > RECONSTRUCTING THE HAPPENS-BEFORE PATTERN IS VERY ERROR-PRONE
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* ·························································· this may
 * (may) hang forever
 * Without flush, the consumer may never see the producer's write.
 * The compiler and hardware can keep 'flag' in a register or cache line
 * indefinitely, never checking shared memory.
 *
 * ==> the same than in the_problem.v2.c
 * ·························································· */

void spinwait_forever(void)
{
  // 'volatile' is NOT sufficient in OpenMP!
  volatile int flag = 0;  
  int data = 0;
  
  printf("\n=== Spin-wait without flush (may hang) ===\n");
  
 #pragma omp parallel sections
  {
   #pragma omp section
    {
      // Producer
      data = 42;
      flag = 1;  // No flush - consumer may never see this!
      printf("Producer: set data=%d, flag=%d\n", data, flag);
    }
    
   #pragma omp section
    {
      // Consumer
      while (flag == 0)  // May spin forever
	;
      printf("Consumer: saw flag=%d, data=%d\n", flag, data);
    }
  }
}


/* ··························································
 * implementation with explicit flush
 * Both producer and consumer flush the flag variable.
 * This ensures visibility across threads.
 
 * ·························································· */
void spinwait_correct(void)
{
    int flag = 0;
    int data = 0;
    
    printf("\n=== Spin-wait with explicit flush ===\n");
    
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // Producer
            printf("Producer: starting work...\n");
            
            // Simulate some work
	    // "volatile" convinces the optimizer not to
	    // skip the loop
            for (volatile int i = 0; i < 10000000; i++);
            
            data = 42;
            #pragma omp flush(data)   // Push data to shared memory
            
            flag = 1;
            #pragma omp flush(flag)   // Push flag to shared memory
				      // *after* data
            
            printf("Producer: set data=%d, flag=%d\n", data, flag);
        }
        
        #pragma omp section
        {
            // Consumer
            printf("Consumer: waiting for flag...\n");
            
            int local_flag;
            do {
	     #pragma omp flush(flag)  // Pull fresh flag from shared memory
				      // --> important: respect the order;
				      // since flag has been pushed after data,
				      // its visibility ensures data visibility
	      local_flag = flag;
            } while (local_flag == 0);
            
            #pragma omp flush(data)  // Pull fresh data from shared memory
            
            printf("Consumer: saw flag=%d, data=%d\n", flag, data);
        }
    }
}

/*  
 * Alternative: Using OpenMP atomic operations (OpenMP 4.0+)
 * 
 * This is cleaner than explicit flush because the semantics are clearer.
 * The atomic read/write operations have well-defined memory ordering.
 *
 * ** > IN PRACTICE DO NOT USE THE FLUSH SEMANTICS, SINCE IT IS MORE
 * ** > A MEMORY FENCE THAN A PROPER SYNCHRONIZATION (ALTHOUGH THE
 * ** > FLUSH(LIST) IS IN PRINCIPLE EQUIVALENT TO ATOMICS).
 * ** > RECONSTRUCTING THE HAPPENS-BEFORE PATTERN IS VERY ERROR-PRONE
 *
 */
void spinwait_with_atomic(void)
{
    int flag = 0;
    int data = 0;
    
    printf("\n=== BEST: Spin-wait with atomic operations ===\n");
    
    #pragma omp parallel sections shared(flag, data)
    {
        #pragma omp section
        {
            // Producer
            printf("Producer: starting work...\n");
            
            // Simulate some work
            for (volatile int i = 0; i < 10000000; i++);
            
            data = 42;
            // Memory fence implicit in the following atomic write
            #pragma omp atomic write
            flag = 1;
            
            printf("Producer: set data=%d, flag=%d\n", data, flag);
        }
        
        #pragma omp section
        {
            // Consumer
            printf("Consumer: waiting for flag...\n");
            
            int local_flag = 0;
            while (local_flag == 0) {
                #pragma omp atomic read
                local_flag = flag;
            }
            
            // At this point, flag=1 is visible, but we need to ensure
            // data is also visible. The atomic provides a memory fence.
            
            printf("Consumer: saw flag=%d, data=%d\n", flag, data);
        }
    }
}

/*
 * Multiple producers, single consumer
 * 
 * A more complex example showing multiple threads signaling completion.
 */
void multi_producer_spinwait(void)
{
    #define NUM_PRODUCERS 4
    int done   [NUM_PRODUCERS] = {0};
    int results[NUM_PRODUCERS] = {0};
    
    printf("\n=== Multi-producer spin-wait ===\n");
    
   #pragma omp parallel num_threads(NUM_PRODUCERS + 1)
    {
      int tid = omp_get_thread_num();
      
      if (tid < NUM_PRODUCERS)
	{
	  // only producers enter here
        
	  // Simulate varying amounts of work
	  int work_amount = (tid + 1) * 5000000;
	  for (volatile int i = 0; i < work_amount; i++);
	  
	  // Store result
	  results[tid] = (tid + 1) * 100;
	 #pragma omp flush(results)
	  
	  // Signal completion
	  done[tid] = 1;
	 #pragma omp flush(done)
	  
	  printf("Producer %d: finished, result=%d\n", tid, results[tid]);
        }
      else
	{
	  // Consumer section
	  printf("Consumer: waiting for all producers...\n");
	  
	  int total = 0;
	  for (int p = 0; p < NUM_PRODUCERS; p++)
	    {
	      // Wait for producer p
	      int local_done;
	      do {
	       #pragma omp flush(done)
		local_done = done[p];
	      } while (local_done == 0);
              
	     #pragma omp flush(results)
	      total += results[p];
	      printf("Consumer: producer %d done, partial sum = %d\n", p, total);
            }
	  
	  printf("Consumer: all done, total = %d\n", total);
        }
    }
   #undef NUM_PRODUCERS
}

/*
 * NOTE about volatile
 * In C, 'volatile' tells the compiler not to optimize away memory accesses,
 * but it does NOT provide:
 * - Memory ordering guarantees between threads
 * - Cache coherence synchronization
 * 
 * OpenMP's flush (or, better, atomics) provides these guarantees.
 *
 *
 *
 *  volatile tells the compiler:
 *  - Don't cache this value in a register across statements
 *  - Don't optimize away repeated reads/writes
 *
 *  volatile does NOT guarantee:
 *  - Memory ordering between threads (reordering can still occur)
 *  - Cache line synchronization across cores
 *  - Visibility to other threads
 *
 *
 *  OpenMP proper synch provides:
 *  - Memory fence (ordering constraint)
 *  - Cache synchronization (write-through to shared memory)
 *
 *
 */

int main(int argc, char *argv[])
{
    printf("OpenMP Spin-Wait with Flush Examples\n");
    printf("====================================\n");
    
    spinwait_correct_with_flush();

    spinwait_with_atomic();

    multi_producer_spinwait();
    
    return 0;
}
