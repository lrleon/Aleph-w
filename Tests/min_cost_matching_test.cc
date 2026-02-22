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
 * @file min_cost_matching_test.cc
 * @brief Rigorous tests for Min_Cost_Matching.H.
 *
 * Coverage:
 * - Deterministic non-bipartite costed instances
 * - Objective-mode behavior (pure minimum-cost vs max-cardinality + min-cost)
 * - Parallel arcs, loops, and domain/overflow validations
 * - Cross-backend consistency (`List_Graph`, `List_SGraph`, `Array_Graph`)
 * - Random small-graph validation against exact exponential DP oracle
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <bit>
# include <chrono>
# include <cstdint>
# include <functional>
# include <limits>
# include <ostream>
# include <random>
# include <stdexcept>
# include <tuple>
# include <unordered_map>
# include <unordered_set>
# include <utility>
# include <vector>

# include <Blossom_Weighted.H>
# include <Min_Cost_Matching.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
  using Cost_Edge = std::tuple<size_t, size_t, long long>;
  using List_Backend =
      List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
  using SGraph_Backend =
      List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>;
  using AGraph_Backend =
      Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>;

  struct Objective
  {
    size_t cardinality = 0;
    long long total_cost = 0;

    bool operator==(const Objective & other) const noexcept
    {
      return cardinality == other.cardinality
             and total_cost == other.total_cost;
    }
  };


  void PrintTo(const Objective & obj, std::ostream * os)
  {
    *os << "{card=" << obj.cardinality
        << ", cost=" << obj.total_cost << "}";
  }


  template <class GT>
  GT build_graph(size_t n, const std::vector<Cost_Edge> & edges)
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    nodes.reserve(n);

    for (size_t i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, c] : edges)
      {
        if (u >= n or v >= n)
          continue;
        g.insert_arc(nodes[u], nodes[v], c);
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
        return lhs.total_cost < rhs.total_cost;
      }

    if (lhs.total_cost != rhs.total_cost)
      return lhs.total_cost < rhs.total_cost;

    return lhs.cardinality > rhs.cardinality;
  }


  std::vector<Cost_Edge>
  normalize_simple_edges(size_t n, const std::vector<Cost_Edge> & edges)
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

    for (auto [u, v, c] : edges)
      {
        if (u >= n or v >= n or u == v)
          continue;

        if (u > v)
          std::swap(u, v);

        const Pair key{u, v};
        const auto it = best.find(key);
        if (it == best.end() or c < it->second)
          best[key] = c;
      }

    std::vector<Cost_Edge> result;
    result.reserve(best.size());
    for (const auto & kv : best)
      result.emplace_back(kv.first.first, kv.first.second, kv.second);

    std::sort(result.begin(), result.end(),
              [](const Cost_Edge & a, const Cost_Edge & b)
              {
                if (std::get<0>(a) != std::get<0>(b))
                  return std::get<0>(a) < std::get<0>(b);
                if (std::get<1>(a) != std::get<1>(b))
                  return std::get<1>(a) < std::get<1>(b);
                return std::get<2>(a) < std::get<2>(b);
              });

    return result;
  }


  Objective exact_minimum_cost_matching_optimum(size_t n,
                                                const std::vector<Cost_Edge> & edges,
                                                bool max_cardinality)
  {
    if (n == 0)
      return Objective{0, 0};

    ah_runtime_error_if(n > 24)
      << "exact_minimum_cost_matching_optimum supports n <= 24";

    constexpr long long NO_EDGE = std::numeric_limits<long long>::max() / 4;
    std::vector<std::vector<long long>> c(n, std::vector<long long>(n, NO_EDGE));

    for (auto [u, v, cost] : normalize_simple_edges(n, edges))
      {
        c[u][v] = cost;
        c[v][u] = cost;
      }

    const uint64_t full_mask = (uint64_t{1} << n) - 1;
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

      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      const uint64_t rest = mask & ~(uint64_t{1} << i);

      Objective best = solve(rest); // i unmatched

      uint64_t options = rest;
      while (options != 0)
        {
          const size_t j = static_cast<size_t>(std::countr_zero(options));
          options &= (options - 1);

          if (c[i][j] == NO_EDGE)
            continue;

          Objective candidate = solve(rest & ~(uint64_t{1} << j));
          ++candidate.cardinality;
          candidate.total_cost += c[i][j];

          if (better(candidate, best, max_cardinality))
            best = candidate;
        }

      seen[pos] = 1;
      memo[pos] = best;
      return best;
    };

    return solve(full_mask);
  }


  Min_Cost_Perfect_Matching_Result<long long>
  exact_minimum_cost_perfect_matching_optimum(size_t n,
                                              const std::vector<Cost_Edge> & edges)
  {
    if (n == 0)
      return Min_Cost_Perfect_Matching_Result<long long>{true, 0, 0};

    if (n % 2 == 1)
      return Min_Cost_Perfect_Matching_Result<long long>{false, 0, 0};

    const Objective best = exact_minimum_cost_matching_optimum(
        n, edges, true);
    if (best.cardinality != n / 2)
      return Min_Cost_Perfect_Matching_Result<long long>{false, 0, 0};

    return Min_Cost_Perfect_Matching_Result<long long>{
        true, best.total_cost, best.cardinality};
  }


  template <class GT>
  Objective compute_objective_from_matching(const DynDlist<typename GT::Arc *> & matching)
  {
    Objective obj;
    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        obj.total_cost += static_cast<long long>(arc->get_info());
        ++obj.cardinality;
      }
    return obj;
  }


  template <class GT>
  Objective solve_min_cost(const GT & g,
                           DynDlist<typename GT::Arc *> & matching,
                           bool max_cardinality)
  {
    const auto res = blossom_minimum_cost_matching<GT>(
        g,
        matching,
        Dft_Dist<GT>(),
        Dft_Show_Arc<GT>(),
        max_cardinality);

    EXPECT_EQ(res.cardinality, matching.size());
    const Objective obj = compute_objective_from_matching<GT>(matching);
    EXPECT_EQ(res.cardinality, obj.cardinality);
    EXPECT_EQ(res.total_cost, obj.total_cost);

    return obj;
  }


  template <class GT>
  Objective solve_backend(size_t n,
                          const std::vector<Cost_Edge> & edges,
                          bool max_cardinality)
  {
    GT g = build_graph<GT>(n, edges);
    DynDlist<typename GT::Arc *> matching;
    const Objective got = solve_min_cost(g, matching, max_cardinality);
    EXPECT_TRUE(verify_matching(g, matching));
    return got;
  }


  template <class GT>
  Min_Cost_Perfect_Matching_Result<long long>
  solve_backend_perfect(size_t n,
                        const std::vector<Cost_Edge> & edges)
  {
    GT g = build_graph<GT>(n, edges);
    DynDlist<typename GT::Arc *> matching;
    const auto got = blossom_minimum_cost_perfect_matching<GT>(g, matching);

    if (got.feasible)
      {
        EXPECT_EQ(got.cardinality, n / 2);
        EXPECT_EQ(matching.size(), got.cardinality);
        EXPECT_TRUE(verify_matching(g, matching));
      }
    else
      {
        EXPECT_TRUE(matching.is_empty());
      }

    return got;
  }
} // namespace


