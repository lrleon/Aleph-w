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
 * @file graph_exhaustive_connectivity_test.cc
 * @brief Connectivity, cycles, bipartition, matchings, spanning trees and
 *        Euler on every small multigraph, against the oracles.
 *
 * Each test runs one family of algorithms over every multigraph of the
 * domain of graph_harness.H (up to 4 nodes and 3 arcs; one more of each
 * with ALEPH_GRAPH_HARNESS=heavy), with loops, parallel arcs and each arc
 * inserted from either end, on the three kinds of graph that are not
 * directed, and compares each answer with graph_oracles.H.
 */

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_components.H>
#include <tpl_graph_utils.H>
#include <tpl_cut_nodes.H>
#include <tpl_test_acyclique.H>
#include <tpl_bipartite.H>
#include <tpl_spanning_tree.H>
#include <Blossom.H>
#include <eulerian.H>

#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;

namespace
{
  template <class List>
  std::set<int> numbers(const List & l)
  {
    std::set<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.insert(num(it.get_curr()));
    return r;
  }

  std::string text(const std::set<int> & s)
  {
    std::string r = "{";
    for (int x : s)
      r += (r.size() > 1 ? " " : "") + std::to_string(x);
    return r + "}";
  }

  /// The nodes from which the searches start: the first and the last
  std::vector<int> starts(const Spec & s)
  {
    if (s.n == 0)
      return {};
    if (s.n == 1)
      return {0};
    return {0, s.n - 1};
  }

  /// What is wrong in sg, built from b: it should hold the nodes and arcs
  /// of the numbers want_nodes and want_arcs, each one mapped to its copy
  /// and the copy to it, with each copy of an arc between the copies of
  /// its nodes
  template <class GT>
  std::string subgraph_error(Built<GT> & b, GT & sg, const std::set<int> & want_nodes,
                             const std::set<int> & want_arcs)
  {
    std::set<int> got_nodes, got_arcs;
    for (auto it = sg.get_node_it(); it.has_curr(); it.next_ne())
      {
        auto q = it.get_curr();
        const int x = num(q);
        got_nodes.insert(x);
        if (x < 0 or size_t(x) >= b.nodes.size() or mapped_node<GT>(q) != b.nodes[size_t(x)])
          return "the copy of " + std::to_string(x) + " maps to another node";
        if (mapped_node<GT>(b.nodes[size_t(x)]) != q)
          return "the node " + std::to_string(x) + " does not map to its copy";
      }
    for (auto it = sg.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto c = it.get_curr();
        const int i = c->get_info();
        got_arcs.insert(i);
        if (i < 0 or size_t(i) >= b.arcs.size() or mapped_arc<GT>(c) != b.arcs[size_t(i)])
          return "the copy of the arc " + std::to_string(i) + " maps to another arc";
        auto a = b.arcs[size_t(i)];
        if (mapped_arc<GT>(a) != c)
          return "the arc " + std::to_string(i) + " does not map to its copy";
        auto src = mapped_node<GT>(sg.get_src_node(c));
        auto tgt = mapped_node<GT>(sg.get_tgt_node(c));
        auto g_src = b.g.get_src_node(a);
        auto g_tgt = b.g.get_tgt_node(a);
        if (not ((src == g_src and tgt == g_tgt) or (src == g_tgt and tgt == g_src)))
          return "the copy of the arc " + std::to_string(i) + " joins other nodes";
      }
    if (got_nodes != want_nodes or sg.get_num_nodes() != want_nodes.size())
      return "the nodes are " + text(got_nodes) + ", not " + text(want_nodes);
    if (got_arcs != want_arcs or sg.get_num_arcs() != want_arcs.size())
      return "the arcs are " + text(got_arcs) + ", not " + text(want_arcs);
    return "";
  }
}

template <class GT>
class ExhaustiveConnectivity : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveConnectivity, Undirected_Kinds);

