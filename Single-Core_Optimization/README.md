# Single-Core Optimization

This folder collects the slides, example codes and additional materials focused on writing efficient serial code on a single core.

The aim is to illustrate how profoundly the peculiarities of modern CPU architecture impact on the way a C/C++ code must be written to exploit their potential - or at least not to incur in severe penalties.

### Slides
The **slides** are organized as follows:

| Topic                                     | description                                                  | file                                                     |
| ----------------------------------------- | ------------------------------------------------------------ | -------------------------------------------------------- |
| References                                | references on architectures, running model, optimizations and all the single topics | [`Optimization-references.pdf`](./Optimization-references.pdf) |
| CPU architecture                          | the fundamental traits of modern CPU architecture,  elements of their historical developments and of the impact they have on programming | [`01--Modern_architecture.pdf`](01--Modern_architecture.pdf) |
| Preliminary concepts and compiler’s usage | Some gentle introduction on “optimization” at large; how to use (some) compiler’s options | [`00--Optimization--preliminaries_and_compiler_usage.pdf`](00--Optimization--preliminaries_and_compiler_usage.pdf) |
| The running model                         | Traits of the running model; the virtual memory address space, the TLB, the stack, the heap | [`02--optimization--heap_and_stack.pdf`](02--optimization--heap_and_stack.pdf) |
| First steps                               | Very basic concepts and trivial mistakes to be avoided       | [`03--optimization--basic_steps.pdf`](03--optimization--basic_steps.pdf) |
| Optimization techniques                   | Fundamental techniques on writing efficient codes accounting for caches, ILP, SIMD, pipelines, branches | [`Single-core__cache_branches_pipelines_loops.pdf`](Single-core__cache_branches_pipelines_loops.pdf) |
| Sparse topics of interest                 | How to measure performance, overview of pointers, integer representation | [`topics.pdf`](topics.pdf) |

### Lecture Notes
While the course progresses, there will also be **extended lecture notes**, elaborated from the lecture transcripts and slightly revised to include additional materials and references, in the [`notes`](./notes) folder.


### Example codes
For every main topic, there are **example codes** that illustrate its traits, as follows

| TOPICS                                         | sub-topics                                                   |
| ---------------------------------------------- | ------------------------------------------------------------ |
| ***stack & heap***                             | order of bytes in memory                                     |
|                                          | understanding the stack |
|                                          | stack overflow and stack smashing |
| ***basic steps***                              |                                                              |
| ***memory hierarchy***                         | contiguous and strided memory traversal of arrays            |
|                                                | memory mountain                                              |
|                                                | matrix transposition                                         |
|                                                | hot & cold fields                                            |
|                                                | Aos vs SoA                                                   |
| ***conditional evaluation<br />and branches*** |                                                              |
| ***loop optimization***                        | matrix multiplication                                        |
|                                                | array reduction                                              |
|                                                | array multiplication                                         |
|                                                | prefetching                                                  |
| ***debugging***                                | serial & parallel debugging                                  |
| ***profiling***                                | using `perf`, `valgrind`, `PAPI`; elements of parallel profiling |







Recordings are available at https://drive.google.com/drive/folders/1Ya4EH2lN6ViO4JpWZU-G2xXwKuUh1Rn4?usp=sharing

