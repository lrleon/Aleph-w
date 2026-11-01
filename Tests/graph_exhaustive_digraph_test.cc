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
 * @file graph_exhaustive_digraph_test.cc
 * @brief Strong components, cycles, topological orders, paths, Euler and
 *        dominators on every small digraph, against the oracles.
 *
 * Each test runs one family of algorithms over every digraph of the
 * domain of graph_harness.H (up to 4 nodes and 3 arcs; one more of each
 * with ALEPH_GRAPH_HARNESS=heavy), with loops, parallel and antiparallel
 * arcs and the arcs in two orders, on the three kinds of digraph, and
 * compares each answer with graph_oracles.H. The searches that take a
 * filter run again with every set of hidden arcs, on graphs of up to 3
 * nodes. The last suite runs the searches meant for a graph whose arcs
 * are read from their source to their target on the three kinds of graph
 * that are not directed.
 */

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Tarjan.H>
#include <kosaraju.H>
#include <topological_sort.H>
#include <tpl_find_path.H>
#include <tpl_test_path.H>
#include <Bidirectional_BFS.H>
#include <warshall.H>
#include <eulerian.H>
#include <Dominators.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;

namespace
{
  /// The numbers of the nodes and of the arcs of a path, in order
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

  /// What is wrong with w as a path of s, along the direction of its arcs,
  /// from node w.nodes[0] to the last one, made of arcs that hidden shows;
  /// empty if nothing
  std::string walk_error(const Spec & s, const Walk & w, unsigned long hidden)
  {
    if (w.nodes.empty())
      return "an empty path";
    if (w.arcs.size() + 1 != w.nodes.size())
      return "the path " + w.str() + " lacks arcs";
    for (size_t i = 0; i < w.arcs.size(); ++i)
      {
        const int a = w.arcs[i];
        if (a < 0 or size_t(a) >= s.arcs.size())
          return "the path " + w.str() + " has an arc that is not of the graph";
        if (s.arcs[size_t(a)][0] != w.nodes[i] or s.arcs[size_t(a)][1] != w.nodes[i + 1])
          return "the path " + w.str() + " does not follow the arc " + std::to_string(a);
        if (not graph_filters::shown(hidden, a))
          return "the path " + w.str() + " takes the hidden arc " + std::to_string(a);
      }
    return "";
  }

  /// What is wrong with w as a path of s from u to v without a repeated
  /// node; empty if nothing
  std::string path_error(const Spec & s, const Walk & w, int u, int v,
                         unsigned long hidden = 0)
  {
    if (auto e = walk_error(s, w, hidden); not e.empty())
      return e;
    if (w.nodes.front() != u or w.nodes.back() != v)
      return "the path " + w.str() + " does not go from " + std::to_string(u)
        + " to " + std::to_string(v);
    if (std::set<int>(w.nodes.begin(), w.nodes.end()).size() != w.nodes.size())
      return "the path " + w.str() + " repeats a node";
    return "";
  }

  /// What is wrong with w as a cycle of s: a closed path of one arc or
  /// more that repeats only its first node, at its end; empty if nothing
  std::string cycle_error(const Spec & s, const Walk & w, unsigned long hidden = 0)
  {
    if (auto e = walk_error(s, w, hidden); not e.empty())
      return e;
    if (w.arcs.empty() or w.nodes.front() != w.nodes.back())
      return "the cycle " + w.str() + " is not closed";
    if (std::set<int>(w.nodes.begin() + 1, w.nodes.end()).size() != w.arcs.size())
      return "the cycle " + w.str() + " repeats a node";
    return "";
  }

  template <class List>
  std::vector<int> sequence(const List & l)
  {
    std::vector<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.push_back(num(it.get_curr()));
    return r;
  }

  std::string text(const std::vector<int> & v)
  {
    std::string r = "(";
    for (size_t i = 0; i < v.size(); ++i)
      r += (i == 0 ? "" : " ") + std::to_string(v[i]);
    return r + ")";
  }

  /// Puts in part the block made of the nodes nums, numbered by its
  /// smallest node, as oracle::scc_partition() does; false if a node is
  /// not of the graph or is already in a block
  bool put_block(std::vector<int> & part, const std::vector<int> & nums)
  {
    if (nums.empty())
      return false;
    const int least = *std::min_element(nums.begin(), nums.end());
    for (int x : nums)
      {
        if (x < 0 or size_t(x) >= part.size() or part[size_t(x)] != -1)
          return false;
        part[size_t(x)] = least;
      }
    return true;
  }

  /// The partition that some lists of nodes give; empty if a node is
  /// missing or repeated
  template <class Lists>
  std::vector<int> partition_of_lists(const Lists & blocks, int n)
  {
    std::vector<int> part(size_t(n), -1);
    for (auto it = blocks.get_it(); it.has_curr(); it.next_ne())
      if (not put_block(part, sequence(it.get_curr())))
        return {};
    if (std::count(part.begin(), part.end(), -1) > 0)
      return {};
    return part;
  }

