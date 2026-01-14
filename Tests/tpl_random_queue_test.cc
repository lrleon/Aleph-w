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
 * @file tpl_random_queue_test.cc
 * @brief Comprehensive tests for Random_Set and shuffle functions
 */

#include <gtest/gtest.h>
#include <tpl_random_queue.H>
#include <tpl_dynDlist.H>
#include <set>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace Aleph;

// =============================================================================
// Basic Functionality Tests
// =============================================================================

class RandomSetTest : public ::testing::Test
{
protected:
  Random_Set<int> rset;
};

TEST_F(RandomSetTest, InitiallyEmpty)
{
  EXPECT_TRUE(rset.is_empty());
  EXPECT_EQ(rset.size(), 0);
}

TEST_F(RandomSetTest, PutAndSize)
{
  rset.put(1);
  EXPECT_EQ(rset.size(), 1);
  EXPECT_FALSE(rset.is_empty());
  
  rset.put(2);
  rset.put(3);
  EXPECT_EQ(rset.size(), 3);
}

TEST_F(RandomSetTest, GetReducesSize)
{
  rset.put(1);
  rset.put(2);
  rset.put(3);
  
  rset.get();
  EXPECT_EQ(rset.size(), 2);
  
  rset.get();
  rset.get();
  EXPECT_EQ(rset.size(), 0);
  EXPECT_TRUE(rset.is_empty());
}

TEST_F(RandomSetTest, GetReturnsInsertedElements)
{
  std::set<int> inserted = {1, 2, 3, 4, 5};
  for (int x : inserted)
    rset.put(x);
  
  std::set<int> retrieved;
  while (!rset.is_empty())
    retrieved.insert(rset.get());
  
  EXPECT_EQ(inserted, retrieved);
}

TEST_F(RandomSetTest, GetFromEmptyThrows)
{
  EXPECT_THROW(rset.get(), std::underflow_error);
}

TEST_F(RandomSetTest, PickDoesNotRemove)
{
  rset.put(1);
  rset.put(2);
  rset.put(3);
  
  rset.pick();  // Should not remove
  EXPECT_EQ(rset.size(), 3);
  
  rset.pick();
  EXPECT_EQ(rset.size(), 3);
}

TEST_F(RandomSetTest, PickFromEmptyThrows)
{
  EXPECT_THROW(rset.pick(), std::underflow_error);
}

// =============================================================================
// Append Tests (Random Insertion)
// =============================================================================

TEST_F(RandomSetTest, AppendInsertsElement)
{
  rset.append(1);
  EXPECT_EQ(rset.size(), 1);
  
  rset.append(2);
  rset.append(3);
  EXPECT_EQ(rset.size(), 3);
}

TEST_F(RandomSetTest, AppendReturnsReference)
{
  int& ref = rset.append(42);
  // The reference should be valid within the container
  EXPECT_GE(ref, 0);  // Just verify it's accessible
}

// =============================================================================
// Copy and Move Semantics
// =============================================================================

TEST_F(RandomSetTest, CopyConstructor)
{
  rset.put(1);
  rset.put(2);
  rset.put(3);
  
  Random_Set<int> copy(rset);
  
  EXPECT_EQ(copy.size(), 3);
  
  // Both should have same elements
  std::set<int> orig_elems, copy_elems;
  while (!rset.is_empty())
    orig_elems.insert(rset.get());
  while (!copy.is_empty())
    copy_elems.insert(copy.get());
  
  EXPECT_EQ(orig_elems, copy_elems);
}

TEST_F(RandomSetTest, MoveConstructor)
{
  rset.put(1);
  rset.put(2);
  rset.put(3);
  
  Random_Set<int> moved(std::move(rset));
  
  EXPECT_EQ(moved.size(), 3);
}

TEST_F(RandomSetTest, CopyAssignment)
{
  rset.put(1);
  rset.put(2);
  
  Random_Set<int> other;
  other.put(10);
  
  other = rset;
  
  EXPECT_EQ(other.size(), 2);
}

TEST_F(RandomSetTest, MoveAssignment)
{
  rset.put(1);
  rset.put(2);
  
  Random_Set<int> other;
  other = std::move(rset);
  
  EXPECT_EQ(other.size(), 2);
}

// =============================================================================
// Initializer List and Iterator Constructors
// =============================================================================

TEST(RandomSetConstructors, InitializerList)
{
  Random_Set<int> rset = {1, 2, 3, 4, 5};
  
  EXPECT_EQ(rset.size(), 5);
  
  std::set<int> retrieved;
  while (!rset.is_empty())
    retrieved.insert(rset.get());
  
  std::set<int> expected = {1, 2, 3, 4, 5};
  EXPECT_EQ(retrieved, expected);
}

TEST(RandomSetConstructors, FromDynList)
{
  DynList<int> list;
  list.append(1);
  list.append(2);
  list.append(3);
  
  Random_Set<int> rset(list);
  
  EXPECT_EQ(rset.size(), 3);
}

TEST(RandomSetConstructors, FromIteratorRange)
{
  std::vector<int> vec = {1, 2, 3, 4, 5};
  
  Random_Set<int> rset(vec.begin(), vec.end());
  
  EXPECT_EQ(rset.size(), 5);
}

// =============================================================================
// Randomness Tests
// =============================================================================

