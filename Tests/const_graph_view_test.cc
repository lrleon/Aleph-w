/*
                          Aleph_w

                           / \  | | ___ _ __ | |__      __      __
                          / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
                         / ___ \| |  __/ |_) | | | |_____\ V  V /
                        /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                                       |_|

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
 * @file const_graph_view_test.cc
 * @brief Const_Graph_View and the read-only graph iterators hand out pointers
 *        to const: neither the information nor the bookkeeping of a node or of
 *        an arc can be written through them, on any graph class.
 */

#include <gtest/gtest.h>

#include <Bellman_Ford.H>
#include <Dijkstra.H>
#include <Negative_Cycles.H>
#include <ah-graph-concepts.H>
#include <filter_iterator.H>
#include <tpl_agraph.H>
#include <tpl_const_graph_view.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Aleph;

namespace
{
  // The expressions that a reader must not be able to write. They are
  // templates so that an ill-formed expression makes the requires-expression
  // false instead of making the program ill-formed.
  template <class It>
  constexpr bool writes_the_information_through = requires (It it)
    {
      it.get_curr()->get_info() = 1;
    };

  template <class It>
  constexpr bool writes_the_node_bookkeeping_through = requires (It it)
    {
      NODE_COOKIE(it.get_curr()) = nullptr;
    }
    or requires (It it) { NODE_COUNTER(it.get_curr()) = 1; }
    or requires (It it) { NODE_BITS(it.get_curr()).set_bit(Depth_First, 1); };

  template <class It>
  constexpr bool writes_the_arc_bookkeeping_through = requires (It it)
    {
      ARC_COOKIE(it.get_curr()) = nullptr;
    }
    or requires (It it) { ARC_COUNTER(it.get_curr()) = 1; }
    or requires (It it) { ARC_BITS(it.get_curr()).set_bit(Depth_First, 1); };

  template <class It>
  constexpr bool writes_the_end_points_through = requires (It it)
    {
      it.get_src_node()->get_info() = 1;
    }
    or requires (It it) { it.get_tgt_node()->get_info() = 1; };

  template <class It>
  constexpr bool removes_through = requires (It it) { it.del(); };

  // Whether a traversal accepts an operation that demands the pointer P.
  template <class G, class P>
  constexpr bool for_each_node_takes = requires (const G & g)
    {
      g.for_each_node([] (P) {});
    };

  template <class G, class P>
  constexpr bool for_each_arc_takes = requires (const G & g)
    {
      g.for_each_arc([] (P) {});
    };

  template <class G, class N, class P>
  constexpr bool for_each_arc_of_a_node_takes = requires (const G & g, N p)
    {
      g.for_each_arc(p, [] (P) {});
    };

  template <class G, class P>
  constexpr bool traverse_nodes_takes = requires (const G & g)
    {
      g.traverse_nodes([] (P) { return true; });
    };

  template <class G, class P>
  constexpr bool traverse_arcs_takes = requires (const G & g)
    {
      g.traverse_arcs([] (P) { return true; });
    };

  template <class G, class P>
  constexpr bool search_node_takes = requires (const G & g)
    {
      g.search_node([] (P) { return true; });
    };

  template <class G, class P>
  constexpr bool search_arc_takes = requires (const G & g)
    {
      g.search_arc([] (P) { return true; });
    };

  template <class G, class P>
  constexpr bool filter_arcs_takes = requires (const G & g)
    {
      g.filter_arcs([] (P) { return true; });
    };

  template <class G, class P>
  constexpr bool free_for_each_arc_takes = requires (const G & g)
    {
      Aleph::for_each_arc(g, [] (P) {});
    };

  // Whether const_view() accepts an expression of type and category G.
  template <class G>
  constexpr bool const_view_accepts = requires
    {
      const_view(std::declval<G>());
    };

  // Arc filters and distances written for a reader and for a writer.
  template <class A>
  struct Reads_Arc
  {
    bool operator () (const A *a) const noexcept { return a->get_info() >= 30; }
  };

  template <class A>
  struct Writes_Arc
  {
    bool operator () (A *a) const noexcept { return ++a->get_info() > 0; }
  };

  template <class N>
  struct Reads_Node
  {
    bool operator () (const N *p) const noexcept { return p->get_info() % 2 == 0; }
  };

  // Five nodes 0..4 and six arcs; no loops and no parallel arcs.
  template <class G>
  struct Sample
  {
    G g;
    std::vector<typename G::Node *> nodes;
    std::vector<typename G::Arc *> arcs;

    Sample()
    {
      for (int i = 0; i < 5; ++i)
        nodes.push_back(g.insert_node(i));
      arcs.push_back(g.insert_arc(nodes[0], nodes[1], 10));
      arcs.push_back(g.insert_arc(nodes[1], nodes[2], 20));
      arcs.push_back(g.insert_arc(nodes[2], nodes[0], 30));
      arcs.push_back(g.insert_arc(nodes[2], nodes[3], 40));
      arcs.push_back(g.insert_arc(nodes[3], nodes[4], 50));
      arcs.push_back(g.insert_arc(nodes[0], nodes[3], 60));
    }
  };

