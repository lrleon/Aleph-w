
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
 * @file ahSort_test.cc
 * @brief Exhaustive tests for ahSort.H sorting functions
 */

#include <gtest/gtest.h>

#include <vector>
#include <deque>
#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include <type_traits>

#include <ahSort.H>
#include <tpl_dynArray.H>
#include <tpl_dynList.H>
#include <tpl_dynDlist.H>

using namespace Aleph;
using namespace std;

namespace {

template <class Compare>
vector<size_t> stable_index_reference(const vector<int> &values,
                                      const Compare &cmp)
{
  vector<size_t> ref(values.size());
  for (size_t i = 0; i < ref.size(); ++i)
    ref[i] = i;

  std::stable_sort(ref.begin(), ref.end(), [&values, &cmp](size_t i, size_t j)
  {
    return cmp(values[i], values[j]);
  });

  return ref;
}

template <class Index>
void expect_index_matches_reference(const Index &idx,
                                    const vector<size_t> &ref)
{
  ASSERT_EQ(idx.size(), ref.size());
  for (size_t i = 0; i < ref.size(); ++i)
    EXPECT_EQ(idx(i), ref[i]) << "position " << i;
}

template <class Compare>
void expect_stable_argsort_matches_reference_exhaustively(const Compare &cmp)
{
  constexpr size_t max_n = 7;
  constexpr size_t alphabet = 3;

  for (size_t n = 0; n <= max_n; ++n)
    {
      size_t cases = 1;
      for (size_t i = 0; i < n; ++i)
        cases *= alphabet;

      for (size_t code = 0; code < cases; ++code)
        {
          vector<int> values(n);
          size_t x = code;
          for (size_t i = 0; i < n; ++i)
            {
              values[i] = static_cast<int>(x % alphabet);
              x /= alphabet;
            }

          const auto idx = stable_argsort(values, cmp);
          const auto ref = stable_index_reference(values, cmp);
          expect_index_matches_reference(idx, ref);
        }
    }
}

} // namespace

// ============================================================================
// Test Fixtures
// ============================================================================

class DynListSortTest : public ::testing::Test
{
protected:
  DynList<int> list;

  void SetUp() override
  {
    list = DynList<int>{5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
  }

  DynList<int> build_list(std::initializer_list<int> items)
  {
    DynList<int> l;
    for (int x : items)
      l.append(x);
    return l;
  }
};

class DynDlistSortTest : public ::testing::Test
{
protected:
  DynDlist<int> list;

  void SetUp() override
  {
    for (int x : {5, 2, 8, 1, 9, 3, 7, 4, 6, 0})
      list.append(x);
  }

  DynDlist<int> build_list(std::initializer_list<int> items)
  {
    DynDlist<int> l;
    for (int x : items)
      l.append(x);
    return l;
  }
};

class DynArraySortTest : public ::testing::Test
{
protected:
  DynArray<int> arr;

  void SetUp() override
  {
    arr.reserve(10);
    int values[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    for (size_t i = 0; i < 10; ++i)
      arr(i) = values[i];
  }

  DynArray<int> build_array(std::initializer_list<int> items)
  {
    DynArray<int> a;
    a.reserve(items.size());
    size_t i = 0;
    for (int x : items)
      a(i++) = x;
    return a;
  }
};

class ArraySortTest : public ::testing::Test
{
protected:
  Array<int> arr;

