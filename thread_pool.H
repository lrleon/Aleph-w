/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

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

/** @file thread_pool.H
 *  @brief A modern, efficient thread pool for parallel task execution.
 *
 *  This file provides a reusable thread pool that maintains a fixed number
 *  of worker threads, avoiding the overhead of creating new threads for
 *  each task (as happens with `std::async`).
 *
 *  ## Features
 *
 *  - **Zero thread creation overhead**: Workers are pre-created and reused
 *  - **Type-safe**: Full template support with automatic return type deduction
 *  - **Future-based**: Returns `std::future<T>` for result retrieval
 *  - **Exception-safe**: Exceptions propagate through futures
 *  - **Resizable**: Can dynamically adjust the number of workers
 *  - **Move-only support**: Accepts move-only callables and arguments
 *  - **Member functions**: Direct support via `std::invoke`
 *
 *  ## Performance Comparison
 *
 *  | Method | Overhead per task |
 *  |--------|-------------------|
 *  | `std::async` | 10-100 μs (thread creation) |
 *  | `ThreadPool` | ~50-100 ns (queue + notify) |
 *
 *  ## Supported Callable Types
 *
 *  - Lambdas (including move-only captures)
 *  - Free functions
 *  - Function pointers
 *  - Functors (function objects)
 *  - `std::function`
 *  - Member function pointers (with object)
 *  - `std::bind` expressions
 *
 *  ## Usage Examples
 *
 *  ```cpp
 *  #include <thread_pool.H>
 *
 *  Aleph::ThreadPool pool(4);
 *
 *  // Lambda with arguments
 *  auto f1 = pool.enqueue([](int x, int y) { return x + y; }, 10, 20);
 *
 *  // Free function
 *  auto f2 = pool.enqueue(std::sqrt, 16.0);
 *
 *  // Member function
 *  struct Calculator {
 *    int multiply(int a, int b) { return a * b; }
 *  };
 *  Calculator calc;
 *  auto f3 = pool.enqueue(&Calculator::multiply, &calc, 6, 7);
 *
 *  // Move-only lambda
 *  auto ptr = std::make_unique<int>(42);
 *  auto f4 = pool.enqueue([p = std::move(ptr)]() { return *p; });
 *
 *  // Fire-and-forget (no future overhead)
 *  pool.enqueue_detached([] { std::cout << "Background task\n"; });
 *
 *  std::cout << f1.get() << " " << f2.get() << " " << f3.get() << " " << f4.get();
 *  // Output: 30 4 42 42
 *  ```
 *
 *  ## Thread Safety
 *
 *  - `enqueue()` and `enqueue_detached()` are thread-safe
 *  - `resize()` and `shutdown()` should not be called concurrently with each other
 *
 *  @see worker_pool.H Alternative with different API
 *  @see future_utils.H Utilities for working with futures
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

#ifndef ALEPH_THREAD_POOL_H
#define ALEPH_THREAD_POOL_H

#include <future>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <memory>
#include <limits>
#include <optional>
#include <chrono>
#include <numeric>
#include <iterator>

namespace Aleph
{

/** @brief Exception thrown when the task queue exceeds its hard limit.
 *
 *  This exception is thrown by `enqueue_bounded()` when the queue size
 *  reaches the hard limit, indicating potential memory exhaustion.
 */
class queue_overflow_error : public std::overflow_error
{
public:
  explicit queue_overflow_error(size_t current_size, size_t hard_limit)
    : std::overflow_error("ThreadPool queue overflow: " + 
                          std::to_string(current_size) + " >= hard_limit " +
                          std::to_string(hard_limit)),
      current_size_(current_size), hard_limit_(hard_limit) {}

  /// Current queue size when exception was thrown
  [[nodiscard]] size_t current_size() const noexcept { return current_size_; }
  
  /// Hard limit that was exceeded
  [[nodiscard]] size_t hard_limit() const noexcept { return hard_limit_; }

private:
  size_t current_size_;
  size_t hard_limit_;
};

/** @brief Statistics collected by ThreadPool.
 *
 *  Provides insight into pool performance for monitoring and tuning.
 */
struct ThreadPoolStats
{
  size_t tasks_completed = 0;         ///< Total tasks completed
  size_t tasks_failed = 0;            ///< Tasks that threw exceptions (detached)
  size_t current_queue_size = 0;      ///< Current pending tasks
  size_t current_active = 0;          ///< Currently executing tasks
  size_t num_workers = 0;             ///< Number of worker threads
  size_t peak_queue_size = 0;         ///< Maximum queue size observed
  
  /// Total tasks processed (completed + failed)
  [[nodiscard]] size_t total_processed() const noexcept 
  { 
    return tasks_completed + tasks_failed; 
  }
  
  /// Queue utilization as percentage (0-100)
  [[nodiscard]] double queue_utilization(size_t soft_limit) const noexcept
  {
    if (soft_limit == 0 || soft_limit == std::numeric_limits<size_t>::max())
      return 0.0;
    return (100.0 * current_queue_size) / soft_limit;
  }
};

/// Type for exception callback in detached tasks
using ExceptionCallback = std::function<void(std::exception_ptr)>;

