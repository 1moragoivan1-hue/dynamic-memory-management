#pragma once

#include <stddef.h>

void *smalloc(size_t num_bytes);
void sfree(void *pointer);
size_t allocator_free_mem_size(void);
size_t allocator_req_mem(size_t payload_bytes);
void allocator_list_dump(void);