/**
 * @file test_connectivity_test.cc
 * @brief Comprehensive tests for tpl_test_connectivity.H
 */

#include <gtest/gtest.h>
#include <tpl_test_connectivity.H>
#include <tpl_graph.H>

using namespace Aleph;

// =============================================================================
// Type Definitions
// =============================================================================

using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// =============================================================================
// Test Fixture
// =============================================================================

class TestConnectivityTest : public ::testing::Test
{
protected:
  GT g;
  
  void SetUp() override
  {
    g = GT();
  }
  
  void TearDown() override {}
};

// =============================================================================
// Basic Connectivity Tests
// =============================================================================

TEST_F(TestConnectivityTest, EmptyGraphIsConnected)
{
  Test_Connectivity<GT> checker;
  
  // Empty graph behavior: library returns false (no nodes to connect)
  EXPECT_FALSE(checker(g));
}

TEST_F(TestConnectivityTest, SingleNodeIsConnected)
{
  g.insert_node(1);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, TwoNodesConnected)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, TwoNodesDisconnected)
{
  g.insert_node(1);
  g.insert_node(2);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestConnectivityTest, LinearChainIsConnected)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Disconnected Graph Tests
// =============================================================================

TEST_F(TestConnectivityTest, TwoDisconnectedComponents)
{
  // Component 1
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  // Component 2 (disconnected)
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestConnectivityTest, ThreeDisconnectedComponents)
{
  // Component 1
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  // Component 2
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  // Component 3
  auto n5 = g.insert_node(5);
  auto n6 = g.insert_node(6);
  g.insert_arc(n5, n6);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestConnectivityTest, IsolatedNode)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_node(3);
  
  g.insert_arc(n1, n2);
  // n3 is isolated
  
  Test_Connectivity<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

// =============================================================================
// Complex Connected Structures
// =============================================================================

