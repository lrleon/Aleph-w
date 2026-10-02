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
 * @file howard_min_mean_cycle_test.cc
 * @brief Tests for howard_minimum_mean_cycle() (Howard_Min_Mean_Cycle.H),
 *        against Karp's algorithm and an exhaustive oracle.
 */

// Self-containment check: must stay the first include.
# include <Howard_Min_Mean_Cycle.H>

# include <gtest/gtest.h>

# include <cmath>
# include <limits>
# include <random>
# include <set>
# include <thread>
# include <tuple>
# include <vector>

# include <Min_Mean_Cycle.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>

# include "negative_cycles_test_support.H"

using namespace Aleph;
using namespace Negative_Cycles_Test_Support;

namespace
{
  using Arith = howard_detail::Arithmetic<long long, true>;

  /// A cycle as the exhaustive oracle sees it: total cost and number of arcs.
  /// `Acc` is the type of the total: `long long` for small costs and `__int128`
  /// for costs near 1e18, whose cycle totals do not fit a `long long`.
  template <typename Acc>
  struct Basic_Mean
  {
    bool exists = false;
    Acc total = 0;
    long long length = 1;

    /// True if this mean is strictly smaller than `total / length`.
    bool less_than(const Acc t, const long long l) const
    {
      return total * l < t * length;
    }
  };

  using Mean = Basic_Mean<long long>;

  /// Exhaustive minimum mean over the simple cycles (small graphs).
  template <typename Acc = long long, class Built>
  Basic_Mean<Acc> exact_minimum_mean(const Built & built)
  {
    using GT = decltype(built.g);
    const size_t n = built.nodes.size();
    std::vector<bool> visited(n, false);
    Basic_Mean<Acc> best;

    std::function<void(size_t, size_t, Acc, long long)> dfs =
      [&](const size_t start, const size_t u, const Acc total, const long long len)
    {
      for (Node_Arc_Iterator<std::remove_cv_t<std::remove_reference_t<GT>>> it(built.nodes[u]);
           it.has_curr(); it.next_ne())
        {
          const size_t v = static_cast<size_t>(it.get_tgt_node()->get_info());
          const Acc w = it.get_current_arc_ne()->get_info();
          if (v == start)
            {
              if (not best.exists or (total + w) * best.length < best.total * (len + 1))
                best = Basic_Mean<Acc>{true, total + w, len + 1};
              continue;
            }
          if (v < start or visited[v])
            continue;
          visited[v] = true;
          dfs(start, v, total + w, len + 1);
          visited[v] = false;
        }
    };

    for (size_t s = 0; s < n; ++s)
      {
        visited[s] = true;
        dfs(s, s, 0, 0);
        visited[s] = false;
      }
    return best;
  }

  // The witness must be a simple cycle of consecutive graph arcs whose total,
  // length and mean agree with the result. The agreement is checked with a
  // plain sum, which adversarial weights defeat; tests built on them pass
  // `check_values = false` and compare the cost with its known exact value.
  template <class GT, typename Cost>
  bool witness_is_simple_cycle(const GT & g, const Howard_Mean_Cycle_Result<GT, Cost> & r,
                               const bool check_values = true)
  {
    if (not r.has_cycle)
      return r.cycle_length == 0 and r.cycle_nodes.is_empty() and r.cycle_arcs.is_empty();

    if (r.cycle_length == 0 or r.cycle_nodes.size() != r.cycle_length + 1
        or r.cycle_arcs.size() != r.cycle_length)
      return false;

    std::set<typename GT::Node *> seen;
    auto node_it = r.cycle_nodes.get_it();
    typename GT::Node * first = node_it.get_curr();
    typename GT::Node * curr = first;
    node_it.next_ne();

    long double sum = 0.0L;
    for (auto arc_it = r.cycle_arcs.get_it(); arc_it.has_curr(); arc_it.next_ne())
      {
        typename GT::Arc * arc = arc_it.get_curr();
        if (not node_it.has_curr() or g.get_src_node(arc) != curr
            or g.get_tgt_node(arc) != node_it.get_curr() or not seen.insert(curr).second)
          return false;
        sum += static_cast<long double>(arc->get_info());
        curr = node_it.get_curr();
        node_it.next_ne();
      }

    if (curr != first or node_it.has_curr() or r.witness_node != first)
      return false;
    if (not check_values)
      return true;
    const long double mean = sum / static_cast<long double>(r.cycle_length);
    if constexpr (std::is_floating_point_v<Cost>)
      return std::fabs(mean - r.minimum_mean) <= 1e-9L * (1.0L + std::fabs(mean))
             and std::fabs(sum - static_cast<long double>(r.cycle_total_cost))
                 <= 1e-9L * (1.0L + std::fabs(sum));
    else
      return mean == r.minimum_mean and sum == static_cast<long double>(r.cycle_total_cost);
  }
} // namespace


// ---------------------------------------------------------------------------
// The cases of min_mean_cycle_test.cc, run against Howard.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, EmptyDigraphReturnsNoCycle)
{
  Graph g;
  const auto r = howard_minimum_mean_cycle(g);
  EXPECT_FALSE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_EQ(r.cycle_length, 0u);
  EXPECT_TRUE(witness_is_simple_cycle(g, r));
}


TEST(HowardMinMeanCycleTest, DagReturnsNoCycle)
{
  auto built = build_graph(4, {{0, 1, -5}, {1, 2, -5}, {0, 2, -9}, {2, 3, -1}});
  const auto r = howard_minimum_mean_cycle(built.g);
  EXPECT_FALSE(r.has_cycle);
  EXPECT_FALSE(howard_minimum_mean_cycle_value(built.g).has_cycle);
}


TEST(HowardMinMeanCycleTest, SelfLoopCanBeOptimalCycle)
{
  auto built = build_graph(3, {{0, 0, -2}, {0, 1, 4}, {1, 2, 4}, {2, 0, 4}});
  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_EQ(r.cycle_total_cost, -2);
  EXPECT_EQ(r.minimum_mean, -2.0L);
  EXPECT_EQ(r.witness_node, built.nodes[0]);
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(HowardMinMeanCycleTest, ChoosesMinimumAmongMultipleCycles)
{
  // Means: 0-1-0 is 1.5, 2-3-4-2 is 1, 5-6-5 is 2.
  auto built = build_graph(7, {{0, 1, 1}, {1, 0, 2},
                               {2, 3, 1}, {3, 4, 1}, {4, 2, 1},
                               {5, 6, 2}, {6, 5, 2}, {1, 2, 9}});
  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.minimum_mean, 1.0L);
  EXPECT_EQ(r.cycle_length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(HowardMinMeanCycleTest, SupportsNegativeMeanCycles)
{
  auto built = build_graph(3, {{0, 1, 2}, {1, 2, -7}, {2, 0, 1}});
  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.cycle_total_cost, -4);
  EXPECT_EQ(r.cycle_length, 3u);
  EXPECT_NEAR(static_cast<double>(r.minimum_mean), -4.0 / 3.0, 1e-15);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(HowardMinMeanCycleTest, ArcFilterChangesResult)
{
  auto built = build_graph(4, {{0, 1, -4}, {1, 0, 1}, {2, 3, -2}, {3, 2, -2}});

  const auto full = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(full.has_cycle);
  EXPECT_EQ(full.minimum_mean, -2.0L);

  const Hide_Arc filter{built.arcs[2]};   // 2 -> 3: no cycle remains there
  const auto filtered = howard_minimum_mean_cycle<Graph, Dft_Dist<Graph>, Hide_Arc>(
      built.g, Dft_Dist<Graph>(), filter);
  ASSERT_TRUE(filtered.has_cycle);
  EXPECT_EQ(filtered.minimum_mean, -1.5L);
  EXPECT_EQ(filtered.cycle_length, 2u);
}


TEST(HowardMinMeanCycleTest, FloatingWeightsAndValueOnlyApiWork)
{
  auto built = build_graph_generic<Float_Graph, double>(
      3, {{0, 1, 0.5}, {1, 2, -1.25}, {2, 0, 0.25}, {2, 2, 0.1}});

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_NEAR(static_cast<double>(r.minimum_mean), -0.5 / 3.0, 1e-12);
  EXPECT_NEAR(r.cycle_total_cost, -0.5, 1e-12);
  EXPECT_EQ(r.cycle_length, 3u);
  // Found with long double arithmetic, then verified with the weights scaled
  // to 64-bit integers: exact.
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));

  const auto value = howard_minimum_mean_cycle_value(built.g);
  ASSERT_TRUE(value.has_cycle);
  EXPECT_EQ(value.minimum_mean, r.minimum_mean);
}


