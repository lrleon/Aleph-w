
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
            // each item with its priority, whatever its place
            std::vector<Item> by_id = a;
            std::sort(by_id.begin(), by_id.end(),
                      [](const Item & x, const Item & y) { return x.id < y.id; });
            EXPECT_EQ(by_id, items);
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
    const std::weak_ptr<int> second = heap[2];
    heap.remove(heap[2]); // not the last one
    const auto alive_of = [&seen]
    {
      return std::count_if(seen.begin(), seen.end(), [](const auto & w) { return not w.expired(); });
    };
    EXPECT_TRUE(second.expired()); // that one, and no other
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

// The heapsorts moved their pointer one item back before they looked at the
// size. With nothing to sort the pointer may be null, as that of an empty
// vector, and moving a null pointer is undefined
TEST(ArrayHeap, HeapsortOfNothingAndOfOneItem)
{
  heapsort<int>(nullptr, 0);
  faster_heapsort<int>(nullptr, 0);
  std::vector<int> none;
  heapsort(none.data(), none.size());
  faster_heapsort(none.data(), none.size());
  int one = 7;
  heapsort(&one, 1);
  EXPECT_EQ(one, 7);
  faster_heapsort(&one, 1);
  EXPECT_EQ(one, 7);
  std::vector<int> two{2, 1};
  heapsort(two.data(), two.size());
  EXPECT_EQ(two, (std::vector<int>{1, 2}));
  two = {2, 1};
  faster_heapsort(two.data(), two.size());
  EXPECT_EQ(two, (std::vector<int>{1, 2}));
}

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

  // A comparison that does not throw. For two items that are not const, as
  // a heap compares its own, the conversion of its result in a condition may
  struct Sly_Less
  {
    long *conversions;
    long *throw_at;

    bool operator()(const int & a, const int & b) const noexcept { return a < b; }

    Sly_Bool operator()(int & a, int & b) const noexcept { return {a < b, conversions, *throw_at}; }
  };

  static_assert(std::is_nothrow_invocable_r_v<bool, Sly_Less &, int &, int &>);
  static_assert(not noexcept(static_cast<bool>(std::declval<Sly_Less &>()(std::declval<int &>(),
                                                                         std::declval<int &>()))));
}

// remove() takes the way that has nothing to undo only if the comparison
// cannot throw, with the conversion to bool that the conditions of the
// sifts make of its result. It asked whether the implicit conversion could
// throw, which may be another function: with this comparison it lost the
// item that it took out
TEST(ArrayHeapThrowingCompare, AConversionInAConditionThatThrowsIsUndone)
{
  size_t failed = 0;
  for (size_t i = 1; i <= 5; ++i)
    for (long at = 1; ; ++at)
      {
        SCOPED_TRACE(testing::Message() << "item " << i << ", conversion " << at);
        long conversions = 0, throw_at = 0;
        ArrayHeap<int, Sly_Less> heap(8, Sly_Less{&conversions, &throw_at});
        for (const int v : {1, 2, 3, 4, 5, 6})
          heap.insert(v);
        const auto items_of = [&heap]
        {
          std::vector<int> items;
          for (size_t k = 1; k <= heap.size(); ++k)
            items.push_back(heap[k]);
          return items;
        };
        const std::vector<int> before = items_of();
        conversions = 0;
        throw_at = at;
        bool thrown = false;
        try
          {
            heap.remove(heap[i]);
          }
        catch (const std::runtime_error &)
          {
            thrown = true;
          }
        throw_at = 0;
        if (not thrown)
          {
            EXPECT_EQ(heap.size(), 5u);
            std::vector<int> left = items_of(), expected = before;
            expected.erase(expected.begin() + static_cast<long>(i - 1));
            std::sort(left.begin(), left.end());
            std::sort(expected.begin(), expected.end());
            EXPECT_EQ(left, expected);
            break;
          }
        ++failed;
        EXPECT_EQ(items_of(), before);
      }
  EXPECT_GT(failed, 5u);
}

namespace
{
  // An item that has no default constructor, and holds something that
  // tells whether it is alive
  struct Rooted
  {
    int pri;
    int id;
    std::shared_ptr<int> held;

