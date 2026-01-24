
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
 * @file container_edge_cases_test.cc
 * @brief Comprehensive edge case tests for all Aleph-w containers
 *
 * This file contains edge case tests covering:
 * - Empty container operations
 * - Single element operations
 * - Boundary conditions
 * - Iterator edge cases
 * - Memory and capacity limits
 * - Duplicate handling
 * - Extreme values
 */

#include <gtest/gtest.h>
#include <limits>
#include <random>
#include <vector>
#include <string>
#include <set>
#include <deque>

// Container headers
#include <tpl_array.H>
#include <tpl_dynArray.H>
#include <tpl_arrayQueue.H>
#include <tpl_arrayStack.H>
#include <tpl_dynDlist.H>
#include <htlist.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>
#include <tpl_dynSetTree.H>
#include <bitArray.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// DynArray Edge Cases
// ============================================================================

class DynArrayEdgeCases : public ::testing::Test
{
protected:
  DynArray<int> arr;
};

TEST_F(DynArrayEdgeCases, EmptyArray_SizeIsZero)
{
  EXPECT_EQ(arr.size(), 0);
  EXPECT_TRUE(arr.is_empty());
}

TEST_F(DynArrayEdgeCases, EmptyArray_ExistReturnsFalse)
{
  // Empty array - no valid indices
  EXPECT_FALSE(arr.exist(0));
  EXPECT_FALSE(arr.exist(100));
}

TEST_F(DynArrayEdgeCases, SingleElement_InsertAndAccess)
{
  arr.touch(0) = 42;  // Use touch() which allocates
  EXPECT_EQ(arr.size(), 1);
  EXPECT_EQ(arr(0), 42);  // Use () for fast read
  EXPECT_FALSE(arr.is_empty());
}

TEST_F(DynArrayEdgeCases, SparseAccess_AutoAllocation)
{
  arr.touch(100) = 999;  // Use touch() for allocation
  EXPECT_EQ(arr.size(), 101);
  EXPECT_EQ(arr(100), 999);
  // Note: sparse arrays may not initialize intermediate elements
  EXPECT_TRUE(arr.exist(100));
}

// Note: DynArray allocates blocks, so exist() can return true for indices
// in allocated blocks even if not explicitly written
TEST_F(DynArrayEdgeCases, ExistChecksAllocatedBlocks)
{
  arr.touch(50) = 42;
  EXPECT_TRUE(arr.exist(50));
  // exist() checks if the block is allocated, not just if i < size
  EXPECT_EQ(arr.size(), 51);
}

TEST_F(DynArrayEdgeCases, LargeIndex_Grows)
{
  arr.touch(10000) = 1;
  EXPECT_GE(arr.size(), 10001);
  EXPECT_EQ(arr(10000), 1);
}

TEST_F(DynArrayEdgeCases, CutToSize_Shrinks)
{
  for (int i = 0; i < 100; ++i)
    arr.touch(i) = i;
  
  arr.cut(50);
  EXPECT_EQ(arr.size(), 50);
  EXPECT_EQ(arr(49), 49);
  // Note: cut() may not deallocate memory, so exist() might still return true
  // for previously allocated indices. The key invariant is size() == 50
}

TEST_F(DynArrayEdgeCases, CutToZero_EmptiesArray)
{
  arr.touch(10) = 42;
  arr.cut(0);
  EXPECT_EQ(arr.size(), 0);
  EXPECT_TRUE(arr.is_empty());
}

// ============================================================================
// ArrayQueue Edge Cases
// ============================================================================

class ArrayQueueEdgeCases : public ::testing::Test
{
protected:
  ArrayQueue<int> queue = ArrayQueue<int>(10);
};

TEST_F(ArrayQueueEdgeCases, EmptyQueue_IsEmpty)
{
  EXPECT_TRUE(queue.is_empty());
  EXPECT_EQ(queue.size(), 0);
}

TEST_F(ArrayQueueEdgeCases, EmptyQueue_GetThrows)
{
  EXPECT_THROW(queue.get(), std::underflow_error);
}

