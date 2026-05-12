// File: affinity_report.c
// Compile: gcc -fopenmp -o affinity_report affinity_report.c
#define _GNU_SOURCE
#include <stdio.h>
#include <omp.h>
#include <sched.h>

int main() {
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int cpu = sched_getcpu();
        int nthreads = omp_get_num_threads();
        #pragma omp critical
        printf("Thread %2d / %2d running on CPU %3d\n", tid, nthreads, cpu);
    }
    return 0;
}
