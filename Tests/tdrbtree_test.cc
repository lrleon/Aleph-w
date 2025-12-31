/**
 * @file tdrbtree_test.cc
 * @brief Exhaustive tests for Top-Down Red-Black Tree implementation.
 * 
 * Tests cover:
 * - Basic operations (insert, search, remove)
 * - Red-black property invariants
 * - Custom comparators
 * - Edge cases (empty tree, single node, duplicates)
 * - Stress tests with random operations
 * - Comparison with bottom-up implementation
 */

#include <gtest/gtest.h>
#include <tpl_tdRbTree.H>
#include <tpl_rb_tree.H>
#include <ahSort.H>
#include <random>
#include <set>
#include <vector>
#include <string>
#include <functional>

using namespace Aleph;

// ============================================================================
// Test Fixtures
// ============================================================================

class TdRbTreeTest : public ::testing::Test
{
protected:
  using Tree = TdRbTree<int>;
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

class TdRbTreeCustomCompareTest : public ::testing::Test
{
protected:
  // Reverse order comparator
  struct ReverseCompare
  {
    bool operator()(int a, int b) const { return a > b; }
  };
  
  using Tree = TdRbTree<int, ReverseCompare>;
  using Node = Tree::Node;
  
  Tree tree;
  std::vector<Node*> allocated_nodes;
  
  void TearDown() override
  {
    for (auto *node : allocated_nodes)
      delete node;
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

TEST_F(TdRbTreeTest, EmptyTree)
{
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.search(42), nullptr);
  EXPECT_EQ(tree.remove(42), nullptr);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, InsertSingleNode)
{
  Node *node = make_node(42);
  EXPECT_EQ(tree.insert(node), node);
  
  EXPECT_FALSE(tree.is_empty());
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_EQ(tree.search(42), node);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, InsertMultipleNodes)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  auto nodes = make_nodes(keys);
  
  for (auto *node : nodes)
    EXPECT_NE(tree.insert(node), nullptr);
  
  EXPECT_EQ(tree.size(), keys.size());
  
  for (int key : keys)
    EXPECT_NE(tree.search(key), nullptr) << "Key " << key << " not found";
  
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, InsertDuplicateKey)
{
  Node *node1 = make_node(42);
  Node *node2 = make_node(42);
  
  EXPECT_EQ(tree.insert(node1), node1);
  EXPECT_EQ(tree.insert(node2), nullptr);  // Duplicate rejected
  
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, SearchNonExistent)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  EXPECT_EQ(tree.search(15), nullptr);
  EXPECT_EQ(tree.search(0), nullptr);
  EXPECT_EQ(tree.search(100), nullptr);
}

TEST_F(TdRbTreeTest, RemoveSingleNode)
{
  Node *node = make_node(42);
  tree.insert(node);
  
  Node *removed = tree.remove(42);
  EXPECT_EQ(removed, node);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, RemoveLeafNode)
{
  auto nodes = make_nodes({50, 25, 75});
  for (auto *node : nodes)
    tree.insert(node);
  
  // Remove leaf (75)
  Node *removed = tree.remove(75);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 75);
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_EQ(tree.search(75), nullptr);
  EXPECT_NE(tree.search(50), nullptr);
  EXPECT_NE(tree.search(25), nullptr);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, RemoveInternalNode)
{
  auto nodes = make_nodes({50, 25, 75, 10, 30, 60, 90});
  for (auto *node : nodes)
    tree.insert(node);
  
  // Remove internal node (25)
  Node *removed = tree.remove(25);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree.size(), 6u);
  EXPECT_EQ(tree.search(25), nullptr);
  
  // All other keys should still be present
  for (int key : {50, 75, 10, 30, 60, 90})
    EXPECT_NE(tree.search(key), nullptr) << "Key " << key << " missing";
  
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, RemoveRoot)
{
  auto nodes = make_nodes({50, 25, 75});
  for (auto *node : nodes)
    tree.insert(node);
  
  Node *removed = tree.remove(50);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_EQ(tree.search(50), nullptr);
  EXPECT_NE(tree.search(25), nullptr);
  EXPECT_NE(tree.search(75), nullptr);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, RemoveNonExistent)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  EXPECT_EQ(tree.remove(15), nullptr);
  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, RemoveAllNodes)
{
  std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
  auto nodes = make_nodes(keys);
  
  for (auto *node : nodes)
    tree.insert(node);
  
  // Remove in different order
  std::vector<int> removal_order = {30, 10, 90, 50, 25, 75, 60};
  for (int key : removal_order)
  {
    EXPECT_NE(tree.remove(key), nullptr) << "Failed to remove " << key;
    EXPECT_TRUE(tree.verifyRedBlack()) << "RB violation after removing " << key;
  }
  
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
}

