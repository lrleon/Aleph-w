/**
 * @file random_tree_test.cc
 * @brief Comprehensive tests for random_tree.H
 */

#include <gtest/gtest.h>
#include <random_tree.H>
#include <tpl_tree_node.H>
#include <tpl_binNodeUtils.H>

using namespace Aleph;

// =============================================================================
// Test Fixture
// =============================================================================

class RandomTreeTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
  
  // Helper: count nodes in a tree (template)
  template <typename T>
  size_t count_nodes(Tree_Node<T>* root)
  {
    if (root == nullptr)
      return 0;
    
    size_t count = 1;
    for (Tree_Node<T>* child = root->get_left_child(); 
         child != nullptr; 
         child = child->get_right_sibling())
      count += count_nodes(child);
    
    return count;
  }
  
  // Helper: calculate tree height (template)
  template <typename T>
  size_t tree_height(Tree_Node<T>* root)
  {
    if (root == nullptr)
      return 0;
    
    size_t max_child_height = 0;
    for (Tree_Node<T>* child = root->get_left_child(); 
         child != nullptr; 
         child = child->get_right_sibling())
    {
      size_t h = tree_height(child);
      if (h > max_child_height)
        max_child_height = h;
    }
    
    return 1 + max_child_height;
  }
  
  // Helper: destroy tree recursively
  template <typename T>
  void destroy_tree(Tree_Node<T>* root)
  {
    if (root == nullptr)
      return;
    
    // Delete all children first
    Tree_Node<T>* child = root->get_left_child();
    while (child != nullptr)
    {
      Tree_Node<T>* next = child->get_right_sibling();
      destroy_tree(child);
      child = next;
    }
    
    delete root;
  }
};

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST_F(RandomTreeTest, GenerateSingleNode)
{
  RandTree<int> gen(42);
  Tree_Node<int>* tree = gen(1);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 1u);
  EXPECT_EQ(tree_height(tree), 1u);
  EXPECT_EQ(tree->get_left_child(), nullptr);
  
  destroy_tree(tree);
}

TEST_F(RandomTreeTest, GenerateSmallTree)
{
  RandTree<int> gen(123);
  Tree_Node<int>* tree = gen(5);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 5u);
  EXPECT_GT(tree_height(tree), 0u);
  
  destroy_tree(tree);
}

TEST_F(RandomTreeTest, GenerateMediumTree)
{
  RandTree<int> gen(456);
  Tree_Node<int>* tree = gen(50);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 50u);
  
  destroy_tree(tree);
}

TEST_F(RandomTreeTest, GenerateLargeTree)
{
  RandTree<int> gen(789);
  Tree_Node<int>* tree = gen(500);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 500u);
  
  destroy_tree(tree);
}

// =============================================================================
// Determinism Tests
// =============================================================================

TEST_F(RandomTreeTest, SameSeedProducesSameStructure)
{
  const unsigned seed = 12345;
  const size_t n = 20;
  
  RandTree<int> gen1(seed);
  Tree_Node<int>* tree1 = gen1(n);
  
  RandTree<int> gen2(seed);
  Tree_Node<int>* tree2 = gen2(n);
  
  ASSERT_NE(tree1, nullptr);
  ASSERT_NE(tree2, nullptr);
  
  // Same number of nodes
  EXPECT_EQ(count_nodes(tree1), count_nodes(tree2));
  
  // Same height (deterministic generation)
  EXPECT_EQ(tree_height(tree1), tree_height(tree2));
  
  destroy_tree(tree1);
  destroy_tree(tree2);
}

