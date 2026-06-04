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
 *  8_signed_overflow.c
 *
 *  Demonstrates signed-integer overflow, one of the most insidious
 *  forms of undefined behaviour (UB), and the canonical example of why
 *  "UB" is not just "a runtime check the compiler skipped for you".
 *
 *  The C standard says: signed-integer overflow is UB.  The compiler
 *  is permitted to ASSUME that signed overflow never occurs, and to
 *  optimize on the basis of that assumption.  The instant your code
 *  actually overflows, the compiler's reasoning becomes inconsistent
 *  with what your code does, and the resulting binary may do anything
 *  -- including things that look insane.
 *
 *  Two demos:
 *
 *    Demo A  -- one-shot:  i = INT_MAX; i = i + 1;
 *               UBSan catches this at the instant of overflow.
 *
 *    Demo B  -- loop whose exit condition relies on wraparound.
 *               At -O0 it terminates (slowly) by relying on the
 *               two's-complement behaviour of the underlying hardware.
 *               At -O2 GCC reasons: "i starts at 1, is only incremented,
 *               cannot overflow per the standard, therefore i > 0 is
 *               always true, therefore this loop is infinite" -- and
 *               removes the exit branch entirely.  The optimized
 *               binary loops forever.
 *
 *  Pedagogical sequence:
 *
 *    [1] Demo A at -O0:
 *
 *        gcc -O0 -g3 -o sov 8_signed_overflow.c
 *        ./sov 1
 *
 *        Output: i wraps from INT_MAX to INT_MIN (typical two's
 *        complement); the program prints both values and exits.
 *
 *    [2] Demo A under UBSan:
 *
 *        gcc -O0 -g3 -fsanitize=undefined -o sov 8_signed_overflow.c
 *        ./sov 1
 *
 *        Output: runtime error report at the i = i + 1 line, with
 *        message "signed integer overflow: 2147483647 + 1 cannot be
 *        represented in type 'int'".
 *
 *    [3] Demo B at -O0:
 *
 *        gcc -O0 -g3 -o sov 8_signed_overflow.c
 *        ./sov 2
 *
 *        Output: the loop runs for roughly 2 * INT_MAX iterations
 *        (counting up to INT_MAX, wrapping to INT_MIN, then climbing
 *        back to 0).  This takes some time at -O0.
 *
 *    [4] Demo B at -O2 -- THE PUNCHLINE:
 *
 *        gcc -O2 -g3 -o sov 8_signed_overflow.c
 *        ./sov 2
 *
 *        Output: nothing.  The loop is infinite, inerrupt with Ctrl-C.
 *
 *        Why: GCC's optimizer relies on the no-UB assumption.
 *        Inspect the disassembly to see the missing exit branch:
 *
 *            objdump -d --no-show-raw-insn ./sov \
 *              | sed -n '/<demo_loop>:/,/^$/p'
 *
 *  Then, UB is not a runtime check the compiler "forgot to do".
 *  It is a "contract" that licenses to ASSUME the UB never occurs.  
 *  The optimizer uses that, aggressively.  The same source code with different
 *  optimization levels can produce qualitatively different programs.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


/* Demo A: explicit single overflow */
void demo_single( void )
{
  int i = INT_MAX;
  printf("before:  i = %d   (INT_MAX = %d)\n", i, INT_MAX);

  i = i + 1;                  /* signed overflow -- UB */

  printf("after:   i = %d\n", i);
}


/* Demo B: loop whose exit depends on signed wraparound */
void demo_loop( void )
{
  int  i;
  long count = 0;

  printf("starting loop (slow at -O0; INFINITE at -O2)...\n");
  printf("if it does not return in a few seconds, you compiled with -O2.\n");

  for ( i = 1; i > 0; i++ )
    count++;

  printf("loop exited:  i = %d   count = %ld\n", i, count);
}


int main( int argc, char **argv )
{
  int which = ( argc > 1 ) ? atoi(argv[1]) : 1;
  if ( which == 1 )
    demo_single();
  else
    demo_loop();

  return 0;
}
