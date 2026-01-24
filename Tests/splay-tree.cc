
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
 * @file splay-tree.cc
 * @brief Tests for Splay Tree
 */

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_splay_tree.H>

using namespace Aleph;

namespace
{
  template <class Node>
  size_t count_nodes(Node * root) noexcept
  {
    if (root == Node::NullPtr)
      return 0;
    return 1 + count_nodes(LLINK(root)) + count_nodes(RLINK(root));
  }

  template <class Node>
  std::vector<typename Node::key_type> inorder_keys(Node * root)
  {
    std::vector<typename Node::key_type> keys;
    if (root == Node::NullPtr)
      return keys;

    auto left = inorder_keys(LLINK(root));
    keys.insert(keys.end(), left.begin(), left.end());
    keys.push_back(KEY(root));
    auto right = inorder_keys(RLINK(root));
    keys.insert(keys.end(), right.begin(), right.end());
    return keys;
  }

  template <class Tree>
  void assert_valid_tree(Tree & tree)
  {
    ASSERT_TRUE(tree.verify()) << "BST invariant violated";
  }

  template <class Tree>
  struct NodePool
  {
    using Node = typename Tree::Node;
    std::vector<Node *> allocated;

    Node * make(typename Node::key_type k)
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

  struct StatefulLess
  {
    bool use_abs = false;

    StatefulLess() = default;
    explicit StatefulLess(bool b) : use_abs(b) {}

    bool operator()(int a, int b) const
    {
      if (use_abs)
        {
          a = std::abs(a);
          b = std::abs(b);
        }
      return a < b;
    }
  };

} // namespace

TEST(SplayTree, EmptyTreeProperties)
{
  Splay_Tree<int> tree;
  EXPECT_EQ(tree.getRoot(), Splay_Tree<int>::Node::NullPtr);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_EQ(tree.remove(42), nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST(SplayTree, InsertSingleElementBecomesRoot)
{
  using Tree = Splay_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  auto * p = pool.make(7);
  EXPECT_EQ(tree.insert(p), p);
  EXPECT_EQ(tree.getRoot(), p);
  assert_valid_tree(tree);
  EXPECT_EQ(count_nodes<Node>(tree.getRoot()), 1u);
}

TEST(SplayTree, InsertRejectsDuplicates)
{
  using Tree = Splay_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  auto * p1 = pool.make(10);
  EXPECT_EQ(tree.insert(p1), p1);

  auto * p2 = pool.make(10);
  EXPECT_EQ(tree.insert(p2), nullptr);

  assert_valid_tree(tree);
  EXPECT_EQ(count_nodes<Node>(tree.getRoot()), 1u);
}

TEST(SplayTree, InsertDupAllowsDuplicates)
{
  using Tree = Splay_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tree.insert_dup(pool.make(42)), nullptr);

  assert_valid_tree(tree);
  EXPECT_EQ(count_nodes<Node>(tree.getRoot()), 5u);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{42, 42, 42, 42, 42}));
}

TEST(SplayTree, SearchFindsExistingKeyAndSplaysToRoot)
{
  using Tree = Splay_Tree<int>;

  Tree tree;
  NodePool<Tree> pool;

  for (int k : {5, 3, 7, 1, 4, 6, 8})
    tree.insert(pool.make(k));

  auto * found = tree.search(4);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(KEY(found), 4);
  EXPECT_EQ(tree.getRoot(), found);

  assert_valid_tree(tree);
}

TEST(SplayTree, SearchMissingSplaysToExtreme)
{
  using Tree = Splay_Tree<int>;

  Tree tree;
  NodePool<Tree> pool;

  for (int k : {1, 3, 5})
    tree.insert(pool.make(k));

  EXPECT_EQ(tree.search(0), nullptr);
  ASSERT_NE(tree.getRoot(), Tree::Node::NullPtr);
  EXPECT_EQ(KEY(tree.getRoot()), 1);

  EXPECT_EQ(tree.search(6), nullptr);
  ASSERT_NE(tree.getRoot(), Tree::Node::NullPtr);
  EXPECT_EQ(KEY(tree.getRoot()), 5);

  assert_valid_tree(tree);
}

TEST(SplayTree, SearchOrInsertDoesNotInsertDuplicate)
{
  using Tree = Splay_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  auto * p1 = pool.make(10);
  EXPECT_EQ(tree.search_or_insert(p1), p1);

  auto * p2 = pool.make(10);
  auto * ret2 = tree.search_or_insert(p2);
  EXPECT_EQ(ret2, p1);

  assert_valid_tree(tree);
  EXPECT_EQ(count_nodes<Node>(tree.getRoot()), 1u);
}

TEST(SplayTree, RemoveExistingKeyDetachesNode)
{
  using Tree = Splay_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  Node * removed = tree.remove(3);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 3);
  EXPECT_EQ(LLINK(removed), Node::NullPtr);
  EXPECT_EQ(RLINK(removed), Node::NullPtr);

  pool.forget(removed);
  delete removed;

  EXPECT_EQ(tree.search(3), nullptr);
  assert_valid_tree(tree);

  auto keys = inorder_keys(tree.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{1, 2, 4, 5}));
}

TEST(SplayTree, RemoveSplaysMaxOfLeftSubtree)
{
  using Tree = Splay_Tree<int>;
  using Node = Tree::Node;

  Tree tree;
  NodePool<Tree> pool;

  for (int k : {1, 2, 3, 4, 5})
    tree.insert(pool.make(k));

  Node * removed = tree.remove(5);
  ASSERT_NE(removed, nullptr);
  pool.forget(removed);
  delete removed;

  ASSERT_NE(tree.getRoot(), Node::NullPtr);
  EXPECT_EQ(KEY(tree.getRoot()), 4);
  assert_valid_tree(tree);
}

TEST(SplayTree, StatefulComparatorAffectsEquality)
{
  using Tree = Splay_Tree<int, StatefulLess>;
  using Node = Tree::Node;

  Tree tree(StatefulLess(true));
  NodePool<Tree> pool;

  auto * p = pool.make(1);
  tree.insert(p);

  Node * found = tree.search(-1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, p);
  EXPECT_EQ(tree.getRoot(), p);
  assert_valid_tree(tree);
}
