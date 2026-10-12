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
 * @file tree_verifiers_test.cc
 * @brief The functions that validate a search tree reject the trees that
 *        break its invariants.
 *
 * Every other test of the trees trusts `verify()`, `is_avl()`,
 * `is_red_black_tree()`, `is_treap()`, `check_rank_tree()` and `check_bst()`
 * to tell a right tree from a wrong one. Here those functions are what is
 * tested, in two ways:
 *
 *  - trees built by hand, node by node, each one with a single defect;
 *  - trees built by the insertions of each tree type, where one node at a
 *    time is spoiled (its key, its counter, its color, its balance factor
 *    or its priority) and `verify()` must notice, wherever the node is.
 */

#include "search_trees_test_common.H"

#include <functional>
#include <utility>

#include <tpl_dynSetTree.H>

using namespace Aleph;
using namespace search_trees_test;

namespace
{
  // A tree built by hand. `node(k, l, r)` makes the node of key `k` over the
  // subtrees `l` and `r`, where a null pointer is the empty tree.
  template <class Node>
  class Hand_Tree
  {
    Node_Pool<Node> pool;

  public:
    Node * node(const int key, Node * l = nullptr, Node * r = nullptr)
    {
      Node * p = pool.make(key);
      if (l != nullptr)
        LLINK(p) = l;
      if (r != nullptr)
        RLINK(p) = r;
      return p;
    }

    Node * black(const int key, Node * l = nullptr, Node * r = nullptr)
    {
      Node * p = node(key, l, r);
      COLOR(p) = BLACK;
      return p;
    }

    Node * red(const int key, Node * l = nullptr, Node * r = nullptr)
    {
      Node * p = node(key, l, r);
      COLOR(p) = RED;
      return p;
    }

    Node * with_priority(const unsigned long priority, const int key,
                         Node * l = nullptr, Node * r = nullptr)
    {
      Node * p = node(key, l, r);
      p->getPriority() = priority;
      return p;
    }
  };

  // Writes in every node the size of its subtree.
  template <class Node>
  size_t set_counters(Node * p) noexcept
  {
    if (p == Node::NullPtr)
      return 0;
    const size_t n = set_counters(LLINK(p)) + set_counters(RLINK(p)) + 1;
    p->getCount() = n;
    return n;
  }

  // Writes in every node its balance factor; returns the height.
  template <class Node>
  int set_balance_factors(Node * p) noexcept
  {
    if (p == Node::NullPtr)
      return 0;
    const int hl = set_balance_factors(LLINK(p));
    const int hr = set_balance_factors(RLINK(p));
    DIFF(p) = static_cast<signed char>(hr - hl);
    return 1 + (hl > hr ? hl : hr);
  }

  // The counters of a hand-made tree, where its nodes have them.
  template <class Node>
  Node * counted(Node * root) noexcept
  {
    if constexpr (Counted_Node<Node>)
      set_counters(root);
    return root;
  }

  // What `Tree::verify()` says of the tree of root `root`.
  template <class Tree>
  bool verified_by(typename Tree::Node * root)
  {
    Tree tree;
    tree.getRoot() = root;
    const bool ok = tree.verify();
    tree.getRoot() = Tree::Node::NullPtr;
    return ok;
  }

  bool is_red_black_tree_of(RbNode<int> * root) { return is_red_black_tree(root); }
  bool is_red_black_tree_of(RbNodeRk<int> * root) { return is_red_black_tree_rk(root); }

  bool has_equal_black_heights(RbNode<int> * root)
  {
    int max = -1;
    return test_black_condition(root, max);
  }

  bool has_equal_black_heights(RbNodeRk<int> * root)
  {
    int max = -1;
    return test_black_condition_rk(root, max);
  }

  bool is_red_black_bst_of(RbNode<int> * root)
  {
    Aleph::less<int> cmp;
    return is_red_black_bst(root, cmp);
  }

  bool is_red_black_bst_of(RbNodeRk<int> * root)
  {
    Aleph::less<int> cmp;
    return is_red_black_bst_rk(root, cmp);
  }

  bool is_avl_of(AvlNode<int> * root) { return is_avl(root); }
  bool is_avl_of(AvlNodeRk<int> * root) { return is_avl_rk(root); }

