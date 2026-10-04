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
 * @file bounded_negative_cycle_test.cc
 * @brief Tests for most_negative_cycle_bounded() (Negative_Cycles.H).
 */

# include <gtest/gtest.h>

# include <cmath>
# include <algorithm>
# include <functional>
# include <limits>
# include <random>
# include <set>
# include <type_traits>
# include <tuple>
# include <vector>

# include <Min_Mean_Cycle.H>
# include <Negative_Cycles.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>

# include "negative_cycles_test_support.H"

using namespace Aleph;
using namespace Negative_Cycles_Test_Support;

namespace
{
  // The witness must be a closed walk of consecutive graph arcs, without
  // repeated nodes, whose weights sum to total_cost.
  template <class GT, typename Cost>
  bool witness_is_simple_cycle(const GT & g, const Bounded_Cycle_Result<GT, Cost> & r)
  {
    if (not r.has_cycle)
      return r.length == 0 and r.cycle_nodes.is_empty() and r.cycle_arcs.is_empty();

    if (r.length == 0 or r.cycle_nodes.size() != r.length + 1 or r.cycle_arcs.size() != r.length)
      return false;

    std::set<typename GT::Node *> seen;
    auto node_it = r.cycle_nodes.get_it();
    typename GT::Node * first = node_it.get_curr();
    typename GT::Node * curr = first;
    node_it.next_ne();

    Cost sum = Cost{0};
    long double abs_sum = 0.0L;
    for (auto arc_it = r.cycle_arcs.get_it(); arc_it.has_curr(); arc_it.next_ne())
      {
        typename GT::Arc * arc = arc_it.get_curr();
        if (not node_it.has_curr() or g.get_src_node(arc) != curr
            or g.get_tgt_node(arc) != node_it.get_curr() or not seen.insert(curr).second)
          return false;
        sum += arc->get_info();
        abs_sum += std::fabs(static_cast<long double>(arc->get_info()));
        curr = node_it.get_curr();
        node_it.next_ne();
      }

    if (curr != first or node_it.has_curr())
      return false;
    if constexpr (std::is_floating_point_v<Cost>)
      // total_cost is a compensated sum: it may differ from the plain sum above by
      // the rounding error of plain summation, at most (terms + 1) * eps * sum of |w|
      return std::fabs(static_cast<long double>(sum) - static_cast<long double>(r.total_cost))
             <= (r.length + 1) * static_cast<long double>(std::numeric_limits<Cost>::epsilon()) * abs_sum;
    else
      return sum == r.total_cost;
  }

  struct Oracle
  {
    bool exists = false;
    long long min_cost = 0;
    size_t min_len = 0;   // fewest arcs among the simple cycles of cost min_cost
  };

  // Exhaustive minimum over simple cycles of at most max_length arcs.
  Oracle exact_bounded_minimum(const Built_Graph_T<Graph> & built, const size_t max_length)
  {
    const size_t n = built.nodes.size();
    std::vector<bool> visited(n, false);
    Oracle oracle;

    std::function<void(size_t, size_t, long long, size_t)> dfs =
      [&](const size_t start, const size_t u, const long long cost, const size_t len)
    {
      if (len >= max_length)
        return;
      for (Node_Arc_Iterator<Graph> it(built.nodes[u]); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_current_arc_ne();
          const size_t v = static_cast<size_t>(it.get_tgt_node()->get_info());
          const long long w = arc->get_info();
          if (v == start)
            {
              const long long total = cost + w;
              if (not oracle.exists or total < oracle.min_cost
                  or (total == oracle.min_cost and len + 1 < oracle.min_len))
                {
                  oracle.exists = true;
                  oracle.min_cost = total;
                  oracle.min_len = len + 1;
                }
              continue;
            }
          if (v < start or visited[v])   // canonical start: the minimum node of the cycle
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


  // Builds the 17-node graph of F3 (auditoria-rama-arbitrage-performance-bugs.md).
  // Arcs are inserted in the order that matters: the dynamic program keeps a
  // single predecessor per (layer, node), so ties are resolved by insertion
  // order. Simple cycles: four 2-arc cycles hanging from the hubs 4..11 of
  // cost -2 or -3, the 4-arc cycle 0-1-2-3 of cost -5, and the separate ring
  // 12..16 of 5 arcs and cost -5.
  Built_Graph_T<Graph> build_tie_graph()
  {
    std::vector<Edge_Def> edges;
    for (size_t i = 0; i < 4; ++i)
      {
        const size_t h = 4 + 2 * i;
        const size_t j = h + 1;
        edges.emplace_back(i, h, 0);
        edges.emplace_back(h, i, -2);
        edges.emplace_back(h, j, -2);
        edges.emplace_back(j, h, -1);
      }
    edges.emplace_back(0, 1, -2);
    edges.emplace_back(1, 2, -1);
    edges.emplace_back(2, 3, -1);
    edges.emplace_back(3, 0, -1);
    for (size_t i = 12; i < 17; ++i)
      edges.emplace_back(i, i == 16 ? 12 : i + 1, -1);
    return build_graph(17, edges);
  }

  // The same triangle of total cost -1 for every cost type.
  template <typename W>
  void expect_triangle_found_with_cost_type()
  {
    using G = List_Digraph<Graph_Node<int>, Graph_Arc<W>>;
    auto built = build_graph_generic<G, W>(
        3, {{0, 1, W(1)}, {1, 2, W(-3)}, {2, 0, W(1)}});

    const auto r = most_negative_cycle_bounded(built.g, 3);
    ASSERT_TRUE(r.has_cycle);
    EXPECT_TRUE(r.matches_relaxed_bound);
    // Small integer weights: every sum is exact in any of these types, so the
    // certificate is a proof also for floating-point costs.
    EXPECT_TRUE(r.is_exact);
    EXPECT_EQ(r.optimality_gap, W(0));
    EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact);
    EXPECT_EQ(r.total_cost, W(-1));
    EXPECT_EQ(r.length, 3u);
    EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
  }
} // namespace


TEST(BoundedNegativeCycleTest, TriangleFoundWithBoundThreeButNotTwo)
{
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}});

  const auto r3 = most_negative_cycle_bounded(built.g, 3);
  ASSERT_TRUE(r3.has_cycle);
  EXPECT_TRUE(r3.is_negative());
  EXPECT_TRUE(r3.is_exact);
  EXPECT_EQ(r3.total_cost, -1);
  EXPECT_EQ(r3.length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r3));

  const auto r2 = most_negative_cycle_bounded(built.g, 2);
  EXPECT_FALSE(r2.has_cycle);
  EXPECT_FALSE(r2.is_negative());
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r2));
}


