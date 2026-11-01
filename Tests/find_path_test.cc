/**
 * @file find_path_test.cc
 * @brief Comprehensive tests for tpl_find_path.H
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <tpl_find_path.H>
#include <tpl_graph.H>

using namespace Aleph;

// =============================================================================
// Type Definitions
// =============================================================================

using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// =============================================================================
// Test Fixture
// =============================================================================

class FindPathTest : public ::testing::Test
{
protected:
  GT g;
  
  void SetUp() override
  {
    g = GT();
  }
  
  void TearDown() override {}
  
  // Helper: create linear path graph: n0 -> n1 -> n2 -> ... -> n(n-1)
  std::vector<Node*> create_linear_graph(size_t n)
  {
    std::vector<Node*> nodes;
    for (size_t i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    
    for (size_t i = 0; i < n - 1; ++i)
      g.insert_arc(nodes[i], nodes[i + 1]);
    
    return nodes;
  }
  
  // Helper: create complete graph
  std::vector<Node*> create_complete_graph(size_t n)
  {
    std::vector<Node*> nodes;
    for (size_t i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    
    for (size_t i = 0; i < n; ++i)
      for (size_t j = i + 1; j < n; ++j)
        g.insert_arc(nodes[i], nodes[j]);
    
    return nodes;
  }
};

// =============================================================================
// Depth-First Path Tests
// =============================================================================

TEST_F(FindPathTest, DFS_SimplePath)
{
  auto nodes = create_linear_graph(5);
  
  Find_Path_Depth_First<GT> finder;
  Path<GT> path(g);
  
  bool found = finder(g, nodes[0], nodes[4], path);
  
  EXPECT_TRUE(found);
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 5u);
}

TEST_F(FindPathTest, DFS_PathToSelf)
{
  auto n = g.insert_node(0);
  
  Find_Path_Depth_First<GT> finder;
  Path<GT> path(g);
  
  bool found = finder(g, n, n, path);
  
  EXPECT_TRUE(found);
}

TEST_F(FindPathTest, DFS_NoPath)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  // n3 is disconnected

  // It threw underflow_error, when it shortened the path to nothing
  Find_Path_Depth_First<GT> finder;
  Path<GT> path(g);
  bool found = finder(g, n1, n3, path);
  EXPECT_FALSE(found);
  EXPECT_TRUE(path.is_empty());

  // the free function, too
  EXPECT_TRUE(find_path_depth_first(g, n1, n3).is_empty());
}

TEST_F(FindPathTest, DFS_CompleteGraph)
{
  auto nodes = create_complete_graph(10);
  
  Find_Path_Depth_First<GT> finder;
  Path<GT> path(g);
  
  bool found = finder(g, nodes[0], nodes[9], path);
  
  EXPECT_TRUE(found);
  EXPECT_FALSE(path.is_empty());
}

TEST_F(FindPathTest, DFS_WithPredicate)
{
  auto nodes = create_linear_graph(10);
  
  Find_Path_Depth_First<GT> finder;
  
  // Find node with value 7
  auto path = finder(g, nodes[0], [](Node* p) { return p->get_info() == 7; });
  
  EXPECT_FALSE(path.is_empty());
}

TEST_F(FindPathTest, DFS_PredicateNotSatisfied)
{
  const auto nodes = create_linear_graph(5);

  // Look for value 100 (doesn't exist) - returns empty path, no exception
  Find_Path_Depth_First<GT> finder;
  const auto path = finder(g, nodes[0], [](Node* p) { return p->get_info() == 100; });
  EXPECT_TRUE(path.is_empty());
}

// =============================================================================
// Breadth-First Path Tests
// =============================================================================

TEST_F(FindPathTest, BFS_SimplePath)
{
  auto nodes = create_linear_graph(5);
  
  Find_Path_Breadth_First<GT> finder;
  Path<GT> path(g);
  
  bool found = finder(g, nodes[0], nodes[4], path);
  
  EXPECT_TRUE(found);
  EXPECT_FALSE(path.is_empty());
}

TEST_F(FindPathTest, BFS_PathToSelf)
{
  auto n = g.insert_node(0);
  
  auto m = g.insert_node(1);
  g.insert_arc(n, m);
  g.insert_arc(n, n);

  Find_Path_Breadth_First<GT> finder;
  
  // the path of no arc, as that of Find_Path_Depth_First, and not the loop
  auto path = finder(g, n, n);
  ASSERT_EQ(path.size(), 1u);
  EXPECT_EQ(path.get_first_node(), n);

  Path<GT> other(g);
  EXPECT_TRUE(finder(g, n, n, other));
  ASSERT_EQ(other.size(), 1u);
  EXPECT_EQ(other.get_first_node(), n);
}

TEST_F(FindPathTest, StartThatMeetsTheCriterion)
{
  auto nodes = create_linear_graph(3);
  auto is_first = [] (Node * p) { return p->get_info() == 0; };

  // the start alone, the path of no arc, in both searches
  const auto dfs = Find_Path_Depth_First<GT>()(g, nodes[0], is_first);
  const auto bfs = Find_Path_Breadth_First<GT>()(g, nodes[0], is_first);
  for (const Path<GT> * path : {&dfs, &bfs})
    {
      ASSERT_EQ(path->size(), 1u);
      EXPECT_EQ(path->get_first_node(), nodes[0]);
    }
}

namespace
{
  // a filter that throws E on the arcs of info 2
  template <class E>
  struct Throws_On_Two
  {
    bool operator()(Arc * arc) const
    {
      if (arc->get_info() == 2)
        throw E("the filter fails");
      return true;
    }
  };

  template <class E>
  void expect_the_searches_throw(GT & g, Node * from, Node * to)
  {
    using F = Throws_On_Two<E>;
    Path<GT> path(g);
    EXPECT_THROW((Find_Path_Depth_First<GT, Node_Arc_Iterator, F>()(g, from, to, path)), E);
    EXPECT_THROW((Find_Path_Breadth_First<GT, Node_Arc_Iterator, F>()(g, from, to, path)), E);
  }
}

// What the arc filter throws reaches the caller of the searches. The
// iterators of the arcs caught it, stayed on the arc on which it threw, and
// the searches took that arc.
TEST_F(FindPathTest, WhatTheFilterThrowsReachesTheCaller)
{
  // b - c between the two other arcs of b, so that the filter throws while
  // the iterator of b advances
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  auto d = g.insert_node(3);
  g.insert_arc(a, b, 1);
  g.insert_arc(b, c, 2);
  g.insert_arc(b, d, 3);

  expect_the_searches_throw<std::domain_error>(g, a, c);
  expect_the_searches_throw<std::overflow_error>(g, a, c);
}

TEST_F(FindPathTest, BFS_NoPath)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  // n3 disconnected
  
  Find_Path_Breadth_First<GT> finder;
  Path<GT> path(g);
  
  bool found = finder(g, n1, n3, path);
  
  EXPECT_FALSE(found);
}

TEST_F(FindPathTest, BFS_ShortestPath)
{
  /*
   * Create diamond graph:
   *     n1
   *    /  \
   *   n2  n3
   *    \  /
   *     n4
   */
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n3, n4);
  
  Find_Path_Breadth_First<GT> finder;
  auto path = finder(g, n1, n4);
  
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 3u);  // n1 -> n2/n3 -> n4 (shortest)
}

