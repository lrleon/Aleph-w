
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
 * @file bounded_cycle_index_allocation_test.cc
 * @brief Bounded_Cycle_Index when an allocation fails: building it throws
 *        std::bad_alloc whatever allocation fails, an index built apart
 *        replaces another without allocating, an assignment that fails
 *        leaves the index as it was, and neither the queries nor the
 *        moves allocate.
 *
 * This program replaces the global operator new, so that a test can make
 * an allocation fail and count those that are asked for.
 */

# include <gtest/gtest.h>

# include "allocation_failure.H"

# include <new>
# include <utility>
# include <vector>

# include <Bounded_Cycle_Index.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Index = Bounded_Cycle_Index<Graph>;

  // The complete digraph of n nodes
  void complete(Graph & g, const int n)
  {
    Array<Graph::Node *> nodes;
    for (int i = 0; i < n; ++i)
      nodes.append(g.insert_node(i));
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j)
        if (i != j)
          g.insert_arc(nodes(i), nodes(j), 0);
  }

  // All that an index answers, to compare two of them: its arcs with
  // their weights, their stamps and their cycles, its cycles with their
  // sums, its clock, and what a query of all its arcs hands
  struct Answers
  {
    std::vector<const void *> arcs;
    std::vector<double> numbers;
    std::vector<size_t> positions;

    explicit Answers(Index & index)
    {
      Array<size_t> all;
      for (size_t a = 0; a < index.num_arcs(); ++a)
        {
          arcs.push_back(index.arc(a));
          numbers.push_back(index.weight(a));
          numbers.push_back(double(index.stamp(a)));
          positions.push_back(index.arc_number(index.arc(a)));
          for (const uint32_t position : index.cycles_of(a))
            positions.push_back(position);
          all.append(a);
        }
      for (size_t position = 0; position < index.num_cycles(); ++position)
        {
          numbers.push_back(index.weight_sum(position));
          for (const uint32_t a : index.cycle_arcs(position))
            positions.push_back(a);
        }
      positions.push_back(index.num_cycles());
      positions.push_back(index.longest_length());
      positions.push_back(size_t(index.clock()));
      positions.push_back(index.for_each_cycle_below(all, 1e300, Index::Max_Length,
                                                     [this] (const size_t position, const double sum)
                                                     {
                                                       positions.push_back(position);
                                                       numbers.push_back(sum);
                                                     }));
    }

    bool operator == (const Answers &) const = default;
  };
}

