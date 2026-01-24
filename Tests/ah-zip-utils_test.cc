
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file ah-zip-utils_test.cc
 * @brief Tests for Ah Zip Utils
 */

/**
 * Comprehensive test suite for ah-zip-utils.H
 * 
 * Tests unified zip operations that work with both STL and Aleph containers:
 * - STL-only combinations
 * - Aleph-only combinations  
 * - Mixed STL + Aleph combinations
 */

#include <gtest/gtest.h>
#include <ah-zip-utils.H>

// Aleph containers
#include <htlist.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynSetHash.H>
#include <tpl_arrayStack.H>
#include <tpl_dynListStack.H>
#include <tpl_arrayQueue.H>
#include <tpl_dynListQueue.H>

// STL containers
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <unordered_set>
#include <array>
#include <string>
#include <forward_list>

using namespace Aleph;

//==============================================================================
// STL-only Tests
//==============================================================================

TEST(UniZipStlOnly, TwoVectors)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<std::string> v2 = {"a", "b", "c"};
  
  int count = 0;
  for (auto [num, str] : uni_zip(v1, v2))
    {
      EXPECT_EQ(num, v1[count]);
      EXPECT_EQ(str, v2[count]);
      ++count;
    }
  EXPECT_EQ(count, 3);
}

TEST(UniZipStlOnly, VectorAndList)
{
  std::vector<int> v = {1, 2, 3};
  std::list<double> l = {1.1, 2.2, 3.3};
  
  auto sums = uni_zip_map([](auto t) {
    return std::get<0>(t) + std::get<1>(t);
  }, v, l);
  
  EXPECT_EQ(sums.size(), 3);
  EXPECT_DOUBLE_EQ(sums[0], 2.1);
}

TEST(UniZipStlOnly, ThreeContainers)
{
  std::vector<int> v1 = {1, 2};
  std::vector<int> v2 = {10, 20};
  std::vector<int> v3 = {100, 200};
  
  int count = 0;
  for (auto [a, b, c] : uni_zip(v1, v2, v3))
    {
      if (count == 0)
        {
          EXPECT_EQ(a, 1);
          EXPECT_EQ(b, 10);
          EXPECT_EQ(c, 100);
        }
      ++count;
    }
  EXPECT_EQ(count, 2);
}

//==============================================================================
// Aleph-only Tests
//==============================================================================

TEST(UniZipAlephOnly, TwoDynLists)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<std::string> l2 = {"a", "b", "c"};
  
  std::vector<int> nums;
  std::vector<std::string> strs;
  
  for (auto [num, str] : uni_zip(l1, l2))
    {
      nums.push_back(num);
      strs.push_back(str);
    }
  
  EXPECT_EQ(nums.size(), 3);
  EXPECT_EQ(nums[0], 1);
  EXPECT_EQ(strs[2], "c");
}

TEST(UniZipAlephOnly, DynListAndDynArray)
{
  DynList<int> list = {10, 20, 30};
  DynArray<std::string> arr;
  arr.append("x");
  arr.append("y");
  arr.append("z");
  
  int count = 0;
  for (auto [num, str] : uni_zip(list, arr))
    ++count;
  
  EXPECT_EQ(count, 3);
}

//==============================================================================
// Mixed STL + Aleph Tests
//==============================================================================

TEST(UniZipMixed, VectorAndDynList)
{
  std::vector<int> stl_vec = {1, 2, 3, 4, 5};
  DynList<std::string> aleph_list = {"a", "b", "c", "d", "e"};
  
  std::vector<std::string> results;
  for (auto [num, str] : uni_zip(stl_vec, aleph_list))
    results.push_back(std::to_string(num) + str);
  
  EXPECT_EQ(results.size(), 5);
  EXPECT_EQ(results[0], "1a");
  EXPECT_EQ(results[4], "5e");
}

TEST(UniZipMixed, DynListAndVector)
{
  DynList<int> aleph_list = {100, 200, 300};
  std::vector<double> stl_vec = {1.5, 2.5, 3.5};
  
  auto products = uni_zip_map([](auto t) {
    return std::get<0>(t) * std::get<1>(t);
  }, aleph_list, stl_vec);
  
  EXPECT_EQ(products.size(), 3);
  EXPECT_DOUBLE_EQ(products[0], 150.0);
  EXPECT_DOUBLE_EQ(products[1], 500.0);
}

TEST(UniZipMixed, ThreeMixedContainers)
{
  std::vector<int> stl_vec = {1, 2, 3};
  DynList<double> aleph_list = {1.1, 2.2, 3.3};
  std::list<std::string> stl_list = {"a", "b", "c"};
  
  int count = 0;
  for (auto [i, d, s] : uni_zip(stl_vec, aleph_list, stl_list))
    {
      EXPECT_EQ(i, count + 1);
      ++count;
    }
  EXPECT_EQ(count, 3);
}

