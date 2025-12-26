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
 * @file tpl_dynListStack.cc
 * @brief Exhaustive test suite for DynListStack<T>
 *
 * This file contains comprehensive tests for the DynListStack class,
 * covering all public methods, edge cases, exception handling,
 * and iterator functionality.
 */

# include <gtest/gtest.h>

# include <string>
# include <memory>
# include <vector>
# include <stdexcept>

# include <tpl_dynListStack.H>
# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Test Fixture for Basic Operations
// =============================================================================

class DynListStackTest : public ::testing::Test
{
protected:
  DynListStack<int> empty_stack;
  DynListStack<int> stack_with_items;

  static constexpr size_t N = 100;

  void SetUp() override
  {
    for (size_t i = 0; i < N; ++i)
      stack_with_items.push(static_cast<int>(i));
  }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(DynListStackTest, DefaultConstruction)
{
  DynListStack<int> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0u);
}

TEST_F(DynListStackTest, CopyConstruction)
{
  DynListStack<int> copy(stack_with_items);

  EXPECT_EQ(copy.size(), stack_with_items.size());
  EXPECT_EQ(copy.size(), N);

  // Verify independent copies - both should have same LIFO order
  while (!stack_with_items.is_empty())
  {
    EXPECT_EQ(stack_with_items.pop(), copy.pop());
  }
  EXPECT_TRUE(stack_with_items.is_empty());
  EXPECT_TRUE(copy.is_empty());
}

TEST_F(DynListStackTest, MoveConstruction)
{
  DynListStack<int> source;
  for (int i = 0; i < 10; ++i)
    source.push(i);

  size_t original_size = source.size();
  int top_value = source.top();
  DynListStack<int> moved(std::move(source));

  EXPECT_EQ(moved.size(), original_size);
  EXPECT_TRUE(source.is_empty());  // Source should be empty after move
  EXPECT_EQ(moved.top(), top_value);
}

TEST_F(DynListStackTest, InitializerListConstruction)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  EXPECT_EQ(s.size(), 5u);
  // Initializer list inserts in order, so last element is on top
  EXPECT_EQ(s.top(), 5);
}

TEST_F(DynListStackTest, IteratorRangeConstruction)
{
  std::vector<int> vec = {10, 20, 30, 40, 50};
  DynListStack<int> s(vec.begin(), vec.end());

  EXPECT_EQ(s.size(), vec.size());
  // Elements should be in reverse order (last pushed is on top)
  EXPECT_EQ(s.top(), 50);
}

TEST_F(DynListStackTest, DynListConstruction)
{
  DynList<int> list = {100, 200, 300};
  DynListStack<int> s(list);

  EXPECT_EQ(s.size(), list.size());
}

// =============================================================================
// Assignment Tests
// =============================================================================

TEST_F(DynListStackTest, CopyAssignment)
{
  DynListStack<int> s;
  s.push(999);  // Pre-existing content

  s = stack_with_items;

  EXPECT_EQ(s.size(), N);
  EXPECT_EQ(s.top(), static_cast<int>(N - 1));  // Last pushed is on top
}

TEST_F(DynListStackTest, CopyAssignmentSelf)
{
  DynListStack<int> s = {1, 2, 3};
  s = s;  // Self-assignment

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.top(), 3);
}

TEST_F(DynListStackTest, MoveAssignment)
{
  DynListStack<int> source = {1, 2, 3};
  DynListStack<int> target;
  target.push(999);

  target = std::move(source);

  EXPECT_EQ(target.size(), 3u);
  EXPECT_EQ(target.top(), 3);
}

// =============================================================================
// Core Stack Operations Tests
// =============================================================================

TEST_F(DynListStackTest, PushByCopy)
{
  DynListStack<int> s;
  int value = 42;

  int& ref = s.push(value);

  EXPECT_EQ(s.size(), 1u);
  EXPECT_EQ(ref, 42);
  EXPECT_EQ(s.top(), 42);
}

