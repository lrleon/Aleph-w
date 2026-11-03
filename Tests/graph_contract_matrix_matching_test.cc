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
 * @file graph_contract_matrix_matching_test.cc
 * @brief The contract matrix of the matchings: every template of
 *        tpl_bipartite.H, Blossom.H, Blossom_Weighted.H and
 *        Min_Cost_Matching.H with every kind of graph, arc filter and
 *        constness that it takes.
 *
 * The matchings take only the three kinds of graph that are not directed.
 * Each test instantiates one template with each of them and with each filter
 * of graph_filter_catalog.H and Dft_Show_Arc, and calls each of its entry
 * points once, on a graph as it is and as a const graph. The graphs are so
 * small that the answer is known, and the filters hide arc number 2, whose
 * absence changes it: a filter that is not applied shows up.
 *
 * The combinations that should compile and do not are kept, as known
 * failures that must not compile yet, in graph_contract_compile_check.cmake.
 */

#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_bipartite.H>
#include <Blossom.H>
#include <Blossom_Weighted.H>
#include <Min_Cost_Matching.H>

#include "graph_harness.H"
#include "graph_filter_catalog.H"
#include "graph_contract_matrix.H"

using namespace Aleph;
using namespace graph_harness;
using namespace graph_contract_matrix;

namespace
{
  /// Whether a filter can be built with SA(), as a default argument builds
  /// it; one of the catalog built so hides nothing
  template <class SA>
  constexpr bool default_built = std::is_default_constructible_v<SA>;


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
  /// The calls that a Counting filter has counted, or 0 for another filter
  template <class GT, class SA>
  size_t counted(const SA & sa)
  {
    if constexpr (std::is_same_v<SA, graph_filters::Counting<GT>>)
      return sa.calls == nullptr ? 0 : *sa.calls;
    else
      return 0;
  }

  /// Expects that a Counting filter given by value was called: its copies
  /// share the counter, so that a search that builds its own filter shows
  template <class GT, class SA>
  void expect_called(const SA & sa, size_t before)
  {
    if constexpr (std::is_same_v<SA, graph_filters::Counting<GT>>)
      {
        EXPECT_GT(counted<GT>(sa), before) << "the filter given is not the one used";
      }
  }

  /** The path 0 - 1 - 2 - 3, with arcs 0: 0-1, 1: 2-1 and 2: 2-3. Its only
      maximum matching is {0, 2}; without arc 2, the maximum matchings have
      one arc. The weights of Dft_Dist are the arc numbers. */
  Spec path_spec()
  {
    Spec s;
    s.n = 4;
    s.arcs = {{0, 1}, {2, 1}, {2, 3}};
    return s;
  }

  /** The cycle 0 - 1 - 2 - 3 - 0, with arc i between i and i + 1 mod 4,
      for the matching by flow. Its maximum matchings are {0, 2} and
      {1, 3}; without arc 2, only {1, 3}.

      Each arc goes from the side of the first node in the order of the
      nodes of g, the one that compute_bipartite() puts on the left: an
      order that follows the addresses of the nodes in List_SGraph. Without
      arc 2 the cycle is a path, and the sides stay. The arcs that go the
      other way are those of graph_exhaustive_matching_test.cc. */
  template <class GT>
  struct Oriented_Cycle
  {
    GT g;
    std::vector<typename GT::Node *> nodes;

    Oriented_Cycle()
    {
      for (int i = 0; i < 4; ++i)
        nodes.push_back(g.insert_node(i));
      const int left = typename GT::Node_Iterator(g).get_curr()->get_info() % 2;
      for (int i = 0; i < 4; ++i)
        {
          int u = i, v = (i + 1) % 4;
          if (u % 2 != left)
            std::swap(u, v);
          g.insert_arc(nodes[size_t(u)], nodes[size_t(v)], i);
        }
    }

    Oriented_Cycle(const Oriented_Cycle &) = delete;
    Oriented_Cycle & operator = (const Oriented_Cycle &) = delete;
  };

  /// Expects a maximum matching of the cycle: {0, 2} or {1, 3}, or {1, 3}
  /// without arc 2
  template <class List>
  void expect_cycle_matching(const List & m, bool hidden)
  {
    const auto arcs = arc_numbers(m);
    if (hidden)
      {
        EXPECT_EQ(arcs, (std::set<int>{1, 3}));
      }
    else
      {
        EXPECT_TRUE(arcs == (std::set<int>{0, 2}) or arcs == (std::set<int>{1, 3}))
          << m.size() << " arcs";
      }
  }

  /// The triangle 0 - 1 - 2 whose arc 2, 0-2, closes the odd cycle: it is
  /// bipartite only without it
  Spec triangle_spec()
  {
    Spec s;
    s.n = 3;
    s.arcs = {{0, 1}, {1, 2}, {0, 2}};
    return s;
  }

