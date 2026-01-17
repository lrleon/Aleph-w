#include <gtest/gtest.h>
#include <ah-mapping.H>

using namespace Aleph;

class AHMappingTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// =============================================================================
// Basic Operations Tests
// =============================================================================

TEST_F(AHMappingTest, DefaultConstructor)
{
  AHMapping<int, std::string> map;
  
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0u);
}

TEST_F(AHMappingTest, InsertAndAccess)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");
  
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), 3u);
  EXPECT_EQ(map[1], "one");
  EXPECT_EQ(map[2], "two");
  EXPECT_EQ(map[3], "three");
}

TEST_F(AHMappingTest, BracketOperatorInsert)
{
  AHMapping<std::string, int> map;
  
  map["one"] = 1;
  map["two"] = 2;
  map["three"] = 3;
  
  EXPECT_EQ(map.size(), 3u);
  EXPECT_EQ(map["one"], 1);
  EXPECT_EQ(map["two"], 2);
  EXPECT_EQ(map["three"], 3);
}

TEST_F(AHMappingTest, ValidKey)
{
  AHMapping<int, std::string> map;
  
  map.insert(10, "ten");
  
  EXPECT_TRUE(map.valid_key(10));
  EXPECT_FALSE(map.valid_key(20));
  EXPECT_FALSE(map.valid_key(0));
}

TEST_F(AHMappingTest, Remove)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");
  
  EXPECT_TRUE(map.remove(2));
  EXPECT_EQ(map.size(), 2u);
  EXPECT_FALSE(map.valid_key(2));
  EXPECT_TRUE(map.valid_key(1));
  EXPECT_TRUE(map.valid_key(3));
  
  EXPECT_FALSE(map.remove(2));  // Already removed
}

TEST_F(AHMappingTest, Clear)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");
  
  map.clear();
  
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0u);
}

TEST_F(AHMappingTest, UpdateValue)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  EXPECT_EQ(map[1], "one");
  
  // Use bracket operator to update value
  map[1] = "ONE";
  EXPECT_EQ(map[1], "ONE");
  EXPECT_EQ(map.size(), 1u);  // Still just one entry
}

// =============================================================================
// Inverse Mapping Tests
// =============================================================================

TEST_F(AHMappingTest, InverseBasic)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");
  
  auto inv = map.inverse();
  
  EXPECT_EQ(inv.size(), 3u);
  EXPECT_EQ(inv["one"], 1);
  EXPECT_EQ(inv["two"], 2);
  EXPECT_EQ(inv["three"], 3);
}

TEST_F(AHMappingTest, InverseEmpty)
{
  AHMapping<int, std::string> map;
  
  auto inv = map.inverse();
  
  EXPECT_TRUE(inv.empty());
  EXPECT_EQ(inv.size(), 0u);
}

TEST_F(AHMappingTest, DoubleInverse)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  
  auto inv1 = map.inverse();
  auto inv2 = inv1.inverse();
  
  EXPECT_EQ(inv2.size(), 2u);
  EXPECT_EQ(inv2[1], "one");
  EXPECT_EQ(inv2[2], "two");
}

// =============================================================================
// Value Search Tests
// =============================================================================

TEST_F(AHMappingTest, ContainsValue)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");
  
  EXPECT_TRUE(map.contains_value("one"));
  EXPECT_TRUE(map.contains_value("two"));
  EXPECT_TRUE(map.contains_value("three"));
  EXPECT_FALSE(map.contains_value("four"));
  EXPECT_FALSE(map.contains_value(""));
}

// =============================================================================
// Iteration Tests
// =============================================================================

TEST_F(AHMappingTest, ForEach)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");
  
  int count = 0;
  map.for_each([&count](const int& k, const std::string& v) {
    count++;
    EXPECT_GT(k, 0);
    EXPECT_FALSE(v.empty());
  });
  
  EXPECT_EQ(count, 3);
}

// =============================================================================
// Copy and Move Tests
// =============================================================================

