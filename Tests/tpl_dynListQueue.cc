
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


/**
 * @file tpl_dynListQueue.cc
 * @brief Exhaustive test suite for DynListQueue<T>
 *
 * This file contains comprehensive tests for the DynListQueue class,
 * covering all public methods, edge cases, exception handling,
 * and iterator functionality.
 */

# include <gtest/gtest.h>

# include <string>
# include <memory>
# include <vector>
# include <stdexcept>

# include <tpl_dynListQueue.H>
# include <ahFunctional.H>
# include <ah-zip.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Test Fixture for Basic Operations
// =============================================================================

class DynListQueueTest : public ::testing::Test
{
protected:
  DynListQueue<int> empty_queue;
  DynListQueue<int> queue_with_items;

  static constexpr size_t N = 100;

  void SetUp() override
  {
    for (size_t i = 0; i < N; ++i)
      queue_with_items.put(static_cast<int>(i));
  }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(DynListQueueTest, DefaultConstruction)
{
  DynListQueue<int> q;
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0u);
}

TEST_F(DynListQueueTest, CopyConstruction)
{
  DynListQueue<int> copy(queue_with_items);

  EXPECT_EQ(copy.size(), queue_with_items.size());
  EXPECT_EQ(copy.size(), N);

  // Verify independent copies
  while (!queue_with_items.is_empty())
  {
    EXPECT_EQ(queue_with_items.get(), copy.get());
  }
  EXPECT_TRUE(queue_with_items.is_empty());
  EXPECT_TRUE(copy.is_empty());
}

TEST_F(DynListQueueTest, MoveConstruction)
{
  DynListQueue<int> source;
  for (int i = 0; i < 10; ++i)
    source.put(i);

  size_t original_size = source.size();
  DynListQueue<int> moved(std::move(source));

  EXPECT_EQ(moved.size(), original_size);
  EXPECT_TRUE(source.is_empty());  // Source should be empty after move

  // Verify content
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(moved.get(), i);
}

TEST_F(DynListQueueTest, InitializerListConstruction)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};

  EXPECT_EQ(q.size(), 5u);
  EXPECT_EQ(q.front(), 1);
  EXPECT_EQ(q.rear(), 5);
}

TEST_F(DynListQueueTest, IteratorRangeConstruction)
{
  std::vector<int> vec = {10, 20, 30, 40, 50};
  DynListQueue<int> q(vec.begin(), vec.end());

  EXPECT_EQ(q.size(), vec.size());
  for (int val : vec)
    EXPECT_EQ(q.get(), val);
}

TEST_F(DynListQueueTest, DynListConstruction)
{
  DynList<int> list = {100, 200, 300};
  DynListQueue<int> q(list);

  EXPECT_EQ(q.size(), list.size());
  // Elements should be in the same order
  auto it = list.get_it();
  while (!q.is_empty() && it.has_curr())
  {
    EXPECT_EQ(q.get(), it.get_curr());
    it.next();
  }
}

// =============================================================================
// Assignment Tests
// =============================================================================

TEST_F(DynListQueueTest, CopyAssignment)
{
  DynListQueue<int> q;
  q.put(999);  // Pre-existing content

  q = queue_with_items;

  EXPECT_EQ(q.size(), N);
  EXPECT_EQ(q.front(), 0);
  EXPECT_EQ(q.rear(), static_cast<int>(N - 1));
}

TEST_F(DynListQueueTest, CopyAssignmentSelf)
{
  DynListQueue<int> q = {1, 2, 3};
  q = q;  // Self-assignment

  EXPECT_EQ(q.size(), 3u);
  EXPECT_EQ(q.front(), 1);
}

TEST_F(DynListQueueTest, MoveAssignment)
{
  DynListQueue<int> source = {1, 2, 3};
  DynListQueue<int> target;
  target.put(999);

  target = std::move(source);

  EXPECT_EQ(target.size(), 3u);
  EXPECT_EQ(target.front(), 1);
  // Source should have the old target content or be empty
}

// =============================================================================
// Core Queue Operations Tests
// =============================================================================

TEST_F(DynListQueueTest, PutByCopy)
{
  DynListQueue<int> q;
  int value = 42;

  int& ref = q.put(value);

  EXPECT_EQ(q.size(), 1u);
  EXPECT_EQ(ref, 42);
  EXPECT_EQ(q.front(), 42);
  EXPECT_EQ(q.rear(), 42);
}

