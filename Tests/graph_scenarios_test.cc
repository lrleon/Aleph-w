
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
 * @file graph_scenarios_test.cc
 * @brief Tests for Graph Scenarios
 */

#include <gtest/gtest.h>

#include <Bellman_Ford.H>
#include <Dijkstra.H>
#include <Tarjan.H>
#include <random_graph.H>
#include <tpl_graph.H>

#include <cstdlib>
#include <limits>
#include <random>
#include <vector>

using namespace Aleph;

namespace
{
using DGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using DNode = DGraph::Node;

template <class GT, class SA>
class Out_Iterator_Ne : public Out_Iterator<GT, SA>
{
public:
  using Base = Out_Iterator<GT, SA>;
  using Base::Base;

  typename GT::Arc * get_current_arc_ne() const noexcept
  {
    return const_cast<Out_Iterator_Ne *>(this)->get_current_arc();
  }
};

struct BinHeapTag
{
  template <class G, class D, class A>
  using Heap = ArcHeap<G, D, A>;
};

using DijkstraInt =
  Dijkstra_Min_Paths<DGraph, Dft_Dist<DGraph>, Node_Arc_Iterator,
                     Dft_Show_Arc<DGraph>, BinHeapTag::template Heap>;

using DijkstraIntAllArcs =
  Dijkstra_Min_Paths<DGraph, Dft_Dist<DGraph>, Out_Iterator_Ne,
                     Dft_Show_Arc<DGraph>, BinHeapTag::template Heap>;

struct InitNodeNoop
{
  void operator()(DGraph &, DNode *) const noexcept {}
};

struct InitArcRandNonNegative
{
  std::mt19937 rng;
  std::uniform_int_distribution<int> dist;

  explicit InitArcRandNonNegative(uint32_t seed, int max_w)
    : rng(seed), dist(0, max_w)
  {
  }

  void operator()(DGraph &, DGraph::Arc *a)
  {
    a->get_info() = dist(rng);
  }
};

struct InitArcRandAllowNegative
{
  std::mt19937 rng;
  std::uniform_int_distribution<int> dist;

  explicit InitArcRandAllowNegative(uint32_t seed, int min_w, int max_w)
    : rng(seed), dist(min_w, max_w)
  {
  }

  void operator()(DGraph &, DGraph::Arc *a)
  {
    a->get_info() = dist(rng);
  }
};

static DNode * pick_node_by_index(DGraph & g, size_t idx)
{
  size_t i = 0;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne(), ++i)
    if (i == idx)
      return it.get_curr();
  return nullptr;
}

static std::pair<DNode *, DNode *> pick_two_distinct_nodes(DGraph & g,
                                                          uint32_t seed)
{
  std::mt19937 rng(seed);
  std::uniform_int_distribution<size_t> dist(0, g.get_num_nodes() - 1);

  const size_t i = dist(rng);
  size_t j = dist(rng);
  while (j == i)
    j = dist(rng);

  auto *s = pick_node_by_index(g, i);
  auto *t = pick_node_by_index(g, j);
  return {s, t};
}

void expect_dijkstra_matches_bellman_ford(DGraph & g, DNode *s, DNode *t)
{
  ASSERT_NE(s, nullptr);
  ASSERT_NE(t, nullptr);

  // Dijkstra
  DijkstraInt dij;
  Path<DGraph> dij_path(g);
  const int dij_dist = dij(g, s, t, dij_path);

  // Bellman-Ford (no negative weights/cycles in this test)
  Bellman_Ford<DGraph> bf(g);
  const bool neg = bf.paint_spanning_tree(s);
  ASSERT_FALSE(neg);

  Path<DGraph> bf_path(g);
  const int bf_dist = bf.get_min_path(t, bf_path);

  // Both implementations use max() as INF on unreachable.
  EXPECT_EQ(dij_dist, bf_dist);

  // If reachable, paths must be non-empty and end at t.
  if (bf_dist != std::numeric_limits<int>::max())
    {
      EXPECT_FALSE(dij_path.is_empty());
      EXPECT_FALSE(bf_path.is_empty());

      EXPECT_EQ(dij_path.get_last_node(), t);
      EXPECT_EQ(bf_path.get_last_node(), t);
    }
  else
    {
      EXPECT_TRUE(dij_path.is_empty());
      EXPECT_TRUE(bf_path.is_empty());
    }
}
} // namespace

