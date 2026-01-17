
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file tpl_spanning_tree_test.cc
 * @brief Tests for Tpl Spanning Tree
 */

/**
 * Comprehensive test suite for tpl_spanning_tree.H
 * 
 * Tests all aspects of spanning tree algorithms including:
 * - Depth-first spanning tree (DFS)
 * - Breadth-first spanning tree (BFS)
 * - All graph types: List_Graph, List_SGraph, Array_Graph (directed/undirected)
 * - Edge cases (empty graphs, single nodes, disconnected graphs)
 * - Node/arc mappings
 * - Error handling
 */

#include <gtest/gtest.h>
#include <tpl_spanning_tree.H>
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

class DFSSpanningTreeTest : public ::testing::Test
{
protected:
  using Graph = ListGraph;
  Graph g;
  Graph tree;
};

class BFSSpanningTreeTest : public ::testing::Test
{
protected:
  using Graph = ListGraph;
  Graph g;
  Graph tree;
};

//==============================================================================
// DFS Spanning Tree Basic Tests
//==============================================================================

TEST_F(DFSSpanningTreeTest, SingleNode)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 1);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

TEST_F(DFSSpanningTreeTest, TwoNodesConnected)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 2);
  EXPECT_EQ(tree.get_num_arcs(), 1);
}

TEST_F(DFSSpanningTreeTest, TriangleGraph)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 3);
  EXPECT_EQ(tree.get_num_arcs(), 2);  // V-1 arcs
}

TEST_F(DFSSpanningTreeTest, LinearChain)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4);
}

TEST_F(DFSSpanningTreeTest, CompleteGraph)
{
  const int N = 5;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  // Complete graph: connect all pairs
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes(i), nodes(j));
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), N);
  EXPECT_EQ(tree.get_num_arcs(), N - 1);  // Spanning tree has V-1 edges
}

TEST_F(DFSSpanningTreeTest, FromSpecificNode)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto tree_node = dfs(g, n2, tree);  // Start from n2
  
  ASSERT_NE(tree_node, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 3);
  EXPECT_EQ(tree.get_num_arcs(), 2);
}

TEST_F(DFSSpanningTreeTest, EmptyGraphThrows)
{
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  
  EXPECT_THROW(dfs(g, tree), std::domain_error);
}

TEST_F(DFSSpanningTreeTest, NullptrNodeThrows)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  
  EXPECT_THROW(dfs(g, nullptr, tree), std::invalid_argument);
}

TEST_F(DFSSpanningTreeTest, NodeMapping)
{
  [[maybe_unused]] auto n1 = g.insert_node(10);
  auto n2 = g.insert_node(20);
  auto n3 = g.insert_node(30);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  dfs(g, tree);
  
  // Verify node mappings are established
  auto t1 = mapped_node<Graph>(n1);
  auto t2 = mapped_node<Graph>(n2);
  auto t3 = mapped_node<Graph>(n3);
  
  EXPECT_NE(t1, nullptr);
  EXPECT_NE(t2, nullptr);
  EXPECT_NE(t3, nullptr);
  
  // Verify values are preserved
  EXPECT_EQ(t1->get_info(), 10);
  EXPECT_EQ(t2->get_info(), 20);
  EXPECT_EQ(t3->get_info(), 30);
}

//==============================================================================
// BFS Spanning Tree Basic Tests
//==============================================================================

TEST_F(BFSSpanningTreeTest, SingleNode)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 1);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

TEST_F(BFSSpanningTreeTest, TwoNodesConnected)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 2);
  EXPECT_EQ(tree.get_num_arcs(), 1);
}

TEST_F(BFSSpanningTreeTest, TriangleGraph)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 3);
  EXPECT_EQ(tree.get_num_arcs(), 2);
}

TEST_F(BFSSpanningTreeTest, CompleteGraph)
{
  const int N = 5;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes(i), nodes(j));
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), N);
  EXPECT_EQ(tree.get_num_arcs(), N - 1);
}

TEST_F(BFSSpanningTreeTest, FromSpecificNode)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  bfs(g, n2, tree);  // Start from n2
  
  EXPECT_EQ(tree.get_num_nodes(), 3);
  EXPECT_EQ(tree.get_num_arcs(), 2);
}

TEST_F(BFSSpanningTreeTest, EmptyGraphThrows)
{
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  
  EXPECT_THROW(bfs(g, tree), std::domain_error);
}

