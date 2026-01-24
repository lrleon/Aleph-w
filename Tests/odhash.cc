
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
 * @file odhash.cc
 * @brief Tests for Odhash
 */
# include <gtest/gtest.h>

# include <random>
# include <set>
# include <chrono>

# include <tpl_odhash.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(ODhashTable, Simplest)
{
  ODhashTable<int> tbl(100);

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

inline size_t fst_hast(const MyRecord & r) noexcept
{
  return dft_hash_fct(r.key);
}

inline size_t snd_hash(const MyRecord & r) noexcept
{
  return snd_hash_fct(r.key);
}


TEST(ODhashTable, Map)
{
  ODhashTable<MyRecord, MyRecord::Eq> tbl(10, fst_hast, snd_hash, MyRecord::Eq());

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

TEST(ODhashTable, KeyToBucketRoundTrip)
{
  ODhashTable<int> tbl;
  auto *ptr = tbl.insert(5);
  ASSERT_NE(ptr, nullptr);

  auto *bucket = decltype(tbl)::key_to_bucket(ptr);
  ASSERT_NE(bucket, nullptr);
  EXPECT_EQ(bucket->key, 5);
  EXPECT_EQ(bucket->status, decltype(tbl)::BUSY);

  EXPECT_NO_THROW(tbl.remove(5));
  EXPECT_EQ(tbl.search(5), nullptr);
}

// Test that removing a non-existent key does NOT corrupt the table state.
// This test exposes a bug where the old remove() would decrement N and
// corrupt probe_counters even when the key was not found.
TEST(ODhashTable, RemoveNonExistentKeyPreservesTableIntegrity)
{
  ODhashTable<int> tbl(100);
  
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
TEST(ODhashTable, RemoveWithExternalKey)
{
  ODhashTable<int> tbl(100);
  
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
TEST(ODhashTable, RemoveWithInternalKey)
{
  ODhashTable<int> tbl(100);
  
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

// Test that verifies capacity doesn't change after failed remove.
// The old buggy code would call rehash() on every failed remove,
// potentially changing capacity. The new code doesn't rehash.
TEST(ODhashTable, RemoveNonExistentDoesNotRehash)
{
  ODhashTable<int> tbl(100);
  
  // Insert elements to create some collision chains
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(tbl.insert(i * 2), nullptr);  // Even numbers
  
  const size_t original_capacity = tbl.capacity();
  const size_t original_size = tbl.size();
  
  // Try to remove many non-existent keys
  // Old buggy code would rehash on each failed remove
  for (int attempt = 0; attempt < 100; ++attempt)
    {
      int non_existent = attempt * 2 + 1;  // Odd numbers don't exist
      EXPECT_THROW(tbl.remove(non_existent), std::domain_error);
    }
  
  // Capacity should be unchanged (no rehash occurred)
  EXPECT_EQ(tbl.capacity(), original_capacity)
    << "Capacity changed - possible unnecessary rehash on failed remove";
  
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
TEST(ODhashTable, Fuzz_RandomOperationsWithOracle)
{
  // Use large table to avoid resize during test
  ODhashTable<int> tbl(20000);
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
                EXPECT_FALSE(in_oracle) << "Insert succeeded for key " << key 
                  << " but oracle already had it";
                oracle.insert(key);
              }
            else
              {
                EXPECT_TRUE(in_oracle) << "Insert failed for key " << key 
                  << " but oracle didn't have it";
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
                    oracle.erase(key);  // Only erase if remove succeeded
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
            EXPECT_EQ(ptr != nullptr, in_oracle)
              << "Search mismatch for key " << key;
            if (ptr)
              EXPECT_EQ(*ptr, key);
            break;
          }
        }
      
      ASSERT_EQ(tbl.size(), oracle.size()) 
        << "Size mismatch at operation " << i << ", key=" << key;
    }
  
  // Final verification
  for (int key : oracle)
    {
      auto ptr = tbl.search(key);
      ASSERT_NE(ptr, nullptr) << "Final check: key " << key << " missing";
    }
}

// Stress test: fill table to near capacity then empty it completely
TEST(ODhashTable, Stress_FillAndEmpty)
{
  ODhashTable<int> tbl(1000);
  const size_t target = tbl.capacity() - 1;  // Leave one empty as sentinel
  
  // Fill the table
  for (size_t i = 0; i < target; ++i)
    {
      auto ptr = tbl.insert(static_cast<int>(i));
      ASSERT_NE(ptr, nullptr) << "Insert failed at i=" << i;
    }
  
  EXPECT_EQ(tbl.size(), target);
  
  // Verify all elements
  for (size_t i = 0; i < target; ++i)
    {
      ASSERT_NE(tbl.search(static_cast<int>(i)), nullptr) 
        << "Element " << i << " not found after fill";
    }
  
  // Empty the table in random order
  vector<int> keys(target);
  iota(keys.begin(), keys.end(), 0);
  
  mt19937 rng(123);
  shuffle(keys.begin(), keys.end(), rng);
  
  for (size_t i = 0; i < target; ++i)
    {
      EXPECT_NO_THROW(tbl.remove(keys[i])) << "Remove failed for key " << keys[i];
      EXPECT_EQ(tbl.size(), target - i - 1);
    }
  
  EXPECT_TRUE(tbl.is_empty());
}

// Stress test: many collisions (all keys hash to same bucket)
TEST(ODhashTable, Stress_ManyCollisions)
{
  // Custom hash that always returns the same value - forces maximum collisions
  auto bad_hash = [](const int &) -> size_t { return 42; };
  
  ODhashTable<int> tbl(100, bad_hash, bad_hash);
  
  // Insert elements - all will collide
  const int num_elements = 50;
  for (int i = 0; i < num_elements; ++i)
    {
      auto ptr = tbl.insert(i);
      ASSERT_NE(ptr, nullptr) << "Insert failed at i=" << i << " with bad hash";
    }
  
  EXPECT_EQ(tbl.size(), num_elements);
  
  // Verify all elements are findable despite collisions
  for (int i = 0; i < num_elements; ++i)
    {
      auto ptr = tbl.search(i);
      ASSERT_NE(ptr, nullptr) << "Element " << i << " not found with collision";
      EXPECT_EQ(*ptr, i);
    }
  
  // Remove in reverse order
  for (int i = num_elements - 1; i >= 0; --i)
    {
      EXPECT_NO_THROW(tbl.remove(i));
      EXPECT_EQ(tbl.search(i), nullptr);
    }
  
  EXPECT_TRUE(tbl.is_empty());
}

// Stress test: repeated insert/remove cycles
TEST(ODhashTable, Stress_InsertRemoveCycles)
{
  ODhashTable<int> tbl(100);
  
  const int cycles = 100;
  const int elements_per_cycle = 50;
  
  for (int cycle = 0; cycle < cycles; ++cycle)
    {
      // Insert phase
      for (int i = 0; i < elements_per_cycle; ++i)
        {
          int key = cycle * elements_per_cycle + i;
          auto ptr = tbl.insert(key);
          ASSERT_NE(ptr, nullptr) << "Insert failed at cycle " << cycle << ", i=" << i;
        }
      
      EXPECT_EQ(tbl.size(), elements_per_cycle);
      
      // Remove phase - remove all
      for (int i = 0; i < elements_per_cycle; ++i)
        {
          int key = cycle * elements_per_cycle + i;
          EXPECT_NO_THROW(tbl.remove(key));
        }
      
      EXPECT_TRUE(tbl.is_empty());
    }
}

// Stress test: trigger resize operations
TEST(ODhashTable, Stress_ResizeOperations)
{
  // Start with small table that will need to resize
  ODhashTable<int> tbl(10);
  
  set<int> oracle;
  mt19937 rng(999);
  uniform_int_distribution<int> key_dist(0, 100000);
  
  // Insert many elements to trigger multiple resizes
  const int num_inserts = 5000;
  for (int i = 0; i < num_inserts; ++i)
    {
      int key = key_dist(rng);
      auto ptr = tbl.insert(key);
      if (oracle.count(key) == 0 && ptr != nullptr)
        oracle.insert(key);
    }
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  // Verify all elements survived resizes
  for (int key : oracle)
    {
      auto ptr = tbl.search(key);
      ASSERT_NE(ptr, nullptr) << "Key " << key << " lost after resize";
    }
  
  // Remove half and verify
  size_t to_remove = oracle.size() / 2;
  auto it = oracle.begin();
  for (size_t i = 0; i < to_remove; ++i, ++it)
    {
      EXPECT_NO_THROW(tbl.remove(*it));
    }
  oracle.erase(oracle.begin(), it);
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  // Verify remaining elements
  for (int key : oracle)
    {
      ASSERT_NE(tbl.search(key), nullptr) << "Key " << key << " missing after partial remove";
    }
}

// Fuzzing: interleaved search during insert/remove
TEST(ODhashTable, Fuzz_InterleavedOperations)
{
  // Use large table to avoid resize
  ODhashTable<int> tbl(5000);
  set<int> oracle;
  
  mt19937 rng(7777);
  uniform_int_distribution<int> key_dist(0, 1000);
  uniform_real_distribution<double> prob_dist(0.0, 1.0);
  
  const int num_ops = 5000;
  
  for (int i = 0; i < num_ops; ++i)
    {
      int key = key_dist(rng);
      double prob = prob_dist(rng);
      
      if (prob < 0.4)  // 40% insert
        {
          auto ptr = tbl.insert(key);
          if (ptr != nullptr)
            oracle.insert(key);
        }
      else if (prob < 0.6)  // 20% remove
        {
          if (oracle.count(key))
            {
              try
                {
                  tbl.remove(key);
                  oracle.erase(key);  // Only erase if remove succeeded
                }
              catch (const domain_error &)
                {
                  FAIL() << "Remove threw for key " << key << " that was in oracle";
                }
            }
        }
      else  // 40% search
        {
          auto ptr = tbl.search(key);
          bool in_oracle = oracle.count(key) > 0;
          EXPECT_EQ(ptr != nullptr, in_oracle)
            << "Search mismatch for key " << key;
        }
      
      // Periodic consistency check
      if (i % 500 == 0)
        ASSERT_EQ(tbl.size(), oracle.size()) << "Size mismatch at i=" << i;
    }
  
  // Final check
  EXPECT_EQ(tbl.size(), oracle.size());
}

// Stress test: with auto-resize enabled
TEST(ODhashTable, Stress_WithAutoResize)
{
  // Enable auto-resize
  ODhashTable<int> tbl(10);  // Small initial size
  set<int> oracle;
  
  mt19937 rng(333);
  uniform_int_distribution<int> key_dist(0, 50000);
  
  // Insert many elements, triggering multiple resizes
  const int num_inserts = 3000;
  for (int i = 0; i < num_inserts; ++i)
    {
      int key = key_dist(rng);
      auto ptr = tbl.insert(key);
      if (ptr != nullptr)
        oracle.insert(key);
    }
  
  // Verify size matches
  EXPECT_EQ(tbl.size(), oracle.size());
  
  // Verify all elements survived resizes
  for (int key : oracle)
    {
      auto ptr = tbl.search(key);
      ASSERT_NE(ptr, nullptr) << "Key " << key << " lost during resize";
      EXPECT_EQ(*ptr, key);
    }
  
  // Now remove some elements
  auto it = oracle.begin();
  size_t to_remove = oracle.size() / 3;
  for (size_t i = 0; i < to_remove && it != oracle.end(); ++i)
    {
      int key = *it;
      ++it;
      tbl.remove(key);
      oracle.erase(key);
    }
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  // Verify remaining elements
  for (int key : oracle)
    ASSERT_NE(tbl.search(key), nullptr);
}

// Stress test with string keys
TEST(ODhashTable, Stress_StringKeys)
{
  ODhashTable<string> tbl(100);
  set<string> oracle;
  
  mt19937 rng(54321);
  uniform_int_distribution<int> len_dist(1, 50);
  uniform_int_distribution<int> char_dist('a', 'z');
  
  auto random_string = [&]() {
    int len = len_dist(rng);
    string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i)
      s += static_cast<char>(char_dist(rng));
    return s;
  };
  
  const int num_ops = 2000;
  
  for (int i = 0; i < num_ops; ++i)
    {
      string key = random_string();
      
      if (oracle.count(key) == 0)
        {
          auto ptr = tbl.insert(key);
          if (ptr != nullptr)
            oracle.insert(key);
        }
      else
        {
          // Remove existing key
          tbl.remove(key);
          oracle.erase(key);
        }
    }
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  // Verify all oracle keys are present
  for (const auto & key : oracle)
    {
      auto ptr = tbl.search(key);
      ASSERT_NE(ptr, nullptr) << "String key missing: " << key;
    }
}

// Test search_or_insert with DELETED entries (exercises hard_allocate_bucket)
TEST(ODhashTable, SearchOrInsert_WithDeletedEntries)
{
  ODhashTable<int> tbl(100);
  
  // Insert some elements
  for (int i = 0; i < 30; ++i)
    EXPECT_NE(tbl.insert(i), nullptr);
  
  // Remove some to create DELETED entries
  for (int i = 0; i < 30; i += 2)
    tbl.remove(i);  // Remove even numbers
  
  const size_t size_after_removes = tbl.size();
  
  // search_or_insert for existing keys should return existing
  for (int i = 1; i < 30; i += 2)
    {
      auto ptr = tbl.search_or_insert(i);
      ASSERT_NE(ptr, nullptr);
      EXPECT_EQ(*ptr, i);
    }
  EXPECT_EQ(tbl.size(), size_after_removes);  // Size unchanged
  
  // search_or_insert for removed keys should insert them
  for (int i = 0; i < 30; i += 2)
    {
      const size_t old_size = tbl.size();
      auto ptr = tbl.search_or_insert(i);
      ASSERT_NE(ptr, nullptr);
      EXPECT_EQ(*ptr, i);
      EXPECT_EQ(tbl.size(), old_size + 1);  // Size increased
    }
  
  // Verify all elements are searchable
  for (int i = 0; i < 30; ++i)
    {
      auto ptr = tbl.search(i);
      ASSERT_NE(ptr, nullptr) << "Key " << i << " not found";
    }
}

// Test contains_or_insert (uses hard_allocate_bucket)
TEST(ODhashTable, ContainsOrInsert_WithCollisions)
{
  // Bad hash to force collisions
  auto bad_hash = [](const int &) -> size_t { return 7; };
  
  ODhashTable<int> tbl(50, bad_hash, bad_hash);
  
  // Insert and remove to create DELETED entries with collisions
  for (int i = 0; i < 20; ++i)
    EXPECT_NE(tbl.insert(i), nullptr);
  
  for (int i = 0; i < 20; i += 3)
    tbl.remove(i);
  
  // contains_or_insert for new keys
  for (int i = 20; i < 30; ++i)
    {
      auto [ptr, existed] = tbl.contains_or_insert(i);
      ASSERT_NE(ptr, nullptr);
      EXPECT_FALSE(existed) << "Key " << i << " should not have existed";
      EXPECT_EQ(*ptr, i);
    }
  
  // contains_or_insert for existing keys
  for (int i = 20; i < 30; ++i)
    {
      auto [ptr, existed] = tbl.contains_or_insert(i);
      ASSERT_NE(ptr, nullptr);
      EXPECT_TRUE(existed) << "Key " << i << " should have existed";
      EXPECT_EQ(*ptr, i);
    }
}

// Stress test for search_or_insert with many DELETED entries
TEST(ODhashTable, Stress_SearchOrInsertWithDeleted)
{
  ODhashTable<int> tbl(200);
  std::set<int> oracle;
  std::mt19937 gen(54321);
  std::uniform_int_distribution<> key_dist(0, 500);
  std::uniform_int_distribution<> op_dist(0, 2);
  
  for (int iter = 0; iter < 5000; ++iter)
    {
      int key = key_dist(gen);
      int op = op_dist(gen);
      
      if (op == 0)  // search_or_insert
        {
          auto ptr = tbl.search_or_insert(key);
          ASSERT_NE(ptr, nullptr) << "search_or_insert returned nullptr for key " << key;
          EXPECT_EQ(*ptr, key);
          oracle.insert(key);
          // Verify immediately after insert
          auto verify_ptr = tbl.search(key);
          ASSERT_NE(verify_ptr, nullptr) 
            << "Key " << key << " not found immediately after search_or_insert at iter " << iter;
        }
      else if (op == 1 && oracle.count(key))  // remove existing
        {
          // Verify key exists before removal
          auto pre_ptr = tbl.search(key);
          ASSERT_NE(pre_ptr, nullptr) 
            << "Key " << key << " should exist before removal at iter " << iter
            << ", oracle.count=" << oracle.count(key) << ", tbl.size=" << tbl.size();
          tbl.remove(key);
          oracle.erase(key);
        }
      else  // search
        {
          auto ptr = tbl.search(key);
          if (oracle.count(key))
            ASSERT_NE(ptr, nullptr) << "Key " << key << " should exist at iter " << iter;
          else
            EXPECT_EQ(ptr, nullptr);
        }
      
      EXPECT_EQ(tbl.size(), oracle.size()) 
        << "Size mismatch at iter " << iter << ": tbl=" << tbl.size() << ", oracle=" << oracle.size();
    }
  
  // Final verification
  for (int key : oracle)
    {
      auto ptr = tbl.search(key);
      ASSERT_NE(ptr, nullptr) << "Key " << key << " missing";
    }
}

// Focused debug test to find the exact bug
TEST(ODhashTable, Debug_SearchOrInsertBug)
{
  ODhashTable<int> tbl(200);
  std::set<int> oracle;
  std::mt19937 gen(54321);
  std::uniform_int_distribution<> key_dist(0, 500);
  std::uniform_int_distribution<> op_dist(0, 2);
  
  // Reproduce up to iteration 701 where the bug occurs
  for (int iter = 0; iter <= 705; ++iter)
    {
      int key = key_dist(gen);
      int op = op_dist(gen);
      
      // Verify oracle consistency before operation
      for (int k : oracle)
        {
          auto p = tbl.search(k);
          ASSERT_NE(p, nullptr) 
            << "Pre-op check: Key " << k << " missing at iter " << iter 
            << " (about to do op " << op << " on key " << key << ")";
        }
      
      if (op == 0)  // search_or_insert
        {
          bool existed_before = oracle.count(key) > 0;
          size_t size_before = tbl.size();
          auto ptr = tbl.search_or_insert(key);
          ASSERT_NE(ptr, nullptr) << "search_or_insert returned nullptr at iter " << iter;
          EXPECT_EQ(*ptr, key);
          oracle.insert(key);
          
          if (!existed_before)
            EXPECT_EQ(tbl.size(), size_before + 1) 
              << "Size should increase for new key at iter " << iter;
        }
      else if (op == 1 && oracle.count(key))  // remove existing
        {
          tbl.remove(key);
          oracle.erase(key);
        }
      
      EXPECT_EQ(tbl.size(), oracle.size()) 
        << "Size mismatch at iter " << iter;
    }
}

// ============================================================================
// COPY/MOVE SEMANTICS TESTS
// ============================================================================

TEST(ODhashTable, CopyConstructor)
{
  ODhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  ODhashTable<int> copy(original);
  
  EXPECT_EQ(copy.size(), original.size());
  EXPECT_EQ(copy.capacity(), original.capacity());
  
  for (int i = 0; i < 50; ++i)
    {
      EXPECT_NE(original.search(i), nullptr);
      EXPECT_NE(copy.search(i), nullptr);
    }
  
  copy.remove(25);
  EXPECT_EQ(copy.search(25), nullptr);
  EXPECT_NE(original.search(25), nullptr);
}

TEST(ODhashTable, MoveConstructor)
{
  ODhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  const size_t orig_size = original.size();
  const size_t orig_cap = original.capacity();
  
  ODhashTable<int> moved(std::move(original));
  
  EXPECT_EQ(moved.size(), orig_size);
  EXPECT_EQ(moved.capacity(), orig_cap);
  
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(moved.search(i), nullptr);
}

TEST(ODhashTable, CopyAssignment)
{
  ODhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  ODhashTable<int> copy(10);
  copy.insert(999);
  
  copy = original;
  
  EXPECT_EQ(copy.size(), original.size());
  
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(copy.search(i), nullptr);
  
  EXPECT_EQ(copy.search(999), nullptr);
}

TEST(ODhashTable, MoveAssignment)
{
  ODhashTable<int> original(100);
  for (int i = 0; i < 50; ++i)
    original.insert(i);
  
  const size_t orig_size = original.size();
  
  ODhashTable<int> target(10);
  target.insert(999);
  
  target = std::move(original);
  
  EXPECT_EQ(target.size(), orig_size);
  
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(target.search(i), nullptr);
}

TEST(ODhashTable, SelfAssignment)
{
  ODhashTable<int> tbl(100);
  for (int i = 0; i < 50; ++i)
    tbl.insert(i);
  
  tbl = tbl;
  
  EXPECT_EQ(tbl.size(), 50);
  for (int i = 0; i < 50; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST(ODhashTable, EmptyTableOperations)
{
  ODhashTable<int> tbl(100);
  
  EXPECT_TRUE(tbl.is_empty());
  EXPECT_EQ(tbl.size(), 0);
  EXPECT_EQ(tbl.search(42), nullptr);
  EXPECT_FALSE(tbl.has(42));
  EXPECT_FALSE(tbl.contains(42));
  EXPECT_THROW(tbl.remove(42), std::domain_error);
}

TEST(ODhashTable, SingleElement)
{
  ODhashTable<int> tbl(100);
  
  tbl.insert(42);
  EXPECT_EQ(tbl.size(), 1);
  EXPECT_NE(tbl.search(42), nullptr);
  
  tbl.remove(42);
  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());
  EXPECT_EQ(tbl.search(42), nullptr);
}

TEST(ODhashTable, DuplicateInsertReturnsNull)
{
  ODhashTable<int> tbl(100);
  
  auto first = tbl.insert(42);
  ASSERT_NE(first, nullptr);
  
  auto second = tbl.insert(42);
  EXPECT_EQ(second, nullptr);
  
  EXPECT_EQ(tbl.size(), 1);
}

TEST(ODhashTable, HasAndContains)
{
  ODhashTable<int> tbl(100);
  
  EXPECT_FALSE(tbl.has(42));
  EXPECT_FALSE(tbl.contains(42));
  
  tbl.insert(42);
  
  EXPECT_TRUE(tbl.has(42));
  EXPECT_TRUE(tbl.contains(42));
  EXPECT_FALSE(tbl.has(43));
  EXPECT_FALSE(tbl.contains(43));
}

TEST(ODhashTable, Find)
{
  ODhashTable<int> tbl(100);
  tbl.insert(42);
  
  EXPECT_NO_THROW({
    int& ref = tbl.find(42);
    EXPECT_EQ(ref, 42);
  });
  
  EXPECT_THROW(tbl.find(999), std::domain_error);
}

// ============================================================================
// REHASH/RESIZE TESTS
// ============================================================================

TEST(ODhashTable, ManualRehash)
{
  ODhashTable<int> tbl(100);
  set<int> oracle;
  
  for (int i = 0; i < 50; ++i)
    {
      tbl.insert(i);
      oracle.insert(i);
    }
  
  for (int i = 0; i < 50; i += 2)
    {
      tbl.remove(i);
      oracle.erase(i);
    }
  
  tbl.rehash();
  
  EXPECT_EQ(tbl.size(), oracle.size());
  
  for (int key : oracle)
    EXPECT_NE(tbl.search(key), nullptr);
}

TEST(ODhashTable, ResizeUp)
{
  ODhashTable<int> tbl(50);
  
  for (int i = 0; i < 30; ++i)
    tbl.insert(i);
  
  const size_t old_cap = tbl.capacity();
  tbl.resize(200);
  
  EXPECT_GT(tbl.capacity(), old_cap);
  EXPECT_EQ(tbl.size(), 30);
  
  for (int i = 0; i < 30; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

TEST(ODhashTable, ResizeDown)
{
  ODhashTable<int> tbl(200);
  
  for (int i = 0; i < 30; ++i)
    tbl.insert(i);
  
  tbl.resize(50);
  
  EXPECT_EQ(tbl.size(), 30);
  
  for (int i = 0; i < 30; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

// ============================================================================
// ITERATOR TESTS
// ============================================================================

TEST(ODhashTable, IteratorBasic)
{
  ODhashTable<int> tbl(100);
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

TEST(ODhashTable, IteratorEmpty)
{
  ODhashTable<int> tbl(100);
  
  auto it = tbl.get_it();
  EXPECT_FALSE(it.has_curr());
}

TEST(ODhashTable, IteratorSingleElement)
{
  ODhashTable<int> tbl(100);
  tbl.insert(42);
  
  auto it = tbl.get_it();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 42);
  
  it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST(ODhashTable, IteratorDelete)
{
  ODhashTable<int> tbl(100);
  
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  auto it = tbl.get_it();
  while (it.has_curr())
    it.del();
  
  EXPECT_TRUE(tbl.is_empty());
}

// ============================================================================
// PROBE_COUNTER CLEANUP TESTS (ODhashTable specific)
// ============================================================================

// Helper to count bucket states for ODhashTable
struct ODhashBucketStats
{
  size_t empty = 0;
  size_t busy = 0;
  size_t deleted = 0;
};

template <typename HashTable>
ODhashBucketStats count_odhash_bucket_states(const HashTable &tbl)
{
  ODhashBucketStats stats;
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

// ODhashTable uses probe_counter to convert DELETED->EMPTY when counter reaches 0
TEST(ODhashTable, ProbeCounterCleanup_LastInChainBecomesEmpty)
{
  auto bad_hash = [](const int &) -> size_t { return 0; };
  ODhashTable<int> tbl(100, bad_hash, bad_hash);
  
  // Insert chain
  for (int i = 0; i < 5; ++i)
    tbl.insert(i);
  
  auto before = count_odhash_bucket_states(tbl);
  EXPECT_EQ(before.busy, 5);
  EXPECT_EQ(before.deleted, 0);
  
  // Remove last element - should become EMPTY due to probe_counter
  tbl.remove(4);
  
  auto after = count_odhash_bucket_states(tbl);
  EXPECT_EQ(after.busy, 4);
  // With probe_counter, DELETED becomes EMPTY when no one depends on it
  EXPECT_EQ(after.deleted, 0) << "Last element should become EMPTY via probe_counter";
  
  for (int i = 0; i < 4; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
  EXPECT_EQ(tbl.search(4), nullptr);
}

TEST(ODhashTable, ProbeCounterCleanup_MiddleStaysDeleted)
{
  auto bad_hash = [](const int &) -> size_t { return 0; };
  ODhashTable<int> tbl(100, bad_hash, bad_hash);
  
  for (int i = 0; i < 5; ++i)
    tbl.insert(i);
  
  // Remove middle - should stay DELETED because others depend on it
  tbl.remove(2);
  
  auto stats = count_odhash_bucket_states(tbl);
  EXPECT_EQ(stats.busy, 4);
  EXPECT_EQ(stats.deleted, 1) << "Middle should stay DELETED";
  
  for (int i = 0; i < 5; ++i)
    {
      if (i == 2)
        EXPECT_EQ(tbl.search(i), nullptr);
      else
        EXPECT_NE(tbl.search(i), nullptr);
    }
}

TEST(ODhashTable, ProbeCounterCleanup_ChainCleanup)
{
  auto bad_hash = [](const int &) -> size_t { return 0; };
  ODhashTable<int> tbl(100, bad_hash, bad_hash);
  
  for (int i = 0; i < 5; ++i)
    tbl.insert(i);
  
  // Remove in reverse order - each should become EMPTY
  for (int i = 4; i >= 0; --i)
    tbl.remove(i);
  
  auto stats = count_odhash_bucket_states(tbl);
  EXPECT_EQ(stats.busy, 0);
  EXPECT_EQ(stats.deleted, 0) << "All should become EMPTY when removed in reverse";
  EXPECT_TRUE(tbl.is_empty());
}

// ============================================================================
// FUNCTIONAL METHODS TEST  
// ============================================================================

TEST(ODhashTable, ForEach)
{
  ODhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  int sum = 0;
  tbl.for_each([&sum](int x) { sum += x; });
  
  EXPECT_EQ(sum, 45);
}

TEST(ODhashTable, All)
{
  ODhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i * 2);
  
  EXPECT_TRUE(tbl.all([](int x) { return x % 2 == 0; }));
  EXPECT_FALSE(tbl.all([](int x) { return x > 5; }));
}

TEST(ODhashTable, Exists)
{
  ODhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  EXPECT_TRUE(tbl.exists([](int x) { return x == 5; }));
  EXPECT_FALSE(tbl.exists([](int x) { return x == 100; }));
}

TEST(ODhashTable, Filter)
{
  ODhashTable<int> tbl(100);
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  
  auto evens = tbl.filter([](int x) { return x % 2 == 0; });
  
  EXPECT_EQ(evens.size(), 5);
}

