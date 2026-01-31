\defgroup ThreadsAndConcurrency Threads and Concurrency
\brief Threading utilities (thread pools, worker pools, timed event queues) and synchronization helpers.

@{

This module groups Aleph-w facilities related to **concurrency** and **thread-based execution**.

It covers:

- \ref thread_pool.H (\ref Aleph::ThreadPool): modern task-based thread pool returning `std::future`.
- \ref worker_pool.H (\ref WorkersSet): worker-function pool for processing queued `void*` work items.
- \ref timeoutQueue.H (\ref TimeoutQueue): timed event scheduler running a background thread.
- \ref useMutex.H / \ref useCondVar.H: POSIX mutex/condvar wrappers.

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

## TimeoutQueue (timed events)

See \ref timeoutQueue.H and the runnable example \ref timeoutQueue_example.C.

## Notes

- Most components here are **thread-safe by design** (e.g. `ThreadPool::enqueue()`), but always check the specific header documentation.
- Some higher-level algorithms (e.g. shortest paths) are generally **not** thread-safe unless you use separate instances.

@}
