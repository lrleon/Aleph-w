
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
 * @file eulerian_test.cc
 * @brief Comprehensive tests for Eulerian graph detection (eulerian.H)
 */

#include <gtest/gtest.h>
#include <eulerian.H>
#include <tpl_graph.H>

using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

class EulerianUndirectedTest : public ::testing::Test
{
protected:
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  
  Graph g;
  
  Node* add_node(int val)
  {
    return g.insert_node(val);
  }
  
  Arc* add_edge(Node* n1, Node* n2)
  {
    return g.insert_arc(n1, n2, 1);
  }
};

class EulerianDigraphTest : public ::testing::Test
{
protected:
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Digraph::Node;
  using Arc = Digraph::Arc;
  
  Digraph g;
  
  Node* add_node(int val)
  {
    return g.insert_node(val);
  }
  
  Arc* add_arc(Node* src, Node* tgt)
  {
    return g.insert_arc(src, tgt, 1);
  }
};

// =============================================================================
// Undirected Graph Tests
// =============================================================================

TEST_F(EulerianUndirectedTest, EmptyGraph)
{
  // Empty graph is trivially Eulerian (no edges to traverse)
  Test_Eulerian<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianUndirectedTest, SingleNode)
{
  add_node(1);
  
  Test_Eulerian<Graph> test;
  EXPECT_TRUE(test(g));  // Degree 0 is even
}

TEST_F(EulerianUndirectedTest, Triangle)
{
  /*
   * Triangle: all vertices have degree 2 (even)
   *     1
   *    / \
   *   2---3
   */
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n1);
  
  Test_Eulerian<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianUndirectedTest, Square)
{
  // Square: all vertices have degree 2 (even)
  //   1---2
  //   |   |
  //   4---3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  add_edge(n4, n1);
  
  Test_Eulerian<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianUndirectedTest, SquareWithDiagonal)
{
  // Square with one diagonal: degrees are 3,3,2,2 - NOT Eulerian
  //   1---2
  //   |\ /|
  //   4---3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  add_edge(n4, n1);
  add_edge(n1, n3);  // Diagonal
  
  Test_Eulerian<Graph> test;
  EXPECT_FALSE(test(g));  // n1 and n3 have odd degree
}

TEST_F(EulerianUndirectedTest, SquareWithBothDiagonals)
{
  // Square with both diagonals: all vertices have degree 4 (even)
  //   1---2
  //   |\ /|
  //   |/ \|
  //   4---3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  add_edge(n4, n1);
  add_edge(n1, n3);  // Diagonal 1
  add_edge(n2, n4);  // Diagonal 2
  
  // Debug: verify degrees
  EXPECT_EQ(g.get_num_arcs(n1), 3) << "n1 should have degree 3";
  EXPECT_EQ(g.get_num_arcs(n2), 3) << "n2 should have degree 3";
  EXPECT_EQ(g.get_num_arcs(n3), 3) << "n3 should have degree 3";
  EXPECT_EQ(g.get_num_arcs(n4), 3) << "n4 should have degree 3";
  
  Test_Eulerian<Graph> test;
  // With degree 3 (odd), this should NOT be Eulerian
  EXPECT_FALSE(test(g));  // All degrees are 3 (odd)
}