TEST(UniZipMixed, DifferentLengths)
{
  std::vector<int> short_vec = {1, 2};
  DynList<std::string> long_list = {"a", "b", "c", "d", "e"};
  
  EXPECT_EQ(uni_zip_length(short_vec, long_list), 2);
}

//==============================================================================
// Functional Operations - Mixed
//==============================================================================

TEST(UniZipMixedOps, All)
{
  std::vector<int> stl_vec = {1, 2, 3};
  DynList<int> aleph_list = {10, 20, 30};
  
  bool result = uni_zip_all([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, stl_vec, aleph_list);
  
  EXPECT_TRUE(result);
}

TEST(UniZipMixedOps, Exists)
{
  std::vector<int> stl_vec = {1, 2, 3};
  DynList<std::string> aleph_list = {"short", "medium", "verylongstring"};
  
  bool result = uni_zip_exists([](auto t) {
    return std::get<1>(t).length() > 10;
  }, stl_vec, aleph_list);
  
  EXPECT_TRUE(result);
}

TEST(UniZipMixedOps, ForEach)
{
  std::vector<int> stl_vec = {1, 2, 3};
  DynList<int> aleph_list = {10, 20, 30};
  
  std::vector<int> sums;
  uni_zip_for_each([&sums](auto t) {
    sums.push_back(std::get<0>(t) + std::get<1>(t));
  }, stl_vec, aleph_list);
  
  EXPECT_EQ(sums.size(), 3);
  EXPECT_EQ(sums[0], 11);
  EXPECT_EQ(sums[2], 33);
}

TEST(UniZipMixedOps, Foldl)
{
  std::vector<int> prices = {10, 20, 30};
  DynList<int> quantities = {2, 3, 1};
  
  int total = uni_zip_foldl(0, [](int acc, auto t) {
    return acc + std::get<0>(t) * std::get<1>(t);
  }, prices, quantities);
  
  EXPECT_EQ(total, 110);  // 10*2 + 20*3 + 30*1
}

TEST(UniZipMixedOps, Map)
{
  std::vector<int> xs = {1, 2, 3};
  DynList<int> ys = {10, 20, 30};
  
  auto sums = uni_zip_map([](auto t) {
    return std::get<0>(t) + std::get<1>(t);
  }, xs, ys);
  
  EXPECT_EQ(sums.size(), 3);
  EXPECT_EQ(sums[0], 11);
}

TEST(UniZipMixedOps, Filter)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<int> ys = {10, 20, 30, 40, 50};
  
  auto evens = uni_zip_filter([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, xs, ys);
  
  EXPECT_EQ(evens.size(), 2);
}

TEST(UniZipMixedOps, FindFirst)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto result = uni_zip_find_first([](auto t) {
    return std::get<0>(t) == 3;
  }, xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 3);
  EXPECT_EQ(std::get<1>(*result), "c");
}

TEST(UniZipMixedOps, Count)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<int> ys = {10, 20, 30, 40, 50};
  
  size_t count = uni_zip_count([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, xs, ys);
  
  EXPECT_EQ(count, 2);
}

TEST(UniZipMixedOps, Nth)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto result = uni_zip_nth(2, xs, ys);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 3);
  EXPECT_EQ(std::get<1>(*result), "c");
}

TEST(UniZipMixedOps, Take)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<int> ys = {10, 20, 30, 40, 50};
  
  auto result = uni_zip_take(3, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[2]), 3);
}

TEST(UniZipMixedOps, Drop)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<int> ys = {10, 20, 30, 40, 50};
  
  auto result = uni_zip_drop(2, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 3);
}

