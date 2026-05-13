
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

#define REQ_TAG 0
#define ANS_TAG 1
#define RES_TAG 2
#define BUNCH_SIZE 10000
#define BUNCH2_SIZE (BUNCH_SIZE*2)



int generate_points( double *array, int N, unsigned short seeds[3] )
{
  for ( int i = 0; i < N; i++ )
    array[i] = erand48( seeds );
  return 0;
}

int main( int argc, char **argv )
{
  /* ··································································
   +
   +  initialize the MPI framework
   +
   * ··································································· */
  
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
  if ( provided < MPI_THREAD_SINGLE )
    {
      // manage
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

  int Myrank, Ntasks;
  MPI_Comm myCOMM_WORLD;

  MPI_Comm_dup ( MPI_COMM_WORLD, &myCOMM_WORLD );
  MPI_Comm_size( myCOMM_WORLD, &Ntasks );    
  MPI_Comm_rank( myCOMM_WORLD, &Myrank );     


  /* ··································································
   +
   +  split the world in two
   +  Rank 0 will stay alone in a communicator, all the rest of tasks
   +  will participate in a second communicator
   +
   * ··································································· */

  MPI_Comm WorkWORLD;
  MPI_Comm_split( myCOMM_WORLD, (Myrank == 0), Myrank, &WorkWORLD );

  
  double AllWtime_start = MPI_Wtime();

  /* ··································································
   +
   +  specialize the execution:
   +  Rank 0 in myCOMM_WORLD will generate rnd numbers and answer to
   +         requests
   +  Ranks > 0 will process the rnd numbers and get the estimate of
   +           PI greek
   +
   * ··································································· */
  
  if ( Myrank == 0 )
    {

      //  Rank 0 will spawn omp threads
      //  th 0 will listen to MPI request and send data
      //  th > 0 will generate rnd values as soon as th 0
      //  has sent some around
      //
      //  rnd values will be placed in arrays, one per each th > 0
      //  a guardian value per array will store the information
      //  about those values being ready to be sent or in need
      //  of being discarded and re-generated
      //
      
     #define READY       1         // signals that a rnd value buffer is ready to be sent
     #define INVALID     0         // signals that a rnd value buffer has already been sent

      //
      //  set-up things
      //
      int nthreads, nworkers;
     #pragma omp parallel
     #pragma omp single
      nthreads = omp_get_num_threads();
      if ( nthreads == 1 )
	nthreads=2, omp_set_num_threads(nthreads);
      nworkers = nthreads - 1;
      
      int     stop = 0;
      double *points[nworkers];    // every worker thread will allocate room for data
      int     guardians[nworkers]; // guardian values
      
      for ( int i = 0; i < nworkers; i++ )
	guardians[i] = INVALID;

     #pragma omp parallel
      {
	int me = omp_get_thread_num();
	
	if ( me == 0 )
	  {
	    
	    int last = 0;
	    do
	      //
	      // a loop until a request with a stop value will be received
	      //
	      {

		// get a request
		int request;
		MPI_Status status;
		MPI_Recv( &request, 1, MPI_INT, MPI_ANY_SOURCE, REQ_TAG, myCOMM_WORLD, &status );
		if ( request != 0 )
		  {
		    // someone wants new data to process

		    // find the first buffer ready, starting from the
		    // most recently used
		    int idx = last;
		    int ready;
		    do
		      {
		       #pragma omp atomic read acquire
			ready = guardians[idx];		
			if ( !ready )
			  idx = ++idx % nworkers;
		      }  while ( !ready );
		    last = idx;

		    // a buffer has been found, we send it
		    MPI_Send( points[idx], BUNCH2_SIZE, MPI_DOUBLE, status.MPI_SOURCE, ANS_TAG, myCOMM_WORLD);
		    
		    // signal that that buffer has been sent and must be re-populated
		   #pragma omp atomic write release
		    guardians[idx] = INVALID;
		  }
		else
		  // signal that the worker tasks ended
		 #pragma omp atomic write release
		  stop = 1;
		
	      }
	    while( !stop );	    

	  }
	
	else	  
	  {
	    int _me = me-1;   // thread 0 is doing MPI, threads > 0 are doing the
			      // rnd number generation.
			      // hence, the indexing of points[] and guardians[]
			      // is from 0 to nworkers-1 and we need that every
			      // thread > 0 indexes its own by me-1

	    // allocate room for the rnd values
	    points[_me]   = (double*)malloc(sizeof(double)*BUNCH2_SIZE);
	    // save a local copy not to access the shared array
	    double *array = points[_me];

	    // initialize my own rnd seeds
	    unsigned short seeds[3] = {time(NULL)+me, time(NULL)+2*me, time(NULL)};
	    seed48( seeds );	    

	    // generate points
	    generate_points( array, BUNCH2_SIZE, seeds );
	    // signal that this buffer is ready
	   #pragma omp atomic write release
	    guardians[_me] = READY;	    

	    // now enter in a loop in which as soon as my
	    // buffer has been sent I will re-populate it
	    //
	    int mystop;
	    do
	      {
		// watch the guardian value
		//
		int myguard;
		do {
		 #pragma omp atomic read acquire
		  myguard = guardians[_me];
		 #pragma omp atomic read acquire
		  mystop = stop;		  
		} while ( (myguard != INVALID) && (!mystop) );


		if ( ! mystop ) {
		  // the game continues, generate new points
		  generate_points( array, BUNCH2_SIZE, seeds );
		  // signal that the buffer is populated again
		 #pragma omp atomic write release
		  guardians[_me] = READY; }

	      }
	    while( !mystop );

	    // free the memory
	    free ( points[_me] );
	  }
	
      }
      
      double pi;
      MPI_Recv( &pi, 1, MPI_DOUBLE, MPI_ANY_SOURCE, RES_TAG, myCOMM_WORLD, MPI_STATUS_IGNORE );
      printf("PI estimate is %12.10g\n", pi);
    }
  
  else
    //
    // Ranks > 0 will ask for points and process them
    // they will stop when the required convergence will be reached
    //
    {

      // get my rank in the workers' world
      int MyWrank, Nworkers;
      MPI_Comm_size( WorkWORLD, &Nworkers );    
      MPI_Comm_rank( WorkWORLD, &MyWrank );     

      double tolerance = (argc > 1 ? atof(*(argv+1)) : 1e-5);
       
      int                 round            = 0;   // how many round we run
      int                 converged        = 0;   // whether the estimate is good enough
      unsigned long long  inner_points     = 0;   // my inner points
      unsigned long long  all_inner_points = 0;   // all the inner points
      unsigned long long  total_points     = 0;   // all the processed points
      long double         prev_pi          = 0;   
      double             *points           = (double*)malloc( 2*BUNCH_SIZE * sizeof(double) );

     #pragma omp parallel
      thtiming = 0;
      
      while (!converged)
	{
	  int ask_for_points = 1;
	  MPI_Send( &ask_for_points, 1, MPI_INT, 0, REQ_TAG, myCOMM_WORLD);
	  MPI_Recv( points, 2*BUNCH_SIZE, MPI_DOUBLE, 0, ANS_TAG, myCOMM_WORLD, MPI_STATUS_IGNORE);

	  total_points += BUNCH_SIZE;
	  
	 #pragma omp parallel
	  {
	    double tstart = TtCPU;
	   #pragma omp for reduction(+:inner_points)
	    for ( int i = 0; i < BUNCH_SIZE; i++)
	      {
		int idx = i*2;
		double dist = points[idx]*points[idx] + points[idx+1]*points[idx+1];
		inner_points += (dist <= 1);
	      }

	    double tstop = TtCPU;
	    thtiming += tstop-tstart;
	  }

	  
	  MPI_Allreduce( &inner_points, &all_inner_points, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, WorkWORLD);
	  long double pi    = (long double)all_inner_points/ (total_points*Nworkers);
	  long double delta = fabsl((pi - prev_pi)/pi);
	  if ( delta < tolerance )
	    {
	      converged = 1;
	      if( MyWrank == 0 ) {
		
		double PI = (double)(4*pi);
		printf("\t >> round %d, pi estimate is %.12Lg, converged with delta %.12Lg\n", round, 4*pi, delta);
		ask_for_points = 0;
		MPI_Send( &ask_for_points, 1, MPI_INT, 0, REQ_TAG, myCOMM_WORLD );		
		MPI_Send( &PI, 1, MPI_DOUBLE, 0, RES_TAG, myCOMM_WORLD ); }

	      MPI_Barrier( WorkWORLD);
	     #pragma omp parallel
	      printf("\t** Worker %02d th %02d has taken %g sec\n", MyWrank, omp_get_thread_num(), thtiming);
	    }
	  else
	    {
	      round++;
	      prev_pi = pi;
	    }
	}

      free ( points );
    }


  double AllWtime_stop = MPI_Wtime();


  printf("Task %d has taken %g sec\n", Myrank, AllWtime_stop - AllWtime_start);
  
  MPI_Finalize();
  return 0;
}
