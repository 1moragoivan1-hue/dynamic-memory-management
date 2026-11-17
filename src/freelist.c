#include <stdio.h>
#include "freelist.h"

#define HEADER_SIZE sizeof(CommonHeader)

CommonHeader* free_list_head = NULL;

void free_list_init (void *mem, size_t mem_size) {
    free_list_head  = (CommonHeader*)mem;
    free_list_head->size = mem_size;
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