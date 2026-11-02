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
 * @file graph_exhaustive_negative_cycles_test.cc
 * @brief Negative cycles, Bellman-Ford and the minimum mean cycle on every
 *        small weighted digraph, against the oracles.
 *
 * Each test runs one family over every digraph of a domain of
 * graph_harness.H, with loops, parallel and antiparallel arcs and the arcs
 * in two orders, on the three kinds of digraph, with every assignment of
 * the weights of a class: from -2 to 2 on the digraphs of up to 2 nodes
 * and 3 arcs, and -2 or 1 on those of up to 3 nodes and 3 arcs (one more
 * node, and one more arc for the second, with ALEPH_GRAPH_HARNESS=heavy).
 * The filters run with every set of hidden arcs, and the whitelists with
 * every set of shown ones, on the smaller domain; there also run the
 * weights near the limits of int, where a sum that leaves int throws
 * overflow_error, and those of minus infinity and NaN, which throw
 * domain_error once the last pass meets them. After an operation that
 * throws, the object has no computation, the graph none of its data, and
 * the object runs again. The random digraphs have 6 to 8 nodes. The
 * distances are int and double.
 *
 * The oracle enumerates the simple cycles of the spec, in exact
 * arithmetic: whether there is a negative cycle, which nodes reach one,
 * the least cost of a cycle and the least mean, as a fraction. Every
 * witness must be a closed walk of arcs that the filter shows, in their
 * direction, whose cost is negative or the one reported, and a simple
 * cycle where the documentation promises one.
 *
 * The families are Bellman_Ford.H, with its tree of shortest paths and
 * the sequences of calls on one object; Negative_Cycles.H;
 * Min_Mean_Cycle.H and Howard_Min_Mean_Cycle.H; and the detection of the
 * negative cycles by Floyd_Warshall.H and Johnson.H, with the answers of
 * Johnson where there is none: its distances, paths, potentials and copy
 * of the graph, with whitelists of numbers and of pointers, double costs
 * with fractions and weights near the limits of int, and the graph that
 * it takes, which it must leave as it was. The last suites take the three
 * kinds of graph that are not directed: Bellman_Ford reads each arc from
 * its source to its target, Floyd_Warshall and Johnson in both
 * directions, and the other searches reject them.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
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
#include <Bellman_Ford.H>
#include <Negative_Cycles.H>
#include <Min_Mean_Cycle.H>
#include <Howard_Min_Mean_Cycle.H>
#include <Floyd_Warshall.H>
#include <Johnson.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;

namespace
{
  /// A number as text, as a stream writes it
  template <typename T>
  std::string text_of(const T & x)
  {
    std::ostringstream s;
    s << x;
    return s.str();
  }

  /// The weight of an arc, read by its number from a vector, as
  /// graph_filters::Weight_Of does, in a vector of weights of type T
  template <class GT, typename T>
  struct Spec_Weight
  {
    using Distance_Type = T;

    const std::vector<T> * weights = nullptr;

    T operator () (typename GT::Arc * a) const
    {
      return (*weights)[size_t(a->get_info())];
    }

    T operator () (typename GT::Arc * a, typename GT::Node *) const
    {
      return (*this)(a);
    }
  };

  /// A whitelist: shows only the arcs whose numbers are set in a mask
  template <class GT>
  struct Show_Mask
  {
    unsigned long shown = 0;

    bool operator () (typename GT::Arc * a) const noexcept
    {
      return a->get_info() >= 0 and ((shown >> a->get_info()) & 1UL) != 0;
    }
  };

  /// A whitelist of pointers: shows only the arcs of a set, and so no arc
  /// of a copy of the graph
  template <class GT>
  struct Show_Arcs
  {
    const std::set<typename GT::Arc *> * arcs = nullptr;

    bool operator () (typename GT::Arc * a) const
    {
      return arcs->count(a) != 0;
    }
  };

  /// Which arcs of s are shown when those of the mask hidden are hidden
  std::vector<bool> shown_but(const Spec & s, unsigned long hidden)
  {
    std::vector<bool> r(s.num_arcs());
    for (size_t i = 0; i < r.size(); ++i)
      r[i] = graph_filters::shown(hidden, int(i));
    return r;
  }

  /// Which arcs of s are shown when only those of the mask shown are
  std::vector<bool> shown_only(const Spec & s, unsigned long shown)
  {
    std::vector<bool> r(s.num_arcs());
    for (size_t i = 0; i < r.size(); ++i)
      r[i] = ((shown >> i) & 1UL) != 0;
    return r;
  }

  /// The weights from -2 to 2
  const std::vector<long long> Small_Weights = {-2, -1, 0, 1, 2};

  /// A negative weight and a positive one: a cycle of two arcs, one of
  /// each, is negative, and one of three, with one negative arc, costs 0
  const std::vector<long long> Sign_Weights = {-2, 1};

  /// The weights of the tests with double costs: a cycle of two arcs of
  /// -1 and 1 costs zero
  const std::vector<long long> Unit_Weights = {-1, 0, 1};

  /// The digraphs that take every weight from -2 to 2: up to 2 nodes and 3
  /// arcs, or 3 nodes with ALEPH_GRAPH_HARNESS=heavy
  Domain small_weights_domain()
  {
    Domain d;
    d.max_nodes = heavy() ? 3 : 2;
    d.max_arcs = 3;
    return d;
  }

  /// The digraphs that take the weights -2 and 1: up to 3 nodes and 3
  /// arcs, or 4 and 4 with ALEPH_GRAPH_HARNESS=heavy
  Domain sign_domain()
  {
    return domain(3, 3);
  }

  /// Calls f(t) with t, a copy of s, with each assignment of values to the
  /// weights of its arcs
  template <class F>
  void for_each_weighting(const Spec & s, const std::vector<long long> & values, F && f)
  {
    Spec t = s;
    t.w.assign(s.num_arcs(), values[0]);
    std::vector<size_t> k(s.num_arcs(), 0);
    while (true)
      {
        f(static_cast<const Spec &>(t));
        size_t i = 0;
        for (; i < k.size(); ++i)
          {
            if (++k[i] < values.size())
              {
                t.w[i] = values[k[i]];
                break;
              }
            k[i] = 0;
            t.w[i] = values[0];
          }
        if (i == k.size())
          return;
      }
  }

  /// Calls f(s) with every weighted spec of the two domains of the tests,
  /// with the arcs read as directed, the first one with the weights of
  /// small, by default those from -2 to 2; those of the second domain that
  /// the first one has are left out
  template <class F>
  void for_each_weighted_spec(F && f, const std::vector<long long> & small_weights = Small_Weights)
  {
    const Domain small = small_weights_domain();
    for_each_spec(small, true, [&] (const Spec & s)
      {
        for_each_weighting(s, small_weights, f);
      });
    for_each_spec(sign_domain(), true, [&] (const Spec & s)
      {
        if (s.n > small.max_nodes or int(s.num_arcs()) > small.max_arcs)
          for_each_weighting(s, Sign_Weights, f);
      });
  }

  /// The numbers of the nodes and of the arcs of a walk, in order
  struct Walk
  {
    std::vector<int> nodes;
    std::vector<int> arcs;

    [[nodiscard]] std::string str() const
    {
      std::string r = "[";
      for (size_t i = 0; i < nodes.size(); ++i)
        {
          r += std::to_string(nodes[i]);
          if (i < arcs.size())
            r += " -" + std::to_string(arcs[i]) + "-> ";
        }
      return r + "]";
    }
  };

  /// The walk of a path, with the number of each arc given by number
  template <class GT, class Number>
  Walk walk_of(const Path<GT> & p, Number && number)
  {
    Walk w;
    for (typename Path<GT>::Iterator it(p); it.has_current_node(); it.next_ne())
      {
        w.nodes.push_back(num(it.get_current_node_ne()));
        if (it.has_current_arc())
          w.arcs.push_back(number(it.get_current_arc_ne()));
      }
    return w;
  }

  /// The walk of a path whose arcs have their numbers as infos
  template <class GT>
  Walk walk_of(const Path<GT> & p)
  {
    return walk_of(p, [] (typename GT::Arc * a) { return num(a); });
  }

  /// The walk of a list of nodes and the list of the arcs between them
  template <class Nodes, class Arcs>
  Walk walk_of(const Nodes & nodes, const Arcs & arcs)
  {
    Walk w;
    for (auto it = nodes.get_it(); it.has_curr(); it.next_ne())
      w.nodes.push_back(num(it.get_curr()));
    for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
      w.arcs.push_back(num(it.get_curr()));
    return w;
  }

  /// What is wrong with w as a walk of s from its first node to its last,
  /// along the direction of its arcs and made of arcs that show says;
  /// empty if nothing
  std::string walk_error(const Spec & s, const Walk & w, const std::vector<bool> & show)
  {
    if (w.nodes.empty() or w.nodes.size() != w.arcs.size() + 1)
      return "the walk " + w.str() + " is empty or lacks arcs";
    for (size_t i = 0; i < w.arcs.size(); ++i)
      {
        const int a = w.arcs[i];
        if (a < 0 or size_t(a) >= s.arcs.size())
          return "the walk " + w.str() + " has an arc that is not of the graph";
        if (s.arcs[size_t(a)][0] != w.nodes[i] or s.arcs[size_t(a)][1] != w.nodes[i + 1])
          return "the walk " + w.str() + " does not follow the arc " + std::to_string(a);
        if (not show[size_t(a)])
          return "the walk " + w.str() + " takes the hidden arc " + std::to_string(a);
      }
    return "";
  }

  /// What is wrong with w as a closed walk of s of one arc or more, as
  /// walk_error() says, and without a repeated node if simple; empty if
  /// nothing
  std::string closed_walk_error(const Spec & s, const Walk & w,
                                const std::vector<bool> & show, bool simple)
  {
    if (auto e = walk_error(s, w, show); not e.empty())
      return e;
    if (w.arcs.empty() or w.nodes.front() != w.nodes.back())
      return "the walk " + w.str() + " is not a closed one";
    if (simple and std::set<int>(w.nodes.begin() + 1, w.nodes.end()).size() != w.arcs.size())
      return "the cycle " + w.str() + " repeats a node";
    return "";
  }

  /// The sum of the weights of the arcs of a walk of s
  long long cost_of(const Spec & s, const Walk & w)
  {
    long long c = 0;
    for (int a : w.arcs)
      c += s.w[size_t(a)];
    return c;
  }

  /// A simple cycle of a spec: the numbers of its arcs from its smallest
  /// node, in their direction, and the sum of their weights
  struct Cycle
  {
    std::vector<int> arcs;
    long long cost = 0;
  };

  /// Every simple cycle of the arcs of s that show says, read from source
  /// to target, each once: a loop is a cycle of one arc, and parallel arcs
  /// make different cycles
  std::vector<Cycle> simple_cycles(const Spec & s, const std::vector<bool> & show)
  {
    std::vector<Cycle> r;
    std::vector<bool> on(size_t(s.n), false);
    std::vector<int> arcs;
    // Extends the path in arcs, from root to x at cost c, by each arc out
    // of x: to root, which closes a cycle, or to a node above root that is
    // not on the path
    auto extend = [&] (auto & self, int root, int x, long long c) -> void
      {
        for (size_t i = 0; i < s.arcs.size(); ++i)
          {
            if (not show[i] or s.arcs[i][0] != x)
              continue;
            const int y = s.arcs[i][1];
            arcs.push_back(int(i));
            if (y == root)
              r.push_back({arcs, c + s.w[i]});
            else if (y > root and not on[size_t(y)])
              {
                on[size_t(y)] = true;
                self(self, root, y, c + s.w[i]);
                on[size_t(y)] = false;
              }
            arcs.pop_back();
          }
      };
    for (int root = 0; root < s.n; ++root)
      extend(extend, root, root, 0);
    return r;
  }

  /// Whether the arcs of s that show says have a negative simple cycle
  bool has_negative_cycle(const Spec & s, const std::vector<bool> & show)
  {
    for (const auto & c : simple_cycles(s, show))
      if (c.cost < 0)
        return true;
    return false;
  }

  /// The spec of the arcs of s that show says, with their weights
  Spec shown_spec(const Spec & s, const std::vector<bool> & show)
  {
    unsigned long hidden = 0;
    for (size_t i = 0; i < show.size(); ++i)
      if (not show[i])
        hidden |= 1UL << i;
    std::vector<int> orig;
    return oracle::without(s, hidden, orig);
  }

  /// What the oracles say of a weighted spec, on the arcs that show says,
  /// read from source to target
  struct Truth
  {
    std::vector<bool> show;
    std::vector<Cycle> cycles;
    std::vector<std::vector<bool>> reach;
    std::vector<std::vector<long long>> dist; ///< Exact where no negative cycle is reached
    bool negative = false;                    ///< There is a negative cycle
    std::vector<bool> negative_from;          ///< A negative cycle is reached from each node

    Truth(const Spec & s, std::vector<bool> shown) : show(std::move(shown))
    {
      cycles = simple_cycles(s, show);
      const Spec t = shown_spec(s, show);
      reach = oracle::closure(t, true);
      bool neg = false;
      dist = oracle::shortest_distances(t, true, neg);
      negative_from.assign(size_t(s.n), false);
      for (const auto & c : cycles)
        if (c.cost < 0)
          {
            negative = true;
            const int x = s.arcs[size_t(c.arcs[0])][0];
            for (int u = 0; u < s.n; ++u)
              if (oracle::reaches(reach, u, x))
                negative_from[size_t(u)] = true;
          }
    }

    [[nodiscard]] bool reaches(int u, int v) const { return oracle::reaches(reach, u, v); }
  };

  /// The potential of each node: its distance from a node with an arc of
  /// weight zero to every other one, min(0, the least distance from any
  /// node to it), where there is no negative cycle
  std::vector<long long> potentials(const Truth & t, int n)
  {
    std::vector<long long> h(size_t(n), 0);
    for (int u = 0; u < n; ++u)
      for (int v = 0; v < n; ++v)
        if (t.dist[size_t(u)][size_t(v)] != oracle::Inf)
          h[size_t(v)] = std::min(h[size_t(v)], t.dist[size_t(u)][size_t(v)]);
    return h;
  }

  /// The least mean of a cycle of t, as num / den with den > 0; false if
  /// there is no cycle
  bool least_mean(const Truth & t, long long & num, long long & den)
  {
    bool found = false;
    for (const auto & c : t.cycles)
      {
        const long long len = (long long) c.arcs.size();
        if (not found or c.cost * den < num * len)
          {
            num = c.cost;
            den = len;
            found = true;
          }
      }
    return found;
  }

  /// The least cost of a simple cycle of t of at most len arcs and the
  /// fewest arcs of a cycle of that cost; false if there is none
  bool least_cost(const Truth & t, size_t len, long long & cost, size_t & arcs)
  {
    bool found = false;
    for (const auto & c : t.cycles)
      if (c.arcs.size() <= len and
          (not found or c.cost < cost or (c.cost == cost and c.arcs.size() < arcs)))
        {
          cost = c.cost;
          arcs = c.arcs.size();
          found = true;
        }
    return found;
  }

  /// The relaxed bound of most_negative_cycle_bounded(): the least cost of
  /// a closed walk of 1 to len arcs of those that show says, that leaves
  /// its origin by its first arc and comes back to it only by its last;
  /// false if there is none
  bool least_closed_walk(const Spec & s, const std::vector<bool> & show, size_t len,
                         long long & cost)
  {
    bool found = false;
    const size_t n = size_t(s.n);
    for (int v = 0; v < s.n; ++v)
      {
        // cur[u]: the least cost of such a walk of k arcs from v to u
        std::vector<long long> cur(n, oracle::Inf);
        for (size_t i = 0; i < s.arcs.size(); ++i)
          if (show[i] and s.arcs[i][0] == v)
            cur[size_t(s.arcs[i][1])] = std::min(cur[size_t(s.arcs[i][1])], s.w[i]);
        for (size_t k = 1; k <= len; ++k)
          {
            if (cur[size_t(v)] != oracle::Inf and (not found or cur[size_t(v)] < cost))
              {
                cost = cur[size_t(v)];
                found = true;
              }
            std::vector<long long> next(n, oracle::Inf);
            for (size_t i = 0; i < s.arcs.size(); ++i)
              {
                const int x = s.arcs[i][0], y = s.arcs[i][1];
                if (show[i] and x != v and cur[size_t(x)] != oracle::Inf)
                  next[size_t(y)] = std::min(next[size_t(y)], cur[size_t(x)] + s.w[i]);
              }
            cur = std::move(next);
          }
      }
    return found;
  }

