/**
 * @file ahAlgo.cc
 * @brief Comprehensive test suite for ahAlgo.H
 *
 * Tests all STL-like algorithm implementations in Aleph-w.
 */

#include <gtest/gtest.h>
#include <vector>

#include <ahAlgo.H>
#include <tpl_dynDlist.H>
#include <tpl_array.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Test Suite: for_each (using std::vector for STL-compatible iterators)
// ============================================================================

TEST(ForEach, SumElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  int sum = 0;
  Aleph::for_each(v.begin(), v.end(), [&sum](int x) { sum += x; });
  EXPECT_EQ(sum, 15);
}

TEST(ForEach, ModifyElements)
{
  vector<int> v = {1, 2, 3};
  
  Aleph::for_each(v.begin(), v.end(), [](int& x) { x *= 2; });
  
  EXPECT_EQ(v[0], 2);
  EXPECT_EQ(v[1], 4);
  EXPECT_EQ(v[2], 6);
}

TEST(ForEach, EmptyRange)
{
  vector<int> empty;
  int count = 0;
  Aleph::for_each(empty.begin(), empty.end(), [&count](int) { ++count; });
  EXPECT_EQ(count, 0);
}

// ============================================================================
// Test Suite: count / count_if
// ============================================================================

TEST(CountIf, CountEvenNumbers)
{
  vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
  auto n = Aleph::count_if(v.begin(), v.end(), 
                            [](int x) { return x % 2 == 0; });
  EXPECT_EQ(n, 4);
}

TEST(CountIf, CountNone)
{
  vector<int> v = {1, 3, 5, 7, 9};
  auto n = Aleph::count_if(v.begin(), v.end(), 
                            [](int x) { return x % 2 == 0; });
  EXPECT_EQ(n, 0);
}

TEST(CountIf, CountAll)
{
  vector<int> v = {2, 4, 6, 8};
  auto n = Aleph::count_if(v.begin(), v.end(), 
                            [](int x) { return x % 2 == 0; });
  EXPECT_EQ(n, 4);
}

TEST(Count, CountValue)
{
  vector<int> v = {1, 2, 3, 2, 4, 2, 5};
  auto n = Aleph::count(v.begin(), v.end(), 2);
  EXPECT_EQ(n, 3);
}

TEST(Count, CountMissing)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto n = Aleph::count(v.begin(), v.end(), 10);
  EXPECT_EQ(n, 0);
}

// ============================================================================
// Test Suite: min_element / max_element
// ============================================================================

TEST(MinElement, FindMinimum)
{
  vector<int> v = {5, 2, 8, 1, 9};
  
  auto it = Aleph::min_element(v.begin(), v.end());
  EXPECT_EQ(*it, 1);
}

TEST(MinElement, EmptyRange)
{
  vector<int> empty;
  auto it = Aleph::min_element(empty.begin(), empty.end());
  EXPECT_EQ(it, empty.end());
}

TEST(MaxElement, FindMaximum)
{
  vector<int> v = {5, 2, 8, 1, 9};
  
  auto it = Aleph::max_element(v.begin(), v.end());
  EXPECT_EQ(*it, 9);
}

// ============================================================================
// Test Suite: find / find_if
// ============================================================================

TEST(FindIf, FindFirstEven)
{
  vector<int> v = {1, 3, 5, 4, 7, 8};
  auto it = Aleph::find_if(v.begin(), v.end(), 
                            [](int x) { return x % 2 == 0; });
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 4);
}

TEST(FindIf, FindNothing)
{
  vector<int> v = {1, 3, 5, 7, 9};
  auto it = Aleph::find_if(v.begin(), v.end(), 
                            [](int x) { return x % 2 == 0; });
  EXPECT_EQ(it, v.end());
}

TEST(Find, FindValue)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto it = Aleph::find(v.begin(), v.end(), 3);
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 3);
}

TEST(Find, FindMissing)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto it = Aleph::find(v.begin(), v.end(), 10);
  EXPECT_EQ(it, v.end());
}

// ============================================================================
// Test Suite: search_n
// ============================================================================

TEST(SearchN, FindConsecutive)
{
  vector<int> v = {1, 2, 2, 2, 3, 4};
  auto it = Aleph::search_n(v.begin(), v.end(), 3, 2);
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 2);
}

