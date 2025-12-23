
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
  EXPECT_EQ(nrange(2, 5, 3), build_dynlist<int>(2, 3, 4));
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
}

