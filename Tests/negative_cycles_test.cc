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
 * @file negative_cycles_test.cc
 * @brief Tests for find_disjoint_negative_cycles() (Negative_Cycles.H).
 */

// Self-containment check: must stay the first include.
# include <Negative_Cycles.H>

# include <gtest/gtest.h>

# include <cmath>
# include <functional>
# include <limits>
# include <random>
# include <set>
# include <thread>
# include <tuple>
# include <type_traits>
# include <vector>

# include <Bellman_Ford.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>

# include "negative_cycles_test_support.H"

using namespace Aleph;
using namespace Negative_Cycles_Test_Support;

namespace
{
  using Item = Negative_Cycle_Item<Graph, long long>;

  // A reported item must be a closed path over consecutive graph arcs that
  // visits no node twice, whose arc weights sum to total_cost, negative.
  template <class GT, typename Cost>
  bool is_valid_negative_simple_cycle(const GT & g, const Negative_Cycle_Item<GT, Cost> & item)
  {
    if (item.cycle.is_empty() or not item.cycle.is_cycle() or not item.cycle.inside_graph(g))
      return false;

    const auto arcs = item.cycle.arcs();
    if (arcs.size() != item.length or item.length == 0)
      return false;

    std::set<typename GT::Node *> seen;
    typename GT::Node * first = item.cycle.get_first_node();
    typename GT::Node * curr = first;
    Cost sum = Cost{0};
    long double abs_sum = 0.0L;
    for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
      {
        typename GT::Arc * arc = it.get_curr();
        if (g.get_src_node(arc) != curr or not seen.insert(curr).second)
          return false;
        sum += arc->get_info();
        abs_sum += std::fabs(static_cast<long double>(arc->get_info()));
        curr = g.get_tgt_node(arc);
      }

    if (curr != first)
      return false;
    if constexpr (std::is_floating_point_v<Cost>)
      // total_cost is a compensated sum: it may differ from the plain sum above by
      // the rounding error of plain summation, at most (terms + 1) * eps * sum of |w|
      return std::fabs(static_cast<long double>(sum) - static_cast<long double>(item.total_cost))
             <= (item.length + 1) * static_cast<long double>(std::numeric_limits<Cost>::epsilon()) * abs_sum
             and item.total_cost < Cost{0};
    else
      return sum == item.total_cost and sum < Cost{0};
  }

  template <class GT, typename Cost>
  std::set<typename GT::Arc *> arc_set(const Negative_Cycle_Item<GT, Cost> & item)
  {
    std::set<typename GT::Arc *> s;
    item.cycle.for_each_arc([&s](typename GT::Arc * a) { s.insert(a); });
    return s;
  }

  template <class GT, typename Cost>
  bool arc_sets_are_pairwise_distinct(const DynList<Negative_Cycle_Item<GT, Cost>> & items,
                                      const bool require_disjoint)
  {
    std::vector<std::set<typename GT::Arc *>> sets;
    for (auto it = items.get_it(); it.has_curr(); it.next_ne())
      sets.push_back(arc_set(it.get_curr()));

    for (size_t i = 0; i < sets.size(); ++i)
      for (size_t j = i + 1; j < sets.size(); ++j)
        {
          if (sets[i] == sets[j])
            return false;
          if (require_disjoint)
            for (auto * a : sets[i])
              if (sets[j].count(a) != 0)
                return false;
        }
    return true;
  }

  // Exhaustive oracle: does the graph have a simple cycle of negative total
  // weight among the arcs accepted by `allow`?
  bool has_negative_simple_cycle(const Built_Graph_T<Graph> & built,
                                 const std::function<bool(Arc *)> & allow)
  {
    const size_t n = built.nodes.size();
    std::vector<bool> visited(n, false);
    bool found = false;

    std::function<void(size_t, size_t, long long)> dfs =
      [&](const size_t start, const size_t u, const long long cost)
    {
      if (found)
        return;
      for (Node_Arc_Iterator<Graph> it(built.nodes[u]); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_current_arc_ne();
          if (not allow(arc))
            continue;
          const size_t v = static_cast<size_t>(it.get_tgt_node()->get_info());
          const long long w = arc->get_info();
          if (v == start)
            {
              if (cost + w < 0)
                found = true;
              continue;
            }
          if (v < start or visited[v])   // canonical start: the minimum node of the cycle
            continue;
          visited[v] = true;
          dfs(start, v, cost + w);
          visited[v] = false;
        }
    };

    for (size_t s = 0; s < n and not found; ++s)
      {
        visited[s] = true;
        dfs(s, s, 0);
        visited[s] = false;
      }
    return found;
  }