TEST(SearchN, NotEnoughConsecutive)
{
  vector<int> v = {1, 2, 2, 3, 2, 2, 4};
  auto it = Aleph::search_n(v.begin(), v.end(), 3, 2);
  EXPECT_EQ(it, v.end());
}

TEST(SearchN, ZeroCount)
{
  vector<int> v = {1, 2, 3};
  auto it = Aleph::search_n(v.begin(), v.end(), 0, 2);
  EXPECT_EQ(it, v.end());
}

// ============================================================================
// Test Suite: adjacent_find
// ============================================================================

TEST(AdjacentFind, FindDuplicate)
{
  vector<int> v = {1, 2, 3, 3, 4, 5};
  auto it = Aleph::adjacent_find(v.begin(), v.end());
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 3);
}

TEST(AdjacentFind, NoDuplicate)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto it = Aleph::adjacent_find(v.begin(), v.end());
  EXPECT_EQ(it, v.end());
}

TEST(AdjacentFind, EmptyRange)
{
  vector<int> empty;
  auto it = Aleph::adjacent_find(empty.begin(), empty.end());
  EXPECT_EQ(it, empty.end());
}

// ============================================================================
// Test Suite: equal
// ============================================================================

TEST(Equal, EqualRanges)
{
  vector<int> v1 = {1, 2, 3, 4, 5};
  vector<int> v2 = {1, 2, 3, 4, 5};
  EXPECT_TRUE(Aleph::equal(v1.begin(), v1.end(), v2.begin()));
}

TEST(Equal, UnequalRanges)
{
  vector<int> v1 = {1, 2, 3, 4, 5};
  vector<int> v2 = {1, 2, 3, 4, 6};
  EXPECT_FALSE(Aleph::equal(v1.begin(), v1.end(), v2.begin()));
}

// ============================================================================
// Test Suite: lexicographical_compare
// ============================================================================

