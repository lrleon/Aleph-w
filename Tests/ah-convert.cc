
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
 * @file ah-convert.cc
 * @brief Tests for Ah Convert
 */

#include <gtest/gtest.h>
#include <string>
#include <set>
#include <map>

#include <ah-convert.H>

using namespace Aleph;

// ==================== to_DynList Tests ====================

TEST(ToDynList, FromEmptyArray)
{
  Array<int> arr;
  auto list = to_DynList(arr);
  EXPECT_TRUE(list.is_empty());
}

TEST(ToDynList, FromSingleElementArray)
{
  Array<int> arr;
  arr.append(42);
  auto list = to_DynList(arr);

  ASSERT_EQ(list.size(), 1);
  EXPECT_EQ(list.get_first(), 42);
}

TEST(ToDynList, FromMultipleElementArray)
{
  Array<int> arr;
  arr.append(1);
  arr.append(2);
  arr.append(3);
  arr.append(4);
  arr.append(5);

  auto list = to_DynList(arr);

  ASSERT_EQ(list.size(), 5);
  int expected = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      ++expected;
    }
}

TEST(ToDynList, WithStrings)
{
  Array<std::string> arr;
  arr.append("hello");
  arr.append("world");

  auto list = to_DynList(arr);

  ASSERT_EQ(list.size(), 2);
  EXPECT_EQ(list.get_first(), "hello");
  EXPECT_EQ(list.get_last(), "world");
}

// ==================== to_Array Tests ====================

TEST(ToArray, FromEmptyDynList)
{
  DynList<int> list;
  auto arr = to_Array(list);
  EXPECT_EQ(arr.size(), 0);
}

TEST(ToArray, FromSingleElementDynList)
{
  DynList<int> list;
  list.append(99);

  auto arr = to_Array(list);

  ASSERT_EQ(arr.size(), 1);
  EXPECT_EQ(arr(0), 99);
}

TEST(ToArray, FromMultipleElementDynList)
{
  DynList<int> list;
  list.append(10);
  list.append(20);
  list.append(30);

  auto arr = to_Array(list);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 10);
  EXPECT_EQ(arr(1), 20);
  EXPECT_EQ(arr(2), 30);
}

TEST(ToArray, WithStrings)
{
  DynList<std::string> list;
  list.append("foo");
  list.append("bar");
  list.append("baz");

  auto arr = to_Array(list);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), "foo");
  EXPECT_EQ(arr(1), "bar");
  EXPECT_EQ(arr(2), "baz");
}

// ==================== dynarray_to_DynList Tests ====================

TEST(DynarrayToDynList, FromEmptyDynArray)
{
  DynArray<int> arr;
  auto list = dynarray_to_DynList(arr);
  EXPECT_TRUE(list.is_empty());
}

TEST(DynarrayToDynList, FromSingleElementDynArray)
{
  DynArray<int> arr;
  arr.append(77);

  auto list = dynarray_to_DynList(arr);

  ASSERT_EQ(list.size(), 1);
  EXPECT_EQ(list.get_first(), 77);
}

TEST(DynarrayToDynList, FromMultipleElementDynArray)
{
  DynArray<int> arr;
  for (int i = 0; i < 10; ++i)
    arr.append(i * i);

  auto list = dynarray_to_DynList(arr);

  ASSERT_EQ(list.size(), 10);
  int idx = 0;
  for (auto it = list.get_it(); it.has_curr(); it.next_ne(), ++idx)
    EXPECT_EQ(it.get_curr(), idx * idx);
}

// ==================== dynlist_to_DynArray Tests ====================

TEST(DynlistToDynArray, FromEmptyDynList)
{
  DynList<int> list;
  auto arr = dynlist_to_DynArray(list);
  EXPECT_EQ(arr.size(), 0);
}

TEST(DynlistToDynArray, FromSingleElementDynList)
{
  DynList<int> list;
  list.append(88);

  auto arr = dynlist_to_DynArray(list);

  ASSERT_EQ(arr.size(), 1);
  EXPECT_EQ(arr(0), 88);
}

TEST(DynlistToDynArray, FromMultipleElementDynList)
{
  DynList<int> list;
  for (int i = 0; i < 5; ++i)
    list.append(i + 100);

  auto arr = dynlist_to_DynArray(list);

  ASSERT_EQ(arr.size(), 5);
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(arr(i), static_cast<int>(i) + 100);
}

// ==================== to_vector Tests ====================

TEST(ToVector, FromEmptyDynList)
{
  DynList<int> list;
  auto vec = to_vector(list);
  EXPECT_TRUE(vec.empty());
}

TEST(ToVector, FromSingleElementDynList)
{
  DynList<int> list;
  list.append(55);

  auto vec = to_vector(list);

  ASSERT_EQ(vec.size(), 1U);
  EXPECT_EQ(vec[0], 55);
}

TEST(ToVector, FromMultipleElementDynList)
{
  DynList<int> list;
  list.append(1);
  list.append(2);
  list.append(3);

  auto vec = to_vector(list);

  ASSERT_EQ(vec.size(), 3U);
  EXPECT_EQ(vec[0], 1);
  EXPECT_EQ(vec[1], 2);
  EXPECT_EQ(vec[2], 3);
}

