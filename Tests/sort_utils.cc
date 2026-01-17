/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
  / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
 / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
/_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
              |_|

This file is part of Aleph-w library

Copyright (c) 2002-2018 Leandro Rabindranath Leon

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file sort_utils.cc
 * @brief Tests for Sort Utils
 */

#include <gtest/gtest.h>

#include <tpl_sort_utils.H>
#include <tpl_dynArray.H>
#include <tpl_array.H>
#include <tpl_dynList.H>
#include <tpl_dynDlist.H>
#include <tpl_dnode.H>

using namespace Aleph;
using namespace std;

namespace {

static DynArray<int> make_dynarray(std::initializer_list<int> xs)
{
  DynArray<int> a;
  a.reserve(xs.size());
  size_t i = 0;
  for (const int x : xs)
    a(i++) = x;
  return a;
}

static DynList<int> make_dynlist(std::initializer_list<int> xs)
{
  DynList<int> l;
  for (int x : xs)
    l.append(x);
  return l;
}

static DynDlist<int> make_dyndlist(std::initializer_list<int> xs)
{
  DynDlist<int> l;
  for (int x : xs)
    l.append(x);
  return l;
}

static Dnode<int> make_dnode_list(std::initializer_list<int> xs)
{
  Dnode<int> h;
  for (int x : xs)
    h.append(new Dnode<int>(x));
  return h;
}

static void delete_all_nodes(Dnode<int> & h)
{
  while (not h.is_empty())
    delete h.remove_first_ne();
}

TEST(SortUtilsSortedness, allows_equal)
{
  DynList<int> l = make_dynlist({1, 1, 2, 2, 3});
  EXPECT_TRUE(is_sorted(l));
  EXPECT_TRUE(test_sorted(l).first);
  EXPECT_TRUE(search_inversion(l).first);
}

TEST(SortUtilsSortedness, detects_inversion)
{
  DynList<int> l = make_dynlist({1, 3, 2, 4});
  EXPECT_FALSE(is_sorted(l));
  auto inv = search_inversion(l);
  EXPECT_FALSE(inv.first);
  EXPECT_EQ(inv.second, 2u);
}

TEST(SortUtilsSortedness, inversely_sorted)
{
  DynList<int> l = make_dynlist({5, 4, 4, 2, 1});
  EXPECT_TRUE(is_inversely_sorted(l));
  EXPECT_FALSE(is_sorted(l));
}

TEST(SortUtilsArraySorts, selection_sort_empty_and_single)
{
  int a0[1] = {0};
  selection_sort(a0, 0);
  selection_sort(a0, 1);
  EXPECT_EQ(a0[0], 0);
}

TEST(SortUtilsArraySorts, selection_sort_basic)
{
  int a[] = {3, 1, 2, 1, 0};
  selection_sort(a, sizeof(a) / sizeof(a[0]));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, insertion_sort_subrange)
{
  auto a = make_dynarray({10, 9, 3, 2, 1, 8, 7});
  // sort only [2..4] (3,2,1)
  insertion_sort(a, 2, 4);
  EXPECT_EQ(a(0), 10);
  EXPECT_EQ(a(1), 9);
  EXPECT_EQ(a(2), 1);
  EXPECT_EQ(a(3), 2);
  EXPECT_EQ(a(4), 3);
  EXPECT_EQ(a(5), 8);
  EXPECT_EQ(a(6), 7);
}

TEST(SortUtilsArraySorts, selection_sort_dynarray)
{
  auto a = make_dynarray({3, 1, 2, 1, 0});
  selection_sort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, selection_sort_dynarray_custom_compare_desc)
{
  auto a = make_dynarray({3, 1, 2, 1, 0});
  selection_sort(a, Aleph::greater<int>());
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_GE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, bubble_sort_dynarray)
{
  auto a = make_dynarray({3, 1, 2, 1, 0});
  bubble_sort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, bubble_sort_dynarray_already_sorted_with_duplicates)
{
  auto a = make_dynarray({0, 0, 1, 1, 2, 2});
  bubble_sort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
  EXPECT_EQ(a(0), 0);
  EXPECT_EQ(a(5), 2);
}

static bool is_min_heap(const DynArray<int> & a, size_t n)
{
  if (n <= 1)
    return true;

  for (size_t child = 2; child <= n; ++child)
    {
      size_t parent = child / 2;
      if (a(parent - 1) > a(child - 1))
        return false;
    }
  return true;
}

TEST(SortUtilsHelpers, select_pivot_op_dynarray_median_of_three)
{
  auto a = make_dynarray({5, 99, 0, 99, 99, 3, 99});
  // l=0 => 5, m=3 => 99, r=6 => 99 => median is 99 => either m or r
  long p = select_pivot_op<int>(a, 0, 6);
  EXPECT_TRUE(p == 3 || p == 6);

  auto b = make_dynarray({10, 99, 5, 99, 99, 99, 0});
  // l=0 => 10, m=3 => 99, r=6 => 0 => median is 10 => l
  EXPECT_EQ(select_pivot_op<int>(b, 0, 6), 0);

  auto c = make_dynarray({0, 99, 99, 5, 99, 99, 10});
  // l=0 => 0, m=3 => 5, r=6 => 10 => median is 5 => m
  EXPECT_EQ(select_pivot_op<int>(c, 0, 6), 3);
}

TEST(SortUtilsHelpers, select_pivot_op_array_small_range_returns_r)
{
  Array<int> a;
  for (int x : {5, 4, 3, 2, 1, 0})
    a.append(x);

  // r - l <= 5 => returns r
  EXPECT_EQ(select_pivot_op<int>(a, 0, 5), 5);
}

TEST(SortUtilsHelpers, partition_op_dynarray_invariants)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  std::vector<int> before;
  before.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    before.push_back(a(i));