/** @brief A reusable thread pool for efficient parallel task execution.
 *
 *  Maintains a pool of worker threads that wait for tasks and execute them.
 *  This avoids the overhead of creating and destroying threads for each task.
 *
 *  The pool uses a shared task queue protected by a mutex. Workers wait on
 *  a condition variable and are notified when new tasks arrive.
 *
 *  ## Implementation Notes
 *
 *  This implementation uses `std::invoke` with `std::tuple` + `std::apply`
 *  instead of `std::bind` for better performance and move-only support.
 *  The overhead per task is approximately 50-100 nanoseconds.
 *
 *  @note The pool automatically joins all workers on destruction.
 *  @note Tasks that throw exceptions will have those exceptions stored in
 *        the returned future and rethrown when `get()` is called.
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */
class ThreadPool
{
  /// Type-erased task wrapper
  struct TaskBase
  {
    virtual ~TaskBase() = default;
    virtual void execute() = 0;
  };

  /// Concrete task implementation with type preservation
  template <typename F>
  struct Task : TaskBase
  {
    F func;

    explicit Task(F&& f) : func(std::move(f)) {}

    void execute() override { func(); }
  };

  std::vector<std::thread> workers;
  std::queue<std::unique_ptr<TaskBase>> tasks;
  mutable std::mutex queue_mutex;
  std::condition_variable condition;        ///< Notifies workers of new tasks
  std::condition_variable space_available;  ///< Notifies enqueuers of queue space
  std::condition_variable idle_condition;   ///< Notifies wait_all() of idle state
  std::atomic<bool> stop{false};
  std::atomic<size_t> active_tasks{0};
  
  // Queue limits for bounded enqueue
  size_t soft_limit_ = std::numeric_limits<size_t>::max();  ///< Block threshold
  size_t hard_limit_ = std::numeric_limits<size_t>::max();  ///< Exception threshold
  
  // Statistics
  std::atomic<size_t> tasks_completed_{0};
  std::atomic<size_t> tasks_failed_{0};
  std::atomic<size_t> peak_queue_size_{0};
  
  // Exception callback for detached tasks
  ExceptionCallback exception_callback_;

  /// Worker thread main loop
  void worker_loop()
  {
    while (true)
      {
        std::unique_ptr<TaskBase> task;
        bool should_notify_space = false;
        {
          std::unique_lock<std::mutex> lock(queue_mutex);
          condition.wait(lock, [this] {
            return stop or not tasks.empty();
          });

          if (stop and tasks.empty())
            return;

          // Check if we should notify waiters after popping
          should_notify_space = (tasks.size() == soft_limit_);
          
          task = std::move(tasks.front());
          tasks.pop();
          ++active_tasks;
        }

        // Notify any blocked enqueuers that space is available
        if (should_notify_space)
          space_available.notify_all();

        task->execute();
        
        // Update stats and check for idle state
        --active_tasks;
        
        // Notify wait_all if we're now idle
        {
          std::unique_lock<std::mutex> lock(queue_mutex);
          if (tasks.empty() && active_tasks == 0)
            idle_condition.notify_all();
        }
      }
  }

  /// Start n worker threads
  void start_workers(size_t n)
  {
    workers.reserve(n);
    for (size_t i = 0; i < n; ++i)
      workers.emplace_back(&ThreadPool::worker_loop, this);
  }

  /// Stop and join all workers
  void stop_workers()
  {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();

    for (auto& worker : workers)
      if (worker.joinable())
        worker.join();

    workers.clear();
  }