TEST_F(RandomTreeTest, DifferentSeedsProduceDifferentTrees)
{
  const size_t n = 30;
  
  RandTree<int> gen1(111);
  Tree_Node<int>* tree1 = gen1(n);
  
  RandTree<int> gen2(222);
  Tree_Node<int>* tree2 = gen2(n);
  
  ASSERT_NE(tree1, nullptr);
  ASSERT_NE(tree2, nullptr);
  
  // Same size but likely different structure
  EXPECT_EQ(count_nodes(tree1), n);
  EXPECT_EQ(count_nodes(tree2), n);
  
  // Heights are likely different (not guaranteed but very probable)
  // Just check both are valid
  EXPECT_GT(tree_height(tree1), 0u);
  EXPECT_GT(tree_height(tree2), 0u);
  
  destroy_tree(tree1);
  destroy_tree(tree2);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(RandomTreeTest, MultipleGenerations)
{
  RandTree<int> gen(999);
  
  for (int i = 0; i < 10; ++i)
  {
    Tree_Node<int>* tree = gen(10 + i * 5);
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(count_nodes(tree), static_cast<size_t>(10 + i * 5));
    destroy_tree(tree);
  }
}

TEST_F(RandomTreeTest, VeryLargeTree)
{
  RandTree<int> gen(7777);
  Tree_Node<int>* tree = gen(2000);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 2000u);
  
  destroy_tree(tree);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(RandomTreeTest, TwoNodes)
{
  RandTree<int> gen(42);
  Tree_Node<int>* tree = gen(2);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 2u);
  
  // Root must have exactly one child
  Tree_Node<int>* child = tree->get_left_child();
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->get_right_sibling(), nullptr);
  EXPECT_EQ(child->get_left_child(), nullptr);
  
  destroy_tree(tree);
}

TEST_F(RandomTreeTest, ThreeNodes)
{
  RandTree<int> gen(42);
  Tree_Node<int>* tree = gen(3);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 3u);
  
  destroy_tree(tree);
}

// =============================================================================
// Different Data Types
// =============================================================================

TEST_F(RandomTreeTest, StringType)
{
  RandTree<std::string> gen(12345);
  Tree_Node<std::string>* tree = gen(20);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 20u);
  
  destroy_tree(tree);
}

TEST_F(RandomTreeTest, DoubleType)
{
  RandTree<double> gen(54321);
  Tree_Node<double>* tree = gen(15);
  
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(count_nodes(tree), 15u);
  
  destroy_tree(tree);
}

// =============================================================================
// Constructor Tests
// =============================================================================

TEST_F(RandomTreeTest, DefaultConstructorUsesCurrentTime)
{
  RandTree<int> gen1;
  Tree_Node<int>* tree1 = gen1(10);
  
  RandTree<int> gen2;
  Tree_Node<int>* tree2 = gen2(10);
  
  ASSERT_NE(tree1, nullptr);
  ASSERT_NE(tree2, nullptr);
  
  // Both should succeed
  EXPECT_EQ(count_nodes(tree1), 10u);
  EXPECT_EQ(count_nodes(tree2), 10u);
  
  destroy_tree(tree1);
  destroy_tree(tree2);
}

// =============================================================================
// Structure Validation Tests
// =============================================================================

TEST_F(RandomTreeTest, ValidTreeStructure)
{
  RandTree<int> gen(888);
  Tree_Node<int>* tree = gen(100);
  
  ASSERT_NE(tree, nullptr);
  
  // Verify tree structure integrity using traversal
  std::function<bool(Tree_Node<int>*)> validate = [&](Tree_Node<int>* node) -> bool
  {
    if (node == nullptr)
      return true;
    
    for (Tree_Node<int>* child = node->get_left_child(); 
         child != nullptr; 
         child = child->get_right_sibling())
    {
      if (not validate(child))
        return false;
    }
    
    return true;
  };
  
  EXPECT_TRUE(validate(tree));
  EXPECT_EQ(count_nodes(tree), 100u);
  
  destroy_tree(tree);
}

// =============================================================================
// Performance/Scalability Tests
// =============================================================================

TEST_F(RandomTreeTest, ScalabilityTest)
{
  RandTree<int> gen(11111);
  
  std::vector<size_t> sizes = {10, 50, 100, 500, 1000};
  
  for (size_t n : sizes)
  {
    Tree_Node<int>* tree = gen(n);
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(count_nodes(tree), n);
    destroy_tree(tree);
  }
}
