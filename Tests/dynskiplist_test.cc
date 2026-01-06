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

#include <gtest/gtest.h>
#include <gsl/gsl_rng.h>
#include <string>
#include <vector>
#include <algorithm>
#include <tpl_dynSkipList.H>

using namespace Aleph;

namespace {

class DynSkipListTest : public ::testing::Test
{
protected:
  DynSkipList<int> sl;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(DynSkipListTest, DefaultConstruction)
{
  EXPECT_TRUE(sl.is_empty());
  EXPECT_EQ(sl.size(), 0);
}

TEST_F(DynSkipListTest, InitializerListConstruction)
{
  DynSkipList<int> sl2({5, 2, 8, 1, 9});
  
  EXPECT_EQ(sl2.size(), 5);
  EXPECT_TRUE(sl2.has(1));
  EXPECT_TRUE(sl2.has(2));
  EXPECT_TRUE(sl2.has(5));
  EXPECT_TRUE(sl2.has(8));
  EXPECT_TRUE(sl2.has(9));
}

TEST_F(DynSkipListTest, ConstructorWithSeed)
{
  DynSkipList<int> sl2(42, 0.5);
  sl2.insert(1);
  sl2.insert(2);
  EXPECT_EQ(sl2.size(), 2);
}

TEST_F(DynSkipListTest, SetSeed)
{
  sl.set_seed(123);
  EXPECT_NE(sl.gsl_rng_object(), nullptr);
}

TEST_F(DynSkipListTest, GslRngObject)
{
  gsl_rng* rng = sl.gsl_rng_object();
  EXPECT_NE(rng, nullptr);
}

TEST_F(DynSkipListTest, CopyConstruction)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  DynSkipList<int> sl2(sl);
  
  EXPECT_EQ(sl2.size(), 3);
  EXPECT_TRUE(sl2.has(1));
  EXPECT_TRUE(sl2.has(2));
  EXPECT_TRUE(sl2.has(3));
  
  // Modify original, copy should be independent
  sl.insert(4);
  EXPECT_FALSE(sl2.has(4));
}

TEST_F(DynSkipListTest, MoveConstruction)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  DynSkipList<int> sl2(std::move(sl));
  
  EXPECT_EQ(sl2.size(), 3);
  EXPECT_TRUE(sl2.has(1));
  EXPECT_TRUE(sl2.has(2));
  EXPECT_TRUE(sl2.has(3));
  
  // Original should be empty
  EXPECT_TRUE(sl.is_empty());
}

// ============================================================================
// Insert Tests
// ============================================================================

TEST_F(DynSkipListTest, InsertSingle)
{
  int * result = sl.insert(42);
  
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
  EXPECT_EQ(sl.size(), 1);
  EXPECT_TRUE(sl.has(42));
}

TEST_F(DynSkipListTest, InsertDuplicate)
{
  sl.insert(42);
  int * result = sl.insert(42);
  
  EXPECT_EQ(result, nullptr);  // Duplicate not inserted
  EXPECT_EQ(sl.size(), 1);
}

TEST_F(DynSkipListTest, InsertMultiple)
{
  for (int i = 1; i <= 10; ++i)
    sl.insert(i);
  
  EXPECT_EQ(sl.size(), 10);
  
  for (int i = 1; i <= 10; ++i)
    EXPECT_TRUE(sl.has(i));
}

TEST_F(DynSkipListTest, InsertDescending)
{
  for (int i = 10; i >= 1; --i)
    sl.insert(i);
  
  EXPECT_EQ(sl.size(), 10);
  EXPECT_EQ(sl.min(), 1);
  EXPECT_EQ(sl.max(), 10);
}

TEST_F(DynSkipListTest, InsertRvalue)
{
  std::string str = "hello";
  DynSkipList<std::string> sl_str;
  
  sl_str.insert(std::move(str));
  
  EXPECT_EQ(sl_str.size(), 1);
  EXPECT_TRUE(sl_str.has("hello"));
}

// ============================================================================
// Search Tests
// ============================================================================

TEST_F(DynSkipListTest, SearchEmpty)
{
  EXPECT_EQ(sl.search(42), nullptr);
}

TEST_F(DynSkipListTest, SearchExisting)
{
  sl.insert(42);
  
  int * result = sl.search(42);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
}

TEST_F(DynSkipListTest, SearchNonExisting)
{
  sl.insert(42);
  
  EXPECT_EQ(sl.search(41), nullptr);
  EXPECT_EQ(sl.search(43), nullptr);
}

