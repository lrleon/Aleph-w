
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


/**
 * @file tpl_graph_test.cc
 * @brief Exhaustive tests for tpl_graph.H (List_Graph, List_Digraph, Path)
 *
 * These tests cover all major operations including:
 * - Node and arc insertion/removal
 * - Iterators (Node_Iterator, Arc_Iterator, Node_Arc_Iterator)
 * - Filtered iterators
 * - Path class operations
 * - Graph copy operations
 * - Directed graph operations
 * - Edge cases and error conditions
 */

#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <tpl_graph.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Type Definitions
// =============================================================================

using Graph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using TestDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using StringGraph = List_Graph<Graph_Node<string>, Graph_Arc<string>>;

// =============================================================================
// Graph_Node Tests
// =============================================================================

TEST(GraphNodeTest, DefaultConstruction)
{
  Graph_Node<int> node;
  EXPECT_EQ(node.get_info(), 0);
  EXPECT_EQ(node.num_arcs, 0u);
}

TEST(GraphNodeTest, ConstructionWithInfo)
{
  Graph_Node<int> node(42);
  EXPECT_EQ(node.get_info(), 42);
}

TEST(GraphNodeTest, CopyConstruction)
{
  Graph_Node<int> node1(42);
  Graph_Node<int> node2(node1);
  EXPECT_EQ(node2.get_info(), 42);
}

TEST(GraphNodeTest, MoveConstruction)
{
  Graph_Node<string> node1(string("test"));
  Graph_Node<string> node2(std::move(node1));
  EXPECT_EQ(node2.get_info(), "test");
}

TEST(GraphNodeTest, CopyAssignment)
{
  Graph_Node<int> node1(42);
  Graph_Node<int> node2(100);
  node2 = node1;
  EXPECT_EQ(node2.get_info(), 42);
}

TEST(GraphNodeTest, SelfAssignment)
{
  Graph_Node<int> node(42);
  node = node;
  EXPECT_EQ(node.get_info(), 42);
}

TEST(GraphNodeTest, ConstructionFromPointer)
{
  Graph_Node<int> node1(42);
  Graph_Node<int> node2(&node1);
  EXPECT_EQ(node2.get_info(), 42);
}

// =============================================================================
// Graph_Arc Tests
// =============================================================================

TEST(GraphArcTest, DefaultConstruction)
{
  Graph_Arc<double> arc;
  EXPECT_EQ(arc.get_info(), 0.0);
}

TEST(GraphArcTest, ConstructionWithInfo)
{
  Graph_Arc<double> arc(3.14);
  EXPECT_EQ(arc.get_info(), 3.14);
}

TEST(GraphArcTest, CopyConstruction)
{
  Graph_Arc<double> arc1(3.14);
  Graph_Arc<double> arc2(arc1);
  EXPECT_EQ(arc2.get_info(), 3.14);
}

TEST(GraphArcTest, CopyAssignment)
{
  Graph_Arc<double> arc1(3.14);
  Graph_Arc<double> arc2(1.0);
  arc2 = arc1;
  EXPECT_EQ(arc2.get_info(), 3.14);
}

TEST(GraphArcTest, SelfAssignment)
{
  Graph_Arc<double> arc(3.14);
  arc = arc;
  EXPECT_EQ(arc.get_info(), 3.14);
}

// =============================================================================
// Basic Graph Construction Tests
// =============================================================================

class GraphBasicTest : public Test
{
protected:
  Graph g;
  TestDigraph dg;
};

TEST_F(GraphBasicTest, DefaultConstructorCreatesEmptyGraph)
{
  EXPECT_EQ(g.get_num_nodes(), 0);
  EXPECT_EQ(g.get_num_arcs(), 0);
  EXPECT_FALSE(g.is_digraph());
}

TEST_F(GraphBasicTest, DefaultConstructorCreatesEmptyDigraph)
{
  EXPECT_EQ(dg.get_num_nodes(), 0);
  EXPECT_EQ(dg.get_num_arcs(), 0);
  EXPECT_TRUE(dg.is_digraph());
}