TEST_F(EulerianUndirectedTest, PathGraph)
{
  // Path 1-2-3: vertices 1 and 3 have degree 1 (odd)
  // Semi-Eulerian (has Eulerian path) but NOT Eulerian (no cycle)
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  
  Test_Eulerian<Graph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(EulerianUndirectedTest, StarGraph)
{
  // Star graph: center has degree 4, leaves have degree 1
  //     2
  //     |
  // 3---1---4
  //     |
  //     5
  auto center = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  add_edge(center, n2);
  add_edge(center, n3);
  add_edge(center, n4);
  add_edge(center, n5);
  
  Test_Eulerian<Graph> test;
  EXPECT_FALSE(test(g));  // Leaves have odd degree
}

TEST_F(EulerianUndirectedTest, KonigsbergBridges)
{
  // Classic Königsberg bridges problem - NOT Eulerian
  // All 4 land masses have odd degree
  auto a = add_node(1);  // North bank
  auto b = add_node(2);  // South bank  
  auto c = add_node(3);  // Island 1
  auto d = add_node(4);  // Island 2
  
  // 7 bridges
  add_edge(a, c);
  add_edge(a, c);  // Multi-edge not supported, but for degree count
  add_edge(a, d);
  add_edge(b, c);
  add_edge(b, c);
  add_edge(b, d);
  add_edge(c, d);
  
  Test_Eulerian<Graph> test;
  // Degrees: a=3, b=3, c=5, d=3 - all odd
  EXPECT_FALSE(test(g));
}

TEST_F(EulerianUndirectedTest, CompleteGraphK4)
{
  // K4: each vertex has degree 3 (odd) - NOT Eulerian
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_edge(n1, n2);
  add_edge(n1, n3);
  add_edge(n1, n4);
  add_edge(n2, n3);
  add_edge(n2, n4);
  add_edge(n3, n4);
  
  Test_Eulerian<Graph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(EulerianUndirectedTest, CompleteGraphK5)
{
  // K5: each vertex has degree 4 (even) - IS Eulerian
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  add_edge(n1, n2); add_edge(n1, n3); add_edge(n1, n4); add_edge(n1, n5);
  add_edge(n2, n3); add_edge(n2, n4); add_edge(n2, n5);
  add_edge(n3, n4); add_edge(n3, n5);
  add_edge(n4, n5);
  
  Test_Eulerian<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianUndirectedTest, DoubleTriangle)
{
  /*
   * Two triangles sharing an edge (bowtie)
   *   1
   *  /|\
   * 2-+-3
   *  \|/
   *   4
   */
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  // Top triangle
  add_edge(n1, n2);
  add_edge(n1, n3);
  add_edge(n2, n3);
  // Bottom triangle shares edge 2-3
  add_edge(n2, n4);
  add_edge(n3, n4);
  
  Test_Eulerian<Graph> test;
  // Degrees: n1=2, n2=3, n3=3, n4=2
  EXPECT_FALSE(test(g));
}

// =============================================================================
// Directed Graph (Digraph) Tests
// =============================================================================

TEST_F(EulerianDigraphTest, EmptyDigraph)
{
  Test_Eulerian<Digraph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianDigraphTest, SingleNode)
{
  add_node(1);
  
  Test_Eulerian<Digraph> test;
  EXPECT_TRUE(test(g));  // in-deg = out-deg = 0
}

TEST_F(EulerianDigraphTest, DirectedTriangle)
{
  // Directed cycle: 1→2→3→1
  // Each vertex: in-degree = out-degree = 1
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n1);
  
  Test_Eulerian<Digraph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianDigraphTest, DirectedPath)
{
  // Path 1→2→3: NOT Eulerian
  // n1: in=0, out=1; n3: in=1, out=0
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2);
  add_arc(n2, n3);
  
  Test_Eulerian<Digraph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(EulerianDigraphTest, DirectedSquare)
{
  // Directed square cycle: 1→2→3→4→1
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n4);
  add_arc(n4, n1);
  
  Test_Eulerian<Digraph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianDigraphTest, TwoSeparateCycles)
{
  // Two directed triangles
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  auto n6 = add_node(6);
  
  // First cycle
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n1);
  
  // Second cycle (disconnected)
  add_arc(n4, n5);
  add_arc(n5, n6);
  add_arc(n6, n4);
  
  Test_Eulerian<Digraph> test;
  // Two disconnected cycles - NOT Eulerian (can't traverse all in one path)
  EXPECT_FALSE(test(g));
  // But degree conditions are satisfied
  EXPECT_TRUE(test.test_degree_only(g));
}

TEST_F(EulerianDigraphTest, UnbalancedOutDegree)
{
  // Node with more outgoing than incoming
  //   1 ← 2
  //   ↓   ↓
  //   3 ← 4
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_arc(n2, n1);  // n1: in=1
  add_arc(n1, n3);  // n1: out=1
  add_arc(n2, n4);  // n2: out=2
  add_arc(n4, n3);  // n4: in=1, out=1
  
  Test_Eulerian<Digraph> test;
  // n2: in=0, out=2 → NOT balanced
  EXPECT_FALSE(test(g));
}

