# 01 — Your first cellular automaton

Goal: build, run and visualize Conway's Game of Life in about 15 minutes.

## The three ingredients

Every CA in Aleph-w is an **engine** parameterized by three things:

1. a **lattice** — the grid and its boundary policy;
2. a **rule** — the per-cell transition function;
3. a **neighborhood** — which cells the rule sees.

For Game of Life those are a 2D toroidal integer lattice, the canonical
`Game_Of_Life_Rule` (B3/S23) and a Moore radius-1 neighborhood.

## The shortest path

`ca-engine-utils.H` ships one-liners for the common cases:

```cpp
#include <iostream>
#include <ca-engine-utils.H>   // make_gol_engine
#include <ca-ascii.H>          // render_ascii

using namespace Aleph;
using namespace Aleph::CA;

int main()
{
  // A 5x5 toroidal Game of Life, initially empty.
  auto eng = make_gol_engine(5, 5);

  eng.run(10);                 // advance 10 generations
  render_ascii(std::cout, eng.frame());
  std::cout << "steps run: " << eng.steps_run() << '\n';
}
```

`run(n)` advances `n` generations; `step()` advances one; `frame()` returns a
read-only reference to the current lattice; `steps_run()` is the generation
counter.

## Seeding a pattern

`make_gol_engine(rows, cols)` starts empty. To stamp an initial pattern, build
the lattice yourself and use the seeding overload:

```cpp
#include <ca-engine-utils.H>
#include <ca-ascii.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

int main()
{
  Grid g({5, 5}, 0);           // 5x5, every cell = 0
  g.set({2, 1}, 1);            // a horizontal blinker
  g.set({2, 2}, 1);
  g.set({2, 3}, 1);

  auto eng = make_gol_engine(std::move(g));

  for (int t = 0; t < 4; ++t)
    {
      render_ascii(std::cout, eng.frame());
      std::cout << "----\n";
      eng.step();
    }
}
```

Coordinates are `{row, col}`. `set` writes a cell; `at` reads one. The blinker
oscillates with period 2 — you will see it flip between horizontal and vertical.

## What just happened

- The lattice is **toroidal**: cell `{-1, 0}` wraps to the last row. Other
  boundary policies (`OpenBoundary`, `ReflectiveBoundary`, …) are drop-in
  type parameters — see [chapter 02](02_custom_rule.md).
- The engine is **double-buffered**: `step()` computes the whole next frame
  from the current one, so updates are synchronous (every cell sees the same
  previous generation).

## A 1D automaton in one line

Elementary Wolfram rules need no extra setup:

```cpp
#include <ca-engine-utils.H>   // make_wolfram_engine

auto wolf = Aleph::CA::make_wolfram_engine(/*rule=*/30, /*width=*/201);
wolf.step();
```

Rendering a 1D rule as a space-time image (one row per generation) is exactly
what the [gallery generator](../../Examples/ca_gallery_gen.cc) does.

## Next

[Chapter 02 — Custom rules](02_custom_rule.md): change B3/S23 into any
Life-like rule, then write a transition function from scratch.