TEST(GraphScenarios, DisconnectedGraph_UnreachablePath)
{
  DGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  auto *c = g.insert_node(3);
  auto *d = g.insert_node(4);

  // Component 1: a -> b
  g.insert_arc(a, b, 5);

  // Component 2: c -> d
  g.insert_arc(c, d, 7);

  DijkstraInt dij;
  Path<DGraph> path(g);
  const int dist = dij(g, a, d, path);

  EXPECT_EQ(dist, std::numeric_limits<int>::max());
  EXPECT_TRUE(path.is_empty());
}

TEST(GraphScenarios, Multigraph_ParallelArcs_DijkstraChoosesMin)
{
  DGraph g;
  auto *s = g.insert_node(0);
  auto *t = g.insert_node(1);

  const auto arcs_before = g.get_num_arcs();

  g.insert_arc(s, t, 10);
  g.insert_arc(s, t, 3);
  g.insert_arc(s, t, 7);

  // Some graph types in Aleph-w behave as simple graphs and may collapse
  // parallel arcs. If so, skip this multigraph-specific expectation.
  if (const auto arcs_after = g.get_num_arcs(); arcs_after - arcs_before < 3)
    GTEST_SKIP() << "Graph type does not appear to support parallel arcs (multigraph).";

  // Important: for multigraphs, use an iterator that visits all arcs.
  // Some iterators may collapse parallel arcs by target node.
  DijkstraIntAllArcs dij;
  Path<DGraph> path(g);
  const int dist = dij(g, s, t, path);

  EXPECT_EQ(dist, 3);
  EXPECT_FALSE(path.is_empty());
}

TEST(GraphScenarios, BellmanFord_NegativeWeights_NoNegativeCycle)
{
  DGraph g;
  auto *n0 = g.insert_node(0);
  auto *n1 = g.insert_node(1);
  auto *n2 = g.insert_node(2);
  auto *n3 = g.insert_node(3);

  // No negative cycle: best path 0->1->2->3 = -2 + 3 + -1 = 0
  g.insert_arc(n0, n1, -2);
  g.insert_arc(n1, n2, 3);
  g.insert_arc(n2, n3, -1);
  g.insert_arc(n0, n3, 5);

  Bellman_Ford<DGraph> bf(g);
  const bool neg = bf.paint_spanning_tree(n0);
  EXPECT_FALSE(neg);

  Path<DGraph> path(g);
  const int dist = bf.get_min_path(n3, path);
  EXPECT_EQ(dist, 0);
  EXPECT_FALSE(path.is_empty());
}

TEST(GraphScenarios, SparseVsDense_DijkstraMatchesBellmanFord)
{
  // Keep these sizes modest; this is a correctness test, not a perf test.
  const size_t N = 200;

  // Sparse: ~4N arcs
  {
    InitNodeNoop init_node;
    InitArcRandNonNegative init_arc(1234, 20);

    Random_Digraph<DGraph, InitNodeNoop, InitArcRandNonNegative> gen(
        777, init_node, init_arc);

    auto g = gen(N, size_t(4 * N), false /* strongly connected? */);

    auto [s, t] = pick_two_distinct_nodes(g, 999);
    expect_dijkstra_matches_bellman_ford(g, s, t);
  }

  // Dense: probability p.
  {
    InitNodeNoop init_node;
    InitArcRandNonNegative init_arc(4321, 20);

    Random_Digraph<DGraph, InitNodeNoop, InitArcRandNonNegative> gen(
        888, init_node, init_arc);

    auto g = gen(N, 0.08, false /* strongly connected? */);

    auto [s, t] = pick_two_distinct_nodes(g, 1001);
    expect_dijkstra_matches_bellman_ford(g, s, t);
  }
}

TEST(GraphScenarios, MultiGraph_TarjanCycleDetection)
{
  DGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);

  // Parallel arcs alone do not create a cycle.
  g.insert_arc(a, b, 1);
  g.insert_arc(a, b, 2);

  Tarjan_Connected_Components<DGraph> tarjan;
  EXPECT_FALSE(tarjan.has_cycle(g));

  // Add one back edge: now there is a cycle.
  g.insert_arc(b, a, 0);
  Tarjan_Connected_Components<DGraph> tarjan2;
  EXPECT_TRUE(tarjan2.has_cycle(g));
}
