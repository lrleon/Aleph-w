cmake_minimum_required(VERSION 3.18)

# Informative benchmarks for graph algorithms. Not wired to the performance
# gate; they document trade-offs between alternative implementations.
set(GRAPH_BENCHMARK_PROGRAMS
    bench_negative_cycles
)

foreach(program IN LISTS GRAPH_BENCHMARK_PROGRAMS)
    add_executable(${program} "${program}.cc")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
endforeach()

# bench_negative_cycles prints the build type and flags next to its timings so
# a result can be reproduced; single-configuration generators only.
string(TOUPPER "${CMAKE_BUILD_TYPE}" _bench_config)
set(_bench_flags "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_bench_config}}")
string(STRIP "${_bench_flags}" _bench_flags)
# The project flags carry quoted -D values that cannot be nested in a string
# literal: show them with single quotes.
string(REPLACE "\"" "'" _bench_flags "${_bench_flags}")
string(REPLACE "\\" "/" _bench_flags "${_bench_flags}")
set(_bench_type "${CMAKE_BUILD_TYPE}")
if(NOT _bench_type)
    set(_bench_type "unspecified")
endif()
target_compile_definitions(bench_negative_cycles PRIVATE
    "BENCH_BUILD_TYPE=\"${_bench_type}\""
    "BENCH_CXX_FLAGS=\"${_bench_flags}\"")

add_custom_target(graph_benchmarks
    DEPENDS ${GRAPH_BENCHMARK_PROGRAMS}
)

# Validation pass of bench_negative_cycles (no timing, about a second): checks
# that every algorithm returns valid witnesses on every graph family. Known
# Bellman-Ford extraction defects are reported but do not fail it.
if(BUILD_TESTS)
    add_test(NAME bench_negative_cycles_check
             COMMAND bench_negative_cycles 60 1 42 --check-only)
endif()

message(STATUS "Configured graph benchmarks")
