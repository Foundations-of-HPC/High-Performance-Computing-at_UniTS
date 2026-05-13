
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
#include <string.h>
#include "mpi.h"

#define COUNTS 100


int main(int argc,char **argv)

{
  // ····················
  // Initialize MPI
  
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
  if ( provided < MPI_THREAD_FUNNELED )
    {
      // manage
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

  int Myrank, Ntasks;
  MPI_Comm myCOMM_WORLD;

  MPI_Comm_dup ( MPI_COMM_WORLD, &myCOMM_WORLD );
  MPI_Comm_size( myCOMM_WORLD, &Ntasks );    
  MPI_Comm_rank( myCOMM_WORLD, &Myrank );     


  // ·······················
  // prepare message limits
  // and buffers
  
  int max_logsize = ( argc > 1 ? atoi(*(argv+1)) : 20 );
  if ( max_logsize >= 24 )
    {
      if ( Myrank == 0 )
	printf("don't be greedy, use max log size < 24\n");
      MPI_Finalize();
      return 1;
    }

  printf ( "running with max log size : %d\n", max_logsize);
  unsigned int max_size = ( 1<< max_logsize);
    
  double timing[max_logsize];
  memset ( timing, 0, max_logsize * sizeof(double) );
  char *buffer_send = (char*)malloc( max_size );
  char *buffer_recv = (char*)malloc( max_size );

  // ····················
  // The communication round
  
  if ( Myrank == 0 )
    {
      for ( unsigned int j = 1; j < max_size; j++ )
	buffer_send[j] = 1;  // not so important, though
      
      for ( int j = 1; j < max_logsize; j++ )
	{
	  unsigned int size = ( 1 << j );
	  timing[j] = MPI_Wtime();

	  for ( int n = 0; n < COUNTS; n++ )
	    {
	      MPI_Send(   // ... complete
                      );

	      MPI_Recv(   // ... complete
                      );
	    }
	  timing[j] = (MPI_Wtime()-timing[j]) / COUNTS;
	}

      // print results
      
    }
  else if ( Myrank == 1)
    {
      // complete
    }

  

  MPI_Finalize();
  return 0;
}
