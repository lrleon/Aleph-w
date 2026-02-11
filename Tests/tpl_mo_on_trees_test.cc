
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
 * @file tpl_mo_on_trees_test.cc
 * @brief Tests for Gen_Mo_On_Trees and built-in policies on all
 *        three graph types (List_Graph, List_SGraph, Array_Graph).
 */

# include <gtest/gtest.h>

# include <tpl_mo_on_trees.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>
# include <tpl_dynSetHash.H>

# include <algorithm>
# include <cstddef>
# include <random>
# include <utility>
# include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  // ── Brute-force helpers ────────────────────────────────────────

  // Compute parent map via DFS from tree_root
  template <class GT>
  DynMapHash<typename GT::Node*, typename GT::Node*>
  compute_parents(const GT & g, typename GT::Node * tree_root)
  {
    using Node = typename GT::Node;
    DynMapHash<Node*, Node*> parent;
    parent.insert(tree_root, nullptr);
    DynListStack<std::pair<Node*, Node*>> stk;
    stk.push({tree_root, nullptr});
    while (not stk.is_empty())
      {
        auto [cur, par] = stk.pop();
        for (auto it = typename GT::Node_Arc_Iterator(cur);
             it.has_curr(); it.next_ne())
          {
            auto * a = it.get_curr();
            auto * nb = g.get_connected_node(a, cur);
            if (nb != par)
              {
                parent.insert(nb, cur);
                stk.push({nb, cur});
              }
          }
      }
    return parent;
  }

  // Brute-force distinct count over subtree of subtree_root
  template <class GT>
  size_t brute_subtree_distinct(
      const GT & g,
      DynMapHash<typename GT::Node*,
                 typename GT::Node*> & parent,
      typename GT::Node * subtree_root)
  {
    using Node = typename GT::Node;
    DynSetHash<typename Node::Node_Type> seen;
    DynListStack<Node*> stk;
    stk.push(subtree_root);
    while (not stk.is_empty())
      {
        auto * cur = stk.pop();
        seen.insert(cur->get_info());
        for (auto it = typename GT::Node_Arc_Iterator(cur);
             it.has_curr(); it.next_ne())
          {
            auto * a = it.get_curr();
            auto * nb = g.get_connected_node(a, cur);
            if (nb != parent.find(cur))
              stk.push(nb);
          }
      }
    return seen.size();
  }

  // Brute-force distinct count on path u→v
  template <class GT>
  size_t brute_path_distinct(
      const GT & g,
      DynMapHash<typename GT::Node*,
                 typename GT::Node*> & parent,
      typename GT::Node * u, typename GT::Node * v)
  {
    (void) g;
    using Node = typename GT::Node;
    // Collect ancestors of u
    DynSetHash<Node*> anc_u;
    for (auto * p = u; p; p = parent.find(p))
      anc_u.insert(p);
    // Find LCA
    Node * lca_node = v;
    while (!anc_u.has(lca_node))
      lca_node = parent.find(lca_node);
    // Collect values on path
    DynSetHash<typename Node::Node_Type> vals;
    for (auto * p = u; p != lca_node; p = parent.find(p))
      vals.insert(p->get_info());
    for (auto * p = v; p != lca_node; p = parent.find(p))
      vals.insert(p->get_info());
    vals.insert(lca_node->get_info());
    return vals.size();
  }

  // ── Tree builder helper ────────────────────────────────────────

  // Build a random tree with n nodes, values in [0, max_val)
  // Returns (graph, root, array_of_all_nodes)
  template <class GT>
  struct TreeInfo
  {
    GT g;
    typename GT::Node * root;
    Array<typename GT::Node *> nodes;
  };

  template <class GT>
  TreeInfo<GT> build_random_tree(size_t n, int max_val, std::mt19937 & rng)
  {
    TreeInfo<GT> info;
    if (n == 0)
      {
        info.root = nullptr;
        return info;
      }

    info.nodes = Array<typename GT::Node *>::create(n);
    for (size_t i = 0; i < n; ++i)
      info.nodes[i] = info.g.insert_node(static_cast<int>(rng() % max_val));

    // Build a random tree: for each node i>0, pick a random parent
    // from [0, i)
    for (size_t i = 1; i < n; ++i)
      {
        size_t parent_idx = rng() % i;
        info.g.insert_arc(info.nodes[parent_idx], info.nodes[i]);
      }

    info.root = info.nodes[0];
    return info;
  }

  // ── Path-based chain builder (for deterministic tests) ─────────

  //   0 — 1 — 2 — ... — (n-1)
  //   values: v[i]
  template <class GT>
  TreeInfo<GT> build_chain(const Array<int> & vals)
  {
    TreeInfo<GT> info;
    info.nodes = Array<typename GT::Node *>::create(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      info.nodes[i] = info.g.insert_node(vals[i]);

    for (size_t i = 1; i < vals.size(); ++i)
      info.g.insert_arc(info.nodes[i-1], info.nodes[i]);
    info.root = info.nodes.is_empty() ? nullptr : info.nodes[0];
    return info;
  }

} // namespace

