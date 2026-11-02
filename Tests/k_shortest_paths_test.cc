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
 * @file k_shortest_paths_test.cc
 * @brief Tests for K_Shortest_Paths.H (Yen / Eppstein-style API).
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <chrono>
# include <cstdlib>
# include <functional>
# include <limits>
# include <random>
# include <stdexcept>
# include <string>
# include <tuple>
# include <utility>

# include <K_Shortest_Paths.H>
# include <tpl_array.H>
# include <tpl_dynArray.H>
# include <tpl_graph.H>
# include <tpl_olhash.H>

using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  using Edge_Def = std::tuple<size_t, size_t, long long>;

  struct Canonical_Path
  {
    long long cost = 0;
    DynArray<int> nodes;

    bool operator==(const Canonical_Path & other) const noexcept
    {
      if (cost != other.cost or nodes.size() != other.nodes.size())
        return false;

      for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes(i) != other.nodes(i))
          return false;

      return true;
    }
  };


  bool same_node_sequence(const DynArray<int> & a, const DynArray<int> & b)
  {
    if (a.size() != b.size())
      return false;

    for (size_t i = 0; i < a.size(); ++i)
      if (a(i) != b(i))
        return false;

    return true;
  }


  bool node_sequence_less(const DynArray<int> & a, const DynArray<int> & b)
  {
    const size_t min_size = std::min(a.size(), b.size());
    for (size_t i = 0; i < min_size; ++i)
      {
        if (a(i) < b(i))
          return true;
        if (a(i) > b(i))
          return false;
      }

    return a.size() < b.size();
  }


  bool canonical_less(const Canonical_Path & a, const Canonical_Path & b)
  {
    if (a.cost != b.cost)
      return a.cost < b.cost;
    return node_sequence_less(a.nodes, b.nodes);
  }


  void sort_canonical_paths(Array<Canonical_Path> & paths)
  {
    for (size_t i = 1; i < paths.size(); ++i)
      {
        Canonical_Path key = paths[i];
        size_t j = i;
        while (j > 0 and canonical_less(key, paths[j - 1]))
          {
            paths[j] = paths[j - 1];
            --j;
          }
        paths[j] = key;
      }
  }


  bool contains_canonical_path(const Array<Canonical_Path> & paths,
                               const Canonical_Path & needle)
  {
    for (size_t i = 0; i < paths.size(); ++i)
      if (paths[i] == needle)
        return true;
    return false;
  }


  Array<Edge_Def> make_edges(std::initializer_list<Edge_Def> init)
  {
    Array<Edge_Def> edges;
    edges.reserve(init.size());
    for (const auto & e : init)
      edges.append(e);
    return edges;
  }


  struct Built_Graph
  {
    Graph g;
    Array<Node *> nodes;
  };


  Built_Graph build_graph(const size_t n, const Array<Edge_Def> & edges)
  {
    Built_Graph built;
    built.nodes.reserve(n);
    for (size_t i = 0; i < n; ++i)
      built.nodes.append(built.g.insert_node(static_cast<int>(i)));

    for (auto it = edges.get_it(); it.has_curr(); it.next_ne())
        {
          const auto & [u, v, w] = it.get_curr();
          if (u < n and v < n)
            built.g.insert_arc(built.nodes[u], built.nodes[v], w);
        }

    return built;
  }


  DynArray<int> extract_node_ids(const Path<Graph> & path)
  {
    DynArray<int> ids;
    for (Path<Graph>::Iterator it(path); it.has_current_node(); it.next_ne())
      ids.append(it.get_current_node_ne()->get_info());
    return ids;
  }


  bool is_simple_path(const Path<Graph> & path)
  {
    OLhashTable<Node *> seen;
    for (Path<Graph>::Iterator it(path); it.has_current_node(); it.next_ne())
      {
        Node * node = it.get_current_node_ne();
        if (seen.contains(node))
          return false;
        seen.insert(node);
      }
    return true;
  }


  template <class Result_List>
  Array<Canonical_Path> normalize_results(const Result_List & results)
  {
    Array<Canonical_Path> out;
    for (auto it = results.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        out.append(Canonical_Path{
            item.total_cost,
            extract_node_ids(item.path)});
      }

    sort_canonical_paths(out);
    return out;
  }


  Array<Canonical_Path>
  exact_k_shortest_simple_paths(Node * source,
                                Node * target,
                                const size_t k)
  {
    if (k == 0)
      return Array<Canonical_Path>();

    if (source == target)
      {
        Canonical_Path p;
        p.cost = 0;
        p.nodes.append(source->get_info());
        Array<Canonical_Path> out;
        out.append(p);
        return out;
      }

    Array<Canonical_Path> all;
    OLhashTable<Node *> visited;
    DynArray<int> stack;

    std::function<void(Node *, long long)> dfs =
        [&](Node * u, const long long cost)
    {
      if (u == target)
        {
          Canonical_Path p;
          p.cost = cost;
          for (size_t i = 0; i < stack.size(); ++i)
            p.nodes.append(stack(i));
          all.append(p);
          return;
        }

      for (Node_Arc_Iterator<Graph> it(u); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_current_arc_ne();
          Node * v = it.get_tgt_node();
          if (visited.contains(v))
            continue;

          visited.insert(v);
          stack.append(v->get_info());
          dfs(v, cost + arc->get_info());
          (void) stack.pop();
          visited.remove(v);
        }
    };

    visited.insert(source);
    stack.append(source->get_info());
    dfs(source, 0);

    sort_canonical_paths(all);

    if (all.size() > k)
      while (all.size() > k)
        (void) all.remove_last();
    return all;
  }


  Array<Edge_Def> random_graph_edges(std::mt19937_64 & rng,
                                     const size_t n)
  {
    std::bernoulli_distribution has_edge(0.36);
    std::uniform_int_distribution<int> base_w(1, 40);

    Array<Edge_Def> edges;
    size_t edge_id = 0;
    for (size_t u = 0; u < n; ++u)
      for (size_t v = 0; v < n; ++v)
        {
          if (u == v or not has_edge(rng))
            continue;
          const long long w = static_cast<long long>(base_w(rng))
                              + static_cast<long long>(edge_id * 101);
          edges.append(Edge_Def{u, v, w});
          ++edge_id;
        }
    return edges;
  }
} // namespace


