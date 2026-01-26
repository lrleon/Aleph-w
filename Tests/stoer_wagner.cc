
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

#include <gtest/gtest.h>
#include <Stoer_Wagner.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

using namespace Aleph;
using namespace std;

// ============================================================================
// Graph Types
// ============================================================================

using IntGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using WeightedGraph = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
using DoubleGraph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using SGraph = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;

// ============================================================================
// Helper Graph Builders
// ============================================================================

namespace {

// Create a simple triangle graph (3 nodes, 3 edges)
IntGraph create_triangle()
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n0, n2, 1);

  return g;
}

// Create a square graph (4 nodes, 4 edges)
IntGraph create_square()
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n3, n0, 1);

  return g;
}

// Create a barbell graph: two K_k cliques connected by a single edge
// Min-cut: 1 (the bridge)
IntGraph create_barbell(int k = 3)
{
  IntGraph g;
  vector<IntGraph::Node*> left(k), right(k);

  for (int i = 0; i < k; ++i)
  {
    left[i] = g.insert_node(i);
    right[i] = g.insert_node(k + i);
  }

  // Left clique
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      g.insert_arc(left[i], left[j], 1);

  // Right clique
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      g.insert_arc(right[i], right[j], 1);

  // Bridge
  g.insert_arc(left[0], right[0], 1);

  return g;
}

// Create a path graph: 0 - 1 - 2 - ... - (n-1)
// Min-cut: 1 (any edge)
IntGraph create_path(int n)
{
  IntGraph g;
  vector<IntGraph::Node*> nodes(n);

  for (int i = 0; i < n; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  return g;
}

// Create a cycle graph: 0 - 1 - 2 - ... - (n-1) - 0
// Min-cut: 2 (need to cut two edges)
IntGraph create_cycle(int n)
{
  IntGraph g;
  vector<IntGraph::Node*> nodes(n);

  for (int i = 0; i < n; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < n; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % n], 1);

  return g;
}

// Create a complete graph K_n
// Min-cut: n-1 (isolate any single vertex)
IntGraph create_complete_graph(int n)
{
  IntGraph g;
  vector<IntGraph::Node*> nodes(n);

  for (int i = 0; i < n; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j)
      g.insert_arc(nodes[i], nodes[j], 1);

  return g;
}

// Create a star graph: center connected to all others
// Min-cut: 1 (any spoke)
IntGraph create_star(int n)
{
  IntGraph g;
  auto center = g.insert_node(0);

  for (int i = 1; i < n; ++i)
  {
    auto leaf = g.insert_node(i);
    g.insert_arc(center, leaf, 1);
  }

  return g;
}

// Create two clusters connected by k edges
// Min-cut: k * weight_per_edge
IntGraph create_two_clusters(int cluster_size, int bridge_count, int weight = 1)
{
  IntGraph g;
  vector<IntGraph::Node*> left(cluster_size), right(cluster_size);

  for (int i = 0; i < cluster_size; ++i)
  {
    left[i] = g.insert_node(i);
    right[i] = g.insert_node(cluster_size + i);
  }

  // Left cluster (fully connected with high weight)
  for (int i = 0; i < cluster_size; ++i)
    for (int j = i + 1; j < cluster_size; ++j)
      g.insert_arc(left[i], left[j], 100);

  // Right cluster (fully connected with high weight)
  for (int i = 0; i < cluster_size; ++i)
    for (int j = i + 1; j < cluster_size; ++j)
      g.insert_arc(right[i], right[j], 100);

  // Bridge edges with specified weight
  for (int i = 0; i < bridge_count; ++i)
    g.insert_arc(left[i % cluster_size], right[i % cluster_size], weight);

  return g;
}

// Create a weighted chain: A -w1- B -w2- C -w3- D
WeightedGraph create_weighted_chain(int w1, int w2, int w3)
{
  WeightedGraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b, w1);
  g.insert_arc(b, c, w2);
  g.insert_arc(c, d, w3);

  return g;
}

} // namespace

// ============================================================================
// Construction Tests
// ============================================================================

TEST(StoerWagnerConstruction, DefaultConstruction)
{
  EXPECT_NO_THROW(Stoer_Wagner_Min_Cut<IntGraph>());
}

TEST(StoerWagnerConstruction, WithCustomDistance)
{
  struct DoubleWeight
  {
    using Distance_Type = int;
    int operator()(IntGraph::Arc* a) const { return a->get_info() * 2; }
  };

  EXPECT_NO_THROW((Stoer_Wagner_Min_Cut<IntGraph, DoubleWeight>()));
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(StoerWagnerErrors, ThrowsOnSingleNode)
{
  IntGraph g;
  g.insert_node(0);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  EXPECT_THROW(sw(g, vs, vt, cut), std::domain_error);
}

TEST(StoerWagnerErrors, HandlesDisconnectedGraph)
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  // Two disconnected components: {0,1} and {2,3}
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n2, n3, 1);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  // Disconnected graph has min-cut = 0
  int min_cut = sw(g, vs, vt, cut);
  EXPECT_EQ(min_cut, 0);
  EXPECT_TRUE(cut.is_empty());
}

