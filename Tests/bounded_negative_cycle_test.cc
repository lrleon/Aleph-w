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

# include <functional>
# include <limits>
# include <random>
# include <set>
# include <tuple>
# include <vector>

# include <Min_Mean_Cycle.H>
# include <Negative_Cycles.H>
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
    for (auto arc_it = r.cycle_arcs.get_it(); arc_it.has_curr(); arc_it.next_ne())
      {
        typename GT::Arc * arc = arc_it.get_curr();
        if (not node_it.has_curr() or g.get_src_node(arc) != curr
            or g.get_tgt_node(arc) != node_it.get_curr() or not seen.insert(curr).second)
          return false;
        sum += arc->get_info();
        curr = node_it.get_curr();
        node_it.next_ne();
      }

    return curr == first and not node_it.has_curr() and sum == r.total_cost;
  }

  struct Oracle
  {
    bool exists = false;
    long long min_cost = 0;
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
              if (not oracle.exists or cost + w < oracle.min_cost)
                {
                  oracle.exists = true;
                  oracle.min_cost = cost + w;
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

  struct Hide_Arc
  {
    Arc * blocked = nullptr;

    bool operator()(Arc * arc) const noexcept
    {
      return arc != blocked;
    }
  };
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
  EXPECT_NEAR(fr.total_cost, -0.25, 1e-12);
  EXPECT_EQ(fr.length, 3u);
  EXPECT_TRUE(witness_is_simple_cycle(fbuilt.g, fr));

  auto abuilt = build_graph_generic<Arr_Digraph, long long>(
      3, {{0, 1, -4}, {1, 0, 1}, {1, 2, 3}, {2, 1, 3}});
  const auto ar = most_negative_cycle_bounded(abuilt.g, 4);
  ASSERT_TRUE(ar.has_cycle);
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
        ASSERT_LE(r.total_cost, 0) << "trial=" << trial;
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
