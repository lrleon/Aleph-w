
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

#include <type_traits>
#include <utility>
#include <vector>

#include <ah-ranges.H>
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

#else // !ALEPH_HAS_RANGES

// Dummy test when ranges/concepts are not available
TEST(AhIterator, ConceptsNotAvailable)
{
  GTEST_SKIP() << "C++20 concepts not fully supported on this platform";
}

#endif // ALEPH_HAS_RANGES
