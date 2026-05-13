
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
#include <string.h>
#include <stdio.h>
#include <mpi.h>

int main( int argc, char **argv )
{

  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
  if ( provided < MPI_THREAD_SINGLE )
    {
      // manage
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

  int me, Ntasks;
  MPI_Comm myCOMM_WORLD;

  MPI_Comm_dup ( MPI_COMM_WORLD, &myCOMM_WORLD );
  MPI_Comm_size( myCOMM_WORLD, &Ntasks );    
  MPI_Comm_rank( myCOMM_WORLD, &me );

  // find the log2P
  //
  int Top = 1;
  for( int log2Ntasks = 0; (1 << log2Ntasks) < Ntasks; log2Ntasks++, Top <<= 1 )
    ;
  //Top = (1 << log2Ntasks);
  
  if ( me == 0)
    printf("ceil of 2^( log2(Ntasks) ) is %d\n\n", Top);

  // the array exchanges will be used to keep
  // track of how many times the pair-wise
  // communication between every tasks and
  // the others is performed
  //
  int buffer = me;
  int exchanges[Ntasks];
  int *Lexchanges;
    
  if ( me == 0 ) 
    Lexchanges = (int*)malloc( Ntasks*Top*sizeof(int) );
  else
    Lexchanges = (int*)malloc( Top*sizeof(int) );

  memset( exchanges, 0, sizeof(int)*Ntasks);
  memset( Lexchanges, 0, sizeof(int)*Top);
  exchanges[me] = 1;
  
  MPI_Barrier(MPI_COMM_WORLD);


  // ======================================================
  
  for( int ngrp = 1; ngrp < Top; ngrp++ )
    {
      int target = me ^ ngrp;

      if(target < Ntasks)
	{
	  exchanges[target]++;
	  Lexchanges[ngrp] = target;
	  int accumulate;
	  MPI_Sendrecv( &buffer, 1, MPI_INT, target, 0, &accumulate, 1, MPI_INT, target, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	  buffer += accumulate;
	}
      else
	Lexchanges[ngrp] = -1;
    }

  if ( me > 0 )
    MPI_Gather(Lexchanges, Top, MPI_INT, 0x0, Top, MPI_INT, 0, MPI_COMM_WORLD);
  else
    {
      MPI_Gather(MPI_IN_PLACE, Top, MPI_INT, Lexchanges, Top, MPI_INT, 0, MPI_COMM_WORLD);

      printf ("COMMUNICATION MATRIX:\n\n" );
      for ( int task = 0; task < Ntasks; task++ ) {
	printf ( "T%02d : ", task);
	int *row = Lexchanges+task*Top;
	
	for ( int ngrp = 1; ngrp < Top; ngrp++)
	  printf ( "%02d ", row[ngrp] );
	
	printf("\n"); }      
    }
  
  // ======================================================
  
  MPI_Barrier(MPI_COMM_WORLD);

  // check that every exchange happened
  // once and only once
  //
  int faults = 0;
  for ( int i = 0; i < Ntasks; i++ )
    {
      if( i == me )
	{
	  for( int j = 0; j < Ntasks; j++ )
	    {
	      if( exchanges[j] == 0 )
		printf("%d never interacted with %d\n", me, j), faults++;
	      else if( exchanges[j] > 1 )
		printf("%d interacted multiple times (%d) with %d\n",
		       me, exchanges[j], j), faults++;
	    }
	}
      MPI_Barrier(MPI_COMM_WORLD);
    }

  
  fflush(stdout);

  // get how many faults there have been,
  // if any
  //
  int myfaults[2] = { (buffer != Ntasks*(Ntasks-1)/2), faults };
  if ( me == 0 )
    MPI_Reduce( MPI_IN_PLACE, &myfaults, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  else
    MPI_Reduce( &faults, NULL, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if( me == 0 )
    {
      if((myfaults[0]+myfaults[1]) > 0)
	printf("tot faults; %d %d\n",
	       myfaults[0], myfaults[1]);
      else
	printf("everybody interacted once and only once with everybody else\n");
    }
  
  MPI_Finalize();
}
