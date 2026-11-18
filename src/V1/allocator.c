#include "allocator.h"
#include "freelist.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT alignof(max_align_t)
#define ALIGN_UP(n) (((n) + (ALIGNMENT - 1)) & ~(size_t)(ALIGNMENT - 1))

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
    if (num_bytes > SIZE_MAX - ALIGNMENT) return NULL; // ALIGN_UP would overflow
    size_t req_size = ALIGN_UP(num_bytes);

    if (free_list_head == NULL) {
        void* ptr = get_mmap_region(MEM_SIZE);
        if (ptr == NULL) return NULL; // mmap failed
        free_list_init(ptr, MEM_SIZE);
    }

    FitResult result = free_list_first_fit(req_size);
    if (result.curr == NULL) return NULL; // Free list empty

    free_list_allocate_block(result, req_size);
    return (char*)result.curr + sizeof(CommonHeader);
}

void sfree(void* p) {
    if (p == NULL) return;
    CommonHeader* block = (CommonHeader*)((char*)p - sizeof(CommonHeader));
    free_list_insert_and_merge(block);
}

size_t allocator_free_mem_size() {
    return free_list_total_free_size();
}

size_t allocator_req_mem(size_t payload_bytes) {
    return ALIGN_UP(payload_bytes) + sizeof(CommonHeader);
} 

void allocator_list_dump() {
    free_list_dump();
}

void allocator_stats(size_t* N, size_t* F, size_t* L) {
    FreeListStats stats;
    get_free_list_stats(&stats);
    *N = stats.block_count;
    *F = stats.free_memory;
    *L = stats.largest_block;
}