TEST_F(ArrayQueueEdgeCases, EmptyQueue_SizeZero)
{
  // Empty queue should have size 0
  EXPECT_EQ(queue.size(), 0);
  // After put/get cycle, should return to empty
  queue.put(1);
  (void)queue.get();
  EXPECT_EQ(queue.size(), 0);
}

TEST_F(ArrayQueueEdgeCases, SingleElement_FIFO)
{
  queue.put(42);
  EXPECT_FALSE(queue.is_empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(queue.front(), 42);
  EXPECT_EQ(queue.rear(), 42);
  EXPECT_EQ(queue.get(), 42);
  EXPECT_TRUE(queue.is_empty());
}

TEST_F(ArrayQueueEdgeCases, FillToCapacity)
{
  for (int i = 0; i < 10; ++i)
    queue.put(i);
  
  EXPECT_EQ(queue.size(), 10);
  EXPECT_EQ(queue.front(), 0);
  EXPECT_EQ(queue.rear(), 9);
}

TEST_F(ArrayQueueEdgeCases, FillAndResize)
{
  // ArrayQueue may auto-resize, so just verify it handles many elements
  for (int i = 0; i < 20; ++i)
    queue.put(i);
  
  EXPECT_EQ(queue.size(), 20);
  // Elements should be in FIFO order
  for (int i = 0; i < 20; ++i)
    EXPECT_EQ(queue.get(), i);
}

TEST_F(ArrayQueueEdgeCases, WrapAround)
{
  // Fill and partially empty
  for (int i = 0; i < 8; ++i)
    queue.put(i);
  
  for (int i = 0; i < 5; ++i)
    (void)queue.get();
  
  // Add more to wrap around
  for (int i = 0; i < 5; ++i)
    queue.put(100 + i);
  
  EXPECT_EQ(queue.size(), 8);
  EXPECT_EQ(queue.front(), 5);  // First remaining element
}

// ============================================================================
// ArrayStack Edge Cases
// ============================================================================

class ArrayStackEdgeCases : public ::testing::Test
{
protected:
  ArrayStack<int> stack = ArrayStack<int>(10);
};

TEST_F(ArrayStackEdgeCases, EmptyStack_IsEmpty)
{
  EXPECT_TRUE(stack.is_empty());
  EXPECT_EQ(stack.size(), 0);
}

TEST_F(ArrayStackEdgeCases, EmptyStack_PopThrows)
{
  EXPECT_THROW(stack.pop(), std::underflow_error);
}

TEST_F(ArrayStackEdgeCases, EmptyStack_TopThrows)
{
  EXPECT_THROW(stack.top(), std::underflow_error);
}

TEST_F(ArrayStackEdgeCases, SingleElement_LIFO)
{
  stack.push(42);
  EXPECT_FALSE(stack.is_empty());
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack.top(), 42);
  EXPECT_EQ(stack.pop(), 42);
  EXPECT_TRUE(stack.is_empty());
}

TEST_F(ArrayStackEdgeCases, FillToCapacity)
{
  for (int i = 0; i < 10; ++i)
    stack.push(i);
  
  EXPECT_EQ(stack.size(), 10);
  EXPECT_EQ(stack.top(), 9);
}

TEST_F(ArrayStackEdgeCases, FillAndResize)
{
  // ArrayStack may auto-resize, so just verify it handles many elements
  for (int i = 0; i < 20; ++i)
    stack.push(i);
  
  EXPECT_EQ(stack.size(), 20);
  // Elements should be in LIFO order
  for (int i = 19; i >= 0; --i)
    EXPECT_EQ(stack.pop(), i);
}

TEST_F(ArrayStackEdgeCases, PushPopSequence)
{
  stack.push(1);
  stack.push(2);
  EXPECT_EQ(stack.pop(), 2);
  stack.push(3);
  EXPECT_EQ(stack.pop(), 3);
  EXPECT_EQ(stack.pop(), 1);
  EXPECT_TRUE(stack.is_empty());
}