// The tests need the operator new of allocation_failure.H
struct BoundedCycleIndexAllocation : public ::testing::Test
{
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

// Whatever allocation fails while an index is built, its constructor
// throws std::bad_alloc; what it had taken is freed, which the sanitizers
// see
TEST_F(BoundedCycleIndexAllocation, BuildingThrowsBadAllocWhateverAllocationFails)
{
  Graph g;
  complete(g, 5);
  size_t failures = 0;
  for (size_t k = 1; k <= 400; ++k)
    {
      bool thrown = false;
      size_t cycles = 0, refused = 0;
      {
        Fail_At the_kth(k);
        try
          {
            const Index index(g, {2, 3, 4});
            cycles = index.num_cycles();
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
        refused = refused_allocations;
      }
      if (refused == 0) // the constructor asks for less than k allocations
        {
          EXPECT_FALSE(thrown);
          EXPECT_EQ(cycles, 10u + 20u + 30u);
          EXPECT_GT(failures, 10u);
          return;
        }
      EXPECT_TRUE(thrown) << "allocation " << k;
      EXPECT_EQ(cycles, 0u) << "allocation " << k;
      ++failures;
    }
  ADD_FAILURE() << "an index asks for more than 400 allocations";
}

// A set of lengths is changed with an index built apart and swap(): if
// there is no memory to build it the index in use keeps what it had, and
// swap() does not ask for memory
TEST_F(BoundedCycleIndexAllocation, AnIndexBuiltApartReplacesAnotherWithoutAllocating)
{
  Graph g;
  complete(g, 5);
  Index in_use(g, {3});
  for (size_t a = 0; a < in_use.num_arcs(); ++a)
    in_use.set_weight(a, double(a));

  bool thrown = false;
  {
    No_Memory no_memory;
    try
      {
        const Index next(g, {3, 4});
      }
    catch (const std::bad_alloc &)
      {
        thrown = true;
      }
  }
  EXPECT_TRUE(thrown);
  EXPECT_EQ(in_use.num_cycles(), 20u);
  EXPECT_EQ(in_use.longest_length(), 3u);
  for (size_t a = 0; a < in_use.num_arcs(); ++a)
    EXPECT_EQ(in_use.weight(a), double(a));

  Index next(g, {3, 4});
  size_t asked = 0;
  {
    No_Memory no_memory;
    in_use.swap(next);
    asked = refused_allocations;
  }
  EXPECT_EQ(asked, 0u);
  EXPECT_EQ(in_use.num_cycles(), 50u);
  EXPECT_EQ(next.num_cycles(), 20u);
  EXPECT_EQ(next.weight(7), 7.0);
}

// A change of weight and the queries ask for no memory
TEST_F(BoundedCycleIndexAllocation, TheQueriesDoNotAllocate)
{
  Graph g;
  complete(g, 5);
  Index index(g, {2, 3, 4});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, a % 2 == 0 ? -1.0 : 0.5);
  const Array<size_t> arcs = {0, 3, 7, 3};
  Graph::Arc *first = index.arc(0);

  size_t handed = 0, seen = 0, asked = 0;
  double sum = 0;
  {
    No_Memory no_memory;
    const auto count = [&handed] (size_t, double) { ++handed; };
    seen += index.update(2, -1.0, 0.0, 4, count);
    seen += index.for_each_cycle_below(arcs, 0.0, 4, count);
    seen += index.for_each_cycle_below({1, 2}, 0.0, 4, count);
    seen += index.for_each_cycle_below(0.0, 4, count);
    index.set_weight(1, 0.25);
    index.touch(1);
    sum += index.weight_sum(0) + index.weight(1);
    seen += index.is_stale(0, 0) + index.cycles_of(0).size() + index.cycle_arcs(0).size();
    seen += index.arc_number(first) + index.length(0) + index.num_cycles_up_to(3);
    asked = refused_allocations;
  }
  EXPECT_EQ(asked, 0u);
  EXPECT_GT(handed, 0u);
  EXPECT_GT(seen, 0u);
  EXPECT_NE(sum, 0.0);
}

// An assignment copies apart and then exchanges: whatever allocation of
// the copy fails, it throws std::bad_alloc and the index keeps all that
// it had
TEST_F(BoundedCycleIndexAllocation, AnAssignmentThatFailsLeavesTheIndexAsItWas)
{
  Graph g, h;
  complete(g, 5);
  complete(h, 3);
  Index source(g, {2, 3, 4});
  for (size_t a = 0; a < source.num_arcs(); ++a)
    source.set_weight(a, double(a) - 7.5);
  const Answers of_the_source(source);

  size_t failures = 0;
  for (size_t k = 1; k <= 400; ++k)
    {
      Index index(h, {2, 3});
      index.set_weight(0, -1);
      index.set_weight(3, 0.25);
      const Answers before(index);
      bool thrown = false;
      size_t refused = 0;
      {
        Fail_At the_kth(k);
        try
          {
            index = source;
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
        refused = refused_allocations;
      }
      if (refused == 0) // the assignment asks for less than k allocations
        {
          EXPECT_FALSE(thrown);
          EXPECT_TRUE(Answers(index) == of_the_source);
          EXPECT_GT(failures, 10u);
          return;
        }
      EXPECT_TRUE(thrown) << "allocation " << k;
      EXPECT_TRUE(Answers(index) == before) << "allocation " << k;
      ++failures;
    }
  ADD_FAILURE() << "an assignment asks for more than 400 allocations";
}

// Moving an index, by construction or by assignment, asks for no memory
TEST_F(BoundedCycleIndexAllocation, MovingAnIndexDoesNotAllocate)
{
  Graph g, h;
  complete(g, 5);
  complete(h, 3);
  Index index(g, {2, 3, 4});
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, a % 2 == 0 ? -1.0 : 0.5);
  Index other(h, {2});
  const Answers before(index);

  size_t asked = 0, left = 0;
  {
    No_Memory no_memory;
    Index taken(std::move(index));
    other = std::move(taken);
    left = index.num_arcs() + taken.num_arcs() + index.num_cycles() + taken.num_cycles();
    asked = refused_allocations;
  }
  EXPECT_EQ(asked, 0u);
  EXPECT_EQ(left, 0u);
  EXPECT_TRUE(Answers(other) == before);
}
