
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
 * @file tpl_graph_utils_test.cc
 * @brief Tests for tpl_graph_utils.H
 *
 * These tests cover:
 * - Depth/breadth traversals (free functions and functors)
 * - Path search (BFS)
 * - Connectivity, reachability, cycle checks
 * - Connected components extraction and mapping
 * - Spanning trees (DFS/BFS) and mapping
 * - Cut nodes / painting / cut graph extraction
 * - Digraph inversion (transpose) and mapping
 * - get_min_path() reconstruction and Total_Cost aggregation
 */

#include <gtest/gtest.h>

#include <functional>
#include <queue>
#include <random>
#include <set>
#include <tuple>
#include <vector>

#include <tpl_dynArray.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using TestDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using WGraph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;

  std::vector<Graph::Node *> make_nodes(Graph &g, int n)
  {
    std::vector<Graph::Node *> nodes;
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    return nodes;
  }

  std::vector<TestDigraph::Node *> make_nodes(TestDigraph &g, int n)
  {
    std::vector<TestDigraph::Node *> nodes;
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    return nodes;
  }

  std::vector<WGraph::Node *> make_nodes(WGraph &g, int n)
  {
    std::vector<WGraph::Node *> nodes;
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    return nodes;
  }

  // ---------------------------------------------------------------------------
  // Helpers for function-pointer traversals
  // ---------------------------------------------------------------------------

  Graph::Node *g_expected_start = nullptr;

  bool check_from_arc_is_null_for_start(const Graph &, Graph::Node *node, Graph::Arc *from)
  {
    if (node == g_expected_start)
      EXPECT_EQ(from, nullptr);
    else
      EXPECT_NE(from, nullptr);
    return false;
  }

  bool stop_immediately(const Graph &, Graph::Node *, Graph::Arc *)
  {
    return true;
  }

  // ---------------------------------------------------------------------------
  // Operation & filter helpers for traversal functors
  // ---------------------------------------------------------------------------

  template <class GT>
  struct CollectVisitedInfos
  {
    std::vector<int> *infos = nullptr;

    bool operator()(const GT &, typename GT::Node *node, typename GT::Arc *)
    {
      infos->push_back(node->get_info());
      return false;
    }
  };

  template <class GT>
  struct EvenArcInfoOnly
  {
    bool operator()(typename GT::Arc *a) const noexcept
    {
      return (a->get_info() % 2) == 0;
    }
  };

  // ---------------------------------------------------------------------------
  // Reference BFS on adjacency list (undirected) for property tests
  // ---------------------------------------------------------------------------

  int bfs_distance(const std::vector<std::vector<int>> &adj, int s, int t)
  {
    if (s == t)
      return 0;

    std::vector<int> dist(adj.size(), -1);
    std::queue<int> q;
    dist[s] = 0;
    q.push(s);

    while (!q.empty())
      {
        int u = q.front();
        q.pop();
        for (int v : adj[u])
          {
            if (dist[v] != -1)
              continue;
            dist[v] = dist[u] + 1;
            if (v == t)
              return dist[v];
            q.push(v);
          }
      }

    return -1;
  }

  template <class GT>
  std::vector<typename GT::Node *> path_nodes(const Path<GT> &path)
  {
    std::vector<typename GT::Node *> nodes;
    path.for_each_node([&](auto p) { nodes.push_back(p); });
    return nodes;
  }

} // namespace

// =============================================================================
// Traversals (free functions)
// =============================================================================

TEST(GraphUtilsTraversal, DepthFirstTraversalCountsReachableNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);

  const auto no_visit =
      static_cast<bool (*)(const Graph &, Graph::Node *, Graph::Arc *)>(nullptr);
  const size_t visited = depth_first_traversal(g, nodes[0], no_visit);
  EXPECT_EQ(visited, 5U);

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    EXPECT_TRUE(IS_NODE_VISITED(it.get_curr(), Depth_First));
}

TEST(GraphUtilsTraversal, DepthFirstTraversalProvidesFromArc)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  g_expected_start = nodes[0];
  depth_first_traversal(g, nodes[0], &check_from_arc_is_null_for_start);
}

TEST(GraphUtilsTraversal, DepthFirstTraversalStopsImmediately)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  const size_t visited = depth_first_traversal(g, nodes[0], &stop_immediately);
  EXPECT_EQ(visited, 1U);
}

