
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
 * @file ranges_iterator_test.cc
 * @brief Tests for Ranges Iterator
 */

/**
 * @file ranges_iterator_test.cc
 * @brief Test that Aleph iterators satisfy std::ranges::input_iterator
 *
 * This test verifies that after the fix to make operator*() const,
 * Aleph containers can be used with std::ranges algorithms like:
 * - std::ranges::all_of
 * - std::ranges::any_of
 * - std::ranges::find
 * - std::ranges::find_if
 * - std::ranges::count
 * - std::ranges::count_if
 * - std::ranges::min_element
 * - std::ranges::max_element
 * - std::ranges::for_each
 */

#include <gtest/gtest.h>
#include <algorithm>

#include <ah-ranges.H>
#include <tpl_dynDlist.H>
#include <tpl_dynArray.H>
#include <tpl_dynSetTree.H>

using namespace Aleph;

#if ALEPH_HAS_RANGES

//============================================================================
// Concept Verification Tests
//============================================================================

// Test that iterator types satisfy the required concepts
TEST(RangesIteratorConcepts, DynDlistIteratorSatisfiesInputIterator)
{
  using Iter = DynDlist<int>::iterator;

  // Check individual concept requirements
  static_assert(std::movable<Iter>, "Iterator must be movable");
  static_assert(std::copyable<Iter>, "Iterator must be copyable");
  static_assert(std::default_initializable<Iter>, "Iterator must be default initializable");
  static_assert(std::weakly_incrementable<Iter>, "Iterator must be weakly incrementable");

  // Check that dereferencing a const iterator works
  static_assert(requires(const Iter it) { *it; },
                "Must be able to dereference const iterator");

  // The main requirement: iter_reference_t must be same for const and non-const
  static_assert(std::same_as<std::iter_reference_t<Iter>,
                             std::iter_reference_t<const Iter>>,
                "iter_reference_t<I> must equal iter_reference_t<const I>");

  // Check indirectly_readable (this is what was failing before)
  static_assert(std::indirectly_readable<Iter>,
                "Iterator must satisfy indirectly_readable");

  // Check input_iterator
  static_assert(std::input_iterator<Iter>,
                "Iterator must satisfy input_iterator");

  // Check forward_iterator (Aleph claims forward_iterator_tag)
  static_assert(std::forward_iterator<Iter>,
                "Iterator must satisfy forward_iterator");

  SUCCEED();
}

TEST(RangesIteratorConcepts, DynDlistConstIteratorSatisfiesInputIterator)
{
  using Iter = DynDlist<int>::const_iterator;

  static_assert(std::indirectly_readable<Iter>,
                "Const iterator must satisfy indirectly_readable");
  static_assert(std::input_iterator<Iter>,
                "Const iterator must satisfy input_iterator");
  static_assert(std::forward_iterator<Iter>,
                "Const iterator must satisfy forward_iterator");

  SUCCEED();
}

TEST(RangesIteratorConcepts, DynArrayIteratorSatisfiesInputIterator)
{
  using Iter = DynArray<int>::iterator;

  static_assert(std::same_as<std::iter_reference_t<Iter>,
                             std::iter_reference_t<const Iter>>,
                "iter_reference_t<I> must equal iter_reference_t<const I>");
  static_assert(std::indirectly_readable<Iter>,
                "Iterator must satisfy indirectly_readable");
  static_assert(std::input_iterator<Iter>,
                "Iterator must satisfy input_iterator");

  SUCCEED();
}

TEST(RangesIteratorConcepts, DynSetTreeIteratorSatisfiesInputIterator)
{
  using Set = DynSetTree<int>;
  using Iter = Set::iterator;

  static_assert(std::same_as<std::iter_reference_t<Iter>,
                             std::iter_reference_t<const Iter>>,
                "iter_reference_t<I> must equal iter_reference_t<const I>");
  static_assert(std::indirectly_readable<Iter>,
                "Iterator must satisfy indirectly_readable");
  static_assert(std::input_iterator<Iter>,
                "Iterator must satisfy input_iterator");

  SUCCEED();
}

