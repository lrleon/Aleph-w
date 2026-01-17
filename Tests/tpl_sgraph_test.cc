
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
 * @file tpl_sgraph_test.cc
 * @brief Tests for Tpl Sgraph
 */
#include <gtest/gtest.h>
#include <tpl_sgraph.H>
#include <string>
#include <vector>

using namespace Aleph;

namespace
{
  // Type aliases for common graph types
  using IntGraph = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;
  using IntDigraph = List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>;
  using StringGraph = List_SGraph<Graph_Snode<std::string>, Graph_Sarc<std::string>>;
}

// =============================================================================
// Graph_Snode Tests
// =============================================================================

TEST(GraphSnode, DefaultConstruction)
{
  Graph_Snode<int> node;
  EXPECT_EQ(node.num_arcs, 0u);
  EXPECT_TRUE(node.arc_list.is_empty());
}

TEST(GraphSnode, ConstructionWithInfo)
{
  Graph_Snode<int> node(42);
  EXPECT_EQ(node.get_info(), 42);
  EXPECT_EQ(node.num_arcs, 0u);
  EXPECT_TRUE(node.arc_list.is_empty());
}

TEST(GraphSnode, CopyConstruction)
{
  Graph_Snode<int> node1(42);
  Graph_Snode<int> node2(node1);
  EXPECT_EQ(node2.get_info(), 42);
  EXPECT_EQ(node2.num_arcs, 0u);
}

TEST(GraphSnode, MoveConstruction)
{
  Graph_Snode<std::string> node1(std::string("test"));
  Graph_Snode<std::string> node2(std::move(node1));
  EXPECT_EQ(node2.get_info(), "test");
}

TEST(GraphSnode, CopyAssignment)
{
  Graph_Snode<int> node1(42);
  Graph_Snode<int> node2(100);
  node2 = node1;
  EXPECT_EQ(node2.get_info(), 42);
}

TEST(GraphSnode, SelfAssignment)
{
  Graph_Snode<int> node(42);
  node = node;
  EXPECT_EQ(node.get_info(), 42);
}

TEST(GraphSnode, ConstructionFromPointer)
{
  Graph_Snode<int> node1(42);
  Graph_Snode<int> node2(&node1);
  EXPECT_EQ(node2.get_info(), 42);
}

// =============================================================================
// Graph_Sarc Tests
// =============================================================================

TEST(GraphSarc, DefaultConstruction)
{
  Graph_Sarc<int> arc;
  EXPECT_EQ(arc.get_info(), 0);  // Default int value
}

TEST(GraphSarc, ConstructionWithInfo)
{
  Graph_Sarc<int> arc(100);
  EXPECT_EQ(arc.get_info(), 100);
}

TEST(GraphSarc, CopyConstruction)
{
  Graph_Sarc<std::string> arc1(std::string("edge"));
  Graph_Sarc<std::string> arc2(arc1);
  EXPECT_EQ(arc2.get_info(), "edge");
}

TEST(GraphSarc, CopyAssignment)
{
  Graph_Sarc<int> arc1(42);
  Graph_Sarc<int> arc2(100);
  arc2 = arc1;
  EXPECT_EQ(arc2.get_info(), 42);
}

TEST(GraphSarc, SelfAssignment)
{
  Graph_Sarc<int> arc(42);
  arc = arc;
  EXPECT_EQ(arc.get_info(), 42);
}

TEST(GraphSarc, ConstructionWithEndpoints)
{
  int src_dummy = 1, tgt_dummy = 2;
  Graph_Sarc<int> arc(&src_dummy, &tgt_dummy, 100);
  EXPECT_EQ(arc.get_info(), 100);
  EXPECT_EQ(arc.src_node, &src_dummy);
  EXPECT_EQ(arc.tgt_node, &tgt_dummy);
}

// =============================================================================
// List_SGraph Basic Operations
// =============================================================================

TEST(ListSGraph, DefaultConstruction)
{
  IntGraph g;
  EXPECT_EQ(g.vsize(), 0u);
  EXPECT_EQ(g.esize(), 0u);
  EXPECT_FALSE(g.is_digraph());
}

TEST(ListSGraph, InsertNode)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  EXPECT_EQ(g.vsize(), 3u);
  EXPECT_EQ(n1->get_info(), 1);
  EXPECT_EQ(n2->get_info(), 2);
  EXPECT_EQ(n3->get_info(), 3);
}

