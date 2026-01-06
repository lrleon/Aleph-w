/* Aleph-w

   Comprehensive tests for tpl_dynMapOhash.H

   Tests MapOpenHash, MapODhash, and MapOLhash implementations.
*/


/**
 * @file dynmapohash_test.cc
 * @brief Tests for Dynmapohash
 */
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <tpl_dynMapOhash.H>

using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

class MapODhashTest : public ::testing::Test
{
protected:
  MapODhash<std::string, int> map;
  MapODhash<int, std::string> int_map;

  void SetUp() override
  {
    // Start with empty maps
  }

  void populate_map()
  {
    map.insert("one", 1);
    map.insert("two", 2);
    map.insert("three", 3);
    map.insert("four", 4);
    map.insert("five", 5);
  }

  void populate_int_map()
  {
    int_map.insert(1, "one");
    int_map.insert(2, "two");
    int_map.insert(3, "three");
  }
};

class MapOLhashTest : public ::testing::Test
{
protected:
  MapOLhash<std::string, int> map;

  void populate_map()
  {
    map.insert("alpha", 1);
    map.insert("beta", 2);
    map.insert("gamma", 3);
  }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(MapODhashTest, DefaultConstruction)
{
  EXPECT_EQ(map.size(), 0u);
  EXPECT_TRUE(map.is_empty());
}

TEST_F(MapOLhashTest, DefaultConstruction)
{
  EXPECT_EQ(map.size(), 0u);
  EXPECT_TRUE(map.is_empty());
}

TEST(MapOpenHashConstruction, WithCustomSize)
{
  MapODhash<int, int> map(101);
  EXPECT_EQ(map.size(), 0u);
}

TEST(MapOpenHashConstruction, CopyConstruction)
{
  MapODhash<std::string, int> original;
  original.insert("a", 1);
  original.insert("b", 2);

  MapODhash<std::string, int> copy(original);
  EXPECT_EQ(copy.size(), original.size());
  EXPECT_TRUE(copy.has("a"));
  EXPECT_TRUE(copy.has("b"));
  EXPECT_EQ(copy["a"], 1);
  EXPECT_EQ(copy["b"], 2);
}

TEST(MapOpenHashConstruction, MoveConstruction)
{
  MapODhash<std::string, int> original;
  original.insert("x", 10);
  original.insert("y", 20);
  size_t orig_size = original.size();

  MapODhash<std::string, int> moved(std::move(original));
  EXPECT_EQ(moved.size(), orig_size);
  EXPECT_TRUE(moved.has("x"));
  EXPECT_TRUE(moved.has("y"));
}

// =============================================================================
// Insert Tests
// =============================================================================

TEST_F(MapODhashTest, InsertCopySemanticsKeyValue)
{
  std::string key = "test";
  int value = 42;

  auto * pair = map.insert(key, value);
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->first, "test");
  EXPECT_EQ(pair->second, 42);
  EXPECT_EQ(map.size(), 1u);
}

TEST_F(MapODhashTest, InsertMoveValue)
{
  std::string key = "moved";
  std::string value = "hello_world";

  auto * pair = int_map.insert(100, std::move(value));
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->first, 100);
  EXPECT_EQ(pair->second, "hello_world");
}

TEST_F(MapODhashTest, InsertMoveKeyAndValue)
{
  std::string key = "key";
  int value = 99;

  auto * pair = map.insert(std::move(key), std::move(value));
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->first, "key");
  EXPECT_EQ(pair->second, 99);
}

TEST_F(MapODhashTest, InsertMoveKeyCopyValue)
{
  std::string key = "movekey";
  int value = 77;

  auto * pair = map.insert(std::move(key), value);
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->first, "movekey");
  EXPECT_EQ(pair->second, 77);
}

TEST_F(MapODhashTest, InsertDuplicateKeyReturnsNullptr)
{
  map.insert("dup", 1);
  auto * pair = map.insert("dup", 2);
  EXPECT_EQ(pair, nullptr);
  EXPECT_EQ(map["dup"], 1); // Original value preserved
}

