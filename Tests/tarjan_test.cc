
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
 * @file tarjan_test.cc
 * @brief Tests for Tarjan
 */
#include <gtest/gtest.h>
#include <tpl_graph.H>
#include <Tarjan.H>

using namespace Aleph;

using TestDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

// Helper functions
TestDigraph::Node *add_node(TestDigraph &g, int value)
{
  return g.insert_node(value);
}

TestDigraph::Arc *add_arc(TestDigraph &g, TestDigraph::Node *src, TestDigraph::Node *tgt,
                      int value = 0)
{
  return g.insert_arc(src, tgt, value);
}

// ============================================================================
// Test: Empty graph
// ============================================================================
TEST(TarjanTest, EmptyGraph)
{
  TestDigraph g;
  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Test connected_components returning node lists
  auto sccs = tarjan.connected_components(g);
  EXPECT_TRUE(sccs.is_empty());

  // Test connected_components with sizes
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_TRUE(sizes.is_empty());

  // Test has_cycle
  EXPECT_FALSE(tarjan.has_cycle(g));

  // Test is_dag
  EXPECT_TRUE(tarjan.is_dag(g));

  // Test compute_cycle
  Path<TestDigraph> path(g);
  EXPECT_FALSE(tarjan.compute_cycle(g, path));
  EXPECT_TRUE(path.is_empty());

  // Test test_connectivity - empty graph is trivially connected
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Single node without self-loop
// ============================================================================
TEST(TarjanTest, SingleNodeNoLoop)
{
  TestDigraph g;
  auto n1 = add_node(g, 1);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Test connected_components
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 1u);
  EXPECT_EQ(sccs.get_first().size(), 1u);
  EXPECT_EQ(sccs.get_first().get_first(), n1);

  // Test sizes
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_EQ(sizes.size(), 1u);
  EXPECT_EQ(sizes.get_first(), 1u);

  // Test has_cycle - single node without self-loop has no cycle
  EXPECT_FALSE(tarjan.has_cycle(g));

  // Test is_dag
  EXPECT_TRUE(tarjan.is_dag(g));

  // Test compute_cycle
  Path<TestDigraph> path(g);
  EXPECT_FALSE(tarjan.compute_cycle(g, path));

  // Test connectivity
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Single node with self-loop
// ============================================================================
TEST(TarjanTest, SingleNodeWithLoop)
{
  TestDigraph g;
  auto n1 = add_node(g, 1);
  add_arc(g, n1, n1);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Test connected_components
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 1u);
  EXPECT_EQ(sccs.get_first().size(), 1u);

  // Test has_cycle - self-loop IS a cycle
  EXPECT_TRUE(tarjan.has_cycle(g));

  // Test is_dag
  EXPECT_FALSE(tarjan.is_dag(g));

  // Test compute_cycle - should find the self-loop cycle
  Path<TestDigraph> path(g);
  bool found = tarjan.compute_cycle(g, path);
  EXPECT_TRUE(found);
  EXPECT_FALSE(path.is_empty());
  // For a self-loop, the path should start and end at the same node
  EXPECT_EQ(path.get_first_node(), path.get_last_node());

  // Test connectivity
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Two nodes, one arc (A -> B), no cycle
// ============================================================================
TEST(TarjanTest, TwoNodesNoCycle)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  add_arc(g, a, b);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Should have 2 SCCs (each node is its own SCC)
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 2u);

  // Test sizes
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_EQ(sizes.size(), 2u);
  for (auto sz : sizes)
    EXPECT_EQ(sz, 1u);

  // No cycle
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));

  Path<TestDigraph> path(g);
  EXPECT_FALSE(tarjan.compute_cycle(g, path));

  // Not strongly connected (can't go from B to A)
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Two nodes with cycle (A <-> B)
// ============================================================================
TEST(TarjanTest, TwoNodesWithCycle)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  add_arc(g, a, b);
  add_arc(g, b, a);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Should have 1 SCC with both nodes
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 1u);
  EXPECT_EQ(sccs.get_first().size(), 2u);

  // Has cycle
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_FALSE(tarjan.is_dag(g));

  // compute_cycle should find the cycle
  Path<TestDigraph> path(g);
  EXPECT_TRUE(tarjan.compute_cycle(g, path));
  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path.get_first_node(), path.get_last_node());

  // Strongly connected
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Linear chain (A -> B -> C -> D), DAG
// ============================================================================
TEST(TarjanTest, LinearChain)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  auto d = add_node(g, 4);
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, d);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // 4 SCCs
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 4u);

  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Simple cycle (A -> B -> C -> A)
// ============================================================================
TEST(TarjanTest, SimpleCycle)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, a);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // 1 SCC with 3 nodes
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 1u);
  EXPECT_EQ(sccs.get_first().size(), 3u);

  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_FALSE(tarjan.is_dag(g));

  Path<TestDigraph> path(g);
  EXPECT_TRUE(tarjan.compute_cycle(g, path));
  EXPECT_EQ(path.get_first_node(), path.get_last_node());

  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Multiple SCCs
