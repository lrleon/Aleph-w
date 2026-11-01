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
 * @file graph_oracles.H
 * @brief Answers computed from a Spec of graph_harness.H, without Aleph.
 *
 * Each oracle works on the list of arcs of a Spec with the standard
 * library only, by brute force or by the simplest algorithm that is
 * plainly right, so that it shares no code with what it checks. They are
 * meant for the small graphs of the harness: several of them are
 * exponential.
 *
 * In a multigraph that is not directed, a loop is a cycle, two parallel
 * arcs make a cycle, and a loop adds 2 to the degree of its node.
 */

#ifndef ALEPH_TESTS_GRAPH_ORACLES_H
#define ALEPH_TESTS_GRAPH_ORACLES_H

#include <algorithm>
#include <limits>
#include <numeric>
#include <set>
#include <vector>

#include "graph_harness.H"

namespace graph_oracles
{
  using graph_harness::Spec;

  /// Union-find over the nodes 0 .. n - 1
  struct Union_Find
  {
    std::vector<int> parent;

    explicit Union_Find(int n) : parent(size_t(n))
    {
      std::iota(parent.begin(), parent.end(), 0);
    }

    int find(int x)
    {
      while (parent[size_t(x)] != x)
        x = parent[size_t(x)] = parent[size_t(parent[size_t(x)])];
      return x;
    }

    /// Joins the sets of a and b; false if they were the same set
    bool join(int a, int b)
    {
      a = find(a);
      b = find(b);
      if (a == b)
        return false;
      parent[size_t(a)] = b;
      return true;
    }
  };

  /// The degree of x: a loop counts 2
  inline int degree(const Spec & s, int x)
  {
    int d = 0;
    for (const auto & a : s.arcs)
      d += (a[0] == x) + (a[1] == x);
    return d;
  }