  /// The numbers of the nodes of a list
  template <class List>
  std::set<int> node_numbers(const List & l)
  {
    std::set<int> r;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      r.insert(it.get_curr()->get_info());
    return r;
  }

  /// Expects the bipartition of the path 0 - 1 - 2: {0, 2} and {1}, on
  /// either side, as the side of the first node follows the order of the
  /// nodes, that of their addresses in List_SGraph
  template <class List>
  void expect_path_bipartition(const List & l, const List & r)
  {
    const std::set<std::set<int>> sides = {node_numbers(l), node_numbers(r)};
    EXPECT_EQ(sides, (std::set<std::set<int>>{{0, 2}, {1}}));
  }

  /// Expects a maximum matching of the path: {0, 2}, or one arc without 2
  template <class List>
  void expect_maximum_matching(const List & m, bool hidden)
  {
    const auto arcs = arc_numbers(m);
    if (hidden)
      {
        EXPECT_EQ(m.size(), 1U);
        EXPECT_EQ(arcs.count(Hidden), 0U);
      }
    else
      {
        EXPECT_EQ(arcs, (std::set<int>{0, 2}));
      }
  }
} // namespace

// ---------------------------------------------------------------------
// tpl_bipartite.H: the filter by value, which a default argument builds
// ---------------------------------------------------------------------

template <class GT>
class ContractMatchingBipartite : public ::testing::Test {};

TYPED_TEST_SUITE(ContractMatchingBipartite, Undirected_Kinds);

TYPED_TEST(ContractMatchingBipartite, ComputeBipartite)
{
  using GT = TypeParam;
  Built<GT> b(triangle_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool h = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Node *> l, r, l2, r2;
          Compute_Bipartite<GT, SA> bipartite(sa);
          if constexpr (h)
            {
              const size_t before = counted<GT>(sa);
              compute_bipartite<GT, SA>(g, l, r, sa);
              expect_path_bipartition(l, r);
              expect_called<GT>(sa, before);
              bipartite(g, l2, r2);
              expect_path_bipartition(l2, r2);
            }
          else
            {
              EXPECT_THROW_OR_FILTER((compute_bipartite<GT, SA>(g, l, r, sa)), std::domain_error);
              EXPECT_THROW_OR_FILTER(bipartite(g, l2, r2), std::domain_error);
            }

          if constexpr (default_built<SA>) // a filter built by default hides nothing
            {
              DynDlist<typename GT::Node *> l3, r3;
              EXPECT_THROW_OR_FILTER((compute_bipartite<GT, SA>(g, l3, r3)), std::domain_error);
              EXPECT_THROW_OR_FILTER((Compute_Bipartite<GT, SA>()(g, l3, r3)), std::domain_error);
            }
        });
    });
}

TYPED_TEST(ContractMatchingBipartite, ComputeBipartiteAllComponents)
{
  using GT = TypeParam;
  Built<GT> b(triangle_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool h = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Node *> l, r;
          const size_t before = counted<GT>(sa);
          const bool bipartite = compute_bipartite_all_components<GT, SA>(g, l, r, sa);
          EXPECT_EQ(bipartite, h);
          if (bipartite)
            {
              expect_path_bipartition(l, r);
            }
          expect_called<GT>(sa, before);

          if constexpr (default_built<SA>)
            {
              DynDlist<typename GT::Node *> l2, r2;
              EXPECT_FALSE((compute_bipartite_all_components<GT, SA>(g, l2, r2)));
            }
        });
    });
}

TYPED_TEST(ContractMatchingBipartite, MaximumCardinalityBipartiteMatching)
{
  using GT = TypeParam;
  Oriented_Cycle<GT> b;

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool h = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m, m2;
          const size_t before = counted<GT>(sa);
          compute_maximum_cardinality_bipartite_matching
            <GT, Ford_Fulkerson_Maximum_Flow, SA>(g, m, sa);
          expect_cycle_matching(m, h);
          expect_called<GT>(sa, before);

          Compute_Maximum_Cardinality_Bipartite_Matching
            <GT, Ford_Fulkerson_Maximum_Flow, SA> matching(sa);
          matching(g, m2);
          expect_cycle_matching(m2, h);

          if constexpr (default_built<SA>)
            {
              DynDlist<typename GT::Arc *> m3;
              Compute_Maximum_Cardinality_Bipartite_Matching
                <GT, Ford_Fulkerson_Maximum_Flow, SA>()(g, m3);
              expect_cycle_matching(m3, false);
            }
        });
    });
}