// ============================================================================
TEST(TarjanTest, MultipleSCCs)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  auto d = add_node(g, 4);
  auto e = add_node(g, 5);

  // SCC1: A -> B -> C -> A
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, a);

  // SCC2: D -> E -> D
  add_arc(g, d, e);
  add_arc(g, e, d);

  // Inter-SCC arc
  add_arc(g, b, d);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // 2 SCCs
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 2u);

  // Check sizes (3 and 2)
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_EQ(sizes.size(), 2u);
  size_t total = 0;
  for (auto sz : sizes)
    total += sz;
  EXPECT_EQ(total, 5u);

  // Has cycle
  EXPECT_TRUE(tarjan.has_cycle(g));

  // Not strongly connected (different SCCs)
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Connected components with block list and arc list
// ============================================================================
TEST(TarjanTest, ConnectedComponentsWithArcs)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  auto d = add_node(g, 4);

  // SCC1: A -> B -> A
  add_arc(g, a, b);
  add_arc(g, b, a);

  // SCC2: C -> D -> C
  add_arc(g, c, d);
  add_arc(g, d, c);

  // Inter-SCC arcs
  auto cross1 = add_arc(g, b, c);
  auto cross2 = add_arc(g, a, d);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  DynList<TestDigraph> blk_list;
  DynList<TestDigraph::Arc *> arc_list;
  tarjan.connected_components(g, blk_list, arc_list);

  // 2 blocks
  EXPECT_EQ(blk_list.size(), 2u);

  // 2 inter-block arcs
  EXPECT_EQ(arc_list.size(), 2u);

  // Verify the cross arcs are in the list
  bool found_cross1 = false, found_cross2 = false;
  for (auto arc : arc_list)
  {
    if (arc == cross1)
      found_cross1 = true;
    if (arc == cross2)
      found_cross2 = true;
  }
  EXPECT_TRUE(found_cross1);
  EXPECT_TRUE(found_cross2);
}

// ============================================================================
// Test: compute_cycle from specific source node
// ============================================================================
TEST(TarjanTest, ComputeCycleFromSource)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, b); // cycle between B and C only

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Starting from A, we should find the cycle in B-C
  Path<TestDigraph> path1(g);
  bool found1 = tarjan.compute_cycle(g, a, path1);
  EXPECT_TRUE(found1);

  // Starting from B, we should find the cycle
  Path<TestDigraph> path2(g);
  bool found2 = tarjan.compute_cycle(g, b, path2);
  EXPECT_TRUE(found2);
}

// ============================================================================
// Test: Compute_Cycle_In_Digraph wrapper class
// ============================================================================
TEST(TarjanTest, ComputeCycleInTestDigraphClass)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, a);

  // Test with operator()(g, path)
  Compute_Cycle_In_Digraph<TestDigraph> finder;
  Path<TestDigraph> path(g);
  bool found = finder(g, path);
  EXPECT_TRUE(found);
  EXPECT_FALSE(path.is_empty());

  // Test with operator()(g) returning path
  auto path2 = finder(g);
  EXPECT_FALSE(path2.is_empty());

  // Test with operator()(g, src) returning path
  auto path3 = finder(g, a);
  EXPECT_FALSE(path3.is_empty());
}

// ============================================================================
// Test: DAG with diamond shape
// ============================================================================
TEST(TarjanTest, DiamondDAG)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  auto d = add_node(g, 4);
  add_arc(g, a, b);
  add_arc(g, a, c);
  add_arc(g, b, d);
  add_arc(g, c, d);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // 4 SCCs (each node is its own SCC)
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 4u);

  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Complex graph with nested cycles
// ============================================================================
TEST(TarjanTest, ComplexNestedCycles)
{
  TestDigraph g;
  auto n1 = add_node(g, 1);
  auto n2 = add_node(g, 2);
  auto n3 = add_node(g, 3);
  auto n4 = add_node(g, 4);
  auto n5 = add_node(g, 5);
  auto n6 = add_node(g, 6);

  // Create a strongly connected component with all 6 nodes
  add_arc(g, n1, n2);
  add_arc(g, n2, n3);
  add_arc(g, n3, n1); // small cycle
  add_arc(g, n3, n4);
  add_arc(g, n4, n5);
  add_arc(g, n5, n6);
  add_arc(g, n6, n1); // large cycle back to n1

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // All nodes should be in one SCC
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 1u);
  EXPECT_EQ(sccs.get_first().size(), 6u);

  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));

  Path<TestDigraph> path(g);
  EXPECT_TRUE(tarjan.compute_cycle(g, path));
  EXPECT_EQ(path.get_first_node(), path.get_last_node());
}

// ============================================================================
// Test: Operator() overloads for DynDlist
// ============================================================================
TEST(TarjanTest, DynDlistOverloads)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  add_arc(g, a, b);
  add_arc(g, b, a);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Test DynDlist<GT> overload
  DynDlist<TestDigraph> blk_list;
  DynDlist<TestDigraph::Arc *> arc_list;
  tarjan(g, blk_list, arc_list);
  EXPECT_EQ(blk_list.size(), 1u);
  EXPECT_TRUE(arc_list.is_empty()); // no inter-block arcs

  // Test DynDlist<DynDlist<Node*>> overload
  DynDlist<DynDlist<TestDigraph::Node *>> blks;
  tarjan(g, blks);
  EXPECT_EQ(blks.size(), 1u);
  EXPECT_EQ(blks.get_first().size(), 2u);
}

