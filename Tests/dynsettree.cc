#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <gtest/gtest.h>

#include <tpl_dynSetTree.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Typed Tests - Run same tests against ALL tree types
// ============================================================================

template <typename T>
class DynSetTreeTypedTest : public ::testing::Test
{
protected:
  T set;
};

using AllTreeTypes = ::testing::Types<
  DynSetBinTree<int>,
  DynSetAvlTree<int>,
  DynSetSplayTree<int>,
  DynSetRandTree<int>,
  DynSetTreap<int>,
  DynSetTreapRk<int>,
  DynSetRbTree<int>
>;

TYPED_TEST_SUITE(DynSetTreeTypedTest, AllTreeTypes);

TYPED_TEST(DynSetTreeTypedTest, EmptySetProperties)
{
  EXPECT_TRUE(this->set.is_empty());
  EXPECT_EQ(this->set.size(), 0u);
  EXPECT_THROW(this->set.min(), domain_error);
  EXPECT_THROW(this->set.max(), domain_error);
  EXPECT_THROW(this->set.get_root(), domain_error);
  EXPECT_FALSE(this->set.contains(42));
  EXPECT_EQ(this->set.search(42), nullptr);
}

TYPED_TEST(DynSetTreeTypedTest, InsertAndSearch)
{
  for (int i : {5, 3, 7, 1, 9, 2, 8})
    {
      auto * p = this->set.insert(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(*p, i);
    }

  EXPECT_EQ(this->set.size(), 7u);
  EXPECT_EQ(this->set.min(), 1);
  EXPECT_EQ(this->set.max(), 9);

  for (int i : {1, 2, 3, 5, 7, 8, 9})
    EXPECT_TRUE(this->set.contains(i));

  EXPECT_FALSE(this->set.contains(4));
  EXPECT_FALSE(this->set.contains(6));
}

TYPED_TEST(DynSetTreeTypedTest, InsertRejectsDuplicates)
{
  auto * p1 = this->set.insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(this->set.size(), 1u);

  auto * p2 = this->set.insert(42);
  EXPECT_EQ(p2, nullptr);
  EXPECT_EQ(this->set.size(), 1u);
}

TYPED_TEST(DynSetTreeTypedTest, RemoveOperations)
{
  for (int i : {5, 3, 7, 1, 9})
    this->set.insert(i);

  EXPECT_EQ(this->set.size(), 5u);

  this->set.remove(3);
  EXPECT_EQ(this->set.size(), 4u);
  EXPECT_FALSE(this->set.contains(3));
  EXPECT_TRUE(this->set.contains(5));
}

TYPED_TEST(DynSetTreeTypedTest, SearchOrInsert)
{
  auto * p1 = this->set.search_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(this->set.size(), 1u);

  auto * p2 = this->set.search_or_insert(42);
  EXPECT_EQ(p1, p2);
  EXPECT_EQ(this->set.size(), 1u);
}

TYPED_TEST(DynSetTreeTypedTest, ContainsOrInsert)
{
  auto [p1, found1] = this->set.contains_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_FALSE(found1);
  EXPECT_EQ(this->set.size(), 1u);

  auto [p2, found2] = this->set.contains_or_insert(42);
  EXPECT_TRUE(found2);
  EXPECT_EQ(this->set.size(), 1u);
}

TYPED_TEST(DynSetTreeTypedTest, FindAndDel)
{
  this->set.insert(42);

  const auto & key = this->set.find(42);
  EXPECT_EQ(key, 42);

  EXPECT_THROW(this->set.find(99), domain_error);

  int removed = this->set.del(42);
  EXPECT_EQ(removed, 42);
  EXPECT_TRUE(this->set.is_empty());

  EXPECT_THROW(this->set.del(42), domain_error);
}

TYPED_TEST(DynSetTreeTypedTest, CopyConstructor)
{
  for (int i : {1, 2, 3, 4, 5})
    this->set.insert(i);

  TypeParam copy(this->set);

  EXPECT_EQ(copy.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(copy.contains(i));

  // Verify independence
  this->set.remove(3);
  EXPECT_FALSE(this->set.contains(3));
  EXPECT_TRUE(copy.contains(3));
}

TYPED_TEST(DynSetTreeTypedTest, MoveConstructor)
{
  for (int i : {1, 2, 3, 4, 5})
    this->set.insert(i);

  TypeParam moved(std::move(this->set));

  EXPECT_EQ(moved.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(moved.contains(i));

  EXPECT_TRUE(this->set.is_empty());
}

TYPED_TEST(DynSetTreeTypedTest, Swap)
{
  TypeParam set2;

  for (int i : {1, 2, 3})
    this->set.insert(i);
  for (int i : {10, 20})
    set2.insert(i);

  this->set.swap(set2);

  EXPECT_EQ(this->set.size(), 2u);
  EXPECT_TRUE(this->set.contains(10));
  EXPECT_EQ(set2.size(), 3u);
  EXPECT_TRUE(set2.contains(1));
}

TYPED_TEST(DynSetTreeTypedTest, EmptyMethod)
{
  for (int i = 0; i < 50; ++i)
    this->set.insert(i);

  EXPECT_EQ(this->set.size(), 50u);

  this->set.empty();

  EXPECT_TRUE(this->set.is_empty());
  EXPECT_EQ(this->set.size(), 0u);
}

TYPED_TEST(DynSetTreeTypedTest, IteratorInOrder)
{
  for (int i : {5, 3, 7, 1, 9})
    this->set.insert(i);

  vector<int> keys;
  for (typename TypeParam::Iterator it(this->set); it.has_curr(); it.next_ne())
    keys.push_back(it.get_curr());

  vector<int> expected = {1, 3, 5, 7, 9};
  EXPECT_EQ(keys, expected);
}

TYPED_TEST(DynSetTreeTypedTest, ForEachInorder)
{
  for (int i : {5, 3, 7, 1, 9})
    this->set.insert(i);

  vector<int> keys;
  this->set.for_each_inorder([&keys](const int & k) { keys.push_back(k); });

  vector<int> expected = {1, 3, 5, 7, 9};
  EXPECT_EQ(keys, expected);
}

TYPED_TEST(DynSetTreeTypedTest, Traverse)
{
  for (int i : {1, 2, 3, 4, 5})
    this->set.insert(i);

  int sum = 0;
  bool completed = this->set.traverse([&sum](const int & k) {
    sum += k;
    return true;
  });

  EXPECT_TRUE(completed);
  EXPECT_EQ(sum, 15);
}

TYPED_TEST(DynSetTreeTypedTest, Verify)
{
  for (int i : {5, 3, 7, 1, 9, 2, 8, 4, 6})
    this->set.insert(i);

  EXPECT_TRUE(this->set.verify());
}

TYPED_TEST(DynSetTreeTypedTest, StressTest)
{
  // Insert 500 elements
  for (int i = 0; i < 500; ++i)
    this->set.insert(i);

  EXPECT_EQ(this->set.size(), 500u);
  EXPECT_EQ(this->set.min(), 0);
  EXPECT_EQ(this->set.max(), 499);

  // Remove half
  for (int i = 0; i < 500; i += 2)
    this->set.remove(i);

  EXPECT_EQ(this->set.size(), 250u);

  // Verify correct ones remain
  for (int i = 1; i < 500; i += 2)
    EXPECT_TRUE(this->set.contains(i));
}

// ============================================================================
// Basic Operations Tests (applicable to all tree types)
// ============================================================================

#if 0

TEST(DynSetTree, EmptySetProperties)
{
  DynSetAvlTree<int> set;

  EXPECT_TRUE(set.is_empty());
  EXPECT_EQ(set.size(), 0u);
  EXPECT_THROW(set.min(), domain_error);
  EXPECT_THROW(set.max(), domain_error);
  EXPECT_THROW(set.get_root(), domain_error);
  EXPECT_THROW(set.get_item(), domain_error);
  EXPECT_FALSE(set.contains(42));
  EXPECT_EQ(set.search(42), nullptr);
}

TEST(DynSetTree, InsertSingleElement)
{
  DynSetAvlTree<int> set;

  auto * p = set.insert(42);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);
  EXPECT_FALSE(set.is_empty());
  EXPECT_EQ(set.size(), 1u);
  EXPECT_TRUE(set.contains(42));
  EXPECT_EQ(set.min(), 42);
  EXPECT_EQ(set.max(), 42);
  EXPECT_EQ(set.get_root(), 42);
}

TEST(DynSetTree, InsertMultipleElements)
{
  DynSetAvlTree<int> set;

  for (int i : {5, 3, 7, 1, 9, 2, 8})
    {
      auto * p = set.insert(i);
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(*p, i);
    }

  EXPECT_EQ(set.size(), 7u);
  EXPECT_EQ(set.min(), 1);
  EXPECT_EQ(set.max(), 9);

  for (int i : {1, 2, 3, 5, 7, 8, 9})
    EXPECT_TRUE(set.contains(i));

  EXPECT_FALSE(set.contains(4));
  EXPECT_FALSE(set.contains(6));
}

TEST(DynSetTree, InsertRejectsDuplicates)
{
  DynSetAvlTree<int> set;

  auto * p1 = set.insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(set.size(), 1u);

  auto * p2 = set.insert(42);
  EXPECT_EQ(p2, nullptr);  // Duplicate rejected
  EXPECT_EQ(set.size(), 1u);  // Size unchanged
}

TEST(DynSetTree, InsertDupAllowsDuplicates)
{
  DynSetRbTree<int> set;

  auto * p1 = set.insert_dup(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);

  auto * p2 = set.insert_dup(42);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 42);

  EXPECT_EQ(set.size(), 2u);
}

TEST(DynSetTree, SearchFindsExistingKey)
{
  DynSetAvlTree<int> set;

  set.insert(10);
  set.insert(20);
  set.insert(30);

  auto * p = set.search(20);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 20);
}

