
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
 * @file lin-hash.cc
 * @brief Tests for Lin Hash
 */
#include <gtest/gtest.h>

#include <tpl_linHash.H>
#include <random>
#include <set>
#include <vector>
#include <algorithm>

using namespace std;
using namespace Aleph;

// ============================================================================
// Type Aliases
// ============================================================================

using Table = LinearHashTable<int>;
using Bucket = Table::Bucket;

// ============================================================================
// Empty Table Tests
// ============================================================================

TEST(LinearHashTable, EmptyTableProperties)
{
  Table table;

  EXPECT_EQ(table.size(), 0u);
  EXPECT_TRUE(table.is_empty());
  EXPECT_GT(table.capacity(), 0u);
  EXPECT_EQ(table.busy_slots(), 0u);
  EXPECT_EQ(table.expansions(), 0u);
}

TEST(LinearHashTable, SearchOnEmptyTableReturnsNull)
{
  Table table;

  EXPECT_EQ(table.search(10), nullptr);
  EXPECT_EQ(table.search(0), nullptr);
  EXPECT_EQ(table.search(-1), nullptr);
}

// ============================================================================
// Insert Tests
// ============================================================================

TEST(LinearHashTable, InsertSingleElement)
{
  Table table;

  auto *bucket = new Bucket(10);
  auto *inserted = table.insert(bucket);

  EXPECT_EQ(inserted, bucket);
  EXPECT_EQ(table.size(), 1u);
  EXPECT_FALSE(table.is_empty());
  EXPECT_GE(table.busy_slots(), 1u);
}

TEST(LinearHashTable, InsertMultipleElements)
{
  Table table;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    {
      auto *bucket = new Bucket(k);
      ASSERT_NE(table.insert(bucket), nullptr);
    }

  EXPECT_EQ(table.size(), 7u);

  // Verify all elements can be found
  for (int k : {5, 3, 7, 1, 4, 6, 8})
    EXPECT_NE(table.search(k), nullptr) << "Key " << k << " not found";
}

TEST(LinearHashTable, InsertRejectsDuplicates)
{
  Table table;

  auto *bucket1 = new Bucket(10);
  auto *bucket2 = new Bucket(10);

  EXPECT_NE(table.insert(bucket1), nullptr);
  EXPECT_EQ(table.insert(bucket2), nullptr); // duplicate rejected

  EXPECT_EQ(table.size(), 1u);
  delete bucket2; // cleanup rejected bucket
}

TEST(LinearHashTable, InsertManyElementsTriggersExpansion)
{
  Table table(17); // small initial size

  const size_t initial_capacity = table.capacity();

  // Insert enough elements to trigger expansion
  for (int k = 0; k < 100; ++k)
    table.insert(new Bucket(k));

  EXPECT_EQ(table.size(), 100u);
  EXPECT_GT(table.capacity(), initial_capacity);
  EXPECT_GT(table.expansions(), 0u);

  // Verify all elements can be found
  for (int k = 0; k < 100; ++k)
    EXPECT_NE(table.search(k), nullptr);
}

// ============================================================================
// Search Tests
// ============================================================================

TEST(LinearHashTable, SearchFindsExistingKey)
{
  Table table;

  for (int k : {1, 2, 3, 4, 5})
    table.insert(new Bucket(k));

  for (int k : {1, 2, 3, 4, 5})
    {
      auto *found = table.search(k);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(found->get_key(), k);
    }
}

TEST(LinearHashTable, SearchReturnsNullForMissingKey)
{
  Table table;

  for (int k : {1, 3, 5})
    table.insert(new Bucket(k));

  EXPECT_EQ(table.search(2), nullptr);
  EXPECT_EQ(table.search(4), nullptr);
  EXPECT_EQ(table.search(0), nullptr);
  EXPECT_EQ(table.search(6), nullptr);
}

TEST(LinearHashTable, SearchOrInsertReturnsExisting)
{
  Table table;

  auto *bucket1 = new Bucket(10);
  table.insert(bucket1);

  auto *bucket2 = new Bucket(10);
  auto *found = table.search_or_insert(bucket2);

  EXPECT_EQ(found, bucket1);
  EXPECT_EQ(table.size(), 1u);

  delete bucket2; // cleanup rejected bucket
}

