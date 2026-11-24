
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
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
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
          { // insert() throws with the node out of the heap
            std::unique_ptr<Failing_Heap::Node> node(new Failing_Heap::Node(v));
            heap.insert(node.get());
            node.release();
          }

        heap.update(heap.getRoot()); // its key is unchanged

        while (not heap.is_empty())
          { // getMin() throws with the node still in the heap
            std::unique_ptr<Failing_Heap::Node> min(heap.getMin());
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

// A remove() that threw took the node out of the heap, which could be left
// out of order. Now it puts the node back at its place, and the last node
// at the end, so the node stays in the heap, which is as it was.
TEST(BinHeapComparison, RemoveLeavesTheNodeInTheHeapIfTheComparisonThrows)
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
        EXPECT_EQ(heap.size(), threw ? values.size() : values.size() - 1);
        if (not threw)
          heap.insert(nodes[i]); // it is out of the heap only if remove() returned
        EXPECT_TRUE(heap.verify_heap());
        std::vector<int> out;
        while (not heap.is_empty())
          {
            std::unique_ptr<Failing_Heap::Node> min(heap.getMin());
            out.push_back(min->get_key());
          }
        EXPECT_EQ(out, sorted) << "node " << i << ", call " << k; // in order

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
    if (root == nullptr)
      {
        err << "a heap of " << n << " nodes has no root; ";
        return err.str();
      }
    at[0] = parent_of(root);
    at[1] = root;
    for (size_t i = 1; i <= n; ++i)
      {
        if (at[i] == nullptr)
          { // nothing below it can be looked at
            err << "no node at the place " << i << "; ";
            return err.str();
          }
        if (2*i <= n)
          at[2*i] = at[i]->getL();
        if (2*i + 1 <= n)
          at[2*i + 1] = at[i]->getR();
      }
    if (at[0] == nullptr)
      {
        err << "the root has no header node; ";
        return err.str();
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

  // The nodes of a heap by their places, after checking its representation.
  // A heap that does not have it ends the test, with an exception that no
  // test expects: what follows would use places that are not what they say
  template <class Heap>
  std::vector<typename Heap::Node *> places(const Heap & heap)
  {
    std::vector<typename Heap::Node *> at;
    const std::string err = shape_errors(heap, at);
    if (not err.empty())
      {
        ADD_FAILURE() << heap.size() << " nodes: " << err;
        throw std::logic_error("the heap does not have its shape");
      }
    return at;
  }

  // Frees the nodes that are in a heap when the test leaves it, by a fatal
  // failure too. A heap that does not have its shape is not walked: its
  // nodes are left, and the failure that ended the test says why
  template <class Heap>
  struct Freed_At_Exit
  {
    Heap & heap;

    ~Freed_At_Exit()
    {
      std::vector<typename Heap::Node *> at;
      if (shape_errors(heap, at).empty())
        heap.remove_all_and_delete();
    }
  };
}

// After each insertion and each extraction, the links of all the nodes are
// those of the representation
TEST(BinHeapShape, InsertionsAndExtractionsKeepTheTreeAndItsList)
{
  std::mt19937 rng(20261121);
  std::uniform_int_distribution<int> keys(0, 1000);
  BinHeap<int> heap;
  Freed_At_Exit<BinHeap<int>> nodes{heap};
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
          Freed_At_Exit<BinHeap<int>> nodes{heap};
          for (size_t k = 0; k < n; ++k)
            heap.insert(new BinHeap<int>::Node(keys(rng)));
          const auto at = places(heap);
          if (op == 0)
            delete heap.remove(at[i]);
          else
            {
              at[i]->get_key() = op == 1 ? -1 : 2000;
              heap.update(at[i]);
              if (op == 1)
                {
                  EXPECT_EQ(heap.top(), at[i]);
                }
            }
          places(heap);
          EXPECT_TRUE(heap.verify_heap());
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
        Freed_At_Exit<Failing_Heap> nodes{heap};
        for (size_t k = 0; k < n; ++k)
          heap.insert(new Failing_Heap::Node(keys(rng)));
        const auto at = places(heap);
        const long before = calls;
        delete heap.remove(at[i]);
        // the root of a heap of 2 nodes leaves it with one: nothing to compare
        EXPECT_EQ(calls == before, i == n or n == 2) << "n " << n << ", place " << i;
      }
}