  template <class G>
  class ConstGraphView : public ::testing::Test {};

  using Graph_Types = ::testing::Types<
    List_Graph<Graph_Node<int>, Graph_Arc<long>>,
    List_Digraph<Graph_Node<int>, Graph_Arc<long>>,
    List_SGraph<Graph_Snode<int>, Graph_Sarc<long>>,
    List_SDigraph<Graph_Snode<int>, Graph_Sarc<long>>,
    Array_Graph<Graph_Anode<int>, Graph_Aarc<long>>,
    Array_Digraph<Graph_Anode<int>, Graph_Aarc<long>>>;

  TYPED_TEST_SUITE(ConstGraphView, Graph_Types);
}

TYPED_TEST(ConstGraphView, the_iterators_hand_out_pointers_to_const)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;
  using View = Const_Graph_View<G>;

  static_assert(std::is_same_v<typename View::Node, const Node>);
  static_assert(std::is_same_v<typename View::Arc, const Arc>);
  static_assert(std::is_same_v<typename View::Node_Iterator, Const_Node_Iterator<G>>);
  static_assert(std::is_same_v<typename View::Arc_Iterator, Const_Arc_Iterator<G>>);
  static_assert(std::is_same_v<typename View::Node_Arc_Iterator, Const_Node_Arc_Iterator<G>>);

  using NIt = Const_Node_Iterator<G>;
  using AIt = Const_Arc_Iterator<G>;
  using NAIt = Const_Node_Arc_Iterator<G>;

  static_assert(std::is_same_v<typename NIt::Item_Type, const Node *>);
  static_assert(std::is_same_v<typename AIt::Item_Type, const Arc *>);
  static_assert(std::is_same_v<typename NAIt::Item_Type, const Arc *>);

  static_assert(std::is_same_v<decltype(std::declval<NIt &>().get_curr()), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<NIt &>().get_curr_ne()), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<NIt &>().get_current_node()), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<AIt &>().get_curr()), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<AIt &>().get_curr_ne()), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<AIt &>().get_current_arc()), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<AIt &>().get_src_node()), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<AIt &>().get_tgt_node()), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<NAIt &>().get_curr()), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<NAIt &>().get_curr_ne()), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<NAIt &>().get_tgt_node()), const Node *>);

  // The iterators are built from a constant graph, from a view and, for the
  // arcs of a node, from a pointer to a constant node.
  static_assert(std::is_constructible_v<NIt, const G &>);
  static_assert(std::is_constructible_v<NIt, const View &>);
  static_assert(std::is_constructible_v<AIt, const G &>);
  static_assert(std::is_constructible_v<AIt, const View &>);
  static_assert(std::is_constructible_v<NAIt, const Node *>);

  // The information and the bookkeeping are read-only through them...
  static_assert(not writes_the_information_through<NIt>);
  static_assert(not writes_the_information_through<AIt>);
  static_assert(not writes_the_information_through<NAIt>);
  static_assert(not writes_the_node_bookkeeping_through<NIt>);
  static_assert(not writes_the_arc_bookkeeping_through<AIt>);
  static_assert(not writes_the_arc_bookkeeping_through<NAIt>);
  static_assert(not writes_the_end_points_through<AIt>);
  static_assert(not removes_through<NIt>);
  static_assert(not removes_through<AIt>);
  static_assert(not removes_through<NAIt>);

  // ...while the iterators of the graph keep handing out modifiable nodes
  // and arcs, even on a constant graph: its topology is what is constant.
  static_assert(std::is_constructible_v<typename G::Node_Iterator, const G &>);
  static_assert(std::is_same_v<decltype(std::declval<typename G::Node_Iterator &>().get_curr()), Node *>);
  static_assert(std::is_same_v<decltype(std::declval<typename G::Arc_Iterator &>().get_curr()), Arc *>);
  static_assert(writes_the_information_through<typename G::Node_Iterator>);
  static_assert(writes_the_information_through<typename G::Arc_Iterator>);
  static_assert(writes_the_information_through<typename G::Node_Arc_Iterator>);
  static_assert(writes_the_node_bookkeeping_through<typename G::Node_Iterator>);
  static_assert(writes_the_arc_bookkeeping_through<typename G::Arc_Iterator>);
  static_assert(writes_the_end_points_through<typename G::Arc_Iterator>);

  // A view is one pointer and its iterators are the iterators of the graph.
  static_assert(sizeof(View) == sizeof(void *));
  static_assert(std::is_trivially_copyable_v<View>);
  static_assert(sizeof(NIt) == sizeof(typename G::Node_Iterator));
  static_assert(sizeof(AIt) == sizeof(typename G::Arc_Iterator));
  static_assert(sizeof(NAIt) == sizeof(typename G::Node_Arc_Iterator));

  // A view is built from a graph that outlives it, never from a temporary.
  static_assert(std::is_constructible_v<View, const G &>);
  static_assert(std::is_constructible_v<View, G &>);
  static_assert(not std::is_constructible_v<View, G>);
  static_assert(not std::is_constructible_v<View, const G>);
  static_assert(not std::is_convertible_v<const G &, View>);
  static_assert(const_view_accepts<G &>);
  static_assert(const_view_accepts<const G &>);
  static_assert(not const_view_accepts<G>);
  static_assert(not const_view_accepts<const G>);
  static_assert(std::is_same_v<decltype(const_view(std::declval<G &>())), View>);

  SUCCEED();
}

