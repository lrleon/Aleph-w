# Algorithm Selection Guide

## When to use each search engine

The Aleph state-space search framework provides five engine families.  Each one
targets a different problem shape.  This guide helps you pick the right engine
for your domain and explains the trade-offs between them.

---

## Decision flowchart

```text
Is the search adversarial (two-player, zero-sum)?
│
├─ YES
│   │
│   └─ Can you write an evaluate(state) from the side-to-move perspective?
│       │
│       ├─ YES, and I need pruning (large game trees)
│       │   └─ ▶ Alpha-Beta  (optionally with TT, iterative deepening,
│       │         aspiration windows, move ordering)
│       │
│       └─ YES, and the tree is small or I want a baseline
│           └─ ▶ Negamax  (optionally with TT)
│
└─ NO  (single-agent optimization or enumeration)
    │
    ├─ Do I need to optimize an objective (min or max)?
    │   │
    │   ├─ YES, and I can compute an optimistic bound
    │   │   └─ ▶ Branch and Bound  (depth-first or best-first)
    │   │
    │   └─ NO, I just need feasible solutions or to enumerate them
    │       │
    │       ├─ Is the cost function monotone and do I want optimal cost?
    │       │   └─ ▶ IDA\*  (with admissible heuristic)
    │       │
    │       └─ I want all solutions, or the first feasible one
    │           └─ ▶ DFS / Backtracking
```

---

## Engine comparison

| Feature | DFS / Backtracking | Branch & Bound | IDA\* | Negamax | Alpha-Beta |
|---------|:-:|:-:|:-:|:-:|:-:|
| Problem type | Enumeration / feasibility | Optimization | Optimal-cost path | Adversarial (baseline) | Adversarial (pruning) |
| Players | 1 | 1 | 1 | 2 | 2 |
| Pruning | Domain-only | Bound vs incumbent | Threshold | None | Alpha-beta window |
| Memory | O(depth) | O(depth) or O(frontier) | O(depth) | O(depth) | O(depth) |
| Requires `bound()` | No | Yes | No | No | No |
| Requires `heuristic()` | No | No | Yes | No | No |
| Requires `evaluate()` | No | No | No | Yes | Yes |
| Optimal guarantee | Exhaustive | Yes (with admissible bound) | Yes (with admissible heuristic) | Yes (full depth) | Yes (full depth) |
| TT support | Via visited-set | Planned | No | Yes | Yes |
| Move ordering | No | Estimated_Bound | No | No | Estimated_Score, killer, history |
| Iterative deepening | No | No | Built-in | Via wrapper | Via wrapper |

---

## Detailed guidance

### DFS / Backtracking

**Use when** you need to find one feasible solution, enumerate all solutions, or
explore an implicit space without optimization.

**Domain contract**: `is_goal()`, `for_each_successor()`, `apply()`, `undo()`.

**Typical problems**:
- N-Queens placement
- Sudoku solving
- Graph coloring (feasibility)
- Constraint satisfaction without optimization

**Key settings**:
- `stop_at_first_solution = true` for early exit
- `max_depth` to limit exploration depth
- `max_solutions` to cap enumeration
- Pass a `SearchSolutionCollector` to collect multiple solutions

**Example**: [backtracking_nqueens_example.cc](../Examples/backtracking_nqueens_example.cc),
[backtracking_subset_sum_example.cc](../Examples/backtracking_subset_sum_example.cc)

---

### Branch and Bound

**Use when** you are solving a combinatorial optimization problem and can
provide an optimistic bound on partial solutions.

**Domain contract**: everything from DFS plus `is_complete()`,
`objective_value()`, `bound()`.

**Typical problems**:
- 0/1 Knapsack
- Assignment / scheduling
- Traveling salesman (with lower-bound relaxation)
- Any problem where partial states admit a cheap optimistic estimate

**Key settings**:
- `Maximize_Objective<T>` or `Minimize_Objective<T>` to set direction
- `Best_First` strategy for best-bound-first exploration (uses a heap)
- `Depth_First` with `MoveOrderingMode::Estimated_Bound` for bound-ordered DFS
- `max_expansions` as a hard budget

**Why not DFS?**  DFS finds solutions but has no mechanism to prune suboptimal
branches.  Branch and bound tracks an incumbent and uses the bound to skip
entire subtrees that cannot improve it.

**Why not IDA\*?**  IDA\* targets shortest-path-like problems with monotone
costs.  Branch and bound handles arbitrary objective functions (maximize or
minimize) and non-monotone bounds.

**Example**: [branch_and_bound_knapsack_example.cc](../Examples/branch_and_bound_knapsack_example.cc),
[branch_and_bound_assignment_example.cc](../Examples/branch_and_bound_assignment_example.cc)

---

### IDA\* (Iterative Deepening A*)

**Use when** you want the optimal-cost solution in a single-agent space and
have an admissible heuristic, but the state space is too large for explicit
storage of the frontier (as BFS/A* would require).

