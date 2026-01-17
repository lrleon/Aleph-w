
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
 * @file ah-uni-functional_test.cc
 * @brief Tests for Ah Uni Functional
 */

/**
 * Comprehensive test suite for ah-uni-functional.H
 * 
 * Tests unified functional programming operations that work with both
 * STL and Aleph containers.
 */

#include <gtest/gtest.h>
#include <ah-uni-functional.H>

// Aleph containers
#include <htlist.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>

// STL containers
#include <vector>
#include <list>
#include <deque>
#include <string>

using namespace Aleph;

//==============================================================================
// STL Container Tests
//==============================================================================

TEST(UniMapStl, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto squares = uni_map([](int x) { return x * x; }, v);
  
  EXPECT_EQ(squares.size(), 5);
  EXPECT_EQ(squares[0], 1);
  EXPECT_EQ(squares[4], 25);
}

TEST(UniFilterStl, Basic)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  auto evens = uni_filter([](int x) { return x % 2 == 0; }, v);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(evens[0], 2);
  EXPECT_EQ(evens[2], 6);
}

TEST(UniFoldlStl, Sum)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  int sum = uni_foldl(0, [](int acc, int x) { return acc + x; }, v);
  EXPECT_EQ(sum, 15);
}

TEST(UniAllStl, AllTrue)
{
  std::vector<int> v = {2, 4, 6, 8};
  EXPECT_TRUE(uni_all([](int x) { return x % 2 == 0; }, v));
}

TEST(UniExistsStl, SomeTrue)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_TRUE(uni_exists([](int x) { return x == 3; }, v));
}

//==============================================================================
// Aleph Container Tests
//==============================================================================

TEST(UniMapAleph, DynList)
{
  DynList<int> l = {1, 2, 3, 4, 5};
  auto squares = uni_map([](int x) { return x * x; }, l);
  
  EXPECT_EQ(squares.size(), 5);
  EXPECT_EQ(squares[0], 1);
  EXPECT_EQ(squares[4], 25);
}

TEST(UniFilterAleph, DynList)
{
  DynList<int> l = {1, 2, 3, 4, 5, 6};
  auto evens = uni_filter([](int x) { return x % 2 == 0; }, l);
  
  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(evens[0], 2);
  EXPECT_EQ(evens[2], 6);
}

TEST(UniFoldlAleph, Sum)
{
  DynList<int> l = {1, 2, 3, 4, 5};
  int sum = uni_foldl(0, [](int acc, int x) { return acc + x; }, l);
  EXPECT_EQ(sum, 15);
}

TEST(UniAllAleph, AllTrue)
{
  DynList<int> l = {2, 4, 6, 8};
  EXPECT_TRUE(uni_all([](int x) { return x % 2 == 0; }, l));
}

TEST(UniExistsAleph, SomeTrue)
{
  DynList<int> l = {1, 2, 3, 4, 5};
  EXPECT_TRUE(uni_exists([](int x) { return x == 3; }, l));
}

TEST(UniMapAleph, DynArray)
{
  DynArray<int> arr;
  arr.append(1);
  arr.append(2);
  arr.append(3);
  
  auto squares = uni_map([](int x) { return x * x; }, arr);
  
  EXPECT_EQ(squares.size(), 3);
  EXPECT_EQ(squares[0], 1);
  EXPECT_EQ(squares[2], 9);
}

TEST(UniFilterAleph, DynDlist)
{
  DynDlist<int> l;
  l.append(1);
  l.append(2);
  l.append(3);
  l.append(4);
  l.append(5);
  l.append(6);
  
  auto evens = uni_filter([](int x) { return x % 2 == 0; }, l);
  
  EXPECT_EQ(evens.size(), 3);
}

//==============================================================================
// Same Function Works on Both - Comparison Tests
//==============================================================================

TEST(UniMapMixed, SameResultBothContainerTypes)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  auto stl_result = uni_map([](int x) { return x * 2; }, stl);
  auto aleph_result = uni_map([](int x) { return x * 2; }, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
}

TEST(UniFilterMixed, SameResultBothContainerTypes)
{
  std::vector<int> stl = {1, 2, 3, 4, 5, 6};
  DynList<int> aleph = {1, 2, 3, 4, 5, 6};
  
  auto stl_result = uni_filter([](int x) { return x > 3; }, stl);
  auto aleph_result = uni_filter([](int x) { return x > 3; }, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
}

TEST(UniFoldlMixed, SameResultBothContainerTypes)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  int stl_sum = uni_foldl(0, [](int acc, int x) { return acc + x; }, stl);
  int aleph_sum = uni_foldl(0, [](int acc, int x) { return acc + x; }, aleph);
  
  EXPECT_EQ(stl_sum, aleph_sum);
  EXPECT_EQ(stl_sum, 15);
}

