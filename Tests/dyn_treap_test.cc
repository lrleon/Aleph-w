#include <gtest/gtest.h>
#include <tpl_dynTreap.H>

using namespace Aleph;

class DynTreapTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DynTreapTest, DefaultConstructor)
{
  DynTreapTree<int, int> treap;
  EXPECT_TRUE(treap.is_empty());
}

TEST_F(DynTreapTest, InsertAndFind)
{
  DynTreapTree<int, std::string> treap;
  
  treap.insert(5, "five");
  treap.insert(3, "three");
  treap.insert(7, "seven");
  
  EXPECT_FALSE(treap.is_empty());
  EXPECT_EQ(treap.size(), 3u);
}

TEST_F(DynTreapTest, BracketOperator)
{
  DynTreapTree<std::string, int> treap;
  
  treap["one"] = 1;
  treap["two"] = 2;
  treap["three"] = 3;
  
  EXPECT_EQ(treap["one"], 1);
  EXPECT_EQ(treap["two"], 2);
  EXPECT_EQ(treap["three"], 3);
}

TEST_F(DynTreapTest, UpdateValue)
{
  DynTreapTree<int, int> treap;
  
  treap[10] = 100;
  EXPECT_EQ(treap[10], 100);
  
  treap[10] = 200;
  EXPECT_EQ(treap[10], 200);
}

TEST_F(DynTreapTest, Has)
{
  DynTreapTree<int, int> treap;
  
  treap.insert(5, 50);
  
  EXPECT_TRUE(treap.has(5));
  EXPECT_FALSE(treap.has(10));
}

TEST_F(DynTreapTest, Remove)
{
  DynTreapTree<int, int> treap;
  
  treap.insert(1, 10);
  treap.insert(2, 20);
  treap.insert(3, 30);
  
  EXPECT_TRUE(treap.remove(2));
  EXPECT_FALSE(treap.has(2));
  EXPECT_TRUE(treap.has(1));
  EXPECT_TRUE(treap.has(3));
}

TEST_F(DynTreapTest, StringKeys)
{
  DynTreapTree<std::string, int> treap;
  
  treap["apple"] = 1;
  treap["banana"] = 2;
  
  EXPECT_EQ(treap["apple"], 1);
  EXPECT_EQ(treap["banana"], 2);
}

TEST_F(DynTreapTest, NegativeKeys)
{
  DynTreapTree<int, int> treap;
  
  treap[-5] = 50;
  treap[-1] = 10;
  treap[0] = 0;
  
  EXPECT_EQ(treap[-5], 50);
  EXPECT_EQ(treap[0], 0);
}

TEST_F(DynTreapTest, ManyInsertions)
{
  DynTreapTree<int, int> treap;
  
  for (int i = 0; i < 100; ++i)
    treap[i] = i * 2;
  
  EXPECT_EQ(treap.size(), 100u);
  
  for (int i = 0; i < 100; i += 10)
    EXPECT_EQ(treap[i], i * 2);
}

TEST_F(DynTreapTest, ManyRemovals)
{
  DynTreapTree<int, int> treap;
  
  for (int i = 0; i < 50; ++i)
    treap.insert(i, i);
  
  EXPECT_EQ(treap.size(), 50u);
  
  for (int i = 0; i < 50; i += 2)
    treap.remove(i);
  
  // After removing evens, only odds remain
  EXPECT_EQ(treap.size(), 25u);
  
  for (int i = 1; i < 50; i += 2)
    EXPECT_TRUE(treap.has(i));
}

TEST_F(DynTreapTest, CopyConstructor)
{
  DynTreapTree<int, std::string> treap1;
  treap1[1] = "one";
  treap1[2] = "two";
  
  DynTreapTree<int, std::string> treap2(treap1);
  
  EXPECT_EQ(treap2.size(), 2u);
  EXPECT_EQ(treap2[1], "one");
  EXPECT_EQ(treap2[2], "two");
}
