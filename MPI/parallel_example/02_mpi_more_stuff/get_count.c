
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
 * In this example we see how you cvan get the actual count of
 * ekements received in a message
 */


#include <stdlib.h>                                                                                                   
#include <stdio.h>
#include <time.h>
#include <mpi.h>                                                                                                      


#define TAG_DATA 1
                                                                                                                      
int main ( int argc, char **argv )                                                                                    
{
  {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
    if ( provided < MPI_THREAD_SINGLE )
      {
	// manage
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
  }

  int Me, Ntasks;                                                                                                     
                                                                                                                      
  MPI_Comm_size( MPI_COMM_WORLD, &Ntasks );                                                                           
  MPI_Comm_rank( MPI_COMM_WORLD, &Me );                                                                               

  int   Ndata = (argc > 1 ? atoi(*argv+1) : 100);
  char *data  = (char*)malloc( Ndata );

  for ( int i = 0; i < Ndata; i++ )
    data[i] = Me;
  
  if ( Me == 0 )                                                                                                      
    {                                                                                                                 
      MPI_Send( data, Ndata, MPI_BYTE, 1, TAG_DATA, MPI_COMM_WORLD );
      printf("Task %d has sent %d data: %d %d, ... %d!ì\n",
	     Me, Ndata, data[0], data[1], data[Ndata-1] );
    }                                                                                                                 
  else                                                                                                                
    {
      MPI_Status status;
      int        Nrecv;
      
      MPI_Recv( data, Ndata, MPI_BYTE, 0, TAG_DATA, MPI_COMM_WORLD, &status );
      
      MPI_Get_count ( &status, MPI_BYTE, &Nrecv );
      printf("Task %d has received %d data: %d %d, ... %d\n",
	     Me, Nrecv, data[0], data[1], data[Nrecv-1] );

    }

  free(data);
  
  MPI_Finalize();
  return 0;
}