// =================================================================
// Structural tests
// =================================================================

TEST(MoOnTrees, EmptyGraph)
{
  List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>> g;
  EXPECT_EQ(g.vsize(), 0u);
  EXPECT_EQ(g.esize(), 0u);
  GTEST_SKIP() << "Gen_Mo_On_Trees requires a non-null root node; "
               << "empty-graph constructor semantics are undefined.";
}

TEST(MoOnTrees, SingleNode)
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;
  auto * r = g.insert_node(42);
  Distinct_Count_Mo_On_Trees<G> mot(g, r);

  EXPECT_EQ(mot.size(), 1u);
  EXPECT_FALSE(mot.is_empty());

  auto sub = mot.subtree_solve({r});
  EXPECT_EQ(sub(0), 1u);

  auto path = mot.path_solve({{r, r}});
  EXPECT_EQ(path(0), 1u);
}

TEST(MoOnTrees, TwoNodes)
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;
  auto * a = g.insert_node(10);
  auto * b = g.insert_node(10);
  g.insert_arc(a, b);

  Distinct_Count_Mo_On_Trees<G> mot(g, a);

  auto sub = mot.subtree_solve({a, b});
  EXPECT_EQ(sub(0), 1u); // subtree(a) = {10, 10} → 1 distinct
  EXPECT_EQ(sub(1), 1u); // subtree(b) = {10} → 1 distinct

  auto path = mot.path_solve({{a, b}});
  EXPECT_EQ(path(0), 1u); // path a→b = {10, 10} → 1 distinct
}

TEST(MoOnTrees, TwoNodesDifferentValues)
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;
  auto * a = g.insert_node(10);
  auto * b = g.insert_node(20);
  g.insert_arc(a, b);

  Distinct_Count_Mo_On_Trees<G> mot(g, a);

  auto sub = mot.subtree_solve({a, b});
  EXPECT_EQ(sub(0), 2u);
  EXPECT_EQ(sub(1), 1u);

  auto path = mot.path_solve({{a, b}, {a, a}, {b, b}});
  EXPECT_EQ(path(0), 2u);
  EXPECT_EQ(path(1), 1u);
  EXPECT_EQ(path(2), 1u);
}

TEST(MoOnTrees, InvalidNodeThrows)
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;
  auto * a = g.insert_node(1);
  auto * b = g.insert_node(2);
  g.insert_arc(a, b);

  // Create a node NOT in the graph
  G g2;
  auto * alien = g2.insert_node(99);

  Distinct_Count_Mo_On_Trees<G> mot(g, a);

  EXPECT_THROW(mot.subtree_solve({alien}), std::domain_error);
  EXPECT_THROW(mot.path_solve({{a, alien}}), std::domain_error);
}

// =================================================================
// Subtree query tests — List_Graph
// =================================================================

TEST(MoOnTreesSubtree, SmallTreeListGraph)
{
  /*      1 (root)
   *     / \
   *    2   1
   *   / \
   *  3   2
   */
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;
  auto * r  = g.insert_node(1);
  auto * a  = g.insert_node(2);
  auto * b  = g.insert_node(1);
  auto * c  = g.insert_node(3);
  auto * d  = g.insert_node(2);
  g.insert_arc(r, a);
  g.insert_arc(r, b);
  g.insert_arc(a, c);
  g.insert_arc(a, d);

  auto parent = compute_parents(g, r);

  Distinct_Count_Mo_On_Trees<G> mot(g, r);
  auto ans = mot.subtree_solve({r, a, b, c, d});

  EXPECT_EQ(ans(0), brute_subtree_distinct(g, parent, r));
  EXPECT_EQ(ans(1), brute_subtree_distinct(g, parent, a));
  EXPECT_EQ(ans(2), brute_subtree_distinct(g, parent, b));
  EXPECT_EQ(ans(3), brute_subtree_distinct(g, parent, c));
  EXPECT_EQ(ans(4), brute_subtree_distinct(g, parent, d));

  // Manual verification
  EXPECT_EQ(ans(0), 3u); // {1,2,3}
  EXPECT_EQ(ans(1), 2u); // {2,3}
  EXPECT_EQ(ans(2), 1u); // {1}
  EXPECT_EQ(ans(3), 1u); // {3}
  EXPECT_EQ(ans(4), 1u); // {2}
}

