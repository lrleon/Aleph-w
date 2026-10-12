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
 * @file min_mean_cycle_test.cc
 * @brief Tests for Karp minimum mean cycle implementation.
 */

# include <gtest/gtest.h>

# include <cmath>
# include <functional>
# include <limits>
# include <random>
# include <tuple>
# include <vector>

# include <Min_Mean_Cycle.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;
  using Float_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
  using Array_Graph = Array_Digraph<Graph_Anode<int>, Graph_Aarc<long long>>;
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  using Edge_Def = std::tuple<size_t, size_t, long long>;
  using Float_Edge_Def = std::tuple<size_t, size_t, double>;
  using Result = Min_Mean_Cycle_Result<Graph, long long>;

  template <class GT>
  struct Built_Graph_T
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    std::vector<typename GT::Arc *> arcs;
  };

  using Built_Graph = Built_Graph_T<Graph>;
  using Built_Float_Graph = Built_Graph_T<Float_Graph>;
  using Built_Array_Graph = Built_Graph_T<Array_Graph>;

  template <class GT, typename Weight_Type>
  Built_Graph_T<GT>
  build_graph_generic(const size_t n, const std::vector<std::tuple<size_t, size_t, Weight_Type>> & edges)
  {
    Built_Graph_T<GT> built;
    built.nodes.reserve(n);
    built.arcs.reserve(edges.size());

    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : edges)
      if (u < n and v < n)
        built.arcs.push_back(built.g.insert_arc(built.nodes[u], built.nodes[v], w));

    return built;
  }

  Built_Graph build_graph(const size_t n, const std::vector<Edge_Def> & edges)
  {
    return build_graph_generic<Graph, long long>(n, edges);
  }

  Built_Float_Graph build_float_graph(const size_t n, const std::vector<Float_Edge_Def> & edges)
  {
    return build_graph_generic<Float_Graph, double>(n, edges);
  }

  Built_Array_Graph build_array_graph(const size_t n, const std::vector<Edge_Def> & edges)
  {
    return build_graph_generic<Array_Graph, long long>(n, edges);
  }


  struct Exact_Oracle_Result
  {
    bool has_cycle = false;
    long double minimum_mean = std::numeric_limits<long double>::infinity();
  };


  Exact_Oracle_Result exact_minimum_mean_cycle(const Built_Graph & built)
  {
    const size_t n = built.nodes.size();

    Exact_Oracle_Result oracle;
    std::vector<bool> visited(n, false);

    std::function<void(size_t, size_t, long long, size_t)> dfs =
      [&](const size_t start,
          const size_t u,
          const long long cost,
          const size_t len)
    {
      for (Node_Arc_Iterator<Graph> it(built.nodes[u]); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_current_arc_ne();
          const size_t v = static_cast<size_t>(it.get_tgt_node()->get_info());
          const long long w = arc->get_info();

          if (v == start)
            {
              const size_t cyc_len = len + 1;
              const long double mean = static_cast<long double>(cost + w)
                                       / static_cast<long double>(cyc_len);
              oracle.has_cycle = true;
              if (mean < oracle.minimum_mean)
                oracle.minimum_mean = mean;
              continue;
            }

          if (visited[v])
            continue;

          visited[v] = true;
          dfs(start, v, cost + w, len + 1);
          visited[v] = false;
        }
    };

    for (size_t s = 0; s < n; ++s)
      {
        visited[s] = true;
        dfs(s, s, 0, 0);
        visited[s] = false;
      }

    return oracle;
  }


  bool witness_cycle_is_consistent(const Graph & g, const Result & r)
  {
    if (r.cycle_length == 0)
      return r.cycle_nodes.is_empty() and r.cycle_arcs.is_empty();

    if (r.cycle_nodes.size() != r.cycle_length + 1)
      return false;
    if (r.cycle_arcs.size() != r.cycle_length)
      return false;

    auto node_it = r.cycle_nodes.get_it();
    if (not node_it.has_curr())
      return false;

    Node * first = node_it.get_curr();
    Node * curr = first;
    node_it.next_ne();

    for (auto arc_it = r.cycle_arcs.get_it(); arc_it.has_curr(); arc_it.next_ne())
      {
        if (not node_it.has_curr())
          return false;

        Arc * arc = arc_it.get_curr();
        Node * next = node_it.get_curr();

        if (g.get_src_node(arc) != curr or g.get_tgt_node(arc) != next)
          return false;

        curr = next;
        node_it.next_ne();
      }

    return curr == first and not node_it.has_curr();
  }


  struct Hide_Arc
  {
    Arc * blocked = nullptr;

    bool operator()(Arc * arc) const noexcept
    {
      return arc != blocked;
    }
  };
} // namespace