TEST_F(MapODhashTest, InsertMultipleEntries)
{
  populate_map();
  EXPECT_EQ(map.size(), 5u);
  EXPECT_TRUE(map.has("one"));
  EXPECT_TRUE(map.has("five"));
}

// =============================================================================
// Search Tests
// =============================================================================

TEST_F(MapODhashTest, SearchExistingKey)
{
  populate_map();

  auto * pair = map.search("three");
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->first, "three");
  EXPECT_EQ(pair->second, 3);
}

TEST_F(MapODhashTest, SearchNonexistentKey)
{
  populate_map();

  auto * pair = map.search("nonexistent");
  EXPECT_EQ(pair, nullptr);
}

TEST_F(MapODhashTest, SearchWithMoveSemantics)
{
  populate_map();

  std::string key = "two";
  auto * pair = map.search(std::move(key));
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->second, 2);
}

TEST_F(MapODhashTest, SearchEmptyMap)
{
  auto * pair = map.search("anything");
  EXPECT_EQ(pair, nullptr);
}

// =============================================================================
// Has/Contains Tests
// =============================================================================

TEST_F(MapODhashTest, HasExistingKey)
{
  populate_map();
  EXPECT_TRUE(map.has("one"));
  EXPECT_TRUE(map.has("five"));
}

TEST_F(MapODhashTest, HasNonexistentKey)
{
  populate_map();
  EXPECT_FALSE(map.has("nonexistent"));
  EXPECT_FALSE(map.has(""));
}

TEST_F(MapODhashTest, HasWithMoveSemantics)
{
  populate_map();
  std::string key = "three";
  EXPECT_TRUE(map.has(std::move(key)));
}

TEST_F(MapODhashTest, ContainsIsAliasForHas)
{
  populate_map();
  EXPECT_EQ(map.contains("one"), map.has("one"));
  EXPECT_EQ(map.contains("nonexistent"), map.has("nonexistent"));
}

TEST_F(MapODhashTest, ContainsWithMoveSemantics)
{
  populate_map();
  std::string key = "two";
  EXPECT_TRUE(map.contains(std::move(key)));
}

// =============================================================================
// Find Tests
// =============================================================================

TEST_F(MapODhashTest, FindExistingKey)
{
  populate_map();

  int & value = map.find("three");
  EXPECT_EQ(value, 3);
}

TEST_F(MapODhashTest, FindAndModify)
{
  populate_map();

  map.find("two") = 22;
  EXPECT_EQ(map.find("two"), 22);
}

TEST_F(MapODhashTest, FindNonexistentKeyThrows)
{
  populate_map();
  EXPECT_THROW((void)map.find("nonexistent"), std::domain_error);
}

TEST_F(MapODhashTest, FindWithMoveSemantics)
{
  populate_map();
  std::string key = "four";
  EXPECT_EQ(map.find(std::move(key)), 4);
}

TEST_F(MapODhashTest, FindConstVersion)
{
  populate_map();
  const auto & const_map = map;

  const int & value = const_map.find("one");
  EXPECT_EQ(value, 1);
}

TEST_F(MapODhashTest, FindConstNonexistentThrows)
{
  populate_map();
  const auto & const_map = map;

  EXPECT_THROW((void)const_map.find("missing"), std::domain_error);
}

// =============================================================================
// Operator[] Tests
// =============================================================================

TEST_F(MapODhashTest, SubscriptAccessExisting)
{
  populate_map();
  EXPECT_EQ(map["one"], 1);
  EXPECT_EQ(map["five"], 5);
}

TEST_F(MapODhashTest, SubscriptInsertsNewKey)
{
  map["new_key"] = 100;
  EXPECT_TRUE(map.has("new_key"));
  EXPECT_EQ(map["new_key"], 100);
}

TEST_F(MapODhashTest, SubscriptDefaultInitializes)
{
  int & value = map["defaulted"];
  EXPECT_EQ(value, 0); // Default-initialized int
  value = 42;
  EXPECT_EQ(map["defaulted"], 42);
}