//==============================================================================
// ML-style Operations Tests
//==============================================================================

TEST(UniMapi, StlAndAleph)
{
  std::vector<std::string> stl = {"a", "b", "c"};
  DynList<std::string> aleph = {"a", "b", "c"};
  
  auto stl_result = uni_mapi([](size_t i, const std::string& s) {
    return std::to_string(i) + s;
  }, stl);
  
  auto aleph_result = uni_mapi([](size_t i, const std::string& s) {
    return std::to_string(i) + s;
  }, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
  EXPECT_EQ(stl_result[0], "0a");
  EXPECT_EQ(stl_result[2], "2c");
}

TEST(UniFilteri, EvenIndices)
{
  std::vector<int> stl = {10, 20, 30, 40, 50};
  DynList<int> aleph = {10, 20, 30, 40, 50};
  
  auto stl_evens = uni_filteri([](size_t i, int) { return i % 2 == 0; }, stl);
  auto aleph_evens = uni_filteri([](size_t i, int) { return i % 2 == 0; }, aleph);
  
  EXPECT_EQ(stl_evens, aleph_evens);
  EXPECT_EQ(stl_evens.size(), 3);
}

TEST(UniScanLeft, RunningSum)
{
  std::vector<int> stl = {1, 2, 3, 4};
  DynList<int> aleph = {1, 2, 3, 4};
  
  auto stl_sums = uni_scan_left(0, [](int acc, int x) { return acc + x; }, stl);
  auto aleph_sums = uni_scan_left(0, [](int acc, int x) { return acc + x; }, aleph);
  
  EXPECT_EQ(stl_sums, aleph_sums);
  EXPECT_EQ(stl_sums.size(), 5);
  EXPECT_EQ(stl_sums[0], 0);
  EXPECT_EQ(stl_sums[4], 10);
}

//==============================================================================
// Find Operations Tests
//==============================================================================

TEST(UniFind, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  auto stl_result = uni_find([](int x) { return x > 3; }, stl);
  auto aleph_result = uni_find([](int x) { return x > 3; }, aleph);
  
  ASSERT_TRUE(stl_result.has_value());
  ASSERT_TRUE(aleph_result.has_value());
  EXPECT_EQ(*stl_result, *aleph_result);
  EXPECT_EQ(*stl_result, 4);
}

TEST(UniFindIndex, StlAndAleph)
{
  std::vector<std::string> stl = {"a", "b", "c", "d"};
  DynList<std::string> aleph = {"a", "b", "c", "d"};
  
  auto stl_idx = uni_find_index([](const std::string& s) { return s == "c"; }, stl);
  auto aleph_idx = uni_find_index([](const std::string& s) { return s == "c"; }, aleph);
  
  ASSERT_TRUE(stl_idx.has_value());
  ASSERT_TRUE(aleph_idx.has_value());
  EXPECT_EQ(*stl_idx, *aleph_idx);
  EXPECT_EQ(*stl_idx, 2);
}

TEST(UniMem, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  EXPECT_TRUE(uni_mem(3, stl));
  EXPECT_TRUE(uni_mem(3, aleph));
  EXPECT_FALSE(uni_mem(10, stl));
  EXPECT_FALSE(uni_mem(10, aleph));
}

//==============================================================================
// Count and Length Tests
//==============================================================================

TEST(UniCount, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5, 6};
  DynList<int> aleph = {1, 2, 3, 4, 5, 6};
  
  size_t stl_count = uni_count([](int x) { return x % 2 == 0; }, stl);
  size_t aleph_count = uni_count([](int x) { return x % 2 == 0; }, aleph);
  
  EXPECT_EQ(stl_count, aleph_count);
  EXPECT_EQ(stl_count, 3);
}

TEST(UniLength, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  EXPECT_EQ(uni_length(stl), uni_length(aleph));
  EXPECT_EQ(uni_length(stl), 5);
}

//==============================================================================
// Take and Drop Tests
//==============================================================================