TEST(MinMeanCycleTest, EmptyDigraphReturnsNoCycle)
{
  Graph g;
  const auto r = karp_minimum_mean_cycle(g);

  EXPECT_FALSE(r.has_cycle);
  EXPECT_EQ(r.cycle_length, 0u);
  EXPECT_TRUE(r.cycle_nodes.is_empty());
  EXPECT_TRUE(r.cycle_arcs.is_empty());
}


TEST(MinMeanCycleTest, DagReturnsNoCycle)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 2}, {1, 2, 3}, {2, 3, 4}, {0, 3, 10}
  };

  auto built = build_graph(4, edges);
  const auto r = karp_minimum_mean_cycle(built.g);

  EXPECT_FALSE(r.has_cycle);
  EXPECT_EQ(r.cycle_length, 0u);
}


TEST(MinMeanCycleTest, SelfLoopCanBeOptimalCycle)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 8}, {1, 1, 3}, {1, 2, 7}, {2, 0, 9}
  };

  auto built = build_graph(3, edges);
  const auto r = karp_minimum_mean_cycle(built.g);

  ASSERT_TRUE(r.has_cycle);
  EXPECT_NEAR(static_cast<double>(r.minimum_mean), 3.0, 1e-12);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_EQ(r.cycle_nodes.size(), 2u);
  EXPECT_EQ(r.cycle_total_cost, 3);
  EXPECT_TRUE(witness_cycle_is_consistent(built.g, r));
}


TEST(MinMeanCycleTest, ChoosesMinimumAmongMultipleCycles)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 3}, {1, 0, 1}, // mean 2
      {1, 2, 1}, {2, 1, 1}, // mean 1 (optimal)
      {0, 2, 5}, {2, 0, 5}
  };

  auto built = build_graph(3, edges);
  const auto r = karp_minimum_mean_cycle(built.g);

  ASSERT_TRUE(r.has_cycle);
  EXPECT_NEAR(static_cast<double>(r.minimum_mean), 1.0, 1e-12);
  EXPECT_TRUE(witness_cycle_is_consistent(built.g, r));
}


TEST(MinMeanCycleTest, SupportsNegativeMeanCycles)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, -5}, {1, 2, 1}, {2, 0, 1}, // mean -1 (optimal)
      {0, 3, 4}, {3, 0, 4}
  };

  auto built = build_graph(4, edges);
  const auto r = karp_minimum_mean_cycle(built.g);

  ASSERT_TRUE(r.has_cycle);
  EXPECT_NEAR(static_cast<double>(r.minimum_mean), -1.0, 1e-12);
  EXPECT_TRUE(witness_cycle_is_consistent(built.g, r));
}


TEST(MinMeanCycleTest, ArcFilterChangesResult)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {1, 0, 1}, // mean 1 (will be blocked)
      {2, 3, 3}, {3, 2, 3}  // mean 3
  };

  auto built = build_graph(4, edges);

  const auto full = karp_minimum_mean_cycle(built.g);
  ASSERT_TRUE(full.has_cycle);
  EXPECT_NEAR(static_cast<double>(full.minimum_mean), 1.0, 1e-12);

  const Hide_Arc filter{built.arcs[0]};
  const auto filtered = karp_minimum_mean_cycle<Graph, Dft_Dist<Graph>, Hide_Arc>(
      built.g, Dft_Dist<Graph>(), filter);

  ASSERT_TRUE(filtered.has_cycle);
  EXPECT_NEAR(static_cast<double>(filtered.minimum_mean), 3.0, 1e-12);
  EXPECT_TRUE(witness_cycle_is_consistent(built.g, filtered));
}

