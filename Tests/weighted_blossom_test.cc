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
 * @file weighted_blossom_test.cc
 * @brief Rigorous tests for Blossom_Weighted.H.
 *
 * Coverage:
 * - Deterministic blossom-heavy weighted instances
 * - Negative-weight behavior and max-cardinality mode
 * - Parallel arcs and loops handling
 * - Arc-filter behavior
 * - Cross-backend consistency (`List_Graph`, `List_SGraph`, `Array_Graph`)
 * - Random small graphs validated against exact exponential DP optimum
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <Blossom_Weighted.H>
#include <tpl_agraph.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
  using Weighted_Edge = std::tuple<size_t, size_t, long long>;
  using List_Backend =
      List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
  using SGraph_Backend =
      List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>;
  using AGraph_Backend =
      Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>;

  struct Objective
  {
    size_t cardinality = 0;
    long long total_weight = 0;

    bool operator==(const Objective & other) const noexcept
    {
      return cardinality == other.cardinality
             and total_weight == other.total_weight;
    }
  };


  template <class GT>
  GT build_graph(size_t n, const std::vector<Weighted_Edge> & edges)
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    nodes.reserve(n);

    for (size_t i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : edges)
      {
        if (u >= n or v >= n)
          continue;
        g.insert_arc(nodes[u], nodes[v], w);
      }

    return g;
  }


  template <class GT>
  bool verify_matching(const GT & g,
                       const DynDlist<typename GT::Arc *> & matching)
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
        if (src == nullptr or tgt == nullptr or src == tgt)
          return false;

        if (not used_nodes.insert(src).second)
          return false;

        if (not used_nodes.insert(tgt).second)
          return false;
      }

    return true;
  }


  bool better(const Objective & lhs,
              const Objective & rhs,
              bool max_cardinality) noexcept
  {
    if (max_cardinality)
      {
        if (lhs.cardinality != rhs.cardinality)
          return lhs.cardinality > rhs.cardinality;
        return lhs.total_weight > rhs.total_weight;
      }

    if (lhs.total_weight != rhs.total_weight)
      return lhs.total_weight > rhs.total_weight;

    return lhs.cardinality > rhs.cardinality;
  }


  std::vector<Weighted_Edge>
  normalize_simple_edges(size_t n, const std::vector<Weighted_Edge> & edges)
  {
    using Pair = std::pair<size_t, size_t>;
    struct Pair_Hash
    {
      size_t operator()(const Pair & p) const noexcept
      {
        const size_t h1 = std::hash<size_t>{}(p.first);
        const size_t h2 = std::hash<size_t>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
      }
    };

    std::unordered_map<Pair, long long, Pair_Hash> best;
    best.reserve(edges.size() * 2 + 1);

    for (auto [u, v, w] : edges)
      {
        if (u >= n or v >= n or u == v)
          continue;

        if (u > v)
          std::swap(u, v);

        const Pair key{u, v};
        const auto it = best.find(key);
        if (it == best.end() or w > it->second)
          best[key] = w;
      }

    std::vector<Weighted_Edge> result;
    result.reserve(best.size());
    for (const auto & kv : best)
      result.emplace_back(kv.first.first, kv.first.second, kv.second);

    std::sort(result.begin(), result.end(),
              [](const Weighted_Edge & a, const Weighted_Edge & b)
              {
                if (std::get<0>(a) != std::get<0>(b))
                  return std::get<0>(a) < std::get<0>(b);
                if (std::get<1>(a) != std::get<1>(b))
                  return std::get<1>(a) < std::get<1>(b);
                return std::get<2>(a) < std::get<2>(b);
              });

    return result;
  }


  Objective exact_weighted_matching_optimum(size_t n,
                                            const std::vector<Weighted_Edge> & edges,
                                            bool max_cardinality)
  {
    if (n == 0)
      return Objective{0, 0};

    // Exhaustive DP for validation in tests.
    ah_runtime_error_if(n > 24)
      << "exact_weighted_matching_optimum supports n <= 24";

    constexpr long long NO_EDGE = std::numeric_limits<long long>::min() / 4;
    std::vector<std::vector<long long>> w(n, std::vector<long long>(n, NO_EDGE));

    for (auto [u, v, wt] : normalize_simple_edges(n, edges))
      {
        w[u][v] = wt;
        w[v][u] = wt;
      }

    const uint64_t full_mask = (n == 64)
                               ? std::numeric_limits<uint64_t>::max()
                               : ((uint64_t{1} << n) - 1);

    const size_t state_count = static_cast<size_t>(full_mask + 1);
    std::vector<Objective> memo(state_count);
    std::vector<char> seen(state_count, 0);

    std::function<Objective(uint64_t)> solve = [&](uint64_t mask) -> Objective
    {
      if (mask == 0)
        return Objective{0, 0};

      const size_t pos = static_cast<size_t>(mask);
      if (seen[pos])
        return memo[pos];

      const size_t i = static_cast<size_t>(__builtin_ctzll(mask));
      const uint64_t rest = mask & ~(uint64_t{1} << i);

      Objective best = solve(rest); // i unmatched

      uint64_t options = rest;
      while (options != 0)
        {
          const size_t j = static_cast<size_t>(__builtin_ctzll(options));
          options &= (options - 1);

          if (w[i][j] == NO_EDGE)
            continue;

          Objective candidate = solve(rest & ~(uint64_t{1} << j));
          ++candidate.cardinality;
          candidate.total_weight += w[i][j];

          if (better(candidate, best, max_cardinality))
            best = candidate;
        }

      seen[pos] = 1;
      memo[pos] = best;
      return best;
    };

    return solve(full_mask);
  }


  template <class GT>
  Objective compute_objective_from_matching(const DynDlist<typename GT::Arc *> & matching)
  {
    Objective obj;

    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        obj.total_weight += static_cast<long long>(arc->get_info());
        ++obj.cardinality;
      }

    return obj;
  }


  template <class GT>
  Objective solve_weighted(const GT & g,
                           DynDlist<typename GT::Arc *> & matching,
                           bool max_cardinality)
  {
    const auto res = blossom_maximum_weight_matching<GT>(
        g,
        matching,
        Dft_Dist<GT>(),
        Dft_Show_Arc<GT>(),
        max_cardinality);

    EXPECT_EQ(res.cardinality, matching.size());
    const Objective obj = compute_objective_from_matching<GT>(matching);
    EXPECT_EQ(res.cardinality, obj.cardinality);
    EXPECT_EQ(res.total_weight, obj.total_weight);

    return obj;
  }


  template <class GT>
  std::vector<std::pair<size_t, size_t>>
  canonical_matching_pairs(const GT & g,
                           const DynDlist<typename GT::Arc *> & matching)
  {
    std::vector<std::pair<size_t, size_t>> pairs;
    pairs.reserve(matching.size());

    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        size_t u = static_cast<size_t>(g.get_src_node(arc)->get_info());
        size_t v = static_cast<size_t>(g.get_tgt_node(arc)->get_info());
        if (u > v)
          std::swap(u, v);
        pairs.emplace_back(u, v);
      }

    std::sort(pairs.begin(), pairs.end());
    return pairs;
  }


  struct Solve_Snapshot
  {
    Objective objective;
    std::vector<std::pair<size_t, size_t>> pairs;
  };


  template <class GT>
  Solve_Snapshot solve_backend(size_t n,
                               const std::vector<Weighted_Edge> & edges,
                               bool max_cardinality)
  {
    GT g = build_graph<GT>(n, edges);

    DynDlist<typename GT::Arc *> matching;
    const Objective objective = solve_weighted(g, matching, max_cardinality);

    EXPECT_TRUE(verify_matching(g, matching));

    return Solve_Snapshot{
        objective,
        canonical_matching_pairs(g, matching)};
  }


  template <class GT>
  void assert_matches_exact(size_t n,
                            const std::vector<Weighted_Edge> & edges,
                            bool max_cardinality)
  {
    GT g = build_graph<GT>(n, edges);

    DynDlist<typename GT::Arc *> matching;
    const Objective got = solve_weighted(g, matching, max_cardinality);

    EXPECT_TRUE(verify_matching(g, matching));

    const Objective expected = exact_weighted_matching_optimum(
        n, edges, max_cardinality);

    EXPECT_EQ(got, expected);
  }


  template <class GT>
  class WeightedBlossomTypedTest : public ::testing::Test
  {
  };

  using GraphBackends = ::testing::Types<
      List_Backend,
      SGraph_Backend,
      AGraph_Backend>;

  TYPED_TEST_SUITE(WeightedBlossomTypedTest, GraphBackends);

} // namespace


