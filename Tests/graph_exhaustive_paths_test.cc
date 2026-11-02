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
 * @file graph_exhaustive_paths_test.cc
 * @brief Shortest paths, with weights that admit no negative cycle, on
 *        every small multigraph, against the oracles.
 *
 * Each test runs one family of algorithms over every multigraph of a
 * domain of graph_harness.H, with loops, parallel and antiparallel arcs
 * and the arcs in two orders, on every kind of graph that the family
 * takes, and compares each answer with an oracle that does not use Aleph:
 * Floyd on long long for the distances, and the enumeration of the paths,
 * or of the walks, in cost order for the k shortest paths. Every path that
 * an algorithm gives is walked: it must go from its start to its end along
 * arcs of the graph, in their direction in a digraph, that the filter
 * shows, and cost both the distance that came with it and the least one.
 *
 * The larger domains (up to 4 nodes and 3 arcs; one more of each with
 * ALEPH_GRAPH_HARNESS=heavy) take a few fixed weightings; the smaller ones
 * every weighting from a few values; and random graphs of 6 to 8 nodes
 * come from the seed of graph_harness.H. Other suites hide every set of
 * arcs, run after another algorithm painted the graph, take distances of
 * type int and double (the halves of the weights, so that a distance kept
 * in an integer loses its fraction) and weights near the limits of int.
 */

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Dijkstra.H>
#include <AStar.H>
#include <Zero_One_BFS.H>
#include <Dial.H>
#include <Floyd_Warshall.H>
#include <Johnson.H>
#include <K_Shortest_Paths.H>

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
  using Matrix = std::vector<std::vector<long long>>;

  constexpr long long Inf = oracle::Inf;

  constexpr long long Int_Max = std::numeric_limits<int>::max();

  /// The weight x as a distance of type T: for a floating type, its half,
  /// so that a distance kept in an integer loses its fraction, or x itself
  /// if whole
  template <typename T>
  T as(long long x, bool whole = false)
  {
    if constexpr (std::is_floating_point_v<T>)
      return whole ? T(x) : T(x) / 2;
    else
      return T(x);
  }

  /// The weights of s as distances of type T
  template <typename T>
  std::vector<T> weights_as(const Spec & s, bool whole = false)
  {
    std::vector<T> r;
    for (long long x : s.w)
      r.push_back(as<T>(x, whole));
    return r;
  }

  template <typename T>
  std::string text_of(T x)
  {
    std::ostringstream o;
    o << x;
    return o.str();
  }

  std::string route(int u, int v)
  {
    return "from " + std::to_string(u) + " to " + std::to_string(v) + ": ";
  }

  /// What f throws: empty if nothing, else the class of the exception and
  /// its text
  template <class F>
  std::string what_throws(F && f)
  {
    try
      {
        f();
      }
    catch (const std::overflow_error & e)
      {
        return std::string("overflow_error ") + e.what();
      }
    catch (const std::domain_error & e)
      {
        return std::string("domain_error ") + e.what();
      }
    catch (const std::invalid_argument & e)
      {
        return std::string("invalid_argument ") + e.what();
      }
    catch (const std::exception & e)
      {
        return std::string("exception ") + e.what();
      }
    return "";
  }

  /// Whether what, as what_throws() gives it, is an exception of a class
  bool is(const std::string & what, const char * kind)
  {
    return what.rfind(kind, 0) == 0;
  }

  /// The shortest distances on the arcs of s that hidden shows, Inf where
  /// there is no path; neg tells whether they have a negative cycle
  Matrix distances(const Spec & s, bool directed, unsigned long hidden, bool & neg)
  {
    std::vector<int> orig;
    return oracle::shortest_distances(oracle::without(s, hidden, orig), directed, neg);
  }

  Matrix distances(const Spec & s, bool directed, unsigned long hidden = 0)
  {
    bool neg = false;
    return distances(s, directed, hidden, neg);
  }

  /// The potentials of Johnson: the distance to each node from a new one
  /// with an arc of weight 0 to every node, which is the least distance to
  /// it from any node, and 0 from itself
  std::vector<long long> potentials(const Matrix & d)
  {
    std::vector<long long> h(d.size(), 0);
    for (size_t x = 0; x < d.size(); ++x)
      for (size_t v = 0; v < d.size(); ++v)
        if (d[x][v] != Inf)
          h[v] = std::min(h[v], d[x][v]);
    return h;
  }

  /// The costs of the simple paths of s from u to v, in increasing order;
  /// a path of a graph that is not directed takes each arc either way
  std::vector<long long> simple_path_costs(const Spec & s, int u, int v, bool directed)
  {
    std::vector<long long> costs;
    std::vector<bool> on(size_t(s.n), false);
    auto walk = [&] (auto & self, int x, long long cost) -> void
      {
        if (x == v)
          {
            costs.push_back(cost);
            return;
          }
        on[size_t(x)] = true;
        for (size_t i = 0; i < s.arcs.size(); ++i)
          for (size_t e = 0; e < (directed ? 1U : 2U); ++e)
            if (s.arcs[i][e] == x and not on[size_t(s.arcs[i][1 - e])])
              self(self, s.arcs[i][1 - e], cost + s.w[i]);
        on[size_t(x)] = false;
      };
    walk(walk, u, 0);
    std::sort(costs.begin(), costs.end());
    return costs;
  }

  /// The costs of the k cheapest walks of s from u to v, in order: a walk
  /// is a sequence of arcs, each from the node where the one before ended
  /// (a loop counts once). The search takes each node at most k times,
  /// since the k cheapest walks to v go through the k cheapest ones to
  /// each of their nodes.
  std::vector<long long> walk_costs(const Spec & s, int u, int v, bool directed, size_t k)
  {
    std::vector<long long> costs;
    std::vector<size_t> taken(size_t(s.n), 0);
    using Item = std::pair<long long, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<>> queue;
    queue.push({0, u});
    while (not queue.empty() and costs.size() < k)
      {
        const auto [cost, x] = queue.top();
        queue.pop();
        if (taken[size_t(x)]++ >= k)
          continue;
        if (x == v)
          costs.push_back(cost);
        for (size_t i = 0; i < s.arcs.size(); ++i)
          if (s.arcs[i][0] == x)
            queue.push({cost + s.w[i], s.arcs[i][1]});
          else if (not directed and s.arcs[i][1] == x)
            queue.push({cost + s.w[i], s.arcs[i][0]});
      }
    return costs;
  }

  /// Whether a search from u meets an arc of s whose weight is not 0 or
  /// 1: one out of a node that u reaches
  bool meets_other_weight(const Spec & s, int u, bool directed)
  {
    std::vector<bool> seen(size_t(s.n), false);
    seen[size_t(u)] = true;
    for (bool grew = true; grew; )
      {
        grew = false;
        for (const auto & a : s.arcs)
          for (size_t e = 0; e < (directed ? 1U : 2U); ++e)
            if (seen[size_t(a[e])] and not seen[size_t(a[1 - e])])
              seen[size_t(a[1 - e])] = grew = true;
      }
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (s.w[i] != 0 and s.w[i] != 1
          and (seen[size_t(s.arcs[i][0])] or (not directed and seen[size_t(s.arcs[i][1])])))
        return true;
    return false;
  }

  /// Whether a search from u, which settles the nodes in the order of
  /// their distances up to the distance bound, may relax an arc whose
  /// weight, added to the distance of its source, goes beyond max: an arc
  /// out of a node so settled to another node whose distance is not less,
  /// which the search may not have settled yet
  bool sum_beyond(const Spec & s, const Matrix & d, int u, long long bound, bool directed,
                  long long max)
  {
    for (size_t i = 0; i < s.arcs.size(); ++i)
      for (size_t e = 0; e < (directed ? 1U : 2U); ++e)
        {
          const int x = s.arcs[i][e], y = s.arcs[i][1 - e];
          const long long dx = d[size_t(u)][size_t(x)], dy = d[size_t(u)][size_t(y)];
          if (x != y and dx != Inf and dx <= bound and dy >= dx and dx + s.w[i] > max)
            return true;
        }
    return false;
  }

  /// Whether two arcs of s join the same two nodes, which differ, in the
  /// same direction if directed
  bool has_parallel_arcs(const Spec & s, bool directed)
  {
    std::set<std::pair<int, int>> seen;
    for (const auto & a : s.arcs)
      if (a[0] != a[1]
          and not seen.insert(directed ? std::pair{a[0], a[1]}
                              : std::pair{std::min(a[0], a[1]), std::max(a[0], a[1])}).second)
        return true;
    return false;
  }

  /// s with the arcs that hidden hides weighing poison, which an algorithm
  /// that respects the filter never takes
  Spec poisoned(const Spec & s, unsigned long hidden, long long poison)
  {
    Spec r = s;
    for (size_t i = 0; i < r.w.size(); ++i)
      if (not graph_filters::shown(hidden, int(i)))
        r.w[i] = poison;
    return r;
  }

  /// The numbers of the nodes and of the arcs of a path, in order
  struct Walk
  {
    std::vector<int> nodes;
    std::vector<int> arcs;

    bool operator == (const Walk &) const = default;

    [[nodiscard]] std::string str() const
    {
      std::string r = "[";
      for (size_t i = 0; i < nodes.size(); ++i)
        {
          r += std::to_string(nodes[i]);
          if (i < arcs.size())
            r += " -" + std::to_string(arcs[i]) + "- ";
        }
      return r + "]";
    }
  };

  template <class GT>
  Walk walk_of(const Path<GT> & p)
  {
    Walk w;
    for (typename Path<GT>::Iterator it(p); it.has_current_node(); it.next_ne())
      {
        w.nodes.push_back(num(it.get_current_node_ne()));
        if (it.has_current_arc())
          w.arcs.push_back(num(it.get_current_arc_ne()));
      }
    return w;
  }

  /// What is wrong with w as a walk of s from u to v: each of its arcs
  /// must be one of s that hidden shows, and join the node before it to
  /// the one after it, in that direction if directed; empty if nothing
  std::string walk_error(const Spec & s, const Walk & w, int u, int v, bool directed,
                         unsigned long hidden)
  {
    if (w.nodes.empty())
      return "an empty path";
    if (w.arcs.size() + 1 != w.nodes.size())
      return "the path " + w.str() + " lacks arcs";
    if (w.nodes.front() != u or w.nodes.back() != v)
      return "the path " + w.str() + " does not go from " + std::to_string(u)
        + " to " + std::to_string(v);
    for (size_t i = 0; i < w.arcs.size(); ++i)
      {
        const int a = w.arcs[i];
        if (a < 0 or size_t(a) >= s.arcs.size())
          return "the path " + w.str() + " has an arc that is not of the graph";
        const int x = s.arcs[size_t(a)][0], y = s.arcs[size_t(a)][1];
        const int p = w.nodes[i], q = w.nodes[i + 1];
        if (not (x == p and y == q) and (directed or not (x == q and y == p)))
          return "the path " + w.str() + " does not follow the arc " + std::to_string(a);
        if (not graph_filters::shown(hidden, a))
          return "the path " + w.str() + " takes the hidden arc " + std::to_string(a);
      }
    return "";
  }

  /// The weight of the arcs of a valid walk
  long long cost_of(const Spec & s, const Walk & w)
  {
    long long c = 0;
    for (int a : w.arcs)
      c += s.w[size_t(a)];
    return c;
  }

  bool is_simple(const Walk & w)
  {
    return std::set<int>(w.nodes.begin(), w.nodes.end()).size() == w.nodes.size();
  }

  /// What is wrong with path as a path of g, made from s, from u to v that
  /// costs want, with got the cost that came with it; empty if nothing
  template <class GT, typename T>
  std::string path_error(const GT & g, const Spec & s, const Path<GT> & path, int u, int v,
                         unsigned long hidden, long long want, T got, bool whole = false)
  {
    if (not path.inside_graph(g))
      return "a path of another graph";
    const Walk w = walk_of(path);
    if (auto e = walk_error(s, w, u, v, g.is_digraph(), hidden); not e.empty())
      return e;
    if (const long long c = cost_of(s, w); c != want)
      return "the path " + w.str() + " costs " + std::to_string(c) + ", not "
        + std::to_string(want);
    if (got != as<T>(want, whole))
      return "the path " + w.str() + " comes with the cost " + text_of(got) + ", not "
        + text_of(as<T>(want, whole));
    return "";
  }

  /// Whether a query for a path to a node that is not reached says so: by
  /// a domain_error, or by max() and an empty path
  template <class GT, typename T>
  bool no_path(const std::string & thrown, T got, const Path<GT> & path)
  {
    return is(thrown, "domain_error")
      or (thrown.empty() and got == std::numeric_limits<T>::max() and path.is_empty());
  }

  /** What is wrong with tree as a tree of shortest paths from u, in a
      graph made from s with the infos of graph_harness.H, on the arcs that
      hidden shows: its nodes are distinct nodes of s, its arcs are arcs of
      s that join in the tree the nodes that they join in s (from the
      parent to the child in a digraph), and the path in the tree from u to
      each node that it reaches costs the distance in row. Gives in dist the
      distance in the tree of each node of s, Inf for those that the tree
      lacks or does not reach, and in unreached how many of its nodes the
      root does not reach; empty if nothing is wrong. */
  template <class GT>
  std::string tree_error(GT & tree, const Spec & s, int u, const std::vector<long long> & row,
                         bool directed, unsigned long hidden, std::vector<long long> & dist,
                         size_t & unreached)
  {
    dist.assign(size_t(s.n), Inf);
    unreached = 0;
    std::vector<bool> in(size_t(s.n), false);
    for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
      {
        const int x = num(it.get_curr());
        if (x < 0 or x >= s.n or in[size_t(x)])
          return "the tree has a node that is not of the graph, or twice";
        in[size_t(x)] = true;
      }
    if (u < 0 or u >= s.n or not in[size_t(u)])
      return "the tree lacks its root";

    // The arcs out of each node of the tree, as (arc, the other end)
    std::vector<std::vector<std::pair<int, int>>> out(size_t(s.n));
    for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr();
        const int i = num(a);
        const int x = num(tree.get_src_node(a)), y = num(tree.get_tgt_node(a));
        if (i < 0 or size_t(i) >= s.arcs.size() or not graph_filters::shown(hidden, i))
          return "the tree has an arc that is not of the graph, or a hidden one";
        const auto & e = s.arcs[size_t(i)];
        if (not (e[0] == x and e[1] == y) and (directed or not (e[0] == y and e[1] == x)))
          return "the arc " + std::to_string(i) + " joins other nodes in the tree";
        out[size_t(x)].push_back({i, y});
        if (not directed)
          out[size_t(y)].push_back({i, x});
      }

    dist[size_t(u)] = 0;
    std::vector<int> queue = {u};
    std::vector<bool> walked(s.arcs.size(), false);
    for (size_t h = 0; h < queue.size(); ++h)
      for (const auto & [i, y] : out[size_t(queue[h])])
        {
          if (walked[size_t(i)])
            continue;
          walked[size_t(i)] = true;
          if (dist[size_t(y)] != Inf)
            return "the tree has a cycle through the arc " + std::to_string(i);
          dist[size_t(y)] = dist[size_t(queue[h])] + s.w[size_t(i)];
          queue.push_back(y);
        }
    unreached = tree.get_num_nodes() - queue.size();
    if (tree.get_num_arcs() + 1 != queue.size())
      return "the tree has arcs that its root does not reach";
    for (int x = 0; x < s.n; ++x)
      if (dist[size_t(x)] != Inf and dist[size_t(x)] != row[size_t(x)])
        return "in the tree " + std::to_string(x) + " is at " + std::to_string(dist[size_t(x)])
          + ", not " + std::to_string(row[size_t(x)]);
    return "";
  }

  /// What is wrong with the mapping between tree and the graph of b: each
  /// node and each arc of the tree is mapped through the cookies to the
  /// one of the graph with its number, and back; empty if nothing
  template <class GT>
  std::string mapping_error(GT & tree, const Built<GT> & b)
  {
    for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
      {
        auto * t = it.get_curr();
        auto * p = mapped_node<GT>(t);
        if (p == nullptr or p != b.nodes[size_t(num(t))] or mapped_node<GT>(p) != t)
          return "the node " + std::to_string(num(t)) + " of the tree is not mapped both ways";
      }
    for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * t = it.get_curr();
        auto * a = mapped_arc<GT>(t);
        if (a == nullptr or a != b.arcs[size_t(num(t))] or mapped_arc<GT>(a) != t)
          return "the arc " + std::to_string(num(t)) + " of the tree is not mapped both ways";
      }
    return "";
  }

  /// What is wrong with a tree that tree_error() checked, as one that
  /// reaches every node at a distance in row; empty if nothing
  std::string coverage_error(const std::vector<long long> & row,
                             const std::vector<long long> & dist)
  {
    for (size_t x = 0; x < row.size(); ++x)
      if (row[x] != Inf and dist[x] == Inf)
        return "the tree lacks the node " + std::to_string(x);
    return "";
  }

  /// Whether the painting of b has a hidden arc, as an unfiltered painting
  /// before leaves it
  template <class GT>
  bool painted_hidden_arc(const Built<GT> & b, unsigned long hidden)
  {
    for (size_t i = 0; i < b.arcs.size(); ++i)
      if (not graph_filters::shown(hidden, int(i))
          and IS_ARC_VISITED(b.arcs[i], Aleph::Spanning_Tree))
        return true;
    return false;
  }

  /// The weightings of the larger domains: every arc 1, which ties many
  /// paths, and the powers of 2, which give each path its own cost, rising
  /// or falling with the order of insertion, so that the first of two
  /// parallel arcs is the lighter or the heavier one
  enum class Weighting { Ones, Rising, Falling };

  Spec weighted(const Spec & shape, Weighting k)
  {
    Spec s = shape;
    const size_t m = shape.arcs.size();
    for (size_t i = 0; i < m; ++i)
      s.w.push_back(k == Weighting::Ones ? 1 : 1LL << (k == Weighting::Rising ? i : m - 1 - i));
    return s;
  }

  constexpr Weighting All_Weightings[] = {Weighting::Ones, Weighting::Rising, Weighting::Falling};

  /// shape with the weight of the arc i given by f(i)
  template <class F>
  Spec weighted(const Spec & shape, F && f)
  {
    Spec s = shape;
    for (size_t i = 0; i < shape.arcs.size(); ++i)
      s.w.push_back(f(i));
    return s;
  }

  /// Calls f with shape weighted in every way that values allow
  template <class F>
  void for_each_weighting(const Spec & shape, const std::vector<long long> & values, F && f)
  {
    Spec s = shape;
    std::vector<size_t> pick(shape.arcs.size(), 0);
    while (true)
      {
        s.w.clear();
        for (size_t i : pick)
          s.w.push_back(values[i]);
        f(static_cast<const Spec &>(s));
        size_t i = 0;
        while (i < pick.size() and ++pick[i] == values.size())
          pick[i++] = 0;
        if (i == pick.size())
          return;
      }
  }

  /// Random specs of 6 to 8 nodes and n to 2n arcs, weighted from lo to
  /// hi, from the seed of the harness; ten times as many in heavy mode
  std::vector<Spec> random_specs(unsigned first_seed, bool directed, size_t count,
                                 long long lo, long long hi)
  {
    std::mt19937 rng(seed(first_seed));
    std::vector<Spec> r;
    for (size_t i = 0, n = heavy() ? 10 * count : count; i < n; ++i)
      {
        const int nodes = 6 + int(rng() % 3);
        const int arcs = nodes + int(rng() % unsigned(nodes + 1));
        Spec s = random_spec(rng, nodes, arcs, directed);
        random_weights(rng, s, lo, hi);
        r.push_back(s);
      }
    return r;
  }

  /// Random digraphs as those of random_specs(), weighted b + p(v) - p(u)
  /// for an arc from u to v, with b from 0 to 4 and a potential p from -4
  /// to 4: some arcs are negative, but no cycle is
  std::vector<Spec> random_potential_specs(unsigned first_seed, size_t count)
  {
    std::mt19937 rng(seed(first_seed));
    std::vector<Spec> r = random_specs(first_seed, true, count, 0, 4);
    for (Spec & s : r)
      {
        std::vector<long long> p(size_t(s.n));
        for (auto & x : p)
          x = (long long) (rng() % 9) - 4;
        for (size_t i = 0; i < s.arcs.size(); ++i)
          s.w[i] += p[size_t(s.arcs[i][1])] - p[size_t(s.arcs[i][0])];
      }
    return r;
  }

  template <class GT, typename T>
  using Weights = Weight_Of<GT, T>;

  template <class GT, typename T, class SA = Dft_Show_Arc<GT>,
            template <class, class, class> class Heap = ArcHeap>
  using Dijkstra_Of = Dijkstra_Min_Paths<GT, Weights<GT, T>, Node_Arc_Iterator, SA, Heap>;

  template <class GT, typename T>
  using No_Heuristic = Zero_Heuristic<GT, Weights<GT, T>>;

  template <class GT, typename T, class SA = Dft_Show_Arc<GT>,
            template <class, class, class> class Heap = ArcHeap>
  using AStar_Of = AStar_Min_Path<GT, Weights<GT, T>, No_Heuristic<GT, T>,
                                  Node_Arc_Iterator, SA, Heap>;

  /// The nodes from which a check runs: every node of s, or the node
  /// only, unless it is negative
  std::vector<int> sources(const Spec & s, int only)
  {
    if (only >= 0)
      return {only};
    std::vector<int> r;
    for (int u = 0; u < s.n; ++u)
      r.push_back(u);
    return r;
  }

  /// The checks of the painting of the shortest paths from each node, and
  /// of the queries that it answers
  struct Painting_Checks
  {
    Check state;
    Check distance;
    Check path;

    Painting_Checks(const std::string & who, const std::string & how = "",
                    bool state_known = false, bool path_known = false)
      : state(who + "::paint_min_paths_tree()" + how, state_known),
        distance(who + "::get_distance()" + how),
        path(who + "::get_min_path(end, path)" + how, path_known)
    {}

    void expect() const
    {
      state.expect();
      distance.expect();
      path.expect();
    }
  };

  /// Paints with solver the shortest paths from each node of b, or from
  /// only, and asks it the distance and the path to every node
  template <typename T, class Solver, class GT>
  void check_painting(Painting_Checks & c, Solver & solver, Built<GT> & b, const Spec & s,
                      const Matrix & d, unsigned long hidden, bool whole = false,
                      int only = -1)
  {
    for (int u : sources(s, only))
      {
        auto * src = b.nodes[size_t(u)];
        const std::string from = "from " + std::to_string(u) + ": ";
        c.state.run();
        if (auto e = what_throws([&] { solver.paint_min_paths_tree(b.g, src); }); not e.empty())
          {
            c.state.fail(s, from + e);
            continue;
          }
        bool computed = true;
        if constexpr (requires { solver.has_computation(); })
          computed = solver.has_computation();
        if (not solver.is_painted() or solver.get_start_node() != src
            or solver.get_graph() != &b.g or not computed)
          c.state.fail(s, from + "the solver does not tell its painting");

        for (int v = 0; v < s.n; ++v)
          {
            auto * tgt = b.nodes[size_t(v)];
            const long long want = d[size_t(u)][size_t(v)];
            const std::string where = route(u, v);

            c.distance.run();
            T got{};
            auto e = what_throws([&] { got = solver.get_distance(tgt); });
            if (want == Inf)
              {
                if (not is(e, "domain_error"))
                  c.distance.fail(s, where + "gives " + (e.empty() ? text_of(got) : e)
                                  + " for a node not reached");
              }
            else if (not e.empty())
              c.distance.fail(s, where + e);
            else if (got != as<T>(want, whole))
              c.distance.fail(s, where + "gives " + text_of(got) + ", not "
                              + text_of(as<T>(want, whole)));

            c.path.run();
            Path<GT> path(b.g);
            e = what_throws([&] { got = solver.get_min_path(tgt, path); });
            if (want == Inf)
              {
                if (not no_path(e, got, path))
                  c.path.fail(s, where + "gives the path " + walk_of(path).str()
                              + " to a node not reached");
              }
            else if (not e.empty())
              c.path.fail(s, where + e);
            else if (auto pe = path_error(b.g, s, path, u, v, hidden, want, got, whole);
                     not pe.empty())
              c.path.fail(s, where + pe);
          }
      }
  }

  /// Paints with solver the shortest paths from each node of b, or from
  /// only, and copies the painted tree, which must be one of shortest
  /// paths, mapped to the graph, with the weight that the copy gives; from
  /// a node that does not reach every other one, nodes, if given, checks
  /// that the copy has only the nodes that the painting reaches
  template <typename T, class Solver, class GT>
  void check_copies(Check & copy, Check * nodes, Solver & solver, Built<GT> & b,
                    const Spec & s, const Matrix & d, unsigned long hidden, int only = -1)
  {
    for (int u : sources(s, only))
      {
        const std::string from = "from " + std::to_string(u) + ": ";
        copy.run();
        GT tree;
        T total{};
        if (auto e = what_throws([&]
              {
                solver.paint_min_paths_tree(b.g, b.nodes[size_t(u)]);
                total = solver.copy_painted_min_paths_tree(b.g, tree);
              }); not e.empty())
          {
            copy.fail(s, from + e);
            continue;
          }
        std::vector<long long> dist;
        size_t unreached = 0;
        const auto & row = d[size_t(u)];
        std::string e = tree_error(tree, s, u, row, b.g.is_digraph(), hidden, dist, unreached);
        if (e.empty())
          e = coverage_error(row, dist);
        if (e.empty())
          e = mapping_error(tree, b);
        if (e.empty())
          {
            T weight{};
            for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
              weight += as<T>(s.w[size_t(num(it.get_curr()))]);
            if (weight != total)
              e = "gives the weight " + text_of(total) + " for a tree of " + text_of(weight);
          }
        if (e.empty() and solver.is_painted())
          e = "the solver is still painted";
        if (not e.empty())
          copy.fail(s, from + e);

        if (nodes == nullptr or std::count(row.begin(), row.end(), Inf) == 0)
          continue;
        nodes->run();
        if (unreached > 0)
          nodes->fail(s, from + "the tree has " + std::to_string(unreached)
                      + " nodes that the painting does not reach");
      }
  }

  /// Runs find(src, tgt, path), which gives the cost of a shortest path
  /// that it puts in path or, without one, max() and an empty path, from
  /// every node (or from only) to every other one with the check pairs,
  /// and, if self is given, from a node to itself with self, which wants
  /// the path of no arc
  template <typename T, class GT, class Find>
  void check_find(Check & pairs, Check * self, Built<GT> & b, const Spec & s, const Matrix & d,
                  unsigned long hidden, Find && find, bool whole = false, int only = -1)
  {
    for (int u : sources(s, only))
      for (int v = 0; v < s.n; ++v)
        {
          if (u == v and self == nullptr)
            continue;
          Check & c = u == v ? *self : pairs;
          c.run();
          const std::string where = route(u, v);
          const long long want = d[size_t(u)][size_t(v)];
          Path<GT> path(b.g);
          T got{};
          if (auto e = what_throws([&]
                { got = find(b.nodes[size_t(u)], b.nodes[size_t(v)], path); });
              not e.empty())
            c.fail(s, where + e);
          else if (want == Inf)
            {
              if (got != std::numeric_limits<T>::max() or not path.is_empty())
                c.fail(s, where + "gives " + text_of(got) + " and the path "
                       + walk_of(path).str() + " to a node not reached");
            }
          else if (auto pe = path_error(b.g, s, path, u, v, hidden, want, got, whole);
                   not pe.empty())
            c.fail(s, where + pe);
        }
  }

  /// Runs paint(src, tgt), which paints the shortest paths from src until
  /// it reaches tgt and tells whether it did, from every node to every
  /// other one with the check pairs, and to itself with self; then asks
  /// solver the distance and the path to tgt
  template <typename T, class Solver, class GT, class Paint>
  void check_partial_painting(Check & pairs, Check & self, Solver & solver, Built<GT> & b,
                              const Spec & s, const Matrix & d, unsigned long hidden,
                              Paint && paint)
  {
    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          Check & c = u == v ? self : pairs;
          c.run();
          auto * tgt = b.nodes[size_t(v)];
          const std::string where = route(u, v);
          const long long want = d[size_t(u)][size_t(v)];
          bool found = false;
          if (auto e = what_throws([&] { found = paint(b.nodes[size_t(u)], tgt); });
              not e.empty())
            {
              c.fail(s, where + e);
              continue;
            }
          if (found != (want != Inf))
            {
              c.fail(s, where + (found ? "reaches a node not reached" : "does not reach the end"));
              continue;
            }
          if (not found)
            continue;
          T dist{}, cost{};
          Path<GT> path(b.g);
          if (auto e = what_throws([&]
                {
                  dist = solver.get_distance(tgt);
                  cost = solver.get_min_path(tgt, path);
                }); not e.empty())
            c.fail(s, where + "then " + e);
          else if (dist != as<T>(want))
            c.fail(s, where + "then get_distance() gives " + text_of(dist) + ", not "
                   + text_of(as<T>(want)));
          else if (auto pe = path_error(b.g, s, path, u, v, hidden, want, cost); not pe.empty())
            c.fail(s, where + "then get_min_path(): " + pe);
        }
  }

  /// The checks of the trees of shortest paths, and of the paths that
  /// get_min_path(tree, end, path) takes from them
  struct Tree_Checks
  {
    Check tree;
    Check paths;
    Check parallel;
    Check out;

    explicit Tree_Checks(const std::string & who)
      : tree(who),
        paths("get_min_path(tree, end, path), after " + who),
        parallel("get_min_path(tree, end, path), after " + who + ", with parallel arcs"),
        out("get_min_path(tree, end, path), after " + who + ", to a node out of the tree")
    {}

    void expect() const
    {
      tree.expect();
      paths.expect();
      parallel.expect();
      out.expect();
    }
  };

  /// Asks solver.get_min_path(tree, end, path) the path from u to v in
  /// the tree that it built from u, with the check pairs, or parallel if
  /// s has parallel arcs, or out if v is not reached
  template <typename T, class Solver, class GT>
  void check_tree_path(Tree_Checks & c, Solver & solver, GT & tree, Built<GT> & b,
                       const Spec & s, const Matrix & d, int u, int v)
  {
    const long long want = d[size_t(u)][size_t(v)];
    Check & pc = want == Inf ? c.out : has_parallel_arcs(s, b.g.is_digraph()) ? c.parallel
      : c.paths;
    pc.run();
    Path<GT> path(b.g);
    T got{};
    const auto e = what_throws([&]
      { got = solver.get_min_path(tree, b.nodes[size_t(v)], path); });
    if (want == Inf)
      {
        if (not no_path(e, got, path))
          pc.fail(s, route(u, v) + "gives " + text_of(got) + " and the path "
                  + walk_of(path).str());
      }
    else if (not e.empty())
      pc.fail(s, route(u, v) + e);
    else if (auto pe = path_error(b.g, s, path, u, v, 0, want, got); not pe.empty())
      pc.fail(s, route(u, v) + pe);
  }

  /// Runs compute(src, tree), which builds in tree the tree of the
  /// shortest paths from src and gives its root, from every node, and
  /// checks the tree, its mapping to the graph and the
  /// paths that solver.get_min_path(tree, end, path) gives to every node
  template <typename T, class Solver, class GT, class Compute>
  void check_trees(Tree_Checks & c, Solver & solver, Built<GT> & b, const Spec & s,
                   const Matrix & d, unsigned long hidden, Compute && compute)
  {
    for (int u = 0; u < s.n; ++u)
      {
        const std::string from = "from " + std::to_string(u) + ": ";
        const auto & row = d[size_t(u)];
        c.tree.run();
        GT tree;
        typename GT::Node * root = nullptr;
        if (auto e = what_throws([&] { root = compute(b.nodes[size_t(u)], tree); });
            not e.empty())
          {
            c.tree.fail(s, from + e);
            continue;
          }
        std::vector<long long> dist;
        size_t unreached = 0;
        std::string e = tree_error(tree, s, u, row, b.g.is_digraph(), hidden, dist, unreached);
        if (e.empty() and unreached > 0)
          e = "the tree has nodes that its root does not reach";
        if (e.empty())
          e = coverage_error(row, dist);
        if (e.empty() and (root == nullptr or num(root) != u))
          e = "the root is not the node of the start";
        if (e.empty())
          e = mapping_error(tree, b);
        if (e.empty() and solver.is_painted())
          e = "the solver says that it painted";
        if (not e.empty())
          {
            c.tree.fail(s, from + e);
            continue;
          }

        for (int v = 0; v < s.n; ++v)
          check_tree_path<T>(c, solver, tree, b, s, d, u, v);
      }
  }

  /// Runs partial(src, tgt, tree), which builds in tree the tree of the
  /// shortest paths from src until it reaches tgt, and gives the node of
  /// tgt in the tree or nullptr, from every node to every node; checks
  /// the tree, its mapping and the path to tgt that
  /// solver.get_min_path(tree, end, path) gives
  template <typename T, class Solver, class GT, class Partial>
  void check_partial_trees(Tree_Checks & c, Solver & solver, Built<GT> & b, const Spec & s,
                           const Matrix & d, unsigned long hidden, Partial && partial)
  {
    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          const std::string where = route(u, v);
          const auto & row = d[size_t(u)];
          const long long want = row[size_t(v)];
          c.tree.run();
          GT tree;
          typename GT::Node * end = nullptr;
          if (auto e = what_throws([&]
                { end = partial(b.nodes[size_t(u)], b.nodes[size_t(v)], tree); });
              not e.empty())
            {
              c.tree.fail(s, where + e);
              continue;
            }
          std::vector<long long> dist;
          size_t unreached = 0;
          std::string e = tree_error(tree, s, u, row, b.g.is_digraph(), hidden, dist, unreached);
          if (e.empty() and unreached > 0)
            e = "the tree has nodes that its root does not reach";
          if (e.empty() and want != Inf and (end == nullptr or num(end) != v))
            e = "the tree lacks the end";
          if (e.empty() and want == Inf and end != nullptr)
            e = "the tree has an end not reached";
          if (e.empty())
            e = mapping_error(tree, b);
          if (not e.empty())
            {
              c.tree.fail(s, where + e);
              continue;
            }

          check_tree_path<T>(c, solver, tree, b, s, d, u, v);
        }
  }

  /// The distance that Floyd_All_Shortest_Paths gives for want: max(),
  /// which means no path, for one that is not below max()
  template <typename T>
  T floyd_value(long long want)
  {
    if (want == Inf
        or (std::is_integral_v<T> and want >= (long long) std::numeric_limits<T>::max()))
      return std::numeric_limits<T>::max();
    return as<T>(want);
  }

  /// The checks of Floyd_All_Shortest_Paths
  struct Floyd_Checks
  {
    Check dist;
    Check path;
    Check index_path;
    Check next;
    Check nodes;

    explicit Floyd_Checks(const std::string & how = "")
      : dist("Floyd_All_Shortest_Paths::get_dist_mat()" + how),
        path("Floyd_All_Shortest_Paths::get_min_path(src, tgt)" + how),
        index_path("Floyd_All_Shortest_Paths::get_min_path(i, j)" + how),
        next("Floyd_All_Shortest_Paths::get_path_mat()" + how),
        nodes("Floyd_All_Shortest_Paths::get_nodes()" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&dist, &path, &index_path, &next, &nodes})
        c->expect();
    }
  };

  /// Checks the matrices, the paths and the nodes of f, a solver of the
  /// graph of b with the arcs that hidden shows, against d
  template <typename T, class Floyd, class GT>
  void check_floyd(Floyd_Checks & c, const Floyd & f, Built<GT> & b, const Spec & s,
                   const Matrix & d, unsigned long hidden, bool full = true)
  {
    const bool directed = b.g.is_digraph();
    const auto & mat = f.get_dist_mat();
    const auto & next = f.get_path_mat();

    if (full)
      {
        c.nodes.run();
        const auto nodes = f.get_nodes();
        const auto & ref = f.get_nodes_ref();
        const auto copy = f.get_nodes_copy();
        std::string e;
        if (nodes.size() != size_t(s.n) or ref.size() != nodes.size()
            or copy.size() != nodes.size())
          e = "the nodes are not those of the graph";
        for (long i = 0; e.empty() and i < s.n; ++i)
          if (nodes(i) != ref(i) or nodes(i) != copy(i) or f.select_node(i) != nodes(i)
              or f.index_node(nodes(i)) != i or (i > 0 and not (nodes(i - 1) < nodes(i))))
            e = "the node at " + std::to_string(i) + " is wrong, or out of order";
        std::set<int> seen;
        for (long i = 0; e.empty() and i < s.n; ++i)
          if (not seen.insert(num(nodes(i))).second)
            e = "a node twice";
        if (not e.empty())
          c.nodes.fail(s, e);
      }

    c.dist.run();
    if (f.has_negative_cycle())
      {
        c.dist.fail(s, "finds a negative cycle");
        return;
      }
    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          auto * nu = b.nodes[size_t(u)];
          auto * nv = b.nodes[size_t(v)];
          const long i = f.index_node(nu), j = f.index_node(nv);
          const long long want = d[size_t(u)][size_t(v)];
          const T value = floyd_value<T>(want);
          const bool none = value == std::numeric_limits<T>::max();
          if (mat(i, j) != value)
            c.dist.fail(s, route(u, v) + "gives " + text_of(mat(i, j)) + ", not " + text_of(value));

          for (Check * pc : {&c.path, &c.index_path})
            {
              if (pc == &c.index_path and not full)
                continue;
              pc->run();
              Path<GT> path(b.g);
              if (auto e = what_throws([&]
                    { path = pc == &c.path ? f.get_min_path(nu, nv) : f.get_min_path(i, j); });
                  not e.empty())
                pc->fail(s, route(u, v) + e);
              else if (none)
                {
                  if (not path.is_empty())
                    pc->fail(s, route(u, v) + "gives the path " + walk_of(path).str()
                             + " to a node not reached");
                }
              else if (auto pe = path_error(b.g, s, path, u, v, hidden, want, mat(i, j));
                       not pe.empty())
                pc->fail(s, route(u, v) + pe);
            }

          // The next node, through an arc that is the first of a shortest
          // path
          if (not full)
            continue;
          c.next.run();
          const long k = next(i, j);
          bool ok;
          if (u == v)
            ok = k == j;
          else if (none)
            ok = k == -1 or (std::is_integral_v<T> and want != Inf);
          else
            {
              ok = false;
              const int x = k >= 0 and k < s.n ? num(f.select_node(k)) : -1;
              for (size_t a = 0; x >= 0 and a < s.arcs.size() and not ok; ++a)
                {
                  const int p = s.arcs[a][0], q = s.arcs[a][1];
                  ok = graph_filters::shown(hidden, int(a))
                    and ((p == u and q == x) or (not directed and q == u and p == x))
                    and d[size_t(x)][size_t(v)] != Inf
                    and s.w[a] + d[size_t(x)][size_t(v)] == want;
                }
            }
          if (not ok)
            c.next.fail(s, route(u, v) + "the next node is at " + std::to_string(k));
        }
  }

  /// The weight of an arc by its number, as Weight_Of, for Johnson, which
  /// calls it on the arcs of the graph only
  template <class GT, typename T>
  struct Johnson_Weight
  {
    using Distance_Type = T;

    const std::vector<T> * weights = nullptr;

    T operator () (typename GT::Arc * a) const
    {
      return (*weights)[size_t(a->get_info())];
    }
  };

  /// The distance that Johnson gives to a node that is not reached
  template <typename T>
  T johnson_none()
  {
    if constexpr (std::numeric_limits<T>::has_infinity)
      return std::numeric_limits<T>::infinity();
    else
      return std::numeric_limits<T>::max();
  }

  /// The checks of Johnson
  struct Johnson_Checks
  {
    Check build;
    Check distance;
    Check self;
    Check path;
    Check all_pairs;
    Check potential;
    Check copy;
    Check reweighted;

    explicit Johnson_Checks(const std::string & how = "", bool answers_known = false,
                            bool reweighted_known = false)
      : build("Johnson::Johnson()" + how),
        distance("Johnson::get_distance()" + how, answers_known),
        self("Johnson::get_distance(), from a node to itself" + how),
        path("Johnson::find_path()" + how, answers_known),
        all_pairs("Johnson::compute_all_pairs_distances()" + how, answers_known),
        potential("Johnson::get_potential()" + how),
        copy("Johnson::get_copy_node()" + how),
        reweighted("Johnson::get_reweighted_graph()" + how, reweighted_known)
    {}

    void expect() const
    {
      for (const Check * c : {&build, &distance, &self, &path, &all_pairs, &potential,
                              &copy, &reweighted})
        c->expect();
    }
  };

  /// Builds a Johnson of the digraph of b, with the arcs that sa shows
  /// (hidden for the oracle) and the weights w, and checks its answers
  /// against d, which has no negative cycle
  template <typename T, class GT, class SA>
  void check_johnson(Johnson_Checks & c, Built<GT> & b, const Spec & s, const Matrix & d,
                     unsigned long hidden, const std::vector<T> & w, SA sa)
  {
    using Solver = Johnson<GT, Johnson_Weight<GT, T>, Arc_Iterator, Node_Arc_Iterator, SA>;
    c.build.run();
    std::unique_ptr<Solver> j;
    if (auto e = what_throws([&]
          { j = std::make_unique<Solver>(b.g, Johnson_Weight<GT, T>{&w}, sa); });
        not e.empty())
      {
        c.build.fail(s, e);
        return;
      }
    if (not j->is_initialized())
      c.build.fail(s, "is not initialized");

    const auto h = potentials(d);
    auto value = [&] (long long want) { return want == Inf ? johnson_none<T>() : as<T>(want); };

    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          auto * nu = b.nodes[size_t(u)];
          auto * nv = b.nodes[size_t(v)];
          const long long want = d[size_t(u)][size_t(v)];
          Check & dc = u == v ? c.self : c.distance;
          dc.run();
          T got{};
          if (auto e = what_throws([&] { got = j->get_distance(nu, nv); }); not e.empty())
            dc.fail(s, route(u, v) + e);
          else if (got != value(want))
            dc.fail(s, route(u, v) + "gives " + text_of(got) + ", not " + text_of(value(want)));

          c.path.run();
          Path<GT> path(b.g);
          if (auto e = what_throws([&] { path = j->find_path(nu, nv); }); not e.empty())
            c.path.fail(s, route(u, v) + e);
          else if (want == Inf)
            {
              if (not path.is_empty())
                c.path.fail(s, route(u, v) + "gives the path " + walk_of(path).str()
                            + " to a node not reached");
            }
          else if (auto pe = path_error(b.g, s, path, u, v, hidden, want, as<T>(want));
                   not pe.empty())
            c.path.fail(s, route(u, v) + pe);
        }

    c.all_pairs.run();
    if (auto e = what_throws([&]
          {
            const auto all = j->compute_all_pairs_distances();
            std::string wrong = all.size() == size_t(s.n * s.n) ? "" : "lacks pairs";
            for (int u = 0; wrong.empty() and u < s.n; ++u)
              for (int v = 0; wrong.empty() and v < s.n; ++v)
                {
                  const auto * p = all.search({b.nodes[size_t(u)], b.nodes[size_t(v)]});
                  const T want = value(d[size_t(u)][size_t(v)]);
                  if (p == nullptr or p->second != want)
                    wrong = route(u, v) + "gives "
                      + (p == nullptr ? std::string("nothing") : text_of(p->second))
                      + ", not " + text_of(want);
                }
            if (not wrong.empty())
              c.all_pairs.fail(s, wrong);
          }); not e.empty())
      c.all_pairs.fail(s, e);

    // Again, with another source at each query, after the tree of the last
    // source that compute_all_pairs_distances() painted
    for (int v = 0; v < s.n; ++v)
      for (int u = s.n - 1; u >= 0; --u)
        {
          c.distance.run();
          T got{};
          const long long want = d[size_t(u)][size_t(v)];
          if (auto e = what_throws([&]
                { got = j->get_distance(b.nodes[size_t(u)], b.nodes[size_t(v)]); });
              not e.empty())
            c.distance.fail(s, route(u, v) + "again: " + e);
          else if (got != value(want))
            c.distance.fail(s, route(u, v) + "again: gives " + text_of(got) + ", not "
                            + text_of(value(want)));
        }

    for (int v = 0; v < s.n; ++v)
      {
        c.potential.run();
        T got{};
        if (auto e = what_throws([&] { got = j->get_potential(b.nodes[size_t(v)]); });
            not e.empty())
          c.potential.fail(s, "of " + std::to_string(v) + ": " + e);
        else if (got != as<T>(h[size_t(v)]))
          c.potential.fail(s, "of " + std::to_string(v) + ": gives " + text_of(got) + ", not "
                           + text_of(as<T>(h[size_t(v)])));

        c.copy.run();
        auto * p = j->get_copy_node(b.nodes[size_t(v)]);
        if (p == nullptr or num(p) != v)
          c.copy.fail(s, "of " + std::to_string(v) + ": a node of another number");
      }

    // The arcs of the copy weigh w + h(x) - h(y) >= 0, as many between two
    // nodes as in the graph; the hidden ones are not reweighted
    if (hidden != 0)
      return;
    // The info of each arc of the copy is its new weight w + h(u) - h(v),
    // which is not negative, converted to the type of the info: an int
    // info keeps the integer part of a double new weight
    c.reweighted.run();
    using Info = typename GT::Arc_Type;
    std::multiset<std::tuple<int, int, Info>> want, got;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        const int x = s.arcs[i][0], y = s.arcs[i][1];
        want.insert({x, y, static_cast<Info>(w[i] + as<T>(h[size_t(x)]) - as<T>(h[size_t(y)]))});
      }
    std::string e;
    const GT & copy = j->get_reweighted_graph();
    for (auto it = copy.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr();
        const int x = num(copy.get_src_node(a)), y = num(copy.get_tgt_node(a));
        if (a->get_info() < Info(0))
          e = "the arc from " + std::to_string(x) + " to " + std::to_string(y)
            + " weighs " + std::to_string(a->get_info());
        got.insert({x, y, a->get_info()});
      }
    if (e.empty() and got != want)
      e = "the infos of the reweighted arcs are not their new weights";
    if (not e.empty())
      c.reweighted.fail(s, e);
  }

  /// What is wrong with items, the k shortest paths of s from u to v that
  /// an algorithm gave, against want, the costs that they must have in
  /// order: as many paths, each from u to v along arcs that hidden shows
  /// (and simple if simple), with the cost that came with it, and none
  /// twice; empty if nothing
  template <typename T, class GT, class Items>
  std::string items_error(const GT & g, const Spec & s, const Items & items, int u, int v,
                          unsigned long hidden, const std::vector<long long> & want, bool simple)
  {
    std::vector<Walk> seen;
    size_t i = 0;
    for (auto it = items.get_it(); it.has_curr(); it.next_ne(), ++i)
      {
        const auto & item = it.get_curr();
        if (i >= want.size())
          return "gives more than " + std::to_string(want.size()) + " paths";
        if (not item.path.inside_graph(g))
          return "a path of another graph";
        const Walk w = walk_of(item.path);
        if (auto e = walk_error(s, w, u, v, g.is_digraph(), hidden); not e.empty())
          return e;
        if (simple and not is_simple(w))
          return "the path " + w.str() + " repeats a node";
        const long long c = cost_of(s, w);
        if (item.total_cost != as<T>(c))
          return "the path " + w.str() + " comes with the cost " + text_of(item.total_cost)
            + ", not " + text_of(as<T>(c));
        if (c != want[i])
          return "the path " + std::to_string(i) + ", " + w.str() + ", costs "
            + std::to_string(c) + ", not " + std::to_string(want[i]);
        if (std::find(seen.begin(), seen.end(), w) != seen.end())
          return "gives the path " + w.str() + " twice";
        seen.push_back(w);
      }
    if (i != want.size())
      return "gives " + std::to_string(i) + " paths, not " + std::to_string(want.size());
    return "";
  }

  /// The checks of the k shortest paths. Eppstein had two classes of
  /// cases of its own, which the harness found: the walks that go on from
  /// the end and come back to it, which it did not give, and the parallel
  /// arcs of a digraph, of which it could take the heavier.
  struct K_Paths_Checks
  {
    Check yen;
    Check yen_class;
    Check eppstein;
    Check through;
    Check parallel;
    Check none;

    explicit K_Paths_Checks(const std::string & how = "")
      : yen("yen_k_shortest_paths()" + how),
        yen_class("Yen_K_Shortest_Paths" + how),
        eppstein("eppstein_k_shortest_paths() and Eppstein_K_Shortest_Paths" + how),
        through("eppstein_k_shortest_paths() and Eppstein_K_Shortest_Paths" + how
                + ", to a node on a cycle"),
        parallel("eppstein_k_shortest_paths() and Eppstein_K_Shortest_Paths" + how
                 + ", with parallel arcs"),
        none("yen_k_shortest_paths() and eppstein_k_shortest_paths(), with k = 0" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&yen, &yen_class, &eppstein, &through, &parallel, &none})
        c->expect();
    }
  };

  /** Runs Yen and Eppstein on the graph of b, with the arcs that sa shows
      (hidden for the oracle), from every node to every node, for a few k:
      Yen gives the k cheapest simple paths in cost order, and Eppstein
      the k cheapest walks, which may repeat nodes and arcs, also in cost
      order. Both give the path of no arc alone from a node to itself, as
      the documentation of Eppstein says. Ties may come in any order. */
  template <typename T, class GT, class SA>
  void check_k_shortest(K_Paths_Checks & c, Built<GT> & b, const Spec & s,
                        unsigned long hidden, const std::vector<T> & w, SA sa,
                        bool yen = true, bool eppstein = true, bool light = false)
  {
    using W = Weights<GT, T>;
    std::vector<int> orig;
    const Spec shown = oracle::without(s, hidden, orig);
    const bool directed = b.g.is_digraph();
    const auto r = oracle::closure(shown, directed);
    const bool parallel = directed and has_parallel_arcs(shown, true);

    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          auto * nu = b.nodes[size_t(u)];
          auto * nv = b.nodes[size_t(v)];
          const auto simple = simple_path_costs(shown, u, v, directed);
          for (size_t k : {size_t(1), size_t(2), simple.size() + 1})
            for (Check * kc : {&c.yen, &c.yen_class})
              {
                if (not yen)
                  break;
                if ((kc == &c.yen_class or light) and k != 2)
                  continue;
                kc->run();
                const std::vector<long long>
                  want(simple.begin(), simple.begin() + long(std::min(k, simple.size())));
                DynList<K_Shortest_Path_Item<GT, T>> items;
                if (auto e = what_throws([&]
                      {
                        if (kc == &c.yen)
                          items = yen_k_shortest_paths<GT, W, SA>(b.g, nu, nv, k, W{&w}, sa);
                        else
                          items = Yen_K_Shortest_Paths<GT, W, SA>(W{&w}, sa)(b.g, nu, nv, k);
                      }); not e.empty())
                  kc->fail(s, route(u, v) + "k = " + std::to_string(k) + ": " + e);
                else if (auto ie = items_error<T>(b.g, s, items, u, v, hidden, want, true);
                         not ie.empty())
                  kc->fail(s, route(u, v) + "k = " + std::to_string(k) + ": " + ie);
              }

          // A walk that may go on from the end, or a choice among parallel
          // arcs, which are classes of their own
          const bool on_cycle = u != v and r[size_t(u)][size_t(v)] and r[size_t(v)][size_t(v)];
          Check & split = on_cycle ? c.through : parallel ? c.parallel : c.eppstein;
          for (size_t k : {size_t(1), size_t(3), size_t(6)})
            for (bool by_class : {false, true})
              {
                if (not eppstein)
                  break;
                if ((by_class or light) and k != 3)
                  continue;
                Check * kc = &split;
                kc->run();
                const std::vector<long long> want
                  = u == v ? std::vector<long long>{0} : walk_costs(shown, u, v, directed, k);
                DynList<K_Shortest_Path_Item<GT, T>> items;
                if (auto e = what_throws([&]
                      {
                        if (not by_class)
                          items = eppstein_k_shortest_paths<GT, W, SA>(b.g, nu, nv, k, W{&w}, sa);
                        else
                          items = Eppstein_K_Shortest_Paths<GT, W, SA>(W{&w}, sa)(b.g, nu, nv, k);
                      }); not e.empty())
                  kc->fail(s, route(u, v) + "k = " + std::to_string(k) + ": " + e);
                else if (auto ie = items_error<T>(b.g, s, items, u, v, hidden, want, false);
                         not ie.empty())
                  kc->fail(s, route(u, v) + "k = " + std::to_string(k) + ": " + ie);
              }
        }

    if (s.n == 0 or not yen or not eppstein)
      return;
    c.none.run();
    if (auto e = what_throws([&]
          {
            if (not yen_k_shortest_paths<GT, W, SA>(b.g, b.nodes[0], b.nodes[0], 0, W{&w}, sa)
                      .is_empty()
                or not eppstein_k_shortest_paths<GT, W, SA>(b.g, b.nodes[0], b.nodes[0], 0,
                                                            W{&w}, sa).is_empty())
              c.none.fail(s, "gives paths");
          }); not e.empty())
      c.none.fail(s, e);
  }

  /// As Weight_Of, with the weights in a variable of the class, for the
  /// constructors that build the distance with Distance()
  template <class GT, typename T>
  struct Static_Weight
  {
    using Distance_Type = T;

    inline static const std::vector<T> * weights = nullptr;

    T operator () (typename GT::Arc * a) const { return (*weights)[size_t(a->get_info())]; }
  };

  /// shape with weights 0 and 1 in turn, from first
  Spec alternating(const Spec & shape, long long first)
  {
    return weighted(shape, [first] (size_t i) { return (first + (long long) i) % 2; });
  }

  /// shape with the weights -1, 0, 1, -1 ... in a digraph, where they may
  /// make a negative cycle; with the falling powers of 2 if not directed,
  /// where a negative arc is one
  Spec with_negatives(const Spec & shape, bool directed)
  {
    if (not directed)
      return weighted(shape, Weighting::Falling);
    return weighted(shape, [] (size_t i) { return (long long) (i % 3) - 1; });
  }
}