  // The nodes of a tree in order, each one with its parent (the null
  // pointer for the root).
  template <class Node>
  void collect(Node * p, Node * parent, std::vector<std::pair<Node *, Node *>> & out)
  {
    if (p == Node::NullPtr)
      return;
    collect(LLINK(p), p, out);
    out.emplace_back(p, parent);
    collect(RLINK(p), p, out);
  }

  // A tree of `num_keys` nodes built by the insertions of its type, with the
  // keys 10, 20, 30... in an order that mixes them.
  template <class Tree>
  class Built_Tree
  {
  public:
    using Node = typename Tree::Node;

    static constexpr int num_keys = 40;

    Tree tree;
    std::vector<std::pair<Node *, Node *>> nodes; // in order, with parents

    Built_Tree()
    {
      if constexpr (Seeded_Tree<Tree>)
        tree.set_seed(20260710);

      // 41 is prime, so i -> 17 i mod 41 visits 1 ... 40 once each
      for (int i = 1; i <= num_keys; ++i)
        if (tree.insert(pool.make(10 * (17 * i % (num_keys + 1)))) == nullptr)
          ADD_FAILURE() << "a new key was not inserted";

      collect(tree.getRoot(), static_cast<Node *>(nullptr), nodes);
    }

  private:
    Node_Pool<Node> pool;
  };

  template <class Node>
  class RedBlackNodes : public ::testing::Test
  {
  protected:
    Hand_Tree<Node> t;
  };

  using Red_Black_Node_Types = ::testing::Types<RbNode<int>, RbNodeRk<int>>;
  TYPED_TEST_SUITE(RedBlackNodes, Red_Black_Node_Types);

  template <class Node>
  class AvlNodes : public ::testing::Test
  {
  protected:
    Hand_Tree<Node> t;

    // The factors and, where the node has them, the counters.
    Node * finished(Node * root)
    {
      set_balance_factors(root);
      return counted(root);
    }
  };

  using Avl_Node_Types = ::testing::Types<AvlNode<int>, AvlNodeRk<int>>;
  TYPED_TEST_SUITE(AvlNodes, Avl_Node_Types);

  template <class Tree> class AnyTreeVerify : public ::testing::Test {};
  template <class Tree> class RankedTreeVerify : public ::testing::Test {};
  template <class Tree> class RedBlackTreeVerify : public ::testing::Test {};
  template <class Tree> class AvlTreeVerify : public ::testing::Test {};
  template <class Tree> class TreapVerify : public ::testing::Test {};

  TYPED_TEST_SUITE(AnyTreeVerify, All_Trees, Tree_Names);
  TYPED_TEST_SUITE(RankedTreeVerify, Ranked_Trees, Tree_Names);
  TYPED_TEST_SUITE(RedBlackTreeVerify, Red_Black_Trees, Tree_Names);
  TYPED_TEST_SUITE(AvlTreeVerify, Avl_Trees, Tree_Names);
  TYPED_TEST_SUITE(TreapVerify, Treaps, Tree_Names);
}

// ---------------------------------------------------------------------------
// Red-black trees built by hand
// ---------------------------------------------------------------------------

TYPED_TEST(RedBlackNodes, AcceptsAValidTree)
{
  auto & t = this->t;
  auto * root = counted(t.black(8,
                                t.red(4, t.black(2, t.red(1)), t.black(6)),
                                t.red(12, t.black(10), t.black(14, nullptr, t.red(15)))));

  EXPECT_TRUE(is_red_black_tree_of(root));
  EXPECT_TRUE(has_equal_black_heights(root));
  EXPECT_TRUE(is_red_black_bst_of(root));
}

TYPED_TEST(RedBlackNodes, AcceptsTheEmptyTreeAndASingleNode)
{
  using Node = TypeParam;

  EXPECT_TRUE(is_red_black_tree_of(Node::NullPtr));
  EXPECT_TRUE(is_red_black_bst_of(Node::NullPtr));
  EXPECT_TRUE(is_red_black_tree_of(counted(this->t.black(1))));
  EXPECT_TRUE(is_red_black_tree_of(counted(this->t.red(1))));
}

// The trees of the library do not force the root to be black.
TYPED_TEST(RedBlackNodes, AcceptsARedRoot)
{
  auto & t = this->t;
  auto * root = counted(t.red(8,
                              t.black(4, t.red(2), t.red(6)),
                              t.black(12, nullptr, t.red(14))));

  EXPECT_TRUE(is_red_black_tree_of(root));
  EXPECT_TRUE(is_red_black_bst_of(root));
}