TEST(MinMeanCycleTest, AccessorIsReadOncePerAcceptedArc)
{
  // An accessor whose answers may change between calls (live prices, say)
  // must be read once per arc: the table and the witness cost then come
  // from the same values.
  struct Counting_Dist
  {
    using Distance_Type = long long;
    int * calls = nullptr;

    Distance_Type operator()(Arc * arc) const
    {
      ++*calls;
      return arc->get_info();
    }
  };

  auto built = build_graph(4, {{0, 1, 1}, {1, 0, 1}, {1, 2, -4}, {2, 1, 1}, {2, 3, 7}});
  const Hide_Arc filter{built.arcs[4]};

  int calls = 0;
  const auto full = karp_minimum_mean_cycle<Graph, Counting_Dist, Hide_Arc>(
      built.g, Counting_Dist{&calls}, filter);
  ASSERT_TRUE(full.has_cycle);
  EXPECT_EQ(full.minimum_mean, -1.5L);
  EXPECT_EQ(full.cycle_total_cost, -3);
  EXPECT_EQ(calls, 4);

  calls = 0;
  const auto value = karp_minimum_mean_cycle_value<Graph, Counting_Dist, Hide_Arc>(
      built.g, Counting_Dist{&calls}, filter);
  EXPECT_EQ(value.minimum_mean, full.minimum_mean);
  EXPECT_EQ(calls, 4);
}

TEST(MinMeanCycleTest, WitnessMeansAreComparedExactly)
{
  // The choice among the simple cycles of Karp's walk. In exact arithmetic
  // they all have the minimum mean, so no graph seen so far depends on it;
  // it is checked here directly.
  using min_mean_cycle_detail::mean_less;
  EXPECT_TRUE(mean_less(-3LL, 2, -2LL, 2));
  EXPECT_FALSE(mean_less(-3LL, 3, -2LL, 2));    // -1 against -1
  EXPECT_TRUE(mean_less(-5LL, 3, -3LL, 2));     // -5/3 < -3/2
  EXPECT_FALSE(mean_less(-3LL, 2, -5LL, 3));
  EXPECT_TRUE(mean_less(1LL, 3, 1LL, 2));
  EXPECT_FALSE(mean_less(2LL, 6, 1LL, 3));      // equal, not smaller
  EXPECT_TRUE(mean_less(-1.5, 1, -1.0, 1));
# if ALEPH_KARP_INT128
  // Means 1 apart where long double, whatever its precision (53 bits on
  // Apple Silicon and MSVC, 64 on x86, 113 on AArch64 Linux), cannot tell
  // them apart: its unit in the last place there is 4.
  using min_mean_cycle_detail::karp_int128_t;
  constexpr int shift = std::numeric_limits<long double>::digits + 2;
  static_assert(shift <= 124, "3 * 2^shift must fit a 128-bit integer");
  const karp_int128_t big = static_cast<karp_int128_t>(1) << shift;
  ASSERT_EQ(static_cast<long double>(big - 1), static_cast<long double>(big));
  EXPECT_TRUE(mean_less(3 * big - 3, 3, big, 1));
  EXPECT_FALSE(mean_less(big, 1, 3 * big - 3, 3));
# endif
}

