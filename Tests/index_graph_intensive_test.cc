#include <gtest/gtest.h>
#include <tpl_indexGraph.H>
#include <tpl_graph.H>

using namespace Aleph;

class IndexGraphIntensiveTest : public ::testing::Test
{
protected:
  using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  GT g;
  
  void SetUp() override {}
  void TearDown() override {}
};

// =============================================================================
// Large Scale Tests
// =============================================================================

TEST_F(IndexGraphIntensiveTest, LargeNumberOfNodes)
{
  Index_Graph<GT> idx(g);
  
  const int N = 1000;
  for (int i = 0; i < N; ++i)
    idx.insert_node(i);
  
  EXPECT_EQ(idx.get_num_nodes(), static_cast<size_t>(N));
  
  // Verify all are searchable
  for (int i = 0; i < N; i += 10)
  {
    auto n = idx.search_node(i);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->get_info(), i);
  }
}

TEST_F(IndexGraphIntensiveTest, LargeNumberOfArcs)
{
  Index_Graph<GT> idx(g);
  
  std::vector<GT::Node*> nodes;
  for (int i = 0; i < 50; ++i)
    nodes.push_back(idx.insert_node(i));
  
  // Create many arcs
  int arc_count = 0;
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    for (size_t j = i + 1; j < nodes.size(); ++j)
    {
      idx.insert_arc(nodes[i], nodes[j], arc_count++);
    }
  }
  
  EXPECT_GT(idx.get_num_arcs(), 1000u);
}

TEST_F(IndexGraphIntensiveTest, DenseGraph)
{
  Index_Graph<GT> idx(g);
  
  const int N = 30;
  std::vector<GT::Node*> nodes;
  for (int i = 0; i < N; ++i)
    nodes.push_back(idx.insert_node(i));
  
  // Create many arcs (but not complete to avoid duplicates in undirected graph)
  int arc_count = 0;
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
    {
      idx.insert_arc(nodes[i], nodes[j], i * 1000 + j);
      arc_count++;
    }
  
  EXPECT_EQ(idx.get_num_nodes(), static_cast<size_t>(N));
  EXPECT_EQ(idx.get_num_arcs(), static_cast<size_t>(arc_count));
}

TEST_F(IndexGraphIntensiveTest, ManySearchOperations)
{
  Index_Graph<GT> idx(g);
  
  for (int i = 0; i < 100; ++i)
    idx.insert_node(i * 10);
  
  // Perform many searches
  for (int iter = 0; iter < 1000; ++iter)
  {
    int search_val = (iter % 100) * 10;
    auto n = idx.search_node(search_val);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->get_info(), search_val);
  }
}

// =============================================================================
// Insertion/Removal Patterns
// =============================================================================

TEST_F(IndexGraphIntensiveTest, AlternatingInsertRemove)
{
  Index_Graph<GT> idx(g);
  
  for (int i = 0; i < 100; ++i)
  {
    idx.insert_node(i);
    if (i % 5 == 4)
    {
      auto n = idx.search_node(i - 2);
      if (n)
        idx.remove_node(n);
    }
  }
  
  EXPECT_GT(idx.get_num_nodes(), 0u);
  EXPECT_LT(idx.get_num_nodes(), 100u);
}

TEST_F(IndexGraphIntensiveTest, BulkInsertThenRemove)
{
  Index_Graph<GT> idx(g);
  
  std::vector<GT::Node*> nodes;
  for (int i = 0; i < 200; ++i)
    nodes.push_back(idx.insert_node(i));
  
  EXPECT_EQ(idx.get_num_nodes(), 200u);
  
  // Remove half
  for (size_t i = 0; i < nodes.size(); i += 2)
    idx.remove_node(nodes[i]);
  
  EXPECT_EQ(idx.get_num_nodes(), 100u);
}

TEST_F(IndexGraphIntensiveTest, RandomInsertionOrder)
{
  Index_Graph<GT> idx(g);
  
  std::vector<int> values = {50, 25, 75, 10, 30, 60, 80, 5, 15, 20, 35, 40, 55, 65, 70, 85, 90, 95};
  
  for (int v : values)
    idx.insert_node(v);
  
  EXPECT_EQ(idx.get_num_nodes(), values.size());
  
  // All should be searchable
  for (int v : values)
  {
    auto n = idx.search_node(v);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->get_info(), v);
  }
}

