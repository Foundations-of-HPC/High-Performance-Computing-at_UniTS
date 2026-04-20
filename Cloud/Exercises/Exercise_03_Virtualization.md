# Exercise 03: Virtualization for Scientific Computing

##  Cloud Computing Course 2024/2025

---

## Exercise Information

| | |
|---|---|
| **Associated Lecture** | Lecture 03 - Virtualization Fundamentals |
| **Type** | Analytical / Calculation + Design |

---

## Learning Objectives

1. Analyze virtualization overhead for HPC workloads
2. Select appropriate I/O virtualization strategies
3. Configure NUMA-aware VM placement
4. Understand live migration impact on scientific applications
5. Make informed decisions between virtualized and bare-metal environments

---

##  Virtualization Overhead Analysis 

### Scenario

You are evaluating whether to virtualize a computational fluid dynamics (CFD) workload that currently runs on bare metal. The workload has these characteristics:
- 64 CPU cores
- 256 GB memory
- Heavy floating-point computation
- Moderate disk I/O (checkpoint every 30 minutes)
- MPI communication between processes (latency-sensitive)

### Tasks

**1.1** Calculate the expected overhead for each virtualization layer:
- CPU virtualization (with VT-x/AMD-V)
- Memory virtualization (with EPT/NPT)
- Network virtualization (with virtio-net)
- Storage virtualization (with virtio-blk)


##  NUMA-Aware VM Configuration 

### Scenario

You have a 2-socket server with:
- 2 × 64-core AMD EPYC processors
- 512 GB RAM (256 GB per NUMA node)
- 4 × NVIDIA A100 GPUs (connected to NUMA node 0)
- Running KVM/QEMU

Server layout:
- NUMA 0: CPUs 0-63, Memory 0-255GB, GPUs
- NUMA 1: CPUs 64-127, Memory 256-511GB

### Tasks

**3.1** Design the CPU pinning strategy for these VMs:

| VM | vCPUs | Memory | GPU Required | NUMA Placement |
|----|-------|--------|--------------|----------------|
| CFD-1 | 64 | 256 GB | No | ? |
| ML-1 | 32 | 128 GB | 2 × A100 | ? |
| Analysis-1 | 16 | 64 GB | No | ? |
| Analysis-2 | 16 | 64 GB | No | ? |

Provide specific CPU ranges and memory allocations.

---

## Live Migration Impact 

### Scenario

A 2 TB memory VM running molecular dynamics simulation needs to be migrated for hardware maintenance. The simulation uses MPI across 4 VMs and checkpoints every 30 minutes.

### Tasks

**4.1** Calculate the minimum network bandwidth required for live migration with:
- Maximum acceptable downtime: 500 ms
- Memory dirty rate: 1 GB/s
- Target total migration time: < 5 minutes



---

##  Virtualization vs Bare Metal Decision 

### Scenario

A research computing center must decide between:
- Option A: 200 bare-metal nodes with Slurm
- Option B: Cloud VMs with Kubernetes
- Option C: Hybrid approach

### Tasks

**5.1** List three scenarios where bare metal is clearly superior, with quantitative justification.

**5.2** List three scenarios where cloud VMs are clearly superior, with quantitative justification.


---

*Exercise 03 - Virtualization for Scientific Computing*
*Course:  HIGH PERFORMANCE AND CLOUD COMPUTING  2025/2026*  
*Applied Data Science & Artificial Intelligence - University of Trieste*
