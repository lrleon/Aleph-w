
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file splay-tree-rk.cc
 * @brief Tests for Splay Tree Rk
 */

#include <algorithm>
#include <numeric>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_splay_treeRk.H>

using namespace Aleph;
using namespace testing;

namespace
{
  using Tree = Splay_Tree_Rk<int>;
  using Node = Tree::Node;

  struct AbsLess
  {
    bool operator()(int a, int b) const
    {
      return std::abs(a) < std::abs(b);
    }
  };

  struct NodePool
  {
    std::vector<Node *> allocated;

    Node * make(int k)
    {
      auto * p = new Node(k);
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
      for (auto * p : allocated)
        delete p;
    }
  };

  void delete_tree(Node * root) noexcept
  {
    if (root == Node::NullPtr)
      return;
    delete_tree(LLINK(root));
    delete_tree(RLINK(root));
    delete root;
  }

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

  void assert_valid_tree(Tree & tree)
  {
    ASSERT_TRUE(tree.verify()) << "Rank tree invariant violated";
    auto * root = tree.getRoot();
    if (root != Node::NullPtr)
      ASSERT_EQ(COUNT(root), tree.size());
  }
}

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST(SplayTreeRk, EmptyTreeProperties)
{
  Tree tree;

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.getRoot(), Node::NullPtr);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST(SplayTreeRk, InsertSingleElement)
{
  Tree tree;
  NodePool pool;

  auto * p = pool.make(42);
  auto * inserted = tree.insert(p);

  EXPECT_EQ(inserted, p);
  EXPECT_FALSE(tree.is_empty());
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_EQ(tree.getRoot(), p);
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, InsertMultipleElements)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    {
      auto * p = pool.make(k);
      ASSERT_NE(tree.insert(p), nullptr);
    }

  EXPECT_EQ(tree.size(), 7u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 3, 4, 5, 6, 7, 8}));
}

TEST(SplayTreeRk, InsertRejectsDuplicates)
{
  Tree tree;
  NodePool pool;

  auto * p1 = pool.make(10);
  EXPECT_NE(tree.insert(p1), nullptr);

  auto * p2 = pool.make(10);
  EXPECT_EQ(tree.insert(p2), nullptr);

  EXPECT_EQ(tree.size(), 1u);
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, InsertDupAllowsDuplicates)
{
  Tree tree;
  NodePool pool;

  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(42)), nullptr);

  EXPECT_EQ(tree.size(), 5u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{42, 42, 42, 42, 42}));
}

TEST(SplayTreeRk, SearchFindsExistingKey)
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

TEST(SplayTreeRk, SearchReturnsNullForMissingKey)
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

TEST(SplayTreeRk, SearchOrInsertBehavior)
{
  Tree tree;
  NodePool pool;

  // Insert via search_or_insert
  auto * p1 = pool.make(10);
  auto * ret1 = tree.search_or_insert(p1);
  EXPECT_EQ(ret1, p1);
  EXPECT_EQ(tree.size(), 1u);

  // Search existing via search_or_insert
  auto * p2 = pool.make(10);
  auto * ret2 = tree.search_or_insert(p2);
  EXPECT_NE(ret2, p2);
  EXPECT_EQ(KEY(ret2), 10);
  EXPECT_EQ(tree.size(), 1u);

  assert_valid_tree(tree);
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST(SplayTreeRk, RemoveExistingKey)
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

  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(tree.search(3), nullptr);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 4, 5}));
}

TEST(SplayTreeRk, RemoveReturnsNullForMissingKey)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.remove(2), nullptr);
  EXPECT_EQ(tree.remove(4), nullptr);
  EXPECT_EQ(tree.size(), 3u);

  assert_valid_tree(tree);
}

TEST(SplayTreeRk, RemoveFromEmptyTree)
{
  Tree tree;

  EXPECT_EQ(tree.remove(42), nullptr);
  EXPECT_TRUE(tree.is_empty());
}

