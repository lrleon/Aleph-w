
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
 * @file dynarrayheap.cc
 * @brief Tests for Dynarrayheap
 */

#include <gtest/gtest.h>

#include <tpl_dynArrayHeap.H>

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <queue>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  template <typename Heap>
  std::vector<typename Heap::Item_Type> drain(Heap & h)
  {
    std::vector<typename Heap::Item_Type> out;
    while (!h.is_empty())
      out.push_back(h.getMin());
    return out;
  }

  struct Greater
  {
    bool operator()(int a, int b) const noexcept { return a > b; }
  };

} // namespace

TEST(DynArrayHeapBasics, EmptyAndSize)
{
  DynArrayHeap<int> heap;
  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0U);
}

TEST(DynArrayHeapBasics, TopAndGetMinThrowOnEmpty)
{
  DynArrayHeap<int> heap;
  EXPECT_THROW((void)heap.top(), std::underflow_error);
  EXPECT_THROW((void)static_cast<const DynArrayHeap<int> &>(heap).top(), std::underflow_error);
  EXPECT_THROW((void)heap.getMin(), std::underflow_error);
}

TEST(DynArrayHeapBasics, InsertAndExtractInOrder)
{
  DynArrayHeap<int> heap;
  heap.insert(5);
  heap.insert(2);
  heap.insert(4);
  heap.insert(1);

  EXPECT_EQ(heap.size(), 4U);
  EXPECT_EQ(heap.top(), 1);

  EXPECT_EQ(heap.getMin(), 1);
  EXPECT_EQ(heap.get(), 2);
  EXPECT_EQ(heap.getMax(), 4);
  EXPECT_EQ(heap.getMin(), 5);
  EXPECT_TRUE(heap.is_empty());
}

TEST(DynArrayHeapBasics, ReserveRejectsShrinkingBelowSize)
{
  DynArrayHeap<int> heap;
  heap.insert(3);
  heap.insert(1);
  EXPECT_THROW(heap.reserve(1), std::out_of_range);
}

TEST(DynArrayHeapBasics, InsertDirectBehavesLikeInsert)
{
  DynArrayHeap<int> heap;
  heap.reserve(16);
  heap.insert(10);
  heap.insert_direct(1);
  heap.insert_direct(5);
  EXPECT_EQ(heap.top(), 1);

  auto drained = drain(heap);
  EXPECT_TRUE(std::is_sorted(drained.begin(), drained.end()));
}

TEST(DynArrayHeapBasics, PutAndAppendAliasesWork)
{
  DynArrayHeap<int> heap;
  heap.put(3);
  heap.append(2);
  heap.put(1);
  EXPECT_EQ(heap.top(), 1);
}

TEST(DynArrayHeapCompare, GreaterMakesMaxHeap)
{
  DynArrayHeap<int, Greater> heap;
  heap.insert(1);
  heap.insert(10);
  heap.insert(3);
  EXPECT_EQ(heap.top(), 10);
  EXPECT_EQ(heap.getMin(), 10);
  EXPECT_EQ(heap.getMin(), 3);
  EXPECT_EQ(heap.getMin(), 1);
}

TEST(DynArrayHeapIterator, TraverseVisitsAllElements)
{
  DynArrayHeap<int> heap;
  for (int i = 1; i <= 10; ++i)
    heap.insert(i);

  std::vector<int> visited;
  struct Op
  {
    std::vector<int> *v;
    bool operator()(int x)
    {
      v->push_back(x);
      return true;
    }
  };

  Op op{&visited};
  EXPECT_TRUE(heap.traverse(op));
  EXPECT_EQ(visited.size(), 10U);
}

TEST(DynArrayHeapIterator, TraverseStopsEarly)
{
  DynArrayHeap<int> heap;
  for (int i = 1; i <= 10; ++i)
    heap.insert(i);

  int count = 0;
  struct Op
  {
    int *c;
    bool operator()(int)
    {
      ++(*c);
      return *c < 3;
    }
  };

  Op op{&count};
  EXPECT_FALSE(heap.traverse(op));
  EXPECT_EQ(count, 3);
}

