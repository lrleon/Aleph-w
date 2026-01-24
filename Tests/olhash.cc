
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
 * @file olhash.cc
 * @brief Tests for Olhash
 */
# include <gtest/gtest.h>

# include <random>
# include <set>

# include <tpl_olhash.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(OLhashTable, Simplest)
{
  OLhashTable<int> tbl(100);

  EXPECT_TRUE(tbl.is_empty());
  EXPECT_EQ(tbl.size(), 0);

  const size_t cap = tbl.capacity();
  for (size_t i = 0; i < cap; ++i)
    {
      EXPECT_EQ(tbl.size(), i);
      EXPECT_NE(tbl.insert(i), nullptr);
      EXPECT_EQ(tbl.size(), i + 1);
      EXPECT_FALSE(tbl.is_empty());
    }

  for (size_t i = 0; i < tbl.size(); ++i)
    {
      ASSERT_NE(tbl.search(i), nullptr);
      auto ptr = tbl.search(i);
      EXPECT_EQ(*ptr, i);
      EXPECT_FALSE(tbl.is_empty());
    }

  for (size_t i = 0, n = tbl.size(); i < n; ++i)
    {
      auto ptr = tbl.search(i);
      EXPECT_EQ(*ptr, i);
      tbl.remove(*ptr);
      EXPECT_EQ(tbl.size(), n - i - 1);
      EXPECT_EQ(tbl.search(i), nullptr);
      EXPECT_FALSE(tbl.contains(i));
    }

  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());
}

struct MyRecord
{
  size_t key;
  string value;
  MyRecord() {}
  MyRecord(size_t k, const string & v) : key(k), value(v) {}
  MyRecord(size_t k) : key(k) {}
  struct Eq
  {
    bool operator () (const MyRecord & r1, const MyRecord & r2) const noexcept
    {
      return r1.key == r2.key;
    }
  };
  bool operator == (const MyRecord & r) const noexcept { return Eq()(*this, r); }
};

inline size_t my_hash(const MyRecord & r) noexcept
{
  return dft_hash_fct(r.key);
}

TEST(OLhashTable, Map)
{
  OLhashTable<MyRecord, MyRecord::Eq> tbl(10, my_hash, MyRecord::Eq());

  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());

  for (size_t i = 0; i < 100; ++i)
    {
      EXPECT_EQ(tbl.size(), i);
      tbl.emplace(i, to_string(i));
      EXPECT_EQ(tbl.size(), i + 1);
      auto ptr = tbl.search(MyRecord(i));
      EXPECT_NE(ptr, nullptr);
      EXPECT_EQ(ptr->key, i);
      EXPECT_EQ(ptr->value, to_string(i));
    }

  for (size_t i = 0, n = tbl.size(); i < n; ++i)
    {
      auto ptr = tbl.search(i);
      EXPECT_EQ(*ptr, i);
      tbl.remove(MyRecord(ptr->key));
      EXPECT_EQ(tbl.size(), n - i - 1);
      EXPECT_EQ(tbl.search(i), nullptr);
      EXPECT_FALSE(tbl.contains(MyRecord(i)));
    }
}

TEST(OLhashTable, KeyToBucketRoundTrip)
{
  OLhashTable<int> tbl;
  auto *ptr = tbl.insert(5);
  ASSERT_NE(ptr, nullptr);

  auto *bucket = decltype(tbl)::key_to_bucket(ptr);
  ASSERT_NE(bucket, nullptr);
  EXPECT_EQ(bucket->key, 5);
  EXPECT_EQ(bucket->status, decltype(tbl)::BUSY);

  EXPECT_NO_THROW(tbl.remove(5));
  EXPECT_EQ(tbl.search(5), nullptr);
}

