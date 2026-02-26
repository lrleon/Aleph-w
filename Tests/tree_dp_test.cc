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
 * @file tree_dp_test.cc
 * @brief Tests for Tree_DP.H.
 */

# include <gtest/gtest.h>

# include <limits>
# include <random>
# include <utility>

# include <Tree_DP.H>

using namespace Aleph;

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

namespace
{
  Array<size_t>
  brute_sum_distances(const size_t n,
                      const Array<std::pair<size_t, size_t>> & edges)
  {
    Array<Array<size_t>> adj;
    adj.reserve(n);
    for (size_t i = 0; i < n; ++i)
      adj.append(Array<size_t>());

    for (size_t i = 0; i < edges.size(); ++i)
      {
        const size_t u = edges[i].first;
        const size_t v = edges[i].second;
        adj(u).append(v);
        adj(v).append(u);
      }

    Array<size_t> sums = Array<size_t>::create(n);
    for (size_t src = 0; src < n; ++src)
      {
        Array<size_t> dist = Array<size_t>::create(n);
        for (size_t i = 0; i < n; ++i)
          dist(i) = std::numeric_limits<size_t>::max();

        Array<size_t> q;
        q.append(src);
        dist(src) = 0;

        for (size_t head = 0; head < q.size(); ++head)
          {
            const size_t u = q[head];
            for (size_t j = 0; j < adj[u].size(); ++j)
              {
                const size_t v = adj[u][j];
                if (dist[v] != std::numeric_limits<size_t>::max())
                  continue;
                dist(v) = dist[u] + 1;
                q.append(v);
              }
          }

        size_t sum = 0;
        for (size_t i = 0; i < n; ++i)
          sum += dist[i];
        sums(src) = sum;
      }
    return sums;
  }
} // namespace


// Helper to build a path tree: 0 -- 1 -- 2 -- ... -- (n-1)
static G build_path(size_t n, Array<G::Node *> & nodes)
{
  G g;
  nodes = Array<G::Node *>::create(n);
  for (size_t i = 0; i < n; ++i)
    nodes(i) = g.insert_node(static_cast<int>(i));
  for (size_t i = 0; i + 1 < n; ++i)
    g.insert_arc(nodes(i), nodes(i + 1), 1);
  return g;
}

// Helper to build a star tree: center=0, leaves=1..n-1
static G build_star(size_t n, Array<G::Node *> & nodes)
{
  G g;
  nodes = Array<G::Node *>::create(n);
  for (size_t i = 0; i < n; ++i)
    nodes(i) = g.insert_node(static_cast<int>(i));
  for (size_t i = 1; i < n; ++i)
    g.insert_arc(nodes(0), nodes(i), 1);
  return g;
}


// ── Subtree sizes ─────────────────────────────────────────────────

TEST(TreeDP, SubtreeSizes_Path)
{
  Array<G::Node *> nodes;
  auto g = build_path(5, nodes);
  // Root at node 0: 0-1-2-3-4
  // subtree(0) = 5, subtree(1) = 4, ..., subtree(4) = 1
  auto sizes = tree_subtree_sizes(g, nodes(0));
  EXPECT_EQ(sizes.size(), 5u);

  // Find the ids. Use Tree_DP to get the mapping.
  Gen_Tree_DP<G, size_t> dp(g, nodes(0),
    [](auto *) -> size_t { return 1; },
    [](auto *, const size_t & a, auto *, const size_t & c) -> size_t {
      return a + c;
    });

  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(dp.value(nodes(i)), 5 - i)
      << "Subtree of node " << i;
}

TEST(TreeDP, SubtreeSizes_Star)
{
  Array<G::Node *> nodes;
  auto g = build_star(5, nodes);
  // Root at center: subtree(center) = 5, subtree(leaf) = 1
  Gen_Tree_DP<G, size_t> dp(g, nodes(0),
    [](auto *) -> size_t { return 1; },
    [](auto *, const size_t & a, auto *, const size_t & c) -> size_t {
      return a + c;
    });

  EXPECT_EQ(dp.value(nodes(0)), 5u);
  for (size_t i = 1; i < 5; ++i)
    EXPECT_EQ(dp.value(nodes(i)), 1u);
}

TEST(TreeDP, SubtreeSizes_SingleNode)
{
  G g;
  auto n = g.insert_node(0);
  auto sizes = tree_subtree_sizes(g, n);
  EXPECT_EQ(sizes.size(), 1u);
  EXPECT_EQ(sizes[0], 1u);
}


// ── Max distance (eccentricity via rerooting) ─────────────────────

TEST(TreeDP, MaxDistance_Path)
{
  Array<G::Node *> nodes;
  auto g = build_path(5, nodes);
  // Path 0-1-2-3-4: max distance from each node
  // node 0: max dist = 4
  // node 1: max dist = 3
  // node 2: max dist = 2
  // node 3: max dist = 3
  // node 4: max dist = 4
  auto dists = tree_max_distance(g, nodes(0));

  Gen_Reroot_DP<G, size_t> dp(g, nodes(0),
    size_t(0),
    [](auto *) -> size_t { return 0; },
    [](const size_t & a, const size_t & b) -> size_t {
      return std::max(a, b);
    },
    [](auto *, auto *, const size_t & v) -> size_t { return v + 1; });

  EXPECT_EQ(dp.value(nodes(0)), 4u);
  EXPECT_EQ(dp.value(nodes(1)), 3u);
  EXPECT_EQ(dp.value(nodes(2)), 2u);
  EXPECT_EQ(dp.value(nodes(3)), 3u);
  EXPECT_EQ(dp.value(nodes(4)), 4u);
}