TEST(StoerWagnerErrors, HandlesTwoNodesNoEdges)
{
  IntGraph g;
  g.insert_node(0);
  g.insert_node(1);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);
  EXPECT_EQ(min_cut, 0);
}

// ============================================================================
// Basic Min-Cut Tests
// ============================================================================

TEST(StoerWagnerMinCut, TwoNodesOneEdge)
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 5);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 5);
  EXPECT_EQ(cut.size(), 1u);
  EXPECT_EQ(vs.size(), 1u);
  EXPECT_EQ(vt.size(), 1u);
}

TEST(StoerWagnerMinCut, Triangle)
{
  auto g = create_triangle();
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut of triangle: isolate one vertex, cut 2 edges
  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());
}

TEST(StoerWagnerMinCut, Square)
{
  auto g = create_square();
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut of square: any 2 edges that disconnect
  EXPECT_EQ(min_cut, 2);
}

TEST(StoerWagnerMinCut, Barbell)
{
  auto g = create_barbell(4);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut is the single bridge
  EXPECT_EQ(min_cut, 1);
  EXPECT_EQ(cut.size(), 1u);
}

TEST(StoerWagnerMinCut, Path)
{
  auto g = create_path(6);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Path graph has min-cut = 1
  EXPECT_EQ(min_cut, 1);
  EXPECT_EQ(cut.size(), 1u);
}

TEST(StoerWagnerMinCut, Cycle)
{
  auto g = create_cycle(6);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Cycle has min-cut = 2
  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(cut.size(), 2u);
}

TEST(StoerWagnerMinCut, Star)
{
  auto g = create_star(6);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Star graph min-cut = 1 (any leaf)
  EXPECT_EQ(min_cut, 1);
}

// ============================================================================
// Complete Graph Tests
// ============================================================================

TEST(StoerWagnerMinCut, CompleteK3)
{
  auto g = create_complete_graph(3);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // K3 min-cut = 2 (isolate one vertex)
  EXPECT_EQ(min_cut, 2);
}

TEST(StoerWagnerMinCut, CompleteK4)
{
  auto g = create_complete_graph(4);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // K4 min-cut = 3 (isolate one vertex)
  EXPECT_EQ(min_cut, 3);
}

TEST(StoerWagnerMinCut, CompleteK5)
{
  auto g = create_complete_graph(5);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // K5 min-cut = 4 (isolate one vertex)
  EXPECT_EQ(min_cut, 4);
}

TEST(StoerWagnerMinCut, CompleteK6)
{
  auto g = create_complete_graph(6);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // K6 min-cut = 5 (isolate one vertex)
  EXPECT_EQ(min_cut, 5);
}

// ============================================================================
// Weighted Graph Tests
// ============================================================================

TEST(StoerWagnerWeighted, WeakMiddleEdge)
{
  auto g = create_weighted_chain(10, 1, 10);
  Stoer_Wagner_Min_Cut<WeightedGraph> sw;

  DynList<WeightedGraph::Node*> vs, vt;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Middle edge (weight 1) is the weakest
  EXPECT_EQ(min_cut, 1);
}

TEST(StoerWagnerWeighted, WeakFirstEdge)
{
  auto g = create_weighted_chain(1, 10, 10);
  Stoer_Wagner_Min_Cut<WeightedGraph> sw;

  DynList<WeightedGraph::Node*> vs, vt;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 1);
}

TEST(StoerWagnerWeighted, WeakLastEdge)
{
  auto g = create_weighted_chain(10, 10, 1);
  Stoer_Wagner_Min_Cut<WeightedGraph> sw;

  DynList<WeightedGraph::Node*> vs, vt;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 1);
}

TEST(StoerWagnerWeighted, TwoClustersWeakBridge)
{
  auto g = create_two_clusters(4, 2, 1);  // 2 bridges with weight 1 each
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut: 2 bridges * weight 1 = 2
  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(cut.size(), 2u);
}

TEST(StoerWagnerWeighted, TwoClustersHeavyBridge)
{
  auto g = create_two_clusters(4, 1, 50);  // 1 bridge with weight 50
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut: 1 bridge * weight 50 = 50
  EXPECT_EQ(min_cut, 50);
  EXPECT_EQ(cut.size(), 1u);
}

