cmake_minimum_required(VERSION 3.20)
project(Dynamic_Memory LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

include(CTest)

add_library(dynamic_memory_V1 STATIC
    src/V1/allocator.c
    src/V1/freelist.c
)

target_include_directories(dynamic_memory_V1
    PUBLIC include/V1
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Criterion REQUIRED IMPORTED_TARGET criterion)

# Add a version here once its dynamic_memory_<ver> library target exists
foreach(ver V1)
    add_executable(tests_${ver} tests/tests.c)
    target_link_libraries(tests_${ver} PRIVATE dynamic_memory_${ver} PkgConfig::Criterion)
    add_test(NAME tests_${ver} COMMAND tests_${ver})

    add_executable(stress_test_${ver} tests/stress_test.c)
    target_link_libraries(stress_test_${ver} PRIVATE dynamic_memory_${ver})
    add_test(NAME stress_test_${ver} COMMAND stress_test_${ver})

    add_executable(benchmarks_${ver} benchmarks/benchmarks.c)
    target_link_libraries(benchmarks_${ver} PRIVATE dynamic_memory_${ver})
    
endforeach()


