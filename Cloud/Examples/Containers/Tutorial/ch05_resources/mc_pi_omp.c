/*
 * mc_pi_omp.c  —  Monte Carlo π estimation, parallelised with OpenMP.
 *
 * Each thread draws N_SAMPLES/nthreads points in the unit square and
 * counts those inside the quarter unit circle. The final estimate is
 * 4 * hits / N_SAMPLES.
 *
 * Configuration via environment variables:
 *   MC_SAMPLES    number of Monte Carlo samples   (default 200_000_000)
 *   MC_REPEATS    how many times to repeat         (default 3)
 *
 * OpenMP controls:
 *   OMP_NUM_THREADS       number of threads (Docker: pair with --cpuset-cpus)
 *   OMP_PROC_BIND         "true" to pin threads to cores
 *   OMP_PLACES            "cores" or "threads" to describe the topology
 *
 * This program is intentionally compute-bound and has a negligible memory
 * footprint — good for studying CPU scaling without confounding from
 * memory bandwidth.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

/* xorshift64* — a small, fast, thread-safe PRNG.
 * One state per thread; each thread is seeded differently so the streams
 * do not overlap. Good enough for Monte Carlo estimation; not for
 * cryptography or high-quality statistics. */
static inline unsigned long long xorshift64s(unsigned long long *s) {
    unsigned long long x = *s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *s = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* Convert a 64-bit unsigned into a double in [0,1). */
static inline double u64_to_unit(unsigned long long x) {
    /* Take the top 53 bits to fit the mantissa exactly. */
    return (x >> 11) * (1.0 / 9007199254740992.0);
}

static long long count_inside(long long n_samples, unsigned long long base_seed) {
    long long inside = 0;

    #pragma omp parallel reduction(+:inside)
    {
        const int tid   = omp_get_thread_num();
        const int nthr  = omp_get_num_threads();

        /* Each thread takes a disjoint chunk of the total sample count. */
        const long long per_thread = n_samples / nthr;
        const long long remainder  = n_samples - per_thread * nthr;
        const long long my_count   = per_thread + (tid < remainder ? 1 : 0);

        /* Per-thread PRNG state, seeded from (base_seed, tid). */
        unsigned long long state = base_seed ^ (0x9E3779B97F4A7C15ULL * (unsigned long long)(tid + 1));
        if (state == 0) state = 1; /* xorshift cannot start from 0 */

        long long local = 0;
        for (long long i = 0; i < my_count; ++i) {
            double x = u64_to_unit(xorshift64s(&state));
            double y = u64_to_unit(xorshift64s(&state));
            if (x * x + y * y <= 1.0) ++local;
        }
        inside += local;
    }
    return inside;
}

int main(void) {
    /* Read configuration from environment. */
    const char *env_samples = getenv("MC_SAMPLES");
    const char *env_repeats = getenv("MC_REPEATS");

    long long n_samples = env_samples ? atoll(env_samples) : 200000000LL;
    int       n_repeats = env_repeats ? atoi(env_repeats)  : 3;
    if (n_samples < 1 || n_repeats < 1) {
        fprintf(stderr, "Invalid MC_SAMPLES or MC_REPEATS\n");
        return 1;
    }

    /* Report configuration and OpenMP view of the world. */
    int max_threads = omp_get_max_threads();
    printf("Monte Carlo Pi — OpenMP\n");
    printf("  samples/run      = %lld\n", n_samples);
    printf("  repeats          = %d\n", n_repeats);
    printf("  OMP max threads  = %d\n", max_threads);

    /* Best timing across repeats — the standard microbenchmark discipline
     * (warmed-up pages, stable frequency, minimum observed time). */
    double best = 1e300;
    double last_pi = 0.0;
    for (int r = 0; r < n_repeats; ++r) {
        double t0 = omp_get_wtime();
        long long inside = count_inside(n_samples, 0xC0FFEEULL + (unsigned)r);
        double dt = omp_get_wtime() - t0;
        double pi = 4.0 * (double)inside / (double)n_samples;
        printf("  run %d: %.3f s   pi=%.6f\n", r + 1, dt, pi);
        if (dt < best) best = dt;
        last_pi = pi;
    }

    double throughput = (double)n_samples / best / 1.0e6; /* Msamples/s */
    printf("\n");
    printf("Best time        = %.3f s\n", best);
    printf("Throughput       = %.1f Msamples/s\n", throughput);
    printf("Pi estimate      = %.6f\n", last_pi);
    return 0;
}

