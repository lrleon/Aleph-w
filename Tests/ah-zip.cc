
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
# include <tpl_dynArray.H>

using namespace std;

struct EmptyGroup : public testing::Test
{
  DynList<int> l1;
  DynSetTree<int> l2;
  DynArray<string> l3;
};

struct CompleteGroup : public testing::Test
{
  static constexpr size_t N = 5;
  DynList<int> l1 = range<int>(0, N - 1);
  DynSetTree<int> l2 = range<int>(0, N - 1);
  DynArray<string> l3 =
    range<int>(0, N - 1).maps<string>([] (auto i) { return to_string(i); });
};

struct InCompleteGroup : public testing::Test
{
  DynList<int> l1 = { 0, 1, 2, 3, 4 };
  DynSetTree<int> l2 = { 0, 1, 2, 3 };
  DynArray<string> l3 = { "0", "1", "2", "3", "4" };
};

TEST_F(EmptyGroup, empty)
{
  {
    auto it = zip_it(l1, l2 , l3);
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), std::overflow_error);
    ASSERT_THROW(it.next(), std::overflow_error);
  }
  {
    auto it = zip_it(l1);
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), std::overflow_error);
    ASSERT_THROW(it.next(), std::overflow_error);
  }

  ASSERT_NO_THROW(zip_it_pos(0, l1, l2, l3));
  ASSERT_THROW(zip_it_pos(1, l1, l2, l3), std::overflow_error);

  {
    auto it = enum_zip_it(l1, l2 , l3);
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), std::overflow_error);
    ASSERT_THROW(it.next(), std::overflow_error);
  }
  {
    auto it = enum_zip_it(l1);
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), std::overflow_error);
    ASSERT_THROW(it.next(), std::overflow_error);
  }

  ASSERT_NO_THROW(enum_zip_it_pos(0, l1, l2, l3));
  ASSERT_THROW(enum_zip_it_pos(1, l1, l2, l3), std::overflow_error);
}

TEST_F(CompleteGroup, Iterators)
{
  size_t i = 0;
  for (auto it = zip_it(l1, l2, l3); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr_ne();
      EXPECT_EQ(get<0>(t), i);
      EXPECT_EQ(get<1>(t), i);
      EXPECT_EQ(get<2>(t), to_string(i));
    }

  i = 1;
  for (auto it = zip_it_pos(1, l1, l2, l3); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr_ne();
      EXPECT_EQ(get<0>(t), i);
      EXPECT_EQ(get<1>(t), i);
      EXPECT_EQ(get<2>(t), to_string(i));
    }

  for (auto it = enum_zip_it(l1, l2, l3); it.has_curr(); it.next_ne())
    {
      auto t = it.get_curr_ne();
      auto & i = get<3>(t);
      EXPECT_EQ(get<0>(t), i);
      EXPECT_EQ(get<1>(t), i);
      EXPECT_EQ(get<2>(t), to_string(i));
    }
}

TEST_F(InCompleteGroup, ml_operations)
{
  EXPECT_FALSE(equal_length(l1, l2, l3));
  EXPECT_TRUE(zip_traverse([] (auto t) { return get<0>(t) == get<1>(t)
	  and to_string(get<0>(t)) == get<2>(t); }, l1, l2, l3));
  EXPECT_FALSE(zip_traverse_eq([] (auto t) { return get<0>(t) == get<1>(t)
	  and to_string(get<0>(t)) == get<2>(t); }, l1, l2, l3));
  EXPECT_NO_THROW(zip_for_each([] (auto) { return; }, l1, l2, l3));
  EXPECT_THROW(zip_for_each_eq([] (auto) { return; }, l1, l2, l3),
	       length_error);

  size_t i = 0;
  zip_for_each([&i] (auto t)
	       {
		 EXPECT_EQ(get<0>(t), i++);
		 EXPECT_EQ(get<0>(t), get<1>(t));
		 EXPECT_EQ(to_string(get<0>(t)), get<2>(t));
	       }, l1, l2, l3);

  i = 0;
  EXPECT_THROW(zip_for_each_eq([&i] (auto t)
	       {
		 EXPECT_EQ(get<0>(t), i++);
		 EXPECT_EQ(get<0>(t), get<1>(t));
		 EXPECT_EQ(to_string(get<0>(t)), get<2>(t));
	       }, l1, l2, l3), length_error);

   EXPECT_FALSE(zip_all([] (auto t) { return get<0>(t) == get<1>(t)
	   and to_string(get<0>(t)) == get<2>(t); }, l1, l2, l3));
   EXPECT_TRUE(zip_exists([] (auto t) { return get<0>(t) == get<1>(t) and
	  get<2>(t) == "3"; }, l1, l2, l3));

   auto lmap = zip_maps<string>([] (auto t)
    {
      return to_string(get<0>(t)) + 
      to_string(get<1>(t)) + get<2>(t);
    }, l1, l2, l3);
  EXPECT_EQ(lmap, build_dynlist<string>("000", "111", "222", "333"));

  lmap = zip_maps_if<string>([] (auto t) { return get<0>(t) != 1; },
			     [] (auto t)
			     {
			       return to_string(get<0>(t)) + 
			       to_string(get<1>(t)) + get<2>(t);
			     }, l1, l2, l3);
  EXPECT_EQ(lmap, build_dynlist<string>("000", "222", "333"));

  EXPECT_THROW(t_zip_eq(l1, l2, l3), length_error);
}

