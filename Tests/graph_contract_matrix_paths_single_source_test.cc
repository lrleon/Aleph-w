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
 * @file graph_contract_matrix_paths_single_source_test.cc
 * @brief The contract matrix of the single-source shortest paths: Dijkstra,
 *        A*, IDA*, Dial and 0-1 BFS, and their common base.
 *
 * Each test instantiates one template of shortest_path_common.H,
 * Dijkstra.H, AStar.H, IDA_Star.H, Dial.H or Zero_One_BFS.H with every kind
 * of graph and every arc filter of graph_filter_catalog.H, on a const graph
 * too, and calls each of its public entry points once, on a small graph
 * whose shortest path needs the arc that the filters hide. The distance is
 * the default one, Dft_Dist, which reads the info of the arcs: the weights
 * are the numbers of the arcs. Each filter gets a graph of its own; what
 * the painting of one leaves in the arc that the next one hides is a known
 * defect, checked apart.
 *
 * The combinations that should compile and do not are left out here;
 * graph_contract_compile_check.cmake checks that they still fail.
 */

#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <shortest_path_common.H>
#include <Dijkstra.H>
#include <AStar.H>
#include <IDA_Star.H>
#include <Dial.H>
#include <Zero_One_BFS.H>

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
  /** 0 - 1 - 2 - 3, with two parallel arcs from 1 to 2. The weights are
      the numbers of the arcs: the hidden one, number 2, is on the only
      shortest path from 0 to 3, of cost 0 + 2 + 1, and on the only
      minimum spanning tree; without it they take the arc 3. */
  Spec ladder()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {2, 3}, {1, 2}, {1, 2}};
    return s;
  }

  /// The cost of the shortest path from 0 to 3 of ladder(), and of its
  /// minimum spanning tree
  template <class GT, class SA>
  constexpr int ladder_cost = hides<GT, SA> ? 4 : 3;

  /// The arcs of the shortest path from 0 to 3 of ladder(), and of its
  /// minimum spanning tree
  template <class GT, class SA>
  std::set<int> ladder_arcs()
  {
    return hides<GT, SA> ? std::set<int>{0, 1, 3} : std::set<int>{0, 1, 2};
  }

  /// 0 - 1 - 2 and the hidden arc 0 - 2, whose weight 2 0-1 BFS rejects
  Spec triangle()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {0, 2}};
    return s;
  }

  /// The numbers of the arcs of a path
  template <class GT>
  std::set<int> path_arcs(const Path<GT> & p)
  {
    std::set<int> r;
    p.for_each_arc([&r] (typename GT::Arc * a) { r.insert(a->get_info()); });
    return r;
  }

  /// The numbers of the arcs of a graph, such as a tree that an algorithm
  /// made, whose arcs keep the infos of those of the graph
  template <class GT>
  std::set<int> graph_arcs(const GT & g)
  {
    std::set<int> r;
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      r.insert(it.get_curr_ne()->get_info());
    return r;
  }
} // namespace

// On the six kinds

template <class GT>
class ContractPathsSingleSource : public ::testing::Test {};

TYPED_TEST_SUITE(ContractPathsSingleSource, All_Kinds);

TYPED_TEST(ContractPathsSingleSource, ShortestPathBase)
{
  using GT = TypeParam;

  // Before a computation: the base cannot paint by itself
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ladder());
      SCOPED_TRACE(name);
      Shortest_Path_Base<GT, Dft_Dist<GT>, Node_Arc_Iterator, SA, ArcHeap> x(Dft_Dist<GT>(), sa);
      EXPECT_FALSE(x.has_computation());
      EXPECT_FALSE(x.is_painted());
      EXPECT_EQ(x.get_start_node(), nullptr);
      EXPECT_EQ(x.get_graph(), nullptr);
      EXPECT_THROW(x.get_distance(b.nodes[3]), std::domain_error);
      Path<GT> path(b.g);
      EXPECT_THROW(x.get_min_path(b.nodes[3], path), std::domain_error);
    });
}

