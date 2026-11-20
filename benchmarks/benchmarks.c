/**
 * READ ME
 * This is a timing benchmark for smalloc and sfree.
 * It runs the same workload as tests/stress_test.c (same seed, parameters and
 * request pattern) but measures how long the calls take instead of collecting
 * free-list stats.
 * - Makes 50,000 allocation requests with random sizes of up to 32 KB.
 * - Keeps 512 live blocks; a full slot is freed before being reused.
 * - Every 128 requests frees a random live block to create holes.
 * - Times every smalloc/sfree call and reports totals, averages and maximums.
 *   The cost of reading the clock is measured at startup and subtracted.
 *   Successful and failed smalloc calls are reported separately, because a
 *   failed call usually walks the whole free list.
 */

#define _POSIX_C_SOURCE 199309L  // expose clock_gettime even under strict -std=c23

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "allocator.h"   // smalloc, sfree

// Tunable Parameters (keep in sync with tests/stress_test.c)
#define N_REQUESTS   50000         // total number of allocation requests
#define MAX_REQ_SIZE 32 * 1024     // cap on a single request size (bytes)
#define D_FREQ       128           // every D_FREQ allocations, free a random live block to create holes
#define LIVE         512           // number of concurrently live allocations to keep

typedef struct {
    size_t count;
    double total_ns;
    double max_ns;
} Timing;

// Random request size in [1..MAX_REQ_SIZE]
static inline size_t rand_size(void) { return (size_t)(rand() % MAX_REQ_SIZE) + 1; }

static inline double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

// Cost of one now_ns() call, measured at startup and removed from every sample.
// It matters on WSL2/VMs, where clock_gettime can take ~1 us instead of ~20 ns.
static double timer_overhead_ns = 0.0;

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

// Median of back-to-back now_ns() deltas: what an empty timed region measures.
static double calibrate_timer(void) {
    enum { SAMPLES = 10001 };
    static double deltas[SAMPLES];
    for (size_t i = 0; i < SAMPLES; ++i) {
        double a = now_ns();
        double b = now_ns();
        deltas[i] = b - a;
    }
    qsort(deltas, SAMPLES, sizeof deltas[0], cmp_double);
    return deltas[SAMPLES / 2];
}

static inline void record(Timing *t, double ns) {
    ns -= timer_overhead_ns;
    if (ns < 0.0) ns = 0.0;
    t->count++;
    t->total_ns += ns;
    if (ns > t->max_ns) t->max_ns = ns;
}

static void timed_free(void *p, Timing *t) {
    double start = now_ns();
    sfree(p);
    record(t, now_ns() - start);
}

static void print_timing(const char *label, const Timing *t) {
    printf("\n%s:\n", label);
    printf("\tCalls: %zu\n", t->count);
    if (t->count == 0) return;
    printf("\tTotal: %.6f s\n", t->total_ns / 1e9);
    printf("\tAverage: %.2f ns\n", t->total_ns / (double)t->count);
    printf("\tMaximum: %.2f ns\n", t->max_ns);
}

int main(void) {
    srand(12345); // same seed as the stress test, so the workload is identical
    timer_overhead_ns = calibrate_timer();

    Timing malloc_ok = {0};    // successful smalloc calls
    Timing malloc_fail = {0};  // smalloc calls that returned NULL
    Timing frees = {0};        // all sfree calls

    // Keep up to LIVE live allocations at any time
    void *pool[LIVE] = {0};
    size_t idx = 0;

    double run_start = now_ns();

    for (size_t i = 0; i < N_REQUESTS; ++i) {
        size_t sz = rand_size();

        double start = now_ns();
        void *p = smalloc(sz);
        double elapsed = now_ns() - start;

        if (p) {
            record(&malloc_ok, elapsed);

            // Keep up to LIVE active allocations: overwrite round-robin slot
            if (pool[idx]) timed_free(pool[idx], &frees);
            pool[idx] = p;
            idx = (idx + 1) % LIVE;
        } else {
            record(&malloc_fail, elapsed);
        }

        // Every D_FREQ requests, free a random live slot to create holes
        if ((i + 1) % D_FREQ == 0) {
            size_t k = (size_t)(rand() % LIVE);
            if (pool[k]) { timed_free(pool[k], &frees); pool[k] = NULL; }
        }
    }

    double run_ns = now_ns() - run_start;

    // Release everything still live (not part of the timed workload)
    for (size_t k = 0; k < LIVE; ++k) {
        if (pool[k]) sfree(pool[k]);
    }

    Timing malloc_all = {
        .count = malloc_ok.count + malloc_fail.count,
        .total_ns = malloc_ok.total_ns + malloc_fail.total_ns,
        .max_ns = malloc_ok.max_ns > malloc_fail.max_ns ? malloc_ok.max_ns : malloc_fail.max_ns,
    };

    // Print Report
    printf("\nOverall:\n");
    printf("\tTotal Memory: %.2f MB\n", MEM_SIZE / (1024.0 * 1024.0));
    printf("\tRequests: %zu\n", (size_t)N_REQUESTS);
    printf("\tTimer Overhead (subtracted per call): %.1f ns\n", timer_overhead_ns);
    printf("\tWall Time (incl. timing overhead): %.6f s\n", run_ns / 1e9);
    printf("\tThroughput: %.0f ops/s\n",
           (double)(malloc_all.count + frees.count) / (run_ns / 1e9));

    print_timing("smalloc (all)", &malloc_all);
    print_timing("smalloc (successful)", &malloc_ok);
    print_timing("smalloc (failed)", &malloc_fail);
    print_timing("sfree", &frees);

    printf("\n");

    return 0;
}