  // The same triangle of total cost -1 for every cost type.
  template <typename W>
  void expect_triangle_found_with_cost_type()
  {
    using G = List_Digraph<Graph_Node<int>, Graph_Arc<W>>;
    auto built = build_graph_generic<G, W>(
        3, {{0, 1, W(1)}, {1, 2, W(-3)}, {2, 0, W(1)}});

    const auto cycles = find_disjoint_negative_cycles(built.g, 5);
    ASSERT_EQ(cycles.size(), 1u);
    EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, cycles.get_first()));
    EXPECT_EQ(cycles.get_first().total_cost, W(-1));
    EXPECT_EQ(cycles.get_first().length, 3u);
  }
} // namespace


TEST(NegativeCyclesTest, TwoArcDisjointNegativeCyclesAreBothFound)
{
  // 0 -> 1 -> 2 -> 0 costs -1 (3 arcs); 3 -> 4 -> 3 costs -1 (2 arcs).
  const std::vector<Edge_Def> edges = {
      {0, 1, 1}, {1, 2, -3}, {2, 0, 1},
      {2, 3, 5},
      {3, 4, -2}, {4, 3, 1}
  };
  auto built = build_graph(5, edges);

  for (const auto policy : {Negative_Cycle_Exclusion::Min_Weight_Arc,
                            Negative_Cycle_Exclusion::All_Arcs})
    {
      const auto cycles = find_disjoint_negative_cycles(built.g, 10, policy);
      ASSERT_EQ(cycles.size(), 2u);

      std::multiset<size_t> lengths;
      for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
        {
          const Item & item = it.get_curr();
          EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, item));
          EXPECT_EQ(item.total_cost, -1);
          lengths.insert(item.length);
        }
      EXPECT_EQ(lengths, (std::multiset<size_t>{2, 3}));
      EXPECT_TRUE(arc_sets_are_pairwise_distinct(cycles, true));
    }
}


TEST(NegativeCyclesTest, SingleNegativeCycleIsReportedOnce)
{
  const std::vector<Edge_Def> edges = {
      {0, 1, 2}, {1, 2, -5}, {2, 0, 1},   // -2
      {2, 3, 1}, {3, 0, 4}                // 0 -> 1 -> 2 -> 3 -> 0 costs 2
  };
  auto built = build_graph(4, edges);

  const auto cycles = find_disjoint_negative_cycles(built.g, 5);
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, cycles.get_first()));
  EXPECT_EQ(cycles.get_first().total_cost, -2);
  EXPECT_EQ(cycles.get_first().length, 3u);
}


TEST(NegativeCyclesTest, NoNegativeCycleReturnsEmptyList)
{
  const std::vector<Edge_Def> positive = {
      {0, 1, 1}, {1, 2, 1}, {2, 0, 1}, {1, 0, 3}
  };
  auto cyc = build_graph(3, positive);
  EXPECT_TRUE(find_disjoint_negative_cycles(cyc.g, 5).is_empty());

  const std::vector<Edge_Def> dag = {
      {0, 1, -4}, {1, 2, -4}, {0, 2, -9}
  };
  auto acyclic = build_graph(3, dag);
  EXPECT_TRUE(find_disjoint_negative_cycles(acyclic.g, 5).is_empty());

  Graph empty;
  EXPECT_TRUE(find_disjoint_negative_cycles(empty, 5).is_empty());
}


TEST(NegativeCyclesTest, MaxCyclesZeroReturnsEmptyList)
{
  const std::vector<Edge_Def> edges = {{0, 1, -1}, {1, 0, -1}};
  auto built = build_graph(2, edges);

  EXPECT_TRUE(find_disjoint_negative_cycles(built.g, 0).is_empty());
  EXPECT_EQ(find_disjoint_negative_cycles(built.g, 1).size(), 1u);
}


// max_cycles == 0 must not snapshot nor validate the graph: no exception for
// an undirected graph and no call to the distance accessor.
TEST(NegativeCyclesTest, MaxCyclesZeroDoesNotTouchTheGraph)
{
  UGraph undirected;
  auto * u0 = undirected.insert_node(0);
  auto * u1 = undirected.insert_node(1);
  undirected.insert_arc(u0, u1, -1);
  EXPECT_NO_THROW({ EXPECT_TRUE(find_disjoint_negative_cycles(undirected, 0).is_empty()); });

  struct Counting_Dist
  {
    using Distance_Type = long long;
    int * calls;
    Distance_Type operator()(Arc * a) const { ++*calls; return a->get_info(); }
  };
  auto built = build_graph(2, {{0, 1, -1}, {1, 0, -1}});
  int calls = 0;
  EXPECT_TRUE((find_disjoint_negative_cycles<Graph, Counting_Dist>(
      built.g, 0, Negative_Cycle_Exclusion::Min_Weight_Arc, Counting_Dist{&calls})).is_empty());
  EXPECT_EQ(calls, 0);
}