TEST(MinMeanCycleTest, TableBytesAreTheBytesArrayReserves)
{
  // Audit 2026-10-02, C3: the bytes of Karp's tables, which the memory limit
  // of Howard's fallback compares. (n + 1) * n entries, rounded up by Array
  // to a power of two; a sum, plus a predecessor position with the witness.
  using namespace min_mean_cycle_detail;
  size_t bytes = 1;
  ASSERT_TRUE((karp_table_bytes<true, long long>(0, Karp_Sums::Long_Long, bytes)));
  EXPECT_EQ(bytes, 0u);
  ASSERT_TRUE((karp_table_bytes<true, long long>(3, Karp_Sums::Long_Long, bytes)));
  EXPECT_EQ(bytes, 16u * (8 + 8));                   // 12 entries -> 16
  ASSERT_TRUE((karp_table_bytes<false, long long>(3, Karp_Sums::Long_Long, bytes)));
  EXPECT_EQ(bytes, 16u * 8);
  ASSERT_TRUE((karp_table_bytes<true, long long>(1000, Karp_Sums::Long_Long, bytes)));
  EXPECT_EQ(bytes, (size_t{1} << 20) * 16);          // 1001000 entries -> 2^20
  ASSERT_TRUE((karp_table_bytes<true, double>(15, Karp_Sums::Cost_Type_Checked, bytes)));
  EXPECT_EQ(bytes, 256u * (sizeof(double) + 8));     // 240 entries -> 256
  ASSERT_TRUE((karp_table_bytes<false, long double>(15, Karp_Sums::Cost_Type_Checked, bytes)));
  EXPECT_EQ(bytes, 256u * sizeof(long double));
# if ALEPH_KARP_INT128
  ASSERT_TRUE((karp_table_bytes<true, long long>(3, Karp_Sums::Int128, bytes)));
  EXPECT_EQ(bytes, 16u * (16 + 8));
# endif

  // Counts that do not fit a size_t: the entries themselves, or the bytes
  // of the power of two that holds them.
  EXPECT_FALSE((karp_table_bytes<true, long long>(std::numeric_limits<size_t>::max() / 2,
                                                  Karp_Sums::Long_Long, bytes)));
  EXPECT_FALSE((karp_table_bytes<false, long long>(size_t{1} << (std::numeric_limits<size_t>::digits / 2 - 1),
                                                   Karp_Sums::Long_Long, bytes)));

  // How the sums are kept: long long while n * max|w| stays below LLONG_MAX.
  using In = Incoming_Arc<Graph, long long>;
  const auto lists = [](const long long w)
  {
    Array<Array<In>> incoming;
    incoming.append(Array<In>());
    incoming.append(Array<In>());
    incoming(0).append(In{1, nullptr, w});
    incoming(1).append(In{0, nullptr, -w});
    return incoming;
  };
  EXPECT_EQ(karp_sums(lists(5)), Karp_Sums::Long_Long);
  EXPECT_EQ(karp_sums(lists(std::numeric_limits<long long>::max() / 2 - 1)), Karp_Sums::Long_Long);
# if ALEPH_KARP_INT128
  EXPECT_EQ(karp_sums(lists(std::numeric_limits<long long>::max() / 2 + 1)), Karp_Sums::Int128);
# else
  EXPECT_EQ(karp_sums(lists(std::numeric_limits<long long>::max() / 2 + 1)), Karp_Sums::Long_Long_Checked);
# endif
  Array<Array<Incoming_Arc<Float_Graph, double>>> floating;
  floating.append(Array<Incoming_Arc<Float_Graph, double>>());
  EXPECT_EQ(karp_sums(floating), Karp_Sums::Cost_Type_Checked);
}

