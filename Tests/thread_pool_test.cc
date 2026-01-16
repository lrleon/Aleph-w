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


/**
 * @file thread_pool_test.cc
 * @brief Tests for Thread Pool
 */
#include <gtest/gtest.h>
#include <thread_pool.H>
#include <atomic>
#include <chrono>
#include <vector>
#include <cmath>
#include <numeric>
#include <iomanip>

using namespace Aleph;
using namespace std::chrono_literals;

class ThreadPoolTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(ThreadPoolTest, DefaultConstruction)
{
  ThreadPool pool;
  EXPECT_GT(pool.num_threads(), 0u);
  EXPECT_TRUE(pool.is_idle());
  EXPECT_FALSE(pool.is_stopped());
}

TEST_F(ThreadPoolTest, ConstructionWithSize)
{
  ThreadPool pool(4);
  EXPECT_EQ(pool.num_threads(), 4u);
}

TEST_F(ThreadPoolTest, ConstructionWithZeroDefaultsToOne)
{
  ThreadPool pool(0);
  EXPECT_GE(pool.num_threads(), 1u);
}

TEST_F(ThreadPoolTest, SimpleTask)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([] { return 42; });
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, TaskWithArguments)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);
  
  EXPECT_EQ(future.get(), 30);
}

TEST_F(ThreadPoolTest, TaskWithReferenceCapture)
{
  ThreadPool pool(2);
  int value = 100;
  
  auto future = pool.enqueue([&value] { return value * 2; });
  
  EXPECT_EQ(future.get(), 200);
}

TEST_F(ThreadPoolTest, VoidTask)
{
  ThreadPool pool(2);
  std::atomic<bool> executed{false};
  
  auto future = pool.enqueue([&executed] { executed = true; });
  future.get();
  
  EXPECT_TRUE(executed);
}

TEST_F(ThreadPoolTest, MultipleTasks)
{
  ThreadPool pool(4);
  const int num_tasks = 100;
  std::vector<std::future<int>> futures;
  
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue([i] { return i * i; }));
  
  for (int i = 0; i < num_tasks; ++i)
    EXPECT_EQ(futures[i].get(), i * i);
}

// ============================================================================
// Status and Query Tests
// ============================================================================

TEST_F(ThreadPoolTest, PendingTasks)
{
  ThreadPool pool(1);
  std::atomic<bool> block{true};
  
  // Block the single worker
  pool.enqueue_detached([&block] { while (block) std::this_thread::yield(); });
  
  // Queue more tasks
  for (int i = 0; i < 5; ++i)
    pool.enqueue_detached([] {});
  
  // Should have pending tasks
  EXPECT_GT(pool.pending_tasks(), 0u);
  
  // Unblock
  block = false;
  pool.wait_all();
  
  EXPECT_EQ(pool.pending_tasks(), 0u);
}

TEST_F(ThreadPoolTest, IsIdleAfterCompletion)
{
  ThreadPool pool(2);
  
  auto f1 = pool.enqueue([] { return 1; });
  auto f2 = pool.enqueue([] { return 2; });
  
  f1.get();
  f2.get();
  
  pool.wait_all();
  EXPECT_TRUE(pool.is_idle());
}

// ============================================================================
// Shutdown Tests
// ============================================================================

TEST_F(ThreadPoolTest, ShutdownCompletesAllTasks)
{
  ThreadPool pool(2);
  std::atomic<int> counter{0};
  
  for (int i = 0; i < 10; ++i)
    pool.enqueue_detached([&counter] { ++counter; });
  
  pool.shutdown();
  
  EXPECT_EQ(counter.load(), 10);
  EXPECT_TRUE(pool.is_stopped());
}

TEST_F(ThreadPoolTest, EnqueueAfterShutdownThrows)
{
  ThreadPool pool(2);
  pool.shutdown();
  
  EXPECT_THROW(pool.enqueue([] { return 0; }), std::runtime_error);
}

TEST_F(ThreadPoolTest, DoubleShutdownIsSafe)
{
  ThreadPool pool(2);
  pool.shutdown();
  EXPECT_NO_THROW(pool.shutdown());
}

// ============================================================================
// Resize Tests
// ============================================================================

TEST_F(ThreadPoolTest, ResizeIncrease)
{
  ThreadPool pool(2);
  EXPECT_EQ(pool.num_threads(), 2u);
  
  pool.resize(4);
  EXPECT_EQ(pool.num_threads(), 4u);
  
  // Verify new workers work
  auto future = pool.enqueue([] { return 42; });
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, ResizeDecrease)
{
  ThreadPool pool(4);
  EXPECT_EQ(pool.num_threads(), 4u);
  
  pool.resize(2);
  EXPECT_EQ(pool.num_threads(), 2u);
  
  // Verify remaining workers work
  auto future = pool.enqueue([] { return 42; });
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, ResizeToSameIsNoOp)
{
  ThreadPool pool(4);
  pool.resize(4);
  EXPECT_EQ(pool.num_threads(), 4u);
}

TEST_F(ThreadPoolTest, ResizeAfterShutdownThrows)
{
  ThreadPool pool(2);
  pool.shutdown();
  
  EXPECT_THROW(pool.resize(4), std::runtime_error);
}

TEST_F(ThreadPoolTest, ResizePreservesPendingTasks)
{
  ThreadPool pool(1);
  std::atomic<bool> block{true};
  std::atomic<int> completed{0};
  
  // Block the worker
  pool.enqueue_detached([&block] { while (block) std::this_thread::yield(); });
  
  // Queue tasks
  for (int i = 0; i < 5; ++i)
    pool.enqueue_detached([&completed] { ++completed; });
  
  // Unblock and resize
  block = false;
  pool.resize(4);
  
  pool.wait_all();
  
  // All tasks should complete (first blocking task + 5 counting tasks)
  EXPECT_EQ(completed.load(), 5);
}

// ============================================================================
// Exception Handling Tests
// ============================================================================

TEST_F(ThreadPoolTest, ExceptionPropagation)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([] {
    throw std::runtime_error("test exception");
    return 0;
  });
  
  EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(ThreadPoolTest, ExceptionDoesNotAffectOtherTasks)
{
  ThreadPool pool(2);
  
  auto f1 = pool.enqueue([] {
    throw std::runtime_error("test");
    return 0;
  });
  
  auto f2 = pool.enqueue([] { return 42; });
  
  EXPECT_THROW(f1.get(), std::runtime_error);
  EXPECT_EQ(f2.get(), 42);
}

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST_F(ThreadPoolTest, ConcurrentEnqueue)
{
  ThreadPool pool(4);
  std::atomic<int> counter{0};
  const int tasks_per_thread = 100;
  const int num_enqueue_threads = 4;
  
  std::vector<std::thread> enqueuers;
  for (int t = 0; t < num_enqueue_threads; ++t)
    {
      enqueuers.emplace_back([&pool, &counter, tasks_per_thread] {
        for (int i = 0; i < tasks_per_thread; ++i)
          pool.enqueue_detached([&counter] { ++counter; });
      });
    }
  
  for (auto& t : enqueuers)
    t.join();
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), tasks_per_thread * num_enqueue_threads);
}

TEST_F(ThreadPoolTest, ParallelExecution)
{
  ThreadPool pool(4);
  std::atomic<int> concurrent_count{0};
  std::atomic<int> max_concurrent{0};
  
  std::vector<std::future<void>> futures;
  for (int i = 0; i < 100; ++i)
    {
      futures.push_back(pool.enqueue([&concurrent_count, &max_concurrent] {
        int current = ++concurrent_count;
        
        // Update max if this is higher
        int prev_max = max_concurrent.load();
        while (current > prev_max && 
               !max_concurrent.compare_exchange_weak(prev_max, current))
          ;
        
        std::this_thread::sleep_for(1ms);
        --concurrent_count;
      }));
    }
  
  for (auto& f : futures)
    f.get();
  
  // Should have had multiple concurrent executions
  EXPECT_GT(max_concurrent.load(), 1);
}

// ============================================================================
// Performance Tests (not strictly unit tests, but useful)
// ============================================================================

