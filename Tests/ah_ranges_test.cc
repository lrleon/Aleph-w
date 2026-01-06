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
 * @file ah_ranges_test.cc
 * @brief Tests for Ah Ranges
 */
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <type_traits>

#include <ah-ranges.H>
#include <htlist.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <tpl_dynListStack.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynSetTree.H>

using namespace Aleph;

// ============================================================================
// Feature Detection Tests
// ============================================================================

TEST(RangesFeatureDetection, CompileTimeChecks) {
    #if ALEPH_HAS_RANGES
        SUCCEED() << "C++20 ranges support is available";
    #else
        GTEST_SKIP() << "std::ranges not fully supported on this platform (libc++ version)";
    #endif
}

#if ALEPH_HAS_RANGES

TEST(RangesFeatureDetection, MacrosAreDefined) {
    #ifdef ALEPH_HAS_RANGES
        SUCCEED();
    #else
        FAIL() << "ALEPH_HAS_RANGES should be defined";
    #endif
    
    #ifdef ALEPH_HAS_STRIDE
        SUCCEED();
    #else
        FAIL() << "ALEPH_HAS_STRIDE should be defined";
    #endif
    
    #ifdef ALEPH_HAS_ENUMERATE
        SUCCEED();
    #else
        FAIL() << "ALEPH_HAS_ENUMERATE should be defined";
    #endif
}

// ============================================================================
// Pipe Adaptor Tests - to_dynlist_v
// ============================================================================

TEST(PipeAdaptors, ToDynListFromIota) {
    auto list = std::views::iota(1, 6) | to_dynlist_v;
    
    ASSERT_EQ(list.size(), 5);
    
    int expected = 1;
    for (auto x : list) {
        EXPECT_EQ(x, expected++);
    }
}

TEST(PipeAdaptors, ToDynListFromFilteredRange) {
    auto evens = std::views::iota(1, 11) 
               | std::views::filter([](int x) { return x % 2 == 0; })
               | to_dynlist_v;
    
    ASSERT_EQ(evens.size(), 5);
    
    int expected = 2;
    for (auto x : evens) {
        EXPECT_EQ(x, expected);
        expected += 2;
    }
}

TEST(PipeAdaptors, ToDynListFromTransformedRange) {
    auto squares = std::views::iota(1, 6) 
                 | std::views::transform([](int x) { return x * x; })
                 | to_dynlist_v;
    
    ASSERT_EQ(squares.size(), 5);
    
    DynList<int> expected = {1, 4, 9, 16, 25};
    auto it1 = squares.get_it();
    auto it2 = expected.get_it();
    while (it1.has_curr() && it2.has_curr()) {
        EXPECT_EQ(it1.get_curr(), it2.get_curr());
        it1.next();
        it2.next();
    }
}

TEST(PipeAdaptors, ToDynListFromVector) {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto list = vec | std::views::all | to_dynlist_v;
    
    ASSERT_EQ(list.size(), 5);
    
    size_t i = 0;
    for (auto x : list) {
        EXPECT_EQ(x, vec[i++]);
    }
}

// ============================================================================
// Pipe Adaptor Tests - to_dynarray_v
// ============================================================================

TEST(PipeAdaptors, ToDynArrayFromIota) {
    auto arr = std::views::iota(1, 6) | to_dynarray_v;
    
    ASSERT_EQ(arr.size(), 5);
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(int(arr[i]), i + 1);
    }
}

TEST(PipeAdaptors, ToDynArrayFromFilteredRange) {
    auto odds = std::views::iota(1, 11) 
              | std::views::filter([](int x) { return x % 2 == 1; })
              | to_dynarray_v;
    
    ASSERT_EQ(odds.size(), 5);
    EXPECT_EQ(int(odds[0]), 1);
    EXPECT_EQ(int(odds[1]), 3);
    EXPECT_EQ(int(odds[2]), 5);
    EXPECT_EQ(int(odds[3]), 7);
    EXPECT_EQ(int(odds[4]), 9);
}

TEST(PipeAdaptors, ToDynArrayComplexPipeline) {
    // Filter -> Transform -> Take
    auto result = std::views::iota(1, 100)
                | std::views::filter([](int x) { return x % 3 == 0; })
                | std::views::transform([](int x) { return x * 2; })
                | std::views::take(5)
                | to_dynarray_v;
    
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(int(result[0]), 6);   // 3 * 2
    EXPECT_EQ(int(result[1]), 12);  // 6 * 2
    EXPECT_EQ(int(result[2]), 18);  // 9 * 2
    EXPECT_EQ(int(result[3]), 24);  // 12 * 2
    EXPECT_EQ(int(result[4]), 30);  // 15 * 2
}

