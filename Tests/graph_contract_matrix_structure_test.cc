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
 * @file graph_contract_matrix_structure_test.cc
 * @brief The contract matrix of the structure algorithms: strong
 *        components, topological sorts, dominators, connectivity, minimum
 *        cuts, planarity and coloring, with every kind of graph, arc filter
 *        and constness that they take.
 *
 * Each test instantiates one template of Tarjan.H, topological_sort.H,
 * Dominators.H, tpl_kgraph.H, Karger.H, Stoer_Wagner.H, Planarity_Test.H
 * or Graph_Coloring.H with each filter of graph_filter_catalog.H and with
 * Dft_Show_Arc, on each kind of graph that its documentation supports, and
 * on a const graph where it takes one, and calls each of its public entry
 * points once, on a small graph whose answer changes when the filter hides
 * arc number 2. Where a template also builds SA() when it is given no
 * filter object, that form runs too, with the filters that have a default
 * constructor, which hide nothing: its answer is that of the whole graph.
 */

#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Tarjan.H>
#include <topological_sort.H>
#include <Dominators.H>
#include <tpl_kgraph.H>
#include <Karger.H>
#include <Stoer_Wagner.H>
#include <Planarity_Test.H>
#include <Graph_Coloring.H>

#include "graph_harness.H"
#include "graph_filter_catalog.H"

using namespace Aleph;
using namespace graph_harness;

namespace
{
  /// The number of the arc that the filters hide
  constexpr int Hidden = 2;

  /// The mask of the filters: only the arc Hidden
  const unsigned long Hidden_Mask = 1UL << Hidden;

  /// Whether a filter of type SA hides the arc Hidden: all but Dft_Show_Arc
  template <class GT, class SA>
  constexpr bool hides = not std::is_same_v<SA, Dft_Show_Arc<GT>>;

  /** Calls f(sa, name) with a filter of each kind of the catalog of
      graph_filter_catalog.H, and with Dft_Show_Arc. All but Dft_Show_Arc
      hide the arc Hidden. f takes the filter by value, so that it can pass
      it on as an lvalue and as an rvalue. The filters whose call is not
      noexcept go through with_each_throw(), which calls f again throwing
      at each of their calls. */
  template <class GT, class F>
  void for_each_filter(F && f)
  {
    size_t calls = 0;
    f(Dft_Show_Arc<GT>(), "Dft_Show_Arc");
    f(graph_filters::Hide_Mask<GT>{Hidden_Mask}, "Hide_Mask");
    f(graph_filters::Counting<GT>{Hidden_Mask, &calls}, "Counting");
    with_each_throw(f, graph_filters::make_capturing_lambda<GT>(Hidden_Mask), "Capturing_Lambda");
    f(graph_filters::Reference_Member<GT>(Hidden_Mask), "Reference_Member");
    f(graph_filters::Const_Member<GT>(Hidden_Mask), "Const_Member");
    with_each_throw(f, graph_filters::Throwing_Copy<GT>(Hidden_Mask), "Throwing_Copy");
    with_each_throw(f, graph_filters::Deque_Member<GT>(Hidden_Mask), "Deque_Member");
  }

  /// Calls f(g) with g as it is and as a const graph
  template <class GT, class F>
  void for_each_constness(GT & g, F && f)
  {
    {
      SCOPED_TRACE("GT &");
      f(g);
    }
    {
      SCOPED_TRACE("const GT &");
      f(std::as_const(g));
    }
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
  /// The numbers of the nodes of a container of nodes, in its order
  template <class List>
  std::vector<int> node_order(const List & l)
  {
    std::vector<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.push_back(it.get_curr()->get_info());
    return r;
  }

  /// The cycle 0 -> 1 -> 2 -> 0, which the hidden arc closes
  Spec cycle_through_hidden()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {2, 0}};
    return s;
  }

  /// The path 0 - 1 - 2 and the hidden shortcut 0 - 2: as a digraph, the
  /// shortcut decides who dominates 2; as a graph, it closes a triangle
  Spec path_and_shortcut()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {0, 2}};
    return s;
  }

  /// K5, which is planar without any of its arcs, the hidden one among
  /// them
  Spec k5()
  {
    Spec s;
    s.n = 5;
    for (int u = 0; u < 5; ++u)
      for (int v = u + 1; v < 5; ++v)
        s.arcs.push_back({u, v});
    return s;
  }

  /// The second of the pair of a list of (node, dominator) pairs whose
  /// first is v
  template <class Node, class List>
  Node * dominator_in(const List & l, Node * v)
  {
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr().first == v)
        return it.get_curr().second;
    return nullptr;
  }
} // namespace

