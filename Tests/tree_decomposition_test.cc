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
 * @file tree_decomposition_test.cc
 * @brief Strong tests for Heavy-Light and Centroid decomposition on Aleph graphs.
 */

# include <gtest/gtest.h>

# include <Tree_Decomposition.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>

# include <cstddef>
# include <limits>
# include <random>
# include <utility>
# include <vector>

using namespace Aleph;

namespace
{
  template <class GT>
  Array<typename GT::Node *>
  build_graph_with_values(GT & g,
                          const std::vector<int> & values,
                          const std::vector<std::pair<size_t, size_t>> & edges)
  {
    using Node = typename GT::Node;

    auto nodes = Array<Node *>::create(values.size());
    for (size_t i = 0; i < values.size(); ++i)
      nodes(i) = g.insert_node(values[i]);

    for (const auto & [u, v] : edges)
      g.insert_arc(nodes(u), nodes(v), 1);

    return nodes;
  }

  std::vector<std::pair<size_t, size_t>>
  random_tree_edges(const size_t n, std::mt19937 & rng)
  {
    std::vector<std::pair<size_t, size_t>> edges;
    edges.reserve(n > 0 ? n - 1 : 0);

    for (size_t v = 1; v < n; ++v)
      {
        std::uniform_int_distribution<size_t> pick(0, v - 1);
        edges.emplace_back(v, pick(rng));
      }

    return edges;
  }

  class Rooted_Oracle
  {
    static constexpr size_t NONE = std::numeric_limits<size_t>::max();

    size_t n_ = 0;
    size_t root_ = 0;

    std::vector<std::vector<size_t>> adj_;
    std::vector<std::vector<size_t>> children_;
    std::vector<size_t> parent_;
    std::vector<size_t> depth_;

  public:
    Rooted_Oracle(const size_t n,
                  const std::vector<std::pair<size_t, size_t>> & edges,
                  const size_t root)
      : n_(n),
        root_(root),
        adj_(n),
        children_(n),
        parent_(n, NONE),
        depth_(n, 0)
    {
      for (const auto & [u, v] : edges)
        {
          adj_[u].push_back(v);
          adj_[v].push_back(u);
        }

      std::vector<char> visited(n_, 0);
      struct Frame
      {
        size_t u;
        size_t p;
        size_t next;
      };

      std::vector<Frame> stack;
      stack.push_back({root_, NONE, 0});
      visited[root_] = 1;
      parent_[root_] = NONE;
      depth_[root_] = 0;

      while (not stack.empty())
        {
          auto & fr = stack.back();
          if (fr.next == adj_[fr.u].size())
            {
              stack.pop_back();
              continue;
            }

          const size_t v = adj_[fr.u][fr.next++];
          if (v == fr.p)
            continue;

          if (visited[v])
            continue;

          visited[v] = 1;
          parent_[v] = fr.u;
          depth_[v] = depth_[fr.u] + 1;
          children_[fr.u].push_back(v);
          stack.push_back({v, fr.u, 0});
        }
    }

    [[nodiscard]] size_t lca(size_t u, size_t v) const
    {
      while (depth_[u] > depth_[v])
        u = parent_[u];

      while (depth_[v] > depth_[u])
        v = parent_[v];

      while (u != v)
        {
          u = parent_[u];
          v = parent_[v];
        }

      return u;
    }

    [[nodiscard]] size_t distance(const size_t u, const size_t v) const
    {
      const size_t a = lca(u, v);
      return depth_[u] + depth_[v] - 2 * depth_[a];
    }

    [[nodiscard]] int path_sum(const std::vector<int> & values,
                               size_t u,
                               size_t v) const
    {
      int sum = 0;

      while (depth_[u] > depth_[v])
        {
          sum += values[u];
          u = parent_[u];
        }

      while (depth_[v] > depth_[u])
        {
          sum += values[v];
          v = parent_[v];
        }

      while (u != v)
        {
          sum += values[u];
          sum += values[v];
          u = parent_[u];
          v = parent_[v];
        }

      sum += values[u];
      return sum;
    }

