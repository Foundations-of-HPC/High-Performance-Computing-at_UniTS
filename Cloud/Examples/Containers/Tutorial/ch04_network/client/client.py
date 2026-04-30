"""
client.py — submits linear-system solve requests to the compute service.

Reads the service URL from the environment so the same client works when
the service runs on localhost (with port mapping) or on another container
reached by name via a user-defined Docker network.

Environment:
    COMPUTE_URL         URL of the compute service (default http://localhost:8000)
    N_REQUESTS          number of sequential requests (default 5)
    PROBLEM_SIZE        size of each linear system (default 500)

Usage (examples):
    # against a service published on the host port 8000
    COMPUTE_URL=http://localhost:8000 python3 client.py

    # against a service reachable by container name on a Docker network
    COMPUTE_URL=http://compute:8000 python3 client.py
"""

import json
import os
import sys
import time

import requests


def main():
    base_url = os.environ.get("COMPUTE_URL", "http://localhost:8000")
    n_req = int(os.environ.get("N_REQUESTS", 5))
    size = int(os.environ.get("PROBLEM_SIZE", 500))

    print(f"client -> {base_url}")
    print(f"  submitting {n_req} requests, n={size}")
    print()

    # Health check first so misconfiguration fails loudly
    try:
        r = requests.get(f"{base_url}/health", timeout=5)
        r.raise_for_status()
        print(f"  health -> {r.json()}")
    except requests.RequestException as e:
        print(f"  health check FAILED: {e}", file=sys.stderr)
        sys.exit(1)

    print()
    total = 0.0
    for i in range(n_req):
        payload = {"n": size, "seed": i}
        t0 = time.perf_counter()
        r = requests.post(f"{base_url}/solve", json=payload, timeout=60)
        wall = time.perf_counter() - t0
        r.raise_for_status()

        resp = r.json()
        total += wall
        print(
            f"  [{i+1}/{n_req}] server={resp['hostname']:<14} "
            f"compute={resp['elapsed_ms']:7.1f} ms  "
            f"wall={wall*1000:7.1f} ms  "
            f"residual={resp['residual_norm']:.2e}"
        )

    print()
    print(f"  total wall time = {total*1000:.1f} ms across {n_req} requests")


if __name__ == "__main__":
    main()