TEST_F(DynListStackTest, PushByMove)
{
  DynListStack<std::string> s;
  std::string value = "hello";

  std::string& ref = s.push(std::move(value));

  EXPECT_EQ(s.size(), 1u);
  EXPECT_EQ(ref, "hello");
}

TEST_F(DynListStackTest, PushMultiple)
{
  DynListStack<int> s;

  s.push(1);
  EXPECT_EQ(s.top(), 1);

  s.push(2);
  EXPECT_EQ(s.top(), 2);

  s.push(3);
  EXPECT_EQ(s.top(), 3);

  EXPECT_EQ(s.size(), 3u);
}

TEST_F(DynListStackTest, PopLIFOOrder)
{
  DynListStack<int> s;
  s.push(1);
  s.push(2);
  s.push(3);

  EXPECT_EQ(s.pop(), 3);  // Last in, first out
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);
  EXPECT_TRUE(s.is_empty());
}

TEST_F(DynListStackTest, PopFromEmptyStack)
{
  DynListStack<int> s;
  EXPECT_THROW(s.pop(), std::underflow_error);
}

TEST_F(DynListStackTest, GetAlias)
{
  DynListStack<int> s = {1, 2, 3};

  EXPECT_EQ(s.get(), 3);
  EXPECT_EQ(s.get(), 2);
  EXPECT_EQ(s.get(), 1);
}

TEST_F(DynListStackTest, TopPeek)
{
  // Top is last pushed (N-1)
  EXPECT_EQ(stack_with_items.top(), static_cast<int>(N - 1));

  // Multiple peeks should return same value
  EXPECT_EQ(stack_with_items.top(), static_cast<int>(N - 1));
  EXPECT_EQ(stack_with_items.top(), static_cast<int>(N - 1));

  // Size should not change
  EXPECT_EQ(stack_with_items.size(), N);
}

TEST_F(DynListStackTest, TopFromEmptyStack)
{
  DynListStack<int> s;
  EXPECT_THROW(s.top(), std::underflow_error);
}

TEST_F(DynListStackTest, PeekAlias)
{
  DynListStack<int> s = {1, 2, 3};

  EXPECT_EQ(s.peek(), 3);
  EXPECT_EQ(s.size(), 3u);  // Size unchanged
}

TEST_F(DynListStackTest, TopModification)
{
  DynListStack<int> s = {1, 2, 3};
  s.top() = 100;

  EXPECT_EQ(s.pop(), 100);
  EXPECT_EQ(s.pop(), 2);
}

// =============================================================================
// Size and Empty Operations Tests
// =============================================================================

TEST_F(DynListStackTest, SizeTracking)
{
  DynListStack<int> s;

  EXPECT_EQ(s.size(), 0u);

  s.push(1);
  EXPECT_EQ(s.size(), 1u);

  s.push(2);
  EXPECT_EQ(s.size(), 2u);

  s.pop();
  EXPECT_EQ(s.size(), 1u);

  s.pop();
  EXPECT_EQ(s.size(), 0u);
}

TEST_F(DynListStackTest, IsEmptyCheck)
{
  DynListStack<int> s;

  EXPECT_TRUE(s.is_empty());

  s.push(1);
  EXPECT_FALSE(s.is_empty());

  s.pop();
  EXPECT_TRUE(s.is_empty());
}

TEST_F(DynListStackTest, EmptyOperation)
{
  EXPECT_EQ(stack_with_items.size(), N);

  stack_with_items.empty();

  EXPECT_TRUE(stack_with_items.is_empty());
  EXPECT_EQ(stack_with_items.size(), 0u);
}

TEST_F(DynListStackTest, EmptyOnEmptyStack)
{
  empty_stack.empty();

  EXPECT_TRUE(empty_stack.is_empty());
  EXPECT_EQ(empty_stack.size(), 0u);
}

TEST_F(DynListStackTest, ClearAlias)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  s.clear();

  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0u);
}