  /** Whether some sum that Bellman_Ford can compute on the arcs that show
      says may leave [lo, hi], and overflow or reach the max() that stands
      for no distance. Each sum is the cost of a walk. From above, it is
      bounded by the costliest simple path followed by one more arc: a
      distance only goes down from its first value, that of a simple path.
      From below, by the cheapest walk of at most k arcs, with k a bound of
      the relaxations of the passes and of the rounds of SPFA. Both bounds
      are taken from every node, which covers the searches without a start,
      from every node at once, and the sums of get_distance() on a path of
      at most two arcs. */
  bool sums_may_overflow(const Spec & s, const std::vector<bool> & show,
                         long long lo, long long hi)
  {
    const size_t n = size_t(s.n), m = s.num_arcs();
    long long most = 0;
    std::vector<bool> on(n, false);
    auto extend = [&] (auto & self, int x, long long c) -> void
      {
        most = std::max(most, c);
        for (size_t i = 0; i < m; ++i)
          if (show[i] and s.arcs[i][0] == x)
            {
              const int y = s.arcs[i][1];
              most = std::max(most, c + s.w[i]);
              if (not on[size_t(y)])
                {
                  on[size_t(y)] = true;
                  self(self, y, c + s.w[i]);
                  on[size_t(y)] = false;
                }
            }
      };
    for (int x = 0; x < s.n; ++x)
      {
        on[size_t(x)] = true;
        extend(extend, x, 0);
        on[size_t(x)] = false;
      }

    const size_t k = (n + 4) * (m + n) + 1;
    long long least = 0;
    std::vector<long long> cur(n, 0);
    for (size_t step = 0; step < k; ++step)
      {
        std::vector<long long> next(n, oracle::Inf);
        for (size_t i = 0; i < m; ++i)
          if (show[i] and cur[size_t(s.arcs[i][0])] != oracle::Inf)
            next[size_t(s.arcs[i][1])] = std::min(next[size_t(s.arcs[i][1])],
                                                  cur[size_t(s.arcs[i][0])] + s.w[i]);
        for (long long x : next)
          if (x != oracle::Inf)
            least = std::min(least, x);
        cur = std::move(next);
      }
    return most > hi or least < lo;
  }

  /// Whether some walk of 1 to len arcs of those that show says costs a
  /// sum out of [lo, hi]: the walks of the dynamic program of
  /// most_negative_cycle_bounded() may go round a loop, or a cycle, more
  /// than once
  bool walks_may_overflow(const Spec & s, const std::vector<bool> & show, size_t len,
                          long long lo, long long hi)
  {
    const size_t n = size_t(s.n);
    // The costliest and the cheapest walk of k arcs that ends at each node
    std::vector<long long> most(n, 0), least(n, 0);
    std::vector<bool> ends(n, true);
    for (size_t k = 1; k <= len; ++k)
      {
        std::vector<long long> next_most(n, std::numeric_limits<long long>::lowest());
        std::vector<long long> next_least(n, std::numeric_limits<long long>::max());
        std::vector<bool> next_ends(n, false);
        for (size_t i = 0; i < s.num_arcs(); ++i)
          {
            const size_t x = size_t(s.arcs[i][0]), y = size_t(s.arcs[i][1]);
            if (not show[i] or not ends[x])
              continue;
            next_ends[y] = true;
            next_most[y] = std::max(next_most[y], most[x] + s.w[i]);
            next_least[y] = std::min(next_least[y], least[x] + s.w[i]);
          }
        for (size_t v = 0; v < n; ++v)
          if (next_ends[v] and (next_most[v] > hi or next_least[v] < lo))
            return true;
        most = std::move(next_most);
        least = std::move(next_least);
        ends = std::move(next_ends);
      }
    return false;
  }

  /// A weighted spec built as a graph of kind GT with weights of type T,
  /// and what the oracles say of it on the arcs that a filter shows
  template <class GT, typename T>
  struct Case
  {
    const Spec & s;
    Built<GT> b;
    std::vector<T> w;
    Spec_Weight<GT, T> d;
    Truth t;
    /// What the failures tell first: with a filter, "hiding m: " or
    /// "showing m: ", where the bit i of the mask m is the arc i
    std::string label;
    /// Whether a sum of an operation may leave the range of an integral T,
    /// which then throws overflow_error
    bool may_overflow = false;
    /// Whether a weight is minus infinity or NaN, which makes an operation
    /// throw domain_error if its last pass meets it
    bool may_reject = false;
    /// The arcs that the filter shows to the operations: those of the
    /// oracle, unless some of them do not count for it
    std::vector<bool> filtered;
    /// The oracle of the signs of the weights, on the arcs filtered, once
    /// an operation has thrown
    std::optional<Truth> signs;

    Case(const Spec & spec, std::vector<bool> show)
      : s(spec), b(spec), w(spec.w.begin(), spec.w.end()), t(spec, std::move(show)),
        filtered(t.show)
    {
      d.weights = &w;
    }

    Case(const Case &) = delete;
    Case & operator = (const Case &) = delete;

    [[nodiscard]] const std::vector<bool> & show() const noexcept { return t.show; }
  };

  template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
  using BF = Bellman_Ford<GT, Spec_Weight<GT, T>, Arc_Iterator, Out_Iterator, SA>;

  /// Whether the graph of b is still that of s, with the numbers of its
  /// arcs, after a search, which must neither add nor change anything
  template <class GT>
  bool intact(const Built<GT> & b, const Spec & s)
  {
    if (b.g.get_num_nodes() != size_t(s.n) or b.g.get_num_arcs() != s.num_arcs())
      return false;
    for (size_t i = 0; i < b.arcs.size(); ++i)
      if (num(b.arcs[i]) != int(i))
        return false;
    return true;
  }

  /// The signs of the weights of s, which fit in every sum of the tests
  Spec with_signs(const Spec & s)
  {
    Spec t = s;
    for (auto & w : t.w)
      w = w < 0 ? -1 : w > 0 ? 1 : 0;
    return t;
  }

  /// What is wrong with the graph of c after an operation threw: a node
  /// keeps its data, the bit Depth_First of SPFA or a painting, or an arc a
  /// painting; empty if nothing
  template <class GT, typename T>
  std::string data_error(const Case<GT, T> & c)
  {
    for (auto * p : c.b.nodes)
      if (NODE_COOKIE(p) != nullptr or IS_NODE_VISITED(p, Aleph::Depth_First)
          or IS_NODE_VISITED(p, Aleph::Spanning_Tree))
        return "the node " + std::to_string(num(p)) + " keeps its data or a bit";
    for (auto * a : c.b.arcs)
      if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
        return "the arc " + std::to_string(num(a)) + " keeps its bit";
    return "";
  }

  /// What is wrong with bf and the graph of c after an operation of bf
  /// threw: bf keeps no computation, and data_error(); empty if nothing
  template <class GT, typename T, class SA>
  std::string state_error(const BF<GT, T, SA> & bf, const Case<GT, T> & c)
  {
    if (bf.has_computation() or bf.is_painted())
      return "the object keeps a computation";
    return data_error(c);
  }

  /// What is wrong after an operation of bf threw: state_error(), and that
  /// bf runs again, here with the signs of the weights of c, which fit;
  /// empty if nothing. The weights of c are left as they were.
  template <class GT, typename T, class SA>
  std::string after_throw_error(BF<GT, T, SA> & bf, Case<GT, T> & c)
  {
    if (auto e = state_error(bf, c); not e.empty() or c.s.n == 0)
      return e;
    const Spec t = with_signs(c.s);
    if (not c.signs)
      c.signs.emplace(t, c.filtered);
    const std::vector<T> weights = c.w;
    for (size_t i = 0; i < c.w.size(); ++i)
      c.w[i] = T(t.w[i]);
    const bool negative = bf.paint_spanning_tree(c.b.nodes[0]);
    c.w = weights;
    return negative == c.signs->negative_from[0] ? "" : "it does not run again";
  }

  /** Runs call(), an operation of bf on c that checks its answer on k and
      counts a run of k when it returns, and returns whether it returned.
      An overflow_error, which counts a run, is right only where c may
      overflow, and a domain_error only where c may reject a weight; then
      bf must be as after_throw_error() says. Another domain_error goes on. */
  template <class GT, typename T, class SA, class Call>
  bool returns(Check & k, BF<GT, T, SA> & bf, Case<GT, T> & c, const std::string & where,
               Call && call)
  {
    try
      {
        call();
        return true;
      }
    catch (const std::overflow_error & e)
      {
        k.run();
        if (not c.may_overflow)
          k.fail(c.s, c.label + where + "throws " + e.what());
        else if (auto err = after_throw_error(bf, c); not err.empty())
          k.fail(c.s, c.label + where + "after overflow_error, " + err);
        return false;
      }
    catch (const std::domain_error &)
      {
        if (not c.may_reject)
          throw;
        k.run();
        if (auto err = after_throw_error(bf, c); not err.empty())
          k.fail(c.s, c.label + where + "after domain_error, " + err);
        return false;
      }
  }

  const char * says(bool found)
  {
    return found ? "finds a negative cycle where there is none" : "finds no negative cycle";
  }

  std::string from_text(int u)
  {
    return "from " + std::to_string(u) + ": ";
  }

  /// The checks of whether Bellman_Ford finds a negative cycle
  struct Detection_Checks
  {
    Check paint;
    Check faster;
    Check from;
    Check global;
    Check weights;
    size_t turn = 0;