TEST(KShortestPathsTest, YenKnownGraphProducesExpectedCosts)
{
  const auto edges = make_edges({
      {0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 3, 1},
      {1, 2, 1}, {2, 1, 1}, {0, 3, 5}
  });
  auto built = build_graph(4, edges);

  const auto got = yen_k_shortest_paths<Graph>(
      built.g, built.nodes[0], built.nodes[3], 5);
  auto normalized = normalize_results(got);

  ASSERT_EQ(normalized.size(), 5u);
  EXPECT_EQ(normalized[0].cost, 2);
  EXPECT_EQ(normalized[1].cost, 2);
  EXPECT_EQ(normalized[2].cost, 3);
  EXPECT_EQ(normalized[3].cost, 3);
  EXPECT_EQ(normalized[4].cost, 5);
}


TEST(KShortestPathsTest, EppsteinApiMatchesYenOnKnownGraph)
{
  const auto edges = make_edges({
      {0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 3, 1},
      {1, 2, 2}, {0, 3, 7}
  });
  auto built = build_graph(4, edges);

  const auto yen = normalize_results(
      yen_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[3], 6));
  const auto epp = normalize_results(
      eppstein_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[3], 6));

  ASSERT_GE(epp.size(), yen.size());

  for (size_t i = 0; i < yen.size(); ++i)
    {
      EXPECT_TRUE(contains_canonical_path(epp, yen[i]));
    }
}


TEST(KShortestPathsTest, EppsteinCanReturnCyclicAlternatives)
{
  const auto edges = make_edges({
      {0, 1, 1}, {1, 2, 1}, {2, 3, 1},
      {2, 4, 1}, {4, 2, 1}
  });
  auto built = build_graph(5, edges);

  const auto yen = normalize_results(
      yen_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[3], 4));
  const auto epp = normalize_results(
      eppstein_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[3], 4));

  ASSERT_EQ(yen.size(), 1u);
  ASSERT_EQ(epp.size(), 4u);

  EXPECT_EQ(epp[0].cost, 3);
  EXPECT_EQ(epp[1].cost, 5);
  EXPECT_EQ(epp[2].cost, 7);
  EXPECT_EQ(epp[3].cost, 9);

  EXPECT_FALSE(same_node_sequence(epp[1].nodes, yen[0].nodes));
}


