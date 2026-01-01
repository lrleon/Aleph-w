/**
 * Comprehensive test suite for tpl_components.H
 * 
 * Tests all aspects of connected components algorithms including:
 * - Build_Subgraph (subgraph construction from a node)
 * - Inconnected_Components (finding all connected components)
 * - All graph types: List_Graph, List_SGraph, Array_Graph (directed/undirected)
 * - Edge cases (empty graphs, single nodes, disconnected graphs)
 * - Node/arc mappings
 * - Error handling
 */

#include <gtest/gtest.h>
#include <tpl_components.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <tpl_agraph.H>

using namespace Aleph;

//==============================================================================
// Graph Type Definitions - All 6 combinations
//==============================================================================

// List-based graphs (tpl_graph.H)
using ListGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using ListDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

// Sparse graphs (tpl_sgraph.H)
using SparseGraph = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;
using SparseDigraph = List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>;

// Array-based graphs (tpl_agraph.H)
using ArrayGraph = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;
using ArrayDigraph = Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>;

//==============================================================================
// Test Fixtures
//==============================================================================

class BuildSubgraphTest : public ::testing::Test
{
protected:
  using Graph = ListGraph;
  Graph g;
  Graph sg;
};

class InconnectedComponentsTest : public ::testing::Test
{
protected:
  using Graph = ListGraph;
  Graph g;
};

//==============================================================================
// Build_Subgraph Basic Tests
//==============================================================================

TEST_F(BuildSubgraphTest, SingleNode)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  g.reset_nodes();
  g.reset_arcs();
  
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  EXPECT_EQ(sg.get_num_nodes(), 1);
  EXPECT_EQ(sg.get_num_arcs(), 0);
}

TEST_F(BuildSubgraphTest, TwoNodesConnected)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  g.reset_nodes();
  g.reset_arcs();
  
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  EXPECT_EQ(sg.get_num_nodes(), 2);
  EXPECT_EQ(sg.get_num_arcs(), 1);
}

TEST_F(BuildSubgraphTest, TriangleGraph)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  g.reset_nodes();
  g.reset_arcs();
  
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  EXPECT_EQ(sg.get_num_nodes(), 3);
  EXPECT_EQ(sg.get_num_arcs(), 3);
}

TEST_F(BuildSubgraphTest, DisconnectedGraph_BuildsOneComponent)
{
  // Create two disconnected components
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  g.reset_nodes();
  g.reset_arcs();
  
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  // Should only build the component containing n1
  EXPECT_EQ(sg.get_num_nodes(), 2);
  EXPECT_EQ(sg.get_num_arcs(), 1);
}

TEST_F(BuildSubgraphTest, NodeMapping)
{
  [[maybe_unused]] auto n1 = g.insert_node(10);
  auto n2 = g.insert_node(20);
  g.insert_arc(n1, n2);
  g.reset_nodes();
  g.reset_arcs();
  
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  // Verify mappings
  auto sg_n1 = mapped_node<Graph>(n1);
  auto sg_n2 = mapped_node<Graph>(n2);
  
  EXPECT_NE(sg_n1, nullptr);
  EXPECT_NE(sg_n2, nullptr);
  EXPECT_EQ(sg_n1->get_info(), 10);
  EXPECT_EQ(sg_n2->get_info(), 20);
}

TEST_F(BuildSubgraphTest, ReturnSubgraph)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.reset_nodes();
  g.reset_arcs();
  
  Build_Subgraph<Graph> builder;
  auto result = builder(g, n1);
  
  EXPECT_EQ(result.get_num_nodes(), 3);
  EXPECT_EQ(result.get_num_arcs(), 2);
}

TEST_F(BuildSubgraphTest, BuildNodeList)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.reset_nodes();
  g.reset_arcs();
  
  DynList<Graph::Node*> node_list;
  Build_Subgraph<Graph> builder;
  builder(g, node_list, n1);
  
  EXPECT_EQ(node_list.size(), 3);
}

TEST_F(BuildSubgraphTest, NullptrThrows)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Build_Subgraph<Graph> builder;
  
  EXPECT_THROW(builder(g, sg, nullptr), std::invalid_argument);
}

//==============================================================================
// Inconnected_Components Basic Tests
//==============================================================================

TEST_F(InconnectedComponentsTest, SingleComponent)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  DynList<Graph> components;
  Inconnected_Components<Graph> cc;
  cc(g, components);
  
  EXPECT_EQ(components.size(), 1);
  EXPECT_EQ(components.get_first().get_num_nodes(), 3);
}

TEST_F(InconnectedComponentsTest, TwoComponents)
{
  // Component 1
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  // Component 2 (disconnected)
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  DynList<Graph> components;
  Inconnected_Components<Graph> cc;
  cc(g, components);
  
  EXPECT_EQ(components.size(), 2);
  
  // Each component should have 2 nodes
  size_t total_nodes = 0;
  for (auto & comp : components)
    total_nodes += comp.get_num_nodes();
  EXPECT_EQ(total_nodes, 4);
}

TEST_F(InconnectedComponentsTest, ThreeComponents)
{
  // Three isolated nodes = three components
  g.insert_node(1);
  g.insert_node(2);
  g.insert_node(3);
  
  DynList<Graph> components;
  Inconnected_Components<Graph> cc;
  cc(g, components);
  
  EXPECT_EQ(components.size(), 3);
}

