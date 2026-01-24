
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
 * @file latex_floyd_test.cc
 * @brief Comprehensive tests for Floyd-Warshall algorithm (latex_floyd.H)
 *
 * Tests cover:
 * - floyd_all_shortest_paths() algorithm correctness
 * - find_min_path() path reconstruction
 * - floyd_all_shortest_paths_latex() LaTeX output format
 * - Edge cases (single node, disconnected graphs, etc.)
 */

#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <limits>
#include <cmath>

#include <tpl_graph.H>
#include <tpl_matgraph.H>
#include <latex_floyd.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// ============================================================================
// Arc type with distance for Floyd-Warshall
// ============================================================================

/// Arc info type with required Distance_Type and constants
struct DistanceArc
{
  using Distance_Type = double;
  
  static constexpr Distance_Type Max_Distance = numeric_limits<double>::infinity();
  static constexpr Distance_Type Zero_Distance = 0.0;
  
  Distance_Type distance;
  
  DistanceArc() : distance(0) {}
  DistanceArc(Distance_Type d) : distance(d) {}
  
  Distance_Type get_distance() const { return distance; }
  
  operator Distance_Type() const { return distance; }
};

/// Integer distance arc type
struct IntDistanceArc
{
  using Distance_Type = int;
  
  static constexpr Distance_Type Max_Distance = numeric_limits<int>::max() / 2; // Avoid overflow
  static constexpr Distance_Type Zero_Distance = 0;
  
  Distance_Type distance;
  
  IntDistanceArc() : distance(0) {}
  IntDistanceArc(Distance_Type d) : distance(d) {}
  
  Distance_Type get_distance() const { return distance; }
  
  operator Distance_Type() const { return distance; }
};

// ============================================================================
// Test Fixtures
// ============================================================================

/// Test fixture with a simple weighted graph
class FloydSimpleGraphTest : public Test
{
protected:
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<DistanceArc>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  using Dist = DistanceArc::Distance_Type;

  Graph g;
  Node* n0;
  Node* n1;
  Node* n2;
  Node* n3;

  void SetUp() override
  {
    // Create a simple graph:
    //     1
    // 0 -----> 1
    // |        |
    // |4       |2
    // v        v
    // 3 <----- 2
    //     1
    //
    // Also: direct edge 0->2 with weight 5 (longer than 0->1->2 = 3)
    
    n0 = g.insert_node(0);
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    
    g.insert_arc(n0, n1, DistanceArc(1.0));  // 0 -> 1: weight 1
    g.insert_arc(n1, n2, DistanceArc(2.0));  // 1 -> 2: weight 2
    g.insert_arc(n2, n3, DistanceArc(1.0));  // 2 -> 3: weight 1
    g.insert_arc(n0, n3, DistanceArc(4.0));  // 0 -> 3: weight 4 (longer than 0->1->2->3 = 4)
    g.insert_arc(n0, n2, DistanceArc(5.0));  // 0 -> 2: weight 5 (longer than 0->1->2 = 3)
  }
  
  // Helper to get node index by node pointer using matrix iteration
  long get_index(Ady_Mat<Graph, Dist>& mat, Node* node) const
  {
    long n = mat.get_num_nodes();
    for (long i = 0; i < n; ++i)
      if (mat(i) == node)
        return i;
    return -1;
  }
};

/// Test fixture with integer weights
class FloydIntGraphTest : public Test
{
protected:
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  using Dist = IntDistanceArc::Distance_Type;

  Graph g;
  vector<Node*> nodes;

  void SetUp() override
  {
    // Create a 4-node graph with known shortest paths
    for (int i = 0; i < 4; ++i)
      nodes.push_back(g.insert_node(i));
    
    // Edges: forms a square with diagonal
    //   0 --2-- 1
    //   |     / |
    //   3   1   4
    //   | /     |
    //   2 --5-- 3
    
    g.insert_arc(nodes[0], nodes[1], IntDistanceArc(2));
    g.insert_arc(nodes[1], nodes[0], IntDistanceArc(2));
    g.insert_arc(nodes[0], nodes[2], IntDistanceArc(3));
    g.insert_arc(nodes[2], nodes[0], IntDistanceArc(3));
    g.insert_arc(nodes[1], nodes[2], IntDistanceArc(1)); // Diagonal shortcut
    g.insert_arc(nodes[2], nodes[1], IntDistanceArc(1));
    g.insert_arc(nodes[1], nodes[3], IntDistanceArc(4));
    g.insert_arc(nodes[3], nodes[1], IntDistanceArc(4));
    g.insert_arc(nodes[2], nodes[3], IntDistanceArc(5));
    g.insert_arc(nodes[3], nodes[2], IntDistanceArc(5));
  }
};

