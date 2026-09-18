

#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include "prefix_sum.serial.h"



inline DTYPE scan( const uint N, DTYPE * restrict array )
{

  DTYPE avg    = array[0];

  for ( uint ii = 1; ii < N; ii++ )
    {
      avg       += array[ii];
      array[ii]  = avg;
    }
  
  return avg;
}


inline DTYPE scan_efficient( const uint N, DTYPE * restrict array )
{

  uint N_4 = (N/4)*4;
  DTYPE carry = (DTYPE)0;

  
  PRAGMA_VECT_LOOP
  for ( uint ii = 0; ii < N_4; ii+=4 )
    {
      DTYPE register a0 = array[ii];
      DTYPE register a1 = array[ii+1];
      DTYPE register a2 = array[ii+2];
      DTYPE register a3 = array[ii+3];
      
      DTYPE t1 = a0 + a1;
      DTYPE t2 = a1 + a2;
      DTYPE t3 = a2 + a3;

      DTYPE u2 = a0 + t2;
      DTYPE u3 = t1 + t3;
      
      array[ii] = a0 + carry;
      array[ii+1] = t1 + carry;
      array[ii+2] = u2 + carry;
      array[ii+3] = u3 + carry;

      carry += u3;
    }

  for ( uint ii = N_4; ii < N; ii++ ) {
    carry += array[ii-1];
    array[ii] = carry; }
  
  return carry;
}


#define N_default  1000
#define _scan      0
#define _scan_e    1

int main ( int argc, char **argv )
{
  
  struct timespec ts;  
  int             Nth_level1 = 1;
  int             Nth_level2 = 0;
  
  // -------------------------------------------------------------
  // variables' initialization to default values
  //

  uint    N        = N_default;
  int    scan_type = _scan;
  
  
  if ( argc > 1 )
    {
      scan_type = atoi( *(argv+1) );
      if ( argc > 2 )
	N  = (unsigned)atoi( *(argv+2) );
    }

  printf( "scan type: %d\n", scan_type );

  
  // -------------------------------------------------------------
  // data init.

  double timing_start;
  double timing_scan;
  double timing_prepare;
  double total_weight;
  
  DTYPE *array   = (DTYPE*)malloc( N * sizeof(DTYPE) );

  timing_start = CPU_TIME;

  for ( uint ii = 0; ii < N; ii++ )
    array[ii] = (DTYPE)ii;
  
  timing_prepare = CPU_TIME - timing_start;

  // ................................................
  //  SCAN
  // ................................................

  if ( scan_type == _scan )
    total_weight = scan( N, array );

  else if (scan_type == _scan_e)
    total_weight = scan_efficient( N, array );


  timing_scan  = CPU_TIME - timing_start;      

  printf("timing for scan is %g, timing for prepare is %g [total weight: %g]\n",
	 timing_scan, timing_prepare, total_weight);
  return 0;
}
