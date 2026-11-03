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
 * @file graph_contract_matrix_core_test.cc
 * @brief The contract matrix of the filtered iterators and of the basic
 *        graph functions: every kind of graph, arc filter and constness
 *        that each template supports.
 *
 * The ArcFilter concept of ah-graph-concepts.H only asks a filter to be a
 * predicate on an arc, and a member of a template that no test calls is
 * never compiled. So each test calls once every public entry point of a
 * template of tpl_graph.H, graph-traverse.H, graph-traverse-generators.H,
 * tpl_graph_utils.H, io_graph.H, generate_graph.H,
 * generate_spanning_tree_picture.H, generate_df_tree.H, graph_to_tree.H,
 * single_graph.H, warshall.H, eulerian.H and hamiltonian.H, on a small
 * graph of each kind that the template supports, with each filter of
 * graph_filter_catalog.H and with Dft_Show_Arc, on a graph and on a const
 * graph where the template takes a const one, and checks, where it is
 * cheap, that the arc that the filters hide does not show up.
 *
 * A combination that should compile and does not is left out with if
 * constexpr, under the name of its check in
 * graph_contract_compile_check.cmake, which expects it to fail to compile
 * until it is fixed. An answer that is wrong at run time because of a
 * known defect is counted, and the test expects it to stay wrong, so that
 * the fix has to remove the mark.
 */

#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
#  include <process.h>
#else
#  include <unistd.h>
#endif

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <graph-traverse.H>
#include <graph-traverse-generators.H>
#include <io_graph.H>
#include <generate_graph.H>
#include <generate_spanning_tree_picture.H>
#include <generate_df_tree.H>
#include <graph_to_tree.H>
#include <single_graph.H>
#include <warshall.H>
#include <eulerian.H>
#include <hamiltonian.H>

#include "graph_harness.H"
#include "graph_filter_catalog.H"
#include "graph_contract_matrix.H"

using namespace Aleph;
using namespace graph_harness;
using namespace graph_contract_matrix;

namespace
{
  /// The number of this process: ctest runs each test in a process of its
  /// own, and several at a time, so that a file of a test has it in its name
  long long process_id() noexcept
  {
#if defined(_WIN32)
    return static_cast<long long>(_getpid());
#else
    return static_cast<long long>(getpid());
#endif
  }


  /// The numbers of the arcs of a container of arcs
  template <class List>
  std::set<int> arc_numbers(const List & l)
  {
    std::set<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.insert(it.get_curr()->get_info());
    return r;
  }
} // namespace

namespace
{
  /// The path 0 - 1 - 2 and the arc 0 - 3, number 2: without it, node 3
  /// is alone
  Spec star_path()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {1, 2}, {0, 3}};
    return s;
  }

  /// A triangle; without its arc 0 - 2, number 2, the path 0 - 1 - 2
  Spec triangle()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {0, 2}};
    return s;
  }

  /// Arc number 2 is the only one out of node 3 and into node 0
  Spec in_and_out()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {1, 2}, {3, 0}};
    return s;
  }

  /// Arc number 2 is parallel to arc number 0
  Spec parallel_pair()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {0, 1}};
    return s;
  }

  /// all, without the arc Hidden if the filter hides it
  std::set<int> visible(std::set<int> all, bool hide)
  {
    if (hide)
      all.erase(Hidden);
    return all;
  }

  /// The numbers of the arcs that an iterator gives from where it is
  template <class It>
  std::set<int> iterated(It & it)
  {
    std::set<int> r;
    for (; it.has_curr(); it.next_ne())
      r.insert(it.get_curr_ne()->get_info());
    return r;
  }

  /// The numbers of the nodes of a container of nodes
  template <class List>
  std::set<int> node_numbers(const List & l)
  {
    std::set<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.insert(it.get_curr()->get_info());
    return r;
  }

  /// The numbers of the arcs of a container of (arc, node) pairs
  template <class List>
  std::set<int> pair_numbers(const List & l)
  {
    std::set<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.insert(std::get<0>(it.get_curr())->get_info());
    return r;
  }

  /// The numbers of the arcs of a graph
  template <class GT>
  std::set<int> graph_arcs(const GT & g)
  {
    std::set<int> r;
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      r.insert(it.get_curr_ne()->get_info());
    return r;
  }

  /// The keys of a tree
  template <class Key>
  std::set<Key> tree_keys(Tree_Node<Key> * root)
  {
    std::set<Key> r;
    root->traverse([&r] (Tree_Node<Key> * p) { r.insert(p->get_key()); return true; });
    return r;
  }

  /// Whether the arcs of GT are in singly linked lists, as in List_SGraph
  /// and List_SDigraph, whose iterators cannot move back
  template <class GT>
  constexpr bool singly_linked = std::is_same_v<typename GT::Node, Graph_Snode<int>>;

  /// Whether a filter can be built with no argument; the functions that
  /// take only its type build it so
  template <class SA>
  constexpr bool default_built = std::is_default_constructible_v<SA>;

  /// A label of the drawing functions, for nodes and arcs
  struct Text
  {
    template <class T>
    std::string operator () (T *) const { return ""; }
  };

  /// The number of the arcs that a drawing writes: the lines ARC and
  /// CURVE-ARC of graphpic, and those of graphviz with an arrow
  size_t drawn_arcs(const std::string & text)
  {
    size_t n = 0;
    std::istringstream in(text);
    for (std::string line; std::getline(in, line); )
      if (line.rfind("ARC ", 0) == 0 or line.rfind("CURVE-ARC ", 0) == 0
          or (line.rfind("//", 0) != 0
              and (line.find("->") != std::string::npos or line.find("--") != std::string::npos)))
        ++n;
    return n;
  }

  /// An operation of Operate_On_Arcs, on a graph or a const graph, that
  /// keeps the numbers of the arcs that it sees
  struct Collect_Arcs
  {
    std::set<int> * seen = nullptr;

    template <class G, class A>
    void operator () (G &, A * a) const { seen->insert(a->get_info()); }
  };

  /// A converter of graph_to_tree.H: the key is the number of the node
  struct To_Key
  {
    template <class N>
    void operator () (N * gnode, Tree_Node<int> * tnode) const
    {
      tnode->get_key() = gnode->get_info();
    }
  };
} // namespace


template <class GT>
class ContractCoreIterators : public ::testing::Test {};

TYPED_TEST_SUITE(ContractCoreIterators, All_Kinds);

