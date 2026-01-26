
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file tpl_test_cycle.cc
 * @brief Tests for Tpl Test Cycle
 */

/**
 * Comprehensive test suite for tpl_test_cycle.H
 * 
 * Tests all aspects of the Test_For_Cycle algorithm including:
 * - Basic cycle detection
 * - Edge cases (empty graphs, single nodes, etc.)
 * - Directed vs undirected graphs
 * - Self-loops
 * - Multiple cycles
 * - Long cycles
 * - Arc filters
 * - Error handling
 * - All graph types: List_Graph, List_SGraph, Array_Graph (directed/undirected)
 */

#include <gtest/gtest.h>
#include <tpl_test_cycle.H>
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
// Test Fixtures for Primary Graph Types
//==============================================================================

class TestForCycleDirected : public ::testing::Test
{
protected:
  using Graph = ListDigraph;
  Graph g;
};

class TestForCycleUndirected : public ::testing::Test
{
protected:
  using Graph = ListGraph;
  Graph g;
};

// Custom arc filter for testing
template <class GT>
struct Even_Arc_Filter
{
  bool operator () (typename GT::Arc * arc) const
  {
    return arc->get_info() % 2 == 0;
  }
};

//==============================================================================
// Basic Functionality Tests
//==============================================================================

TEST_F(TestForCycleDirected, EmptyGraph)
{
  Test_For_Cycle<Graph> cycle_test;
  auto n1 = g.insert_node(1);
  
  // Single node with no arcs has no cycle
  EXPECT_FALSE(cycle_test(g, n1));
}

TEST_F(TestForCycleDirected, NoCycleLinearChain)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  
  // Linear chain has no cycles
  EXPECT_FALSE(cycle_test(g, n1));
  EXPECT_FALSE(cycle_test(g, n2));
  EXPECT_FALSE(cycle_test(g, n3));
  EXPECT_FALSE(cycle_test(g, n4));
}

TEST_F(TestForCycleDirected, SimpleTriangleCycle)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1); // Completes the triangle
  
  // All nodes in the cycle should detect it
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
}

TEST_F(TestForCycleDirected, SelfLoop)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  g.insert_arc(n1, n1); // Self-loop
  
  EXPECT_TRUE(cycle_test(g, n1));
}

TEST_F(TestForCycleDirected, TwoNodeCycle)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1); // Back edge creates cycle
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
}

TEST_F(TestForCycleDirected, LongCycle)
{
  Test_For_Cycle<Graph> cycle_test;
  
  const int N = 100;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  // Create a long cycle: 0 -> 1 -> 2 -> ... -> 99 -> 0
  for (int i = 0; i < N; ++i)
    g.insert_arc(nodes(i), nodes((i + 1) % N));
  
  // All nodes should detect the cycle
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(cycle_test(g, nodes(i)));
}

//==============================================================================
// Single-Source Cycle Detection Tests
//==============================================================================

TEST_F(TestForCycleDirected, CycleNotReachableFromSource)
{
  Test_For_Cycle<Graph> cycle_test;
  
  // Graph structure: n1 -> n2 -> n3 -> n4 -> n2 (cycle not involving n1)
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n2); // Cycle: 2 -> 3 -> 4 -> 2
  
  // n1 is not part of the cycle, but can reach it
  EXPECT_FALSE(cycle_test(g, n1));
  
  // Nodes in the cycle should detect it
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
  EXPECT_TRUE(cycle_test(g, n4));
}

TEST_F(TestForCycleDirected, IsolatedNodeNoCycle)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  // Create cycle between n1 and n2
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  // n3 is isolated, should not detect cycle
  EXPECT_FALSE(cycle_test(g, n3));
}

TEST_F(TestForCycleDirected, PartiallyReachableGraph)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // n1 -> n2, but n3 -> n4 -> n3 (separate cycle)
  g.insert_arc(n1, n2);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n3);
  
  EXPECT_FALSE(cycle_test(g, n1));
  EXPECT_FALSE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
  EXPECT_TRUE(cycle_test(g, n4));
}

//==============================================================================
// Complex Graph Structure Tests
//==============================================================================

