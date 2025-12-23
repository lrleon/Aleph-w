/**
 * @file ah-stl-utils.cc
 * @brief Comprehensive test suite for ah-stl-utils.H
 *
 * Tests all conversion functions between Aleph-w containers and STL containers,
 * tuple utilities, and variadic helpers.
 */

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <set>
#include <string>
#include <tuple>

#include <ah-stl-utils.H>
#include <ahFunctional.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Test Suite: vector_to_DynList / to_DynList
// ============================================================================

TEST(VectorToDynList, EmptyVector)
{
  vector<int> empty;
  auto result = vector_to_DynList(empty);
  EXPECT_TRUE(result.is_empty());
  EXPECT_EQ(result.size(), 0);
}

TEST(VectorToDynList, SingleElement)
{
  vector<int> v = {42};
  auto result = vector_to_DynList(v);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.get_first(), 42);
}

TEST(VectorToDynList, MultipleElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto result = vector_to_DynList(v);
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result, build_dynlist<int>(1, 2, 3, 4, 5));
}

TEST(VectorToDynList, StringVector)
{
  vector<string> v = {"hello", "world", "test"};
  auto result = vector_to_DynList(v);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result.get_first(), "hello");
  EXPECT_EQ(result.get_last(), "test");
}

TEST(VectorToDynList, ToListAlias)
{
  vector<double> v = {1.5, 2.5, 3.5};
  auto result = to_DynList(v);
  EXPECT_EQ(result.size(), 3);
  EXPECT_DOUBLE_EQ(result.get_first(), 1.5);
}

// ============================================================================
// Test Suite: range_to_DynList
// ============================================================================

TEST(RangeToDynList, VectorIterators)
{
  vector<int> v = {10, 20, 30, 40, 50};
  auto result = range_to_DynList(v.begin(), v.end());
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result, build_dynlist<int>(10, 20, 30, 40, 50));
}

TEST(RangeToDynList, PartialRange)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto result = range_to_DynList(v.begin() + 1, v.begin() + 4);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, build_dynlist<int>(2, 3, 4));
}

TEST(RangeToDynList, EmptyRange)
{
  vector<int> v = {1, 2, 3};
  auto result = range_to_DynList(v.begin(), v.begin());
  EXPECT_TRUE(result.is_empty());
}

TEST(RangeToDynList, ListIterators)
{
  list<string> l = {"a", "b", "c"};
  auto result = range_to_DynList(l.begin(), l.end());
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result.get_first(), "a");
}

// ============================================================================
// Test Suite: to_Array
// ============================================================================

TEST(ToArray, EmptyVector)
{
  vector<int> empty;
  auto result = to_Array(empty);
  EXPECT_EQ(result.size(), 0);
}

TEST(ToArray, MultipleElements)
{
  vector<int> v = {10, 20, 30};
  auto result = to_Array(v);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result(0), 10);
  EXPECT_EQ(result(1), 20);
  EXPECT_EQ(result(2), 30);
}

// ============================================================================
// Test Suite: to_vector
// ============================================================================

TEST(ToVector, FromDynList)
{
  DynList<int> l = build_dynlist<int>(5, 10, 15, 20);
  auto result = to_vector(l);
  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 5);
  EXPECT_EQ(result[1], 10);
  EXPECT_EQ(result[2], 15);
  EXPECT_EQ(result[3], 20);
}

TEST(ToVector, EmptyDynList)
{
  DynList<int> empty;
  auto result = to_vector(empty);
  EXPECT_TRUE(result.empty());
}

TEST(ToVector, FromDynSetTree)
{
  DynSetTree<int> tree;
  tree.insert(3);
  tree.insert(1);
  tree.insert(2);
  auto result = to_vector(tree);
  ASSERT_EQ(result.size(), 3);
  // DynSetTree keeps elements sorted
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

// ============================================================================
// Test Suite: map_vector
// ============================================================================

TEST(MapVector, SquareElements)
{
  vector<int> v = {1, 2, 3, 4, 5};
  auto result = map_vector(v, [](int x) { return x * x; });
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 4);
  EXPECT_EQ(result[2], 9);
  EXPECT_EQ(result[3], 16);
  EXPECT_EQ(result[4], 25);
}

TEST(MapVector, TypeConversion)
{
  vector<int> v = {1, 2, 3};
  auto result = map_vector(v, [](int x) { return to_string(x); });
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "1");
  EXPECT_EQ(result[1], "2");
  EXPECT_EQ(result[2], "3");
}

TEST(MapVector, EmptyVector)
{
  vector<int> empty;
  auto result = map_vector(empty, [](int x) { return x * 2; });
  EXPECT_TRUE(result.empty());
}

// ============================================================================
// Test Suite: variadic_to_vector / variadic_to_DynList
// ============================================================================

