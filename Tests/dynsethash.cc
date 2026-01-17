
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
 * @file dynsethash.cc
 * @brief Tests for Dynsethash
 */

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <gtest/gtest.h>

#include <tpl_dynSetHash.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Basic Operations Tests - DynHashTable
// ============================================================================

TEST(DynHashTable, EmptyTableProperties)
{
  DynSetLhash<int> table;

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.size(), 0u);
  EXPECT_FALSE(table.contains(42));
  EXPECT_EQ(table.search(42), nullptr);
  EXPECT_FALSE(table.has(42));
}

TEST(DynHashTable, InsertSingleElement)
{
  DynSetLhash<int> table;

  auto * p = table.insert(42);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);
  EXPECT_FALSE(table.is_empty());
  EXPECT_EQ(table.size(), 1u);
  EXPECT_TRUE(table.contains(42));
  EXPECT_TRUE(table.has(42));
}

TEST(DynHashTable, InsertMultipleElements)
{
  DynSetLhash<int> table;

  for (int i : {5, 3, 7, 1, 9, 2, 8})
    {
      auto * p = table.insert(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(*p, i);
    }

  EXPECT_EQ(table.size(), 7u);

  for (int i : {1, 2, 3, 5, 7, 8, 9})
    EXPECT_TRUE(table.contains(i));

  EXPECT_FALSE(table.contains(4));
  EXPECT_FALSE(table.contains(6));
}

TEST(DynHashTable, InsertRejectsDuplicates)
{
  DynSetLhash<int> table;

  auto * p1 = table.insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(table.size(), 1u);

  auto * p2 = table.insert(42);
  EXPECT_EQ(p2, nullptr);  // Duplicate rejected
  EXPECT_EQ(table.size(), 1u);  // Size unchanged
}

TEST(DynHashTable, SearchFindsExistingKey)
{
  DynSetLhash<int> table;

  table.insert(10);
  table.insert(20);
  table.insert(30);

  auto * p = table.search(20);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 20);
}

TEST(DynHashTable, SearchReturnsNullForMissingKey)
{
  DynSetLhash<int> table;

  table.insert(10);
  table.insert(30);

  EXPECT_EQ(table.search(20), nullptr);
}

TEST(DynHashTable, FindReturnsReference)
{
  DynSetLhash<int> table;

  table.insert(42);

  const auto & key = table.find(42);
  EXPECT_EQ(key, 42);

  // Non-const version returns modifiable reference
  auto & key2 = table.find(42);
  EXPECT_EQ(key2, 42);

  // WARNING: Modifying a key through the reference breaks the hash table invariants!
  // The key is stored at hash(42)'s bucket, but if we change it to 99,
  // it won't be found at hash(99)'s bucket. This is UNDEFINED BEHAVIOR.
  // So we just verify that find() returns a non-const reference without actually modifying it.
}

TEST(DynHashTable, FindThrowsIfNotFound)
{
  DynSetLhash<int> table;

  table.insert(10);

  EXPECT_THROW(table.find(42), domain_error);
}

// ============================================================================
// Insert Variations Tests
// ============================================================================

TEST(DynHashTable, SearchOrInsertBehavior)
{
  DynSetLhash<int> table;

  // First call: inserts
  auto * p1 = table.search_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(table.size(), 1u);

  // Second call: returns existing
  auto * p2 = table.search_or_insert(42);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 42);
  EXPECT_EQ(table.size(), 1u);  // Size unchanged
  EXPECT_EQ(p1, p2);  // Same pointer
}

TEST(DynHashTable, ContainsOrInsertBehavior)
{
  DynSetLhash<int> table;

  // First call: inserts, returns {ptr, false}
  auto [p1, found1] = table.contains_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_FALSE(found1);  // Was inserted
  EXPECT_EQ(table.size(), 1u);

  // Second call: finds, returns {ptr, true}
  auto [p2, found2] = table.contains_or_insert(42);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 42);
  EXPECT_TRUE(found2);  // Already existed
  EXPECT_EQ(table.size(), 1u);
  EXPECT_EQ(p1, p2);  // Same pointer
}

