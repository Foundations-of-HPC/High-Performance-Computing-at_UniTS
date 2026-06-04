/* =============================================================================
 * This file is part of the exercises for the Lectures on
 *   "Foundations of High Performance Computing"
 * given at
 *   Master in HPC and
 *   Master in Data Science and Scientific Computing
 * @ University of Trieste / SISSA / ICTP
 *
 * contact: luca.tornatore@inaf.it
 *
 *     This is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 * =============================================================================
 *
 *  2_missing_barrier.c
 *
 *  A subtle OpenMP bug: a missing barrier between two phases of a
 *  parallel region lets some threads enter phase 2 while others are
 *  still in phase 1.  Result: reads of not-yet-written data.
 *
 *  The structure:
 *      phase 1:  data[tid] = expensive_compute(tid);
 *      phase 2:  use     = data[(tid + 1) % nthreads];
 *
 *  Without a barrier between them, a fast thread that finishes
 *  phase 1 quickly will run into phase 2 and read its neighbour's
 *  slot before the neighbour has written.  The output is unstable
 *  across runs: sometimes correct (the OS happens to schedule the
 *  threads roughly in phase), sometimes not.
 *
 *  Why this is easy to miss:
 *      OpenMP has implicit barriers at the END of `parallel for`
 *      and at the END of the `parallel` region.  Programmers
 *      reasonably assume barriers are everywhere; they are not.
 *      Within a single parallel region, two phases separated by
 *      ordinary code are NOT synchronised unless you say so.
 *      The `nowait` clause REMOVES the implicit barrier; an
 *      explicit `#pragma omp barrier` ADDS one.
 *
 *  Pedagogical sequence:
 *
 *    [1]  Plain run, several times:
 *
 *         gcc -O2 -g3 -fopenmp -o nobarrier 2_missing_barrier.c
 *         OMP_NUM_THREADS=4 ./nobarrier
 *
 *         The printed "neighbour" values vary across runs: some
 *         are -1 (the sentinel), some are correct, some are zero.
 *         The number of misses varies; on a quiet machine the bug
 *         may not fire at all.
 *
 *    [2]  Under ThreadSanitizer:
 *
 *         gcc -O1 -g3 -fopenmp -fsanitize=thread \
 *             -o nobarrier 2_missing_barrier.c
 *         OMP_NUM_THREADS=4 ./nobarrier
 *
 *         TSan reports a data race between the write at line
 *         "data[tid] = ..." and the read at "neighbour = data[...]"
 *         on a neighbouring index.  The diagnostic is precise.
 *
 *    [3]  The fix: uncomment the `#pragma omp barrier` between
 *         the phases.  Re-run: stable output, TSan clean.
 *
 *  Meta-lesson: "I am inside one parallel region" is not a
 *  synchronisation guarantee.  Read the OpenMP specification on
 *  implicit barriers, or — more practically — when data produced
 *  by one thread is consumed by another, draw the data flow on a
 *  napkin and place an explicit barrier at every edge that crosses
 *  threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


#define N_SLOTS 16
int data[N_SLOTS];


/* a fake "expensive computation" whose duration varies, so the
 * threads finish phase 1 at different times.  This is what exposes
 * the missing barrier; with constant-time work, threads tend to run
 * in lockstep on a quiet machine and the bug hides. */
int expensive_compute( int tid )
{
  volatile long w = 0;
  for ( long i = 0; i < (long)(1000000 * (tid + 1)); i++ )
    w += i;
  (void)w;
  return tid * 10;
}


int main( void )
{
  for ( int i = 0; i < N_SLOTS; i++ )
    data[i] = -1;                          /* sentinel */

  #pragma omp parallel default(none) shared(data)
  {
    int tid     = omp_get_thread_num();
    int nthr    = omp_get_num_threads();

    /* PHASE 1: write my slot */
    data[tid] = expensive_compute(tid);

    /* MISSING BARRIER — uncomment to fix:
     *   #pragma omp barrier
     */

    /* PHASE 2: read my neighbour's slot */
    int neighbour = data[(tid + 1) % nthr];

    printf("thread %d:  mine=%d   neighbour(%d)=%d\n",
           tid, data[tid], (tid + 1) % nthr, neighbour);
  }

  return 0;
}
