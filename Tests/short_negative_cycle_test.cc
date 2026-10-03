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

/** @file short_negative_cycle_test.cc
 *  @brief Exact short-cycle search: topology, witnesses and an independent oracle.
 */

// Keep first: the public header must be self-contained.
# include <Negative_Cycles.H>
# include <gtest/gtest.h>
# include <algorithm>
# include <cmath>
# include <functional>
# include <limits>
# include <random>
# include <set>
# include <tuple>
# include <vector>
# include "negative_cycles_test_support.H"

using namespace Aleph;
using namespace Negative_Cycles_Test_Support;

namespace
{
  /** @brief Check alignment, closure and simplicity of a short witness. */
  template <class GT, typename Cost>
  void check_witness(const GT & g, const Short_Cycle_Result<GT, Cost> & r)
  {
    ASSERT_TRUE(r.has_cycle);
    ASSERT_GE(r.length, 1u);
    ASSERT_LE(r.length, 3u);
    ASSERT_EQ(r.cycle_nodes.size(), r.length + 1);
    ASSERT_EQ(r.cycle_arcs.size(), r.length);
    std::set<typename GT::Node *> seen;
    auto ni = r.cycle_nodes.get_it();
    auto * first = ni.get_curr();
    auto * current = first;
    for (auto ai = r.cycle_arcs.get_it(); ai.has_curr(); ai.next_ne())
      {
        ASSERT_TRUE(seen.insert(current).second);
        ASSERT_EQ(g.get_src_node(ai.get_curr()), current);
        ni.next_ne();
        ASSERT_TRUE(ni.has_curr());
        current = ni.get_curr();
        ASSERT_EQ(g.get_tgt_node(ai.get_curr()), current);
      }
    ASSERT_EQ(current, first);
    ni.next_ne();
    ASSERT_FALSE(ni.has_curr());
  }

  struct Optimum
  {
    bool exists = false;
    long long cost = 0;
    size_t length = 0;
  };

  /** @brief Enumerate original arcs, including every parallel choice.
   *  Uses none of the production adjacency, SCC, ranking or collapse helpers.
   */
  Optimum exhaustive(const size_t n, const std::vector<Edge_Def> & edges, const size_t bound)
  {
    Optimum best;
    std::vector<bool> seen(n, false);
    std::function<void(size_t, size_t, long long, size_t)> visit =
      [&](const size_t start, const size_t u, const long long cost, const size_t length)
      {
        if (length == bound)
          return;
        for (const auto & [a, v, w] : edges)
          if (a == u)
            {
              if (v == start)
                {
                  if (not best.exists or cost + w < best.cost
                      or (cost + w == best.cost and length + 1 < best.length))
                    best = {true, cost + w, length + 1};
                }
              else if (not seen[v])
                {
                  seen[v] = true;
                  visit(start, v, cost + w, length + 1);
                  seen[v] = false;
                }
            }
      };
    for (size_t s = 0; s < n; ++s)
      {
        seen[s] = true;
        visit(s, s, 0, 0);
        seen[s] = false;
      }
    return best;
  }
}

TEST(ShortNegativeCycleTest, EmptyAndAcyclicGraphsHaveNoWitness)
{
  for (const size_t n : {0u, 5u})
    {
      auto b = build_graph(n, n == 0 ? std::vector<Edge_Def>{}
                                    : std::vector<Edge_Def>{{0, 1, -5}, {1, 2, -4}, {3, 4, 1}});
      for (size_t length = 0; length <= 3; ++length)
        {
          const auto r = most_negative_cycle_up_to_3(b.g, length);
          EXPECT_FALSE(r.has_cycle);
          EXPECT_FALSE(r.is_negative());
          EXPECT_EQ(r.length, 0u);
          EXPECT_TRUE(r.cycle_nodes.is_empty());
          EXPECT_TRUE(r.cycle_arcs.is_empty());
        }
    }
}

TEST(ShortNegativeCycleTest, BoundSelectsLoopsPairsAndTriangles)
{
  auto b = build_graph(4, {{0, 0, 1}, {0, 1, 0}, {1, 0, -3},
                           {1, 2, 0}, {2, 0, -5}, {2, 3, -10}, {3, 0, -10}});
  for (size_t length = 1; length <= 3; ++length)
    {
      const auto r = most_negative_cycle_up_to_3(b.g, length);
      check_witness(b.g, r);
      EXPECT_EQ(r.length, length);
      EXPECT_EQ(r.total_cost, length == 1 ? 1 : length == 2 ? -3 : -5);
    }
  EXPECT_EQ(most_negative_cycle_up_to_3(b.g).total_cost, -5);
  auto ring = build_graph(4, {{0, 1, -1}, {1, 2, -1}, {2, 3, -1}, {3, 0, -1}});
  EXPECT_FALSE(most_negative_cycle_up_to_3(ring.g).has_cycle);
}