TEST(KShortestPathsTest, SourceEqualsTargetReturnsTrivialPath)
{
  const auto edges = make_edges({
      {0, 1, 2}, {1, 2, 2}, {2, 0, 2}
  });
  auto built = build_graph(3, edges);

  const auto got = yen_k_shortest_paths<Graph>(
      built.g, built.nodes[1], built.nodes[1], 10);

  ASSERT_EQ(got.size(), 1u);
  const auto normalized = normalize_results(got);
  ASSERT_EQ(normalized.size(), 1u);
  EXPECT_EQ(normalized[0].cost, 0);
  ASSERT_EQ(normalized[0].nodes.size(), 1u);
  EXPECT_EQ(normalized[0].nodes(0), 1);
}


TEST(KShortestPathsTest, UnreachableReturnsEmpty)
{
  const auto edges = make_edges({
      {0, 1, 1}, {1, 0, 1}, {2, 3, 1}
  });
  auto built = build_graph(4, edges);

  const auto got = yen_k_shortest_paths<Graph>(
      built.g, built.nodes[0], built.nodes[3], 4);
  EXPECT_TRUE(got.is_empty());
}


TEST(KShortestPathsTest, NegativeWeightsThrowDomainError)
{
  const auto edges = make_edges({
      {0, 1, 2}, {1, 2, -5}, {0, 2, 9}
  });
  auto built = build_graph(3, edges);

  EXPECT_THROW(
      (yen_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[2], 3)),
      std::domain_error);
  EXPECT_THROW(
      (eppstein_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[2], 3)),
      std::domain_error);
}


TEST(KShortestPathsTest, RandomSmallGraphsMatchExactOracle)
{
  std::mt19937_64 rng(0xBADC0FFEEULL);
  std::uniform_int_distribution<int> n_dist(3, 8);

  for (size_t trial = 0; trial < 80; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      auto built = build_graph(n, random_graph_edges(rng, n));
      Node * source = built.nodes[0];
      Node * target = built.nodes[n - 1];
      constexpr size_t K = 7;

      const auto expected = exact_k_shortest_simple_paths(
          source, target, K);

      const auto yen = normalize_results(
          yen_k_shortest_paths<Graph>(built.g, source, target, K));
      const auto epp = normalize_results(
          eppstein_k_shortest_paths<Graph>(built.g, source, target, K));

      ASSERT_EQ(yen.size(), expected.size()) << "trial=" << trial;
      ASSERT_GE(epp.size(), expected.size()) << "trial=" << trial;
      ASSERT_LE(epp.size(), K) << "trial=" << trial;

      for (size_t i = 0; i < expected.size(); ++i)
        {
          EXPECT_EQ(yen[i].cost, expected[i].cost)
              << "trial=" << trial << ", i=" << i;
          EXPECT_TRUE(same_node_sequence(yen[i].nodes, expected[i].nodes))
              << "trial=" << trial << ", i=" << i;
        }

      if (expected.size() > 0)
        EXPECT_EQ(epp[0].cost, expected[0].cost)
            << "trial=" << trial;

      for (size_t i = 1; i < epp.size(); ++i)
        {
          EXPECT_GE(epp[i].cost, epp[i - 1].cost)
              << "trial=" << trial << ", i=" << i;
        }
    }
}


TEST(KShortestPathsTest, ReturnedPathsAreSimpleAndNonDecreasingCost)
{
  const auto edges = make_edges({
      {0, 1, 1}, {0, 2, 4}, {1, 2, 1}, {1, 3, 2},
      {2, 3, 1}, {2, 4, 3}, {3, 4, 1}
  });
  auto built = build_graph(5, edges);

  const auto got = yen_k_shortest_paths<Graph>(
      built.g, built.nodes[0], built.nodes[4], 10);

  long long prev = std::numeric_limits<long long>::min();
  for (auto it = got.get_it(); it.has_curr(); it.next_ne())
    {
      const auto & item = it.get_curr();
      EXPECT_TRUE(is_simple_path(item.path));
      EXPECT_GE(item.total_cost, prev);
      prev = item.total_cost;
    }
}


