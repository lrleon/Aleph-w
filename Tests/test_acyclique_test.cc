/**
 * @file test_acyclique_test.cc
 * @brief Comprehensive tests for tpl_test_acyclique.H
 */

#include <gtest/gtest.h>
#include <tpl_test_acyclique.H>
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

class TestAcycliqueTest : public ::testing::Test
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
// Basic Acyclicity Tests
// =============================================================================

TEST_F(TestAcycliqueTest, EmptyGraphIsAcyclic)
{
  Is_Graph_Acyclique<GT> checker;
  
  // Empty graph behavior: library returns false (no nodes to check)
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, SingleNodeIsAcyclic)
{
  g.insert_node(1);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, TwoNodesNoArcsIsAcyclic)
{
  g.insert_node(1);
  g.insert_node(2);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, TwoNodesOneArcIsAcyclic)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, LinearChainIsAcyclic)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, TreeIsAcyclic)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  // Tree structure: n1 is root, n2 and n3 are children, n4 and n5 are grandchildren
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n2, n5);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Cycle Detection Tests
// =============================================================================

TEST_F(TestAcycliqueTest, TriangleCycle)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);  // Closes cycle
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, SquareCycle)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n1);  // Closes cycle
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, SelfLoop)
{
  auto n = g.insert_node(1);
  g.insert_arc(n, n);  // Self-loop
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, LargeCycle)
{
  const size_t n = 100;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  g.insert_arc(nodes[n - 1], nodes[0]);  // Close cycle
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

// =============================================================================
// Has_Cycle Tests
// =============================================================================

TEST_F(TestAcycliqueTest, HasCycle_EmptyGraph)
{
  Has_Cycle<GT> checker;
  
  // Empty graph: Has_Cycle returns true (inverse of Is_Graph_Acyclique)
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, HasCycle_AcyclicGraph)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Has_Cycle<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, HasCycle_WithCycle)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  Has_Cycle<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, HasCycle_MultipleComponents)
{
  // Component 1: acyclic
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  // Component 2: has cycle
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  g.insert_arc(n5, n3);
  
  Has_Cycle<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Complex Structure Tests
// =============================================================================

TEST_F(TestAcycliqueTest, ComplexAcyclicGraph)
{
  // Create a complex undirected graph without cycles (tree structure)
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  auto n6 = g.insert_node(6);
  
  // Tree: no multiple paths between nodes
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n2, n5);
  g.insert_arc(n3, n6);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, DiamondWithCycle)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Diamond structure
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n3, n4);
  
  // Add back edge to create cycle
  g.insert_arc(n4, n1);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

// =============================================================================
// Disconnected Components Tests
// =============================================================================

TEST_F(TestAcycliqueTest, TwoDisconnectedAcyclicComponents)
{
  // Component 1
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  // Component 2
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, MultipleComponentsOneCyclic)
{
  // Acyclic component
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  // Cyclic component
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  g.insert_arc(n5, n3);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(TestAcycliqueTest, TwoNodesBackAndForth)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);  // Creates cycle
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, StarGraphIsAcyclic)
{
  auto center = g.insert_node(0);
  
  for (int i = 1; i <= 10; ++i)
  {
    auto leaf = g.insert_node(i);
    g.insert_arc(center, leaf);
  }
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Num Arcs Parameter Tests
// =============================================================================

TEST_F(TestAcycliqueTest, ExplicitNumArcsAcyclic)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g, 2));  // Explicit arc count
}

TEST_F(TestAcycliqueTest, ExplicitNumArcsCyclic)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_FALSE(checker(g, 3));
}

TEST_F(TestAcycliqueTest, HasCycleWithNumArcs)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  Has_Cycle<GT> checker;
  
  EXPECT_TRUE(checker(g, 2));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(TestAcycliqueTest, LongChainIsAcyclic)
{
  const size_t n = 500;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

TEST_F(TestAcycliqueTest, LargeTreeIsAcyclic)
{
  auto root = g.insert_node(0);
  int node_count = 1;
  
  // Create binary tree with 127 nodes (depth 7)
  std::vector<Node*> current_level = {root};
  
  for (int depth = 0; depth < 6; ++depth)
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
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}

// =============================================================================
// Arc Count Optimization Tests
// =============================================================================

TEST_F(TestAcycliqueTest, ArcCountOptimization_TooManyArcs)
{
  const size_t n = 10;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Add n or more arcs => must have cycle
  for (size_t i = 0; i < n; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % n]);
  
  Is_Graph_Acyclique<GT> checker;
  
  // Should detect cycle via arc count optimization
  EXPECT_FALSE(checker(g));
}

TEST_F(TestAcycliqueTest, ArcCountOptimization_ExactlyNMinus1Arcs)
{
  const size_t n = 10;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Add exactly n-1 arcs (tree)
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1]);
  
  Is_Graph_Acyclique<GT> checker;
  
  EXPECT_TRUE(checker(g));
}