TEST_F(BFSSpanningTreeTest, NullptrNodeThrows)
{
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  
  EXPECT_THROW(bfs(g, nullptr, tree), std::invalid_argument);
}

TEST_F(BFSSpanningTreeTest, NodeMapping)
{
  [[maybe_unused]] auto n1 = g.insert_node(10);
  auto n2 = g.insert_node(20);
  auto n3 = g.insert_node(30);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  bfs(g, tree);
  
  auto t1 = mapped_node<Graph>(n1);
  auto t2 = mapped_node<Graph>(n2);
  auto t3 = mapped_node<Graph>(n3);
  
  EXPECT_NE(t1, nullptr);
  EXPECT_NE(t2, nullptr);
  EXPECT_NE(t3, nullptr);
  
  EXPECT_EQ(t1->get_info(), 10);
  EXPECT_EQ(t2->get_info(), 20);
  EXPECT_EQ(t3->get_info(), 30);
}

//==============================================================================
// Stress Tests
//==============================================================================

TEST_F(DFSSpanningTreeTest, LargeGraph)
{
  const int N = 100;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  // Create a connected graph
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes(i), nodes(i + 1));
  
  // Add some cross edges
  for (int i = 0; i < N - 10; i += 10)
    g.insert_arc(nodes(i), nodes(i + 10));
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), N);
  EXPECT_EQ(tree.get_num_arcs(), N - 1);
}

TEST_F(BFSSpanningTreeTest, LargeGraph)
{
  const int N = 100;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes(i), nodes(i + 1));
  
  for (int i = 0; i < N - 10; i += 10)
    g.insert_arc(nodes(i), nodes(i + 10));
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), N);
  EXPECT_EQ(tree.get_num_arcs(), N - 1);
}

//==============================================================================
// Typed Tests for All Graph Types
//==============================================================================

template <typename GraphType>
class SpanningTreeAllGraphs : public ::testing::Test
{
protected:
  GraphType g;
  GraphType tree;
};

using GraphTypes = ::testing::Types<
  ListGraph,
  ListDigraph,
  SparseGraph,
  SparseDigraph,
  ArrayGraph,
  ArrayDigraph
>;

TYPED_TEST_SUITE(SpanningTreeAllGraphs, GraphTypes);

// Undirected graphs only - for tests that assume bidirectional traversal
template <typename GraphType>
class SpanningTreeUndirectedGraphs : public ::testing::Test
{
protected:
  GraphType g;
  GraphType tree;
};

using UndirectedGraphTypes = ::testing::Types<
  ListGraph,
  SparseGraph,
  ArrayGraph
>;

TYPED_TEST_SUITE(SpanningTreeUndirectedGraphs, UndirectedGraphTypes);

TYPED_TEST(SpanningTreeAllGraphs, DFSSingleNode)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 1);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

TYPED_TEST(SpanningTreeAllGraphs, DFSTriangle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 3);
  EXPECT_EQ(tree.get_num_arcs(), 2);
}

TYPED_TEST(SpanningTreeAllGraphs, DFSEmptyGraphThrows)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  
  EXPECT_THROW(dfs(g, tree), std::domain_error);
}

TYPED_TEST(SpanningTreeAllGraphs, DFSNullptrThrows)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  
  EXPECT_THROW(dfs(g, nullptr, tree), std::invalid_argument);
}

TYPED_TEST(SpanningTreeAllGraphs, BFSSingleNode)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 1);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

TYPED_TEST(SpanningTreeAllGraphs, BFSTriangle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 3);
  EXPECT_EQ(tree.get_num_arcs(), 2);
}

TYPED_TEST(SpanningTreeAllGraphs, BFSEmptyGraphThrows)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  
  EXPECT_THROW(bfs(g, tree), std::domain_error);
}

TYPED_TEST(SpanningTreeAllGraphs, BFSNullptrThrows)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  
  EXPECT_THROW(bfs(g, nullptr, tree), std::invalid_argument);
}

TYPED_TEST(SpanningTreeUndirectedGraphs, DFSLongerChain)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  
  Find_Depth_First_Spanning_Tree<Graph> dfs;
  auto root = dfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4);
}

TYPED_TEST(SpanningTreeUndirectedGraphs, BFSLongerChain)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Graph & tree = this->tree;
  
  [[maybe_unused]] auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  
  Find_Breadth_First_Spanning_Tree<Graph> bfs;
  auto root = bfs(g, tree);
  
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4);
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
