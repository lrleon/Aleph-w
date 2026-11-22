
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
 * @file arrayheap.cc
 * @brief Tests for Arrayheap
 */

#include <algorithm>
#include <memory>
#include <new>
#include <numeric>
#include <ostream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_arrayHeap.H>

using namespace Aleph;
using namespace testing;

TEST(ArrayHeap, InsertAndExtract)
{
  ArrayHeap<int> heap(8);
  heap.insert(3);
  heap.insert(1);
  heap.insert(2);

  EXPECT_EQ(heap.size(), 3u);
  EXPECT_FALSE(heap.is_empty());
  EXPECT_EQ(heap.top(), 1);

  EXPECT_EQ(heap.getMin(), 1);
  EXPECT_EQ(heap.top(), 2);
  EXPECT_EQ(heap.get(), 2);
  EXPECT_EQ(heap.getMax(), 3); // alias of getMin when comparator flips
  EXPECT_TRUE(heap.is_empty());
}

TEST(ArrayHeap, UpdateAndRemove)
{
  ArrayHeap<int> heap(5);
  auto & ref = heap.insert(5);
  const auto & ref2 = heap.insert(7);

  ref = 1;
  heap.update(ref);
  EXPECT_EQ(heap.top(), 1);
  EXPECT_EQ(ref2, 7);

  heap.remove(ref);
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.top(), 7);
}

TEST(ArrayHeap, CapacityOverflow)
{
  ArrayHeap<int> heap(2);
  heap.insert(10);
  heap.insert(20);
  EXPECT_THROW(heap.insert(30), std::overflow_error);
}

TEST(ArrayHeap, ExtractsSortedOrder)
{
  ArrayHeap<int> heap(10);
  const std::vector<int> input{5, 3, 8, 1, 4, 9, 2, 7};
  for (int value : input)
    heap.insert(value);

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  std::vector<int> sorted = input;
  std::sort(sorted.begin(), sorted.end());
  EXPECT_EQ(drained, sorted);
  EXPECT_TRUE(heap.is_empty());
}

// ─────────────────────────────────────────────────────────────────
// A comparison that throws
// ─────────────────────────────────────────────────────────────────

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

  // The key of an insertion by move takes its item back by a move
  // assignment when the comparison throws: the heap promises to be as it
  // was only if that assignment does not throw, as with these items
  static_assert(std::is_nothrow_move_assignable_v<Item>);

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
  using Counted_Heap = ArrayHeap<Item, Counted_Less>;

  // The items of a heap, in the order of its array
  template <class X>
  std::vector<X> layout(const ArrayHeap<X, Counted_Less_Of<X>> & h)
  {
    std::vector<X> items;
    for (size_t i = 1; i <= h.size(); ++i)
      items.push_back(h[i]);
    return items;
  }

  // The heap of 7.7.5, on a vector whose entry 0 is not used: its sifts
  // exchanged an item with its parent or its child at each level, and its
  // operations stored and counted before they compared
  template <class X>
  struct Old_Heap_Of
  {
    std::vector<X> a = std::vector<X>(1);
    size_t n = 0;
    Counted_Less_Of<X> cmp;

    void sift_up(const size_t l, const size_t r)
    {
      for (size_t p, i = r; i > l; i = p)
        {
          p = i / 2;
          if (cmp(a[p], a[i]))
            return;
          std::swap(a[p], a[i]);
        }
    }

    void sift_down(const size_t l, const size_t r)
    {
      for (size_t i = l; ;)
        {
          size_t c = 2 * i;
          if (c > r)
            return;
          if (c + 1 <= r and cmp(a[c + 1], a[c]))
            ++c;
          if (cmp(a[i], a[c]))
            return;
          std::swap(a[c], a[i]);
          i = c;
        }
    }

    void sift_down_up(const size_t l, const size_t i, const size_t r)
    {
      sift_down(i, r);
      sift_up(l, i);
    }

    void insert(X && x)
    {
      a.resize(std::max(a.size(), n + 2));
      a[n + 1] = std::move(x); // x may be one of the items
      sift_up(1, ++n);
    }

    void insert(const X & x) { insert(X(x)); }

    X getMin()
    {
      const X ret = a[1];
      a[1] = a[n--];
      if (n > 0)
        sift_down(1, n);
      return ret;
    }

    void update(const size_t i) { sift_down_up(1, i, n); }

    void remove(const size_t i)
    {
      if (i == n)
        {
          --n;
          return;
        }
      a[i] = a[n--];
      sift_down_up(1, i, n);
    }

    std::vector<X> layout() const { return {a.begin() + 1, a.begin() + 1 + static_cast<long>(n)}; }
  };

  using Old_Heap = Old_Heap_Of<Item>;

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

  static_assert(std::is_nothrow_move_assignable_v<Husk>);

  void PrintTo(const Husk & x, std::ostream *os) { *os << x.pri << '#' << x.id; }

  using Husk_Less = Counted_Less_Of<Husk>;
  using Husk_Heap = ArrayHeap<Husk, Husk_Less>;
}

