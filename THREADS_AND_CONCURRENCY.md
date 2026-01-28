\defgroup ThreadsAndConcurrency Threads and Concurrency
\brief Threading utilities (thread pools, worker pools, timed event queues) and synchronization helpers.

@{

This module groups Aleph-w facilities related to **concurrency** and **thread-based execution**.

It covers:

- \ref thread__pool_8H (\ref Aleph::ThreadPool): modern task-based thread pool returning `std::future`.
- \ref worker__pool_8H (\ref WorkersSet): worker-function pool for processing queued `void*` work items.
- \ref timeoutQueue_8H (\ref TimeoutQueue): timed event scheduler running a background thread.
- \ref useMutex_8H / \ref useCondVar_8H: POSIX mutex/condvar wrappers.

## ThreadPool (task-based)

See \ref thread__pool_8H and the runnable example \ref thread__pool__example_8cc.

Typical usage pattern:

@code
#include <thread_pool.H>

Aleph::ThreadPool pool(4);

auto f = pool.enqueue([](int x) { return x * x; }, 5);
int v = f.get();
(void) v;
@endcode

## TimeoutQueue (timed events)

See \ref timeoutQueue_8H and the runnable example \ref timeoutQueue__example_8C.

## Notes

- Most components here are **thread-safe by design** (e.g. `ThreadPool::enqueue()`), but always check the specific header documentation.
- Some higher-level algorithms (e.g. shortest paths) are generally **not** thread-safe unless you use separate instances.

@}