template <class GT>
class ContractStructureDigraph : public ::testing::Test {};

TYPED_TEST_SUITE(ContractStructureDigraph, Directed_Kinds);

TYPED_TEST(ContractStructureDigraph, TarjanConnectedComponents)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  Built<GT> b(cycle_through_hidden());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;
      const size_t want = h ? 3 : 1; // the hidden arc closes the only cycle

      for_each_constness(b.g, [&] (auto & g)
        {
          Tarjan_Connected_Components<GT, Out_Iterator, SA> t(sa);

          DynList<GT> blocks;
          DynList<Arc *> cross;
          t.connected_components(g, blocks, cross);
          EXPECT_EQ(blocks.size(), want);
          EXPECT_EQ(arc_numbers(cross), (h ? std::set<int>{0, 1} : std::set<int>{}));

          DynList<DynList<Node *>> lists;
          t.connected_components(g, lists);
          EXPECT_EQ(lists.size(), want);
          EXPECT_EQ(t.connected_components(g).size(), want);
          EXPECT_EQ(t.num_connected_components(g), want);
          DynList<size_t> sizes;
          t.connected_components(g, sizes);
          EXPECT_EQ(sizes.size(), want);

          DynList<GT> blocks2;
          DynList<Arc *> cross2;
          t(g, blocks2, cross2);
          EXPECT_EQ(blocks2.size(), want);
          DynList<DynList<Node *>> lists2;
          t(g, lists2);
          EXPECT_EQ(lists2.size(), want);
          EXPECT_EQ(t(g).size(), want);
          DynDlist<GT> blocks3;
          DynDlist<Arc *> cross3;
          t(g, blocks3, cross3);
          EXPECT_EQ(blocks3.size(), want);
          DynDlist<DynDlist<Node *>> lists3;
          t(g, lists3);
          EXPECT_EQ(lists3.size(), want);

          EXPECT_EQ(t.has_cycle(g), not h);
          EXPECT_EQ(t.is_dag(g), h);
          Path<GT> p(g);
          EXPECT_EQ(t.compute_cycle(g, p), not h);
          EXPECT_EQ(arc_numbers(p.arcs()), (h ? std::set<int>{} : std::set<int>{0, 1, 2}));
          EXPECT_EQ(t.compute_cycle(g, b.nodes[0], p), not h);
          EXPECT_EQ(t.test_connectivity(g), not h);

          EXPECT_TRUE(t.has_computation());
          EXPECT_EQ(t.get_graph(), &b.g);
          static_cast<void>(t.get_filter());
          static_cast<void>(std::as_const(t).get_filter());

          Tarjan_Connected_Components<GT, Out_Iterator, SA> moved(std::move(t));
          EXPECT_EQ(moved.has_cycle(g), not h);
          // The defaulted move assignment is deleted with a filter that
          // cannot be assigned
          if constexpr (std::is_move_assignable_v<SA>)
            {
              Tarjan_Connected_Components<GT, Out_Iterator, SA> other(sa);
              other = std::move(moved);
              EXPECT_EQ(other.is_dag(g), h);
            }
        });
    });
}

TYPED_TEST(ContractStructureDigraph, ComputeCycleInDigraph)
{
  using GT = TypeParam;
  Built<GT> b(cycle_through_hidden());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;

      for_each_constness(b.g, [&] (auto & g)
        {
          const Compute_Cycle_In_Digraph<GT, Out_Iterator, SA> cycle(sa);
          Path<GT> p(g);
          EXPECT_EQ(cycle(g, p), not h);
          if (not h)
            {
              EXPECT_EQ(arc_numbers(p.arcs()), (std::set<int>{0, 1, 2}));
            }
          EXPECT_EQ(cycle(g).is_empty(), h);
          EXPECT_EQ(cycle(g, b.nodes[0]).is_empty(), h);
        });
    });
}

