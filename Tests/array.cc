
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
 * @file array.cc
 * @brief Tests for Array
 */

#include <gtest/gtest.h>

#include <tpl_array.H>
#include <ah-unique.H>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Aleph;

namespace {

// A single integer argument used to mean capacity, or one element when a
// variadic constructor won the overload resolution. It is now rejected.
static_assert(not std::is_constructible_v<Array<int>, int>);
static_assert(not std::is_constructible_v<Array<int>, size_t>);
static_assert(not std::is_constructible_v<Array<int>, long>);
static_assert(not std::is_constructible_v<Array<int>, unsigned>);
static_assert(not std::is_constructible_v<Array<int>, double>);
static_assert(not std::is_constructible_v<Array<int>, bool>);
static_assert(not std::is_constructible_v<Array<size_t>, size_t>);
static_assert(not std::is_constructible_v<Array<double>, int>);
static_assert(not std::is_convertible_v<int, Array<int>>);
static_assert(not std::is_convertible_v<size_t, Array<int>>);

// The other constructors are unaffected.
static_assert(std::is_default_constructible_v<Array<int>>);
static_assert(std::is_copy_constructible_v<Array<int>>);
static_assert(std::is_nothrow_move_constructible_v<Array<int>>);
static_assert(std::is_constructible_v<Array<int>, size_t, int>);
static_assert(std::is_constructible_v<Array<int>, std::initializer_list<int>>);

TEST(ArrayCtors, FactoriesAndBracesSayWhatIsMeant)
{
  const Array<int> by_default;
  EXPECT_TRUE(by_default.is_empty());
  EXPECT_GE(by_default.capacity(), 32u);

  auto reserved = Array<int>::create_reserved(100);
  EXPECT_TRUE(reserved.is_empty());
  EXPECT_GE(reserved.capacity(), 100u);
  reserved.append(5);
  EXPECT_EQ(reserved[0], 5);

  auto small = Array<int>::create_reserved(0);
  EXPECT_TRUE(small.is_empty());
  small.append(1);
  small.append(2);
  EXPECT_EQ(small.size(), 2u);

  auto slots = Array<int>::create(3);
  EXPECT_EQ(slots.size(), 3u);

  const Array<int> item{10};
  ASSERT_EQ(item.size(), 1u);
  EXPECT_EQ(item[0], 10);

  // A parenthesized braced list is still an element list.
  const Array<uint64_t> braced_in_parens({0});
  ASSERT_EQ(braced_in_parens.size(), 1u);
  EXPECT_EQ(braced_in_parens[0], 0u);

  const Array<int> repeated(10, 7);
  ASSERT_EQ(repeated.size(), 10u);
  for (int value : repeated)
    EXPECT_EQ(value, 7);
  const Array<size_t> repeated_size(size_t{3}, size_t{8});
  ASSERT_EQ(repeated_size.size(), 3u);
  EXPECT_EQ(repeated_size[2], 8u);
}

TEST(ArrayCtors, InputRangesSupportSinglePassIteratorsAndSentinels)
{
  std::istringstream input("3 5 8");
  Array<int> streamed{std::istream_iterator<int>(input),
                      std::istream_iterator<int>()};
  ASSERT_EQ(streamed.size(), 3u);
  EXPECT_EQ(streamed[2], 8);

  int values[] = {11, 13, 17};
  Array<int> counted(std::counted_iterator(values, 3), std::default_sentinel);
  EXPECT_EQ(to_stdvector(counted), (std::vector<int>{11, 13, 17}));
}

TEST(ArrayCopyMove, MovedFromArraysRemainEmptyAndReusable)
{
  Array<int> original = {1, 2, 3};
  Array<int> moved(std::move(original));
  EXPECT_TRUE(original.is_empty());
  EXPECT_EQ(original.begin(), original.end());
  Array<int> empty_copy(original);
  EXPECT_TRUE(empty_copy.is_empty());
  Array<int> moved_twice(std::move(original));
  EXPECT_TRUE(moved_twice.is_empty());
  EXPECT_THROW(original[0], std::out_of_range);
  EXPECT_THROW(original.remove_last(), std::underflow_error);

  original.append(9);
  original.insert(8);
  original.reserve(64);
  original.putn(1);
  original[2] = 10;
  EXPECT_EQ(to_stdvector(original), (std::vector<int>{8, 9, 10}));
  EXPECT_EQ(to_stdvector(moved), (std::vector<int>{1, 2, 3}));
  moved_twice.insert(42);
  EXPECT_EQ(moved_twice[0], 42);
}

TEST(ArrayBasics, DefaultConstructionAndBase)
{
  Array<int> arr;
  EXPECT_TRUE(arr.is_empty());
  EXPECT_EQ(arr.size(), 0u);
  EXPECT_THROW(arr.base(), std::underflow_error);

  const Array<int> empty_const;
  EXPECT_THROW(empty_const.base(), std::underflow_error);

  arr.append(10);
  arr.append(20);
  EXPECT_FALSE(arr.is_empty());
  EXPECT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr.base(), 10);
  EXPECT_EQ(arr.get_first(), 10);
  EXPECT_EQ(arr.get_last(), 20);