TEST(UniTake, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  auto stl_result = uni_take(3, stl);
  auto aleph_result = uni_take(3, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
  EXPECT_EQ(stl_result.size(), 3);
  EXPECT_EQ(stl_result[2], 3);
}

TEST(UniDrop, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  auto stl_result = uni_drop(2, stl);
  auto aleph_result = uni_drop(2, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
  EXPECT_EQ(stl_result.size(), 3);
  EXPECT_EQ(stl_result[0], 3);
}

TEST(UniTakeWhile, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 10, 4, 5};
  DynList<int> aleph = {1, 2, 3, 10, 4, 5};
  
  auto stl_result = uni_take_while([](int x) { return x < 10; }, stl);
  auto aleph_result = uni_take_while([](int x) { return x < 10; }, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
  EXPECT_EQ(stl_result.size(), 3);
}

TEST(UniDropWhile, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 10, 4, 5};
  DynList<int> aleph = {1, 2, 3, 10, 4, 5};
  
  auto stl_result = uni_drop_while([](int x) { return x < 10; }, stl);
  auto aleph_result = uni_drop_while([](int x) { return x < 10; }, aleph);
  
  EXPECT_EQ(stl_result, aleph_result);
  EXPECT_EQ(stl_result.size(), 3);
  EXPECT_EQ(stl_result[0], 10);
}

//==============================================================================
// Access Tests
//==============================================================================

TEST(UniFirst, StlAndAleph)
{
  std::vector<int> stl = {10, 20, 30};
  DynList<int> aleph = {10, 20, 30};
  
  auto stl_first = uni_first(stl);
  auto aleph_first = uni_first(aleph);
  
  ASSERT_TRUE(stl_first.has_value());
  ASSERT_TRUE(aleph_first.has_value());
  EXPECT_EQ(*stl_first, *aleph_first);
  EXPECT_EQ(*stl_first, 10);
}

TEST(UniLast, StlAndAleph)
{
  std::vector<int> stl = {10, 20, 30};
  DynList<int> aleph = {10, 20, 30};
  
  auto stl_last = uni_last(stl);
  auto aleph_last = uni_last(aleph);
  
  ASSERT_TRUE(stl_last.has_value());
  ASSERT_TRUE(aleph_last.has_value());
  EXPECT_EQ(*stl_last, *aleph_last);
  EXPECT_EQ(*stl_last, 30);
}

TEST(UniNth, StlAndAleph)
{
  std::vector<int> stl = {10, 20, 30, 40, 50};
  DynList<int> aleph = {10, 20, 30, 40, 50};
  
  auto stl_nth = uni_nth(2, stl);
  auto aleph_nth = uni_nth(2, aleph);
  
  ASSERT_TRUE(stl_nth.has_value());
  ASSERT_TRUE(aleph_nth.has_value());
  EXPECT_EQ(*stl_nth, *aleph_nth);
  EXPECT_EQ(*stl_nth, 30);
}

//==============================================================================
// Min/Max Tests
//==============================================================================

TEST(UniMin, StlAndAleph)
{
  std::vector<int> stl = {3, 1, 4, 1, 5, 9};
  DynList<int> aleph = {3, 1, 4, 1, 5, 9};
  
  auto stl_min = uni_min(stl);
  auto aleph_min = uni_min(aleph);
  
  ASSERT_TRUE(stl_min.has_value());
  ASSERT_TRUE(aleph_min.has_value());
  EXPECT_EQ(*stl_min, *aleph_min);
  EXPECT_EQ(*stl_min, 1);
}

TEST(UniMax, StlAndAleph)
{
  std::vector<int> stl = {3, 1, 4, 1, 5, 9};
  DynList<int> aleph = {3, 1, 4, 1, 5, 9};
  
  auto stl_max = uni_max(stl);
  auto aleph_max = uni_max(aleph);
  
  ASSERT_TRUE(stl_max.has_value());
  ASSERT_TRUE(aleph_max.has_value());
  EXPECT_EQ(*stl_max, *aleph_max);
  EXPECT_EQ(*stl_max, 9);
}

TEST(UniMinMax, StlAndAleph)
{
  std::vector<int> stl = {3, 1, 4, 1, 5, 9};
  DynList<int> aleph = {3, 1, 4, 1, 5, 9};
  
  auto stl_mm = uni_min_max(stl);
  auto aleph_mm = uni_min_max(aleph);
  
  ASSERT_TRUE(stl_mm.has_value());
  ASSERT_TRUE(aleph_mm.has_value());
  EXPECT_EQ(stl_mm->first, aleph_mm->first);
  EXPECT_EQ(stl_mm->second, aleph_mm->second);
  EXPECT_EQ(stl_mm->first, 1);
  EXPECT_EQ(stl_mm->second, 9);
}

//==============================================================================
// Sum and Product Tests
//==============================================================================

TEST(UniSum, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5};
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  EXPECT_EQ(uni_sum(stl), uni_sum(aleph));
  EXPECT_EQ(uni_sum(stl), 15);
}

