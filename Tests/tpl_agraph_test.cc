/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

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

#include <gtest/gtest.h>
#include <tpl_agraph.H>
#include <string>

using namespace Aleph;

namespace
{
  // Type aliases for common graph types
  using IntGraph = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;
  using IntDigraph = Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>;
  using StringGraph = Array_Graph<Graph_Anode<std::string>, Graph_Aarc<std::string>>;
}

// =============================================================================
// Graph_Anode Tests
// =============================================================================

TEST(GraphAnode, DefaultConstruction)
{
  Graph_Anode<int> node;
  EXPECT_EQ(node.num_arcs, 0u);
  EXPECT_EQ(node.arcs_dim, 0u);
  EXPECT_EQ(node.arc_array, nullptr);
}

TEST(GraphAnode, ConstructionWithInfo)
{
  Graph_Anode<int> node(42);
  EXPECT_EQ(node.get_info(), 42);
  EXPECT_EQ(node.num_arcs, 0u);
  EXPECT_GT(node.arcs_dim, 0u);  // Should have default capacity
}

TEST(GraphAnode, CopyConstruction)
{
  Graph_Anode<int> node1(42);
  Graph_Anode<int> node2(node1);
  EXPECT_EQ(node2.get_info(), 42);
  EXPECT_EQ(node2.num_arcs, 0u);
}

TEST(GraphAnode, MoveConstruction)
{
  Graph_Anode<std::string> node1(std::string("test"));
  Graph_Anode<std::string> node2(std::move(node1));
  EXPECT_EQ(node2.get_info(), "test");
}

// =============================================================================
// Graph_Aarc Tests
// =============================================================================

TEST(GraphAarc, DefaultConstruction)
{
  Graph_Aarc<int> arc;
  EXPECT_EQ(arc.get_info(), 0);  // Default int value
}

TEST(GraphAarc, ConstructionWithInfo)
{
  Graph_Aarc<int> arc(100);
  EXPECT_EQ(arc.get_info(), 100);
}

TEST(GraphAarc, CopyConstruction)
{
  Graph_Aarc<std::string> arc1(std::string("edge"));
  Graph_Aarc<std::string> arc2(arc1);
  EXPECT_EQ(arc2.get_info(), "edge");
}

// =============================================================================
// Array_Graph Basic Operations
// =============================================================================

TEST(ArrayGraph, DefaultConstruction)
{
  IntGraph g;
  EXPECT_EQ(g.vsize(), 0u);
  EXPECT_EQ(g.esize(), 0u);
  EXPECT_FALSE(g.is_digraph());
}

TEST(ArrayGraph, InsertNode)
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

TEST(ArrayGraph, InsertArc)
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

TEST(ArrayGraph, UndirectedArcSymmetry)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2, 10);
  
  // In undirected graph, arc appears in both nodes' adjacency lists
  EXPECT_EQ(n1->num_arcs, 1u);
  EXPECT_EQ(n2->num_arcs, 1u);
}

TEST(ArrayGraph, RemoveArc)
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

TEST(ArrayGraph, RemoveNode)
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

TEST(ArrayGraph, MultipleArcs)
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
// Array_Digraph Tests
// =============================================================================

TEST(ArrayDigraph, IsDirected)
{
  IntDigraph g;
  EXPECT_TRUE(g.is_digraph());
}

TEST(ArrayDigraph, DirectedArcAsymmetry)
{
  IntDigraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2, 10);
  
  // In directed graph, arc only appears in source node's adjacency list
  EXPECT_EQ(n1->num_arcs, 1u);
  EXPECT_EQ(n2->num_arcs, 0u);
}

TEST(ArrayDigraph, BidirectionalArcs)
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

TEST(ArrayGraph, NodeIterator)
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

TEST(ArrayGraph, ArcIterator)
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

TEST(ArrayGraph, NodeArcIterator)
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

// =============================================================================
// Copy and Move Semantics
// =============================================================================

