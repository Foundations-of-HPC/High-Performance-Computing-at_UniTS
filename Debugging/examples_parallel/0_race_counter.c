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
 *  0_race_counter.c
 *
 *  The simplest, most canonical concurrency bug: a data race on a
 *  shared counter.  Two OpenMP threads each increment the counter
 *  ITERATIONS times; the expected final value is  2 * ITERATIONS.
 *  You will almost never get it.
 *
 *  The bug.
 *    `counter++` is not atomic.  At the machine level it is at least:
 *         load counter into a register
 *         add 1
 *         store register back to counter
 *    Two such triples from two threads can interleave so that the
 *    store of one overwrites the load+add+store of the other:
 *    the two increments collapse into one.  The lost-update pattern
 *    is the prototype of every race condition.
 *
 *  Pedagogical sequence:
 *
 *    [1] Plain run at -O0:
 *
 *        gcc -O0 -g3 -fopenmp -o race 0_race_counter.c
 *        ./race
 *
 *        Output: counter = (something less than 2 * ITERATIONS).
 *        Re-run; the value changes between runs.  The schedule
 *        decides the answer.
 *
 *    [2] At -O2:
 *
 *        gcc -O2 -g3 -fopenmp -o race 0_race_counter.c
 *        ./race
 *
 *        Output: typically much further from the expected value than
 *        at -O0.  The optimizer keeps the counter in a register for
 *        longer, widening the window in which two threads see the
 *        same stale value.  Same source, worse race.
 *
 *    [3] Under ThreadSanitizer:
 *
 *        gcc -O1 -g3 -fopenmp -fsanitize=thread -o race 0_race_counter.c
 *        ./race
 *
 *        Output: "WARNING: ThreadSanitizer: data race" at the
 *        increment line, with both stack traces (write of thread T1
 *        and write of thread T2), the type and size of each access,
 *        and the name of the racing object.  The diagnostic names
 *        the bug in one go — the only tool that does.
 *
 *    [4] The fix, three ways:
 *
 *        (a)  #pragma omp atomic
 *             counter++;
 *
 *             The cheap, right tool for incrementing a single scalar.
 *             Compiles to a hardware-atomic operation; on x86, one
 *             `lock incq` instruction.
 *
 *        (b)  #pragma omp critical
 *             { counter++; }
 *
 *             Heavier — a full lock around an arbitrary block.  Use
 *             for compound updates, not for a single increment.
 *
 *        (c)  #pragma omp parallel reduction(+:counter)
 *
 *             The right tool when you only care about the TOTAL and
 *             not about the value during the run: each thread
 *             accumulates into a private partial; the runtime
 *             combines the partials at the close of the region.  No
 *             contention at all on the hot path.  See D6 for the
 *             general reduction discussion.
 *
 *  Meta-lesson: a race condition is a property of the schedule, not
 *  of the source.  The bug exists in every run; whether you observe
 *  it depends on whether the OS scheduler interleaved the threads in
 *  a way that exposed it.  Without a tool that watches every access
 *  (TSan), absence of symptom is no evidence of correctness.
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
    for ( long i = 0; i < ITERATIONS; i++ )
      counter++;              /* THE RACE: not atomic */
  }

  printf("counter = %ld   (expected %ld)\n",
         counter, 2L * ITERATIONS);

  return 0;
}
