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
 * @file graph_exhaustive_trees_test.cc
 * @brief Minimum spanning trees, trees from graphs and depth-first trees
 *        with their numbers, on every small multigraph, against the
 *        oracles.
 *
 * Kruskal and Prim run on every multigraph of 3 nodes and 3 arcs with
 * every assignment of the weights -1, 0 and 1, on every one of 4 nodes
 * and 3 arcs with each set of hidden arcs, and on random graphs of up to
 * 8 nodes, on the three kinds of graph that are not directed; the weight
 * of what they give is that of the minimum spanning forest of
 * oracle::min_spanning_forest_weight(). The trees
 * built from a graph, and the depth-first trees with their df and low
 * numbers, are checked against their definitions on every multigraph of
 * the domain of graph_harness.H.
 */

#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Kruskal.H>
#include <Prim.H>
#include <graph_to_tree.H>
#include <generate_df_tree.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;
using graph_filters::Weight_Of;

namespace
{
  /// Calls f with each assignment to the arcs of s of weights from lo to
  /// hi
  template <class F>
  void for_each_weighting(Spec s, long long lo, long long hi, F && f)
  {
    s.w.assign(s.arcs.size(), lo);
    while (true)
      {
        f(static_cast<const Spec &>(s));
        size_t i = 0;
        while (i < s.w.size() and s.w[i] == hi)
          s.w[i++] = lo;
        if (i == s.w.size())
          return;
        ++s.w[i];
      }
  }

  /// What is wrong with the arcs, by number, as a minimum spanning forest
  /// of the shown part of s, or of the component of start in it if start
  /// is not -1; empty if nothing
  std::string forest_error(const Spec & s, const std::vector<int> & arcs,
                           unsigned long hidden, int start = -1)
  {
    std::vector<int> orig;
    const Spec shown = oracle::without(s, hidden, orig);
    const auto comp = oracle::component_of(shown);

    // The part that must be spanned: all, or the component of start
    Spec part;
    part.n = s.n;
    for (size_t i = 0; i < shown.arcs.size(); ++i)
      if (start == -1 or comp[size_t(shown.arcs[i][0])] == comp[size_t(start)])
        {
          part.arcs.push_back(shown.arcs[i]);
          part.w.push_back(shown.w[i]);
        }
    int nodes = s.n;
    if (start != -1)
      nodes = int(std::count(comp.begin(), comp.end(), comp[size_t(start)]));
    const int want_arcs = nodes - (start == -1 ? oracle::components(shown) : 1);

    oracle::Union_Find uf(s.n);
    long long total = 0;
    for (int i : arcs)
      {
        if (i < 0 or size_t(i) >= s.arcs.size())
          return "an arc that is not of the graph";
        if (not graph_filters::shown(hidden, i))
          return "the hidden arc " + std::to_string(i);
        const auto & a = s.arcs[size_t(i)];
        if (start != -1 and comp[size_t(a[0])] != comp[size_t(start)])
          return "the arc " + std::to_string(i) + " out of the component of the start";
        if (not uf.join(a[0], a[1]))
          return "the arc " + std::to_string(i) + " closes a cycle";
        total += s.w[size_t(i)];
      }
    if (int(arcs.size()) != want_arcs)
      return std::to_string(arcs.size()) + " arcs, not " + std::to_string(want_arcs);
    if (const long long want = oracle::min_spanning_forest_weight(part); total != want)
      return "weight " + std::to_string(total) + ", not " + std::to_string(want);
    return "";
  }

