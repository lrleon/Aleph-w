/**
 * @file thread_pool_example.cc
 * @brief Comprehensive ThreadPool Usage Examples: Parallel Task Execution
 *
 * This file demonstrates how to use the ThreadPool class effectively for
 * parallel task execution. ThreadPool provides a high-level interface for
 * managing worker threads and executing tasks concurrently, making it easier
 * to parallelize computations.
 *
 * ## What is a Thread Pool?
 *
 * A **thread pool** is a collection of worker threads that execute tasks
 * from a queue. Instead of creating threads for each task, threads are
 * reused, reducing overhead and improving performance.
 *
 * **Benefits**:
 * - **Reduced overhead**: Reuse threads instead of creating/destroying
 * - **Resource control**: Limit number of concurrent threads
 * - **Task queuing**: Handle more tasks than threads
 * - **Load balancing**: Distribute work across threads
 *
 * ## Features Demonstrated
 *
 * ### Example 1: Basic Parallel Computation
 * - **`enqueue()`**: Submit task and get future
 * - **Futures**: Wait for results asynchronously
 * - **Basic pattern**: Most common usage
 *
 * ### Example 2: Batch Processing
 * - **`enqueue_bulk()`**: Process collections in parallel
 * - **Batch operations**: Efficient parallel processing
 * - **Collection handling**: Work with containers
 *
 * ### Example 3: Fire-and-Forget Tasks
 * - **`enqueue_detached()`**: Submit without waiting
 * - **Async operations**: Don't need results
 * - **Background tasks**: Long-running operations
 *
 * ### Example 4: Backpressure Control
 * - **`enqueue_bounded()`**: Limit queue size
 * - **Flow control**: Prevent queue overflow
 * - **Backpressure**: Handle overload gracefully
 *
 * ### Example 5: Non-Blocking Submission
 * - **`try_enqueue()`**: Submit without blocking
 * - **Load shedding**: Reject when overloaded
 * - **Non-blocking**: Don't wait if queue full
 *
 * ### Example 6: Performance Comparison
 * - **Benchmarking**: Compare parallel vs sequential
 * - **Speedup**: Measure performance gains
 * - **Scalability**: Test with different thread counts
 *
 * ## Quick Start
 *
 * ```cpp
 * // Create thread pool with 4 worker threads
 * ThreadPool pool(4);
 *
 * // Submit a task and get a future
 * auto future = pool.enqueue([](int x) { return x * x; }, 5);
 *
 * // Wait for result
 * int result = future.get();  // result = 25
 * ```
 *
 * ## When to Use ThreadPool
 *
 * ✅ **Good for**:
 * - CPU-intensive tasks
 * - Independent computations
 * - Batch processing
 * - Parallel algorithms
 *
 * ❌ **Not good for**:
 * - I/O-bound tasks (use async I/O)
 * - Very short tasks (overhead too high)
 * - Tasks with dependencies (use task graphs)
 *
 * ## Complexity
 *
 * | Operation | Complexity | Notes |
 * |-----------|-----------|-------|
 * | enqueue() | O(1) amortized | Queue insertion |
 * | Future.get() | O(1) | Wait for completion |
 * | Thread creation | O(1) | Done at pool creation |
 *
 * ## Performance Considerations
 *
 * - **Thread count**: Usually CPU cores - 1 or CPU cores
 * - **Task granularity**: Tasks should be substantial (avoid tiny tasks)
 * - **Overhead**: ThreadPool has overhead, measure before optimizing
 * - **Cache effects**: Consider data locality
 *
 * ## Usage
 *
 * ```bash
 * ./thread_pool_example
 * ```
 *
 * This example has no command-line options; it runs all examples.
 *
 * ## Compilation
 *
 * ```bash
 * g++ -std=c++20 -O2 -pthread -I.. thread_pool_example.cc -o thread_pool_example
 * ```
 *
 * Or using CMake:
 * ```bash
 * cmake --build . --target thread_pool_example
 * ```
 *
 * @see thread_pool.H ThreadPool class implementation
 * @see ah_parallel_example.cc Parallel functional programming (uses ThreadPool)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <thread_pool.H>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>

using namespace Aleph;
using namespace std::chrono_literals;

// Helper to print section headers
void print_header(const std::string& title)
{
  std::cout << "\n";
  std::cout << "+" << std::string(60, '-') << "+\n";
  std::cout << "| " << std::left << std::setw(58) << title << " |\n";
  std::cout << "+" << std::string(60, '-') << "+\n\n";
}

// =============================================================================
// EXAMPLE 1: Basic Parallel Computation
// =============================================================================
//
// This example shows the fundamental pattern:
//   1. Create a ThreadPool
//   2. Submit tasks with enqueue() - returns std::future<T>
//   3. Collect results with future.get()
//
// USE CASE: When you have many independent computations and need all results.
//
// KEY CONCEPTS:
//   - enqueue(function, args...) submits a task and returns a future
//   - The future blocks on .get() until the result is ready
//   - Tasks run in parallel across available workers
//

bool is_prime(int n)
{
  if (n < 2) return false;
  if (n == 2) return true;
  if (n % 2 == 0) return false;
  for (int i = 3; i * i <= n; i += 2)
    if (n % i == 0) return false;
  return true;
}

void example_basic_parallel()
{
  print_header("Example 1: Basic Parallel Computation");
  
  std::cout << "GOAL: Find all prime numbers in a range using parallel computation.\n\n";
  
  const int range_start = 1;
  const int range_end = 100000;
  
  // STEP 1: Create ThreadPool
  // -------------------------
  // By default, uses std::thread::hardware_concurrency() threads.
  // You can specify the number: ThreadPool pool(4);
  
  ThreadPool pool(std::thread::hardware_concurrency());
  std::cout << "Created ThreadPool with " << pool.num_threads() << " workers\n\n";
  
  // STEP 2: Submit tasks
  // --------------------
  // enqueue() returns std::future<T> where T is the return type of the function.
  // Tasks are queued and executed by available workers.
  
  std::cout << "Submitting " << (range_end - range_start + 1) << " tasks...\n";
  
  std::vector<std::future<bool>> futures;
  futures.reserve(range_end - range_start + 1);
  
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int n = range_start; n <= range_end; ++n)
    {
      // Submit task: is_prime(n)
      // The future will hold the boolean result
      futures.push_back(pool.enqueue(is_prime, n));
    }
  
  // STEP 3: Collect results
  // -----------------------
  // future.get() blocks until the result is ready.
  // Results are collected in submission order (not completion order).
  
  std::cout << "Collecting results...\n";
  
  int prime_count = 0;
  for (size_t i = 0; i < futures.size(); ++i)
    {
      if (futures[i].get())  // Blocks until result ready
        ++prime_count;
    }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // RESULTS
  std::cout << "\n✓ RESULT: Found " << prime_count << " primes in range [" 
            << range_start << ", " << range_end << "]\n";
  std::cout << "  Time: " << duration.count() << " ms\n";
}

// =============================================================================
// EXAMPLE 2: Batch Processing with enqueue_bulk()
// =============================================================================
//
// When processing a collection of items with the same function,
// enqueue_bulk() is more convenient than calling enqueue() in a loop.
//
// USE CASE: Processing files, URLs, records, etc. in parallel.
//

struct FileResult
{
  std::string filename;
  int word_count;
  int line_count;
};

FileResult process_file(const std::string& filename)
{
  // Simulate file processing (10ms per file)
  std::this_thread::sleep_for(10ms);
  
  // Generate fake results based on filename
  size_t hash = std::hash<std::string>{}(filename);
  return {
    filename,
    static_cast<int>(hash % 1000 + 100),
    static_cast<int>((hash >> 10) % 100 + 10)
  };
}

void example_batch_processing()
{
  print_header("Example 2: Batch Processing with enqueue_bulk()");
  
  std::cout << "GOAL: Process multiple files in parallel and aggregate results.\n\n";
  
  // Create a list of files to process
  std::vector<std::string> files;
  for (int i = 1; i <= 50; ++i)
    files.push_back("document_" + std::to_string(i) + ".txt");
  
  ThreadPool pool(8);
  
  std::cout << "Processing " << files.size() << " files with " 
            << pool.num_threads() << " workers...\n\n";
  
  auto start = std::chrono::high_resolution_clock::now();
  
  // enqueue_bulk() submits the same function for each element in the container
  // Returns a vector of futures
  auto futures = pool.enqueue_bulk(process_file, files);
  
  // Aggregate results
  int total_words = 0;
  int total_lines = 0;
  
  for (auto& f : futures)
    {
      auto result = f.get();
      total_words += result.word_count;
      total_lines += result.line_count;
    }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  std::cout << "✓ RESULT:\n";
  std::cout << "  Files processed: " << files.size() << "\n";
  std::cout << "  Total words: " << total_words << "\n";
  std::cout << "  Total lines: " << total_lines << "\n";
  std::cout << "  Time: " << duration.count() << " ms\n";
  std::cout << "  (Sequential would take ~" << files.size() * 10 << " ms)\n";
}

// =============================================================================
// EXAMPLE 3: Fire-and-Forget with enqueue_detached()
// =============================================================================
//
// Sometimes you don't need the result of a task - you just want it to run
// in the background. enqueue_detached() is perfect for:
//   - Logging
//   - Metrics/telemetry
//   - Cache updates
//   - Notifications
//
// BENEFITS:
//   - No future overhead (slightly faster)
//   - Main thread doesn't need to track results
//
// WARNING:
//   - Exceptions in detached tasks are silently ignored
//   - Use enqueue() if you need error handling
//

void example_fire_and_forget()
{
  print_header("Example 3: Fire-and-Forget with enqueue_detached()");
  
  std::cout << "GOAL: Perform background logging without blocking main work.\n\n";
  
  ThreadPool pool(2);
  
  std::cout << "Main thread does work while logging happens in background:\n\n";
  
  for (int i = 1; i <= 5; ++i)
    {
      // Fire-and-forget: log message asynchronously
      // Main thread continues immediately
      pool.enqueue_detached([i] {
        // Simulate log write latency
        std::this_thread::sleep_for(5ms);
        std::ostringstream oss;
        oss << "  [BACKGROUND LOG] Processed item " << i << "\n";
        std::cout << oss.str();
      });
      
      // Main work continues without waiting for log
      std::cout << "[MAIN THREAD] Working on item " << i << "...\n";
      std::this_thread::sleep_for(30ms);
    }
  
  std::cout << "\n[MAIN THREAD] Main work complete. Waiting for logs...\n";
  pool.wait_all();  // Ensure all background tasks complete
  std::cout << "\n✓ All background logging completed\n";
}

// =============================================================================
// EXAMPLE 4: Backpressure with enqueue_bounded()
// =============================================================================
//
// PROBLEM: If producers submit tasks faster than workers can process,
// the queue grows unbounded, potentially causing memory issues.
//
// SOLUTION: enqueue_bounded() with queue limits
//   - soft_limit: When queue reaches this size, enqueue BLOCKS
//   - hard_limit: If queue reaches this size, throws queue_overflow_error
//
// This creates natural "backpressure" - fast producers slow down automatically.
//

void example_backpressure()
{
  print_header("Example 4: Backpressure with enqueue_bounded()");
  
  std::cout << "GOAL: Prevent queue overflow when producer is faster than consumers.\n\n";
  
  ThreadPool pool(2);
  
  // Configure queue limits
  // soft_limit = 5: Block when queue has 5+ pending tasks
  // hard_limit = 20: Throw exception if queue exceeds 20 (safety net)
  pool.set_queue_limits(5, 20);
  
  auto [soft, hard] = pool.get_queue_limits();
  std::cout << "Queue limits: soft=" << soft << ", hard=" << hard << "\n\n";
  
  std::atomic<int> processed{0};
  
  // Slow consumer task (50ms each)
  auto slow_task = [&processed] {
    std::this_thread::sleep_for(50ms);
    ++processed;
  };
  
  std::cout << "Enqueueing 20 slow tasks (50ms each)...\n";
  std::cout << "Watch how producer is throttled when queue fills up:\n\n";
  
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 1; i <= 20; ++i)
    {
      auto enqueue_start = std::chrono::high_resolution_clock::now();
      
      // This will BLOCK if queue is at soft_limit
      pool.enqueue_bounded_detached(slow_task);
      
      auto enqueue_end = std::chrono::high_resolution_clock::now();
      auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        enqueue_end - enqueue_start).count();
      
      std::cout << "  Task " << std::setw(2) << i 
                << " enqueued (pending: " << pool.pending_tasks() << ")";
      if (wait_time > 5)
        std::cout << " ← blocked " << wait_time << "ms";
      std::cout << "\n";
    }
  
  pool.wait_all();
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  std::cout << "\n✓ RESULT:\n";
  std::cout << "  Processed: " << processed << " tasks\n";
  std::cout << "  Total time: " << duration.count() << " ms\n";
  std::cout << "  Memory was protected by limiting queue size\n";
}

// =============================================================================
// EXAMPLE 5: Load Shedding with try_enqueue()
// =============================================================================
//
// SCENARIO: High-traffic system where you'd rather DROP requests than
// slow down or run out of memory.
//
// try_enqueue() returns immediately:
//   - std::optional<future> if task was queued
//   - std::nullopt if queue is full (at soft_limit)
//
// try_enqueue_detached() returns:
//   - true if task was queued
//   - false if queue is full
//
// This is useful for:
//   - Web servers that need to reject excess requests
//   - Real-time systems where dropping is better than delaying
//

void example_load_shedding()
{
  print_header("Example 5: Load Shedding with try_enqueue()");
  
  std::cout << "GOAL: Reject excess tasks when system is overloaded.\n\n";
  
  ThreadPool pool(2);
  pool.set_queue_limits(3, 10);  // Accept max 3 pending tasks
  
  std::atomic<int> accepted{0};
  int rejected = 0;
  
  // Slow task (100ms)
  auto task = [&accepted] {
    std::this_thread::sleep_for(100ms);
    ++accepted;
  };
  
  std::cout << "Attempting to enqueue 15 tasks with soft_limit=3:\n\n";
  
  for (int i = 1; i <= 15; ++i)
    {
      // try_enqueue_detached returns bool (non-blocking)
      if (pool.try_enqueue_detached(task))
        {
          std::cout << "  Task " << std::setw(2) << i << ": ✓ ACCEPTED\n";
        }
      else
        {
          std::cout << "  Task " << std::setw(2) << i << ": ✗ REJECTED (queue full)\n";
          ++rejected;
        }
    }
  
  pool.wait_all();
  
  std::cout << "\n✓ RESULT:\n";
  std::cout << "  Accepted and processed: " << accepted << "\n";
  std::cout << "  Rejected (dropped): " << rejected << "\n";
  std::cout << "  System remained responsive - no blocking!\n";
}

// =============================================================================
// EXAMPLE 6: Performance Comparison
// =============================================================================
//
// This example demonstrates the speedup achieved by parallel execution.
// The speedup depends on:
//   - Number of CPU cores
//   - Task duration (longer tasks = better speedup)
//   - Task independence (no shared state = better speedup)
//

void example_performance()
{
  print_header("Example 6: Performance Comparison");
  
  std::cout << "GOAL: Compare parallel execution vs sequential execution.\n\n";
  
  const int num_tasks = 1000;
  
  // CPU-intensive computation
  auto compute = [](int x) {
    double result = 0;
    for (int i = 0; i < 10000; ++i)
      result += std::sin(x * i * 0.001);
    return result;
  };
  
  // SEQUENTIAL EXECUTION
  std::cout << "Running " << num_tasks << " tasks sequentially...\n";
  
  auto seq_start = std::chrono::high_resolution_clock::now();
  double seq_result = 0;
  for (int i = 0; i < num_tasks; ++i)
    seq_result += compute(i);
  auto seq_end = std::chrono::high_resolution_clock::now();
  auto seq_ms = std::chrono::duration_cast<std::chrono::milliseconds>(seq_end - seq_start).count();
  
  // PARALLEL EXECUTION
  std::cout << "Running " << num_tasks << " tasks in parallel...\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  auto par_start = std::chrono::high_resolution_clock::now();
  std::vector<std::future<double>> futures;
  futures.reserve(num_tasks);
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue(compute, i));
  
  double par_result = 0;
  for (auto& f : futures)
    par_result += f.get();
  auto par_end = std::chrono::high_resolution_clock::now();
  auto par_ms = std::chrono::duration_cast<std::chrono::milliseconds>(par_end - par_start).count();
  
  // RESULTS
  double speedup = (par_ms > 0) ? static_cast<double>(seq_ms) / par_ms : 0;
  
  std::cout << "┌────────────────────────────────────────┐\n";
  std::cout << "│           PERFORMANCE RESULTS          │\n";
  std::cout << "├────────────────────────────────────────┤\n";
  std::cout << "│ Tasks:        " << std::setw(24) << num_tasks << " │\n";
  std::cout << "│ Threads:      " << std::setw(24) << pool.num_threads() << " │\n";
  std::cout << "├────────────────────────────────────────┤\n";
  std::cout << "│ Sequential:   " << std::setw(20) << seq_ms << " ms │\n";
  std::cout << "│ Parallel:     " << std::setw(20) << par_ms << " ms │\n";
  std::cout << "├────────────────────────────────────────┤\n";
  std::cout << "│ SPEEDUP:      " << std::setw(20) << std::fixed 
            << std::setprecision(1) << speedup << "x │\n";
  std::cout << "└────────────────────────────────────────┘\n";
  
  // Verify correctness
  std::cout << "\n✓ Results match: " << (std::abs(seq_result - par_result) < 0.01 ? "YES" : "NO") << "\n";
}

// =============================================================================
// MAIN - Run all examples
// =============================================================================

int main()
{
  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║                                                                ║\n";
  std::cout << "║           ALEPH-W THREADPOOL USAGE EXAMPLES                    ║\n";
  std::cout << "║                                                                ║\n";
  std::cout << "║   Learn how to use the ThreadPool for parallel execution       ║\n";
  std::cout << "║                                                                ║\n";
  std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
  
  std::cout << "\nThis program demonstrates 6 common ThreadPool usage patterns.\n";
  std::cout << "Read the source code comments for detailed explanations.\n";
  
  example_basic_parallel();
  example_batch_processing();
  example_fire_and_forget();
  example_backpressure();
  example_load_shedding();
  example_performance();
  
  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║  ✓ ALL EXAMPLES COMPLETED SUCCESSFULLY                         ║\n";
  std::cout << "║                                                                ║\n";
  std::cout << "║  QUICK REFERENCE:                                              ║\n";
  std::cout << "║    enqueue(f, args...)          → std::future<T>               ║\n";
  std::cout << "║    enqueue_detached(f, args...) → void (fire-and-forget)       ║\n";
  std::cout << "║    enqueue_bounded(f, args...)  → std::future<T> (backpressure)║\n";
  std::cout << "║    try_enqueue(f, args...)      → optional<future> (non-block) ║\n";
  std::cout << "║    enqueue_bulk(f, container)   → vector<future> (batch)       ║\n";
  std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
  
  return 0;
}