TEST_F(DynListQueueTest, PutByMove)
{
  DynListQueue<std::string> q;
  std::string value = "hello";

  std::string& ref = q.put(std::move(value));

  EXPECT_EQ(q.size(), 1u);
  EXPECT_EQ(ref, "hello");
  // Original might be empty or in valid but unspecified state
}

TEST_F(DynListQueueTest, AppendAndInsertAliases)
{
  DynListQueue<int> q;

  q.append(1);
  q.insert(2);
  q.put(3);

  EXPECT_EQ(q.size(), 3u);
  // All should be at rear (FIFO order)
  EXPECT_EQ(q.get(), 1);
  EXPECT_EQ(q.get(), 2);
  EXPECT_EQ(q.get(), 3);
}

TEST_F(DynListQueueTest, GetFIFOOrder)
{
  DynListQueue<int> q;
  for (int i = 0; i < 10; ++i)
    q.put(i);

  for (int i = 0; i < 10; ++i)
  {
    EXPECT_EQ(q.front(), i);
    EXPECT_EQ(q.get(), i);
  }

  EXPECT_TRUE(q.is_empty());
}

TEST_F(DynListQueueTest, GetFromEmptyQueue)
{
  DynListQueue<int> q;
  EXPECT_THROW(q.get(), std::underflow_error);
}

TEST_F(DynListQueueTest, FrontPeek)
{
  EXPECT_EQ(queue_with_items.front(), 0);

  // Multiple peeks should return same value
  EXPECT_EQ(queue_with_items.front(), 0);
  EXPECT_EQ(queue_with_items.front(), 0);

  // Size should not change
  EXPECT_EQ(queue_with_items.size(), N);
}

TEST_F(DynListQueueTest, FrontFromEmptyQueue)
{
  DynListQueue<int> q;
  EXPECT_THROW(q.front(), std::underflow_error);
}

TEST_F(DynListQueueTest, RearPeek)
{
  EXPECT_EQ(queue_with_items.rear(), static_cast<int>(N - 1));

  // Multiple peeks should return same value
  EXPECT_EQ(queue_with_items.rear(), static_cast<int>(N - 1));

  // Size should not change
  EXPECT_EQ(queue_with_items.size(), N);
}

TEST_F(DynListQueueTest, RearFromEmptyQueue)
{
  DynListQueue<int> q;
  EXPECT_THROW(q.rear(), std::underflow_error);
}

TEST_F(DynListQueueTest, FrontModification)
{
  DynListQueue<int> q = {1, 2, 3};
  q.front() = 100;

  EXPECT_EQ(q.get(), 100);
  EXPECT_EQ(q.get(), 2);
}

TEST_F(DynListQueueTest, RearModification)
{
  DynListQueue<int> q = {1, 2, 3};
  q.rear() = 300;

  q.get();  // 1
  q.get();  // 2
  EXPECT_EQ(q.get(), 300);
}

// =============================================================================
// Size and Empty Operations Tests
// =============================================================================

TEST_F(DynListQueueTest, SizeTracking)
{
  DynListQueue<int> q;

  EXPECT_EQ(q.size(), 0u);

  q.put(1);
  EXPECT_EQ(q.size(), 1u);

  q.put(2);
  EXPECT_EQ(q.size(), 2u);

  q.get();
  EXPECT_EQ(q.size(), 1u);

  q.get();
  EXPECT_EQ(q.size(), 0u);
}

TEST_F(DynListQueueTest, IsEmptyCheck)
{
  DynListQueue<int> q;

  EXPECT_TRUE(q.is_empty());

  q.put(1);
  EXPECT_FALSE(q.is_empty());

  q.get();
  EXPECT_TRUE(q.is_empty());
}

TEST_F(DynListQueueTest, EmptyOperation)
{
  EXPECT_EQ(queue_with_items.size(), N);

  queue_with_items.empty();

  EXPECT_TRUE(queue_with_items.is_empty());
  EXPECT_EQ(queue_with_items.size(), 0u);
}