  const auto & carr = arr;
  EXPECT_EQ(carr.get_first(), 10);
  EXPECT_EQ(carr.get_last(), 20);
}

TEST(ArrayFunctional, ToArrayPreservesOrder)
{
  Array<int> src = {5, 7, 9, 11};

  auto copy = src.to_array();

  ASSERT_EQ(copy.size(), src.size());
  for (size_t i = 0; i < copy.size(); ++i)
    EXPECT_EQ(copy(i), src(i));

  // original remains unchanged
  EXPECT_EQ(src.size(), 4u);
}

TEST(ArrayModifiers, InsertAppendAndRemove)
{
  Array<int> arr;
  arr.append(1);
  arr.append(2);
  arr.insert(-1);

  ASSERT_EQ(arr.size(), 3u);
  EXPECT_EQ(arr.get_first(), -1);
  EXPECT_EQ(arr.get_last(), 2);

  EXPECT_EQ(arr.remove_first(), -1);
  EXPECT_EQ(arr.remove_last(), 2);
  EXPECT_EQ(arr.size(), 1u);
  EXPECT_EQ(arr.base(), 1);

  arr.empty();
  EXPECT_TRUE(arr.is_empty());
}

TEST(ArrayCopyMove, CopyAndMoveSemantics)
{
  Array<int> original = {1, 2, 3, 4};
  Array<int> copy(original);
  ASSERT_EQ(copy.size(), original.size());
  copy[0] = 100;
  EXPECT_EQ(original[0], 1);

  Array<int> assigned;
  assigned = copy;
  EXPECT_EQ(assigned.size(), copy.size());
  EXPECT_EQ(assigned[0], 100);

  Array<int> moved(std::move(copy));
  EXPECT_EQ(moved.size(), 4u);
  EXPECT_EQ(moved[0], 100);

  Array<int> move_assigned;
  move_assigned.append(999);
  move_assigned = std::move(moved);
  EXPECT_EQ(move_assigned.size(), 4u);
  EXPECT_EQ(move_assigned[3], 4);
}

TEST(ArrayCapacity, ReservePutnAndSwap)
{
  Array<int> arr;
  const auto initial_cap = arr.capacity();
  arr.reserve(initial_cap + 50);
  EXPECT_GE(arr.capacity(), initial_cap + 50);

  arr.putn(5);
  ASSERT_EQ(arr.size(), 5u);
  for (size_t i = 0; i < arr.size(); ++i)
    arr[i] = static_cast<int>(i * 10);

  Array<int> other;
  other.append(-1);
  arr.swap(other);
  EXPECT_EQ(arr.size(), 1u);
  EXPECT_EQ(arr[0], -1);
  EXPECT_EQ(other.size(), 5u);
  EXPECT_EQ(other[2], 20);
}

TEST(ArrayAccessors, BoundsCheckingAndConstVariants)
{
  Array<std::string> arr;
  arr.append("hello");
  arr.append("world");

  EXPECT_EQ(arr[0], "hello");
  EXPECT_EQ(arr(1), "world");
  EXPECT_THROW(arr[2], std::out_of_range);

  const Array<std::string> carr = arr;
  EXPECT_EQ(carr[0], "hello");
  EXPECT_EQ(carr(1), "world");
  EXPECT_THROW(carr[3], std::out_of_range);
}