TYPED_TEST(ExhaustiveConnectivity, ComponentsMatchTheOracle)
{
  using GT = TypeParam;
  Check count("Unconnected_Components::count_components()");
  Check lists("Unconnected_Components::operator()");
  Check inconnected("inconnected_components()");
  Check connectivity("test_connectivity()");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const int want = oracle::components(s);

      count.run();
      const size_t c = Unconnected_Components<GT>().count_components(b.g);
      if (int(c) != want)
        count.fail(s, std::to_string(c) + " components, not " + std::to_string(want));

      auto total_arcs = [] (const auto & l)
        {
          size_t arcs = 0;
          for (auto it = l.get_it(); it.has_curr(); it.next_ne())
            arcs += it.get_curr().get_num_arcs();
          return arcs;
        };

      lists.run();
      DynList<GT> l;
      Unconnected_Components<GT>()(b.g, l);
      if (int(l.size()) != want or total_arcs(l) != s.num_arcs())
        lists.fail(s, std::to_string(l.size()) + " components, "
                   + std::to_string(total_arcs(l)) + " arcs");

      inconnected.run();
      auto ic = inconnected_components(b.g);
      if (int(ic.size()) != want or total_arcs(ic) != s.num_arcs())
        inconnected.fail(s, std::to_string(ic.size()) + " components, "
                         + std::to_string(total_arcs(ic)) + " arcs");

      connectivity.run();
      const bool connected = test_connectivity(b.g);
      if (connected != (s.n > 0 and want == 1))
        connectivity.fail(s, std::string("says ") + (connected ? "" : "not ") + "connected");
    });

  count.expect();
  lists.expect();
  inconnected.expect();
  connectivity.expect();
}

// Build_Subgraph from each start copies its component, with the cookies
// clear and with cookies toward a graph that went, as those that a mapping
// to a graph leaves after it goes
TYPED_TEST(ExhaustiveConnectivity, SubgraphsMatchTheOracle)
{
  using GT = TypeParam;
  Check clear("Build_Subgraph");
  Check stale("Build_Subgraph, with cookies toward a graph that went");
  Check listed("Build_Subgraph, into a list of the nodes");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      const auto comp = oracle::component_of(s);
      for (int start : starts(s))
        {
          std::set<int> want_nodes, want_arcs;
          for (int x = 0; x < s.n; ++x)
            if (comp[size_t(x)] == comp[size_t(start)])
              want_nodes.insert(x);
          for (size_t i = 0; i < s.arcs.size(); ++i)
            if (comp[size_t(s.arcs[i][0])] == comp[size_t(start)])
              want_arcs.insert(int(i));

          for (const bool old_cookies : {false, true})
            {
              Check & check = old_cookies ? stale : clear;
              check.run();
              Built<GT> b(s);
              if (old_cookies)
                {
                  GT other;
                  auto o = other.insert_node(-1);
                  auto oa = other.insert_arc(o, o, -1);
                  for (auto p : b.nodes)
                    NODE_COOKIE(p) = o;
                  for (auto a : b.arcs)
                    ARC_COOKIE(a) = oa;
                }
              GT sg;
              Build_Subgraph<GT>()(b.g, sg, b.nodes[size_t(start)]);
              if (auto e = subgraph_error(b, sg, want_nodes, want_arcs); not e.empty())
                check.fail(s, "from " + std::to_string(start) + ": " + e);
            }

          listed.run();
          Built<GT> b(s);
          DynList<typename GT::Node *> l;
          Build_Subgraph<GT>()(b.g, l, b.nodes[size_t(start)]);
          if (const auto got = numbers(l); got != want_nodes or l.size() != want_nodes.size())
            listed.fail(s, "from " + std::to_string(start) + " gives " + text(got) + ", not "
                        + text(want_nodes));
        }
    });

  clear.expect();
  stale.expect();
  listed.expect();
}

TYPED_TEST(ExhaustiveConnectivity, CutNodesAndBridgesMatchTheOracle)
{
  using GT = TypeParam;
  Check function("compute_cut_nodes()");
  Check object("Compute_Cut_Nodes");
  Check bridges("find_bridges()");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const auto want_bridges = oracle::bridges(s);
      for (int start : starts(s))
        {
          const auto want = oracle::cut_nodes(s, start);
          auto * src = b.nodes[size_t(start)];

          function.run();
          const auto got_f = numbers(compute_cut_nodes(b.g, src));
          if (got_f != want)
            function.fail(s, "from " + std::to_string(start) + " gives " + text(got_f)
                          + ", not " + text(want));

          object.run();
          DynDlist<typename GT::Node *> l;
          Compute_Cut_Nodes<GT> cc(b.g);
          cc(src, l);
          const auto got_o = numbers(l);
          if (got_o != want)
            object.fail(s, "from " + std::to_string(start) + " gives " + text(got_o)
                        + ", not " + text(want));

          bridges.run();
          std::set<int> got_b;
          auto lb = find_bridges(b.g, src);
          for (auto it = lb.get_it(); it.has_curr(); it.next_ne())
            got_b.insert(num(it.get_curr()));
          if (got_b != want_bridges)
            bridges.fail(s, "from " + std::to_string(start) + " gives " + text(got_b)
                         + ", not " + text(want_bridges));
        }
    });

  function.expect();
  object.expect();
  bridges.expect();
}