TEST(BoundedNegativeCycleTest, RanksByTotalCostUnlikeKarpMean)
{
  // T: 0 -> 1 -> 2 -> 0 costs -3 over 3 arcs (mean -1).
  // S: 0 -> 3 -> 4 -> 5 -> 6 -> 0 costs -4 over 5 arcs (mean -0.8).
  const std::vector<Edge_Def> edges = {
      {0, 1, -1}, {1, 2, -1}, {2, 0, -1},
      {0, 3, -1}, {3, 4, -1}, {4, 5, -1}, {5, 6, -1}, {6, 0, 0}
  };
  auto built = build_graph(7, edges);

  const auto karp = karp_minimum_mean_cycle(built.g);
  ASSERT_TRUE(karp.has_cycle);
  EXPECT_NEAR(static_cast<double>(karp.minimum_mean), -1.0, 1e-12);   // Karp picks T

  const auto r = most_negative_cycle_bounded(built.g, 5);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_TRUE(r.is_exact);
  EXPECT_EQ(r.total_cost, -4);   // total cost picks S
  EXPECT_EQ(r.length, 5u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));

  // With the bound below S's length, T is the best cycle that fits.
  const auto r4 = most_negative_cycle_bounded(built.g, 4);
  ASSERT_TRUE(r4.has_cycle);
  EXPECT_EQ(r4.total_cost, -3);
  EXPECT_EQ(r4.length, 3u);
}


TEST(BoundedNegativeCycleTest, TiesPreferShorterCycles)
{
  // Both cycles cost -1: the 2-cycle must win over the 3-cycle.
  auto built = build_graph(4, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {2, 3, -2}, {3, 2, 1}});

  const auto r = most_negative_cycle_bounded(built.g, 3);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -1);
  EXPECT_EQ(r.length, 2u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(BoundedNegativeCycleTest, NonSimpleOptimalWalkIsDecomposedAndFlagged)
{
  // From 0, the walk 0 -> 1 -> 2 -> 1 -> 3 -> 1 -> 0 (6 arcs) costs -4 by
  // going around both 2-cycles hanging from 1, and beats every closed walk
  // that is a simple cycle; the best simple cycle is S: 0 -> 4 -> 5 -> 0 at
  // -3. S must still be reported (it is a candidate through its own nodes),
  // but the -4 walk keeps is_exact from certifying it.
  const std::vector<Edge_Def> edges = {
      {0, 1, 0}, {1, 0, 0},
      {1, 2, -1}, {2, 1, -1},
      {1, 3, -1}, {3, 1, -1},
      {0, 4, -1}, {4, 5, -1}, {5, 0, -1}
  };
  auto built = build_graph(6, edges);

  const auto r = most_negative_cycle_bounded(built.g, 6);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.is_exact);
  EXPECT_EQ(r.total_cost, -3);
  EXPECT_EQ(r.length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));

  // With max_length = 3 no walk can repeat a node: the same answer, certified.
  const auto r3 = most_negative_cycle_bounded(built.g, 3);
  ASSERT_TRUE(r3.has_cycle);
  EXPECT_TRUE(r3.is_exact);
  EXPECT_EQ(r3.total_cost, -3);
}


TEST(BoundedNegativeCycleTest, ReportsTheCheapestCycleEvenWhenNotNegative)
{
  auto built = build_graph(3, {{0, 1, 2}, {1, 0, 3}, {1, 2, 1}, {2, 1, 1}});

  const auto r = most_negative_cycle_bounded(built.g, 4);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.is_negative());
  EXPECT_TRUE(r.is_exact);
  EXPECT_EQ(r.total_cost, 2);   // 1 -> 2 -> 1
  EXPECT_EQ(r.length, 2u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(BoundedNegativeCycleTest, TrivialInputsHaveNoCycle)
{
  Graph empty;
  EXPECT_FALSE(most_negative_cycle_bounded(empty, 5).has_cycle);

  auto dag = build_graph(3, {{0, 1, -4}, {1, 2, -4}, {0, 2, -9}});
  EXPECT_FALSE(most_negative_cycle_bounded(dag.g, 5).has_cycle);

  auto cyc = build_graph(2, {{0, 1, -1}, {1, 0, -1}});
  EXPECT_FALSE(most_negative_cycle_bounded(cyc.g, 0).has_cycle);
  EXPECT_FALSE(most_negative_cycle_bounded(cyc.g, 1).has_cycle);
  EXPECT_TRUE(most_negative_cycle_bounded(cyc.g, 2).has_cycle);
}


// max_length == 0 must not snapshot nor validate the graph: no exception for
// an undirected graph and no call to the distance accessor.
TEST(BoundedNegativeCycleTest, MaxLengthZeroDoesNotTouchTheGraph)
{
  UGraph undirected;
  auto * u0 = undirected.insert_node(0);
  auto * u1 = undirected.insert_node(1);
  undirected.insert_arc(u0, u1, -1);
  EXPECT_NO_THROW({ EXPECT_FALSE(most_negative_cycle_bounded(undirected, 0).has_cycle); });

  auto built = build_graph(2, {{0, 1, -1}, {1, 0, -1}});
  int calls = 0;
  int filter_calls = 0;
  const auto r = most_negative_cycle_bounded<Graph, Counting_Dist, Counting_Filter>(
      built.g, 0, Counting_Dist{&calls}, Counting_Filter{&filter_calls, nullptr});
  EXPECT_FALSE(r.has_cycle);
  EXPECT_EQ(calls, 0);
  EXPECT_EQ(filter_calls, 0);
}


TEST(BoundedNegativeCycleTest, SelfLoopIsALengthOneCycle)
{
  auto built = build_graph(2, {{0, 0, -2}, {0, 1, -5}, {1, 0, 1}});

  const auto r1 = most_negative_cycle_bounded(built.g, 1);
  ASSERT_TRUE(r1.has_cycle);
  EXPECT_EQ(r1.total_cost, -2);
  EXPECT_EQ(r1.length, 1u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r1));

  const auto r2 = most_negative_cycle_bounded(built.g, 2);
  ASSERT_TRUE(r2.has_cycle);
  EXPECT_EQ(r2.total_cost, -4);   // 0 -> 1 -> 0
  EXPECT_EQ(r2.length, 2u);
}