namespace
{
  /// Runs the main checks of Dijkstra_Min_Paths and AStar_Min_Path with
  /// distances of type T on the graphs of up to 3 nodes and 3 arcs
  template <typename T, class GT>
  void check_dijkstra_and_astar_with(const std::string & type)
  {
    using Node = typename GT::Node;
    Painting_Checks paint("Dijkstra_Min_Paths", ", " + type);
    Check copy("Dijkstra_Min_Paths::copy_painted_min_paths_tree(), " + type);
    Check find("Dijkstra_Min_Paths::find_min_path(), " + type);
    Tree_Checks tree("Dijkstra_Min_Paths::compute_min_paths_tree(), " + type);
    Check astar("AStar_Min_Path::find_path(), " + type);
    Check astar_self("AStar_Min_Path::find_path(), " + type + ", from a node to itself");

    std::vector<T> w;
    Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});
    AStar_Of<GT, T> ast(Weights<GT, T>{&w});

    for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
      {
        for (Weighting k : {Weighting::Ones, Weighting::Falling})
          {
            const Spec s = weighted(shape, k);
            Built<GT> b(s);
            w = weights_as<T>(s);
            const Matrix d = distances(s, b.g.is_digraph());
            check_painting<T>(paint, dij, b, s, d, 0);
            check_copies<T>(copy, nullptr, dij, b, s, d, 0);
            check_find<T>(find, nullptr, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
              { return dij.find_min_path(b.g, x, y, p); });
            check_trees<T>(tree, dij, b, s, d, 0, [&] (Node * x, GT & t)
              { return dij.compute_min_paths_tree(b.g, x, t); });
            check_find<T>(astar, &astar_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
              { return ast.find_path(b.g, x, y, p); });
          }
      });

    paint.expect();
    copy.expect();
    find.expect();
    tree.expect();
    astar.expect();
    astar_self.expect();
  }
}

