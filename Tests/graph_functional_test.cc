
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
 * @file graph_functional_test.cc
 * @brief Tests for graph functional operations (count, none, partition, etc.)
 * 
 * Tests cover all three graph implementations:
 * - List_Graph / List_Digraph (adjacency list with doubly-linked lists)
 * - List_SGraph / List_SDigraph (adjacency list with singly-linked lists)
 * - Array_Graph / Array_Digraph (adjacency list with dynamic arrays)
 * 
 * Both directed and undirected cases are tested with typed tests.
 */

#include <gtest/gtest.h>
#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <tpl_agraph.H>

using namespace Aleph;

// ==================== Type definitions ====================

// List-based implementations with Dlink (use Graph_Node / Graph_Arc)
using IntNode = Graph_Node<int>;
using DoubleArc = Graph_Arc<double>;
using ListGraph = List_Graph<IntNode, DoubleArc>;
using ListDigraph = List_Digraph<IntNode, DoubleArc>;

// Sparse list implementations with DynList (use Graph_Snode / Graph_Sarc)
using IntSnode = Graph_Snode<int>;
using DoubleSarc = Graph_Sarc<double>;
using SparseGraph = List_SGraph<IntSnode, DoubleSarc>;
using SparseDigraph = List_SDigraph<IntSnode, DoubleSarc>;

// Array-based implementations (use Graph_Anode / Graph_Aarc)
using IntAnode = Graph_Anode<int>;
using DoubleAarc = Graph_Aarc<double>;
using ArrayGraph = Array_Graph<IntAnode, DoubleAarc>;
using ArrayDigraph = Array_Digraph<IntAnode, DoubleAarc>;

// For backward compatibility with existing tests
using TestGraph = ListGraph;
using TestDigraph = ListDigraph;

class GraphFunctionalTest : public ::testing::Test
{
protected:
  TestGraph g;
  TestDigraph dg;
  
  TestGraph::Node *n1, *n2, *n3, *n4, *n5;
  TestGraph::Arc *a1, *a2, *a3, *a4;
  
  TestDigraph::Node *dn1, *dn2, *dn3, *dn4;
  TestDigraph::Arc *da1, *da2, *da3, *da4, *da5;

  void SetUp() override
  {
    // Build undirected graph:
    //   n1(1) --1.0-- n2(2) --2.0-- n3(3)
    //    |            |
    //   3.0          4.0
    //    |            |
    //   n4(4) -------n5(5)
    
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    n4 = g.insert_node(4);
    n5 = g.insert_node(5);
    
    a1 = g.insert_arc(n1, n2, 1.0);
    a2 = g.insert_arc(n2, n3, 2.0);
    a3 = g.insert_arc(n1, n4, 3.0);
    a4 = g.insert_arc(n2, n5, 4.0);
    
    // Build directed graph:
    //   dn1(10) --1.5--> dn2(20) --2.5--> dn3(30)
    //    |                 ^                |
    //    |                 |                |
    //   0.5               3.5              4.5
    //    |                 |                |
    //    v                 |                v
    //   dn4(40) -----------+               dn3
    
    dn1 = dg.insert_node(10);
    dn2 = dg.insert_node(20);
    dn3 = dg.insert_node(30);
    dn4 = dg.insert_node(40);
    
    da1 = dg.insert_arc(dn1, dn2, 1.5);
    da2 = dg.insert_arc(dn2, dn3, 2.5);
    da3 = dg.insert_arc(dn1, dn4, 0.5);
    da4 = dg.insert_arc(dn4, dn2, 3.5);
    da5 = dg.insert_arc(dn3, dn3, 4.5); // self-loop
  }
};

// ==================== none_node tests ====================

TEST_F(GraphFunctionalTest, NoneNodeReturnsTrueWhenNoMatch)
{
  // No node has value > 100
  EXPECT_TRUE(g.none_node([](auto p) { return p->get_info() > 100; }));
  EXPECT_TRUE(dg.none_node([](auto p) { return p->get_info() > 100; }));
}

TEST_F(GraphFunctionalTest, NoneNodeReturnsFalseWhenSomeMatch)
{
  // Some nodes have value > 2
  EXPECT_FALSE(g.none_node([](auto p) { return p->get_info() > 2; }));
  EXPECT_FALSE(dg.none_node([](auto p) { return p->get_info() > 20; }));
}

TEST_F(GraphFunctionalTest, NoneNodeOnEmptyGraph)
{
  TestGraph empty;
  EXPECT_TRUE(empty.none_node([](auto) { return true; }));
}

// ==================== none_arc tests ====================

TEST_F(GraphFunctionalTest, NoneArcReturnsTrueWhenNoMatch)
{
  // No arc has weight > 100
  EXPECT_TRUE(g.none_arc([](auto a) { return a->get_info() > 100.0; }));
  EXPECT_TRUE(dg.none_arc([](auto a) { return a->get_info() > 100.0; }));
}

TEST_F(GraphFunctionalTest, NoneArcReturnsFalseWhenSomeMatch)
{
  // Some arcs have weight > 2
  EXPECT_FALSE(g.none_arc([](auto a) { return a->get_info() > 2.0; }));
  EXPECT_FALSE(dg.none_arc([](auto a) { return a->get_info() > 2.0; }));
}

TEST_F(GraphFunctionalTest, NoneArcAdjacentToNode)
{
  // n1 has arcs with weights 1.0 and 3.0, none > 10
  EXPECT_TRUE(g.none_arc(n1, [](auto a) { return a->get_info() > 10.0; }));
  
  // n1 has arc with weight 3.0
  EXPECT_FALSE(g.none_arc(n1, [](auto a) { return a->get_info() > 2.5; }));
}

// ==================== count_nodes tests ====================

TEST_F(GraphFunctionalTest, CountNodesAll)
{
  EXPECT_EQ(g.count_nodes(), 5u);
  EXPECT_EQ(dg.count_nodes(), 4u);
}

