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

/**
 * @file bin-node-utils.cc
 * @brief Tests for Bin Node Utils
 */

#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_binNode.H>
#include <tpl_binNodeUtils.H>
#include <tpl_dynArray.H>

using namespace Aleph;
using namespace testing;

namespace
{
  static std::vector<int> * g_visit_acc = nullptr;

  static void visit_push_key(BinNode<int> * p)
  {
    if (g_visit_acc != nullptr)
      g_visit_acc->push_back(KEY(p));
  }

  struct NodePool
  {
    std::vector<BinNode<int> *> allocated;

    BinNode<int> * make(int k)
    {
      auto * p = new BinNode<int>(k);
      allocated.push_back(p);
      return p;
    }
    ~NodePool()
    {
      for (const auto * p : allocated)
        delete p;
    }
  };

  std::vector<int> inorder(BinNode<int> * root)
  {
    std::vector<int> keys;
    Aleph::infix_for_each<BinNode<int>>(root, [&] (BinNode<int> * p) { keys.push_back(KEY(p)); });
    return keys;
  }

  void assert_bst_and_inorder_sorted(BinNode<int> * root)
  {
    ASSERT_TRUE((check_bst<BinNode<int>>(root)));
    auto keys = inorder(root);
    ASSERT_TRUE(std::is_sorted(keys.begin(), keys.end()));
  }

  void delete_tree(BinNode<int> * root) noexcept
  {
    if (root == BinNode<int>::NullPtr)
      return;
    delete_tree(LLINK(root));
    delete_tree(RLINK(root));
    delete root;
  }

  struct LoadIntKey
  {
    bool operator()(BinNode<int> * p, const char * str) const
    {
      if (str == nullptr)
        return false;
      p->get_key() = std::stoi(str);
      return true;
    }
  };

  struct GetIntKey
  {
    std::string operator()(BinNode<int> * p) const
    {
      return std::to_string(KEY(p));
    }
  };

  std::vector<BinNode<int> *> as_vector(const DynList<BinNode<int> *> & l)
  {
    std::vector<BinNode<int> *> v;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      v.push_back(it.get_curr_ne());
    return v;
  }

  std::vector<BinNode<int> *> as_vector(const DynDlist<BinNode<int> *> & l)
  {
    std::vector<BinNode<int> *> v;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      v.push_back(it.get_curr_ne());
    return v;
  }

  std::vector<int> keys_from_nodes(const std::vector<BinNode<int> *> & nodes)
  {
    std::vector<int> out;
    out.reserve(nodes.size());
    for (auto * p : nodes)
      out.push_back(KEY(p));
    return out;
  }

  std::vector<unsigned char> parse_uc_array(const std::string & s, const std::string & var_name)
  {
    const std::string needle = "const unsigned char " + var_name;
    auto pos = s.find(needle);
    if (pos == std::string::npos)
      return {};

    pos = s.find('{', pos);
    if (pos == std::string::npos)
      return {};

    auto end = s.find('}', pos);
    if (end == std::string::npos)
      return {};

    std::vector<unsigned char> bytes;
    std::string inside = s.substr(pos + 1, end - pos - 1);
    std::stringstream ss(inside);
    while (ss)
      {
        int val;
        if (!(ss >> val))
          break;
        bytes.push_back(static_cast<unsigned char>(val));
        while (ss && (ss.peek() == ',' || std::isspace(ss.peek())))
          ss.get();
      }
    return bytes;
  }
}

