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
 *  5_memory_leak.c
 *
 *  Brings a memory leak hidden in an error-handling path.
 *
 *  The bug pattern.
 *    The happy path allocates, uses, and frees correctly.
 *    The error path returns early, skipping the free.
 *    Under low error rates the leak accumulates slowly; over a long
 *    run the process eats more and more memory until the OOM-killer
 *    intervenes.
 *
 *  This mimics what may happen in production: leaks live in
 *  low-frequency code paths, survive testing (which usually exercises
 *  the happy path), and only manifest in long-running jobs at scale.
 *
 *  Sequence:
 *
 *    [1] Plain run:
 *
 *        gcc -O0 -g3 -o leak 5_memory_leak.c
 *        ./leak 0      # happy path: no leak
 *        ./leak 1      # error path: leaks BUF_SIZE * iterations bytes
 *
 *        Output looks the same either way; no symptom on a short run.
 *
 *    [2] Under AddressSanitizer + LeakSanitizer:
 *
 *        gcc -O0 -g3 -fsanitize=address -o leak 5_memory_leak.c
 *        ./leak 1
 *
 *        Result: at program exit, LSan walks the heap and reports each
 *        un-freed allocation with the full stack trace at the malloc
 *        site.  The trace points directly at the line that allocated
 *        the leaked block.
 *
 *    [3] Under Valgrind:
 *
 *        valgrind --leak-check=full --show-leak-kinds=all ./leak 1
 *
 *        Result: similar report, classified as "definitely lost" (no
 *        live pointer remains to the block) vs "possibly lost" (an
 *        interior pointer remains) vs "still reachable" (a pointer
 *        remains but cleanup did not happen).
 *
 *  Meta-lesson: leaks do not crash.  They accumulate, silently, until
 *  the long-running production job hits the memory limit.  The tooling
 *  must be told to look for them, but once told, it points at the
 *  precise allocation that escaped.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096


/* allocate, process, optionally fail, free, return
 *
 * BUG: when trigger_error != 0, the function returns without calling
 * free(buf).  The allocation is leaked on every error iteration.
 */
int process( int trigger_error )
{
  char *buf = malloc( BUF_SIZE );
  if ( buf == NULL )
    return -1;

  /* do some work that justifies the allocation */
  memset(buf, 'A', BUF_SIZE);

  if ( trigger_error )
    {
      /* THE LEAK: early return without free(buf) */
      return -2;
    }

  /* happy path */
  printf("processed %d bytes\n", BUF_SIZE);
  free(buf);
  return 0;
}


int main( int argc, char **argv )
{
  int trigger = ( argc > 1 ) ? atoi(argv[1]) : 0;

  /* call the function in a loop, to make the leak's size obvious */
  for ( int i = 0; i < 10; i++ )
    {
      int rc = process( trigger );
      if ( rc < 0 )
        {
          fprintf(stderr, "iteration %d failed: rc = %d\n", i, rc);
          /* a real program logs and keeps going; the leak quietly
           * accumulates while the service stays up.  this is the
           * common production pattern. */
        }
    }

  return 0;
}