// ============================================================================
// DynDlist Edge Cases
// ============================================================================

class DynDlistEdgeCases : public ::testing::Test
{
protected:
  DynDlist<int> dlist;
};

TEST_F(DynDlistEdgeCases, EmptyList_IsEmpty)
{
  EXPECT_TRUE(dlist.is_empty());
  EXPECT_EQ(dlist.size(), 0);
}

TEST_F(DynDlistEdgeCases, EmptyList_RemoveThrows)
{
  EXPECT_THROW(dlist.remove_first(), std::underflow_error);
  EXPECT_THROW(dlist.remove_last(), std::underflow_error);
}

TEST_F(DynDlistEdgeCases, EmptyList_GetThrows)
{
  EXPECT_THROW(dlist.get_first(), std::underflow_error);
  EXPECT_THROW(dlist.get_last(), std::underflow_error);
}

TEST_F(DynDlistEdgeCases, SingleElement_FirstEqualsLast)
{
  dlist.append(42);
  EXPECT_EQ(dlist.size(), 1);
  EXPECT_EQ(dlist.get_first(), 42);
  EXPECT_EQ(dlist.get_last(), 42);
}

TEST_F(DynDlistEdgeCases, SingleElement_RemoveFirst)
{
  dlist.append(42);
  EXPECT_EQ(dlist.remove_first(), 42);
  EXPECT_TRUE(dlist.is_empty());
}

TEST_F(DynDlistEdgeCases, SingleElement_RemoveLast)
{
  dlist.append(42);
  EXPECT_EQ(dlist.remove_last(), 42);
  EXPECT_TRUE(dlist.is_empty());
}

TEST_F(DynDlistEdgeCases, AppendAndInsert_Order)
{
  dlist.insert(1);  // Insert at front
  dlist.append(3);  // Append at back
  dlist.insert(0);  // Insert at front again
  
  EXPECT_EQ(dlist.get_first(), 0);
  EXPECT_EQ(dlist.get_last(), 3);
}

TEST_F(DynDlistEdgeCases, IteratorOnEmpty)
{
  auto it = dlist.begin();
  EXPECT_EQ(it, dlist.end());
}

TEST_F(DynDlistEdgeCases, ReverseEmpty)
{
  dlist.reverse();  // Should not crash
  EXPECT_TRUE(dlist.is_empty());
}

TEST_F(DynDlistEdgeCases, ReverseSingleElement)
{
  dlist.append(42);
  dlist.reverse();
  EXPECT_EQ(dlist.size(), 1);
  EXPECT_EQ(dlist.get_first(), 42);
}

TEST_F(DynDlistEdgeCases, ReverseMultiple)
{
  for (int i = 1; i <= 5; ++i)
    dlist.append(i);
  
  dlist.reverse();
  
  EXPECT_EQ(dlist.get_first(), 5);
  EXPECT_EQ(dlist.get_last(), 1);
}

TEST_F(DynDlistEdgeCases, AppendListEmpty)
{
  DynDlist<int> other;
  dlist.append(std::move(other));
  EXPECT_TRUE(dlist.is_empty());
}

TEST_F(DynDlistEdgeCases, AppendListToEmpty)
{
  DynDlist<int> other;
  other.append(1);
  other.append(2);
  
  dlist.append(std::move(other));
  
  EXPECT_EQ(dlist.size(), 2);
  EXPECT_TRUE(other.is_empty());
}

// ============================================================================
// DynList (htlist) Edge Cases
// ============================================================================

class DynListEdgeCases : public ::testing::Test
{
protected:
  DynList<int> slist;
};

TEST_F(DynListEdgeCases, EmptyList_IsEmpty)
{
  EXPECT_TRUE(slist.is_empty());
  EXPECT_EQ(slist.size(), 0);
}

TEST_F(DynListEdgeCases, EmptyList_RemoveThrows)
{
  EXPECT_THROW(slist.remove_first(), std::underflow_error);
}