TEST(ToVector, FromArray)
{
  Array<double> arr;
  arr.append(1.1);
  arr.append(2.2);
  arr.append(3.3);

  auto vec = to_vector(arr);

  ASSERT_EQ(vec.size(), 3U);
  EXPECT_DOUBLE_EQ(vec[0], 1.1);
  EXPECT_DOUBLE_EQ(vec[1], 2.2);
  EXPECT_DOUBLE_EQ(vec[2], 3.3);
}

// ==================== vector_to_DynList Tests ====================

TEST(VectorToDynList, FromEmptyVector)
{
  std::vector<int> vec;
  auto list = vector_to_DynList(vec);
  EXPECT_TRUE(list.is_empty());
}

TEST(VectorToDynList, FromSingleElementVector)
{
  std::vector<int> vec = {123};
  auto list = vector_to_DynList(vec);

  ASSERT_EQ(list.size(), 1);
  EXPECT_EQ(list.get_first(), 123);
}

TEST(VectorToDynList, FromMultipleElementVector)
{
  std::vector<int> vec = {5, 10, 15, 20};
  auto list = vector_to_DynList(vec);

  ASSERT_EQ(list.size(), 4);
  int expected = 5;
  for (auto it = list.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      expected += 5;
    }
}

// ==================== vector_to_Array Tests ====================

TEST(VectorToArray, FromEmptyVector)
{
  std::vector<int> vec;
  auto arr = vector_to_Array(vec);
  EXPECT_EQ(arr.size(), 0);
}

TEST(VectorToArray, FromSingleElementVector)
{
  std::vector<int> vec = {999};
  auto arr = vector_to_Array(vec);

  ASSERT_EQ(arr.size(), 1);
  EXPECT_EQ(arr(0), 999);
}

TEST(VectorToArray, FromMultipleElementVector)
{
  std::vector<std::string> vec = {"a", "b", "c", "d"};
  auto arr = vector_to_Array(vec);

  ASSERT_EQ(arr.size(), 4);
  EXPECT_EQ(arr(0), "a");
  EXPECT_EQ(arr(1), "b");
  EXPECT_EQ(arr(2), "c");
  EXPECT_EQ(arr(3), "d");
}

// ==================== Round-trip Tests ====================

TEST(RoundTrip, ArrayToDynListAndBack)
{
  Array<int> original;
  original.append(1);
  original.append(2);
  original.append(3);

  auto list = to_DynList(original);
  auto result = to_Array(list);

  ASSERT_EQ(result.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
    EXPECT_EQ(result(i), original(i));
}

TEST(RoundTrip, DynListToDynArrayAndBack)
{
  DynList<int> original;
  original.append(10);
  original.append(20);
  original.append(30);

  auto arr = dynlist_to_DynArray(original);
  auto result = dynarray_to_DynList(arr);

  ASSERT_EQ(result.size(), original.size());
  auto it1 = original.get_it();
  auto it2 = result.get_it();
  while (it1.has_curr())
    {
      EXPECT_EQ(it1.get_curr(), it2.get_curr());
      it1.next_ne();
      it2.next_ne();
    }
}

TEST(RoundTrip, VectorToDynListAndBack)
{
  std::vector<int> original = {100, 200, 300, 400};
  auto list = vector_to_DynList(original);
  auto result = to_vector(list);

  ASSERT_EQ(result.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
    EXPECT_EQ(result[i], original[i]);
}

TEST(RoundTrip, VectorToArrayAndBack)
{
  std::vector<double> original = {1.5, 2.5, 3.5};
  auto arr = vector_to_Array(original);
  auto result = to_vector(arr);

  ASSERT_EQ(result.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
    EXPECT_DOUBLE_EQ(result[i], original[i]);
}

// ==================== Large Container Tests ====================

TEST(LargeContainers, DynArrayConversions)
{
  DynArray<int> arr;
  constexpr size_t SIZE = 10000;

  for (size_t i = 0; i < SIZE; ++i)
    arr.append(static_cast<int>(i));

  auto list = dynarray_to_DynList(arr);
  ASSERT_EQ(list.size(), SIZE);

  auto arr2 = dynlist_to_DynArray(list);
  ASSERT_EQ(arr2.size(), SIZE);

  for (size_t i = 0; i < SIZE; ++i)
    EXPECT_EQ(arr2(i), static_cast<int>(i));
}

TEST(LargeContainers, VectorConversions)
{
  std::vector<int> vec;
  constexpr size_t SIZE = 10000;

  for (size_t i = 0; i < SIZE; ++i)
    vec.push_back(static_cast<int>(i * 2));

  auto list = vector_to_DynList(vec);
  ASSERT_EQ(list.size(), SIZE);

  auto result = to_vector(list);
  ASSERT_EQ(result.size(), SIZE);

  for (size_t i = 0; i < SIZE; ++i)
    EXPECT_EQ(result[i], static_cast<int>(i * 2));
}

// ==================== vector_to_DynArray Tests ====================

TEST(VectorToDynArray, FromEmptyVector)
{
  std::vector<int> vec;
  auto arr = vector_to_DynArray(vec);
  EXPECT_EQ(arr.size(), 0);
}

TEST(VectorToDynArray, FromSingleElementVector)
{
  std::vector<int> vec = {42};
  auto arr = vector_to_DynArray(vec);

  ASSERT_EQ(arr.size(), 1);
  EXPECT_EQ(arr(0), 42);
}

TEST(VectorToDynArray, FromMultipleElementVector)
{
  std::vector<std::string> vec = {"alpha", "beta", "gamma"};
  auto arr = vector_to_DynArray(vec);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), "alpha");
  EXPECT_EQ(arr(1), "beta");
  EXPECT_EQ(arr(2), "gamma");
}

// ==================== to_DynArray Tests ====================

TEST(ToDynArray, FromEmptyDynList)
{
  DynList<int> list;
  auto arr = to_DynArray(list);
  EXPECT_EQ(arr.size(), 0);
}

TEST(ToDynArray, FromMultipleElementDynList)
{
  DynList<int> list;
  list.append(7);
  list.append(14);
  list.append(21);

  auto arr = to_DynArray(list);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 7);
  EXPECT_EQ(arr(1), 14);
  EXPECT_EQ(arr(2), 21);
}

