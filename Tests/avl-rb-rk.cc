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
 * @file avl-rb-rk.cc
 * @brief Tests for Avl Rb Rk
 */

/* Test for AVL and Red-Black trees with counters (select/position support) */

# include <gtest/gtest.h>
# include <random>
# include <vector>
# include <algorithm>

# include <tpl_avlRk.H>
# include <tpl_rbRk.H>

using namespace std;
using namespace Aleph;

// Random generator
static mt19937 rng(42);

template <typename Tree>
class RankTreeTest : public ::testing::Test
{
protected:
  using Node = typename Tree::Node;
  Tree tree;
  vector<int> keys;
  static constexpr size_t N = 1000;

  void SetUp() override
  {
    keys.resize(N);
    iota(keys.begin(), keys.end(), 0);
    shuffle(keys.begin(), keys.end(), rng);
  }

  void TearDown() override
  {
    while (not tree.is_empty())
      {
        auto p = tree.remove(tree.getRoot()->get_key());
        delete p;
      }
  }

  void insert_all()
  {
    for (int k : keys)
      {
        auto p = new Node(k);
        ASSERT_NE(tree.insert(p), nullptr);
      }
  }
};

using AvlRkTest = RankTreeTest<Avl_Tree_Rk<int>>;
using RbRkTest = RankTreeTest<Rb_Tree_Rk<int>>;

// AVL Tree Tests

TEST_F(AvlRkTest, InsertAndVerify)
{
  insert_all();

  EXPECT_EQ(tree.size(), N);
  EXPECT_TRUE(tree.verify());
}

TEST_F(AvlRkTest, SelectOperations)
{
  insert_all();

  // After inserting 0..N-1, select(i) should return node with key i
  for (size_t i = 0; i < N; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(i));
    }
}

TEST_F(AvlRkTest, PositionOperations)
{
  insert_all();

  // For each key k, position(k) should return k
  for (size_t i = 0; i < N; ++i)
    {
      auto [pos, node] = tree.position(static_cast<int>(i));
      EXPECT_EQ(pos, static_cast<long>(i));
      ASSERT_NE(node, nullptr);
      EXPECT_EQ(node->get_key(), static_cast<int>(i));
    }
}

TEST_F(AvlRkTest, PositionNotFound)
{
  insert_all();

  // Key not in tree should return -1
  auto [pos, node] = tree.position(static_cast<int>(N + 100));
  EXPECT_EQ(pos, -1);
}

TEST_F(AvlRkTest, RemoveAndVerify)
{
  insert_all();

  shuffle(keys.begin(), keys.end(), rng);

  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.remove(keys[i]);
      ASSERT_NE(p, nullptr);
      delete p;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), N - N / 2);
}

TEST_F(AvlRkTest, SelectAfterRemoval)
{
  insert_all();

  // Remove even keys
  for (int k = 0; k < static_cast<int>(N); k += 2)
    {
      auto p = tree.remove(k);
      delete p;
    }

  EXPECT_EQ(tree.size(), N / 2);

  // Now select should return odd keys in order
  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(2 * i + 1));
    }
}

// Red-Black Tree Tests

TEST_F(RbRkTest, InsertAndVerify)
{
  insert_all();

  EXPECT_EQ(tree.size(), N);
  EXPECT_TRUE(tree.verify());
}

TEST_F(RbRkTest, SelectOperations)
{
  insert_all();

  // After inserting 0..N-1, select(i) should return node with key i
  for (size_t i = 0; i < N; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(i));
    }
}

TEST_F(RbRkTest, PositionOperations)
{
  insert_all();

  // For each key k, position(k) should return k
  for (size_t i = 0; i < N; ++i)
    {
      auto [pos, node] = tree.position(static_cast<int>(i));
      EXPECT_EQ(pos, static_cast<long>(i));
      ASSERT_NE(node, nullptr);
      EXPECT_EQ(node->get_key(), static_cast<int>(i));
    }
}

TEST_F(RbRkTest, PositionNotFound)
{
  insert_all();

  // Key not in tree should return -1
  auto [pos, node] = tree.position(static_cast<int>(N + 100));
  EXPECT_EQ(pos, -1);
}

TEST_F(RbRkTest, RemoveAndVerify)
{
  insert_all();

  shuffle(keys.begin(), keys.end(), rng);

  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.remove(keys[i]);
      ASSERT_NE(p, nullptr);
      delete p;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), N - N / 2);
}

TEST_F(RbRkTest, SelectAfterRemoval)
{
  insert_all();

  // Remove even keys
  for (int k = 0; k < static_cast<int>(N); k += 2)
    {
      auto p = tree.remove(k);
      delete p;
    }

  EXPECT_EQ(tree.size(), N / 2);

  // Now select should return odd keys in order
  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(2 * i + 1));
    }
}

