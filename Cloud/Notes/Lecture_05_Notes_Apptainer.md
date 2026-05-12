# Apptainer and Singularity for HPC: Containers on Leonardo @ CINECA

**Course:** High Performance and Cloud Computing 2025/2026
**Programme:** Applied Data Science & Artificial Intelligence - HPC Track
**Institution:** University of Trieste
**Author:** Course instructor
**Duration:** 1 h 45 min (Part 1 ≈ 50 min, Part 2 ≈ 55 min)
---

## Learning Objectives (Bloom's Taxonomy)

By the end of this module, students will be able to:

- **Explain** the design rationale that distinguishes Apptainer/Singularity from Docker in a shared HPC environment
- **Build** custom SIF container images from definition files and from existing Docker images
- **Configure** OpenMP thread affinity and NUMA-aware binding inside containers on multi-socket nodes
- **Deploy** hybrid MPI+OpenMP parallel applications across multiple nodes using the host MPI and container execution
- **Run** GPU-accelerated workloads inside containers on NVIDIA A100 hardware using the `--nv` passthrough mechanism
- **Evaluate** trade-offs: when containers add value vs. when native module stacks suffice

---

## Table of Contents

1. [Lecture 1 — From Docker to Apptainer/Singularity](#lecture-1)
2. [Exercise 1 — First Contact with Apptainer on Leonardo](#exercise-1)
3. [Lecture 2 — Building Your First Container Image](#lecture-2)
4. [Exercise 2 — Building and Importing Images](#exercise-2)
5. [Lecture 3 — OpenMP in Containers: Affinity, Binding, and NUMA](#lecture-3)
6. [Exercise 3 — OpenMP Scaling and Binding on DCGP Nodes](#exercise-3)
7. [Lecture 4 — MPI and GPU in Containers](#lecture-4)
8. [Exercise 4 — Hybrid MPI+OpenMP and GPU Passthrough on the Booster](#exercise-4)
9. [Key Takeaways](#key-takeaways)
10. [References and Further Reading](#references)
11. [Glossary](#glossary)

---

## Lecture 1 — From Docker to Apptainer/Singularity {#lecture-1}

### 1.1 Why Containers on HPC?

Traditional HPC software deployment relies on the *environment module* system (`module load`, Lmod, TCL modules). This works well for centrally-managed, site-compiled software, but creates friction in several scenarios:

- A researcher wants to use a Python environment with specific, incompatible library versions
- A group needs to reproduce the exact software stack used six months ago for a publication
- A workflow originally designed on a laptop needs to run on the cluster unchanged
- A tool from Docker Hub must run on a shared system where Docker itself is not available

**Containers** solve all four problems simultaneously by packaging the entire user-space software environment — libraries, binaries, configuration files, compilers — into a single, portable, immutable artefact that runs on any compatible Linux kernel.

> **Definition**: A *container image* is a self-contained, layered filesystem snapshot that includes a complete user-space software environment. The container *runtime* mediates access between the image's user space and the host kernel, without requiring a separate virtualised kernel.

The critical point for HPC is: containers do **not** virtualise the kernel. The container uses the *host kernel* directly. This means:

- Near-zero startup overhead compared to virtual machines
- Full access to host hardware: InfiniBand HCAs, NVMe SSDs, NVIDIA GPUs
- MPI communication still goes through the host's high-speed network stack

### 1.2 Docker: Why It Is Not Suitable for Shared HPC Systems

Docker is the dominant container platform in industry. Understanding why it cannot be used on multi-user HPC clusters is not just a policy detail — it explains the architectural decisions behind Apptainer.

| Property | Docker | Impact on HPC |
|----------|--------|---------------|
| Daemon | Persistent root daemon (`dockerd`) | Security risk on shared systems; unavailable on most clusters |
| Privilege model | Container can escalate to root on host | A user could gain root on the login node |
| Network model | Isolated bridge network by default | Breaks InfiniBand and MPI collective operations |
| Filesystem | Overlayfs via daemon | Incompatible with Lustre and GPFS parallel filesystems |
| Image format | Layered tarball (OCI) | Thousands of small files — poor performance on Lustre |
| User identity | Containers run as root inside by default | Violates HPC site security policies |

The fundamental problem: **Docker's privilege model assumes a trusted administrator configuring containers for untrusted users. HPC clusters must support untrusted users running their own containers.**

### 1.3 Apptainer and Singularity: History and Ecosystem

The container ecosystem for HPC has a complex history. Understanding it is important because you will encounter all three names in production systems:

```
2015  Singularity created at Lawrence Berkeley National Laboratory (Gregory Kurtzer)
      Design goal: containers for HPC. No root daemon. No privilege escalation.

2018  Sylabs Inc. founded to provide commercial Singularity support.

2020  Gregory Kurtzer leaves Sylabs but retains leadership of the open-source project.

2021  May:      Sylabs forks the project → SingularityCE (community edition, Sylabs-managed)
      Nov 30:   The original project moves to the Linux Foundation → renamed Apptainer
```

**Today (2025) three products coexist**, all derived from the 2015 original:

| Product | Maintainer | License | Command | Notes |
|---------|-----------|---------|---------|-------|
| Singularity | Sylabs (commercial) | Proprietary | `singularity` | Enterprise support, cloud.sylabs.io |
| SingularityCE | Sylabs (community) | BSD-3 | `singularity` | Uses setuid binary by default |
| Apptainer | Linux Foundation | Apache 2.0 | `apptainer` | Uses user namespaces by default; alias `singularity` provided |

> **CINECA policy (2025)**: On Leonardo and Galileo100, the installed runtime is referred to as **Singularity** in documentation. The `singularity` command is available on login nodes and compute nodes. On newer systems (e.g., Pitagora) Apptainer 1.4+ is available. Because Apptainer provides a `singularity` alias and accepts the same CLI syntax and environment variables (with `SINGULARITY_` prefixes), all examples in this lecture work identically on both.

**Key compatibility guarantees**: SIF (Singularity Image Format) files are fully interchangeable between all three products. An image built on your laptop with Apptainer runs on Leonardo's Singularity without modification.

### 1.4 Apptainer's Security Model

Apptainer's core design principle: **your container process runs as you, not as root**.

```
+───────────────────────────────────────────────+
│  Host                                         │
│  User: alice (uid=1001)                       │
│                                               │
│  singularity exec my_container.sif /bin/bash  │
│         │                                     │
│         ▼                                     │
│  +─────────────────────────────────────+      │
│  │  Container user space               │      │
│  │  User: alice (uid=1001)  ← SAME     │      │
│  │  Cannot sudo, cannot escalate       │      │
│  │  /home/alice auto-mounted           │      │
│  +─────────────────────────────────────+      │
│                                               │
│  Host kernel (shared)                         │
+───────────────────────────────────────────────+
```

Consequences that matter for HPC students:

1. **Files created inside the container** are owned by you — no permission conflicts
2. **Privilege escalation inside the container is blocked** — `sudo` inside the container fails
3. **Your `$HOME`, `$PWD`, and `/tmp` are bind-mounted automatically** — no data copy needed
4. **The cluster's parallel filesystem (Lustre on Leonardo)** is accessible from inside the container at the same mount points as on the host

### 1.5 Apptainer vs Docker: Feature Comparison

| Feature | Apptainer/Singularity | Docker |
|---------|----------------------|--------|
| Root daemon required | No | Yes |
| User identity inside container | Same as host user | root (by default) |
| Image format | Single SIF file | Layered OCI (directory tree) |
| Parallel filesystem (Lustre/GPFS) | Compatible | Incompatible |
| MPI / InfiniBand | Full access (host stack) | Requires --network=host |
| GPU (NVIDIA) | `--nv` or `--nvccli` flag | `--gpus` + nvidia-container-toolkit daemon |
| Privilege escalation | Blocked | Possible |
| Image build requires root | Yes (build only) | Yes (daemon) |
| Pull from Docker Hub | Yes (`docker://` prefix) | Yes |
| Pull from NVIDIA NGC | Yes | Yes |
| Suitable for login nodes | Yes | No |

### 1.6 The SIF Image Format

Apptainer's image format is the **Singularity Image Format (SIF)**, a single compressed file containing:

- A SquashFS filesystem with the container's complete user space
- Optional cryptographic signature (GPG)
- Optional encryption (AES-256)
- Embedded metadata (definition file, labels)

The key property for HPC: **one SIF file = one large file**. Lustre and GPFS are optimised for large-file I/O. Docker's layered OCI format, by contrast, expands to tens of thousands of small files in `~/.docker/`, causing severe metadata load on parallel filesystems.

```
# On Leonardo: store SIF files here (not in $HOME)
$SCRATCH/<username>/containers/my_simulation.sif   # Good: large file on Lustre
~/.singularity/cache/                              # Cache: also on $SCRATCH recommended
```

### 1.7 Key Apptainer Commands

| Command | Purpose |
|---------|---------|
| `singularity pull docker://ubuntu:22.04` | Download + convert Docker image to SIF |
| `singularity shell my.sif` | Interactive shell inside container |
| `singularity exec my.sif CMD` | Execute single command inside container |
| `singularity run my.sif` | Execute the `%runscript` defined in the image |
| `singularity build my.sif my.def` | Build SIF from definition file (requires root) |
| `singularity build --sandbox my_dir/ my.sif` | Convert SIF to writable sandbox directory |
| `singularity inspect my.sif` | Show metadata, labels, definition file |
| `singularity cache list` | List cached layers |
| `singularity cache clean` | Clean cache (important on HPC: saves quota) |

### 1.8 Automatic Bind Mounts on Leonardo

By default, Apptainer automatically bind-mounts:

```
$HOME        → /home/<username>    (your home directory, read-write)
$PWD         → <same path>         (current working directory)
/tmp         → /tmp                (node-local temporary storage)
/proc, /sys, /dev                  (kernel pseudo-filesystems)
```

On Leonardo, CINECA's configuration additionally mounts:
```
/leonardo/home/  → /leonardo/home/   (home filesystem)
/leonardo/scratch/  → /leonardo/scratch/  (Lustre scratch)
/leonardo/work/  → /leonardo/work/    (Lustre work area)
```

You can add extra mounts with `--bind src:dest` or by setting `SINGULARITY_BIND`.

---

## Exercise 1 — First Contact with Apptainer on Leonardo {#exercise-1}


**Scientific context**: You are a computational biologist. You need to run a BLAST nucleotide search on Leonardo using a specific NCBI BLAST version that is not available as a CINECA module. This exercise explores the Apptainer runtime before you build your own images.

**Prerequisites**: Active CINECA account, access to Leonardo, basic SLURM knowledge.

---

**Setup**: Log in to Leonardo and set the cache to scratch to avoid quota issues.

```bash
ssh <username>@login.leonardo.cineca.it

# Redirect Apptainer/Singularity cache to scratch (IMPORTANT on Leonardo)
export SINGULARITY_CACHEDIR=$SCRATCH/.singularity_cache
mkdir -p $SINGULARITY_CACHEDIR

# Verify the singularity command is available
singularity --version
# Expected: singularity version 3.x.x  (or apptainer version 1.x.x)
```

---

### Part 1 — Pull and Inspect an Image (8 pts)

```bash
# Request an interactive allocation on the DCGP partition (CPU-only, fast for exploration)
salloc --nodes=1 --ntasks-per-node=1 --cpus-per-task=4 --mem=8G \
       --time=01:00:00 --partition=dcgp_usr_prod --account=<your_account>

# Pull the official NCBI BLAST image from Docker Hub
# docker:// tells Apptainer to convert from OCI format to SIF
singularity pull $SCRATCH/.singularity_cache/blast_latest.sif docker://ncbi/blast:latest

# Inspect the image metadata
singularity inspect blast_latest.sif
singularity inspect --deffile blast_latest.sif  # Show original definition if available
singularity inspect --labels blast_latest.sif   # Show labels

# Check the image size (one large file, not thousands of small ones)
ls -lh blast_latest.sif
```

**Questions**:
1. What Linux distribution is the base image built on? (Hint: `singularity exec blast_latest.sif cat /etc/os-release`)
2. What version of BLAST is installed? (`singularity exec blast_latest.sif blastn -version`)
3. How large is the SIF file? How does this compare to the Docker image size on Docker Hub?
4. List the automatically mounted directories you can see from inside the container (`singularity exec blast_latest.sif df -h | head -20`)

---

### Part 2 — Running Commands and Accessing Host Filesystems 

```bash
# Verify your $HOME and $SCRATCH are accessible from inside the container
singularity exec $SINGULARITY_CACHEDIR/blast_latest.sif ls $HOME
singularity exec $SINGULARITY_CACHEDIR/blast_latest.sif ls $SCRATCH
singularity exec $SINGULARITY_CACHEDIR/blast_latest.sif touch  $SCRATCH/pippo
# Create a small test FASTA file on the host
cat > $SCRATCH/test_query.fasta << 'EOF'
>test_seq_1
ATGCGTACGTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCT
AGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAGCTAG
EOF

# Run BLAST's makeblastdb on a file that lives on Lustre
# The container sees the same path as the host
singularity exec $SINGULARITY_CACHEDIR/blast_latest.sif makeblastdb \
    -in $SCRATCH/test_query.fasta \
    -dbtype nucl \
    -out $SCRATCH/test_blastdb

# Verify the database was created on the host filesystem
ls -lh $SCRATCH/test_blastdb.*

# Run a self-BLAST (query against itself) to verify execution
singularity exec $SINGULARITY_CACHEDIR/blast_latest.sif blastn \
    -query $SCRATCH/test_query.fasta \
    -db    $SCRATCH/test_blastdb \
    -out   $SCRATCH/blast_results.txt \
    -outfmt 6

cat $SCRATCH/blast_results.txt
```

**Questions**:
1. Can you write to `$SCRATCH` from inside the container? Why or why not?
2. Try `singularity Rexec $SINGULAITY_CACHEDIR/blast_latest.sif sudo ls /`. What happens and why?
3. Compare `singularity exec $SINGULARITY_CACHEDIR/blast_latest.sif id` with `id` on the host. What do you observe?
4. What is the container's view of `/etc/hostname`? What about the host's?
5. Use `singularity shell $SINGULAITY_CACHEDIR/blast_latest.sif` to open an interactive shell. Can you modify files under `/usr/` inside the container? Explain.

---

### Part 3 — SLURM Batch Job 

Write a SLURM batch script that runs BLAST inside the container non-interactively:

```bash
cat > $SCRATCH/run_blast_container.slurm << 'SLURM'
#!/bin/bash
#SBATCH --job-name=blast_container
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=16G
#SBATCH --time=00:15:00
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=<your_account>
#SBATCH --output=%x.%j.out
#SBATCH --error=%x.%j.err

# Redirect cache to scratch
export SINGULARITY_CACHEDIR=$SCRATCH/.singularity_cache

# Use the pre-pulled SIF (do not pull inside a job — network may be restricted)
SIF=$SCRATCH/.singularity_cache/blast_latest.sif

# Use multiple threads via BLAST's -num_threads option
singularity exec $SIF blastn \
    -query $SCRATCH/test_query.fasta \
    -db    $SCRATCH/test_blastdb \
    -out   $SCRATCH/blast_batch_results.txt \
    -num_threads ${SLURM_CPUS_PER_TASK} \
    -outfmt 6

echo "BLAST job completed on node: $(hostname)"
echo "Threads used: ${SLURM_CPUS_PER_TASK}"
SLURM

sbatch $SCRATCH/run_blast_container.slurm
```

**Questions**:
1. Explain why SIF files should be stored on `$SCRATCH` rather than `$HOME` on Leonardo.
2. Why should `singularity pull` never be called inside a SLURM job on a compute node?
3. The container's BLAST binary uses `${SLURM_CPUS_PER_TASK}` threads, which is set by SLURM on the host. How does the container inherit this environment variable?

---

## Lecture 2 — Building Your First Container Image {#lecture-2}

### 2.1 Two Paths to a Container Image

There are two primary strategies for obtaining an Apptainer-compatible SIF image:

```
Strategy A: Pull from a registry
  Docker Hub  ──► apptainer pull docker://image:tag ──► image.sif
  NVIDIA NGC  ──► apptainer pull docker://nvcr.io/nvidia/pytorch:24.01-py3 ──► pytorch.sif
  Sylabs Cloud ──► apptainer pull library://user/collection/image:tag ──► image.sif

Strategy B: Build from a definition file
  Write my.def ──► sudo apptainer build my.sif my.def ──► my.sif ──► scp to Leonardo
```

Strategy A is fast and leverages the enormous Docker Hub ecosystem. Strategy B gives complete control and is the basis for reproducible, version-controlled research environments.

### 2.2 The Apptainer Definition File

A definition file (`.def`) is the declarative specification of a container image. It has two parts: a **header** (which base image to start from) and **sections** (what to do to it).

```singularity
# ── HEADER ─────────────────────────────────────────────────────
Bootstrap: docker          # Source: Docker Hub (most common)
From: ubuntu:22.04         # Base image: Ubuntu 22.04 LTS

# ── SECTIONS ───────────────────────────────────────────────────

%arguments
    # Variables usable in the build with {{ VAR }} syntax
    MPI_VERSION=4.1.6

%setup
    # Runs on the HOST before the container is created
    # $APPTAINER_ROOTFS points to the container's root filesystem
    echo "Build started at $(date)" > /tmp/build_log.txt

%files
    # Copy files from HOST into the container
    # src (host path)    dest (container path)
    /tmp/build_log.txt   /opt/build_log.txt

%post
    # THE MOST IMPORTANT SECTION
    # Runs INSIDE the container as root during build
    # Install packages, compile software, configure environment
    apt-get update -y
    apt-get install -y --no-install-recommends \
        build-essential \
        gfortran \
        python3 \
        python3-pip \
        libopenblas-dev \
        wget \
        ca-certificates
    apt-get clean
    rm -rf /var/lib/apt/lists/*

    # Install Python packages
    pip3 install --no-cache-dir numpy scipy matplotlib

%environment
    # RUNS AT CONTAINER STARTUP (not during build)
    # Set environment variables that are present every time the container runs
    export LC_ALL=C
    export LANG=C
    export PATH=/opt/myapp/bin:$PATH
    export LD_LIBRARY_PATH=/opt/myapp/lib:$LD_LIBRARY_PATH

%runscript
    # Executed by 'apptainer run my.sif [ARGS...]'
    # Typically: launch the main application
    echo "Container started with arguments: $@"
    exec python3 "$@"

%test
    # Runs after build to verify the image is correct
    python3 -c "import numpy; print('NumPy version:', numpy.__version__)"
    python3 -c "import scipy; print('SciPy version:', scipy.__version__)"

%labels
    Author      Your Name <your.email@institution.it>
    Version     1.0.0
    Description Scientific Python environment for MHPC course

%help
    This container provides a scientific Python environment with NumPy and SciPy.
    Usage: apptainer run my_python.sif my_script.py
```

**Critical `%post` practices**:
- Always run `apt-get clean && rm -rf /var/lib/apt/lists/*` after installs to reduce image size
- Chain `apt-get update` and `apt-get install` in a single `RUN`-equivalent block
- Use `--no-install-recommends` to minimise unnecessary dependencies

### 2.3 Build Workflow on Your Laptop

Building requires root (for `sudo singularity build`). This is done on your personal machine or a university VM, **not on Leonardo**.

#### Build a simple image

```bash
# On your laptop (Linux with Apptainer installed, or WSL2 on Windows)

# 1. Edit the definition file
cd 00_simple_image 
vim my_image.def    # write the definition above
```

Here is the file
```singularity
Bootstrap: docker
From: ubuntu:22.04

%labels
    Author Your Name
    Version 1.0
    Description Scientific Python environment with MPI

%help
    This container provides Python 3.x with NumPy, SciPy,
    mpi4py, and common scientific libraries.

%environment
    export LC_ALL=C
    export PATH=/opt/conda/bin:$PATH

%post
    # System packages
    apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        wget \
        ca-certificates \
        libopenmpi-dev \
        openmpi-bin \
	libibverbs-dev \
        python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

    # Python scientific stack
    pip install numpy scipy mpi4py matplotlib h5py

%runscript
    echo "Scientific Python container v1.0"
    exec python3 "$@"

%test
    python3 -c "import numpy; import scipy; import mpi4py; print('All imports OK')"
```

```bash
# 2. Build the SIF (requires sudo)
sudo apptainer build my_python.sif my_image.def

# 3. Inspect and test locally
apptainer exec my_python.sif python3 -c "import numpy; print(numpy.__version__)"

# 4. Transfer to Leonardo's scratch
scp my_python.sif <username>@login.leonardo.cineca.it:$SCRATCH/containers/

# 5. Run on Leonardo
ssh <username>@login.leonardo.cineca.it
singularity exec $SCRATCH/containers/my_python.sif python3 dot_product.py
```

### 2.4 Converting Docker Images from Your Laptop

If you have a custom Docker image that cannot be pushed to a public registry:

```bash
# ── On your laptop ────────────────────────────────────────────
# Step 1: Identify the Docker image
docker image ls
# REPOSITORY   TAG    IMAGE ID       CREATED      SIZE
# my_sim       v2.1   a3f9b2cd1234   2 days ago   3.2GB

# Step 2: Export Docker image as a tar archive
docker image save my_sim:v2.1 -o my_sim_v2.1.tar

# Step 3: Convert tar to SIF using Apptainer
sudo apptainer build my_sim_v2.1.sif docker-archive://my_sim_v2.1.tar

# Step 4: Verify
apptainer exec my_sim_v2.1.sif my_simulation --version

# Step 5: Transfer to Leonardo
scp my_sim_v2.1.sif <username>@login.leonardo.cineca.it:$SCRATCH/containers/

# ── On Leonardo ───────────────────────────────────────────────
singularity exec $SCRATCH/containers/my_sim_v2.1.sif my_simulation --input data.h5
```

Pay attention to the architecture: ARMvsAMD

```bash
docker pull --platform linux/amd64 gcc:16.1

docker image save --platform Linux/amd64 gcc:16.1 -o my_gcc.tar
```


### 2.5 Using Images from NVIDIA NGC

NVIDIA NGC (nvcr.io) provides GPU-optimised container images for deep learning frameworks, HPC libraries, and scientific applications. These are production-grade images maintained by NVIDIA and are the recommended starting point for GPU workloads on Leonardo.

```bash
# Pull PyTorch with CUDA 12.x support (verified to work on A100)
singularity pull $SCRATCH/containers/pytorch_24.01.sif \
    docker://nvcr.io/nvidia/pytorch:24.01-py3

# Pull GROMACS GPU-accelerated version
singularity pull $SCRATCH/containers/gromacs_2024.1.sif \
    docker://nvcr.io/hpc/gromacs:2024.1

# Pull LAMMPS
singularity pull $SCRATCH/containers/lammps_2023.sif \
    docker://nvcr.io/hpc/lammps:patch_2Aug2023
```

### 2.6 Sandbox Mode: Interactive Development

When you need to iterate on an image interactively (install, test, fix, repeat), use sandbox mode:

```bash
# Convert existing SIF to writable sandbox directory
sudo apptainer build --sandbox my_sandbox/ my_python.sif

# Open interactive shell with write access
sudo apptainer shell --writable my_sandbox/

# Inside: install additional packages
Apptainer> apt-get install -y htop vim
Apptainer> pip3 install h5py
Apptainer> exit

# Once satisfied, convert sandbox back to immutable SIF
sudo apptainer build my_python_v2.sif my_sandbox/
```

> **Warning**: Sandbox images on Lustre (Leonardo's scratch) generate thousands of small files. Keep sandboxes **local on your laptop**, not on the cluster's shared filesystem.

### 2.7 Containers and Enviroment variables.
Singularity/Apptainer containers inherit some environmental variables.

The file `./01_build_image_from_tar/mc_pi_omp.c` is an OpenMP-parallel Monte Carlo estimation of π. It is the parallel analogue of the Python script used in Chapters 2–3. Each thread draws a disjoint chunk of random points in the unit square and counts those inside the quarter unit circle; a final reduction gives the estimate `4 * hits / N`.

The computation is **embarrassingly parallel** — threads share no state during the main loop — and **compute-bound** — each sample is a handful of floating-point operations on values that live in registers. These two properties make it an ideal workload for studying CPU scaling: if your scaling is poor on this program, you know the cause is *not* memory bandwidth or synchronisation, and so it must be CPU allocation.

Configuration is via environment variables:

- `MC_SAMPLES` — total Monte Carlo samples per run (default 200 M)
- `MC_REPEATS` — number of timed repeats (default 3; we report the best)
- `OMP_NUM_THREADS` — standard OpenMP environment variable

A Makefile is provided to compile it with `gcc -O3 -march=native -fopenmp`.

```bash
docker pull --platform linux/amd64 gcc:16.1

docker image save --platform Linux/amd64 gcc:16.1 -o my_gcc_16.1.tar

sudo apptainer build my_gcc_16.1.sif docker-archive://my_gcc_16.1.tar

# Step 4: Verify
apptainer exec my_gcc_16.1.sif gcc --version


# Step 5: Transfer to Leonardo
scp my_gcc_16.1.sif <username>@login.leonardo.cineca.it:$SCRATCH/containers/

# ── On Leonardo ───────────────────────────────────────────────
singularity exec $SCRATCH/containers/my_gcc_16.1.sif make

gcc -O3 -march=native -fopenmp -Wall -Wextra -o mc_pi_omp mc_pi_omp.c -fopenmp

# exec the program

singularity exec $SCRATCH/containers/my_gcc_16.1.sif mc_pi_omp
Monte Carlo Pi — OpenMP
  samples/run      = 200000000
  repeats          = 3
  OMP max threads  = 36
  run 1: 0.030 s   pi=3.141632
  run 2: 0.028 s   pi=3.141537
  run 3: 0.028 s   pi=3.141635

Best time        = 0.028 s
Throughput       = 7079.5 Msamples/s
Pi estimate      = 3.141635
```
**Questions**:
1. Explain what happens if you export the `MC_SAMPLES` variable from the host.
2. Is there another possibility to assign container ENV variables?
3. If I have a varieble set in the container and in the host, which one will be used?

Sometimes it is necessary to override an environment variable that is already set in the 
container with the value from the host, you can use APPTAINERENV_ or the --env flag. 

```bash
export APPTAINERENV_MC_SAMPLES="$MC_SAMPLES"
apptainer run mycontainer.sif

# or
apptainer run --env "MC_SAMPLES=$MC_SAMPLES"
```
If you do not want the host environment variables to pass into the container you can use the `-e ` or `--cleanenv` option. This gives a clean environment inside the container, with a minimal set of environment variables for correct operation of most software.

### 2.8 Best Practices for Reproducible Research Images

| Practice | Implementation |
|----------|---------------|
| Version everything | Use `From: ubuntu:22.04` not `From: ubuntu:latest` |
| Pin package versions | `pip install numpy==1.26.4 scipy==1.13.0` |
| Document the build | `%labels` section + Git-versioned `.def` file |
| Test in `%test` | Verify critical functionality at build time |
| Minimise image size | `apt-get clean`, `--no-install-recommends`, multi-stage builds |
| Store `.def` in Git | The `.def` file is the source of truth; the `.sif` is the binary artefact |
| Tag with versions | `my_sim_gromacs-2024.1_cuda-12.2.sif` not `my_sim_latest.sif` |

---

## Exercise 2 — Building and Importing Images {#exercise-2}


**Scientific context**: You are building a portable container for a climate modelling preprocessing pipeline. The pipeline requires Python 3.11, NetCDF4, xarray, dask, and the Climate Data Operators (CDO) toolkit. This software combination is not available as a CINECA module.

---

### Part 1 — Build a Scientific Python Image (15 pts)

On your **laptop** (or a Linux VM where you have root), write and build the following definition file:

```singularity
# File: climate_env.def
Bootstrap: docker
From: python:3.11-slim

%post
    # System dependencies for CDO and NetCDF
    apt-get update -y
    apt-get install -y --no-install-recommends \
        cdo \
        libnetcdf-dev \
        libhdf5-dev \
        gcc \
        g++ \
        wget \
        curl \
        ca-certificates
    apt-get clean
    rm -rf /var/lib/apt/lists/*

    # Python scientific stack
    pip install --no-cache-dir --upgrade pip
    pip install --no-cache-dir \
        numpy==1.26.4 \
        scipy==1.13.0 \
        netCDF4==1.7.1 \
        xarray==2024.2.0 \
        dask==2024.2.0 \
        matplotlib==3.8.3 \
        cftime==1.6.3

    # Create standard HPC mount points (for Lustre compatibility)
    mkdir -p /leonardo /scratch /work

%environment
    export LC_ALL=C.UTF-8
    export LANG=C.UTF-8

%test
    python3 -c "import numpy; print('numpy', numpy.__version__)"
    python3 -c "import xarray; print('xarray', xarray.__version__)"
    python3 -c "import netCDF4; print('netCDF4', netCDF4.__version__)"
    cdo --version

%labels
    Author MHPC_Student
    Version 1.0.0
    Description Climate preprocessing: Python 3.11 + CDO + xarray + dask

%help
    Climate preprocessing environment.
    Usage: apptainer exec climate_env.sif python3 my_script.py
```

```bash
# Build (on your laptop, requires sudo)
sudo apptainer build climate_env.sif climate_env.def

# Test locally
apptainer exec climate_env.sif python3 -c "import xarray; print('OK')"
apptainer exec climate_env.sif cdo --version
```

**Questions (5 pts)**:
1. How large is the resulting SIF file? Run `ls -lh climate_env.sif`.
2. The `%post` section installs CDO as an `apt` package. CDO 2.x may not be in the Ubuntu 22.04 repos. How would you compile CDO from source inside `%post` instead? Outline the steps.
3. Why do we create `/leonardo`, `/scratch`, `/work` directories in `%post`? What happens if these do not exist in the container when Apptainer tries to bind-mount them?

---

### Part 2 — Transfer and Run on Leonardo (8 pts)

```bash
# Transfer the SIF to Leonardo
scp climate_env.sif <username>@login.leonardo.cineca.it:$SCRATCH/containers/

# On Leonardo: write a Python test script on the host
cat > $SCRATCH/test_xarray.py << 'PYTHON'
import numpy as np
import xarray as xr

# Create a synthetic 2D temperature field (typical climate model output)
lon = np.linspace(-180, 180, 360)
lat = np.linspace(-90, 90, 180)
lon2d, lat2d = np.meshgrid(lon, lat)
temperature = 15.0 + 30.0 * np.cos(np.deg2rad(lat2d)) + np.random.randn(180, 360) * 2

ds = xr.Dataset({
    'temperature': xr.DataArray(
        temperature,
        dims=['lat', 'lon'],
        coords={'lat': lat, 'lon': lon},
        attrs={'units': 'degC', 'long_name': 'Surface Temperature'}
    )
})

print(ds)
print(f"Global mean temperature: {ds.temperature.mean().item():.2f} degC")

# Save as NetCDF4
ds.to_netcdf('$SCRATCH/synthetic_climate.nc')
print("Written: synthetic_climate.nc")
PYTHON

# Run inside the container on an interactive allocation
salloc --nodes=1 --ntasks=1 --cpus-per-task=4 --mem=8G \
       --time=00:30:00 --partition=dcgp_usr_prod --account=<your_account>

singularity exec $SCRATCH/containers/climate_env.sif \
    python3 $SCRATCH/test_xarray.py
```

**Questions**:
1. The Python script writes `synthetic_climate.nc` to `$SCRATCH`. Verify the file exists on the host after the container exits. Explain why this works without any explicit `--bind` flags.
2. Inspect the NetCDF file with CDO inside the container: `singularity exec climate_env.sif cdo info $SCRATCH/synthetic_climate.nc`. What metadata does CDO report?
3. Why is it preferable to keep the Python script on the host filesystem rather than embedding it in the `%runscript` section?

---

### Part 3 — Convert a Docker Image (7 pts)

If you have Docker installed on your laptop:

```bash
# On your laptop: pull the official GCC image 
docker pull --platform linux/amd64 gcc:16.1

# Export to tar
docker image save --platform Linux/amd64 gcc:16.1 -o my_gcc_16.1.tar

# Convert to SIF using Apptainer
ssudo apptainer build my_gcc_16.1.sif docker-archive://my_gcc_16.1.tar

# Check the resulting SIF size
ls -lh my_gcc_16.1.tar my_gcc_16.1.sif
```

Alternatively, pull directly on Leonardo (requires internet access on compute nodes or login node):

```bash
singularity pull $SCRATCH/containers/my_gcc_16.1.sif \
    docker://gcc:16.1
```

**Questions**:
1. Compare the size of `tar` vs `sif`. Explain the difference.
2. The Docker archive contains multiple image layers. How does Apptainer merge them into a single SIF? (Hint: SquashFS)


---

## Lecture 3 — OpenMP in Containers: Affinity, Binding, and NUMA {#lecture-3}

### 3.1 Why OpenMP Binding Matters Inside Containers

OpenMP parallelism distributes loop iterations across threads. Performance depends critically on where those threads execute: on which physical cores, on which socket, and with which memory access patterns. This is **thread affinity** or **thread binding**.

On Leonardo's DCGP nodes, the hardware topology is:

```
DCGP Node (Intel Sapphire Rapids)
├── Socket 0 (56 cores, NUMA node 0)
│   ├── 56 physical cores  → 112 logical CPUs (hyperthreading)
│   └── Local DDR5 DRAM: ~256 GB
└── Socket 1 (56 cores, NUMA node 1)
    ├── 56 physical cores  → 112 logical CPUs
    └── Local DDR5 DRAM: ~256 GB

Cross-socket (NUMA) memory access: ~2× latency compared to local access
```

Without explicit binding, OpenMP threads may migrate between cores during execution, causing:
- Cache invalidation overhead (thread moved to a core whose cache doesn't have the data)
- NUMA penalties (a thread on Socket 1 reads data allocated on Socket 0's memory)
- Load imbalance (all threads on one socket, other socket idle)

### 3.2 OpenMP Environment Variables

OpenMP thread affinity is controlled by standard environment variables, which work **identically inside and outside containers**:

| Variable | Values | Effect |
|----------|--------|--------|
| `OMP_NUM_THREADS` | integer | Number of OpenMP threads |
| `OMP_PROC_BIND` | `close`, `spread`, `master`, `false` | Thread placement policy |
| `OMP_PLACES` | `cores`, `threads`, `sockets` | Granularity of thread placement |
| `GOMP_CPU_AFFINITY` | CPU list (GCC-specific) | Explicit CPU mask for GCC's libgomp |
| `KMP_AFFINITY` | `granularity=...,compact,verbose` | Intel OpenMP (icc/ifx) affinity |

**Key combinations for scientific computing**:

```bash
# Strategy 1: All threads on one socket (maximise cache reuse, minimise NUMA)
export OMP_NUM_THREADS=56
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Strategy 2: Threads spread across both sockets (maximise memory bandwidth)
export OMP_NUM_THREADS=112
export OMP_PROC_BIND=spread
export OMP_PLACES=cores

# Strategy 3: Intel-specific compact binding (threads close together, hyperthreading aware)
# "A common usage scenario is to bind consecutive threads close together, as is done with KMP_AFFINITY=compact. 
# By doing this, communication overhead, cache line invalidation overhead, and page thrashing are minimized. 
# Now, suppose the application also had a number of parallel regions that did not utilize all of the available OpenMP threads.
# A thread normally executes faster on a core where it is not competing for resources with another active thread on \
# the same core. It is always good to avoid binding multiple threads to the same core while leaving other cores unused. 
export KMP_AFFINITY=granularity=fine,compact,1,0
export OMP_NUM_THREADS=56

# to bind to a specific core and socket
export KMP_AFFINITY=granularity=fine,proclist=[1-8],explicit
export OMP_NUM_THREADS=4

```


### 3.3 How Containers Inherit Environment Variables

A critical point: **environment variables set on the host before launching the container are visible inside the container**.

```bash
# On the host (SLURM job script or interactive session):
export OMP_NUM_THREADS=28
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# These variables are available inside the container:
singularity exec my_omp.sif ./my_openmp_program
# The OpenMP runtime inside the container reads OMP_NUM_THREADS=28
```

You can also set container-specific environment variables using `APPTAINERENV_` (or `SINGULARITYENV_`) prefix:

```bash
# Set OMP_NUM_THREADS=28 only inside the container, not on the host
export APPTAINERENV_OMP_NUM_THREADS=28
singularity exec my_omp.sif ./my_openmp_program
```

### 3.4 SLURM's Role in Pinning

SLURM's `--cpu-bind` option pins the entire container process to specific CPUs. This is the outer layer of affinity; OpenMP binding operates within the allocated CPU set.

```bash
# SLURM allocates 28 cores on Socket 0 and pins the job there
srun --nodes=1 --ntasks=1 --cpus-per-task=28 \
     --cpu-bind=cores \
     singularity exec my_omp.sif ./my_program
# OMP_NUM_THREADS=28 → 28 threads on 28 pinned cores → optimal
```

The interaction is:

```
SLURM --cpu-bind=cores  (outer: limits which CPUs the process can use)
    └── OMP_PROC_BIND=close  (inner: how OpenMP places threads within that set)
```

### 3.5 Inspecting Affinity from Inside the Container

```c
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
```

```bash
# Build the program inside the container (or outside if compatible glibc)
singularity exec my_omp.sif gcc -fopenmp -o affinity_report affinity_report.c

# Run with different binding strategies and compare
export OMP_NUM_THREADS=8
export OMP_PROC_BIND=close; export OMP_PLACES=cores
singularity exec my_omp.sif ./affinity_report

export OMP_PROC_BIND=spread
singularity exec my_omp.sif ./affinity_report
```

### 3.6 NUMA-Aware Memory Allocation Inside Containers

The `numactl` utility controls NUMA memory policies. On Leonardo's DCGP, ensure the container image includes `numactl`:

```singularity
%post
    apt-get install -y numactl
```

```bash
# Run the containerised application, pinning to NUMA node 0 (Socket 0)
singularity exec --bind /dev:/dev my_omp.sif \
    numactl --cpunodebind=0 --membind=0 ./my_program

# Interleave memory across both NUMA nodes (maximise bandwidth for array-heavy code)
singularity exec my_omp.sif \
    numactl --interleave=all ./my_program
```

---

## Exercise 3 — OpenMP Scaling and Binding on DCGP Nodes {#exercise-3}


**Scientific context**: You are evaluating the performance of an OpenMP-parallelised dense matrix multiplication (DGEMM) kernel inside a container. The goal is to characterise scaling efficiency and the impact of thread binding on DCGP nodes (Intel Sapphire Rapids, 2×56 cores).

---

### Part 1 — Build the Benchmark Container 

On your laptop, write and build:

```singularity
# File: omp_bench.def
Bootstrap: docker
From: ubuntu:22.04

%post
    apt-get update -y
    apt-get install -y --no-install-recommends \
        gcc \
        g++ \
        gfortran \
        libomp-dev \
        numactl \
        libnuma-dev \
        hwloc \
        linux-tools-common \
        linux-tools-generic \
        time
    apt-get clean
    rm -rf /var/lib/apt/lists/*

    # Create mount points for Leonardo filesystems
    mkdir -p /leonardo /scratch /work

%environment
    export LC_ALL=C
    export OMP_PROC_BIND=close
    export OMP_PLACES=cores

%test
    gcc --version
    numactl --show

%labels
    Description OpenMP benchmarking environment
```

Also write a C benchmark program `dgemm_omp.c`:

```c
// File: dgemm_omp.c
// Dense matrix multiplication: C = A × B
// Compile inside container: gcc -fopenmp -O3 -march=native -o dgemm_omp dgemm_omp.c
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define N 2048  // Matrix dimension

int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : N;
    
    double *A = malloc(n * n * sizeof(double));
    double *B = malloc(n * n * sizeof(double));
    double *C = calloc(n * n, sizeof(double));
    
    // Initialise matrices
    #pragma omp parallel for
    for (int i = 0; i < n * n; i++) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
    }
    
    double t_start = omp_get_wtime();
    
    // Tiled matrix multiplication (cache-friendly)
    int T = 64;  // Tile size
    #pragma omp parallel for schedule(dynamic,1) collapse(2)
    for (int ii = 0; ii < n; ii += T)
        for (int jj = 0; jj < n; jj += T)
            for (int kk = 0; kk < n; kk += T)
                for (int i = ii; i < ii + T && i < n; i++)
                    for (int k = kk; k < kk + T && k < n; k++) {
                        double a_ik = A[i * n + k];
                        for (int j = jj; j < jj + T && j < n; j++)
                            C[i * n + j] += a_ik * B[k * n + j];
                    }
    
    double t_end = omp_get_wtime();
    double elapsed = t_end - t_start;
    double gflops = 2.0 * n * n * n / elapsed / 1e9;
    
    printf("N=%d  threads=%d  time=%.3fs  GFLOPS=%.2f\n",
           n, omp_get_num_threads(), elapsed, gflops);
    
    free(A); free(B); free(C);
    return 0;
}
```

**Build the benchmark**:
```bash
# On Leonardo (compile inside the container on a compute node)
salloc --nodes=1 --ntasks=1 --cpus-per-task=56 --mem=128G \
       --time=01:00:00 --partition=dcgp_usr_prod --account=<your_account>

singularity exec $SCRATCH/containers/omp_bench.sif \
    gcc -fopenmp -O3 -march=native -o $SCRATCH/dgemm_omp $SCRATCH/dgemm_omp.c
```

---

### Part 2 — Thread Scaling Study 

Write a SLURM script that runs the benchmark with different thread counts:

```bash
cat > $SCRATCH/omp_scaling.slurm << 'SLURM'
#!/bin/bash
#SBATCH --job-name=omp_scaling
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=112          # Request full node (2 sockets × 56 cores)
#SBATCH --mem=200G
#SBATCH --time=00:30:00
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=<your_account>
#SBATCH --output=omp_scaling.%j.out

SIF=$SCRATCH/containers/omp_bench.sif
BIN=$SCRATCH/dgemm_omp

export OMP_PLACES=cores

echo "=== TOPOLOGY ==="
singularity exec $SIF numactl --hardware
echo ""

for NTHREADS in 1 2 4 7 14 28 56 112; do
    export OMP_NUM_THREADS=$NTHREADS
    
    echo "--- OMP_PROC_BIND=close, threads=$NTHREADS ---"
    export OMP_PROC_BIND=close
    singularity exec $SIF $BIN 2048
    
    echo "--- OMP_PROC_BIND=spread, threads=$NTHREADS ---"
    export OMP_PROC_BIND=spread
    singularity exec $SIF $BIN 2048
done
SLURM

sbatch $SCRATCH/omp_scaling.slurm
```

**Questions **:
1. Plot GFLOPS vs thread count for both `close` and `spread` binding strategies. At what thread count does `close` outperform `spread`? Explain physically why.
2. Calculate the **parallel efficiency** E(N) = T(1) / (N × T(N)) for each thread count. Where does efficiency drop most sharply? What hardware effect causes this?
3. At 56 threads (one full socket), compare `close` vs `spread`. Which is faster for DGEMM and why? (Hint: consider L3 cache vs memory bandwidth.)
4. At 112 threads (hyperthreading enabled), does performance improve compared to 56 threads? Explain in terms of floating-point unit availability on Ice Lake/Sapphire Rapids.

---

### Part 3 — Affinity Visibility 

```bash
# Run the affinity_report program (compile it from Lecture 3)
singularity exec $SIF gcc -fopenmp -o $SCRATCH/affinity_report $SCRATCH/affinity_report.c

# Test: no binding
export OMP_NUM_THREADS=8; unset OMP_PROC_BIND; unset OMP_PLACES
singularity exec $SIF $SCRATCH/affinity_report

# Test: close binding on cores
export OMP_PROC_BIND=close; export OMP_PLACES=cores
singularity exec $SIF $SCRATCH/affinity_report

# Test: spread across sockets
export OMP_PROC_BIND=spread; export OMP_PLACES=sockets
singularity exec $SIF $SCRATCH/affinity_report
```

**Questions **:
1. For each binding strategy, report which physical CPUs are assigned to which threads (from the `sched_getcpu()` output). Do the CPU numbers match what `numactl --hardware` reports for each NUMA node?
2. What happens to thread placement if you do not set `OMP_PROC_BIND`? Is there a systematic pattern or random placement?
3. From inside the container, run `hwloc-ls` (if installed in the container). Does the container see the full node topology? Does it correctly identify the two sockets and their associated memory banks?


### Part 4 —  Affinity and bandwidt
To properly test the affinity and bandwidth we use the code prepared for the OMP lecures. In particular:

```C

/* ────────────────────────────────────────────────────────────────────────── *
 │                                                                            │
 │ This file is part of the exercises for the Lectures on                     │
 │   "Foundations of High Performance Computing"                              │
 │ given at                                                                   │
 │   Master in HPC and                                                        │
 │   Master in Data Science and Scientific Computing                          │
 │ @ SISSA, ICTP and University of Trieste                                    │
 │                                                                            │
 │ contact: luca.tornatore@inaf.it                                            │
 │                                                                            │
 │     This is free software; you can redistribute it and/or modify           │
 │     it under the terms of the GNU General Public License as published by   │
 │     the Free Software Foundation; either version 3 of the License, or      │
 │     (at your option) any later version.                                    │
 │     This code is distributed in the hope that it will be useful,           │
 │     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
 │     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
 │     GNU General Public License for more details.                           │
 │                                                                            │
 │     You should have received a copy of the GNU General Public License      │
 │     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
 │                                                                            │
 * ────────────────────────────────────────────────────────────────────────── */

// ················································
//
#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif

#if !defined(_OPENMP)
#error "OpenMP support is mandatory for this code"
#endif

// ················································
//

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>

// needed for numalib interface
#include <numa.h>
#include <numaif.h>


#include <omp.h>

#define CPU_TIME_T ({ struct timespec myts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), \
(double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })

// ················································
//

double stream            ( const double * restrict array, unsigned int N, double *, double * );
void   probe_memory_alloc( uint64_t  );
  
// ················································
//

  
int main( int argc, char **argv )
{
  int numa_alloc             = 0;
  int probe_memory_placement = 0;
  
  {
    int opt;
    while ((opt = getopt(argc, argv, "hpn")) != -1)
      {
	switch(opt)
	  {
	  case 'h':
	    printf("possible options:\n"
		   "-n allocate with explicit numa placement\n"
		   "-p probe where mem pages are\n");
	    return 0;
	  case 'p': probe_memory_placement = 1; break;
	  case '?': printf("invalid option\n"); return 1;
	  }
      }
  }

  
  char *places = getenv("OMP_PLACES");
  char *bind   = getenv("OMP_PROC_BIND");

  if ( (places == NULL) ||
       ( (places!= NULL) && (strcasecmp(places, "sockets")) ) ) {
    printf("It is mandatory to set OMP_PLACES=sockets to run this code\n");
    return 1; }
  
  if ( bind != NULL )
    printf("OMP_PROC_BINDING is set to %s\n", bind);

  int nsockets;
 #pragma omp parallel
 #pragma omp single
  nsockets = omp_get_num_places();

  
  double *data[nsockets];
  int     thread_to_socket[nsockets];
 #define DOUBLE_DATASIZE (1 << 26)   // get beyond the L3
  
 #pragma omp parallel num_threads(nsockets) proc_bind(spread)
  {   
    int me       = omp_get_thread_num();
    int mysocket = omp_get_place_num();  // get on what socket this thread is running on

    thread_to_socket[me] = mysocket;

    if ( numa_alloc )
      {
	// use libnuma to allocate memory on the nearest NUMA node
	data[me] = (double*)numa_alloc_onnode(DOUBLE_DATASIZE * sizeof(double), mysocket);
      }
    else
      {    
	data[me] = (double*)aligned_alloc( 64, DOUBLE_DATASIZE*sizeof(double) );
      }
    
    // initialize, touch, drag through cache and TLB
    for ( int i = 0; i < DOUBLE_DATASIZE; i++ )
      data[me][i] = (double)i;
    
   #pragma omp barrier

   #pragma omp single
    {      
      if ( probe_memory_placement )
	// we want to show where exactly the memory pages
	// are mapped
	probe_memory_alloc( DOUBLE_DATASIZE * sizeof(double) );
      printf( "running the experiment for %d sockets..\n\n", nsockets );  
    }


    double bw[nsockets];   // nsockets is expected to be just a few

    /* ******************************************************************* *
     *  R U N  the   E X P E R I M E N T                                   *     
     * ******************************************************************* */
    
    for ( int runner = 0; runner < nsockets; runner ++ )
      {
	if ( me == runner )
	  {
	    printf( "SOCKET %d\n", runner );	    
	    
	    for ( int j = 0; j < nsockets; j++ ) {
	      printf( "\trunning the stream for S%d <-> S%d : ",
		      mysocket, thread_to_socket[j]);
	      double foo1, foo2;
	      bw[j] = stream ( data[j], DOUBLE_DATASIZE, &foo1, &foo2 );
	      printf( "BW: %4.2g +- %5.3f GB/s\n",
		      bw[j], foo1 );
	      if ( isnan(foo2) )
		printf("This will never be printed, but convince the "
		       "optimizer we really need the loop in strem()\n");
	    }	    
	    
	    printf("----------------------------------------\n\n");
	  }
       #pragma omp barrier
       #pragma omp single
	if ( probe_memory_placement )
	  probe_memory_alloc( DOUBLE_DATASIZE*sizeof(double) );
      }

    // ----------------------------------------
    //  OUTPUT the Bandwidth matrix
    // ----------------------------------------
    
   #pragma omp single
    {
      printf("\nBandwidth matrix (GB/s)\n\n");
      printf("%6s ", "");
      for (int i = 0; i < nsockets; i++)
	printf("S%-6d ", i);
      printf("\n-------");
      for (int i = 0; i < nsockets; i++)
	printf("--------");
      printf("\n");
    }
    
   #pragma omp for ordered
    for ( int runner = 0; runner < nsockets; runner++ )
     #pragma omp ordered
      {
	printf("S%-3d | ", runner);
	for (int i = 0; i < nsockets; i++)
	  printf("%-6.2g  ", bw[i]);
	printf("\n");
      }
   #pragma omp single
    printf("\n");
    

    // ----------------------------------------
    //  Release the memory
    // ----------------------------------------
    
   #pragma omp barrier

    if ( numa_alloc )
       numa_free( data[me], DOUBLE_DATASIZE * sizeof(double) );
    else
       free( data[me] );
   
  }

  return 0;
}


int cmp( const void *a, const void *b )
{
  const double A = *((double*)a);
  const double B = *((double*)b);
  return (A>B) - (A<B);
}

double stream( const double * restrict array, unsigned int N, double *stddev, double *result )
{
 #if !defined(UNROLL_FACTOR)
 #define UNROLL_FACTOR 8
 #endif
 #if !defined(NREPS)
 #define NREPS 1
 #warning "using just 1 repetition for the stream: compile with -DNREPS=N to have N repetitions (advice: N>~10)" 
 #endif


  //unsigned int _N  = N&0xFFFFFF8;  // renders clear that _N is a multiple of 8
  unsigned int _N  = N&(~((UNROLL_FACTOR)-1));  // renders clear that _N is a multiple of 8
  const double *_a = (const double*) __builtin_assume_aligned( array, 64 );

 
  double timings[NREPS] = {0};

  for ( int r = 0; r < NREPS; r++ )
    {
      double S[UNROLL_FACTOR] = {0};
      
      double timing = CPU_TIME_T;
     #pragma GCC ivdep
      for ( unsigned int i = 0; i < _N; i += UNROLL_FACTOR )
	{
	  for ( int j = 0; j < UNROLL_FACTOR; j++)
	    S[j] += _a[i+j]*2.0 + 1.0;
	}      
      timing = CPU_TIME_T - timing;

      timings[r] = timing;
      
      // induce the compiler not to optimize out
      for ( int j = 1; j < UNROLL_FACTOR; j++ )
	S[0] += S[j];
      *result += S[0];
    }

  double avgtime;
  double std_dev = 0;
  
  // sort the timings
  if (NREPS > 3)
    {
      qsort( timings, NREPS, sizeof(double), cmp );
      avgtime = timings[0];
      
      for ( int j = 1; j < NREPS-2; j++ )
	avgtime += timings[j];
      avgtime /= (NREPS-2);

      for ( int j = 0; j < NREPS-2; j++ )
	std_dev += (timings[j] - avgtime)*(timings[j] - avgtime);
      std_dev = sqrt(std_dev / (NREPS-2));
    }
  else
    avgtime = timings[0];

  *stddev = std_dev;
  
  double bandwidth = ( ((double)_N * sizeof(double))/(1<<30) / avgtime );   // calculate the bandwitdh in GB/s

  return bandwidth;
}


void probe_memory_alloc( uint64_t msize )
{
  // get the page size
  size_t pagesize = sysconf(_SC_PAGESIZE);

  // get how many pages we allcoated
  size_t npages = msize / pagesize;

  printf("···········································\n"
	 "probing memory placement on Numa Nodes:\n\n" );
  // visualize where the memory pages are allocated
  char cmd[256];
  snprintf(cmd, sizeof(cmd),
	   "awk '/anon=/ { for(i=1;i<=NF;i++) if($i ~ /^anon=/) { "
	   "split($i,a,\"=\"); if(a[2] > %ld ) print } }' /proc/%d/numa_maps",
	   npages, getpid());
  system(cmd);
  //
  printf("\n···········································\n\n");
  fflush(stdout);
}
```

You can compile it on leonardo using the `omp_bench.sif`

```bash
run -N 1 --partition=dcgp_usr_prod --account=INA24_C3T03_0 --nodes=1 --ntasks=2 --cpus-per-task=56 --pty /bin/bash

export SINGULARITY_CACHEDIR=$SCRATCH/.singularity_cache

export SIF=$SINGULARITY_CACHEDIR/omp_bench.sif

singularity exec $SIF gcc $PWD/bandwidth.numa_alloc.c -fopenmp -o bandwidth.numa_alloc -lnuma

singularity exec $SIF ./bandwidth.numa_alloc

singularity exec --env OMP_PLACES=sockets $SIF ./bandwidth.numa_alloc -b
OMP_PROC_BINDING is set to close
Explicit memory binding: on
running the experiment for 2 sockets..

SOCKET 0
	running the stream for S0 <-> S0 : BW:  4.6 +- 0.000 GB/s
	running the stream for S0 <-> S1 : BW:  4.1 +- 0.000 GB/s
----------------------------------------

SOCKET 1
	running the stream for S1 <-> S0 : BW:  4.1 +- 0.000 GB/s
	running the stream for S1 <-> S1 : BW:  4.5 +- 0.000 GB/s
----------------------------------------


Bandwidth matrix (GB/s)

       S0      S1
-----------------------
S0   | 4.6     4.1
S1   | 4.1     4.5
```

or using leonardo modules and compare the differences

```bash 
# load module
module  load gcc/12.2.0
Loading gcc/12.2.0
  Loading requirement: zlib-ng/2.1.6-jkgunjc zstd/1.5.6-uq5yyux binutils/2.42

# verify the compiler
gcc --version
gcc (Spack GCC) 12.2.0
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# compile 
gcc bandwidth.numa_alloc.c -fopenmp -lnuma -o bandwidth.numa_alloc_hw

# run 
export OMP_PLACES=sockets

./bandwidth.numa_alloc_hw
Explicit memory binding: off
running the experiment for 2 sockets..

SOCKET 0
	running the stream for S0 <-> S0 : BW:  4.6 +- 0.000 GB/s
	running the stream for S0 <-> S1 : BW:  4.1 +- 0.000 GB/s
----------------------------------------

SOCKET 1
	running the stream for S1 <-> S0 : BW:  4.1 +- 0.000 GB/s
	running the stream for S1 <-> S1 : BW:  4.5 +- 0.000 GB/s
----------------------------------------


Bandwidth matrix (GB/s)

       S0      S1
-----------------------
S0   | 4.6     4.1
S1   | 4.1     4.5
```

---
### Part 5 —  Affinity and latency

We use the OMP code to test latency in both containers and host.

```c

/* ────────────────────────────────────────────────────────────────────────── *
 │                                                                            │
 │ This file is part of the exercises for the Lectures on                     │
 │   "Foundations of High Performance Computing"                              │
 │ given at                                                                   │
 │   Master in HPC and                                                        │
 │   Master in Data Science and Scientific Computing                          │
 │ @ SISSA, ICTP and University of Trieste                                    │
 │                                                                            │
 │ contact: luca.tornatore@inaf.it                                            │
 │                                                                            │
 │     This is free software; you can redistribute it and/or modify           │
 │     it under the terms of the GNU General Public License as published by   │
 │     the Free Software Foundation; either version 3 of the License, or      │
 │     (at your option) any later version.                                    │
 │     This code is distributed in the hope that it will be useful,           │
 │     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
 │     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
 │     GNU General Public License for more details.                           │
 │                                                                            │
 │     You should have received a copy of the GNU General Public License      │
 │     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
 │                                                                            │
 * ────────────────────────────────────────────────────────────────────────── */

// ················································
//
#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif

#if !defined(_OPENMP)
#error "OpenMP support is mandatory for this code"
#endif

// ················································
//

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>

// needed for numalib interface
#include <numa.h>
#include <numaif.h>

// needed for large pages stuff
#include <sys/mman.h>

#include <omp.h>

#define CPU_TIME_T ({ struct timespec myts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), \
(double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })

// ················································
//

#define BUFFER_SIZE (256 * 1024 * 1024) // larger than L3
#define CLS 64                          // Cache Line Size
#define STRIDE_ELEMENTS (CLS / sizeof(uint64_t))  // how many uint64_t elements fit in one cache line
#define NUM_LINES (BUFFER_SIZE / CLS) // how many L1 lines are needed for the buffer size


void probe_memory_alloc( uint64_t );

  
// ················································
//

  
int main( int argc, char **argv )
{
  int bind_memory            = 0;
  int numa_alloc             = 0;
  int probe_memory_placement = 0;
  
  {
    int opt;
    while ((opt = getopt(argc, argv, "hbnp")) != -1)
      {
	switch(opt)
	  {
	  case 'h':
	    printf("possible options:\n"
		   "-b explicitly bind memory\n"
		   "-n allocate with explicit numa placement\n"
		   "-p probe where mem pages are\n");
	    return 0;
	  case 'b': bind_memory = 1; break;
	  case 'n': numa_alloc = 1; break;
	  case 'p': probe_memory_placement = 1; break;
	  case '?': printf("invalid option\n"); return 1;
	  }
      }
  }

  
  char *places = getenv("OMP_PLACES");
  char *bind   = getenv("OMP_PROC_BIND");

  if ( (places == NULL) ||
       ( (places!= NULL) && (strcasecmp(places, "sockets")) ) ) {
    printf("It is mandatory to set OMP_PLACES=sockets to run this code\n");
    return 1; }
  
  if ( bind != NULL )
    printf("OMP_PROC_BINDING is set to %s\n", bind);

  printf("Explicit memory binding: %s\n", (bind_memory?"on":"off"));
  
  int nsockets;
 #pragma omp parallel
 #pragma omp single
  nsockets = omp_get_num_places();

  
  uint64_t *data[nsockets];
  int     thread_to_socket[nsockets];
 #define UINT64_DATASIZE (1 << 26)   // get beyond the L3
  
 #pragma omp parallel num_threads(nsockets) proc_bind(spread)
  {

    // --------------------------------------------------------------------------------------
    // 0. setting up
    // --------------------------------------------------------------------------------------
    
    int me       = omp_get_thread_num();
    int mysocket = omp_get_place_num();  // get on what socket this thread is running on
    uint64_t *indexes;
    
    thread_to_socket[me] = mysocket;
   if( numa_alloc ) 
     {
       data[me] = (uint64_t*)numa_alloc_onnode(BUFFER_SIZE * sizeof(uint64_t), mysocket);
       indexes  = (uint64_t*)numa_alloc_onnode(NUM_LINES * sizeof(uint64_t), mysocket);
     }
   else
     {
       data[me] = (uint64_t*)aligned_alloc( 64, BUFFER_SIZE );
       indexes  = (uint64_t*)aligned_alloc( 64, NUM_LINES*sizeof(uint64_t) );
     }


   #pragma omp single
    printf("\n[o] setting up random walk for %d cache lines..\n", NUM_LINES);

    
    // --------------------------------------------------------------------------------------
    // 1. initialize a regular path of cache lines indexes
    // --------------------------------------------------------------------------------------
    
    // note: this also amounts to a first-touch and should force the memory
    //       allocation to the nearest possible NUMA node 
    for ( uint64_t i = 0; i < NUM_LINES; i++ )
      indexes[i] = i;

    
    // --------------------------------------------------------------------------------------
    // 2. randomize the path
    // --------------------------------------------------------------------------------------
    
    unsigned short myseeds[3] = { 1234*me, 567^me, 8919+(1<<me) };
    //unsigned short myseeds[3] = { time(NULL)*me, time(NULL)^me, time(NULL)+(1<<me) };
    
    for ( uint64_t i = NUM_LINES-1; i > 0; i-- )
      {
	uint64_t j    = nrand48( myseeds ) % (i+1);
	uint64_t temp = indexes[i];
	indexes[i]    = indexes[j];
	indexes[j]    = temp;
      }

    for ( uint64_t i = 0; i < NUM_LINES; i++ )
      if ( indexes[i] > NUM_LINES )
	printf("idx[%lu] = %lu\n", i, indexes[i]);
    
   #pragma omp single
    printf("[o] building the dependency chain..\n");

    
    // --------------------------------------------------------------------------------------
    // 3. Build the Dependent Chain
    // array[current_random_line] = next_random_line
    // --------------------------------------------------------------------------------------
    
    for (uint64_t i = 0; i < NUM_LINES - 1; i++)
      {
	if ( indexes[i] * STRIDE_ELEMENTS >= BUFFER_SIZE/sizeof(uint64_t))
	  printf("out-of-bound for i=%lu : %lu %lu\n", i, indexes[i] * STRIDE_ELEMENTS, BUFFER_SIZE/sizeof(uint64_t));
	data[me][indexes[i] * STRIDE_ELEMENTS] = indexes[i + 1] * STRIDE_ELEMENTS;
      }
    
    // Close the loop so it can run infinitely
    data[me][indexes[NUM_LINES - 1] * STRIDE_ELEMENTS] = indexes[0] * STRIDE_ELEMENTS;

    if ( bind_memory )
      {
	// after the first touch, bind to the local node,
	// beyond the current memory policy
	// is numa allocation was used, this is not necessary
	unsigned long nodemask = 1UL << mysocket;
	mbind(data[me], UINT64_DATASIZE*sizeof(uint64_t),
	      MPOL_BIND, &nodemask, sizeof(nodemask)*8, 0);
	mbind(indexes, NUM_LINES*sizeof(uint64_t),
	      MPOL_BIND, &nodemask, sizeof(nodemask)*8, 0);
      }

   #pragma omp barrier

    
    // --------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------
    
   #pragma omp single
    {
      if ( probe_memory_placement )
	// we want to show where exactly the memory pages
	// are mapped
	probe_memory_alloc( UINT64_DATASIZE * sizeof(double) );
      printf( "[o] running the experiment for %d sockets..\n\n", nsockets );
    }

    double L[nsockets];   // nsockets is expected to be just a few
    
    for ( int runner = 0; runner < nsockets; runner ++ )
      {
	if ( me == runner )
	  {
	    printf( "SOCKET %d\n", thread_to_socket[me] );	    
	    
	    for ( int j = 0; j < nsockets; j++ )
	      {
		// -------------------------------------------------------------------------
		// 4. Measure the Latency
		// -------------------------------------------------------------------------
		
		uint64_t current_index = indexes[0] * STRIDE_ELEMENTS;
		uint64_t iterations = 10000000; // 10 million reads
		
		struct timespec start, end;

		double timing = CPU_TIME_T;
		// THE HOT LOOP: The CPU cannot execute the next read until the current one finishes
		for (uint64_t i = 0; i < iterations; i++)
		  current_index = data[j][current_index];
		timing = CPU_TIME_T - timing;
		timing /= iterations;

		L[j] = timing*1000000000ULL;
		
		printf("\t\tAverage Memory Latency S%d <-> S%d: %7.2f ns\n",
		       thread_to_socket[me], thread_to_socket[j], L[j]);


		// 5. cheat to the Compiler: 
		// Use the final result so -O3 doesn't delete the entire loop
		if (current_index == 0xffffffffffffffff)
		  printf("This will never print, but keeps the optimizer honest.\n");
		
	      }
	  }
       #pragma omp barrier
      }


    // ----------------------------------------
    //  OUTPUT the Latency matrix
    // ----------------------------------------

   #pragma omp single
    {
      printf("\nLatency matrix (ns)\n\n");
      printf("%6s ", "");
      for (int i = 0; i < nsockets; i++)
	printf("S%-6d ", i);
      printf("\n-------");
      for (int i = 0; i < nsockets; i++)
	printf("--------");
      printf("\n");
    }
    
   #pragma omp for ordered
    for ( int runner = 0; runner < nsockets; runner++ )
     #pragma omp ordered
      {
	printf("S%-3d | ", runner);
	for (int i = 0; i < nsockets; i++)
	  printf("%-6d  ", (unsigned int)L[i]);
	printf("\n");
      }
   #pragma omp single
    printf("\n");


    // ----------------------------------------
    //  Release the memory
    // ----------------------------------------

   #pragma omp barrier
    free( data[me] );
    free( indexes );
  }

  return 0;
}



void probe_memory_alloc( uint64_t msize )
{
  // get the page size
  size_t pagesize = sysconf(_SC_PAGESIZE);

  // get how many pages we allcoated
  size_t npages = msize / pagesize;

  printf("···········································\n"
	 "probing memory placement on Numa Nodes:\n\n" );
  // visualize where the memory pages are allocated
  char cmd[256];
  snprintf(cmd, sizeof(cmd),
	   "awk '/anon=/ { for(i=1;i<=NF;i++) if($i ~ /^anon=/) { "
	   "split($i,a,\"=\"); if(a[2] > %ld ) print } }' /proc/%d/numa_maps",
	   npages, getpid());
  system(cmd);
  //
  printf("\n···········································\n\n");
  fflush(stdout);
}
```
To compile using container we do:

```bash
#compile
singularity exec $SIF gcc $PWD/memory_latency.c -fopenmp -o latency -lnuma

#run
singularity exec $SIF ./latency
It is mandatory to set OMP_PLACES=sockets to run this code
[gtaffoni@lrdn4252 gtaffoni]$ singularity exec --env OMP_PLACES=sockets $SIF ./latency
OMP_PROC_BINDING is set to close
Explicit memory binding: off

[o] setting up random walk for 4194304 cache lines..
[o] building the dependency chain..
[o] running the experiment for 2 sockets..

SOCKET 0
		Average Memory Latency S0 <-> S0:  104.42 ns
		Average Memory Latency S0 <-> S1:  195.07 ns
SOCKET 1
		Average Memory Latency S1 <-> S0:  188.62 ns
		Average Memory Latency S1 <-> S1:  124.83 ns

Latency matrix (ns)

       S0      S1
-----------------------
S0   | 104     195
S1   | 188     124
```

---

## Lecture 4 — MPI and GPU in Containers {#lecture-4}

### 4.1 MPI in Containers: The Hybrid Model

Multi-node MPI parallelism inside containers requires careful design. There are two architectural approaches:

**Model 1: MPI inside the container (self-contained)**

```
Host SLURM + PMI
     │
     ▼
mpirun → [container: MPI + app]  [container: MPI + app]  ...
              Node 1                    Node 2
```

Each container instance uses its own embedded MPI library. Communication must still traverse the host InfiniBand network, requiring that the container MPI and host network stack are ABI-compatible.

**Model 2: MPI outside the container (hybrid / recommended on CINECA)**

```
Host SLURM + PMI
     │
     ▼
mpirun (host OpenMPI) → singularity exec [container: app only]
                              │
                        Container uses host MPI for communication
                        via PMI2/PMIx process management interface
```

> **CINECA recommendation**: Use the **host MPI** (`module load hpcx-mpi`) to launch, and `singularity exec` to wrap only the application. The container provides the application binary and its non-MPI dependencies; the host provides high-performance MPI and InfiniBand access.

### 4.2 Why Host MPI + Container Application?

| Concern | Inside-container MPI | Host MPI + container app |
|---------|---------------------|--------------------------|
| InfiniBand UCX compatibility | Requires UCX inside container (complex) | Host UCX used automatically |
| Process manager (PMI2/PMIx) | Must match host SLURM | Automatic via host |
| NUMA-aware memory allocation | Must configure inside container | Host numactl or hwloc |
| MPI version management | User responsibility | CINECA maintains modules |
| Portability across systems | High | Low (depends on host MPI) |

For production HPC workloads where InfiniBand performance is critical, the host MPI approach is strongly preferred.

### 4.3 Practical MPI + Singularity on Leonardo

```bash
# SLURM script: MPI job with Singularity
#!/bin/bash
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=4           # 4 OpenMP threads per MPI rank
#SBATCH --mem=250G
#SBATCH --time=00:30:00
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=<your_account>

# Load the host MPI (OpenMPI / HPC-X)
module purge
module load hpcx-mpi/2.19

# OpenMP settings (propagate into the container via environment)
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Launch: mpirun from HOST, exec into CONTAINER
mpirun -np $SLURM_NTASKS \
       --map-by socket:PE=${SLURM_CPUS_PER_TASK} \
       --bind-to core \
       singularity exec --env OPAL_PREFIX=/usr $SCRATCH/containers/my_sim.sif \
           ./my_mpi_omp_program
```


### 4.4 GPU Passthrough with `--nv`

Apptainer/Singularity provides the `--nv` flag to expose NVIDIA GPUs from the host into the container:

```bash
singularity exec --nv my_gpu_container.sif ./my_cuda_program
```

What `--nv` does:
1. Detects NVIDIA GPUs on the host (`/dev/nvidia*` devices)
2. Binds the host CUDA driver libraries into the container at the correct paths
3. Exposes `/dev/nvidia0`, `/dev/nvidia1`, etc. inside the container
4. The container does **not** need to ship CUDA driver libraries — only CUDA runtime (`libcudart`) and above

> **Critical rule**: The CUDA *runtime* version inside the container must be ≤ the CUDA *driver* version on the host. On Leonardo's Booster partition, the NVIDIA driver supports CUDA 12.x. Container images built against CUDA ≥ 12.0 will work; images requiring CUDA > driver version will fail.

```
Host (Leonardo Booster)
├── NVIDIA A100 SXM4 64GB  (×4 per node)
├── NVIDIA driver: supports CUDA 12.x
└── /dev/nvidia{0,1,2,3}, /dev/nvidiactl, /dev/nvidia-uvm

Container (--nv flag)
├── CUDA runtime (libcudart.so.12.x): FROM container image
├── CUDA driver libraries: INJECTED from host at runtime
└── CUDA toolkit (nvcc, cuBLAS, cuDNN, etc.): FROM container image
```

### 4.5 Selecting Specific GPUs

```bash
# Use all available GPUs (SLURM allocates them via --gres=gpu:4)
singularity exec --nv my_gpu.sif python3 my_training.py

# Restrict to specific GPUs using CUDA_VISIBLE_DEVICES
CUDA_VISIBLE_DEVICES=0,1 singularity exec --nv my_gpu.sif ./my_cuda_app

# Inside the container: verify GPU visibility
singularity exec --nv my_gpu.sif nvidia-smi
```

### 4.6 SLURM Script: GPU Job on the Booster Partition

```bash
#!/bin/bash
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4           # 1 MPI rank per GPU
#SBATCH --cpus-per-task=8             # 8 OpenMP threads per rank (32 core / 4 ranks)
#SBATCH --gres=gpu:4                  # 4 A100 GPUs per node
#SBATCH --mem=200G
#SBATCH --time=01:00:00
#SBATCH --partition=boost_usr_prod    # GPU partition
#SBATCH --account=<your_account>
#SBATCH --output=gpu_job.%j.out
#SBATCH --error=gpu_job.%j.err

module purge
module load hpcx-mpi/2.19
module load cuda/12.2

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Each MPI rank is bound to one GPU via CUDA_VISIBLE_DEVICES
# This is managed automatically by SLURM when --gres=gpu:4 is set
mpirun -np $SLURM_NTASKS \
       --map-by ppr:4:node:PE=${SLURM_CPUS_PER_TASK} \
       --bind-to core \
       singularity exec --nv $SCRATCH/containers/my_gpu_sim.sif \
           ./my_mpi_cuda_program --input data.h5 --output results.h5
```

### 4.7 Hybrid MPI+OpenMP+GPU: Full Example with GROMACS

GROMACS is an exemplary case of a production scientific application that combines all three parallelism levels:

```bash
#!/bin/bash
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=4       # 1 rank per GPU
#SBATCH --cpus-per-task=8         # 8 CPU threads per rank
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod
#SBATCH --account=<your_account>
#SBATCH --time=04:00:00

module purge
module load hpcx-mpi/2.19

export OMP_NUM_THREADS=8

# GROMACS from NVIDIA NGC container
GROMACS_SIF=$SCRATCH/containers/gromacs_2024.1.sif

# GROMACS topology and input prepared on host filesystem
RUNDIR=$SCRATCH/gromacs_run

mpirun -np 16 \
    --map-by ppr:4:node:PE=8 \
    singularity exec --nv $GROMACS_SIF \
        gmx_mpi mdrun \
            -ntmpi 1 \
            -ntomp 8 \
            -gpu_id 0123 \
            -s $RUNDIR/topol.tpr \
            -deffnm $RUNDIR/md \
            -maxh 3.9
```

---

## Exercise 4 — Hybrid MPI+OpenMP and GPU Passthrough on the Booster {#exercise-4}


**Scientific context**: You are benchmarking a hybrid MPI+OpenMP heat equation solver and a GPU-accelerated SAXPY kernel, both inside containers, on Leonardo's Booster partition.

---

### Part 1 — Build the MPI+OpenMP Container (5 pts)

```singularity
# File: mpi_omp.def
Bootstrap: docker
From: ubuntu:22.04

%post
    apt-get update -y
    apt-get install -y --no-install-recommends \
        gcc \
        g++ \
        gfortran \
        libopenmpi-dev \
        openmpi-bin \
        libnuma-dev \
        numactl \
        hwloc
    apt-get clean
    rm -rf /var/lib/apt/lists/*
    mkdir -p /leonardo /scratch /work

%environment
    export OMPI_MCA_btl=^openib     # Use UCX (host InfiniBand) not internal OpenIB
    export OMPI_MCA_pml=ucx
    export OMP_PROC_BIND=close
    export OMP_PLACES=cores
```

Write a hybrid MPI+OpenMP ring communication benchmark:

```c
// File: mpi_omp_ring.c
// Each MPI rank does work with OMP threads, then passes a token around the ring
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ARRAY_SIZE (1 << 22)   // 4M doubles = 32 MB per rank

int main(int argc, char *argv[]) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int nthreads = omp_get_max_threads();
    if (rank == 0)
        printf("MPI ranks: %d, OMP threads/rank: %d\n", size, nthreads);
    
    // Allocate work array
    double *data = malloc(ARRAY_SIZE * sizeof(double));
    for (int i = 0; i < ARRAY_SIZE; i++) data[i] = (double)rank;
    
    double t_start = MPI_Wtime();
    
    // OpenMP parallel work: compute element-wise sin (memory bandwidth bound)
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ARRAY_SIZE; i++)
        data[i] = sin(data[i]) + cos(data[i]);
    
    // MPI ring: pass sum around all ranks
    double local_sum = 0.0;
    for (int i = 0; i < ARRAY_SIZE; i++) local_sum += data[i];
    
    double global_sum;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    double t_end = MPI_Wtime();
    
    if (rank == 0)
        printf("Elapsed: %.4f s | Global sum: %.4f\n", t_end - t_start, global_sum);
    
    free(data);
    MPI_Finalize();
    return 0;
}
```

```bash
# Build SIF on laptop, transfer to Leonardo
# Then compile inside the container on Leonardo:
salloc --nodes=1 --ntasks=1 --cpus-per-task=4 --mem=8G \
       --partition=dcgp_usr_prod --account=<your_account>

singularity exec $SCRATCH/containers/mpi_omp.sif \
    mpicc -fopenmp -O3 -o $SCRATCH/mpi_omp_ring $SCRATCH/mpi_omp_ring.c -lm
```

---

### Part 2 — Multi-Node MPI Job 

```bash
cat > $SCRATCH/mpi_omp_job.slurm << 'SLURM'
#!/bin/bash
#SBATCH --job-name=mpi_omp_container
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=7           # 56 cores / 8 ranks = 7 threads/rank
#SBATCH --mem=20G
#SBATCH --time=00:20:00
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=<your_account>
#SBATCH --output=mpi_omp.%j.out

module purge
module load hpcx-mpi/2.19

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close
export OMP_PLACES=cores

mpirun -np $SLURM_NTASKS \
       --map-by socket:PE=$SLURM_CPUS_PER_TASK \
       --bind-to core \
       singularity exec --env OPAL_PREFIX=/usr $SCRATCH/containers/mpi_omp.sif \
           $SCRATCH/mpi_omp_ring
SLURM

sbatch $SCRATCH/mpi_omp_job.slurm
```

**Questions**:
1. The `%environment` section sets `OMPI_MCA_btl=^openib` and `OMPI_MCA_pml=ucx`. What is the purpose of this? What communication path would MPI use without this setting on Leonardo?
2. Explain the `--map-by socket:PE=7` argument to mpirun. How does it place the 8 MPI ranks across the 2 sockets of a DCGP node?
3. Why is `MPI_Init_thread` called with `MPI_THREAD_FUNNELED` rather than `MPI_THREAD_MULTIPLE`? What constraint does this impose on OpenMP parallel regions?
4. Compare performance with 1, 2, and 4 nodes (adjust `--nodes` in the script). Report the scaling efficiency. Is communication overhead visible at 4 nodes?
5. Why is `--env OPAL_PREFIX=/usr` set in the singularity execution? 

---

### Part 3 - Advance MPI+OpenMP

Here we use the codes developed fot the OpenMP lectures to test an advanced use of hybrid computing.
The code is the PI calculation:

```c
/* ────────────────────────────────────────────────────────────────────────── *
 │                                                                            │
 │ This file is part of the exercises for the Lectures on                     │
 │   "Foundations of High Performance Computing"                              │
 │ given at                                                                   │
 │   Master in HPC and                                                        │
 │   Master in Data Science and Scientific Computing                          │
 │ @ SISSA, ICTP and University of Trieste                                    │
 │                                                                            │
 │ contact: luca.tornatore@inaf.it                                            │
 │                                                                            │
 │     This is free software; you can redistribute it and/or modify           │
 │     it under the terms of the GNU General Public License as published by   │
 │     the Free Software Foundation; either version 3 of the License, or      │
 │     (at your option) any later version.                                    │
 │     This code is distributed in the hope that it will be useful,           │
 │     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
 │     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
 │     GNU General Public License for more details.                           │
 │                                                                            │
 │     You should have received a copy of the GNU General Public License      │
 │     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
 │                                                                            │
 * ────────────────────────────────────────────────────────────────────────── */

#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif
#include <stdlib.h>     
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <mpi.h>

#define TCPU ({struct timespec ts; (clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ), (double)ts.tv_sec + \
				    (double)ts.tv_nsec * 1e-9);})

#define TtCPU ({struct timespec ts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &ts ), (double)ts.tv_sec + \
				    (double)ts.tv_nsec * 1e-9);})


double thtiming;
#pragma omp threadprivate(thtiming)

#define REQ_TAG 0
#define ANS_TAG 1
#define RES_TAG 2
#define BUNCH_SIZE 10000
#define BUNCH2_SIZE (BUNCH_SIZE*2)



int generate_points( double *array, int N, unsigned short seeds[3] )
{
  for ( int i = 0; i < N; i++ )
    array[i] = erand48( seeds );
  return 0;
}

int main( int argc, char **argv )
{
  /* ··································································
   +
   +  initialize the MPI framework
   +
   * ··································································· */
  
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
  if ( provided < MPI_THREAD_SINGLE )
    {
      // manage
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

  int Myrank, Ntasks;
  MPI_Comm myCOMM_WORLD;

  MPI_Comm_dup ( MPI_COMM_WORLD, &myCOMM_WORLD );
  MPI_Comm_size( myCOMM_WORLD, &Ntasks );    
  MPI_Comm_rank( myCOMM_WORLD, &Myrank );     


  /* ··································································
   +
   +  split the world in two
   +  Rank 0 will stay alone in a communicator, all the rest of tasks
   +  will participate in a second communicator
   +
   * ··································································· */

  MPI_Comm WorkWORLD;
  MPI_Comm_split( myCOMM_WORLD, (Myrank == 0), Myrank, &WorkWORLD );

  
  double AllWtime_start = MPI_Wtime();

  /* ··································································
   +
   +  specialize the execution:
   +  Rank 0 in myCOMM_WORLD will generate rnd numbers and answer to
   +         requests
   +  Ranks > 0 will process the rnd numbers and get the estimate of
   +           PI greek
   +
   * ··································································· */
  
  if ( Myrank == 0 )
    {

      //  Rank 0 will spawn omp threads
      //  th 0 will listen to MPI request and send data
      //  th > 0 will generate rnd values as soon as th 0
      //  has sent some around
      //
      //  rnd values will be placed in arrays, one per each th > 0
      //  a guardian value per array will store the information
      //  about those values being ready to be sent or in need
      //  of being discarded and re-generated
      //
      
     #define READY       1         // signals that a rnd value buffer is ready to be sent
     #define INVALID     0         // signals that a rnd value buffer has already been sent

      //
      //  set-up things
      //
      int nthreads, nworkers;
     #pragma omp parallel
     #pragma omp single
      nthreads = omp_get_num_threads();
      if ( nthreads == 1 )
	nthreads=2, omp_set_num_threads(nthreads);
      nworkers = nthreads - 1;
      
      int     stop = 0;
      double *points[nworkers];    // every worker thread will allocate room for data
      int     guardians[nworkers]; // guardian values
      
      for ( int i = 0; i < nworkers; i++ )
	guardians[i] = INVALID;

     #pragma omp parallel
      {
	int me = omp_get_thread_num();
	
	if ( me == 0 )
	  {
	    
	    int last = 0;
	    do
	      //
	      // a loop until a request with a stop value will be received
	      //
	      {

		// get a request
		int request;
		MPI_Status status;
		MPI_Recv( &request, 1, MPI_INT, MPI_ANY_SOURCE, REQ_TAG, myCOMM_WORLD, &status );
		if ( request != 0 )
		  {
		    // someone wants new data to process

		    // find the first buffer ready, starting from the
		    // most recently used
		    int idx = last;
		    int ready;
		    do
		      {
		       #pragma omp atomic read acquire
			ready = guardians[idx];		
			if ( !ready )
			  idx = ++idx % nworkers;
		      }  while ( !ready );
		    last = idx;

		    // a buffer has been found, we send it
		    MPI_Send( points[idx], BUNCH2_SIZE, MPI_DOUBLE, status.MPI_SOURCE, ANS_TAG, myCOMM_WORLD);
		    
		    // signal that that buffer has been sent and must be re-populated
		   #pragma omp atomic write release
		    guardians[idx] = INVALID;
		  }
		else
		  // signal that the worker tasks ended
		 #pragma omp atomic write release
		  stop = 1;
		
	      }
	    while( !stop );	    

	  }
	
	else	  
	  {
	    int _me = me-1;   // thread 0 is doing MPI, threads > 0 are doing the
			      // rnd number generation.
			      // hence, the indexing of points[] and guardians[]
			      // is from 0 to nworkers-1 and we need that every
			      // thread > 0 indexes its own by me-1

	    // allocate room for the rnd values
	    points[_me]   = (double*)malloc(sizeof(double)*BUNCH2_SIZE);
	    // save a local copy not to access the shared array
	    double *array = points[_me];

	    // initialize my own rnd seeds
	    unsigned short seeds[3] = {time(NULL)+me, time(NULL)+2*me, time(NULL)};
	    seed48( seeds );	    

	    // generate points
	    generate_points( array, BUNCH2_SIZE, seeds );
	    // signal that this buffer is ready
	   #pragma omp atomic write release
	    guardians[_me] = READY;	    

	    // now enter in a loop in which as soon as my
	    // buffer has been sent I will re-populate it
	    //
	    int mystop;
	    do
	      {
		// watch the guardian value
		//
		int myguard;
		do {
		 #pragma omp atomic read acquire
		  myguard = guardians[_me];
		 #pragma omp atomic read acquire
		  mystop = stop;		  
		} while ( (myguard != INVALID) && (!mystop) );


		if ( ! mystop ) {
		  // the game continues, generate new points
		  generate_points( array, BUNCH2_SIZE, seeds );
		  // signal that the buffer is populated again
		 #pragma omp atomic write release
		  guardians[_me] = READY; }

	      }
	    while( !mystop );

	    // free the memory
	    free ( points[_me] );
	  }
	
      }
      
      double pi;
      MPI_Recv( &pi, 1, MPI_DOUBLE, MPI_ANY_SOURCE, RES_TAG, myCOMM_WORLD, MPI_STATUS_IGNORE );
      printf("PI estimate is %12.10g\n", pi);
    }
  
  else
    //
    // Ranks > 0 will ask for points and process them
    // they will stop when the required convergence will be reached
    //
    {

      // get my rank in the workers' world
      int MyWrank, Nworkers;
      MPI_Comm_size( WorkWORLD, &Nworkers );    
      MPI_Comm_rank( WorkWORLD, &MyWrank );     

      double tolerance = (argc > 1 ? atof(*(argv+1)) : 1e-5);
       
      int                 round            = 0;   // how many round we run
      int                 converged        = 0;   // whether the estimate is good enough
      unsigned long long  inner_points     = 0;   // my inner points
      unsigned long long  all_inner_points = 0;   // all the inner points
      unsigned long long  total_points     = 0;   // all the processed points
      long double         prev_pi          = 0;   
      double             *points           = (double*)malloc( 2*BUNCH_SIZE * sizeof(double) );

     #pragma omp parallel
      thtiming = 0;
      
      while (!converged)
	{
	  int ask_for_points = 1;
	  MPI_Send( &ask_for_points, 1, MPI_INT, 0, REQ_TAG, myCOMM_WORLD);
	  MPI_Recv( points, 2*BUNCH_SIZE, MPI_DOUBLE, 0, ANS_TAG, myCOMM_WORLD, MPI_STATUS_IGNORE);

	  total_points += BUNCH_SIZE;
	  
	 #pragma omp parallel
	  {
	    double tstart = TtCPU;
	   #pragma omp for reduction(+:inner_points)
	    for ( int i = 0; i < BUNCH_SIZE; i++)
	      {
		int idx = i*2;
		double dist = points[idx]*points[idx] + points[idx+1]*points[idx+1];
		inner_points += (dist <= 1);
	      }

	    double tstop = TtCPU;
	    thtiming += tstop-tstart;
	  }

	  
	  MPI_Allreduce( &inner_points, &all_inner_points, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, WorkWORLD);
	  long double pi    = (long double)all_inner_points/ (total_points*Nworkers);
	  long double delta = fabsl((pi - prev_pi)/pi);
	  if ( delta < tolerance )
	    {
	      converged = 1;
	      if( MyWrank == 0 ) {
		
		double PI = (double)(4*pi);
		printf("\t >> round %d, pi estimate is %.12Lg, converged with delta %.12Lg\n", round, 4*pi, delta);
		ask_for_points = 0;
		MPI_Send( &ask_for_points, 1, MPI_INT, 0, REQ_TAG, myCOMM_WORLD );		
		MPI_Send( &PI, 1, MPI_DOUBLE, 0, RES_TAG, myCOMM_WORLD ); }

	      MPI_Barrier( WorkWORLD);
	     #pragma omp parallel
	      printf("\t** Worker %02d th %02d has taken %g sec\n", MyWrank, omp_get_thread_num(), thtiming);
	    }
	  else
	    {
	      round++;
	      prev_pi = pi;
	    }
	}

      free ( points );
    }


  double AllWtime_stop = MPI_Wtime();


  printf("Task %d has taken %g sec\n", Myrank, AllWtime_stop - AllWtime_start);
  
  MPI_Finalize();
  return 0;
}
```
We compile it using the `mpi_omp.sif` image.

```bash
export SIF=$SCRATCH/.singularity_cache/mpi_omp.sif
singularity exec --env OMP_PLACES=sockets $SIF mpicc -fopenmp -O3 -lm -o calculate_pi.thr calculate_pi.thr.c
```

Prepare the submission script:
```bash
cat > mpi_omp_pi.job << 'EOF'
#!/bin/bash
#SBATCH --job-name=mpi_omp_container
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=7           # 56 cores / 8 ranks = 7 threads/rank
#SBATCH --time=00:20:00
#SBATCH --mem=20gb
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=your_account
#SBATCH --output=mpi_omp.%j.out

module purge
module load hpcx-mpi/2.19
export SIF=/leonardo_scratch/large/userexternal/gtaffoni/.singularity_cache/mpi_omp.sif
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close
export OMP_PLACES=cores

mpirun -np $SLURM_NTASKS  --map-by socket:PE=$SLURM_CPUS_PER_TASK --bind-to core singularity exec  --env OPAL_PREFIX=/usr $SIF $SCRATCH/calculate_pi.thr
EOF
```


---

### Part 4 - GPU Passthrough: simple codes

We want to test the use of GPU on Leonardo using a container

```bash
# Build the container including CUDA toolkit
# Pull CUDA development image from NVIDIA NGC
singularity pull $SCRATCH/containers/cuda_12.2.sif \
    docker://nvcr.io/nvidia/cuda:12.2.0-devel-ubuntu22.04

# Request GPU node on Booster partition
salloc --nodes=1 --ntasks=1 --cpus-per-task=8 --gres=gpu:1 \
       --time=00:30:00 --partition=boost_usr_prod --account=<your_account>

export SIF=$SCRATCH/containers/cuda_12.2.sif
```
Execute a simple cuda code that gets the amunt of GPU deviceds available on the board and their properties:

```c++
cat > test_gpu_count.cu << 'EOF'
#include <stdio.h>
#include <cuda_runtime.h>
int main() {
    int n;
    cudaGetDeviceCount(&n);
    printf("Visible GPUs: %d\n", n);
    for (int i = 0; i < n; i++) {
        cudaDeviceProp p;
        cudaGetDeviceProperties(&p, i);
        printf("  Device %d: %s\n", i, p.name);
    }
    return 0;
}
EOF
```
The code must be compiled and executed

```bash
# Compile the code
singularity exec --nv $SIF nvcc -O3 -o test_gpu test_gpu_count.cu

# Run the program
singularity exec --nv $SIF ./test_gpu

Visible GPUs: 4
  Device 0: NVIDIA A100-SXM-64GB
  Device 1: NVIDIA A100-SXM-64GB
  Device 2: NVIDIA A100-SXM-64GB
  Device 3: NVIDIA A100-SXM-64GB
```
**Question** 
1. In the SLURM script for the Booster partition, `--gres=gpu:4` requests all 4 GPUs on a node. When a job script does not specify `CUDA_VISIBLE_DEVICES`, how does SLURM control which GPUs are visible inside the container?

### Part 5 — GPU Passthrough: SAXPY on A100

Write a minimal CUDA SAXPY benchmark:

```c++
// File: saxpy.cu
// SAXPY: y = a*x + y (single-precision)
#include <stdio.h>
#include <cuda_runtime.h>

#define N (1 << 27)   // 128M elements = 512 MB per array

__global__ void saxpy(int n, float a, float *x, float *y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = a * x[i] + y[i];
}

int main() {
    float *d_x, *d_y;
    cudaMalloc(&d_x, N * sizeof(float));
    cudaMalloc(&d_y, N * sizeof(float));
    cudaMemset(d_x, 1, N * sizeof(float));
    cudaMemset(d_y, 1, N * sizeof(float));

    // Warm-up
    saxpy<<<(N+255)/256, 256>>>(N, 2.0f, d_x, d_y);
    cudaDeviceSynchronize();

    // Timed run
    cudaEvent_t start, stop;
    cudaEventCreate(&start); cudaEventCreate(&stop);
    cudaEventRecord(start);
    for (int i = 0; i < 100; i++)
        saxpy<<<(N+255)/256, 256>>>(N, 2.0f, d_x, d_y);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    
    float ms;
    cudaEventElapsedTime(&ms, start, stop);
    double gbps = (3.0 * N * sizeof(float) * 100) / (ms * 1e-3) / 1e9;
    printf("SAXPY: N=%dM  time=%.2fms/iter  bandwidth=%.1f GB/s\n",
           N>>20, ms/100, gbps);
    
    cudaFree(d_x); cudaFree(d_y);
    return 0;
}
```

```bash
# Compile SAXPY inside the CUDA container
singularity exec --nv $SIF \
    nvcc -O3 -arch=sm_80 -o $SCRATCH/saxpy $SCRATCH/saxpy.cu

# Run and verify GPU access
singularity exec --nv $SIF nvidia-smi
singularity exec --nv $SIF $SCRATCH/saxpy
```

**Questions**:
1. Report the achieved memory bandwidth for SAXPY on one A100. The A100 SXM4 has a theoretical peak memory bandwidth of ~2 TB/s. What fraction of peak do you achieve?
2. What does the `-arch=sm_80` flag specify? What is the compute capability of the NVIDIA A100? (Answer: 8.0). Would a binary compiled with `-arch=sm_70` (Volta) run on an A100?
3. Run `singularity exec --nv cuda_12.2.sif nvidia-smi` and report the CUDA driver version and number of visible GPUs. Now run without `--nv`: what changes?

---

## Key Takeaways {#key-takeaways}

1. **Docker ≠ HPC containers**: Docker's root daemon, privilege model, and layered OCI format are incompatible with shared HPC systems, parallel filesystems, and high-speed interconnects. Apptainer was designed from scratch to address these constraints.

2. **SIF is the right format for Lustre**: One large compressed file performs orders of magnitude better than tens of thousands of small Docker layer files on parallel filesystems like Leonardo's Lustre.

3. **Your identity does not change**: The container process runs as you, not as root. Files created inside the container are owned by you. Privilege escalation is blocked.

4. **Build on laptop, run on cluster**: `sudo apptainer build` requires root — use your laptop or a VM. The resulting `.sif` file is architecture-specific (x86_64 for Leonardo) and transferable via `scp`.

5. **Host environment variables propagate**: `OMP_NUM_THREADS`, `OMP_PROC_BIND`, `CUDA_VISIBLE_DEVICES`, and all SLURM variables are visible inside the container without any special configuration.

6. **Use host MPI for multi-node jobs**: For InfiniBand-dependent parallel workloads, launch with the host `mpirun` (from `module load hpcx-mpi`) and wrap only the application binary with `singularity exec`. This gives full access to UCX/InfiniBand without requiring matching libraries inside the container.

7. **`--nv` injects driver libraries**: The container does not ship CUDA driver libraries. The `--nv` flag injects them from the host at runtime. The container's CUDA runtime version must be ≤ host driver's supported CUDA version.

8. **Store SIF in `$SCRATCH`, cache in `$SCRATCH/.singularity_cache`**: Never store SIF files or the Apptainer cache in `$HOME` on Leonardo — you will exhaust your home quota. Lustre scratch handles large files efficiently.

---

## References and Further Reading {#references}

### Official Documentation
- **Apptainer User Guide** (Linux Foundation): https://apptainer.org/docs/user/latest/
- **SingularityCE User Guide**: https://docs.sylabs.io/guides/latest/user-guide/
- **CINECA Singularity/Apptainer Documentation**: https://docs.hpc.cineca.it/services/singularity.html
- **Leonardo System Guide**: https://docs.hpc.cineca.it/hpc/leonardo.html
- **NVIDIA NGC Catalog**: https://catalog.ngc.nvidia.com/

### Key Papers
- Kurtzer, G.M., Sochat, V., Bauer, M.W. (2017). *Singularity: Scientific containers for mobility of compute*. PLOS ONE 12(5): e0177459. https://doi.org/10.1371/journal.pone.0177459
- Priedhorsky, R., Randles, T. (2017). *Charliecloud: Unprivileged Containers for User-Defined Software Stacks in HPC*. SC'17. (Useful comparison with alternative approach.)
- CINECA. (2024). *LEONARDO: A Pan-European Pre-Exascale Supercomputer for HPC and AI applications*. JLSRF, 8, A186. https://doi.org/10.17815/jlsrf-8-186

### Books and Courses
- *Building Containers for Scientific Computing* — CINECA HPC training materials: https://eventi.cineca.it/en/hpc/containerization-hpc
- Kleppmann, M. (2017). *Designing Data-Intensive Applications*. O'Reilly. (For distributed systems context.)
- HSF Training: *Introduction to Apptainer/Singularity*: https://hsf-training.github.io/hsf-training-singularity-webpage/

### Related Course Lectures
- **L04**: Docker fundamentals — image layers, OCI format, Dockerfiles
- **L05/L06**: Kubernetes — container orchestration (different use case: microservices vs HPC)
- **L09** (HPC-Cloud integration): Hybrid workflows between Leonardo and ADA Cloud

---

## Glossary {#glossary}

| Term | Definition |
|------|-----------|
| **Apptainer** | Open-source container runtime for HPC, successor to Singularity, maintained by the Linux Foundation. Provides the `apptainer` command and a `singularity` alias. |
| **Booster partition** | Leonardo's GPU-accelerated partition: 3456 nodes, each with 1 × Intel Ice Lake 32-core CPU + 4 × NVIDIA A100 SXM4 64GB GPUs. |
| **DCGP partition** | Leonardo's CPU-only Data Centric General Purpose partition: 1536 nodes, each with 2 × Intel Sapphire Rapids 56-core CPUs. |
| **Definition file (.def)** | Declarative specification for building a container image. Analogous to a Dockerfile. |
| **GFLOPS** | Giga (10⁹) Floating-Point Operations Per Second. Standard performance metric for compute kernels. |
| **HPC-X (hpcx-mpi)** | CINECA's recommended MPI module on Leonardo, based on OpenMPI with UCX support for InfiniBand HDR. |
| **InfiniBand HDR** | High Data Rate InfiniBand at 200 Gb/s per link. Leonardo's interconnect fabric (Dragonfly+ topology). |
| **Lustre** | Parallel distributed filesystem used on Leonardo for `$SCRATCH` and `$WORK` areas. Optimised for large files and sequential I/O. |
| **NUMA** | Non-Uniform Memory Access. In multi-socket systems, memory on each socket has different access latency depending on which socket the accessing CPU is on. |
| **`--nv` flag** | Apptainer/Singularity option that injects NVIDIA GPU driver libraries from the host into the container, enabling CUDA execution. |
| **OCI** | Open Container Initiative. Standard for container image format (used by Docker). Apptainer can import OCI images and convert them to SIF. |
| **`OMP_PROC_BIND`** | OpenMP environment variable controlling thread placement policy: `close` (near each other), `spread` (distributed). |
| **`OMP_PLACES`** | OpenMP environment variable defining the granularity of binding: `cores`, `threads`, `sockets`. |
| **PMI2 / PMIx** | Process Management Interface (version 2 / extended). Standard protocol between MPI and SLURM for process launch and management. |
| **Sandbox** | A writable directory (not a compressed SIF) used for interactive container development. Not suitable for production use on Lustre. |
| **SAXPY** | Single-precision A·X Plus Y: `y = a*x + y`. Standard memory-bandwidth benchmark for GPUs. |
| **SIF** | Singularity Image Format. A single compressed SquashFS file containing the container's complete user-space environment. Immutable by default. |
| **SingularityCE** | SingularityCE (Community Edition): open-source Singularity fork maintained by Sylabs. |
| **SquashFS** | Read-only compressed filesystem format used inside SIF files. Enables efficient single-file storage of container content. |
| **UCX** | Unified Communication X. High-performance communication framework used by OpenMPI to access InfiniBand, NVLink, and other transports. |

---

*Module: Apptainer and Singularity for HPC*  
*Target platform: Leonardo @ CINECA — Booster (A100 GPU) + DCGP (Intel Sapphire Rapids)*