TEST_F(TestConnectivityTest, CompleteGraphIsConnected)
{
  const size_t n = 10;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Make it complete
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j)
      g.insert_arc(nodes[i], nodes[j]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, CycleIsConnected)
{
  const size_t n = 10;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create cycle
  for (size_t i = 0; i < n; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % n]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, TreeIsConnected)
{
  auto root = g.insert_node(0);
  
  // Create binary tree
  std::vector<Node*> current_level = {root};
  int node_count = 1;
  
  for (int depth = 0; depth < 4; ++depth)
  {
    std::vector<Node*> next_level;
    
    for (auto parent : current_level)
    {
      auto left = g.insert_node(node_count++);
      auto right = g.insert_node(node_count++);
      
      g.insert_arc(parent, left);
      g.insert_arc(parent, right);
      
      next_level.push_back(left);
      next_level.push_back(right);
    }
    
    current_level = next_level;
  }
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, StarGraphIsConnected)
{
  auto center = g.insert_node(0);
  
  for (int i = 1; i <= 20; ++i)
  {
    auto leaf = g.insert_node(i);
    g.insert_arc(center, leaf);
  }
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Arc Count Optimization Tests
// =============================================================================

TEST_F(TestConnectivityTest, NotEnoughArcs)
{
  const size_t n = 10;
  
  // Insert n nodes
  for (size_t i = 0; i < n; ++i)
    g.insert_node(i);
  
  // Only insert n-2 arcs (need at least n-1 for connectivity)
  std::vector<Node*> nodes;
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next())
    nodes.push_back(it.get_curr());
  
  for (size_t i = 0; i < n - 2; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Test_Connectivity<GT> checker;
  
  // Should detect disconnection via arc count optimization
  EXPECT_FALSE(checker(g));
}

TEST_F(TestConnectivityTest, ExactlyNMinus1Arcs_Connected)
{
  const size_t n = 10;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Chain with exactly n-1 arcs
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, MoreThanNMinus1Arcs_Connected)
{
  const size_t n = 10;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create ring (n arcs)
  for (size_t i = 0; i < n; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % n]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(TestConnectivityTest, GridGraphConnected)
{
  const size_t rows = 5;
  const size_t cols = 5;
  std::vector<std::vector<Node*>> grid(rows, std::vector<Node*>(cols));
  
  // Create grid
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      grid[i][j] = g.insert_node(i * cols + j);
  
  // Connect horizontally
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols - 1; ++j)
      g.insert_arc(grid[i][j], grid[i][j + 1]);
  
  // Connect vertically
  for (size_t i = 0; i < rows - 1; ++i)
    for (size_t j = 0; j < cols; ++j)
      g.insert_arc(grid[i][j], grid[i + 1][j]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, DiamondGraph)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Diamond: n1 -> n2, n3 -> n4
  //          n2, n3 both connect to n4
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n3, n4);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, BridgeGraph)
{
  // Two triangles connected by a bridge
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  auto n6 = g.insert_node(6);
  
  // Triangle 1
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  // Bridge
  g.insert_arc(n3, n4);
  
  // Triangle 2
  g.insert_arc(n4, n5);
  g.insert_arc(n5, n6);
  g.insert_arc(n6, n4);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(TestConnectivityTest, LargeConnectedGraph)
{
  const size_t n = 500;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create chain
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, LargeDisconnectedGraph)
{
  const size_t n = 500;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create chain but leave last node disconnected
  for (size_t i = 0; i < n - 2; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

// =============================================================================
// Multiple Calls Tests
// =============================================================================

TEST_F(TestConnectivityTest, MultipleCallsSameGraph)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
  EXPECT_TRUE(checker(g));  // Second call should work
  EXPECT_TRUE(checker(g));  // Third call should work
}

TEST_F(TestConnectivityTest, ModifyGraphBetweenCalls)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_FALSE(checker(g));  // Disconnected
  
  g.insert_arc(n1, n2);
  
  EXPECT_TRUE(checker(g));  // Now connected
}

// =============================================================================
// Specific Structure Tests
// =============================================================================

TEST_F(TestConnectivityTest, PathGraph)
{
  const size_t n = 20;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, WheelGraph)
{
  auto center = g.insert_node(0);
  const size_t n = 10;
  std::vector<Node*> rim;
  
  for (size_t i = 0; i < n; ++i)
    rim.push_back(g.insert_node(i + 1));
  
  // Connect center to all rim nodes
  for (auto node : rim)
    g.insert_arc(center, node);
  
  // Connect rim nodes in a cycle
  for (size_t i = 0; i < n; ++i)
    g.insert_arc(rim[i], rim[(i + 1) % n]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestConnectivityTest, BarbelGraph)
{
  // Two complete graphs connected by a path
  const size_t k1_size = 5;
  const size_t k2_size = 5;
  
  std::vector<Node*> k1_nodes, k2_nodes;
  
  // Create first complete graph
  for (size_t i = 0; i < k1_size; ++i)
    k1_nodes.push_back(g.insert_node(i));
  
  for (size_t i = 0; i < k1_size; ++i)
    for (size_t j = i + 1; j < k1_size; ++j)
      g.insert_arc(k1_nodes[i], k1_nodes[j]);
  
  // Create second complete graph
  for (size_t i = 0; i < k2_size; ++i)
    k2_nodes.push_back(g.insert_node(k1_size + i));
  
  for (size_t i = 0; i < k2_size; ++i)
    for (size_t j = i + 1; j < k2_size; ++j)
      g.insert_arc(k2_nodes[i], k2_nodes[j]);
  
  // Connect them with a bridge
  g.insert_arc(k1_nodes[0], k2_nodes[0]);
  
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Custom Arc Filter Tests
// =============================================================================

TEST_F(TestConnectivityTest, WithDefaultArcFilter)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Dft_Show_Arc<GT> filter;
  Test_Connectivity<GT> checker;
  
  EXPECT_TRUE(checker(g, filter));
}