// The new sifts compare the same items in the same order as those of
// 7.7.5, and leave every item in the same place, those of a same priority
// too: in the insertions, of a copy and of a move, in the extractions, in
// the removals of any item and in the updates of an item whose priority
// changed
TEST(ArrayHeapThrowingCompare, SameComparisonsAndPlacesAsTheOldSifts)
{
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> values(0, 50); // many items of a same priority
  Calls calls, old_calls;
  Counted_Heap heap(6000, Counted_Less{&calls});
  Old_Heap old{.cmp = Counted_Less{&old_calls}};
  size_t updates = 0;
  size_t removals = 0;
  for (int k = 0; k < 6000; ++k)
    {
      const unsigned op = old.n == 0 ? 0 : rng() % 8;
      if (op < 4)
        {
          const Item x(values(rng), k + 1);
          switch (op)
            {
            case 0: heap.insert(x); break;
            case 1: heap.insert(Item(x)); break;
            case 2: heap.insert_ne(x); break;
            default: heap.insert_ne(Item(x));
            }
          old.insert(x);
        }
      else if (op == 4)
        {
          const Item min = heap.getMin();
          ASSERT_EQ(min, old.getMin());
        }
      else if (op == 5)
        {
          const size_t i = 1 + rng() % old.n;
          heap.remove(heap[i]);
          old.remove(i);
          ++removals;
        }
      else
        {
          const size_t i = 1 + rng() % old.n;
          const int pri = values(rng);
          heap[i].pri = pri;
          old.a[i].pri = pri;
          heap.update(heap[i]);
          old.update(i);
          ++updates;
        }
      ASSERT_EQ(calls.items, old_calls.items) << "operation " << k;
      calls.items.clear();
      old_calls.items.clear();
      ASSERT_EQ(layout(heap), old.layout()) << "operation " << k;
    }
  EXPECT_GT(updates, 1000u);
  EXPECT_GT(removals, 500u);
  EXPECT_GT(heap.size(), 100u);
}

