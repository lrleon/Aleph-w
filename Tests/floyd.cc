
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
 * @file floyd.cc
 * @brief Tests for Floyd
 */

#include <gtest/gtest.h>

#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

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
  
  // The direct arc is the shortest path: going through k adds 2k + 1. (The
  // test compared the distances with the maximum of double, which the
  // infinity of int is below.)
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (i != j) {
        EXPECT_EQ(dist(floyd.index_node(nodes[i]), floyd.index_node(nodes[j])),
                  i + j + 1);
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

// Test accessors for node array
TEST(FloydAccessors, node_accessors_behavior)
{
  Grafo g;
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  auto* n3 = g.insert_node(3);
  
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  
  // 1. Test get_nodes_ref() - should return a reference to internal state
  const auto& nodes_ref1 = floyd.get_nodes_ref();
  const auto& nodes_ref2 = floyd.get_nodes_ref();
  EXPECT_EQ(&nodes_ref1, &nodes_ref2); // Verify they point to the same object
  EXPECT_EQ(nodes_ref1.size(), 3u);
  
  // 2. Test get_nodes_copy() - should return an independent copy
  auto nodes_copy = floyd.get_nodes_copy();
  EXPECT_EQ(nodes_copy.size(), 3u);
  
  // 3. Test get_nodes() - should return an independent copy (Snapshot)
  auto nodes_val = floyd.get_nodes();
  EXPECT_EQ(nodes_val.size(), 3u);
  
  // 4. Verify ordering stability across all accessors
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(nodes_ref1(i), nodes_copy(i));
    EXPECT_EQ(nodes_ref1(i), nodes_val(i));
  }
  
  // 5. Verify that mutating a copy doesn't affect the original
  nodes_copy(0) = nullptr;
  EXPECT_NE(floyd.get_nodes_ref()(0), nullptr);
  
  // 6. Verify pointer validity (non-owning) using the solver's own index map.
  EXPECT_EQ(nodes_ref1(floyd.index_node(n1)), n1);
  EXPECT_EQ(nodes_ref1(floyd.index_node(n2)), n2);
  EXPECT_EQ(nodes_ref1(floyd.index_node(n3)), n3);
}

// ========== Parallel arcs, loops, graphs that are not directed ==========

namespace {

using UGrafo = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

template <class GT, class Distance, class SA>
long dist_of(const Floyd_All_Shortest_Paths<GT, Distance, SA> & floyd,
             typename GT::Node * s, typename GT::Node * t)
{
  return floyd.get_dist_mat()(floyd.index_node(s), floyd.index_node(t));
}

template <class GT>
std::vector<typename GT::Arc *> arcs_of(const Path<GT> & path)
{
  std::vector<typename GT::Arc *> arcs;
  path.for_each_arc([&arcs](typename GT::Arc * a) { arcs.push_back(a); });
  return arcs;
}

template <class GT>
long cost_of(const Path<GT> & path)
{
  long total = 0;
  for (auto a : arcs_of(path))
    total += a->get_info();
  return total;
}

// Distances from s by relaxing every arc until nothing changes, apart from
// the algorithm under test; the nodes that s does not reach are not in it
template <class GT>
std::map<typename GT::Node *, long> relaxed_distances(GT & g,
                                                      typename GT::Node * s)
{
  std::map<typename GT::Node *, long> d{{s, 0}};
  const int directions = g.is_digraph() ? 1 : 2;
  for (bool changed = true; changed; )
    {
      changed = false;
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          auto u = g.get_src_node(a);
          auto v = g.get_tgt_node(a);
          for (int dir = 0; dir < directions; ++dir, std::swap(u, v))
            if (d.contains(u) and
                (not d.contains(v) or d[u] + a->get_info() < d[v]))
              {
                d[v] = d[u] + a->get_info();
                changed = true;
              }
        }
    }
  return d;
}

