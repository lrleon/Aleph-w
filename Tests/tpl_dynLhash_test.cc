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
 * @file tpl_dynLhash_test.cc
 * @brief Comprehensive tests for DynLhashTable (dynamic hash table with chaining)
 */

#include <gtest/gtest.h>
#include <tpl_dynLhash.H>
#include <string>
#include <vector>
#include <random>

using namespace Aleph;

// =============================================================================
// Basic Functionality Tests
// =============================================================================

class DynLhashTableTest : public ::testing::Test
{
protected:
  DynLhashTable<int, std::string> table;
};

TEST_F(DynLhashTableTest, InsertAndSearch)
{
  auto* ptr = table.insert(1, "one");
  
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, "one");
  
  auto* found = table.search(1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*found, "one");
}

TEST_F(DynLhashTableTest, SearchNonExistent)
{
  table.insert(1, "one");
  
  auto* found = table.search(2);
  EXPECT_EQ(found, nullptr);
}

TEST_F(DynLhashTableTest, InsertMultiple)
{
  table.insert(1, "one");
  table.insert(2, "two");
  table.insert(3, "three");
  
  EXPECT_EQ(*table.search(1), "one");
  EXPECT_EQ(*table.search(2), "two");
  EXPECT_EQ(*table.search(3), "three");
}

TEST_F(DynLhashTableTest, Remove)
{
  auto* ptr = table.insert(1, "one");
  table.insert(2, "two");
  
  table.remove(ptr);
  
  EXPECT_EQ(table.search(1), nullptr);
  EXPECT_NE(table.search(2), nullptr);
}

TEST_F(DynLhashTableTest, UpdateExisting)
{
  table.insert(1, "one");
  
  auto* ptr = table.search(1);
  ASSERT_NE(ptr, nullptr);
  *ptr = "ONE";
  
  EXPECT_EQ(*table.search(1), "ONE");
}

// =============================================================================
// Operator[] Tests
// =============================================================================

TEST_F(DynLhashTableTest, BracketOperatorInsert)
{
  table[1] = "one";
  
  auto* found = table.search(1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*found, "one");
}

TEST_F(DynLhashTableTest, BracketOperatorUpdate)
{
  table[1] = "one";
  table[1] = "ONE";
  
  EXPECT_EQ(*table.search(1), "ONE");
}

TEST_F(DynLhashTableTest, BracketOperatorReadExisting)
{
  table.insert(1, "one");
  
  std::string val = table[1];
  EXPECT_EQ(val, "one");
}

TEST_F(DynLhashTableTest, BracketOperatorReadNonExisting)
{
  table.insert(1, "one");
  
  // Reading non-existent key should throw
  EXPECT_THROW({
    std::string val = table[2];
  }, std::invalid_argument);
}

// =============================================================================
// Copy and Move Semantics Tests
// =============================================================================

TEST_F(DynLhashTableTest, CopyConstructor)
{
  table.insert(1, "one");
  table.insert(2, "two");
  
  DynLhashTable<int, std::string> copy(table);
  
  EXPECT_EQ(*copy.search(1), "one");
  EXPECT_EQ(*copy.search(2), "two");
  
  // Modify original, copy should be independent
  *table.search(1) = "ONE";
  EXPECT_EQ(*copy.search(1), "one");
}

TEST_F(DynLhashTableTest, CopyAssignment)
{
  table.insert(1, "one");
  table.insert(2, "two");
  
  DynLhashTable<int, std::string> other;
  other.insert(3, "three");
  
  other = table;
  
  EXPECT_EQ(*other.search(1), "one");
  EXPECT_EQ(*other.search(2), "two");
  EXPECT_EQ(other.search(3), nullptr);
}

TEST_F(DynLhashTableTest, MoveConstructor)
{
  table.insert(1, "one");
  table.insert(2, "two");
  
  DynLhashTable<int, std::string> moved(std::move(table));
  
  EXPECT_EQ(*moved.search(1), "one");
  EXPECT_EQ(*moved.search(2), "two");
}

TEST_F(DynLhashTableTest, MoveAssignment)
{
  table.insert(1, "one");
  table.insert(2, "two");
  
  DynLhashTable<int, std::string> other;
  other = std::move(table);
  
  EXPECT_EQ(*other.search(1), "one");
  EXPECT_EQ(*other.search(2), "two");
}

