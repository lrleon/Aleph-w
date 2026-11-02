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
 * @file graph_exhaustive_generators_test.cc
 * @brief The random graphs of random_graph.H, for every size up to 8
 *        nodes and many seeds, against what they promise.
 *
 * Random_Graph and Random_Digraph make graphs without loops nor parallel
 * arcs, connected when asked (strongly, for a digraph); eulerian() makes
 * one with an Eulerian cycle, or throws domain_error only if no arc added
 * to the graph it starts from would do; sufficient_hamiltonian() makes
 * one where Ore's condition holds. The test checks each promise on every
 * size and number of arcs up to 8 nodes, with many seeds, and that a seed
 * gives the same graph again. The nodes are numbered as they are made,
 * by an initializer that the generator keeps by reference.
 */

#include <algorithm>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <tpl_agraph.H>
#include <random_graph.H>

#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;

namespace
{
  /// Numbers the nodes in the order in which the generator makes them
  template <class GT>
  struct Number_Nodes
  {
    int next = 0;
    void operator () (GT &, typename GT::Node * p) { p->get_info() = next++; }
  };

  /// Numbers the arcs in the order in which the generator makes them
  template <class GT>
  struct Number_Arcs
  {
    int next = 0;
    void operator () (GT &, typename GT::Arc * a) { a->get_info() = next++; }
  };

