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
 * @file lca_test.cc
 * @brief Tests for LCA.H (binary lifting and Euler tour + RMQ).
 *
 * Coverage:
 * - Empty and single-node trees
 * - Deterministic rooted trees with known LCAs/distances
 * - Input validation (cycle, disconnected, loop, parallel edge)
 * - Arc-filter behavior
 * - Node ownership and range checks
 * - Random stress against an independent naive oracle
 * - Cross-backend consistency (List_Graph, List_SGraph, Array_Graph)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

#include <LCA.H>
#include <tpl_agraph.H>
#include <tpl_array.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <htlist.H>

#include "test_graph_helpers.h"

using namespace Aleph;
using Aleph_Test_Helpers::build_graph_with_unit_arcs;
using Aleph_Test_Helpers::Positive_Arc_Filter;
using Aleph_Test_Helpers::make_random_tree_edges;

namespace
{
  class Naive_Tree_Oracle
  {
    static constexpr size_t NONE = std::numeric_limits<size_t>::max();

    size_t n_ = 0;
    size_t root_ = 0;
    Array<DynList<size_t>> adj_;
    Array<size_t> parent_;
    Array<size_t> depth_;
    Array<size_t> tin_;
    Array<size_t> tout_;
    size_t timer_ = 0;

    void dfs(const size_t u, const size_t p, const size_t d)
    {
      struct Frame
      {
        size_t u = 0;
        size_t p = NONE;
        size_t d = 0;
        bool entering = true;
      };

      Array<Frame> stack;
      stack.reserve(n_ * 2);
      stack.append(Frame{u, p, d, true});

      while (not stack.is_empty())
        {
          const Frame f = stack.remove_last();
          if (f.entering)
            {
              parent_[f.u] = f.p;
              depth_[f.u] = f.d;
              tin_[f.u] = timer_++;

              stack.append(Frame{f.u, f.p, f.d, false});

              Array<size_t> children;
              for (auto it = adj_[f.u].get_it(); it.has_curr(); it.next_ne())
                {
                  const size_t v = it.get_curr();
                  if (v != f.p)
                    children.append(v);
                }

              for (long i = static_cast<long>(children.size()) - 1; i >= 0; --i)
                stack.append(Frame{children[static_cast<size_t>(i)], f.u, f.d + 1, true});
            }
          else
            {
              tout_[f.u] = timer_ - 1;
            }
        }
    }

  public:
    Naive_Tree_Oracle(const size_t n,
                      const Array<std::pair<size_t, size_t>> & edges,
                      const size_t root)
      : n_(n),
        root_(root),
        adj_(Array<DynList<size_t>>::create(n)),
        parent_(n, NONE),
        depth_(n, 0),
        tin_(n, 0),
        tout_(n, 0)
    {
      ah_runtime_error_if(n_ == 0) << "Naive_Tree_Oracle requires n > 0";
      ah_runtime_error_if(root_ >= n_) << "Naive_Tree_Oracle: invalid root";
      ah_runtime_error_if(edges.size() != n_ - 1)
        << "Naive_Tree_Oracle: edges.size() must be n-1";

      for (typename Array<std::pair<size_t, size_t>>::Iterator it(edges);
           it.has_curr(); it.next_ne())
        {
          const auto & e = it.get_curr();
          const size_t u = e.first;
          const size_t v = e.second;

          ah_runtime_error_if(u >= n_ or v >= n_ or u == v)
            << "Naive_Tree_Oracle: invalid edge";
          adj_[u].append(v);
          adj_[v].append(u);
        }

      dfs(root_, NONE, 0);

      for (size_t i = 0; i < n_; ++i)
        ah_runtime_error_if(parent_[i] == NONE and i != root_)
          << "Naive_Tree_Oracle: disconnected input";
    }

    [[nodiscard]] size_t depth(const size_t u) const
    {
      ah_runtime_error_if(u >= n_) << "Naive_Tree_Oracle::depth: bad index";
      return depth_[u];
    }

