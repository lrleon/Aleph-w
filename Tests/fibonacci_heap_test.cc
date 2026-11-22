
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
 * @file fibonacci_heap_test.cc
 * @brief Tests for Fibonacci Heap
 */
# include <gtest/gtest.h>
# include <numeric>
# include <tpl_fibonacci_heap.H>
# include <vector>
# include <algorithm>
# include <random>
# include <string>
# include <set>
# include <chrono>
# include <functional>
# include <map>
# include <memory>
# include <ostream>
# include <sstream>
# include <stdexcept>
# include <type_traits>
# include <utility>
# if defined(__unix__) || defined(__APPLE__)
#   include <pthread.h>
# endif

using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

class FibonacciHeapTest : public ::testing::Test
{
protected:
  Fibonacci_Heap<int> heap;

  void SetUp() override
  {
    // Fresh heap for each test
  }
};

class FibonacciHeapWithDataTest : public ::testing::Test
{
protected:
  Fibonacci_Heap<int> heap;
  std::vector<Fibonacci_Heap<int>::Node *> nodes;

  void SetUp() override
  {
    // Insert some test data
    for (int i = 10; i >= 1; --i)
      nodes.push_back(heap.insert(i * 10)); // 100, 90, 80, ..., 10
  }
};

// =============================================================================
// Basic Construction Tests
// =============================================================================

namespace
{
  // A comparison whose copy, move and swap may throw
  struct Throwing_Copy_Less
  {
    Throwing_Copy_Less() noexcept = default;
    Throwing_Copy_Less(const Throwing_Copy_Less &) noexcept(false) {}
    Throwing_Copy_Less(Throwing_Copy_Less &&) noexcept(false) {}
    Throwing_Copy_Less & operator = (const Throwing_Copy_Less &) noexcept(false)
    {
      return *this;
    }
    Throwing_Copy_Less & operator = (Throwing_Copy_Less &&) noexcept(false)
    {
      return *this;
    }

    bool operator()(const int a, const int b) const noexcept { return a < b; }
  };

  using Fragile_Compare_Heap = Fibonacci_Heap<int, Throwing_Copy_Less>;
} // namespace

// The constructors, the moves and swap were noexcept whatever the
// comparison, which they copy, move or swap
static_assert(std::is_nothrow_default_constructible_v<Fibonacci_Heap<int>>);
static_assert(std::is_nothrow_move_constructible_v<Fibonacci_Heap<int>>);
static_assert(std::is_nothrow_move_assignable_v<Fibonacci_Heap<int>>);
static_assert(std::is_nothrow_swappable_v<Fibonacci_Heap<int>>);
static_assert(not std::is_nothrow_default_constructible_v<Fragile_Compare_Heap>);
static_assert(not noexcept(Fragile_Compare_Heap(std::in_place,
                                                std::declval<const Throwing_Copy_Less &>())));
static_assert(noexcept(Fragile_Compare_Heap(std::in_place)));
static_assert(not std::is_nothrow_move_constructible_v<Fragile_Compare_Heap>);
static_assert(not std::is_nothrow_move_assignable_v<Fragile_Compare_Heap>);
static_assert(not std::is_nothrow_swappable_v<Fragile_Compare_Heap>);

TEST(FibonacciHeapConstruction, DefaultConstructor)
{
  Fibonacci_Heap<int> h;
  EXPECT_TRUE(h.is_empty());
  EXPECT_EQ(h.size(), 0u);
  EXPECT_EQ(h.get_min_node(), nullptr);
}

TEST(FibonacciHeapConstruction, ConstructorWithComparator)
{
  Fibonacci_Heap<int, Aleph::greater<int>> max_heap;
  max_heap.insert(5);
  max_heap.insert(10);
  max_heap.insert(3);
  EXPECT_EQ(max_heap.get_min(), 10); // Max heap, so "min" is actually max
}

TEST(FibonacciHeapConstruction, MoveConstructor)
{
  Fibonacci_Heap<int> h1;
  h1.insert(5);
  h1.insert(3);
  h1.insert(7);

  Fibonacci_Heap<int> h2(std::move(h1));

  EXPECT_TRUE(h1.is_empty());
  EXPECT_EQ(h1.size(), 0u);

  EXPECT_FALSE(h2.is_empty());
  EXPECT_EQ(h2.size(), 3u);
  EXPECT_EQ(h2.get_min(), 3);
}

TEST(FibonacciHeapConstruction, MoveAssignment)
{
  Fibonacci_Heap<int> h1;
  h1.insert(5);
  h1.insert(3);

  Fibonacci_Heap<int> h2;
  h2.insert(100);

  h2 = std::move(h1);

  EXPECT_TRUE(h1.is_empty());
  EXPECT_EQ(h2.size(), 2u);
  EXPECT_EQ(h2.get_min(), 3);
}

TEST(FibonacciHeapConstruction, MoveAssignmentRoundtrip)
{
  Fibonacci_Heap<int> h;
  h.insert(5);
  h.insert(3);

  const auto original_size = h.size();
  Fibonacci_Heap<int> tmp(std::move(h));
  h = std::move(tmp);

  EXPECT_EQ(h.size(), original_size);
  EXPECT_EQ(h.get_min(), 3);
}

namespace
{
  // A comparison whose move assignment throws while armed
  struct Armed_Move_Less
  {
    static inline bool armed = false;

    Armed_Move_Less() noexcept = default;
    Armed_Move_Less(const Armed_Move_Less &) noexcept = default;
    Armed_Move_Less & operator = (const Armed_Move_Less &) noexcept = default;
    Armed_Move_Less & operator = (Armed_Move_Less &&) noexcept(false)
    {
      if (armed)
        throw std::runtime_error("Armed_Move_Less");
      return *this;
    }

    bool operator()(const int a, const int b) const noexcept { return a < b; }
  };

  // A comparison, ascending or descending by its order, whose moves throw
  // and whose own swap does not
  struct Own_Swap_Order
  {
    bool descending = false;

    explicit Own_Swap_Order(const bool d = false) noexcept : descending(d) {}
    Own_Swap_Order(const Own_Swap_Order &) noexcept = default;
    Own_Swap_Order(Own_Swap_Order &&) noexcept(false)
    {
      throw std::runtime_error("Own_Swap_Order");
    }
    Own_Swap_Order & operator = (const Own_Swap_Order &) noexcept = default;
    Own_Swap_Order & operator = (Own_Swap_Order &&) noexcept(false)
    {
      throw std::runtime_error("Own_Swap_Order");
    }

    friend void swap(Own_Swap_Order & a, Own_Swap_Order & b) noexcept
    {
      std::swap(a.descending, b.descending);
    }

    bool operator()(const int a, const int b) const noexcept
    {
      return descending ? b < a : a < b;
    }
  };
} // namespace

// The move assignment took the nodes of the other heap before it moved the
// comparison: when the move threw, both heaps owned the nodes, and each
// freed them
TEST(FibonacciHeapConstruction, MoveAssignmentWhoseComparisonThrows)
{
  Fibonacci_Heap<int, Armed_Move_Less> to, from;
  to.insert(7);
  for (int i = 0; i < 5; ++i)
    from.insert(i);

  Armed_Move_Less::armed = true;
  EXPECT_THROW(to = std::move(from), std::runtime_error);
  Armed_Move_Less::armed = false;

  EXPECT_TRUE(to.is_empty());
  ASSERT_EQ(from.size(), 5u);
  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(from.extract_min(), i);
}

// swap() said noexcept for the swap of the comparisons that ADL finds, but
// called std::swap, which moves: with the swap of Own_Swap_Order, which
// does not throw, it ended the program
static_assert(std::is_nothrow_swappable_v<Fibonacci_Heap<int, Own_Swap_Order>>);

TEST(FibonacciHeapConstruction, SwapUsesTheSwapOfTheComparison)
{
  Fibonacci_Heap<int, Own_Swap_Order> up(std::in_place, false);
  Fibonacci_Heap<int, Own_Swap_Order> down(std::in_place, true);
  for (int i = 0; i < 4; ++i)
    {
      up.insert(i);
      down.insert(10 + i);
    }

  EXPECT_NO_THROW(up.swap(down));
  EXPECT_TRUE(up.get_compare().descending);
  EXPECT_FALSE(down.get_compare().descending);
  EXPECT_EQ(up.extract_min(), 13);
  EXPECT_EQ(down.extract_min(), 0);

  EXPECT_NO_THROW(swap(up, down));
  EXPECT_EQ(up.extract_min(), 1);
  EXPECT_EQ(down.extract_min(), 12);
}

// =============================================================================
// Insert Tests
// =============================================================================

TEST_F(FibonacciHeapTest, InsertSingleElement)
{
  auto node = heap.insert(42);

  EXPECT_FALSE(heap.is_empty());
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.get_min(), 42);
  EXPECT_EQ(node->data, 42);
}

TEST_F(FibonacciHeapTest, InsertMultipleElements)
{
  heap.insert(10);
  heap.insert(5);
  heap.insert(15);
  heap.insert(3);
  heap.insert(8);

  EXPECT_EQ(heap.size(), 5u);
  EXPECT_EQ(heap.get_min(), 3);
}

TEST_F(FibonacciHeapTest, InsertDescendingOrder)
{
  for (int i = 100; i >= 1; --i)
    heap.insert(i);

  EXPECT_EQ(heap.size(), 100u);
  EXPECT_EQ(heap.get_min(), 1);
}

TEST_F(FibonacciHeapTest, InsertAscendingOrder)
{
  for (int i = 1; i <= 100; ++i)
    heap.insert(i);

  EXPECT_EQ(heap.size(), 100u);
  EXPECT_EQ(heap.get_min(), 1);
}

TEST_F(FibonacciHeapTest, InsertDuplicates)
{
  for (int i = 0; i < 10; ++i)
    heap.insert(42);

  EXPECT_EQ(heap.size(), 10u);
  EXPECT_EQ(heap.get_min(), 42);

  // All extractions should return 42
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(heap.extract_min(), 42);

  EXPECT_TRUE(heap.is_empty());
}

TEST_F(FibonacciHeapTest, InsertWithMoveSemantics)
{
  std::string s = "hello world";
  Fibonacci_Heap<std::string> string_heap;

  auto node = string_heap.insert(std::move(s));

  EXPECT_EQ(node->data, "hello world");
  EXPECT_TRUE(s.empty() || s != "hello world");  // s should be moved-from
}