TEST(ArrayReverse, ReverseAndReverseInPlaceAliases)
{
  Array<int> arr;
  for (int i = 1; i <= 5; ++i)
    arr.append(i);

  const std::array<int, 5> ascending = {1, 2, 3, 4, 5};
  const std::array<int, 5> descending = {5, 4, 3, 2, 1};

  arr.reverse();
  for (size_t i = 0; i < descending.size(); ++i)
    EXPECT_EQ(arr[i], descending[i]) << "reverse() should mutate in place";

  const Array<int> &carr = arr;
  const auto copy = carr.reverse();
  for (size_t i = 0; i < ascending.size(); ++i)
    EXPECT_EQ(copy[i], ascending[i]) << "const reverse() should return new copy";

  arr.reverse_in_place();
  for (size_t i = 0; i < ascending.size(); ++i)
    EXPECT_EQ(arr[i], ascending[i]) << "reverse_in_place() alias should behave like reverse()";

  const auto copy_rev = carr.rev();
  for (size_t i = 0; i < descending.size(); ++i)
    EXPECT_EQ(copy_rev[i], descending[i]) << "const rev() should return reversed copy";
}

TEST(ArrayReverse, NonConstRevReversesInPlaceAndReturnsReference)
{
  Array<int> arr = {1, 2, 3};

  Array<int> &alias = arr.rev();

  EXPECT_EQ(&alias, &arr);
  EXPECT_EQ(arr[0], 3);
  EXPECT_EQ(arr[1], 2);
  EXPECT_EQ(arr[2], 1);
}

struct MoveOnlyOp
{
  bool *called;
  explicit MoveOnlyOp(bool *c) : called(c) {}
  MoveOnlyOp(const MoveOnlyOp &) = delete;
  MoveOnlyOp & operator=(const MoveOnlyOp &) = delete;
  MoveOnlyOp(MoveOnlyOp &&) = default;
  MoveOnlyOp & operator=(MoveOnlyOp &&) = default;
  bool operator()(int)
  {
    *called = true;
    return true;
  }
};

TEST(ArrayTraverse, TraversalVariants)
{
  Array<int> arr = {1, 2, 3, 4};

  int sum = 0;
  auto accumulate = [&sum](int value)
    {
      sum += value;
      return true;
    };
  EXPECT_TRUE(arr.traverse(accumulate));
  EXPECT_EQ(sum, 10);

  int visited = 0;
  auto stop_at_three = [&visited](int value)
    {
      ++visited;
      return value < 3;
    };
  EXPECT_FALSE(arr.traverse(stop_at_three));
  EXPECT_EQ(visited, 3);

  bool called = false;
  EXPECT_TRUE(arr.traverse(MoveOnlyOp(&called)));
  EXPECT_TRUE(called);
}

TEST(ArrayTraverse, ConstElementsCannotBeModified)
{
  Array<int> arr = {1, 2, 3};
  const Array<int> &const_arr = arr;

  auto increment = [](int &value)
    {
      ++value;
      return true;
    };
  int sum = 0;
  auto accumulate_const = [&sum](const int &value)
    {
      sum += value;
      return true;
    };

  EXPECT_TRUE(arr.traverse(increment));
  EXPECT_EQ(arr[0], 2);
  EXPECT_EQ(arr[1], 3);
  EXPECT_EQ(arr[2], 4);
  EXPECT_TRUE(const_arr.traverse(accumulate_const));
  EXPECT_EQ(sum, 9);
  EXPECT_TRUE(const_arr.traverse([](const int &value) { return value > 0; }));
  EXPECT_EQ(arr[0], 2);
  EXPECT_EQ(arr[1], 3);
  EXPECT_EQ(arr[2], 4);
}

template <class Container, class Operation>
concept CanTraverse = requires(Container &container, Operation &operation)
{
  container.traverse(operation);
};

struct MutableArrayOperation
{
  bool operator () (int &) const { return true; }
};

