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
 * @file array.cc
 * @brief Tests for Array
 */

#include <gtest/gtest.h>

#include <tpl_array.H>

#include <array>
#include <numeric>
#include <string>
#include <vector>

using namespace Aleph;

namespace {

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

TEST(ArrayReverse, ReverseAndRevAliases)
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

  arr.rev();
  for (size_t i = 0; i < ascending.size(); ++i)
    EXPECT_EQ(arr[i], ascending[i]) << "rev() alias should behave like reverse()";

  const auto copy_rev = carr.rev();
  for (size_t i = 0; i < descending.size(); ++i)
    EXPECT_EQ(copy_rev[i], descending[i]) << "const rev() should return reversed copy";
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

} // namespace