TEST_F(DynListQueueTest, EmptyOnEmptyQueue)
{
  empty_queue.empty();

  EXPECT_TRUE(empty_queue.is_empty());
  EXPECT_EQ(empty_queue.size(), 0u);
}

// =============================================================================
// Swap Operation Tests
// =============================================================================

TEST_F(DynListQueueTest, SwapQueues)
{
  DynListQueue<int> q1 = {1, 2, 3};
  DynListQueue<int> q2 = {10, 20};

  q1.swap(q2);

  EXPECT_EQ(q1.size(), 2u);
  EXPECT_EQ(q2.size(), 3u);

  EXPECT_EQ(q1.front(), 10);
  EXPECT_EQ(q2.front(), 1);
}

TEST_F(DynListQueueTest, SwapWithEmpty)
{
  DynListQueue<int> q1 = {1, 2, 3};
  DynListQueue<int> q2;

  q1.swap(q2);

  EXPECT_TRUE(q1.is_empty());
  EXPECT_EQ(q2.size(), 3u);
  EXPECT_EQ(q2.front(), 1);
}

TEST_F(DynListQueueTest, SwapSelf)
{
  DynListQueue<int> q = {1, 2, 3};

  q.swap(q);

  EXPECT_EQ(q.size(), 3u);
  EXPECT_EQ(q.front(), 1);
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST_F(DynListQueueTest, IteratorBasic)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};
  DynListQueue<int>::Iterator it(q);

  int expected = 1;
  while (it.has_curr())
  {
    EXPECT_EQ(it.get_curr(), expected);
    it.next();
    ++expected;
  }
  EXPECT_EQ(expected, 6);
}

TEST_F(DynListQueueTest, IteratorTraversalOrder)
{
  // Iterator should visit from oldest (front) to youngest (rear)
  DynListQueue<int> q;
  q.put(1);  // oldest
  q.put(2);
  q.put(3);  // youngest

  std::vector<int> visited;
  for (auto it = q.get_it(); it.has_curr(); it.next())
    visited.push_back(it.get_curr());

  ASSERT_EQ(visited.size(), 3u);
  EXPECT_EQ(visited[0], 1);  // oldest first
  EXPECT_EQ(visited[1], 2);
  EXPECT_EQ(visited[2], 3);  // youngest last
}

TEST_F(DynListQueueTest, STLIteratorRangeFor)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};

  int sum = 0;
  for (const auto& item : q)
    sum += item;

  EXPECT_EQ(sum, 15);
}

TEST_F(DynListQueueTest, STLIteratorBeginEnd)
{
  DynListQueue<int> q = {1, 2, 3};

  auto it = q.begin();
  EXPECT_EQ(*it, 1);
  ++it;
  EXPECT_EQ(*it, 2);
  ++it;
  EXPECT_EQ(*it, 3);
  ++it;
  EXPECT_EQ(it, q.end());
}

TEST_F(DynListQueueTest, STLConstIterator)
{
  const DynListQueue<int> q = {1, 2, 3};

  int sum = 0;
  for (const auto& item : q)
    sum += item;

  EXPECT_EQ(sum, 6);
}

TEST_F(DynListQueueTest, EmptyQueueIterator)
{
  DynListQueue<int> q;
  DynListQueue<int>::Iterator it(q);

  EXPECT_FALSE(it.has_curr());
}

// =============================================================================
// Traverse Operation Tests
// =============================================================================

TEST_F(DynListQueueTest, TraverseAll)
{
  int sum = 0;
  bool result = queue_with_items.traverse([&sum](int& item) {
    sum += item;
    return true;
  });

  EXPECT_TRUE(result);
  EXPECT_EQ(sum, static_cast<int>(N * (N - 1) / 2));
}

TEST_F(DynListQueueTest, TraverseEarlyStop)
{
  int count = 0;
  bool result = queue_with_items.traverse([&count](int&) {
    return ++count < 5;  // Stop after 5 elements
  });

  EXPECT_FALSE(result);
  EXPECT_EQ(count, 5);
}

TEST_F(DynListQueueTest, TraverseEmptyQueue)
{
  bool called = false;
  bool result = empty_queue.traverse([&called](int&) {
    called = true;
    return true;
  });

  EXPECT_TRUE(result);
  EXPECT_FALSE(called);
}