// =============================================================================
// Arc Indexing Tests
// =============================================================================

TEST_F(IndexGraphIntensiveTest, ManyArcSearches)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(1);
  auto n2 = idx.insert_node(2);
  auto n3 = idx.insert_node(3);
  
  idx.insert_arc(n1, n2, 12);
  idx.insert_arc(n2, n3, 23);
  idx.insert_arc(n1, n3, 13);
  
  for (int i = 0; i < 1000; ++i)
  {
    auto a = idx.search_arc(n1, n2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->get_info(), 12);
  }
}

TEST_F(IndexGraphIntensiveTest, ArcRemovalAndReinsertion)
{
  Index_Graph<GT> idx(g);
  
  auto n1 = idx.insert_node(10);
  auto n2 = idx.insert_node(20);
  
  for (int i = 0; i < 50; ++i)
  {
    auto a = idx.insert_arc(n1, n2, i);
    EXPECT_NE(a, nullptr);
    
    idx.remove_arc(a);
    EXPECT_EQ(idx.search_arc(n1, n2), nullptr);
  }
}

// =============================================================================
// Complex Graph Patterns
// =============================================================================

TEST_F(IndexGraphIntensiveTest, StarGraph)
{
  Index_Graph<GT> idx(g);
  
  auto center = idx.insert_node(0);
  std::vector<GT::Node*> leaves;
  
  for (int i = 1; i <= 100; ++i)
  {
    auto leaf = idx.insert_node(i);
    leaves.push_back(leaf);
    idx.insert_arc(center, leaf, i);
  }
  
  EXPECT_EQ(idx.get_num_nodes(), 101u);
  EXPECT_EQ(idx.get_num_arcs(), 100u);
  
  // Verify center connectivity
  for (auto leaf : leaves)
  {
    auto a = idx.search_arc(center, leaf);
    EXPECT_NE(a, nullptr);
  }
}

TEST_F(IndexGraphIntensiveTest, ChainGraph)
{
  Index_Graph<GT> idx(g);
  
  const int N = 200;
  std::vector<GT::Node*> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(idx.insert_node(i));
  
  for (int i = 0; i < N - 1; ++i)
    idx.insert_arc(nodes[i], nodes[i + 1], i);
  
  EXPECT_EQ(idx.get_num_nodes(), static_cast<size_t>(N));
  EXPECT_EQ(idx.get_num_arcs(), static_cast<size_t>(N - 1));
}

TEST_F(IndexGraphIntensiveTest, GridGraph)
{
  Index_Graph<GT> idx(g);
  
  const int ROWS = 20;
  const int COLS = 20;
  std::vector<std::vector<GT::Node*>> grid(ROWS);
  
  // Create grid nodes
  for (int i = 0; i < ROWS; ++i)
  {
    for (int j = 0; j < COLS; ++j)
    {
      grid[i].push_back(idx.insert_node(i * COLS + j));
    }
  }
  
  // Connect horizontally and vertically
  for (int i = 0; i < ROWS; ++i)
  {
    for (int j = 0; j < COLS; ++j)
    {
      if (j < COLS - 1)
        idx.insert_arc(grid[i][j], grid[i][j + 1], 1);
      if (i < ROWS - 1)
        idx.insert_arc(grid[i][j], grid[i + 1][j], 1);
    }
  }
  
  EXPECT_EQ(idx.get_num_nodes(), static_cast<size_t>(ROWS * COLS));
}

// =============================================================================
// Iterator Performance Tests
// =============================================================================

TEST_F(IndexGraphIntensiveTest, IterateAllNodes)
{
  Index_Graph<GT> idx(g);
  
  const int N = 500;
  for (int i = 0; i < N; ++i)
    idx.insert_node(i);
  
  int count = 0;
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next())
  {
    count++;
    auto n = it.get_curr();
    EXPECT_GE(n->get_info(), 0);
    EXPECT_LT(n->get_info(), N);
  }
  
  EXPECT_EQ(count, N);
}

TEST_F(IndexGraphIntensiveTest, IterateAllArcs)
{
  Index_Graph<GT> idx(g);
  
  std::vector<GT::Node*> nodes;
  for (int i = 0; i < 20; ++i)
    nodes.push_back(idx.insert_node(i));
  
  int arc_count = 0;
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    for (size_t j = i + 1; j < nodes.size(); ++j)
    {
      idx.insert_arc(nodes[i], nodes[j], arc_count);
      arc_count++;
    }
  }
  
  int iterated = 0;
  for (Arc_Iterator<GT> it(g); it.has_curr(); it.next())
  {
    iterated++;
  }
  
  EXPECT_EQ(iterated, arc_count);
}

