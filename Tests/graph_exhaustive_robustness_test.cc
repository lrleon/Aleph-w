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
 * @file graph_exhaustive_robustness_test.cc
 * @brief The graph algorithms when an allocation fails, and on a graph
 *        that other algorithms left dirty.
 *
 * This program replaces the global operator new, so that a test can make
 * one allocation fail. Each algorithm runs, on a few small graphs of each
 * kind, with the allocation number k failing, for every k until it runs
 * without reaching it. Each run must either end with the answer that the
 * algorithm gives with memory, or throw std::bad_alloc; then the graph
 * must have its nodes and arcs, and the algorithm, run again with memory,
 * must give its answer. An algorithm that catches the bad_alloc and goes
 * on must still give it. Under ThreadSanitizer nothing is replaced and
 * the tests are skipped; under LeakSanitizer what a failure leaks is
 * reported.
 *
 * Each algorithm must also give its answer on a graph whose nodes and
 * arcs carry what another algorithm could have left in them: every
 * control bit set, a counter, and a cookie.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <tpl_graph.H>
#include <tpl_components.H>
#include <tpl_graph_utils.H>
#include <tpl_cut_nodes.H>
#include <tpl_test_acyclique.H>
#include <tpl_bipartite.H>
#include <tpl_spanning_tree.H>
#include <tpl_find_path.H>
#include <tpl_test_path.H>
#include <Blossom.H>
#include <eulerian.H>
#include <Tarjan.H>
#include <kosaraju.H>
#include <topological_sort.H>
#include <Bidirectional_BFS.H>
#include <warshall.H>
#include <Dominators.H>
#include <Kruskal.H>
#include <Prim.H>
#include <graph_to_tree.H>
#include <generate_df_tree.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"

using namespace Aleph;
using namespace graph_harness;