TEST_F(AHMappingTest, CopyConstructor)
{
  AHMapping<int, std::string> map1;
  map1.insert(1, "one");
  map1.insert(2, "two");
  
  AHMapping<int, std::string> map2(map1);
  
  EXPECT_EQ(map2.size(), 2u);
  EXPECT_EQ(map2[1], "one");
  EXPECT_EQ(map2[2], "two");
  
  // Verify independence
  map1.insert(3, "three");
  EXPECT_EQ(map1.size(), 3u);
  EXPECT_EQ(map2.size(), 2u);
}

TEST_F(AHMappingTest, MoveConstructor)
{
  AHMapping<int, std::string> map1;
  map1.insert(1, "one");
  map1.insert(2, "two");
  
  AHMapping<int, std::string> map2(std::move(map1));
  
  EXPECT_EQ(map2.size(), 2u);
  EXPECT_EQ(map2[1], "one");
  EXPECT_EQ(map2[2], "two");
}

// =============================================================================
// Different Types Tests
// =============================================================================

TEST_F(AHMappingTest, StringToInt)
{
  AHMapping<std::string, int> map;
  
  map["zero"] = 0;
  map["one"] = 1;
  map["two"] = 2;
  
  EXPECT_EQ(map["zero"], 0);
  EXPECT_EQ(map["one"], 1);
  EXPECT_EQ(map["two"], 2);
}

TEST_F(AHMappingTest, DoubleToString)
{
  AHMapping<double, std::string> map;
  
  map[1.5] = "one-half";
  map[2.5] = "two-half";
  
  EXPECT_EQ(map[1.5], "one-half");
  EXPECT_EQ(map[2.5], "two-half");
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST_F(AHMappingTest, EmptyStringValue)
{
  AHMapping<int, std::string> map;
  
  map.insert(1, "");
  
  EXPECT_TRUE(map.valid_key(1));
  EXPECT_EQ(map[1], "");
}

TEST_F(AHMappingTest, ZeroKey)
{
  AHMapping<int, std::string> map;
  
  map.insert(0, "zero");
  
  EXPECT_TRUE(map.valid_key(0));
  EXPECT_EQ(map[0], "zero");
}

TEST_F(AHMappingTest, NegativeKeys)
{
  AHMapping<int, std::string> map;
  
  map.insert(-1, "minus-one");
  map.insert(-10, "minus-ten");
  
  EXPECT_EQ(map[-1], "minus-one");
  EXPECT_EQ(map[-10], "minus-ten");
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(AHMappingTest, LargeMapping)
{
  AHMapping<int, int> map;
  
  const int N = 1000;
  for (int i = 0; i < N; ++i)
    map.insert(i, i * 2);
  
  EXPECT_EQ(map.size(), static_cast<size_t>(N));
  
  for (int i = 0; i < N; ++i)
  {
    EXPECT_TRUE(map.valid_key(i));
    EXPECT_EQ(map[i], i * 2);
  }
}

TEST_F(AHMappingTest, ManyInsertions)
{
  AHMapping<int, std::string> map;
  
  for (int i = 0; i < 500; ++i)
    map.insert(i, std::to_string(i));
  
  EXPECT_EQ(map.size(), 500u);
  
  for (int i = 0; i < 500; i += 10)
    EXPECT_EQ(map[i], std::to_string(i));
}

TEST_F(AHMappingTest, ManyRemovals)
{
  AHMapping<int, std::string> map;
  
  for (int i = 0; i < 100; ++i)
    map.insert(i, std::to_string(i));
  
  for (int i = 0; i < 100; i += 2)
    EXPECT_TRUE(map.remove(i));
  
  EXPECT_EQ(map.size(), 50u);
  
  for (int i = 1; i < 100; i += 2)
    EXPECT_TRUE(map.valid_key(i));
}

TEST_F(AHMappingTest, LargeInverse)
{
  AHMapping<int, std::string> map;
  
  for (int i = 0; i < 200; ++i)
    map.insert(i, "val_" + std::to_string(i));
  
  auto inv = map.inverse();
  
  EXPECT_EQ(inv.size(), 200u);
  
  for (int i = 0; i < 200; i += 10)
  {
    std::string key = "val_" + std::to_string(i);
    EXPECT_TRUE(inv.valid_key(key));
    EXPECT_EQ(inv[key], i);
  }
}