  long p = partition_op<int>(a, 0, static_cast<long>(a.size() - 1));
  ASSERT_GE(p, 0);
  ASSERT_LT(static_cast<size_t>(p), a.size());

  const int pivot = a(p);
  for (long i = 0; i < p; ++i)
    ASSERT_FALSE(Aleph::less<int>()(pivot, a(i)));
  for (long i = p + 1; i < static_cast<long>(a.size()); ++i)
    ASSERT_FALSE(Aleph::less<int>()(a(i), pivot));

  std::vector<int> after;
  after.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    after.push_back(a(i));
  std::sort(before.begin(), before.end());
  std::sort(after.begin(), after.end());
  EXPECT_EQ(before, after);
}

TEST(SortUtilsHelpers, partition_op_array_invariants)
{
  Array<int> a;
  for (int x : {4, 1, 3, 2, 0, 2})
    a.append(x);

  std::vector<int> before;
  before.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    before.push_back(a(i));

  long p = partition_op<int>(a, 0, static_cast<long>(a.size() - 1));
  ASSERT_GE(p, 0);
  ASSERT_LT(static_cast<size_t>(p), a.size());

  const int pivot = a(p);
  for (long i = 0; i < p; ++i)
    ASSERT_FALSE(Aleph::less<int>()(pivot, a(i)));
  for (long i = p + 1; i < static_cast<long>(a.size()); ++i)
    ASSERT_FALSE(Aleph::less<int>()(a(i), pivot));

  std::vector<int> after;
  after.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    after.push_back(a(i));
  std::sort(before.begin(), before.end());
  std::sort(after.begin(), after.end());
  EXPECT_EQ(before, after);
}

TEST(SortUtilsHelpers, __random_select_dynarray_and_array)
{
  {
    auto a = make_dynarray({4, 1, 3, 2, 0, 2});
    auto expected = std::vector<int>{4, 1, 3, 2, 0, 2};
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 0, 0, 5)), expected[0]);
    EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 3, 0, 5)), expected[3]);
    EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 5, 0, 5)), expected[5]);
  }

  {
    Array<int> a;
    for (int x : {4, 1, 3, 2, 0, 2})
      a.append(x);
    auto expected = std::vector<int>{4, 1, 3, 2, 0, 2};
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 0, 0, 5)), expected[0]);
    EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 3, 0, 5)), expected[3]);
    EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 5, 0, 5)), expected[5]);
  }
}

TEST(SortUtilsHelpers, back_index_and_negate_compare)
{
  EXPECT_EQ(back_index(10), 9);

  auto nc = Negate_Compare<int, Aleph::less<int>>(Aleph::less<int>());
  EXPECT_TRUE(nc(2, 1));
  EXPECT_FALSE(nc(1, 2));
}

TEST(SortUtilsHelpers, select_pivot_and_partition_raw)
{
  int a[] = {4, 1, 3, 2, 0, 2};
  Aleph::less<int> cmp;
  int p = select_pivot<int, Aleph::less<int>>(a, 0, 5, cmp);
  EXPECT_GE(p, 0);
  EXPECT_LE(p, 5);

  int q = partition<int, Aleph::less<int>>(a, 0, 5, cmp);
  EXPECT_GE(q, 0);
  EXPECT_LE(q, 5);
}

TEST(SortUtilsHelpers, merge_raw_partitions)
{
  // [0..2] and [3..5] are already sorted
  int a[] = {0, 2, 4, 1, 3, 5};
  merge(a, 0, 2, 5, Aleph::less<int>());
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsHelpers, push2_fixedstack)
{
  FixedStack<int> st(10);
  push2(st, 1, 2);
  EXPECT_EQ(st.pop(), 1);
  EXPECT_EQ(st.pop(), 2);
  EXPECT_TRUE(st.is_empty());
}

TEST(SortUtilsHelpers, quicksort_no_tail_pointer)
{
  int a[] = {5, 4, 3, 2, 1, 0, 0, 9};
  quicksort_no_tail(a, 0, static_cast<int>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsHelpers, compare_tnode_direct)
{
  Snodenc<int> n1(1), n2(2);
  auto cmp = Compare_Tnode<Slinknc, Snodenc, int, Aleph::less<int>>(Aleph::less<int>());
  EXPECT_TRUE(cmp(static_cast<Slinknc *>(&n1), static_cast<Slinknc *>(&n2)));
  EXPECT_TRUE(cmp(static_cast<Slinknc *>(&n1), 5));
}

TEST(SortUtilsHelpers, insert_sorted_dlink_and_list_insertion_sort)
{
  Dnode<int> h;
  Dlink & base = static_cast<Dlink &>(h);

  auto * n2 = new Dnode<int>(2);
  auto * n0 = new Dnode<int>(0);
  auto * n1 = new Dnode<int>(1);

  using Cmp = Compare_Dnode<int, Aleph::less<int>>;
  Cmp cmp{Aleph::less<int>()};

  insert_sorted<Cmp>(base, n2, cmp);
  insert_sorted<Cmp>(base, n0, cmp);
  insert_sorted<Cmp>(base, n1, cmp);
  EXPECT_EQ(search_extreme<int>(h, Aleph::less<int>())->get_data(), 0);

  // list_insertion_sort on Dlink
  Dnode<int> h2 = make_dnode_list({3, 1, 2, 0});
  Dlink & base2 = static_cast<Dlink &>(h2);
  list_insertion_sort<Dlink, Cmp>(base2, cmp);
  EXPECT_EQ(search_extreme<int>(h2, Aleph::less<int>())->get_data(), 0);

  delete_all_nodes(h);
  delete_all_nodes(h2);
}

TEST(SortUtilsHelpers, insert_sorted_htlist_and_list_insertion_sort)
{
  DynList<int> l;
  l.append(0);
  l.append(2);

  auto * node = new Snodenc<int>(1);
  Compare_Snodenc<int, Aleph::less<int>> cmp{Aleph::less<int>()};
  insert_sorted(l, static_cast<Slinknc *>(node), cmp);
  EXPECT_TRUE(is_sorted(l));

  DynList<int> l2;
  for (int x : {3, 1, 2, 0})
    l2.append(x);
  list_insertion_sort<HTList, Compare_Snodenc<int, Aleph::less<int>>>(
    static_cast<HTList &>(l2), cmp);
  EXPECT_TRUE(is_sorted(l2));
}

TEST(SortUtilsHelpers, dlink_random_search_and_dlink_random_select)
{
  auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
  Dlink & base = static_cast<Dlink &>(h);

  using Cmp = Compare_Dnode<int, Aleph::less<int>>;
  Cmp cmp{Aleph::less<int>()};

  auto * found = dlink_random_search(base, 3, cmp);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_data(), 3);

  auto * sel = dlink_random_select(base, 0, cmp);
  ASSERT_NE(sel, nullptr);
  EXPECT_EQ(static_cast<Dnode<int> *>(sel)->get_data(), 0);

  delete_all_nodes(h);
}