TEST_F(DynSkipListTest, HasContainsExist)
{
  sl.insert(42);
  
  EXPECT_TRUE(sl.has(42));
  EXPECT_TRUE(sl.contains(42));
  EXPECT_TRUE(sl.exist(42));
  
  EXPECT_FALSE(sl.has(43));
  EXPECT_FALSE(sl.contains(43));
  EXPECT_FALSE(sl.exist(43));
}

TEST_F(DynSkipListTest, Find)
{
  sl.insert(42);
  
  EXPECT_EQ(sl.find(42), 42);
  
  EXPECT_THROW((void)sl.find(43), std::domain_error);
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST_F(DynSkipListTest, RemoveExisting)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  size_t removed = sl.remove(2);
  
  EXPECT_EQ(removed, 1);  // 1 element was removed
  EXPECT_EQ(sl.size(), 2);
  EXPECT_FALSE(sl.has(2));
  EXPECT_TRUE(sl.has(1));
  EXPECT_TRUE(sl.has(3));
}

TEST_F(DynSkipListTest, RemoveNonExisting)
{
  sl.insert(1);
  sl.insert(2);
  
  size_t removed = sl.remove(99);
  
  EXPECT_EQ(removed, 0);  // Nothing was removed
  EXPECT_EQ(sl.size(), 2);
}

TEST_F(DynSkipListTest, RemoveFirst)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  sl.remove(1);
  
  EXPECT_EQ(sl.min(), 2);
}

TEST_F(DynSkipListTest, RemoveLast)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  sl.remove(3);
  
  EXPECT_EQ(sl.max(), 2);
}

TEST_F(DynSkipListTest, RemoveAll)
{
  for (int i = 1; i <= 5; ++i)
    sl.insert(i);
  
  for (int i = 1; i <= 5; ++i)
    sl.remove(i);
  
  EXPECT_TRUE(sl.is_empty());
}

TEST_F(DynSkipListTest, Del)
{
  sl.insert(42);
  
  int deleted = sl.del(42);
  
  EXPECT_EQ(deleted, 42);
  EXPECT_FALSE(sl.has(42));
  
  EXPECT_THROW(sl.del(42), std::domain_error);
}

// ============================================================================
// Min/Max Tests
// ============================================================================

TEST_F(DynSkipListTest, MinMax)
{
  sl.insert(5);
  sl.insert(2);
  sl.insert(8);
  sl.insert(1);
  sl.insert(9);
  
  EXPECT_EQ(sl.min(), 1);
  EXPECT_EQ(sl.get_first(), 1);
  EXPECT_EQ(sl.max(), 9);
  EXPECT_EQ(sl.get_last(), 9);
}

TEST_F(DynSkipListTest, MinMaxEmpty)
{
  EXPECT_THROW((void)sl.min(), std::domain_error);
  EXPECT_THROW((void)sl.max(), std::domain_error);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(DynSkipListTest, IteratorEmpty)
{
  DynSkipList<int>::Iterator it(sl);
  EXPECT_FALSE(it.has_curr());
}

TEST_F(DynSkipListTest, IteratorTraverse)
{
  for (int i = 1; i <= 5; ++i)
    sl.insert(i);
  
  int expected = 1;
  for (DynSkipList<int>::Iterator it(sl); it.has_curr(); it.next())
    {
      EXPECT_EQ(it.get_curr(), expected);
      ++expected;
    }
  EXPECT_EQ(expected, 6);
}

TEST_F(DynSkipListTest, IteratorRangeBased)
{
  sl.insert(3);
  sl.insert(1);
  sl.insert(2);
  
  std::vector<int> result;
  for (const auto & key : sl)
    result.push_back(key);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST_F(DynSkipListTest, IteratorOperators)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  auto it = sl.begin();
  EXPECT_EQ(*it, 1);
  
  ++it;
  EXPECT_EQ(*it, 2);
  
  it++;
  EXPECT_EQ(*it, 3);
  
  ++it;
  EXPECT_EQ(it, sl.end());
}

// ============================================================================
// Assignment Tests
// ============================================================================

TEST_F(DynSkipListTest, CopyAssignment)
{
  sl.insert(1);
  sl.insert(2);
  
  DynSkipList<int> sl2;
  sl2.insert(99);
  
  sl2 = sl;
  
  EXPECT_EQ(sl2.size(), 2);
  EXPECT_TRUE(sl2.has(1));
  EXPECT_TRUE(sl2.has(2));
  EXPECT_FALSE(sl2.has(99));
}

TEST_F(DynSkipListTest, MoveAssignment)
{
  sl.insert(1);
  sl.insert(2);
  
  DynSkipList<int> sl2;
  sl2.insert(99);
  
  sl2 = std::move(sl);
  
  EXPECT_EQ(sl2.size(), 2);
  EXPECT_TRUE(sl2.has(1));
  EXPECT_TRUE(sl.is_empty());
}

TEST_F(DynSkipListTest, SelfAssignment)
{
  sl.insert(1);
  sl.insert(2);
  
  sl = sl;
  
  EXPECT_EQ(sl.size(), 2);
}

// ============================================================================
// Empty/Swap Tests
// ============================================================================

TEST_F(DynSkipListTest, Empty)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  sl.empty();
  
  EXPECT_TRUE(sl.is_empty());
  EXPECT_EQ(sl.size(), 0);
}