TEST_F(FibonacciHeapTest, EmplaceConstruction)
{
  Fibonacci_Heap<std::pair<int, std::string>> pair_heap;

  auto node = pair_heap.emplace(42, "answer");

  EXPECT_EQ(node->data.first, 42);
  EXPECT_EQ(node->data.second, "answer");
}

// =============================================================================
// Get Min Tests
// =============================================================================

TEST_F(FibonacciHeapTest, GetMinOnEmptyHeapThrows)
{
  EXPECT_THROW(heap.get_min(), std::underflow_error);
}

TEST_F(FibonacciHeapTest, GetMinNodeOnEmptyHeap)
{
  EXPECT_EQ(heap.get_min_node(), nullptr);
}

TEST_F(FibonacciHeapTest, GetMinAfterInserts)
{
  heap.insert(50);
  EXPECT_EQ(heap.get_min(), 50);

  heap.insert(30);
  EXPECT_EQ(heap.get_min(), 30);

  heap.insert(40);
  EXPECT_EQ(heap.get_min(), 30);

  heap.insert(10);
  EXPECT_EQ(heap.get_min(), 10);

  heap.insert(20);
  EXPECT_EQ(heap.get_min(), 10);
}

// =============================================================================
// Extract Min Tests
// =============================================================================

TEST_F(FibonacciHeapTest, ExtractMinOnEmptyHeapThrows)
{
  EXPECT_THROW(heap.extract_min(), std::underflow_error);
}

TEST_F(FibonacciHeapTest, ExtractMinSingleElement)
{
  heap.insert(42);
  int val = heap.extract_min();

  EXPECT_EQ(val, 42);
  EXPECT_TRUE(heap.is_empty());
}

TEST_F(FibonacciHeapTest, ExtractMinMultipleElements)
{
  heap.insert(30);
  heap.insert(10);
  heap.insert(20);

  EXPECT_EQ(heap.extract_min(), 10);
  EXPECT_EQ(heap.extract_min(), 20);
  EXPECT_EQ(heap.extract_min(), 30);
  EXPECT_TRUE(heap.is_empty());
}

TEST_F(FibonacciHeapTest, ExtractMinMaintainsSortedOrder)
{
  std::vector<int> input = {50, 20, 80, 10, 90, 30, 70, 40, 60, 100};
  for (int v : input)
    heap.insert(v);

  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.extract_min());

  std::vector<int> expected = input;
  std::sort(expected.begin(), expected.end());

  EXPECT_EQ(extracted, expected);
}

TEST_F(FibonacciHeapTest, ExtractMinWithDuplicates)
{
  std::vector<int> input = {5, 3, 5, 1, 3, 5, 1, 3};
  for (int v : input)
    heap.insert(v);

  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.extract_min());

  std::sort(input.begin(), input.end());
  EXPECT_EQ(extracted, input);
}

// =============================================================================
// Decrease Key Tests
// =============================================================================

TEST_F(FibonacciHeapWithDataTest, DecreaseKeyToNewMinimum)
{
  // nodes[9] has value 10 (the current minimum)
  // nodes[0] has value 100
  heap.decrease_key(nodes[0], 5);

  EXPECT_EQ(heap.get_min(), 5);
  EXPECT_EQ(nodes[0]->data, 5);
}

TEST_F(FibonacciHeapWithDataTest, DecreaseKeyNotAffectingMinimum)
{
  // nodes[1] has value 90
  heap.decrease_key(nodes[1], 50);

  EXPECT_EQ(heap.get_min(), 10);  // Still 10
  EXPECT_EQ(nodes[1]->data, 50);
}

TEST_F(FibonacciHeapWithDataTest, DecreaseKeyToSameValue)
{
  // Decreasing to same value should work
  heap.decrease_key(nodes[0], 100);
  EXPECT_EQ(nodes[0]->data, 100);
}

TEST_F(FibonacciHeapTest, DecreaseKeyWithInvalidIncreaseThrows)
{
  auto node = heap.insert(50);
  EXPECT_THROW(heap.decrease_key(node, 100), std::domain_error);
}

TEST_F(FibonacciHeapTest, DecreaseKeyWithNullptrThrows)
{
  EXPECT_THROW(heap.decrease_key(nullptr, 10), std::invalid_argument);
}

TEST_F(FibonacciHeapTest, DecreaseKeyTriggersCut)
{
  // Build a heap that will have internal structure
  for (int i = 1; i <= 20; ++i)
    heap.insert(i);

  // Extract some to build tree structure
  for (int i = 0; i < 5; ++i)
    heap.extract_min();

  // Now decrease a key that should trigger cuts
  auto node = heap.insert(100);
  heap.decrease_key(node, 1);  // Should become new minimum

  EXPECT_EQ(heap.get_min(), 1);
}

TEST_F(FibonacciHeapTest, DecreaseKeyTriggersCascadingCuts)
{
  // Insert elements and extract to create tree structure
  std::vector<Fibonacci_Heap<int>::Node *> nodes;
  for (int i = 1; i <= 100; ++i)
    nodes.push_back(heap.insert(i * 10));

  // Extract min multiple times to consolidate
  for (int i = 0; i < 20; ++i)
    heap.extract_min();

  // Find remaining nodes and decrease their keys
  // This should trigger cascading cuts
  for (size_t i = 50; i < 60 && i < nodes.size(); ++i)
    {
      if (nodes[i]->data > 5)
        heap.decrease_key(nodes[i], static_cast<int>(i));
    }

  // Verify heap property is maintained
  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.extract_min());

  for (size_t i = 1; i < extracted.size(); ++i)
    EXPECT_LE(extracted[i - 1], extracted[i]);
}

TEST_F(FibonacciHeapTest, DecreaseKeyMoveSemantics)
{
  Fibonacci_Heap<std::string> string_heap;
  auto node = string_heap.insert("zzzzz");

  std::string new_val = "aaaaa";
  string_heap.decrease_key(node, std::move(new_val));

  EXPECT_EQ(node->data, "aaaaa");
}

// =============================================================================
// Update Key Tests
// =============================================================================

TEST_F(FibonacciHeapTest, UpdateKeyDecrease)
{
  auto node = heap.insert(50);
  auto result = heap.update_key(node, 30);

  EXPECT_EQ(result, node);
  EXPECT_EQ(node->data, 30);
}

TEST_F(FibonacciHeapTest, UpdateKeyIncrease)
{
  heap.insert(30);
  auto node = heap.insert(50);
  heap.insert(70);

  auto result = heap.update_key(node, 80);

  // The result should contain the new value and heap should be consistent
  // Note: The returned pointer may or may not be the same as node due to
  // memory allocator reuse, so we don't check pointer equality
  EXPECT_EQ(result->data, 80);
  EXPECT_EQ(heap.size(), 3u);

  // Verify heap order is maintained: 30, 70, 80
  EXPECT_EQ(heap.extract_min(), 30);
  EXPECT_EQ(heap.extract_min(), 70);
  EXPECT_EQ(heap.extract_min(), 80);
}

TEST_F(FibonacciHeapTest, UpdateKeySameValue)
{
  auto node = heap.insert(50);
  auto result = heap.update_key(node, 50);

  EXPECT_EQ(result, node);
  EXPECT_EQ(node->data, 50);
}

TEST_F(FibonacciHeapTest, UpdateKeyNullptrThrows)
{
  EXPECT_THROW(heap.update_key(nullptr, 10), std::invalid_argument);
}

// =============================================================================
// Delete Node Tests
// =============================================================================

TEST_F(FibonacciHeapTest, DeleteNodeSingleElement)
{
  auto node = heap.insert(42);
  heap.delete_node(node);

  EXPECT_TRUE(heap.is_empty());
}

TEST_F(FibonacciHeapTest, DeleteNodeMinimum)
{
  heap.insert(30);
  auto min_node = heap.insert(10);
  heap.insert(20);

  heap.delete_node(min_node);

  EXPECT_EQ(heap.size(), 2u);
  EXPECT_EQ(heap.get_min(), 20);
}

TEST_F(FibonacciHeapTest, DeleteNodeNonMinimum)
{
  heap.insert(10);
  auto middle = heap.insert(20);
  heap.insert(30);

  heap.delete_node(middle);

  EXPECT_EQ(heap.size(), 2u);
  EXPECT_EQ(heap.get_min(), 10);
  EXPECT_EQ(heap.extract_min(), 10);
  EXPECT_EQ(heap.extract_min(), 30);
}

TEST_F(FibonacciHeapTest, DeleteNodeNullptrThrows)
{
  EXPECT_THROW(heap.delete_node(nullptr), std::invalid_argument);
}

TEST_F(FibonacciHeapTest, DeleteNodeFromDeepTree)
{
  // Create a complex tree structure
  std::vector<Fibonacci_Heap<int>::Node *> nodes;
  for (int i = 1; i <= 50; ++i)
    nodes.push_back(heap.insert(i));

  // Extract to consolidate
  for (int i = 0; i < 10; ++i)
    heap.extract_min();

  // Delete some internal nodes
  heap.delete_node(nodes[30]);
  heap.delete_node(nodes[40]);
  heap.delete_node(nodes[25]);

  // Verify remaining elements
  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.extract_min());

  // Should have 50 - 10 (extracted) - 3 (deleted) = 37 elements
  EXPECT_EQ(extracted.size(), 37u);

  // Should be sorted
  for (size_t i = 1; i < extracted.size(); ++i)
    EXPECT_LE(extracted[i - 1], extracted[i]);
}

// Test for bug: deleting a solitary root with children must consolidate
// to find the true minimum among the children
TEST_F(FibonacciHeapTest, DeleteSolitaryRootWithChildren)
{
  // Insert values: 10 will become root after extract, with children
  (void)heap.insert(10);
  (void)heap.insert(5);
  (void)heap.insert(20);
  (void)heap.insert(15);
  (void)heap.insert(3);

  // Extract minimum (3) - this triggers consolidation
  // After this, the tree structure will have some nodes as children
  EXPECT_EQ(heap.extract_min(), 3);

  // Now extract 5 - more consolidation
  EXPECT_EQ(heap.extract_min(), 5);

  // At this point we have 10, 15, 20 remaining
  // The structure may have 10 as root with children

  // Insert a new minimum so we can control the structure
  (void)heap.insert(1);

  // Extract 1 to consolidate: 10, 15, 20 remain, possibly with 10 as root
  EXPECT_EQ(heap.extract_min(), 1);

  // Now delete the current minimum (should be 10)
  // This tests deleting a node that may be alone with children
  auto min_node = heap.get_min_node();
  heap.delete_node(min_node);

  // After deletion, heap should correctly identify the new minimum
  EXPECT_EQ(heap.size(), 2u);

  // The new minimum should be correctly found (not just any child)
  int new_min = heap.get_min();
  EXPECT_TRUE(new_min == 15 || new_min == 20);

  // Verify we can extract both remaining elements in order
  int first = heap.extract_min();
  int second = heap.extract_min();
  EXPECT_LT(first, second);
  EXPECT_TRUE(heap.is_empty());
}

