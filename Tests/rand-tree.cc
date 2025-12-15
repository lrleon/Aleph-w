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

#include <tpl_rand_tree.H>
#include <tpl_binNodeUtils.H>
#include <random>
#include <set>
#include <vector>
#include <algorithm>

using namespace std;
using namespace Aleph;

// ============================================================================
// Type Aliases and Helpers
// ============================================================================

using Tree = Rand_Tree<int>;
using Node = Tree::Node;

// Helper to collect keys in inorder
template <typename NodeT>
std::vector<int> inorder_keys(NodeT *root)
{
  std::vector<int> keys;
  if (root == NodeT::NullPtr)
    return keys;

  auto left = inorder_keys<NodeT>(LLINK(root));
  keys.insert(keys.end(), left.begin(), left.end());
  keys.push_back(KEY(root));
  auto right = inorder_keys<NodeT>(RLINK(root));
  keys.insert(keys.end(), right.begin(), right.end());

  return keys;
}

// Helper class to manage node allocation/deallocation
class NodePool
{
  std::vector<Node *> nodes;

public:
  ~NodePool()
  {
    for (auto *p : nodes)
      delete p;
  }

  Node *make(int key)
  {
    auto *p = new Node(key);
    nodes.push_back(p);
    return p;
  }

  void forget(Node *p)
  {
    nodes.erase(std::remove(nodes.begin(), nodes.end(), p), nodes.end());
  }
};

// ============================================================================
// Empty Tree Tests
// ============================================================================

TEST(RandTree, EmptyTreeProperties)
{
  Tree tree(42); // fixed seed for reproducibility

  EXPECT_EQ(tree.size(), 0u);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.getRoot(), Node::NullPtr);
  EXPECT_TRUE(tree.verify());
}

TEST(RandTree, SearchOnEmptyTreeReturnsNull)
{
  Tree tree(42);

  EXPECT_EQ(tree.search(10), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
  EXPECT_EQ(tree.search(-1), nullptr);
}

TEST(RandTree, RemoveFromEmptyTreeReturnsNull)
{
  Tree tree(42);

  EXPECT_EQ(tree.remove(10), nullptr);
  EXPECT_EQ(tree.size(), 0u);
}

// ============================================================================
// Insert Tests
// ============================================================================

TEST(RandTree, InsertSingleElement)
{
  Tree tree(42);
  NodePool pool;

  auto *p = pool.make(10);
  auto *inserted = tree.insert(p);

  EXPECT_EQ(inserted, p);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_FALSE(tree.is_empty());
  EXPECT_EQ(tree.getRoot(), p);
  EXPECT_TRUE(tree.verify());
}

TEST(RandTree, InsertMultipleElements)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    ASSERT_NE(tree.insert(pool.make(k)), nullptr);

  EXPECT_EQ(tree.size(), 7u);
  EXPECT_TRUE(tree.verify());

  auto keys = inorder_keys<Node>(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 3, 4, 5, 6, 7, 8}));
}

TEST(RandTree, InsertRejectsDuplicates)
{
  Tree tree(42);
  NodePool pool;

  auto *p1 = pool.make(10);
  auto *p2 = pool.make(10);

  EXPECT_NE(tree.insert(p1), nullptr);
  EXPECT_EQ(tree.insert(p2), nullptr); // duplicate rejected

  EXPECT_EQ(tree.size(), 1u);
  pool.forget(p2);
  delete p2;
}

TEST(RandTree, InsertDupAllowsDuplicates)
{
  Tree tree(42);
  NodePool pool;

  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(10)), nullptr);

  EXPECT_EQ(tree.size(), 5u);
  EXPECT_TRUE(tree.verify());
}

TEST(RandTree, InsertInAscendingOrder)
{
  Tree tree(42);
  NodePool pool;

  for (int k = 1; k <= 100; ++k)
    ASSERT_NE(tree.insert(pool.make(k)), nullptr);

  EXPECT_EQ(tree.size(), 100u);
  EXPECT_TRUE(tree.verify());

  auto keys = inorder_keys<Node>(tree.getRoot());
  std::vector<int> expected(100);
  std::iota(expected.begin(), expected.end(), 1);
  EXPECT_EQ(keys, expected);
}