TYPED_TEST(ContractStructureDigraph, TopologicalSort)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Built<GT> b(cycle_through_hidden());
  const std::vector<int> order = {0, 1, 2};

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;
      for_each_constness(b.g, [&] (auto & g)
        {
          // With the cycle the order is not defined, but has every node
          Topological_Sort<GT, Out_Iterator, SA> lvalue(sa);
          const auto l = lvalue.template perform<DynList>(g);
          EXPECT_EQ(l.size(), 3U);
          if (h)
            {
              EXPECT_EQ(node_order(l), order);
            }
          EXPECT_EQ(lvalue.template perform<DynDlist>(g).size(), 3U);

          Topological_Sort<GT, Out_Iterator, SA> rvalue{SA(sa)};
          DynDlist<Node *> dl;
          rvalue(g, dl);
          EXPECT_EQ(dl.size(), 3U);
          if (h)
            {
              EXPECT_EQ(node_order(dl), order);
            }
        });
    });
}

TYPED_TEST(ContractStructureDigraph, QTopologicalSort)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Built<GT> b(cycle_through_hidden());
  const std::vector<int> order = {0, 1, 2};

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;
      // On the cycle no node is a source: Kahn's algorithm gives none
      const size_t want = h ? 3 : 0;
      for_each_constness(b.g, [&] (auto & g)
        {
          Q_Topological_Sort<GT, Out_Iterator, SA> lvalue(sa);
          const auto l = lvalue.template perform<DynList>(g);
          EXPECT_EQ(l.size(), want);
          if (h)
            {
              EXPECT_EQ(node_order(l), order);
            }
          EXPECT_EQ(lvalue.template perform<DynDlist>(g).size(), want);
          EXPECT_EQ(lvalue.template ranks<>(g).size(), want);
          EXPECT_EQ((lvalue.template ranks<DynDlist, DynList>(g).size()), want);

          Q_Topological_Sort<GT, Out_Iterator, SA> rvalue{SA(sa)};
          DynDlist<DynList<Node *>> r1;
          rvalue(g, r1);
          EXPECT_EQ(r1.size(), want);
          DynList<DynList<Node *>> r2;
          rvalue(g, r2);
          EXPECT_EQ(r2.size(), want);
          DynDlist<Node *> r3;
          rvalue(g, r3);
          EXPECT_EQ(r3.size(), want);
        });
    });
}

TYPED_TEST(ContractStructureDigraph, LengauerTarjanDominators)
{
  using GT = TypeParam;
  Built<GT> b(path_and_shortcut());
  auto * const n0 = b.nodes[0];
  auto * const n1 = b.nodes[1];
  auto * const n2 = b.nodes[2];

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      // Without the shortcut 0 -> 2, every path to 2 goes through 1
      constexpr bool h = hides<GT, SA>;

      // It takes GT &, not a const graph
      Lengauer_Tarjan_Dominators<GT, SA> d(sa);
      const auto idoms = d.compute_idom(b.g, n0);
      EXPECT_EQ(idoms.size(), 3U);
      EXPECT_EQ(dominator_in(idoms, n2), h ? n1 : n0);
      EXPECT_EQ(d.get_idom(b.g, n0, n2), h ? n1 : n0);
      EXPECT_EQ(d.dominates(b.g, n0, n1, n2), h);
      const auto frontiers = d.dominance_frontiers(b.g, n0);
      EXPECT_EQ(frontiers.find(n1).size(), h ? 0U : 1U);

      GT tree;
      d.build_tree(b.g, n0, tree);
      EXPECT_EQ(tree.get_num_nodes(), 3U);
      EXPECT_EQ(tree.get_num_arcs(), 2U);
      GT tree2;
      d(b.g, n0, tree2);
      EXPECT_EQ(tree2.get_num_arcs(), 2U);

      EXPECT_TRUE(d.has_computation());
      EXPECT_EQ(d.get_graph(), &b.g);
      EXPECT_EQ(d.get_root(), n0);
      EXPECT_EQ(d.get_num_reachable(), 3);
      static_cast<void>(d.get_filter());
      static_cast<void>(std::as_const(d).get_filter());

      Lengauer_Tarjan_Dominators<GT, SA> moved(std::move(d));
      EXPECT_EQ(moved.get_idom(b.g, n0, n2), h ? n1 : n0);
      // The defaulted move assignment is deleted with a filter that cannot
      // be assigned
      if constexpr (std::is_move_assignable_v<SA>)
        {
          Lengauer_Tarjan_Dominators<GT, SA> other(sa);
          other = std::move(moved);
          EXPECT_EQ(other.get_idom(b.g, n0, n2), h ? n1 : n0);
        }
    });
}