TEST(HowardMinMeanCycleTest, NonFiniteFloatingWeightsThrowDomainError)
{
  Float_Graph inf_graph;
  auto * a = inf_graph.insert_node(0);
  auto * b = inf_graph.insert_node(1);
  inf_graph.insert_arc(a, b, std::numeric_limits<double>::infinity());
  inf_graph.insert_arc(b, a, 1.0);
  EXPECT_THROW((howard_minimum_mean_cycle(inf_graph)), std::domain_error);

  Float_Graph nan_graph;
  auto * c = nan_graph.insert_node(0);
  nan_graph.insert_arc(c, c, std::numeric_limits<double>::quiet_NaN());
  EXPECT_THROW((howard_minimum_mean_cycle(nan_graph)), std::domain_error);

  // A non-finite weight the filter hides is ignored.
  const auto r = howard_minimum_mean_cycle<Float_Graph, Dft_Dist<Float_Graph>, Finite_Arcs>(
      inf_graph, Dft_Dist<Float_Graph>(), Finite_Arcs());
  EXPECT_FALSE(r.has_cycle);
}


TEST(HowardMinMeanCycleTest, IntegerOverflowThrowsLikeKarp)
{
  // The only cycle costs max + 1: its total cannot be represented, Howard
  // hands the problem to Karp, and Karp's answer (an overflow) comes out.
  const long long M = std::numeric_limits<long long>::max();
  auto built = build_graph(2, {{0, 1, M - 1}, {1, 0, 2}});
  EXPECT_THROW((karp_minimum_mean_cycle(built.g)), std::overflow_error);
  EXPECT_THROW((howard_minimum_mean_cycle(built.g)), std::overflow_error);
}


TEST(HowardMinMeanCycleTest, SupportsArrayDigraphBackend)
{
  auto built = build_graph_generic<Arr_Digraph, long long>(
      3, {{0, 1, 3}, {1, 2, -5}, {2, 0, 1}, {1, 1, 4}});
  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.cycle_total_cost, -1);
  EXPECT_EQ(r.cycle_length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(HowardMinMeanCycleTest, UndirectedGraphThrowsDomainError)
{
  UGraph undirected;
  auto * u0 = undirected.insert_node(0);
  auto * u1 = undirected.insert_node(1);
  undirected.insert_arc(u0, u1, 1);
  EXPECT_THROW((howard_minimum_mean_cycle(undirected)), std::domain_error);
  EXPECT_THROW((howard_minimum_mean_cycle_value(undirected)), std::domain_error);
}


// ---------------------------------------------------------------------------
// What differs from Karp.
// ---------------------------------------------------------------------------

namespace
{
  // Accepts the result of either algorithm: they share their fields.
  template <class Result>
  long double mean_of(const Result & r)
  {
    return r.has_cycle ? r.minimum_mean : std::numeric_limits<long double>::infinity();
  }
}


TEST(HowardMinMeanCycleTest, ResultCanReplaceKarpsResult)
{
  auto built = build_graph(3, {{0, 1, 2}, {1, 2, -7}, {2, 0, 1}});
  const auto howard = howard_minimum_mean_cycle(built.g);
  const auto karp = karp_minimum_mean_cycle(built.g);
  EXPECT_EQ(mean_of(howard), mean_of(karp));

  const Min_Mean_Cycle_Result<Graph, long long> as_karp = howard;   // slicing keeps the fields
  EXPECT_EQ(as_karp.minimum_mean, howard.minimum_mean);
  EXPECT_EQ(as_karp.cycle_total_cost, howard.cycle_total_cost);
}


TEST(HowardMinMeanCycleTest, WitnessIsASimpleCycleWhereKarpsIsAClosedWalk)
{
  // A negative triangle among isolated vertices: Karp's witness is an n-step
  // closed walk that goes around the triangle many times.
  std::vector<Edge_Def> edges = {{0, 1, -1}, {1, 2, -1}, {2, 0, -1}};
  auto built = build_graph(60, edges);

  const auto howard = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(howard.has_cycle);
  EXPECT_EQ(howard.cycle_length, 3u);
  EXPECT_EQ(howard.cycle_total_cost, -3);
  EXPECT_EQ(howard.minimum_mean, -1.0L);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, howard));

  const auto karp = karp_minimum_mean_cycle(built.g);
  ASSERT_TRUE(karp.has_cycle);
  EXPECT_EQ(karp.minimum_mean, howard.minimum_mean);   // same value, possibly another witness
}


TEST(HowardMinMeanCycleTest, MaxCostIsAnOrdinaryCost)
{
  // numeric_limits::max() is not reserved as "unreachable" (Karp reserves it).
  const long long M = std::numeric_limits<long long>::max();
  auto built = build_graph(1, {{0, 0, M}});
  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.cycle_total_cost, M);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_FALSE(r.used_karp);

  const long long lowest = std::numeric_limits<long long>::min();
  auto low = build_graph(1, {{0, 0, lowest}});
  const auto r_low = howard_minimum_mean_cycle(low.g);
  ASSERT_TRUE(r_low.has_cycle);
  EXPECT_EQ(r_low.cycle_total_cost, lowest);
  EXPECT_EQ(r_low.cycle_length, 1u);
  EXPECT_FALSE(r_low.used_karp);
}


TEST(HowardMinMeanCycleTest, AnswersWhereKarpsSumsLeaveTheCostType)
{
  // int costs near the limits of the type: Karp adds up to n of them in its
  // table, which overflowed int even where the cycle is a loop; Howard needs
  // only the cycle it reports. Since the audit's C6 Karp adds them up in
  // long long and answers too.
  using IntGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  auto built = build_graph_generic<IntGraph, int>(
      4, {{2, 2, 653666155}, {2, 2, 585777688}, {1, 3, 641209630}, {3, 0, 491799673},
          {3, 3, -1900485882}});

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.cycle_total_cost, -1900485882);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_FALSE(r.used_karp);

  const auto karp = karp_minimum_mean_cycle(built.g);
  ASSERT_TRUE(karp.has_cycle);
  EXPECT_EQ(karp.cycle_total_cost, -1900485882);
  EXPECT_EQ(karp.minimum_mean, r.minimum_mean);

  // An acyclic graph with large weights has no cycle: no overflow either.
  auto dag = build_graph_generic<IntGraph, int>(
      4, {{0, 1, 2000000000}, {1, 2, 2000000000}, {2, 3, -2000000000}});
  EXPECT_FALSE(howard_minimum_mean_cycle(dag.g).has_cycle);
}


TEST(HowardMinMeanCycleTest, DistanceIsCalledOncePerArc)
{
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {0, 2, 7}, {2, 2, 4}});
  int dist_calls = 0;
  int filter_calls = 0;

  const auto r = howard_minimum_mean_cycle<Graph, Counting_Dist, Counting_Filter>(
      built.g, Counting_Dist{&dist_calls}, Counting_Filter{&filter_calls, built.arcs[3]});

  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_EQ(filter_calls, 5);
  EXPECT_EQ(dist_calls, 4);
}


TEST(HowardMinMeanCycleTest, NonNullCookiesAndBitsAreLeftUntouched)
{
  auto built = build_graph(4, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {2, 3, -2}, {3, 2, 1}});
  Planted_State<Graph> planted;
  planted.plant(built);
  ASSERT_TRUE(planted.intact(built));

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_TRUE(planted.intact(built));
}


// ---------------------------------------------------------------------------
// Agreement with Karp and with an exhaustive oracle.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, RandomSmallGraphsMatchTheExactOracleAndKarp)
{
  std::mt19937_64 rng(0x4F5741524431ULL);
  std::uniform_int_distribution<int> n_dist(1, 7);
  std::bernoulli_distribution has_edge(0.4);
  std::bernoulli_distribution has_loop(0.12);
  std::uniform_int_distribution<int> weight_dist(-6, 9);

  size_t with_cycle = 0;
  for (size_t trial = 0; trial < 400; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Edge_Def> edges;
      for (size_t u = 0; u < n; ++u)
        for (size_t v = 0; v < n; ++v)
          if ((u == v) ? has_loop(rng) : has_edge(rng))
            {
              edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
              if (rng() % 7 == 0)   // a parallel arc
                edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
            }

      auto built = build_graph(n, edges);
      const Mean oracle = exact_minimum_mean(built);
      const auto howard = howard_minimum_mean_cycle(built.g);
      const auto karp = karp_minimum_mean_cycle(built.g);

      ASSERT_EQ(howard.has_cycle, oracle.exists) << "trial=" << trial;
      ASSERT_EQ(howard.has_cycle, karp.has_cycle) << "trial=" << trial;
      ASSERT_FALSE(howard.used_karp) << "trial=" << trial;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, howard)) << "trial=" << trial;
      if (not howard.has_cycle)
        continue;

      ++with_cycle;
      // Exactly the oracle's rational mean: total / length cross-multiplied.
      ASSERT_EQ(howard.cycle_total_cost * oracle.length, oracle.total * static_cast<long long>(howard.cycle_length))
          << "trial=" << trial;
      ASSERT_EQ(howard.minimum_mean, karp.minimum_mean) << "trial=" << trial;
    }
  EXPECT_GT(with_cycle, 150u);
}


