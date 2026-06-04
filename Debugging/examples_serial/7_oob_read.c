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
 *  7_oob_read.c
 *
 *  Demonstrates an out-of-bounds READ: the program reads past the end
 *  of an array.  Unlike out-of-bounds writes (which corrupt other
 *  data and tend to manifest in observable ways), out-of-bounds reads
 *  are usually silent -- the program reads whatever bytes happen to
 *  live just past the end and continues.
 *
 *  This is the most common shape of "information leakage" bugs in
 *  security contexts.  In scientific computing it usually shows as
 *  a numerical answer that is subtly wrong on some inputs.
 *  Note: who know why, the answers are always subtly wrong in these cases..
 *
 *  Sequence:
 *
 *    [1] Plain run:
 *
 *        gcc -O0 -g3 -o oob 7_oob_read.c
 *        ./oob
 *
 *        Result: the program runs to completion.  The first mean is
 *        slightly off (by one out-of-bounds element); the second mean
 *        is more visibly polluted because the OOB element interferes
 *        with the average over all known elements.  But NOTHING
 *        CRASHES.  Without instrumentation there is no symptom; you
 *        only notice if you happen to check the numerical result.
 *
 *    [2] Under AddressSanitizer:
 *
 *        gcc -O0 -g3 -fsanitize=address -o oob 7_oob_read.c
 *        ./oob
 *
 *        Result: ASan reports the offending read with the exact
 *        offset past the end of the array and the source line in
 *        the loop.
 *
 *    [3] Under Valgrind:
 *
 *        valgrind ./oob
 *
 *        Result: Valgrind reports the invalid read with an offset and
 *        a traceback to the read site.
 *
 *  Appreciate thast a bug that does not crash is more dangerous than one
 *  that does.  Without a sanitizer or Valgrind, this program "works"
 *  in the sense that it terminates successfully and produces output.
 *  The output is wrong, but slightly.  In a scientific code, this is
 *  the shape of bugs that propagtes into published results.
 */

#include <stdio.h>
#include <stdlib.h>

#define N 8


/* compute the mean of the first `count` elements of `arr`
 *
 * BUG: the loop reads count + 1 elements, not count.  The off-by-one
 * is on the loop bound (<= instead of <).  When `count` equals the
 * size of the array, this reads one element past the end. */
double mean( const double *arr, int count )
{
  double sum = 0.0;
  for ( int i = 0; i <= count; i++ )      /* <= : the bug */
    sum += arr[i];
  return sum / count;
}


int main( void )
{
  double data[N];
  for ( int i = 0; i < N; i++ )
    data[i] = (double)(i + 1);

  /* mean of the first 4 elements: should be (1+2+3+4)/4 = 2.5 */
  double m4 = mean( data, 4 );
  printf("mean of first 4 elements = %.6f   (expected 2.5)\n", m4);

  /* mean of all 8 elements: should be (1+...+8)/8 = 4.5
   * This call reads data[N], which is OFF THE END of the array. */
  double mall = mean( data, N );
  printf("mean of all %d elements   = %.6f   (expected 4.5)\n", N, mall);

  return 0;
}