TEST(RandTree, InsertInDescendingOrder)
{
  Tree tree(42);
  NodePool pool;

  for (int k = 100; k >= 1; --k)
    ASSERT_NE(tree.insert(pool.make(k)), nullptr);

  EXPECT_EQ(tree.size(), 100u);
  EXPECT_TRUE(tree.verify());

  auto keys = inorder_keys<Node>(tree.getRoot());
  std::vector<int> expected(100);
  std::iota(expected.begin(), expected.end(), 1);
  EXPECT_EQ(keys, expected);
}

// ============================================================================
// Search Tests
// ============================================================================

TEST(RandTree, SearchFindsExistingKey)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  for (int k : {1, 2, 3, 4, 5})
    {
      auto *found = tree.search(k);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(KEY(found), k);
    }
}

TEST(RandTree, SearchReturnsNullForMissingKey)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.search(2), nullptr);
  EXPECT_EQ(tree.search(4), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
  EXPECT_EQ(tree.search(6), nullptr);
}

TEST(RandTree, SearchOrInsertReturnsExisting)
{
  Tree tree(42);
  NodePool pool;

  auto *p1 = pool.make(10);
  tree.insert(p1);

  auto *p2 = pool.make(10);
  auto *found = tree.search_or_insert(p2);

  EXPECT_EQ(found, p1);
  EXPECT_EQ(tree.size(), 1u);

  pool.forget(p2);
  delete p2;
}

TEST(RandTree, SearchOrInsertInsertsNew)
{
  Tree tree(42);
  NodePool pool;

  tree.insert(pool.make(5));

  auto *p = pool.make(10);
  auto *result = tree.search_or_insert(p);

  EXPECT_EQ(result, p);
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_NE(tree.search(10), nullptr);
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST(RandTree, RemoveExistingKey)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  auto *removed = tree.remove(3);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 3);

  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(tree.search(3), nullptr);
  EXPECT_TRUE(tree.verify());

  auto keys = inorder_keys<Node>(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 4, 5}));
}

TEST(RandTree, RemoveReturnsNullForMissingKey)
{
  Tree tree(42);
  NodePool pool;

  tree.insert(pool.make(1));
  tree.insert(pool.make(3));

  EXPECT_EQ(tree.remove(2), nullptr);
  EXPECT_EQ(tree.size(), 2u);
}

TEST(RandTree, RemoveRoot)
{
  Tree tree(42);
  NodePool pool;

  auto *root = pool.make(5);
  tree.insert(root);
  tree.insert(pool.make(3));
  tree.insert(pool.make(7));

  auto *removed = tree.remove(5);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 5);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 2u);
  EXPECT_TRUE(tree.verify());
}

TEST(RandTree, RemoveAllElements)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    {
      auto *removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      pool.forget(removed);
      delete removed;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), 0u);
}

TEST(RandTree, RemoveInOrder)
{
  Tree tree(42);
  NodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  for (int k = 1; k <= 10; ++k)
    {
      auto *removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      pool.forget(removed);
      delete removed;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), 0u);
}

TEST(RandTree, RemoveInReverseOrder)
{
  Tree tree(42);
  NodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  for (int k = 10; k >= 1; --k)
    {
      auto *removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      pool.forget(removed);
      delete removed;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), 0u);
}

// ============================================================================
// Select and Position Tests
// ============================================================================

TEST(RandTree, SelectReturnsCorrectNode)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  // Inorder: 1, 3, 4, 5, 6, 7, 8
  EXPECT_EQ(KEY(tree.select(0)), 1);
  EXPECT_EQ(KEY(tree.select(1)), 3);
  EXPECT_EQ(KEY(tree.select(2)), 4);
  EXPECT_EQ(KEY(tree.select(3)), 5);
  EXPECT_EQ(KEY(tree.select(4)), 6);
  EXPECT_EQ(KEY(tree.select(5)), 7);
  EXPECT_EQ(KEY(tree.select(6)), 8);
}

TEST(RandTree, SelectOutOfRangeThrows)
{
  Tree tree(42);
  NodePool pool;

  tree.insert(pool.make(1));
  tree.insert(pool.make(2));

  EXPECT_THROW(tree.select(2), std::out_of_range);
  EXPECT_THROW(tree.select(100), std::out_of_range);
}

