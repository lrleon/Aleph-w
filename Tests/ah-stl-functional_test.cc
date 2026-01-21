
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file ah-stl-functional_test.cc
 * @brief Tests for Ah Stl Functional
 */

/**
 * Comprehensive test suite for ah-stl-functional.H
 * 
 * Tests functional programming operations for STL containers.
 */

#include <gtest/gtest.h>
#include <ah-stl-functional.H>

#include <vector>
#include <list>
#include <deque>
#include <string>

using namespace Aleph;

//==============================================================================
// Range Generation Tests
//==============================================================================

TEST(StlRange, BasicRange)
{
  auto r = stl_range(1, 5);
  EXPECT_EQ(r.size(), 5);
  EXPECT_EQ(r[0], 1);
  EXPECT_EQ(r[4], 5);
}

TEST(StlRange, RangeWithStep)
{
  auto r = stl_range(0, 10, 2);
  EXPECT_EQ(r.size(), 6);
  EXPECT_EQ(r[0], 0);
  EXPECT_EQ(r[5], 10);
}

TEST(StlRange, SingleArgRange)
{
  auto r = stl_range(5);
  EXPECT_EQ(r.size(), 5);
  EXPECT_EQ(r[0], 0);
  EXPECT_EQ(r[4], 4);
}

TEST(StlLinspace, Basic)
{
  auto r = stl_linspace(0.0, 1.0, 5);
  EXPECT_EQ(r.size(), 5);
  EXPECT_DOUBLE_EQ(r[0], 0.0);
  EXPECT_DOUBLE_EQ(r[4], 1.0);
}

TEST(StlRep, Basic)
{
  auto r = stl_rep(5, 42);
  EXPECT_EQ(r.size(), 5);
  for (int x : r)
    EXPECT_EQ(x, 42);
}

TEST(StlGenerate, Basic)
{
  auto r = stl_generate(5, [](size_t i) { return i * i; });
  EXPECT_EQ(r.size(), 5);
  EXPECT_EQ(r[0], 0);
  EXPECT_EQ(r[2], 4);
  EXPECT_EQ(r[4], 16);
}

//==============================================================================
// Core Functional Operations Tests
//==============================================================================

TEST(StlForEach, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  int sum = 0;
  stl_for_each([&sum](int x) { sum += x; }, v);
  EXPECT_EQ(sum, 15);
}

TEST(StlForEachIndexed, Basic)
{
  std::vector<std::string> v = {"a", "b", "c"};
  std::vector<std::string> results;
  stl_for_each_indexed([&results](size_t i, const std::string& s) {
    results.push_back(std::to_string(i) + ":" + s);
  }, v);
  
  EXPECT_EQ(results[0], "0:a");
  EXPECT_EQ(results[2], "2:c");
}

TEST(StlMap, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto squares = stl_map([](int x) { return x * x; }, v);
  
  EXPECT_EQ(squares.size(), 5);
  EXPECT_EQ(squares[0], 1);
  EXPECT_EQ(squares[2], 9);
  EXPECT_EQ(squares[4], 25);
}

TEST(StlMap, TypeChange)
{
  std::vector<int> v = {1, 2, 3};
  auto strings = stl_map([](int x) { return std::to_string(x); }, v);
  
  EXPECT_EQ(strings[0], "1");
  EXPECT_EQ(strings[2], "3");
}

TEST(StlMapi, Basic)
{
  std::vector<std::string> v = {"a", "b", "c"};
  auto results = stl_mapi([](size_t i, const std::string& s) {
    return std::to_string(i) + s;
  }, v);
  
  EXPECT_EQ(results[0], "0a");
  EXPECT_EQ(results[2], "2c");
}

TEST(StlFilter, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  auto evens = stl_filter([](int x) { return x % 2 == 0; }, v);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(evens[0], 2);
  EXPECT_EQ(evens[2], 6);
}

TEST(StlFilteri, EvenIndices)
{
  std::vector<int> v = {10, 20, 30, 40, 50};
  auto evens = stl_filteri([](size_t i, int) { return i % 2 == 0; }, v);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(evens[0], 10);
  EXPECT_EQ(evens[1], 30);
  EXPECT_EQ(evens[2], 50);
}

//==============================================================================
// Fold Tests
//==============================================================================

TEST(StlFoldl, Sum)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  int sum = stl_foldl(0, [](int acc, int x) { return acc + x; }, v);
  EXPECT_EQ(sum, 15);
}

TEST(StlFoldl, Product)
{
  std::vector<int> v = {1, 2, 3, 4};
  int product = stl_foldl(1, [](int acc, int x) { return acc * x; }, v);
  EXPECT_EQ(product, 24);
}