TEST(SortUtilsHelpers, binindex_explicit)
{
  const auto a = make_dynarray({0, 1, 1, 1, 2, 3});
  EXPECT_EQ(binindex(a, 2), 4);
}

TEST(SortUtilsHelpers, __random_select_raw_pointer)
{
  int a[] = {4, 1, 3, 2, 0, 2};
  std::vector<int> expected(std::begin(a), std::end(a));
  std::sort(expected.begin(), expected.end());

  Aleph::less<int> cmp;
  EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 0, 0, 5, cmp)), expected[0]);
  EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 3, 0, 5, cmp)), expected[3]);
  EXPECT_EQ((Aleph::__random_select<int, Aleph::less<int>>(a, 5, 0, 5, cmp)), expected[5]);
}

TEST(SortUtilsHelpers, sift_up_and_sift_down_restore_min_heap)
{
  Aleph::less<int> cmp;

  // sift_up: insert new element at end and bubble up
  {
    DynArray<int> a;
    a.reserve(4);
    a(0) = 1;
    a(1) = 3;
    a(2) = 5;
    a(3) = 0;
    sift_up<int, Aleph::less<int>>(a, 4, cmp);
    EXPECT_TRUE(is_min_heap(a, 4));
  }

  // sift_down: restore heap after root replaced
  {
    DynArray<int> a;
    a.reserve(4);
    a(0) = 3;
    a(1) = 1;
    a(2) = 2;
    a(3) = 0; // outside heap when n=3
    sift_down<int, Aleph::less<int>>(a, 3, cmp);
    EXPECT_TRUE(is_min_heap(a, 3));
  }
}

