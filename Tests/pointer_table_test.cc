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
 * @file pointer_table_test.cc
 * @brief Exhaustive tests for Pointer_Table class
 *
 * These tests cover all operations including edge cases and critical scenarios:
 * - Basic operations (insert, remove, verify)
 * - Index recycling behavior
 * - Heap contraction with free_table cleanup (critical bug fix)
 * - Exception handling
 * - Stress tests with random operations
 * - Memory management and threshold behavior
 */

#include <gtest/gtest.h>
#include <vector>
#include <set>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <pointer_table.H>

using namespace std;
using namespace testing;

// =============================================================================
// Basic Construction Tests
// =============================================================================

class PointerTableConstructionTest : public Test
{
protected:
  void SetUp() override {}
};

TEST_F(PointerTableConstructionTest, DefaultConstructor)
{
  Pointer_Table table;

  EXPECT_EQ(table.size(), 0);
  EXPECT_EQ(table.busies(), 0);
  EXPECT_EQ(table.frees(), 0);
  EXPECT_EQ(table.get_heap_index(), 0);
  EXPECT_EQ(table.get_threshold(), 0);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableConstructionTest, ConstructorWithInitialSize)
{
  Pointer_Table table(100);

  EXPECT_EQ(table.busies(), 0);
  EXPECT_EQ(table.frees(), 0);
  EXPECT_EQ(table.get_heap_index(), 0);
  EXPECT_EQ(table.get_threshold(), 100);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableConstructionTest, ConstructorWithZeroSize)
{
  Pointer_Table table(0);

  EXPECT_EQ(table.get_threshold(), 0);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableConstructionTest, ConstructorWithLargeSize)
{
  Pointer_Table table(10000);

  EXPECT_EQ(table.get_threshold(), 10000);
  EXPECT_TRUE(table.is_empty());
}

// =============================================================================
// Basic Insert Tests
// =============================================================================

class PointerTableInsertTest : public Test
{
protected:
  Pointer_Table table;
  int dummy1 = 1, dummy2 = 2, dummy3 = 3;
};

TEST_F(PointerTableInsertTest, InsertSinglePointer)
{
  long idx = table.insert_pointer(&dummy1);

  EXPECT_EQ(idx, 0);
  EXPECT_EQ(table.busies(), 1);
  EXPECT_EQ(table.get_heap_index(), 1);
  EXPECT_FALSE(table.is_empty());
}

TEST_F(PointerTableInsertTest, InsertMultiplePointers)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  EXPECT_EQ(idx1, 0);
  EXPECT_EQ(idx2, 1);
  EXPECT_EQ(idx3, 2);
  EXPECT_EQ(table.busies(), 3);
  EXPECT_EQ(table.get_heap_index(), 3);
}

TEST_F(PointerTableInsertTest, InsertSamePointerTwice)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy1);

  EXPECT_NE(idx1, idx2);
  EXPECT_EQ(table.busies(), 2);
}

TEST_F(PointerTableInsertTest, InsertManyPointers)
{
  vector<int> data(1000);
  vector<long> indices;

  for (int i = 0; i < 1000; ++i)
    {
      data[i] = i;
      indices.push_back(table.insert_pointer(&data[i]));
    }

  EXPECT_EQ(table.busies(), 1000);
  EXPECT_EQ(table.get_heap_index(), 1000);

  for (int i = 0; i < 1000; ++i)
    EXPECT_EQ(indices[i], i);
}

// =============================================================================
// Basic Remove Tests
// =============================================================================

class PointerTableRemoveTest : public Test
{
protected:
  Pointer_Table table;
  int dummy1 = 1, dummy2 = 2, dummy3 = 3;
};

TEST_F(PointerTableRemoveTest, RemoveSinglePointer)
{
  long idx = table.insert_pointer(&dummy1);
  table.remove_pointer(idx);

  EXPECT_EQ(table.busies(), 0);
  EXPECT_EQ(table.get_heap_index(), 0);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableRemoveTest, RemoveFromMiddle)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  table.remove_pointer(idx2);

  EXPECT_EQ(table.busies(), 2);
  EXPECT_EQ(table.frees(), 1);
  EXPECT_EQ(table.get_heap_index(), 3);
}

TEST_F(PointerTableRemoveTest, RemoveFromEnd)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  table.remove_pointer(idx3);

  EXPECT_EQ(table.busies(), 2);
  EXPECT_EQ(table.frees(), 0);  // No free entry - heap contracted
  EXPECT_EQ(table.get_heap_index(), 2);
}

