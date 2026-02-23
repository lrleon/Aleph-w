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
 * @file hld_test.cc
 * @brief Tests for HLD.H (Heavy-Light Decomposition).
 *
 * Coverage:
 * - Empty and single-node trees
 * - Deterministic rooted trees with known path sums, max, and subtree queries
 * - Point updates
 * - Edge-weighted path queries
 * - LCA via HLD (cross-validated against Binary_Lifting_LCA)
 * - Decomposition property invariants
 * - Input validation (cycle, disconnected, loop, parallel edge)
 * - Arc-filter behavior
 * - Random stress against an independent naive oracle
 * - Cross-backend consistency (List_Graph, List_SGraph, Array_Graph)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include <HLD.H>
#include <LCA.H>
#include <tpl_agraph.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

#include "test_graph_helpers.h"

using namespace Aleph;
using Aleph_Test_Helpers::build_graph_with_unit_arcs;
using Aleph_Test_Helpers::Positive_Arc_Filter;
using Aleph_Test_Helpers::make_random_tree_edges;

namespace
{
  // Naive oracle that computes path queries by walking to LCA
  class Naive_Path_Oracle
  {
    static constexpr size_t NONE = std::numeric_limits<size_t>::max();

    size_t n_ = 0;
    size_t root_ = 0;
    std::vector<std::vector<size_t>> adj_;
    std::vector<size_t> parent_;
    std::vector<size_t> depth_;
    std::vector<int> values_;

    void dfs(const size_t u, const size_t p, const size_t d)
    {
      parent_[u] = p;
      depth_[u] = d;

      for (const auto v : adj_[u])
        if (v != p)
          dfs(v, u, d + 1);
    }

  public:
    Naive_Path_Oracle(const size_t n,
                      const std::vector<std::pair<size_t, size_t>> & edges,
                      const size_t root,
                      const std::vector<int> & vals)
      : n_(n), root_(root), adj_(n), parent_(n, NONE),
        depth_(n, 0), values_(vals)
    {
      for (const auto & [u, v] : edges)
        {
          adj_[u].push_back(v);
          adj_[v].push_back(u);
        }
      dfs(root_, NONE, 0);
    }

    [[nodiscard]] size_t lca(size_t u, size_t v) const
    {
      if (depth_[u] < depth_[v])
        std::swap(u, v);

      while (depth_[u] > depth_[v])
        u = parent_[u];

      while (u != v)
        {
          u = parent_[u];
          v = parent_[v];
        }
      return u;
    }

    [[nodiscard]] int path_sum(size_t u, size_t v) const
    {
      const size_t a = lca(u, v);
      int result = 0;
      size_t x = u;
      while (x != a)
        {
          result += values_[x];
          x = parent_[x];
        }
      x = v;
      while (x != a)
        {
          result += values_[x];
          x = parent_[x];
        }
      result += values_[a];
      return result;
    }

    [[nodiscard]] int path_max(size_t u, size_t v) const
    {
      const size_t a = lca(u, v);
      int result = std::numeric_limits<int>::lowest();
      size_t x = u;
      while (x != a)
        {
          result = std::max(result, values_[x]);
          x = parent_[x];
        }
      x = v;
      while (x != a)
        {
          result = std::max(result, values_[x]);
          x = parent_[x];
        }
      result = std::max(result, values_[a]);
      return result;
    }

    [[nodiscard]] int path_min(size_t u, size_t v) const
    {
      const size_t a = lca(u, v);
      int result = std::numeric_limits<int>::max();
      size_t x = u;
      while (x != a)
        {
          result = std::min(result, values_[x]);
          x = parent_[x];
        }
      x = v;
      while (x != a)
        {
          result = std::min(result, values_[x]);
          x = parent_[x];
        }
      result = std::min(result, values_[a]);
      return result;
    }

    [[nodiscard]] int subtree_sum(size_t v) const
    {
      int result = values_[v];
      std::vector<size_t> stack = {v};
      while (not stack.empty())
        {
          const size_t u = stack.back();
          stack.pop_back();
          for (const auto c : adj_[u])
            if (c != parent_[u])
              {
                result += values_[c];
                stack.push_back(c);
              }
        }
      return result;
    }