TEST(ToDynArray, FromArray)
{
  Array<double> source;
  source.append(1.5);
  source.append(2.5);

  auto arr = to_DynArray(source);

  ASSERT_EQ(arr.size(), 2);
  EXPECT_DOUBLE_EQ(arr(0), 1.5);
  EXPECT_DOUBLE_EQ(arr(1), 2.5);
}

// ==================== array_to_DynArray Tests ====================

TEST(ArrayToDynArray, FromEmptyArray)
{
  Array<int> source;
  auto arr = array_to_DynArray(source);
  EXPECT_EQ(arr.size(), 0);
}

TEST(ArrayToDynArray, FromMultipleElementArray)
{
  Array<int> source;
  source.append(10);
  source.append(20);
  source.append(30);

  auto arr = array_to_DynArray(source);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 10);
  EXPECT_EQ(arr(1), 20);
  EXPECT_EQ(arr(2), 30);
}

// ==================== dynarray_to_Array Tests ====================

TEST(DynarrayToArray, FromEmptyDynArray)
{
  DynArray<int> source;
  auto arr = dynarray_to_Array(source);
  EXPECT_EQ(arr.size(), 0);
}

TEST(DynarrayToArray, FromMultipleElementDynArray)
{
  DynArray<int> source;
  source.append(100);
  source.append(200);
  source.append(300);

  auto arr = dynarray_to_Array(source);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 100);
  EXPECT_EQ(arr(1), 200);
  EXPECT_EQ(arr(2), 300);
}

// ==================== to_deque Tests ====================

TEST(ToDeque, FromEmptyDynList)
{
  DynList<int> list;
  auto deq = to_deque(list);
  EXPECT_TRUE(deq.empty());
}

TEST(ToDeque, FromMultipleElementDynList)
{
  DynList<int> list;
  list.append(1);
  list.append(2);
  list.append(3);

  auto deq = to_deque(list);

  ASSERT_EQ(deq.size(), 3U);
  EXPECT_EQ(deq[0], 1);
  EXPECT_EQ(deq[1], 2);
  EXPECT_EQ(deq[2], 3);
}

TEST(ToDeque, FromArray)
{
  Array<std::string> arr;
  arr.append("x");
  arr.append("y");

  auto deq = to_deque(arr);

  ASSERT_EQ(deq.size(), 2U);
  EXPECT_EQ(deq[0], "x");
  EXPECT_EQ(deq[1], "y");
}

// ==================== deque_to_* Tests ====================

TEST(DequeToDynList, FromEmptyDeque)
{
  std::deque<int> deq;
  auto list = deque_to_DynList(deq);
  EXPECT_TRUE(list.is_empty());
}

TEST(DequeToDynList, FromMultipleElementDeque)
{
  std::deque<int> deq = {5, 10, 15};
  auto list = deque_to_DynList(deq);

  ASSERT_EQ(list.size(), 3);
  int expected = 5;
  for (auto it = list.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      expected += 5;
    }
}

TEST(DequeToArray, FromEmptyDeque)
{
  std::deque<int> deq;
  auto arr = deque_to_Array(deq);
  EXPECT_EQ(arr.size(), 0);
}

TEST(DequeToArray, FromMultipleElementDeque)
{
  std::deque<int> deq = {11, 22, 33, 44};
  auto arr = deque_to_Array(deq);

  ASSERT_EQ(arr.size(), 4);
  EXPECT_EQ(arr(0), 11);
  EXPECT_EQ(arr(1), 22);
  EXPECT_EQ(arr(2), 33);
  EXPECT_EQ(arr(3), 44);
}

TEST(DequeToDynArray, FromEmptyDeque)
{
  std::deque<int> deq;
  auto arr = deque_to_DynArray(deq);
  EXPECT_EQ(arr.size(), 0);
}

TEST(DequeToDynArray, FromMultipleElementDeque)
{
  std::deque<double> deq = {1.1, 2.2, 3.3};
  auto arr = deque_to_DynArray(deq);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_DOUBLE_EQ(arr(0), 1.1);
  EXPECT_DOUBLE_EQ(arr(1), 2.2);
  EXPECT_DOUBLE_EQ(arr(2), 3.3);
}

// ==================== DynDlist Conversion Tests ====================

TEST(DyndlistToDynList, FromEmptyDynDlist)
{
  DynDlist<int> dlist;
  auto list = dyndlist_to_DynList(dlist);
  EXPECT_TRUE(list.is_empty());
}

TEST(DyndlistToDynList, FromMultipleElementDynDlist)
{
  DynDlist<int> dlist;
  dlist.append(1);
  dlist.append(2);
  dlist.append(3);

  auto list = dyndlist_to_DynList(dlist);

  ASSERT_EQ(list.size(), 3);
  int expected = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      ++expected;
    }
}

