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
 * @file ah-stl-zip_test.cc
 * @brief Tests for Ah Stl Zip
 */

/**
 * Comprehensive test suite for ah-stl-zip.H
 * 
 * Tests lazy zip operations over STL containers including:
 * - StlZipIterator and StlZipView
 * - Functional operations: all, exists, map, foldl, for_each
 * - Various container types: vector, list, set, deque, array
 * - Edge cases: empty containers, different lengths, single elements
 */

#include <gtest/gtest.h>
#include <ah-stl-zip.H>

#include <vector>
#include <list>
#include <set>
#include <deque>
#include <array>
#include <string>
#include <numeric>

using namespace Aleph;

//==============================================================================
// Basic StlZipView Tests
//==============================================================================

class StlZipViewTest : public ::testing::Test
{
protected:
  std::vector<int> vi = {1, 2, 3, 4, 5};
  std::vector<std::string> vs = {"a", "b", "c", "d", "e"};
  std::list<double> ld = {1.1, 2.2, 3.3, 4.4, 5.5};
};

TEST_F(StlZipViewTest, BasicIteration)
{
  int count = 0;
  for (auto [i, s] : stl_zip(vi, vs))
    {
      EXPECT_EQ(i, vi[count]);
      EXPECT_EQ(s, vs[count]);
      ++count;
    }
  EXPECT_EQ(count, 5);
}

TEST_F(StlZipViewTest, ThreeContainers)
{
  int count = 0;
  for (auto [i, s, d] : stl_zip(vi, vs, ld))
    {
      EXPECT_EQ(i, vi[count]);
      EXPECT_EQ(s, vs[count]);
      ++count;
    }
  EXPECT_EQ(count, 5);
}

TEST_F(StlZipViewTest, DifferentLengths_StopsAtShortest)
{
  std::vector<int> short_vec = {10, 20};
  
  int count = 0;
  for (auto [i, s] : stl_zip(short_vec, vs))
    {
      ++count;
    }
  EXPECT_EQ(count, 2);  // Stops at shorter container
}

TEST_F(StlZipViewTest, EmptyContainer)
{
  std::vector<int> empty_vec;
  
  int count = 0;
  for (auto [i, s] : stl_zip(empty_vec, vs))
    {
      ++count;
    }
  EXPECT_EQ(count, 0);
}

TEST_F(StlZipViewTest, SingleElement)
{
  std::vector<int> single = {42};
  std::vector<std::string> single_s = {"answer"};
  
  int count = 0;
  for (auto [i, s] : stl_zip(single, single_s))
    {
      EXPECT_EQ(i, 42);
      EXPECT_EQ(s, "answer");
      ++count;
    }
  EXPECT_EQ(count, 1);
}

TEST_F(StlZipViewTest, ViewSize)
{
  auto view = stl_zip(vi, vs);
  EXPECT_EQ(view.size(), 5);
}

TEST_F(StlZipViewTest, ViewEmpty)
{
  std::vector<int> empty_vec;
  auto view = stl_zip(empty_vec, vs);
  EXPECT_TRUE(view.empty());
  
  auto view2 = stl_zip(vi, vs);
  EXPECT_FALSE(view2.empty());
}

//==============================================================================
// Different Container Types
//==============================================================================

TEST(StlZipContainerTypes, VectorAndList)
{
  std::vector<int> v = {1, 2, 3};
  std::list<int> l = {10, 20, 30};
  
  std::vector<int> sums;
  for (auto [a, b] : stl_zip(v, l))
    sums.push_back(a + b);
  
  EXPECT_EQ(sums.size(), 3);
  EXPECT_EQ(sums[0], 11);
  EXPECT_EQ(sums[1], 22);
  EXPECT_EQ(sums[2], 33);
}

TEST(StlZipContainerTypes, VectorAndSet)
{
  std::vector<int> v = {1, 2, 3};
  std::set<int> s = {100, 200, 300};
  
  int count = 0;
  for (auto [a, b] : stl_zip(v, s))
    {
      EXPECT_EQ(a, count + 1);
      ++count;
    }
  EXPECT_EQ(count, 3);
}