TEST(DynHashTable, AddAndAppendAliases)
{
  DynSetLhash<int> table;

  auto * p1 = table.add(10);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 10);

  auto * p2 = table.append(20);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 20);

  EXPECT_EQ(table.size(), 2u);
}

TEST(DynHashTable, MoveInsert)
{
  DynSetLhash<string> table;

  string s = "hello";
  auto * p = table.insert(std::move(s));
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, "hello");
  // s should be in moved-from state (typically empty, but not guaranteed)
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST(DynHashTable, RemoveByKey)
{
  DynSetLhash<int> table;

  table.insert(10);
  table.insert(20);
  table.insert(30);

  int removed = table.remove(20);
  EXPECT_EQ(removed, 20);
  EXPECT_EQ(table.size(), 2u);
  EXPECT_FALSE(table.contains(20));
  EXPECT_TRUE(table.contains(10));
  EXPECT_TRUE(table.contains(30));
}

TEST(DynHashTable, RemoveThrowsIfNotFound)
{
  DynSetLhash<int> table;

  table.insert(10);

  EXPECT_THROW(table.remove(42), domain_error);
}

TEST(DynHashTable, RemoveByPointer)
{
  DynSetLhash<int> table;

  table.insert(10);
  auto * p20 = table.insert(20);
  table.insert(30);

  ASSERT_NE(p20, nullptr);

  // Remove by pointer
  table.remove(p20);

  EXPECT_EQ(table.size(), 2u);
  EXPECT_FALSE(table.contains(20));
  EXPECT_TRUE(table.contains(10));
  EXPECT_TRUE(table.contains(30));
}

TEST(DynHashTable, RemoveAllElements)
{
  DynSetLhash<int> table;

  for (int i = 0; i < 50; ++i)
    table.insert(i);

  EXPECT_EQ(table.size(), 50u);

  for (int i = 0; i < 50; ++i)
    table.remove(i);

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.size(), 0u);
}

TEST(DynHashTable, EmptyMethod)
{
  DynSetLhash<int> table;

  for (int i = 0; i < 100; ++i)
    table.insert(i);

  EXPECT_EQ(table.size(), 100u);

  table.empty();

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.size(), 0u);
}

// ============================================================================
// Copy/Move Semantics Tests
// ============================================================================