  /// The partition that some mapped subgraphs give, how many times each
  /// arc of s is inside a block, and whether an arc of a block has not the
  /// info of its arc of the graph; part is empty if a node is missing or
  /// repeated, or a node or an arc of a block does not map back to one of
  /// the graph
  template <class GT, class Blocks>
  std::vector<int> partition_of_blocks(const Blocks & blocks, const Spec & s,
                                       std::vector<int> & inside, bool & lost_info)
  {
    std::vector<int> part(size_t(s.n), -1);
    inside.assign(s.arcs.size(), 0);
    lost_info = false;
    for (auto it = blocks.get_it(); it.has_curr(); it.next_ne())
      {
        const GT & blk = it.get_curr();
        std::vector<int> nums;
        for (auto j = blk.get_node_it(); j.has_curr(); j.next_ne())
          {
            auto * bn = j.get_curr();
            auto * gn = mapped_node<GT>(bn);
            if (gn == nullptr or num(gn) != num(bn))
              return {};
            nums.push_back(num(gn));
          }
        if (not put_block(part, nums))
          return {};
        for (auto j = blk.get_arc_it(); j.has_curr(); j.next_ne())
          {
            auto * ba = j.get_curr();
            auto * ga = mapped_arc<GT>(ba);
            if (ga == nullptr or num(ga) < 0 or size_t(num(ga)) >= s.arcs.size()
                or mapped_arc<GT>(ga) != ba)
              return {};
            lost_info = lost_info or num(ga) != num(ba);
            ++inside[size_t(num(ga))];
          }
      }
    if (std::count(part.begin(), part.end(), -1) > 0)
      return {};
    return part;
  }

  /// What is wrong with the blocks of a partition into strong components
  /// with the arcs between them in between, and whether an arc of a block
  /// lost its info; empty if nothing
  template <class GT, class Blocks, class Arcs>
  std::string blocks_error(const Spec & s, const Blocks & blocks, const Arcs & between,
                           bool & lost_info)
  {
    std::vector<int> inside;
    const auto part = partition_of_blocks<GT>(blocks, s, inside, lost_info);
    const auto want = oracle::scc_partition(s);
    if (part != want)
      return "the blocks give " + text(part) + ", not " + text(want);
    for (int a : sequence(between))
      if (a < 0 or size_t(a) >= s.arcs.size() or inside[size_t(a)]++ != 0)
        return "the arc " + std::to_string(a) + " is in two places";
    for (size_t a = 0; a < s.arcs.size(); ++a)
      {
        if (inside[a] != 1)
          return "the arc " + std::to_string(a) + " is missing";
        // An arc of a block joins two nodes of that block
        const bool same = want[size_t(s.arcs[a][0])] == want[size_t(s.arcs[a][1])];
        const auto b = sequence(between);
        if (same == (std::find(b.begin(), b.end(), int(a)) != b.end()))
          return "the arc " + std::to_string(a) + " is on the wrong side";
      }
    return "";
  }

  /// The items of a list from the position from on, iterated as those of
  /// the list
  template <class List>
  struct Tail_Of
  {
    const List & list;
    size_t from = 0;

    struct Iterator
    {
      decltype(std::declval<const List &>().get_it()) it;

      Iterator(const List & l, size_t from) : it(l.get_it())
      {
        for (size_t i = 0; i < from and it.has_curr(); ++i)
          it.next_ne();
      }

      bool has_curr() const { return it.has_curr(); }
      void next_ne() { it.next_ne(); }
      decltype(auto) get_curr() const { return it.get_curr(); }
    };

    Iterator get_it() const { return Iterator(list, from); }
  };

  /// What a block holds, by numbers: its nodes, and its arcs with their
  /// nodes and their infos; or that an arc goes toward a node that is not
  /// of the block
  template <class GT>
  std::string contents_of(const GT & blk)
  {
    std::set<const void *> mine;
    std::vector<int> nodes;
    for (auto j = blk.get_node_it(); j.has_curr(); j.next_ne())
      {
        mine.insert(j.get_curr());
        nodes.push_back(num(j.get_curr()));
      }
    std::vector<std::array<int, 3>> arcs;
    for (auto j = blk.get_arc_it(); j.has_curr(); j.next_ne())
      {
        auto * a = j.get_curr();
        auto * src = blk.get_src_node(a);
        auto * tgt = blk.get_tgt_node(a);
        if (mine.count(src) == 0 or mine.count(tgt) == 0)
          return "an arc toward a node of another block";
        arcs.push_back({num(src), num(tgt), num(a)});
      }
    std::sort(nodes.begin(), nodes.end());
    std::sort(arcs.begin(), arcs.end());
    std::string r = "nodes";
    for (int x : nodes)
      r += " " + std::to_string(x);
    r += "; arcs";
    for (const auto & a : arcs)
      r += " " + std::to_string(a[0]) + "->" + std::to_string(a[1]) + " #" + std::to_string(a[2]);
    return r;
  }

  /// The multiset of the sizes of the blocks of a partition
  std::multiset<size_t> sizes_of(const std::vector<int> & part)
  {
    std::vector<size_t> count(part.size(), 0);
    for (int x : part)
      ++count[size_t(x)];
    std::multiset<size_t> r;
    for (size_t c : count)
      if (c > 0)
        r.insert(c);
    return r;
  }