TEST_F(MapODhashTest, SubscriptWithMoveSemantics)
{
  std::string key = "moved_key";
  map[std::move(key)] = 55;
  EXPECT_EQ(map["moved_key"], 55);
}

TEST_F(MapODhashTest, SubscriptConstVersion)
{
  populate_map();
  const auto & const_map = map;

  EXPECT_EQ(const_map["two"], 2);
}

TEST_F(MapODhashTest, SubscriptConstNonexistentThrows)
{
  populate_map();
  const auto & const_map = map;

  EXPECT_THROW((void)const_map["missing"], std::domain_error);
}

// =============================================================================
// Remove Tests
// =============================================================================

TEST_F(MapODhashTest, RemoveExistingKey)
{
  populate_map();
  size_t original_size = map.size();

  map.remove("three");

  EXPECT_EQ(map.size(), original_size - 1);
  EXPECT_FALSE(map.has("three"));
}

TEST_F(MapODhashTest, RemoveNonexistentKeyThrows)
{
  populate_map();
  EXPECT_THROW(map.remove("nonexistent"), std::domain_error);
}

TEST_F(MapODhashTest, RemoveWithMoveSemantics)
{
  populate_map();
  std::string key = "two";

  map.remove(std::move(key));
  EXPECT_FALSE(map.has("two"));
}

TEST_F(MapODhashTest, RemoveAllEntries)
{
  populate_map();

  map.remove("one");
  map.remove("two");
  map.remove("three");
  map.remove("four");
  map.remove("five");

  EXPECT_EQ(map.size(), 0u);
  EXPECT_TRUE(map.is_empty());
}

TEST_F(MapODhashTest, RemoveByData)
{
  populate_map();
  auto * pair = map.search("three");
  ASSERT_NE(pair, nullptr);

  map.remove_by_data(pair->second);
  EXPECT_FALSE(map.has("three"));
}

// =============================================================================
// Keys and Values Tests
// =============================================================================

TEST_F(MapODhashTest, KeysReturnsAllKeys)
{
  populate_map();

  auto key_list = map.keys();
  EXPECT_EQ(key_list.size(), 5u);

  // Check all expected keys are present
  bool has_one = false, has_two = false, has_three = false;
  bool has_four = false, has_five = false;

  key_list.traverse([&](const std::string & k) {
    if (k == "one") has_one = true;
    else if (k == "two") has_two = true;
    else if (k == "three") has_three = true;
    else if (k == "four") has_four = true;
    else if (k == "five") has_five = true;
    return true;
  });

  EXPECT_TRUE(has_one);
  EXPECT_TRUE(has_two);
  EXPECT_TRUE(has_three);
  EXPECT_TRUE(has_four);
  EXPECT_TRUE(has_five);
}

TEST_F(MapODhashTest, ValuesReturnsAllValues)
{
  populate_map();

  auto value_list = map.values();
  EXPECT_EQ(value_list.size(), 5u);

  // Sum should be 1+2+3+4+5 = 15
  int sum = 0;
  value_list.traverse([&sum](int v) {
    sum += v;
    return true;
  });
  EXPECT_EQ(sum, 15);
}

TEST_F(MapODhashTest, ValuesPtrReturnsPointers)
{
  populate_map();

  auto ptr_list = map.values_ptr();
  EXPECT_EQ(ptr_list.size(), 5u);

  // Modify through pointers
  ptr_list.traverse([](int * p) {
    *p *= 10;
    return true;
  });

  // Verify modifications
  EXPECT_EQ(map["one"], 10);
  EXPECT_EQ(map["two"], 20);
}

TEST_F(MapODhashTest, ItemsPtrReturnsPairPointers)
{
  populate_map();

  auto ptr_list = map.items_ptr();
  EXPECT_EQ(ptr_list.size(), 5u);

  // Verify we can access both key and value
  ptr_list.traverse([](auto * pair) {
    EXPECT_FALSE(pair->first.empty());
    return true;
  });
}