    [[nodiscard]] size_t distance(size_t u, size_t v) const
    {
      const size_t a = lca(u, v);
      return depth_[u] + depth_[v] - 2 * depth_[a];
    }

    void set_value(size_t u, int val) { values_[u] = val; }
    [[nodiscard]] int value(size_t u) const { return values_[u]; }
    [[nodiscard]] size_t depth(size_t u) const { return depth_[u]; }
  };

  template <class GT>
  class HldTypedTest : public ::testing::Test
  {
  };

  using GraphBackends = ::testing::Types<
      List_Graph<Graph_Node<int>, Graph_Arc<int>>,
      List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
      Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

  TYPED_TEST_SUITE(HldTypedTest, GraphBackends);
}


TYPED_TEST(HldTypedTest, EmptyTree)
{
  using Graph = TypeParam;

  Graph g;

  auto nv = [](typename Graph::Node * p) { return p->get_info(); };
  HLD_Sum<Graph, int> hld(g, nv);

  EXPECT_TRUE(hld.is_empty());
  EXPECT_EQ(hld.size(), 0u);
  EXPECT_THROW(hld.path_query_id(0, 0), std::domain_error);
  EXPECT_THROW(hld.subtree_query_id(0), std::domain_error);
  EXPECT_THROW(hld.lca_id(0, 0), std::domain_error);
}

TYPED_TEST(HldTypedTest, SingleNode)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  Graph g;
  Node * n0 = g.insert_node(42);

  auto nv = [](Node * p) { return p->get_info(); };
  HLD_Sum<Graph, int> hld(g, n0, nv);

  EXPECT_EQ(hld.size(), 1u);
  EXPECT_EQ(hld.root(), n0);

  EXPECT_EQ(hld.path_query(n0, n0), 42);
  EXPECT_EQ(hld.subtree_query(n0), 42);
  EXPECT_EQ(hld.lca(n0, n0), n0);
  EXPECT_EQ(hld.distance(n0, n0), 0u);
  EXPECT_EQ(hld.depth_of(n0), 0u);
  EXPECT_EQ(hld.parent_of(n0), nullptr);
  EXPECT_EQ(hld.num_chains(), 1u);
}

TYPED_TEST(HldTypedTest, ManualTreePathSum)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  //  Node values: node_id * 10
  //           0 (val=0)
  //        /          |
  //       1(10)   2(20)
  //      / |     / |
  //     3(30)4(40) 5(50)6(60)
  //               / |
  //             7(70)8(80)

  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2},
      {1, 3}, {1, 4},
      {2, 5}, {2, 6},
      {5, 7}, {5, 8}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 9, edges);

  auto nv = [](Node * p) { return p->get_info() * 10; };
  HLD_Sum<Graph, int> hld(g, nodes(0), nv);

  // path(3, 4) = 30 + 10 + 40 = 80
  EXPECT_EQ(hld.path_query(nodes(3), nodes(4)), 80);

  // path(3, 6) = 30 + 10 + 0 + 20 + 60 = 120
  EXPECT_EQ(hld.path_query(nodes(3), nodes(6)), 120);

  // path(7, 8) = 70 + 50 + 80 = 200
  EXPECT_EQ(hld.path_query(nodes(7), nodes(8)), 200);

  // path(7, 6) = 70 + 50 + 20 + 60 = 200
  EXPECT_EQ(hld.path_query(nodes(7), nodes(6)), 200);

  // path(0, 8) = 0 + 20 + 50 + 80 = 150
  EXPECT_EQ(hld.path_query(nodes(0), nodes(8)), 150);

  // path(3, 8) = 30 + 10 + 0 + 20 + 50 + 80 = 190
  EXPECT_EQ(hld.path_query(nodes(3), nodes(8)), 190);

  // Self path
  EXPECT_EQ(hld.path_query(nodes(5), nodes(5)), 50);
}

TYPED_TEST(HldTypedTest, ManualTreePathMax)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2},
      {1, 3}, {1, 4},
      {2, 5}, {2, 6},
      {5, 7}, {5, 8}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 9, edges);

  // Values: [5, 3, 8, 1, 9, 2, 7, 4, 6]
  const int vals[] = {5, 3, 8, 1, 9, 2, 7, 4, 6};
  auto nv = [&vals](Node * p) { return vals[p->get_info()]; };
  HLD_Max<Graph, int> hld(g, nodes(0), nv);

  // path(3, 4) = max(1, 3, 9) = 9
  EXPECT_EQ(hld.path_query(nodes(3), nodes(4)), 9);

  // path(7, 6) = max(4, 2, 8, 7) = 8
  EXPECT_EQ(hld.path_query(nodes(7), nodes(6)), 8);

  // path(3, 8) = max(1, 3, 5, 8, 2, 6) = 8
  EXPECT_EQ(hld.path_query(nodes(3), nodes(8)), 8);
}