TEST_F(ThreadPoolTest, ManySmallTasks)
{
  ThreadPool pool(std::thread::hardware_concurrency());
  const int num_tasks = 10000;
  std::atomic<int> sum{0};
  
  auto start = std::chrono::high_resolution_clock::now();
  
  std::vector<std::future<void>> futures;
  futures.reserve(num_tasks);
  
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue([&sum, i] { sum += i; }));
  
  for (auto& f : futures)
    f.get();
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Expected sum: 0 + 1 + 2 + ... + (n-1) = n*(n-1)/2
  int expected = num_tasks * (num_tasks - 1) / 2;
  EXPECT_EQ(sum.load(), expected);
  
  // Should complete reasonably fast (less than 5 seconds)
  EXPECT_LT(duration.count(), 5000);
}

TEST_F(ThreadPoolTest, ComputeIntensiveTasks)
{
  ThreadPool pool(std::thread::hardware_concurrency());
  const int num_tasks = 100;
  
  // Compute-intensive task: calculate sum of squares
  auto compute = [](int n) {
    double sum = 0;
    for (int i = 0; i < n; ++i)
      sum += std::sqrt(static_cast<double>(i));
    return sum;
  };
  
  std::vector<std::future<double>> futures;
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue(compute, 10000));
  
  for (auto& f : futures)
    f.get();
  
  // Wait for pool to transition to idle state (fixes race condition in CI)
  const auto start = std::chrono::steady_clock::now();
  while (!pool.is_idle() && 
         std::chrono::steady_clock::now() - start < 1s)
  {
    std::this_thread::sleep_for(1ms);
  }
  
  EXPECT_TRUE(pool.is_idle());
}

// ============================================================================
// Return Type Tests
// ============================================================================

TEST_F(ThreadPoolTest, ReturnString)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([] { return std::string("hello"); });
  
  EXPECT_EQ(future.get(), "hello");
}

TEST_F(ThreadPoolTest, ReturnVector)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([] {
    return std::vector<int>{1, 2, 3, 4, 5};
  });
  
  auto result = future.get();
  EXPECT_EQ(result.size(), 5u);
  EXPECT_EQ(result[2], 3);
}

TEST_F(ThreadPoolTest, ReturnPair)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([](int a, int b) {
    return std::make_pair(a + b, a * b);
  }, 3, 4);
  
  auto [sum, product] = future.get();
  EXPECT_EQ(sum, 7);
  EXPECT_EQ(product, 12);
}

// ============================================================================
// Callable Types Tests
// ============================================================================

int free_function(int x) { return x * 2; }

struct Functor
{
  int value;
  int operator()(int x) const { return x + value; }
};

TEST_F(ThreadPoolTest, FreeFunction)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue(free_function, 21);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, FunctorObject)
{
  ThreadPool pool(2);
  Functor f{10};
  
  auto future = pool.enqueue(f, 32);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, StdFunction)
{
  ThreadPool pool(2);
  std::function<int(int)> func = [](int x) { return x * x; };
  
  auto future = pool.enqueue(func, 6);
  
  EXPECT_EQ(future.get(), 36);
}

// ============================================================================
// Member Function Tests
// ============================================================================

struct Calculator
{
  int value = 10;
  
  int add(int x) { return value + x; }
  int multiply(int a, int b) const { return a * b; }
  static int square(int x) { return x * x; }
};

TEST_F(ThreadPoolTest, MemberFunctionPointer)
{
  ThreadPool pool(2);
  Calculator calc;
  calc.value = 20;
  
  auto future = pool.enqueue(&Calculator::add, &calc, 22);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, ConstMemberFunction)
{
  ThreadPool pool(2);
  Calculator calc;
  
  auto future = pool.enqueue(&Calculator::multiply, &calc, 6, 7);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, StaticMemberFunction)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue(&Calculator::square, 7);
  
  EXPECT_EQ(future.get(), 49);
}

TEST_F(ThreadPoolTest, MemberFunctionWithReference)
{
  ThreadPool pool(2);
  Calculator calc;
  calc.value = 30;
  
  // Using reference wrapper
  auto future = pool.enqueue(&Calculator::add, std::ref(calc), 12);
  
  EXPECT_EQ(future.get(), 42);
}

// ============================================================================
// Move-Only Tests
// ============================================================================

TEST_F(ThreadPoolTest, MoveOnlyLambda)
{
  ThreadPool pool(2);
  
  auto ptr = std::make_unique<int>(42);
  auto future = pool.enqueue([p = std::move(ptr)]() { return *p; });
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, MoveOnlyArgument)
{
  ThreadPool pool(2);
  
  auto ptr = std::make_unique<int>(100);
  auto future = pool.enqueue([](std::unique_ptr<int> p) { return *p * 2; }, 
                              std::move(ptr));
  
  EXPECT_EQ(future.get(), 200);
}

TEST_F(ThreadPoolTest, MoveOnlyFunctor)
{
  ThreadPool pool(2);
  
  struct MoveOnlyFunctor
  {
    std::unique_ptr<int> data;
    
    MoveOnlyFunctor(int v) : data(std::make_unique<int>(v)) {}
    MoveOnlyFunctor(MoveOnlyFunctor&&) = default;
    MoveOnlyFunctor& operator=(MoveOnlyFunctor&&) = default;
    MoveOnlyFunctor(const MoveOnlyFunctor&) = delete;
    
    int operator()() { return *data; }
  };
  
  auto future = pool.enqueue(MoveOnlyFunctor{42});
  
  EXPECT_EQ(future.get(), 42);
}

// ============================================================================
// Enqueue Detached Tests
// ============================================================================

TEST_F(ThreadPoolTest, EnqueueDetachedBasic)
{
  ThreadPool pool(2);
  std::atomic<int> counter{0};
  
  for (int i = 0; i < 10; ++i)
    pool.enqueue_detached([&counter] { ++counter; });
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), 10);
}

TEST_F(ThreadPoolTest, EnqueueDetachedWithArgs)
{
  ThreadPool pool(2);
  std::atomic<int> sum{0};
  
  for (int i = 1; i <= 5; ++i)
    pool.enqueue_detached([&sum](int x) { sum += x; }, i);
  
  pool.wait_all();
  
  EXPECT_EQ(sum.load(), 15);  // 1+2+3+4+5
}

TEST_F(ThreadPoolTest, EnqueueDetachedExceptionsSilent)
{
  ThreadPool pool(2);
  std::atomic<int> counter{0};
  
  // This should not crash - exceptions are silently ignored
  pool.enqueue_detached([] { throw std::runtime_error("ignored"); });
  pool.enqueue_detached([&counter] { ++counter; });
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), 1);
}

TEST_F(ThreadPoolTest, EnqueueDetachedAfterShutdownThrows)
{
  ThreadPool pool(2);
  pool.shutdown();
  
  EXPECT_THROW(pool.enqueue_detached([] {}), std::runtime_error);
}

// ============================================================================
// Enqueue Bulk Tests
// ============================================================================

TEST_F(ThreadPoolTest, EnqueueBulkVector)
{
  ThreadPool pool(4);
  std::vector<int> inputs = {1, 2, 3, 4, 5};
  
  auto futures = pool.enqueue_bulk([](int x) { return x * x; }, inputs);
  
  ASSERT_EQ(futures.size(), 5u);
  EXPECT_EQ(futures[0].get(), 1);
  EXPECT_EQ(futures[1].get(), 4);
  EXPECT_EQ(futures[2].get(), 9);
  EXPECT_EQ(futures[3].get(), 16);
  EXPECT_EQ(futures[4].get(), 25);
}

TEST_F(ThreadPoolTest, EnqueueBulkStrings)
{
  ThreadPool pool(2);
  std::vector<std::string> inputs = {"hello", "world", "test"};
  
  auto futures = pool.enqueue_bulk([](const std::string& s) { 
    return s.size(); 
  }, inputs);
  
  ASSERT_EQ(futures.size(), 3u);
  EXPECT_EQ(futures[0].get(), 5u);
  EXPECT_EQ(futures[1].get(), 5u);
  EXPECT_EQ(futures[2].get(), 4u);
}