    [[nodiscard]] bool is_ancestor(const size_t u, const size_t v) const
    {
      ah_runtime_error_if(u >= n_ or v >= n_) << "Naive_Tree_Oracle::is_ancestor: bad index";
      return tin_[u] <= tin_[v] and tout_[v] <= tout_[u];
    }

    [[nodiscard]] size_t lca(size_t u, size_t v) const
    {
      ah_runtime_error_if(u >= n_ or v >= n_) << "Naive_Tree_Oracle::lca: bad index";

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

    [[nodiscard]] size_t distance(const size_t u, const size_t v) const
    {
      const size_t a = lca(u, v);
      return depth_[u] + depth_[v] - 2 * depth_[a];
    }

    [[nodiscard]] size_t kth_ancestor(const size_t u, const size_t k) const
    {
      ah_runtime_error_if(u >= n_) << "Naive_Tree_Oracle::kth_ancestor: bad index";
      if (k > depth_[u])
        return NONE;

      size_t cur = u;
      for (size_t i = 0; i < k; ++i)
        cur = parent_[cur];
      return cur;
    }

    [[nodiscard]] static size_t none() noexcept { return NONE; }
  };

  template <class GT>
  class LcaTypedTest : public ::testing::Test
  {
  };

  using GraphBackends = ::testing::Types<
      List_Graph<Graph_Node<int>, Graph_Arc<int>>,
      List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
      Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

  TYPED_TEST_SUITE(LcaTypedTest, GraphBackends);
}


TYPED_TEST(LcaTypedTest, EmptyTree)
{
  using Graph = TypeParam;

  Graph g;

  Binary_Lifting_LCA<Graph> bl(g);
  Euler_RMQ_LCA<Graph> er(g);

  EXPECT_TRUE(bl.is_empty());
  EXPECT_TRUE(er.is_empty());
  EXPECT_EQ(bl.size(), 0u);
  EXPECT_EQ(er.size(), 0u);

  EXPECT_THROW(bl.lca_id(0, 0), std::domain_error);
  EXPECT_THROW(er.lca_id(0, 0), std::domain_error);
}

TYPED_TEST(LcaTypedTest, SingleNode)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  Graph g;
  Node * n0 = g.insert_node(7);

  Binary_Lifting_LCA<Graph> bl(g, n0);
  Euler_RMQ_LCA<Graph> er(g, n0);

  EXPECT_EQ(bl.size(), 1u);
  EXPECT_EQ(er.size(), 1u);
  EXPECT_EQ(bl.root(), n0);
  EXPECT_EQ(er.root(), n0);

  EXPECT_EQ(bl.lca(n0, n0), n0);
  EXPECT_EQ(er.lca(n0, n0), n0);

  EXPECT_EQ(bl.depth_of(n0), 0u);
  EXPECT_EQ(er.depth_of(n0), 0u);
  EXPECT_EQ(bl.distance(n0, n0), 0u);
  EXPECT_EQ(er.distance(n0, n0), 0u);

  EXPECT_EQ(bl.parent_of(n0), nullptr);
  EXPECT_EQ(er.parent_of(n0), nullptr);
  EXPECT_EQ(bl.kth_ancestor(n0, 1), nullptr);
}