// More direct test: manually create scenario with solitary root + children
TEST_F(FibonacciHeapTest, DeleteSolitaryRootWithChildrenDirect)
{
  // Create heap with nodes that will form a single tree
  heap.insert(100);  // Will be root
  heap.insert(50);
  heap.insert(200);
  heap.insert(25);
  heap.insert(10);   // Will be minimum

  // Extract to build tree structure
  EXPECT_EQ(heap.extract_min(), 10);
  EXPECT_EQ(heap.extract_min(), 25);

  // Now 50 is minimum, 100 and 200 may be children or siblings
  // Keep extracting until we have a small tree
  EXPECT_EQ(heap.extract_min(), 50);

  // Only 100 and 200 remain
  EXPECT_EQ(heap.size(), 2u);

  // Delete the minimum (100)
  heap.delete_node(heap.get_min_node());

  // Only 200 should remain
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.get_min(), 200);
  EXPECT_EQ(heap.extract_min(), 200);
  EXPECT_TRUE(heap.is_empty());
}

TEST_F(FibonacciHeapTest, DeleteAllNodesOneByOne)
{
  std::vector<Fibonacci_Heap<int>::Node *> nodes;
  for (int i = 1; i <= 20; ++i)
    nodes.push_back(heap.insert(i));

  // Delete in random order
  std::vector<size_t> indices(20);
  std::iota(indices.begin(), indices.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(indices.begin(), indices.end(), g);

  for (size_t idx : indices)
    {
      heap.delete_node(nodes[idx]);
    }

  EXPECT_TRUE(heap.is_empty());
}

// =============================================================================
// Merge Tests
// =============================================================================

TEST_F(FibonacciHeapTest, MergeEmptyHeaps)
{
  Fibonacci_Heap<int> h1, h2;
  h1.merge(h2);

  EXPECT_TRUE(h1.is_empty());
  EXPECT_TRUE(h2.is_empty());
}

TEST_F(FibonacciHeapTest, MergeIntoEmptyHeap)
{
  Fibonacci_Heap<int> h1, h2;
  h2.insert(5);
  h2.insert(3);

  h1.merge(h2);

  EXPECT_EQ(h1.size(), 2u);
  EXPECT_EQ(h1.get_min(), 3);
  EXPECT_TRUE(h2.is_empty());
}

TEST_F(FibonacciHeapTest, MergeEmptyIntoNonEmpty)
{
  Fibonacci_Heap<int> h1, h2;
  h1.insert(5);
  h1.insert(3);

  h1.merge(h2);

  EXPECT_EQ(h1.size(), 2u);
  EXPECT_EQ(h1.get_min(), 3);
}

TEST_F(FibonacciHeapTest, MergeTwoNonEmptyHeaps)
{
  Fibonacci_Heap<int> h1, h2;

  h1.insert(10);
  h1.insert(5);
  h1.insert(15);

  h2.insert(3);
  h2.insert(8);
  h2.insert(12);

  h1.merge(h2);

  EXPECT_EQ(h1.size(), 6u);
  EXPECT_EQ(h1.get_min(), 3);
  EXPECT_TRUE(h2.is_empty());

  // Verify all elements
  std::vector<int> extracted;
  while (!h1.is_empty())
    extracted.push_back(h1.extract_min());

  std::vector<int> expected = {3, 5, 8, 10, 12, 15};
  EXPECT_EQ(extracted, expected);
}

TEST_F(FibonacciHeapTest, MergeWithRvalue)
{
  Fibonacci_Heap<int> h1;
  h1.insert(10);

  Fibonacci_Heap<int> h2;
  h2.insert(5);

  h1.merge(std::move(h2));

  EXPECT_EQ(h1.size(), 2u);
  EXPECT_EQ(h1.get_min(), 5);
}

TEST_F(FibonacciHeapTest, MergeLargeHeaps)
{
  Fibonacci_Heap<int> h1, h2;

  for (int i = 0; i < 1000; i += 2)
    h1.insert(i);

  for (int i = 1; i < 1000; i += 2)
    h2.insert(i);

  h1.merge(h2);

  EXPECT_EQ(h1.size(), 1000u);

  std::vector<int> extracted;
  while (!h1.is_empty())
    extracted.push_back(h1.extract_min());

  for (int i = 0; i < 1000; ++i)
    EXPECT_EQ(extracted[i], i);
}

// =============================================================================
// Swap Tests
// =============================================================================

TEST_F(FibonacciHeapTest, SwapHeaps)
{
  Fibonacci_Heap<int> h1, h2;

  h1.insert(10);
  h1.insert(5);

  h2.insert(100);
  h2.insert(50);
  h2.insert(75);

  h1.swap(h2);

  EXPECT_EQ(h1.size(), 3u);
  EXPECT_EQ(h1.get_min(), 50);

  EXPECT_EQ(h2.size(), 2u);
  EXPECT_EQ(h2.get_min(), 5);
}

TEST_F(FibonacciHeapTest, SwapWithEmptyHeap)
{
  Fibonacci_Heap<int> h1, h2;
  h1.insert(10);

  h1.swap(h2);

  EXPECT_TRUE(h1.is_empty());
  EXPECT_EQ(h2.size(), 1u);
  EXPECT_EQ(h2.get_min(), 10);
}

TEST_F(FibonacciHeapTest, SwapFreeFunction)
{
  Fibonacci_Heap<int> h1, h2;
  h1.insert(5);
  h2.insert(10);

  Aleph::swap(h1, h2);

  EXPECT_EQ(h1.get_min(), 10);
  EXPECT_EQ(h2.get_min(), 5);
}

// =============================================================================
// Clear Tests
// =============================================================================

TEST_F(FibonacciHeapTest, ClearEmptyHeap)
{
  heap.clear();
  EXPECT_TRUE(heap.is_empty());
}

TEST_F(FibonacciHeapTest, ClearNonEmptyHeap)
{
  for (int i = 0; i < 100; ++i)
    heap.insert(i);

  heap.clear();

  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0u);
}