    Rooted() = delete;
    Rooted(const int p, const int i) : pri(p), id(i), held(std::make_shared<int>(i)) {}
  };

  struct Rooted_Nothrow_Less
  {
    bool operator()(const Rooted & a, const Rooted & b) const noexcept { return a.pri < b.pri; }
  };

  // The six items of a test in a heap over a buffer, whose entry 0 a heap
  // does not use, and what tells whether each one is alive
  template <class Heap>
  std::vector<std::weak_ptr<int>> fill_rooted(Heap & heap)
  {
    std::vector<std::weak_ptr<int>> seen;
    int id = 0;
    for (const int v : {5, 3, 8, 1, 9, 2})
      {
        Rooted item(v, ++id);
        seen.push_back(item.held);
        heap.insert(std::move(item));
      }
    return seen;
  }

  size_t alive_of(const std::vector<std::weak_ptr<int>> & seen)
  {
    return static_cast<size_t>(std::count_if(seen.begin(), seen.end(),
                                             [](const auto & w) { return not w.expired(); }));
  }

  // The priority and the identity of each item of a heap, in the order of
  // its array: no copy of the items, which would keep alive what they hold
  template <class Heap>
  std::vector<std::pair<int, int>> rooted_in(const Heap & heap)
  {
    std::vector<std::pair<int, int>> items;
    for (size_t i = 1; i <= heap.size(); ++i)
      items.emplace_back(heap[i].pri, heap[i].id);
    return items;
  }
}

// A heap over a buffer of its caller takes items that have no default
// constructor, and remove() worked with them. It stopped compiling, since
// it built an empty item for the entry of the one that it takes out. Now
// it builds none with a comparison that cannot throw, and with one that
// may it takes the item out of its entry by a move where it cannot build
// an empty one
TEST(ArrayHeap, RemoveNeedsNoDefaultConstructor)
{
  {
    std::vector<Rooted> buffer(8, Rooted(0, 0));
    ArrayHeap<Rooted, Rooted_Nothrow_Less> heap(buffer.data(), 7);
    const auto seen = fill_rooted(heap);
    heap.remove(heap[2]); // not the last one
    EXPECT_EQ(heap.size(), 5u);
    EXPECT_EQ(alive_of(seen), 5u);
    int last = 0;
    while (not heap.is_empty())
      {
        const Rooted min = heap.getMin();
        EXPECT_LE(last, min.pri);
        last = min.pri;
      }
  }

  // with a comparison that may throw, at each of its calls in turn
  size_t failed = 0;
  for (size_t i = 1; i <= 5; ++i)
    for (size_t at = 1; ; ++at)
      {
        SCOPED_TRACE(testing::Message() << "item " << i << ", call " << at);
        std::vector<Rooted> buffer(8, Rooted(0, 0));
        Calls calls;
        ArrayHeap<Rooted, Counted_Less_Of<Rooted>> heap(buffer.data(), 7, Counted_Less_Of<Rooted>{&calls});
        const auto seen = fill_rooted(heap);
        const auto before = rooted_in(heap);
        calls.throw_at = calls.count + at;
        bool thrown = false;
        try
          {
            heap.remove(heap[i]);
          }
        catch (const std::runtime_error &)
          {
            thrown = true;
          }
        calls.throw_at = 0;
        if (not thrown)
          {
            EXPECT_EQ(heap.size(), 5u);
            EXPECT_EQ(alive_of(seen), 5u); // the one that left holds nothing
            break;
          }
        ++failed;
        EXPECT_EQ(rooted_in(heap), before);
        EXPECT_EQ(alive_of(seen), 6u);
      }
  EXPECT_GT(failed, 5u);
}

namespace
{
  // An item whose default constructor throws when it is told to
  struct Touchy
  {
    static inline bool armed = false;
    static inline long built = 0; // by default

    int pri = 0;
    int id = 0;

    Touchy()
    {
      ++built;
      if (armed)
        throw std::runtime_error("default constructor");
    }

    Touchy(const int p, const int i) noexcept : pri(p), id(i) {}
  };