TEST(DynlistToDynDlist, FromEmptyDynList)
{
  DynList<int> list;
  auto dlist = dynlist_to_DynDlist(list);
  EXPECT_TRUE(dlist.is_empty());
}

TEST(DynlistToDynDlist, FromMultipleElementDynList)
{
  DynList<int> list;
  list.append(10);
  list.append(20);
  list.append(30);

  auto dlist = dynlist_to_DynDlist(list);

  ASSERT_EQ(dlist.size(), 3);
  int expected = 10;
  for (auto it = dlist.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      expected += 10;
    }
}

TEST(DyndlistToDynArray, FromEmptyDynDlist)
{
  DynDlist<int> dlist;
  auto arr = dyndlist_to_DynArray(dlist);
  EXPECT_EQ(arr.size(), 0);
}

TEST(DyndlistToDynArray, FromMultipleElementDynDlist)
{
  DynDlist<int> dlist;
  dlist.append(7);
  dlist.append(8);
  dlist.append(9);

  auto arr = dyndlist_to_DynArray(dlist);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 7);
  EXPECT_EQ(arr(1), 8);
  EXPECT_EQ(arr(2), 9);
}

TEST(DynarrayToDynDlist, FromEmptyDynArray)
{
  DynArray<int> arr;
  auto dlist = dynarray_to_DynDlist(arr);
  EXPECT_TRUE(dlist.is_empty());
}

TEST(DynarrayToDynDlist, FromMultipleElementDynArray)
{
  DynArray<int> arr;
  arr.append(100);
  arr.append(200);
  arr.append(300);

  auto dlist = dynarray_to_DynDlist(arr);

  ASSERT_EQ(dlist.size(), 3);
  int expected = 100;
  for (auto it = dlist.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      expected += 100;
    }
}

TEST(VectorToDynDlist, FromEmptyVector)
{
  std::vector<int> vec;
  auto dlist = vector_to_DynDlist(vec);
  EXPECT_TRUE(dlist.is_empty());
}

TEST(VectorToDynDlist, FromMultipleElementVector)
{
  std::vector<std::string> vec = {"a", "b", "c"};
  auto dlist = vector_to_DynDlist(vec);

  ASSERT_EQ(dlist.size(), 3);
  auto it = dlist.get_it();
  EXPECT_EQ(it.get_curr(), "a");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "b");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "c");
}

// ==================== Additional Round-trip Tests ====================

TEST(RoundTrip, VectorToDynArrayAndBack)
{
  std::vector<int> original = {1, 2, 3, 4, 5};
  auto arr = vector_to_DynArray(original);
  auto result = to_vector(arr);

  ASSERT_EQ(result.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
    EXPECT_EQ(result[i], original[i]);
}

TEST(RoundTrip, DynDlistToDynArrayAndBack)
{
  DynDlist<int> original;
  original.append(10);
  original.append(20);
  original.append(30);

  auto arr = dyndlist_to_DynArray(original);
  auto result = dynarray_to_DynDlist(arr);

  ASSERT_EQ(result.size(), original.size());
  auto it1 = original.get_it();
  auto it2 = result.get_it();
  while (it1.has_curr())
    {
      EXPECT_EQ(it1.get_curr(), it2.get_curr());
      it1.next_ne();
      it2.next_ne();
    }
}

TEST(RoundTrip, DequeToArrayAndBack)
{
  std::deque<double> original = {1.5, 2.5, 3.5};
  auto arr = deque_to_Array(original);
  auto result = to_deque(arr);

  ASSERT_EQ(result.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
    EXPECT_DOUBLE_EQ(result[i], original[i]);
}

// ==================== Move Semantics Tests ====================

TEST(MoveSemantics, VectorToDynListMove)
{
  std::vector<std::string> vec = {"hello", "world", "test"};
  size_t original_size = vec.size();

  auto list = vector_to_DynList(std::move(vec));

  ASSERT_EQ(list.size(), original_size);
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), "hello");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "world");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "test");
}

TEST(MoveSemantics, VectorToArrayMove)
{
  std::vector<std::string> vec = {"alpha", "beta"};
  size_t original_size = vec.size();

  auto arr = vector_to_Array(std::move(vec));

  ASSERT_EQ(arr.size(), original_size);
  EXPECT_EQ(arr(0), "alpha");
  EXPECT_EQ(arr(1), "beta");
}

TEST(MoveSemantics, VectorToDynArrayMove)
{
  std::vector<std::string> vec = {"x", "y", "z"};
  size_t original_size = vec.size();

  auto arr = vector_to_DynArray(std::move(vec));

  ASSERT_EQ(arr.size(), original_size);
  EXPECT_EQ(arr(0), "x");
  EXPECT_EQ(arr(1), "y");
  EXPECT_EQ(arr(2), "z");
}

TEST(MoveSemantics, VectorToDynDlistMove)
{
  std::vector<std::string> vec = {"one", "two"};
  size_t original_size = vec.size();

  auto dlist = vector_to_DynDlist(std::move(vec));

  ASSERT_EQ(dlist.size(), original_size);
  auto it = dlist.get_it();
  EXPECT_EQ(it.get_curr(), "one");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "two");
}

// ==================== std::set Conversion Tests ====================

TEST(SetConversions, SetToDynListEmpty)
{
  std::set<int> s;
  auto list = set_to_DynList(s);
  EXPECT_TRUE(list.is_empty());
}