// ============================================================================
// floyd_all_shortest_paths() Tests
// ============================================================================

TEST_F(FloydSimpleGraphTest, DiagonalIsZero)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Diagonal should be zero
  const long n = g.get_num_nodes();
  for (long i = 0; i < n; ++i)
    EXPECT_DOUBLE_EQ(dist(i, i), 0.0) << "Diagonal at " << i << " should be 0";
}

TEST_F(FloydSimpleGraphTest, DirectEdgeDistance)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Get indices for n0 and n1
  long idx0 = get_index(dist, n0);
  long idx1 = get_index(dist, n1);
  
  // Direct edge 0->1 has weight 1
  EXPECT_DOUBLE_EQ(dist(idx0, idx1), 1.0);
}

TEST_F(FloydSimpleGraphTest, ShortestPathOverMultipleEdges)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  long idx0 = get_index(dist, n0);
  long idx2 = get_index(dist, n2);
  
  // 0->2: direct is 5, but 0->1->2 = 1+2 = 3
  EXPECT_DOUBLE_EQ(dist(idx0, idx2), 3.0);
}

TEST_F(FloydSimpleGraphTest, ShortestPathToLastNode)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  long idx0 = get_index(dist, n0);
  long idx3 = get_index(dist, n3);
  
  // 0->3: direct is 4, path 0->1->2->3 = 1+2+1 = 4 (same)
  EXPECT_DOUBLE_EQ(dist(idx0, idx3), 4.0);
}

TEST_F(FloydSimpleGraphTest, UnreachableNodesHaveInfinity)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  long idx1 = get_index(dist, n1);
  long idx0 = get_index(dist, n0);
  
  // Node 0 is not reachable from node 1 (directed graph)
  EXPECT_TRUE(isinf(dist(idx1, idx0)));
}

TEST_F(FloydIntGraphTest, SymmetricGraph)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  const long n = g.get_num_nodes();
  
  // For this undirected-like graph, dist(i,j) == dist(j,i)
  for (long i = 0; i < n; ++i)
    for (long j = 0; j < n; ++j)
      EXPECT_EQ(dist(i, j), dist(j, i)) 
        << "Distance should be symmetric for i=" << i << ", j=" << j;
}

TEST_F(FloydIntGraphTest, ShortcutUsed)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Find indices - nodes[0] should be at some index
  long idx0 = -1, idx3 = -1;
  const long n = g.get_num_nodes();
  for (long i = 0; i < n; ++i) {
    if (dist(i)->get_info() == 0) idx0 = i;
    if (dist(i)->get_info() == 3) idx3 = i;
  }
  
  // 0->3: best path is 0->1->3 = 2+4 = 6
  EXPECT_EQ(dist(idx0, idx3), 6);
}

// ============================================================================
// find_min_path() Tests
// ============================================================================

TEST_F(FloydSimpleGraphTest, PathReconstructionSameNode)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  Path<Graph> p(g);
  long idx0 = get_index(dist, n0);
  
  find_min_path(path, idx0, idx0, p);
  
  // Path from node to itself should just contain the source
  EXPECT_EQ(p.size(), 1u);
}

TEST_F(FloydSimpleGraphTest, PathReconstructionDirectEdge)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  Path<Graph> p(g);
  long idx0 = get_index(dist, n0);
  long idx1 = get_index(dist, n1);
  
  find_min_path(path, idx0, idx1, p);
  
  // Path 0->1 should have 2 nodes
  EXPECT_EQ(p.size(), 2u);
}

TEST_F(FloydSimpleGraphTest, PathReconstructionMultipleEdges)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  Path<Graph> p(g);
  long idx0 = get_index(dist, n0);
  long idx2 = get_index(dist, n2);
  
  find_min_path(path, idx0, idx2, p);
  
  // Shortest path 0->2 goes through 1, so path is 0->1->2 (3 nodes)
  EXPECT_EQ(p.size(), 3u);
}

// ============================================================================
// floyd_all_shortest_paths_latex() Tests
// ============================================================================

// Formatters for LaTeX output - must match mat_latex.H signatures

/// Index formatter: takes matrix and index, returns string
template <class Mat>
struct TestIndexFormatter
{
  string operator()(Mat& mat, int i) const
  {
    return to_string(mat(static_cast<long>(i))->get_info());
  }
};