// Test that removing a non-existent key throws and preserves table integrity
TEST(OLhashTable, RemoveNonExistentKeyPreservesTableIntegrity)
{
  OLhashTable<int> tbl(100);
  
  // Insert some elements
  const int num_elements = 50;
  for (int i = 0; i < num_elements; ++i)
    EXPECT_NE(tbl.insert(i * 2), nullptr);  // Insert even numbers: 0, 2, 4, ..., 98
  
  EXPECT_EQ(tbl.size(), num_elements);
  
  // Try to remove keys that don't exist (odd numbers)
  // This should throw domain_error BUT NOT corrupt the table
  for (int i = 0; i < 10; ++i)
    {
      int non_existent_key = i * 2 + 1;  // Odd numbers: 1, 3, 5, ..., 19
      EXPECT_THROW(tbl.remove(non_existent_key), std::domain_error);
    }
  
  // Verify table size is unchanged
  EXPECT_EQ(tbl.size(), num_elements) 
    << "Table size should not change after failed remove attempts";
  
  // Verify all original elements are still findable
  for (int i = 0; i < num_elements; ++i)
    {
      auto ptr = tbl.search(i * 2);
      EXPECT_NE(ptr, nullptr) 
        << "Element " << i * 2 << " should still be in the table";
      if (ptr)
        EXPECT_EQ(*ptr, i * 2);
    }
  
  // Verify we can still remove elements normally
  for (int i = 0; i < num_elements; ++i)
    {
      auto ptr = tbl.search(i * 2);
      ASSERT_NE(ptr, nullptr);
      EXPECT_NO_THROW(tbl.remove(*ptr));
      EXPECT_EQ(tbl.size(), num_elements - i - 1);
    }
  
  EXPECT_TRUE(tbl.is_empty());
}

// Test remove with external key (key not from a bucket in the table)
TEST(OLhashTable, RemoveWithExternalKey)
{
  OLhashTable<int> tbl(100);
  
  // Insert elements
  for (int i = 0; i < 20; ++i)
    EXPECT_NE(tbl.insert(i), nullptr);
  
  EXPECT_EQ(tbl.size(), 20);
  
  // Remove using external key (not a reference from the table)
  int external_key = 10;
  EXPECT_NO_THROW(tbl.remove(external_key));
  EXPECT_EQ(tbl.size(), 19);
  EXPECT_EQ(tbl.search(10), nullptr);
  
  // Verify other elements are intact
  for (int i = 0; i < 20; ++i)
    {
      if (i == 10) continue;
      EXPECT_NE(tbl.search(i), nullptr) << "Element " << i << " should still exist";
    }
}

// Test remove with internal key (reference from the table bucket)
TEST(OLhashTable, RemoveWithInternalKey)
{
  OLhashTable<int> tbl(100);
  
  // Insert elements
  for (int i = 0; i < 20; ++i)
    EXPECT_NE(tbl.insert(i), nullptr);
  
  EXPECT_EQ(tbl.size(), 20);
  
  // Remove using internal key (reference from search result)
  auto ptr = tbl.search(10);
  ASSERT_NE(ptr, nullptr);
  EXPECT_NO_THROW(tbl.remove(*ptr));  // *ptr is internal reference
  EXPECT_EQ(tbl.size(), 19);
  EXPECT_EQ(tbl.search(10), nullptr);
}

// Test behavior with many collisions (stress test linear probing)
TEST(OLhashTable, ManyCollisions)
{
  // Use a small table to force many collisions
  OLhashTable<int> tbl(17);  // Small prime
  
  // Insert enough elements to cause collisions
  const int num_elements = 15;
  for (int i = 0; i < num_elements; ++i)
    EXPECT_NE(tbl.insert(i), nullptr);
  
  EXPECT_EQ(tbl.size(), num_elements);
  
  // Verify all elements are findable
  for (int i = 0; i < num_elements; ++i)
    EXPECT_NE(tbl.search(i), nullptr) << "Element " << i << " not found";
  
  // Remove every other element
  for (int i = 0; i < num_elements; i += 2)
    {
      auto ptr = tbl.search(i);
      ASSERT_NE(ptr, nullptr);
      tbl.remove(*ptr);
    }
  
  // Verify remaining elements are still findable
  for (int i = 1; i < num_elements; i += 2)
    EXPECT_NE(tbl.search(i), nullptr) << "Element " << i << " not found after removals";
  
  // Verify removed elements are gone
  for (int i = 0; i < num_elements; i += 2)
    EXPECT_EQ(tbl.search(i), nullptr) << "Element " << i << " should be removed";
}

// Test that capacity doesn't change after failed removes
TEST(OLhashTable, RemoveNonExistentDoesNotChangeCapacity)
{
  OLhashTable<int> tbl(100);
  
  // Insert elements
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(tbl.insert(i * 2), nullptr);  // Even numbers
  
  const size_t original_capacity = tbl.capacity();
  const size_t original_size = tbl.size();
  
  // Try to remove many non-existent keys
  for (int attempt = 0; attempt < 100; ++attempt)
    {
      int non_existent = attempt * 2 + 1;  // Odd numbers don't exist
      EXPECT_THROW(tbl.remove(non_existent), std::domain_error);
    }
  
  // Capacity should be unchanged
  EXPECT_EQ(tbl.capacity(), original_capacity)
    << "Capacity changed after failed remove attempts";
  
  // Size should be unchanged
  EXPECT_EQ(tbl.size(), original_size);
  
  // All elements should still be findable
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(tbl.search(i * 2), nullptr) << "Element " << i * 2 << " not found";
}

