
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
 * Hello World example with MPI
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

int main ( int argc, char **argv )
{
  // ·················
  // intialize MPI
  int provided_thread_level;
  MPI_Comm myCOMM_WORLD;
  MPI_Init_thread( &argc, &argv, MPI_THREAD_SINGLE, &provided_thread_level );
  
  if ( provided_thread_level < MPI_THREAD_SINGLE )
    {
      printf("it was impossibile to get the thread level MPI_THREAD_SINGLE\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  
  // ··················
  // Good practice: don't use
  // MPI_COMM_WORLD
  // Create a separate working space
  
   int Ntasks;
   int Myrank;
   MPI_Comm_dup( MPI_COMM_WORLD, &myCOMM_WORLD );
   MPI_Comm_size( myCOMM_WORLD, &Ntasks );
   MPI_Comm_rank( myCOMM_WORLD, &Myrank );

   // ·················
   // Greetings
   printf("hello MPI world, I'm rank %d out of %d\n", Myrank, Ntasks );

   // ·················
   // Finalize MPI
   
   MPI_Finalize();
   
   return 0;
}