// =================================================================
// Path query tests — List_SGraph
// =================================================================

TEST(MoOnTreesPath, ChainListSGraph)
{
  // Chain: 1 — 2 — 3 — 4 — 5
  using G = List_SGraph<Graph_Snode<int>, Graph_Sarc<Empty_Class>>;
  auto info = build_chain<G>({1, 2, 3, 4, 5});
  auto & g = info.g;
  auto & n = info.nodes;
  auto parent = compute_parents(g, n[0]);

  Distinct_Count_Mo_On_Trees<G> mot(g, n[0]);

  auto ans = mot.path_solve({
      {n[0], n[4]},   // 1→2→3→4→5, 5 distinct
      {n[1], n[3]},   // 2→3→4, 3 distinct
      {n[0], n[0]},   // just 1, 1 distinct
      {n[2], n[4]}    // 3→4→5, 3 distinct
  });

  EXPECT_EQ(ans(0), 5u);
  EXPECT_EQ(ans(1), 3u);
  EXPECT_EQ(ans(2), 1u);
  EXPECT_EQ(ans(3), 3u);

  for (size_t i = 0; i < 4; ++i)
    {
      auto [u, v] = std::pair{
        i == 0 ? n[0] : i == 1 ? n[1] : i == 2 ? n[0] : n[2],
        i == 0 ? n[4] : i == 1 ? n[3] : i == 2 ? n[0] : n[4]};
      EXPECT_EQ(ans(i), brute_path_distinct(g, parent, u, v))
        << "query " << i;
    }
}

TEST(MoOnTreesPath, StarListSGraph)
{
  // Star:  center = 1, leaves = 2, 2, 3, 3, 4
  using G = List_SGraph<Graph_Snode<int>, Graph_Sarc<Empty_Class>>;
  G g;
  auto * center = g.insert_node(1);
  auto * l1 = g.insert_node(2);
  auto * l2 = g.insert_node(2);
  auto * l3 = g.insert_node(3);
  auto * l4 = g.insert_node(3);
  auto * l5 = g.insert_node(4);
  g.insert_arc(center, l1);
  g.insert_arc(center, l2);
  g.insert_arc(center, l3);
  g.insert_arc(center, l4);
  g.insert_arc(center, l5);

  auto parent = compute_parents(g, center);
  Distinct_Count_Mo_On_Trees<G> mot(g, center);

  auto ans = mot.path_solve({
      {l1, l2},       // 2→1→2: {1,2} = 2
      {l1, l5},       // 2→1→4: {1,2,4} = 3
      {l3, l4},       // 3→1→3: {1,3} = 2
      {center, l5}    // 1→4: {1,4} = 2
  });

  EXPECT_EQ(ans(0), brute_path_distinct(g, parent, l1, l2));
  EXPECT_EQ(ans(1), brute_path_distinct(g, parent, l1, l5));
  EXPECT_EQ(ans(2), brute_path_distinct(g, parent, l3, l4));
  EXPECT_EQ(ans(3), brute_path_distinct(g, parent, center, l5));
}

// =================================================================
// Array_Graph tests
// =================================================================

TEST(MoOnTreesSubtree, SmallTreeArrayGraph)
{
  /*      10 (root)
   *     /    \
   *   20      30
   *  / \
   * 10  20
   */
  using G = Array_Graph<Graph_Anode<int>, Graph_Aarc<Empty_Class>>;
  G g;
  auto * r  = g.insert_node(10);
  auto * a  = g.insert_node(20);
  auto * b  = g.insert_node(30);
  auto * c  = g.insert_node(10);
  auto * d  = g.insert_node(20);
  g.insert_arc(r, a);
  g.insert_arc(r, b);
  g.insert_arc(a, c);
  g.insert_arc(a, d);

  auto parent = compute_parents(g, r);

  Distinct_Count_Mo_On_Trees<G> mot(g, r);
  auto sub = mot.subtree_solve({r, a, b});
  EXPECT_EQ(sub(0), brute_subtree_distinct(g, parent, r));
  EXPECT_EQ(sub(1), brute_subtree_distinct(g, parent, a));
  EXPECT_EQ(sub(2), brute_subtree_distinct(g, parent, b));

  EXPECT_EQ(sub(0), 3u); // {10,20,30}
  EXPECT_EQ(sub(1), 2u); // {10,20}
  EXPECT_EQ(sub(2), 1u); // {30}

  auto path = mot.path_solve({{c, d}, {c, b}});
  EXPECT_EQ(path(0), brute_path_distinct(g, parent, c, d));
  EXPECT_EQ(path(1), brute_path_distinct(g, parent, c, b));
}

