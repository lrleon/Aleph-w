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

## API reference

### `sort_ranked_moves()`

Sorts a materialized batch of ranked moves in-place using a multi-criteria
comparator.

```cpp
template <typename Move, typename Priority, typename BetterPriority>
void sort_ranked_moves(Array<RankedMove<Move, Priority>> &moves,
                       BetterPriority better_priority,
                       bool prefer_killer,
                       bool prefer_history);
```

**Parameters:**

| Parameter | Description |
|-----------|-------------|
| `moves` | Batch of `RankedMove<Move, Priority>` to sort in-place. |
| `better_priority` | Binary predicate `(Priority, Priority) -> bool` where `true` means the first priority is better. |
| `prefer_killer` | If `true`, moves with `.killer == true` are promoted ahead of non-killers. |
| `prefer_history` | If `true`, moves are secondarily ordered by descending `.history_score`. |

**Ordering (highest to lowest precedence):**

1. Killer moves first (if `prefer_killer`)
2. Greater `history_score` first (if `prefer_history`)
3. Better priority according to `better_priority`
4. Original domain order (`ordinal`) as deterministic tie-breaker

A batch of 0 or 1 elements is returned unchanged.

### `RankedMove<Move, Priority>`

One move plus the metadata used by ordering comparators.

| Field | Type | Description |
|-------|------|-------------|
| `move` | `Move` | The candidate move. |
| `priority` | `Priority` | Cheap heuristic priority (lower or higher depending on engine). |
| `ordinal` | `size_t` | Original order from the successor generator (tie-breaker). |
| `killer` | `bool` | `true` if this move matched a killer slot. |
| `history_score` | `size_t` | Accumulated history heuristic score. |

### `Killer_Move_Table<Move>`

Two-slot killer heuristic table indexed by search depth. Each depth keeps the
two most recent cutoff-producing moves. If `Move` is not
`std::equality_comparable`, the table degrades to a compile-time no-op.

| Member / method | Description |
|-----------------|-------------|
| `static constexpr bool supported` | `true` when `Move` is equality-comparable, `false` otherwise (no-op variant). |
| `clear()` | Remove all recorded killer moves across all depths. |
| `is_killer(depth, move) -> bool` | Returns `true` if `move` is in a killer slot for `depth`. |
| `record(depth, move)` | Record a move that caused a cutoff at `depth`. If the move is already in the primary slot, this is a no-op. Otherwise the current primary is demoted to secondary and the new move becomes primary. |

Internal storage grows lazily via `ensure_depth()` — recording at depth 100
allocates slots 0–100 if they do not already exist.

### `History_Heuristic_Table<Key>`

Sparse history heuristic table over Aleph hash maps. Accumulates a score for
each move key; moves with higher scores are promoted during ordering.

```cpp
template <typename Key,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<Key>>
class History_Heuristic_Table;
```

| Member / method | Description |
|-----------------|-------------|
| `static constexpr bool supported` | Always `true` (functional table). |
| `clear()` | Reset all history scores to zero. |
| `score(key) -> size_t` | Read the current cumulative score for a move key. Returns `0` for unknown keys. |
| `record(key, bonus = 1)` | Increment the history score for `key` by `bonus`. If the key is new, it is inserted with the given bonus. |

### `Null_History_Heuristic_Table`

No-op stub used when the domain does not expose `move_key()`. `clear()` and
`record()` are static no-ops, while `score(const Key &) const noexcept` is a
non-static const method that always returns `0`.

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