TEST(MinCostMatchingStandaloneTest, EmptyGraph)
{
  using Graph = List_Backend;

  Graph g;
  DynDlist<Graph::Arc *> matching;

  const auto result = blossom_minimum_cost_matching<Graph>(g, matching);
  EXPECT_EQ(result.cardinality, 0u);
  EXPECT_EQ(result.total_cost, 0);
  EXPECT_TRUE(matching.is_empty());
}


TEST(MinCostMatchingStandaloneTest, PositiveSingleEdgeIllustratesObjectiveModes)
{
  using Graph = List_Backend;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 9);

  DynDlist<Graph::Arc *> pure_matching;
  const auto pure = blossom_minimum_cost_matching<Graph>(g, pure_matching);
  EXPECT_EQ(pure.cardinality, 0u);
  EXPECT_EQ(pure.total_cost, 0);
  EXPECT_TRUE(pure_matching.is_empty());

  DynDlist<Graph::Arc *> card_matching;
  const auto card = blossom_minimum_cost_matching<Graph>(
      g,
      card_matching,
      Dft_Dist<Graph>(),
      Dft_Show_Arc<Graph>(),
      true);
  EXPECT_EQ(card.cardinality, 1u);
  EXPECT_EQ(card.total_cost, 9);
  EXPECT_EQ(card_matching.size(), 1u);
  EXPECT_TRUE(verify_matching(g, card_matching));
}