TEST(DynSetTree, SearchReturnsNullForMissingKey)
{
  DynSetAvlTree<int> set;

  set.insert(10);
  set.insert(30);

  EXPECT_EQ(set.search(20), nullptr);
}

TEST(DynSetTree, SearchOrInsertBehavior)
{
  DynSetAvlTree<int> set;

  // First call: inserts
  auto * p1 = set.search_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(set.size(), 1u);

  // Second call: returns existing
  auto * p2 = set.search_or_insert(42);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 42);
  EXPECT_EQ(set.size(), 1u);  // Size unchanged
  EXPECT_EQ(p1, p2);  // Same pointer
}

TEST(DynSetTree, ContainsOrInsertBehavior)
{
  DynSetAvlTree<int> set;

  // First call: inserts, returns false
  auto [p1, found1] = set.contains_or_insert(42);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 42);
  EXPECT_FALSE(found1);
  EXPECT_EQ(set.size(), 1u);

  // Second call: finds, returns true
  auto [p2, found2] = set.contains_or_insert(42);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 42);
  EXPECT_TRUE(found2);
  EXPECT_EQ(set.size(), 1u);
}

TEST(DynSetTree, FindReturnsReference)
{
  DynSetAvlTree<int> set;

  set.insert(42);

  const auto & key = set.find(42);
  EXPECT_EQ(key, 42);
}