TEST_F(DynSkipListTest, Swap)
{
  sl.insert(1);
  sl.insert(2);
  
  DynSkipList<int> sl2;
  sl2.insert(10);
  sl2.insert(20);
  sl2.insert(30);
  
  sl.swap(sl2);
  
  EXPECT_EQ(sl.size(), 3);
  EXPECT_TRUE(sl.has(10));
  
  EXPECT_EQ(sl2.size(), 2);
  EXPECT_TRUE(sl2.has(1));
}

// ============================================================================
// Functional Tests
// ============================================================================

TEST_F(DynSkipListTest, ForEach)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  int sum = 0;
  sl.for_each([&sum](int x) { sum += x; });
  
  EXPECT_EQ(sum, 6);
}

TEST_F(DynSkipListTest, All)
{
  sl.insert(2);
  sl.insert(4);
  sl.insert(6);
  
  EXPECT_TRUE(sl.all([](int x) { return x % 2 == 0; }));
  
  sl.insert(3);
  EXPECT_FALSE(sl.all([](int x) { return x % 2 == 0; }));
}

TEST_F(DynSkipListTest, Exists)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  
  EXPECT_TRUE(sl.exists([](int x) { return x > 2; }));
  EXPECT_FALSE(sl.exists([](int x) { return x > 10; }));
}

TEST_F(DynSkipListTest, Traverse)
{
  sl.insert(1);
  sl.insert(2);
  sl.insert(3);
  sl.insert(4);
  sl.insert(5);
  
  // Traverse until we find 3
  int count = 0;
  bool completed = sl.traverse([&count](int x) {
    ++count;
    return x != 3;  // Stop when we find 3
  });
  
  EXPECT_FALSE(completed);
  EXPECT_EQ(count, 3);
}

// ============================================================================
// Large Scale Tests
// ============================================================================

TEST_F(DynSkipListTest, LargeScale)
{
  const int N = 1000;
  
  for (int i = 0; i < N; ++i)
    sl.insert(i);
  
  EXPECT_EQ(sl.size(), N);
  
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(sl.has(i));
  
  for (int i = 0; i < N; i += 2)
    sl.remove(i);
  
  EXPECT_EQ(sl.size(), N / 2);
  
  for (int i = 0; i < N; ++i)
    {
      if (i % 2 == 0)
        EXPECT_FALSE(sl.has(i));
      else
        EXPECT_TRUE(sl.has(i));
    }
}

// ============================================================================
// String Key Tests
// ============================================================================

TEST(DynSkipListStringTest, StringKeys)
{
  DynSkipList<std::string> sl;
  
  sl.insert("banana");
  sl.insert("apple");
  sl.insert("cherry");
  
  EXPECT_EQ(sl.size(), 3);
  EXPECT_EQ(sl.min(), "apple");
  EXPECT_EQ(sl.max(), "cherry");
  
  EXPECT_TRUE(sl.has("banana"));
  EXPECT_FALSE(sl.has("date"));
}

// ============================================================================
// SearchOrInsert Tests
// ============================================================================

TEST_F(DynSkipListTest, SearchOrInsertNew)
{
  int * result = sl.search_or_insert(42);
  
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
  EXPECT_EQ(sl.size(), 1);
}

TEST_F(DynSkipListTest, SearchOrInsertExisting)
{
  sl.insert(42);
  
  int * result = sl.search_or_insert(42);
  
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
  EXPECT_EQ(sl.size(), 1);  // No new insertion
}

} // anonymous namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