TEST(NegativeCyclesTest, MaxCyclesLimitsTheCount)
{
  // Three arc-disjoint negative 2-cycles.
  const std::vector<Edge_Def> edges = {
      {0, 1, -1}, {1, 0, 0},
      {2, 3, -1}, {3, 2, 0},
      {4, 5, -1}, {5, 4, 0},
      {1, 2, 7}, {3, 4, 7}
  };
  auto built = build_graph(6, edges);

  EXPECT_EQ(find_disjoint_negative_cycles(built.g, 2).size(), 2u);
  EXPECT_EQ(find_disjoint_negative_cycles(built.g, 3).size(), 3u);
  EXPECT_EQ(find_disjoint_negative_cycles(built.g, 100).size(), 3u);
}


TEST(NegativeCyclesTest, CyclesSharingANodeButNoArcAreBothFound)
{
  // Two 2-cycles hanging from node 0.
  const std::vector<Edge_Def> edges = {
      {0, 1, -2}, {1, 0, 1},   // -1
      {0, 2, -3}, {2, 0, 1}    // -2
  };
  auto built = build_graph(3, edges);

  for (const auto policy : {Negative_Cycle_Exclusion::Min_Weight_Arc,
                            Negative_Cycle_Exclusion::All_Arcs})
    {
      const auto cycles = find_disjoint_negative_cycles(built.g, 10, policy);
      ASSERT_EQ(cycles.size(), 2u);
      for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
        EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, it.get_curr()));
      EXPECT_TRUE(arc_sets_are_pairwise_distinct(cycles, true));
    }
}


TEST(NegativeCyclesTest, ExclusionPolicyDecidesWhatCyclesSharingAnArcYield)
{
  // A: 0 -> 1 -> 2 -> 0 costs -3; B: 0 -> 1 -> 3 -> 0 costs -2. They share
  // arc 0 -> 1, which is the minimum-weight arc of neither cycle.
  const std::vector<Edge_Def> edges = {
      {0, 1, 1},
      {1, 2, -5}, {2, 0, 1},
      {1, 3, -2}, {3, 0, -1}
  };
  auto built = build_graph(4, edges);

  const auto min_arc = find_disjoint_negative_cycles(
      built.g, 10, Negative_Cycle_Exclusion::Min_Weight_Arc);
  ASSERT_EQ(min_arc.size(), 2u);
  for (auto it = min_arc.get_it(); it.has_curr(); it.next_ne())
    EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, it.get_curr()));
  EXPECT_TRUE(arc_sets_are_pairwise_distinct(min_arc, false));

  const auto all_arcs = find_disjoint_negative_cycles(
      built.g, 10, Negative_Cycle_Exclusion::All_Arcs);
  ASSERT_EQ(all_arcs.size(), 1u);
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, all_arcs.get_first()));
}


TEST(NegativeCyclesTest, ParallelArcsAreToldApart)
{
  // Two arcs 0 -> 1 (weights 5 and -4) and one arc 1 -> 0 (weight 1): the
  // only negative cycle uses the -4 arc, which must be the one reported.
  auto built = build_graph(2, {{0, 1, 5}, {0, 1, -4}, {1, 0, 1}});

  const auto cycles = find_disjoint_negative_cycles(built.g, 5);
  ASSERT_EQ(cycles.size(), 1u);
  const Item & item = cycles.get_first();
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, item));
  EXPECT_EQ(item.total_cost, -3);
  EXPECT_TRUE(item.cycle.contains_arc(built.arcs[1]));
  EXPECT_FALSE(item.cycle.contains_arc(built.arcs[0]));
}


TEST(NegativeCyclesTest, ArcFilterHidesArcs)
{
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}});

  ASSERT_EQ(find_disjoint_negative_cycles(built.g, 5).size(), 1u);

  const Hide_Arc filter{built.arcs[1]};
  const auto hidden = find_disjoint_negative_cycles<Graph, Dft_Dist<Graph>, Hide_Arc>(
      built.g, 5, Negative_Cycle_Exclusion::Min_Weight_Arc, Dft_Dist<Graph>(), filter);
  EXPECT_TRUE(hidden.is_empty());
}