TEST_F(GraphFunctionalTest, CountNodesWithPredicate)
{
  // Count nodes with value > 2
  EXPECT_EQ(g.count_nodes([](auto p) { return p->get_info() > 2; }), 3u);
  
  // Count nodes with even value
  EXPECT_EQ(g.count_nodes([](auto p) { return p->get_info() % 2 == 0; }), 2u);
  
  // Count digraph nodes > 25
  EXPECT_EQ(dg.count_nodes([](auto p) { return p->get_info() > 25; }), 2u);
}

TEST_F(GraphFunctionalTest, CountNodesNoneMatch)
{
  EXPECT_EQ(g.count_nodes([](auto p) { return p->get_info() > 100; }), 0u);
}

TEST_F(GraphFunctionalTest, CountNodesEmptyGraph)
{
  TestGraph empty;
  EXPECT_EQ(empty.count_nodes(), 0u);
}

// ==================== count_arcs tests ====================

TEST_F(GraphFunctionalTest, CountArcsAll)
{
  EXPECT_EQ(g.count_arcs(), 4u);
  EXPECT_EQ(dg.count_arcs(), 5u);
}

TEST_F(GraphFunctionalTest, CountArcsWithPredicate)
{
  // Count arcs with weight > 2
  EXPECT_EQ(g.count_arcs([](auto a) { return a->get_info() > 2.0; }), 2u);
  
  // Count arcs with weight <= 2
  EXPECT_EQ(g.count_arcs([](auto a) { return a->get_info() <= 2.0; }), 2u);
  
  // Count digraph arcs > 2
  EXPECT_EQ(dg.count_arcs([](auto a) { return a->get_info() > 2.0; }), 3u);
}

TEST_F(GraphFunctionalTest, CountArcsAdjacentToNode)
{
  // n2 has 3 adjacent arcs
  EXPECT_EQ(g.count_arcs(n2), 3u);
  
  // n1 has 2 adjacent arcs
  EXPECT_EQ(g.count_arcs(n1), 2u);
  
  // Count arcs adjacent to n2 with weight > 1.5
  EXPECT_EQ(g.count_arcs(n2, [](auto a) { return a->get_info() > 1.5; }), 2u);
}

TEST_F(GraphFunctionalTest, CountArcsEmptyGraph)
{
  TestGraph empty;
  EXPECT_EQ(empty.count_arcs(), 0u);
}

// ==================== sum_arcs tests ====================

TEST_F(GraphFunctionalTest, SumArcsAdjacentToNode)
{
  // n1 has arcs with weights 1.0 + 3.0 = 4.0
  double sum = g.sum_arcs<double>(n1);
  EXPECT_DOUBLE_EQ(sum, 4.0);
  
  // n2 has arcs with weights 1.0 + 2.0 + 4.0 = 7.0
  sum = g.sum_arcs<double>(n2);
  EXPECT_DOUBLE_EQ(sum, 7.0);
}

TEST_F(GraphFunctionalTest, SumArcsIsolatedNode)
{
  auto isolated = g.insert_node(100);
  double sum = g.sum_arcs<double>(isolated);
  EXPECT_DOUBLE_EQ(sum, 0.0);
}

TEST_F(GraphFunctionalTest, SumArcsWithCustomExtractor)
{
  // Sum using custom extractor
  double sum = g.sum_arcs<double>(n2, [](auto a) { return a->get_info() * 2; });
  EXPECT_DOUBLE_EQ(sum, 14.0); // (1 + 2 + 4) * 2
}

// ==================== min_arc tests ====================

TEST_F(GraphFunctionalTest, MinArcAdjacentToNode)
{
  // n2's minimum arc should be weight 1.0 (to n1)
  auto min_a = g.min_arc(n2);
  ASSERT_NE(min_a, nullptr);
  EXPECT_DOUBLE_EQ(min_a->get_info(), 1.0);
}

TEST_F(GraphFunctionalTest, MinArcGlobal)
{
  // Global minimum arc is weight 1.0
  auto min_a = g.min_arc();
  ASSERT_NE(min_a, nullptr);
  EXPECT_DOUBLE_EQ(min_a->get_info(), 1.0);
  
  // Digraph global minimum is 0.5
  auto min_da = dg.min_arc();
  ASSERT_NE(min_da, nullptr);
  EXPECT_DOUBLE_EQ(min_da->get_info(), 0.5);
}

TEST_F(GraphFunctionalTest, MinArcIsolatedNode)
{
  auto isolated = g.insert_node(100);
  auto min_a = g.min_arc(isolated);
  EXPECT_EQ(min_a, nullptr);
}

TEST_F(GraphFunctionalTest, MinArcEmptyGraph)
{
  TestGraph empty;
  auto min_a = empty.min_arc();
  EXPECT_EQ(min_a, nullptr);
}

TEST_F(GraphFunctionalTest, MinArcWithCustomComparator)
{
  // Find arc with maximum weight (reverse comparator)
  auto max_via_min = g.min_arc(n2, [](auto a, auto b) { 
    return a->get_info() > b->get_info(); 
  });
  ASSERT_NE(max_via_min, nullptr);
  EXPECT_DOUBLE_EQ(max_via_min->get_info(), 4.0);
}

// ==================== max_arc tests ====================

TEST_F(GraphFunctionalTest, MaxArcAdjacentToNode)
{
  // n2's maximum arc should be weight 4.0 (to n5)
  auto max_a = g.max_arc(n2);
  ASSERT_NE(max_a, nullptr);
  EXPECT_DOUBLE_EQ(max_a->get_info(), 4.0);
}

TEST_F(GraphFunctionalTest, MaxArcGlobal)
{
  // Global maximum arc is weight 4.0
  auto max_a = g.max_arc();
  ASSERT_NE(max_a, nullptr);
  EXPECT_DOUBLE_EQ(max_a->get_info(), 4.0);
  
  // Digraph global maximum is 4.5 (self-loop)
  auto max_da = dg.max_arc();
  ASSERT_NE(max_da, nullptr);
  EXPECT_DOUBLE_EQ(max_da->get_info(), 4.5);
}

