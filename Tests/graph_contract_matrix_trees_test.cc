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
 * @file graph_contract_matrix_trees_test.cc
 * @brief The contract matrix of the tree algorithms: every template of
 *        LCA.H, HLD.H, Tree_Decomposition.H and Tree_DP.H with every kind
 *        of graph, arc filter and constness that it takes.
 *
 * The tree algorithms take the three kinds of graph that are not directed,
 * and a filter that cuts a tree out of a larger graph. Each test builds one
 * template with each of those kinds and each filter of
 * graph_filter_catalog.H and Dft_Show_Arc, on a graph as it is and as a
 * const graph, and calls each of its entry points once:
 *
 * - on a triangle with a tail, which is the path 0 - 1 - 2 - 3 only without
 *   arc 2: with a filter that hides it, the answers are those of the path,
 *   and with Dft_Show_Arc the construction rejects the cycle, so that a
 *   filter that is not applied shows up;
 * - on the path 0 - 1 - 2, a tree for every filter, since it has no arc 2.
 *
 * All the combinations compile. graph_contract_compile_check.cmake checks
 * that a predicate on nodes, which is not an arc filter, does not
 * (lca_node_filter).
 */

#include <algorithm>
#include <cstdlib>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <LCA.H>
#include <HLD.H>
#include <Tree_Decomposition.H>
#include <Tree_DP.H>

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
  /** A triangle closed by arc 2, with a tail: arcs 0: 0-1, 1: 1-2, 2: 0-2
      and 3: 2-3. Without arc 2 it is the path 0 - 1 - 2 - 3. */
  Spec cycle_spec()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {1, 2}, {0, 2}, {2, 3}};
    return s;
  }

  /// The path 0 - 1 - 2, a tree for every filter: it has no arc 2
  Spec tree_spec()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}};
    return s;
  }

  /// The calls that a Counting filter has counted, or 0 for another filter
  template <class GT, class SA>
  size_t counted(const SA & sa)
  {
    if constexpr (std::is_same_v<SA, graph_filters::Counting<GT>>)
      return sa.calls == nullptr ? 0 : *sa.calls;
    else
      return 0;
  }

  /** Builds a tree object with make(g, nodes, sa), with each filter and
      constness, and checks it:

      - on the graph of cycle_spec(), with path4(t, nodes) where the filter
        hides arc 2, and expecting domain_error, the rejection of the cycle,
        with Dft_Show_Arc; a Counting filter must count calls, as the
        object uses a copy of the filter given, which shares its counter;
      - on the graph of tree_spec(), with path3(t, nodes). */
  template <class GT, class Make, class Path4, class Path3>
  void on_trees(Make && make, Path4 && path4, Path3 && path3)
  {
    Built<GT> cycle(cycle_spec());
    Built<GT> tree(tree_spec());

    for_each_filter<GT>([&] (auto sa, const char * name)
      {
        using SA = decltype(sa);
        SCOPED_TRACE(name);
        for_each_constness(cycle.g, [&] (auto & g)
          {
            if constexpr (hides<GT, SA>)
              {
                const size_t before = counted<GT>(sa);
                path4(make(g, cycle.nodes, sa), cycle.nodes);
                if constexpr (std::is_same_v<SA, graph_filters::Counting<GT>>)
                  {
                    EXPECT_GT(counted<GT>(sa), before) << "the filter given is not the one used";
                  }
              }
            else
              {
                EXPECT_THROW_OR_FILTER(make(g, cycle.nodes, sa), std::domain_error);
              }
          });
        for_each_constness(tree.g, [&] (auto & g)
          {
            path3(make(g, tree.nodes, sa), tree.nodes);
          });
      });
  }

  /// The values of an array, as a sorted vector
  template <class A>
  std::vector<size_t> sorted(const A & a)
  {
    std::vector<size_t> r;
    for (size_t i = 0; i < a.size(); ++i)
      r.push_back(a(i));
    std::sort(r.begin(), r.end());
    return r;
  }

  /// The value of a node: its number
  template <class GT>
  int node_value(typename GT::Node * p)
  {
    return p->get_info();
  }

  /// Checks the queries that both LCA engines have, on the path 0 - 1 - 2 - 3
  /// rooted at 0
  template <class T, class Nodes>
  void expect_lca_on_path4(const T & t, const Nodes & n)
  {
    EXPECT_EQ(t.size(), 4U);
    EXPECT_FALSE(t.is_empty());
    EXPECT_EQ(t.root(), n[0]);
    EXPECT_EQ(t.node_of(t.root_id()), n[0]);
    EXPECT_EQ(t.depth_of(n[3]), 3U);
    EXPECT_EQ(t.depth_of_id(t.id_of(n[2])), 2U);
    EXPECT_EQ(t.parent_of(n[2]), n[1]);
    EXPECT_EQ(t.parent_id(t.id_of(n[1])), t.id_of(n[0]));
    EXPECT_TRUE(t.is_ancestor(n[1], n[3]));
    EXPECT_FALSE(t.is_ancestor_id(t.id_of(n[3]), t.id_of(n[1])));
    EXPECT_EQ(t.lca(n[1], n[3]), n[1]);
    EXPECT_EQ(t.lca_id(t.id_of(n[2]), t.id_of(n[3])), t.id_of(n[2]));
    EXPECT_EQ(t.distance(n[0], n[3]), 3U);
    EXPECT_EQ(t.distance_id(t.id_of(n[1]), t.id_of(n[3])), 2U);
  }
} // namespace