TYPED_TEST(HldTypedTest, ManualTreeSubtreeQuery)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2},
      {1, 3}, {1, 4},
      {2, 5}, {2, 6},
      {5, 7}, {5, 8}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 9, edges);

  auto nv = [](Node * p) { return p->get_info() + 1; };  // val = id + 1
  HLD_Sum<Graph, int> hld(g, nodes(0), nv);

  // subtree(5) = 6 + 8 + 9 = 23
  EXPECT_EQ(hld.subtree_query(nodes(5)), 23);

  // subtree(2) = 3 + 6 + 7 + 8 + 9 = 33
  EXPECT_EQ(hld.subtree_query(nodes(2)), 33);

  // subtree(1) = 2 + 4 + 5 = 11
  EXPECT_EQ(hld.subtree_query(nodes(1)), 11);

  // subtree(0) = 1+2+3+4+5+6+7+8+9 = 45
  EXPECT_EQ(hld.subtree_query(nodes(0)), 45);

  // Leaf subtree
  EXPECT_EQ(hld.subtree_query(nodes(7)), 8);
}

TYPED_TEST(HldTypedTest, PointUpdate)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2}, {1, 3}, {1, 4}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 5, edges);

  // Values: all 10
  auto nv = [](Node *) { return 10; };
  HLD_Sum<Graph, int> hld(g, nodes(0), nv);

  // path(3,4) = 10 + 10 + 10 = 30
  EXPECT_EQ(hld.path_query(nodes(3), nodes(4)), 30);

  // Update node 1 to 100
  hld.point_update(nodes(1), 100);

  // path(3,4) = 10 + 100 + 10 = 120
  EXPECT_EQ(hld.path_query(nodes(3), nodes(4)), 120);

  // subtree(1) = 100 + 10 + 10 = 120
  EXPECT_EQ(hld.subtree_query(nodes(1)), 120);

  // Verify get_value
  EXPECT_EQ(hld.get_value(nodes(1)), 100);
  EXPECT_EQ(hld.get_value(nodes(0)), 10);
}

TYPED_TEST(HldTypedTest, PathQueryEdges)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  // Edge-weighted tree: store edge weight on child node, root = 0
  //        0 (val=0, root)
  //       / |
  //      1   2     (edge 0-1 has weight 5, edge 0-2 has weight 3)
  //     /
  //    3              (edge 1-3 has weight 7)
  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2}, {1, 3}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 4, edges);

  // Values on children represent edge weights: root=0, 1=5, 2=3, 3=7
  const int edge_vals[] = {0, 5, 3, 7};
  auto nv = [&edge_vals](Node * p) { return edge_vals[p->get_info()]; };
  HLD_Sum<Graph, int> hld(g, nodes(0), nv);

  // path_query_edges(3, 2): path is 3-1-0-2, edges are 7+5+3 = 15
  // (skip LCA which is 0 with value 0)
  EXPECT_EQ(hld.path_query_edges(nodes(3), nodes(2)), 15);

  // path_query_edges(1, 3): path is 1-3, edge is 7
  EXPECT_EQ(hld.path_query_edges(nodes(1), nodes(3)), 7);

  // path_query_edges(1, 2): path is 1-0-2, edges are 5+3 = 8
  EXPECT_EQ(hld.path_query_edges(nodes(1), nodes(2)), 8);

  // path_query_edges(v, v) should be identity (0) since no edges
  EXPECT_EQ(hld.path_query_edges(nodes(1), nodes(1)), 0);
}