// ============================================================================
// Pipe Adaptor Tests - to_dyndlist_v
// ============================================================================

TEST(PipeAdaptors, ToDynDlistFromIota) {
    auto dlist = std::views::iota(1, 6) | to_dyndlist_v;
    
    ASSERT_EQ(dlist.size(), 5);
    
    int expected = 1;
    for (auto x : dlist) {
        EXPECT_EQ(x, expected++);
    }
}

// ============================================================================
// Pipe Adaptor Tests - Stack and Queue
// ============================================================================

TEST(PipeAdaptors, ToDynListStack) {
    auto stack = std::views::iota(1, 6) | to_dynliststack_v;
    
    EXPECT_EQ(stack.size(), 5);
    
    // Stack: last pushed is on top (5 is top)
    EXPECT_EQ(stack.top(), 5);
}

TEST(PipeAdaptors, ToDynListQueue) {
    auto queue = std::views::iota(1, 6) | to_dynlistqueue_v;
    
    EXPECT_EQ(queue.size(), 5);
    
    // Queue: first put is at front
    EXPECT_EQ(queue.front(), 1);
    EXPECT_EQ(queue.rear(), 5);
}

// ============================================================================
// Generic to<Container>() Adaptor Tests
// ============================================================================

TEST(GenericToAdaptor, ToDynListGeneric) {
    auto list = std::views::iota(1, 6) | to<DynList<int>>();
    
    ASSERT_EQ(list.size(), 5);
    
    int expected = 1;
    for (auto x : list) {
        EXPECT_EQ(x, expected++);
    }
}

TEST(GenericToAdaptor, ToDynArrayGeneric) {
    auto arr = std::views::iota(10, 15) | to<DynArray<int>>();
    
    ASSERT_EQ(arr.size(), 5);
    EXPECT_EQ(int(arr[0]), 10);
    EXPECT_EQ(int(arr[4]), 14);
}

TEST(GenericToAdaptor, ToDynSetTreeGeneric) {
    auto set = std::views::iota(1, 11) 
             | std::views::filter([](int x) { return x % 2 == 0; })
             | to<DynSetRbTree<int>>();
    
    EXPECT_EQ(set.size(), 5);
    EXPECT_TRUE(set.has(2));
    EXPECT_TRUE(set.has(4));
    EXPECT_TRUE(set.has(6));
    EXPECT_TRUE(set.has(8));
    EXPECT_TRUE(set.has(10));
    EXPECT_FALSE(set.has(1));
    EXPECT_FALSE(set.has(3));
}

// ============================================================================
// Internal Range Functions Tests (using std::vector which is std::ranges::range)
// ============================================================================

TEST(RangesFunctions, RangesAllOf) {
    std::vector<int> all_positive = {1, 2, 3, 4, 5};
    std::vector<int> has_negative = {1, 2, -3, 4, 5};
    
    EXPECT_TRUE(detail::ranges_all_of(all_positive, [](int x) { return x > 0; }));
    EXPECT_FALSE(detail::ranges_all_of(has_negative, [](int x) { return x > 0; }));
}

TEST(RangesFunctions, RangesAnyOf) {
    std::vector<int> no_even = {1, 3, 5, 7, 9};
    std::vector<int> has_even = {1, 2, 3, 4, 5};
    
    EXPECT_FALSE(detail::ranges_any_of(no_even, [](int x) { return x % 2 == 0; }));
    EXPECT_TRUE(detail::ranges_any_of(has_even, [](int x) { return x % 2 == 0; }));
}

TEST(RangesFunctions, RangesNoneOf) {
    std::vector<int> all_positive = {1, 2, 3, 4, 5};
    std::vector<int> has_negative = {1, 2, -3, 4, 5};
    
    EXPECT_TRUE(detail::ranges_none_of(all_positive, [](int x) { return x < 0; }));
    EXPECT_FALSE(detail::ranges_none_of(has_negative, [](int x) { return x < 0; }));
}

TEST(RangesFunctions, RangesFindIf) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto it = detail::ranges_find_if(vec, [](int x) { return x > 3; });
    ASSERT_NE(it, vec.end());
    EXPECT_EQ(*it, 4);
    
    auto not_found = detail::ranges_find_if(vec, [](int x) { return x > 10; });
    EXPECT_EQ(not_found, vec.end());
}

TEST(RangesFunctions, RangesCountIf) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto even_count = detail::ranges_count_if(vec, [](int x) { return x % 2 == 0; });
    EXPECT_EQ(even_count, 5);
    
    auto gt_five = detail::ranges_count_if(vec, [](int x) { return x > 5; });
    EXPECT_EQ(gt_five, 5);
}

