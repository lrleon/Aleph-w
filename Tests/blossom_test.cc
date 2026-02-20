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
 * @file blossom_test.cc
 * @brief Rigorous tests for Blossom.H maximum matching in general graphs.
 *
 * Coverage:
 * - Deterministic graphs with known optimum
 * - Graphs that require blossom contraction
 * - Self-loops and parallel arcs
 * - Cross-backend validation (List_Graph, List_SGraph, Array_Graph)
 * - Random small graphs validated against exact DP optimum
 * - Bipartite subset cross-check with Hopcroft-Karp
 * - Arc-filter behavior
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <cstdint>
# include <functional>
# include <random>
# include <stdexcept>
# include <unordered_set>
# include <utility>
# include <vector>

# include <Blossom.H>
# include <tpl_bipartite.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

using namespace Aleph;

namespace
{
  template <class GT>
  GT build_graph(size_t n,
                 const std::vector<std::pair<size_t, size_t>> & edges,
                 int arc_info = 1)
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    nodes.reserve(n);

    for (size_t i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v] : edges)
      {
        if (u >= n or v >= n)
          continue;
        g.insert_arc(nodes[u], nodes[v], arc_info);
      }

    return g;
  }

  template <class GT>
  bool verify_matching(const GT & g, const DynDlist<typename GT::Arc *> & matching)
  {
    std::unordered_set<typename GT::Arc *> arcs_in_graph;
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      arcs_in_graph.insert(it.get_curr());

    std::unordered_set<typename GT::Node *> used_nodes;
    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        if (arc == nullptr)
          return false;

        if (arcs_in_graph.find(arc) == arcs_in_graph.end())
          return false;

        auto * src = g.get_src_node(arc);
        auto * tgt = g.get_tgt_node(arc);
        if (src == nullptr or tgt == nullptr)
          return false;

        if (src == tgt)
          return false;

        if (not used_nodes.insert(src).second)
          return false;

        if (not used_nodes.insert(tgt).second)
          return false;
      }

    return true;
  }

  std::vector<std::pair<size_t, size_t>> normalize_simple_edges
  (size_t n, const std::vector<std::pair<size_t, size_t>> & edges)
  {
    std::vector<std::pair<size_t, size_t>> simple_edges;
    simple_edges.reserve(edges.size());

    for (auto [u, v] : edges)
      {
        if (u >= n or v >= n or u == v)
          continue;
        if (u > v)
          std::swap(u, v);
        simple_edges.emplace_back(u, v);
      }

    std::sort(simple_edges.begin(), simple_edges.end());
    simple_edges.erase(std::unique(simple_edges.begin(), simple_edges.end()),
                       simple_edges.end());

    return simple_edges;
  }

  size_t exact_maximum_matching_size(size_t n,
                                     const std::vector<std::pair<size_t, size_t>> & edges)
  {
    if (n == 0)
      return 0;

    // Enough for all tests in this file; keeps DP vector small and fast.
    ah_runtime_error_if(n > 20) << "exact_maximum_matching_size supports n <= 20";

    std::vector<uint64_t> adj(n, 0);
    for (auto [u, v] : normalize_simple_edges(n, edges))
      {
        adj[u] |= (uint64_t{1} << v);
        adj[v] |= (uint64_t{1} << u);
      }

    const size_t state_count = static_cast<size_t>(uint64_t{1} << n);
    std::vector<int> memo(state_count, -1);

    std::function<int(uint64_t)> solve = [&](uint64_t mask) -> int
    {
      if (mask == 0)
        return 0;

      int & ans = memo[static_cast<size_t>(mask)];
      if (ans != -1)
        return ans;

      const size_t i = static_cast<size_t>(__builtin_ctzll(mask));
      const uint64_t rest = mask & ~(uint64_t{1} << i);

      ans = solve(rest); // leave i unmatched

      uint64_t options = adj[i] & rest;
      while (options != 0)
        {
          const size_t j = static_cast<size_t>(__builtin_ctzll(options));
          options &= (options - 1);
          ans = std::max(ans,
                         1 + solve(rest & ~(uint64_t{1} << j)));
        }

      return ans;
    };

    const uint64_t all = (uint64_t{1} << n) - 1;
    return static_cast<size_t>(solve(all));
  }

  template <class GT>
  size_t run_blossom(const GT & g, DynDlist<typename GT::Arc *> & matching)
  {
    return blossom_maximum_cardinality_matching<GT>(g, matching);
  }

  template <class GT>
  GT build_bipartite_graph(size_t left_size,
                           size_t right_size,
                           const std::vector<std::pair<size_t, size_t>> & edges_lr)
  {
    GT g;

    std::vector<typename GT::Node *> left_nodes;
    std::vector<typename GT::Node *> right_nodes;
    left_nodes.reserve(left_size);
    right_nodes.reserve(right_size);

    for (size_t i = 0; i < left_size; ++i)
      left_nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (size_t i = 0; i < right_size; ++i)
      right_nodes.push_back(g.insert_node(static_cast<int>(left_size + i)));

    for (const auto & [l, r] : edges_lr)
      {
        if (l >= left_size or r >= right_size)
          continue;
        g.insert_arc(left_nodes[l], right_nodes[r], 1);
      }

    return g;
  }

  template <class GT>
  class BlossomMatchingTypedTest : public ::testing::Test
  {
  };

  using GraphBackends = ::testing::Types<
      List_Graph<Graph_Node<int>, Graph_Arc<int>>,
      List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
      Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

  TYPED_TEST_SUITE(BlossomMatchingTypedTest, GraphBackends);

} // namespace


