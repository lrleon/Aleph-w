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

# include <gtest/gtest.h>

# include <ah-zip.H>
# include <ahFunctional.H>
# include <ah-string-utils.H>
# include <tpl_dynSetTree.H>
# include <tpl_odhash.H>
# include <tpl_dynArray.H>

using namespace std;
using namespace Aleph;

TEST(range, range_combs)
{
  EXPECT_EQ(range(0, 10), build_dynlist<int>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
  EXPECT_EQ(range(0, 10, 2), build_dynlist<int>(0, 2, 4, 6, 8, 10));
  EXPECT_EQ(nrange(2, 5, 3), build_dynlist<int>(2, 3, 5));
  EXPECT_EQ(contiguous_range(1, 5), build_dynlist<int>(1, 2, 3, 4, 5));
  EXPECT_EQ(range(5), build_dynlist<int>(0, 1, 2, 3, 4));
}

TEST(rep, rep)
{
  EXPECT_EQ(rep(5, 0), build_dynlist<int>(0, 0, 0, 0, 0));
  EXPECT_EQ(rep(3), build_dynlist<int>(0, 0, 0));
}

TEST(set_range, set_range_with_op)
{
  auto sq = [] (int x) { return x * x; };
  auto r = set_range<int, DynList>(0, 4, 1, sq);
  EXPECT_EQ(r, build_dynlist<int>(0, 1, 4, 9, 16));

  auto r2 = set_range<int, DynList>(0, 6, 2, sq);
  EXPECT_EQ(r2, build_dynlist<int>(0, 4, 16, 36));
}

TEST(each, each_variations)
{
  size_t count = 0;
  each(5, [&count] () { ++count; });
  EXPECT_EQ(count, 5);

  count = 0;
  each(2, 5, [&count] () { ++count; });
  EXPECT_EQ(count, 4);
}

struct TreeContainer : testing::Test
{
  static constexpr size_t N = 10;
  DynSetTree<size_t> tbl;
  TreeContainer()
  {
    tbl = range<size_t>(0, N - 1);
  }
};

TEST_F(TreeContainer, pointers)
{
  auto l = pointers_list(tbl);

  size_t i = 0;
  for (auto it = l.get_it(); it.has_curr(); it.next_ne(), ++i)
    EXPECT_EQ(*it.get_curr_ne(), i);

  EXPECT_TRUE(zip_all([] (auto t) { return *get<0>(t) == get<1>(t); }, l, tbl));
}

TEST_F(TreeContainer, sublist)
{
  EXPECT_EQ(sublist(tbl, 2), build_dynlist<size_t>(0, 2, 4, 6, 8));
  EXPECT_EQ(sublist(tbl, 1, 3), build_dynlist<size_t>(1, 4, 7));
}

TEST_F(TreeContainer, ml)
{
  size_t i = 0;
  for_each(tbl, [&i] (auto & k)
	   {
	     EXPECT_EQ(i++, k);
	   });

  enum_for_each(tbl, ([] (auto & k, auto & i)
	       {
		 EXPECT_EQ(i, k);
	       }));

  i = 0;
  EXPECT_TRUE(all(tbl, [&i] (auto & k) { return k == i++; }));

  EXPECT_TRUE(exists(tbl, [] (auto & i) { return i == 3; }));
  EXPECT_FALSE(exists(tbl, [] (auto & i) { return i == N; }));

  auto lfilt = filter(tbl, [] (auto & i) { return i <= 3; });
  EXPECT_EQ(lfilt, build_dynlist<size_t>(0, 1, 2, 3));

  auto lp = maps<string>(tbl, [] (const auto & i) { return to_string(i); });
  EXPECT_EQ(lp, build_dynlist<string>
	    ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9"));

  auto lf = foldl<size_t>(tbl, 0, [] (auto & a, auto & i) { return a + i; });
  EXPECT_EQ(lf, N*(N - 1)/2);
}

TEST_F(TreeContainer, gen_seq_list_tuples)
{
  auto l = gen_seq_list_tuples(tbl, 7);
  EXPECT_EQ(l, build_dynlist<DynList<size_t>>
	    ( build_dynlist<size_t>(0, 1, 2, 3, 4, 5, 6),
	      build_dynlist<size_t>(1, 2, 3, 4, 5, 6, 7),
	      build_dynlist<size_t>(2, 3, 4, 5, 6, 7, 8),
	      build_dynlist<size_t>(3, 4, 5, 6, 7, 8, 9) ));
}

TEST_F(TreeContainer, enumerate)
{
  auto l = enumerate(tbl);
  EXPECT_TRUE(l.all([] (auto & p) { return p.first == p.second; }));
}

TEST_F(TreeContainer, indexes_and_tindexes)
{
  auto idx = indexes(tbl);
  size_t i = 0;
  for (auto it = idx.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto & p = it.get_curr();
      EXPECT_EQ(p.first, i);
      EXPECT_EQ(p.second, i);
    }

  auto tidx = tindexes(tbl);
  i = 0;
  for (auto it = tidx.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto & t = it.get_curr();
      EXPECT_EQ(get<0>(t), i);
      EXPECT_EQ(get<1>(t), i);
    }
}

TEST_F(TreeContainer, reverse_test)
{
  DynList<size_t> l = range<size_t>(0, N - 1);
  auto rev = reverse(l);
  EXPECT_EQ(rev, build_dynlist<size_t>(9, 8, 7, 6, 5, 4, 3, 2, 1, 0));
}

TEST_F(TreeContainer, partition_test)
{
  auto p = partition(tbl, [] (const size_t & i) { return i < 5; });
  EXPECT_EQ(p.first, build_dynlist<size_t>(0, 1, 2, 3, 4));
  EXPECT_EQ(p.second, build_dynlist<size_t>(5, 6, 7, 8, 9));
}

TEST_F(TreeContainer, diff_test)
{
  DynList<size_t> l1 = range<size_t>(0, N - 1);
  DynList<size_t> l2 = range<size_t>(0, N - 1);
  EXPECT_FALSE(diff(l1, l2));

  l2.get_last() = 100;
  EXPECT_TRUE(diff(l1, l2));
}

TEST_F(TreeContainer, containers_eq_test)
{
  DynList<size_t> l = range<size_t>(0, N - 1);
  EXPECT_TRUE(containers_eq(tbl, l, std::equal_to<size_t>()));
}

TEST(Compare, comparisons)
{
  constexpr size_t N = 20;
  DynList<size_t> l1 = range(N);
  DynList<size_t> l2 = range(N + 1);
  DynSetTree<size_t> s1 = range(N);
  DynSetTree<size_t> s2 = range(N + 1);

  EXPECT_TRUE(eq(l1, s1));
  EXPECT_TRUE(lesser(l1, l2));
  EXPECT_TRUE(lesser(l1, s2));
  EXPECT_FALSE(lesser(l1, s1));
  EXPECT_FALSE(lesser(s1, l1));

  auto d = are_eq(l1, s1);
  EXPECT_TRUE(get<0>(d));
  EXPECT_EQ(get<1>(d), N);

  // Now we modify the last item of l1
  l1.get_last() = N - 2;
  EXPECT_FALSE(eq(l1, s1));
  EXPECT_TRUE(lesser(l1, s1));

  d = are_eq(l1, s1);
  EXPECT_FALSE(get<0>(d));
  EXPECT_EQ(get<1>(d), N - 1);
  EXPECT_EQ(get<2>(d), N - 2);
  EXPECT_EQ(get<3>(d), N - 1);
}

TEST_F(TreeContainer, zips)
{
  {
    auto z = zip(tbl, range(N));
    EXPECT_TRUE(z.all([] (auto & p) { return p.first == p.second; }));

    auto p = unzip(z);
    EXPECT_TRUE(zip_all([] (auto p) { return get<0>(p) == get<1>(p); },
			p.first, p.second));
  }
  {
    auto z = tzip(tbl, range(N));
    EXPECT_TRUE(z.all([] (auto & p) { return get<0>(p) == get<1>(p); }));

    auto p = tunzip(z);
    EXPECT_TRUE(zip_all([] (auto p) { return get<0>(p) == get<1>(p); },
			get<0>(p), get<1>(p)));
  }
  {
    auto z = zipEq(tbl, range(N));
    EXPECT_TRUE(z.all([] (auto & p) { return p.first == p.second; }));

    EXPECT_THROW(zipEq(tbl, range(N + 1)), length_error);

    auto p = unzip(z);
    EXPECT_TRUE(zip_all([] (auto p) { return get<0>(p) == get<1>(p); },
  			p.first, p.second));
  }
  {
    auto z = tzipEq(tbl, range(N));
    EXPECT_TRUE(z.all([] (auto & p) { return get<0>(p) == get<1>(p); }));

    EXPECT_THROW(tzipEq(tbl, range(N + 1)), length_error);

    auto p = tunzip(z);
    EXPECT_TRUE(zip_all([] (auto p) { return get<0>(p) == get<1>(p); },
  			get<0>(p), get<1>(p)));
  }
}

TEST(sequential_groups_test, sequential_groups)
{
  DynList<int> l = build_dynlist<int>(1, 1, 2, 2, 2, 4, 4, 0, 0, 1);
  auto result = sequential_groups(l);
  EXPECT_EQ(result.second, 5);
  auto & groups = result.first;

  auto it = groups.get_it();
  EXPECT_EQ(it.get_curr(), build_dynlist<int>(1, 1)); it.next();
  EXPECT_EQ(it.get_curr(), build_dynlist<int>(2, 2, 2)); it.next();
  EXPECT_EQ(it.get_curr(), build_dynlist<int>(4, 4)); it.next();
  EXPECT_EQ(it.get_curr(), build_dynlist<int>(0, 0)); it.next();
  EXPECT_EQ(it.get_curr(), build_dynlist<int>(1));

  // Empty container
  DynList<int> empty;
  auto empty_result = sequential_groups(empty);
  EXPECT_EQ(empty_result.second, 0);
  EXPECT_TRUE(empty_result.first.is_empty());
}

TEST(unique_sequential_test, unique_sequential)
{
  DynList<int> l = build_dynlist<int>(1, 1, 2, 2, 2, 4, 4, 0, 0, 1);
  auto result = unique_sequential(l);
  EXPECT_EQ(result.second, 5);
  EXPECT_EQ(result.first, build_dynlist<int>(1, 2, 4, 0, 1));

  // Empty container
  DynList<int> empty;
  auto empty_result = unique_sequential(empty);
  EXPECT_EQ(empty_result.second, 0);
  EXPECT_TRUE(empty_result.first.is_empty());
}

TEST(pair_iterator_test, pair_iterator)
{
  DynList<int> l1 = range(5);
  DynList<int> l2 = range(5);

  auto pit = get_pair_it(l1, l2);
  size_t i = 0;
  while (pit.has_curr())
    {
      auto p = pit.get_curr();
      EXPECT_EQ(p.first, i);
      EXPECT_EQ(p.second, i);
      pit.next();
      ++i;
    }
  EXPECT_EQ(i, 5);
  EXPECT_TRUE(pit.was_traversed());

  // Test with position
  auto pit2 = get_pair_it(l1, l2, 2);
  auto p = pit2.get_curr();
  EXPECT_EQ(p.first, 2);
  EXPECT_EQ(p.second, 2);
}

TEST(container_ops, insert_append_remove)
{
  DynList<int> c;
  size_t n = append_in_container(c, 1, 2, 3);
  EXPECT_EQ(n, 3);
  EXPECT_EQ(c, build_dynlist<int>(1, 2, 3));

  DynSetTree<int> s;
  n = insert_in_container(s, 5, 3, 1, 4, 2);
  EXPECT_EQ(n, 5);
  EXPECT_EQ(s.size(), 5);

  n = remove_from_container(s, 1, 2, 3);
  EXPECT_EQ(n, 3);
  EXPECT_EQ(s.size(), 2);
}

TEST(assign_container_test, assign_container)
{
  DynList<int> src = build_dynlist<int>(1, 2, 3, 4, 5);
  auto dst = assign_container<DynList<int>, DynList<int>>(src);
  EXPECT_EQ(src, dst);
}

TEST(flatten_test, flatten)
{
  DynList<DynList<int>> ll;
  ll.append(build_dynlist<int>(1, 2, 3));
  ll.append(build_dynlist<int>(4, 5));
  ll.append(build_dynlist<int>(6, 7, 8, 9));

  auto flat = flatten(ll);
  EXPECT_EQ(flat, build_dynlist<int>(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(is_inside_test, is_inside)
{
  DynList<int> l = build_dynlist<int>(1, 3, 5, 7, 9);
  EXPECT_TRUE(is_inside(3, l));
  EXPECT_TRUE(is_inside(9, l));
  EXPECT_FALSE(is_inside(2, l));
  EXPECT_FALSE(is_inside(10, l));
}

TEST(is_equal_test, is_equal)
{
  EXPECT_TRUE(is_equal(5, 3, 5, 7));
  EXPECT_TRUE(is_equal(5, 5));
  EXPECT_FALSE(is_equal(5, 1, 2, 3, 4));
  EXPECT_FALSE(is_equal(5));

  // Mixed types
  EXPECT_TRUE(is_equal(5, 5.0));
  EXPECT_TRUE(is_equal(5, 3, 5.0, 7));
}

TEST(FoundItem, FoundItemTests)
{
  int val = 42;
  Some<int> s(val);
  EXPECT_TRUE(s.is_found());
  EXPECT_EQ(s.get_item(), 42);
  s.get_item() = 43;
  EXPECT_EQ(val, 43);

  None<int> n;
  EXPECT_FALSE(n.is_found());
  EXPECT_THROW(n.get_item(), std::logic_error);

  const None<int> cn;
  EXPECT_THROW(cn.get_item(), std::logic_error);

  const Some<int> cs(val);
  EXPECT_EQ(cs.get_item(), 43);
}

TEST(range, range_precision)
{
  // Floating point nrange
  auto r = nrange<double, DynList>(0.0, 1.0, 11);
  EXPECT_EQ(r.size(), 11u);
  EXPECT_DOUBLE_EQ(r.get_first(), 0.0);
  EXPECT_DOUBLE_EQ(r.get_last(), 1.0);
  EXPECT_DOUBLE_EQ(r.nth(5), 0.5);

  // n=1 case
  auto r1 = nrange<int, DynList>(10, 20, 1);
  ASSERT_EQ(r1.size(), 1u);
  EXPECT_EQ(r1.get_first(), 10);

  // n=0 case
  EXPECT_THROW(nrange(0, 10, 0), std::domain_error);
}

TEST(flatten_test, flatten_deep)
{
  DynList<DynList<DynList<int>>> lll;
  lll.append(build_dynlist<DynList<int>>(build_dynlist<int>(1, 2), build_dynlist<int>(3)));
  lll.append(build_dynlist<DynList<int>>(build_dynlist<int>(4, 5, 6)));

  auto flat = flatten(lll);
  EXPECT_EQ(flat, build_dynlist<int>(1, 2, 3, 4, 5, 6));

  DynList<DynList<DynList<DynList<int>>>> llll;
  llll.append(lll);
  auto flat4 = flatten(llll);
  EXPECT_EQ(flat4, build_dynlist<int>(1, 2, 3, 4, 5, 6));
}

TEST(each_test, each_edge)
{
  size_t count = 0;
  each(0, [&count] () { ++count; });
  EXPECT_EQ(count, 0);

  count = 0;
  each(1, [&count] () { ++count; });
  EXPECT_EQ(count, 1);
}

// Tests for new functions

TEST(none_test, none_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);
  EXPECT_TRUE(none(l, [] (int x) { return x > 10; }));
  EXPECT_FALSE(none(l, [] (int x) { return x == 3; }));

  DynList<int> empty;
  EXPECT_TRUE(none(empty, [] (int) { return true; }));
}

TEST(find_ptr_test, find_ptr_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);

  auto * p = find_ptr(l, [] (int x) { return x == 3; });
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 3);

  // Modify through pointer
  *p = 30;
  EXPECT_EQ(l.nth(2), 30);

  auto * not_found = find_ptr(l, [] (int x) { return x == 100; });
  EXPECT_EQ(not_found, nullptr);

  // const version
  const DynList<int> cl = build_dynlist<int>(10, 20, 30);
  const int * cp = find_ptr(cl, [] (int x) { return x == 20; });
  ASSERT_NE(cp, nullptr);
  EXPECT_EQ(*cp, 20);
}

TEST(foldr_test, foldr_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4);

  // foldr with subtraction: 1 - (2 - (3 - (4 - 0))) = 1 - (2 - (3 - 4)) = 1 - (2 - (-1)) = 1 - 3 = -2
  auto result = foldr<int>(l, 0, [] (int a, int b) { return a - b; });
  EXPECT_EQ(result, -2);

  // Compare with foldl: ((((0 - 1) - 2) - 3) - 4) = -10
  auto result_l = foldl<int>(l, 0, [] (int a, int b) { return a - b; });
  EXPECT_EQ(result_l, -10);
}

TEST(sum_product_test, sum_product_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);
  EXPECT_EQ(sum(l), 15);
  EXPECT_EQ(sum(l, 10), 25);

  EXPECT_EQ(product(l), 120);
  EXPECT_EQ(product(l, 2), 240);

  DynList<int> empty;
  EXPECT_EQ(sum(empty), 0);
  EXPECT_EQ(product(empty), 1);

  DynList<double> ld = build_dynlist<double>(1.5, 2.5, 3.0);
  EXPECT_DOUBLE_EQ(sum(ld), 7.0);
}