TYPED_TEST(ContractCoreIterators, NodeArcIterator)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const auto want = visible({0, 2}, hide);

      Node_Arc_Iterator<GT, SA> it(b.nodes[0], sa);
      Node_Arc_Iterator<GT, SA> copy(it);
      EXPECT_EQ(iterated(copy), want);
      ASSERT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), it.get_curr_ne());
      EXPECT_EQ(it.count(), want.size());
      EXPECT_FALSE(it.empty());
      it.reset_first();
      it.next();
      std::set<int> each;
      it.for_each([&each] (typename GT::Arc * a) { each.insert(a->get_info()); });
      EXPECT_EQ(each, want);
      EXPECT_EQ(it.find_if([] (typename GT::Arc * a) { return a->get_info() == Hidden; }),
                not hide);
      EXPECT_TRUE(it.has_container());
      it.set_cookie(&each);
      EXPECT_EQ(it.get_cookie(), &each);
      (void) it.get_filter();
      (void) std::as_const(it).get_filter();
      (void) it.get_iterator();
      (void) std::as_const(it).get_iterator();
      // get_container() is not called: it gives the address of the
      // parameter of the constructor, which is gone

      // A default iterator builds its filter with SA()
      if constexpr (default_built<SA>)
        {
          Node_Arc_Iterator<GT, SA> none;
          EXPECT_FALSE(none.has_container());
        }

      // set_filter() is only for a filter that can be assigned
      if constexpr (std::is_move_assignable_v<SA>)
        {
          it.set_filter(sa);
          it.reset_first();
          EXPECT_EQ(iterated(it), want);
        }

      // The iterators of List_SGraph and List_SDigraph go only forward:
      // see sgraph_iterator_prev and sgraph_iterator_reset_last
      if constexpr (not singly_linked<GT>)
        {
          std::set<int> back;
          it.reset_last();
          for (int i = 0; i < 8 and it.has_curr(); ++i, it.prev_ne())
            back.insert(it.get_curr_ne()->get_info());
          EXPECT_EQ(back, want);
          it.reset_last();
          if (it.has_curr())
            it.prev();
        }
    });
}

TYPED_TEST(ContractCoreIterators, ArcIterator)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const auto want = visible({0, 1, 2}, hide);
      for_each_constness(b.g, [&] (auto & g)
        {
          Arc_Iterator<GT, SA> it(g, sa);
          Arc_Iterator<GT, SA> copy(it);
          EXPECT_EQ(iterated(copy), want);
          ASSERT_TRUE(it.has_curr());
          EXPECT_EQ(it.get_curr(), it.get_curr_ne());
          EXPECT_EQ(it.count(), want.size());
          EXPECT_FALSE(it.empty());
          it.reset_first();
          it.next();
          std::set<int> each;
          it.for_each([&each] (typename GT::Arc * a) { each.insert(a->get_info()); });
          EXPECT_EQ(each, want);
          EXPECT_EQ(it.find_if([] (typename GT::Arc * a) { return a->get_info() == Hidden; }),
                    not hide);
          EXPECT_EQ(&it.get_container(), &b.g);
          it.set_cookie(&each);
          EXPECT_EQ(it.get_cookie(), &each);
          (void) it.get_filter();
          (void) std::as_const(it).get_filter();

          // known failure sgraph_arc_iterator_default_constructor
          if constexpr (default_built<SA> and not singly_linked<GT>)
            {
              Arc_Iterator<GT, SA> none;
              EXPECT_FALSE(none.has_container());
            }

          // set_filter() is only for a filter that can be assigned
          if constexpr (std::is_move_assignable_v<SA>)
            {
              it.set_filter(sa);
              it.reset_first();
              EXPECT_EQ(iterated(it), want);
            }

          // The iterators of List_SGraph and List_SDigraph go only
          // forward: see sgraph_iterator_prev and sgraph_iterator_reset_last
          if constexpr (not singly_linked<GT>)
            {
              std::set<int> back;
              it.reset_last();
              for (int i = 0; i < 8 and it.has_curr(); ++i, it.prev_ne())
                back.insert(it.get_curr_ne()->get_info());
              EXPECT_EQ(back, want);
              it.reset_last();
              if (it.has_curr())
                it.prev();
            }
        });
    });
}