TYPED_TEST(ContractStructureDigraph, DominatorFunctions)
{
  using GT = TypeParam;
  Built<GT> b(path_and_shortcut());
  auto * const n0 = b.nodes[0];
  auto * const n1 = b.nodes[1];
  auto * const n2 = b.nodes[2];

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;

      // They take GT &; on a const graph, these two deduce a const GT
      for_each_constness(b.g, [&] (auto & g)
        {
          EXPECT_EQ(dominator_in(compute_dominators(g, n0, sa), n2), h ? n1 : n0);
          const auto frontiers = compute_dominance_frontiers(g, n0, sa);
          EXPECT_EQ(frontiers.find(n1).size(), h ? 0U : 1U);
        });

      // The tree is a GT & too, which a const GT cannot be
      GT tree;
      build_dominator_tree(b.g, n0, tree, sa);
      EXPECT_EQ(tree.get_num_arcs(), 2U);
    });
}

TYPED_TEST(ContractStructureDigraph, LengauerTarjanPostDominators)
{
  using GT = TypeParam;
  Built<GT> b(path_and_shortcut());
  auto * const n0 = b.nodes[0];
  auto * const n1 = b.nodes[1];
  auto * const n2 = b.nodes[2];

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      // Without the shortcut, every path from 0 to the exit 2 goes through
      // 1. The filter sees the arcs of the inverted digraph, which keep the
      // info, and so the number, of theirs.
      constexpr bool h = hides<GT, SA>;
      for_each_constness(b.g, [&] (auto & g)
        {
          Lengauer_Tarjan_Post_Dominators<GT, SA> d(sa);
          const auto ipdoms = d.compute_ipdom(g, n2);
          EXPECT_EQ(ipdoms.size(), 3U);
          EXPECT_EQ(dominator_in(ipdoms, n0), h ? n1 : n2);
          EXPECT_EQ(d.get_ipdom(g, n2, n0), h ? n1 : n2);
          EXPECT_EQ(d.post_dominates(g, n2, n1, n0), h);
          const auto frontiers = d.post_dominance_frontiers(g, n2);
          EXPECT_EQ(frontiers.find(n1).size(), h ? 0U : 1U);

          GT tree;
          d.build_tree(g, n2, tree);
          EXPECT_EQ(tree.get_num_nodes(), 3U);
          EXPECT_EQ(tree.get_num_arcs(), 2U);
          GT tree2;
          d(g, n2, tree2);
          EXPECT_EQ(tree2.get_num_arcs(), 2U);

          EXPECT_TRUE(d.has_computation());
          EXPECT_EQ(d.get_graph(), &b.g);
          EXPECT_EQ(d.get_exit(), n2);
          static_cast<void>(d.get_filter());
          static_cast<void>(std::as_const(d).get_filter());

          Lengauer_Tarjan_Post_Dominators<GT, SA> moved(std::move(d));
          EXPECT_EQ(moved.get_ipdom(g, n2, n0), h ? n1 : n2);
          if constexpr (std::is_move_assignable_v<SA>)
            {
              Lengauer_Tarjan_Post_Dominators<GT, SA> other(sa);
              other = std::move(moved);
              EXPECT_EQ(other.get_ipdom(g, n2, n0), h ? n1 : n2);
            }
        });
    });
}

