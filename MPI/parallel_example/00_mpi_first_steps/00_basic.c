
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

#include <mpi.h>


int main ( int argc, char **argv )
{

  int Rank;
  int Ntasks;
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

  int planets;
  int myplanets;
  
  planets = (argc > 1? atoi(*(argv+1)) : 0 );

  myplanets  = planets / Ntasks;
  myplanets += (Rank < planets % Ntasks);
  
  printf( "Hello world, I'm task %d among %d friends, i'll model %d planets\n",
	  Rank, Ntasks, myplanets );

  MPI_Finalize();

  return 0;
}