  /// Helper to create a callable from function + arguments using std::invoke
  template <typename F, typename... Args>
  static auto make_invocable(F&& f, Args&&... args)
  {
    // Decay types to store copies/moves, not references
    return [func = std::forward<F>(f),
            args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
    {
      return std::apply([&func](auto&&... a) {
        return std::invoke(std::move(func), std::forward<decltype(a)>(a)...);
      }, std::move(args_tuple));
    };
  }

public:
  /** @brief Construct a thread pool with specified number of workers.
   *
   *  @param n_threads Number of worker threads to create.
   *                   Defaults to `std::thread::hardware_concurrency()`.
   */
  explicit ThreadPool(size_t n_threads = std::thread::hardware_concurrency())
  {
    if (n_threads == 0)
      n_threads = 1;  // Ensure at least one worker

    start_workers(n_threads);
  }

  /// Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  /// Non-movable (due to mutex and condition_variable)
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  /** @brief Destructor - stops all workers and waits for completion.
   *
   *  All pending tasks in the queue will be executed before shutdown.
   */
  ~ThreadPool()
  {
    stop_workers();
  }

  /** @brief Submit a task for execution and get a future for the result.
   *
   *  This is the most general overload, supporting:
   *  - Any callable (lambda, function, functor, member function pointer)
   *  - Any number of arguments
   *  - Move-only callables and arguments
   *  - Member functions with object pointer/reference
   *  - Reference arguments via `std::ref()` / `std::cref()`
   *
   *  @tparam F    Callable type.
   *  @tparam Args Argument types for the callable.
   *
   *  @param f    The callable to execute.
   *  @param args Arguments to pass to the callable (via std::invoke).
   *              Use `std::ref(x)` to pass by reference.
   *
   *  @return A `std::future` that will hold the result (or exception).
   *
   *  @throw std::runtime_error if the pool has been shut down.
   *
   *  ## Examples
   *
   *  ```cpp
   *  // Lambda
   *  auto f1 = pool.enqueue([](int x) { return x * 2; }, 21);
   *
   *  // Free function
   *  auto f2 = pool.enqueue(std::sqrt, 16.0);
   *
   *  // Member function pointer (std::invoke syntax)
   *  struct Foo { int bar(int x) { return x + 1; } };
   *  Foo foo;
   *  auto f3 = pool.enqueue(&Foo::bar, &foo, 41);
   *
   *  // Move-only capture
   *  auto ptr = std::make_unique<int>(100);
   *  auto f4 = pool.enqueue([p = std::move(ptr)]() { return *p; });
   *
   *  // Reference argument (use std::ref)
   *  int value = 0;
   *  pool.enqueue([](int& x) { ++x; }, std::ref(value));
   *  ```
   */
  template <typename F, typename... Args>
  [[nodiscard]] auto enqueue(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
  {
    using return_type = std::invoke_result_t<F, Args...>;

    // Create the invocable that captures function and arguments
    auto invocable = make_invocable(std::forward<F>(f), std::forward<Args>(args)...);

    // Wrap in packaged_task for future support
    auto promise = std::make_shared<std::promise<return_type>>();
    std::future<return_type> result = promise->get_future();

    // Capture stats counter for task completion tracking
    auto completed_counter = &tasks_completed_;
    
    // Create the task that will execute and set the promise
    auto task_func = [invocable = std::move(invocable), promise, completed_counter]() mutable {
      try
        {
          if constexpr (std::is_void_v<return_type>)
            {
              invocable();
              promise->set_value();
            }
          else
            {
              promise->set_value(invocable());
            }
          ++(*completed_counter);
        }
      catch (...)
        {
          promise->set_exception(std::current_exception());
          ++(*completed_counter);  // Still counts as processed
        }
    };

    {
      std::unique_lock<std::mutex> lock(queue_mutex);

      if (stop)
        throw std::runtime_error("enqueue on stopped ThreadPool");

      tasks.push(std::make_unique<Task<decltype(task_func)>>(std::move(task_func)));
      
      // Update peak queue size
      size_t current_size = tasks.size();
      size_t peak = peak_queue_size_.load();
      while (current_size > peak and
             not peak_queue_size_.compare_exchange_weak(peak, current_size))
        ;
    }

    condition.notify_one();
    return result;
  }

  /** @brief Submit a task without tracking the result (fire-and-forget).
   *
   *  More efficient than `enqueue()` when you don't need the result,
   *  as it avoids the overhead of promise/future.
   *
   *  @tparam F    Callable type.
   *  @tparam Args Argument types for the callable.
   *
   *  @param f    The callable to execute.
   *  @param args Arguments to pass to the callable.
   *
   *  @throw std::runtime_error if the pool has been shut down.
   *
   *  @warning Exceptions thrown by the task will be silently ignored.
   *           Use `enqueue()` if you need exception handling.
   *
   *  ## Example
   *
   *  ```cpp
   *  // Fire-and-forget logging
   *  pool.enqueue_detached([](const std::string& msg) {
   *    std::ofstream log("app.log", std::ios::app);
   *    log << msg << std::endl;
   *  }, "User logged in");
   *  ```
   */
  template <typename F, typename... Args>
  void enqueue_detached(F&& f, Args&&... args)
  {
    auto invocable = make_invocable(std::forward<F>(f), std::forward<Args>(args)...);

    // Capture stats and callback
    auto completed_counter = &tasks_completed_;
    auto failed_counter = &tasks_failed_;
    ExceptionCallback callback;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      callback = exception_callback_;
    }

    auto task_func = [invocable = std::move(invocable), 
                      completed_counter, failed_counter,
                      callback = std::move(callback)]() mutable {
      try
        {
          invocable();
          ++(*completed_counter);
        }
      catch (...)
        {
          ++(*failed_counter);
          // Call exception callback if set
          if (callback)
            {
              try { callback(std::current_exception()); }
              catch (...) { /* Ignore callback exceptions */ }
            }
        }
    };

    {
      std::unique_lock<std::mutex> lock(queue_mutex);

      if (stop)
        throw std::runtime_error("enqueue_detached on stopped ThreadPool");

      tasks.push(std::make_unique<Task<decltype(task_func)>>(std::move(task_func)));
      
      // Update peak queue size
      size_t current_size = tasks.size();
      size_t peak = peak_queue_size_.load();
      while (current_size > peak and
             not peak_queue_size_.compare_exchange_weak(peak, current_size))
        ;
    }

    condition.notify_one();
  }

  /** @brief Submit multiple tasks and collect all futures.
   *
   *  Convenience method for submitting a batch of similar tasks.
   *
   *  @tparam F    Callable type.
   *  @tparam Container Container of argument sets (e.g., vector<tuple<Args...>>).
   *
   *  @param f    The callable to execute for each argument set.
   *  @param args_container Container where each element is the arguments for one call.
   *
   *  @return Vector of futures for all submitted tasks.
   *
   *  ## Example
   *
   *  ```cpp
   *  std::vector<int> inputs = {1, 2, 3, 4, 5};
   *  auto futures = pool.enqueue_bulk([](int x) { return x * x; }, inputs);
   *  // futures contains: future<1>, future<4>, future<9>, future<16>, future<25>
   *  ```
   */
  template <typename F, typename Container>
  [[nodiscard]] auto enqueue_bulk(F&& f, const Container& args_container)
    -> std::vector<std::future<std::invoke_result_t<F, typename Container::value_type>>>
  {
    using return_type = std::invoke_result_t<F, typename Container::value_type>;
    std::vector<std::future<return_type>> results;
    results.reserve(args_container.size());

    for (const auto& arg : args_container)
      results.push_back(enqueue(f, arg));

    return results;
  }

  /** @brief Set queue limits for bounded enqueue operations.
   *
   *  Configures backpressure and memory protection for the task queue.
   *
   *  @param soft_limit When queue reaches this size, `enqueue_bounded()` blocks
   *                    until space is available. Set to SIZE_MAX to disable blocking.
   *  @param hard_limit When queue reaches this size, `enqueue_bounded()` throws
   *                    `queue_overflow_error`. Defaults to 10x soft_limit.
   *                    Set to SIZE_MAX to disable the hard limit.
   *
   *  @note These limits only affect `enqueue_bounded()` and `enqueue_bounded_detached()`.
   *        Regular `enqueue()` and `enqueue_detached()` are unaffected.
   *
   *  ## Example
   *
   *  ```cpp
   *  ThreadPool pool(4);
   *  pool.set_queue_limits(1000);        // soft=1000, hard=10000
   *  pool.set_queue_limits(1000, 5000);  // soft=1000, hard=5000
   *  ```
   */
  void set_queue_limits(size_t soft_limit, 
                        size_t hard_limit = std::numeric_limits<size_t>::max())
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    soft_limit_ = soft_limit;
    hard_limit_ = (hard_limit == std::numeric_limits<size_t>::max() && 
                   soft_limit != std::numeric_limits<size_t>::max())
                  ? soft_limit * 10  // Default hard = 10x soft
                  : hard_limit;
  }