TEST(SortUtilsArraySorts, mergesort_pointer)
{
  int a[] = {5, 4, 3, 2, 1, 0, 0, 9};
  mergesort(a, 0, static_cast<int>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, quicksort_pointer)
{
  int a[] = {5, 4, 3, 2, 1, 0, 0, 9};
  quicksort(a, 0, static_cast<int>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, quicksort_rec_pointer)
{
  int a[] = {5, 4, 3, 2, 1, 0, 0, 9};
  quicksort_rec(a, 0, static_cast<int>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, quicksort_rec_min_pointer)
{
  int a[] = {5, 4, 3, 2, 1, 0, 0, 9};
  quicksort_rec_min(a, 0, static_cast<int>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, quicksort_insertion_pointer)
{
  int a[] = {9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
  quicksort_insertion(a, 0, static_cast<int>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));

  int b[] = {2, 1};
  quicksort_insertion(b, 0, 1);
  EXPECT_TRUE(std::is_sorted(std::begin(b), std::end(b)));
}

// Introsort tests - hybrid algorithm with O(n log n) guaranteed
TEST(SortUtilsArraySorts, introsort_pointer_basic)
{
  int a[] = {5, 4, 3, 2, 1, 0, 0, 9};
  introsort(a, 0L, static_cast<long>((sizeof(a) / sizeof(a[0])) - 1));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, introsort_pointer_convenience)
{
  int a[] = {9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
  introsort(a, sizeof(a) / sizeof(a[0]));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, introsort_pointer_custom_compare)
{
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  introsort(a, 0L, 8L, std::greater<int>());
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a), std::greater<int>()));
}

TEST(SortUtilsArraySorts, introsort_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  introsort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, introsort_empty_and_single)
{
  // Empty array
  int empty[] = {0};
  introsort(empty, 0);
  EXPECT_EQ(empty[0], 0);

  // Single element
  int single[] = {42};
  introsort(single, 1);
  EXPECT_EQ(single[0], 42);

  // Empty DynArray
  DynArray<int> da;
  EXPECT_NO_THROW(introsort(da));
}

TEST(SortUtilsArraySorts, introsort_already_sorted)
{
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  introsort(a, sizeof(a) / sizeof(a[0]));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, introsort_reverse_sorted)
{
  int a[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  introsort(a, sizeof(a) / sizeof(a[0]));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, introsort_all_equal)
{
  int a[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
  introsort(a, sizeof(a) / sizeof(a[0]));
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, introsort_large_array_forces_heapsort)
{
  // Create an array large enough that might trigger heapsort fallback
  // This tests the depth limit mechanism
  const size_t n = 10000;
  std::vector<int> v(n);
  for (size_t i = 0; i < n; ++i)
    v[i] = static_cast<int>(n - i); // Reverse sorted (worst case for quicksort)

  introsort(v.data(), n);
  EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(SortUtilsArraySorts, introsort_dynarray_large)
{
  // Test with larger DynArray
  const size_t n = 5000;
  DynArray<int> a;
  a.reserve(n);
  for (size_t i = 0; i < n; ++i)
    a(i) = static_cast<int>(n - i); // Reverse sorted

  introsort(a);
  for (size_t i = 1; i < n; ++i)
    ASSERT_LE(a(i - 1), a(i)) << "Failed at index " << i;
}

// Introsort with STL-style pointer interface (begin, end)
TEST(SortUtilsArraySorts, introsort_pointer_begin_end)
{
  int a[] = {9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
  introsort(a, a + 10);  // STL-style: introsort(begin, end)
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a)));
}

TEST(SortUtilsArraySorts, introsort_pointer_begin_end_partial)
{
  int a[] = {9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
  // Sort only middle portion [2, 7)
  introsort(a + 2, a + 7);
  // Elements 0,1 unchanged, 2-6 sorted, 7-9 unchanged
  EXPECT_EQ(a[0], 9);
  EXPECT_EQ(a[1], 1);
  EXPECT_TRUE(std::is_sorted(a + 2, a + 7));
  EXPECT_EQ(a[7], 4);
}

TEST(SortUtilsArraySorts, introsort_pointer_begin_end_custom_compare)
{
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  introsort(a, a + 9, std::greater<int>());
  EXPECT_TRUE(std::is_sorted(std::begin(a), std::end(a), std::greater<int>()));
}

TEST(SortUtilsArraySorts, introsort_pointer_begin_end_empty)
{
  int a[] = {42};
  // Empty range - should not crash
  introsort(a, a);
  EXPECT_EQ(a[0], 42);  // Unchanged
}

// Introsort for Array<T> container
TEST(SortUtilsArraySorts, introsort_array_container)
{
  Array<int> arr;
  arr.append(5);
  arr.append(2);
  arr.append(8);
  arr.append(1);
  arr.append(9);
  arr.append(3);

  introsort(arr);

  for (size_t i = 1; i < arr.size(); ++i)
    ASSERT_LE(arr(i - 1), arr(i));
}

TEST(SortUtilsArraySorts, introsort_array_container_custom_compare)
{
  Array<int> arr;
  for (int i = 1; i <= 10; ++i)
    arr.append(i);

  introsort(arr, std::greater<int>());

  for (size_t i = 1; i < arr.size(); ++i)
    ASSERT_GE(arr(i - 1), arr(i));  // Descending order
}

TEST(SortUtilsArraySorts, introsort_array_container_empty)
{
  Array<int> arr;
  EXPECT_NO_THROW(introsort(arr));
  EXPECT_TRUE(arr.is_empty());
}

TEST(SortUtilsArraySorts, introsort_array_container_single)
{
  Array<int> arr;
  arr.append(42);
  introsort(arr);
  EXPECT_EQ(arr.size(), 1u);
  EXPECT_EQ(arr(0), 42);
}

TEST(SortUtilsArraySorts, introsort_array_container_large)
{
  const size_t n = 5000;
  Array<int> arr(n);
  for (size_t i = 0; i < n; ++i)
    arr.append(static_cast<int>(n - i));  // Reverse sorted

  introsort(arr);

  for (size_t i = 1; i < n; ++i)
    ASSERT_LE(arr(i - 1), arr(i)) << "Failed at index " << i;
}

TEST(SortUtilsArraySorts, heapsort_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  heapsort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, quicksort_op_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  quicksort_op(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, quicksort_op_empty)
{
  DynArray<int> a;
  EXPECT_NO_THROW(quicksort_op(a));
  EXPECT_TRUE(a.is_empty());
}

TEST(SortUtilsArraySorts, shellsort_dynarray)
{
  auto a = make_dynarray({9, 1, 8, 2, 7, 3, 6, 4, 5, 0});
  shellsort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, quicksort_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  quicksort(a);
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsArraySorts, quicksort_rec_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  quicksort_rec(a, 0, static_cast<long>(a.size() - 1));
  for (size_t i = 1; i < a.size(); ++i)
    ASSERT_LE(a(i - 1), a(i));
}

TEST(SortUtilsListSorts, mergesort_dynlist)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  mergesort(l);
  EXPECT_TRUE(is_sorted(l));
}

TEST(SortUtilsListSorts, mergeinsertsort_dynlist)
{
  DynList<int> l = make_dynlist({9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
  mergeinsertsort(l, Aleph::less<int>(), 3);
  EXPECT_TRUE(is_sorted(l));
}

TEST(SortUtilsListSorts, insertion_sort_dynlist_rvalue)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  DynList<int> sorted = insertion_sort(std::move(l));
  EXPECT_TRUE(is_sorted(sorted));
  EXPECT_TRUE(l.is_empty());
}

TEST(SortUtilsListSorts, mergesort_dynlist_rvalue)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  DynList<int> sorted = mergesort(std::move(l));
  EXPECT_TRUE(is_sorted(sorted));
  EXPECT_TRUE(l.is_empty());
}

TEST(SortUtilsListSorts, quicksort_dynlist)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  quicksort(l);
  EXPECT_TRUE(is_sorted(l));
  EXPECT_EQ(l.get_first(), 0);
  EXPECT_EQ(l.get_last(), 4);
}

TEST(SortUtilsListSorts, merge_lists_dynlist)
{
  DynList<int> l1 = make_dynlist({0, 2, 4});
  DynList<int> l2 = make_dynlist({1, 3, 5});
  DynList<int> out;

  merge_lists(l1, l2, out, Aleph::less<int>());
  EXPECT_TRUE(l1.is_empty());
  EXPECT_TRUE(l2.is_empty());
  EXPECT_TRUE(is_sorted(out));
  EXPECT_EQ(out.size(), 6u);
  EXPECT_EQ(out.get_first(), 0);
  EXPECT_EQ(out.get_last(), 5);
}

TEST(SortUtilsListSorts, merge_lists_dnode)
{
  auto l1 = make_dnode_list({0, 2, 4});
  auto l2 = make_dnode_list({1, 3, 5});
  Dnode<int> out;

  merge_lists(l1, l2, out, Aleph::less<int>());

  EXPECT_TRUE(l1.is_empty());
  EXPECT_TRUE(l2.is_empty());
  EXPECT_FALSE(out.is_empty());

  int expected = 0;
  for (Dnode<int>::Iterator it(out); it.has_curr(); it.next_ne(), ++expected)
    EXPECT_EQ(it.get_curr_ne()->get_data(), expected);
  EXPECT_EQ(expected, 6);

  delete_all_nodes(out);
}

TEST(SortUtilsDyndlistSorts, mergesort_dyndlist)
{
  DynDlist<int> l = make_dyndlist({4, 1, 3, 2, 0, 2});
  mergesort(l);
  EXPECT_TRUE(is_sorted(l));
}

TEST(SortUtilsDyndlistSorts, search_extreme_min)
{
  DynDlist<int> l = make_dyndlist({4, 1, 3, 2, 0, 2});
  int * p = search_extreme(l);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 0);
}

TEST(SortUtilsDyndlistSorts, random_select_dyndlist)
{
  DynDlist<int> l = make_dyndlist({4, 1, 3, 2, 0, 2});
  int * p = random_select(l, 0);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 0);

  p = random_select(l, 5);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 4);
}

