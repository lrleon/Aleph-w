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
 * @file graph_exhaustive_properties_test.cc
 * @brief Colorings and the sufficient conditions of Hamilton cycles on
 *        every small graph, against brute force.
 *
 * The colorings run on every multigraph of the domain of graph_harness.H
 * and on every simple graph of up to 5 nodes (every simple digraph of up
 * to 4 on the kinds of digraph, whose arcs they take as not directed):
 * with a loop they must throw, and else give a proper coloring, the exact
 * one with chromatic_number(). Ore's and Dirac's conditions run on the
 * same graphs, with each set of hidden arcs on those of 3 arcs or 3
 * nodes, and on every multigraph of 3 nodes and up to 6 arcs: they must
 * say what the condition says of the simple graph that the shown arcs
 * give, and never claim a Hamilton cycle that brute force does not find.
 */

#include <algorithm>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Graph_Coloring.H>
#include <hamiltonian.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;

namespace
{
  /// The simple graph of the arcs of s that hidden shows: adj[u][v] if
  /// some arc goes from u to v, or joins them if not directed; loops and
  /// parallel arcs leave no trace
  std::vector<std::vector<bool>> simple_adjacency(const Spec & s, bool directed,
                                                  unsigned long hidden = 0)
  {
    std::vector<std::vector<bool>> adj(size_t(s.n), std::vector<bool>(size_t(s.n), false));
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        const int u = s.arcs[i][0], v = s.arcs[i][1];
        if (u == v or not graph_filters::shown(hidden, int(i)))
          continue;
        adj[size_t(u)][size_t(v)] = true;
        if (not directed)
          adj[size_t(v)][size_t(u)] = true;
      }
    return adj;
  }

  /// Whether the nodes can take colors from k so that no two adjacent
  /// ones share one, by backtracking in the order of the nodes
  bool colorable(const std::vector<std::vector<bool>> & adj, int k,
                 std::vector<int> & color, int x = 0)
  {
    const int n = int(adj.size());
    if (x == n)
      return true;
    for (int c = 0; c < k; ++c)
      {
        bool ok = true;
        for (int y = 0; y < x and ok; ++y)
          ok = color[size_t(y)] != c or not (adj[size_t(x)][size_t(y)] or adj[size_t(y)][size_t(x)]);
        if (not ok)
          continue;
        color[size_t(x)] = c;
        if (colorable(adj, k, color, x + 1))
          return true;
      }
    return false;
  }

  /// The chromatic number, by brute force
  int chromatic(const std::vector<std::vector<bool>> & adj)
  {
    std::vector<int> color(adj.size(), -1);
    int k = 0;
    while (not colorable(adj, k, color))
      ++k;
    return k;
  }

  /// Whether some cycle goes through every node once, by brute force over
  /// the orders of the nodes after the first; it needs 3 nodes or more
  /// in a graph that is not directed, 2 in a digraph
  bool hamiltonian(const std::vector<std::vector<bool>> & adj, bool directed)
  {
    const int n = int(adj.size());
    if (n < (directed ? 2 : 3))
      return false;
    std::vector<int> order(static_cast<size_t>(n));
    std::iota(order.begin(), order.end(), 0);
    do
      {
        bool ok = true;
        for (int i = 0; i < n and ok; ++i)
          ok = adj[size_t(order[size_t(i)])][size_t(order[size_t((i + 1) % n)])];
        if (ok)
          return true;
      }
    while (std::next_permutation(order.begin() + 1, order.end()));
    return false;
  }

  /// The degree of x in a simple graph: its neighbors, or, in a digraph,
  /// the nodes it goes to (out) or that come to it (in)
  int simple_degree(const std::vector<std::vector<bool>> & adj, int x, bool out = true)
  {
    int d = 0;
    for (size_t y = 0; y < adj.size(); ++y)
      d += out ? adj[size_t(x)][y] : adj[y][size_t(x)];
    return d;
  }

  /// Ore's condition, and for a digraph Woodall's, on a simple graph of 3
  /// nodes or more: every pair without an arc between them, or without
  /// one from the first to the second, has degrees that add up to n
  bool ore(const std::vector<std::vector<bool>> & adj, bool directed)
  {
    const int n = int(adj.size());
    if (n < 3)
      return false;
    for (int u = 0; u < n; ++u)
      for (int v = directed ? 0 : u + 1; v < n; ++v)
        if (u != v and not adj[size_t(u)][size_t(v)])
          if (simple_degree(adj, u) + simple_degree(adj, v, not directed) < n)
            return false;
    return true;
  }

  /// Dirac's condition, and for a digraph Ghouila-Houri's, on a simple
  /// graph of 3 nodes or more: every degree, in and out, is n / 2 or more
  bool dirac(const std::vector<std::vector<bool>> & adj, bool directed)
  {
    const int n = int(adj.size());
    if (n < 3)
      return false;
    for (int x = 0; x < n; ++x)
      if (2 * simple_degree(adj, x) < n or (directed and 2 * simple_degree(adj, x, false) < n))
        return false;
    return true;
  }

  /// Whether s has parallel or antiparallel arcs, or loops, among those
  /// that hidden shows: the graph is not simple
  bool multigraph(const Spec & s, bool directed, unsigned long hidden = 0)
  {
    std::set<std::pair<int, int>> seen;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        if (not graph_filters::shown(hidden, int(i)))
          continue;
        int u = s.arcs[i][0], v = s.arcs[i][1];
        if (u == v)
          return true;
        if (not directed and u > v)
          std::swap(u, v);
        if (not seen.insert({u, v}).second)
          return true;
      }
    return false;
  }

  template <class GT>
  using Colors = DynMapTree<typename GT::Node *, size_t>;

  /// What is wrong with colors, returned as num, as a proper coloring of
  /// g with the colors 0 to num - 1; empty if nothing
  template <class GT>
  std::string coloring_error(const Built<GT> & b, const std::vector<std::vector<bool>> & adj,
                             const Colors<GT> & colors, size_t num)
  {
    const int n = int(adj.size());
    std::vector<long> c(size_t(n), -1);
    for (int x = 0; x < n; ++x)
      {
        auto * p = colors.search(b.nodes[size_t(x)]);
        if (p == nullptr)
          return "the node " + std::to_string(x) + " has no color";
        c[size_t(x)] = long(p->second);
      }
    std::set<long> used(c.begin(), c.end());
    if (used.size() != num or (num > 0 and *used.rbegin() != long(num) - 1))
      return "it says " + std::to_string(num) + " colors, and uses " + std::to_string(used.size());
    for (int u = 0; u < n; ++u)
      for (int v = 0; v < n; ++v)
        if ((adj[size_t(u)][size_t(v)] or adj[size_t(v)][size_t(u)]) and c[size_t(u)] == c[size_t(v)])
          return "the adjacent " + std::to_string(u) + " and " + std::to_string(v) + " share a color";
    return "";
  }

  /// The graphs of a test: the multigraphs of the domain, and the simple
  /// graphs of 1 to max_simple nodes
  template <class F>
  void for_each_test_graph(bool directed, int max_simple, F && f)
  {
    for_each_spec(domain(4, 3), directed, f);
    for_each_simple_graph(1, heavy() ? max_simple + 1 : max_simple, directed, f);
  }
}