TEST_F(GraphBasicTest, InsertSingleNode)
{
  auto n = g.insert_node(10);
  EXPECT_NE(n, nullptr);
  EXPECT_EQ(n->get_info(), 10);
  EXPECT_EQ(g.get_num_nodes(), 1);
  EXPECT_EQ(g.get_num_arcs(), 0);
}

TEST_F(GraphBasicTest, InsertMultipleNodes)
{
  vector<Graph::Node*> nodes;
  for (int i = 0; i < 100; ++i)
    nodes.push_back(g.insert_node(i));

  EXPECT_EQ(g.get_num_nodes(), 100);
  for (int i = 0; i < 100; ++i)
    EXPECT_EQ(nodes[i]->get_info(), i);
}

TEST_F(GraphBasicTest, InsertArcBetweenTwoNodes)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 1.5);

  EXPECT_NE(a, nullptr);
  EXPECT_EQ(a->get_info(), 1.5);
  EXPECT_EQ(g.get_num_arcs(), 1);
  EXPECT_EQ(g.get_src_node(a), n1);
  EXPECT_EQ(g.get_tgt_node(a), n2);
}

TEST_F(GraphBasicTest, InsertSelfLoop)
{
  auto n = g.insert_node(1);
  auto a = g.insert_arc(n, n, 0.0);

  EXPECT_NE(a, nullptr);
  EXPECT_EQ(g.get_num_arcs(), 1);
  EXPECT_EQ(g.get_src_node(a), n);
  EXPECT_EQ(g.get_tgt_node(a), n);
}

// =============================================================================
// Node Removal Tests
// =============================================================================

class GraphNodeRemovalTest : public Test
{
protected:
  Graph g;

  void SetUp() override
  {
    for (int i = 0; i < 5; ++i)
      g.insert_node(i);
  }
};

TEST_F(GraphNodeRemovalTest, RemoveSingleNode)
{
  auto n = g.get_first_node();
  g.remove_node(n);
  EXPECT_EQ(g.get_num_nodes(), 4);
}

TEST_F(GraphNodeRemovalTest, RemoveNodeWithArcs)
{
  vector<Graph::Node*> nodes;
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    nodes.push_back(it.get_curr());

  g.insert_arc(nodes[0], nodes[1], 1.0);
  g.insert_arc(nodes[0], nodes[2], 2.0);
  g.insert_arc(nodes[1], nodes[2], 3.0);

  EXPECT_EQ(g.get_num_arcs(), 3);
  g.remove_node(nodes[0]);
  EXPECT_EQ(g.get_num_nodes(), 4);
  EXPECT_EQ(g.get_num_arcs(), 1);
}

TEST_F(GraphNodeRemovalTest, RemoveAllNodes)
{
  while (g.get_num_nodes() > 0)
    g.remove_node(g.get_first_node());

  EXPECT_EQ(g.get_num_nodes(), 0);
  EXPECT_EQ(g.get_num_arcs(), 0);
}

// =============================================================================
// Arc Removal Tests
// =============================================================================

class GraphArcRemovalTest : public Test
{
protected:
  Graph g;
  Graph::Node *n1, *n2, *n3;
  Graph::Arc *a1, *a2, *a3;

  void SetUp() override
  {
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    a1 = g.insert_arc(n1, n2, 1.0);
    a2 = g.insert_arc(n2, n3, 2.0);
    a3 = g.insert_arc(n1, n3, 3.0);
  }
};

TEST_F(GraphArcRemovalTest, RemoveSingleArc)
{
  g.remove_arc(a1);
  EXPECT_EQ(g.get_num_arcs(), 2);
  EXPECT_EQ(g.get_num_nodes(), 3);
}

TEST_F(GraphArcRemovalTest, RemoveAllArcs)
{
  g.remove_arc(a1);
  g.remove_arc(a2);
  g.remove_arc(a3);
  EXPECT_EQ(g.get_num_arcs(), 0);
  EXPECT_EQ(g.get_num_nodes(), 3);
}