  /** @brief Get current queue limits.
   *
   *  @return Pair of (soft_limit, hard_limit).
   */
  [[nodiscard]] std::pair<size_t, size_t> get_queue_limits() const
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return {soft_limit_, hard_limit_};
  }

  /** @brief Submit a task with backpressure and memory protection.
   *
   *  Unlike `enqueue()`, this method respects queue limits:
   *  - If `pending_tasks() >= soft_limit`: blocks until space available
   *  - If `pending_tasks() >= hard_limit`: throws `queue_overflow_error`
   *
   *  This provides natural backpressure (producer slows down) and
   *  protects against memory exhaustion.
   *
   *  @tparam F    Callable type.
   *  @tparam Args Argument types for the callable.
   *
   *  @param f    The callable to execute.
   *  @param args Arguments to pass to the callable.
   *
   *  @return A `std::future` that will hold the result.
   *
   *  @throw queue_overflow_error if queue size >= hard_limit.
   *  @throw std::runtime_error if the pool has been shut down.
   *
   *  ## Example
   *
   *  ```cpp
   *  ThreadPool pool(4);
   *  pool.set_queue_limits(100, 1000);  // soft=100, hard=1000
   *
   *  try {
   *    for (int i = 0; i < 10000; ++i)
   *      pool.enqueue_bounded(expensive_task);  // Will block/throw appropriately
   *  } catch (const queue_overflow_error& e) {
   *    std::cerr << "Queue overflow: " << e.what() << std::endl;
   *  }
   *  ```
   */
  template <typename F, typename... Args>
  [[nodiscard]] auto enqueue_bounded(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
  {
    using return_type = std::invoke_result_t<F, Args...>;

    auto invocable = make_invocable(std::forward<F>(f), std::forward<Args>(args)...);

    auto promise = std::make_shared<std::promise<return_type>>();
    std::future<return_type> result = promise->get_future();

    auto task_func = [invocable = std::move(invocable), promise]() mutable {
      try
        {
          if constexpr (std::is_void_v<return_type>)
            {
              invocable();
              promise->set_value();
            }
          else
            {
              promise->set_value(invocable());
            }
        }
      catch (...)
        {
          promise->set_exception(std::current_exception());
        }
    };

    {
      std::unique_lock<std::mutex> lock(queue_mutex);

      // Check hard limit first (throws immediately)
      if (tasks.size() >= hard_limit_)
        throw queue_overflow_error(tasks.size(), hard_limit_);

      // Block if at soft limit (wait for space)
      space_available.wait(lock, [this] {
        return stop || tasks.size() < soft_limit_;
      });

      if (stop)
        throw std::runtime_error("enqueue_bounded on stopped ThreadPool");

      // Re-check hard limit after waking up
      if (tasks.size() >= hard_limit_)
        throw queue_overflow_error(tasks.size(), hard_limit_);

      tasks.push(std::make_unique<Task<decltype(task_func)>>(std::move(task_func)));
    }

    condition.notify_one();
    return result;
  }

  /** @brief Submit a task with backpressure, without tracking result.
   *
   *  Fire-and-forget version of `enqueue_bounded()`. Respects queue limits
   *  but doesn't return a future.
   *
   *  @tparam F    Callable type.
   *  @tparam Args Argument types for the callable.
   *
   *  @param f    The callable to execute.
   *  @param args Arguments to pass to the callable.
   *
   *  @throw queue_overflow_error if queue size >= hard_limit.
   *  @throw std::runtime_error if the pool has been shut down.
   */
  template <typename F, typename... Args>
  void enqueue_bounded_detached(F&& f, Args&&... args)
  {
    auto invocable = make_invocable(std::forward<F>(f), std::forward<Args>(args)...);

    auto task_func = [invocable = std::move(invocable)]() mutable {
      try
        {
          invocable();
        }
      catch (...)
        {
          // Silently ignore exceptions in detached tasks
        }
    };

    {
      std::unique_lock<std::mutex> lock(queue_mutex);

      // Check hard limit first
      if (tasks.size() >= hard_limit_)
        throw queue_overflow_error(tasks.size(), hard_limit_);

      // Block if at soft limit
      space_available.wait(lock, [this] {
        return stop || tasks.size() < soft_limit_;
      });

      if (stop)
        throw std::runtime_error("enqueue_bounded_detached on stopped ThreadPool");

      // Re-check hard limit after waking up
      if (tasks.size() >= hard_limit_)
        throw queue_overflow_error(tasks.size(), hard_limit_);

      tasks.push(std::make_unique<Task<decltype(task_func)>>(std::move(task_func)));
    }

    condition.notify_one();
  }

  /** @brief Try to submit a task without blocking or throwing.
   *
   *  Non-blocking version of `enqueue_bounded()`. Returns immediately with
   *  either a future (if task was queued) or `std::nullopt` (if queue is full).
   *
   *  @tparam F    Callable type.
   *  @tparam Args Argument types for the callable.
   *
   *  @param f    The callable to execute.
   *  @param args Arguments to pass to the callable.
   *
   *  @return `std::optional<std::future<R>>` - contains the future if the task
   *          was successfully queued, or `std::nullopt` if the queue is at or
   *          above the soft limit.
   *
   *  @throw std::runtime_error only if the pool has been shut down.
   *
   *  @note Uses `soft_limit` as the threshold. Configure with `set_queue_limits()`.
   *
   *  ## Example
   *
   *  ```cpp
   *  ThreadPool pool(4);
   *  pool.set_queue_limits(100);
   *
   *  if (auto future = pool.try_enqueue(compute, arg)) {
   *    // Task was queued, can use future->get() later
   *    results.push_back(std::move(*future));
   *  } else {
   *    // Queue is full, handle backpressure
   *    std::cerr << "Queue full, dropping task\n";
   *  }
   *  ```
   */
  template <typename F, typename... Args>
  [[nodiscard]] auto try_enqueue(F&& f, Args&&... args)
    -> std::optional<std::future<std::invoke_result_t<F, Args...>>>
  {
    using return_type = std::invoke_result_t<F, Args...>;

    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      
      if (stop)
        throw std::runtime_error("try_enqueue on stopped ThreadPool");
      
      // Check if queue is at or above soft limit
      if (tasks.size() >= soft_limit_)
        return std::nullopt;
    }

    // Queue has space, delegate to regular enqueue
    return enqueue(std::forward<F>(f), std::forward<Args>(args)...);
  }

  /** @brief Try to submit a detached task without blocking or throwing.
   *
   *  Non-blocking version of `enqueue_bounded_detached()`.
   *
   *  @tparam F    Callable type.
   *  @tparam Args Argument types for the callable.
   *
   *  @param f    The callable to execute.
   *  @param args Arguments to pass to the callable.
   *
   *  @return `true` if the task was queued, `false` if the queue is full.
   *
   *  @throw std::runtime_error only if the pool has been shut down.
   *
   *  ## Example
   *
   *  ```cpp
   *  if (!pool.try_enqueue_detached(log_message, msg)) {
   *    // Queue full, message dropped
   *  }
   *  ```
   */
  template <typename F, typename... Args>
  bool try_enqueue_detached(F&& f, Args&&... args)
  {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      
      if (stop)
        throw std::runtime_error("try_enqueue_detached on stopped ThreadPool");
      
      if (tasks.size() >= soft_limit_)
        return false;
    }

    enqueue_detached(std::forward<F>(f), std::forward<Args>(args)...);
    return true;
  }

  /** @brief Get the number of worker threads.
   *  @return Number of workers in the pool.
   */
  [[nodiscard]] size_t num_threads() const noexcept
  {
    return workers.size();
  }

  /** @brief Get the number of pending tasks in the queue.
   *  @return Number of tasks waiting to be executed.
   */
  [[nodiscard]] size_t pending_tasks() const
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return tasks.size();
  }

  /** @brief Get the number of tasks currently being executed.
   *  @return Number of active tasks.
   */
  [[nodiscard]] size_t running_tasks() const noexcept
  {
    return active_tasks.load();
  }

  /** @brief Check if the pool is idle (no pending or running tasks).
   *  @return true if no tasks are queued or running.
   */
  [[nodiscard]] bool is_idle() const
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return tasks.empty() && active_tasks == 0;
  }

  /** @brief Check if the pool has been shut down.
   *  @return true if shutdown() was called or destructor is running.
   */
  [[nodiscard]] bool is_stopped() const noexcept
  {
    return stop.load();
  }

  /** @brief Shut down the pool, completing all pending tasks.
   *
   *  After calling this method:
   *  - No new tasks can be enqueued (will throw)
   *  - All pending tasks will complete
   *  - All workers will be joined
   *
   *  @note This method blocks until all workers have finished.
   *  @note Calling shutdown() multiple times is safe (no-op after first).
   */
  void shutdown()
  {
    if (stop.exchange(true))
      return;  // Already stopped

    condition.notify_all();

    for (auto& worker : workers)
      if (worker.joinable())
        worker.join();

    workers.clear();
  }

  /** @brief Resize the pool to a different number of workers.
   *
   *  Stops all current workers and starts a new set with the specified size.
   *  Any tasks currently in the queue will be preserved and executed by
   *  the new workers.
   *
   *  @param new_size New number of worker threads.
   *
   *  @warning This method should not be called while tasks are being
   *           enqueued from other threads.
   *
   *  @throw std::runtime_error if the pool has been shut down.
   */
  void resize(size_t new_size)
  {
    if (new_size == 0)
      new_size = 1;

    if (new_size == workers.size())
      return;

    if (stop)
      throw std::runtime_error("cannot resize a stopped ThreadPool");

    // Stop current workers (but don't clear tasks)
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();

    for (auto& worker : workers)
      if (worker.joinable())
        worker.join();

    workers.clear();

    // Restart with new size
    stop = false;
    start_workers(new_size);
  }

  /** @brief Wait until all current tasks complete.
   *
   *  Blocks until the task queue is empty and no tasks are running.
   *  New tasks can still be enqueued while waiting.
   *
   *  @param poll_interval How often to check for completion (default 1ms).
   */
  void wait_all(std::chrono::milliseconds poll_interval = std::chrono::milliseconds(1))
  {
    while (not is_idle())
      std::this_thread::sleep_for(poll_interval);
  }

  /** @brief Wait until all current tasks complete, with timeout.
   *
   *  Blocks until either:
   *  - The task queue is empty and no tasks are running, OR
   *  - The timeout expires
   *
   *  @param timeout Maximum time to wait.
   *  @return true if pool became idle, false if timeout expired.
   */
  template <typename Rep, typename Period>
  [[nodiscard]] bool wait_all_for(std::chrono::duration<Rep, Period> timeout)
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return idle_condition.wait_for(lock, timeout, [this] {
      return tasks.empty() && active_tasks == 0;
    });
  }

  /** @brief Wait until all current tasks complete, with deadline.
   *
   *  Blocks until either:
   *  - The task queue is empty and no tasks are running, OR
   *  - The deadline is reached
   *
   *  @param deadline Time point at which to stop waiting.
   *  @return true if pool became idle, false if deadline reached.
   */
  template <typename Clock, typename Duration>
  [[nodiscard]] bool wait_all_until(std::chrono::time_point<Clock, Duration> deadline)
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return idle_condition.wait_until(lock, deadline, [this] {
      return tasks.empty() && active_tasks == 0;
    });
  }

  /** @brief Get current pool statistics.
   *
   *  Returns a snapshot of various performance counters.
   *
   *  @return ThreadPoolStats struct with current values.
   */
  [[nodiscard]] ThreadPoolStats get_stats() const
  {
    ThreadPoolStats stats;
    stats.tasks_completed = tasks_completed_.load();
    stats.tasks_failed = tasks_failed_.load();
    stats.current_active = active_tasks.load();
    stats.num_workers = workers.size();
    stats.peak_queue_size = peak_queue_size_.load();
    
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stats.current_queue_size = tasks.size();
    }
    
    return stats;
  }

  /** @brief Reset statistics counters to zero.
   */
  void reset_stats()
  {
    tasks_completed_ = 0;
    tasks_failed_ = 0;
    peak_queue_size_ = 0;
  }

  /** @brief Set callback for exceptions in detached tasks.
   *
   *  When a detached task throws an exception, this callback is invoked
   *  with the exception pointer. This allows logging or handling of
   *  exceptions that would otherwise be silently ignored.
   *
   *  @param callback Function to call with exception_ptr on task failure.
   *                  Pass nullptr to disable.
   *
   *  ## Example
   *
   *  ```cpp
   *  pool.set_exception_callback([](std::exception_ptr ep) {
   *    try {
   *      std::rethrow_exception(ep);
   *    } catch (const std::exception& e) {
   *      std::cerr << "Detached task failed: " << e.what() << std::endl;
   *    }
   *  });
   *  ```
   */
  void set_exception_callback(ExceptionCallback callback)
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    exception_callback_ = std::move(callback);
  }

  /** @brief Submit multiple tasks with a single notification.
   *
   *  More efficient than calling enqueue() multiple times when you have
   *  many tasks to submit, as it reduces mutex contention and notification
   *  overhead.
   *
   *  @tparam F    Callable type.
   *  @tparam Container Container of argument tuples.
   *
   *  @param f    The callable to execute for each argument set.
   *  @param args_list Container where each element is the arguments for one call.
   *
   *  @return Vector of futures for all submitted tasks.
   *
   *  ## Example
   *
   *  ```cpp
   *  std::vector<std::tuple<int, int>> work = {{1, 2}, {3, 4}, {5, 6}};
   *  auto futures = pool.enqueue_batch([](int a, int b) { return a + b; }, work);
   *  // Submits all 3 tasks with one lock acquisition
   *  ```
   */
  template <typename F, typename Container>
  [[nodiscard]] auto enqueue_batch(F&& f, const Container& args_list)
    -> std::vector<std::future<decltype(std::apply(f, *std::begin(args_list)))>>
  {
    using ArgsTuple = typename Container::value_type;
    using return_type = decltype(std::apply(f, std::declval<ArgsTuple>()));
    
    std::vector<std::future<return_type>> results;
    results.reserve(std::distance(std::begin(args_list), std::end(args_list)));
    
    std::vector<std::unique_ptr<TaskBase>> batch_tasks;
    batch_tasks.reserve(results.capacity());
    
    // Capture stats counter
    auto completed_counter = &tasks_completed_;
    
    for (const auto& args : args_list)
      {
        auto promise = std::make_shared<std::promise<return_type>>();
        results.push_back(promise->get_future());
        
        auto task_func = [func = f, args, promise, completed_counter]() mutable {
          try
            {
              if constexpr (std::is_void_v<return_type>)
                {
                  std::apply(func, args);
                  promise->set_value();
                }
              else
                {
                  promise->set_value(std::apply(func, args));
                }
              ++(*completed_counter);
            }
          catch (...)
            {
              promise->set_exception(std::current_exception());
              ++(*completed_counter);
            }
        };
        
        batch_tasks.push_back(
          std::make_unique<Task<decltype(task_func)>>(std::move(task_func)));
      }
    
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      
      if (stop)
        throw std::runtime_error("enqueue_batch on stopped ThreadPool");
      
      for (auto& task : batch_tasks)
        tasks.push(std::move(task));
      
      // Update peak queue size
      size_t current_size = tasks.size();
      size_t peak = peak_queue_size_.load();
      while (current_size > peak and
             not peak_queue_size_.compare_exchange_weak(peak, current_size))
        ;
    }
    
    // Notify all workers for batch
    condition.notify_all();
    
    return results;
  }
};