TYPED_TEST(LcaTypedTest, ManualTreeQueries)
{
  using Graph = TypeParam;

  // Rooted at 0:
  //           0
  //        /     (right)
  //     (left)      2
  //       1      children of 2
  //      / \      5   6
  //     3   4   children of 5
  //             7 and 8
  Array<std::pair<size_t, size_t>> edges;
  edges.reserve(8);
  edges.append(std::make_pair(0, 1));
  edges.append(std::make_pair(0, 2));
  edges.append(std::make_pair(1, 3));
  edges.append(std::make_pair(1, 4));
  edges.append(std::make_pair(2, 5));
  edges.append(std::make_pair(2, 6));
  edges.append(std::make_pair(5, 7));
  edges.append(std::make_pair(5, 8));

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 9, edges);

  Binary_Lifting_LCA<Graph> bl(g, nodes(0));
  Euler_RMQ_LCA<Graph> er(g, nodes(0));

  auto check = [&](const size_t u, const size_t v,
                   const size_t expected_lca, const size_t expected_dist)
  {
    auto * bl_lca = bl.lca(nodes(u), nodes(v));
    auto * er_lca = er.lca(nodes(u), nodes(v));

    EXPECT_EQ(static_cast<size_t>(bl_lca->get_info()), expected_lca)
        << "u=" << u << " v=" << v;
    EXPECT_EQ(static_cast<size_t>(er_lca->get_info()), expected_lca)
        << "u=" << u << " v=" << v;
    EXPECT_EQ(bl_lca, er_lca) << "u=" << u << " v=" << v;

    EXPECT_EQ(bl.distance(nodes(u), nodes(v)), expected_dist);
    EXPECT_EQ(er.distance(nodes(u), nodes(v)), expected_dist);
  };

  check(3, 4, 1, 2);
  check(3, 6, 0, 4);
  check(7, 8, 5, 2);
  check(7, 6, 2, 3);
  check(0, 8, 0, 3);

  EXPECT_EQ(bl.kth_ancestor(nodes(8), 1), nodes(5));
  EXPECT_EQ(bl.kth_ancestor(nodes(8), 2), nodes(2));
  EXPECT_EQ(bl.kth_ancestor(nodes(8), 3), nodes(0));
  EXPECT_EQ(bl.kth_ancestor(nodes(8), 4), nullptr);

  EXPECT_TRUE(bl.is_ancestor(nodes(2), nodes(7)));
  EXPECT_FALSE(bl.is_ancestor(nodes(1), nodes(7)));
  EXPECT_TRUE(er.is_ancestor(nodes(2), nodes(7)));
  EXPECT_FALSE(er.is_ancestor(nodes(1), nodes(7)));
}

TYPED_TEST(LcaTypedTest, DefaultRootIsFirstNode)
{
  using Graph = TypeParam;
  using Node = typename Graph::Node;

  Array<std::pair<size_t, size_t>> edges;
  edges.reserve(4);
  edges.append(std::make_pair(0, 1));
  edges.append(std::make_pair(1, 2));
  edges.append(std::make_pair(2, 3));
  edges.append(std::make_pair(3, 4));

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 5, edges);

  // Find the first node returned by Node_Iterator for this backend.
  // List_Graph / Array_Graph iterate in insertion order, so the first
  // node is nodes(0). List_SGraph iterates by pointer address (BST
  // in-order), so the first node may differ — this is expected behaviour.
  Node * first_node = nullptr;
  for (Node_Iterator<Graph> it(g); it.has_curr(); it.next_ne())
    {
      first_node = it.get_curr();
      break;
    }
  ASSERT_NE(first_node, nullptr);

  Binary_Lifting_LCA<Graph> bl(g);
  Euler_RMQ_LCA<Graph> er(g);

  // Both engines must choose the same default root = first iterated node.
  EXPECT_EQ(bl.root(), first_node);
  EXPECT_EQ(er.root(), first_node);

  // Use the oracle with the actual default root to derive the expected LCA.
  const size_t root_info = static_cast<size_t>(first_node->get_info());
  Naive_Tree_Oracle oracle(5, edges, root_info);

  const size_t expected_lca = oracle.lca(4, 2);
  EXPECT_EQ(static_cast<size_t>(bl.lca(nodes(4), nodes(2))->get_info()), expected_lca);
  EXPECT_EQ(static_cast<size_t>(er.lca(nodes(4), nodes(2))->get_info()), expected_lca);
}

TYPED_TEST(LcaTypedTest, RejectsCycle)
{
  using Graph = TypeParam;

  Array<std::pair<size_t, size_t>> edges;
  edges.reserve(3);
  edges.append(std::make_pair(0, 1));
  edges.append(std::make_pair(1, 2));
  edges.append(std::make_pair(2, 0));

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 3, edges);

  EXPECT_THROW((Binary_Lifting_LCA<Graph>(g, nodes(0))), std::domain_error);
  EXPECT_THROW((Euler_RMQ_LCA<Graph>(g, nodes(0))), std::domain_error);
}