TEST_F(TestForCycleDirected, MultipleCycles)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  // Cycle 1: n1 -> n2 -> n1
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  // Cycle 2: n3 -> n4 -> n5 -> n3
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  g.insert_arc(n5, n3);
  
  // Connection from n2 to n3
  g.insert_arc(n2, n3);
  
  // n1 should detect its own cycle
  EXPECT_TRUE(cycle_test(g, n1));
  
  // n3, n4, n5 should detect their cycle
  EXPECT_TRUE(cycle_test(g, n3));
  EXPECT_TRUE(cycle_test(g, n4));
  EXPECT_TRUE(cycle_test(g, n5));
}

TEST_F(TestForCycleDirected, DiamondStructure)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Diamond: n1 -> n2, n1 -> n3, n2 -> n4, n3 -> n4
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n3, n4);
  
  // No cycle in DAG diamond
  EXPECT_FALSE(cycle_test(g, n1));
  
  // Add back edge to create cycle
  g.insert_arc(n4, n1);
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
  EXPECT_TRUE(cycle_test(g, n4));
}

TEST_F(TestForCycleDirected, NestedCycles)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Outer cycle: 1 -> 2 -> 3 -> 4 -> 1
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n1);
  
  // Inner shortcut: 2 -> 4 (creates additional cycle 2 -> 3 -> 4 -> 2)
  g.insert_arc(n2, n4);
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
  EXPECT_TRUE(cycle_test(g, n4));
}

//==============================================================================
// Undirected Graph Tests
//==============================================================================

TEST_F(TestForCycleUndirected, UndirectedTriangle)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);
  
  // Undirected graph: all edges create bidirectional paths
  // This will detect trivial 2-cycles (as documented in warnings)
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
}

TEST_F(TestForCycleUndirected, UndirectedTree)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Tree structure (no cycles)
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n1, n4);
  
  // Trees have no cycles, even in undirected graphs
  // (The algorithm marks arcs as visited, preventing trivial back-and-forth)
  EXPECT_FALSE(cycle_test(g, n1));
  
  // Add edge to create actual cycle
  g.insert_arc(n2, n3);
  EXPECT_TRUE(cycle_test(g, n1));
}

//==============================================================================
// Arc Filter Tests
//==============================================================================

TEST_F(TestForCycleDirected, ArcFilterBlocksCycle)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  // Create triangle with odd arc values
  auto a1 = g.insert_arc(n1, n2, 1); // odd
  [[maybe_unused]] auto a2 = g.insert_arc(n2, n3, 2); // even
  auto a3 = g.insert_arc(n3, n1, 3); // odd
  
  // With even filter, only a2 is visible -> no cycle
  Test_For_Cycle<Graph, Even_Arc_Filter<Graph>> cycle_test_even;
  EXPECT_FALSE(cycle_test_even(g, n1));
  EXPECT_FALSE(cycle_test_even(g, n2));
  
  // Make all arcs even -> cycle becomes visible
  a1->get_info() = 2;
  a3->get_info() = 4;
  EXPECT_TRUE(cycle_test_even(g, n1));
}

TEST_F(TestForCycleDirected, ArcFilterPartialCycle)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Create path: 1 -> 2 -> 3 -> 4 -> 1
  g.insert_arc(n1, n2, 2); // even
  g.insert_arc(n2, n3, 2); // even
  g.insert_arc(n3, n4, 1); // odd
  g.insert_arc(n4, n1, 2); // even
  
  // With even filter, path is broken at n3 -> n4
  Test_For_Cycle<Graph, Even_Arc_Filter<Graph>> cycle_test_even;
  EXPECT_FALSE(cycle_test_even(g, n1));
}

//==============================================================================
// Error Handling Tests
//==============================================================================

TEST_F(TestForCycleDirected, NullptrThrows)
{
  Test_For_Cycle<Graph> cycle_test;
  
  EXPECT_THROW(cycle_test(g, nullptr), std::invalid_argument);
}

TEST_F(TestForCycleDirected, RepeatedCalls)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  // Multiple calls should give consistent results
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n1));
}

//==============================================================================
// Stress Tests
//==============================================================================

TEST_F(TestForCycleDirected, LargeAcyclicDAG)
{
  Test_For_Cycle<Graph> cycle_test;
  
  const int N = 1000;
  DynArray<Graph::Node*> nodes;
  
  // Create large DAG with no cycles
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  // Connect each node to next few nodes (DAG)
  for (int i = 0; i < N - 1; ++i)
    for (int j = i + 1; j < std::min(i + 5, N); ++j)
      g.insert_arc(nodes(i), nodes(j));
  
  // No cycles should be detected
  EXPECT_FALSE(cycle_test(g, nodes(0)));
  EXPECT_FALSE(cycle_test(g, nodes(N/2)));
  EXPECT_FALSE(cycle_test(g, nodes(N-1)));
}