TEST(StlFoldr, Subtraction)
{
  // Right fold: 1 - (2 - (3 - 0)) = 1 - (2 - 3) = 1 - (-1) = 2
  std::vector<int> v = {1, 2, 3};
  int result = stl_foldr(0, [](int x, int acc) { return x - acc; }, v);
  EXPECT_EQ(result, 2);
}

TEST(StlFoldr, ListConstruction)
{
  // Right fold to construct string: "1" + ("2" + ("3" + ""))
  std::vector<int> v = {1, 2, 3};
  std::string result = stl_foldr(std::string(""), [](int x, std::string acc) {
    return std::to_string(x) + acc;
  }, v);
  EXPECT_EQ(result, "123");
}

TEST(StlScanLeft, RunningSum)
{
  std::vector<int> v = {1, 2, 3, 4};
  auto sums = stl_scan_left(0, [](int acc, int x) { return acc + x; }, v);
  
  EXPECT_EQ(sums.size(), 5);
  EXPECT_EQ(sums[0], 0);   // init
  EXPECT_EQ(sums[1], 1);   // 0 + 1
  EXPECT_EQ(sums[2], 3);   // 1 + 2
  EXPECT_EQ(sums[3], 6);   // 3 + 3
  EXPECT_EQ(sums[4], 10);  // 6 + 4
}

TEST(StlScanRight, Basic)
{
  std::vector<int> v = {1, 2, 3};
  auto results = stl_scan_right(0, [](int x, int acc) { return x + acc; }, v);
  
  // scan_right [1,2,3] with + and 0: [6, 5, 3, 0]
  EXPECT_EQ(results.size(), 4);
  EXPECT_EQ(results[0], 6);  // 1+2+3+0
  EXPECT_EQ(results[1], 5);  // 2+3+0
  EXPECT_EQ(results[2], 3);  // 3+0
  EXPECT_EQ(results[3], 0);  // init
}

//==============================================================================
// Predicate Tests
//==============================================================================

TEST(StlAll, AllTrue)
{
  std::vector<int> v = {2, 4, 6, 8};
  EXPECT_TRUE(stl_all([](int x) { return x % 2 == 0; }, v));
}

TEST(StlAll, SomeFalse)
{
  std::vector<int> v = {2, 3, 6, 8};
  EXPECT_FALSE(stl_all([](int x) { return x % 2 == 0; }, v));
}

TEST(StlExists, SomeTrue)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_TRUE(stl_exists([](int x) { return x == 3; }, v));
}

TEST(StlExists, NoneTrue)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_FALSE(stl_exists([](int x) { return x == 10; }, v));
}

TEST(StlNone, Basic)
{
  std::vector<int> v = {1, 3, 5, 7};
  EXPECT_TRUE(stl_none([](int x) { return x % 2 == 0; }, v));
}

//==============================================================================
// Finding Tests
//==============================================================================

TEST(StlFind, Found)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_find([](int x) { return x > 3; }, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 4);
}

TEST(StlFind, NotFound)
{
  std::vector<int> v = {1, 2, 3};
  auto result = stl_find([](int x) { return x > 10; }, v);
  
  EXPECT_FALSE(result.has_value());
}

TEST(StlFindLast, Found)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_find_last([](int x) { return x % 2 == 0; }, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 4);
}

TEST(StlFindIndex, Found)
{
  std::vector<std::string> v = {"a", "b", "c", "d"};
  auto result = stl_find_index([](const std::string& s) { return s == "c"; }, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 2);
}

TEST(StlFindMapi, Found)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_find_mapi([](size_t i, int x) -> std::optional<std::string> {
    if (x == 3)
      return "found at " + std::to_string(i);
    return std::nullopt;
  }, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "found at 2");
}

TEST(StlMem, Found)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_TRUE(stl_mem(3, v));
  EXPECT_FALSE(stl_mem(10, v));
}

//==============================================================================
// Counting Tests
//==============================================================================

TEST(StlCount, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  size_t count = stl_count([](int x) { return x % 2 == 0; }, v);
  EXPECT_EQ(count, 3);
}

TEST(StlCountValue, Basic)
{
  std::vector<int> v = {1, 2, 2, 3, 2, 4};
  EXPECT_EQ(stl_count_value(2, v), 3);
}

//==============================================================================
// Take and Drop Tests
//==============================================================================

TEST(StlTake, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_take(3, v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[2], 3);
}

TEST(StlDrop, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_drop(2, v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 3);
  EXPECT_EQ(result[2], 5);
}

TEST(StlTakeLast, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_take_last(3, v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 3);
  EXPECT_EQ(result[2], 5);
}

TEST(StlTakeWhile, Basic)
{
  std::vector<int> v = {1, 2, 3, 10, 4, 5};
  auto result = stl_take_while([](int x) { return x < 10; }, v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[2], 3);
}