TEST_F(CompleteGroup, ml_operations)
{
  EXPECT_TRUE(equal_length(l1, l2, l3));
  EXPECT_TRUE(zip_traverse([] (auto t) { return get<0>(t) == get<1>(t)
	  and to_string(get<0>(t)) == get<2>(t); }, l1, l2, l3));
  EXPECT_NO_THROW(zip_traverse_eq([] (auto t) { return get<0>(t) == get<1>(t)
	  and to_string(get<0>(t)) == get<2>(t); }, l1, l2, l3));
  zip_for_each([] (auto t)
	       {
		 EXPECT_EQ(get<0>(t), get<1>(t));
		 EXPECT_EQ(to_string(get<0>(t)), get<2>(t));
	       }, l1, l2, l3);
  zip_for_each_eq([] (auto t)
		  {
		    EXPECT_EQ(get<0>(t), get<1>(t));
		    EXPECT_EQ(to_string(get<0>(t)), get<2>(t));
		  }, l1, l2, l3);
  EXPECT_TRUE(zip_all([] (auto t) { return get<0>(t) == get<1>(t)
	  and to_string(get<0>(t)) == get<2>(t); }, l1, l2, l3));
  EXPECT_TRUE(zip_exists([] (auto t) { return get<0>(t) == get<1>(t) and
	  get<2>(t) == "3"; }, l1, l2, l3));
  EXPECT_FALSE(zip_exists([] (auto t) { return get<0>(t) == get<1>(t) and
	  get<2>(t) == "7"; }, l1, l2, l3));

  auto lmap = zip_maps<string>([] (auto t)
    {
      return to_string(get<0>(t)) + 
      to_string(get<1>(t)) + get<2>(t);
    }, l1, l2, l3);
  EXPECT_EQ(lmap, build_dynlist<string>("000", "111", "222", "333", "444"));

  lmap = zip_maps_if<string>([] (auto t) { return get<0>(t) != 1; },
			     [] (auto t)
			     {
			       return to_string(get<0>(t)) + 
			       to_string(get<1>(t)) + get<2>(t);
			     }, l1, l2, l3);
  EXPECT_EQ(lmap, build_dynlist<string>("000", "222", "333", "444"));

  auto sum = zip_foldl(0, [] (auto a, auto t)
		       { return a + get<0>(t) + get<1>(t); }, l1, l2, l3);
  EXPECT_EQ(sum, N*(N- 1));

  auto lfilt = zip_filter([] (auto t) { return get<1>(t) != 2; }, l1, l2, l3);
  DynList<tuple<int, int, string>> expl =
    { make_tuple(0, 0, "0"), make_tuple(1, 1, "1"),
      make_tuple(3, 3, "3"), make_tuple(4, 4, "4") };
  EXPECT_EQ(lfilt, expl);

  EXPECT_TRUE(zip_cmp([] (auto & i1, auto & i2) { return i1 == i2; },
		      l1, l2, l1, l2));

  auto l1_mutated = l1; l1_mutated.nth(3) = 4;
  EXPECT_FALSE(zip_cmp([] (auto & i1, auto & i2) { return i1 == i2; },
		       l1, l2, l1_mutated, l2));

  auto lzip = t_zip(l1, l2, l3);
  for (auto it = zip_it(lzip, l1, l2, l3); it.has_curr(); it.next_ne())
    {
      auto t = it.get_curr_ne();
      auto & tz = get<0>(t);
      EXPECT_EQ(get<0>(tz), get<1>(t));
    }

  auto uzip = t_unzip(lzip);
  EXPECT_EQ(get<0>(uzip), l1);
  EXPECT_EQ(get<1>(uzip), l2.keys());
  EXPECT_TRUE(eq(get<2>(uzip), l3));

  auto idx = zip_find_index([] (auto t)
			    {
			      return get<0>(t) == get<1>(t) and
			        to_string(get<1>(t)) == get<2>(t) and
			        get<0>(t) == N - 1;
			    }, l1, l2, l3);
  EXPECT_EQ(idx, N - 1);

  idx = zip_find_index([] (auto t)
			    {
			      return get<0>(t) == N;
			    }, l1, l2, l3);
  EXPECT_EQ(idx, N - 0 /* for avoiding linker error with constant */);

  auto part = zip_partition([] (auto t) { return get<0>(t) < 2; }, l1, l2, l3);

  EXPECT_TRUE(eq(get<0>(part).maps<string>([] (auto & t)
    {
      return to_string(get<0>(t)) + to_string(get<1>(t)) + get<2>(t);
    }), build_dynlist<string>("000", "111")));
  EXPECT_EQ(get<1>(part), 2);
  EXPECT_TRUE(eq(get<2>(part).maps<string>([] (auto & t)
    {
      return to_string(get<0>(t)) + to_string(get<1>(t)) + get<2>(t);
    }), build_dynlist<string>("222", "333", "444")));
  EXPECT_EQ(get<3>(part), N - 2);

  auto l = zip_lists(l1, l2, l1, l2);
  for (auto it = zip_it(l, l1, l2); it.has_curr(); it.next_ne())
    {
      auto c = it.get_curr_ne();
      EXPECT_EQ(get<0>(c), build_dynlist<int>(get<1>(c), get<2>(c),
					      get<1>(c), get<2>(c)));
    }
}