TYPED_TEST(ContractPathsSingleSource, DijkstraMinPaths)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      using Dijkstra = Dijkstra_Min_Paths<GT, Dft_Dist<GT>, Node_Arc_Iterator, SA>;
      Built<GT> b(ladder());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();

      for_each_constness(b.g, [&] (auto & g)
        {
          Dijkstra x(Dft_Dist<GT>(), sa);
          Path<GT> path(g);
          EXPECT_EQ(x.find_min_path(g, s, t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
          EXPECT_EQ(x(g, s, t, path), cost);
          EXPECT_TRUE(x.paint_partial_min_paths_tree(g, s, t));
          x.paint_min_paths_tree(g, s);
          EXPECT_TRUE(x.has_computation());
          EXPECT_TRUE(x.is_painted());
          EXPECT_EQ(x.get_start_node(), s);
          EXPECT_EQ(x.get_graph(), &b.g);
          EXPECT_EQ(x.get_distance(t), cost);
          EXPECT_EQ(x.get_min_path(t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);

          GT tree;
          EXPECT_NE(x.compute_min_paths_tree(g, s, tree), nullptr);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x.compute_partial_min_paths_tree(g, s, t, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x(g, s, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);

          Path<GT> on_tree(g);
          EXPECT_EQ(x.get_min_path(tree, t, on_tree), cost);
          EXPECT_EQ(path_arcs(on_tree), arcs);
        });

      // copy_painted_min_paths_tree() takes the graph as GT &
      Dijkstra x(Dft_Dist<GT>(), sa);
      x.paint_min_paths_tree(b.g, s);
      GT tree;
      EXPECT_EQ(x.copy_painted_min_paths_tree(b.g, tree), cost);
      EXPECT_EQ(graph_arcs(tree), arcs);
      EXPECT_FALSE(x.is_painted());
    });
}

TYPED_TEST(ContractPathsSingleSource, AStarMinPath)
{
  using GT = TypeParam;
  using Heuristic = Zero_Heuristic<GT, Dft_Dist<GT>>;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      using AStar = AStar_Min_Path<GT, Dft_Dist<GT>, Heuristic, Node_Arc_Iterator, SA>;
      Built<GT> b(ladder());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();

      for_each_constness(b.g, [&] (auto & g)
        {
          AStar x(Dft_Dist<GT>(), Heuristic(), sa);
          Path<GT> path(g);
          EXPECT_EQ(x.find_path(g, s, t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
          EXPECT_EQ(x(g, s, t, path), cost);
          EXPECT_EQ(x.find_min_path(g, s, t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
          EXPECT_TRUE(x.paint_partial_path(g, s, t));
          EXPECT_TRUE(x.paint_path(g, s, t));
          EXPECT_TRUE(x.paint_partial_min_paths_tree(g, s, t));
          x.paint_min_paths_tree(g, s);
          EXPECT_TRUE(x.has_computation());
          EXPECT_TRUE(x.is_painted());
          EXPECT_EQ(x.get_start_node(), s);
          EXPECT_EQ(x.get_graph(), &b.g);
          EXPECT_EQ(x.get_distance(t), cost);
          EXPECT_EQ(x.get_min_path(t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);

          GT tree;
          EXPECT_NE(x.compute_partial_path(g, s, t, tree), nullptr);
          EXPECT_EQ(graph_arcs(tree), arcs);
          EXPECT_NE(x.compute_path(g, s, t, tree), nullptr);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x.compute_partial_min_paths_tree(g, s, t, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x(g, s, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          EXPECT_NE(x.compute_min_paths_tree(g, s, tree), nullptr);
          EXPECT_EQ(graph_arcs(tree), arcs);

          Path<GT> on_tree(g);
          EXPECT_EQ(x.get_min_path(tree, t, on_tree), cost);
          EXPECT_EQ(path_arcs(on_tree), arcs);
        });

      // copy_painted_min_paths_tree() takes the graph as GT &
      AStar x(Dft_Dist<GT>(), Heuristic(), sa);
      x.paint_min_paths_tree(b.g, s);
      GT tree;
      EXPECT_EQ(x.copy_painted_min_paths_tree(b.g, tree), cost);
      EXPECT_EQ(graph_arcs(tree), arcs);
    });
}

TYPED_TEST(ContractPathsSingleSource, DijkstraAndAStarAfterAnotherPainting)
{
  using GT = TypeParam;
  using Heuristic = Zero_Heuristic<GT, Dft_Dist<GT>>;
  const Spec spec = ladder();
  Built<GT> b(spec);
  auto * s = b.nodes[0];
  auto * t = b.nodes[3];

  // A painting without the filter leaves the bit Spanning_Tree in the arc
  // that the next filter hides: the next computation must reset it, and
  // neither the path nor the copied tree may take that arc
  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      if constexpr (hides<GT, SA>)
        {
          SCOPED_TRACE(name);
          Dijkstra_Min_Paths<GT> unfiltered;
          Path<GT> path(b.g);

          unfiltered.paint_min_paths_tree(b.g, s);
          Dijkstra_Min_Paths<GT, Dft_Dist<GT>, Node_Arc_Iterator, SA> x(Dft_Dist<GT>(), sa);
          EXPECT_EQ(x.find_min_path(b.g, s, t, path), 4);
          EXPECT_EQ(path_arcs(path).count(Hidden), 0U);

          unfiltered.paint_min_paths_tree(b.g, s);
          AStar_Min_Path<GT, Dft_Dist<GT>, Heuristic, Node_Arc_Iterator, SA> y(Dft_Dist<GT>(), Heuristic(), sa);
          EXPECT_EQ(y.find_path(b.g, s, t, path), 4);
          EXPECT_EQ(path_arcs(path).count(Hidden), 0U);

          unfiltered.paint_min_paths_tree(b.g, s);
          x.paint_min_paths_tree(b.g, s);
          GT tree;
          EXPECT_EQ(x.copy_painted_min_paths_tree(b.g, tree), 4);
          EXPECT_EQ(tree.get_num_arcs(), 3U);
        }
    });
}

TYPED_TEST(ContractPathsSingleSource, IdaStar)
{
  using GT = TypeParam;
  using Heuristic = Zero_Heuristic<GT, Dft_Dist<GT>>;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ladder());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();
      for_each_constness(b.g, [&] (auto & g)
        {
          IDA_Star<GT, Dft_Dist<GT>, Heuristic, Node_Arc_Iterator, SA> x(Dft_Dist<GT>(), Heuristic(), sa);
          Path<GT> path(g);
          EXPECT_EQ(x.find_path(g, s, t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
          EXPECT_EQ(x(g, s, t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
        });
    });
}

TYPED_TEST(ContractPathsSingleSource, DialMinPaths)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ladder());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();
      for_each_constness(b.g, [&] (auto & g)
        {
          Dial_Min_Paths<GT, Dft_Dist<GT>, Node_Arc_Iterator, SA> x(Dft_Dist<GT>(), sa);
          Path<GT> path(g);
          EXPECT_EQ(x.find_min_path(g, s, t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
          EXPECT_EQ(x(g, s, t, path), cost);
          x.paint_min_paths_tree(g, s);
          EXPECT_TRUE(x.is_painted());
          EXPECT_EQ(x.get_start_node(), s);
          EXPECT_EQ(x.get_graph(), &b.g);
          EXPECT_EQ(x.get_distance(t), cost);
          EXPECT_EQ(x.get_min_path(t, path), cost);
          EXPECT_EQ(path_arcs(path), arcs);
        });
    });
}

TYPED_TEST(ContractPathsSingleSource, ZeroOneBfs)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(triangle());
      auto * s = b.nodes[0];
      auto * t = b.nodes[2];
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          Zero_One_BFS<GT, Dft_Dist<GT>, Node_Arc_Iterator, SA> x(Dft_Dist<GT>(), sa);
          Path<GT> path(g);
          if constexpr (not hides<GT, SA>)
            { // the arc of weight 2 is seen
              EXPECT_THROW(x.find_min_path(g, s, t, path), std::domain_error);
              EXPECT_THROW(x.paint_min_paths_tree(g, s), std::domain_error);
            }
          else
            {
              EXPECT_EQ(x.find_min_path(g, s, t, path), 1);
              EXPECT_EQ(path_arcs(path), (std::set<int>{0, 1}));
              EXPECT_EQ(x(g, s, t, path), 1);
              x.paint_min_paths_tree(g, s);
              EXPECT_TRUE(x.is_painted());
              EXPECT_EQ(x.get_start_node(), s);
              EXPECT_EQ(x.get_graph(), &b.g);
              EXPECT_EQ(x.get_distance(t), 1);
              EXPECT_EQ(x.get_min_path(t, path), 1);
              EXPECT_EQ(path_arcs(path), (std::set<int>{0, 1}));
            }
        });
    });
}