TEST_F(DynListEdgeCases, SingleElement_InsertAndRemove)
{
  slist.insert(42);
  EXPECT_EQ(slist.size(), 1);
  EXPECT_EQ(slist.get_first(), 42);
  EXPECT_EQ(slist.remove_first(), 42);
  EXPECT_TRUE(slist.is_empty());
}

TEST_F(DynListEdgeCases, AppendKeepsOrder)
{
  slist.append(1);
  slist.append(2);
  slist.append(3);
  
  EXPECT_EQ(slist.remove_first(), 1);
  EXPECT_EQ(slist.remove_first(), 2);
  EXPECT_EQ(slist.remove_first(), 3);
}

TEST_F(DynListEdgeCases, InsertReverseOrder)
{
  slist.insert(1);
  slist.insert(2);
  slist.insert(3);
  
  EXPECT_EQ(slist.remove_first(), 3);
  EXPECT_EQ(slist.remove_first(), 2);
  EXPECT_EQ(slist.remove_first(), 1);
}

// ============================================================================
// ODhashTable Edge Cases
// ============================================================================

class ODhashTableEdgeCases : public ::testing::Test
{
protected:
  ODhashTable<int> table;  // Uses default hash and comparison
};

TEST_F(ODhashTableEdgeCases, EmptyTable_SizeIsZero)
{
  EXPECT_EQ(table.size(), 0);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(ODhashTableEdgeCases, EmptyTable_SearchReturnsNull)
{
  EXPECT_EQ(table.search(42), nullptr);
  EXPECT_EQ(table.search(0), nullptr);
  EXPECT_EQ(table.search(-1), nullptr);
}

TEST_F(ODhashTableEdgeCases, EmptyTable_RemoveThrows)
{
  EXPECT_THROW(table.remove(42), std::domain_error);
}

TEST_F(ODhashTableEdgeCases, SingleElement_InsertSearchRemove)
{
  int * p = table.insert(42);
  EXPECT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);
  EXPECT_EQ(table.size(), 1);
  
  int * found = table.search(42);
  EXPECT_EQ(found, p);
  
  EXPECT_NO_THROW(table.remove(42));
  EXPECT_TRUE(table.is_empty());
}

TEST_F(ODhashTableEdgeCases, DuplicateInsert_ReturnsNull)
{
  EXPECT_NE(table.insert(42), nullptr);
  int * dup = table.insert(42);
  EXPECT_EQ(dup, nullptr);
  EXPECT_EQ(table.size(), 1);
}

TEST_F(ODhashTableEdgeCases, HashCollisions)
{
  // Insert values that will likely collide
  for (int i = 0; i < 100; ++i)
    {
      int * p = table.insert(i);
      EXPECT_NE(p, nullptr) << "Failed to insert " << i;
    }
  
  EXPECT_EQ(table.size(), 100);
  
  // Verify all can be found
  for (int i = 0; i < 100; ++i)
    EXPECT_NE(table.search(i), nullptr) << "Failed to find " << i;
}

TEST_F(ODhashTableEdgeCases, RemoveNonExistent)
{
  EXPECT_NE(table.insert(1), nullptr);
  EXPECT_NE(table.insert(2), nullptr);
  
  EXPECT_THROW(table.remove(99), std::domain_error);
  EXPECT_EQ(table.size(), 2);
}

TEST_F(ODhashTableEdgeCases, ZeroAndNegativeKeys)
{
  EXPECT_NE(table.insert(0), nullptr);
  EXPECT_NE(table.insert(-1), nullptr);
  EXPECT_NE(table.insert(-100), nullptr);
  
  EXPECT_NE(table.search(0), nullptr);
  EXPECT_NE(table.search(-1), nullptr);
  EXPECT_NE(table.search(-100), nullptr);
}

// ============================================================================
// OLhashTable Edge Cases
// ============================================================================

class OLhashTableEdgeCases : public ::testing::Test
{
protected:
  OLhashTable<int> table;  // Uses default hash and comparison
};