  struct Touchy_Nothrow_Less
  {
    bool operator()(const Touchy & a, const Touchy & b) const noexcept { return a.pri < b.pri; }
  };
}

// With a comparison that may throw, remove() built an empty item for the
// entry of the one that it takes out, once that one was out: if the
// default constructor threw, the exception came out of a heap that had
// lost the item. Now it builds an item only if building it cannot throw,
// and none with a comparison that cannot throw
TEST(ArrayHeapThrowingCompare, RemoveBuildsNoItemWhoseConstructorMayThrow)
{
  static_assert(not std::is_nothrow_default_constructible_v<Touchy>);
  Calls calls;
  ArrayHeap<Touchy, Counted_Less_Of<Touchy>> heap(8, Counted_Less_Of<Touchy>{&calls});
  ArrayHeap<Touchy, Touchy_Nothrow_Less> quiet(8);
  int id = 0;
  for (const int v : {5, 3, 8, 1, 9, 2})
    {
      heap.insert(Touchy(v, ++id));
      quiet.insert(Touchy(v, id));
    }

  const long built = Touchy::built;
  Touchy::armed = true;
  EXPECT_NO_THROW(heap.remove(heap[2]));
  EXPECT_NO_THROW(quiet.remove(quiet[2]));
  Touchy::armed = false;
  EXPECT_EQ(Touchy::built, built);
  EXPECT_EQ(heap.size(), 5u);
  EXPECT_EQ(quiet.size(), 5u);
  int last = 0;
  while (not heap.is_empty())
    {
      const Touchy min = heap.getMin();
      EXPECT_LE(last, min.pri);
      last = min.pri;
    }
}

namespace
{
  // An item whose default constructor does not throw and whose move
  // assignment throws when its source is empty, as no item of a heap is.
  // A move leaves its source empty
  struct Fussy
  {
    int pri = 0;
    int id = 0; // none: empty

    Fussy() noexcept = default;
    Fussy(const int p, const int i) noexcept : pri(p), id(i) {}
    Fussy(const Fussy &) = default;
    Fussy & operator=(const Fussy &) = default;
    Fussy(Fussy && o) noexcept : pri(o.pri), id(std::exchange(o.id, 0)) {}

    Fussy & operator=(Fussy && o)
    {
      if (o.id == 0)
        throw std::runtime_error("move assignment from an empty item");
      pri = o.pri;
      id = std::exchange(o.id, 0);
      return *this;
    }
  };

  // The identities, in order, of the items that a heap gives, which must
  // come by priority: pri_and_id gives the two of an item
  template <class Heap, class Pri_And_Id>
  std::vector<int> drained_ids(Heap & heap, Pri_And_Id pri_and_id)
  {
    std::vector<int> ids;
    int last = 0;
    while (not heap.is_empty())
      {
        const auto min = heap.getMin();
        const std::pair<int, int> item = pri_and_id(min);
        EXPECT_LE(last, item.first);
        last = item.first;
        ids.push_back(item.second);
      }
    std::sort(ids.begin(), ids.end());
    return ids;
  }

  // The identities from 1 to n but one
  std::vector<int> all_but(const int n, const int out)
  {
    std::vector<int> ids;
    for (int i = 1; i <= n; ++i)
      if (i != out)
        ids.push_back(i);
    return ids;
  }
}

// With a comparison that may throw, remove() emptied the entry of the item
// that it takes out by assigning an empty item to it, once that one was
// out: if the assignment threw, the exception came out of a heap that had
// lost the item, where trying again with the same reference took another
// one out. Now nothing that may throw runs once the item is out: such an
// item is moved out of its entry
TEST(ArrayHeapThrowingCompare, RemoveAssignsNoEmptyItemIfThatMayThrow)
{
  static_assert(std::is_nothrow_default_constructible_v<Fussy>);
  static_assert(not std::is_nothrow_move_assignable_v<Fussy>);
  static_assert(std::is_nothrow_move_constructible_v<Fussy>);
  std::vector<Fussy> buffer(8);
  Calls calls;
  ArrayHeap<Fussy, Counted_Less_Of<Fussy>> heap(buffer.data(), 7, Counted_Less_Of<Fussy>{&calls});
  int id = 0;
  for (const int v : {5, 3, 8, 1, 9, 2})
    heap.insert(Fussy(v, ++id));
  const int removed = heap[2].id;
  ASSERT_NE(removed, 0);

  EXPECT_NO_THROW(heap.remove(heap[2]));
  EXPECT_EQ(heap.size(), 5u);
  EXPECT_EQ(buffer[6].id, 0); // the entry of the item that left, past the end
  EXPECT_EQ(drained_ids(heap, [](const Fussy & x) { return std::make_pair(x.pri, x.id); }),
            all_but(6, removed));
}

