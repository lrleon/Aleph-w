# 02 — Custom rules

You rarely need Conway's exact rule. This chapter shows the two ways to write
your own transition function.

## Way 1: Life-like (outer-totalistic) rules

A huge family of interesting automata only depends on the cell's own state and
the **count** of live neighbors — the "B/S" (birth/survival) rules. Use
`Outer_Totalistic_Rule<F>`, whose functor is `F(state, alive_count) -> state`:

```cpp
#include <ca-engine-utils.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

int main()
{
  // HighLife: B36/S23 (born on 3 or 6 neighbors, survives on 2 or 3).
  auto highlife = [](int s, std::size_t n)
  {
    const bool birth   = (n == 3 or n == 6);
    const bool survive = (n == 2 or n == 3);
    return ((s == 0 and birth) or (s != 0 and survive)) ? 1 : 0;
  };
  using Rule = Outer_Totalistic_Rule<decltype(highlife)>;

  Grid g({64, 64}, 0);
  // ... seed g ...
  Synchronous_Engine<Grid, Rule, Moore<2, 1>> eng(
      std::move(g), Rule(highlife), Moore<2, 1>{});

  eng.run(100);
}
```

Swap the predicate to get Seeds (`B2/S`), Day & Night (`B3678/S34678`), Maze
(`B3/S12345`) and so on. The [gallery generator](../../Examples/ca_gallery_gen.cc)
renders eight of these side by side.

## Way 2: a fully custom rule

When the transition needs the actual neighbor *values* (not just a count), write
a functor taking a `Neighbor_View`:

```cpp
#include <tpl_ca_neighborhood.H>   // Neighbor_View

struct Sandpile_Rule
{
  template <typename State>
  State operator()(const State &s, Neighbor_View<State> nh) const noexcept
  {
    constexpr State threshold = 4;
    State result = s;
    if (result >= threshold)
      result -= threshold;            // this cell topples
    for (const auto &n : nh)          // each over-threshold neighbor
      if (n >= threshold)             // pushes one grain in
        result += 1;
    return result;
  }
};
```

`Neighbor_View` is an iterable over the neighbor cells defined by the
neighborhood you pass to the engine. The rule must be **pure**: it may read `s`
and `nh` but must not mutate shared state, because the engine evaluates cells in
parallel-safe order against the previous frame.

Plug it into any engine the same way:

```cpp
using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
Synchronous_Engine<Grid, Sandpile_Rule, Moore<2, 1>> eng(
    std::move(g), Sandpile_Rule{}, Moore<2, 1>{});
```

## Choosing a neighborhood

- `Moore<2, 1>` — the 8 surrounding cells (radius 1, rank 2).
- `Von_Neumann<2, 1>` — the 4 orthogonal cells.
- `Custom_Neighborhood<Rank, K>` — an explicit list of offsets (this is what
  the 1D Wolfram engine uses with offsets `{-1}` and `{+1}`).

## Choosing a boundary

The boundary is a **type parameter of the lattice**, resolved by `at_safe`:

| Boundary | Behavior off-grid |
|----------|-------------------|
| `ToroidalBoundary` | wraps around |
| `OpenBoundary` | reads a fixed out-of-range value (0) |
| `ReflectiveBoundary` | mirrors back |
| `NeumannBoundary` | clamps to the nearest in-range cell |

Just change the lattice alias — the rule and engine code stay the same.

## Next

[Chapter 03 — Visualization](03_visualization.md): turn frames into images.