  int num_blocks(const std::vector<int> & part)
  {
    return int(sizes_of(part).size());
  }

  /// Whether order holds, once each, the nodes that keep says, with every
  /// arc between two of them forward
  bool is_order_of(const Spec & s, const std::vector<int> & order,
                   const std::vector<bool> & keep)
  {
    std::vector<int> pos(size_t(s.n), -1);
    for (size_t i = 0; i < order.size(); ++i)
      {
        const int x = order[i];
        if (x < 0 or x >= s.n or not keep[size_t(x)] or pos[size_t(x)] != -1)
          return false;
        pos[size_t(x)] = int(i);
      }
    for (int x = 0; x < s.n; ++x)
      if (keep[size_t(x)] and pos[size_t(x)] == -1)
        return false;
    for (const auto & a : s.arcs)
      if (keep[size_t(a[0])] and keep[size_t(a[1])] and pos[size_t(a[0])] >= pos[size_t(a[1])])
        return false;
    return true;
  }

  bool same_class(Eulerian_Type got, oracle::Euler want)
  {
    return (got == Eulerian_Type::Cycle) == (want == oracle::Euler::Cycle)
      and (got == Eulerian_Type::Path) == (want == oracle::Euler::Path);
  }

  const char * name_of(oracle::Euler e)
  {
    return e == oracle::Euler::Cycle ? "Cycle" : e == oracle::Euler::Path ? "Path" : "None";
  }

  const char * name_of(Eulerian_Type e)
  {
    return e == Eulerian_Type::Cycle ? "Cycle" : e == Eulerian_Type::Path ? "Path" : "None";
  }

  /// Whether a sequence of nodes walks every arc of s once, in its
  /// direction, back to where it began if closed: its steps are, as a
  /// multiset, the arcs of s
  bool is_euler_node_walk(const Spec & s, const std::vector<int> & nodes, bool closed)
  {
    if (s.arcs.empty())
      return nodes.empty();
    if (nodes.size() != s.arcs.size() + 1)
      return false;
    std::multiset<std::pair<int, int>> want, got;
    for (const auto & a : s.arcs)
      want.insert({a[0], a[1]});
    for (size_t i = 0; i + 1 < nodes.size(); ++i)
      got.insert({nodes[i], nodes[i + 1]});
    return got == want and (not closed or nodes.front() == nodes.back());
  }

  /// The number of a node, or -1 for none
  template <class Node>
  int num_or_none(Node * p)
  {
    return p == nullptr ? -1 : num(p);
  }
}

template <class GT>
class ExhaustiveDigraph : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveDigraph, Directed_Kinds);

