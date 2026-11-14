
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
 * @file bounded_cycle_index_test.cc
 * @brief Tests of Bounded_Cycle_Index: the catalog against a search made
 *        apart, the numbers of the arcs, the weights, the filter and its
 *        margin, the changes of several arcs, the limits and the failures.
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <cmath>
# include <cstdint>
# include <functional>
# include <limits>
# include <map>
# include <random>
# include <set>
# include <stdexcept>
# include <type_traits>
# include <utility>
# include <vector>

# include <gmpxx.h>

# include <Bounded_Cycle_Index.H>
# include <tpl_agraph.H>
# include <tpl_const_graph_view.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
  using List_Type = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Sparse_Type = List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>;
  using Array_Type = Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>;

  using Ends = std::vector<std::pair<size_t, size_t>>;
  using Cycle = std::vector<size_t>;

  const double Infinity = std::numeric_limits<double>::infinity();

  // A digraph of n nodes with an arc for each pair of ends, whose
  // information is its place in `ends`; the arcs are inserted in that
  // order or in the reverse one
  template <class GT>
  struct Built
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    std::vector<typename GT::Arc *> arcs; // by its place in `ends`

    Built(const size_t n, const Ends & ends, const bool reversed = false)
      : arcs(ends.size())
    {
      for (size_t i = 0; i < n; ++i)
        nodes.push_back(g.insert_node(int(i)));
      for (size_t i = 0; i < ends.size(); ++i)
        {
          const size_t id = reversed ? ends.size() - 1 - i : i;
          arcs[id] = g.insert_arc(nodes[ends[id].first], nodes[ends[id].second], int(id));
        }
    }
  };

  // The order of the information of the arcs
  struct By_Info
  {
    template <class Arc>
    bool operator () (const Arc *a, const Arc *b) const
    {
      return a->get_info() < b->get_info();
    }
  };

  // The catalog of a digraph, found apart from the index: the arcs are
  // numbered with a stable sort, and the cycles come from walking the
  // simple paths from each node, each one rotated to its least arc
  template <class GT>
  struct Oracle
  {
    using Arc = typename GT::Arc;

    std::vector<Arc *> arcs;                 // by number
    std::map<const void *, size_t> number;   // of each arc
    std::vector<Cycle> cycles;               // by length, then lexicographically
    std::vector<std::vector<size_t>> of_arc; // by arc, the positions of its cycles

    template <class Less>
    Oracle(const GT & g, const std::set<size_t> & lengths, Less less)
    {
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        arcs.push_back(it.get_curr_ne());
      std::stable_sort(arcs.begin(), arcs.end(), less);
      std::vector<const void *> source(arcs.size()), target(arcs.size());
      std::map<const void *, std::vector<size_t>> out;
      for (size_t a = 0; a < arcs.size(); ++a)
        {
          number[arcs[a]] = a;
          source[a] = g.get_src_node(arcs[a]);
          target[a] = g.get_tgt_node(arcs[a]);
          out[source[a]].push_back(a);
        }

      const size_t longest = lengths.empty() ? 0 : *lengths.rbegin();
      std::set<Cycle> found;
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          const void *start = it.get_curr_ne();
          Cycle path;
          std::set<const void *> on_path = {start};
          const std::function<void(const void *)> walk = [&] (const void *node)
            {
              for (const size_t a : out[node])
                if (target[a] == start)
                  {
                    if (lengths.count(path.size() + 1) == 0)
                      continue;
                    Cycle cycle = path;
                    cycle.push_back(a);
                    std::rotate(cycle.begin(), std::min_element(cycle.begin(), cycle.end()),
                                cycle.end());
                    found.insert(cycle);
                  }
                else if (path.size() + 2 <= longest and on_path.count(target[a]) == 0)
                  {
                    path.push_back(a);
                    on_path.insert(target[a]);
                    walk(target[a]);
                    on_path.erase(target[a]);
                    path.pop_back();
                  }
            };
          walk(start);
        }
      cycles.assign(found.begin(), found.end());
      std::sort(cycles.begin(), cycles.end(), [] (const Cycle & a, const Cycle & b)
        {
          return a.size() != b.size() ? a.size() < b.size() : a < b;
        });
      of_arc.resize(arcs.size());
      for (size_t position = 0; position < cycles.size(); ++position)
        for (const size_t a : cycles[position])
          of_arc[a].push_back(position);
    }
  };

  template <class Span>
  std::vector<size_t> as_vector(const Span & items)
  {
    return std::vector<size_t>(items.begin(), items.end());
  }

  // The index has the numbers, the cycles and the cycles of each arc of
  // the oracle
  template <class GT>
  void expect_the_catalog(const Bounded_Cycle_Index<GT> & index, const Oracle<GT> & oracle)
  {
    ASSERT_EQ(index.num_arcs(), oracle.arcs.size());
    for (size_t a = 0; a < oracle.arcs.size(); ++a)
      {
        ASSERT_EQ(index.arc(a), oracle.arcs[a]) << "arc " << a;
        ASSERT_EQ(index.arc_number(oracle.arcs[a]), a);
      }
    ASSERT_EQ(index.num_cycles(), oracle.cycles.size());
    size_t by_length[Bounded_Cycle_Index<GT>::Max_Length + 2] = {};
    for (size_t position = 0; position < oracle.cycles.size(); ++position)
      {
        ASSERT_EQ(as_vector(index.cycle_arcs(position)), oracle.cycles[position])
          << "position " << position;
        ASSERT_EQ(index.length(position), oracle.cycles[position].size());
        ++by_length[oracle.cycles[position].size()];
      }
    size_t up_to = 0;
    for (size_t k = 1; k <= Bounded_Cycle_Index<GT>::Max_Length; ++k)
      {
        EXPECT_EQ(index.num_cycles(k), by_length[k]) << "length " << k;
        up_to += by_length[k];
        EXPECT_EQ(index.num_cycles_up_to(k), up_to) << "length " << k;
      }
    for (size_t a = 0; a < oracle.arcs.size(); ++a)
      ASSERT_EQ(as_vector(index.cycles_of(a)), oracle.of_arc[a]) << "arc " << a;
  }

  // What a query hands: the positions with their sums, in the order of
  // the calls
  using Handed = std::vector<std::pair<size_t, double>>;

  // The cycles of an arc whose length is at most max_length and whose
  // sum is below the bound, from the catalog and weight_sum()
  template <class GT>
  Handed below_of(const Bounded_Cycle_Index<GT> & index, const size_t arc,
                  const double bound, const size_t max_length)
  {
    Handed ret;
    for (const size_t position : index.cycles_of(arc))
      if (index.length(position) <= max_length and index.weight_sum(position) < bound)
        ret.emplace_back(position, index.weight_sum(position));
    return ret;
  }

  // Random ends of m arcs among n nodes: loops and parallel arcs come out
  Ends random_ends(std::mt19937 & rng, const size_t n, const size_t m)
  {
    std::uniform_int_distribution<size_t> node(0, n - 1);
    Ends ends;
    for (size_t i = 0; i < m; ++i)
      {
        const size_t source = node(rng); // the order of the two draws is fixed
        ends.emplace_back(source, node(rng));
      }
    return ends;
  }

  // The complete digraph of n nodes
  // All that an index answers, to compare two of them: its arcs with
  // their weights, their stamps and their cycles, its cycles with their
  // sums, its counts and its clock, and what a query of all its arcs hands
  template <class GT>
  struct Answers
  {
    std::vector<const void *> arcs;
    std::vector<double> weights;
    std::vector<uint64_t> stamps;
    std::vector<std::vector<size_t>> of_arc;
    std::vector<Cycle> cycles;
    std::vector<double> sums;
    std::vector<uint64_t> counts;
    Handed handed;
    size_t seen = 0;

    explicit Answers(Bounded_Cycle_Index<GT> & index)
    {
      Array<size_t> all;
      for (size_t a = 0; a < index.num_arcs(); ++a)
        {
          arcs.push_back(index.arc(a));
          weights.push_back(index.weight(a));
          stamps.push_back(index.stamp(a));
          of_arc.push_back(as_vector(index.cycles_of(a)));
          all.append(a);
        }
      for (size_t position = 0; position < index.num_cycles(); ++position)
        {
          cycles.push_back(as_vector(index.cycle_arcs(position)));
          sums.push_back(index.weight_sum(position));
        }
      for (size_t k = 0; k <= Bounded_Cycle_Index<GT>::Max_Length + 1; ++k)
        {
          counts.push_back(index.num_cycles(k));
          counts.push_back(index.num_cycles_up_to(k));
          counts.push_back(index.has_length(k));
        }
      counts.push_back(index.longest_length());
      counts.push_back(index.clock());
      seen = index.for_each_cycle_below(all, Infinity, Bounded_Cycle_Index<GT>::Max_Length,
                                        [this] (size_t p, double s) { handed.emplace_back(p, s); });
    }

    bool operator == (const Answers &) const = default;
  };

  // The index has no arcs and no cycles, and answers so to every query
  template <class GT>
  void expect_no_arcs_and_no_cycles(Bounded_Cycle_Index<GT> & index,
                                    const typename GT::Arc *arc)
  {
    EXPECT_EQ(index.num_arcs(), 0u);
    EXPECT_EQ(index.num_cycles(), 0u);
    for (size_t k = 0; k <= Bounded_Cycle_Index<GT>::Max_Length + 1; ++k)
      {
        EXPECT_EQ(index.num_cycles(k), 0u) << "length " << k;
        EXPECT_EQ(index.num_cycles_up_to(k), 0u) << "length " << k;
        EXPECT_FALSE(index.has_length(k)) << "length " << k;
      }
    EXPECT_EQ(index.longest_length(), 0u);
    EXPECT_EQ(index.clock(), 0u);
    EXPECT_THROW((void) index.arc_number(arc), std::domain_error);
    EXPECT_THROW((void) index.arc(0), std::out_of_range);
    EXPECT_THROW((void) index.weight(0), std::out_of_range);
    EXPECT_THROW((void) index.stamp(0), std::out_of_range);
    EXPECT_THROW((void) index.cycles_of(0), std::out_of_range);
    EXPECT_THROW((void) index.cycle_arcs(0), std::out_of_range);
    EXPECT_THROW((void) index.weight_sum(0), std::out_of_range);
    EXPECT_THROW((void) index.length(0), std::out_of_range);
    EXPECT_THROW((void) index.is_stale(0, 0), std::out_of_range);
    EXPECT_THROW(index.set_weight(0, 1), std::out_of_range);
    EXPECT_THROW(index.touch(0), std::out_of_range);
    size_t handed = 0;
    const auto count = [&handed] (size_t, double) { ++handed; };
    EXPECT_THROW(index.update(0, 1, Infinity, 8, count), std::out_of_range);
    EXPECT_THROW(index.for_each_cycle_below({0}, Infinity, 8, count), std::out_of_range);
    EXPECT_EQ(index.for_each_cycle_below(Array<size_t>(), Infinity, 8, count), 0u);
    EXPECT_EQ(index.for_each_cycle_below(Infinity, 8, count), 0u);
    EXPECT_EQ(handed, 0u);
  }

  Ends complete_ends(const size_t n)
  {
    Ends ends;
    for (size_t i = 0; i < n; ++i)
      for (size_t j = 0; j < n; ++j)
        if (i != j)
          ends.emplace_back(i, j);
    return ends;
  }

  size_t falling(const size_t n, const size_t k) // n (n - 1) ... (n - k + 1)
  {
    size_t ret = 1;
    for (size_t i = 0; i < k; ++i)
      ret *= n - i;
    return ret;
  }

  template <class GT>
  class BoundedCycleIndexOn : public ::testing::Test {};

  using Digraph_Types = ::testing::Types<List_Type, Sparse_Type, Array_Type>;
  TYPED_TEST_SUITE(BoundedCycleIndexOn, Digraph_Types);
}