// A random graph with parallel arcs and loops, against the relaxation; and
// the cost of each path against the matrix
template <class GT>
void check_against_relaxation(const unsigned seed)
{
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> pick(0, 7);
  std::uniform_int_distribution<int> weight(0, 20);
  GT g;
  std::vector<typename GT::Node *> nodes;
  for (int i = 0; i < 8; ++i)
    nodes.push_back(g.insert_node(i));
  for (int k = 0; k < 24; ++k)
    g.insert_arc(nodes[pick(rng)], nodes[pick(rng)], weight(rng));

  Floyd_All_Shortest_Paths<GT> floyd(g);
  ASSERT_FALSE(floyd.has_negative_cycle()) << "seed " << seed;
  for (auto s : nodes)
    {
      auto d = relaxed_distances(g, s);
      for (auto t : nodes)
        {
          if (not d.contains(t))
            {
              EXPECT_EQ(dist_of(floyd, s, t), numeric_limits<int>::max())
                << "seed " << seed;
              continue;
            }
          EXPECT_EQ(dist_of(floyd, s, t), d[t]) << "seed " << seed;
          auto path = floyd.get_min_path(s, t);
          EXPECT_EQ(path.get_first_node(), s) << "seed " << seed;
          EXPECT_EQ(path.get_last_node(), t) << "seed " << seed;
          EXPECT_EQ(cost_of(path), d[t]) << "seed " << seed;
        }
    }
}

} // namespace

// The matrix took the arc that IndexArc kept for two nodes, the first one
// inserted, and get_min_path() took the first one it found.
TEST(FloydParallelArcs, TakesTheLightest)
{
  for (const bool heavy_first : {true, false})
    {
      Grafo g;
      auto a = g.insert_node(0);
      auto b = g.insert_node(1);
      GArc * light = nullptr;
      if (heavy_first)
        {
          g.insert_arc(a, b, 5);
          light = g.insert_arc(a, b, 1);
        }
      else
        {
          light = g.insert_arc(a, b, 1);
          g.insert_arc(a, b, 5);
        }

      Floyd_All_Shortest_Paths<Grafo> floyd(g);
      EXPECT_EQ(dist_of(floyd, a, b), 1) << heavy_first;
      EXPECT_EQ(arcs_of(floyd.get_min_path(a, b)),
                std::vector<GArc *>{light}) << heavy_first;
    }
}

// get_min_path() looked for each arc in the direction of the path, so in a
// graph that is not directed it threw when the path went against an arc.
TEST(FloydUndirected, PathsGoBothWaysAlongTheArcs)
{
  UGrafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  g.insert_arc(a, b, 3);
  g.insert_arc(c, b, 4);
  g.insert_arc(b, c, 9); // parallel to the arc of 4

  Floyd_All_Shortest_Paths<UGrafo> floyd(g);
  EXPECT_EQ(dist_of(floyd, a, c), 7);
  EXPECT_EQ(dist_of(floyd, c, a), 7);

  auto path = floyd.get_min_path(c, a);
  EXPECT_EQ(path.get_first_node(), c);
  EXPECT_EQ(path.get_last_node(), a);
  EXPECT_EQ(cost_of(path), 7);
}

// In a graph that is not directed, each arc counts both ways, so a negative
// arc is a negative cycle on its own.
TEST(FloydUndirected, ANegativeArcIsANegativeCycle)
{
  UGrafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, b, -1);
  Floyd_All_Shortest_Paths<UGrafo> floyd(g);
  EXPECT_TRUE(floyd.has_negative_cycle());
  EXPECT_THROW(floyd.get_min_path(a, b), std::domain_error);
}

