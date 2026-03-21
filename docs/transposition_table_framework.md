# Transposition Tables and Memoization

## Module purpose

`Transposition_Table.H` provides the reusable storage layer for cached state
evaluations in the search framework.

It is intentionally generic:

- keyed storage over Aleph hash tables
- minimal replacement-policy protocol
- probe/store/replacement statistics
- no hardcoded adversarial or optimization semantics

This separation is deliberate. Reusing the same storage backend is useful;
forcing the same reuse rules across very different search families is not.

## Reused Aleph components

This phase reuses Aleph tangibly instead of introducing a parallel cache stack:

- `MapOLhash` is the default hash-table backend
- `DynHashMap` remains available through `SearchStorageMap`
- `dft_hash_fct()` and `hash_combine()` are used for stable key hashing
- `SearchStorageMap` from `state_search_common.H` keeps the storage backend
  aligned with the rest of the framework

The one place where a custom type was introduced was the adversarial key:
`AdversarialTranspositionKey<StateKey>`.

That was necessary because the default Aleph hash path did not accept the
`std::pair<StateKey, size_t>` shape directly in this integration. Using an
explicit key type keeps hashing under Aleph's normal `aleph_hash_value()`
protocol and avoids adding STL-specific hashing glue to the framework core.

## What is a valid key

A transposition table is only sound if the key represents the full search
configuration needed to reuse a cached result.

For adversarial search in this phase, the key is:

- `state_key(state)`
- remaining search depth

The remaining depth is part of the key because a heuristic value computed at a
shorter horizon cannot be reused blindly as if it came from a deeper search.

The framework therefore expects domains that want convenience TT integration to
expose:

```cpp
using State_Key = ...;
State_Key state_key(const State &) const;
```

The domain is responsible for making `State_Key` match its equivalence notion.
If two states hash to the same key but are not actually equivalent for the
search semantics, cached reuse becomes unsound.

## Stored entry and replacement policy

The generic table stores arbitrary entries. Adversarial search uses:

- score
- remaining depth
- bound kind: exact / lower / upper
- principal variation fragment

The replacement rule is intentionally small:

- prefer exact entries over non-exact ones
- otherwise prefer deeper entries

This is enough for the current engines without forcing aging, buckets, or more
aggressive schemes yet.

## Memory and tradeoffs

Transposition tables trade memory for search reduction.

Benefits:

- fewer repeated evaluations
- fewer repeated subtree traversals
- more cutoffs in Alpha-Beta when bounds are reused

Costs:

- more memory retained per search
- extra hashing/probe/store work on each visited node
- correctness depends on a sound equivalence key

The framework keeps the table explicit and caller-owned so the user controls:

- table lifetime
- reuse across searches
- memory budget
- replacement policy

## Branch and bound: why deferred

The storage backend itself can be reused by branch and bound, but the
adversarial reuse protocol cannot be copied directly.

In branch and bound, the meaning of a cached entry depends on:

- whether the problem is minimization or maximization
- whether the value is exact, partial, feasible or just a bound
- the incumbent that was active when the node was evaluated

Until those semantics are specified carefully, integrating TT-style reuse into
branch and bound would risk unsound pruning or misleading cache hits.

The current decision is:

- reuse `Transposition_Table.H` as the backend
- keep direct integration only in `Negamax.H` and `Alpha_Beta.H`
- defer branch-and-bound memoization to a later phase

## Current validation

This phase includes:

- unit tests for replacement behavior and collision tolerance
- adversarial tests that compare results with and without TT
- a Tic-Tac-Toe example that shows node reduction with TT attached