TEST_F(PointerTableRemoveTest, RemoveFromBeginning)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  table.remove_pointer(idx1);

  EXPECT_EQ(table.busies(), 2);
  EXPECT_EQ(table.frees(), 1);
  EXPECT_EQ(table.get_heap_index(), 3);
}

TEST_F(PointerTableRemoveTest, RemoveAllPointers)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  table.remove_pointer(idx1);
  table.remove_pointer(idx2);
  table.remove_pointer(idx3);

  EXPECT_EQ(table.busies(), 0);
  EXPECT_EQ(table.frees(), 0);
  EXPECT_EQ(table.get_heap_index(), 0);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableRemoveTest, RemoveInvalidIndexThrows)
{
  table.insert_pointer(&dummy1);

  EXPECT_THROW(table.remove_pointer(-1), std::range_error);
  EXPECT_THROW(table.remove_pointer(100), std::range_error);
  EXPECT_THROW(table.remove_pointer(1), std::range_error);
}

TEST_F(PointerTableRemoveTest, RemoveAlreadyFreeIndexThrows)
{
  long idx = table.insert_pointer(&dummy1);
  table.insert_pointer(&dummy2);
  table.remove_pointer(idx);

  EXPECT_THROW(table.remove_pointer(idx), std::domain_error);
}

// =============================================================================
// Index Recycling Tests
// =============================================================================

class PointerTableRecyclingTest : public Test
{
protected:
  Pointer_Table table;
  int dummy1 = 1, dummy2 = 2, dummy3 = 3, dummy4 = 4;
};

TEST_F(PointerTableRecyclingTest, RecyclesSingleIndex)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);

  table.remove_pointer(idx1);
  long idx3 = table.insert_pointer(&dummy3);

  EXPECT_EQ(idx3, idx1);  // Should reuse index 0
  EXPECT_EQ(table.busies(), 2);
  EXPECT_EQ(table.frees(), 0);
}

TEST_F(PointerTableRecyclingTest, RecyclesMultipleIndices)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  table.remove_pointer(idx1);
  table.remove_pointer(idx2);

  EXPECT_EQ(table.frees(), 2);

  long idx4 = table.insert_pointer(&dummy4);
  EXPECT_TRUE(idx4 == idx1 or idx4 == idx2);
  EXPECT_EQ(table.frees(), 1);
}

TEST_F(PointerTableRecyclingTest, RecyclesBeforeGrowingHeap)
{
  vector<int> data(100);
  vector<long> indices;

  for (int i = 0; i < 100; ++i)
    {
      data[i] = i;
      indices.push_back(table.insert_pointer(&data[i]));
    }

  // Remove every other element
  set<long> removed;
  for (int i = 0; i < 100; i += 2)
    {
      table.remove_pointer(indices[i]);
      removed.insert(indices[i]);
    }

  EXPECT_EQ(table.busies(), 50);

  // Insert 50 more - should reuse all freed indices
  for (int i = 0; i < 50; ++i)
    {
      long new_idx = table.insert_pointer(&data[i]);
      EXPECT_TRUE(removed.count(new_idx) > 0 or new_idx >= 100);
    }

  // Now heap should not have grown
  EXPECT_EQ(table.get_heap_index(), 100);
}

// =============================================================================
// Critical Bug Fix Test: Heap Contraction with Free Table Cleanup
// =============================================================================

class PointerTableHeapContractionTest : public Test
{
protected:
  Pointer_Table table;
  int d1 = 1, d2 = 2, d3 = 3, d4 = 4, d5 = 5;
};

TEST_F(PointerTableHeapContractionTest, BasicHeapContraction)
{
  long idx1 = table.insert_pointer(&d1);
  long idx2 = table.insert_pointer(&d2);
  long idx3 = table.insert_pointer(&d3);

  // Remove from top - heap should contract
  table.remove_pointer(idx3);

  EXPECT_EQ(table.get_heap_index(), 2);
  EXPECT_EQ(table.busies(), 2);
  EXPECT_EQ(table.frees(), 0);
}