TEST(VariadicToVector, MultipleInts)
{
  auto result = variadic_to_vector<int>(1, 2, 3, 4, 5);
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[4], 5);
}

TEST(VariadicToVector, SingleElement)
{
  auto result = variadic_to_vector<int>(42);
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 42);
}

TEST(VariadicToVector, MixedConvertibleTypes)
{
  // All should convert to double
  auto result = variadic_to_vector<double>(1, 2.5, 3);
  ASSERT_EQ(result.size(), 3);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_DOUBLE_EQ(result[1], 2.5);
  EXPECT_DOUBLE_EQ(result[2], 3.0);
}

TEST(VariadicToDynList, MultipleElements)
{
  auto result = variadic_to_DynList<int>(10, 20, 30);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, build_dynlist<int>(10, 20, 30));
}

TEST(VariadicToDynList, Strings)
{
  auto result = variadic_to_DynList<string>("a", "b", "c");
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result.get_first(), "a");
}

// ============================================================================
// Test Suite: tuple_for_each
// ============================================================================

TEST(TupleForEach, SumElements)
{
  auto t = make_tuple(1, 2, 3, 4, 5);
  int sum = 0;
  tuple_for_each(t, [&sum](int x) { sum += x; });
  EXPECT_EQ(sum, 15);
}

TEST(TupleForEach, CollectStrings)
{
  auto t = make_tuple(string("a"), string("b"), string("c"));
  string result;
  tuple_for_each(t, [&result](const string& s) { result += s; });
  EXPECT_EQ(result, "abc");
}

TEST(TupleForEach, EmptyTuple)
{
  auto t = make_tuple();
  int count = 0;
  tuple_for_each(t, [&count](auto) { ++count; });
  EXPECT_EQ(count, 0);
}

TEST(TupleForEach, SingleElement)
{
  auto t = make_tuple(42);
  int value = 0;
  tuple_for_each(t, [&value](int x) { value = x; });
  EXPECT_EQ(value, 42);
}

// ============================================================================
// Test Suite: tuple_to_container / tuple_to_dynlist / tuple_to_array
// ============================================================================

TEST(TupleToDynList, HomogeneousTuple)
{
  auto t = make_tuple(1, 2, 3, 4, 5);
  auto result = tuple_to_dynlist(t);
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result, build_dynlist<int>(1, 2, 3, 4, 5));
}

TEST(TupleToDynList, SingleElement)
{
  auto t = make_tuple(42);
  auto result = tuple_to_dynlist(t);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.get_first(), 42);
}

TEST(TupleToDynList, StringTuple)
{
  auto t = make_tuple(string("hello"), string("world"));
  auto result = tuple_to_dynlist(t);
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result.get_first(), "hello");
  EXPECT_EQ(result.get_last(), "world");
}

TEST(TupleToArray, HomogeneousTuple)
{
  auto t = make_tuple(10, 20, 30);
  auto result = tuple_to_array(t);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result(0), 10);
  EXPECT_EQ(result(1), 20);
  EXPECT_EQ(result(2), 30);
}

// ============================================================================
// Test Suite: stl_container_to_dynList
// ============================================================================

TEST(StlContainerToDynList, FromStdVector)
{
  vector<int> v = {1, 2, 3};
  auto result = stl_container_to_dynList(v);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, build_dynlist<int>(1, 2, 3));
}

TEST(StlContainerToDynList, FromStdList)
{
  list<int> l = {4, 5, 6};
  auto result = stl_container_to_dynList(l);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, build_dynlist<int>(4, 5, 6));
}

TEST(StlContainerToDynList, FromStdSet)
{
  set<int> s = {3, 1, 2};  // Will be sorted
  auto result = stl_container_to_dynList(s);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, build_dynlist<int>(1, 2, 3));
}

TEST(StlContainerToDynList, Empty)
{
  vector<int> empty;
  auto result = stl_container_to_dynList(empty);
  EXPECT_TRUE(result.is_empty());
}

// ============================================================================
// Test Suite: list_to_DynList / DynList_to_list
// ============================================================================

TEST(ListConversions, ListToDynList)
{
  list<int> l = {1, 2, 3, 4};
  auto result = list_to_DynList(l);
  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result, build_dynlist<int>(1, 2, 3, 4));
}

TEST(ListConversions, DynListToList)
{
  DynList<int> dl = build_dynlist<int>(5, 6, 7);
  auto result = DynList_to_list(dl);
  ASSERT_EQ(result.size(), 3);
  auto it = result.begin();
  EXPECT_EQ(*it++, 5);
  EXPECT_EQ(*it++, 6);
  EXPECT_EQ(*it++, 7);
}

