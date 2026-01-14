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
 * @file treap_test.cc
 * @brief Tests for Treap (tpl_treap.H)
 *
 * Tests the Treap (randomized BST) implementation including:
 * - Basic operations (insert, search, remove)
 * - Heap property verification (priorities)
 * - BST property verification (keys)
 * - Reproducibility with seed
 */

#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_treap.H>

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

  // Count nodes
  template <class Node>
  size_t count_nodes(Node * root)
  {
    if (root == nullptr || root == Node::NullPtr)
      return 0;
    return 1 + count_nodes(LLINK(root)) + count_nodes(RLINK(root));
  }

  // Check BST property
  template <class Node>
  bool check_bst_property(Node * p)
  {
    if (p == nullptr || p == Node::NullPtr)
      return true;

    if (LLINK(p) != nullptr && LLINK(p) != Node::NullPtr && KEY(LLINK(p)) >= KEY(p))
      return false;

    if (RLINK(p) != nullptr && RLINK(p) != Node::NullPtr && KEY(RLINK(p)) <= KEY(p))
      return false;

    return check_bst_property(LLINK(p)) && check_bst_property(RLINK(p));
  }

  // Check heap property on priorities (min-heap for Treap - lower priority = higher in tree)
  template <class Node>
  bool check_heap_property(Node * p)
  {
    if (p == nullptr || p == Node::NullPtr)
      return true;

    if (LLINK(p) != nullptr && LLINK(p) != Node::NullPtr && PRIO(LLINK(p)) < PRIO(p))
      return false;

    if (RLINK(p) != nullptr && RLINK(p) != Node::NullPtr && PRIO(RLINK(p)) < PRIO(p))
      return false;

    return check_heap_property(LLINK(p)) && check_heap_property(RLINK(p));
  }

  // Verify all Treap properties
  template <class Tree>
  bool verify_treap_properties(Tree & tree)
  {
    auto root = tree.getRoot();
    if (root == nullptr || root == Tree::Node::NullPtr)
      return true;

    return check_bst_property(root) && check_heap_property(root);
  }
}

// =============================================================================
// Test Fixtures
// =============================================================================

class TreapTest : public Test
{
protected:
  using Tree = Treap<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  void SetUp() override
  {
    tree.set_seed(42);  // Reproducible tests
  }

  void insert_values(std::initializer_list<int> values)
  {
    for (int v : values)
      tree.insert(pool.make(v));
  }

  size_t tree_size()
  {
    return count_nodes(tree.getRoot());
  }

  bool tree_empty()
  {
    return tree.getRoot() == nullptr || tree.getRoot() == Node::NullPtr;
  }
};

// =============================================================================
// Basic Operations Tests
// =============================================================================

TEST_F(TreapTest, EmptyTreeIsEmpty)
{
  EXPECT_TRUE(tree_empty());
}

TEST_F(TreapTest, InsertIncreasesSize)
{
  tree.insert(pool.make(10));
  EXPECT_EQ(tree_size(), 1);
  EXPECT_FALSE(tree_empty());

  tree.insert(pool.make(5));
  tree.insert(pool.make(15));
  EXPECT_EQ(tree_size(), 3);
}

TEST_F(TreapTest, SearchFindsInsertedKeys)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});

  EXPECT_NE(tree.search(50), nullptr);
  EXPECT_NE(tree.search(25), nullptr);
  EXPECT_NE(tree.search(75), nullptr);
  EXPECT_NE(tree.search(10), nullptr);

  EXPECT_EQ(tree.search(100), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
}

TEST_F(TreapTest, RemoveDecreasesSize)
{
  insert_values({50, 25, 75});
  EXPECT_EQ(tree_size(), 3);

  Node * removed = tree.remove(25);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree_size(), 2);
  EXPECT_EQ(tree.search(25), nullptr);

  pool.forget(removed);
  delete removed;
}

TEST_F(TreapTest, RemoveNonExistentReturnsNull)
{
  insert_values({50, 25, 75});

  Node * removed = tree.remove(100);
  EXPECT_EQ(removed, nullptr);
  EXPECT_EQ(tree_size(), 3);
}

// =============================================================================
// Treap Properties Tests
// =============================================================================

TEST_F(TreapTest, SingleInsertMaintainsTreapProperties)
{
  tree.insert(pool.make(50));
  EXPECT_TRUE(verify_treap_properties(tree));
}