TYPED_TEST(LcaTypedTest, RejectsDisconnectedGraph)
{
  using Graph = TypeParam;

  Array<std::pair<size_t, size_t>> edges;
  edges.reserve(2);
  edges.append(std::make_pair(0, 1));
  edges.append(std::make_pair(2, 3));

  Graph g;
  auto nodes = build_graph_with_unit_arcs(g, 4, edges);

  EXPECT_THROW((Binary_Lifting_LCA<Graph>(g, nodes(0))), std::domain_error);
  EXPECT_THROW((Euler_RMQ_LCA<Graph>(g, nodes(0))), std::domain_error);
}

TYPED_TEST(LcaTypedTest, RejectsLoopAndParallelEdges)
{
  using Graph = TypeParam;

  {
    Graph g;
    Array<std::pair<size_t, size_t>> edges;
    edges.reserve(3);
    edges.append(std::make_pair(0, 1));
    edges.append(std::make_pair(1, 2));
    edges.append(std::make_pair(2, 2));
    auto nodes = build_graph_with_unit_arcs(g, 3, edges);
    EXPECT_THROW((Binary_Lifting_LCA<Graph>(g, nodes(0))), std::domain_error);
    EXPECT_THROW((Euler_RMQ_LCA<Graph>(g, nodes(0))), std::domain_error);
  }

  {
    Graph g;
    Array<std::pair<size_t, size_t>> edges;
    edges.reserve(2);
    edges.append(std::make_pair(0, 1));
    edges.append(std::make_pair(0, 1));
    auto nodes = build_graph_with_unit_arcs(g, 2, edges);
    EXPECT_THROW((Binary_Lifting_LCA<Graph>(g, nodes(0))), std::domain_error);
    EXPECT_THROW((Euler_RMQ_LCA<Graph>(g, nodes(0))), std::domain_error);
  }
}

TYPED_TEST(LcaTypedTest, ArcFilterSelectsTree)
{
  using Graph = TypeParam;

  Graph g;
  auto nodes = Array<typename Graph::Node *>::create(6);
  for (size_t i = 0; i < 6; ++i)
    nodes(i) = g.insert_node(static_cast<int>(i));

  // Tree edges (kept by filter).
  g.insert_arc(nodes(0), nodes(1), 1);
  g.insert_arc(nodes(0), nodes(2), 1);
  g.insert_arc(nodes(1), nodes(3), 1);
  g.insert_arc(nodes(1), nodes(4), 1);
  g.insert_arc(nodes(2), nodes(5), 1);

  // Extra non-tree edges (ignored by filter).
  g.insert_arc(nodes(3), nodes(5), -1);
  g.insert_arc(nodes(4), nodes(2), -1);

  EXPECT_THROW((Binary_Lifting_LCA<Graph>(g, nodes(0))), std::domain_error);
  EXPECT_THROW((Euler_RMQ_LCA<Graph>(g, nodes(0))), std::domain_error);

  Binary_Lifting_LCA<Graph, Positive_Arc_Filter> bl(g, nodes(0), Positive_Arc_Filter());
  Euler_RMQ_LCA<Graph, Positive_Arc_Filter> er(g, nodes(0), Positive_Arc_Filter());

  EXPECT_EQ(static_cast<size_t>(bl.lca(nodes(3), nodes(4))->get_info()), 1u);
  EXPECT_EQ(static_cast<size_t>(er.lca(nodes(3), nodes(4))->get_info()), 1u);
  EXPECT_EQ(static_cast<size_t>(bl.lca(nodes(3), nodes(5))->get_info()), 0u);
  EXPECT_EQ(static_cast<size_t>(er.lca(nodes(3), nodes(5))->get_info()), 0u);
}