TEST(ListConversions, RoundTrip)
{
  list<string> original = {"a", "b", "c", "d"};
  auto dynlist = list_to_DynList(original);
  auto back = DynList_to_list(dynlist);
  EXPECT_EQ(original, back);
}

TEST(ListConversions, Empty)
{
  list<int> empty_list;
  auto result = list_to_DynList(empty_list);
  EXPECT_TRUE(result.is_empty());

  DynList<int> empty_dynlist;
  auto back = DynList_to_list(empty_dynlist);
  EXPECT_TRUE(back.empty());
}

// ============================================================================
// Test Suite: DynArray_to_vector / vector_to_DynArray
// ============================================================================

TEST(DynArrayConversions, DynArrayToVector)
{
  DynArray<int> arr;
  arr.append(10);
  arr.append(20);
  arr.append(30);
  arr.append(40);
  arr.append(50);

  auto result = DynArray_to_vector(arr);
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 10);
  EXPECT_EQ(result[2], 30);
  EXPECT_EQ(result[4], 50);
}

TEST(DynArrayConversions, VectorToDynArray)
{
  vector<int> v = {100, 200, 300};
  auto result = vector_to_DynArray(v);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result(0), 100);
  EXPECT_EQ(result(1), 200);
  EXPECT_EQ(result(2), 300);
}

TEST(DynArrayConversions, RoundTrip)
{
  vector<double> original = {1.1, 2.2, 3.3, 4.4};
  auto arr = vector_to_DynArray(original);
  auto back = DynArray_to_vector(arr);
  EXPECT_EQ(original, back);
}

TEST(DynArrayConversions, Empty)
{
  vector<int> empty_vec;
  auto arr = vector_to_DynArray(empty_vec);
  EXPECT_EQ(arr.size(), 0);

  auto back = DynArray_to_vector(arr);
  EXPECT_TRUE(back.empty());
}

// ============================================================================
// Test Suite: index_tuple / make_index_tuple
// ============================================================================

TEST(IndexTuple, MakeIndexTuple)
{
  // Test that make_index_tuple<N> creates an index_tuple with indices 0..N-1
  using IT3 = make_index_tuple<3>::type;
  static_assert(std::is_same_v<IT3, index_tuple<0, 1, 2>>,
                "make_index_tuple<3> should produce index_tuple<0, 1, 2>");

  using IT0 = make_index_tuple<0>::type;
  static_assert(std::is_same_v<IT0, index_tuple<>>,
                "make_index_tuple<0> should produce index_tuple<>");

  using IT1 = make_index_tuple<1>::type;
  static_assert(std::is_same_v<IT1, index_tuple<0>>,
                "make_index_tuple<1> should produce index_tuple<0>");
}

TEST(IndexTuple, ToIndexTuple)
{
  using IT = to_index_tuple<int, double, string>;
  static_assert(std::is_same_v<IT, index_tuple<0, 1, 2>>,
                "to_index_tuple with 3 types should produce index_tuple<0, 1, 2>");
}

// ============================================================================
// Test Suite: Edge Cases and Stress Tests
// ============================================================================

TEST(EdgeCases, LargeVector)
{
  const size_t N = 10000;
  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(i);

  auto result = vector_to_DynList(v);
  EXPECT_EQ(result.size(), N);
  EXPECT_EQ(result.get_first(), 0);
  EXPECT_EQ(result.get_last(), static_cast<int>(N - 1));

  auto back = to_vector(result);
  EXPECT_EQ(v, back);
}

TEST(EdgeCases, ComplexType)
{
  struct Point {
    double x, y;
    bool operator==(const Point& other) const {
      return x == other.x && y == other.y;
    }
  };

  vector<Point> v = {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
  auto dynlist = vector_to_DynList(v);
  EXPECT_EQ(dynlist.size(), 3);

  auto back = to_vector(dynlist);
  EXPECT_EQ(v, back);
}

TEST(EdgeCases, NestedContainers)
{
  vector<vector<int>> v = {{1, 2}, {3, 4, 5}, {6}};
  auto dynlist = vector_to_DynList(v);
  EXPECT_EQ(dynlist.size(), 3);

  auto back = to_vector(dynlist);
  EXPECT_EQ(v, back);
}

TEST(EdgeCases, CopySemantics)
{
  vector<string> v = {"long string that should not be copied",
                      "another long string for testing"};

  // Conversion makes copies, original should be unchanged
  auto dynlist = vector_to_DynList(v);
  EXPECT_EQ(dynlist.size(), 2);
  EXPECT_EQ(dynlist.get_first(), "long string that should not be copied");
  // Note: v still has its elements (we copied, not moved)
  EXPECT_EQ(v[0], "long string that should not be copied");
  EXPECT_EQ(v[1], "another long string for testing");
}