/// Path formatter: takes matrix, i, j indices, returns formatted entry
template <class Mat>
struct TestPathFormatter
{
  string operator()(Mat& mat, int i, int j) const
  {
    long val = mat(static_cast<long>(i), static_cast<long>(j));
    return to_string(val);
  }
};

/// Distance formatter: takes matrix, i, j indices, returns formatted entry
template <class Mat>
struct TestDistFormatter
{
  string operator()(Mat& mat, int i, int j) const
  {
    auto val = mat(static_cast<long>(i), static_cast<long>(j));
    if (isinf(static_cast<double>(val)))
      return "\\infty";
    return to_string(static_cast<int>(val));
  }
};

TEST_F(FloydIntGraphTest, LatexOutputContainsBeginFigure)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  // Create temp file
  string filename = "/tmp/floyd_test_latex.tex";
  ofstream output(filename);
  
  floyd_all_shortest_paths_latex<Graph, 
    TestIndexFormatter, TestPathFormatter, TestDistFormatter>
    (g, dist, path, output);
  
  output.close();
  
  // Read back and check
  ifstream input(filename);
  stringstream ss;
  ss << input.rdbuf();
  string content = ss.str();
  
  EXPECT_NE(content.find("\\begin{figure}"), string::npos);
  EXPECT_NE(content.find("\\end{figure}"), string::npos);
}

TEST_F(FloydIntGraphTest, LatexOutputContainsMatrices)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  string filename = "/tmp/floyd_test_latex2.tex";
  ofstream output(filename);
  
  floyd_all_shortest_paths_latex<Graph,
    TestIndexFormatter, TestPathFormatter, TestDistFormatter>
    (g, dist, path, output);
  
  output.close();
  
  ifstream input(filename);
  stringstream ss;
  ss << input.rdbuf();
  string content = ss.str();
  
  // Should contain D_0, P_0, D_1, P_1, etc.
  EXPECT_NE(content.find("D_0"), string::npos);
  EXPECT_NE(content.find("P_0"), string::npos);
  EXPECT_NE(content.find("D_1"), string::npos);
  EXPECT_NE(content.find("P_1"), string::npos);
}

TEST_F(FloydIntGraphTest, LatexOutputHasCorrectNumberOfIterations)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  string filename = "/tmp/floyd_test_latex3.tex";
  ofstream output(filename);
  
  floyd_all_shortest_paths_latex<Graph,
    TestIndexFormatter, TestPathFormatter, TestDistFormatter>
    (g, dist, path, output);
  
  output.close();
  
  ifstream input(filename);
  stringstream ss;
  ss << input.rdbuf();
  string content = ss.str();
  
  // For n=4 nodes, we should have D_0 through D_4 (5 matrices)
  // Actually D_0 initial, then D_1 through D_n
  const long n = g.get_num_nodes();
  for (long i = 0; i <= n; ++i) {
    string dmat = "D_" + to_string(i);
    EXPECT_NE(content.find(dmat), string::npos) << "Missing " << dmat;
  }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(SingleNodeTest, FloydOnSingleNode)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  Graph g;
  g.insert_node(0);
  
  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Single node: distance to itself is 0
  EXPECT_EQ(dist(0L, 0L), 0);
}

TEST(TwoNodeTest, FloydOnTwoNodes)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  Graph g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, IntDistanceArc(5));
  
  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Find indices for nodes
  long idx0 = -1, idx1 = -1;
  for (long i = 0; i < 2; ++i) {
    if (dist(i)->get_info() == 0) idx0 = i;
    if (dist(i)->get_info() == 1) idx1 = i;
  }
  
  EXPECT_EQ(dist(idx0, idx0), 0);
  EXPECT_EQ(dist(idx1, idx1), 0);
  EXPECT_EQ(dist(idx0, idx1), 5);
  EXPECT_EQ(dist(idx1, idx0), IntDistanceArc::Max_Distance); // Not reachable
}

TEST(DisconnectedGraphTest, FloydOnDisconnectedGraph)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  Graph g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  auto* n3 = g.insert_node(3);
  
  // Two disconnected components: {0,1} and {2,3}
  g.insert_arc(n0, n1, IntDistanceArc(1));
  g.insert_arc(n1, n0, IntDistanceArc(1));
  g.insert_arc(n2, n3, IntDistanceArc(2));
  g.insert_arc(n3, n2, IntDistanceArc(2));
  
  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);

  // Map node info -> matrix index (Ady_Mat sorts nodes by pointer value)
  vector<long> idx(4, -1);
  for (long i = 0; i < 4; ++i)
    idx[dist(i)->get_info()] = i;
  
  // Within component 1
  EXPECT_EQ(dist(idx[0], idx[1]), 1);
  EXPECT_EQ(dist(idx[1], idx[0]), 1);
  
  // Within component 2
  EXPECT_EQ(dist(idx[2], idx[3]), 2);
  EXPECT_EQ(dist(idx[3], idx[2]), 2);
  
  // Between components (unreachable)
  EXPECT_EQ(dist(idx[0], idx[2]), IntDistanceArc::Max_Distance);
  EXPECT_EQ(dist(idx[1], idx[3]), IntDistanceArc::Max_Distance);
}