    explicit Detection_Checks(const std::string & how = "")
      : paint("Bellman_Ford::paint_spanning_tree()" + how),
        faster("Bellman_Ford::faster_paint_spanning_tree()" + how),
        from("Bellman_Ford::has_negative_cycle(start)" + how),
        global("Bellman_Ford::has_negative_cycle()" + how),
        weights("Bellman_Ford::compute_nodes_weights()" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&paint, &faster, &from, &global, &weights})
        c->expect();
    }
  };

  /// What is wrong with the potentials that compute_nodes_weights() gives;
  /// empty if nothing
  template <class GT, typename T, class Map>
  std::string potentials_error(const Case<GT, T> & c, const Map & m)
  {
    const auto h = potentials(c.t, c.s.n);
    if (m.size() != size_t(c.s.n))
      return "gives " + std::to_string(m.size()) + " potentials";
    for (int v = 0; v < c.s.n; ++v)
      {
        const auto * p = m.search(c.b.nodes[size_t(v)]);
        if (p == nullptr)
          return "gives no potential to " + std::to_string(v);
        if ((long double) p->second != (long double) h[size_t(v)])
          return "gives " + std::to_string((long double) p->second) + " to "
            + std::to_string(v) + ", not " + std::to_string(h[size_t(v)]);
      }
    return "";
  }

  /// Runs the detection of Bellman_Ford with the filter sa on c, from each
  /// node and in the whole graph, against the oracle; each run paints with
  /// paint_spanning_tree() or with faster_paint_spanning_tree() in turn
  template <class GT, typename T, class SA>
  void run_detection(Detection_Checks & k, Case<GT, T> & c, const SA & sa)
  {
    const Spec & s = c.s;
    const bool faster = k.turn++ % 2 == 1;
    Check & paint = faster ? k.faster : k.paint;
    for (int u = 0; u < s.n; ++u)
      {
        auto * p = c.b.nodes[size_t(u)];
        const bool want = c.t.negative_from[size_t(u)];
        const std::string from = from_text(u);
        {
          BF<GT, T, SA> bf(c.b.g, c.d, sa);
          returns(paint, bf, c, from, [&]
            {
              const bool got = faster ? bf.faster_paint_spanning_tree(p)
                                      : bf.paint_spanning_tree(p);
              paint.run();
              if (got != want)
                paint.fail(s, c.label + from + says(got));
            });
        }
        BF<GT, T, SA> bf(c.b.g, c.d, sa);
        returns(k.from, bf, c, from, [&]
          {
            const bool got = bf.has_negative_cycle(p);
            k.from.run();
            if (got != want)
              k.from.fail(s, c.label + from + says(got));
          });
      }

    {
      BF<GT, T, SA> bf(c.b.g, c.d, sa);
      returns(k.global, bf, c, "", [&]
        {
          const bool got = bf.has_negative_cycle();
          k.global.run();
          if (got != c.t.negative)
            k.global.fail(s, c.label + says(got));
          else if (not intact(c.b, s))
            k.global.fail(s, c.label + "leaves the graph changed");
        });
    }

    BF<GT, T, SA> bf(c.b.g, c.d, sa);
    returns(k.weights, bf, c, "", [&]
      {
        try
          {
            const auto m = bf.compute_nodes_weights();
            k.weights.run();
            if (c.t.negative)
              k.weights.fail(s, c.label + "does not throw on a negative cycle");
            else if (auto e = potentials_error(c, m); not e.empty())
              k.weights.fail(s, c.label + e);
          }
        catch (const std::domain_error &)
          {
            k.weights.run();
            if (not c.t.negative and not c.may_reject)
              k.weights.fail(s, c.label + "throws domain_error without a negative cycle");
            else if (auto e = state_error(bf, c); not e.empty())
              k.weights.fail(s, c.label + "after domain_error, " + e);
          }
      });
    if (not intact(c.b, s))
      k.weights.fail(s, c.label + "leaves the graph changed");
  }

  /// Checks that path, a witness of Bellman_Ford, is empty if want is
  /// false, and otherwise a cycle of c, simple as one of the graph of the
  /// predecessors, along its arcs and of arcs that the filter shows, whose
  /// weights add up to less than zero; with found, also that the search
  /// returned whether it found one
  template <class GT, typename T>
  void check_witness(Check & k, const Case<GT, T> & c, const Path<GT> & path, bool want,
                     const std::string & where, const bool * found = nullptr)
  {
    k.run();
    if (found != nullptr and *found == path.is_empty())
      {
        k.fail(c.s, c.label + where + "returns " + (*found ? "true" : "false") + " with a path of "
               + std::to_string(path.size()) + " nodes");
        return;
      }
    if (path.is_empty())
      {
        if (want)
          k.fail(c.s, c.label + where + "finds no negative cycle");
        return;
      }
    const Walk w = walk_of(path);
    if (auto e = closed_walk_error(c.s, w, c.show(), true); not e.empty())
      k.fail(c.s, c.label + where + e);
    else if (const long long cost = cost_of(c.s, w); cost >= 0)
      k.fail(c.s, c.label + where + "the cycle " + w.str() + " costs " + std::to_string(cost));
    else if (not want)
      k.fail(c.s, c.label + where + "finds the negative cycle " + w.str()
             + ", that the oracle lacks");
  }

  /// The ways in which search_negative_cycle() with it_factor and step
  /// looks for a cycle on its way: from the first round, from the middle,
  /// every other round, and never before the end
  const std::pair<double, size_t> Steps[] = {{0.0, 1}, {0.5, 1}, {1.0, 2}, {4.0, 1}};

  /// The checks of the negative cycles that the four searches of
  /// Bellman_Ford give, from a start or in the whole graph
  struct Witness_Checks
  {
    Check test;
    Check test_path;
    Check search;
    Check steps;
    size_t turn = 0;

    Witness_Checks(bool global, const std::string & how)
      : test(std::string("Bellman_Ford::test_negative_cycle(")
             + (global ? "" : "start") + ")" + how),
        test_path(std::string("Bellman_Ford::test_negative_cycle(")
                  + (global ? "path" : "start, path") + ")" + how),
        search(std::string("Bellman_Ford::search_negative_cycle(")
               + (global ? "" : "start") + ")" + how),
        steps(std::string("Bellman_Ford::search_negative_cycle(")
              + (global ? "" : "start, ") + "it_factor, step)" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&test, &test_path, &search, &steps})
        c->expect();
    }
  };

  /// Runs the searches of a negative cycle of Bellman_Ford with the filter
  /// sa on c, from each node or in the whole graph, against the oracle.
  /// Each run calls test_negative_cycle() with a path or without one, and
  /// search_negative_cycle() with it_factor and step, in the next of their
  /// ways, or without them, all in turn: each pair shares its code.
  template <class GT, typename T, class SA>
  void run_witnesses(Witness_Checks & k, Case<GT, T> & c, const SA & sa, bool global)
  {
    const size_t turn = k.turn++;
    // Not a structured binding, which Clang 14 does not capture in a lambda
    const double factor = Steps[(turn / 4) % std::size(Steps)].first;
    const size_t step = Steps[(turn / 4) % std::size(Steps)].second;
    const bool with_path = turn % 2 == 1;
    const bool with_steps = (turn / 2) % 2 == 1;
    const std::string with = "with " + text_of(factor) + " and " + text_of(step) + ": ";

    // The searches from u, or from no start if u is -1
    auto searches = [&] (int u)
      {
        typename GT::Node * p = u < 0 ? nullptr : c.b.nodes[size_t(u)];
        const bool want = u < 0 ? c.t.negative : c.t.negative_from[size_t(u)];
        const std::string from = u < 0 ? std::string() : from_text(u);
        {
          Check & test = with_path ? k.test_path : k.test;
          BF<GT, T, SA> bf(c.b.g, c.d, sa);
          returns(test, bf, c, from, [&]
            {
              if (not with_path)
                check_witness(test, c, u < 0 ? bf.test_negative_cycle() : bf.test_negative_cycle(p),
                              want, from);
              else
                {
                  Path<GT> path(c.b.g);
                  const bool found = u < 0 ? bf.test_negative_cycle(path)
                                           : bf.test_negative_cycle(p, path);
                  check_witness(test, c, path, want, from, &found);
                }
            });
        }
        Check & search = with_steps ? k.steps : k.search;
        BF<GT, T, SA> bf(c.b.g, c.d, sa);
        returns(search, bf, c, from, [&]
          {
            if (not with_steps)
              check_witness(search, c,
                            u < 0 ? bf.search_negative_cycle() : bf.search_negative_cycle(p),
                            want, from);
            else
              check_witness(search, c,
                            std::get<0>(u < 0 ? bf.search_negative_cycle(factor, step)
                                              : bf.search_negative_cycle(p, factor, step)),
                            want, from + with);
          });
      };

    if (not global)
      for (int u = 0; u < c.s.n; ++u)
        searches(u);
    else
      {
        searches(-1);
        if (not intact(c.b, c.s))
          k.test.fail(c.s, c.label + "a search leaves the graph changed");
      }
  }

  /// The checks of the eight calls of Bellman_Ford_Negative_Cycle, by
  /// their arguments, made with lvalues and with rvalues in turn
  struct Functor_Checks
  {
    Check path;
    Check start_path;
    Check start;
    Check graph;
    size_t turn = 0;

    explicit Functor_Checks(const std::string & how)
      : path("Bellman_Ford_Negative_Cycle(g, path, d, sa)" + how),
        start_path("Bellman_Ford_Negative_Cycle(g, start, path, d, sa)" + how),
        start("Bellman_Ford_Negative_Cycle(g, start, d, sa)" + how),
        graph("Bellman_Ford_Negative_Cycle(g, d, sa)" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&path, &start_path, &start, &graph})
        c->expect();
    }
  };

  /// The first node and the last one of s, the starts of the searches that
  /// do not run from every node
  std::vector<int> ends_of(const Spec & s)
  {
    if (s.n == 0)
      return {};
    if (s.n == 1)
      return {0};
    return {0, s.n - 1};
  }

  /// Runs the calls of Bellman_Ford_Negative_Cycle with the iterator NAit
  /// and the filter sa on c, from the first node and the last one and in
  /// the whole graph, against the oracle. Each run takes lvalues or
  /// rvalues, and the call with a path or the one that returns it, in turn.
  template <template <class, class> class NAit, class GT, typename T, class SA>
  void run_functors(Functor_Checks & k, Case<GT, T> & c, const SA & sa)
  {
    using D = Spec_Weight<GT, T>;
    using F = Bellman_Ford_Negative_Cycle<GT, D, Arc_Iterator, NAit, SA>;
    const size_t turn = k.turn++;
    const bool lvalues = turn % 2 == 0;
    const bool with_path = (turn / 2) % 2 == 0;
    const std::string how = lvalues ? "lvalues: " : "rvalues: ";
    D d = c.d;
    SA a = sa;
    // A call that throws fails its check, and the others still run
    auto guarded = [&] (Check & k, const std::string & where, auto && call)
      {
        try
          {
            call();
          }
        catch (const std::exception & e)
          {
            k.run();
            k.fail(c.s, c.label + where + "throws " + e.what());
          }
      };
    {
      const bool want = c.t.negative;
      if (with_path)
        guarded(k.path, how, [&]
          {
            Path<GT> path(c.b.g);
            const bool found = lvalues ? F()(c.b.g, path, d, a) : F()(c.b.g, path, D(d), SA(a));
            check_witness(k.path, c, path, want, how, &found);
          });
      else
        guarded(k.graph, how, [&]
          {
            check_witness(k.graph, c, lvalues ? F()(c.b.g, d, a) : F()(c.b.g, D(d), SA(a)), want,
                          how);
          });
      if (not intact(c.b, c.s))
        k.graph.fail(c.s, c.label + "a search leaves the graph changed");
    }
    for (int u : ends_of(c.s))
      {
        auto * p = c.b.nodes[size_t(u)];
        const bool want = c.t.negative_from[size_t(u)];
        const std::string where = how + from_text(u);
        if (with_path)
          guarded(k.start_path, where, [&]
            {
              Path<GT> path(c.b.g);
              const bool found = lvalues ? F()(c.b.g, p, path, d, a)
                                         : F()(c.b.g, p, path, D(d), SA(a));
              check_witness(k.start_path, c, path, want, where, &found);
            });
        else
          guarded(k.start, where, [&]
            {
              check_witness(k.start, c,
                            lvalues ? F()(c.b.g, p, d, a) : F()(c.b.g, p, D(d), SA(a)), want,
                            where);
            });
      }
  }

  /// The numbers of the arcs of b that carry the bit Spanning_Tree
  template <class GT>
  std::set<int> painted_arcs(const Built<GT> & b)
  {
    std::set<int> r;
    for (size_t i = 0; i < b.arcs.size(); ++i)
      if (IS_ARC_VISITED(b.arcs[i], Aleph::Spanning_Tree))
        r.insert(int(i));
    return r;
  }

  /// What is wrong with the painting of the tree of shortest paths from u:
  /// the nodes painted are those that u reaches, and each but u has one
  /// painted arc in, that the filter shows and that is on a shortest path;
  /// empty if nothing
  template <class GT, typename T>
  std::string painting_error(const Case<GT, T> & c, int u, const std::set<int> & painted)
  {
    const Spec & s = c.s;
    std::vector<int> into(size_t(s.n), 0);
    for (int i : painted)
      {
        const int x = s.arcs[size_t(i)][0], y = s.arcs[size_t(i)][1];
        if (not c.show()[size_t(i)])
          return "paints the hidden arc " + std::to_string(i);
        if (not c.t.reaches(u, x) or y == u)
          return "paints the arc " + std::to_string(i) + ", out of the tree";
        if (c.t.dist[size_t(u)][size_t(x)] + s.w[size_t(i)] != c.t.dist[size_t(u)][size_t(y)])
          return "paints the arc " + std::to_string(i) + ", which is not on a shortest path";
        ++into[size_t(y)];
      }
    for (int v = 0; v < s.n; ++v)
      {
        if (IS_NODE_VISITED(c.b.nodes[size_t(v)], Aleph::Spanning_Tree) != c.t.reaches(u, v))
          return "paints the node " + std::to_string(v) + " wrongly";
        if (v != u and c.t.reaches(u, v) and into[size_t(v)] != 1)
          return "paints " + std::to_string(into[size_t(v)]) + " arcs into " + std::to_string(v);
      }
    return "";
  }

  /// What is wrong with the distances that get_distance() gives from u on
  /// a painted tree: that of the oracle to each node reached, and
  /// domain_error for the others, or for all if domain_error_is_right;
  /// empty if nothing
  template <class GT, typename T, class SA>
  std::string distances_error(BF<GT, T, SA> & bf, const Case<GT, T> & c, int u,
                              bool domain_error_is_right = false)
  {
    for (int v = 0; v < c.s.n; ++v)
      {
        const bool reached = c.t.reaches(u, v);
        try
          {
            const T got = bf.get_distance(c.b.nodes[size_t(v)]);
            if (not reached)
              return "gives a distance to " + std::to_string(v) + ", which is not reached";
            if ((long double) got != (long double) c.t.dist[size_t(u)][size_t(v)])
              return "gives " + text_of(got) + " to " + std::to_string(v) + ", not "
                + std::to_string(c.t.dist[size_t(u)][size_t(v)]);
          }
        catch (const std::domain_error &)
          {
            if (reached and not domain_error_is_right)
              return "throws domain_error for " + std::to_string(v) + ", which is reached";
          }
      }
    return "";
  }

  /// What is wrong with path, of cost got, as a shortest path of c from u
  /// to v, along its arcs and of arcs that the filter shows; empty if
  /// nothing
  template <class GT, typename T>
  std::string min_path_error(const Case<GT, T> & c, const Path<GT> & path, int u, int v, T got)
  {
    const Walk w = walk_of(path);
    if (auto e = walk_error(c.s, w, c.show()); not e.empty())
      return e;
    if (w.nodes.front() != u or w.nodes.back() != v)
      return "the path " + w.str() + " does not go from " + std::to_string(u) + " to "
        + std::to_string(v);
    if (std::set<int>(w.nodes.begin(), w.nodes.end()).size() != w.nodes.size())
      return "the path " + w.str() + " repeats a node";
    const long long want = c.t.dist[size_t(u)][size_t(v)];
    if (cost_of(c.s, w) != want or (long double) got != (long double) want)
      return "the path " + w.str() + " costs " + std::to_string(cost_of(c.s, w)) + " and "
        + text_of(got) + " is returned, not " + std::to_string(want);
    return "";
  }

  /// What is wrong with the paths that get_min_path() gives from u on a
  /// painted tree: a shortest one to each node reached, and domain_error
  /// for the others, or for all if domain_error_is_right; empty if nothing
  template <class GT, typename T, class SA>
  std::string min_paths_error(BF<GT, T, SA> & bf, const Case<GT, T> & c, int u,
                              bool domain_error_is_right = false)
  {
    for (int v = 0; v < c.s.n; ++v)
      {
        const bool reached = c.t.reaches(u, v);
        try
          {
            Path<GT> path(c.b.g);
            const T got = bf.get_min_path(c.b.nodes[size_t(v)], path);
            if (not reached)
              return "gives a path to " + std::to_string(v) + ", which is not reached";
            if (auto e = min_path_error(c, path, u, v, got); not e.empty())
              return e;
          }
        catch (const std::domain_error &)
          {
            if (reached and not domain_error_is_right)
              return "throws domain_error for " + std::to_string(v) + ", which is reached";
          }
      }
    return "";
  }

  /// What is wrong with tree as the tree of the shortest paths whose arcs
  /// are painted: a node for each node of s, with its number, and an arc
  /// for each painted one, with its number and between the same nodes;
  /// empty if nothing
  template <class GT>
  std::string tree_error(const Spec & s, const GT & tree, const std::set<int> & painted)
  {
    std::multiset<int> nodes, want_nodes;
    for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
      nodes.insert(num(it.get_curr()));
    for (int v = 0; v < s.n; ++v)
      want_nodes.insert(v);
    if (nodes != want_nodes)
      return "the tree has " + std::to_string(nodes.size()) + " nodes, not those of the graph";
    std::multiset<std::array<int, 3>> arcs, want_arcs;
    for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr();
        arcs.insert({num(a), num(tree.get_src_node(a)), num(tree.get_tgt_node(a))});
      }
    for (int i : painted)
      want_arcs.insert({i, s.arcs[size_t(i)][0], s.arcs[size_t(i)][1]});
    if (arcs != want_arcs)
      return "the tree has " + std::to_string(arcs.size()) + " arcs, not the "
        + std::to_string(painted.size()) + " painted ones";
    return "";
  }

  /// What is wrong with the mapping that build_tree(tree) leaves in the
  /// cookies: each node and each painted arc of b and its image in tree
  /// point to each other; empty if nothing
  template <class GT>
  std::string mapping_error(const Built<GT> & b, const GT & tree, const std::set<int> & painted)
  {
    std::set<typename GT::Node *> nodes;
    for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
      nodes.insert(it.get_curr());
    std::set<typename GT::Arc *> arcs;
    for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
      arcs.insert(it.get_curr());
    for (size_t v = 0; v < b.nodes.size(); ++v)
      {
        auto * p = b.nodes[v];
        auto * q = static_cast<typename GT::Node *>(NODE_COOKIE(p));
        if (nodes.count(q) == 0 or num(q) != int(v) or NODE_COOKIE(q) != p)
          return "the node " + std::to_string(v) + " is not mapped to its image";
      }
    for (int i : painted)
      {
        auto * a = b.arcs[size_t(i)];
        auto * q = static_cast<typename GT::Arc *>(ARC_COOKIE(a));
        if (arcs.count(q) == 0 or num(q) != i or ARC_COOKIE(q) != a)
          return "the arc " + std::to_string(i) + " is not mapped to its image";
      }
    return "";
  }

  /// The checks of the tree of shortest paths that Bellman_Ford paints, and
  /// of the queries on it
  struct Tree_Checks
  {
    Check painted;
    Check state;
    Check distance;
    Check path;
    Check extract;
    Check tree;
    Check mapped;
    Check mapped_path;
    Check mapped_distance;
    Check mapped_build;
    Check unpainted;
    Check unpainted_build;
    size_t turn = 0;

    explicit Tree_Checks(const std::string & how = "")
      : painted("Bellman_Ford, the painted tree" + how),
        state("Bellman_Ford, the state after painting" + how),
        distance("Bellman_Ford::get_distance()" + how),
        path("Bellman_Ford::get_min_path()" + how),
        extract("Bellman_Ford::extract_min_spanning_tree()" + how),
        tree("Bellman_Ford::build_tree(tree, false)" + how),
        mapped("Bellman_Ford::build_tree(tree)" + how),
        mapped_path("Bellman_Ford::get_min_path(), after build_tree(tree)" + how),
        mapped_distance("Bellman_Ford::get_distance(), after build_tree(tree)" + how),
        mapped_build("Bellman_Ford::build_tree(tree), after build_tree(tree)" + how),
        unpainted("Bellman_Ford, the queries without a painted tree" + how),
        unpainted_build("Bellman_Ford::build_tree(tree, false), without a painted tree" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&painted, &state, &distance, &path, &extract, &tree, &mapped,
                              &mapped_path, &mapped_distance, &mapped_build, &unpainted,
                              &unpainted_build})
        c->expect();
    }
  };

  /// Checks that build_tree(tree, false) throws domain_error on bf, which
  /// has no painted tree, as build_tree(tree) does
  template <class GT, typename T, class SA>
  void check_unpainted_build(Check & k, BF<GT, T, SA> & bf, const Spec & s,
                             const std::string & where)
  {
    k.run();
    try
      {
        GT tree;
        bf.build_tree(tree, false);
        k.fail(s, where + "builds a tree of " + std::to_string(tree.get_num_arcs()) + " arcs");
      }
    catch (const std::domain_error &)
      {
        // there is no painted tree
      }
    catch (const std::exception & e)
      {
        k.fail(s, where + "throws " + e.what());
      }
  }

  /// Checks the queries of bf, which painted no tree from u because u
  /// reaches a negative cycle: they throw domain_error
  template <class GT, typename T, class SA>
  void check_unpainted(Tree_Checks & k, BF<GT, T, SA> & bf, const Case<GT, T> & c, int u,
                       const std::string & where)
  {
    auto * p = c.b.nodes[size_t(u)];
    k.unpainted.run();
    int thrown = 0;
    auto count = [&] (auto && call)
      {
        try
          {
            call();
          }
        catch (const std::domain_error &)
          {
            ++thrown;
          }
      };
    count([&] { bf.get_distance(p); });
    count([&] { Path<GT> path(c.b.g); bf.get_min_path(p, path); });
    count([&] { bf.extract_min_spanning_tree(); });
    count([&] { GT tree; bf.build_tree(tree); });
    if (bf.is_painted() or thrown != 4)
      k.unpainted.fail(c.s, c.label + where + std::to_string(4 - thrown)
                       + " of 4 queries do not throw domain_error");
    check_unpainted_build(k.unpainted_build, bf, c.s, c.label + where);
  }

  /// Paints the tree of the shortest paths from the first node or from the
  /// last one of c, with paint_spanning_tree() or with
  /// faster_paint_spanning_tree(), each in turn, and checks it and every
  /// query on it against the oracle, and that the queries throw without a
  /// tree
  template <class GT, typename T, class SA>
  void run_tree(Tree_Checks & k, Case<GT, T> & c, const SA & sa)
  {
    const Spec & s = c.s;
    if (s.n == 0)
      return;
    const size_t turn = k.turn++;
    const bool faster = turn % 2 == 1;
    const int u = (turn / 2) % 2 == 0 ? 0 : s.n - 1;
    const std::string where = from_text(u) + (faster ? "SPFA: " : "");
    auto * p = c.b.nodes[size_t(u)];
    BF<GT, T, SA> bf(c.b.g, c.d, sa);
    bool negative = false;
    if (not returns(k.painted, bf, c, where, [&]
          {
            negative = faster ? bf.faster_paint_spanning_tree(p) : bf.paint_spanning_tree(p);
            k.painted.run();
          }))
      return;
    if (negative != c.t.negative_from[size_t(u)])
      {
        k.painted.fail(s, c.label + where + says(negative));
        return;
      }
    if (negative)
      {
        check_unpainted(k, bf, c, u, where);
        return;
      }

    const auto painted = painted_arcs(c.b);
    if (auto e = painting_error(c, u, painted); not e.empty())
      k.painted.fail(s, c.label + where + e);

    k.state.run();
    if (not bf.is_painted() or not bf.has_computation() or bf.get_start_node() != p
        or &bf.get_graph() != &c.b.g)
      k.state.fail(s, c.label + where + "the getters do not tell the painting");

    k.distance.run();
    if (auto e = distances_error(bf, c, u); not e.empty())
      k.distance.fail(s, c.label + where + e);

    k.path.run();
    if (auto e = min_paths_error(bf, c, u); not e.empty())
      k.path.fail(s, c.label + where + e);

    k.extract.run();
    {
      const auto arcs = bf.extract_min_spanning_tree();
      std::set<int> got;
      for (size_t i = 0; i < arcs.size(); ++i)
        if (arcs(i) != nullptr)
          got.insert(num(arcs(i)));
      if (got != painted)
        k.extract.fail(s, c.label + where + "gives " + std::to_string(got.size())
                       + " arcs, not the " + std::to_string(painted.size()) + " painted ones");
    }

    // Without the mapping, the cookies keep the parents
    k.tree.run();
    {
      GT tree;
      bf.build_tree(tree, false);
      if (auto e = tree_error(s, tree, painted); not e.empty())
        k.tree.fail(s, c.label + where + e);
      else if (auto e2 = distances_error(bf, c, u); not e2.empty())
        k.tree.fail(s, c.label + where + "after it, get_distance() " + e2);
    }

    k.mapped.run();
    GT tree;
    bf.build_tree(tree);
    if (auto e = tree_error(s, tree, painted); not e.empty())
      k.mapped.fail(s, c.label + where + e);
    else if (auto e2 = mapping_error(c.b, tree, painted); not e2.empty())
      k.mapped.fail(s, c.label + where + e2);

    // Once the cookies map the tree, the queries that read the parents
    // may give the right answer or throw domain_error
    k.mapped_path.run();
    if (auto e = min_paths_error(bf, c, u, true); not e.empty())
      k.mapped_path.fail(s, c.label + where + e);

    k.mapped_distance.run();
    if (auto e = distances_error(bf, c, u, true); not e.empty())
      k.mapped_distance.fail(s, c.label + where + e);

    k.mapped_build.run();
    try
      {
        GT again;
        bf.build_tree(again);
        if (auto e = tree_error(s, again, painted); not e.empty())
          k.mapped_build.fail(s, c.label + where + e);
      }
    catch (const std::domain_error &)
      {
        // it may refuse to read the parents
      }
    catch (const std::exception & e)
      {
        k.mapped_build.fail(s, c.label + where + "throws " + e.what());
      }
  }

  /// The checks of the searches of Negative_Cycles.H
  struct Search_Checks
  {
    Check disjoint;
    Check disjoint_all;
    Check disjoint_none;
    Check bounded;
    Check bounded_none;
    Check short_cycle;
    Check short_bounds;

    explicit Search_Checks(const std::string & how = "")
      : disjoint("find_disjoint_negative_cycles(), Min_Weight_Arc" + how),
        disjoint_all("find_disjoint_negative_cycles(), All_Arcs" + how),
        disjoint_none("find_disjoint_negative_cycles(), max_cycles 0" + how),
        bounded("most_negative_cycle_bounded()" + how),
        bounded_none("most_negative_cycle_bounded(), max_length 0" + how),
        short_cycle("most_negative_cycle_up_to_3()" + how),
        short_bounds("most_negative_cycle_up_to_3(), max_length 0 and 4" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&disjoint, &disjoint_all, &disjoint_none, &bounded, &bounded_none,
                              &short_cycle, &short_bounds})
        c->expect();
    }
  };

  /// What is wrong with the cycles that find_disjoint_negative_cycles()
  /// gives with max_cycles and exclusion: at most max_cycles distinct
  /// negative simple cycles of arcs that the filter shows, with their
  /// costs and lengths, arc-disjoint with All_Arcs, some if there is a
  /// negative cycle, and, if fewer than max_cycles, none left once the
  /// arcs that the exclusion takes from each cycle are hidden; empty if
  /// nothing
  template <class GT, typename T, class Items>
  std::string disjoint_error(const Case<GT, T> & c, const Items & items, size_t max_cycles,
                             Negative_Cycle_Exclusion exclusion)
  {
    const Spec & s = c.s;
    std::vector<std::vector<int>> cycles;
    std::vector<bool> left = c.show();
    for (auto it = items.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        const Walk w = walk_of(item.cycle);
        if (auto e = closed_walk_error(s, w, c.show(), true); not e.empty())
          return e;
        if (not item.cycle.is_cycle() or item.length != w.arcs.size())
          return "the cycle " + w.str() + " has the length " + std::to_string(item.length);
        const long long cost = cost_of(s, w);
        if (cost >= 0 or (long double) item.total_cost != (long double) cost)
          return "the cycle " + w.str() + " costs " + std::to_string(cost) + ", and "
            + text_of(item.total_cost) + " is given";
        std::vector<int> arcs = w.arcs;
        std::sort(arcs.begin(), arcs.end());
        for (const auto & other : cycles)
          {
            if (other == arcs)
              return "the cycle " + w.str() + " is given twice";
            if (exclusion == Negative_Cycle_Exclusion::All_Arcs)
              for (int a : arcs)
                if (std::binary_search(other.begin(), other.end(), a))
                  return "the cycle " + w.str() + " shares the arc " + std::to_string(a);
          }
        cycles.push_back(arcs);

        // The arcs that the exclusion may have taken: all, or those of
        // least weight
        long long least = s.w[size_t(arcs[0])];
        for (int a : arcs)
          least = std::min(least, s.w[size_t(a)]);
        for (int a : arcs)
          if (exclusion == Negative_Cycle_Exclusion::All_Arcs or s.w[size_t(a)] == least)
            left[size_t(a)] = false;
      }
    if (cycles.size() > max_cycles)
      return "gives " + std::to_string(cycles.size()) + " cycles";
    if (cycles.empty() != not c.t.negative)
      return cycles.empty() ? "finds no negative cycle" : "finds a negative cycle";
    if (cycles.size() < max_cycles and has_negative_cycle(s, left))
      return "stops after " + std::to_string(cycles.size()) + " cycles, with a negative one left";
    return "";
  }

  /// What is wrong with r, the result of most_negative_cycle_bounded() with
  /// max_length len: whether there is a cycle of at most len arcs, and a
  /// simple one of arcs that the filter shows, with its cost and length,
  /// within the gap of the least cost, which the integers give exactly
  /// against the relaxed bound; empty if nothing
  template <class GT, typename T, class Result>
  std::string bounded_error(const Case<GT, T> & c, const Result & r, size_t len)
  {
    const Spec & s = c.s;
    const size_t bound = std::min(len, size_t(s.n));
    long long least = 0;
    size_t fewest = 0;
    const bool has = least_cost(c.t, bound, least, fewest);
    if (r.has_cycle != has)
      return has ? "finds no cycle" : "finds a cycle where there is none";
    if (not has)
      return r.is_negative() ? "is negative without a cycle" : "";
    const Walk w = walk_of(r.cycle_nodes, r.cycle_arcs);
    if (auto e = closed_walk_error(s, w, c.show(), true); not e.empty())
      return e;
    const long long cost = cost_of(s, w);
    if (w.arcs.size() > bound or r.length != w.arcs.size())
      return "the cycle " + w.str() + " has the length " + std::to_string(r.length);
    if ((long double) r.total_cost != (long double) cost)
      return "the cycle " + w.str() + " costs " + std::to_string(cost) + ", and "
        + text_of(r.total_cost) + " is given";
    if (r.is_negative() != (least < 0))
      return "is_negative() is wrong: the least cost is " + std::to_string(least);
    const long double gap = (long double) r.optimality_gap;
    if (gap < 0 or r.is_exact != (gap == 0) or (long double) cost - gap > (long double) least
        or (r.is_exact and cost != least))
      return "the cycle costs " + std::to_string(cost) + " with the gap "
        + text_of(r.optimality_gap) + (r.is_exact ? ", exact" : "") + ", and the least cost is "
        + std::to_string(least);
    if constexpr (std::is_integral_v<T>)
      {
        long long relaxed = 0;
        least_closed_walk(s, c.show(), bound, relaxed);
        // The gap saturates at max() if it overflows
        const long long gap_want =
          std::min<long long>(cost - relaxed, std::numeric_limits<T>::max());
        if (r.numeric_quality != Cycle_Numeric_Quality::Exact
            or r.matches_relaxed_bound != r.is_exact or (long long) r.optimality_gap != gap_want)
          return "the gap " + text_of(r.optimality_gap) + " is not the distance from "
            + std::to_string(cost) + " to the relaxed bound " + std::to_string(relaxed);
      }
    return "";
  }

  /// What is wrong with r, the result of most_negative_cycle_up_to_3() with
  /// max_length len: the cheapest simple cycle of at most len arcs, of the
  /// fewest arcs, of arcs that the filter shows; empty if nothing
  template <class GT, typename T, class Result>
  std::string short_error(const Case<GT, T> & c, const Result & r, size_t len)
  {
    const Spec & s = c.s;
    long long least = 0;
    size_t fewest = 0;
    const bool has = least_cost(c.t, len, least, fewest);
    if (r.has_cycle != has)
      return has ? "finds no cycle" : "finds a cycle where there is none";
    if (not has)
      return r.is_negative() or not r.cycle_arcs.is_empty() ? "gives a cycle" : "";
    const Walk w = walk_of(r.cycle_nodes, r.cycle_arcs);
    if (auto e = closed_walk_error(s, w, c.show(), true); not e.empty())
      return e;
    const long long cost = cost_of(s, w);
    if (cost != least or (long double) r.total_cost != (long double) least)
      return "the cycle " + w.str() + " costs " + std::to_string(cost) + ", and "
        + text_of(r.total_cost) + " is given, not " + std::to_string(least);
    if (r.length != w.arcs.size() or r.length != fewest)
      return "the cycle " + w.str() + " has the length " + std::to_string(r.length) + ", not "
        + std::to_string(fewest);
    if (r.is_negative() != (least < 0))
      return "is_negative() is wrong";
    return "";
  }

  /// Runs the searches of Negative_Cycles.H with the filter sa on c, as a
  /// const graph, against the oracle
  template <class GT, typename T, class SA>
  void run_searches(Search_Checks & k, Case<GT, T> & c, const SA & sa)
  {
    const GT & g = c.b.g;
    const Spec & s = c.s;
    for (auto exclusion : {Negative_Cycle_Exclusion::Min_Weight_Arc,
                           Negative_Cycle_Exclusion::All_Arcs})
      for (size_t max_cycles : {size_t(1), size_t(64)})
        {
          Check & check = exclusion == Negative_Cycle_Exclusion::All_Arcs ? k.disjoint_all
                                                                          : k.disjoint;
          check.run();
          const auto items = find_disjoint_negative_cycles(g, max_cycles, exclusion, c.d, sa);
          if (auto e = disjoint_error(c, items, max_cycles, exclusion); not e.empty())
            check.fail(s, c.label + "up to " + std::to_string(max_cycles) + ": " + e);
        }

    k.disjoint_none.run();
    if (not find_disjoint_negative_cycles(g, 0, Negative_Cycle_Exclusion::Min_Weight_Arc, c.d, sa)
              .is_empty())
      k.disjoint_none.fail(s, c.label + "gives cycles");

    // A bound of n + 1 is cut to n, which for 3 nodes is 3
    for (size_t len : {size_t(1), size_t(2), size_t(s.n + 1)})
      {
        k.bounded.run();
        const auto r = most_negative_cycle_bounded(g, len, c.d, sa);
        if (auto e = bounded_error(c, r, len); not e.empty())
          k.bounded.fail(s, c.label + "up to " + std::to_string(len) + " arcs: " + e);
      }

    k.bounded_none.run();
    if (const auto r = most_negative_cycle_bounded(g, 0, c.d, sa);
        r.has_cycle or r.is_negative() or not r.cycle_arcs.is_empty())
      k.bounded_none.fail(s, c.label + "gives a cycle");

    for (size_t len : {size_t(1), size_t(2), size_t(3)})
      {
        k.short_cycle.run();
        const auto r = most_negative_cycle_up_to_3(g, len, c.d, sa);
        if (auto e = short_error(c, r, len); not e.empty())
          k.short_cycle.fail(s, c.label + "up to " + std::to_string(len) + " arcs: " + e);
      }

    k.short_bounds.run();
    if (const auto r = most_negative_cycle_up_to_3(g, 0, c.d, sa); r.has_cycle)
      k.short_bounds.fail(s, c.label + "gives a cycle of no arc");
    try
      {
        most_negative_cycle_up_to_3(g, 4, c.d, sa);
        k.short_bounds.fail(s, c.label + "takes 4 arcs");
      }
    catch (const std::invalid_argument &)
      {
        // what the documentation says
      }
  }

  /// The checks of the minimum mean cycle, by Karp and by Howard
  struct Mean_Checks
  {
    Check karp;
    Check karp_value;
    Check aliases;
    Check functors;
    Check howard;
    Check howard_value;
    Check howard_limit;
    Check fallback;
    Check fallback_limit;
    size_t turn = 0;

    explicit Mean_Checks(const std::string & how = "")
      : karp("karp_minimum_mean_cycle()" + how),
        karp_value("karp_minimum_mean_cycle_value()" + how),
        aliases("minimum_mean_cycle() and minimum_mean_cycle_value()" + how),
        functors("Karp_Minimum_Mean_Cycle and Karp_Minimum_Mean_Cycle_Value" + how),
        howard("howard_minimum_mean_cycle()" + how),
        howard_value("howard_minimum_mean_cycle_value()" + how),
        howard_limit("howard_minimum_mean_cycle() and its value, without bytes" + how),
        fallback("howard_detail::minimum_mean_cycle(), without iterations" + how),
        fallback_limit("howard_detail::minimum_mean_cycle(), without iterations nor bytes" + how)
    {}

    void expect() const
    {
      for (const Check * c : {&karp, &karp_value, &aliases, &functors, &howard, &howard_value,
                              &howard_limit, &fallback, &fallback_limit})
        c->expect();
    }
  };

  /// The least mean of a cycle of c as the long double that a division of
  /// its numerator by its denominator gives, which rounds it once; infinity
  /// if there is no cycle
  template <class GT, typename T>
  long double least_mean_value(const Case<GT, T> & c, bool & has, long long & num,
                               long long & den)
  {
    num = 0;
    den = 1;
    has = least_mean(c.t, num, den);
    return has ? (long double) num / (long double) den
               : std::numeric_limits<long double>::infinity();
  }

  /// What is wrong with r, a minimum mean cycle: whether there is a cycle,
  /// the least mean num / den rounded once, and a simple cycle of arcs that
  /// the filter shows whose mean is exactly that, with its cost and length;
  /// empty if nothing
  template <class GT, typename T, class Result>
  std::string mean_error(const Case<GT, T> & c, const Result & r)
  {
    bool has = false;
    long long num = 0, den = 1;
    const long double mean = least_mean_value(c, has, num, den);
    if (r.has_cycle != has)
      return has ? "finds no cycle" : "finds a cycle where there is none";
    if (r.minimum_mean != mean)
      return "gives the mean " + text_of(r.minimum_mean) + ", not " + std::to_string(num) + "/"
        + std::to_string(den);
    if (not has)
      return r.cycle_arcs.is_empty() and r.cycle_length == 0 ? "" : "gives a cycle";
    const Walk w = walk_of(r.cycle_nodes, r.cycle_arcs);
    if (auto e = closed_walk_error(c.s, w, c.show(), true); not e.empty())
      return e;
    const long long cost = cost_of(c.s, w);
    if (r.cycle_length != w.arcs.size() or (long double) r.cycle_total_cost != (long double) cost)
      return "the cycle " + w.str() + " has the length " + std::to_string(r.cycle_length)
        + " and the cost " + text_of(r.cycle_total_cost);
    if (cost * den != num * (long long) w.arcs.size())
      return "the cycle " + w.str() + " has not the least mean " + std::to_string(num) + "/"
        + std::to_string(den);
    if (r.witness_node == nullptr)
      return "gives no witness node";
    return "";
  }

  /// What is wrong with the answers of Howard on r beyond those of Karp:
  /// its witness node starts its cycle, and the fallback and the quality
  /// agree; empty if nothing
  template <class Result>
  std::string howard_error(const Result & r)
  {
    if (r.has_cycle and r.witness_node != r.cycle_nodes.get_first())
      return "the witness node does not start the cycle";
    if (r.used_karp != (r.fallback_reason != Howard_Fallback_Reason::None))
      return "used_karp and fallback_reason disagree";
    if (r.has_cycle and (r.numeric_quality == Cycle_Numeric_Quality::Exact) == r.used_karp)
      return std::string("the quality is ")
        + (r.numeric_quality == Cycle_Numeric_Quality::Exact ? "Exact" : "Rounded")
        + (r.used_karp ? " after" : " without") + " the fallback";
    return "";
  }

  /// Whether two results of Karp are the same: whether there is a cycle,
  /// the mean, and the cycle
  template <class Result>
  bool same_cycle(const Result & a, const Result & b)
  {
    if (a.has_cycle != b.has_cycle or a.minimum_mean != b.minimum_mean
        or a.cycle_length != b.cycle_length or a.cycle_total_cost != b.cycle_total_cost)
      return false;
    auto i = a.cycle_arcs.get_it();
    auto j = b.cycle_arcs.get_it();
    for (; i.has_curr() and j.has_curr(); i.next_ne(), j.next_ne())
      if (i.get_curr() != j.get_curr())
        return false;
    return not i.has_curr() and not j.has_curr();
  }

  /// Runs Karp's and Howard's minimum mean cycle with the filter sa on c,
  /// as a const graph, against the oracle
  template <class GT, typename T, class SA>
  void run_means(Mean_Checks & k, Case<GT, T> & c, const SA & sa)
  {
    using D = Spec_Weight<GT, T>;
    const GT & g = c.b.g;
    const Spec & s = c.s;

    k.karp.run();
    const auto r = karp_minimum_mean_cycle(g, c.d, sa);
    if (auto e = mean_error(c, r); not e.empty())
      k.karp.fail(s, c.label + e);

    k.karp_value.run();
    const auto v = karp_minimum_mean_cycle_value(g, c.d, sa);
    if (v.has_cycle != r.has_cycle or v.minimum_mean != r.minimum_mean)
      k.karp_value.fail(s, c.label + "gives " + text_of(v.minimum_mean) + ", and the witness "
                        + text_of(r.minimum_mean));

    // The aliases and the functors, which wrap the functions, in turn
    if (k.turn++ % 2 == 0)
      {
        k.aliases.run();
        const auto a = minimum_mean_cycle(g, c.d, sa);
        const auto b = minimum_mean_cycle_value(g, c.d, sa);
        if (not same_cycle(a, r) or b.has_cycle != v.has_cycle or b.minimum_mean != v.minimum_mean)
          k.aliases.fail(s, c.label + "they do not give what karp_minimum_mean_cycle() gives");
      }
    else
      {
        // A functor gives the same twice
        k.functors.run();
        const Karp_Minimum_Mean_Cycle<GT, D, SA> f(c.d, sa);
      const Karp_Minimum_Mean_Cycle_Value<GT, D, SA> fv(c.d, sa);
      const auto a = f(g), b = f(g);
      const auto x = fv(g), y = fv(g);
        if (not same_cycle(a, r) or not same_cycle(b, r) or x.has_cycle != v.has_cycle
            or x.minimum_mean != v.minimum_mean or y.minimum_mean != v.minimum_mean)
          k.functors.fail(s, c.label + "they do not give what the functions give");
      }

    k.howard.run();
    const auto h = howard_minimum_mean_cycle(g, c.d, sa);
    if (auto e = mean_error(c, h); not e.empty())
      k.howard.fail(s, c.label + e);
    else if (auto e2 = howard_error(h); not e2.empty())
      k.howard.fail(s, c.label + e2);

    k.howard_value.run();
    if (const auto hv = howard_minimum_mean_cycle_value(g, c.d, sa);
        hv.has_cycle != h.has_cycle or hv.minimum_mean != h.minimum_mean)
      k.howard_value.fail(s, c.label + "gives " + text_of(hv.minimum_mean) + ", and the witness "
                          + text_of(h.minimum_mean));

    // Without bytes for the fallback, the same unless Karp has to answer
    k.howard_limit.run();
    try
      {
        const auto z = howard_minimum_mean_cycle(g, c.d, sa, 0);
        const auto zv = howard_minimum_mean_cycle_value(g, c.d, sa, 0);
        if (h.used_karp)
          k.howard_limit.fail(s, c.label + "does not throw length_error");
        else if (not same_cycle(z, h) or zv.minimum_mean != h.minimum_mean)
          k.howard_limit.fail(s, c.label + "gives another answer");
      }
    catch (const std::length_error &)
      {
        if (not h.used_karp)
          k.howard_limit.fail(s, c.label + "throws length_error without the fallback");
      }

    // Without iterations, Karp answers unless the first policy is optimal
    k.fallback.run();
    const auto f = howard_detail::minimum_mean_cycle<GT, D, SA>(g, c.d, sa, 0);
    if (auto e = mean_error(c, f); not e.empty())
      k.fallback.fail(s, c.label + e);
    else if (auto e2 = howard_error(f); not e2.empty())
      k.fallback.fail(s, c.label + e2);
    else if (f.used_karp and f.fallback_reason != Howard_Fallback_Reason::Iteration_Limit)
      k.fallback.fail(s, c.label + "the reason of the fallback is not the iteration limit");
    else if (const auto fv = howard_detail::minimum_mean_cycle<GT, D, SA, false>(g, c.d, sa, 0);
             fv.has_cycle != f.has_cycle or fv.minimum_mean != f.minimum_mean)
      k.fallback.fail(s, c.label + "without the witness it gives " + text_of(fv.minimum_mean));

    // Without bytes either, the fallback throws length_error
    k.fallback_limit.run();
    try
      {
        const auto z = howard_detail::minimum_mean_cycle<GT, D, SA>(g, c.d, sa, 0, 0);
        if (f.used_karp)
          k.fallback_limit.fail(s, c.label + "does not throw length_error");
        else if (not same_cycle(z, f))
          k.fallback_limit.fail(s, c.label + "gives another cycle");
      }
    catch (const std::length_error &)
      {
        if (not f.used_karp)
          k.fallback_limit.fail(s, c.label + "throws length_error without the fallback");
      }
  }

  /// The checks of the detection of the negative cycles by Floyd_Warshall
  /// and by Johnson
  struct All_Pairs_Checks
  {
    Check floyd;
    Check johnson;

    explicit All_Pairs_Checks(const std::string & how = "")
      : floyd("Floyd_All_Shortest_Paths::has_negative_cycle()" + how),
        johnson("Johnson, its constructor" + how)
    {}

    void expect() const
    {
      floyd.expect();
      johnson.expect();
    }
  };

  /// Runs the detection of Floyd_Warshall with the filter sa on c, against
  /// the oracle: has_negative_cycle(), get_min_path(), which throws with a
  /// negative cycle, and the constructor, which throws on an empty graph.
  /// A graph that is not directed takes each arc in both directions.
  template <class GT, typename T, class SA>
  void run_floyd(Check & k, Case<GT, T> & c, const SA & sa)
  {
    using D = Spec_Weight<GT, T>;
    const Spec & s = c.s;
    bool negative = c.t.negative;
    if (not c.b.g.is_digraph())
      oracle::shortest_distances(shown_spec(s, c.show()), false, negative);
    k.run();
    try
      {
        const Floyd_All_Shortest_Paths<GT, D, SA> f(c.b.g, c.d, sa);
        if (s.n == 0)
          k.fail(s, c.label + "takes a graph without nodes");
        else if (f.has_negative_cycle() != negative)
          k.fail(s, c.label + says(f.has_negative_cycle()));
        else if (negative)
          try
            {
              const long zero = 0;
              f.get_min_path(zero, zero);
              k.fail(s, c.label + "get_min_path() gives a path with a negative cycle");
            }
          catch (const std::domain_error &)
            {
              // there are no shortest paths
            }
      }
    catch (const std::invalid_argument &)
      {
        if (s.n != 0)
          k.fail(s, c.label + "throws invalid_argument");
      }
  }

  /// Runs the constructor of Johnson with the filter sa on c, which throws
  /// domain_error if and only if there is a negative cycle, or
  /// overflow_error where c may overflow
  template <class GT, typename T, class SA>
  void run_johnson(Check & k, Case<GT, T> & c, const SA & sa)
  {
    using D = Spec_Weight<GT, T>;
    k.run();
    try
      {
        const Johnson<GT, D, Arc_Iterator, Node_Arc_Iterator, SA> j(c.b.g, c.d, sa);
        if (c.t.negative)
          k.fail(c.s, c.label + "does not throw on a negative cycle");
        else if (not j.is_initialized())
          k.fail(c.s, c.label + "is not initialized");
      }
    catch (const std::domain_error &)
      {
        if (not c.t.negative)
          k.fail(c.s, c.label + "throws domain_error without a negative cycle");
      }
    catch (const std::overflow_error & e)
      {
        if (not c.may_overflow)
          k.fail(c.s, c.label + std::string("throws ") + e.what());
      }
    catch (const std::exception & e)
      {
        k.fail(c.s, c.label + std::string("throws ") + e.what());
      }
  }

  /// Weights near the limits of int: two of the extreme ones add up to
  /// lowest() or to max() - 3, and max() - 5 is a path of one arc
  const std::vector<long long> Limit_Weights = {std::numeric_limits<int>::lowest() / 2, -1,
                                                std::numeric_limits<int>::max() / 2 - 1,
                                                std::numeric_limits<int>::max() - 5};

  /// Weights near the limits of int for Johnson: with an arc of max() - 5
  /// into a node of potential -10, the reweighting leaves int
  const std::vector<long long> Johnson_Limit_Weights = {std::numeric_limits<int>::lowest() / 2,
                                                        -10, 1,
                                                        std::numeric_limits<int>::max() - 5};

  /// Weights near the limits of int for the queries of Johnson: two arcs
  /// of max() - 5 in a row go beyond max(), and one into a node of
  /// potential -10 has a new weight beyond max()
  const std::vector<long long> Johnson_Query_Weights = {std::numeric_limits<int>::lowest() / 2,
                                                        -10,
                                                        std::numeric_limits<int>::max() - 5};

  /// The weights that stand for minus infinity and for NaN in the specs:
  /// the failures print them, -72057594037927936 and 72057594037927936, as
  /// the weights of those arcs
  constexpr long long Minus_Infinity = -(1LL << 56);
  constexpr long long Not_A_Number = 1LL << 56;

  /// The weights of the test of the weights that are not finite
  const std::vector<long long> Infinite_Weights = {Minus_Infinity, Not_A_Number, -1, 1};

  /// The mask of the arcs of s that weigh minus infinity or NaN
  unsigned long not_finite(const Spec & s)
  {
    unsigned long mask = 0;
    for (size_t i = 0; i < s.num_arcs(); ++i)
      if (s.w[i] == Minus_Infinity or s.w[i] == Not_A_Number)
        mask |= 1UL << i;
    return mask;
  }

  /// The mask of the arcs of s that weigh NaN
  unsigned long not_a_number(const Spec & s)
  {
    unsigned long mask = 0;
    for (size_t i = 0; i < s.num_arcs(); ++i)
      if (s.w[i] == Not_A_Number)
        mask |= 1UL << i;
    return mask;
  }

  /// Whether Johnson, with integer weights and without a negative cycle,
  /// gives some arc u -> v a weight w + h(u) - h(v), or a sum w + h(u) on
  /// the way, out of the range of int; h are the potentials
  template <class GT, typename T>
  bool reweighting_overflows(const Case<GT, T> & c)
  {
    const auto h = potentials(c.t, c.s.n);
    constexpr long long lo = std::numeric_limits<int>::lowest();
    constexpr long long hi = std::numeric_limits<int>::max();
    for (size_t i = 0; i < c.s.num_arcs(); ++i)
      {
        if (not c.show()[i])
          continue;
        const long long first = c.s.w[i] + h[size_t(c.s.arcs[i][0])];
        const long long last = first - h[size_t(c.s.arcs[i][1])];
        if (first < lo or first > hi or last < lo or last > hi)
          return true;
      }
    return false;
  }

  /// Gives each arc of b its weight in s as its info, as Dft_Dist reads it,
  /// and returns the number of each arc, by its address
  template <class GT>
  std::map<typename GT::Arc *, int> weights_as_infos(Built<GT> & b, const Spec & s)
  {
    std::map<typename GT::Arc *, int> numbers;
    for (size_t i = 0; i < b.arcs.size(); ++i)
      {
        b.arcs[i]->get_info() = int(s.w[i]);
        numbers[b.arcs[i]] = int(i);
      }
    return numbers;
  }
} // namespace