TEST(NegativeCyclesTest, FloatingWeightsWork)
{
  auto built = build_graph_generic<Float_Graph, double>(
      3, {{0, 1, -0.75}, {1, 2, 0.25}, {2, 0, 0.25}, {2, 1, 0.5}});

  const auto cycles = find_disjoint_negative_cycles(built.g, 5);
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, cycles.get_first()));
  EXPECT_NEAR(cycles.get_first().total_cost, -0.25, 1e-12);
}


TEST(NegativeCyclesTest, ArrayDigraphBackendWorks)
{
  auto built = build_graph_generic<Arr_Digraph, long long>(
      3, {{0, 1, -4}, {1, 0, 1}, {1, 2, 3}, {2, 1, 3}});

  const auto cycles = find_disjoint_negative_cycles(built.g, 5);
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, cycles.get_first()));
  EXPECT_EQ(cycles.get_first().total_cost, -3);
}


TEST(NegativeCyclesTest, InvalidInputsThrow)
{
  UGraph undirected;
  auto * u0 = undirected.insert_node(0);
  auto * u1 = undirected.insert_node(1);
  undirected.insert_arc(u0, u1, -1);
  EXPECT_THROW((find_disjoint_negative_cycles(undirected, 1)), std::domain_error);

  Float_Graph inf_graph;
  auto * i0 = inf_graph.insert_node(0);
  auto * i1 = inf_graph.insert_node(1);
  inf_graph.insert_arc(i0, i1, -std::numeric_limits<double>::infinity());
  inf_graph.insert_arc(i1, i0, 1.0);
  EXPECT_THROW((find_disjoint_negative_cycles(inf_graph, 1)), std::domain_error);

  Float_Graph nan_graph;
  auto * n0 = nan_graph.insert_node(0);
  auto * n1 = nan_graph.insert_node(1);
  nan_graph.insert_arc(n0, n1, std::numeric_limits<double>::quiet_NaN());
  nan_graph.insert_arc(n1, n0, 1.0);
  EXPECT_THROW((find_disjoint_negative_cycles(nan_graph, 1)), std::domain_error);

  // dist[1] becomes -5, then -5 + min overflows.
  auto overflow = build_graph(2, {{0, 1, -5}, {1, 0, std::numeric_limits<long long>::min()}});
  EXPECT_THROW((find_disjoint_negative_cycles(overflow.g, 1)), std::overflow_error);
}


TEST(NegativeCyclesTest, GraphIsLeftUntouched)
{
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}});
  const size_t nodes = built.g.get_num_nodes();
  const size_t arcs = built.g.get_num_arcs();

  const Graph & cg = built.g;   // the function takes a const graph
  const auto cycles = find_disjoint_negative_cycles(cg, 5);
  ASSERT_EQ(cycles.size(), 1u);

  EXPECT_EQ(built.g.get_num_nodes(), nodes);
  EXPECT_EQ(built.g.get_num_arcs(), arcs);
  for (auto * a : built.arcs)
    EXPECT_EQ(ARC_COOKIE(a), nullptr);
  for (auto * p : built.nodes)
    EXPECT_EQ(NODE_COOKIE(p), nullptr);
}


// ---------------------------------------------------------------------------
// Known numeric limitation (F2 of auditoria-rama-arbitrage-performance-bugs.md).
//
// The loop 1 -> 1 of weight -1 is exactly representable and is a negative
// cycle, but the incoming arc of weight -1e16 puts dist[1] at -1e16, where
// adding -1 is absorbed: dist[1] never improves, the loop never enters the
// predecessor graph and no cycle is extracted. most_negative_cycle_bounded()
// keeps per-layer costs and does find the loop. This test pins today's
// behaviour on purpose: the numeric policy of Stage 2 must flip the first
// expectation, and then this comment with it.
// ---------------------------------------------------------------------------
TEST(NegativeCyclesTest, KnownNumericLimitationAbsorptionHidesNegativeLoop)
{
  // Same arcs in both insertion orders: the outcome does not depend on it.
  for (const bool incoming_first : {true, false})
    {
      Float_Graph g;
      auto * s = g.insert_node(0);
      auto * v = g.insert_node(1);
      if (incoming_first)
        {
          g.insert_arc(s, v, -1e16);
          g.insert_arc(v, v, -1.0);
        }
      else
        {
          g.insert_arc(v, v, -1.0);
          g.insert_arc(s, v, -1e16);
        }

      const auto cycles = find_disjoint_negative_cycles(g, 1);
      EXPECT_TRUE(cycles.is_empty()) << "incoming_first=" << incoming_first;   // known limitation

      const auto bounded = most_negative_cycle_bounded(g, 1);
      ASSERT_TRUE(bounded.has_cycle) << "incoming_first=" << incoming_first;
      EXPECT_EQ(bounded.total_cost, -1.0);
      EXPECT_EQ(bounded.length, 1u);
    }
}


