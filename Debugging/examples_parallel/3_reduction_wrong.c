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
 *  3_reduction_wrong.c
 *
 *  A specifically OpenMP failure mode: a loop that accumulates into
 *  a scalar, parallelised with `#pragma omp parallel for` but
 *  WITHOUT the `reduction(+:sum)` clause.  The accumulator is
 *  shared across threads; every iteration races every other
 *  iteration on the same variable.
 *
 *  This is the OpenMP equivalent of `0_race_counter.c`.  The bug is
 *  the same — non-atomic increments of a shared scalar — but in
 *  OpenMP the fix is a clause, not a lock: one keyword and the
 *  compiler arranges the private partial sums and the final
 *  combination for you.
 *
 *  The default visibility rules of `parallel for` make this easy
 *  to miss: `sum` declared OUTSIDE the parallel region is shared;
 *  `sum` declared INSIDE would be private.  Without `default(none)`
 *  the compiler will not complain; the program will simply give
 *  a wrong answer.
 *
 *  Pedagogical sequence:
 *
 *    [1] The wrong loop (this file's `bad_sum`):
 *
 *        gcc -O2 -g3 -fopenmp -o reduce 3_reduction_wrong.c
 *        OMP_NUM_THREADS=4 ./reduce
 *
 *        Output: the printed sum is wrong, varies between runs.
 *        Often it is a fraction of the true value, sometimes
 *        much smaller, occasionally — by coincidence — correct.
 *        The expected value for N = 1,000,000 is 499,999,500,000.
 *
 *    [2] Under TSan:
 *
 *        gcc -O1 -g3 -fopenmp -fsanitize=thread -o reduce 3_reduction_wrong.c
 *        OMP_NUM_THREADS=4 ./reduce
 *
 *        A data-race report on `sum`, with stack traces in both
 *        directions through the OMP-generated worker function.
 *
 *    [3] The fix (this file's `good_sum`):
 *
 *        #pragma omp parallel for reduction(+:sum)
 *        for (...) sum += ... ;
 *
 *        The compiler gives each thread a private `sum`, has each
 *        worker accumulate into its private copy, and combines
 *        the partials with `+` at the end.  Race free, fast,
 *        deterministic.  Equivalently, an `#pragma omp atomic`
 *        on the increment works (correct but much slower).
 *
 *  Meta-lesson:  in OpenMP, the data-sharing clauses are not
 *  decoration.  `default(none)` followed by an explicit
 *  `shared(...) private(...) reduction(...)` list, on every
 *  parallel construct, is the discipline that catches this class
 *  of bug at compile time — when the compiler refuses to guess
 *  what you meant.
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


#define N 1000000


/* THE BUG: sum is shared, += is not atomic, every iteration
 * races every other iteration that reaches sum at the same time. */
long bad_sum( const int *a, int n )
{
  long sum = 0;
  #pragma omp parallel for
  for ( int i = 0; i < n; i++ )
    sum += a[i];
  return sum;
}


/* THE FIX: reduction(+:sum) — private partial sums, combined
 * by the runtime at the end of the parallel region. */
long good_sum( const int *a, int n )
{
  long sum = 0;
  #pragma omp parallel for reduction(+:sum)
  for ( int i = 0; i < n; i++ )
    sum += a[i];
  return sum;
}


int main( void )
{
  int *a = (int *)malloc( N * sizeof(int) );
  if ( a == NULL ) { perror("malloc"); return 1; }
  for ( int i = 0; i < N; i++ )
    a[i] = i;

  long expected = (long)N * (N - 1) / 2;

  long b = bad_sum (a, N);
  long g = good_sum(a, N);

  printf("bad_sum   = %ld   (off by %+ld)\n", b, b - expected);
  printf("good_sum  = %ld   (off by %+ld)\n", g, g - expected);
  printf("expected  = %ld\n", expected);

  free(a);
  return 0;
}
