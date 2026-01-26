
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
 * @file graph_stress_test.cc
 * @brief Tests for Graph Stress
 */

#include <gtest/gtest.h>

#include <Dijkstra.H>
#include <tpl_graph.H>

#include <cstdlib>
#include <limits>
#include <vector>

using namespace Aleph;

namespace
{
using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using Node = Graph::Node;

struct BinHeapTag
{
  template <class G, class D, class A>
  using Heap = ArcHeap<G, D, A>;
};

using DijkstraInt =
  Dijkstra_Min_Paths<Graph, Dft_Dist<Graph>, Node_Arc_Iterator,
                     Dft_Show_Arc<Graph>, BinHeapTag::template Heap>;

static bool should_run_stress()
{
  return std::getenv("ALEPH_RUN_GRAPH_STRESS") != nullptr;
}

static size_t stress_nodes()
{
  const char *env = std::getenv("ALEPH_GRAPH_STRESS_NODES");
  if (env == nullptr)
    return 1000000;

  char *end = nullptr;
  const unsigned long long v = std::strtoull(env, &end, 10);
  if (end == env || *end != '\0')
    return 1000000;

  return static_cast<size_t>(v);
}
} // namespace

TEST(GraphStress, MillionNodeChain_DijkstraPaintOnly)
{
  if (!should_run_stress())
    GTEST_SKIP() << "Set ALEPH_RUN_GRAPH_STRESS=1 (and optionally ALEPH_GRAPH_STRESS_NODES=N) to enable.";

  const size_t N = stress_nodes();
  ASSERT_GE(N, 2u);

  Graph g;
  std::vector<Node *> nodes;
  nodes.reserve(N);

  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  for (size_t i = 0; i + 1 < N; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  DijkstraInt dij;
  const bool found = dij.paint_partial_min_paths_tree(g, nodes.front(), nodes.back());
  ASSERT_TRUE(found);

  const int dist = dij.get_distance(nodes.back());
  ASSERT_EQ(dist, static_cast<int>(N - 1));
}
