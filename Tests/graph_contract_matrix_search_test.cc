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
 * @file graph_contract_matrix_search_test.cc
 * @brief The contract of the path searches, the components, the cut nodes
 *        and bridges, the spanning trees and the tests of acyclicity,
 *        connectivity, cycles and paths, with every kind of arc filter.
 *
 * The ArcFilter concept of ah-graph-concepts.H asks a filter only to be a
 * predicate on an arc. Each test calls every public entry point of one
 * template of tpl_find_path.H, Bidirectional_BFS.H, tpl_components.H,
 * tpl_cut_nodes.H, tpl_spanning_tree.H, tpl_test_acyclique.H,
 * tpl_test_connectivity.H, tpl_test_cycle.H or tpl_test_path.H once, on a
 * small graph of each kind that the template supports, with each filter
 * of graph_filter_catalog.H and with Dft_Show_Arc, passed as an lvalue
 * and as an rvalue where the template takes both, on a graph and on a
 * const graph where it takes a const one. The graphs are such that the
 * answer changes when the arc that the filters hide, number 2, is hidden,
 * so that a check of the answer is a check that the filter is used.
 *
 * The combinations that should compile and do not are left out here, by
 * the trait of the filter that triggers each, and are the known failures
 * of graph_contract_compile_check.cmake, which names them.
 */

#include <set>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_find_path.H>
#include <Bidirectional_BFS.H>
#include <tpl_components.H>
#include <tpl_cut_nodes.H>
#include <tpl_spanning_tree.H>
#include <tpl_test_acyclique.H>
#include <tpl_test_connectivity.H>
#include <tpl_test_cycle.H>
#include <tpl_test_path.H>

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
  /// 0 - 1 - 2 - 3, and the arc 2, from 1 to 3, which skips 2
  Spec shortcut()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {1, 2}, {1, 3}, {2, 3}};
    return s;
  }

  /// 0 - 1 and 2 - 3, which the arc 2, from 1 to 2, joins
  Spec halves()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {2, 3}, {1, 2}};
    return s;
  }

  /// 0 - 1 - 2, which the arc 2, from 2 to 0, closes in a cycle
  Spec triangle()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {2, 0}};
    return s;
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

  /// The numbers of the arcs of a path
  template <class GT>
  std::set<int> path_arcs(const Path<GT> & p)
  {
    std::set<int> r;
    p.for_each_arc([&r] (typename GT::Arc * a) { r.insert(a->get_info()); });
    return r;
  }

  /// The numbers of the nodes of a container of nodes
  template <class List>
  std::set<int> node_numbers(const List & l)
  {
    std::set<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.insert(num(it.get_curr()));
    return r;
  }

  /// Expects p to go from the node from to the node to, without the arc
  /// Hidden if hidden
  template <class GT>
  void expect_path(const Path<GT> & p, int from, int to, bool hidden)
  {
    ASSERT_FALSE(p.is_empty());
    EXPECT_EQ(num(p.get_first_node()), from);
    EXPECT_EQ(num(p.get_last_node()), to);
    if (hidden)
      {
        EXPECT_EQ(path_arcs(p).count(Hidden), 0U) << "the path takes the hidden arc";
      }
  }

  // What some calls need of a filter, and the ArcFilter concept does not
  // ask, by which the tests leave out their known failures: they build the
  // filter with SA()

  /// Whether a filter can be built with SA()
  template <class SA>
  constexpr bool default_built = std::is_default_constructible_v<SA>;
} // namespace

template <class GT>
class ContractSearchAll : public ::testing::Test {};

TYPED_TEST_SUITE(ContractSearchAll, All_Kinds);

template <class GT>
class ContractSearchUndirected : public ::testing::Test {};

TYPED_TEST_SUITE(ContractSearchUndirected, Undirected_Kinds);

template <class GT>
class ContractSearchDirected : public ::testing::Test {};

TYPED_TEST_SUITE(ContractSearchDirected, Directed_Kinds);

TYPED_TEST(ContractSearchAll, FindPathDepthFirst)
{
  using GT = TypeParam;
  Built<GT> b(shortcut());
  auto * s = b.nodes[0];
  auto * t = b.nodes[3];
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          auto goal = [t] (typename GT::Node * p) { return p == t; };
          Find_Path_Depth_First<GT, Node_Arc_Iterator, SA> lvalue(sa);
          Path<GT> p(g);
          EXPECT_TRUE(lvalue(g, s, t, p));
          expect_path(p, 0, 3, hiding);
          expect_path(lvalue(g, s, t), 0, 3, hiding);
          expect_path(lvalue(g, s, goal), 0, 3, hiding);

          SA copy = sa;
          Find_Path_Depth_First<GT, Node_Arc_Iterator, SA> rvalue(std::move(copy));
          expect_path(rvalue(g, s, [t] (typename GT::Node * p) { return p == t; }),
                      0, 3, hiding);

          if constexpr (default_built<SA>) // a filter built by default hides nothing
            expect_path(Find_Path_Depth_First<GT, Node_Arc_Iterator, SA>()(g, s, t),
                        0, 3, false);
        });
    });
}