TEST(RangesFunctions, RangesFoldLeft) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // Sum
    auto sum = detail::ranges_fold_left(vec, 0, std::plus<>{});
    EXPECT_EQ(sum, 15);
    
    // Product
    auto product = detail::ranges_fold_left(vec, 1, std::multiplies<>{});
    EXPECT_EQ(product, 120);
    
    // String concatenation
    std::vector<std::string> strs = {"Hello", " ", "World"};
    auto concat = detail::ranges_fold_left(strs, std::string{}, std::plus<>{});
    EXPECT_EQ(concat, "Hello World");
}

TEST(RangesFunctions, RangesSum) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto sum = detail::ranges_sum(vec);
    EXPECT_EQ(sum, 15);
}

TEST(RangesFunctions, RangesProduct) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto prod = detail::ranges_product(vec);
    EXPECT_EQ(prod, 120);
}

// ============================================================================
// Lazy Range Generation Tests
// ============================================================================

TEST(LazyRanges, LazyRangeBasic) {
    auto range = lazy_range(0, 5);
    
    int count = 0;
    for (auto x : range) {
        EXPECT_EQ(x, count++);
    }
    EXPECT_EQ(count, 5);
}

TEST(LazyRanges, LazyRangeWithTwoArgs) {
    auto range = lazy_range(1, 6);
    
    std::vector<int> result;
    for (auto x : range) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(result[i], i + 1);
    }
}

TEST(LazyRanges, LazyIotaWithTake) {
    auto first_10 = lazy_iota(1) | std::views::take(10) | to_dynarray_v;
    
    ASSERT_EQ(first_10.size(), 10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(int(first_10[i]), i + 1);
    }
}

// ============================================================================
// RangeLike Concept Tests
// ============================================================================

TEST(RangeLikeConcept, StdContainersAreRangeLike) {
    static_assert(RangeLike<std::vector<int>>);
    static_assert(RangeLike<std::string>);
    static_assert(RangeLike<std::array<int, 5>>);
}

