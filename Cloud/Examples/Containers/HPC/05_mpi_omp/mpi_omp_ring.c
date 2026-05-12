#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ARRAY_SIZE (1 << 22)   // 4M doubles = 32 MB per rank

int main(int argc, char *argv[]) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int nthreads = omp_get_max_threads();
    if (rank == 0)
        printf("MPI ranks: %d, OMP threads/rank: %d\n", size, nthreads);
    
    // Allocate work array
    double *data = malloc(ARRAY_SIZE * sizeof(double));
    for (int i = 0; i < ARRAY_SIZE; i++) data[i] = (double)rank;
    
    double t_start = MPI_Wtime();
    
    // OpenMP parallel work: compute element-wise sin (memory bandwidth bound)
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ARRAY_SIZE; i++)
        data[i] = sin(data[i]) + cos(data[i]);
    
    // MPI ring: pass sum around all ranks
    double local_sum = 0.0;
    for (int i = 0; i < ARRAY_SIZE; i++) local_sum += data[i];
    
    double global_sum;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    double t_end = MPI_Wtime();
    
    if (rank == 0)
        printf("Elapsed: %.4f s | Global sum: %.4f\n", t_end - t_start, global_sum);
    
    free(data);
    MPI_Finalize();
    return 0;
}
