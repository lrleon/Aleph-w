
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
 * @file floyd.cc
 * @brief Tests for Floyd
 */

#include <gtest/gtest.h>

#include <tpl_graph_utils.H>
#include <Floyd_Warshall.H>

using namespace Aleph;
using namespace std;

// Graph type using proper node/arc wrappers
using Grafo = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using GNode = Grafo::Node;
using GArc = Grafo::Arc;

namespace {

// Helper to insert arc between nodes by value
void insert_arc_by_value(Grafo& g, int src_val, int tgt_val, int weight)
{
  GNode* src = nullptr;
  GNode* tgt = nullptr;
  
  for (Grafo::Node_Iterator it(g); it.has_curr(); it.next_ne()) {
    auto n = it.get_curr();
    if (n->get_info() == src_val) src = n;
    if (n->get_info() == tgt_val) tgt = n;
  }
  
  if (src && tgt)
    g.insert_arc(src, tgt, weight);
}

} // namespace

// Test basic Floyd algorithm on a simple graph
TEST(FloydBasicGraph, computes_correct_distances)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  // Create edges: 0->1 (weight 1), 1->2 (weight 2), 0->2 (weight 5)
  insert_arc_by_value(g, 0, 1, 1);
  insert_arc_by_value(g, 1, 2, 2);
  insert_arc_by_value(g, 0, 2, 5);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_FALSE(floyd.has_negative_cycle());
  
  const auto& dist = floyd.get_dist_mat();

  const long i0 = floyd.index_node(n0);
  const long i1 = floyd.index_node(n1);
  const long i2 = floyd.index_node(n2);
  
  // Distance from node to itself should be 0
  EXPECT_EQ(dist(i0, i0), 0);
  EXPECT_EQ(dist(i1, i1), 0);
  EXPECT_EQ(dist(i2, i2), 0);
  
  // Distance 0->1 should be 1 (direct edge)
  EXPECT_EQ(dist(i0, i1), 1);
  
  // Distance 0->2 should be 3 (0->1->2, not direct 0->2 with weight 5)
  EXPECT_EQ(dist(i0, i2), 3);
  
  // Distance 1->2 should be 2 (direct edge)
  EXPECT_EQ(dist(i1, i2), 2);
}

// Test handling of unreachable nodes
TEST(FloydBasicGraph, handles_unreachable_nodes)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  // No edges between nodes
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  const auto& dist = floyd.get_dist_mat();
  const int Inf = numeric_limits<int>::max();

  const long i0 = floyd.index_node(n0);
  const long i1 = floyd.index_node(n1);
  
  // Distances to unreachable nodes should be infinity
  EXPECT_EQ(dist(i0, i1), Inf);
  EXPECT_EQ(dist(i1, i0), Inf);

  // Unreachable paths should be reported as empty paths
  EXPECT_TRUE(floyd.get_min_path(i0, i1).is_empty());
  EXPECT_TRUE(floyd.get_min_path(i1, i0).is_empty());
}

// Test negative weights without negative cycles
TEST(FloydNegativeWeights, handles_negative_weights_without_cycles)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  // 0->1 (weight -1), 1->2 (weight 2), 0->2 (weight 3)
  g.insert_arc(n0, n1, -1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 3);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_FALSE(floyd.has_negative_cycle());
  
  // Get actual indices for nodes
  long i0 = floyd.index_node(n0);
  long i1 = floyd.index_node(n1);
  long i2 = floyd.index_node(n2);
  
  const auto& dist = floyd.get_dist_mat();
  
  // Distance 0->1 should be -1 (direct negative edge)
  EXPECT_EQ(dist(i0, i1), -1);
  
  // Distance 0->2 should be 1 (0->1->2: -1+2=1, better than direct 0->2: 3)
  EXPECT_EQ(dist(i0, i2), 1);
}

// Test negative cycle detection
TEST(FloydNegativeCycle, detects_negative_cycles)
{
  Grafo g;
  g.insert_node(0);
  g.insert_node(1);
  g.insert_node(2);
  
  // Create cycle: 0->1->2->0 with total weight -1
  insert_arc_by_value(g, 0, 1, 1);
  insert_arc_by_value(g, 1, 2, -1);
  insert_arc_by_value(g, 2, 0, -1);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_TRUE(floyd.has_negative_cycle());
}

// Test index_node method
TEST(FloydIndexNode, finds_correct_indices)
{
  Grafo g;
  g.insert_node(10);
  g.insert_node(20);
  g.insert_node(30);
  
  insert_arc_by_value(g, 10, 20, 1);
  insert_arc_by_value(g, 20, 30, 1);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  const auto nodes = floyd.get_nodes();
  
  // Test that we can find each node's index
  for (size_t i = 0; i < nodes.size(); ++i) {
    auto node = floyd.select_node(i);
    EXPECT_EQ(floyd.index_node(node), static_cast<long>(i));
  }
}