TYPED_TEST(ContractStructureDigraph, PostDominatorFunctions)
{
  using GT = TypeParam;
  Built<GT> b(path_and_shortcut());
  auto * const n0 = b.nodes[0];
  auto * const n1 = b.nodes[1];
  auto * const n2 = b.nodes[2];

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;
      for_each_constness(b.g, [&] (auto & g)
        {
          EXPECT_EQ(dominator_in(compute_post_dominators(g, n2, sa), n0), h ? n1 : n2);
          GT tree;
          build_post_dominator_tree(g, n2, tree, sa);
          EXPECT_EQ(tree.get_num_arcs(), 2U);
          const auto frontiers = compute_post_dominance_frontiers(g, n2, sa);
          EXPECT_EQ(frontiers.find(n1).size(), h ? 0U : 1U);
        });
    });
}

TYPED_TEST(ContractStructureDigraph, NoexceptOnlyIfTheFilterIs)
{
  using GT = TypeParam;
  using F = graph_filters::Throwing_Copy<GT>;
  static_assert(not std::is_nothrow_move_constructible_v<F>);

  using Plain = Dft_Show_Arc<GT>;

  // A prvalue initializes the parameter without a move, so that noexcept()
  // sees only the constructor, which moves it into the member. Tarjan's is
  // noexcept if the move of the filter is; the dominators reserve arrays.
  EXPECT_FALSE(noexcept(Tarjan_Connected_Components<GT, Out_Iterator, F>(F())));
  EXPECT_TRUE(noexcept(Tarjan_Connected_Components<GT, Out_Iterator, Plain>(Plain())));
  EXPECT_FALSE(noexcept(Lengauer_Tarjan_Dominators<GT, F>(F())));
  EXPECT_FALSE(noexcept(Lengauer_Tarjan_Post_Dominators<GT, F>(F())));
}

template <class GT>
class ContractStructureGraph : public ::testing::Test {};

TYPED_TEST_SUITE(ContractStructureGraph, Undirected_Kinds);

TYPED_TEST(ContractStructureGraph, EdgeAndVertexConnectivity)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  Built<GT> b(path_and_shortcut());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // They take GT &. The triangle is 2-connected, and without the arc
      // Hidden it is a path, 1-connected
      const long k = hide ? 1 : 2;
      EXPECT_EQ((edge_connectivity<GT, Random_Preflow_Maximum_Flow, SA>(b.g, sa)), k);
      EXPECT_EQ((vertex_connectivity<GT, Random_Preflow_Maximum_Flow, SA>(b.g, sa)), k);

      DynSetTree<Node *> l, r;
      DynDlist<Arc *> cut;
      EXPECT_EQ((compute_min_cut<GT, Heap_Preflow_Maximum_Flow, SA>(b.g, l, r, cut, sa)), k);
      EXPECT_EQ(l.size() + r.size(), 3U);
      EXPECT_EQ(cut.size(), size_t(k));

      Compute_Min_Cut<GT, Heap_Preflow_Maximum_Flow, SA> min_cut(sa);
      DynSetTree<Node *> l2, r2;
      DynDlist<Arc *> cut2;
      EXPECT_EQ(min_cut(b.g, l2, r2, cut2), k);
      EXPECT_EQ(cut2.size(), size_t(k));

      // Without the object they build SA(), which hides nothing
      if constexpr (std::is_default_constructible_v<SA>)
        {
          EXPECT_EQ((edge_connectivity<GT, Random_Preflow_Maximum_Flow, SA>(b.g)), 2);
          EXPECT_EQ((vertex_connectivity<GT, Random_Preflow_Maximum_Flow, SA>(b.g)), 2);
          DynSetTree<Node *> l3, r3;
          DynDlist<Arc *> cut3;
          EXPECT_EQ((compute_min_cut<GT, Heap_Preflow_Maximum_Flow, SA>(b.g, l3, r3, cut3)), 2);
          EXPECT_EQ((Compute_Min_Cut<GT, Heap_Preflow_Maximum_Flow, SA>()(b.g, l3, r3, cut3)), 2);
        }
    });
}

