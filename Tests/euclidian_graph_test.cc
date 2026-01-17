/**
 * @file euclidian_graph_test.cc
 * @brief Comprehensive tests for tpl_euclidian_graph.H
 */

#include <gtest/gtest.h>
#include <tpl_euclidian_graph.H>
#include <point.H>

using namespace Aleph;

// =============================================================================
// Type Definitions
// =============================================================================

using ENode = Euclidian_Node<int>;
using EArc = Euclidian_Arc<double>;
using EGraph = Euclidian_Graph<ENode, EArc>;
using EDigraph = Euclidian_Digraph<ENode, EArc>;

// =============================================================================
// Test Fixture
// =============================================================================

class EuclidianGraphTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// =============================================================================
// Node Tests
// =============================================================================

TEST_F(EuclidianGraphTest, NodeDefaultConstructor)
{
  ENode node;
  Point p = node.get_position();
  EXPECT_EQ(p.get_x(), 0.0);
  EXPECT_EQ(p.get_y(), 0.0);
}

TEST_F(EuclidianGraphTest, NodeWithInfo)
{
  ENode node(42);
  EXPECT_EQ(node.get_info(), 42);
}

TEST_F(EuclidianGraphTest, NodeWithPosition)
{
  Point p(10.5, 20.3);
  ENode node(p);
  
  Point retrieved = node.get_position();
  EXPECT_DOUBLE_EQ(retrieved.get_x().get_d(), 10.5);
  EXPECT_DOUBLE_EQ(retrieved.get_y().get_d(), 20.3);
}

TEST_F(EuclidianGraphTest, NodeWithInfoAndPosition)
{
  Point p(5.0, 15.0);
  ENode node(100, p);
  
  EXPECT_EQ(node.get_info(), 100);
  Point retrieved = node.get_position();
  EXPECT_DOUBLE_EQ(retrieved.get_x().get_d(), 5.0);
  EXPECT_DOUBLE_EQ(retrieved.get_y().get_d(), 15.0);
}

TEST_F(EuclidianGraphTest, NodeCopyConstructor)
{
  Point p(7.5, 8.5);
  ENode node1(50, p);
  ENode node2(&node1);
  
  EXPECT_EQ(node2.get_info(), 50);
  Point retrieved = node2.get_position();
  EXPECT_DOUBLE_EQ(retrieved.get_x().get_d(), 7.5);
  EXPECT_DOUBLE_EQ(retrieved.get_y().get_d(), 8.5);
}

// =============================================================================
// Arc Tests
// =============================================================================

TEST_F(EuclidianGraphTest, ArcDefaultConstructor)
{
  EArc arc;
  EXPECT_EQ(arc.get_info(), 0.0);
}

TEST_F(EuclidianGraphTest, ArcWithInfo)
{
  EArc arc(3.14);
  EXPECT_DOUBLE_EQ(arc.get_info(), 3.14);
}

// =============================================================================
// Graph Construction Tests
// =============================================================================

TEST_F(EuclidianGraphTest, EmptyGraph)
{
  EGraph g;
  EXPECT_EQ(g.get_num_nodes(), 0u);
  EXPECT_EQ(g.get_num_arcs(), 0u);
}

TEST_F(EuclidianGraphTest, InsertNodeWithInfo)
{
  EGraph g;
  auto node = g.insert_node(42);
  
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->get_info(), 42);
  EXPECT_EQ(g.get_num_nodes(), 1u);
}

TEST_F(EuclidianGraphTest, InsertNodeWithPosition)
{
  EGraph g;
  Point p(10.0, 20.0);
  auto node = g.insert_node(p);
  
  ASSERT_NE(node, nullptr);
  Point retrieved = node->get_position();
  EXPECT_DOUBLE_EQ(retrieved.get_x().get_d(), 10.0);
  EXPECT_DOUBLE_EQ(retrieved.get_y().get_d(), 20.0);
  EXPECT_EQ(g.get_num_nodes(), 1u);
}

TEST_F(EuclidianGraphTest, InsertNodeWithInfoAndPosition)
{
  EGraph g;
  Point p(5.5, 7.5);
  auto node = g.insert_node(100, p);
  
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->get_info(), 100);
  Point retrieved = node->get_position();
  EXPECT_DOUBLE_EQ(retrieved.get_x().get_d(), 5.5);
  EXPECT_DOUBLE_EQ(retrieved.get_y().get_d(), 7.5);
  EXPECT_EQ(g.get_num_nodes(), 1u);
}

TEST_F(EuclidianGraphTest, InsertMultipleNodes)
{
  EGraph g;
  
  auto n1 = g.insert_node(Point(0.0, 0.0));
  auto n2 = g.insert_node(Point(10.0, 0.0));
  auto n3 = g.insert_node(Point(10.0, 10.0));
  
  ASSERT_NE(n1, nullptr);
  ASSERT_NE(n2, nullptr);
  ASSERT_NE(n3, nullptr);
  EXPECT_EQ(g.get_num_nodes(), 3u);
}

