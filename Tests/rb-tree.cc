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
 * @file rb-tree.cc
 * @brief Tests for Rb Tree
 */

#include <algorithm>
#include <cmath>
#include <random>
#include <set>
#include <vector>
#include <functional>

#include <gtest/gtest.h>

#include <tpl_rb_tree.H>
#include <tpl_hRbTree.H>

using namespace Aleph;
using namespace testing;

namespace
{
  using Tree = Rb_Tree<int>;
  using Node = Tree::Node;

  using HybridTree = HtdRbTree<int>;
  using HybridNode = HybridTree::Node;

  struct NodePool
  {
    std::vector<Node *> allocated;

    Node * make(const int k)
    {
      auto * p = new Node(k);
      allocated.push_back(p);
      return p;
    }

    void forget(const Node * p) noexcept
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
      for (const auto * p : allocated)
        delete p;
    }
  };

  struct HybridNodePool
  {
    std::vector<HybridNode *> allocated;

    HybridNode * make(const int k)
    {
      auto * p = new HybridNode(k);
      allocated.push_back(p);
      return p;
    }

    void forget(const HybridNode * p) noexcept
    {
      for (auto & q : allocated)
        if (q == p)
          {
            q = nullptr;
            return;
          }
    }

    ~HybridNodePool()
    {
      for (const auto * p : allocated)
        delete p;
    }
  };

  std::vector<int> inorder_keys(Node * root)
  {
    std::vector<int> keys;
    if (root == Node::NullPtr)
      return keys;

    auto left = inorder_keys(LLINK(root));
    keys.insert(keys.end(), left.begin(), left.end());
    keys.push_back(KEY(root));
    auto right = inorder_keys(RLINK(root));
    keys.insert(keys.end(), right.begin(), right.end());
    return keys;
  }

  size_t count_nodes(Node * root)
  {
    if (root == Node::NullPtr)
      return 0;
    return 1 + count_nodes(LLINK(root)) + count_nodes(RLINK(root));
  }

  void assert_valid_tree(Tree & tree)
  {
    ASSERT_TRUE(tree.verify()) << "Red-black tree invariant violated";
    ASSERT_TRUE(check_bst(tree.getRoot(), tree.key_comp())) << "BST property violated";
  }

  bool tree_is_empty(Tree & tree)
  {
    return tree.getRoot() == Node::NullPtr;
  }

  template <typename NodeT>
  size_t count_nodes_generic(NodeT * root)
  {
    if (root == NodeT::NullPtr)
      return 0;
    return 1 + count_nodes_generic(LLINK(root)) + count_nodes_generic(RLINK(root));
  }

  template <typename NodeT>
  std::vector<int> inorder_keys_generic(NodeT * root)
  {
    std::vector<int> keys;
    if (root == NodeT::NullPtr)
      return keys;

    auto left = inorder_keys_generic(LLINK(root));
    keys.insert(keys.end(), left.begin(), left.end());
    keys.push_back(KEY(root));
    auto right = inorder_keys_generic(RLINK(root));
    keys.insert(keys.end(), right.begin(), right.end());
    return keys;
  }

  void assert_valid_hybrid_tree(HybridTree & tree)
  {
    ASSERT_TRUE(tree.verify()) << "HtdRbTree red-black invariant violated";
    ASSERT_TRUE(check_bst(tree.getRoot(), tree.key_comp())) << "BST property violated";
  }
}

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST(RbTree, EmptyTreeProperties)
{
  Tree tree;

  EXPECT_EQ(tree.getRoot(), Node::NullPtr);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST(RbTree, InsertSingleElement)
{
  Tree tree;
  NodePool pool;

  auto * p = pool.make(42);
  auto * inserted = tree.insert(p);

  EXPECT_EQ(inserted, p);
  EXPECT_NE(tree.getRoot(), Node::NullPtr);
  EXPECT_EQ(tree.getRoot(), p);
  EXPECT_EQ(count_nodes(tree.getRoot()), 1u);
  assert_valid_tree(tree);
}

TEST(RbTree, InsertMultipleElements)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    {
      auto * p = pool.make(k);
      ASSERT_NE(tree.insert(p), nullptr);
    }

  EXPECT_EQ(count_nodes(tree.getRoot()), 7u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 3, 4, 5, 6, 7, 8}));
}

TEST(RbTree, InsertRejectsDuplicates)
{
  Tree tree;
  NodePool pool;

  auto * p1 = pool.make(10);
  EXPECT_NE(tree.insert(p1), nullptr);

  auto * p2 = pool.make(10);
  EXPECT_EQ(tree.insert(p2), nullptr);

  EXPECT_EQ(count_nodes(tree.getRoot()), 1u);
  assert_valid_tree(tree);
}

TEST(RbTree, InsertDupAllowsDuplicates)
{
  Tree tree;
  NodePool pool;

  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(42)), nullptr);

  EXPECT_EQ(count_nodes(tree.getRoot()), 5u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{42, 42, 42, 42, 42}));
}

TEST(RbTree, SearchFindsExistingKey)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  for (int k : {1, 2, 3, 4, 5})
    {
      auto * found = tree.search(k);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(KEY(found), k);
    }

  assert_valid_tree(tree);
}