TEST(BinNodeUtils, PrefixInfixSuffixLists)
{
  NodePool pool;
  auto * root = pool.make(2);
  LLINK(root) = pool.make(1);
  RLINK(root) = pool.make(3);

  auto pre_nodes = Aleph::prefix(root);
  auto in_nodes = Aleph::infix(root);
  auto post_nodes = Aleph::suffix(root);

  EXPECT_EQ(keys_from_nodes(as_vector(pre_nodes)), (std::vector<int>{2, 1, 3}));
  EXPECT_EQ(keys_from_nodes(as_vector(in_nodes)), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(keys_from_nodes(as_vector(post_nodes)), (std::vector<int>{1, 3, 2}));
}

TEST(BinNodeUtils, ForEachTraversals)
{
  NodePool pool;
  auto * root = pool.make(2);
  LLINK(root) = pool.make(1);
  RLINK(root) = pool.make(3);

  std::vector<int> in, pre, post;
  Aleph::for_each_in_order<BinNode<int>>(root, [&] (BinNode<int> * p) { in.push_back(KEY(p)); });
  Aleph::for_each_preorder<BinNode<int>>(root, [&] (BinNode<int> * p) { pre.push_back(KEY(p)); });
  Aleph::for_each_postorder<BinNode<int>>(root, [&] (BinNode<int> * p) { post.push_back(KEY(p)); });

  EXPECT_EQ(in, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(pre, (std::vector<int>{2, 1, 3}));
  EXPECT_EQ(post, (std::vector<int>{1, 3, 2}));
}

TEST(BinNodeUtils, ComputeNodesInLevel)
{
  NodePool pool;
  auto * root = pool.make(4);
  LLINK(root) = pool.make(2);
  RLINK(root) = pool.make(6);
  LLINK(LLINK(root)) = pool.make(1);
  RLINK(LLINK(root)) = pool.make(3);
  LLINK(RLINK(root)) = pool.make(5);
  RLINK(RLINK(root)) = pool.make(7);

  auto l0 = compute_nodes_in_level(root, 0);
  auto l1 = compute_nodes_in_level(root, 1);
  auto l2 = compute_nodes_in_level(root, 2);

  EXPECT_EQ(keys_from_nodes(as_vector(l0)), (std::vector<int>{4}));
  EXPECT_EQ(keys_from_nodes(as_vector(l1)), (std::vector<int>{2, 6}));
  EXPECT_EQ(keys_from_nodes(as_vector(l2)), (std::vector<int>{1, 3, 5, 7}));
}

TEST(BinNodeUtils, InternalPathLength)
{
  NodePool pool;
  auto * root = pool.make(2);
  LLINK(root) = pool.make(1);
  RLINK(root) = pool.make(3);

  EXPECT_EQ(internal_path_length(root), 0u + 1u + 1u);
}

TEST(BinNodeUtils, BuildTreeFromPreorderAndInorder)
{
  DynArray<int> pre(7);
  pre[0] = 4; pre[1] = 2; pre[2] = 1; pre[3] = 3; pre[4] = 6; pre[5] = 5; pre[6] = 7;
  DynArray<int> in(7);
  in[0] = 1; in[1] = 2; in[2] = 3; in[3] = 4; in[4] = 5; in[5] = 6; in[6] = 7;

  auto * root = build_tree<BinNode, int>(pre, 0, 6, in, 0, 6);
  EXPECT_EQ(inorder(root), (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
  EXPECT_TRUE(check_bst<BinNode<int>>(root));
  delete_tree(root);
}

TEST(BinNodeUtils, BuildTreeFromPostorderAndInorder)
{
  DynArray<int> post(7);
  post[0] = 1; post[1] = 3; post[2] = 2; post[3] = 5; post[4] = 7; post[5] = 6; post[6] = 4;
  DynArray<int> in(7);
  in[0] = 1; in[1] = 2; in[2] = 3; in[3] = 4; in[4] = 5; in[5] = 6; in[6] = 7;

  auto * root = build_postorder<BinNode, int>(post, 0, 6, in, 0, 6);
  EXPECT_EQ(inorder(root), (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
  EXPECT_TRUE(check_bst<BinNode<int>>(root));
  delete_tree(root);
}

TEST(BinNodeUtils, SaveTreeLoadTreeRoundtrip)
{
  NodePool pool;
  auto * root = pool.make(2);
  LLINK(root) = pool.make(1);
  RLINK(root) = pool.make(3);

  std::stringstream ss;
  save_tree(root, ss);
  auto * loaded = load_tree<BinNode<int>>(ss);

  EXPECT_EQ(inorder(loaded), (std::vector<int>{1, 2, 3}));
  EXPECT_TRUE(check_bst<BinNode<int>>(loaded));
  delete_tree(loaded);
}

TEST(BinNodeUtils, SaveTreeInArrayOfCharsLoadTreeFromArrayRoundtrip)
{
  NodePool pool;
  auto * root = pool.make(2);
  LLINK(root) = pool.make(1);
  RLINK(root) = pool.make(3);

  std::ostringstream out;
  const std::string name = "t";
  save_tree_in_array_of_chars<BinNode<int>, GetIntKey>(root, name, out);

  const std::string gen = out.str();

  auto bytes = parse_uc_array(gen, name + "_cdp");
  ASSERT_FALSE(bytes.empty());

  // parse quoted keys from: const char * t_k[] = { "..", ..., nullptr };
  const std::string key_var = name + "_k";
  const std::string needle = "const char * " + key_var + "[]";
  auto pos = gen.find(needle);
  ASSERT_NE(pos, std::string::npos);
  pos = gen.find('{', pos);
  ASSERT_NE(pos, std::string::npos);
  auto end = gen.find("};", pos);
  ASSERT_NE(end, std::string::npos);
  std::string inside = gen.substr(pos + 1, end - pos - 1);

  std::vector<std::string> storage;
  std::vector<const char *> keys;

  size_t i = 0;
  while (i < inside.size())
    {
      while (i < inside.size() && (std::isspace(static_cast<unsigned char>(inside[i])) || inside[i] == ','))
        ++i;
      if (i >= inside.size())
        break;

      if (inside.compare(i, 7, "nullptr") == 0)
        {
          i += 7;
          break;
        }

      ASSERT_EQ(inside[i], '"');
      ++i;
      std::string token;
      while (i < inside.size() && inside[i] != '"')
        {
          if (inside[i] == '\\' && i + 1 < inside.size())
            {
              char esc = inside[i + 1];
              if (esc == 'n')
                token.push_back('\n');
              else if (esc == 't')
                token.push_back('\t');
              else
                token.push_back(esc);
              i += 2;
              continue;
            }
          token.push_back(inside[i++]);
        }
      ASSERT_LT(i, inside.size());
      ASSERT_EQ(inside[i], '"');
      ++i;
      storage.push_back(token);
    }

  for (auto & s : storage)
    keys.push_back(s.c_str());
  keys.push_back(nullptr);

  auto * rebuilt = load_tree_from_array<BinNode<int>, LoadIntKey>(bytes.data(), tree_to_bits(root).size(), keys.data());
  EXPECT_EQ(inorder(rebuilt), (std::vector<int>{1, 2, 3}));
  EXPECT_TRUE(check_bst<BinNode<int>>(rebuilt));
  delete_tree(rebuilt);
}

TEST(BinNodeUtils, ThreadedTraversalsDoNotCorruptTree)
{
  NodePool pool;
  auto * root = pool.make(4);
  LLINK(root) = pool.make(2);
  RLINK(root) = pool.make(6);
  LLINK(LLINK(root)) = pool.make(1);
  RLINK(LLINK(root)) = pool.make(3);
  LLINK(RLINK(root)) = pool.make(5);
  RLINK(RLINK(root)) = pool.make(7);

  auto before_bits = tree_to_bits(root);
  auto before_in = inorder(root);

  std::vector<int> visited_in;
  g_visit_acc = &visited_in;
  inOrderThreaded(root, &visit_push_key);
  g_visit_acc = nullptr;
  EXPECT_EQ(visited_in, before_in);

  std::vector<int> visited_pre;
  g_visit_acc = &visited_pre;
  preOrderThreaded(root, &visit_push_key);
  g_visit_acc = nullptr;
  EXPECT_EQ(visited_pre, (std::vector<int>{4, 2, 1, 3, 6, 5, 7}));

  EXPECT_EQ(inorder(root), before_in);
  EXPECT_EQ(tree_to_bits(root).size(), before_bits.size());
  EXPECT_TRUE(check_bst<BinNode<int>>(root));
}

TEST(BinNodeUtils, InsertInBstRejectsDuplicates)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;

  ASSERT_NE(insert_in_bst(root, pool.make(2)), BinNode<int>::NullPtr);
  ASSERT_NE(insert_in_bst(root, pool.make(1)), BinNode<int>::NullPtr);
  ASSERT_NE(insert_in_bst(root, pool.make(3)), BinNode<int>::NullPtr);

  auto * dup = pool.make(2);
  EXPECT_EQ(insert_in_bst(root, dup), BinNode<int>::NullPtr);

  assert_bst_and_inorder_sorted(root);
}

TEST(BinNodeUtils, InsertDupInBstAllowsDuplicates)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;

  ASSERT_NE(insert_dup_in_bst(root, pool.make(2)), BinNode<int>::NullPtr);
  ASSERT_NE(insert_dup_in_bst(root, pool.make(2)), BinNode<int>::NullPtr);
  ASSERT_NE(insert_dup_in_bst(root, pool.make(2)), BinNode<int>::NullPtr);

  EXPECT_TRUE(check_bst<BinNode<int>>(root));
  EXPECT_EQ(inorder(root), (std::vector<int>{2, 2, 2}));
}

TEST(BinNodeUtils, SearchOrInsertInBst)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;

  auto * p = pool.make(5);
  EXPECT_EQ(search_or_insert_in_bst(root, p), p);

  auto * q = pool.make(5);
  auto * got = search_or_insert_in_bst(root, q);
  EXPECT_NE(got, q);
  EXPECT_EQ(KEY(got), 5);

  assert_bst_and_inorder_sorted(root);
}

TEST(BinNodeUtils, RemoveFromBstReturnsDetachedNode)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;

  for (int k : {3, 1, 4, 2})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  auto * removed = remove_from_bst(root, 1);
  ASSERT_NE(removed, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(removed), 1);
  EXPECT_EQ(LLINK(removed), BinNode<int>::NullPtr);
  EXPECT_EQ(RLINK(removed), BinNode<int>::NullPtr);

  assert_bst_and_inorder_sorted(root);

  ASSERT_NE(insert_in_bst(root, removed), BinNode<int>::NullPtr);
  assert_bst_and_inorder_sorted(root);
}

TEST(BinNodeUtils, JoinExclusiveEmptiesInputs)
{
  NodePool pool;
  BinNode<int> * a = BinNode<int>::NullPtr;
  BinNode<int> * b = BinNode<int>::NullPtr;

  for (int k : {1, 2, 3})
    ASSERT_NE(insert_in_bst(a, pool.make(k)), BinNode<int>::NullPtr);
  for (int k : {4, 5, 6})
    ASSERT_NE(insert_in_bst(b, pool.make(k)), BinNode<int>::NullPtr);

  auto * out = join_exclusive<BinNode<int>>(a, b);
  EXPECT_EQ(a, BinNode<int>::NullPtr);
  EXPECT_EQ(b, BinNode<int>::NullPtr);

  assert_bst_and_inorder_sorted(out);
  EXPECT_EQ(inorder(out), (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST(BinNodeUtils, SplitKeyRecOnlySplitsWhenKeyAbsent)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  BinNode<int> * l = BinNode<int>::NullPtr;
  BinNode<int> * r = BinNode<int>::NullPtr;

  EXPECT_FALSE(split_key_rec(root, 3, l, r));
  EXPECT_NE(root, BinNode<int>::NullPtr);
  EXPECT_EQ(l, BinNode<int>::NullPtr);
  EXPECT_EQ(r, BinNode<int>::NullPtr);

  EXPECT_TRUE(check_bst<BinNode<int>>(root));
  EXPECT_EQ(inorder(root), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(BinNodeUtils, SplitKeyDupRecSplitsAndEmptiesRoot)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  BinNode<int> * l = BinNode<int>::NullPtr;
  BinNode<int> * r = BinNode<int>::NullPtr;

  split_key_dup_rec(root, 3, l, r);
  EXPECT_EQ(root, BinNode<int>::NullPtr);

  assert_bst_and_inorder_sorted(l);
  assert_bst_and_inorder_sorted(r);

  EXPECT_EQ(inorder(l), (std::vector<int>{1, 2}));
  EXPECT_EQ(inorder(r), (std::vector<int>{3, 4, 5}));
}

TEST(BinNodeUtils, InfixIteratorTraversesInSortedOrder)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;

  for (int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  BinNodeInfixIterator<BinNode<int>> it(root);
  std::vector<int> got;
  while (it.has_curr())
    {
      got.push_back(KEY(it.get_curr_ne()));
      it.next_ne();
    }

  EXPECT_EQ(got, (std::vector<int>{2, 3, 4, 5, 6, 7, 8}));
}

TEST(BinNodeUtils, RotationsPreserveInorder)
{
  NodePool pool;

  auto * p = pool.make(2);
  LLINK(p) = pool.make(1);
  RLINK(p) = pool.make(3);

  auto before = inorder(p);
  auto * q = rotate_to_right(p);
  auto after = inorder(q);
  EXPECT_EQ(before, after);

  auto * r = rotate_to_left(q);
  auto after2 = inorder(r);
  EXPECT_EQ(before, after2);
}

TEST(BinNodeUtils, FindMinMax)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  EXPECT_EQ(KEY(find_min(root)), 2);
  EXPECT_EQ(KEY(find_max(root)), 8);
}

TEST(BinNodeUtils, FindSuccessorAndPredecessor)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  auto * p = searchInBinTree(root, 5);
  ASSERT_NE(p, BinNode<int>::NullPtr);

  BinNode<int> * parent = BinNode<int>::NullPtr;
  auto * succ = find_successor(p, parent);
  ASSERT_NE(succ, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(succ), 6);

  auto * q = searchInBinTree(root, 5);
  BinNode<int> * parent2 = BinNode<int>::NullPtr;
  auto * pred = find_predecessor(q, parent2);
  ASSERT_NE(pred, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(pred), 4);
}

TEST(BinNodeUtils, SearchParentReturnsNodeAndUpdatesParent)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  BinNode<int> head;
  head.reset();
  RLINK(&head) = root;

  BinNode<int> * parent = &head;
  auto * got = search_parent(root, 4, parent);
  ASSERT_NE(got, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(got), 4);
  EXPECT_NE(parent, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(parent), 3);
}

TEST(BinNodeUtils, SearchRankParent)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {5, 3, 7, 2, 4, 6, 8})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  auto * p1 = search_rank_parent(root, 6);
  ASSERT_NE(p1, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(p1), 6);

  auto * p2 = search_rank_parent(root, 1);
  ASSERT_NE(p2, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(p2), 2);
}

