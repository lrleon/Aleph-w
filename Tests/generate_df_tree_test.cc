#include <gtest/gtest.h>
#include <tpl_tree_node.H>

// Test basic structures from generate_df_tree.H without Grafo dependency
struct Clave
{
  int key;
  long count;
  long low;
};

struct Clave_Igual
{
  bool operator () (const Clave & c1, const Clave & c2) const
  {
    return c1.key == c2.key;
  }
};

class GenerateDfTreeTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(GenerateDfTreeTest, ClaveStructure)
{
  Clave c;
  c.key = 65;
  c.count = 1;
  c.low = 0;
  
  EXPECT_EQ(c.key, 65);
  EXPECT_EQ(c.count, 1);
  EXPECT_EQ(c.low, 0);
}

TEST_F(GenerateDfTreeTest, ClaveIgual)
{
  Clave c1 = {65, 1, 0};
  Clave c2 = {65, 2, 1};
  Clave c3 = {66, 1, 0};
  
  Clave_Igual cmp;
  
  EXPECT_TRUE(cmp(c1, c2));
  EXPECT_FALSE(cmp(c1, c3));
}

TEST_F(GenerateDfTreeTest, ClaveIgualReflexive)
{
  Clave c = {42, 10, 5};
  
  Clave_Igual cmp;
  EXPECT_TRUE(cmp(c, c));
}

TEST_F(GenerateDfTreeTest, ClaveIgualSymmetric)
{
  Clave c1 = {100, 1, 0};
  Clave c2 = {100, 2, 1};
  
  Clave_Igual cmp;
  EXPECT_TRUE(cmp(c1, c2));
  EXPECT_TRUE(cmp(c2, c1));
}

TEST_F(GenerateDfTreeTest, TreeNodeWithClave)
{
  Tree_Node<Clave> node;
  Clave& k = node.get_key();
  k.key = 'A';
  k.count = 0;
  k.low = 0;
  
  EXPECT_EQ(k.key, 'A');
  EXPECT_EQ(k.count, 0);
}

TEST_F(GenerateDfTreeTest, MultipleClave)
{
  Clave_Igual cmp;
  
  for (int i = 0; i < 10; ++i)
  {
    for (int j = 0; j < 10; ++j)
    {
      Clave c1 = {i, 0, 0};
      Clave c2 = {j, 0, 0};
      
      if (i == j)
        EXPECT_TRUE(cmp(c1, c2));
      else
        EXPECT_FALSE(cmp(c1, c2));
    }
  }
}

TEST_F(GenerateDfTreeTest, ZeroValues)
{
  Clave c = {0, 0, 0};
  
  EXPECT_EQ(c.key, 0);
  EXPECT_EQ(c.count, 0);
  EXPECT_EQ(c.low, 0);
}

TEST_F(GenerateDfTreeTest, NegativeLow)
{
  Clave c = {10, 5, -1};
  
  EXPECT_EQ(c.low, -1);
}

TEST_F(GenerateDfTreeTest, LargeValues)
{
  Clave c = {1000, 1000000, 500000};
  
  EXPECT_EQ(c.key, 1000);
  EXPECT_EQ(c.count, 1000000);
  EXPECT_EQ(c.low, 500000);
}

TEST_F(GenerateDfTreeTest, CompareManyClave)
{
  Clave_Igual cmp;
  std::vector<Clave> claves;
  
  for (int i = 0; i < 50; ++i)
    claves.push_back({i, i * 10, i * 5});
  
  for (size_t i = 0; i < claves.size(); ++i)
  {
    EXPECT_TRUE(cmp(claves[i], claves[i]));
    
    if (i + 1 < claves.size())
      EXPECT_FALSE(cmp(claves[i], claves[i + 1]));
  }
}