TEST(StlDropWhile, Basic)
{
  std::vector<int> v = {1, 2, 3, 10, 4, 5};
  auto result = stl_drop_while([](int x) { return x < 10; }, v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 10);
}

//==============================================================================
// Accessing Tests
//==============================================================================

TEST(StlFirst, Basic)
{
  std::vector<int> v = {10, 20, 30};
  auto result = stl_first(v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 10);
}

TEST(StlFirst, Empty)
{
  std::vector<int> v;
  auto result = stl_first(v);
  
  EXPECT_FALSE(result.has_value());
}

TEST(StlLast, Basic)
{
  std::vector<int> v = {10, 20, 30};
  auto result = stl_last(v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 30);
}

TEST(StlNth, Basic)
{
  std::vector<int> v = {10, 20, 30, 40, 50};
  auto result = stl_nth(2, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 30);
}

//==============================================================================
// Min/Max Tests
//==============================================================================

TEST(StlMin, Basic)
{
  std::vector<int> v = {3, 1, 4, 1, 5, 9};
  auto result = stl_min(v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 1);
}

TEST(StlMax, Basic)
{
  std::vector<int> v = {3, 1, 4, 1, 5, 9};
  auto result = stl_max(v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 9);
}

TEST(StlMinMax, Basic)
{
  std::vector<int> v = {3, 1, 4, 1, 5, 9};
  auto result = stl_min_max(v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, 1);
  EXPECT_EQ(result->second, 9);
}

TEST(StlMinBy, Basic)
{
  std::vector<std::string> v = {"hello", "a", "world"};
  auto result = stl_min_by([](const std::string& s) { return s.length(); }, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "a");
}

TEST(StlMaxBy, Basic)
{
  std::vector<std::string> v = {"hello", "a", "world"};
  auto result = stl_max_by([](const std::string& s) { return s.length(); }, v);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "hello");
}

//==============================================================================
// Sum and Product Tests
//==============================================================================

TEST(StlSum, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_EQ(stl_sum(v), 15);
}

TEST(StlProduct, Basic)
{
  std::vector<int> v = {1, 2, 3, 4};
  EXPECT_EQ(stl_product(v), 24);
}

//==============================================================================
// Partition Tests
//==============================================================================

TEST(StlPartition, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  auto [evens, odds] = stl_partition([](int x) { return x % 2 == 0; }, v);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(odds.size(), 3);
}

//==============================================================================
// Zip and Enumerate Tests
//==============================================================================

TEST(StlZipToPairs, Basic)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<std::string> v2 = {"a", "b", "c"};
  
  auto result = stl_zip_to_pairs(v1, v2);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].first, 1);
  EXPECT_EQ(result[0].second, "a");
}

TEST(StlUnzipPairs, Basic)
{
  std::vector<std::pair<int, std::string>> v = {{1, "a"}, {2, "b"}, {3, "c"}};
  
  auto [nums, strs] = stl_unzip_pairs(v);
  
  EXPECT_EQ(nums.size(), 3);
  EXPECT_EQ(strs.size(), 3);
  EXPECT_EQ(nums[1], 2);
  EXPECT_EQ(strs[1], "b");
}

TEST(StlEnumerateToPairs, Basic)
{
  std::vector<std::string> v = {"a", "b", "c"};
  
  auto result = stl_enumerate_to_pairs(v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].first, 0);
  EXPECT_EQ(result[0].second, "a");
  EXPECT_EQ(result[2].first, 2);
}

//==============================================================================
// Comparison Tests
//==============================================================================

TEST(StlEqual, Equal)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {1, 2, 3};
  
  EXPECT_TRUE(stl_equal(v1, v2));
}

TEST(StlEqual, NotEqual)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {1, 2, 4};
  
  EXPECT_FALSE(stl_equal(v1, v2));
}

TEST(StlCompare, Equal)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {1, 2, 3};
  
  EXPECT_EQ(stl_compare(v1, v2), 0);
}

TEST(StlCompare, Less)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {1, 2, 4};
  
  EXPECT_EQ(stl_compare(v1, v2), -1);
}

TEST(StlCompare, Greater)
{
  std::vector<int> v1 = {1, 2, 4};
  std::vector<int> v2 = {1, 2, 3};
  
  EXPECT_EQ(stl_compare(v1, v2), 1);
}

//==============================================================================
// Reverse and Sort Tests
//==============================================================================

TEST(StlReverse, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_reverse(v);
  
  EXPECT_EQ(result[0], 5);
  EXPECT_EQ(result[4], 1);
}

TEST(StlSort, Basic)
{
  std::vector<int> v = {3, 1, 4, 1, 5, 9};
  auto result = stl_sort(v);
  
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[5], 9);
}