TYPED_TEST(ExhaustiveDigraph, StrongComponentsMatchTheOracle)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check lists("Tarjan_Connected_Components::connected_components(g, lists)");
  Check blocks("Tarjan_Connected_Components::connected_components(g, blocks, arcs)");
  Check count("Tarjan_Connected_Components::num_connected_components()");
  Check sizes("Tarjan_Connected_Components::connected_components(g, sizes)");
  Check strong("Tarjan_Connected_Components::test_connectivity()");
  Check k_lists("kosaraju_connected_components(g)");
  Check k_blocks("kosaraju_connected_components(g, blocks, arcs)");
  Check k_infos("kosaraju_connected_components(g, blocks, arcs), the infos");
  Check infos("Tarjan_Connected_Components::connected_components(g, blocks, arcs), the infos");
  Check k_count("kosaraju_scc_count()");
  Check k_strong("is_strongly_connected()");
  Check reused("Tarjan_Connected_Components::connected_components(g, blocks, arcs), "
               "into lists that hold the blocks of g");
  Check k_reused("kosaraju_connected_components(g, blocks, arcs), into lists that hold "
                 "the blocks of g");

  // Runs blocks_of twice into the same lists: the blocks and arcs of the
  // first time do not change, and those of the second go after them, with
  // g mapped to them
  auto check_reused = [] (Check & check, const Spec & s, auto blocks_of)
    {
      check.run();
      DynList<GT> blk;
      DynList<typename GT::Arc *> arcs;
      blocks_of(blk, arcs);
      std::vector<std::string> before;
      for (auto it = blk.get_it(); it.has_curr(); it.next_ne())
        before.push_back(contents_of(it.get_curr()));
      const auto arcs_before = sequence(arcs);

      blocks_of(blk, arcs);
      std::vector<std::string> after;
      for (auto it = blk.get_it(); it.has_curr() and after.size() < before.size(); it.next_ne())
        after.push_back(contents_of(it.get_curr()));
      auto arcs_after = sequence(arcs);
      arcs_after.resize(std::min(arcs_after.size(), arcs_before.size()));
      bool lost = false;
      if (after != before)
        check.fail(s, "the blocks of the first time changed");
      else if (arcs_after != arcs_before)
        check.fail(s, "the arcs between the blocks of the first time changed");
      else if (auto e = blocks_error<GT>(s, Tail_Of<DynList<GT>>{blk, before.size()},
                                         Tail_Of<DynList<typename GT::Arc *>>{arcs, arcs_before.size()},
                                         lost);
               not e.empty())
        check.fail(s, "those of the second time: " + e);
    };

  for_each_spec(domain(4, 3), true, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const auto want = oracle::scc_partition(s);
      const bool want_strong = s.n <= 1 or num_blocks(want) == 1;

      lists.run();
      {
        DynList<DynList<Node *>> l;
        Tarjan_Connected_Components<GT>().connected_components(b.g, l);
        const auto got = partition_of_lists(l, s.n);
        if (got != want)
          lists.fail(s, "gives " + text(got) + ", not " + text(want));
      }

      blocks.run();
      {
        DynList<GT> blk;
        DynList<typename GT::Arc *> arcs;
        Tarjan_Connected_Components<GT>().connected_components(b.g, blk, arcs);
        bool lost = false;
        if (auto e = blocks_error<GT>(s, blk, arcs, lost); not e.empty())
          blocks.fail(s, e);
        infos.run();
        if (lost)
          infos.fail(s, "an arc of a block has not the info of its arc");
      }

      count.run();
      if (const size_t c = Tarjan_Connected_Components<GT>().num_connected_components(b.g);
          int(c) != num_blocks(want))
        count.fail(s, std::to_string(c) + " components, not " + std::to_string(num_blocks(want)));

      sizes.run();
      {
        DynList<size_t> l;
        Tarjan_Connected_Components<GT>().connected_components(b.g, l);
        std::multiset<size_t> got;
        for (auto it = l.get_it(); it.has_curr(); it.next_ne())
          got.insert(it.get_curr());
        if (got != sizes_of(want))
          sizes.fail(s, "the sizes are wrong");
      }

      strong.run();
      if (Tarjan_Connected_Components<GT>().test_connectivity(b.g) != want_strong)
        strong.fail(s, want_strong ? "says not strongly connected" : "says strongly connected");

      k_lists.run();
      if (const auto got = partition_of_lists(kosaraju_connected_components(b.g), s.n);
          got != want)
        k_lists.fail(s, "gives " + text(got) + ", not " + text(want));

      k_blocks.run();
      {
        DynList<GT> blk;
        DynList<typename GT::Arc *> arcs;
        kosaraju_connected_components(b.g, blk, arcs);
        bool lost = false;
        if (auto e = blocks_error<GT>(s, blk, arcs, lost); not e.empty())
          k_blocks.fail(s, e);
        k_infos.run();
        if (lost)
          k_infos.fail(s, "an arc of a block has not the info of its arc");
      }

      k_count.run();
      if (const size_t c = kosaraju_scc_count(b.g); int(c) != num_blocks(want))
        k_count.fail(s, std::to_string(c) + " components, not " + std::to_string(num_blocks(want)));

      k_strong.run();
      if (is_strongly_connected(b.g) != want_strong)
        k_strong.fail(s, want_strong ? "says not strongly connected" : "says strongly connected");

      Tarjan_Connected_Components<GT> tarjan;
      check_reused(reused, s, [&] (DynList<GT> & blk, DynList<typename GT::Arc *> & arcs)
        {
          tarjan.connected_components(b.g, blk, arcs);
        });
      check_reused(k_reused, s, [&] (DynList<GT> & blk, DynList<typename GT::Arc *> & arcs)
        {
          kosaraju_connected_components(b.g, blk, arcs);
        });
    });

  lists.expect();
  blocks.expect();
  infos.expect();
  count.expect();
  sizes.expect();
  strong.expect();
  k_lists.expect();
  k_blocks.expect();
  k_infos.expect();
  k_count.expect();
  k_strong.expect();
  reused.expect();
  k_reused.expect();
}

TYPED_TEST(ExhaustiveDigraph, CyclesMatchTheOracle)
{
  using GT = TypeParam;
  Check has_cycle("Tarjan_Connected_Components::has_cycle()");
  Check is_dag("Tarjan_Connected_Components::is_dag()");
  Check cycle("Tarjan_Connected_Components::compute_cycle(g, path)");
  Check from("Tarjan_Connected_Components::compute_cycle(g, src, path)");
  Check functor("Compute_Cycle_In_Digraph");

  for_each_spec(domain(4, 3), true, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const bool dag = oracle::is_dag(s);
      const auto r = oracle::closure(s, true);

      has_cycle.run();
      if (Tarjan_Connected_Components<GT>().has_cycle(b.g) == dag)
        has_cycle.fail(s, dag ? "finds a cycle" : "finds no cycle");

      is_dag.run();
      if (Tarjan_Connected_Components<GT>().is_dag(b.g) != dag)
        is_dag.fail(s, dag ? "says not a DAG" : "says a DAG");

      cycle.run();
      {
        Path<GT> path(b.g);
        const bool found = Tarjan_Connected_Components<GT>().compute_cycle(b.g, path);
        if (found == dag)
          cycle.fail(s, found ? "finds a cycle in a DAG" : "finds no cycle");
        else if (found)
          if (auto e = cycle_error(s, walk_of(path)); not e.empty())
            cycle.fail(s, e);
      }

      functor.run();
      {
        const Path<GT> path = Compute_Cycle_In_Digraph<GT>()(b.g);
        if (path.is_empty() != dag)
          functor.fail(s, dag ? "finds a cycle in a DAG" : "finds no cycle");
        else if (not dag)
          if (auto e = cycle_error(s, walk_of(path)); not e.empty())
            functor.fail(s, e);
      }

      // From each node, a cycle is found if one is reached
      for (int u = 0; u < s.n; ++u)
        {
          bool reached = false;
          for (int v = 0; v < s.n; ++v)
            reached = reached or (oracle::reaches(r, u, v) and r[size_t(v)][size_t(v)]);

          from.run();
          Path<GT> path(b.g);
          const bool found
            = Tarjan_Connected_Components<GT>().compute_cycle(b.g, b.nodes[size_t(u)], path);
          if (found != reached)
            from.fail(s, "from " + std::to_string(u)
                      + (found ? " finds a cycle that it does not reach" : " finds no cycle"));
          else if (found)
            if (auto e = cycle_error(s, walk_of(path)); not e.empty())
              from.fail(s, "from " + std::to_string(u) + ": " + e);
        }
    });

  has_cycle.expect();
  is_dag.expect();
  cycle.expect();
  from.expect();
  functor.expect();
}

