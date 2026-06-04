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
 *  4_heisenbug_printf.c
 *
 *  A heisenbug: a bug that vanishes when you try to observe it.
 *  This is the most famous variety, the one every concurrency
 *  course must show at least once.
 *
 *  Same shape as `0_race_counter.c` — two OpenMP threads incrementing
 *  a shared counter — with one twist: a printf inside the increment
 *  loop, supposedly to "see what's going on".  Adding the printf
 *  fixes the race.
 *
 *  Why.
 *    printf() takes an internal lock on the FILE* (stdout uses
 *    flockfile() under the hood, transparently).  Every iteration
 *    of the racing loop now serialises through that lock: only one
 *    thread is between "load counter" and "store counter" at a
 *    time.  The window in which two threads see the same stale
 *    value has been closed by the I/O lock — accidentally and
 *    invisibly.  The bug is still in the source; the schedule no
 *    longer exposes it.
 *
 *  Pedagogical sequence:
 *
 *    [1]  Build with USE_PRINTF undefined (the racing version):
 *
 *         gcc -O2 -g3 -fopenmp -o heisen 4_heisenbug_printf.c
 *         ./heisen
 *
 *         Output: counter < 2 * ITERATIONS, varying across runs.
 *         The race fires.  Same picture as 0_race_counter.c.
 *
 *    [2]  Build with USE_PRINTF defined ("I'll add a printf to
 *         see what's happening"):
 *
 *         gcc -O2 -g3 -fopenmp -DUSE_PRINTF \
 *             -o heisen 4_heisenbug_printf.c
 *         ./heisen > /dev/null
 *
 *         Output: counter == 2 * ITERATIONS, every run.  The bug
 *         is "gone".  It is not gone.  It is hidden.
 *
 *         (Discard stdout because the printfs are noise; the
 *         counter goes to stderr.)
 *
 *    [3]  Under TSan, either variant:
 *
 *         gcc -O1 -g3 -fopenmp -fsanitize=thread \
 *             [-DUSE_PRINTF] -o heisen 4_heisenbug_printf.c
 *         ./heisen
 *
 *         TSan reports the race in BOTH builds.  The race is a
 *         property of the source, not of whether you happen to
 *         observe it on this run.
 *
 *  Meta-lesson.  Every act of observation is an act of perturbation.
 *  The probe locks, the print I/O serialises, the gdb breakpoint
 *  stalls a thread — each changes the schedule, sometimes by
 *  enough to hide the bug.  The two tools that perturb least:
 *      - TSan, which instruments at compile time and does I/O at
 *        report time only;
 *      - rr (B15), which records once and replays deterministically.
 *
 *  The corollary is professional: when a parallel program shows
 *  symptoms intermittently, do not add printfs.  Run TSan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


#define ITERATIONS 1000000


long counter = 0;             /* shared between threads — racy */


int main( void )
{
  #pragma omp parallel num_threads(2)
  {
    int my_id = omp_get_thread_num();

    for ( long i = 0; i < ITERATIONS; i++ )
      {
#ifdef USE_PRINTF
        /* the "harmless" debug print that fixes the bug it was
         * meant to diagnose */
        if ( i % 100000 == 0 )
          printf("thread %d at iteration %ld, counter ~ %ld\n",
                 my_id, i, counter);
#else
        (void)my_id;
#endif
        counter++;                /* THE RACE */
      }
  }

  /* counter on stderr so it survives a `> /dev/null` */
  fprintf(stderr, "counter = %ld   (expected %ld)\n",
          counter, 2L * ITERATIONS);

  return 0;
}