TEST(GraphUtilsTraversal, BreadthFirstTraversalCountsReachableNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);

  const auto no_visit =
      static_cast<bool (*)(const Graph &, Graph::Node *, Graph::Arc *)>(nullptr);
  const size_t visited = breadth_first_traversal(g, nodes[0], no_visit);
  EXPECT_EQ(visited, 5U);

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    EXPECT_TRUE(IS_NODE_VISITED(it.get_curr(), Breadth_First));
}

TEST(GraphUtilsTraversal, BreadthFirstTraversalProvidesFromArc)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  g_expected_start = nodes[0];
  breadth_first_traversal(g, nodes[0], &check_from_arc_is_null_for_start);
}

TEST(GraphUtilsTraversal, BreadthFirstTraversalStopsImmediately)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  const size_t visited = breadth_first_traversal(g, nodes[0], &stop_immediately);
  EXPECT_EQ(visited, 1U);
}

// =============================================================================
// Traversal functors (filters + operations)
// =============================================================================

TEST(GraphUtilsTraversal, DepthFirstTraversalFunctorHonorsArcFilter)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 2); // allowed (even)
  g.insert_arc(nodes[1], nodes[2], 1); // filtered out

  std::vector<int> visited_infos;
  CollectVisitedInfos<Graph> op{&visited_infos};

  Depth_First_Traversal<Graph, CollectVisitedInfos<Graph>, EvenArcInfoOnly<Graph>> dft{
      EvenArcInfoOnly<Graph>()};

  const size_t visited = dft(g, nodes[0], op);
  EXPECT_EQ(visited, 2U);

  std::set<int> s(visited_infos.begin(), visited_infos.end());
  EXPECT_EQ(s, (std::set<int>{0, 1}));
}

TEST(GraphUtilsTraversal, BreadthFirstTraversalFunctorHonorsArcFilter)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 2); // allowed (even)
  g.insert_arc(nodes[1], nodes[2], 1); // filtered out

  std::vector<int> visited_infos;
  CollectVisitedInfos<Graph> op{&visited_infos};

  Breadth_First_Traversal<Graph, CollectVisitedInfos<Graph>, EvenArcInfoOnly<Graph>> bft{
      EvenArcInfoOnly<Graph>()};

  const size_t visited = bft(g, nodes[0], op);
  EXPECT_EQ(visited, 2U);

  std::set<int> s(visited_infos.begin(), visited_infos.end());
  EXPECT_EQ(s, (std::set<int>{0, 1}));
}

// =============================================================================
// Path search
// =============================================================================

TEST(GraphUtilsPath, FindPathBreadthFirstStartEqualsEnd)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  auto path = find_path_breadth_first(g, nodes[1], nodes[1]);
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 1U);
  EXPECT_EQ(path.get_first_node(), nodes[1]);
  EXPECT_EQ(path.get_last_node(), nodes[1]);
}

TEST(GraphUtilsPath, FindPathBreadthFirstReturnsEmptyPathWhenUnreachable)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  auto path = find_path_breadth_first(g, nodes[0], nodes[3]);
  EXPECT_TRUE(path.is_empty());
  EXPECT_TRUE(path.inside_graph(g));
}

TEST(GraphUtilsPath, FindPathBreadthFirstFindsShortestByEdges)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[3], 1); // direct
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1); // longer

  auto path = find_path_breadth_first(g, nodes[0], nodes[3]);
  ASSERT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 2U);

  auto seq = path_nodes(path);
  ASSERT_EQ(seq.size(), 2U);
  EXPECT_EQ(seq.front(), nodes[0]);
  EXPECT_EQ(seq.back(), nodes[3]);
  EXPECT_NE(search_arc(g, seq[0], seq[1]), nullptr);
}