TYPED_TEST(WeightedBlossomTypedTest, EmptyGraph)
{
  using Graph = TypeParam;

  Graph g;
  DynDlist<typename Graph::Arc *> matching;

  const auto result = blossom_maximum_weight_matching<Graph>(g, matching);

  EXPECT_EQ(result.cardinality, 0u);
  EXPECT_EQ(result.total_weight, 0);
  EXPECT_TRUE(matching.is_empty());
}


TYPED_TEST(WeightedBlossomTypedTest, SinglePositiveEdge)
{
  using Graph = TypeParam;

  assert_matches_exact<Graph>(2, {{0, 1, 7}}, false);
}


TYPED_TEST(WeightedBlossomTypedTest, ParallelEdgesAndLoops)
{
  using Graph = TypeParam;

  const std::vector<Weighted_Edge> edges = {
      {0, 0, 99},
      {0, 1, 5},
      {0, 1, 12},
      {1, 2, 8},
      {2, 3, 9},
      {1, 3, 2}};

  assert_matches_exact<Graph>(4, edges, false);
  assert_matches_exact<Graph>(4, edges, true);
}


TYPED_TEST(WeightedBlossomTypedTest, NegativeWeightsDefaultAndMaxCardinality)
{
  using Graph = TypeParam;

  const std::vector<Weighted_Edge> edges = {
      {0, 1, -1},
      {1, 2, -2},
      {2, 3, -1},
      {0, 3, -2},
      {0, 2, -3}};

  assert_matches_exact<Graph>(4, edges, false);
  assert_matches_exact<Graph>(4, edges, true);
}


