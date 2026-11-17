#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "allocator.h"

#define ALLOCATION_COUNT 10000

static double elapsed_seconds(
    const struct timespec *start,
    const struct timespec *end
)
{
    return (double)(end->tv_sec - start->tv_sec)
         + (double)(end->tv_nsec - start->tv_nsec) / 1e9;
}

int main(void)
{
    const size_t sizes[] = {16, 64, 256, 1024};
    void *allocations[ALLOCATION_COUNT] = {0};
    struct timespec start;
    struct timespec end;
    size_t successful_allocations = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i < ALLOCATION_COUNT; ++i) {
        allocations[i] = smalloc(sizes[i % 4]);

        if (allocations[i] == NULL) {
            break;
        }

        successful_allocations++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double allocation_time = elapsed_seconds(&start, &end);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i < successful_allocations; ++i) {
        sfree(allocations[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double free_time = elapsed_seconds(&start, &end);

    printf("Allocator benchmark:\n");
    printf("  allocations: %zu / %d\n",
           successful_allocations, ALLOCATION_COUNT);

    if (successful_allocations == 0) {
        return EXIT_FAILURE;
    }

    printf("  allocation total:   %.6f seconds\n", allocation_time);
    printf("  allocation average: %.2f ns\n",
           allocation_time * 1e9 / successful_allocations);
    printf("  free total:         %.6f seconds\n", free_time);
    printf("  free average:       %.2f ns\n",
           free_time * 1e9 / successful_allocations);

    return successful_allocations == ALLOCATION_COUNT ? EXIT_SUCCESS
                                                       : EXIT_FAILURE;
}
