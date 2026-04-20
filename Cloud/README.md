# Cloud Computing — ADSAI, University of Trieste
### Academic Year 2025/2026

---

## Course Overview

This repository contains all teaching materials for the **Cloud Computing** module within the *HIGH PERFORMANCE AND CLOUD COMPUTING* at *Data Science and Artificial Intelligence* (ADSAI) programme at the University of Trieste.

The course is not a standalone unit: it is an integral part of the **P2 — Systems** pillar of the master curriculum, sitting between foundational courses (computer architecture, parallel programming, numerical methods) and applied courses (scientific computing, machine learning, domain projects). Its defining characteristic is that cloud infrastructure is studied *in the framework of HPC* — meaning that every concept, tool, and design decision is evaluated through the lens of high-performance, large-scale scientific computing.

> *"Don't be a user of pre-cooked tools that you consider as black-boxes."*

The pedagogical approach is **principles-first**: understanding *why* a system behaves as it does takes priority over operational familiarity with any particular tool. Concepts outlast specific products; the ability to reason about trade-offs transfers across environments.

---

## Why Cloud for HPC Practitioners?

Cloud and HPC are converging. AWS, Azure, and GCP all offer HPC-grade instances (InfiniBand fabrics, bare-metal nodes, large-scale GPU clusters), while traditional HPC centres increasingly adopt cloud technologies (containers, orchestration, infrastructure-as-code). Understanding both paradigms is therefore essential for modern scientific computing:

- HPC clusters increasingly rely on cloud technologies (containers, orchestration engines, IaC pipelines).
- Cloud providers offer HPC-grade resources (InfiniBand networking, GPU clusters, bare-metal instances).
- Hybrid workflows are common: prototype on cloud, run production on HPC — or burst HPC workloads onto cloud when local queues are saturated.
- Skills transfer: SLURM ↔ Kubernetes, environment modules ↔ containers, Lustre ↔ object/parallel storage.

---

## Learning Objectives

By the end of this course, students will be able to:

1. Explain the fundamental principles of distributed systems (CAP theorem, consistency models, consensus protocols).
2. Design and deploy containerised applications using Docker and related tooling.
3. Deploy and manage workloads on Kubernetes (Pods, Deployments, Services, Helm, GitOps).
4. Use and provision cloud infrastructure programmatically.
5. Run HPC workloads in containers using Apptainer with MPI on CINECA infrastructure.

---

## Assessment

The final examination is **joint with the HPC module** — there is no separate cloud exam. Assessment integrates both modules to reflect how these competencies interact in real-world scientific computing.

The evaluation consists of two steps:

1. **Final assignment** — distributed before the end of lectures. Each student completes it individually (discussion and collaboration are permitted, but submissions must be strictly individual). The assignment involves:
   - Completing one or more HPC exercises on CINECA's supercomputer.
   - Replicating the same exercises using Apptainer containers.
   - Writing a technical report with an analytical and critical comparison of bare-metal HPC vs. containerised execution (performance, reproducibility, trade-offs).
2. **Oral examination** — each candidate discusses their assignment individually, together with questions covering the broader course content.

**Infrastructure note:** A CINECA account is **required** for the exam. A personal AWS Free Tier account is useful for hands-on practice during the course but is **not mandatory** — all cloud demonstrations can be observed rather than executed.

---

## Repository Structure

```
.
├── Lectures/       # Slide decks (released incrementally as the course progresses)
├── Notes/          # Comprehensive written notes for each lecture (Markdown)
├── Exercises/      # Optional self-assessment exercises (not examined)
└── Examples/       # Code and configuration examples demonstrated during lectures
```

### `Lectures/`

Slide decks in presentation format, released progressively as each lecture is delivered. Slides are intentionally concise: they accompany the spoken explanation and are not self-contained references. For a complete treatment of any topic, refer to the corresponding notes.

### `Notes/`

Detailed written notes for each lecture. These are the primary reference material for the course. Each note file covers the full conceptual content of the corresponding lecture, including:

- Theoretical background and formal definitions.
- Design trade-offs and their quantitative implications.
- Connections to HPC practice (MPI analogies, SLURM comparisons, CINECA-relevant considerations).
- Recommended books, papers, websites, and videos for deeper exploration.

### `Exercises/`

Optional exercises aligned to each lecture. These are designed to help students gauge their own understanding and identify gaps before the final assessment. They are **not required for the exam** and will not be evaluated. They are, however, always available for discussion — students are encouraged to attempt them and bring questions to office hours or to the course forum.

Exercises are grounded in scientific computing scenarios (climate modelling, genomics pipelines, molecular dynamics, CFD) and typically combine analytical reasoning with quantitative calculation or practical implementation.

### `Examples/`

Complete, working code and configuration examples used as tutorials during lectures. These include Dockerfiles, Kubernetes manifests, Terraform modules, Apptainer definition files, and shell scripts. They are intended to be read, run, and modified — not memorised.

---

## Practical Requirements

| Resource | Status | Notes |
|----------|--------|-------|
| CINECA account | **Required** | Needed for the final exam (Apptainer + SLURM exercises) |
| AWS Free Tier account | Optional | Useful for hands-on practice; not required to pass the course |
| Docker, kubectl, AWS CLI, Apptainer | Installed as needed | Installation guides provided in the relevant lecture notes |

When using AWS: **always clean up resources after exercises** to avoid unexpected charges. Every example and exercise includes explicit teardown instructions.

---

## References

The primary reference for the distributed systems foundations of this course is:

> Kleppmann, M. (2017). *Designing Data-Intensive Applications*. O'Reilly Media.

Additional references — including textbooks, foundational papers, and online resources — are listed at the end of each lecture's notes.

---

## Repository

All materials are hosted publicly at:

**[github.com/Foundations-of-HPC](https://github.com/Foundations-of-HPC)**

The landing page of the organisation lists all active repositories with a detailed index of lectures and their current availability status.

---

*Cloud Computing module — ADSAI, University of Trieste —  2025/2026*