static_assert(CanTraverse<Array<int>, MutableArrayOperation>);
static_assert(not CanTraverse<const Array<int>, MutableArrayOperation>);

TEST(ArrayAlgorithms, InPlaceUnique)
{
  Array<int> arr = {1, 2, 1, 3, 2, 4, 4};

  in_place_unique(arr);

  ASSERT_EQ(arr.size(), 4u);
  EXPECT_EQ(arr[0], 1);
  EXPECT_EQ(arr[1], 2);
  EXPECT_EQ(arr[2], 3);
  EXPECT_EQ(arr[3], 4);
}

TEST(ArrayIterators, IteratorCoversAllElements)
{
  Array<int> arr = {0, 1, 2, 3};
  Array<int>::Iterator it(arr);

  int expected = 0;
  for (; it.has_curr(); it.next())
    {
      EXPECT_EQ(it.get_curr(), expected);
      ++expected;
    }
  EXPECT_EQ(expected, 4);
}

TEST(ArrayUtilities, BuildArrayAndStdVector)
{
  auto arr = build_array<int>(5, 4, 3, 2, 1);
  EXPECT_EQ(arr.size(), 5u);
  EXPECT_EQ(arr[0], 5);
  EXPECT_EQ(arr[4], 1);

  const auto vec = to_stdvector(arr);
  ASSERT_EQ(vec.size(), arr.size());
  for (size_t i = 0; i < vec.size(); ++i)
    EXPECT_EQ(vec[i], arr(i));
}

namespace
{
struct DefaultInit
{
  int v;
  DefaultInit() : v(123) {}
  explicit DefaultInit(int x) : v(x) {}
  bool operator==(const DefaultInit &o) const { return v == o.v; }
};
}

TEST(ArrayCtors, ValueConstructorInitializesAllElements_POD)
{
  const size_t n = 8;
  const int value = 42;
  Array<int> arr(n, value);
  ASSERT_EQ(arr.size(), n);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(arr[i], value);
}

TEST(ArrayCtors, ValueConstructorInitializesAllElements_NonPOD)
{
  const size_t n = 6;
  const std::string value = "abc";
  Array<std::string> arr(n, value);
  ASSERT_EQ(arr.size(), n);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(arr[i], value);
}

TEST(ArrayFactory, CreateYieldsLogicalSizeAndRequiresAssignBeforeReadForPOD)
{
  const size_t n = 10;
  auto arr = Array<int>::create(n);
  static_assert(std::is_trivially_default_constructible_v<int>);
  ASSERT_EQ(arr.size(), n);

  for (size_t i = 0; i < arr.size(); ++i)
    arr[i] = static_cast<int>(i * 3);
  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr[i], static_cast<int>(i * 3));
}

TEST(ArrayFactory, CreateDefaultConstructsClassTypes)
{
  const size_t n = 7;
  auto arr = Array<DefaultInit>::create(n);
  static_assert(!std::is_trivially_default_constructible_v<DefaultInit>);
  ASSERT_EQ(arr.size(), n);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(arr[i].v, 123);

  arr[0] = DefaultInit(7);
  EXPECT_EQ(arr[0].v, 7);
}

TEST(ArrayFactory, PutnAndAppendPopulateBothPODAndNonPOD)
{
  Array<int> pod;
  pod.putn(3);
  pod[0] = 1;
  pod[1] = 2;
  pod[2] = 3;
  pod.append(4);
  ASSERT_EQ(pod.size(), 4u);
  EXPECT_EQ(pod[3], 4);

  Array<std::string> nonpod;
  nonpod.putn(2);
  nonpod[0] = "x";
  nonpod[1] = "y";
  nonpod.append("z");
  ASSERT_EQ(nonpod.size(), 3u);
  EXPECT_EQ(nonpod[2], "z");
}

TEST(ArraySearch, Contains)
{
  Array<int> arr = {10, 20, 30, 40};
  EXPECT_TRUE(arr.contains(20));
  EXPECT_TRUE(arr.contains(40));
  EXPECT_FALSE(arr.contains(50));

  EXPECT_TRUE(arr.contains_if([](int x) { return x > 25; }));
  EXPECT_FALSE(arr.contains_if([](int x) { return x > 100; }));
}

