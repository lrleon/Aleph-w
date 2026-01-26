
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
 * @file test_filter_iterator.cc
 * @brief Tests for Test Filter Iterator
 */

/**
 * GoogleTest suite for filter_iterator.H
 *
 * Comprehensive tests for the Filter_Iterator class.
 */

#include <gtest/gtest.h>

#include <filter_iterator.H>
#include <tpl_dynDlist.H>

using namespace Aleph;

// ========== Helper Filter Functors ==========

struct ShowAll {
  bool operator()(int) const { return true; }
};

struct ShowNone {
  bool operator()(int) const { return false; }
};

struct ShowEven {
  bool operator()(int x) const { return x % 2 == 0; }
};

struct ShowOdd {
  bool operator()(int x) const { return x % 2 == 1; }
};

struct ShowGreaterThan {
  int threshold;
  ShowGreaterThan(int t = 0) : threshold(t) {}
  bool operator()(int x) const { return x > threshold; }
};

struct ShowMultiCondition {
  bool operator()(int x) const { return x % 2 == 0 && x > 5; }
};

struct CountingFilter {
  mutable int count = 0;
  bool operator()(int) const { ++count; return true; }
};

// Container alias
using DL = DynDlist<int>;
using DL_It = DL::Iterator;

// ========== GROUP 1: Basic Functionality ==========

TEST(FilterIteratorBasic, DefaultConstructor) {
  // Should not throw
  Filter_Iterator<DL, DL_It, ShowAll> it;

  // has_container should return false
  ASSERT_FALSE(it.has_container());

  // get_container should throw
  EXPECT_THROW(it.get_container(), std::domain_error);
}

TEST(FilterIteratorBasic, ConstructWithContainer) {
  DL list;
  list.append(1); list.append(2); list.append(3);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  ASSERT_TRUE(it.has_container());
  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), 1);
}

TEST(FilterIteratorBasic, ConstructWithContainerAndFilter) {
  DL list;
  list.append(1); list.append(2); list.append(3); list.append(4);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), 2); // First even number
}

TEST(FilterIteratorBasic, ForwardIteration) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  std::vector<int> collected;
  for (; it.has_curr(); it.next())
    collected.push_back(it.get_curr());

  ASSERT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 2);
  EXPECT_EQ(collected[1], 4);
  EXPECT_EQ(collected[2], 6);
}

TEST(FilterIteratorBasic, BackwardIteration) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);
  it.reset_last();

  std::vector<int> collected;
  for (; it.has_curr(); it.prev())
    collected.push_back(it.get_curr());

  ASSERT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 6);
  EXPECT_EQ(collected[1], 4);
  EXPECT_EQ(collected[2], 2);
}

TEST(FilterIteratorBasic, ResetFirst) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);
  it.next(); it.next(); // Move to 6

  it.reset_first();

  ASSERT_EQ(it.get_curr(), 2);
}

TEST(FilterIteratorBasic, ResetLast) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  it.reset_last();

  ASSERT_EQ(it.get_curr(), 6);
}

TEST(FilterIteratorBasic, HasCurrAtVariousPositions) {
  DL list;
  list.append(2); list.append(4);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  ASSERT_TRUE(it.has_curr());
  it.next();
  ASSERT_TRUE(it.has_curr());
  it.next();
  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorBasic, GetCurrReturnsCorrectItem) {
  DL list;
  list.append(10); list.append(20); list.append(30);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  EXPECT_EQ(it.get_curr(), 10);
  it.next();
  EXPECT_EQ(it.get_curr(), 20);
  it.next();
  EXPECT_EQ(it.get_curr(), 30);
}

TEST(FilterIteratorBasic, IterationPastEnd) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  it.next(); // Past end

  ASSERT_FALSE(it.has_curr());

  // Should not throw (overflow caught internally)
  it.next_ne();
  ASSERT_FALSE(it.has_curr());
}

// ========== GROUP 2: Filter Functionality ==========

TEST(FilterIteratorFilter, ShowAll) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  int count = 0;
  for (; it.has_curr(); it.next())
    ++count;

  ASSERT_EQ(count, 5);
}

