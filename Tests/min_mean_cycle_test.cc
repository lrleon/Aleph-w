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
  const std::vector<Edge_Def> edges = {
      {0, 1, std::numeric_limits<long long>::max()},
      {1, 0, 1}
  };

  auto built = build_graph(2, edges);
  EXPECT_THROW((karp_minimum_mean_cycle(built.g)), std::overflow_error);
  EXPECT_THROW((karp_minimum_mean_cycle_value(built.g)), std::overflow_error);
}

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