TEST(BinNodeUtils, InsertRootRequiresKeyAbsent)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {2, 1, 3})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  auto * existing = pool.make(2);
  EXPECT_EQ(insert_root(root, existing), BinNode<int>::NullPtr);

  auto * fresh = pool.make(4);
  ASSERT_NE(insert_root(root, fresh), BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(root), 4);
  assert_bst_and_inorder_sorted(root);
}

TEST(BinNodeUtils, InsertDupRootAlwaysInserts)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (int k : {2, 1, 3})
    ASSERT_NE(insert_in_bst(root, pool.make(k)), BinNode<int>::NullPtr);

  auto * dup = pool.make(2);
  ASSERT_NE(insert_dup_root(root, dup), BinNode<int>::NullPtr);
  assert_bst_and_inorder_sorted(root);
}

TEST(BinNodeUtils, RemoveFromBstRespectsComparator)
{
  struct Greater
  {
    bool operator()(int a, int b) const noexcept { return a > b; }
  };

  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;
  for (const int k : {1, 2, 3, 4, 5})
    {
      auto * inserted = insert_in_bst<BinNode<int>, Greater>(root, pool.make(k), Greater());
      ASSERT_NE(inserted, BinNode<int>::NullPtr);
    }

  auto * removed = remove_from_bst<BinNode<int>, Greater>(root, 4, Greater());
  ASSERT_NE(removed, BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(removed), 4);

  EXPECT_TRUE((check_bst<BinNode<int>, Greater>(root, Greater())));
}