// =============================================================================
// Parallel Algorithms
// =============================================================================

/** @brief Execute a function in parallel over a range.
 *
 *  Divides the range [begin, end) into chunks and executes the function
 *  on each chunk in parallel using the thread pool.
 *
 *  @tparam Iterator Random access iterator type.
 *  @tparam F        Function type: void(Iterator begin, Iterator end) or void(T&).
 *
 *  @param pool  Thread pool to use.
 *  @param begin Start of range.
 *  @param end   End of range.
 *  @param f     Function to apply. Can be:
 *               - void(Iterator, Iterator) - receives chunk bounds
 *               - void(T&) - receives each element by reference
 *  @param chunk_size Number of elements per chunk (0 = auto-calculate).
 *
 *  ## Example
 *
 *  ```cpp
 *  std::vector<int> data(10000);
 *  
 *  // Per-element function
 *  parallel_for(pool, data.begin(), data.end(), [](int& x) { x *= 2; });
 *  
 *  // Chunk function
 *  parallel_for(pool, data.begin(), data.end(), 
 *               [](auto begin, auto end) {
 *                 for (auto it = begin; it != end; ++it) *it *= 2;
 *               });
 *  ```
 */
template <typename Iterator, typename F>
void parallel_for(ThreadPool& pool, Iterator begin, Iterator end, F&& f, 
                  size_t chunk_size = 0)
{
  using value_type = typename std::iterator_traits<Iterator>::value_type;
  
  const size_t total = std::distance(begin, end);
  if (total == 0)
    return;
  
  // Auto-calculate chunk size if not specified
  if (chunk_size == 0)
    chunk_size = std::max(size_t(1), total / (pool.num_threads() * 4));
  
  std::vector<std::future<void>> futures;
  futures.reserve((total + chunk_size - 1) / chunk_size);
  
  // Check if F accepts (Iterator, Iterator) or (T&)
  if constexpr (std::is_invocable_v<F, Iterator, Iterator>)
    {
      // Chunk-based function
      for (Iterator chunk_begin = begin; chunk_begin < end; )
        {
          Iterator chunk_end = chunk_begin;
          std::advance(chunk_end, std::min(chunk_size, 
                       static_cast<size_t>(std::distance(chunk_begin, end))));
          
          futures.push_back(pool.enqueue([f, chunk_begin, chunk_end]() {
            f(chunk_begin, chunk_end);
          }));
          
          chunk_begin = chunk_end;
        }
    }
  else
    {
      // Per-element function
      for (Iterator chunk_begin = begin; chunk_begin < end; )
        {
          Iterator chunk_end = chunk_begin;
          std::advance(chunk_end, std::min(chunk_size, 
                       static_cast<size_t>(std::distance(chunk_begin, end))));
          
          futures.push_back(pool.enqueue([f, chunk_begin, chunk_end]() {
            for (Iterator it = chunk_begin; it != chunk_end; ++it)
              f(*it);
          }));
          
          chunk_begin = chunk_end;
        }
    }
  
  // Wait for all chunks to complete
  for (auto& fut : futures)
    fut.get();
}