TYPED_TEST(ContractSearchAll, FindPathBreadthFirst)
{
  using GT = TypeParam;
  Built<GT> b(shortcut());
  auto * s = b.nodes[0];
  auto * t = b.nodes[3];
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          auto goal = [t] (typename GT::Node * p) { return p == t; };
          Find_Path_Breadth_First<GT, Node_Arc_Iterator, SA> lvalue(sa);
          Path<GT> p(g);
          EXPECT_TRUE(lvalue(g, s, t, p));
          expect_path(p, 0, 3, hiding);
          expect_path(lvalue(g, s, t), 0, 3, hiding);
          expect_path(lvalue(g, s, goal), 0, 3, hiding);

          SA copy = sa;
          Find_Path_Breadth_First<GT, Node_Arc_Iterator, SA> rvalue(std::move(copy));
          expect_path(rvalue(g, s, goal), 0, 3, hiding);

          if constexpr (default_built<SA>)
            expect_path(Find_Path_Breadth_First<GT, Node_Arc_Iterator, SA>()(g, s, t),
                        0, 3, false);
        });
    });
}

TYPED_TEST(ContractSearchAll, DirectedFindPath)
{
  using GT = TypeParam;
  Built<GT> b(shortcut()); // each arc goes from its first node to its second
  auto * s = b.nodes[0];
  auto * t = b.nodes[3];
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          auto goal = [t] (typename GT::Node * p) { return p == t; };
          Directed_Find_Path<GT, Out_Iterator, SA> lvalue(g, sa);
          expect_path(lvalue.dfs(s, t), 0, 3, hiding);
          expect_path(lvalue.bfs(s, t), 0, 3, hiding);
          expect_path(lvalue.dfs(s, goal), 0, 3, hiding);
          expect_path(lvalue.bfs(s, goal), 0, 3, hiding);

          SA copy = sa;
          Directed_Find_Path<GT, Out_Iterator, SA> rvalue(g, std::move(copy));
          expect_path(rvalue.dfs(s, [t] (typename GT::Node * p) { return p == t; }),
                      0, 3, hiding);
          expect_path(rvalue.bfs(s, [t] (typename GT::Node * p) { return p == t; }),
                      0, 3, hiding);

          if constexpr (default_built<SA>)
            expect_path(Directed_Find_Path<GT, Out_Iterator, SA>(g).bfs(s, t), 0, 3, false);
        });
    });
}

TYPED_TEST(ContractSearchUndirected, BidirectionalBFS)
{
  using GT = TypeParam;
  Built<GT> b(shortcut());
  auto * s = b.nodes[0];
  auto * t = b.nodes[3];
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      using BiBFS = Bidirectional_BFS<GT, Node_Arc_Iterator, Node_Arc_Iterator, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          // The filter as an lvalue, const or not
          BiBFS by_lvalue(sa);
          BiBFS by_const(std::as_const(sa));
          Path<GT> p(g);
          EXPECT_TRUE(by_lvalue.find_path(g, s, t, p));
          expect_path(p, 0, 3, hiding);
          EXPECT_TRUE(by_const(g, s, t, p));
          expect_path(p, 0, 3, hiding);
          expect_path(by_lvalue(g, s, t), 0, 3, hiding);

          SA copy = sa;
          BiBFS rvalue(std::move(copy));
          expect_path(rvalue(g, s, t), 0, 3, hiding);

          if constexpr (default_built<SA>)
            expect_path(BiBFS()(g, s, t), 0, 3, false);
        });
    });
}