// =============================================================================
// Swap Operation Tests
// =============================================================================

TEST_F(DynListStackTest, SwapStacks)
{
  DynListStack<int> s1 = {1, 2, 3};
  DynListStack<int> s2 = {10, 20};

  s1.swap(s2);

  EXPECT_EQ(s1.size(), 2u);
  EXPECT_EQ(s2.size(), 3u);

  EXPECT_EQ(s1.top(), 20);
  EXPECT_EQ(s2.top(), 3);
}

TEST_F(DynListStackTest, SwapWithEmpty)
{
  DynListStack<int> s1 = {1, 2, 3};
  DynListStack<int> s2;

  s1.swap(s2);

  EXPECT_TRUE(s1.is_empty());
  EXPECT_EQ(s2.size(), 3u);
  EXPECT_EQ(s2.top(), 3);
}

TEST_F(DynListStackTest, SwapSelf)
{
  DynListStack<int> s = {1, 2, 3};

  s.swap(s);

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.top(), 3);
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST_F(DynListStackTest, IteratorBasic)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};
  DynListStack<int>::Iterator it(s);

  // Iterator visits from top to bottom
  int expected = 5;
  while (it.has_curr())
  {
    EXPECT_EQ(it.get_curr(), expected);
    it.next();
    --expected;
  }
  EXPECT_EQ(expected, 0);
}

TEST_F(DynListStackTest, IteratorTraversalOrder)
{
  // Iterator should visit from top to bottom (LIFO order)
  DynListStack<int> s;
  s.push(1);  // bottom
  s.push(2);
  s.push(3);  // top

  std::vector<int> visited;
  for (auto it = s.get_it(); it.has_curr(); it.next())
    visited.push_back(it.get_curr());

  ASSERT_EQ(visited.size(), 3u);
  EXPECT_EQ(visited[0], 3);  // top first
  EXPECT_EQ(visited[1], 2);
  EXPECT_EQ(visited[2], 1);  // bottom last
}

TEST_F(DynListStackTest, STLIteratorRangeFor)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  int sum = 0;
  for (const auto& item : s)
    sum += item;

  EXPECT_EQ(sum, 15);
}

TEST_F(DynListStackTest, STLIteratorBeginEnd)
{
  DynListStack<int> s = {1, 2, 3};

  auto it = s.begin();
  EXPECT_EQ(*it, 3);  // top
  ++it;
  EXPECT_EQ(*it, 2);
  ++it;
  EXPECT_EQ(*it, 1);  // bottom
  ++it;
  EXPECT_EQ(it, s.end());
}

TEST_F(DynListStackTest, STLConstIterator)
{
  const DynListStack<int> s = {1, 2, 3};

  int sum = 0;
  for (const auto& item : s)
    sum += item;

  EXPECT_EQ(sum, 6);
}

TEST_F(DynListStackTest, EmptyStackIterator)
{
  DynListStack<int> s;
  DynListStack<int>::Iterator it(s);

  EXPECT_FALSE(it.has_curr());
}

// =============================================================================
// Traverse Operation Tests
// =============================================================================

TEST_F(DynListStackTest, TraverseAll)
{
  int sum = 0;
  bool result = stack_with_items.traverse([&sum](int& item) {
    sum += item;
    return true;
  });

  EXPECT_TRUE(result);
  EXPECT_EQ(sum, static_cast<int>(N * (N - 1) / 2));
}

TEST_F(DynListStackTest, TraverseEarlyStop)
{
  int count = 0;
  bool result = stack_with_items.traverse([&count](int&) {
    return ++count < 5;  // Stop after 5 elements
  });

  EXPECT_FALSE(result);
  EXPECT_EQ(count, 5);
}

TEST_F(DynListStackTest, TraverseEmptyStack)
{
  bool called = false;
  bool result = empty_stack.traverse([&called](int&) {
    called = true;
    return true;
  });

  EXPECT_TRUE(result);
  EXPECT_FALSE(called);
}