template <class GT>
class ExhaustivePaths : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustivePaths, All_Kinds);

TYPED_TEST(ExhaustivePaths, DijkstraPaintingsMatchTheOracle)
{
  using GT = TypeParam;
  using T = long long;
  Painting_Checks paint("Dijkstra_Min_Paths");

  // The same solver for every graph
  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : {Weighting::Ones, Weighting::Falling})
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_painting<T>(paint, dij, b, s, distances(s, b.g.is_digraph()), 0);
        }
    });

  paint.expect();
}

TYPED_TEST(ExhaustivePaths, AStarPaintingsMatchTheOracle)
{
  using GT = TypeParam;
  using T = long long;
  Painting_Checks paint("AStar_Min_Path");

  std::vector<T> w;
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : {Weighting::Ones, Weighting::Falling})
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_painting<T>(paint, ast, b, s, distances(s, b.g.is_digraph()), 0);
        }
    });

  paint.expect();
}

TYPED_TEST(ExhaustivePaths, PaintedTreesAreCopied)
{
  using GT = TypeParam;
  using T = long long;
  Check dijkstra("Dijkstra_Min_Paths::copy_painted_min_paths_tree()");
  Check astar("AStar_Min_Path::copy_painted_min_paths_tree()");
  // The copy took every node of the graph, also those that the painting
  // does not reach, which compute_min_paths_tree() leaves out
  Check d_nodes("Dijkstra_Min_Paths::copy_painted_min_paths_tree(), the nodes not reached");
  Check a_nodes("AStar_Min_Path::copy_painted_min_paths_tree(), the nodes not reached");

  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          check_copies<T>(dijkstra, &d_nodes, dij, b, s, d, 0);
          if (k == Weighting::Falling)
            check_copies<T>(astar, &a_nodes, ast, b, s, d, 0);
        }
    });

  dijkstra.expect();
  astar.expect();
  d_nodes.expect();
  a_nodes.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraFindsTheShortestPaths)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Check find("Dijkstra_Min_Paths::find_min_path()");
  Check op("Dijkstra_Min_Paths::operator()(g, start, end, path)");
  Check partial("Dijkstra_Min_Paths::paint_partial_min_paths_tree()");
  // The partial painting did not take its start for its end, so that from
  // a node to itself there was no path; there is the path of no arc
  Check find_self("Dijkstra_Min_Paths::find_min_path(), from a node to itself");
  Check op_self("Dijkstra_Min_Paths::operator()(g, start, end, path), from a node to itself");
  Check partial_self("Dijkstra_Min_Paths::paint_partial_min_paths_tree(), from a node to itself");

  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          check_find<T>(find, &find_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
            { return dij.find_min_path(b.g, x, y, p); });
          if (k == Weighting::Falling)
            check_partial_painting<T>(partial, partial_self, dij, b, s, d, 0,
                                      [&] (Node * x, Node * y)
              { return dij.paint_partial_min_paths_tree(b.g, x, y); });
        }
    });

  // operator() gives find_min_path(): the smaller domain suffices
  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec s = weighted(shape, Weighting::Rising);
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_find<T>(op, &op_self, b, s, distances(s, b.g.is_digraph()), 0,
                    [&] (Node * x, Node * y, Path<GT> & p)
        { return dij(b.g, x, y, p); });
    });

  for (const Check * c : {&find, &op, &partial, &find_self, &op_self, &partial_self})
    c->expect();
}