// ---------------------------------------------------------------------
// LCA.H
// ---------------------------------------------------------------------

template <class GT>
class ContractTreesLca : public ::testing::Test {};

TYPED_TEST_SUITE(ContractTreesLca, Undirected_Kinds);

TYPED_TEST(ContractTreesLca, BinaryLifting)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      // The root is the first node, which follows the addresses in
      // List_SGraph: only its size is known
      EXPECT_EQ((Binary_Lifting_LCA<GT, SA>(g, sa).size()), g.get_num_nodes());
      return Gen_Binary_Lifting_LCA<GT, SA>(g, n[0], sa);
    },
    [] (const auto & t, const auto & n)
    {
      expect_lca_on_path4(t, n);
      EXPECT_GE(t.num_levels(), 2U);
      EXPECT_EQ(t.kth_ancestor(n[3], 2), n[1]);
      EXPECT_EQ(t.kth_ancestor(n[3], 4), nullptr);
      EXPECT_EQ(t.kth_ancestor_id(t.id_of(n[2]), 1), t.id_of(n[1]));
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.distance(n[0], n[2]), 2U);
      EXPECT_EQ(t.lca(n[1], n[2]), n[1]);
    });
}

TYPED_TEST(ContractTreesLca, EulerRmq)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((Euler_RMQ_LCA<GT, SA>(g, sa).size()), g.get_num_nodes());
      return Gen_Euler_RMQ_LCA<GT, SA>(g, n[0], sa);
    },
    [] (const auto & t, const auto & n)
    {
      expect_lca_on_path4(t, n);
      EXPECT_EQ(t.euler_tour_size(), 7U);
      EXPECT_EQ(t.euler_tour().size(), 7U);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.distance(n[0], n[2]), 2U);
      EXPECT_EQ(t.lca(n[1], n[2]), n[1]);
    });
}

// ---------------------------------------------------------------------
// HLD.H: the values of the nodes are their numbers
// ---------------------------------------------------------------------

template <class GT>
class ContractTreesHld : public ::testing::Test {};

TYPED_TEST_SUITE(ContractTreesHld, Undirected_Kinds);

