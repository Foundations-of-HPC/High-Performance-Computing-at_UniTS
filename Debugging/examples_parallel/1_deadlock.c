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
 *  1_deadlock.c
 *
 *  The textbook deadlock: two OpenMP sections, two locks, acquired
 *  in opposite orders.  Small `usleep()` calls ensure that each
 *  section holds one lock and is blocked waiting on the other by
 *  the time the second acquire is attempted.  Every run hangs.
 *
 *  The bug pattern (the "deadly embrace"):
 *
 *      section 1:                     section 2:
 *        omp_set_lock(&A);              omp_set_lock(&B);
 *        omp_set_lock(&B);   <-- blocks omp_set_lock(&A);   <-- blocks
 *
 *    Once both sections hold their first lock and are waiting on the
 *    second, no schedule can make progress.  The set of held-and-
 *    awaited locks forms a cycle; this is the formal condition for
 *    deadlock (Coffman, 1971).
 *
 *  OpenMP gives you several locking primitives:
 *      omp_lock_t           plain mutex (this file)
 *      omp_nest_lock_t      nestable (one thread can re-acquire)
 *      #pragma omp critical [name]    high-level region lock
 *      #pragma omp atomic             single-statement hardware atomic
 *    All of them can deadlock if you create a cycle in the lock-order
 *    graph.  The example uses `omp_lock_t` to make the cycle
 *    explicit; the same trap exists with named `critical` regions.
 *
 *  Pedagogical sequence:
 *
 *    [1] Run the program — it hangs.  Ctrl-C to escape, or:
 *
 *        gcc -O0 -g3 -fopenmp -o deadlock 1_deadlock.c
 *        ./deadlock &
 *        gdb -p $!
 *        (gdb) thread apply all bt
 *
 *        Two worker threads, both blocked inside the OpenMP runtime
 *        (gomp_mutex_lock_slow / omp_set_lock for libgomp), called
 *        from the outlined function for the parallel region.  The
 *        smoking gun: one is waiting on B (and holds A); the other
 *        is waiting on A (and holds B).  No one will wake up.
 *
 *    [2] Under ThreadSanitizer, without the deadlock-inducing
 *        delays:
 *
 *        gcc -O1 -g3 -fopenmp -fsanitize=thread -o deadlock 1_deadlock.c
 *        ./deadlock
 *
 *        TSan detects the lock-order inversion even on runs that
 *        happen NOT to deadlock.  It tracks the order in which each
 *        thread acquires locks; if any pair is acquired in opposite
 *        orders on different threads, the lock-order graph has a
 *        cycle and TSan reports it.  This is the right way to find
 *        deadlocks: before they fire, on a run that succeeded.
 *
 *    [3] The fix: a consistent lock-ordering invariant.  Choose a
 *        total order on locks (their addresses, their names —
 *        anything global) and always acquire them in that order.
 *        Then the cycle that creates the deadlock is impossible.
 *
 *  Meta-lesson: a deadlock is not a subtle bug once you look — every
 *  thread is sitting in the lock acquisition function.  The hard
 *  part is realising you have one.  A hung program may be slow, may
 *  be waiting on I/O, or may be deadlocked.  `thread apply all bt`
 *  answers in one line.
 */

#include <stdio.h>
#include <unistd.h>
#include <omp.h>


omp_lock_t lock_A;
omp_lock_t lock_B;


int main( void )
{
  omp_init_lock(&lock_A);
  omp_init_lock(&lock_B);

  #pragma omp parallel sections num_threads(2) shared(lock_A, lock_B)
  {
    #pragma omp section
    {
      printf("section 1: acquiring A...\n");
      omp_set_lock(&lock_A);

      usleep(100000);                       /* let section 2 grab B */

      printf("section 1: acquiring B...\n");
      omp_set_lock(&lock_B);                /* DEADLOCK */

      /* unreachable */
      omp_unset_lock(&lock_B);
      omp_unset_lock(&lock_A);
    }

    #pragma omp section
    {
      printf("section 2: acquiring B...\n");
      omp_set_lock(&lock_B);

      usleep(100000);                       /* let section 1 grab A */

      printf("section 2: acquiring A...\n");
      omp_set_lock(&lock_A);                /* DEADLOCK */

      /* unreachable */
      omp_unset_lock(&lock_A);
      omp_unset_lock(&lock_B);
    }
  }

  omp_destroy_lock(&lock_A);
  omp_destroy_lock(&lock_B);

  printf("done.\n");                         /* never reached */
  return 0;
}