TEST_F(GraphArcRemovalTest, DisconnectAndReconnectArc)
{
  g.disconnect_arc(a1);
  EXPECT_EQ(g.get_num_arcs(), 2);

  g.connect_arc(a1);
  EXPECT_EQ(g.get_num_arcs(), 3);
  // Note: do not delete a1 here - the graph destructor will handle it
}

// =============================================================================
// Iterator Tests
// =============================================================================

class GraphIteratorTest : public Test
{
protected:
  Graph g;
  vector<Graph::Node*> nodes;
  vector<Graph::Arc*> arcs;

  void SetUp() override
  {
    for (int i = 0; i < 5; ++i)
      nodes.push_back(g.insert_node(i * 10));

    arcs.push_back(g.insert_arc(nodes[0], nodes[1], 0.1));
    arcs.push_back(g.insert_arc(nodes[1], nodes[2], 0.2));
    arcs.push_back(g.insert_arc(nodes[2], nodes[3], 0.3));
    arcs.push_back(g.insert_arc(nodes[3], nodes[4], 0.4));
    arcs.push_back(g.insert_arc(nodes[4], nodes[0], 0.5));
  }
};

TEST_F(GraphIteratorTest, NodeIteratorTraversesAllNodes)
{
  size_t count = 0;
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, 5);
}

TEST_F(GraphIteratorTest, ArcIteratorTraversesAllArcs)
{
  size_t count = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, 5);
}

TEST_F(GraphIteratorTest, NodeArcIteratorTraversesAdjacentArcs)
{
  size_t count = 0;
  for (auto it = g.get_arc_it(nodes[0]); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, 2);
}

TEST_F(GraphIteratorTest, IteratorResetFirst)
{
  auto it = g.get_node_it();
  it.next();
  it.next();
  it.reset_first();
  EXPECT_TRUE(it.has_curr());
}

TEST_F(GraphIteratorTest, IteratorResetLast)
{
  auto it = g.get_node_it();
  it.reset_last();
  EXPECT_TRUE(it.has_curr());
}

// =============================================================================
// Filtered Iterator Tests
// =============================================================================

struct EvenNodeFilter
{
  bool operator()(Graph::Node *n) const noexcept
  {
    return n->get_info() % 20 == 0;
  }
};

struct HighWeightArcFilter
{
  bool operator()(Graph::Arc *a) const noexcept
  {
    return a->get_info() > 0.25;
  }
};

TEST_F(GraphIteratorTest, FilteredNodeIterator)
{
  size_t count = 0;
  for (Node_Iterator<Graph, EvenNodeFilter> it(g); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, 3);
}

TEST_F(GraphIteratorTest, FilteredArcIterator)
{
  size_t count = 0;
  for (Arc_Iterator<Graph, HighWeightArcFilter> it(g); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, 3);
}

// =============================================================================
// Search Tests
// =============================================================================

TEST_F(GraphIteratorTest, SearchArcBetweenNodes)
{
  auto arc = search_arc(g, nodes[0], nodes[1]);
  EXPECT_NE(arc, nullptr);
  EXPECT_EQ(arc->get_info(), 0.1);
}

TEST_F(GraphIteratorTest, SearchArcNotFound)
{
  auto arc = search_arc(g, nodes[0], nodes[2]);
  EXPECT_EQ(arc, nullptr);
}

TEST_F(GraphIteratorTest, SearchDirectedArc)
{
  auto arc = search_directed_arc(g, nodes[0], nodes[1]);
  EXPECT_NE(arc, nullptr);
}

// =============================================================================
// Graph Move Tests (Copy tests removed due to linker dependencies)
// =============================================================================

class GraphMoveTest : public Test
{
protected:
  Graph g;

  void SetUp() override
  {
    auto n1 = g.insert_node(1);
    auto n2 = g.insert_node(2);
    auto n3 = g.insert_node(3);
    g.insert_arc(n1, n2, 1.0);
    g.insert_arc(n2, n3, 2.0);
  }
};

