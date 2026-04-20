# Virtualization Fundamentals

##  Cloud Computing Course 2025/2026

| | |
|---|---|
| **Course** | HIGH PERFORMANCE AND CLOUD COMPUTING - Cloud Computing Module |
| **Lecture** | Virtualization Fundamentals |
| **Duration** | ~1 hour 45 minutes |
| **Prerequisites** | Foundations and Distributed Systems, Cloud Basics and Cloud Economics, basic OS and networking knowledge  |
| **Associated Exercise** | Exercise 03 — Virtualization for Scientific Computing (~2 hours) |

---

## Learning Objectives

By the end of this lecture, you will be able to:

- **Explain** why virtualization is the foundational technology of cloud computing and how it enables multi-tenancy, isolation, and elastic provisioning (Bloom: Understand)
- **Compare** Type 1 and Type 2 hypervisor architectures and justify why cloud providers exclusively deploy Type 1 hypervisors (Bloom: Analyze)
- **Trace** the evolution of CPU virtualization from binary translation through paravirtualization to hardware-assisted techniques, and articulate the trade-offs at each stage (Bloom: Analyze)
- **Describe** shadow page tables versus hardware-assisted memory virtualization (EPT/NPT) and evaluate their performance implications for memory-intensive scientific workloads (Bloom: Evaluate)
- **Categorize** I/O virtualization approaches (emulation, virtio, passthrough, SR-IOV) and select the appropriate strategy for a given HPC workload (Bloom: Evaluate)
- **Explain** how overlay networks (VXLAN) enable cloud VPCs and why this matters for multi-tenant isolation (Bloom: Understand)
- **Assess** the performance overhead of virtualization and identify tuning strategies that achieve 95–98% of bare-metal performance for scientific computing (Bloom: Evaluate)
- **Describe** how cloud providers (AWS Nitro, Firecracker) have evolved virtualization hardware to minimize overhead (Bloom: Understand)

---

## Table of Contents

