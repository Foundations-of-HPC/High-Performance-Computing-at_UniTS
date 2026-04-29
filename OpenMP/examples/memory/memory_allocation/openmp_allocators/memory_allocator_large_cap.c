
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

int main( void )
{
  size_t num_elements = 1000000;
  size_t bytes        = num_elements * sizeof(double);
  
  // 1. Allocate using a predefined OpenMP allocator
  double *data = (double*) omp_alloc(bytes, omp_large_cap_mem_alloc);
  
  if (data == NULL)
    {
      fprintf(stderr, "Allocation failed!\n");
      return 1;
    }
  
  // 2. Safely use the allocated memory in a parallel region
 #pragma omp parallel for
  for (size_t i = 0; i < num_elements; i++)
    data[i] = i * 3.14;
  
  printf("Data[500] = %f\n", data[500]);
  
  // 3. Free the memory using the EXACT SAME allocator it was created with
  omp_free(data, omp_large_cap_mem_alloc);
  
  return 0;
}
