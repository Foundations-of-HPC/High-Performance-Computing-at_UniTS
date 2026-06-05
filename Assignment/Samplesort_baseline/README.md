# Serial sample-sort baseline

This directory contains a serial C11 starting point for the parallel sample-sort exercise, with no OpenMP and MPI, but attempts to keep the same algorithmic shape as the distributed version:

1. split the array into `P` virtual chunks;
2. sort each chunk locally with merge sort;
3. select regular samples from every sorted chunk;
4. sort the gathered samples and choose global pivots;
5. partition each chunk into `P` buckets;
6. merge the `P` incoming sorted streams for each destination bucket;
7. verify that the final global sequence is non-decreasing.

The code is deliberately not a highly tuned sorter.  In particular, the final k-way merge scans all stream heads to choose the next key.  That is clear and
useful starting point, but you are expected to discuss alternatives such as a heap or tournament tree.

## Build

```sh
make
```

The default compiler flags are:

```sh
-std=c11 -O2 -Wall -Wextra -pedantic
```

Obviously these are not optimal: part of the exercise is defining compiler’s flag and options, as well as CPU bindings and so on.

## Example runs

```sh
./sample_sort_serial --n 1000000 --nbuckets 8 --distribution uniform
./sample_sort_serial --n 1000000 --nbuckets 8 --distribution skewed --oversample 4
./sample_sort_serial --n 32 --nbuckets 4 --distribution reverse --print-limit 32
```

A convenience check target runs several small correctness tests:

```sh
make check
```

## Command-line options

```text
--n VALUE              number of keys to sort
--nbuckets VALUE       number of virtual ranks / buckets
--oversample VALUE     regular-sampling multiplier
--seed VALUE           random seed
--distribution NAME    uniform | skewed | few-unique | sorted | reverse | almost-sorted
--print-limit VALUE    print the first VALUE sorted keys
--help                 show usage
```

`--nbuckets` is the serial analogue of the MPI process count.  For example, `--nbuckets 8` means that the single process simulates eight local chunks and
eight final destination buckets.

## Verification

The main verification is obvisou: sortedness.

```text
sorted_ok yes
```

The program also prints an order-independent signature before and after sorting:

```text
multiset_signature_ok yes
```

That signature is not a proof, but it is a useful extra guard while editing the bucket or merge code.  In the final MPI project, you should extend this idea
with a true distributed verifier: local sortedness, boundary ordering between neighbouring ranks, and a global check that elements were neither lost nor
duplicated.

## How to extend the code

Natural next steps for the assignment are marked in comments in the source:

- replace virtual chunks by MPI ranks;
- replace the direct sample array by `MPI_... ?`;
- turn bucket sizes into `MPI_... ?` counts;
- exchange buckets with `MPI_... ?`;
- parallelise or replace the local merge sort;
- replace the naive k-way merge by a heap or tournament-tree merge;
- measure imbalance for `uniform`, `skewed`, and `few-unique` inputs.

The `skewed` input is intentionally simple: most keys are drawn from a small low-value interval.  It is not a full Zipf distribution, but it is enough to make
pivot quality and bucket imbalance visible in a classroom baseline.
