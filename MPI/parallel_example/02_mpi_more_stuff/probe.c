
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

#define TAG_DATA 1
                                                                                                                      
int main ( int argc, char **argv )                                                                                    
{
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
  if ( provided < MPI_THREAD_SINGLE )
    {
      // manage
      MPI_Abort();
    }
                                                                                                                      
  int Me, Ntasks;                                                                                                     
                                                                                                                      
  MPI_Comm_size( MPI_COMM_WORLD, &Ntasks );                                                                           
  MPI_Comm_rank( MPI_COMM_WORLD, &Me );                                                                               
                                                                                                                      
  char myname[10], myname2[10];                                                                                       
                                                                                                                      
  if ( Me == 0 )                                                                                                      
    {                                                                                                                 
      srand(time(NULL));
      int N     = rand() % 769;
      int *data = (int*)malloc( sizeof(int) * N);
      for ( int i = 0; i < N; i++ )
	data[i] = rand();

      MPI_Send( data, N, MPI_INT, 1, TAG_DATA, MPI_COMM_WORLD );
      printf("Task %d has sent %d data: %d %d, ... %d!ì\n",
	     Me, N, data[0], data[1], data[N-1] );

      free( data );
    }                                                                                                                 
  else                                                                                                                
    {
      MPI_Status status;
      int Sender;
      int Tag;
      int N;
      
      MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
      MPI_Get_count ( &status, MPI_INT, &N );

      int *local = (int*)malloc( sizeof(int) * N );
      MPI_Recv( local, N, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status );

      // is that correct to access the data array before knowing exactly
      // how many data have been actually received ?
      //
      printf("Task %d has received %d data: %d %d, ... %d\n",
	     Me, N, local[0], local[1], local[N-1] );

      free( local );      
      
    }

  MPI_Finalize();
  return 0;
}
