
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

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>


int main ( void )
{
  const int N = 1024;
  double A[N];
  double B[N];

  // Instruct the compiler to place array A in High Bandwidth Memory
 #pragma omp allocate(A) allocator(omp_high_bw_mem_alloc)
  
  // Instruct the compiler to place array B in Low Latency Memory (e.g., L3 cache pinning if supported)
 #pragma omp allocate(B) allocator(omp_low_lat_mem_alloc)
  
 #pragma omp parallel for
  for(int i = 0; i < N; i++)
    {
      A[i] = i * 1.0;
      B[i] = i * 2.0;
    }

  printf("A[10] + B[10] = %f\n", A[10] + B[10]);

  // NOTE: the variables are automatic (i.e. they are "in the stack".
  // No omp_free needed; memory is managed automatically based on the variable's scope.
  return 0;
}