TEST_F(ThreadPoolTest, EnqueueBulkEmpty)
{
  ThreadPool pool(2);
  std::vector<int> empty;
  
  auto futures = pool.enqueue_bulk([](int x) { return x; }, empty);
  
  EXPECT_TRUE(futures.empty());
}

// ============================================================================
// Default Pool Tests
// ============================================================================

TEST_F(ThreadPoolTest, DefaultPoolExists)
{
  auto& pool = Aleph::default_pool();
  
  EXPECT_GT(pool.num_threads(), 0u);
  EXPECT_FALSE(pool.is_stopped());
}

TEST_F(ThreadPoolTest, DefaultPoolWorks)
{
  auto future = Aleph::default_pool().enqueue([](int x) { return x * 2; }, 21);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, DefaultPoolIsSingleton)
{
  auto& pool1 = Aleph::default_pool();
  auto& pool2 = Aleph::default_pool();
  
  EXPECT_EQ(&pool1, &pool2);
}

// ============================================================================
// Reference Argument Tests
// ============================================================================

void increment_ref(int& x) { ++x; }
void add_to_ref(int& x, int amount) { x += amount; }

TEST_F(ThreadPoolTest, ReferenceArgumentWithStdRef)
{
  ThreadPool pool(2);
  int value = 10;
  
  auto future = pool.enqueue(increment_ref, std::ref(value));
  future.get();
  
  EXPECT_EQ(value, 11);
}

TEST_F(ThreadPoolTest, ReferenceArgumentMultipleParams)
{
  ThreadPool pool(2);
  int value = 100;
  
  auto future = pool.enqueue(add_to_ref, std::ref(value), 50);
  future.get();
  
  EXPECT_EQ(value, 150);
}

TEST_F(ThreadPoolTest, ConstReferenceWithStdCref)
{
  ThreadPool pool(2);
  const int value = 77;
  
  auto future = pool.enqueue([](const int& x) { return x * 2; }, std::cref(value));
  
  EXPECT_EQ(future.get(), 154);
}

TEST_F(ThreadPoolTest, LambdaCaptureByReference)
{
  ThreadPool pool(2);
  int value = 0;
  
  auto future = pool.enqueue([&value](int x) { value = x; }, 42);
  future.get();
  
  EXPECT_EQ(value, 42);
}

TEST_F(ThreadPoolTest, MemberFunctionWithStdRef)
{
  ThreadPool pool(2);
  Calculator calc;
  calc.value = 30;
  
  auto future = pool.enqueue(&Calculator::add, std::ref(calc), 12);
  
  EXPECT_EQ(future.get(), 42);
}

// ============================================================================
// WaitAll Tests
// ============================================================================

TEST_F(ThreadPoolTest, WaitAllBlocks)
{
  ThreadPool pool(2);
  std::atomic<int> counter{0};
  
  for (int i = 0; i < 10; ++i)
    pool.enqueue_detached([&counter] {
      std::this_thread::sleep_for(10ms);
      ++counter;
    });
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), 10);
}

TEST_F(ThreadPoolTest, WaitAllOnEmptyPool)
{
  ThreadPool pool(2);
  
  // Should return immediately
  auto start = std::chrono::high_resolution_clock::now();
  pool.wait_all();
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_LT(duration.count(), 100);  // Should be nearly instant
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(ThreadPoolTest, StressHighVolume)
{
  ThreadPool pool(std::thread::hardware_concurrency());
  const int num_tasks = 100000;
  std::atomic<int> counter{0};
  
  for (int i = 0; i < num_tasks; ++i)
    pool.enqueue_detached([&counter] { ++counter; });
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), num_tasks);
}

TEST_F(ThreadPoolTest, StressConcurrentEnqueueFromManyThreads)
{
  ThreadPool pool(8);
  const int num_enqueuers = 16;
  const int tasks_per_enqueuer = 1000;
  std::atomic<int> counter{0};
  
  std::vector<std::thread> enqueuers;
  for (int i = 0; i < num_enqueuers; ++i)
    {
      enqueuers.emplace_back([&pool, &counter, tasks_per_enqueuer] {
        for (int j = 0; j < tasks_per_enqueuer; ++j)
          pool.enqueue_detached([&counter] { ++counter; });
      });
    }
  
  for (auto& t : enqueuers)
    t.join();
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), num_enqueuers * tasks_per_enqueuer);
}

TEST_F(ThreadPoolTest, StressMixedWorkloads)
{
  ThreadPool pool(4);
  std::atomic<int> fast_count{0};
  std::atomic<int> slow_count{0};
  
  // Mix of fast and slow tasks
  for (int i = 0; i < 100; ++i)
    {
      pool.enqueue_detached([&fast_count] { ++fast_count; });
      pool.enqueue_detached([&slow_count] {
        std::this_thread::sleep_for(1ms);
        ++slow_count;
      });
    }
  
  pool.wait_all();
  
  EXPECT_EQ(fast_count.load(), 100);
  EXPECT_EQ(slow_count.load(), 100);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ThreadPoolTest, SingleThreadPool)
{
  ThreadPool pool(1);
  std::vector<int> results;
  std::mutex mtx;
  
  // Tasks should execute sequentially
  for (int i = 0; i < 10; ++i)
    {
      pool.enqueue_detached([&results, &mtx, i] {
        std::lock_guard<std::mutex> lock(mtx);
        results.push_back(i);
      });
    }
  
  pool.wait_all();
  
  EXPECT_EQ(results.size(), 10u);
}

TEST_F(ThreadPoolTest, EmptyTasksSequence)
{
  ThreadPool pool(4);
  
  for (int i = 0; i < 1000; ++i)
    pool.enqueue_detached([] {});
  
  pool.wait_all();
  
  EXPECT_TRUE(pool.is_idle());
}

TEST_F(ThreadPoolTest, VeryLargeReturnType)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([] {
    std::vector<int> large(10000);
    std::iota(large.begin(), large.end(), 0);
    return large;
  });
  
  auto result = future.get();
  EXPECT_EQ(result.size(), 10000u);
  EXPECT_EQ(result[9999], 9999);
}

TEST_F(ThreadPoolTest, TaskReturningTask)
{
  ThreadPool pool(2);
  
  auto future = pool.enqueue([&pool] {
    // Enqueue from inside a task
    return pool.enqueue([] { return 42; });
  });
  
  auto inner_future = future.get();
  EXPECT_EQ(inner_future.get(), 42);
}

TEST_F(ThreadPoolTest, RecursiveEnqueue)
{
  ThreadPool pool(4);
  std::atomic<int> counter{0};
  
  std::function<void(int)> recursive = [&pool, &counter, &recursive](int depth) {
    ++counter;
    if (depth > 0)
      pool.enqueue_detached(recursive, depth - 1);
  };
  
  pool.enqueue_detached(recursive, 10);
  
  // Wait a bit for all recursive tasks
  std::this_thread::sleep_for(100ms);
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), 11);  // 0 to 10 inclusive
}

TEST_F(ThreadPoolTest, TaskThrowingDifferentExceptions)
{
  ThreadPool pool(4);
  
  auto f1 = pool.enqueue([]() -> int { throw std::runtime_error("runtime"); });
  auto f2 = pool.enqueue([]() -> int { throw std::logic_error("logic"); });
  auto f3 = pool.enqueue([]() -> int { throw std::out_of_range("range"); });
  auto f4 = pool.enqueue([] { return 42; });  // Normal task
  
  EXPECT_THROW(f1.get(), std::runtime_error);
  EXPECT_THROW(f2.get(), std::logic_error);
  EXPECT_THROW(f3.get(), std::out_of_range);
  EXPECT_EQ(f4.get(), 42);
}

TEST_F(ThreadPoolTest, AllTasksThrowExceptions)
{
  ThreadPool pool(4);
  std::atomic<int> exception_count{0};
  
  std::vector<std::future<int>> futures;
  for (int i = 0; i < 100; ++i)
    {
      futures.push_back(pool.enqueue([]() -> int {
        throw std::runtime_error("test");
      }));
    }
  
  for (auto& f : futures)
    {
      try
        {
          f.get();
        }
      catch (const std::runtime_error&)
        {
          ++exception_count;
        }
    }
  
  EXPECT_EQ(exception_count.load(), 100);
}