TYPED_TEST(ContractTreesHld, GenHld)
{
  using GT = TypeParam;
  using Sum = Aleph::plus<int>;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((Gen_HLD<GT, int, Sum, SA>(g, node_value<GT>, 0, Sum(), sa).size()),
                g.get_num_nodes());
      return Gen_HLD<GT, int, Sum, SA>(g, n[0], node_value<GT>, 0, Sum(), sa);
    },
    [] (auto && t, const auto & n)
    {
      EXPECT_EQ(t.size(), 4U);
      EXPECT_FALSE(t.is_empty());
      EXPECT_EQ(t.root(), n[0]);
      EXPECT_EQ(t.node_of(t.root_id()), n[0]);
      EXPECT_EQ(t.position(t.id_of(n[0])), 0U);
      EXPECT_EQ(t.chain_head_id(t.id_of(n[3])), t.id_of(n[0]));
      EXPECT_EQ(t.num_chains(), 1U);
      EXPECT_EQ(t.depth_of(n[3]), 3U);
      EXPECT_EQ(t.depth_of_id(t.id_of(n[1])), 1U);
      EXPECT_EQ(t.subtree_size_of(n[1]), 3U);
      EXPECT_EQ(t.subtree_size_of_id(t.id_of(n[3])), 1U);
      EXPECT_EQ(t.parent_of(n[3]), n[2]);
      EXPECT_EQ(t.parent_id(t.id_of(n[1])), t.id_of(n[0]));
      EXPECT_EQ(t.path_query(n[0], n[3]), 6);
      EXPECT_EQ(t.path_query_id(t.id_of(n[1]), t.id_of(n[2])), 3);
      EXPECT_EQ(t.path_query_edges(n[1], n[3]), 5);
      EXPECT_EQ(t.path_query_edges_id(t.id_of(n[0]), t.id_of(n[3])), 6);
      EXPECT_EQ(t.subtree_query(n[2]), 5);
      EXPECT_EQ(t.subtree_query_id(t.id_of(n[0])), 6);
      EXPECT_EQ(t.lca(n[1], n[3]), n[1]);
      EXPECT_EQ(t.lca_id(t.id_of(n[2]), t.id_of(n[3])), t.id_of(n[2]));
      EXPECT_EQ(t.distance(n[0], n[3]), 3U);
      EXPECT_EQ(t.distance_id(t.id_of(n[1]), t.id_of(n[3])), 2U);
      EXPECT_EQ(t.position_array().size(), 4U);
      EXPECT_EQ(t.chain_head_array().size(), 4U);
      EXPECT_EQ(t.depth_array().size(), 4U);
      EXPECT_EQ(t.subtree_size_array().size(), 4U);
      EXPECT_EQ(t.parent_array().size(), 4U);
      EXPECT_EQ(t.get_value(n[2]), 2);
      EXPECT_EQ(t.get_value_at_id(t.id_of(n[3])), 3);
      t.point_update(n[3], 10);
      EXPECT_EQ(t.path_query(n[0], n[3]), 13);
      t.point_update_id(t.id_of(n[0]), 5);
      EXPECT_EQ(t.path_query(n[0], n[3]), 18);
    },
    [] (auto && t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[0], n[2]), 3);
      EXPECT_EQ(t.distance(n[0], n[2]), 2U);
    });
}

TYPED_TEST(ContractTreesHld, SumMaxAndMin)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((HLD_Sum<GT, int, SA>(g, node_value<GT>, sa).size()), g.get_num_nodes());
      return HLD_Sum<GT, int, SA>(g, n[0], node_value<GT>, sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[0], n[3]), 6);
      EXPECT_EQ(t.subtree_query(n[2]), 5);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[0], n[2]), 3);
    });

  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((HLD_Max<GT, int, SA>(g, node_value<GT>, sa).size()), g.get_num_nodes());
      return HLD_Max<GT, int, SA>(g, n[0], node_value<GT>, sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[0], n[3]), 3);
      EXPECT_EQ(t.subtree_query(n[2]), 3);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[0], n[2]), 2);
    });

  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((HLD_Min<GT, int, SA>(g, node_value<GT>, sa).size()), g.get_num_nodes());
      return HLD_Min<GT, int, SA>(g, n[0], node_value<GT>, sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[1], n[3]), 1);
      EXPECT_EQ(t.subtree_query(n[2]), 2);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.path_query(n[1], n[2]), 1);
    });
}