// What the description of tpl_binHeap.H says of a heap of 10 nodes
TEST(BinHeapShape, TheHeapOfTenNodesOfTheDescription)
{
  BinHeap<int> heap;
  Freed_At_Exit<BinHeap<int>> nodes{heap};
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
}

// verify_heap() went down by the right child only where there were two, so
// it did not see a node out of order in a left subtree
TEST(BinHeapShape, VerifyHeapLooksAtEveryNode)
{
  for (int n = 1; n <= 40; ++n)
    {
      BinHeap<int> heap;
      Freed_At_Exit<BinHeap<int>> nodes{heap};
      for (int k = 1; k <= n; ++k)
        heap.insert(new BinHeap<int>::Node(10*k)); // the node of key 10 k is at the place k
      const auto at = places(heap);
      EXPECT_TRUE(heap.verify_heap()) << n << " nodes";
      for (int i = 2; i <= n; ++i)
        {
          at[i]->get_key() = 10*(i / 2) - 1; // less than the key of its parent only
          EXPECT_FALSE(heap.verify_heap()) << n << " nodes, place " << i;
          at[i]->get_key() = 10*i;
        }
      EXPECT_TRUE(heap.verify_heap()) << n << " nodes";
    }
}

// ─────────────────────────────────────────────────────────────────
// A comparison that throws leaves the heap as it was
// ─────────────────────────────────────────────────────────────────

namespace
{
  // A key with a priority, which the comparisons look at, and an identity,
  // which they do not: it tells apart the keys of a priority
  struct Pri
  {
    int pri = 0;
    int id = 0;

    bool operator==(const Pri &) const = default;
  };

  void PrintTo(const Pri & x, std::ostream *os) { *os << x.pri << '#' << x.id; }

  // The calls of a Counted_Less: how many, the one that throws (0: none),
  // and the identities of the two keys of each one, in order
  struct Calls
  {
    size_t count = 0;
    size_t throw_at = 0;
    std::vector<std::pair<int, int>> keys;
  };

  struct Counted_Less
  {
    Calls *calls;

    bool operator()(const Pri & a, const Pri & b) const
    {
      if (++calls->count == calls->throw_at)
        throw std::runtime_error("comparison");
      calls->keys.emplace_back(a.id, b.id);
      return a.pri < b.pri;
    }
  };

  using Counted_Heap = BinHeap<Pri, Counted_Less>;

  // The keys of a heap by the places of their nodes
  std::vector<Pri> layout(const Counted_Heap & heap)
  {
    std::vector<Pri> keys;
    const auto at = places(heap);
    for (size_t i = 1; i <= heap.size(); ++i)
      keys.push_back(at[i]->get_key());
    return keys;
  }

  // The heap of 7.7.5 on a vector indexed by the places, whose entry 0 is
  // not used: its sifts compared a node with its parent or its children and
  // exchanged them level by level, and its operations linked and unlinked
  // the nodes before they compared
  struct Old_Heap
  {
    std::vector<Pri> a = std::vector<Pri>(1);
    size_t n = 0;
    Counted_Less cmp;

    size_t sift_up(size_t i)
    {
      for (; i > 1 and cmp(a[i], a[i / 2]); i /= 2)
        std::swap(a[i], a[i / 2]);
      return i;
    }

    size_t sift_down(size_t i)
    {
      while (2*i <= n)
        {
          size_t c = 2*i;
          if (c + 1 <= n and cmp(a[c + 1], a[c]))
            ++c;
          if (cmp(a[i], a[c]))
            break;
          std::swap(a[i], a[c]);
          i = c;
        }
      return i;
    }

    void insert(const Pri & x)
    {
      a.resize(std::max(a.size(), n + 2));
      a[++n] = x;
      sift_up(n);
    }

    Pri getMin()
    {
      const Pri ret = a[1];
      a[1] = a[n];
      if (--n > 0)
        sift_down(1);
      return ret;
    }

    void update(const size_t i) { sift_up(sift_down(i)); }

    void remove(const size_t i)
    {
      if (i == 1)
        {
          getMin();
          return;
        }
      const Pri moved = a[n];
      if (i == n--)
        return;
      a[i] = moved; // the last node takes the place of the removed one
      update(i);
    }