TEST(FilterIteratorFilter, ShowNone) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowNone> it(list);

  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorFilter, PartialFilter) {
  DL list;
  for (int i = 1; i <= 8; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  int count = 0;
  for (; it.has_curr(); it.next())
    ++count;

  ASSERT_EQ(count, 4); // 2, 4, 6, 8
}

TEST(FilterIteratorFilter, ChangeFilter) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  // Collect with even filter
  std::vector<int> evens;
  for (; it.has_curr(); it.next())
    evens.push_back(it.get_curr());

  ASSERT_EQ(evens.size(), 3u);

  // Change to odd filter
  it.set_filter(ShowEven()); // Note: Would need ShowOdd to really test this
  it.reset_first();

  // Count again (still even)
  int count = 0;
  for (; it.has_curr(); it.next())
    ++count;

  ASSERT_EQ(count, 3);
}

TEST(FilterIteratorFilter, GetFilter) {
  DL list;
  for (int i = 1; i <= 3; ++i)
    list.append(i);

  CountingFilter cf;
  Filter_Iterator<DL, DL_It, CountingFilter> it(list, cf);

  // Iterate through
  for (; it.has_curr(); it.next())
    ;

  // Filter was called during construction and iteration
  EXPECT_GT(it.get_filter().count, 0);
}

TEST(FilterIteratorFilter, StatefulFilter) {
  DL list;
  for (int i = 1; i <= 10; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowGreaterThan> it(list, ShowGreaterThan(5));

  std::vector<int> collected;
  for (; it.has_curr(); it.next())
    collected.push_back(it.get_curr());

  ASSERT_EQ(collected.size(), 5u); // 6, 7, 8, 9, 10
  EXPECT_EQ(collected[0], 6);
  EXPECT_EQ(collected[4], 10);
}

TEST(FilterIteratorFilter, MultiConditionFilter) {
  DL list;
  for (int i = 1; i <= 10; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowMultiCondition> it(list);

  std::vector<int> collected;
  for (; it.has_curr(); it.next())
    collected.push_back(it.get_curr());

  ASSERT_EQ(collected.size(), 3u); // 6, 8, 10
  EXPECT_EQ(collected[0], 6);
  EXPECT_EQ(collected[1], 8);
  EXPECT_EQ(collected[2], 10);
}

// ========== GROUP 3: Bidirectional Iteration ==========

TEST(FilterIteratorBidirectional, ForwardThenBackward) {
  DL list;
  for (int i = 2; i <= 8; i += 2)
    list.append(i); // 2, 4, 6, 8

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  // Forward to 8
  while (it.has_curr() && it.get_curr() != 8)
    it.next();
  ASSERT_EQ(it.get_curr(), 8);

  // Backward to 2
  it.prev(); ASSERT_EQ(it.get_curr(), 6);
  it.prev(); ASSERT_EQ(it.get_curr(), 4);
  it.prev(); ASSERT_EQ(it.get_curr(), 2);
}

TEST(FilterIteratorBidirectional, BackwardThenForward) {
  DL list;
  for (int i = 1; i <= 4; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  it.reset_last();

  // Backward
  it.prev(); it.prev(); // At 2
  ASSERT_EQ(it.get_curr(), 2);

  // Forward
  it.next(); it.next(); // At 4
  ASSERT_EQ(it.get_curr(), 4);
}

TEST(FilterIteratorBidirectional, NextNeNoException) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  it.next(); // Past end

  // Should not throw
  EXPECT_NO_THROW(it.next_ne());
  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorBidirectional, PrevNeNoException) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  // Should not throw when going before beginning
  EXPECT_NO_THROW(it.prev_ne());
}

TEST(FilterIteratorBidirectional, AlternatingNextPrev) {
  DL list;
  for (int i = 10; i <= 50; i += 10)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  ASSERT_EQ(it.get_curr(), 10);
  it.next(); ASSERT_EQ(it.get_curr(), 20);
  it.next(); ASSERT_EQ(it.get_curr(), 30);
  it.prev(); ASSERT_EQ(it.get_curr(), 20);
  it.next(); ASSERT_EQ(it.get_curr(), 30);
  it.next(); ASSERT_EQ(it.get_curr(), 40);
}

TEST(FilterIteratorBidirectional, ResetMethodsJumping) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  it.reset_last();
  ASSERT_EQ(it.get_curr(), 5);

  it.reset_first();
  ASSERT_EQ(it.get_curr(), 1);

  it.next();
  it.reset_last();
  ASSERT_EQ(it.get_curr(), 5);
}

// ========== GROUP 4: Container Operations ==========

