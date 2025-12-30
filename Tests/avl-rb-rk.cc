/* Test for AVL and Red-Black trees with counters (select/position support) */

# include <gtest/gtest.h>
# include <random>
# include <vector>
# include <algorithm>

# include <tpl_avlRk.H>
# include <tpl_rbRk.H>

using namespace std;
using namespace Aleph;

// Random generator
static mt19937 rng(42);

template <typename Tree>
class RankTreeTest : public ::testing::Test
{
protected:
  using Node = typename Tree::Node;
  Tree tree;
  vector<int> keys;
  static constexpr size_t N = 1000;

  void SetUp() override
  {
    keys.resize(N);
    iota(keys.begin(), keys.end(), 0);
    shuffle(keys.begin(), keys.end(), rng);
  }

  void TearDown() override
  {
    while (not tree.is_empty())
      {
        auto p = tree.remove(tree.getRoot()->get_key());
        delete p;
      }
  }

  void insert_all()
  {
    for (int k : keys)
      {
        auto p = new Node(k);
        ASSERT_NE(tree.insert(p), nullptr);
      }
  }
};

using AvlRkTest = RankTreeTest<Avl_Tree_Rk<int>>;
using RbRkTest = RankTreeTest<Rb_Tree_Rk<int>>;

// AVL Tree Tests

TEST_F(AvlRkTest, InsertAndVerify)
{
  insert_all();

  EXPECT_EQ(tree.size(), N);
  EXPECT_TRUE(tree.verify());
}

TEST_F(AvlRkTest, SelectOperations)
{
  insert_all();

  // After inserting 0..N-1, select(i) should return node with key i
  for (size_t i = 0; i < N; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(i));
    }
}

TEST_F(AvlRkTest, PositionOperations)
{
  insert_all();

  // For each key k, position(k) should return k
  for (size_t i = 0; i < N; ++i)
    {
      auto [pos, node] = tree.position(static_cast<int>(i));
      EXPECT_EQ(pos, static_cast<long>(i));
      ASSERT_NE(node, nullptr);
      EXPECT_EQ(node->get_key(), static_cast<int>(i));
    }
}

TEST_F(AvlRkTest, PositionNotFound)
{
  insert_all();

  // Key not in tree should return -1
  auto [pos, node] = tree.position(static_cast<int>(N + 100));
  EXPECT_EQ(pos, -1);
}

TEST_F(AvlRkTest, RemoveAndVerify)
{
  insert_all();

  shuffle(keys.begin(), keys.end(), rng);

  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.remove(keys[i]);
      ASSERT_NE(p, nullptr);
      delete p;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), N - N / 2);
}

TEST_F(AvlRkTest, SelectAfterRemoval)
{
  insert_all();

  // Remove even keys
  for (int k = 0; k < static_cast<int>(N); k += 2)
    {
      auto p = tree.remove(k);
      delete p;
    }

  EXPECT_EQ(tree.size(), N / 2);

  // Now select should return odd keys in order
  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(2 * i + 1));
    }
}

// Red-Black Tree Tests

TEST_F(RbRkTest, InsertAndVerify)
{
  insert_all();

  EXPECT_EQ(tree.size(), N);
  EXPECT_TRUE(tree.verify());
}

TEST_F(RbRkTest, SelectOperations)
{
  insert_all();

  // After inserting 0..N-1, select(i) should return node with key i
  for (size_t i = 0; i < N; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(i));
    }
}

TEST_F(RbRkTest, PositionOperations)
{
  insert_all();

  // For each key k, position(k) should return k
  for (size_t i = 0; i < N; ++i)
    {
      auto [pos, node] = tree.position(static_cast<int>(i));
      EXPECT_EQ(pos, static_cast<long>(i));
      ASSERT_NE(node, nullptr);
      EXPECT_EQ(node->get_key(), static_cast<int>(i));
    }
}

TEST_F(RbRkTest, PositionNotFound)
{
  insert_all();

  // Key not in tree should return -1
  auto [pos, node] = tree.position(static_cast<int>(N + 100));
  EXPECT_EQ(pos, -1);
}

TEST_F(RbRkTest, RemoveAndVerify)
{
  insert_all();

  shuffle(keys.begin(), keys.end(), rng);

  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.remove(keys[i]);
      ASSERT_NE(p, nullptr);
      delete p;
      EXPECT_TRUE(tree.verify());
    }

  EXPECT_EQ(tree.size(), N - N / 2);
}

TEST_F(RbRkTest, SelectAfterRemoval)
{
  insert_all();

  // Remove even keys
  for (int k = 0; k < static_cast<int>(N); k += 2)
    {
      auto p = tree.remove(k);
      delete p;
    }

  EXPECT_EQ(tree.size(), N / 2);

  // Now select should return odd keys in order
  for (size_t i = 0; i < N / 2; ++i)
    {
      auto p = tree.select(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->get_key(), static_cast<int>(2 * i + 1));
    }
}

// Combined stress test