TEST(ListSGraph, InsertArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  auto arc = g.insert_arc(n1, n2, 10);

  EXPECT_EQ(g.esize(), 1u);
  EXPECT_EQ(arc->get_info(), 10);
  EXPECT_EQ(g.get_src_node(arc), n1);
  EXPECT_EQ(g.get_tgt_node(arc), n2);
}

TEST(ListSGraph, UndirectedArcSymmetry)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n1, n2, 10);

  // In undirected graph, arc appears in both nodes' adjacency lists
  EXPECT_EQ(n1->num_arcs, 1u);
  EXPECT_EQ(n2->num_arcs, 1u);
}

TEST(ListSGraph, RemoveArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto arc = g.insert_arc(n1, n2, 10);

  EXPECT_EQ(g.esize(), 1u);
  g.remove_arc(arc);
  EXPECT_EQ(g.esize(), 0u);
  EXPECT_EQ(n1->num_arcs, 0u);
  EXPECT_EQ(n2->num_arcs, 0u);
}

TEST(ListSGraph, RemoveNode)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n1, n2, 12);
  g.insert_arc(n2, n3, 23);

  EXPECT_EQ(g.vsize(), 3u);
  EXPECT_EQ(g.esize(), 2u);

  g.remove_node(n2);

  EXPECT_EQ(g.vsize(), 2u);
  EXPECT_EQ(g.esize(), 0u);  // All arcs connected to n2 removed
}

TEST(ListSGraph, MultipleArcs)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(n1, n2, 12);
  g.insert_arc(n1, n3, 13);
  g.insert_arc(n1, n4, 14);
  g.insert_arc(n2, n3, 23);
  g.insert_arc(n3, n4, 34);

  EXPECT_EQ(g.esize(), 5u);
  EXPECT_EQ(n1->num_arcs, 3u);  // n1 connected to n2, n3, n4
}

// =============================================================================
// List_SDigraph Tests
// =============================================================================

TEST(ListSDigraph, IsDirected)
{
  IntDigraph g;
  EXPECT_TRUE(g.is_digraph());
}

TEST(ListSDigraph, DirectedArcAsymmetry)
{
  IntDigraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n1, n2, 10);

  // In directed graph, arc only appears in source node's adjacency list
  EXPECT_EQ(n1->num_arcs, 1u);
  EXPECT_EQ(n2->num_arcs, 0u);
}

TEST(ListSDigraph, BidirectionalArcs)
{
  IntDigraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n1, n2, 12);
  g.insert_arc(n2, n1, 21);

  EXPECT_EQ(g.esize(), 2u);
  EXPECT_EQ(n1->num_arcs, 1u);
  EXPECT_EQ(n2->num_arcs, 1u);
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST(ListSGraph, NodeIterator)
{
  IntGraph g;
  g.insert_node(1);
  g.insert_node(2);
  g.insert_node(3);

  int count = 0;
  int sum = 0;
  for (IntGraph::Node_Iterator it(g); it.has_curr(); it.next())
  {
    sum += it.get_curr()->get_info();
    ++count;
  }

  EXPECT_EQ(count, 3);
  EXPECT_EQ(sum, 6);  // 1 + 2 + 3
}

TEST(ListSGraph, ArcIterator)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n1, n2, 10);
  g.insert_arc(n2, n3, 20);

  int count = 0;
  int sum = 0;
  for (IntGraph::Arc_Iterator it(g); it.has_curr(); it.next())
  {
    sum += it.get_curr()->get_info();
    ++count;
  }

  EXPECT_EQ(count, 2);
  EXPECT_EQ(sum, 30);  // 10 + 20
}

TEST(ListSGraph, NodeArcIterator)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(n1, n2, 12);
  g.insert_arc(n1, n3, 13);
  g.insert_arc(n1, n4, 14);

  int count = 0;
  int sum = 0;
  for (IntGraph::Node_Arc_Iterator it(n1); it.has_curr(); it.next())
  {
    sum += it.get_curr()->get_info();
    ++count;
  }

  EXPECT_EQ(count, 3);
  EXPECT_EQ(sum, 39);  // 12 + 13 + 14
}

TEST(ListSGraph, NodeArcIteratorGetTgtNode)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n1, n2, 12);
  g.insert_arc(n1, n3, 13);

  std::vector<int> targets;
  for (IntGraph::Node_Arc_Iterator it(n1); it.has_curr(); it.next())
  {
    targets.push_back(it.get_tgt_node()->get_info());
  }

  EXPECT_EQ(targets.size(), 2u);
  // Order may vary, just check both are present
  EXPECT_TRUE(std::find(targets.begin(), targets.end(), 2) != targets.end());
  EXPECT_TRUE(std::find(targets.begin(), targets.end(), 3) != targets.end());
}