template <class GT>
class ExhaustiveBellmanFord : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveBellmanFord, Directed_Kinds);

TYPED_TEST(ExhaustiveBellmanFord, DetectionMatchesTheOracle)
{
  using GT = TypeParam;
  Detection_Checks k;
  Check null_start("Bellman_Ford, the searches from a null start");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_detection(k, c, Dft_Show_Arc<GT>());
    });

  // Every search from a start rejects a null one
  for_each_spec(small_weights_domain(), true, [&] (const Spec & s)
    {
      Spec t = s;
      t.w.assign(s.num_arcs(), -1);
      Case<GT, int> c(t, shown_but(t, 0));
      BF<GT, int> bf(c.b.g, c.d);
      int thrown = 0;
      auto count = [&] (auto && call)
        {
          try
            {
              call();
            }
          catch (const std::domain_error &)
            {
              ++thrown;
            }
        };
      null_start.run();
      count([&] { bf.paint_spanning_tree(nullptr); });
      count([&] { bf.faster_paint_spanning_tree(nullptr); });
      count([&] { bf.has_negative_cycle(nullptr); });
      count([&] { bf.test_negative_cycle(nullptr); });
      count([&] { bf.search_negative_cycle(nullptr); });
      count([&] { bf.search_negative_cycle(nullptr, 0.5, 1); });
      count([&] { bf.get_distance(nullptr); });
      if (thrown != 7)
        null_start.fail(t, std::to_string(7 - thrown) + " of 7 do not throw domain_error");
    });

  k.expect();
  null_start.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, WitnessesFromAStartAreNegativeCycles)
{
  using GT = TypeParam;
  Witness_Checks k(false, "");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_witnesses(k, c, Dft_Show_Arc<GT>(), false);
    });

  k.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, WitnessesInTheWholeGraphAreNegativeCycles)
{
  using GT = TypeParam;
  Witness_Checks k(true, "");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_witnesses(k, c, Dft_Show_Arc<GT>(), true);
    });

  k.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, FunctorsMatchTheOracle)
{
  using GT = TypeParam;
  Functor_Checks k("");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_functors<Node_Arc_Iterator>(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, TheTreeOfTheShortestPathsMatchesTheOracle)
{
  using GT = TypeParam;
  Tree_Checks k;

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_tree(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, SequencesOfCallsOnOneObjectMatchTheOracle)
{
  using GT = TypeParam;
  Check repaint("Bellman_Ford, a painting after another, from another start");
  Check after_search("Bellman_Ford, a painting after each search");
  Check after_tree("Bellman_Ford, a painting after build_tree(tree)");
  Check after_clear("Bellman_Ford::clear()");
  Check fresh_build("Bellman_Ford::build_tree(tree, false), on a new object");
  Check bits("Bellman_Ford, the bits Depth_First that a search leaves");
  Check cookies("Bellman_Ford, the cookies that a search leaves");
  size_t turn = 0;

  for_each_spec(sign_domain(), true, [&] (const Spec & shape)
    {
      if (shape.n == 0)
        return;
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          Case<GT, int> c(s, shown_but(s, 0));
          BF<GT, int> bf(c.b.g, c.d);
          const int first = 0, last = s.n - 1;

          check_unpainted_build(fresh_build, bf, s, "");

          // Paints from u and compares with the oracle
          auto painting = [&] (int u, bool faster) -> std::string
            {
              auto * p = c.b.nodes[size_t(u)];
              const bool negative =
                faster ? bf.faster_paint_spanning_tree(p) : bf.paint_spanning_tree(p);
              if (negative != c.t.negative_from[size_t(u)])
                return from_text(u) + says(negative);
              if (negative)
                return bf.is_painted() ? from_text(u) + "is painted" : "";
              if (bf.get_start_node() != p)
                return from_text(u) + "has another start";
              if (auto e = distances_error(bf, c, u); not e.empty())
                return from_text(u) + e;
              return "";
            };

          repaint.run();
          for (auto [u, faster] : {std::pair{first, false}, {last, true}, {first, true},
                                  {last, false}})
            if (auto e = painting(u, faster); not e.empty())
              {
                repaint.fail(s, e);
                break;
              }

          // A search, the next one of the list at each spec, and then a
          // painting
          after_search.run();
          auto * p = c.b.nodes[size_t(first)];
          struct Search
          {
            const char * name;
            std::function<void()> call;
          };
          const Search searches[] = {
            {"has_negative_cycle(start)", [&] { bf.has_negative_cycle(p); }},
            {"test_negative_cycle(start)", [&] { bf.test_negative_cycle(p); }},
            {"search_negative_cycle(start)", [&] { bf.search_negative_cycle(p); }},
            {"search_negative_cycle(start, 0.5, 1)", [&] { bf.search_negative_cycle(p, 0.5, 1); }},
            {"has_negative_cycle()", [&] { bf.has_negative_cycle(); }},
            {"test_negative_cycle()", [&] { bf.test_negative_cycle(); }},
            {"search_negative_cycle()", [&] { bf.search_negative_cycle(); }},
            {"search_negative_cycle(0.5, 1)", [&] { bf.search_negative_cycle(0.5, 1); }},
            {"compute_nodes_weights()", [&]
              {
                try
                  {
                    bf.compute_nodes_weights();
                  }
                catch (const std::domain_error &)
                  {
                    // a negative cycle
                  }
              }}};
          {
            const Search & search = searches[turn++ % std::size(searches)];
            search.call();
            const std::set<void *> nodes(c.b.nodes.begin(), c.b.nodes.end());
            bool marked = false, outside = false;
            for (auto * q : c.b.nodes)
              {
                marked = marked or IS_NODE_VISITED(q, Aleph::Depth_First);
                outside = outside
                  or (NODE_COOKIE(q) != nullptr and nodes.count(NODE_COOKIE(q)) == 0);
              }
            bits.run();
            if (marked)
              bits.fail(s, std::string("after ") + search.name + ", a node keeps the bit");
            cookies.run();
            if (outside)
              cookies.fail(s, std::string("after ") + search.name
                           + ", a cookie points out of the graph");
            if (auto e = painting(last, false); not e.empty())
              after_search.fail(s, std::string("after ") + search.name + ", " + e);
          }

          // A painting after build_tree(tree), and build_tree(tree, false)
          // after it
          after_tree.run();
          if (not c.t.negative_from[size_t(first)] and not c.t.negative_from[size_t(last)])
            {
              bf.paint_spanning_tree(p);
              GT tree;
              bf.build_tree(tree);
              if (auto e = painting(last, false); not e.empty())
                after_tree.fail(s, e);
              else
                {
                  GT again;
                  bf.build_tree(again, false);
                  if (auto e2 = tree_error(s, again, painted_arcs(c.b)); not e2.empty())
                    after_tree.fail(s, "build_tree(tree, false): " + e2);
                }
            }

          // clear() forgets the painting and takes the bits off
          after_clear.run();
          bf.paint_spanning_tree(p);
          bf.clear();
          bool painted = false;
          for (auto * q : c.b.nodes)
            painted = painted or IS_NODE_VISITED(q, Aleph::Spanning_Tree);
          for (auto * a : c.b.arcs)
            painted = painted or IS_ARC_VISITED(a, Aleph::Spanning_Tree);
          int thrown = 0;
          try
            {
              bf.extract_min_spanning_tree();
            }
          catch (const std::domain_error &)
            {
              ++thrown;
            }
          try
            {
              bf.get_distance(p);
            }
          catch (const std::domain_error &)
            {
              ++thrown;
            }
          if (painted or bf.is_painted() or bf.has_computation()
              or bf.get_start_node() != nullptr or thrown != 2)
            after_clear.fail(s, "keeps some of the painting");
        });
    });

  repaint.expect();
  after_search.expect();
  after_tree.expect();
  after_clear.expect();
  fresh_build.expect();
  bits.expect();
  cookies.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, DetectionRespectsTheFilter)
{
  using GT = TypeParam;
  Detection_Checks k(", hiding arcs");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              Case<GT, int> c(s, shown_but(s, hidden));
              c.label = "hiding " + std::to_string(hidden) + ": ";
              run_detection(k, c, Hide_Mask<GT>{hidden});
            }
        });
    });

  k.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, WitnessesRespectTheFilter)
{
  using GT = TypeParam;
  Witness_Checks from(false, ", hiding arcs");
  Witness_Checks global(true, ", hiding arcs");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              Case<GT, int> c(s, shown_but(s, hidden));
              c.label = "hiding " + std::to_string(hidden) + ": ";
              run_witnesses(from, c, Hide_Mask<GT>{hidden}, false);
              run_witnesses(global, c, Hide_Mask<GT>{hidden}, true);
            }
        });
    });

  from.expect();
  global.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, WhitelistsOfNumbersMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", with a whitelist of numbers";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long shown = 0; shown < (1UL << s.num_arcs()); ++shown)
            {
              Case<GT, int> c(s, shown_only(s, shown));
              c.label = "showing " + std::to_string(shown) + ": ";
              const Show_Mask<GT> sa{shown};
              run_detection(k, c, sa);
              run_witnesses(from, c, sa, false);
              run_witnesses(global, c, sa, true);
            }
        });
    });

  k.expect();
  from.expect();
  global.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, WhitelistsOfPointersMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", with a whitelist of pointers";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long shown = 0; shown < (1UL << s.num_arcs()); ++shown)
            {
              Case<GT, int> c(s, shown_only(s, shown));
              c.label = "showing " + std::to_string(shown) + ": ";
              std::set<typename GT::Arc *> arcs;
              for (size_t i = 0; i < s.num_arcs(); ++i)
                if (c.show()[i])
                  arcs.insert(c.b.arcs[i]);
              const Show_Arcs<GT> sa{&arcs};
              run_detection(k, c, sa);
              run_witnesses(from, c, sa, false);
              run_witnesses(global, c, sa, true);
            }
        });
    });

  k.expect();
  from.expect();
  global.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, DoubleDistancesMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", with double distances";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);
  Tree_Checks tree(how);

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Unit_Weights, [&] (const Spec & s)
        {
          Case<GT, double> c(s, shown_but(s, 0));
          run_detection(k, c, Dft_Show_Arc<GT>());
          run_witnesses(from, c, Dft_Show_Arc<GT>(), false);
          run_witnesses(global, c, Dft_Show_Arc<GT>(), true);
          run_tree(tree, c, Dft_Show_Arc<GT>());
        });
    });

  k.expect();
  from.expect();
  global.expect();
  tree.expect();
}

