
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

#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#include <mpi.h>


int main( int argc, char **argv )
{

  // ····················
  // initialize MPI
  {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if ( provided < MPI_THREAD_FUNNELED )
      {
	// manage with some meaningful message
	// free every memory block that you may
	// already allocated
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
  }

  int      rank[2], size[2];
  MPI_Comm SMALL;
  MPI_Comm_size( MPI_COMM_WORLD, &size[0] );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank[0] );


  // create two communicators for even and odd ranks
  //
  MPI_Comm_split( MPI_COMM_WORLD, (rank[0]%2), rank[0], &SMALL);
  // get the size of the new world
  MPI_Comm_size( SMALL, &size[1] );
  // get my rank in the new world
  MPI_Comm_rank( SMALL, &rank[1] );

  printf("[0] Task %d in WORLD of size %d has rank %d in SMALL WORLD of size %d\n",
	 rank[0], size[0], rank[1], size[1] );


  // just a synch and two empty lines
  //
  fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);
  if( rank[0] == 0 )
    printf("\n\n");
  fflush(stdout);  
  MPI_Barrier(MPI_COMM_WORLD);

  // --------------------------------

  
  int myworld = 0;
  if( rank[1] == 0 )
    myworld = (rank[1] != rank[0]);
  MPI_Bcast( &myworld, 1, MPI_INT, 0, SMALL);

  MPI_Barrier(MPI_COMM_WORLD);
  printf("[1] Task %d : my world is the nr %d\n", rank[0], myworld );


  int value;
  MPI_Request req;
  if( rank[1] == 0 )
    value = (1+rank[0])*100;
  MPI_Ibcast( &value, 1, MPI_INT, 0, SMALL, &req);

  MPI_Wait( &req, MPI_STATUS_IGNORE);
  printf("[2] W %d Task %d : %d\n", myworld, rank[1], value );

 #define Nreq 5
  
  MPI_Request *Bcast_req;
  Bcast_req = (MPI_Request*)calloc( Nreq, sizeof(MPI_Request));
  for( int j = 0; j < Nreq; j++ ) Bcast_req[j] = MPI_REQUEST_NULL;

  int values[Nreq] = {0};
  for( int j = 0; j < Nreq; j++ ) {
    if( rank[1] == 0 )
      values[j] = myworld*10 + j;
    MPI_Ibcast( &values[j], 1, MPI_INT, 0, SMALL, &Bcast_req[j] ); }

  for( int j = 0; j < Nreq; j++ ) {
    MPI_Wait( &Bcast_req[j], MPI_STATUS_IGNORE);
    if( rank[1] > 0 )
      printf("[3] W %d Task %d : BC arrived with value %d\n",
	     myworld, rank[1], values[j] ); }
  
  free( Bcast_req);
  MPI_Finalize();

  return 0;
}