TYPED_TEST(ExhaustivePaths, AStarFindsTheShortestPaths)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Check find_path("AStar_Min_Path::find_path()");
  Check find_path_self("AStar_Min_Path::find_path(), from a node to itself");
  Check op("AStar_Min_Path::operator()(g, start, end, path)");
  Check op_self("AStar_Min_Path::operator()(g, start, end, path), from a node to itself");
  Check paint_path("AStar_Min_Path::paint_partial_path()");
  Check paint_path_self("AStar_Min_Path::paint_partial_path(), from a node to itself");
  Check alias("AStar_Min_Path::paint_path()");
  Check alias_self("AStar_Min_Path::paint_path(), from a node to itself");

  std::vector<T> w;
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          if (k != Weighting::Rising)
            check_find<T>(find_path, &find_path_self, b, s, d, 0,
                          [&] (Node * x, Node * y, Path<GT> & p)
              { return ast.find_path(b.g, x, y, p); });
          if (k == Weighting::Falling)
            check_partial_painting<T>(paint_path, paint_path_self, ast, b, s, d, 0,
                                      [&] (Node * x, Node * y)
              { return ast.paint_partial_path(b.g, x, y); });
        }
    });

  // operator() gives find_path(), and paint_path() paint_partial_path():
  // the smaller domain suffices
  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : {Weighting::Ones, Weighting::Rising})
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          if (k == Weighting::Rising)
            check_find<T>(op, &op_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
              { return ast(b.g, x, y, p); });
          else
            check_partial_painting<T>(alias, alias_self, ast, b, s, d, 0, [&] (Node * x, Node * y)
              { return ast.paint_path(b.g, x, y); });
        }
    });

  for (const Check * c : {&find_path, &find_path_self, &op, &op_self, &paint_path,
                          &paint_path_self, &alias, &alias_self})
    c->expect();
}

