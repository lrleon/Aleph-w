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
 * @file graph_contract_matrix_paths_all_pairs_test.cc
 * @brief The contract matrix of the shortest paths of all the pairs, of the
 *        k shortest paths and of the minimum spanning trees: Floyd-Warshall,
 *        Johnson, Yen, Eppstein, Kruskal and Prim.
 *
 * Each test instantiates one template of Floyd_Warshall.H, Johnson.H,
 * K_Shortest_Paths.H, Kruskal.H or Prim.H with every kind of graph that its
 * documentation supports and every arc filter of graph_filter_catalog.H, on
 * a const graph too, and calls each of its public entry points once, on a
 * small graph whose shortest path or tree needs the arc that the filters
 * hide. The distance is the default one, Dft_Dist, which reads the info of
 * the arcs: the weights are the numbers of the arcs, unless a test writes
 * others in the infos.
 *
 * Every combination of these templates compiles, as it should. A check
 * that fails at run time from a defect found by the matrix is marked as a
 * known failure, which must keep failing until the defect is fixed.
 */

#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Floyd_Warshall.H>
#include <Johnson.H>
#include <K_Shortest_Paths.H>
#include <Kruskal.H>
#include <Prim.H>

#include "graph_harness.H"
#include "graph_filter_catalog.H"
#include "graph_contract_matrix.H"

using namespace Aleph;
using namespace graph_harness;
using namespace graph_contract_matrix;

namespace
{
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

  /// The numbers of the arcs of g painted as a spanning tree
  template <class GT>
  std::set<int> painted_arcs(const GT & g)
  {
    std::set<int> r;
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      if (IS_ARC_VISITED(it.get_curr_ne(), Aleph::Spanning_Tree))
        r.insert(it.get_curr_ne()->get_info());
    return r;
  }

  /** 0 -> 1 -> 2 -> 3 through the hidden arc, number 2, against 0 -> 1 -> 3
      through the arc 4, and the arc 3 from 3 to 0. The weights are the
      numbers of the arcs: the shortest path from 0 to 3 costs 0 + 2 + 1,
      or 0 + 4 without the hidden arc. */
  Spec diamond()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {2, 3}, {1, 2}, {3, 0}, {1, 3}};
    return s;
  }

  /// Whether a path of a list of k shortest paths has the hidden arc
  template <class List>
  bool some_path_has_hidden(const List & l)
  {
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      {
        bool has = false;
        it.get_curr().path.for_each_arc([&has] (auto a) { has = has or a->get_info() == Hidden; });
        if (has)
          return true;
      }
    return false;
  }
} // namespace

// On the six kinds

template <class GT>
class ContractPathsAllPairs : public ::testing::Test {};

TYPED_TEST_SUITE(ContractPathsAllPairs, All_Kinds);

TYPED_TEST(ContractPathsAllPairs, FloydAllShortestPaths)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      using Floyd = Floyd_All_Shortest_Paths<GT, Dft_Dist<GT>, SA>;
      Built<GT> b(ladder());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();
      for_each_constness(b.g, [&] (auto & g)
        {
          // The filter as an lvalue and as an rvalue, with a distance, and
          // deduced from them
          Floyd x(g, sa);
          SA moved = sa;
          Floyd y(g, std::move(moved));
          Floyd z(g, Dft_Dist<GT>(), sa);
          Floyd_All_Shortest_Paths w(g, Dft_Dist<GT>(), sa);
          static_assert(std::is_same_v<decltype(w), Floyd>);
          for (const Floyd * f : {&x, &y, &z, &w})
            {
              EXPECT_FALSE(f->has_negative_cycle());
              const long i = f->index_node(s);
              const long j = f->index_node(t);
              EXPECT_EQ(f->select_node(i), s);
              EXPECT_EQ(f->get_dist_mat()(i, j), cost);
              EXPECT_EQ(path_arcs(f->get_min_path(s, t)), arcs);
              EXPECT_EQ(path_arcs(f->get_min_path(i, j)), arcs);
            }
          EXPECT_EQ(x.get_nodes().size(), 4U);
          EXPECT_EQ(x.get_nodes_ref().size(), 4U);
          EXPECT_EQ(x.get_nodes_copy().size(), 4U);
          EXPECT_EQ(x.get_path_mat()(x.index_node(s), x.index_node(s)), x.index_node(s));
          EXPECT_EQ(x.entry(cost), std::to_string(cost));
          testing::internal::CaptureStdout();
          x.print(x.get_dist_mat());
          EXPECT_NE(testing::internal::GetCapturedStdout().find(std::to_string(cost)),
                    std::string::npos);
        });
    });
}