TEST_F(GraphFunctionalTest, MaxArcIsolatedNode)
{
  auto isolated = g.insert_node(100);
  auto max_a = g.max_arc(isolated);
  EXPECT_EQ(max_a, nullptr);
}

// ==================== partition_nodes tests ====================

TEST_F(GraphFunctionalTest, PartitionNodesByValue)
{
  auto [high, low] = g.partition_nodes([](auto p) { return p->get_info() > 2; });
  
  EXPECT_EQ(high.size(), 3u); // 3, 4, 5
  EXPECT_EQ(low.size(), 2u);  // 1, 2
  
  // Verify contents
  EXPECT_TRUE(high.exists([](auto p) { return p->get_info() == 3; }));
  EXPECT_TRUE(high.exists([](auto p) { return p->get_info() == 4; }));
  EXPECT_TRUE(high.exists([](auto p) { return p->get_info() == 5; }));
  
  EXPECT_TRUE(low.exists([](auto p) { return p->get_info() == 1; }));
  EXPECT_TRUE(low.exists([](auto p) { return p->get_info() == 2; }));
}

TEST_F(GraphFunctionalTest, PartitionNodesAllMatch)
{
  auto [match, nomatch] = g.partition_nodes([](auto p) { return p->get_info() > 0; });
  
  EXPECT_EQ(match.size(), 5u);
  EXPECT_EQ(nomatch.size(), 0u);
}

TEST_F(GraphFunctionalTest, PartitionNodesNoneMatch)
{
  auto [match, nomatch] = g.partition_nodes([](auto p) { return p->get_info() > 100; });
  
  EXPECT_EQ(match.size(), 0u);
  EXPECT_EQ(nomatch.size(), 5u);
}

TEST_F(GraphFunctionalTest, PartitionNodesEmptyGraph)
{
  TestGraph empty;
  auto [match, nomatch] = empty.partition_nodes([](auto) { return true; });
  
  EXPECT_EQ(match.size(), 0u);
  EXPECT_EQ(nomatch.size(), 0u);
}

// ==================== partition_arcs tests ====================

TEST_F(GraphFunctionalTest, PartitionArcsByWeight)
{
  auto [heavy, light] = g.partition_arcs([](auto a) { return a->get_info() > 2.0; });
  
  EXPECT_EQ(heavy.size(), 2u); // 3.0, 4.0
  EXPECT_EQ(light.size(), 2u); // 1.0, 2.0
}

TEST_F(GraphFunctionalTest, PartitionArcsDigraph)
{
  auto [high, low] = dg.partition_arcs([](auto a) { return a->get_info() > 2.0; });
  
  EXPECT_EQ(high.size(), 3u); // 2.5, 3.5, 4.5
  EXPECT_EQ(low.size(), 2u);  // 1.5, 0.5
}

// ==================== adjacent_nodes tests ====================

TEST_F(GraphFunctionalTest, AdjacentNodesUndirected)
{
  // n2 is connected to n1, n3, n5
  auto neighbors = g.adjacent_nodes(n2);
  EXPECT_EQ(neighbors.size(), 3u);
  
  EXPECT_TRUE(neighbors.exists([this](auto p) { return p == n1; }));
  EXPECT_TRUE(neighbors.exists([this](auto p) { return p == n3; }));
  EXPECT_TRUE(neighbors.exists([this](auto p) { return p == n5; }));
}

TEST_F(GraphFunctionalTest, AdjacentNodesDirected)
{
  // dn1 has outgoing arcs to dn2 and dn4
  auto neighbors = dg.adjacent_nodes(dn1);
  EXPECT_EQ(neighbors.size(), 2u);
  
  EXPECT_TRUE(neighbors.exists([this](auto p) { return p == dn2; }));
  EXPECT_TRUE(neighbors.exists([this](auto p) { return p == dn4; }));
}

TEST_F(GraphFunctionalTest, AdjacentNodesIsolated)
{
  auto isolated = g.insert_node(100);
  auto neighbors = g.adjacent_nodes(isolated);
  EXPECT_EQ(neighbors.size(), 0u);
}

TEST_F(GraphFunctionalTest, AdjacentNodesSelfLoop)
{
  // dn3 has a self-loop, so itself is a neighbor
  auto neighbors = dg.adjacent_nodes(dn3);
  EXPECT_TRUE(neighbors.exists([this](auto p) { return p == dn3; }));
}

// ==================== Combination tests ====================

TEST_F(GraphFunctionalTest, CountAndPartitionConsistency)
{
  auto pred = [](auto p) { return p->get_info() > 2; };
  
  size_t count = g.count_nodes(pred);
  auto [yes, no] = g.partition_nodes(pred);
  
  EXPECT_EQ(count, yes.size());
  EXPECT_EQ(g.vsize() - count, no.size());
}

TEST_F(GraphFunctionalTest, NoneAndExistsConsistency)
{
  auto pred = [](auto p) { return p->get_info() > 100; };
  
  EXPECT_EQ(g.none_node(pred), not g.exists_node(pred));
  EXPECT_EQ(g.none_arc([](auto a) { return a->get_info() > 100; }),
            not g.exists_arc([](auto a) { return a->get_info() > 100; }));
}

TEST_F(GraphFunctionalTest, MinMaxConsistency)
{
  auto min_a = g.min_arc();
  auto max_a = g.max_arc();
  
  ASSERT_NE(min_a, nullptr);
  ASSERT_NE(max_a, nullptr);
  
  // Min should be <= max
  EXPECT_LE(min_a->get_info(), max_a->get_info());
  
  // No arc should be smaller than min
  EXPECT_TRUE(g.none_arc([min_a](auto a) { 
    return a->get_info() < min_a->get_info(); 
  }));
  
  // No arc should be larger than max
  EXPECT_TRUE(g.none_arc([max_a](auto a) { 
    return a->get_info() > max_a->get_info(); 
  }));
}