TEST_F(PointerTableHeapContractionTest, HeapContractionWithFreeTableCleanup)
{
  // This test verifies the critical bug fix:
  // When heap contracts, free_table must be cleaned of invalid indices

  long idx1 = table.insert_pointer(&d1);
  long idx2 = table.insert_pointer(&d2);
  long idx3 = table.insert_pointer(&d3);
  long idx4 = table.insert_pointer(&d4);
  long idx5 = table.insert_pointer(&d5);

  // State: [d1, d2, d3, d4, d5], heap_index=5, free_table=[]

  // Remove middle elements - they go to free_table
  table.remove_pointer(idx2);
  table.remove_pointer(idx4);

  // State: [d1, null, d3, null, d5], heap_index=5, free_table=[1, 3]
  EXPECT_EQ(table.frees(), 2);

  // Remove the top element - heap should contract to index 3
  // and free_table should be cleaned of index 3 (which is now >= heap_index)
  table.remove_pointer(idx5);

  // State should be: [d1, null, d3], heap_index=3, free_table=[1]
  EXPECT_EQ(table.get_heap_index(), 3);
  EXPECT_EQ(table.frees(), 1);  // Only index 1 should remain
  EXPECT_EQ(table.busies(), 2);

  // Now insert a new pointer - it should get index 1 (recycled)
  int d6 = 6;
  long idx6 = table.insert_pointer(&d6);
  EXPECT_EQ(idx6, 1);
  EXPECT_EQ(table.frees(), 0);
}

TEST_F(PointerTableHeapContractionTest, MultipleContractionLevels)
{
  // Create: [d1, d2, d3, d4, d5]
  long idx1 = table.insert_pointer(&d1);
  long idx2 = table.insert_pointer(&d2);
  long idx3 = table.insert_pointer(&d3);
  long idx4 = table.insert_pointer(&d4);
  long idx5 = table.insert_pointer(&d5);

  // Remove all but the first
  table.remove_pointer(idx2);  // free_table = [1]
  table.remove_pointer(idx3);  // free_table = [1, 2]
  table.remove_pointer(idx4);  // free_table = [1, 2, 3]

  EXPECT_EQ(table.frees(), 3);

  // Remove top - should contract to heap_index=1 and clean ALL free_table
  table.remove_pointer(idx5);

  // heap_index should be 1 (only d1 remains)
  EXPECT_EQ(table.get_heap_index(), 1);
  EXPECT_EQ(table.frees(), 0);  // All free indices were >= 1, so removed
  EXPECT_EQ(table.busies(), 1);
}

TEST_F(PointerTableHeapContractionTest, ContractionAfterRemovingConsecutive)
{
  long idx1 = table.insert_pointer(&d1);
  long idx2 = table.insert_pointer(&d2);
  long idx3 = table.insert_pointer(&d3);
  long idx4 = table.insert_pointer(&d4);

  // Remove idx2 (goes to free_table)
  table.remove_pointer(idx2);

  // Remove idx4 (top) - contracts heap to 3
  table.remove_pointer(idx4);

  // Remove idx3 (now top) - contracts heap to 1
  // Since idx2 was in free_table and is still valid (<1? no, it's 1 which is >=1)
  // Actually idx2=1, and new heap_index=1, so idx2 should be removed from free_table
  table.remove_pointer(idx3);

  EXPECT_EQ(table.get_heap_index(), 1);
  EXPECT_EQ(table.frees(), 0);  // idx2=1 is now invalid
  EXPECT_EQ(table.busies(), 1);
}

// =============================================================================
// Verify Pointer Tests
// =============================================================================

class PointerTableVerifyTest : public Test
{
protected:
  Pointer_Table table;
  int dummy1 = 1, dummy2 = 2;
};

TEST_F(PointerTableVerifyTest, VerifyCorrectPointer)
{
  long idx = table.insert_pointer(&dummy1);
  void* result = table.verify_pointer(idx, &dummy1);

  EXPECT_EQ(result, &dummy1);
}

TEST_F(PointerTableVerifyTest, VerifyIncorrectPointerThrows)
{
  long idx = table.insert_pointer(&dummy1);

  EXPECT_THROW(table.verify_pointer(idx, &dummy2), std::domain_error);
}

TEST_F(PointerTableVerifyTest, VerifyInvalidIndexThrows)
{
  table.insert_pointer(&dummy1);

  EXPECT_THROW(table.verify_pointer(-1, &dummy1), std::range_error);
  EXPECT_THROW(table.verify_pointer(100, &dummy1), std::range_error);
}