TEST(UniZipMixedOps, TakeWhile)
{
  std::vector<int> xs = {1, 2, 3, 10, 4, 5};
  DynList<int> ys = {10, 20, 30, 100, 40, 50};
  
  auto result = uni_zip_take_while([](auto t) {
    return std::get<0>(t) < 10;
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
}

TEST(UniZipMixedOps, DropWhile)
{
  std::vector<int> xs = {1, 2, 3, 10, 4, 5};
  DynList<int> ys = {10, 20, 30, 100, 40, 50};
  
  auto result = uni_zip_drop_while([](auto t) {
    return std::get<0>(t) < 10;
  }, xs, ys);
  
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(std::get<0>(result[0]), 10);
}

TEST(UniZipMixedOps, FirstLast)
{
  std::vector<int> xs = {1, 2, 3};
  DynList<std::string> ys = {"a", "b", "c"};
  
  auto first = uni_zip_first(xs, ys);
  auto last = uni_zip_last(xs, ys);
  
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(last.has_value());
  
  EXPECT_EQ(std::get<0>(*first), 1);
  EXPECT_EQ(std::get<0>(*last), 3);
  EXPECT_EQ(std::get<1>(*last), "c");
}

TEST(UniZipMixedOps, Partition)
{
  std::vector<int> xs = {1, 2, 3, 4, 5};
  DynList<std::string> ys = {"a", "b", "c", "d", "e"};
  
  auto [evens, odds] = uni_zip_partition([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, xs, ys);
  
  EXPECT_EQ(evens.size(), 2);
  EXPECT_EQ(odds.size(), 3);
}

TEST(UniZipMixedOps, ToVector)
{
  std::vector<int> xs = {1, 2, 3};
  DynList<std::string> ys = {"a", "b", "c"};
  
  auto result = uni_zip_to_vector(xs, ys);
  
  EXPECT_EQ(result.size(), 3);
}

TEST(UniZipMixedOps, EqualLength)
{
  std::vector<int> xs = {1, 2, 3};
  DynList<int> ys_same = {10, 20, 30};
  DynList<int> ys_diff = {10, 20};
  
  EXPECT_TRUE(uni_zip_equal_length(xs, ys_same));
  EXPECT_FALSE(uni_zip_equal_length(xs, ys_diff));
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST(UniZipEdgeCases, EmptyContainers)
{
  std::vector<int> empty_stl;
  DynList<int> empty_aleph;
  
  auto view = uni_zip(empty_stl, empty_aleph);
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.size(), 0);
}

TEST(UniZipEdgeCases, OneEmptyMixed)
{
  std::vector<int> empty_stl;
  DynList<std::string> nonempty_aleph = {"a", "b", "c"};
  
  EXPECT_EQ(uni_zip_length(empty_stl, nonempty_aleph), 0);
}

TEST(UniZipEdgeCases, SingleElement)
{
  std::vector<int> single_stl = {42};
  DynList<std::string> single_aleph = {"answer"};
  
  auto result = uni_zip_first(single_stl, single_aleph);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 42);
  EXPECT_EQ(std::get<1>(*result), "answer");
}

//==============================================================================
// EXHAUSTIVE TESTS - All Container Combinations
//==============================================================================

// Helper to populate Aleph containers
template <typename T>
void populate_dynlist(DynList<T>& l, std::initializer_list<T> values)
{
  for (const auto& v : values)
    l.append(v);
}

template <typename T>
void populate_dyndlist(DynDlist<T>& l, std::initializer_list<T> values)
{
  for (const auto& v : values)
    l.append(v);
}

template <typename T>
void populate_dynarray(DynArray<T>& arr, std::initializer_list<T> values)
{
  for (const auto& v : values)
    arr.append(v);
}

//------------------------------------------------------------------------------
// Test: STL vector + All Aleph containers
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, VectorWithDynList)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {10, 20, 30, 40, 50};
  
  auto sums = uni_zip_map([](auto t) {
    return std::get<0>(t) + std::get<1>(t);
  }, stl, aleph);
  
  EXPECT_EQ(sums.size(), 5);
  EXPECT_EQ(sums[0], 11);
  EXPECT_EQ(sums[4], 55);
}

TEST(ExhaustiveMix, VectorWithDynDlist)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynDlist<int> aleph;
  populate_dyndlist(aleph, {10, 20, 30, 40, 50});
  
  int sum = uni_zip_foldl(0, [](int acc, auto t) {
    return acc + std::get<0>(t) * std::get<1>(t);
  }, stl, aleph);
  
  // 1*10 + 2*20 + 3*30 + 4*40 + 5*50 = 10+40+90+160+250 = 550
  EXPECT_EQ(sum, 550);
}

TEST(ExhaustiveMix, VectorWithDynArray)
{
  std::vector<std::string> stl = {"a", "b", "c"};
  DynArray<int> aleph;
  populate_dynarray(aleph, {1, 2, 3});
  
  auto results = uni_zip_map([](auto t) {
    return std::get<0>(t) + std::to_string(std::get<1>(t));
  }, stl, aleph);
  
  EXPECT_EQ(results[0], "a1");
  EXPECT_EQ(results[2], "c3");
}

TEST(ExhaustiveMix, VectorWithDynSetTree)
{
  std::vector<int> stl = {100, 200, 300};
  DynSetTree<int> aleph;
  aleph.insert(1);
  aleph.insert(2);
  aleph.insert(3);
  
  int count = 0;
  for (auto [s, a] : uni_zip(stl, aleph))
    {
      EXPECT_GT(s, a);  // STL values > Aleph values
      ++count;
    }
  EXPECT_EQ(count, 3);
}

//------------------------------------------------------------------------------
// Test: STL list + Aleph containers
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, ListWithDynList)
{
  std::list<double> stl = {1.1, 2.2, 3.3};
  DynList<double> aleph = {10.0, 20.0, 30.0};
  
  auto products = uni_zip_map([](auto t) {
    return std::get<0>(t) * std::get<1>(t);
  }, stl, aleph);
  
  EXPECT_EQ(products.size(), 3);
  EXPECT_DOUBLE_EQ(products[0], 11.0);
}

