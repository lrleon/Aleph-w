#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_binTree.H>

using namespace Aleph;
using namespace testing;

namespace
{
  template <class Tree>
  struct NodePool
  {
    using Node = typename Tree::Node;

    std::vector<Node *> allocated;

    Node * make(const typename Node::Key_Type & key)
    {
      Node * p = new Node(key);
      allocated.push_back(p);
      return p;
    }

    ~NodePool()
    {
      for (Node * p : allocated)
        delete p;
    }
  };

  template <class Node>
  std::vector<typename Node::Key_Type> inorder_keys(Node * root)
  {
    std::vector<typename Node::Key_Type> keys;
    Aleph::infix_for_each<Node>(root, [&] (Node * p) { keys.push_back(KEY(p)); });
    return keys;
  }
}

TEST(BinTree, InsertSearchVerifyAndIteratorOrder)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;

  const std::vector<int> input{5, 3, 7, 2, 4, 6, 8};
  for (int k : input)
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_TRUE(t.verify());

  auto * found = t.search(4);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(KEY(found), 4);

  std::vector<int> iterated;
  for (BinTree<int>::Iterator it(t); it.has_curr(); it.next_ne())
    iterated.push_back(KEY(it.get_curr_ne()));

  std::vector<int> expected = input;
  std::sort(expected.begin(), expected.end());
  EXPECT_EQ(iterated, expected);
}

TEST(BinTree, RemoveReturnsNodeAndAllowsReinsert)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;
  for (int k : {3, 1, 4, 2})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  auto * removed = t.remove(1);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 1);
  EXPECT_EQ(LLINK(removed), BinNode<int>::NullPtr);
  EXPECT_EQ(RLINK(removed), BinNode<int>::NullPtr);
  EXPECT_TRUE(t.verify());

  ASSERT_NE(t.insert(removed), nullptr);
  EXPECT_TRUE(t.verify());
}

TEST(BinTree, InsertRejectsDuplicatesButInsertDupAllows)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;

  auto * a = pool.make(5);
  auto * b = pool.make(5);
  ASSERT_NE(t.insert(a), nullptr);
  EXPECT_EQ(t.insert(b), nullptr);

  BinTree<int> t2;
  NodePool<BinTree<int>> pool2;
  ASSERT_NE(t2.insert_dup(pool2.make(5)), nullptr);
  ASSERT_NE(t2.insert_dup(pool2.make(5)), nullptr);
  EXPECT_TRUE(t2.verify());

  auto keys = inorder_keys<BinNode<int>>(t2.getRoot());
  EXPECT_EQ(keys, (std::vector<int>{5, 5}));
}