    std::vector<Pri> layout() const { return {a.begin() + 1, a.begin() + 1 + static_cast<long>(n)}; }
  };

  // The links and the flags of a node
  template <class Node>
  auto links_of(Node *p)
  {
    return std::make_tuple(p->getL(), p->getR(), parent_of(p),
                           static_cast<bool>(p->get_control_fields().is_leaf),
                           static_cast<bool>(p->get_control_fields().is_left));
  }
}

// The operations compare the same keys in the same order as those of 7.7.5,
// and leave every node at the same place, those of a same priority too
TEST(BinHeapThrowingCompare, SameComparisonsAndPlacesAsBefore)
{
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> values(0, 50); // many keys of a same priority
  Calls calls, old_calls;
  Counted_Heap heap(Counted_Less{&calls});
  Freed_At_Exit<Counted_Heap> nodes{heap};
  Old_Heap old{.cmp = Counted_Less{&old_calls}};
  size_t updates = 0, removals = 0, extractions = 0;
  for (int k = 0; k < 6000; ++k)
    {
      const unsigned op = old.n == 0 ? 0 : rng() % 8;
      if (op < 4)
        {
          const Pri x{values(rng), k + 1};
          heap.insert(new Counted_Heap::Node(x));
          old.insert(x);
        }
      else if (op == 4)
        {
          std::unique_ptr<Counted_Heap::Node> min(heap.getMin());
          ASSERT_EQ(min->get_key(), old.getMin());
          ++extractions;
        }
      else
        {
          const auto at = places(heap);
          const size_t i = 1 + rng() % old.n;
          if (op == 5)
            {
              std::unique_ptr<Counted_Heap::Node> out(heap.remove(at[i]));
              ASSERT_EQ(out.get(), at[i]);
              old.remove(i);
              ++removals;
            }
          else
            {
              const int pri = values(rng);
              at[i]->get_key().pri = pri;
              old.a[i].pri = pri;
              heap.update(at[i]);
              old.update(i);
              ++updates;
            }
        }
      ASSERT_EQ(calls.keys, old_calls.keys) << "operation " << k;
      calls.keys.clear();
      old_calls.keys.clear();
      ASSERT_EQ(layout(heap), old.layout()) << "operation " << k;
    }
  EXPECT_GT(updates, 1000u);
  EXPECT_GT(removals, 500u);
  EXPECT_GT(extractions, 500u);
  EXPECT_GT(heap.size(), 100u);
}

// Each failed comparison of an insertion, an extraction, a removal or an
// update leaves the heap as it was: every node at its place and with its
// links, and the node of a failed insertion out of the heap, ready to be
// inserted again.
// 7.7.5 left the node of an insertion in a heap out of order, and took out
// that of an extraction or of a removal
TEST(BinHeapThrowingCompare, EveryFailedCallLeavesTheHeapAsItWas)
{
  std::mt19937 rng(20261121);
  std::uniform_int_distribution<int> values(0, 30);
  size_t failed = 0;
  for (size_t n = 0; n <= 40; ++n)
    for (int op = 0; op < 4; ++op) // insert, getMin, remove, update
      for (int round = 0; round < (n < 2 ? 1 : 3); ++round)
        {
          if (op != 0 and n == 0)
            continue;
          std::vector<Pri> keys;
          for (size_t k = 0; k < n; ++k)
            keys.push_back(Pri{values(rng), static_cast<int>(k + 1)});
          const Pri key{values(rng), static_cast<int>(n + 1)};
          const size_t where = n == 0 ? 0 : 1 + rng() % n; // of the removal and of the update
          const int new_pri = values(rng);
          for (size_t at_call = 1; ; ++at_call)
            {
              SCOPED_TRACE(testing::Message() << "n " << n << ", op " << op << ", place "
                           << where << ", call " << at_call);
              Calls calls;
              Counted_Heap heap(Counted_Less{&calls});
              Freed_At_Exit<Counted_Heap> nodes{heap};
              for (const Pri & x : keys)
                heap.insert(new Counted_Heap::Node(x));
              std::vector<Counted_Heap::Node *> at = places(heap);
              if (op == 3)
                at[where]->get_key().pri = new_pri; // the heap is as its user left it
              std::vector<decltype(links_of(at[0]))> links;
              for (size_t i = 1; i <= n; ++i)
                links.push_back(links_of(at[i]));
              std::unique_ptr<Counted_Heap::Node> node(new Counted_Heap::Node(key));

              calls.throw_at = calls.count + at_call;
              bool thrown = false;
              Counted_Heap::Node *out = nullptr;
              try
                {
                  switch (op)
                    {
                    case 0: heap.insert(node.get()); break;
                    case 1: out = heap.getMin(); break;
                    case 2: out = heap.remove(at[where]); break;
                    default: heap.update(at[where]);
                    }
                }
              catch (const std::runtime_error &)
                {
                  thrown = true;
                }
              calls.throw_at = 0;
              if (thrown)
                {
                  ++failed;
                  EXPECT_EQ(out, nullptr);
                  EXPECT_EQ(heap.size(), n);
                  EXPECT_EQ(places(heap), at);
                  for (size_t i = 1; i <= n; ++i)
                    EXPECT_EQ(links_of(at[i]), links[i - 1]) << "place " << i;
                  if (op == 0)
                    { // the node is out, and can be inserted again
                      heap.insert(node.release());
                      EXPECT_EQ(heap.size(), n + 1);
                      places(heap);
                      EXPECT_TRUE(heap.verify_heap());
                    }
                }
              else
                {
                  places(heap);
                  EXPECT_TRUE(heap.verify_heap());
                  if (op == 0)
                    node.release(); // it is in the heap
                  delete out;
                }
              if (not thrown)
                break;
            }
        }
  EXPECT_GT(failed, 1000u);
}

