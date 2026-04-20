# Lecture 1: Cloud Computing - Foundations and Distributed Systems

##  Cloud Computing Course 2024/2025

---

## Document Information

| | |
|---|---|
| **Course** | HIGH PERFORMANCE AND CLOUD COMPUTING - Cloud Computing Module |
| **Lecture** |  Foundations and Distributed Systems |
| **Duration** | 1 hour 45 minutes |
| **Prerequisites** | Basic understanding of computer networks and operating systems |
| **Associated Exercise** | Exercise_01_Distributed_Systems.md (~2 hours) |

---

## Learning Objectives

By the end of this lecture, students will be able to:

1. **Trace** the historical evolution from mainframe computing to cloud computing, identifying the key innovations at each stage
2. **Explain** the fundamental distributed systems principles that underpin cloud architectures
3. **Apply** the CAP theorem to reason about trade-offs in distributed system design
4. **Distinguish** between consistency models and articulate their implications for application behavior
5. **Describe** consensus mechanisms and their role in distributed coordination
6. **Articulate** why cloud computing emerged as a synthesis of prior computing paradigms

---

## Table of Contents

1. [Introduction: Why Study Cloud Foundations?](#1-introduction-why-study-cloud-foundations)
2. [Historical Evolution of Computing Paradigms](#2-historical-evolution-of-computing-paradigms)
3. [Distributed Systems Fundamentals](#3-distributed-systems-fundamentals)
4. [The CAP Theorem](#4-the-cap-theorem)
5. [Consistency Models](#5-consistency-models)
6. [Consensus and Coordination](#6-consensus-and-coordination)
7. [Failure Modes and Fault Tolerance](#7-failure-modes-and-fault-tolerance)
8. [Why Cloud Computing?](#8-why-cloud-computing)
9. [Connecting to the Course](#9-connecting-to-the-course)
10. [Key Takeaways](#10-key-takeaways)
11. [References and Further Reading](#11-references-and-further-reading)
12. [Glossary](#12-glossary)

---

## 1. Introduction: Why Study Cloud Foundations?

### 1.1 The Central Question

This course begins with a fundamental question: **Why does cloud computing exist?**

The superficial answer—"to provide computing over the internet"—misses the deeper significance. Cloud computing represents a specific solution to problems that prior computing paradigms could not adequately address:

- The need for **elastic resource allocation** that responds to demand in real-time
- The **economic inefficiency** of owned infrastructure that sits idle most of the time
- The **operational complexity** of managing distributed systems at scale
- The desire to **convert capital expenditure to operational expenditure**

Understanding these motivations is essential because they inform every design decision in cloud systems. When you understand *why* cloud services are built the way they are, you can make better decisions about *how* to use them.

### 1.2 The Black Box Problem

Many cloud computing courses teach students to use cloud services as "black boxes"—click this button to launch a virtual machine, use this API to store data. While this approach gets results quickly, it creates practitioners who:

- **Cannot predict system behavior** under unusual conditions
- **Cannot debug distributed failures** when things go wrong
- **Cannot make informed trade-offs** between competing options
- **Cannot design resilient applications** that handle failure gracefully

This course takes a different approach. We study the principles underlying cloud systems so that you can reason about their behavior from first principles. This understanding transforms you from a cloud *user* into a cloud *practitioner*.

### 1.3 The Foundations Approach

The concepts introduced in this first lecture—distributed systems principles, the CAP theorem, consistency models, consensus mechanisms—will recur throughout the course:

- When we study **container orchestration**, we will see how Kubernetes uses etcd (a consensus-based key-value store) to maintain cluster state
- When we examine **cloud storage**, we will apply CAP theorem reasoning to understand why different storage services make different trade-offs
- When we design **cloud-native applications**, we will leverage our understanding of eventual consistency and distributed coordination

By establishing these foundations now, we create a conceptual framework that makes everything else in the course more comprehensible.

---

## 2. Historical Evolution of Computing Paradigms

Cloud computing did not emerge in a vacuum. It represents the culmination of decades of evolution in how we think about and organize computing resources. Understanding this history illuminates why cloud computing takes its current form.

### 2.1 The Mainframe Era (1950s-1970s)

#### Characteristics

The earliest electronic computers were **mainframes**—large, expensive machines that occupied entire rooms and required specialized staff to operate. Key characteristics included:

- **Centralized computing**: All processing occurred on a single, central machine
- **Time-sharing**: Multiple users shared the computer's resources, with the operating system rapidly switching between tasks to create the illusion of simultaneous access
- **Terminal access**: Users interacted via "dumb terminals"—devices with a keyboard and display but no local processing capability
- **Batch processing**: Jobs were submitted and queued for execution, with results returned later
- **High cost**: Mainframes were extremely expensive, creating strong economic pressure to maximize utilization

#### Relevance to Cloud Computing

The mainframe model anticipated several concepts central to cloud computing:

| Mainframe Concept | Cloud Equivalent |
|-------------------|------------------|
| Time-sharing | Multi-tenancy (multiple customers sharing infrastructure) |
| Centralized resources | Resource pooling in data centers |
| Terminal access | Thin clients, web browsers |
| Batch processing | Serverless functions, batch computing services |
| Maximizing utilization | Cloud provider efficiency through statistical multiplexing |

The economic logic is remarkably similar: expensive resources should be shared to maximize utilization and minimize per-user cost.

### 2.2 The Personal Computing Era (1980s-1990s)

#### The Decentralization Revolution

The development of microprocessors made it economically feasible to give each user their own computer. This **personal computing** revolution fundamentally changed the computing landscape:

- **Decentralized processing**: Each user had dedicated computing power
- **Local software**: Applications were installed and run on individual machines
- **Local storage**: Data resided on local hard drives
- **Individual ownership**: Users (or their organizations) owned their hardware

#### Problems Introduced

While personal computing democratized access to computing, it introduced new problems:

**Inconsistent environments**: Software that worked on one machine might fail on another due to different configurations, operating system versions, or installed libraries. The phrase "it works on my machine" became a common refrain.

**Underutilization**: Desktop computers typically use less than 5% of their CPU capacity on average. Organizations purchased computing power for peak demand that sat idle most of the time.

**Collaboration difficulties**: Sharing data between users required physical media (floppy disks, later CDs) or complex network configurations.

**Maintenance burden**: Each machine required individual maintenance—software updates, security patches, hardware repairs—creating significant IT overhead.

**Data vulnerability**: Data stored only on local drives was vulnerable to hardware failure, theft, or user error.

### 2.3 Client-Server Computing (1990s)

#### The Network Changes Everything

The proliferation of local area networks (LANs) and the emergence of the internet enabled a new model: **client-server computing**. In this model:

- **Servers** provide services (file storage, email, databases, web pages)
- **Clients** consume services (desktop applications, web browsers)
- **Networks** connect clients to servers

This model combined some benefits of both mainframes (centralized services, shared resources) and personal computers (local processing power, user interface responsiveness).

#### Key Developments

**Three-tier architecture** emerged as a common pattern:
1. **Presentation tier**: User interface (client)
2. **Application tier**: Business logic (application server)
3. **Data tier**: Persistent storage (database server)

This separation of concerns allowed each tier to be developed, scaled, and maintained independently.

### 2.4 Cluster Computing (Late 1990s-2000s)

#### Beyond Single Servers

As demand for computing power grew, single servers became insufficient. **Cluster computing** addressed this by connecting multiple commodity machines to work together:

- **Horizontal scaling**: Add more machines rather than buying bigger machines
- **High-speed interconnects**: Fast local networks (often specialized hardware) connected cluster nodes
- **Parallel processing**: Work was divided among nodes and executed simultaneously
- **Homogeneous hardware**: Clusters typically used identical machines for simplicity

#### Programming Models

Cluster computing required new programming models to express parallel computation:

- **Message Passing Interface (MPI)**: Explicit communication between processes on different nodes
- **MapReduce**: A simplified model for processing large datasets (Google, 2004), later implemented as Hadoop

#### Limitations

Cluster computing solved the scale problem but introduced new challenges:

- **Expertise required**: Setting up and managing clusters required specialized knowledge
- **Fixed capacity**: Clusters had fixed size; scaling required purchasing and installing new hardware
- **Single organization**: Clusters served one organization and couldn't easily share resources

### 2.5 Grid Computing (2000s)

#### Federating Distributed Resources

**Grid computing** extended the cluster concept across organizational and geographic boundaries:

- **Wide-area distribution**: Resources spread across multiple sites, connected via the internet
- **Heterogeneous resources**: Different sites contributed different types of hardware
- **Multi-organizational**: Resources owned by different organizations were federated
- **Virtual organizations**: Users from multiple institutions collaborated on shared projects

#### Grid Middleware

Projects like Globus provided middleware to manage grid resources:

- **Resource discovery**: Finding available resources across the grid
- **Authentication and authorization**: Managing identity across organizations
- **Job submission and scheduling**: Running work on remote resources
- **Data management**: Moving data between sites

#### Scientific Applications

Grid computing found its primary application in scientific research:

- **CERN's Large Hadron Collider** uses the Worldwide LHC Computing Grid (WLCG) to analyze particle physics data
- **SETI@home** distributed the search for extraterrestrial intelligence across volunteers' personal computers
- **Climate modeling** and **genomics** research leveraged grid resources

#### Why Grid Computing Didn't Become Cloud Computing

Despite its ambitions, grid computing remained largely confined to scientific applications. Several factors limited broader adoption:

- **Complexity**: Grid middleware was difficult to install, configure, and use
- **Reliability**: Resources came and went unpredictably
- **Performance**: Wide-area networks introduced significant latency
- **Economic model**: No clear pricing or business model for commercial use

### 2.6 Utility Computing

#### Computing as a Utility

The concept of **utility computing** proposed treating computing like electricity or water—a metered service available on demand:

- **Pay-per-use pricing**: Customers pay only for resources consumed
- **On-demand provisioning**: Resources available when needed, released when not
- **Service-level agreements**: Providers guarantee specific performance and availability
- **Abstracted infrastructure**: Customers don't need to know or care about underlying hardware

#### Early Attempts

Several companies attempted utility computing before cloud computing emerged:

- **Sun Microsystems' Sun Grid** (2006): Computing time at $1 per CPU-hour
- **IBM's e-business on demand** (2002): Flexible IT infrastructure
- **HP's Adaptive Infrastructure** (2003): Dynamic resource allocation

These efforts were ahead of their time and achieved limited success, but they established the economic model that cloud computing would later perfect.

### 2.7 The Cloud Computing Synthesis (2006-Present)

#### The Breakthrough: Amazon Web Services

In 2006, Amazon Web Services (AWS) launched Amazon Elastic Compute Cloud (EC2), and cloud computing as we know it began. AWS succeeded where earlier efforts had struggled because it combined:

- **Technical capabilities from grid computing**: Distributed resources, virtualization, APIs
- **Economic model from utility computing**: Pay-per-use, no upfront commitment
- **Self-service provisioning**: Users could provision resources via web interface or API without human intervention
- **Massive scale**: Amazon's infrastructure, built for e-commerce, provided economies of scale

#### Key Milestones

| Year | Milestone |
|------|-----------|
| 2006 | AWS launches S3 (storage) and EC2 (compute) |
| 2008 | Google App Engine introduces Platform-as-a-Service |
| 2009 | Heroku simplifies application deployment |
| 2010 | Microsoft Azure reaches general availability |
| 2010 | OpenStack project founded for open-source cloud infrastructure |
| 2013 | Docker popularizes containerization |
| 2014 | Google releases Kubernetes for container orchestration |
| 2014 | AWS Lambda introduces serverless computing |
| 2017 | Kubernetes becomes dominant orchestration platform |
| 2020s | Multi-cloud strategies, edge computing, AI/ML services mature |

#### What Cloud Computing Inherited

Cloud computing synthesized lessons from all preceding paradigms:

| Source | Contribution to Cloud |
|--------|----------------------|
| **Mainframes** | Centralized management, multi-tenancy, resource pooling, batch processing |
| **Personal computing** | User empowerment, self-service, web-based interfaces |
| **Client-server** | Service-oriented architecture, API-driven interaction |
| **Clusters** | Horizontal scaling, commodity hardware, distributed storage |
| **Grids** | Resource federation, virtualization, heterogeneous resources |
| **Utility computing** | Pay-per-use economics, on-demand provisioning, SLAs |

---

## 3. Distributed Systems Fundamentals

Cloud computing is, at its core, **distributed computing at scale**. To understand cloud systems, we must understand the fundamental principles that govern distributed systems.

### 3.1 Definition and Characteristics

#### What is a Distributed System?

The canonical definition comes from Tanenbaum and Van Steen:

> **A distributed system is a collection of autonomous computing elements that appears to users as a single coherent system.**

Let's unpack this definition:

- **Autonomous computing elements**: Independent computers (nodes) that can operate without central control
- **Collection**: Multiple nodes working together
- **Appears to users as a single system**: The distribution is (ideally) transparent to users

#### Fundamental Properties

Distributed systems exhibit three fundamental properties that distinguish them from single-machine systems:

**1. Concurrency**

Multiple components execute simultaneously. This creates opportunities for parallelism but also introduces race conditions, deadlocks, and coordination challenges. Unlike single-threaded programs where statements execute in a deterministic order, distributed systems have multiple threads of execution that interleave in unpredictable ways.

**2. No Global Clock**

There is no perfectly synchronized clock across all nodes. Each node has its own local clock, and these clocks drift relative to each other. This makes it impossible to definitively order events across nodes based on their timestamps alone. The implications are profound:

- You cannot rely on timestamps to determine which event happened first
- "Simultaneous" events may be ordered differently by different observers
- Coordination requires explicit protocols, not just timestamp comparison

**3. Independent Failures**

Components can fail independently. Node A might crash while nodes B and C continue operating. The network connecting them might fail partially—A can reach B but not C. This **partial failure** is fundamentally different from single-machine failure where the entire system either works or doesn't.

### 3.2 Why Distribute?

Given the complexity that distribution introduces, why do we build distributed systems at all? The answer lies in the benefits that distribution provides:

#### Scalability

Distributed systems can handle increasing load by adding more nodes. This **horizontal scaling** (adding machines) is often more cost-effective and practical than **vertical scaling** (buying bigger machines). There are physical limits to how powerful a single machine can be, but there's no theoretical limit to how many machines can work together.

#### Fault Tolerance

If a system runs on a single machine, that machine is a **single point of failure**—if it fails, the entire system fails. Distributed systems can be designed to survive component failures. If one node fails, others can take over its work.

#### Geographic Distribution

Users are geographically distributed. A system with servers in multiple locations can serve users from nearby servers, reducing latency. This is essential for global services where the speed of light imposes fundamental limits on communication time.

#### Resource Sharing

Multiple users or applications can share a pool of resources more efficiently than if each had dedicated resources. Statistical multiplexing means that resources idle for one user can serve another.

### 3.3 The Challenges of Distribution

The benefits of distribution come with significant challenges:

#### Partial Failure

In a distributed system, some components can fail while others continue operating. This creates scenarios that don't exist in single-machine systems:

- A request might be processed but the acknowledgment lost
- A node might be slow, not failed—but how do you tell the difference?
- Different parts of the system might have inconsistent views of the current state

Designing for partial failure requires fundamentally different approaches than designing for all-or-nothing failure.

#### Network Unreliability

The network connecting distributed components is itself unreliable:

- **Messages can be lost**: Packets may be dropped and never arrive
- **Messages can be delayed**: Network congestion can cause variable, unpredictable latency
- **Messages can be duplicated**: Retransmission mechanisms can cause the same message to arrive multiple times
- **Messages can be reordered**: Different packets may take different paths and arrive out of order

#### Coordination Complexity

Getting distributed components to agree on anything—the current state of data, who is the leader, whether a transaction should commit—requires explicit coordination protocols. These protocols add latency, reduce availability, and introduce their own failure modes.

#### Debugging Difficulty

When something goes wrong in a distributed system, understanding what happened is extraordinarily difficult:

- Events are spread across multiple nodes
- Logs have different timestamps that may not be synchronized
- The order of events may be ambiguous
- Failures may be transient and non-reproducible

### 3.4 The Eight Fallacies of Distributed Computing

In the 1990s, engineers at Sun Microsystems (primarily Peter Deutsch, with additions by James Gosling) identified eight assumptions that developers often incorrectly make about distributed systems. These **fallacies** remain relevant today:

#### Fallacy 1: The Network is Reliable

**Reality**: Networks fail. Cables are cut. Routers crash. Configuration errors partition networks. Distributed systems must be designed to handle network failures gracefully.

**Implication**: Implement retries with backoff. Use timeouts. Design for idempotency so that retried operations are safe.

#### Fallacy 2: Latency is Zero

**Reality**: Every network communication takes time. Even at the speed of light, a round trip from New York to London takes about 56 milliseconds. Add routing, processing, and queuing delays, and real-world latencies are much higher.

**Implication**: Minimize network round trips. Batch operations when possible. Consider latency in user experience design.

#### Fallacy 3: Bandwidth is Infinite

**Reality**: Bandwidth is limited and often shared. Sending large amounts of data takes time and can congest the network, affecting other traffic.

**Implication**: Compress data. Send only what's needed. Consider the network cost of design decisions.

#### Fallacy 4: The Network is Secure

**Reality**: Networks can be eavesdropped, messages can be tampered with, identities can be spoofed. The internet was not designed with security as a primary concern.

**Implication**: Encrypt sensitive data in transit. Authenticate communication partners. Don't trust data received from the network without validation.

#### Fallacy 5: Topology Doesn't Change

**Reality**: Network topology changes constantly. Nodes are added and removed. Routes change. IP addresses are reassigned. Cloud instances come and go.

**Implication**: Don't hardcode network addresses. Use service discovery. Design for dynamic environments.

#### Fallacy 6: There is One Administrator

**Reality**: Different parts of a distributed system may be administered by different people or organizations with different priorities, policies, and schedules.

**Implication**: Don't assume coordinated maintenance windows. Design for components being upgraded independently. Handle version skew.

#### Fallacy 7: Transport Cost is Zero

**Reality**: Sending data across networks has costs—bandwidth charges, processing overhead for serialization/deserialization, memory for buffers.

**Implication**: Consider the cost of communication in system design. Sometimes local processing is cheaper than remote calls.

#### Fallacy 8: The Network is Homogeneous

**Reality**: Networks use different technologies, protocols, and implementations. Interoperability is not guaranteed.

**Implication**: Use standard protocols. Test across different network conditions. Don't assume consistent behavior.

### 3.5 Implications for Cloud Systems

Every cloud service is a distributed system, and every application deployed in the cloud becomes part of a distributed system. The fallacies apply to:

- Communication between your application and cloud services
- Communication between microservices in your application
- Communication between cloud provider components
- Communication between data centers and availability zones

Designing cloud applications requires internalizing these realities and designing accordingly.

---

## 4. The CAP Theorem

The CAP theorem, formulated by Eric Brewer in 2000 and formally proven by Seth Gilbert and Nancy Lynch in 2002, is one of the most important theoretical results in distributed systems. It establishes fundamental limits on what distributed systems can achieve.

### 4.1 The Three Properties

The CAP theorem concerns three desirable properties of distributed systems:

#### Consistency (C)

**Definition**: Every read receives the most recent write or an error.

In a consistent system, all nodes see the same data at the same time. If a write is acknowledged, any subsequent read (from any node) will return that written value. This is also called **linearizability** or **strong consistency**.

**Example**: In a strongly consistent banking system, if you deposit $100, any subsequent balance check (even from a different ATM) will show the deposit.

**What it is NOT**: Consistency in CAP is not the same as the "C" in ACID (database transactions). ACID consistency refers to maintaining database invariants (like foreign key constraints). CAP consistency refers to all nodes having the same data.

#### Availability (A)

**Definition**: Every request receives a non-error response, without guarantee that it contains the most recent write.

An available system continues to respond to requests. It doesn't return errors or time out (except for truly broken requests). However, the response might be based on stale data.

**Example**: In a highly available DNS system, you always get an answer to a DNS query, even if the answer is outdated because a recent change hasn't propagated yet.

**What it is NOT**: Availability in CAP means every request gets a response. It doesn't mean the system is "up" in the colloquial sense. A system could be "up" but return errors, which would not be available in the CAP sense.

#### Partition Tolerance (P)

**Definition**: The system continues to operate despite network partitions.

A **network partition** occurs when the network fails in a way that divides nodes into groups that cannot communicate with each other. Partition tolerance means the system doesn't simply give up when this happens.

**Example**: If a network failure separates a data center in New York from one in London, a partition-tolerant system continues to function (perhaps with degraded capabilities) rather than completely failing.

**Why it's usually required**: In any distributed system that spans multiple machines, network partitions can occur. Choosing to not tolerate partitions means the system fails completely when partitions occur, which is usually unacceptable.

### 4.2 The Theorem Stated

The CAP theorem states:

> **In the presence of a network partition, a distributed system cannot simultaneously provide both consistency and availability. It must choose one or the other.**

More formally: it is impossible for a distributed data store to simultaneously provide more than two of the following three guarantees: Consistency, Availability, and Partition Tolerance.

### 4.3 Understanding the Trade-off

Since network partitions can and do occur in distributed systems, we effectively must choose partition tolerance. This leaves us with a choice:

#### CP Systems: Consistency + Partition Tolerance

When a partition occurs, a CP system sacrifices availability to maintain consistency. It may refuse to respond to requests if it cannot guarantee the response is consistent.

**Behavior during partition**: Some or all requests receive errors (or timeout) rather than potentially stale data.

**Use when**: Correctness is more important than availability. You'd rather have no answer than a wrong answer.

**Examples**:
- **Banking systems**: An incorrect balance could lead to overdrafts or lost money
- **Inventory systems**: Selling items that don't exist causes customer service nightmares
- **Coordination services** (ZooKeeper, etcd): Configuration data must be consistent

#### AP Systems: Availability + Partition Tolerance

When a partition occurs, an AP system sacrifices consistency to maintain availability. It continues to respond to all requests, even if responses might be based on stale data.

**Behavior during partition**: All requests receive responses, but responses may not reflect the most recent writes.

**Use when**: Availability is more important than consistency. A stale answer is better than no answer.

**Examples**:
- **Shopping carts**: Better to let customers keep shopping than to show errors
- **Social media**: A slightly incorrect like count doesn't cause real harm
- **DNS**: The internet should keep working even if DNS updates take time to propagate
- **Content delivery networks**: Serving slightly stale content is better than serving nothing

### 4.4 The Trade-off Visualized

Consider a simple distributed system with two nodes (A and B) that should have the same data:

```
Normal operation:
[Node A] <--network--> [Node B]
  x=5                    x=5
  
Client writes x=7 to Node A:
[Node A] <--network--> [Node B]
  x=7                    x=5
  (must sync)
  
During partition (network broken):
[Node A]     X     [Node B]
  x=7                x=5
  (cannot sync)
```

Now a client reads from Node B. What happens?

- **CP choice**: Node B refuses to respond (or returns an error) because it cannot verify its data is current
- **AP choice**: Node B returns x=5, which is stale but available

### 4.5 CAP is a Spectrum, Not a Binary Choice

Real systems are more nuanced than "CP or AP":

#### Different Operations, Different Trade-offs

A system might make different CAP trade-offs for different operations:
- **Reads**: Might allow stale data (AP behavior)
- **Writes**: Might require consistency (CP behavior)
- **Critical data**: Consistent
- **Non-critical data**: Available

#### Tunable Consistency

Many modern databases let you choose consistency levels per-operation:
- **Amazon DynamoDB**: Strongly consistent reads available at higher cost/latency
- **Apache Cassandra**: Configurable consistency levels (ONE, QUORUM, ALL)
- **Azure Cosmos DB**: Five consistency levels from strong to eventual

#### Partitions Are Usually Rare

The CAP theorem only forces a choice during partitions. Most of the time, networks work correctly, and systems can provide both consistency and availability. The question is what to do during the (hopefully rare) partitions.

### 4.6 PACELC: Extending CAP

Daniel Abadi proposed the PACELC theorem as an extension to CAP that addresses behavior during normal operation:

> **If there is a Partition (P), choose between Availability (A) and Consistency (C); Else (E), when the system is running normally, choose between Latency (L) and Consistency (C).**

This recognizes that even without partitions, there's a trade-off between consistency and latency. Ensuring consistency requires coordination between nodes, which takes time. Systems can be classified by their choices in both scenarios:

| Classification | During Partition | Normal Operation | Example |
|----------------|------------------|------------------|---------|
| PA/EL | Availability | Latency | DynamoDB, Cassandra |
| PC/EC | Consistency | Consistency | Traditional RDBMS, Spanner |
| PA/EC | Availability | Consistency | PNUTS (Yahoo) |
| PC/EL | Consistency | Latency | (rare) |

### 4.7 Practical Implications

When designing or choosing distributed systems, consider:

1. **What are the consequences of inconsistency?** If showing stale data causes real harm (financial, safety, etc.), lean toward CP.

2. **What are the consequences of unavailability?** If users cannot tolerate errors or downtime, lean toward AP.

3. **How often do partitions occur?** In well-managed systems, partitions are rare but do happen.

4. **Can you detect and recover from inconsistencies?** Some systems accept temporary inconsistency and reconcile later.

5. **Can you provide different guarantees for different data?** Not all data has the same requirements.

---

## 5. Consistency Models

While the CAP theorem presents a binary choice between consistency and availability, real systems implement various **consistency models** that offer different guarantees. Understanding these models helps you choose appropriate systems and design applications that work correctly with them.

### 5.1 The Consistency Spectrum

Consistency models form a spectrum from strongest (easiest to program against, but expensive) to weakest (harder to reason about, but performant):

```
Strongest                                                    Weakest
    |                                                            |
    v                                                            v
Linearizable > Sequential > Causal > Read-your-writes > Monotonic > Eventual
```

### 5.2 Strong Consistency (Linearizability)

#### Definition

**Linearizability** provides the illusion that there is only a single copy of the data, and all operations are atomic. After a write completes, all subsequent reads return that value.

#### Properties

- Operations appear to execute atomically at some instant between their start and end times
- All clients see operations in the same order
- That order is consistent with real-time ordering (if A completes before B starts, A is ordered before B)

#### Implementation

Strong consistency typically requires coordination between nodes, often using consensus protocols like Paxos or Raft. Before a write is acknowledged, a majority of nodes must agree.

#### Trade-offs

- **Latency**: Coordination adds round trips between nodes
- **Availability**: During partitions, may be unavailable
- **Throughput**: Coordination limits parallelism

#### When to Use

- Financial transactions where incorrect data causes monetary loss
- Inventory systems where overselling is costly
- Any system where "eventually" correct isn't good enough

#### Example: Bank Transfer

```
Account A: $100    Account B: $50

Transfer $30 from A to B:
1. Read A: $100
2. Read B: $50  
3. Write A: $70
4. Write B: $80

With strong consistency: Any reader sees either (100,50) or (70,80), never (70,50) or (100,80)
```

### 5.3 Sequential Consistency

#### Definition

All operations appear to execute in some sequential order, and each client's operations appear in the order they were issued. However, the global order doesn't need to respect real-time ordering.

#### Difference from Linearizability

If client 1 completes a write before client 2 starts a read, linearizability requires client 2 to see the write. Sequential consistency doesn't—it only requires that each client's operations are ordered correctly relative to each other.

#### When to Use

- Simpler to implement than linearizability
- Useful when clients primarily interact with their own data

### 5.4 Causal Consistency

#### Definition

Operations that are **causally related** are seen by all nodes in the same order. Operations that are not causally related (concurrent) may be seen in different orders by different nodes.

#### What is Causality?

Two operations are causally related if:
- They are by the same client (a client's operations are always causally ordered)
- One reads data that the other wrote
- There is a chain of such relationships

#### Example: Social Media

```
Alice posts: "I got the job!"           (Event A)
Bob sees Alice's post and comments: "Congratulations!"  (Event B)
```

Events A and B are causally related (B happened because of A). Causal consistency ensures that anyone who sees B also sees A. Without causal consistency, Carol might see "Congratulations!" without seeing what it's congratulating.

#### When to Use

- Collaborative applications where actions have cause-and-effect relationships
- Social media feeds
- Messaging systems

### 5.5 Read-Your-Writes Consistency

#### Definition

A client always sees its own writes. If I write a value, my subsequent reads will return that value (or a later value).

#### What It Doesn't Guarantee

Other clients might not see my writes immediately. The guarantee is only for the client that performed the write.

#### Session Consistency

Often implemented as **session consistency**—within a session, you see your own writes. Different sessions might see different data.

#### When to Use

- User profile updates (I should see my own changes)
- Shopping carts (items I add should appear)
- Any application where users expect to see their own actions reflected immediately

#### Implementation

Can be implemented by:
- Routing all of a client's requests to the same replica
- Including version information in requests
- Read-after-write within a data center

### 5.6 Monotonic Reads

#### Definition

If a client reads a value, subsequent reads will never return an older value. Data doesn't go "backward."

#### Without Monotonic Reads

```
Time 1: Client reads x=5 from Replica A
Time 2: Client reads x=3 from Replica B (which hasn't received the update yet)
```

The client sees the value go backward, which is confusing and can cause bugs.

#### When to Use

- Any application where users expect progress, not regression
- Feeds, timelines, logs

### 5.7 Eventual Consistency

#### Definition

If no new updates are made, eventually all reads will return the same value. The system will converge to a consistent state over time.

#### Properties

- Updates propagate asynchronously
- Different clients may see different values at the same time
- No bound on how long convergence takes (though practical systems usually converge quickly)

#### The Inconsistency Window

The time between a write and when all replicas have that write is called the **inconsistency window** or **replication lag**. During this window, different clients may see different values depending on which replica they read from.

```
Timeline:
t=0: Write x=5 to Node A
t=1: Node A sends update to Node B (in transit)
t=2: Read from Node A returns 5, read from Node B returns old value
t=3: Node B receives update
t=4: Read from either node returns 5

Inconsistency window: t=0 to t=3
```

#### Conflict Resolution

With eventual consistency, concurrent updates to the same data can create conflicts. Systems must decide how to resolve them:

- **Last-writer-wins (LWW)**: Use timestamps; latest timestamp wins. Simple but can lose updates.
- **Vector clocks**: Track causality; detect conflicts. More complex but preserves concurrent updates.
- **Application-level resolution**: Return all conflicting values; let the application decide.
- **CRDTs (Conflict-free Replicated Data Types)**: Data structures designed to merge automatically.

#### When to Use

- High availability is critical
- Temporary inconsistency is acceptable
- Data can be merged or conflicts are rare

#### Examples in Practice

**Amazon S3**: Provides strong read-after-write consistency for new objects (as of 2020), but eventual consistency for overwrites and deletes.

**DNS**: Propagation of DNS changes can take up to 48 hours. During this time, different clients may get different IP addresses for the same domain.

**Social Media Counters**: Like counts and view counts are often eventually consistent. The exact number at any moment doesn't matter as much as the trend.

### 5.8 Choosing a Consistency Model

The right consistency model depends on your requirements:

| Requirement | Recommended Model |
|-------------|-------------------|
| Financial transactions | Strong (Linearizable) |
| User sees their own updates | Read-your-writes |
| Collaborative editing | Causal |
| High-volume analytics | Eventual |
| Global scale with low latency | Eventual or Causal |

Many applications use different consistency models for different data:
- User account data: Strong consistency
- User preferences: Read-your-writes
- Analytics data: Eventual consistency
- Cached content: Eventual consistency with TTL

---

## 6. Consensus and Coordination

When distributed systems require multiple nodes to agree on something—who is the leader, whether a transaction should commit, what the current configuration is—they need **consensus** protocols.

### 6.1 The Consensus Problem

#### Formal Definition

The consensus problem requires a group of processes to agree on a single value. Formally, a consensus protocol must satisfy:

- **Agreement**: All correct (non-faulty) processes decide the same value
- **Validity**: The decided value was proposed by some process (no making up values)
- **Termination**: All correct processes eventually decide

#### Why Consensus is Hard

Consensus seems simple—just have everyone agree! But in a distributed system:

- Messages can be lost or delayed
- Nodes can crash at any time
- You can't tell if a node has crashed or is just slow
- Different nodes may receive messages in different orders

### 6.2 The FLP Impossibility Result

In 1985, Fischer, Lynch, and Paterson proved a remarkable result:

> **In an asynchronous distributed system (no timing assumptions), no consensus protocol can guarantee all three properties (agreement, validity, termination) if even a single process can fail.**

This is the **FLP impossibility result**. It says that consensus is impossible to solve perfectly in an asynchronous system with failures.

#### Implications

The FLP result doesn't mean consensus is useless—practical systems use consensus all the time. It means that practical protocols must make compromises:

- **Timing assumptions**: Assume partial synchrony (messages eventually arrive within some bound)
- **Probabilistic guarantees**: Guarantee termination with high probability, not certainty
- **Failure detectors**: Use timeouts to suspect failures (might be wrong sometimes)

### 6.3 Paxos

#### Overview

**Paxos**, developed by Leslie Lamport in 1989 (published 1998), was the first practical consensus protocol. It proves that consensus is achievable if a majority of nodes are functioning.

#### Basic Idea

Paxos works in rounds. Each round has:
1. **Prepare phase**: A proposer asks acceptors to promise not to accept older proposals
2. **Accept phase**: If a majority promise, the proposer asks them to accept a value
3. **Learn phase**: Once a majority accept, the value is decided

#### Why It Works

- A value is decided when a majority accepts it
- Any two majorities overlap by at least one node
- That overlapping node prevents conflicting values from being decided

#### The Paxos Reputation

Paxos is notoriously difficult to understand and implement correctly. Lamport's original paper used a fictional Greek parliament as a metaphor, which some found confusing. Many implementations have had bugs.

### 6.4 Raft

#### Motivation

**Raft** was designed by Diego Ongaro and John Ousterhout in 2014 specifically to be understandable. Their paper is titled "In Search of an Understandable Consensus Algorithm."

#### Key Design Decisions

Raft simplifies consensus by decomposing it into relatively independent subproblems:

1. **Leader election**: Electing a distinguished leader
2. **Log replication**: The leader accepts commands and replicates them to followers
3. **Safety**: Ensuring only nodes with up-to-date logs can become leader

#### How Raft Works

**Normal operation**:
1. One node is the leader; others are followers
2. Clients send all requests to the leader
3. The leader appends the request to its log and sends it to followers
4. Once a majority of followers acknowledge, the entry is committed
5. The leader notifies followers to commit and responds to the client

**Leader election**:
1. Followers expect periodic heartbeats from the leader
2. If no heartbeat arrives (timeout), a follower becomes a candidate
3. The candidate requests votes from other nodes
4. If it gets a majority of votes, it becomes the new leader
5. If no one gets a majority, a new election starts with randomized timeouts

#### Raft in Practice

Raft is implemented in many production systems:
- **etcd**: The key-value store used by Kubernetes
- **Consul**: HashiCorp's service discovery and configuration system
- **CockroachDB**: Distributed SQL database
- **TiKV**: Distributed key-value database

### 6.5 Quorums

#### The Majority Principle

Both Paxos and Raft rely on **quorums**—typically majorities. This is because any two majorities must overlap:

```
5 nodes: {A, B, C, D, E}
Majority 1: {A, B, C}
Majority 2: {C, D, E}
Overlap: {C}
```

The overlapping node ensures that information from one operation is available to the next.

#### Quorum Math

For a cluster of N nodes:
- **Majority (quorum) size**: ⌊N/2⌋ + 1
- **Max failures tolerated**: ⌊(N-1)/2⌋

| Nodes (N) | Quorum | Max Failures |
|-----------|--------|--------------|
| 1 | 1 | 0 |
| 2 | 2 | 0 |
| 3 | 2 | 1 |
| 4 | 3 | 1 |
| 5 | 3 | 2 |
| 6 | 4 | 2 |
| 7 | 4 | 3 |

Notice that even-numbered clusters don't tolerate more failures than the odd number below them (4 nodes tolerates 1 failure, same as 3 nodes). This is why clusters typically use odd numbers of nodes.

#### Read and Write Quorums

For data replication (not just consensus), systems can use separate read and write quorums:

- **N**: Total number of replicas
- **W**: Write quorum (writes must reach W replicas)
- **R**: Read quorum (reads must check R replicas)

If **R + W > N**, reads and writes overlap, so reads see the latest writes.

**Example** (N=5):
- W=3, R=3: Strong consistency, tolerates 2 failures for reads or writes
- W=1, R=5: Fast writes, slow reads, strong consistency
- W=5, R=1: Slow writes, fast reads, strong consistency
- W=2, R=2: Fast operations, but no consistency guarantee (no overlap)

### 6.6 Consensus in Cloud Systems

#### Kubernetes and etcd

Kubernetes uses **etcd** to store all cluster state—what pods should be running, where, with what configuration. etcd uses Raft consensus to ensure this data is consistent and available.

**Implications for cluster design**:
- A 3-node etcd cluster tolerates 1 node failure
- A 5-node etcd cluster tolerates 2 node failures
- If a majority of etcd nodes fail, Kubernetes cannot schedule new pods or respond to changes

#### Cloud Provider Services

Cloud providers use consensus internally for many services:
- **AWS DynamoDB**: Uses Paxos-based replication within storage nodes
- **Google Spanner**: Uses Paxos for global consistency
- **Azure Cosmos DB**: Uses a custom consensus protocol

Users typically don't interact with these protocols directly but benefit from the consistency they provide.

---

## 7. Failure Modes and Fault Tolerance

Distributed systems must handle failures gracefully. Understanding failure modes is essential for designing resilient cloud applications.

### 7.1 Types of Failures

#### Crash Failures

The simplest failure mode: a node stops executing and never recovers (or recovers much later). The node doesn't do anything wrong—it just stops.

**Detection**: Typically via heartbeat timeouts. If a node doesn't respond within expected time, assume it has crashed.

**Handling**: Remove the failed node from the active set; redirect its work to other nodes.

#### Omission Failures

A node fails to send or receive some messages but otherwise operates correctly. This could be due to:
- Network issues affecting specific connections
- Full message queues causing drops
- Bugs that cause some messages to be ignored

**Detection**: Harder than crash failures. Might notice missing responses or inconsistent state.

**Handling**: Retries, redundant communication paths.

#### Timing Failures

A node responds, but not within the expected time bound. In a real-time system, a late response may be as bad as no response.

**Detection**: Timeouts, but distinguishing "slow" from "crashed" is difficult.

**Handling**: Adaptive timeouts, deadline-aware processing.

#### Byzantine Failures

A node behaves arbitrarily—it might send wrong data, send different data to different nodes, or actively try to subvert the system. This includes:
- Bugs that cause incorrect computation
- Hardware errors that corrupt data
- Malicious actors who have compromised a node

**Detection**: Very difficult. Requires comparing information from multiple sources.

**Handling**: Byzantine fault-tolerant (BFT) protocols, which require 3f+1 nodes to tolerate f Byzantine failures. Expensive, rarely used outside of blockchain systems and critical infrastructure.

#### Failure Model Hierarchy

```
Byzantine ⊃ Timing ⊃ Omission ⊃ Crash
```

Each failure mode includes all simpler modes. Byzantine is the most general (and hardest to handle). Most distributed systems assume only crash failures, trusting that nodes either work correctly or stop entirely.

### 7.2 Failure Detection

Detecting failures in a distributed system is fundamentally challenging due to the impossibility of distinguishing between a crashed node and a slow network.

#### Heartbeats

The most common approach:
1. Nodes periodically send "I'm alive" messages
2. If no heartbeat is received within timeout, suspect failure
3. Multiple missed heartbeats trigger failure declaration

**Challenge**: Choosing the timeout. Too short → false positives (declaring live nodes dead). Too long → slow detection of real failures.

#### Phi Accrual Failure Detector

Rather than a binary alive/dead decision, the **phi accrual failure detector** outputs a **suspicion level** based on observed heartbeat patterns:
- Tracks the statistical distribution of heartbeat arrival times
- Computes the probability that a node has failed given the time since the last heartbeat
- Higher phi values indicate higher suspicion

Used in Apache Cassandra and Akka.

#### Gossip Protocols

Nodes periodically exchange state with random peers:
- Each node maintains a list of other nodes and their last known state
- When nodes gossip, they share and merge their lists
- Failure information spreads through the gossip process

**Advantages**: Scalable, no single point of failure.
**Disadvantages**: Slower detection than direct heartbeats.

### 7.3 Redundancy Strategies

#### Replication

Maintain multiple copies of data or computation:

**Active replication** (state machine replication):
- All replicas receive all requests
- All replicas process all requests
- If one fails, others already have the result

**Passive replication** (primary-backup):
- One primary processes requests
- Primary sends state updates to backups
- If primary fails, a backup takes over

#### Geographic Redundancy

Cloud providers organize infrastructure into:

**Availability Zones (AZs)**: Physically separate data centers within a region, connected by low-latency links. Designed so that a failure in one AZ doesn't affect others.

**Regions**: Geographic areas (e.g., us-east-1, eu-west-1) containing multiple AZs. Regions are isolated from each other; a regional disaster affects only that region.

**Deployment patterns**:
- Single AZ: No redundancy (for development)
- Multi-AZ: Tolerates single AZ failure (production standard)
- Multi-region: Tolerates regional disasters (critical systems, global services)

### 7.4 Graceful Degradation

When failures occur, systems should degrade gracefully rather than failing completely:

**Circuit breakers**: If a downstream service is failing, stop calling it rather than waiting for timeouts. Periodically check if it has recovered.

**Fallbacks**: Return cached data, default values, or reduced functionality when full functionality isn't available.

**Load shedding**: When overloaded, reject some requests to maintain quality for others.

**Feature flags**: Disable non-essential features to reduce load during incidents.

---

## 8. Why Cloud Computing?

Having established the foundations of distributed systems, we can now articulate why cloud computing emerged and became the dominant computing paradigm.

### 8.1 The Problems Cloud Solves

#### Capital Expenditure to Operational Expenditure

**Traditional model**: To run applications, you must:
1. Estimate capacity needs for the next 3-5 years
2. Purchase hardware (servers, storage, networking)
3. Build or lease data center space
4. Wait weeks or months for equipment delivery and setup
5. Pay for all this upfront, whether you use it or not

**Cloud model**: To run applications, you:
1. Request resources via API or web console
2. Resources are available in minutes
3. Pay for what you use, by the hour or second
4. Release resources when no longer needed

This converts **capital expenditure (CapEx)** to **operational expenditure (OpEx)**:
- No large upfront investment
- No depreciation schedules
- No disposal of obsolete equipment
- Cost scales with business needs

#### Elastic Scaling

**Traditional model**: Capacity is fixed. If you experience unexpected success, you can't serve the additional users. If demand is lower than expected, you've paid for unused capacity.

**Cloud model**: Scale resources up and down to match demand:
- **Scale out**: Add more instances when traffic increases
- **Scale in**: Remove instances when traffic decreases
- **Auto-scaling**: Automated adjustment based on metrics

This is **elasticity**—the ability to rapidly expand or contract resources.

#### Operational Complexity Offload

Running infrastructure requires expertise in:
- Hardware maintenance and replacement
- Operating system updates and security patches
- Network configuration and security
- Storage management and backup
- Physical security and access control
- Compliance and auditing

Cloud providers handle these concerns at scale, amortizing the cost across many customers. Smaller organizations get access to operational capabilities they couldn't afford independently.

#### Global Distribution

Building a global presence traditionally required:
- Data centers on multiple continents
- Complex international networking
- Local regulatory compliance
- 24/7 operations staff worldwide

Cloud providers have already made these investments. You can deploy applications globally by selecting regions in a console.

### 8.2 The Economic Model

#### Economies of Scale

Cloud providers benefit from massive scale:
- **Hardware**: Bulk purchasing discounts, custom hardware design
- **Power**: Direct contracts with utilities, investment in renewable energy
- **Networking**: Direct peering, private fiber networks
- **Operations**: Automation reduces per-server staff costs
- **Utilization**: Statistical multiplexing across many customers

These savings are passed to customers as lower prices.

#### Statistical Multiplexing

Not all customers need resources at the same time. By aggregating demand across many customers:
- Peak demands average out
- Providers need less total capacity than if each customer had dedicated resources
- Higher utilization reduces cost per unit

This is why cloud is often cheaper than dedicated infrastructure, even after including provider profit margins.

### 8.3 The Trade-offs

Cloud computing is not without disadvantages:

#### Reduced Control

You don't control the hardware, network, or underlying software. You must work within the provider's constraints:
- Available instance types
- Network topology
- Software versions
- Feature release schedules

#### Vendor Lock-in

Moving between cloud providers is costly:
- Proprietary services have no equivalent elsewhere
- Data transfer fees can be substantial
- Applications may need modification
- Operations teams must learn new tools

#### Data Sovereignty

Data may be stored in jurisdictions with different legal frameworks:
- GDPR requires data about EU residents to be handled carefully
- Some countries require certain data to remain within borders
- Government access to data may differ by location

#### Network Dependency

Cloud resources require network access:
- Internet outage means service outage
- Latency to cloud may exceed latency to local servers
- Bandwidth costs for data transfer

#### Cost at Scale

For large, stable workloads, cloud may be more expensive than owned infrastructure:
- Cloud pricing includes provider profit margin
- Reserved capacity requires prediction ability that reduces flexibility benefit
- Data transfer costs accumulate

### 8.4 When Cloud is Appropriate

**Good fit**:
- Variable workloads with unpredictable demand
- New applications without established patterns
- Startups without capital for infrastructure investment
- Global user base requiring geographic distribution
- Experimentation and rapid iteration
- Applications with spiky traffic patterns

**May not fit**:
- Predictable, stable workloads at large scale (do the math)
- Strict data residency requirements
- Ultra-low latency requirements (< 1-2ms)
- Specific hardware requirements (some HPC, specialized accelerators)
- Regulatory environments that prohibit cloud
- Situations where network reliability is insufficient

**Hybrid approaches**: Many organizations use a mix:
- Core infrastructure on-premises or in colocation
- Burst capacity in cloud
- Specific workloads where cloud excels
- Gradual migration over time

---

## 9. Looking Ahead

**Lecture 2: Cloud Service Models and Economics**
- NIST cloud definition and characteristics
- IaaS, PaaS, SaaS, FaaS taxonomy
- Cloud economics and pricing models
- Total Cost of Ownership (TCO) analysis
- Introduction to FinOps

The economic and service model understanding from Lecture 2 will complement the technical foundations from this lecture, giving you both the "how it works" and "how it's structured" perspectives on cloud computing.

---

## 10. Key Takeaways

1. **Cloud computing is an evolution, not a revolution**
   - It synthesizes decades of distributed computing research
   - Technical capabilities from mainframes, clusters, and grids
   - Economic model from utility computing
   - Self-service and APIs from web services

2. **Distributed systems are fundamentally different from single-machine systems**
   - Concurrency, lack of global clock, independent failures
   - The eight fallacies remind us not to assume network reliability
   - Design must account for partial failure

3. **The CAP theorem constrains distributed system design**
   - During network partitions, choose consistency OR availability
   - Most cloud services choose availability (AP) for better user experience
   - The choice depends on consequences of stale data vs. unavailability

4. **Consistency exists on a spectrum**
   - Strong consistency: simple programming model, expensive
   - Eventual consistency: complex programming model, performant
   - Intermediate models (causal, read-your-writes) offer trade-offs
   - Different data may warrant different consistency levels

5. **Consensus enables coordination but has costs**
   - Paxos and Raft allow distributed agreement
   - Requires a majority (quorum) of nodes
   - Adds latency; limits throughput
   - Essential for leader election, distributed transactions, configuration

6. **Failure is normal in distributed systems**
   - Design assuming components WILL fail
   - Use redundancy: replication, multiple AZs, multiple regions
   - Implement graceful degradation
   - Make failure detection and recovery automatic

7. **Understanding foundations enables better decisions**
   - Not just "how to use" but "why it works this way"
   - Enables debugging complex distributed failures
   - Informs architecture decisions
   - Guides service selection based on requirements

---

## 11. References and Further Reading

### Essential Reading

These resources are highly recommended for students wanting to deeply understand the material:

1. **Kleppmann, M. (2017). *Designing Data-Intensive Applications*. O'Reilly Media.**
   - The definitive guide to distributed data systems
   - Chapters 5 (Replication), 7 (Transactions), 8 (Trouble with Distributed Systems), 9 (Consistency and Consensus)
   - Accessible yet rigorous; highly recommended for this course

2. **Brewer, E. (2012). "CAP Twelve Years Later: How the 'Rules' Have Changed." IEEE Computer, 45(2), 23-29.**
   - Brewer's own retrospective on the CAP theorem
   - Clarifies nuances often missed in casual discussion
   - Available: https://www.infoq.com/articles/cap-twelve-years-later-how-the-rules-have-changed/

3. **Vogels, W. (2009). "Eventually Consistent." Communications of the ACM, 52(1), 40-44.**
   - Amazon CTO explains eventual consistency from a practical perspective
   - Describes consistency levels in Amazon's systems
   - Available: https://www.allthingsdistributed.com/2008/12/eventually_consistent.html

### Distributed Systems Theory

4. **Tanenbaum, A. S., & Van Steen, M. (2017). *Distributed Systems: Principles and Paradigms* (3rd ed.). Pearson.**
   - Comprehensive distributed systems textbook
   - Chapters 1-2 for foundations, Chapter 7 for consistency, Chapter 8 for fault tolerance

5. **Gilbert, S., & Lynch, N. (2002). "Brewer's Conjecture and the Feasibility of Consistent, Available, Partition-Tolerant Web Services." ACM SIGACT News, 33(2), 51-59.**
   - The formal proof of the CAP theorem
   - Available: https://users.ece.cmu.edu/~adrian/731-sp04/readings/GL-cap.pdf

6. **Fischer, M. J., Lynch, N. A., & Paterson, M. S. (1985). "Impossibility of Distributed Consensus with One Faulty Process." Journal of the ACM, 32(2), 374-382.**
   - The FLP impossibility result
   - Foundational paper in distributed systems theory

### Consensus Algorithms

7. **Ongaro, D., & Ousterhout, J. (2014). "In Search of an Understandable Consensus Algorithm." USENIX Annual Technical Conference.**
   - The Raft consensus paper
   - Excellent for understanding consensus practically
   - Available: https://raft.github.io/raft.pdf

8. **Lamport, L. (2001). "Paxos Made Simple." ACM SIGACT News, 32(4), 18-25.**
   - Lamport's simplified explanation of Paxos
   - Available: https://lamport.azurewebsites.net/pubs/paxos-simple.pdf

### Online Resources

9. **Raft Visualization**: https://raft.github.io/
   - Interactive visualization of the Raft protocol
   - Excellent for building intuition about consensus

10. **Jepsen**: https://jepsen.io/
    - Kyle Kingsbury's distributed systems testing project
    - Analyses of real databases' consistency guarantees
    - Demonstrates gap between documentation and reality

11. **The Morning Paper** (Adrian Colyer): https://blog.acolyer.org/
    - Accessible summaries of distributed systems papers
    - Excellent for going deeper on specific topics

12. **CAP FAQ**: https://www.the-paper-trail.org/page/cap-faq/
    - Comprehensive FAQ addressing common misconceptions

### Historical Context

13. **Armbrust, M., et al. (2010). "A View of Cloud Computing." Communications of the ACM, 53(4), 50-58.**
    - Seminal paper defining cloud computing
    - From Berkeley's RAD Lab
    - Available: https://www2.eecs.berkeley.edu/Pubs/TechRpts/2009/EECS-2009-28.pdf

14. **Barroso, L. A., Clidaras, J., & Hölzle, U. (2013). *The Datacenter as a Computer: An Introduction to the Design of Warehouse-Scale Machines*. Morgan & Claypool.**
    - How Google thinks about data center design
    - Available free: https://www.morganclaypool.com/doi/abs/10.2200/S00516ED2V01Y201306CAC024

---

## 12. Glossary

**Availability**: The property that every request receives a response. In CAP context, specifically means no request returns an error due to system state.

**Byzantine Failure**: A failure mode where a node behaves arbitrarily, potentially maliciously.

**CAP Theorem**: The theorem stating that a distributed system cannot simultaneously provide consistency, availability, and partition tolerance.

**Causal Consistency**: A consistency model where causally related operations are seen in the same order by all nodes.

**Consensus**: The problem of getting multiple nodes to agree on a single value.

**Consistency (CAP)**: The property that all nodes see the same data at the same time; reads always return the most recent write.

**Crash Failure**: A failure mode where a node stops executing and doesn't recover.

**Eventual Consistency**: A consistency model where, if no new updates are made, all reads will eventually return the same value.

**FLP Impossibility**: The result that no asynchronous consensus protocol can guarantee termination if even one process can fail.

**Heartbeat**: A periodic message sent to indicate that a node is alive.

**Linearizability**: The strongest consistency model; operations appear to occur atomically at some point between their invocation and response.

**Multi-tenancy**: Multiple customers sharing the same infrastructure.

**Network Partition**: A failure where the network divides nodes into groups that cannot communicate with each other.

**PACELC**: An extension of CAP that considers latency/consistency trade-offs during normal operation.

**Paxos**: A family of consensus protocols proven to be correct.

**Quorum**: A subset of nodes (typically a majority) required to agree for an operation to proceed.

**Raft**: A consensus protocol designed for understandability.

**Read-Your-Writes**: A consistency model where a client always sees its own writes.

**Replication**: Maintaining multiple copies of data for availability and durability.

**Sequential Consistency**: A consistency model where all operations appear in some sequential order consistent with each client's program order.

**Strong Consistency**: See Linearizability.

**Time-sharing**: Multiple users sharing a single computer's resources.

---

## Exercises

See the accompanying document: **Exercise_01_Distributed_Systems.md**

The exercise covers:
- CAP theorem analysis and system classification
- Consistency model scenarios and timeline analysis
- Quorum calculations
- Consensus and coordination problems
- System design decisions


---

*Document Version: 1.0*  
*Course: HIGH PERFORMANCE AND CLOUD COMPUTING 2025/2026*  
*Applied Data Science & Artificial Intelligence - University of Trieste*