// Test null pointer handling
TEST(FloydIndexNode, throws_on_null_pointer)
{
  Grafo g;
  g.insert_node(0);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_THROW(floyd.index_node(nullptr), std::invalid_argument);
}

// Test path reconstruction with indices
TEST(FloydPathReconstruction, throws_on_invalid_indices)
{
  Grafo g;
  g.insert_node(0);
  g.insert_node(1);
  insert_arc_by_value(g, 0, 1, 1);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  const long n = g.get_num_nodes();
  
  EXPECT_THROW(floyd.get_min_path(-1L, 0L), std::out_of_range);
  EXPECT_THROW(floyd.get_min_path(0L, n), std::out_of_range);
  EXPECT_THROW(floyd.get_min_path(n, 0L), std::out_of_range);
}

// Test path reconstruction returns valid path
TEST(FloydPathReconstruction, returns_valid_path)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  // Get actual indices
  long i0 = floyd.index_node(n0);
  long i2 = floyd.index_node(n2);
  
  // Get shortest path from n0 to n2
  auto path = floyd.get_min_path(i0, i2);
  
  // Path should have at least 2 nodes (start and end)
  EXPECT_GE(path.size(), 2u);
  
  // First node should be n0, last should be n2
  EXPECT_EQ(path.get_first_node(), n0);
  EXPECT_EQ(path.get_last_node(), n2);
}

// Test self-path
TEST(FloydPathReconstruction, handles_self_paths)
{
  Grafo g;
  g.insert_node(0);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  auto path = floyd.get_min_path(0L, 0L);
  EXPECT_EQ(path.size(), 1u);
}

// Test entry formatting
TEST(FloydUtility, entry_formats_distances_correctly)
{
  Grafo g;
  g.insert_node(0);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_EQ(floyd.entry(42), "42");
  EXPECT_EQ(floyd.entry(numeric_limits<int>::max()), "Inf");
  EXPECT_EQ(floyd.entry(-5), "-5");
}

// Test path matrix structure
TEST(FloydMatrices, path_matrix_has_correct_structure)
{
  Grafo g;
  g.insert_node(0);
  g.insert_node(1);
  g.insert_node(2);
  
  insert_arc_by_value(g, 0, 1, 1);
  insert_arc_by_value(g, 1, 2, 1);
  insert_arc_by_value(g, 0, 2, 5);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  const auto& path_mat = floyd.get_path_mat();
  const auto& dist = floyd.get_dist_mat();
  const int Inf = numeric_limits<int>::max();
  const long n = g.get_num_nodes();
  
  EXPECT_EQ(path_mat.rows(), n);
  EXPECT_EQ(path_mat.cols(), n);
  
  // Path matrix entries should be valid node indices
  for (long i = 0; i < n; ++i) {
    for (long j = 0; j < n; ++j) {
      if (dist(i, j) == Inf) {
        EXPECT_EQ(path_mat(i, j), -1);
        continue;
      }

      // Reachable (including i == j): must be a valid index
      long k = path_mat(i, j);
      EXPECT_GE(k, 0);
      EXPECT_LT(k, n);
    }
  }
}

// Test complete graph
TEST(FloydLargeGraph, handles_complete_graph)
{
  const int N = 10;
  Grafo g;
  vector<GNode*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create complete graph
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (i != j)
        g.insert_arc(nodes[i], nodes[j], i + j + 1);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_FALSE(floyd.has_negative_cycle());
  
  const auto& dist = floyd.get_dist_mat();
  
  // All diagonal elements should be 0
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(dist(i, i), 0);
  
  // All off-diagonal elements should be finite and positive
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (i != j) {
        EXPECT_GT(dist(i, j), 0);
        EXPECT_LT(dist(i, j), numeric_limits<double>::max());
      }
}

// Test single node graph
TEST(FloydStress, handles_single_node_graph)
{
  Grafo g;
  g.insert_node(42);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  EXPECT_FALSE(floyd.has_negative_cycle());
  
  const auto& dist = floyd.get_dist_mat();
  EXPECT_EQ(dist.rows(), 1);
  EXPECT_EQ(dist.cols(), 1);
  EXPECT_EQ(dist(0, 0), 0);
  
  // Test path reconstruction for single node
  auto path = floyd.get_min_path(0L, 0L);
  EXPECT_EQ(path.size(), 1u);
  EXPECT_EQ(path.get_first_node()->get_info(), 42);
}
