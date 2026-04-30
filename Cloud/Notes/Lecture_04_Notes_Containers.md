# Lecture 04 — Containers and Docker

**Course:** High Performance and Cloud Computing 2025/2026
**Programme:** Applied Data Science & Artificial Intelligence — HPC Track
**Institution:** University of Trieste
**Author:** Course instructor
**Duration:** 1 h 45 min (Part 1 ≈ 50 min, Part 2 ≈ 55 min)
**Prerequisites:** Lecture 03 (Virtualization Fundamentals) and its appendix on Memory Virtualization


---

## Learning Objectives

By the end of this lecture, the student will be able to:

1. **Explain** how Linux namespaces and cgroups, in combination, produce the abstraction we call a "container."
2. **Distinguish** containers from virtual machines in terms of architecture, overhead, and the specific security boundary each provides.
3. **Demonstrate**, with a hands-on `unshare` experiment, that PID namespace isolation is real — that the same process can have different PIDs depending on which namespace observes it.
4. **Manipulate** cgroups v2 directly to set memory and CPU limits on a process tree, both with and without root privileges.
5. **Construct** efficient, layered Docker images using Dockerfile, build-cache optimisation, and multi-stage builds.
6. **Design** container networking topologies using the bridge, host, and overlay drivers, and articulate the trade-offs of each.
7. **Evaluate** storage strategies for containerised scientific workloads: bind mounts, named volumes, tmpfs.
8. **Compose** multi-container applications with Docker Compose for reproducible research environments.
9. **Apply** container security best practices: non-root execution, image scanning, capability dropping, resource limits.
10. **Compare** container runtimes (runc, gVisor, Kata, Podman) and explain how OCI standards make them interchangeable.
11. **Justify** the use of HPC-specific container runtimes (Apptainer, Sarus) over Docker on shared cluster systems.

---

## Table of Contents

**Part I — Foundations**
1. The problem containers solve
2. Linux foundations: namespaces and cgroups
3. Docker architecture
4. Docker images, in depth

**Part II — Practice**
5. Container networking
6. Container storage
7. Docker Compose
8. Container registries
9. Container security
10. Container runtimes and OCI standards
11. Containers for scientific computing

*Concluding remarks · Glossary · References*

---

# Part I — Foundations

## §1. The problem containers solve

Before we examine the kernel mechanics, it is worth being clear about *why* containers exist and what specific class of problems they address. Confusion on this point is responsible for most of the bad arguments one hears about containers — for instance, the idea that they are merely "lightweight VMs," or that they exist primarily for performance reasons. Both readings miss the historical and technical motivation.

### §1.1 The reproducibility crisis in computational science

A 2016 survey by *Nature* found that 70% of researchers had failed to reproduce another scientist's results, and over 50% had failed to reproduce their own [(Baker, 2016)](#ref-baker-2016). The reasons are many — statistical methodology, data availability, publication bias — but in computational fields a substantial fraction of failures trace to a single mundane cause: the software environment on the second machine differs, in some small way, from the first, and that small difference produces a different answer.

The root cause is what we will call **environment coupling**. A scientific application does not run on bare hardware; it depends on a stack of components. A typical molecular dynamics code requires a specific MPI implementation, a specific BLAS library, a specific Python interpreter version, a specific CUDA toolkit, a specific kernel version with specific InfiniBand modules loaded. Each of these is a place where two machines can disagree silently. Two laboratories may both claim to use "GROMACS 2024.1," but one was compiled against OpenMPI 4.1 with `-O3 -march=skylake-avx512` while the other used Intel MPI 2021 with `-O2`. The same source code, the same input file, slightly different floating-point sums, slightly different trajectories, eventually diverging answers.

On a traditional HPC cluster, this stack is managed by **environment modules** (`module load openmpi/4.1.4`, `module load gromacs/2024.1`). Modules work — they are why HPC has scaled as far as it has — but they do not travel. The exact set of modules that produces a working environment on Leonardo cannot be loaded on a colleague's laptop, on a CI server, on AWS, on a different cluster. Each new target requires the configuration to be re-derived from documentation, with all the opportunities for divergence that implies.

> **Intuition.** A scientific computation is not just source code. It is source code *plus* an environment. Reproducibility requires capturing both. Modules capture the environment for one specific machine; containers capture it as a portable artefact.

> **The "works on my machine" syndrome.** The folkloric name for this whole class of failure: the application runs correctly in one developer's environment but fails elsewhere because implicit dependencies on system libraries, compiler flags, environment variables, or kernel features are not captured. The phrase originally referred to web development, but in scientific computing it carries even higher stakes — a research conclusion may depend on bits that one researcher can produce and another cannot.

### §1.2 Historical context

Containers did not appear from nowhere. The idea of restricting a process to a partial view of system resources is older than Linux itself, and the modern container is the result of layered ideas that accumulated over four decades.

The first ancestor is the **`chroot`** system call, introduced in Unix Version 7 in 1979. `chroot` changes the apparent root of the filesystem for a process and its descendants: a process placed in `chroot /var/jail` sees `/var/jail` as `/`, and cannot reach files outside it. This is filesystem isolation only — process IDs, network, users, and most other resources remain global — but it established the principle that a process can be given a restricted view of its environment.

**FreeBSD Jails** (2000) extended chroot with process and network isolation: a jailed process could only see other processes within the same jail, and had its own network interface. This is recognisably the container abstraction, fifteen years before Docker.

**Solaris Zones** (2004) added systematic resource controls (CPU, memory limits) and a polished management interface. Zones were used heavily in Sun's enterprise customers but never escaped the Solaris world.

The Linux story has two parallel strands. Around 2006, Google engineers contributed **cgroups** to the kernel (originally called "process containers"), motivated by the need to control resource consumption among the many services running on each machine of Google's fleet. Independently, work on **Linux namespaces** had been ongoing since 2002 (mount namespaces) and accelerated through the late 2000s, adding new namespace types one by one. By around 2013 the kernel had all the ingredients: PID, network, mount, IPC, UTS, and user namespaces, plus cgroups for resource control.

The tooling caught up in 2008 with **LXC** (Linux Containers), which combined namespaces and cgroups into a usable container runtime. But LXC remained a tool for systems engineers — its configuration files were complex, its management ad hoc, its image distribution non-existent.

Then in March 2013, dotCloud, a struggling PaaS company, open-sourced their internal tool **Docker**. Docker's contribution was not a fundamental technical breakthrough — namespaces and cgroups had existed for years, and Docker initially used LXC as its backend. The contribution was an *ecosystem*: a friendly command-line interface (`docker run`), a portable image format with a precise specification, a public registry (Docker Hub) with thousands of pre-built images, and the **Dockerfile** — a simple text format that captured an entire environment's construction in a way both human-readable and machine-executable. The combination made containers usable by application developers, not just by systems engineers, and the field exploded.

The subsequent years saw consolidation. The **Open Container Initiative (OCI)** was founded in 2015 to standardise image and runtime formats so that no single vendor (such as Docker) could control them. **Singularity** (later renamed Apptainer) was released in 2015, taking the container idea but redesigning the security model around the constraints of HPC clusters — the subject of Lecture 03's Chapter 10 and §11 of these notes. **containerd** was donated to the CNCF in 2017, formally separating the high-level runtime (image management, lifecycle) from the low-level runtime (process isolation), which Docker had bundled together. By the early 2020s, Kubernetes — the dominant container orchestrator — had migrated away from Docker entirely and used containerd directly.

> **A useful frame.** Docker did not invent containers. It made them accessible. The technical innovations — namespaces, cgroups, OverlayFS — were Linux kernel work largely done at Google and IBM. Docker's contribution was packaging, ergonomics, and ecosystem. This is worth remembering because it explains why "Docker" and "containers" are not synonymous: Docker is one user-facing tool atop a substrate that is now used by many other tools (Podman, Apptainer, containerd, CRI-O).

### §1.3 Containers versus virtual machines

In Lecture 03 we examined virtual machines in detail. The natural question is: how do containers compare? The honest answer is that they do not compete on the same axis — they are different abstractions providing different guarantees. But because they are sometimes deployed for similar purposes, a side-by-side comparison is illuminating.

| Dimension | Virtual machines | Containers |
|---|---|---|
| Isolation boundary | Hardware-level via hypervisor | Process-level via kernel namespaces |
| Guest overhead | Full OS kernel per VM (hundreds of MB to GB of RAM) | Shared host kernel (a few MB of overhead per container) |
| Boot time | Seconds to minutes | Milliseconds to a few seconds |
| Density | Tens per host | Hundreds to thousands per host |
| Image size | GBs (full OS image) | MBs to low GBs (application + dependencies) |
| Security isolation | Stronger — separate kernel per guest | Weaker — shared kernel attack surface |
| Portability | Hypervisor-specific (OVA, VMDK, AMI) | OCI-standard images, highly portable across runtimes |
| Performance overhead | 2–5% (with hardware-assisted virtualisation) | Near-native — no virtualisation layer at all |

The architectural difference is best seen as a stack diagram. In the VM model, each guest has its own complete operating system, communicating with hardware through a hypervisor:

```
Virtual Machine model            Container model

┌──────────────────────┐         ┌──────────────────────┐
│  App A   │  App B    │         │  App A  │  App B     │
├──────────┼───────────┤         ├─────────┼────────────┤
│ Bins/Libs│ Bins/Libs │         │Bins/Libs│ Bins/Libs  │
├──────────┼───────────┤         ├─────────┴────────────┤
│ Guest OS │ Guest OS  │         │  Container Runtime   │
├──────────┴───────────┤         ├──────────────────────┤
│      Hypervisor      │         │      Host OS         │
├──────────────────────┤         ├──────────────────────┤
│       Hardware       │         │       Hardware       │
└──────────────────────┘         └──────────────────────┘
```

The two diagrams differ in one critical place: **the Guest OS row**. In a VM, each application sits atop a private OS kernel, which is itself a sizeable piece of software (~50 MB to 1 GB). In a container, the OS row is shared. There is exactly one kernel on the machine, and the "container runtime" — runc, crun, gVisor — is a thin user-space tool that arranges processes to see only restricted views of that kernel.

The implications cascade. Because there is only one kernel, there is no nested page table walk (Lecture 03 appendix §A.5), no VM-exit on privileged instructions (Lecture 03 §6), no I/O paravirtualisation (Lecture 03 §9). The container's processes execute exactly like ordinary host processes, with the same scheduler, the same MMU traversal cost, the same I/O stack. The "performance overhead" of containers is nearly zero for the obvious reason: there is nothing to overhead.

But the same shared-kernel property that gives containers their performance gives them weaker security. If a guest VM compromises its own kernel through a privilege-escalation exploit, it gains root inside that one VM only — the hypervisor, the host kernel, and other guests are protected by an architectural boundary. If a containerised process compromises the kernel, it has compromised *the whole host* and every other container on it. The shared-kernel attack surface is the central security trade-off of the container model, and we will return to it in §9.

> **Key insight.** Containers are *not* lightweight VMs. They are a fundamentally different abstraction — restricted processes that share the host kernel. This distinction has deep implications for security, performance, and portability that we will explore throughout this lecture.

> **Modern practice.** In production cloud architectures the two technologies are usually combined. VMs provide tenant-level isolation: the cloud provider gives you a VM that is yours, isolated from other customers by the hypervisor. Within your VM, you run containers for application-level density: dozens or hundreds of containerised microservices share the VM's resources but maintain independent runtime environments. AWS ECS-on-EC2, GKE, and AKS all follow this pattern. The two technologies are complements, not substitutes.

### Self-check §1

1. The 2016 *Nature* survey is sometimes summarised as "70% of researchers cannot reproduce others' results." Why is this only one of several reproducibility problems that containers help with? What other failure modes do they not address?
2. `chroot` was introduced in 1979. Why did it take 35 years for a general-purpose container abstraction to emerge from it?
3. Identify one production scenario in which a container is the wrong choice and a VM is the right one, and explain the technical reason.

---

## §2. Linux foundations — namespaces and cgroups

We now descend into the kernel mechanics. A container, as we have said, is not a special kernel object. The Linux kernel has no `container` system call, no `struct container` in its source code, no documentation chapter named "containers." What it has, instead, is two orthogonal mechanisms — *namespaces* and *control groups* — that, when used together, produce the abstraction we call a container.

The two mechanisms divide labour cleanly. **Namespaces** control what a process can *see*: which other processes exist, which network interfaces are present, which mount points are mounted, which user IDs are valid. **Cgroups** (control groups) control what a process can *use*: how much CPU time, how much memory, how much I/O bandwidth, how many open files, how many child processes.

We will treat them in turn — namespaces first because their effects are dramatic and visible, cgroups second because their effects are subtle and quantitative — and then look at how container runtimes combine them.

### §2.1 Containers are just processes

Before formal definitions, an empirical observation. Run the following on any Linux machine with Docker:

```bash
docker run -d --name demo nginx
docker top demo
```

The `docker top` command prints the host PID of the nginx master process — say, 28451. Now on the host (no Docker required):

```bash
ps -p 28451
ls -la /proc/28451/exe
```

`ps` confirms the process exists. `/proc/28451/exe` points to the nginx binary. The container is a normal Linux process. There is no *extra* OS object representing it. The kernel sees a process; the kernel schedules a process; the kernel kills a process. Docker, runc, and the rest of the stack are simply user-space code that arranges this process to live inside restricted views.

The mechanism is summarised by the diagram below. The kernel provides two orthogonal restriction systems; the container runtime sets them up; the resulting process is what we call a container.

```
                ┌──────────────────────────────────────┐
                │           Linux Kernel               │
                │                                      │
                │  ┌──────────────┐ ┌──────────────┐   │
                │  │  Namespaces  │ │   cgroups    │   │
                │  │ (visibility) │ │  (resources) │   │
                │  └──────┬───────┘ └──────┬───────┘   │
                │         │                │           │
                │    ┌────┴────────────────┴─────┐     │
                │    │     Container Process     │     │
                │    │  • Isolated PID tree      │     │
                │    │  • Private network stack  │     │
                │    │  • Own filesystem view    │     │
                │    │  • CPU/memory limits      │     │
                │    └───────────────────────────┘     │
                └──────────────────────────────────────┘
```

If this seems anticlimactic, that is the point. Most of the apparent complexity of containers is in the user-space tooling — image distribution, networking orchestration, build cache management. The kernel does relatively little; it just answers two questions about every system call: *what can this process see?* and *how much can this process use?*

### §2.2 Namespaces — what a process sees

A namespace is a kernel mechanism that partitions some global resource into independent instances, and assigns each process to exactly one instance per namespace type. Processes in different instances of the same namespace cannot affect each other through that resource.

Linux currently defines seven namespace types, listed below. Each has its own `CLONE_NEW*` flag for the `clone()` system call (the kernel-level interface used to create a process with new namespaces) and its own characteristic effect on the process's view of the system.