// =============================================================================
// Arc Insertion Tests
// =============================================================================

TEST_F(EuclidianGraphTest, InsertArc)
{
  EGraph g;
  auto n1 = g.insert_node(Point(0.0, 0.0));
  auto n2 = g.insert_node(Point(3.0, 4.0));
  
  auto arc = g.insert_arc(n1, n2);
  
  ASSERT_NE(arc, nullptr);
  EXPECT_EQ(g.get_num_arcs(), 1u);
  EXPECT_EQ(g.get_src_node(arc), n1);
  EXPECT_EQ(g.get_tgt_node(arc), n2);
}

TEST_F(EuclidianGraphTest, InsertArcWithInfo)
{
  EGraph g;
  auto n1 = g.insert_node(Point(0.0, 0.0));
  auto n2 = g.insert_node(Point(1.0, 1.0));
  
  auto arc = g.insert_arc(n1, n2, 99.9);
  
  ASSERT_NE(arc, nullptr);
  EXPECT_DOUBLE_EQ(arc->get_info(), 99.9);
}

TEST_F(EuclidianGraphTest, InsertMultipleArcs)
{
  EGraph g;
  auto n1 = g.insert_node(Point(0.0, 0.0));
  auto n2 = g.insert_node(Point(1.0, 0.0));
  auto n3 = g.insert_node(Point(1.0, 1.0));
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n1, n3);
  
  EXPECT_EQ(g.get_num_arcs(), 3u);
}

// =============================================================================
// Distance Calculation Tests
// =============================================================================

TEST_F(EuclidianGraphTest, CalculateDistance)
{
  EGraph g;
  auto n1 = g.insert_node(Point(0.0, 0.0));
  auto n2 = g.insert_node(Point(3.0, 4.0));
  
  auto arc = g.insert_arc(n1, n2);
  Geom_Number dist = g.get_distance(arc);
  
  // Distance = sqrt(3^2 + 4^2) = 5.0
  EXPECT_DOUBLE_EQ(dist.get_d(), 5.0);
}

TEST_F(EuclidianGraphTest, CalculateDistanceZero)
{
  EGraph g;
  auto n1 = g.insert_node(Point(5.0, 5.0));
  auto n2 = g.insert_node(Point(5.0, 5.0));
  
  auto arc = g.insert_arc(n1, n2);
  Geom_Number dist = g.get_distance(arc);
  
  EXPECT_DOUBLE_EQ(dist.get_d(), 0.0);
}

TEST_F(EuclidianGraphTest, CalculateDistanceHorizontal)
{
  EGraph g;
  auto n1 = g.insert_node(Point(0.0, 5.0));
  auto n2 = g.insert_node(Point(10.0, 5.0));
  
  auto arc = g.insert_arc(n1, n2);
  Geom_Number dist = g.get_distance(arc);
  
  EXPECT_DOUBLE_EQ(dist.get_d(), 10.0);
}

TEST_F(EuclidianGraphTest, CalculateDistanceVertical)
{
  EGraph g;
  auto n1 = g.insert_node(Point(5.0, 0.0));
  auto n2 = g.insert_node(Point(5.0, 8.0));
  
  auto arc = g.insert_arc(n1, n2);
  Geom_Number dist = g.get_distance(arc);
  
  EXPECT_DOUBLE_EQ(dist.get_d(), 8.0);
}

// =============================================================================
// Search Tests
// =============================================================================

TEST_F(EuclidianGraphTest, SearchNodeByPosition)
{
  EGraph g;
  Point p1(10.0, 20.0);
  Point p2(30.0, 40.0);
  
  auto n1 = g.insert_node(p1);
  auto n2 = g.insert_node(p2);
  
  auto found1 = g.search_node(p1);
  auto found2 = g.search_node(p2);
  
  EXPECT_EQ(found1, n1);
  EXPECT_EQ(found2, n2);
}

TEST_F(EuclidianGraphTest, SearchNodeNotFound)
{
  EGraph g;
  g.insert_node(Point(10.0, 20.0));
  
  Point search_point(99.0, 99.0);
  auto found = g.search_node(search_point);
  
  EXPECT_EQ(found, nullptr);
}

TEST_F(EuclidianGraphTest, SearchInEmptyGraph)
{
  EGraph g;
  Point p(5.0, 5.0);
  
  auto found = g.search_node(p);
  EXPECT_EQ(found, nullptr);
}

// =============================================================================
// Copy Constructor Tests
// =============================================================================