// Each failed comparison of an insertion, an extraction, a removal or an
// update leaves the heap as it was: its items in the same places, none
// moved from, and its size, with the key of an insertion by move untouched.
// 7.7.5 left the item counted, or the minimum lost, and the heap out of
// order
TEST(ArrayHeapThrowingCompare, EveryFailedCallLeavesTheHeapAsItWas)
{
  std::mt19937 rng(20261121);
  std::uniform_int_distribution<int> values(0, 30);
  size_t failed = 0;
  for (size_t n = 0; n <= 40; ++n)
    for (int op = 0; op < 7; ++op) // 0-3 insertions; 4 getMin; 5 remove; 6 update
      {
        if (op >= 4 and n == 0)
          continue;
        std::vector<Item> items;
        for (size_t k = 0; k < n; ++k)
          items.emplace_back(values(rng), static_cast<int>(k + 1));
        const Item key(values(rng), static_cast<int>(n + 1));
        const size_t where = n == 0 ? 0 : 1 + rng() % n; // of the removal and of the update
        const int new_pri = values(rng);
        for (size_t at = 1; ; ++at)
          {
            SCOPED_TRACE(testing::Message() << "n " << n << ", op " << op << ", call " << at);
            Calls calls;
            Counted_Heap heap(n + 2, Counted_Less{&calls});
            for (const Item & x : items)
              heap.insert(x);
            if (op == 6)
              heap[where].pri = new_pri; // the heap is as its user left it
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
                  case 2: heap.insert_ne(key); break;
                  case 3: heap.insert_ne(std::move(moved)); break;
                  case 4: (void) heap.getMin(); break;
                  case 5: heap.remove(heap[where]); break;
                  default: heap.update(heap[where]);
                  }
              }
            catch (const std::runtime_error &)
              {
                thrown = true;
              }
            calls.throw_at = 0;
            if (not thrown)
              {
                ASSERT_TRUE(valid_heap(&heap[0], 1, heap.size(), Counted_Less{&calls}));
                break;
              }
            ++failed;
            EXPECT_EQ(heap.size(), n);
            EXPECT_EQ(layout(heap), before);
            EXPECT_EQ(moved, key);
          }
      }
  EXPECT_GT(failed, 500u);
}

// A comparison that throws once, as in the notes of 7.7.5 for
// DynArrayHeap: insert(1) on a heap that holds 5 left the size at 2 and the
// top at 5, and getMin() on 1, 2, 3 and 4 left the size at 3, lost the 1,
// and then gave 4, 2 and 3
TEST(ArrayHeapThrowingCompare, OneThrowLosesNothing)
{
  struct Once
  {
    bool *armed;

    bool operator()(const int a, const int b) const
    {
      if (std::exchange(*armed, false))
        throw std::runtime_error("comparison");
      return a < b;
    }
  };

  bool armed = false;
  {
    ArrayHeap<int, Once> heap(8, Once{&armed});
    heap.insert(5);
    armed = true;
    EXPECT_THROW(heap.insert(1), std::runtime_error);
    EXPECT_EQ(heap.size(), 1u);
    EXPECT_EQ(heap.top(), 5);
  }
  {
    ArrayHeap<int, Once> heap(8, Once{&armed});
    for (const int i : {1, 2, 3, 4})
      heap.insert(i);
    armed = true;
    EXPECT_THROW((void) heap.getMin(), std::runtime_error);
    EXPECT_EQ(heap.size(), 4u);
    std::vector<int> drained;
    while (not heap.is_empty())
      drained.push_back(heap.getMin());
    EXPECT_EQ(drained, (std::vector<int>{1, 2, 3, 4}));
  }
}

// The key of an insertion by move may be an item of the heap itself, which
// the move changes where it is. The key is moved before the comparisons, as
// in 7.7.5, so that they see that item as it is left: the same comparisons
// and the same places as then, whatever the place of the item, on the way
// of the new one or not, and whatever the move leaves in it, before the
// other items, among them or after them
TEST(ArrayHeapThrowingCompare, AKeyThatItsMoveChangesInTheHeap)
{
  for (const int left : {-1, 1, 99}) // the priorities are 0, 1 and 2
    for (const bool checked : {false, true})
      for (int n = 1; n <= 12; ++n)
        for (int place = 1; place <= n; ++place)
          {
            SCOPED_TRACE(testing::Message() << "left " << left << ", checked " << checked
                                            << ", n " << n << ", place " << place);
            Husk::left = left;
            Calls calls, old_calls;
            Husk_Heap heap(n + 2, Husk_Less{&calls});
            Old_Heap_Of<Husk> old{.cmp = Husk_Less{&old_calls}};
            for (int k = 1; k <= n; ++k)
              {
                const Husk x(k % 3, k);
                heap.insert(x);
                old.insert(x);
              }
            ASSERT_EQ(layout(heap), old.layout());

            if (checked)
              heap.insert(std::move(heap[place]));
            else
              heap.insert_ne(std::move(heap[place]));
            old.a.resize(n + 2); // before the reference to one of its items
            old.insert(std::move(old.a[place]));

            EXPECT_EQ(calls.items, old_calls.items);
            EXPECT_EQ(layout(heap), old.layout());
          }
}

