
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
 * This code is part of a set of codes that uses a trivial algorithm
 * for the estimate of pi greek to illustrate some basic facts
 * about performance in OpenMP.
 *
 * -- version 1 --
 * discriminate points and accumulate valid ones using an atomic ; 
 * we expect a better performance respect to the usage of critical,
 * both in terms of run-time and of scalability
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>

#define DEFAULT 1000000


int main ( int argc, char **argv)
{

  long long unsigned int M = 0;
  int                    nthreads;
  
 #pragma omp parallel  
 #pragma omp master
  nthreads = omp_get_num_threads();    

  long long int N = (argc > 1 ? atoll(argv[1]) : DEFAULT ) ;
  printf("omp calculation with %d threads\nN=%Ld\n", nthreads ,N);

  double timing = omp_get_wtime();
 #pragma omp parallel
  {
    int myid = omp_get_thread_num();
    
    long int SEED = time(NULL);
    int unsigned short myseeds[3] = {(short)(SEED+(myid)),
				     (short)(SEED^(myid*3+1)),
				     (short)(SEED&(myid*4+2))};
    seed48( myseeds );
    // ------------------------------------------

    for( long long unsigned int i = 0; i < N; i++)
      {
	double x = erand48(myseeds);
	double y = erand48(myseeds);

       #pragma omp atomic update
	M += ( (x*x + y*y) < 1.0 );
      }

  }    
    
  timing = omp_get_wtime() - timing;
  
  printf("Estimation of pi: %1.9f\n Walltime:%g\n",
	 (4.0*(double)M)/(N*nthreads), timing );
  return 0;
}