TEST(ArrayGraph, CopyConstruction)
{
  IntGraph g1;
  auto n1 = g1.insert_node(1);
  auto n2 = g1.insert_node(2);
  g1.insert_arc(n1, n2, 10);
  
  IntGraph g2(g1);
  
  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g2.esize(), 1u);
}

TEST(ArrayGraph, MoveConstruction)
{
  IntGraph g1;
  g1.insert_node(1);
  g1.insert_node(2);
  
  IntGraph g2(std::move(g1));
  
  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g1.vsize(), 0u);  // Moved-from state
}

TEST(ArrayGraph, CopyAssignment)
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

TEST(ArrayGraph, MoveAssignment)
{
  IntGraph g1;
  g1.insert_node(1);
  g1.insert_node(2);
  
  IntGraph g2;
  g2 = std::move(g1);
  
  EXPECT_EQ(g2.vsize(), 2u);
  EXPECT_EQ(g1.vsize(), 0u);
}

// =============================================================================
// Sorting Tests
// =============================================================================

TEST(ArrayGraph, SortNodes)
{
  IntGraph g;
  g.insert_node(3);
  g.insert_node(1);
  g.insert_node(2);
  
  g.sort_nodes([](auto *a, auto *b) { 
    return a->get_info() < b->get_info(); 
  });
  
  std::vector<int> values;
  for (IntGraph::Node_Iterator it(g); it.has_curr(); it.next())
    values.push_back(it.get_curr()->get_info());
  
  EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST(ArrayGraph, SortArcs)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  g.insert_arc(n1, n2, 30);
  g.insert_arc(n1, n2, 10);
  g.insert_arc(n1, n2, 20);
  
  g.sort_arcs([](auto *a, auto *b) { 
    return a->get_info() < b->get_info(); 
  });
  
  std::vector<int> values;
  for (IntGraph::Arc_Iterator it(g); it.has_curr(); it.next())
    values.push_back(it.get_curr()->get_info());
  
  EXPECT_EQ(values, (std::vector<int>{10, 20, 30}));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(ArrayGraph, LargeGraph)
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

TEST(ArrayDigraph, LargeDigraph)
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

TEST(ArrayGraph, SelfLoop)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  
  auto arc = g.insert_arc(n1, n1, 11);
  
  EXPECT_EQ(g.esize(), 1u);
  EXPECT_EQ(n1->num_arcs, 1u);  // Self-loop counted once
  EXPECT_EQ(g.get_src_node(arc), n1);
  EXPECT_EQ(g.get_tgt_node(arc), n1);
}

TEST(ArrayGraph, DisconnectReconnectArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  
  auto arc = g.insert_arc(n1, n2, 10);
  EXPECT_EQ(g.esize(), 1u);
  
  g.disconnect_arc(arc);
  EXPECT_EQ(g.esize(), 0u);
  EXPECT_EQ(n1->num_arcs, 0u);
  
  g.connect_arc(arc);
  EXPECT_EQ(g.esize(), 1u);
  EXPECT_EQ(n1->num_arcs, 1u);
  
  // Clean up the arc we disconnected/reconnected
  g.remove_arc(arc);
}

TEST(ArrayGraph, EmptyGraphIterators)
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

TEST(ArrayGraph, GetFirstNodeArc)
{
  IntGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto arc = g.insert_arc(n1, n2, 10);
  
  EXPECT_NE(g.get_first_node(), nullptr);
  EXPECT_NE(g.get_first_arc(), nullptr);
  EXPECT_EQ(g.get_first_arc(n1), arc);
}

// =============================================================================
// String Data Type Tests
// =============================================================================

TEST(ArrayGraph, StringNodeInfo)
{
  StringGraph g;
  auto n1 = g.insert_node(std::string("node1"));
  auto n2 = g.insert_node(std::string("node2"));
  
  g.insert_arc(n1, n2, std::string("edge"));
  
  EXPECT_EQ(n1->get_info(), "node1");
  EXPECT_EQ(n2->get_info(), "node2");
}