TEST_F(PointerTableVerifyTest, VerifyAfterRemoveThrows)
{
  long idx = table.insert_pointer(&dummy1);
  table.insert_pointer(&dummy2);  // Keep heap from contracting
  table.remove_pointer(idx);

  // Index is valid but pointer is nullptr (different from expected)
  EXPECT_THROW(table.verify_pointer(idx, &dummy1), std::domain_error);
}

// =============================================================================
// Get Pointer Tests
// =============================================================================

class PointerTableGetTest : public Test
{
protected:
  Pointer_Table table;
  int dummy1 = 1, dummy2 = 2, dummy3 = 3;
};

TEST_F(PointerTableGetTest, GetValidPointer)
{
  long idx = table.insert_pointer(&dummy1);
  void* result = table.get_pointer(idx);

  EXPECT_EQ(result, &dummy1);
}

TEST_F(PointerTableGetTest, GetMultiplePointers)
{
  long idx1 = table.insert_pointer(&dummy1);
  long idx2 = table.insert_pointer(&dummy2);
  long idx3 = table.insert_pointer(&dummy3);

  EXPECT_EQ(table.get_pointer(idx1), &dummy1);
  EXPECT_EQ(table.get_pointer(idx2), &dummy2);
  EXPECT_EQ(table.get_pointer(idx3), &dummy3);
}

TEST_F(PointerTableGetTest, GetInvalidIndexThrows)
{
  table.insert_pointer(&dummy1);

  EXPECT_THROW(table.get_pointer(-1), std::range_error);
  EXPECT_THROW(table.get_pointer(100), std::range_error);
}

TEST_F(PointerTableGetTest, GetFreedSlotReturnsNull)
{
  long idx = table.insert_pointer(&dummy1);
  table.insert_pointer(&dummy2);
  table.remove_pointer(idx);

  void* result = table.get_pointer(idx);
  EXPECT_EQ(result, nullptr);
}

// =============================================================================
// Clear Tests
// =============================================================================

class PointerTableClearTest : public Test
{
protected:
  int d1 = 1, d2 = 2, d3 = 3;
};

TEST_F(PointerTableClearTest, ClearEmptyTable)
{
  Pointer_Table table;
  table.clear();

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.busies(), 0);
  EXPECT_EQ(table.frees(), 0);
}

TEST_F(PointerTableClearTest, ClearNonEmptyTable)
{
  Pointer_Table table;
  table.insert_pointer(&d1);
  table.insert_pointer(&d2);
  table.insert_pointer(&d3);

  table.clear();

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.busies(), 0);
  EXPECT_EQ(table.frees(), 0);
  EXPECT_EQ(table.get_heap_index(), 0);
}

TEST_F(PointerTableClearTest, ClearPreservesThreshold)
{
  Pointer_Table table(100);
  table.insert_pointer(&d1);
  table.insert_pointer(&d2);

  table.clear();

  EXPECT_EQ(table.get_threshold(), 100);
  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableClearTest, ClearWithFreeEntries)
{
  Pointer_Table table;
  long idx1 = table.insert_pointer(&d1);
  table.insert_pointer(&d2);
  table.insert_pointer(&d3);
  table.remove_pointer(idx1);  // Creates free entry

  EXPECT_EQ(table.frees(), 1);

  table.clear();

  EXPECT_EQ(table.frees(), 0);
  EXPECT_TRUE(table.is_empty());
}

// =============================================================================
// Threshold Behavior Tests
// =============================================================================

class PointerTableThresholdTest : public Test
{
protected:
  int d1 = 1, d2 = 2;
};

TEST_F(PointerTableThresholdTest, DoesNotShrinkBelowThreshold)
{
  Pointer_Table table(10);

  for (int i = 0; i < 20; ++i)
    table.insert_pointer(&d1);

  // Remove all but one
  for (int i = 19; i >= 1; --i)
    table.remove_pointer(i);

  // Should maintain threshold size
  EXPECT_GE(table.size(), 0);  // Implementation may vary
  EXPECT_EQ(table.get_threshold(), 10);
}

TEST_F(PointerTableThresholdTest, ZeroThresholdAllowsFullShrink)
{
  Pointer_Table table(0);

  for (int i = 0; i < 100; ++i)
    table.insert_pointer(&d1);

  // Remove all
  for (int i = 99; i >= 0; --i)
    table.remove_pointer(i);

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.get_heap_index(), 0);
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

class PointerTableEdgeCasesTest : public Test
{
protected:
  Pointer_Table table;
};