TYPED_TEST(WeightedBlossomTypedTest, WeightedBlossomRegressionCases)
{
  using Graph = TypeParam;

  // Selected cases from known weighted blossom stress regressions.
  const std::vector<std::pair<size_t, std::vector<Weighted_Edge>>> cases = {
      {4, {{1, 2, 8}, {1, 3, 9}, {2, 3, 10}, {3, 4, 7}}},
      {6, {{1, 2, 9}, {1, 3, 8}, {2, 3, 10}, {1, 4, 5}, {4, 5, 4}, {1, 6, 3}}},
      {8,
       {{1, 2, 23}, {1, 5, 22}, {1, 6, 15}, {2, 3, 25},
        {3, 4, 22}, {4, 5, 25}, {4, 8, 14}, {5, 7, 13}}},
      {10,
       {{1, 2, 45}, {1, 5, 45}, {2, 3, 50}, {3, 4, 45}, {4, 5, 50},
        {1, 6, 30}, {3, 9, 35}, {4, 8, 28}, {5, 7, 26}, {9, 10, 5}}},
      {11,
       {{1, 2, 40}, {1, 3, 40}, {2, 3, 60}, {2, 4, 55}, {3, 5, 55},
        {4, 5, 50}, {1, 8, 15}, {5, 7, 30}, {7, 6, 10}, {8, 10, 10}, {4, 9, 30}}}
  };

  for (const auto & [n, edges_one_based] : cases)
    {
      std::vector<Weighted_Edge> edges_zero_based;
      edges_zero_based.reserve(edges_one_based.size());

      for (const auto & [u, v, w] : edges_one_based)
        edges_zero_based.emplace_back(u - 1, v - 1, w);

      assert_matches_exact<Graph>(n, edges_zero_based, false);
      assert_matches_exact<Graph>(n, edges_zero_based, true);
    }
}


