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
 * @file skiplist_test.cc
 * @brief Tests for Skiplist
 */
#include <gtest/gtest.h>
#include <gsl/gsl_rng.h>
#include <climits>
#include <vector>
#include <algorithm>
#include <string>
#include <tpl_skipList.H>

using namespace Aleph;

// Specialize computeMaxKey for int
template<>
int SkipList<int, int>::Node::computeMaxKey() noexcept
{
  return INT_MAX;
}

// Specialize computeMaxKey for std::string keys
template<>
std::string SkipList<std::string, int>::Node::computeMaxKey() noexcept
{
  return std::string(256, '\xFF');  // Max string
}

namespace {

// Fixed seed for reproducible tests
constexpr unsigned long TEST_SEED = 42;

// Helper to allocate a SkipList node with proper size
template <class Key, class Type>
typename SkipList<Key, Type>::Node* allocate_node(const Key& key, const Type& data, int level)
{
  // Node uses flexible array member, need extra space for forward pointers
  size_t size = sizeof(typename SkipList<Key, Type>::Node) + 
                sizeof(typename SkipList<Key, Type>::Node*) * level;
  void* mem = ::operator new(size);
  return new (mem) typename SkipList<Key, Type>::Node(key, data, level);
}

template <class Key, class Type>
void deallocate_node(typename SkipList<Key, Type>::Node* node)
{
  if (node)
    {
      node->~Node();
      ::operator delete(node);
    }
}

// Test fixture
class SkipListTest : public ::testing::Test
{
protected:
  using SL = SkipList<int, int>;
  
  SL* skiplist = nullptr;
  std::vector<SL::Node*> allocated_nodes;

  void SetUp() override
  {
    skiplist = new SL(TEST_SEED, 0.5);
  }

  void TearDown() override
  {
    // Clean up all allocated nodes
    for (auto* node : allocated_nodes)
      deallocate_node<int, int>(node);
    allocated_nodes.clear();
    
    delete skiplist;
  }

  SL::Node* create_node(int key, int data = 0)
  {
    int level = skiplist->generateRandomLevel();
    auto* node = allocate_node<int, int>(key, data, level);
    allocated_nodes.push_back(node);
    return node;
  }

  SL::Node* create_node_with_level(int key, int data, int level)
  {
    auto* node = allocate_node<int, int>(key, data, level);
    allocated_nodes.push_back(node);
    return node;
  }
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(SkipListTest, ConstructorDefault)
{
  EXPECT_TRUE(skiplist->checkSkipList());
}

TEST_F(SkipListTest, ConstructorWithProbability)
{
  SkipList<int, int> sl(TEST_SEED, 0.25);
  EXPECT_TRUE(sl.checkSkipList());
}

TEST_F(SkipListTest, ConstructorWithDefaultSeed)
{
  SkipList<int, int> sl;  // Uses time-based seed
  EXPECT_TRUE(sl.checkSkipList());
}

TEST_F(SkipListTest, SetSeed)
{
  skiplist->set_seed(123);
  int level1 = skiplist->generateRandomLevel();
  skiplist->set_seed(123);
  int level2 = skiplist->generateRandomLevel();
  EXPECT_EQ(level1, level2);  // Same seed should give same result
}

TEST_F(SkipListTest, GslRngObject)
{
  gsl_rng* rng = skiplist->gsl_rng_object();
  EXPECT_NE(rng, nullptr);
}

// ============================================================================
// Insert Tests
// ============================================================================

TEST_F(SkipListTest, InsertSingleElement)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  EXPECT_TRUE(skiplist->checkSkipList());
  EXPECT_EQ(skiplist->get_first(), node);
}

TEST_F(SkipListTest, InsertMultipleElementsAscending)
{
  for (int i = 1; i <= 10; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Verify first element
  auto* first = skiplist->get_first();
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->get_key(), 1);
}

TEST_F(SkipListTest, InsertMultipleElementsDescending)
{
  for (int i = 10; i >= 1; --i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Verify first element is smallest
  auto* first = skiplist->get_first();
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->get_key(), 1);
}

TEST_F(SkipListTest, InsertMultipleElementsRandom)
{
  std::vector<int> keys = {5, 2, 8, 1, 9, 3, 7, 4, 6, 10};
  
  for (int key : keys)
    {
      auto* node = create_node(key, key * 10);
      skiplist->insert(node);
    }
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Verify order by traversing
  auto* current = skiplist->get_first();
  int expected = 1;
  while (current != nullptr && current->get_key() < INT_MAX)
    {
      EXPECT_EQ(current->get_key(), expected);
      ++expected;
      current = current->get_next();
    }
  EXPECT_EQ(expected, 11);  // Should have seen 1-10
}

// ============================================================================
// Search Tests
// ============================================================================

TEST_F(SkipListTest, SearchEmptyList)
{
  auto* result = skiplist->search(42);
  EXPECT_EQ(result, nullptr);
}

TEST_F(SkipListTest, SearchExistingElement)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  auto* found = skiplist->search(42);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_key(), 42);
  EXPECT_EQ(found->get_data(), 100);
}