// ---------------------------------------------------------------------
// Tree_Decomposition.H
// ---------------------------------------------------------------------

template <class GT>
class ContractTreesDecomposition : public ::testing::Test {};

TYPED_TEST_SUITE(ContractTreesDecomposition, Undirected_Kinds);

TYPED_TEST(ContractTreesDecomposition, HeavyLight)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((Heavy_Light_Decomposition<GT, SA>(g, sa).size()), g.get_num_nodes());
      return Gen_Heavy_Light_Decomposition<GT, SA>(g, n[0], sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.size(), 4U);
      EXPECT_FALSE(t.is_empty());
      EXPECT_EQ(t.root(), n[0]);
      EXPECT_EQ(t.node_of(t.root_id()), n[0]);
      EXPECT_EQ(t.parent_of(n[3]), n[2]);
      EXPECT_EQ(t.parent_id(t.id_of(n[1])), t.id_of(n[0]));
      EXPECT_EQ(t.depth_of(n[3]), 3U);
      EXPECT_EQ(t.depth_of_id(t.id_of(n[1])), 1U);
      EXPECT_EQ(t.subtree_size_of(n[1]), 3U);
      EXPECT_EQ(t.subtree_size_of_id(t.id_of(n[3])), 1U);
      EXPECT_EQ(t.heavy_child(n[0]), n[1]);
      EXPECT_EQ(t.heavy_child(n[3]), nullptr);
      EXPECT_EQ(t.heavy_child_id(t.id_of(n[1])), t.id_of(n[2]));
      EXPECT_EQ(t.head_of(n[3]), n[0]);
      EXPECT_EQ(t.head_id(t.id_of(n[2])), t.id_of(n[0]));
      EXPECT_EQ(t.position_of(n[0]), 0U);
      EXPECT_EQ(t.position_of_id(t.id_of(n[3])), 3U);
      const auto r = t.subtree_range(n[2]);
      EXPECT_EQ(r.left, 2U);
      EXPECT_EQ(r.right, 3U);
      EXPECT_EQ(t.subtree_range_id(t.id_of(n[0])).right, 3U);
      EXPECT_TRUE(t.is_ancestor(n[1], n[3]));
      EXPECT_FALSE(t.is_ancestor_id(t.id_of(n[3]), t.id_of(n[1])));
      EXPECT_EQ(t.lca(n[1], n[3]), n[1]);
      EXPECT_EQ(t.lca_id(t.id_of(n[2]), t.id_of(n[3])), t.id_of(n[2]));
      EXPECT_EQ(t.distance(n[0], n[3]), 3U);
      EXPECT_EQ(t.distance_id(t.id_of(n[1]), t.id_of(n[3])), 2U);
      // The path is one heavy chain, so that each query has one segment
      size_t segments = 0;
      t.for_each_path_segment(n[0], n[3], [&] (size_t, size_t, bool) { ++segments; });
      t.for_each_path_segment_id(t.id_of(n[3]), t.id_of(n[1]),
                                 [&] (size_t, size_t, bool) { ++segments; });
      t.for_each_path_segment_undirected_id(t.id_of(n[1]), t.id_of(n[2]),
                                            [&] (size_t, size_t) { ++segments; });
      EXPECT_EQ(segments, 3U);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.distance(n[0], n[2]), 2U);
      EXPECT_EQ(t.lca(n[1], n[2]), n[1]);
    });
}

