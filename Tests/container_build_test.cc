/*
                          Aleph_w

                           / \  | | ___ _ __ | |__      __      __
                          / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
                         / ___ \| |  __/ |_) | | | |_____\ V  V /
                        /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                                       |_|

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
 * @file container_build_test.cc
 * @brief `C::build(items...)` builds the container that `C{items...}` builds,
 *        with the insertion rules of `C`, but forwarding the items: rvalues are
 *        moved and any type convertible to the item type is accepted. A class
 *        derived from a container builds itself, not its base.
 */

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <ahFunctional.H>
#include <al-domain.H>
#include <htlist.H>
#include <tpl_array.H>
#include <tpl_arrayHeap.H>
#include <tpl_arrayQueue.H>
#include <tpl_arrayStack.H>
#include <tpl_dynArray.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynBinHeap.H>
#include <tpl_dynDlist.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynListStack.H>
#include <tpl_dynMapOhash.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetHash.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynSkipList.H>
#include <tpl_dynTreap.H>
#include <tpl_dynarray_set.H>
#include <tpl_hash.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>

using namespace Aleph;

namespace
{
  // The items of `c`, in traversal order.
  template <class C>
  std::vector<typename C::Item_Type> items_of(const C & c)
  {
    std::vector<typename C::Item_Type> v;
    c.for_each([&v](const auto & item) { v.push_back(item); });
    return v;
  }

  // Counts its copies and moves.
  struct Counted
  {
    static inline int copies = 0;
    static inline int moves = 0;

    int value = 0;

    Counted() = default;
    Counted(const int v) : value(v) {}
    Counted(const Counted & other) : value(other.value) { ++copies; }
    Counted(Counted && other) noexcept : value(other.value) { ++moves; }

    Counted & operator = (const Counted & other)
    {
      value = other.value;
      ++copies;
      return *this;
    }

    Counted & operator = (Counted && other) noexcept
    {
      value = other.value;
      ++moves;
      return *this;
    }

    static void reset() noexcept { copies = moves = 0; }
  };

  // Converts to int by throwing.
  struct Throws_On_Conversion
  {
    operator int() const { throw std::runtime_error("conversion failed"); }
  };

  template <class C, class... Items>
  concept Buildable = requires (Items &&... items)
    {
      C::build(std::forward<Items>(items)...);
    };

  template <class C, class... Items>
  concept Can_Nappend = requires (C & c, Items &&... items)
    {
      c.nappend(std::forward<Items>(items)...);
    };

  template <class C, class... Items>
  concept Can_Ninsert = requires (C & c, Items &&... items)
    {
      c.ninsert(std::forward<Items>(items)...);
    };
}

// ---------------------------------------------------------------------------
// The result type
// ---------------------------------------------------------------------------

TEST(ContainerBuild, ContainersBuildTheirOwnType)
{
  static_assert(std::is_same_v<decltype(DynList<int>::build(1)), DynList<int>>);
  static_assert(std::is_same_v<decltype(DynDlist<int>::build(1)), DynDlist<int>>);
  static_assert(std::is_same_v<decltype(Array<int>::build(1)), Array<int>>);
  static_assert(std::is_same_v<decltype(DynArray<int>::build(1)), DynArray<int>>);
  static_assert(std::is_same_v<decltype(ArrayStack<int>::build(1)), ArrayStack<int>>);
  static_assert(std::is_same_v<decltype(ArrayQueue<int>::build(1)), ArrayQueue<int>>);
  static_assert(std::is_same_v<decltype(DynSetTree<int>::build(1)), DynSetTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetHash<int>::build(1)), DynSetHash<int>>);
  static_assert(std::is_same_v<decltype(OLhashTable<int>::build(1)), OLhashTable<int>>);
  static_assert(std::is_same_v<decltype(DynBinHeap<int>::build(1)), DynBinHeap<int>>);
}