TYPED_TEST(ContractSearchDirected, BidirectionalBFSOutIn)
{
  using GT = TypeParam;
  Built<GT> b(shortcut());
  auto * s = b.nodes[0];
  auto * t = b.nodes[3];
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      using BiBFS = Bidirectional_BFS<GT, Out_Iterator, In_Iterator, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          // The filter as an lvalue, const or not
          BiBFS by_lvalue(sa);
          BiBFS by_const(std::as_const(sa));
          Path<GT> p(g);
          EXPECT_TRUE(by_lvalue.find_path(g, s, t, p));
          expect_path(p, 0, 3, hiding);
          EXPECT_TRUE(by_const(g, s, t, p));
          expect_path(p, 0, 3, hiding);
          expect_path(by_lvalue(g, s, t), 0, 3, hiding);

          SA copy = sa;
          BiBFS rvalue(std::move(copy));
          expect_path(rvalue(g, s, t), 0, 3, hiding);

          if constexpr (default_built<SA>)
            expect_path(BiBFS()(g, s, t), 0, 3, false);
        });
    });
}

TYPED_TEST(ContractSearchAll, BuildSubgraph)
{
  using GT = TypeParam;
  Built<GT> b(halves()); // from 0, the arc 2 is the way to 2 and 3
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const size_t nodes = hiding ? 2 : 4;
      const std::set<int> arcs = hiding ? std::set<int>{0} : std::set<int>{0, 1, 2};
      for_each_constness(b.g, [&] (auto & g)
        {
          // The marks of a former copy would stop the next one
          Build_Subgraph<GT, SA> build(sa);
          g.reset_nodes();
          g.reset_arcs();
          GT sg;
          build(g, sg, b.nodes[0]);
          EXPECT_EQ(sg.get_num_nodes(), nodes);
          EXPECT_EQ(graph_arcs(sg), arcs);

          g.reset_nodes();
          g.reset_arcs();
          const GT copy = build(g, b.nodes[0]);
          EXPECT_EQ(copy.get_num_nodes(), nodes);
          EXPECT_EQ(graph_arcs(copy), arcs);

          g.reset_nodes();
          g.reset_arcs();
          DynList<typename GT::Node *> l;
          build(g, l, b.nodes[0]);
          EXPECT_EQ(l.size(), nodes);

          if constexpr (default_built<SA>)
            {
              g.reset_nodes();
              g.reset_arcs();
              EXPECT_EQ((Build_Subgraph<GT, SA>()(g, b.nodes[0]).get_num_nodes()), 4U);
            }
        });
    });
}

TYPED_TEST(ContractSearchAll, UnconnectedComponents)
{
  using GT = TypeParam;
  Built<GT> b(halves());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          Unconnected_Components<GT, SA> cc(sa);
          DynList<GT> blocks;
          cc(g, blocks);
          DynList<DynList<typename GT::Node *>> lists;
          cc(g, lists);
          const size_t count = cc.count_components(g);
          EXPECT_EQ(blocks.size(), count);
          EXPECT_EQ(lists.size(), count);
          size_t arcs = 0;
          for (auto it = blocks.get_it(); it.has_curr(); it.next_ne())
            {
              arcs += it.get_curr().get_num_arcs();
              if (hiding)
                {
                  EXPECT_EQ(graph_arcs(it.get_curr()).count(Hidden), 0U);
                }
            }

          // In a digraph the blocks follow the arcs out of the nodes in
          // the order of the nodes, which a List_SDigraph does not keep
          if (not g.is_digraph())
            {
              EXPECT_EQ(count, hiding ? 2U : 1U);
              EXPECT_EQ(cc.is_connected(g), not hiding);
              EXPECT_EQ(arcs, hiding ? 2U : 3U);
            }
          else
            {
              EXPECT_EQ(cc.is_connected(g), count == 1);
            }

          DynList<GT> blocks2;
          cc.template compute_blocks<DynList>(g, blocks2);
          EXPECT_EQ(blocks2.size(), count);
          DynList<DynList<typename GT::Node *>> lists2;
          cc.template compute_lists<DynList>(g, lists2);
          EXPECT_EQ(lists2.size(), count);

          if constexpr (default_built<SA>)
            if (not g.is_digraph())
              {
                EXPECT_EQ((Unconnected_Components<GT, SA>().count_components(g)), 1U);
              }
        });
    });
}