TYPED_TEST(ContractPathsAllPairs, YenKShortestPaths)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ladder());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      constexpr bool hidden = hides<GT, SA>;
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();
      for_each_constness(b.g, [&] (auto & g)
        {
          const auto explicit_args =
            yen_k_shortest_paths<GT, Dft_Dist<GT>, SA>(g, s, t, 2, Dft_Dist<GT>(), sa);
          const auto deduced = yen_k_shortest_paths(g, s, t, 2, Dft_Dist<GT>(), sa);
          const Yen_K_Shortest_Paths<GT, Dft_Dist<GT>, SA> yen(Dft_Dist<GT>(), sa);
          const auto by_object = yen(g, s, t, 2);
          for (const auto * l : {&explicit_args, &deduced, &by_object})
            {
              // Two simple paths, of costs 3 and 4, or the one without the
              // hidden arc
              ASSERT_EQ(l->size(), hidden ? 1U : 2U);
              EXPECT_EQ(l->get_first().total_cost, cost);
              EXPECT_EQ(path_arcs(l->get_first().path), arcs);
              EXPECT_EQ(some_path_has_hidden(*l), not hidden);
            }
        });
    });
}

TYPED_TEST(ContractPathsAllPairs, EppsteinKShortestPaths)
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
          const auto explicit_args =
            eppstein_k_shortest_paths<GT, Dft_Dist<GT>, SA>(g, s, t, 2, Dft_Dist<GT>(), sa);
          const auto deduced = eppstein_k_shortest_paths(g, s, t, 2, Dft_Dist<GT>(), sa);
          const Eppstein_K_Shortest_Paths<GT, Dft_Dist<GT>, SA> eppstein(Dft_Dist<GT>(), sa);
          const auto by_object = eppstein(g, s, t, 2);
          for (const auto * l : {&explicit_args, &deduced, &by_object})
            {
              // The paths may repeat nodes, through the arc of weight 0
              ASSERT_FALSE(l->is_empty());
              EXPECT_EQ(l->get_first().total_cost, cost);
              EXPECT_EQ(path_arcs(l->get_first().path), arcs);
              if (hides<GT, SA>)
                {
                  EXPECT_FALSE(some_path_has_hidden(*l));
                }
            }
        });
    });
}

// Johnson, on the digraphs

template <class GT>
class ContractPathsJohnson : public ::testing::Test {};

TYPED_TEST_SUITE(ContractPathsJohnson, Directed_Kinds);

TYPED_TEST(ContractPathsJohnson, Johnson)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(diamond());
      auto * s = b.nodes[0];
      auto * t = b.nodes[3];
      SCOPED_TRACE(name);
      const int cost = hides<GT, SA> ? 4 : 3;
      const auto arcs = hides<GT, SA> ? std::set<int>{0, 4} : std::set<int>{0, 1, 2};
      for_each_constness(b.g, [&] (auto & g)
        {
          Johnson<GT, Dft_Dist<GT>, Arc_Iterator, Node_Arc_Iterator, SA> x(g, Dft_Dist<GT>(), sa);
          EXPECT_TRUE(x.is_initialized());
          EXPECT_EQ(x.get_distance(s, t), cost);
          EXPECT_EQ(path_arcs(x.find_path(s, t)), arcs);
          const auto all = x.compute_all_pairs_distances();
          EXPECT_EQ(all.find(std::make_pair(s, t)), cost);
          EXPECT_EQ(x.get_potential(s), 0);
          EXPECT_EQ(x.get_reweighted_graph().get_num_nodes(), 4U);
          EXPECT_EQ(std::as_const(x).get_reweighted_graph().get_num_arcs(), 5U);
          EXPECT_NE(x.get_copy_node(s), nullptr);
        });
    });
}