TEST_F(FibonacciHeapTest, ClearAndReuse)
{
  heap.insert(10);
  heap.insert(5);
  heap.clear();

  heap.insert(20);
  heap.insert(15);

  EXPECT_EQ(heap.size(), 2u);
  EXPECT_EQ(heap.get_min(), 15);
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST(FibonacciHeapTypeAliases, ValueType)
{
  static_assert(std::is_same_v<Fibonacci_Heap<int>::value_type, int>);
  static_assert(std::is_same_v<Fibonacci_Heap<std::string>::value_type, std::string>);
}

TEST(FibonacciHeapTypeAliases, HandleType)
{
  static_assert(std::is_same_v<Fibonacci_Heap<int>::handle_type, Fibonacci_Heap<int>::Node *>);
}

// =============================================================================
// Max Heap Tests
// =============================================================================

TEST(FibonacciMaxHeap, BasicOperations)
{
  Fibonacci_Heap<int, Aleph::greater<int>> max_heap;

  max_heap.insert(10);
  max_heap.insert(30);
  max_heap.insert(20);

  EXPECT_EQ(max_heap.get_min(), 30);  // "min" is actually max
  EXPECT_EQ(max_heap.extract_min(), 30);
  EXPECT_EQ(max_heap.extract_min(), 20);
  EXPECT_EQ(max_heap.extract_min(), 10);
}

TEST(FibonacciMaxHeap, DecreaseKey)
{
  Fibonacci_Heap<int, Aleph::greater<int>> max_heap;

  auto n1 = max_heap.insert(10);
  max_heap.insert(30);
  max_heap.insert(20);

  // In max heap, "decrease" means increase the value
  max_heap.decrease_key(n1, 50);

  EXPECT_EQ(max_heap.get_min(), 50);
}

// =============================================================================
// Custom Type Tests
// =============================================================================

struct Point
{
  double x, y;
  double distance() const { return x * x + y * y; }

  bool operator<(const Point & other) const
  {
    return distance() < other.distance();
  }
};

TEST(FibonacciHeapCustomType, PointHeap)
{
  Fibonacci_Heap<Point> point_heap;

  point_heap.insert({3.0, 4.0});   // distance = 25
  point_heap.insert({1.0, 1.0});   // distance = 2
  point_heap.insert({2.0, 2.0});   // distance = 8

  EXPECT_DOUBLE_EQ(point_heap.get_min().distance(), 2.0);
  EXPECT_DOUBLE_EQ(point_heap.extract_min().distance(), 2.0);
  EXPECT_DOUBLE_EQ(point_heap.extract_min().distance(), 8.0);
  EXPECT_DOUBLE_EQ(point_heap.extract_min().distance(), 25.0);
}

TEST(FibonacciHeapCustomType, PairHeap)
{
  // Heap of (priority, value) pairs
  Fibonacci_Heap<std::pair<int, std::string>> pair_heap;

  pair_heap.insert({3, "three"});
  pair_heap.insert({1, "one"});
  pair_heap.insert({2, "two"});

  EXPECT_EQ(pair_heap.extract_min().second, "one");
  EXPECT_EQ(pair_heap.extract_min().second, "two");
  EXPECT_EQ(pair_heap.extract_min().second, "three");
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(FibonacciHeapStress, LargeNumberOfInserts)
{
  Fibonacci_Heap<int> heap;
  constexpr int N = 100000;

  for (int i = N; i >= 1; --i)
    (void)heap.insert(i);

  EXPECT_EQ(heap.size(), static_cast<size_t>(N));
  EXPECT_EQ(heap.get_min(), 1);
}

TEST(FibonacciHeapStress, LargeNumberOfExtractMin)
{
  Fibonacci_Heap<int> heap;
  constexpr int N = 10000;

  for (int i = N; i >= 1; --i)
    (void)heap.insert(i);

  for (int i = 1; i <= N; ++i)
    EXPECT_EQ(heap.extract_min(), i);

  EXPECT_TRUE(heap.is_empty());
}

TEST(FibonacciHeapStress, InterleavedOperations)
{
  Fibonacci_Heap<int> heap;
  std::multiset<int> reference;  // For verification
  std::random_device rd;
  std::mt19937 gen(42);  // Fixed seed for reproducibility
  std::uniform_int_distribution<> dis(1, 10000);

  constexpr int N = 10000;

  for (int i = 0; i < N; ++i)
    {
      int op = dis(gen) % 3;

      if (op == 0 || reference.empty())
        {
          // Insert
          int val = dis(gen);
          (void)heap.insert(val);
          reference.insert(val);
        }
      else if (op == 1)
        {
          // Extract min
          int heap_min = heap.extract_min();
          int ref_min = *reference.begin();
          reference.erase(reference.begin());
          EXPECT_EQ(heap_min, ref_min);
        }
      else
        {
          // Get min
          EXPECT_EQ(heap.get_min(), *reference.begin());
        }
    }

  // Drain remaining
  while (!heap.is_empty())
    {
      int heap_min = heap.extract_min();
      int ref_min = *reference.begin();
      reference.erase(reference.begin());
      EXPECT_EQ(heap_min, ref_min);
    }
}

TEST(FibonacciHeapStress, ManyDecreaseKeys)
{
  Fibonacci_Heap<int> heap;
  std::vector<Fibonacci_Heap<int>::Node *> nodes;
  constexpr int N = 5000;

  for (int i = 0; i < N; ++i)
    nodes.push_back(heap.insert(i + N));  // Insert N, N+1, ..., 2N-1

  // Extract some to create tree structure
  for (int i = 0; i < N / 4; ++i)
    heap.extract_min();

  // Decrease remaining keys
  int counter = 0;
  for (size_t i = N / 4; i < N; ++i)
    {
      if (nodes[i]->data > counter)
        {
          heap.decrease_key(nodes[i], counter);
          counter++;
        }
    }

  // Verify heap property
  int prev = heap.extract_min();
  while (!heap.is_empty())
    {
      int curr = heap.extract_min();
      EXPECT_LE(prev, curr);
      prev = curr;
    }
}

TEST(FibonacciHeapStress, ManyDeleteNodes)
{
  Fibonacci_Heap<int> heap;
  std::vector<Fibonacci_Heap<int>::Node *> nodes;
  constexpr int N = 1000;

  for (int i = 0; i < N; ++i)
    nodes.push_back(heap.insert(i));

  // Delete every other node
  for (int i = 0; i < N; i += 2)
    heap.delete_node(nodes[i]);

  EXPECT_EQ(heap.size(), static_cast<size_t>(N / 2));

  // Verify remaining elements are odd numbers
  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.extract_min());

  EXPECT_EQ(extracted.size(), static_cast<size_t>(N / 2));
  for (size_t i = 0; i < extracted.size(); ++i)
    EXPECT_EQ(extracted[i], static_cast<int>(2 * i + 1));
}

TEST(FibonacciHeapStress, ManyMerges)
{
  std::vector<Fibonacci_Heap<int>> heaps(100);

  // Insert into each heap
  for (int i = 0; i < 100; ++i)
    for (int j = 0; j < 100; ++j)
      heaps[i].insert(i * 100 + j);

  // Merge all into first
  for (int i = 1; i < 100; ++i)
    heaps[0].merge(heaps[i]);

  EXPECT_EQ(heaps[0].size(), 10000u);

  // Verify sorted order
  int prev = heaps[0].extract_min();
  while (!heaps[0].is_empty())
    {
      int curr = heaps[0].extract_min();
      EXPECT_LE(prev, curr);
      prev = curr;
    }
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(FibonacciHeapEdgeCases, NegativeNumbers)
{
  Fibonacci_Heap<int> heap;

  heap.insert(-10);
  heap.insert(-5);
  heap.insert(-20);
  heap.insert(0);
  heap.insert(10);

  EXPECT_EQ(heap.extract_min(), -20);
  EXPECT_EQ(heap.extract_min(), -10);
  EXPECT_EQ(heap.extract_min(), -5);
  EXPECT_EQ(heap.extract_min(), 0);
  EXPECT_EQ(heap.extract_min(), 10);
}

TEST(FibonacciHeapEdgeCases, IntMinMax)
{
  Fibonacci_Heap<int> heap;

  heap.insert(std::numeric_limits<int>::max());
  heap.insert(0);
  heap.insert(std::numeric_limits<int>::min());

  EXPECT_EQ(heap.extract_min(), std::numeric_limits<int>::min());
  EXPECT_EQ(heap.extract_min(), 0);
  EXPECT_EQ(heap.extract_min(), std::numeric_limits<int>::max());
}

TEST(FibonacciHeapEdgeCases, SingleElementOperations)
{
  Fibonacci_Heap<int> heap;
  auto node = heap.insert(42);

  // Decrease key on single element
  heap.decrease_key(node, 10);
  EXPECT_EQ(heap.get_min(), 10);

  // Delete single element
  heap.delete_node(node);
  EXPECT_TRUE(heap.is_empty());
}

TEST(FibonacciHeapEdgeCases, AlternatingMinMax)
{
  Fibonacci_Heap<int> heap;

  // Insert alternating min and max values
  for (int i = 0; i < 100; ++i)
    {
      if (i % 2 == 0)
        heap.insert(std::numeric_limits<int>::min() + i / 2);
      else
        heap.insert(std::numeric_limits<int>::max() - i / 2);
    }

  // Should extract in sorted order
  int prev = heap.extract_min();
  while (!heap.is_empty())
    {
      int curr = heap.extract_min();
      EXPECT_LE(prev, curr);
      prev = curr;
    }
}

// =============================================================================
// Heap Property Verification Tests
// =============================================================================

// Helper to verify heap property
template <typename T, typename Compare>
bool verify_heap_property(Fibonacci_Heap<T, Compare> & heap)
{
  if (heap.is_empty())
    return true;

  std::vector<T> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.extract_min());

  // Verify sorted (for min-heap)
  for (size_t i = 1; i < extracted.size(); ++i)
    {
      if (extracted[i] < extracted[i - 1])
        return false;
    }

  return true;
}

TEST(FibonacciHeapProperty, RandomInsertions)
{
  Fibonacci_Heap<int> heap;
  std::random_device rd;
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(-10000, 10000);

  for (int i = 0; i < 1000; ++i)
    heap.insert(dis(gen));

  EXPECT_TRUE(verify_heap_property(heap));
}

TEST(FibonacciHeapProperty, AfterDecreaseKeys)
{
  Fibonacci_Heap<int> heap;
  std::vector<Fibonacci_Heap<int>::Node *> nodes;

  for (int i = 0; i < 100; ++i)
    nodes.push_back(heap.insert(i + 100));

  // Extract to build structure
  for (int i = 0; i < 20; ++i)
    heap.extract_min();

  // Decrease some keys
  for (size_t i = 30; i < 50; ++i)
    heap.decrease_key(nodes[i], static_cast<int>(i - 30));

  EXPECT_TRUE(verify_heap_property(heap));
}

TEST(FibonacciHeapProperty, AfterMerge)
{
  Fibonacci_Heap<int> h1, h2;
  std::random_device rd;
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(1, 1000);

  for (int i = 0; i < 500; ++i)
    {
      h1.insert(dis(gen));
      h2.insert(dis(gen));
    }

  h1.merge(h2);

  EXPECT_TRUE(verify_heap_property(h1));
}

// =============================================================================
// Memory and Performance Tests
// =============================================================================

TEST(FibonacciHeapMemory, DestructorFreesMemory)
{
  // This test mainly checks for memory leaks (run with valgrind/asan)
  for (int trial = 0; trial < 10; ++trial)
    {
      Fibonacci_Heap<int> heap;
      for (int i = 0; i < 1000; ++i)
        heap.insert(i);
      // Destructor should free all nodes
    }
}

TEST(FibonacciHeapMemory, ClearFreesMemory)
{
  // Run with valgrind/asan to check for leaks
  Fibonacci_Heap<int> heap;
  for (int i = 0; i < 1000; ++i)
    heap.insert(i);

  heap.clear();

  for (int i = 0; i < 1000; ++i)
    heap.insert(i + 1000);
}

namespace
{
  // Counts the values alive, to check that every node is deleted once
  struct Counted
  {
    static inline long alive = 0;
    int key;

    Counted(int k) : key(k) { ++alive; }
    Counted(const Counted & other) : key(other.key) { ++alive; }
    Counted & operator = (const Counted & other) = default;
    ~Counted() { --alive; }

    bool operator < (const Counted & other) const noexcept
    {
      return key < other.key;
    }
  };

  using CountedHeap = Fibonacci_Heap<Counted>;

  // Fills heap with trees of several children and levels: inserts, extracts
  // to link the roots and decreases keys to cut nodes. Returns the nodes
  // that are still in the heap.
  std::vector<CountedHeap::Node *> fill_with_bushy_trees(CountedHeap & heap)
  {
    std::mt19937 gen(141);
    std::vector<CountedHeap::Node *> nodes;
    for (int i = 0; i < 2000; ++i)
      nodes.push_back(heap.insert(Counted(int(gen() % 1000000))));

    auto extract = [&heap, &nodes]
      {
        nodes.erase(std::find(nodes.begin(), nodes.end(), heap.get_min_node()));
        (void) heap.extract_min();
      };
    for (int i = 0; i < 100; ++i)
      extract();
    for (int i = 0; i < 300; ++i)
      {
        auto *p = nodes[gen() % nodes.size()];
        heap.decrease_key(p, Counted(p->data.key - int(gen() % 1000)));
      }
    for (int i = 0; i < 20; ++i)
      extract();

    return nodes;
  }

  // The largest degree and the largest depth of the given nodes
  std::pair<size_t, size_t>
  degree_and_depth(const std::vector<CountedHeap::Node *> & nodes)
  {
    size_t degree = 0, depth = 0;
    for (auto *p : nodes)
      {
        degree = std::max(degree, p->degree);
        size_t d = 0;
        for (auto *q = p; q->parent != nullptr; q = q->parent)
          ++d;
        depth = std::max(depth, d);
      }
    return {degree, depth};
  }
}

// clear() and the destructor delete each node once, also in trees of several
// children and levels
TEST(FibonacciHeapMemory, ClearDeletesEveryNodeOfBushyTrees)
{
  ASSERT_EQ(Counted::alive, 0);
  {
    CountedHeap heap;
    const auto nodes = fill_with_bushy_trees(heap);
    const auto [degree, depth] = degree_and_depth(nodes);
    ASSERT_GE(degree, 3u);
    ASSERT_GE(depth, 3u);
    ASSERT_EQ(Counted::alive, long(heap.size()));

    heap.clear();
    EXPECT_TRUE(heap.is_empty());
    EXPECT_EQ(Counted::alive, 0);

    (void) fill_with_bushy_trees(heap);
  }
  EXPECT_EQ(Counted::alive, 0); // deleted by the destructor
}

namespace
{
  // The calls of a comparison, and the one that throws (0 for none)
  struct Comparisons
  {
    long calls = 0;
    long fail_at = 0;
  };

  // Orders Counted by key and counts its calls in count, which the copies
  // that the heaps keep share
  struct Failing_Counted_Less
  {
    Comparisons *count = nullptr;

    bool operator()(const Counted & a, const Counted & b) const
    {
      if (count != nullptr and ++count->calls == count->fail_at)
        throw std::runtime_error("comparison failed");
      return a.key < b.key;
    }
  };

  using Failing_Heap = Fibonacci_Heap<Counted, Failing_Counted_Less>;

  // Extracts the elements of heap with a comparison that does not throw,
  // and returns their keys in the order in which they come out
  std::vector<int> drain(Failing_Heap & heap, Comparisons & count)
  {
    count.fail_at = 0;
    std::vector<int> keys;
    while (not heap.is_empty())
      keys.push_back(heap.extract_min().key);
    return keys;
  }

  // Calls each operation that compares keys, with a comparison that counts
  // its calls in count, and returns the keys in the order in which they
  // come out. If the comparison throws, each heap must hold as many
  // elements as its size() says.
  std::vector<int> use_heaps(Comparisons & count)
  {
    Failing_Heap heap(Failing_Counted_Less{&count});
    Failing_Heap other(Failing_Counted_Less{&count});
    try
      {
        std::vector<Failing_Heap::Node *> nodes;
        for (int key : {50, 30, 70, 10, 90, 20, 80, 40, 60, 0})
          nodes.push_back(heap.insert(Counted(key)));
        const Counted copied(35);
        nodes.push_back(heap.insert(copied));
        nodes.push_back(heap.emplace(45));
        (void) heap.extract_min();                         // 0, nodes[9]
        heap.decrease_key(nodes[4], Counted(5));           // 90
        heap.decrease_key(nodes[6], copied);               // 80
        nodes[2] = heap.update_key(nodes[2], Counted(95)); // 70
        heap.delete_node(nodes[7]);                        // 40
        (void) other.insert(Counted(15));
        (void) other.insert(Counted(25));
        heap.merge(other);

        std::vector<int> keys;
        while (not heap.is_empty())
          keys.push_back(heap.extract_min().key);
        return keys;
      }
    catch (...)
      {
        const size_t size = heap.size();
        const size_t other_size = other.size();
        EXPECT_EQ(drain(heap, count).size(), size);
        EXPECT_EQ(drain(other, count).size(), other_size);
        throw;
      }
  }
}

// If the comparison threw, extract_min() and delete_node() lost the node
// that they had taken out, which size() still counted, and they could leave
// min_node on a node that was not a root or some roots out of the root
// list; insert() left in the heap a node that size() did not count, and
// merge() left the nodes of both heaps in each one
TEST(FibonacciHeapComparison, WhatTheComparisonThrowsLeavesEveryNodeCounted)
{
  const long alive = Counted::alive;
  Comparisons count;
  EXPECT_EQ(use_heaps(count),
            (std::vector<int>{5, 10, 15, 20, 25, 30, 35, 35, 45, 50, 60, 95}));
  EXPECT_EQ(Counted::alive, alive);

  for (long k = 1; k <= count.calls; ++k)
    {
      Comparisons failing;
      failing.fail_at = k;
      EXPECT_THROW(use_heaps(failing), std::runtime_error) << "call " << k;
      EXPECT_EQ(Counted::alive, alive) << "call " << k;
    }
}

// insert(), emplace() and merge() compare the keys before they change the
// heaps, so that if the comparison throws the heaps are as they were
TEST(FibonacciHeapComparison, InsertAndMergeLeaveTheHeapsAsTheyWere)
{
  const long alive = Counted::alive;
  {
    Comparisons count;
    Failing_Heap heap(Failing_Counted_Less{&count});
    Failing_Heap other(Failing_Counted_Less{&count});
    for (int key : {4, 2, 6})
      (void) heap.insert(Counted(key));
    (void) heap.extract_min(); // 4 and 6 in one tree
    for (int key : {5, 1})
      (void) other.insert(Counted(key));

    const Counted three(3);
    count.fail_at = count.calls + 1;
    EXPECT_THROW((void) heap.insert(three), std::runtime_error);
    count.fail_at = count.calls + 1;
    EXPECT_THROW((void) heap.insert(Counted(3)), std::runtime_error);
    count.fail_at = count.calls + 1;
    EXPECT_THROW((void) heap.emplace(3), std::runtime_error);
    count.fail_at = count.calls + 1;
    EXPECT_THROW(heap.merge(other), std::runtime_error);

    EXPECT_EQ(heap.size(), 2u);
    EXPECT_EQ(other.size(), 2u);
    EXPECT_EQ(drain(heap, count), (std::vector<int>{4, 6}));
    EXPECT_EQ(drain(other, count), (std::vector<int>{1, 5}));
  }
  EXPECT_EQ(Counted::alive, alive);
}

// If the comparison throws, extract_min() and delete_node() leave their
// node in the heap, which gives all its elements in order afterwards. They
// took the node out and destroyed it, and the heap could be left with a
// minimum that was not the least element
TEST(FibonacciHeapComparison, ExtractAndDeleteLeaveTheirNodeInIfTheComparisonThrows)
{
  const std::vector<int> keys = {8, 3, 5, 1, 9, 2, 7, 4, 6, 0};
  const long alive = Counted::alive;
  for (size_t i = 0; i <= keys.size(); ++i) // keys.size(): extract_min()
    {
      if (i < keys.size() and keys[i] == 0)
        continue; // extracted before
      for (long k = 1; ; ++k)
        {
          Comparisons count;
          Failing_Heap heap(Failing_Counted_Less{&count});
          std::vector<Failing_Heap::Node *> nodes;
          for (int key : keys)
            nodes.push_back(heap.insert(Counted(key)));
          (void) heap.extract_min(); // 0, so that the others are in trees

          const std::vector<int> all = {1, 2, 3, 4, 5, 6, 7, 8, 9};
          std::vector<int> rest = all;
          const int out = i < keys.size() ? keys[i] : 1;
          rest.erase(std::find(rest.begin(), rest.end(), out));

          count.fail_at = count.calls + k;
          bool thrown = false;
          try
            {
              if (i < keys.size())
                heap.delete_node(nodes[i]);
              else
                (void) heap.extract_min();
            }
          catch (const std::runtime_error &)
            {
              thrown = true;
            }
          const std::vector<int> & expected = thrown ? all : rest;
          EXPECT_EQ(heap.size(), expected.size()) << "node " << i << ", call " << k;
          EXPECT_EQ(drain(heap, count), expected) << "node " << i << ", call " << k; // in order
          if (not thrown)
            break;
        }
    }
  EXPECT_EQ(Counted::alive, alive);
}

namespace
{
  // A key with an identity apart from it; a move takes the identity of its
  // source away
  struct Tracked
  {
    static constexpr int gone = -1;

    int key = 0;
    int id = 0;

    Tracked(const int k, const int i) noexcept : key(k), id(i) {}
    Tracked(const Tracked &) = default;
    Tracked & operator=(const Tracked &) = default;
    Tracked(Tracked && o) noexcept : key(o.key), id(std::exchange(o.id, gone)) {}
    Tracked & operator=(Tracked && o) noexcept
    {
      key = o.key;
      id = std::exchange(o.id, gone);
      return *this;
    }
    bool operator==(const Tracked &) const = default;
  };

  void PrintTo(const Tracked & x, std::ostream *os) { *os << x.key << '#' << x.id; }

  // Orders Tracked by key and counts its calls in count, which the copies
  // that the heaps keep share
  struct Failing_Tracked_Less
  {
    Comparisons *count = nullptr;

    bool operator()(const Tracked & a, const Tracked & b) const
    {
      if (count != nullptr and ++count->calls == count->fail_at)
        throw std::runtime_error("comparison failed");
      return a.key < b.key;
    }
  };

  using Tracked_Heap = Fibonacci_Heap<Tracked, Failing_Tracked_Less>;

  // The nodes of a heap, and what is wrong in its trees: the lists of roots
  // and of children are circular and doubly linked, each node has its
  // parent and as many children as its degree says, no key is less than
  // that of its parent, the nodes are as many as size() says, and none has
  // a key less than that of the minimum
  std::string tree_errors(const Tracked_Heap & heap, std::vector<Tracked_Heap::Node *> & nodes)
  {
    using Node = Tracked_Heap::Node;
    std::ostringstream err;
    nodes.clear();
    Node *min = heap.get_min_node();
    if (min == nullptr)
      {
        if (heap.size() != 0)
          err << "no minimum in a heap of " << heap.size() << "; ";
        return err.str();
      }
    std::vector<std::pair<Node *, Node *>> lists{{min, nullptr}}; // a node of each, and their parent
    while (not lists.empty())
      {
        const auto [first, parent] = lists.back();
        lists.pop_back();
        size_t count = 0;
        Node *p = first;
        do
          {
            if (++count > heap.size())
              {
                err << "a list does not close; ";
                return err.str();
              }
            if (nodes.size() >= heap.size())
              { // a node is reached again: its children would be walked for ever
                err << "more nodes than the " << heap.size() << " of the heap; ";
                return err.str();
              }
            nodes.push_back(p);
            if (p->parent != parent)
              err << "the parent of " << p->data.id << "; ";
            if (p->right->left != p or p->left->right != p)
              err << "the links of " << p->data.id << "; ";
            if (parent != nullptr and p->data.key < parent->data.key)
              err << p->data.id << " is less than its parent; ";
            if (p->child != nullptr)
              lists.emplace_back(p->child, p);
            else if (p->degree != 0)
              err << "the degree of " << p->data.id << ", without children; ";
            p = p->right;
          }
        while (p != first);
        if (parent != nullptr and parent->degree != count)
          err << "the degree of " << parent->data.id << "; ";
      }
    if (nodes.size() != heap.size())
      err << nodes.size() << " nodes in a heap of " << heap.size() << "; ";
    std::vector<Node *> sorted = nodes;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
      err << "a node is twice in the heap; ";
    for (Node *p : nodes)
      if (p->data.key < min->data.key)
        err << p->data.id << " is less than the minimum; ";
    return err.str();
  }

  // Fills heap with trees of several children and levels, some of them with
  // marked nodes: inserts, extracts to link the roots, decreases keys to
  // cut nodes and inserts again, so that there are many roots. Many keys
  // are equal
  void fill_with_trees(Tracked_Heap & heap, const unsigned seed)
  {
    std::mt19937 gen(seed);
    std::vector<Tracked_Heap::Node *> nodes;
    for (int i = 0; i < 70; ++i)
      nodes.push_back(heap.insert(Tracked(int(gen() % 40), i + 1)));
    auto extract = [&heap, &nodes]
      {
        nodes.erase(std::find(nodes.begin(), nodes.end(), heap.get_min_node()));
        (void) heap.extract_min();
      };
    for (int i = 0; i < 6; ++i)
      extract();
    for (int i = 0; i < 14; ++i)
      {
        auto *p = nodes[gen() % nodes.size()];
        heap.decrease_key(p, Tracked(p->data.key - int(gen() % 30), p->data.id));
      }
    for (int i = 0; i < 3; ++i)
      extract();
    for (int i = 0; i < 12; ++i) // roots for the next consolidation to link
      (void) heap.insert(Tracked(int(gen() % 40), 100 + i));
  }
}

// If the comparison throws, in whichever of its calls, extract_min(),
// delete_node(), decrease_key() and update_key() leave in the heap the
// elements that it had, each in its node and with its key, and the same
// node as the minimum; the trees are well formed, and the key of a
// decrease_key() by move is not moved from. extract_min() and delete_node()
// destroyed their node, update_key() could destroy it without putting the
// new key in, decrease_key() could leave the new key in a node under a
// parent of a greater key, and all of them could leave as the minimum a
// node that was not the least
TEST(FibonacciHeapComparison, EveryFailedCallLeavesTheElementsAndTheMinimum)
{
  size_t failed = 0, marked = 0, deep = 0;
  for (int op = 0; op < 6; ++op)
    for (unsigned seed = 1; seed <= 8; ++seed)
      for (long k = 1; ; ++k)
        {
          SCOPED_TRACE(testing::Message() << "op " << op << ", seed " << seed << ", call " << k);
          Comparisons count;
          Tracked_Heap heap(Failing_Tracked_Less{&count});
          fill_with_trees(heap, seed);

          std::vector<Tracked_Heap::Node *> before;
          ASSERT_EQ(tree_errors(heap, before), "");
          std::map<Tracked_Heap::Node *, Tracked> keys;
          for (auto *p : before)
            {
              keys.emplace(p, p->data);
              marked += p->mark ? 1 : 0;
              deep += p->parent != nullptr and p->parent->parent != nullptr ? 1 : 0;
            }
          Tracked_Heap::Node *min = heap.get_min_node();

          std::mt19937 gen(seed*31 + unsigned(op));
          Tracked_Heap::Node *x = before[gen() % before.size()];
          const Tracked less_key(x->data.key - 1 - int(gen() % 30), 1000);
          const Tracked more_key(x->data.key + 1 + int(gen() % 30), 1000);
          Tracked moved = less_key;

          count.fail_at = count.calls + k;
          bool thrown = false;
          try
            {
              switch (op)
                {
                case 0: (void) heap.extract_min(); break;
                case 1: heap.delete_node(x); break;
                case 2: heap.decrease_key(x, less_key); break;
                case 3: heap.decrease_key(x, std::move(moved)); break;
                case 4: EXPECT_EQ(heap.update_key(x, less_key), x); break;
                default: EXPECT_EQ(heap.update_key(x, more_key), x);
                }
            }
          catch (const std::runtime_error &)
            {
              thrown = true;
            }
          count.fail_at = 0;

          std::vector<Tracked_Heap::Node *> after;
          ASSERT_EQ(tree_errors(heap, after), "");
          if (thrown)
            {
              ++failed;
              EXPECT_EQ(heap.size(), before.size());
              EXPECT_EQ(heap.get_min_node(), min);
              std::map<Tracked_Heap::Node *, Tracked> now;
              for (auto *p : after)
                now.emplace(p, p->data);
              EXPECT_EQ(now, keys);
              EXPECT_EQ(moved, less_key);
            }
          else if (op >= 2)
            {
              EXPECT_EQ(x->data, op == 5 ? more_key : less_key);
            }

          std::vector<int> out;
          while (not heap.is_empty())
            out.push_back(heap.extract_min().key);
          EXPECT_TRUE(std::is_sorted(out.begin(), out.end()));
          EXPECT_EQ(out.size(), before.size() - (not thrown and op < 2 ? 1 : 0));
          if (not thrown)
            break;
        }
  EXPECT_GT(failed, 300u);
  EXPECT_GT(marked, 0u); // the heaps had marked nodes
  EXPECT_GT(deep, 0u);   // and nodes at the third level
}

// When the comparison throws, a consolidation may have left the node that
// was the minimum under another one of an equivalent key, and that one
// under a third. The minimum is cut from it and is a root again, and the
// node that lost it is marked, as one that loses a child by decrease_key()
// is: the loss of another child cuts it too. It was left unmarked, so it
// could lose two children and stay under its parent
TEST(FibonacciHeapComparison, TheNodeThatLosesTheMinimumIsMarked)
{
  size_t marked = 0, cut = 0;
  for (size_t victim = 0; victim < 10; ++victim)
    for (long k = 1; ; ++k)
      {
        SCOPED_TRACE(testing::Message() << "node " << victim + 1 << ", call " << k);
        Comparisons count;
        Tracked_Heap heap(Failing_Tracked_Less{&count});
        std::vector<Tracked_Heap::Node *> handles;
        for (int id = 1; id <= 10; ++id)
          handles.push_back(heap.insert(Tracked(5, id))); // equivalent keys
        Tracked_Heap::Node *min = heap.get_min_node();

        count.fail_at = count.calls + k;
        bool thrown = false;
        try
          {
            heap.delete_node(handles[victim]);
          }
        catch (const std::runtime_error &)
          {
            thrown = true;
          }
        count.fail_at = 0;
        if (not thrown)
          break;

        std::vector<Tracked_Heap::Node *> nodes;
        ASSERT_EQ(tree_errors(heap, nodes), "");
        ASSERT_EQ(heap.get_min_node(), min);
        // nothing had been cut before, so a marked node is one that lost
        // the minimum, and it is not a root
        Tracked_Heap::Node *loser = nullptr;
        for (auto *p : nodes)
          if (p->mark)
            {
              EXPECT_EQ(loser, nullptr);
              EXPECT_NE(p->parent, nullptr);
              loser = p;
            }
        if (loser == nullptr)
          continue;
        ++marked;
        if (loser->child == nullptr)
          continue;
        // the loss of another child takes it out of its parent
        Tracked_Heap::Node *child = loser->child;
        heap.decrease_key(child, Tracked(4, child->data.id));
        EXPECT_EQ(child->parent, nullptr);
        EXPECT_EQ(loser->parent, nullptr);
        EXPECT_FALSE(loser->mark);
        ASSERT_EQ(tree_errors(heap, nodes), "");
        ++cut;
      }
  EXPECT_GT(marked, 0u);
  EXPECT_GT(cut, 0u);
}

namespace
{
  // A key whose copies throw when they are told to: its copy constructor
  // before it writes anything, and its copy assignment once it has written
  struct Brittle
  {
    static inline bool copy_throws = false;
    static inline bool assignment_throws = false;

    int key = 0;
    int id = 0;

    Brittle(const int k, const int i) noexcept : key(k), id(i) {}

    Brittle(const Brittle & other) : key(other.key), id(other.id)
    {
      if (copy_throws)
        throw std::runtime_error("copy");
    }

    Brittle(Brittle &&) noexcept = default;

    Brittle & operator=(const Brittle & other)
    {
      key = other.key;
      id = other.id;
      if (assignment_throws)
        throw std::runtime_error("assignment");
      return *this;
    }

    Brittle & operator=(Brittle &&) noexcept = default;

    bool operator==(const Brittle &) const = default;
  };

  void PrintTo(const Brittle & x, std::ostream *os) { *os << x.key << '#' << x.id; }

  struct Brittle_Less
  {
    bool operator()(const Brittle & a, const Brittle & b) const noexcept { return a.key < b.key; }
  };
}

// update_key() with a greater key assigned the key to the node once the
// node was out of its tree. An assignment that threw after writing left a
// heap whose minimum was the node of before, whatever its key was by then.
// Now the key is copied before the heap changes and moved into the node at
// the end: a copy that throws changes nothing
TEST(FibonacciHeapComparison, UpdateKeyCopiesTheKeyBeforeItChangesTheHeap)
{
  Fibonacci_Heap<Brittle, Brittle_Less> heap;
  auto *one = heap.insert(Brittle(1, 1));
  auto *five = heap.insert(Brittle(5, 2));
  auto *seven = heap.insert(Brittle(7, 3));
  (void) heap.insert(Brittle(0, 4));
  EXPECT_EQ(heap.extract_min(), Brittle(0, 4)); // one and five are roots now
  ASSERT_EQ(heap.get_min_node(), one);
  ASSERT_EQ(five->parent, nullptr);
  ASSERT_EQ(seven->parent, five); // which update_key() cuts from its tree
  const Brittle nine(9, 5);

  Brittle::copy_throws = true;
  EXPECT_THROW((void) heap.update_key(one, nine), std::runtime_error);
  EXPECT_THROW((void) heap.update_key(five, nine), std::runtime_error);
  EXPECT_THROW((void) heap.update_key(seven, nine), std::runtime_error);
  Brittle::copy_throws = false;
  EXPECT_EQ(heap.size(), 3u);
  EXPECT_EQ(heap.get_min_node(), one);
  EXPECT_EQ(one->data, Brittle(1, 1));
  EXPECT_EQ(five->data, Brittle(5, 2));
  EXPECT_EQ(seven->data, Brittle(7, 3));
  EXPECT_EQ(seven->parent, five);
  EXPECT_EQ(five->degree, 1u);

  // the copy assignment, which throws after it writes, is not what puts
  // the key in the node
  Brittle::assignment_throws = true;
  EXPECT_EQ(heap.update_key(one, nine), one);
  Brittle::assignment_throws = false;
  EXPECT_EQ(one->data, nine);
  EXPECT_EQ(heap.size(), 3u);
  EXPECT_EQ(heap.get_min_node(), five);
  EXPECT_EQ(heap.extract_min(), Brittle(5, 2));
  EXPECT_EQ(heap.extract_min(), Brittle(7, 3));
  EXPECT_EQ(heap.extract_min(), nine);
  EXPECT_TRUE(heap.is_empty());
}

namespace
{
  // A key that is copied, and whose move assignment is deleted
  struct Copied_Only
  {
    int key = 0;

    explicit Copied_Only(const int k) noexcept : key(k) {}
    Copied_Only(const Copied_Only &) = default;
    Copied_Only & operator=(const Copied_Only &) = default;
    Copied_Only & operator=(Copied_Only &&) = delete;

    bool operator<(const Copied_Only & other) const noexcept { return key < other.key; }
  };
}

// update_key() with a greater key deleted the node and inserted a copy of
// the key: it took a key whose move assignment is deleted, as
// decrease_key() does. Moving the key into the node stopped compiling with
// such a key, which is copied into it now
TEST(FibonacciHeapComparison, UpdateKeyTakesAKeyWithNoMoveAssignment)
{
  static_assert(not std::is_move_assignable_v<Copied_Only>);
  static_assert(std::is_copy_assignable_v<Copied_Only>);
  Fibonacci_Heap<Copied_Only> heap;
  const Copied_Only zero(0), one(1), three(3), five(5), seven(7), nine(9);
  auto *n1 = heap.insert(one);
  auto *n5 = heap.insert(five);
  auto *n7 = heap.insert(seven);
  (void) heap.insert(zero);
  EXPECT_EQ(heap.extract_min().key, 0);
  ASSERT_EQ(heap.get_min_node(), n1);
  ASSERT_EQ(n7->parent, n5);

  EXPECT_EQ(heap.update_key(n7, nine), n7); // a child
  EXPECT_EQ(n7->data.key, 9);
  EXPECT_EQ(n7->parent, nullptr);
  EXPECT_EQ(heap.update_key(n1, seven), n1); // the minimum
  EXPECT_EQ(n1->data.key, 7);
  EXPECT_EQ(heap.get_min_node(), n5);
  EXPECT_EQ(heap.update_key(n7, three), n7); // decreases
  EXPECT_EQ(heap.get_min_node(), n7);
  EXPECT_EQ(heap.size(), 3u);
  EXPECT_EQ(heap.extract_min().key, 3);
  EXPECT_EQ(heap.extract_min().key, 5);
  EXPECT_EQ(heap.extract_min().key, 7);
  EXPECT_TRUE(heap.is_empty());
}

// update_key() with a greater key deleted the node and inserted another one.
// Now it keeps the node, so the handle is the same and no node is allocated
TEST(FibonacciHeapComparison, UpdateKeyKeepsTheNodeWhenTheKeyIncreases)
{
  Comparisons count;
  Tracked_Heap heap(Failing_Tracked_Less{&count});
  fill_with_trees(heap, 5);
  std::vector<Tracked_Heap::Node *> nodes;
  ASSERT_EQ(tree_errors(heap, nodes), "");
  const size_t size = heap.size();
  int id = 2000;
  for (auto *p : nodes)
    {
      const Tracked key(p->data.key + 7, ++id);
      EXPECT_EQ(heap.update_key(p, key), p);
      EXPECT_EQ(p->data, key);
      std::vector<Tracked_Heap::Node *> now;
      ASSERT_EQ(tree_errors(heap, now), "");
      ASSERT_EQ(heap.size(), size);
    }
  // the minimum alone, in a heap of one node
  Tracked_Heap one(Failing_Tracked_Less{&count});
  auto *p = one.insert(Tracked(3, 1));
  EXPECT_EQ(one.update_key(p, Tracked(9, 2)), p);
  EXPECT_EQ(one.get_min(), Tracked(9, 2));
  EXPECT_EQ(one.size(), 1u);
}

// With several roots of the least key, a consolidation links the node that
// was the minimum under another of them. If the comparison throws after
// that, in delete_node() or in update_key() of another node, that node is
// the minimum again, and a root
TEST(FibonacciHeapComparison, AMinimumThatWasLinkedIsTheMinimumAgain)
{
  size_t failed = 0;
  for (int op = 0; op < 2; ++op)
    for (size_t which = 1; which < 12; ++which) // the consolidation starts after the node that leaves
      for (long k = 1; ; ++k)
        {
          SCOPED_TRACE(testing::Message() << "op " << op << ", node " << which << ", call " << k);
          Comparisons count;
          Tracked_Heap heap(Failing_Tracked_Less{&count});
          std::vector<Tracked_Heap::Node *> nodes;
          for (int i = 0; i < 12; ++i) // eight roots with the least key
            nodes.push_back(heap.insert(Tracked(i < 8 ? 5 : 5 + i, i + 1)));
          Tracked_Heap::Node *min = heap.get_min_node();
          Tracked_Heap::Node *x = nodes[which];
          ASSERT_NE(x, min);

          count.fail_at = count.calls + k;
          bool thrown = false;
          try
            {
              if (op == 0)
                heap.delete_node(x);
              else
                (void) heap.update_key(x, Tracked(99, 50));
            }
          catch (const std::runtime_error &)
            {
              thrown = true;
            }
          count.fail_at = 0;

          std::vector<Tracked_Heap::Node *> after;
          EXPECT_EQ(tree_errors(heap, after), "");
          if (not thrown)
            break;
          ++failed;
          EXPECT_EQ(heap.get_min_node(), min);
          EXPECT_EQ(min->parent, nullptr);
          EXPECT_EQ(heap.size(), 12u);
        }
  EXPECT_GT(failed, 100u);
}

// A key that increases and is still the least one leaves its node as the
// minimum; if another key is less than it now, the node of that one is
TEST(FibonacciHeapComparison, UpdateKeyOfTheMinimumToAGreaterKey)
{
  Comparisons count;
  Tracked_Heap heap(Failing_Tracked_Less{&count});
  std::vector<Tracked_Heap::Node *> nodes;
  for (int i = 0; i < 9; ++i)
    nodes.push_back(heap.insert(Tracked(10*i, i + 1))); // 0, 10, ..., 80
  EXPECT_EQ(heap.extract_min().key, 0); // the others, in trees
  std::vector<Tracked_Heap::Node *> now;

  Tracked_Heap::Node *p = heap.get_min_node();
  ASSERT_EQ(p, nodes[1]);
  EXPECT_EQ(heap.update_key(p, Tracked(15, 100)), p); // still less than 20
  EXPECT_EQ(heap.get_min_node(), p);
  EXPECT_EQ(tree_errors(heap, now), "");

  EXPECT_EQ(heap.update_key(p, Tracked(25, 101)), p); // 20 is the least now
  EXPECT_EQ(heap.get_min_node(), nodes[2]);
  EXPECT_EQ(tree_errors(heap, now), "");

  std::vector<int> out;
  while (not heap.is_empty())
    out.push_back(heap.extract_min().key);
  EXPECT_EQ(out, (std::vector<int>{20, 25, 30, 40, 50, 60, 70, 80}));
}

// Performance test (disabled by default due to time)
TEST(FibonacciHeapPerformance, DISABLED_TimingComparison)
{
  constexpr int N = 1000000;

  auto start = std::chrono::high_resolution_clock::now();

  Fibonacci_Heap<int> heap;
  for (int i = N; i >= 1; --i)
    (void)heap.insert(i);

  auto after_insert = std::chrono::high_resolution_clock::now();

  while (!heap.is_empty())
    heap.extract_min();

  auto after_extract = std::chrono::high_resolution_clock::now();

  auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      after_insert - start).count();
  auto extract_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      after_extract - after_insert).count();

  std::cout << "Insert " << N << " elements: " << insert_time << " ms\n";
  std::cout << "Extract " << N << " elements: " << extract_time << " ms\n";
}