TEST(KShortestPathsTest, EppsteinResultsAreNonDecreasingCost)
{
  const auto edges = make_edges({
      {0, 1, 1}, {1, 2, 1}, {2, 3, 1},
      {2, 4, 1}, {4, 2, 1}
  });
  auto built = build_graph(5, edges);

  const auto got = eppstein_k_shortest_paths<Graph>(
      built.g, built.nodes[0], built.nodes[3], 8);

  long long prev = std::numeric_limits<long long>::min();
  for (auto it = got.get_it(); it.has_curr(); it.next_ne())
    {
      const auto & item = it.get_curr();
      EXPECT_GE(item.total_cost, prev);
      prev = item.total_cost;
    }
}


TEST(KShortestPathsTest, SelfLoopAndParallelArcs)
{
  // Graph with:
  // - Node 0 -> 1: Two parallel arcs with weights 10 and 20.
  // - Node 1 -> 1: Self-loop with weight 5.
  // - Node 1 -> 2: Arc with weight 5.
  //
  // Shortest paths from 0 to 2:
  // 1. 0->1(cost=10)->2(cost=5) = 15
  // 2. 0->1(cost=20)->2(cost=5) = 25
  //
  // The self-loop 1->1 should be ignored by Yen's (simple paths only).
  // Eppstein's API is general (non-loopless), so a cheaper loopy path may
  // appear before the second simple alternative.

  const auto edges = make_edges({
      {0, 1, 10}, // parallel arc 1
      {0, 1, 20}, // parallel arc 2
      {1, 1, 5},  // self-loop on spur node
      {1, 2, 5}
  });
  auto built = build_graph(3, edges);

  const size_t K = 2;
  const auto yen_results = normalize_results(
      yen_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[2], K));
  const auto epp_results = normalize_results(
      eppstein_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[2], K));

  ASSERT_EQ(yen_results.size(), 2u);
  ASSERT_EQ(epp_results.size(), 2u);

  // Check path 1: Cost 15, nodes [0, 1, 2]
  EXPECT_EQ(yen_results[0].cost, 15);
  EXPECT_EQ(yen_results[0].nodes.size(), 3u);
  EXPECT_EQ(yen_results[0].nodes(0), 0);
  EXPECT_EQ(yen_results[0].nodes(1), 1);
  EXPECT_EQ(yen_results[0].nodes(2), 2);

  // Check path 2: Cost 25, nodes [0, 1, 2]
  EXPECT_EQ(yen_results[1].cost, 25);
  EXPECT_EQ(yen_results[1].nodes.size(), 3u);
  EXPECT_EQ(yen_results[1].nodes(0), 0);
  EXPECT_EQ(yen_results[1].nodes(1), 1);
  EXPECT_EQ(yen_results[1].nodes(2), 2);

  // Eppstein: first path matches Yen's best simple path.
  EXPECT_EQ(epp_results[0], yen_results[0]);

  // Eppstein can use the self-loop:
  // 0->1(cost=10)->1(cost=5)->2(cost=5) = 20
  EXPECT_EQ(epp_results[1].cost, 20);
  EXPECT_EQ(epp_results[1].nodes.size(), 4u);
  EXPECT_EQ(epp_results[1].nodes(0), 0);
  EXPECT_EQ(epp_results[1].nodes(1), 1);
  EXPECT_EQ(epp_results[1].nodes(2), 1);
  EXPECT_EQ(epp_results[1].nodes(3), 2);
}