// The two red nodes are at depths 2 and 3, and every path has two black
// nodes: only a check of every node sees the defect.
TYPED_TEST(RedBlackNodes, RejectsARedNodeWithARedChildBelowTheRoot)
{
  auto & t = this->t;
  auto * root = counted(t.black(8,
                                t.black(4, t.red(2, t.red(1)), t.red(6)),
                                t.black(12, t.red(10), t.red(14))));

  ASSERT_TRUE(has_equal_black_heights(root));
  EXPECT_FALSE(is_red_black_tree_of(root));
  EXPECT_FALSE(is_red_black_bst_of(root));
}

TYPED_TEST(RedBlackNodes, RejectsARedRightChildOfARedNode)
{
  auto & t = this->t;
  auto * root = counted(t.black(8,
                                t.black(4),
                                t.black(12, nullptr, t.red(14, nullptr, t.red(15)))));

  EXPECT_FALSE(is_red_black_tree_of(root));
}

// The path that ends at the empty right subtree of the root crosses one
// black node; the one through its only child crosses two. No leaf shows it.
TYPED_TEST(RedBlackNodes, RejectsABlackNodeWithASingleBlackChild)
{
  auto & t = this->t;
  auto * root = counted(t.black(8, t.black(4)));

  EXPECT_FALSE(has_equal_black_heights(root));
  EXPECT_FALSE(is_red_black_tree_of(root));
  EXPECT_FALSE(is_red_black_bst_of(root));
}

TYPED_TEST(RedBlackNodes, RejectsASingleBlackChildDeepInTheTree)
{
  auto & t = this->t;
  auto * root = counted(t.black(8,
                                t.black(4, t.red(2), t.red(6, nullptr, t.black(7))),
                                t.black(12)));

  EXPECT_FALSE(has_equal_black_heights(root));
  EXPECT_FALSE(is_red_black_tree_of(root));
}

TYPED_TEST(RedBlackNodes, RejectsSubtreesWithDifferentBlackHeights)
{
  auto & t = this->t;
  auto * root = counted(t.black(8,
                                t.black(4, t.black(2), t.black(6)),
                                t.black(12)));

  EXPECT_FALSE(has_equal_black_heights(root));
  EXPECT_FALSE(is_red_black_tree_of(root));
}

TYPED_TEST(RedBlackNodes, RejectsAColorThatIsNeitherRedNorBlack)
{
  auto & t = this->t;
  auto * odd = t.black(2);
  auto * root = counted(t.black(8, t.red(4, odd, t.black(6)), t.black(12)));
  ASSERT_TRUE(is_red_black_tree_of(root));

  COLOR(odd) = 7;
  EXPECT_FALSE(is_red_black_tree_of(root));
}

TYPED_TEST(RedBlackNodes, RejectsKeysOutOfOrder)
{
  auto & t = this->t;
  auto * root = counted(t.black(8,
                                t.red(4, t.black(2), t.black(9)), // 9 > 8
                                t.red(12, t.black(10), t.black(14))));

  EXPECT_TRUE(is_red_black_tree_of(root)); // the colors are right
  EXPECT_FALSE(is_red_black_bst_of(root));
}

TEST(RedBlackRankNodes, RejectsAWrongCounterBelowTheRoot)
{
  Hand_Tree<RbNodeRk<int>> t;
  auto * deep = t.black(2, t.red(1));
  auto * root = counted(t.black(8,
                                t.red(4, deep, t.black(6)),
                                t.red(12, t.black(10), t.black(14))));
  ASSERT_TRUE(is_red_black_tree_rk(root));

  ++deep->getCount();
  EXPECT_FALSE(is_red_black_tree_rk(root));
  EXPECT_FALSE(check_rank_tree(root));
}