TEST(LexCompare, LessThan)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {1, 2, 4};
  EXPECT_TRUE(Aleph::lexicographical_compare(
    v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(LexCompare, GreaterThan)
{
  vector<int> v1 = {1, 2, 4};
  vector<int> v2 = {1, 2, 3};
  EXPECT_FALSE(Aleph::lexicographical_compare(
    v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(LexCompare, PrefixLessThan)
{
  vector<int> v1 = {1, 2};
  vector<int> v2 = {1, 2, 3};
  EXPECT_TRUE(Aleph::lexicographical_compare(
    v1.begin(), v1.end(), v2.begin(), v2.end()));
}

// ============================================================================
// Test Suite: copy / copy_backward
// ============================================================================

TEST(Copy, BasicCopy)
{
  vector<int> src = {1, 2, 3};
  vector<int> dst(3, 0);
  
  Aleph::copy(src.begin(), src.end(), dst.begin());
  
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 2);
  EXPECT_EQ(dst[2], 3);
}

TEST(CopyBackward, BasicCopyBackward)
{
  vector<int> src = {1, 2, 3};
  vector<int> dst(3, 0);
  
  Aleph::copy_backward(src.begin(), src.end(), dst.end());
  
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 2);
  EXPECT_EQ(dst[2], 3);
}

// ============================================================================
// Test Suite: transform
// ============================================================================

TEST(Transform, UnaryTransform)
{
  vector<int> src = {1, 2, 3};
  vector<int> dst(3, 0);
  
  Aleph::transform(src.begin(), src.end(), dst.begin(),
                    [](int x) { return x * 2; });
  
  EXPECT_EQ(dst[0], 2);
  EXPECT_EQ(dst[1], 4);
  EXPECT_EQ(dst[2], 6);
}

TEST(Transform, BinaryTransform)
{
  vector<int> src1 = {1, 2, 3};
  vector<int> src2 = {10, 20, 30};
  vector<int> dst(3, 0);
  
  Aleph::transform(src1.begin(), src1.end(), src2.begin(), dst.begin(),
                    [](int a, int b) { return a + b; });
  
  EXPECT_EQ(dst[0], 11);
  EXPECT_EQ(dst[1], 22);
  EXPECT_EQ(dst[2], 33);
}

// ============================================================================
// Test Suite: swap_ranges
// ============================================================================

TEST(SwapRanges, BasicSwap)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {4, 5, 6};
  
  Aleph::swap_ranges(v1.begin(), v1.end(), v2.begin());
  
  EXPECT_EQ(v1[0], 4);
  EXPECT_EQ(v1[1], 5);
  EXPECT_EQ(v1[2], 6);
  EXPECT_EQ(v2[0], 1);
  EXPECT_EQ(v2[1], 2);
  EXPECT_EQ(v2[2], 3);
}

// ============================================================================
// Test Suite: fill / fill_n
// ============================================================================

TEST(Fill, FillRange)
{
  vector<int> v = {1, 2, 3};
  
  Aleph::fill(v.begin(), v.end(), 42);
  
  EXPECT_EQ(v[0], 42);
  EXPECT_EQ(v[1], 42);
  EXPECT_EQ(v[2], 42);
}

TEST(FillN, FillNElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  
  Aleph::fill_n(v.begin(), 3, 99);
  
  EXPECT_EQ(v[0], 99);
  EXPECT_EQ(v[1], 99);
  EXPECT_EQ(v[2], 99);
  EXPECT_EQ(v[3], 4);  // unchanged
  EXPECT_EQ(v[4], 5);  // unchanged
}

// ============================================================================
// Test Suite: generate / generate_n
// ============================================================================

TEST(Generate, GenerateSequence)
{
  vector<int> v(3, 0);
  
  int counter = 0;
  Aleph::generate(v.begin(), v.end(), [&counter]() { return ++counter; });
  
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 2);
  EXPECT_EQ(v[2], 3);
}

TEST(GenerateN, GenerateNElements)
{
  vector<int> v(5, 0);
  
  int counter = 10;
  Aleph::generate_n(v.begin(), 3, [&counter]() { return counter++; });
  
  EXPECT_EQ(v[0], 10);
  EXPECT_EQ(v[1], 11);
  EXPECT_EQ(v[2], 12);
  EXPECT_EQ(v[3], 0);  // unchanged
}

// ============================================================================
// Test Suite: replace / replace_if
// ============================================================================

TEST(ReplaceIf, ReplaceEvenNumbers)
{
  vector<int> v = {1, 2, 3, 4, 5};
  
  Aleph::replace_if(v.begin(), v.end(),
                     [](int x) { return x % 2 == 0; }, 0);
  
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 0);
  EXPECT_EQ(v[2], 3);
  EXPECT_EQ(v[3], 0);
  EXPECT_EQ(v[4], 5);
}

TEST(Replace, ReplaceValue)
{
  vector<int> v = {1, 2, 1, 3, 1};
  
  Aleph::replace(v.begin(), v.end(), 1, 99);
  
  EXPECT_EQ(v[0], 99);
  EXPECT_EQ(v[1], 2);
  EXPECT_EQ(v[2], 99);
  EXPECT_EQ(v[3], 3);
  EXPECT_EQ(v[4], 99);
}

// ============================================================================
// Test Suite: replace_copy_if
// ============================================================================

TEST(ReplaceCopyIf, BasicReplaceCopy)
{
  vector<int> src = {1, 2, 3, 4, 5};
  vector<int> dst(5, 0);
  
  Aleph::replace_copy_if(src.begin(), src.end(), dst.begin(),
                          [](int x) { return x % 2 == 0; }, 0);
  
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 0);
  EXPECT_EQ(dst[2], 3);
  EXPECT_EQ(dst[3], 0);
  EXPECT_EQ(dst[4], 5);
}

// ============================================================================
// Test Suite: reverse
// ============================================================================

TEST(Reverse, ReverseElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  
  Aleph::reverse(v.begin(), v.end());
  
  EXPECT_EQ(v[0], 5);
  EXPECT_EQ(v[1], 4);
  EXPECT_EQ(v[2], 3);
  EXPECT_EQ(v[3], 2);
  EXPECT_EQ(v[4], 1);
}

TEST(Reverse, ReverseSingleElement)
{
  vector<int> v = {42};
  
  Aleph::reverse(v.begin(), v.end());
  
  EXPECT_EQ(v[0], 42);
}

TEST(Reverse, ReverseEmpty)
{
  vector<int> v;
  EXPECT_NO_THROW(Aleph::reverse(v.begin(), v.end()));
}

// ============================================================================
// Test Suite: reverse_copy
// ============================================================================