// Combined stress test

TEST(StressTest, MixedOperationsAvl)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;
  const size_t N = 5000;

  // Insert
  for (size_t i = 0; i < N; ++i)
    {
      auto p = new Node(static_cast<int>(i));
      tree.insert(p);
    }

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N);

  // Random selects
  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int i = 0; i < 100; ++i)
    {
      size_t pos = dist(rng);
      auto p = tree.select(pos);
      EXPECT_EQ(p->get_key(), static_cast<int>(pos));
    }

  // Random positions
  for (int i = 0; i < 100; ++i)
    {
      int key = static_cast<int>(dist(rng));
      auto [pos, node] = tree.position(key);
      EXPECT_EQ(pos, key);
    }

  // Remove half
  for (int i = 0; i < static_cast<int>(N); i += 2)
    delete tree.remove(i);

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N / 2);

  // Cleanup
  while (not tree.is_empty())
    delete tree.remove(tree.getRoot()->get_key());
}

TEST(StressTest, MixedOperationsRb)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;
  const size_t N = 5000;

  // Insert
  for (size_t i = 0; i < N; ++i)
    {
      auto p = new Node(static_cast<int>(i));
      tree.insert(p);
    }

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N);

  // Random selects
  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int i = 0; i < 100; ++i)
    {
      size_t pos = dist(rng);
      auto p = tree.select(pos);
      EXPECT_EQ(p->get_key(), static_cast<int>(pos));
    }

  // Random positions
  for (int i = 0; i < 100; ++i)
    {
      int key = static_cast<int>(dist(rng));
      auto [pos, node] = tree.position(key);
      EXPECT_EQ(pos, key);
    }

  // Remove half
  for (int i = 0; i < static_cast<int>(N); i += 2)
    delete tree.remove(i);

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N / 2);

  // Cleanup
  while (not tree.is_empty())
    delete tree.remove(tree.getRoot()->get_key());
}

// Test empty tree edge cases

TEST(EdgeCases, EmptyAvlTree)
{
  Avl_Tree_Rk<int> tree;

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_EQ(tree.remove(42), nullptr);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, -1);
}

TEST(EdgeCases, EmptyRbTree)
{
  Rb_Tree_Rk<int> tree;

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_EQ(tree.remove(42), nullptr);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, -1);
}

// Test single element

TEST(EdgeCases, SingleElementAvl)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;

  auto p = new Node(42);
  tree.insert(p);

  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.verify());

  auto selected = tree.select(0);
  EXPECT_EQ(selected->get_key(), 42);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, 0);
  EXPECT_EQ(node->get_key(), 42);

  delete tree.remove(42);
  EXPECT_TRUE(tree.is_empty());
}

TEST(EdgeCases, SingleElementRb)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;

  auto p = new Node(42);
  tree.insert(p);

  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.verify());

  auto selected = tree.select(0);
  EXPECT_EQ(selected->get_key(), 42);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, 0);
  EXPECT_EQ(node->get_key(), 42);

  delete tree.remove(42);
  EXPECT_TRUE(tree.is_empty());
}

// Test search_or_insert

TEST(SearchOrInsert, AvlTree)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;

  auto p1 = new Node(42);
  auto result1 = tree.search_or_insert(p1);
  EXPECT_EQ(result1, p1);
  EXPECT_EQ(tree.size(), 1);

  auto p2 = new Node(42);
  auto result2 = tree.search_or_insert(p2);
  EXPECT_EQ(result2, p1);  // Should return existing node
  EXPECT_EQ(tree.size(), 1);  // Size should not change

  delete p2;  // Delete the non-inserted node
  delete tree.remove(42);
}

TEST(SearchOrInsert, RbTree)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;

  auto p1 = new Node(42);
  auto result1 = tree.search_or_insert(p1);
  EXPECT_EQ(result1, p1);
  EXPECT_EQ(tree.size(), 1);

  auto p2 = new Node(42);
  auto result2 = tree.search_or_insert(p2);
  EXPECT_EQ(result2, p1);  // Should return existing node
  EXPECT_EQ(tree.size(), 1);  // Size should not change

  delete p2;  // Delete the non-inserted node
  delete tree.remove(42);
}

// Test insert_dup

TEST(InsertDup, AvlTree)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 10; ++i)
    {
      auto p = new Node(42);
      tree.insert_dup(p);
    }

  EXPECT_EQ(tree.size(), 10);
  EXPECT_TRUE(tree.verify());

  // All should have key 42
  for (size_t i = 0; i < 10; ++i)
    EXPECT_EQ(tree.select(i)->get_key(), 42);

  while (not tree.is_empty())
    delete tree.remove(42);
}

