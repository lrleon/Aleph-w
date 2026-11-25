
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
 * @file item_copies_test.cc
 * @brief The functions that make a container out of the items of another
 *        copy each item that it holds once
 */

// keys() and values() of the maps took each pair by value, which copied its
// key and its value, and then copied the one that they return: three copies
// where one was needed. ranks() and pair_ranks() of a list, and the copy
// constructor and assignment of a Random_Set, copied each item twice. These
// tests count the copies that such functions make: one for each item of the
// new container, and none of the items that it does not hold.

#include <gtest/gtest.h>

#include <ah-convert.H>
#include <ah-dry-mixin.H>
#include <ahSort.H>
#include <htlist.H>
#include <tpl_array.H>
#include <tpl_dynDlist.H>
#include <tpl_dynMapOhash.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetHash.H>
#include <tpl_flat_map.H>
#include <tpl_random_queue.H>

using namespace Aleph;

namespace
{
  // An item that counts its copies and its moves, those of an item that
  // holds a value: the arrays fill their blocks with copies of a default
  // item, which hold none
  struct Counted
  {
    static inline long copies = 0;
    static inline long moves = 0;

    int v = -1;

    Counted() = default;
    explicit Counted(const int x) noexcept : v(x) {}

    Counted(const Counted & o) : v(o.v) { copies += v >= 0 ? 1 : 0; }
    Counted(Counted && o) noexcept : v(o.v) { moves += v >= 0 ? 1 : 0; }

    Counted & operator=(const Counted & o)
    {
      v = o.v;
      copies += v >= 0 ? 1 : 0;
      return *this;
    }

    Counted & operator=(Counted && o) noexcept
    {
      v = o.v;
      moves += v >= 0 ? 1 : 0;
      return *this;
    }

    bool operator<(const Counted & o) const noexcept { return v < o.v; }
    bool operator==(const Counted & o) const noexcept { return v == o.v; }
  };

  // For the hash tables whose key is a Counted
  size_t aleph_hash_value(const Counted & c) noexcept
  {
    return static_cast<size_t>(c.v) * 0x9E3779B97F4A7C15ull;
  }

  // A container made of the mixins of ah-dry-mixin.H, for KeysMixin::keys()
  class Mixed : public TraverseMixin<Mixed, Counted>,
                public FunctionalMixin<Mixed, Counted>,
                public KeysMixin<Mixed, Counted>
  {
    Array<Counted> data;

  public:
    using Item_Type = Counted;

    void append(const Counted & c) { data.append(c); }

    class Iterator
    {
      const Array<Counted> * items = nullptr;
      size_t pos = 0;

    public:
      Iterator() = default;
      Iterator(const Mixed & c) : items(&c.data) {}

      [[nodiscard]] bool has_curr() const noexcept { return pos < items->size(); }
      [[nodiscard]] Counted & get_curr() const { return const_cast<Counted &>((*items)(pos)); }
      void next() { ++pos; }
      void next_ne() noexcept { ++pos; }
    };

    [[nodiscard]] Iterator get_it() const { return Iterator(*this); }
  };

  template <class Map>
  class MapCopies : public ::testing::Test
  {};

  using Maps = ::testing::Types<DynMapAvlTree<int, Counted>, DynMapRbTree<int, Counted>,
                                DynMapSplayTree<int, Counted>, DynMapTreap<int, Counted>,
                                DynMapRandTree<int, Counted>, DynMapBinTree<int, Counted>,
                                DynMapTreapRk<int, Counted>, DynMapHash<int, Counted>,
                                DynMapLinHash<int, Counted>, MapODhash<int, Counted>,
                                MapOLhash<int, Counted>, FlatMap<int, Counted>>;
  TYPED_TEST_SUITE(MapCopies, Maps);

  // The same maps, with the counted item as their key
  template <class Map>
  class MapKeyCopies : public ::testing::Test
  {};

  using Maps_By_Counted = ::testing::Types<DynMapAvlTree<Counted, int>, DynMapRbTree<Counted, int>,
                                           DynMapSplayTree<Counted, int>, DynMapTreap<Counted, int>,
                                           DynMapRandTree<Counted, int>, DynMapBinTree<Counted, int>,
                                           DynMapTreapRk<Counted, int>, DynMapHash<Counted, int>,
                                           DynMapLinHash<Counted, int>, MapODhash<Counted, int>,
                                           MapOLhash<Counted, int>, FlatMap<Counted, int>>;
  TYPED_TEST_SUITE(MapKeyCopies, Maps_By_Counted);
}