TYPED_TEST(ContractCoreIterators, OutAndInIterators)
{
  using GT = TypeParam;
  const Spec s = in_and_out();
  Built<GT> b(s);
  // In a digraph the list of a node holds its arcs out: In_Iterator(g, p)
  // walks the arcs of the graph there, and In_Iterator(p), which walks
  // the list of p, is only for the graphs that are not directed
  Check in_arcs("In_Iterator(g, p)");
  Check in_list("In_Iterator(p), if not directed");
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const auto want = visible({2}, hide);

      // Their default constructors are not called: known failure
      // digraph_iterator_default_constructors
      Out_Iterator<GT, SA> out(b.nodes[3], sa);
      Out_Iterator<GT, SA> copy(out);
      EXPECT_EQ(iterated(copy), want);
      out.reset_first();
      if (out.has_curr())
        {
          EXPECT_EQ(out.get_curr(), out.get_curr_ne());
          EXPECT_EQ(out.get_curr(), out.get_current_arc());
          EXPECT_EQ(out.get_node(), b.nodes[0]);
          EXPECT_EQ(out.get_node(out.get_curr()), b.nodes[0]);
          EXPECT_EQ(out.get_tgt_node(), b.nodes[0]);
          EXPECT_EQ(out.get_node_ne(), b.nodes[0]);
          EXPECT_EQ(out.get_tgt_node_ne(), b.nodes[0]);
        }
      EXPECT_EQ(iterated(out), want);
      Out_Iterator<GT, SA> first(b.nodes[0], sa);
      EXPECT_EQ(iterated(first), std::set<int>({0}));
      first.reset_first();
      first.next();
      EXPECT_FALSE(first.has_curr());

      // The iterators of List_SGraph and List_SDigraph go only forward:
      // see sgraph_iterator_prev and sgraph_iterator_reset_last
      if constexpr (requires { out.reset_last(); out.prev(); out.end(); })
        {
          out.reset_last();
          if (out.has_curr())
            out.prev();
          out.end();
          EXPECT_FALSE(out.has_curr());
        }

      // The arcs into node 0 through an iterator in, built on it
      auto check_in = [&] (Check & check, auto & in)
        {
          auto in_copy = in;
          check.run();
          if (iterated(in_copy) != want)
            check.fail(s, std::string(name) + ": the arcs into node 0 are not seen");
          in.reset_first();
          if (in.has_curr())
            {
              EXPECT_EQ(in.get_curr(), in.get_curr_ne());
              EXPECT_EQ(in.get_curr(), in.get_current_arc());
              EXPECT_EQ(in.get_node(), b.nodes[3]);
              EXPECT_EQ(in.get_node(in.get_curr()), b.nodes[3]);
              EXPECT_EQ(in.get_node_ne(), b.nodes[3]);
              EXPECT_EQ(in.get_tgt_node(), b.nodes[3]);
              EXPECT_EQ(in.get_tgt_node_ne(), b.nodes[3]);
              in.next();
            }
          EXPECT_FALSE(in.has_curr());
          in.reset_first();
          if (in.has_curr())
            {
              in.next_ne();
              EXPECT_FALSE(in.has_curr());
            }
          if constexpr (requires { in.reset_last(); in.prev(); })
            {
              in.reset_last();
              EXPECT_EQ(iterated(in), want);
              in.reset_last();
              if (in.has_curr())
                in.prev();
              EXPECT_FALSE(in.has_curr());
            }
          if constexpr (requires { in.end(); })
            {
              in.reset_first();
              in.end();
              EXPECT_FALSE(in.has_curr());
            }
        };

      for_each_constness(b.g, [&] (auto & g)
        {
          In_Iterator<GT, SA> in(g, b.nodes[0], sa);
          check_in(in_arcs, in);
          In_Iterator<GT, SA> rvalue(g, b.nodes[0], SA(sa));
          check_in(in_arcs, rvalue);
          if constexpr (default_built<SA>)
            {
              In_Iterator<GT, SA> by_default(g, b.nodes[0]);
              EXPECT_EQ(iterated(by_default), std::set<int>({2}));
            }
        });

      if constexpr (not DigraphType<GT>)
        {
          In_Iterator<GT, SA> in(b.nodes[0], sa);
          check_in(in_list, in);
        }
    });

  // The iterator of the graph type, which takes no filter
  for_each_constness(b.g, [&] (auto & g)
    {
      typename GT::In_Iterator in(g, b.nodes[0]);
      EXPECT_EQ(iterated(in), std::set<int>({2}));
      auto it = g.get_in_it(b.nodes[0]);
      ASSERT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_node(), b.nodes[3]);
      it.next();
      EXPECT_FALSE(it.has_curr());
    });
  if constexpr (not DigraphType<GT>)
    {
      typename GT::In_Iterator in(b.nodes[0]);
      EXPECT_EQ(iterated(in), std::set<int>({2}));
    }

  in_arcs.expect();
  in_list.expect();
}

/// The iterators move back only where the iterators of the lists of the
/// graph do: not on List_SGraph and List_SDigraph, whose lists are singly
/// linked, and where the calls must not compile (see sgraph_iterator_prev
/// in graph_contract_compile_check.cmake). The iterator of In_Arcs_Index
/// moves back on every kind.
TYPED_TEST(ContractCoreIterators, BackwardMovesWhereTheListsGoBack)
{
  using GT = TypeParam;
  using SA = graph_filters::Hide_Mask<GT>;
  constexpr bool back = not singly_linked<GT>;
  auto goes_back = [] (auto & it) { return requires { it.prev(); it.reset_last(); }; };
  Built<GT> b(star_path());
  Node_Arc_Iterator<GT, SA> node_arc(b.nodes[0], SA{Hidden_Mask});
  Arc_Iterator<GT, SA> arc(b.g, SA{Hidden_Mask});
  Out_Iterator<GT, SA> out(b.nodes[0], SA{Hidden_Mask});
  In_Iterator<GT, SA> in(b.g, b.nodes[0], SA{Hidden_Mask});
  typename GT::Out_Iterator member_out(b.nodes[0]);
  typename GT::In_Iterator member_in(b.g, b.nodes[0]);
  const In_Arcs_Index<GT, SA> index(b.g, SA{Hidden_Mask});
  auto index_it = index.get_it(b.nodes[1]);
  EXPECT_EQ(goes_back(node_arc), back);
  EXPECT_EQ(goes_back(arc), back);
  EXPECT_EQ(goes_back(out), back);
  EXPECT_EQ(goes_back(in), back);
  EXPECT_EQ(goes_back(member_out), back);
  EXPECT_EQ(goes_back(member_in), back);
  EXPECT_TRUE(goes_back(index_it));
  EXPECT_EQ((requires { node_arc.prev_ne(); }), back);
  EXPECT_EQ((requires { out.end(); }), back);
}

template <class GT>
class ContractCoreFunctions : public ::testing::Test {};

TYPED_TEST_SUITE(ContractCoreFunctions, All_Kinds);

TYPED_TEST(ContractCoreFunctions, ForEachArcAndForallArc)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          std::set<int> all, at_0;
          for_each_arc(g, [&all] (typename GT::Arc * a) { all.insert(a->get_info()); }, sa);
          for_each_arc(g, b.nodes[0],
                       [&at_0] (typename GT::Arc * a) { at_0.insert(a->get_info()); }, sa);
          EXPECT_EQ(all, visible({0, 1, 2}, hide));
          EXPECT_EQ(at_0, visible({0, 2}, hide));

          auto not_hidden = [] (typename GT::Arc * a) { return a->get_info() != Hidden; };
          EXPECT_EQ(forall_arc(g, not_hidden, sa), hide);
          EXPECT_EQ(forall_arc<GT>(b.nodes[0], not_hidden, sa), hide);
        });
    });
}

TYPED_TEST(ContractCoreFunctions, ArcsMapAndFoldlArcs)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      auto info = [] (typename GT::Arc * a) { return a->get_info(); };
      auto as_set = [] (const DynList<int> & l)
        {
          std::set<int> r;
          for (auto it = l.get_it(); it.has_curr(); it.next_ne())
            r.insert(it.get_curr());
          return r;
        };
      const auto all = visible({0, 1, 2}, hide);
      const auto at_0 = visible({0, 2}, hide);

      // They take GT &: a const graph is outside their contract
      EXPECT_EQ(as_set(arcs_map<GT, int>(b.g, info, sa)), all);
      EXPECT_EQ(as_set(arcs_map<GT, int>(b.g, b.nodes[0], info, sa)), at_0);
      std::function<int (typename GT::Arc *)> f = info;
      EXPECT_EQ(as_set(arcs_map<GT>(b.g, f, sa)), all);
      EXPECT_EQ(as_set(arcs_map<GT>(b.g, b.nodes[0], f, sa)), at_0);

      auto sum = [] (const int & n, typename GT::Arc * a) { return n + a->get_info(); };
      EXPECT_EQ((foldl_arcs<GT, int>(b.g, 0, sum, sa)), hide ? 1 : 3);
      EXPECT_EQ((foldl_arcs<GT, int>(b.g, b.nodes[0], 0, sum, sa)), hide ? 0 : 2);
    });
}