// =================================================================
// Stress tests (random trees, brute-force verification)
// =================================================================

template <class GT>
class MoOnTreesStress : public Test {};

using GraphTypes = Types<
    List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>,
    List_SGraph<Graph_Snode<int>, Graph_Sarc<Empty_Class>>,
    Array_Graph<Graph_Anode<int>, Graph_Aarc<Empty_Class>>
>;

TYPED_TEST_SUITE(MoOnTreesStress, GraphTypes);

TYPED_TEST(MoOnTreesStress, SubtreeRandomSmall)
{
  using G = TypeParam;
  std::mt19937 rng(123);
  auto info = build_random_tree<G>(50, 10, rng);
  auto & g = info.g;
  auto parent = compute_parents(g, info.root);

  Distinct_Count_Mo_On_Trees<G> mot(g, info.root);

  // Query all nodes
  auto queries = Array<typename G::Node*>::create(info.nodes.size());
  for (size_t i = 0; i < info.nodes.size(); ++i)
    queries(i) = info.nodes[i];

  auto ans = mot.subtree_solve(queries);

  for (size_t i = 0; i < info.nodes.size(); ++i)
    EXPECT_EQ(ans(i), brute_subtree_distinct(g, parent, info.nodes[i]))
      << "node " << i;
}

TYPED_TEST(MoOnTreesStress, PathRandomSmall)
{
  using G = TypeParam;
  std::mt19937 rng(456);
  auto info = build_random_tree<G>(50, 10, rng);
  auto & g = info.g;
  auto parent = compute_parents(g, info.root);
  const size_t n = info.nodes.size();

  Distinct_Count_Mo_On_Trees<G> mot(g, info.root);

  // Generate 200 random path queries
  const size_t Q = 200;
  auto queries = Array<std::pair<typename G::Node*,
                                 typename G::Node*>>::create(Q);
  for (size_t i = 0; i < Q; ++i)
    queries(i) = {info.nodes[rng() % n], info.nodes[rng() % n]};

  auto ans = mot.path_solve(queries);

  for (size_t i = 0; i < Q; ++i)
    EXPECT_EQ(ans(i), brute_path_distinct(g, parent,
                                          queries(i).first,
                                          queries(i).second))
      << "query " << i;
}

TYPED_TEST(MoOnTreesStress, SubtreeRandomMedium)
{
  using G = TypeParam;
  std::mt19937 rng(789);
  auto info = build_random_tree<G>(500, 20, rng);
  auto & g = info.g;
  auto parent = compute_parents(g, info.root);
  const size_t n = info.nodes.size();

  Distinct_Count_Mo_On_Trees<G> mot(g, info.root);

  const size_t Q = 500;
  auto queries = Array<typename G::Node*>::create(Q);
  for (size_t i = 0; i < Q; ++i)
    queries(i) = info.nodes[rng() % n];

  auto ans = mot.subtree_solve(queries);

  for (size_t i = 0; i < Q; ++i)
    EXPECT_EQ(ans(i), brute_subtree_distinct(g, parent, queries(i)))
      << "query " << i;
}

TYPED_TEST(MoOnTreesStress, PathRandomMedium)
{
  using G = TypeParam;
  std::mt19937 rng(101112);
  auto info = build_random_tree<G>(500, 20, rng);
  auto & g = info.g;
  auto parent = compute_parents(g, info.root);
  const size_t n = info.nodes.size();

  Distinct_Count_Mo_On_Trees<G> mot(g, info.root);

  const size_t Q = 1000;
  auto queries = Array<std::pair<typename G::Node*,
                                 typename G::Node*>>::create(Q);
  for (size_t i = 0; i < Q; ++i)
    queries(i) = {info.nodes[rng() % n], info.nodes[rng() % n]};

  auto ans = mot.path_solve(queries);

  for (size_t i = 0; i < Q; ++i)
    EXPECT_EQ(ans(i), brute_path_distinct(g, parent,
                                          queries(i).first,
                                          queries(i).second))
      << "query " << i;
}