  /// The arcs of g, by number, that carry the bit Spanning_Tree
  template <class GT>
  std::vector<int> painted_arcs(const GT & g)
  {
    std::vector<int> r;
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree))
        r.push_back(num(it.get_curr()));
    return r;
  }

  /// The arcs of g, by number, that the arcs of a tree map to, if the tree
  /// has a node for each node of g mapped both ways; else {-1}
  template <class GT>
  std::vector<int> tree_arcs(const GT & g, const GT & tree)
  {
    if (tree.get_num_nodes() != g.get_num_nodes())
      return {-1};
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      {
        auto * p = it.get_curr();
        auto * t = mapped_node<GT>(p);
        if (t == nullptr or num(t) != num(p) or mapped_node<GT>(t) != p)
          return {-1};
      }
    std::vector<int> r;
    for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * ta = it.get_curr();
        auto * ga = mapped_arc<GT>(ta);
        if (ga == nullptr or num(ga) != num(ta) or mapped_arc<GT>(ga) != ta)
          return {-1};
        r.push_back(num(ga));
      }
    return r;
  }

  /// Runs Kruskal and Prim on s, with weights s.w, hiding the arcs of
  /// hidden; Prim from every node, or from the first and the last
  template <class GT>
  void check_spanning_trees(const Spec & s, unsigned long hidden, bool every_start,
                            Check & kruskal, Check & kruskal_paint, Check & prim,
                            Check & prim_paint)
  {
    using Dist = Weight_Of<GT, long long>;
    using Filter = Hide_Mask<GT>;
    Built<GT> b(s);
    Dist d;
    d.weights = &s.w;
    const std::string hiding = hidden == 0 ? "" : "hiding " + std::to_string(hidden) + ": ";

    kruskal.run();
    {
      GT tree;
      Kruskal_Min_Spanning_Tree<GT, Dist, Filter>(d, Filter{hidden})(b.g, tree);
      if (auto e = forest_error(s, tree_arcs(b.g, tree), hidden); not e.empty())
        kruskal.fail(s, hiding + e);
    }

    kruskal_paint.run();
    {
      Kruskal_Min_Spanning_Tree<GT, Dist, Filter>(d, Filter{hidden})(b.g);
      if (auto e = forest_error(s, painted_arcs(b.g), hidden); not e.empty())
        kruskal_paint.fail(s, hiding + e);
    }

    std::vector<int> starts;
    for (int x = 0; x < s.n; ++x)
      if (every_start or x == 0 or x == s.n - 1)
        starts.push_back(x);
    for (int start : starts)
      {
        const std::string from = hiding + "from " + std::to_string(start) + ": ";
        prim.run();
        {
          GT tree;
          Prim_Min_Spanning_Tree<GT, Dist, Filter>(d, Filter{hidden})
            (b.g, b.nodes[size_t(start)], tree);
          if (auto e = forest_error(s, tree_arcs(b.g, tree), hidden, start); not e.empty())
            prim.fail(s, from + e);
        }

        prim_paint.run();
        {
          Prim_Min_Spanning_Tree<GT, Dist, Filter>(d, Filter{hidden})(b.g, b.nodes[size_t(start)]);
          if (auto e = forest_error(s, painted_arcs(b.g), hidden, start); not e.empty())
            prim_paint.fail(s, from + e);
        }
      }
  }
}

template <class GT>
class ExhaustiveTrees : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveTrees, Undirected_Kinds);

TYPED_TEST(ExhaustiveTrees, SpanningTreesAreMinimumForEveryWeighting)
{
  using GT = TypeParam;
  Check kruskal("Kruskal_Min_Spanning_Tree, the tree");
  Check kruskal_paint("Kruskal_Min_Spanning_Tree, the painting");
  Check prim("Prim_Min_Spanning_Tree, the tree");
  Check prim_paint("Prim_Min_Spanning_Tree, the painting");

  for_each_spec(domain(3, 3), false, [&] (const Spec & spec)
    {
      for_each_weighting(spec, -1, 1, [&] (const Spec & s)
        {
          check_spanning_trees<GT>(s, 0, true, kruskal, kruskal_paint, prim, prim_paint);
        });
    });

  kruskal.expect();
  kruskal_paint.expect();
  prim.expect();
  prim_paint.expect();
}