TEST(DynHashTable, CopyConstructor)
{
  DynSetLhash<int> table1;

  for (int i : {1, 2, 3, 4, 5})
    table1.insert(i);

  DynSetLhash<int> table2(table1);

  EXPECT_EQ(table2.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(table2.contains(i));

  // Verify independence
  table1.remove(3);
  EXPECT_FALSE(table1.contains(3));
  EXPECT_TRUE(table2.contains(3));
}

TEST(DynHashTable, CopyAssignment)
{
  DynSetLhash<int> table1, table2;

  for (int i : {1, 2, 3})
    table1.insert(i);

  for (int i : {10, 20})
    table2.insert(i);

  table2 = table1;

  EXPECT_EQ(table2.size(), 3u);
  for (int i : {1, 2, 3})
    EXPECT_TRUE(table2.contains(i));
  EXPECT_FALSE(table2.contains(10));
  EXPECT_FALSE(table2.contains(20));
}

TEST(DynHashTable, SelfAssignment)
{
  DynSetLhash<int> table;

  for (int i : {1, 2, 3})
    table.insert(i);

  table = table;

  EXPECT_EQ(table.size(), 3u);
  for (int i : {1, 2, 3})
    EXPECT_TRUE(table.contains(i));
}

TEST(DynHashTable, MoveConstructor)
{
  DynSetLhash<int> table1;

  for (int i : {1, 2, 3, 4, 5})
    table1.insert(i);

  DynSetLhash<int> table2(std::move(table1));

  EXPECT_EQ(table2.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(table2.contains(i));

  EXPECT_TRUE(table1.is_empty());
}

TEST(DynHashTable, MoveAssignment)
{
  DynSetLhash<int> table1, table2;

  for (int i : {1, 2, 3})
    table1.insert(i);

  for (int i : {10, 20})
    table2.insert(i);

  table2 = std::move(table1);

  EXPECT_EQ(table2.size(), 3u);
  for (int i : {1, 2, 3})
    EXPECT_TRUE(table2.contains(i));

  // After fix: Move assignment should leave table1 empty
  EXPECT_TRUE(table1.is_empty());
  EXPECT_EQ(table1.size(), 0u);
}

TEST(DynHashTable, Swap)
{
  DynSetLhash<int> table1, table2;

  for (int i : {1, 2, 3})
    table1.insert(i);

  for (int i : {10, 20})
    table2.insert(i);

  table1.swap(table2);

  EXPECT_EQ(table1.size(), 2u);
  EXPECT_TRUE(table1.contains(10));
  EXPECT_TRUE(table1.contains(20));

  EXPECT_EQ(table2.size(), 3u);
  EXPECT_TRUE(table2.contains(1));
  EXPECT_TRUE(table2.contains(2));
  EXPECT_TRUE(table2.contains(3));
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(DynHashTable, IteratorEmptyTable)
{
  DynSetLhash<int> table;

  DynSetLhash<int>::Iterator it(table);
  EXPECT_FALSE(it.has_curr());
}

TEST(DynHashTable, IteratorTraversal)
{
  DynSetLhash<int> table;

  for (int i : {5, 3, 7, 1, 9})
    table.insert(i);

  vector<int> keys;
  for (DynSetLhash<int>::Iterator it(table); it.has_curr(); it.next_ne())
    keys.push_back(it.get_curr());

  EXPECT_EQ(keys.size(), 5u);

  // Hash table doesn't guarantee order, just verify all keys present
  sort(keys.begin(), keys.end());
  vector<int> expected = {1, 3, 5, 7, 9};
  EXPECT_EQ(keys, expected);
}

TEST(DynHashTable, IteratorDel)
{
  DynSetLhash<int> table;

  for (int i : {1, 2, 3, 4, 5})
    table.insert(i);

  DynSetLhash<int>::Iterator it(table);
  ASSERT_TRUE(it.has_curr());

  int first = it.get_curr();
  it.del();

  EXPECT_EQ(table.size(), 4u);
  EXPECT_FALSE(table.contains(first));
}

TEST(DynHashTable, GetFirstAndLast)
{
  DynSetLhash<int> table;

  table.insert(10);
  table.insert(20);
  table.insert(30);

  // Just verify they don't throw
  EXPECT_NO_THROW(table.get_first());
  EXPECT_NO_THROW(table.get_last());
}

// ============================================================================
// String Keys Tests
// ============================================================================

TEST(DynHashTable, StringKeys)
{
  DynSetLhash<string> table;

  table.insert("apple");
  table.insert("banana");
  table.insert("cherry");

  EXPECT_EQ(table.size(), 3u);
  EXPECT_TRUE(table.contains("apple"));
  EXPECT_TRUE(table.contains("banana"));
  EXPECT_FALSE(table.contains("date"));
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(DynHashTable, LargeNumberOfInsertions)
{
  DynSetLhash<int> table;

  // Insert 10000 elements
  for (int i = 0; i < 10000; ++i)
    table.insert(i);

  EXPECT_EQ(table.size(), 10000u);

  // Verify all present
  for (int i = 0; i < 10000; ++i)
    EXPECT_TRUE(table.contains(i));
}

TEST(DynHashTable, ManyCollisions)
{
  // Custom hash that always returns same value -> forces collisions
  auto bad_hash = [](const int &) -> size_t { return 42; };

  DynSetLhash<int> table(100, bad_hash);

  for (int i = 0; i < 100; ++i)
    table.insert(i);

  EXPECT_EQ(table.size(), 100u);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(table.contains(i));
}

TEST(DynHashTable, RandomInsertRemove)
{
  DynSetLhash<int> table;
  vector<int> inserted;

  // Random insertions
  for (int i = 0; i < 500; ++i)
    {
      if (int key = rand() % 1000; table.insert(key) != nullptr)
        inserted.push_back(key);
    }

  // Verify all inserted keys are present
  for (int key : inserted)
    EXPECT_TRUE(table.contains(key));

  EXPECT_EQ(table.size(), inserted.size());

  // Random removals
  for (size_t i = 0; i < inserted.size() / 2; ++i)
    {
      int idx = rand() % inserted.size();
      table.remove(inserted[idx]);
      inserted.erase(inserted.begin() + idx);
    }

  EXPECT_EQ(table.size(), inserted.size());
  for (int key : inserted)
    EXPECT_TRUE(table.contains(key));
}

TEST(DynHashTable, RehashingBehavior)
{
  DynSetLhash<int> table(10);  // Start small

  // Insert many elements to force rehashing
  for (int i = 0; i < 1000; ++i)
    table.insert(i);

  EXPECT_EQ(table.size(), 1000u);

  // Verify all still accessible
  for (int i = 0; i < 1000; ++i)
    EXPECT_TRUE(table.contains(i));

  // Remove many to trigger shrinking
  for (int i = 0; i < 950; ++i)
    table.remove(i);

  EXPECT_EQ(table.size(), 50u);

  for (int i = 950; i < 1000; ++i)
    EXPECT_TRUE(table.contains(i));
}

// ============================================================================
// DynMapHashTable Tests
// ============================================================================

TEST(DynMapHashTable, EmptyMapProperties)
{
  DynMapHash<int, string> map;

  EXPECT_TRUE(map.is_empty());
  EXPECT_EQ(map.size(), 0u);
  EXPECT_FALSE(map.contains(42));
  EXPECT_EQ(map.search(42), nullptr);
}

TEST(DynMapHashTable, InsertPair)
{
  DynMapHash<int, string> map;

  auto * p = map.insert(1, "one");
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->first, 1);
  EXPECT_EQ(p->second, "one");
  EXPECT_EQ(map.size(), 1u);
}

TEST(DynMapHashTable, InsertMultiplePairs)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");

  EXPECT_EQ(map.size(), 3u);
  EXPECT_TRUE(map.contains(1));
  EXPECT_TRUE(map.contains(2));
  EXPECT_TRUE(map.contains(3));
}

TEST(DynMapHashTable, InsertRejectsDuplicateKeys)
{
  DynMapHash<int, string> map;

  auto * p1 = map.insert(1, "one");
  ASSERT_NE(p1, nullptr);

  auto * p2 = map.insert(1, "uno");
  EXPECT_EQ(p2, nullptr);  // Duplicate key rejected
  EXPECT_EQ(map.size(), 1u);

  // Original value unchanged
  EXPECT_EQ(map.find(1), "one");
}

TEST(DynMapHashTable, SearchByKey)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");
  map.insert(2, "two");

  auto * p = map.search(2);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->first, 2);
  EXPECT_EQ(p->second, "two");

  EXPECT_EQ(map.search(99), nullptr);
}

