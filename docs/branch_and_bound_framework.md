# Branch and Bound Framework

## Module purpose

- `Branch_And_Bound.H`
  Reusable branch-and-bound engine for implicit optimization spaces. It keeps
  the same state-space contract introduced by the common core and adds the
  minimum machinery specific to optimization.

- `state_search_common.H`
  Shared vocabulary reused directly by branch and bound:
  `SearchLimits`, `ExplorationPolicy`, `SearchPath`, `SearchSolution`,
  `SearchSolutionCollector` and the base `SearchStats`.

- `search_move_ordering.H`
  Shared ordering support reused by depth-first branch and bound when child
  branches are ranked by optimistic bound.

- `State_Search.H`
  Convenience umbrella include for applications that want the whole framework.

## What branch and bound reuses from the common core

Branch and bound does not define a second API for implicit spaces. It reuses the
same make/unmake and lazy branching protocol already used by DFS/backtracking:

- `State` and `Move`
  Mutable state plus reversible move.

- `template <class Visitor> bool for_each_successor(const State&, Visitor)`
  Lazy branch generation.

- `void apply(State&, const Move&)`
  Apply one branch decision.

- `void undo(State&, const Move&)`
  Restore the exact previous state.

- `SearchLimits`
  Depth, expansion and solution caps.

- `ExplorationPolicy`
  Shared exploration settings. Branch and bound currently uses
  `Depth_First` and `Best_First`, plus optional `Estimated_Bound` ordering for
  depth-first child expansion.

- `SearchPath<Move>`
  Uses Aleph `Array<Move>` for path snapshots.

## What is specific to branch and bound

Optimization domains add four pieces on top of the common contract:

- `using Objective = ...`
  Totally ordered objective value type.

- `bool is_complete(const State&)`
  Tells the engine that the current state is a complete candidate solution.

- `Objective objective_value(const State&)`
  Returns the exact objective value of a complete solution.

- `Objective bound(const State&)`
  Returns an optimistic bound:
  for maximization, an upper bound;
  for minimization, a lower bound.

The module also adds:

- `Maximize_Objective<T>` and `Minimize_Objective<T>`
  Small objective policies that define comparison, incumbent improvement and
  best-first priority.

- `ObjectiveIncumbent`
  Global incumbent manager shared by both exploration strategies.

- `BranchAndBoundStats`
  Extends the common stats with `pruned_by_bound` and `incumbent_updates`.

- `OptimizationSolution<State, Move, Objective>`
  Extends `SearchSolution` with `objective_value`.

## Modeling a domain

The minimal branch-and-bound adapter looks like this:

```cpp
struct MyDomain
{
  struct Move { /* reversible decision */ };
  using State = MyState;
  using Objective = double;

  bool is_complete(const State &state) const;
  Objective objective_value(const State &state) const;
  Objective bound(const State &state) const;

  void apply(State &state, const Move &move) const;
  void undo(State &state, const Move &move) const;

  template <class Visitor>
  bool for_each_successor(const State &state, Visitor visit) const;
};
```

Optional hooks already understood by the engine:

- `bool is_terminal(const State&)`
  Marks dead-end states that cannot produce a complete solution.

- `bool should_prune(const State&, size_t depth)`
  Domain-side pruning hook, useful for feasibility cuts that are independent of
  the incumbent.

## Running the engine

Maximization with depth-first search:

```cpp
Aleph::Branch_And_Bound<MyDomain> engine(domain);
auto result = engine.search(initial_state);
```

Depth-first with child ordering by optimistic bound:

```cpp
Aleph::ExplorationPolicy policy = Aleph::Branch_And_Bound<MyDomain>::default_policy();
policy.move_ordering = Aleph::MoveOrderingMode::Estimated_Bound;

Aleph::Branch_And_Bound<MyDomain> engine(domain, policy);
auto result = engine.search(initial_state);
```

Minimization with best-first search:

```cpp
using Engine = Aleph::Branch_And_Bound<MyDomain, Aleph::Minimize_Objective<int>>;

Aleph::ExplorationPolicy policy = Engine::default_policy();
policy.strategy = Aleph::ExplorationPolicy::Strategy::Best_First;

Engine engine(domain, policy, {}, Aleph::Minimize_Objective<int>{});
auto result = engine.search(initial_state);
```

The final answer is stored in `result.incumbent`. Search-wide counters live in
`result.stats`.

## Aleph reuse in this phase

- `SearchPath<Move>` stores branch decisions in `Array<Move>`.
- `SearchSolutionCollector` continues using `DynList<Solution>` from the common
  layer.
- Best-first frontier ordering uses `DynBinHeap`.
- Future memoization and transposition storage can reuse
  `SearchStorageMap` / `SearchStorageSet`, already defined over Aleph hash
  containers.

## Design choices

- One domain contract for both DFS/backtracking and branch and bound.
  This keeps the framework coherent and makes later layering easier.

- Incumbent-aware pruning lives in the engine, not in the domain.
  Bounds and feasibility remain domain responsibilities, while incumbent
  comparison stays centralized and reusable.

- Best-first is supported without materializing the full search tree.
  Only frontier snapshots are stored, and only when that strategy is selected.

- Bound ordering stays local in DFS.
  Best-first already prioritizes the global frontier by bound through
  `DynBinHeap`, so the extra local ordering pass is reserved for depth-first
  traversal, where it directly improves incumbent discovery and pruning.

- No seeded incumbent or transposition table yet.
  The current implementation stays intentionally small and predictable.

## Future extensions

- Richer branch-ordering policies on top of the same `for_each_successor` contract.
- Seeded incumbents for warm starts.
- Memoization / transposition tables using Aleph hash maps.
- Parallel exploration once the sequential semantics are stable.