namespace
{
  /// Calls f(s, overflow) with every spec of the smaller domain whose
  /// weights are near the limits of int, as Limit_Weights says, with
  /// overflow if some sum of Bellman_Ford may leave int, or reach the max()
  /// that stands for no distance
  template <class F>
  void for_each_spec_near_the_limits(F && f)
  {
    for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
      {
        for_each_weighting(shape, Limit_Weights, [&] (const Spec & s)
          {
            f(s, sums_may_overflow(s, shown_but(s, 0), std::numeric_limits<int>::lowest(),
                                   std::numeric_limits<int>::max() - 1));
          });
      });
  }
}

TYPED_TEST(ExhaustiveBellmanFord, IntegersNearTheLimitsMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", near the limits of int";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);

  // Where a sum may leave int, overflow_error is also right
  for_each_spec_near_the_limits([&] (const Spec & s, bool overflow)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      c.may_overflow = overflow;
      run_detection(k, c, Dft_Show_Arc<GT>());
      run_witnesses(from, c, Dft_Show_Arc<GT>(), false);
      run_witnesses(global, c, Dft_Show_Arc<GT>(), true);
    });

  k.expect();
  from.expect();
  global.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, TheTreeNearTheLimitsMatchesTheOracle)
{
  using GT = TypeParam;
  Tree_Checks k(", near the limits of int");

  // Where a sum may leave int, the painting may throw overflow_error
  for_each_spec_near_the_limits([&] (const Spec & s, bool overflow)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      c.may_overflow = overflow;
      run_tree(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, TheQueriesOnPathsNearTheLimitsOfInt)
{
  using GT = TypeParam;
  const std::string how = ", on a path near the limits of int";
  Check painted("Bellman_Ford::paint_spanning_tree()" + how);
  // A sum of the weights from the end of the path may leave int although
  // the distance fits; get_distance() and get_min_path() add them from the
  // start, so that each sum is the distance of a node of the path
  Check distance("Bellman_Ford::get_distance()" + how);
  Check path("Bellman_Ford::get_min_path()" + how);
  size_t sums_from_the_end_leave_int = 0;
  constexpr long long lo = std::numeric_limits<int>::lowest();
  constexpr long long hi = std::numeric_limits<int>::max();

  // The path 0 -> 1 -> 2 -> 3, with its arcs inserted in both orders
  for (bool reversed : {false, true})
    {
      Spec shape;
      shape.n = 4;
      shape.arcs = {{0, 1}, {1, 2}, {2, 3}};
      if (reversed)
        std::reverse(shape.arcs.begin(), shape.arcs.end());
      for_each_weighting(shape, Limit_Weights, [&] (const Spec & s)
        {
          // A distance may leave int, and then the painting throws
          Case<GT, int> c(s, shown_but(s, 0));
          c.may_overflow = true;
          BF<GT, int> bf(c.b.g, c.d);
          bool negative = false;
          if (not returns(painted, bf, c, "", [&]
                {
                  negative = bf.paint_spanning_tree(c.b.nodes[0]);
                  painted.run();
                }))
            return;
          if (negative)
            {
              painted.fail(s, "finds a negative cycle");
              return;
            }

          // The weight of the arc from v - 1 to v
          auto weight = [&] (int v)
            {
              for (size_t i = 0; i < s.num_arcs(); ++i)
                if (s.arcs[i][1] == v)
                  return s.w[i];
              return 0LL;
            };
          for (int v = 1; v < s.n; ++v)
            {
              // Whether a sum of the weights from v back to 0 leaves int
              bool back = false;
              long long sum = 0;
              for (int x = v; x > 0; --x)
                {
                  sum += weight(x);
                  back = back or sum < lo or sum > hi;
                }
              const long long want = c.t.dist[0][size_t(v)];
              sums_from_the_end_leave_int += back;
              distance.run();
              try
                {
                  if (const int got = bf.get_distance(c.b.nodes[size_t(v)]); got != want)
                    distance.fail(s, "gives " + std::to_string(got) + " to " + std::to_string(v)
                                  + ", not " + std::to_string(want));
                }
              catch (const std::overflow_error & e)
                {
                  distance.fail(s, "to " + std::to_string(v) + ": throws " + e.what());
                }
              path.run();
              Path<GT> p(c.b.g);
              const int got = bf.get_min_path(c.b.nodes[size_t(v)], p);
              if (auto e = min_path_error(c, p, 0, v, got); not e.empty())
                path.fail(s, e);
            }
        });
    }

  painted.expect();
  distance.expect();
  path.expect();
  EXPECT_GT(sums_from_the_end_leave_int, 0u);
}

namespace
{
  /// Calls f(s, c) with every spec of the smaller domain with the weights
  /// of Infinite_Weights, and c, a case of s with the weights of minus
  /// infinity and NaN in place of those that stand for them, whose oracle
  /// takes the arcs that show says
  template <class GT, class Show, class F>
  void for_each_infinite_case(Show && show, F && f)
  {
    for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
      {
        for_each_weighting(shape, Infinite_Weights, [&] (const Spec & s)
          {
            Case<GT, double> c(s, show(s));
            for (size_t i = 0; i < s.num_arcs(); ++i)
              if (s.w[i] == Minus_Infinity)
                c.w[i] = -std::numeric_limits<double>::infinity();
              else if (s.w[i] == Not_A_Number)
                c.w[i] = std::numeric_limits<double>::quiet_NaN();
            f(s, c);
          });
      });
  }
}

TYPED_TEST(ExhaustiveBellmanFord, WeightsThatAreNotFiniteAreRejectedOrCounted)
{
  using GT = TypeParam;
  const std::string how = ", with weights of minus infinity or NaN";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);
  Tree_Checks tree(how);

  // An operation whose last pass meets a sum of minus infinity or NaN
  // throws domain_error; another one, that stops before, gives the answer
  // of the oracle, where minus infinity is below any finite cost and an arc
  // of NaN, which never relaxes, does not count
  for_each_infinite_case<GT>([] (const Spec & s) { return shown_but(s, not_a_number(s)); },
                             [&] (const Spec & s, Case<GT, double> & c)
    {
      c.may_reject = not_finite(s) != 0;
      c.filtered = shown_but(s, 0);
      run_detection(k, c, Dft_Show_Arc<GT>());
      run_witnesses(from, c, Dft_Show_Arc<GT>(), false);
      run_witnesses(global, c, Dft_Show_Arc<GT>(), true);
      run_tree(tree, c, Dft_Show_Arc<GT>());
    });

  k.expect();
  from.expect();
  global.expect();
  tree.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, WeightsThatAreNotFiniteDoNotCountWhenHidden)
{
  using GT = TypeParam;
  const std::string how = ", with weights of minus infinity or NaN hidden by the filter";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);

  for_each_infinite_case<GT>([] (const Spec & s) { return shown_but(s, not_finite(s)); },
                             [&] (const Spec & s, Case<GT, double> & c)
    {
      const unsigned long bad = not_finite(s);
      if (bad == 0)
        return;
      c.label = "hiding " + std::to_string(bad) + ": ";
      const Hide_Mask<GT> sa{bad};
      run_detection(k, c, sa);
      run_witnesses(from, c, sa, false);
      run_witnesses(global, c, sa, true);
    });

  k.expect();
  from.expect();
  global.expect();
}