TEST(BoundedNegativeCycleTest, BoundAboveNodeCountIsCapped)
{
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}});

  const auto r = most_negative_cycle_bounded(built.g, 1000);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -1);
  EXPECT_EQ(r.length, 3u);
}


TEST(BoundedNegativeCycleTest, ArcFilterChangesResult)
{
  auto built = build_graph(4, {{0, 1, -4}, {1, 0, 1}, {2, 3, -2}, {3, 2, 1}});

  const auto full = most_negative_cycle_bounded(built.g, 2);
  ASSERT_TRUE(full.has_cycle);
  EXPECT_EQ(full.total_cost, -3);

  const Hide_Arc filter{built.arcs[0]};
  const auto filtered = most_negative_cycle_bounded<Graph, Dft_Dist<Graph>, Hide_Arc>(
      built.g, 2, Dft_Dist<Graph>(), filter);
  ASSERT_TRUE(filtered.has_cycle);
  EXPECT_EQ(filtered.total_cost, -1);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, filtered));
}


TEST(BoundedNegativeCycleTest, ParallelArcsUseTheCheapestOne)
{
  auto built = build_graph(2, {{0, 1, 5}, {0, 1, -4}, {1, 0, 1}});

  const auto r = most_negative_cycle_bounded(built.g, 2);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -3);
  EXPECT_EQ(r.cycle_arcs.get_first(), built.arcs[1]);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(BoundedNegativeCycleTest, FloatingWeightsAndArrayBackendWork)
{
  auto fbuilt = build_graph_generic<Float_Graph, double>(
      3, {{0, 1, -0.75}, {1, 2, 0.25}, {2, 0, 0.25}, {2, 1, 0.5}});
  const auto fr = most_negative_cycle_bounded(fbuilt.g, 3);
  ASSERT_TRUE(fr.has_cycle);
  EXPECT_TRUE(fr.matches_relaxed_bound);
  // Binary fractions of a small range: every sum of the search is exact, so the
  // certificate is a proof.
  EXPECT_TRUE(fr.is_exact);
  EXPECT_EQ(fr.optimality_gap, 0.0);
  EXPECT_EQ(fr.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_NEAR(fr.total_cost, -0.25, 1e-12);
  EXPECT_EQ(fr.length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(fbuilt.g, fr));

  auto abuilt = build_graph_generic<Arr_Digraph, long long>(
      3, {{0, 1, -4}, {1, 0, 1}, {1, 2, 3}, {2, 1, 3}});
  const auto ar = most_negative_cycle_bounded(abuilt.g, 4);
  ASSERT_TRUE(ar.has_cycle);
  EXPECT_EQ(ar.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_EQ(ar.total_cost, -3);
  EXPECT_TRUE(witness_is_simple_cycle(abuilt.g, ar));
}


TEST(BoundedNegativeCycleTest, InvalidInputsThrow)
{
  UGraph undirected;
  auto * u0 = undirected.insert_node(0);
  auto * u1 = undirected.insert_node(1);
  undirected.insert_arc(u0, u1, -1);
  EXPECT_THROW((most_negative_cycle_bounded(undirected, 2)), std::domain_error);

  Float_Graph inf_graph;
  auto * i0 = inf_graph.insert_node(0);
  auto * i1 = inf_graph.insert_node(1);
  inf_graph.insert_arc(i0, i1, std::numeric_limits<double>::infinity());
  inf_graph.insert_arc(i1, i0, 1.0);
  EXPECT_THROW((most_negative_cycle_bounded(inf_graph, 2)), std::domain_error);

  // 0 -> 1 reaches max, then max + 1 overflows on the way back.
  auto overflow = build_graph(2, {{0, 1, std::numeric_limits<long long>::max()}, {1, 0, 1}});
  EXPECT_THROW((most_negative_cycle_bounded(overflow.g, 2)), std::overflow_error);
}


// numeric_limits::max() used to double as the "unreachable" sentinel, so a
// closed walk costing exactly max was discarded as if it did not exist.
TEST(BoundedNegativeCycleTest, MaxCostIsNotTreatedAsUnreachable)
{
  const long long M = std::numeric_limits<long long>::max();

  auto loop = build_graph(1, {{0, 0, M}});
  const auto r = most_negative_cycle_bounded(loop.g, 1);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, M);
  EXPECT_EQ(r.length, 1u);
  EXPECT_TRUE(witness_is_simple_cycle(loop.g, r));

  // A 2-cycle whose cost is exactly max is also found and beats nothing else.
  auto pair = build_graph(2, {{0, 1, M - 1}, {1, 0, 1}});
  const auto r2 = most_negative_cycle_bounded(pair.g, 2);
  ASSERT_TRUE(r2.has_cycle);
  EXPECT_EQ(r2.total_cost, M);
}


// ---------------------------------------------------------------------------
// F1 of auditoria-rama-arbitrage-performance-bugs.md (Stage 2, compensated cost).
//
// The only cycle has the weights 1e16, 1, -1e16, -1, all exactly
// representable, and its exact sum is zero (computed here with integers).
// Accumulated in plain double, ((1e16 + 1) - 1e16) - 1 loses the +1 and
// gives -1, which used to be reported as a certified negative cycle. The cost
// of the reported cycle is now recomputed with compensated summation, and the
// relaxed-bound condition is withdrawn when it disagrees in sign with the plain
// sum the search ranked. Whatever order the weights are inserted in, the cycle
// must never be presented as negative, and with floating-point weights it is
// never certified.
// ---------------------------------------------------------------------------
TEST(BoundedNegativeCycleTest, CancellationDoesNotFakeANegativeCycle)
{
  std::vector<double> weights = {-1.0, -1e16, 1.0, 1e16};
  std::sort(weights.begin(), weights.end());

  size_t orders = 0;
  size_t certified = 0;
  do
    {
      auto built = build_graph_generic<Float_Graph, double>(
          4, {{0, 1, weights[0]}, {1, 2, weights[1]}, {2, 3, weights[2]}, {3, 0, weights[3]}});

      long long exact_sum = 0;   // independent oracle: integer arithmetic, exact here
      for (const double w : weights)
        exact_sum += static_cast<long long>(w);
      ASSERT_EQ(exact_sum, 0);

      const auto r = most_negative_cycle_bounded(built.g, 4);
      ASSERT_TRUE(r.has_cycle);
      EXPECT_EQ(r.length, 4u);
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
      EXPECT_EQ(r.total_cost, 0.0);
      EXPECT_FALSE(r.is_negative());

      // The enumeration never reports it either.
      EXPECT_TRUE(find_disjoint_negative_cycles(built.g, 1).is_empty());

      EXPECT_FALSE(r.is_exact);
      certified += r.matches_relaxed_bound;
      ++orders;
    }
  while (std::next_permutation(weights.begin(), weights.end()));

  EXPECT_EQ(orders, 24u);
  // The relaxed-bound condition is kept only when plain and compensated sums
  // agree on the sign: some orders do (plain sum 0), most do not.
  EXPECT_LT(certified, orders);
}


// ---------------------------------------------------------------------------
// Audit 2026-10-02, H1. The ring 0 -> 1 -> 2 -> 3 -> 0 weighs 2^e, 1, -2^e, -2:
// its exact total is -1, an identity between powers of two, but for e = 54
// and e = 100 its plain double sum loses the 1 and gives -2, below the
// self-loop of node 4 (-1.5, the exact optimum). Every weight is a binary
// fraction represented exactly.
//
// The search used to return the ring "certified optimal". The candidates it
// offers are now compared by their exact sums, so the loop wins; and since the
// dynamic program itself ran on rounded sums, no certificate is given: the gap
// is rigorous, and large here because the weights are.
// ---------------------------------------------------------------------------
TEST(BoundedNegativeCycleTest, CandidatesAreRankedExactlyAndTheGapAdmitsRounding)
{
  for (const int e : {54, 100})
    {
      const double big = std::ldexp(1.0, e);
      auto built = build_graph_generic<Float_Graph, double>(
          5, {{0, 1, big}, {1, 2, 1.0}, {2, 3, -big}, {3, 0, -2.0}, {4, 4, -1.5}});

      const auto r = most_negative_cycle_bounded(built.g, 4);
      ASSERT_TRUE(r.has_cycle) << "e=" << e;
      EXPECT_EQ(r.total_cost, -1.5) << "e=" << e;          // the exact optimum
      EXPECT_EQ(r.length, 1u) << "e=" << e;
      EXPECT_TRUE(r.is_negative()) << "e=" << e;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "e=" << e;
      EXPECT_FALSE(r.is_exact) << "e=" << e;                // the search rounded: no proof
      EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded) << "e=" << e;
      EXPECT_GE(r.optimality_gap, 0.5) << "e=" << e;        // covers the ring's rounding (-2 vs -1)
    }

  // The same structure with integer weights, where every sum is exact: the
  // loop wins and the certificate is a proof. (Weights doubled to keep them
  // integral: ring total -2, loop -3.)
  const long long big = 1LL << 54;
  auto built = build_graph(5, {{0, 1, big}, {1, 2, 2}, {2, 3, -big}, {3, 0, -4}, {4, 4, -3}});
  const auto r = most_negative_cycle_bounded(built.g, 4);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_TRUE(r.is_exact);
  EXPECT_EQ(r.optimality_gap, 0);
  EXPECT_EQ(r.total_cost, -3);
  EXPECT_EQ(r.length, 1u);
}