TEST(ExhaustiveMix, ListWithDynArray)
{
  std::list<int> stl = {1, 2, 3, 4};
  DynArray<std::string> aleph;
  aleph.append("one");
  aleph.append("two");
  aleph.append("three");
  aleph.append("four");
  
  std::vector<std::string> results;
  uni_zip_for_each([&results](auto t) {
    results.push_back(std::to_string(std::get<0>(t)) + "-" + std::get<1>(t));
  }, stl, aleph);
  
  EXPECT_EQ(results.size(), 4);
  EXPECT_EQ(results[0], "1-one");
  EXPECT_EQ(results[3], "4-four");
}

//------------------------------------------------------------------------------
// Test: STL deque + Aleph containers
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, DequeWithDynList)
{
  std::deque<int> stl = {5, 10, 15, 20};
  DynList<int> aleph = {1, 2, 3, 4};
  
  bool all_greater = uni_zip_all([](auto t) {
    return std::get<0>(t) > std::get<1>(t);
  }, stl, aleph);
  
  EXPECT_TRUE(all_greater);
}

TEST(ExhaustiveMix, DequeWithDynDlist)
{
  std::deque<char> stl = {'a', 'b', 'c'};
  DynDlist<int> aleph;
  populate_dyndlist(aleph, {1, 2, 3});
  
  auto results = uni_zip_map([](auto t) {
    return std::string(1, std::get<0>(t)) + std::to_string(std::get<1>(t));
  }, stl, aleph);
  
  EXPECT_EQ(results[0], "a1");
  EXPECT_EQ(results[2], "c3");
}

//------------------------------------------------------------------------------
// Test: STL set + Aleph containers
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, SetWithDynList)
{
  std::set<int> stl = {10, 20, 30};  // Ordered: 10, 20, 30
  DynList<std::string> aleph = {"x", "y", "z"};
  
  std::vector<int> nums;
  std::vector<std::string> strs;
  
  for (auto [n, s] : uni_zip(stl, aleph))
    {
      nums.push_back(n);
      strs.push_back(s);
    }
  
  EXPECT_EQ(nums.size(), 3);
  EXPECT_EQ(nums[0], 10);
  EXPECT_EQ(strs[2], "z");
}

TEST(ExhaustiveMix, SetWithDynArray)
{
  std::set<int> stl = {100, 200, 300};
  DynArray<double> aleph;
  populate_dynarray(aleph, {1.5, 2.5, 3.5});
  
  auto products = uni_zip_map([](auto t) {
    return std::get<0>(t) * std::get<1>(t);
  }, stl, aleph);
  
  EXPECT_DOUBLE_EQ(products[0], 150.0);
}

//------------------------------------------------------------------------------
// Test: STL array + Aleph containers
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, StdArrayWithDynList)
{
  std::array<int, 4> stl = {1, 2, 3, 4};
  DynList<int> aleph = {10, 20, 30, 40};
  
  int sum = uni_zip_foldl(0, [](int acc, auto t) {
    return acc + std::get<0>(t) + std::get<1>(t);
  }, stl, aleph);
  
  EXPECT_EQ(sum, 110);  // (1+10) + (2+20) + (3+30) + (4+40)
}

TEST(ExhaustiveMix, StdArrayWithDynDlist)
{
  std::array<std::string, 3> stl = {"hello", "world", "test"};
  DynDlist<int> aleph;
  populate_dyndlist(aleph, {1, 2, 3});
  
  auto results = uni_zip_map([](auto t) {
    return std::get<0>(t) + ":" + std::to_string(std::get<1>(t));
  }, stl, aleph);
  
  EXPECT_EQ(results[0], "hello:1");
  EXPECT_EQ(results[1], "world:2");
}

//------------------------------------------------------------------------------
// Test: Multiple containers (3+)
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, ThreeWayMix_VectorDynListDeque)
{
  std::vector<int> v = {1, 2, 3};
  DynList<int> l = {10, 20, 30};
  std::deque<int> d = {100, 200, 300};
  
  auto sums = uni_zip_map([](auto t) {
    return std::get<0>(t) + std::get<1>(t) + std::get<2>(t);
  }, v, l, d);
  
  EXPECT_EQ(sums[0], 111);
  EXPECT_EQ(sums[1], 222);
  EXPECT_EQ(sums[2], 333);
}

TEST(ExhaustiveMix, ThreeWayMix_DynListVectorDynArray)
{
  DynList<int> l = {1, 2, 3};
  std::vector<std::string> v = {"a", "b", "c"};
  DynArray<double> a;
  populate_dynarray(a, {1.1, 2.2, 3.3});
  
  int count = 0;
  for (auto [num, str, dbl] : uni_zip(l, v, a))
    {
      EXPECT_EQ(num, count + 1);
      ++count;
    }
  EXPECT_EQ(count, 3);
}

TEST(ExhaustiveMix, FourWayMix)
{
  std::vector<int> v1 = {1, 2};
  DynList<int> l = {10, 20};
  std::list<int> v2 = {100, 200};
  DynArray<int> a;
  populate_dynarray(a, {1000, 2000});
  
  auto sums = uni_zip_map([](auto t) {
    return std::get<0>(t) + std::get<1>(t) + std::get<2>(t) + std::get<3>(t);
  }, v1, l, v2, a);
  
  EXPECT_EQ(sums[0], 1111);
  EXPECT_EQ(sums[1], 2222);
}

