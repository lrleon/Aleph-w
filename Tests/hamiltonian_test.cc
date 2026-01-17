
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
 * @file hamiltonian_test.cc
 * @brief Comprehensive tests for Hamiltonian sufficiency testing (hamiltonian.H)
 * 
 * Tests Ore's theorem: For n ≥ 3, if deg(u) + deg(v) ≥ n for every pair
 * of non-adjacent vertices u and v, then the graph is Hamiltonian.
 */

#include <gtest/gtest.h>
#include <hamiltonian.H>
#include <tpl_graph.H>

using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

class HamiltonianUndirectedTest : public ::testing::Test
{
protected:
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  
  Graph g;
  
  Node* add_node(int val)
  {
    return g.insert_node(val);
  }
  
  Arc* add_edge(Node* n1, Node* n2)
  {
    return g.insert_arc(n1, n2, 1);
  }
  
  // Build complete graph Kn
  void build_complete_graph(int n)
  {
    std::vector<Node*> nodes;
    for (int i = 0; i < n; ++i)
      nodes.push_back(add_node(i));
    
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        add_edge(nodes[i], nodes[j]);
  }
};

class HamiltonianDigraphTest : public ::testing::Test
{
protected:
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Digraph::Node;
  using Arc = Digraph::Arc;
  
  Digraph g;
  
  Node* add_node(int val)
  {
    return g.insert_node(val);
  }
  
  Arc* add_arc(Node* src, Node* tgt)
  {
    return g.insert_arc(src, tgt, 1);
  }
};

// =============================================================================
// Undirected Graph Tests - Ore's Theorem
// =============================================================================

TEST_F(HamiltonianUndirectedTest, CompleteGraphK3)
{
  // K3: Each vertex has degree 2, n=3
  // For non-adjacent pairs (none exist), condition trivially satisfied
  build_complete_graph(3);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianUndirectedTest, CompleteGraphK4)
{
  // K4: Each vertex has degree 3, n=4
  // No non-adjacent pairs, so Ore's condition satisfied
  build_complete_graph(4);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianUndirectedTest, CompleteGraphK5)
{
  // K5: Each vertex has degree 4, n=5
  build_complete_graph(5);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianUndirectedTest, CompleteGraphK10)
{
  // Larger complete graph
  build_complete_graph(10);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianUndirectedTest, CycleGraph)
{
  // Cycle C5: degrees are all 2, n=5
  // Non-adjacent pairs: deg sum = 4 < 5, fails Ore
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  add_edge(n4, n5);
  add_edge(n5, n1);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  // Note: Graph IS Hamiltonian, but Ore's condition is NOT satisfied
  // This is expected - Ore gives sufficient but not necessary condition
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, PathGraph)
{
  // Path P4: 1-2-3-4
  // degrees: 1,2,2,1
  // Non-adjacent (1,3): 1+2=3 < 4, fails
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, HighDegreeGraph)
{
  // Graph where each vertex has degree ≥ n/2
  // This satisfies Dirac's condition (stronger than Ore)
  // n=4, each vertex has degree 2 (= n/2)
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  // Make it "almost complete" - missing one edge
  add_edge(n1, n2);
  add_edge(n1, n3);
  add_edge(n2, n3);
  add_edge(n2, n4);
  add_edge(n3, n4);
  // Missing: n1-n4
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  // n1 and n4 non-adjacent: deg(n1)=2, deg(n4)=2, sum=4 ≥ n=4 ✓
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianUndirectedTest, StarGraph)
{
  // Star graph: center connected to all, leaves not connected
  // Center degree = 4, leaf degree = 1
  // Non-adjacent leaves: 1+1=2 < 5, fails
  auto center = add_node(0);
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  add_edge(center, n1);
  add_edge(center, n2);
  add_edge(center, n3);
  add_edge(center, n4);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, PetersenGraphLike)
{
  // A non-Hamiltonian graph that also fails Ore
  // Simple 5-vertex graph with low connectivity
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  // Pentagon with some missing edges
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, NearlyCompleteGraph)
{
  // K5 minus one edge
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  auto n5 = add_node(5);
  
  // All edges except n1-n2
  add_edge(n1, n3); add_edge(n1, n4); add_edge(n1, n5);
  add_edge(n2, n3); add_edge(n2, n4); add_edge(n2, n5);
  add_edge(n3, n4); add_edge(n3, n5);
  add_edge(n4, n5);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  // n1 and n2 non-adjacent: deg(n1)=3, deg(n2)=3, sum=6 ≥ n=5 ✓
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianUndirectedTest, BipartiteK33)
{
  // Complete bipartite K3,3: IS Hamiltonian
  // Each vertex has degree 3, n=6
  // Non-adjacent pairs within same partition: 3+3=6 ≥ 6 ✓
  auto a1 = add_node(1);
  auto a2 = add_node(2);
  auto a3 = add_node(3);
  auto b1 = add_node(4);
  auto b2 = add_node(5);
  auto b3 = add_node(6);
  
  // Connect all a's to all b's
  add_edge(a1, b1); add_edge(a1, b2); add_edge(a1, b3);
  add_edge(a2, b1); add_edge(a2, b2); add_edge(a2, b3);
  add_edge(a3, b1); add_edge(a3, b2); add_edge(a3, b3);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  // Non-adjacent in same partition: 3+3=6 ≥ 6 ✓
  EXPECT_TRUE(test(g));
}

