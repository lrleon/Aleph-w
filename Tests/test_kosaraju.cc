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

/**
 * @file test_kosaraju.cc
 * @brief Tests for Test Kosaraju
 */

/**
 * GoogleTest suite for kosaraju.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Kosaraju's algorithm for strongly connected components.
 */

#include <gtest/gtest.h>

#include <kosaraju.H>
#include <tpl_graph.H>
#include <Tarjan.H>

using namespace Aleph;

// Graph types for tests
using GT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// ========== Helper Functions ==========

// Create a simple chain: 0 -> 1 -> 2 -> ... -> (n-1)
GT create_chain(size_t n) {
  GT g;
  std::vector<Node*> nodes;
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));
  for (size_t i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], static_cast<int>(i));
  return g;
}

// Create a cycle: 0 -> 1 -> 2 -> ... -> (n-1) -> 0
GT create_cycle(size_t n) {
  GT g;
  std::vector<Node*> nodes;
  for (size_t i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));
  for (size_t i = 0; i < n; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % n], static_cast<int>(i));
  return g;
}

// Count total nodes across all SCCs
size_t count_total_nodes(const DynList<DynList<Node*>>& sccs) {
  size_t count = 0;
  for (const auto& scc : sccs)
    count += scc.size();
  return count;
}

// ========== TEST 1: Empty Graph ==========
TEST(KosarajuTest, EmptyGraph) {
  GT g;

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 0u);
}

// ========== TEST 2: Single Node ==========
TEST(KosarajuTest, SingleNode) {
  GT g;
  g.insert_node(0);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 1u);
}

// ========== TEST 3: Single Node with Self-Loop ==========
TEST(KosarajuTest, SingleNodeWithSelfLoop) {
  GT g;
  Node* n = g.insert_node(0);
  g.insert_arc(n, n, 0);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 1u);
}

// ========== TEST 4: Two Disconnected Nodes ==========
TEST(KosarajuTest, TwoDisconnectedNodes) {
  GT g;
  g.insert_node(0);
  g.insert_node(1);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 2u);
  size_t total = count_total_nodes(sccs);
  ASSERT_EQ(total, 2u);
}

// ========== TEST 5: Simple Chain (No SCC) ==========
TEST(KosarajuTest, SimpleChain) {
  GT g = create_chain(5);

  auto sccs = kosaraju_connected_components(g);

  // Each node is its own SCC (no cycles)
  ASSERT_EQ(sccs.size(), 5u);
}

// ========== TEST 6: Simple Cycle (Single SCC) ==========
TEST(KosarajuTest, SimpleCycle) {
  GT g = create_cycle(5);

  auto sccs = kosaraju_connected_components(g);

  // All nodes form one SCC
  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 5u);
}

// ========== TEST 7: Two Separate Cycles ==========
TEST(KosarajuTest, TwoSeparateCycles) {
  GT g;

  // Cycle 1: 0 -> 1 -> 2 -> 0
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n0, 2);

  // Cycle 2: 3 -> 4 -> 3
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);
  g.insert_arc(n3, n4, 3);
  g.insert_arc(n4, n3, 4);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 2u);
  size_t total = count_total_nodes(sccs);
  ASSERT_EQ(total, 5u);
}

// ========== TEST 8: Two Cycles Connected by One Arc ==========
TEST(KosarajuTest, TwoCyclesConnectedByOneArc) {
  GT g;

  // Cycle 1: 0 -> 1 -> 0
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n0, 1);

  // Cycle 2: 2 -> 3 -> 2
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n3, n2, 3);

  // Cross arc: 1 -> 2 (makes two SCCs)
  g.insert_arc(n1, n2, 4);

  auto sccs = kosaraju_connected_components(g);

  // Still 2 SCCs because there's no path from cycle 2 back to cycle 1
  ASSERT_EQ(sccs.size(), 2u);
}

// ========== TEST 9: Two Cycles Connected Bidirectionally ==========
TEST(KosarajuTest, TwoCyclesConnectedBidirectionally) {
  GT g;

  // Cycle 1: 0 -> 1 -> 0
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n0, 1);

  // Cycle 2: 2 -> 3 -> 2
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n3, n2, 3);

  // Cross arcs in both directions
  g.insert_arc(n1, n2, 4);
  g.insert_arc(n3, n0, 5);

  auto sccs = kosaraju_connected_components(g);

  // Now all 4 nodes are in one SCC
  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 4u);
}