namespace
{
  // What a comparison returns, whose conversion to bool may throw
  struct Touchy_Bool
  {
    bool value;

    operator bool() const { return value; }
  };

  // A comparison that does not throw, but the conversion of its result may
  struct Touchy_Less
  {
    Touchy_Bool operator()(const int a, const int b) const noexcept { return {a < b}; }
  };

  using Touchy_Heap = BinHeap<int, Touchy_Less>;
  using Touchy_Dyn_Heap = DynBinHeap<int, Touchy_Less>;
}

// The operations of a heap are noexcept if its comparison is, with the
// conversion of its result to bool, which they also make
static_assert(noexcept(std::declval<BinHeap<int> &>().insert(nullptr)));
static_assert(noexcept(std::declval<BinHeap<int> &>().getMin_ne()));
static_assert(noexcept(std::declval<BinHeap<int> &>().update(nullptr)));
static_assert(not noexcept(std::declval<Touchy_Heap &>().insert(nullptr)));
static_assert(not noexcept(std::declval<Touchy_Heap &>().getMin_ne()));
static_assert(not noexcept(std::declval<Touchy_Heap &>().update(nullptr)));
static_assert(not noexcept(std::declval<Touchy_Dyn_Heap &>().update(std::declval<int &>())));
static_assert(not noexcept(std::declval<Touchy_Dyn_Heap &>().remove(std::declval<int &>())));
static_assert(not noexcept(std::declval<Failing_Heap &>().insert(nullptr)));

namespace
{
  // What a comparison returns. Its implicit conversion to bool does not
  // throw; the explicit one, which a condition chooses for a result that
  // has no name, may
  struct Sly_Bool
  {
    bool value;
    long *conversions; // those of the conditions, so far
    long throw_at;     // the one that throws (0: none)

    operator bool() const & noexcept { return value; }

    explicit operator bool() &&
    {
      if (++*conversions == throw_at)
        throw std::runtime_error("conversion");
      return value;
    }
  };

  // A comparison that does not throw. For two keys that are not const, as
  // the heap compares those of its nodes, the conversion of its result in
  // a condition may
  struct Sly_Less
  {
    long *conversions;
    long *throw_at;

    bool operator()(const int & a, const int & b) const noexcept { return a < b; }

    Sly_Bool operator()(int & a, int & b) const noexcept { return {a < b, conversions, *throw_at}; }
  };

  using Sly_Heap = BinHeap<int, Sly_Less>;
}

// The conversion that counts is the one that the conditions of the sifts
// make, not the implicit one, which this comparison has without throwing
static_assert(std::is_nothrow_invocable_r_v<bool, Sly_Less &, int &, int &>);
static_assert(not noexcept(std::declval<Sly_Heap &>().insert(nullptr)));
static_assert(not noexcept(std::declval<Sly_Heap &>().getMin_ne()));
static_assert(not noexcept(std::declval<Sly_Heap &>().update(nullptr)));

