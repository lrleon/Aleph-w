
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



/**
 * @file future_utils_test.cc
 * @brief Tests for Future Utils
 */
#include <gtest/gtest.h>
#include <future>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <atomic>
#include <future_utils.H>

using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

class FutureUtilsTest : public ::testing::Test
{
protected:
  // Helper to create a simple async task that returns its input
  static std::future<int> make_int_future(int value)
  {
    return std::async(std::launch::async, [value] { return value; });
  }

  // Helper to create a delayed async task
  static std::future<int> make_delayed_future(int value, int delay_ms)
  {
    return std::async(std::launch::async, [value, delay_ms] {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
      return value;
    });
  }

  // Helper to create a void async task
  static std::future<void> make_void_future()
  {
    return std::async(std::launch::async, [] {});
  }

  // Helper to create a void async task that modifies a counter
  static std::future<void> make_void_future_with_effect(std::atomic<int> & counter)
  {
    return std::async(std::launch::async, [&counter] { ++counter; });
  }

  // Helper to create a failing async task
  static std::future<int> make_failing_future()
  {
    return std::async(std::launch::async, []() -> int {
      throw std::runtime_error("Test exception");
    });
  }
};

// =============================================================================
// Basic get_futures Tests (int)
// =============================================================================

TEST_F(FutureUtilsTest, GetFuturesEmptyList)
{
  DynList<std::future<int>> futures;
  auto results = get_futures(futures);

  EXPECT_TRUE(results.is_empty());
  EXPECT_TRUE(futures.is_empty());
}

TEST_F(FutureUtilsTest, GetFuturesSingleElement)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(42));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 1u);
  EXPECT_EQ(results.get_first(), 42);
  EXPECT_TRUE(futures.is_empty());
}

TEST_F(FutureUtilsTest, GetFuturesMultipleElements)
{
  DynList<std::future<int>> futures;
  for (int i = 1; i <= 5; ++i)
    futures.append(make_int_future(i * 10));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 5u);
  EXPECT_TRUE(futures.is_empty());

  // Verify order is preserved
  int expected = 10;
  results.traverse([&expected](int val) {
    EXPECT_EQ(val, expected);
    expected += 10;
    return true;
  });
}

TEST_F(FutureUtilsTest, GetFuturesPreservesOrder)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(1));
  futures.append(make_int_future(2));
  futures.append(make_int_future(3));

  auto results = get_futures(futures);

  auto it = results.get_it();
  EXPECT_EQ(it.get_curr(), 1); it.next();
  EXPECT_EQ(it.get_curr(), 2); it.next();
  EXPECT_EQ(it.get_curr(), 3);
}

TEST_F(FutureUtilsTest, GetFuturesWithDelayedTasks)
{
  DynList<std::future<int>> futures;
  futures.append(make_delayed_future(100, 50));
  futures.append(make_delayed_future(200, 10));
  futures.append(make_delayed_future(300, 30));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 3u);

  // Order should be preserved (100, 200, 300), not completion order
  auto it = results.get_it();
  EXPECT_EQ(it.get_curr(), 100); it.next();
  EXPECT_EQ(it.get_curr(), 200); it.next();
  EXPECT_EQ(it.get_curr(), 300);
}

// =============================================================================
// get_futures Rvalue Overload Tests
// =============================================================================

TEST_F(FutureUtilsTest, GetFuturesRvalue)
{
  auto make_list = []() {
    DynList<std::future<int>> futures;
    futures.append(std::async(std::launch::async, [] { return 42; }));
    futures.append(std::async(std::launch::async, [] { return 43; }));
    return futures;
  };

  auto results = get_futures(make_list());

  EXPECT_EQ(results.size(), 2u);
}

// =============================================================================
// Void Future Tests
// =============================================================================

TEST_F(FutureUtilsTest, GetVoidFuturesEmptyList)
{
  DynList<std::future<void>> futures;
  EXPECT_NO_THROW(get_futures(futures));
  EXPECT_TRUE(futures.is_empty());
}

TEST_F(FutureUtilsTest, GetVoidFuturesSingleElement)
{
  DynList<std::future<void>> futures;
  futures.append(make_void_future());

  EXPECT_NO_THROW(get_futures(futures));
  EXPECT_TRUE(futures.is_empty());
}

TEST_F(FutureUtilsTest, GetVoidFuturesMultipleElements)
{
  std::atomic<int> counter{0};
  DynList<std::future<void>> futures;

  for (int i = 0; i < 5; ++i)
    futures.append(make_void_future_with_effect(counter));

  get_futures(futures);

  EXPECT_EQ(counter.load(), 5);
  EXPECT_TRUE(futures.is_empty());
}

TEST_F(FutureUtilsTest, GetVoidFuturesRvalue)
{
  std::atomic<int> counter{0};

  auto make_list = [&counter]() {
    DynList<std::future<void>> futures;
    futures.append(std::async(std::launch::async, [&counter] { ++counter; }));
    futures.append(std::async(std::launch::async, [&counter] { ++counter; }));
    return futures;
  };

  get_futures(make_list());

  EXPECT_EQ(counter.load(), 2);
}

// =============================================================================
// Exception Handling Tests
// =============================================================================

TEST_F(FutureUtilsTest, GetFuturesPropagatesException)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(1));
  futures.append(make_failing_future());
  futures.append(make_int_future(3));

  EXPECT_THROW(get_futures(futures), std::runtime_error);
}

