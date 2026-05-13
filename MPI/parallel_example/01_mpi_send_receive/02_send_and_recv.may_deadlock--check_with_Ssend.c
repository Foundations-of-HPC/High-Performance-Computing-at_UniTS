
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



#include <stdlib.h>                                                                                                   
#include <stdio.h>
#include <time.h>
#include <mpi.h>                                                                                                      

#define DATA_FIRST 1
#define DATA_SECND 2
#define NAME       3 
#define SURNAME    4

int main ( int argc, char **argv )                                                                                    
{                                                                                                                     
  MPI_Init(&argc, &argv);                                                                                             
                                                                                                                      
  int Me, Ntasks, use_ssend;

  use_ssend = (argc > 1 ? atoi(*(argv+1)) : 0);
  if ( use_ssend )
    printf("Using MPI_Ssend\n");
	       
  MPI_Comm_size( MPI_COMM_WORLD, &Ntasks );
  MPI_Comm_rank( MPI_COMM_WORLD, &Me );

  if ( Me < 2 )
    {

      // -----------------------------------------------------
      //
      //  This first exchange is formally a deadlock
      //  However, it may run smoothly due to the
      //  system's bufferization.
      //  However, as the data size grows there will be
      //  the moment at which the calls will deadlock
      //
 
      
      int N      = ( argc > 1 ? atoi(*(argv+1)) : 1024 );
      char *data = (char*)malloc( 2*N );
      char *data_to_send = data;
      char *data_to_recv = data + N;
      for ( int i = 0; i < N; i++ )
	data[i] = Me;
      
      if ( Me == 0 )
	{
	  if ( use_ssend)
	    MPI_Ssend( data_to_send, N, MPI_BYTE, !Me, DATA_FIRST, MPI_COMM_WORLD );
	  else
	    MPI_Send( data_to_send, N, MPI_BYTE, !Me, DATA_FIRST, MPI_COMM_WORLD );
	  MPI_Recv( data_to_recv, N, MPI_BYTE, !Me, DATA_SECND, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
	}
      else
	{
	  if ( use_ssend)
	    MPI_Ssend( data_to_send, N, MPI_BYTE, !Me, DATA_FIRST, MPI_COMM_WORLD );
	  else	    
	    MPI_Send( data_to_send, N, MPI_BYTE, !Me, DATA_FIRST, MPI_COMM_WORLD );
	  MPI_Recv( data_to_recv, N, MPI_BYTE, !Me, DATA_SECND, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
	}
      
      free( data );
      

      // -----------------------------------------------------
      //
      //  This second exchange also is formally a deadlock
      //  due to the mismatch in the tags.
      //  However, it may run smoothly due to the
      //  system's bufferization.
      //  Using MPI_Ssend instead of MPI_Send, the following
      //  calls will deadlock
      //

      
      char myname[10], myname2[10];                                                          
      
      if ( Me == 0 )                                                                                                      
	{
	  if ( use_ssend) {
	    MPI_Ssend( myname, 100, MPI_BYTE, 1, NAME, MPI_COMM_WORLD );
	    MPI_Ssend( myname2, 100, MPI_BYTE, SURNAME, 0, MPI_COMM_WORLD ); }

	  else {
	    MPI_Send( myname, 100, MPI_BYTE, 1, NAME, MPI_COMM_WORLD );	    
	    MPI_Send( myname2, 100, MPI_BYTE, SURNAME, 0, MPI_COMM_WORLD ); }

	}                                                                                                                 
      else                                                                                                                
	{
	  MPI_Recv( myname2, 100, MPI_BYTE, 0, SURNAME, MPI_COMM_WORLD,
		    MPI_STATUS_IGNORE );	  
	  MPI_Recv( myname, 100, MPI_BYTE, 0, NAME, MPI_COMM_WORLD,
		    MPI_STATUS_IGNORE );
	}

      // ----------------------------------------------

    }

  // all the MPI tasks beyond 0 and 1 will be just waiting here
  //
  MPI_Barrier(MPI_COMM_WORLD);
  
  MPI_Finalize();
  return 0;
}