// The catalog of random multigraphs, with loops and parallel arcs, is that
// of a search made apart, for several sets of lengths and orders of the
// arcs, on each type of digraph and through a view
TYPED_TEST(BoundedCycleIndexOn, HasTheCatalogOfASearchMadeApart)
{
  using GT = TypeParam;
  using View = Const_Graph_View<GT>;
  const std::vector<std::set<size_t>> sets =
    {{3}, {1, 2, 3}, {2, 3, 4}, {3, 4, 5, 6}, {2, 4}, {1, 2, 3, 4, 5, 6, 7, 8}, {8}, {1}};
  std::mt19937 rng(20261113);
  size_t compared = 0, with_loops = 0, with_parallel = 0;
  for (size_t round = 0; round < 600; ++round)
    {
      const size_t n = 1 + rng() % 7;
      const Ends ends = random_ends(rng, n, rng() % 26);
      const Built<GT> built(n, ends, round % 2 == 1);
      const auto & lengths = sets[round % sets.size()];
      const Array<size_t> array_of_lengths(lengths.begin(), lengths.end());
      const auto by_info_down = [] (const typename GT::Arc *a, const typename GT::Arc *b)
        {
          return a->get_info() > b->get_info();
        };
      const auto by_parity = [] (const typename GT::Arc *a, const typename GT::Arc *b)
        {
          return a->get_info() % 2 < b->get_info() % 2; // many arcs are not told apart
        };

      const Bounded_Cycle_Index<GT> in_list_order(built.g, array_of_lengths);
      expect_the_catalog(in_list_order, Oracle<GT>(built.g, lengths, Arc_List_Order()));
      const Bounded_Cycle_Index<GT> by_info(built.g, array_of_lengths, By_Info());
      expect_the_catalog(by_info, Oracle<GT>(built.g, lengths, By_Info()));
      const Bounded_Cycle_Index<GT> down(built.g, array_of_lengths, by_info_down);
      expect_the_catalog(down, Oracle<GT>(built.g, lengths, by_info_down));
      const Bounded_Cycle_Index<GT> parity(built.g, array_of_lengths, by_parity);
      expect_the_catalog(parity, Oracle<GT>(built.g, lengths, by_parity));

      const View view(built.g);
      const Bounded_Cycle_Index<View> through_view(view, array_of_lengths, By_Info());
      expect_the_catalog(through_view, Oracle<View>(view, lengths, By_Info()));
      ASSERT_EQ(through_view.num_cycles(), by_info.num_cycles());
      for (size_t position = 0; position < by_info.num_cycles(); ++position)
        ASSERT_EQ(as_vector(through_view.cycle_arcs(position)),
                  as_vector(by_info.cycle_arcs(position)));

      compared += by_info.num_cycles();
      std::set<std::pair<size_t, size_t>> seen;
      bool loop = false, parallel = false;
      for (const auto & e : ends)
        {
          loop = loop or e.first == e.second;
          parallel = parallel or not seen.insert(e).second;
        }
      with_loops += loop;
      with_parallel += parallel;
      if (::testing::Test::HasFatalFailure())
        return;
    }
  EXPECT_GT(compared, 3000u); // the rounds do have cycles, loops and parallel arcs
  EXPECT_GT(with_loops, 300u);
  EXPECT_GT(with_parallel, 300u);
}