TYPED_TEST(WeightedBlossomTypedTest, RandomSmallGraphsAgainstExactDP)
{
  using Graph = TypeParam;

  std::mt19937_64 rng(0xC0FFEE1234ULL);
  std::uniform_int_distribution<int> n_dist(1, 11);
  std::uniform_real_distribution<double> p_dist(0.0, 1.0);
  std::uniform_int_distribution<int> w_dist(-10, 24);

  for (size_t iter = 0; iter < 180; ++iter)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Weighted_Edge> edges;

      for (size_t u = 0; u < n; ++u)
        for (size_t v = u + 1; v < n; ++v)
          {
            if (p_dist(rng) < 0.45)
              {
                edges.emplace_back(u, v, static_cast<long long>(w_dist(rng)));

                // Add occasional parallel edge with different weight.
                if (p_dist(rng) < 0.18)
                  edges.emplace_back(u, v, static_cast<long long>(w_dist(rng)));
              }
          }

      // Rare loops should be ignored by the algorithm.
      if (n > 0 and p_dist(rng) < 0.2)
        edges.emplace_back(static_cast<size_t>(rng() % n),
                           static_cast<size_t>(rng() % n),
                           static_cast<long long>(w_dist(rng)));

      assert_matches_exact<Graph>(n, edges, false);
      assert_matches_exact<Graph>(n, edges, true);
    }
}


TEST(WeightedBlossomStandaloneTest, CrossBackendUniqueOptimumConsistency)
{
  const std::vector<Weighted_Edge> edges = {
      {0, 1, 14},
      {1, 2, 13},
      {2, 0, 12},
      {2, 3, 30},
      {4, 5, 11},
      {5, 6, 10},
      {4, 6, 9},
      {0, 4, 8},
      {1, 5, 7},
      {3, 6, 6}
  };

  for (bool max_cardinality : {false, true})
    {
      const Solve_Snapshot list_res = solve_backend<List_Backend>(
          7, edges, max_cardinality);
      const Solve_Snapshot sgraph_res = solve_backend<SGraph_Backend>(
          7, edges, max_cardinality);
      const Solve_Snapshot agraph_res = solve_backend<AGraph_Backend>(
          7, edges, max_cardinality);

      const Objective expected = exact_weighted_matching_optimum(
          7, edges, max_cardinality);

      EXPECT_EQ(list_res.objective, expected);
      EXPECT_EQ(sgraph_res.objective, expected);
      EXPECT_EQ(agraph_res.objective, expected);

      EXPECT_EQ(list_res.pairs, sgraph_res.pairs);
      EXPECT_EQ(list_res.pairs, agraph_res.pairs);
    }
}


TEST(WeightedBlossomStandaloneTest, CrossBackendRandomObjectiveConsistency)
{
  std::mt19937_64 rng(0xB10550ULL);
  std::uniform_int_distribution<int> n_dist(1, 10);
  std::uniform_real_distribution<double> p_dist(0.0, 1.0);
  std::uniform_int_distribution<int> w_dist(-12, 25);

  for (size_t iter = 0; iter < 140; ++iter)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Weighted_Edge> edges;

      for (size_t u = 0; u < n; ++u)
        for (size_t v = u + 1; v < n; ++v)
          {
            if (p_dist(rng) < 0.42)
              {
                edges.emplace_back(u, v, static_cast<long long>(w_dist(rng)));
                if (p_dist(rng) < 0.2)
                  edges.emplace_back(u, v, static_cast<long long>(w_dist(rng)));
              }
          }

      if (n > 0 and p_dist(rng) < 0.3)
        edges.emplace_back(static_cast<size_t>(rng() % n),
                           static_cast<size_t>(rng() % n),
                           static_cast<long long>(w_dist(rng)));

      for (bool max_cardinality : {false, true})
        {
          const Solve_Snapshot list_res = solve_backend<List_Backend>(
              n, edges, max_cardinality);
          const Solve_Snapshot sgraph_res = solve_backend<SGraph_Backend>(
              n, edges, max_cardinality);
          const Solve_Snapshot agraph_res = solve_backend<AGraph_Backend>(
              n, edges, max_cardinality);

          const Objective expected = exact_weighted_matching_optimum(
              n, edges, max_cardinality);

          EXPECT_EQ(list_res.objective, expected);
          EXPECT_EQ(sgraph_res.objective, expected);
          EXPECT_EQ(agraph_res.objective, expected);
          EXPECT_EQ(list_res.objective, sgraph_res.objective);
          EXPECT_EQ(list_res.objective, agraph_res.objective);
        }
    }
}


namespace
{
  struct Positive_Arc_Filter
  {
    template <class Arc>
    bool operator()(Arc * arc) const noexcept
    {
      return arc->get_info() > 0;
    }
  };
}