TYPED_TEST(ConstGraphView, the_operations_receive_pointers_to_const)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;
  using View = Const_Graph_View<G>;

  // An operation written for a reader is accepted; one that demands a
  // modifiable node or arc is rejected.
  static_assert(for_each_node_takes<View, const Node *>);
  static_assert(not for_each_node_takes<View, Node *>);
  static_assert(for_each_arc_takes<View, const Arc *>);
  static_assert(not for_each_arc_takes<View, Arc *>);
  static_assert(for_each_arc_of_a_node_takes<View, const Node *, const Arc *>);
  static_assert(not for_each_arc_of_a_node_takes<View, const Node *, Arc *>);
  static_assert(traverse_nodes_takes<View, const Node *>);
  static_assert(not traverse_nodes_takes<View, Node *>);
  static_assert(traverse_arcs_takes<View, const Arc *>);
  static_assert(not traverse_arcs_takes<View, Arc *>);
  static_assert(search_node_takes<View, const Node *>);
  static_assert(not search_node_takes<View, Node *>);
  static_assert(search_arc_takes<View, const Arc *>);
  static_assert(not search_arc_takes<View, Arc *>);
  static_assert(filter_arcs_takes<View, const Arc *>);
  static_assert(not filter_arcs_takes<View, Arc *>);

  // The generic functions of the graphs follow the view.
  static_assert(free_for_each_arc_takes<View, const Arc *>);
  static_assert(not free_for_each_arc_takes<View, Arc *>);

  // The graph itself takes both: it hands out modifiable nodes and arcs.
  static_assert(for_each_node_takes<G, Node *>);
  static_assert(for_each_node_takes<G, const Node *>);
  static_assert(for_each_arc_takes<G, Arc *>);
  static_assert(for_each_arc_takes<G, const Arc *>);
  static_assert(free_for_each_arc_takes<G, Arc *>);

  // What the members of the view hand out.
  static_assert(std::is_same_v<decltype(std::declval<const View &>().get_first_node()), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().get_first_arc()), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().get_src_node(nullptr)), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().get_tgt_node(nullptr)), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().find_node(0)), const Node *>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().find_arc(0)), const Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().nodes()), DynList<const Node *>>);
  static_assert(std::is_same_v<decltype(std::declval<const View &>().arcs()), DynList<const Arc *>>);

  SUCCEED();
}

TYPED_TEST(ConstGraphView, the_view_and_its_iterators_satisfy_the_graph_concepts)
{
  using G = TypeParam;
  using View = Const_Graph_View<G>;

  static_assert(AlephGraph<View>);
  static_assert(BasicGraphIterator<typename View::Node_Iterator>);
  static_assert(BasicGraphIterator<typename View::Arc_Iterator>);
  static_assert(BasicGraphIterator<typename View::Node_Arc_Iterator>);
  static_assert(ResettableGraphIterator<typename View::Node_Iterator>);
  static_assert(GraphNodeIterator<typename View::Node_Iterator, typename View::Node>);
  static_assert(GraphArcIterator<typename View::Arc_Iterator, typename View::Arc>);
  static_assert(NodeArcIterator<typename View::Node_Arc_Iterator, typename View::Node, typename View::Arc>);

  // They do not pass for iterators on modifiable nodes and arcs.
  static_assert(not GraphNodeIterator<typename View::Node_Iterator, typename G::Node>);
  static_assert(not GraphArcIterator<typename View::Arc_Iterator, typename G::Arc>);
  static_assert(not GraphArcIterator<typename View::Node_Arc_Iterator, typename G::Arc>);

  // On a view a filter is a condition on a pointer to const.
  static_assert(NodeFilter<Reads_Node<typename G::Node>, View>);
  static_assert(ArcFilter<Reads_Arc<typename G::Arc>, View>);
  static_assert(not ArcFilter<Writes_Arc<typename G::Arc>, View>);
  static_assert(ArcFilter<Writes_Arc<typename G::Arc>, G>);
  static_assert(ArcFilter<Reads_Arc<typename G::Arc>, G>);

  SUCCEED();
}

