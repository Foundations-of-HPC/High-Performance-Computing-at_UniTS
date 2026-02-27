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
#  if (__STDC_VERSION__ >= 201112L)    // c11
#    define _XOPEN_SOURCE 700
#  elif (__STDC_VERSION__ >= 199901L)  // c99
#    define _XOPEN_SOURCE 600
#  else
#    define _XOPEN_SOURCE 500          // c90
#  endif
#endif


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if _XOPEN_SOURCE >= 600
#  include <strings.h>
#endif
#include <sys/resource.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>


#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ), \
					  (double)ts.tv_sec +           \
					  (double)ts.tv_nsec * 1e-9;})

#define NREPETITIONS 5


void compact_bits(uint32_t a, uint32_t *x, uint32_t *y)
{
  uint32_t b = a >> 1;
  a &= 0x55555555;                 // Extract even bits
  b &= 0x55555555;                 // Extract odd bits

  a = (a ^ (a >> 1)) & 0x33333333; // every 4 bits, gather bits 0 and 2
  b = (b ^ (b >> 1)) & 0x33333333; 
  
  a = (a ^ (a >> 2)) & 0x0f0f0f0f; // every 8 bits, gather bits 0,2,4,6
  b = (b ^ (b >> 2)) & 0x0f0f0f0f;
  
  a = (a ^ (a >> 4)) & 0x00ff00ff; // every 16 bits, gather bits 0,2,4,6,8,10,12,14
  b = (b ^ (b >> 4)) & 0x00ff00ff;
  
  a = (a ^ (a >> 8)) & 0x0000ffff;
  b = (b ^ (b >> 8)) & 0x0000ffff;

  *x = a;
  *y = b;
  
  return;
}


int main ( int argc, char **argv )
{
  if ( (argc > 1) &&
       ((strcmp(*(argv+1), "-h") == 0) ||
	(strcmp(*(argv+1), "--help") == 0)) ) {
    printf("arguments: log_2(linear_size_of_matrix)\n");
    return 0; }
  
  // --------------------------------------------------------------
  //  set-up
    
  // get the power-of-two of the linear dimention of the matrix
  uint32_t k = (argc > 1 ? atoi(*(argv+1)) : 2 );
  k = ( 1 << k );
  // ······························································
  
  // the matrixm size is k^2
  uint32_t size = k*k;

  // --------------------------------------------------------------
  // allocate the memory
  uint32_t *src = (uint32_t*)malloc( sizeof(uint32_t)* size *2 );
  uint32_t *dst = src + size;
  // ······························································


  
  // --------------------------------------------------------------
  // initialize the src matrix
  //
  // also
  // - initialize timing_max to some meaningful value
  // - drag dst through cache
  
  printf ( "initialize a %u x %u matrix\n", k, k );

  double timing_max = CPU_TIME;
  for ( uint32_t counter = 0; counter < size; counter++ ) {
    src [ counter ] = counter;
    dst [ counter ] = counter; } 
  timing_max = CPU_TIME - timing_max;
  // ······························································

  
  // --------------------------------------------------------------
  // transpose the matrix
  double timing_sum = 0;
  for ( int rep = 0; rep < NREPETITIONS; rep++ )
    {            
      double timing = CPU_TIME;
      for ( int i = 0, counter = 0; i < k; i++ )      
	for ( int j = 0; j < k; j++, counter++ )
	  {
	    uint32_t x, y;
	    compact_bits( counter, &x, &y );
	    
	    uint32_t src_pos = x*k + y;
	    uint32_t dst_pos = y*k + x;
	    dst[ dst_pos ] = src [ src_pos ]; 	
	  }
      timing = CPU_TIME - timing;
      timing_sum += timing;
      timing_max  = ( timing_max < timing  ? timing : timing_max );
    }
  timing_sum = (timing_sum-timing_max) / NREPETITIONS;

  printf ( "transposing took %g sec\n", timing_sum);
  
  // ······························································
  
  // verify
  uint32_t fails = 0;
  for ( int i = 0, counter = 0; i < k; i++ )
    {
      int row_offset_src = i*k;
      for ( int j = 0; j < k; j++ )
	{
	  int row_offset_dst = j*k;
	  fails += (src[row_offset_src+j] != dst[row_offset_dst+i]);
	}
    }

  // ······························································

  
  free ( src );
  
  if ( fails > 0 )
    printf("oh noooo, %u fails found\n", fails);
  else
    printf("like a charm!\n");
  
  return 0;
  
}
