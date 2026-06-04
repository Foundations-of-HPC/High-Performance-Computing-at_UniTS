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
 *  1_heap_overflow_delayed.c
 *
 *  Demonstrates a heap-overflow bug whose CRASH HAPPENS FAR FROM THE BUG.
 *  This is a classic demo for why you need more than just a stack
 *  trace: the trace tells you where the program died, not where the
 *  damage was done.
 *
 *  The bug.
 *    The program allocates a fixed-size buffer for each user's name.
 *    One of the names provided exceeds the buffer size.  strcpy writes
 *    past the end of the buffer and corrupts the metadata of the
 *    adjacent heap chunk.  The program continues running -- the
 *    corruption is silent.  The crash arrives later, when free() is
 *    called on a different, valid pointer, and glibc's malloc detects
 *    the corrupted metadata.
 *
 *    BTW, that is why we should use strncpy instead of strcpy *ALWAYS*
 *
 *  Sequence 
 *
 *    [1]  bare GDB
 *        gcc -O0 -g3 -o heap_delayed 1_heap_overflow_delayed.c
 *        gdb ./heap_delayed
 *        (gdb) run
 *        (gdb) bt
 *
 *        Result: crash inside free() / malloc internals; the backtrace
 *        points at the innocent line that called free(), not at the
 *        strcpy that did the damage.  The trace is correct but useless.
 *
 *    [2]  Valgrind
 *        valgrind --leak-check=full ./heap_delayed
 *
 *        Result: "Invalid write of size N" pointing directly at the
 *        strcpy line.  Slow (~30x), but no recompilation needed and
 *        the diagnostic is precise.
 *
 *    [3]  AddressSanitizer
 *        gcc -O0 -g3 -fsanitize=address -o heap_delayed 1_heap_overflow_delayed.c
 *        ./heap_delayed
 *
 *        Result: same diagnostic as Valgrind, ~2x slowdown, runs on
 *        the optimized binary, and the report includes the exact byte
 *        offset, the size of the destination buffer, and the call stack
 *        at the strcpy.  Fastest path to the truth for this class of bug.
 *
 *  Meta-lesson: the location of a crash is not the location of the bug.
 *  Heap corruption can sit silent for arbitrarily long before being
 *  detected; the tooling that finds it (Valgrind, ASan) instruments
 *  memory accesses to catch the WRITE itself, not the consequence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_BUFFER_SIZE 16
#define NUM_USERS        3


typedef struct {
  int   id;
  char *name;
} User;


/* allocate a name buffer and copy the given string into it.
 *
 * BUG: no bounds check.  If `name` is longer than NAME_BUFFER_SIZE - 1,
 * strcpy writes past the end of the malloc'd block and corrupts the
 * adjacent heap-chunk metadata.
 */
void set_user_name( User *user, const char *name )
{
  user->name = (char *)malloc( NAME_BUFFER_SIZE );
  if ( user->name == NULL )
    { perror("malloc"); exit(EXIT_FAILURE); }

  strcpy(user->name, name);                  /* unbounded -- the bug */
  printf("assigned name '%s' to user %d.\n", name, user->id);
}


void print_user_report( User **users, int count )
{
  printf("\n--- user report ---\n");
  for ( int i = 0; i < count; i++ )
    if ( users[i] != NULL )
      printf("  user id: %d   name: %s\n", users[i]->id, users[i]->name);
  printf("-------------------\n\n");
}


int main( void )
{
  User       *user_database[NUM_USERS];
  const char *names[] = { "Alice", "ThatIsAVeryLongName", "Charles" };
  /*                              ^^^^^^^^^^^^^^^^^  19 chars + NUL = 20 bytes
   *                              this one overflows the 16-byte buffer */

  printf("initialising user database...\n");

  /* allocation phase */
  for ( int i = 0; i < NUM_USERS; i++ )
    {
      user_database[i] = (User *)malloc( sizeof(User) );
      if ( user_database[i] == NULL )
        { perror("malloc"); return EXIT_FAILURE; }

      user_database[i]->id = 100 + i;
      /* the overflow happens here, on iteration i = 1 */
      set_user_name( user_database[i], names[i] );
    }

  /* at this point the heap is already corrupted, but the program is fine */
  printf("\ndatabase initialised. everything seems OK.\n");
  print_user_report(user_database, NUM_USERS);

  /* cleanup -- where the crash actually arrives */
  printf("deallocating memory...\n");
  for ( int i = 0; i < NUM_USERS; i++ )
    {
      if ( user_database[i] == NULL ) continue;

      printf("freeing name for user %d ('%s')...\n",
             user_database[i]->id, user_database[i]->name);
      free(user_database[i]->name);

      printf("freeing struct for user %d...\n", user_database[i]->id);
      /* the crash typically arrives on i = 2:  free() walks into a
       * chunk whose header was overwritten by the strcpy above. */
      free(user_database[i]);
    }

  printf("done.\n");      /* rarely reached */
  return EXIT_SUCCESS;
}
