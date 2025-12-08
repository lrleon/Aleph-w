
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


