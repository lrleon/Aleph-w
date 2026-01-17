
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

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