TEST(RbTree, SearchReturnsNullForMissingKey)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.search(2), nullptr);
  EXPECT_EQ(tree.search(4), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
  EXPECT_EQ(tree.search(6), nullptr);

  assert_valid_tree(tree);
}

TEST(RbTree, SearchOrInsertBehavior)
{
  Tree tree;
  NodePool pool;

  // Insert via search_or_insert
  auto * p1 = pool.make(10);
  auto * ret1 = tree.search_or_insert(p1);
  EXPECT_EQ(ret1, p1);
  EXPECT_EQ(count_nodes(tree.getRoot()), 1u);

  // Search existing via search_or_insert
  auto * p2 = pool.make(10);
  auto * ret2 = tree.search_or_insert(p2);
  EXPECT_NE(ret2, p2);  // Should return existing node
  EXPECT_EQ(KEY(ret2), 10);
  EXPECT_EQ(count_nodes(tree.getRoot()), 1u);

  assert_valid_tree(tree);
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST(RbTree, RemoveExistingKey)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  auto * removed = tree.remove(3);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 3);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(count_nodes(tree.getRoot()), 4u);
  EXPECT_EQ(tree.search(3), nullptr);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 4, 5}));
}

TEST(RbTree, RemoveReturnsNullForMissingKey)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.remove(2), nullptr);
  EXPECT_EQ(tree.remove(4), nullptr);
  EXPECT_EQ(count_nodes(tree.getRoot()), 3u);

  assert_valid_tree(tree);
}

TEST(RbTree, RemoveFromEmptyTree)
{
  Tree tree;

  EXPECT_EQ(tree.remove(42), nullptr);
  EXPECT_TRUE(tree_is_empty(tree));
}

TEST(RbTree, RemoveRoot)
{
  Tree tree;
  NodePool pool;

  tree.insert(pool.make(5));
  tree.insert(pool.make(3));
  tree.insert(pool.make(7));

  auto * removed = tree.remove(5);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 5);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(count_nodes(tree.getRoot()), 2u);
  assert_valid_tree(tree);
}

TEST(RbTree, RemoveAllElements)
{
  Tree tree;
  NodePool pool;

  std::vector<int> keys = {5, 3, 7, 1, 4, 6, 8};
  for (int k : keys)
    tree.insert(pool.make(k));

  for (int k : keys)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
      assert_valid_tree(tree);
    }

  EXPECT_TRUE(tree_is_empty(tree));
}

TEST(RbTree, RemoveInOrder)
{
  Tree tree;
  NodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  for (int k = 1; k <= 10; ++k)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
      assert_valid_tree(tree);
    }

  EXPECT_TRUE(tree_is_empty(tree));
}

TEST(RbTree, RemoveInReverseOrder)
{
  Tree tree;
  NodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  for (int k = 10; k >= 1; --k)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
      assert_valid_tree(tree);
    }

  EXPECT_TRUE(tree_is_empty(tree));
}

TEST(RbTree, RemoveDuplicatesInsertedWithInsertDup)
{
  Tree tree;
  NodePool pool;

  constexpr int key = 7;
  for (int i = 0; i < 4; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(key)), nullptr);

  for (int remaining = 4; remaining > 0; --remaining)
    {
      auto * removed = tree.remove(key);
      ASSERT_NE(removed, nullptr);
      EXPECT_EQ(KEY(removed), key);
      pool.forget(removed);
      delete removed;

      EXPECT_EQ(count_nodes(tree.getRoot()),
                static_cast<size_t>(remaining - 1));
      assert_valid_tree(tree);
    }

  EXPECT_EQ(tree.remove(key), nullptr);
  EXPECT_TRUE(tree_is_empty(tree));
}

// ============================================================================
// Red-Black Properties Tests
// ============================================================================

TEST(RbTree, TreeRemainsValidAfterMultipleInserts)
{
  Tree tree;
  NodePool pool;

  tree.insert(pool.make(5));
  assert_valid_tree(tree);

  tree.insert(pool.make(3));
  tree.insert(pool.make(7));
  tree.insert(pool.make(1));
  tree.insert(pool.make(4));

  assert_valid_tree(tree);
}

