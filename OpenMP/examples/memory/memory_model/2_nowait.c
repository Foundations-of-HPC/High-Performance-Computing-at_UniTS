/*
 * 12_nowait.c
 * 
 * Demonstrates the 'nowait' clause for removing implicit barriers.
 * 
 * By default, worksharing constructs (for, sections, single) have an
 * implicit barrier at the end. All threads must finish before any can proceed.
 * 
 * The 'nowait' clause removes this barrier, allowing threads that finish
 * early to proceed immediately to the next work.
 * 
 * CRITICAL: Only use nowait when the next work does NOT depend on
 * results from the current worksharing construct!
 *
 * Compile: gcc -fopenmp -O2 -o 12_nowait 12_nowait.c -lm
 * Run:     ./12_nowait
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define N 10000000

/*
 * Example 1: Independent loops - nowait is SAFE
 * 
 * The two loops operate on different arrays with no dependencies.
 * Threads that finish the first loop can start the second immediately.
 */
void example_independent_loops(void)
{
    double *a = (double *)malloc(N * sizeof(double));
    double *b = (double *)malloc(N * sizeof(double));
    double t_start, t_with_barrier, t_without_barrier;
    
    printf("\n=== Example 1: Independent loops ===\n");
    
    // WITH implicit barrier (default)
    t_start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp for  // implicit barrier at end
        for (int i = 0; i < N; i++)
            a[i] = sin((double)i);
        
        // All threads wait here before starting the next loop
        
        #pragma omp for
        for (int i = 0; i < N; i++)
            b[i] = cos((double)i);
    }
    t_with_barrier = omp_get_wtime() - t_start;
    
    // WITHOUT barrier (nowait)
    t_start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp for nowait  // NO barrier - threads proceed immediately
        for (int i = 0; i < N; i++)
            a[i] = sin((double)i);
        
        // Fast threads can start this loop while slow threads finish above
        
        #pragma omp for nowait
        for (int i = 0; i < N; i++)
            b[i] = cos((double)i);
    }  // Implicit barrier at end of parallel region
    t_without_barrier = omp_get_wtime() - t_start;
    
    printf("Time with barriers:    %.4f s\n", t_with_barrier);
    printf("Time with nowait:      %.4f s\n", t_without_barrier);
    printf("Speedup from nowait:   %.2fx\n", t_with_barrier / t_without_barrier);
    
    free(a);
    free(b);
}

/*
 * Example 2: Dependent loops - nowait is UNSAFE
 * 
 * The second loop reads from array a[], which the first loop writes.
 * Using nowait here would cause a race condition!
 */
void example_dependent_loops(void)
{
    double *a = (double *)malloc(N * sizeof(double));
    double *b = (double *)malloc(N * sizeof(double));
    
    printf("\n=== Example 2: Dependent loops (nowait would be WRONG) ===\n");
    
    #pragma omp parallel
    {
        // First loop writes a[]
        #pragma omp for  // MUST have barrier here!
        for (int i = 0; i < N; i++)
            a[i] = sin((double)i);
        
        // Barrier ensures all of a[] is written before we read it
        
        // Second loop reads a[]
        #pragma omp for
        for (int i = 0; i < N; i++)
            b[i] = a[i] * 2.0;  // Depends on a[i] being written!
    }
    
    // Verify correctness
    int errors = 0;
    for (int i = 0; i < N; i++) {
        double expected = sin((double)i) * 2.0;
        if (fabs(b[i] - expected) > 1e-10) errors++;
    }
    printf("Errors (should be 0): %d\n", errors);
    
    free(a);
    free(b);
}

/*
 * Example 3: nowait with single
 * 
 * 'single' normally has an implicit barrier at the end.
 * With 'nowait', other threads don't wait for the single block to complete.
 */
void example_single_nowait(void)
{
    double *a = (double *)malloc(N * sizeof(double));
    double t_start, t_with_barrier, t_without_barrier;
    
    printf("\n=== Example 3: single with and without nowait ===\n");
    
    // WITH implicit barrier
    t_start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp single  // All other threads wait
        {
            printf("  [single] Thread %d doing initialization...\n", 
                   omp_get_thread_num());
            // Simulate slow initialization
            for (volatile int i = 0; i < 50000000; i++);
        }
        // Barrier here - all threads waited
        
        #pragma omp for
        for (int i = 0; i < N; i++)
            a[i] = (double)i;
    }
    t_with_barrier = omp_get_wtime() - t_start;
    
    // WITHOUT barrier
    t_start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp single nowait  // Other threads proceed immediately
        {
            printf("  [single nowait] Thread %d doing independent logging...\n", 
                   omp_get_thread_num());
            // Simulate slow logging (doesn't affect the computation)
            for (volatile int i = 0; i < 50000000; i++);
        }
        // No barrier - other threads start the loop immediately
        
        #pragma omp for
        for (int i = 0; i < N; i++)
            a[i] = (double)i;
    }
    t_without_barrier = omp_get_wtime() - t_start;
    
    printf("Time with single barrier:    %.4f s\n", t_with_barrier);
    printf("Time with single nowait:     %.4f s\n", t_without_barrier);
    printf("Speedup from nowait:         %.2fx\n", t_with_barrier / t_without_barrier);
    
    free(a);
}

/*
 * Example 4: Multiple nowait in sequence
 * 
 * Shows that you can chain multiple worksharing constructs with nowait,
 * but you MUST ensure there are no dependencies between them.
 */