TEST(MinCostMatchingStandaloneTest, ParallelArcsUseCheapestArc)
{
  using Graph = List_Backend;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  auto * n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 11);
  auto * cheapest = g.insert_arc(n0, n1, -3);
  g.insert_arc(n2, n3, 4);
  g.insert_arc(n1, n2, 10);

  DynDlist<Graph::Arc *> matching;
  const auto result = blossom_minimum_cost_matching<Graph>(g, matching, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), true);

  EXPECT_EQ(result.cardinality, 2u);
  EXPECT_EQ(result.total_cost, 1);
  EXPECT_TRUE(verify_matching(g, matching));

  bool uses_cheapest_parallel = false;
  for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
    if (it.get_curr() == cheapest)
      uses_cheapest_parallel = true;

  EXPECT_TRUE(uses_cheapest_parallel);
}


TEST(MinCostMatchingStandaloneTest, DigraphThrowsDomainError)
{
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;

  Digraph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 7);

  DynDlist<Digraph::Arc *> matching;
  EXPECT_THROW(blossom_minimum_cost_matching<Digraph>(g, matching),
               std::domain_error);
}


TEST(MinCostMatchingStandaloneTest, LLONGMinCostThrowsOverflow)
{
  using Graph = List_Backend;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, std::numeric_limits<long long>::min());

  DynDlist<Graph::Arc *> matching;
  EXPECT_THROW(blossom_minimum_cost_matching<Graph>(g, matching),
               std::overflow_error);
}


TEST(MinCostMatchingStandaloneTest, UnsignedCostAboveLongLongRangeThrowsOverflow)
{
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<unsigned long long>>;

  UGraph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  const auto huge = static_cast<unsigned long long>(std::numeric_limits<long long>::max())
                    + 42ULL;
  g.insert_arc(n0, n1, huge);

  DynDlist<UGraph::Arc *> matching;
  EXPECT_THROW(blossom_minimum_cost_matching<UGraph>(g, matching),
               std::overflow_error);
}


TEST(MinCostMatchingStandaloneTest, FreeAliasAndFunctorAgree)
{
  using Graph = List_Backend;

  const std::vector<Cost_Edge> edges = {
      {0, 1, 5},
      {1, 2, -2},
      {2, 3, 7},
      {0, 3, 1},
      {0, 2, 4},
      {1, 3, 3}
  };

  Graph g = build_graph<Graph>(4, edges);

  DynDlist<Graph::Arc *> free_matching;
  DynDlist<Graph::Arc *> alias_matching;
  DynDlist<Graph::Arc *> functor_matching;

  const auto free_res = compute_minimum_cost_general_matching<Graph>(
      g, free_matching, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), true);

  const auto alias_res = blossom_minimum_cost_matching<Graph>(
      g, alias_matching, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), true);

  Compute_Minimum_Cost_General_Matching<Graph> solver(
      Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), true);
  const auto functor_res = solver(g, functor_matching);

  EXPECT_EQ(free_res.cardinality, alias_res.cardinality);
  EXPECT_EQ(free_res.total_cost, alias_res.total_cost);
  EXPECT_EQ(alias_res.cardinality, functor_res.cardinality);
  EXPECT_EQ(alias_res.total_cost, functor_res.total_cost);
}