TYPED_TEST(ExhaustivePaths, AStarWithoutAHeuristicFindsTheShortestPaths)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Check find("AStar_Min_Path::find_min_path()");
  Check find_self("AStar_Min_Path::find_min_path(), from a node to itself");
  Check partial("AStar_Min_Path::paint_partial_min_paths_tree()");
  Check partial_self("AStar_Min_Path::paint_partial_min_paths_tree(), from a node to itself");

  std::vector<T> w;
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          check_find<T>(find, &find_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
            { return ast.find_min_path(b.g, x, y, p); });
          if (k == Weighting::Falling)
            check_partial_painting<T>(partial, partial_self, ast, b, s, d, 0,
                                      [&] (Node * x, Node * y)
              { return ast.paint_partial_min_paths_tree(b.g, x, y); });
        }
    });

  for (const Check * c : {&find, &find_self, &partial, &partial_self})
    c->expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraTreesMatchTheOracle)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Tree_Checks tree("Dijkstra_Min_Paths::compute_min_paths_tree()");
  Tree_Checks op("Dijkstra_Min_Paths::operator()(g, start, tree)");
  Tree_Checks partial("Dijkstra_Min_Paths::compute_partial_min_paths_tree()");

  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});

  // The falling powers of 2 make the first of two parallel arcs the heavier
  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec s = weighted(shape, Weighting::Falling);
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_trees<T>(tree, dij, b, s, distances(s, b.g.is_digraph()), 0,
                           [&] (Node * x, GT & t)
        { return dij.compute_min_paths_tree(b.g, x, t); });
    });

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          if (k == Weighting::Rising)
            continue;
          if (k == Weighting::Falling)
            check_trees<T>(op, dij, b, s, d, 0, [&] (Node * x, GT & t)
              {
                dij(b.g, x, t);
                return node_of(t, num(x));
              });
          check_partial_trees<T>(partial, dij, b, s, d, 0, [&] (Node * x, Node * y, GT & t)
            {
              dij.compute_partial_min_paths_tree(b.g, x, y, t);
              return node_of(t, num(y));
            });
        }
    });

  tree.expect();
  op.expect();
  partial.expect();
}

TYPED_TEST(ExhaustivePaths, AStarTreesMatchTheOracle)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Tree_Checks tree("AStar_Min_Path::compute_min_paths_tree()");
  Tree_Checks op("AStar_Min_Path::operator()(g, start, tree)");
  Tree_Checks partial("AStar_Min_Path::compute_partial_min_paths_tree()");
  Tree_Checks path("AStar_Min_Path::compute_partial_path()");
  Tree_Checks alias("AStar_Min_Path::compute_path()");

  std::vector<T> w;
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec s = weighted(shape, Weighting::Falling);
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_trees<T>(tree, ast, b, s, distances(s, b.g.is_digraph()), 0,
                           [&] (Node * x, GT & t)
        { return ast.compute_min_paths_tree(b.g, x, t); });
    });

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          if (k == Weighting::Rising)
            continue;
          check_partial_trees<T>(partial, ast, b, s, d, 0, [&] (Node * x, Node * y, GT & t)
            {
              ast.compute_partial_min_paths_tree(b.g, x, y, t);
              return node_of(t, num(y));
            });
          if (k != Weighting::Falling)
            continue;
          check_trees<T>(op, ast, b, s, d, 0, [&] (Node * x, GT & t)
            {
              ast(b.g, x, t);
              return node_of(t, num(x));
            });
          check_partial_trees<T>(path, ast, b, s, d, 0, [&] (Node * x, Node * y, GT & t)
            { return ast.compute_partial_path(b.g, x, y, t); });
          check_partial_trees<T>(alias, ast, b, s, d, 0, [&] (Node * x, Node * y, GT & t)
            { return ast.compute_path(b.g, x, y, t); });
        }
    });

  tree.expect();
  op.expect();
  partial.expect();
  path.expect();
  alias.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraPaintsWithEveryWeighting)
{
  using GT = TypeParam;
  using T = long long;
  Painting_Checks paint("Dijkstra_Min_Paths", ", every weighting");

  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for_each_weighting(shape, {0, 1, 2}, [&] (const Spec & s)
        {
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_painting<T>(paint, dij, b, s, distances(s, b.g.is_digraph()), 0);
        });
    });

  paint.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraFindsWithEveryWeighting)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Check find("Dijkstra_Min_Paths::find_min_path(), every weighting");

  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for_each_weighting(shape, {0, 1, 2}, [&] (const Spec & s)
        {
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_find<T>(find, nullptr, b, s, distances(s, b.g.is_digraph()), 0,
                        [&] (Node * x, Node * y, Path<GT> & p)
            { return dij.find_min_path(b.g, x, y, p); });
        });
    });

  find.expect();
}

TYPED_TEST(ExhaustivePaths, AStarFindsWithEveryWeighting)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Check find("AStar_Min_Path::find_path(), every weighting");
  Check find_self("AStar_Min_Path::find_path(), every weighting, from a node to itself");

  std::vector<T> w;
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for_each_weighting(shape, {0, 1, 2}, [&] (const Spec & s)
        {
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_find<T>(find, &find_self, b, s, distances(s, b.g.is_digraph()), 0,
                        [&] (Node * x, Node * y, Path<GT> & p)
            { return ast.find_path(b.g, x, y, p); });
        });
    });

  find.expect();
  find_self.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraAndAStarOnRandomGraphs)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Painting_Checks paint("Dijkstra_Min_Paths", ", random graphs");
  Check copy("Dijkstra_Min_Paths::copy_painted_min_paths_tree(), random graphs");
  Check find("Dijkstra_Min_Paths::find_min_path(), random graphs");
  Check astar("AStar_Min_Path::find_path(), random graphs");
  Check astar_self("AStar_Min_Path::find_path(), random graphs, from a node to itself");
  Tree_Checks tree("Dijkstra_Min_Paths::compute_min_paths_tree(), random graphs");

  std::vector<T> w;
  Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});
  AStar_Of<GT, T> ast(Weights<GT, T>{&w});

  for (const Spec & s : random_specs(3301, GT().is_digraph(), 60, 0, 9))
    {
      Built<GT> b(s);
      w = weights_as<T>(s);
      const Matrix d = distances(s, b.g.is_digraph());
      check_painting<T>(paint, dij, b, s, d, 0);
      check_copies<T>(copy, nullptr, dij, b, s, d, 0);
      check_find<T>(find, nullptr, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
        { return dij.find_min_path(b.g, x, y, p); });
      check_find<T>(astar, &astar_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
        { return ast.find_path(b.g, x, y, p); });
      check_trees<T>(tree, dij, b, s, d, 0, [&] (Node * x, GT & t)
        { return dij.compute_min_paths_tree(b.g, x, t); });
    }

  paint.expect();
  copy.expect();
  find.expect();
  astar.expect();
  astar_self.expect();
  tree.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraRespectsTheFilter)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  using F = Hide_Mask<GT>;
  Painting_Checks paint("Dijkstra_Min_Paths", ", hiding arcs");
  Check copy("Dijkstra_Min_Paths::copy_painted_min_paths_tree(), hiding arcs");
  Check find("Dijkstra_Min_Paths::find_min_path(), hiding arcs");
  Tree_Checks tree("Dijkstra_Min_Paths::compute_min_paths_tree(), hiding arcs");

  // The hidden arcs weigh -1, which a search that took them would notice
  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec base = weighted(shape, Weighting::Falling);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          const Spec s = poisoned(base, hidden, -1);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph(), hidden);
          Dijkstra_Of<GT, T, F> dij(Weights<GT, T>{&w}, F{hidden});
          check_painting<T>(paint, dij, b, s, d, hidden);
          check_copies<T>(copy, nullptr, dij, b, s, d, hidden);
          check_find<T>(find, nullptr, b, s, d, hidden, [&] (Node * x, Node * y, Path<GT> & p)
            { return dij.find_min_path(b.g, x, y, p); });
          check_trees<T>(tree, dij, b, s, d, hidden, [&] (Node * x, GT & t)
            { return dij.compute_min_paths_tree(b.g, x, t); });
        }
    });

  paint.expect();
  copy.expect();
  find.expect();
  tree.expect();
}

TYPED_TEST(ExhaustivePaths, AStarRespectsTheFilter)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  using F = Hide_Mask<GT>;
  Painting_Checks paint("AStar_Min_Path", ", hiding arcs");
  Check find_path("AStar_Min_Path::find_path(), hiding arcs");
  Check find_path_self("AStar_Min_Path::find_path(), hiding arcs, from a node to itself");
  Check find("AStar_Min_Path::find_min_path(), hiding arcs");
  Tree_Checks path("AStar_Min_Path::compute_partial_path(), hiding arcs");

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec base = weighted(shape, Weighting::Falling);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          const Spec s = poisoned(base, hidden, -1);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph(), hidden);
          AStar_Of<GT, T, F> ast(Weights<GT, T>{&w}, No_Heuristic<GT, T>(), F{hidden});
          check_painting<T>(paint, ast, b, s, d, hidden);
          check_find<T>(find_path, &find_path_self, b, s, d, hidden,
                        [&] (Node * x, Node * y, Path<GT> & p)
            { return ast.find_path(b.g, x, y, p); });
          check_find<T>(find, nullptr, b, s, d, hidden, [&] (Node * x, Node * y, Path<GT> & p)
            { return ast.find_min_path(b.g, x, y, p); });
          check_partial_trees<T>(path, ast, b, s, d, hidden, [&] (Node * x, Node * y, GT & t)
            { return ast.compute_partial_path(b.g, x, y, t); });
        }
    });

  paint.expect();
  find_path.expect();
  find_path_self.expect();
  find.expect();
  path.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraAndAStarAfterAnotherPainting)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  using F = Hide_Mask<GT>;
  Painting_Checks paint("Dijkstra_Min_Paths", ", hiding arcs after another painting");
  Check copy("Dijkstra_Min_Paths::copy_painted_min_paths_tree(), hiding arcs after another "
             "painting");
  Check find("Dijkstra_Min_Paths::find_min_path(), hiding arcs after another painting");
  Check astar("AStar_Min_Path::find_path(), hiding arcs after another painting");
  // Dial_Min_Paths resets every arc before it paints (Dial.H:219-220)
  Check dial_find("Dial_Min_Paths::find_min_path(), hiding arcs after another painting");
  Painting_Checks paint_left("Dijkstra_Min_Paths",
                             ", hiding arcs after a painting that left one painted");
  Check copy_left("Dijkstra_Min_Paths::copy_painted_min_paths_tree(), hiding arcs after a "
                  "painting that left one painted");
  Check find_left("Dijkstra_Min_Paths::find_min_path(), hiding arcs after a painting that "
                  "left one painted");
  Check astar_left("AStar_Min_Path::find_path(), hiding arcs after a painting that left one "
                   "painted");

  // The rising weights make the first of two parallel arcs the lighter, the
  // one that the painting without the filter takes
  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec s = weighted(shape, Weighting::Rising);
      const std::vector<T> w = weights_as<T>(s);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          Built<GT> b(s);
          const Matrix d = distances(s, b.g.is_digraph(), hidden);
          Dijkstra_Of<GT, T> plain(Weights<GT, T>{&w});
          Dijkstra_Of<GT, T, F> dij(Weights<GT, T>{&w}, F{hidden});
          AStar_Of<GT, T, F> ast(Weights<GT, T>{&w}, No_Heuristic<GT, T>(), F{hidden});
          Dial_Min_Paths<GT, Weights<GT, T>, Node_Arc_Iterator, F> dial(Weights<GT, T>{&w},
                                                                        F{hidden});
          for (int u = 0; u < s.n; ++u)
            {
              // What an algorithm without the filter leaves on the graph
              plain.paint_min_paths_tree(b.g, b.nodes[size_t(u)]);
              const bool left = painted_hidden_arc(b, hidden);
              check_find<T>(left ? find_left : find, nullptr, b, s, d, hidden,
                            [&] (Node * x, Node * y, Path<GT> & p)
                { return dij.find_min_path(b.g, x, y, p); }, false, u);
              check_find<T>(left ? astar_left : astar, nullptr, b, s, d, hidden,
                            [&] (Node * x, Node * y, Path<GT> & p)
                { return ast.find_path(b.g, x, y, p); }, false, u);
              check_painting<T>(left ? paint_left : paint, dij, b, s, d, hidden, false, u);
              check_copies<T>(left ? copy_left : copy, nullptr, dij, b, s, d, hidden, u);
              plain.paint_min_paths_tree(b.g, b.nodes[size_t(u)]);
              check_find<T>(dial_find, nullptr, b, s, d, hidden,
                            [&] (Node * x, Node * y, Path<GT> & p)
                { return dial.find_min_path(b.g, x, y, p); }, false, u);
            }
        }
    });

  paint.expect();
  copy.expect();
  find.expect();
  astar.expect();
  dial_find.expect();
  paint_left.expect();
  copy_left.expect();
  find_left.expect();
  astar_left.expect();
}

TYPED_TEST(ExhaustivePaths, DijkstraAndAStarTakeIntAndDouble)
{
  check_dijkstra_and_astar_with<int, TypeParam>("int");
  check_dijkstra_and_astar_with<double, TypeParam>("double");
}

namespace
{
  /** A heuristic of A* that gives the distance from a node to the end that
      the matrix *d says, and max() for one beyond max() or for a node that
      does not reach the end: it does not overestimate, it is consistent,
      and it is as near the limits as the distances. */
  template <class GT, typename T>
  struct Exact_Heuristic
  {
    using Distance_Type = T;

    const Matrix * d = nullptr;

    T operator () (typename GT::Node * from, typename GT::Node * to) const
    {
      const long long x = (*d)[size_t(from->get_info())][size_t(to->get_info())];
      return x > (long long) std::numeric_limits<T>::max()
        ? std::numeric_limits<T>::max() : T(x);
    }
  };

