cmake_minimum_required(VERSION 3.18)

# Performance benchmarks live under per-domain subdirectories: the
# cellular-automata perf-gate anchors and the informative flat-container
# comparisons. Future domains can add their own folder.
add_subdirectory(ca)
add_subdirectory(containers)