// Eppstein made the deviations of a walk also after the last one that was
// asked, and threw when the cost of one did not fit: with a loop of max() at
// the target, a query of one walk threw. A walk whose cost does not fit is
// left out, and overflow_error comes only when one of the first k does not
// fit.
TEST(KShortestPathsTest, EppsteinLeavesOutTheWalksWhoseCostDoesNotFit)
{
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  constexpr int M = std::numeric_limits<int>::max();

  Int_Graph g;
  auto * s = g.insert_node(0);
  auto * t = g.insert_node(1);
  g.insert_arc(s, t, 1);
  g.insert_arc(t, t, M);

  const auto one = eppstein_k_shortest_paths<Int_Graph>(g, s, t, 1);
  ASSERT_EQ(one.size(), 1u);
  EXPECT_EQ(one.get_first().total_cost, 1);
  // The second walk, s->t->t, costs M + 1
  EXPECT_THROW(eppstein_k_shortest_paths<Int_Graph>(g, s, t, 2), std::overflow_error);

  // A second walk that fits comes before the one that does not
  g.insert_arc(s, t, 5);
  const auto two = eppstein_k_shortest_paths<Int_Graph>(g, s, t, 2);
  ASSERT_EQ(two.size(), 2u);
  EXPECT_EQ(two.get_first().total_cost, 1);
  EXPECT_EQ(two.get_last().total_cost, 5);
  EXPECT_THROW(eppstein_k_shortest_paths<Int_Graph>(g, s, t, 3), std::overflow_error);

  // A deviation at the source whose cost does not fit
  Int_Graph h;
  auto * a = h.insert_node(0);
  auto * b = h.insert_node(1);
  auto * x = h.insert_node(2);
  h.insert_arc(a, b, 1);
  h.insert_arc(a, x, M);
  h.insert_arc(x, b, 1);
  EXPECT_EQ(eppstein_k_shortest_paths<Int_Graph>(h, a, b, 1).size(), 1u);
  EXPECT_THROW(eppstein_k_shortest_paths<Int_Graph>(h, a, b, 2), std::overflow_error);
}


// max() marked a node that does not reach the target, and Yen took a
// spur path of that cost for none: a path of exactly max() was not given.
TEST(KShortestPathsTest, APathOfExactlyMaxIsGiven)
{
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  constexpr int M = std::numeric_limits<int>::max();

  Int_Graph g;
  auto * a = g.insert_node(0);
  auto * b = g.insert_node(1);
  auto * c = g.insert_node(2);
  g.insert_arc(a, b, M - 1);
  g.insert_arc(b, c, 1);

  for (bool by_yen : {true, false})
    {
      SCOPED_TRACE(by_yen ? "Yen" : "Eppstein");
      const auto one = by_yen ? yen_k_shortest_paths<Int_Graph>(g, a, c, 2)
                              : eppstein_k_shortest_paths<Int_Graph>(g, a, c, 2);
      ASSERT_EQ(one.size(), 1u);
      EXPECT_EQ(one.get_first().total_cost, M);
    }

  // After a cheaper path
  g.insert_arc(a, b, M / 2);
  for (bool by_yen : {true, false})
    {
      SCOPED_TRACE(by_yen ? "Yen" : "Eppstein");
      const auto two = by_yen ? yen_k_shortest_paths<Int_Graph>(g, a, c, 2)
                              : eppstein_k_shortest_paths<Int_Graph>(g, a, c, 2);
      ASSERT_EQ(two.size(), 2u);
      EXPECT_EQ(two.get_first().total_cost, M / 2 + 1);
      EXPECT_EQ(two.get_last().total_cost, M);
    }
}

// The search of Yen from each spur node is a Dijkstra, which threw when
// the target was reached only beyond max(): the next path, from another
// spur node, was lost.
TEST(KShortestPathsTest, YenGoesOnPastASpurNodeBeyondMax)
{
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  constexpr int M = std::numeric_limits<int>::max();

  Int_Graph g;
  auto * s = g.insert_node(0);
  auto * a = g.insert_node(1);
  auto * c = g.insert_node(2);
  auto * d = g.insert_node(3);
  auto * t = g.insert_node(4);
  g.insert_arc(s, a, 1);
  g.insert_arc(a, t, 1);
  g.insert_arc(a, c, 1);
  g.insert_arc(c, t, 1);
  g.insert_arc(s, d, M - 1); // from s, without s->a, only beyond max()
  g.insert_arc(d, t, M - 1);

  const auto two = yen_k_shortest_paths<Int_Graph>(g, s, t, 2);
  ASSERT_EQ(two.size(), 2u);
  EXPECT_EQ(two.get_first().total_cost, 2);
  EXPECT_EQ(two.get_last().total_cost, 3);
  // The third path, s->d->t, costs 2 M - 2
  EXPECT_THROW(yen_k_shortest_paths<Int_Graph>(g, s, t, 3), std::overflow_error);
}