TEST(TreeDP, MaxDistance_Star)
{
  Array<G::Node *> nodes;
  auto g = build_star(5, nodes);
  // Star with 5 nodes: center distance = 1, leaves distance = 2
  Gen_Reroot_DP<G, size_t> dp(g, nodes(0),
    size_t(0),
    [](auto *) -> size_t { return 0; },
    [](const size_t & a, const size_t & b) -> size_t {
      return std::max(a, b);
    },
    [](auto *, auto *, const size_t & v) -> size_t { return v + 1; });

  EXPECT_EQ(dp.value(nodes(0)), 1u);
  for (size_t i = 1; i < 5; ++i)
    EXPECT_EQ(dp.value(nodes(i)), 2u);
}


// ── Sum of distances (rerooting) ──────────────────────────────────

TEST(TreeDP, SumOfDistances_Path)
{
  Array<G::Node *> nodes;
  auto g = build_path(4, nodes);
  // Path 0-1-2-3
  // sum_dist(0) = 0+1+2+3 = 6
  // sum_dist(1) = 1+0+1+2 = 4
  // sum_dist(2) = 2+1+0+1 = 4
  // sum_dist(3) = 3+2+1+0 = 6
  auto dists = tree_sum_of_distances(g, nodes(0));

  using P = std::pair<size_t, size_t>;
  Gen_Reroot_DP<G, P> dp(g, nodes(0),
    P{0, 0},
    [](auto *) -> P { return {1, 0}; },
    [](const P & a, const P & b) -> P {
      return {a.first + b.first, a.second + b.second};
    },
    [](auto *, auto *, const P & v) -> P {
      return {v.first, v.second + v.first};
    });

  EXPECT_EQ(dp.value(nodes(0)).second, 6u);
  EXPECT_EQ(dp.value(nodes(1)).second, 4u);
  EXPECT_EQ(dp.value(nodes(2)).second, 4u);
  EXPECT_EQ(dp.value(nodes(3)).second, 6u);
}


// ── Error handling ────────────────────────────────────────────────

TEST(TreeDP, NotATree)
{
  // Create a graph with a cycle
  G g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n0, 1); // creates cycle

  EXPECT_THROW(tree_subtree_sizes(g, n0), std::domain_error);
}

TEST(TreeDP, DisconnectedGraph)
{
  G g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n2, n3, 1);
  EXPECT_THROW(tree_subtree_sizes(g, n0), std::domain_error);
}

TEST(TreeDP, RootNotInGraph)
{
  G g1;
  auto r1 = g1.insert_node(1);
  (void) r1;

  G g2;
  auto external_root = g2.insert_node(42);
  EXPECT_THROW(tree_subtree_sizes(g1, external_root), std::domain_error);
}

TEST(TreeDP, ParallelEdgesAreIgnored)
{
  G g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n1, 1); // duplicate edge
  g.insert_arc(n1, n2, 1);

  Gen_Tree_DP<G, size_t> dp(g, n0,
    [](auto *) -> size_t { return 1; },
    [](auto *, const size_t & a, auto *, const size_t & c) -> size_t {
      return a + c;
    });

  EXPECT_EQ(dp.value(n0), 3u);
  EXPECT_EQ(dp.value(n1), 2u);
  EXPECT_EQ(dp.value(n2), 1u);
}

TEST(TreeDP, BinaryTree)
{
  //       0
  //      / |
  //     1   2
  //    / |
  //   3   4
  G g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n1, n4, 1);

  Gen_Tree_DP<G, size_t> dp(g, n0,
    [](auto *) -> size_t { return 1; },
    [](auto *, const size_t & a, auto *, const size_t & c) -> size_t {
      return a + c;
    });

  EXPECT_EQ(dp.value(n0), 5u);
  EXPECT_EQ(dp.value(n1), 3u);
  EXPECT_EQ(dp.value(n2), 1u);
  EXPECT_EQ(dp.value(n3), 1u);
  EXPECT_EQ(dp.value(n4), 1u);
}

TEST(TreeDP, RandomSumOfDistancesVsBfs)
{
  std::mt19937 rng(20260226);

  for (int trial = 0; trial < 80; ++trial)
    {
      const size_t n = 2 + rng() % 40;
      G g;
      Array<G::Node *> nodes = Array<G::Node *>::create(n);
      for (size_t i = 0; i < n; ++i)
        nodes(i) = g.insert_node(static_cast<int>(i));

      Array<std::pair<size_t, size_t>> edges;
      edges.reserve(n - 1);
      for (size_t i = 1; i < n; ++i)
        {
          const size_t p = static_cast<size_t>(rng() % i);
          g.insert_arc(nodes(p), nodes(i), 1);
          edges.append(std::make_pair(p, i));
        }

      const auto brute = brute_sum_distances(n, edges);
      const auto fast = tree_sum_of_distances(g, nodes(0));

      Gen_Tree_DP<G, size_t> map(g, nodes(0),
        [](auto *) -> size_t { return 1; },
        [](auto *, const size_t & a, auto *, const size_t & c) -> size_t {
          return a + c;
        });

      for (size_t i = 0; i < n; ++i)
        {
          const size_t id = map.id_of(nodes(i));
          EXPECT_EQ(fast[id], brute[i])
            << "trial=" << trial << " node=" << i;
        }
    }
}