TEST_F(FutureUtilsTest, GetVoidFuturesPropagatesException)
{
  DynList<std::future<void>> futures;
  futures.append(make_void_future());
  futures.append(std::async(std::launch::async, [] {
    throw std::logic_error("Void exception");
  }));

  EXPECT_THROW(get_futures(futures), std::logic_error);
}

// =============================================================================
// all_ready Tests
// =============================================================================

TEST_F(FutureUtilsTest, AllReadyEmptyList)
{
  DynList<std::future<int>> futures;
  EXPECT_TRUE(all_ready(futures));
}

TEST_F(FutureUtilsTest, AllReadyWithReadyFutures)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(1));
  futures.append(make_int_future(2));

  // Give futures time to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_TRUE(all_ready(futures));
}

TEST_F(FutureUtilsTest, AllReadyWithPendingFutures)
{
  DynList<std::future<int>> futures;
  futures.append(make_delayed_future(1, 500)); // Long delay

  // Check immediately - should not be ready
  EXPECT_FALSE(all_ready(futures));

  // Clean up
  get_futures(futures);
}

TEST_F(FutureUtilsTest, AllReadyMixed)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(1)); // Ready immediately
  futures.append(make_delayed_future(2, 500)); // Still pending

  // Give first future time but not second
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_FALSE(all_ready(futures));

  // Clean up
  get_futures(futures);
}

// =============================================================================
// count_ready Tests
// =============================================================================

TEST_F(FutureUtilsTest, CountReadyEmptyList)
{
  DynList<std::future<int>> futures;
  EXPECT_EQ(count_ready(futures), 0u);
}

TEST_F(FutureUtilsTest, CountReadyAllReady)
{
  DynList<std::future<int>> futures;
  for (int i = 0; i < 5; ++i)
    futures.append(make_int_future(i));

  // Give futures time to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(count_ready(futures), 5u);

  // Clean up
  get_futures(futures);
}

TEST_F(FutureUtilsTest, CountReadyNoneReady)
{
  DynList<std::future<int>> futures;
  for (int i = 0; i < 3; ++i)
    futures.append(make_delayed_future(i, 500));

  // Check immediately
  EXPECT_EQ(count_ready(futures), 0u);

  // Clean up
  get_futures(futures);
}

TEST_F(FutureUtilsTest, CountReadyPartiallyReady)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(1));
  futures.append(make_int_future(2));
  futures.append(make_delayed_future(3, 500));

  // Give fast futures time to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  size_t ready_count = count_ready(futures);
  EXPECT_GE(ready_count, 2u);
  EXPECT_LE(ready_count, 3u);

  // Clean up
  get_futures(futures);
}

// =============================================================================
// Different Types Tests
// =============================================================================

TEST_F(FutureUtilsTest, GetFuturesWithStrings)
{
  DynList<std::future<std::string>> futures;
  futures.append(std::async(std::launch::async, [] { return std::string("hello"); }));
  futures.append(std::async(std::launch::async, [] { return std::string("world"); }));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 2u);

  auto it = results.get_it();
  EXPECT_EQ(it.get_curr(), "hello"); it.next();
  EXPECT_EQ(it.get_curr(), "world");
}

TEST_F(FutureUtilsTest, GetFuturesWithDoubles)
{
  DynList<std::future<double>> futures;
  futures.append(std::async(std::launch::async, [] { return 3.14; }));
  futures.append(std::async(std::launch::async, [] { return 2.71; }));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 2u);

  auto it = results.get_it();
  EXPECT_DOUBLE_EQ(it.get_curr(), 3.14); it.next();
  EXPECT_DOUBLE_EQ(it.get_curr(), 2.71);
}

TEST_F(FutureUtilsTest, GetFuturesWithVectors)
{
  DynList<std::future<std::vector<int>>> futures;
  futures.append(std::async(std::launch::async, [] {
    return std::vector<int>{1, 2, 3};
  }));
  futures.append(std::async(std::launch::async, [] {
    return std::vector<int>{4, 5};
  }));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 2u);

  auto it = results.get_it();
  EXPECT_EQ(it.get_curr().size(), 3u);
  it.next();
  EXPECT_EQ(it.get_curr().size(), 2u);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(FutureUtilsTest, ManyFutures)
{
  DynList<std::future<int>> futures;
  constexpr int N = 100;

  for (int i = 0; i < N; ++i)
    futures.append(make_int_future(i));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), static_cast<size_t>(N));
  EXPECT_TRUE(futures.is_empty());

  // Verify all values
  int expected = 0;
  results.traverse([&expected](int val) {
    EXPECT_EQ(val, expected++);
    return true;
  });
}

TEST_F(FutureUtilsTest, ManyVoidFutures)
{
  std::atomic<int> counter{0};
  DynList<std::future<void>> futures;
  constexpr int N = 100;

  for (int i = 0; i < N; ++i)
    futures.append(make_void_future_with_effect(counter));

  get_futures(futures);

  EXPECT_EQ(counter.load(), N);
  EXPECT_TRUE(futures.is_empty());
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(FutureUtilsTest, FutureWithZeroValue)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(0));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 1u);
  EXPECT_EQ(results.get_first(), 0);
}

TEST_F(FutureUtilsTest, FutureWithNegativeValue)
{
  DynList<std::future<int>> futures;
  futures.append(make_int_future(-42));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 1u);
  EXPECT_EQ(results.get_first(), -42);
}

TEST_F(FutureUtilsTest, FutureWithEmptyString)
{
  DynList<std::future<std::string>> futures;
  futures.append(std::async(std::launch::async, [] { return std::string(); }));

  auto results = get_futures(futures);

  EXPECT_EQ(results.size(), 1u);
  EXPECT_TRUE(results.get_first().empty());
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
