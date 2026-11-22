
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
 * @file binheap.cc
 * @brief Tests for Binheap
 */
#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_binHeap.H>
#include <tpl_dynBinHeap.H>

using namespace Aleph;

TEST(DynBinHeapTest, InsertExtractInOrder)
{
  DynBinHeap<int> heap;
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1};

  for (const int v : values)
    heap.insert(v);

  std::vector<int> expected = values;
  std::sort(expected.begin(), expected.end());

  for (const int v : expected)
    {
      ASSERT_EQ(heap.top(), v);
      EXPECT_EQ(heap.getMin(), v);
    }

  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0u);
}

TEST(DynBinHeapTest, RemoveArbitraryNodes)
{
  DynBinHeap<int> heap;
  const std::vector<int> values = {40, 10, 30, 20, 50, 60, 70, 15};
  std::vector<std::reference_wrapper<int>> refs;
  refs.reserve(values.size());

  for (const int v : values)
    refs.emplace_back(heap.insert(v));

  ASSERT_EQ(heap.size(), values.size());

  heap.remove(refs[3]);  // remove an interior node
  heap.remove(refs.back()); // remove what used to be the last inserted node

  std::vector<int> remaining;
  while (!heap.is_empty())
    remaining.push_back(heap.getMin());

  std::vector<int> expected = values;
  const auto erase_value = [&expected](int needle) {
    const auto it = std::find(expected.begin(), expected.end(), needle);
    ASSERT_NE(it, expected.end());
    expected.erase(it);
  };
  erase_value(20);
  erase_value(15);
  std::sort(expected.begin(), expected.end());

  EXPECT_EQ(remaining, expected);
}

TEST(DynBinHeapTest, UpdateReordersAfterPriorityChange)
{
  DynBinHeap<int> heap;
  auto &high = heap.insert(50);
  auto &low = heap.insert(5);
  heap.insert(20);

  high = 1; // becomes the best priority
  heap.update(high);
  EXPECT_EQ(heap.top(), 1);

  low = 60; // now should sink
  heap.update(low);

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  EXPECT_EQ(drained, std::vector<int>({1, 20, 60}));
}

TEST(DynBinHeapTest, RandomizedInsertExtract)
{
  DynBinHeap<int> heap;
  std::vector<int> values(200);
  std::iota(values.begin(), values.end(), -100);
  std::mt19937 rng(42);
  std::shuffle(values.begin(), values.end(), rng);

  for (int v : values)
    heap.insert(v);

  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.getMin());

  std::vector<int> expected = values;
  std::sort(expected.begin(), expected.end());
  EXPECT_EQ(extracted, expected);
}

TEST(DynBinHeapTest, IteratorTraversesAllElements)
{
  DynBinHeap<int> heap;
  constexpr size_t N = 64;
  for (size_t i = 0; i < N; ++i)
    heap.insert(static_cast<int>(i));

  size_t count = 0;
  for (auto it = heap.get_it(); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, N);
}

TEST(DynBinHeapTest, EmptyRemovesAllNodes)
{
  DynBinHeap<int> heap;
  for (int i = 0; i < 50; ++i)
    heap.insert(i);

  heap.empty();
  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0u);

  // Verify heap can be reused after empty
  heap.insert(10);
  EXPECT_EQ(heap.getMin(), 10);
  EXPECT_TRUE(heap.is_empty());
}

TEST(DynBinHeapTest, UpdateWithDuplicatePriorities)
{
  DynBinHeap<int> heap;
  std::vector<std::reference_wrapper<int>> refs;
  for (int i = 0; i < 5; ++i)
    refs.emplace_back(heap.insert(10)); // duplicates

  // Make one element the new minimum
  refs.front().get() = -1;
  heap.update(refs.front());
  EXPECT_EQ(heap.top(), -1);

  // Make another element very large to ensure sift-down works
  refs.back().get() = 100;
  heap.update(refs.back());

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  EXPECT_TRUE(std::is_sorted(drained.begin(), drained.end()));
  EXPECT_EQ(drained.front(), -1);
  EXPECT_EQ(drained.back(), 100);
}