TEST(SetConversions, SetToDynListMultiple)
{
  std::set<int> s = {3, 1, 4, 1, 5, 9}; // duplicates removed, sorted
  auto list = set_to_DynList(s);

  ASSERT_EQ(list.size(), 5); // unique: 1, 3, 4, 5, 9
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), 1);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 3);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 4);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 5);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 9);
}

TEST(SetConversions, SetToArray)
{
  std::set<std::string> s = {"banana", "apple", "cherry"};
  auto arr = set_to_Array(s);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), "apple");  // sorted
  EXPECT_EQ(arr(1), "banana");
  EXPECT_EQ(arr(2), "cherry");
}

TEST(SetConversions, SetToDynArray)
{
  std::set<int> s = {10, 20, 30};
  auto arr = set_to_DynArray(s);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 10);
  EXPECT_EQ(arr(1), 20);
  EXPECT_EQ(arr(2), 30);
}

TEST(SetConversions, ToSetFromDynList)
{
  DynList<int> list;
  list.append(5);
  list.append(3);
  list.append(5); // duplicate
  list.append(1);

  auto s = to_set(list);

  ASSERT_EQ(s.size(), 3U); // unique: 1, 3, 5
  EXPECT_TRUE(s.count(1));
  EXPECT_TRUE(s.count(3));
  EXPECT_TRUE(s.count(5));
}

TEST(SetConversions, ToSetFromArray)
{
  Array<std::string> arr;
  arr.append("a");
  arr.append("b");
  arr.append("a"); // duplicate

  auto s = to_set(arr);

  ASSERT_EQ(s.size(), 2U);
  EXPECT_TRUE(s.count("a"));
  EXPECT_TRUE(s.count("b"));
}

// ==================== std::map Conversion Tests ====================

TEST(MapConversions, MapToDynListEmpty)
{
  std::map<int, std::string> m;
  auto list = map_to_DynList(m);
  EXPECT_TRUE(list.is_empty());
}

TEST(MapConversions, MapToDynListMultiple)
{
  std::map<int, std::string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
  auto list = map_to_DynList(m);

  ASSERT_EQ(list.size(), 3);
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr().first, 1);
  EXPECT_EQ(it.get_curr().second, "one");
  it.next_ne();
  EXPECT_EQ(it.get_curr().first, 2);
  EXPECT_EQ(it.get_curr().second, "two");
}

TEST(MapConversions, MapToArray)
{
  std::map<std::string, int> m = {{"a", 1}, {"b", 2}};
  auto arr = map_to_Array(m);

  ASSERT_EQ(arr.size(), 2);
  EXPECT_EQ(arr(0).first, "a");
  EXPECT_EQ(arr(0).second, 1);
  EXPECT_EQ(arr(1).first, "b");
  EXPECT_EQ(arr(1).second, 2);
}

TEST(MapConversions, MapKeysToDynList)
{
  std::map<std::string, int> m = {{"x", 10}, {"y", 20}, {"z", 30}};
  auto keys = map_keys_to_DynList(m);

  ASSERT_EQ(keys.size(), 3);
  auto it = keys.get_it();
  EXPECT_EQ(it.get_curr(), "x");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "y");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "z");
}

TEST(MapConversions, MapValuesToDynList)
{
  std::map<std::string, int> m = {{"a", 100}, {"b", 200}};
  auto values = map_values_to_DynList(m);

  ASSERT_EQ(values.size(), 2);
  auto it = values.get_it();
  EXPECT_EQ(it.get_curr(), 100);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 200);
}

// ==================== initializer_list Conversion Tests ====================

TEST(InitializerList, InitToDynListEmpty)
{
  auto list = init_to_DynList<int>({});
  EXPECT_TRUE(list.is_empty());
}

TEST(InitializerList, InitToDynListMultiple)
{
  auto list = init_to_DynList({1, 2, 3, 4, 5});

  ASSERT_EQ(list.size(), 5);
  int expected = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next_ne())
    {
      EXPECT_EQ(it.get_curr(), expected);
      ++expected;
    }
}

TEST(InitializerList, InitToArrayEmpty)
{
  auto arr = init_to_Array<int>({});
  EXPECT_EQ(arr.size(), 0);
}

TEST(InitializerList, InitToArrayMultiple)
{
  auto arr = init_to_Array({10, 20, 30});

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 10);
  EXPECT_EQ(arr(1), 20);
  EXPECT_EQ(arr(2), 30);
}

TEST(InitializerList, InitToDynArrayEmpty)
{
  auto arr = init_to_DynArray<int>({});
  EXPECT_EQ(arr.size(), 0);
}

TEST(InitializerList, InitToDynArrayMultiple)
{
  auto arr = init_to_DynArray({100, 200, 300, 400});

  ASSERT_EQ(arr.size(), 4);
  EXPECT_EQ(arr(0), 100);
  EXPECT_EQ(arr(1), 200);
  EXPECT_EQ(arr(2), 300);
  EXPECT_EQ(arr(3), 400);
}

TEST(InitializerList, InitToDynDlistEmpty)
{
  auto dlist = init_to_DynDlist<int>({});
  EXPECT_TRUE(dlist.is_empty());
}

TEST(InitializerList, InitToDynDlistMultiple)
{
  auto dlist = init_to_DynDlist<std::string>({"hello", "world"});

  ASSERT_EQ(dlist.size(), 2);
  auto it = dlist.get_it();
  EXPECT_EQ(it.get_curr(), "hello");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "world");
}

