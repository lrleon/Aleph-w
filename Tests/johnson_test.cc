/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
  / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
 / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
/_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
              |_|

This file is part of Aleph-w library

Copyright (c) 2002-2018 Leandro Rabindranath Leon

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
 * @file johnson_test.cc
 * @brief Tests for Johnson
 */

/**
 * @file johnson_test.cc
 * @brief Intensive tests for Johnson's all-pairs shortest paths algorithm.
 *
 * Tests cover:
 * - Basic correctness on small graphs
 * - Graphs with negative weights (but no negative cycles)
 * - Detection of negative cycles
 * - Comparison with Floyd-Warshall for verification
 * - Sparse vs dense graphs performance characteristics
 * - Edge cases: disconnected graphs, single node, etc.
 *
 * @author Generated for Aleph-w library
 */

#include <gtest/gtest.h>
#include <tpl_graph.H>
#include <Johnson.H>
#include <Dijkstra.H>
#include <Bellman_Ford.H>
#include <tpl_graph_utils.H>
#include <random>
#include <limits>
#include <vector>
#include <map>
#include <chrono>
#include <cmath>

using namespace Aleph;

// ==================== Type definitions ====================

// Node and Arc types for weighted digraphs
struct WeightedNode : public Graph_Node<int>
{
  using Graph_Node<int>::Graph_Node;
};

struct WeightedArc : public Graph_Arc<double>
{
  using Graph_Arc<double>::Graph_Arc;
};

using TestDigraph = List_Digraph<WeightedNode, WeightedArc>;
using Node = TestDigraph::Node;
using Arc = TestDigraph::Arc;

// Custom distance accessor for double weights
struct DoubleDistance
{
  using Distance_Type = double;
  
  Distance_Type operator()(Arc* arc) const
  {
    return arc->get_info();
  }
  
  static void set_zero(Arc* arc)
  {
    arc->get_info() = 0.0;
  }
  
  void set_weight(Arc* arc, Distance_Type w)
  {
    arc->get_info() = w;
  }
};

// ==================== Test Fixtures ====================

class JohnsonTest : public ::testing::Test
{
protected:
  TestDigraph g;
  std::vector<Node*> nodes;
  
  void SetUp() override
  {
    // Clean state for each test
    nodes.clear();
  }
  
  void TearDown() override
  {
    // Graph destructor handles cleanup
  }
  
  // Helper to create a path graph: 0 -> 1 -> 2 -> ... -> n-1
  void createPathGraph(int n, double weight = 1.0)
  {
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    
    for (int i = 0; i < n - 1; ++i)
      g.insert_arc(nodes[i], nodes[i + 1], weight);
  }
  