TYPED_TEST(ExhaustiveTrees, SpanningTreesRespectTheFilter)
{
  using GT = TypeParam;
  Check kruskal("Kruskal_Min_Spanning_Tree, the tree");
  Check kruskal_paint("Kruskal_Min_Spanning_Tree, the painting");
  Check prim("Prim_Min_Spanning_Tree, the tree");
  Check prim_paint("Prim_Min_Spanning_Tree, the painting");

  for_each_spec(domain(4, 3), false, [&] (const Spec & spec)
    {
      // Distinct weights, against the order of the arcs
      Spec s = spec;
      s.w.clear();
      for (size_t i = 0; i < s.arcs.size(); ++i)
        s.w.push_back(static_cast<long long>(s.arcs.size() - i));
      for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
        check_spanning_trees<GT>(s, hidden, false, kruskal, kruskal_paint, prim, prim_paint);
    });

  kruskal.expect();
  kruskal_paint.expect();
  prim.expect();
  prim_paint.expect();
}

TYPED_TEST(ExhaustiveTrees, SpanningTreesAreMinimumOnRandomGraphs)
{
  using GT = TypeParam;
  Check kruskal("Kruskal_Min_Spanning_Tree, the tree");
  Check kruskal_paint("Kruskal_Min_Spanning_Tree, the painting");
  Check prim("Prim_Min_Spanning_Tree, the tree");
  Check prim_paint("Prim_Min_Spanning_Tree, the painting");

  std::mt19937 rng(seed(732));
  const int graphs = heavy() ? 3000 : 300;
  for (int k = 0; k < graphs; ++k)
    {
      const int n = 5 + int(rng() % 4);
      const int m = n - 2 + int(rng() % unsigned(n + 2));
      Spec s = random_spec(rng, n, m, false);
      random_weights(rng, s, -3, 5);
      check_spanning_trees<GT>(s, 0, true, kruskal, kruskal_paint, prim, prim_paint);
    }

  kruskal.expect();
  kruskal_paint.expect();
  prim.expect();
  prim_paint.expect();
}

template <class GT>
class ExhaustiveTreesOnDigraphs : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveTreesOnDigraphs, Directed_Kinds);

TYPED_TEST(ExhaustiveTreesOnDigraphs, SpanningTreesRejectADigraph)
{
  using GT = TypeParam;
  GT g, tree;
  auto * p = g.insert_node(0);
  auto * q = g.insert_node(1);
  g.insert_arc(p, q, 0);
  EXPECT_THROW(Kruskal_Min_Spanning_Tree<GT>()(g, tree), std::domain_error);
  EXPECT_THROW(Kruskal_Min_Spanning_Tree<GT>()(g), std::domain_error);
  EXPECT_THROW(Prim_Min_Spanning_Tree<GT>()(g, p, tree), std::domain_error);
}

namespace
{
  template <class GT>
  struct Copy_Key
  {
    void operator () (typename GT::Node * p, Tree_Node<int> * t) const
    {
      t->get_key() = p->get_info();
    }
  };

  /// What is wrong with the tree under troot as the tree of the component
  /// of root in the shown part of s: a node for each node of the
  /// component, each child joined to its parent by a shown arc; empty if
  /// nothing
  std::string tree_node_error(const Spec & s, Tree_Node<int> * troot, int root,
                              unsigned long hidden)
  {
    std::vector<int> orig;
    const Spec shown = oracle::without(s, hidden, orig);
    const auto comp = oracle::component_of(shown);
    std::multiset<std::pair<int, int>> joined;
    for (const auto & a : shown.arcs)
      joined.insert({std::min(a[0], a[1]), std::max(a[0], a[1])});

    std::set<int> seen;
    std::vector<Tree_Node<int> *> todo = {troot};
    if (troot->get_key() != root)
      return "the root is " + std::to_string(troot->get_key());
    while (not todo.empty())
      {
        Tree_Node<int> * t = todo.back();
        todo.pop_back();
        const int x = t->get_key();
        if (x < 0 or x >= s.n or not seen.insert(x).second
            or comp[size_t(x)] != comp[size_t(root)])
          return "the node " + std::to_string(x) + " is twice or out of the component";
        for (auto * c = t->get_left_child(); c != nullptr; c = c->get_right_sibling())
          {
            const auto it = joined.find({std::min(x, c->get_key()), std::max(x, c->get_key())});
            if (it == joined.end())
              return "no shown arc joins " + std::to_string(x) + " and "
                + std::to_string(c->get_key());
            joined.erase(it);
            todo.push_back(c);
          }
      }
    const auto size = std::count(comp.begin(), comp.end(), comp[size_t(root)]);
    if (std::ptrdiff_t(seen.size()) != size)
      return std::to_string(seen.size()) + " nodes, not " + std::to_string(size);
    return "";
  }
}