// =============================================================================
// Dijkstra-like Usage Pattern Test
// =============================================================================

TEST(FibonacciHeapUsagePattern, DijkstraSimulation)
{
  // Simulate Dijkstra's algorithm usage pattern
  struct DistNode
  {
    int vertex;
    int distance;

    bool operator<(const DistNode & other) const
    {
      return distance < other.distance;
    }
  };

  Fibonacci_Heap<DistNode> pq;
  std::vector<Fibonacci_Heap<DistNode>::Node *> handles(100, nullptr);

  // Initialize all vertices with infinite distance except source
  for (int v = 0; v < 100; ++v)
    {
      int dist = (v == 0) ? 0 : std::numeric_limits<int>::max();
      handles[v] = pq.insert({v, dist});
    }

  // Simulate relaxation
  std::random_device rd;
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dist_gen(1, 100);

  while (!pq.is_empty())
    {
      DistNode u = pq.extract_min();

      // Mark as processed BEFORE accessing other handles
      // (the extracted node's handle is now invalid)
      handles[u.vertex] = nullptr;

      // Simulate relaxing neighbors
      for (int i = 0; i < 3; ++i)
        {
          int v = dist_gen(gen) % 100;
          // Only access handles that haven't been extracted yet
          if (handles[v] != nullptr && handles[v]->data.distance > u.distance + 10)
            {
              // Decrease key to simulate edge relaxation
              pq.decrease_key(handles[v], {v, u.distance + 10});
            }
        }
    }
}