TEST(MinMeanCycleTest, WitnessIsASimpleCycle)
{
  // Audit 2026-10-02, C4. The witness used to be a closed piece of Karp's
  // walk, chosen by its rounded mean: with floating-point weights rounding
  // often favoured a piece that went around a cycle twice (a third of these
  // graphs). Now it is always a simple cycle, of the reported cost.
  const auto check = [](const auto & g, const auto & r, const bool exact) -> ::testing::AssertionResult
  {
    using GT = std::decay_t<decltype(g)>;
    if (r.cycle_length == 0 or r.cycle_nodes.size() != r.cycle_length + 1
        or r.cycle_arcs.size() != r.cycle_length)
      return ::testing::AssertionFailure() << "sizes";
    std::vector<typename GT::Node *> nodes;
    for (auto it = r.cycle_nodes.get_it(); it.has_curr(); it.next_ne())
      nodes.push_back(it.get_curr());
    if (nodes.front() != nodes.back())
      return ::testing::AssertionFailure() << "not closed";
    for (size_t i = 0; i + 1 < nodes.size(); ++i)
      for (size_t j = i + 1; j + 1 < nodes.size(); ++j)
        if (nodes[i] == nodes[j])
          return ::testing::AssertionFailure() << "node repeated: not a simple cycle";
    long double sum = 0;
    size_t i = 0;
    for (auto it = r.cycle_arcs.get_it(); it.has_curr(); it.next_ne(), ++i)
      {
        auto * arc = it.get_curr();
        if (g.get_src_node(arc) != nodes[i] or g.get_tgt_node(arc) != nodes[i + 1])
          return ::testing::AssertionFailure() << "arcs do not follow the nodes";
        sum += static_cast<long double>(arc->get_info());
      }
    const long double cost = static_cast<long double>(r.cycle_total_cost);
    const long double mean = cost / static_cast<long double>(r.cycle_length);
    const long double tol = exact ? 0.0L : 1e-9L * (1.0L + std::fabs(sum));
    if (std::fabs(sum - cost) > tol)
      return ::testing::AssertionFailure() << "cost " << cost << " but arcs add up to " << sum;
    if (std::fabs(mean - r.minimum_mean) > (exact ? 0.0L : 1e-9L * (1.0L + std::fabs(mean))))
      return ::testing::AssertionFailure() << "witness mean " << mean << ", reported " << r.minimum_mean;
    return ::testing::AssertionSuccess();
  };

  std::mt19937_64 rng(20261002);
  for (size_t trial = 0; trial < 3000; ++trial)
    {
      const size_t n = 1 + rng() % 9;
      const size_t m = rng() % (3 * n + 1);
      std::vector<Float_Edge_Def> fedges;
      std::vector<Edge_Def> iedges;
      for (size_t i = 0; i < m; ++i)
        {
          const size_t u = rng() % n;
          const size_t v = rng() % n;
          const double unit = std::uniform_real_distribution<double>(-1.0, 1.0)(rng);
          const int exponent = static_cast<int>(rng() % 40) - 20;
          const long long w = static_cast<long long>(rng() % 41) - 20;
          fedges.emplace_back(u, v, std::ldexp(unit, exponent));
          iedges.emplace_back(u, v, w);
        }

      auto fg = build_float_graph(n, fedges);
      const auto fr = karp_minimum_mean_cycle(fg.g);
      if (fr.has_cycle)
        ASSERT_TRUE(check(fg.g, fr, false)) << "double, trial=" << trial;

      auto ig = build_graph(n, iedges);
      const auto ir = karp_minimum_mean_cycle(ig.g);
      if (ir.has_cycle)
        ASSERT_TRUE(check(ig.g, ir, true)) << "long long, trial=" << trial;
    }
}

TEST(MinMeanCycleTest, FloatingWeightsAndValueOnlyApiWork)
{
  const std::vector<Float_Edge_Def> edges = {
      {0, 1, 0.5}, {1, 0, 1.0}, // mean 0.75
      {1, 2, 2.5}, {2, 1, 2.5}  // mean 2.5
  };

  auto built = build_float_graph(3, edges);
  const auto full = karp_minimum_mean_cycle(built.g);
  const auto value = karp_minimum_mean_cycle_value(built.g);
  const auto alias_value = minimum_mean_cycle_value(built.g);
  const auto value_functor = Karp_Minimum_Mean_Cycle_Value<Float_Graph>()(built.g);

  ASSERT_TRUE(full.has_cycle);
  EXPECT_NEAR(static_cast<double>(full.minimum_mean), 0.75, 1e-12);

  ASSERT_TRUE(value.has_cycle);
  EXPECT_NEAR(static_cast<double>(value.minimum_mean), 0.75, 1e-12);

  ASSERT_TRUE(alias_value.has_cycle);
  EXPECT_NEAR(static_cast<double>(alias_value.minimum_mean), 0.75, 1e-12);

  ASSERT_TRUE(value_functor.has_cycle);
  EXPECT_NEAR(static_cast<double>(value_functor.minimum_mean), 0.75, 1e-12);
}