// =============================================================================
// Directed Graph Tests
// =============================================================================

TEST_F(HamiltonianDigraphTest, CompleteDigraph)
{
  // Complete directed graph: every pair has arcs in both directions
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2); add_arc(n2, n1);
  add_arc(n1, n3); add_arc(n3, n1);
  add_arc(n2, n3); add_arc(n3, n2);
  
  Test_Hamiltonian_Sufficiency<Digraph> test;
  // out-deg + in-deg = 4 for each, n=3
  // For digraph Ore: out(u) + in(v) ≥ n when no arc u→v
  EXPECT_TRUE(test(g));
}

TEST_F(HamiltonianDigraphTest, DirectedCycle)
{
  // Simple directed cycle 1→2→3→1
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2);
  add_arc(n2, n3);
  add_arc(n3, n1);
  
  Test_Hamiltonian_Sufficiency<Digraph> test;
  // out=1, in=1 for each
  // For pair without arc: out(u) + in(v) = 1+1 = 2 < n=3
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianDigraphTest, DirectedPath)
{
  // Path 1→2→3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2);
  add_arc(n2, n3);
  
  Test_Hamiltonian_Sufficiency<Digraph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianDigraphTest, TournamentGraph)
{
  // Tournament: directed complete graph (one arc between each pair)
  // 1→2, 1→3, 2→3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  
  add_arc(n1, n2);
  add_arc(n1, n3);
  add_arc(n2, n3);
  
  Test_Hamiltonian_Sufficiency<Digraph> test;
  // This tournament has Hamiltonian path but may not satisfy Ore
  // n1: out=2, in=0; n2: out=1, in=1; n3: out=0, in=2
  // Pair without arc 2→1: out(n2)+in(n1) = 1+0 = 1 < 3
  EXPECT_FALSE(test(g));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(HamiltonianUndirectedTest, TwoNodes)
{
  // n=2: Ore's theorem requires n >= 3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  add_edge(n1, n2);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  // Ore's theorem doesn't apply for n < 3
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, TwoNodesDisconnected)
{
  // n=2, disconnected: fails Ore (no edges)
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  // deg(n1)=deg(n2)=0, 0+0=0 < 2
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, LargeCompleteGraph)
{
  // Performance test with larger graph
  build_complete_graph(20);
  
  Test_Hamiltonian_Sufficiency<Graph> test;
  EXPECT_TRUE(test(g));
}

// =============================================================================
// Tests for Dirac's Condition
// =============================================================================

TEST_F(HamiltonianUndirectedTest, DiracCompleteGraphK4)
{
  // K4: each vertex has degree 3, n=4, need deg >= 2
  // 3 >= 2 ✓
  build_complete_graph(4);
  
  Test_Dirac_Condition<Graph> test;
  EXPECT_TRUE(test(g));
  EXPECT_EQ(test.min_required_degree(g), 2);
}

TEST_F(HamiltonianUndirectedTest, DiracCompleteGraphK5)
{
  // K5: each vertex has degree 4, n=5, need deg >= 3
  // 4 >= 3 ✓
  build_complete_graph(5);
  
  Test_Dirac_Condition<Graph> test;
  EXPECT_TRUE(test(g));
  EXPECT_EQ(test.min_required_degree(g), 3);
}

TEST_F(HamiltonianUndirectedTest, DiracCycleFails)
{
  // Cycle C5: each vertex has degree 2, n=5, need deg >= 3
  // 2 < 3 ✗
  const int n = 5;
  std::vector<Graph::Node*> nodes;
  for (int i = 0; i < n; ++i)
    nodes.push_back(add_node(i));
  
  for (int i = 0; i < n; ++i)
    add_edge(nodes[i], nodes[(i+1) % n]);
  
  Test_Dirac_Condition<Graph> test;
  EXPECT_FALSE(test(g));
  
  auto [min_deg, min_node] = test.find_min_degree_vertex(g);
  EXPECT_EQ(min_deg, 2);
}

TEST_F(HamiltonianUndirectedTest, DiracVsOre)
{
  // Graph where Ore passes but Dirac fails
  // Example: Path P4 with some extra edges
  // n=4, Ore needs deg(u)+deg(v) >= 4 for non-adjacent
  // Dirac needs deg(v) >= 2 for all
  
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  auto n3 = add_node(3);
  auto n4 = add_node(4);
  
  // Build a cycle (satisfies Dirac for n=4, need deg >= 2)
  add_edge(n1, n2);
  add_edge(n2, n3);
  add_edge(n3, n4);
  add_edge(n4, n1);
  
  Test_Dirac_Condition<Graph> dirac;
  Test_Hamiltonian_Sufficiency<Graph> ore;
  
  // Both should pass for a cycle of length 4
  EXPECT_TRUE(dirac(g));
  EXPECT_TRUE(ore(g));
}

TEST_F(HamiltonianUndirectedTest, DiracTwoNodes)
{
  // n=2: Dirac requires n >= 3
  auto n1 = add_node(1);
  auto n2 = add_node(2);
  add_edge(n1, n2);
  
  Test_Dirac_Condition<Graph> test;
  EXPECT_FALSE(test(g));
}

TEST_F(HamiltonianUndirectedTest, DiracImpliesOre)
{
  // If Dirac passes, Ore should also pass
  // Test with K6
  build_complete_graph(6);
  
  Test_Dirac_Condition<Graph> dirac;
  Test_Hamiltonian_Sufficiency<Graph> ore;
  
  // Dirac: deg = 5 >= 3 (n/2 = 3) ✓
  EXPECT_TRUE(dirac(g));
  // Ore should also pass (it's less restrictive)
  EXPECT_TRUE(ore(g));
}

// Dirac for Digraphs
TEST_F(HamiltonianDigraphTest, DiracCompleteTournament)
{
  // Complete digraph K4 (tournament)
  // Each vertex: in=out=3, n=4, need >= 2
  const int n = 4;
  std::vector<Digraph::Node*> nodes;
  for (int i = 0; i < n; ++i)
    nodes.push_back(add_node(i));
  
  // Add all directed edges (both directions)
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (i != j)
        add_arc(nodes[i], nodes[j]);
  
  Test_Dirac_Condition<Digraph> test;
  EXPECT_TRUE(test(g));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