  /// The number of the component of each node, as that of its root, of
  /// the graph without the node skip_node and the arc skip_arc (-1: none)
  inline std::vector<int> component_of(const Spec & s, int skip_node = -1,
                                       int skip_arc = -1)
  {
    Union_Find uf(s.n);
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        const int u = s.arcs[i][0], v = s.arcs[i][1];
        if (int(i) == skip_arc or u == skip_node or v == skip_node)
          continue;
        uf.join(u, v);
      }
    std::vector<int> comp(size_t(s.n));
    for (int x = 0; x < s.n; ++x)
      comp[size_t(x)] = uf.find(x);
    return comp;
  }

  /// The number of components, without the node skip_node and the arc
  /// skip_arc
  inline int components(const Spec & s, int skip_node = -1, int skip_arc = -1)
  {
    const auto comp = component_of(s, skip_node, skip_arc);
    std::set<int> roots;
    for (int x = 0; x < s.n; ++x)
      if (x != skip_node)
        roots.insert(comp[size_t(x)]);
    return int(roots.size());
  }

  /// The nodes whose removal splits their component
  inline std::set<int> cut_nodes(const Spec & s)
  {
    const int c = components(s);
    std::set<int> r;
    for (int x = 0; x < s.n; ++x)
      if (components(s, x) > c)
        r.insert(x);
    return r;
  }

  /// The cut nodes of the component of start
  inline std::set<int> cut_nodes(const Spec & s, int start)
  {
    const auto comp = component_of(s);
    std::set<int> r;
    for (int x : cut_nodes(s))
      if (comp[size_t(x)] == comp[size_t(start)])
        r.insert(x);
    return r;
  }

  /// The arcs, by number, whose removal splits their component
  inline std::set<int> bridges(const Spec & s)
  {
    const int c = components(s);
    std::set<int> r;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (components(s, -1, int(i)) > c)
        r.insert(int(i));
    return r;
  }

  /// Whether the multigraph, not directed, has a cycle
  inline bool has_cycle(const Spec & s)
  {
    Union_Find uf(s.n);
    for (const auto & a : s.arcs)
      if (not uf.join(a[0], a[1]))
        return true;
    return false;
  }

  /// Whether the node x lies on a cycle of the multigraph, not directed
  inline bool on_cycle(const Spec & s, int x)
  {
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        const int u = s.arcs[i][0], v = s.arcs[i][1];
        if (u != x and v != x)
          continue;
        if (u == v)
          return true;
        const auto comp = component_of(s, -1, int(i));
        if (comp[size_t(u)] == comp[size_t(v)])
          return true;
      }
    return false;
  }

  /// Whether the multigraph, not directed, has two colors with no arc
  /// inside one (a loop is an odd cycle)
  inline bool is_bipartite(const Spec & s)
  {
    std::vector<int> color(size_t(s.n), -1);
    for (int root = 0; root < s.n; ++root)
      {
        if (color[size_t(root)] != -1)
          continue;
        color[size_t(root)] = 0;
        std::vector<int> stack = {root};
        while (not stack.empty())
          {
            const int x = stack.back();
            stack.pop_back();
            for (const auto & a : s.arcs)
              for (int k = 0; k < 2; ++k)
                if (a[size_t(k)] == x)
                  {
                    const int y = a[size_t(1 - k)];
                    if (color[size_t(y)] == -1)
                      {
                        color[size_t(y)] = 1 - color[size_t(x)];
                        stack.push_back(y);
                      }
                    else if (color[size_t(y)] == color[size_t(x)])
                      return false;
                  }
          }
      }
    return true;
  }

  /// The size of a maximum matching, by brute force over the subsets of
  /// the arcs; loops never match
  inline int max_matching(const Spec & s)
  {
    const size_t m = s.arcs.size();
    int best = 0;
    for (unsigned long sub = 0; sub < (1UL << m); ++sub)
      {
        std::vector<bool> used(size_t(s.n), false);
        int size = 0;
        bool ok = true;
        for (size_t i = 0; i < m and ok; ++i)
          if (sub & (1UL << i))
            {
              const int u = s.arcs[i][0], v = s.arcs[i][1];
              if (u == v or used[size_t(u)] or used[size_t(v)])
                ok = false;
              else
                {
                  used[size_t(u)] = used[size_t(v)] = true;
                  ++size;
                }
            }
        if (ok)
          best = std::max(best, size);
      }
    return best;
  }

  /// Whether the arcs, by number, form a matching of s
  inline bool is_matching(const Spec & s, const std::vector<int> & arcs)
  {
    std::vector<bool> used(size_t(s.n), false);
    for (int i : arcs)
      {
        const int u = s.arcs[size_t(i)][0], v = s.arcs[size_t(i)][1];
        if (u == v or used[size_t(u)] or used[size_t(v)])
          return false;
        used[size_t(u)] = used[size_t(v)] = true;
      }
    return true;
  }

  /// The weight of a minimum spanning forest: Kruskal on the list of arcs
  inline long long min_spanning_forest_weight(const Spec & s)
  {
    std::vector<size_t> order(s.arcs.size());
    std::iota(order.begin(), order.end(), size_t(0));
    std::stable_sort(order.begin(), order.end(),
                     [&] (size_t a, size_t b) { return s.w[a] < s.w[b]; });
    Union_Find uf(s.n);
    long long total = 0;
    for (size_t i : order)
      if (uf.join(s.arcs[i][0], s.arcs[i][1]))
        total += s.w[i];
    return total;
  }

  /// reach[u][v]: v is reached from u by a directed path of one arc or
  /// more (in a graph that is not directed, give directed = false)
  inline std::vector<std::vector<bool>> closure(const Spec & s, bool directed)
  {
    std::vector<std::vector<bool>> r(size_t(s.n), std::vector<bool>(size_t(s.n), false));
    for (const auto & a : s.arcs)
      {
        r[size_t(a[0])][size_t(a[1])] = true;
        if (not directed)
          r[size_t(a[1])][size_t(a[0])] = true;
      }
    for (int k = 0; k < s.n; ++k)
      for (int i = 0; i < s.n; ++i)
        if (r[size_t(i)][size_t(k)])
          for (int j = 0; j < s.n; ++j)
            if (r[size_t(k)][size_t(j)])
              r[size_t(i)][size_t(j)] = true;
    return r;
  }

  /// Whether there is a path, of zero arcs or more, from u to v
  inline bool reaches(const std::vector<std::vector<bool>> & r, int u, int v)
  {
    return u == v or r[size_t(u)][size_t(v)];
  }

  /// The strongly connected components, as the smallest node of the
  /// component of each node
  inline std::vector<int> scc_partition(const Spec & s)
  {
    const auto r = closure(s, true);
    std::vector<int> part(size_t(s.n));
    for (int u = 0; u < s.n; ++u)
      {
        part[size_t(u)] = u;
        for (int v = 0; v < u; ++v)
          if (r[size_t(u)][size_t(v)] and r[size_t(v)][size_t(u)])
            {
              part[size_t(u)] = part[size_t(v)];
              break;
            }
      }
    return part;
  }

  /// Whether the digraph has no directed cycle (a loop is one)
  inline bool is_dag(const Spec & s)
  {
    const auto r = closure(s, true);
    for (int u = 0; u < s.n; ++u)
      if (r[size_t(u)][size_t(u)])
        return false;
    return true;
  }

  /// Whether order, a sequence of all the nodes, is a topological order
  inline bool is_topological_order(const Spec & s, const std::vector<int> & order)
  {
    if (int(order.size()) != s.n)
      return false;
    std::vector<int> pos(size_t(s.n), -1);
    for (size_t i = 0; i < order.size(); ++i)
      {
        if (order[i] < 0 or order[i] >= s.n or pos[size_t(order[i])] != -1)
          return false;
        pos[size_t(order[i])] = int(i);
      }
    for (const auto & a : s.arcs)
      if (pos[size_t(a[0])] >= pos[size_t(a[1])])
        return false;
    return true;
  }

  /// hops[u][v]: the least number of arcs of a path from u to v, 0 from u
  /// to itself and -1 if there is no path
  inline std::vector<std::vector<int>> hops(const Spec & s, bool directed)
  {
    std::vector<std::vector<int>> h(size_t(s.n), std::vector<int>(size_t(s.n), -1));
    for (int u = 0; u < s.n; ++u)
      {
        auto & d = h[size_t(u)];
        d[size_t(u)] = 0;
        std::vector<int> queue = {u};
        for (size_t k = 0; k < queue.size(); ++k)
          for (const auto & a : s.arcs)
            for (size_t e = 0; e < (directed ? 1U : 2U); ++e)
              if (a[e] == queue[k] and d[size_t(a[1 - e])] == -1)
                {
                  d[size_t(a[1 - e])] = d[size_t(queue[k])] + 1;
                  queue.push_back(a[1 - e]);
                }
      }
    return h;
  }

  /// The spec with each arc reversed; the arcs keep their numbers
  inline Spec reversed(const Spec & s)
  {
    Spec r = s;
    for (auto & a : r.arcs)
      std::swap(a[0], a[1]);
    return r;
  }

  /// The spec without the arcs whose numbers are set in hidden, with
  /// orig[i] the number in s of its i-th arc
  inline Spec without(const Spec & s, unsigned long hidden, std::vector<int> & orig)
  {
    Spec r;
    r.n = s.n;
    orig.clear();
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (((hidden >> i) & 1UL) == 0)
        {
          r.arcs.push_back(s.arcs[i]);
          if (i < s.w.size())
            r.w.push_back(s.w[i]);
          orig.push_back(int(i));
        }
    return r;
  }

  /// The rank that Kahn's algorithm gives each node of a digraph: 0 for a
  /// node without arcs in, else 1 + the largest rank of its predecessors.
  /// The nodes that a cycle reaches never come out, and have -1.
  inline std::vector<int> kahn_ranks(const Spec & s)
  {
    const auto r = closure(s, true);
    std::vector<int> rank(size_t(s.n), 0);
    for (int u = 0; u < s.n; ++u)
      if (r[size_t(u)][size_t(u)])
        for (int v = 0; v < s.n; ++v)
          if (reaches(r, u, v))
            rank[size_t(v)] = -1;
    // The predecessors of a node that comes out also come out, so that n
    // rounds give the longest paths
    for (int round = 0; round < s.n; ++round)
      for (const auto & a : s.arcs)
        if (rank[size_t(a[0])] != -1 and rank[size_t(a[1])] != -1)
          rank[size_t(a[1])] = std::max(rank[size_t(a[1])], rank[size_t(a[0])] + 1);
    return rank;
  }

  /// dom[d][v]: d dominates v from root in a digraph, that is, v is
  /// reached from root and every path from root to v goes through d. A
  /// node that is reached dominates itself.
  inline std::vector<std::vector<bool>> dominators(const Spec & s, int root)
  {
    // The nodes reached from root without going through skip
    auto reached = [&] (int skip)
      {
        std::vector<bool> seen(size_t(s.n), false);
        if (root == skip)
          return seen;
        seen[size_t(root)] = true;
        for (bool grew = true; grew; )
          {
            grew = false;
            for (const auto & a : s.arcs)
              if (seen[size_t(a[0])] and a[1] != skip and not seen[size_t(a[1])])
                {
                  seen[size_t(a[1])] = true;
                  grew = true;
                }
          }
        return seen;
      };

    const auto all = reached(-1);
    std::vector<std::vector<bool>> dom(size_t(s.n), std::vector<bool>(size_t(s.n), false));
    for (int d = 0; d < s.n; ++d)
      {
        if (not all[size_t(d)])
          continue;
        const auto rest = reached(d);
        for (int v = 0; v < s.n; ++v)
          dom[size_t(d)][size_t(v)] = all[size_t(v)] and (d == v or not rest[size_t(v)]);
      }
    return dom;
  }

  /// The immediate dominator of each node from root: the strict dominator
  /// that all the others dominate; -1 for root and the nodes not reached
  inline std::vector<int> immediate_dominators(const Spec & s, int root)
  {
    const auto dom = dominators(s, root);
    std::vector<int> idom(size_t(s.n), -1);
    for (int v = 0; v < s.n; ++v)
      for (int d = 0; d < s.n; ++d)
        {
          if (d == v or not dom[size_t(d)][size_t(v)])
            continue;
          bool deepest = true;
          for (int e = 0; e < s.n and deepest; ++e)
            if (e != v and dom[size_t(e)][size_t(v)])
              deepest = dom[size_t(e)][size_t(d)];
          if (deepest)
            idom[size_t(v)] = d;
        }
    return idom;
  }

  /// The dominance frontier of each node from root: y is in that of x if
  /// x dominates a predecessor of y and does not strictly dominate y
  inline std::vector<std::set<int>> dominance_frontiers(const Spec & s, int root)
  {
    const auto dom = dominators(s, root);
    std::vector<std::set<int>> df(size_t(s.n));
    for (int x = 0; x < s.n; ++x)
      for (const auto & a : s.arcs)
        if (dom[size_t(x)][size_t(a[0])]
            and (x == a[1] or not dom[size_t(x)][size_t(a[1])]))
          df[size_t(x)].insert(a[1]);
    return df;
  }

  /// Whether arcs, by number, walk every arc of s once, each from the node
  /// where the one before ended (from either end if not directed), back to
  /// where they began if closed
  inline bool is_euler_walk(const Spec & s, const std::vector<int> & arcs,
                            bool directed, bool closed)
  {
    if (arcs.size() != s.arcs.size())
      return false;
    std::vector<bool> used(s.arcs.size(), false);
    for (int i : arcs)
      {
        if (i < 0 or size_t(i) >= s.arcs.size() or used[size_t(i)])
          return false;
        used[size_t(i)] = true;
      }
    if (arcs.empty())
      return true;
    for (int start : {s.arcs[size_t(arcs[0])][0], s.arcs[size_t(arcs[0])][1]})
      {
        int at = start;
        bool ok = true;
        for (int i : arcs)
          {
            const int u = s.arcs[size_t(i)][0], v = s.arcs[size_t(i)][1];
            if (u == at)
              at = v;
            else if (not directed and v == at)
              at = u;
            else
              {
                ok = false;
                break;
              }
          }
        if (ok and (not closed or at == start))
          return true;
        if (directed)
          break;
      }
    return false;
  }

  /// What kind of Eulerian walk the multigraph has
  enum class Euler { None, Path, Cycle };

  /// The Eulerian class of s: the arcs must be in one component (weakly,
  /// in a digraph), and the degrees even, or balanced, everywhere but at
  /// the two ends of a path. Without arcs, the empty walk is a cycle.
  inline Euler euler_class(const Spec & s, bool directed)
  {
    if (s.arcs.empty())
      return Euler::Cycle;
    Union_Find uf(s.n);
    for (const auto & a : s.arcs)
      uf.join(a[0], a[1]);
    std::set<int> roots;
    for (const auto & a : s.arcs)
      roots.insert(uf.find(a[0]));
    if (roots.size() > 1)
      return Euler::None;

    if (not directed)
      {
        int odd = 0;
        for (int x = 0; x < s.n; ++x)
          odd += degree(s, x) % 2;
        return odd == 0 ? Euler::Cycle : odd == 2 ? Euler::Path : Euler::None;
      }

    int plus = 0, minus = 0;
    for (int x = 0; x < s.n; ++x)
      {
        int out = 0, in = 0;
        for (const auto & a : s.arcs)
          {
            out += a[0] == x;
            in += a[1] == x;
          }
        if (out == in)
          continue;
        if (out == in + 1)
          ++plus;
        else if (in == out + 1)
          ++minus;
        else
          return Euler::None;
      }
    if (plus == 0 and minus == 0)
      return Euler::Cycle;
    return plus == 1 and minus == 1 ? Euler::Path : Euler::None;
  }

  /// No distance, in the matrices of the oracles
  constexpr long long Inf = std::numeric_limits<long long>::max();

  /// a + b, kept within the range of long long
  inline long long saturated_add(long long a, long long b)
  {
    if (b > 0 and a > std::numeric_limits<long long>::max() - b)
      return std::numeric_limits<long long>::max();
    if (b < 0 and a < std::numeric_limits<long long>::lowest() - b)
      return std::numeric_limits<long long>::lowest();
    return a + b;
  }

  /// Floyd on a matrix of long long: the shortest distances, Inf where
  /// there is no path, and neg set if there is a negative cycle; in a
  /// graph that is not directed each arc goes both ways. The sums are
  /// saturated, so weights up to 2^40 in absolute value are exact on the
  /// graphs of the harness, negative cycles included.
  inline std::vector<std::vector<long long>>
  shortest_distances(const Spec & s, bool directed, bool & neg)
  {
    std::vector<std::vector<long long>> d(size_t(s.n), std::vector<long long>(size_t(s.n), Inf));
    for (int i = 0; i < s.n; ++i)
      d[size_t(i)][size_t(i)] = 0;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        const size_t u = size_t(s.arcs[i][0]), v = size_t(s.arcs[i][1]);
        d[u][v] = std::min(d[u][v], s.w[i]);
        if (not directed)
          d[v][u] = std::min(d[v][u], s.w[i]);
      }
    for (size_t k = 0; k < size_t(s.n); ++k)
      for (size_t i = 0; i < size_t(s.n); ++i)
        for (size_t j = 0; j < size_t(s.n); ++j)
          if (d[i][k] != Inf and d[k][j] != Inf)
            d[i][j] = std::min(d[i][j], saturated_add(d[i][k], d[k][j]));
    neg = false;
    for (size_t i = 0; i < size_t(s.n); ++i)
      neg = neg or d[i][i] < 0;
    return d;
  }

  /// Whether the arcs, by number, form a closed walk of s whose weights
  /// add up to less than zero; in a graph that is not directed each arc
  /// may be walked either way
  inline bool is_negative_cycle(const Spec & s, const std::vector<int> & arcs,
                                bool directed)
  {
    if (arcs.empty())
      return false;
    const int first = s.arcs[size_t(arcs[0])][0];
    long long total = 0;
    // Try each direction of the first arc in a graph that is not directed
    for (int start : {first, s.arcs[size_t(arcs[0])][1]})
      {
        int at = start;
        total = 0;
        bool ok = true;
        for (int i : arcs)
          {
            const int u = s.arcs[size_t(i)][0], v = s.arcs[size_t(i)][1];
            if (u == at)
              at = v;
            else if (not directed and v == at)
              at = u;
            else
              {
                ok = false;
                break;
              }
            total += s.w[size_t(i)];
          }
        if (ok and at == start and total < 0)
          return true;
        if (directed)
          break;
      }
    return false;
  }

  /// The maximum flow from src to snk, by Edmonds-Karp on the matrix of
  /// the capacities w (loops carry nothing)
  inline long long max_flow(const Spec & s, int src, int snk)
  {
    std::vector<std::vector<long long>> c(size_t(s.n), std::vector<long long>(size_t(s.n), 0));
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (s.arcs[i][0] != s.arcs[i][1])
        c[size_t(s.arcs[i][0])][size_t(s.arcs[i][1])] += s.w[i];
    long long flow = 0;
    while (true)
      {
        std::vector<int> parent(size_t(s.n), -1);
        parent[size_t(src)] = src;
        std::vector<int> queue = {src};
        for (size_t h = 0; h < queue.size(); ++h)
          for (int v = 0; v < s.n; ++v)
            if (parent[size_t(v)] == -1 and c[size_t(queue[h])][size_t(v)] > 0)
              {
                parent[size_t(v)] = queue[h];
                queue.push_back(v);
              }
        if (parent[size_t(snk)] == -1)
          return flow;
        long long b = Inf;
        for (int v = snk; v != src; v = parent[size_t(v)])
          b = std::min(b, c[size_t(parent[size_t(v)])][size_t(v)]);
        for (int v = snk; v != src; v = parent[size_t(v)])
          {
            c[size_t(parent[size_t(v)])][size_t(v)] -= b;
            c[size_t(v)][size_t(parent[size_t(v)])] += b;
          }
        flow += b;
      }
  }
} // namespace graph_oracles

#endif // ALEPH_TESTS_GRAPH_ORACLES_H