TEST_F(DynListQueueTest, TraverseConst)
{
  const DynListQueue<int>& const_ref = queue_with_items;

  int sum = 0;
  const_ref.traverse([&sum](const int& item) {
    sum += item;
    return true;
  });

  EXPECT_EQ(sum, static_cast<int>(N * (N - 1) / 2));
}

// =============================================================================
// Functional Methods Tests (inherited from FunctionalMethods)
// =============================================================================

TEST_F(DynListQueueTest, ForEach)
{
  int sum = 0;
  queue_with_items.for_each([&sum](const int& item) {
    sum += item;
  });

  EXPECT_EQ(sum, static_cast<int>(N * (N - 1) / 2));
}

TEST_F(DynListQueueTest, Maps)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};
  auto doubled = q.maps([](int i) { return i * 2; });

  EXPECT_EQ(doubled.size(), 5u);

  DynList<int> expected = {2, 4, 6, 8, 10};
  EXPECT_EQ(doubled, expected);
}

TEST_F(DynListQueueTest, Filter)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto evens = q.filter([](int i) { return i % 2 == 0; });

  DynList<int> expected = {2, 4, 6, 8, 10};
  EXPECT_EQ(evens, expected);
}

TEST_F(DynListQueueTest, Foldl)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};
  int product = q.foldl(1, [](int acc, int item) { return acc * item; });

  EXPECT_EQ(product, 120);
}

TEST_F(DynListQueueTest, All)
{
  DynListQueue<int> q = {2, 4, 6, 8, 10};

  EXPECT_TRUE(q.all([](int i) { return i % 2 == 0; }));
  EXPECT_FALSE(q.all([](int i) { return i > 5; }));
}

TEST_F(DynListQueueTest, Exists)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};

  EXPECT_TRUE(q.exists([](int i) { return i == 3; }));
  EXPECT_FALSE(q.exists([](int i) { return i == 10; }));
}

TEST_F(DynListQueueTest, Partition)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5, 6};
  auto [evens, odds] = q.partition([](int i) { return i % 2 == 0; });

  EXPECT_EQ(evens.size(), 3u);
  EXPECT_EQ(odds.size(), 3u);
}

TEST_F(DynListQueueTest, Take)
{
  auto first_five = queue_with_items.take(5);

  EXPECT_EQ(first_five.size(), 5u);
  EXPECT_EQ(first_five.get_first(), 0);
  EXPECT_EQ(first_five.get_last(), 4);
}

TEST_F(DynListQueueTest, Drop)
{
  size_t drop_count = N - 5;
  auto last_five = queue_with_items.drop(drop_count);

  EXPECT_EQ(last_five.size(), 5u);
  EXPECT_EQ(last_five.get_first(), static_cast<int>(N - 5));
  EXPECT_EQ(last_five.get_last(), static_cast<int>(N - 1));
}

TEST_F(DynListQueueTest, Rev)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};
  auto reversed = q.rev();

  DynList<int> expected = {5, 4, 3, 2, 1};
  EXPECT_EQ(reversed, expected);
}

TEST_F(DynListQueueTest, Length)
{
  EXPECT_EQ(queue_with_items.length(), N);
  EXPECT_EQ(empty_queue.length(), 0u);
}

// =============================================================================
// Locate Functions Tests (inherited from LocateFunctions)
// =============================================================================

TEST_F(DynListQueueTest, FindPtr)
{
  auto ptr = queue_with_items.find_ptr([](int i) { return i == 50; });

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 50);
}

TEST_F(DynListQueueTest, FindPtrNotFound)
{
  auto ptr = queue_with_items.find_ptr([](int i) { return i == 9999; });

  EXPECT_EQ(ptr, nullptr);
}

TEST_F(DynListQueueTest, FindIndex)
{
  auto idx = queue_with_items.find_index([](int i) { return i == 50; });

  EXPECT_EQ(idx, 50u);
}

TEST_F(DynListQueueTest, FindItem)
{
  auto [found, value] = queue_with_items.find_item([](int i) { return i == 50; });

  EXPECT_TRUE(found);
  EXPECT_EQ(value, 50);
}