TEST(GraphUtilsPath, FindPathBreadthFirstRandomGraphsMatchesReferenceBfs)
{
  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> n_dist(2, 10);
  std::bernoulli_distribution edge_dist(0.25);
  std::uniform_int_distribution<int> pick_dist(0, 9);

  for (int iter = 0; iter < 200; ++iter)
    {
      const int n = n_dist(rng);

      Graph g;
      auto nodes = make_nodes(g, n);
      std::vector<std::vector<int>> adj(n);

      for (int u = 0; u < n; ++u)
        for (int v = u + 1; v < n; ++v)
          if (edge_dist(rng))
            {
              g.insert_arc(nodes[u], nodes[v], 1);
              adj[u].push_back(v);
              adj[v].push_back(u);
            }

      const int s = pick_dist(rng) % n;
      const int t = pick_dist(rng) % n;

      const int ref = bfs_distance(adj, s, t);
      auto path = find_path_breadth_first(g, nodes[s], nodes[t]);

      if (ref < 0)
        {
          EXPECT_TRUE(path.is_empty());
        }
      else
        {
          ASSERT_FALSE(path.is_empty());
          EXPECT_EQ(path.size(), static_cast<size_t>(ref + 1));

          auto seq = path_nodes(path);
          ASSERT_EQ(seq.front(), nodes[s]);
          ASSERT_EQ(seq.back(), nodes[t]);
          for (size_t i = 0; i + 1 < seq.size(); ++i)
            EXPECT_NE(search_arc(g, seq[i], seq[i + 1]), nullptr);
        }
    }
}

// =============================================================================
// Connectivity / cycles / reachability
// =============================================================================

TEST(GraphUtilsProperties, TestConnectivityBasics)
{
  Graph empty;
  EXPECT_FALSE(test_connectivity(empty));

  Graph single;
  (void)single.insert_node(1);
  EXPECT_TRUE(test_connectivity(single));

  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  EXPECT_FALSE(test_connectivity(g)); // node[3] disconnected

  g.insert_arc(nodes[2], nodes[3], 1);
  EXPECT_TRUE(test_connectivity(g));
}

TEST(GraphUtilsProperties, TestConnectivityRejectsDigraphs)
{
  TestDigraph dg;
  auto nodes = make_nodes(dg, 2);
  dg.insert_arc(nodes[0], nodes[1], 1);
  EXPECT_THROW(test_connectivity(dg), std::domain_error);
}

TEST(GraphUtilsProperties, TestForCycleBehavior)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  EXPECT_FALSE(test_for_cycle(g, nodes[0]));

  g.insert_arc(nodes[2], nodes[0], 1); // triangle
  EXPECT_TRUE(test_for_cycle(g, nodes[0]));

  EXPECT_THROW(test_for_cycle(g, nullptr), std::invalid_argument);
}

TEST(GraphUtilsProperties, AcyclicAndHasCycleAgreement)
{
  Graph empty;
  EXPECT_TRUE(is_graph_acyclique(empty));
  EXPECT_FALSE(has_cycle(empty));

  Graph tree;
  auto nodes = make_nodes(tree, 4);
  tree.insert_arc(nodes[0], nodes[1], 1);
  tree.insert_arc(nodes[1], nodes[2], 1);
  tree.insert_arc(nodes[2], nodes[3], 1);
  EXPECT_TRUE(is_graph_acyclique(tree));
  EXPECT_FALSE(has_cycle(tree));

  tree.insert_arc(nodes[3], nodes[0], 1);
  EXPECT_FALSE(is_graph_acyclique(tree));
  EXPECT_TRUE(has_cycle(tree));
}

TEST(GraphUtilsProperties, AcyclicRejectsDigraphs)
{
  TestDigraph dg;
  auto nodes = make_nodes(dg, 2);
  dg.insert_arc(nodes[0], nodes[1], 1);
  EXPECT_THROW(is_graph_acyclique(dg), std::domain_error);
  EXPECT_THROW(is_graph_acyclique(dg, nodes[0]), std::domain_error);
  EXPECT_THROW(is_graph_acyclique(dg, nullptr), std::domain_error);
}

TEST(GraphUtilsProperties, TestForPathBasicAndRegression)
{
  Graph g;
  auto nodes = make_nodes(g, 6);

  // Two disjoint triangles (E == V) => there is no path across components.
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);

  g.insert_arc(nodes[3], nodes[4], 1);
  g.insert_arc(nodes[4], nodes[5], 1);
  g.insert_arc(nodes[5], nodes[3], 1);

  EXPECT_FALSE(test_for_path(g, nodes[0], nodes[5]));
  EXPECT_TRUE(test_for_path(g, nodes[0], nodes[0]));

  EXPECT_THROW(test_for_path(g, nullptr, nodes[0]), std::invalid_argument);
  EXPECT_THROW(test_for_path(g, nodes[0], nullptr), std::invalid_argument);
}