TEST(ContainerBuild, DerivedClassesBuildThemselves)
{
  // Without Derived_Build() they would inherit a build() of their base.
  static_assert(std::is_same_v<decltype(DynSetBinTree<int>::build(1)), DynSetBinTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetAvlTree<int>::build(1)), DynSetAvlTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetSplayTree<int>::build(1)), DynSetSplayTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetSplayRkTree<int>::build(1)), DynSetSplayRkTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetRandTree<int>::build(1)), DynSetRandTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetTreap<int>::build(1)), DynSetTreap<int>>);
  static_assert(std::is_same_v<decltype(DynSetTreapRk<int>::build(1)), DynSetTreapRk<int>>);
  static_assert(std::is_same_v<decltype(DynSetAvlRkTree<int>::build(1)), DynSetAvlRkTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetRbTree<int>::build(1)), DynSetRbTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetTdRbTree<int>::build(1)), DynSetTdRbTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetRbRkTree<int>::build(1)), DynSetRbRkTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetTdRbRkTree<int>::build(1)), DynSetTdRbRkTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetHtdRbTree<int>::build(1)), DynSetHtdRbTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetHtdRbRkTree<int>::build(1)), DynSetHtdRbRkTree<int>>);
  static_assert(std::is_same_v<decltype(DynSetLhash<int>::build(1)), DynSetLhash<int>>);
  static_assert(std::is_same_v<decltype(DynSetLinHash<int>::build(1)), DynSetLinHash<int>>);
  static_assert(std::is_same_v<decltype(DynArray_Set<int>::build(1)), DynArray_Set<int>>);
  static_assert(std::is_same_v<decltype(HashSet<int>::build(1)), HashSet<int>>);

  using P = std::pair<int, int>;
  static_assert(std::is_same_v<decltype(DynMapTree<int, int>::build(P{})), DynMapTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapBinTree<int, int>::build(P{})), DynMapBinTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapAvlTree<int, int>::build(P{})), DynMapAvlTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapRbTree<int, int>::build(P{})), DynMapRbTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapRandTree<int, int>::build(P{})), DynMapRandTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapTreap<int, int>::build(P{})), DynMapTreap<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapTreapRk<int, int>::build(P{})), DynMapTreapRk<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapSplayTree<int, int>::build(P{})), DynMapSplayTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynTreapTree<int, int>::build(P{})), DynTreapTree<int, int>>);
  static_assert(std::is_same_v<decltype(DynMapHash<int, int>::build(P{})), DynMapHash<int, int>>);
  static_assert(std::is_same_v<decltype(MapOLhash<int, int>::build(P{})), MapOLhash<int, int>>);
  static_assert(std::is_same_v<decltype(HashMap<int, int>::build(P{})), HashMap<int, int>>);
  static_assert(std::is_same_v<decltype(MapODhash<int, int>::build(P{})), MapODhash<int, int>>);
  static_assert(std::is_same_v<decltype(MapOpenHash<int, int>::build(P{})), MapOpenHash<int, int>>);
  static_assert(std::is_same_v<decltype(AlDomain<int>::build(1)), AlDomain<int>>);

  // And they hold what the base container would hold.
  auto avl = DynSetAvlTree<int>::build(3, 1, 2, 1);
  EXPECT_EQ(items_of(avl), (std::vector<int>{1, 2, 3}));
  auto map = DynMapAvlTree<int, std::string>::build(std::pair{2, "two"}, std::pair{1, "one"});
  EXPECT_EQ(map.size(), 2u);
  EXPECT_EQ(map.find(1), "one");
}

// ---------------------------------------------------------------------------
// The rules of each container, as with the braces
// ---------------------------------------------------------------------------

TEST(ContainerBuild, SequencesKeepTheArgumentOrder)
{
  EXPECT_EQ(items_of(DynList<int>::build(3, 1, 2)), (std::vector<int>{3, 1, 2}));
  EXPECT_EQ(items_of(DynDlist<int>::build(3, 1, 2)), (std::vector<int>{3, 1, 2}));
  EXPECT_EQ(items_of(Array<int>::build(3, 1, 2)), (std::vector<int>{3, 1, 2}));
  EXPECT_EQ(items_of(DynArray<int>::build(3, 1, 2)), (std::vector<int>{3, 1, 2}));

  // The same container as the braces.
  EXPECT_EQ(items_of(DynList<int>::build(3, 1, 2)), items_of(DynList<int>{3, 1, 2}));
  EXPECT_EQ(items_of(Array<int>::build(3, 1, 2)), items_of(Array<int>{3, 1, 2}));
}

TEST(ContainerBuild, ArrayBuildOfOneIntegerIsOneItem)
{
  // Unlike the removed Array<T>(n), it never means a capacity.
  const auto a = Array<int>::build(10);
  ASSERT_EQ(a.size(), 1u);
  EXPECT_EQ(a[0], 10);

  const auto reserved = Array<int>::create_reserved(10);
  EXPECT_EQ(reserved.size(), 0u);
}

