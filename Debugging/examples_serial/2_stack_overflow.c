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
 *  2_stack_overflow.c
 *
 *  Demonstrates a stack overflow caused by deep recursion.  The recursive
 *  function is logically correct -- it computes the sum of an array, one
 *  element per recursive call -- but each call consumes a stack frame, and
 *  a sufficiently large array exhausts the per-thread stack limit.
 *
 *  So:
 *
 *    -  the program is correct in the abstract, broken in practice;
 *       "correctness" without resource analysis is incomplete.
 *
 *    -  GDB's backtrace under stack exhaustion eventually stops with
 *       "Backtrace stopped: previous frame identical to this frame" --
 *       the smoking gun for runaway recursion.
 *
 *    -  the OS reports SIGSEGV: the stack guard page was touched.
 *       A bare segfault is not always a pointer bug.
 *
 *    -  `ulimit -s` reveals (and changes) the soft stack limit, usually
 *       8192 KB on Linux.  Raising it postpones the failure but does not
 *       cure it.  The real fix is converting the recursion to iteration
 *       (see 2b_stack_overflow_inspect.c, which shows how -O3 may do
 *       that conversion for you).
 *
 *  BUILD:
 *      gcc -O0 -g3 -o stack_ovf 2_stack_overflow.c
 *
 *  USAGE:
 *      ./stack_ovf            (default 1000 elements: works)
 *      ./stack_ovf 500000     (typically crashes -- depends on ulimit -s)
 *      ulimit -s 32768
 *      ./stack_ovf 500000     (may now work, until count is larger still)
 *
 *  UNDER GDB:
 *      gdb ./stack_ovf
 *      (gdb) run 500000
 *      ... crash ...
 *      (gdb) bt                # truncated backtrace, thousands of frames
 *      (gdb) bt -50            # last 50 frames only
 *      (gdb) info frame        # the recurring identical frame
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE 1000


/* recursive sum: each call eats one stack frame.
 * for `count` of order 10^5..10^6 the default Linux stack (8 MB) is
 * exhausted and SIGSEGV is delivered when the stack guard page is hit. */
long long recursive_sum( int *array, size_t count )
{
  if ( count == 0 )
    return 0;
  return array[0] + recursive_sum( array + 1, count - 1 );
}


int main( int argc, char *argv[] )
{
  size_t size = DEFAULT_SIZE;

  if ( argc > 1 )
    {
      size = (size_t)atol( argv[1] );
      if ( size == 0 )
        {
          fprintf(stderr, "invalid size; using default %d\n", DEFAULT_SIZE);
          size = DEFAULT_SIZE;
        }
    }

  int *numbers = (int *)malloc( size * sizeof(int) );
  if ( numbers == NULL )
    { perror("malloc"); return EXIT_FAILURE; }

  for ( size_t i = 0; i < size; i++ )
    numbers[i] = (int)i;

  long long sum = recursive_sum( numbers, size );

  printf("sum = %lld\n", sum);

  free(numbers);
  return EXIT_SUCCESS;
}