TEST(DynArrayHeapRandomized, MatchesStdPriorityQueueMinHeap)
{
  std::mt19937_64 rng(0xD15EA5EULL);
  std::uniform_int_distribution<int> op_dist(0, 99);
  std::uniform_int_distribution<int> val_dist(-10000, 10000);

  DynArrayHeap<int> heap;
  std::priority_queue<int, std::vector<int>, std::greater<int>> ref;

  constexpr int ops = 30000;
  for (int i = 0; i < ops; ++i)
    {
      const int op = op_dist(rng);
      if (op < 60)
        {
          const int v = val_dist(rng);
          heap.insert(v);
          ref.push(v);
        }
      else
        {
          if (ref.empty())
            {
              EXPECT_TRUE(heap.is_empty());
              EXPECT_THROW((void)heap.getMin(), std::underflow_error);
            }
          else
            {
              ASSERT_FALSE(heap.is_empty());
              EXPECT_EQ(heap.top(), ref.top());
              EXPECT_EQ(heap.getMin(), ref.top());
              ref.pop();
            }
        }

      EXPECT_EQ(heap.size(), ref.size());
      EXPECT_EQ(heap.is_empty(), ref.empty());
      if (!ref.empty())
        EXPECT_EQ(heap.top(), ref.top());
    }

  while (!ref.empty())
    {
      ASSERT_FALSE(heap.is_empty());
      EXPECT_EQ(heap.top(), ref.top());
      EXPECT_EQ(heap.getMin(), ref.top());
      ref.pop();
    }
  EXPECT_TRUE(heap.is_empty());
}

namespace
{
  // A comparison with an overload for items that are not constant, which
  // is noexcept, and another one for constant items, which is not
  struct Two_Overloads
  {
    size_t *plain_calls;
    size_t *const_calls;

    bool operator()(int & a, int & b) noexcept
    {
      ++*plain_calls;
      return a < b;
    }

    bool operator()(const int & a, const int & b) const
    {
      ++*const_calls;
      return a < b;
    }
  };

  // An item whose operator & does not give its address
  struct No_Address
  {
    int v = 0;

    No_Address() = default;
    No_Address(const int x) noexcept : v(x) {}
    No_Address *operator&() noexcept { return nullptr; }
    const No_Address *operator&() const noexcept { return nullptr; }
    bool operator<(const No_Address & o) const noexcept { return v < o.v; }
  };

  // An item whose move construction from a 9 throws while armed. Its move
  // assignment does not throw
  struct Hard_To_Build
  {
    static inline bool armed = false;

    int v = 0;

    Hard_To_Build() = default;
    Hard_To_Build(const int x) noexcept : v(x) {}
    Hard_To_Build(const Hard_To_Build &) = default;
    Hard_To_Build & operator=(const Hard_To_Build &) = default;
    Hard_To_Build(Hard_To_Build && o) : v(o.v)
    {
      if (armed and v == 9)
        throw std::runtime_error("move construction");
    }
    Hard_To_Build & operator=(Hard_To_Build && o) noexcept
    {
      v = o.v;
      return *this;
    }
    bool operator<(const Hard_To_Build & o) const noexcept { return v < o.v; }
  };

  // Compares the first of a pair, its priority, and not the second, its
  // identity
  struct First_Less
  {
    bool operator()(const std::pair<int, int> & a, const std::pair<int, int> & b) const noexcept
    {
      return a.first < b.first;
    }
  };

  // The array of the heap, in its order, as traverse() and the iterator
  // see it
  template <class Heap>
  std::vector<typename Heap::Item_Type> layout(const Heap & h)
  {
    using T = typename Heap::Item_Type;
    std::vector<T> by_traverse;
    h.traverse([&by_traverse](const T & x) { by_traverse.push_back(x); return true; });
    std::vector<T> by_iterator;
    for (typename Heap::Iterator it(h); it.has_curr(); it.next_ne())
      by_iterator.push_back(it.get_curr());
    EXPECT_EQ(by_traverse, by_iterator);
    return by_traverse;
  }
}