namespace
{
  // Less than; counts its calls, and the call number fail_at throws (0 for
  // none).
  struct Failing_Less
  {
    long * calls = nullptr;
    long fail_at = 0;

    bool operator()(const int & a, const int & b) const
    {
      if (calls != nullptr and ++*calls == fail_at)
        throw std::runtime_error("comparison failed");
      return a < b;
    }
  };

  using Failing_Heap = BinHeap<int, Failing_Less>;

  // Inserts and then extracts values with a comparison that fails at its
  // call fail_at; returns the extracted values
  std::vector<int> insert_and_extract(const std::vector<int> & values,
                                      long & calls, long fail_at)
  {
    Failing_Heap heap(Failing_Less{&calls, fail_at});
    std::vector<int> out;
    try
      {
        for (const int v : values)
          heap.insert(new Failing_Heap::Node(v));

        heap.update(heap.getRoot()); // its key is unchanged

        while (not heap.is_empty())
          { // getMin() throws with the node already out of the heap
            std::unique_ptr<Failing_Heap::Node> min(heap.top());
            heap.getMin();
            out.push_back(min->get_key());
          }
      }
    catch (...)
      {
        heap.remove_all_and_delete();
        throw;
      }
    return out;
  }
} // namespace

// insert(), update() and getMin() were noexcept, so a comparison that
// threw ended the program.
TEST(BinHeapComparison, WhatTheComparisonThrowsReachesTheCaller)
{
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1, 8};
  long calls = 0;
  const auto out = insert_and_extract(values, calls, 0);
  ASSERT_TRUE(std::is_sorted(out.begin(), out.end()));
  ASSERT_EQ(out.size(), values.size());

  for (long k = 1; k <= calls; ++k)
    {
      long count = 0;
      EXPECT_THROW(insert_and_extract(values, count, k), std::runtime_error)
        << "call " << k;
    }
}

// Emptying a heap of up to 3 nodes extracted them one by one, comparing
// the keys left, so in a destructor a comparison that throws ended the
// program.
TEST(BinHeapComparison, RemoveAllAndDeleteDoesNotCompare)
{
  for (int n = 1; n <= 6; ++n)
    {
      long calls = 0;
      Failing_Heap heap(Failing_Less{&calls, 0});
      for (int i = 0; i < n; ++i)
        heap.insert(new Failing_Heap::Node(n - i));

      const long before = calls;
      heap.remove_all_and_delete();
      EXPECT_EQ(calls, before) << n << " nodes";
      EXPECT_TRUE(heap.is_empty());

      heap.insert(new Failing_Heap::Node(4)); // the heap can be used again
      EXPECT_EQ(heap.top()->get_key(), 4);
      heap.remove_all_and_delete();
    }
}

// remove() reset the node after it compared the keys, so a node that a
// remove() that threw had taken out of the heap could not be inserted
// again.
TEST(BinHeapComparison, RemoveTakesTheNodeOutEvenIfTheComparisonThrows)
{
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1, 8, 4, 6};
  std::vector<int> sorted = values;
  std::sort(sorted.begin(), sorted.end());

  for (size_t i = 0; i < values.size(); ++i)
    for (long k = 1; ; ++k)
      {
        long calls = 0;
        Failing_Heap heap(Failing_Less{&calls, 0});
        std::vector<Failing_Heap::Node *> nodes;
        for (const int v : values)
          nodes.push_back(heap.insert(new Failing_Heap::Node(v)));

        heap.key_comp().fail_at = calls + k; // the call k of remove()
        bool threw = false;
        try
          {
            heap.remove(nodes[i]);
          }
        catch (const std::runtime_error &)
          {
            threw = true;
          }

        heap.key_comp().fail_at = 0;
        heap.insert(nodes[i]); // it is out of the heap either way
        std::vector<int> out;
        while (not heap.is_empty())
          {
            std::unique_ptr<Failing_Heap::Node> min(heap.getMin());
            out.push_back(min->get_key());
          }
        std::sort(out.begin(), out.end()); // the heap may be out of order
        EXPECT_EQ(out, sorted) << "node " << i << ", call " << k;

        if (not threw)
          break;
      }
}