template <class GT>
class ExhaustiveProperties : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveProperties, All_Kinds);

TYPED_TEST(ExhaustiveProperties, ColoringsAreProperAndTheChromaticNumberExact)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check greedy("greedy_coloring()");
  Check welsh_powell("welsh_powell_coloring()");
  Check dsatur("dsatur_coloring()");
  Check exact("chromatic_number()");
  Check valid("is_valid_coloring()");
  Check functors("Greedy_Coloring, Welsh_Powell_Coloring and DSatur_Coloring");

  for_each_test_graph(digraph, digraph ? 4 : 5, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const bool loop = s.has_loop();
      // The colorings take the arcs as not directed
      const auto adj = simple_adjacency(s, false);

      auto check = [&] (Check & c, auto && color)
        {
          c.run();
          Colors<GT> colors;
          try
            {
              const size_t num = color(colors);
              if (loop)
                c.fail(s, "colors a graph with a loop");
              else if (auto e = coloring_error(b, adj, colors, num); not e.empty())
                c.fail(s, e);
              return num;
            }
          catch (const std::domain_error &)
            {
              if (not loop)
                c.fail(s, "throws on a graph without loops");
            }
          return size_t(0);
        };

      check(greedy, [&] (Colors<GT> & c) { return greedy_coloring(b.g, c); });
      check(welsh_powell, [&] (Colors<GT> & c) { return welsh_powell_coloring(b.g, c); });
      check(dsatur, [&] (Colors<GT> & c) { return dsatur_coloring(b.g, c); });
      const size_t chi = check(exact, [&] (Colors<GT> & c) { return chromatic_number(b.g, c); });
      if (not loop and int(chi) != chromatic(adj))
        exact.fail(s, "gives " + std::to_string(chi) + ", not " + std::to_string(chromatic(adj)));

      check(functors, [&] (Colors<GT> & c) { return Greedy_Coloring<GT>()(b.g, c); });
      check(functors, [&] (Colors<GT> & c) { return Welsh_Powell_Coloring<GT>()(b.g, c); });
      check(functors, [&] (Colors<GT> & c) { return DSatur_Coloring<GT>()(b.g, c); });

      if (loop)
        return;

      // A proper coloring is valid; giving two ends of an arc one color,
      // or leaving a node without one, makes it invalid
      valid.run();
      Colors<GT> colors;
      (void) greedy_coloring(b.g, colors);
      if (not is_valid_coloring(b.g, colors))
        valid.fail(s, "rejects a proper coloring");
      for (size_t i = 0; i < s.arcs.size(); ++i)
        {
          Colors<GT> spoiled = colors;
          spoiled.find(b.nodes[size_t(s.arcs[i][1])]) = spoiled.find(b.nodes[size_t(s.arcs[i][0])]);
          if (is_valid_coloring(b.g, spoiled))
            valid.fail(s, "accepts one color at both ends of the arc " + std::to_string(i));
        }
      if (s.n > 0)
        {
          Colors<GT> partial = colors;
          partial.remove(b.nodes[0]);
          if (is_valid_coloring(b.g, partial))
            valid.fail(s, "accepts a node without a color");
        }
    });

  greedy.expect();
  welsh_powell.expect();
  dsatur.expect();
  exact.expect();
  valid.expect();
  functors.expect();
}