// The same absorption with the huge arc inside a strongly connected component:
// 0 -> 1 of -1e16, 1 -> 0 of +1e16 and the loop of -1 on node 1. Processing the
// strongly connected components separately would not help here (and the
// enumeration does not do it: see the file documentation), so the limitation is
// pinned for this shape too.
TEST(NegativeCyclesTest, KnownNumericLimitationAbsorptionInsideAComponent)
{
  Float_Graph g;
  auto * s = g.insert_node(0);
  auto * v = g.insert_node(1);
  g.insert_arc(s, v, -1e16);
  g.insert_arc(v, s, 1e16);
  g.insert_arc(v, v, -1.0);

  EXPECT_TRUE(find_disjoint_negative_cycles(g, 1).is_empty());   // known limitation

  const auto bounded = most_negative_cycle_bounded(g, 1);
  ASSERT_TRUE(bounded.has_cycle);
  EXPECT_EQ(bounded.total_cost, -1.0);
  EXPECT_EQ(bounded.length, 1u);
}


// ---------------------------------------------------------------------------
// F1 of auditoria-rama-arbitrage-performance-bugs.md for the enumeration.
//
// Each ring below has an exact total of zero (checked with integers) but its
// plain double sum rounds to -1, because the huge weights swallow the small
// ones, and the predecessor graph does contain it. It used to be reported as
// a negative cycle of cost -1. The reported cost is now recomputed with
// compensated summation, so no cycle is reported.
// ---------------------------------------------------------------------------
TEST(NegativeCyclesTest, CancellationDoesNotFakeANegativeCycle)
{
  const std::vector<std::vector<double>> rings = {
    {-7.0, -1e16, 1e16, 7.0},
    {-1e16, -7.0, 1e16, 7.0},
    {-1e16, -3.0, 1e16, 3.0},
    {-3.0, -1e16, 5e15, 3.0, 5e15},
    {-1e16, 4.0, -7.0, 1e16, 3.0}
  };

  for (const auto & weights : rings)
    {
      long long exact_sum = 0;   // independent oracle: integer arithmetic, exact here
      for (const double w : weights)
        exact_sum += static_cast<long long>(w);
      ASSERT_EQ(exact_sum, 0);

      Float_Graph g;
      std::vector<Float_Graph::Node *> nodes;
      for (size_t i = 0; i < weights.size(); ++i)
        nodes.push_back(g.insert_node(static_cast<int>(i)));
      for (size_t i = 0; i < weights.size(); ++i)
        g.insert_arc(nodes[i], nodes[(i + 1) % weights.size()], weights[i]);

      for (const auto policy : {Negative_Cycle_Exclusion::Min_Weight_Arc,
                                Negative_Cycle_Exclusion::All_Arcs})
        EXPECT_TRUE(find_disjoint_negative_cycles(g, 3, policy).is_empty());
    }
}


TEST(NegativeCyclesTest, DistanceAndFilterAreCalledOncePerArc)
{
  // 5 arcs, one of them hidden by the filter: the filter is asked about all
  // of them exactly once and the distance accessor only about accepted ones.
  auto built = build_graph(3, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {0, 2, 7}, {2, 2, 4}});
  int dist_calls = 0;
  int filter_calls = 0;

  const auto cycles = find_disjoint_negative_cycles<Graph, Counting_Dist, Counting_Filter>(
      built.g, 4, Negative_Cycle_Exclusion::All_Arcs, Counting_Dist{&dist_calls},
      Counting_Filter{&filter_calls, built.arcs[3]});

  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_EQ(filter_calls, 5);
  EXPECT_EQ(dist_calls, 4);
}