TEST_F(DynListQueueTest, Nth)
{
  EXPECT_EQ(queue_with_items.nth(0), 0);
  EXPECT_EQ(queue_with_items.nth(50), 50);
  EXPECT_EQ(queue_with_items.nth(N - 1), static_cast<int>(N - 1));
}

TEST_F(DynListQueueTest, NthOutOfRange)
{
  EXPECT_THROW(queue_with_items.nth(N), std::out_of_range);
  EXPECT_THROW(queue_with_items.nth(N + 100), std::out_of_range);
}

TEST_F(DynListQueueTest, GetIt)
{
  auto it = queue_with_items.get_it();
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 0);
}

TEST_F(DynListQueueTest, GetItPosition)
{
  auto it = queue_with_items.get_it(50);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 50);
}

// =============================================================================
// GenericKeys Tests (inherited from GenericKeys)
// =============================================================================

TEST_F(DynListQueueTest, Keys)
{
  DynListQueue<int> q = {1, 2, 3};
  auto keys = q.keys();

  EXPECT_EQ(keys.size(), 3u);
  EXPECT_EQ(keys.get_first(), 1);
  EXPECT_EQ(keys.get_last(), 3);
}

TEST_F(DynListQueueTest, Items)
{
  DynListQueue<int> q = {1, 2, 3};
  auto items = q.items();

  EXPECT_EQ(items.size(), 3u);
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST_F(DynListQueueTest, TypeAliases)
{
  using SetType = typename DynListQueue<int>::Set_Type;
  using ItemType = typename DynListQueue<int>::Item_Type;

  static_assert(std::is_same_v<SetType, DynListQueue<int>>,
                "Set_Type should be DynListQueue<int>");
  static_assert(std::is_same_v<ItemType, int>,
                "Item_Type should be int");
}

// =============================================================================
// Complex Type Tests
// =============================================================================

TEST(DynListQueueComplexTypes, StringQueue)
{
  DynListQueue<std::string> q;

  q.put("hello");
  q.put("world");
  q.put("!");

  EXPECT_EQ(q.size(), 3u);
  EXPECT_EQ(q.get(), "hello");
  EXPECT_EQ(q.get(), "world");
  EXPECT_EQ(q.get(), "!");
}

TEST(DynListQueueComplexTypes, UniquePtr)
{
  DynListQueue<std::unique_ptr<int>> q;

  q.put(std::make_unique<int>(1));
  q.put(std::make_unique<int>(2));
  q.put(std::make_unique<int>(3));

  EXPECT_EQ(q.size(), 3u);

  auto p1 = q.get();
  EXPECT_EQ(*p1, 1);

  auto p2 = q.get();
  EXPECT_EQ(*p2, 2);

  auto p3 = q.get();
  EXPECT_EQ(*p3, 3);
}

struct NonCopyable
{
  int value;
  NonCopyable(int v) : value(v) {}
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&& other) noexcept : value(other.value) {}
  NonCopyable& operator=(NonCopyable&& other) noexcept
  {
    value = other.value;
    return *this;
  }
};

TEST(DynListQueueComplexTypes, MoveOnlyType)
{
  DynListQueue<NonCopyable> q;

  q.put(NonCopyable(1));
  q.put(NonCopyable(2));

  EXPECT_EQ(q.size(), 2u);

  auto item = q.get();
  EXPECT_EQ(item.value, 1);
}

struct ThrowingType
{
  static int construction_count;
  int value;

  ThrowingType(int v) : value(v)
  {
    if (++construction_count > 100)
      throw std::runtime_error("Too many constructions");
  }

  ThrowingType(const ThrowingType& other) : value(other.value)
  {
    if (++construction_count > 100)
      throw std::runtime_error("Too many constructions");
  }

  ThrowingType(ThrowingType&& other) noexcept : value(other.value) {}

  static void reset() { construction_count = 0; }
};

int ThrowingType::construction_count = 0;

