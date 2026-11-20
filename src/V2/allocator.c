#include "allocator.h"
#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include "freelist.h"

enum { TINY, SMALL, MEDIUM, ARENA_COUNT };

static Arena arenas[ARENA_COUNT] = {
    [TINY] = { .capacity = 1 * 1024 * 1024 },
    [SMALL] = { .capacity = 3 * 1024 * 1024 },
    [MEDIUM] = { .capacity = 6 * 1024 * 1024 },
};

static const char *const arena_names[ARENA_COUNT] = {
    [TINY] = "Tiny", [SMALL] = "Small", [MEDIUM] = "Medium",
};

static Arena *arena_for_size(size_t total_size) {
    if (total_size <= 256) return &arenas[TINY];
    if (total_size <= 8 * 1024) return &arenas[SMALL];
    return &arenas[MEDIUM];
}

static void* get_mmap_region(size_t num_bytes) {
    void *ptr = mmap(
        NULL,
        num_bytes,
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        -1,
        0 
    );

    return (ptr == MAP_FAILED) ? NULL : ptr;
}


void* smalloc(size_t num_bytes) {
    if (num_bytes == 0) return NULL;
    size_t total_size = allocator_req_mem(num_bytes);
    if (total_size == 0) return NULL;

    Arena *arena = arena_for_size(total_size);
    if (total_size > arena->capacity) return NULL;

    if (arena->base == NULL) {
        arena->base = get_mmap_region(arena->capacity);
        if (arena->base == NULL) return NULL;
        free_list_init(&arena->head, arena->base, arena->capacity);
    }

    size_t payload_size = total_size - HEADER_SIZE;
    FitResult result = free_list_best_fit(arena->head, payload_size);
    if (result.curr == NULL) return NULL;

    free_list_allocate_block(&arena->head, result, payload_size);
    return (char *)result.curr + HEADER_SIZE;
}

void sfree(void* p) {
    if (p == NULL) return;
    uintptr_t address = (uintptr_t)p;
    for (size_t i = 0; i < ARENA_COUNT; ++i) {
        Arena *arena = &arenas[i];
        if (arena->base == NULL) continue;

        uintptr_t base = (uintptr_t)arena->base;
        if (address >= base && address - base >= HEADER_SIZE &&
            address - base < arena->capacity) {
            CommonHeader *block = (CommonHeader *)((char *)p - HEADER_SIZE);
            free_list_insert_and_merge(&arena->head, block);
            return;
        }
    }
}

size_t allocator_free_mem_size(void) {
    size_t total = 0;
    for (size_t i = 0; i < ARENA_COUNT; ++i) {
        total += free_list_total_free_size(arenas[i].head);
    }
    return total;
}

size_t allocator_req_mem(size_t payload_bytes) {
    size_t remainder = payload_bytes % BLOCK_ALIGNMENT;
    size_t padding = remainder == 0 ? 0 : BLOCK_ALIGNMENT - remainder;
    if (payload_bytes > SIZE_MAX - HEADER_SIZE - padding) return 0;
    return HEADER_SIZE + payload_bytes + padding;
}

void allocator_list_dump(void) {
    for (size_t i = 0; i < ARENA_COUNT; ++i) {
        printf("%s: ", arena_names[i]);
        free_list_dump(arenas[i].head);
    }
}

void allocator_stats(size_t *block_count, size_t *free_bytes, size_t *largest_block) {
    FreeListStats stats = {0};
    for (size_t i = 0; i < ARENA_COUNT; ++i) {
        get_free_list_stats(arenas[i].head, &stats);
    }
    *block_count = stats.block_count;
    *free_bytes = stats.free_memory;
    *largest_block = stats.largest_block;
}
