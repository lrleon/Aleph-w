#include <gtest/gtest.h>
#include <string>

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

