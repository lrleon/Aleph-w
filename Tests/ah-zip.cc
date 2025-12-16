
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
# include <tpl_dynArray.H>
# include <tpl_array.H>
# include <tpl_dynSetHash.H>

using namespace std;
using namespace Aleph;

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
  EXPECT_NO_THROW((void)zip_traverse_eq([] (auto t) { return get<0>(t) == get<1>(t)
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

  auto lfilt = zip_filter([] (auto t)
    { return get<1>(t) != 2; }, l1, l2, l3);
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

// ==================== NEW TESTS ====================

// Test zip_lists_eq with equal length containers
TEST_F(CompleteGroup, zip_lists_eq_success)
{
  auto result = zip_lists_eq(l1, l2, l1);

  EXPECT_EQ(result.size(), N);

  size_t i = 0;
  for (auto it = result.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto & inner_list = it.get_curr();
      EXPECT_EQ(inner_list.size(), 3);
      auto inner_it = inner_list.get_it();
      EXPECT_EQ(inner_it.get_curr(), i);  // from l1
      inner_it.next();
      EXPECT_EQ(inner_it.get_curr(), i);  // from l2
      inner_it.next();
      EXPECT_EQ(inner_it.get_curr(), i);  // from l1 again
    }
}

// Test zip_lists_eq with unequal length containers - should throw
TEST_F(InCompleteGroup, zip_lists_eq_throws)
{
  DynList<int> l_short = { 0, 1, 2 };
  DynList<int> l_long = { 0, 1, 2, 3, 4 };

  EXPECT_THROW(zip_lists_eq(l_short, l_long), length_error);
}

// Test t_enum_zip
TEST_F(CompleteGroup, t_enum_zip_basic)
{
  auto result = t_enum_zip(l1, l2);

  EXPECT_EQ(result.size(), N);

  for (auto it = result.get_it(); it.has_curr(); it.next_ne())
    {
      auto t = it.get_curr();
      auto idx = get<2>(t);  // Index is last element
      EXPECT_EQ(get<0>(t), idx);  // l1 value should match index
      EXPECT_EQ(get<1>(t), idx);  // l2 value should match index
    }
}

// Test t_enum_zip_eq with equal containers
TEST_F(CompleteGroup, t_enum_zip_eq_success)
{
  auto result = t_enum_zip_eq(l1, l2);

  EXPECT_EQ(result.size(), N);

  size_t expected_idx = 0;
  for (auto it = result.get_it(); it.has_curr(); it.next_ne(), ++expected_idx)
    {
      auto t = it.get_curr();
      EXPECT_EQ(get<2>(t), expected_idx);
    }
}

// Test t_enum_zip_eq with unequal containers - should throw
TEST_F(InCompleteGroup, t_enum_zip_eq_throws)
{
  EXPECT_THROW(t_enum_zip_eq(l1, l2), length_error);
}

// Test std_zip with STL vectors
TEST(StdZip, basic_vectors)
{
  vector<int> v1 = { 1, 2, 3, 4, 5 };
  vector<string> v2 = { "a", "b", "c", "d", "e" };

  auto result = std_zip(v1, v2);

  EXPECT_EQ(result.size(), 5);

  for (size_t i = 0; i < result.size(); ++i)
    {
      EXPECT_EQ(result[i].first, static_cast<int>(i + 1));
      EXPECT_EQ(result[i].second, string(1, 'a' + i));
    }
}

// Test std_zip with unequal length - should stop at shorter
TEST(StdZip, unequal_lengths)
{
  vector<int> v1 = { 1, 2, 3 };
  vector<int> v2 = { 10, 20, 30, 40, 50 };

  auto result = std_zip(v1, v2);

  EXPECT_EQ(result.size(), 3);  // Stops at shorter container
  EXPECT_EQ(result[0].first, 1);
  EXPECT_EQ(result[0].second, 10);
  EXPECT_EQ(result[2].first, 3);
  EXPECT_EQ(result[2].second, 30);
}

// Test tzip_std with 2 STL containers (variadic version)
TEST(TzipStd, two_containers)
{
  vector<int> v1 = { 1, 2, 3 };
  vector<string> v2 = { "a", "b", "c" };

  auto result = tzip_std(v1, v2);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(get<0>(result[0]), 1);
  EXPECT_EQ(get<1>(result[0]), "a");
  EXPECT_EQ(get<0>(result[2]), 3);
  EXPECT_EQ(get<1>(result[2]), "c");
}

// Test tzip_std with 3 STL containers
TEST(TzipStd, three_containers)
{
  vector<int> v1 = { 1, 2, 3 };
  vector<double> v2 = { 1.1, 2.2, 3.3 };
  vector<char> v3 = { 'a', 'b', 'c' };

  auto result = tzip_std(v1, v2, v3);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(get<0>(result[0]), 1);
  EXPECT_DOUBLE_EQ(get<1>(result[0]), 1.1);
  EXPECT_EQ(get<2>(result[0]), 'a');
}

// Test tzip_std with 4 STL containers
TEST(TzipStd, four_containers)
{
  vector<int> v1 = { 1, 2, 3 };
  vector<double> v2 = { 1.1, 2.2, 3.3 };
  vector<char> v3 = { 'a', 'b', 'c' };
  vector<string> v4 = { "x", "y", "z" };

  auto result = tzip_std(v1, v2, v3, v4);

  EXPECT_EQ(result.size(), 3);

  EXPECT_EQ(get<0>(result[0]), 1);
  EXPECT_DOUBLE_EQ(get<1>(result[0]), 1.1);
  EXPECT_EQ(get<2>(result[0]), 'a');
  EXPECT_EQ(get<3>(result[0]), "x");

  EXPECT_EQ(get<0>(result[2]), 3);
  EXPECT_DOUBLE_EQ(get<1>(result[2]), 3.3);
  EXPECT_EQ(get<2>(result[2]), 'c');
  EXPECT_EQ(get<3>(result[2]), "z");
}

// Test tzip_std with 5 STL containers
TEST(TzipStd, five_containers)
{
  vector<int> v1 = { 1, 2 };
  vector<double> v2 = { 1.1, 2.2 };
  vector<char> v3 = { 'a', 'b' };
  vector<string> v4 = { "x", "y" };
  vector<long> v5 = { 100L, 200L };

  auto result = tzip_std(v1, v2, v3, v4, v5);

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(get<0>(result[0]), 1);
  EXPECT_DOUBLE_EQ(get<1>(result[0]), 1.1);
  EXPECT_EQ(get<2>(result[0]), 'a');
  EXPECT_EQ(get<3>(result[0]), "x");
  EXPECT_EQ(get<4>(result[0]), 100L);
}

TEST(StdZipN, alias_matches_tzip_std)
{
  vector<int> v1 = { 1, 2, 3 };
  vector<double> v2 = { 1.1, 2.2, 3.3 };
  vector<string> v3 = { "a", "b", "c" };

  auto a = tzip_std(v1, v2, v3);
  auto b = std_zip_n(v1, v2, v3);
  EXPECT_EQ(a, b);
}

TEST(StdZipN, stops_at_shortest)
{
  vector<int> v1 = { 1, 2 };
  vector<int> v2 = { 10, 20, 30 };
  vector<int> v3 = { 100, 200, 300, 400 };

  auto result = std_zip_n(v1, v2, v3);
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(get<0>(result[0]), 1);
  EXPECT_EQ(get<1>(result[0]), 10);
  EXPECT_EQ(get<2>(result[0]), 100);
}

// Test tzip_std with unequal lengths
TEST(TzipStd, unequal_lengths)
{
  vector<int> v1 = { 1, 2 };
  vector<int> v2 = { 10, 20, 30 };
  vector<int> v3 = { 100, 200, 300, 400 };
  vector<int> v4 = { 1000, 2000, 3000, 4000, 5000 };

  auto result = tzip_std(v1, v2, v3, v4);

  EXPECT_EQ(result.size(), 2);  // Stops at shortest
}

// Test single container zip
TEST(SingleContainerZip, basic)
{
  DynList<int> l = { 1, 2, 3, 4, 5 };

  auto it = zip_it(l);
  size_t count = 0;
  for (; it.has_curr(); it.next_ne(), ++count)
    {
      auto t = it.get_curr_ne();
      EXPECT_EQ(get<0>(t), static_cast<int>(count + 1));
    }
  EXPECT_EQ(count, 5);
}

// Test equal_length with single container
TEST(SingleContainerZip, equal_length_single)
{
  DynList<int> l = { 1, 2, 3 };
  EXPECT_TRUE(equal_length(l));
}

// Test get_curr_list with homogeneous containers - verify correct order
TEST_F(CompleteGroup, get_curr_list_homogeneous)
{
  DynList<int> a = { 1, 2, 3 };
  DynList<int> b = { 10, 20, 30 };
  DynList<int> c = { 100, 200, 300 };

  auto it = zip_it(a, b, c);

  ASSERT_TRUE(it.has_curr());
  auto curr_list = it.get_curr_list();

  // Should have 3 elements in order: 1 (from a), 10 (from b), 100 (from c)
  EXPECT_EQ(curr_list.size(), 3);
  auto list_it = curr_list.get_it();
  EXPECT_EQ(list_it.get_curr(), 1);    // First element from first container
  list_it.next();
  EXPECT_EQ(list_it.get_curr(), 10);   // Second element from second container
  list_it.next();
  EXPECT_EQ(list_it.get_curr(), 100);  // Third element from third container

  // Advance and check second position
  it.next();
  curr_list = it.get_curr_list();
  list_it = curr_list.get_it();
  EXPECT_EQ(list_it.get_curr(), 2);
  list_it.next();
  EXPECT_EQ(list_it.get_curr(), 20);
  list_it.next();
  EXPECT_EQ(list_it.get_curr(), 200);
}

// Test zip_cmp with different predicates
TEST_F(CompleteGroup, zip_cmp_less_than)
{
  DynList<int> ascending1 = { 1, 2, 3, 4, 5 };
  DynList<int> ascending2 = { 2, 3, 4, 5, 6 };

  // All elements in ascending1 should be less than corresponding elements in ascending2
  EXPECT_TRUE(zip_cmp([] (auto & a, auto & b) { return a < b; },
                      ascending1, ascending2));
}

// Test with empty containers
TEST(EmptyContainerTests, all_empty)
{
  DynList<int> empty1, empty2, empty3;

  EXPECT_TRUE(equal_length(empty1, empty2, empty3));
  EXPECT_TRUE(zip_traverse([] (auto) { return true; }, empty1, empty2));
  EXPECT_TRUE(zip_all([] (auto) { return true; }, empty1, empty2));
  EXPECT_FALSE(zip_exists([] (auto) { return true; }, empty1, empty2));

  auto mapped = zip_maps<int>([] (auto t) { return get<0>(t); }, empty1, empty2);
  EXPECT_TRUE(mapped.is_empty());

  auto filtered = zip_filter([] (auto) { return true; }, empty1, empty2);
  EXPECT_TRUE(filtered.is_empty());

  auto zipped = t_zip(empty1, empty2);
  EXPECT_TRUE(zipped.is_empty());
}

// Test noexcept guarantees for _ne methods
TEST_F(CompleteGroup, noexcept_methods)
{
  auto it = zip_it(l1, l2);

  // These should be noexcept
  EXPECT_TRUE(noexcept(it.get_curr_ne()));
  EXPECT_TRUE(noexcept(it.next_ne()));

  auto enum_it = enum_zip_it(l1, l2);
  EXPECT_TRUE(noexcept(enum_it.get_curr_ne()));
  EXPECT_TRUE(noexcept(enum_it.next_ne()));
}

// Stress test with larger containers
TEST(StressTest, large_containers)
{
  const size_t SIZE = 1000;
  DynList<int> l1 = range<int>(0, SIZE - 1);
  DynList<int> l2 = range<int>(0, SIZE - 1);

  EXPECT_TRUE(equal_length(l1, l2));

  auto sum = zip_foldl(0, [] (auto acc, auto t)
                       { return acc + get<0>(t) + get<1>(t); }, l1, l2);

  // Sum of 0..999 is 499500, and we have two lists
  EXPECT_EQ(sum, 2 * (SIZE * (SIZE - 1) / 2));

  auto zipped = t_zip(l1, l2);
  EXPECT_EQ(zipped.size(), SIZE);
}

// Test zip_find_index edge cases
TEST_F(CompleteGroup, zip_find_index_not_found)
{
  // Should return N (size of containers) if not found
  auto idx = zip_find_index([] (auto t) { return get<0>(t) == 999; }, l1, l2);
  EXPECT_EQ(idx, N);
}

TEST_F(CompleteGroup, zip_find_index_first_element)
{
  auto idx = zip_find_index([] (auto t) { return get<0>(t) == 0; }, l1, l2);
  EXPECT_EQ(idx, 0);
}

// Test completed() behavior
TEST_F(CompleteGroup, completed_behavior)
{
  auto it = zip_it(l1, l2, l3);

  // Iterate to the end
  while (it.has_curr())
    it.next_ne();

  // All containers same length, so completed() should be true
  EXPECT_TRUE(it.completed());
}

TEST_F(InCompleteGroup, completed_behavior_incomplete)
{
  auto it = zip_it(l1, l2, l3);

  // Iterate to the end (stops at shortest)
  while (it.has_curr())
    it.next_ne();

  // l2 is shorter, so not all are completed
  EXPECT_FALSE(it.completed());
}

// Test zip with 5 different container types and heterogeneous element types
TEST(HeterogeneousContainers, five_different_containers)
{
  // 5 distinct container types with different element types
  DynList<int> c1 = { 1, 2, 3 };
  DynSetTree<double> c2 = { 1.1, 2.2, 3.3 };
  DynArray<string> c3 = { "a", "b", "c" };
  Array<char> c4 = { 'x', 'y', 'z' };  // Use initializer list
  DynList<long> c5 = { 100L, 200L, 300L };

  // Iterate over all 5 containers simultaneously
  size_t count = 0;
  for (auto it = zip_it(c1, c2, c3, c4, c5); it.has_curr(); it.next_ne(), ++count)
    {
      auto t = it.get_curr_ne();
      // t is tuple<int, double, string, char, long>
      EXPECT_EQ(get<0>(t), static_cast<int>(count + 1));
      EXPECT_DOUBLE_EQ(get<1>(t), (count + 1) * 1.1);
      EXPECT_EQ(get<2>(t), string(1, 'a' + count));
      EXPECT_EQ(get<3>(t), 'x' + count);
      EXPECT_EQ(get<4>(t), (count + 1) * 100L);
    }
  EXPECT_EQ(count, 3);

  // Verify equal_length works with 5 containers
  EXPECT_TRUE(equal_length(c1, c2, c3, c4, c5));

  // Test zip_for_each with 5 containers
  count = 0;
  zip_for_each([&count](auto t) {
    ++count;
    // Verify tuple has correct types
    static_assert(std::is_same_v<std::decay_t<decltype(get<0>(t))>, int>);
    static_assert(std::is_same_v<std::decay_t<decltype(get<1>(t))>, double>);
    static_assert(std::is_same_v<std::decay_t<decltype(get<2>(t))>, string>);
    static_assert(std::is_same_v<std::decay_t<decltype(get<3>(t))>, char>);
    static_assert(std::is_same_v<std::decay_t<decltype(get<4>(t))>, long>);
  }, c1, c2, c3, c4, c5);
  EXPECT_EQ(count, 3);
}

// Test with Array container specifically
TEST(ArrayContainer, basic_zip_with_array)
{
  Array<int> arr1 = { 10, 20, 30, 40 };  // Use initializer list
  Array<string> arr2 = { "ten", "twenty", "thirty", "forty" };
  DynList<double> list = { 1.0, 2.0, 3.0, 4.0 };

  size_t count = 0;
  for (auto it = zip_it(arr1, arr2, list); it.has_curr(); it.next_ne(), ++count)
    {
      auto t = it.get_curr_ne();
      EXPECT_EQ(get<0>(t), (count + 1) * 10);
      EXPECT_EQ(get<2>(t), count + 1.0);
    }
  EXPECT_EQ(count, 4);

  EXPECT_TRUE(equal_length(arr1, arr2, list));
}

// Test with hash table containers
TEST(HashContainers, zip_with_dynset_hash)
{
  // DynSetHash - hash set with linear probing
  DynSetLhash<int> hash_set = { 10, 20, 30, 40, 50 };
  DynList<int> list = { 1, 2, 3, 4, 5 };

  // Both have 5 elements
  EXPECT_EQ(hash_set.size(), 5);
  EXPECT_EQ(list.size(), 5);
  EXPECT_TRUE(equal_length(hash_set, list));

  // Iterate and count (hash order is not deterministic, so just check count)
  size_t count = 0;
  for (auto it = zip_it(hash_set, list); it.has_curr(); it.next_ne(), ++count)
    {
      auto t = it.get_curr_ne();
      // get<0>(t) is from hash_set (some int in {10,20,30,40,50})
      // get<1>(t) is from list (sequential 1,2,3,4,5)
      EXPECT_TRUE(hash_set.contains(get<0>(t)));
    }
  EXPECT_EQ(count, 5);

  // Test with multiple hash containers
  DynSetLhash<string> hash_strings = { "a", "b", "c" };
  DynSetLhash<double> hash_doubles = { 1.1, 2.2, 3.3 };
  DynList<int> indices = { 0, 1, 2 };

  EXPECT_TRUE(equal_length(hash_strings, hash_doubles, indices));

  count = 0;
  zip_for_each([&count](auto t) {
    ++count;
    // Verify types are correct
    static_assert(std::is_same_v<std::decay_t<decltype(get<0>(t))>, string>);
    static_assert(std::is_same_v<std::decay_t<decltype(get<1>(t))>, double>);
    static_assert(std::is_same_v<std::decay_t<decltype(get<2>(t))>, int>);
  }, hash_strings, hash_doubles, indices);
  EXPECT_EQ(count, 3);
}

// Test mixing hash containers with tree and list containers
TEST(HashContainers, mixed_hash_tree_list)
{
  DynSetLhash<int> hash = { 1, 2, 3, 4 };
  DynSetTree<int> tree = { 10, 20, 30, 40 };
  DynList<string> list = { "a", "b", "c", "d" };
  Array<double> arr = { 1.1, 2.2, 3.3, 4.4 };

  EXPECT_TRUE(equal_length(hash, tree, list, arr));

  size_t count = 0;
  for (auto it = zip_it(hash, tree, list, arr); it.has_curr(); it.next_ne(), ++count)
    {
      auto t = it.get_curr_ne();
      // Just verify we can access all elements
      EXPECT_TRUE(hash.contains(get<0>(t)));
      EXPECT_TRUE(tree.contains(get<1>(t)));
    }
  EXPECT_EQ(count, 4);
}

// ============================================================================
// Tests for new features (issues 16, 17, 19, 20)
// ============================================================================

// ---------- Issue 16: zip_transform ----------

TEST_F(CompleteGroup, zip_transform_basic)
{
  // Transform tuples into concatenated strings
  auto result = zip_transform([](auto t) {
    return to_string(get<0>(t)) + "-" + to_string(get<1>(t)) + "-" + get<2>(t);
  }, l1, l2, l3);

  EXPECT_EQ(result.size(), N);
  EXPECT_EQ(result[0], "0-0-0");
  EXPECT_EQ(result[1], "1-1-1");
  EXPECT_EQ(result[N-1], to_string(N-1) + "-" + to_string(N-1) + "-" + to_string(N-1));
}

TEST_F(CompleteGroup, zip_transform_numeric)
{
  // Transform to sum of elements
  auto result = zip_transform([](auto t) {
    return get<0>(t) + get<1>(t);
  }, l1, l2);

  EXPECT_EQ(result.size(), N);
  for (size_t i = 0; i < N; ++i)
    EXPECT_EQ(result[i], static_cast<int>(2 * i));
}

TEST_F(CompleteGroup, zip_transform_eq_success)
{
  auto result = zip_transform_eq([](auto t) {
    return get<0>(t) * get<1>(t);
  }, l1, l2);

  EXPECT_EQ(result.size(), N);
  for (size_t i = 0; i < N; ++i)
    EXPECT_EQ(result[i], static_cast<int>(i * i));
}

TEST_F(InCompleteGroup, zip_transform_eq_throws)
{
  EXPECT_THROW(
    zip_transform_eq([](auto t) { return get<0>(t); }, l1, l2),
    length_error
  );
}

TEST(ZipTransform, empty_containers)
{
  DynList<int> empty1, empty2;
  auto result = zip_transform([](auto t) { return get<0>(t); }, empty1, empty2);
  EXPECT_TRUE(result.empty());
}

// ---------- Issue 17: zip_for_each_indexed ----------

TEST_F(CompleteGroup, zip_for_each_indexed_basic)
{
  vector<size_t> indices;
  vector<int> values;

  zip_for_each_indexed([&](size_t idx, auto t) {
    indices.push_back(idx);
    values.push_back(get<0>(t));
  }, l1, l2);

  EXPECT_EQ(indices.size(), N);
  EXPECT_EQ(values.size(), N);

  for (size_t i = 0; i < N; ++i)
    {
      EXPECT_EQ(indices[i], i);
      EXPECT_EQ(values[i], static_cast<int>(i));
    }
}

TEST_F(CompleteGroup, zip_for_each_indexed_verify_tuple)
{
  zip_for_each_indexed([](size_t idx, auto t) {
    EXPECT_EQ(get<0>(t), static_cast<int>(idx));
    EXPECT_EQ(get<1>(t), static_cast<int>(idx));
    EXPECT_EQ(get<2>(t), to_string(idx));
  }, l1, l2, l3);
}

TEST_F(CompleteGroup, zip_for_each_indexed_eq_success)
{
  size_t count = 0;
  EXPECT_NO_THROW(
    zip_for_each_indexed_eq([&count](size_t idx, [[maybe_unused]] auto t) {
      EXPECT_EQ(idx, count);
      ++count;
    }, l1, l2, l3)
  );
  EXPECT_EQ(count, N);
}

TEST_F(InCompleteGroup, zip_for_each_indexed_eq_throws)
{
  EXPECT_THROW(
    zip_for_each_indexed_eq([](size_t, auto) {}, l1, l2),
    length_error
  );
}

TEST(ZipForEachIndexed, empty_containers)
{
  DynList<int> empty1, empty2;
  size_t count = 0;
  zip_for_each_indexed([&count](size_t, auto) { ++count; }, empty1, empty2);
  EXPECT_EQ(count, 0);
}

// ---------- Issue 19: zip_take / zip_drop ----------

TEST_F(CompleteGroup, zip_take_basic)
{
  auto result = zip_take(3, l1, l2, l3);

  EXPECT_EQ(result.size(), 3);

  size_t i = 0;
  for (auto it = result.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr();
      EXPECT_EQ(get<0>(t), static_cast<int>(i));
      EXPECT_EQ(get<1>(t), static_cast<int>(i));
      EXPECT_EQ(get<2>(t), to_string(i));
    }
}

TEST_F(CompleteGroup, zip_take_more_than_available)
{
  auto result = zip_take(100, l1, l2);  // Request more than N elements
  EXPECT_EQ(result.size(), N);  // Should only get N
}

TEST_F(CompleteGroup, zip_take_zero)
{
  auto result = zip_take(0, l1, l2);
  EXPECT_TRUE(result.is_empty());
}

TEST_F(CompleteGroup, zip_drop_basic)
{
  auto result = zip_drop(2, l1, l2, l3);

  EXPECT_EQ(result.size(), N - 2);

  size_t i = 2;  // Start from index 2
  for (auto it = result.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr();
      EXPECT_EQ(get<0>(t), static_cast<int>(i));
      EXPECT_EQ(get<1>(t), static_cast<int>(i));
      EXPECT_EQ(get<2>(t), to_string(i));
    }
}

TEST_F(CompleteGroup, zip_drop_all)
{
  auto result = zip_drop(N, l1, l2);
  EXPECT_TRUE(result.is_empty());
}

TEST_F(CompleteGroup, zip_drop_more_than_available)
{
  auto result = zip_drop(100, l1, l2);
  EXPECT_TRUE(result.is_empty());
}

TEST_F(CompleteGroup, zip_drop_zero)
{
  auto result = zip_drop(0, l1, l2);
  EXPECT_EQ(result.size(), N);
}

TEST_F(CompleteGroup, zip_take_while_basic)
{
  // Take while first element < 3
  auto result = zip_take_while([](auto t) { return get<0>(t) < 3; }, l1, l2, l3);

  EXPECT_EQ(result.size(), 3);  // Elements 0, 1, 2

  size_t i = 0;
  for (auto it = result.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr();
      EXPECT_EQ(get<0>(t), static_cast<int>(i));
      EXPECT_LT(get<0>(t), 3);
    }
}

TEST_F(CompleteGroup, zip_take_while_none)
{
  // Predicate immediately false
  auto result = zip_take_while([](auto t) { return get<0>(t) < 0; }, l1, l2);
  EXPECT_TRUE(result.is_empty());
}

TEST_F(CompleteGroup, zip_take_while_all)
{
  // Predicate always true
  auto result = zip_take_while([](auto t) { return get<0>(t) < 100; }, l1, l2);
  EXPECT_EQ(result.size(), N);
}

TEST_F(CompleteGroup, zip_drop_while_basic)
{
  // Drop while first element < 3, then collect the rest
  auto result = zip_drop_while([](auto t) { return get<0>(t) < 3; }, l1, l2, l3);

  EXPECT_EQ(result.size(), N - 3);  // Elements 3, 4

  size_t i = 3;
  for (auto it = result.get_it(); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr();
      EXPECT_EQ(get<0>(t), static_cast<int>(i));
    }
}

TEST_F(CompleteGroup, zip_drop_while_none)
{
  // Predicate immediately false - drop nothing
  auto result = zip_drop_while([](auto t) { return get<0>(t) < 0; }, l1, l2);
  EXPECT_EQ(result.size(), N);
}

TEST_F(CompleteGroup, zip_drop_while_all)
{
  // Predicate always true - drop all
  auto result = zip_drop_while([](auto t) { return get<0>(t) < 100; }, l1, l2);
  EXPECT_TRUE(result.is_empty());
}

TEST(ZipTakeDrop, empty_containers)
{
  DynList<int> empty1, empty2;

  EXPECT_TRUE(zip_take(5, empty1, empty2).is_empty());
  EXPECT_TRUE(zip_drop(5, empty1, empty2).is_empty());
  EXPECT_TRUE(zip_take_while([](auto) { return true; }, empty1, empty2).is_empty());
  EXPECT_TRUE(zip_drop_while([](auto) { return true; }, empty1, empty2).is_empty());
}

// ---------- Issue 20: Tuple helpers with index_sequence ----------

TEST(TupleHelpers, for_each_in_tuple_basic)
{
  auto t = make_tuple(1, 2.5, string("hello"));

  vector<string> collected;
  for_each_in_tuple([&collected](auto &elem) {
    ostringstream oss;
    oss << elem;
    collected.push_back(oss.str());
  }, t);

  EXPECT_EQ(collected.size(), 3);
  EXPECT_EQ(collected[0], "1");
  EXPECT_EQ(collected[1], "2.5");
  EXPECT_EQ(collected[2], "hello");
}

TEST(TupleHelpers, for_each_in_tuple_modify)
{
  auto t = make_tuple(1, 2, 3);

  for_each_in_tuple([](auto &elem) { elem *= 2; }, t);

  EXPECT_EQ(get<0>(t), 2);
  EXPECT_EQ(get<1>(t), 4);
  EXPECT_EQ(get<2>(t), 6);
}

TEST(TupleHelpers, for_each_in_tuple_empty)
{
  auto t = make_tuple();
  size_t count = 0;
  for_each_in_tuple([&count](auto &) { ++count; }, t);
  EXPECT_EQ(count, 0);
}

TEST(TupleHelpers, transform_tuple_basic)
{
  auto t = make_tuple(1, 2, 3);

  auto result = transform_tuple([](auto x) { return x * 10; }, t);

  EXPECT_EQ(get<0>(result), 10);
  EXPECT_EQ(get<1>(result), 20);
  EXPECT_EQ(get<2>(result), 30);
}

TEST(TupleHelpers, transform_tuple_type_change)
{
  auto t = make_tuple(1, 2, 3);

  // Transform int -> string
  auto result = transform_tuple([](auto x) { return to_string(x); }, t);

  static_assert(is_same_v<decay_t<decltype(get<0>(result))>, string>);
  EXPECT_EQ(get<0>(result), "1");
  EXPECT_EQ(get<1>(result), "2");
  EXPECT_EQ(get<2>(result), "3");
}

TEST(TupleHelpers, transform_tuple_heterogeneous)
{
  auto t = make_tuple(1, 2.5, string("hi"));

  // Double each value (works differently for each type)
  auto result = transform_tuple([](auto x) {
    if constexpr (is_same_v<decay_t<decltype(x)>, string>)
      return x + x;
    else
      return x * 2;
  }, t);

  EXPECT_EQ(get<0>(result), 2);
  EXPECT_DOUBLE_EQ(get<1>(result), 5.0);
  EXPECT_EQ(get<2>(result), "hihi");
}

TEST(TupleHelpers, all_of_tuple_true)
{
  auto t = make_tuple(2, 4, 6, 8);

  bool result = all_of_tuple([](auto x) { return x % 2 == 0; }, t);
  EXPECT_TRUE(result);
}

TEST(TupleHelpers, all_of_tuple_false)
{
  auto t = make_tuple(2, 4, 5, 8);  // 5 is odd

  bool result = all_of_tuple([](auto x) { return x % 2 == 0; }, t);
  EXPECT_FALSE(result);
}

TEST(TupleHelpers, all_of_tuple_empty)
{
  auto t = make_tuple();
  // Empty tuple should return true (vacuous truth)
  bool result = all_of_tuple([](auto) { return false; }, t);
  EXPECT_TRUE(result);
}

TEST(TupleHelpers, any_of_tuple_true)
{
  auto t = make_tuple(1, 3, 4, 7);  // 4 is even

  bool result = any_of_tuple([](auto x) { return x % 2 == 0; }, t);
  EXPECT_TRUE(result);
}

TEST(TupleHelpers, any_of_tuple_false)
{
  auto t = make_tuple(1, 3, 5, 7);  // All odd

  bool result = any_of_tuple([](auto x) { return x % 2 == 0; }, t);
  EXPECT_FALSE(result);
}

TEST(TupleHelpers, any_of_tuple_empty)
{
  auto t = make_tuple();
  // Empty tuple should return false
  bool result = any_of_tuple([](auto) { return true; }, t);
  EXPECT_FALSE(result);
}

TEST(TupleHelpers, all_any_combined)
{
  auto t = make_tuple(10, 20, 30, 40, 50);

  // All > 0
  EXPECT_TRUE(all_of_tuple([](auto x) { return x > 0; }, t));

  // Any > 45
  EXPECT_TRUE(any_of_tuple([](auto x) { return x > 45; }, t));

  // None > 100
  EXPECT_FALSE(any_of_tuple([](auto x) { return x > 100; }, t));

  // Not all > 25
  EXPECT_FALSE(all_of_tuple([](auto x) { return x > 25; }, t));
}

// Stress test for new features
TEST(StressTestNewFeatures, large_containers)
{
  const size_t SIZE = 1000;
  DynList<int> l1 = range<int>(0, SIZE - 1);
  DynList<int> l2 = range<int>(0, SIZE - 1);

  // zip_transform
  auto transformed = zip_transform([](auto t) {
    return get<0>(t) + get<1>(t);
  }, l1, l2);
  EXPECT_EQ(transformed.size(), SIZE);
  for (size_t i = 0; i < SIZE; ++i)
    EXPECT_EQ(transformed[i], static_cast<int>(2 * i));

  // zip_for_each_indexed
  size_t sum_indices = 0;
  zip_for_each_indexed([&sum_indices](size_t idx, auto) {
    sum_indices += idx;
  }, l1, l2);
  EXPECT_EQ(sum_indices, SIZE * (SIZE - 1) / 2);

  // zip_take
  auto taken = zip_take(100, l1, l2);
  EXPECT_EQ(taken.size(), 100);

  // zip_drop
  auto dropped = zip_drop(900, l1, l2);
  EXPECT_EQ(dropped.size(), 100);

  // zip_take_while
  auto taken_while = zip_take_while([](auto t) { return get<0>(t) < 500; }, l1, l2);
  EXPECT_EQ(taken_while.size(), 500);

  // zip_drop_while
  auto dropped_while = zip_drop_while([](auto t) { return get<0>(t) < 500; }, l1, l2);
  EXPECT_EQ(dropped_while.size(), 500);
}