    [[nodiscard]] int subtree_sum(const std::vector<int> & values,
                                  const size_t u) const
    {
      int sum = 0;
      std::vector<size_t> stack;
      stack.push_back(u);

      while (not stack.empty())
        {
          const size_t x = stack.back();
          stack.pop_back();
          sum += values[x];

          for (const auto ch : children_[x])
            stack.push_back(ch);
        }

      return sum;
    }
  };

  template <class GT>
  class TreeDecompositionTypedTest : public ::testing::Test
  {
  };

  using GraphBackends = ::testing::Types<
      List_Graph<Graph_Node<int>, Graph_Arc<int>>,
      List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
      Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

  TYPED_TEST_SUITE(TreeDecompositionTypedTest, GraphBackends);

  struct Positive_Arc_Filter
  {
    template <class Arc>
    bool operator()(Arc * a) const noexcept
    {
      return a->get_info() > 0;
    }
  };
}


TYPED_TEST(TreeDecompositionTypedTest, EmptyGraph)
{
  using Graph = TypeParam;

  Graph g;

  Heavy_Light_Decomposition<Graph> hld(g);
  EXPECT_TRUE(hld.is_empty());
  EXPECT_EQ(hld.size(), 0u);

  HLD_Path_Query<Graph, int, Aleph::plus<int>> q(g, 0);
  EXPECT_TRUE(q.is_empty());
  EXPECT_THROW(q.query_path_id(0, 0), std::domain_error);

  Centroid_Decomposition<Graph> cd(g);
  EXPECT_TRUE(cd.is_empty());
  EXPECT_EQ(cd.size(), 0u);
  EXPECT_EQ(cd.centroid_root(), nullptr);
  EXPECT_THROW(cd.for_each_centroid_ancestor_id(0,
                                                [](const size_t,
                                                   const size_t,
                                                   const size_t) {}),
               std::domain_error);
}

TYPED_TEST(TreeDecompositionTypedTest, InvalidGraphsThrow)
{
  using Graph = TypeParam;

  {
    Graph g;
    auto * n0 = g.insert_node(0);
    auto * n1 = g.insert_node(1);
    auto * n2 = g.insert_node(2);
    g.insert_arc(n0, n1, 1);
    g.insert_arc(n1, n2, 1);
    g.insert_arc(n2, n0, 1); // cycle

    EXPECT_THROW((Heavy_Light_Decomposition<Graph>(g, n0)), std::domain_error);
    EXPECT_THROW((Centroid_Decomposition<Graph>(g, n0)), std::domain_error);
  }

  {
    Graph g;
    auto * n0 = g.insert_node(0);
    auto * n1 = g.insert_node(1);
    auto * n2 = g.insert_node(2);
    auto * n3 = g.insert_node(3);
    g.insert_arc(n0, n1, 1);
    g.insert_arc(n2, n3, 1); // disconnected

    EXPECT_THROW((Heavy_Light_Decomposition<Graph>(g, n0)), std::domain_error);
    EXPECT_THROW((Centroid_Decomposition<Graph>(g, n0)), std::domain_error);
  }

  {
    Graph g;
    auto * n0 = g.insert_node(0);
    g.insert_arc(n0, n0, 1); // self-loop

    EXPECT_THROW((Heavy_Light_Decomposition<Graph>(g, n0)), std::domain_error);
    EXPECT_THROW((Centroid_Decomposition<Graph>(g, n0)), std::domain_error);
  }

  {
    Graph g;
    auto * n0 = g.insert_node(0);
    auto * n1 = g.insert_node(1);
    g.insert_arc(n0, n1, 1);
    g.insert_arc(n0, n1, 1); // parallel edges

    EXPECT_THROW((Heavy_Light_Decomposition<Graph>(g, n0)), std::domain_error);
    EXPECT_THROW((Centroid_Decomposition<Graph>(g, n0)), std::domain_error);
  }
}

