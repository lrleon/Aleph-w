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
 * @file graph_exhaustive_tree_queries_test.cc
 * @brief The queries on a tree given as a graph, on every small labeled
 *        tree with every root, against naive answers: lowest common
 *        ancestors, heavy-light and centroid decompositions, dynamic
 *        programming on trees and Mo's algorithm on trees.
 *
 * Every labeled tree of 1 to 5 nodes, by its Prüfer sequence, and a sample
 * of those of 6 and 7 nodes (all of them with ALEPH_GRAPH_HARNESS=heavy)
 * is rooted at each of its nodes, with its arcs inserted from either end
 * and in either order, and its nodes get values from -2 to 2. Each answer
 * of LCA.H, HLD.H, Tree_Decomposition.H, Tree_DP.H and tpl_mo_on_trees.H
 * is compared with the naive one, computed on the Spec with the standard
 * library only: the parents and the depths by a search from the root, the
 * path between two nodes by walking up, the subtrees by ancestry, the
 * distances by breadth-first search, and the folds and the counts over the
 * nodes of a path or of a subtree. Random trees of up to 64 nodes come
 * from fixed seeds.
 *
 * Every multigraph of the domain of graph_harness.H, with each set of
 * hidden arcs (with ALEPH_GRAPH_HARNESS=heavy, on those of up to 3 arcs or
 * 3 nodes), tells whether each class takes what its documentation says: a
 * cycle, a forest, a loop, parallel arcs and a root of another graph are
 * rejected, the empty graph makes an empty tree, and an arc filter that
 * leaves a tree makes that tree. The classes that check their tree as
 * another one of their header does try only the sets of hidden arcs that
 * leave a tree or n - 1 arcs. Tree_DP.H, which takes the arcs as not
 * directed, and Mo on trees, which follows the arcs out of each node, also
 * run on the three kinds of digraph. The queries on nodes out of the tree
 * throw, the copies and the moves answer as the originals, and a path of
 * 10 000 nodes and a star of 2 000 (100 000 and 20 000 with
 * ALEPH_GRAPH_HARNESS=heavy) run in a thread with a stack of 256 KiB.
 */

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_tree_node.H>
#include <LCA.H>
#include <HLD.H>
#include <Tree_Decomposition.H>
#include <Tree_DP.H>
#include <tpl_mo_on_trees.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;

namespace
{
  /// What the classes give for no node: no parent, no heavy child, no
  /// ancestor, no centroid
  constexpr size_t No_Id = std::numeric_limits<size_t>::max();

  std::string text(const std::vector<int> & xs)
  {
    std::string r = "{";
    for (size_t i = 0; i < xs.size(); ++i)
      r += (i == 0 ? "" : " ") + std::to_string(xs[i]);
    return r + "}";
  }

  /// floor(log2(n)), for n >= 1
  int floor_log2(size_t n)
  {
    int k = 0;
    while ((size_t(2) << k) <= n)
      ++k;
    return k;
  }

  /// The labeled tree of n nodes whose Prüfer sequence is seq: each number
  /// of the sequence in turn takes the smallest leaf left, and the last
  /// two leaves join
  Spec prufer_tree(int n, const std::vector<int> & seq)
  {
    Spec s;
    s.n = n;
    if (n < 2)
      return s;
    std::vector<int> degree(size_t(n), 1);
    for (int x : seq)
      ++degree[size_t(x)];
    for (int x : seq)
      for (int leaf = 0; leaf < n; ++leaf)
        if (degree[size_t(leaf)] == 1)
          {
            s.arcs.push_back({leaf, x});
            --degree[size_t(leaf)];
            --degree[size_t(x)];
            break;
          }
    std::vector<int> last;
    for (int x = 0; x < n; ++x)
      if (degree[size_t(x)] == 1)
        last.push_back(x);
    s.arcs.push_back({last[0], last[1]});
    return s;
  }

  /// Inserts each arc of s from the end, and all of them in the order,
  /// that a generator seeded with key says
  void shuffle_ends(Spec & s, unsigned key)
  {
    std::mt19937 rng(key);
    for (auto & a : s.arcs)
      if (rng() % 2 == 1)
        std::swap(a[0], a[1]);
    if (rng() % 2 == 1)
      std::reverse(s.arcs.begin(), s.arcs.end());
  }

  /// Calls f(s) with every step-th labeled tree of n nodes, by the number
  /// k of its Prüfer sequence, with its arcs as shuffle_ends() leaves them
  /// for k
  template <class F>
  void for_each_tree(int n, size_t step, F && f)
  {
    size_t total = 1;
    for (int i = 0; i < n - 2; ++i)
      total *= size_t(n);
    for (size_t k = 0; k < total; k += step)
      {
        std::vector<int> seq;
        size_t c = k;
        for (int i = 0; i < n - 2; ++i, c /= size_t(n))
          seq.push_back(int(c % size_t(n)));
        Spec s = prufer_tree(n, seq);
        shuffle_ends(s, unsigned(k * 31 + size_t(n)));
        f(static_cast<const Spec &>(s));
      }
  }

  /// The trees of the tests: every one of 1 to 5 nodes, and every
  /// (sparse * 29)-th of 6 nodes and (sparse * 389)-th of 7, some 45 of
  /// each; every one of up to 7 nodes with ALEPH_GRAPH_HARNESS=heavy
  template <class F>
  void for_each_test_tree(F && f, size_t sparse = 1)
  {
    for (int n = 1; n <= 7; ++n)
      for_each_tree(n, heavy() or n <= 5 ? 1 : sparse * (n == 6 ? 29 : 389), f);
  }

  /// A random labeled tree of n nodes, of a random Prüfer sequence
  Spec random_tree(std::mt19937 & rng, int n)
  {
    std::vector<int> seq;
    for (int i = 0; i < n - 2; ++i)
      seq.push_back(int(rng() % unsigned(n)));
    Spec s = prufer_tree(n, seq);
    shuffle_ends(s, unsigned(rng()));
    return s;
  }

  /// A value from lo to hi for each of n nodes
  std::vector<int> random_values(std::mt19937 & rng, int n, int lo = -2, int hi = 2)
  {
    std::vector<int> v(static_cast<size_t>(n));
    for (auto & x : v)
      x = lo + int(rng() % unsigned(hi - lo + 1));
    return v;
  }

  /// What the arcs of s that hidden shows make: a tree of all the nodes;
  /// n - 1 arcs, without loops, between distinct pairs of nodes, that are
  /// not one (a cycle and a node apart); or anything else: no node, a
  /// loop, parallel arcs, in either direction, or another number of arcs.
  /// With parallel_once, the arcs between the same two nodes count once.
  enum class Shape { Tree, Near_Tree, Other };

  Shape shape_of(const Spec & s, unsigned long hidden, bool parallel_once = false)
  {
    if (s.n == 0)
      return Shape::Other;
    std::set<std::pair<int, int>> pairs;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        if (not graph_filters::shown(hidden, int(i)))
          continue;
        const int u = std::min(s.arcs[i][0], s.arcs[i][1]);
        const int v = std::max(s.arcs[i][0], s.arcs[i][1]);
        if (u == v or (not pairs.insert({u, v}).second and not parallel_once))
          return Shape::Other;
      }
    if (int(pairs.size()) != s.n - 1)
      return Shape::Other;
    oracle::Union_Find uf(s.n);
    for (const auto & [u, v] : pairs)
      if (not uf.join(u, v))
        return Shape::Near_Tree;
    return Shape::Tree;
  }

  bool is_tree(const Spec & s, unsigned long hidden, bool parallel_once = false)
  {
    return shape_of(s, hidden, parallel_once) == Shape::Tree;
  }

  /// How many sets of hidden arcs the tests try on s: every one on the
  /// graphs of up to 3 arcs or 3 nodes, and only the empty one on the
  /// larger graphs of ALEPH_GRAPH_HARNESS=heavy
  unsigned long masks_of(const Spec & s)
  {
    return s.num_arcs() <= 3 or s.n <= 3 ? 1UL << s.num_arcs() : 1UL;
  }

  /// The roots from which the tests build an engine on the arcs of s
  /// that hidden shows, -2 for the null one and -1 for the constructor
  /// without one: all of them, and every node, on a tree and on the n - 1
  /// arcs that are not one, which the root decides how to reject; on
  /// anything else, which a loop, parallel arcs or the count of the arcs
  /// rule out from any root, the last node, and no root if nothing is
  /// hidden
  std::vector<int> roots_to_try(const Spec & s, unsigned long hidden, Shape shape)
  {
    std::vector<int> r;
    if (s.n == 0)
      return {-2, -1};
    if (shape == Shape::Other)
      {
        if (hidden == 0)
          r.push_back(-1);
        r.push_back(s.n - 1);
        return r;
      }
    for (int x = -2; x < s.n; ++x)
      r.push_back(x);
    return r;
  }

  /// Whether every node of the digraph s is reached from r
  bool reaches_all(const Spec & s, int r)
  {
    const auto reach = oracle::closure(s, true);
    for (int x = 0; x < s.n; ++x)
      if (not oracle::reaches(reach, r, x))
        return false;
    return true;
  }

  /** The tree that the arcs of s shown by hidden make, rooted at root,
      the arcs taken as not directed: the parent and the depth of each
      node by a breadth-first search from the root, the ancestors by
      walking up, and the distances by a breadth-first search from each
      node. The distances, the paths, the subtrees and the lowest common
      ancestors are kept once found. */
  struct Rooted_Tree
  {
    int n = 0;
    int root = -1;
    std::vector<std::vector<int>> adj;       ///< the neighbors of each node
    std::vector<int> order;                  ///< breadth first from the root
    std::vector<int> parent;                 ///< -1 at the root
    std::vector<int> depth;
    std::vector<std::vector<int>> children;  ///< of each node, increasing
    std::vector<int> size;                   ///< of the subtree of each node

  private:
    mutable std::vector<std::vector<int>> dists;
    mutable std::vector<std::vector<int>> paths;
    mutable std::vector<std::vector<int>> subtrees;
    mutable std::vector<int> lcas;

  public:
    Rooted_Tree(const Spec & s, int r, unsigned long hidden = 0)
      : n(s.n), root(r), adj(size_t(s.n)), parent(size_t(s.n), -1),
        depth(size_t(s.n), 0), children(size_t(s.n)), size(size_t(s.n), 1),
        paths(size_t(s.n) * size_t(s.n)), subtrees(size_t(s.n)),
        lcas(size_t(s.n) * size_t(s.n), -1)
    {
      for (size_t i = 0; i < s.arcs.size(); ++i)
        if (graph_filters::shown(hidden, int(i)))
          {
            adj[size_t(s.arcs[i][0])].push_back(s.arcs[i][1]);
            adj[size_t(s.arcs[i][1])].push_back(s.arcs[i][0]);
          }
      order = search(r, &parent, &depth);
      for (int x = 0; x < n; ++x)
        if (parent[size_t(x)] != -1)
          children[size_t(parent[size_t(x)])].push_back(x);
      for (auto it = order.rbegin(); it != order.rend(); ++it)
        if (parent[size_t(*it)] != -1)
          size[size_t(parent[size_t(*it)])] += size[size_t(*it)];
    }

    /// The nodes reached from x, breadth first, with the parent and the
    /// depth of each one if asked
    std::vector<int> search(int x, std::vector<int> * par, std::vector<int> * dep) const
    {
      std::vector<bool> seen(size_t(n), false);
      std::vector<int> queue = {x};
      seen[size_t(x)] = true;
      (*dep)[size_t(x)] = 0;
      for (size_t h = 0; h < queue.size(); ++h)
        for (int y : adj[size_t(queue[h])])
          if (not seen[size_t(y)])
            {
              seen[size_t(y)] = true;
              if (par != nullptr)
                (*par)[size_t(y)] = queue[h];
              (*dep)[size_t(y)] = (*dep)[size_t(queue[h])] + 1;
              queue.push_back(y);
            }
      return queue;
    }

    /// The number of arcs between u and v, by a breadth-first search
    /// from each node the first time
    [[nodiscard]] int distance(int u, int v) const
    {
      if (dists.empty())
        for (int x = 0; x < n; ++x)
          {
            std::vector<int> d(size_t(n), -1);
            search(x, nullptr, &d);
            dists.push_back(d);
          }
      return dists[size_t(u)][size_t(v)];
    }

    /// Whether a is x or one of its ancestors
    [[nodiscard]] bool is_ancestor(int a, int x) const
    {
      for (; x != -1; x = parent[size_t(x)])
        if (x == a)
          return true;
      return false;
    }

    /// The lowest common ancestor of u and v, walking up from u
    [[nodiscard]] int lca(int u, int v) const
    {
      int & a = lcas[size_t(u) * size_t(n) + size_t(v)];
      if (a == -1)
        {
          a = u;
          while (not is_ancestor(a, v))
            a = parent[size_t(a)];
        }
      return a;
    }

    /// The nodes from u to v, in order
    [[nodiscard]] const std::vector<int> & path(int u, int v) const
    {
      auto & p = paths[size_t(u) * size_t(n) + size_t(v)];
      if (p.empty())
        {
          const int a = lca(u, v);
          std::vector<int> down;
          for (int x = u; x != a; x = parent[size_t(x)])
            p.push_back(x);
          p.push_back(a);
          for (int x = v; x != a; x = parent[size_t(x)])
            down.push_back(x);
          p.insert(p.end(), down.rbegin(), down.rend());
        }
      return p;
    }

    /// The nodes of the subtree of x, increasing
    [[nodiscard]] const std::vector<int> & subtree(int x) const
    {
      auto & sub = subtrees[size_t(x)];
      if (sub.empty())
        for (int y = 0; y < n; ++y)
          if (is_ancestor(x, y))
            sub.push_back(y);
      return sub;
    }

    /// Whether the tests also ask for the pair u, v by node: every pair
    /// on the trees of up to 4 nodes, and those with the root and those of
    /// a node with itself on the larger ones. The overloads by node look
    /// up the ids and call those by id, which see every pair.
    [[nodiscard]] bool by_node(int u, int v) const
    {
      return n <= 4 or u == v or u == root or v == root;
    }
  };

  /// s with each arc from the parent to the child in t, in its place
  Spec from_root(const Spec & s, const Rooted_Tree & t)
  {
    Spec r = s;
    for (auto & a : r.arcs)
      if (t.parent[size_t(a[0])] == a[1])
        std::swap(a[0], a[1]);
    return r;
  }

  /// The info, the control bits, the counter and the cookie of every node
  /// and arc of b
  template <class GT>
  std::vector<std::tuple<int, int, long, void *>> attributes(const Built<GT> & b)
  {
    std::vector<std::tuple<int, int, long, void *>> r;
    auto add = [&r] (auto * x)
      {
        int bits = 0;
        for (int k = 0; k < Num_Bits_Graph; ++k)
          bits |= int(x->attrs.control_bits.get_bit(k)) << k;
        r.emplace_back(x->get_info(), bits, x->attrs.counter, x->attrs.cookie);
      };
    for (auto * p : b.nodes)
      add(p);
    for (auto * a : b.arcs)
      add(a);
    return r;
  }

  /// Whether f() throws E
  template <class E, class F>
  bool throws(F && f)
  {
    try
      {
        f();
      }
    catch (const E &)
      {
        return true;
      }
    catch (...)
      {
        return false;
      }
    return false;
  }

  /** What is wrong with what an engine says of the tree t of b, as far as
      it tells: its size, its root, its ids (a numbering of the nodes from
      0, both ways), the depth, the parent and the subtree size of each
      node, and the lowest common ancestor, the distance and the ancestry
      of each pair, by id and, as by_node() says, by node. Empty if
      nothing. */
  template <class GT, class Engine>
  std::string tree_error(const Built<GT> & b, const Rooted_Tree & t, const Engine & e)
  {
    const int n = t.n;
    if (e.size() != size_t(n))
      return "the size is " + std::to_string(e.size());
    if constexpr (requires { e.is_empty(); })
      if (e.is_empty() != (n == 0))
        return "is_empty() is wrong";

    std::vector<size_t> id(static_cast<size_t>(n));
    std::vector<bool> used(size_t(n), false);
    for (int x = 0; x < n; ++x)
      {
        const size_t i = e.id_of(b.nodes[size_t(x)]);
        if (i >= size_t(n) or used[i] or e.node_of(i) != b.nodes[size_t(x)])
          return "the ids do not number the nodes";
        used[i] = true;
        id[size_t(x)] = i;
      }

    if constexpr (requires { e.root_id(); })
      if (e.root() != b.nodes[size_t(t.root)] or e.root_id() != id[size_t(t.root)])
        return "the root is not " + std::to_string(t.root);

    for (int x = 0; x < n; ++x)
      {
        const size_t i = id[size_t(x)];
        auto * p = b.nodes[size_t(x)];
        const int y = t.parent[size_t(x)];
        const std::string of = " of " + std::to_string(x);
        if constexpr (requires { e.depth_of_id(i); })
          if (e.depth_of_id(i) != size_t(t.depth[size_t(x)])
              or e.depth_of(p) != size_t(t.depth[size_t(x)]))
            return "the depth" + of;
        if constexpr (requires { e.parent_id(i); })
          if (e.parent_id(i) != (y == -1 ? No_Id : id[size_t(y)])
              or e.parent_of(p) != (y == -1 ? nullptr : b.nodes[size_t(y)]))
            return "the parent" + of;
        if constexpr (requires { e.subtree_size_of_id(i); })
          if (e.subtree_size_of_id(i) != size_t(t.size[size_t(x)])
              or e.subtree_size_of(p) != size_t(t.size[size_t(x)]))
            return "the subtree size" + of;
      }

    for (int u = 0; u < n; ++u)
      for (int v = 0; v < n; ++v)
        {
          const size_t i = id[size_t(u)], j = id[size_t(v)];
          auto * p = b.nodes[size_t(u)];
          auto * q = b.nodes[size_t(v)];
          const bool by_node = t.by_node(u, v);
          auto of = [&] { return " of " + std::to_string(u) + " and " + std::to_string(v); };
          if constexpr (requires { e.lca_id(i, j); })
            {
              const int a = t.lca(u, v);
              if (e.lca_id(i, j) != id[size_t(a)] or (by_node and e.lca(p, q) != b.nodes[size_t(a)]))
                return "the lowest common ancestor" + of();
            }
          if constexpr (requires { e.distance_id(i, j); })
            {
              const size_t d = size_t(t.distance(u, v));
              if (e.distance_id(i, j) != d or (by_node and e.distance(p, q) != d))
                return "the distance" + of();
            }
          if constexpr (requires { e.is_ancestor_id(i, j); })
            if (e.is_ancestor_id(i, j) != t.is_ancestor(u, v)
                or (by_node and e.is_ancestor(p, q) != t.is_ancestor(u, v)))
              return "the ancestry" + of();
        }
    return "";
  }

  /** Builds the engine that make() returns on the arcs of s that hidden
      shows, whose shape is shape, rooted at r, or at the first node if r
      is negative, and expects what the documentation says: an empty
      engine on the empty graph, the tree when the shown arcs make one,
      and std::domain_error otherwise. answer(e, t) says what is wrong with
      the engine e of a tree t. */
  template <class GT, class Make, class Answer>
  void expect_taken(Check & c, const Spec & s, const Built<GT> & b, unsigned long hidden,
                    Shape shape, int r, Make make, Answer answer)
  {
    c.run();
    const bool tree = shape == Shape::Tree;
    auto at = [&]
      {
        return "hiding " + std::to_string(hidden)
          + (r < 0 ? ", the first node as the root: " : ", rooted at " + std::to_string(r) + ": ");
      };
    try
      {
        const auto e = make();
        if (s.n == 0)
          {
            if (e.size() != 0)
              c.fail(s, at() + "an engine of the empty graph that is not empty");
          }
        else if (not tree)
          c.fail(s, at() + "takes a graph that is not a tree");
        else
          {
            const Rooted_Tree t(s, r < 0 ? num(b.g.get_first_node()) : r, hidden);
            if (auto err = answer(e, t); not err.empty())
              c.fail(s, at() + err);
          }
      }
    catch (const std::domain_error &)
      {
        if (tree or s.n == 0)
          c.fail(s, at() + "rejects a tree");
      }
    catch (const std::exception & ex)
      {
        c.fail(s, at() + "throws " + ex.what());
      }
  }

  /// The value of the node x in the tests of what the engines take
  int value_of(int x) { return x % 5 - 2; }

  /// The values of the nodes of b, by value_of()
  template <class GT>
  std::vector<int> values_of(const Built<GT> & b)
  {
    std::vector<int> val;
    for (size_t x = 0; x < b.nodes.size(); ++x)
      val.push_back(value_of(int(x)));
    return val;
  }

  /** Whether the engine called name takes exactly what its documentation
      says, by expect_taken(), on every multigraph of the domain of
      graph_harness.H, with each set of hidden arcs that masks_of() tries
      on it and from each root of roots_to_try(). make(b, hidden, root, r)
      builds the engine on the graph b with a filter that hides hidden,
      rooted at root, or without a root if r is -1; answer(b, e, t) tells
      what is wrong with the engine e of the tree t of b. With
      parallel_once, the parallel arcs count once. The engine must also
      reject a root of another graph, and leave the graph as it was.

      With every_mask false, for an engine that checks its tree with the
      same class as another engine of its header tried here with every
      mask, the sets of hidden arcs are tried only where they leave a
      tree or n - 1 arcs, besides every multigraph as it is: the rest,
      which a loop, parallel arcs or the count of the arcs rule out from
      any root, that other engine has rejected under every mask. */
  template <class GT, class Make, class Answer>
  void expect_takes_the_trees(const std::string & name, bool parallel_once, bool every_mask,
                              Make make, Answer answer)
  {
    Check taken(name + ", the graphs it takes");
    Check foreign(name + ", a root of another graph");
    Check untouched(name + " leaves the graph as it was");
    Built<GT> other(Spec{1, {}, {}});

    for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & s)
      {
        Built<GT> b(s);
        const auto before = attributes(b);
        for (unsigned long hidden = 0; hidden < masks_of(s); ++hidden)
          {
            const Shape shape = shape_of(s, hidden, parallel_once);
            if (hidden != 0 and shape == Shape::Other and not every_mask)
              continue;
            // -2 is the null root, and -1 the constructor without a root
            for (int r : roots_to_try(s, hidden, shape))
              expect_taken(taken, s, b, hidden, shape, r, [&]
                {
                  return make(b, hidden, r < 0 ? nullptr : b.nodes[size_t(r)], r);
                }, [&] (const auto & e, const Rooted_Tree & t) { return answer(b, e, t); });
          }

        foreign.run();
        if (not throws<std::domain_error>([&] { (void) make(b, 0, other.nodes[0], 0); }))
          foreign.fail(s, "takes a root of another graph");

        untouched.run();
        if (attributes(b) != before)
          untouched.fail(s, "the infos, bits, counters or cookies changed");
      });

    taken.expect();
    foreign.expect();
    untouched.expect();
  }
}