TEST(StoerWagnerWeighted, WeightedTriangle)
{
  WeightedGraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");

  // A-B: 1, B-C: 2, A-C: 3
  g.insert_arc(a, b, 1);
  g.insert_arc(b, c, 2);
  g.insert_arc(a, c, 3);

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> vs, vt;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut: isolate B, cut {A-B(1), B-C(2)} = 3
  // Or isolate A, cut {A-B(1), A-C(3)} = 4
  // Or isolate C, cut {B-C(2), A-C(3)} = 5
  EXPECT_EQ(min_cut, 3);
}

// ============================================================================
// Partition Validity Tests
// ============================================================================

TEST(StoerWagnerPartitions, PartitionsCoverAllNodes)
{
  auto g = create_barbell(5);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  sw(g, vs, vt, cut);

  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());
}

TEST(StoerWagnerPartitions, PartitionsNonEmpty)
{
  auto g = create_complete_graph(5);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  sw(g, vs, vt, cut);

  EXPECT_FALSE(vs.is_empty());
  EXPECT_FALSE(vt.is_empty());
}

TEST(StoerWagnerPartitions, NoOverlap)
{
  auto g = create_cycle(8);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  sw(g, vs, vt, cut);

  DynSetTree<IntGraph::Node*> vs_set;
  for (auto it = vs.get_it(); it.has_curr(); it.next())
    vs_set.insert(it.get_curr());

  for (auto it = vt.get_it(); it.has_curr(); it.next())
    EXPECT_FALSE(vs_set.has(it.get_curr()));
}

TEST(StoerWagnerPartitions, CutEdgesCrossPartition)
{
  auto g = create_barbell(4);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  sw(g, vs, vt, cut);

  DynSetTree<IntGraph::Node*> vs_set, vt_set;
  for (auto it = vs.get_it(); it.has_curr(); it.next())
    vs_set.insert(it.get_curr());
  for (auto it = vt.get_it(); it.has_curr(); it.next())
    vt_set.insert(it.get_curr());

  for (auto it = cut.get_it(); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    auto src = g.get_src_node(arc);
    auto tgt = g.get_tgt_node(arc);

    bool crosses = (vs_set.has(src) && vt_set.has(tgt)) ||
                   (vt_set.has(src) && vs_set.has(tgt));
    EXPECT_TRUE(crosses);
  }
}

// ============================================================================
// min_cut_weight Method Tests
// ============================================================================

TEST(StoerWagnerWeightOnly, BasicUsage)
{
  auto g = create_barbell(4);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  int weight = sw.min_cut_weight(g);

  EXPECT_EQ(weight, 1);
}

TEST(StoerWagnerWeightOnly, WeightedGraph)
{
  auto g = create_weighted_chain(5, 2, 8);
  Stoer_Wagner_Min_Cut<WeightedGraph> sw;

  int weight = sw.min_cut_weight(g);

  EXPECT_EQ(weight, 2);
}

TEST(StoerWagnerWeightOnly, MatchesFullComputation)
{
  auto g = create_complete_graph(6);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int full_result = sw(g, vs, vt, cut);
  int weight_only = sw.min_cut_weight(g);

  EXPECT_EQ(full_result, weight_only);
}

TEST(StoerWagnerWeightOnly, SmallGraph)
{
  IntGraph g;
  g.insert_node(0);
  g.insert_node(1);
  // No edges

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  int weight = sw.min_cut_weight(g);

  EXPECT_EQ(weight, 0);
}

// ============================================================================
// Unit_Weight Functor Tests
// ============================================================================

TEST(StoerWagnerUnitWeight, IgnoresArcWeights)
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  // High weights that should be ignored
  g.insert_arc(n0, n1, 999);
  g.insert_arc(n1, n2, 888);

  Stoer_Wagner_Min_Cut<IntGraph, Unit_Weight<IntGraph>> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  size_t min_cut = sw(g, vs, vt, cut);

  // Path with unit weights: min-cut = 1
  EXPECT_EQ(min_cut, 1u);
}

TEST(StoerWagnerUnitWeight, CountsEdges)
{
  auto g = create_complete_graph(5);
  Stoer_Wagner_Min_Cut<IntGraph, Unit_Weight<IntGraph>> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  size_t min_cut = sw(g, vs, vt, cut);

  // K5 with unit weights: isolate one vertex = 4 edges
  EXPECT_EQ(min_cut, 4u);
}

// ============================================================================
// Custom Distance Functor Tests
// ============================================================================

TEST(StoerWagnerCustomDistance, DoubleWeight)
{
  struct DoubleWeight
  {
    using Distance_Type = int;
    int operator()(IntGraph::Arc* a) const { return a->get_info() * 2; }
  };

  auto g = create_path(3);  // 0-1-2 with weight 1 each
  Stoer_Wagner_Min_Cut<IntGraph, DoubleWeight> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Original weight 1, doubled = 2
  EXPECT_EQ(min_cut, 2);
}