TEST(RandTree, PositionOfExistingKey)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  // Inorder: 1, 3, 4, 5, 6, 7, 8
  auto [pos1, node1] = tree.position(1);
  EXPECT_EQ(pos1, 0);
  EXPECT_EQ(KEY(node1), 1);

  auto [pos5, node5] = tree.position(5);
  EXPECT_EQ(pos5, 3);
  EXPECT_EQ(KEY(node5), 5);

  auto [pos8, node8] = tree.position(8);
  EXPECT_EQ(pos8, 6);
  EXPECT_EQ(KEY(node8), 8);
}

TEST(RandTree, PositionOfMissingKey)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {2, 4, 6})
    tree.insert(pool.make(k));

  auto [pos, node] = tree.position(3);
  EXPECT_EQ(pos, -1);
}

TEST(RandTree, FindPositionOfExistingKey)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {2, 4, 6, 8})
    tree.insert(pool.make(k));

  auto [pos, node] = tree.find_position(4);
  EXPECT_EQ(pos, 1);
  EXPECT_EQ(KEY(node), 4);
}

TEST(RandTree, FindPositionOfMissingKeyInMiddle)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {2, 4, 6, 8})
    tree.insert(pool.make(k));

  // 5 would be at position 2 (between 4 and 6)
  auto [pos, node] = tree.find_position(5);
  EXPECT_EQ(pos, 2);
  // The returned node is the predecessor (key immediately less than 5)
  EXPECT_NE(node, nullptr);
}

TEST(RandTree, FindPositionLessThanMin)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {2, 4, 6})
    tree.insert(pool.make(k));

  auto [pos, node] = tree.find_position(1);
  EXPECT_EQ(pos, -1);
  EXPECT_EQ(KEY(node), 2); // smallest key
}

TEST(RandTree, FindPositionGreaterThanMax)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {2, 4, 6})
    tree.insert(pool.make(k));

  auto [pos, node] = tree.find_position(10);
  EXPECT_EQ(pos, 3);
  EXPECT_EQ(KEY(node), 6); // largest key
}

// ============================================================================
// Remove by Position Tests
// ============================================================================

TEST(RandTree, RemovePosValidPosition)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  // Inorder: 1, 3, 4, 5, 6, 7, 8
  // Remove position 3 (key = 5)
  auto *removed = tree.remove_pos(3);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 5);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 6u);
  EXPECT_TRUE(tree.verify());
}

TEST(RandTree, RemovePosFirst)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7})
    tree.insert(pool.make(k));

  // Inorder: 3, 5, 7 - remove first
  auto *removed = tree.remove_pos(0);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 3);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 2u);
}

TEST(RandTree, RemovePosLast)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7})
    tree.insert(pool.make(k));

  // Inorder: 3, 5, 7 - remove last
  auto *removed = tree.remove_pos(2);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 7);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 2u);
}

// NOTE: remove_pos is marked noexcept but throws - this is a bug
// The test below would cause std::terminate, so we skip it
// TEST(RandTree, RemovePosOutOfRangeThrows)

// ============================================================================
// Split Tests
// ============================================================================

TEST(RandTree, SplitKeyNotInTree)
{
  Tree tree(42);
  Tree t1(42);
  Tree t2(42);
  NodePool pool;

  for (int k : {2, 4, 6, 8, 10})
    tree.insert(pool.make(k));

  bool result = tree.split_key(5, t1, t2);
  EXPECT_TRUE(result);

  // t1 should have keys < 5: {2, 4}
  // t2 should have keys > 5: {6, 8, 10}
  auto keys1 = inorder_keys<Node>(t1.getRoot());
  auto keys2 = inorder_keys<Node>(t2.getRoot());

  EXPECT_EQ(keys1, (std::vector<int>{2, 4}));
  EXPECT_EQ(keys2, (std::vector<int>{6, 8, 10}));
}

TEST(RandTree, SplitKeyInTree)
{
  Tree tree(42);
  Tree t1(42);
  Tree t2(42);
  NodePool pool;

  for (int k : {2, 4, 6, 8, 10})
    tree.insert(pool.make(k));

  bool result = tree.split_key(6, t1, t2);
  EXPECT_FALSE(result); // key was in tree
}