  /** Runs, with weights near the limits of int, the painting and
      find_min_path() of Dijkstra_Min_Paths, or find_path() of
      AStar_Min_Path if astar, and then also its find_path() and
      compute_partial_path() with Exact_Heuristic. A distance beyond max()
      gives overflow_error, as documented; one that fits is found. */
  template <class GT>
  void check_near_int_limits(bool astar)
  {
    using T = int;
    constexpr long long M = Int_Max;
    const std::string who = astar ? "AStar_Min_Path::find_path()"
      : "Dijkstra_Min_Paths::find_min_path()";
    Check paint("Dijkstra_Min_Paths::paint_min_paths_tree(), near the limits of int");
    Check paint_beyond("Dijkstra_Min_Paths::paint_min_paths_tree(), a distance beyond max()");
    Check find(who + ", near the limits of int");
    Check find_beyond(who + ", a distance beyond max()");
    // Each relaxation added with checked_add(), which threw for a sum beyond
    // max() also when the node is reached by a lighter path, or is not the
    // end; such a sum no longer throws by itself
    Check paint_sum("Dijkstra_Min_Paths::paint_min_paths_tree(), near the limits of int, "
                    "a sum beyond max()");
    Check find_sum(who + ", near the limits of int, a sum beyond max()");
    // With a heuristic, a sum g + h beyond max() gave the arc the priority
    // max(), which tied with a path of exactly max(): the heap could keep
    // the other arc to the node, and then no path fitted
    Check exact("AStar_Min_Path::find_path() and compute_partial_path(), with the distance "
                "to the end as heuristic, near the limits of int");
    Check exact_beyond("AStar_Min_Path::find_path() and compute_partial_path(), with the "
                       "distance to the end as heuristic, a distance beyond max()");

    std::vector<T> w;
    Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});
    AStar_Of<GT, T> ast(Weights<GT, T>{&w});
    Matrix exact_d;
    AStar_Min_Path<GT, Weights<GT, T>, Exact_Heuristic<GT, T>>
      exact_ast(Weights<GT, T>{&w}, Exact_Heuristic<GT, T>{&exact_d});
    const bool directed = GT().is_digraph();

    for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
      {
        for_each_weighting(shape, {1, M / 2, M - 1}, [&] (const Spec & s)
          {
            Built<GT> b(s);
            w = weights_as<T>(s);
            const Matrix d = distances(s, directed);
            exact_d = d;
            for (int u = 0; u < s.n; ++u)
              {
                const auto & row = d[size_t(u)];
                if (not astar)
                  {
                    const bool fits = std::all_of(row.begin(), row.end(), [] (long long x)
                      { return x == Inf or x <= Int_Max; });
                    Check & pc = not fits ? paint_beyond
                      : sum_beyond(s, d, u, Inf, directed, M) ? paint_sum : paint;
                    pc.run();
                    const auto e = what_throws([&]
                      { dij.paint_min_paths_tree(b.g, b.nodes[size_t(u)]); });
                    if (not fits)
                      {
                        if (not is(e, "overflow_error"))
                          pc.fail(s, "from " + std::to_string(u) + ": gives no overflow_error");
                      }
                    else if (not e.empty())
                      pc.fail(s, "from " + std::to_string(u) + ": " + e);
                    else
                      for (int v = 0; v < s.n; ++v)
                        {
                          if (row[size_t(v)] == Inf)
                            continue;
                          T got{};
                          const auto ge = what_throws([&]
                            { got = dij.get_distance(b.nodes[size_t(v)]); });
                          if (not ge.empty() or got != T(row[size_t(v)]))
                            {
                              pc.fail(s, route(u, v) + (ge.empty() ? "gives " + text_of(got) : ge));
                              break;
                            }
                        }
                  }

                for (int v = 0; v < s.n; ++v)
                  {
                    if (u == v)
                      continue;
                    const long long want = row[size_t(v)];
                    const bool beyond = want != Inf and want > M;
                    Check & c = beyond ? find_beyond
                      : sum_beyond(s, d, u, want, directed, M) ? find_sum : find;
                    c.run();
                    Path<GT> path(b.g);
                    T got{};
                    const auto fe = what_throws([&]
                      {
                        auto * x = b.nodes[size_t(u)];
                        auto * y = b.nodes[size_t(v)];
                        got = astar ? ast.find_path(b.g, x, y, path)
                          : dij.find_min_path(b.g, x, y, path);
                      });
                    if (beyond)
                      {
                        if (not is(fe, "overflow_error"))
                          c.fail(s, route(u, v) + "gives no overflow_error");
                      }
                    else if (not fe.empty())
                      c.fail(s, route(u, v) + fe);
                    else if (want == Inf)
                      {
                        if (got != std::numeric_limits<T>::max() or not path.is_empty())
                          c.fail(s, route(u, v) + "gives a path to a node not reached");
                      }
                    else if (auto pe = path_error(b.g, s, path, u, v, 0, want, got); not pe.empty())
                      c.fail(s, route(u, v) + pe);

                    if (not astar)
                      continue;
                    Check & ec = beyond ? exact_beyond : exact;
                    ec.run();
                    auto * x = b.nodes[size_t(u)];
                    auto * y = b.nodes[size_t(v)];
                    Path<GT> found(b.g);
                    T cost{};
                    const auto ee = what_throws([&] { cost = exact_ast.find_path(b.g, x, y, found); });
                    GT tree;
                    typename GT::Node * reached = nullptr;
                    const auto te = what_throws([&]
                      { reached = exact_ast.compute_partial_path(b.g, x, y, tree); });
                    if (beyond)
                      {
                        if (not is(ee, "overflow_error") or not is(te, "overflow_error"))
                          ec.fail(s, route(u, v) + "gives no overflow_error");
                        continue;
                      }
                    if (not ee.empty() or not te.empty())
                      {
                        ec.fail(s, route(u, v) + ee + te);
                        continue;
                      }
                    if (want == Inf)
                      {
                        if (cost != std::numeric_limits<T>::max() or not found.is_empty()
                            or reached != nullptr)
                          ec.fail(s, route(u, v) + "gives a path to a node not reached");
                        continue;
                      }
                    if (auto pe = path_error(b.g, s, found, u, v, 0, want, cost); not pe.empty())
                      {
                        ec.fail(s, route(u, v) + "find_path(): " + pe);
                        continue;
                      }
                    Path<GT> in_tree(b.g);
                    T tree_cost{};
                    const auto ge = what_throws([&]
                      { tree_cost = exact_ast.get_min_path(tree, y, in_tree); });
                    if (reached == nullptr or not ge.empty())
                      ec.fail(s, route(u, v) + "compute_partial_path() gives no path " + ge);
                    else if (auto tpe = path_error(b.g, s, in_tree, u, v, 0, want, tree_cost);
                             not tpe.empty())
                      ec.fail(s, route(u, v) + "compute_partial_path(): " + tpe);
                  }
              }
          });
      });

    for (const Check * c : {&paint, &paint_beyond, &find, &find_beyond, &paint_sum, &find_sum,
                            &exact, &exact_beyond})
      c->expect();
  }
}

namespace
{
  /** Runs Yen and Eppstein, with weights near the limits of int, from
      every node to every other, for k of 1, 2 and 3: they give the first
      k costs when they fit, and overflow_error when one of them does not.
      Eppstein also throws when the distance to the end of a node that
      reaches it does not fit, since its index of the suffixes holds it.
      Both took a path of cost exactly max() for none; Yen threw when the
      search from a spur node reached the end only beyond max(); and
      Eppstein threw on a heavy parallel arc, and when a candidate that was
      not asked, or that came after a cheaper one, did not fit: a walk that
      goes on from the end by a loop near max() made a query of one walk
      throw. */
  template <class GT>
  void check_k_near_int_limits()
  {
    using T = int;
    using W = Weights<GT, T>;
    constexpr long long M = Int_Max;
    Check yen("yen_k_shortest_paths(), near the limits of int");
    Check yen_beyond("yen_k_shortest_paths(), near the limits of int, a path beyond max()");
    Check epp("eppstein_k_shortest_paths(), near the limits of int");
    Check epp_beyond("eppstein_k_shortest_paths(), near the limits of int, a walk or a "
                     "distance to the end beyond max()");

    std::vector<T> w;
    const bool directed = GT().is_digraph();
    for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
      {
        for_each_weighting(shape, {1, M / 2, M - 1}, [&] (const Spec & s)
          {
            Built<GT> b(s);
            w = weights_as<T>(s);
            const Matrix d = distances(s, directed);
            for (int u = 0; u < s.n; ++u)
              for (int v = 0; v < s.n; ++v)
                {
                  if (u == v)
                    continue;
                  auto * nu = b.nodes[size_t(u)];
                  auto * nv = b.nodes[size_t(v)];
                  bool index_beyond = false;
                  for (int x = 0; x < s.n; ++x)
                    if (d[size_t(x)][size_t(v)] != Inf and d[size_t(x)][size_t(v)] > M)
                      index_beyond = true;

                  const auto simple = simple_path_costs(s, u, v, directed);
                  for (size_t k : {size_t(1), size_t(2), size_t(3)})
                    for (bool by_yen : {true, false})
                      {
                        std::vector<long long> want;
                        if (by_yen)
                          want.assign(simple.begin(),
                                      simple.begin() + long(std::min(k, simple.size())));
                        else
                          want = walk_costs(s, u, v, directed, k);
                        const bool beyond = (not by_yen and index_beyond)
                          or std::any_of(want.begin(), want.end(), [] (long long c)
                                         { return c > Int_Max; });
                        Check & c = by_yen ? (beyond ? yen_beyond : yen)
                          : (beyond ? epp_beyond : epp);
                        c.run();
                        const std::string at = route(u, v) + "k = " + std::to_string(k) + ": ";
                        DynList<K_Shortest_Path_Item<GT, T>> items;
                        const auto e = what_throws([&]
                          {
                            items = by_yen ? yen_k_shortest_paths<GT, W>(b.g, nu, nv, k, W{&w})
                              : eppstein_k_shortest_paths<GT, W>(b.g, nu, nv, k, W{&w});
                          });
                        if (beyond)
                          {
                            if (not is(e, "overflow_error"))
                              c.fail(s, at + "gives no overflow_error " + e);
                          }
                        else if (not e.empty())
                          c.fail(s, at + e);
                        else if (auto ie = items_error<T>(b.g, s, items, u, v, 0, want, by_yen);
                                 not ie.empty())
                          c.fail(s, at + ie);
                      }
                }
          });
      });

    for (const Check * c : {&yen, &yen_beyond, &epp, &epp_beyond})
      c->expect();
  }
}

TYPED_TEST(ExhaustivePaths, KShortestPathsNearTheLimitsOfInt)
{
  check_k_near_int_limits<TypeParam>();
}

TYPED_TEST(ExhaustivePaths, DijkstraNearTheLimitsOfInt)
{
  check_near_int_limits<TypeParam>(false);
}

TYPED_TEST(ExhaustivePaths, AStarNearTheLimitsOfInt)
{
  check_near_int_limits<TypeParam>(true);
}

TYPED_TEST(ExhaustivePaths, DijkstraAndAStarWithAFibonacciHeap)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Painting_Checks paint("Dijkstra_Min_Paths<ArcFibonacciHeap>");
  Check find("Dijkstra_Min_Paths<ArcFibonacciHeap>::find_min_path()");
  Check astar("AStar_Min_Path<ArcFibonacciHeap>::find_path()");
  Check astar_self("AStar_Min_Path<ArcFibonacciHeap>::find_path(), from a node to itself");

  std::vector<T> w;
  Dijkstra_Of<GT, T, Dft_Show_Arc<GT>, ArcFibonacciHeap> dij(Weights<GT, T>{&w});
  AStar_Of<GT, T, Dft_Show_Arc<GT>, ArcFibonacciHeap> ast(Weights<GT, T>{&w});

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : All_Weightings)
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          check_painting<T>(paint, dij, b, s, d, 0);
          check_find<T>(find, nullptr, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
            { return dij.find_min_path(b.g, x, y, p); });
          check_find<T>(astar, &astar_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
            { return ast.find_path(b.g, x, y, p); });
        }
    });

  paint.expect();
  find.expect();
  astar.expect();
  astar_self.expect();
}

namespace
{
  /// Runs the painting and find_min_path() of Zero_One_BFS with distances
  /// of type T, every weighting of 0 and 1, on the graphs of up to 3 nodes
  /// and 3 arcs
  template <typename T, class GT>
  void check_zero_one_with(const std::string & type)
  {
    using Node = typename GT::Node;
    Painting_Checks paint("Zero_One_BFS", ", every weighting, " + type);
    Check find("Zero_One_BFS::find_min_path(), every weighting, " + type);
    Check find_self("Zero_One_BFS::find_min_path(), every weighting, " + type
                    + ", from a node to itself");
    std::vector<T> w;

    for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
      {
        for_each_weighting(shape, {0, 1}, [&] (const Spec & s)
          {
            Built<GT> b(s);
            w = weights_as<T>(s, true);
            const Matrix d = distances(s, b.g.is_digraph());
            Zero_One_BFS<GT, Weights<GT, T>> bfs(Weights<GT, T>{&w});
            check_painting<T>(paint, bfs, b, s, d, 0, true);
            check_find<T>(find, &find_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
              { return bfs.find_min_path(b.g, x, y, p); }, true);
          });
      });

    paint.expect();
    find.expect();
    find_self.expect();
  }
}

TYPED_TEST(ExhaustivePaths, ZeroOneBFSMatchesTheOracle)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Painting_Checks paint("Zero_One_BFS");
  Check find("Zero_One_BFS::find_min_path()");
  Check find_self("Zero_One_BFS::find_min_path(), from a node to itself");
  Check op("Zero_One_BFS::operator()");
  Check op_self("Zero_One_BFS::operator(), from a node to itself");
  std::vector<T> w;

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (long long first : {0, 1})
        {
          const Spec s = alternating(shape, first);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          // Declared after the graph, which outlives it
          Zero_One_BFS<GT, Weights<GT, T>> bfs(Weights<GT, T>{&w});
          if (first == 0)
            {
              check_painting<T>(paint, bfs, b, s, d, 0);
              check_find<T>(find, &find_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
                { return bfs.find_min_path(b.g, x, y, p); });
            }
          else
            check_find<T>(op, &op_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
              { return bfs(b.g, x, y, p); });
        }
    });

  paint.expect();
  for (const Check * c : {&find, &find_self, &op, &op_self})
    c->expect();
}

TYPED_TEST(ExhaustivePaths, ZeroOneBFSTakesEveryWeighting)
{
  check_zero_one_with<int, TypeParam>("int");
  check_zero_one_with<double, TypeParam>("double");
}

TYPED_TEST(ExhaustivePaths, ZeroOneBFSRejectsOtherWeights)
{
  using GT = TypeParam;
  using T = long long;
  Check rejects("Zero_One_BFS::paint_min_paths_tree(), meeting a weight that is not 0 or 1");
  Check takes("Zero_One_BFS::paint_min_paths_tree(), meeting only weights 0 and 1");
  std::vector<T> w;
  const bool directed = GT().is_digraph();

  for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
    {
      for_each_weighting(shape, {0, 1, 2}, [&] (const Spec & s)
        {
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, directed);
          Zero_One_BFS<GT, Weights<GT, T>> bfs(Weights<GT, T>{&w});
          for (int u = 0; u < s.n; ++u)
            {
              const bool meets = meets_other_weight(s, u, directed);
              Check & c = meets ? rejects : takes;
              c.run();
              const std::string from = "from " + std::to_string(u) + ": ";
              auto * src = b.nodes[size_t(u)];
              const auto e = what_throws([&] { bfs.paint_min_paths_tree(b.g, src); });
              if (meets)
                {
                  if (not is(e, "domain_error") or bfs.is_painted())
                    c.fail(s, from + "gives no domain_error, or says painted");
                  continue;
                }
              if (not e.empty())
                {
                  c.fail(s, from + e);
                  continue;
                }
              for (int v = 0; v < s.n; ++v)
                {
                  if (d[size_t(u)][size_t(v)] == Inf)
                    continue;
                  T got{};
                  const auto ge = what_throws([&] { got = bfs.get_distance(b.nodes[size_t(v)]); });
                  if (not ge.empty() or got != d[size_t(u)][size_t(v)])
                    {
                      c.fail(s, route(u, v) + (ge.empty() ? "gives " + text_of(got) : ge));
                      break;
                    }
                }
            }
        });
    });

  rejects.expect();
  takes.expect();
}

TYPED_TEST(ExhaustivePaths, ZeroOneBFSRespectsTheFilter)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  using F = Hide_Mask<GT>;
  Painting_Checks paint("Zero_One_BFS", ", hiding arcs");
  Check find("Zero_One_BFS::find_min_path(), hiding arcs");
  Check find_self("Zero_One_BFS::find_min_path(), hiding arcs, from a node to itself");

  // The hidden arcs weigh 2, which the search would reject if it met them
  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec base = alternating(shape, 0);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          const Spec s = poisoned(base, hidden, 2);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph(), hidden);
          Zero_One_BFS<GT, Weights<GT, T>, Node_Arc_Iterator, F> bfs(Weights<GT, T>{&w}, F{hidden});
          check_painting<T>(paint, bfs, b, s, d, hidden);
          check_find<T>(find, &find_self, b, s, d, hidden, [&] (Node * x, Node * y, Path<GT> & p)
            { return bfs.find_min_path(b.g, x, y, p); });
        }
    });

  paint.expect();
  find.expect();
  find_self.expect();
}