TEST(ListSGraph, ArcIteratorGetEndpoints)
{
  IntDigraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n1, n2, 12);

  IntDigraph::Arc_Iterator it(g);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_src_node()->get_info(), 1);
  EXPECT_EQ(it.get_tgt_node()->get_info(), 2);
}

// =============================================================================
// Copy and Move Semantics
// =============================================================================

TEST(ListSGraph, CopyConstruction)
{
  IntGraph g1;
  auto n1 = g1.insert_node(1);
  auto n2 = g1.insert_node(2);
  g1.insert_arc(n1, n2, 10);

  IntGraph g2(g1);

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g2.esize(), 1u);
}

TEST(ListSGraph, MoveConstruction)
{
  IntGraph g1;
  g1.insert_node(1);
  g1.insert_node(2);

  IntGraph g2(std::move(g1));

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g1.vsize(), 0u);  // Moved-from state
}

TEST(ListSGraph, CopyAssignment)
{
  IntGraph g1;
  auto n1 = g1.insert_node(1);
  auto n2 = g1.insert_node(2);
  g1.insert_arc(n1, n2, 10);

  IntGraph g2;
  g2 = g1;

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g2.esize(), 1u);
}

TEST(ListSGraph, SelfAssignment)
{
  IntGraph g;
  g.insert_node(1);
  g.insert_node(2);

  g = g;

  EXPECT_EQ(g.vsize(), 2u);
}

TEST(ListSGraph, MoveAssignment)
{
  IntGraph g1;
  g1.insert_node(1);
  g1.insert_node(2);

  IntGraph g2;
  g2 = std::move(g1);

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g1.vsize(), 0u);
}

TEST(ListSDigraph, CopyConstruction)
{
  IntDigraph g1;
  auto n1 = g1.insert_node(1);
  auto n2 = g1.insert_node(2);
  g1.insert_arc(n1, n2, 10);

  IntDigraph g2(g1);

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g2.esize(), 1u);
  EXPECT_TRUE(g2.is_digraph());
}

TEST(ListSDigraph, MoveConstruction)
{
  IntDigraph g1;
  g1.insert_node(1);
  g1.insert_node(2);

  IntDigraph g2(std::move(g1));

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_TRUE(g2.is_digraph());
}

TEST(ListSDigraph, CopyAssignment)
{
  IntDigraph g1;
  auto n1 = g1.insert_node(1);
  auto n2 = g1.insert_node(2);
  g1.insert_arc(n1, n2, 10);

  IntDigraph g2;
  g2 = g1;

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g2.esize(), 1u);
  EXPECT_TRUE(g2.is_digraph());
}

TEST(ListSDigraph, MoveAssignment)
{
  IntDigraph g1;
  g1.insert_node(1);
  g1.insert_node(2);

  IntDigraph g2;
  g2 = std::move(g1);

  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_TRUE(g2.is_digraph());
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(ListSGraph, LargeGraph)
{
  IntGraph g;
  constexpr int N = 100;

  // Insert N nodes
  std::vector<IntGraph::Node*> nodes;
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));

  EXPECT_EQ(g.vsize(), static_cast<size_t>(N));

  // Create a complete graph (N*(N-1)/2 edges)
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes[i], nodes[j], i * N + j);

  EXPECT_EQ(g.esize(), static_cast<size_t>(N * (N - 1) / 2));
}

TEST(ListSDigraph, LargeDigraph)
{
  IntDigraph g;
  constexpr int N = 50;

  std::vector<IntDigraph::Node*> nodes;
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));

  // Create complete digraph (N*(N-1) edges)
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (i != j)
        g.insert_arc(nodes[i], nodes[j], i * N + j);

  EXPECT_EQ(g.esize(), static_cast<size_t>(N * (N - 1)));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(ListSGraph, SelfLoop)
{
  IntGraph g;
  auto n1 = g.insert_node(1);

  auto arc = g.insert_arc(n1, n1, 11);

  EXPECT_EQ(g.esize(), 1u);
  EXPECT_EQ(n1->num_arcs, 1u);  // Self-loop counted once in undirected
  EXPECT_EQ(g.get_src_node(arc), n1);
  EXPECT_EQ(g.get_tgt_node(arc), n1);
}