TYPED_TEST(TreeDecompositionTypedTest, ArcFilterSupport)
{
  using Graph = TypeParam;

  Graph g;
  auto * n0 = g.insert_node(5);
  auto * n1 = g.insert_node(3);
  auto * n2 = g.insert_node(4);
  auto * n3 = g.insert_node(7);

  // Positive arcs define a valid tree.
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n1, n3, 1);

  // Negative arc exists in graph but is filtered out.
  g.insert_arc(n0, n2, -8);

  Heavy_Light_Decomposition<Graph, Positive_Arc_Filter> hld(g, n0, Positive_Arc_Filter());
  EXPECT_EQ(hld.size(), 4u);
  EXPECT_EQ(hld.distance(n2, n3), 2u);

  HLD_Path_Query<Graph, int, Aleph::plus<int>, Positive_Arc_Filter>
      q(g, n0, 0, Aleph::plus<int>(), Positive_Arc_Filter());

  EXPECT_EQ(q.query_path(n2, n3), 3 + 4 + 7);

  Centroid_Decomposition<Graph, Positive_Arc_Filter> cd(g, n0, Positive_Arc_Filter());
  EXPECT_EQ(cd.size(), 4u);
  EXPECT_NE(cd.centroid_root(), nullptr);
}

TYPED_TEST(TreeDecompositionTypedTest, DeterministicHeavyLightQueries)
{
  using Graph = TypeParam;

  const std::vector<int> values = {5, 3, 4, 7, 2, 6, 1};
  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2}, {1, 3}, {1, 4}, {2, 5}, {5, 6}};

  Graph g;
  auto nodes = build_graph_with_values(g, values, edges);

  Rooted_Oracle oracle(values.size(), edges, 0);

  Heavy_Light_Decomposition<Graph> hld(g, nodes(0));
  HLD_Path_Query<Graph, int, Aleph::plus<int>> q(g, nodes(0), 0);

  for (size_t u = 0; u < values.size(); ++u)
    for (size_t v = 0; v < values.size(); ++v)
      {
        EXPECT_EQ(hld.lca(nodes(u), nodes(v)), nodes(oracle.lca(u, v)));
        EXPECT_EQ(hld.distance(nodes(u), nodes(v)), oracle.distance(u, v));
        EXPECT_EQ(q.query_path(nodes(u), nodes(v)), oracle.path_sum(values, u, v));
      }

  for (size_t u = 0; u < values.size(); ++u)
    {
      const size_t id = hld.id_of(nodes(u));
      const auto range = hld.subtree_range_id(id);
      EXPECT_EQ(range.right - range.left + 1, hld.subtree_size_of_id(id));
      EXPECT_EQ(q.query_subtree(nodes(u)), oracle.subtree_sum(values, u));
    }

  std::vector<int> mutable_values = values;
  q.update_node(nodes(3), 5);  // 7 -> 12
  mutable_values[3] += 5;
  q.set_node(nodes(4), 10);    // 2 -> 10
  mutable_values[4] = 10;

  for (size_t u = 0; u < values.size(); ++u)
    for (size_t v = 0; v < values.size(); ++v)
      EXPECT_EQ(q.query_path(nodes(u), nodes(v)),
                oracle.path_sum(mutable_values, u, v));

  for (size_t u = 0; u < values.size(); ++u)
    EXPECT_EQ(q.query_subtree(nodes(u)), oracle.subtree_sum(mutable_values, u));
}

TYPED_TEST(TreeDecompositionTypedTest, RandomHeavyLightAgainstOracle)
{
  using Graph = TypeParam;

  std::mt19937 rng(0xA13F2026u);
  std::uniform_int_distribution<int> pick_value(-20, 20);

  for (size_t trial = 0; trial < 20; ++trial)
    {
      std::uniform_int_distribution<size_t> pick_n(5, 60);
      const size_t n = pick_n(rng);

      auto edges = random_tree_edges(n, rng);

      std::vector<int> values(n);
      for (size_t i = 0; i < n; ++i)
        values[i] = pick_value(rng);

      Graph g;
      auto nodes = build_graph_with_values(g, values, edges);

      Rooted_Oracle oracle(n, edges, 0);
      HLD_Path_Query<Graph, int, Aleph::plus<int>> q(g, nodes(0), 0);
      const auto & hld = q.decomposition();

      std::uniform_int_distribution<size_t> pick_id(0, n - 1);

      for (size_t it = 0; it < 250; ++it)
        {
          const int action = static_cast<int>(rng() % 100);

          if (action < 35)
            {
              const size_t u = pick_id(rng);
              if ((rng() & 1U) == 0U)
                {
                  const int delta = pick_value(rng);
                  q.update_node(nodes(u), delta);
                  values[u] += delta;
                }
              else
                {
                  const int val = pick_value(rng);
                  q.set_node(nodes(u), val);
                  values[u] = val;
                }
            }
          else
            {
              const size_t u = pick_id(rng);
              const size_t v = pick_id(rng);
              const size_t w = pick_id(rng);

              EXPECT_EQ(hld.lca(nodes(u), nodes(v)), nodes(oracle.lca(u, v)));
              EXPECT_EQ(hld.distance(nodes(u), nodes(v)), oracle.distance(u, v));
              EXPECT_EQ(q.query_path(nodes(u), nodes(v)), oracle.path_sum(values, u, v));
              EXPECT_EQ(q.query_subtree(nodes(w)), oracle.subtree_sum(values, w));
            }
        }
    }
}