//------------------------------------------------------------------------------
// Test: Aleph-only combinations
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, DynListWithDynDlist)
{
  DynList<int> l1 = {1, 2, 3};
  DynDlist<int> l2;
  populate_dyndlist(l2, {10, 20, 30});
  
  auto products = uni_zip_map([](auto t) {
    return std::get<0>(t) * std::get<1>(t);
  }, l1, l2);
  
  EXPECT_EQ(products[0], 10);
  EXPECT_EQ(products[1], 40);
  EXPECT_EQ(products[2], 90);
}

TEST(ExhaustiveMix, DynArrayWithDynSetTree)
{
  DynArray<std::string> arr;
  arr.append("first");
  arr.append("second");
  arr.append("third");
  
  DynSetTree<int> tree;
  tree.insert(1);
  tree.insert(2);
  tree.insert(3);
  
  std::vector<std::string> results;
  uni_zip_for_each([&results](auto t) {
    results.push_back(std::get<0>(t) + "=" + std::to_string(std::get<1>(t)));
  }, arr, tree);
  
  EXPECT_EQ(results.size(), 3);
}

TEST(ExhaustiveMix, ThreeAlephContainers)
{
  DynList<int> l = {1, 2, 3};
  DynDlist<int> d;
  populate_dyndlist(d, {10, 20, 30});
  DynArray<int> a;
  populate_dynarray(a, {100, 200, 300});
  
  int total = uni_zip_foldl(0, [](int acc, auto t) {
    return acc + std::get<0>(t) + std::get<1>(t) + std::get<2>(t);
  }, l, d, a);
  
  EXPECT_EQ(total, 666);  // (1+10+100) + (2+20+200) + (3+30+300)
}

//------------------------------------------------------------------------------
// Test: Different lengths (should stop at shortest)
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, DifferentLengths_StlLonger)
{
  std::vector<int> stl = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  DynList<int> aleph = {100, 200, 300};
  
  EXPECT_EQ(uni_zip_length(stl, aleph), 3);
}

TEST(ExhaustiveMix, DifferentLengths_AlephLonger)
{
  std::vector<int> stl = {1, 2};
  DynList<int> aleph = {10, 20, 30, 40, 50, 60, 70};
  
  EXPECT_EQ(uni_zip_length(stl, aleph), 2);
}

TEST(ExhaustiveMix, DifferentLengths_ThreeWay)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  DynList<int> l = {10, 20};
  std::deque<int> d = {100, 200, 300, 400};
  
  EXPECT_EQ(uni_zip_length(v, l, d), 2);  // Shortest is DynList with 2
}

//------------------------------------------------------------------------------
// Test: Functional operations with mixed containers
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, FilterMixed)
{
  std::vector<int> stl = {1, 2, 3, 4, 5, 6};
  DynList<int> aleph = {10, 20, 30, 40, 50, 60};
  
  // Filter where STL element is even
  auto evens = uni_zip_filter([](auto t) {
    return std::get<0>(t) % 2 == 0;
  }, stl, aleph);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(std::get<0>(evens[0]), 2);
  EXPECT_EQ(std::get<1>(evens[0]), 20);
}

TEST(ExhaustiveMix, PartitionMixed)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynArray<std::string> aleph;
  aleph.append("one");
  aleph.append("two");
  aleph.append("three");
  aleph.append("four");
  aleph.append("five");
  
  auto [odds, evens] = uni_zip_partition([](auto t) {
    return std::get<0>(t) % 2 == 1;
  }, stl, aleph);
  
  EXPECT_EQ(odds.size(), 3);   // 1, 3, 5
  EXPECT_EQ(evens.size(), 2);  // 2, 4
}

TEST(ExhaustiveMix, TakeDropMixed)
{
  DynList<int> aleph = {1, 2, 3, 4, 5};
  std::vector<std::string> stl = {"a", "b", "c", "d", "e"};
  
  auto first_three = uni_zip_take(3, aleph, stl);
  auto last_two = uni_zip_drop(3, aleph, stl);
  
  EXPECT_EQ(first_three.size(), 3);
  EXPECT_EQ(last_two.size(), 2);
  
  EXPECT_EQ(std::get<0>(first_three[0]), 1);
  EXPECT_EQ(std::get<0>(last_two[0]), 4);
}

TEST(ExhaustiveMix, TakeWhileDropWhileMixed)
{
  std::deque<int> stl = {1, 2, 3, 10, 11, 12};
  DynDlist<int> aleph;
  populate_dyndlist(aleph, {100, 200, 300, 1000, 1100, 1200});
  
  auto small = uni_zip_take_while([](auto t) {
    return std::get<0>(t) < 10;
  }, stl, aleph);
  
  auto large = uni_zip_drop_while([](auto t) {
    return std::get<0>(t) < 10;
  }, stl, aleph);
  
  EXPECT_EQ(small.size(), 3);
  EXPECT_EQ(large.size(), 3);
}

