# Cellular-Automata Tutorial

A six-chapter, hands-on introduction to the Aleph-w cellular-automata module.
Read in order; each chapter builds on the previous one. All code is C++20 and
compiles against the flat headers in the repository root.

| # | Chapter | You will learn |
|---|---------|----------------|
| 01 | [Your first CA](01_first_ca.md) | Build and run Conway's Game of Life in ~15 minutes |
| 02 | [Custom rules](02_custom_rule.md) | Write Life-like and fully custom transition rules |
| 03 | [Visualization](03_visualization.md) | Export frames to PNG / GIF / SVG |
| 04 | [Observability](04_observability.md) | Attach observers and read metrics |
| 05 | [Long-running simulations](05_long_running.md) | Checkpoint, compress and resume |
| 06 | [Scaling up](06_distributed.md) | Multi-threaded engine (and the MPI/GPU roadmap) |

See also:

- [Visual gallery](../gallery/index.md) — one rendered PNG per built-in rule.
- [Performance tuning guide](../performance.md) — layout, tiling and parallelism.
- [`Examples/`](../../Examples/) — complete, compilable programs.

## Building the examples

```bash
cmake -S . -B build -G Ninja -DBUILD_EXAMPLES=ON
cmake --build build
```

Every snippet in this tutorial corresponds to a buildable example or a test in
`Tests/ca_*`.