TYPED_TEST(HldTypedTest, LcaFromHLD)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2},
      {1, 3}, {1, 4},
      {2, 5}, {2, 6},
      {5, 7}, {5, 8}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 9, edges);

  auto nv = [](Node * p) { return p->get_info(); };
  HLD_Sum<Graph, int> hld(g, nodes(0), nv);
  Binary_Lifting_LCA<Graph> bl(g, nodes(0));

  // Cross-validate all pairs
  for (size_t u = 0; u < 9; ++u)
    for (size_t v = u; v < 9; ++v)
      {
        auto * hld_lca = hld.lca(nodes(u), nodes(v));
        auto * bl_lca = bl.lca(nodes(u), nodes(v));
        EXPECT_EQ(hld_lca, bl_lca)
          << "u=" << u << " v=" << v;

        EXPECT_EQ(hld.distance(nodes(u), nodes(v)),
                  bl.distance(nodes(u), nodes(v)));
      }
}

TYPED_TEST(HldTypedTest, DecompositionProperties)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  const std::vector<std::pair<size_t, size_t>> edges = {
      {0, 1}, {0, 2},
      {1, 3}, {1, 4},
      {2, 5}, {2, 6},
      {5, 7}, {5, 8}};

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 9, edges);

  auto nv = [](Node * p) { return p->get_info(); };
  HLD_Sum<Graph, int> hld(g, nodes(0), nv);

  EXPECT_EQ(hld.size(), 9u);
  EXPECT_GE(hld.num_chains(), 1u);
  EXPECT_LE(hld.num_chains(), 9u);

  // Every position is unique and in [0, n)
  std::vector<bool> seen(9, false);
  for (size_t i = 0; i < 9; ++i)
    {
      const size_t pos = hld.position(i);
      EXPECT_LT(pos, 9u);
      EXPECT_FALSE(seen[pos]) << "Duplicate position at node " << i;
      seen[pos] = true;
    }

  // Chain head of root is root
  EXPECT_EQ(hld.chain_head_id(hld.root_id()), hld.root_id());

  // Root depth is 0
  EXPECT_EQ(hld.depth_of_id(hld.root_id()), 0u);

  // Subtree size of root = n
  EXPECT_EQ(hld.subtree_size_of_id(hld.root_id()), 9u);

  // For each node, chain_head has depth <= node depth
  for (size_t i = 0; i < 9; ++i)
    {
      const size_t head = hld.chain_head_id(i);
      EXPECT_LE(hld.depth_of_id(head), hld.depth_of_id(i));
    }
}

TYPED_TEST(HldTypedTest, ValidationErrors)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  auto nv = [](Node * p) { return p->get_info(); };

  // Cycle
  {
    Graph g;
    auto nodes = build_graph_with_unit_arcs(g, 3, {{0, 1}, {1, 2}, {2, 0}});
    EXPECT_THROW((HLD_Sum<Graph, int>(g, nodes(0), nv)), std::domain_error);
  }

  // Disconnected
  {
    Graph g;
    auto nodes = build_graph_with_unit_arcs(g, 4, {{0, 1}, {2, 3}});
    EXPECT_THROW((HLD_Sum<Graph, int>(g, nodes(0), nv)), std::domain_error);
  }

  // Self-loop
  {
    Graph g;
    auto nodes = build_graph_with_unit_arcs(g, 3, {{0, 1}, {1, 2}, {2, 2}});
    EXPECT_THROW((HLD_Sum<Graph, int>(g, nodes(0), nv)), std::domain_error);
  }

  // Parallel edge
  {
    Graph g;
    auto nodes = build_graph_with_unit_arcs(g, 2, {{0, 1}, {0, 1}});
    EXPECT_THROW((HLD_Sum<Graph, int>(g, nodes(0), nv)), std::domain_error);
  }
}

TYPED_TEST(HldTypedTest, ArcFilter)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  Graph g;
  auto nodes = Array<Node *>::create(6);
  for (size_t i = 0; i < 6; ++i)
    nodes(i) = g.insert_node(static_cast<int>(i));

  // Tree edges (kept by filter)
  g.insert_arc(nodes(0), nodes(1), 1);
  g.insert_arc(nodes(0), nodes(2), 1);
  g.insert_arc(nodes(1), nodes(3), 1);
  g.insert_arc(nodes(1), nodes(4), 1);
  g.insert_arc(nodes(2), nodes(5), 1);

  // Non-tree edges (filtered out)
  g.insert_arc(nodes(3), nodes(5), -1);
  g.insert_arc(nodes(4), nodes(2), -1);

  // Without filter: should fail (not a tree)
  auto nv = [](Node * p) { return p->get_info(); };
  EXPECT_THROW((HLD_Sum<Graph, int>(g, nodes(0), nv)), std::domain_error);

  // With filter: should work
  HLD_Sum<Graph, int, Positive_Arc_Filter>
    hld(g, nodes(0), nv, Positive_Arc_Filter());

  EXPECT_EQ(hld.size(), 6u);

  auto * l = hld.lca(nodes(3), nodes(5));
  EXPECT_EQ(static_cast<size_t>(l->get_info()), 0u);

  l = hld.lca(nodes(3), nodes(4));
  EXPECT_EQ(static_cast<size_t>(l->get_info()), 1u);
}