// ============================================================================
// STRESS TESTS / FUZZING
// ============================================================================

// Fuzzing test: random operations with oracle verification
TEST(OLhashTable, Fuzz_RandomOperationsWithOracle)
{
  // Use large table to avoid resize
  OLhashTable<int> tbl(20000);
  set<int> oracle;
  
  mt19937 rng(42);
  uniform_int_distribution<int> key_dist(0, 5000);
  uniform_int_distribution<int> op_dist(0, 2);
  
  const int num_operations = 8000;
  
  for (int i = 0; i < num_operations; ++i)
    {
      int key = key_dist(rng);
      int op = op_dist(rng);
      
      switch (op)
        {
        case 0:  // Insert
          {
            bool in_oracle = oracle.count(key) > 0;
            auto ptr = tbl.insert(key);
            if (ptr != nullptr)
              {
                EXPECT_FALSE(in_oracle) << "Insert succeeded but oracle had key " << key;
                oracle.insert(key);
              }
            else
              {
                EXPECT_TRUE(in_oracle) << "Insert failed but oracle didn't have key " << key;
              }
            break;
          }
        case 1:  // Remove
          {
            bool in_oracle = oracle.count(key) > 0;
            if (in_oracle)
              {
                try
                  {
                    tbl.remove(key);
                    oracle.erase(key);
                  }
                catch (const domain_error &)
                  {
                    FAIL() << "Remove threw for key " << key << " that was in oracle";
                  }
              }
            else
              {
                EXPECT_THROW(tbl.remove(key), domain_error);
              }
            break;
          }
        case 2:  // Search
          {
            auto ptr = tbl.search(key);
            bool in_oracle = oracle.count(key) > 0;
            EXPECT_EQ(ptr != nullptr, in_oracle) << "Search mismatch for key " << key;
            if (ptr)
              EXPECT_EQ(*ptr, key);
            break;
          }
        }
      
      ASSERT_EQ(tbl.size(), oracle.size()) << "Size mismatch at operation " << i;
    }
  
  // Final verification
  for (int key : oracle)
    ASSERT_NE(tbl.search(key), nullptr) << "Final: key " << key << " missing";
}

// Stress test: fill and empty completely
TEST(OLhashTable, Stress_FillAndEmpty)
{
  OLhashTable<int> tbl(1000);
  const size_t target = tbl.capacity() - 1;
  
  // Fill
  for (size_t i = 0; i < target; ++i)
    {
      auto ptr = tbl.insert(static_cast<int>(i));
      ASSERT_NE(ptr, nullptr) << "Insert failed at i=" << i;
    }
  
  EXPECT_EQ(tbl.size(), target);
  
  // Verify
  for (size_t i = 0; i < target; ++i)
    ASSERT_NE(tbl.search(static_cast<int>(i)), nullptr);
  
  // Empty in random order
  vector<int> keys(target);
  iota(keys.begin(), keys.end(), 0);
  
  mt19937 rng(123);
  shuffle(keys.begin(), keys.end(), rng);
  
  for (size_t i = 0; i < target; ++i)
    {
      EXPECT_NO_THROW(tbl.remove(keys[i]));
      EXPECT_EQ(tbl.size(), target - i - 1);
    }
  
  EXPECT_TRUE(tbl.is_empty());
}

// Stress test: linear probing with forced collisions
TEST(OLhashTable, Stress_LinearProbingCollisions)
{
  // Bad hash that causes collisions
  auto bad_hash = [](const int &) -> size_t { return 0; };
  
  OLhashTable<int> tbl(100, bad_hash);
  
  const int num_elements = 50;
  for (int i = 0; i < num_elements; ++i)
    {
      auto ptr = tbl.insert(i);
      ASSERT_NE(ptr, nullptr) << "Insert failed at i=" << i;
    }
  
  EXPECT_EQ(tbl.size(), num_elements);
  
  // All elements should be findable
  for (int i = 0; i < num_elements; ++i)
    {
      auto ptr = tbl.search(i);
      ASSERT_NE(ptr, nullptr) << "Element " << i << " not found";
      EXPECT_EQ(*ptr, i);
    }
  
  // Remove in order
  for (int i = 0; i < num_elements; ++i)
    {
      EXPECT_NO_THROW(tbl.remove(i));
      EXPECT_EQ(tbl.search(i), nullptr);
    }
  
  EXPECT_TRUE(tbl.is_empty());
}