TYPED_TEST(ExhaustiveConnectivity, CyclesMatchTheOracle)
{
  using GT = TypeParam;
  Check acyclique("is_graph_acyclique()");
  Check has_cycle("Has_Cycle");
  Check through("test_for_cycle(g, start)");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const bool cycle = oracle::has_cycle(s);

      acyclique.run();
      if (is_graph_acyclique(b.g) == cycle)
        acyclique.fail(s, cycle ? "says acyclic" : "says cyclic");

      has_cycle.run();
      if (Has_Cycle<GT>()(b.g) != cycle)
        has_cycle.fail(s, cycle ? "finds no cycle" : "finds a cycle");

      for (int start : starts(s))
        {
          through.run();
          const bool want = oracle::on_cycle(s, start);
          if (test_for_cycle(b.g, b.nodes[size_t(start)]) != want)
            through.fail(s, "from " + std::to_string(start)
                         + (want ? " finds no cycle" : " finds a cycle"));
        }
    });

  acyclique.expect();
  has_cycle.expect();
  through.expect();
}

TYPED_TEST(ExhaustiveConnectivity, BipartitionAndMatchingsMatchTheOracle)
{
  using GT = TypeParam;
  Check bipartite("compute_bipartite()");
  Check hopcroft_karp("hopcroft_karp_matching()");
  Check by_flow("compute_maximum_cardinality_bipartite_matching()");
  Check blossom("blossom_maximum_cardinality_matching()");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      if (s.n == 0)
        return;
      Built<GT> b(s);
      const bool want_bipartite = oracle::is_bipartite(s);
      const int want_size = oracle::max_matching(s);

      bipartite.run();
      try
        {
          DynDlist<typename GT::Node *> l, r;
          compute_bipartite(b.g, l, r);
          const auto L = numbers(l), R = numbers(r);
          bool ok = want_bipartite and int(L.size() + R.size()) == s.n;
          for (int x : L)
            ok = ok and R.count(x) == 0;
          for (const auto & a : s.arcs)
            ok = ok and L.count(a[0]) != L.count(a[1]);
          if (not ok)
            bipartite.fail(s, want_bipartite ? "gives a wrong partition"
                                             : "does not throw on a graph that is not bipartite");
        }
      catch (const std::domain_error &)
        {
          if (want_bipartite)
            bipartite.fail(s, "throws on a bipartite graph");
        }

      auto check_matching = [&] (Check & check, const DynDlist<typename GT::Arc *> & m)
        {
          std::vector<int> arcs;
          for (auto it = m.get_it(); it.has_curr(); it.next_ne())
            arcs.push_back(num(it.get_curr()));
          if (not oracle::is_matching(s, arcs) or int(arcs.size()) != want_size)
            check.fail(s, std::to_string(arcs.size()) + " arcs, not "
                       + std::to_string(want_size)
                       + (oracle::is_matching(s, arcs) ? "" : ", and not a matching"));
        };

      if (want_bipartite)
        {
          hopcroft_karp.run();
          DynDlist<typename GT::Arc *> m;
          hopcroft_karp_matching(b.g, m);
          check_matching(hopcroft_karp, m);

          by_flow.run();
          try
            {
              DynDlist<typename GT::Arc *> mf;
              compute_maximum_cardinality_bipartite_matching(b.g, mf);
              check_matching(by_flow, mf);
            }
          catch (const std::exception & e)
            {
              by_flow.fail(s, std::string("throws ") + e.what());
            }
        }

      blossom.run();
      DynDlist<typename GT::Arc *> mb;
      const size_t size = blossom_maximum_cardinality_matching(b.g, mb);
      check_matching(blossom, mb);
      if (size != mb.size())
        blossom.fail(s, "returns " + std::to_string(size) + " for "
                     + std::to_string(mb.size()) + " arcs");
    });

  bipartite.expect();
  hopcroft_karp.expect();
  by_flow.expect();
  blossom.expect();
}

