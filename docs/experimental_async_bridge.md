# Experimental Async Bridge

This note records the Phase 4 decision for Aleph-w's experimental async work.

## Assessment

Aleph-w's documented compiler targets are GCC 11+ and Clang 14+, with C++20 as
the default language mode. That target set is sufficient for a small
coroutine-friendly bridge based on `<coroutine>`.

The build configuration still exposes non-C++20 modes, so coroutines cannot be
made part of the default public path. Any async layer must therefore be:

- opt-in,
- isolated from the stable APIs,
- tiny enough to audit easily,
- safe to compile out entirely when C++20 coroutine support is unavailable.

## Implemented Scope

The experimental layer lives in `experimental_async.H` and is guarded by:

- `ALEPH_ENABLE_EXPERIMENTAL_ASYNC=1` to opt in,
- `ALEPH_HAS_EXPERIMENTAL_ASYNC` to detect whether the bridge is actually
  available in the current translation unit.

Public surface:

- `Aleph::experimental::scheduled_operation<T>`
- `Aleph::experimental::schedule(ThreadPool&, f, args...)`
- `Aleph::experimental::schedule(f, args...)`

This is intentionally an interop layer, not a framework.

## Design Notes

- Scheduling still uses the existing `ThreadPool`.
- Results behave like a small move-only `std::future<T>`-style object.
- The returned object supports `wait()`, `get()`, and `co_await`.
- Awaiting resumes on the worker thread that completes the scheduled callable.
- Results are single-consumer.
- `get()` / `await_resume()` consume the state, so `valid()` becomes false
  afterward.
- Destroying a suspended awaiter unregisters its continuation so completion
  will not resume a dead coroutine frame.
- Return-by-reference is rejected on purpose; use values or
  `std::reference_wrapper<T>` instead.

## Explicit Non-Goals

- No coroutine-aware executor abstraction.
- No sender/receiver graph implementation.
- No custom event loop.
- No attempt to retrofit the whole library around `co_await`.

## Current Risks

- The continuation resumes on a pool worker. This is simple and predictable,
  but it is not the same as a GUI/main-thread affinity model.
- There is no cooperative cancellation wiring into the scheduled callable yet.
- There is no `when_all`, `when_any`, or general coroutine task type in the
  public API. That is deliberate to keep the experimental surface small.

## Recommended Next Step

If this bridge proves useful in practice, the next step should be to evaluate a
tiny `task<T>` coroutine return type or a very small `when_all` helper, still
kept under the experimental namespace and feature gate.
