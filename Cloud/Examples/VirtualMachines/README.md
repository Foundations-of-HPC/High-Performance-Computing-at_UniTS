# Virtualization Tutorials

## Overview
This repository contains tutorials for setting up and managing virtualized environments for cloud computing. The tutorials focus on deploying interconnected virtual machines (VMs) that work together in different configurations, including high availability clusters, load balancing clusters, and an HPC cluster using SLURM.

## Tutorials

### **SLURM-Based HPC Cluster in a Box**
This tutorial provides a step-by-step guide to deploying an **HPC Cluster using SLURM** within a virtualized environment.

#### **Steps:**
1. **Create Virtual Machines**
   - Deploy a **head node** and multiple **compute nodes**.
   - Assign proper network configurations.

2. **Install SLURM and Dependencies**
   - Install SLURM workload manager on all nodes.
   - Configure the SLURM controller and compute nodes.

3. **Cluster Networking & Resource Management**
   - Set up SSH key-based authentication.
   - Configure SLURM daemon services.

4. **Run Test Jobs**
   - Submit test jobs to validate SLURM scheduling.
   - Monitor job execution and performance.

## Prerequisites
- A Linux-based virtualization host.
- VirtualBox,  or another hypervisor installed.
- Basic knowledge of Linux command line and networking.

**Detailed tutorial:** [Tutorial  - Slurm Cluster](Slurm.md)

## Contributions
Contributions and improvements are welcome! Please submit pull requests or open an issue to discuss changes.

## License
This project is licensed under the MIT License.