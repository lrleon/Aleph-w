
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
  auto l = gen_seq_list_tuples(tbl, 8);
}
