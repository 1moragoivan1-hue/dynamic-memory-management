# Dynamic Memory Management - V1 Stress Results

## Test setup

The stress test in `tests/stress_test.c` performs 50,000 allocation requests
against a 10 MB heap. Each request is for a random size between 1 byte and
32 KB. At most 512 allocations remain live, and every 128 requests, one live
allocation is freed.

The three V1 configurations were compared using the same fixed random seed,
`12345`, so that they received the same sequence of allocation and free
requests.

## Results

| Configuration | Successful allocations | Successful requests | First failure | Final free blocks | Final external fragmentation | Runtime |
|---|---:|---:|---:|---:|---:|---:|
| First fit, no merge | 10,734 | 21.47% | Request 874 | 7,547 | 0.9996 | 10.41 s |
| Best fit, no merge | 38,519 | 77.04% | Request 3,176 | 2,887 | 0.9960 | 4.31 s |
| Best fit, with merge | 50,000 | 100.00% | None | 76 | 0.2349 | 0.16 s |

Additional values from the runs:

- **First fit, no merge:** 48.15 MB was allocated overall. At the first
  failure, 2.42 MB remained free and the failed request was 28 KB.
- **Best fit, no merge:** 453.48 MB was allocated overall. At the first
  failure, 2.36 MB remained free and the failed request was 28 KB.
- **Best fit, with merge:** 757.05 MB was allocated overall. No allocation
  failed.

## Approach

The allocator uses a linked list of free blocks.

- **First fit** scans the list from the beginning and returns the first block
  large enough for the request.
- **Best fit** scans the complete list and selects the smallest block that can
  satisfy the request.
- **Without merge**, a freed block is inserted as a separate free-list node.
- **With merge**, a freed block is inserted in address order and combined with
  any immediately adjacent free blocks.

The policy is controlled in `src/V1/allocator.c` by `FIT_TYPE` and
`SHOULD_MERGE`.

## Why the results changed

First fit, no merge performed worst because its allocations consume early holes
and leave many small leftover fragments. Although 2.42 MB remained free at
failure, it was split into blocks that could not satisfy the requested 28 KB
allocation. This fragmentation caused failure after only 874 requests.

Best fit, no merge delayed failure until request 3,176. It chose the smallest
suitable hole for each allocation, avoiding waste in larger holes. Larger holes
were therefore preserved for larger requests. It still did not merge neighbors,
so the heap eventually became highly fragmented.

Best fit, with merge reduced fragmentation the most. When a block was freed,
coalescing combined it with adjacent free blocks and rebuilt larger contiguous
regions. As a result, all 50,000 requests succeeded and the free-list remained
small.

## Why the runtime changed

The measured runtimes were affected primarily by the size of the free list:

1. First fit with no merge reached 7,547 free blocks, requiring repeated list
   traversal and longer allocator statistics scans.
2. Best fit with no merge reached 2,887 free blocks. Although best fit searches
   the entire list for every allocation, fewer and better-organized blocks
   reduced the total work enough to complete faster.
3. Best fit with merge kept only 76 free blocks at the end and 100 at maximum.
   Allocation searches and statistics updates were consequently much faster.

Runtime numbers vary by machine and system load, but the large difference
demonstrates that coalescing not only reduces fragmentation, but also keeps
allocator metadata smaller and operations cheaper.

## Timing benchmark

`benchmarks/benchmarks.c` runs the same workload as the stress test (same
seed, parameters and request sequence). Instead of collecting free-list
statistics, it times each `smalloc` and `sfree` call.

### Method

- `now_ns()` reads `CLOCK_MONOTONIC` through `clock_gettime` and returns
  nanoseconds.
- Each call is timed on its own: `start = now_ns(); smalloc(sz); end = now_ns();`.
  `timed_free()` does the same for `sfree`.
- `record()` adds each sample to a `Timing` struct (call count, total time,
  maximum). Successful and failed `smalloc` calls go into separate structs
  because they behave differently. A failure has to scan the entire free list.
