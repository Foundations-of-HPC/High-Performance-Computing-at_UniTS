# High-Performance-Computing-at_UniTS
This repository contains the slides, examples, materials and assignments for the course "Basic HPC" at University of Trieste from year 2025-2026 on. 

*Materials from previous years are stored in repositories named `High-Performance-Computing-YYY`* 

**Teachers:**

_HPC_  

Luca Tornatore, Italian National Institute for Astrophysics, Astronomical Observatory of Trieste, Trieste  
luca.tornatore@inaf.it

Stefano Cozzini, Area di Ricerca, Trieste  
stefano.cozzini@areasciencepark.it

_CLOUD_

Giuliano Taffoni, Italian National Institute for Astrophysics, Astronomical Observatory of Trieste, Trieste  
giuliano.taffoni@inaf.it  

**Instructor:**

Isac Pasianotto, Ph.D. candidate, Università di Trieste, Area di Ricerca, Trieste  
isac.pasianotto@areasciencepark.it

---

The «Basic High-Performance Computing course» aims at teaching to master students the foundations of HPC:

- **modern CPU and single-node architecture**
  Memory hierarchy, branch prediction and out-of-order execution: putting all together to write an efficient serial code that exploits the potential delivered by Instruction- and Data- level parallelism
  Slides, lecture notes, additional notes and example codes in the folder [`./Single-Core_Optimization`](./Single-Core_Optimization).
- **shared-memory parallelism** 
  Multi-threading with OpenMP (intermediate level)
  Slides, lecture notes, additional notes and example codes in the folder [`./OpenMP`](./OpenMP).
- **architecture of an HPC cluster**
  From nodes to network and storage
  Slides in the folder [`./MPI`](./MPI).
- **distributed-memory parallelism** 
  MPI at intermediate level: p2p blocking and non-blocking, collectives, I/O, groups&communicators
  Slides, lecture notes, additional notes and example codes in the folder [`./MPI`](./MPI).
- **debugging & profiling** serial and parallel codes
  Slides, lecture notes, additional notes and example codes in the folder [`./Debugging and Profiling`](./Debugging_and_Profiling).

#### REFERENCES & BOOKS


##### Ref 1: Introduction to High Performance Computing for Scientists and Engineers
by Georg Hager and Gerhard Wellein Paperback: 356 pages Publication date July 2, 2010 Editors (Chapman & Hall/CRC Computational Science)

A lot of material is taken from this book. A very nice and complete reference. A little bit old however..

##### Ref 2: High Performance Computing Modern Systems and Practices
Thomas Sterling Matthew Anderson Maciej Brodowicz eBook ISBN: 9780124202153 Paperback ISBN: 9780124201583

Some of the materials of this book are presented during some lectures.

##### Ref 3: Introduction to High-Performance Scientific Computing,by Victor Eijkhout
The source and pdf of the book (as well as lecture slides) can be found at this link: https:// https://theartofhpc.com//istc/istc.html, DOI: 10.5281/zenodo.49897

Some of the materials presented in classes are taken from this book.

##### Ref 4: Computer Organization and Design
by D. A. Patterson and J. L. Hennessy The Morgan Kaufmann Series in Computer Architecture and Design easily available as pdf on the net.

##### Ref 5: Optimizing HPC Applications with Intel Cluster Tools
Paperback – October 15, 2014 by Alexander Supalov (Author), Andrey Semin (Author), Michael Klemm (Author), & 1 more ISBN-13: 978-1430264965 ISBN-10: 1430264969 Edition: 1st

Nice book but a little bit outdated