TEST(InitializerList, InitWithStrings)
{
  auto list = init_to_DynList<std::string>({"alpha", "beta", "gamma"});

  ASSERT_EQ(list.size(), 3);
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), "alpha");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "beta");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "gamma");
}

// ==================== DynSetTree Conversion Tests ====================

TEST(DynSetTreeConversions, SetTreeToDynListEmpty)
{
  DynSetTree<int> s;
  auto list = settree_to_DynList(s);
  EXPECT_TRUE(list.is_empty());
}

TEST(DynSetTreeConversions, SetTreeToDynListMultiple)
{
  DynSetTree<int> s = {3, 1, 4, 1, 5, 9}; // duplicates ignored
  auto list = settree_to_DynList(s);

  ASSERT_EQ(list.size(), 5); // unique: 1, 3, 4, 5, 9
  // Elements are in sorted order (in-order traversal)
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), 1);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 3);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 4);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 5);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 9);
}

TEST(DynSetTreeConversions, SetTreeToArray)
{
  DynSetTree<std::string> s = {"banana", "apple", "cherry"};
  auto arr = settree_to_Array(s);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), "apple");  // sorted
  EXPECT_EQ(arr(1), "banana");
  EXPECT_EQ(arr(2), "cherry");
}

TEST(DynSetTreeConversions, SetTreeToDynArray)
{
  DynSetTree<int> s = {10, 20, 30};
  auto arr = settree_to_DynArray(s);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 10);
  EXPECT_EQ(arr(1), 20);
  EXPECT_EQ(arr(2), 30);
}

TEST(DynSetTreeConversions, SetTreeToVector)
{
  DynSetTree<int> s = {5, 2, 8};
  auto vec = settree_to_vector(s);

  ASSERT_EQ(vec.size(), 3U);
  EXPECT_EQ(vec[0], 2);
  EXPECT_EQ(vec[1], 5);
  EXPECT_EQ(vec[2], 8);
}

TEST(DynSetTreeConversions, SetTreeToStdSet)
{
  DynSetTree<int> s = {1, 2, 3};
  auto stdset = settree_to_stdset(s);

  ASSERT_EQ(stdset.size(), 3U);
  EXPECT_TRUE(stdset.count(1));
  EXPECT_TRUE(stdset.count(2));
  EXPECT_TRUE(stdset.count(3));
}

TEST(DynSetTreeConversions, ToDynSetTreeFromDynList)
{
  DynList<int> list;
  list.append(5);
  list.append(3);
  list.append(5); // duplicate
  list.append(1);

  auto s = to_DynSetTree(list);

  ASSERT_EQ(s.size(), 3); // unique: 1, 3, 5
  EXPECT_TRUE(s.contains(1));
  EXPECT_TRUE(s.contains(3));
  EXPECT_TRUE(s.contains(5));
}

TEST(DynSetTreeConversions, VectorToDynSetTree)
{
  std::vector<std::string> vec = {"a", "b", "a", "c"};
  auto s = vector_to_DynSetTree(vec);

  ASSERT_EQ(s.size(), 3);
  EXPECT_TRUE(s.contains("a"));
  EXPECT_TRUE(s.contains("b"));
  EXPECT_TRUE(s.contains("c"));
}

// ==================== DynSetHash Conversion Tests ====================

TEST(DynSetHashConversions, SetHashToDynList)
{
  DynSetLhash<int> s;
  s.insert(10);
  s.insert(20);
  s.insert(30);

  auto list = sethash_to_DynList(s);
  ASSERT_EQ(list.size(), 3);

  // Verify all elements are present (order is hash-dependent)
  DynSetTree<int> check;
  list.for_each([&check](int x) { check.insert(x); });
  EXPECT_TRUE(check.contains(10));
  EXPECT_TRUE(check.contains(20));
  EXPECT_TRUE(check.contains(30));
}

TEST(DynSetHashConversions, SetHashToArray)
{
  DynSetLhash<std::string> s;
  s.insert("x");
  s.insert("y");

  auto arr = sethash_to_Array(s);
  ASSERT_EQ(arr.size(), 2);

  DynSetTree<std::string> check;
  for (size_t i = 0; i < arr.size(); ++i)
    check.insert(arr(i));
  EXPECT_TRUE(check.contains("x"));
  EXPECT_TRUE(check.contains("y"));
}

TEST(DynSetHashConversions, SetHashToDynArray)
{
  DynSetLhash<int> s;
  s.insert(1);
  s.insert(2);
  s.insert(3);

  auto arr = sethash_to_DynArray(s);
  ASSERT_EQ(arr.size(), 3);

  DynSetTree<int> check;
  for (size_t i = 0; i < arr.size(); ++i)
    check.insert(arr(i));
  EXPECT_TRUE(check.contains(1));
  EXPECT_TRUE(check.contains(2));
  EXPECT_TRUE(check.contains(3));
}

TEST(DynSetHashConversions, SetHashToVector)
{
  DynSetLhash<int> s;
  s.insert(100);
  s.insert(200);

  auto vec = sethash_to_vector(s);
  ASSERT_EQ(vec.size(), 2U);

  std::set<int> check(vec.begin(), vec.end());
  EXPECT_TRUE(check.count(100));
  EXPECT_TRUE(check.count(200));
}

// ==================== DynMapTree Conversion Tests ====================