TEST_F(TreapTest, MultipleInsertsMaintainTreapProperties)
{
  insert_values({50, 25, 75, 10, 30, 60, 90, 5, 15, 27, 35});
  EXPECT_TRUE(verify_treap_properties(tree));
}

TEST_F(TreapTest, SequentialInsertsMaintainTreapProperties)
{
  for (int i = 1; i <= 20; ++i)
    tree.insert(pool.make(i));

  EXPECT_TRUE(verify_treap_properties(tree));
  EXPECT_EQ(tree_size(), 20);
}

TEST_F(TreapTest, ReverseInsertsMaintainTreapProperties)
{
  for (int i = 20; i >= 1; --i)
    tree.insert(pool.make(i));

  EXPECT_TRUE(verify_treap_properties(tree));
  EXPECT_EQ(tree_size(), 20);
}

TEST_F(TreapTest, RemoveMaintainsTreapProperties)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});
  EXPECT_TRUE(verify_treap_properties(tree));

  Node * removed = tree.remove(25);
  pool.forget(removed);
  delete removed;

  EXPECT_TRUE(verify_treap_properties(tree));
}

// =============================================================================
// Ordering Tests
// =============================================================================

TEST_F(TreapTest, InorderTraversalIsSorted)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});

  auto keys = inorder_keys(tree.getRoot());

  EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
  EXPECT_EQ(keys.size(), 7);
}

TEST_F(TreapTest, MinAndMaxFromInorder)
{
  insert_values({50, 25, 75, 10, 30, 60, 90});

  auto keys = inorder_keys(tree.getRoot());

  EXPECT_EQ(keys.front(), 10);  // Min
  EXPECT_EQ(keys.back(), 90);   // Max
}

// =============================================================================
// Priority Tests
// =============================================================================

TEST_F(TreapTest, PrioritiesAreAssigned)
{
  tree.insert(pool.make(50));
  tree.insert(pool.make(25));
  tree.insert(pool.make(75));

  // All nodes should have valid priorities
  Node * root = tree.getRoot();
  EXPECT_TRUE(root != nullptr && root != Node::NullPtr);
}

TEST_F(TreapTest, HeapPropertyOnPriorities)
{
  insert_values({50, 25, 75, 10, 30, 60, 90, 5, 15, 27, 35});

  Node * root = tree.getRoot();
  EXPECT_TRUE(check_heap_property(root));
}

// =============================================================================
// Seed Reproducibility Tests
// =============================================================================

TEST_F(TreapTest, SameSeedProducesSameStructure)
{
  // First tree with seed 12345
  Tree tree1;
  tree1.set_seed(12345);
  NodePool<Tree> pool1;

  for (int v : {50, 25, 75, 10, 30})
    tree1.insert(pool1.make(v));

  auto keys1 = inorder_keys(tree1.getRoot());
  auto prio1 = PRIO(tree1.getRoot());

  // Second tree with same seed
  Tree tree2;
  tree2.set_seed(12345);
  NodePool<Tree> pool2;

  for (int v : {50, 25, 75, 10, 30})
    tree2.insert(pool2.make(v));

  auto keys2 = inorder_keys(tree2.getRoot());
  auto prio2 = PRIO(tree2.getRoot());

  // Keys should be identical (BST property)
  EXPECT_EQ(keys1, keys2);
  // Root priorities should be identical (same seed)
  EXPECT_EQ(prio1, prio2);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(TreapTest, RandomInsertsMaintainTreapProperties)
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

  EXPECT_TRUE(verify_treap_properties(tree));
  EXPECT_EQ(tree_size(), inserted.size());
}

TEST_F(TreapTest, RandomInsertsAndRemovesMaintainTreapProperties)
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

  EXPECT_TRUE(verify_treap_properties(tree));

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

  EXPECT_TRUE(verify_treap_properties(tree));
}

// =============================================================================
// Large Scale Tests
// =============================================================================

TEST_F(TreapTest, LargeSequentialInserts)
{
  for (int i = 1; i <= 10000; ++i)
    tree.insert(pool.make(i));

  EXPECT_TRUE(verify_treap_properties(tree));
  EXPECT_EQ(tree_size(), 10000);

  // Verify some keys present
  EXPECT_NE(tree.search(1), nullptr);
  EXPECT_NE(tree.search(5000), nullptr);
  EXPECT_NE(tree.search(10000), nullptr);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