| Namespace | `CLONE_` flag | Isolates | Container effect |
|---|---|---|---|
| **PID** | `CLONE_NEWPID` | Process IDs | Container sees its own PID tree; PID 1 is the container's init process |
| **NET** | `CLONE_NEWNET` | Network stack | Own interfaces, IP addresses, routing table, ports, firewall rules |
| **MNT** | `CLONE_NEWNS` | Mount points | Own filesystem view; in-container mounts do not affect the host |
| **UTS** | `CLONE_NEWUTS` | Hostname & domain | Container can set its own hostname without affecting the host |
| **IPC** | `CLONE_NEWIPC` | IPC resources | Separate System V IPC objects, POSIX message queues, shared memory |
| **USER** | `CLONE_NEWUSER` | User and group IDs | UID 0 inside container can map to an unprivileged UID on host |
| **CGROUP** | `CLONE_NEWCGROUP` | Cgroup root view | Container sees only its own subtree of the cgroup hierarchy |

The historical order of introduction (mount namespace 2002; UTS, IPC, network mid-2000s; user namespace 2013; cgroup namespace 2016) reflects the incremental nature of the work. Each namespace type was added when a particular use case demonstrated its necessity. Today's container runtimes use most or all of them simultaneously.

> **Formally.** A namespace is an equivalence class of processes that share a particular view of a kernel resource. The kernel maintains, for each namespace type, a set of *namespace instances*; every process belongs to exactly one instance per type. Two processes interact through the resource only if they are in the same instance.

The namespaces of a running process can be inspected through the `/proc` filesystem. Each process has a directory `/proc/<PID>/ns/` containing one symbolic link per namespace type, pointing to a magic file whose name encodes the namespace's *inode number* — its kernel-level identifier:

```bash
$ ls -la /proc/$$/ns/
lrwxrwxrwx 1 user user 0 ... cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx 1 user user 0 ... ipc    -> 'ipc:[4026531839]'
lrwxrwxrwx 1 user user 0 ... mnt    -> 'mnt:[4026531840]'
lrwxrwxrwx 1 user user 0 ... net    -> 'net:[4026531840]'
lrwxrwxrwx 1 user user 0 ... pid    -> 'pid:[4026531836]'
lrwxrwxrwx 1 user user 0 ... user   -> 'user:[4026531837]'
lrwxrwxrwx 1 user user 0 ... uts    -> 'uts:[4026531838]'
```

Two processes whose `pid` symlinks point to the same inode are in the same PID namespace and see each other's PIDs. Two processes whose `pid` symlinks point to different inodes are in different PID namespaces and cannot. This is the entirety of the abstraction; everything else is bookkeeping.

The most directly visible effect is the **PID namespace**, which we now demonstrate concretely.

### §2.3 A hands-on demonstration: PID namespaces with `unshare`

The fastest way to develop intuition for namespaces is to create one yourself and observe the result. Linux provides the `unshare` command — a thin wrapper around the `unshare()` system call — which executes a program in newly created namespaces. We will use it to create a fresh PID and user namespace, then look around.

The demonstration proceeds in five steps. You should run each command yourself as you read.

**Step 1 — establish a baseline view of the host.**

Run, on any Linux machine:

```bash
$ ps -ef | head -10
```

You will see the standard list of processes on your system — `init` or `systemd` as PID 1, kernel threads (`[kthreadd]`, `[rcu_*]`, etc.), system daemons, your shell. Probably 100–300 processes total. This is the *global* PID namespace.

Now request a list of all namespaces currently in use on the system:

```bash
$ sudo lsns
```

`lsns` lists every namespace instance on the host along with how many processes are in it. On a typical workstation you will see a handful of namespaces of each type — the host's defaults, plus one set for each running container if Docker is active. The columns of interest are `NS` (the namespace inode number), `TYPE`, and `NPROCS` (how many processes are in it).

**Step 2 — confirm what you can see as an unprivileged user.**

Without sudo:

```bash
$ ps -ef
$ ls /proc/1/
```

`ps -ef` works without privileges; you can see all processes on the system, even those owned by root. This is intentional: process visibility is granted by membership in the host's PID namespace, not by ownership. `ls /proc/1/` will show some directories you can read (the `cmdline` of the init process) and some you cannot (its file descriptors). This is a property of file permissions, not namespaces — but it sets up a contrast we will exploit shortly.

**Step 3 — create a new PID namespace.**

Now the punchline:

```bash
$ unshare --user --pid --map-root-user --mount-proc --fork bash
```

Five flags, each doing something specific:

- `--user` creates a new user namespace. This is the key that allows an unprivileged user to do everything else — without it, creating a PID namespace would require root.
- `--pid` creates a new PID namespace. The new shell will see itself as PID 1.
- `--map-root-user` maps your current host UID to UID 0 (root) inside the new user namespace. *Inside* the new namespaces you appear to be root, with all root privileges; *outside*, on the host, you remain your unprivileged self.
- `--mount-proc` creates a new mount namespace as well, and remounts `/proc` inside it. This is essential: `ps` reads `/proc/<PID>/` to enumerate processes, and without a fresh `/proc` it would still see the host's PIDs. We need a `/proc` that reflects the new PID namespace.
- `--fork` is needed because the PID namespace only takes effect for *children* of the namespace creator. `unshare` itself remains in the original namespace; with `--fork`, it forks a child that becomes PID 1 of the new namespace.

After the command, you are in a new shell. The prompt likely shows `root@hostname:#`, even though you typed the command as your normal user. That is the user-namespace mapping in action.

**Step 4 — observe the new namespace.**

Inside the new shell:

```bash
# ps -ef
UID    PID  PPID  C STIME TTY  TIME CMD
root     1     0  0 14:02 pts/0 ... bash
root     5     1  0 14:02 pts/0 ... ps -ef
```

Two processes. **PID 1 is bash**, your shell. There is no systemd, no kernel threads, no other user's daemons. As far as the processes inside this namespace are concerned, the system was just booted, the kernel has just started bash, and that is the entire universe.

```bash
# whoami
root
# id
uid=0(root) gid=0(root) groups=0(root)
```

You appear to be root. You can `mkdir /etc/anything`, edit files in `/`, kill any process you can see. But every "root" action is filtered through the user namespace mapping back to your real UID on the host. If you try to read `/etc/shadow` (which on the host requires root and your real UID is not root), you will get permission denied: the kernel checks the *real* UID for filesystem access, regardless of how you appear inside the namespace.

```bash
# hostname
some-name   # whatever the host hostname is — UTS namespace was NOT unshared
```

We did not pass `--uts`, so the UTS namespace is still the host's; you see the host's hostname. If you also pass `--uts`, you can `hostname container01` and have it stick locally without affecting the host. Each namespace is independent.

**Step 5 — verify on the host that the PIDs differ.**

In another terminal, on the host (still as your normal user):

```bash
$ ps aux | grep bash
yourname  29017  ...  bash    # this is the unshared shell
yourname  29021  ...  bash    # other shells
```

The bash that *inside* its namespace is PID 1 is, *on the host*, PID 29017. **The same process has two different PIDs simultaneously.** This is not a Docker trick. This is the kernel exposing exactly the property the PID namespace is supposed to provide.

> **Try it (the most informative experiment).** Inside the unshared shell, run `ls /proc/`. You will see only PIDs 1, 5, … — the few processes inside the namespace. Then run `ls /proc/1/ns/pid` and note the inode number. In the host shell, run `ls -la /proc/29017/ns/pid` (substituting the actual host PID): the inode matches. Same kernel object, two views, different identities.

> **Why we used `--mount-proc`.** Without it, the unshared shell would still have the host's `/proc` mounted. `ps` reads `/proc/<N>/` for each numeric directory it finds; with the host's `/proc` mounted, it would see all the host's PIDs. The `--mount-proc` flag remounts `/proc` so that it reflects the new PID namespace. This dependency between PID namespace and mount namespace is one of the small surprises of the namespace API: namespaces are orthogonal *in principle*, but for tools like `ps` to work correctly you must coordinate them.

When you `exit`, the namespace and all its inhabitants are destroyed (because there is no other process keeping the namespace alive — namespaces are reference-counted). The host returns to its normal state. You have just created and torn down, by hand, what is structurally the same isolation Docker creates with `docker run`.

> **Self-check §2.3.**
> (a) If you omit `--mount-proc` from the `unshare` invocation, what do you predict you will see when you run `ps -ef` inside the new shell? Try it.
> (b) Why is `--map-root-user` essential? What happens without it (try, but expect failure)?
> (c) Explain what would happen if you also added `--net` to the flags — what would `ip a` show?

### §2.4 The role of the user namespace — and why HPC cares

The user namespace is the most subtle of the seven, and the most important for HPC.

A user namespace defines a mapping between *user IDs inside the namespace* and *user IDs outside it*. The mapping is many-to-many in principle, but most container runtimes use a simple form: a single UID inside (typically 0, root) maps to a single UID outside (the unprivileged user who started the container). Inside the namespace, the process believes it is root: it can `chown` files in its own filesystem view, install software with `apt-get`, bind privileged ports. Outside, on the host, the kernel sees the process as belonging to the unprivileged user; any operation that touches the host filesystem or other processes is checked against the real UID.

This is the mechanism that makes **rootless containers** possible. An ordinary user can run a container that *internally* appears to have root privileges, while *externally* posing no risk to the rest of the system.

For HPC the implications are decisive. On Leonardo, on every shared cluster, ordinary users do not have root. Docker's traditional model — a system-wide daemon running as root, granting effective root to anyone in the `docker` group — is incompatible with this constraint. The user-namespace mechanism allowed two responses:

1. **Apptainer** (formerly Singularity): a daemonless container runtime designed from the start around user namespaces. Each user runs their own containers; no privileged daemon mediates them.
2. **Rootless Docker / Podman**: more recent variants of the existing tools that use user namespaces to drop the root requirement. Podman in particular is a drop-in `docker` replacement that runs entirely in user space.

We return to these tools in §11. For now, remember that the user namespace is what makes any of this possible.

### §2.5 Control groups — what a process can use

If namespaces answer "what does this process see?", control groups answer "what can this process use?" They are the resource-limiting half of the container abstraction.

A cgroup is a collection of processes plus a set of resource limits and accounting counters. Every process on a Linux system belongs to exactly one cgroup per controller (in cgroups v2, exactly one cgroup overall). The cgroups form a hierarchy — each cgroup may have child cgroups, with limits cascading downward.

The list of resources that can be controlled has grown over time. The major controllers are:

| Controller | Controls | Example setting | Enforcement |
|---|---|---|---|
| `cpu` | CPU time allocation | `cpu.max 50000 100000` (50% of one core) | Throttle — process slowed |
| `cpuset` | CPU core pinning | `cpuset.cpus 0-3` (use only cores 0–3) | Hard — process cannot run on other cores |
| `memory` | RAM and swap usage | `memory.max 4294967296` (4 GB) | Hard — OOM kill if exceeded |
| `io` | Block I/O bandwidth | `io.max 8:0 rbps=104857600` (100 MB/s read) | Throttle — I/O delayed |
| `pids` | Number of processes | `pids.max 100` | Hard — fork fails if exceeded |
| `devices` | Device-node access | Allow `/dev/nvidia0` only | Hard — permission denied |

Three different *kinds* of enforcement appear in this table. **Hard limits** (memory, pids, devices) result in failure when exceeded — the kernel kills the process, denies the syscall, or returns an error. **Throttling** (cpu, io) slows the process to its quota without killing it — useful for noisy-neighbour mitigation. **Pinning** (cpuset) restricts where the process may run rather than how much it may consume. Understanding which kind of enforcement applies to which resource is essential for predicting how your containerised workload will behave under contention.

Two regimes of cgroups exist: **v1** (the original 2007 design, with one separate hierarchy per controller) and **v2** (a unified hierarchy introduced in 2016 and standardised around 2019). All modern distributions — Ubuntu 22.04+, Fedora 31+, RHEL 9+, Debian 11+ — default to v2. We will work exclusively with v2 in this lecture; v1 still exists in the kernel but is deprecated.

Verify which version your system uses:

```bash
$ stat -fc %T /sys/fs/cgroup/
cgroup2fs                   # cgroups v2 (modern)
# or
tmpfs                       # cgroups v1 (legacy)
```

If you see `cgroup2fs`, the rest of this section's commands will work directly. If you see `tmpfs` and a structure under `/sys/fs/cgroup/memory/`, `/sys/fs/cgroup/cpu/`, etc., you are on cgroups v1 and the commands need adjustment. We assume v2 throughout.

### §2.6 A hands-on tour of cgroups v2

The cgroups v2 hierarchy is exposed as a virtual filesystem under `/sys/fs/cgroup/`. To create a cgroup is literally to create a directory; to set a limit is to write to a file; to add a process to a cgroup is to write its PID to that cgroup's `cgroup.procs` file. The interface is so simple that it is occasionally surprising.

We will run two demonstrations: a **rootful** version that works on any Linux system without configuration, and a **rootless** version that works only with appropriate cgroup delegation but which is the right model for production HPC workloads.

#### §2.6.1 The rootful path — direct manipulation

The most direct way to learn cgroups is to manipulate them by hand as root.

```bash
# Inspect what is already there
$ sudo systemd-cgtop
```