TYPED_TEST(ExhaustiveBellmanFord, RandomDigraphsMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", on random digraphs";
  Detection_Checks k(how);
  Witness_Checks from(false, how);
  Witness_Checks global(true, how);
  Tree_Checks tree(how);

  std::mt19937 rng(seed(73));
  for (int round = 0; round < (heavy() ? 400 : 40); ++round)
    {
      const int n = 6 + int(rng() % 3);
      Spec s = random_spec(rng, n, n + int(rng() % size_t(n + 1)), true);
      random_weights(rng, s, -1 - (long long) (rng() % 4), 6);
      Case<GT, int> c(s, shown_but(s, 0));
      run_detection(k, c, Dft_Show_Arc<GT>());
      run_tree(tree, c, Dft_Show_Arc<GT>());
      run_witnesses(from, c, Dft_Show_Arc<GT>(), false);
      run_witnesses(global, c, Dft_Show_Arc<GT>(), true);
    }

  k.expect();
  from.expect();
  global.expect();
  tree.expect();
}

template <class GT>
class ExhaustiveNegativeCycleSearches : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveNegativeCycleSearches, Directed_Kinds);

TYPED_TEST(ExhaustiveNegativeCycleSearches, SearchesMatchTheOracle)
{
  using GT = TypeParam;
  Search_Checks k;

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_searches(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, SearchesRespectTheFilter)
{
  using GT = TypeParam;
  Search_Checks k(", hiding arcs");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              Case<GT, int> c(s, shown_but(s, hidden));
              c.label = "hiding " + std::to_string(hidden) + ": ";
              run_searches(k, c, Hide_Mask<GT>{hidden});
            }
        });
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, SearchesWithDoubleCostsMatchTheOracle)
{
  using GT = TypeParam;
  Search_Checks k(", with double costs");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, double> c(s, shown_but(s, 0));
      run_searches(k, c, Dft_Show_Arc<GT>());
    }, Unit_Weights);

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, MeansMatchTheOracle)
{
  using GT = TypeParam;
  Mean_Checks k;

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_means(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, MeansRespectTheFilter)
{
  using GT = TypeParam;
  Mean_Checks k(", hiding arcs");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              Case<GT, int> c(s, shown_but(s, hidden));
              c.label = "hiding " + std::to_string(hidden) + ": ";
              run_means(k, c, Hide_Mask<GT>{hidden});
            }
        });
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, MeansWithDoubleCostsMatchTheOracle)
{
  using GT = TypeParam;
  Mean_Checks k(", with double costs");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, double> c(s, shown_but(s, 0));
      run_means(k, c, Dft_Show_Arc<GT>());
    }, Unit_Weights);

  k.expect();
}

namespace
{
  /// Runs one of the detections run_floyd() and run_johnson() on every
  /// weighted spec, with integer costs and with double ones, and hiding
  /// each set of arcs
  template <class GT, class Run>
  void check_all_pairs_detection(Check & ints, Check & doubles, Check & hiding, Run && run)
  {
    for_each_weighted_spec([&] (const Spec & s)
      {
        Case<GT, int> c(s, shown_but(s, 0));
        run(ints, c, Dft_Show_Arc<GT>());
      });

    for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
      {
        for_each_weighting(shape, Unit_Weights, [&] (const Spec & s)
          {
            Case<GT, double> c(s, shown_but(s, 0));
            run(doubles, c, Dft_Show_Arc<GT>());
          });
        for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
          {
            for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
              {
                Case<GT, int> c(s, shown_but(s, hidden));
                c.label = "hiding " + std::to_string(hidden) + ": ";
                run(hiding, c, Hide_Mask<GT>{hidden});
              }
          });
      });
  }
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, FloydDetectsTheNegativeCycles)
{
  using GT = TypeParam;
  const std::string name = "Floyd_All_Shortest_Paths::has_negative_cycle()";
  Check ints(name);
  Check doubles(name + ", with double costs");
  Check hiding(name + ", hiding arcs");

  check_all_pairs_detection<GT>(ints, doubles, hiding, [] (Check & k, auto & c, const auto & sa)
    {
      run_floyd(k, c, sa);
    });

  ints.expect();
  doubles.expect();
  hiding.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, JohnsonDetectsTheNegativeCycles)
{
  using GT = TypeParam;
  const std::string name = "Johnson, its constructor";
  Check ints(name);
  Check doubles(name + ", with double costs");
  Check hiding(name + ", hiding arcs");

  check_all_pairs_detection<GT>(ints, doubles, hiding, [] (Check & k, auto & c, const auto & sa)
    {
      run_johnson(k, c, sa);
    });

  ints.expect();
  doubles.expect();
  hiding.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, FloydAndJohnsonWithAWhitelistMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", with a whitelist of numbers";
  Check floyd("Floyd_All_Shortest_Paths::has_negative_cycle()" + how);
  Check johnson("Johnson, its constructor" + how);

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long shown = 0; shown < (1UL << s.num_arcs()); ++shown)
            {
              Case<GT, int> c(s, shown_only(s, shown));
              c.label = "showing " + std::to_string(shown) + ": ";
              const Show_Mask<GT> sa{shown};
              run_floyd(floyd, c, sa);
              run_johnson(johnson, c, sa);
            }
        });
    });

  floyd.expect();
  johnson.expect();
}

namespace
{
  /// Whether every simple cycle of c of the least mean costs a sum that
  /// fits in int, as the witnesses of Karp and Howard must
  template <class GT, typename T>
  bool least_means_fit(const Case<GT, T> & c)
  {
    long long num = 0, den = 1;
    if (not least_mean(c.t, num, den))
      return true;
    for (const auto & cy : c.t.cycles)
      if (cy.cost * den == num * (long long) cy.arcs.size()
          and (cy.cost < std::numeric_limits<int>::lowest()
               or cy.cost > std::numeric_limits<int>::max()))
        return false;
    return true;
  }