TEST(RbTree, NoConsecutiveReds)
{
  Tree tree;
  NodePool pool;

  // Insert in a pattern that would cause consecutive reds without fixing
  for (int k : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
    {
      tree.insert(pool.make(k));
      assert_valid_tree(tree);
    }
}

TEST(RbTree, BlackHeightConsistent)
{
  Tree tree;
  NodePool pool;

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 1000);

  for (int i = 0; i < 100; ++i)
    {
      auto * p = pool.make(dist(rng));
      tree.insert(p);
      // Note: duplicates will fail to insert, that's ok
    }

  assert_valid_tree(tree);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(RbTree, SingleElementOperations)
{
  Tree tree;
  NodePool pool;

  auto * p = pool.make(42);
  tree.insert(p);

  EXPECT_EQ(tree.search(42), p);

  auto * removed = tree.remove(42);
  EXPECT_EQ(removed, p);
  EXPECT_TRUE(tree_is_empty(tree));
  pool.forget(removed);
  delete removed;
}

TEST(RbTree, InsertInDescendingOrder)
{
  Tree tree;
  NodePool pool;

  for (int k = 10; k >= 1; --k)
    tree.insert(pool.make(k));

  EXPECT_EQ(count_nodes(tree.getRoot()), 10u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

TEST(RbTree, InsertInAscendingOrder)
{
  Tree tree;
  NodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  EXPECT_EQ(count_nodes(tree.getRoot()), 10u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

// ============================================================================
// Custom Comparator Tests
// ============================================================================

TEST(RbTree, CustomComparatorGreater)
{
  using TreeGt = Gen_Rb_Tree<RbNode, int, std::greater<int>>;
  using NodeGt = TreeGt::Node;

  TreeGt tree;
  std::vector<NodeGt *> nodes;

  for (int k : {1, 2, 3, 4, 5})
    {
      auto * p = new NodeGt(k);
      nodes.push_back(p);
      tree.insert(p);
    }

  EXPECT_EQ(count_nodes(tree.getRoot()), 5u);
  EXPECT_TRUE(tree.verify());

  // With greater<int>, inorder should be descending
  std::vector<int> keys;
  std::function<void(NodeGt*)> collect = [&](NodeGt* r) {
    if (r == NodeGt::NullPtr) return;
    collect(LLINK(r));
    keys.push_back(KEY(r));
    collect(RLINK(r));
  };
  collect(tree.getRoot());

  EXPECT_EQ(keys, (std::vector<int>{5, 4, 3, 2, 1}));

  for (auto * p : nodes)
    delete p;
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(RbTree, RandomInsertSearchRemove)
{
  Tree tree;
  NodePool pool;
  std::set<int> oracle;

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 500);

  // Insert phase
  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto * p = pool.make(k);
      if (tree.insert(p) != nullptr)
        oracle.insert(k);
      else
        {
          pool.forget(p);
          delete p;
        }

      ASSERT_EQ(count_nodes(tree.getRoot()), oracle.size());
      assert_valid_tree(tree);
    }

  // Verify all elements
  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));

  // Search phase
  for (int i = 0; i < 100; ++i)
    {
      int k = dist(rng);
      auto * found = tree.search(k);
      if (oracle.count(k))
        {
          ASSERT_NE(found, nullptr);
          EXPECT_EQ(KEY(found), k);
        }
      else
        EXPECT_EQ(found, nullptr);

      assert_valid_tree(tree);
    }

  // Remove phase
  for (int i = 0; i < 150; ++i)
    {
      int k = dist(rng);
      auto * removed = tree.remove(k);
      if (oracle.count(k))
        {
          ASSERT_NE(removed, nullptr);
          EXPECT_EQ(KEY(removed), k);
          oracle.erase(k);
          pool.forget(removed);
          delete removed;
        }
      else
        EXPECT_EQ(removed, nullptr);

      ASSERT_EQ(count_nodes(tree.getRoot()), oracle.size());
      assert_valid_tree(tree);
    }

  // Final verification
  keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));
}

TEST(RbTree, LargeTreeOperations)
{
  Tree tree;
  NodePool pool;

  const int N = 1000;

  // Insert N elements
  for (int k = 0; k < N; ++k)
    tree.insert(pool.make(k));

  EXPECT_EQ(count_nodes(tree.getRoot()), static_cast<size_t>(N));
  assert_valid_tree(tree);

  // Remove half
  for (int k = 0; k < N; k += 2)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      pool.forget(removed);
      delete removed;
    }

  EXPECT_EQ(count_nodes(tree.getRoot()), static_cast<size_t>(N / 2));
  assert_valid_tree(tree);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(RbTree, IteratorEmptyTree)
{
  Tree tree;
  Tree::Iterator it(tree);

  EXPECT_FALSE(it.has_curr());
}

TEST(RbTree, IteratorTraversesInOrder)
{
  Tree tree;
  NodePool pool;

  std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  for (int k : expected)
    tree.insert(pool.make(k));

  std::vector<int> result;
  for (Tree::Iterator it(tree); it.has_curr(); it.next())
    result.push_back(KEY(it.get_curr()));

  EXPECT_EQ(result, expected);
}

TEST(RbTree, IteratorAfterRemoval)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  auto * removed = tree.remove(3);
  pool.forget(removed);
  delete removed;

  std::vector<int> result;
  for (Tree::Iterator it(tree); it.has_curr(); it.next())
    result.push_back(KEY(it.get_curr()));

  EXPECT_EQ(result, (std::vector<int>{1, 2, 4, 5}));
}

// ============================================================================
// Verify Method Tests
// ============================================================================

TEST(RbTree, VerifyDetectsValidTree)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  EXPECT_TRUE(tree.verify());
}

// ============================================================================
// Size and Empty Method Tests
// ============================================================================

TEST(RbTree, IsEmptyMethod)
{
  Tree tree;
  NodePool pool;

  EXPECT_TRUE(tree.is_empty());

  tree.insert(pool.make(1));
  EXPECT_FALSE(tree.is_empty());

  auto * removed = tree.remove(1);
  pool.forget(removed);
  delete removed;
  EXPECT_TRUE(tree.is_empty());
}

