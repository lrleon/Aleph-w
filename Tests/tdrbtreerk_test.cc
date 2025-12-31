/**
 * @file tdrbtreerk_test.cc
 * @brief Exhaustive tests for Top-Down Red-Black Tree with Rank support.
 */

#include <gtest/gtest.h>
#include <tpl_tdRbTreeRk.H>
#include <tpl_rbRk.H>
#include <random>
#include <set>
#include <vector>
#include <algorithm>

using namespace Aleph;

// ============================================================================
// Test Fixtures
// ============================================================================

class TdRbTreeRkTest : public ::testing::Test
{
protected:
  using Tree = TdRbTreeRk<int>;
  using Node = Tree::Node;
  
  Tree tree;
  std::vector<Node*> allocated_nodes;
  
  void TearDown() override
  {
    for (auto *node : allocated_nodes)
      delete node;
    allocated_nodes.clear();
  }
  
  Node* make_node(int key)
  {
    auto *node = new Node(key);
    allocated_nodes.push_back(node);
    return node;
  }
  
  std::vector<Node*> make_nodes(const std::vector<int>& keys)
  {
    std::vector<Node*> nodes;
    for (int k : keys)
      nodes.push_back(make_node(k));
    return nodes;
  }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, EmptyTree)
{
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST_F(TdRbTreeRkTest, InsertSingleNode)
{
  Node *node = make_node(42);
  EXPECT_EQ(tree.insert(node), node);
  
  EXPECT_FALSE(tree.is_empty());
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_EQ(tree.search(42), node);
  EXPECT_TRUE(tree.verify());
}

TEST_F(TdRbTreeRkTest, InsertMultipleNodes)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  auto nodes = make_nodes(keys);
  
  for (auto *node : nodes)
    EXPECT_NE(tree.insert(node), nullptr);
  
  EXPECT_EQ(tree.size(), keys.size());
  EXPECT_TRUE(tree.verify());
  
  for (int key : keys)
    EXPECT_NE(tree.search(key), nullptr);
}

TEST_F(TdRbTreeRkTest, InsertDuplicateRejected)
{
  Node *node1 = make_node(42);
  Node *node2 = make_node(42);
  
  EXPECT_EQ(tree.insert(node1), node1);
  EXPECT_EQ(tree.insert(node2), nullptr);
  
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(TdRbTreeRkTest, RemoveNode)
{
  auto nodes = make_nodes({50, 25, 75, 10, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  EXPECT_EQ(tree.size(), 5u);
  
  Node *removed = tree.remove(25);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(tree.search(25), nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST_F(TdRbTreeRkTest, RemoveAllNodes)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  auto nodes = make_nodes(keys);
  
  for (auto *node : nodes)
    tree.insert(node);
  
  // Remove in different order
  std::vector<int> removal = {30, 10, 90, 50, 25, 75, 60};
  for (int key : removal)
  {
    EXPECT_NE(tree.remove(key), nullptr) << "Failed to remove " << key;
    EXPECT_TRUE(tree.verify()) << "Verify failed after removing " << key;
  }
  
  EXPECT_TRUE(tree.is_empty());
}

// ============================================================================
// Rank Operation Tests (Select)
// ============================================================================

TEST_F(TdRbTreeRkTest, SelectBasic)
{
  // Insert in random order
  auto nodes = make_nodes({50, 25, 75, 10, 30, 60, 90});
  for (auto *node : nodes)
    tree.insert(node);
  
  // Select should return nodes in sorted order
  std::vector<int> expected = {10, 25, 30, 50, 60, 75, 90};
  
  for (size_t i = 0; i < expected.size(); ++i)
  {
    Node *selected = tree.select(i);
    ASSERT_NE(selected, nullptr) << "select(" << i << ") returned null";
    EXPECT_EQ(KEY(selected), expected[i]) << "select(" << i << ") wrong";
  }
}

TEST_F(TdRbTreeRkTest, SelectAfterRemoval)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  // Remove middle element
  tree.remove(30);
  
  EXPECT_EQ(tree.size(), 4u);
  
  // Check remaining order
  std::vector<int> expected = {10, 20, 40, 50};
  for (size_t i = 0; i < expected.size(); ++i)
  {
    Node *selected = tree.select(i);
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(KEY(selected), expected[i]);
  }
}

TEST_F(TdRbTreeRkTest, SelectLargeTree)
{
  // Insert 100 elements
  for (int i = 0; i < 100; ++i)
    tree.insert(make_node(i * 2));  // Even numbers 0-198
  
  EXPECT_EQ(tree.size(), 100u);
  EXPECT_TRUE(tree.verify());
  
  // Select should return i-th smallest
  for (size_t i = 0; i < 100; ++i)
  {
    Node *selected = tree.select(i);
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(KEY(selected), static_cast<int>(i * 2));
  }
}

// ============================================================================
// Position Operation Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, PositionBasic)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  // Each key should be at its expected position
  auto [pos10, node10] = tree.position(10);
  EXPECT_EQ(pos10, 0);
  EXPECT_NE(node10, nullptr);
  
  auto [pos30, node30] = tree.position(30);
  EXPECT_EQ(pos30, 2);
  
  auto [pos50, node50] = tree.position(50);
  EXPECT_EQ(pos50, 4);
}

TEST_F(TdRbTreeRkTest, PositionNotFound)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  auto [pos, node] = tree.position(25);  // Not in tree
  EXPECT_EQ(pos, -1);
  EXPECT_EQ(node, nullptr);
}

TEST_F(TdRbTreeRkTest, FindPositionExisting)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  auto [pos, node] = tree.find_position(30);
  EXPECT_EQ(pos, 2);
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(KEY(node), 30);
}