TEST(HowardMinMeanCycleTest, RandomLargerGraphsMatchKarpExactly)
{
  std::mt19937_64 rng(0xB16B00B5ULL);
  std::uniform_int_distribution<int> n_dist(8, 60);
  std::uniform_int_distribution<int> weight_dist(-40, 90);

  for (size_t trial = 0; trial < 120; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      const size_t m = n + static_cast<size_t>(rng() % (4 * n));
      std::vector<Edge_Def> edges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
        }

      auto built = build_graph(n, edges);
      const auto howard = howard_minimum_mean_cycle(built.g);
      const auto karp = karp_minimum_mean_cycle(built.g);

      ASSERT_EQ(howard.has_cycle, karp.has_cycle) << "trial=" << trial;
      ASSERT_FALSE(howard.used_karp) << "trial=" << trial;
      if (not howard.has_cycle)
        continue;
      ASSERT_EQ(howard.minimum_mean, karp.minimum_mean) << "trial=" << trial;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, howard)) << "trial=" << trial;
    }
}


TEST(HowardMinMeanCycleTest, RandomFloatingGraphsMatchKarpWithinTolerance)
{
  std::mt19937_64 rng(0xF10A7ULL);
  std::uniform_int_distribution<int> n_dist(2, 40);
  std::uniform_real_distribution<double> weight_dist(-1.0, 2.0);

  for (size_t trial = 0; trial < 150; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      const size_t m = n + static_cast<size_t>(rng() % (3 * n));
      std::vector<std::tuple<size_t, size_t, double>> edges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          edges.emplace_back(u, v, weight_dist(rng));
        }

      auto built = build_graph_generic<Float_Graph, double>(n, edges);
      const auto howard = howard_minimum_mean_cycle(built.g);
      const auto karp = karp_minimum_mean_cycle(built.g);

      ASSERT_EQ(howard.has_cycle, karp.has_cycle) << "trial=" << trial;
      ASSERT_FALSE(howard.used_karp) << "trial=" << trial;   // no oscillation from rounding noise
      if (not howard.has_cycle)
        continue;
      ASSERT_NEAR(static_cast<double>(howard.minimum_mean), static_cast<double>(karp.minimum_mean), 1e-12)
          << "trial=" << trial;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, howard)) << "trial=" << trial;
    }
}


// The tolerance of the floating-point bias comparison has two ways to be
// wrong, and each has its own test: too large and the iteration stops before
// the policy is optimal; too small and rounding noise looks like an
// improvement, so the policy oscillates until Karp takes over.
TEST(HowardMinMeanCycleTest, SmallRangeFloatingWeightsDoNotStopTheIterationEarly)
{
  std::mt19937_64 rng(0x5A11A11ULL);
  std::uniform_real_distribution<double> weight(-0.001, 0.003);

  for (size_t trial = 0; trial < 1500; ++trial)
    {
      const size_t n = 3 + rng() % 28;
      const size_t m = n + rng() % (3 * n);
      std::vector<std::tuple<size_t, size_t, double>> edges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          edges.emplace_back(u, v, weight(rng));
        }

      auto built = build_graph_generic<Float_Graph, double>(n, edges);
      const auto howard = howard_minimum_mean_cycle(built.g);
      const auto karp = karp_minimum_mean_cycle(built.g);

      ASSERT_EQ(howard.has_cycle, karp.has_cycle) << "trial=" << trial;
      if (not howard.has_cycle)
        continue;
      ASSERT_NEAR(static_cast<double>(howard.minimum_mean), static_cast<double>(karp.minimum_mean), 1e-12)
          << "trial=" << trial;
    }
}


TEST(HowardMinMeanCycleTest, DecimalWeightsWithRoundingNoiseDoNotMakeThePolicyOscillate)
{
  // 0.1, 0.2, 0.3... are not representable: cycles of equal mean differ by
  // rounding noise.
  std::mt19937_64 rng(0xDEC1A1ULL);
  const double menu[] = {0.1, 0.2, 0.3, 0.7, -0.1, 0.4};

  for (size_t trial = 0; trial < 12000; ++trial)
    {
      const size_t n = 3 + rng() % 12;
      const size_t m = n + rng() % (3 * n);
      std::vector<std::tuple<size_t, size_t, double>> edges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          edges.emplace_back(u, v, menu[rng() % 6]);
        }

      auto built = build_graph_generic<Float_Graph, double>(n, edges);
      const auto howard = howard_minimum_mean_cycle(built.g);
      ASSERT_FALSE(howard.used_karp) << "trial=" << trial;
      const auto karp = karp_minimum_mean_cycle(built.g);
      ASSERT_EQ(howard.has_cycle, karp.has_cycle) << "trial=" << trial;
      if (howard.has_cycle)
        ASSERT_NEAR(static_cast<double>(howard.minimum_mean), static_cast<double>(karp.minimum_mean), 1e-12)
            << "trial=" << trial;
    }
}