TEST(MinCostMatchingStandaloneTest, MatchesNegatedWeightedBlossomObjective)
{
  using Graph = List_Backend;

  const std::vector<Cost_Edge> edges = {
      {0, 1, 8},
      {0, 2, -4},
      {1, 3, 2},
      {2, 3, 7},
      {2, 4, -3},
      {3, 5, 6},
      {4, 5, 1}
  };

  Graph g = build_graph<Graph>(6, edges);

  struct Negated_Cost
  {
    long long operator()(Graph::Arc * arc) const
    {
      return -arc->get_info();
    }
  };

  for (bool max_cardinality : {false, true})
    {
      DynDlist<Graph::Arc *> min_matching;
      const auto min_res = blossom_minimum_cost_matching<Graph>(
          g, min_matching, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), max_cardinality);

      DynDlist<Graph::Arc *> weighted_matching;
      const auto weighted_res = blossom_maximum_weight_matching<Graph, Negated_Cost>(
          g, weighted_matching, Negated_Cost(), Dft_Show_Arc<Graph>(), max_cardinality);

      EXPECT_EQ(min_res.cardinality, weighted_res.cardinality);
      EXPECT_EQ(min_res.total_cost, -weighted_res.total_weight);
    }
}


TEST(MinCostMatchingPerfectStandaloneTest, EmptyGraphIsPerfectlyMatchable)
{
  using Graph = List_Backend;

  Graph g;
  DynDlist<Graph::Arc *> matching;
  const auto result = blossom_minimum_cost_perfect_matching<Graph>(g, matching);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.cardinality, 0u);
  EXPECT_EQ(result.total_cost, 0);
  EXPECT_TRUE(matching.is_empty());
}


TEST(MinCostMatchingPerfectStandaloneTest, OddNodeCountIsInfeasible)
{
  using Graph = List_Backend;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 3);

  DynDlist<Graph::Arc *> matching;
  const auto result = blossom_minimum_cost_perfect_matching<Graph>(g, matching);

  EXPECT_FALSE(result.feasible);
  EXPECT_TRUE(matching.is_empty());
}


TEST(MinCostMatchingPerfectStandaloneTest, EvenNodeCountCanStillBeInfeasible)
{
  using Graph = List_Backend;

  Graph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  g.insert_node(3);
  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, 1);

  DynDlist<Graph::Arc *> matching;
  const auto result = blossom_minimum_cost_perfect_matching<Graph>(g, matching);

  EXPECT_FALSE(result.feasible);
  EXPECT_TRUE(matching.is_empty());
}


TEST(MinCostMatchingPerfectStandaloneTest, DigraphThrowsDomainError)
{
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;

  Digraph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 4);

  DynDlist<Digraph::Arc *> matching;
  EXPECT_THROW(blossom_minimum_cost_perfect_matching<Digraph>(g, matching),
               std::domain_error);
}


TEST(MinCostMatchingPerfectStandaloneTest, FeasiblePerfectMatchingMatchesExactOracle)
{
  using Graph = List_Backend;

  const std::vector<Cost_Edge> edges = {
      {0, 1, 9},
      {0, 2, 4},
      {0, 5, 2},
      {1, 2, 7},
      {1, 3, 3},
      {2, 4, 6},
      {3, 4, 5},
      {3, 5, 1},
      {4, 5, 8}
  };

  Graph g = build_graph<Graph>(6, edges);
  DynDlist<Graph::Arc *> matching;

  const auto got = blossom_minimum_cost_perfect_matching<Graph>(g, matching);
  const auto expected = exact_minimum_cost_perfect_matching_optimum(6, edges);

  ASSERT_TRUE(expected.feasible);
  EXPECT_TRUE(got.feasible);
  EXPECT_EQ(got.cardinality, expected.cardinality);
  EXPECT_EQ(got.total_cost, expected.total_cost);
  EXPECT_EQ(matching.size(), got.cardinality);
  EXPECT_TRUE(verify_matching(g, matching));
}