// =============================================================================
// Connected components and subgraph mapping
// =============================================================================

TEST(GraphUtilsComponents, InconnectedComponentsReturnsMappedSubgraphs)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 1);

  auto comps = inconnected_components(g);

  std::multiset<std::pair<size_t, size_t>> sizes;
  size_t comp_count = 0;
  for (auto it = comps.get_it(); it.has_curr(); it.next_ne())
    {
      ++comp_count;
      auto &sg = it.get_curr();
      sizes.insert({sg.get_num_nodes(), sg.get_num_arcs()});
    }

  EXPECT_EQ(comp_count, 2U);
  EXPECT_EQ(sizes, (std::multiset<std::pair<size_t, size_t>>{{3U, 2U}, {1U, 0U}}));

  // Nodes and arcs must be mapped both ways through cookies.
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto gp = it.get_curr();
      auto sp = mapped_node<Graph>(gp);
      ASSERT_NE(sp, nullptr);
      EXPECT_EQ(sp->get_info(), gp->get_info());
      EXPECT_EQ(mapped_node<Graph>(sp), gp);
    }

  ASSERT_NE(mapped_arc<Graph>(a01), nullptr);
  ASSERT_NE(mapped_arc<Graph>(a12), nullptr);
  EXPECT_EQ(mapped_arc<Graph>(mapped_arc<Graph>(a01)), a01);
  EXPECT_EQ(mapped_arc<Graph>(mapped_arc<Graph>(a12)), a12);
}

TEST(GraphUtilsComponents, BuildSubgraphCopiesReachableComponent)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  // node[3] is isolated

  g.reset_nodes();
  g.reset_arcs();

  Graph sg;
  size_t visited = 0;
  build_subgraph(g, sg, nodes[0], visited);

  EXPECT_EQ(visited, 3U);
  EXPECT_EQ(sg.get_num_nodes(), 3U);
  EXPECT_EQ(sg.get_num_arcs(), 2U);
}

// =============================================================================
// Spanning trees
// =============================================================================

TEST(GraphUtilsSpanningTrees, DepthFirstSpanningTreeIsTreeAndMapped)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 2);
  g.insert_arc(nodes[2], nodes[3], 3);
  g.insert_arc(nodes[3], nodes[4], 4);
  g.insert_arc(nodes[0], nodes[4], 5); // add a cycle in the original graph

  auto tree = find_depth_first_spanning_tree(g, nodes[0]);
  EXPECT_EQ(tree.get_num_nodes(), g.get_num_nodes());
  EXPECT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);
  EXPECT_TRUE(is_graph_acyclique(tree));

  auto img = mapped_node<Graph>(nodes[0]);
  ASSERT_NE(img, nullptr);
  EXPECT_EQ(mapped_node<Graph>(img), nodes[0]);
}

TEST(GraphUtilsSpanningTrees, BreadthFirstSpanningTreeIsTreeAndMapped)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);

  g.reset_nodes();
  g.reset_arcs();

  auto tree = find_breadth_first_spanning_tree(g, nodes[0]);
  EXPECT_EQ(tree.get_num_nodes(), g.get_num_nodes());
  EXPECT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);
  EXPECT_TRUE(is_graph_acyclique(tree));
}

// =============================================================================
// Build spanning tree from arcs (DynArray)
// =============================================================================

TEST(GraphUtilsSpanningTrees, BuildSpanningTreeFromArcsUsesOneWayMapping)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 2);
  auto a23 = g.insert_arc(nodes[2], nodes[3], 3);
  (void)g.insert_arc(nodes[0], nodes[3], 99);

  g.reset_nodes();
  g.reset_arcs();

  DynArray<Graph::Arc *> arcs;
  arcs.append(a01);
  arcs.append(nullptr); // should be ignored
  arcs.append(a12);
  arcs.append(a23);

  auto tree = build_spanning_tree<Graph>(arcs);
  EXPECT_EQ(tree.get_num_nodes(), 4U);
  EXPECT_EQ(tree.get_num_arcs(), 3U);

  // Mapping is ret -> original via cookies.
  for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto tp = it.get_curr();
      auto gp = static_cast<Graph::Node *>(NODE_COOKIE(tp));
      ASSERT_NE(gp, nullptr);
      EXPECT_EQ(gp->get_info(), tp->get_info());
    }

  for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto ta = it.get_curr();
      auto ga = static_cast<Graph::Arc *>(ARC_COOKIE(ta));
      ASSERT_NE(ga, nullptr);
      EXPECT_EQ(ta->get_info(), ga->get_info());
    }

  // Original graph cookies must remain untouched.
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    EXPECT_EQ(NODE_COOKIE(it.get_curr()), nullptr);
}