void example_chained_nowait(void)
{
    double *a = (double *)malloc(N * sizeof(double));
    double *b = (double *)malloc(N * sizeof(double));
    double *c = (double *)malloc(N * sizeof(double));
    double t_start, t_elapsed;
    
    printf("\n=== Example 4: Chained independent loops with nowait ===\n");
    
    t_start = omp_get_wtime();
    #pragma omp parallel
    {
        // All three loops are independent - safe to use nowait on all
        #pragma omp for nowait
        for (int i = 0; i < N; i++)
            a[i] = sin((double)i);
        
        #pragma omp for nowait
        for (int i = 0; i < N; i++)
            b[i] = cos((double)i);
        
        #pragma omp for nowait
        for (int i = 0; i < N; i++)
            c[i] = tan((double)i * 0.001);  // Small argument to avoid overflow
        
    }  // Implicit barrier at end of parallel region
    t_elapsed = omp_get_wtime() - t_start;
    
    printf("Time for 3 independent loops with nowait: %.4f s\n", t_elapsed);
    
    // Compare with barriers
    t_start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp for
        for (int i = 0; i < N; i++)
            a[i] = sin((double)i);
        
        #pragma omp for
        for (int i = 0; i < N; i++)
            b[i] = cos((double)i);
        
        #pragma omp for
        for (int i = 0; i < N; i++)
            c[i] = tan((double)i * 0.001);
    }
    t_elapsed = omp_get_wtime() - t_start;
    
    printf("Time for 3 loops with barriers:           %.4f s\n", t_elapsed);
    
    free(a);
    free(b);
    free(c);
}

/*
 * Example 5: Partial dependency - careful use of nowait
 * 
 * Sometimes only SOME loops have dependencies. You can use nowait
 * selectively on the independent ones.
 */
void example_partial_dependency(void)
{
    double *a = (double *)malloc(N * sizeof(double));
    double *b = (double *)malloc(N * sizeof(double));
    double *c = (double *)malloc(N * sizeof(double));
    
    printf("\n=== Example 5: Partial dependencies ===\n");
    
    #pragma omp parallel
    {
        // Loop 1: compute a[] - independent
        #pragma omp for nowait
        for (int i = 0; i < N; i++)
            a[i] = sin((double)i);
        
        // Loop 2: compute b[] - independent of a[]
        #pragma omp for  // NEED barrier here because loop 3 reads a[]
        for (int i = 0; i < N; i++)
            b[i] = cos((double)i);
        
        // Loop 3: compute c[] from a[] - depends on loop 1!
        // The barrier after loop 2 ensures loop 1 is done
        #pragma omp for
        for (int i = 0; i < N; i++)
            c[i] = a[i] + b[i];
    }
    
    // Verify
    int errors = 0;
    for (int i = 0; i < 1000; i++) {
        double expected = sin((double)i) + cos((double)i);
        if (fabs(c[i] - expected) > 1e-10) errors++;
    }
    printf("Errors (should be 0): %d\n", errors);
    
    free(a);
    free(b);
    free(c);
}

/*
 * Example 6: Demonstrating load imbalance benefit
 * 
 * When work is unevenly distributed, nowait helps because fast threads
 * don't have to wait for slow threads before starting new work.
 */
void example_load_imbalance(void)
{
    int chunk_sizes[] = {N/2, N/4, N/8, N/8};  // Very uneven distribution
    double *a = (double *)malloc(N * sizeof(double));
    double t_start, t_elapsed;
    
    printf("\n=== Example 6: Load imbalance ===\n");
    printf("Work distribution: 50%%, 25%%, 12.5%%, 12.5%% per thread\n");
    
    // Simulate imbalanced work
    t_start = omp_get_wtime();
    #pragma omp parallel num_threads(4)
    {
        int tid = omp_get_thread_num();
        int start = 0;
        for (int t = 0; t < tid; t++) start += chunk_sizes[t];
        int end = start + chunk_sizes[tid];
        
        // Imbalanced loop 1
        for (int i = start; i < end; i++)
            a[i] = sin((double)i);
        
        #pragma omp barrier  // Everyone waits for thread 0 (doing 50% of work)
        
        // Imbalanced loop 2 (same distribution)
        for (int i = start; i < end; i++)
            a[i] = cos(a[i]);
    }
    t_elapsed = omp_get_wtime() - t_start;
    printf("Time with barrier:  %.4f s\n", t_elapsed);
    
    // With nowait, fast threads start loop 2 while thread 0 finishes loop 1
    t_start = omp_get_wtime();
    #pragma omp parallel num_threads(4)
    {
        int tid = omp_get_thread_num();
        int start = 0;
        for (int t = 0; t < tid; t++) start += chunk_sizes[t];
        int end = start + chunk_sizes[tid];
        
        // Imbalanced loop 1
        for (int i = start; i < end; i++)
            a[i] = sin((double)i);
        
        // No barrier - fast threads proceed
        
        // Imbalanced loop 2 (but each thread works on its own portion)
        for (int i = start; i < end; i++)
            a[i] = cos(a[i]);
    }  // Barrier at end of parallel region
    t_elapsed = omp_get_wtime() - t_start;
    printf("Time without barrier: %.4f s (threads work on own data)\n", t_elapsed);
    
    free(a);
}

int main(int argc, char *argv[])
{
    printf("OpenMP nowait Clause Examples\n");
    printf("=============================\n");
    printf("Using %d threads\n", omp_get_max_threads());
    
    example_independent_loops();
    example_dependent_loops();
    example_single_nowait();
    example_chained_nowait();
    example_partial_dependency();
    example_load_imbalance();
    
    printf("\n=== Key Takeaways ===\n");
    printf("1. Use nowait when the next work does NOT depend on current results\n");
    printf("2. Removing barriers helps when there's load imbalance\n");
    printf("3. If in doubt, KEEP the barrier - correctness > performance\n");
    printf("4. The parallel region still has an implicit barrier at the end\n");
    printf("5. Test carefully: race conditions from wrong nowait are subtle!\n");
    
    return 0;
}
