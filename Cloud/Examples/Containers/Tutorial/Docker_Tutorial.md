# Docker for HPC — A Hands-on Tutorial

**Course:** High Performance and Cloud Computing 2025/2026
**Programme:** Applied Data Science & AI — HPC Track, University of Trieste
**Companion to:** Lecture 04 (Containers)

---


---

## Table of contents

0. **Prerequisites and setup** — Docker vs Podman, verification
1. **Your first scientific container** — running Python+NumPy workloads
2. **Environment variables** — the canonical way to configure a container
3. **Container lifecycle** — start, stop, logs, exec, cleanup
4. **Networking between containers** — service-to-service, port mapping, user-defined networks
5. **CPU pinning and resource limits** — OpenMP scaling inside a container
6. **Memory limits and the OOM killer** — predictable memory budgets
7. **Volumes and persistent data** — input data, result artefacts, caches
8. **Building your first image with Dockerfile** — from `docker run + bash -c` to a reproducible image
9. **Multi-stage builds and image optimisation** — production-grade HPC images
10. **From Docker to Apptainer on Leonardo** — the bridge to CINECA

---

# Chapter 0 — Prerequisites and setup

## 0.1 What you need

This tutorial assumes:

- A **Linux, macOS, or Windows/WSL2** machine. Native Windows `cmd`/PowerShell without WSL2 will work for most of the tutorial but some commands will differ; we will not cover those differences.
- A **terminal** (bash, zsh, or PowerShell — the commands are identical for the tutorial's purposes).
- **Docker Desktop** (on macOS and Windows) or **Docker Engine** (on Linux), or **Podman**. We will explain the difference in §0.3.
- **At least 4 GB of free disk space** for downloaded images.
- **Network access** — Docker pulls images from a registry (Docker Hub by default) the first time you run them.

We do *not* cover installation. Installation is OS-specific, and official instructions on [docs.docker.com](https://docs.docker.com/get-docker/) (or [podman.io](https://podman.io/docs/installation)) are kept up to date. If you have not installed Docker yet, install it now before continuing.

## 0.2 Verify your installation

Open a terminal and run:

```bash
docker --version
```

You should see something like:

```text
Docker version 24.0.7, build afdd53b
```

The exact version does not matter; anything ≥ 20.10 will do.

Next, run the classic smoke test:

```bash
docker run --rm hello-world
```

The first time, Docker will download a tiny image called `hello-world`, run it, and print a welcome message. The essential lines to look for are:

```text
Hello from Docker!
This message shows that your installation appears to be working correctly.
```

If you see this, you are ready to proceed. If not, troubleshoot your installation before continuing.

> **Warning — Linux permissions.** On Linux, `docker` commands typically require `sudo` unless your user is in the `docker` group. Adding your user to the `docker` group gives them effective root on the host (because Docker runs as root); that is a real security trade-off, but acceptable on a personal workstation. On a shared machine, prefer Podman (§0.3). Throughout this tutorial, we omit `sudo`; add it in front of every `docker` command if your setup requires it.

## 0.3 Docker vs Podman — which should you use?

Docker is the most widely used container engine, but it is not the only one. **Podman** is a drop-in replacement from Red Hat that has become standard in the HPC and enterprise-Linux worlds. It matters for us because:

- Podman is **rootless by default**: a normal user can run containers without any elevated privilege. This is essential on HPC clusters, where giving users `sudo` is unacceptable.
- Podman has **no daemon**: each container is a child process of the `podman` CLI, not of a long-running service. This is also aligned with how HPC job schedulers work.
- Podman is **command-line-compatible** with Docker: every `docker` command in this tutorial works by replacing `docker` with `podman`, with very few exceptions.
- Apptainer (which we will use on Leonardo in §10) shares Podman's philosophy: rootless, daemonless. Learning Docker alongside Podman prepares you for both.

For this tutorial, you can use either. Every command is shown as `docker`; if you use Podman, simply type `podman` instead. Where there is a behavioural difference, a **Podman note** will point it out.

> **HPC note.** On HPC clusters such as Leonardo at CINECA, you will *not* have Docker. You will have Apptainer (formerly Singularity) — a third engine specifically designed for HPC. Apptainer reads Docker images natively, so everything you build here will run there. This is the single most important fact about Docker for an HPC user: **Docker is your development environment; Apptainer is your deployment environment**. Chapter 10 closes this loop.

## 0.4 A word on image registries

A *container image* is a packaged, immutable snapshot containing an application and all its dependencies. Images are stored in **registries** — think of a registry as GitHub for container images. The default public registry is [Docker Hub](https://hub.docker.com/) (`docker.io`). When you ran `docker run hello-world`, Docker silently pulled `docker.io/library/hello-world:latest`.

Other important registries:

- **`quay.io`** — Red Hat's registry, hosts many scientific images.
- **`nvcr.io`** — NVIDIA's registry, hosts GPU-enabled scientific images (CUDA, cuDNN, PyTorch).
- **`ghcr.io`** — GitHub Container Registry.

You can pull from any of these by qualifying the image name: `docker pull nvcr.io/nvidia/pytorch:23.12-py3`.

## 0.5 Clean slate for the tutorial

Before proceeding, let's check what images and containers you have. Don't worry if it's empty.

```bash
docker images          # list downloaded images
docker ps -a           # list all containers (running + stopped)
```

If you're on a fresh installation, you should see only `hello-world`. If not, don't panic — nothing we do here will interfere with your existing images.

---

# Chapter 1 — Your first scientific container

The goal of this chapter is to run a real numerical workload inside a container. No toy examples: we will compute a 10-million-element dot product in Python/NumPy, inside a proper scientific Python environment.

You'll learn:

- How to run a container from a public image.
- How to mount your code into the container.
- How to run a one-off interactive command vs a batch job.
- Why the first run is slow and subsequent runs are fast.

## 1.1 The workload

Look at the file `./ch01_hello/dot_product.py`:

```python
"""
dot_product.py — a first scientific workload to run inside a container.
...
"""

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
```

It generates two random vectors of N(0,1)-distributed numbers, computes their dot product, and reports the result and timing. For two independent standard-normal vectors of length N, the dot product is approximately 0 with standard deviation $\sqrt{N}$ — this is a basic statistical check that the script is doing what it claims.

## 1.2 Running the workload — the wrong way

Your first instinct might be to just run it locally:

```bash
cd ch01_hello
python3 dot_product.py
```

If you have NumPy installed, this works. **But it depends on your system having NumPy**, which is exactly the reproducibility problem containers exist to solve. On a different machine, or in a year's time, or on a cluster, that assumption breaks.

Let's do it properly.

## 1.3 Running the workload — inside a container

Ensure you are in the tutorial's top directory (the one containing `ch01_hello/`). Then run:

```bash
docker run --rm \
    -v "$(pwd)/ch01_hello:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 dot_product.py"
```

The first time you run this, Docker will **pull the `python:3.11-slim` image** from Docker Hub. That takes a few tens of seconds and ~45 MB of disk. Then it will install NumPy (another few seconds) and run the script. On subsequent runs, the image is cached and startup takes under a second — though NumPy is still reinstalled every time, which we will fix in §1.6.

Your output should look similar to:

```text
Generating two random vectors of length 10,000,000 ...
Dot product        = -3.879523e+02
Expected (~0 for N(0,1) vectors) = ~0.000000 ± 3162.3
Wall-clock time    = 11.41 ms
Throughput         = 0.88 Gflop/s  (2N flops)
```

The exact value of the dot product will match yours (because we fixed the PRNG seed), but the timing will depend on your CPU.

## 1.4 What just happened — every flag explained

That command had five moving parts. Understanding each one is worth more than any high-level explanation.

### `docker run`

Creates a new container from an image and starts it. This is the workhorse command of the whole tutorial.

### `--rm`

Remove the container automatically when it exits. Without this flag, every `docker run` leaves behind a stopped container on your disk, which adds up fast. For one-off computations, **always** use `--rm`.

### `-v "$(pwd)/ch01_hello:/work"`

This is a **bind mount**. It takes a directory on your host (`$(pwd)/ch01_hello`, i.e. `ch01_hello/` relative to where you are) and makes it appear inside the container at `/work`. The container can read and write to it as if it were a local directory; changes are reflected on your host immediately.

This is how you get your code into a running container without having to rebuild an image every time you modify it. We will revisit bind mounts properly in Chapter 7 on volumes.

> **Podman note.** With Podman in rootless mode, bind-mounted files inside the container appear to be owned by the user who ran `podman`. With Docker, they often appear to be owned by UID 0 (root inside the container). If a script does something that depends on file ownership, this can matter.

### `-w /work`

Sets the **working directory** inside the container to `/work`. Without this, the container starts in `/` and your `python3 dot_product.py` would fail because `dot_product.py` would not be in the current directory.

### `python:3.11-slim`

The **image** to run. `python:3.11-slim` is the official Python image, based on a minimal Debian, with Python 3.11 pre-installed. The `-slim` tag means it excludes common build tools (`gcc`, `make`, ...) to keep the image small — about 45 MB instead of 900 MB for the full `python:3.11` image.

### `bash -c "pip install -q -r requirements.txt && python3 dot_product.py"`

The **command** to execute in the container. We ask bash to install NumPy (quiet mode) and then run the script. The `-c` flag tells bash to interpret the following string as a command.

> **Why not just `python3 dot_product.py`?** Because the image has Python but does not have NumPy. We need to install it first. In §1.6 and Chapter 8 we'll see the proper way to handle this: build a custom image that already has NumPy in it.

## 1.5 Trying a larger size

Edit the command to pass a command-line argument to the script. Because `dot_product.py` takes an optional positional argument, we can pass one:

```bash
docker run --rm \
    -v "$(pwd)/ch01_hello:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 dot_product.py 50000000"
```

You should see about five times the wall-clock time, with roughly the same throughput:

```text
Generating two random vectors of length 50,000,000 ...
Dot product        = -5.xxxe+02
...
Wall-clock time    = ~55 ms
Throughput         = ~1.8 Gflop/s
```

> **Try it.** Try `1_000_000` and `200_000_000`. Plot wall-clock time vs N on paper. You should see near-linear scaling — the dot product is memory-bandwidth-bound for large vectors. This is your first taste of an HPC phenomenon observable inside a container.

## 1.6 The "install every time" problem

The command in §1.3 reinstalls NumPy on every run. That is wasteful: 10 MB of download, 5 seconds of install time, for no good reason. There are two ways to fix this.

**Option A: build a custom image** that already has NumPy installed. This is the proper solution and we will cover it in Chapter 8.

**Option B: use an existing scientific Python image.** Docker Hub hosts many pre-built scientific images; one common choice is `continuumio/miniconda3` or `jupyter/scipy-notebook` (the latter is ~2 GB — large but complete). A simpler choice is to use a Python image and install NumPy into a persistent location — but that defeats the purpose of containers' reproducibility.

For now, accept the small inefficiency. The reinstall costs a few seconds; you will not notice it during this tutorial.

## 1.7 Interactive containers

Sometimes you want to *explore* an image — poke around, try things manually, see what Python version is installed, what system libraries exist. For that, run an interactive shell:

```bash
docker run --rm -it python:3.11-slim bash
```

The `-it` flags (short for `--interactive` + `--tty`) connect your terminal to the container's stdin, stdout, and stderr. You will land in a bash shell:

```text
root@a3b7c9de12f3:/#
```

That hexadecimal after `@` is the container's short ID. You are now inside a fully isolated Debian-based environment. Try:

```bash
root@a3b7c9de12f3:/# cat /etc/os-release
root@a3b7c9de12f3:/# python3 --version
root@a3b7c9de12f3:/# ls /
root@a3b7c9de12f3:/# exit
```

When you type `exit`, the container stops, and because of `--rm`, it is removed. Your host is exactly as it was before.

> **Try it.** Start an interactive container as above. Install a package with `pip install scipy`, import it with `python3 -c "import scipy; print(scipy.__version__)"`, then exit. Start a fresh container and try again — SciPy is gone. Each container is ephemeral; installs inside a container do not persist unless you commit a new image or mount a volume.

---

# Chapter 2 — Environment variables

## 2.1 Why environment variables matter for containers

A container should be **configurable without changing its contents**. The canonical way to do this is environment variables. You pass them from the host at `docker run` time, and your program reads them with `os.environ.get(...)` in Python, `getenv()` in C, and so on.

This convention is not arbitrary. It exists because:

- **Immutability of images.** A container image should be built once and deployed many times, with configuration varying across deployments. Hard-coding parameters in the image violates this.
- **12-factor principle.** Modern deployment practice (the [12-factor app](https://12factor.net/config)) prescribes environment variables for all configuration. Container tooling is built around this assumption.
- **No filesystem writes needed.** An env var is set before the process starts; there is no config file to mount, no write to a settings directory.

For HPC workloads specifically, environment variables are the right place for: problem size, PRNG seeds, number of threads, dataset paths, solver tolerances — anything the user might want to sweep without rebuilding the code.

## 2.2 The workload

Look at `./ch02_env/monte_carlo_pi.py`:

```python
"""
monte_carlo_pi.py — estimate π using the Monte Carlo method.
Reads all configuration from environment variables.
"""

import math
import os
import time
import numpy as np


def estimate_pi(n_samples: int, seed: int, verbose: bool = False):
    rng = np.random.default_rng(seed=seed)
    t0 = time.perf_counter()
    x = rng.random(n_samples)
    y = rng.random(n_samples)
    inside = (x * x + y * y <= 1.0).sum()
    elapsed = time.perf_counter() - t0
    if verbose:
        print(f"  drew {n_samples:,} samples, {inside:,} inside the circle")
    return 4.0 * inside / n_samples, elapsed


def main():
    n = int(os.environ.get("MC_ITERATIONS", 10_000_000))
    seed = int(os.environ.get("MC_SEED", 42))
    verbose = os.environ.get("MC_VERBOSE", "0") == "1"

    print(f"Monte Carlo π estimation")
    print(f"  iterations = {n:,}")
    print(f"  seed       = {seed}")

    pi_hat, elapsed = estimate_pi(n, seed, verbose=verbose)
    error = abs(pi_hat - math.pi)
    # ... (rest of output)
```

The script reads three env vars: `MC_ITERATIONS`, `MC_SEED`, `MC_VERBOSE`. Any of them missing falls back to a sensible default. This is the pattern you want.

The algorithm is textbook Monte Carlo: sample N uniform points in the unit square; count how many fall inside the quarter unit circle ($x^2 + y^2 \le 1$); multiply the fraction by 4 to estimate π. The error is O(1/√N), so doubling N reduces the error by √2.

## 2.3 Running with default configuration

From the top-level tutorial directory:

```bash
docker run --rm \
    -v "$(pwd)/ch02_env:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 monte_carlo_pi.py"
```

Expected output (timing will vary):

```text
Monte Carlo π estimation
  iterations = 10,000,000
  seed       = 42
π estimate         = 3.14130400
π (reference)      = 3.14159265
absolute error     = 2.89e-04
relative error     = 9.19e-05
wall-clock time    = ~367 ms
throughput         = ~27 Msamples/s
```

## 2.4 Passing environment variables with `-e`

The `-e` flag to `docker run` sets an env var inside the container. To ask for 100 million samples with a different seed:

```bash
docker run --rm \
    -e MC_ITERATIONS=100000000 \
    -e MC_SEED=7 \
    -v "$(pwd)/ch02_env:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 monte_carlo_pi.py"
```

Expected output:

```text
Monte Carlo π estimation
  iterations = 100,000,000
  seed       = 7
π estimate         = 3.14139716
π (reference)      = 3.14159265
absolute error     = 1.95e-04
relative error     = 6.22e-05
wall-clock time    = ~3300 ms
throughput         = ~30 Msamples/s
```

Notice that the error decreased from ~2.9e-04 to ~1.9e-04 — roughly a factor of √10 reduction for a 10× increase in samples, exactly as Monte Carlo theory predicts.

> **Try it.** Run the script three times with the same `MC_ITERATIONS=10000000` but three different seeds (say 1, 2, 3). Note that each gives a different estimate of π, each within ~1e-3 of the true value. The spread gives you an empirical confidence interval — another important HPC/statistics concept visible in containerised form.

## 2.5 Convenience: passing existing host env vars

If an env var is already set in your host shell, you can forward it with `-e NAME` (no value):

```bash
export MC_ITERATIONS=50000000
docker run --rm \
    -e MC_ITERATIONS \
    -v "$(pwd)/ch02_env:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 monte_carlo_pi.py"
```

Docker reads `MC_ITERATIONS` from your current shell and passes its value into the container. This is handy when you drive the container from a larger shell script.

## 2.6 Environment files (`.env`)

For many variables, you can put them in a file and pass the file with `--env-file`:

Create a file `mc_config.env` with content:

```text
MC_ITERATIONS=50000000
MC_SEED=12345
MC_VERBOSE=1
```

Then run:

```bash
docker run --rm \
    --env-file ./mc_config.env \
    -v "$(pwd)/ch02_env:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 monte_carlo_pi.py"
```

With `MC_VERBOSE=1`, the script will also print the intermediate "drew X samples, Y inside the circle" line.

> **Warning — secrets in env files.** Never commit an `.env` file containing credentials to git. The file-based approach is convenient for non-sensitive configuration (problem size, seed, tolerances). For secrets (API keys, database passwords), use Docker secrets or a secrets manager — out of scope for this tutorial.

## 2.7 Listing environment variables inside a container

To see all env vars visible to a process inside a container:

```bash
docker run --rm -e MC_SEED=42 python:3.11-slim env
```

You will see `MC_SEED=42` in the output, alongside the Python image's own variables (`PATH`, `PYTHON_VERSION`, and a few more). The built-in `env` command prints every environment variable. This is useful for debugging: if your container is misbehaving, check what env vars it actually received.

## 2.8 Summary — the right pattern

The pattern to internalise:

1. Your script reads its configuration from `os.environ.get("VAR", default)`.
2. You pass configuration with `-e VAR=value` or `--env-file file.env`.
3. The image remains unchanged across runs — only the environment varies.

This is simple, scriptable, and maps cleanly onto the way HPC schedulers (SLURM) and cloud orchestrators (Kubernetes) pass configuration to jobs.

---

# Chapter 3 — Container lifecycle

Up to now we have only used `docker run --rm`, which creates, starts, and removes a container in a single step. For many HPC-style batch workloads, that is all you need. But Docker containers have a richer lifecycle, and understanding it saves you from a common class of beginner confusions — "where did my container go?", "why is my laptop's disk filling up?", "how do I get back into a running container?".

You'll learn:

- The states a container can be in, and how to list them.
- How to start, stop, restart, pause, and kill containers.
- How to see logs from a running or exited container.
- How to attach a shell to a running container with `docker exec`.
- How to clean up (and why you should).

## 3.1 The container state machine

A container can be in one of a few states:

```text
     docker run
        │
        ▼
    ┌───────┐   docker stop    ┌──────────┐
    │running├─────────────────▶│ exited   │
    └───┬───┘                  └────┬─────┘
        │                           │
        │ docker pause              │ docker start
        ▼                           ▼
    ┌───────┐                  (returns to running)
    │paused │
    └───┬───┘
        │ docker unpause
        ▼
    (returns to running)

    Any state → docker rm → GONE
```

- **Running** — the container's main process is executing.
- **Paused** — the process is frozen by the kernel (cgroup freezer). Rarely used in practice.
- **Exited** — the container has stopped. Its filesystem still exists on your host disk until you remove it. You can restart an exited container; its changes (e.g. installed packages) are preserved.
- **Removed** — the container is gone. Its filesystem is deleted.

Images are separate from containers and follow their own lifecycle (`docker pull`, `docker rmi`). A container is an instance of an image, like a process is an instance of a program.

## 3.2 A long-running container

To have something to experiment with, let's run a container that stays alive longer than an instant:

```bash
docker run -d --name my_demo python:3.11-slim \
    bash -c "for i in 1 2 3 4 5 6 7 8 9 10; do echo 'tick' \$i; sleep 2; done"
```

Breaking this down:

- `-d` (detach) runs the container in the background and returns immediately. Without it, your terminal would be attached to the container's output.
- `--name my_demo` gives the container a readable name. Without this, Docker generates a random one like `wonderful_hopper`.
- The command runs 10 iterations of "tick N" with a 2-second sleep — 20 seconds total.

You should see only a long hexadecimal string printed — the full container ID. The container is now running in the background.

## 3.3 Listing containers

```bash
docker ps
```

Shows **running** containers only:

```text
CONTAINER ID   IMAGE              COMMAND                  CREATED          STATUS          PORTS     NAMES
4b2c9a...      python:3.11-slim   "bash -c 'for i in 1…"   5 seconds ago    Up 4 seconds              my_demo
```

To see **all** containers including exited ones:

```bash
docker ps -a
```

## 3.4 Watching logs

```bash
docker logs my_demo
```

Prints everything the container has written to stdout/stderr so far. Add `-f` to follow (like `tail -f`):

```bash
docker logs -f my_demo
```

You will see new "tick N" lines appearing every 2 seconds. Press `Ctrl+C` to stop following (this does not stop the container itself, only your log-watching).

> **HPC note.** `docker logs` is your primary debugging tool when a containerised job misbehaves. Think of it as the container's equivalent of a SLURM `.out` file. When you eventually move to Apptainer on Leonardo, logs go to the SLURM `.out`/`.err` files instead, but the mental model is the same.

## 3.5 Exec: running a second process inside a running container

Sometimes you want to poke inside a running container without interrupting its main process. `docker exec` starts a new process in an already-running container:

```bash
docker exec -it my_demo bash
```

You now have a shell inside `my_demo`, running alongside the "tick" loop. Try:

```bash
root@4b2c9a...:/# ps aux
root@4b2c9a...:/# cat /etc/os-release
root@4b2c9a...:/# exit
```

The `ps aux` will show both the original `bash` (running the tick loop) and the new `bash` you just started. Your `exit` terminates only the second shell; the tick loop continues.

This is the command you will use most for interactive debugging of long-running containers.

## 3.6 Stopping a container

If the "tick" loop is still running, stop it:

```bash
docker stop my_demo
```

Docker sends `SIGTERM` to the main process and waits (up to 10 seconds by default) for it to exit. If it doesn't exit cleanly, Docker follows up with `SIGKILL`.

For a container that ignores signals (or to stop one immediately without giving it a chance to clean up), use `docker kill` instead:

```bash
docker kill my_demo
```

After stopping, check the state:

```bash
docker ps -a
```

You should see your container now has `STATUS` `Exited (N) M seconds ago`. It still exists on disk.

## 3.7 Restarting a container

An exited container can be restarted — its filesystem state is preserved between runs:

```bash
docker start my_demo
docker logs -f my_demo
```

`docker start` runs the container's original command again. If that command was `bash -c "for i in 1 2 3 ..."`, it restarts the loop from the beginning. If the command was a long-running server, the server would start up fresh.

## 3.8 Removing containers

To delete a stopped container:

```bash
docker stop my_demo       # ensure it's stopped first
docker rm my_demo
```

Or to forcibly remove a running container (stop + rm in one step):

```bash
docker rm -f my_demo
```

Clean up is important. Stopped containers can accumulate and take disk space for no benefit.

## 3.9 Cleanup commands

In a tutorial or during development, you will create many containers. Docker ships with cleanup helpers:

```bash
# Remove all stopped containers
docker container prune

# Remove dangling images (images not tagged, not used by any container)
docker image prune

# Remove everything unused: stopped containers, unused networks,
# dangling images, and build cache
docker system prune
```

Add `-a` to `docker system prune` to also remove **all** unused images (not only dangling ones). This can free gigabytes on a well-used developer machine. Be careful: `prune -a` removes images you might actually want to keep if they're not currently in use by a running container.

> **Warning — accidental deletion.** `docker system prune -a --volumes` also removes all volumes that are not in use. If you have persistent data in a named volume that is not currently mounted by any container, it **will be deleted**. We will discuss volumes in Chapter 7. For now, avoid the `--volumes` flag unless you know what you are doing.

## 3.10 The `--rm` shortcut, again

Recall that `docker run --rm` combines run-and-cleanup: the container is automatically removed when it exits. This is what you want for HPC-style batch workloads where each run is independent. It is what we have been using all along.

Use `docker run` without `--rm` when you want to:
- Inspect the container's state after it exits.
- Restart the same container multiple times (`docker start`).
- Keep logs accessible for post-mortem debugging.

Use `docker run --rm` when:
- The container produces all its output to stdout (which you can capture).
- You don't need to look inside after it finishes.
- You are running many short jobs and don't want clutter.

For this tutorial, `--rm` is the default. Only Chapter 3's demos have used named, non-`--rm` containers, because that was the whole point.

## 3.11 Summary of lifecycle commands

| Command | Purpose |
|---|---|
| `docker run` | Create + start a new container |
| `docker run -d` | Run in the background (detach) |
| `docker run --rm` | Auto-remove when the container exits |
| `docker ps` | List running containers |
| `docker ps -a` | List all containers |
| `docker logs NAME` | Show container's stdout/stderr |
| `docker logs -f NAME` | Follow logs live |
| `docker exec -it NAME CMD` | Run a new command in a running container |
| `docker stop NAME` | Send SIGTERM and wait |
| `docker kill NAME` | Send SIGKILL immediately |
| `docker start NAME` | Restart a stopped container |
| `docker rm NAME` | Delete a stopped container |
| `docker rm -f NAME` | Force delete (stop + rm) |
| `docker system prune` | Clean up stopped containers, dangling images, unused networks |

Memorise the first eight; the rest you can look up.

---

# Chapter 4 — Networking between containers

Everything so far has been a single, self-contained workload: one container, one computation, exit. Real HPC-adjacent deployments are rarely that simple. You often have a **long-running compute service** (a worker that exposes an API) and **clients** that submit jobs to it. Multiple containers must find each other, talk to each other, and expose themselves to the outside world selectively.

Docker provides three primitives that cover almost every case:

1. **Port mapping** (`-p`) to expose a container's port on the host.
2. **Docker networks** (user-defined bridges) to let containers talk to each other by name.
3. **Host networking** (`--network=host`) for the rare cases where you want a container to share the host's network stack directly.

You'll learn:

- How to expose a container port to your host.
- Why you almost never want to use `--link` (the old way).
- How to create a user-defined Docker network with automatic DNS.
- How to reach one container from another by name.
- The HPC-relevant trade-offs: bridged vs host networking.

## 4.1 The scenario

We'll build a small two-container system:

- A **compute service** — a FastAPI server that solves dense linear systems on demand. Stateless, CPU-bound, realistic shape of a containerised worker.
- A **client** — a small Python script that submits a batch of solve requests and reports timing.

Both are under `./ch04_network/`:

```text
ch04_network/
├── compute_service/
│   ├── server.py
│   └── requirements.txt
└── client/
    ├── client.py
    └── requirements.txt
```

`server.py` exposes two endpoints:

- `GET /health` — liveness check, returns the container's hostname.
- `POST /solve` — body `{"n": int, "seed": int}`, solves a random n×n symmetric positive-definite system `Ax = b` and returns timing + residual.

`client.py` reads `COMPUTE_URL` from the environment, submits `N_REQUESTS` solve requests of size `PROBLEM_SIZE`, and prints a per-request summary. The URL flexibility is the whole point: the same client code works whether the service is reached via port-mapped localhost or via a container-name hostname on a Docker network.

## 4.2 Starting the compute service with port mapping

Start the service, exposing its port 8000 on your host's port 8000:

```bash
docker run -d --rm \
    --name compute \
    -p 8000:8000 \
    -v "$(pwd)/ch04_network/compute_service:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && uvicorn server:app --host 0.0.0.0 --port 8000"
```

Explanation of each flag:

- `-d` runs the container in the background (detached). We will look at its logs separately.
- `--name compute` gives it a stable name for the commands that follow.
- `-p 8000:8000` is **port mapping**: connect host port 8000 to container port 8000. Anything listening on `0.0.0.0:8000` inside the container is reachable at `http://localhost:8000` on your host. The left side is the host port, the right the container port; `-p 8080:8000` would expose it on the host at 8080 instead.
- `uvicorn server:app --host 0.0.0.0 --port 8000` launches the FastAPI app. The `--host 0.0.0.0` is essential: by default, many web servers bind to `127.0.0.1`, which is only reachable from *within* the container and not through the Docker NAT. Binding to `0.0.0.0` means "all interfaces in my namespace," which is what port mapping requires.

Wait a few seconds for pip to install the dependencies (FastAPI, uvicorn, NumPy). Then verify the service is up:

```bash
docker logs compute
```

You should see uvicorn's startup lines:

```text
INFO:     Uvicorn running on http://0.0.0.0:8000 (Press CTRL+C to quit)
INFO:     Application startup complete.
```

And confirm it responds on your host:

```bash
curl http://localhost:8000/health
```

Expected:

```json
{"status":"ok","hostname":"a3b7c9de12f3"}
```

That `hostname` is the container's ID-derived hostname — a small but diagnostic detail. It confirms you're talking to the container, not to some accidental process on your host.

> **Warning — `--host 127.0.0.1` is a classic Docker mistake.** If you configure your server to bind only to `127.0.0.1`, port mapping will appear to work (the port is open on the host) but every request will hang or fail. Inside a container, `0.0.0.0` is the correct bind address for anything you want to expose.

## 4.3 Submitting requests from the host

Your host can reach the service via port 8000 just like any other local service. Without a container, a simple curl will do:

```bash
curl -X POST http://localhost:8000/solve \
    -H "Content-Type: application/json" \
    -d '{"n": 300, "seed": 1}'
```

Response (abbreviated):

```json
{"n":300,"seed":1,"hostname":"a3b7c9de12f3","elapsed_ms":12.5,"residual_norm":2.47e-12,"solution_head":[...]}
```

You have confirmed, end-to-end: a dense 300×300 SPD system solved by NumPy inside a container, reached from your host, with a tiny residual that proves the computation is correct.

## 4.4 Running the client as a container — the naive attempt

A natural next step is to run the client itself in a container. Let's try the obvious thing first — use `localhost` as the hostname:

```bash
docker run --rm \
    -e COMPUTE_URL=http://localhost:8000 \
    -e N_REQUESTS=3 \
    -e PROBLEM_SIZE=400 \
    -v "$(pwd)/ch04_network/client:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 client.py"
```

**This will fail.** You'll see an error like:

```text
health check FAILED: HTTPConnectionPool(host='localhost', port=8000): Max retries exceeded ... Connection refused
```

Why? Because inside the client container, `localhost` means *the client container itself*, not your host. The compute service is not running inside the client container. This is the fundamental insight about container networking: **each container has its own network namespace, and `localhost` is local to that namespace**.

There are three ways to fix this. We'll look at all three in order of HPC relevance.

## 4.5 Fix 1 — `host.docker.internal` (Mac/Windows Docker Desktop)

On Docker Desktop (macOS or Windows), there's a special DNS name `host.docker.internal` that resolves, from inside a container, to the host machine. So you can do:

```bash
docker run --rm \
    -e COMPUTE_URL=http://host.docker.internal:8000 \
    -e N_REQUESTS=3 \
    -e PROBLEM_SIZE=400 \
    -v "$(pwd)/ch04_network/client:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 client.py"
```

On Linux this name does *not* resolve by default — you'd need to pass `--add-host=host.docker.internal:host-gateway` to enable it. This hostname is an artefact of Docker Desktop's VM-based architecture.

> **HPC note.** Relying on `host.docker.internal` is a symptom of a deeper problem: you're treating the host as a server. In a containerised HPC workflow, *everything* should be inside a container, and containers should reach each other directly — without involving the host's network stack. That's what user-defined networks are for.

## 4.6 Fix 2 — user-defined networks with name-based DNS

The clean way to have two containers talk to each other is to put them on the **same user-defined Docker network**. Docker then provides automatic DNS: a container can reach another by its name, as if the name were a DNS hostname.

First, stop and remove the service started earlier:

```bash
docker stop compute
```

(Because we used `--rm`, `docker stop` also removes the container.)

Now create a user-defined bridge network:

```bash
docker network create hpc-net
```

This creates an isolated virtual network segment. Containers attached to it get private IPs, see each other by name, and are isolated from containers on other networks. Confirm:

```bash
docker network ls
```

You should see `hpc-net` alongside the default `bridge`, `host`, and `none` networks.

Now start the compute service **attached to `hpc-net`**:

```bash
docker run -d --rm \
    --name compute \
    --network hpc-net \
    -v "$(pwd)/ch04_network/compute_service:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && uvicorn server:app --host 0.0.0.0 --port 8000"
```

Two things changed. First, no `-p` flag — we don't need to expose the service to the host, because the client will also live on `hpc-net`. Second, `--network hpc-net` attaches the container to our custom network.

Wait for pip + startup (check with `docker logs compute` as before). Then run the client, also on `hpc-net`, using the service's name as the hostname:

```bash
docker run --rm \
    --network hpc-net \
    -e COMPUTE_URL=http://compute:8000 \
    -e N_REQUESTS=5 \
    -e PROBLEM_SIZE=600 \
    -v "$(pwd)/ch04_network/client:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 client.py"
```

Note `COMPUTE_URL=http://compute:8000`: the hostname `compute` is the service container's `--name`. Docker's embedded DNS resolves this name to the container's private IP on `hpc-net`.

Expected output:

```text
client -> http://compute:8000
  submitting 5 requests, n=600

  health -> {'status': 'ok', 'hostname': 'a3b7c9de12f3'}

  [1/5] server=a3b7c9de12f3  compute=   45.2 ms  wall=   48.7 ms  residual=5.12e-12
  [2/5] server=a3b7c9de12f3  compute=   43.1 ms  wall=   45.8 ms  residual=4.89e-12
  [3/5] server=a3b7c9de12f3  compute=   44.5 ms  wall=   47.2 ms  residual=5.01e-12
  [4/5] server=a3b7c9de12f3  compute=   43.8 ms  wall=   46.1 ms  residual=4.95e-12
  [5/5] server=a3b7c9de12f3  compute=   44.3 ms  wall=   46.9 ms  residual=5.08e-12

  total wall time = 234.7 ms across 5 requests
```

The numbers reveal a real HPC phenomenon: the *wall* time (what the client waited) is slightly larger than the server's reported *compute* time (what NumPy took). The gap — about 2–3 ms per request here — is the round-trip cost of HTTP + serialisation. For a real production pipeline, you would measure this gap and decide whether to use batching, keep-alive, or a binary protocol (gRPC, ZeroMQ) to reduce it.

## 4.7 Fix 3 — host networking

A third option, which HPC users sometimes reach for: `--network=host`. This makes the container share the host's network namespace entirely — no NAT, no bridge, no port mapping. The container's `localhost` *is* the host's `localhost`.

```bash
# Stop previous service
docker stop compute

# Start service with host networking
docker run -d --rm \
    --name compute \
    --network host \
    -v "$(pwd)/ch04_network/compute_service:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && uvicorn server:app --host 0.0.0.0 --port 8000"
```

No `-p` flag is needed because the container is using the host's network directly. `curl http://localhost:8000/health` from your host works instantly.

Host networking has two distinctive properties:

- **Performance**: zero NAT overhead. The container's network syscalls touch the host's network stack directly. For throughput-sensitive workloads (multi-gigabit network I/O, MPI), this matters.
- **Isolation**: gone. The container can bind any host port; port collisions with host services become possible. In multi-tenant environments, this is usually unacceptable.

> **HPC note.** Host networking is the closest Docker comes to the performance profile of Apptainer. On a dedicated HPC node where you trust your own container and do not care about isolation from the host, `--network host` gives you a genuine performance win for network-heavy workloads. This is why MPI-aware containers on a few HPC integrations (including some on CINECA) end up using host networking.
>
> On multi-tenant systems (cloud VMs, shared development workstations), do not use host networking unless you have a specific reason.

Clean up before moving on:

```bash
docker stop compute
docker network rm hpc-net
```

## 4.8 Podman and networking

Podman's networking model is almost but not exactly the same. Two differences worth knowing:

- Podman uses a different default backend (`netavark` on modern versions, replacing the older `CNI` plugins used in early Podman). Functionally transparent to the user for basic cases.
- Rootless Podman has a **restricted port range**: non-root containers cannot bind ports below 1024 on the host. If you need to publish on port 80 or 443, you either run Podman as root, use a higher host port (`-p 8080:80`), or configure `/etc/sysctl.d` to allow unprivileged binding.

For everything we did in this chapter, `podman` can replace `docker` directly:

```bash
podman network create hpc-net
podman run -d --name compute --network hpc-net ...
podman run --rm --network hpc-net -e COMPUTE_URL=http://compute:8000 ...
```

The DNS-by-container-name feature works identically.

## 4.9 Summary — the three patterns

| Pattern | When to use it | Command |
|---|---|---|
| Port mapping (`-p`) | Expose a service to your host or the outside world | `docker run -p 8000:8000 ...` |
| User-defined network | Two or more containers that need to talk to each other | `docker network create X; docker run --network X --name svc ...` |
| Host network (`--network host`) | Throughput-critical, single-tenant, trusted container | `docker run --network host ...` |

**Internalise one pattern in particular**: user-defined networks plus `--name` plus DNS-by-name. It covers 80% of multi-container scenarios and is the basis on which Docker Compose, Kubernetes, and any container orchestrator build more complex topologies.

---

# Chapter 5 — CPU pinning and resource limits

This is the first chapter where Docker's resource-control capabilities become directly relevant to HPC work. On a real cluster node — whether a laptop, a workstation, or a Leonardo compute node — you care very much about *how many CPUs a workload uses*, *which specific CPUs it runs on*, and *how that maps to the NUMA topology of the host*. Docker exposes these controls through a small set of flags, all implemented by Linux cgroups underneath.

You'll learn:

- How to limit a container to a specific number of CPUs.
- How to pin a container to specific physical CPU cores.
- How OpenMP discovers the thread count inside a container — and how to control it.
- What an OpenMP strong-scaling study looks like under Docker resource limits.
- The (important) difference between `--cpus` and `--cpuset-cpus`.

> **Linux-only note.** The scaling measurements in this chapter assume you are on a **Linux host with Docker Engine**. On Docker Desktop for macOS or Windows, the Docker daemon runs in a Linux VM, and the CPU affinity flags apply to *virtual* CPUs inside that VM, not the physical cores of your machine. The commands will all run without error, but benchmark numbers may not reflect the underlying hardware. On Leonardo and on any Linux workstation, everything behaves as described.

## 5.1 The workload

The file `./ch05_resources/mc_pi_omp.c` is an OpenMP-parallel Monte Carlo estimation of π. It is the parallel analogue of the Python script used in Chapters 2–3. Each thread draws a disjoint chunk of random points in the unit square and counts those inside the quarter unit circle; a final reduction gives the estimate `4 * hits / N`.

The computation is **embarrassingly parallel** — threads share no state during the main loop — and **compute-bound** — each sample is a handful of floating-point operations on values that live in registers. These two properties make it an ideal workload for studying CPU scaling: if your scaling is poor on this program, you know the cause is *not* memory bandwidth or synchronisation, and so it must be CPU allocation.

Configuration is via environment variables:

- `MC_SAMPLES` — total Monte Carlo samples per run (default 200 M)
- `MC_REPEATS` — number of timed repeats (default 3; we report the best)
- `OMP_NUM_THREADS` — standard OpenMP environment variable

A Makefile is provided to compile it with `gcc -O3 -march=native -fopenmp`.

## 5.2 Building a minimal OpenMP image

For this chapter we need a container with a C compiler *and* the OpenMP runtime. The stock `python:3.11-slim` image does not include GCC. We will build a small image on the fly using a base that does.

Rather than write a proper Dockerfile (that is Chapter 8), we use a one-liner approach: run a container based on `gcc:13` (an official image that bundles GCC), install no extras, compile the program inside, and execute it. This keeps us focused on *running* and *configuring* rather than on image construction.

Start by compiling once, so the binary is available for the rest of the chapter:

```bash
cd ch05_resources
docker run --rm -v "$(pwd):/work" -w /work gcc:13 make
```

You should see something like:

```text
gcc -O3 -march=native -fopenmp -Wall -Wextra -o mc_pi_omp mc_pi_omp.c -fopenmp
```

and an executable `mc_pi_omp` in the current directory. Because the bind mount is read-write, the binary produced inside the container is persisted on your host.

> **A subtlety.** The binary has been compiled with `-march=native` inside a container whose kernel and CPU are the host's. This means the binary uses the instruction set extensions of the host CPU (AVX2, AVX-512, ...). If you later run this binary in a container on a different host with a less capable CPU, it will fail with an illegal-instruction trap. In production HPC, you pick a specific, conservative `-march` target (e.g. `-march=x86-64-v3` for a modern baseline) that matches the lowest common denominator of your target hardware.

## 5.3 A first run without any limits

Run the binary under a container with no resource limits, so you see the baseline behaviour:

```bash
docker run --rm -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp
```

You should see something like:

```text
Monte Carlo Pi — OpenMP
  samples/run      = 200000000
  repeats          = 3
  OMP max threads  = 16
  run 1: 0.42 s   pi=3.141583
  run 2: 0.41 s   pi=3.141578
  run 3: 0.41 s   pi=3.141580

Best time        = 0.41 s
Throughput       = 485.4 Msamples/s
Pi estimate      = 3.141580
```

The `OMP max threads = 16` line is the important one. By default, OpenMP sets the number of threads equal to the number of hardware threads the host exposes (your laptop's core count, typically 4–16). The container inherits *all* of them — Docker applies no CPU limits by default.

## 5.4 Limiting the number of CPUs — `--cpus`

To restrict the container to a specific quantity of CPU, pass `--cpus=N`:

```bash
docker run --rm --cpus=2 -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp
```

Two effects, both worth understanding separately:

- The cgroup CPU quota is set so the container receives at most `2.0` CPU-seconds per wall-clock second. If the container tries to use more, the scheduler throttles it.
- **But**: the container still *sees* all host CPUs. `OMP_NUM_THREADS` will still default to 16, because OpenMP looks at `/proc/cpuinfo`, which is not virtualised by cgroups.

Run the command above and look at the output. You will see `OMP max threads = 16` again — but the best time will be about `8×` slower than in §5.3, because 16 threads are contending for 2 CPUs' worth of quota:

```text
  OMP max threads  = 16
  run 1: 3.1 s   pi=3.141583
  ...
Best time        = 3.1 s
```

This is a confusing failure mode that HPC newcomers regularly hit. The fix is to **tell OpenMP explicitly how many threads to use**, matching the container's CPU allocation:

```bash
docker run --rm --cpus=2 \
    -e OMP_NUM_THREADS=2 \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp
```

Now:

```text
  OMP max threads  = 2
  run 1: 0.81 s   pi=3.141580
  ...
Best time        = 0.81 s
Throughput       = 247.0 Msamples/s
```

Compared to the 16-thread run of §5.3 (0.41 s), two threads take almost exactly twice as long. This is good scaling: the overhead of going from 2 threads to 16 is minimal, because the workload is embarrassingly parallel.

> **The general rule.** Whenever you constrain a container's CPU budget, also set `OMP_NUM_THREADS` (and `MKL_NUM_THREADS`, `OPENBLAS_NUM_THREADS`, `NUMBA_NUM_THREADS` — every threading library has its own variable). The cost of *not* doing this is oversubscription: more threads than cores, leading to context-switch storms and unpredictable performance.

## 5.5 Pinning to specific cores — `--cpuset-cpus`

`--cpus=N` says *how much* CPU; `--cpuset-cpus` says *which ones*. The two have different semantics, and on HPC the difference matters.

```bash
docker run --rm --cpuset-cpus="0,1" \
    -e OMP_NUM_THREADS=2 \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp
```

`--cpuset-cpus="0,1"` restricts the container to physical CPUs 0 and 1 specifically — *these particular cores* and no others. The container's threads are scheduled only on these cores.

The syntax accepts:
- a list: `"0,2,4,6"` — only these four cores
- a range: `"0-3"` — cores 0 through 3
- combinations: `"0-1,4-5"` — cores 0, 1, 4, 5

Pinning matters for two reasons:

**Cache locality.** Threads pinned to specific cores benefit from warm L1/L2 caches and (on NUMA systems) local-socket memory. Letting the kernel freely migrate threads across sockets introduces latency spikes as caches cool down.

**Reproducibility of benchmarks.** When measuring performance, you want to eliminate sources of variance. Pinning ensures that each run uses the same cores, eliminating variability from the kernel's scheduling decisions.

### The combination with OpenMP's own pinning

OpenMP has its own thread-affinity mechanism, controlled by `OMP_PROC_BIND` and `OMP_PLACES`. Under a container with `--cpuset-cpus`, the most robust combination is:

```bash
docker run --rm --cpuset-cpus="0-3" \
    -e OMP_NUM_THREADS=4 \
    -e OMP_PROC_BIND=true \
    -e OMP_PLACES=cores \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp
```

- `--cpuset-cpus="0-3"`: the container may only touch cores 0, 1, 2, 3.
- `OMP_PROC_BIND=true`: OpenMP pins each thread to a specific core from the allowed set and does not migrate it.
- `OMP_PLACES=cores`: OpenMP considers "cores" as its unit of placement (rather than "threads" which would include SMT siblings).

> **HPC note.** On a NUMA system, pinning to cores on the same NUMA node is essential for performance. If your host has two sockets and you use `--cpuset-cpus="0-7"` but cores 0–3 are on node 0 and cores 4–7 on node 1, your application will cross sockets and pay the NUMA penalty. On Linux, run `lscpu -e` to see the NUMA-to-core mapping, and pick a set of cores from a single node unless you want to span sockets deliberately.

## 5.6 A scaling study

With the pieces in place, run a proper strong-scaling study. Strong scaling means: **fix the problem size**, increase the thread count, observe the speedup.

Run the following four commands and record the best time of each:

```bash
# 1 thread, 1 core
docker run --rm --cpuset-cpus="0" \
    -e OMP_NUM_THREADS=1 \
    -e OMP_PROC_BIND=true \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp

# 2 threads, 2 cores
docker run --rm --cpuset-cpus="0-1" \
    -e OMP_NUM_THREADS=2 \
    -e OMP_PROC_BIND=true \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp

# 4 threads, 4 cores
docker run --rm --cpuset-cpus="0-3" \
    -e OMP_NUM_THREADS=4 \
    -e OMP_PROC_BIND=true \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp

# 8 threads, 8 cores (skip if you have <8 physical cores)
docker run --rm --cpuset-cpus="0-7" \
    -e OMP_NUM_THREADS=8 \
    -e OMP_PROC_BIND=true \
    -v "$(pwd):/work" -w /work gcc:13 ./mc_pi_omp
```

On a modern 8-core workstation, a typical table of results looks like this:

| Threads | Best time (s) | Speedup | Efficiency |
|---|---|---|---|
| 1 | 6.42 | 1.00× | 100% |
| 2 | 3.24 | 1.98× | 99% |
| 4 | 1.65 | 3.89× | 97% |
| 8 | 0.89 | 7.21× | 90% |

Near-linear up to 8 threads, as expected for an embarrassingly parallel workload. The drop in efficiency at higher thread counts usually comes from OS noise (timer interrupts, background daemons) and, on some CPUs, reduced clock frequency under all-core workloads.

> **Try it.** Repeat the 8-thread run with `--cpus=8` instead of `--cpuset-cpus="0-7"`. You will likely see nearly-identical performance on a lightly loaded system, but more variance run-to-run. The difference between `--cpus` and `--cpuset-cpus` is invisible when nothing else is running; it becomes visible under load, because `--cpuset-cpus` guarantees specific cores while `--cpus` lets the scheduler move the container around.

## 5.7 Podman and CPU resources

Podman accepts `--cpus` and `--cpuset-cpus` with the same semantics as Docker. Two caveats:

- **Rootless Podman** may not be able to set CPU quotas on all systems, because cgroups v2 requires specific kernel configuration and user-level delegation to work rootlessly. On modern Fedora/RHEL and recent Ubuntu, it works out of the box. On older distros, `podman --cpus=2 ...` may silently ignore the limit in rootless mode.
- `podman run --cpuset-cpus="0-3"` works rootlessly only if the user's cgroup slice permits the requested cores. If it doesn't, you'll get an error like `cannot set cpuset`. Run as root (or `sudo podman`) in that case.

For benchmarking experiments where you really need reproducible CPU affinity, root-mode Podman or Docker is the safer choice.

## 5.8 Summary — the right pattern for HPC workloads

Whenever you run a CPU-bound workload in a container:

1. **Decide how many CPUs the workload should use** — call this `N`.
2. **Pick specific cores** — with `--cpuset-cpus` on Linux. On a NUMA system, pick cores from one node.
3. **Set all relevant thread-count env vars** — `OMP_NUM_THREADS=N`, plus `MKL_NUM_THREADS`, `OPENBLAS_NUM_THREADS` if the workload links those libraries.
4. **Pin OpenMP threads** — `OMP_PROC_BIND=true`, `OMP_PLACES=cores`.

This is four flags, but it is the difference between a reproducible benchmark and a misleading one.

---

# Chapter 6 — Memory limits and the OOM killer

Memory limits in Docker matter for two related but distinct reasons. The first is **defensive**: a runaway job should not take down the host or the cluster node; a bound says "if you exceed this, you die, and nothing else suffers." The second is **predictive**: for scheduling and cost-attribution on shared systems, you need to know how much memory your job will actually use.

You'll learn:

- How to set a memory limit with `--memory`.
- What happens when a container exceeds it (the OOM killer).
- How to watch memory use in real time with `docker stats`.
- The distinction between `--memory` and `--memory-swap`.
- Some HPC-specific memory considerations.

## 6.1 The workload

The file `./ch06_memory/memhog.py` allocates NumPy arrays of configurable size, step by step, reporting its resident set size (RSS) after each allocation. It is intentionally small and readable — the point is not the computation but the observable memory pressure.

Configuration:

- `TARGET_MB` — total MB to allocate (default 1024)
- `MB_PER_STEP` — size of each allocation step (default 128)
- `SLEEP_SEC` — pause between steps, so you can watch `docker stats` live (default 0.2)
- `HOLD_SEC` — how long to hold peak memory before exit (default 5)

The script uses `numpy.zeros(...).fill(1.0)` rather than plain `numpy.zeros(...)` because NumPy zero-arrays are lazy on Linux — pages are only backed by physical memory when first touched. We need the memory pressure to be real.

## 6.2 An unconstrained run (baseline)

First, run with a modest target without any limit:

```bash
docker run --rm \
    -e TARGET_MB=500 \
    -v "$(pwd)/ch06_memory:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q numpy && python3 memhog.py"
```

Expected output (abbreviated):

```text
memhog — target=500 MB, step=128 MB
  initial RSS = 48.3 MB

  + 128 MB  cumulative=  128 MB  RSS= 176.7 MB
  + 128 MB  cumulative=  256 MB  RSS= 304.9 MB
  + 128 MB  cumulative=  384 MB  RSS= 433.1 MB
  + 116 MB  cumulative=  500 MB  RSS= 549.0 MB

target reached, holding for 5.0 s ...
  final RSS = 549.0 MB
done.
```

RSS tracks the allocated amount plus Python/NumPy baseline (~50 MB) almost exactly, confirming that `fill()` forced real allocation.

## 6.3 Setting a memory limit

Restrict the container to 256 MB:

```bash
docker run --rm \
    --memory=256m \
    -e TARGET_MB=500 \
    -v "$(pwd)/ch06_memory:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q numpy && python3 memhog.py"
```

The unit on `--memory` can be `b`, `k`, `m`, `g` (bytes, kilobytes, megabytes, gigabytes). Here, 256m means 256 MiB.

Expected output:

```text
memhog — target=500 MB, step=128 MB
  initial RSS = 48.3 MB

  + 128 MB  cumulative=  128 MB  RSS= 176.7 MB
  + 128 MB  cumulative=  256 MB  RSS= 237.8 MB
```

…and then the container is abruptly killed. Your shell shows no further output; `docker run` exits with code `137`, which is the Unix convention for "killed by signal 9 (SIGKILL)." Verify:

```bash
echo $?
# 137
```

You have just witnessed the **Linux Out-of-Memory (OOM) killer** in action. When a cgroup exceeds its memory limit, the kernel's OOM killer picks a victim (almost always the offending process) and sends SIGKILL. The process has no chance to clean up. The container exits, and Docker reports 137.

> **Diagnosing after the fact.** If the container was not run with `--rm`, you can inspect what happened:
>
> ```bash
> docker inspect NAME | grep -A2 OOMKilled
> ```
>
> A `"OOMKilled": true` confirms the cause. In CI and batch settings, this is the standard diagnostic for "my container exited mysteriously."

## 6.4 Watching memory live

In a second terminal, before starting the run, launch:

```bash
docker stats
```

This streams a live view of every running container's CPU, memory, and I/O. Then, in the first terminal, start a long-held run with generous steps:

```bash
docker run --rm \
    --name memtest \
    --memory=512m \
    -e TARGET_MB=400 \
    -e MB_PER_STEP=50 \
    -e SLEEP_SEC=1 \
    -e HOLD_SEC=20 \
    -v "$(pwd)/ch06_memory:/work" \
    -w /work \
    python:3.11-slim \
    bash -c "pip install -q numpy && python3 memhog.py"
```

In the `docker stats` window you will see `memtest`'s memory column tick upward, roughly once per second, approaching 400 MiB against the 512 MiB budget. This is the most immediate way to understand how your container behaves under memory pressure.

## 6.5 `--memory` vs `--memory-swap`

By default, `--memory=256m` sets the total memory budget — RAM *plus* swap. If the host has swap available, Docker allows the container to swap out pages when it approaches the RAM limit.

For HPC, swap is almost always undesired: swapped-out pages produce unpredictable latency spikes that corrupt benchmarks and can be orders of magnitude slower than RAM. To prohibit swap entirely:

```bash
docker run --rm \
    --memory=256m \
    --memory-swap=256m \
    ...
```

`--memory-swap` is the total (RAM + swap). Setting it equal to `--memory` means zero swap. Under this configuration, if the container exceeds 256m, the OOM killer triggers immediately — there is no graceful degradation via swap, which is exactly what you want for a predictable HPC job.

The most common four configurations:

| `--memory` | `--memory-swap` | Behaviour |
|---|---|---|
| unset | unset | No limit; container can use all host RAM + swap |
| `256m` | unset | 256 MB RAM + unlimited swap (default Docker behaviour is fuzzy — avoid) |
| `256m` | `256m` | 256 MB RAM, zero swap. **Recommended for HPC.** |
| `256m` | `512m` | 256 MB RAM + 256 MB swap |

## 6.6 Soft limits — `--memory-reservation`

For longer-lived workloads on shared hosts, a hard limit can be complemented with a **soft** limit:

```bash
docker run --rm \
    --memory=1g \
    --memory-reservation=512m \
    ...
```

- `--memory=1g` — hard limit; container dies if it exceeds 1 GiB.
- `--memory-reservation=512m` — soft limit; when the *host* is under memory pressure, the kernel will preferentially reclaim from containers whose RSS exceeds their reservation.

Reservations are relevant for shared Docker hosts (laboratory servers, multi-user workstations). They are largely irrelevant on single-job batch systems where each job has a dedicated node.

## 6.7 Podman and memory

Podman's `--memory` and `--memory-swap` behave identically to Docker's. The rootless-mode caveat from Chapter 5 applies again: on older distros with cgroups v1, rootless Podman cannot set memory limits. Check with `podman info | grep cgroupVersion`; if it says `v2`, memory limits work rootlessly.

## 6.8 HPC-specific memory considerations

Three things worth knowing for scientific workloads:

**Huge pages.** Applications using `transparent huge pages` or explicit `hugetlbfs` backing need appropriate mounts and capabilities. A typical MPI/OpenMP scientific code benefits significantly from huge pages; getting them to work reliably in a Docker container requires care and often `--cap-add=SYS_RESOURCE`. We will not cover this — on Leonardo, Apptainer handles the interaction more smoothly (Chapter 10).

**Memory-mapped I/O.** When your workload mmaps large input files (common in bioinformatics and HDF5-based pipelines), the mmap'd pages count against the container's memory limit once they are resident. On tight limits, this can cause unexpected OOMs even when the heap is small. Be generous with `--memory` for mmap-heavy pipelines.

**NUMA-aware allocation.** Docker does not directly expose NUMA policies. If your code needs NUMA-local allocation (mbind, numa_alloc_local, OpenMP first-touch), you also need `--cpuset-mems` to pin the container's memory to specific NUMA nodes. The full incantation:

```bash
docker run --rm \
    --cpuset-cpus="0-7" \
    --cpuset-mems="0" \
    -e OMP_NUM_THREADS=8 -e OMP_PROC_BIND=true \
    --memory=16g --memory-swap=16g \
    ...
```

This says: run on cores 0–7 (which should be a single NUMA node), allocate memory on NUMA node 0, use 8 OpenMP threads, and cap at 16 GiB.

## 6.9 Summary

For an HPC batch workload, the canonical memory configuration is:

```bash
--memory=XG --memory-swap=XG
```

…with `X` set to *slightly above* the observed peak RSS of your workload (say +10%). Combined with the CPU flags of Chapter 5, this gives you a reproducible resource envelope.

---

# Chapter 7 — Volumes and persistent data

A container's file system is ephemeral. Anything written inside a container disappears when the container is removed — which, with `--rm`, happens automatically. For real scientific workflows this is unacceptable: input datasets need to come from somewhere, intermediate results need to be shareable between stages, final outputs need to be preserved.

Docker has three storage mechanisms:

1. **Bind mounts** — map a host directory into the container. Simple, host-coupled.
2. **Named volumes** — Docker-managed storage that survives container removal. Portable, but you need to cat files with `docker cp` to see them on the host.
3. **tmpfs mounts** — in-memory, ephemeral, fast. For scratch space that should never hit disk.

We already used bind mounts informally in every previous chapter (`-v "$(pwd)/chXX:/work"`). This chapter formalises the distinctions and introduces the patterns appropriate for scientific pipelines.

## 7.1 The scenario

The file `./ch07_volumes/weather_observations.csv` contains 720 hours (one month) of synthetic weather observations: temperature, pressure, humidity. The script `analyse.py` reads it, computes summary statistics, daily aggregates, and pairwise correlations, and writes three output files (`summary.json`, `daily_means.csv`, `correlations.txt`).

The script follows the standard scientific-pipeline convention: **input under `/data/in/`, output under `/data/out/`**. These paths are configurable via `IN_DIR` and `OUT_DIR` environment variables, but the defaults hard-code the `/data/in, /data/out` split because that split is what maps cleanly onto Docker's storage mechanisms.

## 7.2 Bind mounts for input and output

The simplest approach: two bind mounts, one for input and one for output.

```bash
mkdir -p /tmp/weather_out
docker run --rm \
    -v "$(pwd)/ch07_volumes:/data/in:ro" \
    -v "/tmp/weather_out:/data/out" \
    -v "$(pwd)/ch07_volumes:/work" -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 analyse.py"
```

Three bind mounts this time, each with a distinct role:

- `-v "$(pwd)/ch07_volumes:/data/in:ro"` — the input directory mounted read-only. The `:ro` suffix is essential for scientific correctness: your input data should not be modifiable by the container, ever. A bug (or a bad script, or a misbehaving library) cannot corrupt the input. This is a discipline; enforce it.
- `-v "/tmp/weather_out:/data/out"` — the output directory, read-write. Writes from the container appear here on the host.
- `-v "$(pwd)/ch07_volumes:/work" -w /work` — the working directory with the script itself. This pattern is familiar from previous chapters.

After the run:

```bash
ls /tmp/weather_out/
# correlations.txt  daily_means.csv  summary.json

cat /tmp/weather_out/summary.json
```

You should see a JSON with mean/std/min/max for each observation channel.

> **Try it.** Run the same command a second time without clearing `/tmp/weather_out/`. The files are overwritten (same names). Now run with `OUT_DIR=/data/out/run2` as an env var, after creating `/tmp/weather_out/run2/`. You have just implemented simple run-level isolation with no code changes.

## 7.3 The read-only input pattern

Reinforcing §7.2: why `:ro` matters.

A running container has root privileges inside its namespace by default. A malicious image, a buggy script, or even a well-meaning "normalise the data in place" optimisation can modify files on the host through a read-write bind mount. For input data — observations from an experiment, genomic sequences, a downloaded public dataset — this is exactly the wrong behaviour.

Mount input read-only. Always. If you need to write anywhere, mount a separate read-write directory for outputs.

This is also a canonical HPC pattern: on Leonardo and most supercomputers, the input directory (`$SCRATCH/my_inputs/`) is usually kept read-only in batch scripts by convention, and outputs go to a separate `$SCRATCH/my_outputs/`. The same discipline scales from laptops to Leonardo.

## 7.4 Named volumes — portable, Docker-managed

A named volume is a Docker-managed piece of storage that is *not* tied to a specific host directory. Useful when:

- The output should survive across many runs without the user having to manage paths.
- You want the data to be portable between container instances without knowing the host layout.
- You are on Docker Desktop where bind mounts cross a VM boundary (slow on macOS especially).

Create a named volume:

```bash
docker volume create weather-results
```

Run, writing to it instead of a host directory:

```bash
docker run --rm \
    -v "$(pwd)/ch07_volumes:/data/in:ro" \
    -v "weather-results:/data/out" \
    -v "$(pwd)/ch07_volumes:/work" -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 analyse.py"
```

The second `-v` now uses `weather-results` — a volume name, no slash in front — instead of a host path. Docker mounts the named volume at `/data/out`.

After the run, your output is *not* visible anywhere directly on your host file system (or rather: it lives under `/var/lib/docker/volumes/weather-results/_data/`, but Docker strongly prefers you not to touch that). Access it through Docker:

```bash
docker run --rm -v "weather-results:/data" alpine ls /data
# correlations.txt  daily_means.csv  summary.json

docker run --rm -v "weather-results:/data" alpine cat /data/summary.json
```

To copy the results out to the host:

```bash
# Start a dummy container holding the volume
docker run --rm -d --name tmpvol -v "weather-results:/data" alpine sleep 60
docker cp tmpvol:/data ./local_results
docker stop tmpvol
```

This is less convenient than bind mounts on a Linux host but more portable across systems.

List all your named volumes:

```bash
docker volume ls
```

Remove one (irreversibly):

```bash
docker volume rm weather-results
```

> **When to use which?** A rule of thumb:
>
> - Bind mounts when you are on Linux and the output matters to you on the host (most HPC cases).
> - Named volumes when portability across machines matters, or when bind-mount performance is poor (Docker Desktop).
> - On HPC clusters, this distinction is secondary — you almost always want bind mounts onto the filesystem your job controls ($SCRATCH, $WORK).

## 7.5 tmpfs — in-memory scratch space

For truly ephemeral scratch data — intermediate files that should never hit disk — use a `tmpfs` mount:

```bash
docker run --rm \
    --tmpfs /tmp/scratch:size=1g,mode=1777 \
    -v "$(pwd)/ch07_volumes:/data/in:ro" \
    -v "/tmp/weather_out:/data/out" \
    -v "$(pwd)/ch07_volumes:/work" -w /work \
    python:3.11-slim \
    bash -c "pip install -q -r requirements.txt && python3 analyse.py"
```

Inside the container, `/tmp/scratch` is backed by RAM with a cap of 1 GiB. Writes to it never touch disk, are as fast as memory, and vanish when the container exits.

HPC use cases for tmpfs:

- Intermediate MPI dumps that are aggregated at job end.
- Scratch buffers for libraries that insist on a file-backed path (sometimes HDF5, sometimes MKL with heavy I/O).
- Secure temporary files (nothing persists on disk).

The cost is that tmpfs counts against the container's memory allocation. Combining `--tmpfs /tmp/scratch:size=1g` with `--memory=4g` means 1 GiB of your 4 GiB budget can go to scratch files.

## 7.6 Ownership and permissions — a brief but critical note

When you bind-mount a host directory, files created inside the container appear on the host with the UID of the in-container user. Under Docker (daemon-based), that is usually root (UID 0) by default. So:

```bash
mkdir /tmp/myout
docker run --rm -v /tmp/myout:/data ubuntu bash -c "echo hi > /data/x.txt"
ls -la /tmp/myout/x.txt
# -rw-r--r-- 1 root root 3 ... x.txt   # owned by root on your host!
```

This is a common source of confusion: "I can't delete my own output files without sudo."

Two fixes:

**Fix 1: run as your UID.** Pass `--user $(id -u):$(id -g)`:

```bash
docker run --rm --user $(id -u):$(id -g) -v /tmp/myout:/data ubuntu bash -c "echo hi > /data/x.txt"
ls -la /tmp/myout/x.txt
# -rw-r--r-- 1 morgan morgan ...    # owned by your host user
```

**Fix 2: use Podman rootless.** Podman in rootless mode has a user-namespace mapping that makes container UID 0 appear as your host UID automatically. This is one of Podman's strongest selling points for scientific computing:

```bash
podman run --rm -v /tmp/myout:/data ubuntu bash -c "echo hi > /data/x.txt"
ls -la /tmp/myout/x.txt
# -rw-r--r-- 1 morgan morgan ...    # correct by default
```

On shared HPC infrastructure this UID-mapping property is central to why **Apptainer** (not Docker) is the tool of choice there — see Chapter 10.

## 7.7 Summary

| Mechanism | Syntax | Use case |
|---|---|---|
| Bind mount (rw) | `-v /host/path:/ctr/path` | Host-visible input/output (most HPC cases) |
| Bind mount (ro) | `-v /host/path:/ctr/path:ro` | Read-only input |
| Named volume | `-v myvol:/ctr/path` | Docker-managed persistence, portability |
| tmpfs | `--tmpfs /ctr/path:size=1g` | In-memory scratch |

The canonical scientific-pipeline pattern:

```bash
docker run --rm \
    -v /path/to/inputs:/data/in:ro \
    -v /path/to/outputs:/data/out \
    --tmpfs /tmp/scratch:size=1g \
    --user $(id -u):$(id -g) \
    ...
```

Input read-only, output read-write, scratch in RAM, files owned by you. Memorise this pattern; it appears unchanged in Apptainer workflows on Leonardo.

---

# Chapter 8 — Building your first image with Dockerfile

Everything we have done so far uses the pattern:

```bash
docker run --rm python:3.11-slim bash -c "pip install -r requirements.txt && python3 script.py"
```

This is fine for the first day of Docker. It is unacceptable for anything reproducible: the `pip install` runs every time (wasteful), the set of installed packages depends on what PyPI serves *today* (non-reproducible), and the logic of what makes this image what it is lives scattered in a long shell command rather than in a version-controlled file.

The proper solution is to **build your own image**. An image is defined by a small text file called a **Dockerfile**; `docker build` reads the Dockerfile and produces an image artefact you can then `docker run` as many times as you like, without re-installing anything.

You'll learn:

- The anatomy of a Dockerfile.
- The six most common instructions: `FROM`, `WORKDIR`, `COPY`, `RUN`, `USER`, `CMD`.
- Layer caching and why the order of instructions matters.
- How to tag and version your images.
- The difference between `CMD` and `ENTRYPOINT`.

## 8.1 The scenario

We will package the compute service from Chapter 4 (`server.py` + `requirements.txt`) into a self-contained image. The goal is that after `docker build`, a single `docker run -p 8000:8000 compute-service:1.0` starts the service in about a second — no pip install, no bind mount, no bash -c gymnastics.

The files for this chapter are under `./ch08_dockerfile_intro/`:

```text
ch08_dockerfile_intro/
├── Dockerfile
├── requirements.txt
└── server.py
```

`server.py` is unchanged from Chapter 4. `requirements.txt` is unchanged. The `Dockerfile` is new.

## 8.2 The Dockerfile, line by line

Open `./ch08_dockerfile_intro/Dockerfile`. We reproduce the essential content here, then dissect it instruction by instruction:

```dockerfile
FROM python:3.11-slim

LABEL maintainer="HPCC 2025/2026 course"
LABEL description="Stateless compute service for linear-system solves"
LABEL version="1.0"

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir --compile -r requirements.txt

COPY server.py .

RUN useradd --create-home --shell /bin/bash appuser
USER appuser

EXPOSE 8000

CMD ["uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8000"]
```

### `FROM python:3.11-slim`

Every Dockerfile starts with `FROM`, which specifies the **base image**. We build on top of the official Python 3.11 slim image — the same one we have been using for `docker run` throughout. If the base image is not already on your machine, Docker pulls it during the build.

The choice of base matters. `python:3.11` (no `-slim`) is ~900 MB and includes a full build environment. `python:3.11-slim` is ~45 MB and contains only what Python needs. For a server that doesn't compile anything at runtime, slim is the right choice. For an image that needs to build C extensions on the fly, a fuller base is necessary.

### `LABEL ...`

Labels are key=value metadata attached to the image. They don't affect how the image runs; they affect how it is catalogued and searched. `maintainer`, `version`, `description` are useful conventions.

### `WORKDIR /app`

Sets the working directory for subsequent instructions, and also the default working directory inside any container run from this image. Equivalent to `mkdir -p /app && cd /app`. Preferred over `RUN cd /app` because the latter has no lasting effect — each `RUN` starts fresh.

### `COPY requirements.txt .`

Copies `requirements.txt` from the build context (your local directory) to the container's current working directory (`/app` after the `WORKDIR` above). The first argument is relative to the build context; the second is a destination inside the image.

**Why `requirements.txt` alone, before the rest?** This is the key optimisation of the whole Dockerfile — see §8.3 below.

### `RUN pip install --no-cache-dir --compile -r requirements.txt`

Executes a command at **build time** (not at run time). The result — installed packages — is baked into the image. Subsequent containers launched from this image already have these packages; no pip install at run time.

- `--no-cache-dir` prevents pip from keeping its download cache inside the image (saves ~50 MB).
- `--compile` pre-compiles `.pyc` files, making first-startup slightly faster.

### `COPY server.py .`

Copies the application source. Done *after* the dependency install, so changes to `server.py` do not invalidate the dependency-install layer.

### `RUN useradd --create-home --shell /bin/bash appuser` and `USER appuser`

Creates a non-root user and switches the default user to it. This is defence-in-depth: a compromised service running as root inside a container can do more damage (via bind mounts, privileged escape paths) than one running as an ordinary user. Best practice for any serving container.

### `EXPOSE 8000`

Declarative: "this container intends to listen on port 8000." `EXPOSE` does *not* publish the port — that still requires `-p 8000:8000` at `docker run`. It is documentation for users of the image and hint for Docker's own tooling.

### `CMD ["uvicorn", ...]`

The default command to run when the container starts. Because we use the JSON array form (the **exec form**), uvicorn is invoked directly without a shell wrapper; signals (like SIGTERM from `docker stop`) are delivered cleanly to the process. Always prefer exec form for servers.

The alternative **shell form** — `CMD uvicorn server:app --host 0.0.0.0` — runs under `/bin/sh -c`, which complicates signal handling and should be avoided.

## 8.3 Layer caching — the reason `COPY requirements.txt` comes before `COPY server.py`

Every Dockerfile instruction produces an **image layer**. Layers are cached: if an instruction's inputs haven't changed, Docker reuses the cached layer from a previous build, skipping the instruction entirely. This is why a re-build of an image where nothing has changed takes one second.

The cache is invalidated as soon as Docker encounters a changed instruction. From that point down, every subsequent layer is rebuilt from scratch.

Compare two orderings.

**BAD ordering:**

```dockerfile
COPY . /app                        # copies everything
RUN pip install -r requirements.txt
```

Every time you edit `server.py`, the `COPY . /app` step changes, invalidating the cache. The expensive `pip install` then reruns from scratch — even though `requirements.txt` didn't change.

**GOOD ordering (what we have):**

```dockerfile
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY server.py .
```

Editing `server.py` only invalidates the final `COPY server.py` step. Dependencies are reused from cache.

The general rule: **order instructions from least-frequently-changing to most-frequently-changing**. Base images change rarely, dependencies change sometimes, source code changes constantly.

## 8.4 Building the image

From the `ch08_dockerfile_intro/` directory:

```bash
cd ch08_dockerfile_intro
docker build -t compute-service:1.0 .
```

- `-t compute-service:1.0` tags the image with a name and version. Always tag — untagged images are hard to reference later.
- The trailing `.` is the **build context**: the directory sent to the Docker daemon as input. All `COPY` instructions are relative to this.

Expected output (abbreviated):

```text
[+] Building 22.1s (11/11) FINISHED
 => [internal] load build definition from Dockerfile             0.0s
 => [internal] load metadata for docker.io/library/python:3.11-slim   1.5s
 => [1/6] FROM docker.io/library/python:3.11-slim                3.2s
 => [internal] load build context                                0.0s
 => [2/6] WORKDIR /app                                           0.1s
 => [3/6] COPY requirements.txt .                                0.0s
 => [4/6] RUN pip install --no-cache-dir --compile -r requirements.txt  15.3s
 => [5/6] COPY server.py .                                       0.0s
 => [6/6] RUN useradd --create-home ...                          0.4s
 => exporting to image                                           0.3s
 => => writing image sha256:...                                  0.0s
 => => naming to docker.io/library/compute-service:1.0           0.0s
```

Verify the image exists:

```bash
docker images | grep compute-service
```

You should see `compute-service` with the `1.0` tag and a size of ~170 MB (the 45 MB base plus FastAPI, NumPy, and their dependencies).

## 8.5 Running your image

```bash
docker run --rm -p 8000:8000 compute-service:1.0
```

No bind mount, no bash -c, no requirements install. Just the image name. Startup is instant (well, uvicorn takes a second to import NumPy and bind the port). Test with curl from another terminal:

```bash
curl http://localhost:8000/health
```

Should return `{"status":"ok","hostname":"..."}`.

## 8.6 Re-building after a change

Edit `server.py` — for instance, add a comment. Rebuild:

```bash
docker build -t compute-service:1.0 .
```

You should see the output take only ~1 second, with most lines marked `CACHED`:

```text
 => CACHED [2/6] WORKDIR /app
 => CACHED [3/6] COPY requirements.txt .
 => CACHED [4/6] RUN pip install ...
 => [5/6] COPY server.py .         0.1s
 => [6/6] RUN useradd ...          0.3s
```

The expensive step (step 4 — pip install) is cached. Only the layers downstream of the changed `server.py` are rebuilt. **This is the payoff of the ordering discipline.**

Now edit `requirements.txt` and rebuild. You'll see step 4 rerun, taking the full 15 seconds again, because its input changed. As expected.

## 8.7 `.dockerignore`

The build context (everything under `.`) is sent to the Docker daemon at the start of `docker build`. If your project directory contains 5 GB of test data, a Python virtualenv, or a `.git` directory, all of it is uploaded — slowing builds and bloating the context.

Create a `.dockerignore` file (same syntax as `.gitignore`) to exclude things:

```text
.git
.venv
__pycache__
*.pyc
.pytest_cache
build/
dist/
*.egg-info
node_modules/
```

This is a small file, but it is the single most common reason for mysterious multi-GB build contexts.

## 8.8 `CMD` vs `ENTRYPOINT`

Both specify what runs when the container starts. The difference:

- `CMD` defines the **default command**, which users can override by passing arguments to `docker run`.
- `ENTRYPOINT` defines a **fixed command**; user arguments to `docker run` are appended to it.

Our Dockerfile uses `CMD ["uvicorn", "server:app", ...]`. Running `docker run compute-service:1.0 /bin/bash` overrides the CMD entirely and you get a shell.

If instead we had `ENTRYPOINT ["uvicorn", "server:app"]` and `CMD ["--host", "0.0.0.0", "--port", "8000"]`, then:
- `docker run compute-service:1.0` would run `uvicorn server:app --host 0.0.0.0 --port 8000` (ENTRYPOINT + CMD).
- `docker run compute-service:1.0 --port 9000` would run `uvicorn server:app --port 9000` (ENTRYPOINT + overridden CMD).

The ENTRYPOINT + CMD pattern is appropriate when your image **wraps a specific command** and you want users to pass arguments to it, not replace it. For a generic server, plain `CMD` is simpler.

## 8.9 Tagging and versioning

```bash
docker tag compute-service:1.0 compute-service:latest
docker tag compute-service:1.0 myregistry.example.com/team/compute-service:1.0
```

`docker tag` adds a new tag to an existing image (no rebuild, no duplicate storage). Conventional tags:

- `:1.0`, `:1.1`, `:2.0` — semantic versions. The serious way to ship images.
- `:latest` — convenience alias. **Never use `:latest` in production**: every time you pull, you may get something different. Pin to a specific version.
- `:<git-commit-sha>` — for CI/CD pipelines, use the commit SHA as the tag. Every image has a unique, immutable name tied to exact source.

## 8.10 Summary

| Instruction | Purpose |
|---|---|
| `FROM` | Start from a base image |
| `WORKDIR` | Set working directory |
| `COPY` | Copy files from build context into the image |
| `RUN` | Execute a command at build time (baked into the image) |
| `USER` | Switch user for subsequent instructions and runtime |
| `EXPOSE` | Document the listening port (does not publish) |
| `CMD` | Default command at run time (can be overridden) |
| `ENTRYPOINT` | Fixed command at run time (arguments appended) |
| `LABEL` | Metadata |
| `ENV` | Environment variable (build time + runtime) |

The **two habits** to internalise from this chapter:

1. **Order instructions so the expensive, rarely-changing steps come first.** Dependency install before source copy.
2. **Always tag with a version**, never with just `:latest`.

---

# Chapter 9 — Multi-stage builds and image optimisation

A common Dockerfile pattern, up to this point, has been: *take a large base image that contains everything you need*, install your dependencies, add your code, ship the result. The image you ship contains the full build environment — compilers, development headers, package managers, caches.

For an application that only *runs* at production time, this is overkill. Shipping a C application with the entire GCC toolchain increases image size by ~1 GB, slows pulls, and enlarges the attack surface. The solution is the **multi-stage build**: use a heavy image to *build*, then copy only the resulting artefacts into a minimal *runtime* image.

You'll learn:

- How multi-stage builds separate build-time from runtime.
- How to `COPY --from=<stage>` between stages.
- How to dramatically reduce image size for compiled workloads.
- How to pick a sensible `-march=` target for HPC images intended to be portable.

## 9.1 The scenario

Take the OpenMP Monte Carlo program from Chapter 5 (`mc_pi_omp.c`). A naive single-stage Dockerfile would be:

```dockerfile
FROM gcc:13
COPY mc_pi_omp.c Makefile ./
RUN make
CMD ["./mc_pi_omp"]
```

The `gcc:13` base image is ~900 MB. The resulting image ships the full GCC toolchain to every production node — even though those nodes never compile anything.

## 9.2 The multi-stage Dockerfile

Open `./ch09_multistage/Dockerfile`. It has two stages.

**Stage 1 — the builder:**

```dockerfile
FROM gcc:13 AS builder

WORKDIR /build
COPY mc_pi_omp.c Makefile ./

RUN gcc -O3 -march=x86-64-v3 -fopenmp -Wall -Wextra \
        -o mc_pi_omp mc_pi_omp.c -fopenmp

RUN MC_SAMPLES=1000000 MC_REPEATS=1 ./mc_pi_omp
```

- `AS builder` names this stage so we can reference it later.
- We compile with `-march=x86-64-v3` — a modern x86-64 baseline (Haswell and above, AVX2 required). This ensures the binary runs on any reasonably recent x86 CPU, not just the exact one you built on. Discussed further in §9.4.
- The second `RUN` is a **self-test**: the build fails if the binary doesn't run. This catches miscompilations early.

**Stage 2 — the runtime:**

```dockerfile
FROM debian:12-slim AS runtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends libgomp1 && \
    rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --shell /bin/bash runner
USER runner
WORKDIR /home/runner

COPY --from=builder /build/mc_pi_omp /usr/local/bin/mc_pi_omp

CMD ["/usr/local/bin/mc_pi_omp"]
```

- `FROM debian:12-slim` — a minimal Debian, ~80 MB.
- `libgomp1` is the GNU OpenMP runtime library. Our binary links against it dynamically, so it must be present at runtime. Compilers, headers, and development tools are not needed at runtime — they stay in the builder stage.
- `COPY --from=builder /build/mc_pi_omp /usr/local/bin/mc_pi_omp` — **the key line**. This copies a single file out of the builder stage into the runtime stage. The builder's filesystem is otherwise discarded.

## 9.3 Building and comparing sizes

```bash
cd ch09_multistage
docker build -t mc-pi:1.0 .
```

Expected output: the builder stage compiles and self-tests; the runtime stage installs libgomp and copies the binary. Then:

```bash
docker images | grep -E "mc-pi|gcc"
```

You should see:

```text
mc-pi        1.0      ...      ~ 100 MB
gcc          13       ...      ~ 1.1 GB
```

The final `mc-pi:1.0` image is around 10× smaller than `gcc:13`. The 100 MB is mostly the Debian base (80 MB) plus libgomp and its dependencies (~20 MB) plus the 50 KB binary.

Run it:

```bash
docker run --rm mc-pi:1.0
```

Output identical to Chapter 5's `./mc_pi_omp`. Now you have a portable, slim, production-grade HPC container.

Combine with the resource flags from Chapter 5:

```bash
docker run --rm \
    --cpuset-cpus="0-7" \
    -e OMP_NUM_THREADS=8 \
    -e OMP_PROC_BIND=true \
    -e OMP_PLACES=cores \
    -e MC_SAMPLES=1000000000 \
    mc-pi:1.0
```

## 9.4 A note on `-march` and image portability

A recurring HPC question: what CPU architecture should I target when compiling for a container?

Three reasonable answers:

**`-march=native`**: the binary uses every instruction set extension the build machine supports. Fastest on that exact machine; fails with *Illegal instruction* if run on any older CPU. **Only use this if the build machine and the run machine are the same.**

**`-march=x86-64-v3`** (what we used): a GCC-defined "feature level" corresponding to Haswell-era CPUs (AVX2, BMI1/2, FMA, F16C — circa 2013 and later). Runs on essentially any modern x86_64 server, including Leonardo's Ice Lake Xeons and typical laptop CPUs. A good default for images you want to ship.

**`-march=x86-64`** (the default): the original 2003 AMD64 baseline. Runs everywhere, but misses AVX/AVX2 which matters for numerical workloads. **Don't use for HPC images unless true universal portability is required.**

The same logic applies to `-march=armv8-a` for ARM targets. On Leonardo (Intel Ice Lake), `-march=x86-64-v3` is a safe and reasonable choice; `-march=x86-64-v4` (requires AVX-512) works too but limits portability to servers with AVX-512.

## 9.5 Further optimisations

Several additional tricks are worth knowing briefly.

**Minimise layer count in apt-get.** Each `RUN` creates a layer. If you do `RUN apt-get update` in one RUN and `RUN apt-get install libfoo` in another, the update's layer is cached forever, potentially giving you stale package lists later. Always combine them:

```dockerfile
RUN apt-get update && \
    apt-get install -y --no-install-recommends libgomp1 && \
    rm -rf /var/lib/apt/lists/*
```

**Use `.dockerignore`** (already covered in Chapter 8). Critical for multi-stage because the build context is loaded once and used across stages.

**Consider `FROM scratch` for static binaries.** If you can compile a fully static binary (Go makes this easy; C is harder because of glibc), your runtime stage can be `FROM scratch` — an empty image. Final size: 2-5 MB. For C/OpenMP this is impractical because of libgomp's dynamic linking, but it's worth knowing.

**Pin base image digests for reproducibility.** `FROM python:3.11-slim` means "whatever python:3.11-slim points to today." For a truly reproducible build, pin the SHA:

```dockerfile
FROM python:3.11-slim@sha256:9bd...e3c7
```

The image at that digest will never change. This is overkill for teaching; standard practice in production CI.

## 9.6 When *not* to use multi-stage builds

Multi-stage is overkill when the runtime image already contains the tooling you would have needed to build. Two cases:

- **Pure Python workloads** (as in Chapter 8). Python is an interpreter; there's nothing to compile. A single stage from `python:3.11-slim` is correct.
- **Development images**. If the image is for developers iterating on code (tests, debugging, REPL), keep the toolchain present.

Use multi-stage for **compiled production artefacts** — C, C++, Fortran, Rust, Go — and for images where the build step pulls in large development-only dependencies (LaTeX for documentation generation, Node tools for frontend bundles, etc.).

## 9.7 Summary

The pattern:

```dockerfile
FROM <fat-build-image> AS builder
# ... compile artefact ...

FROM <slim-runtime-image>
COPY --from=builder /path/to/artefact /usr/local/bin/
CMD ["/usr/local/bin/artefact"]
```

The benefits:
- **Smaller images** (10×+ for compiled workloads).
- **Reduced attack surface** (no compilers in production).
- **Clearer intent** (the shipped image contains exactly its runtime needs).

---

# Chapter 10 — From Docker to Apptainer on Leonardo

This final chapter bridges the laptop/workstation world of Docker to the HPC cluster world of Apptainer. On Leonardo — CINECA's pre-exascale supercomputer in Bologna — and on essentially every other production HPC system, you will not run Docker directly. Understanding *why* sets up the Apptainer workflow and explains the design choices that make Apptainer the HPC container tool.

You'll learn:

- Why Docker is not used on HPC clusters.
- The key differences between Docker and Apptainer.
- How to convert a Docker image to an Apptainer `.sif` file.
- How to use Apptainer on Leonardo via SLURM.
- The filesystem and environment conventions you need to know.

## 10.1 Why HPC clusters do not run Docker

Docker's architectural model makes sense for a laptop or a cloud VM, but clashes with HPC in four ways.

**1. Root daemon.** The standard Docker installation runs a system-wide daemon (`dockerd`) as root. Any user in the `docker` group can instruct this daemon to launch containers, mount arbitrary host paths, and gain effective root access to the host. On a multi-tenant system like Leonardo — where hundreds of unprivileged users share each node — a tool that grants one user root-equivalent capabilities is unacceptable.

**2. Image storage model.** Docker stores images and container layers under `/var/lib/docker` on the host, managed by the daemon. On HPC clusters, each user's storage is carefully quota-managed on `$HOME`, `$WORK`, `$CINECA_SCRATCH`. A system-wide `/var/lib/docker` — shared across all users of a node — is incompatible with per-user accounting and cleanup.

**3. File ownership mismatch.** Containers default to running as UID 0 (root inside the container). When they write to a bind-mounted host directory, the resulting files appear on the host owned by whichever UID Docker's mapping produces — rarely the user's own UID. On Lustre and other shared filesystems, this causes pain: you cannot delete your own files without administrator help.

**4. Networking integration.** Docker's bridge networking and iptables manipulation conflict with the complex InfiniBand / RoCE fabric that HPC clusters run for MPI traffic. Making Docker's networking coexist with HPC interconnects is possible but requires careful configuration that is not in scope for a shared system.

The cluster-compatible container runtime that emerged from the HPC community to address all four problems is **Apptainer** (formerly Singularity).

## 10.2 Apptainer in brief

Apptainer differs from Docker on exactly the four points above:

**1. No daemon.** Apptainer is a set of command-line tools that run as ordinary user processes. No privileged background service. `apptainer exec image.sif command` is just a program the user runs.

**2. Single-file images.** An Apptainer image is a single file with extension `.sif` (Singularity Image Format). It lives in your home directory or scratch, like any other file. No system-wide image registry to manage, no per-node cache coherence issues, no cleanup problems.

**3. User-space execution.** The container runs as the invoking user's UID by default. Files it writes to bind-mounted host paths are owned by you. This is the critical property that makes Apptainer HPC-compatible.

**4. Transparent host integration.** By default, `$HOME`, `/tmp`, and a few other directories are bind-mounted into the container automatically. This lets an Apptainer job access its usual working directories without explicit configuration — matching the HPC culture of "your code runs where your files live."

For 99% of HPC workloads, Apptainer is equivalent to Docker from the user's perspective: you pull, build, run containers. The differences are in *how* it is installed and privileged, which is exactly what HPC sysadmins care about.

> **Note on names.** Apptainer is the open-source fork (now under the Linux Foundation) of what used to be called Singularity. The proprietary fork is SingularityCE (Sylabs). On Leonardo, the installed tool is called `singularity` (version 3.8.7) — historically, because it was installed before the rename. The command-line interface is compatible; you can type `singularity` or `apptainer` interchangeably on most systems, and everywhere in this chapter either name works. For clarity we write `apptainer` in examples.

## 10.3 Converting a Docker image to Apptainer

There are three paths, depending on where the image currently lives.

### Path A: The image is on Docker Hub (or another public registry)

The simplest case. You can go straight from the registry to SIF:

```bash
# On Leonardo, or on any machine with apptainer installed:
apptainer build mc_pi.sif docker://myuser/mc-pi:1.0
```

This pulls each Docker layer, unpacks it, and re-packages as SIF. One command, one output file.

### Path B: The image is in your local Docker daemon's cache

You just built `mc-pi:1.0` with `docker build` on your laptop, and apptainer is also installed locally. Convert directly:

```bash
apptainer build mc_pi.sif docker-daemon://mc-pi:1.0
```

The `docker-daemon://` URI tells Apptainer to read the image from the local Docker daemon's cache.

### Path C: The image is local but apptainer is not installed (the common laptop case)

On your laptop you have Docker but not Apptainer. You need to land the image on Leonardo somehow. The standard approach: export to a tar archive.

On your laptop:

```bash
docker save -o mc-pi.tar mc-pi:1.0
```

Transfer to Leonardo:

```bash
scp mc-pi.tar <user>@login.leonardo.cineca.it:$CINECA_SCRATCH/
```

On Leonardo:

```bash
cd $CINECA_SCRATCH
apptainer build mc_pi.sif docker-archive://mc-pi.tar
```

The `docker-archive://` URI reads the tar file. This is the workflow the provided helper script `./ch10_apptainer/convert_to_sif.sh` automates.

## 10.4 Running an Apptainer container

The Apptainer analogue of `docker run`:

```bash
apptainer run mc_pi.sif
```

This runs the container's default command (the `CMD` from the original Dockerfile — `/usr/local/bin/mc_pi_omp`).

To run a specific command inside the container (the analogue of `docker exec`):

```bash
apptainer exec mc_pi.sif /usr/local/bin/mc_pi_omp
```

To get an interactive shell:

```bash
apptainer shell mc_pi.sif
```

To pass environment variables into the container:

```bash
apptainer exec --env OMP_NUM_THREADS=8 --env MC_SAMPLES=1000000000 mc_pi.sif mc_pi_omp
```

By default, Apptainer bind-mounts `$HOME`, `/tmp`, and a few other directories. If you want more, add `--bind`:

```bash
apptainer exec --bind /scratch/myproject:/data mc_pi.sif ./mc_pi_omp
```

> **`--cleanenv` and why you want it.** By default, Apptainer inherits most of your host environment into the container. This is usually what you want on HPC (modules, PATH, library paths all carry in). But it can cause surprises: if you have `~/.local/lib/python3.11/` containing a version of NumPy different from the one in the container, the host's version may be imported instead of the container's — and your reproducibility is broken. The `--cleanenv` flag scrubs the host environment to a minimal set. For reproducible runs, add it.

## 10.5 Using Apptainer on Leonardo

Leonardo has Singularity/Apptainer preinstalled system-wide. **No `module load` is required**; the `singularity` (or `apptainer`) command is already in your `$PATH` on login nodes and compute nodes.

Check:

```bash
singularity --version
```

Output: `singularity-ce version 3.8.7` (or similar, depending on when you read this).

The canonical pattern for a batch job:

1. Put your `.sif` file in `$CINECA_SCRATCH` (plenty of quota; fast Lustre filesystem).
2. Write a SLURM submission script that `srun`s `singularity exec`.
3. Submit with `sbatch run.sbatch`.

The file `./ch10_apptainer/run_on_leonardo.sbatch` is a complete, working example. The essential parts:

```bash
#!/bin/bash
#SBATCH --job-name=mc-pi-omp
#SBATCH --time=00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=8G
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=<your_project_account>

cd ${CINECA_SCRATCH}

export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}
export OMP_PROC_BIND=true
export OMP_PLACES=cores

srun singularity exec \
    --cleanenv \
    --env OMP_NUM_THREADS=${OMP_NUM_THREADS} \
    --env OMP_PROC_BIND=${OMP_PROC_BIND} \
    --env OMP_PLACES=${OMP_PLACES} \
    mc_pi.sif /usr/local/bin/mc_pi_omp
```

Walk-through of the important lines:

- `--cpus-per-task=8` — SLURM allocates 8 physical cores to this task. On Leonardo DCGP nodes (Sapphire Rapids 8480+, 56 cores per socket), 8 cores is a single-socket allocation that avoids cross-NUMA issues.
- `--partition=dcgp_usr_prod` — the CPU partition. The GPU partition is `boost_usr_prod`. Replace with whichever suits your workload.
- `--account=<your_project_account>` — every CINECA job charges against a project account. You must supply this; your project PI provides it.
- `export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}` — match OpenMP threads to SLURM's allocation. This is the direct analogue of Chapter 5's `-e OMP_NUM_THREADS=N`.
- `srun singularity exec ... mc_pi.sif ...` — `srun` handles proper process placement per SLURM's allocation. For an OpenMP-only job on a single node, `srun` vs direct invocation produces the same result, but using `srun` is the HPC convention and is required for any multi-node work.
- `--env OMP_NUM_THREADS=...` — Apptainer respects Docker's `ENV` instructions from the original image, but to set variables at run time, you pass `--env`. (Alternative: set `APPTAINERENV_OMP_NUM_THREADS=8` before the `singularity exec` call — Apptainer strips the prefix and propagates.)

Submit:

```bash
sbatch run_on_leonardo.sbatch
```

Check status:

```bash
squeue -u $USER
```

Read output once the job completes:

```bash
cat mc_pi_<JOBID>.out
```

You should see the familiar Monte Carlo output, now running on a Leonardo compute node with 8 real cores.

## 10.6 Leonardo filesystems — a brief orientation

You need to know three filesystem locations when running on Leonardo:

| Variable | Size | Backup | Purpose |
|---|---|---|---|
| `$HOME` | ~50 GB per user | Yes | Permanent files (source, scripts, small configs) |
| `$WORK` | project-dependent | Yes | Shared project space, typically large |
| `$CINECA_SCRATCH` | large (TB) | **No** | Large temporary data (runs, outputs); auto-cleaned after 40 days |

**Put your `.sif` file in `$CINECA_SCRATCH`**, not `$HOME`. A 1 GB container image will quickly fill your home quota and break login. `$CINECA_SCRATCH` has room and performance (Lustre, parallel-optimised) suitable for HPC workloads.

Run the `cindata` command to see your current quotas:

```bash
cindata
```

## 10.7 What about MPI?

Pure OpenMP containers (as in this chapter) work out of the box on Leonardo with the pattern above. **MPI inside a container** is a more subtle story and deserves brief framing.

MPI applications require tight coupling with the host's MPI implementation and the network fabric (InfiniBand on Leonardo). Two strategies exist:

- **Bind mode**: use the host's MPI via bind-mounts. The container has a thin MPI shim that dynamically loads the host's MPI library; at runtime, the binary inside the container talks to InfiniBand via the host libraries. Compatible but requires careful matching of MPI versions.

- **Hybrid mode**: the container bundles its own MPI, which must match the host's ABI. Less dependent on host configuration but fragile across clusters.

On Leonardo, the documented approach is bind mode, using the system's Intel MPI or OpenMPI modules. The details are beyond this tutorial — ask your cluster support team or consult the CINECA documentation at [docs.hpc.cineca.it](https://docs.hpc.cineca.it/). For this course, treating containers as *OpenMP-parallel single-node* workloads is the correct scope.

## 10.8 Summary — the full pipeline

The end-to-end HPC workflow you have now assembled:

1. **Develop locally** with Docker. Iterate fast with bind-mounted source (Chapter 1).
2. **Containerise** with a Dockerfile (Chapter 8) or multi-stage Dockerfile for compiled code (Chapter 9).
3. **Test locally** with appropriate resource flags: `--cpuset-cpus`, `--memory`, `OMP_NUM_THREADS` (Chapters 5, 6).
4. **Convert to SIF** with `apptainer build mc_pi.sif docker-archive://mc-pi.tar` (§10.3).
5. **Transfer** to Leonardo via `scp` into `$CINECA_SCRATCH` (§10.6).
6. **Submit** a SLURM job that `srun`s `singularity exec` with appropriate resource requests (§10.5).

Every container you build in Docker can follow this path — as long as you avoid runtime dependencies on privileged operations, Docker-specific networking, or root-owned in-container state. The discipline established in the earlier chapters — non-root users, env-based config, minimal runtime images, no `:latest` tags — pays off exactly here.

## 10.9 Further reading

- [Apptainer User Guide](https://apptainer.org/docs/user/main/) — the official documentation.
- [CINECA Singularity documentation](https://docs.hpc.cineca.it/services/singularity.html) — Leonardo-specific notes.
- [Leonardo User Guide](https://leonardo-supercomputer.cineca.eu/) — system overview and access policies.
- `module help singularity` on Leonardo — local documentation shortcut.

---

*End of the tutorial. You now have a reproducible path from "I want to try Docker for a scientific workload" to "my job is running on Leonardo inside a container that I built from source." The ten chapters together give you a working mental model of containerisation for HPC — enough to understand the tools you'll encounter, enough to make good choices when the defaults don't fit, and enough to keep learning on your own from here.*