TYPED_TEST(ExhaustivePaths, ZeroOneBFSLeavesOtherPaintingsAlone)
{
  using GT = TypeParam;
  using T = long long;
  Check others("~Zero_One_BFS, after another algorithm painted its graph");
  Check alone("~Zero_One_BFS, after another algorithm painted its graph, without a node reached");
  // A read of the freed graph shows under AddressSanitizer only
  Check dead("~Zero_One_BFS, after its graph was destroyed");
  std::vector<T> w;

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec s = alternating(shape, 1);
      Built<GT> b(s);
      w = weights_as<T>(s);
      const Matrix d = distances(s, b.g.is_digraph());
      Dijkstra_Of<GT, T> dij(Weights<GT, T>{&w});
      for (int u = 0; u < s.n; ++u)
        {
          const auto & row = d[size_t(u)];
          const bool reaches = std::count(row.begin(), row.end(), Inf) + 1 < long(row.size());
          Check & c = reaches ? others : alone;
          c.run();
          if (auto e = what_throws([&]
                {
                  Zero_One_BFS<GT, Weights<GT, T>> bfs(Weights<GT, T>{&w});
                  bfs.paint_min_paths_tree(b.g, b.nodes[size_t(u)]);
                  dij.paint_min_paths_tree(b.g, b.nodes[size_t(u)]);
                }); not e.empty())
            {
              c.fail(s, "from " + std::to_string(u) + ": " + e);
              continue;
            }
          for (int v = 0; v < s.n; ++v)
            {
              T got{};
              const auto e = what_throws([&] { got = dij.get_distance(b.nodes[size_t(v)]); });
              if (row[size_t(v)] == Inf ? not is(e, "domain_error")
                  : not e.empty() or got != row[size_t(v)])
                {
                  c.fail(s, route(u, v) + "the painting of Dijkstra_Min_Paths is lost: "
                         + (e.empty() ? text_of(got) : e));
                  break;
                }
            }
        }
      // The search outlives the graph that it painted: the graph, made
      // after it, is destroyed before it
      dead.run();
      if (auto e = what_throws([&]
            {
              Zero_One_BFS<GT, Weights<GT, T>> bfs(Weights<GT, T>{&w});
              Built<GT> gone(s);
              if (s.n > 0)
                bfs.paint_min_paths_tree(gone.g, gone.nodes[0]);
            }); not e.empty())
        dead.fail(s, e);
    });

  others.expect();
  alone.expect();
  dead.expect();
}

TYPED_TEST(ExhaustivePaths, ZeroOneBFSAndDialOnRandomGraphs)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Painting_Checks bfs_paint("Zero_One_BFS", ", random graphs");
  Check bfs_find("Zero_One_BFS::find_min_path(), random graphs");
  Painting_Checks dial_paint("Dial_Min_Paths", ", random graphs");
  Check dial_find("Dial_Min_Paths::find_min_path(), random graphs");
  std::vector<T> w;
  Dial_Min_Paths<GT, Weights<GT, T>> dial(Weights<GT, T>{&w});

  for (const Spec & s : random_specs(3305, GT().is_digraph(), 40, 0, 1))
    {
      Built<GT> b(s);
      w = weights_as<T>(s);
      const Matrix d = distances(s, b.g.is_digraph());
      Zero_One_BFS<GT, Weights<GT, T>> bfs(Weights<GT, T>{&w});
      check_painting<T>(bfs_paint, bfs, b, s, d, 0);
      check_find<T>(bfs_find, nullptr, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
        { return bfs.find_min_path(b.g, x, y, p); });
    }

  for (const Spec & s : random_specs(3306, GT().is_digraph(), 40, 0, 5))
    {
      Built<GT> b(s);
      w = weights_as<T>(s);
      const Matrix d = distances(s, b.g.is_digraph());
      check_painting<T>(dial_paint, dial, b, s, d, 0);
      check_find<T>(dial_find, nullptr, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
        { return dial.find_min_path(b.g, x, y, p); });
    }

  bfs_paint.expect();
  bfs_find.expect();
  dial_paint.expect();
  dial_find.expect();
}

namespace
{
  /// Runs the painting, if paint, or else find_min_path() of
  /// Dial_Min_Paths with distances of type T, every weighting of 0, 1 and
  /// 3, on the graphs of up to 3 nodes and 3 arcs
  template <typename T, class GT>
  void check_dial_with(const std::string & type, bool painting)
  {
    using Node = typename GT::Node;
    Painting_Checks paint("Dial_Min_Paths", ", every weighting, " + type);
    Check find("Dial_Min_Paths::find_min_path(), every weighting, " + type);
    Check find_self("Dial_Min_Paths::find_min_path(), every weighting, " + type
                    + ", from a node to itself");

    std::vector<T> w;
    Dial_Min_Paths<GT, Weights<GT, T>> dial(Weights<GT, T>{&w});

    for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
      {
        for_each_weighting(shape, {0, 1, 3}, [&] (const Spec & s)
          {
            Built<GT> b(s);
            w = weights_as<T>(s);
            const Matrix d = distances(s, b.g.is_digraph());
            if (painting)
              check_painting<T>(paint, dial, b, s, d, 0);
            else
              check_find<T>(find, &find_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
                { return dial.find_min_path(b.g, x, y, p); });
          });
      });

    paint.expect();
    find.expect();
    find_self.expect();
  }
}

TYPED_TEST(ExhaustivePaths, DialMatchesTheOracle)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  Painting_Checks paint("Dial_Min_Paths");
  Check find("Dial_Min_Paths::find_min_path()");
  Check find_self("Dial_Min_Paths::find_min_path(), from a node to itself");
  Check op("Dial_Min_Paths::operator()");
  Check op_self("Dial_Min_Paths::operator(), from a node to itself");

  std::vector<T> w;
  Dial_Min_Paths<GT, Weights<GT, T>> dial(Weights<GT, T>{&w});

  for_each_spec(domain(4, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : {Weighting::Ones, Weighting::Falling})
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          w = weights_as<T>(s);
          const Matrix d = distances(s, b.g.is_digraph());
          if (k == Weighting::Falling)
            {
              check_painting<T>(paint, dial, b, s, d, 0);
              check_find<T>(find, &find_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
                { return dial.find_min_path(b.g, x, y, p); });
            }
          else
            check_find<T>(op, &op_self, b, s, d, 0, [&] (Node * x, Node * y, Path<GT> & p)
              { return dial(b.g, x, y, p); });
        }
    });

  paint.expect();
  for (const Check * c : {&find, &find_self, &op, &op_self})
    c->expect();
}

TYPED_TEST(ExhaustivePaths, DialPaintsWithEveryWeighting)
{
  check_dial_with<int, TypeParam>("int", true);
}

TYPED_TEST(ExhaustivePaths, DialFindsWithEveryWeighting)
{
  check_dial_with<long long, TypeParam>("long long", false);
}

TYPED_TEST(ExhaustivePaths, DialRespectsTheFilter)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using T = long long;
  using F = Hide_Mask<GT>;
  Painting_Checks paint("Dial_Min_Paths", ", hiding arcs");
  Check find("Dial_Min_Paths::find_min_path(), hiding arcs");
  Check find_self("Dial_Min_Paths::find_min_path(), hiding arcs, from a node to itself");
  Painting_Checks negative("Dial_Min_Paths", ", hiding a negative arc", false, false);

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec base = weighted(shape, Weighting::Falling);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          Built<GT> b(base);
          const Matrix d = distances(base, b.g.is_digraph(), hidden);
          const std::vector<T> w = weights_as<T>(base);
          Dial_Min_Paths<GT, Weights<GT, T>, Node_Arc_Iterator, F> dial(Weights<GT, T>{&w},
                                                                        F{hidden});
          check_painting<T>(paint, dial, b, base, d, hidden);
          check_find<T>(find, &find_self, b, base, d, hidden, [&] (Node * x, Node * y, Path<GT> & p)
            { return dial.find_min_path(b.g, x, y, p); });

          const Spec s = poisoned(base, hidden, -1);
          const std::vector<T> wn = weights_as<T>(s);
          Dial_Min_Paths<GT, Weights<GT, T>, Node_Arc_Iterator, F>
            with_negative(Weights<GT, T>{&wn}, F{hidden});
          check_painting<T>(negative, with_negative, b, s, d, hidden);
        }
    });

  paint.expect();
  find.expect();
  find_self.expect();
  negative.expect();
}

namespace
{
  /// Builds a Floyd_All_Shortest_Paths of the graph of b with the weights
  /// w and the filter sa, and checks it with c; neg tells whether the
  /// arcs make a negative cycle, which the tests leave out
  template <typename T, class GT, class SA = Dft_Show_Arc<GT>>
  void check_floyd_of(Floyd_Checks & c, Built<GT> & b, const Spec & s, const Matrix & d,
                      unsigned long hidden, const std::vector<T> & w, SA sa = SA(),
                      bool full = true)
  {
    std::string e = what_throws([&]
      {
        const Floyd_All_Shortest_Paths<GT, Weights<GT, T>, SA> f(b.g, Weights<GT, T>{&w}, sa);
        check_floyd<T>(c, f, b, s, d, hidden, full);
      });
    if (not e.empty())
      {
        c.dist.run();
        c.dist.fail(s, "the solver " + e);
      }
  }

  /// Runs the checks of Floyd_All_Shortest_Paths with distances of type T
  /// on the graphs of up to 3 nodes and 3 arcs
  template <typename T, class GT>
  void check_floyd_with(const std::string & type)
  {
    Floyd_Checks c(", " + type);
    std::vector<T> w;
    const bool directed = GT().is_digraph();
    for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
      {
        if (shape.n == 0)
          return;
        for (const Spec & s : {weighted(shape, Weighting::Falling),
                               with_negatives(shape, directed)})
          {
            bool neg = false;
            const Matrix d = distances(s, directed, 0, neg);
            if (neg)
              continue;
            Built<GT> b(s);
            w = weights_as<T>(s);
            check_floyd_of<T>(c, b, s, d, 0, w);
          }
      });
    c.expect();
  }
}

TYPED_TEST(ExhaustivePaths, FloydMatchesTheOracle)
{
  using GT = TypeParam;
  using T = long long;
  Floyd_Checks c;
  Check empty("Floyd_All_Shortest_Paths, a graph without nodes");
  std::vector<T> w;
  const bool directed = GT().is_digraph();

  for_each_spec(domain(4, 3), directed, [&] (const Spec & shape)
    {
      if (shape.n == 0)
        {
          empty.run();
          Built<GT> b(shape);
          if (not is(what_throws([&]
                { Floyd_All_Shortest_Paths<GT, Weights<GT, T>> f(b.g, Weights<GT, T>{&w}); }),
                "invalid_argument"))
            empty.fail(shape, "gives no invalid_argument");
          return;
        }
      // A negative cycle is left out: it is a matter of the tests of
      // negative cycles
      const Spec s = with_negatives(shape, directed);
      bool neg = false;
      const Matrix d = distances(s, directed, 0, neg);
      if (neg)
        return;
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_floyd_of<T>(c, b, s, d, 0, w);
    });

  c.expect();
  empty.expect();
}

TYPED_TEST(ExhaustivePaths, FloydTakesEveryWeighting)
{
  using GT = TypeParam;
  using T = long long;
  Floyd_Checks c(", every weighting");
  std::vector<T> w;
  const bool directed = GT().is_digraph();
  // In a graph that is not directed a negative arc is a negative cycle
  const std::vector<long long> values = directed ? std::vector<long long>{-1, 0, 2}
    : std::vector<long long>{0, 1, 2};

  for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
    {
      if (shape.n > 0)
        for_each_weighting(shape, values, [&] (const Spec & s)
          {
            bool neg = false;
            const Matrix d = distances(s, directed, 0, neg);
            if (neg)
              return;
            Built<GT> b(s);
            w = weights_as<T>(s);
            check_floyd_of<T>(c, b, s, d, 0, w, Dft_Show_Arc<GT>(), false);
          });
    });

  c.expect();
}

TYPED_TEST(ExhaustivePaths, FloydTakesNegativeWeights)
{
  using GT = TypeParam;
  using T = long long;
  Floyd_Checks c(", from -2 to 2");
  std::vector<T> w;
  const bool directed = GT().is_digraph();

  // In a graph that is not directed, only the weightings without a
  // negative arc are left
  for_each_spec(domain(3, 2), directed, [&] (const Spec & shape)
    {
      if (shape.n > 0)
        for_each_weighting(shape, {-2, -1, 0, 1, 2}, [&] (const Spec & s)
          {
            bool neg = false;
            const Matrix d = distances(s, directed, 0, neg);
            if (neg)
              return;
            Built<GT> b(s);
            w = weights_as<T>(s);
            check_floyd_of<T>(c, b, s, d, 0, w);
          });
    });

  c.expect();
}

TYPED_TEST(ExhaustivePaths, FloydOnRandomGraphs)
{
  using GT = TypeParam;
  using T = long long;
  Floyd_Checks c(", random graphs");
  std::vector<T> w;
  const bool directed = GT().is_digraph();

  for (const Spec & s : directed ? random_potential_specs(3302, 60)
         : random_specs(3302, false, 60, 0, 9))
    {
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_floyd_of<T>(c, b, s, distances(s, directed), 0, w);
    }

  c.expect();
}

TYPED_TEST(ExhaustivePaths, FloydRespectsTheFilter)
{
  using GT = TypeParam;
  using T = long long;
  using F = Hide_Mask<GT>;
  Floyd_Checks c(", hiding arcs");
  Check lvalue("Floyd_All_Shortest_Paths(g, sa), hiding arcs");
  Check rvalue("Floyd_All_Shortest_Paths(g, SA(...)), hiding arcs");
  const bool directed = GT().is_digraph();

  // The hidden arcs weigh -5, a negative cycle if a solver took them
  for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
    {
      if (shape.n == 0)
        return;
      const Spec base = with_negatives(shape, directed);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          bool neg = false;
          const Matrix d = distances(base, directed, hidden, neg);
          if (neg)
            continue;
          const Spec s = poisoned(base, hidden, -5);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          check_floyd_of<T>(c, b, s, d, hidden, w, F{hidden}, false);

          // The constructors that build the distance with Distance()
          Static_Weight<GT, T>::weights = &w;
          for (Check * k : {&lvalue, &rvalue})
            {
              k->run();
              F sa{hidden};
              std::string wrong;
              const auto e = what_throws([&]
                {
                  using Floyd = Floyd_All_Shortest_Paths<GT, Static_Weight<GT, T>, F>;
                  const auto f = k == &lvalue ? Floyd(b.g, sa) : Floyd(b.g, F{hidden});
                  for (int u = 0; wrong.empty() and u < s.n; ++u)
                    for (int v = 0; wrong.empty() and v < s.n; ++v)
                      if (f.get_dist_mat()(f.index_node(b.nodes[size_t(u)]),
                                           f.index_node(b.nodes[size_t(v)]))
                          != floyd_value<T>(d[size_t(u)][size_t(v)]))
                        wrong = route(u, v) + "a wrong distance";
                });
              if (not e.empty() or not wrong.empty())
                k->fail(s, e + wrong);
            }
        }
    });

  c.expect();
  lvalue.expect();
  rvalue.expect();
}

