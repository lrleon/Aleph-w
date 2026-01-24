
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
 * @file dynarray.cc
 * @brief Tests for Dynarray
 */

#include <gtest/gtest.h>

#include <tpl_dynArray.H>

using namespace Aleph;
using namespace std;

namespace {

TEST(DynArrayBasics, construction_and_size)
{
  DynArray<int> arr;
  EXPECT_TRUE(arr.is_empty());
  EXPECT_EQ(arr.size(), 0u);

  arr.append(42);
  EXPECT_EQ(arr.size(), 1u);
  EXPECT_FALSE(arr.is_empty());
  EXPECT_EQ(arr.access(0), 42);

  arr.append(7);
  EXPECT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr.access(1), 7);
}

TEST(DynArrayBasics, default_values_and_touch)
{
  DynArray<int> arr;
  arr.set_default_initial_value(123);

  arr.reserve(0, 3);
  ASSERT_EQ(arr.size(), 4u);
  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr.access(i), 123);

  arr.access(2) = 77;
  EXPECT_EQ(arr.access(2), 77);

  auto & ref = arr.touch(10);
  ref = 99;
  EXPECT_EQ(arr.size(), 11u);
  EXPECT_EQ(arr.access(10), 99);
}

TEST(DynArrayErrors, stack_operations_on_empty)
{
  DynArray<int> arr;

  EXPECT_THROW(arr.pop(), std::underflow_error);
  EXPECT_THROW(arr.top(), std::underflow_error);
  EXPECT_THROW(arr.get_first(), std::underflow_error);
  EXPECT_THROW(arr.get_last(), std::underflow_error);

  int dummy = 0;
  EXPECT_THROW(arr.remove(dummy), std::underflow_error);
}

TEST(DynArrayReserve, invalid_range_throws)
{
  DynArray<int> arr;
  EXPECT_THROW(arr.reserve(5, 4), std::domain_error);
}

TEST(DynArrayReserve, reserve_and_access)
{
  DynArray<int> arr;
  arr.reserve(5);
  ASSERT_EQ(arr.size(), 5u);
  for (size_t i = 0; i < arr.size(); ++i)
    arr.access(i) = static_cast<int>(i * 2);
  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr.access(i), static_cast<int>(i * 2));
}

TEST(DynArrayIterator, get_it_position)
{
  DynArray<int> arr;
  for (int i = 0; i < 6; ++i)
    arr.append(i);

  auto it = arr.get_it(3);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 3);
  it.next();
  EXPECT_EQ(it.get_curr(), 4);

  const auto & carr = arr;
  auto cit = carr.get_it(5);
  EXPECT_EQ(cit.get_curr(), 5);
  EXPECT_THROW(carr.get_it(6), std::out_of_range);
}

TEST(DynArrayReserve, adjust_and_cut)
{
  DynArray<int> arr;
  arr.adjust(10);
  EXPECT_EQ(arr.size(), 10u);
  arr.cut(3);
  EXPECT_EQ(arr.size(), 3u);
  arr.empty();
  EXPECT_TRUE(arr.is_empty());
  arr.append(1);
  arr.append(2);
  arr.cut(2);
  EXPECT_EQ(arr.size(), 2u);
}

TEST(DynArrayReserve, reserve_invalid_order)
{
  DynArray<int> arr;
  EXPECT_THROW(arr.reserve(4, 3), std::domain_error);
  EXPECT_EQ(arr.size(), 0u);
}

TEST(DynArrayReserve, reserve_touch_consistency)
{
  DynArray<int> arr;
  arr.reserve(2, 5);
  ASSERT_EQ(arr.size(), 6u);
  arr.touch(20) = 100;
  EXPECT_EQ(arr.size(), 21u);
  arr.cut(6);
  EXPECT_EQ(arr.size(), 6u);
}

TEST(DynArrayQueueStack, push_pop_fifo_lifo)
{
  DynArray<int> arr;
  for (int i = 0; i < 5; ++i)
    arr.push(i);
  EXPECT_EQ(arr.get_first(), 0);
  EXPECT_EQ(arr.get_last(), 4);

  arr.insert(-1);
  EXPECT_EQ(arr.get_first(), -1);

  EXPECT_EQ(arr.pop(), 4);
  EXPECT_EQ(arr.size(), 5u);
  EXPECT_EQ(arr.top(), arr.get_last());
}

} // namespace