TEST(StlZipContainerTypes, VectorAndDeque)
{
  std::vector<int> v = {1, 2, 3};
  std::deque<std::string> d = {"x", "y", "z"};
  
  int count = 0;
  for (auto [num, str] : stl_zip(v, d))
    {
      ++count;
    }
  EXPECT_EQ(count, 3);
}

TEST(StlZipContainerTypes, StdArray)
{
  std::array<int, 3> a1 = {1, 2, 3};
  std::array<int, 3> a2 = {10, 20, 30};
  
  std::vector<int> products;
  for (auto [x, y] : stl_zip(a1, a2))
    products.push_back(x * y);
  
  EXPECT_EQ(products.size(), 3);
  EXPECT_EQ(products[0], 10);
  EXPECT_EQ(products[1], 40);
  EXPECT_EQ(products[2], 90);
}

TEST(StlZipContainerTypes, ManyContainers)
{
  std::vector<int> v1 = {1, 2};
  std::vector<int> v2 = {10, 20};
  std::vector<int> v3 = {100, 200};
  std::vector<int> v4 = {1000, 2000};
  
  int count = 0;
  for (auto [a, b, c, d] : stl_zip(v1, v2, v3, v4))
    {
      if (count == 0)
        {
          EXPECT_EQ(a, 1);
          EXPECT_EQ(b, 10);
          EXPECT_EQ(c, 100);
          EXPECT_EQ(d, 1000);
        }
      ++count;
    }
  EXPECT_EQ(count, 2);
}

//==============================================================================
// Functional Operations: stl_zip_all
//==============================================================================

TEST(StlZipAll, AllTrue)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_all([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, xs, ys);
  
  EXPECT_TRUE(result);
}

TEST(StlZipAll, SomeFalse)
{
  std::vector<int> xs = {1, 200, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_all([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, xs, ys);
  
  EXPECT_FALSE(result);
}

TEST(StlZipAll, EmptyContainers)
{
  std::vector<int> empty1, empty2;
  
  bool result = stl_zip_all([](auto) { return false; }, empty1, empty2);
  
  EXPECT_TRUE(result);  // Vacuously true
}

TEST(StlZipAll, EqualLengthCheck)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_all_eq([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, xs, ys);
  
  EXPECT_TRUE(result);
  
  // Different lengths
  std::vector<int> short_vec = {1, 2};
  result = stl_zip_all_eq([](auto) { return true; }, short_vec, ys);
  EXPECT_FALSE(result);  // Lengths differ
}

//==============================================================================
// Functional Operations: stl_zip_exists / stl_zip_any
//==============================================================================

TEST(StlZipExists, Found)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_exists([](auto t) {
    return std::get<0>(t) == 2;
  }, xs, ys);
  
  EXPECT_TRUE(result);
}

TEST(StlZipExists, NotFound)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_exists([](auto t) {
    return std::get<0>(t) == 99;
  }, xs, ys);
  
  EXPECT_FALSE(result);
}

TEST(StlZipExists, EmptyContainers)
{
  std::vector<int> empty1, empty2;
  
  bool result = stl_zip_exists([](auto) { return true; }, empty1, empty2);
  
  EXPECT_FALSE(result);
}

TEST(StlZipAny, AliasWorks)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_any([](auto t) {
    return std::get<0>(t) > 2;
  }, xs, ys);
  
  EXPECT_TRUE(result);
}

TEST(StlZipNone, AllFalse)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_none([](auto t) {
    return std::get<0>(t) > 100;
  }, xs, ys);
  
  EXPECT_TRUE(result);
}

//==============================================================================
// Functional Operations: stl_zip_for_each
//==============================================================================

TEST(StlZipForEach, BasicForEach)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  std::vector<int> sums;
  stl_zip_for_each([&sums](auto t) {
    sums.push_back(std::get<0>(t) + std::get<1>(t));
  }, xs, ys);
  
  EXPECT_EQ(sums.size(), 3);
  EXPECT_EQ(sums[0], 11);
  EXPECT_EQ(sums[1], 22);
  EXPECT_EQ(sums[2], 33);
}

TEST(StlZipForEach, WithIndex)
{
  std::vector<int> xs = {10, 20, 30};
  std::vector<std::string> ys = {"a", "b", "c"};
  
  std::vector<size_t> indices;
  stl_zip_for_each_indexed([&indices](size_t idx, auto) {
    indices.push_back(idx);
  }, xs, ys);
  
  EXPECT_EQ(indices.size(), 3);
  EXPECT_EQ(indices[0], 0);
  EXPECT_EQ(indices[1], 1);
  EXPECT_EQ(indices[2], 2);
}

