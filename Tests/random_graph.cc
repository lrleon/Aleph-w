
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
 * @file random_graph.cc
 * @brief Tests for Random Graph
 */
#include <gtest/gtest.h>
#include <random_graph.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>

using namespace std;
using namespace Aleph;

// Simple graph type for testing (undirected)
using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

// ============================================================================
// Test helper functions
// ============================================================================

/// Verify that a graph has no parallel arcs (simple graph)
template <class GT>
bool is_simple_graph(const GT & g)
{
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr_ne();
      auto src = g.get_src_node(arc);
      auto tgt = g.get_tgt_node(arc);
      
      // Count arcs between src and tgt
      size_t count = 0;
      for (auto ait = g.get_arc_it(); ait.has_curr(); ait.next_ne())
        {
          auto a = ait.get_curr_ne();
          auto s = g.get_src_node(a);
          auto t = g.get_tgt_node(a);
          if ((s == src && t == tgt) || (s == tgt && t == src))
            ++count;
        }
      if (count > 1)
        return false;
    }
  return true;
}

/// Verify that all nodes have even degree (necessary for Eulerian)
template <class GT>
bool all_nodes_have_even_degree(const GT & g)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto node = it.get_curr_ne();
      if (g.get_num_arcs(node) % 2 != 0)
        return false;
    }
  return true;
}

/// Verify Ore's theorem condition for Hamiltonian graphs:
/// For all non-adjacent pairs u, v: deg(u) + deg(v) >= n
template <class GT>
bool satisfies_ore_condition(const GT & g)
{
  const size_t n = g.get_num_nodes();
  
  for (auto it1 = g.get_node_it(); it1.has_curr(); it1.next_ne())
    {
      auto u = it1.get_curr_ne();
      for (auto it2 = g.get_node_it(); it2.has_curr(); it2.next_ne())
        {
          auto v = it2.get_curr_ne();
          if (u == v)
            continue;
          
          // Check if u and v are adjacent
          bool adjacent = false;
          for (auto ait = g.get_arc_it(u); ait.has_curr(); ait.next_ne())
            {
              auto arc = ait.get_curr_ne();
              if (g.get_connected_node(arc, u) == v)
                {
                  adjacent = true;
                  break;
                }
            }
          
          if (!adjacent)
            {
              size_t deg_u = g.get_num_arcs(u);
              size_t deg_v = g.get_num_arcs(v);
              if (deg_u + deg_v < n)
                return false;
            }
        }
    }
  return true;
}

// ============================================================================
// Random_Graph tests (undirected graphs)
// ============================================================================

TEST(RandomGraph, SparseBasic)
{
  
  Random_Graph<Graph> gen(42);
  
  auto g = gen(static_cast<size_t>(10), static_cast<size_t>(15));
  
  EXPECT_TRUE(g.get_num_nodes() == 10);
  EXPECT_TRUE(g.get_num_arcs() <= 15); // May be less due to duplicate avoidance
  EXPECT_TRUE(is_simple_graph(g));
  
}

TEST(RandomGraph, SparseConnected)
{
  
  Random_Graph<Graph> gen(123);
  
  auto g = gen(static_cast<size_t>(20), static_cast<size_t>(30), true);
  
  EXPECT_TRUE(g.get_num_nodes() == 20);
  EXPECT_TRUE(test_connectivity(g));
  EXPECT_TRUE(is_simple_graph(g));
  
}

TEST(RandomGraph, SparseDisconnected)
{
  
  Random_Graph<Graph> gen(456);
  
  // With few arcs and no connectivity requirement, may be disconnected
  const auto g = gen(static_cast<size_t>(20), static_cast<size_t>(5), false);
  
  EXPECT_TRUE(g.get_num_nodes() == 20);
  EXPECT_TRUE(is_simple_graph(g));
  
}

TEST(RandomGraph, Probability)
{
  
  Random_Graph<Graph> gen(789);
  
  auto g = gen(15, 0.3);
  
  EXPECT_TRUE(g.get_num_nodes() == 15);
  EXPECT_TRUE(is_simple_graph(g));
  
  // With p=0.3 we expect roughly 0.3 * (15*14/2) = 31.5 arcs on average
  // but due to randomness it can vary significantly
  
}

TEST(RandomGraph, ProbabilityDense)
{
  
  Random_Graph<Graph> gen(101);
  
  auto g = gen(10, 0.8);
  
  EXPECT_TRUE(g.get_num_nodes() == 10);
  EXPECT_TRUE(is_simple_graph(g));
  
  // With p=0.8 we expect a fairly dense graph
  size_t max_arcs = 10 * 9 / 2; // 45
  EXPECT_TRUE(g.get_num_arcs() > max_arcs / 2); // Should have more than half
  
}

TEST(RandomGraph, InvalidProbability)
{
  
  Random_Graph<Graph> gen(999);
  
  bool caught_p_zero = false;
  try {
    gen(10, 0.0);
  } catch (const std::domain_error &) {
    caught_p_zero = true;
  }
  EXPECT_TRUE(caught_p_zero);
  (void)caught_p_zero;
  
  bool caught_p_negative = false;
  try {
    gen(10, -0.5);
  } catch (const std::domain_error &) {
    caught_p_negative = true;
  }
  EXPECT_TRUE(caught_p_negative);
  (void)caught_p_negative;
  
  bool caught_p_over_one = false;
  try {
    gen(10, 1.5);
  } catch (const std::domain_error &) {
    caught_p_over_one = true;
  }
  EXPECT_TRUE(caught_p_over_one);
  (void)caught_p_over_one;
  
}

