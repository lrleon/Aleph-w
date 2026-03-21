# Move Ordering and Exploration Heuristics

## Module purpose

`search_move_ordering.H` provides the shared, lightweight support used by the
current search engines when they want to override the raw order emitted by
`for_each_successor()`.

The module contains:

- `MoveOrderingMode`
- `MoveOrderingStats`
- `Killer_Move_Table`
- `History_Heuristic_Table`
- `sort_ranked_moves()`

It does not decide by itself how to score a move. Each engine remains
responsible for choosing the priority it wants to attach to a successor.

## What is actually implemented

### Alpha-Beta

Alpha-Beta supports:

- `MoveOrderingMode::Estimated_Score`
  Each child is scored by a one-ply heuristic estimate:
  `-evaluate(child_state)`.

- optional killer heuristic
  Two recent cutoff-producing moves are kept per depth and promoted during
  ordering when the move type supports equality comparison.

- optional history heuristic
  A sparse score per move key is accumulated when a move causes a cutoff.
  This requires the domain to expose:

  ```cpp
  using Move_Key = ...;
  Move_Key move_key(const Move &) const;
  ```

This is the main practical improvement of the phase. The default behavior is
still domain order; ordering is enabled only when requested in
`ExplorationPolicy`.

### Branch and Bound

Depth-first branch and bound supports:

- `MoveOrderingMode::Estimated_Bound`
  Each child is ranked by `bound(child_state)` before exploration.

Best-first branch and bound already prioritizes frontier nodes globally through
the heap and the objective policy, so no extra local move-ordering pass is
performed there.

### Negamax

Negamax keeps the domain order. That choice is deliberate: ordering by itself
does not create pruning there, so paying the materialization/scoring cost would
add complexity with little practical return. If a user requests move ordering
on plain Negamax, the engine rejects the configuration explicitly.

## What is only left as extension

The following pieces are still intentionally deferred:

- iterative deepening plus PV-based reordering
- aspiration windows
- TT-aware move ordering
- full history decay/aging
- counter-move heuristics
- late move reductions or other aggressive selective search

These can be layered later on the same module without changing the current
domain contract.

## Why the design stops here

The goal of this phase was practical improvement without turning the framework
into a search-lab prototype.

The chosen level of complexity is:

- small enough to read and debug
- explicit in `ExplorationPolicy`
- cheap to disable completely
- reusable across engines with very different semantics

Anything more aggressive would have required tighter coupling between the
engines, more stateful policies, and a more complicated explanation of
correctness and performance tradeoffs.
