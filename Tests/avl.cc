
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
 * @file avl.cc
 * @brief Tests for Avl
 */

#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_avl.H>

using namespace Aleph;
using namespace testing;

namespace
{
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

  template <class Node>
  std::vector<typename Node::key_type> inorder_keys(Node * root)
  {
    std::vector<typename Node::key_type> keys;
    Aleph::infix_for_each<Node>(root, [&] (Node * p) { keys.push_back(KEY(p)); });
    return keys;
  }
}

TEST(AvlTree, InsertSearchVerifyAndIteratorOrder)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  const std::vector<int> input{5, 3, 7, 2, 4, 6, 8};
  for (int k : input)
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_TRUE(t.verify());
  EXPECT_TRUE(is_avl(t.getRoot()));

  auto * found = t.search(4);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(KEY(found), 4);

  std::vector<int> it_keys;
  for (Avl_Tree<int>::Iterator it(t); it.has_curr(); it.next_ne())
    it_keys.push_back(KEY(it.get_curr_ne()));

  std::vector<int> expected = input;
  std::sort(expected.begin(), expected.end());
  EXPECT_EQ(it_keys, expected);
}

TEST(AvlTree, InsertRejectsDuplicatesAndSearchOrInsertReturnsExisting)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  auto * a = pool.make(5);
  auto * b = pool.make(5);

  ASSERT_NE(t.insert(a), nullptr);
  EXPECT_EQ(t.insert(b), nullptr);

  auto * c = pool.make(5);
  auto * got = t.search_or_insert(c);
  ASSERT_NE(got, nullptr);
  EXPECT_NE(got, c);
  EXPECT_EQ(KEY(got), 5);

  EXPECT_TRUE(t.verify());
}

// Tests for optimistic search edge cases
TEST(AvlTree, DuplicateAtIntermediateLevel)
{
  /*
   * Build tree:       50
   *                  /  \
   *                25    75
   *               /  \
   *             10    30
   */
  // Then try to insert duplicate of 25 (intermediate level)
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  ASSERT_NE(t.insert(pool.make(50)), nullptr);
  ASSERT_NE(t.insert(pool.make(25)), nullptr);
  ASSERT_NE(t.insert(pool.make(75)), nullptr);
  ASSERT_NE(t.insert(pool.make(10)), nullptr);
  ASSERT_NE(t.insert(pool.make(30)), nullptr);

  // Try duplicate at intermediate level
  EXPECT_EQ(t.insert(pool.make(25)), nullptr);
  EXPECT_TRUE(t.verify());

  // search_or_insert should return existing node
  auto * dup = pool.make(25);
  auto * found = t.search_or_insert(dup);
  EXPECT_NE(found, dup);
  EXPECT_EQ(KEY(found), 25);
  EXPECT_TRUE(t.verify());

  // Remove the intermediate node should work
  auto * removed = t.remove(25);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 25);
  EXPECT_TRUE(t.verify());
}

TEST(AvlTree, DuplicateAfterOnlyLeftDescents)
{
  // Build tree where duplicate search goes only left
  // Tree:     100
  //          /
  //        50
  //       /
  //     25
  // Search for 25 (only left descents, candidate should be nullptr until found)
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  ASSERT_NE(t.insert(pool.make(100)), nullptr);
  ASSERT_NE(t.insert(pool.make(50)), nullptr);
  ASSERT_NE(t.insert(pool.make(25)), nullptr);

  // Insert key smaller than all - should work (no duplicate)
  ASSERT_NE(t.insert(pool.make(10)), nullptr);
  EXPECT_TRUE(t.verify());

  // Try duplicate of leftmost
  EXPECT_EQ(t.insert(pool.make(25)), nullptr);
  EXPECT_TRUE(t.verify());
}