TEST_F(MapODhashTest, KeysOnEmptyMap)
{
  auto key_list = map.keys();
  EXPECT_EQ(key_list.size(), 0u);
}

TEST_F(MapODhashTest, ValuesOnEmptyMap)
{
  auto value_list = map.values();
  EXPECT_EQ(value_list.size(), 0u);
}

// =============================================================================
// Static Helper Function Tests
// =============================================================================

TEST_F(MapODhashTest, KeyToPair)
{
  populate_map();
  auto * pair = map.search("one");
  ASSERT_NE(pair, nullptr);

  auto * recovered_pair = MapODhash<std::string, int>::key_to_pair(&pair->first);
  EXPECT_EQ(recovered_pair, pair);
}

TEST_F(MapODhashTest, DataToPair)
{
  populate_map();
  auto * pair = map.search("two");
  ASSERT_NE(pair, nullptr);

  auto * recovered_pair = MapODhash<std::string, int>::data_to_pair(&pair->second);
  EXPECT_EQ(recovered_pair, pair);
  EXPECT_EQ(recovered_pair->first, "two");
}

TEST_F(MapODhashTest, GetDataFromKey)
{
  populate_map();
  auto * pair = map.search("three");
  ASSERT_NE(pair, nullptr);

  int & data = MapODhash<std::string, int>::get_data(pair->first);
  EXPECT_EQ(data, 3);

  // Modify through this reference
  data = 333;
  EXPECT_EQ(map["three"], 333);
}

TEST_F(MapODhashTest, GetKeyFromDataPtr)
{
  populate_map();
  auto * pair = map.search("four");
  ASSERT_NE(pair, nullptr);

  const std::string & key = MapODhash<std::string, int>::get_key(&pair->second);
  EXPECT_EQ(key, "four");
}

TEST_F(MapODhashTest, KeyToPairConstOverload)
{
  populate_map();
  const auto & const_map = map;
  const auto * pair = const_map.search("one");
  ASSERT_NE(pair, nullptr);

  const auto * recovered_pair = MapODhash<std::string, int>::key_to_pair(&pair->first);
  EXPECT_EQ(recovered_pair, pair);
}

TEST_F(MapODhashTest, DataToPairConstOverload)
{
  populate_map();
  const auto & const_map = map;
  const auto * pair = const_map.search("two");
  ASSERT_NE(pair, nullptr);

  const auto * recovered_pair = MapODhash<std::string, int>::data_to_pair(&pair->second);
  EXPECT_EQ(recovered_pair, pair);
}

TEST_F(MapODhashTest, GetDataConstOverload)
{
  populate_map();
  const auto & const_map = map;
  const auto * pair = const_map.search("three");
  ASSERT_NE(pair, nullptr);

  const int & data = MapODhash<std::string, int>::get_data(pair->first);
  EXPECT_EQ(data, 3);
}

