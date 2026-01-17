
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
 * @file bin-node-xt.cc
 * @brief Tests for Bin Node Xt
 */

#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_binNodeXt.H>
#include <tpl_binNodeUtils.H>

using namespace Aleph;
using namespace testing;

namespace
{
  using Node = BinNodeXt<int>;

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
    Aleph::infix_for_each<Node>(root, [&] (Node * p) { keys.push_back(KEY(p)); });
    return keys;
  }

  void assert_rank_bst(Node * root)
  {
    ASSERT_TRUE(check_rank_tree(root));
    ASSERT_TRUE((check_bst<Node>(root)));
    if (root != Node::NullPtr)
      ASSERT_EQ(COUNT(root), inorder_keys(root).size());
  }

  void assert_rank_only(Node * root)
  {
    ASSERT_TRUE(check_rank_tree(root));
    if (root != Node::NullPtr)
      ASSERT_EQ(COUNT(root), inorder_keys(root).size());
  }

  struct Greater
  {
    bool operator()(int a, int b) const noexcept { return a > b; }
  };
}

TEST(BinNodeXt, SplitKeyRecXtSplitsAndEmptiesRootWhenKeyAbsent)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (int k : {1, 2, 4, 5})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  Node * l = Node::NullPtr;
  Node * r = Node::NullPtr;
  EXPECT_TRUE(split_key_rec_xt(root, 3, l, r));
  EXPECT_EQ(root, Node::NullPtr);

  assert_rank_bst(l);
  assert_rank_bst(r);
  EXPECT_EQ(inorder_keys(l), (std::vector<int>{1, 2}));
  EXPECT_EQ(inorder_keys(r), (std::vector<int>{4, 5}));
}

TEST(BinNodeXt, SplitKeyRecXtReturnsFalseWhenKeyPresentAndDoesNotCorrupt)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 3, 4, 5})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  Node * l = Node::NullPtr;
  Node * r = Node::NullPtr;
  EXPECT_FALSE(split_key_rec_xt(root, 3, l, r));

  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(BinNodeXt, SplitKeyDupRecXtSplitsAndEmptiesRoot)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 2, 3, 4})
    ASSERT_NE(insert_dup_by_key_xt(root, pool.make(k)), Node::NullPtr);

  Node * l = Node::NullPtr;
  Node * r = Node::NullPtr;
  split_key_dup_rec_xt(root, 2, l, r);
  EXPECT_EQ(root, Node::NullPtr);

  assert_rank_bst(l);
  assert_rank_bst(r);
  EXPECT_EQ(inorder_keys(l), (std::vector<int>{1, 2, 2}));
  EXPECT_EQ(inorder_keys(r), (std::vector<int>{3, 4}));
}

TEST(BinNodeXt, InsertRootXtMakesInsertedNodeRoot)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {2, 1, 3})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  auto * p = pool.make(4);
  ASSERT_NE(insert_root_xt(root, p), Node::NullPtr);
  EXPECT_EQ(root, p);
  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{1, 2, 3, 4}));

  auto * dup = pool.make(2);
  EXPECT_EQ(insert_root_xt(root, dup), Node::NullPtr);
  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{1, 2, 3, 4}));
}

TEST(BinNodeXt, InsertDupRootXtAllowsDuplicatesAndBecomesRoot)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 3})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  auto * p = pool.make(2);
  ASSERT_NE(insert_dup_root_xt(root, p), Node::NullPtr);
  EXPECT_EQ(root, p);
  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{1, 2, 2, 3}));
}

TEST(BinNodeXt, InsertByPosXtMaintainsRankButMayBreakBst)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 3, 4, 5})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  auto * p0 = pool.make(99);
  insert_by_pos_xt(root, p0, 0);
  assert_rank_only(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{99, 1, 2, 3, 4, 5}));
  EXPECT_EQ(KEY(select(root, 0)), 99);
  EXPECT_EQ(KEY(select(root, 1)), 1);

  auto expected = std::vector<int>{99, 1, 2, 3, 4, 5};
  auto * p3 = pool.make(77);
  insert_by_pos_xt(root, p3, 3);
  expected.insert(expected.begin() + 3, 77);
  assert_rank_only(root);
  EXPECT_EQ(inorder_keys(root), expected);

  // out_of_range when pos > COUNT(root)
  auto * bad = pool.make(123);
  EXPECT_THROW(insert_by_pos_xt(root, bad, COUNT(root) + 1), std::out_of_range);
  assert_rank_only(root);
}