// ============================================================================
// Test: Disconnected graph components (no edges between them)
// ============================================================================
TEST(TarjanTest, DisconnectedComponents)
{
  TestDigraph g;
  // Component 1: A -> B -> C -> A
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, a);

  // Component 2: D (isolated)
  auto d = add_node(g, 4);
  (void)d;

  // Component 3: E -> F
  auto e = add_node(g, 5);
  auto f = add_node(g, 6);
  add_arc(g, e, f);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Should have 4 SCCs: {A,B,C}, {D}, {E}, {F}
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 4u);

  // Check sizes
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_EQ(sizes.size(), 4u);

  // One SCC has size 3, others have size 1
  size_t count_3 = 0, count_1 = 0;
  for (auto sz : sizes)
  {
    if (sz == 3)
      count_3++;
    else if (sz == 1)
      count_1++;
  }
  EXPECT_EQ(count_3, 1u);
  EXPECT_EQ(count_1, 3u);

  // Has cycle (in A-B-C)
  EXPECT_TRUE(tarjan.has_cycle(g));

  // Not strongly connected
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Large cycle
// ============================================================================
TEST(TarjanTest, LargeCycle)
{
  TestDigraph g;
  const size_t N = 100;

  DynList<TestDigraph::Node *> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.append(add_node(g, static_cast<int>(i)));

  // Create a cycle: 0 -> 1 -> 2 -> ... -> 99 -> 0
  auto it = nodes.get_it();
  auto prev = it.get_curr();
  auto first = prev;
  for (it.next(); it.has_curr(); it.next())
  {
    auto curr = it.get_curr();
    add_arc(g, prev, curr);
    prev = curr;
  }
  add_arc(g, prev, first); // close the cycle

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // One SCC with all N nodes
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 1u);
  EXPECT_EQ(sccs.get_first().size(), N);

  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));

  Path<TestDigraph> path(g);
  EXPECT_TRUE(tarjan.compute_cycle(g, path));
}

// ============================================================================
// Test: Graph with only inter-SCC arcs (tree-like structure of SCCs)
// ============================================================================
TEST(TarjanTest, SCCTree)
{
  TestDigraph g;
  // SCC A: a1 -> a2 -> a1
  auto a1 = add_node(g, 1);
  auto a2 = add_node(g, 2);
  add_arc(g, a1, a2);
  add_arc(g, a2, a1);

  // SCC B: b1 -> b2 -> b1
  auto b1 = add_node(g, 3);
  auto b2 = add_node(g, 4);
  add_arc(g, b1, b2);
  add_arc(g, b2, b1);

  // SCC C: c1 -> c2 -> c1
  auto c1 = add_node(g, 5);
  auto c2 = add_node(g, 6);
  add_arc(g, c1, c2);
  add_arc(g, c2, c1);

  // Tree structure: A -> B, A -> C
  add_arc(g, a1, b1);
  add_arc(g, a2, c1);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // 3 SCCs
  auto sccs = tarjan.connected_components(g);
  EXPECT_EQ(sccs.size(), 3u);

  // Each SCC has 2 nodes
  for (auto &scc : sccs)
    EXPECT_EQ(scc.size(), 2u);

  // Has cycle (within each SCC)
  EXPECT_TRUE(tarjan.has_cycle(g));

  // Not strongly connected
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: num_connected_components convenience method
// ============================================================================
TEST(TarjanTest, NumConnectedComponents)
{
  TestDigraph g;

  // Empty graph
  Tarjan_Connected_Components<TestDigraph> tarjan1;
  EXPECT_EQ(tarjan1.num_connected_components(g), 0u);

  // Single node
  auto a = add_node(g, 1);
  Tarjan_Connected_Components<TestDigraph> tarjan2;
  EXPECT_EQ(tarjan2.num_connected_components(g), 1u);

  // Two nodes, no connection = 2 SCCs
  auto b = add_node(g, 2);
  Tarjan_Connected_Components<TestDigraph> tarjan3;
  EXPECT_EQ(tarjan3.num_connected_components(g), 2u);

  // Add bidirectional connection = 1 SCC
  add_arc(g, a, b);
  add_arc(g, b, a);
  Tarjan_Connected_Components<TestDigraph> tarjan4;
  EXPECT_EQ(tarjan4.num_connected_components(g), 1u);
}

// ============================================================================
// Test: Null source node validation
// ============================================================================
TEST(TarjanTest, NullSourceValidation)
{
  TestDigraph g;
  add_node(g, 1);

  Tarjan_Connected_Components<TestDigraph> tarjan;
  Path<TestDigraph> path(g);

  EXPECT_THROW((void) tarjan.compute_cycle(g, nullptr, path), std::domain_error);
}

// ============================================================================
// Test: Filter accessor
// ============================================================================
TEST(TarjanTest, FilterAccessor)
{
  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Just verify the methods compile and return references
  [[maybe_unused]] auto &filter = tarjan.get_filter();

  const Tarjan_Connected_Components<TestDigraph> &const_tarjan = tarjan;
  [[maybe_unused]] const auto &const_filter = const_tarjan.get_filter();
}

// ============================================================================
// Test: State getters
// ============================================================================
TEST(TarjanTest, StateGetters)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  add_arc(g, a, b);
  add_arc(g, b, a);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Before computation
  EXPECT_FALSE(tarjan.has_computation());
  EXPECT_EQ(tarjan.get_graph(), nullptr);

  // After computation
  auto sccs = tarjan.connected_components(g);
  EXPECT_TRUE(tarjan.has_computation());
  EXPECT_EQ(tarjan.get_graph(), &g);
}