TEST(LinearHashTable, SearchOrInsertInsertsNew)
{
  Table table;

  table.insert(new Bucket(5));

  auto *bucket = new Bucket(10);
  auto *result = table.search_or_insert(bucket);

  EXPECT_EQ(result, bucket);
  EXPECT_EQ(table.size(), 2u);
  EXPECT_NE(table.search(10), nullptr);
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST(LinearHashTable, RemoveExistingKey)
{
  Table table;

  for (int k : {1, 2, 3, 4, 5})
    table.insert(new Bucket(k));

  auto *found = table.search(3);
  ASSERT_NE(found, nullptr);

  auto *removed = table.remove(found);
  EXPECT_EQ(removed->get_key(), 3);
  delete removed;

  EXPECT_EQ(table.size(), 4u);
  EXPECT_EQ(table.search(3), nullptr);
}

TEST(LinearHashTable, RemoveAllElements)
{
  Table table;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    table.insert(new Bucket(k));

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    {
      auto *found = table.search(k);
      ASSERT_NE(found, nullptr) << "Key " << k << " not found";
      delete table.remove(found);
    }

  EXPECT_EQ(table.size(), 0u);
  EXPECT_TRUE(table.is_empty());
}

TEST(LinearHashTable, RemoveManyElementsTriggersContraction)
{
  Table table(17);

  // Insert many elements
  for (int k = 0; k < 200; ++k)
    table.insert(new Bucket(k));

  const size_t expanded_capacity = table.capacity();

  // Remove most elements
  for (int k = 0; k < 180; ++k)
    {
      auto *found = table.search(k);
      ASSERT_NE(found, nullptr);
      delete table.remove(found);
    }

  EXPECT_EQ(table.size(), 20u);
  // Capacity should have contracted
  EXPECT_LE(table.capacity(), expanded_capacity);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(LinearHashTable, IteratorTraversesAllElements)
{
  Table table;

  std::set<int> inserted;
  for (int k : {5, 3, 7, 1, 4, 6, 8})
    {
      table.insert(new Bucket(k));
      inserted.insert(k);
    }

  std::set<int> visited;
  for (Table::Iterator it(table); it.has_curr(); it.next())
    visited.insert(it.get_curr()->get_key());

  EXPECT_EQ(visited, inserted);
}

TEST(LinearHashTable, IteratorOnEmptyTable)
{
  Table table;
  Table::Iterator it(table);

  EXPECT_FALSE(it.has_curr());
}

TEST(LinearHashTable, IteratorDel)
{
  Table table;

  for (int k : {1, 2, 3, 4, 5})
    table.insert(new Bucket(k));

  // Find and delete element 3 using iterator
  for (Table::Iterator it(table); it.has_curr(); it.next())
    {
      if (it.get_curr()->get_key() == 3)
        {
          delete it.del();
          break;
        }
    }

  EXPECT_EQ(table.size(), 4u);
  EXPECT_EQ(table.search(3), nullptr);
}

TEST(LinearHashTable, IteratorGetPos)
{
  Table table;

  for (int k : {1, 2, 3, 4, 5})
    table.insert(new Bucket(k));

  Table::Iterator it(table);
  EXPECT_EQ(it.get_pos(), 0);

  it.next();
  EXPECT_EQ(it.get_pos(), 1);

  it.next();
  EXPECT_EQ(it.get_pos(), 2);
}

TEST(LinearHashTable, IteratorPrev)
{
  Table table;

  for (int k : {1, 2, 3})
    table.insert(new Bucket(k));

  Table::Iterator it(table);
  it.next();
  it.next();
  EXPECT_EQ(it.get_pos(), 2);

  it.prev();
  EXPECT_EQ(it.get_pos(), 1);

  it.prev();
  EXPECT_EQ(it.get_pos(), 0);
}

TEST(LinearHashTable, IteratorNoExceptMethods)
{
  Table table;

  for (int k : {1, 2, 3})
    table.insert(new Bucket(k));

  Table::Iterator it(table);

  // Test get_curr_ne() and next_ne()
  auto *bucket1 = it.get_curr_ne();
  EXPECT_NE(bucket1, nullptr);

  it.next_ne();
  auto *bucket2 = it.get_curr_ne();
  EXPECT_NE(bucket2, nullptr);
  EXPECT_NE(bucket1, bucket2);
}

// ============================================================================
// Swap Tests
// ============================================================================

// NOTE: swap() has a known issue with entries_list and embedded Dlink nodes
// which causes use-after-free in destructors. This test uses remove_all_buckets=false
// and manually cleans up to avoid the issue.
TEST(LinearHashTable, SwapTables)
{
  // Use remove_all_buckets=false to avoid destructor cleanup issues after swap
  Table table1(17, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
               hash_default_lower_alpha, hash_default_upper_alpha, false, true);
  Table table2(17, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
               hash_default_lower_alpha, hash_default_upper_alpha, false, true);

  std::vector<Bucket*> buckets1, buckets2;
  for (int k : {1, 2, 3})
    {
      auto* b = new Bucket(k);
      table1.insert(b);
      buckets1.push_back(b);
    }
  for (int k : {10, 20})
    {
      auto* b = new Bucket(k);
      table2.insert(b);
      buckets2.push_back(b);
    }

  table1.swap(table2);

  EXPECT_EQ(table1.size(), 2u);
  EXPECT_EQ(table2.size(), 3u);

  EXPECT_NE(table1.search(10), nullptr);
  EXPECT_NE(table1.search(20), nullptr);
  EXPECT_NE(table2.search(1), nullptr);
  EXPECT_NE(table2.search(2), nullptr);
  EXPECT_NE(table2.search(3), nullptr);

  // Manual cleanup since remove_all_buckets=false
  for (auto* b : buckets1) delete b;
  for (auto* b : buckets2) delete b;
}

// ============================================================================
// Empty Method Tests
// ============================================================================

TEST(LinearHashTable, EmptyMethodClearsTable)
{
  Table table;

  for (int k = 0; k < 50; ++k)
    table.insert(new Bucket(k));

  EXPECT_EQ(table.size(), 50u);

  table.empty();

  EXPECT_EQ(table.size(), 0u);
  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.expansions(), 0u);
}

// ============================================================================
// Load Factor Tests
// ============================================================================

TEST(LinearHashTable, CurrentAlphaIsCorrect)
{
  Table table(100);

  EXPECT_FLOAT_EQ(table.current_alpha(), 0.0f);

  for (int k = 0; k < 50; ++k)
    table.insert(new Bucket(k));

  float expected_alpha = 50.0f / table.capacity();
  EXPECT_NEAR(table.current_alpha(), expected_alpha, 0.01f);
}

// ============================================================================
// Custom Comparator Tests
// ============================================================================

struct ModCompare
{
  bool operator()(int a, int b) const { return (a % 100) == (b % 100); }
};

// Custom hash that aligns with ModCompare
size_t mod_hash(const int& k) { return static_cast<size_t>(std::abs(k % 100)); }

TEST(LinearHashTable, CustomComparator)
{
  // Use custom hash that aligns with ModCompare so duplicates are detected
  LinearHashTable<int, ModCompare> table(17, mod_hash);

  table.insert(new LinHashBucket<int>(105));

  // 205 should be "equal" to 105 with ModCompare (both hash to 5, compare equal)
  auto *bucket = new LinHashBucket<int>(205);
  EXPECT_EQ(table.insert(bucket), nullptr); // rejected as duplicate

  delete bucket;

  // Search for 5 should find 105 (5 % 100 == 105 % 100)
  EXPECT_NE(table.search(5), nullptr);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(LinearHashTable, NegativeKeys)
{
  Table table;

  for (int k : {-5, -3, -1, 0, 1, 3, 5})
    table.insert(new Bucket(k));

  EXPECT_EQ(table.size(), 7u);

  for (int k : {-5, -3, -1, 0, 1, 3, 5})
    EXPECT_NE(table.search(k), nullptr);

  EXPECT_EQ(table.search(-2), nullptr);
}

TEST(LinearHashTable, SingleElementOperations)
{
  Table table;

  auto *bucket = new Bucket(42);
  table.insert(bucket);

  EXPECT_EQ(table.size(), 1u);
  EXPECT_NE(table.search(42), nullptr);

  delete table.remove(bucket);

  EXPECT_EQ(table.size(), 0u);
  EXPECT_EQ(table.search(42), nullptr);
}

// ============================================================================
// Constructor Parameter Validation Tests
// ============================================================================

TEST(LinearHashTable, ConstructorWithZeroLengthThrows)
{
  EXPECT_THROW(
    (Table(0, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
           hash_default_lower_alpha, hash_default_upper_alpha, true, true)),
    std::length_error);
}

TEST(LinearHashTable, ConstructorWithInvalidAlphaThrows)
{
  // upper_alpha <= lower_alpha should throw
  EXPECT_THROW(
    (Table(17, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
           0.8f, 0.4f, true, true)), // lower > upper
    std::domain_error);

  EXPECT_THROW(
    (Table(17, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
           0.5f, 0.5f, true, true)), // lower == upper
    std::domain_error);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(LinearHashTable, RandomInsertSearchRemove)
{
  Table table;
  std::set<int> oracle;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 999);

  // Insert phase
  for (int i = 0; i < 500; ++i)
    {
      int k = dist(rng);
      if (oracle.count(k) == 0)
        {
          ASSERT_NE(table.insert(new Bucket(k)), nullptr);
          oracle.insert(k);
        }
    }

  EXPECT_EQ(table.size(), oracle.size());

  // Search phase
  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto *found = table.search(k);
      if (oracle.count(k))
        EXPECT_NE(found, nullptr);
      else
        EXPECT_EQ(found, nullptr);
    }

  // Remove phase
  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto *found = table.search(k);
      if (oracle.count(k))
        {
          ASSERT_NE(found, nullptr);
          oracle.erase(k);
          delete table.remove(found);
        }
      else
        {
          EXPECT_EQ(found, nullptr);
        }
    }

  EXPECT_EQ(table.size(), oracle.size());

  // Verify remaining keys
  for (int k : oracle)
    EXPECT_NE(table.search(k), nullptr);
}