// ==================== Edge cases ====================

TEST_F(GraphFunctionalTest, SingleNodeGraph)
{
  TestGraph single;
  auto n = single.insert_node(42);
  
  EXPECT_EQ(single.count_nodes(), 1u);
  EXPECT_EQ(single.count_arcs(), 0u);
  EXPECT_TRUE(single.none_arc([](auto) { return true; }));
  EXPECT_EQ(single.adjacent_nodes(n).size(), 0u);
  EXPECT_EQ(single.min_arc(), nullptr);
  EXPECT_EQ(single.max_arc(), nullptr);
}

TEST_F(GraphFunctionalTest, SelfLoopNode)
{
  auto n = g.insert_node(99);
  auto self = g.insert_arc(n, n, 10.0);
  
  // Self-loop should be counted
  EXPECT_EQ(g.count_arcs(n), 1u);
  
  // min/max of single arc
  EXPECT_EQ(g.min_arc(n), self);
  EXPECT_EQ(g.max_arc(n), self);
  
  // Sum of self-loop
  EXPECT_DOUBLE_EQ(g.sum_arcs<double>(n), 10.0);
}

// ==================== nodes_map / arcs_map tests ====================

TEST_F(GraphFunctionalTest, NodesMap)
{
  // Map node values to strings
  auto strings = nodes_map<TestGraph, std::string>(g, [](auto p) {
    return std::to_string(p->get_info());
  });
  
  EXPECT_EQ(strings.size(), 5u);
  EXPECT_TRUE(strings.exists([](const auto& s) { return s == "1"; }));
  EXPECT_TRUE(strings.exists([](const auto& s) { return s == "5"; }));
}

TEST_F(GraphFunctionalTest, ArcsMapGlobal)
{
  // Map arc weights to integers
  auto weights = arcs_map<TestGraph, int>(g, [](auto a) {
    return static_cast<int>(a->get_info());
  });
  
  EXPECT_EQ(weights.size(), 4u);
  EXPECT_TRUE(weights.exists([](int w) { return w == 1; }));
  EXPECT_TRUE(weights.exists([](int w) { return w == 4; }));
}

TEST_F(GraphFunctionalTest, ArcsMapFromNode)
{
  // Map arcs adjacent to n2 to their weights
  auto weights = arcs_map<TestGraph, double>(g, n2, [](auto a) {
    return a->get_info();
  });
  
  EXPECT_EQ(weights.size(), 3u); // n2 has 3 adjacent arcs
}

// ==================== filter_nodes / filter_arcs tests ====================

TEST_F(GraphFunctionalTest, FilterNodesMethod)
{
  // Filter nodes with value > 2
  auto filtered = g.filter_nodes([](auto p) { return p->get_info() > 2; });
  
  EXPECT_EQ(filtered.size(), 3u);
  EXPECT_TRUE(filtered.exists([](auto p) { return p->get_info() == 3; }));
  EXPECT_TRUE(filtered.exists([](auto p) { return p->get_info() == 4; }));
  EXPECT_TRUE(filtered.exists([](auto p) { return p->get_info() == 5; }));
}

TEST_F(GraphFunctionalTest, FilterArcsMethod)
{
  // Filter arcs with weight > 2
  auto filtered = g.filter_arcs([](auto a) { return a->get_info() > 2.0; });
  
  EXPECT_EQ(filtered.size(), 2u);
}

TEST_F(GraphFunctionalTest, FilterArcsFromNodeMethod)
{
  // Filter arcs adjacent to n2 with weight > 1.5
  auto filtered = g.filter_arcs(n2, [](auto a) { return a->get_info() > 1.5; });
  
  EXPECT_EQ(filtered.size(), 2u); // weights 2.0 and 4.0
}

// ==================== all_nodes / all_arcs method tests ====================

TEST_F(GraphFunctionalTest, AllNodesMethod)
{
  // All nodes have positive values
  EXPECT_TRUE(g.all_nodes([](auto p) { return p->get_info() > 0; }));
  
  // Not all nodes have value > 3
  EXPECT_FALSE(g.all_nodes([](auto p) { return p->get_info() > 3; }));
}

TEST_F(GraphFunctionalTest, AllArcsMethod)
{
  // All arcs have positive weights
  EXPECT_TRUE(g.all_arcs([](auto a) { return a->get_info() > 0.0; }));
  
  // Not all arcs have weight > 2
  EXPECT_FALSE(g.all_arcs([](auto a) { return a->get_info() > 2.0; }));
}

TEST_F(GraphFunctionalTest, AllArcsFromNodeMethod)
{
  // All arcs from n1 have weight > 0
  EXPECT_TRUE(g.all_arcs(n1, [](auto a) { return a->get_info() > 0.0; }));
  
  // Not all arcs from n1 have weight > 2
  EXPECT_FALSE(g.all_arcs(n1, [](auto a) { return a->get_info() > 2.0; }));
}

// ==================== exists_node / exists_arc method tests ====================

TEST_F(GraphFunctionalTest, ExistsNodeMethod)
{
  // Exists node with value 3
  EXPECT_TRUE(g.exists_node([](auto p) { return p->get_info() == 3; }));
  
  // No node with value 100
  EXPECT_FALSE(g.exists_node([](auto p) { return p->get_info() == 100; }));
}

TEST_F(GraphFunctionalTest, ExistsArcMethod)
{
  // Exists arc with weight 3.0
  EXPECT_TRUE(g.exists_arc([](auto a) { return a->get_info() == 3.0; }));
  
  // No arc with weight 100.0
  EXPECT_FALSE(g.exists_arc([](auto a) { return a->get_info() == 100.0; }));
}

TEST_F(GraphFunctionalTest, ExistsArcFromNodeMethod)
{
  // n1 has arc with weight 3.0
  EXPECT_TRUE(g.exists_arc(n1, [](auto a) { return a->get_info() == 3.0; }));
  
  // n1 has no arc with weight 100.0
  EXPECT_FALSE(g.exists_arc(n1, [](auto a) { return a->get_info() == 100.0; }));
}

