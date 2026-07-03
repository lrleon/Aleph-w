cmake_minimum_required(VERSION 3.18)

# Informative benchmarks for the flat containers (FlatSet/FlatMap,
# SmallVector, RingBuffer) against their tree-based Aleph counterparts and
# the STL. Unlike benchmarks/ca these are not wired to the performance
# gate; they exist to document the trade-offs measured in v5.5.0.
set(CONTAINER_BENCHMARK_PROGRAMS
    bench_flat_containers
)

foreach(program IN LISTS CONTAINER_BENCHMARK_PROGRAMS)
    add_executable(${program} "${program}.cc")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
endforeach()

add_custom_target(container_benchmarks
    DEPENDS ${CONTAINER_BENCHMARK_PROGRAMS}
)

message(STATUS "Configured flat-container benchmarks")