TEST(HowardMinMeanCycleTest, EachComponentKeepsItsOwnMeanAndTheSmallestWins)
{
  // Two strongly connected components joined one way, plus an isolated node.
  // Means: {0,1,2} is 3 / 3 = 1, {3,4} is (-1 + 1) / 2 = 0, the bridge is on no cycle.
  auto built = build_graph(6, {{0, 1, 1}, {1, 2, 1}, {2, 0, 1},
                               {3, 4, -1}, {4, 3, 1},
                               {2, 3, -100}});
  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.minimum_mean, 0.0L);
  EXPECT_EQ(r.cycle_length, 2u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


// ---------------------------------------------------------------------------
// The two ways out to Karp, and the oscillation that used to need them.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, ReferenceNodeOfACycleDoesNotMakeThePolicyOscillate)
{
  // With the reference of the biases taken as the first node of a cycle that
  // the walk met, the second phase of the improvement switched node 0 back and
  // forth between two components of equal mean forever (72 iterations, then
  // Karp). The reference is now the smallest node of the cycle.
  auto built = build_graph(8, {{5, 0, -4}, {4, 7, 7}, {0, 1, 2}, {1, 2, 3}, {0, 7, 3}, {7, 2, 1},
                               {7, 5, 4}, {3, 7, 1}, {1, 1, 0}, {7, 3, -5}, {2, 0, 5}, {1, 2, 5},
                               {7, 0, -3}, {7, 7, 4}, {0, 4, 2}, {5, 5, 5}, {1, 0, -3}, {7, 2, 7},
                               {4, 4, -2}, {0, 4, 6}, {0, 3, 3}});

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_LT(r.iterations, 20u);
  EXPECT_EQ(r.minimum_mean, karp_minimum_mean_cycle(built.g).minimum_mean);
  EXPECT_EQ(r.minimum_mean, -2.0L);
}


TEST(HowardMinMeanCycleTest, MeanOfAFloatingCycleDoesNotDependOnWhereTheWalkEntersIt)
{
  // The cycle 2 -> 3 -> 4 -> 1 -> 2 has the weights 0, -1e20, 1e20, 3. Added
  // from node 1 their `long double` sum is 0 (the 3 is lost next to 1e20) and
  // added from node 2 it is 3, so while the total was summed from wherever the
  // walk entered the cycle, its mean was 0 or 3/4 from one iteration to the
  // next. Against the cycle 2 -> 3 -> 0 -> 2 (mean 1/3) it kept winning and
  // losing: 70 iterations, then Karp. The total is now summed from the smallest
  // node of the cycle.
  //
  // The weights span more than the 64 bits of a `long double`, so the value is
  // the minimum only up to that rounding (see the file documentation); what is
  // checked is that the iteration converges by itself.
  auto built = build_graph_generic<Float_Graph, double>(
      5, {{2, 3, 0.0}, {3, 4, -1e20}, {3, 0, 1.0}, {0, 2, 0.0}, {1, 2, 3.0}, {4, 1, 1e20}});

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_LT(r.iterations, 5u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(HowardMinMeanCycleTest, IterationLimitFallsBackToKarp)
{
  // A graph whose initial policy (cheapest arc out of each node) is not optimal.
  auto built = build_graph(4, {{0, 1, 1}, {1, 0, 1}, {0, 2, 2}, {2, 3, -9}, {3, 0, 2}, {1, 2, 5}});

  const auto normal = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(normal.has_cycle);
  EXPECT_FALSE(normal.used_karp);
  ASSERT_GT(normal.iterations, 0u);   // the initial policy had to be improved

  // With no iterations allowed Karp answers, and the value is the same.
  const auto forced = howard_detail::minimum_mean_cycle<Graph, Dft_Dist<Graph>, Dft_Show_Arc<Graph>>(
      built.g, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), 0);
  ASSERT_TRUE(forced.has_cycle);
  EXPECT_TRUE(forced.used_karp);
  EXPECT_EQ(forced.minimum_mean, normal.minimum_mean);
  EXPECT_EQ(normal.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_EQ(forced.numeric_quality, Cycle_Numeric_Quality::Rounded);   // Karp compares long doubles

  // A limit as large as the work needed does not fall back.
  const auto enough = howard_detail::minimum_mean_cycle<Graph, Dft_Dist<Graph>, Dft_Show_Arc<Graph>>(
      built.g, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), normal.iterations);
  EXPECT_FALSE(enough.used_karp);
}


TEST(HowardMinMeanCycleTest, ScaledBiasOverflowFallsBackToKarp)
{
  // The cycle 0 -> 1 -> 2 -> 0 costs 2e17 (mean 2e17 / 3), and every partial
  // sum stays within 3.1e18, so Karp copes. Howard scales the bias by the
  // denominator of the mean, 3, and 3 * (-3.1e18) leaves the range of long
  // long: Karp's answer must come out.
  const long long big = 3100000000000000000LL;
  auto built = build_graph(3, {{0, 1, big}, {1, 2, -big}, {2, 0, 200000000000000000LL}});

  const auto howard = howard_minimum_mean_cycle(built.g);
  const auto karp = karp_minimum_mean_cycle(built.g);
  ASSERT_TRUE(howard.has_cycle);
  EXPECT_TRUE(howard.used_karp);
  EXPECT_EQ(howard.minimum_mean, karp.minimum_mean);
  EXPECT_EQ(howard.minimum_mean, 200000000000000000.0L / 3.0L);
}


// Audit 2026-10-02, H5 and stage C1: the fallback runs Karp on the snapshot
// Howard already took, one strongly connected component at a time.

TEST(HowardMinMeanCycleTest, FallbackDoesNotReadTheGraphAgain)
{
  // The graph of IterationLimitFallsBackToKarp; its last arc is hidden.
  auto built = build_graph(4, {{0, 1, 1}, {1, 0, 1}, {0, 2, 2}, {2, 3, -9}, {3, 0, 2}, {1, 2, 5}});

  int dist_calls = 0;
  int filter_calls = 0;
  const auto r = howard_detail::minimum_mean_cycle<Graph, Counting_Dist, Counting_Filter>(
      built.g, Counting_Dist{&dist_calls}, Counting_Filter{&filter_calls, built.arcs[5]}, 0);

  ASSERT_TRUE(r.has_cycle);
  EXPECT_TRUE(r.used_karp);
  EXPECT_EQ(r.minimum_mean, -5.0L / 3.0L);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
  EXPECT_EQ(filter_calls, 6);   // once per arc
  EXPECT_EQ(dist_calls, 5);     // once per accepted arc, fallback included
}


// Audit 2026-10-02, C2: the value-only function builds no witness and its
// fallback keeps no table of predecessors, and still answers the same.

TEST(HowardMinMeanCycleTest, ValueOnlyMatchesTheFullFunctionBitForBit)
{
  using Full_Int = Howard_Mean_Cycle_Result<Graph, long long>;
  const auto same = [](const auto & full, const auto & value) -> ::testing::AssertionResult
  {
    if (full.has_cycle != value.has_cycle)
      return ::testing::AssertionFailure() << "has_cycle";
    if (not full.has_cycle)
      return ::testing::AssertionSuccess();
    if (full.minimum_mean != value.minimum_mean
        or std::signbit(full.minimum_mean) != std::signbit(value.minimum_mean))
      return ::testing::AssertionFailure()
             << "minimum_mean " << full.minimum_mean << " against " << value.minimum_mean;
    if (full.iterations != value.iterations or full.used_karp != value.used_karp
        or full.fallback_reason != value.fallback_reason
        or full.numeric_quality != value.numeric_quality)
      return ::testing::AssertionFailure() << "how the answer was obtained";
    if (full.used_karp != (full.fallback_reason != Howard_Fallback_Reason::None))
      return ::testing::AssertionFailure() << "used_karp disagrees with fallback_reason";
    if (not value.cycle_nodes.is_empty() or not value.cycle_arcs.is_empty()
        or value.witness_node != nullptr)
      return ::testing::AssertionFailure() << "the value-only result built a witness";
    return ::testing::AssertionSuccess();
  };

  std::mt19937_64 rng(0xC2);
  size_t fallbacks = 0;
  size_t compared = 0;
  for (size_t trial = 0; trial < 3000; ++trial)
    {
      const size_t n = 2 + rng() % 8;
      const size_t m = n + rng() % (2 * n);
      std::vector<Edge_Def> iedges;
      std::vector<Edge_Def> bedges;   // weights of about 2^61: Karp's sums need 128 bits
      std::vector<std::tuple<size_t, size_t, double>> fedges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          const long long w = static_cast<long long>(rng() % 41) - 20;
          const long long big = (1LL << 60) + static_cast<long long>(rng() % (1ULL << 61));
          const double unit = std::uniform_real_distribution<double>(-1.0, 1.0)(rng);
          const int exponent = static_cast<int>(rng() % 40) - 20;
          iedges.emplace_back(u, v, w);
          bedges.emplace_back(u, v, rng() % 2 ? big : -big);
          fedges.emplace_back(u, v, std::ldexp(unit, exponent));
        }

      auto ib = build_graph(n, iedges);
      auto bb = build_graph(n, bedges);
      auto fb = build_graph_generic<Float_Graph, double>(n, fedges);
      for (const size_t limit : {howard_detail::default_iterations, size_t{0}})
        {
          using D = Dft_Dist<Graph>;
          using S = Dft_Show_Arc<Graph>;
          for (auto * built : {&ib, &bb})
            {
              Full_Int full;
              try { full = howard_detail::minimum_mean_cycle<Graph, D, S>(built->g, D(), S(), limit); }
              catch (const std::overflow_error &) { continue; }   // only the full one can refuse
              const auto value = howard_detail::minimum_mean_cycle<Graph, D, S, false>(
                  built->g, D(), S(), limit);
              ASSERT_TRUE(same(full, value)) << "long long, trial=" << trial << " limit=" << limit;
              ++compared;
              fallbacks += full.used_karp;
            }

          using FD = Dft_Dist<Float_Graph>;
          using FS = Dft_Show_Arc<Float_Graph>;
          const auto ffull = howard_detail::minimum_mean_cycle<Float_Graph, FD, FS>(fb.g, FD(), FS(), limit);
          const auto fvalue =
            howard_detail::minimum_mean_cycle<Float_Graph, FD, FS, false>(fb.g, FD(), FS(), limit);
          ASSERT_TRUE(same(ffull, fvalue)) << "double, trial=" << trial << " limit=" << limit;
          ++compared;
          fallbacks += ffull.used_karp;
        }

      // The public function, with the default limit.
      const auto full = howard_minimum_mean_cycle(fb.g);
      const auto value = howard_minimum_mean_cycle_value(fb.g);
      ASSERT_EQ(full.has_cycle, value.has_cycle) << "trial=" << trial;
      if (full.has_cycle)
        ASSERT_EQ(full.minimum_mean, value.minimum_mean) << "trial=" << trial;
    }
  EXPECT_GT(compared, 15000u);
  EXPECT_GT(fallbacks, 1000u);   // limit 0 sends most of them to Karp
}


TEST(HowardMinMeanCycleTest, ValueOnlyReadsTheAccessorOncePerArcAlsoAfterTheFallback)
{
  auto built = build_graph(4, {{0, 1, 1}, {1, 0, 1}, {0, 2, 2}, {2, 3, -9}, {3, 0, 2}, {1, 2, 5}});

  for (const size_t limit : {howard_detail::default_iterations, size_t{0}})
    {
      int dist_calls = 0;
      int filter_calls = 0;
      const auto r = howard_detail::minimum_mean_cycle<Graph, Counting_Dist, Counting_Filter, false>(
          built.g, Counting_Dist{&dist_calls}, Counting_Filter{&filter_calls, built.arcs[5]}, limit);
      ASSERT_TRUE(r.has_cycle);
      EXPECT_EQ(r.used_karp, limit == 0);
      EXPECT_EQ(r.minimum_mean, -5.0L / 3.0L);
      EXPECT_EQ(filter_calls, 6) << "limit=" << limit;
      EXPECT_EQ(dist_calls, 5) << "limit=" << limit;
    }
}


TEST(HowardMinMeanCycleTest, ValueOnlyAnswersWhereTheCostOfTheCycleDoesNotFit)
{
  // The only cycle costs LLONG_MAX + 1, which long long cannot report. The
  // function with the witness has to report it and throws; the value-only
  // one needs only the mean, 2^62, and answers, as
  // karp_minimum_mean_cycle_value() does (the 128-bit sums of Karp's table
  // hold it). Without 128-bit integers Karp's table cannot hold it either.
  const long long max = std::numeric_limits<long long>::max();
  auto built = build_graph(2, {{0, 1, max}, {1, 0, 1}});

  EXPECT_THROW((howard_minimum_mean_cycle(built.g)), std::overflow_error);
# if ALEPH_KARP_INT128
  const auto value = howard_minimum_mean_cycle_value(built.g);
  ASSERT_TRUE(value.has_cycle);
  EXPECT_EQ(value.minimum_mean, 0x1p62L);
  EXPECT_EQ(value.minimum_mean, karp_minimum_mean_cycle_value(built.g).minimum_mean);
# else
  EXPECT_THROW((howard_minimum_mean_cycle_value(built.g)), std::overflow_error);
# endif
}


// Audit 2026-10-02, C5: why Karp's algorithm answered.

TEST(HowardMinMeanCycleTest, FallbackReasonSaysWhyKarpAnswered)
{
  using D = Dft_Dist<Graph>;
  using S = Dft_Show_Arc<Graph>;

  // Howard answers: no reason.
  auto limit_graph = build_graph(4, {{0, 1, 1}, {1, 0, 1}, {0, 2, 2}, {2, 3, -9}, {3, 0, 2}, {1, 2, 5}});
  const auto normal = howard_minimum_mean_cycle(limit_graph.g);
  EXPECT_FALSE(normal.used_karp);
  EXPECT_EQ(normal.fallback_reason, Howard_Fallback_Reason::None);

  // The policy needs improvements that the limit does not allow.
  const auto limited = howard_detail::minimum_mean_cycle<Graph, D, S>(limit_graph.g, D(), S(), 0);
  EXPECT_TRUE(limited.used_karp);
  EXPECT_EQ(limited.fallback_reason, Howard_Fallback_Reason::Iteration_Limit);

  // The scaled bias of ScaledBiasOverflowFallsBackToKarp leaves long long.
  const long long big = 3100000000000000000LL;
  auto bias_graph = build_graph(3, {{0, 1, big}, {1, 2, -big}, {2, 0, 200000000000000000LL}});
  const auto overflowed = howard_minimum_mean_cycle(bias_graph.g);
  EXPECT_TRUE(overflowed.used_karp);
  EXPECT_EQ(overflowed.fallback_reason, Howard_Fallback_Reason::Arithmetic_Overflow);

  // The only cycle costs INT_MAX + 1, which int cannot hold although Howard's
  // long long arithmetic computes it. The function with the witness has to
  // report that cost and throws, after Karp finds the same cycle; the
  // value-only one falls back for the same reason and answers.
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using ID = Dft_Dist<Int_Graph>;
  using IS = Dft_Show_Arc<Int_Graph>;
  const int imax = std::numeric_limits<int>::max();
  auto int_graph = build_graph_generic<Int_Graph, int>(2, {{0, 1, imax}, {1, 0, 1}});
  EXPECT_THROW((howard_minimum_mean_cycle(int_graph.g)), std::overflow_error);
  const auto value = howard_detail::minimum_mean_cycle<Int_Graph, ID, IS, false>(
      int_graph.g, ID(), IS(), howard_detail::default_iterations);
  ASSERT_TRUE(value.has_cycle);
  EXPECT_TRUE(value.used_karp);
  EXPECT_EQ(value.fallback_reason, Howard_Fallback_Reason::Cost_Out_Of_Range);
  EXPECT_EQ(value.minimum_mean, 0x1p30L);
  EXPECT_EQ(howard_minimum_mean_cycle_value(int_graph.g).minimum_mean, 0x1p30L);
}


TEST(HowardMinMeanCycleTest, CycleCostIsAddedInTheOrderTheEvaluationProvedSafe)
{
  // Found by a directed search (audit 2026-10-02, C5). The cost of the
  // optimal cycle, -4626910599202282041, fits a long long, but summed from
  // the node where the witness starts a partial sum leaves the type; the
  // function then fell back to Karp, as if the cost did not fit. Summed from
  // the smallest node, as the policy evaluation did without overflowing, it
  // fits all along.
  auto built = build_graph(7, {{6, 1, 5062036987607531343LL}, {4, 3, -3695868834463239656LL},
                               {4, 1, -3661910840776953621LL}, {2, 6, 5139072827235855353LL},
                               {6, 2, -2377556606531334447LL}, {1, 6, -3191812457526854301LL},
                               {6, 5, -2872654956802585972LL}, {0, 4, -4120068231194526993LL},
                               {5, 2, -6893328469635551422LL}, {1, 5, -3920853593594695630LL},
                               {0, 2, -5174462936561685175LL}, {6, 1, 5725667193745753167LL},
                               {5, 6, 4628323756336527655LL}, {3, 5, 5325742771487410443LL}});

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_EQ(r.fallback_reason, Howard_Fallback_Reason::None);
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_EQ(r.cycle_total_cost, -4626910599202282041LL);
  EXPECT_EQ(r.cycle_length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r, false));   // sums beyond long double's mantissa
# if ALEPH_KARP_INT128
  EXPECT_EQ(r.minimum_mean, karp_minimum_mean_cycle(built.g).minimum_mean);
# else
  // Without 128-bit integers Karp's checked long long sums overflow here:
  // the fallback the summation order used to cause would have thrown.
  EXPECT_THROW((karp_minimum_mean_cycle(built.g)), std::overflow_error);
# endif
}


