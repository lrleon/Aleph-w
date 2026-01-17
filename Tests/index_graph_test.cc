/**
 * @file index_graph_test.cc
 * @brief Comprehensive tests for tpl_indexGraph.H
 */

#include <gtest/gtest.h>
#include <tpl_indexGraph.H>
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

class IndexGraphTest : public ::testing::Test
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
// Basic Construction Tests
// =============================================================================

TEST_F(IndexGraphTest, Construction)
{
  Index_Graph<GT> idx(g);
  
  EXPECT_EQ(idx.get_num_nodes(), 0u);
  EXPECT_EQ(idx.get_num_arcs(), 0u);
}

TEST_F(IndexGraphTest, InsertSingleNode)
{
  Index_Graph<GT> idx(g);
  
  auto n = idx.insert_node(42);
  
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(n->get_info(), 42);
  EXPECT_EQ(idx.get_num_nodes(), 1u);
  EXPECT_EQ(g.get_num_nodes(), 1u);
}

TEST_F(IndexGraphTest, InsertMultipleNodes)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(10);
  auto n2 = idx.insert_node(20);
  auto n3 = idx.insert_node(30);
  
  ASSERT_NE(n1, nullptr);
  ASSERT_NE(n2, nullptr);
  ASSERT_NE(n3, nullptr);
  EXPECT_EQ(idx.get_num_nodes(), 3u);
}

// =============================================================================
// Node Search Tests
// =============================================================================

TEST_F(IndexGraphTest, SearchNodeByPointer)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(100);
  auto n2 = idx.insert_node(200);
  
  auto found1 = idx.search_node(n1);
  auto found2 = idx.search_node(n2);
  
  EXPECT_EQ(found1, n1);
  EXPECT_EQ(found2, n2);
}

TEST_F(IndexGraphTest, SearchNodeByInfo)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(100);
  auto n2 = idx.insert_node(200);
  
  auto found1 = idx.search_node(100);
  auto found2 = idx.search_node(200);
  
  EXPECT_EQ(found1, n1);
  EXPECT_EQ(found2, n2);
}

TEST_F(IndexGraphTest, SearchNonExistentNode)
{
  Index_Graph<GT> idx(g);
  
  idx.insert_node(10);
  idx.insert_node(20);
  
  auto found = idx.search_node(999);
  
  EXPECT_EQ(found, nullptr);
}

TEST_F(IndexGraphTest, SearchInEmptyIndex)
{
  Index_Graph<GT> idx(g);
  
  auto found = idx.search_node(42);
  
  EXPECT_EQ(found, nullptr);
}

// =============================================================================
// Arc Insertion Tests
// =============================================================================

TEST_F(IndexGraphTest, InsertArc)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  
  auto arc = idx.insert_arc(n1, n2);
  
  ASSERT_NE(arc, nullptr);
  EXPECT_EQ(idx.get_num_arcs(), 1u);
  EXPECT_EQ(g.get_num_arcs(), 1u);
}

TEST_F(IndexGraphTest, InsertArcWithInfo)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(10);
  auto n2 = idx.insert_node(20);
  
  auto arc = idx.insert_arc(n1, n2, 999);
  
  ASSERT_NE(arc, nullptr);
  EXPECT_EQ(arc->get_info(), 999);
}

TEST_F(IndexGraphTest, InsertMultipleArcs)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  auto n3 = idx.insert_node(3);
  
  idx.insert_arc(n1, n2);
  idx.insert_arc(n2, n3);
  idx.insert_arc(n1, n3);
  
  EXPECT_EQ(idx.get_num_arcs(), 3u);
}

// =============================================================================
// Arc Search Tests
// =============================================================================

TEST_F(IndexGraphTest, SearchArc)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  
  auto arc = idx.insert_arc(n1, n2);
  
  auto found = idx.search_arc(n1, n2);
  
  EXPECT_EQ(found, arc);
}

