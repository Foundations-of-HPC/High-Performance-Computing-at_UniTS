# Lecture - OpenStack Fundamentals: Private Cloud Architecture and Core Services

**Course:** High Performance and Cloud Computing 2025/2026
**Programme:** Applied Data Science & Artificial Intelligence - HPC Track
**Institution:** University of Trieste
**Author:** Course instructor
**Duration:** 1 h 45 min (Part 1 ≈ 50 min, Part 2 ≈ 55 min)

---

## Learning Objectives

By the end of this lecture, students will be able to:

1. **Explain** why private cloud infrastructure remains essential in scientific computing environments (Bloom: Understand)
2. **Describe** the modular architecture of OpenStack and the role of each core service (Bloom: Understand)
3. **Map** OpenStack services to their AWS equivalents, identifying architectural similarities and differences (Bloom: Analyze)
4. **Configure** authentication and CLI access to OpenStack environments, including CINECA ADA Cloud (Bloom: Apply)
5. **Deploy** virtual machine instances, configure networking, manage security groups, and attach block storage volumes using the OpenStack CLI (Bloom: Apply)
6. **Differentiate** the responsibility model in managed public cloud (AWS) versus private cloud (OpenStack/ADA) deployments (Bloom: Evaluate)

---

## Table of Contents

1. [Introduction: Why Private Cloud?](#1-introduction-why-private-cloud)
2. [OpenStack: History and Market Position](#2-openstack-history-and-market-position)
3. [OpenStack Architecture: Design Principles](#3-openstack-architecture-design-principles)
4. [Core Services: Deep Dive](#4-core-services-deep-dive)
5. [Identity and Authentication with Keystone](#5-identity-and-authentication-with-keystone)
6. [CINECA ADA Cloud: Infrastructure Specifics](#6-cineca-ada-cloud-infrastructure-specifics)
7. [Authentication and CLI Setup](#7-authentication-and-cli-setup)
8. [Working with Compute Instances (Nova)](#8-working-with-compute-instances-nova)
9. [Networking with Neutron](#9-networking-with-neutron)
10. [Block Storage with Cinder](#10-block-storage-with-cinder)
11. [Security Groups and Access Control](#11-security-groups-and-access-control)
12. [Key Takeaways](#12-key-takeaways)
13. [References](#13-references)
14. [Glossary](#14-glossary)

---

## 1. Introduction: Why Private Cloud?

### 1.1 The Spectrum of Cloud Deployment

In Lecture 02, we introduced the four cloud deployment models: public, private, hybrid, and multi-cloud. Public cloud providers such as AWS, Azure, and GCP dominate media attention and industry usage. Yet a significant fraction of enterprise and scientific computing infrastructure relies on **private cloud** - and understanding why is essential for any practitioner working in or adjacent to research computing.

Private cloud is not simply "old-fashioned on-premises computing rebranded." It is the application of the same architectural principles that define public cloud - on-demand self-service, resource pooling, rapid elasticity, measured service, broad network access (NIST SP 800-145) - -to infrastructure that an organization owns or directly controls. The critical distinction lies in *who operates the platform* and *where the data resides*.

### 1.2 Drivers for Private Cloud Adoption

Several factors drive institutions toward private cloud rather than (or alongside) public cloud:

**Data Sovereignty and Legal Compliance**

Research institutions, particularly in Europe, operate under stringent data governance frameworks. The General Data Protection Regulation (GDPR) imposes explicit requirements on where personal data may be stored and processed. Medical genomics data, population surveys, clinical trial records, and certain categories of environmental monitoring data may be subject to national or institutional regulations that prohibit or complicate their transfer to public cloud regions located outside the European Economic Area. Even when technically compliant, the audit burden associated with public cloud storage of sensitive research data can be prohibitive.

Beyond GDPR, export control regulations (such as the U.S. Export Administration Regulations, EAR) affect institutions collaborating internationally on dual-use research. Air-gapped or on-premises deployments eliminate a class of compliance risk entirely.

**Total Cost of Ownership at Scale**

The economics of public cloud, discussed in Lecture 02, favor smaller or highly variable workloads. For institutions with large, stable baseline compute demand, the CapEx model of private infrastructure often yields a lower total cost of ownership over a 5–10 year horizon. A research center running continuous molecular dynamics simulations at 80% cluster utilization year-round will generally find that owning the hardware is cheaper than paying on-demand or even reserved AWS pricing.

**Customization and Hardware Control**

HPC workloads frequently require hardware configurations unavailable or prohibitively expensive in public cloud: custom InfiniBand topologies, direct GPU interconnects, specialized storage architectures, or integration with existing parallel filesystems (Lustre, GPFS). Private cloud platforms allow administrators to expose these resources directly to users while still providing a self-service interface.

**Latency and Integration with HPC Systems**

When researchers need to transfer hundreds of terabytes between a parallel filesystem and a computational cluster, network latency and bandwidth between on-premises storage and a public cloud region become bottlenecks. Private cloud collocated with HPC infrastructure eliminates this constraint.

| Driver | Public Cloud Disadvantage | Private Cloud Advantage |
|--------|--------------------------|-------------------------|
| Data sovereignty | Data crosses jurisdictional boundaries | Data remains on-site or within jurisdiction |
| Cost at scale | Continuous high utilization is expensive | CapEx amortized over hardware lifetime |
| Hardware customization | Limited instance types | Full hardware configurability |
| HPC integration | WAN latency for data transfer | LAN-speed access to parallel filesystems |
| Regulatory audit | Shared responsibility complicates audits | Full control over audit trail |
| Network topology | Fixed AWS network topologies | Custom InfiniBand, RDMA, SR-IOV |

**When NOT to choose private cloud:** If your workload is bursty, unpredictable, or intermittent; if you lack the operations team to manage platform-level software; if geographic distribution is required; or if you need access to specialized managed services (ML inference endpoints, serverless functions, managed Kubernetes) that would be expensive to self-host. The hybrid model - private for stable baseline, public for burst - is often optimal.

---

## 2. OpenStack: History and Market Position

### 2.1 Origins and Governance

**OpenStack** was initiated in 2010 as a joint project between **NASA** (contributing their Nebula cloud platform) and **Rackspace** (contributing their Cloud Files object storage). The stated goal was to create a ubiquitous open-source cloud operating system. The project was notable from inception for its modular design philosophy: rather than a monolithic platform, OpenStack was architected as a collection of cooperating services communicating via REST APIs and a shared message queue.

In 2012, governance transferred to the newly formed **OpenStack Foundation**, subsequently renamed the **Open Infrastructure Foundation (OIF)** in 2020 to reflect expansion beyond OpenStack itself. The project follows a six-month release cycle with alphabetically named releases; as of 2024, the current stable release is **Dalmatian** (2024.2), following the Wallaby (2021.1) release used at CINECA.

### 2.2 Adoption in Scientific Infrastructure

OpenStack has achieved particularly strong adoption in scientific computing environments:

- **CERN** operates one of the world's largest OpenStack deployments, with tens of thousands of cores supporting the LHC computing grid
- **CINECA** (the Italian national HPC center) operates ADA Cloud on OpenStack Wallaby
- **National research networks** across Europe - including those operated by GÉANT member organizations - use OpenStack for research cloud services
- **EOSC (European Open Science Cloud)** federated cloud infrastructure includes OpenStack deployments at multiple sites

> *OpenStack is an open-source cloud operating system that controls large pools of compute, storage, and networking resources throughout a datacenter, managed through a dashboard or via the OpenStack APIs.* - Open Infrastructure Foundation

### 2.3 OpenStack vs. AWS: A Fundamental Comparison

Before diving into architecture, it is essential to frame the relationship between OpenStack and AWS correctly. They are not competitors in the same market segment - they are complementary technologies serving different roles in the cloud ecosystem.

| Aspect | AWS | OpenStack |
|--------|-----|-----------|
| **Type** | Managed public cloud service | Open-source cloud platform software |
| **Operator** | Amazon | Your institution (or service provider) |
| **Cost model** | Pay-per-use (OpEx) | CapEx (hardware) + OpEx (operations) |
| **Customization** | Limited to available services | Full control over all layers |
| **Data location** | AWS regions (external) | Your datacenter |
| **Operations burden** | Managed by AWS | Managed by your team |
| **Feature velocity** | Very high (managed) | Slower (community-driven) |
| **Multi-tenancy** | Platform-enforced | Self-managed via projects |
| **Compliance** | Shared responsibility model | You control the full stack |
| **HPC integration** | Limited (specialized instances) | Full flexibility |

The crucial insight: AWS and OpenStack solve the same *user-facing problem* (on-demand cloud infrastructure) but from opposite positions in the operational spectrum. AWS maximizes convenience at the cost of control; OpenStack maximizes control at the cost of operational complexity.

---

## 3. OpenStack Architecture: Design Principles

### 3.1 Modularity

The defining architectural characteristic of OpenStack is **radical modularity**. Each functional domain (compute, networking, block storage, object storage, image management, identity)  is implemented as an independent service with a well-defined REST API. Services communicate with each other through two channels:

1. **REST API calls**: synchronous request/response for queries and configuration
2. **Message queue (AMQP)**: asynchronous communication for operations that may take time (VM provisioning, volume attachment)

This architecture has both advantages and costs. The advantage is that operators can deploy only the components they need, substitute alternative implementations of any service, and scale individual components independently. The cost is operational complexity: a full OpenStack deployment involves running, monitoring, and maintaining a dozen or more separate services, each with its own configuration, database schema, and upgrade path.

### 3.2 API-Driven Design

Every OpenStack operation  (creating a VM, allocating a floating IP, uploading an image) is expressed as a REST API call. The Horizon dashboard and the `openstack` CLI are both clients of these same APIs. This has a profound pedagogical implication: anything you can do in the dashboard, you can automate with a script. There is no privileged GUI-only functionality.

### 3.3 Full Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         OPENSTACK ARCHITECTURE                               │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐    │
│  │                           HORIZON                                    │    │
│  │                     Web Dashboard (optional)                         │    │
│  └──────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│  ┌──────────────────────────────────────────────────────────────────────┐    │
│  │                         REST APIs                                    │    │
│  │   (Each service exposes its own versioned endpoint)                  │    │
│  └──────────────────────────────────────────────────────────────────────┘    │
│      │          │          │         │         │         │          │        │
│  ┌───┴──┐  ┌────┴───┐  ┌───┴──┐  ┌───┴──┐  ┌───┴──┐  ┌───┴──┐  ┌────┴──┐     │
│  │NOVA  │  │NEUTRON │  │CINDER│  │GLANCE│  │SWIFT │  │HEAT  │  │MANILA │     │
│  │Comp. │  │Network │  │Block │  │Image │  │Object│  │Orch. │  │Shared │     │
│  └───┬──┘  └────┬───┘  └───┬──┘  └───┬──┘  └───┬──┘  └───┬──┘  └────┬──┘     │
│      │          │          │         │         │         │          │        │
│  ┌──────────────────────────────────────────────────────────────────────┐    │
│  │                          KEYSTONE                                    │    │
│  │              Identity, Authentication, Service Catalog               │    │
│  └──────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│  ┌──────────────────────────────────────────────────────────────────────┐    │
│  │                   Message Queue (RabbitMQ / Kafka)                   │    │
│  │                   Database (MySQL / MariaDB / PostgreSQL)            │    │
│  └──────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│  ┌──────────────────────────────────────────────────────────────────────┐    │
│  │              Physical Infrastructure                                 │    │
│  │   Compute Nodes (KVM)  │  Storage Backends  │  Network Fabric        │    │
│  └──────────────────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Core Services: Deep Dive

### 4.1 Service-to-AWS Mapping

One of the most effective ways to internalize OpenStack's architecture, given your familiarity with AWS from Lecture 07, is through direct service mapping. The correspondence is not perfect  (OpenStack services are lower-level and more configurable) but the functional equivalents are clear.

| OpenStack Service | AWS Equivalent | Primary Function |
|-------------------|---------------|------------------|
| **Keystone** | IAM + STS | Identity, authentication, authorization, service catalog |
| **Nova** | EC2 | Compute: virtual machine lifecycle management |
| **Neutron** | VPC + Security Groups | Software-defined networking, virtual networks, routers |
| **Cinder** | EBS | Block storage: persistent volumes for VMs |
| **Glance** | AMI Registry (EC2) | VM image registry and metadata service |
| **Swift** | S3 | Object storage: RESTful blob storage |
| **Horizon** | AWS Management Console | Web-based dashboard |
| **Heat** | CloudFormation | Infrastructure-as-Code orchestration |
| **Ceilometer/Gnocchi** | CloudWatch | Telemetry, metrics, billing data |
| **Barbican** | AWS Secrets Manager | Key management and secrets |
| **Designate** | Route 53 | DNS-as-a-service |
| **Manila** | EFS | Shared filesystem (NFS/CIFS) |
| **Octavia** | ELB/ALB | Load balancer as a service |
| **Magnum** | EKS | Container orchestration (Kubernetes) |

### 4.2 Nova (Compute)

**Nova** is the OpenStack compute service: the component responsible for spawning, scheduling, and managing virtual machines across a fleet of compute nodes. Nova does not implement its own hypervisor; instead, it provides an abstraction layer (the *virt driver*) over multiple hypervisor backends. In practice, the overwhelming majority of OpenStack deployments use **KVM** (via libvirt) as the hypervisor, with Nova handling scheduling and lifecycle management.

Nova's internal architecture is itself distributed. The `nova-api` process accepts API requests. The `nova-scheduler` selects an appropriate compute host for each new VM based on resource availability, host aggregates, and scheduling filters. The `nova-compute` daemon runs on each physical compute node and communicates with the local libvirt/KVM instance. These components communicate via the AMQP message queue (typically RabbitMQ).

**Key Nova concepts:**

- **Flavor**: A predefined hardware profile (vCPUs, RAM, ephemeral disk), equivalent to an EC2 instance type
- **Keypair**: SSH public key registered with Nova and injected into new VMs at boot
- **Server Group**: Policy for affinity/anti-affinity scheduling of related VMs
- **Host Aggregate**: Grouping of compute nodes by hardware capability (e.g., GPU nodes)
- **Availability Zone**: Logical partitioning for fault isolation

### 4.3 Neutron (Networking)

**Neutron** provides software-defined networking for OpenStack. Unlike the relatively opaque VPC model in AWS, Neutron exposes the full networking stack (virtual switches, routers, subnets, DHCP agents, and overlay networks) directly to tenant users.

Neutron's architecture centers on a plugin system. The **ML2 (Modular Layer 2) plugin** is standard and supports multiple mechanism drivers (Linux Bridge, Open vSwitch, SR-IOV) simultaneously. Overlay networking using **VXLAN** (as introduced in Lecture 03) allows tenant networks to span physical hosts without requiring dedicated VLANs, the same technology underlying AWS VPCs.

**Key Neutron concepts:**

- **Network**: A virtual Layer 2 broadcast domain
- **Subnet**: A CIDR block associated with a network, providing IP address allocation via DHCP
- **Router**: A virtual Layer 3 device connecting tenant networks to each other or to the external network
- **Port**: A virtual network interface, associated with a specific subnet and MAC address
- **Floating IP**: A publicly routable IP address that can be dynamically associated with a VM port
- **Security Group**: Stateful firewall rules applied at the virtual port level

### 4.4 Cinder (Block Storage)

**Cinder** provides persistent block storage: volumes that can be attached to Nova instances, detached, and re-attached to different instances. This is conceptually identical to Amazon EBS. Cinder abstracts over multiple backend storage systems through a driver interface; backends include local LVM, Ceph RBD, NetApp, Pure Storage, and others.

**Key Cinder concepts:**

- **Volume**: A persistent block device, analogous to an EBS volume
- **Snapshot**: A point-in-time copy of a volume, usable for backup or volume cloning
- **Volume Type**: A named storage policy (e.g., SSD vs. HDD, replication factor), analogous to EBS volume types (gp3, io2)
- **Backup**: A full or incremental backup of a volume, typically stored in Swift

### 4.5 Glance (Image Service)

**Glance** provides a registry for virtual machine images and their metadata. When you launch a Nova instance, Nova queries Glance for the image and downloads it to the compute node before booting. Glance supports multiple image formats (qcow2, raw, vmdk, ami) and multiple storage backends.

---

## 5. Identity and Authentication with Keystone

### 5.1 The Role of Keystone

**Keystone** is the central authentication and authorization service for the entire OpenStack ecosystem. Every API request to any OpenStack service must carry a valid Keystone token. Keystone thus acts as a **trust anchor** - all services trust Keystone's token validation, enabling cross-service authorization without per-service credential management.

This is architecturally distinct from AWS IAM. In AWS, IAM is a managed service operated by Amazon. In OpenStack, Keystone is a component you deploy and operate yourself.

### 5.2 Core Keystone Concepts

**Domains** are the highest-level administrative boundary in Keystone v3. A domain contains users, groups, and projects. CINECA ADA operates as a single domain; within it, each research group or project has its own **project** (historically called a *tenant*).

**Projects** are the fundamental unit of resource isolation and quota enforcement. All resources (VMs, volumes, networks, floating IPs) belong to a project. Users must be assigned a **role** on a project to access its resources.

**Roles** define the level of access. The standard roles are:

| Role | Permissions |
|------|------------|
| `admin` | Full administrative access to all resources |
| `member` | Standard user access within a project |
| `reader` | Read-only access to project resources |

**Tokens** are time-limited credentials issued by Keystone upon successful authentication. All subsequent API calls carry the token in the `X-Auth-Token` HTTP header. Tokens expire (typically after 1 hour) and must be renewed.

**Service Catalog** is a critical Keystone function often overlooked: it provides each authenticated user with the API endpoints for all OpenStack services deployed in the cloud. When you run `openstack server list`, the CLI first authenticates with Keystone, retrieves the service catalog to find the Nova API endpoint, then calls Nova with the token.

### 5.3 Application Credentials

For automation and CLI access, **Application Credentials** are the recommended authentication method: they do not require storing a user password, can be scoped to specific roles, and can be revoked without changing the user's password. CINECA ADA provides Application Credentials through the Horizon dashboard.

---

## 6. CINECA ADA Cloud: Infrastructure Specifics

### 6.1 Hardware Configuration

The CINECA ADA Cloud is a production OpenStack deployment available to students. Understanding its hardware configuration is useful for calibrating your experiments and understanding the performance envelope.

| Parameter | Value |
|-----------|-------|
| **Platform** | OpenStack Wallaby (2021.1) |
| **Compute nodes** | 71 interactive nodes |
| **CPU per node** | Intel Xeon Platinum 8260 (CascadeLake), 48 cores |
| **RAM per node** | 768 GB DDR4 |
| **Local storage** | 2 TB SSD |
| **Dashboard** | https://adacloud.hpc.cineca.it |

The Intel 8260 (CascadeLake) processors support AVX-512 vector instructions, hardware prefetching optimized for streaming HPC workloads, and hardware-assisted virtualization (Intel VT-x with EPT), as discussed in Lecture 03.

### 6.2 Responsibility Model

A critical difference from AWS: at CINECA ADA, *you* are responsible for managing your virtual machines at the OS level and above. CINECA manages the platform; you manage everything inside your project.

```
┌─────────────────────────────────────────────────────────────────────┐
│                  CINECA ADA RESPONSIBILITY MODEL                    │
│                                                                     │
│  CINECA manages:                                                    │
│  ├── Physical hardware (servers, network, storage array)            │
│  ├── OpenStack platform (Wallaby installation and updates)          │
│  ├── Hypervisor layer (KVM/libvirt on compute nodes)                │
│  ├── Core network infrastructure                                    │
│  └── Platform security, physical access controls                    │
│                                                                     │
│  ─────────────── SHARED RESPONSIBILITY BOUNDARY ──────────────────  │
│                                                                     │
│  YOU manage (within your project):                                  │
│  ├── Virtual machine operating systems (patches, updates)           │
│  ├── Network topology (private networks, routers, security groups)  │
│  ├── Applications installed on VMs                                  │
│  ├── Data stored on volumes and object storage                      │
│  ├── Access control (who has SSH keys, project membership)          │
│  └── Backups of your VMs and volumes                                │
└─────────────────────────────────────────────────────────────────────┘
```

This contrasts with AWS, where Amazon manages the hypervisor and underlying hardware, and you manage from the OS level up. In both cases, the *application* and *data* are always your responsibility.

---

## 7. Authentication and CLI Setup

### 7.1 Installing the OpenStack CLI

The OpenStack unified CLI (`python-openstackclient`) is the primary interface for automation and scripting. It is a Python package that provides the `openstack` command, which dispatches subcommands to the appropriate API endpoints.

```bash
# Create an isolated Python virtual environment (recommended)
python3 -m venv ~/openstack-cli
source ~/openstack-cli/bin/activate

# Install the unified client
# Pin to Wallaby-compatible version for CINECA ADA
pip install python-openstackclient==5.8.0

# Optionally install additional clients for services not covered by the unified CLI
pip install python-cinderclient python-neutronclient

# Verify installation
openstack --version
# Expected: openstack 5.8.0
```

### 7.2 Configuring Authentication

**Step 1**: Log in to the Horizon dashboard at https://adacloud.hpc.cineca.it

**Step 2**: Navigate to **Identity → Application Credentials → Create Application Credential**

**Step 3**: Download the generated `openrc` file. This file contains the necessary environment variables to authenticate the CLI.

**Step 4**: Source the file in your shell:

```bash
# Source the Application Credentials openrc file
source app-cred-myproject-openrc.sh

# For CINECA ADA, you may need to set the CA certificate bundle
# (Required if the ADA cloud uses a certificate signed by CINECA's internal CA)
export OS_CACERT=/path/to/cineca-chain.pem

# Verify authentication - this should return your project list
openstack project list

# Verify token issuance
openstack token issue
```

The `openrc` file sets the following environment variables that the `openstack` CLI reads automatically:

| Variable | Purpose |
|----------|---------|
| `OS_AUTH_URL` | Keystone endpoint URL |
| `OS_APPLICATION_CREDENTIAL_ID` | Application credential ID |
| `OS_APPLICATION_CREDENTIAL_SECRET` | Application credential secret |
| `OS_AUTH_TYPE` | Authentication type (`v3applicationcredential`) |
| `OS_REGION_NAME` | Target region |

### 7.3 CLI Structure

The OpenStack CLI follows a consistent `openstack <resource> <action>` pattern, e.g.:

```bash
openstack server list          # List compute instances
openstack network create ...   # Create a virtual network
openstack volume show <id>     # Show volume details
```

---

## 8. Working with Compute Instances (Nova)

### 8.1 Prerequisites: Images, Flavors, and Keypairs

Before launching an instance, you need three things: an image (the OS to boot), a flavor (the hardware profile), and a keypair (for SSH access).

```bash
# List available images
openstack image list
# Note: images are shared at the platform level; you can see public images

# List available flavors (instance types)
openstack flavor list
# Output shows: ID, Name, RAM (MB), Disk (GB), Ephemeral (GB), VCPUs

# List your registered keypairs
openstack keypair list

# Register a keypair from your SSH public key
openstack keypair create \
  --public-key ~/.ssh/id_rsa.pub \
  my-keypair
```

A representative flavor table (actual values depend on ADA cloud configuration):

| Flavor Name | vCPUs | RAM | Disk | Typical Use Case |
|-------------|-------|-----|------|-----------------|
| `m1.tiny` | 1 | 512 MB | 1 GB | Testing, minimal workloads |
| `m1.small` | 1 | 2 GB | 20 GB | Single-process applications |
| `m1.medium` | 2 | 4 GB | 40 GB | General-purpose |
| `m1.large` | 4 | 8 GB | 80 GB | Multi-threaded workloads |
| `m1.xlarge` | 8 | 16 GB | 160 GB | Memory-intensive tasks |
| `m1.xxlarge` | 16 | 32 GB | 160 GB | Large parallel workloads |

### 8.2 Instance Lifecycle

```bash
# Launch an instance
openstack server create \
  --image "Ubuntu 22.04" \
  --flavor m1.medium \
  --network my-private-network \
  --key-name my-keypair \
  --security-group default \
  --security-group web-sg \
  my-instance

# Monitor instance status
openstack server list
# Status transitions: BUILD → ACTIVE (or ERROR)

# Show detailed instance information
openstack server show my-instance

# View the instance's console output (useful for debugging boot issues)
openstack console log show my-instance

# Get a URL for a browser-based console (noVNC)
openstack console url show my-instance

# Lifecycle operations
openstack server stop my-instance       # Graceful shutdown
openstack server start my-instance      # Power on
openstack server reboot my-instance     # Reboot
openstack server resize my-instance --flavor m1.large   # Resize (if supported)
openstack server delete my-instance     # Terminate permanently
```

### 8.3 Instance States and Transitions

Understanding instance lifecycle states is important for automation:

```
     ┌──────────┐
     │  BUILD   │  ← Initial provisioning, image download, network setup
     └────┬─────┘
          │
     ┌────▼─────┐
     │  ACTIVE  │  ← Running normally; can be connected to
     └────┬─────┘
          │
    ┌─────┴─────┐
    ▼           ▼
┌───────┐  ┌──────────┐
│SHUTOFF│  │SUSPENDED │  ← Stopped (SHUTOFF) or hibernated (SUSPENDED)
└───────┘  └──────────┘
```

If an instance enters the `ERROR` state, `openstack console log show` is usually the first debugging step.

---

## 9. Networking with Neutron

### 9.1 Network Topology

OpenStack networking involves three types of networks: **external networks** (provider networks controlled by CINECA), **tenant/project networks** (private to your project), and the **router** that connects them.

```
┌────────────────────────────────────────────────────────────────────┐
│                     OPENSTACK NETWORK TOPOLOGY                     │
│                                                                    │
│   External Network (CINECA-managed, routable to campus/internet)   │
│   ┌─────────────────────────────────────────────────────────────┐  │
│   │                   Provider Network                          │  │
│   │         (CIDR: e.g., 10.0.0.0/8 or public range)            │  │
│   └────────────────────────┬────────────────────────────────────┘  │
│                            │                                       │
│                   ┌────────┴─────────┐                             │
│                   │  Virtual Router  │ ← Tenant-managed            │
│                   │  Floating IP Pool│                             │
│                   └────────┬─────────┘                             │
│                            │                                       │
│   Tenant Network (your project, private)                           │
│   ┌─────────────────────────────────────────────────────────────┐  │
│   │         Subnet: 192.168.100.0/24                            │  │
│   │   ┌──────────┐   ┌──────────┐   ┌──────────┐                │  │
│   │   │  VM-1    │   │  VM-2    │   │  VM-3    │                │  │
│   │   │.100.10   │   │.100.11   │   │.100.12   │                │  │
│   │   └──────────┘   └──────────┘   └──────────┘                │  │
│   └─────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
```

### 9.2 Creating a Network Stack

The following sequence creates a complete, isolated network environment for your project:

```bash
# 1. Create a private network
openstack network create my-research-network \
  --description "Private network for research workloads"

# 2. Create a subnet with DHCP
openstack subnet create \
  --network my-research-network \
  --subnet-range 192.168.100.0/24 \
  --gateway 192.168.100.1 \
  --dns-nameserver 8.8.8.8 \
  --dns-nameserver 8.8.4.4 \
  my-research-subnet

# 3. Create a router
openstack router create my-router

# 4. Connect the router to the external (provider) network
# Note: 'external-network' is the name provided by CINECA admins
openstack router set \
  --external-gateway external-network \
  my-router

# 5. Connect the router to your private subnet
openstack router add subnet my-router my-research-subnet

# 6. Allocate a floating IP from the external pool
openstack floating ip create external-network

# 7. Assign the floating IP to an instance
# First, identify the instance port
openstack floating ip set \
  --port $(openstack port list --server my-instance -f value -c id | head -1) \
  <floating-ip-address>

# 8. SSH into the instance (using the floating IP)
ssh -i ~/.ssh/id_rsa ubuntu@<floating-ip-address>
```

### 9.3 Neutron vs. AWS VPC

| Feature | AWS VPC | OpenStack Neutron |
|---------|---------|------------------|
| Network creation | `aws ec2 create-vpc` | `openstack network create` |
| Subnets | VPC subnets, explicit AZ assignment | Subnets attached to networks |
| Internet Gateway | Explicit IGW object | Router with external gateway |
| Elastic IP | EIP - persistent public IP | Floating IP - same concept |
| Route tables | Explicit per-subnet routing | Handled by virtual router |
| Security Groups | Stateful, per-ENI | Stateful, per-port |
| Transparency | Abstracted, limited control | Full control over virtual topology |

---

## 10. Block Storage with Cinder

### 10.1 Volume Operations

Cinder volumes persist independently of VM instances. This is critical for data durability: if a VM fails or is terminated, data on attached volumes is preserved (unlike the VM's ephemeral root disk).

```bash
# Create a 100 GB volume
openstack volume create \
  --size 100 \
  --description "Simulation input/output data" \
  research-data-vol

# Verify volume creation
openstack volume list
openstack volume show research-data-vol

# Attach volume to a running instance
openstack server add volume my-instance research-data-vol

# Inside the VM: identify the new block device
# Typically appears as /dev/vdb (second virtio disk)
lsblk

# Format and mount (first-time use only)
sudo mkfs.ext4 /dev/vdb
sudo mkdir -p /data
sudo mount /dev/vdb /data

# Add to /etc/fstab for persistent mounting across reboots
echo "/dev/vdb /data ext4 defaults 0 2" | sudo tee -a /etc/fstab

# Detach volume (volume must be unmounted first)
sudo umount /data
openstack server remove volume my-instance research-data-vol

# Create a snapshot (point-in-time copy, useful before risky operations)
openstack volume snapshot create \
  --volume research-data-vol \
  --description "Before analysis run 42" \
  snapshot-run-42

# Create a new volume from a snapshot (clone)
openstack volume create \
  --snapshot snapshot-run-42 \
  --size 100 \
  research-data-vol-run-42-copy

# Boot an instance from a volume (volume-backed instances)
openstack server create \
  --flavor m1.medium \
  --volume my-bootable-volume \
  --key-name my-keypair \
  volume-backed-instance
```

### 10.2 Cinder vs. EBS

| Feature | AWS EBS | OpenStack Cinder |
|---------|---------|-----------------|
| Volume types | gp3, io2, st1, sc1 | Depends on backend (LVM, Ceph, etc.) |
| Snapshots | Yes, stored in S3 | Yes, stored in Swift/object storage |
| Multi-attach | io1/io2 only | Backend-dependent |
| Encryption | KMS-managed | Barbican-managed |
| Max size | 64 TB | Backend-dependent |
| Performance tiers | Explicit IOPS specification | Volume type (backend-dependent) |

---

## 11. Security Groups and Access Control

### 11.1 Security Group Model

OpenStack security groups are **stateful, virtual firewalls** applied at the virtual network port level. By default, all *inbound* traffic is denied and all *outbound* traffic is permitted. To allow SSH access, for example, you must explicitly add an ingress rule for TCP port 22.

The security group model is **identical in concept to AWS security groups** - stateful rules, applied per-instance, with separate ingress and egress controls. The key difference is that in OpenStack, security groups are attached to *ports* (virtual NICs), while in AWS they attach to ENIs.

```bash
# Create a custom security group
openstack security group create \
  --description "Compute cluster nodes" \
  cluster-sg

# Allow SSH from anywhere (use a specific CIDR in production)
openstack security group rule create \
  --protocol tcp \
  --dst-port 22 \
  --remote-ip 0.0.0.0/0 \
  cluster-sg

# Allow ICMP (ping) for diagnostics
openstack security group rule create \
  --protocol icmp \
  --remote-ip 0.0.0.0/0 \
  cluster-sg

# Allow all traffic within the security group itself (intra-cluster communication)
openstack security group rule create \
  --protocol any \
  --remote-group cluster-sg \
  cluster-sg

# Allow MPI traffic on a port range (example: 10000-10999 for MPICH)
openstack security group rule create \
  --protocol tcp \
  --dst-port 10000:10999 \
  --remote-group cluster-sg \
  cluster-sg

# Apply a security group to an instance
openstack server add security group my-instance cluster-sg

# List all rules in a security group
openstack security group rule list cluster-sg
```

### 11.2 Principle of Least Privilege

The **principle of least privilege** dictates that instances should have access to exactly the network services they require and no more. A common mistake is to attach the `default` security group (which may have permissive intra-project rules) to all instances and add broad rules. Instead:

- Create **purpose-specific security groups** (e.g., `head-node-sg`, `worker-sg`, `storage-sg`)
- Restrict SSH access to **your institution's IP range** rather than `0.0.0.0/0`
- Use **security group references** (allowing traffic from another named security group) rather than broad IP ranges for intra-cluster communication

---

## 12. Key Takeaways

### Part 1 Summary (Architecture and Core Services)

1. **Private cloud rationale**: Data sovereignty, cost-at-scale, hardware control, and HPC integration drive private cloud adoption in research institutions. OpenStack is the dominant open-source platform for this use case.

2. **OpenStack architecture**: Modular, API-driven design. Each service (Nova, Neutron, Cinder, Glance, Keystone) has a well-defined REST API and communicates via a message queue. This enables independent scaling and substitution of components.

3. **Service mapping**: Every OpenStack service has a clear functional equivalent in AWS. The *concept* is the same; the *operational responsibility* is fundamentally different. You operate OpenStack; Amazon operates AWS.

4. **Keystone**: Central trust anchor. Domains → Projects → Users → Roles → Tokens. Application Credentials are the recommended authentication method for automation.

### Part 2 Summary (Hands-On Operations)

5. **CLI is primary**: The `openstack` CLI exposes the full API surface. Everything you can do in Horizon, you can automate. This is the foundation of Infrastructure as Code.

6. **Network topology**: Create private networks + subnets → create router → attach external gateway → allocate floating IPs. This mirrors the AWS VPC + Internet Gateway + Elastic IP pattern.

7. **Block storage**: Cinder volumes are persistent, independent of VM lifecycle. Attach/detach workflow mirrors EBS. Use snapshots before risky operations.

8. **Security groups**: Stateful virtual firewalls at the port level. Apply least-privilege principles: purpose-specific groups, restricted source IPs, intra-group references for cluster communication.

9. **CINECA ADA**: Real OpenStack Wallaby infrastructure with Intel CascadeLake compute nodes. You are responsible for everything inside your project - CINECA manages the platform.

---

## 13. References

- **OpenStack Documentation**: https://docs.openstack.org/wallaby/ (Wallaby release, matching CINECA ADA)
- **CINECA ADA Cloud Documentation**: https://wiki.u-gov.it/confluence/display/SCAIUS/ADA+Cloud
- **OpenStack CLI Reference**: https://docs.openstack.org/python-openstackclient/latest/cli/
- **Keystone Architecture**: https://docs.openstack.org/keystone/wallaby/getting-started/architecture.html
- **Nova (Compute) Concepts**: https://docs.openstack.org/nova/wallaby/user/
- **Neutron Concepts**: https://docs.openstack.org/neutron/wallaby/admin/intro-os-networking.html
- **Open Infrastructure Foundation**: https://openinfra.dev/
- **CERN OpenStack Deployment** (reference case): https://clouddocs.web.cern.ch/
- Pepple, K. (2011). *Deploying OpenStack*. O'Reilly Media.
- Reese, G. (2009). *Cloud Application Architectures*. O'Reilly Media. (Background on cloud IaaS patterns)

---

## 14. Glossary

| Term | Definition |
|------|-----------|
| **Application Credential** | Scoped, revocable credential for programmatic OpenStack access, not tied to user password |
| **Availability Zone** | Logical fault-isolation partition within an OpenStack region |
| **Cinder** | OpenStack block storage service (≈ AWS EBS) |
| **Flavor** | Predefined VM hardware profile (vCPUs, RAM, disk) in Nova (≈ EC2 instance type) |
| **Floating IP** | Publicly routable IP address dynamically associable with a VM port (≈ AWS Elastic IP) |
| **Glance** | OpenStack image registry service (≈ AWS AMI registry) |
| **Heat** | OpenStack orchestration service; accepts HOT templates (≈ AWS CloudFormation) |
| **Horizon** | OpenStack web dashboard |
| **Host Aggregate** | Grouping of compute nodes by hardware capability, used for scheduling |
| **Keystone** | OpenStack identity, authentication, and service catalog service (≈ AWS IAM + STS) |
| **ML2 Plugin** | Modular Layer 2 Neutron plugin supporting multiple mechanism drivers |
| **Neutron** | OpenStack networking service (≈ AWS VPC) |
| **Nova** | OpenStack compute service; manages VM lifecycle (≈ AWS EC2) |
| **OIF** | Open Infrastructure Foundation; governs OpenStack project |
| **Port** | Virtual network interface in Neutron, associated with a subnet and MAC address |
| **Project** | Fundamental resource isolation unit in OpenStack (historically: *tenant*) |
| **Provider Network** | External network managed by the cloud operator, not tenant-controlled |
| **Security Group** | Stateful virtual firewall applied at the Neutron port level |
| **Service Catalog** | Keystone-managed directory of all OpenStack service API endpoints |
| **Swift** | OpenStack object storage service (≈ AWS S3) |
| **Token** | Time-limited Keystone credential carried in API request headers |
| **VXLAN** | Virtual Extensible LAN; Layer 2 overlay protocol used by Neutron for tenant network isolation |

---

*Lecture Notes Version: 2.0 · Course: ADSAI HPC e Cloud, University of Trieste · 2025/2026*
*Lecture: OpenStack Fundamentals*