TYPED_TEST(ContractSearchUndirected, ComputeCutNodes)
{
  using GT = TypeParam;
  Built<GT> b(triangle()); // without the arc 2, 1 is a cut node
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const std::set<int> cut = hiding ? std::set<int>{1} : std::set<int>{};
      for_each_constness(b.g, [&] (auto & g)
        {
          Compute_Cut_Nodes<GT, SA> cc(g, sa);
          DynDlist<typename GT::Node *> l;
          cc(l);
          EXPECT_EQ(node_numbers(l), cut);
          cc(b.nodes[0], l);
          EXPECT_EQ(node_numbers(l), cut);
          cc.cut_nodes(b.nodes[0], l);
          EXPECT_EQ(node_numbers(l), cut);
          // the next color: two blocks around 1, or none painted
          EXPECT_EQ(cc.paint_subgraphs(), hiding ? 3 : 1);

          if (hiding)
            {
              GT block;
              cc.map_subgraph(block, 1);
              EXPECT_EQ(block.get_num_nodes(), 1U);
            }
          GT cut_graph;
          DynDlist<typename GT::Arc *> cross;
          cc.map_cut_graph(cut_graph, cross);
          EXPECT_EQ(cut_graph.get_num_nodes(), cut.size());
          const std::set<int> crossing = hiding ? std::set<int>{0, 1} : std::set<int>{};
          EXPECT_EQ(arc_numbers(cross), crossing);

          Compute_Cut_Nodes<GT, SA> again(g, sa);
          again(l);
          DynDlist<GT> blocks;
          GT cut_graph2;
          DynDlist<typename GT::Arc *> cross2;
          again.compute_blocks(blocks, cut_graph2, cross2);
          EXPECT_EQ(blocks.size(), hiding ? 2U : 1U);

          if constexpr (default_built<SA>)
            {
              Compute_Cut_Nodes<GT, SA> by_default(g);
              by_default(l);
              EXPECT_TRUE(l.is_empty());
            }
        });
    });
}

TYPED_TEST(ContractSearchUndirected, ComputeBridges)
{
  using GT = TypeParam;
  Built<GT> b(triangle()); // without the arc 2, the arcs 0 and 1 are bridges
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const std::set<int> want = hiding ? std::set<int>{0, 1} : std::set<int>{};
      for_each_constness(b.g, [&] (auto & g)
        {
          Compute_Bridges<GT, SA> cb(g, sa);
          EXPECT_EQ(arc_numbers(cb.find_bridges(b.nodes[0])), want);
          EXPECT_EQ(arc_numbers(cb.find_bridges()), want);
          EXPECT_EQ(arc_numbers(cb(b.nodes[0])), want);
          EXPECT_EQ(arc_numbers(cb()), want);

          if constexpr (default_built<SA>)
            {
              EXPECT_TRUE((Compute_Bridges<GT, SA>(g).find_bridges().is_empty()));
            }
        });
    });
}

TYPED_TEST(ContractSearchUndirected, FindBridges)
{
  using GT = TypeParam;
  Built<GT> b(triangle());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const std::set<int> want = hiding ? std::set<int>{0, 1} : std::set<int>{};
      for_each_constness(b.g, [&] (auto & g)
        {
          EXPECT_EQ(arc_numbers(find_bridges(g, b.nodes[0], sa)), want);
          if constexpr (default_built<SA>)
            {
              EXPECT_TRUE((find_bridges<GT, SA>(g, b.nodes[0]).is_empty()));
            }
        });
    });
}

TYPED_TEST(ContractSearchAll, FindDepthFirstSpanningTree)
{
  using GT = TypeParam;
  Built<GT> b(shortcut());
  Built<GT> apart(halves()); // without the arc 2, no node reaches all
  // In a digraph only 0 reaches every node, and the first node of a
  // List_SDigraph, which follows the addresses, may be another one, from
  // which no tree spans the graph
  const bool first_spans = not b.g.is_digraph() or num(b.g.get_first_node()) == 0;
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      auto expect_tree = [&] (const GT & tree, bool hidden)
        {
          EXPECT_EQ(tree.get_num_nodes(), 4U);
          EXPECT_EQ(tree.get_num_arcs() + 1, tree.get_num_nodes());
          if (hidden)
            {
              EXPECT_EQ(graph_arcs(tree).count(Hidden), 0U);
            }
        };
      // The tree takes a graph that is not const
      Find_Depth_First_Spanning_Tree<GT, SA> dfs(sa);
      GT tree;
      if (first_spans)
        {
          EXPECT_EQ(dfs(b.g, tree), b.g.get_first_node());
          expect_tree(tree, hiding);
        }
      else
        EXPECT_EQ(dfs(b.g, tree), nullptr);
      GT tree2;
      EXPECT_NE(dfs(b.g, b.nodes[0], tree2), nullptr);
      expect_tree(tree2, hiding);

      if (hiding)
        {
          GT forest;
          EXPECT_EQ(dfs(apart.g, forest), nullptr);
        }

      if constexpr (default_built<SA>)
        {
          GT tree3;
          auto * root = Find_Depth_First_Spanning_Tree<GT, SA>()(b.g, tree3);
          if (first_spans)
            {
              EXPECT_NE(root, nullptr);
              expect_tree(tree3, false);
            }
          else
            EXPECT_EQ(root, nullptr);
        }
    });
}