TEST(ShortNegativeCycleTest, SelfLoopsCannotHideACheaperTriangle)
{
  // Each best length-three relaxed walk costs -7: a -3 pair plus an
  // interior -4 loop. It hides the simple -6 triangle from the DP.
  auto b = build_graph(3, {{0, 0, -4}, {1, 1, -4}, {2, 2, -4},
                           {0, 1, -2}, {1, 2, -2}, {2, 0, -2},
                           {1, 0, -1}, {2, 1, -1}, {0, 2, -1}});
  const auto r = most_negative_cycle_up_to_3(b.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -6);
  EXPECT_EQ(r.length, 3u);
  check_witness(b.g, r);
  const auto relaxed = most_negative_cycle_bounded(b.g, 3);
  EXPECT_EQ(relaxed.total_cost, -4);
  EXPECT_FALSE(relaxed.is_exact);
}

TEST(ShortNegativeCycleTest, BoundsAreCheckedBeforeAccessingTheGraph)
{
  auto b = build_graph(1, {{0, 0, -1}});
  int reads = 0, filters = 0;
  EXPECT_FALSE(most_negative_cycle_up_to_3(b.g, 0, Counting_Dist{&reads},
                                          Counting_Filter{&filters}).has_cycle);
  EXPECT_THROW(most_negative_cycle_up_to_3(b.g, 4, Counting_Dist{&reads},
                                          Counting_Filter{&filters}), std::invalid_argument);
  EXPECT_THROW(most_negative_cycle_up_to_3(b.g, SIZE_MAX), std::invalid_argument);
  EXPECT_EQ(reads, 0);
  EXPECT_EQ(filters, 0);
  UGraph undirected;
  EXPECT_FALSE(most_negative_cycle_up_to_3(undirected, 0).has_cycle);
  EXPECT_THROW(most_negative_cycle_up_to_3(undirected), std::domain_error);
}

TEST(ShortNegativeCycleTest, ParallelArcsKeepTheirIdentityAndRespectTheFilter)
{
  auto b = build_graph(3, {{0, 1, 10}, {0, 1, -2}, {0, 1, -2},
                           {1, 2, -1}, {2, 0, 0}});
  const auto r = most_negative_cycle_up_to_3(b.g);
  check_witness(b.g, r);
  EXPECT_EQ(r.total_cost, -3);
  EXPECT_EQ(r.cycle_arcs.get_first(), b.arcs[1]);
  const auto hidden = most_negative_cycle_up_to_3(b.g, 3, Dft_Dist<Graph>(), Hide_Arc{b.arcs[1]});
  EXPECT_EQ(hidden.total_cost, -3);
  EXPECT_EQ(hidden.cycle_arcs.get_first(), b.arcs[2]);
}

TEST(ShortNegativeCycleTest, GlobalTiesPreferTheShortestCycle)
{
  auto b = build_graph(6, {{0, 1, 0}, {1, 2, 0}, {2, 0, -3},
                           {3, 4, 0}, {4, 3, -3}, {5, 5, -3}});
  EXPECT_EQ(most_negative_cycle_up_to_3(b.g).length, 1u);
  const auto r = most_negative_cycle_up_to_3(b.g, 3, Dft_Dist<Graph>(), Hide_Arc{b.arcs[5]});
  EXPECT_EQ(r.length, 2u);
  EXPECT_EQ(r.total_cost, -3);
}

TEST(ShortNegativeCycleTest, FloatingCancellationCannotHideTheBestTriangle)
{
  const double huge = std::ldexp(1.0, 100);
  auto b = build_graph_generic<Float_Graph, double>(4,
    {{0, 1, huge}, {1, 2, -1}, {2, 0, -huge}, {3, 3, -0.5}});
  const auto r = most_negative_cycle_up_to_3(b.g);
  check_witness(b.g, r);
  EXPECT_EQ(r.length, 3u);
  EXPECT_EQ(r.total_cost, -1);
  EXPECT_TRUE(r.is_negative());
}

TEST(ShortNegativeCycleTest, FloatingTiesAreDecidedBeforeRoundingTheTotal)
{
  // Both totals round to -1, but the triangle is strictly cheaper.
  const double tiny = std::ldexp(1.0, -100);
  auto b = build_graph_generic<Float_Graph, double>(4,
    {{0, 1, -1}, {1, 2, -tiny}, {2, 0, 0}, {3, 3, -1}});
  const auto r = most_negative_cycle_up_to_3(b.g);
  EXPECT_EQ(r.total_cost, -1);
  EXPECT_EQ(r.length, 3u);
}

