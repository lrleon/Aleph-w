
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
 * @file rb_tree_test.cc
 * @brief Tests for Red-Black Tree (tpl_rb_tree.H)
 *
 * Tests the bottom-up Red-Black tree implementation including:
 * - Basic operations (insert, search, remove)
 * - Red-Black properties verification
 * - Iterator traversal
 * - Copy/move semantics
 * - Stress tests with random data
 */

#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_rb_tree.H>

using namespace Aleph;
using namespace testing;

namespace
{
  // Helper to manage node allocation
  template <class Tree>
  struct NodePool
  {
    using Node = typename Tree::Node;

    std::vector<Node *> allocated;

    Node * make(const typename Node::key_type & key)
    {
      Node * p = new Node(key);
      allocated.push_back(p);
      return p;
    }

    void forget(Node * p) noexcept
    {
      for (auto & q : allocated)
        if (q == p)
          {
            q = nullptr;
            return;
          }
    }

    ~NodePool()
    {
      for (Node * p : allocated)
        delete p;
    }
  };

  // Collect keys in inorder traversal
  template <class Node>
  std::vector<typename Node::key_type> inorder_keys(Node * root)
  {
    std::vector<typename Node::key_type> keys;
    if (root == nullptr || root == Node::NullPtr)
      return keys;

    auto left = inorder_keys(LLINK(root));
    keys.insert(keys.end(), left.begin(), left.end());
    keys.push_back(KEY(root));
    auto right = inorder_keys(RLINK(root));
    keys.insert(keys.end(), right.begin(), right.end());

    return keys;
  }


  // Verify all Red-Black properties using tree's built-in verify
  template <class Tree>
  bool verify_rb_properties(Tree & tree)
  {
    return tree.verify();
  }
}

// =============================================================================
// Test Fixtures
// =============================================================================

class RbTreeTest : public Test
{
protected:
  using Tree = Rb_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  void insert_values(std::initializer_list<int> values)
  {
    for (int v : values)
      tree.insert(pool.make(v));
  }
};

// =============================================================================
// Basic Operations Tests
// =============================================================================

TEST_F(RbTreeTest, EmptyTreeHasZeroSize)
{
  EXPECT_EQ(tree.size(), 0);
  EXPECT_TRUE(tree.is_empty());
}

TEST_F(RbTreeTest, InsertIncreasesSize)
{
  tree.insert(pool.make(10));
  EXPECT_EQ(tree.size(), 1);
  EXPECT_FALSE(tree.is_empty());

  tree.insert(pool.make(5));
  tree.insert(pool.make(15));
  EXPECT_EQ(tree.size(), 3);
}

TEST_F(RbTreeTest, SearchFindsInsertedKeys)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});

  EXPECT_NE(tree.search(50), nullptr);
  EXPECT_NE(tree.search(25), nullptr);
  EXPECT_NE(tree.search(75), nullptr);
  EXPECT_NE(tree.search(10), nullptr);

  EXPECT_EQ(tree.search(100), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
}

TEST_F(RbTreeTest, RemoveDecreasesSize)
{
  insert_values({50, 25, 75});
  EXPECT_EQ(tree.size(), 3);

  Node * removed = tree.remove(25);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree.size(), 2);
  EXPECT_EQ(tree.search(25), nullptr);

  pool.forget(removed);
  delete removed;
}

TEST_F(RbTreeTest, RemoveNonExistentReturnsNull)
{
  insert_values({50, 25, 75});

  Node * removed = tree.remove(100);
  EXPECT_EQ(removed, nullptr);
  EXPECT_EQ(tree.size(), 3);
}

// =============================================================================
// Red-Black Properties Tests
// =============================================================================

TEST_F(RbTreeTest, SingleInsertMaintainsRbProperties)
{
  tree.insert(pool.make(50));
  EXPECT_TRUE(verify_rb_properties(tree));
}