// ============================================================================
// Test: Move semantics
// ============================================================================
TEST(TarjanTest, MoveSemantics)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  add_arc(g, a, b);
  add_arc(g, b, a);

  Tarjan_Connected_Components<TestDigraph> tarjan1;
  auto sccs1 = tarjan1.connected_components(g);
  EXPECT_EQ(sccs1.size(), 1u);

  // Move constructor
  Tarjan_Connected_Components<TestDigraph> tarjan2(std::move(tarjan1));
  auto sccs2 = tarjan2.connected_components(g);
  EXPECT_EQ(sccs2.size(), 1u);

  // Move assignment
  Tarjan_Connected_Components<TestDigraph> tarjan3;
  tarjan3 = std::move(tarjan2);
  auto sccs3 = tarjan3.connected_components(g);
  EXPECT_EQ(sccs3.size(), 1u);
}

// ============================================================================
// Test: Custom arc filter
// ============================================================================
struct SkipWeightZeroArcs
{
  bool operator()(TestDigraph::Arc *a) const noexcept
  {
    return a->get_info() != 0;
  }
};

TEST(TarjanTest, CustomArcFilter)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);

  // Create cycle a -> b -> c -> a, but with weight 0 on c -> a
  add_arc(g, a, b, 1);
  add_arc(g, b, c, 1);
  add_arc(g, c, a, 0); // This arc will be filtered out

  // Without filter: 1 SCC
  Tarjan_Connected_Components<TestDigraph> tarjan1;
  EXPECT_EQ(tarjan1.num_connected_components(g), 1u);

  // With filter: 3 SCCs (c -> a is filtered, breaking the cycle)
  Tarjan_Connected_Components<TestDigraph, Out_Iterator, SkipWeightZeroArcs> tarjan2;
  EXPECT_EQ(tarjan2.num_connected_components(g), 3u);
}

// ============================================================================
// Test: Stress test with large graph
// ============================================================================
TEST(TarjanTest, LargeGraphStress)
{
  TestDigraph g;
  const size_t N = 1000;

  // Create N nodes
  std::vector<TestDigraph::Node*> nodes(N);
  for (size_t i = 0; i < N; ++i)
    nodes[i] = add_node(g, static_cast<int>(i));

  // Create a single large cycle
  for (size_t i = 0; i < N; ++i)
    add_arc(g, nodes[i], nodes[(i + 1) % N]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Should be 1 SCC
  EXPECT_EQ(tarjan.num_connected_components(g), 1u);

  // Should be strongly connected
  EXPECT_TRUE(tarjan.test_connectivity(g));

  // Should have a cycle
  EXPECT_TRUE(tarjan.has_cycle(g));
}

// ============================================================================
// Test: Multiple disjoint cycles
// ============================================================================
TEST(TarjanTest, MultipleDisjointCycles)
{
  TestDigraph g;
  const size_t NUM_CYCLES = 10;
  const size_t CYCLE_SIZE = 5;

  std::vector<TestDigraph::Node*> first_nodes;

  for (size_t c = 0; c < NUM_CYCLES; ++c)
  {
    std::vector<TestDigraph::Node*> cycle_nodes(CYCLE_SIZE);
    for (size_t i = 0; i < CYCLE_SIZE; ++i)
      cycle_nodes[i] = add_node(g, static_cast<int>(c * CYCLE_SIZE + i));

    first_nodes.push_back(cycle_nodes[0]);

    for (size_t i = 0; i < CYCLE_SIZE; ++i)
      add_arc(g, cycle_nodes[i], cycle_nodes[(i + 1) % CYCLE_SIZE]);
  }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Should have NUM_CYCLES SCCs
  EXPECT_EQ(tarjan.num_connected_components(g), NUM_CYCLES);

  // Should have cycles
  EXPECT_TRUE(tarjan.has_cycle(g));

  // Verify each cycle is an SCC
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  for (auto sz : sizes)
    EXPECT_EQ(sz, CYCLE_SIZE);
}

// ============================================================================
// Test: Path cycle verification
// ============================================================================
TEST(TarjanTest, CyclePathVerification)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, a);

  Tarjan_Connected_Components<TestDigraph> tarjan;
  Path<TestDigraph> path(g);
  bool found = tarjan.compute_cycle(g, path);

  ASSERT_TRUE(found);
  ASSERT_FALSE(path.is_empty());

  // Verify cycle property: first node == last node
  EXPECT_EQ(path.get_first_node(), path.get_last_node());

  // Verify all nodes in path are connected
  size_t path_len = 0;
  for (Path<TestDigraph>::Iterator it(path); it.has_curr(); it.next())
    ++path_len;

  // Path should have at least 2 nodes for a valid cycle (start + end being same)
  EXPECT_GE(path_len, 2u);
}