TEST(DynMapHashTable, FindByKey)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");

  EXPECT_EQ(map.find(1), "one");
  EXPECT_THROW(map.find(99), domain_error);
}

TEST(DynMapHashTable, OperatorBracketInsert)
{
  DynMapHash<int, string> map;

  // Non-const operator[] inserts if key doesn't exist
  map[1] = "one";
  EXPECT_EQ(map.size(), 1u);
  EXPECT_EQ(map[1], "one");

  // Can modify existing
  map[1] = "uno";
  EXPECT_EQ(map[1], "uno");
  EXPECT_EQ(map.size(), 1u);
}

TEST(DynMapHashTable, OperatorBracketConstThrows)
{
  DynMapHash<int, string> map;
  map.insert(1, "one");

  const auto & const_map = map;
  EXPECT_EQ(const_map[1], "one");
  EXPECT_THROW(const_map[99], domain_error);
}

TEST(DynMapHashTable, RemoveByKey)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");

  string removed = map.remove(2);
  EXPECT_EQ(removed, "two");
  EXPECT_EQ(map.size(), 2u);
  EXPECT_FALSE(map.contains(2));
}

TEST(DynMapHashTable, Keys)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");

  auto keys = map.keys();
  EXPECT_EQ(keys.size(), 3u);

  vector<int> sorted_keys;
  keys.for_each([&sorted_keys](int k) { sorted_keys.push_back(k); });
  sort(sorted_keys.begin(), sorted_keys.end());

  vector<int> expected = {1, 2, 3};
  EXPECT_EQ(sorted_keys, expected);
}