**Domain contract**: `is_goal()`, `for_each_successor()`, `apply()`, `undo()`,
plus `heuristic(state) -> Cost` and each move carries a `Cost cost` field.

**Typical problems**:
- Sliding-tile puzzles (15-puzzle, 24-puzzle)
- Shortest path in implicit graphs with uniform or weighted edges
- Planning problems with admissible heuristics

**Key settings**:
- The engine manages the cost threshold internally
- `max_depth` limits ply depth per iteration
- `max_expansions` as a global budget across all iterations

**Why not DFS?**  DFS finds *a* solution, not necessarily the cheapest one.
IDA\* guarantees cost-optimality when the heuristic is admissible.

**Why not Branch and Bound?**  For shortest-path problems with monotone edge
costs, IDA\* is simpler (no `bound()` needed, no incumbent management) and
uses less memory than best-first B&B.

**Example**: The test suite in
[state_search_framework_test.cc](../Tests/state_search_framework_test.cc)
contains IDA\* examples with grid-based domains.

---

### Negamax

**Use when** you are modeling a two-player, zero-sum, alternating-turn game and
want a correct baseline engine.

**Domain contract**: `is_terminal()`, `evaluate()` (from the perspective of
the side to move), `for_each_successor()`, `apply()`, `undo()`.

**Typical problems**:
- Small complete games (Tic-Tac-Toe)
- Reference implementation to validate Alpha-Beta results
- Games where the tree is small enough that pruning is unnecessary

**Key settings**:
- `max_depth` sets the search horizon in plies
- Attach a `Transposition_Table` to avoid re-evaluating the same position

**Why not Alpha-Beta?**  Negamax explores the full minimax tree.  Use it when
the tree is small enough that you don't need pruning, or as a reference to
verify that Alpha-Beta returns the same value.

**Example**: [negamax_simple_example.cc](../Examples/negamax_simple_example.cc)

---

### Alpha-Beta

**Use when** you need adversarial search on a game tree that is too large
for full Negamax enumeration.

**Domain contract**: same as Negamax.

**Typical problems**:
- Chess, checkers, Connect-4 (with depth-limited search)
- Any game where good move ordering can prune large portions of the tree

**Key settings**:
- `MoveOrderingMode::Estimated_Score` — sort children by `-evaluate(child)`
  before exploring them (dramatically improves pruning)
- `use_killer_moves = true` — promote recent cutoff-producing moves
- `use_history_heuristic = true` — promote historically successful moves
  (requires `move_key()` on the domain)
- Iterative deepening with `iterative_deepening_alpha_beta_search()`
- Aspiration windows via `AdversarialIterativeDeepeningOptions`
- Transposition tables for memoization across the search

**Why not Negamax?**  Alpha-Beta returns the same value as Negamax but visits
far fewer nodes.  The reduction depends on move ordering quality — with perfect
ordering, Alpha-Beta examines O(b^(d/2)) nodes instead of O(b^d).

**Example**: [negamax_tictactoe_example.cc](../Examples/negamax_tictactoe_example.cc)
(advanced),
[alpha_beta_connect3_example.cc](../Examples/alpha_beta_connect3_example.cc)

---

## Common patterns

### Starting simple, then upgrading

A natural progression for adversarial search:

1. **Negamax** — verify domain contract, check game-theoretic value
2. **Alpha-Beta** — add pruning, confirm same value with fewer nodes
3. **+ Move ordering** — enable `Estimated_Score`, measure cutoff improvement
4. **+ Transposition table** — reduce redundant evaluations
5. **+ Iterative deepening** — get anytime behavior and PV at each depth
6. **+ Aspiration windows** — narrow the root window for faster convergence

For optimization:

1. **DFS / Backtracking** — enumerate solutions, verify domain contract
2. **Branch and Bound** — add `bound()` and objective, enable pruning
3. **+ Best-first** — switch strategy for tighter frontier management
4. **+ Bound ordering** — rank DFS children by bound for earlier incumbents

### Checking your results

- **Negamax vs Alpha-Beta**: both must return the same root value for the same
  position and depth.  If they disagree, the domain contract has a bug.
- **PV replay**: apply the principal variation to the root state and verify
  that `evaluate(terminal)` is consistent with the reported value.
- **Statistics cross-check**: Alpha-Beta `visited_states` must be ≤ Negamax
  `visited_states` for the same position and depth.

---

## Summary table

| I want to... | Use |
|---|---|
| Find any feasible solution | DFS / Backtracking |
| Enumerate all solutions | DFS / Backtracking (`stop_at_first_solution = false`) |
| Find the cheapest path | IDA\* |
| Maximize or minimize an objective | Branch and Bound |
| Solve a two-player game (small) | Negamax |
| Solve a two-player game (large) | Alpha-Beta + move ordering + TT |