TYPED_TEST(TreeDecompositionTypedTest, CentroidAnnotationsAndDynamicNearest)
{
  using Graph = TypeParam;

  const std::vector<int> values = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2}, {1, 3}, {1, 4}, {2, 5}, {2, 6}, {6, 7}, {6, 8}};

  Graph g;
  auto nodes = build_graph_with_values(g, values, edges);

  Rooted_Oracle oracle(values.size(), edges, 0);
  Centroid_Decomposition<Graph> cd(g, nodes(0));

  EXPECT_EQ(cd.size(), values.size());
  EXPECT_NE(cd.centroid_root(), nullptr);

  std::vector<size_t> algo_of_input(values.size());
  std::vector<size_t> input_of_algo(values.size());
  for (size_t i = 0; i < values.size(); ++i)
    {
      const size_t aid = cd.id_of(nodes(i));
      algo_of_input[i] = aid;
      input_of_algo[aid] = i;
    }

  for (size_t u = 0; u < values.size(); ++u)
    {
      const size_t au = algo_of_input[u];
      const auto & anc = cd.centroid_ancestors_of_id(au);
      const auto & dist = cd.centroid_distances_of_id(au);

      ASSERT_EQ(anc.size(), dist.size());
      ASSERT_GT(anc.size(), 0u);

      EXPECT_EQ(anc(0), cd.centroid_root_id());
      EXPECT_EQ(anc(anc.size() - 1), au);
      EXPECT_EQ(dist(anc.size() - 1), 0u);

      for (size_t k = 0; k < anc.size(); ++k)
        EXPECT_EQ(dist(k), oracle.distance(u, input_of_algo[anc(k)]));

      for (size_t k = 1; k < anc.size(); ++k)
        {
          EXPECT_EQ(cd.centroid_parent_id(anc(k)), anc(k - 1));
          EXPECT_EQ(cd.centroid_level_of_id(anc(k)),
                    cd.centroid_level_of_id(anc(k - 1)) + 1);
        }
    }

  const size_t INF = std::numeric_limits<size_t>::max() / 4;
  std::vector<size_t> best(cd.size(), INF);
  std::vector<char> active(values.size(), 0);

  auto activate = [&](const size_t u)
    {
      active[u] = 1;
      cd.for_each_centroid_ancestor_id(
          algo_of_input[u],
          [&](const size_t c, const size_t d, const size_t)
          {
            if (d < best[c])
              best[c] = d;
          });
    };

  auto query = [&](const size_t u)
    {
      size_t ans = INF;
      cd.for_each_centroid_ancestor_id(
          algo_of_input[u],
          [&](const size_t c, const size_t d, const size_t)
          {
            if (best[c] != INF)
              ans = std::min(ans, best[c] + d);
          });
      return ans;
    };

  auto brute_query = [&](const size_t u)
    {
      size_t ans = INF;
      for (size_t v = 0; v < active.size(); ++v)
        if (active[v])
          ans = std::min(ans, oracle.distance(u, v));
      return ans;
    };

  std::mt19937 rng(0xC3E7D01Du);
  std::uniform_int_distribution<size_t> pick_id(0, values.size() - 1);

  activate(0);

  for (size_t it = 0; it < 300; ++it)
    {
      const size_t u = pick_id(rng);
      if ((rng() % 100) < 40)
        activate(u);
      else
        EXPECT_EQ(query(u), brute_query(u));
    }
}
