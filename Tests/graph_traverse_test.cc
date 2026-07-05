
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/



/**
 * @file graph_traverse_test.cc
 * @brief Tests for Graph Traverse
 */
#include <gtest/gtest.h>
#include <vector>
#include <set>
#include <graph-traverse.H>
#include <tpl_graph.H>

using namespace Aleph;

// Test graph types
using TestGraph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using TestDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;

// =============================================================================
// Test Fixtures
// =============================================================================

class GraphTraverseTest : public ::testing::Test
{
protected:
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  void SetUp() override
  {
    // Create a simple connected graph:
    //     0 --- 1
    //     |     |
    //     2 --- 3 --- 4
    for (int i = 0; i < 5; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[0], nodes[2], 2.0);
    g.insert_arc(nodes[1], nodes[3], 3.0);
    g.insert_arc(nodes[2], nodes[3], 4.0);
    g.insert_arc(nodes[3], nodes[4], 5.0);
  }
};

class DisconnectedGraphTest : public ::testing::Test
{
protected:
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  void SetUp() override
  {
    // Create two disconnected components:
    // Component 1: 0 --- 1 --- 2
    // Component 2: 3 --- 4
    for (int i = 0; i < 5; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[3], nodes[4], 3.0);
  }
};

class TreeGraphTest : public ::testing::Test
{
protected:
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  void SetUp() override
  {
    /*
     * Create a tree:
     *        0
     *       /|\
     *      1 2 3
     *     /|   |
     *    4 5   6
     */
    for (int i = 0; i < 7; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[0], nodes[2], 2.0);
    g.insert_arc(nodes[0], nodes[3], 3.0);
    g.insert_arc(nodes[1], nodes[4], 4.0);
    g.insert_arc(nodes[1], nodes[5], 5.0);
    g.insert_arc(nodes[3], nodes[6], 6.0);
  }
};

class CyclicGraphTest : public ::testing::Test
{
protected:
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  void SetUp() override
  {
    // Create a graph with a cycle:
    //   0 --- 1
    //   |     |
    //   3 --- 2
    for (int i = 0; i < 4; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[2], nodes[3], 3.0);
    g.insert_arc(nodes[3], nodes[0], 4.0);
  }
};

class DigraphTraverseTest : public ::testing::Test
{
protected:
  TestDigraph g;
  std::vector<TestDigraph::Node *> nodes;

  void SetUp() override
  {
    // Create a simple directed graph:
    //   0 --> 1 --> 2
    //   |           ^
    //   v           |
    //   3 ----------+
    for (int i = 0; i < 4; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[0], nodes[3], 3.0);
    g.insert_arc(nodes[3], nodes[2], 4.0);
  }
};

// =============================================================================
// DFS Traversal Tests
// =============================================================================

TEST_F(GraphTraverseTest, DFSVisitsAllNodes)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = dfs(nodes[0], op);

  EXPECT_EQ(count, 5);
  EXPECT_EQ(visited.size(), 5);
  for (int i = 0; i < 5; ++i)
    EXPECT_TRUE(visited.count(i) > 0);
}

TEST_F(GraphTraverseTest, DFSStartsFromCorrectNode)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  int first_visited = -1;
  auto op = [&first_visited](TestGraph::Node *node) {
    if (first_visited == -1)
      first_visited = node->get_info();
    return true;
  };

  dfs(nodes[2], op);

  EXPECT_EQ(first_visited, 2);
}

TEST_F(GraphTraverseTest, DFSEarlyTermination)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  int visit_count = 0;
  auto op = [&visit_count](TestGraph::Node *) {
    ++visit_count;
    return visit_count < 3; // Stop after visiting 3 nodes
  };

  size_t count = dfs(nodes[0], op);

  EXPECT_EQ(count, 3);
  EXPECT_EQ(visit_count, 3);
}

// =============================================================================
// BFS Traversal Tests
// =============================================================================

TEST_F(GraphTraverseTest, BFSVisitsAllNodes)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = bfs(nodes[0], op);

  EXPECT_EQ(count, 5);
  EXPECT_EQ(visited.size(), 5);
}