TEST(BinTree, SplitKeyNotPresentEmptiesSource)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;
  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  BinTree<int> l;
  BinTree<int> r;
  ASSERT_TRUE(t.split(0, l, r));

  EXPECT_EQ(t.getRoot(), BinNode<int>::NullPtr);
  EXPECT_TRUE(l.verify());
  EXPECT_TRUE(r.verify());

  auto lkeys = inorder_keys<BinNode<int>>(l.getRoot());
  auto rkeys = inorder_keys<BinNode<int>>(r.getRoot());
  EXPECT_TRUE(lkeys.empty());
  EXPECT_EQ(rkeys, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(BinTree, SplitKeyPresentReturnsFalseAndKeepsTree)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;
  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  BinTree<int> l;
  BinTree<int> r;
  ASSERT_FALSE(t.split(3, l, r));

  EXPECT_TRUE(t.verify());
  EXPECT_EQ(inorder_keys<BinNode<int>>(t.getRoot()), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(BinTree, SplitDupAlwaysSplits)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;
  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  BinTree<int> l;
  BinTree<int> r;
  t.split_dup(3, l, r);
  EXPECT_EQ(t.getRoot(), BinNode<int>::NullPtr);
  EXPECT_TRUE(l.verify());
  EXPECT_TRUE(r.verify());
  EXPECT_EQ(inorder_keys<BinNode<int>>(l.getRoot()), (std::vector<int>{1, 2}));
  EXPECT_EQ(inorder_keys<BinNode<int>>(r.getRoot()), (std::vector<int>{3, 4, 5}));
}

TEST(BinTree, JoinMovesNodesAndCollectsDuplicates)
{
  BinTree<int> a;
  BinTree<int> b;
  BinTree<int> dup;
  NodePool<BinTree<int>> pool;

  for (int k : {1, 3, 5})
    ASSERT_NE(a.insert(pool.make(k)), nullptr);
  for (int k : {2, 3, 4})
    ASSERT_NE(b.insert(pool.make(k)), nullptr);

  a.join(b, dup);

  EXPECT_EQ(b.getRoot(), BinNode<int>::NullPtr);
  EXPECT_TRUE(a.verify());
  EXPECT_TRUE(dup.verify());

  EXPECT_EQ(inorder_keys<BinNode<int>>(a.getRoot()), (std::vector<int>{1, 2, 3, 4, 5}));
  EXPECT_EQ(inorder_keys<BinNode<int>>(dup.getRoot()), (std::vector<int>{3}));
}

TEST(BinTree, JoinDupAllowsDuplicates)
{
  BinTree<int> a;
  BinTree<int> b;
  NodePool<BinTree<int>> pool;

  for (int k : {1, 3})
    ASSERT_NE(a.insert(pool.make(k)), nullptr);
  for (int k : {3, 4})
    ASSERT_NE(b.insert(pool.make(k)), nullptr);

  a.join_dup(b);
  EXPECT_EQ(b.getRoot(), BinNode<int>::NullPtr);
  EXPECT_TRUE(a.verify());

  EXPECT_EQ(inorder_keys<BinNode<int>>(a.getRoot()), (std::vector<int>{1, 3, 3, 4}));
}

TEST(BinTree, RemoveMissingReturnsNull)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;
  for (int k : {1, 2, 3})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_EQ(t.remove(42), nullptr);
  EXPECT_TRUE(t.verify());
}

TEST(BinTree, SearchOrInsertReturnsExistingOrInserted)
{
  BinTree<int> t;
  NodePool<BinTree<int>> pool;

  auto * p = pool.make(2);
  auto * ret1 = t.search_or_insert(p);
  ASSERT_EQ(ret1, p);
  EXPECT_TRUE(t.verify());

  auto * other = pool.make(2);
  auto * ret2 = t.search_or_insert(other);
  ASSERT_NE(ret2, nullptr);
  EXPECT_NE(ret2, other);
  EXPECT_EQ(KEY(ret2), 2);
  EXPECT_TRUE(t.verify());
}

TEST(BinTree, SwapExchangesRoots)
{
  BinTree<int> a;
  BinTree<int> b;
  NodePool<BinTree<int>> pool;

  ASSERT_NE(a.insert(pool.make(1)), nullptr);
  ASSERT_NE(b.insert(pool.make(2)), nullptr);

  a.swap(b);
  EXPECT_TRUE(a.verify());
  EXPECT_TRUE(b.verify());

  EXPECT_EQ(inorder_keys<BinNode<int>>(a.getRoot()), (std::vector<int>{2}));
  EXPECT_EQ(inorder_keys<BinNode<int>>(b.getRoot()), (std::vector<int>{1}));
}

TEST(BinTree, JoinExclusiveConcatenatesWhenRangesAreDisjoint)
{
  BinTree<int> a;
  BinTree<int> b;
  NodePool<BinTree<int>> pool;

  for (int k : {1, 2, 3})
    ASSERT_NE(a.insert(pool.make(k)), nullptr);
  for (int k : {4, 5, 6})
    ASSERT_NE(b.insert(pool.make(k)), nullptr);

  a.join_exclusive(b);
  EXPECT_TRUE(a.verify());
  EXPECT_EQ(b.getRoot(), BinNode<int>::NullPtr);
  EXPECT_EQ(inorder_keys<BinNode<int>>(a.getRoot()), (std::vector<int>{1, 2, 3, 4, 5, 6}));
}