// Stress test: insert/remove cycles
TEST(OLhashTable, Stress_InsertRemoveCycles)
{
  OLhashTable<int> tbl(100);
  
  const int cycles = 100;
  const int elements_per_cycle = 50;
  
  for (int cycle = 0; cycle < cycles; ++cycle)
    {
      for (int i = 0; i < elements_per_cycle; ++i)
        {
          int key = cycle * elements_per_cycle + i;
          ASSERT_NE(tbl.insert(key), nullptr);
        }
      
      EXPECT_EQ(tbl.size(), elements_per_cycle);
      
      for (int i = 0; i < elements_per_cycle; ++i)
        {
          int key = cycle * elements_per_cycle + i;
          EXPECT_NO_THROW(tbl.remove(key));
        }
      
      EXPECT_TRUE(tbl.is_empty());
    }
}

// Stress test: resize operations
TEST(OLhashTable, Stress_ResizeOperations)
{
  OLhashTable<int> tbl(10);
  
  set<int> oracle;
  mt19937 rng(999);
  uniform_int_distribution<int> key_dist(0, 100000);
  
  const int num_inserts = 5000;
  for (int i = 0; i < num_inserts; ++i)
    {
      int key = key_dist(rng);
      auto ptr = tbl.insert(key);
      if (oracle.count(key) == 0 && ptr != nullptr)
        oracle.insert(key);
    }
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  // Verify all survived
  for (int key : oracle)
    ASSERT_NE(tbl.search(key), nullptr) << "Key " << key << " lost after resize";
}

// Fuzz test: interleaved operations
TEST(OLhashTable, Fuzz_InterleavedOperations)
{
  // Use large table to avoid resize
  OLhashTable<int> tbl(5000);
  set<int> oracle;
  
  mt19937 rng(7777);
  uniform_int_distribution<int> key_dist(0, 1000);
  uniform_real_distribution<double> prob_dist(0.0, 1.0);
  
  const int num_ops = 5000;
  
  for (int i = 0; i < num_ops; ++i)
    {
      int key = key_dist(rng);
      double prob = prob_dist(rng);
      
      if (prob < 0.4)
        {
          auto ptr = tbl.insert(key);
          if (ptr != nullptr)
            oracle.insert(key);
        }
      else if (prob < 0.6)
        {
          if (oracle.count(key))
            {
              try
                {
                  tbl.remove(key);
                  oracle.erase(key);
                }
              catch (const domain_error &)
                {
                  FAIL() << "Remove threw for key " << key << " that was in oracle";
                }
            }
        }
      else
        {
          auto ptr = tbl.search(key);
          EXPECT_EQ(ptr != nullptr, oracle.count(key) > 0);
        }
      
      if (i % 500 == 0)
        ASSERT_EQ(tbl.size(), oracle.size());
    }
  
  EXPECT_EQ(tbl.size(), oracle.size());
}

// Stress test: with auto-resize enabled
TEST(OLhashTable, Stress_WithAutoResize)
{
  OLhashTable<int> tbl(10);  // Small initial size, auto-resize enabled
  set<int> oracle;
  
  mt19937 rng(333);
  uniform_int_distribution<int> key_dist(0, 50000);
  
  const int num_inserts = 3000;
  for (int i = 0; i < num_inserts; ++i)
    {
      int key = key_dist(rng);
      auto ptr = tbl.insert(key);
      if (ptr != nullptr)
        oracle.insert(key);
    }
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  for (int key : oracle)
    {
      auto ptr = tbl.search(key);
      ASSERT_NE(ptr, nullptr) << "Key " << key << " lost during resize";
    }
}

// ============================================================================
// DELETED CLEANUP TESTS (Knuth's optimization)
// ============================================================================

// Helper to count bucket states
struct BucketStats
{
  size_t empty = 0;
  size_t busy = 0;
  size_t deleted = 0;
};

template <typename HashTable>
BucketStats count_bucket_states(const HashTable &tbl)
{
  BucketStats stats;
  for (size_t i = 0; i < tbl.capacity(); ++i)
    {
      switch (tbl.table[i].status)
        {
        case HashTable::EMPTY: ++stats.empty; break;
        case HashTable::BUSY: ++stats.busy; break;
        case HashTable::DELETED: ++stats.deleted; break;
        }
    }
  return stats;
}