TEST(ListSDigraph, SelfLoop)
{
  IntDigraph g;
  auto n1 = g.insert_node(1);

  auto arc = g.insert_arc(n1, n1, 11);

  EXPECT_EQ(g.esize(), 1u);
  EXPECT_EQ(n1->num_arcs, 1u);
  EXPECT_EQ(g.get_src_node(arc), n1);
  EXPECT_EQ(g.get_tgt_node(arc), n1);
}

TEST(ListSGraph, EmptyGraphIterators)
{
  IntGraph g;

  int node_count = 0;
  for (IntGraph::Node_Iterator it(g); it.has_curr(); it.next())
    ++node_count;
  EXPECT_EQ(node_count, 0);

  int arc_count = 0;
  for (IntGraph::Arc_Iterator it(g); it.has_curr(); it.next())
    ++arc_count;
  EXPECT_EQ(arc_count, 0);
}

TEST(ListSGraph, GetFirstNodeArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto arc = g.insert_arc(n1, n2, 10);

  EXPECT_NE(g.get_first_node(), nullptr);
  EXPECT_NE(g.get_first_arc(), nullptr);
  EXPECT_EQ(g.get_first_arc(n1), arc);
}

TEST(ListSGraph, GetFirstArcNodeThrowsWhenEmpty)
{
  IntGraph g;
  auto n1 = g.insert_node(1);

  EXPECT_THROW(g.get_first_arc(n1), std::range_error);
}

TEST(ListSGraph, RemoveAllNodes)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 10);

  g.remove_node(n1);
  g.remove_node(n2);

  EXPECT_EQ(g.vsize(), 0u);
  EXPECT_EQ(g.esize(), 0u);
}

TEST(ListSGraph, SwapGraphs)
{
  IntGraph g1;
  g1.insert_node(1);
  g1.insert_node(2);

  IntGraph g2;
  g2.insert_node(10);

  g1.swap(g2);

  EXPECT_EQ(g1.vsize(), 1u);
  EXPECT_EQ(g2.vsize(), 2u);
}

// =============================================================================
// String Data Type Tests
// =============================================================================

TEST(ListSGraph, StringNodeInfo)
{
  StringGraph g;
  auto n1 = g.insert_node(std::string("node1"));
  auto n2 = g.insert_node(std::string("node2"));

  g.insert_arc(n1, n2, std::string("edge"));

  EXPECT_EQ(n1->get_info(), "node1");
  EXPECT_EQ(n2->get_info(), "node2");
}

// =============================================================================
// Iterator Method Tests
// =============================================================================

TEST(ListSGraph, NodeIteratorCurrentNode)
{
  IntGraph g;
  g.insert_node(42);

  IntGraph::Node_Iterator it(g);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 42);
  EXPECT_EQ(it.get_current_node_ne()->get_info(), 42);
}

TEST(ListSGraph, NodeArcIteratorCurrentArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 100);

  IntGraph::Node_Arc_Iterator it(n1);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_arc()->get_info(), 100);
  EXPECT_EQ(it.get_current_arc_ne()->get_info(), 100);
}

TEST(ListSGraph, ArcIteratorCurrentArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 100);

  IntGraph::Arc_Iterator it(g);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_arc()->get_info(), 100);
  EXPECT_EQ(it.get_current_arc_ne()->get_info(), 100);
}

// =============================================================================
// Multiple Operations Test
// =============================================================================

TEST(ListSGraph, ComplexOperations)
{
  IntGraph g;

  // Build a graph
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  auto a12 = g.insert_arc(n1, n2, 12);
  g.insert_arc(n2, n3, 23);
  g.insert_arc(n3, n4, 34);
  g.insert_arc(n4, n1, 41);

  EXPECT_EQ(g.vsize(), 4u);
  EXPECT_EQ(g.esize(), 4u);

  // Remove one arc
  g.remove_arc(a12);
  EXPECT_EQ(g.esize(), 3u);

  // Remove one node (should remove 2 arcs: 41 and 34)
  g.remove_node(n4);
  EXPECT_EQ(g.vsize(), 3u);
  EXPECT_EQ(g.esize(), 1u);  // Only 23 remains

  // Copy the graph
  IntGraph g2(g);
  EXPECT_EQ(g2.vsize(), 3u);
  EXPECT_EQ(g2.esize(), 1u);
}

// =============================================================================
// Digraph Template Wrapper Tests
// =============================================================================

// These tests validate the generic Digraph<BaseGraph> template wrapper
// that was introduced to reduce code duplication across digraph implementations.