// --------------------------------------------------------------------------
// LCA.H
// --------------------------------------------------------------------------

namespace
{
  /// What is wrong with the k-th ancestors that an engine gives on the
  /// tree t of b: for each node and each k up to its depth, the node k
  /// arcs above it, and none beyond, by id and by node
  template <class GT, class LCA>
  std::string kth_error(const Built<GT> & b, const Rooted_Tree & t, const LCA & lca)
  {
    for (int x = 0; x < t.n; ++x)
      {
        const size_t i = lca.id_of(b.nodes[size_t(x)]);
        int a = x;
        for (int k = 0; k <= t.depth[size_t(x)] + 1; ++k)
          {
            const size_t want = a == -1 ? No_Id : lca.id_of(b.nodes[size_t(a)]);
            if (lca.kth_ancestor_id(i, size_t(k)) != want
                or lca.kth_ancestor(b.nodes[size_t(x)], size_t(k))
                   != (a == -1 ? nullptr : b.nodes[size_t(a)]))
              return "the ancestor " + std::to_string(k) + " arcs above " + std::to_string(x);
            a = a == -1 ? -1 : t.parent[size_t(a)];
          }
        if (lca.kth_ancestor_id(i, No_Id) != No_Id)
          return "an ancestor of " + std::to_string(x) + " far above the root";
      }
    return "";
  }

  /// What is wrong with the Euler tour of an engine on t: from the root,
  /// each step goes down to a child not yet visited or, once the children
  /// are done, up to the parent, until it is back at the root with every
  /// node visited; 2n - 1 steps in all
  template <class LCA>
  std::string euler_tour_error(const Rooted_Tree & t, const LCA & er)
  {
    const auto & tour = er.euler_tour();
    const size_t len = er.euler_tour_size();
    if (len != size_t(2 * t.n - 1) or tour.size() != len)
      return "a tour of " + std::to_string(len) + " steps";
    std::vector<int> seq;
    for (size_t i = 0; i < len; ++i)
      {
        if (tour[i] >= size_t(t.n))
          return "a tour through an id out of range";
        seq.push_back(num(er.node_of(tour[i])));
      }
    if (seq[0] != t.root)
      return "a tour that does not start at the root";
    std::vector<bool> seen(size_t(t.n), false);
    seen[size_t(t.root)] = true;
    int at = t.root;
    for (size_t i = 1; i < seq.size(); ++i)
      {
        const int x = seq[i];
        const auto & ch = t.children[size_t(at)];
        const bool done = std::all_of(ch.begin(), ch.end(), [&] (int c) { return seen[size_t(c)]; });
        if (t.parent[size_t(x)] == at and not seen[size_t(x)])
          seen[size_t(x)] = true;
        else if (x != t.parent[size_t(at)] or not done)
          return "the tour goes from " + std::to_string(at) + " to " + std::to_string(x);
        at = x;
      }
    if (at != t.root or std::find(seen.begin(), seen.end(), false) != seen.end())
      return "the tour does not end at the root with every node visited";
    return "";
  }
}

template <class GT>
class ExhaustiveTreeQueries : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveTreeQueries, Undirected_Kinds);

TYPED_TEST(ExhaustiveTreeQueries, LcaEnginesAnswerEveryQuery)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check lifting("Gen_Binary_Lifting_LCA");
  Check kth("Gen_Binary_Lifting_LCA::kth_ancestor()");
  Check levels("Gen_Binary_Lifting_LCA::num_levels()");
  Check euler("Gen_Euler_RMQ_LCA");
  Check tour("Gen_Euler_RMQ_LCA::euler_tour()");
  Check first("the LCA engines rooted at the first node");

  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          const std::string at = "rooted at " + std::to_string(r) + ": ";

          lifting.run();
          const Binary_Lifting_LCA<GT> bl(b.g, b.nodes[size_t(r)]);
          if (auto e = tree_error(b, t, bl); not e.empty())
            lifting.fail(s, at + e);

          kth.run();
          if (auto e = kth_error(b, t, bl); not e.empty())
            kth.fail(s, at + e);

          euler.run();
          const Euler_RMQ_LCA<GT> er(b.g, b.nodes[size_t(r)]);
          if (auto e = tree_error(b, t, er); not e.empty())
            euler.fail(s, at + e);

          tour.run();
          if (auto e = euler_tour_error(t, er); not e.empty())
            tour.fail(s, at + e);
        }

      const Gen_Binary_Lifting_LCA<GT> bl(b.g);
      const size_t want_levels = size_t(floor_log2(size_t(s.n)) + 1);
      levels.run();
      if (bl.num_levels() != want_levels)
        levels.fail(s, std::to_string(bl.num_levels()) + " levels, not "
                    + std::to_string(want_levels));

      // Without a root, or with the null one, the root is the first node
      first.run();
      const Rooted_Tree t(s, num(b.g.get_first_node()));
      const Gen_Euler_RMQ_LCA<GT> er(b.g);
      const Gen_Binary_Lifting_LCA<GT> bl_null(b.g, static_cast<Node *>(nullptr));
      const Gen_Euler_RMQ_LCA<GT> er_null(b.g, static_cast<Node *>(nullptr));
      for (const auto & e : {tree_error(b, t, bl), tree_error(b, t, er),
                             tree_error(b, t, bl_null), tree_error(b, t, er_null)})
        if (not e.empty())
          {
            first.fail(s, e);
            break;
          }
    });

  lifting.expect();
  kth.expect();
  levels.expect();
  euler.expect();
  tour.expect();
  first.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, BinaryLiftingLcaTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  using Engine = Gen_Binary_Lifting_LCA<GT, Filter>;
  expect_takes_the_trees<GT>("Gen_Binary_Lifting_LCA", false, true,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int r)
      {
        return r == -1 ? Engine(b.g, Filter{hidden}) : Engine(b.g, root, Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t) { return tree_error(b, t, e); });
}

TYPED_TEST(ExhaustiveTreeQueries, EulerRmqLcaTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  using Engine = Gen_Euler_RMQ_LCA<GT, Filter>;
  expect_takes_the_trees<GT>("Gen_Euler_RMQ_LCA", false, false,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int r)
      {
        return r == -1 ? Engine(b.g, Filter{hidden}) : Engine(b.g, root, Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t) { return tree_error(b, t, e); });
}

TYPED_TEST(ExhaustiveTreeQueries, LcaEnginesOnRandomTrees)
{
  using GT = TypeParam;
  Check lifting("Gen_Binary_Lifting_LCA");
  Check kth("Gen_Binary_Lifting_LCA::kth_ancestor()");
  Check euler("Gen_Euler_RMQ_LCA");
  Check tour("Gen_Euler_RMQ_LCA::euler_tour()");

  std::mt19937 rng(seed(7331));
  const int trees = heavy() ? 300 : 30;
  for (int k = 0; k < trees; ++k)
    {
      const Spec s = random_tree(rng, 8 + int(rng() % 57));
      Built<GT> b(s);
      for (int r : {0, s.n - 1, int(rng() % unsigned(s.n))})
        {
          const Rooted_Tree t(s, r);
          const std::string at = "rooted at " + std::to_string(r) + ": ";
          const Gen_Binary_Lifting_LCA<GT> bl(b.g, b.nodes[size_t(r)]);
          const Gen_Euler_RMQ_LCA<GT> er(b.g, b.nodes[size_t(r)]);

          lifting.run();
          if (auto e = tree_error(b, t, bl); not e.empty())
            lifting.fail(s, at + e);
          kth.run();
          if (auto e = kth_error(b, t, bl); not e.empty())
            kth.fail(s, at + e);
          euler.run();
          if (auto e = tree_error(b, t, er); not e.empty())
            euler.fail(s, at + e);
          tour.run();
          if (auto e = euler_tour_error(t, er); not e.empty())
            tour.fail(s, at + e);
        }
    }

  lifting.expect();
  kth.expect();
  euler.expect();
  tour.expect();
}

// --------------------------------------------------------------------------
// HLD.H
// --------------------------------------------------------------------------

namespace
{
  /// The sum, the count, the least and the greatest of some values
  struct Summary
  {
    long long sum = 0;
    long long count = 0;
    int low = std::numeric_limits<int>::max();
    int high = std::numeric_limits<int>::lowest();

    bool operator == (const Summary &) const = default;
  };

  /// An operation of the user: associative and commutative, with the
  /// default Summary as its identity
  struct Summary_Op
  {
    Summary operator () (const Summary & a, const Summary & b) const noexcept
    {
      return {a.sum + b.sum, a.count + b.count, std::min(a.low, b.low), std::max(a.high, b.high)};
    }
  };

  Summary summary_of(int v) { return {v, 1, v, v}; }

  std::vector<Summary> summaries(const std::vector<int> & val)
  {
    std::vector<Summary> r;
    for (int v : val)
      r.push_back(summary_of(v));
    return r;
  }