// Test: removing last element in chain should mark as EMPTY, not DELETED
TEST(OLhashTable, DeletedCleanup_LastInChainBecomesEmpty)
{
  // Use bad hash to force all elements into same chain
  auto bad_hash = [](const int &) -> size_t { return 0; };
  OLhashTable<int> tbl(100, bad_hash);
  
  // Insert elements: they will all collide at index 0
  // Chain: [0] -> [1] -> [2] -> [3] -> [4]
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  
  auto before = count_bucket_states(tbl);
  EXPECT_EQ(before.busy, 5);
  EXPECT_EQ(before.deleted, 0);
  
  // Remove last element (4) - should become EMPTY since next is EMPTY
  tbl.remove(4);
  
  auto after = count_bucket_states(tbl);
  EXPECT_EQ(after.busy, 4);
  EXPECT_EQ(after.deleted, 0) << "Last element should become EMPTY, not DELETED";
  EXPECT_EQ(after.empty, before.empty + 1);
  
  // Verify remaining elements are still findable
  for (int i = 0; i < 4; ++i)
    EXPECT_NE(tbl.search(i), nullptr) << "Element " << i << " should still exist";
  
  EXPECT_EQ(tbl.search(4), nullptr);
}

// Test: backward propagation of EMPTY status
TEST(OLhashTable, DeletedCleanup_BackwardPropagation)
{
  // Use bad hash to force chain
  auto bad_hash = [](const int &) -> size_t { return 0; };
  OLhashTable<int> tbl(100, bad_hash);
  
  // Insert chain: positions 0,1,2,3,4
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  
  // Remove middle elements first (they become DELETED)
  tbl.remove(3);  // position 3 -> DELETED (next is BUSY at 4)
  tbl.remove(2);  // position 2 -> DELETED (next is DELETED at 3)
  
  auto mid_stats = count_bucket_states(tbl);
  EXPECT_EQ(mid_stats.busy, 3);  // 0, 1, 4 are BUSY
  EXPECT_EQ(mid_stats.deleted, 2);  // 2, 3 are DELETED
  
  // Now remove element 4 (last in chain)
  // This should trigger backward cleanup: 4->EMPTY, 3->EMPTY, 2->EMPTY
  tbl.remove(4);
  
  auto final_stats = count_bucket_states(tbl);
  EXPECT_EQ(final_stats.busy, 2);  // 0, 1 are BUSY
  EXPECT_EQ(final_stats.deleted, 0) << "All trailing DELETED should become EMPTY";
  
  // Verify remaining elements
  EXPECT_NE(tbl.search(0), nullptr);
  EXPECT_NE(tbl.search(1), nullptr);
  EXPECT_EQ(tbl.search(2), nullptr);
  EXPECT_EQ(tbl.search(3), nullptr);
  EXPECT_EQ(tbl.search(4), nullptr);
}

// Test: DELETED in middle of chain stays DELETED
TEST(OLhashTable, DeletedCleanup_MiddleStaysDeleted)
{
  auto bad_hash = [](const int &) -> size_t { return 0; };
  OLhashTable<int> tbl(100, bad_hash);
  
  // Insert chain: 0,1,2,3,4
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  
  // Remove element in middle (2) - should stay DELETED because 3,4 follow
  tbl.remove(2);
  
  auto stats = count_bucket_states(tbl);
  EXPECT_EQ(stats.busy, 4);
  EXPECT_EQ(stats.deleted, 1) << "Middle element should stay DELETED";
  
  // All other elements still findable
  for (int i = 0; i < 5; ++i)
    {
      if (i == 2)
        EXPECT_EQ(tbl.search(i), nullptr);
      else
        EXPECT_NE(tbl.search(i), nullptr);
    }
}

// Test: no DELETED accumulation after many insert/remove cycles
TEST(OLhashTable, DeletedCleanup_NoAccumulationAfterCycles)
{
  OLhashTable<int> tbl(100);
  
  // Perform many insert/remove cycles
  const int cycles = 50;
  const int elements = 30;
  
  for (int cycle = 0; cycle < cycles; ++cycle)
    {
      // Insert
      for (int i = 0; i < elements; ++i)
        tbl.insert(cycle * 1000 + i);
      
      // Remove all
      for (int i = 0; i < elements; ++i)
        tbl.remove(cycle * 1000 + i);
    }
  
  // After all cycles, table should be mostly EMPTY with no DELETED
  auto stats = count_bucket_states(tbl);
  EXPECT_EQ(stats.busy, 0);
  EXPECT_EQ(stats.deleted, 0) << "Should have no DELETED after complete removal";
  EXPECT_TRUE(tbl.is_empty());
}