// The guard of the sums discarded those below the range of the type, so the
// cycle of a -> b (lowest) and b -> a (-1) went unseen, and
// has_negative_cycle() was false.
TEST(FloydNumeric, ADistanceBelowTheRangeThrows)
{
  const int lowest = numeric_limits<int>::lowest();

  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, b, lowest);
  g.insert_arc(b, a, -1);
  EXPECT_THROW(Floyd_All_Shortest_Paths<Grafo>{g}, std::overflow_error);

  // a cycle of three arcs whose sum, and not that of two, is below
  Grafo h;
  auto x = h.insert_node(0);
  auto y = h.insert_node(1);
  auto z = h.insert_node(2);
  h.insert_arc(x, y, lowest / 2);
  h.insert_arc(y, z, lowest / 2);
  h.insert_arc(z, x, -1);
  EXPECT_THROW(Floyd_All_Shortest_Paths<Grafo>{h}, std::overflow_error);

  UGrafo u;
  auto c = u.insert_node(0);
  auto d = u.insert_node(1);
  u.insert_arc(c, d, lowest);
  EXPECT_THROW(Floyd_All_Shortest_Paths<UGrafo>{u}, std::overflow_error);
}

// A sum that reaches lowest() exactly is a distance like any other.
TEST(FloydNumeric, ADistanceAtTheBottomOfTheRangeCounts)
{
  const int lowest = numeric_limits<int>::lowest();
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  g.insert_arc(a, b, lowest / 2);
  g.insert_arc(b, c, lowest / 2);

  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  EXPECT_FALSE(floyd.has_negative_cycle());
  EXPECT_EQ(dist_of(floyd, a, c), lowest);
  EXPECT_EQ(cost_of(floyd.get_min_path(a, c)), lowest);
}

// The diagonal was set to 0 without looking at the loops, so a negative loop
// was not a negative cycle.
TEST(FloydLoops, ANegativeLoopIsANegativeCycle)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, b, 2);
  g.insert_arc(a, a, -1);
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  EXPECT_TRUE(floyd.has_negative_cycle());

  UGrafo u;
  auto c = u.insert_node(0);
  auto d = u.insert_node(1);
  u.insert_arc(c, d, 2);
  u.insert_arc(d, d, -1);
  Floyd_All_Shortest_Paths<UGrafo> ufloyd(u);
  EXPECT_TRUE(ufloyd.has_negative_cycle());
}

TEST(FloydLoops, APositiveLoopDoesNotCount)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, a, 3);
  g.insert_arc(a, b, 2);
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  EXPECT_FALSE(floyd.has_negative_cycle());
  EXPECT_EQ(dist_of(floyd, a, a), 0);
  EXPECT_EQ(cost_of(floyd.get_min_path(a, b)), 2);
}

// With a negative cycle, rebuilding a path could go around it forever.
TEST(FloydNegativeCycle, GetMinPathThrows)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, a, -1);
  g.insert_arc(a, b, 2);
  Floyd_All_Shortest_Paths<Grafo> floyd(g);
  ASSERT_TRUE(floyd.has_negative_cycle());
  EXPECT_THROW(floyd.get_min_path(a, b), std::domain_error);
}

// get_min_path() looked for the arcs without the filter, so it could take an
// arc that the matrix did not see.
TEST(FloydFilter, PathsUseOnlyTheArcsOfTheFilter)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  GArc * hidden = g.insert_arc(a, b, 1);
  GArc * shown = g.insert_arc(a, b, 5);

  struct Hide
  {
    GArc * hidden = nullptr;
    bool operator()(GArc * arc) const noexcept { return arc != hidden; }
  };

  Hide hide{hidden};
  Floyd_All_Shortest_Paths<Grafo, Dft_Dist<Grafo>, Hide> floyd(g, hide);
  EXPECT_EQ(floyd.get_dist_mat()(floyd.index_node(a), floyd.index_node(b)), 5);
  EXPECT_EQ(arcs_of(floyd.get_min_path(a, b)), std::vector<GArc *>{shown});

  // The solver kept a reference to its filter, so a filter passed as a
  // temporary was gone once the solver was built
  Floyd_All_Shortest_Paths<Grafo, Dft_Dist<Grafo>, Hide>
    from_temporary(g, Hide{hidden});
  EXPECT_EQ(arcs_of(from_temporary.get_min_path(a, b)),
            std::vector<GArc *>{shown});
}