  // Helper to create a complete graph with random weights
  void createCompleteGraph(int n, double minWeight = 1.0, double maxWeight = 10.0,
                           unsigned seed = 42)
  {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(minWeight, maxWeight);
    
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j)
        if (i != j)
          g.insert_arc(nodes[i], nodes[j], dist(gen));
  }
  
  // Helper to create a graph with negative edges (but no negative cycles)
  void createNegativeWeightGraph()
  {
    // Classic example graph with negative edge (3 -> 1 with weight -3):
    //   0 --2--> 1 --1--> 2
    //   |               ^
    //   1               |
    //   v               |
    //   3 ----(-3)---> 1
    //   3 -----(2)---> 2
    
    nodes.push_back(g.insert_node(0));  // node 0
    nodes.push_back(g.insert_node(1));  // node 1
    nodes.push_back(g.insert_node(2));  // node 2
    nodes.push_back(g.insert_node(3));  // node 3
    
    g.insert_arc(nodes[0], nodes[1], 2.0);   // 0 -> 1, weight 2
    g.insert_arc(nodes[1], nodes[2], 1.0);   // 1 -> 2, weight 1
    g.insert_arc(nodes[0], nodes[3], 1.0);   // 0 -> 3, weight 1
    g.insert_arc(nodes[3], nodes[1], -3.0);  // 3 -> 1, weight -3
    g.insert_arc(nodes[3], nodes[2], 2.0);   // 3 -> 2, weight 2
  }
  
  // Helper to create a graph with a negative cycle
  void createNegativeCycleGraph()
  {
    nodes.push_back(g.insert_node(0));
    nodes.push_back(g.insert_node(1));
    nodes.push_back(g.insert_node(2));
    
    g.insert_arc(nodes[0], nodes[1], 1.0);   // 0 -> 1
    g.insert_arc(nodes[1], nodes[2], -2.0);  // 1 -> 2
    g.insert_arc(nodes[2], nodes[0], -1.0);  // 2 -> 0 (creates cycle with total -2)
  }
  
  // Helper to create a sparse random graph
  void createSparseGraph(int n, double edgeProbability = 0.2,
                         double minWeight = 1.0, double maxWeight = 10.0,
                         unsigned seed = 42)
  {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> weightDist(minWeight, maxWeight);
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j)
        if (i != j && probDist(gen) < edgeProbability)
          g.insert_arc(nodes[i], nodes[j], weightDist(gen));
  }
  
  // Compute all-pairs shortest paths using Floyd-Warshall for verification
  std::map<std::pair<int, int>, double> computeFloydWarshall()
  {
    const double INF = std::numeric_limits<double>::infinity();
    int n = nodes.size();
    
    // Initialize distance matrix
    std::vector<std::vector<double>> dist(n, std::vector<double>(n, INF));
    
    for (int i = 0; i < n; ++i)
      dist[i][i] = 0.0;
    
    // Fill in edge weights
    for (Arc_Iterator<TestDigraph> it(g); it.has_curr(); it.next())
    {
      auto arc = it.get_curr();
      auto src = g.get_src_node(arc);
      auto tgt = g.get_tgt_node(arc);
      int i = src->get_info();
      int j = tgt->get_info();
      double w = arc->get_info();
      if (w < dist[i][j])
        dist[i][j] = w;
    }
    
    // Floyd-Warshall relaxation
    for (int k = 0; k < n; ++k)
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
          if (dist[i][k] != INF && dist[k][j] != INF)
            if (dist[i][k] + dist[k][j] < dist[i][j])
              dist[i][j] = dist[i][k] + dist[k][j];
    
    // Convert to map
    std::map<std::pair<int, int>, double> result;
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j)
        if (dist[i][j] != INF)
          result[{i, j}] = dist[i][j];
    
    return result;
  }
};

// ==================== Basic Bellman-Ford Tests (Prerequisites) ====================

TEST_F(JohnsonTest, BellmanFordBasic)
{
  // Simple path graph: 0 -> 1 -> 2
  createPathGraph(3, 2.0);
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle);
  EXPECT_TRUE(bf.is_painted());
}

TEST_F(JohnsonTest, BellmanFordNegativeWeights)
{
  createNegativeWeightGraph();
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle) << "Graph has negative weights but no negative cycles";
}

TEST_F(JohnsonTest, BellmanFordNegativeCycle)
{
  createNegativeCycleGraph();
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.has_negative_cycle(nodes[0]);
  
  EXPECT_TRUE(hasNegCycle) << "Graph should have a negative cycle";
}

TEST_F(JohnsonTest, BellmanFordComputeNodesWeights)
{
  createNegativeWeightGraph();
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  
  // This should not throw (no negative cycles)
  auto weights = bf.compute_nodes_weights();
  
  // All original nodes should have weights
  EXPECT_EQ(weights.size(), nodes.size());
  
  // Verify weights are finite
  for (auto node : nodes)
  {
    auto* weightPtr = weights.search(node);
    ASSERT_NE(weightPtr, nullptr) << "Node should have a weight";
    EXPECT_TRUE(std::isfinite(weightPtr->second));
  }
}