TEST(UniProduct, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4};
  DynList<int> aleph = {1, 2, 3, 4};
  
  EXPECT_EQ(uni_product(stl), uni_product(aleph));
  EXPECT_EQ(uni_product(stl), 24);
}

//==============================================================================
// Partition Tests
//==============================================================================

TEST(UniPartition, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3, 4, 5, 6};
  DynList<int> aleph = {1, 2, 3, 4, 5, 6};
  
  auto [stl_evens, stl_odds] = uni_partition([](int x) { return x % 2 == 0; }, stl);
  auto [aleph_evens, aleph_odds] = uni_partition([](int x) { return x % 2 == 0; }, aleph);
  
  EXPECT_EQ(stl_evens, aleph_evens);
  EXPECT_EQ(stl_odds, aleph_odds);
  EXPECT_EQ(stl_evens.size(), 3);
  EXPECT_EQ(stl_odds.size(), 3);
}

//==============================================================================
// Conversion Tests
//==============================================================================

TEST(UniToVector, AlephToStl)
{
  DynList<int> aleph = {1, 2, 3, 4, 5};
  
  auto vec = uni_to_vector(aleph);
  
  EXPECT_EQ(vec.size(), 5);
  EXPECT_EQ(vec[0], 1);
  EXPECT_EQ(vec[4], 5);
}

//==============================================================================
// Comparison Tests
//==============================================================================

TEST(UniEqual, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3};
  DynList<int> aleph = {1, 2, 3};
  DynList<int> aleph_diff = {1, 2, 4};
  
  EXPECT_TRUE(uni_equal(stl, aleph));
  EXPECT_FALSE(uni_equal(stl, aleph_diff));
}

TEST(UniCompare, StlAndAleph)
{
  std::vector<int> stl = {1, 2, 3};
  DynList<int> aleph_equal = {1, 2, 3};
  DynList<int> aleph_less = {1, 2, 2};
  DynList<int> aleph_greater = {1, 2, 4};
  
  EXPECT_EQ(uni_compare(stl, aleph_equal), 0);
  EXPECT_EQ(uni_compare(stl, aleph_less), 1);
  EXPECT_EQ(uni_compare(stl, aleph_greater), -1);
}

//==============================================================================
// Works with Different STL Container Types
//==============================================================================

TEST(UniMixed, WorksWithList)
{
  std::list<int> l = {1, 2, 3, 4, 5};
  
  auto squares = uni_map([](int x) { return x * x; }, l);
  EXPECT_EQ(squares.size(), 5);
  EXPECT_EQ(squares[4], 25);
}

TEST(UniMixed, WorksWithDeque)
{
  std::deque<int> d = {1, 2, 3, 4, 5, 6};
  
  auto evens = uni_filter([](int x) { return x % 2 == 0; }, d);
  EXPECT_EQ(evens.size(), 3);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST(UniEdgeCases, EmptyStl)
{
  std::vector<int> empty;
  
  auto mapped = uni_map([](int x) { return x * 2; }, empty);
  EXPECT_TRUE(mapped.empty());
  
  auto first = uni_first(empty);
  EXPECT_FALSE(first.has_value());
  
  int sum = uni_foldl(0, std::plus<int>{}, empty);
  EXPECT_EQ(sum, 0);
}

TEST(UniEdgeCases, EmptyAleph)
{
  DynList<int> empty;
  
  auto mapped = uni_map([](int x) { return x * 2; }, empty);
  EXPECT_TRUE(mapped.empty());
  
  auto first = uni_first(empty);
  EXPECT_FALSE(first.has_value());
  
  int sum = uni_foldl(0, std::plus<int>{}, empty);
  EXPECT_EQ(sum, 0);
}

TEST(UniEdgeCases, SingleElement)
{
  std::vector<int> stl = {42};
  DynList<int> aleph = {42};
  
  EXPECT_EQ(*uni_first(stl), *uni_first(aleph));
  EXPECT_EQ(*uni_last(stl), *uni_last(aleph));
  EXPECT_EQ(*uni_min(stl), *uni_min(aleph));
  EXPECT_EQ(*uni_max(stl), *uni_max(aleph));
}

// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