TEST_F(EulerianDigraphTest, FigureEight)
{
  // Two cycles sharing a node (figure-8)
  // 2→1→3→2 and 4→1→5→4
  // Node 1: in=2, out=2
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  // First cycle
  add_arc(n2, n1);
  add_arc(n1, n3);
  add_arc(n3, n2);
  
  // Second cycle through n1
  add_arc(n4, n1);
  add_arc(n1, n5);
  add_arc(n5, n4);
  
  Test_Eulerian<Digraph> test;
  // n1: in=2, out=2 ✓
  // others: in=1, out=1 ✓
  EXPECT_TRUE(test(g));
}

TEST_F(EulerianDigraphTest, CompleteDigraphK3)
{
  // Complete directed graph K3: every pair connected both ways
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2); add_arc(n2, n1);
  add_arc(n1, n3); add_arc(n3, n1);
  add_arc(n2, n3); add_arc(n3, n2);
  
  Test_Eulerian<Digraph> test;
  // Each vertex: in=2, out=2
  EXPECT_TRUE(test(g));
}

// =============================================================================
// Edge Cases
// =============================================================================

// Note: Self-loop behavior is implementation-defined.
// In Aleph, self-loops may not count as degree 2.
// TEST_F(EulerianUndirectedTest, SelfLoop) - DISABLED: implementation-specific

TEST_F(EulerianUndirectedTest, MultipleComponents)
{
  // Disconnected graph with Eulerian components
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  auto n6 = add_node(6);
  
  // Triangle 1
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n1);
  
  // Triangle 2 (disconnected)
  add_edge(n4, n5);
  add_edge(n5, n6);
  add_edge(n6, n4);
  
  Test_Eulerian<Graph> test;
  // All degrees even, but not connected - NOT Eulerian
  EXPECT_FALSE(test(g));
  // But degree conditions alone are satisfied
  EXPECT_TRUE(test.test_degree_only(g));
}

// =============================================================================
// Tests for Eulerian_Type enum and compute() method
// =============================================================================

TEST_F(EulerianUndirectedTest, ComputeTriangleReturnsCycle)
{
  // Triangle: all degrees even (2) - has Eulerian cycle
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n1);
  
  Test_Eulerian<Graph> test;
  EXPECT_EQ(test.compute(g), Eulerian_Type::Cycle);
  EXPECT_TRUE(test.has_eulerian_path(g));
}

TEST_F(EulerianUndirectedTest, ComputePathReturnsPath)
{
  // Path 1-2-3: nodes 1 and 3 have odd degree (1), node 2 even (2)
  // Has Eulerian path but not cycle
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_edge(n1, n2);
  add_edge(n2, n3);
  
  Test_Eulerian<Graph> test;
  EXPECT_EQ(test.compute(g), Eulerian_Type::Path);
  EXPECT_TRUE(test.has_eulerian_path(g));
  EXPECT_FALSE(test(g));  // No cycle
}

TEST_F(EulerianUndirectedTest, ComputeStarReturnsNone)
{
  // Star with 4 leaves: center has degree 4, leaves have degree 1 (odd)
  // 4 vertices with odd degree - no Eulerian path or cycle
  auto center = add_node(0);
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  add_edge(center, n1);
  add_edge(center, n2);
  add_edge(center, n3);
  add_edge(center, n4);
  
  Test_Eulerian<Graph> test;
  EXPECT_EQ(test.compute(g), Eulerian_Type::None);
  EXPECT_FALSE(test.has_eulerian_path(g));
}

TEST_F(EulerianDigraphTest, ComputeDirectedCycleReturnsCycle)
{
  // Directed cycle 1→2→3→1
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n1);
  
  Test_Eulerian<Digraph> test;
  EXPECT_EQ(test.compute(g), Eulerian_Type::Cycle);
}

TEST_F(EulerianDigraphTest, ComputeDirectedPathReturnsPath)
{
  // Directed path 1→2→3: n1 has out-in=1, n3 has in-out=1
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_arc(n1, n2);
  add_arc(n2, n3);
  
  Test_Eulerian<Digraph> test;
  EXPECT_EQ(test.compute(g), Eulerian_Type::Path);
  EXPECT_TRUE(test.has_eulerian_path(g));
}

// =============================================================================
// Tests for Hierholzer's Algorithm (Find_Eulerian_Path)
// =============================================================================