TEST_F(JohnsonTest, BellmanFordComputeNodesWeightsThrowsOnNegativeCycle)
{
  createNegativeCycleGraph();
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  
  // This should throw because of negative cycle
  EXPECT_THROW(bf.compute_nodes_weights(), std::domain_error);
}

// ==================== Johnson Algorithm Tests ====================

TEST_F(JohnsonTest, JohnsonBasicPath)
{
  createPathGraph(4, 1.0);
  
  Johnson<TestDigraph, DoubleDistance> johnson(g);
  EXPECT_TRUE(johnson.is_initialized());
  
  // Distance from 0 to 3 should be 3.0 (three edges of weight 1.0)
  double dist = johnson.get_distance(nodes[0], nodes[3]);
  EXPECT_DOUBLE_EQ(dist, 3.0);
  
  // Distance from 0 to 1 should be 1.0
  dist = johnson.get_distance(nodes[0], nodes[1]);
  EXPECT_DOUBLE_EQ(dist, 1.0);
  
  // Distance from 0 to 0 should be 0.0
  dist = johnson.get_distance(nodes[0], nodes[0]);
  EXPECT_DOUBLE_EQ(dist, 0.0);
}

TEST_F(JohnsonTest, JohnsonNegativeWeights)
{
  createNegativeWeightGraph();
  // Graph structure:
  //   0 --2--> 1 --1--> 2
  //   |
  //   1
  //   v
  //   3 --(-3)--> 1
  //   3 ---(2)--> 2
  //
  // Expected distances from 0:
  //   0->0: 0, 0->1: -2 (via 3), 0->2: -1 (via 3->1), 0->3: 1
  // Expected distances from 1:
  //   1->1: 0, 1->2: 1
  // Expected distances from 3:
  //   3->1: -3, 3->2: -2 (via 1), 3->3: 0
  
  Johnson<TestDigraph, DoubleDistance> johnson(g);
  EXPECT_TRUE(johnson.is_initialized());
  
  // Test known reachable pairs with expected distances
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[0], nodes[0]), 0.0);
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[0], nodes[1]), -2.0);  // via 0->3->1
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[0], nodes[2]), -1.0);  // via 0->3->1->2
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[0], nodes[3]), 1.0);
  
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[1], nodes[1]), 0.0);
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[1], nodes[2]), 1.0);
  
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[2], nodes[2]), 0.0);
  
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[3], nodes[1]), -3.0);
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[3], nodes[2]), -2.0);  // via 3->1->2
  EXPECT_DOUBLE_EQ(johnson.get_distance(nodes[3], nodes[3]), 0.0);
}

TEST_F(JohnsonTest, JohnsonNegativeCycleDetection)
{
  createNegativeCycleGraph();
  
  // Johnson should throw on negative cycle detection
  using JohnsonType = Johnson<TestDigraph, DoubleDistance>;
  EXPECT_THROW(JohnsonType johnson(g), std::domain_error);
}

TEST_F(JohnsonTest, JohnsonAllPairs)
{
  createNegativeWeightGraph();
  
  Johnson<TestDigraph, DoubleDistance> johnson(g);
  auto allPairs = johnson.compute_all_pairs_distances();
  
  // Verify all pairs distances match expected values
  // Based on the graph structure, we know exactly which pairs are reachable
  
  // Helper to check finite distance
  auto checkFiniteDist = [&](int i, int j, double expected) {
    auto* entry = allPairs.search(std::make_pair(nodes[i], nodes[j]));
    ASSERT_NE(entry, nullptr) << "Missing entry for (" << i << ", " << j << ")";
    EXPECT_NEAR(entry->second, expected, 1e-9)
      << "Distance mismatch for (" << i << ", " << j << ")";
  };
  
  // Only verify reachable pairs (finite distances)
  // Unreachable pairs may or may not be in allPairs
  
  // From node 0: can reach all
  checkFiniteDist(0, 0, 0.0);
  checkFiniteDist(0, 1, -2.0);   // via 0->3->1
  checkFiniteDist(0, 2, -1.0);   // via 0->3->1->2
  checkFiniteDist(0, 3, 1.0);
  
  // From node 1: can reach 1 and 2
  checkFiniteDist(1, 1, 0.0);
  checkFiniteDist(1, 2, 1.0);
  
  // From node 2: can only reach itself
  checkFiniteDist(2, 2, 0.0);
  
  // From node 3: can reach 1, 2, and itself
  checkFiniteDist(3, 1, -3.0);
  checkFiniteDist(3, 2, -2.0);   // via 3->1->2
  checkFiniteDist(3, 3, 0.0);
}