TYPED_TEST(ConstGraphView, reads_what_the_graph_reads)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;

  Sample<G> s;
  const G & g = s.g;
  const auto view = const_view(g);

  EXPECT_EQ(view.get_num_nodes(), 5u);
  EXPECT_EQ(view.vsize(), 5u);
  EXPECT_EQ(view.get_num_arcs(), 6u);
  EXPECT_EQ(view.esize(), 6u);
  EXPECT_FALSE(view.is_empty());
  EXPECT_EQ(view.is_digraph(), g.is_digraph());
  EXPECT_EQ(view.get_first_node(), g.get_first_node());
  EXPECT_EQ(view.get_first_arc(), g.get_first_arc());

  // Same nodes, in the same order.
  {
    typename G::Node_Iterator git(g);
    auto vit = view.get_node_it();
    size_t n = 0;
    for (; vit.has_curr(); vit.next_ne(), git.next_ne(), ++n)
      {
        ASSERT_TRUE(git.has_curr());
        EXPECT_EQ(vit.get_curr(), git.get_curr());
        EXPECT_EQ(vit.get_curr_ne(), vit.get_current_node());
        EXPECT_EQ(vit.get_current_node_ne(), vit.get_curr());
      }
    EXPECT_FALSE(git.has_curr());
    EXPECT_EQ(n, 5u);
    EXPECT_THROW((void) vit.get_curr(), std::overflow_error);

    vit.reset_first();
    ASSERT_TRUE(vit.has_curr());
    EXPECT_EQ(vit.get_curr(), typename G::Node_Iterator(g).get_curr());
  }

  // Same arcs, in the same order, with the same end points.
  {
    typename G::Arc_Iterator git(g);
    auto vit = view.get_arc_it();
    size_t n = 0;
    for (; vit.has_curr(); vit.next_ne(), git.next_ne(), ++n)
      {
        ASSERT_TRUE(git.has_curr());
        const Arc *a = vit.get_curr();
        EXPECT_EQ(a, git.get_curr());
        EXPECT_EQ(vit.get_current_arc(), a);
        EXPECT_EQ(vit.get_current_arc_ne(), a);
        EXPECT_EQ(vit.get_src_node(), git.get_src_node());
        EXPECT_EQ(vit.get_tgt_node(), git.get_tgt_node());
        EXPECT_EQ(vit.get_src_node_ne(), vit.get_src_node());
        EXPECT_EQ(vit.get_tgt_node_ne(), vit.get_tgt_node());
        EXPECT_EQ(view.get_src_node(a), g.get_src_node(git.get_curr()));
        EXPECT_EQ(view.get_tgt_node(a), g.get_tgt_node(git.get_curr()));
      }
    EXPECT_FALSE(git.has_curr());
    EXPECT_EQ(n, 6u);
  }

  // Same arcs of every node, with the same node at the other end.
  for (Node *p : s.nodes)
    {
      const Node *cp = p;
      EXPECT_EQ(view.get_num_arcs(cp), g.get_num_arcs(p));
      EXPECT_EQ(view.degree(cp), g.degree(p));

      typename G::Node_Arc_Iterator git(p);
      auto vit = view.get_arc_it(cp);
      size_t n = 0;
      for (; vit.has_curr(); vit.next_ne(), git.next_ne(), ++n)
        {
          ASSERT_TRUE(git.has_curr());
          EXPECT_EQ(vit.get_curr(), git.get_curr());
          EXPECT_EQ(vit.get_current_arc(), git.get_curr());
          EXPECT_EQ(vit.get_current_arc_ne(), git.get_curr());
          EXPECT_EQ(vit.get_tgt_node(), git.get_tgt_node());
          EXPECT_EQ(vit.get_tgt_node_ne(), git.get_tgt_node());
          EXPECT_EQ(view.get_connected_node(vit.get_curr(), cp),
                    g.get_connected_node(git.get_curr(), p));
        }
      EXPECT_FALSE(git.has_curr());
      EXPECT_EQ(view.arcs(cp).size(), n);
      EXPECT_EQ(view.adjacent_nodes(cp).size(), n);
      if (n > 0)
        EXPECT_EQ(view.get_first_arc(cp), g.get_first_arc(p));

      vit.reset_first();
      EXPECT_EQ(vit.has_curr(), n > 0);
    }

  // An arc between two nodes is found from the node that holds it.
  EXPECT_EQ(view.search_arc(s.nodes[0], s.nodes[1]), s.arcs[0]);
  EXPECT_EQ(view.search_arc(s.nodes[0], s.nodes[1]), g.search_arc(s.nodes[0], s.nodes[1]));
  EXPECT_EQ(view.search_arc(s.nodes[4], s.nodes[1]), nullptr);
}