TEST(RedBlackTreesVerify, RejectTheHandMadeDefects)
{
  Hand_Tree<RbNode<int>> t;
  auto * red_red = t.black(8,
                           t.black(4, t.red(2, t.red(1)), t.red(6)),
                           t.black(12, t.red(10), t.red(14)));
  auto * single_black_child = t.black(8, t.black(4));
  auto * out_of_order = t.black(1, t.red(2), t.red(3));
  auto * valid = t.black(2, t.red(1), t.red(3));
  auto * red_root = t.red(2, t.black(1), t.black(3));

  for (auto * root : {valid, red_root})
    {
      EXPECT_TRUE(verified_by<Rb_Tree<int>>(root));
      EXPECT_TRUE(verified_by<TdRbTree<int>>(root));
      EXPECT_TRUE(verified_by<HtdRbTree<int>>(root));
    }

  for (auto * root : {red_red, single_black_child, out_of_order})
    {
      EXPECT_FALSE(verified_by<Rb_Tree<int>>(root)) << "root " << KEY(root);
      EXPECT_FALSE(verified_by<TdRbTree<int>>(root)) << "root " << KEY(root);
      EXPECT_FALSE(verified_by<HtdRbTree<int>>(root)) << "root " << KEY(root);
    }
}

TEST(RedBlackRankTreesVerify, RejectTheHandMadeDefects)
{
  Hand_Tree<RbNodeRk<int>> t;
  auto * red_red = counted(t.black(8,
                                   t.black(4, t.red(2, t.red(1)), t.red(6)),
                                   t.black(12, t.red(10), t.red(14))));
  auto * single_black_child = counted(t.black(8, t.black(4)));
  auto * out_of_order = counted(t.black(1, t.red(2), t.red(3)));
  auto * miscounted = counted(t.black(2, t.red(1), t.red(3)));
  miscounted->getCount() = 4;
  auto * valid = counted(t.black(2, t.red(1), t.red(3)));
  auto * red_root = counted(t.red(2, t.black(1), t.black(3)));

  for (auto * root : {valid, red_root})
    {
      EXPECT_TRUE(verified_by<Rb_Tree_Rk<int>>(root));
      EXPECT_TRUE(verified_by<TdRbTreeRk<int>>(root));
      EXPECT_TRUE(verified_by<HtdRbTreeRk<int>>(root));
    }

  for (auto * root : {red_red, single_black_child, out_of_order, miscounted})
    {
      EXPECT_FALSE(verified_by<Rb_Tree_Rk<int>>(root)) << "root " << KEY(root);
      EXPECT_FALSE(verified_by<TdRbTreeRk<int>>(root)) << "root " << KEY(root);
      EXPECT_FALSE(verified_by<HtdRbTreeRk<int>>(root)) << "root " << KEY(root);
    }
}

TEST(RedBlackTreesVerify, VerifyRedBlackLooksOnlyAtTheColors)
{
  Hand_Tree<RbNode<int>> t;
  auto * out_of_order = t.black(1, t.red(2), t.red(3));
  auto * single_black_child = t.black(8, t.black(4));

  {
    HtdRbTree<int> tree;
    tree.getRoot() = out_of_order;
    EXPECT_TRUE(tree.verifyRedBlack());
    EXPECT_FALSE(tree.verify());
    tree.getRoot() = single_black_child;
    EXPECT_FALSE(tree.verifyRedBlack());
    tree.getRoot() = RbNode<int>::NullPtr;
    EXPECT_TRUE(tree.verifyRedBlack());
  }
  {
    TdRbTree<int> tree;
    tree.getRoot() = out_of_order;
    EXPECT_TRUE(tree.verifyRedBlack());
    EXPECT_FALSE(tree.verify());
    tree.getRoot() = single_black_child;
    EXPECT_FALSE(tree.verifyRedBlack());
    tree.getRoot() = RbNode<int>::NullPtr;
    EXPECT_TRUE(tree.verifyRedBlack());
  }
}

// ---------------------------------------------------------------------------
// AVL trees built by hand
// ---------------------------------------------------------------------------

TYPED_TEST(AvlNodes, AcceptsAValidTree)
{
  auto & t = this->t;
  auto * root = this->finished(t.node(8,
                                      t.node(4, t.node(2, t.node(1)), t.node(6)),
                                      t.node(12, nullptr, t.node(14))));

  EXPECT_TRUE(is_avl_of(root));
  EXPECT_TRUE(is_avl_of(TypeParam::NullPtr));
  EXPECT_TRUE(is_avl_of(this->finished(t.node(1))));
}