// =================================================================
// Powerful array policy on trees
// =================================================================

TEST(MoOnTreesPowerful, ChainPath)
{
  // Chain: 1 — 2 — 1 — 3
  // Path (0→3): values {1,2,1,3}, cnt = {1:2, 2:1, 3:1}
  //   power = 4*1 + 1*2 + 1*3 = 9
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  auto info = build_chain<G>({1, 2, 1, 3});
  auto & n = info.nodes;

  Powerful_Array_Mo_On_Trees<G> mot(info.g, n[0]);

  auto ans = mot.path_solve({{n[0], n[3]}});
  EXPECT_EQ(ans(0), 9LL);

  // Subtree of root = whole tree = same answer
  auto sub = mot.subtree_solve({n[0]});
  EXPECT_EQ(sub(0), 9LL);
}

// =================================================================
// Deep chain (regression: iterative DFS handles deep trees)
// =================================================================

TEST(MoOnTrees, DeepChain)
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  const size_t N = 5000;
  G g;
  std::vector<typename G::Node*> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i % 7)));
  for (size_t i = 1; i < N; ++i)
    g.insert_arc(nodes[i-1], nodes[i]);

  Distinct_Count_Mo_On_Trees<G> mot(g, nodes[0]);

  // Subtree of root = all nodes
  auto sub = mot.subtree_solve({nodes[0]});
  EXPECT_EQ(sub(0), 7u); // values 0..6

  // Path from first to last = all nodes
  auto path = mot.path_solve({{nodes[0], nodes[N-1]}});
  EXPECT_EQ(path(0), 7u);
}

// =================================================================
// No-queries edge case
// =================================================================

TEST(MoOnTrees, EmptyQueryArrays)
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;
  auto * r = g.insert_node(1);
  Distinct_Count_Mo_On_Trees<G> mot(g, r);

  auto sub = mot.subtree_solve(Array<typename G::Node*>());
  EXPECT_EQ(sub.size(), 0u);

  auto path = mot.path_solve(
      Array<std::pair<typename G::Node*, typename G::Node*>>());
  EXPECT_EQ(path.size(), 0u);
}

// =================================================================
// Tree_Node tests
// =================================================================

TEST(MoOnTreeNode, SingleNode)
{
  using TN = Tree_Node<int>;
  auto * r = new TN(42);
  Distinct_Count_Mo_On_Tree_Node<int> mot(r);
  EXPECT_EQ(mot.size(), 1u);

  auto sub = mot.subtree_solve({r});
  EXPECT_EQ(sub(0), 1u);

  auto path = mot.path_solve({{r, r}});
  EXPECT_EQ(path(0), 1u);

  destroy_tree(r);
}

TEST(MoOnTreeNode, SmallTreeSubtree)
{
  using TN = Tree_Node<int>;
  //       r(1)
  //      / | \          .
  //   a(2) b(1) c(3)
  //   / \       |
  // d(4) e(2)  f(1)
  // |
  // g(5)
  auto * r = new TN(1);
  auto * a = new TN(2);
  auto * b = new TN(1);
  auto * c = new TN(3);
  auto * d = new TN(4);
  auto * e = new TN(2);
  auto * f = new TN(1);
  auto * g = new TN(5);

  r->insert_rightmost_child(a);
  r->insert_rightmost_child(b);
  r->insert_rightmost_child(c);
  a->insert_rightmost_child(d);
  a->insert_rightmost_child(e);
  c->insert_rightmost_child(f);
  d->insert_rightmost_child(g);

  Distinct_Count_Mo_On_Tree_Node<int> mot(r);
  EXPECT_EQ(mot.size(), 8u);

  auto sub = mot.subtree_solve({r, a, b, c, d, e, f, g});
  EXPECT_EQ(sub(0), 5u);  // root: {1,2,1,3,4,2,1,5} → 5
  EXPECT_EQ(sub(1), 3u);  // a: {2,4,5,2} → 3
  EXPECT_EQ(sub(2), 1u);  // b: {1} → 1
  EXPECT_EQ(sub(3), 2u);  // c: {3,1} → 2
  EXPECT_EQ(sub(4), 2u);  // d: {4,5} → 2
  EXPECT_EQ(sub(5), 1u);  // e: {2} → 1
  EXPECT_EQ(sub(6), 1u);  // f: {1} → 1
  EXPECT_EQ(sub(7), 1u);  // g: {5} → 1

  destroy_tree(r);
}