TEST(BinNodeUtils, TreeToBitsAndBitsToTreeRoundtripShapeAndKeys)
{
  NodePool pool;

  // Build a small explicit shape:
  //     2
  //    /  right
  //   1   3
  auto * root = pool.make(2);
  LLINK(root) = pool.make(1);
  RLINK(root) = pool.make(3);

  BitArray bits;
  tree_to_bits(root, bits);

  int idx = 0;
  BinNode<int> * rebuilt = bits_to_tree<BinNode<int>>(bits, idx);

  const char * keys[] = {"2", "1", "3", nullptr};
  int key_idx = 0;
  load_tree_keys_from_array<BinNode<int>, LoadIntKey>(rebuilt, keys, key_idx);

  EXPECT_EQ(inorder(rebuilt), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(tree_to_bits(rebuilt).size(), bits.size());

  delete_tree(rebuilt);
}

TEST(BinNodeUtils, PreorderToBstBuildsValidTree)
{
  DynArray<int> pre(7);
  pre[0] = 5;
  pre[1] = 3;
  pre[2] = 2;
  pre[3] = 4;
  pre[4] = 7;
  pre[5] = 6;
  pre[6] = 8;

  BinNode<int> * root = preorder_to_bst<BinNode<int>>(pre, 0, 6);
  assert_bst_and_inorder_sorted(root);
  EXPECT_EQ(inorder(root), (std::vector<int>{2, 3, 4, 5, 6, 7, 8}));

  delete_tree(root);
}

TEST(BinNodeUtils, Property_InsertRemoveRandom_StableInvariants)
{
  NodePool pool;
  BinNode<int> * root = BinNode<int>::NullPtr;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 200);

  std::set<int> present;

  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto * p = pool.make(k);
      auto * ins = insert_in_bst(root, p);
      if (ins != BinNode<int>::NullPtr)
        present.insert(k);
    }

  assert_bst_and_inorder_sorted(root);

  for (int i = 0; i < 100; ++i)
    {
      int k = dist(rng);
      auto * removed = remove_from_bst(root, k);
      if (removed != BinNode<int>::NullPtr)
        {
          EXPECT_EQ(KEY(removed), k);
          present.erase(k);
        }
      assert_bst_and_inorder_sorted(root);
    }
}