TEST_F(SkipListTest, SearchNonExistingElement)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  EXPECT_EQ(skiplist->search(41), nullptr);
  EXPECT_EQ(skiplist->search(43), nullptr);
  EXPECT_EQ(skiplist->search(0), nullptr);
  EXPECT_EQ(skiplist->search(100), nullptr);
}

TEST_F(SkipListTest, SearchMultipleElements)
{
  for (int i = 1; i <= 100; ++i)
    {
      auto* node = create_node(i * 2, i * 100);  // Even keys only
      skiplist->insert(node);
    }
  
  // Search for existing elements
  for (int i = 1; i <= 100; ++i)
    {
      auto* found = skiplist->search(i * 2);
      ASSERT_NE(found, nullptr) << "Key " << i * 2 << " not found";
      EXPECT_EQ(found->get_key(), i * 2);
      EXPECT_EQ(found->get_data(), i * 100);
    }
  
  // Search for non-existing elements (odd numbers)
  for (int i = 1; i <= 100; ++i)
    {
      auto* found = skiplist->search(i * 2 - 1);
      EXPECT_EQ(found, nullptr) << "Key " << i * 2 - 1 << " should not exist";
    }
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST_F(SkipListTest, RemoveFromEmptyList)
{
  auto* result = skiplist->remove(42);
  EXPECT_EQ(result, nullptr);
}

TEST_F(SkipListTest, RemoveSingleElement)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  auto* removed = skiplist->remove(42);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->get_key(), 42);
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Should not find it anymore
  EXPECT_EQ(skiplist->search(42), nullptr);
}

TEST_F(SkipListTest, RemoveNonExistingElement)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  EXPECT_EQ(skiplist->remove(41), nullptr);
  EXPECT_EQ(skiplist->remove(43), nullptr);
  
  // Original should still exist
  EXPECT_NE(skiplist->search(42), nullptr);
}

TEST_F(SkipListTest, RemoveFirstElement)
{
  for (int i = 1; i <= 5; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  auto* removed = skiplist->remove(1);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->get_key(), 1);
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // New first should be 2
  auto* first = skiplist->get_first();
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->get_key(), 2);
}

TEST_F(SkipListTest, RemoveLastElement)
{
  for (int i = 1; i <= 5; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  auto* removed = skiplist->remove(5);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->get_key(), 5);
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // 5 should not be found
  EXPECT_EQ(skiplist->search(5), nullptr);
}

TEST_F(SkipListTest, RemoveMiddleElement)
{
  for (int i = 1; i <= 5; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  auto* removed = skiplist->remove(3);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->get_key(), 3);
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // 3 should not be found, but 2 and 4 should
  EXPECT_EQ(skiplist->search(3), nullptr);
  EXPECT_NE(skiplist->search(2), nullptr);
  EXPECT_NE(skiplist->search(4), nullptr);
}