// Test: cleanup with wrap-around at table boundary
TEST(OLhashTable, DeletedCleanup_WrapAround)
{
  // Hash function that puts elements near end of table
  OLhashTable<int> tbl(17);  // Prime size
  
  // Force insertion near end by using specific values
  // that hash near len-1
  auto near_end_hash = [](const int &k) -> size_t { 
    return static_cast<size_t>(k + 15); // Will wrap around
  };
  
  OLhashTable<int> tbl2(17, near_end_hash);
  
  // Insert elements that will wrap around
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl2.insert(i), nullptr);
  
  EXPECT_EQ(tbl2.size(), 5);
  
  // Remove all - should handle wrap-around correctly
  for (int i = 4; i >= 0; --i)
    {
      EXPECT_NO_THROW(tbl2.remove(i));
    }
  
  auto stats = count_bucket_states(tbl2);
  EXPECT_EQ(stats.deleted, 0) << "Wrap-around cleanup should leave no DELETED";
  EXPECT_TRUE(tbl2.is_empty());
}

// Stress test: verify no DELETED accumulation with random operations
TEST(OLhashTable, DeletedCleanup_StressNoAccumulation)
{
  OLhashTable<int> tbl(500);
  set<int> oracle;
  
  mt19937 rng(12345);
  uniform_int_distribution<int> key_dist(0, 200);
  uniform_real_distribution<double> op_dist(0.0, 1.0);
  
  const int num_ops = 5000;
  
  for (int i = 0; i < num_ops; ++i)
    {
      int key = key_dist(rng);
      double op = op_dist(rng);
      
      if (op < 0.5)
        {
          auto ptr = tbl.insert(key);
          if (ptr) oracle.insert(key);
        }
      else if (oracle.count(key))
        {
          tbl.remove(key);
          oracle.erase(key);
        }
    }
  
  auto stats = count_bucket_states(tbl);
  
  // The number of DELETED should be minimal compared to capacity
  // With Knuth's cleanup, DELETED only accumulates in middle of chains
  double deleted_ratio = static_cast<double>(stats.deleted) / tbl.capacity();
  EXPECT_LT(deleted_ratio, 0.1) 
    << "DELETED ratio should be low with cleanup. Got " 
    << stats.deleted << "/" << tbl.capacity();
  
  // Verify integrity
  EXPECT_EQ(tbl.size(), oracle.size());
  for (int key : oracle)
    EXPECT_NE(tbl.search(key), nullptr);
}

// ============================================================================
// COPY/MOVE SEMANTICS TESTS
// ============================================================================

TEST(OLhashTable, CopyConstructor)
{
  OLhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  OLhashTable<int> copy(original);
  
  EXPECT_EQ(copy.size(), original.size());
  EXPECT_EQ(copy.capacity(), original.capacity());
  
  // Verify all elements exist in both
  for (int i = 0; i < 50; ++i)
    {
      EXPECT_NE(original.search(i), nullptr);
      EXPECT_NE(copy.search(i), nullptr);
    }
  
  // Modify copy, original should be unchanged
  copy.remove(25);
  EXPECT_EQ(copy.search(25), nullptr);
  EXPECT_NE(original.search(25), nullptr);
}

TEST(OLhashTable, MoveConstructor)
{
  OLhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  const size_t orig_size = original.size();
  const size_t orig_cap = original.capacity();
  
  OLhashTable<int> moved(std::move(original));
  
  EXPECT_EQ(moved.size(), orig_size);
  EXPECT_EQ(moved.capacity(), orig_cap);
  
  // Verify all elements exist in moved
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(moved.search(i), nullptr);
}

TEST(OLhashTable, CopyAssignment)
{
  OLhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  OLhashTable<int> copy(10);
  copy.insert(999);
  
  copy = original;
  
  EXPECT_EQ(copy.size(), original.size());
  
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(copy.search(i), nullptr);
  
  EXPECT_EQ(copy.search(999), nullptr);  // Old element gone
}

TEST(OLhashTable, MoveAssignment)
{
  OLhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  const size_t orig_size = original.size();
  
  OLhashTable<int> target(10);
  target.insert(999);
  
  target = std::move(original);
  
  EXPECT_EQ(target.size(), orig_size);
  
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(target.search(i), nullptr);
}