TYPED_TEST(ConstGraphView, the_functional_members_read_the_graph)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;

  Sample<G> s;
  const auto view = const_view(s.g);

  int node_sum = 0;
  view.for_each_node([&node_sum] (const Node *p) { node_sum += p->get_info(); });
  EXPECT_EQ(node_sum, 0 + 1 + 2 + 3 + 4);

  long arc_sum = 0;
  view.for_each_arc([&arc_sum] (const Arc *a) { arc_sum += a->get_info(); });
  EXPECT_EQ(arc_sum, 10 + 20 + 30 + 40 + 50 + 60);

  // The arcs of the node 2, as the graph class sees them.
  long of_node_2 = 0;
  s.g.for_each_arc(s.nodes[2], [&of_node_2] (Arc *a) { of_node_2 += a->get_info(); });
  long seen = 0;
  view.for_each_arc(s.nodes[2], [&seen] (const Arc *a) { seen += a->get_info(); });
  EXPECT_EQ(seen, of_node_2);
  EXPECT_GT(seen, 0);

  // A generic operation is called with pointers to const too.
  view.for_each_arc([] (auto a)
    {
      static_assert(std::is_same_v<decltype(a), const Arc *>);
    });

  size_t visited = 0;
  EXPECT_FALSE(view.traverse_nodes([&visited] (const Node *) { return ++visited < 2; }));
  EXPECT_EQ(visited, 2u);
  EXPECT_TRUE(view.traverse_nodes([] (const Node *) { return true; }));
  EXPECT_TRUE(view.traverse_arcs([] (const Arc *a) { return a->get_info() > 0; }));
  EXPECT_FALSE(view.traverse_arcs([] (const Arc *a) { return a->get_info() < 60; }));
  EXPECT_TRUE(view.traverse_arcs(s.nodes[2], [] (const Arc *a) { return a->get_info() > 0; }));

  EXPECT_TRUE(view.all_nodes([] (const Node *p) { return p->get_info() < 5; }));
  EXPECT_FALSE(view.all_nodes([] (const Node *p) { return p->get_info() < 4; }));
  EXPECT_TRUE(view.all_arcs([] (const Arc *a) { return a->get_info() % 10 == 0; }));
  EXPECT_FALSE(view.all_arcs([] (const Arc *a) { return a->get_info() > 10; }));
  EXPECT_TRUE(view.all_arcs(s.nodes[1], [] (const Arc *a) { return a->get_info() <= 20; }));
  EXPECT_FALSE(view.all_arcs(s.nodes[2], [] (const Arc *a) { return a->get_info() == 30; }));

  EXPECT_TRUE(view.exists_node([] (const Node *p) { return p->get_info() == 3; }));
  EXPECT_FALSE(view.exists_node([] (const Node *p) { return p->get_info() == 7; }));
  EXPECT_TRUE(view.exists_arc([] (const Arc *a) { return a->get_info() == 40; }));
  EXPECT_FALSE(view.exists_arc([] (const Arc *a) { return a->get_info() == 45; }));
  EXPECT_TRUE(view.exists_arc(s.nodes[2], [] (const Arc *a) { return a->get_info() == 40; }));
  EXPECT_FALSE(view.exists_arc(s.nodes[2], [] (const Arc *a) { return a->get_info() == 50; }));

  EXPECT_EQ(view.search_node([] (const Node *p) { return p->get_info() == 3; }), s.nodes[3]);
  EXPECT_EQ(view.search_node([] (const Node *p) { return p->get_info() == 7; }), nullptr);
  EXPECT_EQ(view.search_arc([] (const Arc *a) { return a->get_info() == 40; }), s.arcs[3]);
  EXPECT_EQ(view.search_arc([] (const Arc *a) { return a->get_info() == 45; }), nullptr);
  EXPECT_EQ(view.search_arc(s.nodes[2], [] (const Arc *a) { return a->get_info() == 40; }), s.arcs[3]);
  EXPECT_EQ(view.search_arc(s.nodes[2], [] (const Arc *a) { return a->get_info() == 50; }), nullptr);

  EXPECT_EQ(view.find_node(4), s.nodes[4]);
  EXPECT_EQ(view.find_node(9), nullptr);
  EXPECT_EQ(view.find_arc(60), s.arcs[5]);
  EXPECT_EQ(view.find_arc(65), nullptr);

  EXPECT_EQ(view.count_nodes([] (const Node *p) { return p->get_info() % 2 == 0; }), 3u);
  EXPECT_EQ(view.count_arcs([] (const Arc *a) { return a->get_info() >= 30; }), 4u);

  EXPECT_EQ(view.nodes().size(), 5u);
  EXPECT_EQ(view.arcs().size(), 6u);
  EXPECT_EQ(view.filter_nodes([] (const Node *p) { return p->get_info() % 2 == 0; }).size(), 3u);
  EXPECT_EQ(view.filter_arcs([] (const Arc *a) { return a->get_info() >= 30; }).size(), 4u);
  EXPECT_EQ(view.filter_arcs(s.nodes[3], [] (const Arc *a) { return a->get_info() == 50; }).size(), 1u);

  long mapped = 0;
  view.nodes_map([] (const Node *p) { return 2 * p->get_info(); }).for_each([&mapped] (int x) { mapped += x; });
  EXPECT_EQ(mapped, 20);
  const auto halves = view.template arcs_map<double>([] (const Arc *a) { return a->get_info() / 20.0; });
  static_assert(std::is_same_v<decltype(halves), const DynList<double>>);
  double half_sum = 0;
  halves.for_each([&half_sum] (double x) { half_sum += x; });
  EXPECT_DOUBLE_EQ(half_sum, 10.5);

  EXPECT_EQ(view.foldl_nodes(100, [] (const int & acc, const Node *p) { return acc + p->get_info(); }), 110);
  EXPECT_EQ(view.template foldl_arcs<long>(0, [] (const long & acc, const Arc *a) { return acc + a->get_info(); }),
            210);
}