TYPED_TEST(ExhaustiveProperties, HamiltonConditionsAreThoseOfTheirTheorems)
{
  using GT = TypeParam;
  using Filter = Hide_Mask<GT>;
  const bool digraph = GT().is_digraph();
  Check ore_simple("Test_Hamiltonian_Sufficiency, simple graphs");
  Check dirac_simple("Test_Dirac_Condition, simple graphs");
  Check ore_multi("Test_Hamiltonian_Sufficiency, multigraphs and hidden arcs");
  Check dirac_multi("Test_Dirac_Condition, multigraphs and hidden arcs");
  Check sound("a condition that holds has a Hamilton cycle");

  auto run = [&] (const Spec & s, bool masks)
    {
      Built<GT> b(s);
      for (unsigned long hidden = 0; hidden < (1UL << s.num_arcs()); ++hidden)
        {
          if (hidden != 0 and (not masks or (s.num_arcs() > 3 and s.n > 3)))
            break; // the masks, on the graphs of 3 arcs or 3 nodes
          const auto adj = simple_adjacency(s, digraph, hidden);
          const bool simple = hidden == 0 and not multigraph(s, digraph);
          const std::string at = hidden == 0 ? "" : "hiding " + std::to_string(hidden) + ": ";

          const bool got_ore = Test_Hamiltonian_Sufficiency<GT, Dft_Show_Node<GT>, Filter>
            (Dft_Show_Node<GT>(), Filter{hidden})(b.g);
          const bool got_dirac = Test_Dirac_Condition<GT, Dft_Show_Node<GT>, Filter>
            (Dft_Show_Node<GT>(), Filter{hidden})(b.g);

          Check & o = simple ? ore_simple : ore_multi;
          o.run();
          if (got_ore != ore(adj, digraph))
            o.fail(s, at + (got_ore ? "holds" : "does not hold"));

          Check & d = simple ? dirac_simple : dirac_multi;
          d.run();
          if (got_dirac != dirac(adj, digraph))
            d.fail(s, at + (got_dirac ? "holds" : "does not hold"));

          // What the conditions claim, where they are right
          if (simple)
            {
              sound.run();
              if ((got_ore or got_dirac) and not hamiltonian(adj, digraph))
                sound.fail(s, "a condition holds on a graph without a Hamilton cycle");
            }
        }
    };

  for_each_test_graph(digraph, digraph ? 4 : 5, [&] (const Spec & s) { run(s, true); });

  // Loops and parallel arcs enough to reach the degrees of the conditions
  Domain dense;
  dense.max_nodes = 3;
  dense.max_arcs = 6;
  dense.directions = false;
  dense.reversed = false;
  for_each_spec(dense, digraph, [&] (const Spec & s) { run(s, false); });

  ore_simple.expect();
  dirac_simple.expect();
  ore_multi.expect();
  dirac_multi.expect();
  sound.expect();
}