// =============================================================================
// Comparator Tests
// =============================================================================

TEST(FibonacciHeapComparator, KeyComp)
{
  Fibonacci_Heap<int> heap;
  auto cmp = heap.key_comp();

  EXPECT_TRUE(cmp(1, 2));
  EXPECT_FALSE(cmp(2, 1));
  EXPECT_FALSE(cmp(1, 1));
}

TEST(FibonacciHeapComparator, CustomLambdaComparator)
{
  auto cmp = [](int a, int b) { return a > b; };  // Max heap
  Fibonacci_Heap<int, decltype(cmp)> heap(cmp);

  (void)heap.insert(10);
  (void)heap.insert(30);
  (void)heap.insert(20);

  EXPECT_EQ(heap.extract_min(), 30);
  EXPECT_EQ(heap.extract_min(), 20);
  EXPECT_EQ(heap.extract_min(), 10);
}

// =============================================================================
// Additional Edge Case Tests
// =============================================================================

TEST(FibonacciHeapEdgeCases, DecreaseKeyOnRootNode)
{
  Fibonacci_Heap<int> heap;
  auto node = heap.insert(50);
  heap.insert(60);
  heap.insert(70);

  // Decrease the minimum (root) node - should just update data
  heap.decrease_key(node, 10);

  EXPECT_EQ(heap.get_min(), 10);
  EXPECT_EQ(heap.extract_min(), 10);
  EXPECT_EQ(heap.extract_min(), 60);
  EXPECT_EQ(heap.extract_min(), 70);
}