namespace
{
  // An int that counts the objects alive
  struct Counted
  {
    static inline long alive = 0;
    int value;

    Counted(int v = 0) noexcept : value(v) { ++alive; }
    Counted(const Counted & c) noexcept : value(c.value) { ++alive; }
    ~Counted() { --alive; }
  };

  // Less than of Counted, with the failures of Failing_Less
  struct Failing_Counted_Less
  {
    Failing_Less less;

    bool operator()(const Counted & a, const Counted & b) const
    {
      return less(a.value, b.value);
    }
  };

  using Failing_Dyn_Heap = DynBinHeap<Counted, Failing_Counted_Less>;

  // Inserts the values in heap, whose comparison fails at some call, copies
  // the heap, updates, removes and erases elements, and extracts the rest.
  // If the comparison throws, then after disarm() the heap must still hold
  // as many elements as it says.
  template <class Heap, class Disarm>
  void use_dyn_heap(Heap & heap, const std::vector<int> & values,
                    Disarm disarm)
  {
    using Item = typename Heap::Item_Type;
    try
      {
        std::vector<Item *> items;
        for (const int v : values)
          items.push_back(&heap.insert(Item(v)));

        Heap copy(heap);
        copy = heap;

        heap.update(*items[2]); // its key is unchanged
        heap.remove(*items[3]);
        heap.erase(*items[5]);
        while (not heap.is_empty())
          (void) heap.getMin();
      }
    catch (...)
      {
        disarm();
        const size_t size = heap.size();
        size_t extracted = 0;
        for (; not heap.is_empty(); ++extracted)
          (void) heap.getMin();
        EXPECT_EQ(extracted, size);
        throw;
      }
  }

  // With a comparison that fails at its call fail_at
  void use_dyn_heap(const std::vector<int> & values, long & calls,
                    long fail_at)
  {
    Failing_Dyn_Heap heap(Failing_Counted_Less{{&calls, fail_at}});
    use_dyn_heap(heap, values, [&heap] { heap.key_comp().less.fail_at = 0; });
  }
} // namespace

// insert(), update(), remove() and erase() of DynBinHeap were noexcept, so
// a comparison that threw ended the program. When it threw, getMin() and
// remove() also lost the node that they had taken out of the heap, and the
// copy constructor the nodes that it had copied.
TEST(DynBinHeapComparison, WhatTheComparisonThrowsReachesTheCaller)
{
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1, 8};
  const long alive = Counted::alive;
  long calls = 0;
  use_dyn_heap(values, calls, 0);
  ASSERT_EQ(Counted::alive, alive);

  for (long k = 1; k <= calls; ++k)
    {
      long count = 0;
      EXPECT_THROW(use_dyn_heap(values, count, k), std::runtime_error)
        << "call " << k;
      EXPECT_EQ(Counted::alive, alive) << "call " << k;
    }
}

// They are noexcept when the comparison is
static_assert(noexcept(std::declval<DynBinHeap<int> &>()
                       .update(std::declval<int &>())));
static_assert(noexcept(std::declval<DynBinHeap<int> &>()
                       .remove(std::declval<int &>())));
static_assert(noexcept(std::declval<DynBinHeap<int> &>()
                       .erase(std::declval<int &>())));
static_assert(not noexcept(std::declval<Failing_Dyn_Heap &>()
                           .update(std::declval<Counted &>())));
static_assert(not noexcept(std::declval<Failing_Dyn_Heap &>()
                           .remove(std::declval<Counted &>())));
static_assert(not noexcept(std::declval<Failing_Dyn_Heap &>()
                           .erase(std::declval<Counted &>())));

// The insertions allocate the node, so they never were noexcept
static_assert(not noexcept(std::declval<DynBinHeap<int> &>()
                           .insert(std::declval<const int &>())));
static_assert(not noexcept(std::declval<DynBinHeap<int> &>()
                           .put(std::declval<const int &>())));
static_assert(not noexcept(std::declval<DynBinHeap<int> &>()
                           .append(std::declval<const int &>())));

namespace
{
  // A Counted whose < fails as Failing_Less does, and does not say
  // noexcept; a heap compares it with its default comparison, Aleph::less
  struct Failing_Key : Counted
  {
    using Counted::Counted;