//==============================================================================
// Functional Operations: stl_zip_foldl / stl_zip_reduce
//==============================================================================

TEST(StlZipFoldl, SumProducts)
{
  std::vector<int> prices = {10, 20, 30};
  std::vector<int> quantities = {2, 3, 1};
  
  int total = stl_zip_foldl(0, [](int acc, auto t) {
    return acc + std::get<0>(t) * std::get<1>(t);
  }, prices, quantities);
  
  // 10*2 + 20*3 + 30*1 = 20 + 60 + 30 = 110
  EXPECT_EQ(total, 110);
}

TEST(StlZipFoldl, ConcatStrings)
{
  std::vector<int> nums = {1, 2, 3};
  std::vector<std::string> strs = {"a", "b", "c"};
  
  std::string result = stl_zip_foldl(std::string(""), [](std::string acc, auto t) {
    return acc + std::to_string(std::get<0>(t)) + std::get<1>(t);
  }, nums, strs);
  
  EXPECT_EQ(result, "1a2b3c");
}

TEST(StlZipReduce, AliasWorks)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  int sum = stl_zip_reduce(0, [](int acc, auto t) {
    return acc + std::get<0>(t) + std::get<1>(t);
  }, xs, ys);
  
  EXPECT_EQ(sum, 66);  // 1+10 + 2+20 + 3+30 = 66
}

//==============================================================================
// Functional Operations: stl_zip_map
//==============================================================================

TEST(StlZipMap, BasicMap)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto sums = stl_zip_map([](auto t) {
    return std::get<0>(t) + std::get<1>(t);
  }, xs, ys);
  
  EXPECT_EQ(sums.size(), 3);
  EXPECT_EQ(sums[0], 11);
  EXPECT_EQ(sums[1], 22);
  EXPECT_EQ(sums[2], 33);
}

TEST(StlZipMap, DifferentReturnType)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto strs = stl_zip_map([](auto t) {
    return std::to_string(std::get<0>(t)) + "+" + std::to_string(std::get<1>(t));
  }, xs, ys);
  
  EXPECT_EQ(strs.size(), 3);
  EXPECT_EQ(strs[0], "1+10");
  EXPECT_EQ(strs[1], "2+20");
  EXPECT_EQ(strs[2], "3+30");
}

//==============================================================================
// Functional Operations: stl_zip_filter
//==============================================================================

TEST(StlZipFilter, BasicFilter)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  auto result = stl_zip_filter([](auto t) {
    return std::get<0>(t) % 2 == 0;  // Even xs
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 2);  // (2,20) and (4,40)
}

TEST(StlZipFilter, NoMatches)
{
  std::vector<int> xs = {1, 3, 5};
  std::vector<int> ys = {10, 30, 50};
  
  auto result = stl_zip_filter([](auto t) {
    return std::get<0>(t) % 2 == 0;  // No even numbers
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 0);
}

//==============================================================================
// Functional Operations: stl_zip_find_first
//==============================================================================

TEST(StlZipFindFirst, Found)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto result = stl_zip_find_first([](auto t) {
    return std::get<0>(t) == 3;
  }, xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 3);
  EXPECT_EQ(std::get<1>(*result), "c");
}

TEST(StlZipFindFirst, NotFound)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto result = stl_zip_find_first([](auto t) {
    return std::get<0>(t) == 99;
  }, xs, ys);
  
  EXPECT_FALSE(result.has_value());
}

//==============================================================================
// Functional Operations: stl_zip_count
//==============================================================================

TEST(StlZipCount, CountMatches)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  size_t count = stl_zip_count([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, xs, ys);
  
  EXPECT_EQ(count, 2);  // 2 and 4
}

TEST(StlZipLength, BasicLength)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  EXPECT_EQ(stl_zip_length(xs, ys), 5);
  
  std::vector<int> short_vec = {1, 2};
  EXPECT_EQ(stl_zip_length(short_vec, ys), 2);
}

TEST(StlZipEqualLength, EqualLengths)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  EXPECT_TRUE(stl_zip_equal_length(xs, ys));
}