// With that comparison the operations said that they were noexcept, and
// the conversion that threw in a sift ended the program. Now what it
// throws reaches the caller, from a heap that is as it was
TEST(BinHeapThrowingCompare, AConversionInAConditionThatThrowsIsUndone)
{
  size_t failed = 0;
  for (int op = 0; op < 4; ++op) // insert, getMin, remove, update
    for (long at_conversion = 1; ; ++at_conversion)
      {
        SCOPED_TRACE(testing::Message() << "op " << op << ", conversion " << at_conversion);
        long conversions = 0, throw_at = 0;
        Sly_Heap heap(Sly_Less{&conversions, &throw_at});
        Freed_At_Exit<Sly_Heap> nodes{heap};
        for (const int key : {10, 20, 30, 40, 50, 60, 70})
          heap.insert(new Sly_Heap::Node(key));
        const auto at = places(heap);
        std::vector<decltype(links_of(at[0]))> links;
        for (size_t i = 1; i <= 7; ++i)
          links.push_back(links_of(at[i]));
        std::unique_ptr<Sly_Heap::Node> node(new Sly_Heap::Node(5)); // it goes up to the root
        if (op == 3)
          at[7]->get_key() = 1; // the last node goes up to the root too

        conversions = 0;
        throw_at = at_conversion;
        bool thrown = false;
        Sly_Heap::Node *out = nullptr;
        try
          {
            switch (op)
              {
              case 0: heap.insert(node.get()); break;
              case 1: out = heap.getMin(); break;
              case 2: out = heap.remove(at[2]); break;
              default: heap.update(at[7]);
              }
          }
        catch (const std::runtime_error &)
          {
            thrown = true;
          }
        throw_at = 0;
        if (not thrown)
          {
            places(heap);
            if (op == 0)
              {
                EXPECT_EQ(heap.top(), node.release()); // it is in the heap, at the root
              }
            if (op == 3)
              {
                EXPECT_EQ(heap.top(), at[7]);
              }
            delete out;
            break;
          }
        ++failed;
        EXPECT_EQ(out, nullptr);
        EXPECT_EQ(places(heap), at);
        for (size_t i = 1; i <= 7; ++i)
          EXPECT_EQ(links_of(at[i]), links[i - 1]) << "place " << i;
      }
  EXPECT_GT(failed, 8u);
}

// Each failed comparison of an insertion, an extraction, a removal or an
// update of a DynBinHeap leaves it as it was: the same nodes at the same
// places, and as many items alive as before the call. 7.7.5 left the new
// item in a heap out of order, and destroyed that of an extraction or of a
// removal
TEST(DynBinHeapComparison, EveryFailedCallLeavesTheHeapAsItWas)
{
  using Base = BinHeap<Counted, Failing_Counted_Less>;
  std::mt19937 rng(20261121);
  std::uniform_int_distribution<int> values(0, 30);
  const long alive_at_start = Counted::alive;
  size_t failed = 0;
  for (size_t n = 0; n <= 24; ++n)
    for (int op = 0; op < 5; ++op) // insert of a copy and of a move, getMin, remove, update
      {
        if (op >= 2 and n == 0)
          continue;
        std::vector<int> keys;
        for (size_t k = 0; k < n; ++k)
          keys.push_back(values(rng));
        const int key = values(rng);
        const size_t where = n == 0 ? 0 : 1 + rng() % n;
        const int new_key = values(rng);
        for (long at_call = 1; ; ++at_call)
          {
            SCOPED_TRACE(testing::Message() << "n " << n << ", op " << op << ", place "
                         << where << ", call " << at_call);
            long calls = 0;
            bool thrown = false;
            {
              Failing_Dyn_Heap heap(Failing_Counted_Less{{&calls, 0}});
              for (const int k : keys)
                heap.insert(Counted(k));
              const auto at = places(static_cast<const Base &>(heap));
              if (op == 4)
                at[where]->get_key().value = new_key; // the heap is as its user left it
              const Counted item(key);
              const long alive = Counted::alive;

              heap.key_comp().less.fail_at = calls + at_call;
              try
                {
                  switch (op)
                    {
                    case 0: heap.insert(item); break;
                    case 1: heap.insert(Counted(key)); break;
                    case 2: (void) heap.getMin(); break;
                    case 3: heap.remove(at[where]->get_key()); break;
                    default: heap.update(at[where]->get_key());
                    }
                }
              catch (const std::runtime_error &)
                {
                  thrown = true;
                }
              heap.key_comp().less.fail_at = 0;
              if (thrown)
                {
                  ++failed;
                  EXPECT_EQ(heap.size(), n);
                  EXPECT_EQ(places(static_cast<const Base &>(heap)), at);
                  EXPECT_EQ(Counted::alive, alive);
                }
              else
                {
                  places(static_cast<const Base &>(heap));
                  EXPECT_TRUE(heap.verify_heap());
                }
            }
            EXPECT_EQ(Counted::alive, alive_at_start); // the heap freed every item
            if (not thrown)
              break;
          }
      }
  EXPECT_GT(failed, 200u);
}

