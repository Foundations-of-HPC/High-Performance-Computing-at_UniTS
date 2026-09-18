
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


// GOAL: test the morton order

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <../mypapi.h>

typedef unsigned char uchar;

uchar get_bit  ( uchar **, int, int );
void  set_bits ( int, uchar **, int, int, int );


// extract the x and y from a 2d morton key of 32 bits
// --> x and y are 16 bits each
//
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
    printf("arguments: log2(N) where N is the linear size of the matrix (default value is 3\n");
    return 0; }
  
  uint32_t k          = (argc > 1 ? atoi(*(argv+1)) : 3 );              // log2 of the linear matrix dimension
  int      show_cache = (argc > 2 ? atoi(*(argv+2)) : 0 );              // whether we want to show the cache hits/misses
  int      data_size  = (argc > 3 ? atoi(*(argv+3)) : sizeof(double) ); // the size of data in bytes
  int      line_size  = (argc > 4 ? atoi(*(argv+4)) : 64 );             // the cache line size in bytes

  uint32_t SIZE = (1 << (k+k) );

  
  if ( !show_cache )
    {
      // we just show the access pattern to the matrix entrires
      //
      for ( int i = 0; i < SIZE; i++ )
	{
	  uint32_t x, y;
	  
	  compact_bits( i, &x, &y );
	  printf(" [ %9d ] %5d  %5d\n", i, x, y);
	}
    }
  else
    {            
      int     N           = (1 << k);
      int     N_in_bytes  = (N/8) + (N%8 > 0);
      uchar **cache_loads = (uchar**)malloc( N * sizeof(uchar*) );

      cache_loads[0] = (uchar*)calloc( N * N_in_bytes, 1 );      
      for ( int r = 1; r < N; r++ )
	cache_loads[r] = cache_loads[r-1]+N_in_bytes;

      int    elements_per_line = line_size / data_size;

      uint32_t hits_count = 0;
      for ( int i = 0; i < SIZE; i++ )
	{
	  uint32_t x, y;

	  char status;
	  
	  compact_bits( i, &x, &y );
	  if ( get_bit( cache_loads, x , y ) )
	    { status = 'h'; hits_count++; }
	  else {
	    status = 'm';
	    set_bits(elements_per_line, cache_loads, N, x, y);
	  }
	  
	  printf(" [ %9d ] %5d  %5d  %c\n",
		 i, x, y, status);	       
	}

      printf("%u hits over %u accesses (miss/hits rates: %.1f%%, %.1f%%)\n",
	     hits_count, SIZE, (double)(SIZE-hits_count)/SIZE*100, (double)hits_count/SIZE*100 );
      
      free ( cache_loads[0] );
      free ( cache_loads );

    }
  
  return 0;
  
}


uchar get_bit( uchar **cache, int x, int y )
{
  // get the byte that holds the corresponding bit
  // y is the row, x/8 is the byte we are looking for
  //
  uchar byte = cache[y][x/8];

  // extract the bit from the byte
  return (byte & (1<< (x%8)));
}


void set_bits ( int elements_per_line, uchar **cache, int N, int x, int y )
//
// - elements_per_line : how many type per cache line
// - **cache           : points to the bytes that signals whether
//                       a given entry in the matrix is already loaded in the cache
// - N                 : the dimension of the matrix
// - x,y               : the matrix's element from which starting the cache load
// 
{
  // find how many matrix elements are loaded into the cache
  int n = (N-x);
  n = ( n > elements_per_line ? elements_per_line : n );

  int x_element = x/8;
  
  while ( n > 0 )
    {
      int bits_to_set = (n>8?8:n);
      cache[y][x_element++] = (uchar)( ((int)1<<bits_to_set)-1 );
      n -= bits_to_set;
    }
  
  return;
}
