/*
  This file is part of Aleph-w
  
  Test suite for DynSetTree with all tree types:
  - Binary Tree
  - AVL Tree
  - Red-Black Tree
  - Splay Tree
  - Treap
  - Random Tree
  - Treap with Rank
*/

#include <gtest/gtest.h>
#include <tpl_dynSetTree.H>
#include <vector>
#include <string>

using namespace std;
using namespace Aleph;

// Template test fixture that works with any DynSetTree type
template <typename T>
class DynSetTreeTest : public ::testing::Test
{
protected:
  T set;
};

// Define all tree types to test
using AllTreeTypes = ::testing::Types<
  DynSetBinTree<int>,
  DynSetAvlTree<int>,
  DynSetRbTree<int>,
  DynSetSplayTree<int>,
  DynSetTreap<int>,
  DynSetRandTree<int>,
  DynSetTreapRk<int>
>;

TYPED_TEST_SUITE(DynSetTreeTest, AllTreeTypes);

// Test basic insertion and search
TYPED_TEST(DynSetTreeTest, InsertAndSearch)
{
  // Insert 100 elements
  for (int i = 0; i < 100; ++i)
    {
      auto * p = this->set.insert(i);
      ASSERT_NE(p, nullptr) << "Failed to insert " << i;
      EXPECT_EQ(*p, i);
    }
  
  EXPECT_EQ(this->set.size(), 100u);
  
  // Test existing elements
  for (int i = 0; i < 100; ++i)
    {
      EXPECT_NE(this->set.search(i), nullptr) << "Element " << i << " not found";
      EXPECT_TRUE(this->set.contains(i));
    }

  // Test non-existing elements
  EXPECT_EQ(this->set.search(-1), nullptr);
  EXPECT_EQ(this->set.search(100), nullptr);
  EXPECT_EQ(this->set.search(1000), nullptr);
  EXPECT_FALSE(this->set.contains(-1));
  EXPECT_FALSE(this->set.contains(100));
}

// Test removal of elements
TYPED_TEST(DynSetTreeTest, Remove)
{
  // Insert elements
  for (int i = 0; i < 100; ++i)
    this->set.insert(i);
  
  EXPECT_EQ(this->set.size(), 100u);
  
  // Remove even numbers
  for (int i = 0; i < 100; i += 2)
    this->set.remove(i);

  EXPECT_EQ(this->set.size(), 50u);

  // Verify removal
  for (int i = 0; i < 100; ++i) 
    {
      if (i % 2 == 0)
        {
          EXPECT_EQ(this->set.search(i), nullptr) << "Element " << i << " should be removed";
          EXPECT_FALSE(this->set.contains(i));
        }
      else
        {
          EXPECT_NE(this->set.search(i), nullptr) << "Element " << i << " should exist";
          EXPECT_TRUE(this->set.contains(i));
        }
    }
}

// Test copy constructor
TYPED_TEST(DynSetTreeTest, CopyConstructor)
{
  for (int i = 0; i < 50; ++i)
    this->set.insert(i);
  
  // Copy constructor
  TypeParam set_copy(this->set);
  
  EXPECT_EQ(set_copy.size(), 50u);
  
  // Verify all elements were copied
  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(set_copy.contains(i));
  
  // Verify independence - modify original
  this->set.remove(25);
  EXPECT_FALSE(this->set.contains(25));
  EXPECT_TRUE(set_copy.contains(25));  // Copy should still have it
}

// Test assignment operator
TYPED_TEST(DynSetTreeTest, AssignmentOperator)
{
  for (int i = 0; i < 50; ++i)
    this->set.insert(i);
  
  TypeParam set_copy;
  set_copy = this->set;
  
  EXPECT_EQ(set_copy.size(), 50u);
  
  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(set_copy.contains(i));
}

// Test move constructor
TYPED_TEST(DynSetTreeTest, MoveConstructor)
{
  for (int i = 0; i < 50; ++i)
    this->set.insert(i);
  
  TypeParam set_moved(std::move(this->set));
  
  EXPECT_EQ(set_moved.size(), 50u);
  EXPECT_TRUE(this->set.is_empty());
  
  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(set_moved.contains(i));
}

// Test move assignment
TYPED_TEST(DynSetTreeTest, MoveAssignment)
{
  for (int i = 0; i < 50; ++i)
    this->set.insert(i);
  
  TypeParam set_moved;
  set_moved = std::move(this->set);
  
  EXPECT_EQ(set_moved.size(), 50u);
  EXPECT_TRUE(this->set.is_empty());
  
  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(set_moved.contains(i));
}

// Test iteration
TYPED_TEST(DynSetTreeTest, Iteration)
{
  // Insert in random order
  vector<int> values = {50, 25, 75, 10, 30, 60, 90, 5, 15, 27, 35};
  for (int v : values)
    this->set.insert(v);
  
  // Iterate and collect - should be in sorted order
  vector<int> collected;
  for (auto it = this->set.get_it(); it.has_curr(); it.next_ne())
    collected.push_back(it.get_curr());
  
  // Verify sorted order
  EXPECT_TRUE(std::is_sorted(collected.begin(), collected.end()));
  EXPECT_EQ(collected.size(), values.size());
}