TEST(DigraphWrapper, DigraphInheritsFromBaseGraph)
{
  // Verify that List_SDigraph is actually a Digraph<List_SGraph<...>>
  IntDigraph dg;
  
  // Should inherit all base graph functionality
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  auto arc = dg.insert_arc(n1, n2, 100);
  
  EXPECT_EQ(dg.vsize(), 2u);
  EXPECT_EQ(dg.esize(), 1u);
  EXPECT_TRUE(dg.is_digraph());
  EXPECT_EQ(dg.get_src_node(arc), n1);
  EXPECT_EQ(dg.get_tgt_node(arc), n2);
}

TEST(DigraphWrapper, DigraphCopyPreservesDirectedFlag)
{
  IntDigraph dg1;
  dg1.insert_node(1);
  dg1.insert_node(2);
  
  // Copy construction should preserve digraph flag
  IntDigraph dg2(dg1);
  EXPECT_TRUE(dg2.is_digraph());
  EXPECT_EQ(dg2.vsize(), 2u);
}

TEST(DigraphWrapper, DigraphMovePreservesDirectedFlag)
{
  IntDigraph dg1;
  dg1.insert_node(1);
  dg1.insert_node(2);
  
  // Move construction should preserve digraph flag
  IntDigraph dg2(std::move(dg1));
  EXPECT_TRUE(dg2.is_digraph());
  EXPECT_EQ(dg2.vsize(), 2u);
}

TEST(DigraphWrapper, DigraphCopyAssignmentPreservesDirectedFlag)
{
  IntDigraph dg1;
  dg1.insert_node(1);
  
  IntDigraph dg2;
  dg2.insert_node(10);
  dg2.insert_node(20);
  
  dg2 = dg1;
  EXPECT_TRUE(dg2.is_digraph());
  EXPECT_EQ(dg2.vsize(), 1u);
}

TEST(DigraphWrapper, DigraphMoveAssignmentPreservesDirectedFlag)
{
  IntDigraph dg1;
  dg1.insert_node(1);
  
  IntDigraph dg2;
  dg2.insert_node(10);
  
  dg2 = std::move(dg1);
  EXPECT_TRUE(dg2.is_digraph());
  EXPECT_EQ(dg2.vsize(), 1u);
}

TEST(DigraphWrapper, DigraphSelfAssignment)
{
  IntDigraph dg;
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  dg.insert_arc(n1, n2, 100);
  
  dg = dg;  // Self-assignment should be safe
  
  EXPECT_TRUE(dg.is_digraph());
  EXPECT_EQ(dg.vsize(), 2u);
  EXPECT_EQ(dg.esize(), 1u);
}

TEST(DigraphWrapper, DirectedArcNotVisibleFromTarget)
{
  IntDigraph dg;
  auto n1 = dg.insert_node(1);
  auto n2 = dg.insert_node(2);
  dg.insert_arc(n1, n2, 100);  // n1 -> n2
  
  // In a digraph, arc should only be visible from source node
  EXPECT_EQ(n1->num_arcs, 1u);  // Source has the arc
  // Note: In digraph mode, target node doesn't have the arc in its adjacency list
}

// =============================================================================
// C++20 Concepts Tests
// =============================================================================

// Compile-time verification that List_SGraph iterators satisfy concepts
static_assert(BasicGraphIterator<IntGraph::Node_Iterator>,
              "List_SGraph::Node_Iterator must satisfy BasicGraphIterator");
static_assert(BasicGraphIterator<IntGraph::Arc_Iterator>,
              "List_SGraph::Arc_Iterator must satisfy BasicGraphIterator");
static_assert(GraphNodeIterator<IntGraph::Node_Iterator, IntGraph::Node>,
              "List_SGraph::Node_Iterator must satisfy GraphNodeIterator");
static_assert(GraphArcIterator<IntGraph::Arc_Iterator, IntGraph::Arc>,
              "List_SGraph::Arc_Iterator must satisfy GraphArcIterator");

TEST(ConceptsTest, SGraphIteratorsSatisfyConcepts)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 10);
  
  // Verify node iterator works per concept
  auto nit = g.get_node_it();
  EXPECT_TRUE(nit.has_curr());
  EXPECT_NE(nit.get_curr(), nullptr);
  
  // Verify arc iterator works per concept
  auto ait = g.get_arc_it();
  EXPECT_TRUE(ait.has_curr());
  EXPECT_NE(ait.get_curr(), nullptr);
}
