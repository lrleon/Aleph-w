# Performance tuning guide

Practical advice for getting the most out of the cellular-automata module:
choosing a memory layout, an iteration order, and when to go parallel. The
numbers at the end come from the perf-gate baseline.

## 1. Memory layout: AoS vs SoA

Multi-field lattices (`tpl_ca_multi_field_lattice.H`) take a layout tag:

- **`Layout_AoS`** (Array of Structs) — each cell is one struct holding all
  fields contiguously. Good when a rule reads *most* fields of a cell together
  (the whole cell is one cache line fetch).
- **`Layout_SoA`** (Struct of Arrays) — each field is its own contiguous array.
  Good when a rule sweeps *one field at a time* across the grid, and essential
  for SIMD/vectorization (a field's values are packed with unit stride).

Rule of thumb:

| Access pattern | Prefer |
|----------------|--------|
| Reaction-diffusion / per-field stencils (Gray-Scott, Lotka-Volterra) | `Layout_SoA` |
| Agent-like cells where the rule touches all fields at once | `Layout_AoS` |
| Unsure, want vectorization | `Layout_SoA` |

The Lotka-Volterra benchmark below uses `Layout_SoA` precisely because each
species field is updated in its own pass.

## 2. Iteration order: RowMajor vs Tile

The engine's `Order` template parameter controls cell visitation:

- **`RowMajor`** (default) — simple, cache-friendly for grids whose row fits
  comfortably in L2. Best default.
- **`ColumnMajor`** — rarely useful; only when your storage is transposed.
- **`Tile<W, H>`** — visit the grid in `W×H` blocks. This improves temporal
  locality for **radius-r neighborhoods on large grids**: a tile's working set
  (the tile plus its halo) stays hot in cache across the whole block instead of
  streaming a full row's neighborhood per cell.

### Choosing W and H

- Pick a tile whose working set fits in **L1/L2**. For an `int` cell and a
  Moore radius-1 stencil, a `Tile<32, 32>` touches ~`(32+2)² · 4 B ≈ 4.5 KB`
  of current frame plus the same for the next frame — comfortably in L1.
- Powers of two (`16`, `32`, `64`) keep index arithmetic cheap and align with
  cache lines.
- Tiling only helps once the grid is much larger than cache (≳ 1024²). Below
  that, `RowMajor` already keeps everything hot and tiling just adds loop
  overhead.

Start with `RowMajor`; switch to `Tile<32, 32>` and measure only if profiling
shows memory stalls on a large grid.

## 3. When to go parallel

`Parallel_Synchronous_Engine` (chapter 06) partitions each step across a thread
pool, producing bit-identical output. It has fixed per-step overhead
(partition + join), so:

- **`min_parallel_cells`** — below this cell count the engine runs
  single-threaded automatically. The default (4096) means tiny grids never pay
  the threading tax. Leave it unless you have measured otherwise.
- **`num_partitions`** — `0` uses the thread pool's size (usually a good
  default). More partitions improve load balance for irregular activity but
  raise join overhead; rarely set it above the core count.

Heuristic: parallelism pays off from roughly **256² cells upward** with a
non-trivial rule. For 1D rules or tiny grids, stay sequential.

## 4. Reference-machine numbers

From `benchmarks/baseline.json` — **Intel Core Ultra 7 265KF, gcc 13.3.0,
Release** (captured 2026-06-08). These are the perf-gate anchors; reproduce with
the targets in `benchmarks/ca/`.

| Benchmark | Configuration | Throughput (cells/s) |
|-----------|---------------|---------------------:|
| `gol_1024` | GoL 1024² toroidal, sequential | 30.6 M |
| `parallel_gol_2048` | GoL 2048², parallel engine | 419.9 M |
| `gray_scott_512` | Gray-Scott 512², 1k steps | 35.8 M |
| `multi_field_lv_512` | Lotka-Volterra 512² (SoA) | 30.6 M |
| `wolfram_30_1d` | Wolfram 30, 1D, 1M cells | 246.1 M |
| `hashlife_breeder` | Hashlife, 2²⁰ generations | 0.51 M* |

\* Hashlife's cells/second is not comparable to the dense engines: it counts
logical cells while doing exponentially less work via memoized quadtree nodes.

The parallel engine reaches ~420 M cells/s on a 2048² grid — an order of
magnitude over the sequential dense engines — which is exactly the regime where
its fixed overhead is amortized.

### How to measure your own

```bash
cmake -S . -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build-bench
./build-bench/benchmarks/ca/bench_gol_1024     # prints JSON: cells_per_sec, wall_ns
```

Always benchmark in **Release** with `-fno-strict-aliasing` (the build sets this
automatically — it is required by `htlist.H`).