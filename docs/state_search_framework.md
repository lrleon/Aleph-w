# State-Space Search Framework

## Current modules

For a user-oriented entry point with reference examples, see
`README.state_search.md`.

For the benchmark suite, profiling summary and v1 hardening notes, see
`docs/state_search_v1.md`.

- `state_search_common.H`
  Defines the shared vocabulary for implicit search: `SearchResult`,
  `SearchStats`, `SearchLimits`, `ExplorationPolicy`, `SearchSolution`,
  `BestSolution` / `Incumbent`, `SearchSolutionCollector`, and the minimal
  concepts for domains, moves and solution visitors.

- `search_move_ordering.H`
  Defines the shared move-ordering vocabulary: ordering modes, ordering stats,
  and lightweight killer/history helper tables.

- `Backtracking.H`
  Implements the first concrete family: reusable depth-first search /
  backtracking over mutable states with `apply()` / `undo()`.

- `Branch_And_Bound.H`
  Adds reusable optimization search on top of the same implicit-state contract:
  objective policies, optimistic bounds, incumbent management, bound-based
  pruning, and depth-first / best-first exploration.

- `Negamax.H`
  Introduces the adversarial-search contract and pure Negamax evaluation for
  two-player zero-sum games, plus tracing helpers and iterative-deepening
  result types for that layer.

- `Alpha_Beta.H`
  Adds Alpha-Beta pruning on top of the same adversarial-search contract,
  including iterative deepening and aspiration-window control.

- `Transposition_Table.H`
  Adds reusable memoization/transposition storage over Aleph hash tables.
  Adversarial search already uses it; other families can reuse the same
  backend when their reuse semantics are defined.

- `State_Search.H`
  Umbrella header for users that want the current framework in one include.
  It also exports the convenience facade `Aleph::Search`.

## Modeling a problem as an implicit state space

The current DFS/backtracking engine expects a domain adapter with nested
`State` and `Move` types plus a small operational contract.

### Required pieces

- `State`
  Mutable search state. It should contain exactly the information needed to
  decide feasibility, generate moves and reconstruct a solution snapshot.

- `Move`
  Reversible decision applied to the current state.

- `bool is_goal(const State&)`
  Returns `true` when the current state is an accepted solution. Goal states
  are terminal in the engine and are not expanded further.

- `template <class Visitor> bool for_each_successor(const State&, Visitor)`
  Lazily emits candidate moves. The visitor returns `true` to continue and
  `false` to stop early. This is the preferred generation API because it avoids
  materializing children unless a later phase needs ordering.

- `void apply(State&, const Move&)`
  Applies a move in-place.

- `void undo(State&, const Move&)`
  Restores the state to the exact pre-`apply()` configuration.

### Optional pieces

- `bool is_terminal(const State&)`
  Marks dead-end terminal states that are not solutions. Useful when the domain
  can detect completion/failure before trying to generate more successors.

- `bool should_prune(const State&, size_t depth)`
  Requests an early cutoff. This is a lightweight hook for domain-side pruning.
  Future branch-and-bound layers can build stronger incumbent-aware pruning on
  top of the same DFS kernel.

## Running DFS / backtracking

The simplest use is:

```cpp
#include <State_Search.H>

auto result = Aleph::Search::backtracking_search(domain, initial_state);
```

This keeps the best solution found in `result.best_solution` and records
statistics in `result.stats`.

### Enumerating multiple solutions

If you want more than the best incumbent, pass a callback or a
`SearchSolutionCollector`:

```cpp
Aleph::Search::ExplorationPolicy policy;
policy.stop_at_first_solution = false;

using Engine = Aleph::Search::Backtracking<MyDomain>;
Aleph::Search::Collector<Engine::Solution> collector;

auto result = Aleph::Search::backtracking_search(domain,
                                                 initial_state,
                                                 collector,
                                                 policy);
```

The callback receives a `const SearchSolution<State, Move>&` and returns:

- `true` to continue enumeration
- `false` to stop immediately

### Limits

`SearchLimits` currently supports:

- `max_depth`
  States at exactly `max_depth` are still visited and checked for goal, but are
  not expanded.

- `max_expansions`
  Hard stop on the number of expanded states.

- `max_solutions`
  Hard stop on the number of accepted solutions.

## API reference

### `SearchStatus`

Final state of a search execution, returned in `result.status`:

| Value | Meaning |
|-------|---------|
| `NotStarted` | Search object exists but no traversal has run yet. |
| `Exhausted` | Search space within the configured bounds was fully explored. |
| `StoppedOnSolution` | Search stopped because the solution visitor or `stop_at_first_solution` requested it. |
| `LimitReached` | A hard limit (`max_expansions` or `max_solutions`) terminated the search early. |