namespace
{
  // Every way of going through a heap gives its items, in the order of
  // traverse(), and no more; and each member of the iterator takes the
  // number of items as the end
  template <class Heap>
  void expect_iterated(Heap & h)
  {
    using T = typename Heap::Item_Type;
    std::vector<T> items;
    h.traverse([&items](const T & x) { items.push_back(x); return true; });
    ASSERT_EQ(items.size(), h.size());
    const long n = static_cast<long>(items.size());

    typename Heap::Iterator it(h);
    for (long k = 0; k < n; ++k, it.next())
      {
        ASSERT_TRUE(it.has_curr());
        EXPECT_EQ(it.get_pos(), k);
        EXPECT_EQ(it.get_curr(), items[k]);
        EXPECT_EQ(it.get_curr_ne(), items[k]);
        EXPECT_EQ(it.is_last(), k == n - 1);
      }
    EXPECT_FALSE(it.has_curr()); // at the end
    EXPECT_FALSE(it.is_last());
    EXPECT_EQ(it.get_pos(), n);
    EXPECT_THROW((void) it.get_curr(), std::overflow_error);
    EXPECT_THROW(it.next(), std::overflow_error);

    for (long k = n - 1; k >= 0; --k)
      {
        it.prev();
        ASSERT_TRUE(it.has_curr());
        EXPECT_EQ(it.get_curr(), items[k]);
      }
    it.prev(); // before the first item
    EXPECT_FALSE(it.has_curr());
    EXPECT_THROW((void) it.get_curr(), std::underflow_error);
    EXPECT_THROW(it.prev(), std::underflow_error);

    it.reset_last();
    EXPECT_EQ(it.has_curr(), n > 0);
    EXPECT_EQ(it.is_last(), n > 0);
    EXPECT_EQ(it.get_pos(), n - 1);
    it.reset_first();
    EXPECT_EQ(it.has_curr(), n > 0);
    EXPECT_EQ(it.get_pos(), 0);
    it.end();
    EXPECT_FALSE(it.has_curr());
    EXPECT_EQ(it.get_pos(), n);
    for (long k = 0; k < n; ++k)
      {
        it.set_pos(k);
        EXPECT_EQ(it.get_curr(), items[k]);
      }

    std::vector<T> seen;
    for (const T & x : h)
      seen.push_back(x);
    EXPECT_EQ(seen, items);
    seen.clear();
    for (const T & x : std::as_const(h))
      seen.push_back(x);
    EXPECT_EQ(seen, items);
    seen.clear();
    for (auto i = h.get_it(); i.has_curr(); i.next_ne())
      seen.push_back(i.get_curr());
    EXPECT_EQ(seen, items);
    seen.clear();
    for (auto i = std::as_const(h).get_it(); i.has_curr(); i.next())
      seen.push_back(i.get_curr());
    EXPECT_EQ(seen, items);
  }
}

// The iterator goes through the items of the heap, the entries 1 to size()
// of its array, whatever the array has beyond them. It went to the end of
// the array: after reserve(10) and three insert_direct() it visited 9
// entries, and reset_first() left it on the entry 0, which holds no item
TEST(DynArrayHeapIterator, GoesThroughTheItemsWhateverTheArrayHas)
{
  DynArrayHeap<int> heap;
  expect_iterated(heap); // never used

  heap.reserve(10);
  expect_iterated(heap); // entries and no item
  for (const int x : {5, 1, 3})
    heap.insert_direct(x);
  EXPECT_EQ(layout(heap), (std::vector<int>{1, 5, 3}));
  expect_iterated(heap); // more entries than items

  for (int x = 20; x > 0; --x)
    heap.insert(x); // beyond the reserved entries
  expect_iterated(heap);

  while (heap.size() > 4)
    (void) heap.getMin();
  expect_iterated(heap);

  while (not heap.is_empty())
    (void) heap.getMin();
  expect_iterated(heap); // emptied: the array keeps its entry 0
}

