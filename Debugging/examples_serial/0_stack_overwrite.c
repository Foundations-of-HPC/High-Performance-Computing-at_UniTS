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
 *  0_stack_overwrite.c
 *
 *  Demonstrates a stack overwrite: writing past the end of a local array
 *  clobbers other local variables on the stack.
 *
 *  The most interesting thing about this program is not the
 *  bug itself but how four different builds produce four qualitatively
 *  different outcomes.  
 *
 *  USAGE:
 *      ./stack_overwrite             writes local_array[0..N] (one off the end)
 *      ./stack_overwrite 4           writes the helper malloc'd array too
 *
 *
 *  BUILD VARIANT 1 -- no stack protection
 *      gcc -O0 -g3 -fno-stack-protector -o stack_overwrite 0_stack_overwrite.c
 *      ./stack_overwrite
 *
 *      Expected: the iteration  i = N  writes one double past the end of
 *      local_array.  Depending on stack layout, this corrupts either
 *      bottom_stack or top_stack.  The printed values show one of them
 *      no longer holding its original integer value.
 *
 *
 *  BUILD VARIANT 2 -- with stack-smashing detection:
 *      gcc -O0 -g3 -fstack-protector-strong -o stack_overwrite 0_stack_overwrite.c
 *      ./stack_overwrite
 *
 *      Expected: GCC inserts a canary between locals and the saved return
 *      address; on return from main, the canary is checked, found altered,
 *      and the program aborts with  "*** stack smashing detected ***"  and
 *      a SIGABRT.
 *
 *
 *  BUILD VARIANT 3 -- with the optimizer:
 *      gcc -O2 -g3 -fno-stack-protector -o stack_overwrite 0_stack_overwrite.c
 *      ./stack_overwrite
 *
 *      Expected: with optimization on, locals may live in registers rather
 *      than on the stack, or be reordered; the precise corruption pattern
 *      shifts and may disappear entirely.
 *      Stack layout is not stable across optimization levels, and bugs that
 *      depend on it can "vanish" under different Olevels.
 *
 *
 *  BUILD VARIANT 4 -- under AddressSanitizer:
 *      gcc -O0 -g3 -fsanitize=address -o stack_overwrite 0_stack_overwrite.c
 *      ./stack_overwrite
 *
 *      Expected: ASan stops at the offending store (i = N), reports the
 *      exact frame, the offset past the array, and the source line.
 *      Far more informative than the SIGABRT in variant 2.
 *
 *
 *  Xariants 1, 2, 3, 4 show the same source code producing
 *  (a) silent corruption, (b) a hard abort, (c) a vanished symptom, and
 *  (d) a precise diagnostic. 
 *  
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N 4


int main( int argc, char **argv )
{
  int     bottom_stack = 2;
  int     top_stack    = 1;
  double  local_array[N] = {0};

  int     stop = ( argc > 1 ? atoi(*(argv+1)) : 0 );

  /* unrelated heap allocation; used only when an argv arg is provided */
  double *array = (double *)malloc( 3 * sizeof(double) );
  for ( int j = 0; j < stop; j++ )
    array[j] = (double)j;

  printf("addresses are:\n"
         "  top_stack   : %p\n"
         "  local_array : %p\n"
         "  bottom_stack: %p\n",
         (void*)&top_stack, (void*)local_array, (void*)&bottom_stack);

  /* THE BUG: the loop condition is i <= N rather than i < N */
  for ( int i = 0; i <= N; i++ )
    {
      printf("writing local_array item %d at %p\n", i, (void*)(local_array+i));
      local_array[i] = (double)i;
    }

  printf("new values are:\n"
         "  top_stack         : %d\n"
         "  bottom_stack      : %d\n",
         top_stack, bottom_stack);
  for ( int i = 0; i < N; i++ )
    printf("  local_array[%d] : %g\n", i, local_array[i]);

  free(array);
  return 0;
}