TEST_F(GraphMoveTest, MoveConstructor)
{
  size_t orig_nodes = g.get_num_nodes();
  size_t orig_arcs = g.get_num_arcs();

  Graph moved(std::move(g));
  EXPECT_EQ(moved.get_num_nodes(), orig_nodes);
  EXPECT_EQ(moved.get_num_arcs(), orig_arcs);
  EXPECT_EQ(g.get_num_nodes(), 0);
  EXPECT_EQ(g.get_num_arcs(), 0);
}

TEST_F(GraphMoveTest, MoveAssignment)
{
  size_t orig_nodes = g.get_num_nodes();
  size_t orig_arcs = g.get_num_arcs();

  Graph moved;
  moved = std::move(g);
  EXPECT_EQ(moved.get_num_nodes(), orig_nodes);
  EXPECT_EQ(moved.get_num_arcs(), orig_arcs);
}

TEST_F(GraphMoveTest, SwapGraphs)
{
  Graph g2;
  auto n = g2.insert_node(100);
  g2.insert_arc(n, n, 0.5);

  size_t g1_nodes = g.get_num_nodes();
  size_t g2_nodes = g2.get_num_nodes();

  g.swap(g2);

  EXPECT_EQ(g.get_num_nodes(), g2_nodes);
  EXPECT_EQ(g2.get_num_nodes(), g1_nodes);
}

TEST_F(GraphMoveTest, SelfAssignment)
{
  size_t orig_nodes = g.get_num_nodes();
  size_t orig_arcs = g.get_num_arcs();

  g = g;  // Self-assignment should be safe

  EXPECT_EQ(g.get_num_nodes(), orig_nodes);
  EXPECT_EQ(g.get_num_arcs(), orig_arcs);
}

TEST_F(GraphMoveTest, CopyConstruction)
{
  Graph copy(g);

  EXPECT_EQ(copy.get_num_nodes(), g.get_num_nodes());
  EXPECT_EQ(copy.get_num_arcs(), g.get_num_arcs());

  // Verify deep copy: modifying copy doesn't affect original
  auto n = copy.insert_node(999);
  copy.insert_arc(n, n, 99.0);
  EXPECT_NE(copy.get_num_nodes(), g.get_num_nodes());
  EXPECT_NE(copy.get_num_arcs(), g.get_num_arcs());
}

TEST_F(GraphMoveTest, CopyAssignment)
{
  Graph copy;
  copy = g;

  EXPECT_EQ(copy.get_num_nodes(), g.get_num_nodes());
  EXPECT_EQ(copy.get_num_arcs(), g.get_num_arcs());
}

TEST_F(GraphMoveTest, CopyToNonEmptyGraph)
{
  // Create a non-empty target graph
  Graph target;
  for (int i = 0; i < 10; ++i)
    target.insert_node(i * 100);

  size_t orig_nodes = g.get_num_nodes();
  size_t orig_arcs = g.get_num_arcs();

  // Copy should replace contents, not append
  target = g;

  EXPECT_EQ(target.get_num_nodes(), orig_nodes);
  EXPECT_EQ(target.get_num_arcs(), orig_arcs);
}

// =============================================================================
// Path Tests
// =============================================================================

class PathTest : public Test
{
protected:
  Graph g;
  vector<Graph::Node*> nodes;
  vector<Graph::Arc*> arcs;

  void SetUp() override
  {
    for (int i = 0; i < 5; ++i)
      nodes.push_back(g.insert_node(i));

    arcs.push_back(g.insert_arc(nodes[0], nodes[1], 0.1));
    arcs.push_back(g.insert_arc(nodes[1], nodes[2], 0.2));
    arcs.push_back(g.insert_arc(nodes[2], nodes[3], 0.3));
    arcs.push_back(g.insert_arc(nodes[3], nodes[4], 0.4));
  }
};

TEST_F(PathTest, EmptyPathConstruction)
{
  Path<Graph> path(g);
  EXPECT_TRUE(path.is_empty());
  EXPECT_EQ(path.size(), 0);
}

TEST_F(PathTest, PathWithSingleNode)
{
  Path<Graph> path(g, nodes[0]);
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.size(), 1);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
  EXPECT_EQ(path.get_last_node(), nodes[0]);
}