TEST(OLhashTable, SelfAssignment)
{
  OLhashTable<int> tbl(100);
  for (int i = 0; i < 50; ++i)
    tbl.insert(i);
  
  tbl = tbl;  // Self-assignment
  
  EXPECT_EQ(tbl.size(), 50);
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

// ============================================================================
// DELETED SLOT REUSE TESTS
// ============================================================================

TEST(OLhashTable, DeletedSlotReuse)
{
  auto bad_hash = [](const int &) -> size_t { return 0; };
  OLhashTable<int> tbl(100, bad_hash);
  
  // Insert chain: 0,1,2,3,4 at positions 0,1,2,3,4
  for (int i = 0; i < 5; ++i)
    tbl.insert(i);
  
  // Remove middle element (2) - becomes DELETED
  tbl.remove(2);
  
  auto before = count_bucket_states(tbl);
  EXPECT_EQ(before.deleted, 1);
  
  // Insert new element - should reuse DELETED slot at position 2
  tbl.insert(100);
  
  auto after = count_bucket_states(tbl);
  EXPECT_EQ(after.deleted, 0) << "DELETED slot should be reused";
  EXPECT_EQ(after.busy, 5);
  
  // All elements should be findable
  EXPECT_NE(tbl.search(0), nullptr);
  EXPECT_NE(tbl.search(1), nullptr);
  EXPECT_EQ(tbl.search(2), nullptr);  // Was removed
  EXPECT_NE(tbl.search(3), nullptr);
  EXPECT_NE(tbl.search(4), nullptr);
  EXPECT_NE(tbl.search(100), nullptr);  // New element
}

// ============================================================================
// SEARCH_OR_INSERT AND CONTAINS_OR_INSERT TESTS
// ============================================================================

TEST(OLhashTable, SearchOrInsert_NewKey)
{
  OLhashTable<int> tbl(100);
  
  auto ptr = tbl.search_or_insert(42);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 42);
  EXPECT_EQ(tbl.size(), 1);
}

TEST(OLhashTable, SearchOrInsert_ExistingKey)
{
  OLhashTable<int> tbl(100);
  tbl.insert(42);
  
  auto ptr = tbl.search_or_insert(42);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 42);
  EXPECT_EQ(tbl.size(), 1);  // No duplicate
}

TEST(OLhashTable, ContainsOrInsert_NewKey)
{
  OLhashTable<int> tbl(100);
  
  auto [ptr, existed] = tbl.contains_or_insert(42);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 42);
  EXPECT_FALSE(existed);
  EXPECT_EQ(tbl.size(), 1);
}

TEST(OLhashTable, ContainsOrInsert_ExistingKey)
{
  OLhashTable<int> tbl(100);
  tbl.insert(42);
  
  auto [ptr, existed] = tbl.contains_or_insert(42);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 42);
  EXPECT_TRUE(existed);
  EXPECT_EQ(tbl.size(), 1);
}

// ============================================================================
// REHASH TESTS
// ============================================================================

TEST(OLhashTable, ManualRehash)
{
  OLhashTable<int> tbl(100);
  set<int> oracle;
  
  for (int i = 0; i < 50; ++i)
    {
      tbl.insert(i);
      oracle.insert(i);
    }
  
  // Remove half to create DELETED slots
  for (int i = 0; i < 50; i += 2)
    {
      tbl.remove(i);
      oracle.erase(i);
    }
  
  auto before = count_bucket_states(tbl);
  (void)before;
  // Some DELETED may exist (those in middle of chains)
  
  // Manual rehash should eliminate all DELETED
  tbl.rehash();
  
  auto after = count_bucket_states(tbl);
  EXPECT_EQ(after.deleted, 0) << "Rehash should eliminate all DELETED";
  EXPECT_EQ(after.busy, oracle.size());
  
  // All remaining elements should be findable
  for (int key : oracle)
    EXPECT_NE(tbl.search(key), nullptr);
}