// ============================================================================
// Test: Complete digraph (all nodes connected to all others)
// ============================================================================
TEST(TarjanTest, CompleteDigraph)
{
  TestDigraph g;
  const size_t N = 10;

  std::vector<TestDigraph::Node*> nodes(N);
  for (size_t i = 0; i < N; ++i)
    nodes[i] = add_node(g, static_cast<int>(i));

  // Create complete digraph
  for (size_t i = 0; i < N; ++i)
    for (size_t j = 0; j < N; ++j)
      if (i != j)
        add_arc(g, nodes[i], nodes[j]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // All nodes in one SCC
  EXPECT_EQ(tarjan.num_connected_components(g), 1u);
  EXPECT_TRUE(tarjan.test_connectivity(g));
  EXPECT_TRUE(tarjan.has_cycle(g));
}

// ============================================================================
// Test: Reusing Tarjan instance for multiple graphs
// ============================================================================
TEST(TarjanTest, ReuseInstance)
{
  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Graph 1: simple cycle
  TestDigraph g1;
  auto a1 = add_node(g1, 1);
  auto b1 = add_node(g1, 2);
  add_arc(g1, a1, b1);
  add_arc(g1, b1, a1);

  EXPECT_EQ(tarjan.num_connected_components(g1), 1u);
  EXPECT_TRUE(tarjan.has_cycle(g1));

  // Graph 2: DAG
  TestDigraph g2;
  auto a2 = add_node(g2, 1);
  auto b2 = add_node(g2, 2);
  auto c2 = add_node(g2, 3);
  add_arc(g2, a2, b2);
  add_arc(g2, b2, c2);

  EXPECT_EQ(tarjan.num_connected_components(g2), 3u);
  EXPECT_FALSE(tarjan.has_cycle(g2));
  EXPECT_TRUE(tarjan.is_dag(g2));
}

// ============================================================================
// EXTENDED EXHAUSTIVE TESTS
// ============================================================================

// ============================================================================
// Test: Star graph (hub with many spokes) - outward
// ============================================================================
TEST(TarjanExtended, StarGraphOutward)
{
  TestDigraph g;
  const size_t N = 50;

  auto hub = add_node(g, 0);
  for (size_t i = 1; i <= N; ++i)
    {
      auto spoke = add_node(g, static_cast<int>(i));
      add_arc(g, hub, spoke); // hub -> spoke
    }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // N+1 SCCs (each node is its own SCC)
  EXPECT_EQ(tarjan.num_connected_components(g), N + 1);
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Star graph (hub with many spokes) - inward
// ============================================================================
TEST(TarjanExtended, StarGraphInward)
{
  TestDigraph g;
  const size_t N = 50;

  auto hub = add_node(g, 0);
  for (size_t i = 1; i <= N; ++i)
    {
      auto spoke = add_node(g, static_cast<int>(i));
      add_arc(g, spoke, hub); // spoke -> hub
    }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), N + 1);
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
}

// ============================================================================
// Test: Wheel graph (cycle with hub connected to all)
// ============================================================================
TEST(TarjanExtended, WheelGraph)
{
  TestDigraph g;
  const size_t N = 20;

  auto hub = add_node(g, 0);
  std::vector<TestDigraph::Node *> rim(N);

  for (size_t i = 0; i < N; ++i)
    {
      rim[i] = add_node(g, static_cast<int>(i + 1));
      add_arc(g, hub, rim[i]); // hub -> rim
      add_arc(g, rim[i], hub); // rim -> hub (bidirectional)
    }

  // Create rim cycle
  for (size_t i = 0; i < N; ++i)
    add_arc(g, rim[i], rim[(i + 1) % N]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // All nodes should be in one SCC
  EXPECT_EQ(tarjan.num_connected_components(g), 1u);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Binary tree DAG
// ============================================================================
TEST(TarjanExtended, BinaryTreeDAG)
{
  TestDigraph g;
  const size_t DEPTH = 6; // 2^6 - 1 = 63 nodes

  std::vector<TestDigraph::Node *> nodes;
  nodes.push_back(add_node(g, 0)); // root

  for (size_t level = 0; level < DEPTH - 1; ++level)
    {
      size_t start = (1 << level) - 1;
      size_t end = (1 << (level + 1)) - 1;

      for (size_t i = start; i < end && i < nodes.size(); ++i)
        {
          auto left = add_node(g, static_cast<int>(nodes.size()));
          nodes.push_back(left);
          add_arc(g, nodes[i], left);

          auto right = add_node(g, static_cast<int>(nodes.size()));
          nodes.push_back(right);
          add_arc(g, nodes[i], right);
        }
    }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Each node is its own SCC (DAG)
  EXPECT_EQ(tarjan.num_connected_components(g), nodes.size());
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
}

// ============================================================================
// Test: Multiple self-loops
// ============================================================================
TEST(TarjanExtended, MultipleSelfLoops)
{
  TestDigraph g;
  const size_t N = 20;

  for (size_t i = 0; i < N; ++i)
    {
      auto n = add_node(g, static_cast<int>(i));
      add_arc(g, n, n); // self-loop
    }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Each node is its own SCC (but each has a cycle)
  EXPECT_EQ(tarjan.num_connected_components(g), N);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_FALSE(tarjan.is_dag(g));
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Chain of SCCs
// ============================================================================
TEST(TarjanExtended, ChainOfSCCs)
{
  TestDigraph g;
  const size_t NUM_SCCS = 10;
  const size_t SCC_SIZE = 3;

  std::vector<TestDigraph::Node *> first_of_scc;

  for (size_t s = 0; s < NUM_SCCS; ++s)
    {
      std::vector<TestDigraph::Node *> scc_nodes(SCC_SIZE);
      for (size_t i = 0; i < SCC_SIZE; ++i)
        scc_nodes[i] = add_node(g, static_cast<int>(s * SCC_SIZE + i));

      first_of_scc.push_back(scc_nodes[0]);

      // Create cycle within SCC
      for (size_t i = 0; i < SCC_SIZE; ++i)
        add_arc(g, scc_nodes[i], scc_nodes[(i + 1) % SCC_SIZE]);

      // Connect to previous SCC
      if (s > 0)
        add_arc(g, first_of_scc[s - 1], scc_nodes[0]);
    }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), NUM_SCCS);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_FALSE(tarjan.test_connectivity(g));

  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  for (auto sz : sizes)
    EXPECT_EQ(sz, SCC_SIZE);
}

// ============================================================================
// Test: Diamond pattern SCCs
// ============================================================================
TEST(TarjanExtended, DiamondPatternSCCs)
{
  TestDigraph g;

  // Top SCC
  auto t1 = add_node(g, 1);
  auto t2 = add_node(g, 2);
  add_arc(g, t1, t2);
  add_arc(g, t2, t1);

  // Left SCC
  auto l1 = add_node(g, 3);
  auto l2 = add_node(g, 4);
  add_arc(g, l1, l2);
  add_arc(g, l2, l1);

  // Right SCC
  auto r1 = add_node(g, 5);
  auto r2 = add_node(g, 6);
  add_arc(g, r1, r2);
  add_arc(g, r2, r1);

  // Bottom SCC
  auto b1 = add_node(g, 7);
  auto b2 = add_node(g, 8);
  add_arc(g, b1, b2);
  add_arc(g, b2, b1);

  // Connect in diamond pattern
  add_arc(g, t1, l1);
  add_arc(g, t2, r1);
  add_arc(g, l2, b1);
  add_arc(g, r2, b2);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), 4u);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_FALSE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Bipartite-like structure (no cycles)
