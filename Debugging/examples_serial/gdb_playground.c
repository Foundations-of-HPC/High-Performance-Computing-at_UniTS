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
 *     This code is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>
 * =============================================================================
 *
 *  gdb_playground.c
 *
 *  A small linked-list sandbox for practicing GDB on a clean serial program.
 *  THREE bugs of increasing subtlety have been deliberately planted in the
 *  code below.  Each illustrates a different debugging technique:
 *
 *     Bug 1  --  inspect-and-step              (basic print/next)
 *     Bug 2  --  watchpoint on a loop counter
 *     Bug 3  --  backtrace through recursion
 *
 *  See EXERCISES.md for the guided walk-through.
 *
 *  BUILD:    gcc -O0 -g3 -Wall -Wextra -o gdb_playground gdb_playground.c
 *
 *  USAGE:    ./gdb_playground value1 value2 ...
 *
 *  EXAMPLES (each is targeted in EXERCISES.md):
 *
 *     ./gdb_playground 1.0 2.0 3.0                   (sanity check)
 *     ./gdb_playground 1.0 2.0 3.0 4.0 5.0           (triggers bug 1)
 *     ./gdb_playground 1.0 2.0 3.0 4.0 5.0 6.0       (triggers bug 2)
 *     ./gdb_playground -1.0 -2.0 -3.0                (triggers bug 3)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Node
{
  int            id;
  double         value;
  struct Node   *next;
} Node;


/* a global useful for practicing watchpoints later in the lecture */
static int g_node_count = 0;


/* allocate one node and bump the global counter */
Node *create_node( int id, double value )
{
  Node *n = malloc( sizeof(Node) );
  if ( n == NULL )
    { perror("malloc"); exit(EXIT_FAILURE); }

  n->id    = id;
  n->value = value;
  n->next  = NULL;
  g_node_count++;
  return n;
}


/* insert into a sorted linked list (ascending by value);
 * maintains  head->value <= head->next->value <= ... */
Node *insert_sorted( Node *head, Node *newnode )
{
  if ( head == NULL )
    return newnode;

  if ( newnode->value < head->value )
    {
      newnode->next = head;
      return newnode;
    }

  Node *curr = head;
  while ( curr->next != NULL && curr->next->value < newnode->value )
    curr = curr->next;

  newnode->next = curr->next;
  curr->next    = newnode;
  return head;
}


/* print the list in order */
void print_list( const Node *head )
{
  int idx = 0;
  for ( const Node *c = head; c != NULL; c = c->next )
    printf("  [%2d]  id=%-3d  value=%.3f\n", idx++, c->id, c->value);
}


/* return the k-th node (0-indexed) of the list, or NULL if k is out of range
 *
 * BUG #2 LIVES HERE -- see EXERCISES.md, exercise 2
 */
Node *find_kth( Node *head, int k )
{
  Node *curr = head;
  while ( --k > 0 && curr != NULL )
    curr = curr->next;
  return curr;
}


/* count the nodes whose value lies in the closed interval [lo, hi]
 *
 * BUG #1 LIVES HERE -- see EXERCISES.md, exercise 1
 */
int count_in_range( const Node *head, double lo, double hi )
{
  int count = 0;
  for ( const Node *c = head; c != NULL; c = c->next )
    if ( c->value >= lo && c->value < hi )
      count++;
  return count;
}


/* compute the maximum value in the list, recursively
 *
 * BUG #3 LIVES HERE -- see EXERCISES.md, exercise 3
 */
double find_max_recursive( const Node *head )
{
  if ( head == NULL )
    return 0.0;

  double rest_max = find_max_recursive( head->next );
  return ( head->value > rest_max ) ? head->value : rest_max;
}


/* sum the values, recursively (no bug here -- used as a control) */
double sum_recursive( const Node *head )
{
  if ( head == NULL )
    return 0.0;
  return head->value + sum_recursive( head->next );
}


/* free the whole list */
void free_list( Node *head )
{
  while ( head != NULL )
    {
      Node *next = head->next;
      free(head);
      head = next;
    }
}


int main( int argc, char **argv )
{
  if ( argc < 2 )
    {
      fprintf(stderr,
              "usage:  %s value1 [value2 ...]\n"
              "  build a sorted linked list of the given doubles,\n"
              "  then print summary statistics.\n",
              argv[0]);
      return EXIT_FAILURE;
    }

  /* build the list */
  Node *head = NULL;
  for ( int i = 1; i < argc; i++ )
    {
      double v = atof( argv[i] );
      Node  *n = create_node( i, v );
      head     = insert_sorted( head, n );
    }

  printf("\nBuilt a list of %d nodes:\n", g_node_count);
  print_list(head);

  /* statistics */
  double s = sum_recursive( head );
  double m = find_max_recursive( head );
  printf("\nsum = %.3f\nmax = %.3f\n", s, m);

  /* k-th element (0-indexed), if available */
  if ( g_node_count >= 3 )
    {
      int    k     = 2;
      Node  *third = find_kth( head, k );
      if ( third != NULL )
        printf("k-th element (k=%d): id=%d  value=%.3f\n",
               k, third->id, third->value);
    }

  /* count in a hard-coded test range */
  double lo = 0.0, hi = 5.0;
  int in_range = count_in_range( head, lo, hi );
  printf("nodes in [%.1f, %.1f]: %d\n", lo, hi, in_range);

  free_list(head);
  return EXIT_SUCCESS;
}