TYPED_TEST(ContractTreesDecomposition, PathQuery)
{
  using GT = TypeParam;
  using Sum = Aleph::plus<int>;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      using Q = Gen_HLD_Path_Query<GT, int, Sum, SA>;
      EXPECT_EQ(Q(g, 0, Sum(), sa).size(), g.get_num_nodes());
      EXPECT_EQ(Q(g, node_value<GT>, 0, Sum(), sa).size(), g.get_num_nodes());
      EXPECT_EQ((HLD_Path_Query<GT, int, Sum, SA>(g, n[0], node_value<GT>, 0, Sum(), sa)
                   .query_subtree(n[0])),
                int(g.get_num_nodes() * (g.get_num_nodes() - 1) / 2));
      return Q(g, n[0], 0, Sum(), sa);
    },
    [] (auto && t, const auto & n)
    {
      EXPECT_EQ(t.size(), 4U);
      EXPECT_FALSE(t.is_empty());
      EXPECT_EQ(t.decomposition().size(), 4U);
      EXPECT_EQ(t.query_path(n[0], n[3]), 6);
      EXPECT_EQ(t.query_path_id(t.decomposition().id_of(n[1]),
                                t.decomposition().id_of(n[2])), 3);
      EXPECT_EQ(t.query_subtree(n[2]), 5);
      EXPECT_EQ(t.query_subtree_id(t.decomposition().id_of(n[0])), 6);
      EXPECT_EQ(t.get_node(n[3]), 3);
      t.update_node(n[3], 1);
      EXPECT_EQ(t.get_node(n[3]), 4);
      t.update_node_id(t.decomposition().id_of(n[0]), 1);
      EXPECT_EQ(t.get_node_id(t.decomposition().id_of(n[0])), 1);
      t.set_node(n[3], 0);
      t.set_node_id(t.decomposition().id_of(n[0]), 0);
      EXPECT_EQ(t.query_path(n[0], n[3]), 3);
    },
    [] (auto && t, const auto & n)
    {
      EXPECT_EQ(t.query_path(n[0], n[2]), 3);
    });
}

TYPED_TEST(ContractTreesDecomposition, Centroid)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((Centroid_Decomposition<GT, SA>(g, sa).size()), g.get_num_nodes());
      return Gen_Centroid_Decomposition<GT, SA>(g, n[0], sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.size(), 4U);
      EXPECT_FALSE(t.is_empty());
      EXPECT_EQ(t.root(), n[0]);
      EXPECT_EQ(t.node_of(t.root_id()), n[0]);
      // The centroids of the path 0 - 1 - 2 - 3 are 1 and 2
      auto * c = t.centroid_root();
      ASSERT_TRUE(c == n[1] or c == n[2]);
      const size_t cid = t.centroid_root_id();
      EXPECT_EQ(t.id_of(c), cid);
      EXPECT_EQ(t.centroid_parent(c), nullptr);
      EXPECT_EQ(t.centroid_level_of(c), 0U);
      EXPECT_EQ(t.centroid_level_of_id(cid), 0U);
      EXPECT_GE(t.max_centroid_level(), 1U);
      for (size_t i = 0; i < 4; ++i)
        {
          auto * u = n[i];
          const size_t uid = t.id_of(u);
          const auto & chain = t.centroid_ancestors_of(u);
          const auto & dists = t.centroid_distances_of(u);
          ASSERT_GE(chain.size(), 1U);
          EXPECT_EQ(chain(0), cid);
          EXPECT_EQ(chain(chain.size() - 1), uid);
          EXPECT_EQ(dists(dists.size() - 1), 0U);
          EXPECT_EQ(t.centroid_path_length_of(u), chain.size());
          EXPECT_EQ(t.centroid_path_length_of_id(uid), chain.size());
          EXPECT_EQ(t.centroid_ancestors_of_id(uid).size(), chain.size());
          EXPECT_EQ(t.centroid_distances_of_id(uid).size(), chain.size());
          EXPECT_TRUE(t.is_centroid_ancestor_id(cid, uid));
          const int d = std::abs(int(i) - c->get_info());
          EXPECT_EQ(t.distance_to_centroid_id(uid, cid), size_t(d));
          size_t visits = 0;
          t.for_each_centroid_ancestor(u, [&] (size_t, size_t, size_t) { ++visits; });
          t.for_each_centroid_ancestor_id(uid, [&] (size_t, size_t, size_t) { ++visits; });
          EXPECT_EQ(visits, 2 * chain.size());
          if (u != c)
            {
              EXPECT_NE(t.centroid_parent(u), nullptr);
              EXPECT_NE(t.centroid_parent_id(uid), uid);
            }
        }
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.centroid_root(), n[1]);
    });
}