TYPED_TEST(ContractCoreFunctions, ArcsAndNodesOfANode)
{
  using GT = TypeParam;
  const Spec s = in_and_out();
  Built<GT> b(s);
  // The forms that take the graph, on every kind; those of the node, only
  // where the list of a node holds the arcs into it
  Check ins("in_nodes(g, p), in_arcs(g, p), in_pairs(g, p) and in_degree(g, p)");
  Check ins_list("in_nodes(p), in_arcs(p), in_pairs(p) and in_degree(p), if not directed");
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const auto want = visible({2}, hide);
      const auto want_nodes = hide ? std::set<int>() : std::set<int>({3});
      auto * out_node = b.nodes[3];
      auto * in_node = b.nodes[0];

      EXPECT_EQ(node_numbers(out_nodes<GT>(out_node, sa)),
                hide ? std::set<int>() : std::set<int>({0}));
      EXPECT_EQ(arc_numbers(out_arcs<GT>(out_node, sa)), want);
      EXPECT_EQ(pair_numbers(out_pairs<GT>(out_node, sa)), want);
      EXPECT_EQ(out_degree<GT>(out_node, sa), want.size());
      EXPECT_EQ(arc_numbers(arcs<GT>(in_node, sa)),
                b.g.is_digraph() ? std::set<int>({0}) : visible({0, 2}, hide));

      for_each_constness(b.g, [&] (auto & g)
        {
          ins.run();
          if (node_numbers(in_nodes(g, in_node, sa)) != want_nodes
              or arc_numbers(in_arcs(g, in_node, sa)) != want
              or pair_numbers(in_pairs(g, in_node, sa)) != want
              or in_degree(g, in_node, sa) != want.size()
              or in_degree(g, out_node, sa) != 0)
            ins.fail(s, std::string(name) + ": the arcs into node 0 are not seen");
          if constexpr (default_built<SA>)
            {
              EXPECT_EQ(in_degree(g, in_node), 1U);
            }
        });

      if constexpr (not DigraphType<GT>)
        {
          ins_list.run();
          if (node_numbers(in_nodes<GT>(in_node, sa)) != want_nodes
              or arc_numbers(in_arcs<GT>(in_node, sa)) != want
              or pair_numbers(in_pairs<GT>(in_node, sa)) != want
              or in_degree<GT>(in_node, sa) != want.size())
            ins_list.fail(s, std::string(name) + ": the arcs into node 0 are not seen");
        }
    });

  ins.expect();
  ins_list.expect();
}

TYPED_TEST(ContractCoreFunctions, InArcsIndex)
{
  using GT = TypeParam;
  const Spec s = in_and_out();
  Built<GT> b(s);
  Check index("In_Arcs_Index");
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const auto want = visible({2}, hide);
      for_each_constness(b.g, [&] (auto & g)
        {
          // The filter as an lvalue and as an rvalue
          const In_Arcs_Index<GT, SA> in(g, sa);
          const In_Arcs_Index<GT, SA> rvalue(g, SA(sa));
          index.run();
          auto it = in.get_it(b.nodes[0]);
          auto rt = rvalue.get_it(b.nodes[0]);
          if (iterated(it) != want or in.in_degree(b.nodes[0]) != want.size()
              or in.in_degree(b.nodes[3]) != 0 or in.size() != visible({0, 1, 2}, hide).size()
              or iterated(rt) != want)
            index.fail(s, std::string(name) + ": the arcs into node 0 are not seen");

          typename In_Arcs_Index<GT, SA>::Iterator kt(in, b.nodes[0]);
          EXPECT_EQ(iterated(kt), want);

          auto jt = in.get_it(b.nodes[0]);
          EXPECT_EQ(jt.size(), want.size());
          if (jt.has_curr())
            {
              EXPECT_EQ(jt.get_curr(), jt.get_curr_ne());
              EXPECT_EQ(jt.get_curr(), jt.get_current_arc());
              EXPECT_EQ(jt.get_current_arc_ne(), b.arcs[2]);
              EXPECT_EQ(jt.get_node(), b.nodes[3]);
              EXPECT_EQ(jt.get_node_ne(), b.nodes[3]);
              jt.next();
              EXPECT_FALSE(jt.has_curr());
              EXPECT_THROW(jt.next(), std::overflow_error);
              jt.reset_last();
              EXPECT_TRUE(jt.has_curr());
              jt.prev();
              EXPECT_FALSE(jt.has_curr());
              EXPECT_THROW((void) jt.get_curr(), std::underflow_error);
              jt.reset_first();
              jt.end();
              EXPECT_FALSE(jt.has_curr());
            }
          else
            EXPECT_THROW((void) jt.get_curr(), std::overflow_error);

          if constexpr (default_built<SA>)
            {
              const In_Arcs_Index<GT, SA> all(g);
              EXPECT_EQ(all.in_degree(b.nodes[0]), 1U);
              EXPECT_EQ(all.size(), 3U);
            }

          // A node of another graph
          GT other;
          auto * q = other.insert_node(0);
          EXPECT_THROW((void) in.in_degree(q), std::domain_error);
          EXPECT_THROW((void) in.get_it(q), std::domain_error);
        });
    });

  // On an empty graph
  GT empty;
  const In_Arcs_Index<GT> nothing(empty);
  EXPECT_EQ(nothing.size(), 0U);

  index.expect();
}