  std::string show(long long x) { return std::to_string(x); }

  std::string show(const Summary & s)
  {
    return "(" + std::to_string(s.sum) + " " + std::to_string(s.count) + " "
      + std::to_string(s.low) + " " + std::to_string(s.high) + ")";
  }

  /// op folded from identity over the values val of the nodes, but skip
  template <typename T, class Op>
  T fold(const std::vector<int> & nodes, const std::vector<T> & val, const T & identity,
         Op op, int skip = -1)
  {
    T r = identity;
    for (int x : nodes)
      if (x != skip)
        r = op(r, val[size_t(x)]);
    return r;
  }

  /** What is wrong with the folds of a Gen_HLD on the tree t of b whose
      node x has the value val[x], with the operation op of identity
      identity: the value of each node, the fold over its subtree, and
      over the path of each pair with and without its lowest common
      ancestor; by id, and by node as Rooted_Tree::by_node() says unless
      by_node is false. Empty if nothing. */
  template <class GT, class HLD, typename T, class Op>
  std::string fold_error(const Built<GT> & b, const Rooted_Tree & t, const HLD & h,
                         const std::vector<T> & val, const T & identity, Op op,
                         bool by_node = true)
  {
    const int n = t.n;
    std::vector<size_t> id(static_cast<size_t>(n));
    for (int x = 0; x < n; ++x)
      id[size_t(x)] = h.id_of(b.nodes[size_t(x)]);

    for (int x = 0; x < n; ++x)
      {
        auto * p = b.nodes[size_t(x)];
        if (h.get_value_at_id(id[size_t(x)]) != val[size_t(x)]
            or (by_node and h.get_value(p) != val[size_t(x)]))
          return "the value of " + std::to_string(x);
        const T want = fold(t.subtree(x), val, identity, op);
        const T got = h.subtree_query_id(id[size_t(x)]);
        if (got != want or (by_node and h.subtree_query(p) != want))
          return "the subtree of " + std::to_string(x) + " gives " + show(got) + ", not "
            + show(want);
      }

    for (int u = 0; u < n; ++u)
      for (int v = 0; v < n; ++v)
        {
          const auto & path = t.path(u, v);
          auto * p = b.nodes[size_t(u)];
          auto * q = b.nodes[size_t(v)];
          const bool pair_by_node = by_node and t.by_node(u, v);
          auto from = [&] { return " from " + std::to_string(u) + " to " + std::to_string(v); };
          const T want = fold(path, val, identity, op);
          const T got = h.path_query_id(id[size_t(u)], id[size_t(v)]);
          if (got != want or (pair_by_node and h.path_query(p, q) != want))
            return "the path" + from() + " gives " + show(got) + ", not " + show(want);
          const T want_edges = fold(path, val, identity, op, t.lca(u, v));
          const T got_edges = h.path_query_edges_id(id[size_t(u)], id[size_t(v)]);
          if (got_edges != want_edges or (pair_by_node and h.path_query_edges(p, q) != want_edges))
            return "the arcs of the path" + from() + " give " + show(got_edges) + ", not "
              + show(want_edges);
        }
    return "";
  }

  /** What is wrong with a heavy-light layout of t, given for each node by
      number: pos, its place in the base array, and head, the first node
      of its chain. The subtree of each node must be the range of places
      that starts at it; the chain of each node with children goes on to
      one of those with the largest subtree, at the next place, and every
      other child starts a chain; no path from the root takes more than
      log2(n) arcs into a new chain. Empty if nothing. */
  std::string layout_error(const Rooted_Tree & t, const std::vector<size_t> & pos,
                           const std::vector<int> & head)
  {
    const int n = t.n;
    std::vector<bool> taken(size_t(n), false);
    for (int x = 0; x < n; ++x)
      {
        if (pos[size_t(x)] >= size_t(n) or taken[pos[size_t(x)]])
          return "the places do not number the nodes";
        taken[pos[size_t(x)]] = true;
      }
    for (int x = 0; x < n; ++x)
      for (int y = 0; y < n; ++y)
        if (t.is_ancestor(x, y) != (pos[size_t(x)] <= pos[size_t(y)]
                                    and pos[size_t(y)] < pos[size_t(x)] + size_t(t.size[size_t(x)])))
          return "the subtree of " + std::to_string(x) + " is not its range of places";
    if (head[size_t(t.root)] != t.root)
      return "the root does not start a chain";
    for (int x = 0; x < n; ++x)
      {
        int going_on = 0;
        for (int c : t.children[size_t(x)])
          if (head[size_t(c)] == head[size_t(x)])
            {
              ++going_on;
              for (int d : t.children[size_t(x)])
                if (t.size[size_t(d)] > t.size[size_t(c)])
                  return "the chain of " + std::to_string(x) + " goes on to the light child "
                    + std::to_string(c);
              if (pos[size_t(c)] != pos[size_t(x)] + 1)
                return "the chain of " + std::to_string(x) + " is not contiguous";
            }
          else if (head[size_t(c)] != c)
            return "the light child " + std::to_string(c) + " does not start a chain";
        if (going_on != (t.children[size_t(x)].empty() ? 0 : 1))
          return "the chain of " + std::to_string(x) + " goes on to "
            + std::to_string(going_on) + " children";
      }
    for (int x = 0; x < n; ++x)
      {
        int light = 0;
        for (int y = x; y != t.root; y = t.parent[size_t(y)])
          light += head[size_t(y)] == y;
        if (light > floor_log2(size_t(n)))
          return "the path to " + std::to_string(x) + " enters " + std::to_string(light)
            + " chains";
      }
    return "";
  }

  /// What is wrong with the layout of a Gen_HLD on the tree t of b: the
  /// position and the chain head of each node, which must make a layout of
  /// layout_error(), the arrays that tell them and the depths, the subtree
  /// sizes and the parents, and the number of chains
  template <class GT, class HLD>
  std::string hld_layout_error(const Built<GT> & b, const Rooted_Tree & t, const HLD & h)
  {
    const size_t n = size_t(t.n);
    if (h.position_array().size() != n or h.chain_head_array().size() != n
        or h.depth_array().size() != n or h.subtree_size_array().size() != n
        or h.parent_array().size() != n)
      return "an array without a place for each node";
    std::vector<size_t> pos(n);
    std::vector<int> head(n);
    size_t heads = 0;
    for (int x = 0; x < t.n; ++x)
      {
        const size_t i = h.id_of(b.nodes[size_t(x)]);
        const size_t hd = h.chain_head_id(i);
        const int y = t.parent[size_t(x)];
        if (hd >= n)
          return "the chain of " + std::to_string(x) + " has no head";
        pos[size_t(x)] = h.position(i);
        head[size_t(x)] = num(h.node_of(hd));
        heads += head[size_t(x)] == x;
        if (h.position_array()[i] != pos[size_t(x)] or h.chain_head_array()[i] != hd
            or h.depth_array()[i] != size_t(t.depth[size_t(x)])
            or h.subtree_size_array()[i] != size_t(t.size[size_t(x)])
            or h.parent_array()[i] != (y == -1 ? No_Id : h.id_of(b.nodes[size_t(y)])))
          return "the arrays are wrong at " + std::to_string(x);
      }
    if (h.num_chains() != heads)
      return std::to_string(h.num_chains()) + " chains, not " + std::to_string(heads);
    return layout_error(t, pos, head);
  }

  struct Plus
  {
    long long operator () (long long a, long long b) const { return a + b; }
  };

  struct Greatest
  {
    long long operator () (long long a, long long b) const { return std::max(a, b); }
  };

  struct Least
  {
    long long operator () (long long a, long long b) const { return std::min(a, b); }
  };

  constexpr int Lowest = std::numeric_limits<int>::lowest();
  constexpr int Highest = std::numeric_limits<int>::max();
}

TYPED_TEST(ExhaustiveTreeQueries, HldFoldsAndLayoutMatchTheTree)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check layout("Gen_HLD, the tree and its layout");
  Check sum("HLD_Sum");
  Check max("HLD_Max");
  Check min("HLD_Min");
  Check user("Gen_HLD with an operation of the user");
  Check first("Gen_HLD rooted at the first node");

  std::mt19937 rng(seed(7332));
  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      std::vector<int> val;
      std::vector<Summary> sval;
      auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
      auto snv = [&sval] (Node * p) { return sval[size_t(num(p))]; };
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          val = random_values(rng, s.n);
          sval = summaries(val);
          const std::string at = "rooted at " + std::to_string(r) + ": ";

          const HLD_Sum<GT, int> hs(b.g, b.nodes[size_t(r)], nv);
          layout.run();
          if (auto e = tree_error(b, t, hs); not e.empty())
            layout.fail(s, at + e);
          else if (auto e2 = hld_layout_error(b, t, hs); not e2.empty())
            layout.fail(s, at + e2);

          sum.run();
          if (auto e = fold_error(b, t, hs, val, 0, Plus()); not e.empty())
            sum.fail(s, at + e);

          max.run();
          const HLD_Max<GT, int> hx(b.g, b.nodes[size_t(r)], nv);
          if (auto e = fold_error(b, t, hx, val, Lowest, Greatest(), false); not e.empty())
            max.fail(s, at + e);

          min.run();
          const HLD_Min<GT, int> hn(b.g, b.nodes[size_t(r)], nv);
          if (auto e = fold_error(b, t, hn, val, Highest, Least(), false); not e.empty())
            min.fail(s, at + e);

          user.run();
          const Gen_HLD<GT, Summary, Summary_Op> hu(b.g, b.nodes[size_t(r)], snv, Summary());
          if (auto e = fold_error(b, t, hu, sval, Summary(), Summary_Op()); not e.empty())
            user.fail(s, at + e);
        }

      // Without a root, the root is the first node
      first.run();
      const Rooted_Tree t(s, num(b.g.get_first_node()));
      const HLD_Sum<GT, int> hs(b.g, nv);
      const HLD_Max<GT, int> hx(b.g, nv);
      const HLD_Min<GT, int> hn(b.g, nv);
      const Gen_HLD<GT, Summary, Summary_Op> hu(b.g, snv, Summary());
      for (const auto & e : {tree_error(b, t, hs), hld_layout_error(b, t, hs),
                             fold_error(b, t, hs, val, 0, Plus(), false),
                             fold_error(b, t, hx, val, Lowest, Greatest(), false),
                             fold_error(b, t, hn, val, Highest, Least(), false),
                             tree_error(b, t, hu),
                             fold_error(b, t, hu, sval, Summary(), Summary_Op(), false)})
        if (not e.empty())
          {
            first.fail(s, e);
            break;
          }
    });

  layout.expect();
  sum.expect();
  max.expect();
  min.expect();
  user.expect();
  first.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, HldFoldsFollowPointUpdates)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check sum("HLD_Sum::point_update()");
  Check max("HLD_Max::point_update()");
  Check min("HLD_Min::point_update()");
  Check user("Gen_HLD::point_update() with an operation of the user");

  std::mt19937 rng(seed(7333));
  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          std::vector<int> val = random_values(rng, s.n);
          std::vector<Summary> sval = summaries(val);
          auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
          auto snv = [&sval] (Node * p) { return sval[size_t(num(p))]; };
          HLD_Sum<GT, int> hs(b.g, b.nodes[size_t(r)], nv);
          HLD_Max<GT, int> hx(b.g, b.nodes[size_t(r)], nv);
          HLD_Min<GT, int> hn(b.g, b.nodes[size_t(r)], nv);
          Gen_HLD<GT, Summary, Summary_Op> hu(b.g, b.nodes[size_t(r)], snv, Summary());

          // Two updates, the first by node and the second by id
          for (int k = 0; k < 2; ++k)
            {
              const int x = int(rng() % unsigned(s.n));
              const int v = int(rng() % 9) - 4;
              auto * p = b.nodes[size_t(x)];
              val[size_t(x)] = v;
              sval[size_t(x)] = summary_of(v);
              if (k == 0)
                {
                  hs.point_update(p, v);
                  hx.point_update(p, v);
                  hn.point_update(p, v);
                  hu.point_update(p, summary_of(v));
                }
              else
                {
                  hs.point_update_id(hs.id_of(p), v);
                  hx.point_update_id(hx.id_of(p), v);
                  hn.point_update_id(hn.id_of(p), v);
                  hu.point_update_id(hu.id_of(p), summary_of(v));
                }
              const std::string at = "rooted at " + std::to_string(r) + ", " + std::to_string(x)
                + " set to " + std::to_string(v) + ": ";

              sum.run();
              if (auto e = fold_error(b, t, hs, val, 0, Plus(), false); not e.empty())
                sum.fail(s, at + e);
              max.run();
              if (auto e = fold_error(b, t, hx, val, Lowest, Greatest(), false); not e.empty())
                max.fail(s, at + e);
              min.run();
              if (auto e = fold_error(b, t, hn, val, Highest, Least(), false); not e.empty())
                min.fail(s, at + e);
              user.run();
              if (auto e = fold_error(b, t, hu, sval, Summary(), Summary_Op(), false); not e.empty())
                user.fail(s, at + e);
            }
        }
    }, 2);

  sum.expect();
  max.expect();
  min.expect();
  user.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, HldSumTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Filter = Hide_Mask<GT>;
  using Engine = HLD_Sum<GT, int, Filter>;
  auto nv = [] (Node * p) { return value_of(num(p)); };
  expect_takes_the_trees<GT>("HLD_Sum", false, true,
    [&nv] (const Built<GT> & b, unsigned long hidden, Node * root, int r)
      {
        return r == -1 ? Engine(b.g, nv, Filter{hidden}) : Engine(b.g, root, nv, Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        if (auto err = tree_error(b, t, e); not err.empty())
          return err;
        return fold_error(b, t, e, values_of(b), 0, Plus(), false);
      });
}

TYPED_TEST(ExhaustiveTreeQueries, HldWithAnOperationOfTheUserTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Filter = Hide_Mask<GT>;
  using Engine = Gen_HLD<GT, Summary, Summary_Op, Filter>;
  auto nv = [] (Node * p) { return summary_of(value_of(num(p))); };
  expect_takes_the_trees<GT>("Gen_HLD with an operation of the user", false, false,
    [&nv] (const Built<GT> & b, unsigned long hidden, Node * root, int r)
      {
        return r == -1 ? Engine(b.g, nv, Summary(), Summary_Op(), Filter{hidden})
                       : Engine(b.g, root, nv, Summary(), Summary_Op(), Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        if (auto err = hld_layout_error(b, t, e); not err.empty())
          return err;
        return fold_error(b, t, e, summaries(values_of(b)), Summary(), Summary_Op(), false);
      });
}

TYPED_TEST(ExhaustiveTreeQueries, HldOnRandomTrees)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check layout("Gen_HLD, the tree and its layout");
  Check sum("HLD_Sum, with point updates");
  Check user("Gen_HLD with an operation of the user, with point updates");

  std::mt19937 rng(seed(7334));
  const int trees = heavy() ? 200 : 20;
  for (int k = 0; k < trees; ++k)
    {
      const Spec s = random_tree(rng, 8 + int(rng() % 41));
      Built<GT> b(s);
      const int r = int(rng() % unsigned(s.n));
      const Rooted_Tree t(s, r);
      std::vector<int> val = random_values(rng, s.n, -50, 50);
      std::vector<Summary> sval = summaries(val);
      auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
      auto snv = [&sval] (Node * p) { return sval[size_t(num(p))]; };
      HLD_Sum<GT, int> hs(b.g, b.nodes[size_t(r)], nv);
      Gen_HLD<GT, Summary, Summary_Op> hu(b.g, b.nodes[size_t(r)], snv, Summary());
      const std::string at = "rooted at " + std::to_string(r) + ": ";

      layout.run();
      if (auto e = tree_error(b, t, hs); not e.empty())
        layout.fail(s, at + e);
      else if (auto e2 = hld_layout_error(b, t, hs); not e2.empty())
        layout.fail(s, at + e2);

      for (int u = 0; u < 3; ++u)
        {
          sum.run();
          if (auto e = fold_error(b, t, hs, val, 0, Plus(), u == 0); not e.empty())
            sum.fail(s, at + e);
          user.run();
          if (auto e = fold_error(b, t, hu, sval, Summary(), Summary_Op(), u == 0); not e.empty())
            user.fail(s, at + e);
          const int x = int(rng() % unsigned(s.n));
          val[size_t(x)] = int(rng() % 101) - 50;
          sval[size_t(x)] = summary_of(val[size_t(x)]);
          hs.point_update(b.nodes[size_t(x)], val[size_t(x)]);
          hu.point_update_id(hu.id_of(b.nodes[size_t(x)]), sval[size_t(x)]);
        }
    }

  layout.expect();
  sum.expect();
  user.expect();
}