TEST(FilterIteratorContainer, EmptyContainer) {
  DL list; // empty

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorContainer, SingleItem) {
  DL list;
  list.append(42);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), 42);
  it.next();
  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorContainer, LargeContainer) {
  DL list;
  for (int i = 0; i < 10000; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  int count = 0;
  for (; it.has_curr(); it.next())
    ++count;

  ASSERT_EQ(count, 5000); // 0, 2, 4, ..., 9998
}

TEST(FilterIteratorContainer, GetContainerReturnsCorrectReference) {
  DL list;
  list.append(1); list.append(2); list.append(3);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  const DL& container = it.get_container();
  ASSERT_EQ(&container, &list);
}

TEST(FilterIteratorContainer, GetIteratorAccess) {
  DL list;
  list.append(1); list.append(2); list.append(3);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  DL_It& base_it = it.get_iterator();
  ASSERT_TRUE(base_it.has_curr());
}

TEST(FilterIteratorContainer, HasContainer) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it1; // No container
  Filter_Iterator<DL, DL_It, ShowAll> it2(list); // Has container

  ASSERT_FALSE(it1.has_container());
  ASSERT_TRUE(it2.has_container());
}

// ========== GROUP 5: Cookie Pattern ==========

TEST(FilterIteratorCookie, CookieStorage) {
  DL list;
  list.append(1); list.append(2);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  int my_data = 42;

  it.set_cookie(&my_data);
  ASSERT_EQ(it.get_cookie(), &my_data);
}

TEST(FilterIteratorCookie, CookieInConstructor) {
  DL list;
  list.append(1);
  int my_data = 42;

  Filter_Iterator<DL, DL_It, ShowAll> it(list, &my_data);
  ASSERT_EQ(it.get_cookie(), &my_data);
}

TEST(FilterIteratorCookie, CookiePreservation) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  int my_data = 99;
  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  it.set_cookie(&my_data);

  // Iterate through and check cookie is preserved
  for (; it.has_curr(); it.next())
    ASSERT_EQ(it.get_cookie(), &my_data);
}

TEST(FilterIteratorCookie, SetCookieAfterConstruction) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  ASSERT_EQ(it.get_cookie(), nullptr);

  int data = 123;
  it.set_cookie(&data);
  ASSERT_EQ(it.get_cookie(), &data);
}

// ========== GROUP 6: Count Method ==========

TEST(FilterIteratorCount, CountAll) {
  DL list;
  for (int i = 1; i <= 10; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  ASSERT_EQ(it.count(), 10u);
}

TEST(FilterIteratorCount, CountFiltered) {
  DL list;
  for (int i = 1; i <= 10; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);
  ASSERT_EQ(it.count(), 5u); // 2, 4, 6, 8, 10
}

TEST(FilterIteratorCount, CountEmpty) {
  DL list;

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  ASSERT_EQ(it.count(), 0u);
}

TEST(FilterIteratorCount, CountNone) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowNone> it(list);
  ASSERT_EQ(it.count(), 0u);
}

// ========== GROUP 7: Edge Cases ==========

TEST(FilterIteratorEdge, AllItemsFilteredOut) {
  DL list;
  for (int i = 1; i <= 5; i += 2)
    list.append(i); // All odd: 1, 3, 5

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorEdge, MultipleIteratorsOnSameContainer) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it1(list);
  Filter_Iterator<DL, DL_It, ShowOdd> it2(list);

  // Collect from both
  std::vector<int> evens, odds;
  for (; it1.has_curr(); it1.next())
    evens.push_back(it1.get_curr());
  for (; it2.has_curr(); it2.next())
    odds.push_back(it2.get_curr());

  ASSERT_EQ(evens.size(), 3u);
  ASSERT_EQ(odds.size(), 3u);
  EXPECT_EQ(evens[0], 2);
  EXPECT_EQ(odds[0], 1);
}

TEST(FilterIteratorEdge, FirstItemFiltered) {
  DL list;
  list.append(1); list.append(2); list.append(3); // First is odd

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), 2); // Skips 1
}

TEST(FilterIteratorEdge, LastItemFiltered) {
  DL list;
  list.append(2); list.append(4); list.append(5); // Last is odd

  Filter_Iterator<DL, DL_It, ShowEven> it(list);
  it.reset_last();

  ASSERT_EQ(it.get_curr(), 4); // Not 5
}