TYPED_TEST(ContractCoreFunctions, FunctionalOperationsOnTheArcsIntoANode)
{
  using GT = TypeParam;
  using Arc = typename GT::Arc;
  Built<GT> b(in_and_out());
  auto * p = b.nodes[0];
  auto info = [] (Arc * a) { return a->get_info(); };
  auto is_2 = [] (Arc * a) { return a->get_info() == Hidden; };
  auto sum = [] (const int & n, Arc * a) { return n + a->get_info(); };
  auto as_set = [] (const DynList<int> & l)
    {
      std::set<int> r;
      for (auto it = l.get_it(); it.has_curr(); it.next_ne())
        r.insert(it.get_curr());
      return r;
    };
  std::function<int (Arc *)> f = info;

  for_each_constness(b.g, [&] (auto & g)
    {
      std::set<int> each;
      for_each_in_arc(g, p, [&each] (Arc * a) { each.insert(a->get_info()); });
      EXPECT_EQ(each, std::set<int>({2}));
      EXPECT_TRUE(traverse_in_arcs(g, p, is_2));
      EXPECT_TRUE(all_in_arc(g, p, is_2));
      EXPECT_TRUE(exists_in_arc(g, p, is_2));
      EXPECT_FALSE(exists_in_arc(g, b.nodes[3], is_2));
      EXPECT_EQ(search_in_arc(g, p, is_2), b.arcs[2]);
      EXPECT_EQ(as_set(map_in_arcs<GT, int>(g, p, info)), std::set<int>({2}));
      EXPECT_EQ(as_set(map_in_arcs<GT>(g, p, f)), std::set<int>({2}));
      EXPECT_EQ((foldl_in_arcs<GT, int>(g, p, 0, sum)), 2);
      EXPECT_EQ(arc_numbers(filter_in_arcs(g, p, is_2)), std::set<int>({2}));
    });

  // The members, and the forms of the node where the list holds the arcs
  // into it
  EXPECT_EQ(arc_numbers(b.g.in_arcs(p)), std::set<int>({2}));
  EXPECT_EQ(b.g.in_degree(p), 1U);
  EXPECT_EQ(b.g.search_in_arc(p, is_2), b.arcs[2]);
  EXPECT_EQ(arc_numbers(b.g.filter_in_arcs(p, is_2)), std::set<int>({2}));
  EXPECT_EQ(b.g.foldl_in_arcs(p, 0, sum), 2);
  if constexpr (not DigraphType<GT>)
    {
      EXPECT_TRUE(traverse_in_arcs<GT>(p, is_2));
      EXPECT_TRUE(exists_in_arc<GT>(p, is_2));
      EXPECT_EQ(search_in_arc<GT>(p, is_2), b.arcs[2]);
      EXPECT_EQ(as_set(map_in_arcs<GT, int>(p, info)), std::set<int>({2}));
      EXPECT_EQ((foldl_in_arcs<GT, int>(p, 0, sum)), 2);
      EXPECT_EQ(arc_numbers(filter_in_arcs<GT>(p, is_2)), std::set<int>({2}));
    }
}

TYPED_TEST(ContractCoreFunctions, SearchArc)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      auto * want = hide ? nullptr : b.arcs[Hidden];
      for_each_constness(b.g, [&] (auto & g)
        {
          EXPECT_EQ(search_arc(g, b.nodes[0], b.nodes[3], sa), want);
          EXPECT_EQ(search_directed_arc(g, b.nodes[0], b.nodes[3], sa), want);
          EXPECT_EQ(search_arc(g, b.nodes[0], b.nodes[1], sa), b.arcs[0]);
        });
    });
}

TYPED_TEST(ContractCoreFunctions, KeptFilter)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // An lvalue is used where it is; an rvalue is moved in, and copied
      // with the keeper
      Kept_Filter<SA> by_lvalue(sa);
      Kept_Filter<SA> by_rvalue{SA(sa)};
      Kept_Filter<SA> copy(by_rvalue);
      Kept_Filter<SA> moved(std::move(copy));
      SA & f = by_lvalue;
      EXPECT_EQ(&f, &sa);
      EXPECT_NE(&by_rvalue.get(), &sa);
      EXPECT_EQ(bool(f(b.arcs[Hidden])), not hide);
      EXPECT_EQ(bool(moved.get()(b.arcs[Hidden])), not hide);
      EXPECT_TRUE(bool(by_rvalue.get()(b.arcs[0])));
    });
}

TYPED_TEST(ContractCoreFunctions, OperateOnArcs)
{
  using GT = TypeParam;
  const Spec s = star_path();
  Built<GT> b(s);
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const Operate_On_Arcs<GT, Collect_Arcs, SA> op(sa);
      for_each_constness(b.g, [&] (auto & g)
        {
          std::set<int> all;
          op(g, Collect_Arcs{&all});
          EXPECT_EQ(all, visible({0, 1, 2}, hide));

          std::set<int> at_0;
          op(g, b.nodes[0], Collect_Arcs{&at_0});
          EXPECT_EQ(at_0, visible({0, 2}, hide));
        });
    });
}

TYPED_TEST(ContractCoreFunctions, CopyGraph)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // It takes GT & for the source: a const graph is outside its contract
      Copy_Graph<GT, Dft_Show_Node<GT>, SA> copy(sa);
      GT tgt;
      copy(tgt, b.g);
      EXPECT_EQ(tgt.get_num_nodes(), 4U);
      EXPECT_EQ(graph_arcs(tgt), visible({0, 1, 2}, hide));
      copy(tgt, b.g, false);
      EXPECT_EQ(graph_arcs(tgt), visible({0, 1, 2}, hide));
    });
}

template <class GT>
class ContractCoreTraversals : public ::testing::Test {};

TYPED_TEST_SUITE(ContractCoreTraversals, All_Kinds);

TYPED_TEST(ContractCoreTraversals, GraphTraverse)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      using Itor = Node_Arc_Iterator<GT, SA>;
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const size_t want = hide ? 3 : 4;
      auto node_op = [] (typename GT::Node *) { return true; };
      auto pair_op = [] (typename GT::Node *, typename GT::Arc *) { return true; };
      auto arc_op = [] (typename GT::Arc *) { return true; };

      // They take GT &: a const graph is outside their contract
      Graph_Traverse_DFS<GT, Itor, SA> dfs(b.g, sa);
      EXPECT_EQ(dfs(b.nodes[0], node_op), want);
      EXPECT_EQ(dfs(b.nodes[0], [] (typename GT::Node *) { return true; }), want);
      EXPECT_EQ(dfs.exec(b.nodes[0], pair_op), want);
      EXPECT_EQ(dfs.exec(b.nodes[0],
                         [] (typename GT::Node *, typename GT::Arc *) { return true; }),
                want);
      EXPECT_EQ(std::get<0>(dfs(b.nodes[0], node_op, arc_op)), want);
      EXPECT_EQ(std::get<0>(dfs(b.nodes[0], [] (typename GT::Node *) { return true; },
                                [] (typename GT::Arc *) { return true; })),
                want);

      Graph_Traverse_BFS<GT, Itor, SA> bfs(b.g, sa);
      EXPECT_EQ(bfs(b.nodes[0], node_op), want);
      EXPECT_EQ(bfs.exec(b.nodes[0], pair_op), want);
      EXPECT_EQ(std::get<0>(bfs(b.nodes[0], node_op, arc_op)), want);
    });
}