//============================================================================
// Range Concept Tests
//============================================================================

TEST(RangesIteratorConcepts, DynDlistSatisfiesRangeConcept)
{
  static_assert(std::ranges::range<DynDlist<int>>,
                "DynDlist must satisfy std::ranges::range");
  static_assert(std::ranges::input_range<DynDlist<int>>,
                "DynDlist must satisfy std::ranges::input_range");
  static_assert(std::ranges::forward_range<DynDlist<int>>,
                "DynDlist must satisfy std::ranges::forward_range");

  SUCCEED();
}

TEST(RangesIteratorConcepts, ConstDynDlistSatisfiesRangeConcept)
{
  static_assert(std::ranges::range<const DynDlist<int>>,
                "const DynDlist must satisfy std::ranges::range");
  static_assert(std::ranges::input_range<const DynDlist<int>>,
                "const DynDlist must satisfy std::ranges::input_range");

  SUCCEED();
}

//============================================================================
// Functional Tests with std::ranges Algorithms
//============================================================================

class RangesAlgorithmTest : public ::testing::Test
{
protected:
  DynDlist<int> list;

  void SetUp() override
  {
    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);
    list.append(5);
  }
};

TEST_F(RangesAlgorithmTest, AllOf)
{
  EXPECT_TRUE(std::ranges::all_of(list, [](int x) { return x > 0; }));
  EXPECT_FALSE(std::ranges::all_of(list, [](int x) { return x > 3; }));
}

TEST_F(RangesAlgorithmTest, AnyOf)
{
  EXPECT_TRUE(std::ranges::any_of(list, [](int x) { return x == 3; }));
  EXPECT_FALSE(std::ranges::any_of(list, [](int x) { return x == 10; }));
}

TEST_F(RangesAlgorithmTest, NoneOf)
{
  EXPECT_TRUE(std::ranges::none_of(list, [](int x) { return x < 0; }));
  EXPECT_FALSE(std::ranges::none_of(list, [](int x) { return x == 3; }));
}

TEST_F(RangesAlgorithmTest, Find)
{
  auto it = std::ranges::find(list, 3);
  EXPECT_NE(it, list.end());
  EXPECT_EQ(*it, 3);

  auto not_found = std::ranges::find(list, 10);
  EXPECT_EQ(not_found, list.end());
}

TEST_F(RangesAlgorithmTest, FindIf)
{
  auto it = std::ranges::find_if(list, [](int x) { return x % 2 == 0; });
  EXPECT_NE(it, list.end());
  EXPECT_EQ(*it, 2);  // First even number
}

TEST_F(RangesAlgorithmTest, Count)
{
  list.append(3);  // Add another 3
  auto count = std::ranges::count(list, 3);
  EXPECT_EQ(count, 2);
}

TEST_F(RangesAlgorithmTest, CountIf)
{
  auto count = std::ranges::count_if(list, [](int x) { return x % 2 == 0; });
  EXPECT_EQ(count, 2);  // 2 and 4
}

TEST_F(RangesAlgorithmTest, MinElement)
{
  auto it = std::ranges::min_element(list);
  EXPECT_NE(it, list.end());
  EXPECT_EQ(*it, 1);
}

TEST_F(RangesAlgorithmTest, MaxElement)
{
  auto it = std::ranges::max_element(list);
  EXPECT_NE(it, list.end());
  EXPECT_EQ(*it, 5);
}

TEST_F(RangesAlgorithmTest, ForEach)
{
  int sum = 0;
  std::ranges::for_each(list, [&sum](int x) { sum += x; });
  EXPECT_EQ(sum, 15);  // 1+2+3+4+5
}

TEST_F(RangesAlgorithmTest, ForEachWithProjection)
{
  int sum = 0;
  std::ranges::for_each(list, [&sum](int x) { sum += x; },
                        [](int x) { return x * 2; });  // Project: double each
  EXPECT_EQ(sum, 30);  // (1+2+3+4+5)*2
}