TEST(MoOnTreeNode, SmallTreePath)
{
  using TN = Tree_Node<int>;
  // Same tree as above
  auto * r = new TN(1);
  auto * a = new TN(2);
  auto * b = new TN(1);
  auto * c = new TN(3);
  auto * d = new TN(4);
  auto * e = new TN(2);
  auto * f = new TN(1);
  auto * g = new TN(5);

  r->insert_rightmost_child(a);
  r->insert_rightmost_child(b);
  r->insert_rightmost_child(c);
  a->insert_rightmost_child(d);
  a->insert_rightmost_child(e);
  c->insert_rightmost_child(f);
  d->insert_rightmost_child(g);

  Distinct_Count_Mo_On_Tree_Node<int> mot(r);

  auto path = mot.path_solve({{g, f}, {e, b}, {d, c}, {r, r}, {a, g}});
  // g→f: g(5)→d(4)→a(2)→r(1)→c(3)→f(1) → {5,4,2,1,3} = 5
  EXPECT_EQ(path(0), 5u);
  // e→b: e(2)→a(2)→r(1)→b(1) → {2,1} = 2
  EXPECT_EQ(path(1), 2u);
  // d→c: d(4)→a(2)→r(1)→c(3) → {4,2,1,3} = 4
  EXPECT_EQ(path(2), 4u);
  // r→r: {1} = 1
  EXPECT_EQ(path(3), 1u);
  // a→g: a(2)→d(4)→g(5) → {2,4,5} = 3
  EXPECT_EQ(path(4), 3u);

  destroy_tree(r);
}

TEST(MoOnTreeNode, StressRandomSubtree)
{
  using TN = Tree_Node<int>;
  const size_t N = 200;
  const int max_val = 10;

  std::mt19937 rng(54321);
  std::uniform_int_distribution<int> val_dist(0, max_val - 1);
  std::uniform_int_distribution<size_t> parent_dist;

  // Build random tree
  std::vector<TN*> nodes(N);
  nodes[0] = new TN(val_dist(rng));
  for (size_t i = 1; i < N; ++i)
    {
      nodes[i] = new TN(val_dist(rng));
      size_t par = parent_dist(rng) % i;
      nodes[par]->insert_rightmost_child(nodes[i]);
    }

  Distinct_Count_Mo_On_Tree_Node<int> mot(nodes[0]);
  EXPECT_EQ(mot.size(), N);

  // Brute-force subtree distinct: DFS collecting values
  auto brute_subtree = [&](TN * sub_root) -> size_t
    {
      DynSetHash<int> seen;
      std::vector<TN*> stk;
      stk.push_back(sub_root);
      while (!stk.empty())
        {
          auto * cur = stk.back();
          stk.pop_back();
          seen.insert(cur->get_key());
          for (auto * ch = cur->get_left_child(); ch != nullptr;
               ch = ch->get_right_sibling())
            stk.push_back(ch);
        }
      return seen.size();
    };

  // Query every node
  auto qroots = Array<TN*>::create(N);
  for (size_t i = 0; i < N; ++i)
    qroots(i) = nodes[i];

  auto ans = mot.subtree_solve(qroots);
  for (size_t i = 0; i < N; ++i)
    EXPECT_EQ(ans(i), brute_subtree(nodes[i]))
      << "Mismatch at subtree node " << i;

  destroy_tree(nodes[0]);
}

TEST(MoOnTreeNode, NullRootThrows)
{
  EXPECT_THROW(
    Distinct_Count_Mo_On_Tree_Node<int>(nullptr),
    std::domain_error);
}

TEST(MoOnTreeNode, EmptyQueryArrays)
{
  using TN = Tree_Node<int>;
  auto * r = new TN(1);
  Distinct_Count_Mo_On_Tree_Node<int> mot(r);

  auto sub = mot.subtree_solve(Array<TN*>());
  EXPECT_EQ(sub.size(), 0u);

  auto path = mot.path_solve(Array<std::pair<TN*, TN*>>());
  EXPECT_EQ(path.size(), 0u);

  destroy_tree(r);
}