TEST(RbTree, SizeMethod)
{
  Tree tree;
  NodePool pool;

  EXPECT_EQ(tree.size(), 0u);

  for (int k = 1; k <= 10; ++k)
    {
      tree.insert(pool.make(k));
      EXPECT_EQ(tree.size(), static_cast<size_t>(k));
    }

  for (int k = 1; k <= 5; ++k)
    {
      auto * removed = tree.remove(k);
      pool.forget(removed);
      delete removed;
    }
  EXPECT_EQ(tree.size(), 5u);
}

// ============================================================================
// Swap and Move Semantics Tests
// ============================================================================

TEST(RbTree, SwapTrees)
{
  Tree tree1;
  Tree tree2;
  NodePool pool;

  tree1.insert(pool.make(1));
  tree1.insert(pool.make(2));
  tree1.insert(pool.make(3));

  tree2.insert(pool.make(10));
  tree2.insert(pool.make(11));

  tree1.swap(tree2);

  EXPECT_EQ(count_nodes(tree1.getRoot()), 2u);
  EXPECT_EQ(count_nodes(tree2.getRoot()), 3u);

  auto keys1 = inorder_keys(tree1.getRoot());
  EXPECT_EQ(keys1, (std::vector<int>{10, 11}));

  auto keys2 = inorder_keys(tree2.getRoot());
  EXPECT_EQ(keys2, (std::vector<int>{1, 2, 3}));

  assert_valid_tree(tree1);
  assert_valid_tree(tree2);
}

TEST(RbTree, MoveConstructor)
{
  Tree tree1;
  auto * p1 = new Node(1);
  auto * p2 = new Node(2);
  auto * p3 = new Node(3);
  tree1.insert(p1);
  tree1.insert(p2);
  tree1.insert(p3);

  Tree tree2(std::move(tree1));

  EXPECT_TRUE(tree1.is_empty());
  EXPECT_EQ(tree2.size(), 3u);
  assert_valid_tree(tree2);

  // Clean up
  delete tree2.remove(1);
  delete tree2.remove(2);
  delete tree2.remove(3);
}

TEST(RbTree, MoveAssignment)
{
  Tree tree1;
  Tree tree2;
  auto * p1 = new Node(1);
  auto * p2 = new Node(2);
  tree1.insert(p1);
  tree1.insert(p2);

  tree2 = std::move(tree1);

  EXPECT_TRUE(tree1.is_empty());
  EXPECT_EQ(tree2.size(), 2u);
  assert_valid_tree(tree2);

  delete tree2.remove(1);
  delete tree2.remove(2);
}

// ============================================================================
// Hybrid red-black tree (tpl_hRbTree.H) compatibility tests
// ============================================================================

TEST(HtdRbTreeCompat, EmptyTreeProperties)
{
  HybridTree tree;
  EXPECT_EQ(tree.getRoot(), HybridNode::NullPtr);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_TRUE(tree.verify());
  EXPECT_TRUE(check_bst(tree.getRoot(), tree.key_comp()));
}

TEST(HtdRbTreeCompat, InsertRejectsDuplicates)
{
  HybridTree tree;
  HybridNodePool pool;

  auto * p1 = pool.make(10);
  EXPECT_NE(tree.insert(p1), nullptr);
  EXPECT_EQ(tree.size(), 1u);

  auto * p2 = pool.make(10);
  EXPECT_EQ(tree.insert(p2), nullptr);
  EXPECT_EQ(tree.size(), 1u);

  EXPECT_TRUE(tree.verify());
  EXPECT_TRUE(check_bst(tree.getRoot(), tree.key_comp()));
}

TEST(HtdRbTreeCompat, InsertDupAllowsDuplicates)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(42)), nullptr);

  EXPECT_EQ(tree.size(), 5u);
  EXPECT_EQ(count_nodes_generic(tree.getRoot()), 5u);
  EXPECT_TRUE(tree.verify());
  EXPECT_TRUE(check_bst(tree.getRoot(), tree.key_comp()));
}

TEST(HtdRbTreeCompat, SearchOrInsertBehavior)
{
  HybridTree tree;
  HybridNodePool pool;

  auto * p1 = pool.make(10);
  auto * ret1 = tree.search_or_insert(p1);
  EXPECT_EQ(ret1, p1);
  EXPECT_EQ(tree.size(), 1u);

  auto * p2 = pool.make(10);
  auto * ret2 = tree.search_or_insert(p2);
  EXPECT_NE(ret2, p2);
  EXPECT_EQ(KEY(ret2), 10);
  EXPECT_EQ(tree.size(), 1u);

  EXPECT_TRUE(tree.verify());
  EXPECT_TRUE(check_bst(tree.getRoot(), tree.key_comp()));
}

TEST(HtdRbTreeCompat, RemoveExistingKey)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  auto * removed = tree.remove(3);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 3);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(tree.search(3), nullptr);
  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, RemoveFromSingleElementTree)
{
  HybridTree tree;
  HybridNodePool pool;

  tree.insert(pool.make(42));
  EXPECT_EQ(tree.size(), 1u);

  auto * removed = tree.remove(42);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 42);
  pool.forget(removed);
  delete removed;

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
}

