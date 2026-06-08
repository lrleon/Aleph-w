cmake_minimum_required(VERSION 3.18)

# Performance benchmarks live under per-domain subdirectories. Today only the
# cellular-automata anchors exist; future domains can add their own folder.
add_subdirectory(ca)