TEST(DynListQueueComplexTypes, ExceptionSafety)
{
  ThrowingType::reset();
  DynListQueue<ThrowingType> q;

  // Should be able to add some elements
  for (int i = 0; i < 50; ++i)
    q.put(ThrowingType(i));

  EXPECT_EQ(q.size(), 50u);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DynListQueueStress, LargeQueue)
{
  constexpr size_t LARGE_N = 100000;
  DynListQueue<int> q;

  for (size_t i = 0; i < LARGE_N; ++i)
    q.put(static_cast<int>(i));

  EXPECT_EQ(q.size(), LARGE_N);
  EXPECT_EQ(q.front(), 0);
  EXPECT_EQ(q.rear(), static_cast<int>(LARGE_N - 1));

  for (size_t i = 0; i < LARGE_N; ++i)
    EXPECT_EQ(q.get(), static_cast<int>(i));

  EXPECT_TRUE(q.is_empty());
}

TEST(DynListQueueStress, InterleavedOperations)
{
  DynListQueue<int> q;

  // Interleave puts and gets
  int put_count = 0;
  int get_count = 0;

  for (int round = 0; round < 1000; ++round)
  {
    // Put 3 elements
    for (int i = 0; i < 3; ++i)
      q.put(put_count++);

    // Get 2 elements
    for (int i = 0; i < 2; ++i)
    {
      EXPECT_EQ(q.get(), get_count);
      get_count++;
    }
  }

  // Queue should have 1000 elements remaining
  EXPECT_EQ(q.size(), 1000u);

  // Drain remaining
  while (!q.is_empty())
  {
    EXPECT_EQ(q.get(), get_count);
    get_count++;
  }
}

TEST(DynListQueueStress, RepeatedEmptyFill)
{
  DynListQueue<int> q;

  for (int round = 0; round < 100; ++round)
  {
    // Fill
    for (int i = 0; i < 100; ++i)
      q.put(i);

    EXPECT_EQ(q.size(), 100u);

    // Empty
    q.empty();

    EXPECT_TRUE(q.is_empty());
    EXPECT_EQ(q.size(), 0u);
  }
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST(DynListQueueEdgeCases, SingleElement)
{
  DynListQueue<int> q;

  q.put(42);

  EXPECT_EQ(q.size(), 1u);
  EXPECT_EQ(q.front(), 42);
  EXPECT_EQ(q.rear(), 42);
  EXPECT_EQ(q.get(), 42);
  EXPECT_TRUE(q.is_empty());
}

TEST(DynListQueueEdgeCases, AlternatingEmptyNonEmpty)
{
  DynListQueue<int> q;

  for (int i = 0; i < 100; ++i)
  {
    EXPECT_TRUE(q.is_empty());

    q.put(i);
    EXPECT_FALSE(q.is_empty());
    EXPECT_EQ(q.front(), i);
    EXPECT_EQ(q.rear(), i);

    int val = q.get();
    EXPECT_EQ(val, i);
    EXPECT_TRUE(q.is_empty());
  }
}

TEST(DynListQueueEdgeCases, ZeroValue)
{
  DynListQueue<int> q;

  q.put(0);
  EXPECT_EQ(q.front(), 0);
  EXPECT_EQ(q.get(), 0);
}

TEST(DynListQueueEdgeCases, NegativeValues)
{
  DynListQueue<int> q;

  for (int i = -100; i <= 100; ++i)
    q.put(i);

  for (int i = -100; i <= 100; ++i)
    EXPECT_EQ(q.get(), i);
}

TEST(DynListQueueEdgeCases, EmptyString)
{
  DynListQueue<std::string> q;

  q.put("");
  q.put("non-empty");
  q.put("");

  EXPECT_EQ(q.get(), "");
  EXPECT_EQ(q.get(), "non-empty");
  EXPECT_EQ(q.get(), "");
}

// =============================================================================
// Noexcept Tests
// =============================================================================

TEST(DynListQueueNoexcept, SwapIsNoexcept)
{
  DynListQueue<int> q1, q2;
  static_assert(noexcept(q1.swap(q2)), "swap should be noexcept");
}

TEST(DynListQueueNoexcept, SizeIsNoexcept)
{
  DynListQueue<int> q;
  static_assert(noexcept(q.size()), "size should be noexcept");
}

TEST(DynListQueueNoexcept, IsEmptyIsNoexcept)
{
  DynListQueue<int> q;
  static_assert(noexcept(q.is_empty()), "is_empty should be noexcept");
}

TEST(DynListQueueNoexcept, EmptyIsNoexcept)
{
  DynListQueue<int> q;
  static_assert(noexcept(q.empty()), "empty should be noexcept");
}

TEST(DynListQueueNoexcept, MoveConstructorIsNoexcept)
{
  static_assert(std::is_nothrow_move_constructible_v<DynListQueue<int>>,
                "Move constructor should be noexcept");
}

TEST(DynListQueueNoexcept, MoveAssignmentIsNoexcept)
{
  static_assert(std::is_nothrow_move_assignable_v<DynListQueue<int>>,
                "Move assignment should be noexcept");
}

// =============================================================================
// Emplace Tests
// =============================================================================

TEST(DynListQueueEmplace, EmplaceBasic)
{
  DynListQueue<std::pair<int, std::string>> q;

  q.emplace(1, "one");
  q.emplace(2, "two");
  q.emplace(3, "three");

  EXPECT_EQ(q.size(), 3u);

  auto p1 = q.get();
  EXPECT_EQ(p1.first, 1);
  EXPECT_EQ(p1.second, "one");
}

TEST(DynListQueueEmplace, EmplaceReturnsReference)
{
  DynListQueue<std::pair<int, int>> q;

  auto& ref = q.emplace(10, 20);
  EXPECT_EQ(ref.first, 10);
  EXPECT_EQ(ref.second, 20);

  // Modifying through reference
  ref.first = 100;
  EXPECT_EQ(q.front().first, 100);
}

TEST(DynListQueueEmplace, EmplaceWithString)
{
  DynListQueue<std::string> q;

  // Emplace constructs string from const char*
  q.emplace("hello");
  q.emplace(5, 'x');  // std::string(5, 'x') = "xxxxx"

  EXPECT_EQ(q.get(), "hello");
  EXPECT_EQ(q.get(), "xxxxx");
}

// =============================================================================
// Pop and Clear Alias Tests
// =============================================================================

TEST(DynListQueueAliases, PopAlias)
{
  DynListQueue<int> q = {1, 2, 3};

  EXPECT_EQ(q.pop(), 1);
  EXPECT_EQ(q.pop(), 2);
  EXPECT_EQ(q.pop(), 3);
  EXPECT_TRUE(q.is_empty());
}

TEST(DynListQueueAliases, PopFromEmpty)
{
  DynListQueue<int> q;
  EXPECT_THROW(q.pop(), std::underflow_error);
}

TEST(DynListQueueAliases, ClearAlias)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};

  EXPECT_EQ(q.size(), 5u);
  q.clear();
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0u);
}