//------------------------------------------------------------------------------
// Test: String as container (sequence of chars)
//------------------------------------------------------------------------------

TEST(ExhaustiveMix, StringWithDynList)
{
  std::string stl = "ABC";
  DynList<int> aleph = {1, 2, 3};
  
  auto results = uni_zip_map([](auto t) {
    return std::string(1, std::get<0>(t)) + std::to_string(std::get<1>(t));
  }, stl, aleph);
  
  EXPECT_EQ(results[0], "A1");
  EXPECT_EQ(results[1], "B2");
  EXPECT_EQ(results[2], "C3");
}

//==============================================================================
// ML-style Operations Tests (Mixed STL + Aleph)
//==============================================================================

TEST(UniZipMlOps, Mapi)
{
  std::vector<int> stl = {10, 20, 30};
  DynList<int> aleph = {1, 2, 3};
  
  auto results = uni_zip_mapi([](size_t i, auto t) {
    return std::to_string(i) + ":" + std::to_string(std::get<0>(t) + std::get<1>(t));
  }, stl, aleph);
  
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results[0], "0:11");
  EXPECT_EQ(results[2], "2:33");
}

TEST(UniZipMlOps, Filteri)
{
  DynList<int> aleph = {10, 20, 30, 40, 50};
  std::vector<std::string> stl = {"a", "b", "c", "d", "e"};
  
  auto evens = uni_zip_filteri([](size_t i, auto) {
    return i % 2 == 0;
  }, aleph, stl);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(std::get<0>(evens[0]), 10);
  EXPECT_EQ(std::get<0>(evens[2]), 50);
}

TEST(UniZipMlOps, ScanLeft)
{
  std::vector<int> stl = {1, 2, 3, 4};
  DynList<int> aleph = {10, 20, 30, 40};
  
  auto sums = uni_zip_scan_left(0, [](int acc, auto t) {
    return acc + std::get<0>(t);
  }, stl, aleph);
  
  EXPECT_EQ(sums.size(), 5);
  EXPECT_EQ(sums[0], 0);   // init
  EXPECT_EQ(sums[1], 1);   // 0 + 1
  EXPECT_EQ(sums[2], 3);   // 1 + 2
  EXPECT_EQ(sums[3], 6);   // 3 + 3
  EXPECT_EQ(sums[4], 10);  // 6 + 4
}

TEST(UniZipMlOps, FindMapi)
{
  DynList<int> aleph = {1, 2, 3, 4, 5};
  std::vector<std::string> stl = {"a", "b", "c", "d", "e"};
  
  auto result = uni_zip_find_mapi([](size_t i, auto t) -> std::optional<std::string> {
    if (std::get<0>(t) == 3)
      return "found at " + std::to_string(i);
    return std::nullopt;
  }, aleph, stl);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "found at 2");
}

TEST(UniZipMlOps, EqualBy)
{
  std::vector<int> stl = {1, 2, 3};
  DynList<int> aleph = {10, 20, 30};
  
  bool result = uni_zip_equal_by([](auto t) {
    return std::get<0>(t) * 10 == std::get<1>(t);
  }, stl, aleph);
  
  EXPECT_TRUE(result);
}

TEST(UniZipMlOps, Mem)
{
  std::vector<int> stl = {1, 2, 3};
  DynList<std::string> aleph = {"a", "b", "c"};
  
  EXPECT_TRUE(uni_zip_mem(std::make_tuple(2, std::string("b")), stl, aleph));
  EXPECT_FALSE(uni_zip_mem(std::make_tuple(2, std::string("x")), stl, aleph));
}

TEST(UniZipMlOps, Assoc)
{
  DynList<std::string> keys = {"one", "two", "three"};
  std::vector<int> values = {1, 2, 3};
  
  auto result = uni_zip_assoc(std::string("two"), keys, values);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), "two");
  EXPECT_EQ(std::get<1>(*result), 2);
}

TEST(UniZipMlOps, Min)
{
  std::vector<int> stl = {3, 1, 4, 1, 5};
  DynList<int> aleph = {30, 10, 40, 10, 50};
  
  auto result = uni_zip_min(stl, aleph);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 1);
}

TEST(UniZipMlOps, Max)
{
  std::vector<int> stl = {3, 1, 4, 1, 5};
  DynList<int> aleph = {30, 10, 40, 10, 50};
  
  auto result = uni_zip_max(stl, aleph);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(*result), 5);
}

TEST(UniZipMlOps, MinMax)
{
  DynList<int> aleph = {3, 1, 4, 1, 5};
  std::vector<int> stl = {30, 10, 40, 10, 50};
  
  auto result = uni_zip_min_max(aleph, stl);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<0>(result->first), 1);   // min
  EXPECT_EQ(std::get<0>(result->second), 5);  // max
}

//==============================================================================
// Unzip Operations Tests
//==============================================================================

