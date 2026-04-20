# Exercise 01: Distributed Systems Foundations

##  Cloud Computing Course 2024/2025

This exercise reinforces the distributed systems concepts introduced in Lecture 1 through analysis, scenario evaluation, and reasoning about consistency and coordination.

---

## Part 1: CAP Theorem Analysis 

### Exercise 1.1: System Classification

For each system below, classify it as **CP** (Consistency + Partition Tolerance) or **AP** (Availability + Partition Tolerance). Provide justification.

| System | Your Classification | Justification |
|--------|---------------------|---------------|
| A. Banking application (balance checks, transfers) | | |
| B. Social media "like" counter | | |
| C. E-commerce shopping cart | | |
| D. Feature flag configuration service | | |
| E. DNS (Domain Name System) | | |
| F. Stock trading platform | | |

**For each system, also answer**: What would be the consequence of choosing the opposite trade-off?

---

## Part 2: Consistency Model Scenarios 

### Exercise 2.1: Eventual Consistency Timeline

Three nodes (A, B, C) in an eventually consistent system. Initial state: `x = 0` on all nodes.

Events:
```
t=1: Client writes x=5 to Node A
t=2: Client reads x from Node B
t=3: Node A propagates update to Node B  
t=4: Client reads x from Node C
t=5: Client reads x from Node B
t=6: Node B propagates update to Node C
t=7: Client reads x from Node C
```

**Fill in the table**:

| Time | Action | Return Value |
|------|--------|--------------|
| t=2 | Read from Node B | ? |
| t=4 | Read from Node C | ? |
| t=5 | Read from Node B | ? |
| t=7 | Read from Node C | ? |

**Questions**:
1. At what time does the system become fully consistent?
2. What is the maximum inconsistency window in this scenario?
3. How would you reduce the inconsistency window?

---

## Part 3: Consensus and Quorums 

### Exercise 3.1: Kubernetes etcd Sizing

You are designing a Kubernetes cluster for production use.

**Questions**:
1. etcd requires a majority for consensus. Complete the table:

| etcd Nodes | Majority Required | Max Node Failures Tolerated |
|------------|-------------------|----------------------------|
| 1 | | |
| 3 | | |
| 5 | | |
| 7 | | |

2. Why does Kubernetes documentation recommend 5-node etcd clusters for production (not 4 or 6)?

3. What happens to your Kubernetes cluster if a majority of etcd nodes become unavailable?

---

## Part 4: Synthesis

### Exercise 4.1: Design Decision

You are architecting a new cloud-based application: a **collaborative document editor** (like Google Docs).

**Requirements**:
- Multiple users can edit simultaneously
- Users should see each other's changes quickly
- The document should never be corrupted
- Service should remain available even during partial outages

**Questions**:
1. Is this application better suited to CP or AP? Justify.
2. What consistency model would you use for document updates?
3. How would you handle conflicting edits (two users editing the same paragraph)?
4. Where would you use consensus in this system?

---


*Exercise Version: 1.0*  
*Course:  HIGH PERFORMANCE AND CLOUD COMPUTING  2025/2026*  
*Applied Data Science & Artificial Intelligence - University of Trieste*