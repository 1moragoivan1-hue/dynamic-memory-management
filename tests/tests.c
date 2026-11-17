#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include "allocator.h"

#define RANDOM_ALLOCATION_COUNT 16

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
        void *memory = smalloc(request_size);

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

    printf("\nFree list: ");
    allocator_list_dump();

    size_t remaining_free_memory = allocator_free_mem_size();
    char allocation_status[128];
    char failure_report[128];

    if (allocation_failed) {
        snprintf(
            allocation_status,
            sizeof(allocation_status),
            "ALLOCATION ATTEMPT FAILED AFTER %zu ALLOCATIONS",
            successful_allocations
        );
        snprintf(
            failure_report,
            sizeof(failure_report),
            "Allocation Required at Failure: %zu bytes",
            allocator_req_mem(failed_request_size)
        );
    } else {
        snprintf(
            allocation_status,
            sizeof(allocation_status),
            "ALLOCATION DONE WITH NO FAILURES"
        );
        snprintf(
            failure_report,
            sizeof(failure_report),
            "Allocation Required at Failure: N/A"
        );
    }

    printf(
        "\n%s\n"
        "%s\n"
        "Total Memory Size: %zu bytes\n"
        "Memory Used (data + headers): %zu bytes\n"
        "Remaining Free Memory: %zu bytes\n\n",
        allocation_status,
        failure_report,
        (size_t)MEMORY_SIZE,
        (size_t)MEMORY_SIZE - remaining_free_memory,
        remaining_free_memory
    );

    for (size_t i = 0; i < 3; ++i) {
        sfree(slots[i]);
    }

    cr_assert(!allocation_failed);
}