TYPED_TEST(ContractStructureGraph, KargerMinCut)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  Built<GT> b(path_and_shortcut());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;
      // The triangle has cuts of 2 arcs; without the arc 0 - 2, a path of
      // cuts of 1. Every contraction finds them.
      const size_t want = h ? 1 : 2;

      // It takes GT &, not a const graph
      Karger_Min_Cut<GT, SA> karger(17, sa);
      EXPECT_EQ(karger.get_seed(), 17U);
      karger.reseed(3);
      EXPECT_EQ(karger.get_seed(), 3U);
      EXPECT_EQ(karger.compute_min_cut_size(b.g), want);
      EXPECT_EQ(karger.compute_min_cut_size(b.g, 2), want);

      DynList<Node *> vs, vt;
      DynList<Arc *> cut;
      EXPECT_EQ(karger(b.g, vs, vt, cut, 2), want);
      EXPECT_EQ(cut.size(), want);
      if (h)
        {
          EXPECT_EQ(arc_numbers(cut).count(Hidden), 0U);
        }
      EXPECT_EQ(karger(b.g, vs, vt, cut), want);
      EXPECT_EQ(vs.size() + vt.size(), 3U);
      EXPECT_EQ(karger.fast(b.g, vs, vt, cut), want);
      EXPECT_EQ(karger.fast(b.g, vs, vt, cut, 2), want);
      if (h)
        {
          EXPECT_EQ(arc_numbers(cut).count(Hidden), 0U);
        }

      Karger_Min_Cut<GT, SA> moved(std::move(karger));
      EXPECT_EQ(moved.compute_min_cut_size(b.g), want);
      // The move assignment is only for a filter that can be assigned
      if constexpr (std::is_move_assignable_v<SA>)
        {
          Karger_Min_Cut<GT, SA> other(19, sa);
          other = std::move(moved);
          EXPECT_EQ(other.compute_min_cut_size(b.g), want);
        }
    });
}

TYPED_TEST(ContractStructureGraph, StoerWagnerMinCut)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  Built<GT> b(path_and_shortcut());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // It takes GT &. The weights are the arc numbers; the cut around
      // node 1 weighs 0 + 1, and without the arc Hidden, the cut around
      // node 0 weighs 0
      Stoer_Wagner_Min_Cut<GT, Dft_Dist<GT>, SA> sw(Dft_Dist<GT>(), sa);
      DynList<Node *> vs, vt;
      DynList<Arc *> cut;
      EXPECT_EQ(sw(b.g, vs, vt, cut), hide ? 0 : 1);
      EXPECT_EQ(vs.size() + vt.size(), 3U);
      EXPECT_EQ(arc_numbers(cut), (hide ? std::set<int>{0} : std::set<int>{0, 1}));
      EXPECT_EQ(sw.min_cut_weight(b.g), hide ? 0 : 1);

      // Without the object it builds SA(), which hides nothing
      if constexpr (std::is_default_constructible_v<SA>)
        {
          Stoer_Wagner_Min_Cut<GT, Dft_Dist<GT>, SA> by_default;
          EXPECT_EQ(by_default.min_cut_weight(b.g), 1);

          Stoer_Wagner_Min_Cut<GT, Dft_Dist<GT>, SA> with_distance{Dft_Dist<GT>()};
          EXPECT_EQ(with_distance.min_cut_weight(b.g), 1);
        }
    });
}

TYPED_TEST(ContractStructureGraph, NoexceptOnlyIfTheFilterIs)
{
  using GT = TypeParam;
  using F = graph_filters::Throwing_Copy<GT>;
  using Karger = Karger_Min_Cut<GT, F>;
  using Plain = Karger_Min_Cut<GT>;

  // The moves are noexcept if those of the filter are
  EXPECT_FALSE(noexcept(Karger(std::declval<Karger>())));
  EXPECT_FALSE(noexcept(std::declval<Karger &>() = std::declval<Karger>()));
  EXPECT_TRUE(noexcept(Plain(std::declval<Plain>())));
  EXPECT_TRUE(noexcept(std::declval<Plain &>() = std::declval<Plain>()));
}

