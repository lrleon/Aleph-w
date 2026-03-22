# Adversarial Search Framework

## Module purpose

- `Negamax.H`
  Base adversarial engine for two-player, zero-sum, alternating-turn games.
  It defines the shared adversarial vocabulary: domain contract, result type,
  principal variation and statistics.

- `Alpha_Beta.H`
  Adds Alpha-Beta pruning on top of the same Negamax recurrence and the same
  domain/result contract. It now also exposes iterative deepening and
  aspiration-window control on top of the same engine.

- `Transposition_Table.H`
  Provides the reusable memo/transposition backend used by adversarial search:
  keyed storage over Aleph hash tables, replacement policies and probe/store
  statistics.

- `search_move_ordering.H`
  Provides the opt-in ordering layer reused by Alpha-Beta: ordering modes,
  ordering stats, killer slots and history tables.

- `state_search_common.H`
  Still provides the reusable infrastructure that both adversarial engines
  inherit: `SearchLimits`, `ExplorationPolicy`, `SearchPath` and the baseline
  counters in `SearchStats`.

## Why Negamax is the base

Negamax is the natural base for this phase because, in a zero-sum game, a
player's gain is the opponent's loss. Once the domain agrees to evaluate every
state from the perspective of the player to move, the recurrence collapses to a
single maximization rule:

`value(state) = max(-value(child))`

This gives three practical benefits:

- one implementation model for both players
- simpler Alpha-Beta on top of the same recurrence
- a small domain contract with no explicit "max player / min player" split

## Domain contract

Adversarial engines expect an implicit game domain with:

- `using State`
  Mutable position, including the player to move.

- `using Move`
  Reversible move representation.

- `using Score`
  Signed integral or floating-point score type.

- `bool is_terminal(const State&)`
  Detects terminal positions.

- `Score evaluate(const State&)`
  Returns the score from the perspective of the player to move in `state`.
  This convention is mandatory.

- `template <class Visitor> bool for_each_successor(const State&, Visitor)`
  Lazily emits legal moves in exploration order.

- `void apply(State&, const Move&)`
  Applies one move and flips the turn through the state.

- `void undo(State&, const Move&)`
  Restores the exact previous position.

Optional:

- `bool should_prune(const State&, size_t depth)`
  Domain-side pruning hook reused from the common infrastructure.

- `using State_Key` and `State_Key state_key(const State&)`
  Stable key used by transposition tables. This is optional unless the caller
  wants the convenience overloads that bind a table directly to the domain.

## Shared with the rest of the framework

Adversarial search reuses only the parts that are genuinely common:

- `SearchLimits`
  `max_depth` is the search horizon in plies; `max_expansions` is a hard stop.

- `ExplorationPolicy`
  Currently only `Depth_First` applies here. Best-first is branch-and-bound
  specific and is rejected by the adversarial engines.

- `SearchPath<Move>`
  Principal variations are stored in Aleph `Array<Move>`.

- `SearchStats`
  Base counters such as visited states, expanded states, generated successors,
  depth cutoffs and maximum depth reached are reused directly.

- `MoveOrderingStats`
  Shared ordering counters are nested under `SearchStats` and reused when
  Alpha-Beta materializes and reorders successor batches.

## Not shared with branch and bound

Branch and bound and adversarial search are intentionally separate:

- branch and bound reasons about complete solutions, incumbents and optimistic
  objective bounds
- adversarial search reasons about game values, terminal states and principal
  variations under optimal opposition

The adversarial layer therefore defines its own:

- `AdversarialSearchResult`
- `AdversarialIterativeDeepeningResult`
- `AdversarialTraceEvent`
- `AdversarialSearchStats`
- `Negamax`
- `Alpha_Beta`

## `AdversarialSearchResult<Move, Score>`

Returned by `Negamax::search()`, `Alpha_Beta::search()` and the convenience
wrappers.

| Member / method | Type | Description |
|-----------------|------|-------------|
| `status` | `SearchStatus` | Final execution state. |
| `policy` | `ExplorationPolicy` | Exploration policy used for the run. |
| `limits` | `SearchLimits` | Limits used for the run. |
| `stats` | `AdversarialSearchStats` | Collected adversarial-search statistics. |
| `value` | `Score` | Root score from the current player's perspective. |
| `principal_variation` | `SearchPath<Move>` (`Array<Move>`) | Best line found from the root — the sequence of moves chosen by both players under optimal play. |
| `exhausted()` | `bool` | Returns `true` if the search exhausted the configured region. |
| `limit_reached()` | `bool` | Returns `true` if a hard search limit stopped the traversal. |
| `has_principal_variation()` | `bool` | Returns `true` if the PV is non-empty. |
| `first_move()` | `const Move&` | First move of the principal variation. Throws if empty. |

## `AdversarialSearchStats`

Extends `SearchStats` with adversarial-specific counters:

| Field | Type | Description |
|-------|------|-------------|
| `heuristic_evaluations` | `size_t` | Number of calls to `evaluate(state)`. |
| `alpha_beta_cutoffs` | `size_t` | Number of cutoffs performed by Alpha-Beta. |
| `transpositions` | `TranspositionStats` | Transposition-table usage during this run: probes, hits, misses, cutoffs, stores, replacements, rejected updates. |

## Statistics

Both engines expose:

- visited states
- expanded states
- generated successors
- terminal states
- depth cutoffs
- maximum depth reached
- heuristic evaluations

Alpha-Beta additionally reports:

- `alpha_beta_cutoffs`