TEST(ArraySearch, ContainsEmpty)
{
  Array<int> empty;
  EXPECT_FALSE(empty.contains(10));
  EXPECT_FALSE(empty.contains_if([](int) { return true; }));
}


/// @brief Expect the elements of `a`, in order.
template <class T>
void expect_items(const Array<T> &a, std::initializer_list<T> expected)
{
  ASSERT_EQ(a.size(), expected.size());
  size_t i = 0;
  for (const T &item : expected)
    {
      EXPECT_EQ(a[i], item) << "at position " << i;
      ++i;
    }
}

/// @brief A string too long for the small-string buffer.
std::string long_string(const char c)
{
  return std::string(40, c);
}

// The argument of append() and insert() may be an element of the array
// itself. Growth used to free it before reading it, and the gap opened by
// insert() used to overwrite it.

TEST(ArraySelfInsertion, AppendOwnElement)
{
  auto a = Array<int>::create_reserved(4);
  for (int i : {10, 20, 30, 40})
    a.append(i);
  ASSERT_EQ(a.size(), a.capacity());

  EXPECT_EQ(a.append(a[0]), 10);
  expect_items(a, {10, 20, 30, 40, 10});
  EXPECT_EQ(a.append(a[3]), 40);  // with free capacity
  expect_items(a, {10, 20, 30, 40, 10, 40});
}

TEST(ArraySelfInsertion, AppendOwnMovedElement)
{
  auto a = Array<std::string>::create_reserved(4);
  for (char c : {'a', 'b', 'c', 'd'})
    a.append(long_string(c));
  ASSERT_EQ(a.size(), a.capacity());

  EXPECT_EQ(a.append(std::move(a[0])), long_string('a'));
  ASSERT_EQ(a.size(), 5u);
  EXPECT_EQ(a[3], long_string('d'));
  EXPECT_EQ(a[4], long_string('a'));
}

TEST(ArraySelfInsertion, InsertOwnElement)
{
  auto a = Array<int>::create_reserved(4);
  for (int i : {10, 20, 30})
    a.append(i);

  EXPECT_EQ(a.insert(a[1]), 20);
  expect_items(a, {20, 10, 20, 30});
  ASSERT_EQ(a.size(), a.capacity());
  EXPECT_EQ(a.insert(a[3]), 30);
  expect_items(a, {30, 20, 10, 20, 30});
}

TEST(ArraySelfInsertion, InsertOwnMovedElement)
{
  auto a = Array<std::string>::create_reserved(4);
  for (char c : {'a', 'b', 'c'})
    a.append(long_string(c));

  EXPECT_EQ(a.insert(std::move(a[2])), long_string('c'));
  EXPECT_EQ(a[1], long_string('a'));
  ASSERT_EQ(a.size(), a.capacity());
  EXPECT_EQ(a.insert(std::move(a[2])), long_string('b'));
  ASSERT_EQ(a.size(), 5u);
  EXPECT_EQ(a[1], long_string('c'));
  EXPECT_EQ(a[2], long_string('a'));
}


// get_first() and get_last() were noexcept, so the underflow_error thrown
// on an empty array called std::terminate.
static_assert(not noexcept(std::declval<Array<int> &>().get_first()));
static_assert(not noexcept(std::declval<const Array<int> &>().get_first()));
static_assert(not noexcept(std::declval<Array<int> &>().get_last()));
static_assert(not noexcept(std::declval<const Array<int> &>().get_last()));

TEST(ArrayAccess, GetFirstAndGetLastOfEmptyArrayThrow)
{
  Array<int> a;
  const Array<int> &ca = a;
  EXPECT_THROW((void) a.get_first(), std::underflow_error);
  EXPECT_THROW((void) ca.get_first(), std::underflow_error);
  EXPECT_THROW((void) a.get_last(), std::underflow_error);
  EXPECT_THROW((void) ca.get_last(), std::underflow_error);

  a.append(10);
  a.append(20);
  EXPECT_EQ(ca.get_first(), 10);
  EXPECT_EQ(ca.get_last(), 20);
  a.get_first() = 1;
  a.get_last() = 2;
  expect_items(a, {1, 2});
}

} // namespace
