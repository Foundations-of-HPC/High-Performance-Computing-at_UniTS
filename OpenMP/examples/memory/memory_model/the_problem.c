#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>


#if !defined(_OPENMP)
#error "openmp is required to run this code"
#endif

int main( void )
{
  int x = 0;
  int y = 0;
 #pragma omp parallel num_threads(2)
  {
    int myid = omp_get_thread_num();
    
    if ( myid == 0 )
      x = 1;
    else
      y = 1;
    
    printf("th%d sees x= %d and y = %d\n", myid, x, y );
  }

  return 0;
}