TEST(concat_test, concat_basic)
{
  DynList<int> l1 = build_dynlist<int>(1, 2, 3);
  DynList<int> l2 = build_dynlist<int>(4, 5, 6);

  auto c = concat(l1, l2);
  EXPECT_EQ(c, build_dynlist<int>(1, 2, 3, 4, 5, 6));

  DynList<int> empty;
  EXPECT_EQ(concat(empty, l1), l1);
  EXPECT_EQ(concat(l1, empty), l1);
}

TEST(take_while_test, take_while_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 10, 5, 6);

  auto result = take_while(l, [] (int x) { return x < 5; });
  EXPECT_EQ(result, build_dynlist<int>(1, 2, 3));

  // All elements satisfy
  auto all = take_while(l, [] (int) { return true; });
  EXPECT_EQ(all, l);

  // No elements satisfy
  auto none_result = take_while(l, [] (int x) { return x > 100; });
  EXPECT_TRUE(none_result.is_empty());
}

TEST(drop_while_test, drop_while_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 10, 5, 6);

  auto result = drop_while(l, [] (int x) { return x < 5; });
  EXPECT_EQ(result, build_dynlist<int>(10, 5, 6));

  // All elements satisfy - drop all
  auto all_dropped = drop_while(l, [] (int) { return true; });
  EXPECT_TRUE(all_dropped.is_empty());

  // No elements satisfy - keep all
  auto none_dropped = drop_while(l, [] (int x) { return x > 100; });
  EXPECT_EQ(none_dropped, l);
}