TEST_F(PathTest, AppendArcToPath)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
  EXPECT_EQ(path.get_last_node(), nodes[1]);
}

TEST_F(PathTest, AppendNodeToPath)
{
  Path<Graph> path(g, nodes[0]);
  path.append(nodes[1]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_last_node(), nodes[1]);
}

TEST_F(PathTest, InsertArcToPath)
{
  Path<Graph> path(g, nodes[1]);
  path.insert(arcs[0]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
}

TEST_F(PathTest, InsertNodeToPath)
{
  Path<Graph> path(g, nodes[1]);
  path.insert(nodes[0]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
}

TEST_F(PathTest, BuildFullPath)
{
  Path<Graph> path(g, nodes[0]);
  for (size_t i = 0; i < arcs.size(); ++i)
    path.append(arcs[i]);

  EXPECT_EQ(path.size(), 5);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
  EXPECT_EQ(path.get_last_node(), nodes[4]);
}

TEST_F(PathTest, PathContainsNode)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  EXPECT_TRUE(path.contains_node(nodes[0]));
  EXPECT_TRUE(path.contains_node(nodes[1]));
  EXPECT_TRUE(path.contains_node(nodes[2]));
  EXPECT_FALSE(path.contains_node(nodes[3]));
}

TEST_F(PathTest, PathContainsArc)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  EXPECT_TRUE(path.contains_arc(arcs[0]));
  EXPECT_TRUE(path.contains_arc(arcs[1]));
  EXPECT_FALSE(path.contains_arc(arcs[2]));
}

TEST_F(PathTest, PathIterator)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  size_t node_count = 0;
  for (typename Path<Graph>::Iterator it(path); it.has_current_node(); it.next())
    ++node_count;

  EXPECT_EQ(node_count, 3);
}

TEST_F(PathTest, PathNodesList)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  auto node_list = path.nodes();
  EXPECT_EQ(node_list.size(), 3);
}

TEST_F(PathTest, PathArcsList)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  auto arc_list = path.arcs();
  EXPECT_EQ(arc_list.size(), 2);
}

TEST_F(PathTest, PathCopyConstruction)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  Path<Graph> copy(path);
  EXPECT_EQ(copy.size(), path.size());
}

TEST_F(PathTest, PathMoveConstruction)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  size_t orig_size = path.size();
  Path<Graph> moved(std::move(path));
  EXPECT_EQ(moved.size(), orig_size);
}

TEST_F(PathTest, RemoveLastNode)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  auto removed = path.remove_last_node();
  EXPECT_EQ(removed, nodes[2]);
  EXPECT_EQ(path.size(), 2);
}

TEST_F(PathTest, RemoveFirstNode)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  auto removed = path.remove_first_node();
  EXPECT_EQ(removed, nodes[0]);
  EXPECT_EQ(path.size(), 2);
}

TEST_F(PathTest, IsCycle)
{
  auto loop_arc = g.insert_arc(nodes[4], nodes[0], 0.5);

  Path<Graph> path(g, nodes[0]);
  for (auto a : arcs)
    path.append(a);
  path.append(loop_arc);

  EXPECT_TRUE(path.is_cycle());
}

TEST_F(PathTest, IsNotCycle)
{
  Path<Graph> path(g, nodes[0]);
  path.append(arcs[0]);
  path.append(arcs[1]);

  EXPECT_FALSE(path.is_cycle());
}

// =============================================================================
// Directed Path Tests
// =============================================================================

class DirectedPathTest : public Test
{
protected:
  TestDigraph dg;
  vector<TestDigraph::Node*> nodes;
  vector<TestDigraph::Arc*> arcs;

  void SetUp() override
  {
    for (int i = 0; i < 5; ++i)
      nodes.push_back(dg.insert_node(i));

    arcs.push_back(dg.insert_arc(nodes[0], nodes[1], 0.1));
    arcs.push_back(dg.insert_arc(nodes[1], nodes[2], 0.2));
    arcs.push_back(dg.insert_arc(nodes[2], nodes[3], 0.3));
    arcs.push_back(dg.insert_arc(nodes[3], nodes[4], 0.4));
  }
};