// ==================== Reweighting Verification Tests ====================

TEST_F(JohnsonTest, ReweightingPreservesShortestPaths)
{
  // Test that Johnson's reweighting preserves shortest path structure
  // After reweighting: w'(u,v) = w(u,v) + h(u) - h(v)
  // For any path p from s to t: w'(p) = w(p) + h(s) - h(t)
  // So shortest paths are preserved since h(s) - h(t) is constant
  
  createNegativeWeightGraph();
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  auto weights = bf.compute_nodes_weights();
  
  // Verify the reweighting formula produces non-negative weights
  for (Arc_Iterator<TestDigraph> it(g); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    auto u = g.get_src_node(arc);
    auto v = g.get_tgt_node(arc);
    double w = arc->get_info();
    
    auto* hu_ptr = weights.search(u);
    auto* hv_ptr = weights.search(v);
    ASSERT_NE(hu_ptr, nullptr);
    ASSERT_NE(hv_ptr, nullptr);
    
    double hu = hu_ptr->second;
    double hv = hv_ptr->second;
    double w_prime = w + hu - hv;
    
    EXPECT_GE(w_prime, 0.0) 
      << "Reweighted edge should be non-negative: w=" << w 
      << ", h(u)=" << hu << ", h(v)=" << hv 
      << ", w'=" << w_prime;
  }
}

// ==================== Dijkstra on Reweighted Graph Tests ====================

TEST_F(JohnsonTest, DijkstraAfterReweighting)
{
  createNegativeWeightGraph();
  
  // Get node weights from Bellman-Ford
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  auto weights = bf.compute_nodes_weights();
  
  // Create a copy and reweight the edges
  TestDigraph reweighted;
  DynMapTree<Node*, Node*> nodeMap;
  
  // Copy nodes
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    auto orig = it.get_curr();
    auto copy = reweighted.insert_node(orig->get_info());
    nodeMap.insert(orig, copy);
  }
  
  // Copy and reweight arcs
  for (Arc_Iterator<TestDigraph> it(g); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    auto u = g.get_src_node(arc);
    auto v = g.get_tgt_node(arc);
    double w = arc->get_info();
    
    double hu = weights.find(u);
    double hv = weights.find(v);
    double w_prime = w + hu - hv;
    
    auto* uCopy = nodeMap.search(u);
    auto* vCopy = nodeMap.search(v);
    reweighted.insert_arc(uCopy->second, vCopy->second, w_prime);
  }
  
  // Now Dijkstra should work on the reweighted graph
  // (all weights are non-negative)
  auto* srcPtr = nodeMap.search(nodes[0]);
  ASSERT_NE(srcPtr, nullptr);
  Dijkstra_Min_Paths<TestDigraph, DoubleDistance> dijkstra;
  dijkstra.paint_min_paths_tree(reweighted, srcPtr->second);
  
  EXPECT_TRUE(dijkstra.is_painted());
}

// ==================== Comprehensive All-Pairs Tests ====================

TEST_F(JohnsonTest, AllPairsSmallComplete)
{
  // Small complete graph to verify all-pairs computation
  createCompleteGraph(5, 1.0, 10.0);
  
  auto floydResults = computeFloydWarshall();
  
  // Verify Floyd-Warshall gives us some results
  EXPECT_FALSE(floydResults.empty());
  
  // All pairs should be reachable in a complete graph
  int n = nodes.size();
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      EXPECT_NE(floydResults.find({i, j}), floydResults.end())
        << "Path from " << i << " to " << j << " should exist";
}