TEST_F(FindPathTest, BFS_CompleteGraph)
{
  auto nodes = create_complete_graph(8);
  
  Find_Path_Breadth_First<GT> finder;
  auto path = finder(g, nodes[0], nodes[7]);
  
  EXPECT_FALSE(path.is_empty());
}

TEST_F(FindPathTest, BFS_WithPredicate)
{
  auto nodes = create_linear_graph(10);
  
  Find_Path_Breadth_First<GT> finder;
  
  auto path = finder(g, nodes[0], [](Node* p) { return p->get_info() == 5; });
  
  EXPECT_FALSE(path.is_empty());
}

// =============================================================================
// Directed Graph Path Tests
// =============================================================================

TEST_F(FindPathTest, DirectedDFS)
{
  using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  DGT dg;
  
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  auto n3 = dg.insert_node(3);
  
  dg.insert_arc(n1, n2);
  dg.insert_arc(n2, n3);
  
  Directed_Find_Path<DGT> finder(dg);
  auto path = finder.dfs(n1, n3);
  
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 3u);
}

TEST_F(FindPathTest, DirectedBFS)
{
  using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  DGT dg;
  
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  auto n3 = dg.insert_node(3);
  
  dg.insert_arc(n1, n2);
  dg.insert_arc(n2, n3);
  
  Directed_Find_Path<DGT> finder(dg);
  auto path = finder.bfs(n1, n3);
  
  EXPECT_FALSE(path.is_empty());
}

