
/* ────────────────────────────────────────────────────────────────────────── *
│                                                                            │
│ This file is part of the exercises for the Lectures on                     │
│   "Foundations of High Performance Computing"                              │
│ given at                                                                   │
│   Master in HPC and                                                        │
│   Master in Data Science and Scientific Computing                          │
│ @ SISSA, ICTP and University of Trieste                                    │
│                                                                            │
│ contact: luca.tornatore@inaf.it                                            │
│                                                                            │
│     This is free software; you can redistribute it and/or modify           │
│     it under the terms of the GNU General Public License as published by   │
│     the Free Software Foundation; either version 3 of the License, or      │
│     (at your option) any later version.                                    │
│     This code is distributed in the hope that it will be useful,           │
│     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
│     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
│     GNU General Public License for more details.                           │
│                                                                            │
│     You should have received a copy of the GNU General Public License      │
│     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
│                                                                            │
* ────────────────────────────────────────────────────────────────────────── */

/*
 * Simple demonstration of Send and Recv
*  that will deadlock
 * ··········································
 *
 * Even Ranks send a message to subsequent odd ranks,
 * and receive a reply
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>


int main ( int argc, char **argv )
{

  char hostname[MPI_MAX_PROCESSOR_NAME];
  gethostname( &hostname[0], MPI_MAX_PROCESSOR_NAME );

  printf( "Rank %d is running on host %s\n", Rank, hostname );

  MPI_Barrier(MPI_COMM_WORLD);

  // ············································
  // determines who's my buddy
  
  int Im_even = (Rank % 2 == 0);
  int my_buddy;
  if ( Im_even )
    my_buddy = Rank+1;
  else
    my_buddy = Rank-1;


  // ············································
  // if I do have a buddy..
  if ( my_buddy < Ntasks )
    {

      #define FIRST_ROUND 0
      #define SECOND_ROUND 1

      /*
       * here it is guaranteed that we end in a deadlock, because the MPI_Recv is
       * _blocking_ in the sense that it will never return before the recection
       * of the message has ended.
       */

      
      if ( Im_even )
	{
	  int        buddy_rank;
	  MPI_Status status;

	  MPI_Recv( &buddy_rank, 1, MPI_INT, my_buddy, SECOND_ROUND, MPI_COMM_WORLD, &status );
	  MPI_Send( &Rank, 1, MPI_INT, my_buddy, FIRST_ROUND, MPI_COMM_WORLD );	  

	  if ( buddy_rank == my_buddy )
	    printf("\tRank %d says: I confirm I have received a reply from my buddy\n", Rank);
	  else
	    printf("oops, a stranger has replied to me\n");	  
	}
      else
	{
	  int        buddy_rank;
	  MPI_Status status;

	  MPI_Recv( &buddy_rank, 1, MPI_INT, my_buddy, FIRST_ROUND, MPI_COMM_WORLD, &status );
	  MPI_Send( &Rank, 1, MPI_INT, my_buddy, SECOND_ROUND, MPI_COMM_WORLD );
	  

	  if ( buddy_rank == my_buddy )
	    printf("Rank %d says: I confirm I have received the message from my buddy\n", Rank);
	  else
	    printf("oops, a stranger has communicated with me\n");	  
	}
      
    }
  else
    // ·······················
    // sadness
    printf("Rank %d has got nobody to talk with\n", Rank );
    

  MPI_Finalize();

  return 0;
}