TYPED_TEST(ConstGraphView, a_view_is_not_a_snapshot_and_copies_share_the_graph)
{
  using G = TypeParam;
  using Arc = typename G::Arc;

  Sample<G> s;
  const auto view = const_view(s.g);
  const Const_Graph_View<G> copy = view;
  const Const_Graph_View deduced(s.g);
  static_assert(std::is_same_v<decltype(deduced), const Const_Graph_View<G>>);

  // The owner of the graph keeps its modifiable interface.
  s.g.for_each_arc([] (Arc *a) { a->get_info() += 1; });
  auto *extra = s.g.insert_node(99);
  s.g.insert_arc(s.nodes[4], extra, 71);

  for (const auto & v : {view, copy, deduced})
    {
      EXPECT_EQ(v.get_num_nodes(), 6u);
      EXPECT_EQ(v.get_num_arcs(), 7u);
      EXPECT_EQ(v.find_node(99), extra);
      EXPECT_EQ(v.template foldl_arcs<long>(0, [] (const long & acc, const Arc *a) { return acc + a->get_info(); }),
                11 + 21 + 31 + 41 + 51 + 61 + 71);
    }
}

TYPED_TEST(ConstGraphView, an_empty_graph_is_read_as_empty)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;

  const G g;
  const auto view = const_view(g);

  EXPECT_TRUE(view.is_empty());
  EXPECT_EQ(view.get_num_nodes(), 0u);
  EXPECT_EQ(view.get_num_arcs(), 0u);
  EXPECT_FALSE(view.get_node_it().has_curr());
  EXPECT_FALSE(view.get_arc_it().has_curr());
  EXPECT_TRUE(view.nodes().is_empty());
  EXPECT_TRUE(view.arcs().is_empty());
  EXPECT_EQ(view.search_node([] (const Node *) { return true; }), nullptr);
  EXPECT_EQ(view.search_arc([] (const Arc *) { return true; }), nullptr);
  EXPECT_TRUE(view.all_nodes([] (const Node *) { return false; }));
  EXPECT_FALSE(view.exists_arc([] (const Arc *) { return true; }));
  EXPECT_EQ(view.count_nodes([] (const Node *) { return true; }), 0u);
  EXPECT_EQ(view.foldl_nodes(7, [] (const int & acc, const Node *) { return acc + 1; }), 7);
}

// get_arc() and get_arc(p) of the graph classes, and get_first_arc(p) of
// List_Graph, did not compile: nothing instantiated them.
TYPED_TEST(ConstGraphView, the_entry_points_of_the_graph_agree_with_the_view)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;

  static_assert(std::is_same_v<decltype(std::declval<const G &>().get_node()), Node *>);
  static_assert(std::is_same_v<decltype(std::declval<const G &>().get_arc()), Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<const G &>().get_arc(nullptr)), Arc *>);
  static_assert(std::is_same_v<decltype(std::declval<const G &>().get_first_arc(nullptr)), Arc *>);

  Sample<G> s;
  const G & g = s.g;
  const auto view = const_view(g);

  EXPECT_EQ(g.get_node(), g.get_first_node());
  EXPECT_EQ(g.get_arc(), g.get_first_arc());
  EXPECT_EQ(view.get_first_node(), g.get_node());
  EXPECT_EQ(view.get_first_arc(), g.get_arc());

  // The node 0 has arcs on every graph class, directed or not.
  Node *p = s.nodes[0];
  EXPECT_EQ(g.get_arc(p), g.get_first_arc(p));
  EXPECT_EQ(view.get_first_arc(p), g.get_arc(p));
  EXPECT_TRUE(g.get_src_node(g.get_arc(p)) == p or g.get_tgt_node(g.get_arc(p)) == p);

  // A node without arcs has no first arc.
  Node *lonely = s.g.insert_node(77);
  EXPECT_THROW((void) g.get_first_arc(lonely), std::range_error);
  EXPECT_THROW((void) g.get_arc(lonely), std::range_error);
  EXPECT_THROW((void) view.get_first_arc(lonely), std::range_error);
}