TEST(RangeLikeConcept, ViewsAreRangeLike) {
    using IotaView = decltype(std::views::iota(1, 10));
    static_assert(RangeLike<IotaView>);
    
    std::vector<int> v = {1, 2, 3};
    using FilteredView = decltype(v | std::views::filter([](int x) { return x > 0; }));
    static_assert(RangeLike<FilteredView>);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(EdgeCases, EmptyRangeToDynList) {
    auto empty = std::views::iota(1, 1) | to_dynlist_v;  // [1, 1) is empty
    EXPECT_EQ(empty.size(), 0);
}

TEST(EdgeCases, EmptyRangeToDynArray) {
    auto empty = std::views::iota(5, 5) | to_dynarray_v;
    EXPECT_EQ(empty.size(), 0);
}

TEST(EdgeCases, SingleElementRange) {
    auto single = std::views::iota(42, 43) | to_dynlist_v;
    ASSERT_EQ(single.size(), 1);
    EXPECT_EQ(single.get_first(), 42);
}

TEST(EdgeCases, LargeRange) {
    const int N = 10000;
    auto large = std::views::iota(1, N + 1) | to_dynarray_v;
    
    ASSERT_EQ(large.size(), N);
    EXPECT_EQ(int(large[0]), 1);
    EXPECT_EQ(int(large[N - 1]), N);
    
    // Verify sum using fold
    long long sum = detail::ranges_fold_left(large, 0LL, std::plus<>{});
    EXPECT_EQ(sum, static_cast<long long>(N) * (N + 1) / 2);
}

// ============================================================================
// String Type Tests
// ============================================================================

TEST(StringTypes, StringsToDynList) {
    std::vector<std::string> strs = {"hello", "world", "test"};
    auto list = strs | std::views::all | to_dynlist_v;
    
    ASSERT_EQ(list.size(), 3);
    
    auto it = list.get_it();
    EXPECT_EQ(it.get_curr(), "hello");
    it.next();
    EXPECT_EQ(it.get_curr(), "world");
    it.next();
    EXPECT_EQ(it.get_curr(), "test");
}

TEST(StringTypes, TransformStrings) {
    std::vector<std::string> strs = {"a", "bb", "ccc"};
    auto lengths = strs 
                 | std::views::transform([](const std::string& s) { return s.length(); })
                 | to_dynarray_v;
    
    ASSERT_EQ(lengths.size(), 3);
    EXPECT_EQ(size_t(lengths[0]), 1);
    EXPECT_EQ(size_t(lengths[1]), 2);
    EXPECT_EQ(size_t(lengths[2]), 3);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(StressTests, ChainedOperations) {
    const int N = 1000;
    
    auto result = std::views::iota(1, N + 1)
                | std::views::filter([](int x) { return x % 2 == 0; })  // 500 evens
                | std::views::transform([](int x) { return x * 3; })    // multiply by 3
                | std::views::filter([](int x) { return x % 6 == 0; })  // divisible by 6
                | std::views::take(100)
                | to_dynlist_v;
    
    ASSERT_EQ(result.size(), 100);
    
    // All elements should be divisible by 6
    for (auto x : result) {
        EXPECT_EQ(x % 6, 0);
    }
}

TEST(StressTests, MultiplePipeConversions) {
    // Build list from iota view
    auto list1 = std::views::iota(1, 101) | to_dynlist_v;
    EXPECT_EQ(list1.size(), 100);
    
    // Build set from filtered iota view
    auto set = std::views::iota(1, 51) 
             | std::views::filter([](int x) { return x > 40; })
             | to<DynSetRbTree<int>>();
    EXPECT_EQ(set.size(), 10);  // 41..50
}

// ============================================================================
// Tests for Aleph Container Iteration (using range-based for)
// ============================================================================

TEST(AlephContainerIteration, DynListRangeFor) {
    DynList<int> list;
    for (int i = 1; i <= 5; ++i)
        list.append(i);
    
    int expected = 1;
    for (auto x : list) {
        EXPECT_EQ(x, expected++);
    }
}

TEST(AlephContainerIteration, DynArrayRangeFor) {
    DynArray<int> arr;
    for (int i = 1; i <= 5; ++i)
        arr.append(i);
    
    int expected = 1;
    for (auto x : arr) {
        EXPECT_EQ(x, expected++);
    }
}

TEST(AlephContainerIteration, DynSetTreeRangeFor) {
    DynSetRbTree<int> set;
    for (int i = 1; i <= 5; ++i)
        set.insert(i);
    
    int count = 0;
    for (auto x : set) {
        EXPECT_GE(x, 1);
        EXPECT_LE(x, 5);
        ++count;
    }
    EXPECT_EQ(count, 5);
}

// ============================================================================
// Note on std::ranges compatibility
// ============================================================================

// Aleph iterators currently don't satisfy the full requirements of
// std::ranges::input_iterator because:
// - iter_reference_t<const I> != iter_reference_t<I>
//
// This means std::ranges algorithms like std::ranges::all_of, std::ranges::find,
// std::ranges::min_element cannot be used directly with Aleph containers.
//
// The workarounds are:
// 1. Use the pipe adaptors (to_dynlist_v, to_dynarray_v, etc.) to convert
//    views to Aleph containers
// 2. Use the internal detail::ranges_* functions which work with std::vector
// 3. Use the traditional Aleph algorithms from ahFunctional.H

// Test that std::ranges works with std::vector (as a sanity check)
TEST(StdRanges, WorksWithStdVector) {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    
    EXPECT_TRUE(std::ranges::any_of(vec, [](int x) { return x > 5; }));
    EXPECT_FALSE(std::ranges::all_of(vec, [](int x) { return x < 5; }));
    EXPECT_TRUE(std::ranges::none_of(vec, [](int x) { return x > 10; }));
    
    auto it = std::ranges::find(vec, 5);
    ASSERT_NE(it, vec.end());
    EXPECT_EQ(*it, 5);
    
    auto min_it = std::ranges::min_element(vec);
    ASSERT_NE(min_it, vec.end());
    EXPECT_EQ(*min_it, 1);
    
    auto max_it = std::ranges::max_element(vec);
    ASSERT_NE(max_it, vec.end());
    EXPECT_EQ(*max_it, 9);
}

// ============================================================================
// Additional Pipe Adaptor Tests - ArrayStack, ArrayQueue, Random_Set
// ============================================================================

#include <tpl_arrayStack.H>
#include <tpl_arrayQueue.H>
#include <tpl_random_queue.H>  // Contains Random_Set

TEST(PipeAdaptors, ToArrayStack) {
    auto stack = std::views::iota(1, 6) | to_arraystack_v;
    
    EXPECT_EQ(stack.size(), 5);
    // Stack: last pushed is on top
    EXPECT_EQ(stack.top(), 5);
    
    // Pop in LIFO order
    EXPECT_EQ(stack.pop(), 5);
    EXPECT_EQ(stack.pop(), 4);
    EXPECT_EQ(stack.pop(), 3);
}

TEST(PipeAdaptors, ToArrayQueue) {
    auto queue = std::views::iota(10, 15) | to_arrayqueue_v;
    
    EXPECT_EQ(queue.size(), 5);
    // Queue: first put is at front
    EXPECT_EQ(queue.front(), 10);
    EXPECT_EQ(queue.rear(), 14);
    
    // Pop in FIFO order
    EXPECT_EQ(queue.get(), 10);
    EXPECT_EQ(queue.get(), 11);
}

TEST(PipeAdaptors, ToRandomSet) {
    auto set = std::views::iota(1, 11) | to_randomset_v;
    
    EXPECT_EQ(set.size(), 10);
    
    // Random_Set uses for_each for iteration
    int sum = 0;
    set.for_each([&sum](int x) { sum += x; });
    
    // Sum of 1 to 10 is 55
    EXPECT_EQ(sum, 55);
}

TEST(PipeAdaptors, ToRandomSetMultipleElements) {
    // Random_Set allows duplicates (it's more like a random queue)
    std::vector<int> vec = {1, 2, 2, 3, 3, 3};
    auto set = vec | std::views::all | to_randomset_v;
    
    // All elements are appended (duplicates allowed)
    EXPECT_EQ(set.size(), 6);
}

// ============================================================================
// Detail Range Functions - Transform, Filter, Take, Drop
// ============================================================================

TEST(RangesFunctions, RangesTransform) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto doubled = detail::ranges_transform(vec, [](int x) { return x * 2; });
    
    // Materialize to check results
    std::vector<int> result;
    for (auto x : doubled) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 6);
    EXPECT_EQ(result[3], 8);
    EXPECT_EQ(result[4], 10);
}