TEST(flat_map_test, flat_map_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3);

  // Duplicate each element
  auto result = flat_map(l, [] (int x) {
    return build_dynlist<int>(x, x);
  });
  EXPECT_EQ(result, build_dynlist<int>(1, 1, 2, 2, 3, 3));

  // Create range for each
  auto ranges = flat_map(l, [] (int x) {
    return range(x);
  });
  EXPECT_EQ(ranges, build_dynlist<int>(0, 0, 1, 0, 1, 2));
}

TEST(scanl_test, scanl_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4);

  auto sums = scanl<int>(l, 0, [] (int a, int b) { return a + b; });
  EXPECT_EQ(sums, build_dynlist<int>(0, 1, 3, 6, 10));

  auto sums2 = scanl_sum(l, 0);
  EXPECT_EQ(sums2, build_dynlist<int>(0, 1, 3, 6, 10));

  DynList<int> empty;
  auto empty_scan = scanl<int>(empty, 100, [] (int a, int b) { return a + b; });
  EXPECT_EQ(empty_scan, build_dynlist<int>(100));
}

TEST(min_max_test, min_max_basic)
{
  DynList<int> l = build_dynlist<int>(5, 2, 8, 1, 9, 3);

  auto * min_p = min_ptr(l);
  ASSERT_NE(min_p, nullptr);
  EXPECT_EQ(*min_p, 1);

  auto * max_p = max_ptr(l);
  ASSERT_NE(max_p, nullptr);
  EXPECT_EQ(*max_p, 9);

  auto [minp, maxp] = minmax_ptr(l);
  ASSERT_NE(minp, nullptr);
  ASSERT_NE(maxp, nullptr);
  EXPECT_EQ(*minp, 1);
  EXPECT_EQ(*maxp, 9);

  // Empty container
  DynList<int> empty;
  EXPECT_EQ(min_ptr(empty), nullptr);
  EXPECT_EQ(max_ptr(empty), nullptr);
  auto [emp_min, emp_max] = minmax_ptr(empty);
  EXPECT_EQ(emp_min, nullptr);
  EXPECT_EQ(emp_max, nullptr);

  // Single element
  DynList<int> single = build_dynlist<int>(42);
  auto [s_min, s_max] = minmax_ptr(single);
  EXPECT_EQ(*s_min, 42);
  EXPECT_EQ(*s_max, 42);

  // Custom comparator
  DynList<int> abs_list = build_dynlist<int>(-10, 5, -20, 3);
  auto * max_abs = max_ptr(abs_list,
                           [] (const int a, const int b) { return std::abs(a) < std::abs(b); });
  EXPECT_EQ(*max_abs, -20);
}