TEST(BinNodeXt, SearchOrInsertRootRecXtUsagePattern)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 3})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  auto * p = pool.make(0);
  auto * ret = search_or_insert_root_rec_xt<Node, int>(root, p);
  if (ret == p)
    root = p;

  EXPECT_EQ(root, p);
  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{0, 1, 2, 3}));

  auto * q = pool.make(2);
  auto * ret2 = search_or_insert_root_rec_xt<Node, int>(root, q);
  EXPECT_NE(ret2, q);
  EXPECT_EQ(KEY(ret2), 2);
  // root should remain as-is (only update when ret == inserted node)
  EXPECT_EQ(root, p);
  assert_rank_bst(root);
}

TEST(BinNodeXt, ComparatorGreaterInsertFindRemove)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  Greater cmp;

  for (const int k : {1, 2, 3, 4, 5})
    ASSERT_NE((insert_by_key_xt<Node, Greater>(root, pool.make(k), cmp)), Node::NullPtr);

  ASSERT_TRUE(check_rank_tree(root));
  ASSERT_TRUE((check_bst<Node, Greater>(root, cmp)));
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{5, 4, 3, 2, 1}));

  Node * p = Node::NullPtr;
  const auto pos = find_position<Node, Greater>(root, 3, p, cmp);
  EXPECT_EQ(pos, 2);
  EXPECT_EQ(KEY(p), 3);

  auto * removed = remove_by_key_xt<Node, Greater>(root, 4, cmp);
  ASSERT_NE(removed, Node::NullPtr);
  pool.forget(removed);
  delete removed;
  ASSERT_TRUE(check_rank_tree(root));
  ASSERT_TRUE((check_bst<Node, Greater>(root, cmp)));
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{5, 3, 2, 1}));
}

TEST(BinNodeXt, OutOfRangeExceptions)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (int k : {1, 2, 3})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  EXPECT_THROW(select(root, COUNT(root)), std::out_of_range);
  EXPECT_THROW(select_rec(root, COUNT(root)), std::out_of_range);
  EXPECT_THROW(remove_by_pos_xt(root, COUNT(root)), std::out_of_range);

  Node * ts = Node::NullPtr;
  Node * tg = Node::NullPtr;
  EXPECT_THROW(split_pos_rec(root, COUNT(root) + 1, ts, tg), std::out_of_range);
}

TEST(BinNodeXt, SentinelCountIsZero)
{
  EXPECT_EQ(COUNT(Node::NullPtr), 0u);
}

TEST(BinNodeXt, InsertByKeyMaintainsCountsAndBst)
{
  NodePool pool;
  Node * root = Node::NullPtr;

  for (const int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{2, 3, 4, 5, 6, 7, 8}));
  EXPECT_EQ(COUNT(root), 7u);
}

TEST(BinNodeXt, InsertRejectsDuplicates)
{
  NodePool pool;
  Node * root = Node::NullPtr;

  ASSERT_NE(insert_by_key_xt(root, pool.make(2)), Node::NullPtr);
  ASSERT_NE(insert_by_key_xt(root, pool.make(1)), Node::NullPtr);
  ASSERT_NE(insert_by_key_xt(root, pool.make(3)), Node::NullPtr);

  auto * dup = pool.make(2);
  EXPECT_EQ(insert_by_key_xt(root, dup), Node::NullPtr);

  assert_rank_bst(root);
}

TEST(BinNodeXt, InsertDupAllowsDuplicatesAndCounts)
{
  NodePool pool;
  Node * root = Node::NullPtr;

  ASSERT_NE(insert_dup_by_key_xt(root, pool.make(2)), Node::NullPtr);
  ASSERT_NE(insert_dup_by_key_xt(root, pool.make(2)), Node::NullPtr);
  ASSERT_NE(insert_dup_by_key_xt(root, pool.make(2)), Node::NullPtr);

  assert_rank_bst(root);
  EXPECT_EQ(COUNT(root), 3u);
}

TEST(BinNodeXt, SearchOrInsertByKeyReturnsExistingOrInserted)
{
  NodePool pool;
  Node * root = Node::NullPtr;

  auto * p = pool.make(10);
  EXPECT_EQ(search_or_insert_by_key_xt(root, p), p);
  assert_rank_bst(root);
  EXPECT_EQ(COUNT(root), 1u);

  auto * q = pool.make(10);
  auto * got = search_or_insert_by_key_xt(root, q);
  EXPECT_NE(got, q);
  EXPECT_EQ(KEY(got), 10);
  assert_rank_bst(root);
  EXPECT_EQ(COUNT(root), 1u);
}

TEST(BinNodeXt, SelectRecAndSelectNeAndSelectParent)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  for (size_t i = 0; i < COUNT(root); ++i)
    {
      auto * a = select_rec(root, i);
      auto * b = select_ne(root, i);
      auto * c = select(root, i);
      EXPECT_EQ(a, b);
      EXPECT_EQ(a, c);
    }

  Node * parent = Node::NullPtr;
  auto * mid = select(root, 3, parent);
  ASSERT_NE(mid, Node::NullPtr);
  EXPECT_EQ(KEY(mid), 5);
  EXPECT_NE(parent, mid);
}