TEST(SortUtilsDyndlistSorts, random_search_dyndlist)
{
  DynDlist<int> l = make_dyndlist({4, 1, 3, 2, 0, 2});

  int * p = random_search(l, 3);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 3);

  p = random_search(l, 99);
  EXPECT_EQ(p, nullptr);
}

TEST(SortUtilsDnodeDlinkAlgorithms, random_select_and_random_search_on_dlink)
{
  auto h = make_dnode_list({4, 1, 3, 2, 0, 2});

  auto * n0 = random_select<int>(h, 0);
  ASSERT_NE(n0, nullptr);
  EXPECT_EQ(n0->get_data(), 0);

  auto * nmax = random_select<int>(h, 5);
  ASSERT_NE(nmax, nullptr);
  EXPECT_EQ(nmax->get_data(), 4);

  auto * nfound = random_search<int>(h, 3);
  ASSERT_NE(nfound, nullptr);
  EXPECT_EQ(nfound->get_data(), 3);

  auto * nmiss = random_search<int>(h, 99);
  EXPECT_EQ(nmiss, nullptr);

  delete_all_nodes(h);
}

TEST(SortUtilsHTListPath, quicksort_htlist_via_dynlist)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  quicksort<int>(static_cast<HTList &>(l), Aleph::less<int>());
  EXPECT_TRUE(is_sorted(l));
}

TEST(SortUtilsSearchSequential, sequential_search_raw_array)
{
  int a[] = {4, 1, 3, 2, 0, 2};
  EXPECT_EQ(sequential_search(a, int{3}, 0, 5), 2);
  EXPECT_EQ(sequential_search(a, int{99}, 0, 5), Not_Found);
}

TEST(SortUtilsSearchSequential, sequential_search_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  EXPECT_EQ(sequential_search(a, 3, 0, static_cast<int>(a.size() - 1)), 2);
  EXPECT_EQ(sequential_search(a, 99, 0, static_cast<int>(a.size() - 1)), Not_Found);
}

TEST(SortUtilsSearchSequential, sequential_search_dynlist)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  int * p = sequential_search(l, 3);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 3);

  p = sequential_search(l, 99);
  EXPECT_EQ(p, nullptr);
}

TEST(SortUtilsSearchSequential, sequential_search_dyndlist)
{
  DynDlist<int> l = make_dyndlist({4, 1, 3, 2, 0, 2});
  int * p = sequential_search(l, 3);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 3);
  p = sequential_search(l, 99);
  EXPECT_EQ(p, nullptr);
}

TEST(SortUtilsSearchSequential, sequential_search_dnode)
{
  auto h = make_dnode_list({4, 1, 3, 2, 0, 2});

  auto * n = sequential_search(h, 3);
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(n->get_data(), 3);

  n = sequential_search(h, 99);
  EXPECT_EQ(n, nullptr);

  delete_all_nodes(h);
}

TEST(SortUtilsSearchExtreme, search_extreme_dnode_min)
{
  auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
  auto * n = search_extreme<int>(h, Aleph::less<int>());
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(n->get_data(), 0);
  delete_all_nodes(h);
}

TEST(SortUtilsSearchExtreme, search_min_max_raw_array)
{
  int a[] = {4, 1, 3, 2, 0, 2};
  EXPECT_EQ(search_min(a, 0, 5), 4);
  EXPECT_EQ(search_max(a, 0, 5), 0);
}

TEST(SortUtilsSearchExtreme, search_extreme_and_search_max_dynarray)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  EXPECT_EQ(search_extreme(a, 0, static_cast<long>(a.size() - 1)), 4);
  EXPECT_EQ(search_max(a, 0, static_cast<long>(a.size() - 1)), 0);
}

TEST(SortUtilsSearchExtreme, search_min_max_dyndlist)
{
  DynDlist<int> l = make_dyndlist({4, 1, 3, 2, 0, 2});
  int * mn = search_min(l);
  int * mx = search_max(l);
  ASSERT_NE(mn, nullptr);
  ASSERT_NE(mx, nullptr);
  EXPECT_EQ(*mn, 0);
  EXPECT_EQ(*mx, 4);
}