TEST_F(TreeGraphTest, BFSVisitsInLevelOrder)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  std::vector<int> visit_order;
  auto op = [&visit_order](TestGraph::Node *node) {
    visit_order.push_back(node->get_info());
    return true;
  };

  bfs(nodes[0], op);

  // First should be root (0)
  EXPECT_EQ(visit_order[0], 0);

  // Level 1 nodes (1, 2, 3) should come before level 2 nodes (4, 5, 6)
  std::set<int> level1 = {1, 2, 3};
  std::set<int> level2 = {4, 5, 6};

  // Indices 1, 2, 3 should be level 1 nodes
  for (int i = 1; i <= 3; ++i)
    EXPECT_TRUE(level1.count(visit_order[i]) > 0);

  // Indices 4, 5, 6 should be level 2 nodes
  for (int i = 4; i <= 6; ++i)
    EXPECT_TRUE(level2.count(visit_order[i]) > 0);
}

TEST_F(GraphTraverseTest, BFSEarlyTermination)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  int visit_count = 0;
  auto op = [&visit_count](TestGraph::Node *) {
    ++visit_count;
    return visit_count < 2;
  };

  size_t count = bfs(nodes[0], op);

  EXPECT_EQ(count, 2);
}

// =============================================================================
// Disconnected Graph Tests
// =============================================================================

TEST_F(DisconnectedGraphTest, TraversalStopsAtComponentBoundary)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  // Start from component 1
  size_t count = dfs(nodes[0], op);

  EXPECT_EQ(count, 3); // Only nodes 0, 1, 2
  EXPECT_TRUE(visited.count(0) > 0);
  EXPECT_TRUE(visited.count(1) > 0);
  EXPECT_TRUE(visited.count(2) > 0);
  EXPECT_FALSE(visited.count(3) > 0);
  EXPECT_FALSE(visited.count(4) > 0);
}

TEST_F(DisconnectedGraphTest, CanTraverseSecondComponent)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  // Start from component 2
  size_t count = dfs(nodes[3], op);

  EXPECT_EQ(count, 2); // Only nodes 3, 4
  EXPECT_TRUE(visited.count(3) > 0);
  EXPECT_TRUE(visited.count(4) > 0);
}

// =============================================================================
// Cyclic Graph Tests
// =============================================================================

TEST_F(CyclicGraphTest, DFSHandlesCycle)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = dfs(nodes[0], op);

  // Should visit all 4 nodes exactly once despite cycle
  EXPECT_EQ(count, 4);
  EXPECT_EQ(visited.size(), 4);
}

TEST_F(CyclicGraphTest, BFSHandlesCycle)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = bfs(nodes[0], op);

  EXPECT_EQ(count, 4);
  EXPECT_EQ(visited.size(), 4);
}

// =============================================================================
// exec() Method Tests (with arc information)
// =============================================================================

TEST_F(GraphTraverseTest, ExecProvidesArcInformation)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::vector<std::pair<int, bool>> visits; // (node_info, has_arc)
  auto op = [&visits](TestGraph::Node *node, TestGraph::Arc *arc) {
    visits.push_back({node->get_info(), arc != nullptr});
    return true;
  };

  size_t count = dfs.exec(nodes[0], op);

  EXPECT_EQ(count, 5);

  // First node should have nullptr arc
  EXPECT_EQ(visits[0].first, 0);
  EXPECT_FALSE(visits[0].second);

  // All other nodes should have an arc
  for (size_t i = 1; i < visits.size(); ++i)
    EXPECT_TRUE(visits[i].second);
}

TEST_F(GraphTraverseTest, ExecEarlyTermination)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  int visit_count = 0;
  auto op = [&visit_count](TestGraph::Node *, TestGraph::Arc *) {
    ++visit_count;
    return visit_count < 2;
  };

  size_t count = bfs.exec(nodes[0], op);

  EXPECT_EQ(count, 2);
}