TYPED_TEST(ExhaustiveDigraph, TopologicalOrdersMatchTheOracle)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Check dfs("Topological_Sort::perform()");
  Check dfs_op("Topological_Sort::operator()");
  Check bfs("Q_Topological_Sort::perform()");
  Check ranks("Q_Topological_Sort::ranks()");
  Check bfs_ops("Q_Topological_Sort::operator()");

  for_each_spec(domain(4, 3), true, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const auto rank = oracle::kahn_ranks(s);
      std::vector<bool> keep(size_t(s.n));
      for (int x = 0; x < s.n; ++x)
        keep[size_t(x)] = rank[size_t(x)] != -1;

      // The search in depth is defined on a DAG only
      if (oracle::is_dag(s))
        {
          dfs.run();
          if (const auto o = sequence(Topological_Sort<GT>().template perform<DynList>(b.g));
              not oracle::is_topological_order(s, o))
            dfs.fail(s, "gives " + text(o));

          dfs_op.run();
          DynDlist<Node *> l;
          Topological_Sort<GT>()(b.g, l);
          if (const auto o = sequence(l); not oracle::is_topological_order(s, o))
            dfs_op.fail(s, "gives " + text(o));
        }

      // Kahn's algorithm leaves out the nodes that a cycle reaches
      bfs.run();
      if (const auto o = sequence(Q_Topological_Sort<GT>().template perform<DynList>(b.g));
          not is_order_of(s, o, keep))
        bfs.fail(s, "gives " + text(o));

      auto ranks_error = [&] (const auto & lists)
        {
          std::vector<int> got(size_t(s.n), -1);
          int i = 0;
          for (auto it = lists.get_it(); it.has_curr(); it.next_ne(), ++i)
            for (int x : sequence(it.get_curr()))
              {
                if (x < 0 or x >= s.n or got[size_t(x)] != -1)
                  return std::string("a node in two ranks");
                got[size_t(x)] = i;
              }
          return got == rank ? std::string() : "gives " + text(got) + ", not " + text(rank);
        };

      ranks.run();
      if (auto e = ranks_error(Q_Topological_Sort<GT>().ranks(b.g)); not e.empty())
        ranks.fail(s, e);

      bfs_ops.run();
      {
        DynDlist<DynList<Node *>> dl;
        Q_Topological_Sort<GT>()(b.g, dl);
        DynList<DynList<Node *>> ll;
        Q_Topological_Sort<GT>()(b.g, ll);
        DynDlist<Node *> flat;
        Q_Topological_Sort<GT>()(b.g, flat);
        if (auto e = ranks_error(dl); not e.empty())
          bfs_ops.fail(s, "into a DynDlist of ranks: " + e);
        else if (auto e2 = ranks_error(ll); not e2.empty())
          bfs_ops.fail(s, "into a DynList of ranks: " + e2);
        else if (const auto o = sequence(flat); not is_order_of(s, o, keep))
          bfs_ops.fail(s, "into a list: gives " + text(o));
      }
    });

  dfs.expect();
  dfs_op.expect();
  bfs.expect();
  ranks.expect();
  bfs_ops.expect();
}

namespace
{
  /// The checks of the path searches, against the reachability of the
  /// oracle on the arcs that a filter shows: between two nodes, or from a
  /// node to itself, which has the path of no arc. In a graph that is not
  /// directed, read as directed, only the searches meant for that run.
  struct Path_Checks
  {
    Check dfs;
    Check dfs_out;
    Check bfs;
    Check bfs_out;
    Check directed_dfs;
    Check directed_bfs;
    Check test_function;
    Check test_class;
    Check bibfs;

    explicit Path_Checks(bool self)
      : dfs(name("Find_Path_Depth_First", self)),
        dfs_out(name("Find_Path_Depth_First<Out_Iterator>", self)),
        bfs(name("Find_Path_Breadth_First", self)),
        bfs_out(name("Find_Path_Breadth_First<Out_Iterator>", self)),
        directed_dfs(name("Directed_Find_Path::dfs()", self)),
        directed_bfs(name("Directed_Find_Path::bfs()", self)),
        test_function(name("test_for_path()", self)),
        test_class(name("Test_For_Path", self)),
        bibfs(name("Bidirectional_BFS<Out_Iterator, In_Iterator>", self))
    {}