TEST(WeightedBlossomStandaloneTest, ArcFilterIsRespected)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  auto * n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, -100);
  g.insert_arc(n2, n3, 6);
  g.insert_arc(n0, n3, -50);

  DynDlist<Graph::Arc *> matching;
  const auto result = blossom_maximum_weight_matching<
      Graph,
      Dft_Dist<Graph>,
      Positive_Arc_Filter>(g,
                           matching,
                           Dft_Dist<Graph>(),
                           Positive_Arc_Filter(),
                           false);

  EXPECT_EQ(result.cardinality, 2u);
  EXPECT_EQ(result.total_weight, 11);
  EXPECT_TRUE(verify_matching(g, matching));
}


TEST(WeightedBlossomStandaloneTest, DigraphThrowsDomainError)
{
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;

  Digraph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 42);

  DynDlist<Digraph::Arc *> matching;

  EXPECT_THROW(blossom_maximum_weight_matching<Digraph>(g, matching),
               std::domain_error);
}


TEST(WeightedBlossomStandaloneTest, MaxCardinalityCanBeatPureWeightCardinality)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;

  const std::vector<Weighted_Edge> edges = {
      {0, 1, 2},
      {0, 4, 3},
      {1, 2, 7},
      {1, 5, 2},
      {2, 3, 9},
      {2, 5, 4},
      {3, 4, 8},
      {3, 5, 4}
  };

  Graph g = build_graph<Graph>(6, edges);

  DynDlist<Graph::Arc *> weight_matching;
  const Objective pure_weight = solve_weighted(g, weight_matching, false);

  DynDlist<Graph::Arc *> card_matching;
  const Objective max_card = solve_weighted(g, card_matching, true);

  EXPECT_TRUE(verify_matching(g, weight_matching));
  EXPECT_TRUE(verify_matching(g, card_matching));

  EXPECT_GE(max_card.cardinality, pure_weight.cardinality);

  const Objective exact_weight = exact_weighted_matching_optimum(6, edges, false);
  const Objective exact_card = exact_weighted_matching_optimum(6, edges, true);

  EXPECT_EQ(pure_weight, exact_weight);
  EXPECT_EQ(max_card, exact_card);
}


TEST(WeightedBlossomMwMatchingStandaloneTest, FloatingWeightsAndFiniteValidation)
{
  namespace mw = blossom_weighted_detail::mwmatching;

  Array<mw::Edge<double>> edges;
  edges.reserve(3);
  edges.append(mw::Edge<double>(0, 1, 1.5));
  edges.append(mw::Edge<double>(1, 2, 2.0));
  edges.append(mw::Edge<double>(0, 2, 1.0));

  const Array<mw::VertexPair> sol = mw::maximum_weight_matching(edges);
  ASSERT_EQ(sol.size(), 1u);
  const auto [u, v] = sol[0];
  EXPECT_TRUE((u == 1 and v == 2) or (u == 2 and v == 1));

  Array<mw::Edge<double>> nan_edges;
  nan_edges.append(
      mw::Edge<double>(0, 1, std::numeric_limits<double>::quiet_NaN()));
  EXPECT_THROW((mw::maximum_weight_matching(nan_edges)),
               std::invalid_argument);
  EXPECT_THROW((mw::adjust_weights_for_maximum_cardinality_matching(nan_edges)),
               std::invalid_argument);

  Array<mw::Edge<double>> inf_edges;
  inf_edges.append(
      mw::Edge<double>(0, 1, std::numeric_limits<double>::infinity()));
  EXPECT_THROW((mw::maximum_weight_matching(inf_edges)),
               std::invalid_argument);
}


TEST(WeightedBlossomMwMatchingStandaloneTest, RejectsSelfLoopsAndDuplicateEdges)
{
  namespace mw = blossom_weighted_detail::mwmatching;

  Array<mw::Edge<long long>> self_loop;
  self_loop.append(mw::Edge<long long>(0, 0, 7));
  EXPECT_THROW((mw::maximum_weight_matching(self_loop)),
               std::invalid_argument);

  Array<mw::Edge<long long>> duplicate;
  duplicate.append(mw::Edge<long long>(0, 1, 8));
  duplicate.append(mw::Edge<long long>(1, 0, 9));
  EXPECT_THROW((mw::maximum_weight_matching(duplicate)),
               std::invalid_argument);
}