// ============================================================================
TEST(TarjanExtended, BipartiteDAG)
{
  TestDigraph g;
  const size_t LEFT_SIZE = 10;
  const size_t RIGHT_SIZE = 10;

  std::vector<TestDigraph::Node *> left(LEFT_SIZE);
  std::vector<TestDigraph::Node *> right(RIGHT_SIZE);

  for (size_t i = 0; i < LEFT_SIZE; ++i)
    left[i] = add_node(g, static_cast<int>(i));

  for (size_t i = 0; i < RIGHT_SIZE; ++i)
    right[i] = add_node(g, static_cast<int>(LEFT_SIZE + i));

  // All left -> all right
  for (size_t i = 0; i < LEFT_SIZE; ++i)
    for (size_t j = 0; j < RIGHT_SIZE; ++j)
      add_arc(g, left[i], right[j]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), LEFT_SIZE + RIGHT_SIZE);
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
}

// ============================================================================
// Test: Strongly connected bipartite (all edges both ways)
// ============================================================================
TEST(TarjanExtended, StronglyConnectedBipartite)
{
  TestDigraph g;
  const size_t LEFT_SIZE = 5;
  const size_t RIGHT_SIZE = 5;

  std::vector<TestDigraph::Node *> left(LEFT_SIZE);
  std::vector<TestDigraph::Node *> right(RIGHT_SIZE);

  for (size_t i = 0; i < LEFT_SIZE; ++i)
    left[i] = add_node(g, static_cast<int>(i));

  for (size_t i = 0; i < RIGHT_SIZE; ++i)
    right[i] = add_node(g, static_cast<int>(LEFT_SIZE + i));

  // All left <-> all right (bidirectional)
  for (size_t i = 0; i < LEFT_SIZE; ++i)
    for (size_t j = 0; j < RIGHT_SIZE; ++j)
      {
        add_arc(g, left[i], right[j]);
        add_arc(g, right[j], left[i]);
      }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), 1u);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Grid graph (no cycles, DAG)
// ============================================================================
TEST(TarjanExtended, GridDAG)
{
  TestDigraph g;
  const size_t ROWS = 5;
  const size_t COLS = 5;

  std::vector<std::vector<TestDigraph::Node *>> grid(ROWS,
      std::vector<TestDigraph::Node *>(COLS));

  for (size_t r = 0; r < ROWS; ++r)
    for (size_t c = 0; c < COLS; ++c)
      grid[r][c] = add_node(g, static_cast<int>(r * COLS + c));

  // Connect right and down only (DAG)
  for (size_t r = 0; r < ROWS; ++r)
    for (size_t c = 0; c < COLS; ++c)
      {
        if (c + 1 < COLS)
          add_arc(g, grid[r][c], grid[r][c + 1]);
        if (r + 1 < ROWS)
          add_arc(g, grid[r][c], grid[r + 1][c]);
      }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), ROWS * COLS);
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
}