TEST_F(DynListStackTest, TraverseConst)
{
  const DynListStack<int>& const_ref = stack_with_items;

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

TEST_F(DynListStackTest, ForEach)
{
  int sum = 0;
  stack_with_items.for_each([&sum](const int& item) {
    sum += item;
  });

  EXPECT_EQ(sum, static_cast<int>(N * (N - 1) / 2));
}

TEST_F(DynListStackTest, Maps)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};
  auto doubled = s.maps([](int i) { return i * 2; });

  EXPECT_EQ(doubled.size(), 5u);

  // Check the doubled values
  DynList<int> expected = {10, 8, 6, 4, 2};  // top to bottom order
  EXPECT_EQ(doubled, expected);
}

TEST_F(DynListStackTest, Filter)
{
  DynListStack<int> s = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto evens = s.filter([](int i) { return i % 2 == 0; });

  EXPECT_EQ(evens.size(), 5u);
}

TEST_F(DynListStackTest, Foldl)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};
  int product = s.foldl(1, [](int acc, int item) { return acc * item; });

  EXPECT_EQ(product, 120);
}

TEST_F(DynListStackTest, All)
{
  DynListStack<int> s = {2, 4, 6, 8, 10};

  EXPECT_TRUE(s.all([](int i) { return i % 2 == 0; }));
  EXPECT_FALSE(s.all([](int i) { return i > 5; }));
}

TEST_F(DynListStackTest, Exists)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  EXPECT_TRUE(s.exists([](int i) { return i == 3; }));
  EXPECT_FALSE(s.exists([](int i) { return i == 10; }));
}

TEST_F(DynListStackTest, Partition)
{
  DynListStack<int> s = {1, 2, 3, 4, 5, 6};
  auto [evens, odds] = s.partition([](int i) { return i % 2 == 0; });

  EXPECT_EQ(evens.size(), 3u);
  EXPECT_EQ(odds.size(), 3u);
}

TEST_F(DynListStackTest, Take)
{
  auto first_five = stack_with_items.take(5);

  EXPECT_EQ(first_five.size(), 5u);
}

TEST_F(DynListStackTest, Drop)
{
  size_t drop_count = N - 5;
  auto last_five = stack_with_items.drop(drop_count);

  EXPECT_EQ(last_five.size(), 5u);
}

TEST_F(DynListStackTest, Rev)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};
  auto reversed = s.rev();

  EXPECT_EQ(reversed.size(), 5u);
}

TEST_F(DynListStackTest, Length)
{
  EXPECT_EQ(stack_with_items.length(), N);
  EXPECT_EQ(empty_stack.length(), 0u);
}

// =============================================================================
// Locate Functions Tests (inherited from LocateFunctions)
// =============================================================================

TEST_F(DynListStackTest, FindPtr)
{
  auto ptr = stack_with_items.find_ptr([](int i) { return i == 50; });

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 50);
}

TEST_F(DynListStackTest, FindPtrNotFound)
{
  auto ptr = stack_with_items.find_ptr([](int i) { return i == 9999; });

  EXPECT_EQ(ptr, nullptr);
}

TEST_F(DynListStackTest, FindIndex)
{
  // Stack has items in reverse order (N-1 at top, 0 at bottom)
  // So index 0 corresponds to top element (N-1)
  auto idx = stack_with_items.find_index([](int i) { return i == static_cast<int>(N - 1); });

  EXPECT_EQ(idx, 0u);  // Top of stack is index 0
}

TEST_F(DynListStackTest, FindItem)
{
  auto [found, value] = stack_with_items.find_item([](int i) { return i == 50; });

  EXPECT_TRUE(found);
  EXPECT_EQ(value, 50);
}

TEST_F(DynListStackTest, Nth)
{
  // nth(0) is top of stack
  EXPECT_EQ(stack_with_items.nth(0), static_cast<int>(N - 1));
}