TEST_F(DirectedPathTest, AppendDirectedNode)
{
  Path<TestDigraph> path(dg, nodes[0]);
  path.append_directed(nodes[1]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_last_node(), nodes[1]);
}

TEST_F(DirectedPathTest, AppendDirectedArc)
{
  Path<TestDigraph> path(dg, nodes[0]);
  path.append_directed(arcs[0]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_last_node(), nodes[1]);
}

TEST_F(DirectedPathTest, InsertDirectedNode)
{
  Path<TestDigraph> path(dg, nodes[1]);
  path.insert_directed(nodes[0]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
}

TEST_F(DirectedPathTest, InsertDirectedArc)
{
  Path<TestDigraph> path(dg, nodes[1]);
  path.insert_directed(arcs[0]);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path.get_first_node(), nodes[0]);
}

// =============================================================================
// Exception Tests
// =============================================================================

class GraphExceptionTest : public Test
{
protected:
  Graph g;
};

TEST_F(GraphExceptionTest, GetFirstNodeOnEmptyGraphThrows)
{
  EXPECT_THROW(g.get_first_node(), std::range_error);
}

TEST_F(GraphExceptionTest, GetFirstArcOnEmptyGraphThrows)
{
  g.insert_node(1);
  EXPECT_THROW(g.get_first_arc(), std::range_error);
}

TEST_F(GraphExceptionTest, PathAppendOnEmptyPathThrows)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 1.0);

  Path<Graph> path(g);
  EXPECT_THROW(path.append(a), std::domain_error);
}

TEST_F(GraphExceptionTest, PathAppendInvalidArcThrows)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto a = g.insert_arc(n2, n3, 1.0);

  Path<Graph> path(g, n1);
  EXPECT_THROW(path.append(a), std::invalid_argument);
}

TEST_F(GraphExceptionTest, PathInsertOnEmptyPathThrows)
{
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 1.0);

  Path<Graph> path(g);
  EXPECT_THROW(path.insert(a), std::domain_error);
}

// =============================================================================
// Digraph-specific Tests
// =============================================================================

class DigraphTest : public Test
{
protected:
  TestDigraph dg;
  vector<TestDigraph::Node*> nodes;
  vector<TestDigraph::Arc*> arcs;

  void SetUp() override
  {
    for (int i = 0; i < 4; ++i)
      nodes.push_back(dg.insert_node(i));

    arcs.push_back(dg.insert_arc(nodes[0], nodes[1], 1.0));
    arcs.push_back(dg.insert_arc(nodes[1], nodes[2], 2.0));
    arcs.push_back(dg.insert_arc(nodes[2], nodes[3], 3.0));
    arcs.push_back(dg.insert_arc(nodes[0], nodes[2], 4.0));
  }
};

TEST_F(DigraphTest, NodeDegrees)
{
  // Count outgoing arcs manually
  size_t out_count_0 = 0;
  for (auto it = dg.get_arc_it(nodes[0]); it.has_curr(); it.next())
    if (dg.get_src_node(it.get_curr()) == nodes[0])
      ++out_count_0;
  EXPECT_EQ(out_count_0, 2);

  // Node 3 has no outgoing arcs
  size_t out_count_3 = 0;
  for (auto it = dg.get_arc_it(nodes[3]); it.has_curr(); it.next())
    if (dg.get_src_node(it.get_curr()) == nodes[3])
      ++out_count_3;
  EXPECT_EQ(out_count_3, 0);
}

TEST_F(DigraphTest, TraverseArcsFromNode)
{
  size_t count = 0;
  for (auto it = dg.get_arc_it(nodes[0]); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, 2);
}

TEST_F(DigraphTest, ArcDirection)
{
  // Verify arc direction
  EXPECT_EQ(dg.get_src_node(arcs[0]), nodes[0]);
  EXPECT_EQ(dg.get_tgt_node(arcs[0]), nodes[1]);
}