TYPED_TEST(ExhaustiveTrees, TreesFromGraphsMatchTheirComponents)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  Check function("graph_to_tree_node()");
  Check functor("Graph_To_Tree_Node");
  Check hides_cycle("Graph_To_Tree_Node, a filter that hides a cycle");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const bool cyclic = oracle::has_cycle(s);
      for (int root = 0; root < s.n; ++root)
        {
          const std::string from = "from " + std::to_string(root) + ": ";
          function.run();
          try
            {
              Tree_Node<int> * t
                = graph_to_tree_node<GT, int, Copy_Key<GT>>(b.g, b.nodes[size_t(root)]);
              if (cyclic)
                function.fail(s, from + "converts a graph with a cycle");
              else if (auto e = tree_node_error(s, t, root, 0); not e.empty())
                function.fail(s, from + e);
              destroy_tree(t);
            }
          catch (const std::domain_error &)
            {
              if (not cyclic)
                function.fail(s, from + "rejects a forest");
            }

          for (unsigned long hidden = 0; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              std::vector<int> orig;
              const bool shown_cyclic = oracle::has_cycle(oracle::without(s, hidden, orig));
              Check & check = cyclic and not shown_cyclic ? hides_cycle : functor;
              check.run();
              const std::string at = from + "hiding " + std::to_string(hidden) + ": ";
              try
                {
                  Tree_Node<int> * t = Graph_To_Tree_Node<GT, int, Copy_Key<GT>, Filter>
                    (Filter{hidden})(b.g, b.nodes[size_t(root)]);
                  if (shown_cyclic)
                    check.fail(s, at + "converts a graph with a cycle");
                  else if (auto e = tree_node_error(s, t, root, hidden); not e.empty())
                    check.fail(s, at + e);
                  destroy_tree(t);
                }
              catch (const std::domain_error &)
                {
                  if (not shown_cyclic)
                    check.fail(s, at + "rejects a forest");
                }
            }
        }
    });

  function.expect();
  functor.expect();
  hides_cycle.expect();
}