  /// The spec of a generated graph, by the numbers of its nodes
  template <class GT>
  Spec spec_of(const GT & g)
  {
    Spec s;
    s.n = int(g.get_num_nodes());
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr();
        s.arcs.push_back({num(g.get_src_node(a)), num(g.get_tgt_node(a))});
      }
    return s;
  }

  /// The arcs of s as a set of pairs, each pair ordered unless directed
  std::set<std::pair<int, int>> pairs_of(const Spec & s, bool directed)
  {
    std::set<std::pair<int, int>> r;
    for (const auto & a : s.arcs)
      r.insert(directed ? std::pair(a[0], a[1])
                        : std::pair(std::min(a[0], a[1]), std::max(a[0], a[1])));
    return r;
  }

  /// What is wrong with s as a graph of n nodes, numbered from 0, without
  /// loops nor parallel arcs; empty if nothing
  std::string simple_error(const Spec & s, int n, bool directed)
  {
    if (s.n != n)
      return std::to_string(s.n) + " nodes, not " + std::to_string(n);
    for (const auto & a : s.arcs)
      if (a[0] < 0 or a[0] >= n or a[1] < 0 or a[1] >= n)
        return "a node is not numbered";
      else if (a[0] == a[1])
        return "a loop";
    if (pairs_of(s, directed).size() != s.arcs.size())
      return "parallel arcs";
    return "";
  }

  /// Whether every node reaches every other one (by arcs either way if
  /// not directed)
  bool connected(const Spec & s, bool directed)
  {
    const auto r = oracle::closure(s, directed);
    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        if (not oracle::reaches(r, u, v))
          return false;
    return true;
  }

  /// Whether arcs can be added to the simple graph s, without loops nor
  /// parallel arcs, so that every node has an even degree, or, in a
  /// digraph, as many arcs in as out: whether each component of the
  /// complement has an even number of nodes of odd degree; a digraph can
  /// always become the complete one, which is balanced
  bool can_be_made_eulerian(const Spec & s, bool directed)
  {
    if (directed)
      return true;
    const auto have = pairs_of(s, false);
    Spec complement;
    complement.n = s.n;
    for (int u = 0; u < s.n; ++u)
      for (int v = u + 1; v < s.n; ++v)
        if (not have.count({u, v}))
          complement.arcs.push_back({u, v});
    const auto comp = oracle::component_of(complement);
    std::vector<int> odd(size_t(s.n), 0);
    for (int x = 0; x < s.n; ++x)
      odd[size_t(comp[size_t(x)])] += oracle::degree(s, x) % 2;
    return std::all_of(odd.begin(), odd.end(), [] (int k) { return k % 2 == 0; });
  }

  /// Ore's condition on a simple graph of 3 nodes or more
  bool ore(const Spec & s)
  {
    if (s.n < 3)
      return false;
    const auto have = pairs_of(s, false);
    for (int u = 0; u < s.n; ++u)
      for (int v = u + 1; v < s.n; ++v)
        if (not have.count({u, v}) and oracle::degree(s, u) + oracle::degree(s, v) < s.n)
          return false;
    return true;
  }

  /// Whether some cycle goes through every node once
  bool hamiltonian(const Spec & s)
  {
    if (s.n < 3)
      return false;
    const auto have = pairs_of(s, false);
    std::vector<int> order(static_cast<size_t>(s.n));
    std::iota(order.begin(), order.end(), 0);
    do
      {
        bool ok = true;
        for (int i = 0; i < s.n and ok; ++i)
          {
            const int u = order[size_t(i)], v = order[size_t((i + 1) % s.n)];
            ok = have.count({std::min(u, v), std::max(u, v)}) > 0;
          }
        if (ok)
          return true;
      }
    while (std::next_permutation(order.begin() + 1, order.end()));
    return false;
  }

  /** Runs the generators of kind Gen, Random_Graph or Random_Digraph, on
      every size up to max_nodes nodes, with several numbers of arcs and
      probabilities, and many seeds. */
  template <class GT, template <class, class, class> class Gen>
  void check_generators(bool directed, int max_nodes, int seeds)
  {
    using Make = Gen<GT, Number_Nodes<GT>, Number_Arcs<GT>>;
    Check sparse("operator()(n, m, connected)");
    Check dense("operator()(n, p, connected)");
    // connect() and eulerian() picked among the nodes in the order of their
    // addresses, which List_SGraph keeps, so that on it the same seed gave
    // another graph in another run
    Check same("the same seed gives the same graph");
    Check eulerian_m("eulerian(n, m)");
    Check eulerian_p("eulerian(n, p)");
    // The graph that eulerian() starts from, which these judge it by, is
    // the one that the seed gives
    Check kept_m("eulerian(n, m), from the graph that the seed gives");
    Check kept_p("eulerian(n, p), from the graph that the seed gives");
    Check hamilton("sufficient_hamiltonian(n, p)");

    // A graph made with a seed, by a generator of its own
    auto make = [] (unsigned long seed, auto && f)
      {
        Number_Nodes<GT> nodes;
        Number_Arcs<GT> arcs;
        Make gen(seed, nodes, arcs);
        return spec_of(f(gen));
      };

    for (int n = 2; n <= max_nodes; ++n)
      {
        const int pairs = directed ? n * (n - 1) : n * (n - 1) / 2;
        for (int m = 0; m <= pairs; m += 1 + pairs / 8)
          for (unsigned long seed = 1; seed <= unsigned(seeds); ++seed)
            for (bool conn : {false, true})
              {
                const std::string what = "n=" + std::to_string(n) + " m=" + std::to_string(m)
                  + " seed=" + std::to_string(seed) + (conn ? " connected" : "") + ": ";
                Spec at;
                at.n = n;

                sparse.run();
                const Spec s = make(seed, [&] (Make & g) { return g(size_t(n), size_t(m), conn); });
                if (auto e = simple_error(s, n, directed); not e.empty())
                  sparse.fail(at, what + e);
                else if (conn and not connected(s, directed))
                  sparse.fail(at, what + "not connected");

                same.run();
                if (pairs_of(make(seed, [&] (Make & g) { return g(size_t(n), size_t(m), conn); }),
                             directed) != pairs_of(s, directed))
                  same.fail(at, what + "another graph");
              }

        for (double p : {0.1, 0.3, 0.5, 0.8, 1.0})
          for (unsigned long seed = 1; seed <= unsigned(seeds); ++seed)
            {
              const std::string what = "n=" + std::to_string(n) + " p=" + std::to_string(p)
                + " seed=" + std::to_string(seed) + ": ";
              Spec at;
              at.n = n;

              dense.run();
              const Spec s = make(seed, [&] (Make & g) { return g(size_t(n), p, true); });
              if (auto e = simple_error(s, n, directed); not e.empty())
                dense.fail(at, what + e);
              else if (not connected(s, directed))
                dense.fail(at, what + "not connected");
              else if (p == 1.0 and int(s.arcs.size()) != (directed ? n * (n - 1) : n * (n - 1) / 2))
                dense.fail(at, what + "p = 1 without every arc");

              if (not directed and n >= 3)
                {
                  hamilton.run();
                  const Spec h = make(seed, [&] (Make & g) { return g.sufficient_hamiltonian(size_t(n), p); });
                  if (auto e = simple_error(h, n, false); not e.empty())
                    hamilton.fail(at, what + e);
                  else if (not ore(h) or not hamiltonian(h))
                    hamilton.fail(at, what + "neither Ore's condition nor a Hamilton cycle");
                }
            }

        // eulerian() starts from the connected graph that the seed gives,
        // which the test can make again if the seed gives the same graph
        auto check_eulerian = [&] (Check & check, Check & kept, const std::string & what,
                                   const Spec & base, bool known_base, auto && f)
          {
            Spec at;
            at.n = n;
            check.run();
            try
              {
                const Spec e = f();
                if (auto err = simple_error(e, n, directed); not err.empty())
                  check.fail(at, what + err);
                else if (oracle::euler_class(e, directed) != oracle::Euler::Cycle
                         or not connected(e, directed))
                  check.fail(at, what + "not Eulerian");
                else if (known_base)
                  {
                    kept.run();
                    const auto have = pairs_of(e, directed);
                    for (const auto & pr : pairs_of(base, directed))
                      if (not have.count(pr))
                        {
                          kept.fail(at, what + "an arc of the graph it starts from is gone");
                          break;
                        }
                  }
              }
            catch (const std::domain_error &)
              {
                if (known_base)
                  {
                    kept.run();
                    if (can_be_made_eulerian(base, directed))
                      kept.fail(at, what + "throws, but arcs added would do");
                  }
              }
          };

        for (int m = 0; m <= pairs; m += 1 + pairs / 8)
          for (unsigned long seed = 1; seed <= unsigned(seeds); ++seed)
            {
              const std::string what = "n=" + std::to_string(n) + " m=" + std::to_string(m)
                + " seed=" + std::to_string(seed) + ": ";
              const Spec base = make(seed, [&] (Make & g) { return g(size_t(n), size_t(m), true); });
              const bool known_base = pairs_of(base, directed) == pairs_of
                (make(seed, [&] (Make & g) { return g(size_t(n), size_t(m), true); }), directed);
              check_eulerian(eulerian_m, kept_m, what, base, known_base, [&]
                {
                  return make(seed, [&] (Make & g) { return g.eulerian(size_t(n), size_t(m)); });
                });
            }

        for (double p : {0.1, 0.3, 0.5, 0.8, 1.0})
          for (unsigned long seed = 1; seed <= unsigned(seeds); ++seed)
            {
              const std::string what = "n=" + std::to_string(n) + " p=" + std::to_string(p)
                + " seed=" + std::to_string(seed) + ": ";
              const Spec base = make(seed, [&] (Make & g) { return g(size_t(n), p, true); });
              const bool known_base = pairs_of(base, directed) == pairs_of
                (make(seed, [&] (Make & g) { return g(size_t(n), p, true); }), directed);
              check_eulerian(eulerian_p, kept_p, what, base, known_base, [&]
                {
                  return make(seed, [&] (Make & g) { return g.eulerian(size_t(n), p); });
                });
            }
      }

    sparse.expect();
    dense.expect();
    same.expect();
    eulerian_m.expect();
    eulerian_p.expect();
    kept_m.expect();
    kept_p.expect();
    hamilton.expect();
  }
}

template <class GT>
class ExhaustiveGenerators : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveGenerators, Undirected_Kinds);

TYPED_TEST(ExhaustiveGenerators, RandomGraphsKeepTheirPromises)
{
  check_generators<TypeParam, Random_Graph>(false, heavy() ? 9 : 8, heavy() ? 60 : 12);
}

template <class GT>
class ExhaustiveDigraphGenerators : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveDigraphGenerators, Directed_Kinds);

TYPED_TEST(ExhaustiveDigraphGenerators, RandomDigraphsKeepTheirPromises)
{
  check_generators<TypeParam, Random_Digraph>(true, heavy() ? 8 : 7, heavy() ? 60 : 12);
}
