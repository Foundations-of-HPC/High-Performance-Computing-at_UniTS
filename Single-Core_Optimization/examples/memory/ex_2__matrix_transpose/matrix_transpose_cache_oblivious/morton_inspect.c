
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
#include <stdint.h>


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
  uint32_t k = (argc > 1 ? atoi(*(argv+1)) : 3 );
  uint32_t N = (1 << (k+k) );

  for ( int i = 0; i < N; i++ )
    {
        uint32_t x, y;
	
	compact_bits( i, &x, &y );
	printf(" [ %9d ] %5d  %5d\n", i, x, y);
    }
  
  
  return 0;
  
}