TEST(StressTest, MixedOperationsAvl)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;
  const size_t N = 5000;

  // Insert
  for (size_t i = 0; i < N; ++i)
    {
      auto p = new Node(static_cast<int>(i));
      tree.insert(p);
    }

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N);

  // Random selects
  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int i = 0; i < 100; ++i)
    {
      size_t pos = dist(rng);
      auto p = tree.select(pos);
      EXPECT_EQ(p->get_key(), static_cast<int>(pos));
    }

  // Random positions
  for (int i = 0; i < 100; ++i)
    {
      int key = static_cast<int>(dist(rng));
      auto [pos, node] = tree.position(key);
      EXPECT_EQ(pos, key);
    }

  // Remove half
  for (int i = 0; i < static_cast<int>(N); i += 2)
    delete tree.remove(i);

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N / 2);

  // Cleanup
  while (not tree.is_empty())
    delete tree.remove(tree.getRoot()->get_key());
}

TEST(StressTest, MixedOperationsRb)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;
  const size_t N = 5000;

  // Insert
  for (size_t i = 0; i < N; ++i)
    {
      auto p = new Node(static_cast<int>(i));
      tree.insert(p);
    }

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N);

  // Random selects
  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int i = 0; i < 100; ++i)
    {
      size_t pos = dist(rng);
      auto p = tree.select(pos);
      EXPECT_EQ(p->get_key(), static_cast<int>(pos));
    }

  // Random positions
  for (int i = 0; i < 100; ++i)
    {
      int key = static_cast<int>(dist(rng));
      auto [pos, node] = tree.position(key);
      EXPECT_EQ(pos, key);
    }

  // Remove half
  for (int i = 0; i < static_cast<int>(N); i += 2)
    delete tree.remove(i);

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), N / 2);

  // Cleanup
  while (not tree.is_empty())
    delete tree.remove(tree.getRoot()->get_key());
}

// Test empty tree edge cases

TEST(EdgeCases, EmptyAvlTree)
{
  Avl_Tree_Rk<int> tree;

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_EQ(tree.remove(42), nullptr);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, -1);
}

TEST(EdgeCases, EmptyRbTree)
{
  Rb_Tree_Rk<int> tree;

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_EQ(tree.remove(42), nullptr);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, -1);
}

// Test single element

TEST(EdgeCases, SingleElementAvl)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;

  auto p = new Node(42);
  tree.insert(p);

  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.verify());

  auto selected = tree.select(0);
  EXPECT_EQ(selected->get_key(), 42);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, 0);
  EXPECT_EQ(node->get_key(), 42);

  delete tree.remove(42);
  EXPECT_TRUE(tree.is_empty());
}

TEST(EdgeCases, SingleElementRb)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;

  auto p = new Node(42);
  tree.insert(p);

  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.verify());

  auto selected = tree.select(0);
  EXPECT_EQ(selected->get_key(), 42);

  auto [pos, node] = tree.position(42);
  EXPECT_EQ(pos, 0);
  EXPECT_EQ(node->get_key(), 42);

  delete tree.remove(42);
  EXPECT_TRUE(tree.is_empty());
}

// Test search_or_insert

TEST(SearchOrInsert, AvlTree)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;

  auto p1 = new Node(42);
  auto result1 = tree.search_or_insert(p1);
  EXPECT_EQ(result1, p1);
  EXPECT_EQ(tree.size(), 1);

  auto p2 = new Node(42);
  auto result2 = tree.search_or_insert(p2);
  EXPECT_EQ(result2, p1);  // Should return existing node
  EXPECT_EQ(tree.size(), 1);  // Size should not change

  delete p2;  // Delete the non-inserted node
  delete tree.remove(42);
}

TEST(SearchOrInsert, RbTree)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;

  auto p1 = new Node(42);
  auto result1 = tree.search_or_insert(p1);
  EXPECT_EQ(result1, p1);
  EXPECT_EQ(tree.size(), 1);

  auto p2 = new Node(42);
  auto result2 = tree.search_or_insert(p2);
  EXPECT_EQ(result2, p1);  // Should return existing node
  EXPECT_EQ(tree.size(), 1);  // Size should not change

  delete p2;  // Delete the non-inserted node
  delete tree.remove(42);
}

// Test insert_dup

TEST(InsertDup, AvlTree)
{
  Avl_Tree_Rk<int> tree;
  using Node = Avl_Tree_Rk<int>::Node;

  for (int i = 0; i < 10; ++i)
    {
      auto p = new Node(42);
      tree.insert_dup(p);
    }

  EXPECT_EQ(tree.size(), 10);
  EXPECT_TRUE(tree.verify());

  // All should have key 42
  for (size_t i = 0; i < 10; ++i)
    EXPECT_EQ(tree.select(i)->get_key(), 42);

  while (not tree.is_empty())
    delete tree.remove(42);
}

TEST(InsertDup, RbTree)
{
  Rb_Tree_Rk<int> tree;
  using Node = Rb_Tree_Rk<int>::Node;

  for (int i = 0; i < 10; ++i)
    {
      auto p = new Node(42);
      tree.insert_dup(p);
    }

  EXPECT_EQ(tree.size(), 10);
  EXPECT_TRUE(tree.verify());

  // All should have key 42
  for (size_t i = 0; i < 10; ++i)
    EXPECT_EQ(tree.select(i)->get_key(), 42);

  while (not tree.is_empty())
    delete tree.remove(42);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}