TYPED_TEST(BlossomMatchingTypedTest, EmptyGraph)
{
  using Graph = TypeParam;

  Graph g;
  DynDlist<typename Graph::Arc *> matching;

  const size_t cardinality = run_blossom(g, matching);

  EXPECT_EQ(cardinality, 0u);
  EXPECT_TRUE(matching.is_empty());
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, SingleEdge)
{
  using Graph = TypeParam;

  auto g = build_graph<Graph>(2, {{0, 1}});
  DynDlist<typename Graph::Arc *> matching;

  const size_t cardinality = run_blossom(g, matching);

  EXPECT_EQ(cardinality, 1u);
  EXPECT_EQ(matching.size(), 1u);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, OddCycleC5)
{
  using Graph = TypeParam;

  auto g = build_graph<Graph>(5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}});
  DynDlist<typename Graph::Arc *> matching;

  const size_t cardinality = run_blossom(g, matching);

  EXPECT_EQ(cardinality, 2u);
  EXPECT_EQ(matching.size(), 2u);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, CompleteGraphK6)
{
  using Graph = TypeParam;

  std::vector<std::pair<size_t, size_t>> edges;
  for (size_t i = 0; i < 6; ++i)
    for (size_t j = i + 1; j < 6; ++j)
      edges.emplace_back(i, j);

  auto g = build_graph<Graph>(6, edges);
  DynDlist<typename Graph::Arc *> matching;

  const size_t cardinality = run_blossom(g, matching);

  EXPECT_EQ(cardinality, 3u);
  EXPECT_EQ(matching.size(), 3u);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, BlossomContractionScenario)
{
  using Graph = TypeParam;

  // 5-cycle plus three stems:
  // cycle: 0-1-2-3-4-0, stems: 1-5, 2-6, 3-7
  // Maximum matching = 4
  auto g = build_graph<Graph>(8,
                              {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0},
                               {1, 5}, {2, 6}, {3, 7}});
  DynDlist<typename Graph::Arc *> matching;

  const size_t cardinality = run_blossom(g, matching);

  EXPECT_EQ(cardinality, 4u);
  EXPECT_EQ(matching.size(), 4u);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, HandlesLoopsAndParallelArcs)
{
  using Graph = TypeParam;

  auto g = build_graph<Graph>(4,
                              {
                                {0, 0},
                                {0, 1},
                                {0, 1},
                                {1, 2},
                                {2, 3},
                                {3, 3}
                              });

  DynDlist<typename Graph::Arc *> matching;
  const size_t cardinality = run_blossom(g, matching);

  EXPECT_EQ(cardinality, 2u);
  EXPECT_EQ(matching.size(), 2u);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, FunctorWrapperMatchesFreeFunction)
{
  using Graph = TypeParam;

  auto g = build_graph<Graph>(7,
                              {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4},
                               {4, 5}, {5, 6}, {1, 6}});

  DynDlist<typename Graph::Arc *> free_matching;
  DynDlist<typename Graph::Arc *> functor_matching;

  const size_t c1 = blossom_maximum_cardinality_matching<Graph>(g, free_matching);
  const size_t c2 = Compute_Maximum_Cardinality_General_Matching<Graph>()(g, functor_matching);

  EXPECT_EQ(c1, c2);
  EXPECT_EQ(free_matching.size(), functor_matching.size());
  EXPECT_TRUE(verify_matching(g, free_matching));
  EXPECT_TRUE(verify_matching(g, functor_matching));
}