TEST_F(SkipListTest, RemoveAllElements)
{
  for (int i = 1; i <= 10; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  // Remove in random order
  std::vector<int> order = {5, 1, 9, 3, 7, 2, 8, 4, 6, 10};
  for (int key : order)
    {
      auto* removed = skiplist->remove(key);
      ASSERT_NE(removed, nullptr) << "Failed to remove key " << key;
      EXPECT_TRUE(skiplist->checkSkipList());
    }
}

// ============================================================================
// Level Generation Tests
// ============================================================================

TEST_F(SkipListTest, GenerateRandomLevelBounds)
{
  constexpr int max_level = 32;  // Same as SL::maxLevel
  for (int i = 0; i < 1000; ++i)
    {
      int level = skiplist->generateRandomLevel();
      EXPECT_GE(level, 1);
      EXPECT_LE(level, max_level);
    }
}

TEST_F(SkipListTest, GenerateRandomLevelDistribution)
{
  // With p=0.5, about 50% should be level 1, 25% level 2, etc.
  constexpr int max_level = 32;  // Same as SL::maxLevel
  std::vector<int> counts(max_level + 1, 0);
  const int trials = 10000;
  
  for (int i = 0; i < trials; ++i)
    {
      int level = skiplist->generateRandomLevel();
      counts[level]++;
    }
  
  // Level 1 should be most common (roughly 50%)
  EXPECT_GT(counts[1], trials * 0.4);  // At least 40%
  EXPECT_LT(counts[1], trials * 0.6);  // At most 60%
  
  // Level 2 should be about half of level 1
  double ratio = static_cast<double>(counts[2]) / counts[1];
  EXPECT_GT(ratio, 0.35);
  EXPECT_LT(ratio, 0.65);
}

// ============================================================================
// Node Tests
// ============================================================================

TEST_F(SkipListTest, NodeGettersSetters)
{
  auto* node = create_node_with_level(42, 100, 5);
  
  EXPECT_EQ(node->get_key(), 42);
  EXPECT_EQ(node->get_data(), 100);
  EXPECT_EQ(node->getLevel(), 5);
  
  // Modify data
  node->get_data() = 200;
  EXPECT_EQ(node->get_data(), 200);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(SkipListTest, InterleavedInsertRemove)
{
  // Insert some elements
  for (int i = 1; i <= 5; ++i)
    {
      auto* node = create_node(i, i);
      skiplist->insert(node);
    }
  
  // Remove even, insert more
  (void)skiplist->remove(2);
  (void)skiplist->remove(4);
  
  auto* node6 = create_node(6, 6);
  auto* node7 = create_node(7, 7);
  skiplist->insert(node6);
  skiplist->insert(node7);
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Check expected state
  EXPECT_NE(skiplist->search(1), nullptr);
  EXPECT_EQ(skiplist->search(2), nullptr);
  EXPECT_NE(skiplist->search(3), nullptr);
  EXPECT_EQ(skiplist->search(4), nullptr);
  EXPECT_NE(skiplist->search(5), nullptr);
  EXPECT_NE(skiplist->search(6), nullptr);
  EXPECT_NE(skiplist->search(7), nullptr);
}

TEST_F(SkipListTest, LargeScale)
{
  const int N = 1000;
  
  // Insert N elements
  for (int i = 0; i < N; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Search all
  for (int i = 0; i < N; ++i)
    {
      auto* found = skiplist->search(i);
      ASSERT_NE(found, nullptr) << "Key " << i << " not found";
    }
  
  // Remove half (even numbers)
  for (int i = 0; i < N; i += 2)
    {
      auto* removed = skiplist->remove(i);
      ASSERT_NE(removed, nullptr) << "Failed to remove key " << i;
    }
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Verify remaining
  for (int i = 0; i < N; ++i)
    {
      auto* found = skiplist->search(i);
      if (i % 2 == 0)
        EXPECT_EQ(found, nullptr) << "Key " << i << " should have been removed";
      else
        EXPECT_NE(found, nullptr) << "Key " << i << " should still exist";
    }
}

// ============================================================================
// String Key Tests
// ============================================================================

class SkipListStringTest : public ::testing::Test
{
protected:
  using SL = SkipList<std::string, int>;
  
  SL* skiplist = nullptr;
  std::vector<SL::Node*> allocated_nodes;

  void SetUp() override
  {
    skiplist = new SL(TEST_SEED, 0.5);
  }

  void TearDown() override
  {
    for (auto* node : allocated_nodes)
      deallocate_node<std::string, int>(node);
    allocated_nodes.clear();
    delete skiplist;
  }

  SL::Node* create_node(const std::string& key, int data = 0)
  {
    int level = skiplist->generateRandomLevel();
    auto* node = allocate_node<std::string, int>(key, data, level);
    allocated_nodes.push_back(node);
    return node;
  }
};

TEST_F(SkipListStringTest, StringKeys)
{
  auto* node1 = create_node("apple", 1);
  auto* node2 = create_node("banana", 2);
  auto* node3 = create_node("cherry", 3);
  
  skiplist->insert(node2);
  skiplist->insert(node1);
  skiplist->insert(node3);
  
  EXPECT_TRUE(skiplist->checkSkipList());
  
  // Should be in alphabetical order
  auto* first = skiplist->get_first();
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->get_key(), "apple");
  
  // Search
  auto* found = skiplist->search("banana");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_data(), 2);
  
  EXPECT_EQ(skiplist->search("date"), nullptr);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(SkipListTest, IteratorEmpty)
{
  SL::Iterator it(*skiplist);
  EXPECT_FALSE(it.has_curr());
  EXPECT_EQ(it.get_curr_ne(), nullptr);
}

TEST_F(SkipListTest, IteratorSingleElement)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  SL::Iterator it(*skiplist);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), node);
  EXPECT_EQ(it.get_key(), 42);
  EXPECT_EQ(it.get_data(), 100);
  EXPECT_TRUE(it.is_last());
  
  it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST_F(SkipListTest, IteratorMultipleElements)
{
  for (int i = 1; i <= 5; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  // Count elements via iterator
  int count = 0;
  int expected_key = 1;
  for (SL::Iterator it(*skiplist); it.has_curr(); it.next())
    {
      EXPECT_EQ(it.get_key(), expected_key);
      EXPECT_EQ(it.get_data(), expected_key * 10);
      ++expected_key;
      ++count;
    }
  EXPECT_EQ(count, 5);
}

TEST_F(SkipListTest, IteratorRangeBasedFor)
{
  for (int i = 1; i <= 5; ++i)
    {
      auto* node = create_node(i, i * 10);
      skiplist->insert(node);
    }
  
  // Use range-based for with begin()/end()
  int count = 0;
  int expected_key = 1;
  for (auto it = skiplist->begin(); it != skiplist->end(); ++it)
    {
      EXPECT_EQ((*it)->get_key(), expected_key);
      ++expected_key;
      ++count;
    }
  EXPECT_EQ(count, 5);
}

TEST_F(SkipListTest, IteratorReset)
{
  for (int i = 1; i <= 3; ++i)
    {
      auto* node = create_node(i, i);
      skiplist->insert(node);
    }
  
  SL::Iterator it(*skiplist);
  EXPECT_EQ(it.get_key(), 1);
  
  it.next();
  EXPECT_EQ(it.get_key(), 2);
  
  it.reset();
  EXPECT_EQ(it.get_key(), 1);
}

TEST_F(SkipListTest, IteratorCopy)
{
  for (int i = 1; i <= 3; ++i)
    {
      auto* node = create_node(i, i);
      skiplist->insert(node);
    }
  
  SL::Iterator it1(*skiplist);
  it1.next();  // Now on key 2
  
  SL::Iterator it2 = it1;  // Copy
  EXPECT_EQ(it2.get_key(), 2);
  
  it1.next();  // it1 on key 3
  EXPECT_EQ(it1.get_key(), 3);
  EXPECT_EQ(it2.get_key(), 2);  // it2 unchanged
}

TEST_F(SkipListTest, IteratorEquality)
{
  auto* node = create_node(42, 100);
  skiplist->insert(node);
  
  SL::Iterator it1(*skiplist);
  SL::Iterator it2(*skiplist);
  
  EXPECT_TRUE(it1 == it2);
  EXPECT_FALSE(it1 != it2);
  
  it1.next();
  EXPECT_FALSE(it1 == it2);
  EXPECT_TRUE(it1 != it2);
}

TEST_F(SkipListTest, IteratorOperators)
{
  for (int i = 1; i <= 3; ++i)
    {
      auto* node = create_node(i, i);
      skiplist->insert(node);
    }
  
  SL::Iterator it(*skiplist);
  
  // Pre-increment
  EXPECT_EQ(it.get_key(), 1);
  ++it;
  EXPECT_EQ(it.get_key(), 2);
  
  // Post-increment
  SL::Iterator it2 = it++;
  EXPECT_EQ(it2.get_key(), 2);  // Old value
  EXPECT_EQ(it.get_key(), 3);   // New value
}

TEST_F(SkipListTest, IteratorThrowsOnOverflow)
{
  SL::Iterator it(*skiplist);  // Empty list
  
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  
  // No throw versions should not throw
  EXPECT_NO_THROW(it.get_curr_ne());
  EXPECT_NO_THROW(it.next_ne());
}

TEST_F(SkipListTest, IteratorLargeScale)
{
  const int N = 500;
  for (int i = 0; i < N; ++i)
    {
      auto* node = create_node(i, i);
      skiplist->insert(node);
    }
  
  int count = 0;
  int expected = 0;
  for (SL::Iterator it(*skiplist); it.has_curr(); it.next())
    {
      EXPECT_EQ(it.get_key(), expected);
      ++expected;
      ++count;
    }
  EXPECT_EQ(count, N);
}

} // anonymous namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