TEST_F(TestForCycleDirected, DenseGraphWithCycle)
{
  Test_For_Cycle<Graph> cycle_test;
  
  const int N = 50;
  DynArray<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.append(g.insert_node(i));
  
  // Create dense graph with many forward edges
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes(i), nodes(j));
  
  // Add single back edge to create cycle
  g.insert_arc(nodes(N-1), nodes(0));
  
  // Cycle should be detected from all nodes
  EXPECT_TRUE(cycle_test(g, nodes(0)));
  EXPECT_TRUE(cycle_test(g, nodes(N/2)));
  EXPECT_TRUE(cycle_test(g, nodes(N-1)));
}

//==============================================================================
// Const Correctness Tests
//==============================================================================

TEST_F(TestForCycleDirected, ConstGraph)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  const Graph & const_g = g;
  Test_For_Cycle<Graph> cycle_test;
  
  // Should compile and work with const graph
  EXPECT_TRUE(cycle_test(const_g, n1));
}

TEST_F(TestForCycleDirected, ConstCycleTest)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  const Test_For_Cycle<Graph> cycle_test;
  
  // Should compile and work with const tester
  EXPECT_TRUE(cycle_test(g, n1));
}

//==============================================================================
// Special Cases
//==============================================================================

TEST_F(TestForCycleDirected, GraphWithMultipleSelfLoops)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n1); // Self-loop at n1
  g.insert_arc(n2, n2); // Self-loop at n2
  g.insert_arc(n1, n2); // Connection
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
}

TEST_F(TestForCycleDirected, ParallelArcs)
{
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  // Multiple arcs between same nodes
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  g.insert_arc(n2, n1);
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
}

TEST_F(TestForCycleDirected, DisconnectedComponents)
{
  Test_For_Cycle<Graph> cycle_test;
  
  // Component 1: cycle
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  // Component 2: no cycle
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n3, n4);
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_FALSE(cycle_test(g, n3));
  EXPECT_FALSE(cycle_test(g, n4));
}

//==============================================================================
// Typed Tests for All Graph Types
//==============================================================================

// Helper template to run basic cycle tests on any graph type
template <typename GraphType>
class TestForCycleAllGraphs : public ::testing::Test
{
protected:
  GraphType g;
};

// Define the list of graph types to test
using GraphTypes = ::testing::Types<
  ListGraph,
  ListDigraph,
  SparseGraph,
  SparseDigraph,
  ArrayGraph,
  ArrayDigraph
>;

TYPED_TEST_SUITE(TestForCycleAllGraphs, GraphTypes);

TYPED_TEST(TestForCycleAllGraphs, BasicNoCycle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  
  EXPECT_FALSE(cycle_test(g, n1));
  EXPECT_FALSE(cycle_test(g, n2));
  EXPECT_FALSE(cycle_test(g, n3));
}

TYPED_TEST(TestForCycleAllGraphs, BasicWithCycle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);  // Create cycle
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
  EXPECT_TRUE(cycle_test(g, n3));
}

TYPED_TEST(TestForCycleAllGraphs, SelfLoop)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  g.insert_arc(n1, n1);  // Self-loop
  
  EXPECT_TRUE(cycle_test(g, n1));
}

TYPED_TEST(TestForCycleAllGraphs, SingleNode)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  
  EXPECT_FALSE(cycle_test(g, n1));
}

TYPED_TEST(TestForCycleAllGraphs, NullptrThrows)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  EXPECT_THROW(cycle_test(g, nullptr), std::invalid_argument);
}

TYPED_TEST(TestForCycleAllGraphs, TwoNodeCycle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n1);
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n2));
}

TYPED_TEST(TestForCycleAllGraphs, LongerChainWithCycle)
{
  using Graph = TypeParam;
  Graph & g = this->g;
  Test_For_Cycle<Graph> cycle_test;
  
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n4);
  g.insert_arc(n4, n5);
  g.insert_arc(n5, n1);  // Complete cycle
  
  EXPECT_TRUE(cycle_test(g, n1));
  EXPECT_TRUE(cycle_test(g, n3));
  EXPECT_TRUE(cycle_test(g, n5));
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
