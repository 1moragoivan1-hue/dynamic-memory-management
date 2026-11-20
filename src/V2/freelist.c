#include <stdio.h>
#include <stdint.h>
#include "freelist.h"

#define HEADER_SIZE sizeof(CommonHeader)

void free_list_init (CommonHeader **head, void *mem, size_t mem_size) {
    *head = mem;
    (*head)->size = mem_size - HEADER_SIZE;
    (*head)->next = NULL;
}

FitResult free_list_first_fit (CommonHeader *head, size_t req_size) {
    CommonHeader *curr = head;
    CommonHeader *prev = NULL;
    while (curr) {
        if (curr->size >= req_size) return (FitResult){prev,curr};
        prev = curr;
        curr = curr->next;        
    }
    return (FitResult){NULL, NULL};
}
FitResult free_list_best_fit (CommonHeader *head, size_t req_size) {
    CommonHeader *curr = head;
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

static void replace_node(CommonHeader **head, CommonHeader *prev, CommonHeader *replacement) {
    if (prev == NULL) {
        *head = replacement;
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

void free_list_allocate_block(CommonHeader **head, FitResult fit, size_t req_size) {
    CommonHeader *prev = fit.prev;
    CommonHeader *block = fit.curr;
    if (!block) return;

    size_t remaining = block->size - req_size;

    if (remaining >= HEADER_SIZE + 1) {
        CommonHeader *remainder = split_block(block, req_size);
        replace_node(head, prev, remainder);
    } 
    else {
        replace_node(head, prev, block->next);
    }

    block->next = NULL;
}

void free_list_insert_head(CommonHeader **head, CommonHeader *block) {
    if (!block) return;
    block->next = *head;
    *head = block;
}

static int blocks_touch(const CommonHeader *left, const CommonHeader *right) {
    return (uintptr_t)right - (uintptr_t)left == HEADER_SIZE + left->size;
}

static void merge_with_next(CommonHeader *block) {
    CommonHeader *next = block->next;
    block->size += HEADER_SIZE + next->size;
    block->next = next->next;
}

void free_list_insert_and_merge(CommonHeader **head, CommonHeader *block) {
    CommonHeader *prev = NULL;
    CommonHeader *curr = *head;

    while (curr && ((uintptr_t)curr < (uintptr_t)block)) {
        prev = curr;
        curr = curr->next;
    }

    block->next = curr;
    replace_node(head, prev, block);

    if (curr && blocks_touch(block, curr)) merge_with_next(block);
    if (prev && blocks_touch(prev, block)) merge_with_next(prev);

}

void free_list_dump(CommonHeader *head) {
    CommonHeader *curr = head;
    while (curr != NULL) {
        printf("[%zu]", curr->size);
        if (curr->next != NULL) {
            printf(" -> ");
        }
        curr = curr->next;
    }
    printf("\n");
}

size_t free_list_total_free_size(CommonHeader *head) {
    size_t total = 0;
    CommonHeader *curr = head;
    while (curr != NULL) {
        total += curr->size;
        curr = curr->next;
    }
    return total;
}

void get_free_list_stats(CommonHeader *head, FreeListStats *stats) {
    CommonHeader *block = head;

    while (block) {
        if (block->size > stats->largest_block) {
            stats->largest_block = block->size;
        }
        stats->block_count++;
        stats->free_memory += block->size;

        block = block->next;
    }
}