// =============================================================================
// Cut nodes / painting / cut graph extraction
// =============================================================================

TEST(GraphUtilsCutNodes, ComputeCutNodesClearsCookiesAndFindsArticulations)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  auto cut_nodes = compute_cut_nodes(g, nodes[0]);

  std::set<int> infos;
  for (auto it = cut_nodes.get_it(); it.has_curr(); it.next_ne())
    infos.insert(it.get_curr()->get_info());

  EXPECT_EQ(infos, (std::set<int>{1, 2}));

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    EXPECT_EQ(NODE_COOKIE(it.get_curr()), nullptr);

  // Digraphs are rejected.
  TestDigraph dg;
  auto dn = make_nodes(dg, 2);
  dg.insert_arc(dn[0], dn[1], 1);
  EXPECT_THROW(compute_cut_nodes(dg, dn[0]), std::domain_error);
}

TEST(GraphUtilsCutNodes, PaintAndExtractCutGraphOnChain)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 1);
  auto a23 = g.insert_arc(nodes[2], nodes[3], 1);

  auto cut_nodes = compute_cut_nodes(g, nodes[0]);
  const long colors_end = paint_subgraphs(g, cut_nodes);
  EXPECT_EQ(colors_end, 3);

  auto [cut_graph, cross_arcs] = map_cut_graph(g, cut_nodes);
  EXPECT_EQ(cut_graph.get_num_nodes(), 2U);
  EXPECT_EQ(cut_graph.get_num_arcs(), 1U);

  // Cross arcs are the two edges adjacent to the cut nodes but reaching non-cut nodes.
  std::set<Graph::Arc *> cross_set;
  for (auto it = cross_arcs.get_it(); it.has_curr(); it.next_ne())
    cross_set.insert(it.get_curr());

  EXPECT_EQ(cross_set, (std::set<Graph::Arc *>{a01, a23}));

  (void)a12; // cut arc is between the two cut nodes
}

TEST(GraphUtilsCutNodes, PaintAndExtractBlocksOnStar)
{
  Graph g;
  auto nodes = make_nodes(g, 6); // 0 center, 1..5 leaves

  for (int i = 1; i < 6; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  auto cut_nodes = compute_cut_nodes(g, nodes[0]);
  const long colors_end = paint_subgraphs(g, cut_nodes);

  // Each leaf is a separate block => 5 colors: [1..5], so colors_end == 6.
  EXPECT_EQ(colors_end, 6);

  // Extract each painted block: each should be a single-node graph.
  for (long c = 1; c < colors_end; ++c)
    {
      auto sg = map_subgraph(g, c);
      EXPECT_EQ(sg.get_num_nodes(), 1U);
      EXPECT_EQ(sg.get_num_arcs(), 0U);
    }

  auto [cut_graph, cross_arcs] = map_cut_graph(g, cut_nodes);
  EXPECT_EQ(cut_graph.get_num_nodes(), 1U);
  EXPECT_EQ(cut_graph.get_num_arcs(), 0U);

  size_t cross_count = 0;
  for (auto it = cross_arcs.get_it(); it.has_curr(); it.next_ne())
    ++cross_count;
  EXPECT_EQ(cross_count, 5U);
}

// =============================================================================
// Digraph inversion (transpose)
// =============================================================================

TEST(GraphUtilsDigraph, InvertDigraphPreservesIsolatedNodesAndMapsArcs)
{
  TestDigraph g;
  auto nodes = make_nodes(g, 3);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 42);
  // nodes[2] is isolated

  auto gi = invert_digraph(g);
  EXPECT_EQ(gi.get_num_nodes(), 3U);
  EXPECT_EQ(gi.get_num_arcs(), 1U);

  auto n0i = mapped_node<TestDigraph>(nodes[0]);
  auto n1i = mapped_node<TestDigraph>(nodes[1]);
  auto n2i = mapped_node<TestDigraph>(nodes[2]);
  ASSERT_NE(n0i, nullptr);
  ASSERT_NE(n1i, nullptr);
  ASSERT_NE(n2i, nullptr);

  auto inv = search_directed_arc(gi, n1i, n0i);
  ASSERT_NE(inv, nullptr);
  EXPECT_EQ(inv->get_info(), 42);

  ASSERT_NE(mapped_arc<TestDigraph>(a01), nullptr);
  EXPECT_EQ(mapped_arc<TestDigraph>(mapped_arc<TestDigraph>(a01)), a01);
}