TEST(DisconnectedGraphTest, UnreachableStaysInfinityWithNegativeEdges)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;

  Graph g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  auto* n3 = g.insert_node(3);

  // Component 1: negative edge but no negative cycle
  g.insert_arc(n0, n1, IntDistanceArc(-5));
  g.insert_arc(n1, n0, IntDistanceArc(6));

  // Component 2: separate disconnected component
  g.insert_arc(n2, n3, IntDistanceArc(2));
  g.insert_arc(n3, n2, IntDistanceArc(2));

  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);

  floyd_all_shortest_paths(g, dist, path);

  // Map node info -> matrix index
  vector<long> idx(4, -1);
  for (long i = 0; i < 4; ++i)
    idx[dist(i)->get_info()] = i;

  // Within component 1
  EXPECT_EQ(dist(idx[0], idx[1]), -5);
  EXPECT_EQ(dist(idx[1], idx[0]), 6);

  // Between components (unreachable) must remain infinity sentinel even with negative weights
  EXPECT_EQ(dist(idx[0], idx[2]), IntDistanceArc::Max_Distance);
  EXPECT_EQ(dist(idx[1], idx[3]), IntDistanceArc::Max_Distance);
  EXPECT_EQ(dist(idx[2], idx[0]), IntDistanceArc::Max_Distance);
  EXPECT_EQ(dist(idx[3], idx[1]), IntDistanceArc::Max_Distance);
}

TEST(CompleteGraphTest, FloydOnCompleteGraph)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  const int N = 5;
  Graph g;
  vector<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Complete graph with all edges having weight 1
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (i != j)
        g.insert_arc(nodes[i], nodes[j], IntDistanceArc(1));
  
  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // In complete graph with unit weights, all distances should be 0 or 1
  for (long i = 0; i < N; ++i) {
    for (long j = 0; j < N; ++j) {
      if (i == j)
        EXPECT_EQ(dist(i, j), 0);
      else
        EXPECT_EQ(dist(i, j), 1);
    }
  }
}

// ============================================================================
// Custom Compare and Plus Tests
// ============================================================================

// Max-plus semiring: find widest paths (max of min edge weights)
struct MaxCompare
{
  bool operator()(int a, int b) const { return a > b; }
};

struct MinPlus
{
  int operator()(int a, int b) const { return min(a, b); }
};

TEST(CustomSemiringTest, MaxMinSemiring)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  Graph g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  
  // Path 0->2 direct: capacity 2
  // Path 0->1->2: min(5, 3) = 3 (better!)
  g.insert_arc(n0, n1, IntDistanceArc(5));
  g.insert_arc(n1, n2, IntDistanceArc(3));
  g.insert_arc(n0, n2, IntDistanceArc(2));
  
  Ady_Mat<Graph, int> cap(g);
  Ady_Mat<Graph, long> path(g);
  
  // Note: This would need a different initialization for max-min
  // This test just verifies custom functors compile and run
  floyd_all_shortest_paths<Graph, MaxCompare, MinPlus>(g, cap, path);
  
  // The result depends on initialization which assumes min-plus
  // So we just verify it runs without crashing
  SUCCEED();
}

// ============================================================================
// Stress Test
// ============================================================================

TEST(StressTest, FloydOnLargerGraph)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  const int N = 20;
  Graph g;
  vector<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create a chain with some shortcuts
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], IntDistanceArc(1));
  
  // Add some shortcuts (longer, so they won't be used)
  for (int i = 0; i < N - 2; i += 2)
    g.insert_arc(nodes[i], nodes[i + 2], IntDistanceArc(3)); // Longer than 2 hops
  
  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Build index mapping from node info to matrix index
  vector<long> idx(N, -1);
  for (long m = 0; m < N; ++m) {
    int info = dist(m)->get_info();
    idx[info] = m;
  }
  
  // Check that chain distances are correct using proper indices
  for (int i = 0; i < N; ++i)
    for (int j = i; j < N; ++j)
      EXPECT_EQ(dist(idx[i], idx[j]), j - i) << "Distance from " << i << " to " << j;
}