TEST(StlZipEqualLength, DifferentLengths)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20};
  
  EXPECT_FALSE(stl_zip_equal_length(xs, ys));
}

//==============================================================================
// Functional Operations: stl_zip_nth
//==============================================================================

TEST(StlZipNth, ValidIndex)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto result = stl_zip_nth(2, xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 3);
  EXPECT_EQ(std::get<1>(*result), "c");
}

TEST(StlZipNth, InvalidIndex)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto result = stl_zip_nth(10, xs, ys);
  
  EXPECT_FALSE(result.has_value());
}

//==============================================================================
// Functional Operations: stl_zip_take / stl_zip_drop
//==============================================================================

TEST(StlZipTake, TakeFirst)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  auto result = stl_zip_take(3, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 1);
  EXPECT_EQ(std::get<0>(result[2]), 3);
}

TEST(StlZipTake, TakeMoreThanAvailable)
{
  std::vector<int> xs = {1, 2};
  std::vector<int> ys = {10, 20};
  
  auto result = stl_zip_take(10, xs, ys);
  
  EXPECT_EQ(result.size(), 2);
}

TEST(StlZipDrop, DropFirst)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  auto result = stl_zip_drop(2, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 3);
  EXPECT_EQ(std::get<0>(result[2]), 5);
}

TEST(StlZipDrop, DropAll)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto result = stl_zip_drop(10, xs, ys);
  
  EXPECT_EQ(result.size(), 0);
}

//==============================================================================
// Functional Operations: stl_zip_partition
//==============================================================================

TEST(StlZipPartition, BasicPartition)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto [evens, odds] = stl_zip_partition([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, xs, ys);
  
  EXPECT_EQ(evens.size(), 2);  // (2,"b"), (4,"d")
  EXPECT_EQ(odds.size(), 3);   // (1,"a"), (3,"c"), (5,"e")
}

//==============================================================================
// Functional Operations: stl_zip_to_vector
//==============================================================================

TEST(StlZipToVector, Materialize)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<std::string> ys = {"a", "b", "c"};
  
  auto result = stl_zip_to_vector(xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 1);
  EXPECT_EQ(std::get<1>(result[0]), "a");
}

//==============================================================================
// Traverse Operations
//==============================================================================

TEST(StlZipTraverse, AllTrue)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_traverse([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, xs, ys);
  
  EXPECT_TRUE(result);
}

TEST(StlZipTraverse, StopsEarly)
{
  std::vector<int> xs = {1, 200, 3};
  std::vector<int> ys = {10, 20, 30};
  
  int count = 0;
  bool result = stl_zip_traverse([&count](auto t) {
    ++count;
    return std::get<0>(t) < std::get<1>(t);
  }, xs, ys);
  
  EXPECT_FALSE(result);
  EXPECT_EQ(count, 2);  // Stopped at second element
}

//==============================================================================
// Comparison: stl_zip_cmp
//==============================================================================

TEST(StlZipCmp, AllEqual)
{
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 2, 3};
  std::vector<int> c = {1, 2, 3};
  
  bool result = stl_zip_cmp([](auto x, auto y) { return x == y; }, a, b, c);
  
  EXPECT_TRUE(result);
}

TEST(StlZipCmp, NotAllEqual)
{
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 99, 3};
  std::vector<int> c = {1, 2, 3};
  
  bool result = stl_zip_cmp([](auto x, auto y) { return x == y; }, a, b, c);
  
  EXPECT_FALSE(result);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST(StlZipEdgeCases, BothEmpty)
{
  std::vector<int> empty1, empty2;
  
  auto view = stl_zip(empty1, empty2);
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.size(), 0);
}

TEST(StlZipEdgeCases, OneEmpty)
{
  std::vector<int> empty;
  std::vector<int> nonempty = {1, 2, 3};
  
  auto view = stl_zip(empty, nonempty);
  EXPECT_TRUE(view.empty());
}