TEST(UniUnzip, StlVectorOfPairs)
{
  std::vector<std::pair<int, std::string>> pairs = {
    {1, "a"}, {2, "b"}, {3, "c"}
  };
  
  auto [nums, strs] = uni_unzip(pairs);
  
  EXPECT_EQ(nums.size(), 3);
  EXPECT_EQ(strs.size(), 3);
  
  auto num_it = nums.get_it();
  EXPECT_EQ(num_it.get_curr(), 1);
  num_it.next();
  EXPECT_EQ(num_it.get_curr(), 2);
  
  auto str_it = strs.get_it();
  EXPECT_EQ(str_it.get_curr(), "a");
}

TEST(UniUnzip, AlephDynListOfPairs)
{
  DynList<std::pair<int, double>> pairs;
  pairs.append({1, 1.1});
  pairs.append({2, 2.2});
  pairs.append({3, 3.3});
  
  auto [ints, doubles] = uni_unzip(pairs);
  
  EXPECT_EQ(ints.size(), 3);
  EXPECT_EQ(doubles.size(), 3);
}

TEST(UniUnzipTuple, StlVectorOfTuples)
{
  std::vector<std::tuple<int, double, char>> tuples = {
    {1, 1.1, 'a'},
    {2, 2.2, 'b'},
    {3, 3.3, 'c'}
  };
  
  auto [ints, doubles, chars] = uni_unzip_tuple(tuples);
  
  EXPECT_EQ(ints.size(), 3);
  EXPECT_EQ(doubles.size(), 3);
  EXPECT_EQ(chars.size(), 3);
  
  auto int_it = ints.get_it();
  EXPECT_EQ(int_it.get_curr(), 1);
  
  auto char_it = chars.get_it();
  EXPECT_EQ(char_it.get_curr(), 'a');
}

TEST(UniUnzipTuple, AlephDynListOfTuples)
{
  DynList<std::tuple<int, std::string>> tuples;
  tuples.append({1, "one"});
  tuples.append({2, "two"});
  tuples.append({3, "three"});
  
  auto [ints, strs] = uni_unzip_tuple(tuples);
  
  EXPECT_EQ(ints.size(), 3);
  EXPECT_EQ(strs.size(), 3);
}

TEST(UniZipToDynlist, Basic)
{
  std::vector<int> stl = {1, 2, 3};
  DynList<std::string> aleph = {"a", "b", "c"};

  auto result = uni_zip_to_dynlist(stl, aleph);

  EXPECT_EQ(result.size(), 3);

  auto it = result.get_it();
  EXPECT_EQ(std::get<0>(it.get_curr()), 1);
  EXPECT_EQ(std::get<1>(it.get_curr()), "a");
}

//==============================================================================
// Bug Fix Tests - Stateful Callables
//==============================================================================

// Test that stateful callables work correctly (std::forward bug fix)
struct StatefulCounter
{
  mutable int count = 0;
  bool operator()(auto) const { ++count; return true; }
};

struct StatefulAccumulator
{
  mutable int sum = 0;
  void operator()(auto t) const { sum += std::get<0>(t); }
};

TEST(UniZipStatefulCallable, ForEachPreservesState)
{
  std::vector<int> v1 = {1, 2, 3, 4, 5};
  std::vector<int> v2 = {10, 20, 30, 40, 50};

  StatefulAccumulator acc;
  uni_zip_for_each(std::ref(acc), v1, v2);

  EXPECT_EQ(acc.sum, 15);  // 1+2+3+4+5
}

TEST(UniZipStatefulCallable, AllPreservesState)
{
  std::vector<int> v1 = {1, 2, 3, 4, 5};
  std::vector<int> v2 = {10, 20, 30, 40, 50};

  StatefulCounter counter;
  uni_zip_all(std::ref(counter), v1, v2);

  EXPECT_EQ(counter.count, 5);  // Called 5 times
}

TEST(UniZipStatefulCallable, MapPreservesState)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};

  int call_count = 0;
  auto result = uni_zip_map([&call_count](auto t) {
    ++call_count;
    return std::get<0>(t) + std::get<1>(t);
  }, v1, v2);

  EXPECT_EQ(call_count, 3);
  EXPECT_EQ(result.size(), 3);
}

TEST(UniZipStatefulCallable, FilterPreservesState)
{
  std::vector<int> v1 = {1, 2, 3, 4, 5};
  std::vector<int> v2 = {10, 20, 30, 40, 50};

  int call_count = 0;
  auto result = uni_zip_filter([&call_count](auto t) {
    ++call_count;
    return std::get<0>(t) % 2 == 0;
  }, v1, v2);

  EXPECT_EQ(call_count, 5);  // Predicate called for all elements
  EXPECT_EQ(result.size(), 2);  // Only evens pass
}

TEST(UniZipStatefulCallable, FoldlPreservesState)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};

  int call_count = 0;
  auto result = uni_zip_foldl(0, [&call_count](int acc, auto t) {
    ++call_count;
    return acc + std::get<0>(t);
  }, v1, v2);

  EXPECT_EQ(call_count, 3);
  EXPECT_EQ(result, 6);  // 1+2+3
}