/** @brief Transform elements in parallel and store results.
 *
 *  Applies a transformation function to each element and stores the result.
 *  Similar to std::transform but parallelized.
 *
 *  @tparam InputIt  Input iterator type.
 *  @tparam OutputIt Output iterator type.
 *  @tparam F        Transformation function type.
 *
 *  @param pool  Thread pool to use.
 *  @param first Start of input range.
 *  @param last  End of input range.
 *  @param d_first Start of output range.
 *  @param f     Transformation function.
 *  @param chunk_size Elements per chunk (0 = auto).
 *
 *  @return Iterator to end of output range.
 *
 *  ## Example
 *
 *  ```cpp
 *  std::vector<int> input = {1, 2, 3, 4, 5};
 *  std::vector<int> output(5);
 *  
 *  parallel_transform(pool, input.begin(), input.end(), output.begin(),
 *                     [](int x) { return x * x; });
 *  // output = {1, 4, 9, 16, 25}
 *  ```
 */
template <typename InputIt, typename OutputIt, typename F>
OutputIt parallel_transform(ThreadPool& pool, InputIt first, InputIt last, 
                            OutputIt d_first, F&& f, size_t chunk_size = 0)
{
  const size_t total = std::distance(first, last);
  if (total == 0)
    return d_first;
  
  if (chunk_size == 0)
    chunk_size = std::max(size_t(1), total / (pool.num_threads() * 4));
  
  std::vector<std::future<void>> futures;
  futures.reserve((total + chunk_size - 1) / chunk_size);
  
  InputIt chunk_in = first;
  OutputIt chunk_out = d_first;
  
  while (chunk_in < last)
    {
      size_t chunk_len = std::min(chunk_size, 
                                  static_cast<size_t>(std::distance(chunk_in, last)));
      InputIt chunk_in_end = chunk_in;
      std::advance(chunk_in_end, chunk_len);
      
      futures.push_back(pool.enqueue([f, chunk_in, chunk_in_end, chunk_out]() {
        InputIt in = chunk_in;
        OutputIt out = chunk_out;
        while (in != chunk_in_end)
          {
            *out = f(*in);
            ++in;
            ++out;
          }
      }));
      
      chunk_in = chunk_in_end;
      std::advance(chunk_out, chunk_len);
    }
  
  for (auto& fut : futures)
    fut.get();
  
  return d_first + total;
}