If a transposition table is attached, both engines also report:

- TT probes
- TT hits and misses
- TT cutoffs enabled by cached entries

If move ordering is attached, Alpha-Beta also reports through
`stats.move_ordering`:

- ordered batches
- ordered moves
- score estimates
- killer hits
- history hits

Iterative deepening additionally reports:

- completed iterations
- explicit per-depth principal variations
- cumulative stats across all iterations
- aspiration retries

## Model limits

The current adversarial layer is intentionally narrow:

- exactly two players
- zero-sum payoffs
- alternating turns
- deterministic, perfect-information games
- no stochastic nodes
- no hidden information

These assumptions are what keep Negamax and Alpha-Beta small and predictable.

## Move ordering

Alpha-Beta is the only adversarial engine that currently uses move ordering.

Supported mode:

- `MoveOrderingMode::Estimated_Score`
  A one-ply estimate computed as `-evaluate(child_state)`.

Optional hooks:

- killer heuristic
  Two recent cutoff-producing moves per depth.

- history heuristic
  Sparse per-move scores keyed by `domain.move_key(move)`.

Negamax intentionally rejects these options for now. Reordering pure Negamax
changes tie-breaking, but by itself does not create pruning, so the extra
materialization/scoring cost is not justified yet.

## Iterative deepening

The framework now exposes:

- `iterative_deepening_negamax_search(...)`
- `iterative_deepening_alpha_beta_search(...)`

Both wrappers keep the same domain contract and the same result vocabulary.
The target horizon still comes from `SearchLimits::max_depth`; iterative
deepening only controls how the engine reaches that horizon.

The returned `AdversarialIterativeDeepeningResult` keeps:

- the deepest completed result in `result`
- one explicit per-depth snapshot in `iterations`
- cumulative cost in `total_stats`

This makes the principal variation explicit at each completed depth rather
than only at the final horizon.

## Aspiration windows

Aspiration windows are currently implemented only for iterative Alpha-Beta.

Configuration lives in:

- `AspirationWindow<Score>`
- `AdversarialIterativeDeepeningOptions<Score>`

When enabled, iteration `d + step` starts from a narrow root window centered on
the previous completed root value. On fail-low or fail-high, the wrapper widens
the window and re-searches until the score fits or the full range is reached.

This keeps the design narrow:

- no special semantics are added to branch and bound
- no new engine is introduced
- the same `Alpha_Beta` implementation is reused through `search_with_window()`

## Tracing and debugging

The adversarial layer now exposes a synchronous trace hook:

- `AdversarialTraceEventKind`
- `AdversarialTraceEvent<Move, Score>`
- `AdversarialTraceCollector<Move, Score>`

The engines can emit:

- node entry / exit
- terminal and horizon cutoffs
- Alpha-Beta cutoffs
- transposition hits
- iterative-deepening iteration begin / end
- aspiration retries

This is intended for debugging, logging and small tree visualizations. It is
not meant to be enabled indiscriminately on very large searches.

## Transposition-table integration

The adversarial layer reuses the generic `Transposition_Table<Key, Entry>`
module rather than embedding a game-specific cache container in each engine.

The default adversarial alias is:

`AdversarialTranspositionTable<StateKey, Move, Score>`

Its key is `(state_key, remaining_depth)`. The remaining depth is part of the
key because the same position searched to different horizons does not have the
same meaning for heuristic search.

The stored entry keeps:

- cached score
- remaining depth
- bound type: exact, lower, or upper
- principal variation fragment

Replacement is intentionally simple in this phase:

- exact results are preferred over non-exact ones
- deeper entries are preferred over shallower ones

This is enough to make Alpha-Beta and full-depth Tic-Tac-Toe measurably
smaller without forcing a more complicated table discipline yet.

The same transposition table instance can now be reused across iterative
deepening iterations and aspiration retries. That reuse is deliberate: it is
one of the main practical reasons to layer iterative deepening on top of the
existing TT-aware engines instead of creating a separate control loop outside
the framework.

## Why this is not shared with branch and bound yet

The storage backend is reusable, but the reuse rules are not the same.

For branch and bound, a cached value or bound depends on more than the local
state:

- minimization vs maximization objective
- incumbent value at the time of the probe/store
- meaning of the bound with respect to completion and feasibility

That means a naive reuse of the adversarial TT protocol would be unsound or at
least misleading. The current decision is to reuse only the storage module now
and postpone branch-and-bound memoization until its dominance and incumbent
semantics are specified explicitly.

## Examples in this phase

- `Examples/adversarial_artificial_example.cc`
  Minimal structural tree for sanity checking values and pruning.

- `Examples/negamax_tictactoe_example.cc`
  Complete Tic-Tac-Toe search from the empty board, with and without a
  transposition table, plus iterative Alpha-Beta with aspiration windows and
  tracing.

- `Examples/alpha_beta_connect3_example.cc`
  Reduced Connect-3 position searched with Alpha-Beta and a heuristic horizon.

## Small Aleph integrations

These extensions kept the same repo priorities:

- per-depth iteration history uses Aleph `Array`
- trace collection uses Aleph `DynList`
- TT reuse still relies on the existing Aleph hash-map backend

I did not add explicit TT `reserve()` sizing because the current Aleph hash-map
wrapper used here does not expose a clean reserve-style API at this layer.

## Next extension points

- TT-aware move ordering and PV reordering
- quiescence search
- aspiration tuning per domain
- branch-and-bound memoization rules over the same storage backend