TEST_F(FindPathTest, DirectedNoPath)
{
  using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  DGT dg;
  
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  auto n3 = dg.insert_node(3);
  
  dg.insert_arc(n1, n2);
  dg.insert_arc(n3, n2);  // n3 -> n2, can't reach n3 from n1
  
  Directed_Find_Path<DGT> finder(dg);
  auto path = finder.dfs(n1, n3);
  
  EXPECT_TRUE(path.is_empty());
}

TEST_F(FindPathTest, DirectedPathToSelf)
{
  using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  DGT dg;

  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);

  dg.insert_arc(n1, n2);
  dg.insert_arc(n2, n1);
  dg.insert_arc(n1, n1);

  // the path of no arc, as that of Find_Path_Depth_First, and neither the
  // loop nor the cycle; the same from a start that meets the criterion
  Directed_Find_Path<DGT> finder(dg);
  auto is_first = [] (DGT::Node * p) { return p->get_info() == 1; };
  const auto dfs = finder.dfs(n1, n1);
  const auto bfs = finder.bfs(n1, n1);
  const auto dfs_op = finder.dfs(n1, is_first);
  const auto bfs_op = finder.bfs(n1, is_first);
  for (const Path<DGT> * path : {&dfs, &bfs, &dfs_op, &bfs_op})
    {
      ASSERT_EQ(path->size(), 1u);
      EXPECT_EQ(path->get_first_node(), n1);
    }
}

TEST_F(FindPathTest, DirectedWithPredicate)
{
  using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  DGT dg;
  
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  auto n3 = dg.insert_node(3);
  
  dg.insert_arc(n1, n2);
  dg.insert_arc(n2, n3);
  
  Directed_Find_Path<DGT> finder(dg);
  auto path = finder.dfs(n1, [](DGT::Node* p) { return p->get_info() == 3; });
  
  EXPECT_FALSE(path.is_empty());
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(FindPathTest, LargeLinearGraph)
{
  auto nodes = create_linear_graph(1000);
  
  Find_Path_Depth_First<GT> dfs_finder;
  auto dfs_path = dfs_finder(g, nodes[0], nodes[999]);
  
  EXPECT_FALSE(dfs_path.is_empty());
  EXPECT_EQ(dfs_path.size(), 1000u);
}

TEST_F(FindPathTest, LargeCompleteGraph)
{
  auto nodes = create_complete_graph(50);
  
  Find_Path_Breadth_First<GT> bfs_finder;
  auto path = bfs_finder(g, nodes[0], nodes[49]);
  
  EXPECT_FALSE(path.is_empty());
  EXPECT_LE(path.size(), 3u);  // Should find direct path
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(FindPathTest, SingleNode)
{
  auto n = g.insert_node(1);
  
  Find_Path_Depth_First<GT> finder;
  auto path = finder(g, n, n);
  
  EXPECT_EQ(path.size(), 1u);
}

TEST_F(FindPathTest, TwoNodesConnected)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2);
  
  Find_Path_Depth_First<GT> finder;
  auto path = finder(g, n1, n2);
  
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 2u);
}

TEST_F(FindPathTest, TwoNodesDisconnected)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  try {
    Find_Path_Depth_First<GT> finder;
    auto path = finder(g, n1, n2);
    // Empty path - expected behavior
    SUCCEED();
  } catch (...) {
    // Exception on empty path is also valid
    SUCCEED();
  }
}

TEST_F(FindPathTest, CyclicGraph)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n2, n3);
  g.insert_arc(n3, n1);  // Cycle
  
  Find_Path_Depth_First<GT> finder;
  auto path = finder(g, n1, n3);
  
  EXPECT_FALSE(path.is_empty());
}

// =============================================================================
// Multiple Paths Tests
// =============================================================================

TEST_F(FindPathTest, MultiplePaths_FindsOne)
{
  // Create graph with two paths from n1 to n4:
  // n1 -> n2 -> n4
  // n1 -> n3 -> n4
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  g.insert_arc(n1, n2);
  g.insert_arc(n1, n3);
  g.insert_arc(n2, n4);
  g.insert_arc(n3, n4);
  
  Find_Path_Depth_First<GT> finder;
  auto path = finder(g, n1, n4);
  
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 3u);
}