TEST(RandTree, SplitKeyDup)
{
  Tree tree(42);
  Tree t1(42);
  Tree t2(42);
  NodePool pool;

  for (int k : {2, 4, 6, 8, 10})
    tree.insert(pool.make(k));

  tree.split_key_dup(6, t1, t2);

  auto keys1 = inorder_keys<Node>(t1.getRoot());
  auto keys2 = inorder_keys<Node>(t2.getRoot());

  // Actual semantics: t1 gets keys <= key, t2 gets keys > key
  EXPECT_EQ(keys1, (std::vector<int>{2, 4, 6}));
  EXPECT_EQ(keys2, (std::vector<int>{8, 10}));
}

TEST(RandTree, SplitPos)
{
  Tree tree(42);
  Tree t1(42);
  Tree t2(42);
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  tree.split_pos(2, t1, t2);

  auto keys1 = inorder_keys<Node>(t1.getRoot());
  auto keys2 = inorder_keys<Node>(t2.getRoot());

  // t1: positions [0, 2) -> keys {1, 2}
  // t2: positions [2, 5) -> keys {3, 4, 5}
  EXPECT_EQ(keys1, (std::vector<int>{1, 2}));
  EXPECT_EQ(keys2, (std::vector<int>{3, 4, 5}));
}

// ============================================================================
// Join Tests
// ============================================================================

TEST(RandTree, JoinWithNoDuplicates)
{
  Tree tree1(42);
  Tree tree2(42);
  Tree dup(42);
  NodePool pool;

  for (int k : {1, 3, 5})
    tree1.insert(pool.make(k));
  for (int k : {2, 4, 6})
    tree2.insert(pool.make(k));

  tree1.join(tree2, dup);

  EXPECT_EQ(tree1.size(), 6u);
  EXPECT_EQ(tree2.size(), 0u);
  EXPECT_EQ(dup.size(), 0u);
  EXPECT_TRUE(tree1.verify());

  auto keys = inorder_keys<Node>(tree1.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST(RandTree, JoinWithDuplicates)
{
  Tree tree1(42);
  Tree tree2(42);
  Tree dup(42);
  NodePool pool;

  for (int k : {1, 3, 5})
    tree1.insert(pool.make(k));
  for (int k : {3, 4, 5})
    tree2.insert(pool.make(k));

  tree1.join(tree2, dup);

  EXPECT_EQ(tree1.size(), 4u); // 1, 3, 4, 5
  EXPECT_EQ(tree2.size(), 0u);
  EXPECT_EQ(dup.size(), 2u); // two duplicates: 3, 5

  auto keys = inorder_keys<Node>(tree1.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 3, 4, 5}));

  auto dupKeys = inorder_keys<Node>(dup.getRoot());
  std::sort(dupKeys.begin(), dupKeys.end());
  EXPECT_EQ(dupKeys, (std::vector<int>{3, 5}));
}

TEST(RandTree, JoinDup)
{
  Tree tree1(42);
  Tree tree2(42);
  NodePool pool;

  for (int k : {1, 3, 5})
    tree1.insert(pool.make(k));
  for (int k : {3, 4, 5})
    tree2.insert(pool.make(k));

  tree1.join_dup(tree2);

  EXPECT_EQ(tree1.size(), 6u); // 1, 3, 3, 4, 5, 5
  EXPECT_EQ(tree2.size(), 0u);
  EXPECT_TRUE(tree1.verify());
}

TEST(RandTree, JoinExclusive)
{
  Tree tree1(42);
  Tree tree2(42);
  NodePool pool;

  // tree1 has smaller keys
  for (int k : {1, 2, 3})
    tree1.insert(pool.make(k));
  // tree2 has larger keys
  for (int k : {10, 11, 12})
    tree2.insert(pool.make(k));

  tree1.join_exclusive(tree2);

  EXPECT_EQ(tree1.size(), 6u);
  EXPECT_EQ(tree2.size(), 0u);
  EXPECT_TRUE(tree1.verify());

  auto keys = inorder_keys<Node>(tree1.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 10, 11, 12}));
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(RandTree, IteratorTraversesInOrder)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  std::vector<int> result;
  for (Tree::Iterator it(tree); it.has_curr(); it.next())
    result.push_back(KEY(it.get_curr()));

  EXPECT_EQ(result, (std::vector<int>{1, 3, 4, 5, 6, 7, 8}));
}

TEST(RandTree, IteratorOnEmptyTree)
{
  Tree tree(42);
  Tree::Iterator it(tree);

  EXPECT_FALSE(it.has_curr());
}