TEST(OLhashTable, ResizeUp)
{
  OLhashTable<int> tbl(50);
  
  for (int i = 0; i < 30; ++i)
    tbl.insert(i);
  
  const size_t old_cap = tbl.capacity();
  tbl.resize(200);
  
  EXPECT_GT(tbl.capacity(), old_cap);
  EXPECT_EQ(tbl.size(), 30);
  
  for (int i = 0; i < 30; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

TEST(OLhashTable, ResizeDown)
{
  OLhashTable<int> tbl(200);
  
  for (int i = 0; i < 30; ++i)
    tbl.insert(i);
  
  tbl.resize(50);
  
  EXPECT_EQ(tbl.size(), 30);
  
  for (int i = 0; i < 30; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST(OLhashTable, EmptyTableOperations)
{
  OLhashTable<int> tbl(100);
  
  EXPECT_TRUE(tbl.is_empty());
  EXPECT_EQ(tbl.size(), 0);
  EXPECT_EQ(tbl.search(42), nullptr);
  EXPECT_FALSE(tbl.has(42));
  EXPECT_FALSE(tbl.contains(42));
  EXPECT_THROW(tbl.remove(42), std::domain_error);
}

TEST(OLhashTable, SingleElement)
{
  OLhashTable<int> tbl(100);
  
  tbl.insert(42);
  EXPECT_EQ(tbl.size(), 1);
  EXPECT_NE(tbl.search(42), nullptr);
  
  tbl.remove(42);
  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());
  EXPECT_EQ(tbl.search(42), nullptr);
  
  // Table should be clean (no DELETED for single element at end)
  auto stats = count_bucket_states(tbl);
  EXPECT_EQ(stats.deleted, 0);
}

TEST(OLhashTable, DuplicateInsertReturnsNull)
{
  OLhashTable<int> tbl(100);
  
  auto first = tbl.insert(42);
  ASSERT_NE(first, nullptr);
  
  auto second = tbl.insert(42);
  EXPECT_EQ(second, nullptr) << "Duplicate insert should return nullptr";
  
  EXPECT_EQ(tbl.size(), 1);
}

TEST(OLhashTable, HasAndContains)
{
  OLhashTable<int> tbl(100);
  
  EXPECT_FALSE(tbl.has(42));
  EXPECT_FALSE(tbl.contains(42));
  
  tbl.insert(42);
  
  EXPECT_TRUE(tbl.has(42));
  EXPECT_TRUE(tbl.contains(42));
  EXPECT_FALSE(tbl.has(43));
  EXPECT_FALSE(tbl.contains(43));
}

TEST(OLhashTable, Find)
{
  OLhashTable<int> tbl(100);
  tbl.insert(42);
  
  EXPECT_NO_THROW({
    int& ref = tbl.find(42);
    EXPECT_EQ(ref, 42);
  });
  
  EXPECT_THROW(tbl.find(999), std::domain_error);
}

// ============================================================================
// ITERATOR TESTS
// ============================================================================

TEST(OLhashTable, IteratorBasic)
{
  OLhashTable<int> tbl(100);
  set<int> oracle;
  
  for (int i = 0; i < 50; ++i)
    {
      tbl.insert(i);
      oracle.insert(i);
    }
  
  set<int> visited;
  for (auto it = tbl.get_it(); it.has_curr(); it.next())
    visited.insert(it.get_curr());
  
  EXPECT_EQ(visited, oracle);
}

TEST(OLhashTable, IteratorEmpty)
{
  OLhashTable<int> tbl(100);
  
  auto it = tbl.get_it();
  EXPECT_FALSE(it.has_curr());
}

TEST(OLhashTable, IteratorSingleElement)
{
  OLhashTable<int> tbl(100);
  tbl.insert(42);
  
  auto it = tbl.get_it();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 42);
  
  it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST(OLhashTable, IteratorDelete)
{
  OLhashTable<int> tbl(100);
  
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  // Delete all elements via iterator
  auto it = tbl.get_it();
  while (it.has_curr())
    it.del();
  
  EXPECT_TRUE(tbl.is_empty());
}

// ============================================================================
// STATS TEST
// ============================================================================

TEST(OLhashTable, StatsCorrectness)
{
  auto bad_hash = [](const int &) -> size_t { return 0; };
  OLhashTable<int> tbl(100, bad_hash);
  
  // Insert chain
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  // Remove some in middle
  tbl.remove(3);
  tbl.remove(5);
  tbl.remove(7);
  
  auto stats = tbl.stats();
  
  EXPECT_EQ(stats.num_busy, 7);
  // Some DELETED may exist depending on cleanup
  EXPECT_EQ(stats.num_busy + stats.num_deleted + stats.num_empty, tbl.capacity());
}

// ============================================================================
// FUNCTIONAL METHODS TEST  
// ============================================================================

TEST(OLhashTable, ForEach)
{
  OLhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  int sum = 0;
  tbl.for_each([&sum](int x) { sum += x; });
  
  EXPECT_EQ(sum, 45);  // 0+1+2+...+9
}

TEST(OLhashTable, All)
{
  OLhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i * 2);  // Even numbers
  
  EXPECT_TRUE(tbl.all([](int x) { return x % 2 == 0; }));
  EXPECT_FALSE(tbl.all([](int x) { return x > 5; }));
}

TEST(OLhashTable, Exists)
{
  OLhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  EXPECT_TRUE(tbl.exists([](int x) { return x == 5; }));
  EXPECT_FALSE(tbl.exists([](int x) { return x == 100; }));
}

TEST(OLhashTable, Filter)
{
  OLhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  auto evens = tbl.filter([](int x) { return x % 2 == 0; });
  
  EXPECT_EQ(evens.size(), 5);
}