TEST(InsertDup, RbTree)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 10; ++i)
    {
      auto p = new Node(42);
      tree.insert_dup(p);
    }

  EXPECT_EQ(tree.size(), 10);
  EXPECT_TRUE(tree.verify());

  // All should have key 42
  for (size_t i = 0; i < 10; ++i)
    EXPECT_EQ(tree.select(i)->get_key(), 42);

  while (not tree.is_empty())
    delete tree.remove(42);
}

// ==============================================================
// Join/Split tests
// ==============================================================

// Helper to cleanup a tree
template <typename Tree>
void destroy_tree(Tree & tree)
{
  while (not tree.is_empty())
    delete tree.remove(tree.getRoot()->get_key());
}

// AVL Join Tests

TEST(JoinSplitAvl, JoinExclusiveBasic)
{
  Avl_Tree_Rk<int> t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  // Insert 0-49 in t1, 50-99 in t2
  for (int i = 0; i < 50; ++i)
    t1.insert(new Node(i));
  for (int i = 50; i < 100; ++i)
    t2.insert(new Node(i));

  EXPECT_EQ(t1.size(), 50);
  EXPECT_EQ(t2.size(), 50);
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  t1.join_exclusive(t2);

  EXPECT_EQ(t1.size(), 100);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  // Check all elements are present and in order
  for (int i = 0; i < 100; ++i)
    {
      auto p = t1.select(i);
      EXPECT_EQ(p->get_key(), i);
    }

  destroy_tree(t1);
}

TEST(JoinSplitAvl, JoinExclusiveEmptyLeft)
{
  Avl_Tree_Rk<int> t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t2.insert(new Node(i));

  t1.join_exclusive(t2);

  EXPECT_EQ(t1.size(), 50);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  destroy_tree(t1);
}

TEST(JoinSplitAvl, JoinExclusiveEmptyRight)
{
  Avl_Tree_Rk<int> t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t1.insert(new Node(i));

  t1.join_exclusive(t2);

  EXPECT_EQ(t1.size(), 50);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  destroy_tree(t1);
}