// =============================================================================
// Dual Operation Tests (node_op and arc_op)
// =============================================================================

TEST_F(GraphTraverseTest, DualOpVisitsNodesAndArcs)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::set<int> visited_nodes;
  int arc_count = 0;

  auto node_op = [&visited_nodes](TestGraph::Node *node) {
    visited_nodes.insert(node->get_info());
    return true;
  };

  auto arc_op = [&arc_count](TestGraph::Arc *) {
    ++arc_count;
    return true;
  };

  auto [nodes_visited, arcs_visited] = dfs(nodes[0], node_op, arc_op);

  EXPECT_EQ(nodes_visited, 5);
  EXPECT_EQ(visited_nodes.size(), 5);
  // The graph has 5 arcs, DFS visits all of them (tree + back edges)
  EXPECT_EQ(arcs_visited, 5);
  EXPECT_EQ(arc_count, 5);
}

TEST_F(GraphTraverseTest, DualOpNodeEarlyTermination)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  int node_count = 0;
  int arc_count = 0;

  auto node_op = [&node_count](TestGraph::Node *) {
    ++node_count;
    return node_count < 2; // Stop after 2 nodes
  };

  auto arc_op = [&arc_count](TestGraph::Arc *) {
    ++arc_count;
    return true;
  };

  auto [nodes_visited, arcs_visited] = dfs(nodes[0], node_op, arc_op);

  EXPECT_EQ(nodes_visited, 2);
}

TEST_F(GraphTraverseTest, DualOpArcEarlyTermination)
{
  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  int node_count = 0;
  int arc_count = 0;

  auto node_op = [&node_count](TestGraph::Node *) {
    ++node_count;
    return true;
  };

  auto arc_op = [&arc_count](TestGraph::Arc *) {
    ++arc_count;
    return arc_count < 2; // Stop after 2 arcs
  };

  auto [nodes_visited, arcs_visited] = dfs(nodes[0], node_op, arc_op);

  EXPECT_EQ(arcs_visited, 2);
}

// =============================================================================
// Digraph Tests (using Node_Arc_Iterator with matching filter)
// =============================================================================

