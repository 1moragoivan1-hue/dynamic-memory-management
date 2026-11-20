#include "allocator.h"
#include <sys/mman.h>
#include <stdio.h>
#include "freelist.h"

static Arena heap = {.capacity = MEM_SIZE };

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
    if (heap.base == NULL) {
        heap.base = get_mmap_region(heap.capacity);
        if (heap.base == NULL) return NULL; // mmap failed
        free_list_init(&heap.head, heap.base, heap.capacity);
    }

    FitResult result = free_list_first_fit(heap.head, num_bytes);
    if (result.curr == NULL) return NULL; // Free list empty

    free_list_allocate_block(&heap.head, result, num_bytes);
    return (char*)result.curr + sizeof(CommonHeader);
}

void sfree(void* p) {
    if (p == NULL) return;
    CommonHeader* block = (CommonHeader*)((char*)p - sizeof(CommonHeader));
    free_list_insert_and_merge(&heap.head, block);
}

size_t allocator_free_mem_size() {
    return free_list_total_free_size(heap.head);
}

size_t allocator_req_mem(size_t payload_bytes) {
    return payload_bytes + sizeof(CommonHeader);
} 

void allocator_list_dump() {
    free_list_dump(heap.head);
}