// ============================================================================
// Shutdown and Lifecycle Tests
// ============================================================================

TEST_F(ThreadPoolTest, ShutdownWhileTasksRunning)
{
  ThreadPool pool(2);
  std::atomic<int> started{0};
  std::atomic<int> finished{0};
  std::atomic<bool> can_finish{false};
  
  // Start some blocking tasks
  for (int i = 0; i < 4; ++i)
    {
      pool.enqueue_detached([&started, &finished, &can_finish] {
        ++started;
        while (!can_finish)
          std::this_thread::sleep_for(1ms);
        ++finished;
      });
    }
  
  // Wait for tasks to start
  while (started < 2)
    std::this_thread::sleep_for(1ms);
  
  // Allow tasks to finish
  can_finish = true;
  
  // Shutdown should wait for all tasks
  pool.shutdown();
  
  EXPECT_EQ(finished.load(), 4);
}

TEST_F(ThreadPoolTest, DestructorWaitsForTasks)
{
  std::atomic<int> counter{0};
  
  {
    ThreadPool pool(2);
    for (int i = 0; i < 10; ++i)
      {
        pool.enqueue_detached([&counter] {
          std::this_thread::sleep_for(10ms);
          ++counter;
        });
      }
    // Destructor should wait
  }
  
  EXPECT_EQ(counter.load(), 10);
}

TEST_F(ThreadPoolTest, ResizeUnderLoad)
{
  ThreadPool pool(2);
  std::atomic<int> counter{0};
  std::atomic<bool> keep_running{true};
  std::atomic<bool> resize_in_progress{false};
  
  // Start continuous work
  std::thread producer([&pool, &counter, &keep_running, &resize_in_progress] {
    while (keep_running)
      {
        // Skip enqueue during resize to avoid race
        if (!resize_in_progress)
          {
            try
              {
                pool.enqueue_detached([&counter] { ++counter; });
              }
            catch (const std::runtime_error&)
              {
                // Pool might be stopped during resize, ignore
              }
          }
        std::this_thread::sleep_for(1ms);
      }
  });
  
  // Resize while under load
  std::this_thread::sleep_for(20ms);
  resize_in_progress = true;
  pool.resize(8);
  resize_in_progress = false;
  std::this_thread::sleep_for(20ms);
  resize_in_progress = true;
  pool.resize(2);
  resize_in_progress = false;
  std::this_thread::sleep_for(20ms);
  
  keep_running = false;
  producer.join();
  pool.wait_all();
  
  EXPECT_GT(counter.load(), 0);
}

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST_F(ThreadPoolTest, DataIntegrityUnderConcurrency)
{
  ThreadPool pool(8);
  const int num_tasks = 10000;
  std::vector<std::future<int>> futures;
  futures.reserve(num_tasks);
  
  // Each task returns its index
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue([i] { return i; }));
  
  // Verify all results are correct
  for (int i = 0; i < num_tasks; ++i)
    EXPECT_EQ(futures[i].get(), i);
}

TEST_F(ThreadPoolTest, AtomicOperationsCorrectness)
{
  ThreadPool pool(8);
  std::atomic<long long> sum{0};
  const int num_tasks = 10000;
  
  for (int i = 1; i <= num_tasks; ++i)
    pool.enqueue_detached([&sum, i] { sum += i; });
  
  pool.wait_all();
  
  // Sum of 1 to n = n*(n+1)/2
  long long expected = static_cast<long long>(num_tasks) * (num_tasks + 1) / 2;
  EXPECT_EQ(sum.load(), expected);
}

TEST_F(ThreadPoolTest, MutexProtectedSharedState)
{
  ThreadPool pool(8);
  std::vector<int> shared_vec;
  std::mutex mtx;
  const int num_tasks = 1000;
  
  for (int i = 0; i < num_tasks; ++i)
    {
      pool.enqueue_detached([&shared_vec, &mtx, i] {
        std::lock_guard<std::mutex> lock(mtx);
        shared_vec.push_back(i);
      });
    }
  
  pool.wait_all();
  
  EXPECT_EQ(shared_vec.size(), static_cast<size_t>(num_tasks));
  
  // Sort and verify all values present
  std::sort(shared_vec.begin(), shared_vec.end());
  for (int i = 0; i < num_tasks; ++i)
    EXPECT_EQ(shared_vec[i], i);
}

// ============================================================================
// Timing and Performance Tests
// ============================================================================

TEST_F(ThreadPoolTest, ParallelSpeedup)
{
  const int num_tasks = 100;
  const auto work_duration = 10ms;
  
  auto do_work = [work_duration] {
    std::this_thread::sleep_for(work_duration);
  };
  
  // Sequential baseline (single thread)
  ThreadPool pool_single(1);
  auto start_single = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_tasks; ++i)
    pool_single.enqueue_detached(do_work);
  pool_single.wait_all();
  auto end_single = std::chrono::high_resolution_clock::now();
  auto duration_single = std::chrono::duration_cast<std::chrono::milliseconds>(end_single - start_single);
  
  // Parallel (multiple threads)
  size_t num_threads = std::min(8u, std::thread::hardware_concurrency());
  ThreadPool pool_parallel(num_threads);
  auto start_parallel = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_tasks; ++i)
    pool_parallel.enqueue_detached(do_work);
  pool_parallel.wait_all();
  auto end_parallel = std::chrono::high_resolution_clock::now();
  auto duration_parallel = std::chrono::duration_cast<std::chrono::milliseconds>(end_parallel - start_parallel);
  
  // Parallel should be significantly faster
  double speedup = static_cast<double>(duration_single.count()) / duration_parallel.count();
  EXPECT_GT(speedup, 1.5);  // At least 1.5x speedup
}

TEST_F(ThreadPoolTest, LowLatencySmallTasks)
{
  ThreadPool pool(4);
  const int num_tasks = 1000;
  
  auto start = std::chrono::high_resolution_clock::now();
  
  std::vector<std::future<int>> futures;
  futures.reserve(num_tasks);
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue([i] { return i; }));
  
  for (auto& f : futures)
    f.get();
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  // Should complete 1000 trivial tasks in less than 100ms
  EXPECT_LT(duration.count(), 100000);
}

// ============================================================================
// Complex Callable Tests
// ============================================================================

TEST_F(ThreadPoolTest, NestedLambdas)
{
  ThreadPool pool(2);
  
  auto outer = [](int x) {
    return [x](int y) {
      return x + y;
    };
  };
  
  auto future = pool.enqueue([outer] {
    auto inner = outer(10);
    return inner(32);
  });
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, StdBindExpression)
{
  ThreadPool pool(2);
  
  auto add = [](int a, int b, int c) { return a + b + c; };
  auto bound = std::bind(add, 10, std::placeholders::_1, 20);
  
  auto future = pool.enqueue(bound, 12);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, GenericLambda)
{
  ThreadPool pool(2);
  
  // Generic lambda (C++14+)
  auto generic_add = [](auto a, auto b) { return a + b; };
  
  auto future_int = pool.enqueue(generic_add, 20, 22);
  auto future_double = pool.enqueue(generic_add, 20.5, 21.5);
  
  EXPECT_EQ(future_int.get(), 42);
  EXPECT_DOUBLE_EQ(future_double.get(), 42.0);
}

TEST_F(ThreadPoolTest, CaptureByMoveInLambda)
{
  ThreadPool pool(2);
  
  std::vector<int> large_data(1000, 42);
  
  auto future = pool.enqueue([data = std::move(large_data)]() mutable {
    return std::accumulate(data.begin(), data.end(), 0);
  });
  
  EXPECT_EQ(future.get(), 42000);
}

// ============================================================================
// Edge Case: Very Many Arguments
// ============================================================================