TEST(count_if_test, count_if_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

  EXPECT_EQ(count_if(l, [] (int x) { return x % 2 == 0; }), 5);
  EXPECT_EQ(count_if(l, [] (int x) { return x > 5; }), 5);
  EXPECT_EQ(count_if(l, [] (int x) { return x > 100; }), 0);

  DynList<int> empty;
  EXPECT_EQ(count_if(empty, [] (int) { return true; }), 0);
}

TEST(contains_test, contains_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);

  EXPECT_TRUE(contains(l, 3));
  EXPECT_TRUE(contains(l, 1));
  EXPECT_TRUE(contains(l, 5));
  EXPECT_FALSE(contains(l, 0));
  EXPECT_FALSE(contains(l, 6));

  DynList<int> empty;
  EXPECT_FALSE(contains(empty, 1));
}

TEST(enumerate_tuple_test, enumerate_tuple_basic)
{
  DynList<string> l = build_dynlist<string>("a", "b", "c");

  auto result = enumerate_tuple(l);
  EXPECT_EQ(result.size(), 3);

  auto it = result.get_it();
  EXPECT_EQ(get<0>(it.get_curr()), 0);
  EXPECT_EQ(get<1>(it.get_curr()), "a");
  it.next();
  EXPECT_EQ(get<0>(it.get_curr()), 1);
  EXPECT_EQ(get<1>(it.get_curr()), "b");
  it.next();
  EXPECT_EQ(get<0>(it.get_curr()), 2);
  EXPECT_EQ(get<1>(it.get_curr()), "c");
}

