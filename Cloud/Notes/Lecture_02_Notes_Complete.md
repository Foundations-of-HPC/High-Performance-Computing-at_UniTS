# Cloud Service Models and Economics

##  Cloud Computing Course 2024/2025

---

## Document Information

| | |
|---|---|
| **Course** | HIGH PERFORMANCE AND CLOUD COMPUTING - Cloud Computing Module |
| **Lecture** | Cloud Service Models and Economics |
| **Duration** | ~1 hour 45 minutes |
| **Prerequisites** | Foundations and Distributed Systems |
| **Associated Exercise** | Exercise_02_Cloud_Economics.md (~2 hours) |

---

## Learning Objectives

By the end of this lecture, students will be able to:

1. **Define** cloud computing using NIST's essential characteristics
2. **Distinguish** between IaaS, PaaS, SaaS, and FaaS service models
3. **Evaluate** deployment models (public, private, hybrid, multi-cloud)
4. **Analyze** cloud economics, including CapEx vs OpEx trade-offs
5. **Calculate** Total Cost of Ownership for cloud vs on-premise scenarios
6. **Apply** FinOps principles for cloud cost management
7. **Compare** major cloud providers and their service offerings

---

## Table of Contents

1. [What is Cloud Computing?](#1-what-is-cloud-computing)
2. [The Five Essential Characteristics](#2-the-five-essential-characteristics)
3. [Service Models](#3-service-models)
4. [Deployment Models](#4-deployment-models)
5. [Cloud Economics](#5-cloud-economics)
6. [Cloud Pricing Models](#6-cloud-pricing-models)
7. [Total Cost of Ownership](#7-total-cost-of-ownership)
8. [Introduction to FinOps](#8-introduction-to-finops)
9. [Major Cloud Providers](#9-major-cloud-providers)
10. [Key Takeaways](#10-key-takeaways)
11. [References and Further Reading](#11-references-and-further-reading)
12. [Glossary](#12-glossary)

---

## 1. What is Cloud Computing?

### 1.1 The Need for a Definition

Before the NIST definition became standard, "cloud computing" meant different things to different people. Common misconceptions included:

- **"It's just someone else's computer"**: This oversimplifies the model, missing the essential characteristics that distinguish cloud from traditional hosting
- **"It's virtualization"**: Virtualization is a key enabling technology, but it's only one component
- **"It's hosting"**: Traditional hosting lacks the self-service, elasticity, and measured service that define cloud

A precise definition matters because it allows us to:
- Evaluate whether a service truly offers cloud benefits
- Compare offerings across providers
- Understand what capabilities we should expect
- Design systems that leverage cloud characteristics

### 1.2 The NIST Definition

The National Institute of Standards and Technology (NIST) published Special Publication 800-145 in 2011, establishing the canonical definition of cloud computing:

> **"Cloud computing is a model for enabling ubiquitous, convenient, on-demand network access to a shared pool of configurable computing resources (e.g., networks, servers, storage, applications, and services) that can be rapidly provisioned and released with minimal management effort or service provider interaction."**

This definition emphasizes several key points:
- **Ubiquitous and convenient access**: Available from anywhere, easy to use
- **On-demand**: Resources available when needed, not pre-provisioned
- **Shared pool**: Resources are pooled and shared across customers
- **Configurable**: Resources can be customized to needs
- **Rapidly provisioned and released**: Fast scaling in both directions
- **Minimal management effort**: Automated, self-service

### 1.3 The NIST Model Structure

NIST defines cloud computing through three dimensions:

1. **Five Essential Characteristics**: What makes a service "cloud"
2. **Three Service Models**: How services are delivered (IaaS, PaaS, SaaS)
3. **Four Deployment Models**: Where cloud runs (public, private, hybrid, community)

We will examine each dimension in detail.

---

## 2. The Five Essential Characteristics

For a service to be considered cloud computing, it must exhibit all five of these characteristics. If any is missing, the service may be useful but is not truly "cloud."

### 2.1 On-demand Self-service

**Definition**: A consumer can unilaterally provision computing capabilities, such as server time and network storage, as needed automatically without requiring human interaction with each service provider.

**What this means in practice**:
- No phone calls to sales representatives
- No support tickets to provision resources
- No waiting for approval workflows
- Web console or API access available 24/7
- Resources can be provisioned in minutes, not weeks

**Example**: At 2am on a Sunday, you can launch 100 virtual machines via an API call without any human interaction with the cloud provider.

**Why it matters**: This characteristic enables agility. Teams can experiment, prototype, and respond to demand without waiting for IT procurement processes.

### 2.2 Broad Network Access

**Definition**: Capabilities are available over the network and accessed through standard mechanisms that promote use by heterogeneous thin or thick client platforms.

**What this means in practice**:
- Access from any device: laptops, phones, tablets, servers
- Standard protocols: HTTPS, SSH, standard APIs
- No proprietary client software required
- Works from anywhere with network connectivity

**Implication**: Cloud computing requires network connectivity. This is both a feature (access from anywhere) and a limitation (no network = no cloud access). Organizations must consider network reliability and latency when adopting cloud.

### 2.3 Resource Pooling

**Definition**: The provider's computing resources are pooled to serve multiple consumers using a multi-tenant model, with different physical and virtual resources dynamically assigned and reassigned according to consumer demand.

**What this means in practice**:
- Physical resources (servers, storage, network) are shared across many customers
- Customers generally don't know or control the exact physical location of resources
- Resources are dynamically allocated based on demand
- Multi-tenancy: multiple customers share infrastructure while maintaining isolation

**Location independence**: Customers can often specify location at a higher level of abstraction (region, availability zone) but not the specific data center or server.

**Why it matters**: Resource pooling enables economies of scale. By sharing infrastructure across thousands of customers, providers achieve higher utilization and lower per-unit costs than any single customer could achieve alone.

### 2.4 Rapid Elasticity

**Definition**: Capabilities can be elastically provisioned and released, in some cases automatically, to scale rapidly outward and inward commensurate with demand. To the consumer, the capabilities available for provisioning often appear to be unlimited and can be appropriated in any quantity at any time.

**What this means in practice**:
- **Scale up/out**: Add capacity when demand increases
- **Scale down/in**: Remove capacity when demand decreases
- **Automatic**: Can be triggered automatically based on metrics
- **Appears unlimited**: From the customer's perspective, there's no practical limit

**The key differentiator**: Traditional infrastructure scales in months (procurement, delivery, installation). Cloud scales in minutes. This changes how applications can be designed.

**Example**: An e-commerce site can scale from 10 servers to 1,000 servers during Black Friday, then back to 10 servers afterward—paying only for what was used.

### 2.5 Measured Service

**Definition**: Cloud systems automatically control and optimize resource use by leveraging a metering capability at some level of abstraction appropriate to the type of service. Resource usage can be monitored, controlled, and reported, providing transparency for both the provider and consumer of the utilized service.

**What this means in practice**:
- Pay-per-use pricing: charged for actual consumption
- Metering: CPU hours, storage GB, network bytes, API calls
- Visibility: dashboards, billing reports, usage analytics
- Control: budgets, alerts, spending limits

**The utility model**: Cloud computing operates like utilities (electricity, water). You don't buy a power plant; you pay for kilowatt-hours consumed. Similarly, you don't buy servers; you pay for compute-hours consumed.

### 2.6 Summary: The Litmus Test

When evaluating whether a service is truly "cloud," check for all five characteristics:

| Characteristic | Question to Ask |
|----------------|-----------------|
| On-demand self-service | Can I provision resources without human interaction? |
| Broad network access | Can I access from anywhere via standard protocols? |
| Resource pooling | Are resources shared across customers? |
| Rapid elasticity | Can I scale up and down quickly? |
| Measured service | Do I pay for what I use with visibility into consumption? |

If any answer is "no," the service may not deliver the full benefits of cloud computing.

---

## 3. Service Models

Service models define what the cloud provider manages versus what the customer manages. They form a stack of increasing abstraction.

### 3.1 The Cloud Stack

```
┌─────────────────────────────────────────────┐
│              SaaS                           │  ← Most abstraction
│     Software as a Service                   │     Least management
│     (Gmail, Salesforce, Office 365)         │
├─────────────────────────────────────────────┤
│              FaaS                           │
│     Function as a Service                   │
│     (Lambda, Cloud Functions)               │
├─────────────────────────────────────────────┤
│              PaaS                           │
│     Platform as a Service                   │
│     (Heroku, App Engine)                    │
├─────────────────────────────────────────────┤
│              IaaS                           │  ← Least abstraction
│     Infrastructure as a Service             │     Most management
│     (EC2, Azure VMs, GCE)                   │
└─────────────────────────────────────────────┘
```

As you move up the stack:
- **More abstraction**: Less visibility into underlying infrastructure
- **Less management**: Provider handles more operational concerns
- **Less flexibility**: Fewer options for customization
- **Faster deployment**: Less to configure and manage

### 3.2 IaaS: Infrastructure as a Service

**Definition**: The capability to provision processing, storage, networks, and other fundamental computing resources where the consumer can deploy and run arbitrary software, including operating systems and applications.

#### What the Provider Manages
- Physical data centers
- Physical servers, storage, networking hardware
- Hypervisor / virtualization layer
- Physical security, power, cooling

#### What You Manage
- Operating system (patches, updates, configuration)
- Middleware and runtime
- Applications
- Data

#### Responsibility Model

```
┌──────────────────────────────────────────┐
│ Applications          │  YOU MANAGE      │
├───────────────────────┤                  │
│ Data                  │                  │
├───────────────────────┤                  │
│ Runtime               │                  │
├───────────────────────┤                  │
│ Middleware            │                  │
├───────────────────────┤                  │
│ Operating System      │                  │
├───────────────────────┼──────────────────┤
│ Virtualization        │  PROVIDER        │
├───────────────────────┤  MANAGES         │
│ Servers               │                  │
├───────────────────────┤                  │
│ Storage               │                  │
├───────────────────────┤                  │
│ Networking            │                  │
└───────────────────────┴──────────────────┘
```

#### Examples
- **AWS EC2**: Virtual machines with full OS control
- **Azure Virtual Machines**: Windows and Linux VMs
- **Google Compute Engine**: VMs on Google infrastructure
- **DigitalOcean Droplets**: Simple VM hosting

#### When to Use IaaS
- You need full control over the operating system
- You're running legacy applications that require specific OS configurations
- You have existing VM-based operations expertise
- You need to comply with regulations that require OS-level control
- You're migrating existing on-premise VMs to cloud (lift-and-shift)

#### IaaS Characteristics
- **Maximum flexibility**: Install any software, any configuration
- **Maximum responsibility**: You handle patching, security, scaling
- **Closest to traditional IT**: Skills transfer from on-premise
- **Slowest deployment**: Most components to configure

### 3.3 PaaS: Platform as a Service

**Definition**: The capability to deploy onto the cloud infrastructure consumer-created or acquired applications created using programming languages, libraries, services, and tools supported by the provider.

#### What the Provider Manages
- Everything IaaS manages, plus:
- Operating system
- Middleware
- Runtime environment

#### What You Manage
- Applications
- Data

#### Responsibility Model

```
┌──────────────────────────────────────────┐
│ Applications          │  YOU MANAGE      │
├───────────────────────┤                  │
│ Data                  │                  │
├───────────────────────┼──────────────────┤
│ Runtime               │  PROVIDER        │
├───────────────────────┤  MANAGES         │
│ Middleware            │                  │
├───────────────────────┤                  │
│ Operating System      │                  │
├───────────────────────┤                  │
│ Virtualization        │                  │
├───────────────────────┤                  │
│ Servers               │                  │
├───────────────────────┤                  │
│ Storage               │                  │
├───────────────────────┤                  │
│ Networking            │                  │
└───────────────────────┴──────────────────┘
```

#### Examples
- **Heroku**: Deploy applications from Git push
- **Google App Engine**: Managed application platform
- **AWS Elastic Beanstalk**: Deploy and scale web apps
- **Azure App Service**: Web and mobile app hosting
- **Cloud Foundry**: Open-source PaaS

#### When to Use PaaS
- You want to focus on application code, not infrastructure
- Your application fits the platform's supported languages/frameworks
- You value faster time-to-market over flexibility
- You don't need custom OS configurations
- You're building new cloud-native applications

#### PaaS Characteristics
- **Focus on code**: Deploy your application, platform handles the rest
- **Automatic scaling**: Platform scales based on demand
- **Managed updates**: Provider patches OS and middleware
- **Less flexibility**: Must use supported languages and frameworks
- **Faster deployment**: Push code, platform handles deployment

### 3.4 SaaS: Software as a Service

**Definition**: The capability to use the provider's applications running on a cloud infrastructure. The applications are accessible from various client devices through either a thin client interface, such as a web browser, or a program interface.

#### What the Provider Manages
- Everything: infrastructure through application
- Application functionality
- Feature updates
- Data backups

#### What You Manage
- Your data within the application
- User configuration and settings
- User management and access control

#### Examples
- **Email**: Gmail, Microsoft 365 (Outlook)
- **CRM**: Salesforce
- **Collaboration**: Slack, Microsoft Teams, Zoom
- **Storage**: Dropbox, Google Drive, OneDrive
- **Productivity**: Google Workspace, Microsoft 365

#### When to Use SaaS
- A ready-made application meets your needs
- You want zero infrastructure management
- You value rapid deployment (sign up and start using)
- You're comfortable with vendor control over the application
- The application is not a competitive differentiator

#### SaaS Characteristics
- **Zero infrastructure management**: Just use the application
- **Immediate availability**: Sign up and start using
- **Automatic updates**: Always on the latest version
- **Least flexibility**: Use the features provided; limited customization
- **Multi-tenant by design**: Shared with other customers

### 3.5 FaaS: Function as a Service

FaaS, often called "serverless computing," extends the abstraction beyond PaaS. While not part of the original NIST model, it has become a significant service model.

**Definition**: A cloud execution model where the cloud provider dynamically manages the allocation of machine resources. The customer writes functions (small pieces of code) that are executed in response to events.

#### What the Provider Manages
- Everything PaaS manages, plus:
- Function execution environment
- Scaling (including to zero)
- High availability

#### What You Manage
- Function code only
- Function configuration (memory, timeout)
- Event triggers

#### Key Characteristics

**Event-driven**: Functions execute in response to events:
- HTTP requests (API calls)
- Database changes
- File uploads (e.g., S3 object creation)
- Queue messages
- Scheduled triggers (cron-like)

**Stateless**: Functions don't maintain state between invocations. Any state must be stored externally (database, cache).

**Auto-scaling**: Automatically scales from zero to thousands of concurrent executions based on demand.

**Sub-second billing**: Pay per invocation and execution duration (often in 1ms increments).

#### Examples
- **AWS Lambda**: The original FaaS, launched 2014
- **Azure Functions**: Microsoft's serverless platform
- **Google Cloud Functions**: GCP's event-driven functions
- **Cloudflare Workers**: Edge-based serverless

#### When to Use FaaS

**Good fit**:
- Event processing (file uploads, queue messages)
- API backends with variable traffic
- Scheduled tasks (cron jobs)
- Webhooks and integrations
- Glue code between services

**Poor fit**:
- Long-running processes (Lambda limit: 15 minutes)
- Stateful applications
- Consistent high-volume traffic (always-on might be cheaper)
- Low-latency requirements (cold start penalty)
- Applications requiring specific runtime environments

#### The "Cold Start" Problem

When a function hasn't been invoked recently, the platform must:
1. Allocate compute resources
2. Load the function code
3. Initialize the runtime environment

This "cold start" can add latency (100ms to several seconds depending on runtime and code size). Subsequent invocations within a time window reuse the "warm" environment and are faster.

**Mitigation strategies**:
- Keep functions small
- Use lightweight runtimes (Python, Node.js faster than Java)
- Use provisioned concurrency (pre-warmed instances, at extra cost)

### 3.6 Service Model Comparison

| Aspect | IaaS | PaaS | FaaS | SaaS |
|--------|------|------|------|------|
| You manage | OS and up | App and data | Code only | Data only |
| Flexibility | Maximum | Medium | Low | Minimum |
| Complexity | High | Medium | Low | None |
| Time to deploy | Hours/days | Minutes | Seconds | Instant |
| Scaling | Manual/auto | Automatic | Automatic | Automatic |
| Use case | Custom infra | App development | Event processing | End-user apps |

### 3.7 Choosing a Service Model

The right choice depends on your requirements:

**Choose IaaS when**:
- You need full control over the environment
- You're running legacy applications
- You have specific compliance requirements
- You're doing lift-and-shift migration

**Choose PaaS when**:
- You want to focus on code, not infrastructure
- Your app fits supported frameworks
- You value development speed
- You're building new applications

**Choose FaaS when**:
- You have event-driven workloads
- Traffic is variable or unpredictable
- You want to pay only for actual execution
- Functions are short-lived

**Choose SaaS when**:
- A ready-made solution exists
- You don't want any infrastructure responsibility
- The application is not a differentiator
- You need to deploy immediately

---

## 4. Deployment Models

Deployment models describe where cloud infrastructure is located and who has access to it.

### 4.1 Public Cloud

**Definition**: The cloud infrastructure is provisioned for open use by the general public. It may be owned, managed, and operated by a business, academic, or government organization, or some combination of them. It exists on the premises of the cloud provider.

#### Characteristics
- Infrastructure owned and operated by the provider
- Shared across all customers (multi-tenant)
- Accessed over the public internet
- Pay-per-use pricing
- Virtually unlimited scale

#### Advantages
- **No upfront investment**: Start immediately, pay as you go
- **Economies of scale**: Provider's scale drives lower costs
- **Global reach**: Data centers worldwide
- **Managed operations**: Provider handles infrastructure

#### Considerations
- **Shared infrastructure**: Your workloads run alongside others
- **Data sovereignty**: Data may be stored in various jurisdictions
- **Vendor lock-in**: Proprietary services create switching costs
- **Network dependency**: Requires internet connectivity

#### Examples
- Amazon Web Services (AWS)
- Microsoft Azure
- Google Cloud Platform (GCP)
- Alibaba Cloud
- Oracle Cloud Infrastructure

### 4.2 Private Cloud

**Definition**: The cloud infrastructure is provisioned for exclusive use by a single organization comprising multiple consumers. It may be owned, managed, and operated by the organization, a third party, or some combination of them, and it may exist on or off premises.

#### Characteristics
- Dedicated to a single organization
- Can be on-premise or hosted
- Organization controls the infrastructure
- Not shared with other organizations

#### Advantages
- **Full control**: Complete control over hardware, software, security
- **Data sovereignty**: Data stays within your control
- **Customization**: Configure to specific requirements
- **Compliance**: Easier to meet regulatory requirements

#### Considerations
- **Higher cost**: No economies of scale from sharing
- **Limited scale**: Bounded by your infrastructure investment
- **Operational burden**: You manage (or pay someone to manage)
- **Capital expenditure**: Requires upfront investment

#### Examples
- VMware vSphere / vCloud
- OpenStack deployments
- Microsoft Azure Stack
- AWS Outposts (hybrid approach)

#### Hosted Private Cloud
A third party hosts dedicated infrastructure for your organization. You get private cloud benefits without managing physical infrastructure, but at higher cost than public cloud.

### 4.3 Hybrid Cloud

**Definition**: The cloud infrastructure is a composition of two or more distinct cloud infrastructures (private, community, or public) that remain unique entities, but are bound together by standardized or proprietary technology that enables data and application portability.

#### Key Requirement
The infrastructures must be **interconnected**—simply using both public and private cloud doesn't make it hybrid unless they work together as an integrated system.

#### Use Cases

**Cloud bursting**: Run baseline workloads on private cloud; burst to public cloud during peak demand.
```
Normal:   [Private Cloud: 10 servers] 
Peak:     [Private Cloud: 10 servers] + [Public Cloud: 50 servers]
```

**Tiered data**: Keep sensitive data on private cloud; run analytics on public cloud.
```
[Private Cloud: Customer PII] → [Public Cloud: Anonymized analytics]
```

**Disaster recovery**: Run production on private; replicate to public for DR.
```
[Private Cloud: Production] → [Public Cloud: DR replica]
```

**Gradual migration**: Migrate workloads incrementally from private to public.

#### Challenges
- **Networking complexity**: Secure connectivity between environments
- **Management complexity**: Different tools, APIs, skills
- **Data synchronization**: Keeping data consistent across environments
- **Security consistency**: Uniform security policies across environments

### 4.4 Multi-Cloud

**Definition**: Using services from multiple public cloud providers simultaneously.

Note: NIST originally defined a "Community Cloud" as the fourth deployment model (shared by organizations with common concerns). Multi-cloud has emerged as a more common pattern.

#### Motivations

**Avoid vendor lock-in**: Not dependent on a single provider's pricing, features, or availability.

**Best-of-breed services**: Use each provider's strongest services:
- AWS for breadth of services
- GCP for machine learning and Kubernetes
- Azure for Microsoft integration

**Regulatory requirements**: Some regulations require data to be stored with multiple providers or in specific jurisdictions.

**Resilience**: Survive provider-level outages (though rare).

#### Challenges

**Operational complexity**: Different APIs, CLIs, consoles for each provider.

**Skill requirements**: Team must know multiple platforms.

**Networking**: Connecting workloads across providers is complex and costly.

**Data transfer costs**: Moving data between providers is expensive.

**Inconsistent tooling**: Management, monitoring, security tools differ.

#### Reality Check
True multi-cloud (same workload running across providers) is rare and complex. More common:
- Different workloads on different providers
- Secondary provider for specific services
- Avoiding lock-in at the design level (containers, Kubernetes)

### 4.5 Deployment Model Summary

| Model | Infrastructure | Best For |
|-------|---------------|----------|
| **Public** | Provider-owned, shared | Startups, variable workloads, rapid scaling |
| **Private** | Organization-owned, dedicated | Regulated industries, sensitive data, legacy systems |
| **Hybrid** | Mix of public + private | Gradual migration, burst capacity, data sovereignty |
| **Multi-cloud** | Multiple public providers | Avoiding lock-in, best-of-breed, regulatory requirements |

---

## 5. Cloud Economics

Understanding cloud economics is essential for making informed decisions about cloud adoption and ongoing cloud management.

### 5.1 CapEx vs OpEx

The fundamental shift in cloud economics is from Capital Expenditure (CapEx) to Operational Expenditure (OpEx).

#### Capital Expenditure (CapEx)

Traditional IT model:

- **Buy hardware upfront**: Servers, storage, networking equipment
- **Depreciate over time**: Typically 3-5 years
- **Fixed cost**: Pay whether you use it or not
- **Requires planning**: Estimate needs years in advance
- **Large initial investment**: Significant cash outlay

**Example CapEx budget**:
```
Year 0: $500,000 (hardware purchase)
Year 1: $50,000 (maintenance, power)
Year 2: $50,000 
Year 3: $50,000 + $500,000 (refresh)
...
```

#### Operational Expenditure (OpEx)

Cloud model:

- **Pay as you go**: Charged for actual consumption
- **Expense immediately**: No depreciation accounting
- **Variable cost**: Scales with usage
- **No planning required**: Provision on demand
- **No upfront investment**: Start immediately

**Example OpEx budget**:
```
Month 1: $5,000 (initial usage)
Month 2: $8,000 (growth)
Month 3: $4,000 (optimization)
...
```

### 5.2 Why OpEx Matters

#### Cash Flow
- No large upfront investment required
- Preserve capital for core business activities
- Easier for startups and small businesses

#### Flexibility
- Scale costs up with growth
- Scale costs down during downturns
- Match IT spend to business cycles

#### Risk Reduction
- No stranded assets if project fails
- No obsolete hardware to dispose of
- Technology changes don't strand investment

#### Agility
- Experiment without large commitment
- Fail fast, fail cheap
- Pivot quickly based on learnings

#### Tax Treatment
- Operating expenses often immediately deductible
- No depreciation schedules to manage
- Simpler accounting (though consult your CFO)

### 5.3 Economies of Scale

Cloud providers achieve economies of scale that individual organizations cannot match:

**Hardware procurement**: Buying millions of servers means significant discounts and custom hardware designs.

**Power and cooling**: Direct contracts with utilities, investment in renewable energy, efficient data center designs (PUE < 1.1).

**Operations**: Automated management reduces per-server staff costs. One engineer can manage thousands of servers.

**Utilization**: Statistical multiplexing across thousands of customers means higher average utilization.

These savings are passed to customers through lower prices than equivalent on-premise infrastructure would cost.

### 5.4 The Utilization Problem

Traditional IT suffers from poor utilization:

**Peak provisioning**: Buy for maximum expected load, even if it occurs rarely.

**Growth headroom**: Add capacity for expected growth that may not materialize.

**Average utilization**: Typical enterprise servers run at 10-20% average CPU utilization.

Cloud solves this through:
- **Sharing**: Your idle capacity serves other customers
- **Elasticity**: Scale to peak only when needed
- **Pay-per-use**: Only pay for actual consumption

---

## 6. Cloud Pricing Models

Cloud providers offer multiple pricing models to balance flexibility, commitment, and cost.

### 6.1 On-Demand Pricing

**How it works**: Pay per hour (or per second) with no commitment. Start and stop at any time.

**Characteristics**:
- Maximum flexibility
- No commitment required
- Highest per-unit price
- Ideal for variable or unknown workloads

**When to use**:
- Development and testing
- Unpredictable workloads
- Short-term projects
- Initial exploration before committing

**Example** (AWS EC2 t3.medium, us-east-1):
```
On-demand: $0.0416/hour × 730 hours/month = ~$30/month
```

### 6.2 Reserved Instances / Committed Use

**How it works**: Commit to using a specific instance type for 1-3 years in exchange for significant discounts.

**Characteristics**:
- Requires commitment (1 or 3 years)
- 30-75% discount versus on-demand
- Payment options: all upfront, partial upfront, no upfront (varying discounts)
- Best for predictable, steady-state workloads

**When to use**:
- Production workloads with predictable demand
- Databases and other always-on services
- Workloads you're confident will run long-term

**Example** (AWS EC2 t3.medium, 1-year reserved):
```
On-demand: ~$30/month
1-year reserved (no upfront): ~$20/month (33% savings)
3-year reserved (all upfront): ~$12/month (60% savings)
```

### 6.3 Spot Instances / Preemptible VMs

**How it works**: Bid on spare capacity at steep discounts. Instances can be terminated with short notice when capacity is needed.

**Characteristics**:
- Up to 90% discount
- Can be interrupted (2 minutes notice on AWS)
- Prices fluctuate based on supply/demand
- Requires fault-tolerant architecture

**When to use**:
- Batch processing
- CI/CD pipelines
- Big data analytics (Spark, etc.)
- Fault-tolerant web tiers (behind load balancer)
- HPC workloads that can checkpoint

**When NOT to use**:
- Databases
- Stateful applications
- Single points of failure
- Latency-sensitive workloads

**HPC consideration**: Spot instances are excellent for embarrassingly parallel HPC workloads where losing a node means losing only that node's work, not the entire job. Consider checkpointing for longer simulations.

### 6.4 Savings Plans

**How it works**: Commit to a consistent amount of usage (measured in $/hour) for 1-3 years, with flexibility in what resources you use.

**Characteristics**:
- More flexible than reserved instances
- Applies across instance families, regions, OS
- 20-50% discount
- Automatically applies to eligible usage

**Example**:
```
Commitment: $10/hour for 1 year
This covers any combination of compute that totals $10/hour
Discount applies automatically to matching usage
```

### 6.5 Pricing Model Comparison

| Model | Discount | Commitment | Flexibility | Risk |
|-------|----------|------------|-------------|------|
| On-Demand | 0% | None | Maximum | None |
| Savings Plans | 20-50% | 1-3 years ($) | High | Commitment |
| Reserved | 30-75% | 1-3 years (instance) | Low | Commitment |
| Spot | 60-90% | None | Variable | Interruption |

### 6.6 Pricing Strategy

A typical cost-optimized strategy:

1. **Cover baseline with Reserved/Savings Plans**: Predictable, always-on workloads
2. **Use Spot for fault-tolerant workloads**: Batch, CI/CD, stateless web tiers
3. **Use On-Demand for the rest**: Variable workloads, peaks, new projects

```
│ Peak (On-Demand) ─────────────────────
│                  ╱╲      ╱╲
│ Variable (Spot) ─╱──╲────╱──╲─────────
│                 ╱    ╲  ╱    ╲
│ Baseline (Reserved) ───────────────────
└─────────────────────────────────────────
```

---

## 7. Total Cost of Ownership

Total Cost of Ownership (TCO) compares the complete cost of cloud versus on-premise over time.

### 7.1 On-Premise Costs (Often Underestimated)

A complete on-premise TCO must include:

**Hardware**:
- Servers (compute, storage, networking)
- Racks, cables, PDUs
- Redundant components
- Refresh cycles (typically 3-5 years)

**Facilities**:
- Data center space (build or lease)
- Power (including redundant feeds)
- Cooling (often 30-50% of power costs)
- Physical security
- Fire suppression

**Network**:
- Internet connectivity
- WAN links between locations
- Load balancers, firewalls

**Operations**:
- System administrators (24/7 coverage)
- Network engineers
- Security team
- On-call rotations

**Software**:
- Operating system licenses
- Virtualization licenses (VMware, etc.)
- Management and monitoring tools
- Backup software

**Overhead**:
- Procurement time and process
- Vendor management
- Compliance and auditing
- Insurance

### 7.2 Cloud Costs (Often Underestimated)

Complete cloud TCO must include:

**Compute**:
- VM instances
- Container services
- Serverless invocations

**Storage**:
- Object storage (S3, etc.)
- Block storage (EBS, etc.)
- Database storage

**Network** (often surprising):
- **Data egress**: Transferring data OUT of cloud ($0.09/GB+)
- **Cross-AZ transfer**: Traffic between availability zones
- **Cross-region transfer**: Even more expensive
- Load balancer hours and data processing

**Managed Services**:
- Managed databases (RDS, etc.)
- Managed Kubernetes (EKS, etc.)
- Caching services (ElastiCache, etc.)

**Support**:
- Support plans (can be 3-10% of spend)
- Training

**Operations** (don't forget!):
- Cloud engineers (different skills than on-prem)
- Cost management (FinOps)
- Security in the cloud

### 7.3 Hidden Cloud Costs

Common surprises in cloud bills:

| Cost | Why It's Surprising |
|------|---------------------|
| Data egress | Charged per GB leaving the cloud |
| Cross-AZ traffic | Multi-AZ deployments generate internal transfer costs |
| API calls | S3 requests, Lambda invocations, etc. |
| Idle resources | Forgotten instances, unattached volumes |
| Over-provisioning | Larger instances than needed |
| Snapshots | Old EBS snapshots accumulate |
| Logging | CloudWatch logs can grow expensive |
| NAT Gateway | Per-GB data processing charges |

**Rule of thumb**: Budget 20-30% above computed estimates for surprises until you have experience with actual usage patterns.

### 7.4 When Cloud is Cheaper

Cloud tends to be more cost-effective when:

- **Variable workloads**: Can scale down when not needed
- **Rapid growth**: No upfront investment in capacity
- **Unknown demand**: Pay for actuals, not estimates
- **Short projects**: No stranded assets
- **Small scale**: Can't achieve on-prem economies

### 7.5 When On-Premise is Cheaper

On-premise may be more cost-effective when:

- **Large, stable workloads**: High utilization amortizes fixed costs
- **Long time horizon**: Time to amortize capital investment
- **Predictable demand**: Accurate capacity planning possible
- **Low egress**: Data stays mostly on-premise
- **Existing investment**: Sunk costs in facilities and staff

### 7.6 TCO Analysis Approach

1. **Define the workload**: What are you running? For how long?
2. **Size requirements**: Compute, storage, network
3. **Calculate on-premise costs**: ALL costs, not just hardware
4. **Calculate cloud costs**: ALL costs, including egress and support
5. **Consider intangibles**: Agility, risk, opportunity cost
6. **Sensitivity analysis**: What if assumptions are wrong?

---

## 8. Introduction to FinOps

FinOps (Cloud Financial Operations) is an operational framework for managing cloud costs.

### 8.1 Why FinOps?

Traditional IT had predictable costs (CapEx budgets). Cloud's variable costs create new challenges:

- **Anyone can spend**: Developers can provision resources directly
- **Easy to overspend**: No physical constraint on resource creation
- **Complex pricing**: Multiple pricing models, many services
- **Delayed visibility**: Bills arrive after consumption
- **Organizational silos**: Engineering, finance, business don't communicate

### 8.2 FinOps Principles

**Teams own their costs**: Engineers who provision resources are accountable for costs.

**Business value drives decisions**: Not just "spend less" but "optimize value."

**Everyone collaborates**: Finance, engineering, and business work together.

**Timely reports**: Real-time or near-real-time cost visibility.

**Centralized governance**: Central team sets policies and practices.

**Continuous optimization**: FinOps is ongoing, not one-time.

### 8.3 FinOps Practices

#### Tagging
Label all resources with metadata:
- Owner (team or individual)
- Project or application
- Environment (dev, staging, prod)
- Cost center

```
Tags:
  Owner: platform-team
  Project: customer-api
  Environment: production
  CostCenter: CC-1234
```

Without tags, you cannot allocate costs to teams or projects.

#### Budgets and Alerts
Set spending limits and alert before exceeded:
- Monthly budget per team/project
- Alert at 50%, 80%, 100% of budget
- Automatic notifications to owners

#### Rightsizing
Match instance size to actual usage:
- Analyze CPU, memory utilization
- Downsize over-provisioned instances
- Use appropriate instance families

**Example**: A t3.large running at 5% CPU could be a t3.small at 20% CPU—75% savings.

#### Reserved Capacity Planning
Identify workloads suitable for reservations:
- Analyze usage patterns over time
- Calculate break-even points
- Purchase reservations for stable workloads

#### Cleanup Automation
Automatically remove unused resources:
- Terminate idle instances
- Delete unattached volumes
- Remove old snapshots
- Clean up unused load balancers

#### Cost Allocation
Chargeback or showback to teams:
- **Chargeback**: Teams pay from their budget
- **Showback**: Teams see costs but don't pay directly

Both create accountability for cloud spending.

### 8.4 FinOps Tools

**Native tools**:
- AWS Cost Explorer, Budgets
- Azure Cost Management
- GCP Billing Reports

**Third-party tools**:
- CloudHealth
- Cloudability
- Spot.io (formerly Spotinst)
- Kubecost (for Kubernetes)

### 8.5 FinOps Lifecycle

1. **Inform**: Visibility into who is spending what
2. **Optimize**: Take action to improve efficiency
3. **Operate**: Continuously maintain cost efficiency

FinOps is not a project with an end date—it's an ongoing operational practice.

---

## 9. Major Cloud Providers

### 9.1 Market Overview

As of 2024, the cloud infrastructure market is dominated by three providers:

| Provider | Market Share | Strengths |
|----------|-------------|-----------|
| AWS | ~32% | Broadest services, most mature, market leader |
| Azure | ~23% | Enterprise integration, hybrid cloud, Microsoft stack |
| GCP | ~10% | Data/ML, Kubernetes, networking |

Other providers include Oracle Cloud, IBM Cloud, Alibaba Cloud (dominant in China), and specialized providers like DigitalOcean.

### 9.2 AWS (Amazon Web Services)

**Launched**: 2006 (EC2 and S3)

**Strengths**:
- First mover, most mature
- Broadest service catalog (200+ services)
- Most availability zones and regions
- Strongest ecosystem (partners, training, community)

**Considerations**:
- Can be complex (many services, many options)
- Some services have proprietary APIs
- Premium pricing on some services

**HPC-specific offerings**:
- AWS ParallelCluster
- HPC-optimized instances (Hpc6a, Hpc7g)
- Elastic Fabric Adapter (EFA) for low-latency networking
- FSx for Lustre

### 9.3 Microsoft Azure

**Launched**: 2010

**Strengths**:
- Strong enterprise relationships (existing Microsoft customers)
- Best Microsoft stack integration (Active Directory, Office 365)
- Strong hybrid cloud story (Azure Stack, Arc)
- Growing service catalog

**Considerations**:
- Some services less mature than AWS
- Naming can be confusing
- Enterprise-focused (complexity for small users)

**HPC-specific offerings**:
- Azure CycleCloud
- HBv3/HBv4 series VMs with InfiniBand
- Azure Batch

### 9.4 Google Cloud Platform (GCP)

**Launched**: 2008 (App Engine), 2013 (GCE)

**Strengths**:
- Excellent for data and ML (BigQuery, Vertex AI)
- Kubernetes leadership (GKE is excellent)
- Strong networking (Google's global network)
- Competitive pricing

**Considerations**:
- Smaller service catalog than AWS
- Fewer regions than AWS/Azure
- Less enterprise focus historically

**HPC-specific offerings**:
- Compute Engine HPC VMs
- Cloud HPC Toolkit
- Batch service

### 9.5 Service Name Mapping

Same concepts, different names:

| Category | AWS | Azure | GCP |
|----------|-----|-------|-----|
| Compute (VMs) | EC2 | Virtual Machines | Compute Engine |
| Object Storage | S3 | Blob Storage | Cloud Storage |
| Block Storage | EBS | Managed Disks | Persistent Disk |
| Serverless | Lambda | Functions | Cloud Functions |
| Container Registry | ECR | Container Registry | Artifact Registry |
| Managed Kubernetes | EKS | AKS | GKE |
| SQL Database | RDS | SQL Database | Cloud SQL |
| NoSQL (Key-Value) | DynamoDB | Cosmos DB | Firestore |
| Data Warehouse | Redshift | Synapse Analytics | BigQuery |
| ML Platform | SageMaker | Machine Learning | Vertex AI |

### 9.6 Why AWS for This Course

This course uses AWS as the primary cloud platform because:

1. **Market leader**: Most likely to encounter in jobs
2. **Best free tier**: Most generous for learning
3. **Excellent documentation**: Extensive tutorials and guides
4. **Transferable skills**: Concepts apply to any provider
5. **HPC focus**: ParallelCluster and HPC instances

The concepts you learn apply to all providers—only the specific service names and APIs differ.

---

## 10. Key Takeaways

1. **NIST defines cloud through 5 essential characteristics**
   - On-demand self-service, broad network access, resource pooling, rapid elasticity, measured service
   - All five must be present for a service to be truly "cloud"

2. **Service models trade control for convenience**
   - IaaS: You manage OS and up
   - PaaS: You manage app and data
   - FaaS: You manage code only
   - SaaS: You manage data configuration only
   - Choose based on control needs vs. operational burden

3. **Deployment models determine where cloud runs**
   - Public: Shared, provider-owned
   - Private: Dedicated, organization-controlled
   - Hybrid: Integrated combination
   - Multi-cloud: Multiple providers

4. **Cloud converts CapEx to OpEx**
   - Pay for use, not capacity
   - Variable costs scale with business
   - No upfront investment required
   - Enables agility and reduces risk

5. **Multiple pricing models enable optimization**
   - On-demand: Maximum flexibility, highest cost
   - Reserved: Commitment for discounts
   - Spot: Steep discounts, interruption risk
   - Savings Plans: Flexible commitment

6. **TCO requires honest, complete comparison**
   - Include ALL on-premise costs
   - Include ALL cloud costs (especially egress)
   - Consider intangibles (agility, risk)

7. **FinOps is essential for cloud success**
   - Tag everything
   - Set budgets and alerts
   - Rightsize resources
   - Clean up unused resources
   - Make teams accountable

---

## 11. References and Further Reading

### Standards and Frameworks

1. **NIST SP 800-145: The NIST Definition of Cloud Computing**
   - The canonical cloud definition
   - Available: https://csrc.nist.gov/publications/detail/sp/800-145/final

2. **FinOps Foundation**
   - FinOps framework and community
   - Available: https://www.finops.org/

### Provider Documentation

3. **AWS**
   - Pricing: https://aws.amazon.com/pricing/
   - Economics: https://aws.amazon.com/economics/
   - Calculator: https://calculator.aws/

4. **Azure**
   - Pricing: https://azure.microsoft.com/pricing/
   - Calculator: https://azure.microsoft.com/pricing/calculator/

5. **GCP**
   - Pricing: https://cloud.google.com/pricing
   - Calculator: https://cloud.google.com/products/calculator

### Books

6. **Bastani, A., & Carnegie, L. (2020). Cloud FinOps. O'Reilly Media.**
   - Comprehensive guide to cloud financial management

7. **Wittig, A., & Wittig, M. (2019). Amazon Web Services in Action (2nd ed.). Manning.**
   - Practical introduction to AWS

### Cost Optimization

8. **AWS Well-Architected Framework - Cost Optimization Pillar**
   - Best practices for cost efficiency
   - Available: https://docs.aws.amazon.com/wellarchitected/latest/cost-optimization-pillar/

9. **Google Cloud Architecture Framework - Cost Optimization**
   - GCP cost optimization guidance
   - Available: https://cloud.google.com/architecture/framework/cost-optimization

---

## 12. Glossary

**CapEx (Capital Expenditure)**: Upfront investment in physical assets that are depreciated over time.

**Cold Start**: The latency penalty when a serverless function must be initialized before execution.

**Data Egress**: Transferring data out of a cloud region; typically incurs charges.

**Elasticity**: The ability to rapidly scale resources up or down to match demand.

**FinOps**: Cloud Financial Operations; the practice of managing cloud costs.

**IaaS (Infrastructure as a Service)**: Cloud service model providing virtual infrastructure (VMs, storage, network).

**Multi-tenancy**: Multiple customers sharing the same physical infrastructure with logical isolation.

**NIST**: National Institute of Standards and Technology; published the standard cloud definition.

**On-demand**: Pay-per-use pricing with no commitment.

**OpEx (Operational Expenditure)**: Ongoing operational costs expensed as incurred.

**PaaS (Platform as a Service)**: Cloud service model providing a platform for deploying applications.

**Reserved Instance**: Commitment to use specific resources for 1-3 years in exchange for discounts.

**Rightsizing**: Matching instance size to actual workload requirements.

**SaaS (Software as a Service)**: Cloud service model providing complete applications.

**Savings Plan**: Commitment to a consistent spending level for 1-3 years with flexible resource usage.

**FaaS (Function as a Service)**: Cloud service model for event-driven function execution (serverless).

**Spot Instance**: Spare cloud capacity available at steep discounts but subject to interruption.

**TCO (Total Cost of Ownership)**: Complete cost comparison including all direct and indirect costs.

**Tagging**: Adding metadata labels to cloud resources for organization and cost allocation.

---

## Exercises

See the accompanying document: **Exercise_02_Cloud_Economics.md**

The exercise covers:
- Service model selection for different scenarios
- TCO analysis: Cloud vs on-premise
- Pricing model optimization
- FinOps practice implementation


---

*Document Version: 1.0*  
*Course: HIGH PERFORMANCE AND CLOUD COMPUTING  2025/2026*  
*Applied Data Science & Artificial Intelligence - University of Trieste*