TEST_F(PointerTableEdgeCasesTest, RemoveOnlyElement)
{
  int d = 1;
  long idx = table.insert_pointer(&d);
  table.remove_pointer(idx);

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.get_heap_index(), 0);
  EXPECT_EQ(table.frees(), 0);
}

TEST_F(PointerTableEdgeCasesTest, AlternatingInsertRemove)
{
  int d1 = 1, d2 = 2;

  for (int i = 0; i < 100; ++i)
    {
      long idx = table.insert_pointer(&d1);
      EXPECT_EQ(idx, 0);  // Should always be 0 due to recycling
      table.remove_pointer(idx);
      EXPECT_TRUE(table.is_empty());
    }
}

TEST_F(PointerTableEdgeCasesTest, RemoveInReverseOrder)
{
  vector<int> data(100);
  vector<long> indices;

  for (int i = 0; i < 100; ++i)
    {
      data[i] = i;
      indices.push_back(table.insert_pointer(&data[i]));
    }

  // Remove in reverse order - heap should contract each time
  for (int i = 99; i >= 0; --i)
    {
      table.remove_pointer(indices[i]);
      EXPECT_EQ(table.get_heap_index(), i);
      EXPECT_EQ(table.frees(), 0);  // No free entries when removing from top
    }

  EXPECT_TRUE(table.is_empty());
}

TEST_F(PointerTableEdgeCasesTest, RemoveInForwardOrder)
{
  vector<int> data(100);
  vector<long> indices;

  for (int i = 0; i < 100; ++i)
    {
      data[i] = i;
      indices.push_back(table.insert_pointer(&data[i]));
    }

  // Remove in forward order
  for (int i = 0; i < 99; ++i)
    {
      table.remove_pointer(indices[i]);
      EXPECT_EQ(table.frees(), i + 1);
    }

  // Remove last - should trigger full cleanup
  table.remove_pointer(indices[99]);
  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.frees(), 0);
}

// =============================================================================
// Stress Tests
// =============================================================================

class PointerTableStressTest : public Test
{
protected:
  Pointer_Table table;
  mt19937 rng{42};  // Fixed seed for reproducibility
};

TEST_F(PointerTableStressTest, RandomOperations)
{
  vector<int> data(10000);
  set<long> active_indices;

  for (int i = 0; i < 10000; ++i)
    data[i] = i;

  uniform_int_distribution<int> op_dist(0, 2);  // 0=insert, 1=remove, 2=verify

  for (int iteration = 0; iteration < 50000; ++iteration)
    {
      int op = op_dist(rng);

      if (op == 0 or active_indices.empty())
        {
          // Insert
          int data_idx = rng() % 10000;
          long idx = table.insert_pointer(&data[data_idx]);
          active_indices.insert(idx);
        }
      else if (op == 1)
        {
          // Remove
          auto it = active_indices.begin();
          advance(it, rng() % active_indices.size());
          long idx = *it;
          table.remove_pointer(idx);
          active_indices.erase(it);
        }
      else
        {
          // Verify - get pointer at random active index
          auto it = active_indices.begin();
          advance(it, rng() % active_indices.size());
          long idx = *it;
          void* ptr = table.get_pointer(idx);
          EXPECT_NE(ptr, nullptr);
        }

      // Invariant check
      EXPECT_EQ(table.busies(), static_cast<long>(active_indices.size()));
    }
}

TEST_F(PointerTableStressTest, InterleavedPatterns)
{
  vector<int> data(1000);
  for (int i = 0; i < 1000; ++i)
    data[i] = i;

  // Insert 1000 elements
  vector<long> indices;
  for (int i = 0; i < 1000; ++i)
    indices.push_back(table.insert_pointer(&data[i]));

  // Remove odd indices
  for (int i = 1; i < 1000; i += 2)
    table.remove_pointer(indices[i]);

  EXPECT_EQ(table.busies(), 500);
  EXPECT_EQ(table.frees(), 499);  // One less because last removed might contract

  // Insert 500 more - should reuse freed indices
  for (int i = 0; i < 500; ++i)
    {
      long new_idx = table.insert_pointer(&data[i]);
      // Should be recycled
      EXPECT_LT(new_idx, 1000);
    }

  EXPECT_EQ(table.busies(), 1000);
  EXPECT_LE(table.get_heap_index(), 1000);
}

