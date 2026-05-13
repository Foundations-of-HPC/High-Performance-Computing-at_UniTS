
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


/* ················································
 *
 *  This example is provided in the frame of the
 *  course "Introduction to HPC" at University
 *  of Trieste, 2024-2025.
 *
 *  This code calculates the value of pi greek by
 *  throwing N dice in the first quadrant of the
 *  circle of radius 1, and counting how many fall
 *  within distance_from_origin = 1.
 *
 *  Check the examples calculate_pi.collectives.c,
 *  calculate_pi.producer_consumer.c and
 *  calculate_pi.producer_consumer.threads.c for
 *  different implementations
 *
 *  DISCLAIMER : the code is correct as for the
 *               MPI parallelization (purposely, no
 *               collectives are used) but contains
 *               a simple bug.
 *               Exdercise: find and fix it
 *
 *
 *  luca.tornatore@inaf.it
 *
 * ················································ */



#if defined(__STDC__)                                                                                                                          
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <mpi.h>

#define TCPU ({struct timespec ts; (clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ), (double)ts.tv_sec + \
                                    (double)ts.tv_nsec * 1e-9);})

#define TtCPU ({struct timespec ts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &ts ), (double)ts.tv_sec + \
				     (double)ts.tv_nsec * 1e-9);})

double thtiming;
#pragma omp threadprivate(thtiming)

int main( int argc, char **argv )
{
  /* initialize MPI */
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
  
  if ( provided < MPI_THREAD_SINGLE )
    {
      // manage the failure with some message
      // ...
      //
      MPI_Abort(MPI_COMM_WORLD, 1);
    }                                                                                                          
                                                                                                                                          
  int Myrank, Ntasks;
  MPI_Comm myCOMM_WORLD;
  
  MPI_Comm_dup ( MPI_COMM_WORLD, &myCOMM_WORLD );
  MPI_Comm_size( myCOMM_WORLD, &Ntasks );
  MPI_Comm_rank( myCOMM_WORLD, &Myrank );

  /* initialize the problem */

  // init the psuedo-random generator
  srand48( Myrank + time(NULL) );

  // get how many shots in total
  unsigned int N = ( argc > 1 ? atoi(*(argv+1)) : 1000000 );
  // translate in how many shots per thread
  N = (N / Ntasks) + (N % Ntasks > 0);

  /* throw the dice and count the inner points */
  
  unsigned int inner_points = 0;
  for ( unsigned int i = 0; i < N; i++ )
    {
      double x = drand48();
      double y = drand48();

      inner_points += ( (x*x + y*y) < 1.0 );
    }

  
  if ( Myrank == 0 )
    /* collect the partial results */
    {
      unsigned long long all_inner_points = inner_points; 
      for ( int t = 0; t < Ntasks-1; t++ )
	/* get the result of every single MPI tasks */
	{
	  unsigned int inner_points;
	  MPI_Status status;
	  MPI_Recv( &inner_points, 1, MPI_INT, MPI_ANY_SOURCE, 0, myCOMM_WORLD, &status );
	  all_inner_points += inner_points;
	  printf("Task 0 has received the result from task %d\n", status.MPI_SOURCE );
	}

      unsigned long long int all_points = (unsigned long long int)N * Ntasks;
      printf ( "pi greek estimate out of %llu points is: %g\n", all_points, (double)all_inner_points*4.0 / all_points );
    }
  else
    /* send the partial result */
    MPI_Send( &inner_points, 1, MPI_INT, 0, 0, myCOMM_WORLD );

  
  MPI_Finalize();
  return 0;
}