TEST(RandomGraph, EulerianSparse)
{
  
  Random_Graph<Graph> gen(2024);
  
  auto g = gen.eulerian(size_t(12), size_t(20));
  
  EXPECT_TRUE(g.get_num_nodes() == 12);
  EXPECT_TRUE(test_connectivity(g));
  EXPECT_TRUE(all_nodes_have_even_degree(g));
  
}

TEST(RandomGraph, EulerianProbability)
{
  
  Random_Graph<Graph> gen(2025);
  
  auto g = gen.eulerian(10, 0.4);
  
  EXPECT_TRUE(g.get_num_nodes() == 10);
  EXPECT_TRUE(test_connectivity(g));
  EXPECT_TRUE(all_nodes_have_even_degree(g));
  
}

TEST(RandomGraph, Hamiltonian)
{
  
  Random_Graph<Graph> gen(3000);
  
  auto g = gen.sufficient_hamiltonian(8, 0.5);
  
  EXPECT_TRUE(g.get_num_nodes() == 8);
  EXPECT_TRUE(test_connectivity(g));
  EXPECT_TRUE(satisfies_ore_condition(g));
  
}

TEST(RandomGraph, DeterministicSeed)
{
  
  Random_Graph<Graph> gen1(12345);
  Random_Graph<Graph> gen2(12345);
  
  auto g1 = gen1(static_cast<size_t>(10), static_cast<size_t>(20));
  auto g2 = gen2(static_cast<size_t>(10), static_cast<size_t>(20));
  
  EXPECT_TRUE(g1.get_num_nodes() == g2.get_num_nodes());
  EXPECT_TRUE(g1.get_num_arcs() == g2.get_num_arcs());
  
}

TEST(RandomGraph, DifferentSeeds)
{
  
  Random_Graph<Graph> gen1(11111);
  Random_Graph<Graph> gen2(22222);
  
  auto g1 = gen1(static_cast<size_t>(15), static_cast<size_t>(30));
  auto g2 = gen2(static_cast<size_t>(15), static_cast<size_t>(30));
  
  // With different seeds, graphs should (almost certainly) differ
  // We can't guarantee they differ, but it's extremely unlikely they're identical
  EXPECT_EQ(g1.get_num_nodes(), 15u);
  EXPECT_EQ(g2.get_num_nodes(), 15u);
  EXPECT_TRUE(is_simple_graph(g1));
  EXPECT_TRUE(is_simple_graph(g2));
  
}

// Random_Digraph, and Random_Graph on every kind of graph, are checked by
// graph_exhaustive_generators_test.cc

// ============================================================================
// Edge case tests
// ============================================================================

TEST(RandomGraph, SingleNodeGraph)
{
  
  Random_Graph<Graph> gen(1);
  
  auto g = gen(static_cast<size_t>(1), static_cast<size_t>(0));
  
  EXPECT_TRUE(g.get_num_nodes() == 1);
  EXPECT_TRUE(g.get_num_arcs() == 0);
  
}

TEST(RandomGraph, ZeroNodesRejected)
{
  
  Random_Graph<Graph> gen(42);
  
  bool caught = false;
  try {
    gen(static_cast<size_t>(0), static_cast<size_t>(10));
  } catch (const std::domain_error &) {
    caught = true;
  }
  EXPECT_TRUE(caught);
  (void)caught;
  
}

TEST(RandomGraph, TwoNodeGraph)
{
  
  Random_Graph<Graph> gen(2);
  
  auto g = gen(static_cast<size_t>(2), static_cast<size_t>(1), true);
  
  EXPECT_TRUE(g.get_num_nodes() == 2);
  EXPECT_TRUE(g.get_num_arcs() >= 1); // At least 1 arc to be connected
  EXPECT_TRUE(test_connectivity(g));
  
}

TEST(RandomGraph, CompleteGraphLimit)
{
  
  Random_Graph<Graph> gen(100);
  
  // For 5 nodes, max arcs = 5*4/2 = 10
  auto g = gen(static_cast<size_t>(5), static_cast<size_t>(100)); // Request 100, but can only have 10
  
  EXPECT_TRUE(g.get_num_nodes() == 5);
  EXPECT_TRUE(g.get_num_arcs() <= 10);
  
}


// ============================================================================
// Custom initializer tests
// ============================================================================

struct NodeInitializer
{
  int counter = 0;
  void operator()(Graph & /* g */, Graph::Node * node)
  {
    node->get_info() = counter++;
  }
};

struct ArcInitializer
{
  int weight = 1;
  void operator()(Graph & /* g */, Graph::Arc * arc)
  {
    arc->get_info() = weight++;
  }
};

TEST(RandomGraph, CustomInitializers)
{

  NodeInitializer node_init;
  ArcInitializer arc_init;
  
  Random_Graph<Graph, NodeInitializer, ArcInitializer> gen(5555, node_init, arc_init);
  
  auto g = gen(static_cast<size_t>(5), static_cast<size_t>(6));
  
  // Verify nodes were initialized with sequential values
  bool nodes_initialized = true;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    if (auto node = it.get_curr_ne(); node->get_info() < 0 || node->get_info() >= 5)
        nodes_initialized = false;

  EXPECT_TRUE(nodes_initialized);
  
  // Verify arcs were initialized with sequential weights
  bool arcs_initialized = true;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      if (auto arc = it.get_curr_ne(); arc->get_info() < 1)
        arcs_initialized = false;
    }
  EXPECT_TRUE(arcs_initialized);
  
}
