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
 *  5_spin_wait_attach.c
 *
 *  The classic "spin-and-attach" trick: a few lines of code that
 *  pause a program early enough for you to attach a debugger to it.
 *  Useful whenever you cannot conveniently launch under gdb — most
 *  obviously, an MPI rank running inside a batch job at a centre,
 *  but also any OpenMP program where startup is brief and the bug
 *  is in a parallel region you need to be attached for.
 *
 *  The pattern, applied to OpenMP:
 *
 *      [1]  the master thread (only) waits for the debugger BEFORE
 *           the parallel region;
 *      [2]  once the flag is set, the program enters `omp parallel`
 *           and runs whatever you actually wanted to debug;
 *      [3]  inside `omp parallel`, breakpoints, prints, watchpoints —
 *           all of Section B applies, and `info threads` (D2) shows
 *           the OpenMP worker pool.
 *
 *  In MPI the same idiom runs in every rank.  Each rank prints its
 *  world-rank and its PID; you attach to the rank(s) you care about
 *  (rank 47 of 1024, the one that crashes) and release the rest by
 *  setting the flag from inside gdb, or by a small helper script.
 *  No permission from the queue system is required beyond the right
 *  to ptrace your own processes (B14: yama ptrace_scope = 1 is
 *  enough; you are attaching to a descendant).
 *
 *  Pedagogical sequence:
 *
 *    [1]  Build and run in one terminal:
 *
 *         gcc -O0 -g3 -fopenmp -o spin 5_spin_wait_attach.c
 *         ./spin
 *
 *         Output:
 *             PID 41312:  spin_flag = 0, waiting for debugger...
 *             attach with:   gdb -p 41312
 *             then in gdb:   (gdb) set spin_flag = 1
 *                            (gdb) continue
 *
 *    [2]  In a second terminal:
 *
 *         gdb -p 41312
 *         (gdb) print spin_flag
 *         $1 = 0
 *         (gdb) set spin_flag = 1
 *         (gdb) continue
 *
 *         The master thread escapes the loop and enters the parallel
 *         region.  At that point `info threads` shows the worker
 *         pool; from here you debug exactly as in Section B.
 *
 *    [3]  Compile-time variant: a debug-only build that includes
 *         the spin only when SPIN_FOR_DEBUGGER is defined.  In
 *         production builds the loop disappears completely.  In
 *         practice this is the form to keep in a real codebase —
 *         the spin is a one-line preprocessor switch, off by
 *         default, no impact on shipped binaries.
 *
 *  Why this is the open-source alternative to DDT/Forge for
 *  "attach to rank/thread N at startup".  Commercial parallel
 *  debuggers build this in: you click a rank, they attach.  Without
 *  one of those, the spin-wait trick gives you the same capability
 *  with five lines of code and gdb.
 *
 *  Meta-lesson: a debugger is most useful when the program is
 *  reachable.  When the program would otherwise be unreachable —
 *  inside an MPI job, behind a queue system, mid-startup before
 *  you can blink — the spin-wait gives you a window.  Cheap,
 *  portable, and reliable.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>


volatile int spin_flag = 0;       /* volatile: do not optimise the loop away */


/* call this as early as you can stand to, on the master thread,
 * BEFORE any parallel region.  In MPI, after MPI_Init and after a
 * "rank N PID M" print, so the operator can pick the rank to
 * attach to. */
static void wait_for_debugger( void )
{
  fprintf(stderr,
          "PID %d:  spin_flag = 0, waiting for debugger...\n"
          "attach with:   gdb -p %d\n"
          "then in gdb:   (gdb) set spin_flag = 1\n"
          "               (gdb) continue\n",
          (int)getpid(), (int)getpid());
  fflush(stderr);

  while ( spin_flag == 0 )
    {
      /* a small sleep keeps the CPU usage modest while we wait.
       * usleep is not async-signal-safe but is fine here. */
      usleep(100000);
    }

  fprintf(stderr, "PID %d:  spin_flag = 1, continuing.\n",
          (int)getpid());
}


/* a tiny piece of OpenMP work, standing in for whatever the
 * application actually does.  Replace by the parallel region you
 * are trying to debug. */
static void do_work( void )
{
  long sum = 0;

  #pragma omp parallel for reduction(+:sum)
  for ( int i = 0; i < 1000000; i++ )
    sum += i;

  printf("sum = %ld   (running with up to %d threads)\n",
         sum, omp_get_max_threads());
}


int main( void )
{
  wait_for_debugger();
  do_work();
  return 0;
}