// Eppstein chose the arc of its tree among the parallel arcs to the parent
// by a checked sum, which threw on a heavy one.
TEST(KShortestPathsTest, EppsteinTakesAHeavyParallelArcOfTheTree)
{
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Int_Undigraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  constexpr int M = std::numeric_limits<int>::max();

  Int_Graph g;
  auto * s = g.insert_node(0);
  auto * a = g.insert_node(1);
  auto * t = g.insert_node(2);
  g.insert_arc(s, a, 1);
  g.insert_arc(s, a, M); // M + d(a, t) does not fit
  g.insert_arc(a, t, 1);
  const auto walks = eppstein_k_shortest_paths<Int_Graph>(g, s, t, 1);
  ASSERT_EQ(walks.size(), 1u);
  EXPECT_EQ(walks.get_first().total_cost, 2);

  Int_Undigraph u;
  auto * us = u.insert_node(0);
  auto * ua = u.insert_node(1);
  auto * ut = u.insert_node(2);
  u.insert_arc(us, ua, 1);
  u.insert_arc(us, ua, M);
  u.insert_arc(ua, ut, 1);
  const auto uwalks = eppstein_k_shortest_paths<Int_Undigraph>(u, us, ut, 1);
  ASSERT_EQ(uwalks.size(), 1u);
  EXPECT_EQ(uwalks.get_first().total_cost, 2);
}


// The search of Yen from each spur node takes a target that it reaches
// only beyond max() for no path, but an overflow_error that the distance or
// the filter throws, at whatever call, passes on, as in Eppstein.
TEST(KShortestPathsTest, AnOverflowErrorOfTheDistanceOrOfTheFilterPassesOn)
{
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Arc = Int_Graph::Arc;
  constexpr int M = std::numeric_limits<int>::max();

  // As in YenGoesOnPastASpurNodeBeyondMax: from s, without s->a, the target
  // is reached only beyond max()
  Int_Graph g;
  auto * s = g.insert_node(0);
  auto * a = g.insert_node(1);
  auto * c = g.insert_node(2);
  auto * d = g.insert_node(3);
  auto * t = g.insert_node(4);
  g.insert_arc(s, a, 1);
  g.insert_arc(a, t, 1);
  g.insert_arc(a, c, 1);
  g.insert_arc(c, t, 1);
  g.insert_arc(s, d, M - 1);
  g.insert_arc(d, t, M - 1);

  // Each throws at its call number at; with at == 0, never
  struct Distance
  {
    using Distance_Type = int;
    size_t * calls;
    size_t at;
    int operator()(Arc * arc) const
    {
      if (++*calls == at)
        throw std::overflow_error("thrown by the distance");
      return arc->get_info();
    }
  };
  struct Filter
  {
    size_t * calls;
    size_t at;
    bool operator()(Arc *) const
    {
      if (++*calls == at)
        throw std::overflow_error("thrown by the filter");
      return true;
    }
  };

  size_t distance_calls = 0, filter_calls = 0;
  for (bool by_yen : {true, false})
    for (bool by_distance : {true, false})
      {
        SCOPED_TRACE(std::string(by_yen ? "Yen" : "Eppstein")
                     + (by_distance ? ", the distance" : ", the filter"));
        const size_t & calls = by_distance ? distance_calls : filter_calls;
        const char * what =
          by_distance ? "thrown by the distance" : "thrown by the filter";
        auto run = [&] (size_t at)
          {
            distance_calls = filter_calls = 0;
            const Distance distance{&distance_calls, by_distance ? at : 0};
            const Filter filter{&filter_calls, by_distance ? 0 : at};
            if (by_yen)
              return yen_k_shortest_paths(g, s, t, 2, distance, filter).size();
            return eppstein_k_shortest_paths(g, s, t, 2, distance, filter).size();
          };

        ASSERT_EQ(run(0), 2u);
        const size_t total = calls;
        ASSERT_GT(total, 0u);
        for (size_t at = 1; at <= total; ++at)
          {
            bool threw = false;
            try
              {
                (void) run(at);
              }
            catch (const std::overflow_error & e)
              {
                threw = true;
                EXPECT_STREQ(e.what(), what) << "at the call " << at;
              }
            // A run may make fewer calls
            EXPECT_TRUE(threw or calls < at)
              << "the call " << at << " threw nothing out";
          }
      }
}


