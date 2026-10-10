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
 * @file search_trees_test_common.H
 * @brief What the tests of the binary search trees share: the list of tree
 *        types, what each node carries, and a pool that owns the nodes.
 */

# ifndef SEARCH_TREES_TEST_COMMON_H
# define SEARCH_TREES_TEST_COMMON_H

# include <gtest/gtest.h>

# include <cstddef>
# include <memory>
# include <string>
# include <vector>

# include <tpl_avl.H>
# include <tpl_avlRk.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binTree.H>
# include <tpl_hRbTree.H>
# include <tpl_hRbTreeRk.H>
# include <tpl_rand_tree.H>
# include <tpl_rbRk.H>
# include <tpl_rb_tree.H>
# include <tpl_splay_tree.H>
# include <tpl_splay_treeRk.H>
# include <tpl_tdRbTree.H>
# include <tpl_tdRbTreeRk.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>

namespace search_trees_test
{
  // Most of the trees are in Aleph; the splay trees are in the global
  // namespace.
  using namespace Aleph;

  /// The node has a red or black color.
  template <class Node>
  concept Colored_Node = requires(Node * p) { p->getColor(); };

  /// The node stores the AVL balance factor.
  template <class Node>
  concept Avl_Balanced_Node = requires(Node * p) { p->getDiff(); };

  /// The node stores the size of its subtree.
  template <class Node>
  concept Counted_Node = requires(Node * p) { p->getCount(); };

  /// The node stores a heap priority.
  template <class Node>
  concept Prioritized_Node = requires(Node * p) { p->getPriority(); };

  /// The tree draws random numbers, from a seed that can be set.
  template <class Tree>
  concept Seeded_Tree = requires(Tree & tree) { tree.set_seed(1UL); };

  /// Owns the nodes of a tree, which does not free them.
  template <class Node>
  class Node_Pool
  {
    std::vector<std::unique_ptr<Node>> nodes;

  public:
    Node * make(const int key)
    {
      nodes.push_back(std::make_unique<Node>(key));
      return nodes.back().get();
    }

    [[nodiscard]] size_t size() const noexcept { return nodes.size(); }
  };

  /// The name a tree type shows in the name of its tests.
  template <class Tree>
  struct Tree_Name;

# define SEARCH_TREES_TEST_NAME(Tree)                           \
  template <>                                                   \
  struct Tree_Name<Tree<int>>                                   \
  {                                                             \
    static constexpr const char * value = #Tree;                \
  }

  SEARCH_TREES_TEST_NAME(BinTree);
  SEARCH_TREES_TEST_NAME(Avl_Tree);
  SEARCH_TREES_TEST_NAME(Avl_Tree_Rk);
  SEARCH_TREES_TEST_NAME(Rb_Tree);
  SEARCH_TREES_TEST_NAME(Rb_Tree_Rk);
  SEARCH_TREES_TEST_NAME(TdRbTree);
  SEARCH_TREES_TEST_NAME(TdRbTreeRk);
  SEARCH_TREES_TEST_NAME(HtdRbTree);
  SEARCH_TREES_TEST_NAME(HtdRbTreeRk);
  SEARCH_TREES_TEST_NAME(Treap);
  SEARCH_TREES_TEST_NAME(Treap_Rk);
  SEARCH_TREES_TEST_NAME(Rand_Tree);
  SEARCH_TREES_TEST_NAME(Splay_Tree);
  SEARCH_TREES_TEST_NAME(Splay_Tree_Rk);

# undef SEARCH_TREES_TEST_NAME

  /// Names the instances of a typed test suite after their tree.
  struct Tree_Names
  {
    template <class Tree>
    static std::string GetName(int)
    {
      return Tree_Name<Tree>::value;
    }
  };

  /// Every binary search tree of the library.
  using All_Trees = ::testing::Types<
    BinTree<int>, Avl_Tree<int>, Avl_Tree_Rk<int>,
    Rb_Tree<int>, Rb_Tree_Rk<int>, TdRbTree<int>,
    TdRbTreeRk<int>, HtdRbTree<int>, HtdRbTreeRk<int>,
    Treap<int>, Treap_Rk<int>, Rand_Tree<int>,
    Splay_Tree<int>, Splay_Tree_Rk<int>>;

  /// The trees whose nodes count the nodes of their subtree.
  using Ranked_Trees = ::testing::Types<
    Avl_Tree_Rk<int>, Rb_Tree_Rk<int>, TdRbTreeRk<int>,
    HtdRbTreeRk<int>, Treap_Rk<int>, Rand_Tree<int>,
    Splay_Tree_Rk<int>>;

  /// The red-black trees.
  using Red_Black_Trees = ::testing::Types<
    Rb_Tree<int>, Rb_Tree_Rk<int>, TdRbTree<int>,
    TdRbTreeRk<int>, HtdRbTree<int>, HtdRbTreeRk<int>>;

  /// The AVL trees.
  using Avl_Trees = ::testing::Types<Avl_Tree<int>, Avl_Tree_Rk<int>>;

  /// The treaps.
  using Treaps = ::testing::Types<Treap<int>, Treap_Rk<int>>;
}

# endif // SEARCH_TREES_TEST_COMMON_H