TEST_F(DynListStackTest, NthOutOfRange)
{
  EXPECT_THROW(stack_with_items.nth(N), std::out_of_range);
  EXPECT_THROW(stack_with_items.nth(N + 100), std::out_of_range);
}

TEST_F(DynListStackTest, GetIt)
{
  auto it = stack_with_items.get_it();
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), static_cast<int>(N - 1));  // Top of stack
}

// =============================================================================
// GenericKeys Tests (inherited from GenericKeys)
// =============================================================================

TEST_F(DynListStackTest, Keys)
{
  DynListStack<int> s = {1, 2, 3};
  auto keys = s.keys();

  EXPECT_EQ(keys.size(), 3u);
}

TEST_F(DynListStackTest, Items)
{
  DynListStack<int> s = {1, 2, 3};
  auto items = s.items();

  EXPECT_EQ(items.size(), 3u);
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST_F(DynListStackTest, TypeAliases)
{
  using SetType = typename DynListStack<int>::Set_Type;
  using ItemType = typename DynListStack<int>::Item_Type;

  static_assert(std::is_same_v<SetType, DynListStack<int>>,
                "Set_Type should be DynListStack<int>");
  static_assert(std::is_same_v<ItemType, int>,
                "Item_Type should be int");
}

// =============================================================================
// Complex Type Tests
// =============================================================================

TEST(DynListStackComplexTypes, StringStack)
{
  DynListStack<std::string> s;

  s.push("hello");
  s.push("world");
  s.push("!");

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.pop(), "!");
  EXPECT_EQ(s.pop(), "world");
  EXPECT_EQ(s.pop(), "hello");
}

