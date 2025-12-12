#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_binNode.H>
#include <tpl_binNodeUtils.H>
#include <tpl_dynArray.H>

using namespace Aleph;
using namespace testing;

namespace
{
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
      for (auto * p : allocated)
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
  //    / \\
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