TEST_F(DigraphTraverseTest, DFSOnDigraph)
{
  using Itor = Node_Arc_Iterator<TestDigraph>;
  using Filter = Dft_Show_Arc<TestDigraph>;
  Graph_Traverse<TestDigraph, Itor, DynListStack, Filter> dfs(g);

  std::set<int> visited;
  auto op = [&visited](TestDigraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = dfs(nodes[0], op);

  // Should reach all 4 nodes following directed edges
  EXPECT_EQ(count, 4);
  EXPECT_EQ(visited.size(), 4);
}

TEST_F(DigraphTraverseTest, BFSOnDigraph)
{
  using Itor = Node_Arc_Iterator<TestDigraph>;
  using Filter = Dft_Show_Arc<TestDigraph>;
  Graph_Traverse<TestDigraph, Itor, DynListQueue, Filter> bfs(g);

  std::vector<int> visit_order;
  auto op = [&visit_order](TestDigraph::Node *node) {
    visit_order.push_back(node->get_info());
    return true;
  };

  bfs(nodes[0], op);

  EXPECT_EQ(visit_order.size(), 4);
  EXPECT_EQ(visit_order[0], 0); // Start node first
}

// =============================================================================
// Single Node Graph Tests
// =============================================================================

TEST(SingleNodeTest, DFSSingleNode)
{
  TestGraph g;
  auto node = g.insert_node(42);

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  int visited_value = -1;
  auto op = [&visited_value](TestGraph::Node *n) {
    visited_value = n->get_info();
    return true;
  };

  size_t count = dfs(node, op);

  EXPECT_EQ(count, 1);
  EXPECT_EQ(visited_value, 42);
}

TEST(SingleNodeTest, BFSSingleNode)
{
  TestGraph g;
  auto node = g.insert_node(42);

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  int visited_value = -1;
  auto op = [&visited_value](TestGraph::Node *n) {
    visited_value = n->get_info();
    return true;
  };

  size_t count = bfs(node, op);

  EXPECT_EQ(count, 1);
  EXPECT_EQ(visited_value, 42);
}

// =============================================================================
// Arc Filter Tests
// =============================================================================

template <class GT>
struct EvenArcFilter
{
  bool operator()(typename GT::Arc *arc) const
  {
    return static_cast<int>(arc->get_info()) % 2 == 0;
  }
};

TEST_F(GraphTraverseTest, DFSWithArcFilter)
{
  using Itor = Node_Arc_Iterator<TestGraph, EvenArcFilter<TestGraph>>;
  Graph_Traverse_DFS<TestGraph, Itor, EvenArcFilter<TestGraph>> dfs(g, EvenArcFilter<TestGraph>());

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = dfs(nodes[0], op);

  // With even arc filter, only arcs with weights 2.0 and 4.0 are visible
  // From node 0: only arc to node 2 (weight 2.0) is visible
  // From node 2: only arc to node 3 (weight 4.0) is visible
  // Node 3 has arc to node 4 (weight 5.0 - odd, filtered out)
  EXPECT_LE(count, 5);
  EXPECT_TRUE(visited.count(0) > 0);
  EXPECT_TRUE(visited.count(2) > 0);
}

// =============================================================================
// Linear Chain Tests
// =============================================================================

TEST(LinearChainTest, DFSLinearChain)
{
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  // Create linear chain: 0 -- 1 -- 2 -- 3 -- 4
  for (int i = 0; i < 5; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < 4; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], static_cast<double>(i));

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::vector<int> visit_order;
  auto op = [&visit_order](TestGraph::Node *node) {
    visit_order.push_back(node->get_info());
    return true;
  };

  dfs(nodes[0], op);

  EXPECT_EQ(visit_order.size(), 5);
  EXPECT_EQ(visit_order[0], 0); // Must start at 0
}

TEST(LinearChainTest, BFSLinearChain)
{
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  for (int i = 0; i < 5; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < 4; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], static_cast<double>(i));

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  std::vector<int> visit_order;
  auto op = [&visit_order](TestGraph::Node *node) {
    visit_order.push_back(node->get_info());
    return true;
  };

  bfs(nodes[0], op);

  // BFS on a linear chain visits in order
  EXPECT_EQ(visit_order.size(), 5);
  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(visit_order[i], i);
}

// =============================================================================
// Complete Graph Tests
// =============================================================================

TEST(CompleteGraphTest, DFSCompleteGraph)
{
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  // Create K4 (complete graph with 4 nodes)
  for (int i = 0; i < 4; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
      g.insert_arc(nodes[i], nodes[j], static_cast<double>(i * 10 + j));

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  std::set<int> visited;
  auto op = [&visited](TestGraph::Node *node) {
    visited.insert(node->get_info());
    return true;
  };

  size_t count = dfs(nodes[0], op);

  EXPECT_EQ(count, 4);
  EXPECT_EQ(visited.size(), 4);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(StressTest, LargeGraphDFS)
{
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  const int N = 1000;

  // Create a chain of 1000 nodes
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1.0);

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);

  int count = 0;
  auto op = [&count](TestGraph::Node *) {
    ++count;
    return true;
  };

  size_t result = dfs(nodes[0], op);

  EXPECT_EQ(result, N);
  EXPECT_EQ(count, N);
}

TEST(StressTest, LargeGraphBFS)
{
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;

  const int N = 1000;

  // Create a star graph (central node connected to all others)
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 1; i < N; ++i)
    g.insert_arc(nodes[0], nodes[i], 1.0);

  using Itor = Node_Arc_Iterator<TestGraph>;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);

  int count = 0;
  auto op = [&count](TestGraph::Node *) {
    ++count;
    return true;
  };

  size_t result = bfs(nodes[0], op);

  EXPECT_EQ(result, N);
  EXPECT_EQ(count, N);
}