TEST(ShortNegativeCycleTest, OppositeExtremeWeightsDoNotOverflowTheirComparison)
{
  const double m = std::numeric_limits<double>::max();
  auto b = build_graph_generic<Float_Graph, double>(2, {{0, 0, m}, {1, 1, -m}});
  for (size_t bound = 1; bound <= 3; ++bound)
    EXPECT_EQ(most_negative_cycle_up_to_3(b.g, bound).total_cost, -m);
}

TEST(ShortNegativeCycleTest, IntegersCheckOverflowButDoNotReserveMaximumAsInfinity)
{
  const long long m = std::numeric_limits<long long>::max();
  auto one = build_graph(1, {{0, 0, m}});
  EXPECT_EQ(most_negative_cycle_up_to_3(one.g).total_cost, m);
  auto cycle = build_graph(2, {{0, 1, m}, {1, 0, 1}});
  EXPECT_THROW(most_negative_cycle_up_to_3(cycle.g), std::overflow_error);
  auto dag = build_graph(3, {{0, 1, m}, {1, 2, 1}});
  EXPECT_FALSE(most_negative_cycle_up_to_3(dag.g).has_cycle);
}

TEST(ShortNegativeCycleTest, NonFiniteWeightsAreValidatedOnlyAfterFiltering)
{
  for (const double invalid : {std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()})
    {
      auto b = build_graph_generic<Float_Graph, double>(2, {{0, 0, -1}, {0, 1, invalid}});
      EXPECT_THROW(most_negative_cycle_up_to_3(b.g), std::domain_error);
      EXPECT_EQ(most_negative_cycle_up_to_3(b.g, 3, Dft_Dist<Float_Graph>(), Finite_Arcs()).total_cost, -1);
    }
}

TEST(ShortNegativeCycleTest, ReadsWeightsOnceAndPreservesGraphState)
{
  auto b = build_graph(4, {{0, 1, -1}, {0, 1, 5}, {1, 2, 0}, {2, 0, 0}, {3, 3, -2}});
  Planted_State<Graph> state;
  state.plant(b);
  for (size_t bound = 1; bound <= 3; ++bound)
    {
      int reads = 0;
      const auto r = most_negative_cycle_up_to_3(b.g, bound, Counting_Dist{&reads}, Hide_Arc{b.arcs[1]});
      EXPECT_TRUE(r.has_cycle);
      EXPECT_EQ(reads, 4);
      EXPECT_TRUE(state.intact(b));
    }
}

TEST(ShortNegativeCycleTest, ArrayDigraphAndUnsignedWeightsAreSupported)
{
  auto a = build_graph_generic<Arr_Digraph, long long>(3, {{0, 1, -3}, {1, 2, 1}, {2, 0, 1}});
  const auto r = most_negative_cycle_up_to_3(a.g);
  check_witness(a.g, r);
  EXPECT_EQ(r.total_cost, -1);
  using U = List_Digraph<Graph_Node<int>, Graph_Arc<unsigned long long>>;
  auto b = build_graph_generic<U, unsigned long long>(3, {{0, 1, 2}, {1, 2, 3}, {2, 0, 1}});
  EXPECT_EQ(most_negative_cycle_up_to_3(b.g).total_cost, 6u);
}

TEST(ShortNegativeCycleTest, RandomMultigraphsAgreeWithExhaustiveEnumerationAndTheDPBound)
{
  std::mt19937_64 rng(0xD420261002ULL);
  for (size_t trial = 0; trial < 3000; ++trial)
    {
      const size_t n = 1 + rng() % 8, m = rng() % (n * n + 4);
      std::vector<Edge_Def> edges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n, v = rng() % n;
          const long long w = static_cast<long long>(rng() % 17) - 8;
          edges.emplace_back(u, v, w);
        }
      auto b = build_graph(n, edges);
      for (size_t bound = 1; bound <= 3; ++bound)
        {
          const auto expected = exhaustive(n, edges, bound);
          const auto r = most_negative_cycle_up_to_3(b.g, bound);
          const auto dp = most_negative_cycle_bounded(b.g, bound);
          ASSERT_EQ(r.has_cycle, expected.exists) << trial << ":" << bound;
          ASSERT_EQ(r.has_cycle, dp.has_cycle) << trial << ":" << bound;
          if (not r.has_cycle)
            continue;
          ASSERT_EQ(r.total_cost, expected.cost) << trial << ":" << bound;
          ASSERT_EQ(r.length, expected.length) << trial << ":" << bound;
          ASSERT_LE(r.total_cost, dp.total_cost) << trial << ":" << bound;
          if (dp.is_exact)
            ASSERT_EQ(r.total_cost, dp.total_cost) << trial << ":" << bound;
          check_witness(b.g, r);
        }
    }
}
