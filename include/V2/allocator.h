#pragma once

#include <stddef.h>
#include "freelist.h"

#define MEM_SIZE (10 * 1024 * 1024) // 10MB

typedef struct {
    void *base;
    size_t capacity;
    CommonHeader *head;
} Arena;


void *smalloc(size_t num_bytes);
void sfree(void *pointer);
size_t allocator_free_mem_size(void);
size_t allocator_req_mem(size_t payload_bytes);
void allocator_list_dump(void);
void allocator_stats(size_t *block_count, size_t *free_bytes, size_t *largest_block);
