#include "allocator.h"
#include "freelist.h"
#include <sys/mman.h>
#include <stdio.h>

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
    if (free_list_head == NULL) {
        void* ptr = get_mmap_region(MEM_SIZE);
        if (ptr == NULL) return NULL; // mmap failed
        free_list_init(ptr, MEM_SIZE);
    }

    FitResult result = free_list_first_fit(num_bytes);
    if (result.curr == NULL) return NULL; // Free list empty

    free_list_allocate_block(result, num_bytes);
    return (char*)result.curr + sizeof(CommonHeader);
}

void sfree(void* p) {
    if (p == NULL) return;
    CommonHeader* block = (CommonHeader*)((char*)p - sizeof(CommonHeader));
    free_list_insert_head(block);
}

size_t allocator_free_mem_size() {
    return free_list_total_free_size();
}

size_t allocator_req_mem(size_t payload_bytes) {
    return payload_bytes + sizeof(CommonHeader);
} 

void allocator_list_dump() {
    free_list_dump();
}