TYPED_TEST(ExhaustivePaths, FloydNearTheLimitsOfInt)
{
  using GT = TypeParam;
  using T = int;
  constexpr long long M = Int_Max;
  // A sum above max() does not count, as documented, so that a distance
  // beyond max() is no path
  Floyd_Checks c(", near the limits of int");
  std::vector<T> w;
  const bool directed = GT().is_digraph();

  auto check = [&] (const Spec & s)
    {
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_floyd_of<T>(c, b, s, distances(s, directed), 0, w, Dft_Show_Arc<GT>(), false);
    };

  // The sums of two halves fit, and with a 1 reach max(), which means no
  // path; the others do not fit
  for_each_spec(domain(3, 3), directed, [&] (const Spec & shape)
    {
      if (shape.n > 0)
        for_each_weighting(shape, {M / 2, M - 1}, check);
    });
  for_each_spec(domain(3, 2), directed, [&] (const Spec & shape)
    {
      if (shape.n > 0)
        for_each_weighting(shape, {1, M / 2, M - 1}, check);
    });

  c.expect();
}

TYPED_TEST(ExhaustivePaths, FloydTakesIntAndDouble)
{
  check_floyd_with<int, TypeParam>("int");
  check_floyd_with<double, TypeParam>("double");
}

namespace
{
  /// Runs the checks of Johnson with distances of type T, and with the
  /// weights -1, 0, 1 and the falling powers of 2, on the digraphs of up
  /// to 3 nodes and 3 arcs without a negative cycle, with the checks that
  /// pick(s, d) gives
  template <typename T, class GT, class Pick>
  void check_johnson_with(Pick && pick)
  {
    std::vector<T> w;
    for_each_spec(domain(3, 3), true, [&] (const Spec & shape)
      {
        for (const Spec & s : {weighted(shape, Weighting::Falling), with_negatives(shape, true)})
          {
            bool neg = false;
            const Matrix d = distances(s, true, 0, neg);
            if (neg)
              continue;
            Built<GT> b(s);
            w = weights_as<T>(s);
            check_johnson<T>(pick(s, d), b, s, d, 0, w, Dft_Show_Arc<GT>());
          }
      });
  }

  /// The weights w + h(u) - h(v) of the arcs that hidden shows, with the
  /// potentials h of Johnson for the distances d
  std::vector<long long> reweighted(const Spec & s, const Matrix & d, unsigned long hidden)
  {
    const auto h = potentials(d);
    std::vector<long long> r;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (graph_filters::shown(hidden, int(i)))
        r.push_back(s.w[i] + h[size_t(s.arcs[i][0])] - h[size_t(s.arcs[i][1])]);
    return r;
  }

  /// Whether Johnson gives an arc that hidden shows a weight that is the
  /// number of a hidden arc
  bool reweighting_hides(const Spec & s, const Matrix & d, unsigned long hidden)
  {
    for (long long x : reweighted(s, d, hidden))
      if (not graph_filters::shown(hidden, int(x)))
        return true;
    return false;
  }

  /// Whether hidden hides one of two parallel arcs of s and shows the other
  bool hides_a_parallel_arc(const Spec & s, unsigned long hidden)
  {
    for (size_t i = 0; i < s.arcs.size(); ++i)
      for (size_t j = 0; j < s.arcs.size(); ++j)
        if (s.arcs[i] == s.arcs[j] and not graph_filters::shown(hidden, int(i))
            and graph_filters::shown(hidden, int(j)))
          return true;
    return false;
  }

  /// Whether Johnson gives an arc a weight with a fraction, as halves of
  /// the weights of s
  bool reweighting_halves(const Spec & s, const Matrix & d)
  {
    for (long long x : reweighted(s, d, 0))
      if (x % 2 != 0)
        return true;
    return false;
  }
}

template <class GT>
class ExhaustiveJohnson : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveJohnson, Directed_Kinds);

TYPED_TEST(ExhaustiveJohnson, JohnsonMatchesTheOracle)
{
  using GT = TypeParam;
  using T = long long;
  Johnson_Checks c;
  Check none("Johnson(), without a graph");
  std::vector<T> w;

  for_each_spec(domain(4, 3), true, [&] (const Spec & shape)
    {
      // A negative cycle is left out: it is a matter of the tests of
      // negative cycles
      for (const Spec & s : {weighted(shape, Weighting::Falling), with_negatives(shape, true)})
        {
          bool neg = false;
          const Matrix d = distances(s, true, 0, neg);
          if (neg)
            continue;
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_johnson<T>(c, b, s, d, 0, w, Dft_Show_Arc<GT>());
        }

      // Without a graph, the queries throw
      if (shape.n == 0)
        return;
      none.run();
      Built<GT> b(shape);
      Johnson<GT, Johnson_Weight<GT, T>> j;
      auto * p = b.nodes[0];
      if (j.is_initialized()
          or not is(what_throws([&] { (void) j.get_distance(p, p); }), "domain_error")
          or not is(what_throws([&] { (void) j.find_path(p, p); }), "domain_error")
          or not is(what_throws([&] { (void) j.compute_all_pairs_distances(); }), "domain_error")
          or not is(what_throws([&] { (void) j.get_potential(p); }), "domain_error"))
        none.fail(shape, "answers without a graph");
    });

  c.expect();
  none.expect();
}

TYPED_TEST(ExhaustiveJohnson, JohnsonTakesEveryWeighting)
{
  using GT = TypeParam;
  using T = long long;
  Johnson_Checks c(", every weighting");
  std::vector<T> w;

  for_each_spec(domain(3, 3), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, {-1, 0, 2}, [&] (const Spec & s)
        {
          bool neg = false;
          const Matrix d = distances(s, true, 0, neg);
          if (neg)
            return;
          Built<GT> b(s);
          w = weights_as<T>(s);
          check_johnson<T>(c, b, s, d, 0, w, Dft_Show_Arc<GT>());
        });
    });

  c.expect();
}

TYPED_TEST(ExhaustiveJohnson, JohnsonOnRandomGraphs)
{
  using GT = TypeParam;
  using T = long long;
  Johnson_Checks c(", random graphs");
  std::vector<T> w;

  for (const Spec & s : random_potential_specs(3303, 40))
    {
      Built<GT> b(s);
      w = weights_as<T>(s);
      check_johnson<T>(c, b, s, distances(s, true), 0, w, Dft_Show_Arc<GT>());
    }

  c.expect();
}

TYPED_TEST(ExhaustiveJohnson, JohnsonRespectsTheFilter)
{
  using GT = TypeParam;
  using T = long long;
  using F = Hide_Mask<GT>;
  Johnson_Checks c(", hiding arcs");
  // Two classes of cases of their own, which the harness found: an arc
  // whose new weight w + h(u) - h(v) is the number of a hidden one, which
  // the filter hid when it read the infos of the copies of the arcs, where
  // those weights are; and one of two parallel arcs hidden, whose copies
  // were paired with the arcs by their ends, so that in List_SDigraph,
  // which gives its arcs in the order of their addresses, the shown one
  // could take the weight of the hidden one, and the path the hidden arc
  Johnson_Checks hides(", hiding arcs, a new weight that the filter hides");
  Johnson_Checks paired(", hiding one of two parallel arcs");

  // The hidden arcs weigh -5, a negative cycle if the solver took them
  for_each_spec(domain(3, 3), true, [&] (const Spec & shape)
    {
      const Spec base = with_negatives(shape, true);
      for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
        {
          bool neg = false;
          const Matrix d = distances(base, true, hidden, neg);
          if (neg)
            continue;
          const Spec s = poisoned(base, hidden, -5);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          Johnson_Checks & which = reweighting_hides(base, d, hidden) ? hides
            : hides_a_parallel_arc(base, hidden) ? paired : c;
          check_johnson<T>(which, b, s, d, hidden, w, F{hidden});
        }
    });

  c.expect();
  hides.expect();
  paired.expect();
}

TYPED_TEST(ExhaustiveJohnson, JohnsonNearTheLimitsOfInt)
{
  using GT = TypeParam;
  using T = int;
  constexpr long long M = Int_Max;
  Check fits("Johnson, near the limits of int");
  // Two classes of cases of their own, which the harness found: a new
  // weight w + h(u) - h(v), or a sum of Dijkstra on those weights, beyond
  // max() although every distance fits, which threw overflow_error while
  // they were kept in int; and a distance beyond max(), which made every
  // query from its source throw while the whole tree was painted in int
  Check too_big("Johnson, near the limits of int, a reweighted value beyond max()");
  Check beyond("Johnson, near the limits of int, a distance beyond max()");
  std::vector<T> w;

  for_each_spec(domain(3, 3), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, {-2, 1, M - 1}, [&] (const Spec & s)
        {
          bool neg = false;
          const Matrix d = distances(s, true, 0, neg);
          if (neg)
            return;
          const auto h = potentials(d);
          bool all_fit = true;
          for (const auto & row : d)
            for (long long x : row)
              all_fit = all_fit and (x == Inf or x <= M);
          bool over = false;
          for (size_t i = 0; i < s.arcs.size() and not over; ++i)
            {
              const size_t x = size_t(s.arcs[i][0]), y = size_t(s.arcs[i][1]);
              const long long wp = s.w[i] + h[x] - h[y];
              over = wp > M;
              for (int src = 0; src < s.n and not over; ++src)
                over = x != y and d[size_t(src)][x] != Inf
                  and d[size_t(src)][x] + h[size_t(src)] - h[x] + wp > M;
            }
          Check & c = not all_fit ? beyond : over ? too_big : fits;
          c.run();

          Built<GT> b(s);
          w = weights_as<T>(s);
          using Solver = Johnson<GT, Johnson_Weight<GT, T>>;
          std::unique_ptr<Solver> j;
          if (auto e = what_throws([&]
                { j = std::make_unique<Solver>(b.g, Johnson_Weight<GT, T>{&w}); });
              not e.empty())
            {
              c.fail(s, "the constructor " + e);
              return;
            }
          for (int u = 0; u < s.n; ++u)
            for (int v = 0; v < s.n; ++v)
              {
                const long long want = d[size_t(u)][size_t(v)];
                T got{};
                const auto e = what_throws([&]
                  { got = j->get_distance(b.nodes[size_t(u)], b.nodes[size_t(v)]); });
                const bool ok = want != Inf and want > M ? is(e, "overflow_error")
                  : e.empty() and got == (want == Inf ? std::numeric_limits<T>::max() : T(want));
                if (not ok)
                  {
                    c.fail(s, route(u, v) + (e.empty() ? "gives " + text_of(got) : e));
                    return;
                  }
              }
        });
    });

  fits.expect();
  beyond.expect();
  too_big.expect();
}

TYPED_TEST(ExhaustiveJohnson, JohnsonTakesIntAndDouble)
{
  Johnson_Checks with_int(", int");
  check_johnson_with<int, TypeParam>([&] (const Spec &, const Matrix &) -> Johnson_Checks &
    { return with_int; });
  with_int.expect();

  Johnson_Checks with_double(", double");
  // A class of its own, which the harness found: a new weight w + h(u) -
  // h(v) with a fraction, which the answers once lost; the infos of the
  // copy, int here, keep its integer part, as get_reweighted_graph() says
  Johnson_Checks halves(", double, a new weight with a fraction");
  check_johnson_with<double, TypeParam>([&] (const Spec & s, const Matrix & d) -> Johnson_Checks &
    { return reweighting_halves(s, d) ? halves : with_double; });
  with_double.expect();
  halves.expect();
}

TYPED_TEST(ExhaustivePaths, YenGivesTheSimplePathsInCostOrder)
{
  using GT = TypeParam;
  using T = long long;
  K_Paths_Checks c;

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : {Weighting::Ones, Weighting::Falling})
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          check_k_shortest<T>(c, b, s, 0, w, Dft_Show_Arc<GT>(), true, false);
        }
    });

  c.expect();
}

TYPED_TEST(ExhaustivePaths, EppsteinGivesTheWalksInCostOrder)
{
  using GT = TypeParam;
  using T = long long;
  K_Paths_Checks c;

  for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
    {
      for (Weighting k : {Weighting::Ones, Weighting::Falling})
        {
          const Spec s = weighted(shape, k);
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          check_k_shortest<T>(c, b, s, 0, w, Dft_Show_Arc<GT>(), false, true,
                              k == Weighting::Ones);
        }
    });

  c.expect();
}

TYPED_TEST(ExhaustivePaths, KShortestPathsTakeEveryWeighting)
{
  using GT = TypeParam;
  using T = long long;
  K_Paths_Checks c(", every weighting");

  for_each_spec(domain(3, 2), GT().is_digraph(), [&] (const Spec & shape)
    {
      for_each_weighting(shape, {0, 1, 2}, [&] (const Spec & s)
        {
          Built<GT> b(s);
          const std::vector<T> w = weights_as<T>(s);
          check_k_shortest<T>(c, b, s, 0, w, Dft_Show_Arc<GT>(), true, true, true);
        });
    });

  c.expect();
}

namespace
{
  /// Runs Yen, or else Eppstein, with every set of arcs hidden, on the
  /// graphs of up to 3 nodes and 3 arcs; the hidden arcs weigh -1, which
  /// both functions reject if they read it
  template <class GT>
  void check_k_shortest_hiding(bool yen)
  {
    using T = long long;
    using F = Hide_Mask<GT>;
    K_Paths_Checks c(", hiding arcs");
    for_each_spec(domain(3, 3), GT().is_digraph(), [&] (const Spec & shape)
      {
        const Spec base = weighted(shape, Weighting::Falling);
        for (unsigned long hidden = 1; hidden < (1UL << shape.num_arcs()); ++hidden)
          {
            const Spec s = poisoned(base, hidden, -1);
            Built<GT> b(s);
            const std::vector<T> w = weights_as<T>(s);
            check_k_shortest<T>(c, b, s, hidden, w, F{hidden}, yen, not yen, true);
          }
      });
    c.expect();
  }
}

TYPED_TEST(ExhaustivePaths, YenRespectsTheFilter)
{
  check_k_shortest_hiding<TypeParam>(true);
}

TYPED_TEST(ExhaustivePaths, EppsteinRespectsTheFilter)
{
  check_k_shortest_hiding<TypeParam>(false);
}

TYPED_TEST(ExhaustivePaths, KShortestPathsOnRandomGraphs)
{
  using GT = TypeParam;
  using T = long long;
  // Without the cases of Eppstein that fail by now, which the domains above
  // cover
  K_Paths_Checks c(", random graphs");

  for (const Spec & s : random_specs(3304, GT().is_digraph(), 6, 0, 9))
    {
      Built<GT> b(s);
      const std::vector<T> w = weights_as<T>(s);
      check_k_shortest<T>(c, b, s, 0, w, Dft_Show_Arc<GT>());
    }

  c.expect();
}

TYPED_TEST(ExhaustivePaths, KShortestPathsTakeIntAndDouble)
{
  using GT = TypeParam;
  K_Paths_Checks with_int(", int");
  K_Paths_Checks with_double(", double");

  for_each_spec(domain(3, 2), GT().is_digraph(), [&] (const Spec & shape)
    {
      const Spec s = weighted(shape, Weighting::Falling);
      Built<GT> b(s);
      const std::vector<int> wi = weights_as<int>(s);
      check_k_shortest<int>(with_int, b, s, 0, wi, Dft_Show_Arc<GT>());
      const std::vector<double> wd = weights_as<double>(s);
      check_k_shortest<double>(with_double, b, s, 0, wd, Dft_Show_Arc<GT>());
    });

  with_int.expect();
  with_double.expect();
}