// --------------------------------------------------------------------------
// Tree_Decomposition.H
// --------------------------------------------------------------------------

namespace
{
  /** What is wrong with what a Gen_Heavy_Light_Decomposition says of the
      tree t of b, beyond tree_error(): the place of each node and the
      head of its chain, by id and by node, which must make a layout of
      layout_error(); the heavy child of each node, the child that its
      chain goes on to; the range of each subtree; and the segments of the
      path of each pair, which, each read in its direction, must give the
      nodes of the path in order. Empty if nothing. */
  template <class GT, class HLD>
  std::string decomposition_error(const Built<GT> & b, const Rooted_Tree & t, const HLD & h)
  {
    const int n = t.n;
    std::vector<size_t> id(static_cast<size_t>(n)), pos(static_cast<size_t>(n));
    std::vector<int> head(static_cast<size_t>(n)), at(size_t(n), -1);
    for (int x = 0; x < n; ++x)
      {
        auto * p = b.nodes[size_t(x)];
        id[size_t(x)] = h.id_of(p);
        pos[size_t(x)] = h.position_of_id(id[size_t(x)]);
        if (pos[size_t(x)] >= size_t(n) or h.position_of(p) != pos[size_t(x)])
          return "the place of " + std::to_string(x);
        at[pos[size_t(x)]] = x;
        const size_t hd = h.head_id(id[size_t(x)]);
        if (hd >= size_t(n) or h.head_of(p) != h.node_of(hd))
          return "the head of the chain of " + std::to_string(x);
        head[size_t(x)] = num(h.node_of(hd));
      }
    if (auto e = layout_error(t, pos, head); not e.empty())
      return e;

    for (int x = 0; x < n; ++x)
      {
        auto * p = b.nodes[size_t(x)];
        int going_on = -1;
        for (int c : t.children[size_t(x)])
          if (head[size_t(c)] == head[size_t(x)])
            going_on = c;
        if (h.heavy_child_id(id[size_t(x)]) != (going_on == -1 ? No_Id : id[size_t(going_on)])
            or h.heavy_child(p) != (going_on == -1 ? nullptr : b.nodes[size_t(going_on)]))
          return "the heavy child of " + std::to_string(x);
        const auto range = h.subtree_range_id(id[size_t(x)]);
        const auto by_node = h.subtree_range(p);
        if (range.left != pos[size_t(x)] or range.right != pos[size_t(x)] + size_t(t.size[size_t(x)]) - 1
            or by_node.left != range.left or by_node.right != range.right)
          return "the range of the subtree of " + std::to_string(x);
      }

    for (int u = 0; u < n; ++u)
      for (int v = 0; v < n; ++v)
        {
          std::vector<int> walk, walk_by_node, covered;
          bool bad = false;
          auto reader = [&] (std::vector<int> & out)
            {
              return [&bad, &out, &at, n] (size_t l, size_t r, bool reversed)
                {
                  if (l > r or r >= size_t(n))
                    {
                      bad = true;
                      return;
                    }
                  if (reversed)
                    for (size_t k = r + 1; k-- > l; )
                      out.push_back(at[k]);
                  else
                    for (size_t k = l; k <= r; ++k)
                      out.push_back(at[k]);
                };
            };
          h.for_each_path_segment_id(id[size_t(u)], id[size_t(v)], reader(walk));
          if (t.by_node(u, v))
            h.for_each_path_segment(b.nodes[size_t(u)], b.nodes[size_t(v)], reader(walk_by_node));
          else
            walk_by_node = t.path(u, v);
          h.for_each_path_segment_undirected_id(id[size_t(u)], id[size_t(v)],
                                                [&] (size_t l, size_t r)
                                                  {
                                                    reader(covered)(l, r, false);
                                                  });
          auto path = t.path(u, v);
          auto from = [&] { return " from " + std::to_string(u) + " to " + std::to_string(v); };
          if (bad or walk != path or walk_by_node != path)
            return "the segments of the path" + from() + " give " + text(walk);
          std::sort(path.begin(), path.end());
          std::sort(covered.begin(), covered.end());
          if (covered != path)
            return "the segments without direction of the path" + from() + " cover "
              + text(covered);
        }
    return "";
  }

  /** What is wrong with the folds of a Gen_HLD_Path_Query on the tree t of
      b whose node x has the value val[x], with the operation op of
      identity identity: its size and decomposition(), the value of each
      node, the fold over its subtree and over the path of each pair, by
      id, and by node as Rooted_Tree::by_node() says unless by_node is
      false. Empty if nothing. */
  template <class GT, class Query, typename T, class Op>
  std::string path_query_error(const Built<GT> & b, const Rooted_Tree & t, const Query & q,
                               const std::vector<T> & val, const T & identity, Op op,
                               bool by_node = true)
  {
    const int n = t.n;
    if (q.size() != size_t(n) or q.is_empty() != (n == 0))
      return "the size is " + std::to_string(q.size());
    if (by_node)
      if (auto e = tree_error(b, t, q.decomposition()); not e.empty())
        return "decomposition(): " + e;
    const auto & h = q.decomposition();
    std::vector<size_t> id(static_cast<size_t>(n));
    for (int x = 0; x < n; ++x)
      id[size_t(x)] = h.id_of(b.nodes[size_t(x)]);
    for (int x = 0; x < n; ++x)
      {
        auto * p = b.nodes[size_t(x)];
        const size_t i = id[size_t(x)];
        if (q.get_node_id(i) != val[size_t(x)] or (by_node and q.get_node(p) != val[size_t(x)]))
          return "the value of " + std::to_string(x);
        const T want = fold(t.subtree(x), val, identity, op);
        const T got = q.query_subtree_id(i);
        if (got != want or (by_node and q.query_subtree(p) != want))
          return "the subtree of " + std::to_string(x) + " gives " + show(got) + ", not "
            + show(want);
      }
    for (int u = 0; u < n; ++u)
      for (int v = 0; v < n; ++v)
        {
          auto * p = b.nodes[size_t(u)];
          auto * pv = b.nodes[size_t(v)];
          const T want = fold(t.path(u, v), val, identity, op);
          const T got = q.query_path_id(id[size_t(u)], id[size_t(v)]);
          if (got != want or (by_node and t.by_node(u, v) and q.query_path(p, pv) != want))
            return "the path from " + std::to_string(u) + " to " + std::to_string(v) + " gives "
              + show(got) + ", not " + show(want);
        }
    return "";
  }

  /** What is wrong with a Gen_Centroid_Decomposition of the tree t of b.
      For each node, its chain of centroid ancestors, from the root of the
      centroid tree down to the node itself, and the distances to them,
      by id, by node and through for_each_centroid_ancestor(); its parent
      and level in the centroid tree, and the length of its chain. Each
      centroid c must be a centroid of its part, the nodes whose chains
      hold it: the component of c once its centroid ancestors are taken
      out of the tree, in which no component left without c has more than
      half the nodes of the part. Empty if nothing. */
  template <class GT, class Centroids>
  std::string centroid_error(const Built<GT> & b, const Rooted_Tree & t, const Centroids & cd)
  {
    const int n = t.n;
    std::vector<size_t> id(static_cast<size_t>(n));
    for (int x = 0; x < n; ++x)
      id[size_t(x)] = cd.id_of(b.nodes[size_t(x)]);
    auto * top = cd.centroid_root();
    if (top == nullptr or cd.centroid_root_id() != cd.id_of(top))
      return "the root of the centroid tree";

    std::vector<std::vector<int>> chain(static_cast<size_t>(n));
    for (int x = 0; x < n; ++x)
      {
        auto * p = b.nodes[size_t(x)];
        const size_t i = id[size_t(x)];
        const std::string of = " of " + std::to_string(x);
        const auto & anc = cd.centroid_ancestors_of_id(i);
        const auto & dist = cd.centroid_distances_of_id(i);
        if (&cd.centroid_ancestors_of(p) != &anc or &cd.centroid_distances_of(p) != &dist)
          return "the chain" + of + " by node";
        if (anc.size() == 0 or anc.size() != dist.size())
          return "the chain" + of + " and its distances";
        for (size_t k = 0; k < anc.size(); ++k)
          {
            if (anc[k] >= size_t(n))
              return "a centroid ancestor" + of + " out of range";
            const int c = num(cd.node_of(anc[k]));
            chain[size_t(x)].push_back(c);
            if (dist[k] != size_t(t.distance(x, c)))
              return "the distance" + of + " to its centroid ancestor " + std::to_string(c);
          }
        if (chain[size_t(x)].back() != x or chain[size_t(x)].front() != num(top))
          return "the chain" + of + " is " + text(chain[size_t(x)]);

        std::vector<std::tuple<size_t, size_t, size_t>> seen, seen_by_node;
        cd.for_each_centroid_ancestor_id(i, [&] (size_t c, size_t d, size_t k)
          {
            seen.emplace_back(c, d, k);
          });
        cd.for_each_centroid_ancestor(p, [&] (size_t c, size_t d, size_t k)
          {
            seen_by_node.emplace_back(c, d, k);
          });
        bool same = seen.size() == anc.size() and seen == seen_by_node;
        for (size_t k = 0; same and k < seen.size(); ++k)
          same = seen[k] == std::make_tuple(anc[k], dist[k], k);
        if (not same)
          return "for_each_centroid_ancestor()" + of;

        const size_t len = chain[size_t(x)].size();
        const size_t parent = len == 1 ? No_Id : id[size_t(chain[size_t(x)][len - 2])];
        if (cd.centroid_parent_id(i) != parent
            or cd.centroid_parent(p) != (len == 1 ? nullptr : cd.node_of(parent))
            or cd.centroid_level_of_id(i) != len - 1 or cd.centroid_level_of(p) != len - 1
            or cd.centroid_path_length_of_id(i) != len or cd.centroid_path_length_of(p) != len)
          return "the parent, the level or the chain length" + of;
      }

    size_t deepest = 0;
    for (int c = 0; c < n; ++c)
      {
        const auto & ch = chain[size_t(c)];
        deepest = std::max(deepest, ch.size() - 1);
        if (ch.size() > 1 and std::vector<int>(ch.begin(), ch.end() - 1) != chain[size_t(ch[ch.size() - 2])])
          return "the chain of " + std::to_string(c) + " does not extend that of its parent";

        // The part of c, and the component of c without its ancestors
        std::vector<bool> part(size_t(n), false), out(size_t(n), false), reached(size_t(n), false);
        for (int x = 0; x < n; ++x)
          part[size_t(x)] = std::find(chain[size_t(x)].begin(), chain[size_t(x)].end(), c)
            != chain[size_t(x)].end();
        for (size_t k = 0; k + 1 < ch.size(); ++k)
          out[size_t(ch[k])] = true;
        std::vector<int> todo = {c};
        reached[size_t(c)] = true;
        while (not todo.empty())
          {
            const int x = todo.back();
            todo.pop_back();
            for (int y : t.adj[size_t(x)])
              if (not out[size_t(y)] and not reached[size_t(y)])
                {
                  reached[size_t(y)] = true;
                  todo.push_back(y);
                }
          }
        if (reached != part)
          return "the part of the centroid " + std::to_string(c)
            + " is not its component without its ancestors";

        const auto size = std::count(part.begin(), part.end(), true);
        std::vector<bool> taken(size_t(n), false);
        for (int y : t.adj[size_t(c)])
          if (part[size_t(y)] and not taken[size_t(y)])
            {
              long count = 0;
              std::vector<int> comp = {y};
              taken[size_t(y)] = true;
              while (not comp.empty())
                {
                  const int x = comp.back();
                  comp.pop_back();
                  ++count;
                  for (int z : t.adj[size_t(x)])
                    if (part[size_t(z)] and z != c and not taken[size_t(z)])
                      {
                        taken[size_t(z)] = true;
                        comp.push_back(z);
                      }
                }
              if (2 * count > size)
                return std::to_string(c) + " is not a centroid of its part";
            }
      }
    if (cd.max_centroid_level() != deepest or int(deepest) > floor_log2(size_t(n)))
      return "max_centroid_level() is " + std::to_string(cd.max_centroid_level()) + ", not "
        + std::to_string(deepest);

    for (int a = 0; a < n; ++a)
      for (int x = 0; x < n; ++x)
        {
          const bool in = std::find(chain[size_t(x)].begin(), chain[size_t(x)].end(), a)
            != chain[size_t(x)].end();
          if (cd.is_centroid_ancestor_id(id[size_t(a)], id[size_t(x)]) != in
              or cd.distance_to_centroid_id(id[size_t(x)], id[size_t(a)])
                 != (in ? size_t(t.distance(x, a)) : No_Id))
            return "whether " + std::to_string(a) + " is a centroid ancestor of "
              + std::to_string(x) + ", or how far";
        }
    return "";
  }
}

