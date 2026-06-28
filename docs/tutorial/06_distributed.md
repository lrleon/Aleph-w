# 06 — Scaling up

When a single-threaded `step()` is too slow, the first lever is the
**multi-threaded engine**. Distributed (MPI) and GPU backends are on the
roadmap — see the note at the end.

## The parallel engine

`Parallel_Synchronous_Engine` is a drop-in replacement for `Synchronous_Engine`:
same lattice, rule and neighborhood, but it partitions each step across a thread
pool. Output is **bit-identical** to the sequential engine.

```cpp
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>

using namespace Aleph;
using namespace Aleph::CA;

using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

int main()
{
  L seed({2048, 2048}, 0);
  // ... seed ...

  Parallel_Engine_Config cfg;
  cfg.num_partitions     = 4;       // 0 = use the pool's thread count
  cfg.min_parallel_cells = 4096;    // below this, run single-threaded

  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> eng(
      seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  eng.run(100);
}
```

### When it pays off

The parallel engine has fixed per-step overhead (partitioning + join). It only
wins above `min_parallel_cells`: for a 2048² grid the speedup is substantial,
for a 32² grid the threading overhead dominates and it falls back to sequential
automatically. The [performance guide](../performance.md) has measured numbers
and tuning advice for `num_partitions` and tiling.

### Correctness guarantee

Because the rule is pure and the engine is double-buffered, partitions never
race: each reads the previous frame and writes a disjoint slice of the next
one. The test suite asserts the parallel and sequential engines produce
identical frames for the same seed.

## Checkpoint, observe, visualize — unchanged

Everything from chapters 03–05 works the same with the parallel engine:
`attach_observer`, `save_checkpoint`, the frame sinks — they all take any engine
satisfying the engine concept.

## Roadmap: distributed and GPU

These backends are **planned but not yet implemented**
(see `cell-automata-industrial-roadmap.md`):

- **MPI domain decomposition** (Phase 22) — partition the grid across nodes with
  non-blocking halo exchange, for grids that exceed a single machine's memory.
- **CUDA / SYCL GPU backend** (Phase 23) — offload the per-cell kernel to the
  GPU for large 2D/3D grids.

Until then, the multi-threaded engine above is the supported way to use more
than one core. For grids that fit in RAM it scales well across a single
machine's cores.

## Where to go next

- [Performance tuning guide](../performance.md) — layouts, tiling, parallelism.
- [Visual gallery](../gallery/index.md) — every built-in rule rendered.
- [`Examples/`](../../Examples/) and `Tests/ca_*` — complete, compilable code.