namespace
{
  // The part of an item that the standard library exchanges by its own
  // swap, with no move, and whose default and move constructors throw when
  // they are told to. It holds something that tells whether it is alive
  struct Welded
  {
    static inline bool armed = false;

    int pri = 0;
    int id = 0;
    std::shared_ptr<int> held;

    Welded()
    {
      if (armed)
        throw std::runtime_error("default constructor");
    }

    Welded(const int p, const int i) : pri(p), id(i), held(std::make_shared<int>(i)) {}
    Welded(const Welded &) = default;
    Welded & operator=(const Welded &) = default;

    Welded(Welded && o) : pri(o.pri), id(o.id), held(o.held)
    {
      if (armed)
        throw std::runtime_error("move constructor");
    }

    Welded & operator=(Welded &&) noexcept = default;

    friend void swap(Welded & a, Welded & b) noexcept
    {
      std::swap(a.pri, b.pri);
      std::swap(a.id, b.id);
      a.held.swap(b.held);
    }
  };

  // std::swap() of a pair calls the swap of each of its members: this item
  // is exchanged with no move, as a std::deque is, whose move constructor
  // may throw in some standard libraries
  using Welded_Item = std::pair<Welded, int>;

  struct Welded_Less
  {
    bool operator()(const Welded_Item & a, const Welded_Item & b) const // may throw
    {
      return a.first.pri < b.first.pri;
    }
  };
}

// With a comparison that may throw and an item that it could not empty
// otherwise, remove() moved the item out of its entry once it was out of
// the heap: if the move constructor threw, the exception came out of a
// heap that had lost the item. An exchange that does not throw, which is
// what the heap asks of its items, does not mean a move that does not. Now
// such an item stays in its entry until another one takes it
TEST(ArrayHeapThrowingCompare, RemoveMovesNoItemOutIfThatMayThrow)
{
  static_assert(noexcept(std::swap(std::declval<Welded_Item &>(), std::declval<Welded_Item &>())));
  static_assert(not std::is_nothrow_default_constructible_v<Welded_Item>);
  static_assert(not std::is_nothrow_move_constructible_v<Welded_Item>);
  std::vector<Welded_Item> buffer(8);
  ArrayHeap<Welded_Item, Welded_Less> heap(buffer.data(), 7);
  std::vector<std::weak_ptr<int>> seen;
  int id = 0;
  for (const int v : {5, 3, 8, 1, 9, 2})
    {
      Welded_Item item(Welded(v, ++id), v);
      seen.push_back(item.first.held);
      heap.insert(std::move(item));
    }
  const int removed = heap[2].first.id;

  Welded::armed = true;
  EXPECT_NO_THROW(heap.remove(heap[2]));
  Welded::armed = false;
  EXPECT_EQ(heap.size(), 5u);
  EXPECT_EQ(buffer[6].first.id, removed); // in its entry, which is past the end
  EXPECT_EQ(alive_of(seen), 6u);

  Welded_Item seventh(Welded(4, ++id), 4);
  seen.push_back(seventh.first.held);
  heap.insert(std::move(seventh)); // takes that entry
  EXPECT_EQ(alive_of(seen), 6u);
  EXPECT_TRUE(seen[static_cast<size_t>(removed) - 1].expired());
  EXPECT_EQ(drained_ids(heap, [](const Welded_Item & x) { return std::make_pair(x.first.pri, x.first.id); }),
            all_but(7, removed));
}