TYPED_TEST(ExhaustiveTreeQueries, HeavyLightDecompositionMatchesItsDefinition)
{
  using GT = TypeParam;
  Check hld("Gen_Heavy_Light_Decomposition");
  Check first("Gen_Heavy_Light_Decomposition rooted at the first node");

  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          hld.run();
          const Heavy_Light_Decomposition<GT> h(b.g, b.nodes[size_t(r)]);
          if (auto e = tree_error(b, t, h); not e.empty())
            hld.fail(s, "rooted at " + std::to_string(r) + ": " + e);
          else if (auto e2 = decomposition_error(b, t, h); not e2.empty())
            hld.fail(s, "rooted at " + std::to_string(r) + ": " + e2);
        }

      first.run();
      const Rooted_Tree t(s, num(b.g.get_first_node()));
      const Gen_Heavy_Light_Decomposition<GT> h(b.g);
      const Gen_Heavy_Light_Decomposition<GT> h_null(b.g, static_cast<typename GT::Node *>(nullptr));
      for (const auto & e : {tree_error(b, t, h), decomposition_error(b, t, h),
                             tree_error(b, t, h_null)})
        if (not e.empty())
          {
            first.fail(s, e);
            break;
          }
    });

  hld.expect();
  first.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, HldPathQueriesFoldAndFollowUpdates)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check info("Gen_HLD_Path_Query of the infos");
  Check getter("Gen_HLD_Path_Query with a getter");
  Check max("Gen_HLD_Path_Query with Max_Op");
  Check min("Gen_HLD_Path_Query with Min_Op");
  Check user("Gen_HLD_Path_Query with an operation of the user");
  Check first("Gen_HLD_Path_Query rooted at the first node");
  Check update("Gen_HLD_Path_Query::update_node()");
  Check set("Gen_HLD_Path_Query::set_node()");

  std::mt19937 rng(seed(7335));
  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      std::vector<int> numbers(size_t(s.n));
      for (int x = 0; x < s.n; ++x)
        numbers[size_t(x)] = x;
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          std::vector<int> val = random_values(rng, s.n);
          std::vector<Summary> sval = summaries(val);
          auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
          auto snv = [&sval] (Node * p) { return sval[size_t(num(p))]; };
          auto * root = b.nodes[size_t(r)];
          const std::string at = "rooted at " + std::to_string(r) + ": ";

          // The values are the infos, the numbers of the nodes
          info.run();
          const HLD_Path_Query<GT> qi(b.g, root, 0);
          if (auto e = path_query_error(b, t, qi, numbers, 0, Plus(), false); not e.empty())
            info.fail(s, at + e);

          getter.run();
          HLD_Path_Query<GT> q(b.g, root, nv, 0);
          if (auto e = path_query_error(b, t, q, val, 0, Plus()); not e.empty())
            getter.fail(s, at + e);

          max.run();
          const HLD_Path_Query<GT, int, Max_Op<int>> qx(b.g, root, nv, Lowest);
          if (auto e = path_query_error(b, t, qx, val, Lowest, Greatest(), false); not e.empty())
            max.fail(s, at + e);

          min.run();
          const HLD_Path_Query<GT, int, Min_Op<int>> qn(b.g, root, nv, Highest);
          if (auto e = path_query_error(b, t, qn, val, Highest, Least(), false); not e.empty())
            min.fail(s, at + e);

          user.run();
          Gen_HLD_Path_Query<GT, Summary, Summary_Op> qu(b.g, root, snv, Summary());
          if (auto e = path_query_error(b, t, qu, sval, Summary(), Summary_Op()); not e.empty())
            user.fail(s, at + e);

          // An update adds to the value, by node and by id; a set replaces it
          const int x = int(rng() % unsigned(s.n));
          const int delta = int(rng() % 5) - 2;
          update.run();
          q.update_node(b.nodes[size_t(x)], delta);
          qu.update_node_id(qu.decomposition().id_of(b.nodes[size_t(x)]), summary_of(delta));
          val[size_t(x)] += delta;
          sval[size_t(x)] = Summary_Op()(sval[size_t(x)], summary_of(delta));
          if (auto e = path_query_error(b, t, q, val, 0, Plus(), false); not e.empty())
            update.fail(s, at + "adding " + std::to_string(delta) + " to " + std::to_string(x)
                        + ": " + e);
          else if (auto e2 = path_query_error(b, t, qu, sval, Summary(), Summary_Op(), false); not e2.empty())
            update.fail(s, at + "adding to " + std::to_string(x) + ": " + e2);

          const int y = int(rng() % unsigned(s.n));
          const int v = int(rng() % 5) - 2;
          set.run();
          q.set_node_id(q.decomposition().id_of(b.nodes[size_t(y)]), v);
          qu.set_node(b.nodes[size_t(y)], summary_of(v));
          val[size_t(y)] = v;
          sval[size_t(y)] = summary_of(v);
          if (auto e = path_query_error(b, t, q, val, 0, Plus(), false); not e.empty())
            set.fail(s, at + "setting " + std::to_string(y) + " to " + std::to_string(v) + ": " + e);
          else if (auto e2 = path_query_error(b, t, qu, sval, Summary(), Summary_Op(), false); not e2.empty())
            set.fail(s, at + "setting " + std::to_string(y) + ": " + e2);
        }

      // Without a root, the root is the first node
      first.run();
      const Rooted_Tree t(s, num(b.g.get_first_node()));
      std::vector<int> val = random_values(rng, s.n);
      auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
      const HLD_Path_Query<GT> qi(b.g, 0);
      const HLD_Path_Query<GT> q(b.g, nv, 0);
      for (const auto & e : {path_query_error(b, t, qi, numbers, 0, Plus()),
                             path_query_error(b, t, q, val, 0, Plus())})
        if (not e.empty())
          {
            first.fail(s, e);
            break;
          }
    }, 2);

  info.expect();
  getter.expect();
  max.expect();
  min.expect();
  user.expect();
  first.expect();
  update.expect();
  set.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, CentroidDecompositionMatchesItsDefinition)
{
  using GT = TypeParam;
  Check centroids("Gen_Centroid_Decomposition");
  Check first("Gen_Centroid_Decomposition rooted at the first node");

  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          centroids.run();
          const Centroid_Decomposition<GT> cd(b.g, b.nodes[size_t(r)]);
          if (auto e = tree_error(b, t, cd); not e.empty())
            centroids.fail(s, "rooted at " + std::to_string(r) + ": " + e);
          else if (auto e2 = centroid_error(b, t, cd); not e2.empty())
            centroids.fail(s, "rooted at " + std::to_string(r) + ": " + e2);
        }

      first.run();
      const Rooted_Tree t(s, num(b.g.get_first_node()));
      const Gen_Centroid_Decomposition<GT> cd(b.g);
      if (auto e = tree_error(b, t, cd); not e.empty())
        first.fail(s, e);
      else if (auto e2 = centroid_error(b, t, cd); not e2.empty())
        first.fail(s, e2);
    });

  centroids.expect();
  first.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, HeavyLightDecompositionTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  using Engine = Gen_Heavy_Light_Decomposition<GT, Filter>;
  expect_takes_the_trees<GT>("Gen_Heavy_Light_Decomposition", false, true,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int r)
      {
        return r == -1 ? Engine(b.g, Filter{hidden}) : Engine(b.g, root, Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        if (auto err = tree_error(b, t, e); not err.empty())
          return err;
        return decomposition_error(b, t, e);
      });
}

TYPED_TEST(ExhaustiveTreeQueries, HldPathQueryTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  using Engine = Gen_HLD_Path_Query<GT, int, Aleph::plus<int>, Filter>;
  expect_takes_the_trees<GT>("Gen_HLD_Path_Query", false, false,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int r)
      {
        return r == -1 ? Engine(b.g, 0, Aleph::plus<int>(), Filter{hidden})
                       : Engine(b.g, root, 0, Aleph::plus<int>(), Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        // The values are the infos, the numbers of the nodes
        std::vector<int> numbers;
        for (int x = 0; x < t.n; ++x)
          numbers.push_back(x);
        return path_query_error(b, t, e, numbers, 0, Plus());
      });
}

TYPED_TEST(ExhaustiveTreeQueries, CentroidDecompositionTakesExactlyTheTrees)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  using Engine = Gen_Centroid_Decomposition<GT, Filter>;
  expect_takes_the_trees<GT>("Gen_Centroid_Decomposition", false, false,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int r)
      {
        return r == -1 ? Engine(b.g, Filter{hidden}) : Engine(b.g, root, Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        if (auto err = tree_error(b, t, e); not err.empty())
          return err;
        return centroid_error(b, t, e);
      });
}

TYPED_TEST(ExhaustiveTreeQueries, TreeDecompositionsOnRandomTrees)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check hld("Gen_Heavy_Light_Decomposition");
  Check query("Gen_HLD_Path_Query, with updates");
  Check centroids("Gen_Centroid_Decomposition");

  std::mt19937 rng(seed(7336));
  const int trees = heavy() ? 200 : 20;
  for (int k = 0; k < trees; ++k)
    {
      const Spec s = random_tree(rng, 8 + int(rng() % 41));
      Built<GT> b(s);
      const int r = int(rng() % unsigned(s.n));
      const Rooted_Tree t(s, r);
      const std::string at = "rooted at " + std::to_string(r) + ": ";

      hld.run();
      const Gen_Heavy_Light_Decomposition<GT> h(b.g, b.nodes[size_t(r)]);
      if (auto e = tree_error(b, t, h); not e.empty())
        hld.fail(s, at + e);
      else if (auto e2 = decomposition_error(b, t, h); not e2.empty())
        hld.fail(s, at + e2);

      std::vector<int> val = random_values(rng, s.n, -50, 50);
      auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
      Gen_HLD_Path_Query<GT> q(b.g, b.nodes[size_t(r)], nv, 0);
      for (int u = 0; u < 3; ++u)
        {
          query.run();
          if (auto e = path_query_error(b, t, q, val, 0, Plus(), u == 0); not e.empty())
            query.fail(s, at + e);
          const int x = int(rng() % unsigned(s.n));
          const int delta = int(rng() % 21) - 10;
          val[size_t(x)] += delta;
          q.update_node(b.nodes[size_t(x)], delta);
        }

      centroids.run();
      const Gen_Centroid_Decomposition<GT> cd(b.g, b.nodes[size_t(r)]);
      if (auto e = tree_error(b, t, cd); not e.empty())
        centroids.fail(s, at + e);
      else if (auto e2 = centroid_error(b, t, cd); not e2.empty())
        centroids.fail(s, at + e2);
    }

  hld.expect();
  query.expect();
  centroids.expect();
}

// --------------------------------------------------------------------------
// Tree_DP.H
// --------------------------------------------------------------------------

namespace
{
  /// The label of x in the texts of the trees: one character, neither
  /// of the parentheses, for up to 79 nodes
  char label(int x) { return char('0' + x); }

  /// The canonical text of the subtree of x in t: the label of x, then the
  /// texts of the subtrees of its children, each in parentheses, sorted
  std::string canonical(const Rooted_Tree & t, int x)
  {
    std::vector<std::string> subs;
    for (int c : t.children[size_t(x)])
      subs.push_back("(" + canonical(t, c) + ")");
    std::sort(subs.begin(), subs.end());
    std::string r(1, label(x));
    for (const auto & sub : subs)
      r += sub;
    return r;
  }

  /// The canonical text of the tree that text writes from pos on, as a
  /// label and its subtrees in parentheses, in any order; empty if it is
  /// not well written
  std::string canonical(const std::string & text, size_t & pos)
  {
    if (pos >= text.size() or text[pos] == '(' or text[pos] == ')')
      return "";
    std::string r(1, text[pos++]);
    std::vector<std::string> subs;
    while (pos < text.size() and text[pos] == '(')
      {
        ++pos;
        const std::string sub = canonical(text, pos);
        if (sub.empty() or pos >= text.size() or text[pos] != ')')
          return "";
        ++pos;
        subs.push_back("(" + sub + ")");
      }
    std::sort(subs.begin(), subs.end());
    for (const auto & sub : subs)
      r += sub;
    return r;
  }

  /// The canonical text of a tree written as a whole by text
  std::string canonical(const std::string & text)
  {
    size_t pos = 0;
    std::string r = canonical(text, pos);
    return pos == text.size() ? r : "";
  }

  /** What is wrong with a Gen_Tree_DP of the sums of the values val over
      the subtrees of t: its size and its ids, and the value of each node
      by node and in values(). Empty if nothing. */
  template <class GT, class DP>
  std::string subtree_sums_error(const Built<GT> & b, const Rooted_Tree & t, const DP & dp,
                                 const std::vector<int> & val)
  {
    if (auto e = tree_error(b, t, dp); not e.empty())
      return e;
    if (dp.values().size() != size_t(t.n))
      return std::to_string(dp.values().size()) + " values";
    for (int x = 0; x < t.n; ++x)
      {
        long long want = 0;
        for (int y : t.subtree(x))
          want += val[size_t(y)];
        auto * p = b.nodes[size_t(x)];
        if (dp.value(p) != want or dp.values()[dp.id_of(p)] != want)
          return "the subtree of " + std::to_string(x) + " sums " + std::to_string(dp.value(p))
            + ", not " + std::to_string(want);
      }
    return "";
  }

  /** What is wrong with the sums of the distances and the eccentricities
      that Gen_Reroot_DP and the functions give on the tree t of b, from
      each node: the sum of the values val, with that of each node by its
      distance (a Gen_Reroot_DP of pairs), the greatest distance
      (tree_max_distance()) and the sum of the distances
      (tree_sum_of_distances()). Empty if nothing. */
  template <class GT, class Reroot, class Max, class Sums>
  std::string distances_error(const Built<GT> & b, const Rooted_Tree & t, const Reroot & dp,
                              const Max & ecc, const Sums & sums, const std::vector<int> & val)
  {
    if (dp.size() != size_t(t.n) or dp.values().size() != size_t(t.n)
        or ecc.size() != size_t(t.n) or sums.size() != size_t(t.n))
      return "an answer without a place for each node";
    long long total = 0;
    for (int y = 0; y < t.n; ++y)
      total += val[size_t(y)];
    for (int x = 0; x < t.n; ++x)
      {
        auto * p = b.nodes[size_t(x)];
        const size_t i = dp.id_of(p);
        long long weighted = 0, far = 0, sum = 0;
        for (int y = 0; y < t.n; ++y)
          {
            const int d = t.distance(x, y);
            weighted += val[size_t(y)] * d;
            far = std::max<long long>(far, d);
            sum += d;
          }
        const std::string from = " from " + std::to_string(x);
        if (dp.value(p) != std::make_pair(total, weighted) or dp.values()[i] != dp.value(p))
          return "the weighted sum of the distances" + from + " is "
            + std::to_string(dp.value(p).second) + ", not " + std::to_string(weighted);
        if (ecc[i] != size_t(far))
          return "the greatest distance" + from + " is " + std::to_string(ecc[i]) + ", not "
            + std::to_string(far);
        if (sums[i] != size_t(sum))
          return "the sum of the distances" + from + " is " + std::to_string(sums[i]) + ", not "
            + std::to_string(sum);
      }
    return "";
  }

  /// A Gen_Tree_DP that writes the text of each subtree, and records in
  /// wrong whether a combination did not get a parent with its child
  template <class GT, class SA = Dft_Show_Arc<GT>>
  Tree_DP<GT, std::string, SA> shape_dp(const Built<GT> & b, const Rooted_Tree & t,
                                         typename GT::Node * root, bool & wrong, SA sa = SA())
  {
    using Node = typename GT::Node;
    return Tree_DP<GT, std::string, SA>
      (b.g, root, [] (Node * p) { return std::string(1, label(num(p))); },
       [&t, &wrong] (Node * par, const std::string & acc, Node * child, const std::string & c)
         {
           if (t.parent[size_t(num(child))] != num(par) or acc.empty() or acc[0] != label(num(par))
               or c.empty() or c[0] != label(num(child)))
             wrong = true;
           return acc + "(" + c + ")";
         }, std::move(sa));
  }

  /// A Gen_Reroot_DP that writes the text of the tree from each node, and
  /// records in wrong whether an arc was applied to the wrong nodes
  template <class GT, class SA = Dft_Show_Arc<GT>>
  Reroot_DP<GT, std::string, SA> reroot_shape_dp(const Built<GT> & b, const Rooted_Tree & t,
                                                 typename GT::Node * root, bool & wrong,
                                                 SA sa = SA())
  {
    using Node = typename GT::Node;
    return Reroot_DP<GT, std::string, SA>
      (b.g, root, std::string(), [] (Node * p) { return std::string(1, label(num(p))); },
       [] (const std::string & x, const std::string & y) { return x + y; },
       [&t, &wrong] (Node * p, Node * c, const std::string & x)
         {
           if (t.distance(num(p), num(c)) != 1 or x.empty() or x[0] != label(num(c)))
             wrong = true;
           return "(" + x + ")";
         }, std::move(sa));
  }

  using Pair = std::pair<long long, long long>;

  /// A Gen_Reroot_DP of the sum of the values val and of the sum of each
  /// one by its distance
  template <class GT, class SA = Dft_Show_Arc<GT>>
  Gen_Reroot_DP<GT, Pair, SA> weighted_dp(const Built<GT> & b, typename GT::Node * root,
                                          const std::vector<int> & val, SA sa = SA())
  {
    using Node = typename GT::Node;
    return Gen_Reroot_DP<GT, Pair, SA>
      (b.g, root, Pair(0, 0), [&val] (Node * p) { return Pair(val[size_t(num(p))], 0); },
       [] (const Pair & x, const Pair & y) { return Pair(x.first + y.first, x.second + y.second); },
       [] (Node *, Node *, const Pair & x) { return Pair(x.first, x.second + x.first); },
       std::move(sa));
  }

  /// A Gen_Tree_DP of the sums of the values val over the subtrees
  template <class GT, class SA = Dft_Show_Arc<GT>>
  Gen_Tree_DP<GT, long long, SA> sums_dp(const Built<GT> & b, typename GT::Node * root,
                                         const std::vector<int> & val, SA sa = SA())
  {
    using Node = typename GT::Node;
    return Gen_Tree_DP<GT, long long, SA>
      (b.g, root, [&val] (Node * p) { return (long long) val[size_t(num(p))]; },
       [] (Node *, const long long & acc, Node *, const long long & c) { return acc + c; },
       std::move(sa));
  }
}