TEST(SplayTreeRk, RemoveRootWithNoLeftChild)
{
  Tree tree;
  NodePool pool;

  // Insert in order to create a tree where root has no left child
  tree.insert(pool.make(1));
  tree.insert(pool.make(2));
  tree.insert(pool.make(3));

  // After splay for 1, root should be 1 which has no left child
  auto * removed = tree.remove(1);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 1);
  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.size(), 2u);
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, RemoveAllElements)
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

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
}

// ============================================================================
// Rank Operations Tests (select, position)
// ============================================================================

TEST(SplayTreeRk, SelectByPosition)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  // Expected order: 1, 3, 4, 5, 6, 7, 8
  std::vector<int> expected = {1, 3, 4, 5, 6, 7, 8};

  for (size_t i = 0; i < expected.size(); ++i)
    {
      auto * node = tree.select(i);
      ASSERT_NE(node, Node::NullPtr) << "select(" << i << ") returned null";
      EXPECT_EQ(KEY(node), expected[i]) << "select(" << i << ") wrong key";
    }
}

TEST(SplayTreeRk, SelectOutOfRangeThrows)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 2, 3})
    tree.insert(pool.make(k));

  EXPECT_THROW(tree.select(3), std::out_of_range);
  EXPECT_THROW(tree.select(100), std::out_of_range);
}

TEST(SplayTreeRk, PositionFindsCorrectRank)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  // Expected order: 1, 3, 4, 5, 6, 7, 8 (positions 0-6)
  std::vector<std::pair<int, long>> expected = {
    {1, 0}, {3, 1}, {4, 2}, {5, 3}, {6, 4}, {7, 5}, {8, 6}
  };

  for (auto [key, pos] : expected)
    {
      auto result = tree.position(key);
      EXPECT_EQ(result.first, pos) << "position(" << key << ") wrong";
      ASSERT_NE(result.second, nullptr);
      EXPECT_EQ(KEY(result.second), key);
    }
}

TEST(SplayTreeRk, PositionReturnsMinusOneForMissingKey)
{
  Tree tree;
  NodePool pool;

  for (int k : {2, 4, 6})
    tree.insert(pool.make(k));

  auto result = tree.position(3);
  EXPECT_EQ(result.first, -1);
  // Note: result.second may be uninitialized in current implementation
}

TEST(SplayTreeRk, PositionOnEmptyTree)
{
  Tree tree;

  // This should handle empty tree gracefully
  // Current implementation may crash - this tests the fix
  auto result = tree.position(42);
  EXPECT_EQ(result.first, -1);
}

// ============================================================================
// Splay Operation Tests
// ============================================================================