TEST(RangesFunctions, RangesFilter) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto evens = detail::ranges_filter(vec, [](int x) { return x % 2 == 0; });
    
    std::vector<int> result;
    for (auto x : evens) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 6);
    EXPECT_EQ(result[3], 8);
    EXPECT_EQ(result[4], 10);
}

TEST(RangesFunctions, RangesTake) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto first_three = detail::ranges_take(vec, 3);
    
    std::vector<int> result;
    for (auto x : first_three) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
}

TEST(RangesFunctions, RangesDrop) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto last_three = detail::ranges_drop(vec, 2);
    
    std::vector<int> result;
    for (auto x : last_three) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 3);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 5);
}

TEST(RangesFunctions, RangesTakeZero) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto empty = detail::ranges_take(vec, 0);
    
    int count = 0;
    for ([[maybe_unused]] auto x : empty) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(RangesFunctions, RangesDropAll) {
    std::vector<int> vec = {1, 2, 3};
    auto empty = detail::ranges_drop(vec, 10);  // More than size
    
    int count = 0;
    for ([[maybe_unused]] auto x : empty) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

// ============================================================================
// Detail Range Functions - Reverse, Min, Max, Sort
// ============================================================================

TEST(RangesFunctions, RangesReverse) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto reversed = detail::ranges_reverse(vec);
    
    std::vector<int> result;
    for (auto x : reversed) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 5);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 3);
    EXPECT_EQ(result[3], 2);
    EXPECT_EQ(result[4], 1);
}

TEST(RangesFunctions, RangesMin) {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    
    auto min_it = detail::ranges_min(vec);
    ASSERT_NE(min_it, vec.end());
    EXPECT_EQ(*min_it, 1);
}

TEST(RangesFunctions, RangesMax) {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    
    auto max_it = detail::ranges_max(vec);
    ASSERT_NE(max_it, vec.end());
    EXPECT_EQ(*max_it, 9);
}

TEST(RangesFunctions, RangesMinSingleElement) {
    std::vector<int> vec = {42};
    
    auto min_it = detail::ranges_min(vec);
    ASSERT_NE(min_it, vec.end());
    EXPECT_EQ(*min_it, 42);
}

TEST(RangesFunctions, RangesSort) {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    detail::ranges_sort(vec);
    
    ASSERT_EQ(vec.size(), 9);
    for (int i = 0; i < 9; ++i) {
        EXPECT_EQ(vec[i], i + 1);
    }
}

TEST(RangesFunctions, RangesSortDescending) {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    detail::ranges_sort(vec, std::greater<>{});
    
    ASSERT_EQ(vec.size(), 9);
    for (int i = 0; i < 9; ++i) {
        EXPECT_EQ(vec[i], 9 - i);
    }
}

TEST(RangesFunctions, RangesSortStrings) {
    std::vector<std::string> vec = {"banana", "apple", "cherry", "date"};
    
    detail::ranges_sort(vec);
    
    EXPECT_EQ(vec[0], "apple");
    EXPECT_EQ(vec[1], "banana");
    EXPECT_EQ(vec[2], "cherry");
    EXPECT_EQ(vec[3], "date");
}