TEST_F(IndexGraphTest, SearchNonExistentArc)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  auto n3 = idx.insert_node(3);
  
  idx.insert_arc(n1, n2);
  
  auto found = idx.search_arc(n1, n3);
  
  EXPECT_EQ(found, nullptr);
}

TEST_F(IndexGraphTest, SearchArcInEmptyGraph)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  
  auto found = idx.search_arc(n1, n2);
  
  EXPECT_EQ(found, nullptr);
}

// =============================================================================
// Node Removal Tests
// =============================================================================

TEST_F(IndexGraphTest, RemoveNode)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(10);
  auto n2 = idx.insert_node(20);
  
  idx.remove_node(n1);
  
  EXPECT_EQ(idx.get_num_nodes(), 1u);
  EXPECT_EQ(g.get_num_nodes(), 1u);
  
  auto found = idx.search_node(10);
  EXPECT_EQ(found, nullptr);
}

TEST_F(IndexGraphTest, RemoveNodeWithArcs)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  auto n3 = idx.insert_node(3);
  
  idx.insert_arc(n1, n2);
  idx.insert_arc(n1, n3);
  
  idx.remove_node(n1);
  
  EXPECT_EQ(idx.get_num_nodes(), 2u);
  EXPECT_EQ(idx.get_num_arcs(), 0u);  // Arcs connected to n1 removed
}

TEST_F(IndexGraphTest, RemoveMultipleNodes)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  auto n3 = idx.insert_node(3);
  
  idx.remove_node(n1);
  idx.remove_node(n2);
  
  EXPECT_EQ(idx.get_num_nodes(), 1u);
}

// =============================================================================
// Arc Removal Tests
// =============================================================================

TEST_F(IndexGraphTest, RemoveArc)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  
  auto arc = idx.insert_arc(n1, n2);
  
  idx.remove_arc(arc);
  
  EXPECT_EQ(idx.get_num_arcs(), 0u);
  EXPECT_EQ(g.get_num_arcs(), 0u);
  
  auto found = idx.search_arc(n1, n2);
  EXPECT_EQ(found, nullptr);
}

TEST_F(IndexGraphTest, RemoveMultipleArcs)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  auto n3 = idx.insert_node(3);
  
  auto arc1 = idx.insert_arc(n1, n2);
  auto arc2 = idx.insert_arc(n2, n3);
  
  idx.remove_arc(arc1);
  
  EXPECT_EQ(idx.get_num_arcs(), 1u);
  
  idx.remove_arc(arc2);
  
  EXPECT_EQ(idx.get_num_arcs(), 0u);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(IndexGraphTest, ManyNodes)
{
  Index_Graph<GT> idx(g);
  
  const size_t n = 1000;
  
  for (size_t i = 0; i < n; ++i)
  {
    idx.insert_node(i);
  }
  
  EXPECT_EQ(idx.get_num_nodes(), n);
  
  // Search for random nodes
  for (size_t i = 0; i < n; i += 100)
  {
    auto found = idx.search_node(static_cast<int>(i));
    EXPECT_NE(found, nullptr);
  }
}

TEST_F(IndexGraphTest, ManyArcs)
{
  Index_Graph<GT> idx(g);
  
  const size_t n = 100;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
  {
    nodes.push_back(idx.insert_node(i));
  }
  
  // Create chain
  for (size_t i = 0; i < n - 1; ++i)
  {
    idx.insert_arc(nodes[i], nodes[i + 1]);
  }
  
  EXPECT_EQ(idx.get_num_arcs(), n - 1);
}

TEST_F(IndexGraphTest, CompleteGraphSmall)
{
  Index_Graph<GT> idx(g);
  
  const size_t n = 20;
  std::vector<Node*> nodes;
  
  for (size_t i = 0; i < n; ++i)
  {
    nodes.push_back(idx.insert_node(i));
  }
  
  for (size_t i = 0; i < n; ++i)
  {
    for (size_t j = i + 1; j < n; ++j)
    {
      idx.insert_arc(nodes[i], nodes[j]);
    }
  }
  
  size_t expected = n * (n - 1) / 2;
  EXPECT_EQ(idx.get_num_arcs(), expected);
}