TEST(ReverseCopy, BasicReverseCopy)
{
  vector<int> src = {1, 2, 3, 4, 5};
  vector<int> dst(5, 0);
  
  Aleph::reverse_copy(src.begin(), src.end(), dst.begin());
  
  EXPECT_EQ(dst[0], 5);
  EXPECT_EQ(dst[1], 4);
  EXPECT_EQ(dst[2], 3);
  EXPECT_EQ(dst[3], 2);
  EXPECT_EQ(dst[4], 1);
}

// ============================================================================
// Test Suite: rotate
// ============================================================================

TEST(Rotate, RotateElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  
  Aleph::rotate(v.begin(), v.begin() + 2, v.end());
  
  EXPECT_EQ(v[0], 3);
  EXPECT_EQ(v[1], 4);
  EXPECT_EQ(v[2], 5);
  EXPECT_EQ(v[3], 1);
  EXPECT_EQ(v[4], 2);
}

// ============================================================================
// Test Suite: lower_bound / upper_bound
// ============================================================================

TEST(LowerBound, FindPosition)
{
  vector<int> v = {1, 2, 4, 5, 6};
  auto it = Aleph::lower_bound(v.begin(), v.end(), 4);
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 4);
}

TEST(LowerBound, ValueNotPresent)
{
  vector<int> v = {1, 2, 4, 5, 6};
  auto it = Aleph::lower_bound(v.begin(), v.end(), 3);
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 4);  // First element >= 3
}

TEST(UpperBound, FindPosition)
{
  vector<int> v = {1, 2, 4, 5, 6};
  auto it = Aleph::upper_bound(v.begin(), v.end(), 4);
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 5);  // First element > 4
}

// ============================================================================
// Test Suite: binary_search
// ============================================================================

TEST(BinarySearch, ValueExists)
{
  vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_TRUE(Aleph::binary_search(v.begin(), v.end(), 3));
}

TEST(BinarySearch, ValueMissing)
{
  vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_FALSE(Aleph::binary_search(v.begin(), v.end(), 10));
}

// ============================================================================
// Test Suite: equal_range
// ============================================================================

TEST(EqualRange, FindRange)
{
  vector<int> v = {1, 2, 2, 2, 3, 4};
  auto range = Aleph::equal_range(v.begin(), v.end(), 2);
  
  EXPECT_EQ(*range.first, 2);
  EXPECT_EQ(*range.second, 3);
}

// ============================================================================
// Test Suite: includes
// ============================================================================