TEST(BoundedNegativeCycleTest, TheRoundingFilterKeepsCandidatesNearItsBound)
{
  // Stage D6 drops, without an exact sum, a candidate whose plain sum is
  // above the best one's by more than twice the rounding bound. Here
  // 2^54 + 2 ties to even at 2^54, so the triangle's plain sum is -4 while
  // its exact cost is -2: it is offered first, and the loop just below -2,
  // two units worse in plain sums, is exactly better. A filter whose bound
  // is about 32 times too small drops the loop.
  const double big = std::ldexp(1.0, 54);
  const double loop = std::nextafter(-2.0, -3.0);
  ASSERT_EQ(static_cast<double>(big + 2.0) + -(big + 4.0), -4.0);   // the cast drops excess precision
  auto built = build_graph_generic<Float_Graph, double>(
      4, {{0, 1, big}, {1, 2, 2.0}, {2, 0, -(big + 4.0)}, {3, 3, loop}});
  const auto r = most_negative_cycle_bounded(built.g, 3);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.length, 1u);
  EXPECT_EQ(r.total_cost, loop);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}

TEST(BoundedNegativeCycleTest, SumsAtTheEdgeOfTheExactRangeAreCertified)
{
  // Final independent review of the branch, mutation M06: nothing pinned the
  // exact regime from the conservative side. Sums of two weights with bits
  // from 2^0 to 2^51 have at most 53 bits, so every sum of the search is
  // exact and the result is a proof; with bits up to 2^52 they may round.
  auto edge = build_graph_generic<Float_Graph, double>(
      2, {{0, 1, 0x1p51 + 1}, {1, 0, -(0x1p51 + 3)}});
  const auto r = most_negative_cycle_bounded(edge.g, 2);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -2.0);
  EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Exact);
  EXPECT_EQ(r.optimality_gap, 0.0);
  EXPECT_TRUE(r.is_exact);

  auto beyond = build_graph_generic<Float_Graph, double>(
      2, {{0, 1, 0x1p52 + 1}, {1, 0, -(0x1p52 + 3)}});
  const auto s = most_negative_cycle_bounded(beyond.g, 2);
  ASSERT_TRUE(s.has_cycle);
  EXPECT_EQ(s.total_cost, -2.0);
  EXPECT_EQ(s.numeric_quality, Cycle_Numeric_Quality::Rounded);
  EXPECT_GT(s.optimality_gap, 0.0);
}