    static std::string name(const std::string & search, bool self)
    {
      return search + (self ? ", from a node to itself" : "");
    }

    void expect() const
    {
      for (const Check * c : {&dfs, &dfs_out, &bfs, &bfs_out, &directed_dfs, &directed_bfs,
                              &test_function, &test_class, &bibfs})
        c->expect();
    }
  };

  /// Runs the path searches on s, with the arcs of the mask hidden hidden,
  /// from each node to each other one, with the checks pairs, and from
  /// each node to itself, with the checks self. A graph that is not
  /// directed is read as directed, from the source of each arc to its
  /// target, by the searches meant for that.
  template <class GT>
  void run_path_searches(Path_Checks & pairs, Path_Checks & self, const Spec & s,
                         unsigned long hidden)
  {
    using Filter = Hide_Mask<GT>;
    Built<GT> b(s);
    const bool digraph = b.g.is_digraph();
    std::vector<int> orig;
    const Spec shown = oracle::without(s, hidden, orig);
    const auto h = oracle::hops(shown, true);

    for (int u = 0; u < s.n; ++u)
      for (int v = 0; v < s.n; ++v)
        {
          auto * src = b.nodes[size_t(u)];
          auto * tgt = b.nodes[size_t(v)];
          const std::string where = std::to_string(u) + " to " + std::to_string(v)
            + (hidden == 0 ? "" : " hiding " + std::to_string(hidden)) + ": ";
          Path_Checks & c = u == v ? self : pairs;

          const bool want = h[size_t(u)][size_t(v)] != -1;
          auto check_path = [&] (Check & check, bool found, const Path<GT> & path, bool shortest)
            {
              check.run();
              if (found != want)
                {
                  check.fail(s, where + (found ? "finds a path that is not" : "finds no path"));
                  return;
                }
              if (not found)
                return;
              const Walk w = walk_of(path);
              if (auto e = path_error(s, w, u, v, hidden); not e.empty())
                check.fail(s, where + e);
              else if (shortest and int(w.arcs.size()) != h[size_t(u)][size_t(v)])
                check.fail(s, where + "the path " + w.str() + " is not a shortest one");
            };

          auto check_answer = [&] (Check & check, bool found)
            {
              check.run();
              if (found != want)
                check.fail(s, where + (want ? "finds no path" : "finds a path"));
            };

          // Node_Arc_Iterator gives the arcs out of a node in a digraph
          // only
          if (digraph)
            {
              Path<GT> path(b.g);
              const bool found = Find_Path_Depth_First<GT, Node_Arc_Iterator, Filter>
                (Filter{hidden})(b.g, src, tgt, path);
              check_path(c.dfs, found, path, false);

              Path<GT> shortest(b.g);
              const bool found_shortest = Find_Path_Breadth_First<GT, Node_Arc_Iterator, Filter>
                (Filter{hidden})(b.g, src, tgt, shortest);
              check_path(c.bfs, found_shortest, shortest, true);

              if (hidden == 0)
                check_answer(c.test_function, test_for_path(b.g, src, tgt));
              check_answer(c.test_class, Test_For_Path<GT, Filter>(Filter{hidden})(b.g, src, tgt));
            }

          {
            Path<GT> path(b.g);
            const bool found = Find_Path_Depth_First<GT, Out_Iterator, Filter>
              (Filter{hidden})(b.g, src, tgt, path);
            check_path(c.dfs_out, found, path, false);
          }

          {
            Path<GT> path(b.g);
            const bool found = Find_Path_Breadth_First<GT, Out_Iterator, Filter>
              (Filter{hidden})(b.g, src, tgt, path);
            check_path(c.bfs_out, found, path, true);
          }

          {
            const auto path = Directed_Find_Path<GT, Out_Iterator, Filter>
              (b.g, Filter{hidden}).dfs(src, tgt);
            check_path(c.directed_dfs, not path.is_empty(), path, false);

            const auto shortest = Directed_Find_Path<GT, Out_Iterator, Filter>
              (b.g, Filter{hidden}).bfs(src, tgt);
            check_path(c.directed_bfs, not shortest.is_empty(), shortest, true);
          }

          {
            Path<GT> path(b.g);
            const bool found = Bidirectional_BFS<GT, Out_Iterator, In_Iterator, Filter>
              (Filter{hidden})(b.g, src, tgt, path);
            check_path(c.bibfs, found, path, true);
          }
        }
  }

  /// Runs the path searches over the specs of d, hiding each set of arcs
  /// if hiding, in a graph of kind GT
  template <class GT>
  void check_path_searches(const Domain & d, bool hiding)
  {
    Path_Checks pairs(false);
    Path_Checks self(true);
    for_each_spec(d, true, [&] (const Spec & s)
      {
        for (unsigned long hidden = hiding ? 1 : 0;
             hidden < (hiding ? 1UL << s.num_arcs() : 1UL); ++hidden)
          run_path_searches<GT>(pairs, self, s, hidden);
      });
    pairs.expect();
    self.expect();
  }
}