/** @brief Reduce elements in parallel.
 *
 *  Computes a reduction (like std::reduce) but parallelized.
 *  The binary operation must be associative and commutative.
 *
 *  @tparam Iterator Iterator type.
 *  @tparam T        Value type for result.
 *  @tparam BinaryOp Binary operation type.
 *
 *  @param pool  Thread pool to use.
 *  @param first Start of range.
 *  @param last  End of range.
 *  @param init  Initial value.
 *  @param op    Binary operation (must be associative & commutative).
 *  @param chunk_size Elements per chunk (0 = auto).
 *
 *  @return Reduction result.
 *
 *  ## Example
 *
 *  ```cpp
 *  std::vector<int> data = {1, 2, 3, 4, 5};
 *  
 *  // Sum
 *  int sum = parallel_reduce(pool, data.begin(), data.end(), 0,
 *                            std::plus<int>());
 *  // sum = 15
 *  
 *  // Product
 *  int prod = parallel_reduce(pool, data.begin(), data.end(), 1,
 *                             std::multiplies<int>());
 *  // prod = 120
 *  ```
 */
template <typename Iterator, typename T, typename BinaryOp>
T parallel_reduce(ThreadPool& pool, Iterator first, Iterator last, 
                  T init, BinaryOp op, size_t chunk_size = 0)
{
  const size_t total = std::distance(first, last);
  if (total == 0)
    return init;
  
  if (chunk_size == 0)
    chunk_size = std::max(size_t(1), total / (pool.num_threads() * 4));
  
  std::vector<std::future<T>> futures;
  futures.reserve((total + chunk_size - 1) / chunk_size);
  
  for (Iterator chunk_begin = first; chunk_begin < last; )
    {
      Iterator chunk_end = chunk_begin;
      std::advance(chunk_end, std::min(chunk_size, 
                   static_cast<size_t>(std::distance(chunk_begin, last))));
      
      futures.push_back(pool.enqueue([op, chunk_begin, chunk_end]() {
        T local_result = *chunk_begin;
        for (Iterator it = chunk_begin + 1; it != chunk_end; ++it)
          local_result = op(local_result, *it);
        return local_result;
      }));
      
      chunk_begin = chunk_end;
    }
  
  // Reduce partial results
  T result = init;
  for (auto& fut : futures)
    result = op(result, fut.get());
  
  return result;
}