TEST_F(ThreadPoolTest, ManyArguments)
{
  ThreadPool pool(2);
  
  auto sum_all = [](int a, int b, int c, int d, int e, 
                    int f, int g, int h, int i, int j) {
    return a + b + c + d + e + f + g + h + i + j;
  };
  
  auto future = pool.enqueue(sum_all, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  
  EXPECT_EQ(future.get(), 55);
}

// ============================================================================
// Producer-Consumer Pattern
// ============================================================================

TEST_F(ThreadPoolTest, ProducerConsumerPattern)
{
  ThreadPool pool(4);
  std::queue<int> work_queue;
  std::mutex queue_mutex;
  std::condition_variable cv;
  std::atomic<bool> done{false};
  std::atomic<int> consumed{0};
  
  // Producer task
  pool.enqueue_detached([&work_queue, &queue_mutex, &cv, &done] {
    for (int i = 0; i < 100; ++i)
      {
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          work_queue.push(i);
        }
        cv.notify_one();
        std::this_thread::sleep_for(1ms);
      }
    done = true;
    cv.notify_all();
  });
  
  // Consumer tasks
  for (int c = 0; c < 3; ++c)
    {
      pool.enqueue_detached([&work_queue, &queue_mutex, &cv, &done, &consumed] {
        while (true)
          {
            {
              std::unique_lock<std::mutex> lock(queue_mutex);
              cv.wait_for(lock, 10ms, [&] { return !work_queue.empty() || done; });
              if (work_queue.empty() && done)
                return;
              if (work_queue.empty())
                continue;
              work_queue.pop();  // Consume the item
            }
            ++consumed;
          }
      });
    }
  
  pool.wait_all();
  
  EXPECT_EQ(consumed.load(), 100);
}

// ============================================================================
// Bounded Queue Tests
// ============================================================================

TEST_F(ThreadPoolTest, SetQueueLimitsBasic)
{
  ThreadPool pool(2);
  
  pool.set_queue_limits(100, 500);
  auto [soft, hard] = pool.get_queue_limits();
  
  EXPECT_EQ(soft, 100u);
  EXPECT_EQ(hard, 500u);
}

TEST_F(ThreadPoolTest, SetQueueLimitsDefaultHard)
{
  ThreadPool pool(2);
  
  pool.set_queue_limits(100);  // hard should default to 10x soft
  auto [soft, hard] = pool.get_queue_limits();
  
  EXPECT_EQ(soft, 100u);
  EXPECT_EQ(hard, 1000u);  // 10 * 100
}

TEST_F(ThreadPoolTest, EnqueueBoundedBelowSoftLimit)
{
  ThreadPool pool(4);
  pool.set_queue_limits(100, 1000);
  
  // Should not block when below soft limit
  std::vector<std::future<int>> futures;
  for (int i = 0; i < 50; ++i)
    futures.push_back(pool.enqueue_bounded([i] { return i; }));
  
  for (int i = 0; i < 50; ++i)
    EXPECT_EQ(futures[i].get(), i);
}

TEST_F(ThreadPoolTest, EnqueueBoundedBlocksAtSoftLimit)
{
  ThreadPool pool(1);  // Single worker
  pool.set_queue_limits(5, 100);
  
  std::atomic<bool> worker_blocked{true};
  std::atomic<int> enqueued{0};
  
  // Block the only worker
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Enqueue up to soft limit (should succeed immediately)
  for (int i = 0; i < 5; ++i)
    {
      pool.enqueue_bounded_detached([&enqueued] { ++enqueued; });
    }
  
  EXPECT_EQ(pool.pending_tasks(), 5u);
  
  // Next enqueue should block
  std::atomic<bool> enqueue_completed{false};
  std::thread blocker([&pool, &enqueue_completed] {
    pool.enqueue_bounded_detached([] {});
    enqueue_completed = true;
  });
  
  // Give it time to try to enqueue
  std::this_thread::sleep_for(50ms);
  EXPECT_FALSE(enqueue_completed);  // Should still be blocked
  
  // Release the worker
  worker_blocked = false;
  
  // Now it should complete
  blocker.join();
  pool.wait_all();
  
  EXPECT_TRUE(enqueue_completed);
}

TEST_F(ThreadPoolTest, EnqueueBoundedThrowsAtHardLimit)
{
  ThreadPool pool(1);
  pool.set_queue_limits(10, 15);
  
  std::atomic<bool> worker_blocked{true};
  
  // Block the worker
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Fill up to hard limit using regular enqueue (bypasses limits)
  for (int i = 0; i < 15; ++i)
    pool.enqueue_detached([] {});
  
  // Now bounded enqueue should throw
  EXPECT_THROW(pool.enqueue_bounded([] { return 0; }), Aleph::queue_overflow_error);
  
  // Clean up
  worker_blocked = false;
  pool.wait_all();
}

TEST_F(ThreadPoolTest, QueueOverflowErrorContainsInfo)
{
  ThreadPool pool(1);
  pool.set_queue_limits(5, 10);
  
  std::atomic<bool> worker_blocked{true};
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Fill queue beyond hard limit
  for (int i = 0; i < 10; ++i)
    pool.enqueue_detached([] {});
  
  try
    {
      pool.enqueue_bounded([] { return 0; });
      FAIL() << "Expected queue_overflow_error";
    }
  catch (const Aleph::queue_overflow_error& e)
    {
      EXPECT_GE(e.current_size(), 10u);
      EXPECT_EQ(e.hard_limit(), 10u);
      EXPECT_NE(std::string(e.what()).find("overflow"), std::string::npos);
    }
  
  worker_blocked = false;
  pool.wait_all();
}

TEST_F(ThreadPoolTest, EnqueueBoundedDetachedBlocksAtSoftLimit)
{
  ThreadPool pool(1);
  pool.set_queue_limits(3, 100);
  
  std::atomic<bool> worker_blocked{true};
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Fill to soft limit
  for (int i = 0; i < 3; ++i)
    pool.enqueue_bounded_detached([] {});
  
  // Next should block
  std::atomic<bool> done{false};
  std::thread t([&pool, &done] {
    pool.enqueue_bounded_detached([] {});
    done = true;
  });
  
  std::this_thread::sleep_for(30ms);
  EXPECT_FALSE(done);
  
  worker_blocked = false;
  t.join();
  pool.wait_all();
  
  EXPECT_TRUE(done);
}

TEST_F(ThreadPoolTest, BoundedEnqueueOnStoppedPoolThrows)
{
  ThreadPool pool(2);
  pool.set_queue_limits(100, 1000);
  pool.shutdown();
  
  EXPECT_THROW(pool.enqueue_bounded([] { return 0; }), std::runtime_error);
  EXPECT_THROW(pool.enqueue_bounded_detached([] {}), std::runtime_error);
}

TEST_F(ThreadPoolTest, BackpressureSlowsProducer)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  
  std::atomic<int> produced{0};
  std::atomic<int> consumed{0};
  std::atomic<bool> stop_producing{false};
  
  // Consumer tasks (slow)
  auto consume = [&consumed] {
    std::this_thread::sleep_for(5ms);
    ++consumed;
  };
  
  // Producer thread (fast)
  std::thread producer([&] {
    while (!stop_producing && produced < 50)
      {
        try
          {
            pool.enqueue_bounded_detached(consume);
            ++produced;
          }
        catch (const Aleph::queue_overflow_error&)
          {
            // Hard limit reached, stop
            break;
          }
      }
  });
  
  // Let it run for a bit
  std::this_thread::sleep_for(200ms);
  stop_producing = true;
  producer.join();
  pool.wait_all();
  
  // Producer should have been slowed down by backpressure
  // (can't produce much faster than consumers can consume)
  EXPECT_EQ(consumed.load(), produced.load());
}

TEST_F(ThreadPoolTest, ConcurrentBoundedEnqueue)
{
  ThreadPool pool(4);
  pool.set_queue_limits(50, 200);
  
  std::atomic<int> counter{0};
  const int num_producers = 8;
  const int tasks_per_producer = 100;
  
  std::vector<std::thread> producers;
  for (int p = 0; p < num_producers; ++p)
    {
      producers.emplace_back([&pool, &counter, tasks_per_producer] {
        for (int i = 0; i < tasks_per_producer; ++i)
          {
            try
              {
                pool.enqueue_bounded_detached([&counter] { ++counter; });
              }
            catch (const Aleph::queue_overflow_error&)
              {
                // Retry after small delay
                std::this_thread::sleep_for(1ms);
                --i;
              }
          }
      });
    }
  
  for (auto& t : producers)
    t.join();
  
  pool.wait_all();
  
  EXPECT_EQ(counter.load(), num_producers * tasks_per_producer);
}