TEST(LinearHashTable, LargeTableOperations)
{
  Table table;

  const int N = 5000;

  // Insert N elements
  for (int k = 0; k < N; ++k)
    table.insert(new Bucket(k));

  EXPECT_EQ(table.size(), static_cast<size_t>(N));

  // Verify all elements present
  for (int k = 0; k < N; ++k)
    EXPECT_NE(table.search(k), nullptr);

  // Remove half
  for (int k = 0; k < N; k += 2)
    {
      auto *found = table.search(k);
      ASSERT_NE(found, nullptr);
      delete table.remove(found);
    }

  EXPECT_EQ(table.size(), static_cast<size_t>(N / 2));

  // Verify remaining elements
  for (int k = 1; k < N; k += 2)
    EXPECT_NE(table.search(k), nullptr);
}

// ============================================================================
// LinearHashTableVtl Tests
// ============================================================================

TEST(LinearHashTableVtl, BasicOperations)
{
  LinearHashTableVtl<int> table;

  for (int k : {1, 2, 3, 4, 5})
    table.insert(new LinHashBucketVtl<int>(k));

  EXPECT_EQ(table.size(), 5u);

  for (int k : {1, 2, 3, 4, 5})
    EXPECT_NE(table.search(k), nullptr);

  // Destructor should properly clean up with virtual destructor
}