namespace
{
  /// The graphs of the sweeps: few, with what tends to break a search, a
  /// loop, parallel and antiparallel arcs, a node apart and a cycle
  std::vector<Spec> sweep_specs(bool digraph)
  {
    Spec a;
    a.n = 5;
    a.arcs = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 3}, {1, 2}};
    Spec b;
    b.n = 6;
    b.arcs = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}};
    Spec c;
    c.n = 4;
    c.arcs = digraph ? std::vector<std::array<int, 2>>{{0, 1}, {1, 0}, {1, 2}, {2, 3}, {3, 1}}
                     : std::vector<std::array<int, 2>>{{0, 1}, {2, 3}, {3, 2}};
    for (Spec * s : {&a, &b, &c})
      for (size_t i = 0; i < s->arcs.size(); ++i)
        s->w.push_back(static_cast<long long>(s->arcs.size() - i));
    return {a, b, c};
  }

  std::string text(std::vector<int> v, bool sorted = true)
  {
    if (sorted)
      std::sort(v.begin(), v.end());
    std::string r = "(";
    for (size_t i = 0; i < v.size(); ++i)
      r += (i == 0 ? "" : " ") + std::to_string(v[i]);
    return r + ")";
  }

  template <class List>
  std::vector<int> numbers(const List & l)
  {
    std::vector<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.push_back(num(it.get_curr()));
    return r;
  }

  /// The lists of nodes, each sorted, in sorted order
  template <class Lists>
  std::string text_of_lists(const Lists & lists)
  {
    std::vector<std::string> r;
    for (auto it = lists.get_it(); it.has_curr(); it.next_ne())
      r.push_back(text(numbers(it.get_curr())));
    std::sort(r.begin(), r.end());
    std::string t;
    for (const auto & x : r)
      t += x;
    return t;
  }

  template <class GT>
  std::string text_of_graphs(const DynList<GT> & graphs)
  {
    std::vector<std::string> r;
    for (auto it = graphs.get_it(); it.has_curr(); it.next_ne())
      {
        std::vector<int> nodes, arcs;
        for (auto j = it.get_curr().get_node_it(); j.has_curr(); j.next_ne())
          nodes.push_back(num(j.get_curr()));
        for (auto j = it.get_curr().get_arc_it(); j.has_curr(); j.next_ne())
          arcs.push_back(num(j.get_curr()));
        r.push_back(text(nodes) + text(arcs));
      }
    std::sort(r.begin(), r.end());
    std::string t;
    for (const auto & x : r)
      t += x;
    return t;
  }

  template <class GT>
  std::string text_of_path(const Path<GT> & p)
  {
    std::string r;
    for (typename Path<GT>::Iterator it(p); it.has_current_node(); it.next_ne())
      {
        r += std::to_string(num(it.get_current_node_ne()));
        if (it.has_current_arc())
          r += "-" + std::to_string(num(it.get_current_arc_ne())) + "-";
      }
    return r;
  }

  /// "cycle" if p is a closed walk of s along its arcs, "none" if p is
  /// empty, or the path if it is not a cycle
  template <class GT>
  std::string closed_walk_text(const Path<GT> & p, const Spec & s)
  {
    if (p.is_empty())
      return "none";
    std::vector<int> nodes, arcs;
    for (typename Path<GT>::Iterator it(p); it.has_current_node(); it.next_ne())
      {
        nodes.push_back(num(it.get_current_node_ne()));
        if (it.has_current_arc())
          arcs.push_back(num(it.get_current_arc_ne()));
      }
    bool ok = not arcs.empty() and nodes.front() == nodes.back()
      and nodes.size() == arcs.size() + 1;
    for (size_t i = 0; ok and i < arcs.size(); ++i)
      ok = s.arcs[size_t(arcs[i])][0] == nodes[i] and s.arcs[size_t(arcs[i])][1] == nodes[i + 1];
    return ok ? "cycle" : "not a cycle: " + text_of_path(p);
  }

  /// The nodes and arcs of a tree mapped to g, by number
  template <class GT>
  std::string text_of_tree(const GT & tree)
  {
    std::vector<int> nodes, arcs;
    for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
      nodes.push_back(num(it.get_curr()));
    for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
      arcs.push_back(num(it.get_curr()));
    return text(nodes) + text(arcs);
  }

  /// The arcs of g that carry the bit b
  template <class GT>
  std::string painted(const GT & g, unsigned b)
  {
    std::vector<int> arcs;
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      if (ARC_BITS(it.get_curr()).get_bit(b))
        arcs.push_back(num(it.get_curr()));
    return text(arcs);
  }

  std::string text_of_df_tree(Df_Low_Tree_Node * t)
  {
    const auto & k = t->get_key();
    std::string r = k.label + "," + std::to_string(k.df) + "," + std::to_string(k.low) + "[";
    for (auto * c = t->get_left_child(); c != nullptr; c = c->get_right_sibling())
      r += text_of_df_tree(c);
    return r + "]";
  }

  template <class GT>
  struct Copy_Key
  {
    void operator () (typename GT::Node * p, Tree_Node<int> * t) const
    {
      t->get_key() = p->get_info();
    }
  };

  /// An algorithm on a built graph, which gives its answer as text
  template <class GT>
  struct Algorithm
  {
    std::string name;
    std::function<std::string (Built<GT> &, const Spec &)> run;
  };

  /// The algorithms of the sweeps for a kind of graph
  template <class GT>
  std::vector<Algorithm<GT>> algorithms()
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Dist = graph_filters::Weight_Of<GT, long long>;
    std::vector<Algorithm<GT>> r;
    auto add = [&r] (std::string name, auto f) { r.push_back({std::move(name), f}); };



    if (not GT().is_digraph())
      {
        add("Unconnected_Components, lists", [] (Built<GT> & b, const Spec &)
          {
            DynList<DynList<Node *>> l;
            Unconnected_Components<GT>()(b.g, l);
            return text_of_lists(l);
          });
        add("Unconnected_Components, graphs", [] (Built<GT> & b, const Spec &)
          {
            DynList<GT> l;
            Unconnected_Components<GT>()(b.g, l);
            return text_of_graphs(l);
          });
        add("compute_cut_nodes()", [] (Built<GT> & b, const Spec &)
          {
            return text(numbers(compute_cut_nodes(b.g, b.nodes[0])));
          });
        add("find_bridges()", [] (Built<GT> & b, const Spec &)
          {
            return text(numbers(find_bridges(b.g, b.nodes[0])));
          });
        add("is_graph_acyclique()", [] (Built<GT> & b, const Spec &)
          {
            return std::string(is_graph_acyclique(b.g) ? "acyclic" : "cyclic");
          });
        add("test_for_cycle()", [] (Built<GT> & b, const Spec &)
          {
            return std::string(test_for_cycle(b.g, b.nodes[0]) ? "cycle" : "none");
          });
        add("compute_bipartite()", [] (Built<GT> & b, const Spec &)
          {
            try
              {
                DynDlist<Node *> l, rr;
                compute_bipartite(b.g, l, rr);
                return text(numbers(l)) + text(numbers(rr));
              }
            catch (const std::domain_error &)
              {
                return std::string("not bipartite");
              }
          });
        add("blossom_maximum_cardinality_matching()", [] (Built<GT> & b, const Spec &)
          {
            DynDlist<Arc *> m;
            blossom_maximum_cardinality_matching(b.g, m);
            return text(numbers(m));
          });
        add("Find_Depth_First_Spanning_Tree", [] (Built<GT> & b, const Spec &)
          {
            GT tree;
            Find_Depth_First_Spanning_Tree<GT>()(b.g, b.nodes[0], tree);
            return text_of_tree(tree);
          });
        add("Find_Breadth_First_Spanning_Tree", [] (Built<GT> & b, const Spec &)
          {
            GT tree;
            Find_Breadth_First_Spanning_Tree<GT>()(b.g, b.nodes[0], tree);
            return text_of_tree(tree);
          });
        add("Kruskal_Min_Spanning_Tree", [] (Built<GT> & b, const Spec & s)
          {
            Dist d;
            d.weights = &s.w;
            GT tree;
            Kruskal_Min_Spanning_Tree<GT, Dist> kruskal(d);
            kruskal(b.g, tree);
            return text_of_tree(tree);
          });
        add("Kruskal_Min_Spanning_Tree, painted", [] (Built<GT> & b, const Spec & s)
          {
            Dist d;
            d.weights = &s.w;
            Kruskal_Min_Spanning_Tree<GT, Dist> kruskal(d);
            kruskal(b.g);
            return painted(b.g, Aleph::Spanning_Tree);
          });
        add("Prim_Min_Spanning_Tree", [] (Built<GT> & b, const Spec & s)
          {
            Dist d;
            d.weights = &s.w;
            GT tree;
            Prim_Min_Spanning_Tree<GT, Dist> prim(d);
            prim(b.g, b.nodes[0], tree);
            return text_of_tree(tree);
          });
        add("Prim_Min_Spanning_Tree, painted", [] (Built<GT> & b, const Spec & s)
          {
            Dist d;
            d.weights = &s.w;
            Prim_Min_Spanning_Tree<GT, Dist> prim(d);
            prim(b.g, b.nodes[0]);
            return painted(b.g, Aleph::Spanning_Tree);
          });
        add("build_df_low_tree()", [] (Built<GT> & b, const Spec &)
          {
            DynList<Df_Low_Non_Tree_Arc> non_tree;
            // freed if writing the answer runs out of memory
            const std::unique_ptr<Df_Low_Tree_Node, void (*)(Df_Low_Tree_Node *)>
              t(build_df_low_tree(b.g, b.nodes[0], non_tree),
                [] (Df_Low_Tree_Node * p) { destroy_tree(p); });
            return text_of_df_tree(t.get()) + std::to_string(non_tree.size());
          });
        add("Find_Eulerian_Path", [] (Built<GT> & b, const Spec &)
          {
            const auto res = Find_Eulerian_Path<GT>()(b.g);
            return text(numbers(res.path), false);
          });
        add("Bidirectional_BFS", [] (Built<GT> & b, const Spec & s)
          {
            return text_of_path(Bidirectional_BFS<GT>()(b.g, b.nodes[0], b.nodes[size_t(s.n - 1)]));
          });
      }
    else
      {
        add("Tarjan_Connected_Components, lists", [] (Built<GT> & b, const Spec &)
          {
            DynList<DynList<Node *>> l;
            Tarjan_Connected_Components<GT>().connected_components(b.g, l);
            return text_of_lists(l);
          });
        add("Tarjan_Connected_Components, blocks", [] (Built<GT> & b, const Spec &)
          {
            DynList<GT> blk;
            DynList<Arc *> arcs;
            Tarjan_Connected_Components<GT>().connected_components(b.g, blk, arcs);
            return text_of_graphs(blk) + text(numbers(arcs));
          });
        // The cycle goes through a block whose nodes are new, so that in
        // List_SDigraph, which keeps its nodes by address, it may be
        // another one in each run: the answer is whether it is one
        add("Tarjan_Connected_Components::compute_cycle()", [] (Built<GT> & b, const Spec & s)
          {
            Path<GT> p(b.g);
            (void) Tarjan_Connected_Components<GT>().compute_cycle(b.g, p);
            return closed_walk_text(p, s);
          });
        add("kosaraju_connected_components(), lists", [] (Built<GT> & b, const Spec &)
          {
            return text_of_lists(kosaraju_connected_components(b.g));
          });
        add("kosaraju_connected_components(), blocks", [] (Built<GT> & b, const Spec &)
          {
            DynList<GT> blk;
            DynList<Arc *> arcs;
            kosaraju_connected_components(b.g, blk, arcs);
            return text_of_graphs(blk) + text(numbers(arcs));
          });
        add("Q_Topological_Sort::ranks()", [] (Built<GT> & b, const Spec &)
          {
            const auto ranks = Q_Topological_Sort<GT>().ranks(b.g);
            std::string r;
            for (auto it = ranks.get_it(); it.has_curr(); it.next_ne())
              r += text(numbers(it.get_curr()));
            return r;
          });
        add("compute_dominators()", [] (Built<GT> & b, const Spec &)
          {
            std::vector<int> idoms;
            const auto l = compute_dominators(b.g, b.nodes[0]);
            for (auto it = l.get_it(); it.has_curr(); it.next_ne())
              idoms.push_back(num(it.get_curr().first) * 100
                              + (it.get_curr().second ? num(it.get_curr().second) : 99));
            return text(idoms);
          });
        add("build_dominator_tree()", [] (Built<GT> & b, const Spec &)
          {
            GT tree;
            build_dominator_tree(b.g, b.nodes[0], tree);
            return std::to_string(tree.get_num_nodes()) + "," + std::to_string(tree.get_num_arcs());
          });
        add("compute_post_dominators()", [] (Built<GT> & b, const Spec & s)
          {
            std::vector<int> idoms;
            const auto l = compute_post_dominators(b.g, b.nodes[size_t(s.n - 1)]);
            for (auto it = l.get_it(); it.has_curr(); it.next_ne())
              idoms.push_back(num(it.get_curr().first) * 100
                              + (it.get_curr().second ? num(it.get_curr().second) : 99));
            return text(idoms);
          });
        add("warshall_compute_transitive_clausure()", [] (Built<GT> & b, const Spec & s)
          {
            Bit_Mat_Graph<GT> mat;
            warshall_compute_transitive_clausure(b.g, mat);
            std::string r;
            for (int u = 0; u < s.n; ++u)
              for (int v = 0; v < s.n; ++v)
                r += char('0' + int(mat(b.nodes[size_t(u)], b.nodes[size_t(v)])));
            return r;
          });
      }

    // On both
    add("Find_Path_Depth_First", [] (Built<GT> & b, const Spec & s)
      {
        Path<GT> p(b.g);
        (void) Find_Path_Depth_First<GT>()(b.g, b.nodes[0], b.nodes[size_t(s.n - 1)], p);
        return text_of_path(p);
      });
    add("Find_Path_Breadth_First", [] (Built<GT> & b, const Spec & s)
      {
        Path<GT> p(b.g);
        (void) Find_Path_Breadth_First<GT>()(b.g, b.nodes[0], b.nodes[size_t(s.n - 1)], p);
        return text_of_path(p);
      });
    add("Test_For_Path", [] (Built<GT> & b, const Spec & s)
      {
        return std::string(Test_For_Path<GT>()(b.g, b.nodes[0], b.nodes[size_t(s.n - 1)])
                           ? "path" : "none");
      });
    add("Test_Eulerian::compute()", [] (Built<GT> & b, const Spec &)
      {
        return std::to_string(int(Test_Eulerian<GT>().compute(b.g)));
      });
    add("copy_graph() with the cookies", [] (Built<GT> & b, const Spec &)
      {
        GT c;
        copy_graph(c, b.g, true);
        return text_of_tree(c);
      });
    return r;
  }

  /// What is wrong with the nodes and arcs of b against s; empty if
  /// nothing
  template <class GT>
  std::string structure_error(const Built<GT> & b, const Spec & s)
  {
    if (b.g.get_num_nodes() != size_t(s.n) or b.g.get_num_arcs() != s.arcs.size())
      return "the graph lost nodes or arcs";
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (b.g.get_src_node(b.arcs[i]) != b.nodes[size_t(s.arcs[i][0])]
          or b.g.get_tgt_node(b.arcs[i]) != b.nodes[size_t(s.arcs[i][1])])
        return "the arc " + std::to_string(i) + " lost its ends";
    for (int x = 0; x < s.n; ++x)
      {
        size_t count = 0;
        for (typename GT::Node_Arc_Iterator it(b.nodes[size_t(x)]); it.has_curr(); it.next_ne())
          ++count;
        if (count != b.g.get_num_arcs(b.nodes[size_t(x)]))
          return "the list of the node " + std::to_string(x) + " is broken";
      }
    return "";
  }

  /** Runs algo on b with each allocation failing in turn. Returns what
      went wrong, or empty. A failure that ends the program would end the
      test too: the sweep finds it, but cannot report it.
      ALEPH_GRAPH_HARNESS_TRACE names each sweep on the standard error,
      to tell which one it was. */
  template <class GT>
  std::string sweep(const Algorithm<GT> & algo, Built<GT> & b, const Spec & s)
  {
    if (std::getenv("ALEPH_GRAPH_HARNESS_TRACE") != nullptr)
      std::cerr << algo.name << " on " << s.str() << std::endl;
    const std::string want = algo.run(b, s);
    for (size_t k = 1; k < 1000000; ++k)
      {
        std::string got;
        bool threw = false;
        size_t refused = 0;
        {
          Fail_At fail(k);
          try
            {
              got = algo.run(b, s);
            }
          catch (const std::bad_alloc &)
            {
              threw = true;
            }
          refused = refused_allocations;
        }
        if (auto e = structure_error(b, s); not e.empty())
          return "at the allocation " + std::to_string(k) + ": " + e;
        if (threw)
          {
            if (const auto again = algo.run(b, s); again != want)
              return "after a failure at the allocation " + std::to_string(k)
                + ", it gives " + again + ", not " + want;
            continue;
          }
        // std::stable_sort and the streams of the error messages take
        // memory without throwing, and do without it
        if (got != want)
          return (refused > 0 ? "it caught the failure of the allocation " + std::to_string(k)
                  + " and went on, and gives " : "it gives ") + got + ", not " + want;
        if (refused == 0)
          return ""; // it did not reach the allocation k
      }
    return "it never ends";
  }
}