// An iterator built without a heap has no item
TEST(DynArrayHeapIterator, WithoutAHeapItHasNoItem)
{
  DynArrayHeap<int>::Iterator it;
  EXPECT_FALSE(it.has_curr());
  EXPECT_FALSE(it.is_last());
  EXPECT_THROW((void) it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  it.reset_last();
  EXPECT_FALSE(it.has_curr());
  it.end();
  EXPECT_FALSE(it.has_curr());
}

// An insertion by move makes the place of the key before it moves it. If
// that move throws, the heap is as it was for the iterator too, which saw
// the new entry of the array as one item more
TEST(DynArrayHeapIterator, DoesNotSeeThePlaceOfAKeyThatWasNotMoved)
{
  DynArrayHeap<Hard_To_Build> heap;
  for (const int x : {1, 5, 3})
    heap.insert(Hard_To_Build(x));

  Hard_To_Build key(9);
  Hard_To_Build::armed = true;
  EXPECT_THROW(heap.insert(std::move(key)), std::runtime_error);
  Hard_To_Build::armed = false;
  EXPECT_EQ(key.v, 9);
  EXPECT_EQ(heap.size(), 3U);
  std::vector<int> seen;
  for (DynArrayHeap<Hard_To_Build>::Iterator it(heap); it.has_curr(); it.next_ne())
    seen.push_back(it.get_curr().v);
  EXPECT_EQ(seen, (std::vector<int>{1, 5, 3}));
}

// The noexcept specification of the sifts looks at the comparison of two
// items that are not constant. The heap must call that overload alone, as
// 7.7.4 did: another one that threw would end the program
TEST(DynArrayHeapCompare, ComparesItemsThatAreNotConstant)
{
  static_assert(not NothrowPredicateWith<Two_Overloads &, int &, const int &>);
  static_assert(noexcept(sift_up(std::declval<DynArray<int> &>(), size_t(), size_t(),
                                 std::declval<Two_Overloads &>())));
  static_assert(noexcept(sift_down(std::declval<DynArray<int> &>(), size_t(), size_t(),
                                   std::declval<Two_Overloads &>())));

  size_t plain_calls = 0, const_calls = 0;
  DynArrayHeap<int, Two_Overloads> heap(Two_Overloads{&plain_calls, &const_calls});
  heap.reserve(64);
  std::vector<int> expected;
  for (int k = 0; k < 40; ++k)
    {
      const int key = (k * 7) % 11;
      switch (k % 4)
        {
        case 0: heap.insert(key); break;
        case 1: heap.insert(int(key)); break;
        case 2: heap.insert_direct(key); break;
        default: heap.insert_direct(int(key));
        }
      expected.push_back(key);
    }
  std::sort(expected.begin(), expected.end());
  EXPECT_EQ(drain(heap), expected);
  EXPECT_GT(plain_calls, 0U);
  EXPECT_EQ(const_calls, 0U);
}

// The sifts keep the addresses of the items that they pass: the real ones,
// whatever the operator & of the items gives
TEST(DynArrayHeapBasics, ItemsWithTheirOwnOperatorAmpersand)
{
  DynArrayHeap<No_Address> heap;
  for (int k = 0; k < 50; ++k)
    if (const No_Address key((k * 37) % 50); k % 2 == 0) // each of 0 to 49, once
      heap.insert(key);
    else
      heap.insert(No_Address(key));
  for (int k = 0; k < 50; ++k)
    ASSERT_EQ(heap.getMin().v, k);
  EXPECT_TRUE(heap.is_empty());
}

// getMin() takes the last item to its place with assignments alone, as
// 7.7.4 did. A temporary built from it could throw with the minimum
// already out of the heap, which lost it
TEST(DynArrayHeapBasics, GetMinBuildsNoTemporaryOfTheLastItem)
{
  DynArrayHeap<Hard_To_Build> heap;
  for (const int x : {1, 5, 3, 9}) // the array is 1, 5, 3, 9
    heap.insert(Hard_To_Build(x));

  int min = 0;
  Hard_To_Build::armed = true;
  EXPECT_NO_THROW(min = heap.getMin().v);
  Hard_To_Build::armed = false;
  EXPECT_EQ(min, 1);
  EXPECT_EQ(heap.size(), 3U);
  for (const int x : {3, 5, 9})
    EXPECT_EQ(heap.getMin().v, x);
}

// The key may be an item of the heap itself when it is copied, or when a
// move leaves it as it was, even on the way of the new item, among others
// of its priority: the heap must end as with a key that is not in it
TEST(DynArrayHeapBasics, AKeyThatIsAnItemOfTheHeap)
{
  using Pair = std::pair<int, int>; // a priority and an identity
  using Heap = DynArrayHeap<Pair, First_Less>;
  for (int op = 0; op < 4; ++op) // insert and insert_direct, of a copy and of a move
    for (int n = 1; n <= 12; ++n)
      for (int place = 1; place <= n; ++place) // that of the key in the array
        {
          SCOPED_TRACE(testing::Message() << "op " << op << ", n " << n << ", place " << place);
          Heap heap, other;
          heap.reserve(n + 2);
          other.reserve(n + 2);
          for (int k = 1; k <= n; ++k)
            {
              heap.insert(Pair(k % 3, k));
              other.insert(Pair(k % 3, k));
            }
          Pair *key = nullptr;
          int at = 0;
          heap.traverse([&key, &at, place](Pair & x)
                        {
                          if (++at == place)
                            key = &x;
                          return true;
                        });
          ASSERT_NE(key, nullptr);
          const Pair copy = *key;
          switch (op)
            {
            case 0: heap.insert(*key); other.insert(copy); break;
            case 1: heap.insert(std::move(*key)); other.insert(Pair(copy)); break;
            case 2: heap.insert_direct(*key); other.insert_direct(copy); break;
            default: heap.insert_direct(std::move(*key)); other.insert_direct(Pair(copy));
            }
          EXPECT_EQ(layout(heap), layout(other));
        }
}

namespace
{
  // An item with a priority, which the comparisons look at, and an
  // identity, which they do not: it tells apart the items of a priority.
  // A move takes the identity of its source away
  struct Item
  {
    static constexpr int gone = -1;

    int pri = 0;
    int id = 0;

    Item() = default;
    Item(const int p, const int i) noexcept : pri(p), id(i) {}
    Item(const Item &) = default;
    Item & operator=(const Item &) = default;
    Item(Item && o) noexcept : pri(o.pri), id(std::exchange(o.id, gone)) {}
    Item & operator=(Item && o) noexcept
    {
      pri = o.pri;
      id = std::exchange(o.id, gone);
      return *this;
    }
    bool operator==(const Item &) const = default;
  };

  void PrintTo(const Item & x, std::ostream *os) { *os << x.pri << '#' << x.id; }

  // The calls of a Counted_Less: how many, the one that throws (0: none),
  // and the identities of the two items of each one, in order
  struct Calls
  {
    size_t count = 0;
    size_t throw_at = 0;
    std::vector<std::pair<int, int>> items;
  };

  template <class X>
  struct Counted_Less_Of
  {
    Calls *calls;

    bool operator()(const X & a, const X & b) const
    {
      if (++calls->count == calls->throw_at)
        throw std::runtime_error("comparison");
      calls->items.emplace_back(a.id, b.id);
      return a.pri < b.pri;
    }
  };

  using Counted_Less = Counted_Less_Of<Item>;
  using Counted_Heap = DynArrayHeap<Item, Counted_Less>;

  // An item whose move leaves in its source the priority `left`, which
  // each test chooses, and no identity
  struct Husk
  {
    static inline int left = 0;

    int pri = 0;
    int id = 0;

    Husk() = default;
    Husk(const int p, const int i) noexcept : pri(p), id(i) {}
    Husk(const Husk &) = default;
    Husk & operator=(const Husk &) = default;
    Husk(Husk && o) noexcept : pri(std::exchange(o.pri, left)), id(std::exchange(o.id, 0)) {}
    Husk & operator=(Husk && o) noexcept
    {
      pri = std::exchange(o.pri, left);
      id = std::exchange(o.id, 0);
      return *this;
    }
    bool operator==(const Husk &) const = default;
  };

  // The key of an insertion by move takes its item back by a move
  // assignment when the comparison throws: the heap promises to be as it
  // was only if that assignment does not throw, as with these items
  static_assert(std::is_nothrow_move_assignable_v<Item>);
  static_assert(std::is_nothrow_move_assignable_v<Husk>);

  void PrintTo(const Husk & x, std::ostream *os) { *os << x.pri << "#" << x.id; }

  // The sifts of 7.7.4, which exchanged an item with its parent or child
  // at each level, as a reference for the order of the new ones
  template <class X>
  size_t old_sift_up(DynArray<X> & a, size_t l, size_t r, Counted_Less_Of<X> & cmp)
  {
    for (size_t p, i = r; i > l; i = p)
      {
        p = i / 2;
        if (cmp(a.access(p), a.access(i)))
          return i;
        std::swap(a.access(p), a.access(i));
      }
    return l;
  }

  template <class X>
  void old_sift_down(DynArray<X> & a, size_t l, size_t r, Counted_Less_Of<X> & cmp)
  {
    for (size_t i = l; ;)
      {
        size_t c = 2 * i;
        if (c > r)
          return;
        if (c + 1 <= r and cmp(a.access(c + 1), a.access(c)))
          ++c;
        if (cmp(a.access(i), a.access(c)))
          return;
        std::swap(a.access(c), a.access(i));
        i = c;
      }
  }
}

// Each failed comparison of an insertion or an extraction leaves the heap
// as it was: its items in the same places, none moved from, and its size,
// with the key of an insertion by move untouched. 7.7.4 left the item
// counted, or the minimum lost, and the heap out of order
TEST(DynArrayHeapThrowingCompare, EveryFailedCallLeavesTheHeapAsItWas)
{
  std::mt19937 rng(20261120);
  std::uniform_int_distribution<int> values(0, 30);
  for (size_t n = 0; n <= 40; ++n)
    for (int op = 0; op < 5; ++op) // insert and insert_direct, of a copy and of a move; getMin
      {
        if (op == 4 and n == 0)
          continue;
        std::vector<Item> items;
        for (size_t k = 0; k < n; ++k)
          items.emplace_back(values(rng), static_cast<int>(k + 1));
        const Item key(values(rng), static_cast<int>(n + 1));
        for (size_t at = 1; ; ++at)
          {
            SCOPED_TRACE(testing::Message() << "n " << n << ", op " << op << ", call " << at);
            Calls calls;
            Counted_Heap heap(Counted_Less{&calls});
            const bool reserved = op == 2 or op == 3; // insert_direct() writes in reserved entries
            if (reserved)
              heap.reserve(n + 2);
            for (const Item & x : items)
              heap.insert(x);
            const std::vector<Item> before = layout(heap);
            calls.throw_at = calls.count + at;
            bool thrown = false;
            Item moved = key;
            try
              {
                switch (op)
                  {
                  case 0: heap.insert(key); break;
                  case 1: heap.insert(std::move(moved)); break;
                  case 2: heap.insert_direct(key); break;
                  case 3: heap.insert_direct(std::move(moved)); break;
                  default: (void) heap.getMin();
                  }
              }
            catch (const std::runtime_error &)
              {
                thrown = true;
              }
            calls.throw_at = 0;
            if (not thrown)
              break;
            EXPECT_EQ(heap.size(), n);
            EXPECT_EQ(layout(heap), before);
            EXPECT_EQ(moved, key);
          }
      }
}

// The new sifts compare the same items in the same order as the old ones,
// and leave every item in the same place, those of a same priority too
TEST(DynArrayHeapThrowingCompare, SameComparisonsAndPlacesAsTheOldSifts)
{
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> values(0, 50); // many items of a same priority
  Calls calls, old_calls;
  Counted_Heap heap(Counted_Less{&calls});
  Counted_Less old_cmp{&old_calls};
  DynArray<Item> old;
  size_t old_n = 0;
  for (int k = 0; k < 5000; ++k)
    {
      if (old_n == 0 or rng() % 3 != 0)
        {
          const Item x(values(rng), k + 1);
          if (k % 2 == 0)
            heap.insert(x);
          else
            heap.insert(Item(x));
          old.touch(++old_n) = x;
          old_sift_up(old, 1, old_n, old_cmp);
        }
      else
        {
          const Item min = heap.getMin();
          ASSERT_EQ(min, old.access(1));
          old.access(1) = old.access(old_n);
          --old_n;
          old_sift_down(old, 1, old_n, old_cmp);
        }
      ASSERT_EQ(calls.items, old_calls.items) << "operation " << k;
      calls.items.clear();
      old_calls.items.clear();
      std::vector<Item> old_layout;
      for (size_t i = 1; i <= old_n; ++i)
        old_layout.push_back(old.access(i));
      ASSERT_EQ(layout(heap), old_layout) << "operation " << k;
    }
}

namespace
{
  using Husk_Less = Counted_Less_Of<Husk>;
  using Husk_Heap = DynArrayHeap<Husk, Husk_Less>;

  // The item at a place of the array of a heap, from 1
  Husk & item_at(Husk_Heap & heap, const int place)
  {
    Husk *found = nullptr;
    int at = 0;
    heap.traverse([&found, &at, place](Husk & x)
                  {
                    if (++at == place)
                      found = &x;
                    return true;
                  });
    return *found;
  }
}

// The key of an insertion by move may be an item of the heap itself, which
// the move changes where it is. The key is moved before the comparisons, as
// in 7.7.4, so that they see that item as it is left: the same comparisons
// and the same places as then, whatever the place of the item, on the way
// of the new one or not, and whatever the move leaves in it, before the
// other items, among them or after them
TEST(DynArrayHeapBasics, AKeyThatItsMoveChangesInTheHeap)
{
  for (const int left : {-1, 1, 99}) // the priorities are 0, 1 and 2
    for (const bool direct : {false, true})
      for (int n = 1; n <= 12; ++n)
        for (int place = 1; place <= n; ++place)
          {
            SCOPED_TRACE(testing::Message() << "left " << left << ", direct " << direct
                                            << ", n " << n << ", place " << place);
            Husk::left = left;
            Calls calls, old_calls;
            Husk_Heap heap(Husk_Less{&calls});
            heap.reserve(n + 2);
            Husk_Less old_cmp{&old_calls};
            DynArray<Husk> old;
            std::vector<Husk> old_layout;
            for (int k = 1; k <= n; ++k)
              {
                const Husk x(k % 3, k);
                heap.insert(x);
                old.touch(k) = x;
                old_sift_up(old, 1, k, old_cmp);
              }
            for (int i = 1; i <= n; ++i)
              old_layout.push_back(old.access(i));
            ASSERT_EQ(layout(heap), old_layout);

            Husk & key = item_at(heap, place);
            if (direct)
              heap.insert_direct(std::move(key));
            else
              heap.insert(std::move(key));
            old.touch(n + 1) = std::move(old.access(place));
            old_sift_up(old, 1, n + 1, old_cmp);

            EXPECT_EQ(calls.items, old_calls.items);
            old_layout.clear();
            for (int i = 1; i <= n + 1; ++i)
              old_layout.push_back(old.access(i));
            EXPECT_EQ(layout(heap), old_layout);
          }
}

// If the comparison throws in that insertion, the key, one of the items of
// the heap, takes back what was moved from it, and the heap is as it was
TEST(DynArrayHeapThrowingCompare, AKeyThatItsMoveChangesStaysInTheHeap)
{
  for (const int left : {-1, 99})
    for (const bool direct : {false, true})
      for (int n = 1; n <= 9; ++n)
        for (int place = 1; place <= n; ++place)
          for (size_t at = 1; ; ++at)
            {
              SCOPED_TRACE(testing::Message() << "left " << left << ", direct " << direct << ", n " << n
                                              << ", place " << place << ", call " << at);
              Husk::left = left;
              Calls calls;
              Husk_Heap heap(Husk_Less{&calls});
              heap.reserve(n + 2);
              for (int k = 1; k <= n; ++k)
                heap.insert(Husk(k % 3, k));
              const std::vector<Husk> before = layout(heap);
              Husk & key = item_at(heap, place);
              calls.throw_at = calls.count + at;
              bool thrown = false;
              try
                {
                  if (direct)
                    heap.insert_direct(std::move(key));
                  else
                    heap.insert(std::move(key));
                }
              catch (const std::runtime_error &)
                {
                  thrown = true;
                }
              calls.throw_at = 0;
              if (not thrown)
                break;
              EXPECT_EQ(heap.size(), static_cast<size_t>(n));
              EXPECT_EQ(layout(heap), before);
            }
}