// ---------------------------------------------------------------------------
// Audit 2026-10-02, H2. The six-arc ring with weights 2^100, 1, 2^-100, -1,
// -2^100, -2^-100 (in some rotation) sums exactly to zero: three pairs that
// cancel. Compensated summation, which the reported cost used to come from,
// gave -2^-100 because its compensation is rounded too, and the zero cycle was
// presented as negative. The cost is now an exact sum rounded once.
// ---------------------------------------------------------------------------
TEST(BoundedNegativeCycleTest, ACycleOfExactTotalZeroIsNeverNegative)
{
  const double big = std::ldexp(1.0, 100);
  const double tiny = std::ldexp(1.0, -100);
  const std::vector<double> ring = {big, 1.0, tiny, -1.0, -big, -tiny};

  for (size_t rotation = 0; rotation < ring.size(); ++rotation)
    {
      std::vector<std::tuple<size_t, size_t, double>> edges;
      for (size_t i = 0; i < ring.size(); ++i)
        edges.emplace_back(i, (i + 1) % ring.size(), ring[(i + rotation) % ring.size()]);
      auto built = build_graph_generic<Float_Graph, double>(ring.size(), edges);

      const auto r = most_negative_cycle_bounded(built.g, ring.size());
      ASSERT_TRUE(r.has_cycle) << "rotation=" << rotation;
      EXPECT_EQ(r.length, ring.size()) << "rotation=" << rotation;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "rotation=" << rotation;
      EXPECT_EQ(r.total_cost, 0.0) << "rotation=" << rotation;
      EXPECT_FALSE(r.is_negative()) << "rotation=" << rotation;
      EXPECT_EQ(r.numeric_quality, Cycle_Numeric_Quality::Rounded) << "rotation=" << rotation;

      // The enumeration does not report it either.
      EXPECT_TRUE(find_disjoint_negative_cycles(built.g, 5).is_empty()) << "rotation=" << rotation;
    }
}


// ---------------------------------------------------------------------------
// F3: the documented contract does not promise the globally cheapest cycle
// with fewest arcs, nor that `is_exact` is false only when no simple walk
// attains the minimum. These two cases pin the behaviour with small integers
// (no floating point involved) and an independent exhaustive oracle.
// ---------------------------------------------------------------------------
TEST(BoundedNegativeCycleTest, OptimalCostDoesNotImplyFewestArcs)
{
  auto built = build_tie_graph();

  const Oracle oracle = exact_bounded_minimum(built, 5);
  ASSERT_TRUE(oracle.exists);
  EXPECT_EQ(oracle.min_cost, -5);
  EXPECT_EQ(oracle.min_len, 4u);   // the 4-arc cycle 0-1-2-3 attains the optimum

  const auto r = most_negative_cycle_bounded(built.g, 5);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_TRUE(r.is_exact);                 // the cost is certified optimal...
  EXPECT_EQ(r.total_cost, oracle.min_cost);
  EXPECT_EQ(r.length, 5u);                 // ...the number of arcs is not: the ring, not the 4-cycle
  EXPECT_GT(r.length, oracle.min_len);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(BoundedNegativeCycleTest, SimpleMinimumExistsButCertificateIsFalse)
{
  auto built = build_tie_graph();

  const Oracle oracle = exact_bounded_minimum(built, 4);
  ASSERT_TRUE(oracle.exists);
  EXPECT_EQ(oracle.min_cost, -5);   // the 4-cycle 0-1-2-3 is a simple cycle of cost -5

  // The dynamic program keeps a non-simple walk of the same cost at some
  // states, so the 4-cycle is never offered: the result is the best of the
  // evaluated candidates, strictly worse than the optimum, and the flag says so.
  const auto r = most_negative_cycle_bounded(built.g, 4);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_FALSE(r.is_exact);
  EXPECT_EQ(r.total_cost, -3);
  EXPECT_EQ(r.length, 2u);
  EXPECT_GT(r.total_cost, oracle.min_cost);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
  EXPECT_LT(r.total_cost, 0);   // still strictly negative: the bound c_opt <= cost <= c_opt/m holds
}


TEST(BoundedNegativeCycleTest, DistanceAndFilterAreCalledOncePerArc)
{
  // 5 arcs, one of them hidden by the filter: the filter is asked about all
  // of them exactly once and the distance accessor only about accepted ones.
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {0, 2, 7}, {2, 2, 4}});
  int dist_calls = 0;
  int filter_calls = 0;

  const auto r = most_negative_cycle_bounded<Graph, Counting_Dist, Counting_Filter>(
      built.g, 4, Counting_Dist{&dist_calls}, Counting_Filter{&filter_calls, built.arcs[3]});

  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -1);
  EXPECT_EQ(filter_calls, 5);
  EXPECT_EQ(dist_calls, 4);
}


TEST(BoundedNegativeCycleTest, NonFiniteWeightsHiddenByTheFilterAreIgnored)
{
  Float_Graph g;
  auto * a = g.insert_node(0);
  auto * b = g.insert_node(1);
  auto * c = g.insert_node(2);
  g.insert_arc(a, b, -2.0);
  g.insert_arc(b, a, 1.0);                                          // cycle of cost -1
  g.insert_arc(b, c, std::numeric_limits<double>::infinity());
  g.insert_arc(c, a, std::numeric_limits<double>::quiet_NaN());

  EXPECT_THROW((most_negative_cycle_bounded(g, 4)), std::domain_error);

  const auto r = most_negative_cycle_bounded<Float_Graph, Dft_Dist<Float_Graph>, Finite_Arcs>(
      g, 4, Dft_Dist<Float_Graph>(), Finite_Arcs());
  ASSERT_TRUE(r.has_cycle);
  EXPECT_NEAR(r.total_cost, -1.0, 1e-12);
  EXPECT_EQ(r.length, 2u);
}


TEST(BoundedNegativeCycleTest, NonNullCookiesAndBitsAreLeftUntouched)
{
  auto built = build_graph(4, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {2, 3, -2}, {3, 2, 1}});
  Planted_State<Graph> planted;
  planted.plant(built);
  ASSERT_TRUE(planted.intact(built));

  const auto r = most_negative_cycle_bounded(built.g, 4);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_TRUE(planted.intact(built));
}


TEST(BoundedNegativeCycleTest, SingleNodeSelfLoops)
{
  for (const long long w : {-3LL, 0LL, 3LL})
    {
      auto built = build_graph(1, {{0, 0, w}});
      for (const size_t bound : {1u, 2u, 100u})
        {
          const auto r = most_negative_cycle_bounded(built.g, bound);
          ASSERT_TRUE(r.has_cycle) << "w=" << w << " bound=" << bound;
          EXPECT_EQ(r.total_cost, w);
          EXPECT_EQ(r.length, 1u);
          EXPECT_TRUE(r.is_exact);
          EXPECT_EQ(r.is_negative(), w < 0);
          EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
        }
    }

  Graph lonely;
  lonely.insert_node(0);
  EXPECT_FALSE(most_negative_cycle_bounded(lonely, 3).has_cycle);
}


