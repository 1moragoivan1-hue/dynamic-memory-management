#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include "allocator.h"

#define RANDOM_ALLOCATION_COUNT 45

static size_t random_request_size(size_t max_request_size)
{
    return (size_t)(rand() % max_request_size) + 1;
}

Test(dynamic_memory, request_size_includes_payload)
{
    size_t header_size = allocator_req_mem(0);

    cr_assert_eq(allocator_req_mem(32), header_size + 32);
}

Test(dynamic_memory, allocates_memory)
{
    unsigned char *memory = smalloc(32);

    cr_assert_not_null(memory);

    memory[0] = 0xAA;
    memory[31] = 0x55;

    cr_assert_eq(memory[0], 0xAA);
    cr_assert_eq(memory[31], 0x55);

    sfree(memory);
}

Test(dynamic_memory, null_free_is_safe)
{
    sfree(NULL);
    cr_assert(1);
}

Test(dynamic_memory, handles_randomized_allocations)
{
    void *slots[3] = {NULL, NULL, NULL};
    size_t successful_allocations = 0;
    size_t failed_request_size = 0;
    int allocation_failed = 0;
    const int max_request_size = 16;

    srand(1);

    for (size_t i = 0; i < RANDOM_ALLOCATION_COUNT; ++i) {
        size_t slot = i % 3;
        size_t request_size = random_request_size(max_request_size);
        void *memory = smalloc(random_request_size(max_request_size));

        if (memory == NULL) {
            allocation_failed = 1;
            failed_request_size = request_size;
            break;
        }

        if (slots[slot] != NULL) {
            sfree(slots[slot]);
        }
        slots[slot] = memory;
        successful_allocations++;

        if ((i + 1) % 3 == 0) {
            size_t slot_to_free = (size_t)(rand() % 3);

            if (slots[slot_to_free] != NULL) {
                sfree(slots[slot_to_free]);
                slots[slot_to_free] = NULL;
            }
        }
    }

    for (size_t i = 0; i < 3; ++i) {
        sfree(slots[i]);
    }

    printf("\nFree list: ");
    allocator_list_dump();

    size_t remaining_free_memory = allocator_free_mem_size();

    if (allocation_failed) {
        printf("\nALLOCATION ATTEMPT FAILED AFTER %zu ALLOCATIONS\n",
               successful_allocations);
        printf("Allocation Required at Failure: %zu bytes\n",
               allocator_req_mem(failed_request_size));
    } else {
        printf("\nALLOCATION DONE WITH NO FAILURES\n");
    }

    printf("Remaining Free Memory: %zu bytes\n", remaining_free_memory);

    cr_assert(!allocation_failed);
}