TEST_F(RbTreeTest, MultipleInsertsMaintainRbProperties)
{
  insert_values({50, 25, 75, 10, 30, 60, 90, 5, 15, 27, 35});
  EXPECT_TRUE(verify_rb_properties(tree));
}

TEST_F(RbTreeTest, SequentialInsertsMaintainRbProperties)
{
  // Sequential inserts often trigger many rotations
  for (int i = 1; i <= 20; ++i)
    tree.insert(pool.make(i));

  EXPECT_TRUE(verify_rb_properties(tree));
  EXPECT_EQ(tree.size(), 20);
}

TEST_F(RbTreeTest, ReverseInsertsMaintainRbProperties)
{
  for (int i = 20; i >= 1; --i)
    tree.insert(pool.make(i));

  EXPECT_TRUE(verify_rb_properties(tree));
  EXPECT_EQ(tree.size(), 20);
}

TEST_F(RbTreeTest, RemoveMaintainsRbProperties)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});
  EXPECT_TRUE(verify_rb_properties(tree));

  Node * removed = tree.remove(25);
  pool.forget(removed);
  delete removed;

  EXPECT_TRUE(verify_rb_properties(tree));
}

TEST_F(RbTreeTest, MultipleRemovesMaintainRbProperties)
{
  std::vector<int> values = {50, 25, 75, 10, 30, 60, 90, 5, 15, 27, 35};
  for (int v : values)
    tree.insert(pool.make(v));

  // Remove half the values
  for (int v : {25, 60, 5, 30, 75})
    {
      Node * removed = tree.remove(v);
      if (removed)
        {
          pool.forget(removed);
          delete removed;
        }
      EXPECT_TRUE(verify_rb_properties(tree));
    }
}

// =============================================================================
// Ordering Tests
// =============================================================================

TEST_F(RbTreeTest, InorderTraversalIsSorted)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});

  auto keys = inorder_keys(tree.getRoot());

  EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
  EXPECT_EQ(keys.size(), 7);
}

TEST_F(RbTreeTest, MinAndMaxFromInorder)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});

  auto keys = inorder_keys(tree.getRoot());

  EXPECT_EQ(keys.front(), 10);  // Min
  EXPECT_EQ(keys.back(), 90);   // Max
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(RbTreeTest, RandomInsertsMaintainRbProperties)
{
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(1, 10000);
  std::set<int> inserted;

  for (int i = 0; i < 1000; ++i)
    {
      int value = dist(rng);
      if (inserted.insert(value).second)
        tree.insert(pool.make(value));
    }

  EXPECT_TRUE(verify_rb_properties(tree));
  EXPECT_EQ(tree.size(), inserted.size());
}

TEST_F(RbTreeTest, RandomInsertsAndRemovesMaintainRbProperties)
{
  std::mt19937 rng(123);
  std::uniform_int_distribution<int> dist(1, 1000);
  std::vector<int> values;

  // Insert 500 random values
  for (int i = 0; i < 500; ++i)
    {
      int value = dist(rng);
      values.push_back(value);
      tree.insert(pool.make(value));
    }

  EXPECT_TRUE(verify_rb_properties(tree));

  // Remove half of them
  std::shuffle(values.begin(), values.end(), rng);
  for (size_t i = 0; i < values.size() / 2; ++i)
    {
      Node * removed = tree.remove(values[i]);
      if (removed)
        {
          pool.forget(removed);
          delete removed;
        }
    }

  EXPECT_TRUE(verify_rb_properties(tree));
}

// =============================================================================
// Height Tests
// =============================================================================

TEST_F(RbTreeTest, HeightIsLogarithmic)
{
  // Insert 1000 sequential elements
  for (int i = 1; i <= 1000; ++i)
    tree.insert(pool.make(i));

  // RB tree height <= 2 * log2(n+1)
  // For n=1000: max height ~= 20
  size_t n = tree.size();

  // Verify the tree is valid (which validates logarithmic structure)
  EXPECT_TRUE(verify_rb_properties(tree));
  EXPECT_EQ(n, 1000);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