// With an order of the arcs that tells them all apart, the catalog does
// not depend on the order in which the arcs were inserted
TYPED_TEST(BoundedCycleIndexOn, DoesNotDependOnTheOrderOfInsertion)
{
  using GT = TypeParam;
  std::mt19937 rng(7);
  for (size_t round = 0; round < 40; ++round)
    {
      const size_t n = 2 + rng() % 5;
      const Ends ends = random_ends(rng, n, 4 + rng() % 12);
      const Built<GT> forward(n, ends), backward(n, ends, true);
      const Bounded_Cycle_Index<GT> a(forward.g, {1, 2, 3, 4, 5}, By_Info());
      const Bounded_Cycle_Index<GT> b(backward.g, {1, 2, 3, 4, 5}, By_Info());
      ASSERT_EQ(a.num_cycles(), b.num_cycles());
      for (size_t arc = 0; arc < a.num_arcs(); ++arc)
        ASSERT_EQ(a.arc(arc)->get_info(), b.arc(arc)->get_info());
      for (size_t position = 0; position < a.num_cycles(); ++position)
        ASSERT_EQ(as_vector(a.cycle_arcs(position)), as_vector(b.cycle_arcs(position)));
    }
}

// The complete digraph of n nodes has C(n, k) (k - 1)! cycles of length
// k: n (n - 1) ... (n - k + 1) / k
TEST(BoundedCycleIndex, CountsTheCyclesOfACompleteDigraph)
{
  for (size_t n = 1; n <= 8; ++n)
    {
      const Built<List_Type> built(n, complete_ends(n));
      const Bounded_Cycle_Index index(built.g, {1, 2, 3, 4, 5, 6, 7, 8});
      EXPECT_EQ(index.num_cycles(1), 0u);
      size_t total = 0;
      for (size_t k = 2; k <= 8; ++k)
        {
          const size_t expected = k > n ? 0 : falling(n, k) / k;
          EXPECT_EQ(index.num_cycles(k), expected) << n << " nodes, length " << k;
          total += expected;
        }
      EXPECT_EQ(index.num_cycles(), total);
      EXPECT_EQ(index.num_cycles(0), 0u);
      EXPECT_EQ(index.num_cycles(9), 0u);
      EXPECT_EQ(index.num_cycles_up_to(100), total);
      EXPECT_EQ(index.num_cycles_up_to(0), 0u);
    }
}

// Each cycle starts with its least arc, the cycles go by length and then
// lexicographically, and those up to a length are the catalog of the
// smaller set, position by position
TEST(BoundedCycleIndex, OrdersTheCatalogAndKeepsThePrefixes)
{
  std::mt19937 rng(11);
  for (size_t round = 0; round < 30; ++round)
    {
      const size_t n = 3 + rng() % 4;
      const Built<List_Type> built(n, random_ends(rng, n, 8 + rng() % 12));
      const Bounded_Cycle_Index whole(built.g, {1, 2, 3, 4, 5, 6});
      for (size_t position = 0; position < whole.num_cycles(); ++position)
        {
          const auto arcs = as_vector(whole.cycle_arcs(position));
          EXPECT_EQ(*std::min_element(arcs.begin(), arcs.end()), arcs.front());
          EXPECT_EQ(std::set<size_t>(arcs.begin(), arcs.end()).size(), arcs.size());
          if (position == 0)
            continue;
          const auto before = as_vector(whole.cycle_arcs(position - 1));
          EXPECT_TRUE(before.size() < arcs.size() or
                      (before.size() == arcs.size() and before < arcs));
        }
      for (size_t m = 1; m <= 6; ++m)
        {
          Array<size_t> lengths;
          for (size_t k = 1; k <= m; ++k)
            lengths.append(k);
          const Bounded_Cycle_Index<List_Type> prefix(built.g, lengths);
          ASSERT_EQ(prefix.num_cycles(), whole.num_cycles_up_to(m));
          EXPECT_EQ(prefix.longest_length(), m);
          for (size_t position = 0; position < prefix.num_cycles(); ++position)
            ASSERT_EQ(as_vector(prefix.cycle_arcs(position)),
                      as_vector(whole.cycle_arcs(position)));
        }
      const Bounded_Cycle_Index some(built.g, {2, 5});
      EXPECT_EQ(some.num_cycles(), whole.num_cycles(2) + whole.num_cycles(5));
      EXPECT_TRUE(some.has_length(2) and some.has_length(5));
      EXPECT_FALSE(some.has_length(3) or some.has_length(0) or some.has_length(9));
    }
}

// Parallel arcs give different cycles, a loop is a cycle of one arc and
// is in no longer cycle, and the two senses of a cycle are two cycles
TEST(BoundedCycleIndex, TellsParallelArcsLoopsAndSensesApart)
{
  // 0 -> 1 twice, 1 -> 0, a loop at 1, and the triangle 0 1 2 both ways
  const Built<List_Type> built(3, {{0, 1}, {0, 1}, {1, 0}, {1, 1}, {1, 2}, {2, 0}, {2, 1}, {0, 2}});
  const Bounded_Cycle_Index index(built.g, {1, 2, 3});
  EXPECT_EQ(index.num_cycles(1), 1u);
  EXPECT_EQ(as_vector(index.cycle_arcs(0)), Cycle({3}));
  // of two arcs: 0 2, 1 2, 4 6 (1 -> 2 -> 1) and 5 7 (2 -> 0 -> 2)
  EXPECT_EQ(index.num_cycles(2), 4u);
  EXPECT_EQ(as_vector(index.cycle_arcs(1)), Cycle({0, 2}));
  EXPECT_EQ(as_vector(index.cycle_arcs(2)), Cycle({1, 2}));
  // of three: 0 -> 1 -> 2 -> 0 with each parallel arc, and 0 -> 2 -> 1 -> 0
  EXPECT_EQ(index.num_cycles(3), 3u);
  EXPECT_EQ(as_vector(index.cycle_arcs(5)), Cycle({0, 4, 5}));
  EXPECT_EQ(as_vector(index.cycle_arcs(6)), Cycle({1, 4, 5}));
  EXPECT_EQ(as_vector(index.cycle_arcs(7)), Cycle({2, 7, 6}));
  EXPECT_EQ(as_vector(index.cycles_of(3)), std::vector<size_t>({0})); // the loop
}

// The arcs take their numbers in the order given and, where it does not
// tell them apart, in that of the iterator; an arc of another graph has
// no number
TEST(BoundedCycleIndex, NumbersTheArcsInTheOrderGiven)
{
  const Built<List_Type> built(3, {{0, 1}, {1, 2}, {2, 0}, {0, 2}});
  const Built<List_Type> other(2, {{0, 1}});
  const auto down = [] (const List_Type::Arc *a, const List_Type::Arc *b)
    {
      return a->get_info() > b->get_info();
    };
  const Bounded_Cycle_Index index(built.g, {3}, down);
  ASSERT_EQ(index.num_arcs(), 4u);
  for (size_t number = 0; number < 4; ++number)
    {
      EXPECT_EQ(index.arc(number)->get_info(), int(3 - number));
      EXPECT_EQ(index.arc_number(index.arc(number)), number);
    }
  EXPECT_THROW((void) index.arc(4), std::out_of_range);
  EXPECT_THROW((void) index.arc_number(other.arcs[0]), std::domain_error);
  EXPECT_THROW((void) index.arc_number(nullptr), std::domain_error);

  const Bounded_Cycle_Index in_list_order(built.g, {3});
  size_t number = 0;
  for (List_Type::Arc_Iterator it(built.g); it.has_curr(); it.next_ne(), ++number)
    EXPECT_EQ(in_list_order.arc(number), it.get_curr_ne());
}

