
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
# include <tpl_fibonacci_heap.H>
# include <vector>
# include <algorithm>
# include <random>
# include <string>
# include <set>
# include <chrono>
# include <functional>

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

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
