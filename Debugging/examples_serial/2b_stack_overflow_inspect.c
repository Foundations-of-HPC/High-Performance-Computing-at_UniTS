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
 *  2b_stack_overflow_inspect.c
 *
 *  The same recursive sum as 2_stack_overflow.c, instrumented to print
 *  the current frame pointer at each call.  Used to demonstrate that
 *  with -O3, GCC may convert the recursion into iteration via tail-call
 *  optimization, in which case the frame pointer stays constant and the
 *  stack overflow disappears.
 *
 *  The frame address is obtained portably with __builtin_frame_address(0)
 *  rather than via inline assembly.  This works on any GCC/Clang target,
 *  but the value is only meaningful when the compiler has not optimized
 *  the function into a form without a stack frame (compile with -fno-omit-frame-pointer) .
 *
 *  Workflow:
 *
 *    [1] At -O0, observe descending frame pointers:
 *
 *        gcc -O0 -g3 -fno-omit-frame-pointer -o inspect 2b_stack_overflow_inspect.c
 *        ./inspect 1000  |  head -20
 *
 *        Each line shows BP about 32 or 48 bytes below the previous
 *        (the stack frame size of recursive_sum).
 *
 *    [2] At -O3, observe a CONSTANT frame pointer:
 *
 *        gcc -O3 -g3 -fno-omit-frame-pointer -o inspect 2b_stack_overflow_inspect.c
 *        ./inspect 1000  |  head -20
 *
 *        Each line shows the same BP, because GCC has folded the
 *        recursion into a loop (tail-call elimination), and the
 *        program now uses one stack frame regardless of array size.
 *
 *    [3] To see the transformation in the generated code, examine the
 *        disassembly of recursive_sum:
 *
 *        objdump -d --no-show-raw-insn ./inspect | sed -n '/<recursive_sum>:/,/^$/p'
 *
 *        At -O0 you will see a `call` instruction; at -O3 you will see
 *        a backward `jmp` (or a fall-through loop with `add` and `dec`).
 *
 *  Meta-lesson: optimization is not just "the same code, faster" -- the
 *  optimizer can change the asymptotic behaviour of a program.  Tail-call
 *  elimination changes a recursive function from O(n) stack to O(1)
 *  stack.  Whether you can rely on this depends on the compiler, the
 *  call pattern (only tail calls can be eliminated), and the flags.
 *
 *  -fno-omit-frame-pointer is included above because we want the frame
 *  pointer to exist; otherwise __builtin_frame_address(0) may return an
 *  uninformative value.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE 1000


long long recursive_sum( int *array, size_t count )
{
  /* portable replacement for the inline-asm trick of inspecting RBP */
  void *bp = __builtin_frame_address(0);

  if ( count == 0 )
    return 0;

  printf("\tBP = %p   count = %zu   element = %d\n",
         bp, count, array[0]);

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

  printf("\nsum = %lld\n", sum);

  free(numbers);
  return EXIT_SUCCESS;
}