TEST_F(ThreadPoolTest, BoundedEnqueueWithFutureResults)
{
  ThreadPool pool(4);
  pool.set_queue_limits(20, 100);
  
  std::vector<std::future<int>> futures;
  for (int i = 0; i < 100; ++i)
    futures.push_back(pool.enqueue_bounded([i] { return i * i; }));
  
  for (int i = 0; i < 100; ++i)
    EXPECT_EQ(futures[i].get(), i * i);
}

TEST_F(ThreadPoolTest, BoundedEnqueueExceptionPropagation)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  
  auto future = pool.enqueue_bounded([]() -> int { 
    throw std::runtime_error("test"); 
  });
  
  EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(ThreadPoolTest, MixedBoundedAndUnboundedEnqueue)
{
  ThreadPool pool(4);
  pool.set_queue_limits(10, 50);
  
  std::atomic<int> counter{0};
  
  // Mix bounded and unbounded
  for (int i = 0; i < 100; ++i)
    {
      if (i % 2 == 0)
        pool.enqueue_detached([&counter] { ++counter; });
      else
        {
          try
            {
              pool.enqueue_bounded_detached([&counter] { ++counter; });
            }
          catch (const Aleph::queue_overflow_error&)
            {
              // Unbounded doesn't care about limits
              pool.enqueue_detached([&counter] { ++counter; });
            }
        }
    }
  
  pool.wait_all();
  EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, BoundedEnqueueStressTest)
{
  ThreadPool pool(8);
  pool.set_queue_limits(100, 500);
  
  std::atomic<int> counter{0};
  const int num_tasks = 10000;
  
  for (int i = 0; i < num_tasks; ++i)
    {
      // Keep trying until successful
      bool success = false;
      while (!success)
        {
          try
            {
              pool.enqueue_bounded_detached([&counter] { ++counter; });
              success = true;
            }
          catch (const Aleph::queue_overflow_error&)
            {
              std::this_thread::sleep_for(100us);
            }
        }
    }
  
  pool.wait_all();
  EXPECT_EQ(counter.load(), num_tasks);
}

// Note: soft_limit=0 is not a practical use case and has edge case issues,
// so we don't test it. Use soft_limit >= 1 for bounded queues.

TEST_F(ThreadPoolTest, UnlimitedByDefault)
{
  ThreadPool pool(2);
  
  auto [soft, hard] = pool.get_queue_limits();
  
  EXPECT_EQ(soft, std::numeric_limits<size_t>::max());
  EXPECT_EQ(hard, std::numeric_limits<size_t>::max());
}

TEST_F(ThreadPoolTest, BoundedEnqueueWithArguments)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  
  auto future = pool.enqueue_bounded([](int a, int b, int c) {
    return a + b + c;
  }, 10, 20, 12);
  
  EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, BoundedEnqueueWithReference)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  
  int value = 10;
  auto future = pool.enqueue_bounded([](int& x) { x *= 2; return x; }, 
                                      std::ref(value));
  
  EXPECT_EQ(future.get(), 20);
  EXPECT_EQ(value, 20);
}

// ============================================================================
// Try Enqueue Tests
// ============================================================================

TEST_F(ThreadPoolTest, TryEnqueueSucceedsWhenSpaceAvailable)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  
  auto result = pool.try_enqueue([] { return 42; });
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->get(), 42);
}

TEST_F(ThreadPoolTest, TryEnqueueFailsWhenQueueFull)
{
  ThreadPool pool(1);
  pool.set_queue_limits(5, 100);
  
  std::atomic<bool> worker_blocked{true};
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Fill to soft limit
  for (int i = 0; i < 5; ++i)
    pool.enqueue_detached([] {});
  
  // try_enqueue should return nullopt
  auto result = pool.try_enqueue([] { return 0; });
  EXPECT_FALSE(result.has_value());
  
  worker_blocked = false;
  pool.wait_all();
}

TEST_F(ThreadPoolTest, TryEnqueueDetachedSucceeds)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  std::atomic<int> counter{0};
  
  bool success = pool.try_enqueue_detached([&counter] { ++counter; });
  
  EXPECT_TRUE(success);
  pool.wait_all();
  EXPECT_EQ(counter.load(), 1);
}

TEST_F(ThreadPoolTest, TryEnqueueDetachedFailsWhenFull)
{
  ThreadPool pool(1);
  pool.set_queue_limits(3, 100);
  
  std::atomic<bool> worker_blocked{true};
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Fill to soft limit
  for (int i = 0; i < 3; ++i)
    pool.enqueue_detached([] {});
  
  // try_enqueue_detached should return false
  bool success = pool.try_enqueue_detached([] {});
  EXPECT_FALSE(success);
  
  worker_blocked = false;
  pool.wait_all();
}

TEST_F(ThreadPoolTest, TryEnqueueOnStoppedPoolThrows)
{
  ThreadPool pool(2);
  pool.shutdown();
  
  EXPECT_THROW(pool.try_enqueue([] { return 0; }), std::runtime_error);
  EXPECT_THROW(pool.try_enqueue_detached([] {}), std::runtime_error);
}

TEST_F(ThreadPoolTest, TryEnqueueWithArguments)
{
  ThreadPool pool(2);
  pool.set_queue_limits(10, 100);
  
  auto result = pool.try_enqueue([](int a, int b) { return a + b; }, 20, 22);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->get(), 42);
}

TEST_F(ThreadPoolTest, TryEnqueueNonBlockingBehavior)
{
  ThreadPool pool(1);
  pool.set_queue_limits(2, 100);
  
  std::atomic<bool> worker_blocked{true};
  pool.enqueue_detached([&worker_blocked] { 
    while (worker_blocked) 
      std::this_thread::sleep_for(1ms); 
  });
  
  // Fill queue
  pool.enqueue_detached([] {});
  pool.enqueue_detached([] {});
  
  // Measure time for try_enqueue when queue is full
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; ++i)
    pool.try_enqueue([] { return 0; });  // Should all fail fast
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  // 1000 non-blocking calls should complete in < 10ms
  EXPECT_LT(duration.count(), 10000);
  
  worker_blocked = false;
  pool.wait_all();
}

// ============================================================================
// Benchmark Tests
// ============================================================================

TEST_F(ThreadPoolTest, BenchmarkVsSequential)
{
  const int num_tasks = 1000;
  const auto work_duration = 100us;
  
  auto do_work = [work_duration] {
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < work_duration)
      ;  // Busy wait
    return 1;
  };
  
  // Sequential baseline
  auto seq_start = std::chrono::high_resolution_clock::now();
  int seq_sum = 0;
  for (int i = 0; i < num_tasks; ++i)
    seq_sum += do_work();
  auto seq_end = std::chrono::high_resolution_clock::now();
  auto seq_duration = std::chrono::duration_cast<std::chrono::milliseconds>(seq_end - seq_start);
  
  // ThreadPool parallel
  size_t num_threads = std::thread::hardware_concurrency();
  ThreadPool pool(num_threads);
  
  auto pool_start = std::chrono::high_resolution_clock::now();
  std::vector<std::future<int>> futures;
  futures.reserve(num_tasks);
  for (int i = 0; i < num_tasks; ++i)
    futures.push_back(pool.enqueue(do_work));
  
  int pool_sum = 0;
  for (auto& f : futures)
    pool_sum += f.get();
  auto pool_end = std::chrono::high_resolution_clock::now();
  auto pool_duration = std::chrono::duration_cast<std::chrono::milliseconds>(pool_end - pool_start);
  
  EXPECT_EQ(seq_sum, pool_sum);
  
  double speedup = static_cast<double>(seq_duration.count()) / pool_duration.count();
  
  std::cout << "\n=== Benchmark: ThreadPool vs Sequential ===\n";
  std::cout << "Tasks: " << num_tasks << ", Work per task: 100μs\n";
  std::cout << "Threads: " << num_threads << "\n";
  std::cout << "Sequential: " << seq_duration.count() << " ms\n";
  std::cout << "ThreadPool: " << pool_duration.count() << " ms\n";
  std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
  std::cout << "============================================\n\n";
  
  // Should achieve at least some speedup with multiple cores
  if (num_threads > 1)
    EXPECT_GT(speedup, 1.2);
}