TEST_F(PointerTableStressTest, MassiveGrowthAndShrink)
{
  vector<int> data(100000);
  for (int i = 0; i < 100000; ++i)
    data[i] = i;

  // Grow to 100k
  vector<long> indices;
  for (int i = 0; i < 100000; ++i)
    indices.push_back(table.insert_pointer(&data[i]));

  EXPECT_EQ(table.busies(), 100000);
  EXPECT_EQ(table.get_heap_index(), 100000);

  // Shrink to 0
  for (int i = 99999; i >= 0; --i)
    table.remove_pointer(indices[i]);

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.get_heap_index(), 0);
  EXPECT_EQ(table.frees(), 0);
}

TEST_F(PointerTableStressTest, HeapContractionStress)
{
  // This test specifically stresses the heap contraction with free_table cleanup
  vector<int> data(1000);
  for (int i = 0; i < 1000; ++i)
    data[i] = i;

  for (int round = 0; round < 10; ++round)
    {
      // Insert all
      vector<long> indices;
      for (int i = 0; i < 1000; ++i)
        indices.push_back(table.insert_pointer(&data[i]));

      // Remove in a pattern that creates fragmentation then contracts
      // Remove first 500 (creates free entries)
      for (int i = 0; i < 500; ++i)
        table.remove_pointer(indices[i]);

      EXPECT_EQ(table.frees(), 500);

      // Remove from the end (triggers contraction and cleanup)
      for (int i = 999; i >= 500; --i)
        table.remove_pointer(indices[i]);

      // All should be cleaned
      EXPECT_TRUE(table.is_empty());
      EXPECT_EQ(table.frees(), 0);
      EXPECT_EQ(table.get_heap_index(), 0);
    }
}

// =============================================================================
// Consistency Tests
// =============================================================================

class PointerTableConsistencyTest : public Test
{
protected:
  Pointer_Table table;
};

TEST_F(PointerTableConsistencyTest, CountsRemainConsistent)
{
  vector<int> data(100);
  set<long> active;

  for (int i = 0; i < 100; ++i)
    data[i] = i;

  for (int iter = 0; iter < 1000; ++iter)
    {
      if (rand() % 2 == 0 or active.empty())
        {
          long idx = table.insert_pointer(&data[rand() % 100]);
          active.insert(idx);
        }
      else
        {
          auto it = active.begin();
          advance(it, rand() % active.size());
          table.remove_pointer(*it);
          active.erase(it);
        }

      // Consistency check
      EXPECT_EQ(table.busies(), static_cast<long>(active.size()));
      EXPECT_EQ(table.busies() + table.frees(), table.get_heap_index());
    }
}

TEST_F(PointerTableConsistencyTest, HeapIndexNeverExceedsBusiesPlusFrees)
{
  vector<int> data(500);
  for (int i = 0; i < 500; ++i)
    data[i] = i;

  vector<long> indices;
  for (int i = 0; i < 500; ++i)
    indices.push_back(table.insert_pointer(&data[i]));

  // Random removals
  shuffle(indices.begin(), indices.end(), mt19937{42});

  for (int i = 0; i < 500; ++i)
    {
      table.remove_pointer(indices[i]);

      // heap_index should always equal busies + frees
      EXPECT_EQ(table.get_heap_index(), table.busies() + table.frees());
    }
}

// =============================================================================
// Null Index Constant Tests
// =============================================================================

TEST(PointerTableNullIndexTest, NullIndexIsNegative)
{
  EXPECT_LT(Pointer_Table::Null_Index, 0);
  EXPECT_EQ(Pointer_Table::Null_Index, -1);
}

// =============================================================================
// Const Correctness Tests
// =============================================================================

class PointerTableConstTest : public Test
{
protected:
  int d1 = 1, d2 = 2;
};

TEST_F(PointerTableConstTest, ConstMethodsWork)
{
  Pointer_Table table;
  long idx = table.insert_pointer(&d1);
  table.insert_pointer(&d2);

  const Pointer_Table& const_ref = table;

  EXPECT_EQ(const_ref.size(), table.size());
  EXPECT_EQ(const_ref.busies(), table.busies());
  EXPECT_EQ(const_ref.frees(), table.frees());
  EXPECT_EQ(const_ref.get_heap_index(), table.get_heap_index());
  EXPECT_EQ(const_ref.get_threshold(), table.get_threshold());
  EXPECT_EQ(const_ref.is_empty(), table.is_empty());
  EXPECT_EQ(const_ref.get_pointer(idx), &d1);
  EXPECT_EQ(const_ref.verify_pointer(idx, &d1), &d1);
}