TYPED_TEST(ConstGraphView, the_filtered_iterators_and_the_generic_functions_read_a_view)
{
  using G = TypeParam;
  using Node = typename G::Node;
  using Arc = typename G::Arc;
  using View = Const_Graph_View<G>;

  Sample<G> s;
  const View view(s.g);

  // The filtered iterators of the graphs, instantiated on the view.
  using Filtered_Arcs = Aleph::Arc_Iterator<View, Reads_Arc<Arc>>;
  static_assert(std::is_same_v<decltype(std::declval<Filtered_Arcs &>().get_curr()), const Arc *>);
  static_assert(not writes_the_information_through<Filtered_Arcs>);
  static_assert(not removes_through<Filtered_Arcs>);

  long filtered = 0;
  for (Filtered_Arcs it(view); it.has_curr(); it.next())
    filtered += it.get_curr()->get_info();
  EXPECT_EQ(filtered, 30 + 40 + 50 + 60);

  using Filtered_Nodes = Aleph::Node_Iterator<View, Reads_Node<Node>>;
  static_assert(std::is_same_v<decltype(std::declval<Filtered_Nodes &>().get_curr()), const Node *>);
  static_assert(not writes_the_information_through<Filtered_Nodes>);

  int even = 0;
  for (Filtered_Nodes it(view); it.has_curr(); it.next())
    even += it.get_curr()->get_info();
  EXPECT_EQ(even, 0 + 2 + 4);

  // The arcs of the node 2 that pass the filter, as the graph gives them.
  using Filtered_Node_Arcs = Aleph::Node_Arc_Iterator<View, Reads_Arc<Arc>>;
  static_assert(std::is_same_v<decltype(std::declval<Filtered_Node_Arcs &>().get_curr()), const Arc *>);
  static_assert(not writes_the_information_through<Filtered_Node_Arcs>);

  long expected = 0;
  for (Aleph::Node_Arc_Iterator<G, Reads_Arc<Arc>> it(s.nodes[2]); it.has_curr(); it.next())
    expected += it.get_curr()->get_info();
  long of_node = 0;
  const Node *n2 = s.nodes[2];
  for (Filtered_Node_Arcs it(n2); it.has_curr(); it.next())
    of_node += it.get_curr()->get_info();
  EXPECT_EQ(of_node, expected);
  EXPECT_GT(of_node, 0);

  // Filter_Iterator directly, with any condition on a pointer to const.
  auto cheap = [] (const Arc *a) { return a->get_info() < 30; };
  Filter_Iterator<View, typename View::Arc_Iterator, decltype(cheap)> fit(view, cheap);
  static_assert(std::is_same_v<decltype(fit.get_curr()), const Arc *>);
  long cheap_sum = 0;
  for (; fit.has_curr(); fit.next())
    cheap_sum += fit.get_curr()->get_info();
  EXPECT_EQ(cheap_sum, 10 + 20);

  // The free functions of the graphs.
  long total = 0;
  Aleph::for_each_arc(view, [&total] (const Arc *a) { total += a->get_info(); });
  EXPECT_EQ(total, 210);

  int nodes_total = 0;
  Aleph::for_each_node(view, [&nodes_total] (const Node *p) { nodes_total += p->get_info(); });
  EXPECT_EQ(nodes_total, 10);

  long through_filter = 0;
  Aleph::for_each_arc(view, [&through_filter] (const Arc *a) { through_filter += a->get_info(); },
                      Reads_Arc<Arc>());
  EXPECT_EQ(through_filter, 180);

  EXPECT_TRUE(Aleph::forall_arc(view, [] (const Arc *a) { return a->get_info() > 0; }));
  EXPECT_FALSE(Aleph::forall_node(view, [] (const Node *p) { return p->get_info() > 0; }));
}

namespace
{
  using Market = List_Digraph<Graph_Node<int>, Graph_Arc<long>>;
  using Market_View = Const_Graph_View<Market>;

  // An arc distance for a reader: it receives a pointer to const.
  struct Read_Distance
  {
    using Distance_Type = long;

    long operator () (Market_View::Arc *a) const noexcept { return a->get_info(); }
  };

  struct Market_Sample
  {
    Market g;
    Market::Node *a = g.insert_node(0);
    Market::Node *b = g.insert_node(1);
    Market::Node *c = g.insert_node(2);
    Market::Node *d = g.insert_node(3);

    explicit Market_Sample(const long closing)
    {
      g.insert_arc(a, b, 2);
      g.insert_arc(b, c, -5);
      g.insert_arc(c, a, closing);  // a -> b -> c -> a costs closing - 3
      g.insert_arc(a, c, 10);
      g.insert_arc(c, d, 4);
    }
  };

  DynList<long> arc_information(const Market_View & view)
  {
    return view.arcs_map([] (Market_View::Arc *a) { return a->get_info(); });
  }

  DynList<int> node_information(const Market_View & view)
  {
    return view.nodes_map([] (Market_View::Node *p) { return p->get_info(); });
  }

  template <class T>
  bool same_items(const DynList<T> & l1, const DynList<T> & l2)
  {
    auto it1 = l1.get_it();
    auto it2 = l2.get_it();
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      if (not (it1.get_curr() == it2.get_curr()))
        return false;
    return not it1.has_curr() and not it2.has_curr();
  }
}

