cmake_minimum_required(VERSION 3.20)
project(Dynamic_Memory LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(app src/main.c)

enable_testing()

find_package(PkgConfig REQUIRED)
pkg_check_modules(Criterion REQUIRED IMPORTED_TARGET criterion)

add_executable(tests tests/tests.c)
target_link_libraries(tests PRIVATE PkgConfig::Criterion)
add_test(NAME tests COMMAND tests)

add_executable(benchmarks benchmarks/benchmarks.c)
target_link_libraries(benchmarks PRIVATE PkgConfig::Criterion)

