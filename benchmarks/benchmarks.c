#include <stddef.h>
#include <stdio.h>
#include <time.h>

typedef void (*benchmark_function)(void*);

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
        function(context);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < iterations; ++i) {
        function(context);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double total = elapsed_seconds(&start, &end);

    printf("%s:\n", name);
    printf("  total:   %.6f seconds\n", total);
    printf("  average: %.2f ns\n",
           total * 1e9 / (double)iterations);
}