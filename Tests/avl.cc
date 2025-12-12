#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_avl.H>

using namespace Aleph;
using namespace testing;

namespace
{
  template <class Tree>
  struct NodePool
  {
    using Node = typename Tree::Node;

    std::vector<Node *> allocated;

    Node * make(const typename Node::key_type & key)
    {
      Node * p = new Node(key);
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
      for (Node * p : allocated)
        delete p;
    }
  };

  template <class Node>
  std::vector<typename Node::key_type> inorder_keys(Node * root)
  {
    std::vector<typename Node::key_type> keys;
    Aleph::infix_for_each<Node>(root, [&] (Node * p) { keys.push_back(KEY(p)); });
    return keys;
  }
}

TEST(AvlTree, InsertSearchVerifyAndIteratorOrder)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  const std::vector<int> input{5, 3, 7, 2, 4, 6, 8};
  for (int k : input)
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_TRUE(t.verify());
  EXPECT_TRUE(is_avl(t.getRoot()));

  auto * found = t.search(4);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(KEY(found), 4);

  std::vector<int> it_keys;
  for (Avl_Tree<int>::Iterator it(t); it.has_curr(); it.next_ne())
    it_keys.push_back(KEY(it.get_curr_ne()));

  std::vector<int> expected = input;
  std::sort(expected.begin(), expected.end());
  EXPECT_EQ(it_keys, expected);
}

TEST(AvlTree, InsertRejectsDuplicatesAndSearchOrInsertReturnsExisting)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  auto * a = pool.make(5);
  auto * b = pool.make(5);

  ASSERT_NE(t.insert(a), nullptr);
  EXPECT_EQ(t.insert(b), nullptr);

  auto * c = pool.make(5);
  auto * got = t.search_or_insert(c);
  ASSERT_NE(got, nullptr);
  EXPECT_NE(got, c);
  EXPECT_EQ(KEY(got), 5);

  EXPECT_TRUE(t.verify());
}

TEST(AvlTree, InsertDupAllowsDuplicates)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  ASSERT_NE(t.insert_dup(pool.make(5)), nullptr);
  ASSERT_NE(t.insert_dup(pool.make(5)), nullptr);
  ASSERT_NE(t.insert_dup(pool.make(5)), nullptr);

  EXPECT_TRUE(t.verify());
  EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{5, 5, 5}));
}

TEST(AvlTree, RemoveMissingReturnsNull)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  for (int k : {1, 2, 3})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_EQ(t.remove(42), nullptr);
  EXPECT_TRUE(t.verify());
}

TEST(AvlTree, RemoveReturnsDetachedNode)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;
  for (int k : {3, 1, 4, 2})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  auto * removed = t.remove(1);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 1);
  EXPECT_EQ(LLINK(removed), Avl_Tree<int>::Node::NullPtr);
  EXPECT_EQ(RLINK(removed), Avl_Tree<int>::Node::NullPtr);
  EXPECT_EQ(static_cast<int>(DIFF(removed)), 0);

  EXPECT_TRUE(t.verify());

  pool.forget(removed);
  delete removed;
}

TEST(AvlTree, Property_RandomInsertRemove_StableInvariants)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 500);

  std::set<int> present;

  for (int i = 0; i < 300; ++i)
    {
      int k = dist(rng);
      auto * p = pool.make(k);
      auto * ins = t.insert(p);
      if (ins == nullptr)
        {
          pool.forget(p);
          delete p;
        }
      else
        present.insert(k);

      ASSERT_TRUE(t.verify());
    }

  for (int i = 0; i < 200; ++i)
    {
      int k = dist(rng);
      auto * removed = t.remove(k);
      if (removed != nullptr)
        {
          present.erase(k);
          pool.forget(removed);
          delete removed;
        }
      ASSERT_TRUE(t.verify());
    }
}

TEST(AvlTree, Rotations_LL_RR_LR_RL)
{
  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(3)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr);
    ASSERT_NE(t.insert(pool.make(1)), nullptr); // LL
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }

  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(1)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr);
    ASSERT_NE(t.insert(pool.make(3)), nullptr); // RR
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }

  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(3)), nullptr);
    ASSERT_NE(t.insert(pool.make(1)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr); // LR
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }

  {
    Avl_Tree<int> t;
    NodePool<Avl_Tree<int>> pool;
    ASSERT_NE(t.insert(pool.make(1)), nullptr);
    ASSERT_NE(t.insert(pool.make(3)), nullptr);
    ASSERT_NE(t.insert(pool.make(2)), nullptr); // RL
    ASSERT_TRUE(t.verify());
    EXPECT_EQ(inorder_keys<Avl_Tree<int>::Node>(t.getRoot()), (std::vector<int>{1, 2, 3}));
  }
}

TEST(AvlTree, Property_OracleSet_MatchesInorder)
{
  Avl_Tree<int> t;
  NodePool<Avl_Tree<int>> pool;

  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> dist(0, 2000);
  std::bernoulli_distribution do_insert(0.6);

  std::set<int> oracle;

  for (int i = 0; i < 1500; ++i)
    {
      int k = dist(rng);
      if (do_insert(rng))
        {
          auto * p = pool.make(k);
          auto * ins = t.insert(p);
          if (ins == nullptr)
            {
              pool.forget(p);
              delete p;
            }
          else
            oracle.insert(k);
        }
      else
        {
          auto * removed = t.remove(k);
          if (removed != nullptr)
            {
              oracle.erase(k);
              pool.forget(removed);
              delete removed;
            }
        }

      ASSERT_TRUE(t.verify());

      std::vector<int> expected(oracle.begin(), oracle.end());
      auto got = inorder_keys<Avl_Tree<int>::Node>(t.getRoot());
      ASSERT_EQ(got, expected);
    }
}

TEST(AvlTree, WorksWithCustomComparator)
{
  struct Greater
  {
    bool operator()(int a, int b) const noexcept { return a > b; }
  };

  Avl_Tree<int, Greater> t;
  NodePool<Avl_Tree<int, Greater>> pool;

  for (int k : {1, 2, 3, 4, 5})
    ASSERT_NE(t.insert(pool.make(k)), nullptr);

  EXPECT_TRUE(t.verify());
  auto got = inorder_keys<Avl_Tree<int, Greater>::Node>(t.getRoot());
  const std::vector<int> expected{5, 4, 3, 2, 1};
  EXPECT_EQ(got, expected);

  auto * removed = t.remove(4);
  ASSERT_NE(removed, nullptr);
  pool.forget(removed);
  delete removed;

  EXPECT_TRUE(t.verify());
}