TEST(SplayTreeRk, SplayBringsNodeToRoot)
{
  Tree tree;
  NodePool pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  // Search for 1 should bring it to root
  tree.search(1);
  EXPECT_EQ(KEY(tree.getRoot()), 1);
  assert_valid_tree(tree);

  // Search for 5 should bring it to root
  tree.search(5);
  EXPECT_EQ(KEY(tree.getRoot()), 5);
  assert_valid_tree(tree);

  // Search for 3 should bring it to root
  tree.search(3);
  EXPECT_EQ(KEY(tree.getRoot()), 3);
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, CountsMaintainedAfterSplay)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  size_t original_size = tree.size();

  // Multiple searches should maintain counts
  for (int k : {1, 8, 4, 6, 3, 7, 5})
    {
      tree.search(k);
      EXPECT_EQ(tree.size(), original_size);
      assert_valid_tree(tree);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(SplayTreeRk, SingleElementOperations)
{
  Tree tree;
  NodePool pool;

  auto * p = pool.make(42);
  tree.insert(p);

  EXPECT_EQ(tree.select(0), p);
  EXPECT_EQ(tree.position(42).first, 0);
  EXPECT_EQ(tree.search(42), p);

  auto * removed = tree.remove(42);
  EXPECT_EQ(removed, p);
  EXPECT_TRUE(tree.is_empty());
  pool.forget(removed);
  delete removed;
}

TEST(SplayTreeRk, InsertInDescendingOrder)
{
  Tree tree;
  NodePool pool;

  for (int k = 10; k >= 1; --k)
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 10u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

TEST(SplayTreeRk, InsertInAscendingOrder)
{
  Tree tree;
  NodePool pool;

  for (int k = 1; k <= 10; ++k)
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.size(), 10u);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

// ============================================================================
// Custom Comparator Tests
// ============================================================================

TEST(SplayTreeRk, CustomComparatorGreater)
{
  using TreeGt = GenTdSplayTreeRk<BinNodeXt, int, std::greater<int>>;
  using NodeGt = TreeGt::Node;

  TreeGt tree;
  std::vector<NodeGt *> nodes;

  for (int k : {1, 2, 3, 4, 5})
    {
      auto * p = new NodeGt(k);
      nodes.push_back(p);
      tree.insert(p);
    }

  EXPECT_EQ(tree.size(), 5u);
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

TEST(SplayTreeRk, StatefulComparatorAffectsEquality)
{
  using TreeAbs = Splay_Tree_Rk<int, AbsLess>;
  using NodeAbs = TreeAbs::Node;

  TreeAbs tree(AbsLess{});
  auto * p = new NodeAbs(1);
  tree.insert(p);

  auto * found = tree.search(-1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, p);
  EXPECT_TRUE(tree.verify());

  delete tree.remove(1);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(SplayTreeRk, RandomInsertSearchRemove)
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

      ASSERT_EQ(tree.size(), oracle.size());
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

      ASSERT_EQ(tree.size(), oracle.size());
      assert_valid_tree(tree);
    }

  // Final verification
  keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, std::vector<int>(oracle.begin(), oracle.end()));
}

TEST(SplayTreeRk, SelectAndPositionConsistency)
{
  Tree tree;
  NodePool pool;

  std::mt19937 rng(123);
  std::uniform_int_distribution<int> dist(0, 1000);

  std::set<int> oracle;
  for (int i = 0; i < 100; ++i)
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
    }

  // For each position, select should return correct element
  std::vector<int> sorted(oracle.begin(), oracle.end());
  for (size_t i = 0; i < sorted.size(); ++i)
    {
      auto * node = tree.select(i);
      ASSERT_NE(node, Node::NullPtr);
      EXPECT_EQ(KEY(node), sorted[i]);

      // Position of that key should be i
      auto pos_result = tree.position(sorted[i]);
      EXPECT_EQ(pos_result.first, static_cast<long>(i));
    }
}

// ============================================================================
// Verify Method Tests
// ============================================================================

TEST(SplayTreeRk, VerifyDetectsValidTree)
{
  Tree tree;
  NodePool pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  EXPECT_TRUE(tree.verify());
}

// ============================================================================
// Virtual Destructor Variant Tests
// ============================================================================

// Note: Splay_Tree_Rk_Vtl test is disabled because BinNodeXtVtl has a private
// sentinel constructor that prevents the splay() method from working correctly.
// This is a design issue in the template that should be addressed separately.

// ============================================================================
// Swap Test
// ============================================================================

TEST(SplayTreeRk, SwapTrees)
{
  Tree tree1, tree2;
  NodePool pool;

  for (int k : {1, 2, 3})
    tree1.insert(pool.make(k));

  for (int k : {10, 20})
    tree2.insert(pool.make(k));

  EXPECT_EQ(tree1.size(), 3u);
  EXPECT_EQ(tree2.size(), 2u);

  tree1.swap(tree2);

  EXPECT_EQ(tree1.size(), 2u);
  EXPECT_EQ(tree2.size(), 3u);

  EXPECT_NE(tree1.search(10), nullptr);
  EXPECT_NE(tree2.search(1), nullptr);
}

// ============================================================================
// Additional Stress and Fuzz Tests
// ============================================================================

TEST(SplayTreeRk, Stress_AscendingInsertion)
{
  Tree tree;
  NodePool pool;
  
  const int N = 5000;
  for (int k = 0; k < N; ++k)
    {
      tree.insert(pool.make(k));
      if (k % 500 == 0)
        assert_valid_tree(tree);
    }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
  
  // Verify select and position
  for (int i = 0; i < 100; ++i)
    {
      int pos = rand() % N;
      auto * node = tree.select(pos);
      ASSERT_NE(node, Node::NullPtr);
      EXPECT_EQ(KEY(node), pos);
    }
}