TEST(DynListQueueAliases, ClearOnEmpty)
{
  DynListQueue<int> q;

  q.clear();  // Should not throw
  EXPECT_TRUE(q.is_empty());
}

TEST(DynListQueueAliases, ClearIsNoexcept)
{
  DynListQueue<int> q;
  static_assert(noexcept(q.clear()), "clear should be noexcept");
}

// =============================================================================
// Memory Safety Tests
// =============================================================================

TEST(DynListQueueMemory, DestructorFreesMemory)
{
  // This test verifies that queue destructor properly cleans up
  // Run with valgrind or AddressSanitizer for proper verification

  {
    DynListQueue<int> q;
    for (int i = 0; i < 1000; ++i)
      q.put(i);
    // q destroyed here
  }

  // If we get here without memory errors, destructor works
  SUCCEED();
}

TEST(DynListQueueMemory, EmptyFreesMemory)
{
  DynListQueue<int> q;

  for (int i = 0; i < 1000; ++i)
    q.put(i);

  q.empty();

  EXPECT_TRUE(q.is_empty());

  // Verify queue is reusable after empty
  for (int i = 0; i < 100; ++i)
    q.put(i);

  EXPECT_EQ(q.size(), 100u);
}

// =============================================================================
// Const Correctness Tests
// =============================================================================

TEST(DynListQueueConstCorrectness, ConstFrontReturnsConstReference)
{
  const DynListQueue<int> q = {1, 2, 3};

  const int& ref = q.front();
  EXPECT_EQ(ref, 1);

  // Verify it's a const reference (should not compile if uncommented):
  // ref = 100;  // This should fail to compile
}

TEST(DynListQueueConstCorrectness, NonConstFrontReturnsModifiableReference)
{
  DynListQueue<int> q = {1, 2, 3};

  int& ref = q.front();
  ref = 100;

  EXPECT_EQ(q.front(), 100);
}