1. [Introduction and Motivation](#1-introduction-and-motivation)
2. [Hypervisor Architectures](#2-hypervisor-architectures)
3. [CPU Virtualization](#3-cpu-virtualization)
4. [Memory Virtualization](#4-memory-virtualization)
5. [I/O Virtualization](#5-io-virtualization)
6. [Storage Virtualization](#6-storage-virtualization)
7. [Network Virtualization](#7-network-virtualization)
8. [Live Migration](#8-live-migration)
9. [Performance Considerations](#9-performance-considerations)
10. [Cloud Provider Implementations](#10-cloud-provider-implementations)
11. [VMs vs Containers: A Preview](#11-vms-vs-containers-a-preview)
12. [Key Takeaways](#12-key-takeaways)

---

## 1. Introduction and Motivation

### 1.1 Why Virtualization Matters

Virtualization is the foundational technology that makes cloud computing possible. Without it, the fundamental cloud promise — multiple independent tenants sharing the same physical infrastructure while maintaining complete isolation — would be unrealizable. Every time you launch an EC2 instance, spin up a Google Compute Engine VM, or create an Azure Virtual Machine, you are relying on decades of virtualization research and engineering.

Recall from Lecture 02 that cloud economics depend on **resource pooling** and **measured service** (two of the five NIST essential characteristics). Both require the ability to subdivide physical hardware into isolated, independently manageable units. Virtualization is the mechanism that delivers this capability.

**The core value proposition of virtualization** rests on four pillars:

- **Multi-tenancy**: Multiple customers share the same physical servers, each operating under the illusion of having dedicated resources. This sharing enables the economies of scale that drive cloud pricing below what most organizations could achieve on their own.
- **Isolation**: Workloads are protected from one another. A memory leak, kernel panic, or security breach in one virtual machine does not propagate to neighboring VMs on the same host. This isolation is not merely convenient — it is a fundamental security requirement for any multi-tenant platform.
- **Resource efficiency**: Without virtualization, the typical enterprise server sits at 10–15% average CPU utilization. The remaining 85–90% of purchased compute capacity is idle, yet still consuming power, cooling, and floor space. Virtualization enables consolidation ratios of 10:1 to 20:1, raising utilization to 60–80%.
- **Operational agility**: Provisioning a physical server historically took weeks to months (procurement, shipping, racking, cabling, OS installation). A virtual machine can be provisioned in seconds to minutes, with any operating system and any configuration, on demand.

### 1.2 The Problem Before Virtualization: Server Sprawl

Before virtualization became widespread in the early 2000s, enterprise data centers suffered from a pathology known as **server sprawl**. The standard practice was to dedicate one physical server per application, primarily for isolation and stability reasons. If an application crashed or had a memory leak, it would not affect other applications. However, this practice led to severe inefficiencies:

```
Traditional Data Center (Pre-Virtualization)
┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
│  App A   │  │  App B   │  │  App C   │  │  App D   │
│ ████░░░░ │  │ █░░░░░░░ │  │ ██░░░░░░ │  │ ███░░░░░ │
│  ~40%    │  │  ~5%     │  │  ~15%    │  │  ~20%    │
│ Physical │  │ Physical │  │ Physical │  │ Physical │
│ Server 1 │  │ Server 2 │  │ Server 3 │  │ Server 4 │
└──────────┘  └──────────┘  └──────────┘  └──────────┘
Average utilization: ~15%     85% of purchased capacity IDLE

Virtualized Data Center
┌────────────────────────────────────────────┐
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐       │
│  │ VM A │ │ VM B │ │ VM C │ │ VM D │       │
│  │ 40%  │ │  5%  │ │ 15%  │ │ 20%  │       │
│  └──────┘ └──────┘ └──────┘ └──────┘       │ 
│  ████████████████████████████░░░░░░░░░░░░  │
│  Combined ~80% utilization                 │
│  Physical Server 1 (single host)           │
└────────────────────────────────────────────┘
```

This consolidation directly translates to the cost savings discussed in Lecture 02: fewer physical machines means lower capital expenditure, less power consumption, less cooling, less floor space, and fewer human operators.

### 1.3 Definition

> **Virtualization** is the creation of an abstraction layer between physical hardware and the software running on it, enabling multiple isolated execution environments (virtual machines) to share the same underlying physical resources.

Each Virtual Machine (VM) operates under the illusion that it has exclusive access to CPU, memory, storage, and network resources. In reality, the **hypervisor** (or Virtual Machine Monitor, VMM) multiplexes physical resources across multiple VMs, enforcing isolation boundaries while maximizing utilization.

### 1.4 Historical Context

Virtualization is not a modern invention. It traces back to IBM's mainframe era, well before the personal computer revolution. Understanding this history illuminates a remarkable pattern: cloud computing essentially revived and scaled mainframe-era concepts for commodity x86 hardware.

| Year | Milestone | Significance |
|------|-----------|-------------|
| 1964 | IBM CP-40 | First implementation of virtual machines on mainframes |
| 1967 | IBM CP-67 | Production mainframe virtualization; introduced the term "hypervisor" |
| 1972 | IBM VM/370 | Commercial mainframe virtualization product; widely adopted |
| 1974 | Popek & Goldberg | Formal requirements for virtualizable architectures published |
| 1998 | VMware founded | First practical x86 virtualization using binary translation |
| 1999 | VMware Workstation | First commercial x86 virtualization product |
| 2001 | VMware ESX Server | First bare-metal x86 hypervisor for data centers |
| 2003 | Xen released | Open-source paravirtualization; adopted by early AWS |
| 2005 | Intel VT-x | Hardware virtualization extensions for x86 CPUs |
| 2006 | AMD-V (Pacifica) | AMD's hardware virtualization extensions |
| 2006 | AWS EC2 launch | First major public cloud; originally Xen-based |
| 2007 | KVM merged into Linux | Kernel-based Virtual Machine; open-source Type 1 hypervisor |
| 2008 | Intel EPT | Hardware-assisted memory virtualization (Extended Page Tables) |
| 2017 | AWS Nitro | Custom hardware offloading hypervisor functions |
| 2018 | AWS Firecracker | MicroVM technology for serverless (Lambda, Fargate) |

**Key insight for HPC practitioners**: The same virtualization technologies that power public clouds are increasingly used in HPC environments for hybrid cloud workflows, cloud bursting, and multi-tenant research clusters. Understanding their mechanics — and their overhead — is essential for making informed architectural decisions.

### 1.5 Why HPC Practitioners Need Virtualization Knowledge

If you come from a traditional HPC background, you may wonder why a high-performance computing programme covers virtualization. The answer is convergence: HPC and cloud are no longer separate worlds.

- **Cloud-based HPC**: AWS, Azure, and GCP all offer HPC-grade instances with InfiniBand (or equivalent) interconnects, bare-metal options, and GPU acceleration. Running MPI applications in the cloud requires understanding how virtualization affects latency, bandwidth, and NUMA topology.
- **Hybrid workflows**: Many research groups develop and test on cloud infrastructure, then run production jobs on dedicated clusters — or burst to the cloud when local capacity is exhausted. Understanding virtualization overhead informs these cost-performance trade-offs.
- **Multi-tenant clusters**: Even traditional HPC centers are adopting container and VM technologies for isolation between research groups, reproducibility of computational environments, and simplified software management.
- **The exam context**: Your practical exam requires building a complete cloud-based HPC environment. You cannot design or debug this infrastructure without understanding the virtualization layer beneath it.

---

## 2. Hypervisor Architectures

### 2.1 What is a Hypervisor?

A **hypervisor** (also called a **Virtual Machine Monitor**, or VMM) is the software — and increasingly hardware — layer responsible for creating, running, and managing virtual machines. It is the fundamental component that mediates between guest operating systems and physical hardware.

A hypervisor has four core responsibilities:

1. **CPU scheduling**: Allocating physical CPU time to virtual CPUs (vCPUs) across all running VMs. This is conceptually similar to OS process scheduling, but operates at a different level of abstraction — the hypervisor schedules entire virtual processors, each of which in turn runs its own OS scheduler.
2. **Memory management**: Maintaining the mapping between each VM's view of physical memory (Guest Physical Address, GPA) and the actual host memory (Host Physical Address, HPA). This involves additional levels of address translation beyond what a standard OS performs.
3. **I/O mediation**: Multiplexing access to physical devices (NICs, disks, GPUs) across multiple VMs. This is often the most performance-critical responsibility, as I/O operations can dominate virtualization overhead.
4. **Isolation enforcement**: Guaranteeing that no VM can access another VM's memory, intercept its network traffic, or interfere with its CPU execution. Isolation is not optional — it is the security foundation of multi-tenant cloud computing.

### 2.2 Key Terminology

| Term | Definition |
|------|------------|
| **Host** | The physical machine running the hypervisor |
| **Guest** | A virtual machine running on the host |
| **VMM** | Virtual Machine Monitor — synonym for hypervisor |
| **vCPU** | Virtual CPU — a CPU core as presented to the guest OS |
| **Dom0 / DomU** | Xen terminology: Dom0 is the privileged management domain; DomU is an unprivileged guest domain |
| **QEMU** | Quick Emulator — provides full-system emulation and serves as the device model for KVM |
| **Hypervisor call (Hypercall)** | An explicit function call from a guest OS to the hypervisor (analogous to a system call) |
| **VMCS / VMCB** | Virtual Machine Control Structure (Intel) / Virtual Machine Control Block (AMD) — stores guest/host state for fast transitions |

### 2.3 Type 1: Bare-Metal Hypervisor

A Type 1 hypervisor runs directly on the physical hardware, with no intervening host operating system. It has complete control over hardware resources and acts as a minimal, purpose-built operating system whose primary function is to host VMs.

```
Type 1 (Bare-Metal) Architecture

┌──────────┐  ┌───────────┐   ┌──────────┐
│  VM 1    │  │  VM 2     │   │  VM 3    │
│ (Linux)  │  │(Windows)  │   │ (Linux)  │
│ App+Libs │  │ App+Libs  │   │ App+Libs │
│ Guest OS │  │ Guest OS  │   │ Guest OS │
└────┬─────┘  └─────┬─────┘   └────┬─────┘
     └──────────────┼──────────────┘
              ┌─────┴──────┐
              │ Hypervisor │  ← Runs directly on hardware
              │  (Type 1)  │     No host OS beneath
              └─────┬──────┘
         ┌──────────┴───────────┐
         │   Physical Hardware  │
         │ CPU │ RAM │ NIC │GPU │
         └──────────────────────┘
```

**Characteristics**: Direct hardware access, minimal software stack, smaller attack surface, best performance. Requires a dedicated machine — you cannot casually "install" a Type 1 hypervisor alongside desktop applications.

**Examples**: VMware ESXi (commercial, dominant in enterprise), Microsoft Hyper-V (ships with Windows Server), Xen (open-source, historically used by early AWS), KVM (open-source, Linux-integrated).

### 2.4 Type 2: Hosted Hypervisor

A Type 2 hypervisor runs as an application within a conventional host operating system. It relies on the host OS for hardware access, device drivers, and resource management.

```
Type 2 (Hosted) Architecture

     ┌──────────┐   ┌──────────┐
     │  VM 1    │   │  VM 2    │
     │ (Linux)  │   │(Windows) │
     │ App+Libs │   │ App+Libs │
     │ Guest OS │   │ Guest OS │
     └────┬─────┘   └────┬─────┘
          └──────┬───────┘
           ┌─────┴──────┐
           │ Hypervisor │  ← Runs as application
           │  (Type 2)  │     on top of host OS
           └─────┬──────┘
           ┌─────┴──────┐
           │  Host OS   │  ← Full operating system
           │(Win/macOS/ │     (introduces overhead)
           │   Linux)   │
           └─────┬──────┘
         ┌───────┴──────────┐
         │ Physical Hardware│
         └──────────────────┘
```

**Characteristics**: Easy to install (like any application), broad hardware support (leverages host OS drivers), additional overhead from the host OS layer, good for development, testing, and education.

**Examples**: Oracle VirtualBox (free, cross-platform), VMware Workstation/Fusion (commercial), Parallels Desktop (macOS).

### 2.5 Type 1 vs Type 2 Comparison

| Dimension | Type 1 (Bare-Metal) | Type 2 (Hosted) |
|-----------|---------------------|-----------------|
| **Performance** | Better — direct hardware access, no host OS overhead | Overhead from host OS context switches and resource sharing |
| **Security** | Smaller attack surface; hypervisor code is minimal | Host OS vulnerabilities are inherited; larger attack surface |
| **Use case** | Data centers, production cloud, enterprise servers | Desktop development, testing, education, personal use |
| **Setup complexity** | Dedicated machine required; specialized installation | Install like any application on existing OS |
| **Hardware support** | Limited to hypervisor's own drivers | Broad — leverages host OS driver ecosystem |
| **Resource overhead** | Minimal (hypervisor is lightweight) | Host OS consumes CPU, memory, and disk resources |
| **Live migration** | Supported (standard feature) | Typically not supported |
| **Cloud relevance** | All major cloud providers use Type 1 | Not used in production cloud environments |

> **Cloud providers exclusively use Type 1 hypervisors** for production workloads. The performance, security, and operational benefits are not negotiable at cloud scale.

### 2.6 KVM: A Hybrid Approach

**KVM** (Kernel-based Virtual Machine) challenges the clean Type 1 / Type 2 distinction. Merged into the Linux kernel in 2007, KVM is implemented as a kernel module that turns the entire Linux kernel into a Type 1 hypervisor.

**How KVM works**:
1. The `kvm.ko` kernel module enables hardware virtualization extensions (VT-x/AMD-V)
2. Each VM runs as a regular Linux process (visible via `ps`, manageable via standard signals)
3. Each vCPU runs as a thread within that process
4. QEMU runs in userspace, providing device emulation and VM management
5. The Linux kernel provides CPU scheduling, memory management, and device drivers — all of which are mature, battle-tested subsystems

```
KVM/QEMU Architecture

┌──────────────────────────────────────────────┐
│  VM 1 (qemu process)  │  VM 2 (qemu process) │  Userspace
│  ┌─────────────────┐  │  ┌─────────────────┐ │
│  │ Guest OS + Apps │  │  │ Guest OS + Apps │ │
│  └────────┬────────┘  │  └────────┬────────┘ │
│  ┌────────┴────────┐  │  ┌────────┴────────┐ │
│  │  QEMU (devices) │  │  │  QEMU (devices) │ │
│  └────────┬────────┘  │  └────────┬────────┘ │
├───────────┼───────────┼───────────┼──────────┤
│           └──────┬────┼───────────┘          │
│           ┌──────┴────┴──────┐               │  Kernel
│           │   KVM Module     │               │
│           │  (VT-x/AMD-V)    │               │
│           └──────┬───────────┘               │
│           ┌──────┴───────────┐               │
│           │   Linux Kernel   │               │
│           │  (scheduling,    │               │
│           │   memory, I/O)   │               │
│           └──────┬───────────┘               │
├──────────────────┼───────────────────────────┤
│           ┌──────┴───────────┐               │
│           │ Physical Hardware│               │  Hardware
│           └──────────────────┘               │
└──────────────────────────────────────────────┘
```

**Why KVM dominates cloud infrastructure**: KVM inherits decades of Linux kernel development — scheduling algorithms, memory management, cgroups, namespaces, device drivers, security frameworks (SELinux, seccomp). Rather than reimplementing these capabilities in a custom hypervisor, KVM leverages them directly. This is why KVM/QEMU is the foundation of OpenStack, Google Compute Engine, Alibaba Cloud, and (via the Nitro evolution) AWS.

---

## 3. CPU Virtualization

### 3.1 The Challenge: x86 Privilege Rings

The x86 architecture organizes code execution into four privilege levels, called **rings**:

```
x86 Privilege Rings

        Ring 3 (Least privileged)
      ┌─────────────────────────┐
      │    User Applications    │
      │  ┌───────────────────┐  │
      │  │    Ring 2 (unused)│  │
      │  │  ┌─────────────┐  │  │
      │  │  │  Ring 1     │  │  │
      │  │  │  (unused)   │  │  │
      │  │  │ ┌─────────┐ │  │  │
      │  │  │ │ Ring 0  │ │  │  │
      │  │  │ │ (Kernel)│ │  │  │
      │  │  │ └─────────┘ │  │  │
      │  │  └─────────────┘  │  │
      │  └───────────────────┘  │
      └─────────────────────────┘
```

In a non-virtualized system, the OS kernel runs in Ring 0 with full hardware access, and user applications run in Ring 3 with restricted access. When an application attempts a privileged operation, the CPU generates a trap, transferring control to the kernel.

**The virtualization problem**: A guest OS expects to run in Ring 0, because it needs to execute privileged instructions (managing page tables, configuring interrupt controllers, accessing I/O ports). But the hypervisor already occupies Ring 0. If the guest runs in Ring 3 (or Ring 1), its privileged instructions either fail silently or return incorrect results.

### 3.2 The Popek-Goldberg Theorem (1974)

In 1974, Gerald Popek and Robert Goldberg published a seminal paper establishing the formal requirements for a virtualizable architecture. Their key requirement:

> All **sensitive instructions** — those that depend on or affect the configuration of system resources — must be a subset of **privileged instructions** (those that trap when executed in an unprivileged mode).

If this condition holds, the hypervisor can run guest code at reduced privilege and intercept all sensitive operations via traps. The hypervisor then emulates the intended behavior and returns control to the guest.

**The x86 problem**: The x86 instruction set architecture violates this requirement. There are **17 sensitive instructions** that do not trap when executed at reduced privilege — they either silently fail, return incorrect results, or behave differently than when executed in Ring 0. Examples include `SGDT`, `SIDT`, `SLDT` (which reveal the real descriptor table locations), and `POPF` (which silently ignores privilege flags when executed in Ring 3).

This violation meant that x86 was considered "non-virtualizable" by the classical Popek-Goldberg definition for decades. Three major engineering approaches were developed to overcome this limitation.

### 3.3 Solution 1: Binary Translation (VMware, 1998)

VMware's founders solved the x86 virtualization problem with a software technique called **binary translation**.

**How it works**:
1. The hypervisor intercepts guest kernel code before execution
2. A translator scans for sensitive or privileged instructions
3. Each problematic instruction is replaced with a safe equivalent sequence that produces the same guest-visible effect but routes through the hypervisor
4. Translated blocks are cached in a **Translation Cache** for reuse
5. User-mode (Ring 3) code runs natively — translation only applies to kernel code

**Example of translation**: The `SGDT` instruction (Store Global Descriptor Table Register) returns the address of the real GDT. The binary translator replaces it with code that returns the guest's virtual GDT address instead.

| Aspect | Binary Translation |
|--------|--------------------|
| **Overhead** | 5–20% for CPU-bound workloads |
| **Guest modification** | None required (runs unmodified guests) |
| **Complexity** | Very high (must handle all x86 corner cases) |
| **Self-modifying code** | Requires special handling |
| **Status today** | Legacy — replaced by hardware-assisted virtualization |

Binary translation was an extraordinary engineering achievement that made x86 virtualization commercially viable, but its overhead and complexity motivated the search for better solutions.

### 3.4 Solution 2: Paravirtualization (Xen, 2003)

The Xen project took a radically different approach: instead of working around the guest OS, **modify the guest to cooperate** with the hypervisor.

**How it works**:
1. The guest OS kernel is modified to replace all sensitive instructions with explicit **hypercalls** — direct calls to the hypervisor (analogous to system calls from userspace to the kernel)
2. The guest "knows" it is running inside a virtual machine and collaborates with the hypervisor
3. User-mode applications run unmodified

**Example**: Instead of directly writing to the CR3 register to update the page table base address, the paravirtualized guest calls `HYPERVISOR_mmu_update()`, which asks the hypervisor to perform the operation on its behalf.

| Aspect | Paravirtualization |
|--------|-------------------|
| **Overhead** | Minimal for CPU operations (hypercalls are efficient) |
| **Guest modification** | Required — guest kernel must be patched |
| **Proprietary OSes** | Cannot paravirtualize Windows (no source code access) |
| **Complexity** | Simpler hypervisor, but requires maintaining guest patches |
| **Status today** | CPU paravirtualization is largely obsolete; **I/O paravirtualization (virtio) remains dominant** |

**Historical note**: Early AWS EC2 (launched 2006) ran on Xen with paravirtualized Linux guests. This is why the earliest AMIs were "PV" (paravirtualized) instances. AWS later transitioned to HVM (Hardware Virtual Machine) instances as hardware-assisted virtualization matured.

### 3.5 Solution 3: Hardware-Assisted Virtualization (Intel VT-x / AMD-V, 2005–2006)

Intel and AMD independently solved the problem in hardware by adding virtualization extensions to the x86 instruction set.

**How it works**:
1. The CPU gains two new operating modes: **VMX root mode** (for the hypervisor) and **VMX non-root mode** (for guests)
2. Both modes have their own Ring 0 through Ring 3. The guest kernel runs in Ring 0 of non-root mode — it truly believes it is in Ring 0
3. All sensitive instructions are configured to automatically trap (cause a **VMEXIT**) when executed in non-root mode, transferring control to the hypervisor in root mode
4. A **VMCS** (Virtual Machine Control Structure) stores the complete guest and host state, enabling fast transitions
5. **VMENTER**: hypervisor loads guest state from VMCS and enters non-root mode
6. **VMEXIT**: CPU saves guest state to VMCS, loads host state, and enters root mode

```
Hardware-Assisted Virtualization: VMENTER / VMEXIT Cycle

┌────────────────────────┐            ┌────────────────────────┐
│     VMX Root Mode      │            │   VMX Non-Root Mode    │
│     (Hypervisor)       │            │      (Guest VM)        │
│                        │   VMENTER  │                        │
│  1. Configure VMCS     │ ─────────► │  3. Guest executes     │
│  2. Issue VMENTER      │            │     normally           │
│                        │            │  4. Sensitive instr.   │
│  6. Handle exit reason │ ◄───────── │     triggers VMEXIT    │
│  7. Update VMCS        │   VMEXIT   │                        │
│  8. Resume guest       │            │  5. CPU saves guest    │
│     (back to step 2)   │            │     state to VMCS      │
└────────────────────────┘            └────────────────────────┘
```

**Common VMEXIT triggers**: Privileged instructions (CPUID, MOV to CR3, RDMSR, WRMSR), I/O port access, memory-mapped I/O, external interrupts, certain exceptions, and explicitly configured intercepts.

### 3.6 VMEXIT: The Critical Performance Metric

Each VMEXIT costs approximately **500–1500 CPU cycles**. During this time, the CPU must:
1. Save the complete guest state (registers, control fields) to the VMCS
2. Load the host state from the VMCS
3. Begin executing hypervisor code (which then determines the exit reason, emulates the operation, and prepares to resume the guest)

For a modern processor running at 3 GHz, 1000 cycles represents approximately 333 nanoseconds — seemingly insignificant. But I/O-intensive workloads can trigger thousands of VMEXITs per second. At 10,000 VMEXITs/second with 1000 cycles each, that is 10 million cycles lost per second — approximately 0.3% overhead from VMEXITs alone, and this compounds with the associated cache and TLB pollution.

**Minimizing VMEXIT frequency is the single most important performance optimization in virtualization.** This principle drives the design of paravirtual I/O (virtio), hardware memory virtualization (EPT/NPT), and posted interrupts.

### 3.7 CPU Virtualization Evolution Summary

| Technique | Year | Overhead | Unmodified Guest? | Key Innovation | Status Today |
|-----------|------|----------|--------------------|----------------|--------------|
| Binary Translation | 1998 | 5–20% | Yes | Software rewriting of sensitive instructions | Legacy |
| Paravirtualization | 2003 | 2–5% (CPU) | No (modified kernel) | Guest cooperates via hypercalls | I/O only (virtio) |
| Hardware-Assisted | 2005–06 | 1–3% (CPU) | Yes | CPU hardware modes (VMX root/non-root) | **Primary method** |

**Modern best practice: Hardware-assisted CPU virtualization (VT-x/AMD-V) + paravirtualized I/O (virtio).** This combination leverages the best of both worlds — unmodified guest kernels with efficient I/O that minimizes VMEXITs.

---

## 4. Memory Virtualization

### 4.1 The Three-Level Translation Problem

In a non-virtualized system, the Memory Management Unit (MMU) performs a single address translation: Virtual Address → Physical Address. Virtualization introduces an additional layer:

```
Three-Level Address Translation

┌─────────────────────┐
│ Guest Virtual Addr. │  ← What guest user processes see
│       (GVA)         │
└─────────┬───────────┘
          │  Guest page tables (managed by guest OS)
          ▼
┌─────────────────────┐
│ Guest Physical Addr.│  ← What guest OS thinks is physical memory
│       (GPA)         │     (actually an abstraction)
└─────────┬───────────┘
          │  Hypervisor translation (shadow PT or EPT/NPT)
          ▼
┌─────────────────────┐
│ Host Physical Addr. │  ← Actual physical RAM addresses
│       (HPA)         │
└─────────────────────┘
```

The challenge is making this three-level translation **efficient**. A naive implementation would double the number of page table walks for every memory access — unacceptable for performance-sensitive workloads.

### 4.2 Solution 1: Shadow Page Tables

The original software approach, used by VMware and early KVM before hardware memory virtualization was available.

**How it works**:
1. The hypervisor maintains **shadow page tables** that map GVA directly to HPA (collapsing the two-step translation into one)
2. The guest's own page tables (GVA → GPA) are write-protected by the hypervisor
3. Whenever the guest attempts to modify its page tables, the write protection triggers a trap (VMEXIT)
4. The hypervisor intercepts this trap, reads the guest's intended modification, computes the corresponding GVA → HPA mapping, and updates the shadow page table
5. The hardware MMU uses only the shadow page tables — it never sees the guest's own page tables

**The cost**: Every guest page table modification causes a VMEXIT. For workloads that frequently modify page tables (process creation, context switches, memory mapping operations), this generates a high volume of VMEXITs. Benchmarks show shadow page table overhead of 10–30% for memory-management-intensive workloads.

### 4.3 Solution 2: EPT/NPT (Hardware-Assisted Memory Virtualization)

Intel **Extended Page Tables** (EPT, introduced 2008) and AMD **Nested Page Tables** (NPT, introduced 2008) solve this problem in hardware.

**How it works**:
1. The guest maintains its own page tables (GVA → GPA) normally, with no hypervisor intervention
2. The hypervisor maintains a second set of page tables (GPA → HPA) in the EPT/NPT structures
3. On every memory access, the hardware MMU performs a **two-dimensional page walk**: first resolving GVA → GPA using guest tables, then resolving GPA → HPA using EPT/NPT tables
4. The combined result (GVA → HPA) is cached in the **TLB** (Translation Lookaside Buffer) for fast subsequent lookups

**Key benefit**: No VMEXIT is required when the guest modifies its page tables. The hypervisor only needs to update EPT/NPT when the GPA → HPA mapping changes (e.g., during memory ballooning or page migration), which is far less frequent.

**Trade-off**: A 2D page walk visits more page table entries than a single-level walk. In the worst case, a 4-level guest walk combined with a 4-level EPT walk requires up to 24 memory accesses (each level of the guest walk must itself be translated through all EPT levels). However, TLB caching and hardware prefetching mitigate this in practice, and the elimination of shadow page table VMEXITs more than compensates.

| Approach | VMEXIT on PT Modification | Walk Depth | Implementation | Status |
|----------|---------------------------|------------|----------------|--------|
| Shadow PT | Yes (every modification) | Single (GVA→HPA) | Software (complex) | Legacy |
| EPT/NPT | No | 2D (up to 24 accesses) | Hardware | **Current standard** |

**Tagged TLBs and VPID/ASID**: Modern processors tag TLB entries with a Virtual Processor Identifier (Intel VPID) or Address Space Identifier (AMD ASID), eliminating the need to flush the TLB on every VMENTER/VMEXIT transition. This is critical for performance — TLB flushes cause a burst of expensive page table walks after every VM context switch.

### 4.4 Memory Overcommitment Techniques

**Memory overcommitment** means allocating more total virtual memory to VMs than the host physically has. While CPU and network can be overcommitted with relatively minor consequences (time-sharing), memory overcommitment is riskier because running out of physical memory causes severe performance degradation or out-of-memory kills.

| Technique | Mechanism | Performance Impact | Risk |
|-----------|-----------|-------------------|------|
| **Ballooning** | Guest balloon driver "inflates" (allocates memory) to signal unused pages back to the hypervisor | Low — guest OS makes intelligent decisions about which pages to release | Moderate — guest may balloon too aggressively |
| **KSM** (Kernel Same-page Merging) | Hypervisor scans for identical memory pages across VMs and deduplicates them (copy-on-write) | Low — background process; write triggers copy | Low — transparent to guests |
| **Swapping** | Hypervisor swaps guest pages to disk | **Severe** — orders of magnitude slower than RAM | High — should be last resort |
| **Compression** | Compress cold pages in memory before resorting to swapping | Moderate — CPU cost for compression/decompression | Low — buys time before swapping |

> **Critical cloud provider policy**: AWS, GCP, and Azure do **not** overcommit memory for customer VMs. When you purchase an `m5.xlarge` with 16 GiB of RAM, you are guaranteed 16 GiB of physical memory. This predictability is essential for performance-sensitive workloads, including scientific computing. Memory overcommitment is common in enterprise virtualization (VMware vSphere) but not in public cloud.

---

## 5. I/O Virtualization

### 5.1 The I/O Bottleneck

While CPU virtualization is largely a "solved problem" with hardware-assisted techniques achieving near-native performance, I/O virtualization remains the primary source of overhead in virtualized environments. The reasons are structural:

- Physical devices are designed for exclusive access; multiplexing them across VMs requires significant mediation
- I/O operations are frequent — a busy web server may process hundreds of thousands of network packets per second, each potentially triggering a VMEXIT
- DMA (Direct Memory Access) allows devices to write directly to system memory, creating security concerns if not properly isolated
- Interrupt delivery must be routed to the correct VM, adding routing overhead

Four approaches have been developed, forming a progression from maximum flexibility to maximum performance.

### 5.2 Approach 1: Full Device Emulation (QEMU)

QEMU's default mode — complete software simulation of physical devices.

**How it works**: The hypervisor presents a virtual device to the guest (e.g., an Intel e1000 Gigabit NIC). The guest uses its standard, unmodified driver for that device. Every I/O register read, DMA setup, and interrupt acknowledgment is intercepted by the hypervisor and handled in software.

**Advantages**: Maximum compatibility — any guest OS with a driver for the emulated device works without modification. Useful for legacy devices that are no longer physically available.

**Disadvantages**: Extremely slow. Every I/O operation causes a VMEXIT plus a context switch to the QEMU userspace process. Network throughput can drop to a small fraction of native speeds.

### 5.3 Approach 2: Paravirtualization (virtio)

**virtio** is the standard paravirtual I/O framework for KVM environments. Rather than emulating a physical device, virtio defines an efficient virtual device interface that both guest and host understand.

**How it works**:
1. The guest installs virtio drivers (included in the Linux kernel since 2.6.25; available for Windows)
2. Guest and hypervisor share memory regions containing ring buffers (**virtqueues**)
3. The guest places I/O requests in the ring buffer and signals the hypervisor (a single notification rather than per-operation traps)
4. The hypervisor processes batches of requests, reducing the number of VMEXITs
5. Common virtio devices: `virtio-net` (network), `virtio-blk` (block storage), `virtio-scsi` (SCSI), `virtio-gpu` (graphics)

**Performance**: Approximately 10× faster than full emulation for network I/O. With **vhost** (which moves the virtio backend processing from QEMU userspace into the kernel), performance improves further by eliminating user/kernel transitions.

### 5.4 Approach 3: Device Passthrough (Direct Assignment)

Assigns a physical device exclusively to a single VM, with the guest accessing hardware directly.

**How it works**:
1. The hypervisor uses the **IOMMU** (Intel VT-d / AMD-Vi) to remap the device's DMA transactions so they target only the assigned VM's memory
2. The guest accesses device registers directly — no hypervisor involvement in the data path
3. Interrupts from the device are routed directly to the guest (via interrupt remapping)

**Use cases in HPC**: GPU passthrough for CUDA/OpenCL workloads, NVMe passthrough for high-IOPS storage, InfiniBand HCA passthrough for MPI communication at native latency.

**Limitation**: A passthrough device can only be used by **one** VM — no sharing. This limits consolidation ratios.

### 5.5 Approach 4: SR-IOV (Single Root I/O Virtualization)

SR-IOV is a PCIe specification that enables a single physical device to present itself as multiple independent virtual devices, each assignable to a different VM.

**How it works**:
1. The physical device exposes a **Physical Function (PF)** — the full-featured device interface, managed by the hypervisor
2. The PF creates multiple **Virtual Functions (VFs)** — lightweight PCIe functions with their own configuration space, registers, and DMA channels
3. Each VF can be directly assigned to a VM via IOMMU
4. The hardware multiplexes traffic between VFs internally, with no hypervisor involvement in the data path

**Example**: An Intel X710 40GbE NIC can expose up to 128 VFs. Each VF provides near-native network performance to its assigned VM, while the hardware handles fair queuing and isolation.

**Cloud implementations**: AWS Elastic Network Adapter (ENA) uses SR-IOV to provide enhanced networking. AWS Elastic Fabric Adapter (EFA), used by HPC instances, extends this with OS-bypass capabilities for low-latency MPI communication. Azure Accelerated Networking similarly uses SR-IOV with Mellanox NICs.

### 5.6 IOMMU: The Security Foundation

The **IOMMU** (I/O Memory Management Unit), branded as Intel VT-d or AMD-Vi, is essential for safe device passthrough and SR-IOV.

**Why IOMMU is required**: DMA allows devices to read from and write to physical memory addresses directly, bypassing the CPU. Without the IOMMU, a device assigned to VM-A could be programmed to read VM-B's memory — a catastrophic isolation failure. The IOMMU provides:

- **DMA remapping**: Translates device DMA addresses so each device can only access the memory region assigned to its VM
- **Interrupt remapping**: Ensures device interrupts are delivered only to the intended VM, preventing interrupt-based attacks

### 5.7 I/O Virtualization Comparison

| Dimension | Full Emulation | virtio (Paravirt) | Passthrough | SR-IOV |
|-----------|---------------|-------------------|-------------|--------|
| **Latency** | Highest | Moderate | Near-native | Near-native |
| **Throughput** | Lowest | Good (~10× emulation) | Near-native | Near-native |
| **Guest changes** | None | virtio drivers required | None (uses native driver) | VF driver required |
| **Device sharing** | Yes (software) | Yes (software) | No (exclusive) | Yes (hardware) |
| **IOMMU required** | No | No | Yes | Yes |
| **Complexity** | Low | Low–Moderate | Moderate | High |
| **Live migration** | Easy | Easy | Difficult (device state) | Difficult (device state) |
| **HPC suitability** | Unsuitable | Acceptable | Excellent | **Excellent** |

> **HPC recommendation**: For latency-sensitive MPI workloads, SR-IOV or passthrough is mandatory. virtio is acceptable for throughput-oriented workloads where microsecond-level latency is not critical. Full emulation should never be used for performance-sensitive scientific computing.

---

## 6. Storage Virtualization

### 6.1 Virtual Disk Formats

VMs use virtual disk images — files or block devices that appear as physical disks to the guest OS.

| Format | Provider | Key Features | Performance |
|--------|----------|-------------|-------------|
| **QCOW2** | QEMU/KVM | Snapshots, compression, thin provisioning, encryption, backing files | Good (slight overhead from metadata) |
| **VMDK** | VMware | Snapshots, split files, streaming optimization | Good |
| **VHD/VHDX** | Microsoft | Dynamic expansion, differencing disks, 64TB max (VHDX) | Good |
| **RAW** | All | No metadata overhead, maximum performance, simplest format | **Best** |

**QCOW2** (QEMU Copy-On-Write version 2) is the standard for KVM-based clouds. Its backing file feature enables efficient cloning: a new VM image is created as a thin delta on top of a base image, storing only the blocks that differ.

### 6.2 Thin vs Thick Provisioning

| Aspect | Thick Provisioning | Thin Provisioning |
|--------|-------------------|-------------------|
| **Space allocation** | Immediate (100GB disk = 100GB allocated on host) | On-demand (grows as guest writes data) |
| **Performance** | More predictable (no allocation during I/O) | Potential fragmentation and allocation latency |
| **Space efficiency** | Lower (unused space is reserved) | Higher (only used space consumed) |
| **Overcommitment risk** | None | Possible — total virtual capacity can exceed physical storage |
| **Use case** | Performance-critical databases | General-purpose, development, cost optimization |

Cloud providers use thin provisioning by default — you pay for allocated capacity, but the underlying storage is provisioned dynamically. AWS EBS volumes are thin-provisioned at the infrastructure level.

### 6.3 Storage Backends

| Backend | Characteristics | Enables Live Migration? |
|---------|-----------------|------------------------|
| **Local disk** | Fastest I/O, but VM is tied to a specific physical host | No (unless storage migration used) |
| **NFS/NAS** | Shared storage over network; simple, widely supported | Yes |
| **iSCSI / Fibre Channel SAN** | Block storage over network; enterprise-grade performance | Yes |
| **Ceph / GlusterFS** | Distributed software-defined storage; scalable, replicated, self-healing | Yes |
| **Cloud block storage** | Network-attached, replicated, snapshotable (AWS EBS, Azure Managed Disks, GCP PD) | Yes (transparent) |

For HPC workloads requiring high-throughput parallel I/O (e.g., checkpoint/restart of large MPI simulations), cloud storage options include AWS FSx for Lustre, which provides a familiar parallel filesystem interface atop S3-backed storage.

---

## 7. Network Virtualization

### 7.1 The Network Virtualization Stack

Network virtualization creates a complete, isolated network environment for each tenant — including switches, routers, firewalls, and load balancers — entirely in software. This stack has five conceptual layers:

| Layer | Component | Function |
|-------|-----------|----------|
| 1 | **Virtual NIC (vNIC)** | Emulated or virtio network interface presented to the VM |
| 2 | **Virtual Switch (vSwitch)** | Software switch connecting VMs on the same host (Linux Bridge, Open vSwitch) |
| 3 | **VLAN** | Layer 2 isolation using IEEE 802.1Q tags (up to 4096 VLANs) |
| 4 | **Overlay Network** | Virtual networks tunneled over physical infrastructure (VXLAN, GENEVE, GRE) |
| 5 | **SDN Controller** | Centralized, programmable network management (OpenFlow, OVN) |

### 7.2 Virtual Switches: Linux Bridge vs Open vSwitch

**Linux Bridge**: Built into the kernel, simple and reliable, suitable for basic VM connectivity. Limited advanced features (no native VXLAN, no OpenFlow).

**Open vSwitch (OVS)**: Production-grade, programmable virtual switch. Supports OpenFlow for SDN, native tunneling protocols (VXLAN, GENEVE, GRE), QoS, port mirroring, and LACP. Used by OpenStack Neutron, Kubernetes networking plugins (OVN-Kubernetes), and many cloud platforms.

### 7.3 Overlay Networks: VXLAN

**VXLAN** (Virtual Extensible LAN, RFC 7348) is the dominant overlay network protocol in cloud environments and the foundation of virtually every cloud VPC implementation.

**The problem it solves**: Traditional VLANs are limited to 4096 networks (12-bit VLAN ID). A cloud provider with millions of tenants needs millions of isolated networks. Additionally, VLANs operate at Layer 2 and cannot span across Layer 3 boundaries (routers).

**How VXLAN works**:
1. Original Ethernet frames from VMs are encapsulated in UDP packets
2. A 24-bit **VXLAN Network Identifier (VNI)** allows up to ~16.7 million isolated virtual networks
3. **VTEP** (VXLAN Tunnel Endpoint) components at each host handle encapsulation/decapsulation
4. Encapsulated packets travel over the standard IP network (Layer 3), enabling virtual networks to span data centers, availability zones, or even regions

```
VXLAN Encapsulation

Original frame from VM:
┌────────┬────────────┬─────────┐
│Eth Hdr │  IP Header │ Payload │  ← VM's original L2 frame
└────────┴────────────┴─────────┘

After VXLAN encapsulation:
┌──────────┬──────────┬──────┬──────┬────────┬────────────┬─────────┐
│Outer Eth │Outer IP  │ UDP  │VXLAN │Eth Hdr │  IP Header │ Payload │
│  Header  │ Header   │(4789)│ VNI  │(orig.) │  (orig.)   │ (orig.) │
└──────────┴──────────┴──────┴──────┴────────┴────────────┴─────────┘
                                ↑
                         24-bit VNI
                    (16.7M networks)
```

> **Your AWS VPC is an overlay network.** When you create a VPC with its own CIDR block and subnets, AWS implements this as a VXLAN-like overlay on top of the physical data center network. Your traffic is completely isolated from other tenants, even though you share the same physical switches and routers.

---

## 8. Live Migration

### 8.1 Definition and Motivation

**Live migration** is the ability to move a running virtual machine from one physical host to another with minimal service interruption — typically 10–100 milliseconds of downtime, imperceptible to most applications.

This capability is essential for cloud operations: hardware maintenance (patching, firmware updates, replacements), load balancing (redistributing VMs for even host utilization), power management (consolidating VMs to fewer hosts during low-demand periods), and proactive failure avoidance (evacuating VMs from hosts showing early signs of hardware degradation).

### 8.2 Pre-Copy Migration (Dominant Approach)

The pre-copy algorithm is the most widely used live migration technique:

```
Pre-Copy Migration Timeline

Source Host                                    Destination Host
┌──────────────┐                              ┌───────────────┐
│ VM running   │ ──── Phase 1: Bulk ────────► │ Receiving     │
│ (continues   │      copy all pages          │ memory pages  │
│  executing)  │                              │               │
│              │ ──── Phase 2: Iterative ───► │ Receiving     │
│ VM dirtying  │      re-copy dirty pages     │ dirty pages   │
│ pages        │      (repeat N rounds)       │               │
│              │                              │               │
│ VM PAUSED    │ ──── Phase 3: Stop & Copy ─► │ Final dirty   │
│ (brief)      │      CPU state + last pages  │ pages + state │
│              │                              │               │
│              │                              │ VM RESUMED    │
│ VM stopped   │                              │ (running)     │
└──────────────┘                              └───────────────┘
```

**Phase details**:
1. **Initial bulk transfer**: Copy all of the VM's memory pages to the destination while the VM continues running on the source
2. **Iterative dirty-page tracking**: Pages modified ("dirtied") during the transfer are retransmitted. This phase repeats for several rounds, with each round typically transferring fewer pages as the dirty set converges
3. **Stop-and-copy**: When the dirty page rate is sufficiently low (or a maximum iteration count is reached), the VM is briefly paused. The remaining dirty pages and complete CPU state (registers, VMCS) are transferred
4. **Resume**: The VM resumes execution on the destination host. Network traffic is rerouted (via GARP or SDN updates)

### 8.3 The Convergence Problem

Pre-copy migration relies on the assumption that the dirty page rate will decrease across iterations, eventually reaching a point where the stop-and-copy phase is brief. For **write-intensive workloads** — such as in-memory databases, large-scale simulations writing checkpoints, or applications with high memory churn — the dirty rate may exceed the network transfer rate, preventing convergence.

**Solutions**: Page write throttling (slowing the VM's write rate), memory compression before transfer, RDMA-based transfer for higher bandwidth, or falling back to **post-copy migration** (start the VM immediately on the destination and fetch pages on demand — lower total migration time but higher risk if the source fails before all pages are transferred).

### 8.4 Bandwidth Calculation Example

Consider migrating a VM with 256 GiB of memory over a 25 Gbps network link:

- Bulk transfer time: 256 GiB / (25 Gbps / 8) ≈ 256 / 3.125 ≈ **82 seconds**
- If the VM dirties 10 GiB/s, each iterative round produces substantial dirty data
- With page compression (2:1 ratio), effective bandwidth doubles to ~50 Gbps equivalent
- Target downtime < 200 ms requires the final dirty set to be < (25 Gbps × 0.2 s) / 8 ≈ **625 MB**

**HPC consideration**: Live migration of MPI applications is particularly challenging because MPI collective operations have distributed state. If one rank is migrated while a collective is in progress, the communication pattern may be disrupted. Cloud providers typically do not guarantee migration transparency for tightly coupled parallel applications, which is why dedicated HPC instances (e.g., AWS `hpc6a`) run on dedicated hosts that are not subject to migration.

### 8.5 Requirements

| Requirement | Purpose |
|-------------|---------|
| Shared storage (or storage migration) | VM disk must be accessible from both source and destination |
| Compatible CPU family | Destination must support the same instruction set extensions |
| Network connectivity | Sufficient bandwidth between source and destination hosts |
| VXLAN/overlay networking | VM's network identity (IP, MAC) must be preserved after migration |

---

## 9. Performance Considerations

### 9.1 Sources of Virtualization Overhead

| Source | Cause | Typical Impact | Mitigation |
|--------|-------|---------------|------------|
| **VMEXITs** | Privileged instructions, I/O operations, interrupt delivery | 1–5% CPU overhead | Hardware-assisted virtualization, operation batching |
| **Memory translation** | Two-dimensional page walks (EPT/NPT) | 1–3% memory-bound overhead | Large/huge pages (2MB, 1GB), tagged TLBs (VPID/ASID) |
| **I/O mediation** | Device emulation, data copies between host and guest | 5–50% I/O overhead (varies by method) | virtio, SR-IOV, passthrough |
| **vCPU scheduling** | Contention between VMs for physical CPU time | Variable | CPU pinning, dedicated cores, cgroup isolation |
| **Interrupt routing** | Delivering physical interrupts to the correct VM | 1–2% | Posted interrupts (Intel APICv), MSI-X direct delivery |
| **Cache/TLB pollution** | VM context switches flush caches and TLBs | Variable, often underestimated | VPID/ASID tags, large pages, NUMA-aware placement |

### 9.2 Performance Ranges

| Configuration | Overhead vs Bare Metal |
|---------------|----------------------|
| Well-tuned VM (compute-bound, HW-assisted, pinned CPUs, huge pages) | **2–5%** |
| Typical cloud VM (shared resources, no pinning) | **5–10%** |
| I/O-intensive without SR-IOV (virtio only) | **10–20%** |
| Poorly configured (emulated I/O, no HW-assist, overcommitted) | **20%+** |

### 9.3 HPC Optimization Techniques

For scientific computing workloads requiring near-bare-metal performance, the following tuning strategies are essential:

- **CPU pinning**: Bind each vCPU to a specific physical core, eliminating scheduler migration and cache invalidation. On Linux/KVM, use `virsh vcpupin` or libvirt XML: `<vcpupin vcpu='0' cpuset='2'/>`.
- **NUMA-aware placement**: Ensure all vCPUs and memory of a VM reside within a single NUMA node. Cross-NUMA memory access incurs 1.5–2× latency penalty on modern Intel/AMD processors.
- **Huge pages**: Configure 2 MB or 1 GB pages to reduce TLB misses. A standard 4 KB page table covering 256 GiB of memory requires 67 million entries; 2 MB pages reduce this to 131,072 entries; 1 GB pages to just 256 entries.
- **SR-IOV / Passthrough**: Use hardware I/O virtualization for network and storage, eliminating software-mediated I/O overhead.
- **Disable unnecessary features**: Turn off memory ballooning, KSM, and power management (C-states, P-states) for latency-sensitive workloads.

**Result**: With these optimizations, HPC VMs routinely achieve **95–98% of bare-metal performance** on compute benchmarks (SPEC CPU, STREAM, HPL). AWS HPC instances (hpc6a, hpc7g) use EFA (Elastic Fabric Adapter) with OS-bypass capabilities to provide MPI latency comparable to on-premises InfiniBand.

---

## 10. Cloud Provider Implementations

### 10.1 AWS Nitro System

AWS developed the **Nitro System** to push virtualization overhead as close to zero as possible by offloading hypervisor functions to custom hardware.

**Architecture components**:
- **Nitro Cards**: Custom ASICs that handle VPC networking, EBS storage I/O, and instance storage — entirely in hardware, outside the host CPU
- **Nitro Security Chip**: Provides hardware root of trust, protecting firmware and preventing unauthorized access even by AWS operators
- **Nitro Hypervisor**: A minimal, KVM-based hypervisor with dramatically reduced functionality (networking and storage are handled by Nitro Cards, not the hypervisor)

**Impact**: By offloading I/O to dedicated hardware, the Nitro hypervisor consumes virtually zero host CPU resources. The host's full compute capacity is available to customer VMs. All modern AWS instance families (C5, M5, R5, and newer) are Nitro-based.

**For HPC practitioners**: Nitro-based instances provide near-bare-metal CPU performance by default. Combined with EFA for networking and NVMe for local storage, the virtualization tax is negligible for most scientific workloads.

### 10.2 AWS Firecracker

**Firecracker** is a microVM monitor developed by AWS and open-sourced in 2018. It powers AWS Lambda (serverless) and AWS Fargate (serverless containers).

| Metric | Firecracker MicroVM | Traditional VM (QEMU/KVM) |
|--------|--------------------|-----------------------------|
| Boot time | < 125 ms | 5–30 seconds |
| Memory overhead | < 5 MB per microVM | 30–100 MB per VM |
| Device model | Minimal (virtio-net, virtio-blk, serial, i8042) | Full (dozens of emulated devices) |
| Security model | Seccomp + jailer process isolation | Standard QEMU isolation |

**Design philosophy**: Security through minimality. By implementing only the devices needed for serverless workloads, Firecracker drastically reduces the attack surface compared to QEMU's hundreds of thousands of lines of device emulation code.

### 10.3 Other Cloud Providers

- **Google Compute Engine**: Custom-modified KVM with emphasis on live migration. Google famously live-migrates millions of VMs routinely for maintenance and security patching, often without any customer-visible impact.
- **Microsoft Azure**: Custom Hyper-V implementation with hardware acceleration for networking (Azure Accelerated Networking via SR-IOV with Mellanox NICs) and storage.
- **All major providers**: Have converged on the pattern of offloading virtualization functions to custom hardware, reflecting the industry consensus that software-only virtualization cannot achieve the performance density required at hyperscale.

### 10.4 How Cloud VMs Differ from On-Premises VMs

| Aspect | Cloud VMs | On-Premises VMs |
|--------|-----------|-----------------|
| **Performance predictability** | High (no memory overcommit, dedicated I/O hardware) | Variable (depends on configuration and overcommit policies) |
| **Noisy neighbor mitigation** | Hardware-level isolation (Nitro Cards, SR-IOV) | Software-level (scheduling, cgroups) |
| **Network performance** | Consistent (dedicated virtual NIC bandwidth) | Shared (software vSwitch contention) |
| **Storage performance** | Provisioned IOPS (EBS gp3: configurable) | Depends on shared storage backend |
| **Instance type selection** | Choose compute/memory/GPU ratio | Flexible VM sizing |

---

## 11. VMs vs Containers: A Preview

This section previews concepts that will be explored in depth in Lecture 04 (Containers and Docker).

| Dimension | Virtual Machines | Containers |
|-----------|------------------|------------|
| **Isolation level** | Hardware-level (separate kernel per VM) | Process-level (shared kernel, namespace/cgroup isolation) |
| **Overhead** | Full guest OS (GBs of memory, CPU for OS processes) | Shared kernel (MBs of overhead) |
| **Boot time** | Seconds to minutes | Milliseconds to seconds |
| **Image size** | Gigabytes (full OS + application) | Megabytes to hundreds of MBs (application + dependencies) |
| **Density** | Tens per host | Hundreds to thousands per host |
| **Security** | Strong (hypervisor isolation boundary) | Weaker (kernel attack surface shared) |
| **OS flexibility** | Any OS (Linux, Windows, FreeBSD) | Same kernel as host (Linux containers on Linux host) |
| **Use case** | Different OSes, strong isolation, untrusted workloads | Microservices, rapid scaling, CI/CD, developer environments |

**The modern pattern**: Containers running inside VMs — combining the strong isolation boundary of VMs (protecting tenants from each other) with the efficiency and density of containers (running many application instances within a single VM). Every major cloud provider uses this hybrid approach: AWS ECS and EKS run containers on EC2 VMs; Fargate runs containers inside Firecracker microVMs; GKE runs containers inside Compute Engine VMs.

**Bridge to Lecture 04**: In the next lecture, we will explore how containers achieve process-level isolation using Linux kernel primitives — namespaces (what a process can *see*) and cgroups (what a process can *use*) — without requiring a hypervisor or a separate kernel. You will see that containers are not "lightweight VMs" — they are a fundamentally different abstraction.

---

## 12. Key Takeaways

1. **Virtualization is the foundation of cloud computing**, enabling multi-tenancy, isolation, and elastic provisioning. Without it, the cloud economic model would not be viable.

2. **Type 1 (bare-metal) hypervisors** are used exclusively in cloud production environments. KVM/QEMU, as a Linux-integrated Type 1 hypervisor, dominates the open-source cloud ecosystem.

3. **CPU virtualization evolved through three stages**: binary translation (software, 5–20% overhead) → paravirtualization (guest modifications, minimal CPU overhead) → hardware-assisted (VT-x/AMD-V, near-native). Modern systems combine hardware-assisted CPU virtualization with paravirtualized I/O.

4. **VMEXIT minimization is the central performance optimization**. Each VMEXIT costs 500–1500 cycles; all major virtualization improvements (EPT/NPT, virtio, posted interrupts) exist to reduce VMEXIT frequency.

5. **I/O virtualization progresses from emulation → virtio → SR-IOV → passthrough**, trading flexibility for performance. HPC workloads require SR-IOV or passthrough for acceptable latency.

6. **EPT/NPT eliminate shadow page table overhead** by performing two-dimensional page walks in hardware, transforming memory virtualization from a major bottleneck into a near-transparent operation.

7. **Live migration** enables zero-downtime maintenance but faces convergence challenges with write-intensive workloads. Pre-copy is the dominant algorithm.

8. **Cloud providers have invested in custom hardware** (AWS Nitro, Google's custom KVM, Azure's custom Hyper-V) to offload virtualization overhead, achieving near-bare-metal performance for customer workloads.

9. **With proper tuning** (CPU pinning, NUMA-aware placement, huge pages, SR-IOV), HPC VMs achieve **95–98% of bare-metal performance** — making cloud a viable platform for scientific computing.

---

## References

### Academic Papers
- Popek, G. J., & Goldberg, R. P. (1974). "Formal Requirements for Virtualizable Third Generation Architectures." *Communications of the ACM*, 17(7), 412–421.
- Barham, P., Dragovic, B., Fraser, K., et al. (2003). "Xen and the Art of Virtualization." *Proceedings of the 19th ACM Symposium on Operating Systems Principles (SOSP)*.
- Kivity, A., Kamay, Y., Laor, D., Lublin, U., & Liguori, A. (2007). "KVM: the Linux Virtual Machine Monitor." *Proceedings of the Linux Symposium (OLS)*.
- Agache, A., Brooker, M., Iordache, A., et al. (2020). "Firecracker: Lightweight Virtualization for Serverless Applications." *USENIX NSDI*.
- Ben-Yehuda, M., Mason, J., Krieger, O., et al. (2006). "Utilizing IOMMUs for Virtualization in Linux and Xen." *Proceedings of the Linux Symposium (OLS)*.

### Industry Documentation
- Intel Corporation. "Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3C: System Programming Guide, Part 3" (VMX chapters).
- Intel Corporation. "Intel Virtualization Technology for Directed I/O Architecture Specification."
- AWS. "The Nitro System." https://aws.amazon.com/ec2/nitro/
- AWS. "Firecracker." https://firecracker-microvm.github.io/
- PCI-SIG. "Single Root I/O Virtualization and Sharing Specification."
- IETF RFC 7348. "Virtual eXtensible Local Area Network (VXLAN)."

### Books
- Tanenbaum, A. S., & Bos, H. (2014). *Modern Operating Systems* (4th ed.). Pearson. (Chapters on virtualization)
- Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). *Operating System Concepts* (10th ed.). Wiley. (Chapter 18: Virtual Machines)
- VMware. "vSphere Resource Management Guide." https://docs.vmware.com/

---

## Glossary

| Term | Definition |
|------|------------|
| **EPT** | Extended Page Tables — Intel's hardware-assisted memory virtualization, performing GPA → HPA translation in hardware |
| **GPA** | Guest Physical Address — the memory address the guest OS believes is physical |
| **GVA** | Guest Virtual Address — the virtual address used by processes inside the guest |
| **HPA** | Host Physical Address — the actual physical memory address on the host machine |
| **Hypercall** | An explicit call from a guest OS to the hypervisor, analogous to a system call |
| **IOMMU** | I/O Memory Management Unit — provides DMA and interrupt remapping for safe device passthrough |
| **KSM** | Kernel Same-page Merging — Linux feature that deduplicates identical memory pages across VMs |
| **KVM** | Kernel-based Virtual Machine — a Linux kernel module that turns Linux into a Type 1 hypervisor |
| **NPT** | Nested Page Tables — AMD's hardware-assisted memory virtualization (equivalent to Intel EPT) |
| **QEMU** | Quick Emulator — full-system emulator that provides device models for KVM; also usable standalone |
| **SR-IOV** | Single Root I/O Virtualization — PCIe specification enabling hardware-level device sharing via VFs |
| **virtio** | A standard paravirtual I/O framework for efficient guest-host communication using shared ring buffers |
| **VMCS** | Virtual Machine Control Structure — Intel data structure storing guest/host state for fast VMENTER/VMEXIT |
| **VMEXIT** | Transition from VMX non-root mode (guest) to VMX root mode (hypervisor); triggered by sensitive operations |
| **VMM** | Virtual Machine Monitor — the component that manages VM execution; synonym for hypervisor |
| **VPID** | Virtual Processor Identifier — CPU tag for TLB entries, eliminating TLB flushes on VM context switches |
| **VT-d** | Intel Virtualization Technology for Directed I/O — Intel's IOMMU implementation |
| **VT-x** | Intel Virtualization Technology — CPU extensions providing VMX root/non-root modes for hardware-assisted virtualization |
| **VTEP** | VXLAN Tunnel Endpoint — the component that encapsulates/decapsulates VXLAN packets |
| **VXLAN** | Virtual Extensible LAN — overlay network protocol using UDP encapsulation and 24-bit VNIs |

---

*Document Version: 1.0*  
*Course:  HIGH PERFORMANCE AND CLOUD COMPUTING  2025/2026*  
*Applied Data Science & Artificial Intelligence - University of Trieste*