- `calibrate_timer()` runs at startup and times 10,001 empty regions (two
  back-to-back `now_ns()` calls). It takes the median as the timer overhead,
  and `record()` subtracts that overhead from every sample. This matters on
  WSL2 and VMs. Here, `clock_gettime` costs about 600–1,000 ns per call,
  compared with about 20 ns on native Linux.
- Blocks still live at the end are freed after timing stops.

### Results

Each configuration was built with `gcc -O2` and run 7 times, with the
configurations interleaved. The values below are medians. They were measured
on WSL2 (x86-64, 12 threads).

| Configuration | `smalloc` avg (all) | avg successful | avg failed | `sfree` avg | Total `smalloc` time | Total `sfree` time | Wall time |
|---|---:|---:|---:|---:|---:|---:|---:|
| First fit, no merge | 80.2 µs | 9.7 µs | 99.2 µs | ~0.4 µs | 4.01 s | 4.1 ms | 4.17 s |
| Best fit, no merge | 34.7 µs | 32.2 µs | 42.6 µs | ~0.2 µs | 1.74 s | 8.6 ms | 1.90 s |
| Best fit, with merge | 0.33 µs | 0.33 µs | n/a (no failures) | ~0.08 µs | 16.4 ms | 4.1 ms | 0.22 s |

Call counts were identical on every run: 10,734, 38,519 and 50,000 successful
allocations, with 39,266, 11,481 and 0 failures respectively.

**How to read these numbers:**

- Run-to-run variation was large, up to about 2× between the fastest and
  slowest run of the same configuration. The differences between
  configurations are much larger than that variation, so the ranking is
  reliable. The individual values are only rough.
- Sub-microsecond values (all `sfree` averages, and `smalloc` with merge) are
  close to the noise in the timer overhead, which varied between 600 and
  2,300 ns. Treat them as "well under 1 µs", not as exact figures.
- Wall time includes the timer overhead (two clock reads per call). With
  merge enabled, the allocator calls themselves take only about 20 ms of the
  0.22 s total.

### Comparison

**Allocation time follows the length of the free list.** Every `smalloc`
walks the list, so its cost is about (nodes visited) × (cost per node). The
nodes are spread across the 10 MB heap, so each step is usually a cache miss.

- **First fit, no merge:** successful calls are relatively cheap (9.7 µs)
  because the search stops at the first block that fits. But 79% of requests
  fail, and each failure walks the whole list of about 7,500 fragments
  (99 µs). Failures account for almost all of the 4 s.
- **Best fit, no merge:** every call, successful or not, scans the whole
  list, so successful calls cost about the same as failed ones (32 µs vs
  43 µs). That makes a successful call about 3× slower than with first fit.
  Overall it is still about 2.3× faster, because best fit fragments less: the
  list stays at about 2,900 nodes and far fewer requests fail.
- **Best fit, with merge:** coalescing keeps the list at about 100 nodes or
  fewer. A full best-fit scan of 100 nodes takes about 0.3 µs. That is
  roughly 250× faster than first fit without merge and 100× faster than best
  fit without merge, and every request succeeds.

**Free time is small in every configuration.** Without merge, `sfree` pushes
onto the head of the list, which takes constant time. With merge, `sfree`
walks the list to find the block's address-ordered position, which takes time
proportional to the list length. Because merging keeps the list short, that
walk is still cheap (well under 1 µs). In no configuration is `sfree` the
bottleneck.

**Conclusion:** merging is the change that matters most. It improves both
memory efficiency and speed, because a short free list makes every search
cheap. The choice between best fit and first fit matters much less once
merging is enabled.

### Reproducing

Set `FIT_TYPE` and `SHOULD_MERGE` in `src/V1/allocator.c`. Then build and run
an optimised benchmark:

```sh
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target benchmarks_V1
./build-release/benchmarks_V1
```