TEST(StoerWagnerCustomDistance, ConstantWeight)
{
  struct ConstWeight
  {
    using Distance_Type = int;
    int operator()(IntGraph::Arc*) const { return 7; }
  };

  auto g = create_triangle();
  Stoer_Wagner_Min_Cut<IntGraph, ConstWeight> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Triangle min-cut: 2 edges * constant 7 = 14
  EXPECT_EQ(min_cut, 14);
}

// ============================================================================
// Arc Filter Tests
// ============================================================================

template <class GT>
struct WeightFilter
{
  int threshold;

  WeightFilter(int t = 5) : threshold(t) {}

  bool operator()(typename GT::Arc* a) const
  {
    return a->get_info() <= threshold;
  }
};

TEST(StoerWagnerArcFilter, FiltersByWeight)
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  // Path: 0 -1- 1 -1- 2 -1- 3
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);

  // High-weight edge that should be filtered
  g.insert_arc(n0, n3, 10);

  WeightFilter<IntGraph> filter(5);
  Stoer_Wagner_Min_Cut<IntGraph, Dft_Dist<IntGraph>, WeightFilter<IntGraph>> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Without filter: n0-n3 edge would create a cycle
  // With filter: graph is a path, min-cut = 1
  EXPECT_EQ(min_cut, 1);
}

// ============================================================================
// Different Graph Types
// ============================================================================

TEST(StoerWagnerGraphTypes, ListSGraph)
{
  SGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n0, n2, 1);

  Stoer_Wagner_Min_Cut<SGraph> sw;
  DynList<SGraph::Node*> vs, vt;
  DynList<SGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 2);
}

TEST(StoerWagnerGraphTypes, DoubleWeights)
{
  DoubleGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.5);
  g.insert_arc(n1, n2, 2.5);
  g.insert_arc(n0, n2, 0.5);

  struct DoubleDistance
  {
    using Distance_Type = double;
    double operator()(DoubleGraph::Arc* a) const { return a->get_info(); }
  };

  Stoer_Wagner_Min_Cut<DoubleGraph, DoubleDistance> sw;
  DynList<DoubleGraph::Node*> vs, vt;
  DynList<DoubleGraph::Arc*> cut;

  double min_cut = sw(g, vs, vt, cut);

  // Min-cut: isolate n2 via edge 0.5 + 2.5 = 3.0
  // Or isolate n0 via edge 1.5 + 0.5 = 2.0
  // Or isolate n1 via edge 1.5 + 2.5 = 4.0
  EXPECT_DOUBLE_EQ(min_cut, 2.0);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(StoerWagnerEdgeCases, ZeroWeightEdge)
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);  // Zero weight
  g.insert_arc(n1, n2, 5);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // Min-cut should use zero-weight edge
  EXPECT_EQ(min_cut, 0);
}

TEST(StoerWagnerEdgeCases, AllSameWeight)
{
  auto g = create_complete_graph(4);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // K4 min-cut = 3 (all edges weight 1)
  EXPECT_EQ(min_cut, 3);
}

TEST(StoerWagnerEdgeCases, LargeWeights)
{
  IntGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1000000);
  g.insert_arc(n1, n2, 1);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 1);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(StoerWagnerPerformance, MediumGraph50Nodes)
{
  IntGraph g;
  const int N = 50;
  vector<IntGraph::Node*> nodes(N);

  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Create a connected graph with path + extra edges
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  // Add some cross edges
  for (int i = 0; i < N; i += 5)
    for (int j = i + 10; j < N; j += 10)
      g.insert_arc(nodes[i], nodes[j], 1);

  Stoer_Wagner_Min_Cut<IntGraph> sw;
  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  EXPECT_NO_THROW(sw(g, vs, vt, cut));
  EXPECT_GT(vs.size(), 0u);
  EXPECT_GT(vt.size(), 0u);
}

TEST(StoerWagnerPerformance, DenseGraph20Nodes)
{
  auto g = create_complete_graph(20);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs, vt;
  DynList<IntGraph::Arc*> cut;

  int min_cut = sw(g, vs, vt, cut);

  // K20 min-cut = 19
  EXPECT_EQ(min_cut, 19);
}

// ============================================================================
// Determinism Test
// ============================================================================

TEST(StoerWagnerDeterminism, SameResultOnMultipleCalls)
{
  auto g = create_complete_graph(8);
  Stoer_Wagner_Min_Cut<IntGraph> sw;

  DynList<IntGraph::Node*> vs1, vt1, vs2, vt2;
  DynList<IntGraph::Arc*> cut1, cut2;

  int result1 = sw(g, vs1, vt1, cut1);
  int result2 = sw(g, vs2, vt2, cut2);

  // Stoer-Wagner is deterministic
  EXPECT_EQ(result1, result2);
  EXPECT_EQ(cut1.size(), cut2.size());
}