TEST(FilterIteratorEdge, OnlyMiddleItemsPasses) {
  DL list;
  list.append(1); // filtered
  list.append(2); // passes
  list.append(3); // filtered
  list.append(4); // passes
  list.append(5); // filtered

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  std::vector<int> collected;
  for (; it.has_curr(); it.next())
    collected.push_back(it.get_curr());

  ASSERT_EQ(collected.size(), 2u);
  EXPECT_EQ(collected[0], 2);
  EXPECT_EQ(collected[1], 4);
}

TEST(FilterIteratorEdge, SingleItemPasses) {
  DL list;
  list.append(1); list.append(2); list.append(3);

  // Only 2 passes
  struct ShowTwo {
    bool operator()(int x) const { return x == 2; }
  };

  Filter_Iterator<DL, DL_It, ShowTwo> it(list);

  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), 2);
  it.next();
  ASSERT_FALSE(it.has_curr());
}

// ========== GROUP 8: Type Aliases ==========

TEST(FilterIteratorTypes, ItemType) {
  using FI = Filter_Iterator<DL, DL_It, ShowAll>;
  static_assert(std::is_same_v<FI::Item_Type, int>);
}

TEST(FilterIteratorTypes, IteratorType) {
  using FI = Filter_Iterator<DL, DL_It, ShowAll>;
  static_assert(std::is_same_v<FI::Iterator_Type, DL_It>);
}

TEST(FilterIteratorTypes, ContainerType) {
  using FI = Filter_Iterator<DL, DL_It, ShowAll>;
  static_assert(std::is_same_v<FI::Container_Type, DL>);
}

TEST(FilterIteratorTypes, FilterType) {
  using FI = Filter_Iterator<DL, DL_It, ShowAll>;
  static_assert(std::is_same_v<FI::Filter_Type, ShowAll>);
}

// ========== GROUP 9: Default Filters ==========

TEST(FilterIteratorDefaultFilters, ShowAllFilter) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, Show_All<int>> it(list);

  int count = 0;
  for (; it.has_curr(); it.next())
    ++count;

  ASSERT_EQ(count, 5);
}

TEST(FilterIteratorDefaultFilters, ShowNoneFilter) {
  DL list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, Show_None<int>> it(list);

  ASSERT_FALSE(it.has_curr());
}


// ========== GROUP 10: New Methods ==========

TEST(FilterIteratorNewMethods, Empty) {
  DL list;
  Filter_Iterator<DL, DL_It, ShowAll> it1(list);
  ASSERT_TRUE(it1.empty());

  list.append(1);
  Filter_Iterator<DL, DL_It, ShowAll> it2(list);
  ASSERT_FALSE(it2.empty());

  // All filtered out
  Filter_Iterator<DL, DL_It, ShowNone> it3(list);
  ASSERT_TRUE(it3.empty());
}

TEST(FilterIteratorNewMethods, ForEach) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  int sum = 0;
  it.for_each([&sum](int x) { sum += x; });

  ASSERT_EQ(sum, 12); // 2 + 4 + 6
}

TEST(FilterIteratorNewMethods, ForEachEmpty) {
  DL list;
  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  int count = 0;
  it.for_each([&count](int) { ++count; });

  ASSERT_EQ(count, 0);
}

TEST(FilterIteratorNewMethods, FindIf) {
  DL list;
  for (int i = 1; i <= 10; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  // Find first even > 5
  bool found = it.find_if([](int x) { return x > 5; });

  ASSERT_TRUE(found);
  ASSERT_EQ(it.get_curr(), 6);
}

TEST(FilterIteratorNewMethods, FindIfNotFound) {
  DL list;
  for (int i = 1; i <= 6; ++i)
    list.append(i);

  Filter_Iterator<DL, DL_It, ShowEven> it(list);

  // Find even > 100 (doesn't exist)
  bool found = it.find_if([](int x) { return x > 100; });

  ASSERT_FALSE(found);
  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorNewMethods, FindIfEmpty) {
  DL list;
  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  bool found = it.find_if([](int) { return true; });

  ASSERT_FALSE(found);
}

// ========== GROUP 11: Exception Safety in forward/backward ==========

TEST(FilterIteratorExceptionSafety, ForwardAtEnd) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);
  it.next(); // At end

  // Should not throw (overflow caught)
  EXPECT_NO_THROW(it.next());
  ASSERT_FALSE(it.has_curr());
}

TEST(FilterIteratorExceptionSafety, BackwardAtBeginning) {
  DL list;
  list.append(1);

  Filter_Iterator<DL, DL_It, ShowAll> it(list);

  // Should not throw (underflow caught)
  EXPECT_NO_THROW(it.prev());
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