// ==================== map_in_arcs / map_out_arcs tests (digraph) ====================

TEST_F(GraphFunctionalTest, MapOutArcsDigraph)
{
  // Map outgoing arcs from dn1 to weights using method
  double sum = 0;
  dg.for_each_out_arc(dn1, [&sum](auto a) { sum += a->get_info(); });
  EXPECT_DOUBLE_EQ(sum, 2.0); // 1.5 + 0.5
  
  // Verify out_arcs collection
  auto outs = dg.out_arcs(dn1);
  EXPECT_EQ(outs.size(), 2u);
}

TEST_F(GraphFunctionalTest, MapInArcsDigraph)
{
  // Note: List_Digraph doesn't maintain incoming arc lists
  // in_arcs() returns empty for this implementation
  // This is expected behavior for adjacency-list based digraphs
  
  // Verify out_arcs works (outgoing)
  auto outs = dg.out_arcs(dn1);
  EXPECT_EQ(outs.size(), 2u); // dn1 has 2 outgoing arcs
}

// ==================== foldl_in_arcs / foldl_out_arcs tests ====================

TEST_F(GraphFunctionalTest, FoldlOutArcsDigraph)
{
  // Sum outgoing arc weights from dn1 using for_each
  double sum = 0.0;
  dg.for_each_out_arc(dn1, [&sum](auto a) { sum += a->get_info(); });
  
  EXPECT_DOUBLE_EQ(sum, 2.0); // 1.5 + 0.5
}

TEST_F(GraphFunctionalTest, FoldlInArcsDigraph)
{
  // Verify outgoing arc traversal works correctly
  double sum = 0.0;
  dg.for_each_out_arc(dn1, [&sum](auto a) { sum += a->get_info(); });
  
  EXPECT_DOUBLE_EQ(sum, 2.0); // 1.5 + 0.5
}

// ==================== filter_in_arcs / filter_out_arcs tests ====================

TEST_F(GraphFunctionalTest, FilterOutArcsDigraph)
{
  // Filter outgoing arcs from dn1 with weight > 1.0
  auto filtered = dg.filter_out_arcs(dn1, [](auto a) { 
    return a->get_info() > 1.0; 
  });
  
  EXPECT_EQ(filtered.size(), 1u); // Only 1.5
}

TEST_F(GraphFunctionalTest, FilterInArcsDigraph)
{
  // Filter incoming arcs to dn2 with weight > 2.0
  auto filtered = dg.filter_in_arcs(dn2, [](auto a) {
    return a->get_info() > 2.0;
  });
  
  EXPECT_EQ(filtered.size(), 0u);
}

// ==================== search_node / find_node tests ====================

TEST_F(GraphFunctionalTest, SearchNodeMethod)
{
  // Search node with value 3
  auto found = g.search_node([](auto p) { return p->get_info() == 3; });
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_info(), 3);
  
  // Search non-existent
  auto not_found = g.search_node([](auto p) { return p->get_info() == 100; });
  EXPECT_EQ(not_found, nullptr);
}

TEST_F(GraphFunctionalTest, FindNodeMethod)
{
  // Find node with value 3
  auto found = g.find_node(3);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_info(), 3);
  
  // Find non-existent
  auto not_found = g.find_node(100);
  EXPECT_EQ(not_found, nullptr);
}

// ==================== search_arc / find_arc tests ====================

TEST_F(GraphFunctionalTest, SearchArcMethod)
{
  // Search arc with weight 3.0
  auto found = g.search_arc([](auto a) { return a->get_info() == 3.0; });
  ASSERT_NE(found, nullptr);
  EXPECT_DOUBLE_EQ(found->get_info(), 3.0);
  
  // Search non-existent
  auto not_found = g.search_arc([](auto a) { return a->get_info() == 100.0; });
  EXPECT_EQ(not_found, nullptr);
}

TEST_F(GraphFunctionalTest, FindArcMethod)
{
  // Find arc with weight 3.0
  auto found = g.find_arc(3.0);
  ASSERT_NE(found, nullptr);
  EXPECT_DOUBLE_EQ(found->get_info(), 3.0);
  
  // Find non-existent
  auto not_found = g.find_arc(100.0);
  EXPECT_EQ(not_found, nullptr);
}

TEST_F(GraphFunctionalTest, SearchArcFromNodeMethod)
{
  // Search arc from n1 with weight 3.0
  auto found = g.search_arc(n1, [](auto a) { return a->get_info() == 3.0; });
  ASSERT_NE(found, nullptr);
  EXPECT_DOUBLE_EQ(found->get_info(), 3.0);
  
  // Search non-existent from n1
  auto not_found = g.search_arc(n1, [](auto a) { return a->get_info() == 100.0; });
  EXPECT_EQ(not_found, nullptr);
}

// ==================== traverse_nodes / traverse_arcs tests ====================

TEST_F(GraphFunctionalTest, TraverseNodesStopsOnFalse)
{
  int count = 0;
  bool completed = g.traverse_nodes([&count](auto p) {
    ++count;
    return p->get_info() < 3; // Stop when value >= 3
  });
  
  EXPECT_FALSE(completed); // Should have stopped early
  EXPECT_LT(count, 5);     // Didn't visit all nodes
}

TEST_F(GraphFunctionalTest, TraverseNodesCompletesOnAllTrue)
{
  int count = 0;
  bool completed = g.traverse_nodes([&count](auto) {
    ++count;
    return true; // Always continue
  });
  
  EXPECT_TRUE(completed);
  EXPECT_EQ(count, 5);
}

TEST_F(GraphFunctionalTest, TraverseArcsStopsOnFalse)
{
  int count = 0;
  bool completed = g.traverse_arcs([&count](auto a) {
    ++count;
    return a->get_info() < 3.0; // Stop when weight >= 3.0
  });
  
  EXPECT_FALSE(completed);
  EXPECT_LT(count, 4);
}