namespace
{
  // A comparison that contradicts itself: each call gives what a generator
  // of random bits says, whatever the keys
  struct Random_Less
  {
    std::mt19937 *rng;

    bool operator()(const int, const int) const { return (*rng)() & 1; }
  };
}

// With a comparison that contradicts itself the heap cannot be in order,
// but its tree and the list of its nodes that lack a child are well formed
// after each operation, and no node is lost: the ways that the operations
// find never leave the tree
TEST(BinHeapShape, AComparisonThatContradictsItselfKeepsTheTree)
{
  std::mt19937 bits(99), rng(3);
  using Heap = BinHeap<int, Random_Less>;
  Heap heap(Random_Less{&bits});
  Freed_At_Exit<Heap> nodes{heap};
  std::vector<std::unique_ptr<Heap::Node>> out; // those that left the heap
  size_t removals = 0, updates = 0;
  for (int k = 0; k < 20000; ++k)
    {
      unsigned op = heap.is_empty() ? 0 : rng() % 7;
      if (op < 3 and heap.size() >= 150)
        op = 3; // the heap stays small, so that looking at all of it is cheap
      const size_t size = heap.size();
      if (op < 3)
        heap.insert(new Heap::Node(k));
      else if (op == 3)
        out.emplace_back(heap.getMin());
      else
        {
          std::vector<Heap::Node *> at;
          ASSERT_EQ(shape_errors(heap, at), "");
          Heap::Node *p = at[1 + rng() % size];
          if (op == 4)
            {
              Heap::Node *removed = heap.remove(p);
              out.emplace_back(removed);
              ASSERT_EQ(removed, p);
              ++removals;
            }
          else
            {
              heap.update(p);
              ++updates;
            }
        }
      std::vector<Heap::Node *> at;
      ASSERT_EQ(shape_errors(heap, at), "") << "operation " << k;
      ASSERT_EQ(heap.size(), op < 3 ? size + 1 : op <= 4 ? size - 1 : size);
      out.clear();
    }
  EXPECT_GT(removals, 1000u);
  EXPECT_GT(updates, 1000u);
}

namespace
{
  // Its default construction throws while fail is true
  struct Fragile_Default_Key
  {
    static inline bool fail = false;
    int value = 0;

    Fragile_Default_Key()
    {
      if (fail)
        throw std::runtime_error("default construction failed");
    }

    Fragile_Default_Key(int v) noexcept : value(v) {}

    bool operator < (const Fragile_Default_Key & other) const noexcept
    {
      return value < other.value;
    }
  };
} // namespace