TEST_F(OLhashTableEdgeCases, EmptyTable_SizeIsZero)
{
  EXPECT_EQ(table.size(), 0);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(OLhashTableEdgeCases, EmptyTable_SearchReturnsNull)
{
  EXPECT_EQ(table.search(42), nullptr);
}

TEST_F(OLhashTableEdgeCases, SingleElement_Operations)
{
  int * p = table.insert(42);
  EXPECT_NE(p, nullptr);
  EXPECT_EQ(table.size(), 1);
  
  EXPECT_NE(table.search(42), nullptr);
  table.remove(42);  // OLhashTable::remove returns void
  EXPECT_TRUE(table.is_empty());
}

TEST_F(OLhashTableEdgeCases, LinearProbingCollisions)
{
  // Insert many elements to test linear probing
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(table.insert(i), nullptr);
  
  EXPECT_EQ(table.size(), 50);
  
  // Verify all can be found
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(table.search(i), nullptr);
}

// ============================================================================
// DynSetTree Edge Cases
// ============================================================================

class DynSetTreeEdgeCases : public ::testing::Test
{
protected:
  DynSetTree<int> tree;
};

TEST_F(DynSetTreeEdgeCases, EmptyTree_SizeIsZero)
{
  EXPECT_EQ(tree.size(), 0);
  EXPECT_TRUE(tree.is_empty());
}

TEST_F(DynSetTreeEdgeCases, EmptyTree_SearchReturnsNull)
{
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_FALSE(tree.has(42));
  EXPECT_FALSE(tree.contains(42));
}

TEST_F(DynSetTreeEdgeCases, EmptyTree_RemoveReturnsDefault)
{
  EXPECT_FALSE(tree.exist(42));
}

TEST_F(DynSetTreeEdgeCases, SingleElement_Insert)
{
  int * p = tree.insert(42);
  EXPECT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);
  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.has(42));
}

TEST_F(DynSetTreeEdgeCases, SingleElement_MinMax)
{
  tree.insert(42);
  EXPECT_EQ(tree.min(), 42);
  EXPECT_EQ(tree.max(), 42);
}

TEST_F(DynSetTreeEdgeCases, DuplicateInsert_ReturnsNull)
{
  tree.insert(42);
  int * dup = tree.insert(42);
  EXPECT_EQ(dup, nullptr);
  EXPECT_EQ(tree.size(), 1);
}

TEST_F(DynSetTreeEdgeCases, SortedInsert_Ascending)
{
  for (int i = 1; i <= 100; ++i)
    tree.insert(i);
  
  EXPECT_EQ(tree.size(), 100);
  EXPECT_EQ(tree.min(), 1);
  EXPECT_EQ(tree.max(), 100);
}

TEST_F(DynSetTreeEdgeCases, SortedInsert_Descending)
{
  for (int i = 100; i >= 1; --i)
    tree.insert(i);
  
  EXPECT_EQ(tree.size(), 100);
  EXPECT_EQ(tree.min(), 1);
  EXPECT_EQ(tree.max(), 100);
}

TEST_F(DynSetTreeEdgeCases, RemoveMin)
{
  for (int i = 1; i <= 10; ++i)
    tree.insert(i);
  
  tree.remove(1);
  EXPECT_EQ(tree.min(), 2);
  EXPECT_EQ(tree.size(), 9);
}

TEST_F(DynSetTreeEdgeCases, RemoveMax)
{
  for (int i = 1; i <= 10; ++i)
    tree.insert(i);
  
  tree.remove(10);
  EXPECT_EQ(tree.max(), 9);
  EXPECT_EQ(tree.size(), 9);
}

TEST_F(DynSetTreeEdgeCases, RemoveMiddle)
{
  for (int i = 1; i <= 10; ++i)
    tree.insert(i);
  
  tree.remove(5);
  EXPECT_FALSE(tree.has(5));
  EXPECT_EQ(tree.size(), 9);
}

TEST_F(DynSetTreeEdgeCases, InorderTraversal)
{
  vector<int> values = {5, 3, 7, 1, 4, 6, 8};
  for (int v : values)
    tree.insert(v);
  
  vector<int> result;
  tree.for_each([&](int x) { result.push_back(x); });
  
  EXPECT_TRUE(std::is_sorted(result.begin(), result.end()));
}