TEST(DynMapTreeConversions, MapTreeToDynListEmpty)
{
  DynMapTree<int, std::string> m;
  auto list = maptree_to_DynList(m);
  EXPECT_TRUE(list.is_empty());
}

TEST(DynMapTreeConversions, MapTreeToDynListMultiple)
{
  DynMapTree<int, std::string> m;
  m.insert(1, "one");
  m.insert(2, "two");
  m.insert(3, "three");

  auto list = maptree_to_DynList(m);
  ASSERT_EQ(list.size(), 3);

  // Verify pairs (in sorted key order)
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr().first, 1);
  EXPECT_EQ(it.get_curr().second, "one");
  it.next_ne();
  EXPECT_EQ(it.get_curr().first, 2);
  EXPECT_EQ(it.get_curr().second, "two");
  it.next_ne();
  EXPECT_EQ(it.get_curr().first, 3);
  EXPECT_EQ(it.get_curr().second, "three");
}

TEST(DynMapTreeConversions, MapTreeToArray)
{
  DynMapTree<std::string, int> m;
  m.insert("a", 1);
  m.insert("b", 2);

  auto arr = maptree_to_Array(m);
  ASSERT_EQ(arr.size(), 2);
  EXPECT_EQ(arr(0).first, "a");
  EXPECT_EQ(arr(0).second, 1);
  EXPECT_EQ(arr(1).first, "b");
  EXPECT_EQ(arr(1).second, 2);
}

TEST(DynMapTreeConversions, MapTreeToStdMap)
{
  DynMapTree<std::string, int> m;
  m.insert("x", 10);
  m.insert("y", 20);

  auto stdmap = maptree_to_stdmap(m);
  ASSERT_EQ(stdmap.size(), 2U);
  EXPECT_EQ(stdmap["x"], 10);
  EXPECT_EQ(stdmap["y"], 20);
}

TEST(DynMapTreeConversions, MapTreeKeysToDynList)
{
  DynMapTree<std::string, int> m;
  m.insert("alpha", 1);
  m.insert("beta", 2);
  m.insert("gamma", 3);

  auto keys = maptree_keys_to_DynList(m);
  ASSERT_EQ(keys.size(), 3);

  auto it = keys.get_it();
  EXPECT_EQ(it.get_curr(), "alpha");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "beta");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "gamma");
}

TEST(DynMapTreeConversions, MapTreeValuesToDynList)
{
  DynMapTree<int, std::string> m;
  m.insert(1, "a");
  m.insert(2, "b");

  auto values = maptree_values_to_DynList(m);
  ASSERT_EQ(values.size(), 2);

  auto it = values.get_it();
  EXPECT_EQ(it.get_curr(), "a");
  it.next_ne();
  EXPECT_EQ(it.get_curr(), "b");
}

TEST(DynMapTreeConversions, StdMapToDynMapTree)
{
  std::map<std::string, int> stdmap = {{"a", 1}, {"b", 2}, {"c", 3}};
  auto m = stdmap_to_DynMapTree(stdmap);

  ASSERT_EQ(m.size(), 3);
  EXPECT_EQ(m["a"], 1);
  EXPECT_EQ(m["b"], 2);
  EXPECT_EQ(m["c"], 3);
}

// ==================== MapOpenHash Conversion Tests ====================

TEST(MapHashConversions, MapHashToDynList)
{
  MapODhash<std::string, int> m;
  m.insert("one", 1);
  m.insert("two", 2);

  auto list = maphash_to_DynList(m);
  ASSERT_EQ(list.size(), 2);

  // Verify all pairs are present (order is hash-dependent)
  std::map<std::string, int> check;
  list.for_each([&check](const auto & p) { check[p.first] = p.second; });
  EXPECT_EQ(check["one"], 1);
  EXPECT_EQ(check["two"], 2);
}

TEST(MapHashConversions, MapHashToArray)
{
  MapODhash<int, std::string> m;
  m.insert(10, "ten");
  m.insert(20, "twenty");

  auto arr = maphash_to_Array(m);
  ASSERT_EQ(arr.size(), 2);

  std::map<int, std::string> check;
  for (size_t i = 0; i < arr.size(); ++i)
    check[arr(i).first] = arr(i).second;
  EXPECT_EQ(check[10], "ten");
  EXPECT_EQ(check[20], "twenty");
}

TEST(MapHashConversions, MapHashToStdMap)
{
  MapODhash<std::string, int> m;
  m.insert("x", 100);
  m.insert("y", 200);

  auto stdmap = maphash_to_stdmap(m);
  ASSERT_EQ(stdmap.size(), 2U);
  EXPECT_EQ(stdmap["x"], 100);
  EXPECT_EQ(stdmap["y"], 200);
}

TEST(MapHashConversions, MapHashKeysToDynList)
{
  MapODhash<std::string, int> m;
  m.insert("a", 1);
  m.insert("b", 2);

  auto keys = maphash_keys_to_DynList(m);
  ASSERT_EQ(keys.size(), 2);

  DynSetTree<std::string> check;
  keys.for_each([&check](const auto & k) { check.insert(k); });
  EXPECT_TRUE(check.contains("a"));
  EXPECT_TRUE(check.contains("b"));
}

TEST(MapHashConversions, MapHashValuesToDynList)
{
  MapODhash<std::string, int> m;
  m.insert("x", 10);
  m.insert("y", 20);

  auto values = maphash_values_to_DynList(m);
  ASSERT_EQ(values.size(), 2);

  DynSetTree<int> check;
  values.for_each([&check](int v) { check.insert(v); });
  EXPECT_TRUE(check.contains(10));
  EXPECT_TRUE(check.contains(20));
}