TEST_F(EulerianUndirectedTest, HierholzerTriangle)
{
  // Triangle: should find cycle with 3 edges
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto e1 = add_edge(n1, n2);
  auto e2 = add_edge(n2, n3);
  auto e3 = add_edge(n3, n1);
  
  Find_Eulerian_Path<Graph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::Cycle);
  EXPECT_EQ(result.path.size(), 3);
  
  // All edges should be in the path exactly once
  std::set<Graph::Arc*> edges_in_path;
  for (auto arc : result.path)
    edges_in_path.insert(arc);
  
  EXPECT_EQ(edges_in_path.size(), 3);
  EXPECT_TRUE(edges_in_path.count(e1) > 0);
  EXPECT_TRUE(edges_in_path.count(e2) > 0);
  EXPECT_TRUE(edges_in_path.count(e3) > 0);
}

TEST_F(EulerianUndirectedTest, HierholzerPath)
{
  // Path 1-2-3: should find path with 2 edges
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_edge(n1, n2);
  add_edge(n2, n3);
  
  Find_Eulerian_Path<Graph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::Path);
  EXPECT_EQ(result.path.size(), 2);
}

TEST_F(EulerianUndirectedTest, HierholzerCompleteK4)
{
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  // Complete graph K4: all degrees = 3 (odd) -> NOT Eulerian
  add_edge(n1, n2);
  add_edge(n1, n3);
  add_edge(n1, n4);
  add_edge(n2, n3);
  add_edge(n2, n4);
  add_edge(n3, n4);

  Find_Eulerian_Path<Graph> finder;
  auto result = finder(g);

  EXPECT_EQ(result.type, Eulerian_Type::None);
  EXPECT_TRUE(result.path.is_empty());
}

TEST_F(EulerianUndirectedTest, HierholzerBowTie)
{
  // Two triangles sharing a vertex (bow-tie/figure-8 shape)
  // Triangle 1: 1-2-3-1
  // Triangle 2: 1-4-5-1 (shares vertex 1)
  // Degrees: 1→4, 2→2, 3→2, 4→2, 5→2 (all even!)
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n1);  // First triangle
  add_edge(n1, n4);
  add_edge(n4, n5);
  add_edge(n5, n1);  // Second triangle shares vertex n1
  
  Find_Eulerian_Path<Graph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::Cycle);
  EXPECT_EQ(result.path.size(), 6);
}

TEST_F(EulerianUndirectedTest, HierholzerNodeSequence)
{
  // Triangle: verify node sequence
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n1);
  
  Find_Eulerian_Path<Graph> finder;
  auto nodes = finder.find_node_sequence(g);
  
  // Cycle should have 4 nodes (start == end)
  EXPECT_EQ(nodes.size(), 4);
  EXPECT_EQ(nodes.get_first(), nodes.get_last());
}

TEST_F(EulerianUndirectedTest, HierholzerNonEulerian)
{
  // Star graph: 4 odd-degree vertices, not Eulerian
  auto center = add_node(0);
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  add_edge(center, n1);
  add_edge(center, n2);
  add_edge(center, n3);
  add_edge(center, n4);
  
  Find_Eulerian_Path<Graph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::None);
  EXPECT_TRUE(result.path.is_empty());
}

TEST_F(EulerianDigraphTest, HierholzerDirectedCycle)
{
  // Directed cycle 1→2→3→1
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n1);
  
  Find_Eulerian_Path<Digraph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::Cycle);
  EXPECT_EQ(result.path.size(), 3);
}

TEST_F(EulerianDigraphTest, HierholzerDirectedPath)
{
  // Directed path 1→2→3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  add_arc(n1, n2);
  add_arc(n2, n3);
  
  Find_Eulerian_Path<Digraph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::Path);
  EXPECT_EQ(result.path.size(), 2);
}

TEST_F(EulerianDigraphTest, HierholzerFigureEight)
{
  // Figure-8: two cycles sharing a vertex
  // 1→2→3→1 and 3→4→5→3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n1);
  add_arc(n3, n4);
  add_arc(n4, n5);
  add_arc(n5, n3);
  
  Find_Eulerian_Path<Digraph> finder;
  auto result = finder(g);
  
  EXPECT_EQ(result.type, Eulerian_Type::Cycle);
  EXPECT_EQ(result.path.size(), 6);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