TEST(ContainerBuild, StacksEndWithTheLastItemOnTop)
{
  auto s = ArrayStack<int>::build(1, 2, 3);
  EXPECT_EQ(s.top(), 3);
  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);

  auto fs = FixedStack<int>::build(1, 2, 3);
  EXPECT_EQ(fs.top(), 3);
  EXPECT_EQ(fs.size(), 3u);

  auto ls = DynListStack<int>::build(1, 2, 3);
  EXPECT_EQ(ls.top(), 3);
  EXPECT_EQ(ls.size(), 3u);
}

TEST(ContainerBuild, QueuesEndWithTheFirstItemInFront)
{
  auto q = ArrayQueue<int>::build(1, 2, 3);
  EXPECT_EQ(q.front(), 1);
  EXPECT_EQ(q.get(), 1);
  EXPECT_EQ(q.get(), 2);
  EXPECT_EQ(q.get(), 3);

  auto fq = FixedQueue<int>::build(1, 2, 3);
  EXPECT_EQ(fq.front(), 1);
  EXPECT_EQ(fq.size(), 3u);

  auto lq = DynListQueue<int>::build(1, 2, 3);
  EXPECT_EQ(lq.get(), 1);
  EXPECT_EQ(lq.get(), 2);
  EXPECT_EQ(lq.get(), 3);
}