template <class GT>
class ExhaustiveTreeDp : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveTreeDp, All_Kinds);

TYPED_TEST(ExhaustiveTreeDp, TreeDpFoldsEverySubtree)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check sums("Gen_Tree_DP, the sums over the subtrees");
  Check shape("Gen_Tree_DP, the subtrees that the combination sees");
  Check sizes("tree_subtree_sizes()");
  Check first("Gen_Tree_DP with the null root, rooted at the first node");

  std::mt19937 rng(seed(7337));
  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int r = -1; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r == -1 ? num(b.g.get_first_node()) : r);
          Node * root = r == -1 ? nullptr : b.nodes[size_t(r)];
          const std::string at = r == -1 ? "" : "rooted at " + std::to_string(r) + ": ";
          const auto val = random_values(rng, s.n);
          Check & c = r == -1 ? first : sums;

          c.run();
          const auto dp = sums_dp(b, root, val);
          if (auto e = subtree_sums_error(b, t, dp, val); not e.empty())
            c.fail(s, at + e);

          shape.run();
          bool wrong = false;
          const auto sh = shape_dp(b, t, root, wrong);
          for (int x = 0; x < s.n and not wrong; ++x)
            if (canonical(sh.value(b.nodes[size_t(x)])) != canonical(t, x))
              {
                shape.fail(s, at + "the subtree of " + std::to_string(x) + " is "
                           + sh.value(b.nodes[size_t(x)]) + ", not " + canonical(t, x));
                break;
              }
          if (wrong)
            shape.fail(s, at + "a combination of a node with one that is not its child");

          sizes.run();
          const auto z = tree_subtree_sizes(b.g, root);
          bool ok = z.size() == size_t(s.n);
          for (int x = 0; ok and x < s.n; ++x)
            ok = z[dp.id_of(b.nodes[size_t(x)])] == size_t(t.size[size_t(x)]);
          if (not ok)
            sizes.fail(s, at + "wrong sizes");
        }
    });

  sums.expect();
  shape.expect();
  sizes.expect();
  first.expect();
}

TYPED_TEST(ExhaustiveTreeDp, RerootDpAnswersForEveryRoot)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check shape("Gen_Reroot_DP, the tree from each node");
  Check distances("Gen_Reroot_DP, tree_max_distance() and tree_sum_of_distances()");

  std::mt19937 rng(seed(7338));
  for_each_test_tree([&] (const Spec & s)
    {
      Built<GT> b(s);
      std::vector<std::string> canon;
      for (int x = 0; x < s.n; ++x)
        canon.push_back(canonical(Rooted_Tree(s, x), x));
      for (int r = -1; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r == -1 ? num(b.g.get_first_node()) : r);
          Node * root = r == -1 ? nullptr : b.nodes[size_t(r)];
          const std::string at = r == -1 ? "the null root: " : "rooted at " + std::to_string(r) + ": ";

          shape.run();
          bool wrong = false;
          const auto sh = reroot_shape_dp(b, t, root, wrong);
          if (auto e = tree_error(b, t, sh); not e.empty())
            shape.fail(s, at + e);
          else
            for (int x = 0; x < s.n and not wrong; ++x)
              if (canonical(sh.value(b.nodes[size_t(x)])) != canon[size_t(x)]
                  or sh.values()[sh.id_of(b.nodes[size_t(x)])] != sh.value(b.nodes[size_t(x)]))
                {
                  shape.fail(s, at + "the tree from " + std::to_string(x) + " is "
                             + sh.value(b.nodes[size_t(x)]) + ", not " + canon[size_t(x)]);
                  break;
                }
          if (wrong)
            shape.fail(s, at + "an arc applied to two nodes that it does not join");

          distances.run();
          const auto val = random_values(rng, s.n);
          if (auto e = distances_error(b, t, weighted_dp(b, root, val),
                                       tree_max_distance(b.g, root),
                                       tree_sum_of_distances(b.g, root), val);
              not e.empty())
            distances.fail(s, at + e);
        }
    }, 2);

  shape.expect();
  distances.expect();
}

TYPED_TEST(ExhaustiveTreeDp, TreeDpTakesTheTreesAndCountsParallelArcsOnce)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  // No constructor without a root: -1 is the null root, as -2
  expect_takes_the_trees<GT>("Gen_Tree_DP", true, true,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int)
      {
        return sums_dp(b, root, values_of(b), Filter{hidden});
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        return subtree_sums_error(b, t, e, values_of(b));
      });
}

namespace
{
  /// A Gen_Reroot_DP of the distances and the functions of Tree_DP.H on
  /// one graph, which take the same graphs
  template <class GT, class SA>
  struct Distances
  {
    Gen_Reroot_DP<GT, Pair, SA> dp;
    Array<size_t> ecc;
    Array<size_t> sums;

    [[nodiscard]] size_t size() const noexcept { return dp.size(); }
  };
}

TYPED_TEST(ExhaustiveTreeDp, RerootDpTakesTheTreesAndCountsParallelArcsOnce)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  expect_takes_the_trees<GT>("Gen_Reroot_DP, tree_max_distance() and tree_sum_of_distances()", true, false,
    [] (const Built<GT> & b, unsigned long hidden, typename GT::Node * root, int)
      {
        return Distances<GT, Filter>{weighted_dp(b, root, values_of(b), Filter{hidden}),
                                     tree_max_distance(b.g, root, Filter{hidden}),
                                     tree_sum_of_distances(b.g, root, Filter{hidden})};
      },
    [] (const Built<GT> & b, const auto & e, const Rooted_Tree & t)
      {
        return distances_error(b, t, e.dp, e.ecc, e.sums, values_of(b));
      });
}

TYPED_TEST(ExhaustiveTreeDp, TreeDpOnRandomTrees)
{
  using GT = TypeParam;
  Check sums("Gen_Tree_DP, the sums over the subtrees");
  Check shape("Gen_Reroot_DP, the tree from each node");
  Check distances("Gen_Reroot_DP, tree_max_distance() and tree_sum_of_distances()");

  std::mt19937 rng(seed(7339));
  const int trees = heavy() ? 200 : 20;
  for (int k = 0; k < trees; ++k)
    {
      const Spec s = random_tree(rng, 8 + int(rng() % 41));
      Built<GT> b(s);
      const int r = int(rng() % unsigned(s.n));
      const Rooted_Tree t(s, r);
      const auto val = random_values(rng, s.n, -50, 50);
      const std::string at = "rooted at " + std::to_string(r) + ": ";

      sums.run();
      if (auto e = subtree_sums_error(b, t, sums_dp(b, b.nodes[size_t(r)], val), val); not e.empty())
        sums.fail(s, at + e);

      shape.run();
      bool wrong = false;
      const auto sh = reroot_shape_dp(b, t, b.nodes[size_t(r)], wrong);
      for (int x = 0; x < s.n and not wrong; ++x)
        if (canonical(sh.value(b.nodes[size_t(x)])) != canonical(Rooted_Tree(s, x), x))
          {
            shape.fail(s, at + "the tree from " + std::to_string(x));
            break;
          }
      if (wrong)
        shape.fail(s, at + "an arc applied to two nodes that it does not join");

      distances.run();
      if (auto e = distances_error(b, t, weighted_dp(b, b.nodes[size_t(r)], val),
                                   tree_max_distance(b.g, b.nodes[size_t(r)]),
                                   tree_sum_of_distances(b.g, b.nodes[size_t(r)]), val);
          not e.empty())
        distances.fail(s, at + e);
    }

  sums.expect();
  shape.expect();
  distances.expect();
}

// --------------------------------------------------------------------------
// tpl_mo_on_trees.H
// --------------------------------------------------------------------------

namespace
{
  /// A policy of the user for Mo: the values in the window, sorted, or
  /// {-1000} once it is asked to remove one that is not there
  struct Window_Values
  {
    using answer_type = std::vector<int>;

    std::multiset<int> window;
    bool broken = false;

    void init(const Array<int> &, size_t)
    {
      window.clear();
      broken = false;
    }

    void add(const Array<int> & data, size_t i) { window.insert(data[i]); }

    void remove(const Array<int> & data, size_t i)
    {
      const auto it = window.find(data[i]);
      if (it == window.end())
        broken = true;
      else
        window.erase(it);
    }

    [[nodiscard]] answer_type answer() const
    {
      if (broken)
        return {-1000};
      return {window.begin(), window.end()};
    }
  };

  /// What is wrong with the answer of Window_Values on the nodes: their
  /// values, sorted
  struct Window_Of
  {
    const std::vector<int> * val = nullptr;

    std::string operator () (const std::vector<int> & nodes, const std::vector<int> & got) const
    {
      std::vector<int> want;
      for (int x : nodes)
        want.push_back((*val)[size_t(x)]);
      std::sort(want.begin(), want.end());
      return got == want ? "" : "gives " + text(got) + ", not " + text(want);
    }
  };

  /// What is wrong with the answer of Distinct_Count_Policy on the nodes:
  /// how many distinct values they have
  struct Distinct_Of
  {
    const std::vector<int> * val = nullptr;

    std::string operator () (const std::vector<int> & nodes, size_t got) const
    {
      std::set<int> seen;
      for (int x : nodes)
        seen.insert((*val)[size_t(x)]);
      return got == seen.size() ? "" : "gives " + std::to_string(got) + ", not "
        + std::to_string(seen.size());
    }
  };

  /// What is wrong with the answer of Powerful_Array_Policy on the nodes:
  /// the sum of each value by the square of its count
  struct Powerful_Of
  {
    const std::vector<int> * val = nullptr;

    std::string operator () (const std::vector<int> & nodes, long long got) const
    {
      std::vector<int> v;
      for (int x : nodes)
        v.push_back((*val)[size_t(x)]);
      long long want = 0;
      for (int a : std::set<int>(v.begin(), v.end()))
        {
          const long long c = std::count(v.begin(), v.end(), a);
          want += c * c * a;
        }
      return got == want ? "" : "gives " + std::to_string(got) + ", not " + std::to_string(want);
    }
  };

  /// What is wrong with the answer of Range_Mode_Policy on the nodes: the
  /// greatest count of a value, and a value with that count
  struct Mode_Of
  {
    const std::vector<int> * val = nullptr;

    std::string operator () (const std::vector<int> & nodes, const std::pair<size_t, int> & got) const
    {
      std::vector<int> v;
      for (int x : nodes)
        v.push_back((*val)[size_t(x)]);
      size_t most = 0;
      for (int a : v)
        most = std::max(most, size_t(std::count(v.begin(), v.end(), a)));
      if (got.first != most or size_t(std::count(v.begin(), v.end(), got.second)) != most)
        return "gives the value " + std::to_string(got.second) + " counted "
          + std::to_string(got.first) + ", which is no mode of " + text(v);
      return "";
    }
  };

  /** What is wrong with the answers of an engine of Mo whose nodes are
      node[x], on the nodes of part, a subtree of t: on the subtree of each
      node and on the path of each pair, all at once and again two of each
      in the lists of the overloads, against want(nodes, answer). Empty if
      nothing. */
  template <class Mo, class NodePtr, class Want>
  std::string mo_error(const Mo & mo, const std::vector<NodePtr> & node, const Rooted_Tree & t,
                       const std::vector<int> & part, Want want)
  {
    if (mo.size() != part.size() or mo.is_empty() != part.empty())
      return "the size is " + std::to_string(mo.size());

    Array<NodePtr> roots;
    for (int x : part)
      roots.append(node[size_t(x)]);
    const auto sub = mo.subtree_solve(roots);
    if (sub.size() != part.size())
      return std::to_string(sub.size()) + " answers for the subtrees";
    for (size_t k = 0; k < part.size(); ++k)
      if (auto e = want(t.subtree(part[k]), sub[k]); not e.empty())
        return "the subtree of " + std::to_string(part[k]) + " " + e;

    Array<std::pair<NodePtr, NodePtr>> pairs;
    for (int u : part)
      for (int v : part)
        pairs.append(std::make_pair(node[size_t(u)], node[size_t(v)]));
    const auto paths = mo.path_solve(pairs);
    if (paths.size() != pairs.size())
      return std::to_string(paths.size()) + " answers for the paths";
    size_t k = 0;
    for (int u : part)
      for (int v : part)
        if (auto e = want(t.path(u, v), paths[k++]); not e.empty())
          return "the path from " + std::to_string(u) + " to " + std::to_string(v) + " " + e;

    const int a = part.front(), z = part.back();
    const auto il_sub = mo.subtree_solve({node[size_t(z)], node[size_t(a)]});
    const auto il_paths = mo.path_solve({{node[size_t(z)], node[size_t(a)]},
                                         {node[size_t(a)], node[size_t(z)]}});
    if (il_sub.size() != 2 or il_paths.size() != 2
        or not want(t.subtree(z), il_sub[0]).empty() or not want(t.subtree(a), il_sub[1]).empty()
        or not want(t.path(z, a), il_paths[0]).empty() or not want(t.path(a, z), il_paths[1]).empty())
      return "the overloads that take a list";
    if (mo.subtree_solve(Array<NodePtr>()).size() != 0
        or mo.path_solve(Array<std::pair<NodePtr, NodePtr>>()).size() != 0)
      return "answers without queries";
    return "";
  }

  /// Leaves in each node of b the value val[x] of its number x
  template <class GT>
  void set_values(Built<GT> & b, const std::vector<int> & val)
  {
    for (size_t x = 0; x < b.nodes.size(); ++x)
      b.nodes[x]->get_info() = val[x];
  }

  /// Runs the four policies of Mo on the tree t of s, rooted at its
  /// root, with the values val
  template <class GT>
  void check_mo(const Spec & s, const Rooted_Tree & t, const std::vector<int> & val,
                const std::string & at, Check & window, Check & distinct, Check & powerful,
                Check & mode)
  {
    std::vector<int> all(size_t(s.n)), numbers(size_t(s.n));
    for (int x = 0; x < s.n; ++x)
      all[size_t(x)] = numbers[size_t(x)] = x;

    // The policy of the user sees the numbers of the nodes; the others,
    // their values
    window.run();
    {
      Built<GT> b(s);
      const auto before = attributes(b);
      const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, b.nodes[size_t(t.root)]);
      if (auto e = mo_error(mo, b.nodes, t, all, Window_Of{&numbers}); not e.empty())
        window.fail(s, at + e);
      else if (attributes(b) != before)
        window.fail(s, at + "the graph changed");
    }

    Built<GT> b(s);
    set_values(b, val);
    auto * root = b.nodes[size_t(t.root)];
    distinct.run();
    if (auto e = mo_error(Distinct_Count_Mo_On_Trees<GT>(b.g, root), b.nodes, t, all,
                          Distinct_Of{&val}); not e.empty())
      distinct.fail(s, at + e);
    powerful.run();
    if (auto e = mo_error(Powerful_Array_Mo_On_Trees<GT>(b.g, root), b.nodes, t, all,
                          Powerful_Of{&val}); not e.empty())
      powerful.fail(s, at + e);
    mode.run();
    if (auto e = mo_error(Range_Mode_Mo_On_Trees<GT>(b.g, root), b.nodes, t, all,
                          Mode_Of{&val}); not e.empty())
      mode.fail(s, at + e);
  }
}

template <class GT>
class ExhaustiveMoOnTrees : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveMoOnTrees, All_Kinds);

