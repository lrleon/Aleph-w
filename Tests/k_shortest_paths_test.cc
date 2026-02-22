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
# include <functional>
# include <limits>
# include <random>
# include <tuple>
# include <unordered_map>
# include <unordered_set>
# include <utility>
# include <vector>

# include <K_Shortest_Paths.H>
# include <tpl_graph.H>

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
    std::vector<int> nodes;

    bool operator==(const Canonical_Path & other) const noexcept
    {
      return cost == other.cost and nodes == other.nodes;
    }
  };


  struct Built_Graph
  {
    Graph g;
    std::vector<Node *> nodes;
  };


  Built_Graph build_graph(const size_t n, const std::vector<Edge_Def> & edges)
  {
    Built_Graph built;
    built.nodes.reserve(n);
    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : edges)
      if (u < n and v < n)
        built.g.insert_arc(built.nodes[u], built.nodes[v], w);

    return built;
  }


  std::vector<int> extract_node_ids(const Path<Graph> & path)
  {
    std::vector<int> ids;
    for (Path<Graph>::Iterator it(path); it.has_current_node(); it.next_ne())
      ids.push_back(it.get_current_node_ne()->get_info());
    return ids;
  }


  bool is_simple_path(const Path<Graph> & path)
  {
    std::unordered_set<Node *> seen;
    for (Path<Graph>::Iterator it(path); it.has_current_node(); it.next_ne())
      {
        Node * node = it.get_current_node_ne();
        if (not seen.insert(node).second)
          return false;
      }
    return true;
  }


  template <class Result_List>
  std::vector<Canonical_Path> normalize_results(const Result_List & results)
  {
    std::vector<Canonical_Path> out;
    for (typename Result_List::Iterator it(results); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        out.push_back(Canonical_Path{
            item.total_cost,
            extract_node_ids(item.path)});
      }

    std::sort(out.begin(), out.end(),
              [](const Canonical_Path & a, const Canonical_Path & b)
              {
                if (a.cost != b.cost)
                  return a.cost < b.cost;
                return a.nodes < b.nodes;
              });
    return out;
  }


  std::vector<Canonical_Path>
  exact_k_shortest_simple_paths(Node * source,
                                Node * target,
                                const size_t k)
  {
    if (k == 0)
      return {};

    if (source == target)
      return {Canonical_Path{0, {source->get_info()}}};

    std::vector<Canonical_Path> all;
    std::unordered_set<Node *> visited;
    std::vector<int> stack;

    std::function<void(Node *, long long)> dfs =
        [&](Node * u, const long long cost)
    {
      if (u == target)
        {
          all.push_back(Canonical_Path{cost, stack});
          return;
        }

      for (Node_Arc_Iterator<Graph> it(u); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_current_arc_ne();
          Node * v = it.get_tgt_node();
          if (visited.contains(v))
            continue;

          visited.insert(v);
          stack.push_back(v->get_info());
          dfs(v, cost + arc->get_info());
          stack.pop_back();
          visited.erase(v);
        }
    };

    visited.insert(source);
    stack.push_back(source->get_info());
    dfs(source, 0);

    std::sort(all.begin(), all.end(),
              [](const Canonical_Path & a, const Canonical_Path & b)
              {
                if (a.cost != b.cost)
                  return a.cost < b.cost;
                return a.nodes < b.nodes;
              });

    if (all.size() > k)
      all.resize(k);
    return all;
  }


  std::vector<Edge_Def> random_graph_edges(std::mt19937_64 & rng,
                                           const size_t n)
  {
    std::bernoulli_distribution has_edge(0.36);
    std::uniform_int_distribution<int> base_w(1, 40);

    std::vector<Edge_Def> edges;
    size_t edge_id = 0;
    for (size_t u = 0; u < n; ++u)
      for (size_t v = 0; v < n; ++v)
        {
          if (u == v or not has_edge(rng))
            continue;
          const long long w = static_cast<long long>(base_w(rng))
                              + static_cast<long long>(edge_id * 101);
          edges.emplace_back(u, v, w);
          ++edge_id;
        }
    return edges;
  }
} // namespace


TEST(KShortestPathsTest, YenKnownGraphProducesExpectedCosts)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 3, 1},
      {1, 2, 1}, {2, 1, 1}, {0, 3, 5}
  };
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
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 3, 1},
      {1, 2, 2}, {0, 3, 7}
  };
  auto built = build_graph(4, edges);

  const auto yen = normalize_results(
      yen_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[3], 6));
  const auto epp = normalize_results(
      eppstein_k_shortest_paths<Graph>(built.g, built.nodes[0], built.nodes[3], 6));

  ASSERT_GE(epp.size(), yen.size());

  for (const auto & y: yen)
    {
      const auto found = std::find(epp.begin(), epp.end(), y);
      EXPECT_NE(found, epp.end());
    }
}


TEST(KShortestPathsTest, EppsteinCanReturnCyclicAlternatives)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {1, 2, 1}, {2, 3, 1},
      {2, 4, 1}, {4, 2, 1}
  };
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

  EXPECT_NE(epp[1].nodes, yen[0].nodes);
}


TEST(KShortestPathsTest, SourceEqualsTargetReturnsTrivialPath)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 2}, {1, 2, 2}, {2, 0, 2}
  };
  auto built = build_graph(3, edges);

  const auto got = yen_k_shortest_paths<Graph>(
      built.g, built.nodes[1], built.nodes[1], 10);

  ASSERT_EQ(got.size(), 1u);
  const auto normalized = normalize_results(got);
  ASSERT_EQ(normalized.size(), 1u);
  EXPECT_EQ(normalized[0].cost, 0);
  ASSERT_EQ(normalized[0].nodes.size(), 1u);
  EXPECT_EQ(normalized[0].nodes[0], 1);
}


TEST(KShortestPathsTest, UnreachableReturnsEmpty)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {1, 0, 1}, {2, 3, 1}
  };
  auto built = build_graph(4, edges);

  const auto got = yen_k_shortest_paths<Graph>(
      built.g, built.nodes[0], built.nodes[3], 4);
  EXPECT_TRUE(got.is_empty());
}


TEST(KShortestPathsTest, NegativeWeightsThrowDomainError)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 2}, {1, 2, -5}, {0, 2, 9}
  };
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
          EXPECT_EQ(yen[i].nodes, expected[i].nodes)
              << "trial=" << trial << ", i=" << i;
        }

      if (not expected.empty())
        EXPECT_EQ(epp.front().cost, expected.front().cost)
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
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {0, 2, 4}, {1, 2, 1}, {1, 3, 2},
      {2, 3, 1}, {2, 4, 3}, {3, 4, 1}
  };
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
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {1, 2, 1}, {2, 3, 1},
      {2, 4, 1}, {4, 2, 1}
  };
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