TEST(NegativeCyclesTest, NonFiniteWeightsHiddenByTheFilterAreIgnored)
{
  Float_Graph g;
  auto * a = g.insert_node(0);
  auto * b = g.insert_node(1);
  auto * c = g.insert_node(2);
  g.insert_arc(a, b, -2.0);
  g.insert_arc(b, a, 1.0);                                          // cycle of cost -1
  g.insert_arc(b, c, std::numeric_limits<double>::infinity());
  g.insert_arc(c, a, std::numeric_limits<double>::quiet_NaN());

  // Without the filter the non-finite weights are rejected...
  EXPECT_THROW((find_disjoint_negative_cycles(g, 5)), std::domain_error);

  // ...and with a filter that hides exactly those arcs they are never read.
  const auto cycles = find_disjoint_negative_cycles<Float_Graph, Dft_Dist<Float_Graph>, Finite_Arcs>(
      g, 5, Negative_Cycle_Exclusion::Min_Weight_Arc, Dft_Dist<Float_Graph>(), Finite_Arcs());
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_NEAR(cycles.get_first().total_cost, -1.0, 1e-12);
}


TEST(NegativeCyclesTest, NonNullCookiesAndBitsAreLeftUntouched)
{
  auto built = build_graph(4, {{0, 1, 1}, {1, 2, -3}, {2, 0, 1}, {2, 3, -2}, {3, 2, 1}});
  Planted_State<Graph> planted;
  planted.plant(built);
  ASSERT_TRUE(planted.intact(built));

  for (const auto policy : {Negative_Cycle_Exclusion::Min_Weight_Arc,
                            Negative_Cycle_Exclusion::All_Arcs})
    {
      const auto cycles = find_disjoint_negative_cycles(built.g, 5, policy);
      EXPECT_EQ(cycles.size(), 2u);
      EXPECT_TRUE(planted.intact(built));
    }
}


TEST(NegativeCyclesTest, SingleNodeSelfLoops)
{
  auto negative = build_graph(1, {{0, 0, -3}});
  const auto found = find_disjoint_negative_cycles(negative.g, 5);
  ASSERT_EQ(found.size(), 1u);
  EXPECT_TRUE(is_valid_negative_simple_cycle(negative.g, found.get_first()));
  EXPECT_EQ(found.get_first().total_cost, -3);
  EXPECT_EQ(found.get_first().length, 1u);

  for (const long long w : {0LL, 3LL})
    {
      auto not_negative = build_graph(1, {{0, 0, w}});
      EXPECT_TRUE(find_disjoint_negative_cycles(not_negative.g, 5).is_empty()) << "w=" << w;
    }

  Graph lonely;
  lonely.insert_node(0);
  EXPECT_TRUE(find_disjoint_negative_cycles(lonely, 5).is_empty());
}


TEST(NegativeCyclesTest, DisconnectedGraphsAreSearchedInEveryComponent)
{
  // 0 <-> 1 positive, 2 isolated, 3 <-> 4 negative, 5 -> 6 acyclic.
  auto built = build_graph(7, {{0, 1, 2}, {1, 0, 2}, {3, 4, -2}, {4, 3, 1}, {5, 6, -9}});

  const auto cycles = find_disjoint_negative_cycles(built.g, 5);
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, cycles.get_first()));
  EXPECT_EQ(cycles.get_first().total_cost, -1);
  EXPECT_TRUE(cycles.get_first().cycle.contains_node(built.nodes[3]));
  EXPECT_TRUE(cycles.get_first().cycle.contains_node(built.nodes[4]));
}


TEST(NegativeCyclesTest, IntCostsWork)
{
  expect_triangle_found_with_cost_type<int>();
}


TEST(NegativeCyclesTest, LongDoubleCostsWork)
{
  expect_triangle_found_with_cost_type<long double>();
}


TEST(NegativeCyclesTest, CostsAtTheLimitsOfTheType)
{
  using IntGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using LdGraph = List_Digraph<Graph_Node<int>, Graph_Arc<long double>>;
  const int int_min = std::numeric_limits<int>::min();
  const int int_max = std::numeric_limits<int>::max();
  const long double ld_lowest = std::numeric_limits<long double>::lowest();
  const long double ld_max = std::numeric_limits<long double>::max();

  // A loop of the lowest value is a legitimate negative cycle...
  auto int_loop = build_graph_generic<IntGraph, int>(1, {{0, 0, int_min}});
  const auto int_found = find_disjoint_negative_cycles(int_loop.g, 1);
  ASSERT_EQ(int_found.size(), 1u);
  EXPECT_EQ(int_found.get_first().total_cost, int_min);

  auto ld_loop = build_graph_generic<LdGraph, long double>(1, {{0, 0, ld_lowest}});
  const auto ld_found = find_disjoint_negative_cycles(ld_loop.g, 1);
  ASSERT_EQ(ld_found.size(), 1u);
  EXPECT_EQ(ld_found.get_first().total_cost, ld_lowest);

  // ...the largest value is simply not negative...
  auto int_pos = build_graph_generic<IntGraph, int>(1, {{0, 0, int_max}});
  EXPECT_TRUE(find_disjoint_negative_cycles(int_pos.g, 1).is_empty());
  auto ld_pos = build_graph_generic<LdGraph, long double>(1, {{0, 0, ld_max}});
  EXPECT_TRUE(find_disjoint_negative_cycles(ld_pos.g, 1).is_empty());

  // ...and sums that leave the type throw: overflow for integers, a
  // non-finite accumulation for floating point.
  auto int_two = build_graph_generic<IntGraph, int>(2, {{0, 1, int_min}, {1, 0, int_min}});
  EXPECT_THROW((find_disjoint_negative_cycles(int_two.g, 1)), std::overflow_error);
  auto ld_two = build_graph_generic<LdGraph, long double>(2, {{0, 1, ld_lowest}, {1, 0, ld_lowest}});
  EXPECT_THROW((find_disjoint_negative_cycles(ld_two.g, 1)), std::domain_error);
}