TEST(AvlTree, DuplicateDeepInTree)
{
  // Build a deeper tree and test duplicate at various levels
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  // Insert in order that creates a balanced tree
  for (int k : {50, 25, 75, 12, 37, 62, 87, 6, 18, 31, 43})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_TRUE(t.verify());

  // Test duplicates at different levels
  EXPECT_EQ(t.insert(pool.make(50)), nullptr);  // root
  EXPECT_EQ(t.insert(pool.make(25)), nullptr);  // level 1
  EXPECT_EQ(t.insert(pool.make(37)), nullptr);  // level 2
  EXPECT_EQ(t.insert(pool.make(31)), nullptr);  // level 3

  EXPECT_TRUE(t.verify());

  // Remove and verify tree integrity
  for (int k : {31, 37, 25, 50})
    {
      auto * rem = t.remove(k);
      ASSERT_NE(rem, nullptr);
      EXPECT_EQ(KEY(rem), k);
      EXPECT_TRUE(t.verify());
    }
}

TEST(AvlTree, InsertDupAllowsDuplicates)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  ASSERT_NE(t.insert_dup(pool.make(5)), nullptr);
  ASSERT_NE(t.insert_dup(pool.make(5)), nullptr);
  ASSERT_NE(t.insert_dup(pool.make(5)), nullptr);

  EXPECT_TRUE(t.verify());
  EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{5, 5, 5}));
}

TEST(AvlTree, RemoveMissingReturnsNull)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  for (int k : {1, 2, 3})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_EQ(t.remove(42), nullptr);
  EXPECT_TRUE(t.verify());
}

TEST(AvlTree, RemoveReturnsDetachedNode)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  for (int k : {3, 1, 4, 2})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  auto * removed = t.remove(1);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 1);
  EXPECT_EQ(LLINK(removed), Avl_Tree<int>::Node::NullPtr);
  EXPECT_EQ(RLINK(removed), Avl_Tree<int>::Node::NullPtr);
  EXPECT_EQ(static_cast<int>(DIFF(removed)), 0);

  EXPECT_TRUE(t.verify());

  pool.forget(removed);
  delete removed;
}

TEST(AvlTree, Property_RandomInsertRemove_StableInvariants)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 500);

  std::set<int> present;

  for (int i = 0; i < 300; ++i)
    {
      int k = dist(rng);
      auto * p = pool.make(k);
      auto * ins = t.insert(p);
      if (ins == nullptr)
        {
          pool.forget(p);
          delete p;
        }
      else
        present.insert(k);

      ASSERT_TRUE(t.verify());
    }

  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto * removed = t.remove(k);
      if (removed != nullptr)
        {
          present.erase(k);
          pool.forget(removed);
          delete removed;
        }
      ASSERT_TRUE(t.verify());
    }
}

TEST(AvlTree, Rotations_LL_RR_LR_RL)
{
  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(3)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr);
    ASSERT_NE(t.insert(pool.make(1)), nullptr); // LL
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }

  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(1)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr);
    ASSERT_NE(t.insert(pool.make(3)), nullptr); // RR
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }

  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(3)), nullptr);
    ASSERT_NE(t.insert(pool.make(1)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr); // LR
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }

  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(1)), nullptr);
    ASSERT_NE(t.insert(pool.make(3)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr); // RL
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }
}

TEST(AvlTree, Property_OracleSet_MatchesInorder)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> dist(0, 2000);
  std::bernoulli_distribution do_insert(0.6);

  std::set<int> oracle;

  for (int i = 0; i < 1500; ++i)
    {
      int k = dist(rng);
      if (do_insert(rng))
        {
          auto * p = pool.make(k);
          auto * ins = t.insert(p);
          if (ins == nullptr)
            {
              pool.forget(p);
              delete p;
            }
          else
            oracle.insert(k);
        }
      else
        {
          if (auto * removed = t.remove(k); removed != nullptr)
            {
              oracle.erase(k);
              pool.forget(removed);
              delete removed;
            }
        }

      ASSERT_TRUE(t.verify());

      std::vector<int> expected(oracle.begin(), oracle.end());
      auto got = inorder_keys<Avl_Tree<int>::Node>(t.getRoot());
      ASSERT_EQ(got, expected);
    }
}