TEST(StlZipEdgeCases, VeryLongContainers)
{
  std::vector<int> xs(1000);
  std::vector<int> ys(1000);
  std::iota(xs.begin(), xs.end(), 0);
  std::iota(ys.begin(), ys.end(), 1000);
  
  int sum = stl_zip_foldl(0, [](int acc, auto t) {
    return acc + std::get<0>(t) + std::get<1>(t);
  }, xs, ys);
  
  // Sum of 0..999 + sum of 1000..1999
  // = 999*1000/2 + (1999*2000/2 - 999*1000/2)
  // = 499500 + 1499500 = 1999000
  EXPECT_EQ(sum, 1999000);
}

//==============================================================================
// Enumerate Tests
//==============================================================================

TEST(StlEnumerate, Basic)
{
  std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
  
  std::vector<size_t> indices;
  std::vector<std::string> collected_names;
  
  for (auto [idx, name] : stl_enumerate(names))
    {
      indices.push_back(idx);
      collected_names.push_back(name);
    }
  
  EXPECT_EQ(indices.size(), 3);
  EXPECT_EQ(indices[0], 0);
  EXPECT_EQ(indices[1], 1);
  EXPECT_EQ(indices[2], 2);
  EXPECT_EQ(collected_names[0], "Alice");
}

TEST(StlEnumerate, Empty)
{
  std::vector<int> empty;
  
  int count = 0;
  for (auto [idx, val] : stl_enumerate(empty))
    ++count;
  
  EXPECT_EQ(count, 0);
}

//==============================================================================
// Take While / Drop While Tests
//==============================================================================

TEST(StlZipTakeWhile, Basic)
{
  std::vector<int> xs = {1, 2, 3, 10, 4, 5};
  std::vector<int> ys = {10, 20, 30, 100, 40, 50};
  
  auto result = stl_zip_take_while([](auto t) {
    return std::get<0>(t) < 10;
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 1);
  EXPECT_EQ(std::get<0>(result[2]), 3);
}

TEST(StlZipTakeWhile, TakeNone)
{
  std::vector<int> xs = {10, 20, 30};
  std::vector<int> ys = {1, 2, 3};
  
  auto result = stl_zip_take_while([](auto t) {
    return std::get<0>(t) < 5;
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 0);
}

TEST(StlZipDropWhile, Basic)
{
  std::vector<int> xs = {1, 2, 3, 10, 4, 5};
  std::vector<int> ys = {10, 20, 30, 100, 40, 50};
  
  auto result = stl_zip_drop_while([](auto t) {
    return std::get<0>(t) < 10;
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 10);
}

TEST(StlZipDropWhile, DropAll)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto result = stl_zip_drop_while([](auto t) {
    return std::get<0>(t) < 100;
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 0);
}

//==============================================================================
// First / Last Tests
//==============================================================================

TEST(StlZipFirst, Basic)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<std::string> ys = {"a", "b", "c"};
  
  auto result = stl_zip_first(xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 1);
  EXPECT_EQ(std::get<1>(*result), "a");
}

TEST(StlZipFirst, Empty)
{
  std::vector<int> empty1, empty2;
  
  auto result = stl_zip_first(empty1, empty2);
  
  EXPECT_FALSE(result.has_value());
}

TEST(StlZipLast, Basic)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<std::string> ys = {"a", "b", "c"};
  
  auto result = stl_zip_last(xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 3);
  EXPECT_EQ(std::get<1>(*result), "c");
}

TEST(StlZipFindLast, Basic)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  auto result = stl_zip_find_last([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 4);  // Last even
}

TEST(StlZipFindIndex, Found)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<int> ys = {10, 20, 30, 40, 50};
  
  size_t idx = stl_zip_find_index([](auto t) {
    return std::get<0>(t) == 3;
  }, xs, ys);
  
  EXPECT_EQ(idx, 2);
}

TEST(StlZipFindIndex, NotFound)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  size_t idx = stl_zip_find_index([](auto t) {
    return std::get<0>(t) == 99;
  }, xs, ys);
  
  EXPECT_EQ(idx, 3);  // Returns total count when not found
}

//==============================================================================
// Unzip Tests
//==============================================================================

TEST(StlUnzip, Pairs)
{
  std::vector<std::pair<int, std::string>> pairs = {{1, "a"}, {2, "b"}, {3, "c"}};
  
  auto [nums, strs] = stl_unzip(pairs);
  
  EXPECT_EQ(nums.size(), 3);
  EXPECT_EQ(strs.size(), 3);
  EXPECT_EQ(nums[0], 1);
  EXPECT_EQ(strs[2], "c");
}