TEST_F(JohnsonTest, AllPairsSparse)
{
  // Sparse graph - some pairs may be unreachable
  createSparseGraph(10, 0.3, 1.0, 10.0);
  
  auto floydResults = computeFloydWarshall();
  
  // At least self-loops should have distance 0
  for (int i = 0; i < (int)nodes.size(); ++i)
  {
    auto it = floydResults.find({i, i});
    ASSERT_NE(it, floydResults.end());
    EXPECT_DOUBLE_EQ(it->second, 0.0);
  }
}

// ==================== Edge Cases ====================

TEST_F(JohnsonTest, SingleNode)
{
  nodes.push_back(g.insert_node(0));
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle);
}

TEST_F(JohnsonTest, TwoNodesNoEdge)
{
  nodes.push_back(g.insert_node(0));
  nodes.push_back(g.insert_node(1));
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle);
}

TEST_F(JohnsonTest, TwoNodesOneEdge)
{
  nodes.push_back(g.insert_node(0));
  nodes.push_back(g.insert_node(1));
  g.insert_arc(nodes[0], nodes[1], 5.0);
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle);
  EXPECT_TRUE(bf.is_painted());
}

TEST_F(JohnsonTest, SelfLoop)
{
  nodes.push_back(g.insert_node(0));
  g.insert_arc(nodes[0], nodes[0], 1.0);
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle);
}

TEST_F(JohnsonTest, NegativeSelfLoop)
{
  nodes.push_back(g.insert_node(0));
  g.insert_arc(nodes[0], nodes[0], -1.0);  // Negative self-loop = negative cycle
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.has_negative_cycle(nodes[0]);
  
  EXPECT_TRUE(hasNegCycle);
}

TEST_F(JohnsonTest, ParallelEdges)
{
  nodes.push_back(g.insert_node(0));
  nodes.push_back(g.insert_node(1));
  g.insert_arc(nodes[0], nodes[1], 5.0);
  g.insert_arc(nodes[0], nodes[1], 3.0);  // Shorter parallel edge
  g.insert_arc(nodes[0], nodes[1], 7.0);  // Longer parallel edge
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  
  EXPECT_FALSE(hasNegCycle);
}

// ==================== Stress Tests ====================

TEST_F(JohnsonTest, MediumGraph)
{
  createCompleteGraph(20, 1.0, 100.0);
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  
  // Bellman-Ford from node 0
  bool hasNegCycle = bf.paint_spanning_tree(nodes[0]);
  EXPECT_FALSE(hasNegCycle);
  
  // Verify we can get paths to all nodes
  for (size_t i = 1; i < nodes.size(); ++i)
  {
    Path<TestDigraph> path(g);
    bf.get_min_path(nodes[i], path);
    EXPECT_FALSE(path.is_empty()) << "Path to node " << i << " should exist";
  }
}

TEST_F(JohnsonTest, DISABLED_LargeGraphPerformance)
{
  // Disabled by default - for manual performance testing
  createCompleteGraph(100, 1.0, 100.0);
  
  // Johnson should be O(V^2 log V + VE) which is better than
  // Floyd-Warshall O(V^3) for sparse graphs
  
  auto start = std::chrono::high_resolution_clock::now();
  
  // Bellman-Ford for node weights
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  auto weights = bf.compute_nodes_weights();
  
  auto mid = std::chrono::high_resolution_clock::now();
  
  // Then |V| Dijkstra executions
  // (simplified - full Johnson would reweight and run Dijkstra from each node)
  
  auto end = std::chrono::high_resolution_clock::now();
  
  auto bfTime = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
  auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  std::cout << "Bellman-Ford time: " << bfTime.count() << " ms\n";
  std::cout << "Total time: " << totalTime.count() << " ms\n";
}