TEST_F(TdRbTreeRkTest, FindPositionNotExisting)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  // 25 would be at position 2 if it existed
  auto [pos, node] = tree.find_position(25);
  EXPECT_GE(pos, 0);  // Should give a valid position
}

// ============================================================================
// Insert Dup Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, InsertDupAllowsDuplicates)
{
  EXPECT_NE(tree.insert_dup(make_node(42)), nullptr);
  EXPECT_NE(tree.insert_dup(make_node(42)), nullptr);
  EXPECT_NE(tree.insert_dup(make_node(42)), nullptr);
  
  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verify());
}

// ============================================================================
// Split by Position Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, SplitPosMiddle)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  TdRbTreeRk<int> t1, t2;
  tree.split_pos(2, t1, t2);  // Split at position 2
  
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(t1.size(), 2u);  // [10, 20]
  EXPECT_EQ(t2.size(), 3u);  // [30, 40, 50]
  
  EXPECT_NE(t1.search(10), nullptr);
  EXPECT_NE(t1.search(20), nullptr);
  EXPECT_NE(t2.search(30), nullptr);
  EXPECT_NE(t2.search(40), nullptr);
  EXPECT_NE(t2.search(50), nullptr);
  
  EXPECT_TRUE(t1.verify());
  EXPECT_TRUE(t2.verify());
}

TEST_F(TdRbTreeRkTest, SplitPosBeginning)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  TdRbTreeRk<int> t1, t2;
  tree.split_pos(0, t1, t2);
  
  EXPECT_EQ(t1.size(), 0u);
  EXPECT_EQ(t2.size(), 3u);
}

TEST_F(TdRbTreeRkTest, SplitPosEnd)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  TdRbTreeRk<int> t1, t2;
  tree.split_pos(10, t1, t2);  // Beyond end
  
  EXPECT_EQ(t1.size(), 3u);
  EXPECT_EQ(t2.size(), 0u);
}

// ============================================================================
// Remove by Position Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, RemovePos)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  for (auto *node : nodes)
    tree.insert(node);
  
  // Remove middle element (position 2 = 30)
  Node *removed = tree.remove_pos(2);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 30);
  
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_EQ(tree.search(30), nullptr);
  EXPECT_TRUE(tree.verify());
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, MoveConstructor)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  TdRbTreeRk<int> tree2(std::move(tree));
  
  EXPECT_EQ(tree2.size(), 3u);
  EXPECT_TRUE(tree.is_empty());  // NOLINT
  EXPECT_TRUE(tree2.verify());
}

TEST_F(TdRbTreeRkTest, MoveAssignment)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  TdRbTreeRk<int> tree2;
  tree2.insert(make_node(100));
  
  tree2 = std::move(tree);
  
  EXPECT_EQ(tree2.size(), 3u);
  EXPECT_TRUE(tree.is_empty());  // NOLINT
  EXPECT_TRUE(tree2.verify());
}

// ============================================================================
// Comparison with Bottom-Up Rb_Tree_Rk
// ============================================================================