TEST(MinCostMatchingPerfectStandaloneTest, AliasAndFunctorAgree)
{
  using Graph = List_Backend;

  const std::vector<Cost_Edge> edges = {
      {0, 1, 5},
      {0, 3, 9},
      {1, 2, 2},
      {2, 3, 4}
  };

  Graph g = build_graph<Graph>(4, edges);

  DynDlist<Graph::Arc *> alias_matching;
  DynDlist<Graph::Arc *> free_matching;
  DynDlist<Graph::Arc *> functor_matching;

  const auto alias_res = blossom_minimum_cost_perfect_matching<Graph>(
      g, alias_matching);
  const auto free_res = compute_minimum_cost_perfect_general_matching<Graph>(
      g, free_matching);

  Compute_Minimum_Cost_Perfect_General_Matching<Graph> solver;
  const auto functor_res = solver(g, functor_matching);

  EXPECT_EQ(alias_res.feasible, free_res.feasible);
  EXPECT_EQ(alias_res.feasible, functor_res.feasible);
  EXPECT_EQ(alias_res.cardinality, free_res.cardinality);
  EXPECT_EQ(alias_res.cardinality, functor_res.cardinality);
  EXPECT_EQ(alias_res.total_cost, free_res.total_cost);
  EXPECT_EQ(alias_res.total_cost, functor_res.total_cost);
}


TEST(MinCostMatchingBackendsTest, DeterministicScenarioMatchesExactAcrossBackends)
{
  const std::vector<Cost_Edge> edges = {
      {0, 1, 5},
      {1, 2, -3},
      {2, 3, 4},
      {3, 4, -6},
      {4, 5, 5},
      {5, 0, 2},
      {0, 3, 1},
      {1, 4, 7},
      {2, 5, -2},
      {1, 3, 3}
  };

  for (bool max_cardinality : {false, true})
    {
      const Objective expected = exact_minimum_cost_matching_optimum(
          6, edges, max_cardinality);

      const Objective list_obj = solve_backend<List_Backend>(6, edges, max_cardinality);
      const Objective sgraph_obj = solve_backend<SGraph_Backend>(6, edges, max_cardinality);
      const Objective agraph_obj = solve_backend<AGraph_Backend>(6, edges, max_cardinality);

      EXPECT_EQ(list_obj, expected);
      EXPECT_EQ(sgraph_obj, expected);
      EXPECT_EQ(agraph_obj, expected);
    }
}


TEST(MinCostMatchingPerfectBackendsTest, RandomSmallGraphsMatchExactPerfectOracle)
{
  std::mt19937_64 rng(0xA11CE55ULL);
  std::uniform_int_distribution<int> n_half_dist(1, 5); // n in {2,4,6,8,10}
  std::uniform_int_distribution<int> c_dist(-15, 20);
  std::bernoulli_distribution edge_coin(0.45);
  std::bernoulli_distribution duplicate_coin(0.15);
  std::bernoulli_distribution loop_coin(0.08);

  for (size_t trial = 0; trial < 100; ++trial)
    {
      const size_t n = static_cast<size_t>(2 * n_half_dist(rng));
      std::vector<Cost_Edge> edges;
      edges.reserve(n * (n - 1));

      for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
          if (edge_coin(rng))
            {
              edges.emplace_back(i, j, static_cast<long long>(c_dist(rng)));
              if (duplicate_coin(rng))
                edges.emplace_back(i, j, static_cast<long long>(c_dist(rng)));
            }

      for (size_t i = 0; i < n; ++i)
        if (loop_coin(rng))
          edges.emplace_back(i, i, static_cast<long long>(c_dist(rng)));

      const auto expected = exact_minimum_cost_perfect_matching_optimum(n, edges);
      const auto list_obj = solve_backend_perfect<List_Backend>(n, edges);
      const auto sgraph_obj = solve_backend_perfect<SGraph_Backend>(n, edges);
      const auto agraph_obj = solve_backend_perfect<AGraph_Backend>(n, edges);

      EXPECT_EQ(list_obj.feasible, expected.feasible) << "trial=" << trial;
      EXPECT_EQ(sgraph_obj.feasible, expected.feasible) << "trial=" << trial;
      EXPECT_EQ(agraph_obj.feasible, expected.feasible) << "trial=" << trial;

      if (expected.feasible)
        {
          EXPECT_EQ(list_obj.cardinality, expected.cardinality) << "trial=" << trial;
          EXPECT_EQ(sgraph_obj.cardinality, expected.cardinality) << "trial=" << trial;
          EXPECT_EQ(agraph_obj.cardinality, expected.cardinality) << "trial=" << trial;
          EXPECT_EQ(list_obj.total_cost, expected.total_cost) << "trial=" << trial;
          EXPECT_EQ(sgraph_obj.total_cost, expected.total_cost) << "trial=" << trial;
          EXPECT_EQ(agraph_obj.total_cost, expected.total_cost) << "trial=" << trial;
        }
    }
}