/** @brief Apply a function to each element in parallel (index-based).
 *
 *  Convenience function for parallel loops with index access.
 *
 *  @param pool  Thread pool to use.
 *  @param start Start index (inclusive).
 *  @param end   End index (exclusive).
 *  @param f     Function: void(size_t index).
 *  @param chunk_size Iterations per chunk (0 = auto).
 *
 *  ## Example
 *
 *  ```cpp
 *  std::vector<int> data(1000);
 *  parallel_for_index(pool, 0, data.size(), [&data](size_t i) {
 *    data[i] = expensive_compute(i);
 *  });
 *  ```
 */
template <typename F>
void parallel_for_index(ThreadPool& pool, size_t start, size_t end, F&& f,
                        size_t chunk_size = 0)
{
  if (start >= end)
    return;
  
  const size_t total = end - start;
  
  if (chunk_size == 0)
    chunk_size = std::max(size_t(1), total / (pool.num_threads() * 4));
  
  std::vector<std::future<void>> futures;
  futures.reserve((total + chunk_size - 1) / chunk_size);
  
  for (size_t chunk_start = start; chunk_start < end; )
    {
      size_t chunk_end = std::min(chunk_start + chunk_size, end);
      
      futures.push_back(pool.enqueue([f, chunk_start, chunk_end]() {
        for (size_t i = chunk_start; i < chunk_end; ++i)
          f(i);
      }));
      
      chunk_start = chunk_end;
    }
  
  for (auto& fut : futures)
    fut.get();
}

/** @brief Global default thread pool.
 *
 *  A lazily-initialized global thread pool for convenience.
 *  Uses hardware concurrency for the number of threads.
 *
 *  ## Example
 *
 *  ```cpp
 *  auto future = Aleph::default_pool().enqueue(compute_something);
 *  ```
 *
 *  @return Reference to the global thread pool.
 */
inline ThreadPool& default_pool()
{
  static ThreadPool pool;
  return pool;
}

} // end namespace Aleph

#endif // ALEPH_THREAD_POOL_H