TEST(HtdRbTreeCompat, RemoveAllElementsInOrder)
{
  HybridTree tree;
  HybridNodePool pool;

  std::vector<int> keys = {5, 3, 7, 1, 4, 6, 8};
  for (int k : keys)
    tree.insert(pool.make(k));

  std::sort(keys.begin(), keys.end());
  for (int k : keys)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
      assert_valid_hybrid_tree(tree);
    }

  EXPECT_TRUE(tree.is_empty());
}

TEST(HtdRbTreeCompat, RemoveReturnsNullForMissingKey)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.remove(2), nullptr);
  EXPECT_EQ(tree.remove(4), nullptr);
  EXPECT_EQ(tree.size(), 3u);
  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, RemoveDuplicatesInsertedWithInsertDup)
{
  HybridTree tree;
  HybridNodePool pool;

  constexpr int key = 5;
  for (int i = 0; i < 3; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(key)), nullptr);

  for (int remaining = 3; remaining > 0; --remaining)
    {
      auto * removed = tree.remove(key);
      ASSERT_NE(removed, nullptr);
      EXPECT_EQ(KEY(removed), key);
      pool.forget(removed);
      delete removed;

      EXPECT_EQ(tree.size(), static_cast<size_t>(remaining - 1));
      assert_valid_hybrid_tree(tree);
    }

  EXPECT_EQ(tree.remove(key), nullptr);
  EXPECT_TRUE(tree.is_empty());
}

TEST(HtdRbTreeCompat, IteratorTraversesInOrder)
{
  HybridTree tree;
  HybridNodePool pool;

  std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7};
  for (int k : {4, 2, 6, 1, 3, 5, 7})
    tree.insert(pool.make(k));

  std::vector<int> got;
  for (HybridTree::Iterator it(tree); it.has_curr(); it.next())
    got.push_back(KEY(it.get_curr()));

  EXPECT_EQ(got, expected);
  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, IteratorEmptyTree)
{
  HybridTree tree;
  HybridTree::Iterator it(tree);

  EXPECT_FALSE(it.has_curr());
}

TEST(HtdRbTreeCompat, SwapTrees)
{
  HybridTree tree1;
  HybridTree tree2;
  HybridNodePool pool;

  tree1.insert(pool.make(1));
  tree1.insert(pool.make(2));
  tree1.insert(pool.make(3));

  tree2.insert(pool.make(10));
  tree2.insert(pool.make(11));

  ASSERT_EQ(tree1.size(), 3u);
  ASSERT_EQ(tree2.size(), 2u);

  tree1.swap(tree2);

  EXPECT_EQ(tree1.size(), 2u);
  EXPECT_EQ(tree2.size(), 3u);

  auto keys1 = inorder_keys_generic(tree1.getRoot());
  EXPECT_EQ(keys1, (std::vector<int>{10, 11}));

  auto keys2 = inorder_keys_generic(tree2.getRoot());
  EXPECT_EQ(keys2, (std::vector<int>{1, 2, 3}));

  assert_valid_hybrid_tree(tree1);
  assert_valid_hybrid_tree(tree2);
}

TEST(HtdRbTreeCompat, StatefulComparatorAffectsEquality)
{
  struct AbsLess
  {
    bool operator()(int a, int b) const { return std::abs(a) < std::abs(b); }
  };

  using TreeAbs = HtdRbTree<int, AbsLess>;
  using NodeAbs = TreeAbs::Node;

  TreeAbs tree(AbsLess{});
  auto * p = new NodeAbs(1);
  ASSERT_NE(tree.insert(p), nullptr);

  auto * found = tree.search(-1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, p);
  EXPECT_TRUE(tree.verify());
  EXPECT_TRUE(check_bst(tree.getRoot(), tree.key_comp()));

  auto * removed = tree.remove(1);
  ASSERT_NE(removed, nullptr);
  delete removed;
}

TEST(HtdRbTreeCompat, NegativeKeys)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {-5, -3, -1, 0, 1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 7u);
  assert_valid_hybrid_tree(tree);

  auto keys = inorder_keys_generic(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{-5, -3, -1, 0, 1, 3, 5}));

  // Search for negative keys
  EXPECT_NE(tree.search(-3), nullptr);
  EXPECT_EQ(tree.search(-2), nullptr);
}

TEST(HtdRbTreeCompat, RandomInsertSearchRemove)
{
  HybridTree tree;
  HybridNodePool pool;
  std::set<int> oracle;

  std::mt19937 rng(123);
  std::uniform_int_distribution<int> dist(0, 500);

  // Insert phase
  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto * p = pool.make(k);
      if (tree.insert(p) != nullptr)
        oracle.insert(k);
      else
        {
          pool.forget(p);
          delete p;
        }

      ASSERT_EQ(tree.size(), oracle.size());
      assert_valid_hybrid_tree(tree);
    }

  // Verify all elements
  auto keys = inorder_keys_generic(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));

  // Search phase
  for (int i = 0; i < 100; ++i)
    {
      int k = dist(rng);
      auto * found = tree.search(k);
      if (oracle.count(k))
        {
          ASSERT_NE(found, nullptr);
          EXPECT_EQ(KEY(found), k);
        }
      else
        EXPECT_EQ(found, nullptr);
    }

  // Remove phase
  for (int i = 0; i < 150; ++i)
    {
      int k = dist(rng);
      auto * removed = tree.remove(k);
      if (oracle.count(k))
        {
          ASSERT_NE(removed, nullptr);
          EXPECT_EQ(KEY(removed), k);
          oracle.erase(k);
          pool.forget(removed);
          delete removed;
        }
      else
        EXPECT_EQ(removed, nullptr);

      ASSERT_EQ(tree.size(), oracle.size());
      assert_valid_hybrid_tree(tree);
    }

  // Final verification
  keys = inorder_keys_generic(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));
}

