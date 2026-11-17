cmake_minimum_required(VERSION 3.20)
project(Dynamic_Memory LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

include(CTest)

add_library(dynamic_memory STATIC
    src/allocator.c
    src/freelist.c
)

target_include_directories(dynamic_memory
    PUBLIC include
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Criterion REQUIRED IMPORTED_TARGET criterion)

add_executable(tests tests/tests.c)
target_link_libraries(tests PRIVATE dynamic_memory PkgConfig::Criterion)
add_test(NAME tests COMMAND tests)

add_executable(benchmarks benchmarks/benchmarks.c)
target_link_libraries(benchmarks PRIVATE dynamic_memory)