// =============================================================================
// Self-Loop Regression Tests
// =============================================================================
//
// The initial frontier-seeding loop in operator()(start, op), exec(), and
// operator()(start, node_op, arc_op) used to unconditionally overwrite the
// target node's state to Processing. For a self-loop at the start node
// (tgt == start), this clobbered start's already-Processed state, causing
// it to be re-discovered and visited a second time — and, depending on
// queue ordering, could prevent other reachable nodes from being visited
// at all.

TEST(SelfLoopTest, SingleOpVisitsStartOnce)
{
  TestGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  g.insert_arc(a, a, 0.0);  // self-loop at the start node
  g.insert_arc(a, b, 1.0);

  using Itor = Node_Arc_Iterator<TestGraph>;
  std::vector<int> bfs_seen;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);
  size_t bfs_count = bfs(a, [&](TestGraph::Node *n) {
    bfs_seen.push_back(n->get_info());
    return true;
  });
  EXPECT_EQ(bfs_count, 2u);
  EXPECT_EQ(bfs_seen, (std::vector<int>{1, 2}));

  std::vector<int> dfs_seen;
  Graph_Traverse_DFS<TestGraph, Itor> dfs(g);
  size_t dfs_count = dfs(a, [&](TestGraph::Node *n) {
    dfs_seen.push_back(n->get_info());
    return true;
  });
  EXPECT_EQ(dfs_count, 2u);
  EXPECT_EQ(dfs_seen, (std::vector<int>{1, 2}));
}

TEST(SelfLoopTest, ExecVisitsStartOnce)
{
  TestGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  g.insert_arc(a, a, 0.0);
  g.insert_arc(a, b, 1.0);

  using Itor = Node_Arc_Iterator<TestGraph>;
  std::vector<int> seen;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);
  size_t count = bfs.exec(a, [&](TestGraph::Node *n, TestGraph::Arc *) {
    seen.push_back(n->get_info());
    return true;
  });
  EXPECT_EQ(count, 2u);
  EXPECT_EQ(seen, (std::vector<int>{1, 2}));
}

TEST(SelfLoopTest, DualOpVisitsStartOnceAndSkipsSelfLoopArc)
{
  TestGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  g.insert_arc(a, a, 0.0);
  g.insert_arc(a, b, 1.0);

  using Itor = Node_Arc_Iterator<TestGraph>;
  std::vector<int> node_seen;
  int arc_count = 0;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);
  auto node_op = [&](TestGraph::Node *n) {
    node_seen.push_back(n->get_info());
    return true;
  };
  auto arc_op = [&](TestGraph::Arc *) {
    ++arc_count;
    return true;
  };
  auto [node_count, total_arc_count] = bfs(a, node_op, arc_op);

  EXPECT_EQ(node_count, 2u);
  EXPECT_EQ(node_seen, (std::vector<int>{1, 2}));
  // Only the real arc to b is reported; the self-loop arc leads to an
  // already-Processed node and is skipped, matching the main loop's own
  // "don't re-report arcs into already-visited nodes" behavior.
  EXPECT_EQ(total_arc_count, 1u);
  EXPECT_EQ(arc_count, 1);
}

TEST(SelfLoopTest, MidTraversalSelfLoopDoesNotBlockDiscovery)
{
  // A self-loop on a node discovered mid-traversal was already handled
  // correctly by the main loop's own guard; kept here for symmetry with
  // the start-node cases above.
  TestGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  auto *c = g.insert_node(3);
  g.insert_arc(a, b, 1.0);
  g.insert_arc(b, b, 0.0);  // self-loop at a non-start node
  g.insert_arc(b, c, 1.0);

  using Itor = Node_Arc_Iterator<TestGraph>;
  std::vector<int> seen;
  Graph_Traverse_BFS<TestGraph, Itor> bfs(g);
  size_t count = bfs(a, [&](TestGraph::Node *n) {
    seen.push_back(n->get_info());
    return true;
  });
  EXPECT_EQ(count, 3u);
  EXPECT_EQ(seen, (std::vector<int>{1, 2, 3}));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
