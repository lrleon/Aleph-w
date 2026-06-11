cmake_minimum_required(VERSION 3.18)

# Anchor benchmarks for the cellular-automata performance-regression gate
# (roadmap Phase 20). Each program times one kernel and prints a single line
# of structured JSON consumed by ../perf_gate.py.
set(CA_BENCHMARK_PROGRAMS
    bench_gol_1024
    bench_wolfram_30_1d
    bench_gray_scott_512
    bench_parallel_gol
    bench_hashlife_breeder
    bench_multi_field_lv
)

foreach(program IN LISTS CA_BENCHMARK_PROGRAMS)
    add_executable(${program} "${program}.cc")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
endforeach()

add_custom_target(ca_benchmarks
    DEPENDS ${CA_BENCHMARK_PROGRAMS}
)

message(STATUS "Configured cellular-automata performance-gate benchmarks")