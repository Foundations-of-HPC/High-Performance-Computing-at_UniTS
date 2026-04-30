"""
memhog.py — controlled memory allocator for Docker memory-limit demos.

Allocates NumPy arrays of MB_PER_STEP megabytes at a time, each one
zero-initialised, touching every byte so the pages are actually committed
(NumPy allocation is lazy otherwise). Reports resident memory after each
allocation.

Environment:
    TARGET_MB       total memory to allocate in MB (default 1024)
    MB_PER_STEP     size of each allocation step (default 128)
    SLEEP_SEC       seconds to pause between steps (default 0.2)
                    — helps you watch `docker stats` live
    HOLD_SEC        seconds to hold memory once target is reached (default 5)

Typical demos:
    # unconstrained — should succeed fast
    TARGET_MB=500 python3 memhog.py

    # under a container limit smaller than the target — will OOM
    docker run --memory=256m ... TARGET_MB=500 python3 memhog.py
"""

import os
import sys
import time

import numpy as np


def get_rss_mb() -> float:
    """Return current process resident set size in megabytes.

    Uses /proc/self/status, which is always present under Linux (hence
    inside every Linux container). Returns -1 if unavailable.
    """
    try:
        with open("/proc/self/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    # line is like "VmRSS:\t   12345 kB"
                    kb = int(line.split()[1])
                    return kb / 1024.0
    except FileNotFoundError:
        pass
    return -1.0


def main():
    target_mb = int(os.environ.get("TARGET_MB", 1024))
    step_mb   = int(os.environ.get("MB_PER_STEP", 128))
    sleep_s   = float(os.environ.get("SLEEP_SEC", 0.2))
    hold_s    = float(os.environ.get("HOLD_SEC", 5.0))

    print(f"memhog — target={target_mb} MB, step={step_mb} MB")
    print(f"  initial RSS = {get_rss_mb():.1f} MB")
    print()

    # Hold references so the arrays are not garbage-collected.
    held = []
    allocated_mb = 0

    while allocated_mb < target_mb:
        this_step = min(step_mb, target_mb - allocated_mb)
        # Allocate a float64 array: 8 bytes per element.
        n_elems = (this_step * 1024 * 1024) // 8
        # np.zeros() is lazy on Linux — pages are only committed on first
        # touch. We force actual allocation with .fill() so the memory
        # pressure is real.
        buf = np.zeros(n_elems, dtype=np.float64)
        buf.fill(1.0)
        held.append(buf)

        allocated_mb += this_step
        rss = get_rss_mb()
        print(f"  +{this_step:4d} MB  cumulative={allocated_mb:5d} MB  RSS={rss:6.1f} MB",
              flush=True)
        time.sleep(sleep_s)

    print()
    print(f"target reached, holding for {hold_s} s ...", flush=True)
    time.sleep(hold_s)
    print(f"  final RSS = {get_rss_mb():.1f} MB")
    print("done.")


if __name__ == "__main__":
    try:
        main()
    except MemoryError:
        # A Python-level MemoryError is rare in Docker — the OOM killer
        # usually gets there first and SIGKILLs the process before Python
        # sees the failure. Still, print something if we somehow catch it.
        rss = get_rss_mb()
        print(f"\nMemoryError raised at RSS={rss:.1f} MB", file=sys.stderr)
        sys.exit(137)  # match OOM-kill convention