TEST(BinNodeXt, InorderPositionReturnsRankOrMinusOne)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  for (size_t i = 0; i < COUNT(root); ++i)
    {
      auto * p = select(root, i);
      Node * out = Node::NullPtr;
      EXPECT_EQ(inorder_position(root, KEY(p), out), static_cast<long>(i));
      EXPECT_EQ(out, p);
    }

  EXPECT_EQ(inorder_position(root, 42), -1);
}

TEST(BinNodeXt, FindPositionContract)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {2, 4, 6, 8})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  Node * p = Node::NullPtr;

  // key smaller than min
  EXPECT_EQ(find_position(root, 1, p), -1);
  EXPECT_EQ(KEY(p), 2);

  // exact key
  EXPECT_EQ(find_position(root, 6, p), 2);
  EXPECT_EQ(KEY(p), 6);

  // between keys
  EXPECT_EQ(find_position(root, 5, p), 1);
  EXPECT_EQ(KEY(p), 6);

  // bigger than max
  EXPECT_EQ(find_position(root, 9, p), 3);
  EXPECT_EQ(p, Node::NullPtr);
}

TEST(BinNodeXt, SplitPosRecAndJoinExclusiveRoundtrip)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 3, 4, 5, 6, 7})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  Node * l = Node::NullPtr;
  Node * r = Node::NullPtr;
  split_pos_rec(root, 3, l, r);
  EXPECT_EQ(root, Node::NullPtr);

  assert_rank_bst(l);
  assert_rank_bst(r);
  EXPECT_EQ(inorder_keys(l), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(inorder_keys(r), (std::vector<int>{4, 5, 6, 7}));

  Node * joined = join_exclusive_xt(l, r);
  EXPECT_EQ(l, Node::NullPtr);
  EXPECT_EQ(r, Node::NullPtr);

  assert_rank_bst(joined);
  EXPECT_EQ(inorder_keys(joined), (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
}

TEST(BinNodeXt, RemoveByPosAndRemoveByKey)
{
  NodePool pool;
  Node * root = Node::NullPtr;
  for (const int k : {1, 2, 3, 4, 5, 6, 7})
    ASSERT_NE(insert_by_key_xt(root, pool.make(k)), Node::NullPtr);

  auto * removed = remove_by_pos_xt(root, 3);
  ASSERT_NE(removed, Node::NullPtr);
  EXPECT_EQ(KEY(removed), 4);
  EXPECT_EQ(COUNT(removed), 1u);
  assert_rank_bst(root);

  pool.forget(removed);
  delete removed;

  auto * removed2 = remove_by_key_xt(root, 6);
  ASSERT_NE(removed2, Node::NullPtr);
  EXPECT_EQ(KEY(removed2), 6);
  pool.forget(removed2);
  delete removed2;

  assert_rank_bst(root);
  EXPECT_EQ(inorder_keys(root), (std::vector<int>{1, 2, 3, 5, 7}));
}

TEST(BinNodeXt, RotationsMaintainRankInvariant)
{
  NodePool pool;

  auto * p = pool.make(2);
  LLINK(p) = pool.make(1);
  RLINK(p) = pool.make(3);
  COUNT(LLINK(p)) = 1;
  COUNT(RLINK(p)) = 1;
  COUNT(p) = 3;

  const auto before = inorder_keys(p);
  auto * q = rotate_to_right_xt(p);
  assert_rank_bst(q);
  EXPECT_EQ(inorder_keys(q), before);

  auto * r = rotate_to_left_xt(q);
  assert_rank_bst(r);
  EXPECT_EQ(inorder_keys(r), before);
}

TEST(BinNodeXt, Property_RandomInsertRemove_CountsAndOrder)
{
  NodePool pool;
  Node * root = Node::NullPtr;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 400);

  std::set<int> oracle;

  for (int i = 0; i < 300; ++i)
    {
      int k = dist(rng);
      auto * p = pool.make(k);
      auto * ins = insert_by_key_xt(root, p);
      if (ins == Node::NullPtr)
        {
          // duplicate
          pool.forget(p);
          delete p;
        }
      else
        oracle.insert(k);

      assert_rank_bst(root);
      EXPECT_EQ(inorder_keys(root), std::vector<int>(oracle.begin(), oracle.end()));
    }

  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      if (auto * removed = remove_by_key_xt(root, k); removed != Node::NullPtr)
        {
          oracle.erase(k);
          pool.forget(removed);
          delete removed;
        }
      assert_rank_bst(root);
      EXPECT_EQ(inorder_keys(root), std::vector<int>(oracle.begin(), oracle.end()));
    }
}