// ==================== Correctness vs Floyd-Warshall ====================

TEST_F(JohnsonTest, CorrectnessVsFloydWarshall)
{
  // Create a small graph with mixed weights (some negative)
  nodes.push_back(g.insert_node(0));
  nodes.push_back(g.insert_node(1));
  nodes.push_back(g.insert_node(2));
  nodes.push_back(g.insert_node(3));
  
  g.insert_arc(nodes[0], nodes[1], 3.0);
  g.insert_arc(nodes[0], nodes[2], 8.0);
  g.insert_arc(nodes[1], nodes[2], -2.0);  // Negative edge
  g.insert_arc(nodes[1], nodes[3], 1.0);
  g.insert_arc(nodes[2], nodes[3], 2.0);
  
  auto floydResults = computeFloydWarshall();
  
  // Expected distances from node 0:
  // 0->0: 0
  // 0->1: 3
  // 0->2: 3 + (-2) = 1 (via 0->1->2)
  // 0->3: 3 + (-2) + 2 = 3 (via 0->1->2->3) or 3 + 1 = 4 (via 0->1->3)
  //       min = 3
  
  auto d00 = floydResults[std::make_pair(0, 0)];
  auto d01 = floydResults[std::make_pair(0, 1)];
  auto d02 = floydResults[std::make_pair(0, 2)];
  auto d03 = floydResults[std::make_pair(0, 3)];
  
  EXPECT_DOUBLE_EQ(d00, 0.0);
  EXPECT_DOUBLE_EQ(d01, 3.0);
  EXPECT_DOUBLE_EQ(d02, 1.0);
  EXPECT_DOUBLE_EQ(d03, 3.0);
  
  // Now verify with Bellman-Ford (as a step towards Johnson)
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  bf.paint_spanning_tree(nodes[0]);
  
  // Get distances from Bellman-Ford
  Path<TestDigraph> path(g);
  
  // Distance to node 2
  auto dist2 = bf.get_min_path(nodes[2], path);
  EXPECT_DOUBLE_EQ(dist2, 1.0) << "Distance 0->2 should be 1.0";
  
  // Distance to node 3
  path = Path<TestDigraph>(g);
  auto dist3 = bf.get_min_path(nodes[3], path);
  EXPECT_DOUBLE_EQ(dist3, 3.0) << "Distance 0->3 should be 3.0";
}

// ==================== SPFA Variant Tests ====================

TEST_F(JohnsonTest, SPFAvsStandard)
{
  createNegativeWeightGraph();
  
  // Standard Bellman-Ford
  Bellman_Ford<TestDigraph, DoubleDistance> bf1(g);
  bool hasNeg1 = bf1.paint_spanning_tree(nodes[0]);
  
  // SPFA variant (faster version)
  Bellman_Ford<TestDigraph, DoubleDistance> bf2(g);
  bool hasNeg2 = bf2.faster_paint_spanning_tree(nodes[0]);
  
  EXPECT_EQ(hasNeg1, hasNeg2);
  
  // Both should detect no negative cycle
  EXPECT_FALSE(hasNeg1);
  EXPECT_FALSE(hasNeg2);
}

// ==================== Negative Cycle Tests ====================

TEST_F(JohnsonTest, NegativeCyclePathRetrieval)
{
  createNegativeCycleGraph();
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  auto cycle = bf.test_negative_cycle(nodes[0]);
  
  // If negative cycle is found, path should not be empty
  // and should form a cycle
  if (!cycle.is_empty())
  {
    // In Aleph-w, the path might not explicitly repeat the first node
    // but should form a connected cycle
    EXPECT_GE(cycle.size(), 2) << "Cycle should have at least 2 nodes";
  }
}

