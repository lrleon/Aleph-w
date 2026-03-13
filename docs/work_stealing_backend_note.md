# Work-Stealing Backend Note

This phase evaluated whether Aleph-w should add an opt-in work-stealing pool
alongside `ThreadPool`.

## Decision

Work stealing is deferred for now.

## Why It Was Deferred

Aleph-w's current parallel surface now routes configuration through
`ParallelOptions`, but executor selection is still represented as
`ThreadPool *pool`. A clean work-stealing backend would therefore require at
least one of these changes:

- a small executor abstraction shared by `ThreadPool` and `WorkStealingPool`, or
- a templated/variant-backed executor slot inside `ParallelOptions`.

Doing that safely is larger than this phase because it would affect:

- task submission APIs (`enqueue`, detached submission, bulk submission),
- shutdown and `wait_all()` semantics,
- exception propagation guarantees,
- cancellation polling points,
- example code and documentation for executor selection.

Adding a partial backend without that abstraction would either duplicate a large
part of the current API or force unsafe casts/adapters around `ParallelOptions`.

## What A Future Backend Should Provide

If Aleph-w adds a `WorkStealingPool` later, it should remain opt-in and preserve
the semantics users already rely on from `ThreadPool`:

- `enqueue()` returning `std::future<T>`
- detached submission with exception handling
- `wait_all()`
- `num_threads()`
- clean shutdown/join semantics
- compatibility with cooperative cancellation

The backend itself would likely need:

- one deque per worker,
- owner-fast LIFO pop,
- thief FIFO steal,
- a global injection path for external submissions,
- deterministic shutdown rules for pending local and stolen work.

## Suggested Integration Path

1. Introduce a minimal executor concept or non-virtual adapter type.
2. Change `ParallelOptions::pool` into an executor handle that can wrap either
   `ThreadPool` or `WorkStealingPool`.
3. Add focused tests for fairness, shutdown, exception propagation, and nested
   parallelism before enabling the backend in examples.

Until those pieces exist, the current `ThreadPool` remains the stable default.