// =============================================================================
// Functional Operations Tests
// =============================================================================

class GraphFunctionalTest : public Test
{
protected:
  Graph g;
  vector<Graph::Node*> nodes;

  void SetUp() override
  {
    for (int i = 1; i <= 5; ++i)
      nodes.push_back(g.insert_node(i * 10));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[2], nodes[3], 3.0);
    g.insert_arc(nodes[3], nodes[4], 4.0);
  }
};

TEST_F(GraphFunctionalTest, ForEachNode)
{
  int sum = 0;
  for_each_node<Graph>(g, [&sum](Graph::Node *n) { sum += n->get_info(); });
  EXPECT_EQ(sum, 150);
}

TEST_F(GraphFunctionalTest, ForEachArc)
{
  double sum = 0;
  for_each_arc<Graph>(g, [&sum](Graph::Arc *a) { sum += a->get_info(); });
  EXPECT_EQ(sum, 10.0);
}

TEST_F(GraphFunctionalTest, ForallNode)
{
  bool all_positive = forall_node<Graph>(g, [](Graph::Node *n) {
    return n->get_info() > 0;
  });
  EXPECT_TRUE(all_positive);
}

TEST_F(GraphFunctionalTest, ForallArc)
{
  bool all_positive = forall_arc<Graph>(g, [](Graph::Arc *a) {
    return a->get_info() > 0;
  });
  EXPECT_TRUE(all_positive);
}

TEST_F(GraphFunctionalTest, FoldlNodes)
{
  int sum = foldl_nodes<Graph, int>(g, 0, [](const int & acc, Graph::Node *n) {
    return acc + n->get_info();
  });
  EXPECT_EQ(sum, 150);
}

TEST_F(GraphFunctionalTest, FoldlArcs)
{
  double sum = foldl_arcs<Graph, double>(g, 0.0, [](const double & acc, Graph::Arc *a) {
    return acc + a->get_info();
  });
  EXPECT_EQ(sum, 10.0);
}

// =============================================================================
// Find Path Tests
// =============================================================================

TEST_F(GraphFunctionalTest, FindPathDepthFirst)
{
  auto path = find_path_depth_first(g, nodes[0], nodes[4]);
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.get_first_node(), nodes[0]);
  EXPECT_EQ(path.get_last_node(), nodes[4]);
}

TEST_F(GraphFunctionalTest, FindPathDepthFirstNotFound)
{
  Graph g2;
  auto n1 = g2.insert_node(1);
  auto n2 = g2.insert_node(2);

  auto path = find_path_depth_first(g2, n1, n2);
  EXPECT_TRUE(path.is_empty());
}

// =============================================================================
// Stress Tests
// =============================================================================

class GraphStressTest : public Test
{
protected:
  Graph g;
  static constexpr size_t NUM_NODES = 1000;
  static constexpr size_t NUM_ARCS = 5000;
};

TEST_F(GraphStressTest, InsertManyNodes)
{
  for (size_t i = 0; i < NUM_NODES; ++i)
    g.insert_node(static_cast<int>(i));

  EXPECT_EQ(g.get_num_nodes(), NUM_NODES);
}

