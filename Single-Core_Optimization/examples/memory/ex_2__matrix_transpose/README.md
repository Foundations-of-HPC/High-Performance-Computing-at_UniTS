# Matrix transpose

### A clean example on the impact of the memory access pattern

---

This folder collects the example codes on a classic exercise, that here we give in multiple versions to highlight many different aspects.

| FOLDER                                                       | Description                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| [`matrix_transpose`](./matrix_transpose)                     | This is the starting point, where a plain version of the matrix transposition code is offered; the user can switch from having write- or read-contiguous accesses. See details in the `README` in that folder. |
| [`matrix_transpose_by_blocks`](matrix_transpose_by_blocks)   | This is the 2nd step: a cache-aware algorithm that enhances the locality by changing the access pattern, via tuning a free parameter (the tile size). |
| [`matrix_transpose_cache_oblivious`](matrix_transpose_cache_oblivious) | In this folder we collect two examples of cache-oblivious algorithms, i.e. algorithms that “naturally” lead to cache locality.<br />*Very interesting facts arise from the analysis and behaviour of these codes, including (i) how algorithms that on the paper are elegant and effective may be in practice not-so-efficient, and (iI) a crystal example of why memory aliasing from poorly written codes stops the compiler’s optimization.* |