TYPED_TEST(ContractMatchingBipartite, HopcroftKarp)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      constexpr bool h = hides<GT, SA>;
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m, m2;
          const size_t before = counted<GT>(sa);
          hopcroft_karp_matching<GT, SA>(g, m, sa);
          expect_maximum_matching(m, h);
          expect_called<GT>(sa, before);

          Hopcroft_Karp_Matching<GT, SA> matching(sa);
          matching(g, m2);
          expect_maximum_matching(m2, h);

          if constexpr (default_built<SA>)
            {
              DynDlist<typename GT::Arc *> m3;
              Hopcroft_Karp_Matching<GT, SA>()(g, m3);
              expect_maximum_matching(m3, false);
            }
        });
    });
}

namespace
{
  /// Hides arc Hidden, and counts how many times a filter of its type is
  /// built with its default constructor
  template <class GT>
  struct Counting_Defaults
  {
    static inline size_t built = 0;

    Counting_Defaults() noexcept { ++built; }

    bool operator () (typename GT::Arc * a) const noexcept
    {
      return a->get_info() != Hidden;
    }
  };
}

// The bipartition, the iterators and the frames of the search copy the
// filter given, and build none with SA(): no state of a filter reached the
// search when they did
TYPED_TEST(ContractMatchingBipartite, HopcroftKarpCopiesTheFilterGiven)
{
  using GT = TypeParam;
  using SA = Counting_Defaults<GT>;
  Built<GT> b(path_spec());

  SA sa;
  SA::built = 0;
  DynDlist<typename GT::Arc *> m;
  hopcroft_karp_matching<GT, SA>(b.g, m, sa);
  expect_maximum_matching(m, true);
  Hopcroft_Karp_Matching<GT, SA> matching(sa);
  DynDlist<typename GT::Arc *> m2;
  matching(b.g, m2);
  expect_maximum_matching(m2, true);
  EXPECT_EQ(SA::built, 0U);

  DynDlist<typename GT::Arc *> m3;
  hopcroft_karp_matching<GT, SA>(b.g, m3); // that of the default argument
  expect_maximum_matching(m3, true);
  EXPECT_EQ(SA::built, 1U);
}

// ---------------------------------------------------------------------
// Blossom.H: maximum cardinality, the filter by value
// ---------------------------------------------------------------------

template <class GT>
class ContractMatchingBlossom : public ::testing::Test {};

TYPED_TEST_SUITE(ContractMatchingBlossom, Undirected_Kinds);

TYPED_TEST(ContractMatchingBlossom, MaximumCardinalityGeneralMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m;
          const size_t before = counted<GT>(sa);
          const size_t n = compute_maximum_cardinality_general_matching(g, m, sa);
          EXPECT_EQ(n, m.size());
          expect_maximum_matching(m, hides<GT, SA>);
          expect_called<GT>(sa, before);
        });
    });
}

TYPED_TEST(ContractMatchingBlossom, BlossomMaximumCardinalityMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m;
          const size_t n = blossom_maximum_cardinality_matching(g, m, sa);
          EXPECT_EQ(n, m.size());
          expect_maximum_matching(m, hides<GT, SA>);
        });
    });
}

TYPED_TEST(ContractMatchingBlossom, ComputeMaximumCardinalityGeneralMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m;
          const size_t before = counted<GT>(sa);
          Compute_Maximum_Cardinality_General_Matching<GT, SA> matcher(sa);
          EXPECT_EQ(matcher(g, m), m.size());
          expect_maximum_matching(m, hides<GT, SA>);
          expect_called<GT>(sa, before);
        });
    });
}

// ---------------------------------------------------------------------
// Blossom_Weighted.H and Min_Cost_Matching.H: the weights are the arc
// numbers, 0, 1 and 2
// ---------------------------------------------------------------------

namespace
{
  /** Expects the maximum weight matching of the path: weight 2, by {2} or
      {0, 2}, or {0, 2} when the cardinality goes first; without arc 2,
      {1}, of weight 1, either way */
  template <class List, class Result>
  void expect_maximum_weight(const List & m, const Result & r, bool hidden,
                             bool max_cardinality)
  {
    EXPECT_EQ(r.cardinality, m.size());
    if (hidden)
      {
        EXPECT_EQ(r.total_weight, 1);
        EXPECT_EQ(arc_numbers(m), (std::set<int>{1}));
      }
    else
      {
        EXPECT_EQ(r.total_weight, 2);
        EXPECT_EQ(arc_numbers(m).count(Hidden), 1U);
        if (max_cardinality)
          {
            EXPECT_EQ(arc_numbers(m), (std::set<int>{0, 2}));
          }
      }
  }

  /** Expects the minimum cost matching of the path that has as many arcs
      as possible: {0, 2}, of cost 2; without arc 2, {0}, of cost 0 */
  template <class List, class Result>
  void expect_minimum_cost(const List & m, const Result & r, bool hidden)
  {
    EXPECT_EQ(r.cardinality, m.size());
    if (hidden)
      {
        EXPECT_EQ(r.total_cost, 0);
        EXPECT_EQ(arc_numbers(m), (std::set<int>{0}));
      }
    else
      {
        EXPECT_EQ(r.total_cost, 2);
        EXPECT_EQ(arc_numbers(m), (std::set<int>{0, 2}));
      }
  }