  /// Calls f(), which counts a run of k and checks the answer, and fails k
  /// if f() throws overflow_error, unless that is allowed
  template <class F>
  void unless_it_overflows(Check & k, const Spec & s, bool allowed, const std::string & what,
                           F && f)
  {
    try
      {
        f();
      }
    catch (const std::overflow_error & e)
      {
        if (not allowed)
          k.fail(s, what + ": throws " + e.what());
      }
  }
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, SearchesAndMeansNearTheLimitsOfInt)
{
  using GT = TypeParam;
  const std::string how = ", near the limits of int";
  Check disjoint("find_disjoint_negative_cycles()" + how);
  Check bounded("most_negative_cycle_bounded()" + how);
  Check short_cycle("most_negative_cycle_up_to_3()" + how);
  Check karp("karp_minimum_mean_cycle() and its value" + how);
  Check howard("howard_minimum_mean_cycle() and its value" + how);

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Johnson_Limit_Weights, [&] (const Spec & s)
        {
          Case<GT, int> c(s, shown_but(s, 0));
          const GT & g = c.b.g;
          // The sums of the relaxations may leave int, which they say with
          // overflow_error
          const bool may_overflow = sums_may_overflow(s, c.show(),
                                                      std::numeric_limits<int>::lowest(),
                                                      std::numeric_limits<int>::max());
          unless_it_overflows(disjoint, s, may_overflow, "up to 64", [&]
            {
              disjoint.run();
              const auto items =
                find_disjoint_negative_cycles(g, 64, Negative_Cycle_Exclusion::Min_Weight_Arc, c.d);
              if (auto e = disjoint_error(c, items, 64, Negative_Cycle_Exclusion::Min_Weight_Arc);
                  not e.empty())
                disjoint.fail(s, e);
            });

          for (size_t len : {size_t(1), size_t(2), size_t(3)})
            {
              const std::string up_to = "up to " + std::to_string(len) + " arcs";
              const bool walks_overflow =
                walks_may_overflow(s, c.show(), len, std::numeric_limits<int>::lowest(),
                                   std::numeric_limits<int>::max());
              unless_it_overflows(bounded, s, walks_overflow, up_to, [&]
                {
                  bounded.run();
                  if (auto e = bounded_error(c, most_negative_cycle_bounded(g, len, c.d), len);
                      not e.empty())
                    bounded.fail(s, up_to + ": " + e);
                });

              // Only the total of the best cycle has to fit
              long long least = 0;
              size_t fewest = 0;
              const bool has = least_cost(c.t, len, least, fewest);
              const bool out = has and (least < std::numeric_limits<int>::lowest()
                                        or least > std::numeric_limits<int>::max());
              unless_it_overflows(short_cycle, s, out, up_to, [&]
                {
                  short_cycle.run();
                  const auto r = most_negative_cycle_up_to_3(g, len, c.d);
                  if (out)
                    short_cycle.fail(s, up_to + ": gives a total out of int");
                  else if (auto e = short_error(c, r, len); not e.empty())
                    short_cycle.fail(s, up_to + ": " + e);
                });
            }

          // A witness has to fit, and the values never overflow
          const bool fit = least_means_fit(c);
          unless_it_overflows(karp, s, not fit, "karp_minimum_mean_cycle()", [&]
            {
              karp.run();
              if (auto e = mean_error(c, karp_minimum_mean_cycle(g, c.d)); not e.empty())
                karp.fail(s, e);
            });
          unless_it_overflows(karp, s, false, "karp_minimum_mean_cycle_value()", [&]
            {
              karp.run();
              const auto r = karp_minimum_mean_cycle_value(g, c.d);
              bool has = false;
              long long num = 0, den = 1;
              const long double mean = least_mean_value(c, has, num, den);
              if (r.has_cycle != has or r.minimum_mean != mean)
                karp.fail(s, "the value gives " + text_of(r.minimum_mean));
            });
          unless_it_overflows(howard, s, not fit, "howard_minimum_mean_cycle()", [&]
            {
              howard.run();
              const auto r = howard_minimum_mean_cycle(g, c.d);
              if (auto e = mean_error(c, r); not e.empty())
                howard.fail(s, e);
              else if (auto e2 = howard_error(r); not e2.empty())
                howard.fail(s, e2);
            });
          unless_it_overflows(howard, s, false, "howard_minimum_mean_cycle_value()", [&]
            {
              howard.run();
              const auto r = howard_minimum_mean_cycle_value(g, c.d);
              bool has = false;
              long long num = 0, den = 1;
              const long double mean = least_mean_value(c, has, num, den);
              if (r.has_cycle != has or r.minimum_mean != mean)
                howard.fail(s, "the value gives " + text_of(r.minimum_mean));
            });
        });
    });

  disjoint.expect();
  bounded.expect();
  short_cycle.expect();
  karp.expect();
  howard.expect();
}

namespace
{
  /// The checks of the answers of Johnson on graphs without a negative
  /// cycle: its queries, its potentials and its copy of the graph, against
  /// the oracle, and the graph that it takes, which it must not change
  struct Johnson_Answer_Checks
  {
    Check build;
    Check distance;
    Check path;
    Check all_pairs;
    Check potential;
    Check copy;
    Check graph;

    explicit Johnson_Answer_Checks(const std::string & how)
      : build("Johnson, its constructor" + how),
        distance("Johnson::get_distance()" + how),
        path("Johnson::find_path()" + how),
        all_pairs("Johnson::compute_all_pairs_distances()" + how),
        potential("Johnson::get_potential()" + how),
        copy("Johnson::get_reweighted_graph() and get_copy_node()" + how),
        graph("Johnson, the graph that it takes" + how)
    {}

    void expect() const
    {
      for (const Check * k : {&build, &distance, &path, &all_pairs, &potential, &copy, &graph})
        k->expect();
    }
  };

  /// The infos, the control bits, the counters and the cookies of the
  /// nodes and of the arcs of a built graph
  template <class GT>
  struct Attributes
  {
    using Of = std::tuple<int, std::vector<bool>, long, void *>;

    std::vector<Of> nodes;
    std::vector<Of> arcs;

    explicit Attributes(const Built<GT> & b)
    {
      auto bits = [] (const Bit_Fields & f)
        {
          std::vector<bool> r;
          for (int i = 0; i < Num_Bits_Graph; ++i)
            r.push_back(f.get_bit(i));
          return r;
        };
      for (auto * p : b.nodes)
        nodes.emplace_back(num(p), bits(NODE_BITS(p)), NODE_COUNTER(p), NODE_COOKIE(p));
      for (auto * a : b.arcs)
        arcs.emplace_back(num(a), bits(ARC_BITS(a)), ARC_COUNTER(a), ARC_COOKIE(a));
    }

    bool operator == (const Attributes &) const = default;
  };

  /// The distance that Johnson gives to a node that is not reached
  template <typename T>
  T unreached_by_johnson()
  {
    if constexpr (std::numeric_limits<T>::has_infinity)
      return std::numeric_limits<T>::infinity();
    else
      return std::numeric_limits<T>::max();
  }

  /// What is wrong with path, which find_path() gave, as a shortest path
  /// of c from u to v along arcs that the filter shows, where the oracle
  /// says that u reaches v; empty if nothing
  template <class GT, typename T>
  std::string johnson_path_error(const Case<GT, T> & c, const Path<GT> & path, int u, int v)
  {
    const Walk w = walk_of(path);
    if (auto e = walk_error(c.s, w, c.show()); not e.empty())
      return e;
    if (w.nodes.front() != u or w.nodes.back() != v)
      return "the path " + w.str() + " does not go from " + std::to_string(u) + " to "
        + std::to_string(v);
    if (const long long want = c.t.dist[size_t(u)][size_t(v)]; cost_of(c.s, w) != want)
      return "the path " + w.str() + " costs " + std::to_string(cost_of(c.s, w)) + ", not "
        + std::to_string(want);
    return "";
  }

  /// Calls f(), which tells what is wrong, as a run of k: a failure if it
  /// says something or throws
  template <class F>
  void guarded_query(Check & k, const Spec & s, const std::string & where, F && f)
  {
    k.run();
    try
      {
        if (const std::string e = f(); not e.empty())
          k.fail(s, where + e);
      }
    catch (const std::exception & e)
      {
        k.fail(s, where + "throws " + e.what());
      }
  }

  /// Builds a Johnson of c with the filter sa and checks its answers
  /// against the oracle. The weights of c are those of its spec times
  /// scale, so that one half gives potentials and new weights with a
  /// fraction; c has no negative cycle on the arcs that sa shows.
  template <class GT, typename T, class SA>
  void check_johnson_answers(Johnson_Answer_Checks & k, Case<GT, T> & c, const SA & sa,
                             T scale = T(1))
  {
    using Solver = Johnson<GT, Spec_Weight<GT, T>, Arc_Iterator, Node_Arc_Iterator, SA>;
    const Spec & s = c.s;
    const Attributes<GT> before(c.b);
    auto want = [&] (int u, int v)
      {
        const long long x = c.t.dist[size_t(u)][size_t(v)];
        return x == oracle::Inf ? unreached_by_johnson<T>() : T(x) * scale;
      };

    k.build.run();
    std::unique_ptr<Solver> j;
    try
      {
        j = std::make_unique<Solver>(c.b.g, c.d, sa);
      }
    catch (const std::exception & e)
      {
        k.build.fail(s, c.label + "throws " + e.what());
        return;
      }

    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          auto * p = c.b.nodes[size_t(u)];
          auto * q = c.b.nodes[size_t(v)];
          const std::string where = c.label + "from " + std::to_string(u) + " to "
            + std::to_string(v) + ": ";
          guarded_query(k.distance, s, where, [&]
            {
              const T got = j->get_distance(p, q);
              return got == want(u, v) ? std::string()
                : "gives " + text_of(got) + ", not " + text_of(want(u, v));
            });
          guarded_query(k.path, s, where, [&]
            {
              const Path<GT> path = j->find_path(p, q);
              if (c.t.dist[size_t(u)][size_t(v)] != oracle::Inf)
                return johnson_path_error(c, path, u, v);
              return path.is_empty() ? std::string() : std::string("gives a path to a node not reached");
            });
        }

    guarded_query(k.all_pairs, s, c.label, [&]
      {
        const auto all = j->compute_all_pairs_distances();
        if (all.size() != size_t(s.n) * size_t(s.n))
          return std::string("lacks pairs");
        for (int u = 0; u < s.n; ++u)
          for (int v = 0; v < s.n; ++v)
            {
              const auto * x = all.search({c.b.nodes[size_t(u)], c.b.nodes[size_t(v)]});
              if (x == nullptr or x->second != want(u, v))
                return "from " + std::to_string(u) + " to " + std::to_string(v) + " gives "
                  + (x == nullptr ? std::string("nothing") : text_of(x->second));
            }
        return std::string();
      });

    const auto h = potentials(c.t, s.n);
    guarded_query(k.potential, s, c.label, [&]
      {
        for (int v = 0; v < s.n; ++v)
          if (const T got = j->get_potential(c.b.nodes[size_t(v)]);
              got != T(h[size_t(v)]) * scale)
            return "of " + std::to_string(v) + " gives " + text_of(got);
        return std::string();
      });

    // Every arc is copied: one that the filter shows with its new weight,
    // converted to the type of the info, and one that it hides with its
    // number
    guarded_query(k.copy, s, c.label, [&]
      {
        const GT & r = std::as_const(*j).get_reweighted_graph();
        std::multiset<std::tuple<int, int, int>> wanted, got;
        for (size_t i = 0; i < s.num_arcs(); ++i)
          {
            const int x = s.arcs[i][0], y = s.arcs[i][1];
            const T w = (T(s.w[i]) + T(h[size_t(x)]) - T(h[size_t(y)])) * scale;
            wanted.insert({x, y, c.show()[i] ? static_cast<int>(w) : int(i)});
          }
        for (auto it = r.get_arc_it(); it.has_curr(); it.next_ne())
          {
            auto * a = it.get_curr_ne();
            got.insert({num(r.get_src_node(a)), num(r.get_tgt_node(a)), a->get_info()});
          }
        if (j->get_reweighted_graph().get_num_nodes() != size_t(s.n) or got != wanted)
          return std::string("the copy is not the graph with the new weights");
        for (int v = 0; v < s.n; ++v)
          if (auto * p = j->get_copy_node(c.b.nodes[size_t(v)]); p == nullptr or num(p) != v)
            return "the copy of " + std::to_string(v) + " is another node";
        return std::string();
      });

    k.graph.run();
    if (not (Attributes<GT>(c.b) == before))
      k.graph.fail(s, c.label + "the graph changed");
  }

  /// The checks of the queries of Johnson near the limits of int, and of
  /// its copy of the graph, whose infos hold the new weights, or max() for
  /// one beyond max()
  struct Johnson_Limit_Checks
  {
    Check distance;
    Check path;
    Check all_pairs;
    Check copy;
    size_t saturated = 0; ///< Cases with a new weight beyond max()

    explicit Johnson_Limit_Checks(const std::string & how)
      : distance("Johnson::get_distance()" + how),
        path("Johnson::find_path()" + how),
        all_pairs("Johnson::compute_all_pairs_distances()" + how),
        copy("Johnson::get_reweighted_graph()" + how)
    {}

    void expect() const
    {
      distance.expect();
      path.expect();
      all_pairs.expect();
      copy.expect();
    }
  };

  /// Checks the queries of a Johnson of c, which has no negative cycle,
  /// near the limits of int, unless its constructor throws overflow_error,
  /// which run_johnson() judges: each distance that fits in int is given,
  /// as its path, and only one that does not throws overflow_error, as the
  /// map of all of them. Returns whether some distance does not fit.
  template <class GT>
  bool check_johnson_queries_near_the_limits(Johnson_Limit_Checks & k, Case<GT, int> & c)
  {
    using Solver = Johnson<GT, Spec_Weight<GT, int>>;
    constexpr long long lo = std::numeric_limits<int>::lowest();
    constexpr long long hi = std::numeric_limits<int>::max();
    const Spec & s = c.s;
    std::unique_ptr<Solver> j;
    try
      {
        j = std::make_unique<Solver>(c.b.g, c.d);
      }
    catch (const std::overflow_error &)
      {
        return false;
      }

    auto fits = [&] (int u, int v)
      {
        const long long d = c.t.dist[size_t(u)][size_t(v)];
        return d == oracle::Inf or (d >= lo and d <= hi);
      };
    auto want = [&] (int u, int v)
      {
        const long long d = c.t.dist[size_t(u)][size_t(v)];
        return d == oracle::Inf ? std::numeric_limits<int>::max() : int(d);
      };

    bool all_fit = true;
    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          all_fit = all_fit and fits(u, v);
          auto * p = c.b.nodes[size_t(u)];
          auto * q = c.b.nodes[size_t(v)];
          const std::string where = "from " + std::to_string(u) + " to " + std::to_string(v)
            + ": ";
          k.distance.run();
          try
            {
              const int got = j->get_distance(p, q);
              if (not fits(u, v) or got != want(u, v))
                k.distance.fail(s, where + "gives " + std::to_string(got));
            }
          catch (const std::overflow_error & e)
            {
              if (fits(u, v))
                k.distance.fail(s, where + "throws " + e.what());
            }

          k.path.run();
          try
            {
              const Path<GT> path = j->find_path(p, q);
              if (not fits(u, v))
                k.path.fail(s, where + "gives a path of a distance beyond int");
              else if (c.t.dist[size_t(u)][size_t(v)] == oracle::Inf)
                {
                  if (not path.is_empty())
                    k.path.fail(s, where + "gives a path to a node not reached");
                }
              else if (auto e = johnson_path_error(c, path, u, v); not e.empty())
                k.path.fail(s, where + e);
            }
          catch (const std::overflow_error & e)
            {
              if (fits(u, v))
                k.path.fail(s, where + "throws " + e.what());
            }
        }

    k.all_pairs.run();
    try
      {
        const auto all = j->compute_all_pairs_distances();
        if (not all_fit)
          k.all_pairs.fail(s, "gives distances beyond int");
        else
          for (int u = 0; u < s.n; ++u)
            for (int v = 0; v < s.n; ++v)
              if (const auto * x = all.search({c.b.nodes[size_t(u)], c.b.nodes[size_t(v)]});
                  x == nullptr or x->second != want(u, v))
                k.all_pairs.fail(s, "from " + std::to_string(u) + " to " + std::to_string(v)
                                 + ": a wrong distance");
      }
    catch (const std::overflow_error & e)
      {
        if (all_fit)
          k.all_pairs.fail(s, std::string("throws ") + e.what());
      }

    k.copy.run();
    const auto h = potentials(c.t, s.n);
    std::multiset<std::tuple<int, int, long long>> wanted, got;
    bool saturated = false;
    for (size_t i = 0; i < s.num_arcs(); ++i)
      {
        const int x = s.arcs[i][0], y = s.arcs[i][1];
        const long long w = s.w[i] + h[size_t(x)] - h[size_t(y)];
        saturated = saturated or w > hi;
        wanted.insert({x, y, std::min(w, hi)});
      }
    k.saturated += saturated;
    const GT & r = j->get_reweighted_graph();
    for (auto it = r.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr_ne();
        got.insert({num(r.get_src_node(a)), num(r.get_tgt_node(a)), (long long) a->get_info()});
      }
    if (got != wanted)
      k.copy.fail(s, "the copy is not the graph with the new weights");

    return not all_fit;
  }
} // namespace

