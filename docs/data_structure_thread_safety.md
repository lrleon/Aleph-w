# Data Structure Thread-Safety Vocabulary

This document defines the concurrency vocabulary that new Aleph data
structures should use in Doxygen, README entries, examples, and tests. The goal
is to make thread-safety claims precise and testable before adding new
concurrent containers.

## Thread-Safety Categories

**Sequential** means the object is intended for one thread at a time. Concurrent
access is only valid when all calls are externally serialized by the user.

**Externally synchronized** means the type does not synchronize internally, but
it is valid to protect it with an external mutex, channel, monitor, or another
user-provided synchronization object. Documentation should say which operations
may run while the external lock is held.

**Internally synchronized** means the type protects its own shared state. Public
member functions may be called concurrently according to the documented
operation matrix.

**Lock-free** means at least one participating thread makes progress in a
finite number of steps, even if other participating threads are delayed. This is
a progress guarantee, not a latency guarantee.

**Wait-free** means every participating thread completes its operation in a
bounded number of steps. Do not document a structure as wait-free unless every
public operation with that claim has the guarantee.

## Producer/Consumer Topologies

**SPSC** means single producer, single consumer.

**MPSC** means multiple producers, single consumer.

**SPMC** means single producer, multiple consumers.

**MPMC** means multiple producers, multiple consumers.

Queue documentation must state the supported topology explicitly. Calling a
queue from a topology wider than the documented one is a precondition violation,
even if a particular test run appears to work.

## Operation Guarantees

Each new concurrent data structure should document these points:

- Which operations are safe to call concurrently.
- Whether `size()`, `empty()`, or `is_empty()` are exact or only advisory under
  concurrent mutation.
- Whether iteration is supported while mutation is in progress.
- Whether references, pointers, or iterators returned by the structure may
  outlive the operation that produced them.
- Whether blocking operations can be interrupted or cancelled.
- Whether the type supports move-only payloads.

When a guarantee only applies to a subset of operations, document it on those
operations rather than only at class level.

## Testing Expectations

Concurrent containers should have tests for:

- Single-thread API behavior.
- Move-only payloads when the API accepts values.
- Destructor and lifetime accounting for stored elements.
- Producer/consumer stress with deterministic thread start.
- Randomized operation traces replayed against a simple sequential reference.
- Conservation checks: every produced element is consumed or remains stored
  exactly once, according to the data structure contract.
- ThreadSanitizer runs when the local toolchain supports TSAN.

Tests must not depend on sleeps for correctness. Short sleeps may be used only
to increase scheduling pressure after the test already has a deterministic
synchronization point.

## Reusable Test Harness

New concurrent data-structure tests should prefer the helpers in
`Tests/concurrency_test_utils.H` before writing local ad hoc synchronization.
The current helpers provide:

- `Deterministic_Start_Gate` for single-shot thread start synchronization.
- `run_producer_consumer_stress()` for producer/consumer conservation stress.
- `make_random_operation_trace()` for deterministic randomized operation
  traces.
- `replay_trace_and_collect_mismatches()` for parity checks against a simple
  sequential reference model.

These helpers are intentionally test-only. They should not be included from
production headers.

## CTest Selection

The current test CMake files primarily rely on target and GoogleTest case names,
not CTest labels. Until a project-wide label convention exists, new
concurrency tests should be named so they are selectable with filters such as:

```bash
ctest --test-dir build -R "Concurrency|ThreadPool|concurrency_test_utils" --output-on-failure
```

If labels are introduced later, use at least `concurrency` for normal
concurrency tests and `tsan` for tests intended to run under ThreadSanitizer.