// Until an arc is given a weight it cannot be used: no cycle through it
// is below a bound, not even an infinite one
TEST(BoundedCycleIndex, StartsWithArcsThatCannotBeUsed)
{
  const Built<List_Type> built(4, complete_ends(4));
  Bounded_Cycle_Index index(built.g, {2, 3, 4});
  size_t handed = 0;
  const auto count = [&handed] (size_t, double) { ++handed; };
  for (size_t a = 0; a < index.num_arcs(); ++a)
    EXPECT_EQ(index.weight(a), Infinity);
  EXPECT_EQ(index.for_each_cycle_below(Infinity, 8, count), index.num_cycles());
  EXPECT_EQ(handed, 0u);
  for (size_t position = 0; position < index.num_cycles(); ++position)
    EXPECT_EQ(index.weight_sum(position), Infinity);

  // all but one arc: only the cycles without it are below
  for (size_t a = 1; a < index.num_arcs(); ++a)
    index.set_weight(a, 1);
  index.for_each_cycle_below(Infinity, 8, count);
  EXPECT_EQ(handed, index.num_cycles() - index.cycles_of(0).size());
  handed = 0;
  EXPECT_EQ(index.update(0, 1, Infinity, 8, count), index.cycles_of(0).size());
  EXPECT_EQ(handed, index.cycles_of(0).size());
  handed = 0;
  index.update(0, Infinity, Infinity, 8, count); // and it can be taken out again
  EXPECT_EQ(handed, 0u);
}

// update() hands the cycles of its arc below the bound and up to the
// length, by ascending position, each with the sum of weight_sum(), which
// is that of the weights added in the order of the cycle
TYPED_TEST(BoundedCycleIndexOn, UpdateHandsTheCyclesBelowTheBound)
{
  using GT = TypeParam;
  std::mt19937 rng(20261114);
  std::uniform_real_distribution<double> weight(-3, 3);
  size_t handed_in_all = 0, looked_at = 0;
  for (size_t round = 0; round < 60; ++round)
    {
      const size_t n = 2 + rng() % 5;
      const Built<GT> built(n, random_ends(rng, n, 6 + rng() % 14));
      Bounded_Cycle_Index<GT> index(built.g, {1, 2, 3, 4, 5, 6});
      std::vector<double> weights(index.num_arcs());
      for (size_t a = 0; a < index.num_arcs(); ++a)
        index.set_weight(a, weights[a] = rng() % 9 == 0 ? Infinity : weight(rng));
      for (size_t step = 0; step < 40; ++step)
        {
          const size_t arc = rng() % index.num_arcs();
          const double w = rng() % 11 == 0 ? Infinity : weight(rng);
          const double bound = weight(rng);
          const size_t max_length = rng() % 10;
          weights[arc] = w;
          Handed handed;
          const size_t seen = index.update(arc, w, bound, max_length, [&] (size_t p, double s)
            {
              handed.emplace_back(p, s);
            });
          ASSERT_EQ(index.weight(arc), w);
          ASSERT_EQ(handed, below_of(index, arc, bound, max_length));
          size_t expected_seen = 0;
          for (const size_t position : index.cycles_of(arc))
            expected_seen += index.length(position) <= max_length;
          ASSERT_EQ(seen, expected_seen);
          for (const auto & [position, sum] : handed)
            {
              const auto arcs = index.cycle_arcs(position);
              double expected = weights[arcs[0]];
              for (size_t j = 1; j < arcs.size(); ++j)
                expected += weights[arcs[j]];
              ASSERT_EQ(sum, expected); // the same number: the weights in the order of the cycle
            }
          handed_in_all += handed.size();
          looked_at += seen;
        }
    }
  EXPECT_GT(handed_in_all, 500u);
  EXPECT_GT(looked_at, handed_in_all);
}

// The sum of a cycle is the same number whichever of its arcs changed
TEST(BoundedCycleIndex, HandsTheSameSumWhicheverArcChanged)
{
  const Built<List_Type> built(4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
  Bounded_Cycle_Index index(built.g, {4});
  const double weights[] = {0.1, 1e16, -1e16, 0.3}; // the order of the sum matters
  for (size_t a = 0; a < 4; ++a)
    index.set_weight(a, weights[a]);
  const double sum = ((0.1 + 1e16) - 1e16) + 0.3;
  EXPECT_EQ(index.weight_sum(0), sum);
  for (size_t a = 0; a < 4; ++a)
    {
      Handed handed;
      index.update(a, weights[a], Infinity, 4, [&] (size_t p, double s)
        {
          handed.emplace_back(p, s);
        });
      ASSERT_EQ(handed, Handed({{0, sum}})) << "arc " << a;
    }
}

// A change of several arcs is seen whole, and each cycle once: also the
// cycle of two arcs that has both, and with an arc given twice
TYPED_TEST(BoundedCycleIndexOn, HandsOnceEachCycleOfSeveralArcs)
{
  using GT = TypeParam;
  std::mt19937 rng(42);
  std::uniform_real_distribution<double> weight(-2, 2);
  size_t shared = 0;
  for (size_t round = 0; round < 80; ++round)
    {
      const size_t n = 2 + rng() % 4;
      const Built<GT> built(n, random_ends(rng, n, 5 + rng() % 12));
      Bounded_Cycle_Index<GT> index(built.g, {1, 2, 3, 4, 5});
      for (size_t a = 0; a < index.num_arcs(); ++a)
        index.set_weight(a, weight(rng));
      for (size_t step = 0; step < 12; ++step)
        {
          Array<size_t> arcs;
          for (size_t i = 0, m = rng() % 5; i < m; ++i)
            arcs.append(rng() % index.num_arcs()); // some come twice
          for (size_t i = 0; i < arcs.size(); ++i)
            index.set_weight(arcs(i), weight(rng));
          const double bound = weight(rng);
          const size_t max_length = 1 + rng() % 6;

          std::set<size_t> expected;
          size_t expected_seen = 0, memberships = 0;
          std::set<size_t> of_any;
          for (size_t i = 0; i < arcs.size(); ++i)
            for (const size_t position : index.cycles_of(arcs(i)))
              if (index.length(position) <= max_length)
                {
                  ++memberships;
                  if (not of_any.insert(position).second)
                    continue;
                  ++expected_seen;
                  if (index.weight_sum(position) < bound)
                    expected.insert(position);
                }
          shared += memberships - expected_seen;

          Handed handed;
          const size_t seen =
            index.for_each_cycle_below(arcs, bound, max_length, [&] (size_t p, double s)
              {
                handed.emplace_back(p, s);
              });
          ASSERT_EQ(seen, expected_seen);
          std::set<size_t> positions;
          for (const auto & [position, sum] : handed)
            {
              ASSERT_TRUE(positions.insert(position).second) << "handed twice";
              ASSERT_EQ(sum, index.weight_sum(position));
            }
          ASSERT_EQ(positions, expected);

          // and nothing is left marked: the same call gives the same
          size_t again = 0;
          index.for_each_cycle_below(arcs, bound, max_length,
                                     [&again] (size_t, double) { ++again; });
          ASSERT_EQ(again, handed.size());
        }
    }
  EXPECT_GT(shared, 200u); // the cycles with several of the arcs did come out

  // the two arcs of a cycle of length 2, as the two arcs of a market
  const Built<GT> pair(2, {{0, 1}, {1, 0}});
  Bounded_Cycle_Index<GT> index(pair.g, {2});
  index.set_weight(0, -1);
  index.set_weight(1, -1);
  Handed handed;
  EXPECT_EQ(index.for_each_cycle_below({0, 1, 0}, 0, 2, [&] (size_t p, double s)
                                       {
                                         handed.emplace_back(p, s);
                                       }), 1u);
  EXPECT_EQ(handed, Handed({{0, -2.0}}));
}

// for_each_cycle_below() on the whole catalog hands what weight_sum() says
TEST(BoundedCycleIndex, WalksTheWholeCatalog)
{
  std::mt19937 rng(3);
  std::uniform_real_distribution<double> weight(-1, 1);
  const Built<List_Type> built(6, complete_ends(6));
  Bounded_Cycle_Index index(built.g, {2, 3, 4, 5});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, weight(rng));
  const auto & constant = index;
  for (size_t max_length = 0; max_length <= 9; ++max_length)
    {
      Handed handed, expected;
      const size_t seen = constant.for_each_cycle_below(-0.5, max_length, [&] (size_t p, double s)
        {
          handed.emplace_back(p, s);
        });
      EXPECT_EQ(seen, index.num_cycles_up_to(max_length));
      for (size_t position = 0; position < index.num_cycles_up_to(max_length); ++position)
        if (index.weight_sum(position) < -0.5)
          expected.emplace_back(position, index.weight_sum(position));
      EXPECT_EQ(handed, expected);
      EXPECT_FALSE(max_length >= 2 and handed.empty());
    }
}