TEST(NegativeCyclesTest, RandomGraphsAgreeWithExhaustiveOracle)
{
  std::mt19937_64 rng(0x5EED0C1CULL);
  std::uniform_int_distribution<int> n_dist(2, 7);
  std::bernoulli_distribution has_edge(0.4);
  std::bernoulli_distribution has_loop(0.1);
  std::uniform_int_distribution<int> weight_dist(-9, 13);

  size_t graphs_with_cycles = 0;
  for (size_t trial = 0; trial < 150; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Edge_Def> edges;
      for (size_t u = 0; u < n; ++u)
        for (size_t v = 0; v < n; ++v)
          {
            const bool keep = (u == v) ? has_loop(rng) : has_edge(rng);
            if (keep)
              edges.emplace_back(u, v, static_cast<long long>(weight_dist(rng)));
          }

      auto built = build_graph(n, edges);
      const bool expected = has_negative_simple_cycle(built, [](Arc *) { return true; });
      ASSERT_EQ(Bellman_Ford<Graph>(built.g).has_negative_cycle(), expected) << "trial=" << trial;
      graphs_with_cycles += expected;

      for (const auto policy : {Negative_Cycle_Exclusion::Min_Weight_Arc,
                                Negative_Cycle_Exclusion::All_Arcs})
        {
          const auto cycles = find_disjoint_negative_cycles(built.g, 10, policy);
          ASSERT_EQ(not cycles.is_empty(), expected) << "trial=" << trial;
          ASSERT_LE(cycles.size(), 10u);
          for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
            ASSERT_TRUE(is_valid_negative_simple_cycle(built.g, it.get_curr())) << "trial=" << trial;
          ASSERT_TRUE(arc_sets_are_pairwise_distinct(
              cycles, policy == Negative_Cycle_Exclusion::All_Arcs)) << "trial=" << trial;

          // After excluding every reported arc there is no negative cycle left
          // among the rest, otherwise the search stopped too early.
          if (policy == Negative_Cycle_Exclusion::All_Arcs and cycles.size() < 10)
            {
              std::set<Arc *> used;
              for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
                for (auto * a : arc_set(it.get_curr()))
                  used.insert(a);
              ASSERT_FALSE(has_negative_simple_cycle(
                  built, [&used](Arc * a) { return used.count(a) == 0; })) << "trial=" << trial;
            }
        }
    }
  EXPECT_GT(graphs_with_cycles, 20u);   // the sample exercises both outcomes
}


// ---------------------------------------------------------------------------
// Node index of the snapshot (open addressing over node pointers).
// ---------------------------------------------------------------------------

TEST(NegativeCyclesTest, LargeGraphsAreIndexedCorrectly)
{
  // A ring of many nodes whose total is -1 and a loop among a crowd of
  // isolated nodes: every node must be found by its pointer.
  constexpr size_t ring = 5000;
  std::vector<Edge_Def> edges;
  for (size_t i = 0; i < ring; ++i)
    edges.emplace_back(i, (i + 1) % ring, i == 0 ? -static_cast<long long>(ring) : 1);
  auto built = build_graph(ring, edges);

  const auto cycles = find_disjoint_negative_cycles(built.g, 3);
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_EQ(cycles.get_first().length, ring);
  EXPECT_EQ(cycles.get_first().total_cost, -1);
  EXPECT_TRUE(is_valid_negative_simple_cycle(built.g, cycles.get_first()));

  constexpr size_t crowd = 20000;
  auto sparse = build_graph(crowd, {{12345, 12345, -2}});
  const auto loops = find_disjoint_negative_cycles(sparse.g, 3);
  ASSERT_EQ(loops.size(), 1u);
  EXPECT_EQ(loops.get_first().length, 1u);
  EXPECT_EQ(loops.get_first().cycle.get_first_node(), sparse.nodes[12345]);
}