TEST_F(EuclidianGraphTest, CopyConstructor)
{
  EGraph g1;
  auto n1 = g1.insert_node(Point(0.0, 0.0));
  auto n2 = g1.insert_node(Point(1.0, 1.0));
  g1.insert_arc(n1, n2);
  
  EGraph g2(g1);
  
  EXPECT_EQ(g2.get_num_nodes(), g1.get_num_nodes());
  EXPECT_EQ(g2.get_num_arcs(), g1.get_num_arcs());
}

// =============================================================================
// Assignment Operator Tests
// =============================================================================

TEST_F(EuclidianGraphTest, AssignmentOperator)
{
  EGraph g1;
  auto n1 = g1.insert_node(Point(0.0, 0.0));
  auto n2 = g1.insert_node(Point(5.0, 5.0));
  g1.insert_arc(n1, n2);
  
  EGraph g2;
  g2 = g1;
  
  EXPECT_EQ(g2.get_num_nodes(), 2u);
  EXPECT_EQ(g2.get_num_arcs(), 1u);
}

TEST_F(EuclidianGraphTest, SelfAssignment)
{
  EGraph g;
  auto n1 = g.insert_node(Point(0.0, 0.0));
  auto n2 = g.insert_node(Point(1.0, 1.0));
  g.insert_arc(n1, n2);
  
  g = g;  // Self-assignment
  
  EXPECT_EQ(g.get_num_nodes(), 2u);
  EXPECT_EQ(g.get_num_arcs(), 1u);
}

// =============================================================================
// Digraph Tests
// =============================================================================

TEST_F(EuclidianGraphTest, DigraphConstruction)
{
  EDigraph dg;
  EXPECT_TRUE(dg.is_digraph());
  EXPECT_EQ(dg.get_num_nodes(), 0u);
}

TEST_F(EuclidianGraphTest, DigraphInsertNodes)
{
  EDigraph dg;
  auto n1 = dg.insert_node(Point(0.0, 0.0));
  auto n2 = dg.insert_node(Point(10.0, 10.0));
  
  EXPECT_EQ(dg.get_num_nodes(), 2u);
  EXPECT_TRUE(dg.is_digraph());
}

TEST_F(EuclidianGraphTest, DigraphInsertArcs)
{
  EDigraph dg;
  auto n1 = dg.insert_node(Point(0.0, 0.0));
  auto n2 = dg.insert_node(Point(5.0, 5.0));
  
  auto arc = dg.insert_arc(n1, n2);
  
  ASSERT_NE(arc, nullptr);
  EXPECT_EQ(dg.get_num_arcs(), 1u);
}

TEST_F(EuclidianGraphTest, DigraphCopyConstructor)
{
  EDigraph dg1;
  dg1.insert_node(Point(0.0, 0.0));
  dg1.insert_node(Point(1.0, 1.0));
  
  EDigraph dg2(dg1);
  
  EXPECT_TRUE(dg2.is_digraph());
  EXPECT_EQ(dg2.get_num_nodes(), dg1.get_num_nodes());
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(EuclidianGraphTest, ManyNodes)
{
  EGraph g;
  const size_t n = 1000;
  
  for (size_t i = 0; i < n; ++i)
  {
    double x = static_cast<double>(i);
    double y = static_cast<double>(i * 2);
    g.insert_node(Point(x, y));
  }
  
  EXPECT_EQ(g.get_num_nodes(), n);
}

TEST_F(EuclidianGraphTest, ManyArcs)
{
  EGraph g;
  const size_t n = 100;
  
  std::vector<ENode*> nodes;
  for (size_t i = 0; i < n; ++i)
  {
    nodes.push_back(g.insert_node(Point(i, i)));
  }
  
  // Create complete graph
  for (size_t i = 0; i < n; ++i)
  {
    for (size_t j = i + 1; j < n; ++j)
    {
      g.insert_arc(nodes[i], nodes[j]);
    }
  }
  
  size_t expected_arcs = n * (n - 1) / 2;
  EXPECT_EQ(g.get_num_arcs(), expected_arcs);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(EuclidianGraphTest, NegativeCoordinates)
{
  EGraph g;
  auto n1 = g.insert_node(Point(-5.0, -10.0));
  auto n2 = g.insert_node(Point(-1.0, -2.0));
  
  ASSERT_NE(n1, nullptr);
  ASSERT_NE(n2, nullptr);
  
  auto arc = g.insert_arc(n1, n2);
  Geom_Number dist = g.get_distance(arc);
  
  EXPECT_GT(dist.get_d(), 0.0);
}

TEST_F(EuclidianGraphTest, LargeCoordinates)
{
  EGraph g;
  auto n1 = g.insert_node(Point(1e6, 1e6));
  auto n2 = g.insert_node(Point(1e6 + 1, 1e6 + 1));
  
  auto arc = g.insert_arc(n1, n2);
  Geom_Number dist = g.get_distance(arc);
  
  EXPECT_DOUBLE_EQ(dist.get_d(), std::sqrt(2.0));
}