TEST_F(DynLhashTableTest, SelfAssignment)
{
  table.insert(1, "one");
  
  table = table;  // Self-assignment
  
  EXPECT_EQ(*table.search(1), "one");
}

// =============================================================================
// String Key Tests
// =============================================================================

TEST(DynLhashTableStringKey, BasicOperations)
{
  DynLhashTable<std::string, int> table;
  
  table.insert("one", 1);
  table.insert("two", 2);
  table.insert("three", 3);
  
  EXPECT_EQ(*table.search("one"), 1);
  EXPECT_EQ(*table.search("two"), 2);
  EXPECT_EQ(*table.search("three"), 3);
  EXPECT_EQ(table.search("four"), nullptr);
}

TEST(DynLhashTableStringKey, LongStrings)
{
  DynLhashTable<std::string, int> table;
  
  std::string longKey(1000, 'x');
  table.insert(longKey, 42);
  
  EXPECT_EQ(*table.search(longKey), 42);
}

// =============================================================================
// Collision Handling Tests
// =============================================================================

TEST(DynLhashTableCollision, SameHashBucket)
{
  // Use small table size to force collisions
  DynLhashTable<int, std::string> table(7);
  
  // Insert values that will likely collide
  for (int i = 0; i < 100; ++i)
    table.insert(i, "val" + std::to_string(i));
  
  // Verify all values are retrievable
  for (int i = 0; i < 100; ++i)
    EXPECT_EQ(*table.search(i), "val" + std::to_string(i));
}

// =============================================================================
// Move Semantics for Values
// =============================================================================

TEST(DynLhashTableMoveValue, MoveInsert)
{
  DynLhashTable<int, std::string> table;
  
  std::string value = "hello";
  table.insert(1, std::move(value));
  
  EXPECT_EQ(*table.search(1), "hello");
  // value should be moved-from (possibly empty)
}

TEST(DynLhashTableMoveValue, MoveKeyAndValue)
{
  DynLhashTable<std::string, std::string> table;
  
  std::string key = "key";
  std::string value = "value";
  table.insert(std::move(key), std::move(value));
  
  EXPECT_EQ(*table.search("key"), "value");
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DynLhashTableStress, ManyInsertions)
{
  DynLhashTable<int, int> table;
  
  const int N = 10000;
  for (int i = 0; i < N; ++i)
    table.insert(i, i * 2);
  
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(*table.search(i), i * 2);
}

TEST(DynLhashTableStress, ManyInsertionsAndRemovals)
{
  DynLhashTable<int, int> table;
  
  const int N = 1000;
  std::vector<int*> pointers;
  
  // Insert
  for (int i = 0; i < N; ++i)
    pointers.push_back(table.insert(i, i));
  
  // Remove half
  for (int i = 0; i < N / 2; ++i)
    table.remove(pointers[i]);
  
  // Verify remaining
  for (int i = N / 2; i < N; ++i)
    EXPECT_EQ(*table.search(i), i);
  
  // Verify removed are gone
  for (int i = 0; i < N / 2; ++i)
    EXPECT_EQ(table.search(i), nullptr);
}