TEST(NegativeCyclesTest, GraphsWithoutNodesOrWithOneNodeAreIndexed)
{
  Graph empty;
  EXPECT_TRUE(find_disjoint_negative_cycles(empty, 3).is_empty());

  auto one = build_graph(1, {{0, 0, -1}});
  const auto cycles = find_disjoint_negative_cycles(one.g, 3);
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_EQ(cycles.get_first().total_cost, -1);
}


// The index of the snapshot answers with the position of every node it was
// given, and refuses what it was not given: a null pointer must not match a
// free slot, and an unknown node must not loop or answer with a position.
TEST(NegativeCyclesTest, NodeIndexFindsEveryNodeAndRejectsTheRest)
{
  auto built = build_graph(300, {});
  auto stranger = build_graph(1, {});

  negative_cycles_detail::Node_Index<Node> index(built.nodes.size());
  for (size_t i = 0; i < built.nodes.size(); ++i)
    index.insert(built.nodes[i], 1000 + i);

  for (size_t i = 0; i < built.nodes.size(); ++i)
    EXPECT_EQ(index.find(built.nodes[i]), 1000 + i);

  EXPECT_THROW((void) index.find(nullptr), std::domain_error);
  EXPECT_THROW((void) index.find(stranger.nodes[0]), std::domain_error);
  EXPECT_THROW(index.insert(nullptr, 0), std::domain_error);

  // A table that is exactly as full as it will ever be still answers.
  negative_cycles_detail::Node_Index<Node> single(1);
  single.insert(built.nodes[7], 42);
  EXPECT_EQ(single.find(built.nodes[7]), 42u);
  EXPECT_THROW((void) single.find(built.nodes[8]), std::domain_error);
}


// The file documentation promises that several threads may search the same
// graph while nobody modifies it: the functions take a `const GT &` and keep
// all their state local. Every thread must get exactly the single-threaded
// answer.
TEST(NegativeCyclesTest, ConcurrentSearchesOnTheSameGraphAgree)
{
  // Two negative triangles and some positive clutter, with ties.
  std::vector<Edge_Def> edges = {{0, 1, 2}, {1, 2, -5}, {2, 0, 2},
                                 {3, 4, 1}, {4, 5, -4}, {5, 3, 2},
                                 {2, 3, 1}, {5, 0, 1}, {1, 4, 3}, {4, 1, 3}};
  for (size_t u = 6; u < 40; ++u)
    {
      edges.emplace_back(u, (u + 1) % 40, 1);
      edges.emplace_back(u, (u * 7 + 3) % 40, 2);
    }
  auto built = build_graph(40, edges);

  const auto cycles_ref = find_disjoint_negative_cycles(built.g, 5);
  const auto bounded_ref = most_negative_cycle_bounded(built.g, 6);
  ASSERT_GE(cycles_ref.size(), 2u);
  ASSERT_TRUE(bounded_ref.has_cycle);

  auto same_cycles = [&](const decltype(cycles_ref) & a)
  {
    if (a.size() != cycles_ref.size())
      return false;
    auto ia = a.get_it();
    auto ir = cycles_ref.get_it();
    for (; ia.has_curr(); ia.next_ne(), ir.next_ne())
      if (ia.get_curr().total_cost != ir.get_curr().total_cost
          or ia.get_curr().cycle.arcs() != ir.get_curr().cycle.arcs())
        return false;
    return true;
  };

  constexpr size_t threads = 8;
  constexpr size_t rounds = 25;
  std::vector<int> failures(threads, 0);
  std::vector<std::thread> pool;
  for (size_t t = 0; t < threads; ++t)
    pool.emplace_back([&, t]
    {
      for (size_t i = 0; i < rounds; ++i)
        {
          const auto cycles = find_disjoint_negative_cycles(built.g, 5);
          const auto bounded = most_negative_cycle_bounded(built.g, 6);
          if (not same_cycles(cycles) or bounded.total_cost != bounded_ref.total_cost
              or bounded.length != bounded_ref.length
              or bounded.is_exact != bounded_ref.is_exact
              or bounded.cycle_arcs != bounded_ref.cycle_arcs)
            ++failures[t];
        }
    });
  for (auto & th : pool)
    th.join();

  for (size_t t = 0; t < threads; ++t)
    EXPECT_EQ(failures[t], 0) << "thread " << t;
}