// If the comparison throws in that insertion, the key, one of the items of
// the heap, takes back what was moved from it, and the heap is as it was
TEST(ArrayHeapThrowingCompare, AKeyThatItsMoveChangesStaysInTheHeap)
{
  size_t failed = 0;
  for (const int left : {-1, 99})
    for (const bool checked : {false, true})
      for (int n = 1; n <= 9; ++n)
        for (int place = 1; place <= n; ++place)
          for (size_t at = 1; ; ++at)
            {
              SCOPED_TRACE(testing::Message() << "left " << left << ", checked " << checked << ", n " << n
                                              << ", place " << place << ", call " << at);
              Husk::left = left;
              Calls calls;
              Husk_Heap heap(n + 2, Husk_Less{&calls});
              for (int k = 1; k <= n; ++k)
                heap.insert(Husk(k % 3, k));
              const std::vector<Husk> before = layout(heap);
              calls.throw_at = calls.count + at;
              bool thrown = false;
              try
                {
                  if (checked)
                    heap.insert(std::move(heap[place]));
                  else
                    heap.insert_ne(std::move(heap[place]));
                }
              catch (const std::runtime_error &)
                {
                  thrown = true;
                }
              calls.throw_at = 0;
              if (not thrown)
                break;
              ++failed;
              EXPECT_EQ(heap.size(), static_cast<size_t>(n));
              EXPECT_EQ(layout(heap), before);
            }
  EXPECT_GT(failed, 100u);
}

// A heapsort whose comparison throws leaves in the array the items that it
// had, in another order, and none of them moved from
TEST(ArrayHeapThrowingCompare, AFailedHeapsortKeepsEveryItem)
{
  std::mt19937 rng(5);
  std::uniform_int_distribution<int> values(0, 20);
  size_t failed = 0;
  for (const bool faster : {false, true})
    for (size_t n = 0; n <= 24; ++n)
      {
        std::vector<Item> items;
        for (size_t k = 0; k < n; ++k)
          items.emplace_back(values(rng), static_cast<int>(k + 1));
        for (size_t at = 1; ; ++at)
          {
            SCOPED_TRACE(testing::Message() << "faster " << faster << ", n " << n << ", call " << at);
            std::vector<Item> a = items;
            Calls calls;
            calls.throw_at = at;
            bool thrown = false;
            try
              {
                if (faster)
                  faster_heapsort(a.data(), a.size(), Counted_Less{&calls});
                else
                  heapsort(a.data(), a.size(), Counted_Less{&calls});
              }
            catch (const std::runtime_error &)
              {
                thrown = true;
              }
            std::vector<int> ids;
            for (const Item & x : a)
              ids.push_back(x.id);
            std::sort(ids.begin(), ids.end());
            std::vector<int> all(n);
            std::iota(all.begin(), all.end(), 1);
            EXPECT_EQ(ids, all);
            if (not thrown)
              {
                EXPECT_TRUE(std::is_sorted(a.begin(), a.end(),
                                           [](const Item & x, const Item & y) { return x.pri < y.pri; }));
                break;
              }
            ++failed;
          }
      }
  EXPECT_GT(failed, 1000u);
}

namespace
{
  // A comparison that contradicts itself: each call gives what a generator
  // of random bits says, whatever the items
  struct Random_Less
  {
    std::mt19937 *rng;