TEST(MinMeanCycleTest, NonFiniteFloatingWeightsThrowDomainError)
{
  Float_Graph inf_graph;
  auto * i0 = inf_graph.insert_node(0);
  auto * i1 = inf_graph.insert_node(1);
  inf_graph.insert_arc(i0, i1, std::numeric_limits<double>::infinity());
  inf_graph.insert_arc(i1, i0, 1.0);

  EXPECT_THROW((karp_minimum_mean_cycle(inf_graph)), std::domain_error);
  EXPECT_THROW((karp_minimum_mean_cycle_value(inf_graph)), std::domain_error);

  Float_Graph nan_graph;
  auto * n0 = nan_graph.insert_node(0);
  auto * n1 = nan_graph.insert_node(1);
  nan_graph.insert_arc(n0, n1, std::numeric_limits<double>::quiet_NaN());
  nan_graph.insert_arc(n1, n0, 1.0);

  EXPECT_THROW((karp_minimum_mean_cycle(nan_graph)), std::domain_error);
  EXPECT_THROW((karp_minimum_mean_cycle_value(nan_graph)), std::domain_error);
}

TEST(MinMeanCycleTest, IntegerOverflowInAccumulationThrows)
{
  // The only cycle costs LLONG_MAX + 1. With 128-bit sums (audit 2026-10-02,
  // C6) the table holds it: the mean, 2^62, comes out, and only the witness,
  // whose cost long long cannot hold, throws. Without them every sum is a
  // checked long long and both throw.
  const std::vector<Edge_Def> edges = {
      {0, 1, std::numeric_limits<long long>::max()},
      {1, 0, 1}
  };

  auto built = build_graph(2, edges);
  EXPECT_THROW((karp_minimum_mean_cycle(built.g)), std::overflow_error);
# if ALEPH_KARP_INT128
  const auto value = karp_minimum_mean_cycle_value(built.g);
  ASSERT_TRUE(value.has_cycle);
  EXPECT_EQ(value.minimum_mean, 0x1p62L);
# else
  EXPECT_THROW((karp_minimum_mean_cycle_value(built.g)), std::overflow_error);
# endif
}

TEST(MinMeanCycleTest, SumsBeyondTheCostTypeAreExact)
{
  // Audit 2026-10-02, C6. A loop of cost LLONG_MAX used to read as the
  // "unreachable" sentinel and was ignored. And the 2-cycle 0 -> 1 -> 0
  // costs 2^63, which a long long table could not add up, although the
  // cheaper loop at 0 is the answer.
  const long long max = std::numeric_limits<long long>::max();
  const long long q = 1LL << 62;

  auto loop = build_graph(1, {{0, 0, max}});
  auto pair = build_graph(3, {{0, 1, q}, {1, 0, q}, {0, 0, q - 1}, {2, 0, -q}, {2, 1, -q}});
# if ALEPH_KARP_INT128
  const auto r = karp_minimum_mean_cycle(loop.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.minimum_mean, static_cast<long double>(max));
  EXPECT_EQ(r.cycle_total_cost, max);
  EXPECT_EQ(r.cycle_length, 1u);

  const auto p = karp_minimum_mean_cycle(pair.g);
  ASSERT_TRUE(p.has_cycle);
  EXPECT_EQ(p.minimum_mean, static_cast<long double>(q - 1));
  EXPECT_EQ(p.cycle_total_cost, q - 1);
  EXPECT_EQ(p.cycle_length, 1u);
  EXPECT_EQ(karp_minimum_mean_cycle_value(pair.g).minimum_mean, p.minimum_mean);
# else
  EXPECT_FALSE(karp_minimum_mean_cycle(loop.g).has_cycle);   // the known limit without them
# endif

  // Narrower integers are added up in long long on every platform.
  using Int_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  const int imax = std::numeric_limits<int>::max();
  auto narrow = build_graph_generic<Int_Graph, int>(3, {{0, 1, imax}, {1, 0, imax}, {0, 0, imax - 1}});
  const auto i = karp_minimum_mean_cycle(narrow.g);
  ASSERT_TRUE(i.has_cycle);
  EXPECT_EQ(i.cycle_total_cost, imax - 1);
  EXPECT_EQ(i.minimum_mean, static_cast<long double>(imax - 1));
}