TEST(DynListStackComplexTypes, UniquePtr)
{
  DynListStack<std::unique_ptr<int>> s;

  s.push(std::make_unique<int>(1));
  s.push(std::make_unique<int>(2));
  s.push(std::make_unique<int>(3));

  EXPECT_EQ(s.size(), 3u);

  auto p3 = s.pop();
  EXPECT_EQ(*p3, 3);

  auto p2 = s.pop();
  EXPECT_EQ(*p2, 2);

  auto p1 = s.pop();
  EXPECT_EQ(*p1, 1);
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

TEST(DynListStackComplexTypes, MoveOnlyType)
{
  DynListStack<NonCopyable> s;

  s.push(NonCopyable(1));
  s.push(NonCopyable(2));

  EXPECT_EQ(s.size(), 2u);

  auto item = s.pop();
  EXPECT_EQ(item.value, 2);  // LIFO - last pushed first
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

TEST(DynListStackComplexTypes, ExceptionSafety)
{
  ThrowingType::reset();
  DynListStack<ThrowingType> s;

  for (int i = 0; i < 50; ++i)
    s.push(ThrowingType(i));

  EXPECT_EQ(s.size(), 50u);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DynListStackStress, LargeStack)
{
  constexpr size_t LARGE_N = 100000;
  DynListStack<int> s;

  for (size_t i = 0; i < LARGE_N; ++i)
    s.push(static_cast<int>(i));

  EXPECT_EQ(s.size(), LARGE_N);
  EXPECT_EQ(s.top(), static_cast<int>(LARGE_N - 1));

  // Pop in LIFO order
  for (size_t i = LARGE_N; i > 0; --i)
    EXPECT_EQ(s.pop(), static_cast<int>(i - 1));

  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStackStress, InterleavedOperations)
{
  DynListStack<int> s;

  int push_count = 0;

  for (int round = 0; round < 1000; ++round)
  {
    // Push 3 elements
    for (int i = 0; i < 3; ++i)
      s.push(push_count++);

    // Pop 2 elements
    s.pop();
    s.pop();
  }

  // Stack should have 1000 elements remaining
  EXPECT_EQ(s.size(), 1000u);
}

TEST(DynListStackStress, RepeatedEmptyFill)
{
  DynListStack<int> s;

  for (int round = 0; round < 100; ++round)
  {
    // Fill
    for (int i = 0; i < 100; ++i)
      s.push(i);

    EXPECT_EQ(s.size(), 100u);

    // Empty
    s.clear();

    EXPECT_TRUE(s.is_empty());
    EXPECT_EQ(s.size(), 0u);
  }
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST(DynListStackEdgeCases, SingleElement)
{
  DynListStack<int> s;

  s.push(42);

  EXPECT_EQ(s.size(), 1u);
  EXPECT_EQ(s.top(), 42);
  EXPECT_EQ(s.pop(), 42);
  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStackEdgeCases, AlternatingEmptyNonEmpty)
{
  DynListStack<int> s;

  for (int i = 0; i < 100; ++i)
  {
    EXPECT_TRUE(s.is_empty());

    s.push(i);
    EXPECT_FALSE(s.is_empty());
    EXPECT_EQ(s.top(), i);

    int val = s.pop();
    EXPECT_EQ(val, i);
    EXPECT_TRUE(s.is_empty());
  }
}

TEST(DynListStackEdgeCases, ZeroValue)
{
  DynListStack<int> s;

  s.push(0);
  EXPECT_EQ(s.top(), 0);
  EXPECT_EQ(s.pop(), 0);
}

TEST(DynListStackEdgeCases, NegativeValues)
{
  DynListStack<int> s;

  for (int i = -100; i <= 100; ++i)
    s.push(i);

  // Pop in reverse order (LIFO)
  for (int i = 100; i >= -100; --i)
    EXPECT_EQ(s.pop(), i);
}

TEST(DynListStackEdgeCases, EmptyString)
{
  DynListStack<std::string> s;

  s.push("");
  s.push("non-empty");
  s.push("");

  EXPECT_EQ(s.pop(), "");
  EXPECT_EQ(s.pop(), "non-empty");
  EXPECT_EQ(s.pop(), "");
}

// =============================================================================
// Noexcept Tests
// =============================================================================

TEST(DynListStackNoexcept, SwapIsNoexcept)
{
  DynListStack<int> s1, s2;
  static_assert(noexcept(s1.swap(s2)), "swap should be noexcept");
}

TEST(DynListStackNoexcept, SizeIsNoexcept)
{
  DynListStack<int> s;
  static_assert(noexcept(s.size()), "size should be noexcept");
}

TEST(DynListStackNoexcept, IsEmptyIsNoexcept)
{
  DynListStack<int> s;
  static_assert(noexcept(s.is_empty()), "is_empty should be noexcept");
}

TEST(DynListStackNoexcept, EmptyIsNoexcept)
{
  DynListStack<int> s;
  static_assert(noexcept(s.empty()), "empty should be noexcept");
}

TEST(DynListStackNoexcept, ClearIsNoexcept)
{
  DynListStack<int> s;
  static_assert(noexcept(s.clear()), "clear should be noexcept");
}

TEST(DynListStackNoexcept, MoveConstructorIsNoexcept)
{
  static_assert(std::is_nothrow_move_constructible_v<DynListStack<int>>,
                "Move constructor should be noexcept");
}

TEST(DynListStackNoexcept, MoveAssignmentIsNoexcept)
{
  static_assert(std::is_nothrow_move_assignable_v<DynListStack<int>>,
                "Move assignment should be noexcept");
}

// =============================================================================
// Emplace Tests
// =============================================================================

TEST(DynListStackEmplace, EmplaceBasic)
{
  DynListStack<std::pair<int, std::string>> s;

  s.emplace(1, "one");
  s.emplace(2, "two");
  s.emplace(3, "three");

  EXPECT_EQ(s.size(), 3u);

  auto p3 = s.pop();
  EXPECT_EQ(p3.first, 3);
  EXPECT_EQ(p3.second, "three");
}

TEST(DynListStackEmplace, EmplaceReturnsReference)
{
  DynListStack<std::pair<int, int>> s;

  auto& ref = s.emplace(10, 20);
  EXPECT_EQ(ref.first, 10);
  EXPECT_EQ(ref.second, 20);

  // Modifying through reference
  ref.first = 100;
  EXPECT_EQ(s.top().first, 100);
}

TEST(DynListStackEmplace, EmplaceWithString)
{
  DynListStack<std::string> s;

  s.emplace("hello");
  s.emplace(5, 'x');  // std::string(5, 'x') = "xxxxx"

  EXPECT_EQ(s.pop(), "xxxxx");
  EXPECT_EQ(s.pop(), "hello");
}

// =============================================================================
// Memory Safety Tests
// =============================================================================

TEST(DynListStackMemory, DestructorFreesMemory)
{
  {
    DynListStack<int> s;
    for (int i = 0; i < 1000; ++i)
      s.push(i);
  }

  SUCCEED();
}

TEST(DynListStackMemory, EmptyFreesMemory)
{
  DynListStack<int> s;

  for (int i = 0; i < 1000; ++i)
    s.push(i);

  s.empty();

  EXPECT_TRUE(s.is_empty());

  // Verify stack is reusable after empty
  for (int i = 0; i < 100; ++i)
    s.push(i);

  EXPECT_EQ(s.size(), 100u);
}

// =============================================================================
// Const Correctness Tests
// =============================================================================

TEST(DynListStackConstCorrectness, ConstTopReturnsConstReference)
{
  const DynListStack<int> s = {1, 2, 3};

  const int& ref = s.top();
  EXPECT_EQ(ref, 3);
}

TEST(DynListStackConstCorrectness, NonConstTopReturnsModifiableReference)
{
  DynListStack<int> s = {1, 2, 3};

  int& ref = s.top();
  ref = 100;

  EXPECT_EQ(s.top(), 100);
}

TEST(DynListStackConstCorrectness, ConstPeekReturnsConstReference)
{
  const DynListStack<int> s = {1, 2, 3};

  const int& ref = s.peek();
  EXPECT_EQ(ref, 3);
}

TEST(DynListStackConstCorrectness, NonConstPeekReturnsModifiableReference)
{
  DynListStack<int> s = {1, 2, 3};

  int& ref = s.peek();
  ref = 300;

  EXPECT_EQ(s.pop(), 300);
}

// =============================================================================
// Equality Operator Tests (using EqualToMethod mixin)
// =============================================================================

TEST(DynListStackEquality, EqualStacksAreEqual)
{
  DynListStack<int> s1 = {1, 2, 3, 4, 5};
  DynListStack<int> s2 = {1, 2, 3, 4, 5};

  EXPECT_TRUE(s1 == s2);
  EXPECT_FALSE(s1 != s2);
}

TEST(DynListStackEquality, DifferentSizesAreNotEqual)
{
  DynListStack<int> s1 = {1, 2, 3};
  DynListStack<int> s2 = {1, 2, 3, 4};

  EXPECT_FALSE(s1 == s2);
  EXPECT_TRUE(s1 != s2);
}

TEST(DynListStackEquality, DifferentElementsAreNotEqual)
{
  DynListStack<int> s1 = {1, 2, 3};
  DynListStack<int> s2 = {1, 2, 4};

  EXPECT_FALSE(s1 == s2);
  EXPECT_TRUE(s1 != s2);
}

TEST(DynListStackEquality, EmptyStacksAreEqual)
{
  DynListStack<int> s1;
  DynListStack<int> s2;

  EXPECT_TRUE(s1 == s2);
  EXPECT_FALSE(s1 != s2);
}

TEST(DynListStackEquality, SelfEquality)
{
  DynListStack<int> s = {1, 2, 3};

  EXPECT_TRUE(s == s);
  EXPECT_FALSE(s != s);
}

TEST(DynListStackEquality, EmptyVsNonEmpty)
{
  DynListStack<int> empty;
  DynListStack<int> non_empty = {1};

  EXPECT_FALSE(empty == non_empty);
  EXPECT_TRUE(empty != non_empty);
}

// =============================================================================
// Search Method Tests
// =============================================================================

TEST(DynListStackSearch, SearchExistingElement)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  auto ptr = s.search(3);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 3);
}

TEST(DynListStackSearch, SearchNonExistingElement)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  auto ptr = s.search(10);
  EXPECT_EQ(ptr, nullptr);
}

