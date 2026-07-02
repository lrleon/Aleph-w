
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
 * @file ah_iterator_test.cc
 * @brief Tests for Ah Iterator
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

#include <ah-ranges.H>
#include <array_it.H>
#include <tpl_array.H>
#include <tpl_dynArray.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynDlist.H>
#include <tpl_dynSetTree.H>

using namespace Aleph;

#if ALEPH_HAS_RANGES

#include <concepts>
#include <iterator>

namespace
{

template <class C>
concept HasBeginEnd = requires(C & c) {
  c.begin();
  c.end();
};

} // namespace

TEST(AhIteratorConcepts, DynDlistIteratorsSatisfyForwardIterator)
{
  using Iter = DynDlist<int>::iterator;
  using CIter = DynDlist<int>::const_iterator;

  static_assert(std::forward_iterator<Iter>);
  static_assert(std::forward_iterator<CIter>);

  static_assert(std::same_as<std::iter_reference_t<Iter>, std::iter_reference_t<const Iter>>);
  static_assert(std::same_as<std::iter_reference_t<CIter>, std::iter_reference_t<const CIter>>);

  SUCCEED();
}

TEST(AhIteratorConcepts, DynSetTreeIteratorsSatisfyForwardIterator)
{
  using Set = DynSetTree<int>;
  using Iter = Set::iterator;
  using CIter = Set::const_iterator;

  static_assert(std::forward_iterator<Iter>);
  static_assert(std::forward_iterator<CIter>);

  static_assert(std::same_as<std::iter_reference_t<Iter>, std::iter_reference_t<const Iter>>);
  static_assert(std::same_as<std::iter_reference_t<CIter>, std::iter_reference_t<const CIter>>);

  using Ref = std::iter_reference_t<Iter>;
  using Ptr = std::add_pointer_t<std::remove_reference_t<Ref>>;
  static_assert(std::same_as<decltype(std::declval<const Iter &>().operator->()), Ptr>,
                "operator-> must return a pointer consistent with operator*");

  SUCCEED();
}

TEST(AhIteratorConstCorrectness, ConstContainerBeginReturnsConstIterator)
{
  DynDlist<int> list;
  list.append(1);
  list.append(2);

  const DynDlist<int> & clist = list;

  static_assert(std::same_as<decltype(clist.begin()), DynDlist<int>::const_iterator>);
  static_assert(std::same_as<decltype(begin(clist)), DynDlist<int>::const_iterator>);

  SUCCEED();
}

TEST(AhIteratorConstCorrectness, MutableIteratorCannotBeCreatedFromConstContainer)
{
  using C = const DynDlist<int>;

  static_assert(std::same_as<decltype(std::declval<C &>().begin()), DynDlist<int>::const_iterator>);
  static_assert(std::same_as<decltype(begin(std::declval<C &>())), DynDlist<int>::const_iterator>);

  SUCCEED();
}

TEST(AhIteratorRuntime, RangeForWorksForMutableAndConst)
{
  DynDlist<int> list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  std::vector<int> v;
  for (int & x : list)
    v.push_back(x);

  ASSERT_EQ(v.size(), 5u);
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[4], 5);

  const DynDlist<int> & clist = list;
  int sum = 0;
  for (const int & x : clist)
    sum += x;

  EXPECT_EQ(sum, 15);
}

TEST(AhIteratorRuntime, MutableIteratorAllowsMutationForSequenceContainers)
{
  DynDlist<int> list;
  list.append(1);
  list.append(2);

  auto it = list.begin();
  ASSERT_NE(it, list.end());

  *it = 10;
  EXPECT_EQ(*list.begin(), 10);
}

TEST(AhIteratorRuntime, RangesAlgorithmsWork)
{
  DynSetTree<int> set;
  set.insert(5);
  set.insert(2);
  set.insert(8);
  set.insert(1);
  set.insert(9);

  EXPECT_TRUE(std::ranges::all_of(set, [](int x) { return x > 0; }));

  auto it = std::ranges::find(set, 5);
  ASSERT_NE(it, set.end());
  EXPECT_EQ(*it, 5);

  auto min_it = std::ranges::min_element(set);
  ASSERT_NE(min_it, set.end());
  EXPECT_EQ(*min_it, 1);
}