TEST(StlSortBy, Basic)
{
  std::vector<std::string> v = {"hello", "a", "world"};
  auto result = stl_sort_by([](const std::string& a, const std::string& b) {
    return a.length() < b.length();
  }, v);
  
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[2], "world");
}

//==============================================================================
// Uniqueness Tests
//==============================================================================

TEST(StlUnique, Basic)
{
  std::vector<int> v = {1, 1, 2, 2, 2, 3, 3};
  auto result = stl_unique(v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST(StlDistinct, Basic)
{
  std::vector<int> v = {1, 2, 1, 3, 2, 4};
  auto result = stl_distinct(v);
  
  EXPECT_EQ(result.size(), 4);
}

//==============================================================================
// Concat and Flatten Tests
//==============================================================================

TEST(StlConcat, Basic)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {4, 5, 6};
  
  auto result = stl_concat(v1, v2);
  
  EXPECT_EQ(result.size(), 6);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[5], 6);
}

TEST(StlFlatten, Basic)
{
  std::vector<std::vector<int>> v = {{1, 2}, {3, 4}, {5}};
  
  auto result = stl_flatten(v);
  
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[4], 5);
}

TEST(StlFlatMap, Basic)
{
  std::vector<int> v = {1, 2, 3};
  auto result = stl_flat_map([](int x) {
    return std::vector<int>{x, x * 10};
  }, v);
  
  EXPECT_EQ(result.size(), 6);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 10);
  EXPECT_EQ(result[4], 3);
  EXPECT_EQ(result[5], 30);
}

//==============================================================================
// Grouping Tests
//==============================================================================

TEST(StlGroup, Basic)
{
  std::vector<int> v = {1, 1, 2, 2, 2, 3};
  
  auto result = stl_group(v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].size(), 2);  // {1, 1}
  EXPECT_EQ(result[1].size(), 3);  // {2, 2, 2}
  EXPECT_EQ(result[2].size(), 1);  // {3}
}

TEST(StlGroupBy, Basic)
{
  std::vector<std::string> v = {"a", "bb", "c", "dd", "eee"};
  
  auto result = stl_group_by([](const std::string& s) { return s.length(); }, v);
  
  EXPECT_EQ(result.size(), 3);  // lengths 1, 2, 3
}

//==============================================================================
// Works with Different Container Types
//==============================================================================

TEST(StlFunctional, WorksWithList)
{
  std::list<int> l = {1, 2, 3, 4, 5};
  
  auto squares = stl_map([](int x) { return x * x; }, l);
  EXPECT_EQ(squares.size(), 5);
  EXPECT_EQ(squares[4], 25);
  
  int sum = stl_foldl(0, [](int acc, int x) { return acc + x; }, l);
  EXPECT_EQ(sum, 15);
  
  // foldr works with list (has rbegin/rend)
  int result = stl_foldr(0, [](int x, int acc) { return x - acc; }, l);
  EXPECT_EQ(result, 3);  // 1-(2-(3-(4-(5-0)))) = 1-(2-(3-(4-5))) = ...
}

TEST(StlFunctional, WorksWithDeque)
{
  std::deque<int> d = {1, 2, 3, 4, 5};
  
  auto evens = stl_filter([](int x) { return x % 2 == 0; }, d);
  EXPECT_EQ(evens.size(), 2);
}

//==============================================================================
// Combinatorics Tests
//==============================================================================

TEST(StlPermutations, Basic)
{
  std::vector<int> v = {1, 2, 3};
  auto perms = stl_permutations(v);
  
  EXPECT_EQ(perms.size(), 6);  // 3! = 6
}

TEST(StlPermutations, TraverseWithStop)
{
  std::vector<int> v = {1, 2, 3};
  int count = 0;
  
  bool completed = stl_traverse_permutations([&count](const auto&) {
    ++count;
    return count < 3;  // stop after 3
  }, v);
  
  EXPECT_FALSE(completed);
  EXPECT_EQ(count, 3);
}

TEST(StlCombinations, ChooseTwo)
{
  std::vector<int> v = {1, 2, 3, 4};
  auto combos = stl_combinations(2, v);
  
  // C(4,2) = 6
  EXPECT_EQ(combos.size(), 6);
  
  // Verify one combination
  EXPECT_EQ(combos[0][0], 1);
  EXPECT_EQ(combos[0][1], 2);
}

TEST(StlCombinations, ChooseThree)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto combos = stl_combinations(3, v);
  
  // C(5,3) = 10
  EXPECT_EQ(combos.size(), 10);
}

