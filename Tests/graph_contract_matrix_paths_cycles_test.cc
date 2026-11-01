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
 * @file graph_contract_matrix_paths_cycles_test.cc
 * @brief The contract matrix of Bellman-Ford and of the negative and the
 *        minimum mean cycles.
 *
 * Each test instantiates one template of Bellman_Ford.H, Min_Mean_Cycle.H,
 * Howard_Min_Mean_Cycle.H or Negative_Cycles.H with the three kinds of
 * digraph and every arc filter of graph_filter_catalog.H, on a const graph
 * too where it takes one, and calls each of its public entry points once,
 * on a small graph whose path or cycle needs the arc that the filters hide.
 * The distance is the default one, Dft_Dist, which reads the info of the
 * arcs: the weights are the numbers of the arcs, unless a test writes
 * others in the infos.
 *
 * The combinations that should compile and do not are left out here;
 * graph_contract_compile_check.cmake checks that they still fail.
 */

#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Bellman_Ford.H>
#include <Min_Mean_Cycle.H>
#include <Howard_Min_Mean_Cycle.H>
#include <Negative_Cycles.H>

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

  /// The cycle 0 -> 1 -> 2 -> 0, closed by the hidden arc; its mean is 1
  Spec ring()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {2, 0}};
    return s;
  }

  /** 0 -> 1 (weight -3), 1 -> 2 (1) and the hidden 1 -> 0 (2): the only
      cycle, of cost -1, needs the hidden arc. The weights are written in
      the infos, which Dft_Dist reads; only the hidden arc has the info 2,
      so the filters still hide it alone. */
  template <class GT>
  void make_negative(Built<GT> & b)
  {
    b.arcs[0]->get_info() = -3;
  }

  Spec negative()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {1, 0}};
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

// On the digraphs

template <class GT>
class ContractPathsCycles : public ::testing::Test {};

TYPED_TEST_SUITE(ContractPathsCycles, Directed_Kinds);

TYPED_TEST(ContractPathsCycles, BellmanFord)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      using BF = Bellman_Ford<GT, Dft_Dist<GT>, Arc_Iterator, Out_Iterator, SA>;
      Built<GT> l(ladder());
      Built<GT> neg(negative());
      make_negative(neg);
      SCOPED_TRACE(name);
      constexpr bool hidden = hides<GT, SA>;
      const int cost = ladder_cost<GT, SA>;
      const auto arcs = ladder_arcs<GT, SA>();

      // The constructor takes the graph as GT &. build_tree() goes last,
      // since it maps the cookies through which get_distance() walks
      {
        BF x(l.g, Dft_Dist<GT>(), sa);
        EXPECT_FALSE(x.has_computation());
        EXPECT_EQ(&x.get_graph(), &l.g);
        EXPECT_FALSE(x.paint_spanning_tree(l.nodes[0]));
        EXPECT_TRUE(x.is_painted());
        EXPECT_EQ(x.get_start_node(), l.nodes[0]);
        EXPECT_EQ(x.get_distance(l.nodes[3]), cost);
        Path<GT> path(l.g);
        EXPECT_EQ(x.get_min_path(l.nodes[3], path), cost);
        EXPECT_EQ(path_arcs(path), arcs);
        std::set<int> tree_arcs;
        const auto tree_array = x.extract_min_spanning_tree();
        for (size_t i = 0; i < tree_array.size(); ++i)
          if (tree_array(i) != nullptr)
            tree_arcs.insert(tree_array(i)->get_info());
        EXPECT_EQ(tree_arcs, arcs);
        GT tree;
        x.build_tree(tree, true);
        EXPECT_EQ(graph_arcs(tree), arcs);
      }
      {
        BF x(l.g, Dft_Dist<GT>(), sa);
        EXPECT_FALSE(x.faster_paint_spanning_tree(l.nodes[0]));
        EXPECT_EQ(x.get_distance(l.nodes[3]), cost);
        x.clear();
        EXPECT_FALSE(x.is_painted());
        EXPECT_EQ(x.compute_nodes_weights().size(), 4U);
      }

      // A negative cycle only through the hidden arc
      BF x(neg.g, Dft_Dist<GT>(), sa);
      EXPECT_EQ(x.has_negative_cycle(neg.nodes[0]), not hidden);
      EXPECT_EQ(x.has_negative_cycle(), not hidden);
      if constexpr (hidden)
        {
          EXPECT_FALSE(x.paint_spanning_tree(neg.nodes[0]));
          EXPECT_EQ(x.get_distance(neg.nodes[2]), -2);
        }

      EXPECT_EQ(x.test_negative_cycle(neg.nodes[0]).is_empty(), hidden);
      EXPECT_EQ(x.test_negative_cycle().is_empty(), hidden);
      EXPECT_EQ(std::get<0>(x.search_negative_cycle(neg.nodes[0], 0.7, 1)).is_empty(), hidden);
      EXPECT_EQ(x.search_negative_cycle(neg.nodes[0]).is_empty(), hidden);
      EXPECT_EQ(std::get<0>(x.search_negative_cycle(0.7, 1)).is_empty(), hidden);
      EXPECT_EQ(x.search_negative_cycle().is_empty(), hidden);
      Path<GT> cycle(neg.g);
      EXPECT_EQ(x.test_negative_cycle(neg.nodes[0], cycle), not hidden);
      EXPECT_EQ(x.test_negative_cycle(cycle), not hidden);
      EXPECT_EQ(path_arcs(cycle).count(Hidden), hidden ? 0U : 1U);
    });
}