TEST(AvlTree, WorksWithCustomComparator)
{
  struct Greater
  {
    bool operator()(int a, int b) const noexcept { return a > b; }
  };

  Avl_Tree<int, Greater> t;
  NodePool<Avl_Tree<int, Greater>> pool;

  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_TRUE(t.verify());
  auto got = inorder_keys<Avl_Tree<int, Greater>::Node>(t.getRoot());
  const std::vector<int> expected{5, 4, 3, 2, 1};
  EXPECT_EQ(got, expected);

  auto * removed = t.remove(4);
  ASSERT_NE(removed, nullptr);
  pool.forget(removed);
  delete removed;

  EXPECT_TRUE(t.verify());
}

// ============================================================================
// STRESS TESTS / FUZZING
// ============================================================================

// Stress test: ascending insertion (worst case for naive BST)
TEST(AvlTree, Stress_AscendingInsertion)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  const int n = 10000;
  
  for (int i = 0; i < n; ++i)
    {
      ASSERT_NE(t.insert(pool.make(i)), nullptr) << "Insert failed at i=" << i;
      ASSERT_TRUE(t.verify()) << "AVL invariant violated at i=" << i;
    }
  
  // Verify all elements are present
  for (int i = 0; i < n; ++i)
    ASSERT_NE(t.search(i), nullptr) << "Element " << i << " not found";
}

// Stress test: descending insertion
TEST(AvlTree, Stress_DescendingInsertion)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  const int n = 10000;
  
  for (int i = n - 1; i >= 0; --i)
    {
      ASSERT_NE(t.insert(pool.make(i)), nullptr);
      ASSERT_TRUE(t.verify());
    }
  
  for (int i = 0; i < n; ++i)
    ASSERT_NE(t.search(i), nullptr);
}

// Stress test: zigzag insertion pattern
TEST(AvlTree, Stress_ZigzagInsertion)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  const int n = 5000;
  
  // Insert in zigzag pattern: 0, n-1, 1, n-2, 2, n-3, ...
  for (int i = 0; i < n / 2; ++i)
    {
      ASSERT_NE(t.insert(pool.make(i)), nullptr);
      ASSERT_TRUE(t.verify());
      ASSERT_NE(t.insert(pool.make(n - 1 - i)), nullptr);
      ASSERT_TRUE(t.verify());
    }
  
  for (int i = 0; i < n; ++i)
    ASSERT_NE(t.search(i), nullptr);
}

// Stress test: large scale fuzzing with oracle
TEST(AvlTree, Fuzz_LargeScaleRandomOps)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  std::mt19937 rng(99999);
  std::uniform_int_distribution<int> key_dist(0, 50000);
  std::uniform_int_distribution<int> op_dist(0, 2);
  
  std::set<int> oracle;
  
  const int num_ops = 20000;
  
  for (int i = 0; i < num_ops; ++i)
    {
      int key = key_dist(rng);
      int op = op_dist(rng);
      
      switch (op)
        {
        case 0:  // Insert
          {
            auto * p = pool.make(key);
            auto * ins = t.insert(p);
            if (ins == nullptr)
              {
                pool.forget(p);
                delete p;
                EXPECT_TRUE(oracle.count(key) > 0);
              }
            else
              {
                EXPECT_TRUE(oracle.count(key) == 0);
                oracle.insert(key);
              }
            break;
          }
        case 1:  // Remove
          {
            auto * removed = t.remove(key);
            if (removed != nullptr)
              {
                EXPECT_TRUE(oracle.count(key) > 0);
                oracle.erase(key);
                pool.forget(removed);
                delete removed;
              }
            else
              {
                EXPECT_TRUE(oracle.count(key) == 0);
              }
            break;
          }
        case 2:  // Search
          {
            auto * found = t.search(key);
            bool in_oracle = oracle.count(key) > 0;
            EXPECT_EQ(found != nullptr, in_oracle);
            if (found)
              EXPECT_EQ(KEY(found), key);
            break;
          }
        }
      
      ASSERT_TRUE(t.verify()) << "AVL invariant violated at op " << i;
    }
  
  // Final verification
  std::vector<int> expected(oracle.begin(), oracle.end());
  auto got = inorder_keys<Avl_Tree<int>::Node>(t.getRoot());
  ASSERT_EQ(got, expected);
}

