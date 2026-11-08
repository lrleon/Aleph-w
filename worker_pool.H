
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file worker_pool.H
 *  @brief General-purpose worker thread pool for parallel task execution.
 *
 *  This file provides WorkersSet, a configurable thread pool that executes
 *  user-provided worker functions in parallel.
 *
 *  ## Features
 *
 *  - Configurable number of worker threads
 *  - Generic worker function via template
 *  - Job completion signaling
 *  - Graceful shutdown
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Worker function: returns true when job is done
 *  bool my_worker(void* params) {
 *    auto* data = static_cast<MyData*>(params);
 *    // Process data...
 *    return data->is_last;  // true stops all workers
 *  }
 *
 *  WorkersSet<decltype(&my_worker)> pool(&my_worker, 100, 4);
 *
 *  // Schedule work items; the pool frees them with free(), so that they
 *  // are allocated with malloc() and trivially destructible
 *  pool.prepare_num_workers(items.size());
 *  for (auto& item : items) {
 *    auto* data = static_cast<MyData*>(std::malloc(sizeof(MyData)));
 *    *data = MyData{item};
 *    pool.schedule_call(data);
 *  }
 *
 *  pool.wait_until_all_workers_finished_or_job_is_done();
 *  ```
 *
 *  @deprecated WorkersSet goes in 8.0.0: use ThreadPool (thread_pool.H).
 *
 *  @see q-consumer-threads.H Queue-based variant
 *  @ingroup Utilities
 *  @ingroup ThreadsAndConcurrency
 *  @author Leandro Rabindranath León
 */

# ifndef WORKER_POOL_H
# define WORKER_POOL_H

# include <atomic>
# include <cstdlib>
# include <thread>
# include <mutex>
# include <condition_variable>
# include <memory>

# include <ah-errors.H>
# include <tpl_arrayQueue.H>
# include <iostream>

using namespace Aleph;

/**
 * @brief Thread pool for parallel worker function execution.
 *
 * Manages a pool of worker threads that execute a user-provided function.
 * Work items are scheduled via a queue and distributed to available workers.
 *
 * @tparam WorkerFct Function pointer type: `bool (*)(void*)`
 *
 * ## Worker Function Contract
 *
 * The worker function receives a `void*` to parameters and returns `bool`:
 * - Return `false`: Continue processing more items
 * - Return `true`: Signal "job done", stop all workers
 *
 * @note The pool takes ownership of scheduled `void*` pointers and frees
 * them with `free()`: they are allocated with `malloc()`, also those that
 * the pool does not pass to the worker function, because a call already
 * returned true or the pool shut down.
 *
 * @deprecated WorkersSet goes in 8.0.0: use ThreadPool (thread_pool.H).
 *
 * @ingroup Utilities
 *  @author Leandro Rabindranath León
 */
template <class WorkerFct>
class [[deprecated("WorkersSet goes in 8.0.0: use ThreadPool (thread_pool.H)")]]
WorkersSet
{
private:

  WorkerFct worker_fct = nullptr;
  size_t num_threads = 0;
  size_t queue_capacity = 0;
  mutable std::mutex m;  // all the state below; mutable, for is_jobs_done()
  std::condition_variable cond;

  std::atomic<size_t> num_workers = 0;

  FixedQueue<void*> q;
  DynList<std::thread> threads;
  bool shut_down = false;
  bool job_done = false;

  // Notified, with m, when the scheduled items are done or a worker
  // returned true
  std::condition_variable job_done_cond;

  void worker_handler()
  {
    std::unique_lock<std::mutex> lock(m);
    while (true)
      {
        cond.wait(lock, [this] { return num_workers > 0 or shut_down; });
         if (shut_down)
          return;

         while (num_workers > 0)
           {
             cond.wait(lock, [this]
             {
               return (not q.is_empty()) or shut_down;
             });
             
             if (shut_down)
               return;

             if (not q.is_empty())
               {
                 void * pars_ptr = q.get();

                 if (not job_done)
                   {
                     // The function runs without the lock; job_done is
                     // read and written only with it
                     lock.unlock();
                     bool done = false;
                     try
                       {
                         done = (*worker_fct)(pars_ptr);
                       }
                     catch (std::exception & e)
                       {
                         std::cout << "Warning: workers exception "
                                   << e.what() << std::endl;
                       }
                     catch (...)
                       {
                         std::cout << "Warning: workers exception" << std::endl;
                       }
                     lock.lock();
                     if (done)
                       {
                         job_done = true;
                         job_done_cond.notify_all();
                       }
                   }
                 free(pars_ptr);  // also an item that the function did not take
                 --num_workers;
               }
           }

         job_done_cond.notify_all();
      }
  }
  
public:

  /**
   * @brief Construct worker pool with specified thread count.
   *
   * @param worker_fct Function to execute for each work item
   * @param qsize Maximum number of queued work items
   * @param n Number of worker threads (default 16)
   */
  WorkersSet(WorkerFct worker_fct,
             const size_t qsize, const size_t n = 16)
    : worker_fct(worker_fct), num_threads(n), queue_capacity(qsize), q(qsize)
  {
    for (size_t i = 0; i < n; ++i)
      threads.append(std::thread(&WorkersSet::worker_handler, this));
  }

  /**
   * @brief Signal all workers to shut down.
   *
   * Workers will complete their current task and exit.
   */
  void shutdown()
  {
    std::unique_lock<std::mutex> lock(m);
    shut_down = true;
    cond.notify_all();
  }

  /// Destructor shuts down and joins all worker threads, and frees the
  /// items that are still queued
  ~WorkersSet()
  {
    shutdown();
    threads.mutable_for_each([] (std::thread & th) { th.join(); });
    while (not q.is_empty())
      free(q.get());
  }

  /**
   * @brief Set the expected number of work items.
   *
   * @param n Number of work items to expect
   */
  void prepare_num_workers(size_t n)
  {
    std::unique_lock<std::mutex> lock(m);
    num_workers = n;
    // The items scheduled before wait for it: wake the workers
    cond.notify_all();
  }

  /**
   * @brief Schedule a work item for execution.
   *
   * The work item is added to the queue and will be picked up
   * by an available worker thread.
   *
   * @param pars_ptr Pointer to parameters (freed after worker returns)
   *
   * @throw std::overflow_error if the queue already holds `qsize` items;
   *        the pool does not take pars_ptr then
   *
   * @note Caller allocates pars_ptr with malloc(); pool takes ownership and
   *       frees it with free().
   */
  void schedule_call(void * pars_ptr)
  {
    std::unique_lock<std::mutex> lock(m);
    ah_overflow_error_if(q.size() >= queue_capacity)
      << "WorkersSet::schedule_call(): the queue holds " << queue_capacity
      << " items";
    q.put(pars_ptr);
    cond.notify_one();
  }

  /**
   * @brief Check if a worker has signaled job completion.
   *
   * @return true if worker returned true (job done)
   */
  bool is_jobs_done() const
  {
    std::unique_lock<std::mutex> lock(m);
    return job_done;
  }

  /**
   * @brief Block until all work items processed or job done signaled.
   *
   * Returns when either:
   * - All scheduled work items have been processed, or
   * - A worker function returned true (job done)
   */
  void wait_until_all_workers_finished_or_job_is_done()
  {
    // With a predicate: the items may be done before the wait starts, and
    // a wakeup may be spurious
    std::unique_lock<std::mutex> lock(m);
    job_done_cond.wait(lock, [this] { return num_workers == 0 or job_done; });
  }
};


# endif