// ============================================================================
// Opt-in deterministic performance regression test.
//
// Activate with:  PERF_TESTS_ENABLED=1 ./build/Tests/k_shortest_paths_test
//
// Parameters (pinned for reproducibility; update the comment if you change them):
//   MASTER_SEED  0xDEADBEEFCAFEBABEULL — drives all graph generation
//   N            40 nodes per graph   (vs. 3–8 in RandomSmallGraphsMatchExactOracle)
//   K            25 shortest paths requested
//   TRIALS       3 independent random graphs drawn from the same RNG stream
//   Edge model   random_graph_edges(): Bernoulli p=0.36, weight = base ∈ [1,40]
//                                      + 101*edge_index (unique, non-negative)
//   Budget       1000 ms per algorithm invocation (wall-clock via steady_clock)
//
// When a machine-specific baseline is established, record it here and tighten
// the budget accordingly.
// ============================================================================

TEST(KShortestPathsTest, PerfRegression)
{
  const char * env = std::getenv("PERF_TESTS_ENABLED");
  if (env == nullptr or std::string(env) != "1")
    GTEST_SKIP() << "Skipped: set PERF_TESTS_ENABLED=1 to run";

  constexpr size_t   N           = 40;
  constexpr size_t   K           = 25;
  constexpr int      TRIALS      = 3;
  constexpr long long BUDGET_MS  = 1000; // milliseconds per algorithm call
  constexpr uint64_t MASTER_SEED = 0xDEADBEEFCAFEBABEULL;

  std::mt19937_64 rng(MASTER_SEED);

  for (int trial = 0; trial < TRIALS; ++trial)
    {
      auto built = build_graph(N, random_graph_edges(rng, N));
      Node * src = built.nodes[0];
      Node * tgt = built.nodes[N - 1];

      // --- Yen k-shortest (simple) paths ---
      {
        const auto t0      = std::chrono::steady_clock::now();
        const auto result  = yen_k_shortest_paths<Graph>(built.g, src, tgt, K);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();

        EXPECT_LE(elapsed, BUDGET_MS)
            << "yen_k_shortest_paths exceeded budget on trial " << trial
            << ": " << elapsed << " ms (budget " << BUDGET_MS << " ms,"
            << " N=" << N << ", K=" << K << ", seed=" << MASTER_SEED << ")";

        EXPECT_LE(result.size(), K) << "trial=" << trial;

        // Non-decreasing cost sanity check
        long long prev = std::numeric_limits<long long>::min();
        for (auto it = result.get_it(); it.has_curr(); it.next_ne())
          {
            EXPECT_GE(it.get_curr().total_cost, prev) << "trial=" << trial;
            prev = it.get_curr().total_cost;
          }
      }

      // --- Eppstein k-shortest (possibly non-simple) paths ---
      {
        const auto t0      = std::chrono::steady_clock::now();
        const auto result  = eppstein_k_shortest_paths<Graph>(built.g, src, tgt, K);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();

        EXPECT_LE(elapsed, BUDGET_MS)
            << "eppstein_k_shortest_paths exceeded budget on trial " << trial
            << ": " << elapsed << " ms (budget " << BUDGET_MS << " ms,"
            << " N=" << N << ", K=" << K << ", seed=" << MASTER_SEED << ")";

        EXPECT_LE(result.size(), K) << "trial=" << trial;

        long long prev = std::numeric_limits<long long>::min();
        for (auto it = result.get_it(); it.has_curr(); it.next_ne())
          {
            EXPECT_GE(it.get_curr().total_cost, prev) << "trial=" << trial;
            prev = it.get_curr().total_cost;
          }
      }
    }
}