// An arc that is in no cycle, a digraph of one arc, a digraph without
// cycles and a digraph without arcs give empty ranges, and their arcs can
// be changed
TEST(BoundedCycleIndex, GivesEmptyRanges)
{
  size_t handed = 0;
  const auto count = [&handed] (size_t, double) { ++handed; };
  {
    const Built<List_Type> one_arc(2, {{0, 1}});
    Bounded_Cycle_Index index(one_arc.g, {3});
    EXPECT_EQ(index.num_cycles(), 0u);
    EXPECT_TRUE(index.cycles_of(0).empty());
    EXPECT_EQ(index.update(0, -1, Infinity, 8, count), 0u);
    EXPECT_EQ(index.for_each_cycle_below({0}, Infinity, 8, count), 0u);
    EXPECT_EQ(index.for_each_cycle_below(Infinity, 8, count), 0u);
    EXPECT_EQ(index.weight(0), -1);
    EXPECT_THROW((void) index.length(0), std::out_of_range);
    EXPECT_THROW((void) index.cycle_arcs(0), std::out_of_range);
    EXPECT_THROW((void) index.weight_sum(0), std::out_of_range);
    EXPECT_THROW((void) index.is_stale(0, 0), std::out_of_range);
  }
  {
    // a triangle, and an arc that leaves it: in no cycle
    const Built<List_Type> with_tail(4, {{0, 1}, {1, 2}, {2, 0}, {2, 3}});
    Bounded_Cycle_Index index(with_tail.g, {2, 3, 4});
    EXPECT_EQ(index.num_cycles(), 1u);
    EXPECT_TRUE(index.cycles_of(3).empty());
    EXPECT_EQ(index.update(3, -1, Infinity, 8, count), 0u);
    // the triangle is of length 3: none of its arcs has cycles of 2 or 4
    for (size_t a = 0; a < 3; ++a)
      EXPECT_EQ(index.update(a, -1, Infinity, 2, count), 0u);
    EXPECT_EQ(handed, 0u);
    EXPECT_EQ(index.update(0, -1, Infinity, 3, count), 1u);
    EXPECT_EQ(handed, 1u);
  }
  {
    const Built<List_Type> acyclic(5, {{0, 1}, {1, 2}, {0, 2}, {2, 3}, {3, 4}, {0, 4}});
    Bounded_Cycle_Index index(acyclic.g, {1, 2, 3, 4, 5, 6, 7, 8});
    EXPECT_EQ(index.num_cycles(), 0u);
    for (size_t a = 0; a < index.num_arcs(); ++a)
      {
        EXPECT_TRUE(index.cycles_of(a).empty());
        EXPECT_EQ(index.update(a, -1, Infinity, 8, count), 0u);
      }
  }
  {
    const Built<List_Type> no_arcs(3, {});
    Bounded_Cycle_Index index(no_arcs.g, {1, 2, 3});
    EXPECT_EQ(index.num_arcs(), 0u);
    EXPECT_EQ(index.num_cycles(), 0u);
    EXPECT_THROW((void) index.cycles_of(0), std::out_of_range);
    EXPECT_THROW(index.update(0, 1, 0, 3, count), std::out_of_range);
    EXPECT_EQ(index.for_each_cycle_below(Array<size_t>(), 0, 3, count), 0u);
    EXPECT_EQ(index.for_each_cycle_below(0, 3, count), 0u);
    List_Type empty;
    Bounded_Cycle_Index of_empty(empty, {1});
    EXPECT_EQ(of_empty.num_arcs(), 0u);
  }
  EXPECT_EQ(handed, 1u);
}

// What is not a weight, a bound, an arc or a position is rejected, and
// the index keeps what it had
TEST(BoundedCycleIndex, RejectsWhatIsNotValid)
{
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const Built<List_Type> built(3, {{0, 1}, {1, 2}, {2, 0}});
  Bounded_Cycle_Index index(built.g, {3});
  for (size_t a = 0; a < 3; ++a)
    index.set_weight(a, 1);
  size_t handed = 0;
  const auto count = [&handed] (size_t, double) { ++handed; };
  const uint64_t clock = index.clock();

  EXPECT_THROW(index.set_weight(0, nan), std::domain_error);
  EXPECT_THROW(index.set_weight(0, -Infinity), std::domain_error);
  EXPECT_THROW(index.set_weight(3, 1), std::out_of_range);
  EXPECT_THROW(index.update(0, nan, 10, 3, count), std::domain_error);
  EXPECT_THROW(index.update(0, -Infinity, 10, 3, count), std::domain_error);
  EXPECT_THROW(index.update(0, 2, nan, 3, count), std::domain_error);
  EXPECT_THROW(index.update(3, 2, 10, 3, count), std::out_of_range);
  EXPECT_THROW(index.for_each_cycle_below(nan, 3, count), std::domain_error);
  EXPECT_THROW(index.for_each_cycle_below({0}, nan, 3, count), std::domain_error);
  EXPECT_THROW(index.for_each_cycle_below({0, 3}, 10, 3, count), std::out_of_range);
  EXPECT_THROW((void) index.weight(3), std::out_of_range);
  EXPECT_THROW((void) index.stamp(3), std::out_of_range);
  EXPECT_THROW(index.touch(3), std::out_of_range);
  EXPECT_THROW((void) index.cycles_of(3), std::out_of_range);
  EXPECT_THROW((void) index.length(1), std::out_of_range);

  EXPECT_EQ(handed, 0u);
  EXPECT_EQ(index.clock(), clock);
  for (size_t a = 0; a < 3; ++a)
    EXPECT_EQ(index.weight(a), 1);
  EXPECT_EQ(index.for_each_cycle_below({0, 1, 2}, 10, 3, count), 1u); // no mark was left
  EXPECT_EQ(handed, 1u);
  EXPECT_EQ(index.update(0, Infinity, 10, 3, count), 1u); // plus infinity is a weight
  EXPECT_EQ(handed, 1u);
}