namespace
{
  /** What is wrong with a depth-first tree of the component of src in the
      shown part of s, with the arcs that are not in it: its nodes are
      those of the component, labeled with their numbers; df numbers the
      tree in preorder; each child is joined to its parent by an arc, and
      each arc out of the tree, but the loops, joins a node with one of its
      ancestors; low is the least df that a node and its descendants reach
      by one arc out of the tree, or -1 at the root. Empty if nothing. */
  std::string df_low_error(const Spec & s, Df_Low_Tree_Node * root,
                           const DynList<Df_Low_Non_Tree_Arc> & non_tree,
                           int src, unsigned long hidden)
  {
    std::vector<int> orig;
    const Spec shown = oracle::without(s, hidden, orig);
    const auto comp = oracle::component_of(shown);
    std::multiset<std::pair<int, int>> joined;
    for (const auto & a : shown.arcs)
      if (a[0] != a[1] and comp[size_t(a[0])] == comp[size_t(src)])
        joined.insert({std::min(a[0], a[1]), std::max(a[0], a[1])});

    // The preorder, the parents and the df of each node
    std::map<Df_Low_Tree_Node *, Df_Low_Tree_Node *> parent;
    std::vector<Df_Low_Tree_Node *> preorder;
    std::vector<Df_Low_Tree_Node *> todo = {root};
    parent[root] = nullptr;
    while (not todo.empty())
      {
        auto * t = todo.back();
        todo.pop_back();
        preorder.push_back(t);
        std::vector<Df_Low_Tree_Node *> children;
        for (auto * c = t->get_left_child(); c != nullptr; c = c->get_right_sibling())
          {
            children.push_back(c);
            parent[c] = t;
          }
        for (auto it = children.rbegin(); it != children.rend(); ++it)
          todo.push_back(*it);
      }

    std::set<int> seen;
    for (size_t i = 0; i < preorder.size(); ++i)
      {
        const Df_Low_Key & k = preorder[i]->get_key();
        const int x = std::stoi(k.label);
        if (x < 0 or x >= s.n or not seen.insert(x).second or comp[size_t(x)] != comp[size_t(src)])
          return "the node " + k.label + " is twice or out of the component";
        if (k.df != long(i))
          return "the df of " + k.label + " is " + std::to_string(k.df) + ", not "
            + std::to_string(i);
        if (auto * p = parent[preorder[i]]; p != nullptr)
          {
            const int y = std::stoi(p->get_key().label);
            const auto it = joined.find({std::min(x, y), std::max(x, y)});
            if (it == joined.end())
              return "no arc joins " + k.label + " and its parent " + p->get_key().label;
            joined.erase(it);
          }
      }
    if (std::ptrdiff_t(seen.size()) != std::count(comp.begin(), comp.end(), comp[size_t(src)])
        or std::stoi(root->get_key().label) != src)
      return "the tree does not span the component";

    // The arcs out of the tree go from a descendant to an ancestor, and
    // they are the arcs left
    std::map<Df_Low_Tree_Node *, long> reach;
    for (auto it = non_tree.get_it(); it.has_curr(); it.next_ne())
      {
        auto [d, a] = it.get_curr();
        bool ancestor = false;
        for (auto * p = parent[d]; p != nullptr and not ancestor; p = parent[p])
          ancestor = p == a;
        const int x = std::stoi(d->get_key().label), y = std::stoi(a->get_key().label);
        const auto j = joined.find({std::min(x, y), std::max(x, y)});
        if (not ancestor or j == joined.end())
          return "the arc out of the tree " + std::to_string(x) + "-" + std::to_string(y)
            + " is not from a descendant to an ancestor, or is not left";
        joined.erase(j);
        reach[d] = reach.count(d) ? std::min(reach[d], a->get_key().df) : a->get_key().df;
      }
    if (not joined.empty())
      return "an arc out of the tree is missing";

    // low, from the leaves up
    std::map<Df_Low_Tree_Node *, long> low;
    for (auto it = preorder.rbegin(); it != preorder.rend(); ++it)
      {
        auto * t = *it;
        long l = t->get_key().df;
        if (reach.count(t))
          l = std::min(l, reach[t]);
        for (auto * c = t->get_left_child(); c != nullptr; c = c->get_right_sibling())
          l = std::min(l, low[c]);
        low[t] = l;
        const long want = t == root ? -1 : l;
        if (t->get_key().low != want)
          return "the low of " + t->get_key().label + " is " + std::to_string(t->get_key().low)
            + ", not " + std::to_string(want);
      }
    return "";
  }
}

TYPED_TEST(ExhaustiveTrees, DepthFirstTreesHaveTheirNumbers)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  Check df_low("build_df_low_tree()");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      for (unsigned long hidden = 0; hidden < (1UL << s.num_arcs()); ++hidden)
        for (int src = 0; src < s.n; ++src)
          {
            df_low.run();
            DynList<Df_Low_Non_Tree_Arc> non_tree;
            Df_Low_Tree_Node * root
              = build_df_low_tree(b.g, b.nodes[size_t(src)], non_tree,
                                  Dft_Df_Tree_Label<GT>(), Filter{hidden});
            if (auto e = df_low_error(s, root, non_tree, src, hidden); not e.empty())
              df_low.fail(s, "from " + std::to_string(src) + " hiding " + std::to_string(hidden)
                          + ": " + e);
            destroy_tree(root);
          }
    });

  df_low.expect();
}