TEST(WeightedBlossomMwMatchingStandaloneTest, RejectsUnsafeIntegerRanges)
{
  namespace mw = blossom_weighted_detail::mwmatching;

  const long long max_safe = std::numeric_limits<long long>::max() / 4;
  const long long min_safe = std::numeric_limits<long long>::min() / 4;

  Array<mw::Edge<long long>> too_large_weight;
  too_large_weight.append(mw::Edge<long long>(0, 1, max_safe + 1));
  EXPECT_THROW((mw::maximum_weight_matching(too_large_weight)),
               std::invalid_argument);

  Array<mw::Edge<long long>> too_small_weight;
  too_small_weight.append(mw::Edge<long long>(0, 1, min_safe - 1));
  EXPECT_THROW((mw::adjust_weights_for_maximum_cardinality_matching(
                    too_small_weight)),
               std::invalid_argument);

  Array<mw::Edge<long long>> huge_range;
  huge_range.append(mw::Edge<long long>(0, 1, 0));
  huge_range.append(mw::Edge<long long>(0, 2, max_safe));
  EXPECT_THROW((mw::adjust_weights_for_maximum_cardinality_matching(huge_range)),
               std::invalid_argument);
}


TEST(WeightedBlossomStandaloneTest, ToLlCheckedRejectsOverflow)
{
  EXPECT_THROW(
      (blossom_weighted_detail::to_ll_checked<unsigned long long>(
          std::numeric_limits<unsigned long long>::max())),
      std::overflow_error);

  EXPECT_NO_THROW(
      (blossom_weighted_detail::to_ll_checked<long long>(
          std::numeric_limits<long long>::max())));

  EXPECT_NO_THROW(
      (blossom_weighted_detail::to_ll_checked<long long>(
          std::numeric_limits<long long>::min())));

  EXPECT_NO_THROW(
      (blossom_weighted_detail::to_ll_checked<unsigned long long>(
          static_cast<unsigned long long>(
              std::numeric_limits<long long>::max()))));

  EXPECT_NO_THROW(
      (blossom_weighted_detail::to_ll_checked<long long>(0LL)));
}


TEST(WeightedBlossomStandaloneTest, StressLargeRandomGraphsCrossBackendConsistency)
{
  std::mt19937_64 rng(0x5EED5EED1234ULL);
  std::uniform_int_distribution<int> n_dist(45, 85);
  std::uniform_real_distribution<double> p_dist(0.0, 1.0);
  std::uniform_int_distribution<int> w_dist(-40, 120);

  for (size_t iter = 0; iter < 8; ++iter)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Weighted_Edge> edges;
      edges.reserve((n * n) / 5);

      for (size_t u = 0; u < n; ++u)
        for (size_t v = u + 1; v < n; ++v)
          {
            if (p_dist(rng) < 0.14)
              {
                edges.emplace_back(u, v, static_cast<long long>(w_dist(rng)));
                if (p_dist(rng) < 0.05)
                  edges.emplace_back(u, v, static_cast<long long>(w_dist(rng)));
              }
          }

      const Solve_Snapshot list_weight = solve_backend<List_Backend>(
          n, edges, false);
      const Solve_Snapshot sgraph_weight = solve_backend<SGraph_Backend>(
          n, edges, false);
      const Solve_Snapshot agraph_weight = solve_backend<AGraph_Backend>(
          n, edges, false);

      EXPECT_EQ(list_weight.objective, sgraph_weight.objective);
      EXPECT_EQ(list_weight.objective, agraph_weight.objective);
      EXPECT_LE(list_weight.objective.cardinality, n / 2);

      if (iter < 3)
        {
          const Solve_Snapshot list_card = solve_backend<List_Backend>(
              n, edges, true);
          const Solve_Snapshot sgraph_card = solve_backend<SGraph_Backend>(
              n, edges, true);
          const Solve_Snapshot agraph_card = solve_backend<AGraph_Backend>(
              n, edges, true);

          EXPECT_EQ(list_card.objective, sgraph_card.objective);
          EXPECT_EQ(list_card.objective, agraph_card.objective);
          EXPECT_LE(list_card.objective.cardinality, n / 2);
        }
    }
}