TEST(StlUnzipTuple, Basic)
{
  std::vector<std::tuple<int, double, char>> tuples = {
    {1, 1.1, 'a'}, {2, 2.2, 'b'}, {3, 3.3, 'c'}
  };
  
  auto [ints, doubles, chars] = stl_unzip_tuple(tuples);
  
  EXPECT_EQ(ints.size(), 3);
  EXPECT_EQ(doubles.size(), 3);
  EXPECT_EQ(chars.size(), 3);
  EXPECT_EQ(ints[1], 2);
  EXPECT_DOUBLE_EQ(doubles[1], 2.2);
  EXPECT_EQ(chars[1], 'b');
}

//==============================================================================
// Adjacent Tests
//==============================================================================

TEST(StlAdjacent, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  
  auto pairs = stl_adjacent(v);
  
  EXPECT_EQ(pairs.size(), 4);
  EXPECT_EQ(pairs[0].first, 1);
  EXPECT_EQ(pairs[0].second, 2);
  EXPECT_EQ(pairs[3].first, 4);
  EXPECT_EQ(pairs[3].second, 5);
}

TEST(StlAdjacent, SingleElement)
{
  std::vector<int> v = {42};
  
  auto pairs = stl_adjacent(v);
  
  EXPECT_EQ(pairs.size(), 0);
}

TEST(StlAdjacent, Empty)
{
  std::vector<int> v;
  
  auto pairs = stl_adjacent(v);
  
  EXPECT_EQ(pairs.size(), 0);
}

TEST(StlAdjacentMap, Differences)
{
  std::vector<int> v = {1, 3, 6, 10};
  
  auto diffs = stl_adjacent_map([](int a, int b) { return b - a; }, v);
  
  EXPECT_EQ(diffs.size(), 3);
  EXPECT_EQ(diffs[0], 2);
  EXPECT_EQ(diffs[1], 3);
  EXPECT_EQ(diffs[2], 4);
}

TEST(StlAdjacentAll, Sorted)
{
  std::vector<int> sorted = {1, 2, 3, 4, 5};
  std::vector<int> unsorted = {1, 3, 2, 4, 5};
  
  EXPECT_TRUE(stl_adjacent_all([](int a, int b) { return a < b; }, sorted));
  EXPECT_FALSE(stl_adjacent_all([](int a, int b) { return a < b; }, unsorted));
}

TEST(StlAdjacentExists, HasDuplicate)
{
  std::vector<int> with_dup = {1, 2, 2, 3};
  std::vector<int> no_dup = {1, 2, 3, 4};
  
  EXPECT_TRUE(stl_adjacent_exists([](int a, int b) { return a == b; }, with_dup));
  EXPECT_FALSE(stl_adjacent_exists([](int a, int b) { return a == b; }, no_dup));
}

//==============================================================================
// ML-style Operations Tests
//==============================================================================

TEST(StlZipMapi, Basic)
{
  std::vector<int> xs = {10, 20, 30};
  std::vector<int> ys = {1, 2, 3};
  
  auto results = stl_zip_mapi([](size_t i, auto t) {
    return std::to_string(i) + ":" + std::to_string(std::get<0>(t) + std::get<1>(t));
  }, xs, ys);
  
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results[0], "0:11");
  EXPECT_EQ(results[1], "1:22");
  EXPECT_EQ(results[2], "2:33");
}

TEST(StlZipFilteri, EvenIndices)
{
  std::vector<int> xs = {10, 20, 30, 40, 50};
  std::vector<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto evens = stl_zip_filteri([](size_t i, auto) {
    return i % 2 == 0;
  }, xs, ys);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(std::get<0>(evens[0]), 10);
  EXPECT_EQ(std::get<0>(evens[1]), 30);
  EXPECT_EQ(std::get<0>(evens[2]), 50);
}

TEST(StlZipScanLeft, RunningSum)
{
  std::vector<int> xs = {1, 2, 3, 4};
  std::vector<int> ys = {10, 20, 30, 40};
  
  auto sums = stl_zip_scan_left(0, [](int acc, auto t) {
    return acc + std::get<0>(t);
  }, xs, ys);
  
  EXPECT_EQ(sums.size(), 5);
  EXPECT_EQ(sums[0], 0);   // init
  EXPECT_EQ(sums[1], 1);   // 0 + 1
  EXPECT_EQ(sums[2], 3);   // 1 + 2
  EXPECT_EQ(sums[3], 6);   // 3 + 3
  EXPECT_EQ(sums[4], 10);  // 6 + 4
}