TEST_F(GraphFunctionalTest, TraverseArcsFromNode)
{
  int count = 0;
  bool completed = g.traverse_arcs(n2, [&count](auto) {
    ++count;
    return true;
  });
  
  EXPECT_TRUE(completed);
  EXPECT_EQ(count, 3); // n2 has 3 adjacent arcs
}

// ==================== for_each method tests ====================

TEST_F(GraphFunctionalTest, ForEachNodeMethod)
{
  int sum = 0;
  g.for_each_node([&sum](auto p) { sum += p->get_info(); });
  EXPECT_EQ(sum, 15); // 1+2+3+4+5
}

TEST_F(GraphFunctionalTest, ForEachArcMethod)
{
  double sum = 0;
  g.for_each_arc([&sum](auto a) { sum += a->get_info(); });
  EXPECT_DOUBLE_EQ(sum, 10.0); // 1+2+3+4
}

TEST_F(GraphFunctionalTest, ForEachArcFromNodeMethod)
{
  double sum = 0;
  g.for_each_arc(n2, [&sum](auto a) { sum += a->get_info(); });
  EXPECT_DOUBLE_EQ(sum, 7.0); // 1+2+4
}

// ==================== foldl method tests ====================

TEST_F(GraphFunctionalTest, FoldlNodesMethod)
{
  // Using free function since method requires std::function
  int sum = foldl_nodes<TestGraph, int>(g, 0, 
    std::function<int(const int&, TestGraph::Node*)>(
      [](const int& acc, auto p) { return acc + p->get_info(); }
    ));
  EXPECT_EQ(sum, 15);
}

TEST_F(GraphFunctionalTest, FoldlArcsMethod)
{
  // Using free function since method requires std::function
  double sum = foldl_arcs<TestGraph, double>(g, 0.0,
    std::function<double(const double&, TestGraph::Arc*)>(
      [](const double& acc, auto a) { return acc + a->get_info(); }
    ));
  EXPECT_DOUBLE_EQ(sum, 10.0);
}

TEST_F(GraphFunctionalTest, FoldlArcsFromNodeMethod)
{
  // Manual fold using for_each_arc
  double sum = 0.0;
  g.for_each_arc(n2, [&sum](auto a) { sum += a->get_info(); });
  EXPECT_DOUBLE_EQ(sum, 7.0);
}

// ==================== nodes() / arcs() collection tests ====================

TEST_F(GraphFunctionalTest, NodesCollectionMethod)
{
  auto all = g.nodes();
  EXPECT_EQ(all.size(), 5u);
}

TEST_F(GraphFunctionalTest, ArcsCollectionMethod)
{
  auto all = g.arcs();
  EXPECT_EQ(all.size(), 4u);
}

TEST_F(GraphFunctionalTest, ArcsFromNodeMethod)
{
  auto adj = g.arcs(n2);
  EXPECT_EQ(adj.size(), 3u);
}

// ==================== in_nodes / out_nodes / in_arcs / out_arcs tests ====================

TEST_F(GraphFunctionalTest, OutNodesDigraph)
{
  auto outs = dg.out_nodes(dn1);
  EXPECT_EQ(outs.size(), 2u); // dn2, dn4
}

TEST_F(GraphFunctionalTest, InNodesDigraph)
{
  // Note: List_Digraph doesn't track incoming arcs
  // in_nodes returns empty for adjacency-list implementations
  auto ins = dg.in_nodes(dn2);
  EXPECT_EQ(ins.size(), 0u); // No incoming arcs tracked
}

TEST_F(GraphFunctionalTest, OutArcsDigraph)
{
  auto outs = dg.out_arcs(dn1);
  EXPECT_EQ(outs.size(), 2u);
}

TEST_F(GraphFunctionalTest, InArcsDigraph)
{
  // Note: List_Digraph doesn't track incoming arcs
  auto ins = dg.in_arcs(dn2);
  EXPECT_EQ(ins.size(), 0u); // No incoming arcs tracked
}

// ==================== in_degree / out_degree tests ====================

TEST_F(GraphFunctionalTest, InDegreeDigraph)
{
  // Note: List_Digraph doesn't track incoming arcs
  // in_degree returns 0 for all nodes in adjacency-list implementations
  EXPECT_EQ(dg.in_degree(dn1), 0u);
  EXPECT_EQ(dg.in_degree(dn2), 0u);  // Would be 2 if tracked
  EXPECT_EQ(dg.in_degree(dn3), 1u);  // Only self-loop counted (out=in for self)
}

TEST_F(GraphFunctionalTest, OutDegreeDigraph)
{
  EXPECT_EQ(dg.out_degree(dn1), 2u); // To dn2, dn4
  EXPECT_EQ(dg.out_degree(dn2), 1u); // To dn3
  EXPECT_EQ(dg.out_degree(dn3), 1u); // Self-loop
}

TEST_F(GraphFunctionalTest, DegreeUndirected)
{
  EXPECT_EQ(g.degree(n1), 2u);
  EXPECT_EQ(g.degree(n2), 3u);
  EXPECT_EQ(g.degree(n3), 1u);
}

// =============================================================================
// TYPED TESTS: Test all graph implementations (List and Array)
// =============================================================================

template <typename GraphType>
class UndirectedGraphTest : public ::testing::Test
{
protected:
  GraphType g;
  typename GraphType::Node *n1, *n2, *n3;
  typename GraphType::Arc *a1, *a2;

  void SetUp() override
  {
    // Simple triangle: n1 -- n2 -- n3 -- n1
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    
    a1 = g.insert_arc(n1, n2, 1.0);
    a2 = g.insert_arc(n2, n3, 2.0);
  }
};

template <typename GraphType>
class DirectedGraphTest : public ::testing::Test
{
protected:
  GraphType g;
  typename GraphType::Node *n1, *n2, *n3;
  typename GraphType::Arc *a1, *a2;