TYPED_TEST(ContractPathsJohnson, JohnsonFiltersTheArcsOfTheGraph)
{
  using GT = TypeParam;

  // 0 -> 1 (weight -1), 1 -> 2 (5), the hidden 0 -> 2 (2), 2 -> 3 (0) and
  // 1 -> 3 (3): without the hidden arc the shortest path from 0 to 3 is
  // 0 -> 1 -> 3, of cost 2. The weights are written in the infos; only
  // the hidden arc has the info 2.
  Spec spec;
  spec.n = 4;
  spec.arcs = {{0, 1}, {1, 2}, {0, 2}, {2, 3}, {1, 3}};
  Built<GT> b(spec);
  const int weight[] = {-1, 5, 2, 0, 3};
  for (size_t i = 0; i < b.arcs.size(); ++i)
    b.arcs[i]->get_info() = weight[i];

  // A case of its own, which the matrix found: Johnson gave the filter the
  // arcs of its copy of the graph, whose infos it had overwritten with the
  // reweighted weights, so that the filter read 2 in the copy of 1 -> 3
  // (3 + h(1) - h(3) = 3 - 1 - 0) and hid it
  Check johnson("Johnson with a filter that reads the infos of the arcs");
  Check bellman_ford("Bellman_Ford on the same graph and filter");

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      if constexpr (hides<GT, SA>)
        {
          johnson.run();
          Johnson<GT, Dft_Dist<GT>, Arc_Iterator, Node_Arc_Iterator, SA> x(b.g, Dft_Dist<GT>(), sa);
          const int d = x.get_distance(b.nodes[0], b.nodes[3]);
          if (d != 2)
            johnson.fail(spec, std::string(name) + ": distance " + std::to_string(d) + ", not 2");

          bellman_ford.run();
          Bellman_Ford<GT, Dft_Dist<GT>, Arc_Iterator, Out_Iterator, SA> y(b.g, Dft_Dist<GT>(), sa);
          y.paint_spanning_tree(b.nodes[0]);
          const int e = y.get_distance(b.nodes[3]);
          if (e != 2)
            bellman_ford.fail(spec, std::string(name) + ": distance " + std::to_string(e) + ", not 2");
        }
    });

  johnson.expect();
  bellman_ford.expect();
}

// The minimum spanning trees, on the kinds that are not directed

template <class GT>
class ContractPathsSpanningTree : public ::testing::Test {};

TYPED_TEST_SUITE(ContractPathsSpanningTree, Undirected_Kinds);

TYPED_TEST(ContractPathsSpanningTree, KruskalMinSpanningTree)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ladder());
      SCOPED_TRACE(name);
      const auto arcs = ladder_arcs<GT, SA>();
      for_each_constness(b.g, [&] (auto & g)
        {
          Kruskal_Min_Spanning_Tree<GT, Dft_Dist<GT>, SA> x(Dft_Dist<GT>(), sa);
          x.paint_min_spanning_tree(g);
          EXPECT_TRUE(x.is_painted());
          EXPECT_EQ(painted_arcs(g), arcs);
          GT tree;
          x.paint_min_spanning_tree(g, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x(g, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x(g);
          EXPECT_EQ(painted_arcs(g), arcs);
        });
    });
}

TYPED_TEST(ContractPathsSpanningTree, PrimMinSpanningTree)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ladder());
      SCOPED_TRACE(name);
      const auto arcs = ladder_arcs<GT, SA>();
      for_each_constness(b.g, [&] (auto & g)
        {
          Prim_Min_Spanning_Tree<GT, Dft_Dist<GT>, SA> x(Dft_Dist<GT>(), sa);
          GT tree;
          x(g, tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x(g, b.nodes[3], tree);
          EXPECT_EQ(graph_arcs(tree), arcs);
          x(g);
          EXPECT_EQ(painted_arcs(g), arcs);
          x(g, b.nodes[3]);
          EXPECT_EQ(painted_arcs(g), arcs);
        });
    });
}