TEST(StlArrangements, ChooseTwo)
{
  std::vector<int> v = {1, 2, 3};
  auto arrs = stl_arrangements(2, v);
  
  // P(3,2) = 3!/(3-2)! = 6
  EXPECT_EQ(arrs.size(), 6);
}

TEST(StlCartesianProduct, Basic)
{
  std::vector<std::vector<int>> sets = {{1, 2}, {3, 4}};
  auto product = stl_cartesian_product(sets);
  
  // 2 * 2 = 4
  EXPECT_EQ(product.size(), 4);
}

TEST(StlPowerSet, Basic)
{
  std::vector<int> v = {1, 2, 3};
  auto ps = stl_power_set(v);
  
  // 2^3 = 8
  EXPECT_EQ(ps.size(), 8);
}

//==============================================================================
// Ruby/ML Operations Tests
//==============================================================================

TEST(StlSlidingWindow, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto windows = stl_sliding_window(3, v);
  
  EXPECT_EQ(windows.size(), 3);
  EXPECT_EQ(windows[0], (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(windows[1], (std::vector<int>{2, 3, 4}));
  EXPECT_EQ(windows[2], (std::vector<int>{3, 4, 5}));
}

TEST(StlChunks, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto chunks = stl_chunks(2, v);
  
  EXPECT_EQ(chunks.size(), 3);
  EXPECT_EQ(chunks[0], (std::vector<int>{1, 2}));
  EXPECT_EQ(chunks[1], (std::vector<int>{3, 4}));
  EXPECT_EQ(chunks[2], (std::vector<int>{5}));
}

TEST(StlIntersperse, Basic)
{
  std::vector<int> v = {1, 2, 3};
  auto result = stl_intersperse(0, v);
  
  EXPECT_EQ(result, (std::vector<int>{1, 0, 2, 0, 3}));
}

TEST(StlSplitAt, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto [first, second] = stl_split_at(2, v);
  
  EXPECT_EQ(first, (std::vector<int>{1, 2}));
  EXPECT_EQ(second, (std::vector<int>{3, 4, 5}));
}

TEST(StlSpan, Basic)
{
  std::vector<int> v = {1, 2, 3, 10, 4, 5};
  auto [first, second] = stl_span([](int x) { return x < 10; }, v);
  
  EXPECT_EQ(first, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(second, (std::vector<int>{10, 4, 5}));
}

TEST(StlInit, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_init(v);
  
  EXPECT_EQ(result, (std::vector<int>{1, 2, 3, 4}));
}

TEST(StlTail, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto result = stl_tail(v);
  
  EXPECT_EQ(result, (std::vector<int>{2, 3, 4, 5}));
}

TEST(StlTally, Basic)
{
  std::vector<int> v = {1, 2, 2, 3, 3, 3};
  auto result = stl_tally(v);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].first, 1);
  EXPECT_EQ(result[0].second, 1);
  EXPECT_EQ(result[1].first, 2);
  EXPECT_EQ(result[1].second, 2);
  EXPECT_EQ(result[2].first, 3);
  EXPECT_EQ(result[2].second, 3);
}

TEST(StlReject, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  auto result = stl_reject([](int x) { return x % 2 == 0; }, v);

  EXPECT_EQ(result, (std::vector<int>{1, 3, 5}));
}

//==============================================================================
// Performance Tests - Large Containers (Hash Path)
//==============================================================================

TEST(StlDistinctPerformance, LargeContainerUsesHashPath)
{
  // Create container with 1000 elements, 100 distinct values
  std::vector<int> large;
  large.reserve(1000);
  for (int i = 0; i < 1000; ++i)
    large.push_back(i % 100);

  auto result = stl_distinct(large);

  EXPECT_EQ(result.size(), 100);
  // Verify order preserved (first occurrence)
  EXPECT_EQ(result[0], 0);
  EXPECT_EQ(result[1], 1);
  EXPECT_EQ(result[99], 99);
}

TEST(StlDistinctPerformance, VeryLargeContainer)
{
  // 10000 elements, 500 distinct
  std::vector<int> very_large;
  very_large.reserve(10000);
  for (int i = 0; i < 10000; ++i)
    very_large.push_back(i % 500);

  auto result = stl_distinct(very_large);

  EXPECT_EQ(result.size(), 500);
}

TEST(StlTallyPerformance, LargeContainerUsesHashPath)
{
  std::vector<int> large;
  large.reserve(1000);
  for (int i = 0; i < 1000; ++i)
    large.push_back(i % 100);

  auto result = stl_tally(large);

  EXPECT_EQ(result.size(), 100);
  // Each value appears 10 times
  for (const auto & [val, count] : result)
    EXPECT_EQ(count, 10);
}

TEST(StlGroupByPerformance, LargeContainerUsesHashPath)
{
  std::vector<int> large;
  large.reserve(1000);
  for (int i = 0; i < 1000; ++i)
    large.push_back(i);

  // Group by last digit
  auto result = stl_group_by([](int x) { return x % 10; }, large);

  EXPECT_EQ(result.size(), 10);
  for (const auto & [key, group] : result)
    EXPECT_EQ(group.size(), 100);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST(StlDistinctEdgeCases, EmptyContainer)
{
  std::vector<int> empty;
  auto result = stl_distinct(empty);
  EXPECT_TRUE(result.empty());
}

TEST(StlDistinctEdgeCases, SingleElement)
{
  std::vector<int> single = {42};
  auto result = stl_distinct(single);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 42);
}

TEST(StlDistinctEdgeCases, AllSame)
{
  std::vector<int> all_same(100, 42);
  auto result = stl_distinct(all_same);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 42);
}