// The lengths, the kind of graph and the number of cycles have limits
TEST(BoundedCycleIndex, HasLimits)
{
  const Built<List_Type> built(5, complete_ends(5));
  EXPECT_THROW(Bounded_Cycle_Index(built.g, {0}), std::domain_error);
  EXPECT_THROW(Bounded_Cycle_Index(built.g, {3, 9}), std::domain_error);
  EXPECT_THROW(Bounded_Cycle_Index(built.g, std::initializer_list<size_t>()), std::domain_error);
  EXPECT_THROW(Bounded_Cycle_Index<List_Type>(built.g, Array<size_t>()), std::domain_error);
  EXPECT_THROW(Bounded_Cycle_Index<List_Type>(built.g, Array<size_t>({2, 12})), std::domain_error);

  // 10 cycles of length 2 and 20 of length 3
  EXPECT_NO_THROW(Bounded_Cycle_Index(built.g, {2, 3}, Arc_List_Order(), 30));
  EXPECT_THROW(Bounded_Cycle_Index(built.g, {2, 3}, Arc_List_Order(), 29), std::length_error);
  EXPECT_THROW(Bounded_Cycle_Index(built.g, {2, 3}, Arc_List_Order(), 0), std::length_error);
  EXPECT_NO_THROW(Bounded_Cycle_Index(built.g, {1}, Arc_List_Order(), 0)); // it has no loops

  using Undirected = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  Undirected g;
  auto p = g.insert_node(0), q = g.insert_node(1);
  g.insert_arc(p, q, 0);
  EXPECT_THROW(Bounded_Cycle_Index<Undirected>(g, {2}), std::domain_error);
}

// With more arcs than fit in 16 bits the index keeps the numbers of the
// arcs of each cycle in 32, and answers the same
TEST(BoundedCycleIndex, HoldsMoreArcsThanFitIn16Bits)
{
  const size_t chain = 70000; // a path: its arcs are in no cycle
  Ends ends;
  for (size_t i = 0; i < chain; ++i)
    ends.emplace_back(i, i + 1);
  const size_t base = chain + 1; // then a complete digraph of 4 nodes
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
      if (i != j)
        ends.emplace_back(base + i, base + j);
  const Built<List_Type> built(base + 4, ends);
  Bounded_Cycle_Index index(built.g, {2, 3, 4});
  ASSERT_EQ(index.num_arcs(), chain + 12);
  EXPECT_EQ(index.num_cycles(2), 6u);
  EXPECT_EQ(index.num_cycles(3), 8u);
  EXPECT_EQ(index.num_cycles(4), 6u);
  expect_the_catalog(index, Oracle<List_Type>(built.g, {2, 3, 4}, Arc_List_Order()));

  std::mt19937 rng(5);
  std::uniform_real_distribution<double> weight(-1, 1);
  for (size_t a = chain; a < chain + 12; ++a)
    index.set_weight(a, weight(rng));
  size_t handed_in_all = 0;
  for (size_t step = 0; step < 200; ++step)
    {
      const size_t arc = chain + rng() % 12;
      const double w = weight(rng), bound = weight(rng);
      Handed handed;
      index.update(arc, w, bound, 4, [&] (size_t p, double s) { handed.emplace_back(p, s); });
      ASSERT_EQ(handed, below_of(index, arc, bound, 4));
      handed_in_all += handed.size();
    }
  EXPECT_GT(handed_in_all, 100u);
  EXPECT_TRUE(index.cycles_of(0).empty());
  EXPECT_EQ(index.update(chain - 1, -5, Infinity, 4, [] (size_t, double) {}), 0u);
  Handed once;
  index.for_each_cycle_below({chain, chain + 3, chain + 1}, Infinity, 4,
                             [&] (size_t p, double s) { once.emplace_back(p, s); });
  std::set<size_t> positions;
  for (const auto & [position, sum] : once)
    EXPECT_TRUE(positions.insert(position).second);
}

// If the function throws, the weight is changed, the index is whole and
// no mark is left
TEST(BoundedCycleIndex, StaysWholeWhenTheFunctionThrows)
{
  const Built<List_Type> built(4, complete_ends(4));
  Bounded_Cycle_Index index(built.g, {2, 3, 4});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, 1);
  size_t calls = 0;
  const auto third_throws = [&calls] (size_t, double)
    {
      if (++calls == 3)
        throw std::runtime_error("the third one");
    };
  EXPECT_THROW(index.update(0, -7, Infinity, 4, third_throws), std::runtime_error);
  EXPECT_EQ(calls, 3u);
  EXPECT_EQ(index.weight(0), -7);

  calls = 0;
  EXPECT_THROW(index.for_each_cycle_below({1, 2, 3}, Infinity, 4, third_throws),
               std::runtime_error);
  calls = 0;
  EXPECT_THROW(index.for_each_cycle_below(Infinity, 4, third_throws), std::runtime_error);

  // afterwards each query hands all that it must
  size_t handed = 0;
  const auto count = [&handed] (size_t, double) { ++handed; };
  std::set<size_t> of_any;
  for (const size_t a : {1, 2, 3})
    for (const size_t position : index.cycles_of(a))
      of_any.insert(position);
  EXPECT_EQ(index.for_each_cycle_below({1, 2, 3}, Infinity, 4, count), of_any.size());
  EXPECT_EQ(handed, of_any.size());
  handed = 0;
  EXPECT_EQ(index.update(0, -7, Infinity, 4, count), index.cycles_of(0).size());
  EXPECT_EQ(handed, index.cycles_of(0).size());
}

// Each write of an arc takes the next value of the clock, and a cycle is
// stale if one of its arcs was written after the moment given
TEST(BoundedCycleIndex, StampsTheWritesOfEachArc)
{
  // two triangles that share the arc 0 -> 1
  const Built<List_Type> built(4, {{0, 1}, {1, 2}, {2, 0}, {1, 3}, {3, 0}});
  Bounded_Cycle_Index index(built.g, {3});
  ASSERT_EQ(index.num_cycles(), 2u); // 0 1 2 and 0 3 4
  EXPECT_EQ(index.clock(), 0u);
  for (size_t a = 0; a < 5; ++a)
    EXPECT_EQ(index.stamp(a), 0u);
  EXPECT_FALSE(index.is_stale(0, 0));

  for (size_t a = 0; a < 5; ++a)
    index.set_weight(a, 1);
  EXPECT_EQ(index.clock(), 5u);
  EXPECT_EQ(index.stamp(3), 4u);
  const uint64_t computed_at = index.clock();
  EXPECT_FALSE(index.is_stale(0, computed_at));
  EXPECT_FALSE(index.is_stale(1, computed_at));

  index.update(4, 2, 0, 3, [] (size_t, double) {}); // of the second triangle
  EXPECT_EQ(index.stamp(4), 6u);
  EXPECT_FALSE(index.is_stale(0, computed_at));
  EXPECT_TRUE(index.is_stale(1, computed_at));
  EXPECT_FALSE(index.is_stale(1, index.clock()));

  index.touch(0); // of both, and its weight is the same
  EXPECT_EQ(index.weight(0), 1);
  EXPECT_EQ(index.stamp(0), 7u);
  EXPECT_TRUE(index.is_stale(0, computed_at));
  EXPECT_TRUE(index.is_stale(1, 6));
  index.set_weight(0, 1); // the same weight is a write too
  EXPECT_EQ(index.clock(), 8u);
}