TYPED_TEST(AvlNodes, RejectsAStoredFactorThatIsNotTheRealOneBelowTheRoot)
{
  auto & t = this->t;
  auto * deep = t.node(2, t.node(1));
  auto * root = this->finished(t.node(8,
                                      t.node(4, deep, t.node(6)),
                                      t.node(12, nullptr, t.node(14))));
  ASSERT_TRUE(is_avl_of(root));
  ASSERT_EQ(DIFF(deep), -1);

  for (const int wrong : {-2, 0, 1, 2})
    {
      DIFF(deep) = static_cast<signed char>(wrong);
      EXPECT_FALSE(is_avl_of(root)) << "factor " << wrong;
    }

  DIFF(deep) = -1;
  EXPECT_TRUE(is_avl_of(root));
}

// The node of key 12 has a right subtree two levels taller than its left
// one, whatever its factor claims.
TYPED_TEST(AvlNodes, RejectsSubtreesWhoseHeightsDifferByTwo)
{
  auto & t = this->t;
  auto * leaning = t.node(12, nullptr, t.node(14, nullptr, t.node(15)));
  auto * root = this->finished(t.node(8,
                                      t.node(4, t.node(2, t.node(1)), t.node(6)),
                                      leaning));
  ASSERT_EQ(DIFF(leaning), 2);
  EXPECT_FALSE(is_avl_of(root));

  for (const int claimed : {-1, 0, 1})
    {
      DIFF(leaning) = static_cast<signed char>(claimed);
      EXPECT_FALSE(is_avl_of(root)) << "factor " << claimed;
    }
}

TEST(AvlRankNodes, RejectsAWrongCounterBelowTheRoot)
{
  Hand_Tree<AvlNodeRk<int>> t;
  auto * deep = t.node(2, t.node(1));
  auto * root = t.node(8, t.node(4, deep, t.node(6)), t.node(12, nullptr, t.node(14)));
  set_balance_factors(root);
  set_counters(root);
  ASSERT_TRUE(is_avl_rk(root));

  --deep->getCount();
  EXPECT_FALSE(is_avl_rk(root));
  EXPECT_FALSE(check_rank_tree(root));
}

// Balanced, and its factors are right, but 2 is at the left of 1.
TEST(AvlTreesVerify, RejectKeysOutOfOrder)
{
  {
    Hand_Tree<AvlNode<int>> t;
    auto * root = t.node(1, t.node(2), t.node(3));
    ASSERT_TRUE(is_avl(root));
    EXPECT_FALSE(verified_by<Avl_Tree<int>>(root));
    EXPECT_TRUE(verified_by<Avl_Tree<int>>(t.node(2, t.node(1), t.node(3))));
  }
  {
    Hand_Tree<AvlNodeRk<int>> t;
    auto * root = counted(t.node(1, t.node(2), t.node(3)));
    ASSERT_TRUE(is_avl_rk(root));
    EXPECT_FALSE(verified_by<Avl_Tree_Rk<int>>(root));
    EXPECT_TRUE(verified_by<Avl_Tree_Rk<int>>(counted(t.node(2, t.node(1), t.node(3)))));
  }
}

// ---------------------------------------------------------------------------
// Treaps and ranked trees built by hand
// ---------------------------------------------------------------------------

TEST(TreapNodes, IsTreapChecksTheHeapOrderAtEveryNode)
{
  Hand_Tree<TreapNode<int>> t;
  auto * deep = t.with_priority(30, 1);
  auto * root = t.with_priority(10, 8,
                                t.with_priority(20, 4, t.with_priority(25, 2, deep)),
                                t.with_priority(15, 12));
  EXPECT_TRUE(is_treap(root));
  EXPECT_TRUE(is_treap(TreapNode<int>::NullPtr));

  PRIO(deep) = 24; // less than the priority of its parent
  EXPECT_FALSE(is_treap(root));

  PRIO(deep) = 25; // equal priorities keep the heap order
  EXPECT_TRUE(is_treap(root));
}

// A heap on the priorities, but 2 is at the left of 1.
TEST(TreapsVerify, RejectKeysOutOfOrder)
{
  {
    Hand_Tree<TreapNode<int>> t;
    auto * root = t.with_priority(1, 1, t.with_priority(2, 2), t.with_priority(3, 3));
    ASSERT_TRUE(is_treap(root));
    EXPECT_FALSE(verified_by<Treap<int>>(root));
    EXPECT_TRUE(verified_by<Treap<int>>(t.with_priority(1, 2, t.with_priority(2, 1),
                                                         t.with_priority(3, 3))));
  }
  {
    Hand_Tree<Treap_Rk_Node<int>> t;
    auto * root = counted(t.with_priority(1, 1, t.with_priority(2, 2), t.with_priority(3, 3)));
    ASSERT_TRUE(is_treap(root));
    EXPECT_FALSE(verified_by<Treap_Rk<int>>(root));
    EXPECT_TRUE(verified_by<Treap_Rk<int>>(counted(t.with_priority(1, 2, t.with_priority(2, 1),
                                                                    t.with_priority(3, 3)))));
  }
}

