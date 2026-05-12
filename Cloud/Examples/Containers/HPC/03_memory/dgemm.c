#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define N 2048  // Matrix dimension

int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : N;
    
    double *A = malloc(n * n * sizeof(double));
    double *B = malloc(n * n * sizeof(double));
    double *C = calloc(n * n, sizeof(double));
    
    // Initialise matrices
    #pragma omp parallel for
    for (int i = 0; i < n * n; i++) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
    }
    
    double t_start = omp_get_wtime();
    
    // Tiled matrix multiplication (cache-friendly)
    int T = 64;  // Tile size
    #pragma omp parallel for schedule(dynamic,1) collapse(2)
    for (int ii = 0; ii < n; ii += T)
        for (int jj = 0; jj < n; jj += T)
            for (int kk = 0; kk < n; kk += T)
                for (int i = ii; i < ii + T && i < n; i++)
                    for (int k = kk; k < kk + T && k < n; k++) {
                        double a_ik = A[i * n + k];
                        for (int j = jj; j < jj + T && j < n; j++)
                            C[i * n + j] += a_ik * B[k * n + j];
                    }
    
    double t_end = omp_get_wtime();
    double elapsed = t_end - t_start;
    double gflops = 2.0 * n * n * n / elapsed / 1e9;
    
    printf("N=%d  threads=%d  time=%.3fs  GFLOPS=%.2f\n",
           n, omp_get_num_threads(), elapsed, gflops);
    
    free(A); free(B); free(C);
    return 0;
}