// swap() exchanges two indexes, also of different sets of lengths and of
// different graphs, and a copy does not share with its original
TEST(BoundedCycleIndex, SwapsAndCopies)
{
  const Built<List_Type> built(5, complete_ends(5));
  const Built<List_Type> other(3, {{0, 1}, {1, 2}, {2, 0}});
  Bounded_Cycle_Index small(built.g, {3});
  Bounded_Cycle_Index large(built.g, {3, 4, 5});
  Bounded_Cycle_Index triangle(other.g, {3});
  static_assert(noexcept(small.swap(large)));
  small.set_weight(0, 0.5);
  large.set_weight(0, 0.25);
  const size_t in_small = small.num_cycles(), in_large = large.num_cycles();

  small.swap(large);
  EXPECT_EQ(small.num_cycles(), in_large);
  EXPECT_EQ(large.num_cycles(), in_small);
  EXPECT_EQ(small.longest_length(), 5u);
  EXPECT_EQ(large.longest_length(), 3u);
  EXPECT_EQ(small.weight(0), 0.25);
  EXPECT_EQ(large.weight(0), 0.5);
  EXPECT_EQ(small.clock(), 1u);
  expect_the_catalog(small, Oracle<List_Type>(built.g, {3, 4, 5}, Arc_List_Order()));
  expect_the_catalog(large, Oracle<List_Type>(built.g, {3}, Arc_List_Order()));

  small.swap(triangle);
  EXPECT_EQ(small.num_arcs(), 3u);
  EXPECT_EQ(triangle.num_arcs(), 20u);
  EXPECT_EQ(small.arc_number(other.arcs[1]), 1u);
  EXPECT_THROW((void) small.arc_number(built.arcs[0]), std::domain_error);
  expect_the_catalog(triangle, Oracle<List_Type>(built.g, {3, 4, 5}, Arc_List_Order()));
  EXPECT_EQ(small.clock(), 0u);
  EXPECT_EQ(triangle.clock(), 1u);
  EXPECT_EQ(triangle.stamp(0), 1u);

  // each one answers with what it has now
  for (Bounded_Cycle_Index<List_Type> *index : {&small, &triangle})
    {
      for (size_t a = 0; a < index->num_arcs(); ++a)
        index->set_weight(a, a % 3 == 0 ? -1.0 : 0.25);
      const size_t last = index->num_arcs() - 1;
      Handed handed;
      const auto keep = [&handed] (size_t p, double s) { handed.emplace_back(p, s); };
      index->update(last, -0.5, 0, 8, keep);
      EXPECT_EQ(handed, below_of(*index, last, 0, 8));
      EXPECT_FALSE(handed.empty());
      handed.clear();
      const size_t seen = index->for_each_cycle_below({0, last}, 0, 8, keep);
      std::set<size_t> of_any;
      for (const size_t a : {size_t(0), last})
        for (const size_t position : index->cycles_of(a))
          of_any.insert(position);
      EXPECT_EQ(seen, of_any.size());
      size_t below = 0;
      for (const size_t position : of_any)
        below += index->weight_sum(position) < 0;
      EXPECT_EQ(handed.size(), below);
    }

  Bounded_Cycle_Index copy = triangle;
  copy.set_weight(0, 9);
  EXPECT_EQ(triangle.weight(0), -1);
  EXPECT_EQ(copy.weight(0), 9);
  EXPECT_EQ(copy.clock(), triangle.clock() + 1);
  expect_the_catalog(copy, Oracle<List_Type>(built.g, {3, 4, 5}, Arc_List_Order()));
  Bounded_Cycle_Index moved = std::move(copy);
  EXPECT_EQ(moved.weight(0), 9);
  expect_the_catalog(moved, Oracle<List_Type>(built.g, {3, 4, 5}, Arc_List_Order()));
}

// A copy made by the function that receives the cycles, while some arcs
// are marked, is in no query: it hands every cycle that it is asked for.
// So does an index that is assigned there.
TEST(BoundedCycleIndex, StartsACopyMadeInsideAQueryWithoutMarks)
{
  // two cycles of two arcs, apart: the arcs 0 and 1, and the arcs 2 and 3
  const Built<List_Type> built(4, {{0, 1}, {1, 0}, {2, 3}, {3, 2}});
  const Built<List_Type> other(3, {{0, 1}, {1, 2}, {2, 0}});
  Bounded_Cycle_Index index(built.g, {2});
  ASSERT_EQ(index.num_cycles(), 2u);
  for (size_t a = 0; a < 4; ++a)
    index.set_weight(a, -1);

  std::vector<Bounded_Cycle_Index<List_Type>> copies;
  Bounded_Cycle_Index assigned(other.g, {3});
  size_t calls = 0;
  index.for_each_cycle_below({0, 2}, 0, 2, [&] (size_t, double)
    {
      if (++calls == 2) // the cycle of the arc 2: the arc 0 is marked
        {
          copies.push_back(index);
          assigned = index;
        }
    });
  ASSERT_EQ(calls, 2u);
  ASSERT_EQ(copies.size(), 1u);

  // the arc 1 after another arc: its cycle goes through the arc 0
  for (Bounded_Cycle_Index<List_Type> *each : {&copies[0], &assigned, &index})
    {
      size_t handed = 0;
      EXPECT_EQ(each->for_each_cycle_below({2, 1}, 0, 2, [&handed] (size_t, double) { ++handed; }),
                2u);
      EXPECT_EQ(handed, 2u);
    }
  EXPECT_EQ(Answers(copies[0]), Answers(index));
  EXPECT_EQ(Answers(assigned), Answers(index));
}

// An index that is moved from, by construction or by assignment, is left
// without arcs and without cycles; it answers every query, and it can
// be copied, exchanged and given an index again
TEST(BoundedCycleIndex, LeavesAnIndexThatIsMovedFromWithoutArcsAndCycles)
{
  static_assert(std::is_nothrow_move_constructible_v<Bounded_Cycle_Index<List_Type>>);
  static_assert(std::is_nothrow_move_assignable_v<Bounded_Cycle_Index<List_Type>>);
  const Built<List_Type> built(4, complete_ends(4));
  const Built<List_Type> other(3, {{0, 1}, {1, 2}, {2, 0}});
  Bounded_Cycle_Index index(built.g, {2, 3});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, a % 2 == 0 ? -1.0 : 0.5);
  Bounded_Cycle_Index copy = index;
  const Answers before(copy);
  ASSERT_GT(before.cycles.size(), 0u);
  ASSERT_GT(before.handed.size(), 0u);

  Bounded_Cycle_Index taken(std::move(index));
  EXPECT_EQ(Answers(taken), before);
  expect_no_arcs_and_no_cycles(index, built.arcs[0]);

  Bounded_Cycle_Index triangle(other.g, {3});
  triangle.set_weight(0, 0.125);
  triangle = std::move(taken); // what the triangle had is freed
  EXPECT_EQ(Answers(triangle), before);
  expect_no_arcs_and_no_cycles(taken, built.arcs[0]);

  // a copy of one without arcs has none either, and so is what is
  // exchanged with it
  Bounded_Cycle_Index of_none = index;
  expect_no_arcs_and_no_cycles(of_none, built.arcs[0]);
  of_none.swap(triangle);
  EXPECT_EQ(Answers(of_none), before);
  expect_no_arcs_and_no_cycles(triangle, built.arcs[0]);

  // and each of them takes an index again, by copy and by move
  index = copy;
  EXPECT_EQ(Answers(index), before);
  taken = std::move(index);
  EXPECT_EQ(Answers(taken), before);
  expect_no_arcs_and_no_cycles(index, built.arcs[0]);
  taken.set_weight(0, 7);
  EXPECT_EQ(copy.weight(0), -1);
}