// ============================================================================
// Detail Range Functions - Flatten (Join)
// ============================================================================

TEST(RangesFunctions, RangesFlatten) {
    std::vector<std::vector<int>> nested = {{1, 2}, {3}, {4, 5, 6}};
    
    auto flat = detail::ranges_flatten(nested);
    
    std::vector<int> result;
    for (auto x : flat) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 6);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
    EXPECT_EQ(result[3], 4);
    EXPECT_EQ(result[4], 5);
    EXPECT_EQ(result[5], 6);
}

TEST(RangesFunctions, RangesFlattenEmpty) {
    std::vector<std::vector<int>> nested = {{}, {}, {}};
    
    auto flat = detail::ranges_flatten(nested);
    
    int count = 0;
    for ([[maybe_unused]] auto x : flat) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(RangesFunctions, RangesFlattenMixed) {
    std::vector<std::vector<int>> nested = {{1}, {}, {2, 3}, {}};
    
    auto flat = detail::ranges_flatten(nested);
    
    std::vector<int> result;
    for (auto x : flat) {
        result.push_back(x);
    }
    
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
}

// ============================================================================
// Collect Function Tests
// ============================================================================

TEST(CollectFunction, CollectToDynList) {
    auto list = collect<DynList<int>>(std::views::iota(1, 6));
    
    ASSERT_EQ(list.size(), 5);
    int expected = 1;
    for (auto x : list) {
        EXPECT_EQ(x, expected++);
    }
}

TEST(CollectFunction, CollectToDynArray) {
    auto arr = collect<DynArray<int>>(std::views::iota(10, 15));
    
    ASSERT_EQ(arr.size(), 5);
    EXPECT_EQ(int(arr[0]), 10);
    EXPECT_EQ(int(arr[4]), 14);
}

TEST(CollectFunction, CollectToSet) {
    auto set = collect<DynSetRbTree<int>>(std::views::iota(1, 11));
    
    EXPECT_EQ(set.size(), 10);
    for (int i = 1; i <= 10; ++i) {
        EXPECT_TRUE(set.has(i));
    }
}

TEST(CollectFunction, CollectFromTransformedRange) {
    auto cubes = collect<DynList<int>>(
        std::views::iota(1, 6) | std::views::transform([](int x) { return x * x * x; })
    );
    
    ASSERT_EQ(cubes.size(), 5);
    
    DynList<int> expected = {1, 8, 27, 64, 125};
    auto it1 = cubes.get_it();
    auto it2 = expected.get_it();
    while (it1.has_curr()) {
        EXPECT_EQ(it1.get_curr(), it2.get_curr());
        it1.next();
        it2.next();
    }
}

// ============================================================================
// Chained Operations with Multiple Views
// ============================================================================

TEST(ChainedOperations, FilterTransformTake) {
    auto result = std::views::iota(1, 100)
                | std::views::filter([](int x) { return x % 2 == 0; })
                | std::views::transform([](int x) { return x * x; })
                | std::views::take(5)
                | to_dynlist_v;
    
    ASSERT_EQ(result.size(), 5);
    // 2² = 4, 4² = 16, 6² = 36, 8² = 64, 10² = 100
    DynList<int> expected = {4, 16, 36, 64, 100};
    
    auto it1 = result.get_it();
    auto it2 = expected.get_it();
    while (it1.has_curr()) {
        EXPECT_EQ(it1.get_curr(), it2.get_curr());
        it1.next();
        it2.next();
    }
}

TEST(ChainedOperations, TransformFilterDrop) {
    auto result = std::views::iota(1, 11)
                | std::views::transform([](int x) { return x * 10; })
                | std::views::filter([](int x) { return x % 30 != 0; })
                | std::views::drop(2)
                | to_dynarray_v;
    
    // 10, 20, 40, 50, 70, 80, 100 (drop first 2) -> 40, 50, 70, 80, 100
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(int(result[0]), 40);
    EXPECT_EQ(int(result[1]), 50);
    EXPECT_EQ(int(result[2]), 70);
}

TEST(ChainedOperations, ReverseFilter) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto result = detail::ranges_reverse(vec);
    std::vector<int> rev;
    for (auto x : result) {
        if (x % 2 == 1) rev.push_back(x);
    }
    
    // Reversed odds: 9, 7, 5, 3, 1
    ASSERT_EQ(rev.size(), 5);
    EXPECT_EQ(rev[0], 9);
    EXPECT_EQ(rev[1], 7);
    EXPECT_EQ(rev[2], 5);
    EXPECT_EQ(rev[3], 3);
    EXPECT_EQ(rev[4], 1);
}