TYPED_TEST(ContractCoreTraversals, GraphTraverseGenerator)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      using Itor = Node_Arc_Iterator<GT, SA>;
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const std::set<int> want =
        hide ? std::set<int>({0, 1, 2}) : std::set<int>({0, 1, 2, 3});

      // They take GT &: a const graph is outside their contract
      Graph_Traverse_DFS_Generator<GT, Itor, SA> dfs(b.g, sa);
      Graph_Traverse_BFS_Generator<GT, Itor, SA> bfs(b.g, sa);
      std::set<int> by_dfs, by_bfs;
      for (typename GT::Node * p : dfs.traverse(b.nodes[0]))
        by_dfs.insert(p->get_info());
      for (typename GT::Node * p : bfs.traverse(b.nodes[0]))
        by_bfs.insert(p->get_info());
      EXPECT_EQ(by_dfs, want);
      EXPECT_EQ(by_bfs, want);
    });
}

TYPED_TEST(ContractCoreTraversals, DepthAndBreadthFirstTraversal)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const size_t want = hide ? 3 : 4;
      for_each_constness(b.g, [&] (auto & g)
        {
          // The first node of a List_SGraph follows the addresses, so that
          // the traversals from it are not checked

          Depth_First_Traversal<GT, Default_Visit_Op<GT>, SA> dft(sa);
          (void) dft(g);
          (void) dft(g, Default_Visit_Op<GT>());
          EXPECT_EQ(dft(g, b.nodes[0]), want);
          EXPECT_EQ(dft(g, b.nodes[0], Default_Visit_Op<GT>()), want);

          Breadth_First_Traversal<GT, Default_Visit_Op<GT>, SA> bft(sa);
          Default_Visit_Op<GT> op;
          (void) bft(g, Default_Visit_Op<GT>());
          EXPECT_EQ(bft(g, b.nodes[0]), want);
          EXPECT_EQ(bft(g, b.nodes[0], Default_Visit_Op<GT>()), want);
          EXPECT_EQ(bft(g, b.nodes[0], op), want);
        });
    });
}

TYPED_TEST(ContractCoreTraversals, SearchSpanningTreeArcAndTotalCost)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          EXPECT_EQ(search_spanning_tree_arc(g, b.nodes[0], b.nodes[3], sa),
                    hide ? nullptr : b.arcs[Hidden]);
        });

      // It takes GT &: a const graph is outside its contract
      Total_Cost<GT, Dft_Dist<GT>, SA> cost(Dft_Dist<GT>(), sa);
      EXPECT_EQ(cost.total_cost(b.g), hide ? 1 : 3);
      EXPECT_EQ(cost(b.g), hide ? 1 : 3);
      cost.reset();
      EXPECT_TRUE(cost(b.arcs[Hidden]));
      EXPECT_EQ(cost.value(), hide ? 0 : Hidden);
    });
}

TYPED_TEST(ContractCoreTraversals, IOGraph)
{
  using GT = TypeParam;
  const Spec s = star_path();
  Built<GT> b(s);
  const std::string file = ::testing::TempDir() + "graph_contract_matrix_core_"
    + std::to_string(process_id()) + ".bin";
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      using IO = IO_Graph<GT, Dft_Load_Node<GT>, Dft_Store_Node<GT>, Dft_Load_Arc<GT>,
                          Dft_Store_Arc<GT>, Dft_Show_Node<GT>, SA>;
      // It takes GT &: a const graph is outside its contract
      IO io(b.g, sa);
      IO by_pointer(&b.g, sa);
      by_pointer.set_verbose(false);
      by_pointer.set_load_node(Dft_Load_Node<GT>());
      by_pointer.set_store_node(Dft_Store_Node<GT>());
      by_pointer.set_load_arc(Dft_Load_Arc<GT>());
      by_pointer.set_store_arc(Dft_Store_Arc<GT>());
      by_pointer.set_node_filter(Dft_Show_Node<GT>());
      EXPECT_FALSE(by_pointer.is_verbose());

      // Whether what save_in_text_mode() and save() of saver write loads
      // back as the graph without the arcs that hidden says
      auto text_round = [&] (IO & saver, bool hidden)
        {
          std::ostringstream text;
          saver.save_in_text_mode(text);
          GT h;
          IO loader(h, sa);
          std::istringstream in(text.str());
          loader.load_in_text_mode(in);
          return h.get_num_nodes() == 4 and graph_arcs(h) == visible({0, 1, 2}, hidden);
        };
      auto binary_round = [&] (IO & saver, bool hidden)
        {
          {
            std::ofstream out(file, std::ios::binary);
            saver.save(out);
          }
          GT h;
          IO loader(h, sa);
          std::ifstream in(file, std::ios::binary);
          loader.load(in);
          return h.get_num_nodes() == 4 and graph_arcs(h) == visible({0, 1, 2}, hidden);
        };

      EXPECT_TRUE(text_round(io, hide));
      EXPECT_TRUE(binary_round(io, hide));
      EXPECT_TRUE(text_round(by_pointer, hide));

      // IO_Graph(g) builds AF(), which hides nothing, and set_arc_filter()
      // assigns the filter: for those that have both
      if constexpr (default_built<SA>)
        {
          IO later(b.g);
          EXPECT_TRUE(text_round(later, false));
          if constexpr (std::is_copy_assignable_v<SA>)
            {
              later.set_arc_filter(sa);
              EXPECT_TRUE(text_round(later, hide));
              EXPECT_TRUE(binary_round(later, hide));
            }
        }
    });
  std::remove(file.c_str());
}