TEST(TreapsVerify, TreapRkRejectsAWrongCounter)
{
  Hand_Tree<Treap_Rk_Node<int>> t;
  auto * leaf = t.with_priority(3, 1);
  auto * root = counted(t.with_priority(1, 4, t.with_priority(2, 2, leaf), t.with_priority(5, 6)));
  ASSERT_TRUE(verified_by<Treap_Rk<int>>(root));

  leaf->getCount() = 2;
  EXPECT_FALSE(verified_by<Treap_Rk<int>>(root));
}

TEST(RankedNodes, CheckRankTreeChecksEveryCounter)
{
  Hand_Tree<BinNodeXt<int>> t;
  auto * deep = t.node(2, t.node(1));
  auto * root = t.node(8, t.node(4, deep, t.node(6)), t.node(12));
  set_counters(root);
  EXPECT_TRUE(check_rank_tree(root));
  EXPECT_TRUE(check_rank_tree(BinNodeXt<int>::NullPtr));

  for (const size_t wrong : {size_t(0), size_t(1), size_t(3)})
    {
      COUNT(deep) = wrong;
      EXPECT_FALSE(check_rank_tree(root)) << "counter " << wrong;
    }

  COUNT(deep) = 2;
  EXPECT_TRUE(check_rank_tree(root));
}

TEST(RankedTreesVerify, RandTreeAndSplayTreeRkRejectKeysOutOfOrder)
{
  {
    Hand_Tree<Rand_Tree<int>::Node> t;
    EXPECT_FALSE(verified_by<Rand_Tree<int>>(counted(t.node(1, t.node(2), t.node(3)))));
    EXPECT_TRUE(verified_by<Rand_Tree<int>>(counted(t.node(2, t.node(1), t.node(3)))));
  }
  {
    Hand_Tree<Splay_Tree_Rk<int>::Node> t;
    EXPECT_FALSE(verified_by<Splay_Tree_Rk<int>>(counted(t.node(1, t.node(2), t.node(3)))));
    EXPECT_TRUE(verified_by<Splay_Tree_Rk<int>>(counted(t.node(2, t.node(1), t.node(3)))));
  }
}

// A key that is in order with its parent but not with an ancestor further
// up: 9 is a right child of 4, at the left of 8.
TEST(BstOrder, CheckBstComparesWithEveryAncestor)
{
  Hand_Tree<BinNode<int>> t;
  auto * stray = t.node(6);
  auto * root = t.node(8, t.node(4, t.node(2), stray), t.node(12));
  EXPECT_TRUE(check_bst(root));

  KEY(stray) = 9;
  EXPECT_FALSE(check_bst(root));
  EXPECT_FALSE(verified_by<BinTree<int>>(root));
  EXPECT_FALSE(verified_by<Splay_Tree<int>>(root));
}

TEST(BstOrder, CheckBstFollowsTheComparator)
{
  Hand_Tree<BinNode<int>> t;
  auto * descending = t.node(8, t.node(12), t.node(4));

  EXPECT_FALSE(check_bst(descending));
  EXPECT_TRUE(check_bst(descending, std::greater<int>()));
  EXPECT_TRUE(check_bst(t.node(5, t.node(5), t.node(5)))); // repeated keys
}

// ---------------------------------------------------------------------------
// One defect at a time in the trees that each type builds
// ---------------------------------------------------------------------------

TYPED_TEST(AnyTreeVerify, AcceptsTheEmptyTreeAndWhatInsertBuilds)
{
  EXPECT_TRUE(TypeParam().verify());

  Built_Tree<TypeParam> built;
  ASSERT_EQ(built.nodes.size(), size_t(built.num_keys));
  EXPECT_TRUE(built.tree.verify());
}