TEST(DynListStackSearch, SearchInEmptyStack)
{
  DynListStack<int> s;

  auto ptr = s.search(1);
  EXPECT_EQ(ptr, nullptr);
}

TEST(DynListStackSearch, SearchTopElement)
{
  DynListStack<int> s = {1, 2, 3};

  auto ptr = s.search(3);  // 3 is on top
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 3);
}

TEST(DynListStackSearch, SearchBottomElement)
{
  DynListStack<int> s = {1, 2, 3};

  auto ptr = s.search(1);  // 1 is at bottom
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 1);
}

TEST(DynListStackSearch, ConstSearch)
{
  const DynListStack<int> s = {1, 2, 3, 4, 5};

  const int* ptr = s.search(3);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 3);
}

TEST(DynListStackSearch, SearchDuplicates)
{
  DynListStack<int> s = {1, 2, 2, 2, 3};

  auto ptr = s.search(2);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 2);
}

// =============================================================================
// LIFO Behavior Verification
// =============================================================================

TEST(DynListStackLIFO, VerifyLIFOBehavior)
{
  DynListStack<int> s;

  // Push elements 1, 2, 3, 4, 5
  for (int i = 1; i <= 5; ++i)
    s.push(i);

  // Pop should return in reverse order: 5, 4, 3, 2, 1
  EXPECT_EQ(s.pop(), 5);
  EXPECT_EQ(s.pop(), 4);
  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);
}

