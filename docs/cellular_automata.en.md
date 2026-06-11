# Cellular Automata Module

`Aleph::CA` is Aleph-w's C++20 module for building, running, measuring and
exporting cellular automata. The public API follows the project's `.H`
convention and is covered by focused GoogleTest binaries and portable examples.

## Architecture

The module is organized as a small stack of independent layers:

```text
Storage -> Lattice -> Neighborhood -> Rule -> Engine -> Observers / Exporters
```

- **Storage**: dense and bit-packed cell buffers.
- **Lattice**: boundary-aware grids, ghost layers, hexagonal and triangular
  lattices, and graph-based automata.
- **Neighborhood**: Moore, Von Neumann, hexagonal, triangular and custom
  offset sets.
- **Rule**: totalistic, outer-totalistic, lookup, stochastic, continuous and
  composite transitions.
- **Engine**: synchronous double-buffered engines, parallel engines and
  Hashlife for binary outer-totalistic automata.
- **Observers and exporters**: metrics, cycle detection, step hooks, Life/RLE,
  CSV, JSON, PPM/PGM, TikZ, SVG, PNG, GIF, VTK, NPY, HTML, DOT, ASCII and
  frame sinks.

## Minimal Game of Life

```cpp
#include <ca-engine-utils.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>

#include <cstddef>
#include <utility>

using namespace Aleph::CA;

int main()
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

  Grid grid({ 8, 8 }, 0);
  grid.set({ 1, 2 }, 1);
  grid.set({ 2, 3 }, 1);
  grid.set({ 3, 1 }, 1);
  grid.set({ 3, 2 }, 1);
  grid.set({ 3, 3 }, 1);

  Engine engine(std::move(grid), make_game_of_life_rule(), Moore<2, 1>{});
  for (std::size_t step = 0; step < 5; ++step)
    engine.step();
}
```

## Main Headers

| Area | Headers |
|---|---|
| Concepts and traits | `tpl_ca_concepts.H`, `ca-traits.H` |
| Storage and lattices | `tpl_ca_storage.H`, `tpl_ca_bit_storage.H`, `tpl_ca_lattice.H`, `tpl_ca_ghost_lattice.H` |
| Special topologies | `tpl_ca_hex_lattice.H`, `tpl_ca_triangular_lattice.H`, `tpl_ca_graph_automaton.H` |
| Neighborhoods and rules | `tpl_ca_neighborhood.H`, `tpl_ca_rule.H`, `tpl_ca_stochastic_rules.H`, `tpl_ca_continuous_rules.H` |
| Engines | `tpl_ca_engine.H`, `tpl_ca_parallel_engine.H`, `tpl_ca_hashlife.H`, `ca-engine-utils.H` |
| Observability | `ca-observer.H`, `ca-metrics.H`, `ca-rng.H`, `ca-kernels.H`, `ca-tiling.H`, `ca-bench.H` |
| IO and visualization | `ca-io.H`, `ca-tikz.H`, `ca-frame-stream.H`, `ca-png.H`, `ca-gif.H`, `ca-svg.H`, `ca-npy.H`, `ca-vtk.H`, `ca-html-player.H`, `ca-ffmpeg-sink.H`, `ca-dot.H`, `ca-ascii.H`, `ca-x11-viewer.H` |

## Build and Test

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build -R 'CA|ca_|tpl_ca' --output-on-failure
```

Useful examples include `ca_game_of_life_example.cc`,
`ca_wolfram1d_example.cc`, `ca_parallel_gol_example.cc`,
`ca_sir_epidemic_example.cc`, `ca_forest_fire_example.cc`,
`ca_ising_magnetisation_example.cc`, `ca_gray_scott_example.cc`,
`ca_hashlife_example.cc` and `ca_visualization_gallery_example.cc`.

## Design Notes

- Synchronous engines use double buffering; rules should treat the current
  frame as immutable during a step.
- Boundary semantics are explicit through policy tags.
- Stochastic rules use reproducible RNG plumbing so results are independent of
  thread scheduling.
- External tools are optional. Base exporters write directly; the ffmpeg sink
  detects availability at runtime.
- The X11 live viewer is interactive and requires a valid `DISPLAY`.
