
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
  omp_allocator_handle_t custom_hbm_alloc;
  omp_alloctrait_t traits[2];
  
  // Trait 1: 64-byte alignment for AVX-512 / SIMD optimization
  traits[0].key = omp_atk_alignment;
  traits[0].value = 64;
  
  // Trait 2: Fallback policy. If High Bandwidth Memory is exhausted, 
  // fall back to default RAM instead of returning a NULL pointer.
  traits[1].key = omp_atk_fallback;
  traits[1].value = omp_atv_default_mem_fb;
  
  // Initialize the allocator, binding it to the High Bandwidth memory space
  custom_hbm_alloc = omp_init_allocator(omp_high_bw_mem_space, 2, traits);

  // check that the initialization has been successful
  if (custom_hbm_alloc == omp_null_allocator)
    {
      fprintf(stderr, "Failed to initialize custom allocator.\n");
      return 1;
    }
  
  // Use the custom allocator
  size_t size = 2048 * sizeof(float);
  float *vector = (float*) omp_alloc(size, custom_hbm_alloc);
  
 #pragma omp parallel for
  for (int i = 0; i < 2048; i++)
    vector[i] = i * 2.0f;
  
  printf("Vector[100] = %f\n", vector[100]);
  
  // Cleanup memory AND the allocator handle
  omp_free(vector, custom_hbm_alloc);
  omp_destroy_allocator(custom_hbm_alloc);
  
  return 0;
}