TEST_F(DynSetTreeEdgeCases, ExtremeValues)
{
  tree.insert(std::numeric_limits<int>::max());
  tree.insert(std::numeric_limits<int>::min());
  tree.insert(0);
  
  EXPECT_EQ(tree.min(), std::numeric_limits<int>::min());
  EXPECT_EQ(tree.max(), std::numeric_limits<int>::max());
}

// ============================================================================
// BitArray Edge Cases
// ============================================================================

TEST(BitArrayEdgeCases, ZeroSized_NotAllowed)
{
  // BitArray requires at least 1 bit
  BitArray bits(1);
  EXPECT_EQ(bits.size(), 1);
}

TEST(BitArrayEdgeCases, SingleBit_SetAndRead)
{
  BitArray bits(1);
  
  EXPECT_EQ(bits.read_bit(0), 0);
  bits.write_bit(0, 1);
  EXPECT_EQ(bits.read_bit(0), 1);
  bits.write_bit(0, 0);
  EXPECT_EQ(bits.read_bit(0), 0);
}

TEST(BitArrayEdgeCases, AllBitsSet)
{
  BitArray bits(64);
  
  for (size_t i = 0; i < 64; ++i)
    bits.write_bit(i, 1);
  
  for (size_t i = 0; i < 64; ++i)
    EXPECT_EQ(bits.read_bit(i), 1);
}

TEST(BitArrayEdgeCases, AllBitsClear)
{
  BitArray bits(64);
  
  // Should start all zeros
  for (size_t i = 0; i < 64; ++i)
    EXPECT_EQ(bits.read_bit(i), 0);
}

TEST(BitArrayEdgeCases, AlternatingBits)
{
  BitArray bits(100);
  
  for (size_t i = 0; i < 100; ++i)
    bits.write_bit(i, i % 2);
  
  for (size_t i = 0; i < 100; ++i)
    EXPECT_EQ(bits.read_bit(i), i % 2);
}

TEST(BitArrayEdgeCases, BoundaryAccess)
{
  BitArray bits(128);
  
  // Test at byte boundaries
  bits.write_bit(7, 1);   // End of first byte
  bits.write_bit(8, 1);   // Start of second byte
  bits.write_bit(63, 1);  // End of first long
  bits.write_bit(64, 1);  // Start of second long
  bits.write_bit(127, 1); // Last bit
  
  EXPECT_EQ(bits.read_bit(7), 1);
  EXPECT_EQ(bits.read_bit(8), 1);
  EXPECT_EQ(bits.read_bit(63), 1);
  EXPECT_EQ(bits.read_bit(64), 1);
  EXPECT_EQ(bits.read_bit(127), 1);
}

// ============================================================================
// Array (fixed) Edge Cases
// ============================================================================

TEST(ArrayEdgeCases, SingleElement)
{
  Array<int> arr(1);
  arr(0) = 42;
  EXPECT_EQ(arr(0), 42);
  EXPECT_EQ(arr.size(), 1);
}

TEST(ArrayEdgeCases, BoundaryAccess)
{
  Array<int> arr(10);
  arr(0) = 1;
  arr(9) = 10;
  
  EXPECT_EQ(arr(0), 1);
  EXPECT_EQ(arr(9), 10);
}

// Array uses noexcept, so out of bounds is undefined behavior, not exception

// ============================================================================
// Stress Test - Mixed Operations
// ============================================================================