// ============================================================================
// Matrix Properties Tests
// ============================================================================

TEST_F(FloydIntGraphTest, PathMatrixPointsToNextHop)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  const long n = g.get_num_nodes();
  
  // For reachable nodes, path(i,j) should point to next hop or j itself
  for (long i = 0; i < n; ++i) {
    for (long j = 0; j < n; ++j) {
      if (i == j) {
        EXPECT_EQ(path(i, j), j) << "Diagonal should point to self";
      } else if (dist(i, j) < IntDistanceArc::Max_Distance) {
        long next = path(i, j);
        EXPECT_GE(next, 0L);
        EXPECT_LT(next, n);
      }
    }
  }
}

TEST_F(FloydSimpleGraphTest, DistanceMatrixTriangleInequality)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  const long n = g.get_num_nodes();
  
  // Triangle inequality: dist(i,j) <= dist(i,k) + dist(k,j)
  for (long i = 0; i < n; ++i) {
    for (long j = 0; j < n; ++j) {
      for (long k = 0; k < n; ++k) {
        if (!isinf(dist(i, k)) && !isinf(dist(k, j))) {
          EXPECT_LE(dist(i, j), dist(i, k) + dist(k, j))
            << "Triangle inequality violated for i=" << i << ", j=" << j << ", k=" << k;
        }
      }
    }
  }
}

// ============================================================================
// Initialize_Dist Tests (implicitly tested via floyd_all_shortest_paths)
// ============================================================================

TEST_F(FloydIntGraphTest, InitializationSetsEdgeWeightsCorrectly)
{
  Ady_Mat<Graph, Dist> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  // Run Floyd-Warshall which internally uses Initialize_Dist
  floyd_all_shortest_paths(g, dist, path);
  
  // After running, we should have valid distances
  const long n = g.get_num_nodes();
  
  // Check diagonal is zero (set by initialization)
  for (long i = 0; i < n; ++i)
    EXPECT_EQ(dist(i, i), 0);
  
  // Check that at least some edges have finite weights
  bool found_finite = false;
  for (long i = 0; i < n; ++i)
    for (long j = 0; j < n; ++j)
      if (i != j && dist(i, j) < IntDistanceArc::Max_Distance)
        found_finite = true;
  
  EXPECT_TRUE(found_finite);
}

// ============================================================================
// Negative Weights (Note: Floyd-Warshall handles them, but not negative cycles)
// ============================================================================

TEST(NegativeWeightsTest, FloydWithNegativeWeights)
{
  // Use double to allow negative values
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<DistanceArc>>;
  
  Graph g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  
  // 0 --1--> 1 ---(-3)---> 2
  // 0 --2--> 2 (direct, but longer than 0->1->2 = 1 + (-3) = -2)
  g.insert_arc(n0, n1, DistanceArc(1.0));
  g.insert_arc(n1, n2, DistanceArc(-3.0)); // Negative weight
  g.insert_arc(n0, n2, DistanceArc(2.0));
  
  Ady_Mat<Graph, double> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Find indices
  long idx0 = -1, idx2 = -1;
  for (long i = 0; i < 3; ++i) {
    if (dist(i)->get_info() == 0) idx0 = i;
    if (dist(i)->get_info() == 2) idx2 = i;
  }
  
  // Shortest 0->2 should be -2 (through 1)
  EXPECT_DOUBLE_EQ(dist(idx0, idx2), -2.0);
}

// ============================================================================
// Large Dense Graph Test
// ============================================================================

TEST(DenseGraphTest, FloydOnDenseGraph)
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<IntDistanceArc>>;
  
  const int N = 10;
  Graph g;
  vector<Graph::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Dense graph: edge from i to j with weight (i+j+1) mod N + 1
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (i != j)
        g.insert_arc(nodes[i], nodes[j], IntDistanceArc((i + j + 1) % N + 1));
  
  Ady_Mat<Graph, int> dist(g);
  Ady_Mat<Graph, long> path(g);
  
  floyd_all_shortest_paths(g, dist, path);
  
  // Verify diagonal is zero
  for (long i = 0; i < N; ++i)
    EXPECT_EQ(dist(i, i), 0);
  
  // Verify all pairs are reachable
  for (long i = 0; i < N; ++i)
    for (long j = 0; j < N; ++j)
      EXPECT_LT(dist(i, j), IntDistanceArc::Max_Distance)
        << "Should be reachable from " << i << " to " << j;
}
