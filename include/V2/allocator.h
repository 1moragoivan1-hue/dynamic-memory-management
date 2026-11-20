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
