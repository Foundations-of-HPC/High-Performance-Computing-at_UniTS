
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
 * Simple demonstration of Send and Recv that may block
 * ··········································
 *
 * Even Ranks send a message to subsequent odd ranks,
 * and receive a reply.
 *
 * Due to wrong ordering of MPI calls, the code m,ay hang
 * depending on implementation details and the message size
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>


int main ( int argc, char **argv )
{
  
  char hostname[MPI_MAX_PROCESSOR_NAME];
  gethostname( hostname, MPI_MAX_PROCESSOR_NAME );

  // ············································
  // initialize MPI
  int  Rank;
  int  Ntasks;
  int  MsgSize;

  {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
    if ( provided < MPI_THREAD_SINGLE )
      {
	// manage
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
  }

  MPI_Comm_rank( MPI_COMM_WORLD, &Rank );
  MPI_Comm_size( MPI_COMM_WORLD, &Ntasks );

  MsgSize = (argc > 1 ? atoi(*(argv+1)) : 1 );    

  // ············································
  // determines who's my buddy

  
  int Im_even = (Rank % 2 == 0);
  int my_buddy;
  if ( Im_even )
    my_buddy = Rank+1;
  else
    my_buddy = Rank-1;

  printf("Rank %d has buddy %d, message size is %d\n", Rank, my_buddy, MsgSize);

  MPI_Barrier(MPI_COMM_WORLD);

  // ············································
  // communicate

  
  if ( my_buddy < Ntasks )
    {

      #define FIRST_ROUND 0
      #define SECOND_ROUND 1

      /*
       * here it is highly possible (not guaranteed) that we end in a deadlock,
       * because the MPI_Send are posted concurrently.
       * The MPI specifications leave to the implementer to decide whether
       * MPI_Send is _blocking_ in the sense that it will never return before the
       * reception signal has been posted OR before the availability of the
       * local sent buffer.
       * The point is that the beheviour may depends on the SIZE of the message.
       * For short-enough messages (i.e. few KB), the data are sent immediately
       * and some caching happens at receiver-size if the corresponding Recv
       * (i.e. the dedicated memory space has not been guaranteed) has not been
       * posted yet.
       * That is called "eager" protocal.
       * For larger messages, a "rendezvous" protocal may be adopted: only the
       * requests for sending are posted, and the actual send happens when the
       * corresponding Recv is issued.
       * NOTE: what is "small" and  "large" is implementation dependent.
       *
       */

      
      if ( Im_even )
	{
	  /*
	   * ------------------------------------------------
	   *  send the message
	   */
	  int       *message = (int*)malloc( MsgSize * sizeof(int) );
	  for( int  j = 0; j < MsgSize; j++ )
	    message[j] = Rank;
	  
	  MPI_Send( (void*)message, MsgSize, MPI_INT, my_buddy, FIRST_ROUND, MPI_COMM_WORLD );	  

	  /*
	   * ------------------------------------------------
	   *  get the answer
	   */
	  
	  int        buddy_rank;
	  MPI_Status status;

	  MPI_Recv( (void*)&buddy_rank, 1, MPI_INT, my_buddy, SECOND_ROUND, MPI_COMM_WORLD, &status );

	  if ( buddy_rank == my_buddy )
	    printf("[R] Rank %d says: I confirm I have received a reply from my buddy\n", Rank);
	  else
	    printf("\t[R] Rank %d says: oops, a stranger (id: %d, expected: %d) has replied to me\n",
		   Rank, buddy_rank, my_buddy);

	  free(message);
	}
      else
	{

	  /*
	   * ------------------------------------------------
	   *  send the answer first
	   *  that may NOT result in a deadlock; however, it
	   *  is NOT guaranteed that on every system/implementation
	   *  you get through seamlessly
	   *
	   */
	  MPI_Send( (void*)&Rank, 1, MPI_INT, my_buddy, SECOND_ROUND, MPI_COMM_WORLD );	  
	  
	  /*
	   * ------------------------------------------------
	   *  get the message
	   */
	  
	  MPI_Status status;	  
	  int       *message = (int*)malloc( MsgSize * sizeof(int) );
	  
	  MPI_Recv( (void*)message, MsgSize, MPI_INT, my_buddy, FIRST_ROUND, MPI_COMM_WORLD, &status );
	  
	  if ( message[0] == my_buddy )
	    printf("Rank %d says: I confirm I have received the message from my buddy\n", Rank);
	  else
	    printf("Rank %d says: oops, a stranger (id: %d, expected: %d) has communicated with me\n",
		   Rank, message[0], my_buddy);

	  free(message);
	}
      
    }
  else
    printf("Rank %d has got nobody to talk with\n", Rank );
    

  MPI_Finalize();

  return 0;
}