    static inline Failing_Less less;

    bool operator<(const Failing_Key & other) const
    {
      return less(value, other.value);
    }
  };

  using Failing_Key_Heap = DynBinHeap<Failing_Key>;

  // With keys whose < fails at its call fail_at
  void use_key_heap(const std::vector<int> & values, long & calls,
                    long fail_at)
  {
    struct Reset
    {
      ~Reset() { Failing_Key::less = Failing_Less{}; }
    } reset;
    Failing_Key::less = Failing_Less{&calls, fail_at};
    Failing_Key_Heap heap;
    use_dyn_heap(heap, values, [] { Failing_Key::less.fail_at = 0; });
  }
} // namespace

// Aleph::less is noexcept as the < of its type, and so are the operations
// of a heap that compares with it. Counted has no <, and its heap, which
// compares with another functor, can still be copied: overload resolution
// names DynBinHeap<Counted>, whose Aleph::less must give no error then
static_assert(std::is_copy_constructible_v<Failing_Dyn_Heap>);
static_assert(noexcept(Aleph::less<int>()(0, 0)));
static_assert(not noexcept(Aleph::less<Failing_Key>()(
                std::declval<const Failing_Key &>(),
                std::declval<const Failing_Key &>())));
static_assert(not noexcept(std::declval<Failing_Key_Heap &>()
                           .update(std::declval<Failing_Key &>())));
static_assert(not noexcept(std::declval<Failing_Key_Heap &>()
                           .remove(std::declval<Failing_Key &>())));
static_assert(not noexcept(std::declval<Failing_Key_Heap &>()
                           .erase(std::declval<Failing_Key &>())));

// Aleph::less was noexcept whatever the < of its type, so a heap that
// compared with it ended the program when that < threw
TEST(DynBinHeapComparison, WhatTheLessThanOfTheKeysThrowsReachesTheCaller)
{
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1, 8};
  const long alive = Counted::alive;
  long calls = 0;
  use_key_heap(values, calls, 0);
  ASSERT_EQ(Counted::alive, alive);
  ASSERT_GT(calls, 0);

  for (long k = 1; k <= calls; ++k)
    {
      long count = 0;
      EXPECT_THROW(use_key_heap(values, count, k), std::runtime_error)
        << "call " << k;
      EXPECT_EQ(Counted::alive, alive) << "call " << k;
    }
}

namespace
{
  std::vector<int> drain(DynBinHeap<int> & heap)
  {
    std::vector<int> out;
    while (not heap.is_empty())
      out.push_back(heap.getMin());
    return out;
  }
} // namespace

// swap() exchanged the roots but left each root pointing up to the header
// of the other heap, so the moves of DynBinHeap, which swap, broke the heap.
TEST(DynBinHeapTest, MoveConstructionOutlivesTheSource)
{
  for (int n = 0; n <= 7; ++n)
    {
      std::unique_ptr<DynBinHeap<int>> moved;
      {
        DynBinHeap<int> source;
        for (int i = n; i > 0; --i)
          source.insert(i);
        moved = std::make_unique<DynBinHeap<int>>(std::move(source));
        EXPECT_TRUE(source.is_empty());
        source.insert(10); // the source can be used again
        EXPECT_EQ(source.getMin(), 10);
      }

      std::vector<int> expected(n);
      std::iota(expected.begin(), expected.end(), 1);
      EXPECT_EQ(drain(*moved), expected) << n << " values";
    }
}

TEST(DynBinHeapTest, MoveAssignmentOutlivesTheSource)
{
  DynBinHeap<int> target;
  for (int i : {20, 30})
    target.insert(i);
  {
    DynBinHeap<int> source;
    for (int i : {5, 3, 8, 1, 9, 2})
      source.insert(i);
    target = std::move(source);
  }
  EXPECT_EQ(drain(target), (std::vector<int>{1, 2, 3, 5, 8, 9}));
}