// =============================================================================
// COMPREHENSIVE TESTS FOR ahFunctional.H
// =============================================================================

// --- zip_longest tests ---

TEST(zip_longest_test, basic_different_lengths)
{
  DynList<int> l1 = build_dynlist<int>(1, 2, 3, 4, 5);
  DynList<int> l2 = build_dynlist<int>(10, 20, 30);

  auto result = zip_longest(l1, l2, -1, -1);
  EXPECT_EQ(result.size(), 5);

  auto it = result.get_it();
  EXPECT_EQ(it.get_curr(), make_pair(1, 10)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(2, 20)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(3, 30)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(4, -1)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(5, -1));
}

TEST(zip_longest_test, second_longer)
{
  DynList<int> l1 = build_dynlist<int>(1, 2);
  DynList<int> l2 = build_dynlist<int>(10, 20, 30, 40);

  auto result = zip_longest(l1, l2, 0, 0);
  EXPECT_EQ(result.size(), 4);

  auto it = result.get_it();
  EXPECT_EQ(it.get_curr(), make_pair(1, 10)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(2, 20)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(0, 30)); it.next();
  EXPECT_EQ(it.get_curr(), make_pair(0, 40));
}

TEST(zip_longest_test, equal_lengths)
{
  DynList<int> l1 = build_dynlist<int>(1, 2, 3);
  DynList<int> l2 = build_dynlist<int>(4, 5, 6);

  auto result = zip_longest(l1, l2, -1, -1);
  EXPECT_EQ(result.size(), 3);

  EXPECT_TRUE(result.all([] (auto & p) {
    return p.first + 3 == p.second;
  }));
}