TEST(FloydRandom, MatchesRelaxationWithParallelArcsAndLoops)
{
  for (unsigned seed = 1; seed <= 30; ++seed)
    {
      check_against_relaxation<Grafo>(seed);
      check_against_relaxation<UGrafo>(seed);
    }
}

// ========== A distance given to the constructor ==========

namespace {

// The weight of an arc times a factor; with a negative factor, the heaviest
// arcs are the lightest
struct Scaled_Dist
{
  using Distance_Type = int;
  int factor = 1;
  int operator()(GArc * a) const noexcept { return factor * a->get_info(); }
};

// The weight of an arc taken from a base, with no default constructor
struct From_Base_Dist
{
  using Distance_Type = int;
  int base;
  explicit From_Base_Dist(int b) noexcept : base(b) {}
  int operator()(GArc * a) const noexcept { return base - a->get_info(); }
};

// The weight of an arc, with a call operator that is not const
struct Not_Const_Dist
{
  using Distance_Type = int;
  int operator()(GArc * a) noexcept { return a->get_info(); }
};

} // namespace

// The solver built its distance with Distance(), so a distance with state
// could not be given to it. get_min_path() must take, of the parallel arcs,
// the lightest for that distance.
TEST(FloydDistance, UsesTheDistanceItWasGiven)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  g.insert_arc(a, b, 1);
  GArc * heavy = g.insert_arc(a, b, 5);
  GArc * bc = g.insert_arc(b, c, 2);

  Floyd_All_Shortest_Paths<Grafo, Scaled_Dist> floyd(g, Scaled_Dist{-1});
  ASSERT_FALSE(floyd.has_negative_cycle());
  EXPECT_EQ(dist_of(floyd, a, b), -5);
  EXPECT_EQ(dist_of(floyd, a, c), -7);
  EXPECT_EQ(arcs_of(floyd.get_min_path(a, c)),
            (std::vector<GArc *>{heavy, bc}));
}

// A distance without a default constructor, alone and with a filter
TEST(FloydDistance, TakesADistanceWithoutADefaultConstructor)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  GArc * heavy = g.insert_arc(a, b, 9); // the lightest for a base of 10
  GArc * light = g.insert_arc(a, b, 4);

  Floyd_All_Shortest_Paths<Grafo, From_Base_Dist> floyd(g, From_Base_Dist(10));
  EXPECT_EQ(dist_of(floyd, a, b), 1);
  EXPECT_EQ(arcs_of(floyd.get_min_path(a, b)), std::vector<GArc *>{heavy});

  struct Hide
  {
    GArc * hidden = nullptr;
    bool operator()(GArc * arc) const noexcept { return arc != hidden; }
  };

  Floyd_All_Shortest_Paths<Grafo, From_Base_Dist, Hide>
    filtered(g, From_Base_Dist(10), Hide{heavy});
  EXPECT_EQ(dist_of(filtered, a, b), 6);
  EXPECT_EQ(arcs_of(filtered.get_min_path(a, b)), std::vector<GArc *>{light});
}

// The solver called a distance built for each arc, so its call operator did
// not have to be const; get_min_path(), which is const, now calls the copy
// that the solver keeps.
TEST(FloydDistance, TakesACallOperatorThatIsNotConst)
{
  Grafo g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, b, 5);
  GArc * light = g.insert_arc(a, b, 1);

  const Floyd_All_Shortest_Paths<Grafo, Not_Const_Dist> floyd(g);
  EXPECT_EQ(dist_of(floyd, a, b), 1);
  EXPECT_EQ(arcs_of(floyd.get_min_path(a, b)), std::vector<GArc *>{light});
}