TEST(StlDistinctEdgeCases, AllUnique)
{
  auto input = stl_range(0, 99);
  auto result = stl_distinct(input);
  EXPECT_EQ(result.size(), 100);
}

TEST(StlTallyEdgeCases, EmptyContainer)
{
  std::vector<int> empty;
  auto result = stl_tally(empty);
  EXPECT_TRUE(result.empty());
}

TEST(StlTallyEdgeCases, SingleElement)
{
  std::vector<int> single = {42};
  auto result = stl_tally(single);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].first, 42);
  EXPECT_EQ(result[0].second, 1);
}

TEST(StlGroupByEdgeCases, EmptyContainer)
{
  std::vector<int> empty;
  auto result = stl_group_by([](int x) { return x; }, empty);
  EXPECT_TRUE(result.empty());
}

TEST(StlGroupByEdgeCases, SingleElement)
{
  std::vector<int> single = {42};
  auto result = stl_group_by([](int x) { return x % 10; }, single);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].first, 2);
  EXPECT_EQ(result[0].second.size(), 1);
}

TEST(StlGroupByEdgeCases, AllSameKey)
{
  std::vector<int> v = {10, 20, 30, 40, 50};
  auto result = stl_group_by([](int x) { return x % 10; }, v);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].first, 0);
  EXPECT_EQ(result[0].second.size(), 5);
}

//==============================================================================
// Small Container Tests (Linear Path - threshold <= 64)
//==============================================================================

TEST(StlDistinctSmall, UsesLinearPath)
{
  // Exactly at threshold
  std::vector<int> at_threshold;
  for (int i = 0; i < 64; ++i)
    at_threshold.push_back(i % 32);

  auto result = stl_distinct(at_threshold);
  EXPECT_EQ(result.size(), 32);
}

TEST(StlTallySmall, UsesLinearPath)
{
  std::vector<int> small = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
  auto result = stl_tally(small);

  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result[0].second, 1);  // 1 appears once
  EXPECT_EQ(result[1].second, 2);  // 2 appears twice
  EXPECT_EQ(result[2].second, 3);  // 3 appears three times
  EXPECT_EQ(result[3].second, 4);  // 4 appears four times
}

//==============================================================================
// Custom Types Tests
//==============================================================================

namespace {

struct Point
{
  int x, y;

  bool operator==(const Point & other) const
  {
    return x == other.x && y == other.y;
  }
};

struct PointHash
{
  size_t operator()(const Point & p) const
  {
    return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
  }
};

// Make Point hashable for std::unordered_set/map
} // anonymous namespace

namespace std {
template<>
struct hash<Point>
{
  size_t operator()(const Point & p) const
  {
    return hash<int>{}(p.x) ^ (hash<int>{}(p.y) << 1);
  }
};
} // namespace std

TEST(StlDistinctCustomType, PointsSmall)
{
  std::vector<Point> points = {{1, 2}, {3, 4}, {1, 2}, {5, 6}, {3, 4}};
  auto result = stl_distinct(points);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].x, 1);
  EXPECT_EQ(result[0].y, 2);
}

TEST(StlDistinctCustomType, PointsLarge)
{
  std::vector<Point> points;
  points.reserve(200);
  for (int i = 0; i < 200; ++i)
    points.push_back({i % 50, i % 25});

  auto result = stl_distinct(points);
  // 50 * 25 = 1250 possible, but we only have 200 elements with some repeats
  EXPECT_LE(result.size(), 200);
  EXPECT_GT(result.size(), 0);
}

TEST(StlTallyCustomType, Points)
{
  std::vector<Point> points = {{1, 2}, {3, 4}, {1, 2}, {5, 6}, {1, 2}};
  auto result = stl_tally(points);

  EXPECT_EQ(result.size(), 3);
  // Find the count for {1,2}
  auto it = std::find_if(result.begin(), result.end(),
                         [](const auto & p) { return p.first.x == 1 && p.first.y == 2; });
  ASSERT_NE(it, result.end());
  EXPECT_EQ(it->second, 3);
}