// Test empty operation
TYPED_TEST(DynSetTreeTest, Empty)
{
  for (int i = 0; i < 50; ++i)
    this->set.insert(i);
  
  EXPECT_FALSE(this->set.is_empty());
  EXPECT_EQ(this->set.size(), 50u);
  
  this->set.empty();
  
  EXPECT_TRUE(this->set.is_empty());
  EXPECT_EQ(this->set.size(), 0u);
  
  // Verify all elements were removed
  for (int i = 0; i < 50; ++i)
    EXPECT_FALSE(this->set.contains(i));
}

// Test min and max
TYPED_TEST(DynSetTreeTest, MinMax)
{
  for (int i : {50, 25, 75, 10, 90})
    this->set.insert(i);
  
  EXPECT_EQ(this->set.min(), 10);
  EXPECT_EQ(this->set.max(), 90);
  
  this->set.remove(10);
  EXPECT_EQ(this->set.min(), 25);
  
  this->set.remove(90);
  EXPECT_EQ(this->set.max(), 75);
}

// Test insert duplicate (should return nullptr)
TYPED_TEST(DynSetTreeTest, InsertDuplicate)
{
  auto * p1 = this->set.insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(this->set.size(), 1u);
  
  auto * p2 = this->set.insert(42);
  EXPECT_EQ(p2, nullptr);  // Duplicate rejected
  EXPECT_EQ(this->set.size(), 1u);
}

// Test search_or_insert
TYPED_TEST(DynSetTreeTest, SearchOrInsert)
{
  auto * p1 = this->set.search_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(this->set.size(), 1u);
  
  auto * p2 = this->set.search_or_insert(42);
  EXPECT_EQ(p1, p2);  // Should return existing
  EXPECT_EQ(this->set.size(), 1u);
}

// Test with large dataset
TYPED_TEST(DynSetTreeTest, LargeDataset)
{
  const int N = 10000;
  
  // Insert N elements
  for (int i = 0; i < N; ++i)
    this->set.insert(i);
  
  EXPECT_EQ(this->set.size(), static_cast<size_t>(N));
  EXPECT_EQ(this->set.min(), 0);
  EXPECT_EQ(this->set.max(), N - 1);
  
  // Verify some elements exist
  for (int i = 0; i < N; i += 100)
    EXPECT_TRUE(this->set.contains(i));
  
  // Remove half the elements
  for (int i = 0; i < N; i += 2)
    this->set.remove(i);
  
  EXPECT_EQ(this->set.size(), static_cast<size_t>(N / 2));
  
  // Verify removal
  for (int i = 0; i < N; ++i)
    {
      if (i % 2 == 0)
        EXPECT_FALSE(this->set.contains(i));
      else
        EXPECT_TRUE(this->set.contains(i));
    }
}

// Test swap operation
TYPED_TEST(DynSetTreeTest, Swap)
{
  TypeParam set2;
  
  for (int i : {1, 2, 3})
    this->set.insert(i);
  for (int i : {10, 20})
    set2.insert(i);
  
  this->set.swap(set2);
  
  EXPECT_EQ(this->set.size(), 2u);
  EXPECT_TRUE(this->set.contains(10));
  EXPECT_TRUE(this->set.contains(20));
  
  EXPECT_EQ(set2.size(), 3u);
  EXPECT_TRUE(set2.contains(1));
  EXPECT_TRUE(set2.contains(2));
  EXPECT_TRUE(set2.contains(3));
}

// Test with string type (separate test)
TEST(DynSetTreeStringTest, StringKeys)
{
  using StringSet = DynSetAvlTree<string>;
  StringSet set;
  
  vector<string> words = {"apple", "banana", "cherry", "date", "elderberry"};
  
  for (const auto& word : words)
    set.insert(word);
  
  EXPECT_EQ(set.size(), 5u);
  
  // Test search
  for (const auto& word : words)
    {
      EXPECT_NE(set.search(word), nullptr);
      EXPECT_TRUE(set.contains(word));
    }
  
  // Test iteration order (should be alphabetical)
  vector<string> collected;
  for (auto it = set.get_it(); it.has_curr(); it.next_ne())
    collected.push_back(it.get_curr());
  EXPECT_TRUE(std::is_sorted(collected.begin(), collected.end()));
}

// Test with custom comparator (separate test)
TEST(DynSetTreeCustomCompare, DescendingOrder)
{
  using SetType = DynSetTree<int, Avl_Tree, std::greater<int>>;
  SetType set;
  
  for (int i : {5, 2, 8, 1, 9})
    set.insert(i);
  
  // Verify descending order via iteration
  vector<int> collected;
  for (auto it = set.get_it(); it.has_curr(); it.next_ne())
    collected.push_back(it.get_curr());
  
  vector<int> expected = {9, 8, 5, 2, 1};
  EXPECT_EQ(collected, expected);
}