TYPED_TEST(MapCopies, KeysCopyNoValueAndValuesCopyEachValueOnce)
{
  constexpr int n = 20;
  TypeParam m;
  for (int i = 0; i < n; ++i)
    (void) m.insert(i, Counted(i));

  Counted::copies = Counted::moves = 0;
  const auto keys = m.keys();
  EXPECT_EQ(keys.size(), static_cast<size_t>(n));
  EXPECT_EQ(Counted::copies, 0);
  EXPECT_EQ(Counted::moves, 0);

  // each value is copied into the list, and not copied and then moved
  Counted::copies = Counted::moves = 0;
  const auto values = m.values();
  EXPECT_EQ(values.size(), static_cast<size_t>(n));
  EXPECT_EQ(Counted::copies, n);
  EXPECT_EQ(Counted::moves, 0);
  int sum = 0;
  values.for_each([&sum] (const Counted & c) { sum += c.v; });
  EXPECT_EQ(sum, n * (n - 1) / 2);
}

TYPED_TEST(MapKeyCopies, KeysCopyEachKeyOnceAndMoveNone)
{
  constexpr int n = 20;
  TypeParam m;
  for (int i = 0; i < n; ++i)
    (void) m.insert(Counted(i), i);

  Counted::copies = Counted::moves = 0;
  const auto keys = m.keys();
  EXPECT_EQ(keys.size(), static_cast<size_t>(n));
  EXPECT_EQ(Counted::copies, n);
  EXPECT_EQ(Counted::moves, 0);
  int sum = 0;
  keys.for_each([&sum] (const Counted & c) { sum += c.v; });
  EXPECT_EQ(sum, n * (n - 1) / 2);
}

// The list that a container gives of its items copies each one into the
// list: the function that maps() applies returns a reference to the item,
// and not a copy that maps() would then move
TEST(ItemCopies, TheListOfTheItemsOfAContainerCopiesEachOnceAndMovesNone)
{
  constexpr int n = 10;
  DynList<Counted> list;
  DynDlist<Counted> dlist;
  Array<Counted> array;
  Mixed mixed;
  for (int i = 0; i < n; ++i)
    {
      list.append(Counted(i));
      dlist.append(Counted(i));
      array.append(Counted(i));
      mixed.append(Counted(i));
    }

  auto expect_one_copy = [n] (const char * what, auto && make)
    {
      Counted::copies = Counted::moves = 0;
      const auto items = make();
      EXPECT_EQ(items.size(), static_cast<size_t>(n)) << what;
      EXPECT_EQ(Counted::copies, n) << what;
      EXPECT_EQ(Counted::moves, 0) << what;
      int sum = 0;
      items.for_each([&sum] (const Counted & c) { sum += c.v; });
      EXPECT_EQ(sum, n * (n - 1) / 2) << what;
    };
  expect_one_copy("DynList::items()", [&list] { return list.items(); });
  expect_one_copy("DynList::keys()", [&list] { return list.keys(); });
  expect_one_copy("DynDlist::items()", [&dlist] { return dlist.items(); });
  expect_one_copy("to_dynlist(DynDlist)", [&dlist] { return to_dynlist(dlist); });
  expect_one_copy("to_DynList(Array)", [&array] { return to_DynList(array); });
  expect_one_copy("KeysMixin::keys()", [&mixed] { return mixed.keys(); });
  expect_one_copy("KeysMixin::items()", [&mixed] { return mixed.items(); });
}

TEST(ItemCopies, TheRanksOfAListCopyEachItemOnce)
{
  DynList<Counted> list;
  DynDlist<Counted> dlist;
  for (const int v : {5, 3, 8, 1, 9, 2})
    {
      list.append(Counted(v));
      dlist.append(Counted(v));
    }
  const size_t expected[] = {3, 2, 4, 0, 5, 1};

  Counted::copies = 0;
  const auto r1 = ranks(list);
  EXPECT_EQ(Counted::copies, 6);
  Counted::copies = 0;
  const auto r2 = ranks(dlist);
  EXPECT_EQ(Counted::copies, 6);
  Counted::copies = 0;
  const auto p1 = pair_ranks(list);
  EXPECT_EQ(Counted::copies, 6);
  Counted::copies = 0;
  const auto p2 = pair_ranks(dlist);
  EXPECT_EQ(Counted::copies, 6);

  for (size_t i = 0; i < 6; ++i)
    {
      EXPECT_EQ(r1(i), expected[i]);
      EXPECT_EQ(r2(i), expected[i]);
      EXPECT_EQ(p1(i).second, expected[i]);
      EXPECT_EQ(p2(i).second, expected[i]);
    }
  EXPECT_EQ(p1(0).first, Counted(5));
  EXPECT_EQ(p2(5).first, Counted(2));
}

TEST(ItemCopies, ACopyOfARandomSetCopiesEachItemOnce)
{
  Random_Set<Counted> s;
  for (int i = 0; i < 10; ++i)
    s.append(Counted(i));

  Counted::copies = 0;
  Random_Set<Counted> copy(s);
  EXPECT_EQ(Counted::copies, 10);
  EXPECT_EQ(copy.size(), 10u);

  Random_Set<Counted> assigned;
  assigned.append(Counted(99));
  Counted::copies = 0;
  assigned = s;
  EXPECT_EQ(Counted::copies, 10);
  EXPECT_EQ(assigned.size(), 10u);
}