TEST(DynListStackLIFO, TopAlwaysReturnsLastPushed)
{
  DynListStack<int> s;

  for (int i = 1; i <= 10; ++i)
  {
    s.push(i);
    EXPECT_EQ(s.top(), i);
  }
}

// =============================================================================
// Compatibility Alias Tests (put, get, insert for queue/list-like interfaces)
// =============================================================================

TEST(DynListStackCompatibility, PutAlias)
{
  DynListStack<int> s;

  s.put(1);
  s.put(2);
  s.put(3);

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.top(), 3);  // LIFO - last put is on top
}

TEST(DynListStackCompatibility, GetAlias)
{
  DynListStack<int> s;
  s.put(1);
  s.put(2);
  s.put(3);

  // get() is alias for pop()
  EXPECT_EQ(s.get(), 3);
  EXPECT_EQ(s.get(), 2);
  EXPECT_EQ(s.get(), 1);
}

TEST(DynListStackCompatibility, InsertAlias)
{
  DynListStack<int> s;

  s.insert(10);
  s.insert(20);
  s.insert(30);

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.top(), 30);
}

TEST(DynListStackCompatibility, PutAndGetSymmetry)
{
  // This tests the queue-like interface that graph-traverse.H uses
  DynListStack<int> s;

  s.put(1);
  s.put(2);
  s.put(3);

  // For a stack, get returns in LIFO order
  EXPECT_EQ(s.get(), 3);
  EXPECT_EQ(s.get(), 2);
  EXPECT_EQ(s.get(), 1);
  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStackCompatibility, MixedOperations)
{
  DynListStack<int> s;

  // Mix push/put/insert - all should work the same
  s.push(1);
  s.put(2);
  s.insert(3);

  EXPECT_EQ(s.size(), 3u);

  // Mix pop/get - all should work the same
  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.get(), 2);
  EXPECT_EQ(s.pop(), 1);
}

// =============================================================================
// Main function
// =============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}