TEST_F(InconnectedComponentsTest, EmptyGraph)
{
  DynList<Graph> components;
  Inconnected_Components<Graph> cc;
  cc(g, components);
  
  EXPECT_EQ(components.size(), 0);
}

TEST_F(InconnectedComponentsTest, ComputeAsNodeLists)
{
  // Two components
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  [[maybe_unused]] auto n3 = g.insert_node(3);
  
  DynList<DynList<Graph::Node*>> node_lists;
  Inconnected_Components<Graph> cc;
  cc(g, node_lists);
  
  EXPECT_EQ(node_lists.size(), 2);
}

TEST_F(InconnectedComponentsTest, CountComponents)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  g.insert_node(3);  // Isolated
  g.insert_node(4);  // Isolated
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_EQ(cc.count_components(g), 3);
}

TEST_F(InconnectedComponentsTest, IsConnected_True)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_TRUE(cc.is_connected(g));
}

TEST_F(InconnectedComponentsTest, IsConnected_False)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  g.insert_node(3);  // Isolated
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_FALSE(cc.is_connected(g));
}

TEST_F(InconnectedComponentsTest, IsConnected_EmptyGraph)
{
  Inconnected_Components<Graph> cc;
  
  EXPECT_TRUE(cc.is_connected(g));  // Empty graph is considered connected
}

TEST_F(InconnectedComponentsTest, IsConnected_SingleNode)
{
  g.insert_node(1);
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_TRUE(cc.is_connected(g));
}

//==============================================================================
// Stress Tests
//==============================================================================

TEST_F(InconnectedComponentsTest, ManyComponents)
{
  // Create 50 isolated nodes = 50 components
  for (int i = 0; i < 50; ++i)
    g.insert_node(i);
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_EQ(cc.count_components(g), 50);
}

TEST_F(InconnectedComponentsTest, LargeConnectedGraph)
{
  const int N = 100;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  // Create a connected chain
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes(i), nodes(i + 1));
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_TRUE(cc.is_connected(g));
  EXPECT_EQ(cc.count_components(g), 1);
}

//==============================================================================
// Typed Tests for All Graph Types
//==============================================================================

template <typename GraphType>
class ComponentsAllGraphs : public ::testing::Test
{
protected:
  GraphType g;
};

using GraphTypes = ::testing::Types<
  ListGraph,
  ListDigraph,
  SparseGraph,
  SparseDigraph,
  ArrayGraph,
  ArrayDigraph
>;

TYPED_TEST_SUITE(ComponentsAllGraphs, GraphTypes);

// Undirected graphs only - for connectivity tests
// (Inconnected_Components is designed for undirected graphs)
template <typename GraphType>
class ComponentsUndirectedGraphs : public ::testing::Test
{
protected:
  GraphType g;
};

using UndirectedGraphTypes = ::testing::Types<
  ListGraph,
  SparseGraph,
  ArrayGraph
>;

TYPED_TEST_SUITE(ComponentsUndirectedGraphs, UndirectedGraphTypes);

TYPED_TEST(ComponentsAllGraphs, BuildSubgraphSingleNode)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  g.reset_nodes();
  g.reset_arcs();
  
  Graph sg;
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  EXPECT_EQ(sg.get_num_nodes(), 1);
  EXPECT_EQ(sg.get_num_arcs(), 0);
}

TYPED_TEST(ComponentsAllGraphs, BuildSubgraphTriangle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  g.reset_nodes();
  g.reset_arcs();
  
  Graph sg;
  Build_Subgraph<Graph> builder;
  builder(g, sg, n1);
  
  EXPECT_EQ(sg.get_num_nodes(), 3);
  EXPECT_EQ(sg.get_num_arcs(), 3);
}

TYPED_TEST(ComponentsAllGraphs, BuildSubgraphNullptrThrows)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Graph sg;
  Build_Subgraph<Graph> builder;
  
  EXPECT_THROW(builder(g, sg, nullptr), std::invalid_argument);
}

TYPED_TEST(ComponentsUndirectedGraphs, SingleComponent)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  DynList<Graph> components;
  Inconnected_Components<Graph> cc;
  cc(g, components);
  
  EXPECT_EQ(components.size(), 1);
}

TYPED_TEST(ComponentsUndirectedGraphs, TwoComponents)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  DynList<Graph> components;
  Inconnected_Components<Graph> cc;
  cc(g, components);
  
  EXPECT_EQ(components.size(), 2);
}

TYPED_TEST(ComponentsUndirectedGraphs, CountComponents)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  g.insert_node(1);
  g.insert_node(2);
  g.insert_node(3);
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_EQ(cc.count_components(g), 3);
}

TYPED_TEST(ComponentsUndirectedGraphs, IsConnectedTrue)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_TRUE(cc.is_connected(g));
}

TYPED_TEST(ComponentsUndirectedGraphs, IsConnectedFalse)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  g.insert_node(3);
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_FALSE(cc.is_connected(g));
}

TYPED_TEST(ComponentsUndirectedGraphs, EmptyGraphIsConnected)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  
  Inconnected_Components<Graph> cc;
  
  EXPECT_TRUE(cc.is_connected(g));
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