// =============================================================================
// Duplicate Handling Tests
// =============================================================================

TEST_F(IndexGraphIntensiveTest, DuplicateNodeValues)
{
  Index_Graph<GT> idx(g);
  
  idx.insert_node(42);
  idx.insert_node(42);  // Duplicate value
  
  // Only one node should exist
  EXPECT_EQ(idx.get_num_nodes(), 1u);
}

TEST_F(IndexGraphIntensiveTest, ManyDuplicateAttempts)
{
  Index_Graph<GT> idx(g);
  
  for (int i = 0; i < 100; ++i)
    idx.insert_node(5);
  
  EXPECT_EQ(idx.get_num_nodes(), 1u);
  
  auto n = idx.search_node(5);
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(n->get_info(), 5);
}

// =============================================================================
// Memory and Cleanup Tests
// =============================================================================

TEST_F(IndexGraphIntensiveTest, MultipleGraphInstances)
{
  // Test creating multiple graph instances
  for (int iter = 0; iter < 5; ++iter)
  {
    GT temp_g;
    Index_Graph<GT> idx(temp_g);
    
    for (int i = 0; i < 100; ++i)
      idx.insert_node(i);
    
    EXPECT_EQ(idx.get_num_nodes(), 100u);
  }
}

TEST_F(IndexGraphIntensiveTest, LargeGraphCleanup)
{
  Index_Graph<GT> idx(g);
  
  std::vector<GT::Node*> nodes;
  for (int i = 0; i < 200; ++i)
    nodes.push_back(idx.insert_node(i));
  
  for (size_t i = 0; i < nodes.size() - 1; ++i)
    idx.insert_arc(nodes[i], nodes[i + 1], i);
  
  EXPECT_GT(idx.get_num_nodes(), 0u);
  EXPECT_GT(idx.get_num_arcs(), 0u);
  
  // Let destructor clean up
}

// =============================================================================
// Search Efficiency Tests
// =============================================================================

TEST_F(IndexGraphIntensiveTest, SearchAfterManyOperations)
{
  Index_Graph<GT> idx(g);
  
  // Build complex graph
  for (int i = 0; i < 100; ++i)
    idx.insert_node(i);
  
  // Remove some
  for (int i = 20; i < 40; ++i)
  {
    auto n = idx.search_node(i);
    if (n)
      idx.remove_node(n);
  }
  
  // Add more
  for (int i = 100; i < 150; ++i)
    idx.insert_node(i);
  
  // Search should still be efficient
  for (int i = 0; i < 20; ++i)
  {
    auto n = idx.search_node(i);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->get_info(), i);
  }
  
  for (int i = 100; i < 150; ++i)
  {
    auto n = idx.search_node(i);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->get_info(), i);
  }
}

TEST_F(IndexGraphIntensiveTest, NegativeValueSearch)
{
  Index_Graph<GT> idx(g);
  
  for (int i = -50; i <= 50; ++i)
    idx.insert_node(i);
  
  EXPECT_EQ(idx.get_num_nodes(), 101u);
  
  for (int i = -50; i <= 50; i += 5)
  {
    auto n = idx.search_node(i);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->get_info(), i);
  }
}

// =============================================================================
// Extreme Cases
// =============================================================================

TEST_F(IndexGraphIntensiveTest, VeryLargeValues)
{
  Index_Graph<GT> idx(g);
  
  idx.insert_node(1000000);
  idx.insert_node(2000000);
  idx.insert_node(3000000);
  
  auto n = idx.search_node(2000000);
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(n->get_info(), 2000000);
}

TEST_F(IndexGraphIntensiveTest, MixedOperationPattern)
{
  Index_Graph<GT> idx(g);
  
  for (int i = 0; i < 100; ++i)
  {
    idx.insert_node(i);
    
    if (i % 10 == 0 && i > 0)
    {
      auto n = idx.search_node(i - 5);
      if (n)
        idx.remove_node(n);
    }
    
    if (i % 7 == 0)
    {
      auto n = idx.search_node(i);
      EXPECT_NE(n, nullptr);
    }
  }
  
  EXPECT_GT(idx.get_num_nodes(), 0u);
}