// ============================================================================
// Edge Cases - More Comprehensive
// ============================================================================

TEST(EdgeCases, VeryLargeRange) {
    const int N = 100000;
    auto sum = detail::ranges_fold_left(
        std::views::iota(1, N + 1),
        0LL,
        std::plus<>{}
    );
    
    EXPECT_EQ(sum, static_cast<long long>(N) * (N + 1) / 2);
}

TEST(EdgeCases, EmptyVectorOperations) {
    std::vector<int> empty;
    
    EXPECT_TRUE(detail::ranges_all_of(empty, [](int x) { return x > 0; }));
    EXPECT_FALSE(detail::ranges_any_of(empty, [](int x) { return x > 0; }));
    EXPECT_TRUE(detail::ranges_none_of(empty, [](int x) { return x > 0; }));
    EXPECT_EQ(detail::ranges_count_if(empty, [](int x) { return x > 0; }), 0);
}

TEST(EdgeCases, SingleElementOperations) {
    std::vector<int> single = {42};
    
    EXPECT_TRUE(detail::ranges_all_of(single, [](int x) { return x == 42; }));
    EXPECT_TRUE(detail::ranges_any_of(single, [](int x) { return x == 42; }));
    EXPECT_TRUE(detail::ranges_none_of(single, [](int x) { return x != 42; }));
    EXPECT_EQ(detail::ranges_count_if(single, [](int x) { return x == 42; }), 1);
    EXPECT_EQ(detail::ranges_sum(single), 42);
    EXPECT_EQ(detail::ranges_product(single), 42);
}

TEST(EdgeCases, NegativeNumbers) {
    std::vector<int> neg = {-5, -3, -1, 0, 1, 3, 5};
    
    EXPECT_EQ(detail::ranges_sum(neg), 0);
    EXPECT_EQ(detail::ranges_count_if(neg, [](int x) { return x < 0; }), 3);
    
    auto min_it = detail::ranges_min(neg);
    EXPECT_EQ(*min_it, -5);
    
    auto max_it = detail::ranges_max(neg);
    EXPECT_EQ(*max_it, 5);
}

TEST(EdgeCases, FloatingPointOperations) {
    std::vector<double> floats = {1.5, 2.5, 3.5, 4.5};
    
    double sum = detail::ranges_fold_left(floats, 0.0, std::plus<>{});
    EXPECT_DOUBLE_EQ(sum, 12.0);
    
    double product = detail::ranges_fold_left(floats, 1.0, std::multiplies<>{});
    EXPECT_DOUBLE_EQ(product, 1.5 * 2.5 * 3.5 * 4.5);
}

// ============================================================================
// Complex Type Tests
// ============================================================================

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator<(const Point& other) const { 
        return x < other.x || (x == other.x && y < other.y); 
    }
};

TEST(ComplexTypes, StructTransform) {
    std::vector<Point> points = {{1, 2}, {3, 4}, {5, 6}};
    
    auto distances = detail::ranges_transform(points, [](const Point& p) {
        return p.x * p.x + p.y * p.y;  // squared distance from origin
    });
    
    std::vector<int> result;
    for (auto d : distances) {
        result.push_back(d);
    }
    
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 5);   // 1² + 2²
    EXPECT_EQ(result[1], 25);  // 3² + 4²
    EXPECT_EQ(result[2], 61);  // 5² + 6²
}

TEST(ComplexTypes, StructFilter) {
    std::vector<Point> points = {{0, 0}, {1, 1}, {2, 0}, {0, 2}, {3, 3}};
    
    auto on_diagonal = detail::ranges_filter(points, [](const Point& p) {
        return p.x == p.y;
    });
    
    int count = 0;
    for ([[maybe_unused]] auto& p : on_diagonal) {
        ++count;
    }
    
    EXPECT_EQ(count, 3);  // (0,0), (1,1), (3,3)
}

// ============================================================================
// Lazy Range with Complex Pipelines
// ============================================================================

TEST(LazyRanges, FibonacciLike) {
    // Generate first 10 Fibonacci-like numbers using lazy evaluation
    auto fib = std::views::iota(0)
             | std::views::transform([](int n) {
                   // Using formula for testing purposes
                   int a = 0, b = 1;
                   for (int i = 0; i < n; ++i) {
                       int temp = a + b;
                       a = b;
                       b = temp;
                   }
                   return a;
               })
             | std::views::take(10)
             | to_dynarray_v;
    
    ASSERT_EQ(fib.size(), 10);
    EXPECT_EQ(int(fib[0]), 0);
    EXPECT_EQ(int(fib[1]), 1);
    EXPECT_EQ(int(fib[2]), 1);
    EXPECT_EQ(int(fib[3]), 2);
    EXPECT_EQ(int(fib[4]), 3);
    EXPECT_EQ(int(fib[5]), 5);
    EXPECT_EQ(int(fib[6]), 8);
}