TEST(StlGroupByCustomType, PointsByQuadrant)
{
  std::vector<Point> points = {
    {1, 1}, {-1, 1}, {-1, -1}, {1, -1},
    {2, 3}, {-2, 3}, {-2, -3}, {2, -3}
  };

  auto quadrant = [](const Point & p) {
    if (p.x >= 0 && p.y >= 0) return 1;
    if (p.x < 0 && p.y >= 0) return 2;
    if (p.x < 0 && p.y < 0) return 3;
    return 4;
  };

  auto result = stl_group_by(quadrant, points);

  EXPECT_EQ(result.size(), 4);
  for (const auto & [quad, pts] : result)
    EXPECT_EQ(pts.size(), 2);
}

//==============================================================================
// Move Semantics and Forwarding Tests
//==============================================================================

namespace {

struct MoveTracker
{
  int value;
  static int move_count;
  static int copy_count;

  MoveTracker(int v = 0) : value(v) {}

  MoveTracker(const MoveTracker & other) : value(other.value)
  {
    ++copy_count;
  }

  MoveTracker(MoveTracker && other) noexcept : value(other.value)
  {
    other.value = -1;
    ++move_count;
  }

  MoveTracker & operator=(const MoveTracker & other)
  {
    value = other.value;
    ++copy_count;
    return *this;
  }

  MoveTracker & operator=(MoveTracker && other) noexcept
  {
    value = other.value;
    other.value = -1;
    ++move_count;
    return *this;
  }

  bool operator==(const MoveTracker & other) const
  {
    return value == other.value;
  }

  static void reset()
  {
    move_count = 0;
    copy_count = 0;
  }
};

int MoveTracker::move_count = 0;
int MoveTracker::copy_count = 0;

} // anonymous namespace

namespace std {
template<>
struct hash<MoveTracker>
{
  size_t operator()(const MoveTracker & m) const
  {
    return hash<int>{}(m.value);
  }
};
} // namespace std

TEST(StlMapForwarding, LambdaWithCapture)
{
  std::vector<int> v = {1, 2, 3};
  std::string prefix = "num_";

  auto result = stl_map([&prefix](int x) {
    return prefix + std::to_string(x);
  }, v);

  EXPECT_EQ(result[0], "num_1");
  EXPECT_EQ(result[1], "num_2");
  EXPECT_EQ(result[2], "num_3");
}

TEST(StlFilterForwarding, MutableLambda)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  int threshold = 3;

  // Mutable lambda that modifies captured state
  int call_count = 0;
  auto result = stl_filter([&call_count, threshold](int x) mutable {
    ++call_count;
    return x > threshold;
  }, v);

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(call_count, 5);
}

TEST(StlFoldlForwarding, AccumulatorByValue)
{
  std::vector<std::string> words = {"hello", " ", "world"};

  auto result = stl_foldl(std::string{}, [](std::string acc, const std::string & w) {
    return acc + w;
  }, words);

  EXPECT_EQ(result, "hello world");
}

//==============================================================================
// Different Container Types
//==============================================================================

TEST(StlDistinctContainerTypes, List)
{
  std::list<int> l = {1, 2, 1, 3, 2, 4};
  auto result = stl_distinct(l);
  EXPECT_EQ(result.size(), 4);
}

TEST(StlDistinctContainerTypes, Deque)
{
  std::deque<int> d = {1, 2, 1, 3, 2, 4};
  auto result = stl_distinct(d);
  EXPECT_EQ(result.size(), 4);
}

TEST(StlTallyContainerTypes, List)
{
  std::list<int> l = {1, 2, 2, 3, 3, 3};
  auto result = stl_tally(l);
  EXPECT_EQ(result.size(), 3);
}

TEST(StlGroupByContainerTypes, List)
{
  std::list<std::string> l = {"a", "bb", "ccc", "dd", "e"};
  auto result = stl_group_by([](const std::string & s) { return s.length(); }, l);
  EXPECT_EQ(result.size(), 3);
}

//==============================================================================
// Order Preservation Tests
//==============================================================================

TEST(StlDistinctOrder, PreservesFirstOccurrence)
{
  std::vector<int> v = {5, 3, 5, 1, 3, 7, 1, 5};
  auto result = stl_distinct(v);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 5);  // First occurrence
  EXPECT_EQ(result[1], 3);
  EXPECT_EQ(result[2], 1);
  EXPECT_EQ(result[3], 7);
}