TYPED_TEST(HldTypedTest, StressRandomAgainstNaive)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  std::mt19937 rng(0x41d2026u);

  for (size_t sample = 0; sample < 20; ++sample)
    {
      std::uniform_int_distribution<size_t> n_pick(2, 80);
      const size_t n = n_pick(rng);
      const auto edges = make_random_tree_edges<std::vector<std::pair<size_t, size_t>>>(n, rng);

      std::uniform_int_distribution<int> val_pick(-100, 100);
      std::vector<int> vals(n);
      for (size_t i = 0; i < n; ++i)
        vals[i] = val_pick(rng);

      std::uniform_int_distribution<size_t> root_pick(0, n - 1);
      const size_t root = root_pick(rng);

      Graph g;
      auto nodes = build_graph_with_unit_arcs(g, n, edges);

      auto nv_sum = [&vals](Node * p) { return vals[p->get_info()]; };
      auto nv_max = [&vals](Node * p) { return vals[p->get_info()]; };
      auto nv_min = [&vals](Node * p) { return vals[p->get_info()]; };

      HLD_Sum<Graph, int> hld_sum(g, nodes(root), nv_sum);
      HLD_Max<Graph, int> hld_max(g, nodes(root), nv_max);
      HLD_Min<Graph, int> hld_min(g, nodes(root), nv_min);

      Naive_Path_Oracle oracle(n, edges, root, vals);

      std::uniform_int_distribution<size_t> node_pick(0, n - 1);

      // Test path queries
      for (size_t q = 0; q < 200; ++q)
        {
          const size_t u = node_pick(rng);
          const size_t v = node_pick(rng);

          EXPECT_EQ(hld_sum.path_query(nodes(u), nodes(v)),
                    oracle.path_sum(u, v))
            << "sample=" << sample << " u=" << u << " v=" << v;

          EXPECT_EQ(hld_max.path_query(nodes(u), nodes(v)),
                    oracle.path_max(u, v))
            << "sample=" << sample << " u=" << u << " v=" << v;

          EXPECT_EQ(hld_min.path_query(nodes(u), nodes(v)),
                    oracle.path_min(u, v))
            << "sample=" << sample << " u=" << u << " v=" << v;

          // LCA
          EXPECT_EQ(static_cast<size_t>(hld_sum.lca(nodes(u), nodes(v))->get_info()),
                    oracle.lca(u, v))
            << "sample=" << sample << " u=" << u << " v=" << v;

          // Distance
          EXPECT_EQ(hld_sum.distance(nodes(u), nodes(v)),
                    oracle.distance(u, v))
            << "sample=" << sample << " u=" << u << " v=" << v;
        }

      // Test subtree queries (sum only)
      for (size_t q = 0; q < 50; ++q)
        {
          const size_t v = node_pick(rng);
          EXPECT_EQ(hld_sum.subtree_query(nodes(v)),
                    oracle.subtree_sum(v))
            << "sample=" << sample << " v=" << v;
        }

      // Test point updates
      for (size_t q = 0; q < 20; ++q)
        {
          const size_t v = node_pick(rng);
          const int new_val = val_pick(rng);

          hld_sum.point_update(nodes(v), new_val);
          oracle.set_value(v, new_val);

          // Verify a few queries after update
          const size_t u1 = node_pick(rng);
          const size_t v1 = node_pick(rng);
          EXPECT_EQ(hld_sum.path_query(nodes(u1), nodes(v1)),
                    oracle.path_sum(u1, v1))
            << "sample=" << sample << " after update";
        }
    }
}