TYPED_TEST(ExhaustiveMoOnTrees, MoAnswersOnEverySubtreeAndPath)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check window("Gen_Mo_On_Trees with a policy of the user");
  Check distinct("Distinct_Count_Mo_On_Trees");
  Check powerful("Powerful_Array_Mo_On_Trees");
  Check mode("Range_Mode_Mo_On_Trees");
  Check directions("Gen_Mo_On_Trees on a digraph with its arcs as inserted");

  std::mt19937 rng(seed(7340));
  for_each_test_tree([&] (const Spec & s)
    {
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          const auto val = random_values(rng, s.n);
          const std::string at = "rooted at " + std::to_string(r) + ": ";
          // A digraph has its arcs from the parents to the children
          check_mo<GT>(digraph ? from_root(s, t) : s, t, val, at, window, distinct, powerful, mode);

          // As inserted, the arcs of a digraph may not reach every node
          if (not digraph)
            continue;
          directions.run();
          Built<GT> b(s);
          try
            {
              const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, b.nodes[size_t(r)]);
              std::vector<int> numbers(size_t(s.n)), all(size_t(s.n));
              for (int x = 0; x < s.n; ++x)
                numbers[size_t(x)] = all[size_t(x)] = x;
              if (not reaches_all(s, r))
                directions.fail(s, at + "takes a root that does not reach every node");
              else if (auto e = mo_error(mo, b.nodes, t, all, Window_Of{&numbers}); not e.empty())
                directions.fail(s, at + e);
            }
          catch (const std::domain_error &)
            {
              if (reaches_all(s, r))
                directions.fail(s, at + "rejects a root that reaches every node");
            }
        }
    }, 4);

  window.expect();
  distinct.expect();
  powerful.expect();
  mode.expect();
  directions.expect();
}

TYPED_TEST(ExhaustiveMoOnTrees, MoTakesWhatItDocuments)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check trees("Gen_Mo_On_Trees, the trees");
  Check rejected("Gen_Mo_On_Trees, a graph whose root does not reach every node");
  Check others("Gen_Mo_On_Trees, a connected graph that is not a tree");
  Check empty("Gen_Mo_On_Trees, the empty graph with the null root");
  Check empty_foreign("Gen_Mo_On_Trees, the empty graph with a root of another graph");
  Check foreign("Gen_Mo_On_Trees, a null root or one of another graph");
  Check queries("Gen_Mo_On_Trees, the queries on nodes out of the tree");
  Built<GT> other(Spec{1, {}, {}});

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      std::vector<int> numbers(size_t(s.n)), all(size_t(s.n));
      for (int x = 0; x < s.n; ++x)
        numbers[size_t(x)] = all[size_t(x)] = x;

      if (s.n == 0)
        {
          empty.run();
          try
            {
              const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, nullptr);
              if (mo.size() != 0 or not mo.is_empty()
                  or mo.subtree_solve(Array<typename GT::Node *>()).size() != 0
                  or not throws<std::domain_error>([&] { (void) mo.subtree_solve({other.nodes[0]}); }))
                empty.fail(s, "an engine that is not empty");
            }
          catch (const std::exception & ex)
            {
              empty.fail(s, std::string("throws ") + ex.what());
            }
          empty_foreign.run();
          if (not throws<std::domain_error>([&]
                { const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, other.nodes[0]); }))
            empty_foreign.fail(s, "takes a root of another graph");
          return;
        }

      for (int r = 0; r < s.n; ++r)
        {
          // What it must take: the trees, as their arcs go from the root
          // in a digraph; and what it must reject: the graphs whose root
          // does not reach every node. A connected graph that is not a
          // tree breaks the precondition: it only must not crash.
          const bool reached = digraph ? reaches_all(s, r) : oracle::components(s) == 1;
          const bool tree = is_tree(s, 0) and (not digraph or reached);
          Check & c = tree ? trees : reached ? others : rejected;
          const std::string at = "rooted at " + std::to_string(r) + ": ";
          c.run();
          try
            {
              const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, b.nodes[size_t(r)]);
              if (not reached)
                c.fail(s, at + "takes a root that does not reach every node");
              else if (tree)
                {
                  const Rooted_Tree t(s, r);
                  if (auto e = mo_error(mo, b.nodes, t, all, Window_Of{&numbers}); not e.empty())
                    c.fail(s, at + e);
                }
              else
                (void) mo.path_solve({{b.nodes[0], b.nodes[size_t(s.n - 1)]}});

              // The queries on nodes out of the tree, from the last root
              if (r != s.n - 1)
                continue;
              queries.run();
              using Node = typename GT::Node;
              if (not throws<std::domain_error>([&] { (void) mo.subtree_solve({other.nodes[0]}); })
                  or not throws<std::domain_error>([&] { (void) mo.subtree_solve({static_cast<Node *>(nullptr)}); })
                  or not throws<std::domain_error>([&] { (void) mo.path_solve({{b.nodes[0], other.nodes[0]}}); })
                  or not throws<std::domain_error>([&] { (void) mo.path_solve({{static_cast<Node *>(nullptr), b.nodes[0]}}); }))
                queries.fail(s, at + "answers on a node out of the tree");
            }
          catch (const std::domain_error &)
            {
              if (tree)
                c.fail(s, at + "rejects a tree");
            }
        }

      foreign.run();
      if (not throws<std::domain_error>([&] { const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, nullptr); })
          or not throws<std::domain_error>([&] { const Gen_Mo_On_Trees<GT, Window_Values> mo(b.g, other.nodes[0]); }))
        foreign.fail(s, "takes a null root or one of another graph");
    });

  trees.expect();
  rejected.expect();
  others.expect();
  empty.expect();
  empty_foreign.expect();
  foreign.expect();
  queries.expect();
}

TYPED_TEST(ExhaustiveMoOnTrees, MoOnRandomTrees)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check window("Gen_Mo_On_Trees with a policy of the user");
  Check distinct("Distinct_Count_Mo_On_Trees");
  Check powerful("Powerful_Array_Mo_On_Trees");
  Check mode("Range_Mode_Mo_On_Trees");

  std::mt19937 rng(seed(7341));
  const int trees = heavy() ? 100 : 10;
  for (int k = 0; k < trees; ++k)
    {
      const Spec s = random_tree(rng, 8 + int(rng() % 33));
      const int r = int(rng() % unsigned(s.n));
      const Rooted_Tree t(s, r);
      const auto val = random_values(rng, s.n, -3, 3);
      check_mo<GT>(digraph ? from_root(s, t) : s, t, val, "rooted at " + std::to_string(r) + ": ",
                   window, distinct, powerful, mode);
    }

  window.expect();
  distinct.expect();
  powerful.expect();
  mode.expect();
}

namespace
{
  /// The tree t as Tree_Node, each with its number as its key, with the
  /// children in increasing order or, if reversed, decreasing; node[x] is
  /// the node of x
  Tree_Node<int> * tree_nodes(const Rooted_Tree & t, std::vector<Tree_Node<int> *> & node,
                              bool reversed)
  {
    node.assign(size_t(t.n), nullptr);
    for (int x = 0; x < t.n; ++x)
      node[size_t(x)] = new Tree_Node<int>(x);
    // From the root down, so that each child is still a leaf when it goes in
    for (int x : t.order)
      {
        auto ch = t.children[size_t(x)];
        if (reversed)
          std::reverse(ch.begin(), ch.end());
        for (int c : ch)
          node[size_t(x)]->insert_rightmost_child(node[size_t(c)]);
      }
    return node[size_t(t.root)];
  }
}

TEST(ExhaustiveMoOnTreeNodes, MoAnswersOnEverySubtreeAndPath)
{
  Check window("Gen_Mo_On_Tree_Node with a policy of the user");
  Check distinct("Distinct_Count_Mo_On_Tree_Node");
  Check powerful("Powerful_Array_Mo_On_Tree_Node");
  Check mode("Range_Mode_Mo_On_Tree_Node");
  Check inner("Gen_Mo_On_Tree_Node from a node that has a parent");
  Check null("Gen_Mo_On_Tree_Node, the null root");

  null.run();
  if (not throws<std::domain_error>([] { Gen_Mo_On_Tree_Node<int, Window_Values> mo(nullptr); }))
    null.fail(Spec(), "takes the null root");

  std::mt19937 rng(seed(7342));
  for_each_test_tree([&] (const Spec & s)
    {
      for (int r = 0; r < s.n; ++r)
        {
          const Rooted_Tree t(s, r);
          const auto val = random_values(rng, s.n);
          const std::string at = "rooted at " + std::to_string(r) + ": ";
          std::vector<int> numbers(size_t(s.n)), all(size_t(s.n));
          for (int x = 0; x < s.n; ++x)
            numbers[size_t(x)] = all[size_t(x)] = x;
          std::vector<Tree_Node<int> *> node;
          Tree_Node<int> * root = tree_nodes(t, node, r % 2 == 1);

          window.run();
          if (auto e = mo_error(Gen_Mo_On_Tree_Node<int, Window_Values>(root), node, t, all,
                                Window_Of{&numbers}); not e.empty())
            window.fail(s, at + e);

          // From a node with a parent, only its subtree; the rest is out
          inner.run();
          for (int c = 0; c < s.n; ++c)
            {
              if (c == r)
                continue;
              const auto part = t.subtree(c);
              const Gen_Mo_On_Tree_Node<int, Window_Values> mo(node[size_t(c)]);
              if (auto e = mo_error(mo, node, t, part, Window_Of{&numbers}); not e.empty())
                {
                  inner.fail(s, at + "from " + std::to_string(c) + ": " + e);
                  break;
                }
              if (not throws<std::domain_error>([&] { (void) mo.subtree_solve({node[size_t(r)]}); })
                  or not throws<std::domain_error>([&] { (void) mo.path_solve({{node[size_t(c)], node[size_t(r)]}}); }))
                {
                  inner.fail(s, at + "from " + std::to_string(c) + ": answers on a node out of it");
                  break;
                }
            }

          for (int x = 0; x < s.n; ++x)
            node[size_t(x)]->get_key() = val[size_t(x)];
          distinct.run();
          if (auto e = mo_error(Distinct_Count_Mo_On_Tree_Node<int>(root), node, t, all,
                                Distinct_Of{&val}); not e.empty())
            distinct.fail(s, at + e);
          powerful.run();
          if (auto e = mo_error(Powerful_Array_Mo_On_Tree_Node<int>(root), node, t, all,
                                Powerful_Of{&val}); not e.empty())
            powerful.fail(s, at + e);
          mode.run();
          if (auto e = mo_error(Range_Mode_Mo_On_Tree_Node<int>(root), node, t, all,
                                Mode_Of{&val}); not e.empty())
            mode.fail(s, at + e);

          destroy_tree(root);
        }
    }, 4);

  window.expect();
  distinct.expect();
  powerful.expect();
  mode.expect();
  inner.expect();
  null.expect();
}

// --------------------------------------------------------------------------
// All the engines
// --------------------------------------------------------------------------