// Audit 2026-10-02, C3: a limit on the memory of the fallback.

namespace
{
  // The family of audit H5: two components of three nodes, the first one
  // with weights whose scaled bias overflows, and `isolated` nodes that lie
  // on no cycle. Howard falls back to Karp (arithmetic overflow).
  Built_Graph_T<Graph> h5_graph(const size_t isolated)
  {
    const long long big = 3100000000000000000LL;
    return build_graph(6 + isolated,
                       {{0, 1, big}, {1, 2, -big}, {2, 0, 1}, {3, 4, -1}, {4, 5, -1}, {5, 3, -2}});
  }

  // Bytes of Karp's tables for that family: for each component of 3 nodes,
  // (3 + 1) * 3 = 12 entries, of which Array reserves 16. The sums of the
  // first one need 128 bits (3 * 3.1e18 > LLONG_MAX), 8 bytes without them;
  // the predecessor positions take 8 more.
# if ALEPH_KARP_INT128
  constexpr size_t h5_witness_bytes = 16 * (16 + 8);
  constexpr size_t h5_value_bytes = 16 * 16;
# else
  constexpr size_t h5_witness_bytes = 16 * (8 + 8);
  constexpr size_t h5_value_bytes = 16 * 8;
# endif
} // namespace


TEST(HowardMinMeanCycleTest, FallbackWithinTheMemoryLimitAnswersAsWithout)
{
  using D = Dft_Dist<Graph>;
  using S = Dft_Show_Arc<Graph>;

  // With 3000 isolated nodes a table over the whole graph would take some
  // 3003^2 entries, hundreds of MiB; the fallback needs the tables of two
  // components of three nodes, a few hundred bytes.
  auto built = h5_graph(3000);
  const auto unlimited = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(unlimited.used_karp);
  ASSERT_EQ(unlimited.fallback_reason, Howard_Fallback_Reason::Arithmetic_Overflow);

  for (const size_t limit : {h5_witness_bytes, size_t{1024}})
    {
      const auto limited = howard_minimum_mean_cycle(built.g, D(), S(), limit);
      ASSERT_TRUE(limited.has_cycle) << "limit=" << limit;
      EXPECT_EQ(limited.fallback_reason, Howard_Fallback_Reason::Arithmetic_Overflow);
      EXPECT_EQ(limited.minimum_mean, unlimited.minimum_mean);
      EXPECT_EQ(limited.cycle_total_cost, unlimited.cycle_total_cost);
      EXPECT_EQ(limited.cycle_length, unlimited.cycle_length);
      EXPECT_TRUE(witness_is_simple_cycle(built.g, limited));
    }

  // The value-only function needs no predecessors: a smaller table.
  for (const size_t limit : {h5_value_bytes, size_t{1024}})
    EXPECT_EQ(howard_minimum_mean_cycle_value(built.g, D(), S(), limit).minimum_mean,
              unlimited.minimum_mean) << "limit=" << limit;
}