TEST(Includes, SubsetIncluded)
{
  vector<int> v1 = {1, 2, 3, 4, 5, 6, 7};
  vector<int> v2 = {2, 4, 6};
  EXPECT_TRUE(Aleph::includes(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(Includes, SubsetNotIncluded)
{
  vector<int> v1 = {1, 2, 3, 4, 5};
  vector<int> v2 = {2, 4, 8};
  EXPECT_FALSE(Aleph::includes(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(Includes, EmptySubset)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2;
  EXPECT_TRUE(Aleph::includes(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

// ============================================================================
// Test Suite: merge
// ============================================================================

TEST(Merge, MergeSortedRanges)
{
  vector<int> v1 = {1, 3, 5};
  vector<int> v2 = {2, 4, 6};
  vector<int> result(6, 0);
  
  Aleph::merge(v1.begin(), v1.end(), v2.begin(), v2.end(), result.begin());
  
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
  EXPECT_EQ(result[3], 4);
  EXPECT_EQ(result[4], 5);
  EXPECT_EQ(result[5], 6);
}

TEST(Merge, MergeWithEmpty)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2;
  vector<int> result(3, 0);
  
  Aleph::merge(v1.begin(), v1.end(), v2.begin(), v2.end(), result.begin());
  
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

// ============================================================================
// Test Suite: accumulate
// ============================================================================

TEST(Accumulate, SumElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto sum = Aleph::accumulate(v.begin(), v.end(), 0);
  EXPECT_EQ(sum, 15);
}

TEST(Accumulate, ProductElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto product = Aleph::accumulate(v.begin(), v.end(), 1,
                                    [](int a, int b) { return a * b; });
  EXPECT_EQ(product, 120);
}

TEST(Accumulate, WithInitialValue)
{
  vector<int> v = {1, 2, 3};
  auto sum = Aleph::accumulate(v.begin(), v.end(), 10);
  EXPECT_EQ(sum, 16);
}

TEST(Accumulate, EmptyRange)
{
  vector<int> empty;
  auto sum = Aleph::accumulate(empty.begin(), empty.end(), 42);
  EXPECT_EQ(sum, 42);
}

// ============================================================================
// Test Suite: inner_product
// ============================================================================

TEST(InnerProduct, DotProduct)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {4, 5, 6};
  
  auto result = Aleph::inner_product(v1.begin(), v1.end(), v2.begin(), 0);
  EXPECT_EQ(result, 32);  // 1*4 + 2*5 + 3*6
}

TEST(InnerProduct, CustomOperations)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {1, 1, 1};
  
  // Sum of differences
  auto result = Aleph::inner_product(v1.begin(), v1.end(), v2.begin(), 0,
                                      [](int a, int b) { return a + b; },
                                      [](int a, int b) { return a - b; });
  EXPECT_EQ(result, 3);  // (1-1) + (2-1) + (3-1)
}

// ============================================================================
// Test Suite: partial_sum
// ============================================================================

TEST(PartialSum, CumulativeSum)
{
  vector<int> src = {1, 2, 3, 4, 5};
  vector<int> dst(5, 0);
  
  Aleph::partial_sum(src.begin(), src.end(), dst.begin());
  
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 3);
  EXPECT_EQ(dst[2], 6);
  EXPECT_EQ(dst[3], 10);
  EXPECT_EQ(dst[4], 15);
}

TEST(PartialSum, CustomOperation)
{
  vector<int> src = {1, 2, 3, 4};
  vector<int> dst(4, 0);
  
  Aleph::partial_sum(src.begin(), src.end(), dst.begin(),
                      [](int a, int b) { return a * b; });
  
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 2);  // 1 * 2
  EXPECT_EQ(dst[2], 6);  // 2 * 3
  EXPECT_EQ(dst[3], 24); // 6 * 4
}

TEST(PartialSum, EmptyRange)
{
  vector<int> empty;
  vector<int> dst;
  
  auto it = Aleph::partial_sum(empty.begin(), empty.end(), dst.begin());
  EXPECT_EQ(it, dst.begin());
}

// ============================================================================
// Test Suite: adjacent_difference
// ============================================================================

TEST(AdjacentDifference, ComputeDifferences)
{
  vector<int> src = {1, 3, 6, 10, 15};
  vector<int> dst(5, 0);
  
  Aleph::adjacent_difference(src.begin(), src.end(), dst.begin());
  
  EXPECT_EQ(dst[0], 1);   // First element unchanged
  EXPECT_EQ(dst[1], 2);   // 3 - 1
  EXPECT_EQ(dst[2], 3);   // 6 - 3
  EXPECT_EQ(dst[3], 4);   // 10 - 6
  EXPECT_EQ(dst[4], 5);   // 15 - 10
}

TEST(AdjacentDifference, CustomOperation)
{
  vector<int> src = {1, 2, 4, 8};
  vector<int> dst(4, 0);
  
  Aleph::adjacent_difference(src.begin(), src.end(), dst.begin(),
                              [](int a, int b) { return a / b; });
  
  EXPECT_EQ(dst[0], 1);  // First unchanged
  EXPECT_EQ(dst[1], 2);  // 2 / 1
  EXPECT_EQ(dst[2], 2);  // 4 / 2
  EXPECT_EQ(dst[3], 2);  // 8 / 4
}

// ============================================================================
// Test Suite: unique
// ============================================================================

TEST(Unique, RemoveDuplicates)
{
  vector<int> v = {1, 1, 2, 2, 2, 3, 3};
  
  auto it = Aleph::unique(v.begin(), v.end());
  
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 2);
  EXPECT_EQ(v[2], 3);
  EXPECT_EQ(std::distance(v.begin(), it), 3);
}

TEST(Unique, NoDuplicates)
{
  vector<int> v = {1, 2, 3, 4};
  
  auto it = Aleph::unique(v.begin(), v.end());
  EXPECT_EQ(it, v.end());
}

// ============================================================================
// Test Suite: remove / remove_if
// ============================================================================

TEST(RemoveIf, RemoveEvenNumbers)
{
  vector<int> v = {1, 2, 3, 4, 5, 6};
  
  auto it = Aleph::remove_if(v.begin(), v.end(),
                              [](int x) { return x % 2 == 0; });
  
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 3);
  EXPECT_EQ(v[2], 5);
  EXPECT_EQ(std::distance(v.begin(), it), 3);
}

TEST(Remove, RemoveValue)
{
  vector<int> v = {1, 2, 1, 3, 1};
  
  auto it = Aleph::remove(v.begin(), v.end(), 1);
  
  EXPECT_EQ(v[0], 2);
  EXPECT_EQ(v[1], 3);
  EXPECT_EQ(std::distance(v.begin(), it), 2);
}

// ============================================================================
// Test Suite: remove_copy_if
// ============================================================================

TEST(RemoveCopyIf, BasicCopy)
{
  vector<int> src = {1, 2, 3, 4, 5, 6};
  vector<int> dst;
  dst.reserve(6);
  
  auto it = Aleph::remove_copy_if(src.begin(), src.end(),
                                   std::back_inserter(dst),
                                   [](int x) { return x % 2 == 0; });
  (void)it;
  
  ASSERT_EQ(dst.size(), 3);
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 3);
  EXPECT_EQ(dst[2], 5);
}

// ============================================================================
// Test Suite: unique_copy
// ============================================================================

TEST(UniqueCopy, BasicCopy)
{
  vector<int> src = {1, 1, 2, 2, 3, 3};
  vector<int> dst;
  
  Aleph::unique_copy(src.begin(), src.end(), std::back_inserter(dst));
  
  ASSERT_EQ(dst.size(), 3);
  EXPECT_EQ(dst[0], 1);
  EXPECT_EQ(dst[1], 2);
  EXPECT_EQ(dst[2], 3);
}

// ============================================================================
// Test Suite: Edge Cases
// ============================================================================

TEST(EdgeCases, SingleElement)
{
  vector<int> v = {42};
  
  // count
  EXPECT_EQ(Aleph::count(v.begin(), v.end(), 42), 1);
  
  // min/max
  EXPECT_EQ(*Aleph::min_element(v.begin(), v.end()), 42);
  
  // accumulate
  EXPECT_EQ(Aleph::accumulate(v.begin(), v.end(), 0), 42);
}

TEST(EdgeCases, LargeRange)
{
  const int N = 10000;
  vector<int> v(N);
  for (int i = 0; i < N; ++i)
    v[i] = i;
  
  // Count all even
  auto count = Aleph::count_if(v.begin(), v.end(),
                                [](int x) { return x % 2 == 0; });
  EXPECT_EQ(count, N / 2);
  
  // Sum all
  auto sum = Aleph::accumulate(v.begin(), v.end(), 0L);
  EXPECT_EQ(sum, (long)(N - 1) * N / 2);
}

// ============================================================================
// Test Suite: mismatch
// ============================================================================

TEST(Mismatch, FindMismatch)
{
  vector<int> v1 = {1, 2, 3, 4, 5};
  vector<int> v2 = {1, 2, 3, 9, 5};
  
  auto result = Aleph::mismatch(v1.begin(), v1.end(), v2.begin());
  
  EXPECT_EQ(*result.first, 4);
  EXPECT_EQ(*result.second, 9);
}

TEST(Mismatch, NoMismatch)
{
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {1, 2, 3};
  
  auto result = Aleph::mismatch(v1.begin(), v1.end(), v2.begin());
  
  EXPECT_EQ(result.first, v1.end());
}

// ============================================================================
// Test Suite: search
// ============================================================================

TEST(Search, FindSubsequence)
{
  vector<int> v = {1, 2, 3, 4, 5, 6};
  vector<int> sub = {3, 4, 5};
  
  auto it = Aleph::search(v.begin(), v.end(), sub.begin(), sub.end());
  
  EXPECT_NE(it, v.end());
  EXPECT_EQ(*it, 3);
}

TEST(Search, SubsequenceNotFound)
{
  vector<int> v = {1, 2, 3, 4, 5};
  vector<int> sub = {3, 5, 4};
  
  auto it = Aleph::search(v.begin(), v.end(), sub.begin(), sub.end());
  
  EXPECT_EQ(it, v.end());
}