TYPED_TEST(ExhaustiveTreeQueries, QueriesRejectWhatIsNotOfTheTree)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check lca("the LCA engines, the queries on what is not of the tree");
  Check hld("Gen_HLD, the queries on what is not of the tree");
  Check decomposition("the tree decompositions, the queries on what is not of the tree");
  Check dp("Gen_Tree_DP and Gen_Reroot_DP, the queries on what is not of the tree");
  Check empty("the engines of the empty graph");
  Built<GT> other(Spec{1, {}, {}});
  Node * const foreign = other.nodes[0];
  Node * const null = nullptr;

  for (int n = 1; n <= 4; ++n)
    for_each_tree(n, 1, [&] (const Spec & s)
      {
        Built<GT> b(s);
        const size_t out = size_t(s.n);   // the first id out of range
        for (int r = 0; r < s.n; ++r)
          {
            Node * const p = b.nodes[size_t(r)];
            const std::string at = "rooted at " + std::to_string(r);
            auto nv = [] (Node *) { return 1; };

            // A null node or one of another graph is a logic error, and an
            // id out of range is out of range
            auto rejects_nodes = [&] (const auto & e)
              {
                for (Node * q : {null, foreign})
                  if (not throws<std::logic_error>([&] { (void) e.id_of(q); })
                      or not throws<std::logic_error>([&] { (void) e.lca(q, p); })
                      or not throws<std::logic_error>([&] { (void) e.lca(p, q); })
                      or not throws<std::logic_error>([&] { (void) e.distance(q, p); })
                      or not throws<std::logic_error>([&] { (void) e.depth_of(q); })
                      or not throws<std::logic_error>([&] { (void) e.parent_of(q); }))
                    return false;
                return throws<std::out_of_range>([&] { (void) e.node_of(out); })
                  and throws<std::out_of_range>([&] { (void) e.lca_id(out, 0); })
                  and throws<std::out_of_range>([&] { (void) e.lca_id(0, out); })
                  and throws<std::out_of_range>([&] { (void) e.distance_id(0, out); })
                  and throws<std::out_of_range>([&] { (void) e.depth_of_id(out); })
                  and throws<std::out_of_range>([&] { (void) e.parent_id(out); });
              };

            lca.run();
            {
              const Binary_Lifting_LCA<GT> bl(b.g, p);
              const Euler_RMQ_LCA<GT> er(b.g, p);
              if (not rejects_nodes(bl) or not rejects_nodes(er)
                  or not throws<std::logic_error>([&] { (void) bl.is_ancestor(foreign, p); })
                  or not throws<std::logic_error>([&] { (void) er.is_ancestor(p, null); })
                  or not throws<std::logic_error>([&] { (void) bl.kth_ancestor(foreign, 0); })
                  or not throws<std::out_of_range>([&] { (void) bl.is_ancestor_id(out, 0); })
                  or not throws<std::out_of_range>([&] { (void) er.is_ancestor_id(0, out); })
                  or not throws<std::out_of_range>([&] { (void) bl.kth_ancestor_id(out, 0); }))
                lca.fail(s, at);
            }

            hld.run();
            {
              HLD_Sum<GT, int> h(b.g, p, nv);
              if (not rejects_nodes(h)
                  or not throws<std::logic_error>([&] { (void) h.path_query(foreign, p); })
                  or not throws<std::logic_error>([&] { (void) h.subtree_query(null); })
                  or not throws<std::logic_error>([&] { h.point_update(foreign, 1); })
                  or not throws<std::logic_error>([&] { (void) h.get_value(null); })
                  or not throws<std::out_of_range>([&] { (void) h.path_query_id(out, 0); })
                  or not throws<std::out_of_range>([&] { (void) h.path_query_edges_id(0, out); })
                  or not throws<std::out_of_range>([&] { (void) h.subtree_query_id(out); })
                  or not throws<std::out_of_range>([&] { h.point_update_id(out, 1); })
                  or not throws<std::out_of_range>([&] { (void) h.get_value_at_id(out); })
                  or not throws<std::out_of_range>([&] { (void) h.position(out); })
                  or not throws<std::out_of_range>([&] { (void) h.chain_head_id(out); })
                  or not throws<std::out_of_range>([&] { (void) h.subtree_size_of_id(out); }))
                hld.fail(s, at);
            }

            decomposition.run();
            {
              const Gen_Heavy_Light_Decomposition<GT> h(b.g, p);
              Gen_HLD_Path_Query<GT> q(b.g, p, 0);
              const Gen_Centroid_Decomposition<GT> cd(b.g, p);
              auto f = [] (size_t, size_t, bool) {};
              auto g = [] (size_t, size_t, size_t) {};
              if (not rejects_nodes(h)
                  or not throws<std::logic_error>([&] { (void) h.heavy_child(foreign); })
                  or not throws<std::logic_error>([&] { h.for_each_path_segment(p, null, f); })
                  or not throws<std::out_of_range>([&] { (void) h.heavy_child_id(out); })
                  or not throws<std::out_of_range>([&] { (void) h.head_id(out); })
                  or not throws<std::out_of_range>([&] { (void) h.position_of_id(out); })
                  or not throws<std::out_of_range>([&] { (void) h.subtree_range_id(out); })
                  or not throws<std::out_of_range>([&] { (void) h.is_ancestor_id(0, out); })
                  or not throws<std::out_of_range>([&] { h.for_each_path_segment_id(out, 0, f); })
                  or not throws<std::logic_error>([&] { (void) q.query_path(foreign, p); })
                  or not throws<std::logic_error>([&] { q.update_node(null, 1); })
                  or not throws<std::out_of_range>([&] { (void) q.query_path_id(0, out); })
                  or not throws<std::out_of_range>([&] { (void) q.query_subtree_id(out); })
                  or not throws<std::out_of_range>([&] { q.update_node_id(out, 1); })
                  or not throws<std::out_of_range>([&] { q.set_node_id(out, 1); })
                  or not throws<std::out_of_range>([&] { (void) q.get_node_id(out); })
                  or not throws<std::logic_error>([&] { (void) cd.id_of(foreign); })
                  or not throws<std::logic_error>([&] { (void) cd.centroid_parent(null); })
                  or not throws<std::logic_error>([&] { cd.for_each_centroid_ancestor(foreign, g); })
                  or not throws<std::out_of_range>([&] { (void) cd.node_of(out); })
                  or not throws<std::out_of_range>([&] { (void) cd.centroid_parent_id(out); })
                  or not throws<std::out_of_range>([&] { (void) cd.centroid_level_of_id(out); })
                  or not throws<std::out_of_range>([&] { (void) cd.centroid_path_length_of_id(out); })
                  or not throws<std::out_of_range>([&] { (void) cd.centroid_ancestors_of_id(out); })
                  or not throws<std::out_of_range>([&] { (void) cd.centroid_distances_of_id(out); })
                  or not throws<std::out_of_range>([&] { (void) cd.is_centroid_ancestor_id(out, 0); })
                  or not throws<std::out_of_range>([&] { (void) cd.distance_to_centroid_id(0, out); })
                  or not throws<std::out_of_range>([&] { cd.for_each_centroid_ancestor_id(out, g); }))
                decomposition.fail(s, at);
            }

            dp.run();
            {
              const std::vector<int> val(size_t(s.n), 1);
              const auto sums = sums_dp(b, p, val);
              const auto weighted = weighted_dp(b, p, val);
              if (not throws<std::logic_error>([&] { (void) sums.value(foreign); })
                  or not throws<std::logic_error>([&] { (void) sums.value(null); })
                  or not throws<std::logic_error>([&] { (void) sums.id_of(foreign); })
                  or not throws<std::logic_error>([&] { (void) weighted.value(foreign); })
                  or not throws<std::logic_error>([&] { (void) weighted.id_of(null); })
                  or not throws<std::out_of_range>([&] { (void) sums.node_of(out); })
                  or not throws<std::out_of_range>([&] { (void) weighted.node_of(out); }))
                dp.fail(s, at);
            }
          }
      });

  // The engines of the empty graph are empty, and answer no query
  empty.run();
  {
    Spec s;
    Built<GT> b(s);
    auto nv = [] (Node *) { return 1; };
    const Gen_Binary_Lifting_LCA<GT> bl(b.g);
    const Gen_Euler_RMQ_LCA<GT> er(b.g);
    const HLD_Sum<GT, int> h(b.g, nv);
    const Gen_Heavy_Light_Decomposition<GT> d(b.g);
    const Gen_HLD_Path_Query<GT> q(b.g, 0);
    const Gen_Centroid_Decomposition<GT> cd(b.g);
    const std::vector<int> val;
    const auto sums = sums_dp(b, null, val);
    const auto weighted = weighted_dp(b, null, val);
    if (not bl.is_empty() or bl.size() != 0 or bl.root() != nullptr or bl.num_levels() != 0
        or not er.is_empty() or er.euler_tour_size() != 0 or not h.is_empty() or h.num_chains() != 0
        or not d.is_empty() or not q.is_empty() or q.size() != 0 or not cd.is_empty()
        or cd.centroid_root() != nullptr or cd.centroid_root_id() != No_Id
        or cd.max_centroid_level() != 0 or sums.size() != 0 or sums.values().size() != 0
        or weighted.size() != 0 or weighted.values().size() != 0
        or tree_subtree_sizes(b.g, null).size() != 0 or tree_max_distance(b.g, null).size() != 0
        or tree_sum_of_distances(b.g, null).size() != 0)
      empty.fail(s, "an engine that is not empty");
    if (not throws<std::logic_error>([&] { (void) bl.lca_id(0, 0); })
        or not throws<std::logic_error>([&] { (void) er.lca_id(0, 0); })
        or not throws<std::logic_error>([&] { (void) bl.depth_of_id(0); })
        or not throws<std::logic_error>([&] { (void) h.path_query_id(0, 0); })
        or not throws<std::logic_error>([&] { (void) h.subtree_query_id(0); })
        or not throws<std::logic_error>([&] { (void) d.lca_id(0, 0); })
        or not throws<std::logic_error>([&] { (void) q.query_path_id(0, 0); })
        or not throws<std::logic_error>([&] { (void) cd.for_each_centroid_ancestor_id(0, [] (size_t, size_t, size_t) {}); })
        or not throws<std::logic_error>([&] { (void) sums.node_of(0); }))
      empty.fail(s, "a query on the empty graph that does not throw");

    // A root on the empty graph can only be of another graph
    if (not throws<std::domain_error>([&] { Gen_Binary_Lifting_LCA<GT> e(b.g, foreign); })
        or not throws<std::domain_error>([&] { Gen_Euler_RMQ_LCA<GT> e(b.g, foreign); })
        or not throws<std::domain_error>([&] { HLD_Sum<GT, int> e(b.g, foreign, nv); })
        or not throws<std::domain_error>([&] { Gen_Heavy_Light_Decomposition<GT> e(b.g, foreign); })
        or not throws<std::domain_error>([&] { Gen_HLD_Path_Query<GT> e(b.g, foreign, 0); })
        or not throws<std::domain_error>([&] { Gen_Centroid_Decomposition<GT> e(b.g, foreign); })
        or not throws<std::domain_error>([&] { (void) sums_dp(b, foreign, val); })
        or not throws<std::domain_error>([&] { (void) weighted_dp(b, foreign, val); }))
      empty.fail(s, "takes a root of another graph");
  }

  lca.expect();
  hld.expect();
  decomposition.expect();
  dp.expect();
  empty.expect();
}

namespace
{
  /** What is wrong with the copies of the engine that make(b, r) builds on
      the tree of b rooted at r, by answer(engine): a copy, a move of it,
      and, if the engine can be assigned, a copy and a move assigned over
      an engine of the tree of other; then the original. Empty if
      nothing. */
  template <class GT, class Make, class Answer>
  std::string copies_error(const Built<GT> & b, int r, const Built<GT> & other, Make make,
                           Answer answer)
  {
    const auto e = make(b, r);
    using E = std::remove_const_t<decltype(e)>;
    E copy(e);
    if (auto err = answer(copy); not err.empty())
      return "a copy: " + err;
    E moved(std::move(copy));
    if (auto err = answer(moved); not err.empty())
      return "a move: " + err;
    if constexpr (std::is_copy_assignable_v<E> and std::is_move_assignable_v<E>)
      {
        E assigned = make(other, 0);
        assigned = e;
        if (auto err = answer(assigned); not err.empty())
          return "a copy assigned: " + err;
        E move_assigned = make(other, 0);
        move_assigned = std::move(assigned);
        if (auto err = answer(move_assigned); not err.empty())
          return "a move assigned: " + err;
      }
    if (auto err = answer(e); not err.empty())
      return "the original after its copies: " + err;
    return "";
  }
}

TYPED_TEST(ExhaustiveTreeQueries, CopiesAndMovesAnswerAsTheOriginal)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check lca("the LCA engines, their copies and moves");
  Check hld("Gen_HLD, its copies and moves");
  Check decomposition("the tree decompositions, their copies and moves");
  Check dp("Gen_Tree_DP and Gen_Reroot_DP, their copies and moves");
  Check mo("Gen_Mo_On_Trees, its copies and moves");

  // Another tree, of other size and shape
  Spec star;
  star.n = 6;
  for (int x = 1; x < 6; ++x)
    star.arcs.push_back({0, x});
  Built<GT> other(star);

  for (int n = 1; n <= 5; ++n)
    for_each_tree(n, n <= 4 ? 1 : 5, [&] (const Spec & s)
      {
        Built<GT> b(s);
        // Values for the nodes of both trees
        std::vector<int> val(size_t(other.g.get_num_nodes())), all(size_t(s.n)), numbers(size_t(s.n));
        for (size_t x = 0; x < val.size(); ++x)
          val[x] = 2 - int(x % 5);
        for (int x = 0; x < s.n; ++x)
          all[size_t(x)] = numbers[size_t(x)] = x;
        auto nv = [&val] (Node * p) { return val[size_t(num(p))]; };
        for (int r : {0, s.n - 1})
          {
            const Rooted_Tree t(s, r);
            const std::string at = "rooted at " + std::to_string(r) + ": ";
            auto tree = [&] (const auto & e) { return tree_error(b, t, e); };

            lca.run();
            for (const auto & e : {copies_error(b, r, other, [] (const Built<GT> & g, int x)
                                                  { return Binary_Lifting_LCA<GT>(g.g, g.nodes[size_t(x)]); },
                                                  [&] (const auto & e) { return tree(e) + kth_error(b, t, e); }),
                                   copies_error(b, r, other, [] (const Built<GT> & g, int x)
                                                  { return Euler_RMQ_LCA<GT>(g.g, g.nodes[size_t(x)]); },
                                                  [&] (const auto & e) { return tree(e) + euler_tour_error(t, e); })})
              if (not e.empty())
                {
                  lca.fail(s, at + e);
                  break;
                }

            hld.run();
            if (auto e = copies_error(b, r, other, [&] (const Built<GT> & g, int x)
                                        { return HLD_Sum<GT, int>(g.g, g.nodes[size_t(x)], nv); },
                                        [&] (const auto & e)
                                        { return tree(e) + hld_layout_error(b, t, e)
                                            + fold_error(b, t, e, val, 0, Plus(), false); });
                not e.empty())
              hld.fail(s, at + e);

            decomposition.run();
            for (const auto & e : {copies_error(b, r, other, [] (const Built<GT> & g, int x)
                                                  { return Gen_Heavy_Light_Decomposition<GT>(g.g, g.nodes[size_t(x)]); },
                                                  [&] (const auto & e) { return tree(e) + decomposition_error(b, t, e); }),
                                   copies_error(b, r, other, [] (const Built<GT> & g, int x)
                                                  { return Gen_HLD_Path_Query<GT>(g.g, g.nodes[size_t(x)], 0); },
                                                  [&] (const auto & e) { return path_query_error(b, t, e, numbers, 0, Plus()); }),
                                   copies_error(b, r, other, [] (const Built<GT> & g, int x)
                                                  { return Gen_Centroid_Decomposition<GT>(g.g, g.nodes[size_t(x)]); },
                                                  [&] (const auto & e) { return tree(e) + centroid_error(b, t, e); })})
              if (not e.empty())
                {
                  decomposition.fail(s, at + e);
                  break;
                }

            dp.run();
            for (const auto & e : {copies_error(b, r, other, [&] (const Built<GT> & g, int x)
                                                  { return sums_dp(g, g.nodes[size_t(x)], val); },
                                                  [&] (const auto & e) { return subtree_sums_error(b, t, e, val); }),
                                   copies_error(b, r, other, [&] (const Built<GT> & g, int x)
                                                  { return weighted_dp(g, g.nodes[size_t(x)], val); },
                                                  [&] (const auto & e)
                                                  { return distances_error(b, t, e, tree_max_distance(b.g, b.nodes[size_t(r)]),
                                                                           tree_sum_of_distances(b.g, b.nodes[size_t(r)]), val); })})
              if (not e.empty())
                {
                  dp.fail(s, at + e);
                  break;
                }

            mo.run();
            if (auto e = copies_error(b, r, other, [] (const Built<GT> & g, int x)
                                        { return Gen_Mo_On_Trees<GT, Window_Values>(g.g, g.nodes[size_t(x)]); },
                                        [&] (const auto & e) { return mo_error(e, b.nodes, t, all, Window_Of{&numbers}); });
                not e.empty())
              mo.fail(s, at + e);
          }
      });

  lca.expect();
  hld.expect();
  decomposition.expect();
  dp.expect();
  mo.expect();
}

TYPED_TEST(ExhaustiveTreeQueries, LongPathsAndStarsRunOnASmallStack)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  const int path_size = heavy() ? 100000 : 10000;
  auto total = [] (const Array<size_t> & a)
    {
      long long r = 0;
      for (size_t i = 0; i < a.size(); ++i)
        r += (long long) a[i];
      return r;
    };
  auto greatest = [] (const Array<size_t> & a)
    {
      long long r = 0;
      for (size_t i = 0; i < a.size(); ++i)
        r = std::max(r, (long long) a[i]);
      return r;
    };

  // The path is deep, and the star, of a fifth of its nodes, broad
  for (const bool path : {true, false})
    {
      Spec s;
      s.n = path ? path_size : path_size / 5;
      const int n = s.n;
      for (int x = 1; x < n; ++x)
        s.arcs.push_back({path ? x - 1 : 0, x});
      Built<GT> b(s);
      Node * first = b.nodes[0];
      Node * last = b.nodes[size_t(n - 1)];
      const size_t far = path ? size_t(n - 1) : 1;
      std::string error;

      const bool ran = run_with_small_stack([&]
        {
          try
            {
              auto nv = [] (Node *) { return 1; };
              const Binary_Lifting_LCA<GT> bl(b.g, first);
              const Euler_RMQ_LCA<GT> er(b.g, first);
              const HLD_Sum<GT, int> h(b.g, first, nv);
              const Gen_Heavy_Light_Decomposition<GT> d(b.g, first);
              const Gen_HLD_Path_Query<GT> q(b.g, first, nv, 0);
              const Gen_Centroid_Decomposition<GT> cd(b.g, first);
              const auto sizes = tree_subtree_sizes(b.g, first);
              const auto sums = tree_sum_of_distances(b.g, first);
              const Distinct_Count_Mo_On_Trees<GT> mo(b.g, first);
              const auto mo_sub = mo.subtree_solve({first, last});
              const auto mo_path = mo.path_solve({{first, last}, {last, last}});
              if (bl.depth_of(last) != far or bl.lca(last, first) != first
                  or bl.distance(first, last) != far or bl.kth_ancestor(last, far) != first)
                error = "Gen_Binary_Lifting_LCA";
              else if (er.depth_of(last) != far or er.lca(first, last) != first
                       or er.euler_tour_size() != size_t(2 * n - 1))
                error = "Gen_Euler_RMQ_LCA";
              else if (h.path_query(first, last) != int(far) + 1 or h.subtree_query(first) != n
                       or h.distance(last, first) != far)
                error = "Gen_HLD";
              else if (d.lca(last, first) != first or d.distance(first, last) != far
                       or q.query_path(last, first) != int(far) + 1 or q.query_subtree(first) != n)
                error = "Gen_Heavy_Light_Decomposition and Gen_HLD_Path_Query";
              else if (int(cd.max_centroid_level()) > floor_log2(size_t(n)))
                error = "Gen_Centroid_Decomposition";
              else if (sizes.size() != size_t(n) or sums.size() != size_t(n)
                       or total(sizes) != (path ? n * (n + 1LL) / 2 : 2LL * n - 1)
                       or greatest(sums) != (path ? n * (n - 1LL) / 2 : 2LL * n - 3))
                error = "tree_subtree_sizes() and tree_sum_of_distances()";
              else if (mo_sub[0] != size_t(n) or mo_sub[1] != 1 or mo_path[0] != far + 1
                       or mo_path[1] != 1)
                error = "Distinct_Count_Mo_On_Trees";
            }
          catch (const std::exception & ex)
            {
              error = std::string("throws ") + ex.what();
            }
        });

      EXPECT_TRUE(ran) << "the thread with a small stack could not run";
      EXPECT_EQ(error, "") << (path ? "a path" : "a star") << " of " << n << " nodes";
    }
}