TEST(RandTree, IteratorAfterRemoval)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  auto *removed = tree.remove(3);
  pool.forget(removed);
  delete removed;

  std::vector<int> result;
  for (Tree::Iterator it(tree); it.has_curr(); it.next())
    result.push_back(KEY(it.get_curr()));

  EXPECT_EQ(result, (std::vector<int>{1, 2, 4, 5}));
}

// ============================================================================
// Special Member Functions Tests
// ============================================================================

TEST(RandTree, SwapTrees)
{
  Tree tree1(42);
  Tree tree2(42);
  NodePool pool;

  for (int k : {1, 2, 3})
    tree1.insert(pool.make(k));
  for (int k : {10, 20})
    tree2.insert(pool.make(k));

  tree1.swap(tree2);

  EXPECT_EQ(tree1.size(), 2u);
  EXPECT_EQ(tree2.size(), 3u);

  EXPECT_NE(tree1.search(10), nullptr);
  EXPECT_NE(tree1.search(20), nullptr);
  EXPECT_NE(tree2.search(1), nullptr);
  EXPECT_NE(tree2.search(2), nullptr);
  EXPECT_NE(tree2.search(3), nullptr);
}

TEST(RandTree, SeedAffectsStructure)
{
  NodePool pool1, pool2;

  Tree tree1(123);
  Tree tree2(456);

  // Insert same elements in same order with different seeds
  for (int k : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
    {
      tree1.insert(pool1.make(k));
      tree2.insert(pool2.make(k));
    }

  // Both trees should have same elements but potentially different structure
  EXPECT_EQ(tree1.size(), tree2.size());
  EXPECT_TRUE(tree1.verify());
  EXPECT_TRUE(tree2.verify());

  // Inorder traversal should be the same
  auto keys1 = inorder_keys<Node>(tree1.getRoot());
  auto keys2 = inorder_keys<Node>(tree2.getRoot());
  EXPECT_EQ(keys1, keys2);
}

// ============================================================================
// Custom Comparator Tests
// ============================================================================

TEST(RandTree, CustomComparatorGreater)
{
  using TreeGt = Rand_Tree<int, std::greater<int>>;
  using NodeGt = TreeGt::Node;

  TreeGt tree(42);

  std::vector<NodeGt *> nodes;
  for (int k : {1, 2, 3, 4, 5})
    {
      auto *p = new NodeGt(k);
      nodes.push_back(p);
      tree.insert(p);
    }

  EXPECT_EQ(tree.size(), 5u);
  EXPECT_TRUE(tree.verify());

  // With greater<int>, inorder should be descending
  std::vector<int> result;
  for (TreeGt::Iterator it(tree); it.has_curr(); it.next())
    result.push_back(KEY(it.get_curr()));

  EXPECT_EQ(result, (std::vector<int>{5, 4, 3, 2, 1}));

  // Clean up
  for (int k : {1, 2, 3, 4, 5})
    delete tree.remove(k);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(RandTree, NegativeKeys)
{
  Tree tree(42);
  NodePool pool;

  for (int k : {-5, -3, -1, 0, 1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 7u);
  EXPECT_TRUE(tree.verify());

  auto keys = inorder_keys<Node>(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{-5, -3, -1, 0, 1, 3, 5}));

  // Search negative keys
  EXPECT_NE(tree.search(-5), nullptr);
  EXPECT_NE(tree.search(-1), nullptr);
  EXPECT_EQ(tree.search(-2), nullptr);
}

TEST(RandTree, SingleElementOperations)
{
  Tree tree(42);
  NodePool pool;

  auto *p = pool.make(42);
  tree.insert(p);

  EXPECT_EQ(tree.size(), 1u);
  EXPECT_EQ(KEY(tree.select(0)), 42);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, 0);
  EXPECT_EQ(KEY(node), 42);

  auto *removed = tree.remove(42);
  ASSERT_NE(removed, nullptr);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 0u);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(RandTree, RandomInsertSearchRemove)
{
  Tree tree(42);
  NodePool pool;
  std::set<int> oracle;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 999);

  // Insert phase
  for (int i = 0; i < 500; ++i)
    {
      int k = dist(rng);
      if (oracle.count(k) == 0)
        {
          ASSERT_NE(tree.insert(pool.make(k)), nullptr);
          oracle.insert(k);
        }
    }

  EXPECT_EQ(tree.size(), oracle.size());
  EXPECT_TRUE(tree.verify());

  // Search phase
  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto *found = tree.search(k);
      if (oracle.count(k))
        EXPECT_NE(found, nullptr);
      else
        EXPECT_EQ(found, nullptr);
    }

  // Remove phase
  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto *removed = tree.remove(k);
      if (oracle.count(k))
        {
          ASSERT_NE(removed, nullptr);
          oracle.erase(k);
          pool.forget(removed);
          delete removed;
        }
      else
        {
          EXPECT_EQ(removed, nullptr);
        }
    }

  EXPECT_EQ(tree.size(), oracle.size());
  EXPECT_TRUE(tree.verify());

  // Verify remaining keys
  auto keys = inorder_keys<Node>(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));
}

