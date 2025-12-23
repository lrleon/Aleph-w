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