TEST(RandomSetRandomness, GetIsRandomWithDifferentSeeds)
{
  // Verify that different seeds produce different extraction order
  const int N = 100;
  
  // First extraction with seed 1
  Random_Set<int> rset1;
  for (int i = 0; i < N; ++i)
    rset1.put(i);
  rset1.set_seed(12345);
  std::vector<int> seq1;
  while (!rset1.is_empty())
    seq1.push_back(rset1.get());
  
  // Second extraction with seed 2
  Random_Set<int> rset2;
  for (int i = 0; i < N; ++i)
    rset2.put(i);
  rset2.set_seed(67890);
  std::vector<int> seq2;
  while (!rset2.is_empty())
    seq2.push_back(rset2.get());
  
  // Different seeds should produce different orders
  EXPECT_NE(seq1, seq2) << "Different seeds should produce different extraction orders";
}

TEST(RandomSetRandomness, SetSeedProducesReproducibleResults)
{
  Random_Set<int> rset1, rset2;
  
  for (int i = 0; i < 10; ++i)
    {
      rset1.put(i);
      rset2.put(i);
    }
  
  rset1.set_seed(12345);
  rset2.set_seed(12345);
  
  std::vector<int> seq1, seq2;
  while (!rset1.is_empty())
    seq1.push_back(rset1.get());
  while (!rset2.is_empty())
    seq2.push_back(rset2.get());
  
  EXPECT_EQ(seq1, seq2);
}

// =============================================================================
// Traverse Tests
// =============================================================================

TEST_F(RandomSetTest, TraverseAllElements)
{
  rset.put(1);
  rset.put(2);
  rset.put(3);
  
  std::set<int> visited;
  rset.traverse([&visited](int x) {
    visited.insert(x);
    return true;
  });
  
  std::set<int> expected = {1, 2, 3};
  EXPECT_EQ(visited, expected);
}

TEST_F(RandomSetTest, TraverseEarlyTermination)
{
  for (int i = 1; i <= 10; ++i)
    rset.put(i);
  
  int count = 0;
  rset.traverse([&count](int) {
    ++count;
    return count < 5;  // Stop after 5 elements
  });
  
  EXPECT_EQ(count, 5);
}

// =============================================================================
// Shuffle Function Tests
// =============================================================================

TEST(ShuffleFunction, ShufflesDynList)
{
  DynList<int> list;
  for (int i = 0; i < 10; ++i)
    list.append(i);
  
  auto shuffled = shuffle(list);
  
  // Same size
  EXPECT_EQ(shuffled.size(), list.size());
  
  // Same elements (just reordered)
  std::set<int> orig_set, shuffled_set;
  list.for_each([&](int x) { orig_set.insert(x); });
  shuffled.for_each([&](int x) { shuffled_set.insert(x); });
  
  EXPECT_EQ(orig_set, shuffled_set);
}

TEST(ShuffleFunction, ShuffleProducesPermutation)
{
  DynList<int> list;
  for (int i = 0; i < 100; ++i)
    list.append(i);
  
  auto shuffled = shuffle(list);
  
  // Check it's actually shuffled (not in original order)
  std::vector<int> orig_vec, shuffled_vec;
  list.for_each([&](int x) { orig_vec.push_back(x); });
  shuffled.for_each([&](int x) { shuffled_vec.push_back(x); });
  
  // With 100 elements, probability of same order is 1/100! ≈ 0
  EXPECT_NE(orig_vec, shuffled_vec);
}

// =============================================================================
// Move Semantics for Elements
// =============================================================================

TEST(RandomSetMoveSemantics, PutWithMove)
{
  Random_Set<std::string> rset;
  
  std::string s = "hello";
  rset.put(std::move(s));
  
  EXPECT_EQ(rset.size(), 1);
  // s may be empty after move
}

TEST(RandomSetMoveSemantics, AppendWithMove)
{
  Random_Set<std::string> rset;
  
  std::string s = "world";
  rset.append(std::move(s));
  
  EXPECT_EQ(rset.size(), 1);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(RandomSetStress, ManyElements)
{
  Random_Set<int> rset;
  
  const int N = 10000;
  for (int i = 0; i < N; ++i)
    rset.put(i);
  
  EXPECT_EQ(rset.size(), N);
  
  std::set<int> retrieved;
  while (!rset.is_empty())
    retrieved.insert(rset.get());
  
  EXPECT_EQ(retrieved.size(), N);
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(retrieved.count(i));
}

TEST(RandomSetStress, ManyPutGetCycles)
{
  Random_Set<int> rset;
  
  for (int cycle = 0; cycle < 100; ++cycle)
    {
      // Add some elements
      for (int i = 0; i < 100; ++i)
        rset.put(cycle * 100 + i);
      
      // Remove some
      for (int i = 0; i < 50; ++i)
        rset.get();
    }
  
  // Should have 50 * 100 = 5000 elements remaining
  EXPECT_EQ(rset.size(), 5000);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(RandomSetTest, SingleElement)
{
  rset.put(42);
  
  EXPECT_EQ(rset.get(), 42);
  EXPECT_TRUE(rset.is_empty());
}

TEST_F(RandomSetTest, DuplicateElements)
{
  rset.put(1);
  rset.put(1);
  rset.put(1);
  
  EXPECT_EQ(rset.size(), 3);
  
  int sum = 0;
  while (!rset.is_empty())
    sum += rset.get();
  
  EXPECT_EQ(sum, 3);  // Three 1's
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