// =============================================================================
// Mixed Operations Tests
// =============================================================================

TEST_F(IndexGraphTest, InsertSearchRemove)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(100);
  auto n2 = idx.insert_node(200);
  
  auto found = idx.search_node(100);
  EXPECT_EQ(found, n1);
  
  idx.remove_node(n1);
  
  found = idx.search_node(100);
  EXPECT_EQ(found, nullptr);
}

TEST_F(IndexGraphTest, InsertRemoveInsert)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(42);
  idx.remove_node(n1);
  
  auto n2 = idx.insert_node(42);
  
  ASSERT_NE(n2, nullptr);
  EXPECT_EQ(idx.get_num_nodes(), 1u);
}

// =============================================================================
// Duplicate Handling
// =============================================================================

TEST_F(IndexGraphTest, DuplicateNodes)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(42);
  auto n2 = idx.insert_node(42);  // Same value
  
  // Index_Graph uses value-based indexing: duplicate value doesn't add new node
  // Second insert may return different pointer but doesn't increase node count
  EXPECT_EQ(idx.get_num_nodes(), 1u);
  
  // Both refer to nodes with value 42
  EXPECT_EQ(n1->get_info(), 42);
  EXPECT_EQ(n2->get_info(), 42);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(IndexGraphTest, EmptyGraph)
{
  Index_Graph<GT> idx(g);
  
  EXPECT_EQ(idx.get_num_nodes(), 0u);
  EXPECT_EQ(idx.get_num_arcs(), 0u);
}

TEST_F(IndexGraphTest, SingleNodeNoArcs)
{
  Index_Graph<GT> idx(g);
  
  auto n = idx.insert_node(1);
  
  EXPECT_EQ(idx.get_num_nodes(), 1u);
  EXPECT_EQ(idx.get_num_arcs(), 0u);
  
  auto found = idx.search_node(1);
  EXPECT_EQ(found, n);
}

TEST_F(IndexGraphTest, SelfLoop)
{
  Index_Graph<GT> idx(g);
  
  auto n = idx.insert_node(1);
  auto arc = idx.insert_arc(n, n);
  
  ASSERT_NE(arc, nullptr);
  EXPECT_EQ(g.get_src_node(arc), n);
  EXPECT_EQ(g.get_tgt_node(arc), n);
}

// =============================================================================
// Graph Equality Tests (are_equal function)
// =============================================================================

TEST_F(IndexGraphTest, EqualityEmptyGraphs)
{
  GT g1, g2;
  
  EXPECT_TRUE(are_equal(g1, g2));
}

TEST_F(IndexGraphTest, EqualitySameStructure)
{
  GT g1, g2;
  
  auto n1_g1 = g1.insert_node(1);
  auto n2_g1 = g1.insert_node(2);
  g1.insert_arc(n1_g1, n2_g1);
  
  auto n1_g2 = g2.insert_node(1);
  auto n2_g2 = g2.insert_node(2);
  g2.insert_arc(n1_g2, n2_g2);
  
  EXPECT_TRUE(are_equal(g1, g2));
}

TEST_F(IndexGraphTest, EqualityDifferentNodes)
{
  GT g1, g2;
  
  g1.insert_node(1);
  g1.insert_node(2);
  
  g2.insert_node(1);
  g2.insert_node(3);  // Different
  
  EXPECT_FALSE(are_equal(g1, g2));
}

TEST_F(IndexGraphTest, EqualityDifferentArcs)
{
  GT g1, g2;
  
  auto n1_g1 = g1.insert_node(1);
  auto n2_g1 = g1.insert_node(2);
  g1.insert_arc(n1_g1, n2_g1);
  
  auto n1_g2 = g2.insert_node(1);
  auto n2_g2 = g2.insert_node(2);
  // No arc in g2
  
  EXPECT_FALSE(are_equal(g1, g2));
}