TEST(FibonacciHeapEdgeCases, DecreaseKeyChildBecomesSmallerThanParent)
{
  Fibonacci_Heap<int> heap;

  // Insert values that will create parent-child relationships after consolidate
  for (int i = 1; i <= 10; ++i)
    heap.insert(i * 10);

  // Extract to force consolidation
  heap.extract_min();  // Remove 10
  heap.extract_min();  // Remove 20

  // Insert a large value and then decrease it
  auto node = heap.insert(1000);
  heap.decrease_key(node, 5);  // Now smaller than any remaining

  EXPECT_EQ(heap.get_min(), 5);

  // Verify heap property is maintained
  int prev = heap.extract_min();
  while (!heap.is_empty())
    {
      int curr = heap.extract_min();
      EXPECT_LE(prev, curr);
      prev = curr;
    }
}

TEST(FibonacciHeapEdgeCases, DeleteNodeWithMultipleChildren)
{
  Fibonacci_Heap<int> heap;
  std::vector<Fibonacci_Heap<int>::Node *> nodes;

  // Insert 15 elements to create trees with multiple children
  for (int i = 0; i < 15; ++i)
    nodes.push_back(heap.insert(i + 1));

  // Extract several times to build tree structure with children
  for (int i = 0; i < 4; ++i)
    heap.extract_min();

  // Delete a node that likely has children (after consolidation)
  // Node with value 8 should still be in the heap
  heap.delete_node(nodes[7]);  // Delete node with original value 8

  // Verify heap property and correct count
  EXPECT_EQ(heap.size(), 10u);  // 15 - 4 extracted - 1 deleted

  int prev = heap.extract_min();
  while (!heap.is_empty())
    {
      int curr = heap.extract_min();
      EXPECT_LE(prev, curr);
      prev = curr;
    }
}