TYPED_TEST(LcaTypedTest, RejectsNodeFromAnotherGraphAndBadIds)
{
  using Graph = TypeParam;

  Graph g1;
  Array<std::pair<size_t, size_t>> edges1;
  edges1.reserve(2);
  edges1.append(std::make_pair(0, 1));
  edges1.append(std::make_pair(1, 2));
  auto n1 = build_graph_with_unit_arcs(g1, 3, edges1);

  Graph g2;
  Array<std::pair<size_t, size_t>> edges2;
  edges2.reserve(1);
  edges2.append(std::make_pair(0, 1));
  auto n2 = build_graph_with_unit_arcs(g2, 2, edges2);

  Binary_Lifting_LCA<Graph> bl(g1, n1(0));
  Euler_RMQ_LCA<Graph> er(g1, n1(0));

  EXPECT_THROW(bl.lca(n1(0), n2(0)), std::domain_error);
  EXPECT_THROW(er.lca(n1(0), n2(0)), std::domain_error);
  EXPECT_THROW(bl.depth_of(n2(0)), std::domain_error);
  EXPECT_THROW(er.depth_of(n2(0)), std::domain_error);

  EXPECT_THROW(bl.lca_id(0, 99), std::out_of_range);
  EXPECT_THROW(er.lca_id(0, 99), std::out_of_range);
  EXPECT_THROW(bl.node_of(99), std::out_of_range);
  EXPECT_THROW(er.node_of(99), std::out_of_range);
}

TYPED_TEST(LcaTypedTest, RandomTreesAgainstNaiveOracle)
{
  using Graph = TypeParam;

  std::mt19937 rng(0x1ca2026u);

  for (size_t sample = 0; sample < 24; ++sample)
    {
      std::uniform_int_distribution<size_t> n_pick(2, 90);
      const size_t n = n_pick(rng);
      const auto edges = make_random_tree_edges<Array<std::pair<size_t, size_t>>>(n, rng);

      Graph g;
      auto nodes = build_graph_with_unit_arcs(g, n, edges);

      std::uniform_int_distribution<size_t> root_pick(0, n - 1);
      const size_t root = root_pick(rng);

      Binary_Lifting_LCA<Graph> bl(g, nodes(root));
      Euler_RMQ_LCA<Graph> er(g, nodes(root));
      Naive_Tree_Oracle oracle(n, edges, root);

      for (size_t u = 0; u < n; ++u)
        {
          EXPECT_EQ(bl.depth_of(nodes(u)), oracle.depth(u));
          EXPECT_EQ(er.depth_of(nodes(u)), oracle.depth(u));
          EXPECT_EQ(bl.node_of(bl.id_of(nodes(u))), nodes(u));
          EXPECT_EQ(er.node_of(er.id_of(nodes(u))), nodes(u));

          std::uniform_int_distribution<size_t> k_pick(0, oracle.depth(u) + 2);
          const size_t k = k_pick(rng);
          const size_t expected = oracle.kth_ancestor(u, k);
          auto * got = bl.kth_ancestor(nodes(u), k);
          if (expected == Naive_Tree_Oracle::none())
            EXPECT_EQ(got, nullptr);
          else
            EXPECT_EQ(static_cast<size_t>(got->get_info()), expected);
        }

      std::uniform_int_distribution<size_t> node_pick(0, n - 1);
      for (size_t q = 0; q < 450; ++q)
        {
          const size_t u = node_pick(rng);
          const size_t v = node_pick(rng);

          const size_t expected_lca = oracle.lca(u, v);
          const size_t expected_dist = oracle.distance(u, v);

          auto * bl_lca = bl.lca(nodes(u), nodes(v));
          auto * er_lca = er.lca(nodes(u), nodes(v));

          EXPECT_EQ(static_cast<size_t>(bl_lca->get_info()), expected_lca)
              << "sample=" << sample;
          EXPECT_EQ(static_cast<size_t>(er_lca->get_info()), expected_lca)
              << "sample=" << sample;
          EXPECT_EQ(bl_lca, er_lca) << "sample=" << sample;

          EXPECT_EQ(bl.distance(nodes(u), nodes(v)), expected_dist);
          EXPECT_EQ(er.distance(nodes(u), nodes(v)), expected_dist);

          EXPECT_EQ(bl.is_ancestor(nodes(expected_lca), nodes(u)),
                    oracle.is_ancestor(expected_lca, u));
          EXPECT_EQ(er.is_ancestor(nodes(expected_lca), nodes(v)),
                    oracle.is_ancestor(expected_lca, v));
        }
    }
}

