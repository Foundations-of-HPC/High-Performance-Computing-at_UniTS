# Exercise 02: Cloud Economics Analysis for Scientific Computing

## MHPC Cloud Computing Course 2024/2025

---

## Exercise Information

| | |
|---|---|
| **Associated Lecture** | Lecture 02 - Cloud Service Models and Economics |
| **Type** | Analytical / Calculation |

---

## Learning Objectives

Through this exercise, you will:

1. Apply service model selection criteria to scientific computing scenarios
2. Calculate and compare Total Cost of Ownership (TCO) for cloud vs on-premise HPC
3. Analyze cloud pricing models for research workloads
4. Design a FinOps strategy for a research institution
5. Evaluate deployment models for sensitive research data

---

## Part 1: Service Model Selection for Scientific Workloads 

### Scenario 1.1: Climate Modeling Simulation

A climate research group runs the Community Earth System Model (CESM) for long-term climate projections. Requirements:
- MPI-based parallel code requiring low-latency interconnects
- Simulations run for 2-4 weeks continuously
- Custom-compiled libraries (NetCDF, PnetCDF, ESMF)
- Specific compiler optimizations (Intel Fortran with particular flags)
- Output: 50TB per simulation run
- Need to run on 500-2000 cores depending on resolution

**Questions:**
a) Which service model would you recommend? Why?
b) What are the specific technical requirements that drive your choice?
c) How would you handle the large data output cost-effectively?

---

### Scenario 1.2: Genomics Pipeline

A bioinformatics lab processes DNA sequencing data through a standard pipeline:
- BWA-MEM2 for alignment
- GATK for variant calling
- Annotation with VEP

Characteristics:
- Each sample takes 4-8 hours to process
- 10-50 samples per week (variable based on sequencing runs)
- Pipeline is containerized (Docker)
- Intermediate files: 200GB per sample
- Final results: 5GB per sample

**Questions:**
a) Which service model would you recommend? Why?
b) Would you recommend the same model for all pipeline stages? Explain.
c) How would spot/preemptible instances fit into this workflow?


---

### Scenario 1.3: Collaborative Research Platform

A multi-institution physics collaboration (50 researchers across 10 universities) needs:
- Shared Jupyter notebooks for analysis
- Common datasets (500TB, read-heavy)
- Version-controlled analysis code
- Reproducible computational environments
- Interactive visualization of detector data

**Questions:**
a) Which service model(s) would you recommend?
b) How would you balance ease-of-use against customization needs?
c) What existing SaaS/PaaS scientific platforms might fit this use case?

---

## Part 2: Total Cost of Ownership - HPC Cluster Analysis 

### Scenario

The Computational Physics Department is deciding whether to purchase a new on-premise HPC cluster or use cloud resources. They need to perform a 5-year TCO analysis.

### Current Workload Profile

| Workload Type | Core-Hours/Year | Characteristics |
|---------------|-----------------|-----------------|
| CFD simulations | 2,000,000 | Tightly coupled MPI, needs fast interconnect |
| Monte Carlo | 1,500,000 | Embarrassingly parallel, fault-tolerant |
| Machine Learning | 500,000 | GPU-intensive, can use spot instances |
| Interactive analysis | 200,000 | Low-latency requirement |

**Total: 4,200,000 core-hours/year**

### Option A: On-Premise HPC Cluster

Proposed configuration:
- 50 compute nodes: Dual AMD EPYC 7763 (128 cores/node), 512GB RAM
- 4 GPU nodes: 4× NVIDIA A100 each
- 200TB parallel filesystem (Lustre)
- HDR InfiniBand interconnect
- Head nodes, login nodes, management

| Category | Cost |
|----------|------|
| Compute nodes (50 × $25,000) | $1,250,000 |
| GPU nodes (4 × $150,000) | $600,000 |
| Storage system | $400,000 |
| InfiniBand network | $200,000 |
| Head/login/management nodes | $100,000 |
| Rack, PDUs, cabling | $50,000 |
| **Total Hardware** | **$2,600,000** |