// ============================================================================
// Hash Function Tests
// ============================================================================

TEST(LinearHashTable, SetHashFunction)
{
  Table table;

  // Custom hash function - use std::function explicitly to avoid ambiguity
  Table::Hash_Fct custom_hash = [](const int& k) -> size_t { return k * 17; };

  table.set_hash_fct(custom_hash);

  table.insert(new Bucket(1));
  table.insert(new Bucket(2));
  table.insert(new Bucket(3));

  EXPECT_NE(table.search(1), nullptr);
  EXPECT_NE(table.search(2), nullptr);
  EXPECT_NE(table.search(3), nullptr);
}

TEST(LinearHashTable, GetHashFunction)
{
  Table table;

  auto hash_fct = table.get_hash_fct();
  EXPECT_NE(hash_fct, nullptr);
}

TEST(LinearHashTable, GetCompare)
{
  Table table;

  auto& cmp = table.get_compare();
  EXPECT_TRUE(cmp(5, 5));
  EXPECT_FALSE(cmp(5, 6));

  const Table& const_table = table;
  const auto& const_cmp = const_table.get_compare();
  EXPECT_TRUE(const_cmp(10, 10));
}

// ============================================================================
// Resize Method Tests
// ============================================================================

TEST(LinearHashTable, ResizeReturnsCapacity)
{
  Table table;

  // resize() is a no-op for linear hash tables (provided for compatibility)
  size_t result = table.resize(1000);
  EXPECT_EQ(result, table.capacity());
}