TEST(DynListQueueConstCorrectness, ConstRearReturnsConstReference)
{
  const DynListQueue<int> q = {1, 2, 3};

  const int& ref = q.rear();
  EXPECT_EQ(ref, 3);
}

TEST(DynListQueueConstCorrectness, NonConstRearReturnsModifiableReference)
{
  DynListQueue<int> q = {1, 2, 3};

  int& ref = q.rear();
  ref = 300;

  q.get();  // 1
  q.get();  // 2
  EXPECT_EQ(q.get(), 300);
}

// =============================================================================
// Equality Operator Tests (using EqualToMethod mixin)
// =============================================================================

TEST(DynListQueueEquality, EqualQueuesAreEqual)
{
  DynListQueue<int> q1 = {1, 2, 3, 4, 5};
  DynListQueue<int> q2 = {1, 2, 3, 4, 5};

  // Use intermediate variables to avoid -Ofast optimization issues
  bool eq = (q1 == q2);
  bool neq = (q1 != q2);
  EXPECT_TRUE(eq);
  EXPECT_FALSE(neq);
}

TEST(DynListQueueEquality, DifferentSizesAreNotEqual)
{
  DynListQueue<int> q1 = {1, 2, 3};
  DynListQueue<int> q2 = {1, 2, 3, 4};

  bool eq = (q1 == q2);
  bool neq = (q1 != q2);
  EXPECT_FALSE(eq);
  EXPECT_TRUE(neq);
}

TEST(DynListQueueEquality, DifferentElementsAreNotEqual)
{
  DynListQueue<int> q1 = {1, 2, 3};
  DynListQueue<int> q2 = {1, 2, 4};

  bool eq = (q1 == q2);
  bool neq = (q1 != q2);
  EXPECT_FALSE(eq);
  EXPECT_TRUE(neq);
}

TEST(DynListQueueEquality, EmptyQueuesAreEqual)
{
  DynListQueue<int> q1;
  DynListQueue<int> q2;

  bool eq = (q1 == q2);
  bool neq = (q1 != q2);
  EXPECT_TRUE(eq);
  EXPECT_FALSE(neq);
}

TEST(DynListQueueEquality, SelfEquality)
{
  DynListQueue<int> q = {1, 2, 3};

  // Use intermediate variables to avoid compiler optimization issues with -Ofast
  bool eq = (q == q);
  bool neq = (q != q);
  EXPECT_TRUE(eq);
  EXPECT_FALSE(neq);
}

TEST(DynListQueueEquality, EmptyVsNonEmpty)
{
  DynListQueue<int> empty;
  DynListQueue<int> non_empty = {1};

  bool eq = (empty == non_empty);
  bool neq = (empty != non_empty);
  EXPECT_FALSE(eq);
  EXPECT_TRUE(neq);
}

// =============================================================================
// Search Method Tests
// =============================================================================

TEST(DynListQueueSearch, SearchExistingElement)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};

  auto ptr = q.search(3);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 3);
}

TEST(DynListQueueSearch, SearchNonExistingElement)
{
  DynListQueue<int> q = {1, 2, 3, 4, 5};

  auto ptr = q.search(10);
  EXPECT_EQ(ptr, nullptr);
}

TEST(DynListQueueSearch, SearchInEmptyQueue)
{
  DynListQueue<int> q;

  auto ptr = q.search(1);
  EXPECT_EQ(ptr, nullptr);
}

TEST(DynListQueueSearch, SearchFirstElement)
{
  DynListQueue<int> q = {1, 2, 3};

  auto ptr = q.search(1);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 1);
}

TEST(DynListQueueSearch, SearchLastElement)
{
  DynListQueue<int> q = {1, 2, 3};

  auto ptr = q.search(3);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 3);
}

TEST(DynListQueueSearch, ConstSearch)
{
  const DynListQueue<int> q = {1, 2, 3, 4, 5};

  const int* ptr = q.search(3);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 3);
}

TEST(DynListQueueSearch, SearchDuplicates)
{
  DynListQueue<int> q = {1, 2, 2, 2, 3};

  auto ptr = q.search(2);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 2);
  // search returns first match (from front)
}

// =============================================================================
// Main function
// =============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}