// ============================================================================
// Reset and Swap Tests
// ============================================================================

TEST_F(TdRbTreeTest, Reset)
{
  auto nodes = make_nodes({10, 20, 30});
  for (auto *node : nodes)
    tree.insert(node);
  
  tree.reset();
  
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, Swap)
{
  Tree tree1, tree2;
  
  auto node1 = make_node(10);
  auto node2 = make_node(20);
  auto node3 = make_node(30);
  
  tree1.insert(node1);
  tree2.insert(node2);
  tree2.insert(node3);
  
  EXPECT_EQ(tree1.size(), 1u);
  EXPECT_EQ(tree2.size(), 2u);
  
  tree1.swap(tree2);
  
  EXPECT_EQ(tree1.size(), 2u);
  EXPECT_EQ(tree2.size(), 1u);
  EXPECT_NE(tree1.search(20), nullptr);
  EXPECT_NE(tree1.search(30), nullptr);
  EXPECT_NE(tree2.search(10), nullptr);
}

// ============================================================================
// Custom Comparator Tests
// ============================================================================

TEST_F(TdRbTreeCustomCompareTest, ReverseOrder)
{
  auto nodes = make_nodes({10, 20, 30, 40, 50});
  
  for (auto *node : nodes)
    tree.insert(node);
  
  EXPECT_EQ(tree.size(), 5u);
  
  // All nodes should be findable
  for (int key : {10, 20, 30, 40, 50})
    EXPECT_NE(tree.search(key), nullptr);
  
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST(TdRbTreeFunctorCompare, AbsoluteValueComparator)
{
  // Functor comparator: compare by absolute value
  struct AbsCompare
  {
    bool operator()(int a, int b) const { return std::abs(a) < std::abs(b); }
  };
  
  TdRbTree<int, AbsCompare> tree;
  
  std::vector<RbNode<int>*> nodes;
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  tree.insert(make(-5));
  tree.insert(make(3));
  tree.insert(make(-10));
  tree.insert(make(7));
  
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_NE(tree.search(-5), nullptr);
  EXPECT_NE(tree.search(3), nullptr);
  
  // With abs compare, -5 and 5 should be considered equal
  EXPECT_EQ(tree.insert(make(5)), nullptr);  // Duplicate by abs value
  
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// String Key Tests
// ============================================================================

TEST(TdRbTreeStringTest, StringKeys)
{
  TdRbTree<std::string> tree;
  std::vector<RbNode<std::string>*> nodes;
  
  auto make = [&nodes](const std::string& s) {
    auto *n = new RbNode<std::string>(s);
    nodes.push_back(n);
    return n;
  };
  
  tree.insert(make("banana"));
  tree.insert(make("apple"));
  tree.insert(make("cherry"));
  tree.insert(make("date"));
  
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_NE(tree.search("apple"), nullptr);
  EXPECT_NE(tree.search("banana"), nullptr);
  EXPECT_EQ(tree.search("fig"), nullptr);
  
  auto *removed = tree.remove("banana");
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree.search("banana"), nullptr);
  
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Insertion Patterns Tests
// ============================================================================

TEST_F(TdRbTreeTest, InsertAscending)
{
  for (int i = 1; i <= 100; ++i)
    tree.insert(make_node(i));
  
  EXPECT_EQ(tree.size(), 100u);
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (int i = 1; i <= 100; ++i)
    EXPECT_NE(tree.search(i), nullptr);
}

TEST_F(TdRbTreeTest, InsertDescending)
{
  for (int i = 100; i >= 1; --i)
    tree.insert(make_node(i));
  
  EXPECT_EQ(tree.size(), 100u);
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (int i = 1; i <= 100; ++i)
    EXPECT_NE(tree.search(i), nullptr);
}

TEST_F(TdRbTreeTest, InsertZigZag)
{
  // Insert pattern: 50, 25, 75, 12, 37, 62, 87, ...
  int low = 0, high = 100;
  int mid;
  std::vector<int> inserted;
  
  while (low <= high)
  {
    mid = (low + high) / 2;
    tree.insert(make_node(mid));
    inserted.push_back(mid);
    low = mid + 1;
  }
  
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (int key : inserted)
    EXPECT_NE(tree.search(key), nullptr);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(TdRbTreeStressTest, RandomInsertRemove)
{
  TdRbTree<int> tree;
  std::set<int> reference;  // For comparison
  std::vector<RbNode<int>*> all_nodes;
  
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> key_dist(1, 10000);
  std::uniform_int_distribution<int> op_dist(0, 2);  // 0: insert, 1: remove, 2: search
  
  auto make = [&all_nodes](int k) {
    auto *n = new RbNode<int>(k);
    all_nodes.push_back(n);
    return n;
  };
  
  const int NUM_OPS = 5000;
  
  for (int i = 0; i < NUM_OPS; ++i)
  {
    int op = op_dist(rng);
    int key = key_dist(rng);
    
    if (op == 0)  // Insert
    {
      auto *node = make(key);
      bool tree_inserted = (tree.insert(node) != nullptr);
      bool ref_inserted = reference.insert(key).second;
      EXPECT_EQ(tree_inserted, ref_inserted) 
          << "Insert mismatch at op " << i << " key " << key;
    }
    else if (op == 1)  // Remove
    {
      bool tree_removed = (tree.remove(key) != nullptr);
      bool ref_removed = (reference.erase(key) > 0);
      EXPECT_EQ(tree_removed, ref_removed)
          << "Remove mismatch at op " << i << " key " << key;
    }
    else  // Search
    {
      bool tree_found = (tree.search(key) != nullptr);
      bool ref_found = (reference.count(key) > 0);
      EXPECT_EQ(tree_found, ref_found)
          << "Search mismatch at op " << i << " key " << key;
    }
    
    // Verify RB properties periodically
    if (i % 500 == 0)
      EXPECT_TRUE(tree.verifyRedBlack()) << "RB violation at op " << i;
  }
  
  EXPECT_EQ(tree.size(), reference.size());
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (auto *n : all_nodes)
    delete n;
}

TEST(TdRbTreeStressTest, LargeTree)
{
  TdRbTree<int> tree;
  std::vector<RbNode<int>*> nodes;
  
  const int N = 10000;
  
  // Insert N unique keys
  for (int i = 0; i < N; ++i)
  {
    auto *node = new RbNode<int>(i);
    nodes.push_back(node);
    EXPECT_NE(tree.insert(node), nullptr);
  }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N));
  EXPECT_TRUE(tree.verifyRedBlack());
  
  // Verify all keys present
  for (int i = 0; i < N; ++i)
    EXPECT_NE(tree.search(i), nullptr);
  
  // Remove half
  for (int i = 0; i < N; i += 2)
  {
    EXPECT_NE(tree.remove(i), nullptr);
  }
  
  EXPECT_EQ(tree.size(), static_cast<size_t>(N / 2));
  EXPECT_TRUE(tree.verifyRedBlack());
  
  // Verify remaining keys
  for (int i = 0; i < N; ++i)
  {
    if (i % 2 == 0)
      EXPECT_EQ(tree.search(i), nullptr);
    else
      EXPECT_NE(tree.search(i), nullptr);
  }
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Comparison with Bottom-Up Implementation
// ============================================================================

TEST(TdRbTreeComparisonTest, SameResultsAsBottomUp)
{
  TdRbTree<int> td_tree;
  Rb_Tree<int> bu_tree;  // Bottom-up implementation
  
  std::vector<RbNode<int>*> td_nodes;
  std::vector<Rb_Tree<int>::Node*> bu_nodes;
  
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(1, 1000);
  
  const int N = 500;
  std::vector<int> keys;
  
  // Generate unique keys
  std::set<int> key_set;
  while (key_set.size() < static_cast<size_t>(N))
    key_set.insert(dist(rng));
  
  for (int k : key_set)
    keys.push_back(k);
  
  // Insert into both trees
  for (int key : keys)
  {
    auto *td_node = new RbNode<int>(key);
    td_nodes.push_back(td_node);
    auto *bu_node = new Rb_Tree<int>::Node(key);
    bu_nodes.push_back(bu_node);
    
    bool td_ok = (td_tree.insert(td_node) != nullptr);
    bool bu_ok = (bu_tree.insert(bu_node) != nullptr);
    
    EXPECT_EQ(td_ok, bu_ok) << "Insert mismatch for key " << key;
  }
  
  EXPECT_EQ(td_tree.size(), bu_tree.size());
  
  // Shuffle keys for removal order
  std::shuffle(keys.begin(), keys.end(), rng);
  
  // Remove from both and compare
  for (int key : keys)
  {
    bool td_found = (td_tree.search(key) != nullptr);
    bool bu_found = (bu_tree.search(key) != nullptr);
    EXPECT_EQ(td_found, bu_found) << "Search mismatch for key " << key;
    
    bool td_removed = (td_tree.remove(key) != nullptr);
    bool bu_removed = (bu_tree.remove(key) != nullptr);
    EXPECT_EQ(td_removed, bu_removed) << "Remove mismatch for key " << key;
    
    EXPECT_TRUE(td_tree.verifyRedBlack());
    EXPECT_TRUE(bu_tree.verify());
  }
  
  EXPECT_TRUE(td_tree.is_empty());
  EXPECT_TRUE(bu_tree.is_empty());
  
  for (auto *n : td_nodes) delete n;
  for (auto *n : bu_nodes) delete n;
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(TdRbTreeTest, InsertRemoveInsert)
{
  Node *node = make_node(42);
  
  EXPECT_NE(tree.insert(node), nullptr);
  EXPECT_NE(tree.remove(42), nullptr);
  
  // Reset node for reinsertion
  LLINK(node) = RLINK(node) = Node::NullPtr;
  COLOR(node) = RED;
  
  EXPECT_NE(tree.insert(node), nullptr);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_NE(tree.search(42), nullptr);
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, NegativeKeys)
{
  auto nodes = make_nodes({-50, -25, 0, 25, 50});
  
  for (auto *node : nodes)
    tree.insert(node);
  
  EXPECT_EQ(tree.size(), 5u);
  
  for (int key : {-50, -25, 0, 25, 50})
    EXPECT_NE(tree.search(key), nullptr);
  
  EXPECT_TRUE(tree.verifyRedBlack());
}

TEST_F(TdRbTreeTest, MinMaxIntKeys)
{
  auto nodes = make_nodes({
    std::numeric_limits<int>::min(),
    std::numeric_limits<int>::max(),
    0
  });
  
  for (auto *node : nodes)
    tree.insert(node);
  
  EXPECT_NE(tree.search(std::numeric_limits<int>::min()), nullptr);
  EXPECT_NE(tree.search(std::numeric_limits<int>::max()), nullptr);
  EXPECT_NE(tree.search(0), nullptr);
  EXPECT_TRUE(tree.verifyRedBlack());
}

// ============================================================================
// Virtual Destructor Variant Tests
// ============================================================================

TEST(TdRbTreeVtlTest, BasicOperations)
{
  TdRbTreeVtl<int> tree;
  std::vector<RbNodeVtl<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNodeVtl<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  tree.insert(make(30));
  tree.insert(make(10));
  tree.insert(make(50));
  tree.insert(make(5));
  tree.insert(make(15));
  
  EXPECT_EQ(tree.size(), 5u);
  EXPECT_NE(tree.search(30), nullptr);
  EXPECT_NE(tree.search(5), nullptr);
  EXPECT_EQ(tree.search(100), nullptr);
  
  EXPECT_TRUE(tree.verifyRedBlack());
  
  EXPECT_NE(tree.remove(10), nullptr);
  EXPECT_EQ(tree.size(), 4u);
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(TdRbTreeMoveTest, MoveConstructor)
{
  TdRbTree<int> tree1;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  tree1.insert(make(50));
  tree1.insert(make(25));
  tree1.insert(make(75));
  
  EXPECT_EQ(tree1.size(), 3u);
  
  TdRbTree<int> tree2(std::move(tree1));
  
  EXPECT_EQ(tree2.size(), 3u);
  EXPECT_TRUE(tree1.is_empty());  // NOLINT: testing moved-from state
  EXPECT_EQ(tree1.size(), 0u);    // NOLINT
  
  EXPECT_NE(tree2.search(50), nullptr);
  EXPECT_NE(tree2.search(25), nullptr);
  EXPECT_NE(tree2.search(75), nullptr);
  EXPECT_TRUE(tree2.verifyRedBlack());
  
  for (auto *n : nodes)
    delete n;
}

TEST(TdRbTreeMoveTest, MoveAssignment)
{
  TdRbTree<int> tree1, tree2;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  tree1.insert(make(10));
  tree1.insert(make(20));
  tree1.insert(make(30));
  
  tree2.insert(make(100));
  
  tree2 = std::move(tree1);
  
  EXPECT_EQ(tree2.size(), 3u);
  EXPECT_TRUE(tree1.is_empty());  // NOLINT
  
  EXPECT_NE(tree2.search(10), nullptr);
  EXPECT_NE(tree2.search(20), nullptr);
  EXPECT_TRUE(tree2.verifyRedBlack());
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Insert Dup Tests
// ============================================================================

TEST(TdRbTreeInsertDupTest, AllowsDuplicates)
{
  TdRbTree<int> tree;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  EXPECT_NE(tree.insert_dup(make(42)), nullptr);
  EXPECT_NE(tree.insert_dup(make(42)), nullptr);
  EXPECT_NE(tree.insert_dup(make(42)), nullptr);
  
  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verifyRedBlack());
  
  // Search finds first occurrence
  EXPECT_NE(tree.search(42), nullptr);
  
  for (auto *n : nodes)
    delete n;
}

TEST(TdRbTreeInsertDupTest, MixedInsert)
{
  TdRbTree<int> tree;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  tree.insert_dup(make(50));
  tree.insert_dup(make(25));
  tree.insert_dup(make(75));
  tree.insert_dup(make(25));  // duplicate
  tree.insert_dup(make(50));  // duplicate
  tree.insert_dup(make(25));  // duplicate
  
  EXPECT_EQ(tree.size(), 6u);
  EXPECT_TRUE(tree.verifyRedBlack());
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Search or Insert Tests
// ============================================================================

TEST(TdRbTreeSearchOrInsertTest, InsertWhenNotFound)
{
  TdRbTree<int> tree;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  auto *node1 = make(42);
  auto *result = tree.search_or_insert(node1);
  
  EXPECT_EQ(result, node1);  // Inserted
  EXPECT_EQ(tree.size(), 1u);
  
  for (auto *n : nodes)
    delete n;
}

TEST(TdRbTreeSearchOrInsertTest, ReturnsExistingWhenFound)
{
  TdRbTree<int> tree;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  auto *node1 = make(42);
  auto *node2 = make(42);  // Same key
  
  tree.insert(node1);
  auto *result = tree.search_or_insert(node2);
  
  EXPECT_EQ(result, node1);  // Returns existing, not inserted
  EXPECT_EQ(tree.size(), 1u);
  
  for (auto *n : nodes)
    delete n;
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(TdRbTreeIteratorTest, InOrderTraversal)
{
  TdRbTree<int> tree;
  std::vector<RbNode<int>*> nodes;
  
  auto make = [&nodes](int k) {
    auto *n = new RbNode<int>(k);
    nodes.push_back(n);
    return n;
  };
  
  // Insert in random order
  tree.insert(make(50));
  tree.insert(make(25));
  tree.insert(make(75));
  tree.insert(make(10));
  tree.insert(make(30));
  tree.insert(make(60));
  tree.insert(make(90));
  
  // Iterator should traverse in sorted order
  std::vector<int> traversal;
  for (TdRbTree<int>::Iterator it(tree); it.has_curr(); it.next())
    traversal.push_back(KEY(it.get_curr()));
  
  std::vector<int> expected = {10, 25, 30, 50, 60, 75, 90};
  EXPECT_EQ(traversal, expected);
  
  for (auto *n : nodes)
    delete n;
}

TEST(TdRbTreeIteratorTest, EmptyTree)
{
  TdRbTree<int> tree;
  
  TdRbTree<int>::Iterator it(tree);
  EXPECT_FALSE(it.has_curr());
}

TEST(TdRbTreeIteratorTest, SingleElement)
{
  TdRbTree<int> tree;
  auto *node = new RbNode<int>(42);
  tree.insert(node);
  
  TdRbTree<int>::Iterator it(tree);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(KEY(it.get_curr()), 42);
  it.next();
  EXPECT_FALSE(it.has_curr());
  
  delete node;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

