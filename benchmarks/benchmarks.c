#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "allocator.h"

#define FRAGMENTATION_SLOTS 512
#define ALLOCATION_SLOT_STRIDE 37
#define FREE_SLOT_STRIDE 97
#define FREE_INTERVAL 4

typedef void (*benchmark_function)(void *, size_t);

static double elapsed_seconds(
    const struct timespec *start,
    const struct timespec *end
)
{
    return (double)(end->tv_sec - start->tv_sec)
         + (double)(end->tv_nsec - start->tv_nsec) / 1e9;
}

static void benchmark(
    const char *name,
    benchmark_function function,
    void *context,
    size_t iterations)
{
    struct timespec start;
    struct timespec end;

    // Warmup function
    for (size_t i = 0; i < 1000; ++i) {
        function(context, i);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < iterations; ++i) {
        function(context, i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double total = elapsed_seconds(&start, &end);

    printf("%s:\n", name);
    printf("  total:   %.6f seconds\n", total);
    printf("  average: %.2f ns\n",
           total * 1e9 / (double)iterations);
}

typedef struct {
    const size_t *allocation_sizes;
    size_t size_count;
    void *slots[FRAGMENTATION_SLOTS];
    size_t allocation_failures;
} allocation_context;

static void benchmark_fragmented_allocations(void *context, size_t iteration)
{
    allocation_context *allocation = context;
    size_t slot = (iteration * ALLOCATION_SLOT_STRIDE) % FRAGMENTATION_SLOTS;

    if (iteration % FREE_INTERVAL == 0) {
        size_t slot_to_free = (iteration * FREE_SLOT_STRIDE) % FRAGMENTATION_SLOTS;

        if (allocation->slots[slot_to_free] != NULL) {
            sfree(allocation->slots[slot_to_free]);
            allocation->slots[slot_to_free] = NULL;
        }
    }

    if (allocation->slots[slot] == NULL) {
        size_t size = allocation->allocation_sizes[iteration % allocation->size_count];
        allocation->slots[slot] = smalloc(size);

        if (allocation->slots[slot] == NULL) {
            allocation->allocation_failures++;
        }
    }
}

static void free_remaining_allocations(allocation_context *allocation)
{
    for (size_t i = 0; i < FRAGMENTATION_SLOTS; ++i) {
        sfree(allocation->slots[i]);
        allocation->slots[i] = NULL;
    }
}

int main(void)
{
    const size_t num_sizes = 5;
    const size_t allocation_sizes[] = {16, 64, 256, 1024, 4096};
    allocation_context allocation = {
        .allocation_sizes = allocation_sizes,
        .size_count = num_sizes
    };

    benchmark(
        "smalloc with fragmented free list",
        benchmark_fragmented_allocations,
        &allocation,
        100000
    );

    printf("  allocation failures: %zu\n", allocation.allocation_failures);

    free_remaining_allocations(&allocation);

    return 0;
}