Annual operating costs:

| Category | Annual Cost |
|----------|-------------|
| Power (250kW × $0.10/kWh × 8760h × 0.7 utilization) | $153,300 |
| Cooling (40% of power) | $61,320 |
| Data center space | $36,000 |
| System administration (1.5 FTE × $90,000) | $135,000 |
| Hardware maintenance (5% of hardware) | $130,000 |
| Software licenses (scheduler, compilers, libraries) | $50,000 |
| Network connectivity | $24,000 |
| **Total Annual Operating** | **$589,620** |

### Option B: Cloud HPC (AWS)

Proposed equivalent:
- Compute: hpc6a.48xlarge (96 cores, 384GB) for MPI workloads
- GPU: p4d.24xlarge (8× A100) for ML workloads
- Storage: FSx for Lustre
- Interconnect: Elastic Fabric Adapter (EFA)

AWS Pricing (US-East-1):

| Resource | On-Demand/hour | Spot (avg) | 1-Year Savings Plan |
|----------|----------------|------------|---------------------|
| hpc6a.48xlarge | $2.88 | $0.86 | $1.82 |
| p4d.24xlarge | $32.77 | $9.83 | $20.73 |
| c6i.32xlarge (general) | $5.44 | $1.63 | $3.44 |

Storage:
- FSx for Lustre: $0.145/GB-month (persistent)
- S3: $0.023/GB-month
- Data transfer out: $0.09/GB (first 10TB), $0.085/GB (next 40TB)

Additional costs:
- AWS support (Business): 10% of compute spend
- Cloud engineer (0.5 FTE): $50,000/year

### Questions

**2.1** Calculate 5-Year On-Premise TCO 

**2.2** Calculate 5-Year Cloud TCO with optimized pricing strategy 

**2.3** Qualitative Factors: Beyond cost, what are the advantages/disadvantages of each approach?

---

## Part 3: Pricing Optimization for Research Computing 

### Scenario

You manage cloud resources for a computational biology center with the following monthly workloads:

| Workload | Instance Type | Hours/Month | Characteristics |
|----------|---------------|-------------|-----------------|
| AlphaFold predictions | p3.8xlarge × 2 | 400 | Can checkpoint every hour |
| Molecular dynamics (GROMACS) | hpc6a.48xlarge × 20 | 600 | MPI, can restart from checkpoint |
| Sequence alignment | c6i.8xlarge × 10 | 300 | Embarrassingly parallel |
| Database server | r6i.2xlarge × 1 | 730 (24/7) | Must be always available |
| Interactive Jupyter | t3.xlarge × 5 | 400 | Researchers need on-demand access |
| Backup/archive jobs | c6i.xlarge × 4 | 100 | Runs nightly, very flexible |

### Pricing Reference

| Instance | On-Demand/hr | 1-Year SP | Spot (avg) |
|----------|--------------|-----------|------------|
| p3.8xlarge | $12.24 | $7.73 | $3.67 |
| hpc6a.48xlarge | $2.88 | $1.82 | $0.86 |
| c6i.8xlarge | $1.36 | $0.86 | $0.41 |
| r6i.2xlarge | $0.504 | $0.32 | $0.15 |
| t3.xlarge | $0.166 | $0.105 | $0.05 |
| c6i.xlarge | $0.17 | $0.107 | $0.05 |

### Questions

**3.1** Calculate baseline monthly cost (all on-demand)

**3.2** Design optimized pricing strategy for each workload with justification

**3.3** Calculate total savings (monthly, percentage, annual)

---

*Exercise Version: 1.0*
*Course:  HIGH PERFORMANCE AND CLOUD COMPUTING  2025/2026*  
*Applied Data Science & Artificial Intelligence - University of Trieste*
