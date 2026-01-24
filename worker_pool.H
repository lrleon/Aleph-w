
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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
 *  // Schedule work items
 *  for (auto& item : items) {
 *    auto* data = new MyData(item);
 *    pool.schedule_call(data);
 *  }
 *
 *  pool.wait_until_all_workers_finished_or_job_is_done();
 *  ```
 *
 *  @see q-consumer-threads.H Queue-based variant
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef WORKER_POOL_H
# define WORKER_POOL_H

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
 * @note The pool takes ownership of scheduled `void*` pointers and frees them.
 *
 * @ingroup Utilities
 *  @author Leandro Rabindranath León
 */
template <class WorkerFct>
class WorkersSet
{
private:

  WorkerFct worker_fct = nullptr;
  size_t num_threads = 0;
  std::mutex m;
  std::condition_variable cond;

  std::atomic<size_t> num_workers = 0;

  FixedQueue<void*> q;
  DynList<std::thread> threads;
  bool shut_down = false;
  bool job_done = false;

  std::condition_variable job_done_cond;
  std::mutex job_done_mutex;

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
                     lock.unlock();
                     try
                       {
                         job_done = (*worker_fct)(pars_ptr);
                       }
                    catch (std::exception & e)
                      {
                        std::cout << "Warning: workers exception "
                                  << e.what() << std::endl;
                       }
                     lock.lock();
                     free(pars_ptr);
                   }
                 --num_workers;
               }
           }

         assert(q.is_empty());
         job_done_cond.notify_one();
         continue;
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
    : worker_fct(worker_fct), num_threads(n), q(qsize)
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

  /// Destructor shuts down and joins all worker threads
  ~WorkersSet()
  {
    shutdown();
    threads.mutable_for_each([] (std::thread & th) { th.join(); });
  }

  /**
   * @brief Set expected number of work items.
   *
   * @param n Number of work items to expect
   */
  void prepare_num_workers(size_t n)
  {
    std::unique_lock<std::mutex> lock(m);
    num_workers = n;
  }

  /**
   * @brief Schedule a work item for execution.
   *
   * The work item is added to the queue and will be picked up
   * by an available worker thread.
   *
   * @param pars_ptr Pointer to parameters (freed after worker returns)
   *
   * @note Caller allocates pars_ptr; pool takes ownership and frees it.
   */
  void schedule_call(void * pars_ptr)
  {
    std::unique_lock<std::mutex> lock(m);
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
    std::unique_lock<std::mutex> lock(job_done_mutex);
    job_done_cond.wait(lock);
  }
};


# endif