TEST_F(JohnsonTest, NegativeCycleNotReachable)
{
  // Create a graph where negative cycle exists but is not reachable from start
  nodes.push_back(g.insert_node(0));  // Isolated node
  nodes.push_back(g.insert_node(1));  // Part of cycle
  nodes.push_back(g.insert_node(2));  // Part of cycle
  
  // Cycle between nodes 1 and 2 (not reachable from 0)
  g.insert_arc(nodes[1], nodes[2], -1.0);
  g.insert_arc(nodes[2], nodes[1], -1.0);
  
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  
  // From node 0, should not find the negative cycle
  bool hasNegFromNode0 = bf.has_negative_cycle(nodes[0]);
  EXPECT_FALSE(hasNegFromNode0) << "Negative cycle not reachable from node 0";
  
  // Global check should find it
  Bellman_Ford<TestDigraph, DoubleDistance> bf2(g);
  bool hasNegGlobal = bf2.has_negative_cycle();
  EXPECT_TRUE(hasNegGlobal) << "Global check should find unreachable negative cycle";
}

// ==================== Integration Test: Manual Johnson Implementation ====================

TEST_F(JohnsonTest, ManualJohnsonImplementation)
{
  // This test manually implements Johnson's algorithm to verify correctness
  // and serves as a reference for fixing the actual implementation
  
  createNegativeWeightGraph();
  
  // Step 1: Add dummy node connected to all with 0-weight edges
  //         and run Bellman-Ford to get node potentials
  Bellman_Ford<TestDigraph, DoubleDistance> bf(g);
  DynMapTree<Node*, double> h;
  
  try
  {
    h = bf.compute_nodes_weights();
  }
  catch (const std::domain_error&)
  {
    FAIL() << "Unexpected negative cycle in test graph";
  }
  
  // Step 2: Reweight edges: w'(u,v) = w(u,v) + h(u) - h(v)
  std::map<Arc*, double> originalWeights;
  std::map<Arc*, double> reweightedWeights;
  
  for (Arc_Iterator<TestDigraph> it(g); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    auto u = g.get_src_node(arc);
    auto v = g.get_tgt_node(arc);
    double w = arc->get_info();
    
    originalWeights[arc] = w;
    
    double hu = h.find(u);
    double hv = h.find(v);
    double w_prime = w + hu - hv;
    
    EXPECT_GE(w_prime, -1e-9) << "Reweighted edge should be non-negative";
    
    reweightedWeights[arc] = w_prime;
    arc->get_info() = w_prime;  // Apply reweighting
  }
  
  // Step 3: Run Dijkstra from each source
  std::map<std::pair<int, int>, double> johnsonDistances;
  
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    Dijkstra_Min_Paths<TestDigraph, DoubleDistance> dijkstra;
    dijkstra.paint_min_paths_tree(g, nodes[i]);
    
    for (size_t j = 0; j < nodes.size(); ++j)
    {
      if (i == j)
      {
        johnsonDistances[{(int)i, (int)j}] = 0.0;
        continue;
      }
      
      Path<TestDigraph> path(g);
      try
      {
        double d_prime = dijkstra.get_min_path(nodes[j], path);
        
        // Step 4: Adjust back: d(u,v) = d'(u,v) - h(u) + h(v)
        double hu = h.find(nodes[i]);
        double hv = h.find(nodes[j]);
        double d = d_prime - hu + hv;
        
        johnsonDistances[{(int)i, (int)j}] = d;
      }
      catch (...)
      {
        // Node not reachable
      }
    }
  }
  
  // Restore original weights
  for (Arc_Iterator<TestDigraph> it(g); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    arc->get_info() = originalWeights[arc];
  }
  
  // Compare with Floyd-Warshall
  auto floydDistances = computeFloydWarshall();
  
  for (const auto& [pair, dist] : johnsonDistances)
  {
    auto it = floydDistances.find(pair);
    if (it != floydDistances.end())
    {
      EXPECT_NEAR(dist, it->second, 1e-9)
        << "Distance mismatch for pair (" << pair.first << ", " << pair.second << "): "
        << "Johnson=" << dist << ", Floyd=" << it->second;
    }
  }
}

// ==================== Main ====================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