TEST(HowardMinMeanCycleTest, FallbackBeyondTheMemoryLimitThrowsLengthError)
{
  using D = Dft_Dist<Graph>;
  using S = Dft_Show_Arc<Graph>;
  auto built = h5_graph(10);

  // One byte below what the tables need.
  EXPECT_THROW((howard_minimum_mean_cycle(built.g, D(), S(), h5_witness_bytes - 1)),
               std::length_error);
  EXPECT_THROW((howard_minimum_mean_cycle_value(built.g, D(), S(), h5_value_bytes - 1)),
               std::length_error);

  // The message gives the bytes, the component and the reason.
  try
    {
      (void) howard_minimum_mean_cycle(built.g, D(), S(), 100);
      FAIL() << "the limit was ignored";
    }
  catch (const std::length_error & e)
    {
      const std::string what = e.what();
      EXPECT_NE(what.find("arithmetic overflow"), std::string::npos) << what;
      EXPECT_NE(what.find(std::to_string(h5_witness_bytes) + " bytes"), std::string::npos) << what;
      EXPECT_NE(what.find("component of 3 nodes"), std::string::npos) << what;
      EXPECT_NE(what.find("limit of 100 bytes"), std::string::npos) << what;
    }

  // A limit of 0 refuses any fallback, and only a fallback: where Howard
  // answers by itself it does not matter.
  auto plain = build_graph(4, {{0, 1, 1}, {1, 0, 1}, {0, 2, 2}, {2, 3, -9}, {3, 0, 2}, {1, 2, 5}});
  const auto r = howard_minimum_mean_cycle(plain.g, D(), S(), 0);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_EQ(r.minimum_mean, -5.0L / 3.0L);
  try
    {
      (void) howard_detail::minimum_mean_cycle<Graph, D, S>(plain.g, D(), S(), 0, 0);
      FAIL() << "a limit of 0 let the fallback run";
    }
  catch (const std::length_error & e)
    {
      EXPECT_NE(std::string(e.what()).find("iteration limit"), std::string::npos) << e.what();
    }
}


TEST(HowardMinMeanCycleTest, FallbackRunsKarpOnEachComponent)
{
  // Two copies of the graph above (nodes 0-3 and 4-7, same mean -5/3), a
  // component of mean -1 (nodes 8-9), arcs between components, which lie on
  // no cycle, and many isolated nodes. Only the cyclic core reaches Karp.
  const size_t isolated = 2000;
  std::vector<Edge_Def> edges;
  for (size_t base : {0u, 4u})
    for (const auto &[u, v, w] : std::vector<Edge_Def>{{0, 1, 1}, {1, 0, 1}, {0, 2, 2},
                                                         {2, 3, -9}, {3, 0, 2}, {1, 2, 5}})
      edges.emplace_back(base + u, base + v, w);
  edges.emplace_back(8, 9, -1);
  edges.emplace_back(9, 8, -1);
  edges.emplace_back(9, 0, -100);   // between components
  edges.emplace_back(3, 4, -100);
  edges.emplace_back(10, 8, -100);  // from an isolated node
  auto built = build_graph(10 + isolated, edges);

  const auto r = howard_detail::minimum_mean_cycle<Graph, Dft_Dist<Graph>, Dft_Show_Arc<Graph>>(
      built.g, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), 0);
  ASSERT_TRUE(r.has_cycle);
  ASSERT_TRUE(r.used_karp);
  EXPECT_EQ(r.minimum_mean, -5.0L / 3.0L);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));

  // The tie between the two copies goes to the one that comes first, as in
  // a single table over the whole graph.
  for (auto it = r.cycle_nodes.get_it(); it.has_curr(); it.next_ne())
    EXPECT_LT(it.get_curr()->get_info(), 4);
  EXPECT_LT(r.witness_node->get_info(), 4);

  // The same value as Karp on the whole graph, which agrees exactly here:
  // integral costs make both round the exact minimum mean once.
  EXPECT_EQ(r.minimum_mean, karp_minimum_mean_cycle(built.g).minimum_mean);
}


TEST(HowardMinMeanCycleTest, EveryFallbackPassesTheChecksOfHowardsOwnAnswer)
{
  // Audit 2026-10-02, C4: after the fallback the witness is a simple cycle,
  // `witness_node` is its first node and its cost is that of its arcs, as in
  // Howard's own answer. Before, it was Karp's closed walk, not simple in a
  // third of the floating-point cases.
  std::mt19937_64 rng(0xC4);
  size_t fallbacks = 0;
  for (size_t trial = 0; trial < 3000; ++trial)
    {
      const size_t n = 2 + rng() % 8;
      const size_t m = n + rng() % (2 * n);
      std::vector<Edge_Def> iedges;
      std::vector<std::tuple<size_t, size_t, double>> fedges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          const long long w = static_cast<long long>(rng() % 41) - 20;
          const double unit = std::uniform_real_distribution<double>(-1.0, 1.0)(rng);
          const int exponent = static_cast<int>(rng() % 40) - 20;
          iedges.emplace_back(u, v, w);
          fedges.emplace_back(u, v, std::ldexp(unit, exponent));
        }

      auto ib = build_graph(n, iedges);
      const auto ir = howard_detail::minimum_mean_cycle<Graph, Dft_Dist<Graph>, Dft_Show_Arc<Graph>>(
          ib.g, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), 0);
      ASSERT_EQ(ir.used_karp, ir.fallback_reason != Howard_Fallback_Reason::None) << "trial=" << trial;
      if (ir.used_karp)
        {
          ++fallbacks;
          ASSERT_EQ(ir.fallback_reason, Howard_Fallback_Reason::Iteration_Limit) << "trial=" << trial;
          ASSERT_TRUE(witness_is_simple_cycle(ib.g, ir)) << "long long, trial=" << trial;
          ASSERT_EQ(ir.minimum_mean, howard_minimum_mean_cycle(ib.g).minimum_mean) << "trial=" << trial;
        }

      auto fb = build_graph_generic<Float_Graph, double>(n, fedges);
      const auto fr =
        howard_detail::minimum_mean_cycle<Float_Graph, Dft_Dist<Float_Graph>, Dft_Show_Arc<Float_Graph>>(
          fb.g, Dft_Dist<Float_Graph>(), Dft_Show_Arc<Float_Graph>(), 0);
      ASSERT_EQ(fr.used_karp, fr.fallback_reason != Howard_Fallback_Reason::None) << "trial=" << trial;
      if (fr.used_karp)
        {
          ++fallbacks;
          ASSERT_EQ(fr.fallback_reason, Howard_Fallback_Reason::Iteration_Limit) << "trial=" << trial;
          ASSERT_TRUE(witness_is_simple_cycle(fb.g, fr)) << "double, trial=" << trial;
          ASSERT_EQ(fr.numeric_quality, Cycle_Numeric_Quality::Rounded);
        }
    }
  EXPECT_GT(fallbacks, 1000u);   // the iteration limit 0 forces most of them
}


TEST(HowardMinMeanCycleTest, FallbackSumsInsideAComponentDoNotOverflow)
{
  // Found by a directed search (audit 2026-10-02, C1 and C6). Inside the
  // component {1, ..., 5} every walk is costly and the long long sums of a
  // per-component table overflow; on the whole graph the arc 0 -> 1, from
  // outside, kept them low. The per-component fallback then threw where the
  // old whole-graph one answered. 128-bit sums remove that.
  auto built = build_graph(6, {{3, 4, 2412223573619837724LL}, {2, 3, 2422047310724953863LL},
                               {1, 2, 3179646298365416052LL}, {0, 1, -3127445719569793886LL},
                               {1, 2, 3008898773151771847LL}, {4, 4, 3004633816298768677LL},
                               {0, 5, 2726043725430506698LL}, {3, 5, 1938808504231304943LL},
                               {4, 1, 3287975327571625454LL}, {1, 2, 2527433209753059253LL},
                               {1, 1, 1921533484238488548LL}, {5, 3, 1325352911222347034LL},
                               {2, 3, 2519116059040766616LL}, {2, 2, 1472205099507841752LL},
                               {2, 2, 3345130543762169200LL}, {1, 4, 1779708353840524248LL},
                               {1, 5, 3214578466714837685LL}});

  const auto exact = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(exact.has_cycle);
  ASSERT_FALSE(exact.used_karp);
# if ALEPH_KARP_INT128
  const auto forced = howard_detail::minimum_mean_cycle<Graph, Dft_Dist<Graph>, Dft_Show_Arc<Graph>>(
      built.g, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), 0);
  ASSERT_TRUE(forced.has_cycle);
  ASSERT_TRUE(forced.used_karp);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, forced, false));   // sums beyond long double's mantissa
  EXPECT_EQ(forced.minimum_mean, exact.minimum_mean);
  EXPECT_EQ(forced.minimum_mean, karp_minimum_mean_cycle(built.g).minimum_mean);