TEST(ConstGraphViewAlgorithms, the_algorithms_that_keep_bookkeeping_run_on_the_graph_and_preserve_it)
{
  Market_Sample negative(1);  // the cycle a -> b -> c -> a costs -2
  const Market_View view(negative.g);
  const auto arcs_before = arc_information(view);
  const auto nodes_before = node_information(view);

  // Bellman-Ford keeps its state in the cookies, the counters and the bits
  // of the nodes and of the arcs, so it takes the graph itself; the
  // information stays as it was.
  static_assert(std::is_constructible_v<Bellman_Ford<Market>, Market &>);
  static_assert(not std::is_constructible_v<Bellman_Ford<Market>, const Market &>);
  EXPECT_TRUE(Bellman_Ford<Market>(negative.g).has_negative_cycle(negative.a));
  EXPECT_TRUE(same_items(arc_information(view), arcs_before));
  EXPECT_TRUE(same_items(node_information(view), nodes_before));
  EXPECT_EQ(view.get_num_nodes(), 4u);
  EXPECT_EQ(view.get_num_arcs(), 5u);

  Market_Sample positive(8);  // b -> c is still negative, but no cycle is
  const Market_View positive_view(positive.g);
  const auto positive_before = arc_information(positive_view);
  EXPECT_FALSE(Bellman_Ford<Market>(positive.g).has_negative_cycle(positive.a));
  EXPECT_TRUE(same_items(arc_information(positive_view), positive_before));
}

TEST(ConstGraphViewAlgorithms, dijkstra_runs_on_the_graph_and_preserves_the_information)
{
  Market g;
  auto *a = g.insert_node(0);
  auto *b = g.insert_node(1);
  auto *c = g.insert_node(2);
  auto *d = g.insert_node(3);
  g.insert_arc(a, b, 1);
  g.insert_arc(b, c, 2);
  g.insert_arc(a, c, 7);
  g.insert_arc(c, d, 3);
  g.insert_arc(a, d, 9);

  const Market_View view(g);
  const auto arcs_before = arc_information(view);
  const auto nodes_before = node_information(view);

  Path<Market> path(g);
  EXPECT_EQ(Dijkstra_Min_Paths<Market>().find_min_path(g, a, d, path), 6);
  EXPECT_EQ(path.size(), 4u);  // a, b, c, d

  EXPECT_TRUE(same_items(arc_information(view), arcs_before));
  EXPECT_TRUE(same_items(node_information(view), nodes_before));
}

TEST(ConstGraphViewAlgorithms, the_algorithms_without_bookkeeping_run_on_a_view)
{
  Market_Sample negative(1);
  const Market_View view(negative.g);
  const auto arcs_before = arc_information(view);

  static_assert(ArcDistance<Read_Distance, Market_View>);

  // The same search, on the graph and through the view.
  const auto on_graph = most_negative_cycle_bounded(negative.g, 3);
  const auto on_view = most_negative_cycle_bounded(view, 3, Read_Distance());
  static_assert(std::is_same_v<decltype(on_view.cycle_arcs), DynList<const Market::Arc *>>);
  static_assert(std::is_same_v<decltype(on_view.cycle_nodes), DynList<const Market::Node *>>);
  static_assert(std::is_same_v<decltype(on_graph.cycle_arcs), DynList<Market::Arc *>>);

  ASSERT_TRUE(on_graph.has_cycle);
  ASSERT_TRUE(on_view.has_cycle);
  EXPECT_EQ(on_view.total_cost, -2);
  EXPECT_EQ(on_view.total_cost, on_graph.total_cost);
  EXPECT_EQ(on_view.cycle_arcs.size(), 3u);
  EXPECT_EQ(on_view.cycle_arcs.size(), on_graph.cycle_arcs.size());

  long recomputed = 0;
  on_view.cycle_arcs.for_each([&recomputed] (const Market::Arc *a) { recomputed += a->get_info(); });
  EXPECT_EQ(recomputed, -2);

  const auto cycles = find_disjoint_negative_cycles(view, 3, Negative_Cycle_Exclusion::Min_Weight_Arc,
                                                    Read_Distance());
  ASSERT_EQ(cycles.size(), 1u);
  EXPECT_EQ(cycles.get_first().total_cost, -2);
  EXPECT_EQ(cycles.get_first().length, 3u);

  EXPECT_TRUE(same_items(arc_information(view), arcs_before));

  // Without a negative cycle the bounded search reports the cheapest cycle
  // and the disjoint search reports nothing, on the graph and on the view.
  Market_Sample positive(8);
  const auto positive_view = const_view(positive.g);
  const auto cheapest = most_negative_cycle_bounded(positive_view, 3, Read_Distance());
  ASSERT_TRUE(cheapest.has_cycle);
  EXPECT_EQ(cheapest.total_cost, 5);
  EXPECT_EQ(cheapest.total_cost, most_negative_cycle_bounded(positive.g, 3).total_cost);
  EXPECT_TRUE(find_disjoint_negative_cycles(positive_view, 3, Negative_Cycle_Exclusion::Min_Weight_Arc,
                                            Read_Distance()).is_empty());
  EXPECT_TRUE(find_disjoint_negative_cycles(positive.g, 3).is_empty());
}