TYPED_TEST(AnyTreeVerify, RejectsTwoNeighborKeysSwappedAnywhere)
{
  Built_Tree<TypeParam> built;
  auto & nodes = built.nodes;
  ASSERT_TRUE(built.tree.verify());

  for (size_t i = 0; i + 1 < nodes.size(); ++i)
    {
      auto * p = nodes[i].first;
      auto * q = nodes[i + 1].first;
      std::swap(KEY(p), KEY(q));
      EXPECT_FALSE(built.tree.verify()) << "keys " << KEY(q) << " and " << KEY(p);
      std::swap(KEY(p), KEY(q));
    }

  EXPECT_TRUE(built.tree.verify());
}

TYPED_TEST(RankedTreeVerify, RejectsAWrongCounterAnywhere)
{
  Built_Tree<TypeParam> built;
  ASSERT_TRUE(built.tree.verify());

  for (auto & [p, parent] : built.nodes)
    {
      const size_t count = p->getCount();

      p->getCount() = count + 1;
      EXPECT_FALSE(built.tree.verify()) << "key " << KEY(p) << ", one more";

      p->getCount() = count - 1;
      EXPECT_FALSE(built.tree.verify()) << "key " << KEY(p) << ", one less";

      p->getCount() = count;
    }

  EXPECT_TRUE(built.tree.verify());
}

TYPED_TEST(RedBlackTreeVerify, RejectsARecoloredNodeAnywhere)
{
  Built_Tree<TypeParam> built;
  ASSERT_TRUE(built.tree.verify());

  size_t reds = 0, blacks = 0;
  for (auto & [p, parent] : built.nodes)
    {
      const auto color = COLOR(p);
      (color == RED ? reds : blacks)++;

      COLOR(p) = 2;
      EXPECT_FALSE(built.tree.verify()) << "key " << KEY(p) << ", no color";

      // Recoloring any node but the root changes the black nodes of the
      // paths through it and leaves the other paths as they were.
      if (parent != nullptr)
        {
          COLOR(p) = color == RED ? BLACK : RED;
          EXPECT_FALSE(built.tree.verify()) << "key " << KEY(p) << ", recolored";
        }

      COLOR(p) = color;
    }

  EXPECT_GT(reds, 0u);
  EXPECT_GT(blacks, 1u);
  EXPECT_TRUE(built.tree.verify());
}

TYPED_TEST(AvlTreeVerify, RejectsAWrongBalanceFactorAnywhere)
{
  Built_Tree<TypeParam> built;
  ASSERT_TRUE(built.tree.verify());

  for (auto & [p, parent] : built.nodes)
    {
      const signed char factor = DIFF(p);

      for (const int wrong : {-2, -1, 0, 1, 2})
        if (wrong != factor)
          {
            DIFF(p) = static_cast<signed char>(wrong);
            EXPECT_FALSE(built.tree.verify()) << "key " << KEY(p) << ", factor " << wrong;
          }

      DIFF(p) = factor;
    }

  EXPECT_TRUE(built.tree.verify());
}

TYPED_TEST(TreapVerify, RejectsAPriorityBelowTheOneOfItsParentAnywhere)
{
  Built_Tree<TypeParam> built;
  ASSERT_TRUE(built.tree.verify());

  size_t checked = 0;
  for (auto & [p, parent] : built.nodes)
    {
      if (parent == nullptr or PRIO(parent) == 0)
        continue;

      const unsigned long priority = PRIO(p);
      PRIO(p) = PRIO(parent) - 1;
      EXPECT_FALSE(built.tree.verify()) << "key " << KEY(p);
      PRIO(p) = priority;
      ++checked;
    }

  EXPECT_GE(checked + 2, built.nodes.size());
  EXPECT_TRUE(built.tree.verify());
}

// ---------------------------------------------------------------------------
// DynSetTree
// ---------------------------------------------------------------------------

namespace
{
  // The order of a set is checked with the comparator of the set.
  template <template <typename, class> class Tree>
  void check_set_in_descending_order()
  {
    DynSetTree<int, Tree, std::greater<int>> set;
    for (int i = 1; i <= 40; ++i)
      set.insert(17 * i % 41);

    EXPECT_TRUE(set.verify());
    EXPECT_EQ(set.size(), 40u);
    EXPECT_EQ(set.min(), 40);
    EXPECT_EQ(set.max(), 1);
  }
}

