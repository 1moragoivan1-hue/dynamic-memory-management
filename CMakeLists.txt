cmake_minimum_required(VERSION 3.20)
project(Dynamic_Memory LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

include(CTest)

add_library(dynamic_memory_v1 STATIC
    src/V1/allocator.c
    src/V1/freelist.c
)

target_include_directories(dynamic_memory_v1
    PUBLIC include/V1
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Criterion REQUIRED IMPORTED_TARGET criterion)

add_executable(tests tests/tests.c)
target_link_libraries(tests PRIVATE dynamic_memory_v1 PkgConfig::Criterion)
add_test(NAME tests COMMAND tests)

add_executable(benchmarks benchmarks/benchmarks.c)
target_link_libraries(benchmarks PRIVATE dynamic_memory_v1)

