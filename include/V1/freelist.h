#pragma once
#include <stddef.h>

typedef struct common_header {
    size_t size;
    struct common_header *next;
} CommonHeader;

extern CommonHeader* free_list_head;

typedef struct {
    CommonHeader *prev;
    CommonHeader *curr;
} FitResult;

typedef struct {
    size_t free_memory;
    size_t block_count;
    size_t largest_block;
} FreeListStats;



FitResult free_list_first_fit(size_t req_size);
void free_list_init(void *mem, size_t mem_size);
void free_list_allocate_block(FitResult block, size_t req_size);
void free_list_insert_head(CommonHeader *block);
void free_list_dump(void);
size_t free_list_total_free_size(void);
FreeListStats get_free_list_stats(void);