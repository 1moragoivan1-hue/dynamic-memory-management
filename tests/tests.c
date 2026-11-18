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

Test(dynamic_memory, returns_null_when_full)
{
    cr_assert_eq(smalloc(MEM_SIZE + 1), NULL);
}