// ========== TEST 10: Classic Example from CLRS ==========
TEST(KosarajuTest, ClassicCLRSExample) {
  // This is the classic example from CLRS (Cormen et al.)
  // Graph with 8 nodes and specific SCCs
  GT g;

  Node* a = g.insert_node(0); // a
  Node* b = g.insert_node(1); // b
  Node* c = g.insert_node(2); // c
  Node* d = g.insert_node(3); // d
  Node* e = g.insert_node(4); // e
  Node* f = g.insert_node(5); // f
  Node* h = g.insert_node(6); // h (renamed from g to avoid conflict)
  Node* i = g.insert_node(7); // i (renamed from h)

  // Arcs
  g.insert_arc(a, b, 0);
  g.insert_arc(b, c, 1);
  g.insert_arc(c, a, 2); // SCC 1: {a, b, c}

  g.insert_arc(b, e, 3);
  g.insert_arc(c, d, 4);

  g.insert_arc(d, e, 5);
  g.insert_arc(e, f, 6);
  g.insert_arc(f, d, 7); // SCC 2: {d, e, f}

  g.insert_arc(f, h, 8);
  g.insert_arc(h, i, 9);
  g.insert_arc(i, h, 10); // SCC 3: {h, i}

  auto sccs = kosaraju_connected_components(g);

  // Should have 3 SCCs
  ASSERT_EQ(sccs.size(), 3u);
  ASSERT_EQ(count_total_nodes(sccs), 8u);
}

// ========== TEST 11: Subgraph Version ==========
TEST(KosarajuTest, SubgraphVersion) {
  GT g;

  // Create two SCCs
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // SCC 1: 0 <-> 1
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n0, 1);

  // SCC 2: 2 <-> 3
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n3, n2, 3);

  // Cross arc: 1 -> 2
  Arc* cross = g.insert_arc(n1, n2, 4);

  DynList<GT> blk_list;
  DynList<Arc*> arc_list;
  kosaraju_connected_components(g, blk_list, arc_list);

  ASSERT_EQ(blk_list.size(), 2u);
  ASSERT_EQ(arc_list.size(), 1u);
  ASSERT_EQ(arc_list.get_first(), cross);

  // Verify each subgraph
  for (auto& blk : blk_list)
    {
      ASSERT_EQ(blk.get_num_nodes(), 2u);
      ASSERT_EQ(blk.get_num_arcs(), 2u);
    }
}

// ========== TEST 12: Cross Arc List Correctness ==========
TEST(KosarajuTest, CrossArcListCorrectness) {
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // SCC 1: {0}
  // SCC 2: {1}
  // SCC 3: {2}

  Arc* a01 = g.insert_arc(n0, n1, 0);
  Arc* a12 = g.insert_arc(n1, n2, 1);
  Arc* a02 = g.insert_arc(n0, n2, 2);

  DynList<GT> blk_list;
  DynList<Arc*> arc_list;
  kosaraju_connected_components(g, blk_list, arc_list);

  ASSERT_EQ(blk_list.size(), 3u);
  ASSERT_EQ(arc_list.size(), 3u);

  // All arcs should be cross arcs
  bool found_a01 = false, found_a12 = false, found_a02 = false;
  for (auto a : arc_list) {
    if (a == a01) found_a01 = true;
    if (a == a12) found_a12 = true;
    if (a == a02) found_a02 = true;
  }

  ASSERT_TRUE(found_a01);
  ASSERT_TRUE(found_a12);
  ASSERT_TRUE(found_a02);
}

// ========== TEST 13: is_strongly_connected True Case ==========
TEST(KosarajuTest, IsStronglyConnectedTrue) {
  GT g = create_cycle(5);

  ASSERT_TRUE(is_strongly_connected(g));
}

// ========== TEST 14: is_strongly_connected False Case ==========
TEST(KosarajuTest, IsStronglyConnectedFalse) {
  GT g = create_chain(5);

  ASSERT_FALSE(is_strongly_connected(g));
}

// ========== TEST 15: is_strongly_connected Empty Graph ==========
TEST(KosarajuTest, IsStronglyConnectedEmpty) {
  GT g;

  ASSERT_TRUE(is_strongly_connected(g));
}

// ========== TEST 16: is_strongly_connected Single Node ==========
TEST(KosarajuTest, IsStronglyConnectedSingleNode) {
  GT g;
  g.insert_node(0);

  ASSERT_TRUE(is_strongly_connected(g));
}

// ========== TEST 17: kosaraju_scc_count ==========
TEST(KosarajuTest, SccCount) {
  GT g;

  // Create 4 separate SCCs
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n2, n3, 1);

  ASSERT_EQ(kosaraju_scc_count(g), 4u);
}

// ========== TEST 18: Functor Interface ==========
TEST(KosarajuTest, FunctorInterface) {
  GT g = create_cycle(3);

  Kosaraju_Connected_Components<GT> functor;

  // Test list version
  auto sccs = functor(g);
  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 3u);

  // Test subgraph version
  DynList<GT> blk_list;
  DynList<Arc*> arc_list;
  functor(g, blk_list, arc_list);
  ASSERT_EQ(blk_list.size(), 1u);
  ASSERT_EQ(arc_list.size(), 0u);
}

// ========== TEST 19: Large Chain ==========
TEST(KosarajuTest, LargeChain) {
  GT g = create_chain(100);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 100u);
}

// ========== TEST 20: Large Cycle ==========
TEST(KosarajuTest, LargeCycle) {
  GT g = create_cycle(100);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 100u);
}