TEST(zip_longest_test, empty_containers)
{
  DynList<int> empty1, empty2;
  DynList<int> l = build_dynlist<int>(1, 2, 3);

  auto r1 = zip_longest(empty1, empty2, 0, 0);
  EXPECT_TRUE(r1.is_empty());

  auto r2 = zip_longest(l, empty2, 0, -1);
  EXPECT_EQ(r2.size(), 3);
  EXPECT_TRUE(r2.all([] (auto & p) { return p.second == -1; }));

  auto r3 = zip_longest(empty1, l, -1, 0);
  EXPECT_EQ(r3.size(), 3);
  EXPECT_TRUE(r3.all([] (auto & p) { return p.first == -1; }));
}

// --- tzip_longest tests ---

TEST(tzip_longest_test, basic_different_lengths)
{
  DynList<int> l1 = build_dynlist<int>(1, 2, 3);
  DynList<string> l2 = build_dynlist<string>("a", "b");

  auto result = tzip_longest(l1, l2, 0, string("X"));
  EXPECT_EQ(result.size(), 3);

  auto it = result.get_it();
  EXPECT_EQ(get<0>(it.get_curr()), 1);
  EXPECT_EQ(get<1>(it.get_curr()), "a");
  it.next();
  EXPECT_EQ(get<0>(it.get_curr()), 2);
  EXPECT_EQ(get<1>(it.get_curr()), "b");
  it.next();
  EXPECT_EQ(get<0>(it.get_curr()), 3);
  EXPECT_EQ(get<1>(it.get_curr()), "X");
}

// --- zip_longest_opt tests ---

TEST(zip_longest_opt_test, basic_different_lengths)
{
  DynList<int> l1 = build_dynlist<int>(1, 2, 3, 4);
  DynList<int> l2 = build_dynlist<int>(10, 20);

  auto result = zip_longest_opt(l1, l2);
  EXPECT_EQ(result.size(), 4);

  auto it = result.get_it();
  auto p1 = it.get_curr();
  EXPECT_TRUE(p1.first.has_value());
  EXPECT_TRUE(p1.second.has_value());
  EXPECT_EQ(p1.first.value(), 1);
  EXPECT_EQ(p1.second.value(), 10);

  it.next(); it.next(); it.next();
  auto p4 = it.get_curr();
  EXPECT_TRUE(p4.first.has_value());
  EXPECT_FALSE(p4.second.has_value());
  EXPECT_EQ(p4.first.value(), 4);
}

// --- group_by tests ---

TEST(group_by_test, basic_grouping)
{
  DynList<int> l = build_dynlist<int>(1, 1, 2, 2, 2, 3, 1, 1);

  // group_by takes a key function that extracts the grouping key
  auto result = group_by(l, [] (int x) { return x; });
  EXPECT_EQ(result.size(), 4);

  auto it = result.get_it();
  // Each element is pair<key, DynList<T>>
  EXPECT_EQ(it.get_curr().first, 1);
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(1, 1));
  it.next();
  EXPECT_EQ(it.get_curr().first, 2);
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(2, 2, 2));
  it.next();
  EXPECT_EQ(it.get_curr().first, 3);
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(3));
  it.next();
  EXPECT_EQ(it.get_curr().first, 1);
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(1, 1));
}

TEST(group_by_test, group_by_parity)
{
  DynList<int> l = build_dynlist<int>(1, 3, 5, 2, 4, 6, 7, 9);

  // Group by parity (consecutive groups with same parity)
  auto result = group_by(l, [] (int x) { return x % 2; });
  EXPECT_EQ(result.size(), 3);

  auto it = result.get_it();
  EXPECT_EQ(it.get_curr().first, 1);  // odd
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(1, 3, 5));
  it.next();
  EXPECT_EQ(it.get_curr().first, 0);  // even
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(2, 4, 6));
  it.next();
  EXPECT_EQ(it.get_curr().first, 1);  // odd again
  EXPECT_EQ(it.get_curr().second, build_dynlist<int>(7, 9));
}

TEST(group_by_test, empty_container)
{
  DynList<int> empty;
  auto result = group_by(empty, [] (int x) { return x; });
  EXPECT_TRUE(result.is_empty());
}

TEST(group_by_test, single_element)
{
  DynList<int> single = build_dynlist<int>(42);
  auto result = group_by(single, [] (int x) { return x; });
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.get_first().first, 42);
  EXPECT_EQ(result.get_first().second, build_dynlist<int>(42));
}

// --- group_by_reduce tests ---

TEST(group_by_reduce_test, sum_groups)
{
  DynList<int> l = build_dynlist<int>(1, 1, 2, 2, 2, 3);

  auto result = group_by_reduce(l,
    [] (int x) { return x; },                        // key function
    [] (const DynList<int> & g) { return sum(g); }   // reducer (takes group)
  );

  // Result should be list of (key, reduced_value) pairs
  EXPECT_EQ(result.size(), 3);

  auto it = result.get_it();
  // First group: key=1, sum=1+1=2
  EXPECT_EQ(it.get_curr().first, 1);
  EXPECT_EQ(it.get_curr().second, 2);
  it.next();
  // Second group: key=2, sum=2+2+2=6
  EXPECT_EQ(it.get_curr().first, 2);
  EXPECT_EQ(it.get_curr().second, 6);
  it.next();
  // Third group: key=3, sum=3
  EXPECT_EQ(it.get_curr().first, 3);
  EXPECT_EQ(it.get_curr().second, 3);
}