TEST(FibonacciHeapEdgeCases, MergeSelfNoOp)
{
  Fibonacci_Heap<int> heap;
  heap.insert(10);
  heap.insert(20);

  heap.merge(heap);  // Self-merge should be no-op

  EXPECT_EQ(heap.size(), 2u);
  EXPECT_EQ(heap.get_min(), 10);
}

TEST(FibonacciHeapEdgeCases, UpdateKeyToSameValueOnChild)
{
  Fibonacci_Heap<int> heap;

  // Create structure with parent-child relationships
  for (int i = 1; i <= 10; ++i)
    heap.insert(i * 10);

  // Extract to consolidate
  heap.extract_min();

  // Insert and update to same value
  auto node = heap.insert(500);
  auto result = heap.update_key(node, 500);

  EXPECT_EQ(result, node);
  EXPECT_EQ(result->data, 500);
}

TEST(FibonacciHeapEdgeCases, ConsecutiveDecreaseKeys)
{
  Fibonacci_Heap<int> heap;

  auto node = heap.insert(100);
  heap.insert(200);
  heap.insert(300);

  // Multiple consecutive decrease keys on same node
  heap.decrease_key(node, 90);
  EXPECT_EQ(heap.get_min(), 90);

  heap.decrease_key(node, 50);
  EXPECT_EQ(heap.get_min(), 50);

  heap.decrease_key(node, 10);
  EXPECT_EQ(heap.get_min(), 10);

  // Verify extraction order
  EXPECT_EQ(heap.extract_min(), 10);
  EXPECT_EQ(heap.extract_min(), 200);
  EXPECT_EQ(heap.extract_min(), 300);
}

TEST(FibonacciHeapEdgeCases, EmplaceWithSingleArg)
{
  Fibonacci_Heap<int> heap;

  // emplace with single arg should work (goes through insert path)
  auto node = heap.emplace(42);

  EXPECT_EQ(node->data, 42);
  EXPECT_EQ(heap.get_min(), 42);
}

TEST(FibonacciHeapEdgeCases, LargeDegreeTrees)
{
  Fibonacci_Heap<int> heap;

  // Insert enough elements to create high-degree trees
  // Fibonacci heap can have trees of degree up to log_phi(n)
  constexpr int N = 10000;
  for (int i = N; i >= 1; --i)
    heap.insert(i);

  // Extract half to create complex tree structure
  for (int i = 0; i < N / 2; ++i)
    {
      int val = heap.extract_min();
      EXPECT_EQ(val, i + 1);
    }

  // Verify remaining half
  for (int i = N / 2 + 1; i <= N; ++i)
    {
      int val = heap.extract_min();
      EXPECT_EQ(val, i);
    }

  EXPECT_TRUE(heap.is_empty());
}

TEST(FibonacciHeapEdgeCases, SwapEmptyHeaps)
{
  Fibonacci_Heap<int> h1, h2;

  h1.swap(h2);

  EXPECT_TRUE(h1.is_empty());
  EXPECT_TRUE(h2.is_empty());
}

TEST(FibonacciHeapEdgeCases, MoveAssignToSelf)
{
  Fibonacci_Heap<int> heap;
  heap.insert(1);
  heap.insert(2);
  heap.insert(3);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
  heap = std::move(heap);
#pragma GCC diagnostic pop

  // After self-move, the heap should still be valid (either empty or same)
  // This is implementation-defined, but should not crash
}

TEST(FibonacciHeapEdgeCases, DeleteLastTwoNodes)
{
  Fibonacci_Heap<int> heap;
  auto n1 = heap.insert(10);
  auto n2 = heap.insert(20);

  heap.delete_node(n1);
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.get_min(), 20);

  heap.delete_node(n2);
  EXPECT_TRUE(heap.is_empty());
}

// =============================================================================
// Regression Tests
// =============================================================================

// Test that delete_node properly handles the case where the deleted node
// is alone in the root list but has children
TEST(FibonacciHeapRegression, DeleteAloneRootWithChildren)
{
  Fibonacci_Heap<int> heap;

  // Build a specific structure
  heap.insert(1);
  heap.insert(2);
  heap.insert(3);
  heap.insert(4);

  // Extract to consolidate into a single tree
  EXPECT_EQ(heap.extract_min(), 1);
  EXPECT_EQ(heap.extract_min(), 2);

  // Now we have 3 and 4, likely in a parent-child relationship
  // Delete the root (3)
  auto root = heap.get_min_node();
  heap.delete_node(root);

  // Should have only 4 left
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.get_min(), 4);
}

// Test cascading cuts trigger properly
TEST(FibonacciHeapRegression, CascadingCutsChain)
{
  Fibonacci_Heap<int> heap;
  std::vector<Fibonacci_Heap<int>::Node *> nodes;

  // Create a deep tree by inserting many elements
  for (int i = 1; i <= 100; ++i)
    nodes.push_back(heap.insert(i * 100));

  // Extract several to build tree structure
  for (int i = 0; i < 30; ++i)
    heap.extract_min();

  // Now decrease keys to trigger cascading cuts
  // Decrease non-extracted nodes in sequence
  int key = 1;
  for (size_t i = 50; i < 70; ++i)
    {
      if (nodes[i]->data > key)
        {
          heap.decrease_key(nodes[i], key);
          ++key;
        }
    }

  // Verify heap property is maintained
  int prev = heap.extract_min();
  while (!heap.is_empty())
    {
      int curr = heap.extract_min();
      EXPECT_LE(prev, curr);
      prev = curr;
    }
}

namespace
{
  // Runs f, which must not throw, in a thread with a stack of 256 KiB, which
  // a recursion as deep as the trees of these tests are high overflows.
  // Without POSIX threads, it runs f in this thread.
  template <class F>
  void run_with_small_stack(F f)
  {
# if defined(__unix__) || defined(__APPLE__)
    pthread_attr_t attr;
    ASSERT_EQ(pthread_attr_init(&attr), 0);
    ASSERT_EQ(pthread_attr_setstacksize(&attr, 256 * 1024), 0);
    auto start = [](void *arg) -> void *
      {
        (*static_cast<F *>(arg))();
        return nullptr;
      };
    pthread_t thread;
    const int created = pthread_create(&thread, &attr, start, &f);
    pthread_attr_destroy(&attr);
    ASSERT_EQ(created, 0);
    ASSERT_EQ(pthread_join(thread, nullptr), 0);
# else
    f();
# endif
  }

  // Puts in heap, which must be empty, a single tree that is a chain of n
  // nodes (CLRS, exercise 19.4-1). Each round inserts a < b < c below the
  // root and extracts a, so that b takes c and the chain as children; then
  // it cuts c with decrease_key() and extracts it, so that b ends on top of
  // the chain.
  void build_chain(Fibonacci_Heap<long> & heap, long n)
  {
    long key = 4 * n;
    (void) heap.insert(key);
    for (long i = 1; i < n; ++i)
      {
        const long b = key - 2;
        (void) heap.insert(key - 3);
        (void) heap.insert(b);
        auto *c = heap.insert(key - 1);
        (void) heap.extract_min();
        heap.decrease_key(c, b - 1);
        (void) heap.extract_min();
        key = b;
      }
  }

  // The number of nodes from the root down by child
  size_t height_of_chain(const Fibonacci_Heap<long> & heap)
  {
    size_t height = 0;
    for (auto *p = heap.get_min_node(); p != nullptr; p = p->child)
      ++height;
    return height;
  }
}

// clear() deleted the children of each node recursively, so a tree as high
// as the heap is large overflowed the stack
TEST(FibonacciHeapRegression, ClearsATreeAsHighAsTheHeap)
{
  constexpr long n = 50000;
  Fibonacci_Heap<long> heap;
  build_chain(heap, n);
  ASSERT_EQ(heap.size(), size_t(n));
  ASSERT_EQ(heap.get_min_node()->right, heap.get_min_node()); // one tree
  ASSERT_EQ(height_of_chain(heap), size_t(n));

  run_with_small_stack([&heap] { heap.clear(); });
  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0u);

  (void) heap.insert(7);
  EXPECT_EQ(heap.get_min(), 7);
}

// The destructor deletes the nodes as clear() does
TEST(FibonacciHeapRegression, DestroysATreeAsHighAsTheHeap)
{
  constexpr long n = 50000;
  auto heap = std::make_unique<Fibonacci_Heap<long>>();
  build_chain(*heap, n);
  ASSERT_EQ(height_of_chain(*heap), size_t(n));

  run_with_small_stack([&heap] { heap.reset(); });
  EXPECT_EQ(heap, nullptr);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
