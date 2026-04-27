# Lecture 03 — Virtualization

| | |
|---|---|
| **Course** | HIGH PERFORMANCE AND CLOUD COMPUTING - Cloud Computing Module |
| **Lecture** | Virtualization Fundamentals |
| **Duration** | ~1 hour 45 minutes |
| **Prerequisites** | Foundations and Distributed Systems, Cloud Basics and Cloud Economics, basic OS and networking knowledge  |
| **Associated Exercise** | Exercise 03 — Virtualization for Scientific Computing (~2 hours) |


---

# Part I — Theory: from Popek–Goldberg to modern VMMs

## §1. Motivation — why virtualization matters

In the late 1990s, the dominant pattern of server deployment in corporate IT was *one application per physical machine*. A mail server ran on a box, a database on another, a web frontend on a third. This pattern was not chosen for technical reasons; it was a consequence of the reliability culture of the time. Running two services on the same machine meant that a failure in one could, through shared resources, memory pressure, or operator error, bring down the other. The remedy was isolation by hardware.

The consequences of this pattern were substantial. Studies from IDC and Gartner estimated that the average x86 server of the early 2000s sustained a CPU utilisation of 10–15% [(Barroso & Hölzle, 2007)](#ref-barroso-2007). The other 85–90% was paid for — in capital expenditure, in floor space, in power and cooling — but went unused. Data centres were simultaneously too full (of boxes) and too empty (of computation). Each new workload required a new physical machine, a purchase cycle of weeks to months, and a corresponding growth in the facilities to host it.
If an application crashed or had a memory leak, it would not affect other applications. However, this practice led to severe inefficiencies:

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
```

Software engineers independently developed their own reason to hate the pattern: *dependency hell*. A working server configuration was often the accumulated result of years of small adjustments to operating system packages, library versions, environment variables, and kernel parameters. Reproducing that configuration on a new machine was slow, error-prone, and — famously — a machine that worked in development would frequently fail in production for reasons that nobody could explain.

Virtualization addresses both problems simultaneously. By allowing multiple independent operating systems to run on one physical machine, it raises utilisation from 10–15% to 60–80% without sacrificing isolation: each workload still sees a separate operating system with its own kernel, its own file system, its own network stack. And by encapsulating the entire software stack — kernel, libraries, applications, configuration — into a single *virtual machine image*, it solves the reproducibility problem: the image can be copied from development to production unchanged, and it will behave the same way because, from its point of view, nothing has changed.

```
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

> **Intuition.** Virtualization revives, on commodity x86 hardware, the time-sharing model that mainframes pioneered in the 1960s. A single expensive resource (the physical machine) is multiplexed among many users (the VMs), each of whom sees an apparently dedicated computer.

The second consequence — workload mobility and reproducibility — turned out to matter as much as the first. Once a full operating system plus application stack can be treated as a single file, that file can be copied, stored, moved between physical hosts, started and stopped at will. This is the technical precondition for cloud computing: without virtualization, a cloud provider cannot rent out fractional portions of its infrastructure, cannot migrate running workloads to balance load, and cannot give customers the illusion of infinite capacity. Virtualization is to the cloud what the steam engine was to the Industrial Revolution: not the product itself, but the enabling technology without which the product cannot exist.

#### Definitions

> **Virtualization**  is the ability to simulate a hardware platform, such as a server, storage device or network resource, in software. All of the functionality is separated (abstracted) from the hardware and simulated as a “virtual instance” with the ability to operate just like the hardware solution. A single hardware platform can be used to support multiple virtual devices or machines, which are easy to spin up or down as needed.

> **Virtual Machine**  is the software simulation of a computer. It is able to run an Operating Systems and applications interacting with the virtualized abstracted resources, not with the physical resources, of the actual host computer.


> **Hypervisor (or Virtual Machine Monitor)**  is a software tool installed on the physical host system to provide the thin software layer of abstraction that decouples the OS from the physical bare-metal. It allows to split a computer in different separate environment, the Virtual Machines, distributing them the computer resources

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

### A brief history

The idea of virtualization is older than most people realise. The first production system recognisably descended from modern hypervisors was IBM's **CP-40** (1967), which ran on the specially modified System/360 Model 40 at the IBM Cambridge Scientific Center. CP-40 multiplexed the machine among multiple independent copies of a guest operating system — each copy believing it had the whole machine to itself [(Creasy, 1981)](#ref-creasy-1981). The successor systems CP-67 and CP/CMS ran on unmodified System/370 hardware and were commercially successful. Through the 1970s and 1980s, virtualization remained a mainframe feature, largely invisible to the broader computing world.

The rebirth of the idea on commodity hardware is the story of **VMware**, founded in 1998 by Mendel Rosenblum and colleagues at Stanford. The core innovation — which we will study in detail in §8 — was that the x86 architecture, though not designed for virtualization, could be made to support it through a technique called *binary translation*, applied at runtime by the VMM. VMware Workstation (1999) and ESX Server (2001) brought virtualization out of the mainframe and into the mass market.

The next major milestone was **Xen**, developed by Ian Pratt and colleagues at the University of Cambridge and presented at SOSP 2003 [(Barham et al., 2003)](#ref-barham-2003). Xen took a different approach from VMware: instead of cleverly translating the guest's instructions, it modified the guest operating system itself to cooperate with the VMM — a technique called *paravirtualization*. This trade-off (modified guests, simpler VMM) proved popular in the Linux ecosystem.

The hardware industry responded in **2006** with Intel VT-x and AMD-V, two architectural extensions that made x86 directly virtualizable for the first time. With these extensions in place, the elaborate software techniques of VMware and Xen became largely unnecessary for CPU virtualization: the hardware now did what, according to Popek and Goldberg, it always should have done. We will understand exactly what that means in §7.

The Linux kernel absorbed virtualization as a first-class feature in 2008 with the merge of **KVM** (Kernel-based Virtual Machine) [(Kivity et al., 2007)](#ref-kivity-2007). KVM turns the Linux kernel itself into a hypervisor, leveraging VT-x/AMD-V to run guests directly on the CPU. It remains, as of 2026, the dominant open-source hypervisor and the technology underpinning most public clouds.

The final milestone — for now — is AWS's **Nitro system**, introduced around 2017, which offloads most of the virtualization work (network, storage, management) from the host CPU to custom silicon on the server board. We will preview Nitro in §13, and it will be the subject of Lecture 07.

Each of these milestones addressed a limitation of the previous one. The arc is not accidental: virtualization is a field where each new technique is a response to a precise problem in the previous one, and the problem is best understood through the framework we now introduce.

---

## §2. A simplified hardware model

Before we can reason about virtualization precisely, we need a model of the hardware being virtualized. Real CPUs are complicated: the x86-64 instruction set has over a thousand instructions, four privilege rings (of which two are commonly used), several memory-virtualization mechanisms (segmentation, paging, EPT), and dozens of control registers. To state a theorem clearly, we adopt a minimal abstraction, due to Popek and Goldberg, that captures the essence of a protected operating system without getting lost in details.

Our simplified machine has exactly three features: a *privilege bit*, *virtual memory* (in the form of segmentation), and a *trap architecture*. These three together are the minimum a computer needs to run a protected operating system — one that can isolate applications from each other and from itself.

### 2.1 The privilege bit

A single hardware bit inside the CPU determines the privilege level at which the current instruction stream executes. When the bit is *0*, the CPU is in **kernel mode** (also called supervisor, privileged, or ring 0 mode, depending on the architecture). When the bit is *1*, the CPU is in **user mode** (unprivileged, ring 3). The meaning of this bit is simple: in kernel mode, all instructions of the ISA are permitted; in user mode, some instructions — called *privileged* instructions — are forbidden and cause a fault if attempted.

The privilege bit cannot be flipped by arbitrary code. It changes only in two circumstances: on a *trap* (a transition from user to kernel, discussed in §2.3), and on an explicit *return from trap* instruction executed in kernel mode (which switches back to user). This asymmetry is the whole reason the OS kernel can protect itself: user code cannot elevate its own privilege without the OS's consent, because the very instruction that would switch privilege levels is itself privileged.

Without a privilege bit, there is no protection. Any application could overwrite any memory, disable interrupts, reprogram the page tables, or halt the machine. The operating system would be just another program, with no way to enforce any invariant about the system. The privilege bit is, in one bit, the foundation of operating-system protection.

### 2.2 Virtual memory

The second feature is a level of indirection between the addresses seen by running code — *virtual addresses* — and the addresses that actually index physical memory — *physical addresses*. Real hardware implements this indirection with page tables (typically 4 KB pages) walked by a hardware unit called the Memory Management Unit (MMU). For our minimal model, we adopt a simpler scheme called **segmentation**, based on just two privileged registers:

- The **relocation register** holds the base physical address at which virtual address 0 is mapped.
- The **bound register** holds the size of the region that the running process is allowed to access.

When code issues a virtual address $va$, the hardware computes the physical address $pa = reloc + va$ and checks that $va < bound$; if the check fails, a protection fault is raised. Both registers are privileged: only kernel-mode code can modify them. The reason is identical to the one for the privilege bit: if user code could rewrite the relocation register, it could relocate its own view of memory to point anywhere in physical RAM, and the isolation between processes would collapse.

Segmentation is a didactic simplification. Real systems use page tables, which give finer granularity, better memory utilisation, and support for demand paging. But the essential property is the same: a privileged indirection mechanism that the OS sets up and the user program cannot tamper with.

### 2.3 The trap architecture

The third feature is a mechanism to transition between user and kernel modes. Traps on any realistic CPU come in three flavours:

1. A **system call** is a voluntary trap: the user program asks the kernel to perform some service (read from disk, allocate memory, send a network packet) by executing a special instruction such as `syscall` on x86 or `svc` on ARM. Control transfers to the kernel at a pre-registered entry point; the kernel services the request and returns.

2. An **exception** is an involuntary trap caused by the CPU detecting a condition that prevents the current instruction from completing: division by zero, page fault, invalid opcode, privileged-instruction-in-user-mode, and so on. Control transfers to a kernel handler that decides what to do (possibly kill the offending process, possibly service the fault transparently).

3. An **interrupt** is an asynchronous trap caused by external hardware: the timer ticks, a disk completes a request, a packet arrives on the network. The CPU saves enough state to resume what it was doing and transfers control to a kernel handler.

All three kinds of trap do three things in common: they switch the privilege bit to kernel mode, they transfer control to a handler address pre-registered by the kernel, and they save enough CPU state (program counter, flags, possibly a few general-purpose registers) to allow later resumption of the interrupted code. These three actions are the entry points through which the OS kernel can assert control over the machine.

### 2.4 Why these three features suffice

Given the privilege bit, virtual memory, and a trap architecture, a protected operating system is possible. Remove any one and the whole structure collapses:

- Without a privilege bit, user code is indistinguishable from kernel code. There is no protection.
- Without virtual memory, two user processes share the same physical address space; there is no isolation between them.
- Without a trap architecture, the kernel can never regain control once user code starts running. The machine is effectively stuck in whatever the first program decides to do.

Hold this three-feature model in your head. We will now build a VMM on top of it and ask what additional property the hardware must have — beyond these three features — for virtualization to be efficient.

> **Self-check §2.**
> (a) Explain why the relocation register must be privileged.
> (b) Could virtualization work with a two-valued privilege bit, or does it require more levels?
> (c) On a machine with no trap architecture, can you still write a functional OS? What do you lose?

---

## §3. Virtual machines, VMMs, and the three requirements

We now introduce the central abstractions of the field and the formal criteria a correct virtualization system must meet.

### 3.1 The virtual machine abstraction

A **virtual machine (VM)** is an abstraction whose interface is sufficiently equivalent to a real machine that an operating system can run inside it, unmodified, as if it were executing on physical hardware. The word "sufficiently" hides all the difficulty, and most of this lecture consists of unpacking it.

The software that implements the VM abstraction is called a **Virtual Machine Monitor (VMM)**, a term introduced by Popek and Goldberg in 1974, or a **hypervisor**, a term coined in the IBM tradition (originally denoting a supervisor of supervisors — that is, a kernel over kernels). The two terms are interchangeable; "hypervisor" is the more common in modern usage, "VMM" in academic literature.

Crucially, the VMM runs *beneath* one or more guest operating systems. Each guest believes it has full control over a dedicated machine: its own CPU, its own memory, its own devices. The VMM's job is to maintain this illusion while actually sharing the physical hardware among multiple guests.

> **Formally.** A VMM is a piece of software that provides an abstraction indistinguishable, from the guest's perspective, from the hardware interface on which it runs, while allowing multiple such abstractions to coexist on the same physical machine.

### 3.2 The three requirements

Popek and Goldberg, in their 1974 paper *Formal Requirements for Virtualizable Third-Generation Architectures* [(Popek & Goldberg, 1974)](#ref-popek-goldberg), identified three requirements that a correctly implemented VMM must satisfy. These three properties remain the organising principle for reasoning about virtualization fifty years later.

**Equivalence.** Software running inside the VM must behave as if it were running on the bare hardware. More precisely, a program executing on the VM should produce the same results it would produce on equivalent bare hardware, *with the exception* of timing differences and resource limitations. The exception is important: we do not require that a program running in a VM be as fast as on bare metal (Popek & Goldberg are explicit about this), nor that it see the same total amount of memory. We only require that its *logical* behaviour — the sequence of states it passes through, the values it computes, the system calls it issues — be the same.

**Safety (resource control).** The VMM must retain complete control over the physical resources. No guest VM may access physical hardware that the VMM has not granted it, nor interfere with another VM's resources. The VMM is the sole authority over the machine; guests are tenants, not landlords.

**Performance.** A statistically dominant subset of the guest's instructions must execute directly on the physical CPU without intervention by the VMM. This requirement rules out pure interpretation (where the VMM would fetch, decode, and simulate every guest instruction in software), which would be correct but far too slow to be useful.

### 3.3 Why the three together are hard

Any two of these requirements are easy to satisfy in isolation. The difficulty lies in satisfying all three simultaneously.

Suppose we satisfy *equivalence* and *safety* by interpreting every guest instruction in software. This is straightforward and indisputably correct: the VMM maintains a model of the virtual CPU state, and for each instruction it fetches from the guest, it updates the model accordingly. The guest never touches the real CPU, so there is no risk to safety, and as long as the interpreter is faithful, equivalence is preserved. But the cost is crushing: interpretation overhead is typically 10× to 100× the native execution time. No serious workload can tolerate this.

Suppose instead we satisfy *equivalence* and *performance* by simply running the guest directly in kernel mode on the physical hardware. Now performance is native — there is no overhead — and equivalence is also preserved, because the guest really is running on the hardware. But safety is gone: a misbehaving guest can overwrite the VMM's memory, reprogram devices, halt the machine, or attack the other guests.

Finally, suppose we satisfy *safety* and *performance* by exposing a simplified non-equivalent interface: something like the Java Virtual Machine, or WebAssembly, or a managed runtime. Safety is preserved because the guest can only do what the managed runtime permits, and performance can be excellent thanks to JIT compilation. But equivalence is lost: we cannot run an unmodified operating system in a Java VM. We have built something useful but it is not *virtualization* in the sense Popek and Goldberg defined.

The central question of virtualization is therefore: **can all three requirements be satisfied at the same time?** And if so, under what hardware conditions? The answer, which we will reach in §7, is that they can, but only if the hardware satisfies a precise architectural condition.

---

## §4. Direct execution and trap-and-emulate

The architectural choice that made modern virtualization possible is called **direct execution**: the VMM does *not* interpret the guest's instructions one by one, but lets the guest execute directly on the physical CPU. This seems dangerous at first — how can the VMM maintain safety if the guest is running on the real hardware? The answer is a specific discipline, called *trap-and-emulate*, which controls when and how the VMM intervenes.

### 4.1 The analogy with ordinary operating systems

Before describing trap-and-emulate, observe that the pattern is familiar: it is exactly how a normal operating system runs its applications. When you launch a program on Linux, the kernel does not interpret its instructions; it loads the program into memory, sets up the page tables, and issues a `sysret`-like instruction that transfers control to the program in user mode. The program then runs directly on the CPU, at full speed, without kernel involvement — until it issues a system call, causes a fault, or is preempted by an interrupt.

Virtualization generalises this idea one level. The VMM stands to the guest OS in the same relationship the OS stands to its applications:

| | Standard OS | Virtualized |
|---|---|---|
| Kernel-mode | OS | VMM |
| User-mode (deprivileged) | Applications | Guest OS + applications |
| Execution model | direct execution, intervention on traps | direct execution, intervention on traps |

The insight is that the mechanisms an OS already provides — the privilege bit, virtual memory, the trap architecture — are exactly the mechanisms a VMM needs. They simply need to be used at a new level of indirection.

### 4.2 Deprivileging the guest

The VMM arranges the privilege levels as follows. The VMM itself runs in kernel mode; it has the full privileges of an ordinary OS kernel, because on bare metal, that is what it is. The **guest operating system**, however, is forced to run in *user mode*, even though it was written assuming it would run in kernel mode. This is called **deprivileging** the guest. The guest's own applications then run in user mode too, as always.

The key property of deprivileging is that the guest OS, when it attempts to execute a privileged instruction — something that it would normally do freely as a kernel, such as disabling interrupts or loading a segment register — fails with a fault. The fault traps into the VMM (which is the entity currently running in kernel mode), and the VMM gets the opportunity to decide what should happen.

### 4.3 The trap-and-emulate loop

Putting it all together, the VMM operates in a loop whose structure is simple and repeats for as long as the VM is running:

1. The guest runs directly on the physical CPU, in user mode, at native speed. Regular instructions — arithmetic, memory access, control flow — execute normally without any VMM involvement.
2. The guest attempts a privileged instruction. Because it is in user mode, this triggers a general-protection fault (#GP on x86) or an equivalent exception on other ISAs.
3. The hardware delivers the fault to the VMM, because the VMM (in kernel mode) is the designated handler.
4. The VMM examines the faulting instruction, understands what the guest was trying to do, and **emulates** it: it updates its internal data structures to reflect the operation's effect on the guest's virtual state, without actually performing the privileged operation on the real hardware (or, if the real operation is safe, performs a carefully controlled version of it).
5. The VMM returns control to the guest, which resumes execution at the instruction following the faulting one. From the guest's perspective, the privileged operation appeared to succeed.

The ratio between native-speed execution (step 1) and trapping instructions (steps 2–5) is crucial to performance. If only a tiny fraction of the guest's execution involves privileged instructions — which is typical, since kernel code is a minority of most workloads — then the trap-and-emulate VMM achieves near-native performance.

> **Intuition.** Trap-and-emulate is the "lazy" discipline: let the guest do whatever it wants at native speed, and only step in when absolutely necessary. The rarer the intervention, the faster the VM.

### 4.4 The invisible assumption

This construction works beautifully — under one condition, which Popek and Goldberg formalised and which we will now make precise. The VMM relies on a specific property of the hardware: that **every instruction whose behaviour matters for virtualization must trap when attempted in user mode**. If any instruction can observe or modify the virtualized state without trapping, the VMM loses control over that instruction, and equivalence is silently violated.

The question "does every instruction that matters trap?" is a hardware question, determined by the ISA designer decades before anyone thought about virtualization. For some ISAs the answer is yes (IBM z/Architecture, PowerPC); for others it is no (classic x86, MIPS, ARM before the virtualization extensions). On the latter, pure trap-and-emulate simply does not work, and something more elaborate is required. To understand why — and what "something more elaborate" looks like — we need a precise classification of instructions, which is the subject of the next section.

> **Self-check §4.**
> (a) In what sense is the VMM "to the guest OS as the OS is to applications"?
> (b) What happens in the trap-and-emulate loop if the guest executes a long run of arithmetic instructions with no system calls or faults?
> (c) Why is the expression "direct execution" preferable to "native execution" in describing step 1 of the loop?

---

## §5. Does trap-and-emulate always work? Classifying instructions

The question we ended §4 with — does every instruction that matters for virtualization trap in user mode? — is so important that it deserves precise vocabulary. We will classify every instruction of an ISA along two orthogonal axes: a *hardware* axis, which captures how the CPU reacts to the instruction in user mode, and a *semantic* axis, which captures whether the instruction's meaning depends on the machine's control state. The four combinations these axes generate are unequal: three are benign, one is the source of every virtualization difficulty on x86.

### 5.1 The hardware axis: privileged vs non-privileged

An instruction is **privileged** if the hardware is designed to trap when the instruction is executed in user mode. It is **non-privileged** if the hardware allows it to execute in both modes without trapping.

This is a fact about silicon: it is fixed by the ISA designer at the time the architecture is defined, and it has nothing to do with what the instruction semantically does. The choice of which instructions to make privileged reflects a compromise between performance (making an instruction privileged forces a trap on every user-mode use, which is expensive if the instruction is common) and safety (making an instruction non-privileged allows user code to execute it, which is dangerous if the instruction has side effects on global state).

On x86, examples of privileged instructions include `HLT` (halt the CPU), `LIDT` and `LGDT` (load the interrupt and global descriptor tables), `MOV` to a control register such as `CR0` or `CR3`, and the I/O instructions `IN`/`OUT`. Each of these traps with a general-protection fault (#GP) if attempted in user mode.

### 5.2 The semantic axis: sensitive vs regular

An instruction is **sensitive** if its behaviour depends on, or modifies, the *control state* of the machine. By control state we mean the architectural state that determines how subsequent instructions are interpreted: the privilege bit, the memory-virtualization registers (reloc, bound, or page-table base), the interrupt-enable flag, the descriptor tables, and so on.

Popek and Goldberg distinguish two subspecies. An instruction is **control-sensitive** if it reads or writes the control state directly; it is **behaviour-sensitive** if its semantics depend on the control state, even if it does not read or write it explicitly. The distinction matters because an instruction can be behaviour-sensitive without being syntactically obvious: consider an instruction that returns the physical address corresponding to a given virtual address. Its input and output are data, not control state, but its *meaning* depends on the current value of the relocation register, so it is behaviour-sensitive.

An instruction is **regular** if it is neither control-sensitive nor behaviour-sensitive: its meaning is fully determined by its operands, without reference to the machine's control state. The overwhelming majority of instructions in any ISA are regular: `ADD`, `MOV` between general-purpose registers, `JMP`, `CMP`, and so on.

Sensitivity is a *semantic* fact about what the instruction does, not a hardware fact about how the CPU implements it. It is independent of whether the instruction is privileged or not.

### 5.3 The 2×2 matrix

Cross-classifying instructions on these two orthogonal axes gives four cells:

| | Privileged | Non-privileged |
|---|---|---|
| **Sensitive** | **GOOD CELL** — instruction is sensitive *and* traps in user mode. VMM intercepts and emulates. Example: `HLT`. | **BAD CELL** — instruction is sensitive *but* executes silently in user mode. VMM is never invoked. Example: `popf`. |
| **Regular** | Harmless but wasteful: VMM gets unnecessary traps. Example: `INVLPG` on some ISAs. | The normal case: native execution, no VMM involvement. Example: `ADD`, `MOV`. |

Three of the four cells are benign. The upper-right cell — **sensitive but non-privileged** — is the root of every classical virtualization difficulty. An instruction in this cell touches control state (so the VMM must know about it, to maintain equivalence) but executes silently in user mode (so the VMM never hears about it, because no trap fires). The guest and the VMM diverge: the guest has changed its view of the control state, but the VMM's model of the guest is still the old one.

This is the Popek–Goldberg "bad cell." Any ISA whose upper-right cell is non-empty cannot support pure trap-and-emulate virtualization, because the VMM has no way to intercept the sensitive-but-non-privileged instructions. The central theorem of virtualization, which we state in §7, is essentially the claim that this cell must be empty.

### 5.4 Rare fourth case: privileged but regular

The lower-left cell — privileged but regular — is rare but worth mentioning for completeness. Some ISAs privilege instructions whose semantics are not actually sensitive: cache-management instructions like `INVLPG` (invalidate one page-table entry) or `WBINVD` (write-back and invalidate cache) fall here on some architectures. These instructions trap unnecessarily from the VMM's correctness standpoint, which makes the guest incur VMM round-trips it does not logically need. The consequence is performance overhead, not correctness violation, but it is worth being aware of when measuring virtualized performance.

> **Self-check §5.**
> (a) Why is it useful to separate the hardware axis (privileged) from the semantic axis (sensitive)?
> (b) Give an example of a control-sensitive instruction and a behaviour-sensitive instruction. Can an instruction be both?
> (c) If an ISA has no sensitive-but-non-privileged instructions, what does that imply for VMM design?

---

## §6. Two concrete examples: `HLT` and `popf`

Abstract classifications are best understood through examples. We work through two instructions from x86 — one from the good cell, one from the bad cell — tracing in detail what happens when they execute under a trap-and-emulate VMM.

### 6.1 The well-behaved case: `HLT`

The `HLT` instruction halts the CPU until an interrupt arrives. Operating systems use it extensively in the idle loop: when there is no runnable process, the kernel issues `HLT`, the CPU stops consuming power, and the next timer interrupt (10 milliseconds later, typically) wakes it up to check again. Under virtualization, `HLT` is both sensitive (it modifies the CPU's activity state) and privileged (on x86, it is designed to raise #GP when executed in user mode).

Suppose the guest OS, running deprivileged in user mode, reaches its idle loop and issues `HLT`. The CPU raises a general-protection fault, which the hardware delivers to the VMM (running in kernel mode). The VMM examines the faulting instruction, recognises it as `HLT`, and emulates it: it marks the virtual CPU as "halted" in its internal state, and *does not* actually halt the physical CPU — that would stop not only this VM but everything else on the host. Instead, the VMM takes the opportunity to run something else: another VM, or its own idle logic. When a virtual interrupt eventually needs to be delivered to the halted VM (because its virtual timer fires, or because an I/O completion needs to be signalled), the VMM marks the VM as runnable again, restores its vCPU state, and returns to it.

The guest observes nothing unusual. It issued `HLT`, and at some later point it resumed execution immediately after the `HLT` — exactly as it would have done on bare hardware, where the resumption would have been triggered by a real interrupt. Equivalence is preserved, the VMM retains control, and the cost is a single trap-and-emulate round per `HLT` invocation. This is virtualization working as designed.

### 6.2 The pathological case: `popf`

The `popf` instruction pops a word from the top of the stack into the `EFLAGS` register on x86. `EFLAGS` is a single 32-bit register that mixes two very different kinds of flags:

- **Arithmetic flags** such as the zero flag (ZF), carry flag (CF), sign flag (SF), and overflow flag (OF). These are set by nearly every arithmetic or logical instruction and are part of the normal, user-level programming model.
- **Control flags**, in particular the **interrupt-enable flag (IF)**, which determines whether the CPU delivers external interrupts. The IF flag is privileged: it must only be modifiable by code operating at kernel-level privilege, because a user-mode program that could disable interrupts could monopolise the CPU indefinitely.

The standard use of `popf` in system code is the pair `pushf / ... / popf`, which saves `EFLAGS` onto the stack, performs some operation that clobbers the arithmetic flags, and restores them. In *kernel* code, the same pattern is used to save and restore the IF flag around a critical section: `pushf` to save current interrupt state, `cli` to disable interrupts, perform the critical section, and `popf` to restore whatever interrupt state was in effect before.

The question the x86 architects faced in the 1980s was: what should `popf` do when executed in user mode? The instruction, after all, can write to IF, which is privileged. They faced a dilemma.

Option one: make `popf` trap in user mode. This is architecturally clean — `popf` becomes a privileged instruction, and any user-mode use raises #GP. But `popf` is used everywhere in application code to restore arithmetic flags, not just in kernel critical sections. Making it trap would cripple the performance of every x86 program, because every ordinary user-space `popf` would now entail a round-trip through the OS kernel's fault handler.

Option two: let `popf` execute in user mode, but make it capable of modifying IF. This would be a security disaster: any user program could disable interrupts.

Option three, which is what the x86 actually does: let `popf` execute silently in user mode, **ignoring** writes to the IF bit. The arithmetic flags are restored as expected, but attempts to modify IF are simply dropped on the floor. User programs do not care (they do not need to touch IF), and kernel code — which *does* run in kernel mode — can freely modify IF. The compromise is elegant in the non-virtualized setting.

In the virtualized setting, however, the compromise is catastrophic. Here is why, traced step by step.

### 6.3 Trace: a kernel critical section under trap-and-emulate

Suppose the guest kernel — running deprivileged in user mode, because of the VMM — executes the following standard critical-section idiom:

```
pushf                ; save flags (including current IF)
cli                  ; disable interrupts
... critical code ...
popf                 ; restore saved flags (intending to re-enable IF)
```

Before entering this sequence, the guest believes that interrupts are enabled; the VMM's internal model of the guest agrees. Both have `virtual-IF = 1`.

**Step 1: `pushf`.** `pushf` is not privileged; it simply reads EFLAGS and pushes it onto the stack. It executes in user mode without any trap. The guest's saved value records `IF=1`. The VMM is not involved; its internal model is unchanged.

**Step 2: `cli`.** `cli` clears the IF flag, and it *is* privileged on x86. The guest attempts it in user mode; the CPU raises #GP; the trap is delivered to the VMM. The VMM emulates the `cli`: it updates its internal state to record that the guest has now disabled interrupts (`VMM.virtual-IF[guest] := 0`), and thenceforth suppresses delivery of virtual interrupts to the guest. The VMM returns, and the guest resumes, believing that `cli` succeeded and interrupts are now disabled. So far, so good: guest and VMM agree that `virtual-IF = 0`.

**Step 3: critical section.** The guest executes whatever code the critical section protects — arithmetic, memory access, whatever. No traps fire. The VMM's state remains consistent with the guest's.

**Step 4: `popf`.** This is where the trap-and-emulate model breaks down. `popf` is *not* privileged on x86. The guest executes it in user mode; no trap fires; the VMM is not invoked. The CPU executes `popf` according to its user-mode semantics: arithmetic flags are restored, and the write to IF is **silently ignored**.

What does the guest believe? The guest thinks `popf` has restored the saved flags, including IF=1. The guest is now proceeding under the assumption that interrupts are enabled.

What does the VMM know? The VMM was never told about the `popf`. Its internal model still says `VMM.virtual-IF[guest] = 0`, because the last event it saw was the `cli` in step 2. The VMM continues to suppress delivery of virtual interrupts.

**Step 5: aftermath.** The guest expects virtual interrupts — the timer ticking every 10 ms, the scheduler preempting long-running tasks, I/O completions waking sleeping processes. The VMM suppresses all of them, because from its point of view, the guest is still in the critical section with interrupts disabled. The guest hangs, or — more insidiously — misbehaves without hanging: long-running computations are not preempted, and the overall system becomes unresponsive in ways that are very hard to diagnose.

There is no crash, no exception, no diagnostic message. The guest and the VMM quietly disagree about a single bit of interrupt state, and the consequence is a functional malfunction that is invisible at the architectural level. This is the worst possible failure mode: not an error, but a silent divergence from specified behaviour.

### 6.4 Why `popf` is the pedagogical example

The `popf` case is not a peculiarity. It is the canonical example of a sensitive-but-non-privileged instruction on x86, and it concentrates, in one short trace, every element of a Popek–Goldberg violation:

1. **Sensitive**: `popf` writes IF, which is control state. A correct VMM must know about it.
2. **Non-privileged**: `popf` executes silently in user mode. No trap is raised, so the VMM cannot know about it through its usual mechanism.
3. **In the bad cell of the matrix**: exactly the Popek–Goldberg forbidden cell.
4. **Silent divergence**: the failure mode is invisible, not loud. This is what makes the problem so insidious.
5. **Representative, not contrived**: the `pushf / cli / ... / popf` pattern is the canonical kernel critical section. Every serious operating system uses it.

Beyond `popf`, x86 has a zoo of similar instructions — `sgdt`, `sidt`, `smsw`, `str` — which read the descriptor tables or system registers into general-purpose registers. All are non-privileged (for historical performance reasons) but all are sensitive (they leak virtualized control state to the guest). Each contributes a reason why pure trap-and-emulate does not work on x86.

The natural next question is: precisely when does trap-and-emulate work? The answer is a theorem.

> **Self-check §6.**
> (a) If `popf` were privileged on x86, what would be the performance impact on ordinary user programs?
> (b) Why is a silent failure worse than a crash, from a debugging standpoint?
> (c) Identify the step in the `popf` trace at which the guest and the VMM first disagree.

---

## §7. The Popek–Goldberg theorem

We have gathered the vocabulary. The classical theorem of virtualization can now be stated in one sentence.

### 7.1 Statement

**Theorem (Popek & Goldberg, 1974).** A trap-and-emulate VMM for a given ISA can be constructed if and only if every sensitive instruction in the ISA is also privileged.

Formally, let $S$ denote the set of sensitive instructions (the union of control-sensitive and behaviour-sensitive) and $P$ the set of privileged instructions. The condition is

$$S \subseteq P.$$

In Venn diagram terms: the sensitive set must fit entirely inside the privileged set. Equivalently, the "bad cell" of the 2×2 matrix — sensitive but not privileged — must be empty.

### 7.2 Intuition for the two directions

The theorem has two directions, and it is instructive to understand both.

**Necessity (if $S \not\subseteq P$, no trap-and-emulate VMM can exist).** If there is an instruction $i$ that is sensitive but not privileged, the guest can execute $i$ in user mode without trapping. The VMM is never invoked. Because $i$ is sensitive, it affects the control state — either changing it (control-sensitive) or acting on its basis (behaviour-sensitive). The guest's view of the control state now diverges from the VMM's view. Equivalence fails. The `popf` trace of §6.3 is exactly this argument made concrete.

**Sufficiency (if $S \subseteq P$, a trap-and-emulate VMM can be constructed).** Popek and Goldberg proved this by *explicit construction*. Their argument, in essence, is:

1. Run the guest deprivileged, in user mode.
2. Every time the guest tries to execute a sensitive instruction, the inclusion $S \subseteq P$ guarantees that it is also privileged, so it traps to the VMM.
3. The VMM emulates the instruction — updating its model of the guest's virtual state — and returns.
4. Memory virtualization is handled by a second layer of indirection: the VMM sets up its own page tables such that the guest's virtual memory maps to host physical memory in a way consistent with the guest's intent but controlled by the VMM.
5. I/O is handled by mapping guest I/O instructions (which are privileged, and therefore trap) to emulated device implementations.

The resulting VMM satisfies all three requirements: equivalence (because every sensitive operation is mediated by the VMM, so it can maintain a consistent model), safety (because the guest never has privilege to affect host state), and performance (because regular instructions execute natively, and the dominant fraction of a typical workload is regular).

The theorem is therefore not merely a definition; it is a theorem with real content, giving a diagnostic criterion for whether a given ISA is classically virtualizable.

### 7.3 Applying the theorem to real ISAs

Which ISAs satisfy Popek–Goldberg? The answer is historical and surprisingly mixed.

**IBM System/360, /370, z/Architecture.** From the earliest days, IBM mainframe ISAs were designed with virtualization in mind (because CP-40 and its successors needed it). The sensitive set is a subset of the privileged set. z/Architecture, the modern descendant, continues this tradition. As a consequence, hypervisors on IBM mainframes (like z/VM) have always used pure trap-and-emulate.

**PowerPC.** The original PowerPC architecture was designed with a clean privilege model. Its sensitive instructions are privileged. Trap-and-emulate works.

**x86 (before 2006).** The original 8086 had no privilege bit at all; privilege was added incrementally from the 80286 onward, with the constraints of backward compatibility. As a result, the x86 sensitive set contains 17 instructions that are not privileged [(Robin & Irvine, 2000)](#ref-robin-irvine): `popf`, `pushf`, `sgdt`, `sidt`, `sldt`, `smsw`, `str`, and various protected-mode instructions. The bad cell is non-empty. Pure trap-and-emulate fails.

**MIPS (classical).** MIPS has several non-trapping sensitive instructions, primarily in its system coprocessor interface. Pure trap-and-emulate fails.

**ARM (before Virtualization Extensions).** ARMv7 and earlier had similar problems. ARM's Virtualization Extensions (ARMv7-A VE, introduced 2010) and the refined design in ARMv8-A added a new exception level EL2, which solves the problem in the same style as Intel VT-x.

In tabular form:

| ISA | PG Condition | Problematic instructions | Consequence |
|---|---|---|---|
| IBM z/Arch | **Passes** | — | Trap-and-emulate works. |
| PowerPC | **Passes** | — | Trap-and-emulate works. |
| x86 (pre-2006) | **Fails** | `popf`, `pushf`, `sgdt`, `sidt`, ... | Cannot use pure trap-and-emulate. |
| MIPS (classical) | **Fails** | Several, in coprocessor interface | Cannot use pure trap-and-emulate. |
| ARM (pre-VE) | **Fails** | Several, in system-control interface | Cannot use pure trap-and-emulate. |

The first three quarters of the history of virtualization on commodity hardware is essentially the story of how the industry compensated for the bottom three rows of this table. Three different compensations were developed, each of which became a major branch of virtualization technology.

> **Self-check §7.**
> (a) Why is the Popek–Goldberg condition stated as $S \subseteq P$ rather than $S = P$?
> (b) The theorem was published in 1974. Why did it take decades before the implications for x86 became practically urgent?
> (c) Could a PC-compatible x86 CPU have been made Popek–Goldberg-compliant from the start, without breaking compatibility with existing software?

---

## §8. Three answers to Popek–Goldberg

The 1990s and early 2000s saw three distinct responses to the Popek–Goldberg problem on x86. Each places the "fix" at a different point in the lifecycle of the software stack, which is the right framework for understanding them. The four possible fix-times form a natural axis:

- **CPU design-time** — change the silicon.
- **Guest OS compile-time** — change the guest.
- **VMM runtime, before execution** — rewrite the guest's code as it runs.
- **CPU runtime, during execution** — the pure trap-and-emulate baseline, which requires ISA compliance.

The first three of these correspond to the three industrial responses: **hardware-assisted virtualization** (Intel VT-x, 2006), **paravirtualization** (Xen, 2003), and **binary translation** (VMware, 1998). We present them in historical order, which is also the order of increasing hardware requirement.

### 8.1 Binary translation — VMware, 1998

The first workable approach, invented by VMware's founders at Stanford in the mid-1990s and productised in 1998, is **binary translation**. The idea is to let the VMM intervene between the guest's instruction stream and the physical CPU: the VMM scans the guest's code in chunks (basic blocks) and, *before* each block is executed, produces a translated version in which problematic instructions are replaced with safe equivalents.

Specifically: for a basic block of guest code, the VMM walks through the instructions one by one. Regular instructions are copied verbatim into a translation cache. Sensitive-but-non-privileged instructions — the ones Popek–Goldberg would trap, if the ISA permitted it — are replaced with a call into a VMM handler that simulates the instruction's semantics correctly. Privileged instructions, which would trap anyway, can either be left to trap or be translated into direct VMM calls for efficiency. The translated block is then executed directly on the CPU. For subsequent executions of the same guest code, the cached translation is reused — an idea called *block chaining*, borrowed from dynamic binary translators like QEMU's TCG.

The technical challenge is considerable. The translator must handle self-modifying code, indirect jumps whose targets cannot be determined statically, and the interaction between the guest's own page tables and the VMM's translation cache. VMware's engineers famously spent years on these details; the result was a VMM that achieved 5–20% overhead on compute-intensive workloads and somewhat more on system-intensive ones, while running fully unmodified guest operating systems — including Windows, which was the commercial linchpin.

A crucial simplification is that **user-mode code in the guest does not need to be translated**, in most implementations. Binary translation is applied only to the guest's kernel code, where the problematic instructions reside. Guest applications run at native speed. This is what made the technique practical.

> **Part I analogue / industrial impact.** Binary translation is the "VMM at runtime" answer: the fix is applied by the VMM, just before the guest executes, without touching either hardware or guest source. It proved that x86 virtualization is feasible on 1998 hardware, and it founded VMware as a company — arguably launching the modern virtualization industry.

The trade-offs are clear. Pros: guest is completely unmodified (this is huge for Windows and other closed-source OSes); no hardware requirements; works on any x86 of 1998 vintage. Cons: enormous VMM engineering complexity; 5–20% overhead; largely obsolete for CPU virtualization after VT-x became widespread (2006), because there is no longer a good reason to go through software translation when the hardware will do the trapping for you. VMware itself still supports binary translation as a fallback, but its modern products use VT-x by default.

### 8.2 Paravirtualization — Xen, 2003

The second response, taken by Xen at Cambridge in 2003 [(Barham et al., 2003)](#ref-barham-2003), moves the fix upstream: **modify the guest operating system** at the source-code level so that it explicitly cooperates with the VMM. Rather than executing problematic instructions and relying on the VMM to catch them (via traps or translation), the paravirtualized guest is rewritten to call the VMM directly for the relevant operations. These calls are called **hypercalls**, by analogy with system calls: a system call is how user code asks the OS for a service, and a hypercall is how an OS (the guest) asks the hypervisor (the VMM) for a service.

Concretely, a paravirtualized Linux kernel has its `cli`, `sti`, `popf` (when used to modify IF), page-table manipulation, and I/O-port access all replaced with calls into a small API the hypervisor exposes. Rather than

```
popf                   ; restore flags (possibly changing IF)
```

the paravirtualized guest does

```
HYPERCALL_set_flags(saved_flags)
```

The hypercall is an ordinary procedure call into code that, in the Xen case, lives in a different protection ring and is called via a specific software interrupt or direct call mechanism. The VMM receives the call, knows exactly what the guest intended (because the guest told it explicitly), and updates its internal state consistently.

Paravirtualization turns the Popek–Goldberg problem inside out. Instead of fighting the ISA, it changes the contract between guest and host: the guest is now VMM-aware, and the VMM is now a peer rather than an impostor.

> **Part I analogue / industrial impact.** Paravirtualization is the "compile-time" answer: the fix is applied by the guest OS developer, in source code. It removes the need for hardware support or runtime translation, and it produces a VMM that is dramatically simpler than a binary translator. Xen's VMM code base is small and auditable, which contributed to its success in academic and security-sensitive settings.

Pros: near-native performance (no translation, no silent failures); VMM is dramatically simpler than a binary-translation engine; no hardware requirements. Cons: the guest OS must be modified, which works well for open-source Linux (and to a lesser extent BSD) but is impossible for Windows without vendor cooperation (which Microsoft eventually provided via its "enlightenments," a paravirtualization interface for Hyper-V). A paravirtualized guest is tied to a specific hypervisor ABI and cannot be booted unchanged on bare metal.

Paravirtualization was superseded, for CPU virtualization, by the arrival of VT-x in 2006. But it was never superseded for **I/O virtualization**: the `virtio` interface, which we will study in Part II (§9.3), is essentially paravirtualization applied to devices, and it remains the industry standard on KVM-based clouds to this day.

### 8.3 Hardware-assisted virtualization — Intel VT-x and AMD-V, 2006

The third response, and the one that defined the modern era, came from Intel and AMD independently, in 2005–2006. Rather than work around the ISA's failure to satisfy Popek–Goldberg, they *changed the ISA* — making the condition true in silicon. The resulting extensions are **Intel VT-x** and **AMD-V**; the details differ but the structural idea is identical.

The key architectural move is to introduce a new, *orthogonal* privilege axis: **root mode** vs **non-root mode**. The existing ring 0 / ring 3 (kernel / user) distinction is left alone, but each of these rings now exists in two variants: root and non-root. The VMM runs in root mode (at any ring, but typically ring 0); the guest OS runs in non-root mode, ring 0. The guest, crucially, *believes* it is in ordinary kernel mode — and for the vast majority of instructions, it is. Regular instructions and most privileged instructions execute at native speed.

The difference is that certain instructions and events, when they occur in non-root mode, trigger a **VM-exit**: a transition from non-root back to root, delivering control to the VMM. The set of instructions that cause a VM-exit is configurable: the VMM sets up a data structure called the **VMCS** (Virtual Machine Control Structure) that specifies exactly which instructions, which memory accesses, and which events should cause a VM-exit. In particular, the sensitive-but-non-privileged x86 instructions — `popf` included — can be configured to cause a VM-exit, thereby giving the VMM the control point that the original ISA denied it.

With VT-x, the Popek–Goldberg condition is satisfied *by hardware configuration* rather than by a property of the original ISA. The guest's `popf` now causes a VM-exit; the VMM emulates it consistently; and the silent-failure mode of §6 simply cannot occur.

> **Part I analogue / industrial impact.** Hardware-assisted virtualization is the "silicon" answer: the fix is in the CPU itself, invisible to both VMM and guest. Since 2006, it has been the default for every major hypervisor — VMware, Xen, KVM, Hyper-V. Binary translation for CPU virtualization has become a historical curiosity, and paravirtualization survives mainly for I/O.

Pros: near-native performance; guest runs unmodified (important for Windows, legacy OSes); VMM implementation is dramatically simpler than binary translation. Cons: requires the extended hardware (universally available since ~2010 on any server or desktop CPU, but historically a limitation); early VT-x implementations had high VM-exit latencies, but successive generations have reduced these substantially.

### 8.4 The three side by side

To summarise:

| | Binary translation | Paravirtualization | Hardware-assisted (VT-x) |
|---|---|---|---|
| Year introduced | 1998 (VMware) | 2003 (Xen) | 2006 (Intel / AMD) |
| Who applies the fix | VMM at runtime (pre-execution) | Guest OS developer (source) | CPU designer (silicon) |
| Guest modified? | No | **Yes** | No |
| New hardware needed? | No | No | **Yes** |
| Typical performance | 5–20% overhead | Near-native | Near-native |
| VMM complexity | Very high | Moderate | Low |
| Status today | Historical / fallback | Alive in virtio I/O | Industry standard |

Each technique was a response to a real problem. Binary translation proved that x86 virtualization was possible at all. Paravirtualization proved that cooperation between guest and VMM produces simpler systems. Hardware-assisted virtualization proved that the cleanest solution is to make Popek–Goldberg true in silicon. Between them, they cover the whole design space of §8's temporal axis: the fix can be applied at design-time (silicon), compile-time (guest source), or runtime (VMM).

### 8.5 Recap of Part I

We have come a long way. Let us recapitulate what we have learned before moving on to Part II.

1. **Virtualization has an abstraction (the VM) and an implementation (the VMM).** Popek and Goldberg formalised the requirements in 1974: equivalence, safety, performance. Each pair is achievable; all three together is the hard problem.

2. **Trap-and-emulate is the canonical VMM construction.** The guest runs deprivileged, at native speed; the VMM intercepts privileged instructions via traps; regular instructions are not slowed down. But this only works if the hardware cooperates.

3. **The Popek–Goldberg theorem tells us when the hardware cooperates.** The condition is $S \subseteq P$: every sensitive instruction must be privileged. It is a diagnostic tool, not a prescription.

4. **x86 (pre-2006), MIPS, and ARM (pre-VE) fail the condition.** The canonical counterexample is x86's `popf`, which silently ignores writes to the IF flag when run in user mode. Silent failure — rather than a clean crash — is what makes the violation so insidious.

5. **Three industrial responses: binary translation, paravirtualization, hardware-assisted.** Each places the fix at a different lifecycle point (runtime, compile-time, design-time). Binary translation founded VMware; paravirtualization founded Xen; VT-x/AMD-V made pure trap-and-emulate viable on x86.

6. **In Part II, the same three ideas reappear — applied to I/O.** We will see that emulation, `virtio`, and passthrough/SR-IOV are the I/O analogues of binary translation, paravirtualization, and hardware-assisted CPU virtualization. The framework is the same; the axis has shifted from the CPU to devices.


---

# Part II — Practice: resource virtualization

Part I established the theoretical framework for CPU virtualization. Part II extends the framework to the resources that, in a real system, matter as much as the CPU: I/O devices, persistent storage, and the network. The punchline — which you should keep in mind as you read — is that the same three ideas reappear in each domain. The specific techniques have different names (emulation, `virtio`, passthrough, SR-IOV), but they correspond one-to-one to the three Part I responses (binary translation, paravirtualization, hardware-assisted). The generality of the framework is itself a deep lesson: once you understand Popek–Goldberg, you understand why every virtualization problem admits essentially the same three solutions.

## §9. Hypervisor Architectures

### 9.1 What is a Hypervisor?

A **hypervisor** (also called a **Virtual Machine Monitor**, or VMM) is the software — and increasingly hardware — layer responsible for creating, running, and managing virtual machines. It is the fundamental component that mediates between guest operating systems and physical hardware.

A hypervisor has four core responsibilities:

1. **CPU scheduling**: Allocating physical CPU time to virtual CPUs (vCPUs) across all running VMs. This is conceptually similar to OS process scheduling, but operates at a different level of abstraction — the hypervisor schedules entire virtual processors, each of which in turn runs its own OS scheduler.
2. **Memory management**: Maintaining the mapping between each VM's view of physical memory (Guest Physical Address, GPA) and the actual host memory (Host Physical Address, HPA). This involves additional levels of address translation beyond what a standard OS performs.
3. **I/O mediation**: Multiplexing access to physical devices (NICs, disks, GPUs) across multiple VMs. This is often the most performance-critical responsibility, as I/O operations can dominate virtualization overhead.
4. **Isolation enforcement**: Guaranteeing that no VM can access another VM's memory, intercept its network traffic, or interfere with its CPU execution. Isolation is not optional — it is the security foundation of multi-tenant cloud computing.



### 9.2 Type 1: Bare-Metal Hypervisor

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

### 9.3 Type 2: Hosted Hypervisor

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

### 9.4 Type 1 vs Type 2 Comparison

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

## §10. I/O virtualization

Giving a guest OS access to a device — a network card, a disk, a GPU — is logically the same problem as giving it access to the CPU. The device was designed to be owned by a single operating system, which programs it directly via memory-mapped I/O (MMIO), port I/O, and Direct Memory Access (DMA) descriptors. Sharing the device among multiple guests requires the VMM to mediate access, and the same three-way trade-off applies: the VMM can do all the work (expensive), the guest can cooperate (fast but invasive), or the hardware can be extended to support virtualization directly (fastest but requires new silicon).

### 10.1 Four approaches at a glance

In practice, four implementation strategies have emerged for I/O virtualization, differing in performance, flexibility, and hardware requirements:

1. **Device emulation.** The VMM presents a software model of a physical device. Corresponds to binary translation.
2. **Paravirtual I/O (virtio).** Guest and host cooperate via a shared-memory ring buffer. Corresponds to paravirtualization.
3. **Passthrough.** A physical device is dedicated to a single guest via IOMMU-based address remapping. Corresponds to hardware-assisted virtualization.
4. **SR-IOV.** A single device is split by hardware into multiple virtual functions, each passed through to a different guest. A refinement of passthrough that adds multi-tenancy.

We now examine each in turn.

### 10.2 Device emulation

In device emulation, the VMM presents to the guest an interface indistinguishable from that of a real physical device. The guest's device driver — say, the Intel e1000 gigabit Ethernet driver, included in every Linux kernel — is unaware that it is running in a VM and programs the "card" exactly as it would program real hardware: writing to MMIO registers, posting DMA descriptors in shared memory, reading interrupt status bits.

The VMM, meanwhile, intercepts every MMIO access (each access is a trap, because the MMIO region is mapped in a way that causes page faults), reads the DMA descriptor rings from guest memory, performs the logically equivalent operation on a real device (often a different one — the VMM might emulate an e1000 but actually use a modern Intel X710 NIC on the host), and delivers virtual interrupts back to the guest.

The canonical implementation of this approach is **QEMU** [(QEMU, 2024)](#ref-qemu-docs), which emulates a wide zoo of devices — NICs, IDE/SCSI/NVMe disks, graphics cards, USB controllers, audio cards, parallel ports. When KVM is used with QEMU, the CPU virtualization is hardware-assisted (VT-x) but the device virtualization is typically emulation (unless explicitly configured otherwise).

The advantage of emulation is maximum compatibility. Any guest OS with a driver for the emulated device can run, without modification. This matters for Windows, for legacy operating systems, for network appliances sold as VM images, and for the general principle of supporting arbitrary workloads.

The disadvantage is performance. Each MMIO access triggers a VM-exit (on modern hardware) or a binary-translation hook (on old), and a typical network packet involves dozens of MMIO accesses in the guest driver — several to set up the descriptor, a doorbell write to tell the "hardware" the descriptor is ready, a status poll after completion. Each of these crosses the VMM boundary, accumulating overhead. The result is I/O throughput 10× to 100× slower than native. For a kernel build or a low-traffic web server this is tolerable; for HPC workloads or high-throughput networking, it is not.

> **Part I analogue.** Device emulation is binary translation applied to I/O: the VMM does all the hard work, at runtime, catching and re-interpreting each device interaction. Like binary translation, its virtue is universal compatibility and its vice is performance.

### 10.3 Paravirtual I/O: `virtio`

The paravirtualization approach to I/O, like its CPU counterpart, discards the pretence that the guest is unaware of being virtualized. Instead of emulating a specific physical device, the VMM and guest agree on a common interface — the `virtio` interface — designed from the ground up for efficient virtualized I/O [(Russell, 2008)](#ref-russell-2008). The `virtio` standard is maintained by OASIS and is openly published [(OASIS, 2022)](#ref-oasis-virtio).

The key structural idea is the **virtqueue**: a shared-memory ring buffer, mapped into both the guest's and the VMM's address spaces, through which requests and completions are communicated. The mechanics are straightforward:

1. The guest driver prepares a request descriptor — "read sector 1024 of the disk into this buffer," or "transmit this packet" — and inserts it into the ring.
2. The guest *notifies* the VMM that a new request is available. This notification typically requires a single VM-exit, though in high-throughput configurations the VMM can poll the ring and avoid even that.
3. The VMM (or a dedicated backend thread on the host) reads the descriptor, performs the corresponding operation on the real device, and places a completion in the ring.
4. The VMM signals the guest (via a virtual interrupt or via a polled memory location) that the completion is available.

Because multiple requests can be batched on each notification, and because the ring buffer minimises the number of boundary crossings per operation, `virtio` achieves performance an order of magnitude better than emulation. A single virtqueue notification can amortise tens or hundreds of queued requests.

`virtio` is used for every significant class of virtualized device:

- **virtio-net** for network interfaces;
- **virtio-blk** for block storage devices;
- **virtio-scsi** for SCSI-protocol block devices (more expressive than virtio-blk);
- **virtio-gpu** for graphics (newer);
- **virtio-crypto** for cryptographic accelerators.

All major Linux distributions ship with virtio drivers built into the kernel. Windows supports virtio via signed drivers distributed by the community (the "virtio-win" package). FreeBSD and macOS guests are similarly supported.

> **Part I analogue.** `virtio` is paravirtualization applied to I/O. The guest is modified — it uses virtio drivers rather than pretending its NIC is an e1000 — and in exchange it gets near-native performance at a fraction of the VMM complexity.

The practical consequence is that `virtio` has become the default for I/O in every KVM-based cloud, including OpenStack, Proxmox, and much of the AWS and Google Cloud Linux-guest infrastructure. It is the quiet workhorse of virtualized I/O.

### 10.4 Passthrough

For workloads where even `virtio` overhead is unacceptable — certain database servers, GPU compute, HPC network fabrics — the VMM can step out of the way entirely and let the guest talk to a physical device directly. This is called **passthrough**, sometimes also *device assignment*. The hardware mechanism that makes it safe is the **IOMMU** — Intel calls theirs VT-d, AMD calls theirs AMD-Vi — which is to DMA what the MMU is to CPU memory accesses [(Abramson et al., 2006)](#ref-vtd).

Without an IOMMU, passthrough would be impossible in a multi-tenant setting: a guest with direct device access could program the device's DMA engine to read or write arbitrary host memory, compromising every other guest and the VMM itself. The IOMMU prevents this by remapping DMA addresses through a per-device page table: when the passthrough device issues a DMA to "physical address" X, the IOMMU translates X according to the guest's IOMMU page table, ensuring the DMA can only touch memory assigned to that guest. Interrupts are similarly remapped: the device generates an interrupt, which the IOMMU routes to the guest that owns the device.

Passthrough gives near-native performance — typically indistinguishable from bare metal — because the fast path has no VMM involvement at all. The guest's device driver programs real hardware; the real hardware responds. The VMM is not on the critical path.

The price is rigidity. Because the device is owned exclusively by one guest, other guests cannot share it. A 10 Gbps NIC assigned to one VM is useless to the others. A GPU passed through to one VM cannot be shared with a second VM. Passthrough also breaks **live migration**: a running VM cannot be moved to a different host without losing access to the device, because the target host does not have that specific physical device.

In the HPC context, passthrough is the standard approach for GPUs. AWS's p4d and p5 instances, Azure's NDv5 instances, and Google Cloud's A3 machines all use passthrough for NVIDIA H100 and similar accelerators. For InfiniBand and high-speed NVMe, passthrough is also common, though often superseded by SR-IOV for multi-tenant settings.

> **Part I analogue.** Passthrough is the VT-x of I/O: the hardware cooperates, the VMM gets out of the way, the guest sees real hardware. The cost is the same as with VT-x: new silicon (the IOMMU), and a specific tenancy model (one device per guest).

### 10.5 SR-IOV

SR-IOV — **Single-Root I/O Virtualization** — is a refinement of passthrough that solves the multi-tenancy problem. Defined by the PCI-SIG as part of the PCI Express standard [(PCI-SIG, 2010)](#ref-sr-iov), SR-IOV allows a single physical device to present itself to the system as one **Physical Function (PF)** and multiple **Virtual Functions (VFs)**. Each VF appears as an independent PCIe device with its own configuration space, its own DMA context, and its own interrupts, and each can be passed through to a different guest via the IOMMU.

The multiplexing is done *inside the device itself*, by hardware the vendor has added to the NIC or other peripheral. The physical wire-side resources — the Ethernet MAC, the PCIe uplink, the InfiniBand port — are shared among the VFs according to scheduling policy enforced by hardware. Because there is no VMM on the data path, each VF achieves near-native performance. Because there are many VFs, many guests can be served.

A typical SR-IOV-capable NIC (such as Intel's X710 family or Mellanox/NVIDIA ConnectX) exposes between 64 and 128 VFs per physical port. This is enough for dense multi-tenant hosting.

SR-IOV's importance in the cloud cannot be overstated. AWS's **ENA** (Elastic Network Adapter) uses SR-IOV to give each EC2 instance a dedicated VF on a shared physical NIC, enabling 100+ Gbps per VM. AWS's **EFA** (Elastic Fabric Adapter) extends the idea to MPI-level communication, providing microsecond-scale latency suitable for HPC workloads by bypassing the kernel entirely. Azure's HBv3/HBv4 and HC series similarly use SR-IOV for their InfiniBand fabric. On Google Cloud, high-performance network offload uses a related approach based on hardware-offloaded virtual switching.

> **Part I analogue.** SR-IOV combines VT-x-style hardware assistance with hardware-internal multiplexing. It is the most aggressive application of the hardware-assisted idea — pushing virtualization into the device itself.

### 10.6 Side-by-side comparison

The four approaches to I/O virtualization span a clear Pareto frontier:

| | Emulation | virtio | Passthrough | SR-IOV |
|---|---|---|---|---|
| Part I analogue | Binary translation | Paravirtualization | Hardware-assisted | HW-assisted + HW multiplex |
| Guest modified? | No | Yes (virtio drivers) | No | No |
| Performance | 10–100× slower than native | ~10× better than emulation | Near-native | Near-native |
| Multi-tenancy | Yes | Yes | **No** (1 device : 1 VM) | Yes (1 device : N VMs) |
| HPC usability | Unsuitable | Adequate for general I/O | GPU, NVMe | Networking, InfiniBand |

The clean analogy with Part I is the payoff of this section: learn the CPU story in Part I, and the I/O story in Part II writes itself.

### 10.7 Why I/O matters for HPC

A brief aside on the relevance of all this to scientific computing. HPC workloads are I/O-intensive in very specific ways that determine the viability of cloud HPC offerings:

- **MPI jobs** need microsecond-scale inter-node latency. Traditional TCP/IP networking, even virtualized via virtio, is nowhere near this. Only kernel-bypass networking via SR-IOV (EFA, InfiniBand) can meet the latency budget.
- **GPU compute** workloads move tens of GB/s across PCIe. Emulated or paravirtualized GPU access is not feasible; passthrough is mandatory.
- **Parallel file systems** (Lustre, BeeGFS, GPFS) saturate multi-GB/s per node. Virtio-blk can carry this, but passthrough to the interconnect NIC is generally preferred.

The consequence is that **cloud HPC lives or dies by the quality of its I/O virtualization**. AWS HPC, Azure HB-series, and Google's HPC-optimised VMs all invest heavily in SR-IOV and passthrough specifically to reach the performance levels required. Virtualization that is "good enough" for a web server is often nowhere near "good enough" for MPI.

> **Self-check §9.**
> (a) Why does `virtio` outperform emulation by an order of magnitude? Identify the specific architectural reasons.
> (b) Why does passthrough break live migration?
> (c) If SR-IOV gives both multi-tenancy and near-native performance, why is passthrough still used for GPUs?

---

## §11. Storage virtualization

Storage virtualization is, at its core, the application of the same *indirection* principle we saw for memory and CPU to the block device. A *virtual disk* is a file (or a logical volume) on the host, exposed to the guest as if it were a physical block device. The VMM translates block-level operations from the guest's view to the host's view: when the guest reads sector $N$, the VMM maps $N$ to an offset within the backing file and issues the corresponding read to host storage. The principle is so simple that the interesting questions are not about the indirection but about the details: in what *format* is the virtual disk represented? How is space *provisioned*? Where does the data physically live?

### 11.1 Virtual disk formats

Multiple formats have emerged, each a product of its vendor's ecosystem:

**QCOW2** (QEMU Copy-On-Write, version 2) is the native format of the KVM/QEMU ecosystem. It supports *thin provisioning* (space is allocated only as it is written), *snapshots* (the entire disk state can be saved atomically and later reverted), *compression*, *encryption*, and *backing files* (a QCOW2 can be defined as "the delta from this base image"). Its feature set is the richest of the common formats, at the cost of some per-access overhead relative to raw block access. The format specification is open [(QEMU QCOW2 Specification, 2024)](#ref-qcow2-spec).

**VMDK** (Virtual Machine Disk) is VMware's format and the medium of choice in VMware ecosystems. It comes in several flavours (sparse, flat, stream-optimised for distribution) and supports snapshots. VMDK is also the format used inside the OVF/OVA packaging standard for portable VM distribution.

**VHD** and its successor **VHDX** are Microsoft's formats, used by Hyper-V and Azure. VHDX, introduced with Windows Server 2012, supports disks up to 64 TB, logs its metadata changes for resilience against power failure, and has a larger block size for better performance.

**RAW** is the simplest format: a byte-for-byte image of the block device, with no metadata whatsoever. RAW has no snapshots, no thin provisioning, and no feature set — but it has no per-access overhead either. When maximum performance matters and advanced features are not needed, RAW is the right choice.

The rule of thumb: rich formats win on features; RAW wins on throughput. For HPC workloads, especially for scratch data, RAW or direct passthrough to a high-performance file system usually dominates.

### 11.2 Thin vs thick provisioning

Independently of format, the VMM must decide how to allocate host storage for the virtual disk when the VM is created. Two strategies:

**Thick provisioning** reserves the full capacity on the host at VM creation time. If the VM is configured with a 100 GB virtual disk, 100 GB of host storage is allocated immediately — before the VM has written a single byte. The advantages are predictable performance (no allocation work on the write path), no risk of the host unexpectedly running out of storage mid-operation, and a simple operational model. The disadvantage is that unused space is *actually* unused: a VM that is provisioned at 100 GB but uses only 10 GB still consumes 100 GB on the host.

**Thin provisioning** allocates host storage only when the guest actually writes. A 100 GB virtual disk initially consumes almost nothing; as the guest writes, the backing file grows on demand. This enables **overcommitment**: a host with 1 TB of storage can safely host ten VMs with 200 GB virtual disks each, as long as the guests do not all fill their disks. In typical workloads, this allows dramatically better utilisation. The costs are a first-write penalty (each new block must be allocated before the write completes), the need for monitoring to prevent overcommitted hosts from running out of physical space, and some added metadata overhead.

Cloud providers universally use thin provisioning for general-purpose VMs (it is the only economically sensible choice at scale), but offer thick provisioning as an option for workloads that cannot tolerate unpredictable latency spikes.

### 11.3 Storage backends

Where do the actual bytes live? The hardware below the virtualization layer matters as much as the virtualization itself, especially for HPC.

- **Local disk** (direct-attached SSD/NVMe on the host) offers the lowest latency and highest IOPS. Used for scratch space, VM-ephemeral storage, and checkpointing.
- **NFS / NAS** provides file-level access over an IP network. Simple to operate, supports shared access, but limited in performance by the single-server NFS model.
- **iSCSI / Fibre Channel SAN** provides block-level access over a SAN fabric. Historically the enterprise HPC storage of choice; fast and reliable, but architecturally monolithic.
- **Ceph, GlusterFS, and other distributed object/block stores** scale out across many storage nodes. Ceph in particular has become a cornerstone of OpenStack deployments and is the backing store for much of the open-source cloud world.
- **Parallel file systems** such as **Lustre**, **BeeGFS**, and **IBM Spectrum Scale (GPFS)** are the HPC workhorse [(Lustre Documentation, 2024)](#ref-lustre). They stripe data across many storage servers and support hundreds of concurrent clients reading and writing at aggregate throughput of multiple GB/s per client. They are the default for large HPC deployments, from Tier-0 supercomputers to cloud HPC offerings.

The interaction with virtualization matters. For HPC workloads, the virtualization layer should stay as far out of the I/O path as possible: direct passthrough of an InfiniBand HCA to the guest, followed by a Lustre or BeeGFS client running inside the guest that talks to real storage over that HCA, is the canonical high-performance stack. Anything that inserts the VMM into the I/O fast path — emulation, virtio — is a potential bottleneck for tight HPC workloads, though virtio with multi-queue support can carry most general-purpose workloads adequately.

> **Self-check §10.**
> (a) Under what circumstances is thick provisioning preferable to thin?
> (b) Why would a cloud provider offer Lustre as a managed service rather than requiring customers to deploy it inside their own VMs?
> (c) QCOW2 supports snapshots by writing new data to a fresh location while keeping the original intact. What is the performance implication for write-heavy workloads?

---

## §12. Network virtualization

Network virtualization is a layered stack, each layer providing a distinct service. Unlike storage, which is essentially "virtualized block device," networks bring in additional concerns: tenant isolation across a shared fabric, layer-2 segmentation, overlay networks that cross subnets, and software-defined control planes. Modern cloud networking — AWS VPC, Azure VNet, GCP VPC — is built from this stack.

### 12.1 The virtualization stack

Five layers, each building on the previous:

**Layer 1: Virtual NICs (vNICs).** Each VM is given a virtual network interface that looks, to the guest OS, like an ordinary Ethernet card. The vNIC may be implemented as a `virtio-net` device (paravirtualized), as an emulated e1000, or as an SR-IOV Virtual Function passed through from real hardware. Regardless of the underlying implementation, the guest sees an Ethernet NIC with a MAC address.

**Layer 2: Virtual switches.** Inside the host, a software switch connects the vNICs to each other and to the physical uplink. The traditional Linux bridge is the simplest implementation; **Open vSwitch (OVS)** [(Open vSwitch, 2024)](#ref-ovs) is the industry standard for cloud-scale deployments, supporting OpenFlow, sophisticated flow tables, and hardware offload. The virtual switch is where packets between VMs on the same host travel, never reaching the wire.

**Layer 3: VLANs.** The IEEE 802.1Q standard adds a 12-bit VLAN tag to Ethernet frames, allowing a single physical network to be partitioned into up to 4094 logically separate Layer-2 networks. VLANs are used heavily in enterprise networking and within a single data-centre Layer-2 domain. Their limits are the 12-bit address space (4094 is not enough for a large cloud tenant population) and the fact that they cannot span Layer-3 boundaries (VLANs are confined to a single broadcast domain).

**Layer 4: Overlay networks (VXLAN and friends).** VXLAN — **Virtual eXtensible Local Area Network** [(RFC 7348, 2014)](#ref-vxlan) — addresses both limitations by *encapsulating* Layer-2 Ethernet frames inside UDP packets. An Ethernet frame from one VM is wrapped in a VXLAN header (with a 24-bit Virtual Network Identifier, VNI, giving 16 million separate networks), then in a UDP header, then in an outer IP header, and sent over the physical network to the host where the destination VM lives. The recipient unwraps the envelope and delivers the inner frame. Because the transport is UDP/IP, VXLAN traffic can cross Layer-3 routers — the overlay is independent of the underlay topology. The 24-bit VNI gives vastly more tenant IDs than VLAN's 12 bits.

**Layer 5: Software-defined networking (SDN).** The control plane — who can talk to whom, what routes exist, how traffic is shaped — is programmed via APIs rather than via manual configuration of individual switches. OpenFlow is the canonical SDN protocol [(McKeown et al., 2008)](#ref-openflow); the cloud providers have each developed their own, typically built on top of OVS or custom switches. SDN is what makes "create a subnet" a one-second API call rather than a week of network-team tickets.

### 12.2 VXLAN in one picture

VXLAN is the foundation of every modern cloud VPC, so it is worth understanding in detail. An encapsulated VXLAN packet has the following structure, from outermost to innermost:

1. **Outer Ethernet header** (host-to-host MAC addresses on the physical network).
2. **Outer IP header** (host-to-host IP addresses).
3. **UDP header** (destination port 4789 for VXLAN).
4. **VXLAN header** (8 bytes, containing the 24-bit VNI).
5. **Inner Ethernet frame** — the actual packet the guest wanted to send, with the guest's own source and destination MAC addresses.

The outer four headers are the *underlay*: standard IP networking between hosts. The inner frame is the *overlay*: a logically separate L2 network identified by the VNI. Different VNIs give different tenants isolation that is enforced by encapsulation — there is no physical way for a frame in one VNI to become visible in another, because the VNI is checked at the decapsulation point.

### 12.3 From VXLAN to cloud VPCs

A **Virtual Private Cloud (VPC)** is, in essence, a VXLAN overlay (or a close variant) plus an SDN control plane. On AWS, each VPC is identified by a tenant ID that is conceptually analogous to a VXLAN VNI (the actual encapsulation is custom rather than literal VXLAN on the wire, but the model is the same). Multiple VPCs coexist on the same physical network with complete Layer-2 and Layer-3 isolation. Subnets, routing tables, security groups — the entire logical topology of the VPC — are programmed via AWS API calls that eventually translate into flow rules on the SDN fabric.

On AWS specifically, the encapsulation work has been offloaded to the **Nitro cards** (§13), freeing the host CPU from the overhead and achieving wire-speed performance even for small packets. Azure's Accelerated Networking and Google's Andromeda stack do analogous things. Once you understand the VXLAN pattern, the architecture of every major cloud's virtual networking is essentially the same.

> **Self-check §11.**
> (a) Why can VLANs not support a cloud with millions of tenants?
> (b) What is the performance cost of VXLAN encapsulation, and how does hardware offload mitigate it?
> (c) Why is it important that the VXLAN overlay is independent of the underlay topology?

---

## §13. Performance considerations for HPC

Virtualization is not free. Every abstraction has a cost, and for HPC workloads — which often run at the edge of hardware capabilities — these costs can compound into significant slowdowns if not managed. The encouraging news is that the costs are all understood and individually addressable. With proper tuning, typical HPC workloads achieve **95–98% of bare-metal performance** inside a well-configured VM.

### 13.1 Where the overhead comes from

Five major sources, in order of typical impact:

**VMEXITs.** Every transition from non-root (guest) to root (VMM) mode costs hundreds to thousands of CPU cycles: the processor must save the guest's state, switch mode, and load the VMM's state. The more frequent the exits, the higher the overhead. Minimising VMEXITs is therefore a first-order optimisation target; it is why `virtio` batches requests, why SR-IOV avoids the VMM path entirely, and why `HLT` is handled specially on hosts with idle logic that can absorb its cost.

**Memory virtualization overhead.** With nested page tables (Intel EPT, AMD NPT), every TLB miss triggers a *two-dimensional* page walk: first walking the guest page tables to translate guest-virtual to guest-physical, then walking the host page tables to translate guest-physical to host-physical. On x86-64 with 4-level paging, the worst-case cost is 4×4 = 16 memory accesses per TLB miss, compared to 4 on bare metal. For memory-intensive HPC workloads, this can be substantial. The standard mitigation is **huge pages** (2 MB or 1 GB pages), which reduce TLB pressure dramatically.

**I/O emulation.** Pure emulation is, as we saw, 10–100× slower than native. The answer is to not use it: use `virtio` for general workloads, passthrough or SR-IOV for performance-critical ones.

**Scheduling effects.** The host scheduler may preempt a guest's vCPU and run something else — the VMM's own housekeeping, another VM, a host process. This is harmless for most workloads but deadly for tightly synchronous MPI workloads: a single vCPU delayed for 10 ms can stall the entire collective operation. The remedies are **CPU pinning** (bind each vCPU to a specific physical core) and **dedicated hosts** (no other VMs on the same machine).

**Interrupt delivery.** Virtual interrupts incur extra latency compared to physical ones: the VMM must inject them into the guest, which typically means a VM-entry, a handler run, and a return. Hardware features such as **posted interrupts** on VT-x reduce this cost by allowing the interrupt to be delivered without VMM intervention. Direct-assigned interrupts (with IOMMU interrupt remapping) are another remedy.

### 13.2 HPC-specific tuning

The concrete levers for HPC tuning are:

1. **CPU pinning and NUMA awareness.** Pin each vCPU to a specific physical core, and ensure that the guest's memory is allocated on the NUMA node local to those cores. Cross-NUMA memory access can cost 2×–3× in latency, and for MPI applications that heavily use shared memory, it is a major performance concern. Virtualization tools like libvirt expose CPU and memory pinning as configuration options; cloud providers expose them indirectly via instance types (an "HPC-optimised" instance typically has pinning and NUMA-awareness configured by default).

2. **Huge pages.** Use 2 MB pages for most memory (the kernel's Transparent Huge Pages do this by default on modern Linux) and 1 GB pages for workloads with very large working sets. The TLB benefit can be 5–15% on memory-bound codes, and much more on specific micro-benchmarks.

3. **SR-IOV for networking.** InfiniBand, RoCE, and AWS EFA all use SR-IOV; the guest gets a VF that bypasses the kernel's networking stack entirely. MPI latency drops from ~100 μs (with virtio) to ~5–15 μs (with SR-IOV) — often the difference between a usable HPC setup and an unusable one.

4. **GPU passthrough.** For ML training and scientific GPU compute, pass through a full GPU (or a Multi-Instance GPU slice) to the guest. No multi-tenancy on the GPU, but near-native GPU performance and full access to the vendor's driver stack (CUDA, ROCm, SYCL).

### 13.3 The verdict

With these levers applied, virtualized HPC performance is very close to bare metal. Published benchmarks from AWS, Azure, and Google, as well as independent measurements on internal clusters, consistently report 95–98% of native performance for typical HPC benchmarks including HPL, HPCG, and various MPI microbenchmarks. The remaining 2–5% is concentrated in the overhead sources above, and is the price paid for the enormous flexibility virtualization provides: elastic scaling, live migration (where compatible with passthrough), snapshots, image portability, multi-tenancy.

The practical conclusion: **virtualization is not free, but it is good enough**. The right attitude for an HPC practitioner is not "avoid virtualization" but "configure it correctly." Modern cloud HPC offerings (AWS HPC, Azure HBv3/v4, Google C3/C4) ship with the right defaults out of the box; the real risk is deploying a workload on a general-purpose instance type and assuming HPC-grade behaviour.

> **Self-check §12.**
> (a) Why does CPU pinning matter more for MPI than for embarrassingly parallel workloads?
> (b) If nested page tables cost 16 memory accesses per TLB miss, why don't we see HPC slowdowns of 4× on memory-bound codes?
> (c) What would be your first measurement to make, and why, if an HPC workload ran 20% slower in a VM than on bare metal?

---

## §14. A brief preview — modern cloud hypervisors

We close Part II with a brief look at two modern systems that will be the subject of Lecture 07. Both of them represent rethinkings of what a hypervisor is, triggered by the specific scale and security requirements of public clouds.

### 14.1 AWS Nitro

AWS's **Nitro system**, introduced in 2017 and rolled out across the EC2 fleet over the subsequent years, offloads most of the virtualization work — network, storage, system management — from the host CPU to custom silicon on the server board [(AWS, 2022)](#ref-aws-nitro). The host CPU is freed up to serve almost entirely as the guest's CPU, with the hypervisor itself reduced to a thin software layer coordinating the hardware components. Nitro is what makes AWS's "bare-metal" EC2 instances possible: a bare-metal instance is simply one where the guest OS runs with no hypervisor at all on the x86 CPU, while the Nitro cards still provide virtualized network, storage, and management. The virtualization overhead of Nitro-based instances is close to zero on the CPU path.

Nitro is also the foundation of AWS's networking performance: the VXLAN-like encapsulation of VPC traffic is handled on the Nitro card, not on the host CPU. This is why AWS can offer 100+ Gbps per instance without the host CPU being a bottleneck.

### 14.2 Firecracker

Firecracker, introduced by AWS in 2018 [(Agache et al., 2020)](#ref-firecracker), is a minimal KVM-based hypervisor — a *microVM* — written in Rust for security and safety. It supports only the devices needed for Lambda and Fargate workloads (virtio-net, virtio-blk, a serial console, a minimal set of control registers), and by omitting everything else it achieves boot times below 125 milliseconds and memory footprints under 5 MB per microVM. This combination — VM-level isolation with container-like density and startup time — is what makes serverless platforms at AWS scale possible.

Firecracker is open source and used beyond AWS: Kata Containers, OpenStack, and various experimental research systems are all built on it.

### 14.3 A pattern for the future

Both Nitro and Firecracker illustrate a broader trend. The original VMware model — a monolithic hypervisor providing the full classical VMM abstraction, running on commodity hardware — is giving way to *specialised* hypervisors, each optimised for a specific deployment niche: Nitro for dense multi-tenant clouds with hardware offload; Firecracker for serverless and short-lived VMs; KVM still for general-purpose and HPC. The common foundation remains the VT-x/VT-d hardware extensions and the architectural principles of Popek–Goldberg — but the implementations have fractured along application-specific lines. Understanding this landscape will be the work of Lecture 07.


---

## Appendix A - The problem: two operating systems on the same RAM

In a non-virtualized system, the mapping of memory is a negotiation between two parties: the application and the OS. The application issues virtual addresses; the OS maintains page tables that translate them to physical addresses; the hardware's Memory Management Unit (MMU) walks those page tables on every translation. The negotiation is well defined because there is exactly one physical address space, and one authoritative owner of it — the OS kernel.

Under virtualization, this neat picture breaks down. Consider two guest VMs running on the same physical host:

- VM A's guest OS maintains page tables mapping GVA to what it believes is physical memory. It allocates GPA 0 through, say, 4 GB.
- VM B's guest OS, running next to it, does exactly the same. It also allocates GPA 0 through 4 GB.
- But there is only one physical RAM. Both VMs' "GPA 0" cannot map to HPA 0. If they did, the two guests would overwrite each other's memory at every access.

The VMM's job is to reconcile these claims. It must produce, for each guest, the *illusion* that the guest owns a contiguous physical address space starting at 0, while actually distributing the real RAM in whatever way is convenient (possibly fragmented, possibly overcommitted). This requires a *second* level of indirection on top of the one the guest OS itself provides.

> **Intuition.** Memory virtualization turns the OS's single-level page table into a two-level affair. The guest OS does its usual work — mapping its applications' virtual addresses to what it thinks is physical memory. The VMM then does a second, invisible translation — from what the guest thinks is physical to what actually is. The guest has no idea the second translation is happening.

This second translation is the central topic of this appendix. Everything else — shadow page tables, EPT, ballooning, memory deduplication — is a technique for implementing it efficiently or for managing its consequences.

### What we want from memory virtualization

Four requirements frame the design of any memory virtualization scheme:

1. **Equivalence** (a Popek–Goldberg requirement, specialised to memory). The guest's view of its memory must be indistinguishable from what it would see on bare hardware. In particular, the guest's own page tables must still work — the guest OS must be able to map its applications' virtual addresses to what it believes is physical memory, and these mappings must behave correctly.

2. **Isolation** between VMs. One guest must not be able to read or write the memory of another guest, regardless of what GPA values it programs into its page tables. This is the memory analogue of Popek–Goldberg's safety requirement.

3. **Performance**. A TLB miss in a normal OS costs ~4 memory accesses (walking a 4-level page table on x86-64). Memory virtualization should not cost orders of magnitude more than this.

4. **Flexibility**. The VMM should be able to manipulate physical memory on its own terms — remapping, overcommitting, deduplicating, reclaiming — without breaking the guest's illusion.

Each of the techniques in this appendix trades off differently along these four axes.

---

## Recap: single-level paging

Before we can double the indirection, we need to be clear on what a single level of indirection looks like. On x86-64 running ordinary Linux (no virtualization), the story is:

1. The process issues a virtual address, 64 bits wide (of which the low 48 are used in canonical mode).
2. The MMU splits the address into 5 parts: four 9-bit fields that select entries in successive levels of a 4-level page table (PML4, PDPT, PD, PT), and a 12-bit offset into a 4 KB page.
3. The CPU's **CR3 register** holds the physical address of the top-level page table (PML4). The MMU reads the PML4 entry selected by the top 9 bits; that entry's contents give the physical address of the next-level table (PDPT), and so on.
4. After four memory accesses, the MMU has a **page table entry (PTE)** in the bottom-level table (PT). The PTE contains the physical frame number (PFN) corresponding to the virtual page, plus some bits for permission and caching.
5. The physical frame number, concatenated with the 12-bit offset, is the physical address the CPU issues on the memory bus.

This 4-memory-access walk happens on every TLB miss. The Translation Lookaside Buffer (TLB) caches the most recent results, so in steady state most memory accesses do not pay the full walk cost — only a handful of cycles to look up the TLB. But when the TLB misses, 4 memory accesses is the price.

> **Formally.** In a non-virtualized system, the address translation function is $T: GVA \to GPA$, where here we use GPA to mean simply "the CPU's physical address." The function $T$ is defined by the page tables maintained by the single OS running on the bare hardware.

Hold this picture clearly in mind. The virtualized case is the same picture, played twice.

---

## The two-level indirection

In a virtualized system, the address translation becomes a composition of two functions:

$$T_{guest}: GVA \to GPA \quad\quad T_{host}: GPA \to HPA$$

The guest OS maintains $T_{guest}$ through its own page tables, exactly as a non-virtualized OS would. The VMM maintains $T_{host}$. The complete translation from a guest virtual address to a host physical address is

$$GVA \xrightarrow{T_{guest}} GPA \xrightarrow{T_{host}} HPA$$

The guest is entirely unaware that $T_{host}$ exists. From its perspective, it owns the physical memory; what it calls "physical address" really is physical. The VMM's job is to maintain this illusion — which means making the composed translation happen in a way the guest cannot distinguish from a single-level translation.

The question that organises the rest of this appendix is: **how is this composition actually computed?** There are two historically significant answers, which mirror the two CPU-virtualization responses we saw in Lecture 03.

The first answer is a pure software workaround, invented in the late 1990s when x86 lacked hardware support. It is called **shadow page tables**. The VMM intercepts every modification the guest makes to its own page tables, computes the composed translation $T_{host} \circ T_{guest}$ directly, and stores the result in *shadow* page tables that the CPU actually walks. This is analogous to binary translation: the VMM does all the work, at runtime, in software.

The second answer is a hardware extension introduced by Intel (2008) and AMD (2007–2008) that allows the CPU itself to walk both levels of page tables. The Intel implementation is called **Extended Page Tables (EPT)**; the AMD equivalent is **Nested Page Tables (NPT)**. Here the CPU does the composition in hardware, at the cost of more memory accesses per TLB miss. This is analogous to VT-x: the hardware is extended to do natively what the VMM had to simulate.

We also mention in passing a third approach — **paravirtualized memory management**, as used by early Xen — where the guest OS is modified to call the VMM explicitly whenever it wants to modify its page tables. This was a workaround for the software cost of shadow page tables on early CPUs without VT-x. It has been largely superseded by EPT/NPT, though traces of it linger.

The two figures that accompany §A.4 (shadow) and §A.5 (EPT/NPT) should be studied carefully; they are more informative than any amount of prose.

> **Self-check §A.3.**
> (a) Why can the guest not be allowed to program the real CR3 register directly?
> (b) If the guest writes to one of its own page table entries, what must happen for the translation to remain consistent?
> (c) Given GVA, GPA, and HPA, which translation is the guest OS aware of, and which is the VMM's responsibility?

---

## Shadow page tables: the software workaround

When the first x86 VMMs were written — VMware in the late 1990s, Xen in the early 2000s — the hardware had no support for the two-level translation we described in §A.3. The CPU knew how to walk exactly one set of page tables, the ones pointed to by CR3. The VMM had to make this single hardware mechanism implement a logically two-level translation.

The technique they developed is called **shadow page tables**. The idea is to have the VMM maintain a separate set of page tables — the *shadow* — that directly map GVA to HPA, skipping the GPA intermediate. The CPU's CR3 points to the shadow, not to the guest's own page tables. The guest's tables are maintained (the guest believes they are active) but are not consulted by the hardware.

### How shadow tables work

At any moment, the VMM holds two things: the guest's own page tables (maintained by the guest OS, believed by the guest to be active), and the shadow page tables (actually used by the CPU). For every GVA the guest has mapped, the shadow contains an entry giving the corresponding HPA directly, computed by applying both the guest's mapping GVA → GPA and the VMM's own mapping GPA → HPA and composing them.

The hard part is keeping the shadow synchronised with the guest's own tables. If the guest modifies its page tables — adding a mapping when a process allocates memory, removing one when a process exits, changing permissions — the shadow must be updated to match. Otherwise the illusion breaks: the guest believes some memory is mapped, but the CPU, looking at the stale shadow, says otherwise.

The standard technique for keeping in sync is called **write protection**. The VMM marks the pages containing the guest's page tables as read-only in the shadow. When the guest tries to modify one of its page table entries, the write causes a page fault (because the shadow says the page is read-only). The page fault is delivered to the VMM, which inspects the attempted write, updates the shadow accordingly, and returns to the guest — which perceives the write as having succeeded.

This is the heart of shadow page tables: every write to a guest page table entry becomes a VM-exit. In return, the CPU walks the shadow at full speed on TLB misses — 4 memory accesses, exactly like bare metal.

*[See figure **FM2 — Shadow page tables** for the architectural diagram.]*

### The trade-offs

Shadow page tables work, and they worked well enough to power VMware and Xen for the better part of a decade. But their performance profile is distinctive.

**Wins.** On the *translation* fast path (TLB miss), shadow page tables are as fast as bare metal — 4 memory accesses per miss on x86-64, no overhead. For workloads whose memory footprint is stable (few page table modifications after startup), this is excellent.

**Losses.** Every guest page table modification is a VM-exit, which costs hundreds to thousands of cycles. Workloads that frequently modify the page tables — forking processes, `mmap`/`munmap`-heavy workloads, just-in-time compilers that install new mappings dynamically — pay a large overhead. Database workloads with rapid connection churn and memory allocation were particularly hurt.

A further cost is *space*: the VMM must maintain a shadow per-process-per-guest. If the guest runs 100 processes, the VMM maintains 100 shadows for that guest alone. When the guest switches processes (changes its notion of CR3), the VMM must switch shadows too; the old shadow's TLB entries become irrelevant.

The final cost, less appreciated in textbook treatments but significant in practice, is *complexity*. The VMM logic for maintaining shadows correctly — handling every corner of the x86 paging architecture, including access/dirty bits, global pages, huge pages, and shadow updates under concurrent guest activity — is notoriously intricate. The VMware and Xen engineers of the 2000s could tell long stories about subtle bugs.

### Why shadow page tables were superseded

By 2007–2008, it was clear that the costs were unacceptable for the workloads being targeted. The cloud was starting to take shape, and cloud providers ran heterogeneous workloads with unpredictable page-table behaviour. A workaround that worked well for static enterprise servers struggled to scale to dynamic cloud-scale deployments.

The industry's response was to fix the problem in hardware, as it had done with the CPU side of Popek–Goldberg. The result was EPT and NPT.

> **Self-check §A.4.**
> (a) Why does the VMM mark guest page tables as read-only in the shadow, rather than intercepting writes some other way?
> (b) Consider a guest running 50 processes. How does the memory cost of shadow page tables compare to a non-virtualized OS running the same 50 processes?
> (c) Would shadow page tables work in an environment where the guest OS is malicious (actively trying to break the illusion)?

---

## EPT and NPT: hardware support for two-level walks

Intel introduced **Extended Page Tables (EPT)** with the Nehalem microarchitecture (2008); AMD's **Nested Page Tables (NPT)** — also historically called **Rapid Virtualization Indexing (RVI)** — appeared in 2007 with the Barcelona core. The architectures are different in detail but structurally identical: both extend the CPU's MMU to walk *two* sets of page tables in hardware, directly implementing the composition $T_{host} \circ T_{guest}$ we discussed in §A.3.

### The architectural change

The change is conceptually simple. The CPU gains a second CR3-like register that points to the nested page tables (the EPT or NPT) maintained by the VMM. When the guest is running (in non-root mode, per VT-x), the MMU consults *both* sets of tables on each TLB miss. The guest's own tables are the ones pointed to by the guest's (virtualized) CR3; the VMM's nested tables are pointed to by a field in the VMCS set up during VM-entry.

The CPU walks them in a specific order: to translate a single GVA to an HPA, the MMU walks the guest's 4-level table, but every memory access required by that walk *must itself be translated through the nested table*. The result is that what was once a simple 4-step walk on bare metal becomes a 2-dimensional walk on virtualized hardware.

### The 2D walk in detail

Imagine the guest issues a virtual address $V$. On a TLB miss, the MMU must compute the HPA. It proceeds as follows:

1. Look up the address of the guest's top-level page table (PML4). This is held in the guest's virtualized CR3 — which contains a GPA, not an HPA. To actually read the PML4, the MMU must first translate that GPA to an HPA, which takes a walk through the nested page tables (4 memory accesses on x86-64).
2. Read the guest PML4 entry selected by the top 9 bits of $V$. The entry contains a GPA pointing to the next-level guest page table (PDPT). To read the PDPT, the MMU again walks the nested tables — another 4 accesses.
3. Repeat for the next two levels of the guest page table walk.
4. Finally, the bottom-level guest PTE contains a GPA — the "guest physical address" the guest believes it is accessing. Translate this final GPA to an HPA through the nested tables (one last 4-access walk).

Counting memory accesses: the guest's own 4-level walk requires 4 reads (of guest page table entries). But each of those 4 reads is itself a GPA access that needs translation, costing 4 more accesses. Plus one final translation of the bottom-level GPA. **Total: 4 × 4 + 4 = 20 memory accesses in the worst case**, though a common simplified analysis gives 4 × 4 = 16.

This is a dramatic increase over the 4 accesses of bare metal. Yet EPT/NPT consistently outperforms shadow page tables in practice. Why?

*[See figure **FM3 — EPT/NPT** for the architectural diagram and the 2D walk breakdown.]*

### Why EPT/NPT wins despite the higher miss cost

Several factors conspire to make hardware-assisted memory virtualization the better choice in practice.

**No VM-exits on page table modifications.** This is the biggest win. Under shadow page tables, every guest page table write triggers a VM-exit. Under EPT/NPT, the guest modifies its own page tables freely, at full CPU speed — because those modifications only affect the guest's level of the walk, which the VMM does not need to synchronise anything against. The nested tables, maintained by the VMM, describe a different translation (GPA → HPA) that changes only when the VMM itself rearranges the guest's memory, which is rare.

**The TLB absorbs the cost.** Modern CPUs have large TLBs (hundreds of entries for normal pages, dozens for huge pages) and sophisticated prefetching. In steady state, most memory accesses hit in the TLB and incur no walk cost at all. The 16-access worst case applies only to cold misses.

**Huge pages drastically reduce miss rates.** Using 2 MB or 1 GB pages reduces the number of page table entries needed to cover a given address range by 512× or 262144× respectively. Fewer PTEs means fewer TLB entries needed, means higher hit rates, means fewer 2D walks. For HPC workloads with large working sets, the benefit is substantial.

**Simpler VMM.** The VMM no longer maintains shadows, no longer needs to write-protect guest page tables, no longer needs to handle the intricate cases of concurrent modification. The EPT/NPT code paths in modern KVM are a fraction of the size of the shadow-page-table code they replaced.

**Better isolation properties.** The VMM's nested tables are separate per-VM data structures; a bug in the guest's page table handling cannot propagate into the VMM's. Under shadow page tables, the VMM's shadows were a complex function of guest state, which was a security concern.

The empirical result is that, on workloads with stable memory mappings, EPT/NPT is slightly slower than shadow page tables (because of the higher per-miss cost). On workloads with rapid mapping churn — which includes most real cloud workloads — EPT/NPT is substantially faster, because it avoids the per-modification VM-exit.

### The paravirtualized alternative, briefly

Early Xen, running on pre-VT-x hardware, took a third path: **paravirtualized memory management**. The guest OS was modified to know it was virtualized, and to make explicit hypercalls to the VMM whenever it wanted to modify its page tables. Rather than trapping on write-protected page table updates, the guest cooperated.

This avoided the #PF-per-write overhead of shadow page tables, at the cost of requiring a modified guest. Like paravirtualized CPU virtualization, it was eventually superseded by hardware-assisted virtualization. But it is worth remembering as the third point of the triangle: whenever you encounter a virtualization problem, the three options of *software workaround / guest cooperation / hardware extension* reappear. Memory is no exception.

> **Self-check §A.5.**
> (a) Why is the 2D walk potentially 16 memory accesses, and when is it actually that many?
> (b) Under what conditions would shadow page tables outperform EPT on a modern CPU?
> (c) EPT was introduced in 2008, two years after VT-x (2006). What does this suggest about the relative difficulty of the two problems?

---

## Memory overcommitment and ballooning

So far we have treated $T_{host}: GPA \to HPA$ as if it were a static, one-to-one function: for each GPA in a VM, there is a dedicated HPA somewhere in real RAM. But a major benefit of virtualization is *overcommitment*: the VMM can promise more total memory to its VMs than the host physically has, betting that not all of them will use their full allocation simultaneously. Three techniques make this possible, each worth understanding briefly.

### Ballooning

The classical technique, invented by VMware's Carl Waldspurger in 2002 [(Waldspurger, 2002)](#ref-waldspurger), is **memory ballooning**. The VMM installs a cooperative driver — the *balloon driver* — inside the guest OS. When the host starts running low on memory, the VMM tells the balloon driver to "inflate": allocate a specified amount of memory from the guest OS, through the guest's ordinary allocation APIs.

The guest OS, responding to the allocation request, uses its normal mechanisms to free up memory — evicting page cache, swapping out idle anonymous pages, reclaiming slab caches. The balloon driver, having obtained this memory, reports the GPAs it holds to the VMM. The VMM unmaps those GPAs from host physical memory and reassigns the physical pages to another VM or to itself.

Later, when memory pressure eases, the VMM tells the driver to "deflate": release its holdings. The driver returns them to the guest OS, which sees its available memory grow.

The elegance of ballooning is that the **guest OS makes the eviction decisions**. The guest has better information than the VMM about which pages are hot and which are cold, and its own memory-management subsystem (the kernel's page reclamation logic) is designed exactly for this purpose. The VMM merely asks for memory; the guest decides which memory to release.

Ballooning is a paravirtualization technique: it requires a modified guest (specifically, an installed balloon driver), but most operating systems today ship with one. On KVM, the modern implementation is **virtio-balloon**, part of the virtio family we saw in Lecture 03 Part II.

*[See figure **FM4 — Ballooning** for the mechanism diagram.]*

###  Kernel Same-page Merging (KSM)

A second overcommitment technique exploits memory *redundancy across VMs*. Consider a cloud host running 50 Linux VMs. Each VM has essentially the same `/bin/bash`, the same `libc`, the same systemd, the same kernel text. In principle, these identical pages could be shared among all 50 VMs — 50 copies collapsed to one.

**Kernel Same-page Merging (KSM)** scans guest memory looking for identical pages (based on cryptographic hashes for efficiency) and collapses duplicates into a single copy, marking them copy-on-write. When any guest attempts to modify a shared page, the VMM transparently allocates a fresh copy for that guest, preserving isolation.

KSM works on Linux/KVM since ~2009. On dense hosts with many similar VMs, it can reclaim 20–30% of total memory — substantial overcommit headroom.

###  Swapping

The crudest overcommitment technique is simply **swapping** guest pages to disk, as a host-level OS would swap a user-space process. When the VMM needs memory and the balloon driver cannot release enough, the VMM picks some guest page and writes it to a host-level swap file. When the guest next accesses that page, a page fault brings it back.

This works but is very painful. The guest has no idea its page has been swapped; from its perspective, the page is "in RAM" and should respond in nanoseconds. An access that turns into a disk I/O turns that nanosecond access into milliseconds. If this happens frequently — "double swapping," where both the guest and the host are swapping — performance collapses.

Host-level swapping is therefore the overcommitment mechanism of last resort. Production cloud platforms generally avoid it entirely, relying on ballooning and KSM for predictable overcommitment and provisioning carefully enough that swapping never kicks in.

### practical note: cloud providers and overcommit

Public cloud providers are careful about memory overcommitment. AWS, Azure, and Google Cloud generally do **not** overcommit memory on production instances — when you rent an instance with 32 GB of RAM, you get 32 GB of real, dedicated HPA. The rationale is predictability: cloud customers expect consistent performance, and overcommitment introduces unpredictable latency spikes when ballooning or swapping activates.

Where overcommitment *is* used at cloud scale, it is typically on burstable or discounted instance families, or in internal platform services where the provider itself is the customer. Knowing which of your instances are on overcommitted hosts and which are not is useful diagnostic information when investigating unexplained latency spikes.

> **Self-check §A.6.**
> (a) Why does the balloon driver release memory to the VMM by allocating rather than by some direct mechanism?
> (b) Under what conditions would KSM fail to provide benefit?
> (c) Why do cloud providers avoid memory overcommit for production customer instances?

---

## Bridge to Lecture 04: why containers do not do this

We close this appendix with a forward-pointer. Lecture 04 will discuss **containers** — a different kind of workload isolation, widely used for cloud-native and microservices workloads, and increasingly visible in HPC (Apptainer, Sarus, Shifter). A natural question is: do containers need memory virtualization?

The answer is *no*, and it explains a great deal about why containers are "lighter" than VMs.

A container is not a separate OS. It is a collection of processes running on the **host kernel**, isolated from other containers through Linux namespaces (for what they can see) and cgroups (for how much they can use). There is exactly one operating system on the machine — the host — and all containers' processes are processes under that one OS.

Because there is only one OS, there is only one page table hierarchy. Each containerised process has its own virtual address space, mapped to host physical memory through the ordinary, single-level page tables the host kernel maintains. No GPA intermediate, no nested page tables, no shadow tables, no ballooning. The MMU walks at the same cost as for any non-containerised process — 4 memory accesses on a TLB miss, no overhead.

This is why containers can achieve near-zero memory virtualization overhead: there is no memory virtualization. What containers give up, in exchange, is the isolation strength of VMs: a kernel vulnerability exploited in one container can compromise the host and therefore all other containers, because they share the same kernel. VMs, by contrast, provide strong isolation at the cost of the entire memory-virtualization apparatus we have spent this appendix describing.

This trade-off — **performance vs isolation** — is the fundamental axis along which VMs and containers differ. Understanding the machinery of memory virtualization is what lets you appreciate why that trade-off exists and why it cannot be easily eliminated.

---

## Concluding remarks

Memory virtualization spans twenty-five years of systems engineering, from Waldspurger's 2002 ballooning paper through Intel's 2008 EPT and up to today's careful cloud provisioning. Its central theme is the same one we saw for CPU virtualization in Lecture 03: a problem that begins as a software workaround (shadow page tables), gets partially addressed by guest cooperation (paravirtualized memory), and is finally solved by hardware support (EPT/NPT).

The practical consequences matter for anyone running workloads on virtualized infrastructure:

- **Use huge pages** for memory-intensive applications. The reduction in TLB miss rate translates directly to reduced 2D walk cost on virtualized hosts. This is first-line advice for HPC in the cloud.
- **Be aware of overcommitment** on your instances. Check your cloud provider's documentation for the specific instance family you are using. Public cloud production instances generally do not overcommit, but internal platforms may.
- **Containers avoid memory virtualization overhead** precisely because they avoid the second level of indirection. This is not a free lunch — the cost is reduced isolation — but for trusted workloads it is a substantial win.
- **Monitor your TLB miss rate** if performance matters. On Linux, `perf stat` with `dTLB-load-misses` and `iTLB-load-misses` gives direct visibility.

The next lecture will apply this framework to containers, where the absence of memory virtualization is itself the defining feature.

---


# Appendix B — Glossary

**Binary translation.** A virtualization technique in which the VMM, at runtime, rewrites the guest's machine code in chunks before execution, replacing problematic (sensitive but non-privileged) instructions with safe equivalents. Pioneered by VMware.

**Behaviour-sensitive.** Of an instruction: one whose semantics depend on the machine's control state (e.g., on the privilege bit or memory-virtualization registers) even if it does not explicitly read or write that state. Contrast with *control-sensitive*.

**Control-sensitive.** Of an instruction: one that reads or writes the machine's control state directly.

**Deprivileging.** The practice of running the guest operating system in user mode (rather than the kernel mode for which it was written), so that its privileged instructions trap to the VMM.

**Direct execution.** The design choice of running guest instructions directly on the physical CPU (as opposed to interpreting them in software).

**DMA (Direct Memory Access).** A hardware mechanism by which a peripheral device can read or write host memory without CPU involvement.

**EFA (Elastic Fabric Adapter).** AWS's high-performance networking adapter, designed for MPI-grade low-latency communication, implemented as an SR-IOV NIC with kernel-bypass messaging.

**ENA (Elastic Network Adapter).** AWS's standard high-performance network adapter for EC2 instances, using SR-IOV.

**EPT (Extended Page Tables).** Intel's hardware support for nested page tables, allowing the VMM to avoid software-managed shadow page tables. AMD's equivalent is NPT.

**Equivalence.** The first of Popek and Goldberg's three VMM requirements: software running in the VM must behave as it would on bare hardware, modulo timing and resource limits.

**Firecracker.** A minimal KVM-based hypervisor developed by AWS for Lambda and Fargate, emphasising security and fast boot time.

**Guest.** The operating system (and its applications) running inside a VM.

**HLT.** An x86 instruction that halts the CPU until the next interrupt. Privileged and sensitive — the canonical example of a well-behaved instruction under trap-and-emulate.

**Host.** The physical machine and the software running directly on it (the VMM and its ancillary processes), as opposed to the guests.

**Huge pages.** Memory pages of 2 MB or 1 GB (on x86-64), used to reduce TLB pressure, particularly valuable in virtualized environments with nested page tables.

**Hypercall.** A call by a paravirtualized guest to the VMM, analogous to a system call from an application to the OS.

**Hypervisor.** Synonym for VMM.

**IOMMU.** Input-Output Memory Management Unit. Hardware that remaps DMA addresses and interrupts, allowing safe device passthrough to guests. Intel VT-d and AMD-Vi are implementations.

**KVM (Kernel-based Virtual Machine).** The open-source hypervisor merged into the Linux kernel in 2008, leveraging VT-x/AMD-V for CPU virtualization.

**Nitro.** AWS's modern EC2 hypervisor system, offloading most virtualization work to dedicated hardware cards.

**NPT (Nested Page Tables).** AMD's hardware support for two-level page tables; equivalent to Intel's EPT.

**Paravirtualization.** A virtualization technique in which the guest OS is modified to cooperate explicitly with the VMM via hypercalls, rather than relying on trap-and-emulate.

**Passthrough.** Dedicating a physical device (via IOMMU) to a single guest for near-native performance, at the cost of multi-tenancy.

**Popek–Goldberg theorem.** The 1974 result stating that a trap-and-emulate VMM for an ISA is constructible iff every sensitive instruction is privileged.

**popf.** An x86 instruction that pops flags from the stack into EFLAGS. Canonical example of a sensitive-but-non-privileged instruction: the source of x86's virtualization difficulties pre-2006.

**Privileged instruction.** One that traps when executed in user mode. A hardware property fixed by the ISA.

**QCOW2.** QEMU's native virtual-disk format, supporting thin provisioning, snapshots, and compression.

**QEMU.** The leading open-source emulator, used both standalone (for cross-architecture emulation) and as the device-emulation backend for KVM.

**Ring buffer / virtqueue.** A shared-memory data structure used by virtio for communicating I/O requests and completions between guest and VMM.

**Root / non-root mode.** The orthogonal privilege axis introduced by Intel VT-x: the VMM runs in root mode, the guest in non-root mode.

**Safety (resource control).** The second of Popek and Goldberg's three requirements: the VMM retains complete control over physical resources.

**Sensitive instruction.** One whose behaviour depends on or modifies the machine's control state. A semantic property.

**SR-IOV (Single-Root I/O Virtualization).** A PCIe extension allowing a single physical device to expose multiple Virtual Functions, each passed through to a different guest.

**Trap-and-emulate.** The canonical VMM discipline: let the guest run directly, and on each privileged instruction (trap), emulate the instruction in the VMM.

**VF (Virtual Function).** One of the virtual device instances exposed by an SR-IOV-capable device.

**VM (Virtual Machine).** The abstract machine presented to a guest OS by the VMM.

**VMCS (Virtual Machine Control Structure).** The Intel VT-x data structure controlling which instructions and events cause VM-exits.

**VM-exit / VM-entry.** The hardware-assisted transitions between non-root (guest) and root (VMM) mode.

**VMM (Virtual Machine Monitor).** Synonym for hypervisor.

**virtio.** The open paravirtualization standard for virtualized I/O devices.

**VT-d.** Intel's IOMMU implementation, enabling safe device passthrough.

**VT-x.** Intel's hardware-assisted virtualization extensions for the x86 CPU, introduced in 2006.

**VXLAN (Virtual eXtensible LAN).** An overlay-networking standard that encapsulates Layer-2 Ethernet frames in UDP, with a 24-bit VNI, enabling cloud-scale multi-tenant networks.

**VPC (Virtual Private Cloud).** A logically isolated section of a cloud provider's network, typically implemented as a VXLAN-like overlay plus SDN.

**Xen.** The open-source hypervisor developed at Cambridge, originator of paravirtualization on x86.

---

# Appendix C — Further reading and references

The literature on virtualization is vast. The list below emphasises authoritative primary sources and highly-cited surveys; it is a starting point, not an exhaustive bibliography.

### Foundational papers

<a id="ref-popek-goldberg"></a>
**Popek, G. J. & Goldberg, R. P. (1974).** *Formal Requirements for Virtualizable Third Generation Architectures.* Communications of the ACM, 17(7), 412–421. The paper that founded the field. Worth reading in the original: it is concise, precise, and still instructive fifty years later. DOI: [10.1145/361011.361073](https://doi.org/10.1145/361011.361073).

<a id="ref-creasy-1981"></a>
**Creasy, R. J. (1981).** *The origin of the VM/370 time-sharing system.* IBM Journal of Research and Development, 25(5), 483–490. A first-hand account of CP-40/CP-67 and VM/370 by one of their architects.

<a id="ref-robin-irvine"></a>
**Robin, J. S. & Irvine, C. E. (2000).** *Analysis of the Intel Pentium's Ability to Support a Secure Virtual Machine Monitor.* Proceedings of the 9th USENIX Security Symposium. Identifies the 17 sensitive-but-non-privileged x86 instructions — a definitive empirical confirmation that x86 fails the Popek–Goldberg condition.

<a id="ref-barham-2003"></a>
**Barham, P., Dragovic, B., Fraser, K., Hand, S., Harris, T., Ho, A., Neugebauer, R., Pratt, I., & Warfield, A. (2003).** *Xen and the Art of Virtualization.* Proceedings of the 19th ACM Symposium on Operating Systems Principles (SOSP), 164–177. The paper that introduced Xen and popularised paravirtualization.

<a id="ref-kivity-2007"></a>
**Kivity, A., Kamay, Y., Laor, D., Lublin, U., & Liguori, A. (2007).** *KVM: the Linux Virtual Machine Monitor.* Proceedings of the Linux Symposium, Ottawa. The original announcement of KVM.

<a id="ref-russell-2008"></a>
**Russell, R. (2008).** *virtio: towards a de-facto standard for virtual I/O devices.* ACM SIGOPS Operating Systems Review, 42(5), 95–103. The paper that laid out the virtio design principles.

<a id="ref-agache-2020"></a><a id="ref-firecracker"></a>
**Agache, A., Brooker, M., Iordache, C., Liguori, A., Neugebauer, R., Piwonka, P., & Popa, D. (2020).** *Firecracker: Lightweight Virtualization for Serverless Applications.* Proceedings of the 17th USENIX Symposium on Networked Systems Design and Implementation (NSDI), 419–434. The definitive Firecracker paper.

<a id="ref-barroso-2007"></a>
**Barroso, L. A. & Hölzle, U. (2007).** *The Case for Energy-Proportional Computing.* IEEE Computer, 40(12), 33–37. The classic reference for the "10–15% CPU utilisation" figure on pre-virtualization servers.

<a id="ref-mckeown-2008"></a><a id="ref-openflow"></a>
**McKeown, N., Anderson, T., Balakrishnan, H., Parulkar, G., Peterson, L., Rexford, J., Shenker, S., & Turner, J. (2008).** *OpenFlow: Enabling Innovation in Campus Networks.* ACM SIGCOMM Computer Communication Review, 38(2), 69–74. The foundational SDN paper.

### Books and comprehensive surveys

**Smith, J. E. & Nair, R. (2005).** *Virtual Machines: Versatile Platforms for Systems and Processes.* Morgan Kaufmann. The standard textbook on the subject; thorough, careful, and still the best long-form treatment of process VMs alongside system VMs.

**Bugnion, E., Nieh, J., & Tsafrir, D. (2017).** *Hardware and Software Support for Virtualization.* Synthesis Lectures on Computer Architecture, Morgan & Claypool. A modern, architecture-focused monograph; excellent for the hardware side of the story.

**Tanenbaum, A. S. & Bos, H. (2023).** *Modern Operating Systems* (5th ed.). Pearson. Chapter 7 covers virtualization at textbook depth, with good exposition of CPU, memory, and I/O virtualization.

### Specifications and official documentation

<a id="ref-intel-sdm"></a>
**Intel Corporation.** *Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3: System Programming Guide.* Available online at [intel.com/sdm](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html). Chapter 23 onwards documents VT-x in full. The authoritative reference for the hardware.

<a id="ref-vtd"></a>
**Abramson, D., Jackson, J., Muthrasanallur, S., Neiger, G., Regnier, G., Sankaran, R., Schoinas, I., Uhlig, R., Vembu, B., & Wiegert, J. (2006).** *Intel Virtualization Technology for Directed I/O.* Intel Technology Journal, 10(3). Original description of VT-d.

<a id="ref-sr-iov"></a>
**PCI-SIG (2010).** *Single Root I/O Virtualization and Sharing Specification, Revision 1.1.* PCI Special Interest Group. The SR-IOV standard itself. Available to PCI-SIG members.

<a id="ref-oasis-virtio"></a>
**OASIS (2022).** *Virtual I/O Device (VIRTIO) Specification, Version 1.2.* OASIS Standard. Available at [docs.oasis-open.org/virtio](https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html). The definitive virtio specification.

<a id="ref-vxlan"></a>
**Mahalingam, M., Dutt, D., Duda, K., Agarwal, P., Kreeger, L., Sridhar, T., Bursell, M., & Wright, C. (2014).** *Virtual eXtensible Local Area Network (VXLAN): A Framework for Overlaying Virtualized Layer 2 Networks over Layer 3 Networks.* IETF RFC 7348. Available at [rfc-editor.org/rfc/rfc7348](https://www.rfc-editor.org/rfc/rfc7348).


<a id="ref-waldspurger"></a>
**Waldspurger, C. A. (2002).** *Memory Resource Management in VMware ESX Server.* Proceedings of the 5th USENIX Symposium on Operating Systems Design and Implementation (OSDI), 181–194. The original ballooning paper; still a model of clear systems writing.

**Bhargava, R., Serebrin, B., Spadini, F., & Manne, S. (2008).** *Accelerating Two-Dimensional Page Walks for Virtualized Systems.* Proceedings of the 13th International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), 26–35. The foundational paper on AMD's NPT, with detailed analysis of 2D walk performance.

**Neiger, G., Santoni, A., Leung, F., Rodgers, D., & Uhlig, R. (2006).** *Intel Virtualization Technology: Hardware support for efficient processor virtualization.* Intel Technology Journal, 10(3). The official Intel description of VT-x and, in its extended discussion, the motivation for EPT that would follow in 2008.

**Adams, K. & Agesen, O. (2006).** *A Comparison of Software and Hardware Techniques for x86 Virtualization.* Proceedings of the 12th International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), 2–13. A careful measurement study comparing binary translation (shadow PTs) to then-new VT-x, instructive for understanding why hardware support wins in practice.

**Arcangeli, A., Eidus, I., & Wright, C. (2009).** *Increasing memory density by using KSM.* Proceedings of the Ottawa Linux Symposium. The definitive paper on Kernel Same-page Merging as implemented in Linux/KVM.

**Intel Corporation.** *Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3C, Chapter 28: VMX Support for Address Translation.* The authoritative specification of EPT. Available at [intel.com/sdm](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html).

**Linux Kernel Documentation.** *Transparent Hugepages.* [kernel.org/doc/html/latest/admin-guide/mm/transhuge.html](https://www.kernel.org/doc/html/latest/admin-guide/mm/transhuge.html). Practical reference for the huge-page mechanisms relevant to HPC tuning.

### Online documentation and tutorials

<a id="ref-qemu-docs"></a>
**QEMU Project.** *QEMU Documentation.* [qemu.org/docs](https://www.qemu.org/docs/master/). The canonical reference for the most widely used open-source emulator/hypervisor frontend.

<a id="ref-qcow2-spec"></a>
**QEMU Project.** *QCOW2 Image Format Specification.* [gitlab.com/qemu-project/qemu/-/blob/master/docs/interop/qcow2.txt](https://gitlab.com/qemu-project/qemu/-/blob/master/docs/interop/qcow2.txt). The definitive QCOW2 format specification.

**Linux Kernel.** *Documentation/virt/ in the Linux source tree.* The primary reference for KVM internals. Available at [kernel.org/doc/html/latest/virt/](https://www.kernel.org/doc/html/latest/virt/).

**libvirt Project.** *Libvirt Documentation.* [libvirt.org/docs.html](https://libvirt.org/docs.html). The standard abstraction layer used by OpenStack, oVirt, Proxmox, and others to manage hypervisors from a uniform API.

<a id="ref-ovs"></a>
**Open vSwitch Project.** *Open vSwitch Documentation.* [docs.openvswitch.org](https://docs.openvswitch.org/). The virtual switch used by most cloud networking platforms.

<a id="ref-aws-nitro"></a>
**Amazon Web Services (2022).** *The Security Design of the AWS Nitro System.* AWS whitepaper. Available at [docs.aws.amazon.com/whitepapers](https://docs.aws.amazon.com/whitepapers/latest/security-design-of-aws-nitro-system/security-design-of-aws-nitro-system.html). The definitive public description of the Nitro architecture.

<a id="ref-lustre"></a>
**Lustre Project.** *Lustre Documentation.* [doc.lustre.org](https://doc.lustre.org/lustre_manual.xhtml). The reference for the dominant parallel file system in HPC.

### Historical and contextual references

**Goldberg, R. P. (1974).** *Survey of Virtual Machine Research.* IEEE Computer, 7(6), 34–45. Contemporaneous survey by one of the Popek–Goldberg authors, useful for historical context.

**Rosenblum, M. & Garfinkel, T. (2005).** *Virtual Machine Monitors: Current Technology and Future Trends.* IEEE Computer, 38(5), 39–47. Mid-decade snapshot of the field by one of VMware's founders.

---

*These notes are provided for the exclusive use of students enrolled in HPCC 2025/2026 at the University of Trieste. They reflect the author's interpretation of the material and are not a substitute for the primary references cited above. Comments, corrections, and clarifications are welcome and should be addressed to the instructor via the usual course channels.*

*End of Lecture 03 notes.*
