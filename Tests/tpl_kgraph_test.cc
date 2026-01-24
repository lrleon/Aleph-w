
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
                                    const DynSetTree<typename GT::Node *> &l,
                                    const DynSetTree<typename GT::Node *> &r,
                                    const DynDlist<typename GT::Arc *> &cut,
                                    size_t expected_cut_size)
  {
    EXPECT_EQ(l.size() + r.size(), g.get_num_nodes());

    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto node = it.get_curr();
        const bool in_l = l.contains(node);
        const bool in_r = r.contains(node);
        EXPECT_NE(in_l, in_r);
      }

    for (Arc_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto src = g.get_src_node(arc);
        auto tgt = g.get_tgt_node(arc);
        const bool src_in_l = l.contains(src);
        const bool tgt_in_l = l.contains(tgt);
        const bool crosses = (src_in_l != tgt_in_l);
        EXPECT_EQ(arc_in_cut<GT>(cut, arc), crosses);
      }

    for (typename DynDlist<typename GT::Arc *>::Iterator it(cut);
         it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto src = g.get_src_node(arc);
        auto tgt = g.get_tgt_node(arc);
        const bool src_in_l = l.contains(src);
        const bool tgt_in_l = l.contains(tgt);
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
  DynSetTree<Graph::Node *> l;
  DynSetTree<Graph::Node *> r;
  DynDlist<Graph::Arc *> cut;

  const long min_cut = compute_min_cut<Graph, Heap_Preflow_Maximum_Flow>(g, l, r, cut);
  EXPECT_EQ(min_cut, 0L);
  expect_cut_matches_partition(g, l, r, cut, 0U);
}

TEST(KGraphMinCut, SingleNodeTrivialCut)
{
  Graph g;
  g.insert_node(1);
  DynSetTree<Graph::Node *> l;
  DynSetTree<Graph::Node *> r;
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
  DynSetTree<Graph::Node *> l;
  DynSetTree<Graph::Node *> r;
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
  DynSetTree<Graph::Node *> l;
  DynSetTree<Graph::Node *> r;
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
  DynSetTree<Graph::Node *> l;
  DynSetTree<Graph::Node *> r;
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
  DynSetTree<Graph::Node *> l;
  DynSetTree<Graph::Node *> r;
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

  DynSetTree<TestDigraph::Node *> l;
  DynSetTree<TestDigraph::Node *> r;
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
