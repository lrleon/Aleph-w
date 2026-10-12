/*
                          Aleph_w

                           / \  | | ___ _ __ | |__      __      __
                          / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
                         / ___ \| |  __/ |_) | | | |_____\ V  V /
                        /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                                       |_|

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
 * @file tree_path_capacity_test.cc
 * @brief The search path of a balanced tree holds the head node and then up
 *        to `Node::MaxHeight` nodes, so a tree as tall as `MaxHeight` can be
 *        updated at its deepest level.
 *
 * The trees are built on nodes with a `MaxHeight` of 4, which 15 keys fill.
 * Removing a leaf at depth 3 builds a path of 5 entries: before the path
 * reserved a slot for the head node, the last push overflowed it (a Debug
 * assertion, a write past the inline storage otherwise).
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <tpl_avl.H>
#include <tpl_binNodeUtils.H>
#include <tpl_rb_tree.H>

using namespace Aleph;

// Nodes whose MaxHeight a few keys reach.
DECLARE_BINNODE(Short_AvlNode, 4, AvlNode_Data);
DECLARE_BINNODE_SENTINEL(Short_RbNode, 4, RbNode_Data);

namespace
{
  // Owns the nodes of a tree, which does not free them.
  template <class Node>
  struct Node_Pool
  {
    std::vector<std::unique_ptr<Node>> nodes;

    Node * make(const int key)
    {
      nodes.push_back(std::make_unique<Node>(key));
      return nodes.back().get();
    }
  };

  // The keys 1 ... 15 in the level order of a perfect tree: inserted in
  // this order they keep a balanced tree of height 4 with no rotation.
  constexpr int level_order[] = {8, 4, 12, 2, 6, 10, 14, 1, 3, 5, 7, 9, 11, 13, 15};

  // Its leaves, at depth 3.
  constexpr int leaves[] = {1, 3, 5, 7, 9, 11, 13, 15};
}

TEST(TreePathCapacity, AvlTreeAsTallAsMaxHeight)
{
  using Tree = Gen_Avl_Tree<Short_AvlNode, int, Aleph::less<int>>;
  static_assert(Tree::Node::MaxHeight == 4);

  Node_Pool<Tree::Node> pool;
  Tree tree;
  for (const int key : level_order)
    ASSERT_NE(tree.insert(pool.make(key)), nullptr);
  ASSERT_EQ(computeHeightRec(tree.getRoot()), 4u);
  ASSERT_TRUE(tree.verify());

  for (const int key : leaves)
    {
      ASSERT_NE(tree.remove(key), nullptr) << "key " << key;
      ASSERT_TRUE(tree.verify());
    }
  EXPECT_EQ(size(tree.getRoot()), 7u);
}

TEST(TreePathCapacity, RbTreeAsTallAsMaxHeight)
{
  using Tree = Gen_Rb_Tree<Short_RbNode, int, Aleph::less<int>>;
  static_assert(Tree::Node::MaxHeight == 4);

  Node_Pool<Tree::Node> pool;
  Tree tree;
  for (const int key : level_order)
    ASSERT_NE(tree.insert(pool.make(key)), nullptr);
  ASSERT_EQ(computeHeightRec(tree.getRoot()), 4u);
  ASSERT_TRUE(tree.verify());

  for (const int key : leaves)
    {
      ASSERT_NE(tree.remove(key), nullptr) << "key " << key;
      ASSERT_TRUE(tree.verify());
    }
  EXPECT_EQ(tree.size(), 7u);
}