template <class GT>
class ContractStructureAllKinds : public ::testing::Test {};

TYPED_TEST_SUITE(ContractStructureAllKinds, All_Kinds);

TYPED_TEST(ContractStructureAllKinds, PlanarityTestAndIsPlanarGraph)
{
  using GT = TypeParam;
  Built<GT> b(k5());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      constexpr bool h = hides<GT, SA>;
      for_each_constness(b.g, [&] (auto & g)
        {
          const auto plain = planarity_test(g, sa);
          EXPECT_EQ(plain.is_planar, h);
          EXPECT_EQ(plain.num_input_arcs, h ? 9U : 10U);

          Planarity_Test_Options options;
          options.compute_embedding = true;
          options.compute_nonplanar_certificate = true;
          const auto full = planarity_test(g, SA(sa), options);
          EXPECT_EQ(full.is_planar, h);
          EXPECT_EQ(full.has_combinatorial_embedding, h);
          EXPECT_EQ(full.has_nonplanar_certificate, not h);

          EXPECT_EQ(is_planar_graph(g, sa), h);
          EXPECT_EQ(is_planar_graph(g, SA(sa), options), h);

          const Planarity_Test<GT, SA> test(sa);
          EXPECT_EQ(test(g).is_planar, h);
          const Planarity_Test<GT, SA> test_with_options(SA(sa), options);
          EXPECT_EQ(test_with_options(g).has_combinatorial_embedding, h);
          const Is_Planar_Graph<GT, SA> is_planar(sa);
          EXPECT_EQ(is_planar(g), h);
          const Is_Planar_Graph<GT, SA> is_planar_with_options(SA(sa), options);
          EXPECT_EQ(is_planar_with_options(g), h);
        });
    });
}

TYPED_TEST(ContractStructureAllKinds, ColoringFunctionsAndClasses)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  Built<GT> b(path_and_shortcut());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hide = hides<GT, SA>;
      SCOPED_TRACE(name);
      // The triangle takes 3 colors, and without the arc Hidden it is a
      // path, which takes 2
      const size_t want = hide ? 2 : 3;
      for_each_constness(b.g, [&] (auto & g)
        {
          DynMapTree<Node *, size_t> colors;
          EXPECT_EQ((greedy_coloring<GT, SA>(g, colors, sa)), want);
          EXPECT_TRUE((is_valid_coloring<GT, SA>(g, colors, sa)));
          EXPECT_EQ((welsh_powell_coloring<GT, SA>(g, colors, sa)), want);
          EXPECT_TRUE((is_valid_coloring<GT, SA>(g, colors, sa)));
          EXPECT_EQ((dsatur_coloring<GT, SA>(g, colors, sa)), want);
          EXPECT_TRUE((is_valid_coloring<GT, SA>(g, colors, sa)));
          EXPECT_EQ((chromatic_number<GT, SA>(g, colors, sa)), want);
          EXPECT_TRUE((is_valid_coloring<GT, SA>(g, colors, sa)));

          const Greedy_Coloring<GT, SA> greedy(sa);
          EXPECT_EQ(greedy(g, colors), want);
          const Welsh_Powell_Coloring<GT, SA> welsh_powell(sa);
          EXPECT_EQ(welsh_powell(g, colors), want);
          const DSatur_Coloring<GT, SA> dsatur(sa);
          EXPECT_EQ(dsatur(g, colors), want);

          // Without the object they build SA(), which hides nothing
          if constexpr (std::is_default_constructible_v<SA>)
            {
              EXPECT_EQ((greedy_coloring<GT, SA>(g, colors)), 3U);
              EXPECT_TRUE((is_valid_coloring<GT, SA>(g, colors)));
              EXPECT_EQ((Greedy_Coloring<GT, SA>()(g, colors)), 3U);
              EXPECT_EQ((Welsh_Powell_Coloring<GT, SA>()(g, colors)), 3U);
              EXPECT_EQ((DSatur_Coloring<GT, SA>()(g, colors)), 3U);
              EXPECT_EQ((chromatic_number<GT, SA>(g, colors)), 3U);
            }
        });
    });
}
