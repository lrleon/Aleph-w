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
# include <gtest/gtest.h>

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