TEST(SortUtilsSearchExtreme, search_extreme_dynlist_min_max)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  int * mn = search_extreme(l);
  int * mx = search_extreme(l, Aleph::greater<int>());
  ASSERT_NE(mn, nullptr);
  ASSERT_NE(mx, nullptr);
  EXPECT_EQ(*mn, 0);
  EXPECT_EQ(*mx, 4);
}

TEST(SortUtilsSearchExtreme, search_min_max_dynlist)
{
  DynList<int> l = make_dynlist({4, 1, 3, 2, 0, 2});
  int * mn = search_extreme(l, Aleph::less<int>());
  int * mx = search_extreme(l, Aleph::greater<int>());
  ASSERT_NE(mn, nullptr);
  ASSERT_NE(mx, nullptr);
  EXPECT_EQ(*mn, 0);
  EXPECT_EQ(*mx, 4);
}

TEST(SortUtilsDnodeSorts, selection_insertion_quick_merge)
{
  {
    auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
    selection_sort(h, Aleph::less<int>());
    EXPECT_EQ(search_extreme<int>(h, Aleph::less<int>())->get_data(), 0);
    delete_all_nodes(h);
  }

  {
    auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
    insertion_sort(h);
    EXPECT_EQ(search_extreme<int>(h, Aleph::less<int>())->get_data(), 0);
    delete_all_nodes(h);
  }

  {
    auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
    quicksort(h);
    EXPECT_EQ(search_extreme<int>(h, Aleph::less<int>())->get_data(), 0);
    delete_all_nodes(h);
  }

  {
    auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
    mergesort(h);
    EXPECT_EQ(search_extreme<int>(h, Aleph::less<int>())->get_data(), 0);
    delete_all_nodes(h);
  }
}

TEST(SortUtilsDlinkSorts, quicksort_dlink)
{
  auto h = make_dnode_list({4, 1, 3, 2, 0, 2});
  Dlink & base = static_cast<Dlink &>(h);

  using Cmp = Compare_Dnode<int, Aleph::less<int>>;
  quicksort(base, Cmp(Aleph::less<int>()));

  EXPECT_EQ(search_extreme<int>(h, Aleph::less<int>())->get_data(), 0);
  EXPECT_EQ(search_extreme<int>(h, Aleph::greater<int>())->get_data(), 4);

  delete_all_nodes(h);
}

TEST(SortUtilsSearch, binary_search_dup_and_bsearch)
{
  auto a = make_dynarray({0, 1, 1, 1, 2, 3});

  auto idxs = binary_search_dup(a, 1);
  ASSERT_EQ(idxs.size(), 3u);
  EXPECT_EQ(idxs.get_first(), 1u);
  EXPECT_EQ(idxs.get_last(), 3u);

  int * p = bsearch(a, 1);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 1);
  EXPECT_EQ(bsearch(a, 99), nullptr);
}

TEST(SortUtilsSearch, binary_search_dup_boundary_cases)
{
  {
    auto a = make_dynarray({1, 1, 1, 2, 3});
    auto idxs = binary_search_dup(a, 1);
    ASSERT_EQ(idxs.size(), 3u);
    EXPECT_EQ(idxs.get_first(), 0u);
    EXPECT_EQ(idxs.get_last(), 2u);
  }

  {
    auto a = make_dynarray({0, 1, 2, 3, 3, 3});
    auto idxs = binary_search_dup(a, 3);
    ASSERT_EQ(idxs.size(), 3u);
    EXPECT_EQ(idxs.get_first(), 3u);
    EXPECT_EQ(idxs.get_last(), 5u);
  }
}

TEST(SortUtilsSearch, binary_search_dup_custom_compare_descending)
{
  // Descending sorted with duplicates
  auto a = make_dynarray({5, 4, 3, 3, 3, 2, 1, 1, 0});
  auto idxs = binary_search_dup(a, 3, Aleph::greater<int>());
  ASSERT_EQ(idxs.size(), 3u);
  EXPECT_EQ(idxs.get_first(), 2u);
  EXPECT_EQ(idxs.get_last(), 4u);

  idxs = binary_search_dup(a, 1, Aleph::greater<int>());
  ASSERT_EQ(idxs.size(), 2u);
  EXPECT_EQ(idxs.get_first(), 6u);
  EXPECT_EQ(idxs.get_last(), 7u);
}

TEST(SortUtilsSearch, bsearch_dup_custom_compare_descending)
{
  auto a = make_dynarray({5, 4, 3, 3, 3, 2, 1, 1, 0});
  auto ps = bsearch_dup(a, 3, Aleph::greater<int>());
  ASSERT_EQ(ps.size(), 3u);
  for (auto p : ps)
    {
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(*p, 3);
    }
}

TEST(SortUtilsIndexBuild, build_index_and_build_index_ptr)
{
  auto a = make_dynarray({3, 1, 2, 1, 0});

  auto idx = build_index(a);
  ASSERT_EQ(idx.size(), a.size());
  for (size_t i = 1; i < idx.size(); ++i)
    ASSERT_LE(a(idx(i - 1)), a(idx(i)));

  auto ptrs = build_index_ptr(a);
  ASSERT_EQ(ptrs.size(), a.size());
  for (size_t i = 1; i < ptrs.size(); ++i)
    ASSERT_LE(*ptrs(i - 1), *ptrs(i));
}