TEST_F(GraphStressTest, InsertManyArcs)
{
  vector<Graph::Node*> nodes;
  for (size_t i = 0; i < NUM_NODES; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  std::mt19937 rng(42);
  std::uniform_int_distribution<size_t> dist(0, NUM_NODES - 1);

  for (size_t i = 0; i < NUM_ARCS; ++i)
  {
    size_t src = dist(rng);
    size_t tgt = dist(rng);
    g.insert_arc(nodes[src], nodes[tgt], static_cast<double>(i));
  }

  EXPECT_EQ(g.get_num_arcs(), NUM_ARCS);
}

TEST_F(GraphStressTest, InsertAndRemoveManyNodes)
{
  vector<Graph::Node*> nodes;
  for (size_t i = 0; i < 100; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  EXPECT_EQ(g.get_num_nodes(), 100);

  // Remove half
  for (size_t i = 0; i < 50; ++i)
    g.remove_node(nodes[i]);

  EXPECT_EQ(g.get_num_nodes(), 50);
}

// =============================================================================
// Sorting Tests
// =============================================================================

TEST_F(GraphFunctionalTest, SortNodes)
{
  g.sort_nodes([](Graph::Node *a, Graph::Node *b) {
    return a->get_info() > b->get_info();
  });

  auto it = g.get_node_it();
  int prev = it.get_curr()->get_info();
  it.next();

  while (it.has_curr())
  {
    EXPECT_GE(prev, it.get_curr()->get_info());
    prev = it.get_curr()->get_info();
    it.next();
  }
}

TEST_F(GraphFunctionalTest, SortArcs)
{
  g.sort_arcs([](Graph::Arc *a, Graph::Arc *b) {
    return a->get_info() > b->get_info();
  });

  auto it = g.get_arc_it();
  double prev = it.get_curr()->get_info();
  it.next();

  while (it.has_curr())
  {
    EXPECT_GE(prev, it.get_curr()->get_info());
    prev = it.get_curr()->get_info();
    it.next();
  }
}

// =============================================================================
// Clear Graph Tests
// =============================================================================

TEST_F(GraphFunctionalTest, ClearGraph)
{
  clear_graph(g);
  EXPECT_EQ(g.get_num_nodes(), 0);
  EXPECT_EQ(g.get_num_arcs(), 0);
}

// =============================================================================
// C++20 Concepts Tests
// =============================================================================

// Compile-time verification that iterators satisfy concepts
static_assert(BasicGraphIterator<Graph::Node_Iterator>,
              "Node_Iterator must satisfy BasicGraphIterator");
static_assert(BasicGraphIterator<Graph::Arc_Iterator>,
              "Arc_Iterator must satisfy BasicGraphIterator");
static_assert(GraphNodeIterator<Graph::Node_Iterator, Graph::Node>,
              "Node_Iterator must satisfy GraphNodeIterator");
static_assert(GraphArcIterator<Graph::Arc_Iterator, Graph::Arc>,
              "Arc_Iterator must satisfy GraphArcIterator");

// Also verify for digraph
static_assert(BasicGraphIterator<TestDigraph::Node_Iterator>,
              "Digraph::Node_Iterator must satisfy BasicGraphIterator");
static_assert(BasicGraphIterator<TestDigraph::Arc_Iterator>,
              "Digraph::Arc_Iterator must satisfy BasicGraphIterator");

TEST(ConceptsTest, NodeIteratorSatisfiesConcept)
{
  Graph g;
  g.insert_node(1);
  g.insert_node(2);
  
  auto it = g.get_node_it();
  
  // Verify the interface works as expected by the concept
  EXPECT_TRUE(it.has_curr());
  EXPECT_NE(it.get_curr(), nullptr);
  it.next();
  EXPECT_TRUE(it.has_curr());
  it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST(ConceptsTest, ArcIteratorSatisfiesConcept)
{
  Graph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 1.0);
  
  auto it = g.get_arc_it();
  
  EXPECT_TRUE(it.has_curr());
  EXPECT_NE(it.get_curr(), nullptr);
  it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST(ConceptsTest, NodeArcIteratorSatisfiesConcept)
{
  Graph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 1.0);
  
  auto it = g.get_arc_it(n1);
  
  EXPECT_TRUE(it.has_curr());
  auto* arc = it.get_curr();
  EXPECT_NE(arc, nullptr);
  
  // Node arc iterator should provide get_tgt_node
  auto* tgt = it.get_tgt_node();
  EXPECT_EQ(tgt, n2);
}

// Test that concepts work with constrained template functions
template <BasicGraphIterator It>
size_t count_elements(It it)
{
  size_t count = 0;
  for (; it.has_curr(); it.next())
    ++count;
  return count;
}

TEST(ConceptsTest, ConceptConstrainedFunction)
{
  Graph g;
  g.insert_node(1);
  g.insert_node(2);
  g.insert_node(3);
  
  auto node_it = g.get_node_it();
  EXPECT_EQ(count_elements(node_it), 3u);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
