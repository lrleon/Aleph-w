cmake_minimum_required(VERSION 3.18)

# Informative benchmarks for dynamic spatial indexes. These are not wired to
# the CA performance gate; they document RTree/RStarTree trade-offs against
# brute-force scans and the static AABBTree.
set(SPATIAL_BENCHMARK_PROGRAMS
    bench_r_tree
)

foreach(program IN LISTS SPATIAL_BENCHMARK_PROGRAMS)
    add_executable(${program} "${program}.cc")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
endforeach()

add_custom_target(spatial_benchmarks
    DEPENDS ${SPATIAL_BENCHMARK_PROGRAMS}
)

message(STATUS "Configured spatial-index benchmarks")