TEST_F(ThreadPoolTest, BenchmarkEnqueueOverhead)
{
  ThreadPool pool(4);
  const int num_tasks = 100000;
  
  // Measure enqueue overhead (tasks do nothing)
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_tasks; ++i)
    pool.enqueue_detached([] {});
  pool.wait_all();
  auto end = std::chrono::high_resolution_clock::now();
  
  auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  double ns_per_task = static_cast<double>(total_ns) / num_tasks;
  
  std::cout << "\n=== Benchmark: Enqueue Overhead ===\n";
  std::cout << "Tasks: " << num_tasks << "\n";
  std::cout << "Total time: " << total_ns / 1000000.0 << " ms\n";
  std::cout << "Per task: " << std::fixed << std::setprecision(0) << ns_per_task << " ns\n";
  std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
            << (num_tasks * 1e9 / total_ns) << " tasks/sec\n";
  std::cout << "===================================\n\n";
  
  // Should be able to enqueue at least 50k tasks/sec
  // (relaxed for slower CI environments)
  EXPECT_LT(ns_per_task, 20000);  // < 20μs per task
}

TEST_F(ThreadPoolTest, BenchmarkVsStdAsync)
{
  const int num_tasks = 500;  // Fewer tasks because async is slow
  
  auto compute = [](int x) -> long long {
    long long sum = 0;
    for (int i = 0; i < 1000; ++i)
      sum += static_cast<long long>(i) * x;
    return sum;
  };
  
  // std::async
  auto async_start = std::chrono::high_resolution_clock::now();
  std::vector<std::future<long long>> async_futures;
  async_futures.reserve(num_tasks);
  for (int i = 0; i < num_tasks; ++i)
    async_futures.push_back(std::async(std::launch::async, compute, i));
  
  long long async_sum = 0;
  for (auto& f : async_futures)
    async_sum += f.get();
  auto async_end = std::chrono::high_resolution_clock::now();
  auto async_duration = std::chrono::duration_cast<std::chrono::microseconds>(async_end - async_start);
  
  // ThreadPool
  ThreadPool pool(std::thread::hardware_concurrency());
  
  auto pool_start = std::chrono::high_resolution_clock::now();
  std::vector<std::future<long long>> pool_futures;
  pool_futures.reserve(num_tasks);
  for (int i = 0; i < num_tasks; ++i)
    pool_futures.push_back(pool.enqueue(compute, i));
  
  long long pool_sum = 0;
  for (auto& f : pool_futures)
    pool_sum += f.get();
  auto pool_end = std::chrono::high_resolution_clock::now();
  auto pool_duration = std::chrono::duration_cast<std::chrono::microseconds>(pool_end - pool_start);
  
  EXPECT_EQ(async_sum, pool_sum);
  
  double speedup = static_cast<double>(async_duration.count()) / pool_duration.count();
  
  std::cout << "\n=== Benchmark: ThreadPool vs std::async ===\n";
  std::cout << "Tasks: " << num_tasks << "\n";
  std::cout << "std::async: " << async_duration.count() / 1000.0 << " ms\n";
  std::cout << "ThreadPool: " << pool_duration.count() / 1000.0 << " ms\n";
  std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
  std::cout << "============================================\n\n";
  
  // ThreadPool should be faster than creating new threads each time
  EXPECT_GT(speedup, 1.0);
}

// =============================================================================
// New Features Tests
// =============================================================================

// --- Statistics Tests ---

TEST_F(ThreadPoolTest, GetStatsInitialValues)
{
  ThreadPool pool(2);
  auto stats = pool.get_stats();
  
  EXPECT_EQ(stats.tasks_completed, 0u);
  EXPECT_EQ(stats.tasks_failed, 0u);
  EXPECT_EQ(stats.current_queue_size, 0u);
  EXPECT_EQ(stats.current_active, 0u);
  EXPECT_EQ(stats.num_workers, 2u);
  EXPECT_EQ(stats.peak_queue_size, 0u);
  EXPECT_EQ(stats.total_processed(), 0u);
}

TEST_F(ThreadPoolTest, StatsTrackCompletedTasks)
{
  ThreadPool pool(2);
  
  std::vector<std::future<int>> futures;
  for (int i = 0; i < 10; ++i)
    futures.push_back(pool.enqueue([] { return 42; }));
  
  for (auto& f : futures)
    f.get();
  
  pool.wait_all();
  
  auto stats = pool.get_stats();
  EXPECT_EQ(stats.tasks_completed, 10u);
  EXPECT_EQ(stats.total_processed(), 10u);
}

TEST_F(ThreadPoolTest, StatsTrackPeakQueueSize)
{
  ThreadPool pool(1);
  
  // Block the worker
  std::promise<void> blocker;
  pool.enqueue([&blocker] { blocker.get_future().wait(); });
  
  // Queue up several tasks
  for (int i = 0; i < 5; ++i)
    pool.enqueue_detached([] {});
  
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  auto stats = pool.get_stats();
  EXPECT_GE(stats.peak_queue_size, 5u);
  
  // Release worker
  blocker.set_value();
  pool.wait_all();
}

TEST_F(ThreadPoolTest, ResetStatsWorks)
{
  ThreadPool pool(2);
  
  pool.enqueue([] { return 1; }).get();
  // Wait for stats to be updated (tasks_completed incremented after future is set)
  pool.wait_all();
  
  auto stats1 = pool.get_stats();
  EXPECT_GE(stats1.tasks_completed, 1u);
  
  pool.reset_stats();
  
  auto stats2 = pool.get_stats();
  EXPECT_EQ(stats2.tasks_completed, 0u);
  EXPECT_EQ(stats2.peak_queue_size, 0u);
}

// --- Exception Callback Tests ---

TEST_F(ThreadPoolTest, ExceptionCallbackInvokedOnDetachedTaskFailure)
{
  ThreadPool pool(2);
  
  std::atomic<bool> callback_called{false};
  std::atomic<bool> correct_exception{false};
  
  pool.set_exception_callback([&](std::exception_ptr ep) {
    callback_called = true;
    try {
      std::rethrow_exception(ep);
    } catch (const std::runtime_error& e) {
      correct_exception = (std::string(e.what()) == "test error");
    } catch (...) {
      correct_exception = false;
    }
  });
  
  pool.enqueue_detached([] {
    throw std::runtime_error("test error");
  });
  
  pool.wait_all();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  
  EXPECT_TRUE(callback_called.load());
  EXPECT_TRUE(correct_exception.load());
}

TEST_F(ThreadPoolTest, ExceptionCallbackCanBeCleared)
{
  ThreadPool pool(2);
  
  std::atomic<int> callback_count{0};
  
  pool.set_exception_callback([&](std::exception_ptr) {
    ++callback_count;
  });
  
  pool.enqueue_detached([] { throw std::runtime_error("error"); });
  pool.wait_all();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  
  EXPECT_EQ(callback_count.load(), 1);
  
  // Clear callback
  pool.set_exception_callback(nullptr);
  
  pool.enqueue_detached([] { throw std::runtime_error("error"); });
  pool.wait_all();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  
  // Count should still be 1
  EXPECT_EQ(callback_count.load(), 1);
}

TEST_F(ThreadPoolTest, StatsTrackFailedDetachedTasks)
{
  ThreadPool pool(2);
  
  pool.enqueue_detached([] { throw std::runtime_error("fail"); });
  pool.enqueue_detached([] { throw std::logic_error("fail"); });
  pool.enqueue_detached([] { /* success */ });
  
  pool.wait_all();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  
  auto stats = pool.get_stats();
  EXPECT_EQ(stats.tasks_failed, 2u);
  EXPECT_EQ(stats.tasks_completed, 1u);
  EXPECT_EQ(stats.total_processed(), 3u);
}

// --- wait_all_for/wait_all_until Tests ---

TEST_F(ThreadPoolTest, WaitAllForSucceedsWhenIdle)
{
  ThreadPool pool(2);
  
  pool.enqueue([] { return 42; }).get();
  
  bool result = pool.wait_all_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(result);
}