// ---------------------------------------------------------------------
// Tree_DP.H
// ---------------------------------------------------------------------

template <class GT>
class ContractTreesDp : public ::testing::Test {};

TYPED_TEST_SUITE(ContractTreesDp, Undirected_Kinds);

namespace
{
  /// Each node counts 1
  template <class GT>
  size_t one(typename GT::Node *) { return 1; }

  /// A subtree adds the sizes of its children
  template <class GT>
  size_t add_child(typename GT::Node *, const size_t & acc, typename GT::Node *,
                   const size_t & child)
  {
    return acc + child;
  }

  /// The rerooting of the eccentricities: 0 at a node, a maximum, plus one
  /// per edge
  template <class GT>
  size_t zero(typename GT::Node *) { return 0; }

  size_t larger(const size_t & a, const size_t & b) { return std::max(a, b); }

  template <class GT>
  size_t one_more(typename GT::Node *, typename GT::Node *, const size_t & v) { return v + 1; }
} // namespace

TYPED_TEST(ContractTreesDp, SubtreeSizes)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((Tree_DP<GT, size_t, SA>(g, n[0], one<GT>, add_child<GT>, sa).size()),
                g.get_num_nodes());
      return Gen_Tree_DP<GT, size_t, SA>(g, n[0], one<GT>, add_child<GT>, sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.size(), 4U);
      EXPECT_EQ(t.value(n[0]), 4U);
      EXPECT_EQ(t.value(n[3]), 1U);
      EXPECT_EQ(t.values().size(), 4U);
      EXPECT_EQ(t.node_of(t.id_of(n[2])), n[2]);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.value(n[0]), 3U);
    });
}

TYPED_TEST(ContractTreesDp, Rerooting)
{
  using GT = TypeParam;
  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      using SA = decltype(sa);
      EXPECT_EQ((Reroot_DP<GT, size_t, SA>(g, n[0], size_t(0), zero<GT>, larger,
                                           one_more<GT>, sa).size()),
                g.get_num_nodes());
      return Gen_Reroot_DP<GT, size_t, SA>(g, n[0], size_t(0), zero<GT>, larger,
                                           one_more<GT>, sa);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.size(), 4U);
      EXPECT_EQ(t.value(n[0]), 3U);
      EXPECT_EQ(t.value(n[1]), 2U);
      EXPECT_EQ(t.value(n[2]), 2U);
      EXPECT_EQ(t.value(n[3]), 3U);
      EXPECT_EQ(t.values().size(), 4U);
      EXPECT_EQ(t.node_of(t.id_of(n[2])), n[2]);
    },
    [] (const auto & t, const auto & n)
    {
      EXPECT_EQ(t.value(n[1]), 1U);
    });
}

TYPED_TEST(ContractTreesDp, Functions)
{
  using GT = TypeParam;
  using V = std::vector<size_t>;

  // An object that holds the three answers, so that on_trees() checks them
  struct Answers
  {
    Array<size_t> sizes, eccentricities, sums;
  };

  on_trees<GT>([] (auto & g, const auto & n, auto sa)
    {
      return Answers{tree_subtree_sizes(g, n[0], sa), tree_max_distance(g, n[0], sa),
                     tree_sum_of_distances(g, n[0], sa)};
    },
    [] (const Answers & a, const auto &)
    {
      EXPECT_EQ(sorted(a.sizes), (V{1, 2, 3, 4}));
      EXPECT_EQ(sorted(a.eccentricities), (V{2, 2, 3, 3}));
      EXPECT_EQ(sorted(a.sums), (V{4, 4, 6, 6}));
    },
    [] (const Answers & a, const auto &)
    {
      EXPECT_EQ(sorted(a.sizes), (V{1, 2, 3}));
      EXPECT_EQ(sorted(a.eccentricities), (V{1, 2, 2}));
      EXPECT_EQ(sorted(a.sums), (V{2, 3, 3}));
    });
}