TYPED_TEST(ContractPathsCycles, BellmanFordNegativeCycle)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> neg(negative());
      make_negative(neg);
      auto * s = neg.nodes[0];
      SCOPED_TRACE(name);
      constexpr bool hidden = hides<GT, SA>;

      const Bellman_Ford_Negative_Cycle<GT, Dft_Dist<GT>, Arc_Iterator,
                                        Node_Arc_Iterator, SA> x;
      Path<GT> path(neg.g);

      // The distance and the filter as lvalues
      Dft_Dist<GT> d;
      EXPECT_EQ(x(neg.g, path, d, sa), not hidden);
      EXPECT_EQ(x(neg.g, s, path, d, sa), not hidden);
      EXPECT_EQ(x(neg.g, s, d, sa).is_empty(), hidden);
      EXPECT_EQ(x(neg.g, d, sa).is_empty(), hidden);

      // And as rvalues
      EXPECT_EQ(x(neg.g, path, Dft_Dist<GT>(), SA(sa)), not hidden);
      EXPECT_EQ(x(neg.g, s, path, Dft_Dist<GT>(), SA(sa)), not hidden);
      EXPECT_EQ(x(neg.g, s, Dft_Dist<GT>(), SA(sa)).is_empty(), hidden);
      EXPECT_EQ(x(neg.g, Dft_Dist<GT>(), SA(sa)).is_empty(), hidden);
    });
}

TYPED_TEST(ContractPathsCycles, KarpMinimumMeanCycle)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ring());
      SCOPED_TRACE(name);
      constexpr bool hidden = hides<GT, SA>;
      for_each_constness(b.g, [&] (auto & g)
        {
          const auto r = karp_minimum_mean_cycle(g, Dft_Dist<GT>(), sa);
          EXPECT_EQ(r.has_cycle, not hidden);
          if (not hidden)
            {
              EXPECT_EQ(r.minimum_mean, 1.0L);
              EXPECT_EQ(arc_numbers(r.cycle_arcs), (std::set<int>{0, 1, 2}));
            }
          EXPECT_EQ(minimum_mean_cycle(g, Dft_Dist<GT>(), sa).has_cycle, not hidden);
          EXPECT_EQ(karp_minimum_mean_cycle_value(g, Dft_Dist<GT>(), sa).has_cycle, not hidden);
          EXPECT_EQ(minimum_mean_cycle_value(g, Dft_Dist<GT>(), sa).has_cycle, not hidden);

          const Karp_Minimum_Mean_Cycle<GT, Dft_Dist<GT>, SA> karp(Dft_Dist<GT>(), sa);
          EXPECT_EQ(karp(g).has_cycle, not hidden);
          const Karp_Minimum_Mean_Cycle_Value<GT, Dft_Dist<GT>, SA> value(Dft_Dist<GT>(), sa);
          EXPECT_EQ(value(g).has_cycle, not hidden);
        });
    });
}

TYPED_TEST(ContractPathsCycles, HowardMinimumMeanCycle)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> b(ring());
      SCOPED_TRACE(name);
      constexpr bool hidden = hides<GT, SA>;
      for_each_constness(b.g, [&] (auto & g)
        {
          const auto r = howard_minimum_mean_cycle(g, Dft_Dist<GT>(), sa);
          EXPECT_EQ(r.has_cycle, not hidden);
          if (not hidden)
            {
              EXPECT_EQ(r.minimum_mean, 1.0L);
              EXPECT_EQ(arc_numbers(r.cycle_arcs), (std::set<int>{0, 1, 2}));
            }
          EXPECT_EQ(howard_minimum_mean_cycle(g, Dft_Dist<GT>(), sa, size_t(1) << 20).has_cycle,
                    not hidden);
          EXPECT_EQ(howard_minimum_mean_cycle_value(g, Dft_Dist<GT>(), sa).has_cycle, not hidden);
          EXPECT_EQ(howard_minimum_mean_cycle_value(g, Dft_Dist<GT>(), sa, size_t(1) << 20).has_cycle,
                    not hidden);
        });
    });
}

TYPED_TEST(ContractPathsCycles, NegativeCycles)
{
  using GT = TypeParam;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      Built<GT> neg(negative());
      make_negative(neg);
      SCOPED_TRACE(name);
      constexpr bool hidden = hides<GT, SA>;
      for_each_constness(neg.g, [&] (auto & g)
        {
          const auto cycles = find_disjoint_negative_cycles(g, 2, Negative_Cycle_Exclusion::Min_Weight_Arc,
                                                            Dft_Dist<GT>(), sa);
          ASSERT_EQ(cycles.size(), hidden ? 0U : 1U);
          if (not hidden)
            {
              EXPECT_EQ(cycles.get_first().total_cost, -1);
            }
          EXPECT_EQ(most_negative_cycle_bounded(g, 3, Dft_Dist<GT>(), sa).is_negative(), not hidden);
          EXPECT_EQ(most_negative_cycle_up_to_3(g, 3, Dft_Dist<GT>(), sa).is_negative(), not hidden);
        });
    });
}