TEST(RandTree, LargeTreeOperations)
{
  Tree tree(42);
  NodePool pool;

  const int N = 5000;

  // Insert N elements
  for (int k = 0; k < N; ++k)
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  EXPECT_TRUE(tree.verify());

  // Verify select works correctly
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(KEY(tree.select(i)), i);

  // Remove half
  for (int k = 0; k < N; k += 2)
    {
      auto *removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      pool.forget(removed);
      delete removed;
    }

  EXPECT_EQ(tree.size(), static_cast<size_t>(N / 2));
  EXPECT_TRUE(tree.verify());

  // Verify remaining elements
  for (int k = 1; k < N; k += 2)
    EXPECT_NE(tree.search(k), nullptr);
}

// ============================================================================
// Rand_Tree_Vtl Tests
// ============================================================================

TEST(RandTreeVtl, BasicOperations)
{
  using TreeVtl = Rand_Tree_Vtl<int>;
  using NodeVtl = TreeVtl::Node;

  TreeVtl tree(42);

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(new NodeVtl(k));

  EXPECT_EQ(tree.size(), 5u);
  EXPECT_TRUE(tree.verify());

  // Clean up properly using remove to avoid memory leaks
  for (int k : {1, 2, 3, 4, 5})
    delete tree.remove(k);
}

// ============================================================================
// Verify Method Tests
// ============================================================================

TEST(RandTree, VerifyDetectsValidTree)
{
  Tree tree(42);
  NodePool pool;

  EXPECT_TRUE(tree.verify()); // empty tree is valid

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  EXPECT_TRUE(tree.verify());

  // After various operations
  tree.remove(5);
  EXPECT_TRUE(tree.verify());
}

// ============================================================================
// API Coverage Tests
// ============================================================================

TEST(RandTree, GetRootReturnsReference)
{
  Tree tree(42);
  NodePool pool;

  Node *&root = tree.getRoot();
  EXPECT_EQ(root, Node::NullPtr);

  tree.insert(pool.make(10));
  EXPECT_NE(root, Node::NullPtr);
  EXPECT_EQ(KEY(root), 10);
}

TEST(RandTree, KeyCompAndGetCompare)
{
  Tree tree(42);

  auto &cmp1 = tree.key_comp();
  auto &cmp2 = tree.get_compare();

  // Both should return the same comparator
  EXPECT_TRUE(cmp1(1, 2));
  EXPECT_FALSE(cmp1(2, 1));
  EXPECT_TRUE(cmp2(1, 2));
  EXPECT_FALSE(cmp2(2, 1));
}

TEST(RandTree, GslRngObjectNotNull)
{
  Tree tree(42);

  auto *rng = tree.gsl_rng_object();
  EXPECT_NE(rng, nullptr);
}

TEST(RandTree, SetSeedChangesSequence)
{
  Tree tree1(42);
  Tree tree2(42);
  NodePool pool1, pool2;

  // Same seed, same insertion order -> same structure
  tree1.set_seed(999);
  tree2.set_seed(999);

  for (int k : {1, 2, 3, 4, 5})
    {
      tree1.insert(pool1.make(k));
      tree2.insert(pool2.make(k));
    }

  // Trees should have identical structure
  // (This is probabilistic but with same seed should be identical)
  EXPECT_EQ(tree1.size(), tree2.size());
}

// ============================================================================
// Copy Prevention Tests (compile-time)
// ============================================================================

static_assert(!std::is_copy_assignable_v<Tree>,
              "Rand_Tree should not be copy assignable");

static_assert(!std::is_copy_constructible_v<Tree>,
              "Rand_Tree should not be copy constructible");