TEST(SplayTreeRk, Stress_DescendingInsertion)
{
  Tree tree;
  NodePool pool;
  
  const int N = 5000;
  for (int k = N - 1; k >= 0; --k)
    tree.insert(pool.make(k));
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, Stress_ZigzagInsertion)
{
  Tree tree;
  NodePool pool;
  
  const int N = 3000;
  for (int i = 0; i < N; ++i)
    {
      int k = (i % 2 == 0) ? i / 2 : N - 1 - i / 2;
      tree.insert(pool.make(k));
    }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, Fuzz_LargeScaleRandomOps)
{
  Tree tree;
  NodePool pool;
  std::set<int> oracle;
  
  std::mt19937 gen(98765);
  std::uniform_int_distribution<> key_dist(0, 20000);
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
}

TEST(SplayTreeRk, Stress_BulkInsertBulkRemove)
{
  Tree tree;
  NodePool pool;
  
  const int N = 5000;
  
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

TEST(SplayTreeRk, Stress_ManyDuplicates)
{
  Tree tree;
  NodePool pool;
  
  const int N = 100;
  const int DUPS = 5;
  
  for (int k = 0; k < N; ++k)
    for (int d = 0; d < DUPS; ++d)
      tree.insert_dup(pool.make(k));
  
  size_t initial_size = tree.size();
  EXPECT_GT(initial_size, 0u);
  assert_valid_tree(tree);
  
  // Remove all elements until tree is empty
  size_t total_removed = 0;
  while (!tree.is_empty())
    {
      // Get any key from the tree via select
      auto * node = tree.select(0);
      ASSERT_NE(node, Node::NullPtr);
      int k = KEY(node);
      
      auto * removed = tree.remove(k);
      ASSERT_NE(removed, nullptr);
      pool.forget(removed);
      delete removed;
      ++total_removed;
      
      // Validate tree structure periodically
      if (total_removed % 50 == 0)
        assert_valid_tree(tree);
    }
  
  EXPECT_EQ(total_removed, initial_size);
  EXPECT_TRUE(tree.is_empty());
}

TEST(SplayTreeRk, Stress_RankOperationsUnderLoad)
{
  Tree tree;
  NodePool pool;
  std::set<int> oracle;
  
  std::mt19937 gen(22222);
  std::uniform_int_distribution<> key_dist(0, 5000);
  
  // Insert random keys
  for (int i = 0; i < 2000; ++i)
    {
      int key = key_dist(gen);
      auto * p = pool.make(key);
      if (tree.insert(p) != nullptr)
        oracle.insert(key);
      else
        {
          pool.forget(p);
          delete p;
        }
    }
  
  std::vector<int> sorted(oracle.begin(), oracle.end());
  
  // Test select and position consistency
  for (size_t i = 0; i < sorted.size(); ++i)
    {
      auto * node = tree.select(i);
      ASSERT_NE(node, Node::NullPtr);
      EXPECT_EQ(KEY(node), sorted[i]);
      
      auto pos_result = tree.position(sorted[i]);
      EXPECT_EQ(pos_result.first, static_cast<long>(i));
    }
  
  assert_valid_tree(tree);
}

TEST(SplayTreeRk, Stress_FrequentAccessPattern)
{
  Tree tree;
  NodePool pool;
  
  const int N = 1000;
  for (int k = 0; k < N; ++k)
    tree.insert(pool.make(k));
  
  std::mt19937 gen(33333);
  std::uniform_int_distribution<> dist(0, N - 1);
  
  // Simulate access pattern with some keys accessed frequently
  int hot_key = 500;
  for (int i = 0; i < 5000; ++i)
    {
      int key = (i % 3 == 0) ? hot_key : dist(gen);
      auto * found = tree.search(key);
      ASSERT_NE(found, nullptr);
      EXPECT_EQ(KEY(found), key);
    }
  
  // Hot key should be near root due to splay property
  assert_valid_tree(tree);
}