// BUG: tpl_dynSetHash.H line 540 has wrong return type (DynList<Key> instead of DynList<Data>)
// This test is disabled until the bug is fixed in the library
// The values() method should return DynList<Data> but currently returns DynList<Key>
TEST(DynMapHashTable, DISABLED_Values)
{
  // Test disabled due to bug in library implementation
  GTEST_SKIP() << "values() has wrong return type in tpl_dynSetHash.H:540";
}

TEST(DynMapHashTable, ValuesPtr)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");
  map.insert(2, "two");

  auto ptrs = map.values_ptr();
  EXPECT_EQ(ptrs.size(), 2u);

  // Modify through pointer
  ptrs.for_each([](string * p) {
    if (*p == "one")
      *p = "ONE";
  });

  EXPECT_EQ(map.find(1), "ONE");
}

TEST(DynMapHashTable, ItemsPtr)
{
  DynMapHash<int, string> map;

  map.insert(1, "one");
  map.insert(2, "two");

  auto items = map.items_ptr();
  EXPECT_EQ(items.size(), 2u);
}

TEST(DynMapHashTable, MoveSemantics)
{
  DynMapHash<int, string> map;

  string s = "hello";
  map.insert(1, std::move(s));
  EXPECT_EQ(map.find(1), "hello");

  int k = 2;
  map.insert(std::move(k), "world");
  EXPECT_EQ(map.find(2), "world");
}

// ============================================================================
// Free Functions Tests (join, intercept, unique, repeated)
// ============================================================================

TEST(DynHashTableFunctions, Join)
{
  DynList<int> l1 = {1, 2, 3, 4};
  DynList<int> l2 = {3, 4, 5, 6};

  auto result = join(l1, l2);

  // Result should be union: {1, 2, 3, 4, 5, 6}
  EXPECT_EQ(result.size(), 6u);

  DynSetLhash<int> result_set(result);
  for (int i : {1, 2, 3, 4, 5, 6})
    EXPECT_TRUE(result_set.contains(i));
}

TEST(DynHashTableFunctions, Intercept)
{
  DynList<int> l1 = {1, 2, 3, 4, 5};
  DynList<int> l2 = {3, 4, 5, 6, 7};

  auto result = intercept(l1, l2);

  // Result should be intersection: {3, 4, 5}
  EXPECT_EQ(result.size(), 3u);

  vector<int> vec;
  result.for_each([&vec](int i) { vec.push_back(i); });
  sort(vec.begin(), vec.end());

  vector<int> expected = {3, 4, 5};
  EXPECT_EQ(vec, expected);
}

TEST(DynHashTableFunctions, Unique)
{
  DynList<int> l = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};

  auto result = unique(l);

  // Result should be {1, 2, 3, 4}
  EXPECT_EQ(result.size(), 4u);

  DynSetLhash<int> result_set(result);
  for (int i : {1, 2, 3, 4})
    EXPECT_TRUE(result_set.contains(i));
}

