# Multi-stage Dockerfile — separating build environment from runtime
#
# Stage 1 ("builder"): full GCC toolchain (~1 GB). Used only at build time.
# Stage 2 ("runtime"): debian:slim with just libgomp (~80 MB). Shipped.
#
# The resulting image is ~10× smaller than one based on gcc:13, and
# contains no compilers — a security benefit (no gcc on the production
# node), a size benefit (faster push/pull/deploy), and a clarity benefit
# (the shipped image contains exactly the artefacts it needs to run).
#
# Usage:
#     docker build -t mc-pi:1.0 .
#     docker run --rm mc-pi:1.0
#     docker run --rm -e OMP_NUM_THREADS=4 --cpuset-cpus=0-3 mc-pi:1.0

# =====================================================================
# Stage 1: BUILDER
# =====================================================================
# gcc:13 is an official image with GCC 13 and a full build environment.
# ~900 MB. We will not ship this image — it exists only as a build tool.
FROM gcc:13 AS builder

WORKDIR /build

# Copy only the sources needed to build
COPY mc_pi_omp.c Makefile ./

# Build. -march=x86-64-v3 targets a modern x86_64 baseline (AVX2,
# Haswell-and-above) rather than -march=native, so the binary is
# portable to any reasonably modern x86_64 machine — not just the
# exact CPU we built on. For HPC image portability this matters.
RUN gcc -O3 -march=x86-64-v3 -fopenmp -Wall -Wextra \
        -o mc_pi_omp mc_pi_omp.c -fopenmp

# Optional: verify the binary runs in the build stage (fails build
# if the compile-time checks pass but the program is broken).
RUN MC_SAMPLES=1000000 MC_REPEATS=1 ./mc_pi_omp

# =====================================================================
# Stage 2: RUNTIME
# =====================================================================
# debian:12-slim is ~80 MB. No compilers, no development headers —
# just a C runtime and some utilities. We add exactly one library
# — libgomp — because our binary needs the OpenMP runtime.
FROM debian:12-slim AS runtime

# Install the OpenMP runtime library only.
# --no-install-recommends keeps the image small.
# Clean apt caches in the same RUN layer to avoid bloat.
RUN apt-get update && \
    apt-get install -y --no-install-recommends libgomp1 && \
    rm -rf /var/lib/apt/lists/*

# Non-root user, same rationale as Chapter 8
RUN useradd --create-home --shell /bin/bash runner
USER runner
WORKDIR /home/runner

# Copy ONLY the compiled binary from the builder stage.
# The --from=builder flag references the build stage by name.
COPY --from=builder /build/mc_pi_omp /usr/local/bin/mc_pi_omp

# Default command
CMD ["/usr/local/bin/mc_pi_omp"]