// Stress test: bulk insert then bulk remove
TEST(AvlTree, Stress_BulkInsertBulkRemove)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  const int n = 10000;
  
  // Bulk insert
  std::vector<int> keys(n);
  std::iota(keys.begin(), keys.end(), 0);
  
  std::mt19937 rng(12345);
  std::shuffle(keys.begin(), keys.end(), rng);
  
  for (int k : keys)
    {
      ASSERT_NE(t.insert(pool.make(k)), nullptr);
    }
  
  ASSERT_TRUE(t.verify());
  
  // Bulk remove in different random order
  std::shuffle(keys.begin(), keys.end(), rng);
  
  for (int k : keys)
    {
      auto * removed = t.remove(k);
      ASSERT_NE(removed, nullptr) << "Remove failed for key " << k;
      pool.forget(removed);
      delete removed;
      ASSERT_TRUE(t.verify());
    }
  
  EXPECT_EQ(t.getRoot(), Avl_Tree<int>::Node::NullPtr);
}

// Stress test: repeated insert_dup (duplicates allowed)
TEST(AvlTree, Stress_ManyDuplicates)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  const int num_keys = 100;
  const int dups_per_key = 50;
  
  // Insert many duplicates of each key
  for (int k = 0; k < num_keys; ++k)
    {
      for (int d = 0; d < dups_per_key; ++d)
        {
          ASSERT_NE(t.insert_dup(pool.make(k)), nullptr);
        }
    }
  
  ASSERT_TRUE(t.verify());
  
  // Verify inorder traversal has correct count
  auto keys = inorder_keys<Avl_Tree<int>::Node>(t.getRoot());
  EXPECT_EQ(keys.size(), num_keys * dups_per_key);
  
  // Verify sorted
  EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
}

// Stress test: alternating insert and remove
TEST(AvlTree, Stress_AlternatingInsertRemove)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  
  std::set<int> present;
  std::mt19937 rng(54321);
  std::uniform_int_distribution<int> dist(0, 10000);
  
  const int n = 15000;
  
  for (int i = 0; i < n; ++i)
    {
      int k = dist(rng);
      
      if (i % 2 == 0)  // Insert
        {
          auto * p = pool.make(k);
          if (t.insert(p) == nullptr)
            {
              pool.forget(p);
              delete p;
            }
          else
            {
              present.insert(k);
            }
        }
      else  // Remove
        {
          if (auto * removed = t.remove(k); removed)
            {
              present.erase(k);
              pool.forget(removed);
              delete removed;
            }
        }
      
      ASSERT_TRUE(t.verify()) << "AVL invariant violated at i=" << i;
    }
  
  // Final check
  std::vector<int> expected(present.begin(), present.end());
  EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), expected);
}

// Stress test with string keys
TEST(AvlTree, Stress_StringKeys)
{
  Avl_Tree<std::string> t;
  NodePool<Avl_Tree<std::string>> pool;
  
  std::mt19937 rng(11111);
  std::uniform_int_distribution<int> len_dist(1, 30);
  std::uniform_int_distribution<int> char_dist('a', 'z');
  
  auto random_string = [&]() {
    int len = len_dist(rng);
    std::string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i)
      s += static_cast<char>(char_dist(rng));
    return s;
  };
  
  std::set<std::string> oracle;
  
  const int n = 5000;
  
  for (int i = 0; i < n; ++i)
    {
      std::string s = random_string();
      auto * p = pool.make(s);
      if (t.insert(p) != nullptr)
        oracle.insert(s);
      else
        {
          pool.forget(p);
          delete p;
        }
    }
  
  ASSERT_TRUE(t.verify());
  
  // Verify all oracle strings are present
  for (const auto & s : oracle)
    ASSERT_NE(t.search(s), nullptr) << "String key missing: " << s;
}