TYPED_TEST(ExhaustiveNegativeCycleSearches, FloydAndJohnsonNearTheLimitsOfInt)
{
  using GT = TypeParam;
  const std::string how = ", near the limits of int";
  Check floyd("Floyd_All_Shortest_Paths::has_negative_cycle()" + how);
  Check johnson("Johnson, its constructor" + how);
  // A class of its own, which the harness found: a new weight w + h(u) -
  // h(v), or the sum w + h(u) on the way, out of the range of int although
  // every distance fits, which threw overflow_error while Johnson kept
  // them in int
  Check reweighting("Johnson, its constructor" + how + ", reweighting beyond int");
  size_t reweighted_beyond = 0;

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Johnson_Limit_Weights, [&] (const Spec & s)
        {
          Case<GT, int> c(s, shown_but(s, 0));

          // Floyd may throw overflow_error only with a weight below
          // lowest() / (2 (n - 1)), as it documents
          try
            {
              run_floyd(floyd, c, Dft_Show_Arc<GT>());
            }
          catch (const std::overflow_error &)
            {
              const long long least = *std::min_element(s.w.begin(), s.w.end());
              if (s.n < 2 or least >= std::numeric_limits<int>::lowest() / (2 * (s.n - 1)))
                floyd.fail(s, "throws overflow_error");
            }

          // Where a sum of Bellman-Ford may leave int, overflow_error is
          // right too
          c.may_overflow = sums_may_overflow(s, c.show(), std::numeric_limits<int>::lowest(),
                                             std::numeric_limits<int>::max() - 1);
          const bool reweighted = not c.may_overflow and not c.t.negative
            and reweighting_overflows(c);
          reweighted_beyond += reweighted;
          run_johnson(reweighted ? reweighting : johnson, c, Dft_Show_Arc<GT>());
        });
    });

  floyd.expect();
  johnson.expect();
  reweighting.expect();
  EXPECT_GT(reweighted_beyond, 0U);
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, JohnsonQueriesNearTheLimitsOfInt)
{
  using GT = TypeParam;
  // Two classes of their own, which the harness found: a distance beyond
  // max(), which made every query from its source throw while Johnson
  // painted its whole tree in int; and a sum of Dijkstra beyond max() on
  // the way to a node whose distance fits
  Johnson_Limit_Checks queries(", near the limits of int");
  size_t beyond = 0;

  for_each_spec(sign_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Johnson_Query_Weights, [&] (const Spec & s)
        {
          Case<GT, int> c(s, shown_but(s, 0));
          if (not c.t.negative and check_johnson_queries_near_the_limits(queries, c))
            ++beyond;
        });
    });

  queries.expect();
  EXPECT_GT(beyond, 0U);
  EXPECT_GT(queries.saturated, 0U);
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, JohnsonAnswersOnTheArcsThatTheFilterShows)
{
  using GT = TypeParam;
  // Two classes of their own, which the harness found: a filter that
  // reads the numbers in the infos of the arcs, which once read the new
  // weights in the infos of the copies of the arcs, and a filter of
  // pointers to the arcs, which once got the copies; and parallel arcs,
  // one hidden, whose copies were paired with the arcs by their ends, so
  // that in List_SDigraph, whose arcs go in the order of their addresses,
  // the shown one could take the weight of the hidden one
  Johnson_Answer_Checks numbers(", with a whitelist of numbers");
  Johnson_Answer_Checks pointers(", with a whitelist of pointers");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long shown = 0; shown < (1UL << s.num_arcs()); ++shown)
            {
              Case<GT, int> c(s, shown_only(s, shown));
              if (c.t.negative)
                continue;
              c.label = "showing " + std::to_string(shown) + ": ";
              check_johnson_answers(numbers, c, Show_Mask<GT>{shown});
              std::set<typename GT::Arc *> arcs;
              for (size_t i = 0; i < s.num_arcs(); ++i)
                if (c.show()[i])
                  arcs.insert(c.b.arcs[i]);
              check_johnson_answers(pointers, c, Show_Arcs<GT>{&arcs});
            }
        });
    });

  numbers.expect();
  pointers.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, JohnsonAnswersWithCostsThatHaveFractions)
{
  using GT = TypeParam;
  // A class of its own, which the harness found: double costs, the halves
  // of the weights, whose potentials and new weights have fractions, which
  // the answers once lost in the infos of the copies of the arcs, of type
  // int; the infos of the copy still lose them, as documented
  Johnson_Answer_Checks halves(", with the halves of the weights as double costs");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, double> c(s, shown_but(s, 0));
      if (c.t.negative)
        return;
      for (double & x : c.w)
        x /= 2;
      check_johnson_answers(halves, c, Dft_Show_Arc<GT>(), 0.5);
    });

  halves.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, RandomDigraphsMatchTheOracle)
{
  using GT = TypeParam;
  const std::string how = ", on random digraphs";
  Search_Checks searches(how);
  Mean_Checks means(how);
  All_Pairs_Checks all_pairs(how);

  std::mt19937 rng(seed(79));
  for (int round = 0; round < (heavy() ? 1000 : 100); ++round)
    {
      const int n = 6 + int(rng() % 3);
      Spec s = random_spec(rng, n, n + int(rng() % size_t(n + 1)), true);
      random_weights(rng, s, -1 - (long long) (rng() % 4), 6);
      Case<GT, int> c(s, shown_but(s, 0));
      run_searches(searches, c, Dft_Show_Arc<GT>());
      run_means(means, c, Dft_Show_Arc<GT>());
      run_floyd(all_pairs.floyd, c, Dft_Show_Arc<GT>());
      run_johnson(all_pairs.johnson, c, Dft_Show_Arc<GT>());
    }

  searches.expect();
  means.expect();
  all_pairs.expect();
}

TYPED_TEST(ExhaustiveNegativeCycleSearches, DefaultArgumentsReadTheInfoOfTheArcs)
{
  using GT = TypeParam;
  Check bf("Bellman_Ford<GT>");
  Check others("the searches with Dft_Dist and Dft_Show_Arc");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          Case<GT, int> c(s, shown_but(s, 0));
          const auto numbers = weights_as_infos(c.b, s);
          auto number = [&] (typename GT::Arc * a)
            {
              const auto it = numbers.find(a);
              return it == numbers.end() ? -1 : it->second;
            };
          // The cycle of a path, a negative closed walk if want
          auto right = [&] (const Path<GT> & path, bool want)
            {
              if (path.is_empty())
                return not want;
              const Walk w = walk_of(path, number);
              return want and closed_walk_error(s, w, c.show(), true).empty()
                and cost_of(s, w) < 0;
            };

          Check & k = bf;
          k.run();
          for (int u = 0; u < s.n; ++u)
            {
              auto * p = c.b.nodes[size_t(u)];
              const bool want = c.t.negative_from[size_t(u)];
              Path<GT> path(c.b.g);
              if (Bellman_Ford<GT>(c.b.g).has_negative_cycle(p) != want
                  or Bellman_Ford<GT>(c.b.g).paint_spanning_tree(p) != want
                  or not right(Bellman_Ford<GT>(c.b.g).test_negative_cycle(p), want)
                  or not right(Bellman_Ford_Negative_Cycle<GT>()(c.b.g, p), want)
                  or Bellman_Ford_Negative_Cycle<GT>()(c.b.g, p, path) != want
                  or not right(path, want))
                k.fail(s, from_text(u) + "a wrong answer");
            }
          {
            Path<GT> path(c.b.g);
            if (Bellman_Ford<GT>(c.b.g).has_negative_cycle() != c.t.negative
                or not right(Bellman_Ford<GT>(c.b.g).search_negative_cycle(), c.t.negative)
                or not right(Bellman_Ford_Negative_Cycle<GT>()(c.b.g), c.t.negative)
                or Bellman_Ford_Negative_Cycle<GT>()(c.b.g, path) != c.t.negative
                or not right(path, c.t.negative))
              k.fail(s, "a wrong answer in the whole graph");
          }

          // The answers that do not depend on the cycle that is chosen
          others.run();
          bool has_mean = false;
          long long num = 0, den = 1;
          const long double mean = least_mean_value(c, has_mean, num, den);
          long long least = 0;
          size_t fewest = 0;
          const bool has_short = least_cost(c.t, 3, least, fewest);
          const auto karp = karp_minimum_mean_cycle(c.b.g);
          const auto howard = howard_minimum_mean_cycle(c.b.g);
          const auto three = most_negative_cycle_up_to_3(c.b.g);
          const bool negative_bounded = most_negative_cycle_bounded(c.b.g, 3).is_negative();
          bool johnson_throws = false;
          try
            {
              Johnson<GT> j(c.b.g);
            }
          catch (const std::domain_error &)
            {
              johnson_throws = true;
            }
          if (karp.has_cycle != has_mean or karp.minimum_mean != mean
              or howard.has_cycle != has_mean or howard.minimum_mean != mean
              or three.has_cycle != has_short or (has_short and three.total_cost != least)
              or negative_bounded != (has_short and least < 0)
              or find_disjoint_negative_cycles(c.b.g, 8).is_empty() == c.t.negative
              or (s.n > 0 and Floyd_All_Shortest_Paths<GT>(c.b.g).has_negative_cycle()
                  != c.t.negative)
              or johnson_throws != c.t.negative)
            others.fail(s, "a wrong answer");
        });
    });

  bf.expect();
  others.expect();
}

template <class GT>
class ExhaustiveNegativeCyclesNotDirected : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveNegativeCyclesNotDirected, Undirected_Kinds);

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, DetectionReadsEachArcFromItsSource)
{
  using GT = TypeParam;
  Detection_Checks k(", not directed");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_detection(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, WitnessesFromAStartReadEachArcFromItsSource)
{
  using GT = TypeParam;
  Witness_Checks k(false, ", not directed");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_witnesses(k, c, Dft_Show_Arc<GT>(), false);
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, WitnessesInTheWholeGraphReadEachArcFromItsSource)
{
  using GT = TypeParam;
  Witness_Checks k(true, ", not directed");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_witnesses(k, c, Dft_Show_Arc<GT>(), true);
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, TheTreeReadsEachArcFromItsSource)
{
  using GT = TypeParam;
  Tree_Checks k(", not directed");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_tree(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, FunctorsWithOutIteratorReadEachArcFromItsSource)
{
  using GT = TypeParam;
  Functor_Checks k(", not directed, with Out_Iterator");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_functors<Out_Iterator>(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, FunctorsWithTheirDefaultIteratorReadEachArc)
{
  using GT = TypeParam;
  Functor_Checks k(", not directed");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_functors<Node_Arc_Iterator>(k, c, Dft_Show_Arc<GT>());
    });

  k.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, TheOtherSearchesRejectTheGraph)
{
  using GT = TypeParam;
  Check rejected("Negative_Cycles.H, Karp and Howard, on a graph that is not directed");
  Check empty("Negative_Cycles.H, with max_cycles or max_length 0,"
              " on a graph that is not directed");

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      Spec s = shape;
      s.w.assign(s.num_arcs(), -1);
      Case<GT, int> c(s, shown_but(s, 0));
      const GT & g = c.b.g;
      int thrown = 0;
      auto count = [&] (auto && call)
        {
          try
            {
              call();
            }
          catch (const std::domain_error &)
            {
              ++thrown;
            }
        };
      rejected.run();
      count([&] { find_disjoint_negative_cycles(g, 1, Negative_Cycle_Exclusion::All_Arcs, c.d); });
      count([&] { most_negative_cycle_bounded(g, 2, c.d); });
      count([&] { most_negative_cycle_up_to_3(g, 2, c.d); });
      count([&] { karp_minimum_mean_cycle(g, c.d); });
      count([&] { karp_minimum_mean_cycle_value(g, c.d); });
      count([&] { howard_minimum_mean_cycle(g, c.d); });
      count([&] { howard_minimum_mean_cycle_value(g, c.d); });
      if (thrown != 7)
        rejected.fail(s, std::to_string(7 - thrown) + " of 7 do not throw domain_error");

      // With nothing to search, the graph is not looked at
      empty.run();
      if (not find_disjoint_negative_cycles(g, 0, Negative_Cycle_Exclusion::All_Arcs, c.d)
                .is_empty()
          or most_negative_cycle_bounded(g, 0, c.d).has_cycle
          or most_negative_cycle_up_to_3(g, 0, c.d).has_cycle)
        empty.fail(s, "a cycle");
    });

  rejected.expect();
  empty.expect();
}

namespace
{
  /// What is wrong with path as a shortest path of c from u to v, of cost
  /// want, along arcs that the filter shows, each in either direction;
  /// empty if nothing
  template <class GT>
  std::string undirected_path_error(const Case<GT, int> & c, const Path<GT> & path, int u,
                                    int v, long long want)
  {
    const Walk w = walk_of(path);
    if (w.nodes.empty() or w.nodes.size() != w.arcs.size() + 1)
      return "the path " + w.str() + " is empty or lacks arcs";
    if (w.nodes.front() != u or w.nodes.back() != v)
      return "the path " + w.str() + " does not go from " + std::to_string(u) + " to "
        + std::to_string(v);
    long long cost = 0;
    for (size_t i = 0; i < w.arcs.size(); ++i)
      {
        const auto & ends = c.s.arcs[size_t(w.arcs[i])];
        const std::set<int> joined = {ends[0], ends[1]}, walked = {w.nodes[i], w.nodes[i + 1]};
        if (joined != walked)
          return "the path " + w.str() + " does not follow the arc " + std::to_string(w.arcs[i]);
        if (not c.show()[size_t(w.arcs[i])])
          return "the path " + w.str() + " takes a hidden arc";
        cost += c.s.w[size_t(w.arcs[i])];
      }
    return cost == want ? "" : "the path " + w.str() + " costs " + std::to_string(cost);
  }

  /// Runs Johnson with the filter sa on c, a graph that is not directed,
  /// against the oracle that takes each arc in both directions: it throws
  /// domain_error if and only if then there is a negative cycle, and else
  /// it gives each distance and a shortest path
  template <class GT, class SA>
  void run_johnson_not_directed(Check & k, Case<GT, int> & c, const SA & sa)
  {
    bool negative = false;
    const auto d = oracle::shortest_distances(shown_spec(c.s, c.show()), false, negative);
    k.run();
    try
      {
        Johnson<GT, Spec_Weight<GT, int>, Arc_Iterator, Node_Arc_Iterator, SA> j(c.b.g, c.d, sa);
        if (negative)
          {
            k.fail(c.s, c.label + "does not throw on a negative cycle");
            return;
          }
        for (int u = 0; u < c.s.n; ++u)
          for (int v = 0; v < c.s.n; ++v)
            {
              const long long want = d[size_t(u)][size_t(v)];
              auto * p = c.b.nodes[size_t(u)];
              auto * q = c.b.nodes[size_t(v)];
              const std::string where = c.label + "from " + std::to_string(u) + " to "
                + std::to_string(v) + ": ";
              const int got = j.get_distance(p, q);
              if (got != (want == oracle::Inf ? std::numeric_limits<int>::max() : int(want)))
                k.fail(c.s, where + "gives " + std::to_string(got));
              const Path<GT> path = j.find_path(p, q);
              if (want == oracle::Inf ? not path.is_empty()
                  : not undirected_path_error(c, path, u, v, want).empty())
                k.fail(c.s, where + "a wrong path");
            }
      }
    catch (const std::domain_error &)
      {
        if (not negative)
          k.fail(c.s, c.label + "throws domain_error without a negative cycle");
      }
  }
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, JohnsonTakesEachArcInBothDirections)
{
  using GT = TypeParam;
  Check k("Johnson, not directed");
  Check hiding("Johnson, not directed, hiding arcs");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_johnson_not_directed(k, c, Dft_Show_Arc<GT>());
    });

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              Case<GT, int> c(s, shown_but(s, hidden));
              c.label = "hiding " + std::to_string(hidden) + ": ";
              run_johnson_not_directed(hiding, c, Hide_Mask<GT>{hidden});
            }
        });
    });

  k.expect();
  hiding.expect();
}

TYPED_TEST(ExhaustiveNegativeCyclesNotDirected, FloydTakesEachArcInBothDirections)
{
  using GT = TypeParam;
  Check k("Floyd_All_Shortest_Paths::has_negative_cycle(), not directed");
  Check hiding("Floyd_All_Shortest_Paths::has_negative_cycle(), not directed, hiding arcs");

  for_each_weighted_spec([&] (const Spec & s)
    {
      Case<GT, int> c(s, shown_but(s, 0));
      run_floyd(k, c, Dft_Show_Arc<GT>());
    });

  for_each_spec(small_weights_domain(), true, [&] (const Spec & shape)
    {
      for_each_weighting(shape, Sign_Weights, [&] (const Spec & s)
        {
          for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
            {
              Case<GT, int> c(s, shown_but(s, hidden));
              c.label = "hiding " + std::to_string(hidden) + ": ";
              run_floyd(hiding, c, Hide_Mask<GT>{hidden});
            }
        });
    });

  k.expect();
  hiding.expect();
}