TYPED_TEST(ContractSearchAll, FindBreadthFirstSpanningTree)
{
  using GT = TypeParam;
  Built<GT> b(shortcut());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      // As in the depth-first tree: from the first node of a digraph,
      // maybe fewer nodes than from 0
      auto expect_tree = [&] (const GT & tree, size_t nodes, bool hidden)
        {
          if (nodes != 0)
            {
              EXPECT_EQ(tree.get_num_nodes(), nodes);
            }
          EXPECT_EQ(tree.get_num_arcs() + 1, tree.get_num_nodes());
          if (hidden)
            {
              EXPECT_EQ(graph_arcs(tree).count(Hidden), 0U);
            }
        };
      const size_t from_first = b.g.is_digraph() ? 0 : 4;
      // The tree takes a graph that is not const
      Find_Breadth_First_Spanning_Tree<GT, SA> bfs(sa);
      GT tree;
      bfs(b.g, b.nodes[0], tree);
      expect_tree(tree, 4, hiding);
      GT tree2;
      EXPECT_NE(bfs(b.g, tree2), nullptr);
      expect_tree(tree2, from_first, hiding);

      if constexpr (default_built<SA>)
        {
          GT tree3;
          EXPECT_NE((Find_Breadth_First_Spanning_Tree<GT, SA>()(b.g, tree3)), nullptr);
          expect_tree(tree3, from_first, false);
        }
    });
}

TYPED_TEST(ContractSearchUndirected, IsGraphAcyclique)
{
  using GT = TypeParam;
  Built<GT> b(triangle());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const bool acyclic = hiding;
      // The test takes a graph that is not const
      Is_Graph_Acyclique<GT, SA> lvalue(sa);
      EXPECT_EQ(lvalue(b.g), acyclic);
      EXPECT_EQ(lvalue(b.g, acyclic ? 2 : 3), acyclic);

      SA copy = sa;
      Is_Graph_Acyclique<GT, SA> rvalue(std::move(copy));
      EXPECT_EQ(rvalue(b.g), acyclic);

      if constexpr (default_built<SA>)
        {
          EXPECT_FALSE((Is_Graph_Acyclique<GT, SA>()(b.g)));
        }
    });
}

TYPED_TEST(ContractSearchUndirected, HasCycle)
{
  using GT = TypeParam;
  Built<GT> b(triangle());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const bool cycle = not hiding;
      // The test takes a graph that is not const
      const Has_Cycle<GT, SA> lvalue(sa);
      EXPECT_EQ(lvalue(b.g), cycle);
      EXPECT_EQ(lvalue(b.g, cycle ? 3 : 2), cycle);

      SA copy = sa;
      const Has_Cycle<GT, SA> rvalue(std::move(copy));
      EXPECT_EQ(rvalue(b.g), cycle);

      if constexpr (default_built<SA>)
        {
          EXPECT_TRUE((Has_Cycle<GT, SA>()(b.g)));
        }
    });
}

TYPED_TEST(ContractSearchUndirected, TestConnectivity)
{
  using GT = TypeParam;
  Built<GT> b(halves());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      const bool connected = not hiding;
      // The test takes a graph that is not const
      const Test_Connectivity<GT, SA> test;
      EXPECT_EQ(test(b.g, sa), connected);
      SA copy = sa;
      EXPECT_EQ(test(b.g, std::move(copy)), connected);

      if constexpr (default_built<SA>)
        {
          EXPECT_TRUE(test(b.g));
        }
    });
}

TYPED_TEST(ContractSearchAll, TestForCycle)
{
  using GT = TypeParam;
  Built<GT> b(triangle());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          const Test_For_Cycle<GT, SA> test(sa);
          EXPECT_EQ(test(g, b.nodes[0]), not hiding);

          if constexpr (default_built<SA>)
            {
              EXPECT_TRUE((Test_For_Cycle<GT, SA>()(g, b.nodes[0])));
            }
        });
    });
}

TYPED_TEST(ContractSearchAll, TestForPath)
{
  using GT = TypeParam;
  Built<GT> b(halves());
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool hiding = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          Test_For_Path<GT, SA> test(sa);
          EXPECT_EQ(test(g, b.nodes[0], b.nodes[3]), not hiding);

          if constexpr (default_built<SA>)
            {
              EXPECT_TRUE((Test_For_Path<GT, SA>()(g, b.nodes[0], b.nodes[3])));
            }
        });
    });
}
