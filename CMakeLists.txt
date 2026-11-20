cmake_minimum_required(VERSION 3.20)
project(Dynamic_Memory LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

include(CTest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Criterion REQUIRED IMPORTED_TARGET criterion)

foreach(ver V1 V2)
    add_library(dynamic_memory_${ver} STATIC
        src/${ver}/allocator.c
        src/${ver}/freelist.c
    )
    target_include_directories(dynamic_memory_${ver} PUBLIC include/${ver})

    add_executable(tests_${ver} tests/tests.c)
    target_link_libraries(tests_${ver} PRIVATE dynamic_memory_${ver} PkgConfig::Criterion)
    add_test(NAME tests_${ver} COMMAND tests_${ver})

    add_executable(stress_test_${ver} tests/stress_test.c)
    target_link_libraries(stress_test_${ver} PRIVATE dynamic_memory_${ver})
    add_test(NAME stress_test_${ver} COMMAND stress_test_${ver})

    add_executable(benchmarks_${ver} benchmarks/benchmarks.c)
    target_link_libraries(benchmarks_${ver} PRIVATE dynamic_memory_${ver})
    
endforeach()


