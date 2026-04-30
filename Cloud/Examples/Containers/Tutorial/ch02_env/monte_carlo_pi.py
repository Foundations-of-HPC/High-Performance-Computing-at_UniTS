"""
monte_carlo_pi.py — estimate π using the Monte Carlo method.

Generates N random points in the unit square [0,1] x [0,1]. Counts those
falling inside the quarter unit circle (x^2 + y^2 <= 1). The ratio
multiplied by 4 estimates π.

Configuration is read from environment variables — this is deliberate:
containerised workloads almost always configure themselves through env
vars rather than command-line arguments, because Docker makes env vars
easy to pass.

Environment variables:
    MC_ITERATIONS   number of samples (default: 10_000_000)
    MC_SEED         PRNG seed for reproducibility (default: 42)
    MC_VERBOSE      if "1", print progress (default: "0")
"""

import math
import os
import time

import numpy as np


def estimate_pi(n_samples: int, seed: int, verbose: bool = False) -> tuple[float, float]:
    """Return (pi_estimate, elapsed_seconds)."""
    rng = np.random.default_rng(seed=seed)

    t0 = time.perf_counter()
    # Vectorised sampling — much faster than a Python loop
    x = rng.random(n_samples)
    y = rng.random(n_samples)
    inside = (x * x + y * y <= 1.0).sum()
    elapsed = time.perf_counter() - t0

    if verbose:
        print(f"  drew {n_samples:,} samples, {inside:,} inside the circle")

    pi_estimate = 4.0 * inside / n_samples
    return pi_estimate, elapsed


def main():
    # Parse configuration from environment
    n = int(os.environ.get("MC_ITERATIONS", 10_000_000))
    seed = int(os.environ.get("MC_SEED", 42))
    verbose = os.environ.get("MC_VERBOSE", "0") == "1"

    print(f"Monte Carlo π estimation")
    print(f"  iterations = {n:,}")
    print(f"  seed       = {seed}")

    pi_hat, elapsed = estimate_pi(n, seed, verbose=verbose)
    error = abs(pi_hat - math.pi)
    rel_error = error / math.pi

    print(f"π estimate         = {pi_hat:.8f}")
    print(f"π (reference)      = {math.pi:.8f}")
    print(f"absolute error     = {error:.2e}")
    print(f"relative error     = {rel_error:.2e}")
    print(f"wall-clock time    = {elapsed*1000:.2f} ms")
    print(f"throughput         = {(n/elapsed)/1e6:.2f} Msamples/s")


if __name__ == "__main__":
    main()