# endif
}


// ---------------------------------------------------------------------------
// Audit 2026-10-02, H2-H4, fixed in its stage B. Every weight below is a
// binary fraction represented exactly, and the exact answer follows from
// identities between powers of two, not from another algorithm. The policy
// is found with long double arithmetic and then re-solved exactly with the
// weights scaled to 64- or 128-bit integers when they fit; when they do not,
// the choice among the cycles of the policy is still made with exact sums.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, CancellationDoesNotChooseAWorseCycle)
{
  // Ring 0 -> 1 -> 2 -> 3 -> 0 with weights 2^e, 1, -2^e, -2: exact mean -1/4.
  // Self-loop of node 4: -0.375, the exact optimum. For e = 100 and e = 200
  // the plain sum of the policy evaluation loses the 1 and the ring looks like
  // mean -1/2; it used to be returned. With e = 200 the weights span 204 bits,
  // too many for the exact verification: the policy stays the rounded one, and
  // only the exact choice among its cycles picks the loop.
  for (const int e : {50, 100, 200})
    {
      const double big = std::ldexp(1.0, e);
      auto built = build_graph_generic<Float_Graph, double>(
          5, {{0, 1, big}, {1, 2, 1.0}, {2, 3, -big}, {3, 0, -2.0}, {4, 4, -0.375}});

      const auto r = howard_minimum_mean_cycle(built.g);
      ASSERT_TRUE(r.has_cycle) << "e=" << e;
      EXPECT_FALSE(r.used_karp) << "e=" << e;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "e=" << e;
      EXPECT_EQ(r.minimum_mean, -0.375L) << "e=" << e;
      EXPECT_EQ(r.cycle_total_cost, -0.375) << "e=" << e;
      EXPECT_EQ(r.cycle_length, 1u) << "e=" << e;

      // The scaled weights need 54 bits for e = 50, 104 for e = 100 and 204
      // for e = 200.
# if ALEPH_HOWARD_INT128
      const bool verified = e <= 100;
# else
      const bool verified = e == 50;
# endif
      EXPECT_EQ(r.numeric_quality, verified ? Cycle_Numeric_Quality::Exact
                                            : Cycle_Numeric_Quality::Rounded) << "e=" << e;
    }
}


TEST(HowardMinMeanCycleTest, OverflowOfTheExactVerificationFallsBackToTheExactChoice)
{
  // Ring 0 -> 1 -> 2 -> 3 -> 0 with weights 2^122, 1, -2^122, -1.875 (exact mean
  // -0.21875) and a self-loop of -0.375, the optimum. The rounded iteration
  // loses the 1 and prefers the ring. Scaled to integers the weights need 126
  // bits, so the 128-bit verification is tried, but the ring's mean has
  // denominator 4 and 4 * 2^125 overflows it: the verification must notice and
  // give up, leaving the exact choice among the cycles to pick the loop.
  const double big = std::ldexp(1.0, 122);
  auto built = build_graph_generic<Float_Graph, double>(
      5, {{0, 1, big}, {1, 2, 1.0}, {2, 3, -big}, {3, 0, -1.875}, {4, 4, -0.375}});

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.used_karp);
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded);
  EXPECT_EQ(r.minimum_mean, -0.375L);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r, false));
}


TEST(HowardMinMeanCycleTest, ImprovementsBelowTheToleranceAreFoundExactly)
{
  // Loops of weight 0 on nodes 0 and 1, and the cycle 0 -> 1 -> 0 with weights
  // d and -2d: mean -d/2, the optimum. With d = 2^-60 the improvement is below
  // the absolute floor of the bias tolerance (64 epsilon of a long double), so
  // the long double iteration stops at a loop of mean 0; the exact one, which
  // starts from that policy, finds the cycle.
  for (const int exponent : {-60, 0})
    {
      const double d = std::ldexp(1.0, exponent);
      auto built = build_graph_generic<Float_Graph, double>(
          2, {{0, 0, 0.0}, {1, 1, 0.0}, {0, 1, d}, {1, 0, -2 * d}});

      const auto r = howard_minimum_mean_cycle(built.g);
      ASSERT_TRUE(r.has_cycle) << "exponent=" << exponent;
      EXPECT_FALSE(r.used_karp) << "exponent=" << exponent;
      EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact) << "exponent=" << exponent;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "exponent=" << exponent;
      EXPECT_EQ(r.minimum_mean, -static_cast<long double>(d) / 2) << "exponent=" << exponent;
      EXPECT_EQ(r.cycle_total_cost, -d) << "exponent=" << exponent;
      EXPECT_EQ(r.cycle_length, 2u) << "exponent=" << exponent;
    }
}


TEST(HowardMinMeanCycleTest, ACycleOfExactTotalZeroIsNeverNegative)
{
  // Six-arc ring whose weights 2^100, 1, 2^-100, -1, -2^100, -2^-100 (in some
  // rotation) sum exactly to zero. Its published cost used to be a
  // compensated sum, -2^-100 for some rotations; it is now the exact sum. The
  // weights span 201 binary orders of magnitude, too many for the exact
  // verification, so the result stays Rounded.
  const double big = std::ldexp(1.0, 100);
  const double tiny = std::ldexp(1.0, -100);
  const std::vector<double> ring = {big, 1.0, tiny, -1.0, -big, -tiny};

  for (size_t rotation = 0; rotation < ring.size(); ++rotation)
    {
      std::vector<std::tuple<size_t, size_t, double>> edges;
      for (size_t i = 0; i < ring.size(); ++i)
        edges.emplace_back(i, (i + 1) % ring.size(), ring[(i + rotation) % ring.size()]);
      auto built = build_graph_generic<Float_Graph, double>(ring.size(), edges);

      const auto r = howard_minimum_mean_cycle(built.g);
      ASSERT_TRUE(r.has_cycle) << "rotation=" << rotation;
      EXPECT_FALSE(r.used_karp) << "rotation=" << rotation;
      EXPECT_EQ(r.cycle_length, ring.size()) << "rotation=" << rotation;
      EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded) << "rotation=" << rotation;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r, false)) << "rotation=" << rotation;
      EXPECT_EQ(r.cycle_total_cost, 0.0) << "rotation=" << rotation;
      EXPECT_EQ(r.minimum_mean, 0.0L) << "rotation=" << rotation;
    }
}


TEST(HowardMinMeanCycleTest, CostsOfAboutTenToTheEighteenAgreeWithKarp)
{
  // Whenever Karp can answer, Howard (directly or through the fallback) gives
  // the same value; Howard may also answer where Karp's sums overflow.
  //
  // The mean is a `long double`. With a 64-bit mantissa (x86) converting a sum
  // of up to ~1e19 is exact and both algorithms round the same quotient once,
  // so they agree bit for bit. With a 53-bit one (`long double` is a `double`
  // on Apple Silicon and MSVC) every conversion of such a sum rounds, and
  // Karp's subtract-then-divide drifts from the exact mean by a few rounding
  // errors *of the sums*, which are large next to a small mean. So Howard's
  // cycle is checked exactly against a 128-bit exhaustive oracle on every
  // platform, and Karp's value with a tolerance that is zero where
  // `long double` is wide enough to make the two bit-identical.
  using Mean_Limits = std::numeric_limits<long double>;
  std::mt19937_64 rng(0x0F3EF10ULL);
  for (size_t trial = 0; trial < 1500; ++trial)
    {
      const size_t n = 2 + rng() % 5;
      const long long big = (trial % 2 == 0) ? 1000000000000000000LL : 2000000000000000000LL;
      std::vector<Edge_Def> edges;
      const size_t m = n + rng() % (2 * n);
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          const long long w = static_cast<long long>(rng() % static_cast<unsigned long long>(2 * big)) - big;
          edges.emplace_back(u, v, w);
        }

      auto built = build_graph(n, edges);
      Howard_Mean_Cycle_Result<Graph, long long> howard;
      Min_Mean_Cycle_Result<Graph, long long> karp;
      bool karp_threw = false;
      try { karp = karp_minimum_mean_cycle(built.g); } catch (const std::overflow_error &) { karp_threw = true; }
      try { howard = howard_minimum_mean_cycle(built.g); }
      catch (const std::overflow_error &)
        {
          // Only an answer that Karp could not give either may be refused.
          ASSERT_TRUE(karp_threw) << "trial=" << trial;
          continue;
        }

# if ALEPH_HOWARD_INT128
      // Howard's own answer, also where Karp threw: exactly the oracle's
      // rational mean, `total / length` cross-multiplied in 128 bits. (After
      // the fallback the witness is Karp's closed walk: nothing to compare.)
      // Without a 128-bit integer (MSVC) only the comparison with Karp below
      // remains.
      const auto oracle = exact_minimum_mean<__int128>(built);
      ASSERT_EQ(howard.has_cycle, oracle.exists) << "trial=" << trial;
      if (howard.has_cycle and not howard.used_karp)
        ASSERT_TRUE(static_cast<__int128>(howard.cycle_total_cost) * oracle.length
                    == oracle.total * static_cast<__int128>(howard.cycle_length))
          << "trial=" << trial;
# endif

      if (karp_threw)
        continue;
      ASSERT_EQ(howard.has_cycle, karp.has_cycle) << "trial=" << trial;
      if (not howard.has_cycle)
        continue;

      // Karp rounds the conversions of its sums (each below n * big in
      // absolute value) and their difference: a few of their rounding errors,
      // `epsilon * n * big`, bound the drift. That holds after the fallback
      // too, which runs Karp's table on each component, not on the whole
      // graph: other sums, the same exact mean.
      const long double tolerance =
        Mean_Limits::digits >= 64
          ? 0.0L
          : 8.0L * Mean_Limits::epsilon() * static_cast<long double>(n)
              * static_cast<long double>(big);
      ASSERT_LE(std::fabs(howard.minimum_mean - karp.minimum_mean), tolerance)
        << "trial=" << trial;
    }
}