template <class GT>
class GraphFailAt : public ::testing::Test
{
protected:
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

TYPED_TEST_SUITE(GraphFailAt, All_Kinds);

TYPED_TEST(GraphFailAt, EveryAllocationOfEveryAlgorithmMayFail)
{
  using GT = TypeParam;
  const auto algos = algorithms<GT>();
  std::vector<Check> checks;
  for (const auto & a : algos)
    checks.emplace_back(a.name);

  for (const Spec & s : sweep_specs(GT().is_digraph()))
    for (size_t i = 0; i < algos.size(); ++i)
      {
        checks[i].run();
        Built<GT> b(s);
        if (auto e = sweep(algos[i], b, s); not e.empty())
          checks[i].fail(s, e);
      }

  for (const auto & c : checks)
    c.expect();
}

namespace
{
  /// What a cookie left by another algorithm points to: zeros
  alignas(64) unsigned char left_over[4096] = {};

  /// Leaves in every node and arc of g what another algorithm could have
  /// left: every control bit set, a counter and a cookie
  template <class GT>
  void dirty(const GT & g)
  {
    auto soil = [] (auto * x)
      {
        for (int bit = 0; bit < Num_Bits_Graph; ++bit)
          x->attrs.control_bits.set_bit(bit, 1);
        x->attrs.counter = 1000003;
        x->attrs.cookie = left_over;
      };
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      soil(it.get_curr());
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      soil(it.get_curr());
  }
}

template <class GT>
class GraphDirtyState : public ::testing::Test {};

TYPED_TEST_SUITE(GraphDirtyState, All_Kinds);

TYPED_TEST(GraphDirtyState, EveryAlgorithmIgnoresWhatOthersLeft)
{
  using GT = TypeParam;
  const auto algos = algorithms<GT>();
  std::vector<Check> checks;
  for (const auto & a : algos)
    checks.emplace_back(a.name);

  for (const Spec & s : sweep_specs(GT().is_digraph()))
    for (size_t i = 0; i < algos.size(); ++i)
      {
        checks[i].run();
        Built<GT> b(s);
        const std::string want = algos[i].run(b, s);
        dirty(b.g);
        ASSERT_TRUE(IS_NODE_VISITED(b.nodes[0], Aleph::Depth_First)
                    and NODE_COUNTER(b.nodes[0]) == 1000003
                    and NODE_COOKIE(b.nodes[0]) == left_over);
        if (const std::string got = algos[i].run(b, s); got != want)
          checks[i].fail(s, "gives " + got + ", not " + want);
      }

  for (const auto & c : checks)
    c.expect();
}