TYPED_TEST(ExhaustiveConnectivity, SpanningTreesCoverTheComponentOfTheStart)
{
  using GT = TypeParam;
  Check depth("Find_Depth_First_Spanning_Tree");
  Check breadth("Find_Breadth_First_Spanning_Tree");

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const auto comp = oracle::component_of(s);
      for (int start : starts(s))
        {
          int size = 0;
          for (int x = 0; x < s.n; ++x)
            size += comp[size_t(x)] == comp[size_t(start)];

          for (Check * check : {&depth, &breadth})
            {
              check->run();
              GT tree;
              if (check == &depth)
                Find_Depth_First_Spanning_Tree<GT>()(b.g, b.nodes[size_t(start)], tree);
              else
                Find_Breadth_First_Spanning_Tree<GT>()(b.g, b.nodes[size_t(start)], tree);

              // The tree spans the component of start, and its nodes map
              // to those of g and back
              bool ok = int(tree.get_num_nodes()) == size
                and int(tree.get_num_arcs()) == size - 1;
              std::set<int> seen;
              for (typename GT::Node_Iterator it(tree); it.has_curr() and ok; it.next_ne())
                {
                  auto * t = it.get_curr_ne();
                  auto * p = static_cast<typename GT::Node *>(NODE_COOKIE(t));
                  ok = p != nullptr and num(p) == num(t)
                    and comp[size_t(num(p))] == comp[size_t(start)]
                    and NODE_COOKIE(p) == t and seen.insert(num(p)).second;
                }
              if (not ok)
                check->fail(s, "from " + std::to_string(start) + ": a tree of "
                            + std::to_string(tree.get_num_nodes()) + " nodes and "
                            + std::to_string(tree.get_num_arcs()) + " arcs, not "
                            + std::to_string(size) + " nodes");
            }
        }
    });

  depth.expect();
  breadth.expect();
}

TYPED_TEST(ExhaustiveConnectivity, EulerMatchesTheOracle)
{
  using GT = TypeParam;
  Check euler("Test_Eulerian::compute()");
  Check walk("Find_Eulerian_Path");
  Check nodes("Find_Eulerian_Path::find_node_sequence()");
  // Without arcs, Find_Eulerian_Path said None, and Test_Eulerian Cycle
  Check empty("Find_Eulerian_Path, without arcs");

  auto same_class = [] (Eulerian_Type got, oracle::Euler want)
    {
      return (got == Eulerian_Type::Cycle) == (want == oracle::Euler::Cycle)
        and (got == Eulerian_Type::Path) == (want == oracle::Euler::Path);
    };

  for_each_spec(domain(4, 3), false, [&] (const Spec & s)
    {
      Built<GT> b(s);
      euler.run();
      const auto want = oracle::euler_class(s, false);
      if (not same_class(Test_Eulerian<GT>().compute(b.g), want))
        euler.fail(s, "a wrong class");

      // The walks, where there is one
      if (want == oracle::Euler::None)
        return;
      const bool closed = want == oracle::Euler::Cycle;

      if (s.arcs.empty())
        {
          empty.run();
          const auto r = Find_Eulerian_Path<GT>()(b.g);
          if (r.type != Eulerian_Type::Cycle or not r.path.is_empty())
            empty.fail(s, "says another class, or gives arcs");
          return;
        }

      walk.run();
      const auto r = Find_Eulerian_Path<GT>()(b.g);
      std::vector<int> arcs;
      for (auto it = r.path.get_it(); it.has_curr(); it.next_ne())
        arcs.push_back(num(it.get_curr()));
      if (not same_class(r.type, want) or not oracle::is_euler_walk(s, arcs, false, closed))
        walk.fail(s, "a wrong class or walk");

      // The nodes go along the arcs that the finder gives, from an end
      nodes.run();
      std::vector<int> seq;
      const auto l = Find_Eulerian_Path<GT>().find_node_sequence(b.g);
      for (auto it = l.get_it(); it.has_curr(); it.next_ne())
        seq.push_back(num(it.get_curr()));
      bool ok = seq.size() == arcs.size() + 1;
      for (size_t i = 0; ok and i < arcs.size(); ++i)
        {
          const auto & a = s.arcs[size_t(arcs[i])];
          ok = (a[0] == seq[i] and a[1] == seq[i + 1]) or (a[1] == seq[i] and a[0] == seq[i + 1]);
        }
      if (not ok or (closed and seq.front() != seq.back()))
        nodes.fail(s, "the nodes do not follow the arcs");
    });

  euler.expect();
  walk.expect();
  nodes.expect();
  empty.expect();
}