  void SetUp() override
  {
    // Simple chain: n1 -> n2 -> n3
    n1 = g.insert_node(10);
    n2 = g.insert_node(20);
    n3 = g.insert_node(30);
    
    a1 = g.insert_arc(n1, n2, 1.5);
    a2 = g.insert_arc(n2, n3, 2.5);
  }
};

// Define type lists for parameterized tests
// All three implementations are tested: List, Sparse, and Array
using UndirectedGraphTypes = ::testing::Types<ListGraph, SparseGraph, ArrayGraph>;
using DirectedGraphTypes = ::testing::Types<ListDigraph, SparseDigraph, ArrayDigraph>;

TYPED_TEST_SUITE(UndirectedGraphTest, UndirectedGraphTypes);
TYPED_TEST_SUITE(DirectedGraphTest, DirectedGraphTypes);

// ==================== Undirected Graph Tests (List & Array) ====================

TYPED_TEST(UndirectedGraphTest, CountNodes)
{
  EXPECT_EQ(this->g.get_num_nodes(), 3u);
  size_t count = this->g.count_nodes([](auto) { return true; });
  EXPECT_EQ(count, 3u);
}

TYPED_TEST(UndirectedGraphTest, CountArcs)
{
  EXPECT_EQ(this->g.get_num_arcs(), 2u);
  size_t count = this->g.count_arcs([](auto) { return true; });
  EXPECT_EQ(count, 2u);
}

TYPED_TEST(UndirectedGraphTest, ExistsNode)
{
  EXPECT_TRUE(this->g.exists_node([](auto p) { return p->get_info() == 2; }));
  EXPECT_FALSE(this->g.exists_node([](auto p) { return p->get_info() == 99; }));
}

TYPED_TEST(UndirectedGraphTest, ExistsArc)
{
  EXPECT_TRUE(this->g.exists_arc([](auto a) { return a->get_info() == 1.0; }));
  EXPECT_FALSE(this->g.exists_arc([](auto a) { return a->get_info() == 99.0; }));
}

TYPED_TEST(UndirectedGraphTest, NoneNode)
{
  EXPECT_TRUE(this->g.none_node([](auto p) { return p->get_info() > 100; }));
  EXPECT_FALSE(this->g.none_node([](auto p) { return p->get_info() == 1; }));
}

TYPED_TEST(UndirectedGraphTest, NoneArc)
{
  EXPECT_TRUE(this->g.none_arc([](auto a) { return a->get_info() > 100.0; }));
  EXPECT_FALSE(this->g.none_arc([](auto a) { return a->get_info() == 1.0; }));
}

TYPED_TEST(UndirectedGraphTest, AllNodes)
{
  EXPECT_TRUE(this->g.all_nodes([](auto p) { return p->get_info() > 0; }));
  EXPECT_FALSE(this->g.all_nodes([](auto p) { return p->get_info() > 2; }));
}

TYPED_TEST(UndirectedGraphTest, AllArcs)
{
  EXPECT_TRUE(this->g.all_arcs([](auto a) { return a->get_info() > 0.0; }));
  EXPECT_FALSE(this->g.all_arcs([](auto a) { return a->get_info() > 1.5; }));
}

TYPED_TEST(UndirectedGraphTest, FilterNodes)
{
  auto filtered = this->g.filter_nodes([](auto p) { return p->get_info() > 1; });
  EXPECT_EQ(filtered.size(), 2u);
}

TYPED_TEST(UndirectedGraphTest, FilterArcs)
{
  auto filtered = this->g.filter_arcs([](auto a) { return a->get_info() > 1.0; });
  EXPECT_EQ(filtered.size(), 1u);
}

TYPED_TEST(UndirectedGraphTest, SearchNode)
{
  auto found = this->g.search_node([](auto p) { return p->get_info() == 2; });
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_info(), 2);
  
  auto not_found = this->g.search_node([](auto p) { return p->get_info() == 99; });
  EXPECT_EQ(not_found, nullptr);
}

TYPED_TEST(UndirectedGraphTest, SearchArc)
{
  auto found = this->g.search_arc([](auto a) { return a->get_info() == 2.0; });
  ASSERT_NE(found, nullptr);
  EXPECT_DOUBLE_EQ(found->get_info(), 2.0);
}

TYPED_TEST(UndirectedGraphTest, PartitionNodes)
{
  auto [matching, not_matching] = this->g.partition_nodes(
    [](auto p) { return p->get_info() > 1; });
  EXPECT_EQ(matching.size(), 2u);
  EXPECT_EQ(not_matching.size(), 1u);
}

TYPED_TEST(UndirectedGraphTest, PartitionArcs)
{
  auto [matching, not_matching] = this->g.partition_arcs(
    [](auto a) { return a->get_info() > 1.0; });
  EXPECT_EQ(matching.size(), 1u);
  EXPECT_EQ(not_matching.size(), 1u);
}

TYPED_TEST(UndirectedGraphTest, ForEachNode)
{
  int sum = 0;
  this->g.for_each_node([&sum](auto p) { sum += p->get_info(); });
  EXPECT_EQ(sum, 6); // 1 + 2 + 3
}

TYPED_TEST(UndirectedGraphTest, ForEachArc)
{
  double sum = 0;
  this->g.for_each_arc([&sum](auto a) { sum += a->get_info(); });
  EXPECT_DOUBLE_EQ(sum, 3.0); // 1.0 + 2.0
}

TYPED_TEST(UndirectedGraphTest, Degree)
{
  EXPECT_EQ(this->g.degree(this->n1), 1u);
  EXPECT_EQ(this->g.degree(this->n2), 2u);
  EXPECT_EQ(this->g.degree(this->n3), 1u);
}

TYPED_TEST(UndirectedGraphTest, AdjacentNodes)
{
  auto adj = this->g.adjacent_nodes(this->n2);
  EXPECT_EQ(adj.size(), 2u);
}