TEST(HtdRbTreeCompat, InsertSingleElement)
{
  HybridTree tree;
  HybridNodePool pool;

  auto * p = pool.make(42);
  auto * inserted = tree.insert(p);

  EXPECT_EQ(inserted, p);
  EXPECT_NE(tree.getRoot(), HybridNode::NullPtr);
  EXPECT_EQ(tree.getRoot(), p);
  EXPECT_EQ(tree.size(), 1u);
  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, InsertMultipleElements)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    ASSERT_NE(tree.insert(pool.make(k)), nullptr);

  EXPECT_EQ(tree.size(), 7u);
  assert_valid_hybrid_tree(tree);

  auto keys = inorder_keys_generic(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 3, 4, 5, 6, 7, 8}));
}

TEST(HtdRbTreeCompat, SearchFindsExistingKey)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  for (int k : {1, 2, 3, 4, 5})
    {
      auto * found = tree.search(k);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(KEY(found), k);
    }

  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, SearchReturnsNullForMissingKey)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.search(2), nullptr);
  EXPECT_EQ(tree.search(4), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
  EXPECT_EQ(tree.search(6), nullptr);

  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, RemoveFromEmptyTree)
{
  HybridTree tree;

  EXPECT_EQ(tree.remove(42), nullptr);
  EXPECT_TRUE(tree.is_empty());
}

TEST(HtdRbTreeCompat, RemoveRoot)
{
  HybridTree tree;
  HybridNodePool pool;

  tree.insert(pool.make(5));
  tree.insert(pool.make(3));
  tree.insert(pool.make(7));

  auto * removed = tree.remove(5);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 5);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 2u);
  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, RemoveInReverseOrder)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  for (int k = 10; k >= 1; --k)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
      assert_valid_hybrid_tree(tree);
    }

  EXPECT_TRUE(tree.is_empty());
}

TEST(HtdRbTreeCompat, InsertInDescendingOrder)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k = 10; k >= 1; --k)
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 10u);
  assert_valid_hybrid_tree(tree);

  auto keys = inorder_keys_generic(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

TEST(HtdRbTreeCompat, InsertInAscendingOrder)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 10u);
  assert_valid_hybrid_tree(tree);

  auto keys = inorder_keys_generic(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

TEST(HtdRbTreeCompat, LargeTreeOperations)
{
  HybridTree tree;
  HybridNodePool pool;

  const int N = 1000;

  for (int k = 0; k < N; ++k)
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_hybrid_tree(tree);

  // Remove half
  for (int k = 0; k < N; k += 2)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      pool.forget(removed);
      delete removed;
    }

  EXPECT_EQ(tree.size(), static_cast<size_t>(N / 2));
  assert_valid_hybrid_tree(tree);
}

TEST(HtdRbTreeCompat, CustomComparatorGreater)
{
  using TreeGt = HtdRbTree<int, std::greater<int>>;
  using NodeGt = TreeGt::Node;

  TreeGt tree;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(new NodeGt(k));

  EXPECT_EQ(tree.size(), 5u);
  EXPECT_TRUE(tree.verify());

  // With greater<int>, inorder should be descending
  std::vector<int> keys;
  for (TreeGt::Iterator it(tree); it.has_curr(); it.next())
    keys.push_back(KEY(it.get_curr()));

  EXPECT_EQ(keys, (std::vector<int>{5, 4, 3, 2, 1}));

  // Clean up
  for (int k : {1, 2, 3, 4, 5})
    delete tree.remove(k);
}

TEST(HtdRbTreeCompat, IteratorAfterRemoval)
{
  HybridTree tree;
  HybridNodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  auto * removed = tree.remove(3);
  pool.forget(removed);
  delete removed;

  std::vector<int> result;
  for (HybridTree::Iterator it(tree); it.has_curr(); it.next())
    result.push_back(KEY(it.get_curr()));

  EXPECT_EQ(result, (std::vector<int>{1, 2, 4, 5}));
}

TEST(HtdRbTreeCompat, MoveConstructor)
{
  HybridTree tree1;
  auto * p1 = new HybridNode(1);
  auto * p2 = new HybridNode(2);
  auto * p3 = new HybridNode(3);
  tree1.insert(p1);
  tree1.insert(p2);
  tree1.insert(p3);

  HybridTree tree2(std::move(tree1));

  EXPECT_TRUE(tree1.is_empty());
  EXPECT_EQ(tree2.size(), 3u);
  assert_valid_hybrid_tree(tree2);

  delete tree2.remove(1);
  delete tree2.remove(2);
  delete tree2.remove(3);
}

