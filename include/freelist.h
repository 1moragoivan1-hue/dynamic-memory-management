#pragma once

#include <stddef.h>

typedef struct common_header {
    size_t size;
    struct common_header *next;
} CommonHeader;

typedef struct {
    CommonHeader *prev;
    CommonHeader *curr;
} FitResult;

FitResult *free_list_first_fit(size_t req_size);
void free_list_init(void *mem, size_t mem_size);
void free_list_allocate_block(CommonHeader *block, size_t req_size);
void free_list_insert_head(CommonHeader *block);
void free_list_dump(void);
size_t free_list_total_free_size(void);
