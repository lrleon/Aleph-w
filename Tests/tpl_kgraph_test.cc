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
 * @file tpl_kgraph_test.cc
 * @brief Tests for tpl_kgraph.H
 *
 * Covers:
 * - edge_connectivity()
 * - compute_min_cut()
 * - vertex_connectivity()
 */

#include <gtest/gtest.h>

#include <vector>

#include <tpl_graph.H>
#include <tpl_kgraph.H>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using TestDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Digraph = TestDigraph;

  std::vector<Graph::Node *> make_nodes(Graph &g, int n)
  {
    std::vector<Graph::Node *> nodes;
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    return nodes;
  }

  void make_cycle(Graph &g, const std::vector<Graph::Node *> &nodes)
  {
    if (nodes.empty())
      return;

    const size_t n = nodes.size();
    for (size_t i = 0; i < n; ++i)
      g.insert_arc(nodes[i], nodes[(i + 1) % n]);
  }

  void make_complete(Graph &g, const std::vector<Graph::Node *> &nodes)
  {
    for (size_t i = 0; i < nodes.size(); ++i)
      for (size_t j = i + 1; j < nodes.size(); ++j)
        g.insert_arc(nodes[i], nodes[j]);
  }

  template <class GT>
  bool arc_in_cut(const DynDlist<typename GT::Arc *> &cut,
                  typename GT::Arc *arc)
  {
    for (typename DynDlist<typename GT::Arc *>::Iterator it(cut);
         it.has_curr(); it.next_ne())
      if (it.get_curr() == arc)
        return true;
    return false;
  }

  template <class GT>
  void expect_cut_matches_partition(const GT &g,
                                    const Aleph::set<typename GT::Node *> &l,
                                    const Aleph::set<typename GT::Node *> &r,
                                    const DynDlist<typename GT::Arc *> &cut,
                                    size_t expected_cut_size)
  {
    EXPECT_EQ(l.size() + r.size(), g.get_num_nodes());

    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto node = it.get_curr();
        const bool in_l = l.count(node) != 0;
        const bool in_r = r.count(node) != 0;
        EXPECT_NE(in_l, in_r);
      }

    for (Arc_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto src = g.get_src_node(arc);
        auto tgt = g.get_tgt_node(arc);
        const bool src_in_l = l.count(src) != 0;
        const bool tgt_in_l = l.count(tgt) != 0;
        const bool crosses = (src_in_l != tgt_in_l);
        EXPECT_EQ(arc_in_cut<GT>(cut, arc), crosses);
      }

    for (typename DynDlist<typename GT::Arc *>::Iterator it(cut);
         it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto src = g.get_src_node(arc);
        auto tgt = g.get_tgt_node(arc);
        const bool src_in_l = l.count(src) != 0;
        const bool tgt_in_l = l.count(tgt) != 0;
        EXPECT_NE(src_in_l, tgt_in_l);
      }

    EXPECT_EQ(cut.size(), expected_cut_size);
  }
} // namespace

TEST(KGraphEdgeConnectivity, EmptyGraphReturnsZero)
{
  Graph g;
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 0L);
}

TEST(KGraphEdgeConnectivity, SingleNodeReturnsZero)
{
  Graph g;
  g.insert_node(1);
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 0L);
}

TEST(KGraphEdgeConnectivity, SingleEdgeReturnsOne)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 1L);
}

TEST(KGraphEdgeConnectivity, DisconnectedGraphReturnsZero)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[2], nodes[3]);
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 0L);
}

TEST(KGraphEdgeConnectivity, CycleReturnsTwo)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  make_cycle(g, nodes);
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 2L);
}

TEST(KGraphEdgeConnectivity, CompleteGraphReturnsNMinusOne)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  make_complete(g, nodes);
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 3L);
}

TEST(KGraphEdgeConnectivity, ParallelEdgesIncreaseConnectivity)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[1]);
  const auto k = edge_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 2L);
}

TEST(KGraphMinCut, EmptyGraphProducesEmptyPartition)
{
  Graph g;
  Aleph::set<Graph::Node *> l;
  Aleph::set<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 0L);
  expect_cut_matches_partition(g, l, r, cut, 0U);
}

TEST(KGraphMinCut, SingleNodeTrivialCut)
{
  Graph g;
  g.insert_node(1);
  Aleph::set<Graph::Node *> l;
  Aleph::set<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 0L);
  expect_cut_matches_partition(g, l, r, cut, 0U);
}

TEST(KGraphMinCut, SingleEdgeCut)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);
  Aleph::set<Graph::Node *> l;
  Aleph::set<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 1L);
  expect_cut_matches_partition(g, l, r, cut, static_cast<size_t>(min_cut));
}

TEST(KGraphMinCut, DisconnectedGraphReturnsZero)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[2], nodes[3]);
  Aleph::set<Graph::Node *> l;
  Aleph::set<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 0L);
  expect_cut_matches_partition(g, l, r, cut, 0U);
}

TEST(KGraphMinCut, CycleReturnsTwo)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  make_cycle(g, nodes);
  Aleph::set<Graph::Node *> l;
  Aleph::set<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 2L);
  expect_cut_matches_partition(g, l, r, cut, static_cast<size_t>(min_cut));
}

TEST(KGraphMinCut, ParallelEdgesReturnFullCut)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[1]);
  Aleph::set<Graph::Node *> l;
  Aleph::set<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 2L);
  expect_cut_matches_partition(g, l, r, cut, static_cast<size_t>(min_cut));
}

TEST(KGraphVertexConnectivity, EmptyGraphReturnsZero)
{
  Graph g;
  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 0L);
}

TEST(KGraphVertexConnectivity, SingleNodeReturnsZero)
{
  Graph g;
  g.insert_node(1);
  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 0L);
}

TEST(KGraphVertexConnectivity, TwoNodesOneEdgeReturnsOne)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);
  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 1L);
}

TEST(KGraphVertexConnectivity, DisconnectedGraphReturnsZero)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[2], nodes[3]);
  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 0L);
}

TEST(KGraphVertexConnectivity, CycleReturnsTwo)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  make_cycle(g, nodes);
  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 2L);
}

TEST(KGraphVertexConnectivity, CompleteGraphReturnsNMinusOne)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  make_complete(g, nodes);
  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 3L);
}

TEST(KGraphVertexConnectivity, BowtieGraphHasConnectivityOne)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[0]);
  g.insert_arc(nodes[0], nodes[3]);
  g.insert_arc(nodes[3], nodes[4]);
  g.insert_arc(nodes[4], nodes[0]);

  const auto k = vertex_connectivity<Graph, Heap_Preflow_Maximum_Flow>(g);
  EXPECT_EQ(k, 1L);
}

TEST(KGraphPreconditions, RejectsDigraphs)
{
  TestDigraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);

  Aleph::set<TestDigraph::Node *> l;
  Aleph::set<TestDigraph::Node *> r;
  DynDlist<TestDigraph::Arc *> cut;

  auto edge_conn_call = [&]() {
    return edge_connectivity<TestDigraph, Heap_Preflow_Maximum_Flow>(g);
  };
  auto min_cut_call = [&]() {
    return compute_min_cut<TestDigraph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  };
  auto vertex_conn_call = [&]() {
    return vertex_connectivity<TestDigraph, Heap_Preflow_Maximum_Flow>(g);
  };

  EXPECT_THROW(edge_conn_call(), std::domain_error);
  EXPECT_THROW(min_cut_call(), std::domain_error);
  EXPECT_THROW(vertex_conn_call(), std::domain_error);
}