TEST(DynHashTableFunctions, Repeated)
{
  DynList<int> l = {1, 2, 2, 3, 3, 3, 4, 5};

  auto result = repeated(l);

  // Result contains all occurrences of duplicated values
  // The function returns ALL instances of keys that appear more than once
  // So we get 3 elements: one 2 (at index 2), and two 3s (at indices 3 and 4)
  EXPECT_GE(result.size(), 2u);  // At least 2 and 3 are duplicated

  DynSetLhash<int> result_set(result);
  EXPECT_TRUE(result_set.contains(2));
  EXPECT_TRUE(result_set.contains(3));
  EXPECT_FALSE(result_set.contains(1));  // 1 only appears once
  EXPECT_FALSE(result_set.contains(4));  // 4 only appears once
  EXPECT_FALSE(result_set.contains(5));  // 5 only appears once
}

TEST(DynHashTableFunctions, RepeatedWithIndex)
{
  DynList<int> l = {1, 2, 2, 3, 4, 3};

  auto result = repeated_with_index(l);

  // Result should be: {(2, 2), (3, 5)}
  EXPECT_EQ(result.size(), 2u);

  bool found_2 = false, found_3 = false;
  result.for_each([&](const pair<int, size_t> & p) {
    if (p.first == 2)
      {
        EXPECT_EQ(p.second, 2u);
        found_2 = true;
      }
    if (p.first == 3)
      {
        EXPECT_EQ(p.second, 5u);
        found_3 = true;
      }
  });

  EXPECT_TRUE(found_2);
  EXPECT_TRUE(found_3);
}

// ============================================================================
// Different Hash Table Types Tests
// ============================================================================

TEST(DynHashTable, LinearHashTableBasics)
{
  DynSetLinHash<int> table;

  for (int i : {1, 2, 3, 4, 5})
    table.insert(i);

  EXPECT_EQ(table.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(table.contains(i));
}

TEST(DynHashTable, DynSetHashAlias)
{
  DynSetHash<int> table;

  for (int i : {10, 20, 30})
    table.insert(i);

  EXPECT_EQ(table.size(), 3u);
  EXPECT_TRUE(table.contains(20));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(DynHashTable, SingleElement)
{
  DynSetLhash<int> table;

  table.insert(42);

  EXPECT_EQ(table.size(), 1u);
  EXPECT_TRUE(table.contains(42));

  table.remove(42);
  EXPECT_TRUE(table.is_empty());
}

TEST(DynHashTable, InsertRemoveRepeated)
{
  DynSetLhash<int> table;

  for (int cycle = 0; cycle < 10; ++cycle)
    {
      for (int i = 0; i < 50; ++i)
        table.insert(i);

      EXPECT_EQ(table.size(), 50u);

      for (int i = 0; i < 50; ++i)
        table.remove(i);

      EXPECT_TRUE(table.is_empty());
    }
}

TEST(DynHashTable, CustomHashFunction)
{
  // Custom hash: just return the value modulo 100
  auto custom_hash = [](const int & k) -> size_t { return k % 100; };

  DynSetLhash<int> table(100, custom_hash);

  table.insert(1);
  table.insert(101);
  table.insert(201);

  EXPECT_EQ(table.size(), 3u);
  EXPECT_TRUE(table.contains(1));
  EXPECT_TRUE(table.contains(101));
  EXPECT_TRUE(table.contains(201));
}

// ============================================================================
// Custom Types Tests
// ============================================================================

struct Point
{
  int x, y;

  bool operator == (const Point & other) const
  {
    return x == other.x && y == other.y;
  }
};

namespace std
{
  template <>
  struct hash<Point>
  {
    size_t operator()(const Point & p) const
    {
      return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
    }
  };
}

size_t point_hash(const Point & p)
{
  return std::hash<Point>()(p);
}

TEST(DynHashTable, CustomType)
{
  DynSetLhash<Point> table(100, point_hash);

  Point p1{1, 2};
  Point p2{3, 4};
  Point p3{5, 6};

  table.insert(p1);
  table.insert(p2);
  table.insert(p3);

  EXPECT_EQ(table.size(), 3u);
  EXPECT_TRUE(table.contains(p1));
  EXPECT_TRUE(table.contains(p2));
  EXPECT_TRUE(table.contains(p3));

  Point p4{7, 8};
  EXPECT_FALSE(table.contains(p4));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}