//==============================================================================
// Bug Fix Tests - Equal Length Semantics
//==============================================================================

TEST(UniZipEqualLength, AllCompletedCheck)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};
  std::vector<int> v3 = {100, 200};  // Shorter

  // Same length containers
  EXPECT_TRUE(uni_zip_equal_length(v1, v2));

  // Different length containers
  EXPECT_FALSE(uni_zip_equal_length(v1, v3));
  EXPECT_FALSE(uni_zip_equal_length(v2, v3));
}

TEST(UniZipEqualLength, AllEqSemanticsCorrect)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};
  std::vector<int> v3 = {100, 200};  // Shorter

  // All true + equal length = true
  EXPECT_TRUE(uni_zip_all_eq([](auto) { return true; }, v1, v2));

  // All true but different length = false
  EXPECT_FALSE(uni_zip_all_eq([](auto) { return true; }, v1, v3));

  // One false + equal length = false
  EXPECT_FALSE(uni_zip_all_eq([](auto t) {
    return std::get<0>(t) != 2;
  }, v1, v2));
}

TEST(UniZipEqualLength, EqualByWithLengthCheck)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};
  std::vector<int> v3 = {10, 20};  // Shorter

  // Condition passes, equal length
  EXPECT_TRUE(uni_zip_equal_by([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, v1, v2));

  // Condition passes but different length
  EXPECT_FALSE(uni_zip_equal_by([](auto t) {
    return std::get<0>(t) < std::get<1>(t);
  }, v1, v3));
}

//==============================================================================
// Bug Fix Tests - Sentinel End() O(1)
//==============================================================================

TEST(UniZipSentinel, EndIsO1)
{
  std::vector<int> v1 = {1, 2, 3, 4, 5};
  std::vector<int> v2 = {10, 20, 30, 40, 50};

  auto view = uni_zip(v1, v2);

  // end() should return sentinel immediately (O(1))
  auto sentinel = view.end();
  (void)sentinel;  // Just verify it compiles and returns

  // Iteration should work
  int count = 0;
  for (auto [a, b] : view)
    {
      (void)a; (void)b;
      ++count;
    }
  EXPECT_EQ(count, 5);
}

TEST(UniZipSentinel, IteratorSentinelComparison)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};

  auto view = uni_zip(v1, v2);
  auto it = view.begin();
  auto end = view.end();

  // Iterator not at end initially
  EXPECT_NE(it, end);
  EXPECT_TRUE(it.has_curr());

  // Advance to end
  ++it; ++it; ++it;

  // Now at end
  EXPECT_EQ(it, end);
  EXPECT_FALSE(it.has_curr());
}

//==============================================================================
// New API Tests - any_has_curr and all_completed
//==============================================================================

TEST(UniZipIteratorAPI, AnyHasCurrVsHasCurr)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20};  // Shorter

  auto it = uni_zip_it(v1, v2);

  // Both have elements initially
  EXPECT_TRUE(it.has_curr());
  EXPECT_TRUE(it.any_has_curr());

  it.next();
  it.next();

  // v2 exhausted, v1 still has one more
  EXPECT_FALSE(it.has_curr());      // Not ALL have elements
  EXPECT_TRUE(it.any_has_curr());   // But SOME still have

  // all_completed is false because v1 still has elements
  EXPECT_FALSE(it.all_completed());
}

TEST(UniZipIteratorAPI, AllCompletedVsCompleted)
{
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {10, 20, 30};

  auto it = uni_zip_it(v1, v2);

  // Exhaust iterator
  while (it.has_curr())
    it.next();

  // Both containers exhausted at same time
  EXPECT_TRUE(it.all_completed());
  EXPECT_TRUE(it.completed());  // Backward compatible
}

//==============================================================================
// Edge Cases for Fixes
//==============================================================================

TEST(UniZipEdgeCases, EmptyContainersWithCallables)
{
  std::vector<int> empty1;
  std::vector<int> empty2;

  // Should handle empty gracefully with stateful callables
  int call_count = 0;
  uni_zip_for_each([&call_count](auto) { ++call_count; }, empty1, empty2);
  EXPECT_EQ(call_count, 0);

  auto result = uni_zip_map([&call_count](auto t) {
    ++call_count;
    return std::get<0>(t);
  }, empty1, empty2);
  EXPECT_EQ(call_count, 0);
  EXPECT_TRUE(result.empty());
}

TEST(UniZipEdgeCases, SingleElementWithStatefulCallable)
{
  std::vector<int> v1 = {42};
  std::vector<std::string> v2 = {"answer"};

  int call_count = 0;
  auto result = uni_zip_map([&call_count](auto t) {
    ++call_count;
    return std::to_string(std::get<0>(t)) + ":" + std::get<1>(t);
  }, v1, v2);

  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "42:answer");
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