TEST(DynBinHeapTest, SwapExchangesTheContents)
{
  DynBinHeap<int> a, b, spare;
  for (int i : {4, 2, 6, 1})
    a.insert(i);
  for (int i : {7, 3, 5})
    b.insert(i);

  a.swap(b);
  a.insert(0); // both heaps keep working after the swap
  b.insert(10);
  EXPECT_EQ(drain(a), (std::vector<int>{0, 3, 5, 7}));
  EXPECT_EQ(drain(b), (std::vector<int>{1, 2, 4, 6, 10}));

  for (int i : {9, 8})
    a.insert(i);
  a.swap(spare); // with an empty heap
  EXPECT_TRUE(a.is_empty());
  a.insert(1);
  EXPECT_EQ(drain(a), std::vector<int>{1});
  EXPECT_EQ(drain(spare), (std::vector<int>{8, 9}));
}

// ─────────────────────────────────────────────────────────────────
// The representation: a complete tree whose nodes that lack a child are in
// a list
// ─────────────────────────────────────────────────────────────────

namespace
{
  // The parent of a node; that of the root is the header node of its heap
  template <class Node>
  Node * parent_of(Node *p)
  {
    return reinterpret_cast<Node *&>(p->getU());
  }

  // The nodes of a heap by their places, which are numbered by levels from
  // 1 (the entry 0 has the header node), taken through the children of the
  // nodes that have two, and what in the links of those nodes differs from
  // the representation that tpl_binHeap.H describes: the node at the place
  // i has its parent at i / 2 and, if it has them, its children at 2 i and
  // 2 i + 1, and the nodes at the places (n + 1) / 2 to n are in a circular
  // doubly linked list, in that order, with their right link as the next
  // node and their left one as the previous one
  template <class Heap>
  std::string shape_errors(const Heap & heap, std::vector<typename Heap::Node *> & at)
  {
    using Node = typename Heap::Node;
    std::ostringstream err;
    const size_t n = heap.size();
    Node *root = heap.getRoot();
    at.assign(n + 1, nullptr);
    if (n == 0)
      {
        if (root != nullptr)
          err << "an empty heap has a root; ";
        return err.str();
      }
    at[0] = parent_of(root);
    at[1] = root;
    for (size_t i = 1; i <= n; ++i)
      {
        if (2*i <= n)
          at[2*i] = at[i]->getL();
        if (2*i + 1 <= n)
          at[2*i + 1] = at[i]->getR();
      }
    std::vector<Node *> sorted(at.begin() + 1, at.end());
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
      err << "two places have the same node; ";
    if (at[0]->getR() != root)
      err << "the root is not the right child of the header node; ";
    for (size_t i = 1; i <= n; ++i)
      {
        Node *p = at[i];
        if (parent_of(p) != at[i / 2])
          err << "the parent of " << i << "; ";
        if (static_cast<bool>(p->get_control_fields().is_left) != (i > 1 and i % 2 == 0))
          err << "is_left of " << i << "; ";
        if (static_cast<bool>(p->get_control_fields().is_leaf) != (2*i > n))
          err << "is_leaf of " << i << "; ";
      }
    const size_t first = (n + 1) / 2;
    for (size_t i = first; i <= n; ++i)
      {
        if (at[i]->getR() != at[i == n ? first : i + 1])
          err << "the next node of " << i << "; ";
        if (at[i]->getL() != at[i == first ? n : i - 1])
          err << "the previous node of " << i << "; ";
      }
    return err.str();
  }

  // The nodes of a heap by their places, after checking its representation
  template <class Heap>
  std::vector<typename Heap::Node *> places(const Heap & heap)
  {
    std::vector<typename Heap::Node *> at;
    EXPECT_EQ(shape_errors(heap, at), "") << heap.size() << " nodes";
    return at;
  }
}

// After each insertion and each extraction, the links of all the nodes are
// those of the representation
TEST(BinHeapShape, InsertionsAndExtractionsKeepTheTreeAndItsList)
{
  std::mt19937 rng(20261121);
  std::uniform_int_distribution<int> keys(0, 1000);
  BinHeap<int> heap;
  places(heap);
  for (int k = 0; k < 600; ++k)
    {
      heap.insert(new BinHeap<int>::Node(keys(rng)));
      places(heap);
    }
  while (not heap.is_empty())
    {
      delete heap.getMin();
      places(heap);
    }
}