TEST(HtdRbTreeCompat, MoveAssignment)
{
  HybridTree tree1;
  HybridTree tree2;
  auto * p1 = new HybridNode(1);
  auto * p2 = new HybridNode(2);
  tree1.insert(p1);
  tree1.insert(p2);

  tree2 = std::move(tree1);

  EXPECT_TRUE(tree1.is_empty());
  EXPECT_EQ(tree2.size(), 2u);
  assert_valid_hybrid_tree(tree2);

  delete tree2.remove(1);
  delete tree2.remove(2);
}

// ============================================================================

TEST(RbTreeVtl, BasicOperations)
{
  using TreeVtl = Rb_Tree_Vtl<int>;
  using NodeVtl = TreeVtl::Node;

  TreeVtl tree;

  for (int k : {1, 2, 3, 4, 5})
    {
      auto * p = new NodeVtl(k);
      tree.insert(p);
    }

  EXPECT_EQ(count_nodes_generic(tree.getRoot()), 5u);
  EXPECT_EQ(tree.size(), 5u);
  EXPECT_TRUE(tree.verify());

  auto * found = tree.search(3);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(KEY(found), 3);

  // Properly remove and delete each node
  for (int k : {1, 2, 3, 4, 5})
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      delete removed;
    }

  EXPECT_TRUE(tree.is_empty());
}

TEST(RbTree, NegativeKeys)
{
  Tree tree;
  NodePool pool;

  for (int k : {-5, -3, -1, 0, 1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 7u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{-5, -3, -1, 0, 1, 3, 5}));

  EXPECT_NE(tree.search(-3), nullptr);
  EXPECT_EQ(tree.search(-2), nullptr);
}

TEST(RbTree, CustomComparatorWithRemove)
{
  using TreeGt = Gen_Rb_Tree<RbNode, int, std::greater<int>>;
  using NodeGt = TreeGt::Node;

  TreeGt tree;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(new NodeGt(k));

  EXPECT_EQ(tree.size(), 5u);
  EXPECT_TRUE(tree.verify());

  // Remove some elements
  auto * removed = tree.remove(3);
  ASSERT_NE(removed, nullptr);
  delete removed;

  removed = tree.remove(1);
  ASSERT_NE(removed, nullptr);
  delete removed;

  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verify());

  // Clean up remaining
  for (int k : {2, 4, 5})
    delete tree.remove(k);
}

// ============================================================================
// Stress and Fuzz Tests
// ============================================================================

TEST(RbTree, Stress_AscendingInsertion)
{
  Tree tree;
  NodePool pool;
  
  // Ascending insertion is worst case for naive BST
  const int N = 10000;
  for (int k = 0; k < N; ++k)
    {
      tree.insert(pool.make(k));
      if (k % 1000 == 0)
        assert_valid_tree(tree);
    }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
  
  // Verify all elements
  for (int k = 0; k < N; ++k)
    ASSERT_NE(tree.search(k), nullptr) << "Missing key " << k;
}

TEST(RbTree, Stress_DescendingInsertion)
{
  Tree tree;
  NodePool pool;
  
  const int N = 10000;
  for (int k = N - 1; k >= 0; --k)
    {
      tree.insert(pool.make(k));
      if (k % 1000 == 0)
        assert_valid_tree(tree);
    }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
}

TEST(RbTree, Stress_ZigzagInsertion)
{
  Tree tree;
  NodePool pool;
  
  // Zigzag pattern: 0, N-1, 1, N-2, 2, N-3, ...
  const int N = 5000;
  for (int i = 0; i < N; ++i)
    {
      int k = (i % 2 == 0) ? i / 2 : N - 1 - i / 2;
      tree.insert(pool.make(k));
    }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
}

TEST(RbTree, Fuzz_LargeScaleRandomOps)
{
  Tree tree;
  NodePool pool;
  std::set<int> oracle;
  
  std::mt19937 gen(98765);
  std::uniform_int_distribution<> key_dist(0, 50000);
  std::uniform_int_distribution<> op_dist(0, 2);
  
  for (int iter = 0; iter < 20000; ++iter)
    {
      int key = key_dist(gen);
      int op = op_dist(gen);
      
      if (op == 0)  // insert
        {
          auto * p = pool.make(key);
          if (tree.insert(p) != nullptr)
            oracle.insert(key);
          else
            {
              pool.forget(p);
              delete p;
            }
        }
      else if (op == 1 && !oracle.empty())  // remove
        {
          // Pick a random key from oracle
          auto it = oracle.begin();
          std::advance(it, gen() % oracle.size());
          int k = *it;
          
          auto * removed = tree.remove(k);
          ASSERT_NE(removed, nullptr) << "Failed to remove existing key " << k;
          pool.forget(removed);
          delete removed;
          oracle.erase(k);
        }
      else  // search
        {
          auto * found = tree.search(key);
          if (oracle.count(key))
            ASSERT_NE(found, nullptr);
          else
            EXPECT_EQ(found, nullptr);
        }
      
      EXPECT_EQ(tree.size(), oracle.size());
      
      if (iter % 2000 == 0)
        assert_valid_tree(tree);
    }
  
  assert_valid_tree(tree);
  
  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));
}

