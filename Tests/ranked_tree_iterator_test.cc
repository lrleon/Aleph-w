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

/** @file ranked_tree_iterator_test.cc
 *  @brief The iterators of the ranked trees that keep an inorder position,
 *         and find its node with select() when they are asked for it.
 *
 *  A position outside the tree has no node. The iterator gets there with
 *  reset_to_pos() or with the constructor that takes a position, past the
 *  end with next_ne(), or when the tree loses keys under it; get_curr(),
 *  which is noexcept, called select() on it, which throws, and the program
 *  ended. A new tree whose iterator keeps a position is one more type in
 *  Positional_Iterator_Trees.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <string>

#include <tpl_array.H>

#include <tpl_dynSetTree.H>

#include "search_trees_test_common.H"

using namespace search_trees_test;

namespace
{
  /// The trees whose iterator keeps an inorder position.
  using Positional_Iterator_Trees =
    ::testing::Types<Avl_Tree_Rk<int>, Rb_Tree_Rk<int>, Treap_Rk<int>>;

  template <class Tree>
  class RankedTreeIterator : public ::testing::Test
  {
  protected:
    Tree tree;
    Node_Pool<typename Tree::Node> pool;

    // The keys 1, 3, 4, 5 and 8, at the positions 0 to 4
    void SetUp() override
    {
      for (const int key : {5, 3, 8, 1, 4})
        ASSERT_NE(tree.insert(pool.make(key)), nullptr);
    }
  };

  TYPED_TEST_SUITE(RankedTreeIterator, Positional_Iterator_Trees, Tree_Names);

  /// The keys from the position of it to the end.
  template <class Iterator>
  Array<int> keys_from(Iterator it)
  {
    Array<int> keys;
    for (; it.has_curr(); it.next_ne())
      keys.append(KEY(it.get_curr()));
    return keys;
  }

  /// The keys of a set, through its iterator and through a range for.
  template <template <typename, class> class Tree>
  void expect_the_set_iterates_in_order()
  {
    DynSetTree<int, Tree> set;
    for (const int key : {5, 3, 8, 1, 4})
      set.insert(key);

    Array<int> keys;
    for (auto it = set.get_it(); it.has_curr(); it.next_ne())
      keys.append(it.get_curr());
    EXPECT_EQ(keys, (Array<int>{1, 3, 4, 5, 8}));

    keys.clear();
    for (const int key : set)
      keys.append(key);
    EXPECT_EQ(keys, (Array<int>{1, 3, 4, 5, 8}));
  }
}

// The iterator of Avl_Tree_Rk and Rb_Tree_Rk, BinTreeXt_Iterator, named
// BinTreeXt_Operation before its header declared it: has_curr() did not
// compile, so that those trees, and their sets, could not be iterated
TYPED_TEST(RankedTreeIterator, IteratesInOrderWithThePositions)
{
  const Array<int> expected = {1, 3, 4, 5, 8};
  typename TypeParam::Iterator it(this->tree);
  size_t pos = 0;
  for (; it.has_curr(); it.next_ne(), ++pos)
    {
      ASSERT_LT(pos, expected.size());
      EXPECT_EQ(KEY(it.get_curr()), expected[pos]);
      EXPECT_EQ(it.get_pos(), pos);
    }
  EXPECT_EQ(pos, expected.size());
  EXPECT_EQ(it.get_curr(), nullptr);
}

TEST(RankedTreeIteratorSet, TheSetsOfTheseTreesIterateInOrder)
{
  expect_the_set_iterates_in_order<Avl_Tree_Rk>();
  expect_the_set_iterates_in_order<Rb_Tree_Rk>();
  expect_the_set_iterates_in_order<Treap_Rk>();
}

// Every way of reaching a position without node: get_curr() threw
// out_of_range from select() inside noexcept, and the program ended
TYPED_TEST(RankedTreeIterator, APositionPastTheEndIsTheEnd)
{
  using Iterator = typename TypeParam::Iterator;
  auto expect_the_end = [](const Iterator & it, const char * how)
  {
    EXPECT_FALSE(it.has_curr()) << how;
    EXPECT_EQ(it.get_curr(), nullptr) << how;
    EXPECT_EQ(it.get_curr_ne(), nullptr) << how;
  };

  Iterator it(this->tree);
  it.reset_to_pos(5);
  expect_the_end(it, "reset_to_pos(size())");
  it.reset_to_pos(7);
  expect_the_end(it, "reset_to_pos() past the end");
  expect_the_end(Iterator(this->tree, size_t{7}), "a constructor with a position past the end");

  it.reset_last();
  it.next_ne();
  it.next_ne();
  expect_the_end(it, "next_ne() at the end");

  it.reset_last(); // on the key 8, at the position 4
  ASSERT_NE(this->tree.remove(1), nullptr);
  ASSERT_NE(this->tree.remove(3), nullptr);
  expect_the_end(it, "keys removed from the tree");
  EXPECT_TRUE(this->tree.verify());
}

// reset_to_node() of Treap_Rk marked the position as that of an empty tree:
// has_curr() was false on a node of the tree
TYPED_TEST(RankedTreeIterator, ResetToNodeStartsAtThatNode)
{
  typename TypeParam::Iterator it(this->tree);
  it.reset_to_node(this->tree.search(4));
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_pos(), 2u);
  EXPECT_EQ(keys_from(it), (Array<int>{4, 5, 8}));
}

// reset_to_key() moved the position but kept the node that get_curr() had
// found: get_curr() gave the node of the previous position
TYPED_TEST(RankedTreeIterator, ResetToKeyGoesToTheNodeOfTheKey)
{
  typename TypeParam::Iterator it(this->tree);
  ASSERT_EQ(KEY(it.get_curr()), 1);
  it.reset_to_key(5);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(KEY(it.get_curr()), 5);
  EXPECT_EQ(it.get_pos(), 3u);
  EXPECT_EQ(keys_from(it), (Array<int>{5, 8}));
}

// An iterator built by default has no tree: get_curr_ne() read the root of
// the tree that it does not have before it saw that it has no position
TYPED_TEST(RankedTreeIterator, AnIteratorBuiltByDefaultHasNoNode)
{
  typename TypeParam::Iterator it;
  EXPECT_FALSE(it.has_curr());
  EXPECT_EQ(it.get_curr_ne(), nullptr);
  EXPECT_EQ(it.get_curr(), nullptr);
}

// The position of a node that the iterator was put on is that of the node:
// among equal keys, the search by its key found another node, which the
// iterator took as its current one
TYPED_TEST(RankedTreeIterator, ResetToNodeKeepsTheNodeAmongEqualKeys)
{
  using Node = typename TypeParam::Node;
  Array<Node *> equal = {this->tree.search(4)};
  for (int i = 0; i < 4; ++i)
    equal.append(this->tree.insert_dup(this->pool.make(4)));

  Array<Node *> in_order;  // by position, without searching by key
  for (typename TypeParam::Iterator it(this->tree); it.has_curr(); it.next_ne())
    in_order.append(it.get_curr());
  ASSERT_EQ(in_order.size(), 9u);

  for (size_t i = 0; i < equal.size(); ++i)
    {
      SCOPED_TRACE("the node " + std::to_string(i) + " of the key 4");
      typename TypeParam::Iterator it(this->tree);
      it.reset_to_node(equal[i]);
      ASSERT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), equal[i]);
      const size_t pos = it.get_pos();
      ASSERT_LT(pos, in_order.size());
      EXPECT_EQ(in_order[pos], equal[i]);
    }
}

// A position past the end that an int cannot hold is the end too: kept in
// an int, it lost its high bits, and 2^32 + 2 was the position 2. Kept as
// the greatest int, next_ne() overflowed it, which the sanitizer of
// undefined behavior reports
TYPED_TEST(RankedTreeIterator, APositionThatAnIntCannotHoldIsTheEnd)
{
  if constexpr (sizeof(size_t) <= sizeof(int))
    GTEST_SKIP() << "a size_t holds no position that an int cannot";
  else
    {
      using Iterator = typename TypeParam::Iterator;
      const size_t far = size_t{std::numeric_limits<unsigned>::max()} + 3;
      const Iterator built(this->tree, far);
      EXPECT_FALSE(built.has_curr());
      EXPECT_EQ(built.get_curr(), nullptr);

      Iterator it(this->tree);
      it.reset_to_pos(far);
      EXPECT_FALSE(it.has_curr());
      EXPECT_EQ(it.get_curr(), nullptr);

      Iterator moved = built;
      for (int i = 0; i < 2; ++i)
        {
          it.next_ne();
          moved.next_ne();
        }
      EXPECT_FALSE(it.has_curr()) << "next_ne() at that end";
      EXPECT_EQ(it.get_curr(), nullptr);
      EXPECT_FALSE(moved.has_curr()) << "next_ne() at that end";
      EXPECT_EQ(moved.get_curr(), nullptr);

      it.reset_to_pos(size_t{std::numeric_limits<int>::max()});
      EXPECT_FALSE(it.has_curr()) << "the greatest int";
      EXPECT_EQ(it.get_curr(), nullptr);
      it.next_ne();
      EXPECT_FALSE(it.has_curr()) << "next_ne() at the greatest int";
    }
}