TEST(BoundedNegativeCycleTest, DisconnectedGraphsAreSearchedInEveryComponent)
{
  // 0 <-> 1 positive, 2 isolated, 3 <-> 4 negative, 5 -> 6 acyclic.
  auto built = build_graph(7, {{0, 1, 2}, {1, 0, 2}, {3, 4, -2}, {4, 3, 1}, {5, 6, -9}});

  const auto r = most_negative_cycle_bounded(built.g, 4);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -1);
  EXPECT_EQ(r.length, 2u);
  EXPECT_TRUE(r.is_exact);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
  EXPECT_EQ(r.cycle_nodes.get_first(), built.nodes[3]);   // the negative component, not the positive one
}


TEST(BoundedNegativeCycleTest, IntCostsWork)
{
  expect_triangle_found_with_cost_type<int>();
}


TEST(BoundedNegativeCycleTest, LongDoubleCostsWork)
{
  expect_triangle_found_with_cost_type<long double>();
}


TEST(BoundedNegativeCycleTest, CostsAtTheLimitsOfTheType)
{
  using IntGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using LdGraph = List_Digraph<Graph_Node<int>, Graph_Arc<long double>>;
  const int int_min = std::numeric_limits<int>::min();
  const int int_max = std::numeric_limits<int>::max();
  const long double ld_lowest = std::numeric_limits<long double>::lowest();
  const long double ld_max = std::numeric_limits<long double>::max();

  // A loop of the extreme value is a cycle of that cost, found even though
  // the value equals the type's maximum (no value is reserved as a sentinel).
  auto int_lo = build_graph_generic<IntGraph, int>(1, {{0, 0, int_min}});
  const auto r_lo = most_negative_cycle_bounded(int_lo.g, 1);
  ASSERT_TRUE(r_lo.has_cycle);
  EXPECT_EQ(r_lo.total_cost, int_min);

  auto int_hi = build_graph_generic<IntGraph, int>(1, {{0, 0, int_max}});
  const auto r_hi = most_negative_cycle_bounded(int_hi.g, 1);
  ASSERT_TRUE(r_hi.has_cycle);
  EXPECT_EQ(r_hi.total_cost, int_max);
  EXPECT_FALSE(r_hi.is_negative());

  auto ld_lo = build_graph_generic<LdGraph, long double>(1, {{0, 0, ld_lowest}});
  const auto q_lo = most_negative_cycle_bounded(ld_lo.g, 1);
  ASSERT_TRUE(q_lo.has_cycle);
  EXPECT_EQ(q_lo.total_cost, ld_lowest);

  auto ld_hi = build_graph_generic<LdGraph, long double>(1, {{0, 0, ld_max}});
  const auto q_hi = most_negative_cycle_bounded(ld_hi.g, 1);
  ASSERT_TRUE(q_hi.has_cycle);
  EXPECT_EQ(q_hi.total_cost, ld_max);

  // Sums that leave the type throw: overflow for integers, a non-finite
  // accumulation for floating point.
  auto int_two = build_graph_generic<IntGraph, int>(2, {{0, 1, int_max}, {1, 0, int_max}});
  EXPECT_THROW((most_negative_cycle_bounded(int_two.g, 2)), std::overflow_error);
  auto ld_two = build_graph_generic<LdGraph, long double>(2, {{0, 1, ld_max}, {1, 0, ld_max}});
  EXPECT_THROW((most_negative_cycle_bounded(ld_two.g, 2)), std::domain_error);
}


TEST(BoundedNegativeCycleTest, RandomGraphsAgreeWithExhaustiveOracle)
{
  std::mt19937_64 rng(0xB0DEDC1CULL);
  std::uniform_int_distribution<int> n_dist(2, 7);
  std::uniform_int_distribution<int> len_dist(1, 6);
  std::bernoulli_distribution has_edge(0.4);
  std::bernoulli_distribution has_loop(0.1);
  std::uniform_int_distribution<int> weight_dist(-9, 13);

  size_t exact_results = 0;
  size_t results = 0;
  for (size_t trial = 0; trial < 200; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      const size_t L = static_cast<size_t>(len_dist(rng));
      std::vector<Edge_Def> edges;
      for (size_t u = 0; u < n; ++u)
        for (size_t v = 0; v < n; ++v)
          {
            const bool keep = (u == v) ? has_loop(rng) : has_edge(rng);
            if (keep)
              edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
          }

      auto built = build_graph(n, edges);
      const Oracle oracle = exact_bounded_minimum(built, L);
      const auto r = most_negative_cycle_bounded(built.g, L);

      ASSERT_EQ(r.has_cycle, oracle.exists) << "trial=" << trial;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, r)) << "trial=" << trial;
      if (not r.has_cycle)
        continue;

      ++results;
      ASSERT_LE(r.length, L) << "trial=" << trial;
      // A simple cycle can never beat the exhaustive optimum.
      ASSERT_GE(r.total_cost, oracle.min_cost) << "trial=" << trial;
      if (r.is_exact)
        {
          ++exact_results;
          ASSERT_EQ(r.total_cost, oracle.min_cost) << "trial=" << trial;
        }
      else if (oracle.min_cost < 0)
        // An optimal negative walk decomposes into simple cycles at least one of
        // which is strictly negative, and every component is a candidate.
        ASSERT_LT(r.total_cost, 0) << "trial=" << trial;
    }
  EXPECT_GT(results, 100u);
  EXPECT_GT(exact_results * 10, results * 8);   // the optimum walk is usually simple
}


TEST(BoundedNegativeCycleTest, BoundThreeWithoutSelfLoopsIsAlwaysExact)
{
  std::mt19937_64 rng(0x3C1C1E5ULL);
  std::uniform_int_distribution<int> n_dist(2, 7);
  std::bernoulli_distribution has_edge(0.45);
  std::uniform_int_distribution<int> weight_dist(-9, 13);

  for (size_t trial = 0; trial < 150; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Edge_Def> edges;
      for (size_t u = 0; u < n; ++u)
        for (size_t v = 0; v < n; ++v)
          if (u != v and has_edge(rng))
            edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));

      auto built = build_graph(n, edges);
      const Oracle oracle = exact_bounded_minimum(built, 3);
      const auto r = most_negative_cycle_bounded(built.g, 3);

      ASSERT_EQ(r.has_cycle, oracle.exists) << "trial=" << trial;
      if (not r.has_cycle)
        continue;
      ASSERT_TRUE(r.is_exact) << "trial=" << trial;
      ASSERT_EQ(r.total_cost, oracle.min_cost) << "trial=" << trial;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, r)) << "trial=" << trial;
    }
}