// ============================================================================
// Test: Grid graph with back edges (cycles)
// ============================================================================
TEST(TarjanExtended, GridWithCycles)
{
  TestDigraph g;
  const size_t ROWS = 4;
  const size_t COLS = 4;

  std::vector<std::vector<TestDigraph::Node *>> grid(ROWS,
      std::vector<TestDigraph::Node *>(COLS));

  for (size_t r = 0; r < ROWS; ++r)
    for (size_t c = 0; c < COLS; ++c)
      grid[r][c] = add_node(g, static_cast<int>(r * COLS + c));

  // Connect in all 4 directions (bidirectional)
  for (size_t r = 0; r < ROWS; ++r)
    for (size_t c = 0; c < COLS; ++c)
      {
        if (c + 1 < COLS)
          {
            add_arc(g, grid[r][c], grid[r][c + 1]);
            add_arc(g, grid[r][c + 1], grid[r][c]);
          }
        if (r + 1 < ROWS)
          {
            add_arc(g, grid[r][c], grid[r + 1][c]);
            add_arc(g, grid[r + 1][c], grid[r][c]);
          }
      }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // All nodes in one SCC
  EXPECT_EQ(tarjan.num_connected_components(g), 1u);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));
}

// ============================================================================
// Test: Very deep linear chain (stress test for recursion)
// ============================================================================
TEST(TarjanExtended, DeepLinearChain)
{
  TestDigraph g;
  const size_t DEPTH = 500; // Deep enough to stress but not overflow

  std::vector<TestDigraph::Node *> nodes(DEPTH);
  for (size_t i = 0; i < DEPTH; ++i)
    nodes[i] = add_node(g, static_cast<int>(i));

  for (size_t i = 0; i + 1 < DEPTH; ++i)
    add_arc(g, nodes[i], nodes[i + 1]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), DEPTH);
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
}

// ============================================================================
// Test: Consistency between different connected_components overloads
// ============================================================================
TEST(TarjanExtended, ConsistencyBetweenOverloads)
{
  TestDigraph g;

  // Create a complex graph
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  auto d = add_node(g, 4);
  auto e = add_node(g, 5);

  // SCC1: {a, b, c}
  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, a);

  // SCC2: {d, e}
  add_arc(g, d, e);
  add_arc(g, e, d);

  // Inter-SCC arcs
  add_arc(g, b, d);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Method 1: Get sizes
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);

  // Method 2: Get node lists
  auto node_lists = tarjan.connected_components(g);

  // Method 3: Get blocks with arcs
  DynList<TestDigraph> blocks;
  DynList<TestDigraph::Arc *> inter_arcs;
  tarjan.connected_components(g, blocks, inter_arcs);

  // All should report same number of SCCs
  EXPECT_EQ(sizes.size(), node_lists.size());
  EXPECT_EQ(sizes.size(), blocks.size());
  EXPECT_EQ(sizes.size(), 2u);

  // Sizes should match
  size_t total_from_sizes = 0;
  for (auto sz : sizes)
    total_from_sizes += sz;

  size_t total_from_lists = 0;
  for (auto &list : node_lists)
    total_from_lists += list.size();

  size_t total_from_blocks = 0;
  for (auto &block : blocks)
    total_from_blocks += block.get_num_nodes();

  EXPECT_EQ(total_from_sizes, 5u);
  EXPECT_EQ(total_from_lists, 5u);
  EXPECT_EQ(total_from_blocks, 5u);

  // Inter-SCC arcs
  EXPECT_EQ(inter_arcs.size(), 1u);
}

// ============================================================================
// Test: Cycle detection vs SCC count consistency
// ============================================================================
TEST(TarjanExtended, CycleDetectionConsistency)
{
  // A graph has a cycle iff some SCC has size > 1 OR some node has self-loop
  TestDigraph g;

  // 10 isolated nodes (no cycles)
  for (int i = 0; i < 10; ++i)
    add_node(g, i);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_EQ(tarjan.num_connected_components(g), 10u);

  // All SCCs should have size 1
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  for (auto sz : sizes)
    EXPECT_EQ(sz, 1u);
}

// ============================================================================
// Test: Very large strongly connected component
// ============================================================================
TEST(TarjanExtended, LargeSCC)
{
  TestDigraph g;
  const size_t N = 2000;

  std::vector<TestDigraph::Node *> nodes(N);
  for (size_t i = 0; i < N; ++i)
    nodes[i] = add_node(g, static_cast<int>(i));

  // Create cycle: 0 -> 1 -> 2 -> ... -> N-1 -> 0
  for (size_t i = 0; i < N; ++i)
    add_arc(g, nodes[i], nodes[(i + 1) % N]);

  // Add some random cross-edges to make it more connected
  for (size_t i = 0; i < N; i += 7)
    add_arc(g, nodes[i], nodes[(i + N / 3) % N]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // All nodes in one SCC
  EXPECT_EQ(tarjan.num_connected_components(g), 1u);
  EXPECT_TRUE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));

  // Verify size
  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_EQ(sizes.size(), 1u);
  EXPECT_EQ(sizes.get_first(), N);
}

// ============================================================================
// Test: Many small SCCs
// ============================================================================
TEST(TarjanExtended, ManySmallSCCs)
{
  TestDigraph g;
  const size_t NUM_SCCS = 500;

  for (size_t i = 0; i < NUM_SCCS; ++i)
    {
      auto a = add_node(g, static_cast<int>(i * 2));
      auto b = add_node(g, static_cast<int>(i * 2 + 1));
      add_arc(g, a, b);
      add_arc(g, b, a);
    }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  EXPECT_EQ(tarjan.num_connected_components(g), NUM_SCCS);
  EXPECT_TRUE(tarjan.has_cycle(g));

  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  for (auto sz : sizes)
    EXPECT_EQ(sz, 2u);
}