# define CHECK_SET_IN_DESCENDING_ORDER(Tree)    \
  {                                             \
    SCOPED_TRACE(#Tree);                        \
    check_set_in_descending_order<Tree>();      \
  }

TEST(DynSetTreeVerify, ChecksTheOrderWithTheComparatorOfTheSet)
{
  CHECK_SET_IN_DESCENDING_ORDER(BinTree);
  CHECK_SET_IN_DESCENDING_ORDER(Avl_Tree);
  CHECK_SET_IN_DESCENDING_ORDER(Avl_Tree_Rk);
  CHECK_SET_IN_DESCENDING_ORDER(Rb_Tree);
  CHECK_SET_IN_DESCENDING_ORDER(Rb_Tree_Rk);
  CHECK_SET_IN_DESCENDING_ORDER(TdRbTree);
  CHECK_SET_IN_DESCENDING_ORDER(TdRbTreeRk);
  CHECK_SET_IN_DESCENDING_ORDER(HtdRbTree);
  CHECK_SET_IN_DESCENDING_ORDER(HtdRbTreeRk);
  CHECK_SET_IN_DESCENDING_ORDER(Treap);
  CHECK_SET_IN_DESCENDING_ORDER(Treap_Rk);
  CHECK_SET_IN_DESCENDING_ORDER(Rand_Tree);
  CHECK_SET_IN_DESCENDING_ORDER(Splay_Tree);
  CHECK_SET_IN_DESCENDING_ORDER(Splay_Tree_Rk);
}

// ---------------------------------------------------------------------------
// A comparator that throws
// ---------------------------------------------------------------------------

namespace
{
  struct Comparator_Error {};

  // Compares as less<int> does, until it is told to fail.
  struct Fallible_Less
  {
    static inline bool fails = false;

    bool operator ()(const int a, const int b) const
    {
      if (fails)
        throw Comparator_Error();
      return a < b;
    }
  };

  // While one lives, Fallible_Less throws.
  struct Failing_Comparisons
  {
    Failing_Comparisons() noexcept { Fallible_Less::fails = true; }
    ~Failing_Comparisons() { Fallible_Less::fails = false; }
  };

  using Trees_With_A_Fallible_Comparator = ::testing::Types<
    BinTree<int, Fallible_Less>, Avl_Tree<int, Fallible_Less>,
    Avl_Tree_Rk<int, Fallible_Less>, Rb_Tree<int, Fallible_Less>,
    Rb_Tree_Rk<int, Fallible_Less>, TdRbTree<int, Fallible_Less>,
    TdRbTreeRk<int, Fallible_Less>, HtdRbTree<int, Fallible_Less>,
    HtdRbTreeRk<int, Fallible_Less>, Treap<int, Fallible_Less>,
    Treap_Rk<int, Fallible_Less>, Rand_Tree<int, Fallible_Less>,
    Splay_Tree<int, Fallible_Less>, Splay_Tree_Rk<int, Fallible_Less>>;

  // The names of those trees, in the same order.
  struct Fallible_Tree_Names
  {
    template <class Tree>
    static std::string GetName(const int i)
    {
      static constexpr const char * names[] = {
        "BinTree", "Avl_Tree", "Avl_Tree_Rk", "Rb_Tree", "Rb_Tree_Rk",
        "TdRbTree", "TdRbTreeRk", "HtdRbTree", "HtdRbTreeRk", "Treap",
        "Treap_Rk", "Rand_Tree", "Splay_Tree", "Splay_Tree_Rk"};
      return names[i];
    }
  };
}

template <class Tree>
struct VerifyWithAFallibleComparator : ::testing::Test {};

TYPED_TEST_SUITE(VerifyWithAFallibleComparator, Trees_With_A_Fallible_Comparator,
                 Fallible_Tree_Names);

// verify() compares the keys, so what the comparator throws has to reach the
// caller: a noexcept verify() would end the program instead.
TYPED_TEST(VerifyWithAFallibleComparator, LetsTheExceptionThrough)
{
  using Tree = TypeParam;
  static_assert(not noexcept(std::declval<const Tree &>().verify()));

  Node_Pool<typename Tree::Node> pool;
  Tree tree;
  for (const int key : {4, 2, 6, 1, 3, 5, 7})
    ASSERT_NE(tree.insert(pool.make(key)), nullptr);
  ASSERT_TRUE(tree.verify());

  {
    const Failing_Comparisons failing;
    EXPECT_THROW((void) tree.verify(), Comparator_Error);
  }

  EXPECT_TRUE(tree.verify());
}
