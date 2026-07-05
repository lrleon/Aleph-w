/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file tpl_binNodeGenerators_test.cc
 * @brief Tests for lazy binary tree traversals (tpl_binNodeGenerators.H).
 *
 * Property tests comparing lazy_in_order/lazy_pre_order/lazy_post_order
 * against the eager for_each_in_order/for_each_preorder/for_each_postorder
 * traversals from tpl_binNodeUtils.H: same node sequence, same order.
 * Also covers an empty tree, a single node, early break, and a deliberately
 * degenerate (linked-list-shaped) tree to exercise deep explicit stack state.
 */

#include <vector>

#include <gtest/gtest.h>

#include <tpl_binNode.H>
#include <tpl_binNodeGenerators.H>
#include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace
{
// Owns every node allocated for a test tree and frees them on destruction.
struct NodePool
{
  std::vector<BinNode<int> *> allocated;

  BinNode<int> *make(int k)
  {
    auto *p = new BinNode<int>(k);
    allocated.push_back(p);
    return p;
  }

  ~NodePool()
  {
    for (BinNode<int> *p : allocated)
      delete p;
  }
};

// Builds the complete binary tree with root 4, children 2 and 6, and
// grandchildren 1, 3, 5, 7 (in that left-to-right order).
BinNode<int> *build_balanced(NodePool &pool)
{
  BinNode<int> *n1 = pool.make(1);
  BinNode<int> *n2 = pool.make(2);
  BinNode<int> *n3 = pool.make(3);
  BinNode<int> *n4 = pool.make(4);
  BinNode<int> *n5 = pool.make(5);
  BinNode<int> *n6 = pool.make(6);
  BinNode<int> *n7 = pool.make(7);

  n4->getL() = n2;
  n4->getR() = n6;
  n2->getL() = n1;
  n2->getR() = n3;
  n6->getL() = n5;
  n6->getR() = n7;
  return n4;
}

// Right-degenerate chain 1 -> 2 -> 3 -> ... -> n (every node's right child
// is the next one; no left children). Exercises O(n)-deep traversal state.
BinNode<int> *build_degenerate_chain(NodePool &pool, int n)
{
  BinNode<int> *root = pool.make(1);
  BinNode<int> *tail = root;
  for (int k = 2; k <= n; ++k)
    {
      BinNode<int> *next = pool.make(k);
      tail->getR() = next;
      tail = next;
    }
  return root;
}

template <class Node>
std::vector<int> eager_in_order(Node *root)
{
  std::vector<int> keys;
  for_each_in_order<Node>(root, [&](Node *p) { keys.push_back(KEY(p)); });
  return keys;
}

template <class Node>
std::vector<int> eager_pre_order(Node *root)
{
  std::vector<int> keys;
  for_each_preorder<Node>(root, [&](Node *p) { keys.push_back(KEY(p)); });
  return keys;
}

template <class Node>
std::vector<int> eager_post_order(Node *root)
{
  std::vector<int> keys;
  for_each_postorder<Node>(root, [&](Node *p) { keys.push_back(KEY(p)); });
  return keys;
}

template <class Node>
std::vector<int> lazy_in_order_keys(Node *root)
{
  std::vector<int> keys;
  for (Node *n : lazy_in_order(root))
    keys.push_back(KEY(n));
  return keys;
}

template <class Node>
std::vector<int> lazy_pre_order_keys(Node *root)
{
  std::vector<int> keys;
  for (Node *n : lazy_pre_order(root))
    keys.push_back(KEY(n));
  return keys;
}

template <class Node>
std::vector<int> lazy_post_order_keys(Node *root)
{
  std::vector<int> keys;
  for (Node *n : lazy_post_order(root))
    keys.push_back(KEY(n));
  return keys;
}
}  // namespace

TEST(BinNodeGenerators, EmptyTreeYieldsNothing)
{
  EXPECT_TRUE(lazy_in_order_keys<BinNode<int>>(BinNode<int>::NullPtr).empty());
  EXPECT_TRUE(lazy_pre_order_keys<BinNode<int>>(BinNode<int>::NullPtr).empty());
  EXPECT_TRUE(lazy_post_order_keys<BinNode<int>>(BinNode<int>::NullPtr).empty());
}

TEST(BinNodeGenerators, SingleNodeYieldsThatNode)
{
  NodePool pool;
  BinNode<int> *root = pool.make(42);
  EXPECT_EQ(lazy_in_order_keys(root), (std::vector<int>{42}));
  EXPECT_EQ(lazy_pre_order_keys(root), (std::vector<int>{42}));
  EXPECT_EQ(lazy_post_order_keys(root), (std::vector<int>{42}));
}

TEST(BinNodeGenerators, InOrderMatchesEagerTraversal)
{
  NodePool pool;
  BinNode<int> *root = build_balanced(pool);
  EXPECT_EQ(lazy_in_order_keys(root), eager_in_order(root));
  EXPECT_EQ(lazy_in_order_keys(root), (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
}

TEST(BinNodeGenerators, PreOrderMatchesEagerTraversal)
{
  NodePool pool;
  BinNode<int> *root = build_balanced(pool);
  EXPECT_EQ(lazy_pre_order_keys(root), eager_pre_order(root));
  EXPECT_EQ(lazy_pre_order_keys(root), (std::vector<int>{4, 2, 1, 3, 6, 5, 7}));
}

TEST(BinNodeGenerators, PostOrderMatchesEagerTraversal)
{
  NodePool pool;
  BinNode<int> *root = build_balanced(pool);
  EXPECT_EQ(lazy_post_order_keys(root), eager_post_order(root));
  EXPECT_EQ(lazy_post_order_keys(root), (std::vector<int>{1, 3, 2, 5, 7, 6, 4}));
}

TEST(BinNodeGenerators, EarlyBreakStopsTraversal)
{
  NodePool pool;
  BinNode<int> *root = build_balanced(pool);

  std::vector<int> seen;
  for (BinNode<int> *n : lazy_in_order(root))
    {
      seen.push_back(KEY(n));
      if (KEY(n) == 3)
        break;
    }
  // In-order of the balanced tree starts 1, 2, 3, ...; must stop right there.
  EXPECT_EQ(seen, (std::vector<int>{1, 2, 3}));
}

TEST(BinNodeGenerators, DegenerateChainMatchesEagerTraversal)
{
  constexpr int n = 500;  // deep enough to exercise explicit stack state
  NodePool pool;
  BinNode<int> *root = build_degenerate_chain(pool, n);

  std::vector<int> expected(n);
  for (int i = 0; i < n; ++i)
    expected[i] = i + 1;

  // A right-only chain is its own pre-order, in-order and post-order... in
  // different shapes: in-order and pre-order both visit root-then-right
  // recursively so they coincide with the chain order; post-order visits
  // right subtree fully before the (chain) root, i.e. reverse order.
  EXPECT_EQ(lazy_in_order_keys(root), expected);
  EXPECT_EQ(lazy_pre_order_keys(root), expected);
  EXPECT_EQ(lazy_in_order_keys(root), eager_in_order(root));
  EXPECT_EQ(lazy_pre_order_keys(root), eager_pre_order(root));
  EXPECT_EQ(lazy_post_order_keys(root), eager_post_order(root));

  std::vector<int> reversed(expected.rbegin(), expected.rend());
  EXPECT_EQ(lazy_post_order_keys(root), reversed);
}
