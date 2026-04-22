#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <omp.h>



#if !defined(_OPENMP)
#error "openmp is required to run this code"
#endif

int main( void )
{
  int data  = 0;
  int ready = 0;
 #pragma omp parallel num_threads(2)
  {
    int p;
    int myid = omp_get_thread_num();
    if ( myid == 0 )
      {
	// wait a bit, then signal
	//nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 5000}, NULL);
	data = 42;
	ready = 1;
      }
    else
      {
	/*
	 * when compiled with -O3 the compiler
	 * hoists the load before the loop - it
	 * spots that nothinig inside the loop
	 * modifies the "ready" variable - and
	 * then spins on the register.
	 */
	while ( ready == 0)
	;
	  //nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 500}, NULL);//spin	
	printf("th%d sees data= %d and ready = %d\n",
	       myid, data, ready );
      }
  }
  return 0;
}
