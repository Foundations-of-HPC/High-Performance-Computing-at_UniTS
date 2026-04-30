"""
server.py — a small HPC-style compute service.

Exposes a REST endpoint that solves a dense linear system Ax = b using NumPy.
The workload is realistic enough to saturate a CPU on moderate sizes, small
enough to run in seconds on a laptop. It is intentionally CPU-bound and
stateless — the canonical shape of a containerised compute worker.

Endpoints:
    GET  /health        liveness check
    POST /solve         body: {"n": int, "seed": int}
                        response: timing + residual of A@x - b

Configuration (environment):
    COMPUTE_MAX_N       maximum allowed problem size (default 2000)
    COMPUTE_LOG_LEVEL   log level (default "INFO")
"""

import logging
import os
import socket
import time

import numpy as np
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field


LOG_LEVEL = os.environ.get("COMPUTE_LOG_LEVEL", "INFO")
MAX_N = int(os.environ.get("COMPUTE_MAX_N", "2000"))

logging.basicConfig(
    level=getattr(logging, LOG_LEVEL),
    format="%(asctime)s [%(levelname)s] %(message)s",
)
log = logging.getLogger("compute")

app = FastAPI(title="Compute Service", version="1.0")


class SolveRequest(BaseModel):
    n: int = Field(..., ge=1, description="size of the linear system")
    seed: int = Field(0, description="PRNG seed for reproducibility")


class SolveResponse(BaseModel):
    n: int
    seed: int
    hostname: str
    elapsed_ms: float
    residual_norm: float
    solution_head: list[float]


@app.get("/health")
def health():
    return {"status": "ok", "hostname": socket.gethostname()}


@app.post("/solve", response_model=SolveResponse)
def solve(req: SolveRequest):
    if req.n > MAX_N:
        raise HTTPException(
            status_code=400,
            detail=f"n={req.n} exceeds COMPUTE_MAX_N={MAX_N}",
        )

    log.info("solve n=%d seed=%d", req.n, req.seed)

    rng = np.random.default_rng(seed=req.seed)
    # Build a well-conditioned symmetric positive-definite matrix:
    # A = M @ M.T + n*I ensures SPD with controlled conditioning
    M = rng.standard_normal((req.n, req.n))
    A = M @ M.T + req.n * np.eye(req.n)
    b = rng.standard_normal(req.n)

    t0 = time.perf_counter()
    x = np.linalg.solve(A, b)
    elapsed = time.perf_counter() - t0

    residual = np.linalg.norm(A @ x - b)

    log.info("  done in %.2f ms, residual=%.2e", elapsed * 1000, residual)

    return SolveResponse(
        n=req.n,
        seed=req.seed,
        hostname=socket.gethostname(),
        elapsed_ms=elapsed * 1000,
        residual_norm=float(residual),
        solution_head=x[:5].tolist(),
    )