TEST(StlZipScanLeft, RunningProduct)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto products = stl_zip_scan_left(1, [](int acc, auto t) {
    return acc * std::get<0>(t) * std::get<1>(t);
  }, xs, ys);
  
  EXPECT_EQ(products.size(), 4);
  EXPECT_EQ(products[0], 1);      // init
  EXPECT_EQ(products[1], 10);     // 1 * 1 * 10
  EXPECT_EQ(products[2], 400);    // 10 * 2 * 20
  EXPECT_EQ(products[3], 36000);  // 400 * 3 * 30
}

TEST(StlZipFindMapi, Found)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  std::vector<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto result = stl_zip_find_mapi([](size_t i, auto t) -> std::optional<std::string> {
    if (std::get<0>(t) == 3)
      return "found at " + std::to_string(i);
    return std::nullopt;
  }, xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "found at 2");
}

TEST(StlZipFindMapi, NotFound)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  auto result = stl_zip_find_mapi([](size_t, auto t) -> std::optional<int> {
    if (std::get<0>(t) > 100)
      return std::get<0>(t);
    return std::nullopt;
  }, xs, ys);
  
  EXPECT_FALSE(result.has_value());
}

TEST(StlZipEqual, SameLength)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  EXPECT_TRUE(stl_zip_equal(xs, ys));
}

TEST(StlZipEqual, DifferentLength)
{
  std::vector<int> xs = {1, 2, 3, 4};
  std::vector<int> ys = {10, 20, 30};
  
  EXPECT_FALSE(stl_zip_equal(xs, ys));
}

TEST(StlZipEqualBy, AllMatch)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {10, 20, 30};
  
  bool result = stl_zip_equal_by([](auto t) {
    return std::get<0>(t) * 10 == std::get<1>(t);
  }, xs, ys);
  
  EXPECT_TRUE(result);
}

TEST(StlZipMem, Found)
{
  std::vector<int> xs = {1, 2, 3};
  std::vector<std::string> ys = {"a", "b", "c"};
  
  EXPECT_TRUE(stl_zip_mem(std::make_tuple(2, std::string("b")), xs, ys));
  EXPECT_FALSE(stl_zip_mem(std::make_tuple(2, std::string("x")), xs, ys));
}

TEST(StlZipAssoc, Found)
{
  std::vector<std::string> keys = {"one", "two", "three"};
  std::vector<int> values = {1, 2, 3};
  
  auto result = stl_zip_assoc(std::string("two"), keys, values);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), "two");
  EXPECT_EQ(std::get<1>(*result), 2);
}

TEST(StlZipAssoc, NotFound)
{
  std::vector<std::string> keys = {"one", "two", "three"};
  std::vector<int> values = {1, 2, 3};
  
  auto result = stl_zip_assoc(std::string("four"), keys, values);
  
  EXPECT_FALSE(result.has_value());
}

TEST(StlZipMin, Basic)
{
  std::vector<int> xs = {3, 1, 4, 1, 5};
  std::vector<int> ys = {30, 10, 40, 10, 50};
  
  auto result = stl_zip_min(xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 1);
}

TEST(StlZipMax, Basic)
{
  std::vector<int> xs = {3, 1, 4, 1, 5};
  std::vector<int> ys = {30, 10, 40, 10, 50};
  
  auto result = stl_zip_max(xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 5);
}

TEST(StlZipMinMax, Basic)
{
  std::vector<int> xs = {3, 1, 4, 1, 5};
  std::vector<int> ys = {30, 10, 40, 10, 50};
  
  auto result = stl_zip_min_max(xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(result->first), 1);   // min
  EXPECT_EQ(std::get<0>(result->second), 5);  // max
}

TEST(StlZipMinMax, Empty)
{
  std::vector<int> empty;
  std::vector<int> ys = {1, 2, 3};
  
  auto result = stl_zip_min_max(empty, ys);
  
  EXPECT_FALSE(result.has_value());
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