TYPED_TEST(BlossomMatchingTypedTest, RandomGraphsAgreeWithExactDP)
{
  using Graph = TypeParam;

  std::mt19937 rng(20260220);
  std::uniform_real_distribution<double> coin(0.0, 1.0);

  for (size_t n = 2; n <= 10; ++n)
    for (int sample = 0; sample < 30; ++sample)
      {
        const double p = 0.10 + 0.03 * static_cast<double>(sample);

        std::vector<std::pair<size_t, size_t>> edges;
        for (size_t u = 0; u < n; ++u)
          for (size_t v = u + 1; v < n; ++v)
            if (coin(rng) < p)
              edges.emplace_back(u, v);

        auto g = build_graph<Graph>(n, edges);

        DynDlist<typename Graph::Arc *> matching;
        const size_t blossom_size = run_blossom(g, matching);
        const size_t exact_size = exact_maximum_matching_size(n, edges);

        EXPECT_EQ(blossom_size, exact_size)
          << "n=" << n << " sample=" << sample;
        EXPECT_EQ(matching.size(), blossom_size)
          << "n=" << n << " sample=" << sample;
        EXPECT_TRUE(verify_matching(g, matching))
          << "n=" << n << " sample=" << sample;
      }
}

TYPED_TEST(BlossomMatchingTypedTest, AgreesWithHopcroftKarpOnBipartiteGraphs)
{
  using Graph = TypeParam;

  std::mt19937 rng(7);
  std::uniform_real_distribution<double> coin(0.0, 1.0);

  for (size_t left = 1; left <= 6; ++left)
    for (size_t right = 1; right <= 6; ++right)
      for (int sample = 0; sample < 10; ++sample)
        {
          const double p = 0.15 + 0.07 * static_cast<double>(sample);
          std::vector<std::pair<size_t, size_t>> edges_lr;

          for (size_t l = 0; l < left; ++l)
            for (size_t r = 0; r < right; ++r)
              if (coin(rng) < p)
                edges_lr.emplace_back(l, r);

          auto g = build_bipartite_graph<Graph>(left, right, edges_lr);

          DynDlist<typename Graph::Arc *> blossom_matching;
          DynDlist<typename Graph::Arc *> hk_matching;

          const size_t blossom_size =
              blossom_maximum_cardinality_matching<Graph>(g, blossom_matching);
          hopcroft_karp_matching<Graph>(g, hk_matching);

          EXPECT_EQ(blossom_size, hk_matching.size())
            << "left=" << left << " right=" << right << " sample=" << sample;
          EXPECT_TRUE(verify_matching(g, blossom_matching));
          EXPECT_TRUE(verify_matching(g, hk_matching));
        }
}

