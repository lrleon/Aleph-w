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

add_custom_target(graph_benchmarks
    DEPENDS ${GRAPH_BENCHMARK_PROGRAMS}
)

message(STATUS "Configured graph benchmarks")