// So they are after the removal of the node at each place of a heap of each
// size, and after the update of that node with a key that takes it to the
// root, and with one that takes it down
TEST(BinHeapShape, RemovalsAndUpdatesKeepTheTreeAndItsList)
{
  std::mt19937 rng(7);
  std::uniform_int_distribution<int> keys(0, 1000);
  for (size_t n = 1; n <= 40; ++n)
    for (size_t i = 1; i <= n; ++i)
      for (int op = 0; op < 3; ++op)
        {
          SCOPED_TRACE(testing::Message() << "n " << n << ", place " << i << ", op " << op);
          BinHeap<int> heap;
          for (size_t k = 0; k < n; ++k)
            heap.insert(new BinHeap<int>::Node(keys(rng)));
          const auto at = places(heap);
          if (op == 0)
            delete heap.remove(at[i]);
          else
            {
              at[i]->get_key() = op == 1 ? -1 : 2000;
              heap.update(at[i]);
              EXPECT_EQ(heap.top(), op == 1 ? at[i] : heap.top());
            }
          places(heap);
          heap.remove_all_and_delete();
        }
}

// The last node of the heap is the one at the place n: its removal compares
// nothing, and that of any other node does
TEST(BinHeapShape, TheLastNodeIsTheOneAtThePlaceN)
{
  std::mt19937 rng(11);
  std::uniform_int_distribution<int> keys(0, 1000);
  for (size_t n = 2; n <= 40; ++n)
    for (size_t i = 1; i <= n; ++i)
      {
        long calls = 0;
        Failing_Heap heap(Failing_Less{&calls, 0});
        for (size_t k = 0; k < n; ++k)
          heap.insert(new Failing_Heap::Node(keys(rng)));
        const auto at = places(heap);
        const long before = calls;
        delete heap.remove(at[i]);
        // the root of a heap of 2 nodes leaves it with one: nothing to compare
        EXPECT_EQ(calls == before, i == n or n == 2) << "n " << n << ", place " << i;
        heap.remove_all_and_delete();
      }
}

// What the description of tpl_binHeap.H says of a heap of 10 nodes
TEST(BinHeapShape, TheHeapOfTenNodesOfTheDescription)
{
  BinHeap<int> heap;
  for (int k = 1; k <= 10; ++k)
    heap.insert(new BinHeap<int>::Node(k)); // in order: the node of key k is at the place k
  const auto at = places(heap);
  for (int k = 1; k <= 10; ++k)
    ASSERT_EQ(at[k]->get_key(), k);

  // 1 to 4 have two children
  for (int k = 1; k <= 4; ++k)
    {
      EXPECT_EQ(at[k]->getL(), at[2*k]);
      EXPECT_EQ(at[k]->getR(), at[2*k + 1]);
    }
  // the list: 5, 6, 7, 8, 9, 10 and 5 again
  for (int k = 5; k <= 10; ++k)
    {
      EXPECT_EQ(at[k]->getR(), at[k == 10 ? 5 : k + 1]);
      EXPECT_EQ(at[k]->getL(), at[k == 5 ? 10 : k - 1]);
    }
  // 5 has a left child only, 10, which is its previous node of the list too
  EXPECT_EQ(at[5]->getL(), at[10]);
  EXPECT_EQ(parent_of(at[10]), at[5]);
  EXPECT_FALSE(static_cast<bool>(at[5]->get_control_fields().is_leaf));
  EXPECT_EQ(at[5]->getR(), at[6]);
  EXPECT_NE(parent_of(at[6]), at[5]);

  // the next node goes under the first one of the list, which follows the last
  BinHeap<int>::Node *p = heap.insert(new BinHeap<int>::Node(11));
  EXPECT_EQ(parent_of(p), at[5]);
  EXPECT_EQ(at[5]->getR(), p);
  EXPECT_EQ(p->getL(), at[10]); // after the last one in the list
  EXPECT_EQ(p->getR(), at[6]);  // 5 left the list: 6 is its first node now
  heap.remove_all_and_delete();
}