// ============================================================================
// Test: Path finding in cycle
// ============================================================================
TEST(TarjanExtended, CyclePathDetails)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  auto d = add_node(g, 4);

  add_arc(g, a, b);
  add_arc(g, b, c);
  add_arc(g, c, d);
  add_arc(g, d, b); // cycle: b -> c -> d -> b

  Tarjan_Connected_Components<TestDigraph> tarjan;

  Path<TestDigraph> path(g);
  bool found = tarjan.compute_cycle(g, path);

  ASSERT_TRUE(found);

  // Verify cycle: first == last
  EXPECT_EQ(path.get_first_node(), path.get_last_node());

  // Verify path length (at least 2 nodes for a cycle representation)
  size_t len = 0;
  for (Path<TestDigraph>::Iterator it(path); it.has_curr(); it.next())
    ++len;
  EXPECT_GE(len, 2u);
}

// ============================================================================
// Test: Empty graph edge cases
// ============================================================================
TEST(TarjanExtended, EmptyGraphEdgeCases)
{
  TestDigraph g;
  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Multiple calls on empty graph should all work
  EXPECT_EQ(tarjan.num_connected_components(g), 0u);
  EXPECT_FALSE(tarjan.has_cycle(g));
  EXPECT_TRUE(tarjan.is_dag(g));
  EXPECT_TRUE(tarjan.test_connectivity(g));

  DynList<DynList<TestDigraph::Node *>> node_lists;
  tarjan.connected_components(g, node_lists);
  EXPECT_TRUE(node_lists.is_empty());

  DynList<size_t> sizes;
  tarjan.connected_components(g, sizes);
  EXPECT_TRUE(sizes.is_empty());
}

// ============================================================================
// Test: Graph with parallel arcs (multi-edges)
// ============================================================================
TEST(TarjanExtended, ParallelArcs)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);

  // Multiple arcs in same direction
  add_arc(g, a, b);
  add_arc(g, a, b);
  add_arc(g, a, b);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Still 2 SCCs (no back edge)
  EXPECT_EQ(tarjan.num_connected_components(g), 2u);
  EXPECT_FALSE(tarjan.has_cycle(g));

  // Add back edges
  add_arc(g, b, a);
  add_arc(g, b, a);

  Tarjan_Connected_Components<TestDigraph> tarjan2;
  EXPECT_EQ(tarjan2.num_connected_components(g), 1u);
  EXPECT_TRUE(tarjan2.has_cycle(g));
}

// ============================================================================
// Test: Tournament graph (complete asymmetric digraph)
// ============================================================================
TEST(TarjanExtended, TournamentGraph)
{
  TestDigraph g;
  const size_t N = 8;

  std::vector<TestDigraph::Node *> nodes(N);
  for (size_t i = 0; i < N; ++i)
    nodes[i] = add_node(g, static_cast<int>(i));

  // Tournament: for each pair (i,j) with i < j, add exactly one arc
  // Direction based on some pattern
  for (size_t i = 0; i < N; ++i)
    for (size_t j = i + 1; j < N; ++j)
      {
        if ((i + j) % 2 == 0)
          add_arc(g, nodes[i], nodes[j]);
        else
          add_arc(g, nodes[j], nodes[i]);
      }

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Tournament graphs are interesting - may or may not be strongly connected
  // Just verify the algorithm completes successfully
  size_t num_sccs = tarjan.num_connected_components(g);
  EXPECT_GE(num_sccs, 1u);
  EXPECT_LE(num_sccs, N);
}

// ============================================================================
// Test: Dense random-like graph
// ============================================================================
TEST(TarjanExtended, DenseGraph)
{
  TestDigraph g;
  const size_t N = 50;

  std::vector<TestDigraph::Node *> nodes(N);
  for (size_t i = 0; i < N; ++i)
    nodes[i] = add_node(g, static_cast<int>(i));

  // Add many edges based on a pattern
  for (size_t i = 0; i < N; ++i)
    for (size_t j = 0; j < N; ++j)
      if (i != j && (i * 7 + j * 3) % 5 < 3) // ~60% edge density
        add_arc(g, nodes[i], nodes[j]);

  Tarjan_Connected_Components<TestDigraph> tarjan;

  // Just verify it completes and gives consistent results
  auto sccs = tarjan.connected_components(g);
  size_t total = 0;
  for (auto &scc : sccs)
    total += scc.size();
  EXPECT_EQ(total, N);
}

// ============================================================================
// Test: Compute_Cycle_In_Digraph with no cycle
// ============================================================================
TEST(TarjanExtended, ComputeCycleNoCycle)
{
  TestDigraph g;
  auto a = add_node(g, 1);
  auto b = add_node(g, 2);
  auto c = add_node(g, 3);
  add_arc(g, a, b);
  add_arc(g, b, c);

  Compute_Cycle_In_Digraph<TestDigraph> finder;

  Path<TestDigraph> path(g);
  bool found = finder(g, path);
  EXPECT_FALSE(found);

  auto path2 = finder(g);
  EXPECT_TRUE(path2.is_empty());

  auto path3 = finder(g, a);
  EXPECT_TRUE(path3.is_empty());
}