`systemd-cgtop` is the cgroup analogue of `top`: a live, sortable view of cgroups by CPU, memory, I/O. The tree-like structure on the left reflects the cgroup hierarchy. On a typical desktop you will see `system.slice` (system services), `user.slice` (logged-in users' processes), and possibly `machine.slice` (Docker, libvirt VMs). Each has children, and each has resource accounting. Spend a moment scrolling through; press `q` to exit.

Now create a cgroup by hand:

```bash
$ sudo mkdir /sys/fs/cgroup/demo
$ ls /sys/fs/cgroup/demo/
cgroup.controllers  cgroup.events  cgroup.freeze  cgroup.max.depth  ...
cpu.max  memory.max  memory.current  pids.max  ...
```

The kernel populated the new directory with control files automatically, one per available controller. Some files are read-only (`memory.current`, `cpu.stat`); some are read-write (`memory.max`, `cpu.max`); some are write-only (`cgroup.procs`, which adds processes to the cgroup).

Set a memory limit:

```bash
$ echo "100M" | sudo tee /sys/fs/cgroup/demo/memory.max
$ cat /sys/fs/cgroup/demo/memory.max
104857600
```

The kernel parsed `100M` as 100 MiB and stored 104857600 bytes. Set a CPU quota:

```bash
$ echo "50000 100000" | sudo tee /sys/fs/cgroup/demo/cpu.max
```

Two numbers: the cgroup may use 50000 microseconds of CPU per 100000 microseconds of wall-clock time, i.e., half a core.

Now place a process into the cgroup. The simplest is to start a shell and add it:

```bash
$ sudo bash
# echo $$ > /sys/fs/cgroup/demo/cgroup.procs
# cat /sys/fs/cgroup/demo/cgroup.procs
12345    # the bash itself
```

Any command you launch from this shell is now subject to the limits, because cgroup membership is inherited by children. Try a memory hog:

```bash
# python3 -c "x = bytearray(200 * 1024 * 1024)"
Killed
```

The kernel killed the process at the 100 MiB limit. The familiar exit code 137 (= 128 + 9 = SIGKILL) is what Docker reports as `OOMKilled: true` for containers that exceed `--memory`. Same mechanism, exposed directly.

When you are done, exit the shell, remove the cgroup:

```bash
# exit
$ sudo rmdir /sys/fs/cgroup/demo
```

(`rmdir` only works if the cgroup is empty and has no children — the kernel enforces this to prevent orphan processes.)

> **What about `cgcreate` and `cgexec`?** The `libcgroup` package provides higher-level commands: `cgcreate -g memory,cpu:demo` to create a cgroup, `cgexec -g memory,cpu:demo command` to run a command in it, `cgset -r memory.max=100M demo` to set limits. These exist and work, but they are a thin wrapper around the same `/sys/fs/cgroup/` interface we used above. On modern systems with cgroups v2, working directly with the filesystem is simpler and more transparent — and is exactly what container runtimes do internally. If you have `cgcreate` available and prefer it, the equivalent is:
>
> ```bash
> $ sudo cgcreate -g memory,cpu:demo
> $ sudo cgset -r memory.max=100M demo
> $ sudo cgset -r cpu.max="50000 100000" demo
> $ sudo cgexec -g memory,cpu:demo bash
> ```

#### §2.6.2 The rootless path — `systemd-run`

The rootful path requires `sudo` and is the same model Docker uses internally. But the more interesting question — and the one that matters for HPC — is whether an unprivileged user can set their own resource limits.

In principle, cgroups v2 supports this through *delegation*: an administrator delegates control of a cgroup subtree to a user, who can then create cgroups within that subtree without root privileges. In practice, the situation is fragmented:

- On systems with **systemd** (any modern Linux distribution), each user session has a delegated cgroup at `/sys/fs/cgroup/user.slice/user-$UID.slice/user@$UID.service/`.
- The **memory** and **pids** controllers are typically delegated to user sessions on Fedora, RHEL 9, recent Debian, and recent Ubuntu.
- The **cpu** and **cpuset** controllers are *not* delegated by default on Ubuntu (any version); enabling them requires editing `/etc/systemd/system.conf` (or a drop-in) to add `Delegate=cpu cpuset memory pids` and rebooting. Fedora delegates cpu by default since version 36 or so.
- The **io** controller is rarely delegated.

For a portable rootless example, we use **`systemd-run`**, which creates a transient cgroup managed by systemd's per-user instance:

```bash
$ systemd-run --user --scope --property=MemoryMax=100M python3 -c "x = bytearray(200*1024*1024)"
Running scope as unit: run-rXXXX.scope
Killed
```

Here:
- `--user` tells systemd to use the per-user instance, not the system-wide one.
- `--scope` runs the command in a transient *scope* unit (analogous to a cgroup) rather than as a service. Scopes inherit the current shell environment, which is what you want for ad-hoc commands.
- `--property=MemoryMax=100M` sets the memory limit.

If your distribution delegates only `memory` and `pids`, the above works. If you try `--property=CPUQuota=50%` and your system does not delegate `cpu` to user sessions, you will get an error like `Failed to set unit properties: Permission denied` — the kernel refused because your user namespace's cgroup is not allowed to set CPU limits.

Verify what your user has access to:

```bash
$ cat /sys/fs/cgroup/user.slice/user-$(id -u).slice/cgroup.controllers
memory pids   # only these are delegated on Ubuntu 22.04 by default
```

The output lists which controllers are available within your user's cgroup subtree. On a system where only `memory` and `pids` appear, you cannot set `cpu` or `cpuset` limits as an unprivileged user.

> **HPC implication.** This is exactly the situation Apptainer and Podman face on shared clusters. Their solution is twofold: where rootless cgroup delegation works (recent kernels, properly configured systems), they use it; where it does not, they fall back to running with no resource limits — relying on the cluster scheduler (Slurm) to enforce limits at a higher level. Slurm puts your job into a cgroup *itself*, so even if Apptainer cannot create its own cgroup, the Slurm-imposed limits still apply.

> **Try it.** Open two terminals. In the first, run `systemd-cgtop`. In the second, run `systemd-run --user --scope --property=MemoryMax=200M stress -m 1 --vm-bytes 150M --timeout 30` (assuming you have `stress` installed; if not, use the Python example above with a smaller allocation). In the first terminal, watch the cgroup `user.slice/user-$UID.slice/.../run-*.scope` appear, accumulate ~150 MB of memory, and disappear after 30 seconds. This is the same view a sysadmin gets in production. Press `c` and `m` to sort by CPU and memory.

### §2.7 Putting it together — what makes a "container"

A container is what you get when a process is placed into a fresh set of namespaces *and* a cgroup. Specifically:

1. A container runtime (runc, crun, gVisor) calls `clone()` with the `CLONE_NEW*` flags for the namespaces it wants to create — typically all seven, though some may be shared with the host for performance.
2. The new process's `/proc/<pid>/ns/*` symlinks point to fresh namespace instances.
3. The runtime creates a cgroup under `/sys/fs/cgroup/` and writes the new process's PID to that cgroup's `cgroup.procs` file. From now on, all of the process's children are in the same cgroup.
4. The runtime sets resource limits in the cgroup's control files (`memory.max`, `cpu.max`, etc.).
5. The runtime sets up the container's filesystem (typically by mounting an OverlayFS view of a Docker image), changes root via `pivot_root` to point to it, and `exec`s the container's entry-point command.

That is a complete container. Everything Docker, Podman, or Apptainer does is some elaboration of this sequence: image management, network configuration, volume mounting, security-hardening (seccomp filters, capability dropping). The kernel-level core is six steps long.

> **Intuition.** The kernel does not "have containers." It has namespaces and cgroups. A container is a *convention* — a particular pattern of using these mechanisms — that the user-space community has standardised. This is why so many container runtimes exist (Docker, Podman, Apptainer, containerd, crun, gVisor, Kata): they are all implementations of the same convention, with different trade-offs.

### Self-check §2

1. Why is a fresh `/proc` mount required for `ps` to see only the new PID namespace's processes? Trace the chain of file accesses.
2. Suppose a container is created with `--user --pid` but not `--mount`. Will the container see the host's filesystem? Will it be able to mount new filesystems that affect the host? Explain.
3. What is the difference between a *namespace* and a *cgroup*, expressed in one sentence each?
4. A user runs `systemd-run --user --scope --property=CPUQuota=50%` and gets a permission-denied error. Give two possible system configurations under which this would succeed, and one under which it would fail.
5. If you place a process into a cgroup and then move it back out, what happens to its resource accounting? Read the kernel documentation if you do not know.

---

## §3. Docker architecture

Having seen the kernel mechanisms, we now look at the user-space layer that turned them into a usable product. Docker is not one program; it is a stack of four cooperating components, each at a different level of abstraction, with stable interfaces between them. Understanding this stack is essential because it explains how Kubernetes can use part of it (containerd) but not other parts (the Docker daemon), and why the same image runs identically under Docker, Podman, and any OCI-compliant runtime.

### §3.1 The component stack

Working from the user down to the kernel:

```
┌──────────────────────────────────────────────────────────┐
│                     User / CI Pipeline                   │
│                                                          │
│  ┌────────────────┐    ┌──────────────────────────────┐  │
│  │  Docker CLI    │    │  REST API / Docker SDK       │  │
│  │  (docker ...)  │    │  (Python, Go, curl)          │  │
│  └───────┬────────┘    └──────────────┬───────────────┘  │
│          └──────────────┬─────────────┘                  │
│                         ▼                                │
│  ┌──────────────────────────────────────────────────┐    │
│  │              Docker Daemon (dockerd)             │    │
│  │  • Image management  • Network management        │    │
│  │  • Volume management • Build orchestration       │    │
│  └──────────────────────┬───────────────────────────┘    │
│                         │ gRPC                           │
│                         ▼                                │
│  ┌──────────────────────────────────────────────────┐    │
│  │               containerd                         │    │
│  │  • Image pull/push   • Container lifecycle       │    │
│  │  • Snapshot drivers  • Content store             │    │
│  └──────────────────────┬───────────────────────────┘    │
│                         │ OCI Runtime Spec               │
│                         ▼                                │
│  ┌──────────────────────────────────────────────────┐    │
│  │           runc (OCI low-level runtime)           │    │
│  │  • Creates namespaces   • Sets up cgroups        │    │
│  │  • Executes container   • Applies seccomp        │    │
│  └──────────────────────┬───────────────────────────┘    │
│                         ▼                                │
│  ┌──────────────────────────────────────────────────┐    │
│  │              Linux Kernel                        │    │
│  │  namespaces · cgroups · seccomp · OverlayFS      │    │
│  └──────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────┘
```

Each layer has a precise responsibility.

The **Docker CLI** is what users type — `docker run`, `docker build`, `docker push`. It is a thin client; it parses arguments and translates them into REST API calls to the daemon. The CLI itself does no container work and could be replaced with `curl` against the API.

The **Docker daemon (`dockerd`)** is the long-running process that does most of what users associate with "Docker." It manages images (cache, layers, tags), networks (bridges, port forwarding, DNS), volumes, and orchestrates builds (parsing Dockerfiles, executing build steps, producing layered images). It runs as root by default — this is the security property that makes Docker incompatible with HPC, as we discussed in Lecture 03 §10.1. The daemon does not, itself, run containers; it delegates to containerd via gRPC.

**containerd** is the high-level container runtime. Originally developed inside Docker, it was extracted in 2017 and donated to the CNCF, becoming an independent project. containerd handles image transport (pull from registry, unpack layers), snapshots (the on-disk filesystem state of running containers), and container lifecycle (create, start, stop, delete). It does *not* directly create namespaces or cgroups — that is delegated to a low-level runtime.

**runc** is the OCI-compliant low-level runtime. It is a small Go program (a few thousand lines) whose only job is to take an OCI runtime specification (a JSON file describing the container's namespaces, cgroups, mounts, command) and turn it into a running process. runc does the kernel calls — `clone()`, `mount()`, `pivot_root()`, the cgroup file writes — that we walked through in §2.7. After the container is running, runc exits; the container process is reparented to a small "shim" that watches it.

**The Linux kernel**, finally, provides the primitives: namespaces, cgroups, seccomp filters, OverlayFS, security modules.

### §3.2 Why the layered architecture matters

This decomposition is not academic; it has direct practical consequences.

**Kubernetes does not use the Docker daemon.** Since version 1.24 (released in 2022), Kubernetes formally removed Docker as a supported container runtime. Kubernetes now communicates directly with containerd (or its alternative, CRI-O) via the **Container Runtime Interface (CRI)** — a gRPC API that abstracts over the runtime. The Docker daemon, with its monolithic design and root-privileged build/network/volume/runtime logic, was unnecessary for Kubernetes' use case. Docker images, however, still work everywhere because they conform to the OCI image specification, which is independent of the daemon.

If you debug a container crash on Kubernetes, the relevant logs are containerd's, not Docker's. If you debug an image build failure, the Docker daemon is the right place to look — but only on a machine that uses it for building.

**containerd is the industry standard.** AWS ECS, Google GKE, Azure AKS, and most other production container platforms run containerd directly, with no Docker daemon present. Many developer workstations still use Docker; many production systems do not. Understanding the boundary helps you reason about where issues arise.

**The low-level runtime can be swapped.** Because runc is OCI-compliant, you can replace it with another OCI-compliant runtime without changing the rest of the stack. Two notable alternatives:

- **gVisor** (Google) intercepts syscalls in user space and re-implements a subset of the Linux kernel API in Go. The container's processes see what looks like a Linux kernel, but every syscall is filtered and interpreted by gVisor. The result is dramatically stronger isolation — a kernel exploit in the container has nothing real to attack — at the cost of significant performance overhead (10–30% on syscall-heavy workloads).
- **Kata Containers** runs each container in a tiny VM (a "microVM" similar to AWS Firecracker, Lecture 03 §13). Each container has its own kernel and its own memory; the isolation is hardware-level. The performance cost is small (~5%) and the security gain is large; the cost is that containers no longer share resources as cheaply.

You do not need to understand these alternatives in depth, but their existence demonstrates the value of the layered design. The OCI specification turned "container" from a Docker product into an industry standard.

### §3.3 Docker objects

Docker manipulates a small set of named objects. The vocabulary appears throughout the lecture; we summarise it here.

| Object | Description | Mental model |
|---|---|---|
| **Image** | A read-only template containing application, dependencies, configuration | A class definition |
| **Container** | A running (or stopped) instance of an image, with a writable layer on top | An instantiated object |
| **Dockerfile** | A text file with build instructions used to construct an image | A Makefile or build recipe |
| **Volume** | Persistent storage managed by Docker, surviving container deletion | A mounted filesystem |
| **Network** | A virtual network connecting containers | A VLAN or subnet |
| **Registry** | A repository service for storing and distributing images | An apt or yum repository |

The relationship `image : container :: class : object` is genuinely useful and worth internalising. An image is built once and is immutable. A container is instantiated from an image; many containers can be instantiated from one image; each container has its own writable layer (its mutable state) but shares the underlying image layers (its immutable state) with all other containers from the same image. Stopping a container does not destroy it; the writable layer persists until the container is removed (`docker rm`). Re-running the image creates a new container, with its own fresh writable layer — independent from the previous one.

We will spend §4 examining the image layer model in depth, because it is responsible for many of Docker's distinctive properties — fast deploys, efficient storage, and the famous build-cache behaviour that you can either exploit or fight.

### Self-check §3

1. Trace, step by step, what happens when you type `docker run nginx`. Identify which component does each piece of work.
2. The Docker daemon runs as root. Why is this incompatible with HPC clusters? (Refer back to Lecture 03 §10.1 if needed.)
3. Kubernetes 1.24 removed Docker support but still runs Docker images. How are these two facts compatible?
4. Suppose you replace runc with gVisor on a production node. Which other parts of the stack do you need to reconfigure? Which keep working unchanged?

---

*Sections 4–11, concluding remarks, glossary, and references continue in Parts 2 and 3 of these notes.*

## §4. Docker images, in depth

We have spoken loosely of "images" several times. It is now time to be precise. An image is the artefact you ship — the thing you push to a registry, pull on another machine, and run as a container. Understanding its internal structure explains a long list of otherwise mysterious Docker behaviours: why builds are sometimes instantaneous and sometimes take hours, why the order of Dockerfile instructions matters, why a 3 GB image somehow takes only 200 MB of disk space when ten copies are running.

The whole story rests on one design choice: an image is not a single blob but a stack of *layers*, each immutable, each addressed by the cryptographic hash of its contents. Once you understand layers, everything else about Docker images follows.

### §4.1 The layer model

A Docker image is an ordered collection of read-only filesystem layers, each produced by a single Dockerfile instruction. Layers are stacked using a **union filesystem** — almost always OverlayFS on Linux — to present, to the running container, a single unified view.

```
When running a container:

  ┌──────────────────────────────────┐
  │   Container Layer (Read-Write)   │ ◄── Writable; deleted with container
  ╞══════════════════════════════════╡
  │ Layer 5: COPY app.py /app/       │ ◄── Application code
  ├──────────────────────────────────┤
  │ Layer 4: RUN pip install flask   │ ◄── Python packages
  ├──────────────────────────────────┤
  │ Layer 3: COPY requirements.txt   │ ◄── Dependency manifest
  ├──────────────────────────────────┤
  │ Layer 2: RUN apt-get update ...  │ ◄── System packages
  ├──────────────────────────────────┤
  │ Layer 1: FROM python:3.11-slim   │ ◄── Base image layers
  └──────────────────────────────────┘

  All image layers are READ-ONLY.
  When the container modifies a file from an image layer,
  it is COPIED to the writable layer first (Copy-on-Write).
```

When a container is launched, OverlayFS combines these layers into a single filesystem view: a file in layer 5 shadows the same path in layer 1; a directory listing shows the union of entries from all layers; the topmost writable layer captures any modifications the running container makes. The container has no awareness of the layered structure — it sees a single filesystem rooted at `/`.

> **Intuition.** Think of layers as a stack of glass plates, each with words written on it. Looking down from above, you see the top word for each position, with the words on lower plates visible only where the upper plates are blank. Adding a new layer to the top is cheap; modifying a word means dropping a new plate on top with the new word — the old plate is unchanged.

The benefits of this model are pervasive:

**Sharing across containers.** If ten containers are launched from the same `python:3.11-slim` image, the underlying base layers exist exactly once on disk. Each container has its own thin writable layer, but the heavy read-only material is shared. This is why `docker run` is fast — there is no image to copy, only a writable overlay to create.

**Efficient transfers.** When pushing or pulling images, Docker transfers only those layers not already present at the destination. Two images sharing a `python:3.11-slim` base will share most of their bytes; only the differing top layers cross the network. This is the same principle that made content-addressed storage (Git, IPFS) influential.

**Build cache.** Each layer's identity is a hash of its instruction plus its inputs. If the instruction and inputs have not changed since a previous build, Docker reuses the cached layer and skips the work. This is the property that makes incremental development bearable: editing one Python file should not trigger a fifteen-minute reinstall of NumPy.

**Storage efficiency at scale.** On a host running fifty containers from twenty related images, layer deduplication can save tens of gigabytes. Production container hosts rely on this implicitly.

### §4.2 Copy-on-Write semantics

The phrase "OverlayFS combines layers" hides one subtlety that matters for performance. When a container writes to a file that exists in an image layer, the file is not modified in place — image layers are read-only. Instead, OverlayFS copies the file to the topmost writable layer first, and the modification is applied to the copy. This is *Copy-on-Write* (CoW), the same principle we encountered in Lecture 03's discussion of QCOW2 disk images.

The performance implication is subtle but real. The first write to any file from an image layer pays a copy cost proportional to the file's size. Subsequent writes to the same file go to the now-local copy and run at native speed. For most application workloads this is invisible — files written to are usually small, copied once, then written many times. For some workloads it becomes visible: a database that mutates a multi-gigabyte file, a scientific code that rewrites parts of a large checkpoint, an ML training run that overwrites large weights. For these cases, the recommendation is consistent: do not write to the container layer; write to a *volume* (which we examine in §6) where there is no CoW indirection.

### §4.3 Dockerfile instructions

A Dockerfile is a text file containing a sequence of build instructions. Each instruction either creates a new layer or modifies the build's metadata. Understanding which is which is the key to writing efficient Dockerfiles.

| Instruction | Purpose | New layer? | Example |
|---|---|:-:|---|
| `FROM` | Set base image | Yes | `FROM python:3.11-slim` |
| `RUN` | Execute command at build time | Yes | `RUN pip install numpy` |
| `COPY` | Copy files from build context | Yes | `COPY src/ /app/src/` |
| `ADD` | Like COPY but auto-extracts archives, fetches URLs | Yes | `ADD data.tar.gz /data/` |
| `WORKDIR` | Set working directory | No (metadata) | `WORKDIR /app` |
| `ENV` | Set environment variable (build & run) | No (metadata) | `ENV PYTHONUNBUFFERED=1` |
| `ARG` | Build-time-only variable | No (metadata) | `ARG CUDA_VERSION=12.2` |
| `EXPOSE` | Document a port (does not publish) | No (metadata) | `EXPOSE 8888` |
| `CMD` | Default command (overridable at run) | No (metadata) | `CMD ["python", "app.py"]` |
| `ENTRYPOINT` | Fixed command prefix (args appended) | No (metadata) | `ENTRYPOINT ["python"]` |
| `USER` | Set UID for subsequent instructions | No (metadata) | `USER 1000` |
| `HEALTHCHECK` | Define container health probe | No (metadata) | `HEALTHCHECK CMD curl -f /` |
| `LABEL` | Add metadata key-value pairs | No (metadata) | `LABEL maintainer="x@y.it"` |

Two pairs of instructions deserve direct comparison.

**`COPY` versus `ADD`.** Both copy files from the build context into the image. `ADD` additionally auto-extracts tar archives at the destination and can fetch URLs. The auto-extraction is occasionally convenient; the URL fetching is almost always a mistake — it produces non-cacheable layers (the URL might serve different content tomorrow), gives no error handling for network failures, and obscures the build's reproducibility. Use `COPY` by default; reach for `ADD` only when you specifically want tar extraction.

**`CMD` versus `ENTRYPOINT`.** Both define what the container runs by default. `CMD` is overridable: `docker run myimage some_other_command` replaces the CMD entirely. `ENTRYPOINT` is fixed; arguments to `docker run` are *appended* to it. They can also combine: with `ENTRYPOINT ["python"]` and `CMD ["app.py"]`, plain `docker run myimage` executes `python app.py`, while `docker run myimage other_script.py` executes `python other_script.py`. Use `ENTRYPOINT` when the container should always run the same executable (a scientific simulation binary, a CLI tool); use `CMD` for general-purpose images where users may want to override.

### §4.4 The build cache and instruction ordering

We can now state precisely the rule that governs Dockerfile efficiency. **Docker reuses a cached layer if and only if the instruction's text and its inputs are identical to a previous build. The moment any layer is invalidated, all subsequent layers are rebuilt.**

This second clause — cascading invalidation — is what makes instruction ordering critical. Compare two Dockerfiles for the same Python application.

```dockerfile
# BAD — every code change reinstalls all dependencies
FROM python:3.11-slim
COPY . /app                          # code changes invalidate this layer
WORKDIR /app
RUN pip install -r requirements.txt  # ...and therefore this layer reruns
CMD ["python", "app.py"]
```

```dockerfile
# GOOD — dependencies cached until requirements.txt itself changes
FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .              # changes rarely → cache valid
RUN pip install -r requirements.txt  # rerun only when requirements change
COPY . .                             # only this layer is rebuilt on code changes
CMD ["python", "app.py"]
```

In the bad version, every edit to a Python source file invalidates the `COPY . /app` layer, which in turn forces `pip install` to rerun on the next layer — even though `requirements.txt` did not change. The fifteen-minute install repeats every iteration.

In the good version, the rare event (changing dependencies) and the frequent event (changing application code) are separated into distinct layers. Editing application code invalidates only the final `COPY . .` layer; the expensive dependency installation is reused from cache.

The general rule is therefore: **order instructions from least-frequently changing to most-frequently changing.** Base image, system packages, language runtime, dependency manifest, dependency installation, application code — that is the canonical sequence.

> **Try it.** Take any Dockerfile from a project you have used recently. Identify the first instruction that depends on something that changes often (your application code, configuration). Everything below that instruction reruns on every build. Move slow operations above; verify by rebuilding twice and observing which steps say `CACHED`.

### §4.5 The build context and `.dockerignore`

A subtlety that catches first-time Dockerfile users: when you run `docker build .`, the trailing `.` is not just "build in this directory." It is the **build context** — Docker tars up the entire contents of that directory and sends them to the daemon, where they become the source for any `COPY` and `ADD` instructions.

This has performance implications. A directory containing a 5 GB dataset, a `.git` history, a `node_modules/` folder, or a Python virtualenv will see all of those bytes streamed to the daemon at the start of every build, even if the Dockerfile copies only a single file. Build times balloon for no apparent reason; build logs become unreadable.

The remedy is a `.dockerignore` file in the build context root, with the same syntax as `.gitignore`:

```
.git
__pycache__
*.pyc
node_modules
*.log
.env
data/
.venv/
build/
dist/
*.egg-info
```

This is a small file with disproportionate impact. Make a habit of writing one for every project.

### §4.6 Multi-stage builds

A common problem in scientific imagery: building the application requires compilers, header files, development libraries that are useless at runtime. A naive Dockerfile that installs GCC, builds a C++ simulation, and ships the result in a single image will carry around 1–2 GB of build tools to every production node — for nothing.

**Multi-stage builds** address this by allowing multiple `FROM` instructions in a single Dockerfile, each starting a fresh stage. Only artefacts explicitly copied across stages are carried forward.

```dockerfile
# Stage 1: build
FROM gcc:13 AS builder
WORKDIR /build
COPY src/ .
RUN g++ -O3 -march=x86-64-v3 -o simulation main.cpp \
    -lm -lfftw3 -lopenmpi

# Stage 2: runtime
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libfftw3-3 libopenmpi3 && \
    rm -rf /var/lib/apt/lists/*
COPY --from=builder /build/simulation /usr/local/bin/
ENTRYPOINT ["simulation"]
```

The builder stage may grow to 1.5 GB (GCC, headers, build artefacts, intermediate object files). It exists only during the build. The runtime stage starts fresh from `ubuntu:22.04` (~80 MB), installs only the runtime libraries, and copies the compiled binary across. The shipped image is perhaps 150 MB — an order of magnitude smaller, with a correspondingly smaller attack surface (no compilers to be exploited in a compromised container).

The pattern generalises beyond compiled languages. For Python, where wheel building can pull in tens of compile-time dependencies, the multi-stage equivalent is:

```dockerfile
# Stage 1: build wheels
FROM python:3.11 AS builder
WORKDIR /build
COPY requirements.txt .
RUN pip wheel --no-cache-dir --wheel-dir /wheels -r requirements.txt

# Stage 2: minimal runtime
FROM python:3.11-slim
COPY --from=builder /wheels /wheels
RUN pip install --no-cache-dir /wheels/* && rm -rf /wheels
WORKDIR /app
COPY src/ .
CMD ["python", "main.py"]
```

The full `python:3.11` image (~900 MB, including build tools) is used to compile any C-extension wheels (`numpy`, `scipy`, `cryptography`); the compiled wheels are copied into the slim runtime (~120 MB), where pip installs them without recompiling.

Multi-stage builds are the standard pattern for production HPC images. We will use them again in Lecture 06 and the Docker tutorial.

### §4.7 Tagging, versioning, and reproducibility

A complete image reference has the form

```
registry.example.com/organization/repository:tag
└──────┬───────────┘ └─────┬──────┘ └───┬────┘ └┬┘
    registry         namespace      image    tag
```

Each part can be omitted with reasonable defaults: omitting the registry implies Docker Hub (`docker.io`); omitting the namespace implies the registry's `library/` (used for official images like `python`, `nginx`); omitting the tag implies `:latest`.

The `:latest` default deserves a warning. Despite its name, `:latest` is not in any way special to Docker — it is just a tag, and like any tag it can be re-pushed to point to a different image at any time. A deployment that uses `python:latest` today and `python:latest` tomorrow may be running entirely different software, with no record of what changed. **Never use `:latest` in production or in published Dockerfiles.** Use explicit version tags.

Three conventions for versioning are worth knowing:

- **Semantic versioning** (`myapp:1.2.3`) for human-meaningful releases. Major.Minor.Patch with the usual semantics.
- **Git SHA tagging** (`myapp:a3f5b2c`) for CI/CD: every commit produces an image whose tag is the commit hash. Provides exact build provenance.
- **Digest pinning** (`python@sha256:abc123...`) for maximum reproducibility: the image is referenced by the cryptographic hash of its content, immutable even if the tag is later re-pushed.

For research code that you want to be reproducible six months from now, digest pinning is the gold standard. For development work, SemVer or git-SHA tags are the practical baseline.

### Self-check §4

1. Suppose you change line 50 of an application source file and rebuild. Which Dockerfile layers are rebuilt? Which are reused? Sketch the full set in order.
2. Why is writing to a *volume* faster than writing to the container's writable layer for large files?
3. Could the `python:3.11-slim` runtime stage of the multi-stage Python example be replaced with `FROM scratch`? What would have to change?
4. A colleague tells you "we always build from `:latest`, that way we get the newest version." Articulate the two distinct problems with this practice.

---

## §5. Container networking

A container, by virtue of its network namespace, has its own private network stack: its own interfaces, its own routing table, its own firewall rules, its own port space. To communicate with anything outside its namespace — another container, the host, the public internet — explicit configuration is required. Docker provides this configuration through *network drivers*, and the default behaviour is good enough that most users never look beyond it. For HPC workloads, however, the defaults are often wrong, and understanding what they actually do is essential.

### §5.1 Network drivers

Docker supports several network drivers, each suited to different scenarios. The five you should know:

| Driver | Scope | Typical use | DNS by container name |
|---|---|---|:-:|
| `bridge` | Single host | Default; containers on isolated private network | Yes (user-defined only) |
| `host` | Single host | Container shares host's network stack directly | N/A |
| `none` | Single host | No networking at all | N/A |
| `overlay` | Multi-host | Cross-host communication (Swarm, Kubernetes) | Yes |
| `macvlan` | Single host | Container gets a MAC address on the physical network | No |

The `bridge` driver is the default — when you run a container without specifying a network, Docker attaches it to a default bridge called `docker0`. The `host` driver is the simplest: the container shares the host's network namespace entirely, with no isolation. The `overlay` driver is what makes multi-host orchestration possible — it builds VXLAN-based networks that span machines, exactly the mechanism we examined in Lecture 03 §11. The `macvlan` driver gives a container its own MAC address on the physical network, useful when integrating containers with legacy network appliances. The `none` driver gives the container a network namespace but no interfaces — useful for security-sensitive batch jobs that should not network at all.

We will spend most of our attention on `bridge` (the default) and `host` (the HPC-relevant alternative).

### §5.2 The bridge driver, in detail

The default bridge network creates an isolated Layer 2 network on the host. Docker manages a virtual bridge (`docker0` for the default network, `br-XXXX` for user-defined networks) and connects each container to it via a *veth pair*: a virtual Ethernet cable with one end inside the container's network namespace and the other end attached to the bridge.

```
Docker Host (10.0.0.5)
┌─────────────────────────────────────────────────────┐
│                                                     │
│   ┌───────────┐   ┌───────────┐   ┌───────────┐     │
│   │ Container │   │ Container │   │ Container │     │
│   │ nginx     │   │ flask     │   │ postgres  │     │
│   │ 172.18.   │   │ 172.18.   │   │ 172.18.   │     │
│   │  0.2:80   │   │  0.3:5000 │   │  0.4:5432 │     │
│   └─────┬─────┘   └─────┬─────┘   └─────┬─────┘     │
│    veth0│          veth1│          veth2│           │
│         └────────────┬──┴───────────────┘           │
│                      │                              │
│               ┌──────┴──────┐                       │
│               │  br-mynet   │ (user-defined bridge) │
│               │  172.18.0.1 │                       │
│               └──────┬──────┘                       │
│                      │                              │
│               ┌──────┴──────┐                       │
│               │  iptables   │ NAT + port mapping    │
│               │  MASQUERADE │                       │
│               └──────┬──────┘                       │
│                      │                              │
│               ┌──────┴──────┐                       │
│               │   eth0      │ 10.0.0.5              │
│               └─────────────┘                       │
└─────────────────────────────────────────────────────┘
```

Each container receives a private IP on a subnet specific to its network (172.18.0.0/16 by default for user-defined bridges; 172.17.0.0/16 for the default `docker0`). Outbound traffic from a container is NATed by iptables — the container's private IP is rewritten to the host's IP as packets leave the host. Inbound traffic relies on explicit *port publishing* (`docker run -p 8080:80`), which installs an iptables rule mapping a host port to a container port.

The default `docker0` bridge has historical baggage. It is the network you get when you do not specify one, but it lacks **automatic DNS resolution by container name** — containers on `docker0` see each other only by IP, and IPs are dynamic. A user-defined bridge (`docker network create mynet`) adds two important features over `docker0`:

1. **Embedded DNS**: containers on a user-defined bridge resolve each other by container name, automatically and without explicit configuration. `nginx` can connect to `database` simply by hostname.
2. **Network-level isolation**: containers on different user-defined networks cannot communicate unless explicitly attached to both.

The practical consequence: **always create a user-defined bridge for any project involving more than one container.** The default `docker0` exists for backward compatibility; modern usage avoids it.

```bash
# Create a user-defined bridge
docker network create science-net

# Start cooperating containers on it
docker run -d --name db    --network science-net postgres:15
docker run -d --name web   --network science-net -p 8080:5000 myapp

# Inside the 'web' container:
#   curl http://db:5432   ← resolves via Docker's embedded DNS
#                           to db's IP on science-net
```

> **Formally.** Docker's embedded DNS server runs at 127.0.0.11 inside every container attached to a user-defined network. Each container's `/etc/resolv.conf` is configured to query this address. The DNS server resolves container names to IPs by consulting Docker's internal record of which containers are on which network.

### §5.3 Port publishing

A container that listens on port 80 inside its network namespace is reachable from the host *only* if the port is published. Three syntaxes are used:

```bash
docker run -p 8080:80 nginx               # host:8080 → container:80, all interfaces
docker run -p 127.0.0.1:8080:80 nginx     # only localhost on host can reach
docker run -P nginx                       # publish all EXPOSEd ports to random
                                          # high host ports (rarely useful)
```

The first form is by far the most common. The second form binds only to the loopback interface — useful when you want a service accessible to other processes on the same host but not to anything on the network. The third form is mostly useful for testing.

> **A subtle and frequent error.** A service inside the container must bind to `0.0.0.0` (all interfaces in the container's network namespace) for port publishing to work. If the service binds to `127.0.0.1` *inside the container*, port publishing will appear to be configured but no traffic will reach the service — `127.0.0.1` is only the container's loopback, not reachable from outside its network namespace. This is the most common networking bug for new container users.

### §5.4 Host networking and the HPC trade-off

The `bridge` driver provides isolation through NAT and iptables. For most workloads this is a reasonable trade-off: a small per-packet latency in exchange for clean network separation between containers. For HPC, the trade-off changes sign.

Consider an MPI application whose ranks need to exchange messages over an InfiniBand fabric. Two costs accumulate at the bridge driver:

1. **Latency from NAT**. Every packet crosses iptables twice (in and out), pays the bridge's switching cost, and traverses two veth pairs. Microseconds, but on a fabric whose native one-way latency is ~1 µs, this is multiplicative.
2. **Loss of fabric-aware addressing**. InfiniBand's RDMA semantics depend on direct addressing of remote memory regions; this does not work through the kernel's bridge code. Effectively, you cannot use IB with bridge networking.

The `host` driver removes both costs by skipping the network namespace entirely. The container shares the host's network stack — it sees the host's interfaces, the host's routing table, the host's port space. There is no isolation, but there is also no overhead:

```bash
docker run --network host nginx
```

This is the default in HPC-optimised runtimes. **Apptainer and Sarus run with host networking by default**; isolation, in those settings, comes from cgroups and file system mounts rather than from network namespaces. The cost is that two containers cannot use the same port (because there is only one port space, the host's), but on HPC each rank typically owns its own node anyway.

> **HPC implication.** When designing for InfiniBand or similar high-performance interconnects, accept that network isolation is fundamentally incompatible with kernel-bypass RDMA. Use host networking, control sharing through scheduler-level allocation (Slurm gives you the node), and rely on the cluster's overall security model. This is exactly what Apptainer, Sarus, and similar HPC runtimes do, and it is the right answer.

### Self-check §5

1. Why must a service inside a container bind to `0.0.0.0` rather than `127.0.0.1` for `-p host:container` port publishing to work?
2. Two containers on different user-defined bridges. Without changing networks, can they communicate? What configuration would let them?
3. Trace the path of a single TCP packet from a container on a bridge network to an external host on the internet. Identify each translation point.
4. Under what circumstances is `--network none` a sensible default? Give a concrete HPC example.

---

## §6. Container storage

A running container's filesystem appears, from inside, to be a normal Linux directory tree. Beneath the appearance, however, it is the union-filesystem stack we examined in §4: a stack of read-only image layers plus one writable layer on top. The writable layer is *ephemeral* — it lives and dies with the container. Anything written there is lost when the container is removed. For scientific computing, where simulations may run for hours and produce gigabytes of output, this ephemerality is unacceptable. Persistent data must live elsewhere.

Docker provides three mechanisms for managing data outside the container's writable layer: **volumes**, **bind mounts**, and **tmpfs mounts**. The choice between them shapes the reproducibility, performance, and security characteristics of a containerised pipeline.

### §6.1 The ephemeral filesystem problem

Why is the writable layer ephemeral? Because it is part of the container, and Docker's lifecycle model treats containers as disposable: `docker rm` removes a container, including everything it has written that is not on a mount. This is intentional. Treating containers as disposable is what allows you to upgrade an application by removing the old container and starting a new one from an updated image, with no fear of losing important state — because important state, by discipline, is never in the container itself.

This discipline must be learned. The default writable layer is *seductive* — it works, you can write to it, things appear to persist between restarts. They do, until someone types `docker rm`. The professional habit is to assume the writable layer is throwaway and put anything you care about somewhere else.

The performance consequence reinforces the point. As we noted in §4.2, every first write to a file from an image layer pays a Copy-on-Write penalty. For small files this is invisible; for databases, large logs, scientific output files, the cost is significant. Volumes and bind mounts skip CoW entirely — they write to host-native storage at native speed.

### §6.2 The three storage mechanisms

The choice of mechanism is the choice of *where the data physically lives* and *who manages it*.

| Property | Volumes | Bind mounts | tmpfs mounts |
|---|---|---|---|
| Managed by | Docker daemon | User (host path) | Kernel (RAM) |
| Location on host | `/var/lib/docker/volumes/` | Any host directory | Memory only |
| Persists after `docker rm` | Yes | N/A (host data unchanged) | No |
| Shareable between containers | Yes | Yes | No |
| Performance | Near-native | Native | Fastest (RAM-backed) |
| Portability | Docker-managed; works on all OSes | Requires host path to exist | Linux only |
| Security | Isolated from host | Container can modify host files | Never touches disk |
| Best for | Databases, output data | Development, input data | Secrets, scratch space |

A **volume** is a piece of storage that Docker creates and manages. The user names it (`docker volume create sim-results`) and references it by name; Docker decides where on the host it actually lives. The data persists across container removal and can be shared between containers.

A **bind mount** maps a specific host directory into the container at a specific path. The user owns the host path; Docker just does the mount. The advantage is direct visibility from the host — you can `ls` the directory, edit files in it with a host editor, copy files in and out with normal tools. The disadvantage is portability: the bind mount only works on machines where the same host path exists, with the same content.

A **tmpfs mount** creates a RAM-backed filesystem inside the container's mount namespace. It is fast, never touches disk, and disappears when the container stops. Useful for scratch data and secrets.

The HPC-relevant rule of thumb: bind mounts for input data (already organised in your scratch directory; you want to see results from the host), volumes for output data that should outlive specific containers, tmpfs for ephemeral computation that must not hit disk for performance or security reasons.

### §6.3 Working with volumes

```bash
# Create a named volume
docker volume create sim-results

# Run a simulation that writes to the volume
docker run --name sim -v sim-results:/output simulation:v1

# Even after the container is removed, the volume persists
docker rm sim
docker volume ls               # sim-results still listed

# Mount the same volume in another container for analysis
docker run -v sim-results:/data:ro analysis:v1 python analyze.py

# Remove the volume when no longer needed
docker volume rm sim-results
```

The `:ro` suffix mounts read-only — a small but important detail for the analysis container. If two containers share a volume and only one writes, marking the others read-only documents intent and prevents accidents.

### §6.4 Storage patterns for scientific computing

Scientific workflows typically involve several data categories with different access patterns, security requirements, and persistence needs. Mapping each category to the right storage mechanism is part of designing a containerised pipeline.

| Data category | Mechanism | Mount mode | Rationale |
|---|---|:-:|---|
| Input datasets (FASTQ, HDF5, simulation parameters) | Bind mount | `:ro` | Already on host; read-only prevents accidental corruption |
| Reference databases (UniProt, BLAST DBs) | Named volume | `:ro` | Shared across containers; downloaded once, reused many times |
| Intermediate / scratch files | tmpfs | — | High I/O, no persistence needed; avoids disk wear |
| Simulation output | Named volume | `:rw` | Must persist; accessible to post-processing containers |
| Configuration files | Bind mount | `:ro` | Easy to edit on host; immutable to container |
| Secrets (API keys, tokens) | tmpfs or Docker secrets | — | Never written to disk; in-memory only |

The recurring `:ro` for input is a discipline worth internalising. A read-only mount is a contract: *the container may not modify this data, even by accident*. On HPC pipelines where the same input dataset feeds many analyses, this contract is what guarantees that one buggy analysis cannot poison the others. The pattern is identical to the one that emerges naturally on Leonardo and other clusters, where input directories are kept read-only by convention.

### Self-check §6

1. Why is writing to a volume faster than writing to the container's writable layer for a 1 GB file?
2. Suppose you bind-mount `/scratch/morgan/inputs:/data/in:ro` and the container tries to delete a file under `/data/in`. What happens?
3. A multi-container pipeline produces results that the next container in line consumes. Which storage mechanism is appropriate, and why not the others?
4. Why are secrets best stored in tmpfs rather than in a volume?

---

## §7. Docker Compose

Real scientific workflows rarely fit in a single container. A typical analysis pipeline involves several cooperating services: a Jupyter front-end for interactive exploration, a Postgres database for metadata, a worker pool for parallel computation, perhaps a message queue for task distribution. Managing all of these with individual `docker run` commands is possible but error-prone — you must remember network names, volume names, environment variables, port mappings, and the correct startup order. A small mistake renders the pipeline non-reproducible.

**Docker Compose** addresses this by replacing imperative commands with a declarative YAML file describing the entire application stack. A single command (`docker compose up`) creates all networks, volumes, and containers; another (`docker compose down`) tears them down. The YAML file is a versionable artefact that captures the topology of the application — exactly the same role that the Dockerfile plays for a single image.

### §7.1 Anatomy of a Compose file

A complete example, taken from the molecular dynamics analysis pipeline that will appear in the exercises:

```yaml
# docker-compose.yml — molecular dynamics analysis pipeline
services:
  jupyter:
    image: jupyter/scipy-notebook:latest
    ports:
      - "8888:8888"
    volumes:
      - ./notebooks:/home/jovyan/work
      - results:/home/jovyan/results
    environment:
      - JUPYTER_ENABLE_LAB=yes
    depends_on:
      database:
        condition: service_healthy

  database:
    image: postgres:15
    environment:
      POSTGRES_DB: simulations
      POSTGRES_USER: researcher
      POSTGRES_PASSWORD_FILE: /run/secrets/db_password
    volumes:
      - db-data:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U researcher"]
      interval: 10s
      timeout: 5s
      retries: 5
    secrets:
      - db_password

  worker:
    build:
      context: ./worker
      dockerfile: Dockerfile
    deploy:
      replicas: 4
      resources:
        limits:
          cpus: '2.0'
          memory: 4G
    volumes:
      - ./input-data:/data/input:ro
      - results:/data/output
    depends_on:
      database:
        condition: service_healthy

volumes:
  results:
  db-data:

secrets:
  db_password:
    file: ./secrets/db_password.txt
```

Three top-level keys structure the file: `services` (the containers and how they relate), `volumes` (named volumes Compose will create), and `secrets` (sensitive values mounted as files). Each service has a name (`jupyter`, `database`, `worker`) which doubles as its DNS hostname on the project's auto-created network — the `jupyter` container reaches the database simply as `database:5432`.

Several features of this file are worth examining individually.

### §7.2 Dependency management with health checks

The `depends_on` clause with `condition: service_healthy` is the modern way to manage startup order. In a naive Compose file, `depends_on: [database]` only waits for the database container to *start* — not to be ready to accept connections. For a Postgres database that takes ten seconds to initialise, this is the difference between a working pipeline and a series of confusing connection refusals on the first run.

The healthcheck block defines what "ready" means for the database:

```yaml
healthcheck:
  test: ["CMD-SHELL", "pg_isready -U researcher"]
  interval: 10s
  timeout: 5s
  retries: 5
```

Compose runs `pg_isready` every 10 seconds. The database is `service_healthy` once `pg_isready` succeeds. Only then does the dependent `jupyter` service start.

This is the same machinery Kubernetes uses for *liveness* and *readiness* probes. Learning it in Compose pays dividends when we move to Kubernetes in Lecture 05.

### §7.3 Resource limits

The `deploy.resources.limits` section maps directly to the cgroups constraints we examined in §2.5:

```yaml
deploy:
  resources:
    limits:
      cpus: '2.0'
      memory: 4G
```

This translates to `--cpus=2.0 --memory=4g` on `docker run`, which translates to cgroup writes — `cpu.max 200000 100000`, `memory.max 4294967296`. A runaway worker cannot consume the host's resources past these bounds. For shared-host pipelines, this is essential discipline.

### §7.4 Secrets

Secrets — passwords, API keys, certificates — pose a recurring problem in containerised systems. The temptation is to set them as environment variables, which is easy and works, but environment variables are visible in `docker inspect` output, in process listings (`ps -ef` may show them in the command line), and in logs that capture environment dumps. For anything more sensitive than a development password, this is a leak.

Compose's `secrets` mechanism mounts secret values as files in `/run/secrets/`:

```yaml
secrets:
  db_password:
    file: ./secrets/db_password.txt
```

The application reads the file at startup. The secret never appears in the container's environment, never shows up in `docker inspect`, never leaks into logs. The file is mounted as `tmpfs` so it is never written to disk inside the container.

This is the pattern Kubernetes calls a *Secret*. Same idea, same threat model, same mitigation.

### §7.5 The Compose command line

| Command | Effect |
|---|---|
| `docker compose up -d` | Create and start all services in detached mode |
| `docker compose down` | Stop and remove containers and networks (volumes preserved) |
| `docker compose down -v` | Also remove named volumes (caution: data loss) |
| `docker compose ps` | List running services and their status |
| `docker compose logs -f worker` | Follow logs of a specific service |
| `docker compose exec jupyter bash` | Open a shell in a running service |
| `docker compose build` | Build or rebuild images defined with `build:` |
| `docker compose pull` | Pull latest images for all services |
| `docker compose config` | Validate and display the resolved configuration |

The networking is handled automatically: Compose creates a dedicated bridge network for each project and attaches all services to it. Service names resolve to container IPs via embedded DNS. Cross-project communication, when needed, uses the standard `docker network connect` mechanism.

### §7.6 Compose, Kubernetes, and the road forward

Docker Compose is excellent for development, single-host deployments, and small production systems. It is intentionally simple — one machine, one network, declarative YAML.

For multi-host orchestration, automated scaling, rolling updates, and high availability, the industry standard is **Kubernetes**, which we will study in Lectures 05 and 06. The mental model carries forward: Compose's `services` become Kubernetes `Deployments`, Compose's `volumes` become `PersistentVolumeClaims`, Compose's `secrets` become Kubernetes `Secrets`, the project network becomes a `Service` mesh. The vocabulary expands and the abstraction generalises across machines, but the problems Compose addresses (multi-container topology, declarative configuration, dependency management) are the same problems Kubernetes addresses on a larger scale.

Treat Compose as the entry point. The reasoning skills you develop with Compose YAML transfer directly to Kubernetes manifests.

### Self-check §7

1. A Compose file declares two services, `web` and `database`, with `web` having `depends_on: [database]` (no health check). The database takes 8 seconds to initialise. Predict the behaviour on first start, and explain how to fix it.
2. Why are environment-variable-based secrets considered a security mistake? Give two concrete leakage paths.
3. The `worker` service in the example uses `replicas: 4`. What does this produce, and how is the workload distributed among the four replicas?
4. A pipeline runs nightly and produces ~50 GB of output. Which Compose storage construct should hold the output, and why not the alternatives?

---

*Sections 8–11, concluding remarks, glossary, and references continue in Part 3.*

## §8. Container registries

A container registry is a server that stores container images and serves them on demand. It is the equivalent of an apt repository for packages, a Maven repository for Java artefacts, or a Git remote for source code. Without registries, every machine that wanted to run a container would need to build it locally — a contradiction of the whole point of containerisation.

The vocabulary is straightforward. You **build** an image locally with `docker build`, you **tag** it with a registry-qualified name, you **push** it to the registry, and others (or your future self on another machine) **pull** it. Pulling is what `docker run` does silently when the image is not already present locally.

### §8.1 The registry landscape

The major registries you will encounter:

| Registry | URL pattern | Notes |
|---|---|---|
| Docker Hub | `docker.io` | The default registry; rate-limited on the free tier (100 pulls per 6 h per IP) |
| GitHub Container Registry | `ghcr.io` | Integrated with GitHub Actions; free for public images |
| Amazon ECR | `<account>.dkr.ecr.<region>.amazonaws.com` | AWS-native; IAM authentication |
| Google Artifact Registry | `<region>-docker.pkg.dev` | GCP-native; replaced the older Google Container Registry |
| Azure Container Registry | `<n>.azurecr.io` | Azure-native; integrates with Azure Active Directory |
| Quay.io | `quay.io` | Red Hat; built-in vulnerability scanning |
| Harbor | self-hosted | Open-source CNCF project; full institutional control |

Three observations are useful. First, every cloud provider has its own registry, integrated with that cloud's identity and access management — this matters because pulling an image from a private registry requires authentication, and the cloud-native registries make this almost transparent inside the cloud, more painful outside. Second, **Docker Hub has rate limits** on the free tier; in CI pipelines that build frequently, this becomes a recurring annoyance and has driven many projects to GHCR or a private mirror. Third, for institutional use (a lab's internal images, an institute's archive of reproducible research environments), self-hosted Harbor is the standard answer.

### §8.2 The push/pull workflow

A typical end-to-end flow:

```bash
# 1. Build with a registry-qualified tag
docker build -t ghcr.io/myorg/simulation:v2.1.0 .

# 2. Authenticate to the registry
echo "$GITHUB_TOKEN" | docker login ghcr.io -u USERNAME --password-stdin

# 3. Push
docker push ghcr.io/myorg/simulation:v2.1.0

# 4. On another machine — or in a CI step, or on a cluster
docker pull ghcr.io/myorg/simulation:v2.1.0
docker run ghcr.io/myorg/simulation:v2.1.0
```

The tag is the registry-qualified name. The first segment (`ghcr.io`) tells Docker which registry to talk to; the remainder identifies the image within that registry. If you push without the registry prefix, Docker assumes Docker Hub. If you forget the namespace, the image goes to your personal namespace on Docker Hub — a frequent first-time accident.

### §8.3 The CI/CD handoff

In practice, the registry is the *handoff point* between two pipelines that may run on different infrastructure with different security postures.

```
Developer pushes code → CI builds image → CI tags with git SHA
                     → CI pushes to registry → CD pulls specific tag
                     → CD deploys to cluster
```

Each step is automated. The CI system (GitHub Actions, GitLab CI, Jenkins) builds the image, tags it with the commit hash plus any release tags, runs tests, scans for vulnerabilities, and pushes. The CD system (ArgoCD, Flux, Kubernetes itself) pulls by tag and deploys. The *only* shared state between CI and CD is the registry. This is one of the more important architectural insights of modern container ops, and it is the reason registry availability is treated as a first-class production concern.

### §8.4 Image signing and verification

A registry, by default, lets anyone with credentials push any image with any tag. There is no cryptographic guarantee that the image you pulled today is the image its purported author published. For internal use among trusted teams this is usually acceptable; for software supply-chain integrity it is not.

The modern answer is **cosign**, part of the Sigstore project, which lets you cryptographically sign images and verify signatures before deployment:

```bash
# Sign an image
cosign sign --key cosign.key ghcr.io/myorg/simulation:v2.1.0

# Verify before running
cosign verify --key cosign.pub ghcr.io/myorg/simulation:v2.1.0
```

This is increasingly required in regulated production environments and is recommended as a research practice: signing the exact image used to produce a published result, then archiving both the image and its signature, lets future readers verify they are running the same software.

### Self-check §8

1. You build an image and push it as `myimage:1.0`. A week later, with code changes, you push again as `myimage:1.0`. What happens to clients who pulled the first version? Trace the implications.
2. Why does Docker Hub's free-tier rate limiting (100 pulls per 6 h per IP) become a serious problem for CI pipelines? Suggest two mitigations.
3. The CD pipeline pulls from the registry. Under what circumstances is the registry's availability part of your *runtime* dependencies, not just deployment dependencies? (Hint: think about Kubernetes node restarts.)

---

## §9. Container security

We have alluded several times to the security trade-off at the heart of the container model: containers share the host kernel. This is what gives them their performance advantage over VMs (no nested virtualisation, no second OS to load) and what gives them their security disadvantage. A vulnerability in the kernel that allows a process to escape its namespace boundaries — a *container escape* — gives the attacker the host, and through the host every other container running on that host. By contrast, a comparable exploit in a VM gives the attacker control of one VM only; the hypervisor and hardware enforce a stronger architectural boundary.

This is not a hypothetical concern. Container escapes have been demonstrated through kernel bugs, through misconfigurations of capabilities and seccomp profiles, through volumes mounted with too much trust, through container-runtime bugs themselves. The list of CVEs is long. The right way to think about container security is therefore not "is it secure?" — the answer is "it depends" — but rather "what is the threat model, and what defences are in place against each threat?"

### §9.1 Attack vectors

The principal threats and their mitigations form a small but comprehensive matrix:

| Threat | Description | Mitigation |
|---|---|---|
| Kernel exploits | Privilege escalation via kernel vulnerability → container escape | Patch host kernel; consider gVisor or Kata for untrusted workloads |
| Vulnerable base images | Known CVEs in OS packages or libraries baked into the image | Scan with Trivy/Snyk; use minimal bases; rebuild regularly |
| Running as root | Container processes run as UID 0 by default → escape gives host root | `USER` instruction in Dockerfile; rootless Docker; Podman |
| Secrets in image layers | Passwords/keys committed via Dockerfile → visible in `docker history` | Multi-stage builds; build secrets (`--secret`); external vaults |
| Unrestricted resources | Container can exhaust host CPU/memory → denial-of-service to host | `--memory`, `--cpus`, `--pids-limit`; orchestrator quotas |
| Unrestricted syscalls | Container can call dangerous syscalls (e.g., `ptrace`, `keyctl`) | seccomp profiles; AppArmor/SELinux |
| Writable filesystem | Malware can modify application binaries at runtime | `--read-only` flag with tmpfs for required writable paths |

These are not independent risks. A vulnerable base image lets an attacker land arbitrary code in the container; running as root inside the container lets that code do more; an unrestricted seccomp profile gives that code access to dangerous syscalls; a kernel exploit invoked through such a syscall achieves the escape. The defence-in-depth principle says: weaken the chain at every link.

### §9.2 Best practices

The seven habits worth internalising. None of them is exotic; all have direct consequences for image quality and runtime safety.

**Use minimal base images.** Fewer packages mean fewer CVEs. The space ranges from `scratch` (zero bytes, only suitable for fully static binaries) to `ubuntu:22.04` (full apt ecosystem). The right choice depends on the workload.

| Base image | Approximate size | Packages included | Suitable for |
|---|---|---|---|
| `scratch` | 0 MB | none | statically compiled Go/Rust binaries |
| `alpine:3.19` | ~7 MB | musl libc, busybox | general-purpose lightweight containers |
| `python:3.11-slim` | ~130 MB | Debian minimal + Python | Python applications |
| `ubuntu:22.04` | ~77 MB | full apt ecosystem | when specific Ubuntu packages are required |
| `gcr.io/distroless/python3` | ~50 MB | Python runtime only, no shell | production Python deployments |

The "distroless" family from Google deserves special mention: images that contain *only* the language runtime and the application — no shell, no package manager, no `/bin/ls`. An attacker who lands in such an image has no shell to run, no `curl` to fetch additional payloads, no package manager to install tools. The defence is partial (a determined attacker can still execute Python code or compiled binaries), but the attack surface is dramatically reduced.

**Run as non-root.** Docker's default of running container processes as UID 0 is a historical accident, not a design choice. Always create an unprivileged user and switch to it:

```dockerfile
FROM python:3.11-slim
RUN groupadd -r appgroup && \
    useradd -r -g appgroup -u 1000 -d /app appuser
WORKDIR /app
COPY --chown=appuser:appgroup . .
RUN pip install --no-cache-dir -r requirements.txt
USER appuser
CMD ["python", "app.py"]
```

The `USER appuser` switches the default user for the running container. A compromised application running as `appuser` cannot, even with a container escape, gain root on the host — the kernel sees a non-root UID throughout.

**Scan images for vulnerabilities.** Tools like Trivy, Snyk, and Grype walk the image's installed packages and check them against vulnerability databases (NVD, GHSA). A typical CI integration:

```bash
# Scan a local image; print all findings
trivy image myapp:v1.0

# Fail CI if any critical vulnerability is found
trivy image --exit-code 1 --severity CRITICAL myapp:v1.0
```

The discipline is to scan on every build and to rebuild base images regularly — a CVE discovered tomorrow may apply to images built yesterday.

**Never bake secrets into images.** Every layer of an image is visible via `docker history`. A `RUN echo "API_KEY=..." > /etc/...` is permanently embedded; deleting the file in a later layer does not remove it from the earlier layer. Three patterns avoid this:

```bash
# Bad: secret visible in docker history forever
RUN echo "API_KEY=sk-12345" >> /app/.env

# Good: inject at runtime, never stored in the image
docker run -e API_KEY=sk-12345 myapp

# Best: use BuildKit build secrets (never written to a layer)
RUN --mount=type=secret,id=api_key cat /run/secrets/api_key
```

The third pattern, BuildKit's `--mount=type=secret`, makes the secret available during one specific RUN step but does not persist it in the image.

**Set resource limits.** Prevents a single container from exhausting the host:

```bash
docker run --memory=4g --cpus=2 --pids-limit=100 myapp
```

The `--pids-limit` flag is often overlooked but is critical against fork bombs.

**Use a read-only root filesystem when possible.** Prevents runtime tampering with application binaries:

```bash
docker run --read-only --tmpfs /tmp --tmpfs /var/run myapp
```

The `--read-only` flag mounts the entire root filesystem read-only; the `--tmpfs` mounts provide writable areas only where the application genuinely needs them. Many applications can run this way with minor configuration; doing so eliminates a class of attacks that depend on writing executables into the container.

**Drop unnecessary capabilities.** Docker grants containers a default set of Linux capabilities (`CAP_NET_BIND_SERVICE`, `CAP_SETUID`, etc.). Most workloads need only a small subset. Drop them all and add back only what is needed:

```bash
docker run --cap-drop=ALL --cap-add=NET_BIND_SERVICE myapp
```

This is the principle of least privilege applied at the kernel-capability layer.

### §9.3 Rootless containers

A complementary approach to the seven habits above: instead of running containers as root with hardening, run the entire container stack without root in the first place.

Traditional Docker requires the daemon (`dockerd`) to run as root. A container escape can therefore gain root privileges on the host. **Rootless Docker** is a recent variant in which the entire daemon runs under a regular user account; an escape in this configuration yields only that user's privileges, not root.

**Podman** went further — it has no daemon at all. Each `podman` invocation is a standalone process running as the invoking user. The CLI is intentionally Docker-compatible (`alias docker=podman` works for most use cases), making Podman a near drop-in replacement.

The implications for HPC are decisive. We saw in §2.4 that the user namespace makes rootless container execution possible at the kernel level. Apptainer's daemonless, rootless architecture was designed specifically for shared multi-user clusters, where running a system-wide root daemon is not an option. We turn to this in detail in §11.

### Self-check §9

1. A penetration test reveals that a containerised web application running as root has been compromised. The attacker gained shell access *inside* the container. What additional steps are required to escalate to host root, and which mitigations from §9.2 would have prevented each step?
2. Why does `docker history` expose secrets that have been deleted in subsequent layers? Trace the union-filesystem mechanism that produces this behaviour.
3. A team uses `--read-only` on all production containers. Their application crashes with permission errors. What is the most likely cause and how do they fix it without sacrificing the read-only property?
4. Compare the security posture of (a) Docker as root, (b) rootless Docker, (c) Podman, and (d) Apptainer. Rank from weakest to strongest and justify the ordering.

---

## §10. Container runtimes and the OCI standard

We saw in §3 that "Docker" is not one program but a stack of components — Docker CLI, dockerd, containerd, runc, the kernel. This stack made sense for the project that produced it but raised an obvious concern: a single vendor, Docker Inc., effectively controlled the container ecosystem. Image formats, runtime behaviour, registry protocols — all defined by Docker, all subject to change at Docker's commercial discretion. The industry, having invested heavily in containers, was unwilling to leave that control concentrated in one company.

The response, in 2015, was the **Open Container Initiative (OCI)**, founded by Docker, CoreOS, Google, IBM, Red Hat, and others to define vendor-neutral standards. Within a few years the OCI had published three specifications that, together, govern the container ecosystem.

### §10.1 The three OCI specifications

| Specification | Defines | Ensures |
|---|---|---|
| **Image Specification** | How container images are packaged: layers, manifest, configuration, image-index format | Images built with Docker work with Podman, containerd, CRI-O, Apptainer, ... |
| **Runtime Specification** | How a container is created and executed from an image: the `config.json` describing namespaces, cgroups, mounts, command | runc, crun, gVisor, Kata are interchangeable low-level runtimes |
| **Distribution Specification** | How images are pushed to and pulled from registries: HTTP API, blob and manifest endpoints | Docker Hub, ECR, GHCR, Harbor all speak the same protocol |

The Image Spec is the most consequential for users. It says: regardless of which tool you used to *build* an image, the resulting artefact is a tarball with a specific manifest format and specific blob layout. Any tool implementing the spec can consume it. This is why a Dockerfile-built image runs unchanged under Podman, why Apptainer can pull from Docker Hub, why Kubernetes does not care which tool built the images it deploys.

The Runtime Spec defines what we walked through in §2.7: the JSON description of namespaces to create, cgroups to apply, mounts to set up, command to execute. A *runtime* is anything that, given such a JSON, produces a running container. This is what allows runc to be replaced with gVisor (which does the same thing but interposes a user-space kernel) or Kata (which does it inside a microVM) — they all consume the same input.

The Distribution Spec standardises the registry HTTP protocol. This is the layer that lets `docker pull docker.io/library/python` and `apptainer pull docker://library/python` retrieve the same bytes from the same server.

### §10.2 The runtime landscape

The set of runtimes that the user might encounter has grown beyond runc. The principal options:

```
┌──────────────────────────────────────────────────────┐
│     Orchestration layer                              │
│     Kubernetes · Docker Swarm · Nomad                │
├──────────────────────────────────────────────────────┤
│     High-level runtime (CRI-compliant)               │
│     containerd · CRI-O                               │
├──────────────────────────────────────────────────────┤
│     Low-level runtime (OCI-compliant)                │
│     runc · crun · gVisor (runsc) · Kata Containers   │
├──────────────────────────────────────────────────────┤
│     Linux Kernel                                     │
│     namespaces · cgroups · seccomp · OverlayFS       │
└──────────────────────────────────────────────────────┘
```

| Runtime | Type | Distinguishing feature | When to choose |
|---|---|---|---|
| `runc` | Low-level (OCI) | Reference implementation, written in Go | Default; trusted workloads |
| `crun` | Low-level (OCI) | Written in C, lower memory footprint, faster startup | Performance-sensitive; many short-lived containers |
| `gVisor` (`runsc`) | Sandboxed (OCI) | User-space kernel intercepting syscalls | Untrusted workloads, multi-tenant |
| Kata Containers | MicroVM (OCI) | Lightweight VM per container | Strong isolation with container-like UX |
| `containerd` | High-level (CRI) | Industry-standard; used by Docker and Kubernetes | Production Kubernetes |
| CRI-O | High-level (CRI) | Kubernetes-native, minimal | OpenShift; minimal-footprint Kubernetes |
| Podman | CLI + runtime | Daemonless, rootless, Docker-CLI-compatible | Development, rootless CI |

The first two distinctions worth understanding: low-level vs high-level, and OCI vs CRI.

A **low-level runtime** (runc, crun, gVisor, Kata) takes an OCI runtime configuration and creates the running container. It does the namespace and cgroup work we examined in §2.7. It does not pull images, manage caches, or handle networking — that is someone else's job.

A **high-level runtime** (containerd, CRI-O) does that someone-else's job: pulls images from registries, manages local image storage, handles container lifecycle (create, start, stop, delete), exposes the Container Runtime Interface (CRI) that Kubernetes speaks. Internally it delegates the actual container creation to a low-level runtime.

This decomposition is why you can mix and match: containerd + runc is the standard stack; containerd + Kata is the same management with stronger isolation; Podman + crun is daemonless with fast startup. The OCI specifications make these combinations possible.

### §10.3 "Docker is deprecated in Kubernetes" — what it actually means

A confusion worth dispelling. In Kubernetes 1.24 (mid-2022), support for the Docker daemon as a CRI runtime was removed. Many users assumed their Docker images would stop working. They did not. The change was about which *runtime daemon* Kubernetes talks to, not about the image format.

To unpack:

- Kubernetes removed support for the **Docker daemon (`dockerd`)** as a CRI runtime. Before 1.24, a shim called `dockershim` translated CRI calls into Docker daemon calls; this shim was removed.
- Kubernetes now communicates directly with **containerd** or **CRI-O**, both of which speak CRI natively.
- Docker **images** still work perfectly, because they comply with the OCI Image Spec, which is what containerd and CRI-O consume.
- You can still use Docker (the CLI + daemon) to **build** images. You just cannot use the Docker daemon to **run** them in Kubernetes.

The practical effect on application developers is essentially zero. Build with Docker as before; deploy to Kubernetes with no changes. Behind the scenes, what runs your container is containerd, not dockerd. This is exactly what the OCI standards were designed to make possible.

### Self-check §10

1. A vendor delivers a container image as a `.tar` file. Without Docker Hub or any other registry, can you (a) inspect its contents, (b) run it with Podman, (c) run it with Apptainer? What standard makes each of these possible?
2. Trace the path of a container creation in Kubernetes 1.24+: what calls what, in what order?
3. gVisor and Kata Containers both provide stronger isolation than runc. They achieve this in different ways. Identify each approach and the performance trade-off it implies.

---

## §11. Containers for scientific computing

We have arrived at the section that motivates much of the lecture for an HPC audience. Containers, as we have presented them, were designed for cloud microservices: stateless services running on disposable VMs in commercial data centres. HPC clusters have a different culture, different software stack, and different security posture, and it turns out that the architectural choices Docker made — many of them sensible for cloud — clash with HPC requirements at almost every point. This section explains the clashes and the runtimes that the HPC community developed to resolve them.

### §11.1 Why Docker does not work on HPC clusters

Five concrete points of friction:

| Docker assumption | HPC reality | Conflict |
|---|---|---|
| Root daemon required | Users have no root access on shared clusters | Security and policy violation |
| Container runs as root by default | UID 0 in container may map to root on host | Filesystem corruption risk on shared storage |
| Bridge networking with NAT | MPI and InfiniBand require direct host networking | Latency; incompatible with RDMA |
| No scheduler integration | SLURM/PBS manages all resources | Cannot allocate resources properly |
| Isolated filesystem by default | Shared parallel filesystems (Lustre, GPFS, BeeGFS) must be visible | Data inaccessible inside container |

The first item is the most fundamental. Docker's daemon (`dockerd`) runs as root and exposes a Unix socket that users in the `docker` group can talk to. Anyone who can talk to that socket can effectively become root — `docker run --privileged --pid=host -v /:/host ...` gives you the host. On a shared cluster, putting users in the `docker` group is equivalent to giving them root, which is not acceptable.

The second item compounds the first. Even without an explicit privilege escalation, a container that writes to a bind-mounted host directory does so as the container's UID, which by default is 0. Without user-namespace mapping, that means files appear on the host filesystem owned by root — a problem for shared storage where users expect to own the files they create.

The third item — bridge networking with NAT — is incompatible with InfiniBand RDMA, as we discussed in §5.4. MPI workloads need direct access to the host's network fabric, which the kernel-bypass nature of RDMA cannot route through a Docker bridge.

The fourth and fifth items are integration concerns. SLURM allocates resources at the node level: CPU cores, memory, GPUs, network ports. A container started by `docker run` knows nothing about the SLURM allocation; it can use whatever the host gives it, which may conflict with what SLURM expects. And shared filesystems — Lustre on Leonardo, GPFS on many clusters, BeeGFS on others — must be visible inside the container for the workload to access input data. Docker's default isolated filesystem requires explicit mounts; the HPC-natural pattern of "I see my home directory" is not the default.

### §11.2 The HPC container runtimes

Four runtimes have emerged to address these issues, each with slightly different trade-offs:

| Runtime | Developer | Distinguishing features |
|---|---|---|
| **Apptainer** (formerly Singularity) | Linux Foundation / community | No daemon; rootless; SIF format; native MPI; SLURM integration |
| **Sarus** | ETH Zurich / CSCS | OCI-compliant; native MPI hook; SSH hook for parallel jobs |
| **Shifter** | NERSC | Converts Docker images; SLURM integration; parallel pulls |
| **Enroot** | NVIDIA | Unprivileged; GPU-optimised; used with Pyxis SLURM plugin |

All four share the core HPC adaptations:
- No system-wide daemon. Each runtime invocation is a standalone user process.
- User-namespace-based UID mapping (the §2.4 mechanism), so files written by the container appear on the host owned by the invoking user.
- Host networking by default, for RDMA compatibility.
- Native MPI integration: the container's MPI implementation can use the host's InfiniBand stack, typically by bind-mounting the host's MPI libraries.
- Integration with the workload manager (SLURM hooks, environment-variable propagation).

**Apptainer** is the dominant choice in the broader HPC community — it is the runtime installed on Leonardo and on most other Tier-0 and Tier-1 systems in Europe and North America. Sarus is used at CSCS and a few other sites with similar workflow needs. Shifter is the original and is still used at NERSC. Enroot is the GPU-optimised choice at NVIDIA-led sites and increasingly in academic GPU clusters.

For this course, Apptainer is the runtime we will use; it is also the subject of the second half of Lecture 06, the cloud-HPC bridging lecture. The Docker tutorial accompanying this course closes with a chapter on converting Docker images to SIF and running them on Leonardo.

### §11.3 Apptainer in one page

Apptainer's command-line interface is intentionally simple — three or four commands cover most usage. The principles directly mirror Docker; only the underlying execution model is different.

```bash
# Build a SIF image from a Docker Hub image
apptainer build numpy.sif docker://python:3.11-slim

# Run a command inside the container
apptainer exec numpy.sif python -c "import numpy; print(numpy.__version__)"

# Bind-mount a host directory into the container
apptainer exec --bind /scratch:/scratch numpy.sif python analysis.py

# Multi-node MPI run via SLURM
srun --ntasks=64 --nodes=4 \
  apptainer exec --bind /scratch numpy.sif \
  mpirun -np 64 ./simulation --input /scratch/data/input.h5
```

Three things distinguish this from the Docker equivalent. First, an Apptainer image is a single file (`.sif`) — the **Singularity Image Format** — rather than a registry-managed set of layers. You can `scp` it, archive it, attach it to a paper as supplementary material; it is just a file. Second, Apptainer runs as the invoking user, not as root; UID 0 inside the container maps to the user's UID outside, automatically. Third, Apptainer's default networking is the host's; there is no bridge, no NAT, no port mapping. RDMA works out of the box.

Apptainer also supports its own native build format — a `.def` file similar to a Dockerfile but with HPC-specific bindings — but for most users the workflow is to build images with Docker (where the toolchain is mature) and convert them to SIF at the cluster boundary.

### §11.4 GPU containers

Scientific GPU workloads — ML training, molecular dynamics, computational chemistry — require GPU access from inside the container. This is more involved than CPU access because the GPU has its own driver, its own compute libraries (CUDA, ROCm), and its own version compatibility constraints.

The standard mechanism for Docker is the **NVIDIA Container Toolkit** (formerly known as nvidia-docker), which provides a runtime hook that maps NVIDIA device files (`/dev/nvidia*`) and driver libraries from the host into the container at run time:

```bash
# Docker with GPU access
docker run --gpus all nvidia/cuda:12.2.0-runtime-ubuntu22.04 nvidia-smi
```

The `--gpus all` flag activates the toolkit's hook. Inside the container, `nvidia-smi` works as if running on the host.

Apptainer has the same capability with a single flag:

```bash
# Apptainer with GPU access
apptainer exec --nv pytorch.sif python train.py
```

The `--nv` flag instructs Apptainer to bind-mount the host's NVIDIA driver libraries into the container at runtime. The container itself does not need to ship NVIDIA driver libraries — only the CUDA toolkit version it was built against.

> **A subtle compatibility rule.** The CUDA *toolkit* inside the container must be supported by the NVIDIA *driver* on the host. The constraint is: container CUDA toolkit version ≤ maximum CUDA version supported by the host driver. So a container built with CUDA 12.2 can run on a host with NVIDIA driver supporting CUDA 12.4, but not on a host whose driver is too old. On Leonardo, where the host drivers are kept current, this is rarely a problem; on a workstation that has not been updated for a year, it can be. Always check `nvidia-smi` on the host to see the maximum CUDA version it supports.

### §11.5 Reproducibility for science

Containers are powerful tools for computational reproducibility, but only if used with discipline. Several practices distinguish a *reproducible* container from a merely *containerised* one:

**Pin all package versions.** `pip install numpy==1.26.2`, not `pip install numpy`. The latter installs whatever is current today, which is not what was current six months ago and will not be what is current six months from now. The same rule applies to apt packages, conda environments, npm, cargo, etc.

**Pin the base image by digest.** `FROM python@sha256:abc123...`, not `FROM python:3.11`. Tags are mutable; digests are not. The latter form guarantees that the base image is byte-for-byte identical to the one used during development.

**Archive the image.** Push the exact image used to produce a published result to a long-term registry — Zenodo, an institutional registry, a controlled GHCR repository. The image's digest, recorded in the paper, lets future readers retrieve exactly the same artefact.

**Include the Dockerfile in the paper's supplementary material.** This documents the complete build procedure, even if the registry hosting the image disappears.

**Record build metadata.** `docker inspect` and `apptainer inspect` capture build date, layer hashes, and command history. Keep this output alongside the paper.

These practices are not extra work for already-tedious science; they are habits that, once adopted, become invisible. A research group that adopts them will be in a position to defend any computational result it publishes — and to reuse its own work years later, when the original author has moved on and the original laptop has been recycled.

### Self-check §11

1. Why does Docker's UID-0-by-default model cause problems on a shared filesystem like Lustre, even when no privilege escalation occurs?
2. Apptainer mounts the host's MPI libraries into the container at run time rather than bundling them in the image. What is the rationale, and what failure mode does this introduce?
3. A research group builds an Apptainer image based on PyTorch + CUDA 12.4 on a workstation with NVIDIA driver 550. They try to run it on a six-month-old cluster node with NVIDIA driver 525. Predict the outcome and explain why.
4. Argue both sides of the question: should HPC clusters allow Docker (with appropriate hardening), or should they require Apptainer? Identify one strong argument for each position.

---

# Concluding remarks

We have now traversed the container model from the kernel mechanisms that make it possible to the HPC-specific runtimes that adapt it to scientific computing. Three threads run through the lecture, and they are the threads worth carrying forward.

The first is the structural one. **A container is not a special kernel object — it is a process placed into restricted views (namespaces) and resource budgets (cgroups).** Everything Docker, Podman, Apptainer, and the rest do at user level is some elaboration of this kernel-level core. Internalise this and you will find the user-space tooling demystified: features like image layers, volumes, and networks are conventions on top of the same six-step kernel sequence we walked through in §2.7.

The second thread is the trade-off. **Containers achieve their performance by sharing the host kernel; they pay for that performance by sharing the host kernel.** The shared-kernel attack surface is the central security trade-off, and it is why containers and VMs occupy different niches in production architectures. VMs provide tenant-level isolation; containers provide application-level density. Modern systems use both, in layers.

The third thread is the bridge to HPC. **Docker's defaults — root daemon, bridge networking, isolated filesystems — are antithetical to multi-user cluster operation.** Apptainer and its peers are not Docker variants; they are different runtimes with different design choices, addressing the same kernel mechanisms from a different starting point. The sensible attitude for an HPC practitioner is therefore: develop with Docker on a laptop, convert to Apptainer at the cluster boundary, accept that the two are tools for different stages of the same workflow.

Lecture 06 will pick up this last thread in detail, building the full pipeline from local Docker development to Apptainer deployment on Leonardo. Lectures 05 and 06 in between will introduce Kubernetes, the orchestrator that uses the same containerd runtime we examined here but at the scale of clusters of machines. The vocabulary you have acquired in this lecture — namespaces, cgroups, layers, volumes, registries, OCI — will appear unchanged.


---

# Appendix A — Glossary

**ADD** — Dockerfile instruction that copies files into the image and additionally auto-extracts tar archives or fetches URLs. `COPY` is preferred for ordinary file copying.

**Apptainer** — Daemonless, rootless container runtime designed for HPC. Formerly known as Singularity (the open-source fork is now under the Linux Foundation). Uses single-file SIF images.

**Bind mount** — A storage mechanism that maps a specific host directory into the container at a specific path, with the host's user managing both ends.

**Build context** — The set of files sent to the Docker daemon at the start of a build, used as the source for `COPY` and `ADD` instructions. Constrained by `.dockerignore`.

**Capability (Linux)** — A subset of root privileges that can be granted or denied independently. Container security best practice is to drop all capabilities and add back only those required.

**`cgcreate`, `cgexec`, `cgset`** — User-space tools from the `libcgroup` package for creating, executing in, and configuring cgroups. A higher-level alternative to writing directly to `/sys/fs/cgroup/`.

**cgroup (control group)** — Linux kernel mechanism for limiting, accounting for, and isolating resource usage (CPU, memory, I/O) of a group of processes.

**cgroups v2** — The unified-hierarchy redesign of cgroups, default on modern Linux distributions. All controllers operate within a single tree, in contrast to the per-controller trees of v1.

**`clone()`** — Linux system call that creates a new process. With `CLONE_NEW*` flags, it creates the new process in fresh namespaces. The kernel-level interface used by container runtimes.

**CMD** — Dockerfile instruction setting the default command for the container. Overridable at run time.

**Container** — A process (or process tree) isolated via Linux namespaces and resource-limited via cgroups. Not a special kernel object; a convention.

**Container escape** — A vulnerability or misconfiguration that allows a process to break out of its container's isolation and gain access to the host or other containers.

**containerd** — Industry-standard high-level container runtime. Manages images, container lifecycle, snapshots; delegates actual container creation to a low-level runtime such as runc.

**Copy-on-Write (CoW)** — Filesystem strategy in which a file is copied to the writable layer only when first modified. Used by OverlayFS to combine read-only image layers with a writable container layer.

**cosign** — Tool from the Sigstore project for cryptographically signing and verifying container images.

**CRI (Container Runtime Interface)** — gRPC API used by Kubernetes to communicate with container runtimes. containerd and CRI-O implement CRI; Docker daemon does not (which is why it was removed in Kubernetes 1.24).

**Distroless image** — A minimal base image containing only the application's runtime (e.g., Python interpreter), no shell, no package manager, no general-purpose utilities.

**Dockerfile** — Text file containing sequential instructions used by `docker build` to construct an image.

**Docker Compose** — Tool for defining and running multi-container applications using a YAML configuration file.

**Docker daemon (`dockerd`)** — The long-running process at the core of traditional Docker installations. Runs as root; manages images, networks, builds. Replaced in Kubernetes by direct CRI communication with containerd.

**Docker Hub** — The default Docker registry (`docker.io`). Free tier is rate-limited to 100 image pulls per 6 hours per IP.

**Embedded DNS** — The DNS server (`127.0.0.11`) that Docker runs inside each container attached to a user-defined network, resolving container names to IPs.

**ENTRYPOINT** — Dockerfile instruction setting a fixed command prefix; arguments to `docker run` are appended.

**`/etc/resolv.conf`** — Linux file specifying DNS nameservers; in containers attached to user-defined Docker networks, points to Docker's embedded DNS at 127.0.0.11.

**Health check** — A periodically executed command that determines whether a container is ready to serve requests. Used by Compose's `depends_on: condition: service_healthy` and by Kubernetes liveness/readiness probes.

**Image** — A read-only layered filesystem template from which containers are instantiated. Identified by registry-qualified name and tag, or by content digest.

**Layer** — A single filesystem delta produced by one Dockerfile instruction. Layers are cached, deduplicated, and combined via union filesystem.

**Linux capabilities** — see *Capability*.

**`lsns`** — Command to list namespaces currently in use on a Linux system, with the number of processes in each.

**Multi-stage build** — A Dockerfile pattern using multiple `FROM` instructions, where build-time stages produce artefacts that are selectively copied into a minimal runtime stage.

**Namespace** — Linux kernel mechanism that gives a process an isolated view of some kernel resource (PIDs, network, mount points, etc.). Seven types currently exist: PID, NET, MNT, UTS, IPC, USER, CGROUP.

**OCI (Open Container Initiative)** — Industry consortium defining vendor-neutral standards for container images, runtimes, and registry distribution.

**OOM killer** — Linux kernel component that terminates processes when memory limits (system-wide or cgroup) are exceeded. A container that hits its `memory.max` is OOM-killed; this surfaces as exit code 137.

**OverlayFS** — Linux union filesystem used by Docker (and most other container runtimes) to combine read-only image layers with a writable container layer.

**Podman** — Daemonless, rootless container engine with a Docker-compatible CLI. Drop-in replacement for `docker` in development and CI workflows.

**Port publishing** — The mechanism (`-p host:container`) by which a port inside a container's network namespace is made reachable from the host.

**`/proc/<pid>/ns/`** — Filesystem directory containing one symbolic link per namespace type, identifying the namespace instance to which the process belongs.

**Registry** — Server that stores and distributes container images. Examples: Docker Hub, GHCR, Amazon ECR, Harbor.

**runc** — The reference low-level OCI-compliant container runtime. Creates namespaces and cgroups, then executes the container's process.

**Seccomp** — Linux kernel feature for filtering allowed system calls. Container runtimes apply default seccomp profiles to restrict which syscalls a container may invoke.

**SIF (Singularity Image Format)** — The single-file image format used by Apptainer. Contains the entire image — layers, metadata, optional cryptographic signature — in one self-contained file.

**`/sys/fs/cgroup/`** — Virtual filesystem exposing the cgroups v2 hierarchy. Cgroups are created by `mkdir`, configured by writing to control files, populated by writing PIDs to `cgroup.procs`.

**`systemd-cgtop`** — Live, sortable view of cgroups by CPU, memory, I/O usage. The cgroup analogue of `top`.

**`systemd-run`** — Tool that runs a command inside a transient systemd unit (typically a scope), with optional resource limits set via `--property`. Allows rootless cgroup configuration where controllers are delegated to user sessions.

**Tag** — Mutable label attached to an image. `:latest` is the default but should not be used in production.

**tmpfs mount** — RAM-backed mount inside the container's mount namespace. Fast, never touches disk, disappears with the container.

**`unshare`** — Command (and underlying system call) that disassociates parts of the calling process's execution context — including running a program in newly created namespaces.

**User namespace** — A Linux namespace type that maps user IDs between an inner and outer view. Allows an unprivileged user to appear as root inside the namespace, while remaining unprivileged on the host. The mechanism that makes rootless containers possible.

**veth pair** — Virtual Ethernet device pair; one end inside a container's network namespace, the other attached to a Docker bridge.

**Volume** — Docker-managed persistent storage independent of the container lifecycle. Survives `docker rm`; can be shared between containers.

---

# Appendix B — Further reading and references

The literature on containers spans systems papers, vendor documentation, and a growing body of HPC-specific guidance. The list below emphasises authoritative primary sources and is a starting point, not an exhaustive bibliography.

### Foundational papers

<a id="ref-merkel-2014"></a>
**Merkel, D. (2014).** *Docker: Lightweight Linux containers for consistent development and deployment.* Linux Journal, 2014(239). The original Docker introduction in a venue read by Linux developers.

<a id="ref-kurtzer-2017"></a>
**Kurtzer, G. M., Sochat, V., & Bauer, M. W. (2017).** *Singularity: Scientific containers for mobility of compute.* PLoS ONE, 12(5), e0177459. The foundational Apptainer/Singularity paper, framing the HPC container case.

<a id="ref-jacobsen-2015"></a>
**Jacobsen, D. M. & Canon, R. S. (2015).** *Contain this, unleashing Docker for HPC.* Proceedings of the Cray User Group, 2015. NERSC's analysis of why Docker does not work directly on HPC and the design of Shifter.

<a id="ref-baker-2016"></a>
**Baker, M. (2016).** *1,500 scientists lift the lid on reproducibility.* Nature, 533(7604), 452–454. The widely cited reproducibility survey.

### Books

**Rice, L. (2020).** *Container Security.* O'Reilly Media. The standard reference on container security, covering namespaces, capabilities, seccomp, image scanning, runtime protection, and supply-chain integrity.

**Poulton, N. (2023).** *Docker Deep Dive.* Independently published. Operationally focused; covers the full Docker stack including swarm and BuildKit.

**Hausenblas, M. & Schimanski, S. (2019).** *Programming Kubernetes.* O'Reilly Media. For when Lecture 05 picks up the Kubernetes side of the story.

### Specifications

<a id="ref-oci-image"></a>
**Open Container Initiative — Image Specification.** [github.com/opencontainers/image-spec](https://github.com/opencontainers/image-spec). The authoritative definition of the layered image format.

<a id="ref-oci-runtime"></a>
**Open Container Initiative — Runtime Specification.** [github.com/opencontainers/runtime-spec](https://github.com/opencontainers/runtime-spec). Defines the JSON configuration that any OCI-compliant runtime consumes.

<a id="ref-oci-distribution"></a>
**Open Container Initiative — Distribution Specification.** [github.com/opencontainers/distribution-spec](https://github.com/opencontainers/distribution-spec). The HTTP API that all OCI-compliant registries implement.

### Linux kernel documentation

**`man 7 namespaces`** — Comprehensive overview of all seven Linux namespace types. The first reading after these notes for anyone who wants kernel-level depth.

**`man 7 cgroups`** — Overview of cgroups v1 and v2 with controller-by-controller detail.

**`man 7 user_namespaces`** — Specific detail on the user namespace, UID/GID mapping rules, and capability semantics.

**`man 2 clone`** and **`man 2 unshare`** — The system calls that create namespaces.

**Linux Kernel Documentation — Control Groups v2.** [kernel.org/doc/html/latest/admin-guide/cgroup-v2.html](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html). The authoritative reference for cgroups v2 controller semantics.

### Tooling documentation

**Docker Documentation.** [docs.docker.com](https://docs.docker.com/). The official Docker reference. Particularly: the Dockerfile reference, the BuildKit documentation, and the Compose specification.

**Podman Documentation.** [docs.podman.io](https://docs.podman.io/). For the rootless, daemonless alternative.

**Apptainer User Guide.** [apptainer.org/docs/user/main/](https://apptainer.org/docs/user/main/). The authoritative Apptainer reference, with detailed coverage of the SIF format, MPI integration, and SLURM hooks.

**containerd Documentation.** [containerd.io/docs/](https://containerd.io/docs/). For the high-level runtime that lies underneath both Docker and Kubernetes.

### HPC container guidance

**NERSC Containers Documentation.** [docs.nersc.gov/development/containers/](https://docs.nersc.gov/development/containers/). One of the more comprehensive HPC-site-specific container guides, covering Shifter and Podman-HPC.

**CINECA Singularity Documentation.** [docs.hpc.cineca.it/services/singularity.html](https://docs.hpc.cineca.it/services/singularity.html). Leonardo-specific notes; the reference for the second half of Lecture 06.

**Sarus Documentation.** [sarus.readthedocs.io](https://sarus.readthedocs.io/). The CSCS-developed runtime, used at Piz Daint and a small number of other sites.

**Enroot.** [github.com/NVIDIA/enroot](https://github.com/NVIDIA/enroot). NVIDIA's GPU-optimised unprivileged container runtime, often paired with the Pyxis SLURM plugin.

### Security

<a id="ref-cncf-security"></a>
**CNCF Cloud Native Security Whitepaper.** [github.com/cncf/tag-security](https://github.com/cncf/tag-security). Practitioner-oriented guide to container security across the lifecycle.

**Sigstore / cosign Documentation.** [docs.sigstore.dev](https://docs.sigstore.dev/). For image signing and supply-chain integrity.

**Trivy Documentation.** [aquasecurity.github.io/trivy](https://aquasecurity.github.io/trivy/). The de facto open-source vulnerability scanner.

---

*Lecture Notes Version: 3.0 · Course: ADSAI HPC e Cloud, University of Trieste · 2025/2026*

*End of Lecture 04 notes.*