// ==================== vector_to_DynSetTree Tests ====================

TEST(VectorToDynSetTree, FromEmptyVector)
{
  std::vector<int> vec;
  auto s = vector_to_DynSetTree(vec);
  EXPECT_TRUE(s.is_empty());
}

TEST(VectorToDynSetTree, FromSingleElementVector)
{
  std::vector<int> vec = {42};
  auto s = vector_to_DynSetTree(vec);

  ASSERT_EQ(s.size(), 1);
  EXPECT_TRUE(s.contains(42));
}

TEST(VectorToDynSetTree, FromMultipleElementVector)
{
  std::vector<int> vec = {5, 2, 8, 2, 1, 8, 3};
  auto s = vector_to_DynSetTree(vec);

  ASSERT_EQ(s.size(), 5); // unique: 1, 2, 3, 5, 8
  EXPECT_TRUE(s.contains(1));
  EXPECT_TRUE(s.contains(2));
  EXPECT_TRUE(s.contains(3));
  EXPECT_TRUE(s.contains(5));
  EXPECT_TRUE(s.contains(8));
}

TEST(VectorToDynSetTree, WithStrings)
{
  std::vector<std::string> vec = {"cat", "dog", "cat", "bird", "dog"};
  auto s = vector_to_DynSetTree(vec);

  ASSERT_EQ(s.size(), 3); // unique: bird, cat, dog
  EXPECT_TRUE(s.contains("bird"));
  EXPECT_TRUE(s.contains("cat"));
  EXPECT_TRUE(s.contains("dog"));
  EXPECT_FALSE(s.contains("fish"));
}

TEST(VectorToDynSetTree, RoundTrip)
{
  std::vector<int> original = {7, 3, 9, 3, 1};
  auto s = vector_to_DynSetTree(original);
  auto result = settree_to_vector(s);

  ASSERT_EQ(s.size(), 4U); // unique: 1, 3, 7, 9
  // Vector from tree is sorted
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 3);
  EXPECT_EQ(result[2], 7);
  EXPECT_EQ(result[3], 9);
}

// ==================== Generic to_Array() Tests ====================

TEST(GenericToArray, FromDynList)
{
  DynList<int> list;
  list.append(10);
  list.append(20);
  list.append(30);

  auto arr = to_Array(list);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_EQ(arr(0), 10);
  EXPECT_EQ(arr(1), 20);
  EXPECT_EQ(arr(2), 30);
}

TEST(GenericToArray, FromDynArray)
{
  DynArray<double> darray;
  darray.append(1.5);
  darray.append(2.5);
  darray.append(3.5);

  auto arr = to_Array(darray);

  ASSERT_EQ(arr.size(), 3);
  EXPECT_DOUBLE_EQ(arr(0), 1.5);
  EXPECT_DOUBLE_EQ(arr(1), 2.5);
  EXPECT_DOUBLE_EQ(arr(2), 3.5);
}

TEST(GenericToArray, FromDynSetTree)
{
  DynSetTree<std::string> stree;
  stree.insert("zebra");
  stree.insert("apple");
  stree.insert("mango");

  auto arr = to_Array(stree);

  ASSERT_EQ(arr.size(), 3);
  // SetTree elements are sorted
  EXPECT_EQ(arr(0), "apple");
  EXPECT_EQ(arr(1), "mango");
  EXPECT_EQ(arr(2), "zebra");
}

TEST(GenericToArray, FromEmptyContainer)
{
  DynList<int> list;
  auto arr = to_Array(list);
  EXPECT_EQ(arr.size(), 0);
}

// ==================== Generic to_DynArray() Tests ====================

TEST(GenericToDynArray, FromDynList)
{
  DynList<int> list;
  list.append(5);
  list.append(10);

  auto darr = to_DynArray(list);

  ASSERT_EQ(darr.size(), 2);
  EXPECT_EQ(darr(0), 5);
  EXPECT_EQ(darr(1), 10);
}

TEST(GenericToDynArray, FromDynSetTree)
{
  DynSetTree<int> stree;
  stree.insert(50);
  stree.insert(10);
  stree.insert(30);

  auto darr = to_DynArray(stree);

  ASSERT_EQ(darr.size(), 3);
  EXPECT_EQ(darr(0), 10);
  EXPECT_EQ(darr(1), 30);
  EXPECT_EQ(darr(2), 50);
}

TEST(GenericToDynArray, FromDynDlist)
{
  DynDlist<std::string> dlist;
  dlist.append("foo");
  dlist.append("bar");
  dlist.append("baz");

  auto darr = to_DynArray(dlist);

  ASSERT_EQ(darr.size(), 3);
  EXPECT_EQ(darr(0), "foo");
  EXPECT_EQ(darr(1), "bar");
  EXPECT_EQ(darr(2), "baz");
}

TEST(GenericToDynArray, FromEmptyContainer)
{
  DynList<int> list;
  auto darr = to_DynArray(list);
  EXPECT_EQ(darr.size(), 0);
}

TEST(GenericToDynArray, RoundTripWithDynList)
{
  DynList<int> original;
  original.append(100);
  original.append(200);
  original.append(300);

  auto darr = to_DynArray(original);
  auto list = dynarray_to_DynList(darr);

  ASSERT_EQ(list.size(), 3);
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), 100);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 200);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 300);
}