TEST(SortUtilsSlinkncPath, sequential_search_and_search_extreme_on_slinknc)
{
  Slinknc head;
  auto * n1 = new Snodenc<int>(4);
  auto * n2 = new Snodenc<int>(1);
  auto * n3 = new Snodenc<int>(3);
  auto * n4 = new Snodenc<int>(0);
  head.insert(n1);
  head.insert(n2);
  head.insert(n3);
  head.insert(n4);

  auto found = sequential_search(head, 3);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(static_cast<Snodenc<int>*>(found)->get_data(), 3);

  auto missing = sequential_search(head, 99);
  EXPECT_EQ(missing, nullptr);

  auto extreme_min = search_extreme(head, Compare_Snodenc<int, Aleph::less<int>>(Aleph::less<int>()));
  ASSERT_NE(extreme_min, nullptr);
  EXPECT_EQ(static_cast<Snodenc<int>*>(extreme_min)->get_data(), 0);

  while (not head.is_empty())
    delete static_cast<Snodenc<int>*>(head.remove_next());
}

TEST(SortUtilsSlinkncPath, sequential_search_default_equal_on_slinknc)
{
  Slinknc head;
  auto * n1 = new Snodenc<int>(2);
  auto * n2 = new Snodenc<int>(1);
  auto * n3 = new Snodenc<int>(0);
  head.insert(n1);
  head.insert(n2);
  head.insert(n3);

  auto * found = sequential_search<int>(head, 1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->to_data<int>(), 1);

  auto * missing = sequential_search<int>(head, 99);
  EXPECT_EQ(missing, nullptr);

  while (not head.is_empty())
    delete static_cast<Snodenc<int>*>(head.remove_next());
}

TEST(SortUtilsDlinkPath, sequential_search_and_selection_sort_on_dlink)
{
  auto h = make_dnode_list({3, 1, 2, 0});
  Dlink & base = static_cast<Dlink &>(h);

  auto * found = sequential_search<int>(base, 2);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(static_cast<Dnode<int>*>(found)->get_data(), 2);

  selection_sort(base, Compare_Dnode<int, Aleph::less<int>>(Aleph::less<int>()));
  auto * mn = search_extreme<int>(h, Aleph::less<int>());
  ASSERT_NE(mn, nullptr);
  EXPECT_EQ(mn->get_data(), 0);

  delete_all_nodes(h);
}

TEST(SortUtilsRandomSearch, random_search_raw_and_dynarray)
{
  int a[] = {4, 1, 3, 2, 0, 2};
  int idx = random_search(a, 3, 0, 5);
  ASSERT_NE(idx, Not_Found);
  EXPECT_EQ(a[idx], 3);
  EXPECT_EQ(random_search(a, 99, 0, 5), Not_Found);

  auto d = make_dynarray({4, 1, 3, 2, 0, 2});
  idx = random_search(d, 3, 0, static_cast<long>(d.size() - 1));
  ASSERT_NE(idx, Not_Found);
  EXPECT_EQ(d(idx), 3);
  EXPECT_EQ(random_search(d, 99, 0, static_cast<long>(d.size() - 1)), Not_Found);
}

TEST(SortUtilsRandomSelect, random_select_array_container)
{
  Array<int> a;
  for (int x : {4, 1, 3, 2, 0, 2})
    a.append(x);

  EXPECT_EQ(random_select(a, 0), 0);
  EXPECT_EQ(random_select(a, 5), 4);
}

TEST(SortUtilsRandomSelect, random_select_out_of_range_and_empty)
{
  {
    DynArray<int> a;
    EXPECT_THROW(random_select(a, 0), std::out_of_range);
  }

  {
    auto a = make_dynarray({4, 1, 3});
    EXPECT_THROW(random_select(a, 3), std::out_of_range);
  }

  {
    Array<int> a;
    a.append(1);
    EXPECT_THROW(random_select(a, 1), std::out_of_range);
  }

  {
    int b[] = {4, 1, 3};
    using Cmp = Aleph::less<int>;
    auto fn = static_cast<const int & (*)(int *, const long, const long, const Cmp &)>(
      &Aleph::random_select<int, Cmp>);
    EXPECT_THROW(fn(b, 3, 3, Cmp()), std::out_of_range);
  }

  {
    Dnode<int> h;
    EXPECT_EQ(random_select<int>(h, 0), nullptr);
    EXPECT_EQ(random_select<int>(h, 1), nullptr);
  }

  {
    DynDlist<int> l;
    EXPECT_EQ(random_select(l, 0), nullptr);
    EXPECT_EQ(random_select(l, 1), nullptr);
  }

  {
    auto h = make_dnode_list({4, 1});
    EXPECT_THROW(random_select<int>(h, 2), std::out_of_range);
    delete_all_nodes(h);
  }

  {
    auto l = make_dyndlist({4, 1});
    EXPECT_THROW(random_select(l, 2), std::out_of_range);
  }
}

TEST(SortUtilsSearch, binary_search_insertion_point_container)
{
  auto a = make_dynarray({0, 2, 4, 6});

  EXPECT_EQ(binary_search(a, 0), 0);
  EXPECT_EQ(binary_search(a, 6), 3);

  // insertion points
  EXPECT_EQ(binary_search(a, 1), 1);
  EXPECT_EQ(binary_search(a, 5), 3);
  EXPECT_EQ(binary_search(a, 7), 4);
}

TEST(SortUtilsSearch, binary_search_insertion_point_raw)
{
  int a[] = {0, 2, 4, 6};
  EXPECT_EQ(binary_search(a, 0, 0, 3), 0);
  EXPECT_EQ(binary_search(a, 6, 0, 3), 3);
  EXPECT_EQ(binary_search(a, 1, 0, 3), 1);
  EXPECT_EQ(binary_search(a, 5, 0, 3), 3);
  EXPECT_EQ(binary_search(a, 7, 0, 3), 4);
}