TEST(MinMeanCycleTest, BoundedSumsMayReachOneBelowTheSentinel)
{
  // Independent audit of stage C, C1. With n * max|w| = 2^63 - 2 the table
  // stays in long long, and D_2 reaches LLONG_MAX - 1: only LLONG_MAX may
  // mark an unreachable state.
  const long long w = (1LL << 62) - 1;
  auto b = build_graph(2, {{0, 1, w}, {1, 0, w}});
  const auto r = karp_minimum_mean_cycle(b.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.minimum_mean, static_cast<long double>(w));
  EXPECT_EQ(r.cycle_total_cost, 2 * w);
  EXPECT_TRUE(karp_minimum_mean_cycle_value(b.g).has_cycle);
}

TEST(MinMeanCycleTest, AMinimumMeanCycleWhoseCostFitsIsPreferred)
{
  // Independent audit of stage C, T1. The 2-cycle and the loop both have
  // mean -124, but only the loop's cost fits an int8_t. Karp's vertex 0
  // ends a walk with just the 2-cycle; another vertex of the same mean
  // gives the loop, instead of an overflow_error. Equal means are told
  // apart only where they are compared exactly (128-bit integers).
  using Small_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int8_t>>;
# if ALEPH_KARP_INT128
  auto b = build_graph_generic<Small_Graph, int8_t>(3, {{0, 1, -124}, {1, 0, -124}, {2, 2, -124}});
  const auto r = karp_minimum_mean_cycle(b.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.minimum_mean, -124.0L);
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_EQ(r.cycle_total_cost, -124);
  EXPECT_EQ(r.witness_node, b.nodes[2]);
# endif

  // When no minimum-mean cycle fits, the witness cannot be reported.
  auto alone = build_graph_generic<Small_Graph, int8_t>(2, {{0, 1, -124}, {1, 0, -124}});
  EXPECT_THROW(karp_minimum_mean_cycle(alone.g), std::overflow_error);
  EXPECT_EQ(karp_minimum_mean_cycle_value(alone.g).minimum_mean, -124.0L);
}

# if ALEPH_KARP_INT128
TEST(MinMeanCycleTest, TheVertexIsChosenWithExactMeans)
{
  // Independent audit of stage C, K1. The 5-cycle has mean 2^62 + 1/5 and
  // the loop 2^62; with 64 bits of long double, or 53, both round to 2^62.
  // The vertex used to be chosen by those rounded means, the first one on
  // the 5-cycle, whose cost 5 * 2^62 + 1 does not fit a long long.
  const long long a = 1LL << 62;
  auto b = build_graph(6, {{0, 1, a}, {1, 2, a}, {2, 3, a}, {3, 4, a}, {4, 0, a + 1}, {5, 5, a}});
  const auto r = karp_minimum_mean_cycle(b.g);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.minimum_mean, static_cast<long double>(a));
  EXPECT_EQ(r.cycle_length, 1u);
  EXPECT_EQ(r.cycle_total_cost, a);
  EXPECT_EQ(karp_minimum_mean_cycle_value(b.g).minimum_mean, r.minimum_mean);
}
# endif

