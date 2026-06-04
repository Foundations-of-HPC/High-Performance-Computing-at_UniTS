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
 *  4_uninitialized_read.c
 *
 *  Demonstrates a read of an uninitialized local variable.
 *
 *  The bug pattern.
 *    A local is declared without an initializer.  Code that follows
 *    assigns it on some code paths but not all; a later read may
 *    therefore observe whatever the stack happened to contain when
 *    the function was entered.
 *
 *  Sequence:
 *
 *    [1] Plain run, with the "all-paths-assign" input:
 *
 *        gcc -O0 -g3 -o uninit 4_uninitialized_read.c
 *        ./uninit 1
 *
 *        Output: result = 10.  Program appears correct.
 *
 *    [2] Plain run, with the "missing-path" input:
 *
 *        ./uninit 0
 *
 *        Output: result = (garbage, often 0 at -O0).  The output
 *        looks plausible.  Without tooling there is no symptom.
 *
 *    [3] At -O2:
 *
 *        gcc -O2 -g3 -o uninit 4_uninitialized_read.c
 *        ./uninit 0
 *
 *        Output: result = arbitrary register contents.  At -O2 the
 *        local lives in a register that may contain anything; the
 *        "accidental zero" at -O0 is gone.
 *
 *    [4] Under Valgrind:
 *
 *        valgrind ./uninit 0
 *
 *        Result: "Conditional jump or move depends on uninitialised
 *        value(s)" with file:line of the use, plus a traceback to the
 *        function where the uninitialized local was declared.
 *
 *    [5] Under MemorySanitizer (Clang only -- gcc does not implement MSan):
 *
 *        clang -O0 -g3 -fsanitize=memory -fno-omit-frame-pointer \
 *              -o uninit 4_uninitialized_read.c
 *        ./uninit 0
 *
 *        Result: an immediate report with both the allocation site
 *        of the local and the use site.  MSan is the most precise
 *        tool for this class of bug, but requires Clang and an
 *        MSan-instrumented libc to avoid false positives.
 *
 *  Meta-lesson: an uninitialized read is UB.  Whether the program
 *  appears correct depends on what was on the stack -- which depends
 *  on the calling context, the optimization level, the compiler,
 *  and the input data.  "Works in testing" means nothing.
 */

#include <stdio.h>
#include <stdlib.h>


/* compute_offset is supposed to return a positive offset based on a flag.
 *
 * The bug: when flag == 0, `offset` is never assigned.  The function
 * returns whatever happened to be in the local's storage. */
int compute_offset( int flag )
{
  int offset;       /* <-- no initializer */

  if ( flag != 0 )
    offset = flag * 10;
  /* else branch is missing entirely; offset has no defined value */

  return offset;
}


int main( int argc, char **argv )
{
  int flag = ( argc > 1 ) ? atoi(argv[1]) : 1;

  int result = compute_offset( flag );

  printf("flag   = %d\n", flag);
  printf("result = %d\n", result);

  /* the bug becomes louder if we branch on the value */
  if ( result > 0 )
    printf("positive offset\n");
  else
    printf("non-positive offset\n");

  return 0;
}