TYPED_TEST(LcaTypedTest, PerfRegression)
{
  using Graph = TypeParam;
  using Clock = std::chrono::steady_clock;

# ifndef LCA_PERF_TEST
  const char * run_perf = std::getenv("ALEPH_RUN_LCA_PERF");
  if (run_perf == nullptr or run_perf[0] != '1' or run_perf[1] != '\0')
    GTEST_SKIP() << "LCA perf test disabled by default. "
                 << "Enable with -DLCA_PERF_TEST or ALEPH_RUN_LCA_PERF=1";
# endif

  constexpr size_t n = 100000;
  constexpr size_t query_count = 300000;
# ifdef NDEBUG
  constexpr long long kBudgetMs = 12000;
# else
  constexpr long long kBudgetMs = 25000;
# endif

  std::mt19937 rng(0x1ca2026u);
  const auto edges = make_random_tree_edges<Array<std::pair<size_t, size_t>>>(n, rng);

  Graph g;
  const auto nodes = build_graph_with_unit_arcs(g, n, edges);

  std::uniform_int_distribution<size_t> root_pick(0, n - 1);
  const size_t root = root_pick(rng);

  const auto t_start = Clock::now();
  Binary_Lifting_LCA<Graph> bl(g, nodes(root));
  Euler_RMQ_LCA<Graph> er(g, nodes(root));
  const auto t_after_build = Clock::now();

  // Keep an independent sanity check without impacting timed query loop.
  Naive_Tree_Oracle oracle(n, edges, root);
  std::uniform_int_distribution<size_t> sanity_pick(0, n - 1);
  for (size_t i = 0; i < 64; ++i)
    {
      const size_t u = sanity_pick(rng);
      const size_t v = sanity_pick(rng);
      const size_t expected_lca = oracle.lca(u, v);
      const size_t expected_dist = oracle.distance(u, v);

      ASSERT_EQ(static_cast<size_t>(bl.lca(nodes(u), nodes(v))->get_info()),
                expected_lca);
      ASSERT_EQ(static_cast<size_t>(er.lca(nodes(u), nodes(v))->get_info()),
                expected_lca);
      ASSERT_EQ(bl.distance(nodes(u), nodes(v)), expected_dist);
      ASSERT_EQ(er.distance(nodes(u), nodes(v)), expected_dist);
    }

  // Reuse the same node-picking pattern as RandomTreesAgainstNaiveOracle.
  std::uniform_int_distribution<size_t> node_pick(0, n - 1);
  size_t checksum = 0;
  for (size_t q = 0; q < query_count; ++q)
    {
      const size_t u = node_pick(rng);
      const size_t v = node_pick(rng);

      auto * bl_lca = bl.lca(nodes(u), nodes(v));
      auto * er_lca = er.lca(nodes(u), nodes(v));
      ASSERT_EQ(bl_lca, er_lca);

      const size_t d1 = bl.distance(nodes(u), nodes(v));
      const size_t d2 = er.distance(nodes(u), nodes(v));
      ASSERT_EQ(d1, d2);

      checksum ^= (static_cast<size_t>(bl_lca->get_info()) + 0x9e3779b9u);
      checksum ^= (d1 + (q << 1));
    }

  const auto t_end = Clock::now();

  const auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      t_after_build - t_start).count();
  const auto query_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      t_end - t_after_build).count();
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      t_end - t_start).count();

  ASSERT_NE(checksum, 0u);
  ASSERT_LT(elapsed_ms, kBudgetMs)
      << "Perf regression: n=" << n
      << ", queries=" << query_count
      << ", build_ms=" << build_ms
      << ", query_ms=" << query_ms
      << ", elapsed_ms=" << elapsed_ms
      << ", budget_ms=" << kBudgetMs;
}
