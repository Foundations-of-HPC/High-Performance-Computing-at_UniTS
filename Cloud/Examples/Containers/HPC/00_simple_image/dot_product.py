import numpy as np
import sys
import time


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 10_000_000

    print(f"Generating two random vectors of length {n:,} ...")
    rng = np.random.default_rng(seed=42)
    a = rng.standard_normal(n)
    b = rng.standard_normal(n)

    t0 = time.perf_counter()
    result = np.dot(a, b)
    elapsed = time.perf_counter() - t0

    print(f"Dot product        = {result:.6e}")
    print(f"Expected (~0 for N(0,1) vectors) = ~{0.0:.6f} ± {np.sqrt(n):.1f}")
    print(f"Wall-clock time    = {elapsed*1000:.2f} ms")
    print(f"Throughput         = {(n/elapsed)/1e9:.2f} Gflop/s  (2N flops)")


if __name__ == "__main__":
    main()