TEST(AhIteratorSemantics, DefaultConstructedIteratorsCompareEqual)
{
  using Iter = DynDlist<int>::iterator;

  Iter a;
  Iter b;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(AhIteratorSemantics, PostIncrementReturnsOldValue)
{
  DynDlist<int> list;
  list.append(1);
  list.append(2);

  auto it = list.begin();
  auto old = it++;

  EXPECT_EQ(*old, 1);
  EXPECT_EQ(*it, 2);
}

TEST(AhIteratorConcepts, DynArrayIteratorSatisfiesRandomAccess)
{
  using Iter = DynArray<int>::iterator;

  static_assert(std::random_access_iterator<Iter>);
  static_assert(std::same_as<
                  typename std::iterator_traits<Iter>::iterator_category,
                  std::random_access_iterator_tag>);

  SUCCEED();
}

TEST(AhIteratorConcepts, ArrayIteratorSatisfiesRandomAccess)
{
  using Iter = Array<int>::iterator;

  static_assert(std::random_access_iterator<Iter>);

  SUCCEED();
}

TEST(AhIteratorConcepts, ConstIteratorsArePromotedToRandomAccessToo)
{
  static_assert(std::random_access_iterator<DynArray<int>::const_iterator>);
  static_assert(std::random_access_iterator<Array<int>::const_iterator>);

  // Non-opted-in containers must keep their forward category.
  static_assert(std::forward_iterator<DynDlist<int>::const_iterator>);
  static_assert(not std::random_access_iterator<DynDlist<int>::const_iterator>);

  SUCCEED();
}

TEST(AhIteratorConcepts, DynArrayHeapIteratorStaysNonRandomAccessFailSafe)
{
  // The heap reuses DynArray::Iterator as a base but redefines get_pos() with
  // an offset. The self-referential marker prevents inheriting the
  // random-access promotion, so it must NOT be a random_access_iterator.
  using Iter = DynArrayHeap<int>::iterator;

  static_assert(std::forward_iterator<Iter>);
  static_assert(not std::random_access_iterator<Iter>);

  SUCCEED();
}

TEST(AhIteratorRandomAccess, DynArrayArithmeticIndexingAndOrdering)
{
  DynArray<int> a;
  for (int i = 0; i < 10; ++i)
    a.append(i);

  auto first = a.begin();

  EXPECT_EQ(a.end() - first, 10);
  EXPECT_EQ(*(first + 3), 3);
  EXPECT_EQ(*(3 + first), 3);  // symmetric operator+
  EXPECT_EQ(first[7], 7);

  auto it = first;
  it += 5;
  EXPECT_EQ(*it, 5);
  it -= 2;
  EXPECT_EQ(*it, 3);
  EXPECT_EQ(it - first, 3);

  EXPECT_TRUE(first < it);
  EXPECT_TRUE(it > first);
  EXPECT_TRUE(first <= first);
  EXPECT_TRUE(first >= first);

  --it;
  EXPECT_EQ(*it, 2);
  auto prev = it--;
  EXPECT_EQ(*prev, 2);
  EXPECT_EQ(*it, 1);
}

TEST(AhIteratorRandomAccess, StdSortAndRangesSortOnDynArray)
{
  DynArray<int> a;
  for (int v : {5, 2, 8, 1, 9, 3, 7, 4, 6, 0})
    a.append(v);

  std::sort(a.begin(), a.end());

  std::vector<int> v;
  for (int x : a)
    v.push_back(x);
  EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
  EXPECT_EQ(v.front(), 0);
  EXPECT_EQ(v.back(), 9);

  std::ranges::sort(a, std::ranges::greater{});
  v.clear();
  for (int x : a)
    v.push_back(x);
  EXPECT_EQ(v.front(), 9);
  EXPECT_EQ(v.back(), 0);
}

TEST(AhIteratorRandomAccess, StdSortOnArray)
{
  Array<int> a;
  for (int v : {3, 1, 2, 5, 4})
    a.append(v);

  std::sort(a.begin(), a.end());

  std::vector<int> v;
  for (int x : a)
    v.push_back(x);
  EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
  EXPECT_EQ(v.front(), 1);
  EXPECT_EQ(v.back(), 5);
}

TEST(AhIteratorSemantics, DefaultConstructedRandomAccessIteratorsCompareEqual)
{
  // std::regular (required by std::random_access_iterator) demands that two
  // value-initialized iterators compare equal. DynArray::Iterator::has_curr()
  // must therefore be null-safe on singular iterators.
  DynArray<int>::iterator a, b;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  Array<int>::iterator c, d;
  EXPECT_TRUE(c == d);
  EXPECT_FALSE(c != d);
}

TEST(AhIteratorRandomAccess, ConstIteratorArithmeticAndBinarySearch)
{
  DynArray<int> a;
  for (int i = 0; i < 10; ++i)
    a.append(i);

  const DynArray<int> &ca = a;
  auto first = ca.begin();

  EXPECT_EQ(ca.end() - first, 10);
  EXPECT_EQ(*(first + 3), 3);
  EXPECT_EQ(*(3 + first), 3);
  EXPECT_EQ(first[7], 7);
  EXPECT_TRUE(first < ca.end());

  auto it = first;
  it += 5;
  EXPECT_EQ(*it, 5);
  --it;
  EXPECT_EQ(*it, 4);
  it -= 2;
  EXPECT_EQ(*it, 2);
  EXPECT_EQ(it - first, 2);

  // O(log n) algorithms require random access on the const view.
  EXPECT_TRUE(std::binary_search(ca.begin(), ca.end(), 5));
  auto lb = std::lower_bound(ca.begin(), ca.end(), 7);
  ASSERT_NE(lb, ca.end());
  EXPECT_EQ(*lb, 7);

  Array<int> arr;
  for (int v : {1, 2, 3})
    arr.append(v);
  const Array<int> &carr = arr;
  EXPECT_EQ(carr.end() - carr.begin(), 3);
  EXPECT_EQ(carr.begin()[2], 3);
}

TEST(AhIteratorRandomAccess, EmptyContainersHaveZeroDistanceAndConsistentOrder)
{
  // Regression: Array_Iterator::end() used to leave pos == -1 on empty
  // containers (reset_last() with num_items == 0), so end() - begin() was -1
  // and begin() > end() while begin() == end() also held. The end state must
  // be position num_items (0 when empty).
  Array<int> a;
  EXPECT_EQ(a.end() - a.begin(), 0);
  EXPECT_TRUE(a.begin() == a.end());
  EXPECT_FALSE(a.begin() < a.end());
  EXPECT_FALSE(a.begin() > a.end());
  EXPECT_EQ(std::distance(a.begin(), a.end()), 0);
  std::sort(a.begin(), a.end()); // must be a harmless no-op
  EXPECT_EQ(a.size(), 0u);

  const Array<int> &ca = a;
  EXPECT_EQ(ca.end() - ca.begin(), 0);
  EXPECT_TRUE(ca.begin() == ca.end());

  DynArray<int> d;
  EXPECT_EQ(d.end() - d.begin(), 0);
  EXPECT_TRUE(d.begin() == d.end());
}

TEST(AhIteratorRandomAccess, ArrayIteratorCircularSetPosMatchesEnd)
{
  // Physical buffer of size 5; logical sequence of 4 items starting at physical
  // index 3 and wrapping around: logical [0..3] -> physical [3, 4, 0, 1].
  int buf[5] = {20, 30, 999, 0, 10};
  Array_Iterator<int> it(buf, 5, 4, 3, 1); // ptr, dim, num_items, first, last

  it.set_pos(0);
  EXPECT_EQ(it.get_curr(), 0);   // buf[3]
  it.set_pos(1);
  EXPECT_EQ(it.get_curr(), 10);  // buf[4]
  it.set_pos(2);
  EXPECT_EQ(it.get_curr(), 20);  // buf[0]
  it.set_pos(3);
  EXPECT_EQ(it.get_curr(), 30);  // buf[1]

  // set_pos(num_items) must reproduce the exact end() state.
  Array_Iterator<int> at_end(buf, 5, 4, 3, 1);
  at_end.end();

  it.set_pos(4);
  EXPECT_FALSE(it.has_curr());
  EXPECT_EQ(it.get_pos(), at_end.get_pos());
}

#else // !ALEPH_HAS_RANGES

// Dummy test when ranges/concepts are not available
TEST(AhIterator, ConceptsNotAvailable)
{
  GTEST_SKIP() << "C++20 concepts not fully supported on this platform";
}

#endif // ALEPH_HAS_RANGES