  /** Expects the minimum cost perfect matching of the path: {0, 2}, of
      cost 2; without arc 2, node 3 has no arc, and there is none */
  template <class List, class Result>
  void expect_perfect(const List & m, const Result & r, bool hidden)
  {
    if (hidden)
      {
        EXPECT_FALSE(r.feasible);
        EXPECT_TRUE(m.is_empty());
      }
    else
      {
        EXPECT_TRUE(r.feasible);
        EXPECT_EQ(r.total_cost, 2);
        EXPECT_EQ(arc_numbers(m), (std::set<int>{0, 2}));
      }
  }
} // namespace

template <class GT>
class ContractMatchingWeighted : public ::testing::Test {};

TYPED_TEST_SUITE(ContractMatchingWeighted, Undirected_Kinds);

TYPED_TEST(ContractMatchingWeighted, MaximumWeightGeneralMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          for (bool max_cardinality : {false, true})
            {
              DynDlist<typename GT::Arc *> m;
              const size_t before = counted<GT>(sa);
              const auto r = compute_maximum_weight_general_matching
                (g, m, Dft_Dist<GT>(), sa, max_cardinality);
              expect_maximum_weight(m, r, hides<GT, SA>, max_cardinality);
              expect_called<GT>(sa, before);
            }
        });
    });
}

TYPED_TEST(ContractMatchingWeighted, BlossomMaximumWeightMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          for (bool max_cardinality : {false, true})
            {
              DynDlist<typename GT::Arc *> m;
              const auto r = blossom_maximum_weight_matching
                (g, m, Dft_Dist<GT>(), sa, max_cardinality);
              expect_maximum_weight(m, r, hides<GT, SA>, max_cardinality);
            }
        });
    });
}

TYPED_TEST(ContractMatchingWeighted, ComputeMaximumWeightGeneralMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m;
          Compute_Maximum_Weight_General_Matching<GT, Dft_Dist<GT>, SA>
            matcher(Dft_Dist<GT>(), sa, true);
          const auto r = matcher(g, m);
          expect_maximum_weight(m, r, hides<GT, SA>, true);
        });
    });
}

TYPED_TEST(ContractMatchingWeighted, MinimumCostGeneralMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m, m2;
          const size_t before = counted<GT>(sa);
          const auto r = compute_minimum_cost_general_matching
            (g, m, Dft_Dist<GT>(), sa, true);
          expect_minimum_cost(m, r, hides<GT, SA>);
          expect_called<GT>(sa, before);

          // Without the cardinality first, a matching of cost 0 is enough
          const auto r2 = compute_minimum_cost_general_matching(g, m2, Dft_Dist<GT>(), sa);
          EXPECT_EQ(r2.total_cost, 0);
          EXPECT_EQ(arc_numbers(m2).count(Hidden), 0U);
        });
    });
}

TYPED_TEST(ContractMatchingWeighted, BlossomMinimumCostMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m;
          const auto r = blossom_minimum_cost_matching(g, m, Dft_Dist<GT>(), sa, true);
          expect_minimum_cost(m, r, hides<GT, SA>);
        });
    });
}

TYPED_TEST(ContractMatchingWeighted, MinimumCostPerfectGeneralMatching)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m, m2;
          const size_t before = counted<GT>(sa);
          const auto r = compute_minimum_cost_perfect_general_matching(g, m, Dft_Dist<GT>(), sa);
          expect_perfect(m, r, hides<GT, SA>);
          expect_called<GT>(sa, before);

          const auto r2 = blossom_minimum_cost_perfect_matching(g, m2, Dft_Dist<GT>(), sa);
          expect_perfect(m2, r2, hides<GT, SA>);
        });
    });
}

TYPED_TEST(ContractMatchingWeighted, MinimumCostMatchingClasses)
{
  using GT = TypeParam;
  Built<GT> b(path_spec());

  for_each_filter<GT>([&] (auto sa, const char * name)
    {
      using SA = decltype(sa);
      SCOPED_TRACE(name);
      for_each_constness(b.g, [&] (auto & g)
        {
          DynDlist<typename GT::Arc *> m, m2;
          Compute_Minimum_Cost_General_Matching<GT, Dft_Dist<GT>, SA>
            matcher(Dft_Dist<GT>(), sa, true);
          expect_minimum_cost(m, matcher(g, m), hides<GT, SA>);

          Compute_Minimum_Cost_Perfect_General_Matching<GT, Dft_Dist<GT>, SA>
            perfect(Dft_Dist<GT>(), sa);
          expect_perfect(m2, perfect(g, m2), hides<GT, SA>);
        });
    });
}