TYPED_TEST(ContractCoreTraversals, Drawings)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      const size_t arcs = hide ? 2 : 3;
      for_each_constness(b.g, [&] (auto & g)
        {
          std::ostringstream pic, dot1, dot2, dot3, dot4, dot5;
          generate_graphpic<GT, Text, Text, Text, Text, SA>(g, 1.0, 1.0, pic, sa);
          generate_graphviz<GT, Text, Text, Dummy_Attr<GT>, Dummy_Attr<GT>,
                            Dummy_Attr<GT>, Dummy_Attr<GT>, SA, Dft_Show_Node<GT>>
            (g, dot1, "TB", 0.2, 0.2, sa);
          generate_graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>, Dft_Show_Node<GT>, SA>
            (g, dot2, Dft_Node_Attr<GT>(), Dft_Arc_Attr<GT>(), "TB", sa);
          digraph_graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>, Dft_Show_Node<GT>, SA>
            (g, dot3, Dft_Node_Attr<GT>(), Dft_Arc_Attr<GT>(), "LR", sa);
          To_Graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>, Dft_Show_Node<GT>, SA> to_dot(sa);
          to_dot(g, dot4);
          to_dot.digraph(g, dot5);
          for (const auto * out : {&pic, &dot1, &dot2, &dot3, &dot4, &dot5})
            EXPECT_EQ(drawn_arcs(out->str()), arcs);
        });

      // They take GT &: a const graph is outside their contract
      std::ostringstream dot, cross1, cross2, net1, net2;
      Generate_Graphviz<GT, Text, Text, Dummy_Attr<GT>, Dummy_Attr<GT>,
                        Dummy_Attr<GT>, Dummy_Attr<GT>, SA> generate(sa);
      generate(b.g, dot);
      generate_cross_graph<GT, Text, Text, Text, Text, SA>(b.g, 2, 1.0, 1.0, cross1, sa);
      generate_cross_graph<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, cross2, sa);
      generate_net_graph<GT, Text, Text, Text, Text, SA>(b.g, 2, 1.0, 1.0, net1, sa);
      generate_net_graph<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, net2, sa);
      for (const auto * out : {&dot, &cross1, &cross2, &net1, &net2})
        EXPECT_EQ(drawn_arcs(out->str()), arcs);

      std::ofstream nowhere;
      generate_cross_spanning_tree<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, nowhere, sa);
      generate_net_spanning_tree<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, nowhere, sa);

      // Without the object they build SA(), which hides nothing
      if constexpr (default_built<SA>)
        {
          std::ostringstream pic, dot6, cross3;
          generate_graphpic<GT, Text, Text, Text, Text, SA>(b.g, 1.0, 1.0, pic);
          To_Graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>, Dft_Show_Node<GT>, SA>()(b.g, dot6);
          generate_cross_graph<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, cross3);
          for (const auto * out : {&pic, &dot6, &cross3})
            EXPECT_EQ(drawn_arcs(out->str()), 3U);
          generate_cross_spanning_tree<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, nowhere);
          generate_net_spanning_tree<GT, Text, Text, SA>(b.g, 2, 1.0, 1.0, nowhere);
        }
    });

  // The overloads without a filter
  std::ostringstream cross, net;
  generate_cross_graph<GT, Text, Text, Text, Text>(b.g, 2, 1.0, 1.0, cross);
  generate_cross_graph<GT, Text, Text>(b.g, 2, 1.0, 1.0, cross);
  generate_net_graph<GT, Text, Text, Text, Text>(b.g, 2, 1.0, 1.0, net);
  generate_net_graph<GT, Text, Text>(b.g, 2, 1.0, 1.0, net);
  EXPECT_FALSE(cross.str().empty());
  EXPECT_FALSE(net.str().empty());
  std::ofstream nowhere;
  generate_cross_spanning_tree<GT, Text, Text>(b.g, 2, 1.0, 1.0, nowhere);
  generate_net_spanning_tree<GT, Text, Text>(b.g, 2, 1.0, 1.0, nowhere);
}

TYPED_TEST(ContractCoreTraversals, Warshall)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // It takes GT &: a const graph is outside its contract. Without the
      // arc Hidden, 0 - 3, no node reaches 3
      Bit_Mat_Graph<GT, SA> mat(b.g, sa);
      EXPECT_EQ(int(mat(b.nodes[0], b.nodes[3])), hide ? 0 : 1);
      warshall_compute_transitive_clausure(b.g, mat, sa);
      EXPECT_EQ(int(mat(b.nodes[0], b.nodes[2])), 1);
      EXPECT_EQ(int(mat(b.nodes[0], b.nodes[3])), hide ? 0 : 1);

      Bit_Mat_Graph<GT, SA> by_functor(b.g, sa);
      const Warshall_Compute_Transitive_Clausure<GT, SA> closure(sa);
      closure(b.g, by_functor);
      EXPECT_EQ(int(by_functor(b.nodes[0], b.nodes[2])), 1);
      EXPECT_EQ(int(by_functor(b.nodes[0], b.nodes[3])), hide ? 0 : 1);

      // Without the object, SA(), which hides nothing
      if constexpr (default_built<SA>)
        {
          Bit_Mat_Graph<GT, SA> all(b.g);
          warshall_compute_transitive_clausure(b.g, all);
          EXPECT_EQ(int(all(b.nodes[0], b.nodes[3])), 1);
          Warshall_Compute_Transitive_Clausure<GT, SA>()(b.g, all);
          EXPECT_EQ(int(all(b.nodes[0], b.nodes[3])), 1);
        }
    });
}

TYPED_TEST(ContractCoreTraversals, SingleGraph)
{
  using GT = TypeParam;
  const Spec s = parallel_pair();
  Built<GT> b(s);
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // It takes the filters only as rvalues, and GT &: an lvalue filter
      // and a const graph are outside its contract
      Test_Single_Graph<GT, Dft_Show_Node<GT>, SA> single{Dft_Show_Node<GT>(), SA(sa)};
      // Simple only without the parallel arc
      EXPECT_EQ(single(b.g), hide);
      EXPECT_EQ(single.test_node(b.nodes[0]), hide);
    });
}