TEST_F(MapODhashTest, GetKeyConstOverload)
{
  populate_map();
  const auto & const_map = map;
  const auto * pair = const_map.search("four");
  ASSERT_NE(pair, nullptr);

  const std::string & key = MapODhash<std::string, int>::get_key(&pair->second);
  EXPECT_EQ(key, "four");
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST_F(MapODhashTest, IteratorTraversal)
{
  populate_map();

  size_t count = 0;
  for (MapODhash<std::string, int>::Iterator it(map); it.has_curr(); it.next_ne())
    {
      auto & pair = it.get_curr();
      EXPECT_FALSE(pair.first.empty());
      ++count;
    }

  EXPECT_EQ(count, 5u);
}

TEST_F(MapODhashTest, IteratorOnEmptyMap)
{
  MapODhash<std::string, int>::Iterator it(map);
  EXPECT_FALSE(it.has_curr());
}

TEST_F(MapODhashTest, TraverseFunctional)
{
  populate_map();

  int sum = 0;
  map.traverse([&sum](const auto & pair) {
    sum += pair.second;
    return true;
  });

  EXPECT_EQ(sum, 15);
}

TEST_F(MapODhashTest, TraverseEarlyTermination)
{
  populate_map();

  int count = 0;
  bool result = map.traverse([&count](const auto &) {
    ++count;
    return count < 3; // Stop after 3 elements
  });

  EXPECT_FALSE(result); // Stopped early
  EXPECT_EQ(count, 3);
}

// =============================================================================
// MapOLhash Specific Tests
// =============================================================================

TEST_F(MapOLhashTest, BasicOperations)
{
  populate_map();

  EXPECT_TRUE(map.has("alpha"));
  EXPECT_TRUE(map.has("beta"));
  EXPECT_TRUE(map.has("gamma"));
  EXPECT_EQ(map.size(), 3u);
}

TEST_F(MapOLhashTest, InsertAndSearch)
{
  map.insert("test", 42);
  auto * pair = map.search("test");
  ASSERT_NE(pair, nullptr);
  EXPECT_EQ(pair->second, 42);
}

TEST_F(MapOLhashTest, RemoveOperation)
{
  populate_map();
  map.remove("beta");

  EXPECT_FALSE(map.has("beta"));
  EXPECT_TRUE(map.has("alpha"));
  EXPECT_TRUE(map.has("gamma"));
}

// =============================================================================
// Large Scale Tests
// =============================================================================

TEST(MapOpenHashScaling, InsertManyElements)
{
  MapODhash<int, int> map;

  constexpr int N = 10000;
  for (int i = 0; i < N; ++i)
    map.insert(i, i * 2);

  EXPECT_EQ(map.size(), static_cast<size_t>(N));

  // Verify all elements
  for (int i = 0; i < N; ++i)
    {
      ASSERT_TRUE(map.has(i)) << "Missing key: " << i;
      EXPECT_EQ(map[i], i * 2);
    }
}

TEST(MapOpenHashScaling, RemoveManyElements)
{
  MapODhash<int, int> map;

  constexpr int N = 1000;
  for (int i = 0; i < N; ++i)
    map.insert(i, i);

  // Remove even numbers
  for (int i = 0; i < N; i += 2)
    map.remove(i);

  EXPECT_EQ(map.size(), static_cast<size_t>(N / 2));

  // Verify only odd numbers remain
  for (int i = 0; i < N; ++i)
    {
      if (i % 2 == 0)
        EXPECT_FALSE(map.has(i));
      else
        EXPECT_TRUE(map.has(i));
    }
}

TEST(MapOpenHashScaling, InsertRemoveInterleaved)
{
  MapODhash<int, int> map;

  for (int i = 0; i < 500; ++i)
    {
      map.insert(i, i);
      if (i > 100)
        map.remove(i - 100);
    }

  // Remaining keys are 0 plus the last 100 inserted keys (400-499).
  EXPECT_EQ(map.size(), 101u);
  EXPECT_TRUE(map.has(0));
  for (int i = 400; i < 500; ++i)
    EXPECT_TRUE(map.has(i));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(MapODhashTest, EmptyStringKey)
{
  map.insert("", 0);
  EXPECT_TRUE(map.has(""));
  EXPECT_EQ(map[""], 0);
}

TEST_F(MapODhashTest, NegativeValues)
{
  map.insert("neg", -100);
  EXPECT_EQ(map["neg"], -100);
}

TEST(MapOpenHashEdgeCases, ZeroKey)
{
  MapODhash<int, std::string> map;
  map.insert(0, "zero");
  EXPECT_TRUE(map.has(0));
  EXPECT_EQ(map[0], "zero");
}

TEST(MapOpenHashEdgeCases, NegativeKey)
{
  MapODhash<int, int> map;
  map.insert(-5, 50);
  map.insert(-100, 1000);

  EXPECT_TRUE(map.has(-5));
  EXPECT_TRUE(map.has(-100));
  EXPECT_EQ(map[-5], 50);
  EXPECT_EQ(map[-100], 1000);
}

// =============================================================================
// Assignment Operators
// =============================================================================

TEST(MapOpenHashAssignment, CopyAssignment)
{
  MapODhash<std::string, int> original;
  original.insert("a", 1);
  original.insert("b", 2);

  MapODhash<std::string, int> copy;
  copy.insert("x", 100); // Some existing content

  copy = original;

  EXPECT_EQ(copy.size(), 2u);
  EXPECT_TRUE(copy.has("a"));
  EXPECT_TRUE(copy.has("b"));
  EXPECT_FALSE(copy.has("x")); // Old content gone
}

TEST(MapOpenHashAssignment, MoveAssignment)
{
  MapODhash<std::string, int> original;
  original.insert("m", 10);
  original.insert("n", 20);

  MapODhash<std::string, int> target;
  target = std::move(original);

  EXPECT_EQ(target.size(), 2u);
  EXPECT_TRUE(target.has("m"));
  EXPECT_TRUE(target.has("n"));
}

TEST(MapOpenHashAssignment, SelfAssignment)
{
  MapODhash<std::string, int> map;
  map.insert("self", 42);

  map = map; // Self-assignment

  EXPECT_EQ(map.size(), 1u);
  EXPECT_EQ(map["self"], 42);
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST_F(MapODhashTest, TypeAliases)
{
  // Verify type aliases exist and are correct
  static_assert(std::is_same_v<MapODhash<std::string, int>::Key_Type, std::string>);
  static_assert(std::is_same_v<MapODhash<std::string, int>::Data_Type, int>);
  static_assert(std::is_same_v<MapODhash<std::string, int>::Value_Type, int>);
  static_assert(std::is_same_v<MapODhash<std::string, int>::Item_Type,
                               std::pair<std::string, int>>);
}

// =============================================================================
// Comparison with Linear Probing
// =============================================================================

TEST(MapVariantComparison, BothVariantsProduceSameResults)
{
  MapODhash<int, std::string> od_map;
  MapOLhash<int, std::string> ol_map;

  // Insert same data
  for (int i = 0; i < 100; ++i)
    {
      std::string val = std::to_string(i * 10);
      od_map.insert(i, val);
      ol_map.insert(i, val);
    }

  EXPECT_EQ(od_map.size(), ol_map.size());

  // Verify same content
  for (int i = 0; i < 100; ++i)
    {
      EXPECT_TRUE(od_map.has(i));
      EXPECT_TRUE(ol_map.has(i));
      EXPECT_EQ(od_map[i], ol_map[i]);
    }
}

// =============================================================================
// Functional Methods (inherited)
// =============================================================================

TEST_F(MapODhashTest, FilterMethod)
{
  populate_map();

  auto filtered = map.filter([](const auto & pair) {
    return pair.second > 2;
  });

  EXPECT_EQ(filtered.size(), 3u); // three, four, five
}

TEST_F(MapODhashTest, MapsMethod)
{
  populate_map();

  auto doubled = map.template maps<int>([](const auto & pair) {
    return pair.second * 2;
  });

  int sum = 0;
  doubled.traverse([&sum](int v) {
    sum += v;
    return true;
  });

  EXPECT_EQ(sum, 30); // (1+2+3+4+5) * 2 = 30
}

TEST_F(MapODhashTest, FoldlMethod)
{
  populate_map();

  int sum = map.template foldl<int>(0, [](int acc, const auto & pair) {
    return acc + pair.second;
  });

  EXPECT_EQ(sum, 15);
}

TEST_F(MapODhashTest, AllMethod)
{
  populate_map();

  bool all_positive = map.all([](const auto & pair) {
    return pair.second > 0;
  });

  EXPECT_TRUE(all_positive);

  map["zero"] = 0;

  all_positive = map.all([](const auto & pair) {
    return pair.second > 0;
  });

  EXPECT_FALSE(all_positive);
}

TEST_F(MapODhashTest, ExistsMethod)
{
  populate_map();

  bool has_three = map.exists([](const auto & pair) {
    return pair.second == 3;
  });

  EXPECT_TRUE(has_three);

  bool has_ten = map.exists([](const auto & pair) {
    return pair.second == 10;
  });

  EXPECT_FALSE(has_ten);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}