// ---------------------------------------------------------------------------
// Cyclic core (reduction to strongly connected components) and traversal order
// ---------------------------------------------------------------------------

TEST(BoundedNegativeCycleTest, AcyclicGraphsHaveNoCycleWhateverTheBound)
{
  // Many very negative arcs, all pointing forward: no cycle at all.
  std::vector<Edge_Def> edges;
  for (size_t u = 0; u < 12; ++u)
    for (size_t v = u + 1; v < 12; ++v)
      if ((u + v) % 3 != 0)
        edges.emplace_back(u, v, -100 + static_cast<long long>(u * v));
  auto built = build_graph(12, edges);

  for (const size_t L : {1u, 2u, 5u, 12u, 40u})
    {
      const auto r = most_negative_cycle_bounded(built.g, L);
      EXPECT_FALSE(r.has_cycle) << "L=" << L;
      EXPECT_FALSE(r.is_negative()) << "L=" << L;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "L=" << L;
    }
}


TEST(BoundedNegativeCycleTest, OneLoopAmongManyIsolatedNodes)
{
  constexpr size_t n = 500;
  auto built = build_graph(n, {{250, 250, -3}});

  for (const size_t L : {1u, 2u, 7u, 500u, 509u})
    {
      const auto r = most_negative_cycle_bounded(built.g, L);
      ASSERT_TRUE(r.has_cycle) << "L=" << L;
      EXPECT_TRUE(r.is_exact) << "L=" << L;
      EXPECT_EQ(r.total_cost, -3) << "L=" << L;
      EXPECT_EQ(r.length, 1u) << "L=" << L;
      EXPECT_EQ(r.cycle_nodes.get_first(), built.nodes[250]) << "L=" << L;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "L=" << L;
    }
}


TEST(BoundedNegativeCycleTest, ArcsBetweenComponentsAreNeverPartOfACycle)
{
  // Triangle {0,1,2} of cost -1, triangle {3,4,5} of cost -2, and the arc
  // 2 -> 3 of cost -1000 joining them one way: it can close no cycle.
  auto built = build_graph(6, {{0, 1, 1}, {1, 2, 1}, {2, 0, -3},
                               {2, 3, -1000},
                               {3, 4, 1}, {4, 5, 1}, {5, 3, -4}});

  const auto r = most_negative_cycle_bounded(built.g, 6);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -2);
  EXPECT_EQ(r.length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
  for (auto it = r.cycle_arcs.get_it(); it.has_curr(); it.next_ne())
    EXPECT_NE(it.get_curr(), built.arcs[3]) << "the bridge is on no cycle";
}


TEST(BoundedNegativeCycleTest, SelfLoopsDownstreamOfAComponentStayCyclic)
{
  // {0,1} is a positive component, 1 -> 2 leaves it, node 2 has a negative
  // self-loop of its own and 2 -> 3 leads to a node without cycles.
  auto built = build_graph(4, {{0, 1, 2}, {1, 0, 2}, {1, 2, 1}, {2, 2, -4}, {2, 3, 1}});

  const auto r = most_negative_cycle_bounded(built.g, 3);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -4);
  EXPECT_EQ(r.length, 1u);
  EXPECT_EQ(r.cycle_nodes.get_first(), built.nodes[2]);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


TEST(BoundedNegativeCycleTest, OverflowOutsideTheCyclicCoreIsNotReported)
{
  // The chain 0 -> 1 -> 2 would overflow if it were accumulated, but it is
  // on no cycle: only the arcs of the cyclic core are ever added up. (An
  // overflow on a cycle does throw: see CostsAtTheLimitsOfTheType.)
  const long long big = std::numeric_limits<long long>::max();
  auto built = build_graph(5, {{0, 1, big}, {1, 2, big}, {3, 4, -1}, {4, 3, -1}});

  const auto r = most_negative_cycle_bounded(built.g, 5);
  ASSERT_TRUE(r.has_cycle);
  EXPECT_EQ(r.total_cost, -2);
  EXPECT_TRUE(witness_is_simple_cycle(built.g, r));
}


// Among equally cheap arcs into a node, the one with the lowest index in the
// snapshot wins, whether the step visits the arcs of the reached nodes only
// (many arcs elsewhere) or scans them all (few arcs).
TEST(BoundedNegativeCycleTest, EqualCostArcsKeepTheLowestIndexInEveryTraversal)
{
  for (const bool padded : {false, true})
    {
      // 0 -> 1 and 0 -> 2 are reached in step 1, in that order; 2 -> 3 is
      // inserted before 1 -> 3 and both cost the same, so 3 must be reached
      // through 2: the cycle is 0 -> 2 -> 3 -> 0.
      std::vector<Edge_Def> edges = {{0, 1, 1}, {0, 2, 1}, {2, 3, 1}, {1, 3, 1}, {3, 0, -5}};
      if (padded)   // a positive complete digraph that the origin 0 never reaches
        for (size_t u = 4; u < 12; ++u)
          for (size_t v = 4; v < 12; ++v)
            if (u != v)
              edges.emplace_back(u, v, 5);
      auto built = build_graph(padded ? 12 : 4, edges);

      const auto r = most_negative_cycle_bounded(built.g, 3);
      ASSERT_TRUE(r.has_cycle) << "padded=" << padded;
      EXPECT_EQ(r.total_cost, -3) << "padded=" << padded;
      ASSERT_EQ(r.length, 3u) << "padded=" << padded;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, r)) << "padded=" << padded;

      std::vector<int> ids;
      for (auto it = r.cycle_nodes.get_it(); it.has_curr(); it.next_ne())
        ids.push_back(it.get_curr()->get_info());
      EXPECT_EQ(ids, (std::vector<int>{0, 2, 3, 0})) << "padded=" << padded;
    }
}