// --- maps and maps_if tests ---

TEST(maps_test, basic_map)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);

  auto result = maps<int>(l, [] (int x) { return x * x; });
  EXPECT_EQ(result, build_dynlist<int>(1, 4, 9, 16, 25));
}

TEST(maps_test, type_conversion)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3);

  auto result = maps<string>(l, [] (int x) { return to_string(x); });
  EXPECT_EQ(result, build_dynlist<string>("1", "2", "3"));
}

TEST(filter_map_test, conditional_map)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5, 6);

  // Filter then map: square only even numbers
  auto filtered = filter(l, [] (int x) { return x % 2 == 0; });
  auto result = maps<int>(filtered, [] (int x) { return x * x; });

  EXPECT_EQ(result, build_dynlist<int>(4, 16, 36));
}

TEST(filter_map_test, no_matches)
{
  DynList<int> l = build_dynlist<int>(1, 3, 5, 7);

  auto filtered = filter(l, [] (int x) { return x % 2 == 0; });
  auto result = maps<int>(filtered, [] (int x) { return x * 2; });

  EXPECT_TRUE(result.is_empty());
}

// --- split tests using DynList split method ---

TEST(split_test, basic_split)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5, 6);

  // Use take and drop to simulate split at position 3
  auto left = l.take(3);
  auto right = l.drop(3);

  EXPECT_EQ(left, build_dynlist<int>(1, 2, 3));
  EXPECT_EQ(right, build_dynlist<int>(4, 5, 6));
}

// --- take and drop comprehensive tests ---

TEST(take_drop_test, take_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);

  EXPECT_EQ(l.take(3), build_dynlist<int>(1, 2, 3));
  EXPECT_EQ(l.take(0), DynList<int>());
  EXPECT_EQ(l.take(10), l);
}

TEST(take_drop_test, drop_basic)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5);

  EXPECT_EQ(l.drop(2), build_dynlist<int>(3, 4, 5));
  EXPECT_EQ(l.drop(0), l);
  EXPECT_EQ(l.drop(10), DynList<int>());
}

TEST(take_drop_test, take_drop_range)
{
  DynList<int> l = build_dynlist<int>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

  // Take elements from position 3 to 7
  auto result = l.take(3, 7);
  EXPECT_EQ(result, build_dynlist<int>(3, 4, 5, 6, 7));
}

// --- to_dynlist tests ---

TEST(to_dynlist_test, from_tree)
{
  DynSetTree<int> tree;
  for (int i = 0; i < 5; ++i)
    tree.insert(i);

  auto list = to_dynlist(tree);
  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(list, build_dynlist<int>(0, 1, 2, 3, 4));
}

TEST(to_dynlist_test, from_array)
{
  DynArray<int> arr;
  for (int i = 0; i < 5; ++i)
    arr.append(i * 2);

  auto list = to_dynlist(arr);
  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(list, build_dynlist<int>(0, 2, 4, 6, 8));
}

// --- traverse with filter test ---

TEST(traverse_filter_test, count_evens)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

  size_t count = 0;
  l.traverse([&count] (int x) {
    if (x % 2 == 0) ++count;
    return true;
  });

  EXPECT_EQ(count, 5);
}

// --- Edge cases and stress tests ---

TEST(functional_stress_test, large_container)
{
  constexpr size_t N = 10000;
  DynList<size_t> l = range<size_t>(N);

  // Test foldl on large container
  auto sum = foldl<size_t>(l, 0, [] (size_t a, size_t b) { return a + b; });
  EXPECT_EQ(sum, N * (N - 1) / 2);

  // Test filter on large container
  auto evens = filter(l, [] (size_t x) { return x % 2 == 0; });
  EXPECT_EQ(evens.size(), N / 2);

  // Test maps on large container
  auto doubled = maps<size_t>(l, [] (size_t x) { return x * 2; });
  EXPECT_EQ(doubled.size(), N);
  EXPECT_EQ(doubled.get_first(), 0);
  EXPECT_EQ(doubled.get_last(), (N - 1) * 2);
}

TEST(functional_stress_test, chained_operations)
{
  DynList<int> l = range(100);

  // Chain: filter even -> map square -> take first 5 -> sum
  auto result = filter(l, [] (int x) { return x % 2 == 0; });  // 50 evens
  auto squared = maps<int>(result, [] (int x) { return x * x; });
  auto first5 = squared.take(5);  // 0, 4, 16, 36, 64
  auto total = sum(first5);

  EXPECT_EQ(total, 0 + 4 + 16 + 36 + 64);
}