### `SearchResult<Solution, Compare>`

Aggregates the outcome of one search execution.

| Member / method | Type | Description |
|-----------------|------|-------------|
| `status` | `SearchStatus` | Final execution state. |
| `policy` | `ExplorationPolicy` | Exploration policy used for the run. |
| `limits` | `SearchLimits` | Limits used for the run. |
| `stats` | `SearchStats` | Statistics collected during the run. |
| `best_solution` | `BestSolution<Solution, Compare>` | Best incumbent retained by the engine. |
| `found_solution()` | `bool` | Returns `true` if at least one solution was retained. |
| `exhausted()` | `bool` | Returns `true` if the search exhausted the configured region (`status == Exhausted`). |
| `stopped_on_solution()` | `bool` | Returns `true` if the search stopped because the solution visitor requested it. |
| `limit_reached()` | `bool` | Returns `true` if a hard search limit stopped the traversal. |

### `SearchSolutionCollector<Solution>`

Stores accepted solutions in a `DynList` for later inspection.

| Member / method | Type | Description |
|-----------------|------|-------------|
| `SearchSolutionCollector()` | constructor | Build a collector with no limit. |
| `SearchSolutionCollector(max)` | constructor | Build a collector that stops after `max` solutions. |
| `operator()(solution)` | `bool` | Append one solution; returns `false` when the limit is reached. |
| `solutions()` | `const DynList<Solution>&` | Read-only access to collected solutions. |
| `size()` | `size_t` | Number of collected solutions. |
| `is_empty()` | `bool` | Returns `true` if no solution has been collected. |
| `clear()` | `void` | Remove all collected solutions and reset the counter. |

## Design decisions in this phase

- Mutable state plus reversible moves.
  This keeps the hot path close to the classic make/unmake style used in
  exact search, games and combinatorial optimization.

- Lazy successor generation.
  `for_each_successor()` is the base contract. Future move ordering can still
  materialize moves explicitly when needed, but DFS does not pay that cost by
  default.

- Aleph containers reused directly.
  Paths use `Array<Move>`, collectors use `DynList<Solution>`, and future
  memo/transposition storage hangs from `SearchStorageMap` / `SearchStorageSet`.

- Storage is shared, semantics are not forced.
  `Transposition_Table.H` is generic storage plus replacement policy. Each
  search family still decides what a valid cached entry means. That keeps
  branch and bound, backtracking and adversarial search from leaking semantics
  into each other.

- One kernel, no parallel API.
  The DFS/backtracking family reuses the same `SearchResult`, `SearchStats`,
  `SearchLimits` and `ExplorationPolicy` introduced by the common core.

- Move ordering stays opt-in.
  Engines keep the lazy `for_each_successor()` fast path unless
  `ExplorationPolicy` explicitly requests ordering.

## Included examples in this phase

- `Examples/state_search_framework_example.cc`
  Minimal artificial decision tree.

- `Examples/backtracking_nqueens_example.cc`
  Exhaustive N-Queens enumeration.

- `Examples/backtracking_subset_sum_example.cc`
  Small subset-sum enumeration with domain-side pruning.

- `Examples/branch_and_bound_artificial_example.cc`
  Artificial maximization and minimization trees with incumbent-driven pruning.

- `Examples/branch_and_bound_knapsack_example.cc`
  0/1 knapsack with fractional upper bound.

- `Examples/branch_and_bound_assignment_example.cc`
  Small assignment minimization with a row-wise lower bound.

- `Examples/adversarial_artificial_example.cc`
  Minimal adversarial tree for Negamax and Alpha-Beta.

- `Examples/negamax_tictactoe_example.cc`
  Complete Tic-Tac-Toe search from the empty board, with and without
  transposition tables.

- `Examples/alpha_beta_connect3_example.cc`
  Reduced Connect-3 search with Alpha-Beta, comparing raw domain order versus
  heuristic move ordering.

- `Examples/state_search_benchmark.cc`
  Reproducible benchmark suite for N-Queens, knapsack and Alpha-Beta on
  Tic-Tac-Toe.

## Next extension points

- Extend move ordering with TT-aware ordering, PV reordering and stronger history policies.
- Add seeded incumbents and stronger reusable bounding helpers for branch and bound.
- Add branch-and-bound memoization once incumbent-aware reuse rules are explicit.
- Add iterative deepening and stronger replacement policies for adversarial search.
