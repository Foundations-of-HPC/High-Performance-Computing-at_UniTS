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
 *  3_use_after_free.c
 *
 *  Demonstrates a use-after-free: dereferencing a pointer after the
 *  memory it points to has been freed.
 *
 *  The bug pattern.
 *    Two pointers refer to the same allocation (one "primary" pointer
 *    and one "stale" copy taken before the free).  The primary frees
 *    the allocation.  The stale pointer is then dereferenced; the
 *    memory has been returned to the allocator but its bytes are
 *    typically still readable.  What you get back depends on what
 *    the allocator has done with the block since.
 *
 *
 *    [1] Without sanitizers:
 *
 *        gcc -O0 -g3 -o uaf 3_use_after_free.c
 *        ./uaf
 *
 *        Result: the program usually appears to work.  The freed
 *        block is often still intact when read shortly after free();
 *        the output looks plausible.  THIS IS THE DANGEROUS CASE:
 *        a real bug that produces no symptom.
 *
 *    [2] Under AddressSanitizer:
 *
 *        gcc -O0 -g3 -fsanitize=address -o uaf 3_use_after_free.c
 *        ./uaf
 *
 *        Result: ASan stops at the dereference, reports the allocation
 *        site, the free site, and the use site -- the full life cycle
 *        of the buggy access.
 *
 *  Meta-lesson: silence is not correctness.  Tooling that detects
 *  use-after-free instruments allocator events; the runtime knows
 *  which addresses are currently "live" and traps access to any that
 *  are not.  Without that instrumentation, the bug is invisible until
 *  the allocator happens to reuse the block, at which point the
 *  failure mode is non-deterministic and orders of magnitude harder
 *  to diagnose.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Node {
  int           id;
  char          label[16];
  struct Node  *next;
} Node;


Node *make_node( int id, const char *label )
{
  Node *n = malloc( sizeof(Node) );
  if ( n == NULL ) { perror("malloc"); exit(EXIT_FAILURE); }

  n->id = id;
  strncpy(n->label, label, sizeof(n->label) - 1);
  n->label[sizeof(n->label) - 1] = '\0';
  n->next = NULL;
  return n;
}


int main( void )
{
  /* build a 2-node list */
  Node *head = make_node( 1, "alpha" );
  head->next = make_node( 2, "beta"  );

  /* keep a SECOND pointer to the second node (the "stale" alias) */
  Node *stale = head->next;

  printf("before free:\n");
  printf("  stale points to id=%d label='%s'\n", stale->id, stale->label);

  /* free the whole list through the head */
  free(head->next);
  free(head);

  /* THE BUG: dereference the stale pointer.
   * The bytes are usually still readable; the bug is silent at -O0
   * without sanitizers. */
  printf("\nafter free (dereferencing stale pointer):\n");
  printf("  stale->id    = %d\n",   stale->id);
  printf("  stale->label = '%s'\n", stale->label);

  return 0;
}
