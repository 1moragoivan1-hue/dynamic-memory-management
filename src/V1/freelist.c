#include <stdio.h>
#include <stdint.h>
#include "freelist.h"

#define HEADER_SIZE sizeof(CommonHeader)

CommonHeader* free_list_head = NULL;

void free_list_init (void *mem, size_t mem_size) {
    free_list_head  = (CommonHeader*)mem;
    free_list_head->size = mem_size - HEADER_SIZE;
    free_list_head->next = NULL;
}

FitResult free_list_first_fit (size_t req_size) {
    CommonHeader *curr = free_list_head;
    CommonHeader *prev = NULL;
    while (curr) {
        if (curr->size >= req_size) return (FitResult){prev,curr};
        prev = curr;
        curr = curr->next;        
    }
    return (FitResult){NULL, NULL};
}
FitResult free_list_best_fit (size_t req_size) {
    CommonHeader *curr = free_list_head;
    CommonHeader *prev = NULL;
    FitResult best = {NULL, NULL};
    while (curr) {
        if ((curr->size >= req_size) &&
        (best.curr == NULL || curr->size < best.curr->size)){
            best.curr = curr;
            best.prev = prev;
        }
        prev = curr;
        curr = curr->next;
    }
    return best;
}

static void *get_payload_ptr(CommonHeader *block) {
    return (void *)((char *)block + HEADER_SIZE);
}

static void replace_node(CommonHeader *prev, CommonHeader *replacement) {
    if (prev == NULL) {
        free_list_head = replacement;
    } 
    else {
        prev->next = replacement;
    }
}

static CommonHeader *split_block(CommonHeader *block, size_t req_size) {
    size_t remaining = block->size - req_size;

    CommonHeader *remainder = (CommonHeader *)((char *)get_payload_ptr(block) + req_size);
    remainder->size = remaining - HEADER_SIZE;
    remainder->next = block->next;

    block->size = req_size;
    return remainder;
}

void free_list_allocate_block(FitResult fit, size_t req_size) {
    CommonHeader *prev = fit.prev;
    CommonHeader *block = fit.curr;
    if (!block) return;

    size_t remaining = block->size - req_size;

    if (remaining >= HEADER_SIZE + 1) {
        CommonHeader *remainder = split_block(block, req_size);
        replace_node(prev, remainder);
    } 
    else {
        replace_node(prev, block->next);
    }

    block->next = NULL;
}

void free_list_insert_head(CommonHeader *block) {
    if (!block) return;
    block->next = free_list_head;
    free_list_head = block;
}

static int blocks_touch(const CommonHeader *left, const CommonHeader *right) {
    return (uintptr_t)right - (uintptr_t)left == HEADER_SIZE + left->size;
}

static void merge_with_next(CommonHeader *block) {
    CommonHeader *next = block->next;
    block->size += HEADER_SIZE + next->size;
    block->next = next->next;
}

void free_list_insert_and_merge(CommonHeader *block) {
    CommonHeader *prev = NULL;
    CommonHeader *curr = free_list_head;

    while (curr && ((uintptr_t)curr < (uintptr_t)block)) {
        prev = curr;
        curr = curr->next;
    }

    block->next = curr;
    replace_node(prev, block);

    if (curr && blocks_touch(block, curr)) merge_with_next(block);
    if (prev && blocks_touch(prev, block)) merge_with_next(prev);

}

void free_list_dump(void) {
    CommonHeader *curr = free_list_head;
    while (curr != NULL) {
        printf("[%zu]", curr->size);
        if (curr->next != NULL) {
            printf(" -> ");
        }
        curr = curr->next;
    }
    printf("\n");
}

size_t free_list_total_free_size(void) {
    size_t total = 0;
    CommonHeader *curr = free_list_head;
    while (curr != NULL) {
        total += curr->size;
        curr = curr->next;
    }
    return total;
}

void get_free_list_stats(FreeListStats *stats) {
    CommonHeader *block = free_list_head;

    while (block) {
        if (block->size > stats->largest_block) {
            stats->largest_block = block->size;
        }
        stats->block_count++;
        stats->free_memory += block->size;

        block = block->next;
    }
}