TEST(MinCostMatchingBackendsTest, RandomSmallGraphsMatchExactOptimum)
{
  std::mt19937_64 rng(0xC0FFEEULL);
  std::uniform_int_distribution<int> n_dist(2, 10);
  std::uniform_int_distribution<int> c_dist(-15, 20);
  std::bernoulli_distribution edge_coin(0.45);
  std::bernoulli_distribution duplicate_coin(0.15);
  std::bernoulli_distribution loop_coin(0.10);

  for (size_t trial = 0; trial < 120; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Cost_Edge> edges;
      edges.reserve(n * (n - 1));

      for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
          if (edge_coin(rng))
            {
              edges.emplace_back(i, j, static_cast<long long>(c_dist(rng)));
              if (duplicate_coin(rng))
                edges.emplace_back(i, j, static_cast<long long>(c_dist(rng)));
            }

      for (size_t i = 0; i < n; ++i)
        if (loop_coin(rng))
          edges.emplace_back(i, i, static_cast<long long>(c_dist(rng)));

      for (bool max_cardinality : {false, true})
        {
          const Objective expected = exact_minimum_cost_matching_optimum(
              n, edges, max_cardinality);

          const Objective list_obj = solve_backend<List_Backend>(
              n, edges, max_cardinality);
          const Objective sgraph_obj = solve_backend<SGraph_Backend>(
              n, edges, max_cardinality);
          const Objective agraph_obj = solve_backend<AGraph_Backend>(
              n, edges, max_cardinality);

          EXPECT_EQ(list_obj, expected)
              << "trial=" << trial << ", mode=" << max_cardinality;
          EXPECT_EQ(sgraph_obj, expected)
              << "trial=" << trial << ", mode=" << max_cardinality;
          EXPECT_EQ(agraph_obj, expected)
              << "trial=" << trial << ", mode=" << max_cardinality;
        }
    }
}


