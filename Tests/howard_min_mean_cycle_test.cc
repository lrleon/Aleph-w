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
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded);
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
  // table and overflows even where the cycle is a loop; Howard needs only the
  // cycle it reports.
  using IntGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  auto built = build_graph_generic<IntGraph, int>(
      4, {{2, 2, 653666155}, {2, 2, 585777688}, {1, 3, 641209630}, {3, 0, 491799673},
          {3, 3, -1900485882}});

  EXPECT_THROW((karp_minimum_mean_cycle(built.g)), std::overflow_error);

  const auto r = howard_minimum_mean_cycle(built.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.cycle_total_cost, -1900485882);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_FALSE(r.used_karp);

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


// ---------------------------------------------------------------------------
// Audit 2026-10-02, H2-H4: what floating-point costs cannot promise. Every
// weight below is a binary fraction represented exactly, and the exact answer
// follows from identities between powers of two, not from another algorithm.
// The KNOWN LIMITATION assertions pin the current behaviour; when policies are
// compared with exact or verified sums they must be inverted.
// ---------------------------------------------------------------------------

TEST(HowardMinMeanCycleTest, KnownNumericLimitationCancellationCanChooseAWorseCycle)
{
  // Ring 0 -> 1 -> 2 -> 3 -> 0 with weights 2^e, 1, -2^e, -2: exact mean -1/4.
  // Self-loop of node 4: -0.375, the exact optimum. For e = 50 every partial
  // sum is exact even in a 53-bit `long double` and Howard is right; for
  // e = 100 the plain sum of the policy evaluation loses the 1, the ring looks
  // like mean -1/2 and wins.
  for (const int e : {50, 100})
    {
      const double big = std::ldexp(1.0, e);
      auto built = build_graph_generic<Float_Graph, double>(
          5, {{0, 1, big}, {1, 2, 1.0}, {2, 3, -big}, {3, 0, -2.0}, {4, 4, -0.375}});

      const auto r = howard_minimum_mean_cycle(built.g);
      ASSERT_TRUE(r.has_cycle) << "e=" << e;
      EXPECT_FALSE(r.used_karp) << "e=" << e;
      EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded) << "e=" << e;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r, false)) << "e=" << e;
      if (e == 50)
        {
          EXPECT_EQ(r.minimum_mean, -0.375L);
          EXPECT_EQ(r.cycle_length, 1u);
        }
      else
        {
          // KNOWN LIMITATION: the ring, published with its recomputed mean.
          EXPECT_EQ(r.minimum_mean, -0.25L);
          EXPECT_EQ(r.cycle_total_cost, -1.0);
          EXPECT_EQ(r.cycle_length, 4u);
        }
    }
}


TEST(HowardMinMeanCycleTest, KnownNumericLimitationImprovementsBelowTheToleranceFloorAreIgnored)
{
  // Loops of weight 0 on nodes 0 and 1, and the cycle 0 -> 1 -> 0 with weights
  // d and -2d: mean -d/2, the optimum. With d = 2^-60 the improvement is below
  // the absolute floor of the bias tolerance (64 epsilon of a long double) and
  // Howard stops at a loop of mean 0; the same graph scaled by 2^60 is solved.
  for (const int exponent : {-60, 0})
    {
      const double d = std::ldexp(1.0, exponent);
      auto built = build_graph_generic<Float_Graph, double>(
          2, {{0, 0, 0.0}, {1, 1, 0.0}, {0, 1, d}, {1, 0, -2 * d}});

      const auto r = howard_minimum_mean_cycle(built.g);
      ASSERT_TRUE(r.has_cycle) << "exponent=" << exponent;
      EXPECT_FALSE(r.used_karp) << "exponent=" << exponent;
      EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded) << "exponent=" << exponent;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "exponent=" << exponent;
      if (exponent == 0)
        EXPECT_EQ(r.minimum_mean, -0.5L);
      else
        {
          // KNOWN LIMITATION: mean 0 instead of -2^-61. (The floor depends on the
          // width of long double; with 53 bits it is larger, so this holds too.)
          EXPECT_EQ(r.minimum_mean, 0.0L);
          EXPECT_EQ(r.cycle_length, 1u);
        }
    }
}


TEST(HowardMinMeanCycleTest, KnownNumericLimitationCompensatedSumCanMakeAZeroCycleNegative)
{
  // Six-arc ring whose weights 2^100, 1, 2^-100, -1, -2^100, -2^-100 (in some
  // rotation) sum exactly to zero. The published cost is a compensated sum,
  // which is not exact: for some rotations it comes out as -2^-100. The cycle
  // itself is right (it is the only one); its sign is not.
  const double big = std::ldexp(1.0, 100);
  const double tiny = std::ldexp(1.0, -100);
  const std::vector<double> ring = {big, 1.0, tiny, -1.0, -big, -tiny};

  size_t negative = 0;
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
      // Either the exact 0 or the rounding residue -2^-100, nothing else.
      EXPECT_TRUE(r.cycle_total_cost == 0.0 or r.cycle_total_cost == -tiny)
          << "rotation=" << rotation << " cost=" << r.cycle_total_cost;
      negative += r.cycle_total_cost < 0.0;
    }
  // KNOWN LIMITATION: at least one rotation presents the zero cycle as negative.
  EXPECT_GE(negative, 1u);
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

# ifdef __SIZEOF_INT128__
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

      if (howard.used_karp)   // the same function ran: identical on every platform
        ASSERT_EQ(howard.minimum_mean, karp.minimum_mean) << "trial=" << trial;
      else
        {
          // Karp rounds the conversions of its sums (each below n * big in
          // absolute value) and their difference: a few of their rounding
          // errors, `epsilon * n * big`, bound the drift.
          const long double tolerance =
            Mean_Limits::digits >= 64
              ? 0.0L
              : 8.0L * Mean_Limits::epsilon() * static_cast<long double>(n)
                  * static_cast<long double>(big);
          ASSERT_LE(std::fabs(howard.minimum_mean - karp.minimum_mean), tolerance)
            << "trial=" << trial;
        }
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

# ifdef __SIZEOF_INT128__
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