    bool operator()(const Item &, const Item &) const { return (*rng)() & 1; }
  };
}

// With a comparison that contradicts itself the heap cannot be in order,
// but it keeps its items after each operation, none lost, repeated or
// moved from: the places that the operations find are always in the array
TEST(ArrayHeapThrowingCompare, AComparisonThatContradictsItselfKeepsTheItems)
{
  std::mt19937 bits(99), rng(3);
  ArrayHeap<Item, Random_Less> heap(160, Random_Less{&bits});
  std::vector<int> ids; // of the items in the heap
  size_t removals = 0, updates = 0;
  for (int k = 0; k < 20000; ++k)
    {
      const unsigned op = heap.is_empty() ? 0 : rng() % 7;
      if (op < 3 and heap.size() < 150) // small, so that looking at all of it is cheap
        {
          if (k % 2 == 0)
            heap.insert(Item(k, k + 1));
          else
            heap.insert_ne(Item(k, k + 1));
          ids.push_back(k + 1);
        }
      else if (op <= 3)
        {
          const Item min = heap.getMin();
          const auto it = std::find(ids.begin(), ids.end(), min.id);
          ASSERT_NE(it, ids.end()) << "operation " << k;
          ids.erase(it);
        }
      else
        {
          const size_t i = 1 + rng() % heap.size();
          if (op == 4)
            {
              const auto it = std::find(ids.begin(), ids.end(), heap[i].id);
              ASSERT_NE(it, ids.end()) << "operation " << k;
              ids.erase(it);
              heap.remove(heap[i]);
              ++removals;
            }
          else
            {
              heap.update(heap[i]);
              ++updates;
            }
        }
      ASSERT_EQ(heap.size(), ids.size()) << "operation " << k;
      std::vector<int> now;
      for (size_t i = 1; i <= heap.size(); ++i)
        now.push_back(heap[i].id);
      std::sort(now.begin(), now.end());
      std::vector<int> sorted = ids;
      std::sort(sorted.begin(), sorted.end());
      ASSERT_EQ(now, sorted) << "operation " << k;
    }
  EXPECT_GT(removals, 1000u);
  EXPECT_GT(updates, 1000u);
}

namespace
{
  // An item whose copies throw while `armed`, and whose moves never do; it
  // counts its copies
  struct Fragile
  {
    static inline bool armed = false;
    static inline size_t copies = 0;

    int pri = 0;

    Fragile() = default;
    Fragile(const int p) noexcept : pri(p) {}
    Fragile(const Fragile & o) : pri(o.pri) { copied(); }
    Fragile & operator=(const Fragile & o)
    {
      copied();
      pri = o.pri;
      return *this;
    }
    Fragile(Fragile &&) noexcept = default;
    Fragile & operator=(Fragile &&) noexcept = default;

    static void copied()
    {
      if (armed)
        throw std::bad_alloc();
      ++copies;
    }

    bool operator<(const Fragile & o) const noexcept { return pri < o.pri; }
  };
}

// An insertion whose copy of the key throws leaves the heap as it was: the
// item was counted before it was copied, so the heap had one item more,
// which was whatever its array held. And getMin() copies nothing: it copied
// the minimum, and then the last item over it after counting it out, so a
// copy that threw there lost the last item
TEST(ArrayHeapThrowingCompare, ACopyThatThrowsLeavesTheHeapAsItWas)
{
  ArrayHeap<Fragile> heap(8);
  for (const int p : {5, 3, 8, 1})
    heap.insert(Fragile(p));

  const Fragile key(0);
  Fragile::armed = true;
  EXPECT_THROW(heap.insert(key), std::bad_alloc);
  EXPECT_THROW(heap.insert_ne(key), std::bad_alloc);
  EXPECT_EQ(heap.size(), 4u);
  EXPECT_EQ(heap.top().pri, 1);

  std::vector<int> out; // with the copies still armed
  while (not heap.is_empty())
    out.push_back(heap.getMin().pri);
  Fragile::armed = false;
  EXPECT_EQ(out, (std::vector<int>{1, 3, 5, 8}));

  for (const int p : {5, 3, 8, 1})
    heap.insert(Fragile(p));
  Fragile::copies = 0;
  (void) heap.getMin();
  heap.remove(heap[2]);
  EXPECT_EQ(Fragile::copies, 0u);
}