TYPED_TEST(ExhaustiveDigraph, PathsMatchTheReachabilityOfTheOracle)
{
  check_path_searches<TypeParam>(domain(4, 3), false);
}

TYPED_TEST(ExhaustiveDigraph, PathsRespectTheFilter)
{
  check_path_searches<TypeParam>(domain(3, 3), true);
}

TYPED_TEST(ExhaustiveDigraph, WarshallMatchesTheClosureOfTheOracle)
{
  using GT = TypeParam;
  Check warshall("warshall_compute_transitive_clausure()");

  for_each_spec(domain(4, 3), true, [&] (const Spec & s)
    {
      warshall.run();
      Built<GT> b(s);
      Bit_Mat_Graph<GT> mat;
      warshall_compute_transitive_clausure(b.g, mat);
      const auto r = oracle::closure(s, true);
      for (int u = 0; u < s.n; ++u)
        for (int v = 0; v < s.n; ++v)
          if (bool(int(mat(b.nodes[size_t(u)], b.nodes[size_t(v)]))) != r[size_t(u)][size_t(v)])
            {
              warshall.fail(s, "says " + std::string(r[size_t(u)][size_t(v)] ? "no " : "")
                            + "path of one arc or more from " + std::to_string(u)
                            + " to " + std::to_string(v));
              return;
            }
    });

  warshall.expect();
}

TYPED_TEST(ExhaustiveDigraph, EulerMatchesTheOracle)
{
  using GT = TypeParam;
  Check euler("Test_Eulerian::compute()");
  Check walk("Find_Eulerian_Path");
  Check nodes("Find_Eulerian_Path::find_node_sequence()");
  // Without arcs, Find_Eulerian_Path said None, and Test_Eulerian Cycle
  Check empty("Find_Eulerian_Path, without arcs");

  for_each_spec(domain(4, 3), true, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const auto want = oracle::euler_class(s, true);

      euler.run();
      if (const auto got = Test_Eulerian<GT>().compute(b.g); not same_class(got, want))
        euler.fail(s, std::string("says ") + name_of(got) + ", not " + name_of(want));

      if (want == oracle::Euler::None)
        return;
      const bool closed = want == oracle::Euler::Cycle;

      if (s.arcs.empty())
        {
          empty.run();
          const auto r = Find_Eulerian_Path<GT>()(b.g);
          if (r.type != Eulerian_Type::Cycle or not r.path.is_empty())
            empty.fail(s, std::string("says ") + name_of(r.type) + " with "
                       + std::to_string(r.path.size()) + " arcs");
          return;
        }

      walk.run();
      {
        const auto r = Find_Eulerian_Path<GT>()(b.g);
        const auto arcs = sequence(r.path);
        if (not same_class(r.type, want))
          walk.fail(s, std::string("says ") + name_of(r.type) + ", not " + name_of(want));
        else if (not oracle::is_euler_walk(s, arcs, true, closed))
          walk.fail(s, "gives the arcs " + text(arcs));
      }

      nodes.run();
      if (const auto seq = sequence(Find_Eulerian_Path<GT>().find_node_sequence(b.g));
          not is_euler_node_walk(s, seq, closed))
        nodes.fail(s, "gives the nodes " + text(seq));
    });

  euler.expect();
  walk.expect();
  nodes.expect();
  empty.expect();
}