TEST(SortUtilsSearch, binary_search_rec_insertion_point_raw)
{
  int a[] = {0, 2, 4, 6};
  EXPECT_EQ(binary_search_rec(a, 0, 0, 3), 0);
  EXPECT_EQ(binary_search_rec(a, 6, 0, 3), 3);
  EXPECT_EQ(binary_search_rec(a, 1, 0, 3), 1);
  EXPECT_EQ(binary_search_rec(a, 5, 0, 3), 3);
  EXPECT_EQ(binary_search_rec(a, 7, 0, 3), 4);
}

TEST(SortUtilsSearch, random_select_dynarray_and_raw)
{
  auto a = make_dynarray({4, 1, 3, 2, 0, 2});
  EXPECT_EQ(random_select(a, 0), 0);
  EXPECT_EQ(random_select(a, 5), 4);

  int b[] = {4, 1, 3, 2, 0, 2};
  using Cmp = Aleph::less<int>;
  auto fn = static_cast<const int & (*)(int *, const long, const long, const Cmp &)>(
    &Aleph::random_select<int, Cmp>);
  EXPECT_EQ(fn(b, 0, 6, Cmp()), 0);
  EXPECT_EQ(fn(b, 5, 6, Cmp()), 4);
}

TEST(SortUtilsSearch, binary_search_ptr_container)
{
  auto a = make_dynarray({0, 2, 4, 6});
  DynArray<int *> idx;
  idx.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    idx(i) = &a(i);

  // already sorted pointers by value
  EXPECT_EQ(binary_search(idx, 0), 0);
  EXPECT_EQ(binary_search(idx, 6), 3);
  EXPECT_EQ(binary_search(idx, 1), 1);
  EXPECT_EQ(binary_search(idx, 7), 4);
}

TEST(SortUtilsSearch, bsearch_dup_and_binindex_dup)
{
  auto a = make_dynarray({0, 1, 1, 1, 2, 3});
  auto ptrs = bsearch_dup(a, 1);
  ASSERT_EQ(ptrs.size(), 3u);
  for (auto p : ptrs)
    ASSERT_NE(p, nullptr);

  auto idxs = binindex_dup(a, 1);
  ASSERT_EQ(idxs.size(), 3u);
  EXPECT_EQ(idxs.get_first(), 1);
  EXPECT_EQ(idxs.get_last(), 3);
}

TEST(SortUtilsSearch, bsearch_dup_ptr_container_custom_compare_descending)
{
  auto a = make_dynarray({5, 4, 3, 3, 3, 2, 1, 1, 0});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  auto dup = bsearch_dup(ptrs, 3, Aleph::greater<int>());
  ASSERT_EQ(dup.size(), 3u);
  for (auto p : dup)
    {
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(*p, 3);
    }
}

TEST(SortUtilsSearch, binindex_dup_ptr_container_custom_compare_descending)
{
  auto a = make_dynarray({5, 4, 3, 3, 3, 2, 1, 1, 0});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  auto idxs = binindex_dup(ptrs, 3, Aleph::greater<int>());
  ASSERT_EQ(idxs.size(), 3u);
  EXPECT_EQ(idxs.get_first(), 2);
  EXPECT_EQ(idxs.get_last(), 4);
}

TEST(SortUtilsSearch, bsearch_dup_ptr_container)
{
  auto a = make_dynarray({0, 1, 1, 1, 2, 3});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  auto found = bsearch(ptrs, 1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*found, 1);

  auto dup = bsearch_dup(ptrs, 1);
  ASSERT_EQ(dup.size(), 3u);
  for (auto p : dup)
    {
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(*p, 1);
    }
}

TEST(SortUtilsSearch, binindex_dup_ptr_container)
{
  auto a = make_dynarray({0, 1, 1, 1, 2, 3});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  auto idxs = binindex_dup(ptrs, 1);
  ASSERT_EQ(idxs.size(), 3u);
  EXPECT_EQ(idxs.get_first(), 1);
  EXPECT_EQ(idxs.get_last(), 3);
}

TEST(SortUtilsSearch, binary_search_ptr_container_custom_compare)
{
  auto a = make_dynarray({5, 4, 3, 2, 1, 0});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  // The container is sorted in descending order, so we must use greater<int>
  EXPECT_EQ(binary_search(ptrs, 5, Aleph::greater<int>()), 0);
  EXPECT_EQ(binary_search(ptrs, 0, Aleph::greater<int>()), 5);
  EXPECT_EQ(binary_search(ptrs, 3, Aleph::greater<int>()), 2);
}

TEST(SortUtilsSearch, binary_search_ptr_container_range_less)
{
  auto a = make_dynarray({0, 1, 2, 3, 4, 5});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  // search only in [2..4] => values {2,3,4}
  EXPECT_EQ(binary_search(ptrs, 3, 2, 4), 3);

  // insertion points within the restricted range
  EXPECT_EQ(binary_search(ptrs, 1, 2, 4), 2);
  EXPECT_EQ(binary_search(ptrs, 5, 2, 4), 5);
}

TEST(SortUtilsSearch, binary_search_ptr_container_range_greater)
{
  auto a = make_dynarray({5, 4, 3, 2, 1, 0});
  DynArray<int *> ptrs;
  ptrs.reserve(a.size());
  for (size_t i = 0; i < a.size(); ++i)
    ptrs(i) = &a(i);

  // search only in [1..3] => values {4,3,2} under greater<int>
  EXPECT_EQ(binary_search(ptrs, 3, 1, 3, Aleph::greater<int>()), 2);

  // insertion points within the restricted range for descending order
  // 5 would be inserted before 4 => at l
  EXPECT_EQ(binary_search(ptrs, 5, 1, 3, Aleph::greater<int>()), 1);
  // 0 would be inserted after 2 => at r+1
  EXPECT_EQ(binary_search(ptrs, 0, 1, 3, Aleph::greater<int>()), 4);
}

} // namespace