TEST(DynLhashTableStress, RandomOperations)
{
  DynLhashTable<int, int> table;
  std::map<int, int*> reference;  // Track what's in the table
  
  std::mt19937 rng(42);
  std::uniform_int_distribution<> key_dist(0, 999);
  std::uniform_int_distribution<> op_dist(0, 2);
  
  for (int iter = 0; iter < 5000; ++iter)
    {
      int key = key_dist(rng);
      int op = op_dist(rng);
      
      if (op == 0)  // Insert
        {
          auto* ptr = table.insert(key, key * 2);
          reference[key] = ptr;
        }
      else if (op == 1)  // Search
        {
          auto* found = table.search(key);
          if (reference.count(key))
            {
              ASSERT_NE(found, nullptr);
              EXPECT_EQ(*found, key * 2);
            }
        }
      else  // Remove
        {
          if (reference.count(key))
            {
              table.remove(reference[key]);
              reference.erase(key);
            }
        }
    }
  
  // Final verification
  for (auto& [key, ptr] : reference)
    {
      auto* found = table.search(key);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(*found, key * 2);
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(DynLhashTableTest, EmptyTable)
{
  EXPECT_EQ(table.search(1), nullptr);
  EXPECT_EQ(table.search(0), nullptr);
  EXPECT_EQ(table.search(-1), nullptr);
}

TEST_F(DynLhashTableTest, NegativeKeys)
{
  table.insert(-1, "negative one");
  table.insert(-100, "negative hundred");
  table.insert(0, "zero");
  
  EXPECT_EQ(*table.search(-1), "negative one");
  EXPECT_EQ(*table.search(-100), "negative hundred");
  EXPECT_EQ(*table.search(0), "zero");
}

TEST_F(DynLhashTableTest, SameKeyMultipleInserts)
{
  table.insert(1, "first");
  table.insert(1, "second");  // Same key, different value
  
  // Behavior depends on implementation - usually both are stored
  // or second replaces first. Search returns one of them.
  auto* found = table.search(1);
  ASSERT_NE(found, nullptr);
  // At minimum, we should find SOMETHING for key 1
}

// =============================================================================
// Custom Hash Function
// =============================================================================

size_t custom_hash(const int& key)
{
  return static_cast<size_t>(key * 31 + 17);
}

TEST(DynLhashTableCustomHash, WithCustomHashFunction)
{
  DynLhashTable<int, std::string> table(101, custom_hash);
  
  table.insert(1, "one");
  table.insert(2, "two");
  
  EXPECT_EQ(*table.search(1), "one");
  EXPECT_EQ(*table.search(2), "two");
}

// =============================================================================
// Swap Tests
// =============================================================================

TEST(DynLhashTableSwap, SwapTables)
{
  DynLhashTable<int, std::string> table1;
  DynLhashTable<int, std::string> table2;
  
  table1.insert(1, "one");
  table1.insert(2, "two");
  table2.insert(10, "ten");
  
  table1.swap(table2);
  
  // table1 should now have table2's contents
  EXPECT_EQ(table1.search(1), nullptr);
  EXPECT_EQ(table1.search(2), nullptr);
  EXPECT_EQ(*table1.search(10), "ten");
  
  // table2 should now have table1's original contents
  EXPECT_EQ(*table2.search(1), "one");
  EXPECT_EQ(*table2.search(2), "two");
  EXPECT_EQ(table2.search(10), nullptr);
}

TEST(DynLhashTableSwap, SwapWithEmpty)
{
  DynLhashTable<int, std::string> table1;
  DynLhashTable<int, std::string> table2;
  
  table1.insert(1, "one");
  
  table1.swap(table2);
  
  EXPECT_EQ(table1.search(1), nullptr);
  EXPECT_EQ(*table2.search(1), "one");
}

// =============================================================================
// Post-Move State Tests
// =============================================================================

TEST(DynLhashTablePostMove, MoveConstructorTransfersOwnership)
{
  DynLhashTable<int, std::string> source;
  source.insert(1, "one");
  source.insert(2, "two");
  
  DynLhashTable<int, std::string> dest(std::move(source));
  
  // Destination should have all the data
  EXPECT_EQ(*dest.search(1), "one");
  EXPECT_EQ(*dest.search(2), "two");
  
  // Note: source is in moved-from state - only safe for destruction/assignment
}

TEST(DynLhashTablePostMove, MoveAssignmentTransfersOwnership)
{
  DynLhashTable<int, std::string> source;
  source.insert(1, "one");
  source.insert(2, "two");
  
  DynLhashTable<int, std::string> dest;
  dest.insert(10, "ten");  // Will be replaced
  
  dest = std::move(source);
  
  // Destination should have source's data
  EXPECT_EQ(*dest.search(1), "one");
  EXPECT_EQ(*dest.search(2), "two");
  EXPECT_EQ(dest.search(10), nullptr);  // Old data gone
  
  // Note: source is in moved-from state - only safe for destruction/assignment
}

// =============================================================================
// Complex Types Tests
// =============================================================================

struct ComplexValue
{
  std::string data;
  int counter;
  static int copy_count;
  static int move_count;
  
  ComplexValue() : data(""), counter(0) {}
  ComplexValue(const std::string& d, int c) : data(d), counter(c) {}
  
  ComplexValue(const ComplexValue& other) 
    : data(other.data), counter(other.counter) { ++copy_count; }
  
  ComplexValue(ComplexValue&& other) noexcept
    : data(std::move(other.data)), counter(other.counter) { ++move_count; }
  
  ComplexValue& operator=(const ComplexValue& other)
  {
    data = other.data;
    counter = other.counter;
    ++copy_count;
    return *this;
  }
  
  ComplexValue& operator=(ComplexValue&& other) noexcept
  {
    data = std::move(other.data);
    counter = other.counter;
    ++move_count;
    return *this;
  }
  
  bool operator==(const ComplexValue& other) const
  {
    return data == other.data && counter == other.counter;
  }
};

int ComplexValue::copy_count = 0;
int ComplexValue::move_count = 0;

TEST(DynLhashTableComplexType, MoveSemanticsPropagated)
{
  ComplexValue::copy_count = 0;
  ComplexValue::move_count = 0;
  
  DynLhashTable<int, ComplexValue> table;
  
  ComplexValue val("test", 42);
  table.insert(1, std::move(val));
  
  // Should have moved, not copied
  EXPECT_GT(ComplexValue::move_count, 0);
  
  auto* found = table.search(1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->data, "test");
  EXPECT_EQ(found->counter, 42);
}

// =============================================================================
// Duplicate Keys Behavior Tests
// =============================================================================

TEST(DynLhashTableDuplicates, InsertDuplicateKeysAllowed)
{
  DynLhashTable<int, std::string> table;
  
  auto* ptr1 = table.insert(1, "first");
  auto* ptr2 = table.insert(1, "second");
  
  // Both inserts should succeed and return different pointers
  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  EXPECT_NE(ptr1, ptr2);
  
  // Both values should be accessible via their pointers
  EXPECT_EQ(*ptr1, "first");
  EXPECT_EQ(*ptr2, "second");
}

TEST(DynLhashTableDuplicates, RemoveOneDuplicate)
{
  DynLhashTable<int, std::string> table;
  
  auto* ptr1 = table.insert(1, "first");
  auto* ptr2 = table.insert(1, "second");
  
  // Remove the second inserted (which search() finds first due to chain order)
  table.remove(ptr2);
  
  // ptr1 should still be valid and now searchable
  EXPECT_EQ(*ptr1, "first");
  auto* found = table.search(1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*found, "first");
}

// =============================================================================
// Large Scale Stress Tests
// =============================================================================

TEST(DynLhashTableStress, LargeScaleWithVerification)
{
  DynLhashTable<int, int> table;
  const int N = 100000;
  
  // Insert many elements
  for (int i = 0; i < N; ++i)
    table.insert(i, i * 3);
  
  // Verify all elements
  for (int i = 0; i < N; ++i)
    {
      auto* found = table.search(i);
      ASSERT_NE(found, nullptr) << "Key " << i << " not found";
      EXPECT_EQ(*found, i * 3) << "Wrong value for key " << i;
    }
}

TEST(DynLhashTableStress, InterleavedInsertRemove)
{
  DynLhashTable<int, int> table;
  std::vector<int*> ptrs;
  const int N = 10000;
  
  // Insert first batch
  for (int i = 0; i < N; ++i)
    ptrs.push_back(table.insert(i, i));
  
  // Remove odd indices, insert new values
  for (int i = 1; i < N; i += 2)
    {
      table.remove(ptrs[i]);
      ptrs[i] = nullptr;
    }
  
  // Insert replacement values
  for (int i = N; i < N + N/2; ++i)
    table.insert(i, i);
  
  // Verify even indices remain
  for (int i = 0; i < N; i += 2)
    {
      auto* found = table.search(i);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(*found, i);
    }
  
  // Verify odd indices removed
  for (int i = 1; i < N; i += 2)
    EXPECT_EQ(table.search(i), nullptr);
  
  // Verify new values
  for (int i = N; i < N + N/2; ++i)
    {
      auto* found = table.search(i);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(*found, i);
    }
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