TEST(JoinSplitAvl, SplitKeyBasic)
{
  Avl_Tree_Rk<int> tree, t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 100; ++i)
    tree.insert(new Node(i));

  auto pivot = tree.split_key(50, t1, t2);

  EXPECT_NE(pivot, nullptr);
  EXPECT_EQ(pivot->get_key(), 50);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  EXPECT_EQ(t1.size(), 50);  // 0-49
  EXPECT_EQ(t2.size(), 49);  // 51-99

  // Check t1 contains 0-49
  for (int i = 0; i < 50; ++i)
    EXPECT_EQ(t1.select(i)->get_key(), i);

  // Check t2 contains 51-99
  for (int i = 0; i < 49; ++i)
    EXPECT_EQ(t2.select(i)->get_key(), i + 51);

  delete pivot;
  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(JoinSplitAvl, SplitKeyNotFound)
{
  Avl_Tree_Rk<int> tree, t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  // Insert even numbers only
  for (int i = 0; i < 100; i += 2)
    tree.insert(new Node(i));

  // Split by odd number (not in tree)
  auto pivot = tree.split_key(51, t1, t2);

  EXPECT_EQ(pivot, nullptr);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  // t1 should have 0, 2, 4, ..., 50 (26 elements)
  EXPECT_EQ(t1.size(), 26);
  // t2 should have 52, 54, ..., 98 (24 elements)
  EXPECT_EQ(t2.size(), 24);

  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(JoinSplitAvl, SplitPosBasic)
{
  Avl_Tree_Rk<int> tree, t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 100; ++i)
    tree.insert(new Node(i));

  tree.split_pos(30, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  EXPECT_EQ(t1.size(), 30);  // positions 0-29
  EXPECT_EQ(t2.size(), 70);  // positions 30-99

  // Check t1 contains 0-29
  for (int i = 0; i < 30; ++i)
    EXPECT_EQ(t1.select(i)->get_key(), i);

  // Check t2 contains 30-99
  for (int i = 0; i < 70; ++i)
    EXPECT_EQ(t2.select(i)->get_key(), i + 30);

  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(JoinSplitAvl, SplitPosZero)
{
  Avl_Tree_Rk<int> tree, t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    tree.insert(new Node(i));

  tree.split_pos(0, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.is_empty());
  EXPECT_EQ(t2.size(), 50);
  EXPECT_TRUE(t2.verify());

  destroy_tree(t2);
}

TEST(JoinSplitAvl, SplitPosEnd)
{
  Avl_Tree_Rk<int> tree, t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    tree.insert(new Node(i));

  tree.split_pos(50, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(t1.size(), 50);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  destroy_tree(t1);
}

TEST(JoinSplitAvl, JoinThenSplit)
{
  Avl_Tree_Rk<int> t1, t2, t3, t4;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t1.insert(new Node(i));
  for (int i = 50; i < 100; ++i)
    t2.insert(new Node(i));

  t1.join_exclusive(t2);
  EXPECT_EQ(t1.size(), 100);
  EXPECT_TRUE(t1.verify());

  auto pivot = t1.split_key(50, t3, t4);
  EXPECT_EQ(pivot->get_key(), 50);
  EXPECT_EQ(t3.size(), 50);
  EXPECT_EQ(t4.size(), 49);
  EXPECT_TRUE(t3.verify());
  EXPECT_TRUE(t4.verify());

  delete pivot;
  destroy_tree(t3);
  destroy_tree(t4);
}

// Red-Black Join Tests

TEST(JoinSplitRb, JoinExclusiveBasic)
{
  Rb_Tree_Rk<int> t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t1.insert(new Node(i));
  for (int i = 50; i < 100; ++i)
    t2.insert(new Node(i));

  EXPECT_EQ(t1.size(), 50);
  EXPECT_EQ(t2.size(), 50);
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  t1.join_exclusive(t2);

  EXPECT_EQ(t1.size(), 100);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  for (int i = 0; i < 100; ++i)
    {
      auto p = t1.select(i);
      EXPECT_EQ(p->get_key(), i);
    }

  destroy_tree(t1);
}

TEST(JoinSplitRb, JoinExclusiveEmptyLeft)
{
  Rb_Tree_Rk<int> t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t2.insert(new Node(i));

  t1.join_exclusive(t2);

  EXPECT_EQ(t1.size(), 50);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  destroy_tree(t1);
}

TEST(JoinSplitRb, JoinExclusiveEmptyRight)
{
  Rb_Tree_Rk<int> t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t1.insert(new Node(i));

  t1.join_exclusive(t2);

  EXPECT_EQ(t1.size(), 50);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  destroy_tree(t1);
}

TEST(JoinSplitRb, SplitKeyBasic)
{
  Rb_Tree_Rk<int> tree, t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 100; ++i)
    tree.insert(new Node(i));

  auto pivot = tree.split_key(50, t1, t2);

  EXPECT_NE(pivot, nullptr);
  EXPECT_EQ(pivot->get_key(), 50);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  EXPECT_EQ(t1.size(), 50);  // 0-49
  EXPECT_EQ(t2.size(), 49);  // 51-99

  for (int i = 0; i < 50; ++i)
    EXPECT_EQ(t1.select(i)->get_key(), i);

  for (int i = 0; i < 49; ++i)
    EXPECT_EQ(t2.select(i)->get_key(), i + 51);

  delete pivot;
  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(JoinSplitRb, SplitKeyNotFound)
{
  Rb_Tree_Rk<int> tree, t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 100; i += 2)
    tree.insert(new Node(i));

  auto pivot = tree.split_key(51, t1, t2);

  EXPECT_EQ(pivot, nullptr);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  EXPECT_EQ(t1.size(), 26);
  EXPECT_EQ(t2.size(), 24);

  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(JoinSplitRb, SplitPosBasic)
{
  Rb_Tree_Rk<int> tree, t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 100; ++i)
    tree.insert(new Node(i));

  tree.split_pos(30, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  EXPECT_EQ(t1.size(), 30);
  EXPECT_EQ(t2.size(), 70);

  for (int i = 0; i < 30; ++i)
    EXPECT_EQ(t1.select(i)->get_key(), i);

  for (int i = 0; i < 70; ++i)
    EXPECT_EQ(t2.select(i)->get_key(), i + 30);

  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(JoinSplitRb, SplitPosZero)
{
  Rb_Tree_Rk<int> tree, t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    tree.insert(new Node(i));

  tree.split_pos(0, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.is_empty());
  EXPECT_EQ(t2.size(), 50);
  EXPECT_TRUE(t2.verify());

  destroy_tree(t2);
}

TEST(JoinSplitRb, SplitPosEnd)
{
  Rb_Tree_Rk<int> tree, t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    tree.insert(new Node(i));

  tree.split_pos(50, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(t1.size(), 50);
  EXPECT_TRUE(t2.is_empty());
  EXPECT_TRUE(t1.verify());

  destroy_tree(t1);
}

TEST(JoinSplitRb, JoinThenSplit)
{
  Rb_Tree_Rk<int> t1, t2, t3, t4;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    t1.insert(new Node(i));
  for (int i = 50; i < 100; ++i)
    t2.insert(new Node(i));

  t1.join_exclusive(t2);
  EXPECT_EQ(t1.size(), 100);
  EXPECT_TRUE(t1.verify());

  auto pivot = t1.split_key(50, t3, t4);
  EXPECT_EQ(pivot->get_key(), 50);
  EXPECT_EQ(t3.size(), 50);
  EXPECT_EQ(t4.size(), 49);
  EXPECT_TRUE(t3.verify());
  EXPECT_TRUE(t4.verify());

  delete pivot;
  destroy_tree(t3);
  destroy_tree(t4);
}

// Large scale join/split stress tests

TEST(JoinSplitStress, AvlLargeJoinSplit)
{
  Avl_Tree_Rk<int> t1, t2, t3, t4;
  using Node = Avl_Tree_Rk<int>::Node;
  const int N = 1000;

  for (int i = 0; i < N / 2; ++i)
    t1.insert(new Node(i));
  for (int i = N / 2; i < N; ++i)
    t2.insert(new Node(i));

  t1.join_exclusive(t2);
  EXPECT_EQ(t1.size(), N);
  EXPECT_TRUE(t1.verify());

  t1.split_pos(N / 3, t3, t4);
  EXPECT_EQ(t3.size(), N / 3);
  EXPECT_EQ(t4.size(), N - N / 3);
  EXPECT_TRUE(t3.verify());
  EXPECT_TRUE(t4.verify());

  t3.join_exclusive(t4);
  EXPECT_EQ(t3.size(), N);
  EXPECT_TRUE(t3.verify());

  destroy_tree(t3);
}

TEST(JoinSplitStress, RbLargeJoinSplit)
{
  Rb_Tree_Rk<int> t1, t2, t3, t4;
  using Node = Rb_Tree_Rk<int>::Node;
  const int N = 1000;

  for (int i = 0; i < N / 2; ++i)
    t1.insert(new Node(i));
  for (int i = N / 2; i < N; ++i)
    t2.insert(new Node(i));

  t1.join_exclusive(t2);
  EXPECT_EQ(t1.size(), N);
  EXPECT_TRUE(t1.verify());

  t1.split_pos(N / 3, t3, t4);
  EXPECT_EQ(t3.size(), N / 3);
  EXPECT_EQ(t4.size(), N - N / 3);
  EXPECT_TRUE(t3.verify());
  EXPECT_TRUE(t4.verify());

  t3.join_exclusive(t4);
  EXPECT_EQ(t3.size(), N);
  EXPECT_TRUE(t3.verify());

  destroy_tree(t3);
}

// Test split_key_dup

TEST(SplitDup, AvlSplitKeyDup)
{
  Avl_Tree_Rk<int> tree, t1, t2;
  using Node = Avl_Tree_Rk<int>::Node;

  // Insert with duplicates: several 50s
  for (int i = 0; i < 50; ++i)
    tree.insert_dup(new Node(i));
  for (int i = 0; i < 10; ++i)
    tree.insert_dup(new Node(50));
  for (int i = 51; i < 100; ++i)
    tree.insert_dup(new Node(i));

  EXPECT_EQ(tree.size(), 109);  // 50 + 10 + 49

  tree.split_key_dup(50, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  // t1 should have keys <= 50 (0-49 + 10 duplicates of 50 = 50 + 10 = 60)
  EXPECT_EQ(t1.size(), 60);
  // t2 should have keys > 50 (51-99 = 49)
  EXPECT_EQ(t2.size(), 49);

  destroy_tree(t1);
  destroy_tree(t2);
}

TEST(SplitDup, RbSplitKeyDup)
{
  Rb_Tree_Rk<int> tree, t1, t2;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 50; ++i)
    tree.insert_dup(new Node(i));
  for (int i = 0; i < 10; ++i)
    tree.insert_dup(new Node(50));
  for (int i = 51; i < 100; ++i)
    tree.insert_dup(new Node(i));

  EXPECT_EQ(tree.size(), 109);

  tree.split_key_dup(50, t1, t2);

  EXPECT_TRUE(tree.is_empty());
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());

  // t1 should have keys <= 50 (0-49 + 10 duplicates of 50 = 50 + 10 = 60)
  EXPECT_EQ(t1.size(), 60);
  EXPECT_EQ(t2.size(), 49);

  destroy_tree(t1);
  destroy_tree(t2);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}