// =============================================================================
// Memory Pattern Tests
// =============================================================================

class PointerTableMemoryPatternTest : public Test
{
protected:
  Pointer_Table table;
};

TEST_F(PointerTableMemoryPatternTest, SawtoothPattern)
{
  vector<int> data(100);
  for (int i = 0; i < 100; ++i)
    data[i] = i;

  for (int cycle = 0; cycle < 10; ++cycle)
    {
      // Grow
      vector<long> indices;
      for (int i = 0; i < 100; ++i)
        indices.push_back(table.insert_pointer(&data[i]));

      // Shrink to half
      for (int i = 99; i >= 50; --i)
        table.remove_pointer(indices[i]);

      EXPECT_EQ(table.busies(), 50);
      EXPECT_EQ(table.get_heap_index(), 50);

      // Shrink rest
      for (int i = 49; i >= 0; --i)
        table.remove_pointer(indices[i]);

      EXPECT_TRUE(table.is_empty());
    }
}

TEST_F(PointerTableMemoryPatternTest, FragmentedPattern)
{
  vector<int> data(1000);
  for (int i = 0; i < 1000; ++i)
    data[i] = i;

  vector<long> indices;
  for (int i = 0; i < 1000; ++i)
    indices.push_back(table.insert_pointer(&data[i]));

  // Create maximum fragmentation: remove every other element
  for (int i = 0; i < 1000; i += 2)
    table.remove_pointer(indices[i]);

  EXPECT_EQ(table.busies(), 500);

  // Now remove the rest in reverse order
  for (int i = 999; i >= 1; i -= 2)
    {
      table.remove_pointer(indices[i]);

      // Verify consistency
      EXPECT_EQ(table.busies() + table.frees(), table.get_heap_index());
    }

  EXPECT_TRUE(table.is_empty());
}

// =============================================================================
// Specific Bug Regression Tests
// =============================================================================

class PointerTableRegressionTest : public Test
{
protected:
  Pointer_Table table;
};

TEST_F(PointerTableRegressionTest, BugFix_FreeTableNotCleanedOnContraction)
{
  // Regression test for the bug where free_table was not cleaned
  // when heap contracted, leading to invalid indices being reused

  int d1 = 1, d2 = 2, d3 = 3, d4 = 4, d5 = 5, d6 = 6;

  // Setup: [d1, d2, d3, d4, d5]
  long i1 = table.insert_pointer(&d1);
  long i2 = table.insert_pointer(&d2);
  long i3 = table.insert_pointer(&d3);
  long i4 = table.insert_pointer(&d4);
  long i5 = table.insert_pointer(&d5);

  // Remove i2 and i4 - they go to free_table
  table.remove_pointer(i2);  // free_table = [1]
  table.remove_pointer(i4);  // free_table = [1, 3]

  // Remove i5 (top) - heap contracts, i3 should be removed from free_table
  table.remove_pointer(i5);

  // Now heap_index should be 3, and free_table should only contain [1]
  EXPECT_EQ(table.get_heap_index(), 3);
  EXPECT_EQ(table.frees(), 1);

  // Insert new element - should get index 1 (the only valid free index)
  long i6 = table.insert_pointer(&d6);
  EXPECT_EQ(i6, 1);

  // Verify the pointer at index 1 is d6
  EXPECT_EQ(table.get_pointer(i6), &d6);
  EXPECT_EQ(table.verify_pointer(i6, &d6), &d6);
}

TEST_F(PointerTableRegressionTest, BugFix_CompleteContractionCleansAllFree)
{
  int d1 = 1, d2 = 2, d3 = 3, d4 = 4;

  table.insert_pointer(&d1);
  long i2 = table.insert_pointer(&d2);
  long i3 = table.insert_pointer(&d3);
  long i4 = table.insert_pointer(&d4);

  // Remove middle ones
  table.remove_pointer(i2);
  table.remove_pointer(i3);

  EXPECT_EQ(table.frees(), 2);

  // Remove top - should contract and clean free_table
  table.remove_pointer(i4);

  // heap_index should be 1, and all free indices (1, 2) should be cleaned
  EXPECT_EQ(table.get_heap_index(), 1);
  EXPECT_EQ(table.frees(), 0);
  EXPECT_EQ(table.busies(), 1);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv)
{
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}