TEST(MinCostMatchingBackendsTest, DISABLED_PerfRegressionLargeFixedSeed)
{
  using Clock = std::chrono::steady_clock;

  constexpr size_t n = 1400;
  constexpr size_t oracle_n = 24; // exact_* supports n <= 24

# ifdef NDEBUG
  constexpr long long kBudgetGeneralMs = 12000;
  constexpr long long kBudgetPerfectMs = 12000;
# else
  constexpr long long kBudgetGeneralMs = 30000;
  constexpr long long kBudgetPerfectMs = 30000;
# endif

  std::mt19937_64 rng(0xBADC0FFEE0DDF00DULL);
  std::uniform_int_distribution<int> c_dist(-100, 150);
  std::bernoulli_distribution edge_coin(0.015);
  std::bernoulli_distribution duplicate_coin(0.08);
  std::bernoulli_distribution loop_coin(0.02);

  std::vector<Cost_Edge> edges;
  edges.reserve(n * 18);

  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j)
      if (edge_coin(rng))
        {
          edges.emplace_back(i, j, static_cast<long long>(c_dist(rng)));
          if (duplicate_coin(rng))
            edges.emplace_back(i, j, static_cast<long long>(c_dist(rng)));
        }

  for (size_t i = 0; i < n; ++i)
    if (loop_coin(rng))
      edges.emplace_back(i, i, static_cast<long long>(c_dist(rng)));

  if (edges.empty())
    edges.emplace_back(0, 1, 1);

  std::vector<Cost_Edge> oracle_edges;
  oracle_edges.reserve(edges.size() / 4 + 1);
  for (const auto & [u, v, c] : edges)
    if (u < oracle_n and v < oracle_n)
      oracle_edges.emplace_back(u, v, c);

  // Sanity pass through the same exact/solver entry points used by
  // correctness tests, but on a bounded oracle-sized subproblem.
  for (bool max_cardinality : {false, true})
    {
      const Objective expected = exact_minimum_cost_matching_optimum(
          oracle_n, oracle_edges, max_cardinality);
      const Objective list_obj = solve_backend<List_Backend>(
          oracle_n, oracle_edges, max_cardinality);
      const Objective sgraph_obj = solve_backend<SGraph_Backend>(
          oracle_n, oracle_edges, max_cardinality);
      const Objective agraph_obj = solve_backend<AGraph_Backend>(
          oracle_n, oracle_edges, max_cardinality);

      ASSERT_EQ(list_obj, expected);
      ASSERT_EQ(sgraph_obj, expected);
      ASSERT_EQ(agraph_obj, expected);
    }

  const auto expected_perfect = exact_minimum_cost_perfect_matching_optimum(
      oracle_n, oracle_edges);
  const auto list_perfect_small = solve_backend_perfect<List_Backend>(
      oracle_n, oracle_edges);
  const auto sgraph_perfect_small = solve_backend_perfect<SGraph_Backend>(
      oracle_n, oracle_edges);
  const auto agraph_perfect_small = solve_backend_perfect<AGraph_Backend>(
      oracle_n, oracle_edges);

  ASSERT_EQ(list_perfect_small.feasible, expected_perfect.feasible);
  ASSERT_EQ(sgraph_perfect_small.feasible, expected_perfect.feasible);
  ASSERT_EQ(agraph_perfect_small.feasible, expected_perfect.feasible);
  if (expected_perfect.feasible)
    {
      ASSERT_EQ(list_perfect_small.cardinality, expected_perfect.cardinality);
      ASSERT_EQ(sgraph_perfect_small.cardinality, expected_perfect.cardinality);
      ASSERT_EQ(agraph_perfect_small.cardinality, expected_perfect.cardinality);
      ASSERT_EQ(list_perfect_small.total_cost, expected_perfect.total_cost);
      ASSERT_EQ(sgraph_perfect_small.total_cost, expected_perfect.total_cost);
      ASSERT_EQ(agraph_perfect_small.total_cost, expected_perfect.total_cost);
    }

  const auto general_start = Clock::now();
  const Objective list_general = solve_backend<List_Backend>(n, edges, true);
  const Objective sgraph_general = solve_backend<SGraph_Backend>(n, edges, true);
  const Objective agraph_general = solve_backend<AGraph_Backend>(n, edges, true);
  const auto general_end = Clock::now();
  const auto general_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      general_end - general_start).count();

  ASSERT_EQ(sgraph_general, list_general);
  ASSERT_EQ(agraph_general, list_general);
  ASSERT_LT(general_elapsed_ms, kBudgetGeneralMs)
      << "general perf regression: n=" << n
      << ", edges=" << edges.size()
      << ", elapsed_ms=" << general_elapsed_ms
      << ", budget_ms=" << kBudgetGeneralMs;

  const auto perfect_start = Clock::now();
  const auto list_perfect = solve_backend_perfect<List_Backend>(n, edges);
  const auto sgraph_perfect = solve_backend_perfect<SGraph_Backend>(n, edges);
  const auto agraph_perfect = solve_backend_perfect<AGraph_Backend>(n, edges);
  const auto perfect_end = Clock::now();
  const auto perfect_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      perfect_end - perfect_start).count();

  ASSERT_EQ(sgraph_perfect.feasible, list_perfect.feasible);
  ASSERT_EQ(agraph_perfect.feasible, list_perfect.feasible);
  if (list_perfect.feasible)
    {
      ASSERT_EQ(sgraph_perfect.cardinality, list_perfect.cardinality);
      ASSERT_EQ(agraph_perfect.cardinality, list_perfect.cardinality);
      ASSERT_EQ(sgraph_perfect.total_cost, list_perfect.total_cost);
      ASSERT_EQ(agraph_perfect.total_cost, list_perfect.total_cost);
    }

  ASSERT_LT(perfect_elapsed_ms, kBudgetPerfectMs)
      << "perfect perf regression: n=" << n
      << ", edges=" << edges.size()
      << ", elapsed_ms=" << perfect_elapsed_ms
      << ", budget_ms=" << kBudgetPerfectMs;
}