TYPED_TEST(BlossomMatchingTypedTest, NestedOddCyclesWithStems)
{
  using Graph = TypeParam;

  // Two connected 5-cycles plus stems:
  // C1: 0-1-2-3-4-0
  // C2: 2-5-6-7-8-2
  // stems: 1-9, 4-10, 6-11
  //
  // This family tends to trigger multiple blossom contractions and expansions.
  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0},
      {2, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 2},
      {1, 9}, {4, 10}, {6, 11}
  };

  auto g = build_graph<Graph>(12, edges);
  DynDlist<typename Graph::Arc *> matching;

  const size_t blossom_size = run_blossom(g, matching);
  const size_t exact_size = exact_maximum_matching_size(12, edges);

  EXPECT_EQ(blossom_size, exact_size);
  EXPECT_EQ(matching.size(), blossom_size);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, FlowerWithSharedCenterAndBridge)
{
  using Graph = TypeParam;

  // "Flower": three odd petals sharing vertex 0, connected to a tail.
  // Petals:
  //   0-1-2-0
  //   0-3-4-0
  //   0-5-6-0
  // Tail:
  //   2-7-8-9
  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {1, 2}, {2, 0},
      {0, 3}, {3, 4}, {4, 0},
      {0, 5}, {5, 6}, {6, 0},
      {2, 7}, {7, 8}, {8, 9}
  };

  auto g = build_graph<Graph>(10, edges);
  DynDlist<typename Graph::Arc *> matching;

  const size_t blossom_size = run_blossom(g, matching);
  const size_t exact_size = exact_maximum_matching_size(10, edges);

  EXPECT_EQ(blossom_size, exact_size);
  EXPECT_EQ(matching.size(), blossom_size);
  EXPECT_TRUE(verify_matching(g, matching));
}

TYPED_TEST(BlossomMatchingTypedTest, DenseRandomStressCrossBackendAgreement)
{
  using Graph = TypeParam;

  std::mt19937 rng(0xB10550u);
  std::uniform_real_distribution<double> coin(0.0, 1.0);

  // Stress sizes beyond exact-DP range.
  for (int sample = 0; sample < 12; ++sample)
    {
      const size_t n = 30 + static_cast<size_t>(sample % 11); // [30, 40]
      const double p = 0.16 + 0.01 * static_cast<double>(sample);

      std::vector<std::pair<size_t, size_t>> edges;
      for (size_t u = 0; u < n; ++u)
        for (size_t v = u + 1; v < n; ++v)
          if (coin(rng) < p)
            edges.emplace_back(u, v);

      auto g = build_graph<Graph>(n, edges);
      DynDlist<typename Graph::Arc *> matching;
      const size_t blossom_size = run_blossom(g, matching);

      EXPECT_EQ(matching.size(), blossom_size)
          << "sample=" << sample << " n=" << n;
      EXPECT_TRUE(verify_matching(g, matching))
          << "sample=" << sample << " n=" << n;
      EXPECT_LE(blossom_size, n / 2)
          << "sample=" << sample << " n=" << n;
    }
}

// -----------------------------------------------------------------------------
// Dedicated non-typed tests
// -----------------------------------------------------------------------------

namespace
{
  struct Positive_Arc_Filter
  {
    template <class Arc>
    bool operator()(Arc * a) const noexcept
    {
      return a->get_info() > 0;
    }
  };
}

TEST(BlossomMatchingStandaloneTest, ArcFilterIsRespected)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  auto * n3 = g.insert_node(3);

  // Positive arc.
  g.insert_arc(n0, n1, 1);
  // Zero arcs (filtered out).
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);
  g.insert_arc(n0, n3, 0);

  DynDlist<Graph::Arc *> matching;
  const size_t cardinality =
      blossom_maximum_cardinality_matching<Graph, Positive_Arc_Filter>(
          g, matching, Positive_Arc_Filter());

  EXPECT_EQ(cardinality, 1u);
  EXPECT_EQ(matching.size(), 1u);
  EXPECT_TRUE(verify_matching(g, matching));
}

TEST(BlossomMatchingStandaloneTest, DigraphThrowsDomainError)
{
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  Digraph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  DynDlist<Digraph::Arc *> matching;

  EXPECT_THROW(blossom_maximum_cardinality_matching<Digraph>(g, matching),
               std::domain_error);
}