TEST(functional_edge_test, empty_container_operations)
{
  DynList<int> empty;

  // All these should handle empty containers gracefully
  EXPECT_TRUE(filter(empty, [] (int) { return true; }).is_empty());
  EXPECT_TRUE(maps<int>(empty, [] (int x) { return x; }).is_empty());
  EXPECT_EQ(foldl<int>(empty, 42, [] (int a, int b) { return a + b; }), 42);
  EXPECT_TRUE(all(empty, [] (int) { return false; }));  // vacuously true
  EXPECT_FALSE(exists(empty, [] (int) { return true; }));
  EXPECT_TRUE(none(empty, [] (int) { return true; }));
  EXPECT_EQ(sum(empty), 0);
  EXPECT_EQ(product(empty), 1);
  EXPECT_TRUE(reverse(empty).is_empty());
}

TEST(functional_edge_test, single_element)
{
  DynList<int> single = build_dynlist<int>(42);

  EXPECT_EQ(filter(single, [] (int x) { return x > 0; }).size(), 1);
  EXPECT_EQ(maps<int>(single, [] (int x) { return x * 2; }).get_first(), 84);
  EXPECT_EQ(sum(single), 42);
  EXPECT_EQ(product(single), 42);
  EXPECT_EQ(reverse(single).get_first(), 42);
  EXPECT_EQ(*min_ptr(single), 42);
  EXPECT_EQ(*max_ptr(single), 42);
}

// --- Complex type tests ---

struct Person
{
  string name;
  int age;

  bool operator==(const Person & other) const
  {
    return name == other.name && age == other.age;
  }
};

TEST(functional_complex_type_test, struct_operations)
{
  DynList<Person> people;
  people.append({"Alice", 30});
  people.append({"Bob", 25});
  people.append({"Charlie", 35});
  people.append({"Diana", 28});

  // Filter by age
  auto over28 = filter(people, [] (const Person & p) { return p.age > 28; });
  EXPECT_EQ(over28.size(), 2);

  // Map to names
  auto names = maps<string>(people, [] (const Person & p) { return p.name; });
  EXPECT_EQ(names, build_dynlist<string>("Alice", "Bob", "Charlie", "Diana"));

  // Find oldest
  auto * oldest = max_ptr(people, [] (const Person & a, const Person & b) {
    return a.age < b.age;
  });
  ASSERT_NE(oldest, nullptr);
  EXPECT_EQ(oldest->name, "Charlie");

  // Sum of ages
  auto total_age = foldl<int>(people, 0, [] (int acc, const Person & p) {
    return acc + p.age;
  });
  EXPECT_EQ(total_age, 30 + 25 + 35 + 28);
}

// --- Lazy evaluation / composition tests ---

TEST(functional_composition_test, nested_maps)
{
  DynList<int> l = build_dynlist<int>(1, 2, 3);

  // map twice
  auto result = maps<int>(maps<int>(l, [] (int x) { return x + 1; }),
                          [] (int x) { return x * x; });
  EXPECT_EQ(result, build_dynlist<int>(4, 9, 16));  // (1+1)^2, (2+1)^2, (3+1)^2
}

TEST(functional_composition_test, filter_then_map)
{
  DynList<int> l = range(10);

  auto result = maps<string>(
    filter(l, [] (int x) { return x % 3 == 0; }),
    [] (int x) { return "val" + to_string(x); }
  );

  EXPECT_EQ(result, build_dynlist<string>("val0", "val3", "val6", "val9"));
}

// --- Quantifier tests ---

TEST(quantifier_test, all_comprehensive)
{
  DynList<int> l = build_dynlist<int>(2, 4, 6, 8, 10);

  EXPECT_TRUE(all(l, [] (int x) { return x % 2 == 0; }));  // all even
  EXPECT_TRUE(all(l, [] (int x) { return x > 0; }));       // all positive
  EXPECT_FALSE(all(l, [] (int x) { return x > 5; }));      // not all > 5

  // Empty container - vacuously true
  DynList<int> empty;
  EXPECT_TRUE(all(empty, [] (int) { return false; }));
}

TEST(quantifier_test, exists_comprehensive)
{
  DynList<int> l = build_dynlist<int>(1, 3, 5, 6, 7);

  EXPECT_TRUE(exists(l, [] (int x) { return x % 2 == 0; }));   // 6 is even
  EXPECT_TRUE(exists(l, [] (int x) { return x == 7; }));       // 7 exists
  EXPECT_FALSE(exists(l, [] (int x) { return x > 100; }));     // none > 100

  // Empty container - false
  DynList<int> empty;
  EXPECT_FALSE(exists(empty, [] (int) { return true; }));
}

TEST(quantifier_test, none_comprehensive)
{
  DynList<int> l = build_dynlist<int>(1, 3, 5, 7, 9);

  EXPECT_TRUE(none(l, [] (int x) { return x % 2 == 0; }));   // no even
  EXPECT_TRUE(none(l, [] (int x) { return x > 100; }));      // none > 100
  EXPECT_FALSE(none(l, [] (int x) { return x == 5; }));      // 5 exists

  // Empty container - vacuously true
  DynList<int> empty;
  EXPECT_TRUE(none(empty, [] (int) { return true; }));
}