namespace
{
  // A comparison of items that says that it does not throw
  struct Nothrow_Less
  {
    bool operator()(const Item & a, const Item & b) const noexcept { return a.pri < b.pri; }
  };

  // Orders shared pointers by what they point to; it may throw or not
  template <bool nothrow>
  struct Pointed_Less
  {
    bool operator()(const std::shared_ptr<int> & a, const std::shared_ptr<int> & b) const noexcept(nothrow)
    {
      return *a < *b;
    }
  };
}

// With a comparison that does not throw, remove() has nothing to undo and
// moves the last item over the removed one, as before. The heap keeps the
// other items, each one whole, and in order
TEST(ArrayHeap, RemoveWithAComparisonThatDoesNotThrow)
{
  std::mt19937 rng(17);
  std::uniform_int_distribution<int> values(0, 50);
  ArrayHeap<Item, Nothrow_Less> heap(300);
  std::vector<int> ids;
  size_t removals = 0;
  for (int k = 0; k < 6000; ++k)
    {
      if (heap.size() < 2 or (rng() % 2 == 0 and heap.size() < 290))
        {
          heap.insert(Item(values(rng), k + 1));
          ids.push_back(k + 1);
        }
      else
        {
          const size_t i = 1 + rng() % heap.size();
          ids.erase(std::find(ids.begin(), ids.end(), heap[i].id));
          heap.remove(heap[i]);
          ++removals;
        }
      std::vector<int> now;
      for (size_t i = 1; i <= heap.size(); ++i)
        now.push_back(heap[i].id);
      std::sort(now.begin(), now.end());
      std::vector<int> sorted = ids;
      std::sort(sorted.begin(), sorted.end());
      ASSERT_EQ(now, sorted) << "operation " << k;
      Nothrow_Less less;
      ASSERT_TRUE(valid_heap(&heap[0], 1, heap.size(), less)) << "operation " << k;
    }
  EXPECT_GT(removals, 2000u);
}

// As before, an item that remove() takes out from among the others holds
// nothing afterwards, whether the comparison may throw or not: the heap does
// not keep it alive in the entry that it no longer counts. The last item of
// the array, which is removed without moving anything, stays in its entry
// until another one takes it
TEST(ArrayHeap, RemoveReleasesTheItem)
{
  auto check = [](auto & heap)
  {
    std::vector<std::weak_ptr<int>> seen;
    for (const int v : {5, 3, 8, 1, 9, 2})
      {
        auto p = std::make_shared<int>(v);
        seen.push_back(p);
        heap.insert(std::move(p));
      }
    heap.remove(heap[2]); // not the last one
    const auto alive_of = [&seen]
    {
      return std::count_if(seen.begin(), seen.end(), [](const auto & w) { return not w.expired(); });
    };
    EXPECT_EQ(alive_of(), 5);
    heap.remove(heap[heap.size()]); // the last one
    EXPECT_EQ(alive_of(), 5);
    EXPECT_EQ(heap.size(), 4u);
    size_t in_heap = 0;
    for (size_t i = 1; i <= heap.size(); ++i)
      in_heap += heap[i] != nullptr ? 1 : 0;
    EXPECT_EQ(in_heap, 4u);
  };
  ArrayHeap<std::shared_ptr<int>, Pointed_Less<true>> quiet(8);
  check(quiet);
  ArrayHeap<std::shared_ptr<int>, Pointed_Less<false>> loud(8);
  check(loud);
}
