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

typedef struct {
    size_t free_memory;
    size_t block_count;
    size_t largest_block;
} FreeListStats;



FitResult free_list_first_fit(CommonHeader *head, size_t req_size);
FitResult free_list_best_fit(CommonHeader *head, size_t req_size);
void free_list_init(CommonHeader **head, void *mem, size_t mem_size);
void free_list_allocate_block(CommonHeader **head, FitResult fit, size_t req_size);
void free_list_insert_head(CommonHeader **head, CommonHeader *block);
void free_list_insert_and_merge(CommonHeader **head, CommonHeader *block);
void free_list_dump(CommonHeader *head);
size_t free_list_total_free_size(CommonHeader *head);
void get_free_list_stats(CommonHeader *head, FreeListStats *stats);