TEST(BoundedNegativeCycleTest, EqualCostParallelArcsKeepTheFirstInserted)
{
  for (const bool padded : {false, true})
    {
      // Two parallel arcs 0 -> 1 of the same cost, inserted after 1 -> 0.
      std::vector<Edge_Def> edges = {{1, 0, -5}, {0, 1, 1}, {0, 1, 1}};
      if (padded)
        for (size_t u = 2; u < 10; ++u)
          for (size_t v = 2; v < 10; ++v)
            if (u != v)
              edges.emplace_back(u, v, 5);
      auto built = build_graph(padded ? 10 : 2, edges);

      const auto r = most_negative_cycle_bounded(built.g, 2);
      ASSERT_TRUE(r.has_cycle) << "padded=" << padded;
      EXPECT_EQ(r.total_cost, -4) << "padded=" << padded;
      ASSERT_EQ(r.length, 2u) << "padded=" << padded;
      EXPECT_TRUE(witness_is_simple_cycle(built.g, r)) << "padded=" << padded;

      bool uses_first = false;
      bool uses_second = false;
      for (auto it = r.cycle_arcs.get_it(); it.has_curr(); it.next_ne())
        {
          uses_first = uses_first or it.get_curr() == built.arcs[1];
          uses_second = uses_second or it.get_curr() == built.arcs[2];
        }
      EXPECT_TRUE(uses_first) << "padded=" << padded;
      EXPECT_FALSE(uses_second) << "padded=" << padded;
    }
}


TEST(BoundedNegativeCycleTest, RandomGraphsWithAcyclicFringesAgreeWithExhaustiveOracle)
{
  // A random core plus nodes that feed it, nodes it feeds, isolated nodes and
  // an occasional self-loop on a fringe node: the reduction to the cyclic
  // core must change nothing observable.
  std::mt19937_64 rng(0xC0DEC0DEULL);
  std::uniform_int_distribution<int> core_dist(2, 5);
  std::uniform_int_distribution<int> fringe_dist(0, 3);
  std::uniform_int_distribution<int> len_dist(1, 6);
  std::bernoulli_distribution has_edge(0.45);
  std::bernoulli_distribution has_loop(0.1);
  std::bernoulli_distribution coin(0.5);
  std::uniform_int_distribution<int> weight_dist(-9, 13);

  size_t results = 0;
  size_t exact_results = 0;
  for (size_t trial = 0; trial < 300; ++trial)
    {
      const size_t c = static_cast<size_t>(core_dist(rng));
      const size_t tin = static_cast<size_t>(fringe_dist(rng));
      const size_t tout = static_cast<size_t>(fringe_dist(rng));
      const size_t iso = static_cast<size_t>(fringe_dist(rng));
      const size_t n = c + tin + tout + iso;
      const size_t L = static_cast<size_t>(len_dist(rng));

      std::vector<Edge_Def> edges;
      auto add = [&](const size_t u, const size_t v)
      { edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng))); };

      for (size_t u = 0; u < c; ++u)
        for (size_t v = 0; v < c; ++v)
          if ((u == v) ? has_loop(rng) : has_edge(rng))
            add(u, v);
      for (size_t i = 0; i < tin; ++i)           // feeders: into the core, forward among themselves
        {
          for (size_t u = 0; u < c; ++u)
            if (coin(rng))
              add(c + i, u);
          if (i > 0 and coin(rng))
            add(c + i - 1, c + i);
        }
      for (size_t i = 0; i < tout; ++i)          // fed by the core, forward among themselves
        {
          for (size_t u = 0; u < c; ++u)
            if (coin(rng))
              add(u, c + tin + i);
          if (i > 0 and coin(rng))
            add(c + tin + i - 1, c + tin + i);
        }
      if (tin > 0 and tout > 0 and coin(rng))    // a bridge that skips the core
        add(c, c + tin);
      if (n > c and has_loop(rng))               // a cyclic component made of one node
        edges.emplace_back(c, c, static_cast<long long>(weight_dist(rng)));

      auto built = build_graph(n, edges);
      const Oracle oracle = exact_bounded_minimum(built, L);
      const auto r = most_negative_cycle_bounded(built.g, L);

      ASSERT_EQ(r.has_cycle, oracle.exists) << "trial=" << trial;
      ASSERT_TRUE(witness_is_simple_cycle(built.g, r)) << "trial=" << trial;
      if (not r.has_cycle)
        continue;

      ++results;
      ASSERT_LE(r.length, L) << "trial=" << trial;
      ASSERT_GE(r.total_cost, oracle.min_cost) << "trial=" << trial;
      if (r.is_exact)
        {
          ++exact_results;
          ASSERT_EQ(r.total_cost, oracle.min_cost) << "trial=" << trial;
        }
      else if (oracle.min_cost < 0)
        ASSERT_LT(r.total_cost, 0) << "trial=" << trial;
    }
  EXPECT_GT(results, 100u);
  EXPECT_GT(exact_results * 10, results * 7);
}


TEST(BoundedNegativeCycleTest, BoundIsCappedByTheNodeCountOfTheWholeGraph)
{
  // 0 <-> 1 with a negative self-loop on 1: a closed walk that goes around
  // the loop several times beats the simple loop, so the certificate needs
  // the walks of at least 3 arcs to have been examined. `L` is
  // min(max_length, V) with V the node count of the *whole* graph, isolated
  // nodes included, and not of the cyclic core: with V = 2 the bound is 2 and
  // the certificate holds; with 4 isolated nodes added it is 4 and it does not.
  const std::vector<Edge_Def> edges = {{0, 1, -1}, {1, 0, -1}, {1, 1, -5}};

  auto small = build_graph(2, edges);
  const auto r_small = most_negative_cycle_bounded(small.g, 4);
  ASSERT_TRUE(r_small.has_cycle);
  EXPECT_EQ(r_small.total_cost, -5);
  EXPECT_TRUE(r_small.is_exact);

  auto padded = build_graph(6, edges);
  const auto r_padded = most_negative_cycle_bounded(padded.g, 4);
  ASSERT_TRUE(r_padded.has_cycle);
  EXPECT_EQ(r_padded.total_cost, -5);   // the same cycle...
  EXPECT_EQ(r_padded.length, 1u);
  EXPECT_FALSE(r_padded.is_exact);      // ...but a cheaper walk exists in the larger bound
}