// ========== TEST 21: Node Mapping Correctness ==========
TEST(KosarajuTest, NodeMappingCorrectness) {
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n0, 1);

  DynList<GT> blk_list;
  DynList<Arc*> arc_list;
  kosaraju_connected_components(g, blk_list, arc_list);

  ASSERT_EQ(blk_list.size(), 1u);

  // Verify mapping
  GT& blk = blk_list.get_first();
  for (typename GT::Node_Iterator it(blk); it.has_curr(); it.next())
    {
      auto blk_node = it.get_curr();
      auto orig_node = mapped_node<GT>(blk_node);
      ASSERT_NE(orig_node, nullptr);
      ASSERT_EQ(blk_node->get_info(), orig_node->get_info());
    }
}

// ========== TEST 22: Arc Mapping Correctness ==========
TEST(KosarajuTest, ArcMappingCorrectness) {
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Arc* a01 = g.insert_arc(n0, n1, 10);
  Arc* a10 = g.insert_arc(n1, n0, 20);

  DynList<GT> blk_list;
  DynList<Arc*> arc_list;
  kosaraju_connected_components(g, blk_list, arc_list);

  GT& blk = blk_list.get_first();

  // Verify arc count matches
  ASSERT_EQ(blk.get_num_arcs(), 2u);

  // Verify original arcs are mapped to block arcs
  auto blk_a01 = mapped_arc<GT>(a01);
  auto blk_a10 = mapped_arc<GT>(a10);
  ASSERT_NE(blk_a01, nullptr);
  ASSERT_NE(blk_a10, nullptr);

  // Verify the block arcs are different
  ASSERT_NE(blk_a01, blk_a10);
}

// ========== TEST 23: Diamond Graph ==========
TEST(KosarajuTest, DiamondGraph) {
  //     0
  //    / \
  //   1   2
  //    \ /
  //     3
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 2);
  g.insert_arc(n2, n3, 3);

  auto sccs = kosaraju_connected_components(g);

  // No cycles, each node is its own SCC
  ASSERT_EQ(sccs.size(), 4u);
}

// ========== TEST 24: Complete Digraph (K4) ==========
TEST(KosarajuTest, CompleteDigraph) {
  GT g;

  std::vector<Node*> nodes;
  for (int i = 0; i < 4; ++i)
    nodes.push_back(g.insert_node(i));

  // Add all directed edges
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      if (i != j)
        g.insert_arc(nodes[i], nodes[j], i * 10 + j);

  auto sccs = kosaraju_connected_components(g);

  // Complete digraph is strongly connected
  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 4u);
}

// ========== TEST 25: Multiple Self-Loops ==========
TEST(KosarajuTest, MultipleSelfLoops) {
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n0, 0);
  g.insert_arc(n1, n1, 1);
  g.insert_arc(n2, n2, 2);

  auto sccs = kosaraju_connected_components(g);

  // Each node is its own SCC (self-loops don't connect nodes)
  ASSERT_EQ(sccs.size(), 3u);
}

// ========== TEST 26: Tree Structure ==========
TEST(KosarajuTest, TreeStructure) {
  //      0
  //     / \
  //    1   2
  //   / \
  //  3   4
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 2);
  g.insert_arc(n1, n4, 3);

  auto sccs = kosaraju_connected_components(g);

  // Tree has no cycles, each node is its own SCC
  ASSERT_EQ(sccs.size(), 5u);
}

// ========== TEST 27: Comparison with Tarjan ==========
TEST(KosarajuTest, CompareWithTarjan) {
  GT g;

  // Create a moderately complex graph
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n0, 2);
  g.insert_arc(n2, n3, 3);
  g.insert_arc(n3, n4, 4);
  g.insert_arc(n4, n3, 5);

  auto kosaraju_sccs = kosaraju_connected_components(g);

  // Use Tarjan for comparison
  DynList<DynList<Node*>> tarjan_sccs;
  Tarjan_Connected_Components<GT>().connected_components(g, tarjan_sccs);

  // Both should find the same number of SCCs
  ASSERT_EQ(kosaraju_sccs.size(), tarjan_sccs.size());
}

// ========== TEST 28: Stress Test ==========
TEST(KosarajuTest, StressTest) {
  GT g;
  const size_t N = 500;

  std::vector<Node*> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  // Create a large cycle
  for (size_t i = 0; i < N; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % N], static_cast<int>(i));

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), N);
}

// ========== TEST 29: Graph with Isolated Nodes and SCCs ==========
TEST(KosarajuTest, IsolatedNodesAndSCCs) {
  GT g;

  // Isolated nodes
  g.insert_node(0);
  g.insert_node(1);

  // SCC: 2 <-> 3
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  g.insert_arc(n2, n3, 0);
  g.insert_arc(n3, n2, 1);

  auto sccs = kosaraju_connected_components(g);

  ASSERT_EQ(sccs.size(), 3u);
}

// ========== TEST 30: Parallel Arcs ==========
TEST(KosarajuTest, ParallelArcs) {
  GT g;

  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  // Multiple arcs in same direction
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n0, 2);

  auto sccs = kosaraju_connected_components(g);

  // Both nodes in same SCC (there's a cycle)
  ASSERT_EQ(sccs.size(), 1u);
  ASSERT_EQ(sccs.get_first().size(), 2u);
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