//============================================================================
// Tests with const containers
//============================================================================

TEST(RangesConstContainerTest, AllOfWithConstContainer)
{
  DynDlist<int> mutable_list;
  mutable_list.append(1);
  mutable_list.append(2);
  mutable_list.append(3);

  const DynDlist<int>& const_list = mutable_list;

  EXPECT_TRUE(std::ranges::all_of(const_list, [](int x) { return x > 0; }));
}

TEST(RangesConstContainerTest, FindWithConstContainer)
{
  DynDlist<int> mutable_list;
  mutable_list.append(1);
  mutable_list.append(2);
  mutable_list.append(3);

  const DynDlist<int>& const_list = mutable_list;

  auto it = std::ranges::find(const_list, 2);
  EXPECT_NE(it, const_list.end());
  EXPECT_EQ(*it, 2);
}

//============================================================================
// Tests with range adaptors (views)
//============================================================================

TEST_F(RangesAlgorithmTest, FilterView)
{
  auto evens = list | std::views::filter([](int x) { return x % 2 == 0; });

  std::vector<int> result;
  for (int x : evens)
    result.push_back(x);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 4);
}

TEST_F(RangesAlgorithmTest, TransformView)
{
  auto doubled = list | std::views::transform([](int x) { return x * 2; });

  std::vector<int> result;
  for (int x : doubled)
    result.push_back(x);

  ASSERT_EQ(result.size(), 5u);
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 4);
  EXPECT_EQ(result[2], 6);
  EXPECT_EQ(result[3], 8);
  EXPECT_EQ(result[4], 10);
}

TEST_F(RangesAlgorithmTest, TakeView)
{
  auto first_three = list | std::views::take(3);

  std::vector<int> result;
  for (int x : first_three)
    result.push_back(x);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST_F(RangesAlgorithmTest, ChainedViews)
{
  // Filter evens, then double them
  auto result_view = list
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * 2; });

  std::vector<int> result;
  for (int x : result_view)
    result.push_back(x);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], 4);   // 2 * 2
  EXPECT_EQ(result[1], 8);   // 4 * 2
}

//============================================================================
// Tests with DynArray
//============================================================================

TEST(RangesDynArrayTest, BasicAlgorithms)
{
  DynArray<int> arr;
  arr.append(10);
  arr.append(20);
  arr.append(30);

  EXPECT_TRUE(std::ranges::all_of(arr, [](int x) { return x >= 10; }));

  auto it = std::ranges::find(arr, 20);
  EXPECT_NE(it, arr.end());
  EXPECT_EQ(*it, 20);

  auto max_it = std::ranges::max_element(arr);
  EXPECT_EQ(*max_it, 30);
}

//============================================================================
// Tests with DynSetTree
//============================================================================

TEST(RangesDynSetTreeTest, BasicAlgorithms)
{
  DynSetTree<int> set;
  set.insert(5);
  set.insert(2);
  set.insert(8);
  set.insert(1);
  set.insert(9);

  // All elements positive
  EXPECT_TRUE(std::ranges::all_of(set, [](int x) { return x > 0; }));

  // Find element
  auto it = std::ranges::find(set, 5);
  EXPECT_NE(it, set.end());
  EXPECT_EQ(*it, 5);

  // Count elements > 5
  auto count = std::ranges::count_if(set, [](int x) { return x > 5; });
  EXPECT_EQ(count, 2);  // 8 and 9

  // Min element (sets are ordered)
  auto min_it = std::ranges::min_element(set);
  EXPECT_EQ(*min_it, 1);
}

#else // !ALEPH_HAS_RANGES

// Dummy test when ranges are not available
TEST(RangesIterator, RangesNotAvailable)
{
  GTEST_SKIP() << "std::ranges not fully supported on this platform";
}

#endif // ALEPH_HAS_RANGES

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}