TEST(TdRbTreeRkComparison, SameResultsAsBottomUp)
{
  TdRbTreeRk<int> td_tree;
  Rb_Tree_Rk<int> bu_tree;
  
  std::vector<RbNodeRk<int>*> td_nodes, bu_nodes;
  
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(1, 1000);
  
  std::set<int> keys_set;
  while (keys_set.size() < 100)
    keys_set.insert(dist(rng));
  
  std::vector<int> keys(keys_set.begin(), keys_set.end());
  
  // Insert into both
  for (int key : keys)
  {
    auto *td_node = new RbNodeRk<int>(key);
    auto *bu_node = new RbNodeRk<int>(key);
    td_nodes.push_back(td_node);
    bu_nodes.push_back(bu_node);
    
    td_tree.insert(td_node);
    bu_tree.insert(bu_node);
  }
  
  // Compare sizes
  EXPECT_EQ(td_tree.size(), bu_tree.size());
  
  // Compare select results
  for (size_t i = 0; i < keys.size(); ++i)
  {
    auto *td_sel = td_tree.select(i);
    auto *bu_sel = bu_tree.select(i);
    ASSERT_NE(td_sel, nullptr);
    ASSERT_NE(bu_sel, nullptr);
    EXPECT_EQ(KEY(td_sel), KEY(bu_sel)) << "select(" << i << ") differs";
  }
  
  // Verify both
  EXPECT_TRUE(td_tree.verify());
  EXPECT_TRUE(bu_tree.verify());
  
  // Cleanup
  for (auto *n : td_nodes) delete n;
  for (auto *n : bu_nodes) delete n;
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(TdRbTreeRkStress, RandomOperations)
{
  TdRbTreeRk<int> tree;
  std::set<int> reference;
  std::vector<RbNodeRk<int>*> all_nodes;
  
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> key_dist(1, 5000);
  std::uniform_int_distribution<int> op_dist(0, 2);
  
  auto make = [&all_nodes](int k) {
    auto *n = new RbNodeRk<int>(k);
    all_nodes.push_back(n);
    return n;
  };
  
  const int NUM_OPS = 2000;
  
  for (int i = 0; i < NUM_OPS; ++i)
  {
    int op = op_dist(rng);
    int key = key_dist(rng);
    
    if (op == 0)  // Insert
    {
      auto *node = make(key);
      bool td_inserted = (tree.insert(node) != nullptr);
      bool ref_inserted = reference.insert(key).second;
      EXPECT_EQ(td_inserted, ref_inserted);
    }
    else if (op == 1)  // Remove
    {
      bool td_removed = (tree.remove(key) != nullptr);
      bool ref_removed = (reference.erase(key) > 0);
      EXPECT_EQ(td_removed, ref_removed);
    }
    else  // Search
    {
      bool td_found = (tree.search(key) != nullptr);
      bool ref_found = (reference.count(key) > 0);
      EXPECT_EQ(td_found, ref_found);
    }
    
    if (i % 200 == 0)
    {
      EXPECT_TRUE(tree.verify()) << "Verify failed at op " << i;
      EXPECT_EQ(tree.size(), reference.size());
    }
  }
  
  EXPECT_TRUE(tree.verify());
  
  for (auto *n : all_nodes)
    delete n;
}

TEST(TdRbTreeRkStress, LargeTreeSelectPerformance)
{
  TdRbTreeRk<int> tree;
  std::vector<RbNodeRk<int>*> nodes;
  
  const int N = 5000;
  
  for (int i = 0; i < N; ++i)
  {
    auto *node = new RbNodeRk<int>(i);
    nodes.push_back(node);
    tree.insert(node);
  }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  EXPECT_TRUE(tree.verify());
  
  // Test select at various positions
  for (int i = 0; i < N; i += 100)
  {
    auto *sel = tree.select(i);
    ASSERT_NE(sel, nullptr);
    EXPECT_EQ(KEY(sel), i);
  }
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(TdRbTreeRkTest, IteratorInOrder)
{
  auto nodes = make_nodes({50, 25, 75, 10, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  std::vector<int> traversal;
  for (TdRbTreeRk<int>::Iterator it(tree); it.has_curr(); it.next())
    traversal.push_back(KEY(it.get_curr()));
  
  std::vector<int> expected = {10, 25, 30, 50, 75};
  EXPECT_EQ(traversal, expected);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