TEST(ContainerBuild, SetsKeepOneItemPerKey)
{
  auto tree = DynSetTree<int>::build(3, 1, 2, 1, 3);
  EXPECT_EQ(items_of(tree), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(items_of(tree), items_of(DynSetTree<int>{3, 1, 2, 1, 3}));

  EXPECT_EQ(DynSetHash<int>::build(3, 1, 2, 1).size(), 3u);
  EXPECT_EQ(DynSetLinHash<int>::build(3, 1, 2, 1).size(), 3u);
  EXPECT_EQ(OLhashTable<int>::build(3, 1, 2, 1).size(), 3u);
  EXPECT_EQ(ODhashTable<int>::build(3, 1, 2, 1).size(), 3u);
  EXPECT_EQ(HashSet<int>::build(3, 1, 2, 1).size(), 3u);

  // DynArray_Set allows repeated items.
  EXPECT_EQ(DynArray_Set<int>::build(3, 1, 3).size(), 3u);

  // A skip list too, for rvalue and for lvalue items: its append() of an
  // rvalue used to throw for a key already in the set.
  const auto skip = DynSkipList<int>::build(3, 1, 2, 1);
  EXPECT_EQ(items_of(skip), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(items_of(skip), items_of(DynSkipList<int>{3, 1, 2, 1}));
  const int one = 1;
  EXPECT_EQ(DynSkipList<int>::build(one, one).size(), 1u);
}

TEST(ContainerBuild, MapsKeepTheFirstOfTwoEqualKeys)
{
  auto tree = DynMapTree<int, std::string>::build(std::pair{1, "one"},
                                                  std::pair{2, "two"},
                                                  std::pair{1, "uno"});
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_EQ(tree.find(1), "one");
  EXPECT_EQ(tree.find(2), "two");

  auto hash = DynMapHash<int, std::string>::build(std::pair{1, "one"},
                                                  std::pair{1, "uno"});
  EXPECT_EQ(hash.size(), 1u);
  EXPECT_EQ(hash.find(1), "one");

  auto open = MapOLhash<int, std::string>::build(std::pair{1, "one"},
                                                 std::pair{1, "uno"});
  EXPECT_EQ(open.size(), 1u);
  EXPECT_EQ(open.find(1), "one");
}

TEST(ContainerBuild, HeapsKeepEveryItem)
{
  auto bin = DynBinHeap<int>::build(3, 1, 2, 1);
  EXPECT_EQ(bin.size(), 4u);
  EXPECT_EQ(bin.top(), 1);

  auto dyn = DynArrayHeap<int>::build(3, 1, 2, 1);
  EXPECT_EQ(dyn.size(), 4u);
  EXPECT_EQ(dyn.top(), 1);

  auto fixed = ArrayHeap<int>::build(3, 1, 2, 1);
  EXPECT_EQ(fixed.size(), 4u);
  EXPECT_EQ(fixed.top(), 1);
}

TEST(ContainerBuild, NoItemsBuildsAnEmptyContainer)
{
  EXPECT_TRUE(DynList<int>::build().is_empty());
  EXPECT_TRUE(DynSetTree<int>::build().is_empty());
  EXPECT_TRUE((DynMapTree<int, int>::build().is_empty()));
}

// ---------------------------------------------------------------------------
// What the braces cannot do
// ---------------------------------------------------------------------------

TEST(ContainerBuild, MoveOnlyItems)
{
  auto list = DynList<std::unique_ptr<int>>::build(std::make_unique<int>(1),
                                                   std::make_unique<int>(2));
  ASSERT_EQ(list.size(), 2u);
  EXPECT_EQ(*list.get_first(), 1);
  EXPECT_EQ(*list.get_last(), 2);

  auto array = Array<std::unique_ptr<int>>::build(std::make_unique<int>(7));
  ASSERT_EQ(array.size(), 1u);
  EXPECT_EQ(*array[0], 7);

  auto dlist = DynDlist<std::unique_ptr<int>>::build(std::make_unique<int>(5));
  EXPECT_EQ(*dlist.get_first(), 5);
}

TEST(ContainerBuild, RvaluesAreMovedAndLvaluesCopiedOnce)
{
  Counted::reset();
  Counted lvalue(1);
  auto list = DynList<Counted>::build(lvalue, Counted(2));
  EXPECT_EQ(Counted::copies, 1);  // only the lvalue
  EXPECT_GE(Counted::moves, 1);
  ASSERT_EQ(list.size(), 2u);
  EXPECT_EQ(list.get_first().value, 1);
  EXPECT_EQ(list.get_last().value, 2);
}

TEST(ContainerBuild, ItemsConvertWithoutNarrowingErrors)
{
  // A braced list rejects the non-constant int as a narrowing conversion.
  const int n = 1;
  EXPECT_EQ(items_of(Array<double>::build(n, 2.5)), (std::vector<double>{1.0, 2.5}));
  EXPECT_EQ(items_of(DynList<std::string>::build("a", std::string("b"))),
            (std::vector<std::string>{"a", "b"}));
}

TEST(ContainerBuild, OnlyItemsAreAccepted)
{
  static_assert(Buildable<DynList<int>, int, long, short>);
  static_assert(not Buildable<DynList<int>, std::string>);
  // DynList::append(DynList &) concatenates a list: build() takes items.
  static_assert(not Buildable<DynList<int>, DynList<int> &>);
  // A map takes pairs: a bare key would get a default-constructed value.
  static_assert(not Buildable<DynMapTree<int, std::string>, int>);
  static_assert(Buildable<DynMapTree<int, std::string>, std::pair<int, const char *>>);
  SUCCEED();
}

TEST(ContainerBuild, ThrowingItemLeavesNothingBehind)
{
  // The partially built container is destroyed; the sanitizer jobs check
  // that nothing leaks.
  EXPECT_THROW((void) DynList<int>::build(1, Throws_On_Conversion{}, 3), std::runtime_error);
  EXPECT_THROW((void) DynSetTree<int>::build(1, Throws_On_Conversion{}), std::runtime_error);
}

// ---------------------------------------------------------------------------
// The functions that append several items
// ---------------------------------------------------------------------------

TEST(ContainerBuild, NappendAndNinsertForwardTheirItems)
{
  DynList<std::unique_ptr<int>> list;
  EXPECT_EQ(list.nappend(std::make_unique<int>(1), std::make_unique<int>(2)), 2u);
  EXPECT_EQ(list.ninsert(std::make_unique<int>(0)), 1u);
  ASSERT_EQ(list.size(), 3u);
  EXPECT_EQ(*list.get_first(), 0);
  EXPECT_EQ(*list.get_last(), 2);
}

TEST(ContainerBuild, NappendKeepsTheArgumentOrderAndNinsertFollowsInsert)
{
  DynList<int> appended;
  EXPECT_EQ(appended.nappend(1, 2, 3), 3u);
  EXPECT_EQ(items_of(appended), (std::vector<int>{1, 2, 3}));

  // DynList::insert() puts each item first: the last argument ends first.
  DynList<int> inserted;
  EXPECT_EQ(inserted.ninsert(1, 2, 3), 3u);
  EXPECT_EQ(items_of(inserted), (std::vector<int>{3, 2, 1}));

  // ArrayQueue::insert() is put(): the queue keeps the argument order.
  ArrayQueue<int> queue;
  EXPECT_EQ(queue.ninsert(1, 2, 3), 3u);
  EXPECT_EQ(items_of(queue), (std::vector<int>{1, 2, 3}));
}

TEST(ContainerBuild, NappendAndNinsertCountTheItemsAdded)
{
  // They returned the number of arguments, also when a set refused some.
  DynSetTree<int> tree;
  EXPECT_EQ(tree.ninsert(1, 1, 2), 2u);
  EXPECT_EQ(tree.nappend(2, 3, 3), 1u);
  EXPECT_EQ(items_of(tree), (std::vector<int>{1, 2, 3}));

  DynSetHash<int> hash;
  EXPECT_EQ(hash.nappend(5, 5, 5), 1u);
  EXPECT_EQ(hash.ninsert(5, 6), 1u);
  EXPECT_EQ(hash.size(), 2u);

  using Pair = std::pair<int, std::string>;
  DynMapTree<int, std::string> map;
  EXPECT_EQ(map.nappend(Pair{1, "one"}, Pair{1, "uno"}, Pair{2, "two"}), 2u);
  EXPECT_EQ(map.size(), 2u);
  EXPECT_EQ(map.find(1), "one");

  // ninsert() of a skip list counts the keys it adds. Its append() returns
  // the key already in the set, so nappend() cannot tell them apart.
  DynSkipList<int> skip;
  EXPECT_EQ(skip.ninsert(1, 1, 2), 2u);
  (void) skip.nappend(2, 3);
  EXPECT_EQ(items_of(skip), (std::vector<int>{1, 2, 3}));

  // The free functions follow the same rule.
  DynSetTree<int> other;
  EXPECT_EQ(append_in_container(other, 1, 1, 2), 2u);
  EXPECT_EQ(insert_in_container(other, 2, 3), 1u);
  EXPECT_EQ(items_of(other), (std::vector<int>{1, 2, 3}));

  // A sequence adds every item.
  DynList<int> list;
  EXPECT_EQ(list.nappend(1, 1, 1), 3u);
  EXPECT_EQ(append_in_container(list, 1, 1), 2u);
  EXPECT_EQ(list.size(), 5u);
}

TEST(ContainerBuild, NappendAndNinsertKeepTheItemsAddedBeforeAThrow)
{
  DynList<int> appended;
  EXPECT_THROW((void) appended.nappend(1, 2, Throws_On_Conversion{}, 4), std::runtime_error);
  EXPECT_EQ(items_of(appended), (std::vector<int>{1, 2}));

  DynList<int> inserted;
  EXPECT_THROW((void) inserted.ninsert(1, Throws_On_Conversion{}, 3), std::runtime_error);
  EXPECT_EQ(items_of(inserted), (std::vector<int>{1}));
}

TEST(ContainerBuild, NappendAndNinsertConvertAndMoveTheirItems)
{
  DynList<long> longs;
  EXPECT_EQ(longs.nappend(1, short{2}, 3L), 3u);
  EXPECT_EQ(items_of(longs), (std::vector<long>{1, 2, 3}));

  DynList<std::string> strings;
  EXPECT_EQ(strings.ninsert("b", std::string("a")), 2u);
  EXPECT_EQ(items_of(strings), (std::vector<std::string>{"a", "b"}));

  Counted::reset();
  Counted lvalue(1);
  DynList<Counted> counted;
  EXPECT_EQ(counted.nappend(lvalue, Counted(2)), 2u);
  EXPECT_EQ(Counted::copies, 1);  // only the lvalue
  EXPECT_EQ(counted.ninsert(lvalue, Counted(3)), 2u);
  EXPECT_EQ(Counted::copies, 2);

  // Only items: neither another type nor a whole list.
  static_assert(Can_Nappend<DynList<int>, int, long>);
  static_assert(Can_Ninsert<DynList<int>, int, long>);
  static_assert(not Can_Nappend<DynList<int>, std::string>);
  static_assert(not Can_Ninsert<DynList<int>, std::string>);
  static_assert(not Can_Nappend<DynList<int>, DynList<int> &>);
}

TEST(ContainerBuild, BuildFunctionsForwardTheirItems)
{
  auto list = build_dynlist<std::unique_ptr<int>>(std::make_unique<int>(1));
  EXPECT_EQ(*list.get_first(), 1);

  auto array = build_array<std::unique_ptr<int>>(std::make_unique<int>(2));
  EXPECT_EQ(*array[0], 2);

  auto set = build_container<DynSetAvlTree<int>>(3, 1, 2);
  static_assert(std::is_same_v<decltype(set), DynSetAvlTree<int>>);
  EXPECT_EQ(items_of(set), (std::vector<int>{1, 2, 3}));
}

TEST(ContainerBuild, AppendInContainerTreatsEveryArgumentAsAnItem)
{
  // The former recursive implementation took a leading size_t lvalue for
  // its internal counter instead of appending it.
  DynList<size_t> list;
  size_t first = 7;
  EXPECT_EQ(append_in_container(list, first, size_t{8}), 2u);
  EXPECT_EQ(items_of(list), (std::vector<size_t>{7, 8}));
  EXPECT_EQ(first, 7u);
}