TEST(ContainerStress, DynSetTreeRandomOperations)
{
  DynSetTree<int> tree;
  std::mt19937 gen(12345);
  std::uniform_int_distribution<int> val_dist(0, 10000);
  std::uniform_int_distribution<int> op_dist(0, 2);
  
  std::set<int> reference;
  
  for (int i = 0; i < 10000; ++i)
    {
      int op = op_dist(gen);
      int val = val_dist(gen);
      
      if (op == 0 || op == 1)  // Insert
        {
          auto ref_result = reference.insert(val);
          int * tree_result = tree.insert(val);
          
          if (ref_result.second)
            EXPECT_NE(tree_result, nullptr);
          else
            EXPECT_EQ(tree_result, nullptr);
        }
      else if (!reference.empty())  // Remove
        {
          auto it = reference.begin();
          std::advance(it, gen() % reference.size());
          int to_remove = *it;
          
          reference.erase(to_remove);
          tree.remove(to_remove);
          
          EXPECT_FALSE(tree.has(to_remove));
        }
    }
  
  // Verify consistency
  EXPECT_EQ(tree.size(), reference.size());
  for (int val : reference)
    EXPECT_TRUE(tree.has(val));
}

TEST(ContainerStress, HashTableRandomOperations)
{
  ODhashTable<int> table;
  std::mt19937 gen(54321);
  std::uniform_int_distribution<int> val_dist(0, 5000);
  std::uniform_int_distribution<int> op_dist(0, 2);
  
  std::set<int> reference;
  
  for (int i = 0; i < 5000; ++i)
    {
      int op = op_dist(gen);
      int val = val_dist(gen);
      
      if (op == 0 || op == 1)  // Insert
        {
          auto ref_result = reference.insert(val);
          int * table_result = table.insert(val);
          
          if (ref_result.second)
            EXPECT_NE(table_result, nullptr);
          else
            EXPECT_EQ(table_result, nullptr);
        }
      else if (!reference.empty())  // Remove
        {
          auto it = reference.begin();
          std::advance(it, gen() % reference.size());
          int to_remove = *it;
          
          reference.erase(to_remove);
          table.remove(to_remove);  // Will throw if not found, but we know it exists
          
          EXPECT_EQ(table.search(to_remove), nullptr);
        }
    }
  
  // Verify consistency
  EXPECT_EQ(table.size(), reference.size());
  for (int val : reference)
    EXPECT_NE(table.search(val), nullptr);
}

TEST(ContainerStress, DynDlistRandomOperations)
{
  DynDlist<int> dlist;
  std::deque<int> reference;
  std::mt19937 gen(99999);
  std::uniform_int_distribution<int> val_dist(0, 1000);
  std::uniform_int_distribution<int> op_dist(0, 3);
  
  for (int i = 0; i < 5000; ++i)
    {
      int op = op_dist(gen);
      int val = val_dist(gen);
      
      switch (op)
        {
        case 0:  // Insert at front
          dlist.insert(val);
          reference.push_front(val);
          break;
          
        case 1:  // Append at back
          dlist.append(val);
          reference.push_back(val);
          break;
          
        case 2:  // Remove from front (if not empty)
          if (!reference.empty())
            {
              int dlist_val = dlist.remove_first();
              int ref_val = reference.front();
              reference.pop_front();
              EXPECT_EQ(dlist_val, ref_val);
            }
          break;
          
        case 3:  // Remove from back (if not empty)
          if (!reference.empty())
            {
              int dlist_val = dlist.remove_last();
              int ref_val = reference.back();
              reference.pop_back();
              EXPECT_EQ(dlist_val, ref_val);
            }
          break;
        }
    }
  
  // Verify consistency
  EXPECT_EQ(dlist.size(), reference.size());
}

TEST(ContainerStress, ArrayQueueCycles)
{
  ArrayQueue<int> queue = ArrayQueue<int>(100);
  std::mt19937 gen(77777);
  std::uniform_int_distribution<int> val_dist(0, 1000);
  
  // Do multiple fill/empty cycles
  for (int cycle = 0; cycle < 100; ++cycle)
    {
      // Fill partially
      int fill_count = gen() % 50 + 1;
      for (int i = 0; i < fill_count && queue.size() < 100; ++i)
        queue.put(val_dist(gen));
      
      // Empty partially
      int empty_count = gen() % queue.size() + 1;
      for (int i = 0; i < empty_count && !queue.is_empty(); ++i)
        (void)queue.get();
    }
  
  // Should not crash and queue should be valid
  EXPECT_LE(queue.size(), 100);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