TEST_F(ThreadPoolTest, WaitAllForTimesOutWhenBusy)
{
  ThreadPool pool(1);
  
  // Start a long task
  pool.enqueue_detached([] {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  });
  
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  bool result = pool.wait_all_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
  
  // Wait for cleanup
  pool.wait_all();
}

TEST_F(ThreadPoolTest, WaitAllUntilWorks)
{
  ThreadPool pool(2);
  
  pool.enqueue([] { return 1; }).get();
  
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
  bool result = pool.wait_all_until(deadline);
  EXPECT_TRUE(result);
}

// --- enqueue_batch Tests ---

TEST_F(ThreadPoolTest, EnqueueBatchExecutesAllTasks)
{
  ThreadPool pool(4);
  
  std::vector<std::tuple<int, int>> args = {
    {1, 2}, {3, 4}, {5, 6}, {7, 8}
  };
  
  auto futures = pool.enqueue_batch([](int a, int b) { return a + b; }, args);
  
  EXPECT_EQ(futures.size(), 4u);
  
  std::vector<int> results;
  for (auto& f : futures)
    results.push_back(f.get());
  
  EXPECT_EQ(results, (std::vector<int>{3, 7, 11, 15}));
}

TEST_F(ThreadPoolTest, EnqueueBatchWithLargeWorkload)
{
  ThreadPool pool(4);
  
  std::vector<std::tuple<int>> args;
  for (int i = 0; i < 100; ++i)
    args.emplace_back(i);
  
  auto futures = pool.enqueue_batch([](int x) { return x * x; }, args);
  
  int sum = 0;
  for (auto& f : futures)
    sum += f.get();
  
  // Sum of squares 0^2 + 1^2 + ... + 99^2 = 328350
  EXPECT_EQ(sum, 328350);
}

TEST_F(ThreadPoolTest, EnqueueBatchEmptyContainer)
{
  ThreadPool pool(2);
  
  std::vector<std::tuple<int>> empty;
  auto futures = pool.enqueue_batch([](int x) { return x; }, empty);
  
  EXPECT_TRUE(futures.empty());
}

// --- parallel_for Tests ---

TEST_F(ThreadPoolTest, ParallelForModifiesElements)
{
  ThreadPool pool(4);
  
  std::vector<int> data(100, 1);
  
  parallel_for(pool, data.begin(), data.end(), [](int& x) { x *= 2; });
  
  for (int x : data)
    EXPECT_EQ(x, 2);
}

TEST_F(ThreadPoolTest, ParallelForWithChunkFunction)
{
  ThreadPool pool(4);
  
  std::vector<int> data(100, 1);
  
  parallel_for(pool, data.begin(), data.end(), 
               [](auto begin, auto end) {
                 for (auto it = begin; it != end; ++it)
                   *it = 5;
               });
  
  for (int x : data)
    EXPECT_EQ(x, 5);
}

TEST_F(ThreadPoolTest, ParallelForEmptyRange)
{
  ThreadPool pool(2);
  
  std::vector<int> empty;
  
  // Should not throw or crash
  parallel_for(pool, empty.begin(), empty.end(), [](int& x) { x = 0; });
  
  EXPECT_TRUE(empty.empty());
}

TEST_F(ThreadPoolTest, ParallelForCustomChunkSize)
{
  ThreadPool pool(2);
  
  std::vector<std::atomic<int>> data(50);
  for (auto& x : data)
    x = 0;
  
  parallel_for(pool, data.begin(), data.end(), 
               [](std::atomic<int>& x) { ++x; },
               10);  // chunk size = 10
  
  for (const auto& x : data)
    EXPECT_EQ(x.load(), 1);
}

// --- parallel_for_index Tests ---

TEST_F(ThreadPoolTest, ParallelForIndexWorks)
{
  ThreadPool pool(4);
  
  std::vector<int> data(100, 0);
  
  parallel_for_index(pool, 0, data.size(), [&data](size_t i) {
    data[i] = static_cast<int>(i * 2);
  });
  
  for (size_t i = 0; i < data.size(); ++i)
    EXPECT_EQ(data[i], static_cast<int>(i * 2));
}

TEST_F(ThreadPoolTest, ParallelForIndexEmptyRange)
{
  ThreadPool pool(2);
  
  // Should not throw
  parallel_for_index(pool, 10, 10, [](size_t) { /* nothing */ });
  parallel_for_index(pool, 10, 5, [](size_t) { /* nothing */ });
}

// --- parallel_transform Tests ---

TEST_F(ThreadPoolTest, ParallelTransformBasic)
{
  ThreadPool pool(4);
  
  std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  std::vector<int> output(10);
  
  parallel_transform(pool, input.begin(), input.end(), output.begin(),
                     [](int x) { return x * x; });
  
  EXPECT_EQ(output, (std::vector<int>{1, 4, 9, 16, 25, 36, 49, 64, 81, 100}));
}

TEST_F(ThreadPoolTest, ParallelTransformLargeData)
{
  ThreadPool pool(4);
  
  std::vector<int> input(1000);
  std::iota(input.begin(), input.end(), 0);
  
  std::vector<int> output(1000);
  
  parallel_transform(pool, input.begin(), input.end(), output.begin(),
                     [](int x) { return x + 1; });
  
  for (int i = 0; i < 1000; ++i)
    EXPECT_EQ(output[i], i + 1);
}

TEST_F(ThreadPoolTest, ParallelTransformEmpty)
{
  ThreadPool pool(2);
  
  std::vector<int> input;
  std::vector<int> output;
  
  auto end = parallel_transform(pool, input.begin(), input.end(), 
                                output.begin(), [](int x) { return x; });
  
  EXPECT_EQ(end, output.begin());
}

// --- parallel_reduce Tests ---

TEST_F(ThreadPoolTest, ParallelReduceSum)
{
  ThreadPool pool(4);
  
  std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  
  int sum = parallel_reduce(pool, data.begin(), data.end(), 0, std::plus<int>());
  
  EXPECT_EQ(sum, 55);
}

TEST_F(ThreadPoolTest, ParallelReduceProduct)
{
  ThreadPool pool(4);
  
  std::vector<int> data = {1, 2, 3, 4, 5};
  
  int product = parallel_reduce(pool, data.begin(), data.end(), 1, 
                                std::multiplies<int>());
  
  EXPECT_EQ(product, 120);
}

TEST_F(ThreadPoolTest, ParallelReduceMax)
{
  ThreadPool pool(4);
  
  std::vector<int> data = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
  
  int max_val = parallel_reduce(pool, data.begin(), data.end(), 
                                std::numeric_limits<int>::min(),
                                [](int a, int b) { return std::max(a, b); });
  
  EXPECT_EQ(max_val, 9);
}

TEST_F(ThreadPoolTest, ParallelReduceLargeData)
{
  ThreadPool pool(4);
  
  std::vector<long long> data(10000);
  std::iota(data.begin(), data.end(), 1LL);
  
  long long sum = parallel_reduce(pool, data.begin(), data.end(), 0LL,
                                  std::plus<long long>());
  
  // Sum 1+2+...+10000 = 10000*10001/2 = 50005000
  EXPECT_EQ(sum, 50005000LL);
}

TEST_F(ThreadPoolTest, ParallelReduceEmpty)
{
  ThreadPool pool(2);
  
  std::vector<int> empty;
  
  int result = parallel_reduce(pool, empty.begin(), empty.end(), 42,
                               std::plus<int>());
  
  EXPECT_EQ(result, 42);  // Returns init for empty range
}

// --- ThreadPoolStats Tests ---

TEST_F(ThreadPoolTest, ThreadPoolStatsQueueUtilization)
{
  ThreadPoolStats stats;
  stats.current_queue_size = 50;
  
  EXPECT_NEAR(stats.queue_utilization(100), 50.0, 0.01);
  EXPECT_NEAR(stats.queue_utilization(200), 25.0, 0.01);
  EXPECT_NEAR(stats.queue_utilization(0), 0.0, 0.01);  // Edge case
  EXPECT_NEAR(stats.queue_utilization(std::numeric_limits<size_t>::max()), 0.0, 0.01);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