TEST(MinMeanCycleTest, SupportsArrayDigraphBackend)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 3}, {1, 2, 3}, {2, 0, 0}, // mean 2
      {0, 3, 5}, {3, 0, 5}
  };

  auto built = build_array_graph(4, edges);
  const auto r = karp_minimum_mean_cycle(built.g);
  const auto v = karp_minimum_mean_cycle_value(built.g);

  ASSERT_TRUE(r.has_cycle);
  EXPECT_NEAR(static_cast<double>(r.minimum_mean), 2.0, 1e-12);
  ASSERT_TRUE(v.has_cycle);
  EXPECT_NEAR(static_cast<double>(v.minimum_mean), 2.0, 1e-12);
}


TEST(MinMeanCycleTest, FunctorAndAliasMatchFreeFunction)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 2}, {1, 0, 2},
      {1, 2, 1}, {2, 1, 1}
  };

  auto built = build_graph(3, edges);

  const auto free_result = karp_minimum_mean_cycle(built.g);
  const auto alias_result = minimum_mean_cycle(built.g);
  const auto functor_result = Karp_Minimum_Mean_Cycle<Graph>()(built.g);

  ASSERT_EQ(free_result.has_cycle, alias_result.has_cycle);
  ASSERT_EQ(free_result.has_cycle, functor_result.has_cycle);
  if (free_result.has_cycle)
    {
      EXPECT_NEAR(static_cast<double>(free_result.minimum_mean),
                  static_cast<double>(alias_result.minimum_mean),
                  1e-12);
      EXPECT_NEAR(static_cast<double>(free_result.minimum_mean),
                  static_cast<double>(functor_result.minimum_mean),
                  1e-12);
    }
}


TEST(MinMeanCycleTest, UndirectedGraphThrowsDomainError)
{
  UGraph g;
  auto * n0 = g.insert_node(0);
  auto * n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  EXPECT_THROW((karp_minimum_mean_cycle(g)), std::domain_error);
}


TEST(MinMeanCycleTest, RandomSmallGraphsMatchExactOracle)
{
  std::mt19937_64 rng(0xDA7A5EEDULL);
  std::uniform_int_distribution<int> n_dist(2, 7);
  std::bernoulli_distribution has_edge(0.36);
  std::uniform_int_distribution<int> weight_dist(-9, 13);

  for (size_t trial = 0; trial < 90; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Edge_Def> edges;
      for (size_t u = 0; u < n; ++u)
        for (size_t v = 0; v < n; ++v)
          {
            if (u == v)
              {
                if (std::bernoulli_distribution(0.12)(rng))
                  edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
                continue;
              }

            if (has_edge(rng))
              edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
          }

      auto built = build_graph(n, edges);
      const auto exact = exact_minimum_mean_cycle(built);
      const auto got = karp_minimum_mean_cycle(built.g);
      const auto value_only = karp_minimum_mean_cycle_value(built.g);

      ASSERT_EQ(got.has_cycle, exact.has_cycle) << "trial=" << trial;
      ASSERT_EQ(value_only.has_cycle, exact.has_cycle) << "trial=" << trial;
      if (not got.has_cycle)
        continue;

      EXPECT_NEAR(static_cast<double>(got.minimum_mean),
                  static_cast<double>(exact.minimum_mean),
                  1e-10)
          << "trial=" << trial;
      EXPECT_NEAR(static_cast<double>(value_only.minimum_mean),
                  static_cast<double>(exact.minimum_mean),
                  1e-10)
          << "trial=" << trial;

      EXPECT_GT(got.cycle_length, 0u) << "trial=" << trial;
      EXPECT_TRUE(witness_cycle_is_consistent(built.g, got)) << "trial=" << trial;

      const long double witness_mean = static_cast<long double>(got.cycle_total_cost)
                                       / static_cast<long double>(got.cycle_length);
      EXPECT_GE(witness_mean + 1e-12L, got.minimum_mean) << "trial=" << trial;
    }
}
