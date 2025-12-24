
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
                           [] (int a, int b) { return std::abs(a) < std::abs(b); });
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

