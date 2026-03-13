\defgroup ThreadsAndConcurrency Threads and Concurrency
\brief Threading utilities (thread pools, worker pools, timed event queues) and synchronization helpers.

@{

This module groups Aleph-w facilities related to **concurrency** and **thread-based execution**.

It covers:

- \ref thread_pool.H (\ref Aleph::ThreadPool): modern task-based thread pool returning `std::future`.
- \ref thread_pool.H (\ref Aleph::TaskGroup): small structured-concurrency helper on top of `ThreadPool`.
- \ref thread_pool.H (\ref Aleph::CancellationSource, \ref Aleph::CancellationToken): cooperative cancellation primitives.
- \ref thread_pool.H (\ref Aleph::ParallelOptions): shared tuning/cancellation options for parallel helpers.
- \ref thread_pool.H (`parallel_invoke`, `pscan`, `pexclusive_scan`, `pmerge`): foundational parallel building blocks for composing higher-level algorithms.
- \ref concurrency_utils.H (\ref Aleph::bounded_channel, \ref Aleph::synchronized, \ref Aleph::rw_synchronized, \ref Aleph::spsc_queue): modern coordination helpers for channels, shared state, and SPSC handoff.
- \ref worker_pool.H (\ref WorkersSet): worker-function pool for processing queued `void*` work items.
- \ref timeoutQueue.H (\ref TimeoutQueue): timed event scheduler running a background thread.
- \ref useMutex.H (\ref UseMutex): legacy POSIX mutex wrapper.
- \ref useCondVar.H (\ref UseCondVar): legacy POSIX condition variable wrapper.

## ThreadPool (task-based)

See \ref thread_pool.H and the runnable example \ref thread_pool_example.cc.

Typical usage pattern:

```cpp
#include <thread_pool.H>

Aleph::ThreadPool pool(4);

auto f = pool.enqueue([](int x) { return x * x; }, 5);
int v = f.get();
(void) v;
```

Structured task groups:

```cpp
#include <thread_pool.H>

Aleph::ThreadPool pool(4);
Aleph::TaskGroup group(pool);

group.launch([] { /* task A */ });
group.launch([] { /* task B */ });
group.wait();
```

Cooperative cancellation:

```cpp
Aleph::CancellationSource cancel;
auto token = cancel.token();

pool.enqueue([token] {
  while (not token.stop_requested())
    do_some_work_step();
});

cancel.request_cancel();
```

Parallel algorithms can now be configured through `ParallelOptions`, including
`min_size`, `chunk_size`, `max_tasks`, `pool`, and `cancel_token`.

This applies to the main helpers in `ah-parallel.H`, including map/filter/fold,
parallel sort, scan/merge wrappers, zip, and enumerate operations. Existing
`ThreadPool&` overloads remain available as compatibility wrappers.

If `cancel_token` is signaled and the algorithm observes it, the operation
throws `Aleph::operation_canceled`.

Foundational building blocks now available in `thread_pool.H`:

```cpp
Aleph::parallel_invoke(options, [] { /* branch A */ }, [] { /* branch B */ });
Aleph::pscan(first, last, out, std::plus<int>{}, options);
Aleph::pexclusive_scan(first, last, out, 0, std::plus<int>{}, options);
Aleph::pmerge(a_first, a_last, b_first, b_last, out, std::less<int>{}, options);
```

Modern coordination helpers in `concurrency_utils.H`:

```cpp
#include <concurrency_utils.H>

Aleph::bounded_channel<int> ch(16);
Aleph::synchronized<std::vector<int>> items(std::in_place);
Aleph::rw_synchronized<int> shared_sum(0);
Aleph::spsc_queue<int> handoff(32);

ch.send(10);
if (auto item = ch.recv())
  items.with_lock([&](auto &out) { out.push_back(*item); });

shared_sum.with_write_lock([](int &value) { value += 5; });
const int snapshot = shared_sum.with_read_lock([](const int &value) {
  return value;
});
(void) snapshot;

handoff.try_push(1);
auto popped = handoff.try_pop();
```

`bounded_channel<T>` also offers cancellation-aware blocking operations:

```cpp
Aleph::CancellationSource cancel;
Aleph::bounded_channel<int> jobs(8);

try
  {
    jobs.send(42, cancel.token());
    auto item = jobs.recv(cancel.token());
    (void) item;
  }
catch (const Aleph::operation_canceled &)
  {
    // stop blocked send/recv cleanly
  }
```

`UseMutex` and `UseCondVar` are still available for backward compatibility with
older POSIX-style code, but new code should generally prefer
`concurrency_utils.H` and standard C++ synchronization primitives.

See also the runnable example `Examples/concurrency_utils_example.cc`.

Work stealing was evaluated for this phase and intentionally deferred; see
`docs/work_stealing_backend_note.md` for the design note and rationale.

## TimeoutQueue (timed events)

See \ref timeoutQueue.H and the runnable example \ref timeoutQueue_example.C.

## Notes

- Most components here are **thread-safe by design** (e.g. `ThreadPool::enqueue()`), but always check the specific header documentation.
- Some higher-level algorithms (e.g. shortest paths) are generally **not** thread-safe unless you use separate instances.

@}
