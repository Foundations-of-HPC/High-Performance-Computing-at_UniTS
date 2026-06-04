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
 *  6_double_free.c
 *
 *  Demonstrates a double-free: the same allocation is passed to free()
 *  twice.
 *
 *  The bug pattern.
 *    Two pieces of cleanup code both think they own a buffer.  The
 *    first frees it correctly; the second sees a non-NULL pointer
 *    (because the first did not nullify the field after free) and
 *    frees again.
 *
 *  Sequence:
 *
 *    [1] Plain run:
 *
 *        gcc -O0 -g3 -o df 6_double_free.c
 *        ./df
 *
 *        Result on modern glibc: the C library detects the double-free
 *        and aborts the program with
 *
 *           free(): double free detected in tcache 2
 *           Aborted (core dumped)
 *
 *        Diagnostic value: limited.  You know there is a double-free;
 *        you do not know where the two frees are.
 *
 *    [2] Under AddressSanitizer:
 *
 *        gcc -O0 -g3 -fsanitize=address -o df 6_double_free.c
 *        ./df
 *
 *        Result: ASan reports both free sites with file:line, plus the
 *        original allocation site.  All three pieces of information,
 *        with stack traces.
 *
 *  So, glibc's own malloc has been progressively hardened
 *  since the early 2000s (the tcache check is from ~2017) and detects
 *  many common heap-corruption patterns.  But the error messages are
 *  terse and not actionable; you know a double-free happened, not
 *  where.  ASan turns the same detection event into an actionable
 *  report with three stack traces.
 *
 *  BEST PRACTICE:
 *  doing "p = NULL" immediately after "free(p)" makes the second free a
 *  no-op (since free(NULL) is valid).  That is the cheap structural
 *  fix; the cleaner fix is establishing clear ownership for every
 *  allocation in the program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Buffer {
  char    *data;
  size_t   size;
} Buffer;


Buffer make_buffer( size_t size )
{
  Buffer b;
  b.data = malloc( size );
  b.size = size;
  return b;
}


/* the primary cleanup -- frees but DOES NOT NULLIFY the field */
void cleanup_primary( Buffer *b )
{
  if ( b->data != NULL )
    {
      free(b->data);
      /* deliberate omission: should also set b->data = NULL.
       * leaving the pointer non-NULL is what enables the double-free
       * in cleanup_secondary below. */
    }
}


/* a secondary cleanup, written by a different developer who assumed
 * the previous cleanup had nullified the pointer */
void cleanup_secondary( Buffer *b )
{
  if ( b->data != NULL )      /* still non-NULL: previous cleanup left a stale ptr */
    free(b->data);            /* DOUBLE FREE */
}


int main( void )
{
  Buffer b = make_buffer( 64 );
  if ( b.data == NULL ) { perror("malloc"); return EXIT_FAILURE; }

  memset(b.data, 'X', b.size);
  printf("buffer allocated at %p (size = %zu)\n", (void *)b.data, b.size);

  /* both cleanups run, in sequence -- the second is the double-free */
  cleanup_primary( &b );
  cleanup_secondary( &b );

  printf("done.\n");         /* usually unreachable: glibc aborts before here */
  return 0;
}