TEST(GraphUtilsDigraph, InvertDigraphRejectsUndirectedGraphs)
{
  Graph g;
  (void)g.insert_node(0);
  EXPECT_THROW(invert_digraph(g), std::domain_error);
}

TEST(GraphUtilsDigraph, InvertDigraphFunctorFiltersArcs)
{
  struct OnlyTwo
  {
    bool operator()(TestDigraph::Arc *a) const noexcept { return a->get_info() == 2; }
    void set_cookie(void *) noexcept {}
  };

  TestDigraph g;
  auto nodes = make_nodes(g, 3);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 2);

  Invert_Digraph<TestDigraph, OnlyTwo> inv(OnlyTwo{});
  auto gi = inv(g);

  EXPECT_EQ(gi.get_num_nodes(), 3U);
  EXPECT_EQ(gi.get_num_arcs(), 1U);

  auto n1i = mapped_node<TestDigraph>(nodes[1]);
  auto n2i = mapped_node<TestDigraph>(nodes[2]);
  ASSERT_NE(n1i, nullptr);
  ASSERT_NE(n2i, nullptr);

  EXPECT_NE(search_directed_arc(gi, n2i, n1i), nullptr);
  EXPECT_EQ(ARC_COOKIE(a01), nullptr);
  EXPECT_NE(ARC_COOKIE(a12), nullptr);
}

// =============================================================================
// Dft_Dist / get_min_path / Total_Cost
// =============================================================================

TEST(GraphUtilsCosts, DftDistConstants)
{
  EXPECT_EQ((Dft_Dist<Graph>::Zero_Distance), 0);
  EXPECT_EQ((Dft_Dist<Graph>::Max_Distance), std::numeric_limits<int>::max());
}

TEST(GraphUtilsCosts, GetMinPathReconstructsCookieChainAndComputesCost)
{
  WGraph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 2.5);
  g.insert_arc(nodes[1], nodes[2], 3.0);

  g.reset_nodes();

  NODE_COOKIE(nodes[1]) = nodes[0];
  NODE_COOKIE(nodes[2]) = nodes[1];

  Path<WGraph> path(g);
  const double dist = get_min_path<WGraph>(nodes[0], nodes[2], path);
  EXPECT_DOUBLE_EQ(dist, 5.5);
  EXPECT_EQ(path.size(), 3U);

  auto seq = path_nodes(path);
  ASSERT_EQ(seq.size(), 3U);
  EXPECT_EQ(seq.front(), nodes[0]);
  EXPECT_EQ(seq.back(), nodes[2]);

  // start == end is a valid trivial path.
  const double dist0 = get_min_path<WGraph>(nodes[1], nodes[1], path);
  EXPECT_DOUBLE_EQ(dist0, 0.0);
  EXPECT_EQ(path.size(), 1U);

  // Broken chain (nullptr) must throw.
  g.reset_nodes();
  NODE_COOKIE(nodes[2]) = nullptr;
  EXPECT_THROW(get_min_path<WGraph>(nodes[0], nodes[2], path), std::domain_error);
}

TEST(GraphUtilsCosts, TotalCostSumsArcsAndAccumulatorWorks)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[2], 2);
  g.insert_arc(nodes[0], nodes[3], 3);

  Total_Cost<Graph> cost;
  EXPECT_EQ(cost.total_cost(g), 6);

  // Accumulator usage with traverse_arcs(): sum incident arcs to node[0].
  cost.reset();
  EXPECT_TRUE((traverse_arcs<Graph, Node_Arc_Iterator<Graph>>(nodes[0], std::ref(cost))));
  EXPECT_EQ(cost.value(), 6);
}
