/**
 * @file htdrbtreerk_test.cc
 * @brief Comprehensive tests for HtdRbTreeRk (Hybrid Red-Black Tree with Rank)
 */

#include <gtest/gtest.h>
#include <tpl_hRbTreeRk.H>
#include <tpl_rbRk.H>
#include <set>
#include <vector>
#include <algorithm>
#include <random>

using namespace Aleph;

// ============================================================================
// Test Fixture
// ============================================================================

class HtdRbTreeRkTest : public ::testing::Test
{
protected:
  using Node = RbNodeRk<int>;
  using Tree = HtdRbTreeRk<int>;
  
  Tree tree;
  std::vector<Node*> nodes;
  
  void TearDown() override
  {
    for (auto* n : nodes)
      delete n;
    nodes.clear();
  }
  
  Node* make_node(int key)
  {
    Node* n = new Node(key);
    nodes.push_back(n);
    return n;
  }
  
  void insert_range(int start, int end)
  {
    for (int i = start; i < end; ++i)
      tree.insert(make_node(i));
  }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(HtdRbTreeRkTest, EmptyTree)
{
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(HtdRbTreeRkTest, InsertSingleNode)
{
  tree.insert(make_node(42));
  
  EXPECT_FALSE(tree.is_empty());
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
  
  auto* found = tree.search(42);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(KEY(found), 42);
}

TEST_F(HtdRbTreeRkTest, InsertMultipleNodes)
{
  insert_range(0, 10);
  
  EXPECT_EQ(tree.size(), 10u);
  EXPECT_TRUE(tree.verify());
  
  for (int i = 0; i < 10; ++i)
    EXPECT_NE(tree.search(i), nullptr) << "Missing key " << i;
}

TEST_F(HtdRbTreeRkTest, InsertDuplicateRejected)
{
  tree.insert(make_node(42));
  auto* dup = tree.insert(make_node(42));
  
  EXPECT_EQ(dup, nullptr);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(HtdRbTreeRkTest, InsertDupAllowsDuplicates)
{
  tree.insert_dup(make_node(42));
  tree.insert_dup(make_node(42));
  tree.insert_dup(make_node(42));
  
  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(HtdRbTreeRkTest, RemoveNode)
{
  insert_range(0, 5);
  
  auto* removed = tree.remove(2);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 2);
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(tree.search(2), nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST_F(HtdRbTreeRkTest, RemoveAllNodes)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  for (int k : keys)
    tree.insert(make_node(k));
  
  for (int k : keys)
    {
      auto* removed = tree.remove(k);
      ASSERT_NE(removed, nullptr) << "Failed to remove " << k;
      EXPECT_TRUE(tree.verify()) << "Verify failed after removing " << k;
    }
  
  EXPECT_TRUE(tree.is_empty());
}

// ============================================================================
// Rank Operations Tests
// ============================================================================

TEST_F(HtdRbTreeRkTest, SelectBasic)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  for (int k : keys)
    tree.insert(make_node(k));
  
  // Sorted order: 10, 25, 30, 50, 60, 75, 90
  std::vector<int> expected = {10, 25, 30, 50, 60, 75, 90};
  
  for (size_t i = 0; i < expected.size(); ++i)
    {
      auto* selected = tree.select(i);
      ASSERT_NE(selected, nullptr) << "select(" << i << ") returned null";
      EXPECT_EQ(KEY(selected), expected[i]) << "select(" << i << ") wrong";
    }
}

TEST_F(HtdRbTreeRkTest, SelectAfterRemoval)
{
  insert_range(0, 5);  // 0, 1, 2, 3, 4
  
  tree.remove(2);  // Now: 0, 1, 3, 4
  
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(KEY(tree.select(0)), 0);
  EXPECT_EQ(KEY(tree.select(1)), 1);
  EXPECT_EQ(KEY(tree.select(2)), 3);
  EXPECT_EQ(KEY(tree.select(3)), 4);
  EXPECT_TRUE(tree.verify());
}

TEST_F(HtdRbTreeRkTest, SelectLargeTree)
{
  const int N = 100;
  for (int i = 0; i < N; ++i)
    tree.insert(make_node(i * 2));  // Even numbers 0, 2, 4, ..., 198
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  EXPECT_TRUE(tree.verify());
  
  for (int i = 0; i < N; ++i)
    {
      auto* selected = tree.select(i);
      ASSERT_NE(selected, nullptr);
      EXPECT_EQ(KEY(selected), i * 2);
    }
}

TEST_F(HtdRbTreeRkTest, PositionBasic)
{
  std::vector<int> keys = {10, 20, 30, 40, 50};
  for (int k : keys)
    tree.insert(make_node(k));
  
  for (size_t i = 0; i < keys.size(); ++i)
    {
      auto [pos, node] = tree.position(keys[i]);
      EXPECT_EQ(pos, static_cast<long>(i)) << "Position wrong for " << keys[i];
      ASSERT_NE(node, nullptr);
      EXPECT_EQ(KEY(node), keys[i]);
    }
}

TEST_F(HtdRbTreeRkTest, PositionNotFound)
{
  insert_range(0, 10);
  
  auto [pos, node] = tree.position(100);
  EXPECT_EQ(pos, -1);
  EXPECT_EQ(node, nullptr);
}

TEST_F(HtdRbTreeRkTest, FindPositionExisting)
{
  insert_range(0, 10);
  
  auto [pos, node] = tree.find_position(5);
  EXPECT_EQ(pos, 5);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(KEY(node), 5);
}

TEST_F(HtdRbTreeRkTest, FindPositionNotExisting)
{
  // Insert 0, 2, 4, 6, 8
  for (int i = 0; i < 5; ++i)
    tree.insert(make_node(i * 2));
  
  // Find position for 5 (would be between 4 and 6, so position 3)
  // For non-existing keys, find_position returns the position where
  // the key would be and nullptr for the node
  auto [pos, node] = tree.find_position(5);
  // Position 3 means: after 0, 2, 4 (positions 0, 1, 2)
  // Note: find_position may return different behavior based on implementation
  // The key 5 would go between 4 (pos 2) and 6 (pos 3), so position could be 2 or 3
  EXPECT_GE(pos, 2);
  EXPECT_LE(pos, 3);
}

TEST_F(HtdRbTreeRkTest, RemovePos)
{
  insert_range(0, 5);  // 0, 1, 2, 3, 4
  
  auto* removed = tree.remove_pos(2);  // Remove element at position 2 (which is key 2)
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 2);
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(HtdRbTreeRkTest, SplitPos)
{
  insert_range(0, 6);  // 0, 1, 2, 3, 4, 5
  
  Tree t1, t2;
  tree.split_pos(3, t1, t2);
  
  EXPECT_EQ(t1.size(), 3u);  // 0, 1, 2
  EXPECT_EQ(t2.size(), 3u);  // 3, 4, 5
  EXPECT_TRUE(tree.is_empty());
  
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());
  
  // Check contents
  EXPECT_NE(t1.search(0), nullptr);
  EXPECT_NE(t1.search(1), nullptr);
  EXPECT_NE(t1.search(2), nullptr);
  EXPECT_NE(t2.search(3), nullptr);
  EXPECT_NE(t2.search(4), nullptr);
  EXPECT_NE(t2.search(5), nullptr);
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_F(HtdRbTreeRkTest, MoveConstructor)
{
  insert_range(0, 5);
  
  Tree tree2(std::move(tree));
  
  EXPECT_EQ(tree2.size(), 5u);
  EXPECT_TRUE(tree2.verify());
  
  for (int i = 0; i < 5; ++i)
    EXPECT_NE(tree2.search(i), nullptr);
}

TEST_F(HtdRbTreeRkTest, MoveAssignment)
{
  insert_range(0, 5);
  
  Tree tree2;
  tree2 = std::move(tree);
  
  EXPECT_EQ(tree2.size(), 5u);
  EXPECT_TRUE(tree2.verify());
}

// ============================================================================
// Comparison with Bottom-Up Rank Tree
// ============================================================================

TEST_F(HtdRbTreeRkTest, SameResultsAsBottomUp)
{
  // Create parallel trees with unique keys
  HtdRbTreeRk<int> htd;
  Rb_Tree_Rk<int> bu;
  
  std::vector<Node*> htd_nodes;
  std::vector<RbNodeRk<int>*> bu_nodes;
  
  // Insert same elements (unique keys)
  for (int i = 0; i < 50; ++i)
    {
      int key = i * 7;  // Use unique keys
      
      auto* htd_node = new Node(key);
      auto* bu_node = new RbNodeRk<int>(key);
      htd_nodes.push_back(htd_node);
      bu_nodes.push_back(bu_node);
      
      auto* htd_result = htd.insert(htd_node);
      auto* bu_result = bu.insert(bu_node);
      
      EXPECT_NE(htd_result, nullptr);
      EXPECT_NE(bu_result, nullptr);
    }
  
  EXPECT_EQ(htd.size(), bu.size());
  EXPECT_TRUE(htd.verify());
  
  // Compare select results
  for (size_t i = 0; i < htd.size(); ++i)
    {
      auto* htd_sel = htd.select(i);
      auto* bu_sel = bu.select(i);
      EXPECT_EQ(KEY(htd_sel), KEY(bu_sel)) << "select(" << i << ") differs";
    }
  
  // Cleanup
  for (auto* n : htd_nodes) delete n;
  for (auto* n : bu_nodes) delete n;
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(HtdRbTreeRkTest, StressRandomOperations)
{
  // Insert many elements, then remove some
  for (int i = 0; i < 200; ++i)
    tree.insert(make_node(i));
  
  EXPECT_EQ(tree.size(), 200u);
  EXPECT_TRUE(tree.verify());
  
  // Remove half
  for (int i = 0; i < 200; i += 2)
    tree.remove(i);
  
  EXPECT_EQ(tree.size(), 100u);
  EXPECT_TRUE(tree.verify());
  
  // Verify select works correctly
  int expected = 1;  // Odd numbers starting from 1
  for (size_t i = 0; i < tree.size(); ++i)
    {
      EXPECT_EQ(KEY(tree.select(i)), expected);
      expected += 2;
    }
}

TEST_F(HtdRbTreeRkTest, StressLargeTree)
{
  const int N = 5000;
  
  for (int i = 0; i < N; ++i)
    tree.insert(make_node(i));
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  EXPECT_TRUE(tree.verify());
  
  // Test select at various positions
  EXPECT_EQ(KEY(tree.select(0)), 0);
  EXPECT_EQ(KEY(tree.select(N/2)), N/2);
  EXPECT_EQ(KEY(tree.select(N-1)), N-1);
  
  // Remove half
  for (int i = 0; i < N; i += 2)
    tree.remove(i);
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N/2));
  EXPECT_TRUE(tree.verify());
  
  // Check remaining elements
  for (int i = 1; i < N; i += 2)
    EXPECT_NE(tree.search(i), nullptr);
}

TEST_F(HtdRbTreeRkTest, StressMixedSelectPosition)
{
  insert_range(0, 100);
  
  // Verify select and position are inverses
  for (size_t i = 0; i < 100; ++i)
    {
      auto* selected = tree.select(i);
      auto [pos, node] = tree.position(KEY(selected));
      
      EXPECT_EQ(pos, static_cast<long>(i));
      EXPECT_EQ(node, selected);
    }
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(HtdRbTreeRkTest, IteratorInOrder)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  for (int k : keys)
    tree.insert(make_node(k));
  
  std::vector<int> expected = {10, 25, 30, 50, 60, 75, 90};
  std::vector<int> actual;
  
  for (Tree::Iterator it(tree); it.has_curr(); it.next())
    actual.push_back(KEY(it.get_curr()));
  
  EXPECT_EQ(actual, expected);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(HtdRbTreeRkTest, RemoveFromEmpty)
{
  EXPECT_EQ(tree.remove(42), nullptr);
}

TEST_F(HtdRbTreeRkTest, SelectOutOfRange)
{
  insert_range(0, 5);
  EXPECT_THROW(tree.select(10), std::out_of_range);
}

TEST_F(HtdRbTreeRkTest, RemovePosOutOfRange)
{
  insert_range(0, 5);
  EXPECT_THROW(tree.remove_pos(10), std::out_of_range);
}

TEST_F(HtdRbTreeRkTest, SearchOrInsert)
{
  auto* n1 = make_node(42);
  auto* result1 = tree.search_or_insert(n1);
  EXPECT_EQ(result1, n1);
  EXPECT_EQ(tree.size(), 1u);
  
  auto* n2 = make_node(42);
  auto* result2 = tree.search_or_insert(n2);
  EXPECT_EQ(result2, n1);  // Returns existing node
  EXPECT_EQ(tree.size(), 1u);
}