TYPED_TEST(ExhaustiveDigraph, DominatorsMatchTheDefinition)
{
  using GT = TypeParam;
  Check idoms("compute_dominators()");
  Check idom("Lengauer_Tarjan_Dominators::get_idom()");
  Check dominates("Lengauer_Tarjan_Dominators::dominates()");
  Check tree("build_dominator_tree()");
  Check frontiers("compute_dominance_frontiers()");
  Check post("compute_post_dominators()");
  Check post_tree("build_post_dominator_tree()");
  Check post_frontiers("compute_post_dominance_frontiers()");

  // The idoms as numbers, -1 for none, from a list of (node, idom); the
  // nodes not in the list have -2
  auto idoms_of = [] (const auto & l, int n)
    {
      std::vector<int> r(size_t(n), -2);
      for (auto it = l.get_it(); it.has_curr(); it.next_ne())
        r[size_t(num(it.get_curr().first))] = num_or_none(it.get_curr().second);
      return r;
    };

  // The idoms that a tree of arcs from idom to node gives, -2 for the
  // nodes out of the tree; the infos of the tree are the numbers
  auto tree_idoms = [] (const GT & t, int n)
    {
      std::vector<int> r(size_t(n), -2);
      for (auto it = t.get_node_it(); it.has_curr(); it.next_ne())
        r[size_t(num(it.get_curr()))] = -1;
      for (auto it = t.get_arc_it(); it.has_curr(); it.next_ne())
        {
          auto * a = it.get_curr();
          r[size_t(num(t.get_tgt_node(a)))] = num(t.get_src_node(a));
        }
      return r;
    };

  // The idoms of the oracle in the same form: -2 for the nodes not reached
  auto want_idoms = [] (const Spec & s, int root)
    {
      const auto dom = oracle::dominators(s, root);
      auto r = oracle::immediate_dominators(s, root);
      for (int v = 0; v < s.n; ++v)
        if (not dom[size_t(v)][size_t(v)])
          r[size_t(v)] = -2;
      return r;
    };

  // The frontiers as sets of numbers, from a map of node to set of nodes
  auto frontiers_text = [] (const std::vector<std::set<int>> & f)
    {
      std::string r;
      for (size_t x = 0; x < f.size(); ++x)
        {
          r += (x == 0 ? "" : " ") + std::to_string(x) + ":{";
          for (int y : f[x])
            r += (r.back() == '{' ? "" : " ") + std::to_string(y);
          r += "}";
        }
      return r;
    };

  auto frontiers_of = [] (const auto & m, int n)
    {
      std::vector<std::set<int>> r(static_cast<size_t>(n));
      for (auto it = m.get_it(); it.has_curr(); it.next_ne())
        {
          const auto & [node, set] = it.get_curr();
          for (auto j = set.get_it(); j.has_curr(); j.next_ne())
            r[size_t(num(node))].insert(num(j.get_curr()));
        }
      return r;
    };

  for_each_spec(domain(4, 3), true, [&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int root = 0; root < s.n; ++root)
        {
          auto * rp = b.nodes[size_t(root)];
          const auto dom = oracle::dominators(s, root);
          const auto want = want_idoms(s, root);
          const std::string from = "from " + std::to_string(root) + ": ";

          idoms.run();
          if (const auto got = idoms_of(compute_dominators(b.g, rp), s.n); got != want)
            idoms.fail(s, from + "gives " + text(got) + ", not " + text(want));

          Lengauer_Tarjan_Dominators<GT> lt;
          idom.run();
          for (int v = 0; v < s.n; ++v)
            {
              const int w = want[size_t(v)] == -2 ? -1 : want[size_t(v)];
              if (const int got = num_or_none(lt.get_idom(b.g, rp, b.nodes[size_t(v)]));
                  got != w)
                {
                  idom.fail(s, from + "the idom of " + std::to_string(v) + " is "
                            + std::to_string(got) + ", not " + std::to_string(w));
                  break;
                }
            }

          // Every node dominates itself, as dominates() documents, even if
          // it is not reached
          dominates.run();
          for (int d = 0; d < s.n; ++d)
            for (int v = 0; v < s.n; ++v)
              if (const bool want_dom = d == v or dom[size_t(d)][size_t(v)];
                  lt.dominates(b.g, rp, b.nodes[size_t(d)], b.nodes[size_t(v)]) != want_dom)
                {
                  dominates.fail(s, from + std::to_string(d)
                                 + (want_dom ? " dominates " : " does not dominate ")
                                 + std::to_string(v));
                  d = s.n;
                  break;
                }

          tree.run();
          {
            GT t;
            build_dominator_tree(b.g, rp, t);
            if (const auto got = tree_idoms(t, s.n); got != want)
              tree.fail(s, from + "the tree gives " + text(got) + ", not " + text(want));
          }

          frontiers.run();
          {
            const auto got = frontiers_of(compute_dominance_frontiers(b.g, rp), s.n);
            if (const auto f = oracle::dominance_frontiers(s, root); got != f)
              frontiers.fail(s, from + "gives " + frontiers_text(got) + ", not "
                             + frontiers_text(f));
          }

          // The post-dominators are the dominators of the reversed digraph
          const Spec rs = oracle::reversed(s);
          const auto post_want = want_idoms(rs, root);

          post.run();
          if (const auto got = idoms_of(compute_post_dominators(b.g, rp), s.n);
              got != post_want)
            post.fail(s, "to " + std::to_string(root) + ": gives " + text(got)
                      + ", not " + text(post_want));

          post_tree.run();
          {
            GT t;
            build_post_dominator_tree(b.g, rp, t);
            if (const auto got = tree_idoms(t, s.n); got != post_want)
              post_tree.fail(s, "to " + std::to_string(root) + ": the tree gives "
                             + text(got) + ", not " + text(post_want));
          }

          post_frontiers.run();
          {
            const auto got = frontiers_of(compute_post_dominance_frontiers(b.g, rp), s.n);
            if (const auto f = oracle::dominance_frontiers(rs, root); got != f)
              post_frontiers.fail(s, "to " + std::to_string(root) + ": gives "
                                  + frontiers_text(got) + ", not " + frontiers_text(f));
          }
        }
    });

  idoms.expect();
  idom.expect();
  dominates.expect();
  tree.expect();
  frontiers.expect();
  post.expect();
  post_tree.expect();
  post_frontiers.expect();
}

template <class GT>
class ExhaustiveDirectedUse : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveDirectedUse, Undirected_Kinds);

TYPED_TEST(ExhaustiveDirectedUse, PathsMatchTheReachabilityOfTheOracle)
{
  check_path_searches<TypeParam>(domain(4, 3), false);
}

TYPED_TEST(ExhaustiveDirectedUse, PathsRespectTheFilter)
{
  check_path_searches<TypeParam>(domain(3, 3), true);
}
