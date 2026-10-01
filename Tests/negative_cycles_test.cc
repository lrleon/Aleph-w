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

# include <functional>
# include <limits>
# include <random>
# include <set>
# include <tuple>
# include <vector>

# include <Bellman_Ford.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;
  using Float_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
  using Arr_Digraph = Array_Digraph<Graph_Anode<int>, Graph_Aarc<long long>>;
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  using Edge_Def = std::tuple<size_t, size_t, long long>;
  using Item = Negative_Cycle_Item<Graph, long long>;

  template <class GT>
  struct Built_Graph_T
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    std::vector<typename GT::Arc *> arcs;
  };

  template <class GT, typename Weight_Type>
  Built_Graph_T<GT>
  build_graph_generic(const size_t n,
                      const std::vector<std::tuple<size_t, size_t, Weight_Type>> & edges)
  {
    Built_Graph_T<GT> built;
    built.nodes.reserve(n);
    built.arcs.reserve(edges.size());

    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : edges)
      built.arcs.push_back(built.g.insert_arc(built.nodes[u], built.nodes[v], w));

    return built;
  }

  Built_Graph_T<Graph> build_graph(const size_t n, const std::vector<Edge_Def> & edges)
  {
    return build_graph_generic<Graph, long long>(n, edges);
  }

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
    for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
      {
        typename GT::Arc * arc = it.get_curr();
        if (g.get_src_node(arc) != curr or not seen.insert(curr).second)
          return false;
        sum += arc->get_info();
        curr = g.get_tgt_node(arc);
      }

    return curr == first and sum == item.total_cost and sum < Cost{0};
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

  struct Hide_Arc
  {
    Arc * blocked = nullptr;

    bool operator()(Arc * arc) const noexcept
    {
      return arc != blocked;
    }
  };
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