TYPED_TEST(ContractCoreTraversals, EulerAndHamilton)
{
  using GT = TypeParam;
  const Spec s = triangle();
  Built<GT> b(s);
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // They take the filters only as rvalues, and GT &: an lvalue filter
      // and a const graph are outside their contract
      Test_Eulerian<GT, Dft_Show_Node<GT>, SA> test{Dft_Show_Node<GT>(), SA(sa)};
      const Eulerian_Type type = test.compute(b.g);
      (void) test(b.g);
      (void) test.has_eulerian_path(b.g);
      (void) test.test_degree_only(b.g);
      // Without arc 2 the triangle is a path; with it, a cycle, or nothing
      // in a digraph, whose arcs are 0 -> 1, 1 -> 2 and 0 -> 2
      const Eulerian_Type want = hide ? Eulerian_Type::Path
        : b.g.is_digraph() ? Eulerian_Type::None : Eulerian_Type::Cycle;
      EXPECT_EQ(type, want);

      Find_Eulerian_Path<GT, Dft_Show_Node<GT>, SA> find{Dft_Show_Node<GT>(), SA(sa)};
      const auto r = find(b.g);
      if (hide)
        {
          EXPECT_EQ(arc_numbers(r.path).count(Hidden), 0U);
        }
      (void) find.find_path(b.g);
      (void) find.find_node_sequence(b.g);

      Test_Hamiltonian_Sufficiency<GT, Dft_Show_Node<GT>, SA> ore{Dft_Show_Node<GT>(), SA(sa)};
      Test_Dirac_Condition<GT, Dft_Show_Node<GT>, SA> dirac{Dft_Show_Node<GT>(), SA(sa)};
      const bool by_ore = ore(b.g);
      const bool by_dirac = dirac(b.g);
      EXPECT_EQ(dirac.min_required_degree(b.g), 2U);
      EXPECT_NE(dirac.find_min_degree_vertex(b.g).second, nullptr);
      if (not b.g.is_digraph())
        {
          // The triangle satisfies both conditions; the path 0 - 1 - 2,
          // neither
          EXPECT_EQ(by_ore, not hide);
          EXPECT_EQ(by_dirac, not hide);
        }
    });
}

template <class GT>
class ContractCoreDigraphs : public ::testing::Test {};

TYPED_TEST_SUITE(ContractCoreDigraphs, Directed_Kinds);

TYPED_TEST(ContractCoreDigraphs, InvertDigraphAndRanks)
{
  using GT = TypeParam;
  Built<GT> b(star_path());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          const Invert_Digraph<GT, SA> invert(sa);
          GT inverted = invert(g);
          EXPECT_EQ(inverted.get_num_nodes(), 4U);
          EXPECT_EQ(graph_arcs(inverted), visible({0, 1, 2}, hide));

          // The ranks are those of the arcs shown: {0, 3}, {1}, {2} without
          // the arc Hidden, 0 -> 3; {0}, {1, 3}, {2} with it
          std::ostringstream ranks, dot;
          EXPECT_EQ((rank_graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>, Dft_Show_Node<GT>, SA>
                     (g, ranks, Dft_Node_Attr<GT>(), Dft_Arc_Attr<GT>(), "LR", sa)), 3U);
          EXPECT_EQ(drawn_arcs(ranks.str()), hide ? 2U : 3U);
          To_Graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>, Dft_Show_Node<GT>, SA>(sa)
            .ranks(g, dot);
          EXPECT_EQ(drawn_arcs(dot.str()), hide ? 2U : 3U);

          // Without the object, SA(), which hides nothing
          if constexpr (default_built<SA>)
            {
              std::ostringstream all;
              EXPECT_EQ((rank_graphviz<GT, Dft_Node_Attr<GT>, Dft_Arc_Attr<GT>,
                                       Dft_Show_Node<GT>, SA>(g, all)), 3U);
              EXPECT_EQ(drawn_arcs(all.str()), 3U);
            }
        });
    });
}

template <class GT>
class ContractCoreTrees : public ::testing::Test {};

TYPED_TEST_SUITE(ContractCoreTrees, Undirected_Kinds);

TYPED_TEST(ContractCoreTrees, DfLowTree)
{
  using GT = TypeParam;
  Built<GT> b(triangle());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynList<Df_Low_Non_Tree_Arc> non_tree;
          Df_Low_Tree_Node * root =
            build_df_low_tree(g, b.nodes[0], non_tree, Dft_Df_Tree_Label<GT>(), sa);
          ASSERT_NE(root, nullptr);
          size_t nodes = 0;
          root->traverse([&nodes] (Df_Low_Tree_Node *) { ++nodes; return true; });
          destroy_tree(root);
          EXPECT_EQ(nodes, 3U);
          // Without arc 2, no arc is out of the tree
          EXPECT_EQ(non_tree.size(), hide ? 0U : 1U);

          std::ostringstream out;
          write_df_low_tree(g, b.nodes[0], out, Dft_Df_Tree_Label<GT>(), sa);
          write_df_low_tree<Write_Df_Tree_Df>(g, b.nodes[0], out, Dft_Df_Tree_Label<GT>(), sa);
          EXPECT_FALSE(out.str().empty());
        });
    });
}

TYPED_TEST(ContractCoreTrees, GraphToTreeNode)
{
  using GT = TypeParam;
  Built<GT> tree(star_path());
  const Spec s = triangle();
  Built<GT> cycle(s);
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // They take GT &: a const graph is outside their contract

      Tree_Node<int> * built =
        graph_to_tree_node<GT, int, To_Key, SA>(tree.g, tree.nodes[0], sa);
      EXPECT_EQ(tree_keys(built),
                hide ? std::set<int>({0, 1, 2}) : std::set<int>({0, 1, 2, 3}));
      destroy_tree(built);
      if (hide) // the filter makes a tree of the triangle
        {
          built = graph_to_tree_node<GT, int, To_Key, SA>(cycle.g, cycle.nodes[0], sa);
          EXPECT_EQ(tree_keys(built), std::set<int>({0, 1, 2}));
          destroy_tree(built);
        }
      else
        EXPECT_THROW_OR_FILTER((void) (graph_to_tree_node<GT, int, To_Key, SA>(cycle.g,
                                                                               cycle.nodes[0], sa)),
                               std::domain_error);

      // Without the object, SA(), which hides nothing
      if constexpr (default_built<SA>)
        {
          built = graph_to_tree_node<GT, int, To_Key, SA>(tree.g, tree.nodes[0]);
          EXPECT_EQ(tree_keys(built), std::set<int>({0, 1, 2, 3}));
          destroy_tree(built);
        }

      Graph_To_Tree_Node<GT, int, To_Key, SA> convert(sa);
      To_Key key;
      Tree_Node<int> * root = convert(tree.g, tree.nodes[0]);
      EXPECT_EQ(tree_keys(root),
                hide ? std::set<int>({0, 1, 2}) : std::set<int>({0, 1, 2, 3}));
      destroy_tree(root);
      root = convert(tree.g, tree.nodes[0], key);
      destroy_tree(root);

      // The filter makes a tree of the triangle
      if (hide)
        {
          root = convert(cycle.g, cycle.nodes[0]);
          EXPECT_EQ(tree_keys(root), std::set<int>({0, 1, 2}));
          destroy_tree(root);
        }
      else
        EXPECT_THROW_OR_FILTER((void) convert(cycle.g, cycle.nodes[0]), std::domain_error);
    });
}