TEST(LazyRanges, PrimeSieve) {
    // Find first 10 primes using lazy evaluation
    auto is_prime = [](int n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        for (int i = 3; i * i <= n; i += 2)
            if (n % i == 0) return false;
        return true;
    };
    
    auto primes = lazy_iota(2)
                | std::views::filter(is_prime)
                | std::views::take(10)
                | to_dynlist_v;
    
    ASSERT_EQ(primes.size(), 10);
    
    DynList<int> expected = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    auto it1 = primes.get_it();
    auto it2 = expected.get_it();
    while (it1.has_curr()) {
        EXPECT_EQ(it1.get_curr(), it2.get_curr());
        it1.next();
        it2.next();
    }
}

// ============================================================================
// Aleph Container with std::ranges Algorithms (via conversion)
// ============================================================================

TEST(AlephWithRanges, DynListToVectorAndBack) {
    DynList<int> original;
    for (int i = 1; i <= 5; ++i)
        original.append(i);
    
    // Convert to vector for std::ranges operations
    std::vector<int> vec(original.begin(), original.end());
    
    // Use std::ranges
    EXPECT_TRUE(std::ranges::all_of(vec, [](int x) { return x > 0; }));
    EXPECT_TRUE(std::ranges::any_of(vec, [](int x) { return x == 3; }));
    
    // Transform and convert back
    auto doubled = vec 
                 | std::views::transform([](int x) { return x * 2; })
                 | to_dynlist_v;
    
    ASSERT_EQ(doubled.size(), 5);
    
    DynList<int> expected = {2, 4, 6, 8, 10};
    auto it1 = doubled.get_it();
    auto it2 = expected.get_it();
    while (it1.has_curr()) {
        EXPECT_EQ(it1.get_curr(), it2.get_curr());
        it1.next();
        it2.next();
    }
}

TEST(AlephWithRanges, DynArrayFilter) {
    DynArray<int> arr;
    for (int i = 1; i <= 10; ++i)
        arr.append(i);
    
    // DynArray should work with range-based for
    std::vector<int> vec(arr.begin(), arr.end());
    
    auto evens = vec 
               | std::views::filter([](int x) { return x % 2 == 0; })
               | to_dynarray_v;
    
    ASSERT_EQ(evens.size(), 5);
    EXPECT_EQ(int(evens[0]), 2);
    EXPECT_EQ(int(evens[4]), 10);
}

// ============================================================================
// Stress Tests - Performance and Correctness
// ============================================================================

TEST(StressTests, LargeChainedPipeline) {
    const int N = 50000;
    
    auto result = std::views::iota(1, N + 1)
                | std::views::filter([](int x) { return x % 3 == 0; })
                | std::views::transform([](int x) { return x * 2; })
                | std::views::filter([](int x) { return x % 4 == 0; })
                | std::views::take(1000)
                | to_dynlist_v;
    
    EXPECT_LE(result.size(), 1000);
    
    // All elements should be divisible by 4
    for (auto x : result) {
        EXPECT_EQ(x % 4, 0);
    }
}

TEST(StressTests, MultipleConversions) {
    // Build from range -> DynList -> vector -> DynArray -> set
    auto list = std::views::iota(1, 101) | to_dynlist_v;
    EXPECT_EQ(list.size(), 100);
    
    std::vector<int> vec(list.begin(), list.end());
    EXPECT_EQ(vec.size(), 100);
    
    auto arr = vec | std::views::filter([](int x) { return x > 50; }) | to_dynarray_v;
    EXPECT_EQ(arr.size(), 50);
    
    std::vector<int> arr_vec(arr.begin(), arr.end());
    auto set = arr_vec | std::views::all | to<DynSetRbTree<int>>();
    EXPECT_EQ(set.size(), 50);
    
    EXPECT_TRUE(set.has(51));
    EXPECT_TRUE(set.has(100));
    EXPECT_FALSE(set.has(50));
}

TEST(StressTests, SumLargeRange) {
    const long long N = 100000;
    
    auto sum = detail::ranges_fold_left(
        std::views::iota(1LL, N + 1),
        0LL,
        std::plus<>{}
    );
    
    EXPECT_EQ(sum, N * (N + 1) / 2);
}

#endif // ALEPH_HAS_RANGES

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