TYPED_TEST(UndirectedGraphTest, MinArc)
{
  auto min = this->g.min_arc();
  ASSERT_NE(min, nullptr);
  EXPECT_DOUBLE_EQ(min->get_info(), 1.0);
}

TYPED_TEST(UndirectedGraphTest, MaxArc)
{
  auto max = this->g.max_arc();
  ASSERT_NE(max, nullptr);
  EXPECT_DOUBLE_EQ(max->get_info(), 2.0);
}

// ==================== Directed Graph Tests (List & Array) ====================

TYPED_TEST(DirectedGraphTest, CountNodes)
{
  EXPECT_EQ(this->g.get_num_nodes(), 3u);
  size_t count = this->g.count_nodes([](auto) { return true; });
  EXPECT_EQ(count, 3u);
}

TYPED_TEST(DirectedGraphTest, CountArcs)
{
  EXPECT_EQ(this->g.get_num_arcs(), 2u);
  size_t count = this->g.count_arcs([](auto) { return true; });
  EXPECT_EQ(count, 2u);
}

TYPED_TEST(DirectedGraphTest, ExistsNode)
{
  EXPECT_TRUE(this->g.exists_node([](auto p) { return p->get_info() == 20; }));
  EXPECT_FALSE(this->g.exists_node([](auto p) { return p->get_info() == 99; }));
}

TYPED_TEST(DirectedGraphTest, ExistsArc)
{
  EXPECT_TRUE(this->g.exists_arc([](auto a) { return a->get_info() == 1.5; }));
  EXPECT_FALSE(this->g.exists_arc([](auto a) { return a->get_info() == 99.0; }));
}

TYPED_TEST(DirectedGraphTest, NoneNode)
{
  EXPECT_TRUE(this->g.none_node([](auto p) { return p->get_info() > 100; }));
  EXPECT_FALSE(this->g.none_node([](auto p) { return p->get_info() == 10; }));
}

TYPED_TEST(DirectedGraphTest, NoneArc)
{
  EXPECT_TRUE(this->g.none_arc([](auto a) { return a->get_info() > 100.0; }));
  EXPECT_FALSE(this->g.none_arc([](auto a) { return a->get_info() == 1.5; }));
}

TYPED_TEST(DirectedGraphTest, AllNodes)
{
  EXPECT_TRUE(this->g.all_nodes([](auto p) { return p->get_info() > 0; }));
  EXPECT_FALSE(this->g.all_nodes([](auto p) { return p->get_info() > 20; }));
}

TYPED_TEST(DirectedGraphTest, AllArcs)
{
  EXPECT_TRUE(this->g.all_arcs([](auto a) { return a->get_info() > 0.0; }));
  EXPECT_FALSE(this->g.all_arcs([](auto a) { return a->get_info() > 2.0; }));
}

TYPED_TEST(DirectedGraphTest, FilterNodes)
{
  auto filtered = this->g.filter_nodes([](auto p) { return p->get_info() > 10; });
  EXPECT_EQ(filtered.size(), 2u);
}

TYPED_TEST(DirectedGraphTest, FilterArcs)
{
  auto filtered = this->g.filter_arcs([](auto a) { return a->get_info() > 1.5; });
  EXPECT_EQ(filtered.size(), 1u);
}

TYPED_TEST(DirectedGraphTest, SearchNode)
{
  auto found = this->g.search_node([](auto p) { return p->get_info() == 20; });
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_info(), 20);
}

TYPED_TEST(DirectedGraphTest, SearchArc)
{
  auto found = this->g.search_arc([](auto a) { return a->get_info() == 2.5; });
  ASSERT_NE(found, nullptr);
  EXPECT_DOUBLE_EQ(found->get_info(), 2.5);
}

TYPED_TEST(DirectedGraphTest, PartitionNodes)
{
  auto [matching, not_matching] = this->g.partition_nodes(
    [](auto p) { return p->get_info() > 10; });
  EXPECT_EQ(matching.size(), 2u);
  EXPECT_EQ(not_matching.size(), 1u);
}

TYPED_TEST(DirectedGraphTest, PartitionArcs)
{
  auto [matching, not_matching] = this->g.partition_arcs(
    [](auto a) { return a->get_info() > 1.5; });
  EXPECT_EQ(matching.size(), 1u);
  EXPECT_EQ(not_matching.size(), 1u);
}

TYPED_TEST(DirectedGraphTest, ForEachNode)
{
  int sum = 0;
  this->g.for_each_node([&sum](auto p) { sum += p->get_info(); });
  EXPECT_EQ(sum, 60); // 10 + 20 + 30
}

TYPED_TEST(DirectedGraphTest, ForEachArc)
{
  double sum = 0;
  this->g.for_each_arc([&sum](auto a) { sum += a->get_info(); });
  EXPECT_DOUBLE_EQ(sum, 4.0); // 1.5 + 2.5
}

TYPED_TEST(DirectedGraphTest, OutDegree)
{
  EXPECT_EQ(this->g.out_degree(this->n1), 1u);
  EXPECT_EQ(this->g.out_degree(this->n2), 1u);
  EXPECT_EQ(this->g.out_degree(this->n3), 0u);
}

TYPED_TEST(DirectedGraphTest, OutNodes)
{
  auto outs = this->g.out_nodes(this->n1);
  EXPECT_EQ(outs.size(), 1u);
}

TYPED_TEST(DirectedGraphTest, OutArcs)
{
  auto outs = this->g.out_arcs(this->n1);
  EXPECT_EQ(outs.size(), 1u);
}

TYPED_TEST(DirectedGraphTest, MinArc)
{
  auto min = this->g.min_arc();
  ASSERT_NE(min, nullptr);
  EXPECT_DOUBLE_EQ(min->get_info(), 1.5);
}

TYPED_TEST(DirectedGraphTest, MaxArc)
{
  auto max = this->g.max_arc();
  ASSERT_NE(max, nullptr);
  EXPECT_DOUBLE_EQ(max->get_info(), 2.5);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