TEST(StlDistinctOrder, PreservesFirstOccurrenceLarge)
{
  // Test with hash path
  std::vector<int> v;
  v.reserve(200);
  for (int i = 199; i >= 0; --i)
    v.push_back(i % 100);

  auto result = stl_distinct(v);

  ASSERT_EQ(result.size(), 100);
  // First occurrence of 99 is at index 0 (199 % 100 = 99)
  EXPECT_EQ(result[0], 99);
  // First occurrence of 98 is at index 1 (198 % 100 = 98)
  EXPECT_EQ(result[1], 98);
}

TEST(StlTallyOrder, PreservesFirstOccurrence)
{
  std::vector<int> v = {5, 3, 5, 1, 3, 7, 1, 5};
  auto result = stl_tally(v);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0].first, 5);
  EXPECT_EQ(result[0].second, 3);
  EXPECT_EQ(result[1].first, 3);
  EXPECT_EQ(result[1].second, 2);
  EXPECT_EQ(result[2].first, 1);
  EXPECT_EQ(result[2].second, 2);
  EXPECT_EQ(result[3].first, 7);
  EXPECT_EQ(result[3].second, 1);
}

TEST(StlGroupByOrder, PreservesFirstOccurrence)
{
  std::vector<int> v = {15, 23, 31, 42, 54};
  auto result = stl_group_by([](int x) { return x % 10; }, v);

  ASSERT_EQ(result.size(), 5);
  // First key seen is 5 (from 15)
  EXPECT_EQ(result[0].first, 5);
  // Second key seen is 3 (from 23)
  EXPECT_EQ(result[1].first, 3);
}

//==============================================================================
// String Tests
//==============================================================================

TEST(StlDistinctStrings, Basic)
{
  std::vector<std::string> v = {"apple", "banana", "apple", "cherry", "banana"};
  auto result = stl_distinct(v);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "apple");
  EXPECT_EQ(result[1], "banana");
  EXPECT_EQ(result[2], "cherry");
}

TEST(StlDistinctStrings, Large)
{
  std::vector<std::string> v;
  v.reserve(200);
  for (int i = 0; i < 200; ++i)
    v.push_back("str_" + std::to_string(i % 50));

  auto result = stl_distinct(v);
  EXPECT_EQ(result.size(), 50);
}

TEST(StlGroupByStrings, ByLength)
{
  std::vector<std::string> v = {"a", "bb", "ccc", "dd", "eee", "f"};
  auto result = stl_group_by([](const std::string & s) { return s.length(); }, v);

  EXPECT_EQ(result.size(), 3);
}

TEST(StlGroupByStrings, ByFirstChar)
{
  std::vector<std::string> v = {"apple", "apricot", "banana", "blueberry", "cherry"};
  auto result = stl_group_by([](const std::string & s) { return s[0]; }, v);

  EXPECT_EQ(result.size(), 3);  // a, b, c
}

//==============================================================================
// Boundary Tests (around threshold = 64)
//==============================================================================

TEST(StlDistinctBoundary, JustBelowThreshold)
{
  std::vector<int> v;
  for (int i = 0; i < 63; ++i)
    v.push_back(i % 30);

  auto result = stl_distinct(v);
  EXPECT_EQ(result.size(), 30);
}

TEST(StlDistinctBoundary, ExactlyAtThreshold)
{
  std::vector<int> v;
  for (int i = 0; i < 64; ++i)
    v.push_back(i % 30);

  auto result = stl_distinct(v);
  EXPECT_EQ(result.size(), 30);
}

TEST(StlDistinctBoundary, JustAboveThreshold)
{
  std::vector<int> v;
  for (int i = 0; i < 65; ++i)
    v.push_back(i % 30);

  auto result = stl_distinct(v);
  EXPECT_EQ(result.size(), 30);
}

//==============================================================================
// Composition Tests
//==============================================================================

TEST(StlComposition, DistinctThenMap)
{
  std::vector<int> v = {1, 2, 1, 3, 2, 4};
  auto distinct = stl_distinct(v);
  auto squared = stl_map([](int x) { return x * x; }, distinct);

  EXPECT_EQ(squared, (std::vector<int>{1, 4, 9, 16}));
}

TEST(StlComposition, FilterThenDistinct)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6, 1, 2, 3};
  auto evens = stl_filter([](int x) { return x % 2 == 0; }, v);
  auto unique_evens = stl_distinct(evens);

  EXPECT_EQ(unique_evens, (std::vector<int>{2, 4, 6}));
}

TEST(StlComposition, GroupByThenMap)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  auto groups = stl_group_by([](int x) { return x % 2; }, v);
  auto sums = stl_map([](const auto & p) {
    return std::make_pair(p.first, stl_foldl(0, std::plus<int>{}, p.second));
  }, groups);

  EXPECT_EQ(sums.size(), 2);
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
