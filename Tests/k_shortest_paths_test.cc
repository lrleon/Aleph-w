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

    for (typename Array<Edge_Def>::Iterator it(edges); it.has_curr(); it.next_ne())
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
    for (typename Result_List::Iterator it(results); it.has_curr(); it.next_ne())
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