// ---------------------------------------------------------------------------
// The exact arithmetic.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, FractionsAreComparedWithoutOverflow)
{
  const long long hi = std::numeric_limits<long long>::max();
  const long long lo = std::numeric_limits<long long>::min();

  EXPECT_EQ(Arith::compare_fractions(1, 3, 2, 6), 0);
  EXPECT_EQ(Arith::compare_fractions(-1, 3, 1, 3), -1);
  EXPECT_EQ(Arith::compare_fractions(7, 2, 10, 3), 1);       // 3.5 > 3.33..
  EXPECT_EQ(Arith::compare_fractions(hi, 1, hi - 1, 1), 1);
  EXPECT_EQ(Arith::compare_fractions(lo, 1, lo + 1, 1), -1);
  EXPECT_EQ(Arith::compare_fractions(lo, 3, lo, 2), 1);      // -x/3 > -x/2
  EXPECT_EQ(Arith::compare_fractions(hi, hi, 1, 1), 0);
  EXPECT_EQ(Arith::compare_fractions(hi - 1, hi, hi - 2, hi - 1), 1);   // 1 - 1/hi vs 1 - 1/(hi-1)

# if ALEPH_HOWARD_INT128
  std::mt19937_64 rng(0xC0FFEE);
  for (size_t i = 0; i < 200000; ++i)
    {
      auto draw = [&]
      {
        const unsigned long long r = rng();
        switch (rng() % 4)
          {
          case 0: return static_cast<long long>(r);
          case 1: return static_cast<long long>(r % 2000) - 1000;
          case 2: return static_cast<long long>(r >> (rng() % 63)) * (rng() % 2 ? 1 : -1);
          default: return (rng() % 2) ? hi - static_cast<long long>(r % 5) : lo + static_cast<long long>(r % 5);
          }
      };
      auto positive = [&]
      {
        long long b = draw();
        if (b == lo)
          b = hi;
        return b < 0 ? -b : (b == 0 ? 1 : b);
      };
      const long long a1 = draw(), a2 = draw(), b1 = positive(), b2 = positive();
      const __int128 lhs = static_cast<__int128>(a1) * b2;
      const __int128 rhs = static_cast<__int128>(a2) * b1;
      const int expected = lhs < rhs ? -1 : (lhs > rhs ? 1 : 0);
      ASSERT_EQ(Arith::compare_fractions(a1, b1, a2, b2), expected)
          << a1 << "/" << b1 << " vs " << a2 << "/" << b2;
    }
# endif
}


TEST(HowardMinMeanCycleTest, MeansAreReducedAndStepsReportOverflow)
{
  const auto e = Arith::make_eta(-6, 4);
  EXPECT_EQ(e.num, -3);
  EXPECT_EQ(e.den, 2);
  EXPECT_TRUE(Arith::equal(Arith::make_eta(2, 4), Arith::make_eta(1, 2)));
  EXPECT_FALSE(Arith::equal(Arith::make_eta(1, 3), Arith::make_eta(1, 2)));
  EXPECT_TRUE(Arith::less(Arith::make_eta(1, 3), Arith::make_eta(1, 2)));
  EXPECT_FALSE(Arith::less(Arith::make_eta(1, 2), Arith::make_eta(1, 2)));
  const auto zero = Arith::make_eta(0, 7);
  EXPECT_EQ(zero.num, 0);
  EXPECT_EQ(zero.den, 1);
  const auto extreme = Arith::make_eta(std::numeric_limits<long long>::min(), 2);
  EXPECT_EQ(extreme.den, 1);
  EXPECT_EQ(extreme.num, std::numeric_limits<long long>::min() / 2);

  long long out = 0;
  EXPECT_TRUE(Arith::step(5, Arith::Eta{3, 2}, 10, out));    // 2 * 5 - 3 + 10
  EXPECT_EQ(out, 17);
  const long long hi = std::numeric_limits<long long>::max();
  EXPECT_FALSE(Arith::step(hi, Arith::Eta{0, 2}, 0, out));   // 2 * max overflows
  EXPECT_FALSE(Arith::step(0, Arith::Eta{std::numeric_limits<long long>::min(), 1}, 0, out));   // 0 - min
  EXPECT_FALSE(Arith::step(0, Arith::Eta{0, 1}, hi, out) and Arith::step(1, Arith::Eta{0, 1}, hi, out));
}


# if ALEPH_HOWARD_INT128
TEST(HowardMinMeanCycleTest, ExactArithmeticOn128BitsReportsOverflow)
{
  // The 128-bit arithmetic of the exact verification: products whose factors
  // fit 64 bits take a fast path, the others the portable test.
  using Wide = howard_detail::int128_t;
  using Ar = howard_detail::Arithmetic<Wide, true>;
  const Wide two_125 = Wide(1) << 125;
  const Wide max = howard_detail::Exact_Integer<Wide>::max;

  Wide out = 0;
  EXPECT_TRUE(Ar::step(Wide(5), Ar::Eta{3, 2}, Wide(10), out));        // 2 * 5 - 3 + 10, fast path
  EXPECT_EQ(out, Wide(17));
  EXPECT_TRUE(Ar::step(two_125, Ar::Eta{0, 3}, Wide(0), out));         // 3 * 2^125 < 2^127
  EXPECT_EQ(out, 3 * two_125);
  EXPECT_FALSE(Ar::step(two_125, Ar::Eta{0, 4}, Wide(0), out));        // 4 * 2^125 = 2^127 overflows
  EXPECT_FALSE(Ar::step(-two_125, Ar::Eta{0, 5}, Wide(0), out));       // -5 * 2^125 overflows
  EXPECT_FALSE(Ar::step(max, Ar::Eta{0, 2}, Wide(0), out));
  EXPECT_FALSE(Ar::step(Wide(0), Ar::Eta{0, 1}, max, out) and Ar::step(Wide(1), Ar::Eta{0, 1}, max, out));

  const auto e = Ar::make_eta(-(Wide(3) << 100), 6);   // -3 * 2^100 / 6 = -2^99 / 1
  EXPECT_EQ(e.den, Wide(1));
  EXPECT_EQ(e.num, -(Wide(1) << 99));
}
# endif


// ---------------------------------------------------------------------------
// The file documentation promises that several threads may search one graph.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, ConcurrentSearchesOnTheSameGraphAgree)
{
  std::vector<Edge_Def> edges;
  for (size_t u = 0; u < 40; ++u)
    {
      edges.emplace_back(u, (u + 1) % 40, 1 + static_cast<long long>(u % 5));
      edges.emplace_back(u, (u * 7 + 3) % 40, 2 - static_cast<long long>(u % 3));
    }
  auto built = build_graph(40, edges);
  const auto reference = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(reference.has_cycle);

  constexpr size_t threads = 8;
  std::vector<int> failures(threads, 0);
  std::vector<std::thread> pool;
  for (size_t t = 0; t < threads; ++t)
    pool.emplace_back([&, t]
    {
      for (size_t i = 0; i < 40; ++i)
        {
          const auto r = howard_minimum_mean_cycle(built.g);
          if (r.minimum_mean != reference.minimum_mean or r.cycle_arcs != reference.cycle_arcs
              or r.iterations != reference.iterations)
            ++failures[t];
        }
    });
  for (auto & th : pool)
    th.join();
  for (size_t t = 0; t < threads; ++t)
    EXPECT_EQ(failures[t], 0) << "thread " << t;
}