TEST(RbTree, Stress_BulkInsertBulkRemove)
{
  Tree tree;
  NodePool pool;
  
  const int N = 10000;
  
  // Bulk insert
  for (int k = 0; k < N; ++k)
    tree.insert(pool.make(k));
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
  
  // Bulk remove in random order
  std::vector<int> keys_to_remove(N);
  std::iota(keys_to_remove.begin(), keys_to_remove.end(), 0);
  std::mt19937 gen(11111);
  std::shuffle(keys_to_remove.begin(), keys_to_remove.end(), gen);
  
  for (int k : keys_to_remove)
    {
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
    }
  
  EXPECT_TRUE(tree.is_empty());
}

TEST(RbTree, Stress_ManyDuplicates)
{
  Tree tree;
  NodePool pool;
  
  // Insert many duplicates using insert_dup
  const int N = 1000;
  const int DUPS = 10;
  
  for (int k = 0; k < N; ++k)
    for (int d = 0; d < DUPS; ++d)
      tree.insert_dup(pool.make(k));
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N * DUPS));
  assert_valid_tree(tree);
  
  // Remove all
  for (int k = 0; k < N; ++k)
    for (int d = 0; d < DUPS; ++d)
      {
        auto * removed = tree.remove(k);
        ASSERT_NE(removed, nullptr);
        pool.forget(removed);
        delete removed;
      }
  
  EXPECT_TRUE(tree.is_empty());
}

TEST(RbTree, Stress_AlternatingInsertRemove)
{
  Tree tree;
  NodePool pool;
  std::set<int> oracle;
  
  std::mt19937 gen(22222);
  std::uniform_int_distribution<> key_dist(0, 1000);
  
  for (int iter = 0; iter < 10000; ++iter)
    {
      int key = key_dist(gen);
      
      if (iter % 2 == 0)  // insert
        {
          auto * p = pool.make(key);
          if (tree.insert(p) != nullptr)
            oracle.insert(key);
          else
            {
              pool.forget(p);
              delete p;
            }
        }
      else if (!oracle.empty())  // remove random existing
        {
          auto it = oracle.begin();
          std::advance(it, gen() % oracle.size());
          int k = *it;
          
          auto * removed = tree.remove(k);
          ASSERT_NE(removed, nullptr);
          pool.forget(removed);
          delete removed;
          oracle.erase(k);
        }
      
      EXPECT_EQ(tree.size(), oracle.size());
    }
  
  assert_valid_tree(tree);
}

TEST(RbTree, Stress_StringKeys)
{
  using StrTree = Rb_Tree<std::string>;
  using StrNode = StrTree::Node;
  
  StrTree tree;
  std::vector<StrNode*> nodes;
  std::set<std::string> oracle;
  
  std::mt19937 gen(33333);
  
  auto random_string = [&gen]() {
    std::string s;
    int len = 5 + gen() % 20;
    for (int i = 0; i < len; ++i)
      s += 'a' + gen() % 26;
    return s;
  };
  
  // Insert phase
  for (int i = 0; i < 2000; ++i)
    {
      std::string key = random_string();
      auto * p = new StrNode(key);
      nodes.push_back(p);
      if (tree.insert(p) != nullptr)
        oracle.insert(key);
    }
  
  EXPECT_EQ(tree.size(), oracle.size());
  EXPECT_TRUE(tree.verify());
  
  // Verify
  for (const auto & key : oracle)
    ASSERT_NE(tree.search(key), nullptr);
  
  // Cleanup
  for (auto * p : nodes)
    delete p;
}

// Hybrid tree stress tests
TEST(HtdRbTreeCompat, Stress_LargeScaleOps)
{
  HybridTree tree;
  HybridNodePool pool;
  std::set<int> oracle;
  
  std::mt19937 gen(44444);
  std::uniform_int_distribution<> key_dist(0, 10000);
  std::uniform_int_distribution<> op_dist(0, 2);
  
  for (int iter = 0; iter < 10000; ++iter)
    {
      int key = key_dist(gen);
      int op = op_dist(gen);
      
      if (op == 0)  // insert
        {
          auto * p = pool.make(key);
          if (tree.insert(p) != nullptr)
            oracle.insert(key);
          else
            {
              pool.forget(p);
              delete p;
            }
        }
      else if (op == 1 && !oracle.empty())  // remove
        {
          auto it = oracle.begin();
          std::advance(it, gen() % oracle.size());
          int k = *it;
          
          auto * removed = tree.remove(k);
          ASSERT_NE(removed, nullptr);
          pool.forget(removed);
          delete removed;
          oracle.erase(k);
        }
      else  // search
        {
          auto * found = tree.search(key);
          if (oracle.count(key))
            ASSERT_NE(found, nullptr);
          else
            EXPECT_EQ(found, nullptr);
        }
      
      EXPECT_EQ(tree.size(), oracle.size());
    }
  
  assert_valid_hybrid_tree(tree);
}