// The constructors of the heaps were noexcept, but they build the header
// node, whose key is built by default: a key whose default construction
// throws, as one that allocates, ended the program
TEST(BinHeapConstruction, ItsNoexceptFollowsTheDefaultConstructionOfTheKey)
{
  static_assert(std::is_nothrow_default_constructible_v<BinHeap<int>>);
  static_assert(std::is_nothrow_default_constructible_v<BinHeapVtl<int>>);
  static_assert(std::is_nothrow_default_constructible_v<DynBinHeap<int>>);
  static_assert(std::is_nothrow_default_constructible_v<DynBinHeap<std::string>>);
  static_assert(not std::is_nothrow_default_constructible_v<BinHeap<Fragile_Default_Key>>);
  static_assert(not std::is_nothrow_default_constructible_v<BinHeapVtl<Fragile_Default_Key>>);
  static_assert(not std::is_nothrow_default_constructible_v<DynBinHeap<Fragile_Default_Key>>);

  Fragile_Default_Key::fail = true;
  EXPECT_THROW({ BinHeap<Fragile_Default_Key> heap; }, std::runtime_error);
  EXPECT_THROW({ DynBinHeap<Fragile_Default_Key> heap; }, std::runtime_error);
  Aleph::less<Fragile_Default_Key> cmp;
  EXPECT_THROW({ DynBinHeap<Fragile_Default_Key> heap(cmp); }, std::runtime_error);
  Fragile_Default_Key::fail = false;

  DynBinHeap<Fragile_Default_Key> heap;
  for (int i = 5; i > 0; --i)
    heap.insert(Fragile_Default_Key(i));
  ASSERT_EQ(heap.size(), 5u);
  EXPECT_EQ(heap.top().value, 1);
}

namespace
{
  // A comparison with two copies: the one from a comparison that is not
  // const may throw, and the one from a const one cannot
  struct Mutable_Copy_Less
  {
    static inline bool fail = false;

    Mutable_Copy_Less() noexcept = default;
    Mutable_Copy_Less(const Mutable_Copy_Less &) noexcept = default;

    Mutable_Copy_Less(Mutable_Copy_Less &)
    {
      if (fail)
        throw std::runtime_error("the copy of the comparison");
    }

    Mutable_Copy_Less(Mutable_Copy_Less &&) noexcept = default;
    Mutable_Copy_Less & operator = (const Mutable_Copy_Less &) noexcept = default;
    Mutable_Copy_Less & operator = (Mutable_Copy_Less &&) noexcept = default;

    bool operator () (const int a, const int b) const noexcept { return a < b; }
  };
}

// The heap takes its comparison by value and copies it, from a comparison
// that is not const: its noexcept follows that copy, and what it throws
// reaches the caller
TEST(BinHeapConstruction, ItsNoexceptFollowsTheCopyOfTheComparisonThatItTakes)
{
  using Less = Mutable_Copy_Less;
  using Heap = BinHeap<int, Less>;
  static_assert(std::is_nothrow_copy_constructible_v<Less>);
  static_assert(not std::is_nothrow_constructible_v<Less, Less &>);
  static_assert(not std::is_nothrow_constructible_v<Heap, Less>);
  static_assert(std::is_nothrow_constructible_v<BinHeap<int>, Aleph::less<int>>);

  Less::fail = true;
  EXPECT_THROW({ Heap heap{Less()}; }, std::runtime_error);
  Less::fail = false;

  Heap heap{Less()};
  EXPECT_TRUE(heap.is_empty());
}

namespace
{
  // A comparison whose default construction may throw, when asked
  struct Default_Throws_Less
  {
    static inline bool fail = false;

    Default_Throws_Less()
    {
      if (fail)
        throw std::runtime_error("the default construction of the comparison");
    }

    bool operator () (const int a, const int b) const noexcept { return a < b; }
  };
}

// BinHeap and BinHeapVtl inherit the constructors of GenBinHeap: had the
// comparison been a default argument of the one that takes it, their
// implicit default constructor would be noexcept without counting it, and
// what its construction throws would end the program
TEST(BinHeapConstruction, BuiltByDefaultItLetsTheFailureOfTheComparisonThrough)
{
  using Less = Default_Throws_Less;
  using Heap = BinHeap<int, Less>;
  using Heap_Vtl = BinHeapVtl<int, Less>;
  using Dyn_Heap = DynBinHeap<int, Less>;
  static_assert(not std::is_nothrow_default_constructible_v<Heap>);
  static_assert(not std::is_nothrow_default_constructible_v<Heap_Vtl>);
  static_assert(not std::is_nothrow_default_constructible_v<Dyn_Heap>);

  Less::fail = true;
  EXPECT_THROW({ Heap heap; }, std::runtime_error);
  EXPECT_THROW({ Heap_Vtl heap; }, std::runtime_error);
  EXPECT_THROW({ Dyn_Heap heap; }, std::runtime_error);
  Less::fail = false;

  Heap heap;
  EXPECT_TRUE(heap.is_empty());
}
