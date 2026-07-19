cmake_minimum_required(VERSION 3.18)

# Performance benchmarks live under per-domain subdirectories: the
# cellular-automata perf-gate anchors and informative comparison suites.
add_subdirectory(ca)
add_subdirectory(containers)
add_subdirectory(spatial)