// An index that is assigned to itself, by copy or by move, keeps all
// that it had
TEST(BoundedCycleIndex, KeepsWhatItHadWhenItIsAssignedToItself)
{
  const Built<List_Type> built(4, complete_ends(4));
  Bounded_Cycle_Index index(built.g, {2, 3, 4});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, a % 3 == 0 ? -2.0 : 0.25);
  const Answers before(index);
  Bounded_Cycle_Index<List_Type> & same = index;
  index = same;
  EXPECT_EQ(Answers(index), before);
  index = std::move(same);
  EXPECT_EQ(Answers(index), before);
  index.swap(same);
  EXPECT_EQ(Answers(index), before);
}

// If the function throws when an arc was already looked at, the mark of
// that arc is cleared: the next query hands the cycles of that arc
TEST(BoundedCycleIndex, ClearsTheMarkOfAnArcWhenTheFunctionThrowsAfterIt)
{
  const Built<List_Type> built(4, complete_ends(4));
  Bounded_Cycle_Index index(built.g, {2, 3, 4});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, 1);
  // the arcs 1 and 2 leave the same node: no cycle has both
  const size_t of_first = index.cycles_of(1).size();
  ASSERT_GT(of_first, 0u);
  size_t calls = 0;
  const auto throws_at_the_second_arc = [&calls, of_first] (size_t, double)
    {
      if (++calls == of_first + 1)
        throw std::runtime_error("the first cycle of the second arc");
    };
  EXPECT_THROW(index.for_each_cycle_below({1, 2}, Infinity, 4, throws_at_the_second_arc),
               std::runtime_error);
  EXPECT_EQ(calls, of_first + 1);

  size_t handed = 0;
  const auto count = [&handed] (size_t, double) { ++handed; };
  const size_t of_both = of_first + index.cycles_of(2).size();
  EXPECT_EQ(index.for_each_cycle_below({2, 1}, Infinity, 4, count), of_both);
  EXPECT_EQ(handed, of_both);
}

// The additions that widen the bound are rounded as well: a bound that is
// rounded down to the sum of a cycle loses it, and one that is rounded up
// after each addition does not
TEST(BoundedCycleIndex, NeedsTheBoundRoundedUp)
{
  // a loop that weighs exactly 1, below an exact threshold of 1 + 2^-54
  // that the caller has as 1, within 2^-54
  const Built<List_Type> built(1, {{0, 0}});
  Bounded_Cycle_Index index(built.g, {1});
  const double approximate = 1, its_error = 0x1p-54;
  const double of_the_sum = Bounded_Cycle_Index<List_Type>::sum_error_bound(1, 1);
  EXPECT_EQ(of_the_sum, 0);
  size_t handed = 0;
  const auto count = [&handed] (size_t, double) { ++handed; };

  const double rounded_down = approximate + of_the_sum + its_error;
  EXPECT_EQ(rounded_down, 1);
  index.update(0, 1, rounded_down, 1, count);
  EXPECT_EQ(handed, 0u);

  double bound = std::nextafter(approximate + of_the_sum, Infinity);
  bound = std::nextafter(bound + its_error, Infinity);
  index.update(0, 1, bound, 1, count);
  EXPECT_EQ(handed, 1u);
}

// With only the loops asked for, the index has each loop of a multigraph
// and no other cycle, also among many parallel arcs
TEST(BoundedCycleIndex, HasOnlyTheLoopsWhenOnlyTheyAreAskedFor)
{
  Ends ends;
  for (size_t i = 0; i < 300; ++i)
    {
      ends.emplace_back(0, 1);
      ends.emplace_back(1, 2);
      if (i % 100 == 0)
        ends.emplace_back(i / 100, i / 100); // a loop at the nodes 0, 1 and 2
    }
  ends.emplace_back(2, 0); // closes cycles of three arcs, which are not asked for
  const Built<List_Type> built(3, ends);
  Bounded_Cycle_Index index(built.g, {1});
  expect_the_catalog(index, Oracle<List_Type>(built.g, {1}, Arc_List_Order()));
  EXPECT_EQ(index.num_cycles(), 3u);
  for (size_t position = 0; position < index.num_cycles(); ++position)
    {
      const size_t loop = index.cycle_arcs(position)[0];
      EXPECT_EQ(built.g.get_src_node(index.arc(loop)), built.g.get_tgt_node(index.arc(loop)));
      EXPECT_EQ(as_vector(index.cycles_of(loop)), std::vector<size_t>{position});
    }
}

// The sum that the index computes is within sum_error_bound() of the
// exact sum of the weights, taken with rationals; and with the bound
// widened by it, no cycle whose exact sum is below a threshold is lost
TEST(BoundedCycleIndex, BoundsTheErrorOfTheSum)
{
  using Index = Bounded_Cycle_Index<List_Type>;
  EXPECT_EQ(Index::sum_error_bound(0, 100), 0);
  EXPECT_EQ(Index::sum_error_bound(1, 100), 0);
  EXPECT_GT(Index::sum_error_bound(2, 100), 0);
  static_assert(Index::sum_error_bound(6, 64) < 4e-13); // the figure of the report
  static_assert(Index::sum_error_bound(6, 64) > 2e-13);

  std::mt19937_64 rng(20261115);
  size_t inexact = 0, kept_by_the_margin = 0;
  for (size_t k = 2; k <= 8; ++k)
    {
      Ends ring;
      for (size_t i = 0; i < k; ++i)
        ring.emplace_back(i, (i + 1) % k);
      const Built<List_Type> built(k, ring);
      Index index(built.g, {k});
      ASSERT_EQ(index.num_cycles(), 1u);
      for (size_t round = 0; round < 4000; ++round)
        {
          // weights of very different magnitudes, some of them cancelling
          const double scale = std::ldexp(1.0, int(rng() % 60));
          double largest = 0;
          mpq_class exact = 0;
          double previous = 0;
          for (size_t a = 0; a < k; ++a)
            {
              // two statements: the order of the arguments of a call is
              // not fixed, and each compiler would draw other weights
              const double fraction = double(int64_t(rng() >> 11)) / 9007199254740992.0 - 0.5;
              double w = std::ldexp(fraction, int(rng() % 60));
              if (a % 3 == 2)
                w = -previous * (1 + std::ldexp(1.0, -30 - int(rng() % 20)));
              w = std::fmin(std::fmax(w, -scale), scale);
              index.set_weight(a, previous = w);
              largest = std::fmax(largest, std::fabs(w));
              exact += mpq_class(w);
            }
          const double sum = index.weight_sum(0);
          const double error_bound = Index::sum_error_bound(k, largest);
          const mpq_class error = abs(mpq_class(sum) - exact);
          // not ASSERT_LE: it would print the rationals, and with libc++
          // the inserters of the library of GMP do not link
          ASSERT_TRUE(error <= mpq_class(error_bound)) << "length " << k << ", sum " << sum;
          inexact += error != 0;

          // a threshold just above the exact sum: the cycle is below it
          const mpq_class threshold =
            exact + abs(exact) / mpz_class("1000000000000000000") +
            1 / mpq_class(std::ldexp(1.0, 200));
          const double t = threshold.get_d(); // truncated towards zero
          const double t_error = std::fabs(t) * 0x1p-52 + 0x1p-1000;
          size_t handed = 0;
          index.for_each_cycle_below(t + t_error + error_bound, k,
                                     [&handed] (size_t, double) { ++handed; });
          ASSERT_EQ(handed, 1u) << "a cycle below the threshold was lost";
          handed = 0;
          index.for_each_cycle_below(t, k, [&handed] (size_t, double) { ++handed; });
          kept_by_the_margin += handed == 0;
        }
    }
  EXPECT_GT(inexact, 5000u);            // the sums did round
  EXPECT_GT(kept_by_the_margin, 500u);  // and without the margin cycles are lost
}