  void SetUp() override
  {
    int values[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    for (int v : values)
      arr.append(v);
  }

  Array<int> build_array(std::initializer_list<int> items)
  {
    Array<int> a;
    for (int x : items)
      a.append(x);
    return a;
  }
};

// ============================================================================
// DynList sort() tests
// ============================================================================

TEST_F(DynListSortTest, SortReturnsSortedCopy)
{
  auto sorted = sort(list);

  // Original unchanged
  EXPECT_EQ(list.get_first(), 5);

  // Result is sorted
  int prev = -1;
  sorted.for_each([&prev](int x) {
    EXPECT_GT(x, prev);
    prev = x;
  });
}

TEST_F(DynListSortTest, SortWithCustomComparator)
{
  auto sorted = sort(list, std::greater<int>());

  // Descending order
  int prev = 100;
  sorted.for_each([&prev](int x) {
    EXPECT_LT(x, prev);
    prev = x;
  });
}

TEST_F(DynListSortTest, SortMoveSemantics)
{
  DynList<int> temp = build_list({3, 1, 2});
  auto sorted = sort(std::move(temp));

  // Original moved from
  EXPECT_TRUE(temp.is_empty());

  // Result is sorted
  EXPECT_EQ(sorted.get_first(), 1);
  EXPECT_EQ(sorted.get_last(), 3);
}

TEST_F(DynListSortTest, InPlaceSortModifiesOriginal)
{
  in_place_sort(list);

  // Original is now sorted
  int prev = -1;
  list.for_each([&prev](int x) {
    EXPECT_GT(x, prev);
    prev = x;
  });
}

TEST_F(DynListSortTest, InPlaceSortReturnsReference)
{
  auto & ref = in_place_sort(list);
  EXPECT_EQ(&ref, &list);
}

TEST_F(DynListSortTest, SortEmptyList)
{
  DynList<int> empty;
  auto sorted = sort(empty);
  EXPECT_TRUE(sorted.is_empty());
}

TEST_F(DynListSortTest, SortSingleElement)
{
  DynList<int> single;
  single.append(42);
  auto sorted = sort(single);
  EXPECT_EQ(sorted.size(), 1u);
  EXPECT_EQ(sorted.get_first(), 42);
}

TEST_F(DynListSortTest, SortAlreadySorted)
{
  DynList<int> already = build_list({1, 2, 3, 4, 5});
  auto sorted = sort(already);
  EXPECT_TRUE(is_sorted(sorted));
}

TEST_F(DynListSortTest, SortReversed)
{
  DynList<int> reversed = build_list({5, 4, 3, 2, 1});
  auto sorted = sort(reversed);
  EXPECT_TRUE(is_sorted(sorted));
  EXPECT_EQ(sorted.get_first(), 1);
}

TEST_F(DynListSortTest, SortWithDuplicates)
{
  DynList<int> dups = build_list({3, 1, 3, 1, 2, 2});
  auto sorted = sort(dups);
  EXPECT_TRUE(is_sorted(sorted));
}

// ============================================================================
// DynDlist sort() tests
// ============================================================================

TEST_F(DynDlistSortTest, SortReturnsSortedCopy)
{
  auto sorted = sort(list);
  EXPECT_TRUE(is_sorted(sorted));
  EXPECT_EQ(list.get_first(), 5);  // Original unchanged
}

TEST_F(DynDlistSortTest, SortWithCustomComparator)
{
  auto sorted = sort(list, std::greater<int>());

  int prev = 100;
  sorted.for_each([&prev](int x) {
    EXPECT_LT(x, prev);
    prev = x;
  });
}

TEST_F(DynDlistSortTest, SortMoveSemantics)
{
  DynDlist<int> temp = build_list({3, 1, 2});
  auto sorted = sort(std::move(temp));
  EXPECT_TRUE(temp.is_empty());
  EXPECT_TRUE(is_sorted(sorted));
}

TEST_F(DynDlistSortTest, InPlaceSortModifiesOriginal)
{
  in_place_sort(list);
  EXPECT_TRUE(is_sorted(list));
}

// ============================================================================
// DynArray sort() tests
// ============================================================================

TEST_F(DynArraySortTest, SortReturnsSortedCopy)
{
  auto sorted = sort(arr);

  // Original unchanged
  EXPECT_EQ(arr(0), 5);

  // Result is sorted
  for (size_t i = 1; i < sorted.size(); ++i)
    EXPECT_LE(sorted(i-1), sorted(i));
}

TEST_F(DynArraySortTest, SortWithCustomComparator)
{
  auto sorted = sort(arr, std::greater<int>());

  for (size_t i = 1; i < sorted.size(); ++i)
    EXPECT_GE(sorted(i-1), sorted(i));
}

TEST_F(DynArraySortTest, SortMoveSemantics)
{
  DynArray<int> temp = build_array({3, 1, 2});
  auto sorted = sort(std::move(temp));

  // Original should be empty after move
  EXPECT_EQ(temp.size(), 0u);

  // Result is sorted
  EXPECT_EQ(sorted(0), 1);
  EXPECT_EQ(sorted(1), 2);
  EXPECT_EQ(sorted(2), 3);
}

TEST_F(DynArraySortTest, InPlaceSortModifiesOriginal)
{
  in_place_sort(arr);

  for (size_t i = 1; i < arr.size(); ++i)
    EXPECT_LE(arr(i-1), arr(i));
}

TEST_F(DynArraySortTest, InPlaceSortReturnsReference)
{
  auto & ref = in_place_sort(arr);
  EXPECT_EQ(&ref, &arr);
}

TEST_F(DynArraySortTest, SortEmptyArray)
{
  DynArray<int> empty;
  auto sorted = sort(empty);
  EXPECT_EQ(sorted.size(), 0u);
}

TEST_F(DynArraySortTest, SortSingleElement)
{
  DynArray<int> single;
  single.reserve(1);
  single.touch(0) = 42;
  auto sorted = sort(single);
  EXPECT_EQ(sorted.size(), 1u);
  EXPECT_EQ(sorted(0), 42);
}

// ============================================================================
// Array sort() tests
// ============================================================================

TEST_F(ArraySortTest, SortReturnsSortedCopy)
{
  auto sorted = sort(arr);
  EXPECT_EQ(arr(0), 5);  // Original unchanged

  for (size_t i = 1; i < sorted.size(); ++i)
    EXPECT_LE(sorted(i-1), sorted(i));
}

TEST_F(ArraySortTest, SortMoveSemantics)
{
  Array<int> temp = build_array({3, 1, 2});
  auto sorted = sort(std::move(temp));
  EXPECT_EQ(sorted(0), 1);
  EXPECT_EQ(sorted(2), 3);
}

TEST_F(ArraySortTest, InPlaceSortModifiesOriginal)
{
  in_place_sort(arr);
  for (size_t i = 1; i < arr.size(); ++i)
    EXPECT_LE(arr(i-1), arr(i));
}

// ============================================================================
// stdsort() tests
// ============================================================================

TEST(StdSortTest, SortsVector)
{
  std::vector<int> v = {5, 2, 8, 1, 9};
  auto sorted = stdsort(v);

  EXPECT_EQ(v[0], 5);  // Original unchanged
  EXPECT_EQ(sorted, (std::vector<int>{1, 2, 5, 8, 9}));
}

TEST(StdSortTest, SortsVectorWithCustomComparator)
{
  std::vector<int> v = {5, 2, 8, 1, 9};
  auto sorted = stdsort(v, std::greater<int>());
  EXPECT_EQ(sorted, (std::vector<int>{9, 8, 5, 2, 1}));
}

TEST(StdSortTest, SortsDeque)
{
  std::deque<int> d = {5, 2, 8, 1, 9};
  auto sorted = stdsort(d);
  EXPECT_EQ(sorted, (std::deque<int>{1, 2, 5, 8, 9}));
}

TEST(StdSortTest, SortsEmptyContainer)
{
  std::vector<int> empty;
  auto sorted = stdsort(empty);
  EXPECT_TRUE(sorted.empty());
}

// ============================================================================
// argsort() tests
// ============================================================================

TEST(ArgsortTest, ArrayReturnsSortedOrderIndices)
{
  Array<int> arr;
  arr.append(30);
  arr.append(10);
  arr.append(20);

  auto idx = argsort(arr);

  static_assert(std::is_same_v<decltype(idx), Array<size_t>>);
  ASSERT_EQ(idx.size(), arr.size());
  EXPECT_EQ(idx(0), 1u);
  EXPECT_EQ(idx(1), 2u);
  EXPECT_EQ(idx(2), 0u);
  EXPECT_EQ(arr(idx(0)), 10);
  EXPECT_EQ(arr(idx(1)), 20);
  EXPECT_EQ(arr(idx(2)), 30);
}

TEST(ArgsortTest, DynArrayReturnsSortedOrderIndices)
{
  DynArray<int> arr;
  arr.reserve(3);
  arr(0) = 30;
  arr(1) = 10;
  arr(2) = 20;

  auto idx = argsort(arr);

  ASSERT_EQ(idx.size(), arr.size());
  EXPECT_EQ(arr(idx(0)), 10);
  EXPECT_EQ(arr(idx(1)), 20);
  EXPECT_EQ(arr(idx(2)), 30);
}

TEST(ArgsortTest, StdVectorReturnsSortedOrderIndices)
{
  vector<int> values = {30, 10, 20};

  auto idx = argsort(values);

  ASSERT_EQ(idx.size(), values.size());
  EXPECT_EQ(idx(0), 1u);
  EXPECT_EQ(idx(1), 2u);
  EXPECT_EQ(idx(2), 0u);
  EXPECT_EQ(values[idx(0)], 10);
  EXPECT_EQ(values[idx(1)], 20);
  EXPECT_EQ(values[idx(2)], 30);
}

TEST(ArgsortTest, EmptyStdVectorReturnsEmptyIndex)
{
  vector<int> values;

  auto idx = argsort(values);

  EXPECT_TRUE(idx.is_empty());
}

TEST(ArgsortTest, CustomComparatorDescending)
{
  vector<int> values = {10, 30, 20};

  auto idx = argsort(values, std::greater<int>());

  ASSERT_EQ(idx.size(), values.size());
  EXPECT_EQ(values[idx(0)], 30);
  EXPECT_EQ(values[idx(1)], 20);
  EXPECT_EQ(values[idx(2)], 10);
}

TEST(ArgsortTest, MatchesBuildIndexForAlephArrays)
{
  DynArray<int> arr;
  arr.reserve(5);
  arr(0) = 5;
  arr(1) = 1;
  arr(2) = 4;
  arr(3) = 2;
  arr(4) = 3;

  auto idx = argsort(arr);
  auto ref = build_index(arr);

  ASSERT_EQ(idx.size(), ref.size());
  for (size_t i = 0; i < idx.size(); ++i)
    EXPECT_EQ(idx(i), ref(i));
}

TEST(StableArgsortTest, ArrayPreservesEquivalentOrder)
{
  Array<int> arr;
  arr.append(2);
  arr.append(1);
  arr.append(2);
  arr.append(1);
  arr.append(2);

  auto idx = stable_argsort(arr);

  static_assert(std::is_same_v<decltype(idx), Array<size_t>>);
  ASSERT_EQ(idx.size(), arr.size());
  EXPECT_EQ(idx(0), 1u);
  EXPECT_EQ(idx(1), 3u);
  EXPECT_EQ(idx(2), 0u);
  EXPECT_EQ(idx(3), 2u);
  EXPECT_EQ(idx(4), 4u);
}

TEST(StableArgsortTest, DynArrayPreservesEquivalentOrder)
{
  DynArray<int> arr;
  arr.reserve(5);
  arr(0) = 2;
  arr(1) = 1;
  arr(2) = 2;
  arr(3) = 1;
  arr(4) = 2;

  auto idx = stable_argsort(arr);

  ASSERT_EQ(idx.size(), arr.size());
  EXPECT_EQ(idx(0), 1u);
  EXPECT_EQ(idx(1), 3u);
  EXPECT_EQ(idx(2), 0u);
  EXPECT_EQ(idx(3), 2u);
  EXPECT_EQ(idx(4), 4u);
}

TEST(StableArgsortTest, StdVectorPreservesEquivalentOrder)
{
  vector<int> values = {2, 1, 2, 1, 2};

  auto idx = stable_argsort(values);

  ASSERT_EQ(idx.size(), values.size());
  EXPECT_EQ(idx(0), 1u);
  EXPECT_EQ(idx(1), 3u);
  EXPECT_EQ(idx(2), 0u);
  EXPECT_EQ(idx(3), 2u);
  EXPECT_EQ(idx(4), 4u);
}

TEST(StableArgsortTest, AllEquivalentValuesPreserveOriginalOrder)
{
  vector<int> values(64, 7);

  auto idx = stable_argsort(values);

  ASSERT_EQ(idx.size(), values.size());
  for (size_t i = 0; i < idx.size(); ++i)
    EXPECT_EQ(idx(i), i);
}

TEST(StableArgsortTest, LargeDuplicateGroupsPreserveOriginalOrder)
{
  vector<int> values;
  for (size_t i = 0; i < 75; ++i)
    values.push_back(static_cast<int>((i * 17) % 5));

  auto idx = stable_argsort(values);

  ASSERT_EQ(idx.size(), values.size());
  for (size_t i = 1; i < idx.size(); ++i)
    {
      ASSERT_LE(values[idx(i - 1)], values[idx(i)]);
      if (values[idx(i - 1)] == values[idx(i)])
        EXPECT_LT(idx(i - 1), idx(i));
    }
}

TEST(StableArgsortTest, ExhaustiveSmallInputsMatchStableSortReference)
{
  expect_stable_argsort_matches_reference_exhaustively(std::less<int>());
}

TEST(StableArgsortTest, ExhaustiveSmallInputsMatchDescendingStableSortReference)
{
  expect_stable_argsort_matches_reference_exhaustively(std::greater<int>());
}

TEST(StableArgsortTest, CustomComparatorDescendingPreservesEquivalentOrder)
{
  vector<int> values = {1, 3, 2, 3, 2};

  auto idx = stable_argsort(values, std::greater<int>());

  ASSERT_EQ(idx.size(), values.size());
  EXPECT_EQ(idx(0), 1u);
  EXPECT_EQ(idx(1), 3u);
  EXPECT_EQ(idx(2), 2u);
  EXPECT_EQ(idx(3), 4u);
  EXPECT_EQ(idx(4), 0u);
}

TEST(StableArgsortTest, MatchesStableBuildIndexForAlephArrays)
{
  DynArray<int> arr;
  arr.reserve(5);
  arr(0) = 2;
  arr(1) = 1;
  arr(2) = 2;
  arr(3) = 1;
  arr(4) = 2;

  auto idx = stable_argsort(arr);
  auto ref = stable_build_index(arr);

  ASSERT_EQ(idx.size(), ref.size());
  for (size_t i = 0; i < idx.size(); ++i)
    EXPECT_EQ(idx(i), ref(i));
}

// ============================================================================
// ranks() tests
// ============================================================================

TEST(RanksTest, DynArrayRanks)
{
  DynArray<int> arr;
  arr.reserve(3);
  arr(0) = 30;  // rank 2
  arr(1) = 10;  // rank 0
  arr(2) = 20;  // rank 1

  auto r = ranks(arr);

  EXPECT_EQ(r(0), 2u);  // 30 is largest -> rank 2
  EXPECT_EQ(r(1), 0u);  // 10 is smallest -> rank 0
  EXPECT_EQ(r(2), 1u);  // 20 is middle -> rank 1
}

TEST(RanksTest, ArrayRanks)
{
  Array<int> arr;
  arr.append(30);
  arr.append(10);
  arr.append(20);

  auto r = ranks(arr);

  EXPECT_EQ(r(0), 2u);
  EXPECT_EQ(r(1), 0u);
  EXPECT_EQ(r(2), 1u);
}

TEST(RanksTest, DynListRanks)
{
  DynList<int> list;
  list.append(30);
  list.append(10);
  list.append(20);

  auto r = ranks(list);

  EXPECT_EQ(r(0), 2u);
  EXPECT_EQ(r(1), 0u);
  EXPECT_EQ(r(2), 1u);
}

TEST(RanksTest, DynDlistRanks)
{
  DynDlist<int> list;
  list.append(30);
  list.append(10);
  list.append(20);

  auto r = ranks(list);

  EXPECT_EQ(r(0), 2u);
  EXPECT_EQ(r(1), 0u);
  EXPECT_EQ(r(2), 1u);
}

TEST(RanksTest, EmptyContainer)
{
  DynArray<int> empty;
  auto r = ranks(empty);
  EXPECT_EQ(r.size(), 0u);
}

TEST(RanksTest, SingleElement)
{
  DynArray<int> single;
  single.reserve(1);
  single.touch(0) = 42;
  auto r = ranks(single);
  EXPECT_EQ(r.size(), 1u);
  EXPECT_EQ(r(0), 0u);
}

TEST(RanksTest, AlreadySorted)
{
  DynArray<int> arr;
  arr.reserve(5);
  for (size_t i = 0; i < 5; ++i)
    arr(i) = static_cast<int>(i);

  auto r = ranks(arr);

  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(r(i), i);
}

TEST(RanksTest, Reversed)
{
  DynArray<int> arr;
  arr.reserve(5);
  for (size_t i = 0; i < 5; ++i)
    arr(i) = static_cast<int>(4 - i);

  auto r = ranks(arr);

  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(r(i), 4 - i);
}

TEST(RanksTest, DuplicatesOrderingProperty)
{
  DynArray<int> arr;
  arr.reserve(6);
  arr(0) = 5;
  arr(1) = 1;
  arr(2) = 5;
  arr(3) = 2;
  arr(4) = 2;
  arr(5) = 1;

  auto r = ranks(arr);
  ASSERT_EQ(r.size(), arr.size());

  // ranks() must be a permutation of 0..n-1
  std::vector<size_t> seen(r.size(), 0);
  for (size_t i = 0; i < r.size(); ++i)
    {
      ASSERT_LT(r(i), r.size());
      ++seen[r(i)];
    }
  for (size_t k = 0; k < seen.size(); ++k)
    EXPECT_EQ(seen[k], 1u);

  // Ordering property: if a[i] < a[j] then r[i] < r[j]
  for (size_t i = 0; i < arr.size(); ++i)
    for (size_t j = 0; j < arr.size(); ++j)
      if (arr(i) < arr(j))
        EXPECT_LT(r(i), r(j));
}

// ============================================================================
// pair_ranks() tests
// ============================================================================

TEST(PairRanksTest, DynArrayPairRanks)
{
  DynArray<int> arr;
  arr.reserve(3);
  arr(0) = 30;
  arr(1) = 10;
  arr(2) = 20;

  auto pr = pair_ranks(arr);

  EXPECT_EQ(pr(0).first, 30);
  EXPECT_EQ(pr(0).second, 2u);
  EXPECT_EQ(pr(1).first, 10);
  EXPECT_EQ(pr(1).second, 0u);
  EXPECT_EQ(pr(2).first, 20);
  EXPECT_EQ(pr(2).second, 1u);
}

TEST(PairRanksTest, ArrayPairRanks)
{
  Array<int> arr;
  arr.append(30);
  arr.append(10);
  arr.append(20);

  auto pr = pair_ranks(arr);

  EXPECT_EQ(pr(0).first, 30);
  EXPECT_EQ(pr(0).second, 2u);
}

TEST(PairRanksTest, DynListPairRanks)
{
  DynList<int> list;
  list.append(30);
  list.append(10);
  list.append(20);

  auto pr = pair_ranks(list);

  EXPECT_EQ(pr(0).first, 30);
  EXPECT_EQ(pr(0).second, 2u);
  EXPECT_EQ(pr(1).first, 10);
  EXPECT_EQ(pr(1).second, 0u);
}

TEST(PairRanksTest, DynDlistPairRanks)
{
  DynDlist<int> list;
  list.append(30);
  list.append(10);
  list.append(20);

  auto pr = pair_ranks(list);

  EXPECT_EQ(pr(0).first, 30);
  EXPECT_EQ(pr(0).second, 2u);
}

// ============================================================================
// in_place_multisort_arrays() tests
// ============================================================================

TEST(MultiSortTest, BasicSort)
{
  std::vector<int> keys = {3, 1, 2};
  std::vector<std::string> names = {"Charlie", "Alice", "Bob"};

  in_place_multisort_arrays(std::less<int>(), keys, names);

  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(names, (std::vector<std::string>{"Alice", "Bob", "Charlie"}));
}

TEST(MultiSortTest, ThreeArrays)
{
  std::vector<int> ids = {3, 1, 2};
  std::vector<std::string> names = {"Charlie", "Alice", "Bob"};
  std::vector<int> ages = {30, 25, 28};

  in_place_multisort_arrays(std::less<int>(), ids, names, ages);

  EXPECT_EQ(ids, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(names, (std::vector<std::string>{"Alice", "Bob", "Charlie"}));
  EXPECT_EQ(ages, (std::vector<int>{25, 28, 30}));
}

TEST(MultiSortTest, DescendingOrder)
{
  std::vector<int> keys = {1, 2, 3};
  std::vector<char> values = {'a', 'b', 'c'};

  in_place_multisort_arrays(std::greater<int>(), keys, values);

  EXPECT_EQ(keys, (std::vector<int>{3, 2, 1}));
  EXPECT_EQ(values, (std::vector<char>{'c', 'b', 'a'}));
}

TEST(MultiSortTest, EmptyArrays)
{
  std::vector<int> keys;
  std::vector<int> values;

  // Should not throw
  in_place_multisort_arrays(std::less<int>(), keys, values);

  EXPECT_TRUE(keys.empty());
  EXPECT_TRUE(values.empty());
}

TEST(MultiSortTest, SingleElement)
{
  std::vector<int> keys = {42};
  std::vector<std::string> values = {"answer"};

  in_place_multisort_arrays(std::less<int>(), keys, values);

  EXPECT_EQ(keys, (std::vector<int>{42}));
  EXPECT_EQ(values, (std::vector<std::string>{"answer"}));
}

TEST(MultiSortTest, StableSort)
{
  std::vector<int> keys = {2, 1, 2, 1, 2};
  std::vector<char> aux = {'a', 'b', 'c', 'd', 'e'};

  in_place_multisort_arrays(std::less<int>(), keys, aux);

  EXPECT_EQ(keys, (std::vector<int>{1, 1, 2, 2, 2}));
  // Stable: elements with equal keys preserve relative order
  EXPECT_EQ(aux, (std::vector<char>{'b', 'd', 'a', 'c', 'e'}));
}

TEST(MultiSortTest, AlreadySorted)
{
  std::vector<int> keys = {1, 2, 3, 4, 5};
  std::vector<int> values = {10, 20, 30, 40, 50};

  in_place_multisort_arrays(std::less<int>(), keys, values);

  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5}));
  EXPECT_EQ(values, (std::vector<int>{10, 20, 30, 40, 50}));
}

TEST(MultiSortTest, StablePropertyRandom)
{
  std::mt19937 rng(123456u);
  std::uniform_int_distribution<int> key_dist(0, 5);

  for (size_t trial = 0; trial < 50; ++trial)
    {
      const size_t n = 100;
      std::vector<int> keys(n);
      std::vector<size_t> pos(n);
      for (size_t i = 0; i < n; ++i)
        {
          keys[i] = key_dist(rng);
          pos[i] = i;
        }

      in_place_multisort_arrays(std::less<int>(), true, keys, pos);

      for (size_t i = 1; i < n; ++i)
        {
          ASSERT_LE(keys[i - 1], keys[i]);
          if (keys[i - 1] == keys[i])
            ASSERT_LT(pos[i - 1], pos[i]);
        }
    }
}

TEST(MultiSortTest, UnstablePropertyPermutation)
{
  std::mt19937 rng(78910u);
  std::uniform_int_distribution<int> key_dist(0, 5);

  const size_t n = 200;
  std::vector<int> keys(n);
  std::vector<size_t> pos(n);
  for (size_t i = 0; i < n; ++i)
    {
      keys[i] = key_dist(rng);
      pos[i] = i;
    }

  in_place_multisort_arrays(std::less<int>(), false, keys, pos);

  for (size_t i = 1; i < n; ++i)
    ASSERT_LE(keys[i - 1], keys[i]);

  std::vector<size_t> seen(n, 0);
  for (auto p : pos)
    {
      ASSERT_LT(p, n);
      ++seen[p];
    }
  for (size_t k = 0; k < n; ++k)
    ASSERT_EQ(seen[k], 1u);
}

TEST(MultiSortTest, ReverseSorted)
{
  std::vector<int> keys = {5, 4, 3, 2, 1};
  std::vector<int> values = {50, 40, 30, 20, 10};

  in_place_multisort_arrays(std::less<int>(), keys, values);

  EXPECT_EQ(keys, (std::vector<int>{1, 2, 3, 4, 5}));
  EXPECT_EQ(values, (std::vector<int>{10, 20, 30, 40, 50}));
}

TEST(MultiSortTest, SizeMismatchThrows)
{
  std::vector<int> keys = {1, 2};
  std::vector<int> values = {10};

  EXPECT_THROW(in_place_multisort_arrays(std::less<int>(), keys, values),
               std::invalid_argument);
}

TEST(MultiSortTest, AlephArrays)
{
  Array<int> keys;
  keys.append(3); keys.append(1); keys.append(2);

  Array<std::string> values;
  values.append("c"); values.append("a"); values.append("b");

  in_place_multisort_arrays(std::less<int>(), keys, values);

  EXPECT_EQ(keys(0), 1);
  EXPECT_EQ(keys(1), 2);
  EXPECT_EQ(keys(2), 3);
  EXPECT_EQ(values(0), "a");
  EXPECT_EQ(values(1), "b");
  EXPECT_EQ(values(2), "c");
}

TEST(MultiSortTest, StableFlagTrue)
{
  std::vector<int> keys = {2, 1, 2, 1, 2};
  std::vector<char> aux = {'a', 'b', 'c', 'd', 'e'};

  in_place_multisort_arrays(std::less<int>(), true, keys, aux);

  EXPECT_EQ(keys, (std::vector<int>{1, 1, 2, 2, 2}));
  EXPECT_EQ(aux, (std::vector<char>{'b', 'd', 'a', 'c', 'e'}));
}

TEST(MultiSortTest, StableFlagFalse)
{
  std::vector<int> keys = {2, 1, 2, 1, 2};
  std::vector<char> aux = {'a', 'b', 'c', 'd', 'e'};

  in_place_multisort_arrays(std::less<int>(), false, keys, aux);

  EXPECT_EQ(keys, (std::vector<int>{1, 1, 2, 2, 2}));
  // Result order may differ from stable sort; only keys are guaranteed
  EXPECT_EQ(keys.size(), aux.size());
}

TEST(MultiSortTest, StableFlagFalseWithCustomComparator)
{
  std::vector<std::string> keys = {"banana", "apple", "banana", "apple"};
  std::vector<int> values = {2, 1, 3, 4};

  in_place_multisort_arrays(std::greater<std::string>(), false, keys, values);

  EXPECT_EQ(keys, (std::vector<std::string>{"banana", "banana", "apple", "apple"}));
  EXPECT_EQ(values.size(), 4);
}

// ============================================================================
// Type traits and compile-time checks
// ============================================================================

TEST(SortTraitsTest, SortIsNodiscard)
{
  // The [[nodiscard]] attribute is tested implicitly:
  // If we call sort() without using the result, the compiler would warn.
  // This test just verifies the functions compile correctly.
  DynList<int> list;
  list.append(1);
  [[maybe_unused]] auto s1 = sort(list);
  [[maybe_unused]] auto s2 = sort(std::move(list));
}

TEST(SortTraitsTest, RanksIsNodiscard)
{
  DynArray<int> arr;
  arr.reserve(1);
  arr.touch(0) = 1;
  [[maybe_unused]] auto r = ranks(arr);
}

TEST(SortTraitsTest, PairRanksIsNodiscard)
{
  DynArray<int> arr;
  arr.reserve(1);
  arr.touch(0) = 1;
  [[maybe_unused]] auto pr = pair_ranks(arr);
}

// ============================================================================
// Edge cases and stress tests
// ============================================================================

TEST(SortStressTest, LargeDynList)
{
  DynList<int> list;
  for (int i = 999; i >= 0; --i)
    list.append(i);

  auto sorted = sort(list);
  EXPECT_TRUE(is_sorted(sorted));
  EXPECT_EQ(sorted.get_first(), 0);
  EXPECT_EQ(sorted.get_last(), 999);
}

TEST(SortStressTest, LargeDynArray)
{
  DynArray<int> arr;
  arr.reserve(1000);
  for (size_t i = 0; i < 1000; ++i)
    arr(i) = static_cast<int>(999 - i);

  in_place_sort(arr);

  for (size_t i = 1; i < arr.size(); ++i)
    EXPECT_LE(arr(i-1), arr(i));
}

TEST(SortStressTest, AllSameElements)
{
  DynList<int> list;
  for (int i = 0; i < 100; ++i)
    list.append(42);

  auto sorted = sort(list);
  EXPECT_TRUE(is_sorted(sorted));

  sorted.for_each([](int x) {
    EXPECT_EQ(x, 42);
  });
}

TEST(SortEdgeCaseTest, StringSort)
{
  DynList<std::string> list;
  list.append("banana");
  list.append("apple");
  list.append("cherry");

  auto sorted = sort(list);

  EXPECT_EQ(sorted.get_first(), "apple");
  EXPECT_EQ(sorted.get_last(), "cherry");
}

TEST(SortEdgeCaseTest, LambdaComparator)
{
  DynArray<int> arr;
  arr.reserve(5);
  arr(0) = 1; arr(1) = 2; arr(2) = 3; arr(3) = 4; arr(4) = 5;

  // Sort by absolute difference from 3
  auto sorted = sort(arr, [](int a, int b) {
    return std::abs(a - 3) < std::abs(b - 3);
  });

  EXPECT_EQ(sorted(0), 3);  // difference 0
}