TEST(DynSetTree, FindThrowsIfNotFound)
{
  DynSetAvlTree<int> set;

  set.insert(10);

  EXPECT_THROW(set.find(42), domain_error);
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST(DynSetTree, RemoveExistingKey)
{
  DynSetAvlTree<int> set;

  set.insert(10);
  set.insert(20);
  set.insert(30);

  size_t new_size = set.remove(20);
  EXPECT_EQ(new_size, 2u);
  EXPECT_EQ(set.size(), 2u);
  EXPECT_FALSE(set.contains(20));
  EXPECT_TRUE(set.contains(10));
  EXPECT_TRUE(set.contains(30));
}

TEST(DynSetTree, RemoveReturnsCurrentSizeIfNotFound)
{
  DynSetAvlTree<int> set;

  set.insert(10);
  set.insert(30);

  size_t new_size = set.remove(20);
  EXPECT_EQ(new_size, 2u);
  EXPECT_EQ(set.size(), 2u);
}

TEST(DynSetTree, RemoveFromEmptySet)
{
  DynSetAvlTree<int> set;

  size_t new_size = set.remove(42);
  EXPECT_EQ(new_size, 0u);
  EXPECT_TRUE(set.is_empty());
}

TEST(DynSetTree, DelReturnsRemovedKey)
{
  DynSetAvlTree<int> set;

  set.insert(42);

  int removed = set.del(42);
  EXPECT_EQ(removed, 42);
  EXPECT_FALSE(set.contains(42));
  EXPECT_EQ(set.size(), 0u);
}

TEST(DynSetTree, DelThrowsIfNotFound)
{
  DynSetAvlTree<int> set;

  set.insert(10);

  EXPECT_THROW(set.del(42), domain_error);
}

TEST(DynSetTree, RemoveAllElements)
{
  DynSetAvlTree<int> set;

  for (int i = 0; i < 10; ++i)
    set.insert(i);

  EXPECT_EQ(set.size(), 10u);

  for (int i = 0; i < 10; ++i)
    set.remove(i);

  EXPECT_TRUE(set.is_empty());
  EXPECT_EQ(set.size(), 0u);
}

TEST(DynSetTree, EmptyMethod)
{
  DynSetAvlTree<int> set;

  for (int i = 0; i < 100; ++i)
    set.insert(i);

  EXPECT_EQ(set.size(), 100u);

  set.empty();

  EXPECT_TRUE(set.is_empty());
  EXPECT_EQ(set.size(), 0u);
}

// ============================================================================
// Copy/Move Semantics Tests
// ============================================================================

TEST(DynSetTree, CopyConstructor)
{
  DynSetAvlTree<int> set1;

  for (int i : {1, 2, 3, 4, 5})
    set1.insert(i);

  DynSetAvlTree<int> set2(set1);

  EXPECT_EQ(set2.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(set2.contains(i));

  // Verify independence
  set1.remove(3);
  EXPECT_FALSE(set1.contains(3));
  EXPECT_TRUE(set2.contains(3));
}

TEST(DynSetTree, CopyAssignment)
{
  DynSetAvlTree<int> set1, set2;

  for (int i : {1, 2, 3})
    set1.insert(i);

  for (int i : {10, 20})
    set2.insert(i);

  set2 = set1;

  EXPECT_EQ(set2.size(), 3u);
  for (int i : {1, 2, 3})
    EXPECT_TRUE(set2.contains(i));
  EXPECT_FALSE(set2.contains(10));
  EXPECT_FALSE(set2.contains(20));
}

TEST(DynSetTree, SelfAssignment)
{
  DynSetAvlTree<int> set;

  for (int i : {1, 2, 3})
    set.insert(i);

  set = set;

  EXPECT_EQ(set.size(), 3u);
  for (int i : {1, 2, 3})
    EXPECT_TRUE(set.contains(i));
}

TEST(DynSetTree, MoveConstructor)
{
  DynSetAvlTree<int> set1;

  for (int i : {1, 2, 3, 4, 5})
    set1.insert(i);

  DynSetAvlTree<int> set2(std::move(set1));

  EXPECT_EQ(set2.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(set2.contains(i));

  EXPECT_TRUE(set1.is_empty());
}

TEST(DynSetTree, MoveAssignment)
{
  DynSetAvlTree<int> set1, set2;

  for (int i : {1, 2, 3})
    set1.insert(i);

  for (int i : {10, 20})
    set2.insert(i);

  set2 = std::move(set1);

  EXPECT_EQ(set2.size(), 3u);
  for (int i : {1, 2, 3})
    EXPECT_TRUE(set2.contains(i));

  EXPECT_TRUE(set1.is_empty());
}

TEST(DynSetTree, Swap)
{
  DynSetAvlTree<int> set1, set2;

  for (int i : {1, 2, 3})
    set1.insert(i);

  for (int i : {10, 20})
    set2.insert(i);

  set1.swap(set2);

  EXPECT_EQ(set1.size(), 2u);
  EXPECT_TRUE(set1.contains(10));
  EXPECT_TRUE(set1.contains(20));

  EXPECT_EQ(set2.size(), 3u);
  EXPECT_TRUE(set2.contains(1));
  EXPECT_TRUE(set2.contains(2));
  EXPECT_TRUE(set2.contains(3));
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(DynSetTree, IteratorEmptySet)
{
  DynSetAvlTree<int> set;

  DynSetAvlTree<int>::Iterator it(set);
  EXPECT_FALSE(it.has_curr());
}

TEST(DynSetTree, IteratorTraversesInOrder)
{
  DynSetAvlTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  vector<int> keys;
  for (DynSetAvlTree<int>::Iterator it(set); it.has_curr(); it.next_ne())
    keys.push_back(it.get_curr());

  vector<int> expected = {1, 3, 5, 7, 9};
  EXPECT_EQ(keys, expected);
}

TEST(DynSetTree, IteratorReset)
{
  DynSetAvlTree<int> set;

  for (int i : {1, 2, 3})
    set.insert(i);

  DynSetAvlTree<int>::Iterator it(set);

  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 1);

  it.next_ne();
  EXPECT_EQ(it.get_curr(), 2);

  it.reset_first();
  EXPECT_EQ(it.get_curr(), 1);
}

// ============================================================================
// Traversal Tests (for_each_*)
// ============================================================================

TEST(DynSetTree, ForEachInorder)
{
  DynSetAvlTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  vector<int> keys;
  set.for_each_inorder([&keys](const int & k) { keys.push_back(k); });

  vector<int> expected = {1, 3, 5, 7, 9};
  EXPECT_EQ(keys, expected);
}

TEST(DynSetTree, ForEachPreorder)
{
  DynSetAvlTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  vector<int> keys;
  set.for_each_preorder([&keys](const int & k) { keys.push_back(k); });

  // Just verify we got all keys (order depends on tree structure)
  EXPECT_EQ(keys.size(), 5u);
  for (int i : {1, 3, 5, 7, 9})
    EXPECT_TRUE(find(keys.begin(), keys.end(), i) != keys.end());
}

TEST(DynSetTree, ForEachPostorder)
{
  DynSetAvlTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  vector<int> keys;
  set.for_each_postorder([&keys](const int & k) { keys.push_back(k); });

  // Just verify we got all keys
  EXPECT_EQ(keys.size(), 5u);
  for (int i : {1, 3, 5, 7, 9})
    EXPECT_TRUE(find(keys.begin(), keys.end(), i) != keys.end());
}

TEST(DynSetTree, Traverse)
{
  DynSetAvlTree<int> set;

  for (int i : {1, 2, 3, 4, 5})
    set.insert(i);

  int sum = 0;
  bool completed = set.traverse([&sum](const int & k) {
    sum += k;
    return true;
  });

  EXPECT_TRUE(completed);
  EXPECT_EQ(sum, 15);
}

TEST(DynSetTree, TraverseEarlyExit)
{
  DynSetAvlTree<int> set;

  for (int i : {1, 2, 3, 4, 5})
    set.insert(i);

  int count = 0;
  bool completed = set.traverse([&count](const int &) {
    ++count;
    return count < 3;  // Stop after 3 elements
  });

  EXPECT_FALSE(completed);
  EXPECT_EQ(count, 3);
}

#endif

// ============================================================================
// Range Operations Tests (only for trees with rank support)
// ============================================================================

TEST(DynSetTreapRk, SelectByPosition)
{
  DynSetTreapRk<int> set;

  for (int i : {5, 3, 7, 1, 9, 2, 8})
    set.insert(i);

  // Sorted order: 1, 2, 3, 5, 7, 8, 9
  EXPECT_EQ(set.select(0), 1);
  EXPECT_EQ(set.select(1), 2);
  EXPECT_EQ(set.select(2), 3);
  EXPECT_EQ(set.select(3), 5);
  EXPECT_EQ(set.select(6), 9);
}

TEST(DynSetTreapRk, Position)
{
  DynSetTreapRk<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  // Sorted order: 1, 3, 5, 7, 9
  EXPECT_EQ(set.position(1), 0);
  EXPECT_EQ(set.position(3), 1);
  EXPECT_EQ(set.position(5), 2);
  EXPECT_EQ(set.position(7), 3);
  EXPECT_EQ(set.position(9), 4);
  EXPECT_EQ(set.position(99), -1);  // Not found
}

TEST(DynSetTreapRk, FindPosition)
{
  DynSetTreapRk<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  auto [pos, key_ptr] = set.find_position(5);
  EXPECT_EQ(pos, 2);
  ASSERT_NE(key_ptr, nullptr);
  EXPECT_EQ(*key_ptr, 5);

  auto [pos2, key_ptr2] = set.find_position(6);  // Not found
  EXPECT_GE(pos2, 2);  // Position where it would be
  ASSERT_NE(key_ptr2, nullptr);
}

TEST(DynSetTreapRk, RemovePos)
{
  DynSetTreapRk<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  // Remove element at position 2 (which is 5 in sorted order 1,3,5,7,9)
  int removed = set.remove_pos(2);
  EXPECT_EQ(removed, 5);
  EXPECT_EQ(set.size(), 4u);
  EXPECT_FALSE(set.contains(5));
}

TEST(DynSetTreapRk, OperatorParenthesis)
{
  DynSetTreapRk<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  // Sorted order: 1, 3, 5, 7, 9
  EXPECT_EQ(set(0), 1);
  EXPECT_EQ(set(2), 5);
  EXPECT_EQ(set(4), 9);
}

// ============================================================================
// Set Operations Tests (join, split)
// ============================================================================

TEST(DynSetTreapRk, SplitKey)
{
  DynSetTreapRk<int> set;

  // Insert even numbers only, so we can split on an odd number not in the tree
  for (int i : {2, 4, 6, 8, 10, 12, 14, 16, 18})
    set.insert(i);

  DynSetTreapRk<int> left, right;

  // Split on 11, which is not in the tree
  bool success = set.split_key(11, left, right);

  EXPECT_TRUE(success);
  EXPECT_TRUE(set.is_empty());

  // left should have {2, 4, 6, 8, 10} - keys < 11
  EXPECT_EQ(left.size(), 5u);
  for (int i : {2, 4, 6, 8, 10})
    EXPECT_TRUE(left.contains(i));

  // right should have {12, 14, 16, 18} - keys > 11
  EXPECT_EQ(right.size(), 4u);
  for (int i : {12, 14, 16, 18})
    EXPECT_TRUE(right.contains(i));

  // 11 is not in either (it was never inserted)
  EXPECT_FALSE(left.contains(11));
  EXPECT_FALSE(right.contains(11));
}

TEST(DynSetTreapRk, SplitKeyDup)
{
  DynSetTreapRk<int> set;

  for (int i : {1, 2, 3, 4, 5, 6, 7, 8, 9})
    set.insert(i);

  DynSetTreapRk<int> left, right;

  set.split_key_dup(5, left, right);

  EXPECT_TRUE(set.is_empty());

  // left should have {1, 2, 3, 4, 5} - keys <= 5
  EXPECT_EQ(left.size(), 5u);
  EXPECT_TRUE(left.contains(5));

  // right should have {6, 7, 8, 9} - keys > 5
  EXPECT_EQ(right.size(), 4u);
  EXPECT_FALSE(right.contains(5));
}

TEST(DynSetTreapRk, SplitPos)
{
  DynSetTreapRk<int> set;

  for (int i : {1, 2, 3, 4, 5, 6, 7, 8, 9})
    set.insert(i);

  DynSetTreapRk<int> left, right;

  set.split_pos(4, left, right);

  EXPECT_TRUE(set.is_empty());

  // left should have first 5 elements: {1, 2, 3, 4, 5}
  EXPECT_EQ(left.size(), 5u);
  for (int i : {1, 2, 3, 4, 5})
    EXPECT_TRUE(left.contains(i));

  // right should have rest: {6, 7, 8, 9}
  EXPECT_EQ(right.size(), 4u);
  for (int i : {6, 7, 8, 9})
    EXPECT_TRUE(right.contains(i));
}

TEST(DynSetTreapRk, SplitPosEdgeCases)
{
  DynSetTreapRk<int> set;

  for (int i : {1, 2, 3, 4, 5})
    set.insert(i);

  // pos == 0 => left has {1}, right has {2,3,4,5}
  {
    DynSetTreapRk<int> tmp(set);
    DynSetTreapRk<int> left, right;
    tmp.split_pos(0, left, right);
    EXPECT_TRUE(tmp.is_empty());
    EXPECT_EQ(left.size(), 1u);
    EXPECT_EQ(right.size(), 4u);
    EXPECT_TRUE(left.contains(1));
    EXPECT_TRUE(right.contains(5));
  }

  // pos == size-1 => left has all, right empty
  {
    DynSetTreapRk<int> tmp(set);
    DynSetTreapRk<int> left, right;
    tmp.split_pos(tmp.size() - 1, left, right);
    EXPECT_TRUE(tmp.is_empty());
    EXPECT_EQ(left.size(), 5u);
    EXPECT_TRUE(right.is_empty());
  }

  // out of range
  {
    DynSetTreapRk<int> tmp(set);
    DynSetTreapRk<int> left, right;
    EXPECT_THROW(tmp.split_pos(tmp.size(), left, right), out_of_range);
    EXPECT_THROW(tmp.split_pos(tmp.size() + 10, left, right), out_of_range);
  }
}

TEST(DynSetTreapRk, RankOperationsOutOfRange)
{
  DynSetTreapRk<int> set;
  for (int i : {1, 2, 3})
    set.insert(i);

  EXPECT_THROW(set.select(3), out_of_range);
  EXPECT_THROW(set.remove_pos(3), out_of_range);

  // empty
  DynSetTreapRk<int> empty;
  EXPECT_THROW(empty.select(0), out_of_range);
  EXPECT_THROW(empty.remove_pos(0), out_of_range);
  DynSetTreapRk<int> left, right;
  EXPECT_THROW(empty.split_pos(0, left, right), out_of_range);
}

TEST(DynSetTreapRk, Join)
{
  DynSetTreapRk<int> set1, set2;

  for (int i : {1, 3, 5})
    set1.insert(i);

  for (int i : {2, 4, 6})
    set2.insert(i);

  DynSetTreapRk<int> dup;
  set1.join(set2, dup);

  EXPECT_TRUE(set2.is_empty());
  EXPECT_EQ(set1.size(), 6u);
  for (int i : {1, 2, 3, 4, 5, 6})
    EXPECT_TRUE(set1.contains(i));

  EXPECT_TRUE(dup.is_empty());
}

TEST(DynSetTreapRk, JoinWithDuplicates)
{
  DynSetTreapRk<int> set1, set2;

  for (int i : {1, 2, 3, 4, 5})
    set1.insert(i);

  for (int i : {3, 4, 5, 6, 7})
    set2.insert(i);

  DynSetTreapRk<int> dup;
  set1.join(set2, dup);

  EXPECT_TRUE(set2.is_empty());
  EXPECT_EQ(set1.size(), 7u);
  for (int i : {1, 2, 3, 4, 5, 6, 7})
    EXPECT_TRUE(set1.contains(i));

  EXPECT_EQ(dup.size(), 3u);
  for (int i : {3, 4, 5})
    EXPECT_TRUE(dup.contains(i));
}

TEST(DynSetTreapRk, JoinDup)
{
  DynSetTreapRk<int> set1, set2;

  for (int i : {1, 2, 3})
    set1.insert(i);

  for (int i : {3, 4, 5})
    set2.insert(i);

  set1.join_dup(set2);

  EXPECT_TRUE(set2.is_empty());
  EXPECT_EQ(set1.size(), 6u);  // Duplicates allowed (3 appears twice)
}

// =========================================================================
// insert_dup() Tests
// =========================================================================

template <typename T>
static void insert_dup_traversal_test()
{
  T set;
  set.insert_dup(42);
  set.insert_dup(42);
  set.insert_dup(7);

  EXPECT_EQ(set.size(), 3u);

  size_t count_42 = 0;
  for (typename T::Iterator it(set); it.has_curr(); it.next_ne())
    if (it.get_curr() == 42)
      ++count_42;

  EXPECT_EQ(count_42, 2u);
}

TEST(DynSetTree, InsertDupTraversesDuplicates)
{
  insert_dup_traversal_test<DynSetRbTree<int>>();
  insert_dup_traversal_test<DynSetTreap<int>>();
  insert_dup_traversal_test<DynSetTreapRk<int>>();
  insert_dup_traversal_test<DynSetSplayTree<int>>();
  insert_dup_traversal_test<DynSetRandTree<int>>();
  insert_dup_traversal_test<DynSetBinTree<int>>();
  insert_dup_traversal_test<DynSetAvlTree<int>>();
}

// =========================================================================
// Range APIs on non-rank trees must throw domain_error
// =========================================================================

template <typename T>
static void range_methods_throw_domain_error_test()
{
  T set;

  // On empty: must still report "unsupported" rather than pretending it works.
  EXPECT_THROW(set.position(1), domain_error);
  EXPECT_THROW(set.find_position(1), domain_error);
  EXPECT_THROW(set.select(0), domain_error);
  EXPECT_THROW(set.remove_pos(0), domain_error);
  T left, right;
  EXPECT_THROW(set.split_pos(0, left, right), domain_error);
}

TEST(DynSetTree, RangeMethodsThrowOnNonRankTrees)
{
  range_methods_throw_domain_error_test<DynSetAvlTree<int>>();
  range_methods_throw_domain_error_test<DynSetRbTree<int>>();
  range_methods_throw_domain_error_test<DynSetSplayTree<int>>();
  range_methods_throw_domain_error_test<DynSetTreap<int>>();
  range_methods_throw_domain_error_test<DynSetBinTree<int>>();
}

// ============================================================================
// Different Tree Types Tests
// ============================================================================

#if 0

TEST(DynSetBinTree, BasicOperations)
{
  DynSetBinTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  EXPECT_EQ(set.size(), 5u);
  EXPECT_TRUE(set.contains(3));
  EXPECT_FALSE(set.contains(4));
}

TEST(DynSetSplayTree, BasicOperations)
{
  DynSetSplayTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  EXPECT_EQ(set.size(), 5u);
  EXPECT_TRUE(set.contains(3));
}

TEST(DynSetRandTree, BasicOperations)
{
  DynSetRandTree<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  EXPECT_EQ(set.size(), 5u);
  EXPECT_TRUE(set.contains(3));
}

TEST(DynSetTreap, BasicOperations)
{
  DynSetTreap<int> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  EXPECT_EQ(set.size(), 5u);
  EXPECT_TRUE(set.contains(3));
}

#endif

// ============================================================================
// Custom Comparator Tests
// ============================================================================

TEST(DynSetTree, CustomComparatorGreater)
{
  DynSetTree<int, Avl_Tree, std::greater<int>> set;

  for (int i : {5, 3, 7, 1, 9})
    set.insert(i);

  EXPECT_EQ(set.min(), 9);  // Max in normal order
  EXPECT_EQ(set.max(), 1);  // Min in normal order

  vector<int> keys;
  for (DynSetTree<int, Avl_Tree, std::greater<int>>::Iterator it(set); it.has_curr(); it.next_ne())
    keys.push_back(it.get_curr());

  vector<int> expected = {9, 7, 5, 3, 1};
  EXPECT_EQ(keys, expected);
}

// ============================================================================
// Operator[] Tests
// ============================================================================

TEST(DynSetTree, OperatorBracketConstThrows)
{
  const DynSetAvlTree<int> set;

  EXPECT_THROW(set[42], domain_error);
}

TEST(DynSetTree, OperatorBracketNonConstInserts)
{
  DynSetAvlTree<int> set;

  EXPECT_FALSE(set.contains(42));

  const int & ref = set[42];
  EXPECT_EQ(ref, 42);
  EXPECT_TRUE(set.contains(42));
  EXPECT_EQ(set.size(), 1u);
}

// ============================================================================
// Verify Tests
// ============================================================================

TEST(DynSetTree, VerifyValidTree)
{
  DynSetAvlTree<int> set;

  for (int i : {5, 3, 7, 1, 9, 2, 8, 4, 6})
    set.insert(i);

  EXPECT_TRUE(set.verify());
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(DynSetTree, LargeSetOperations)
{
  DynSetRbTree<int> set;

  // Insert 1000 elements
  for (int i = 0; i < 1000; ++i)
    set.insert(i);

  EXPECT_EQ(set.size(), 1000u);
  EXPECT_EQ(set.min(), 0);
  EXPECT_EQ(set.max(), 999);

  // Verify all present
  for (int i = 0; i < 1000; ++i)
    EXPECT_TRUE(set.contains(i));

  // Remove half
  for (int i = 0; i < 1000; i += 2)
    set.remove(i);

  EXPECT_EQ(set.size(), 500u);

  // Verify correct ones remain
  for (int i = 0; i < 1000; ++i)
    {
      if (i % 2 == 0)
        EXPECT_FALSE(set.contains(i));
      else
        EXPECT_TRUE(set.contains(i));
    }
}

TEST(DynSetTree, RandomInsertRemove)
{
  DynSetAvlTree<int> set;
  vector<int> inserted;

  // Random insertions
  for (int i = 0; i < 200; ++i)
    {
      if (int key = rand() % 500; set.insert(key) != nullptr)
        inserted.push_back(key);
    }

  // Verify all inserted keys are present
  for (int key : inserted)
    EXPECT_TRUE(set.contains(key));

  EXPECT_EQ(set.size(), inserted.size());

  // Random removals
  for (size_t i = 0; i < inserted.size() / 2; ++i)
    {
      int idx = rand() % inserted.size();
      set.remove(inserted[idx]);
      inserted.erase(inserted.begin() + idx);
    }

  EXPECT_EQ(set.size(), inserted.size());
  for (int key : inserted)
    EXPECT_TRUE(set.contains(key));
}

// ============================================================================
// String Key Tests
// ============================================================================

TEST(DynSetTree, StringKeys)
{
  DynSetAvlTree<string> set;

  set.insert("apple");
  set.insert("banana");
  set.insert("cherry");

  EXPECT_EQ(set.size(), 3u);
  EXPECT_TRUE(set.contains("apple"));
  EXPECT_TRUE(set.contains("banana"));
  EXPECT_FALSE(set.contains("date"));

  EXPECT_EQ(set.min(), "apple");
  EXPECT_EQ(set.max(), "cherry");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(DynSetTree, SingleElement)
{
  DynSetAvlTree<int> set;

  set.insert(42);

  EXPECT_EQ(set.min(), 42);
  EXPECT_EQ(set.max(), 42);
  EXPECT_EQ(set.get_root(), 42);
  EXPECT_EQ(set.size(), 1u);

  set.remove(42);
  EXPECT_TRUE(set.is_empty());
}

TEST(DynSetTree, InsertRemoveRepeated)
{
  DynSetAvlTree<int> set;

  for (int cycle = 0; cycle < 10; ++cycle)
    {
      for (int i = 0; i < 50; ++i)
        set.insert(i);

      EXPECT_EQ(set.size(), 50u);

      for (int i = 0; i < 50; ++i)
        set.remove(i);

      EXPECT_TRUE(set.is_empty());
    }
}

TEST(DynSetTree, AccessMethods)
{
  DynSetAvlTree<int> set;

  for (int i : {1, 2, 3, 4, 5})
    set.insert(i);

  EXPECT_EQ(set.get_first(), 1);
  EXPECT_EQ(set.get_last(), 5);
  EXPECT_EQ(set.get(), 5);
  EXPECT_NO_THROW(set.get_item());
}

namespace {

struct TrackedKey
{
  int v;
  inline static int alive = 0;

  TrackedKey() : v(0) { ++alive; }
  explicit TrackedKey(int vv) : v(vv) { ++alive; }
  TrackedKey(const TrackedKey & other) : v(other.v) { ++alive; }
  TrackedKey(TrackedKey && other) noexcept : v(other.v) { ++alive; }
  ~TrackedKey() { --alive; }
};

inline bool operator < (const TrackedKey & a, const TrackedKey & b)
{
  return a.v < b.v;
}

template <typename Key, class Compare>
class ThrowingSearchOrInsertTree
{
public:

  using Node = BinNode<Key>;

  inline static bool enabled = false;

  ThrowingSearchOrInsertTree(Compare c = Compare()) : cmp(c) { /* empty */ }

  Compare get_compare() const { return cmp; }

  Node *& getRoot() noexcept { return root; }
  Node * getRoot() const noexcept { return root; }

  void swap(ThrowingSearchOrInsertTree & other) noexcept
  {
    std::swap(root, other.root);
    std::swap(cmp, other.cmp);
  }

  Node * search_or_insert(Node * p)
  {
    if (enabled)
      throw std::runtime_error("search_or_insert failed");

    if (root == Node::NullPtr)
      return root = p;

    return root;
  }

  bool verify() const { return true; }

private:

  Node * root = Node::NullPtr;
  Compare cmp;
};

using ThrowingSet = DynSetTree<TrackedKey, ThrowingSearchOrInsertTree, Aleph::less<TrackedKey>>;

} // namespace

TEST(DynSetTreeHardening, InsertDoesNotLeakOnThrow)
{
  TrackedKey::alive = 0;
  ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = false;

  {
    ThrowingSet set;
    const int baseline = TrackedKey::alive;
    set.insert(TrackedKey(1));
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(TrackedKey::alive, baseline + 1);

    ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = true;
    EXPECT_THROW((void) set.insert(TrackedKey(2)), std::runtime_error);
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(TrackedKey::alive, baseline + 1);
  }

  ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = false;
  EXPECT_EQ(TrackedKey::alive, 0);
}

TEST(DynSetTreeHardening, SearchOrInsertDoesNotLeakOnThrow)
{
  TrackedKey::alive = 0;
  ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = false;

  {
    ThrowingSet set;
    const int baseline = TrackedKey::alive;
    set.insert(TrackedKey(1));
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(TrackedKey::alive, baseline + 1);

    ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = true;
    EXPECT_THROW((void) set.search_or_insert(TrackedKey(2)), std::runtime_error);
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(TrackedKey::alive, baseline + 1);
  }

  ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = false;
  EXPECT_EQ(TrackedKey::alive, 0);
}

TEST(DynSetTreeHardening, ContainsOrInsertDoesNotLeakOnThrow)
{
  TrackedKey::alive = 0;
  ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = false;

  {
    ThrowingSet set;
    const int baseline = TrackedKey::alive;
    set.insert(TrackedKey(1));
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(TrackedKey::alive, baseline + 1);

    ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = true;
    EXPECT_THROW((void) set.contains_or_insert(TrackedKey(2)), std::runtime_error);
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(TrackedKey::alive, baseline + 1);
  }

  ThrowingSearchOrInsertTree<TrackedKey, Aleph::less<TrackedKey>>::enabled = false;
  EXPECT_EQ(TrackedKey::alive, 0);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}