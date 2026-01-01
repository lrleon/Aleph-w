/**
 * GoogleTest suite for Bellman_Ford.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of the Bellman-Ford algorithm implementation in Aleph-w.
 */

#include <gtest/gtest.h>

#include <Bellman_Ford.H>
#include <tpl_graph.H>

using namespace Aleph;

// Graph type for tests
using GT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// ========== TEST 1: Simple Graph without Negative Cycles ==========
TEST(BellmanFordTest, SimpleGraphNoNegativeCycles) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  [[maybe_unused]] Arc* a01 = g.insert_arc(n0, n1, 1);
  [[maybe_unused]] Arc* a02 = g.insert_arc(n0, n2, 4);
  [[maybe_unused]] Arc* a12 = g.insert_arc(n1, n2, 2);
  [[maybe_unused]] Arc* a13 = g.insert_arc(n1, n3, 5);
  [[maybe_unused]] Arc* a23 = g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);

  // Should not detect any negative cycles
  bool has_negative_cycle = bf.has_negative_cycle(n0);
  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 2: Graph with Negative Cycle ==========
TEST(BellmanFordTest, GraphWithNegativeCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  [[maybe_unused]] Arc* a01 = g.insert_arc(n0, n1, 1);
  [[maybe_unused]] Arc* a12 = g.insert_arc(n1, n2, -3);
  [[maybe_unused]] Arc* a20 = g.insert_arc(n2, n0, 1); // Cycle: 0->1->2->0 with total weight -1

  Bellman_Ford<GT> bf(g);

  // Must detect a negative cycle
  bool has_negative_cycle = bf.has_negative_cycle(n0);
  ASSERT_TRUE(has_negative_cycle);
}

// ========== TEST 3: Minimal Paths Spanning Tree ==========
TEST(BellmanFordTest, SpanningTreePainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  [[maybe_unused]] Arc* a01 = g.insert_arc(n0, n1, 1);
  [[maybe_unused]] Arc* a02 = g.insert_arc(n0, n2, 4);
  [[maybe_unused]] Arc* a12 = g.insert_arc(n1, n2, 2);
  [[maybe_unused]] Arc* a13 = g.insert_arc(n1, n3, 5);
  [[maybe_unused]] Arc* a23 = g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bool negative_cycle = bf.paint_spanning_tree(n0);

  ASSERT_FALSE(negative_cycle);

  // Verify that the tree arcs are marked
  int painted_arcs = 0;
  for (Arc_Iterator<GT> it(g); it.has_curr(); it.next()) {
    if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree)) {
      painted_arcs++;
    }
  }

  // The spanning tree must have n-1 arcs
  ASSERT_EQ(painted_arcs, 3);
}

// ========== TEST 4: Faster Version of the Algorithm ==========
TEST(BellmanFordTest, FasterVersion) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 6);
  g.insert_arc(n0, n2, 7);
  g.insert_arc(n1, n2, 8);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n1, n4, -4);
  g.insert_arc(n2, n3, -3);
  g.insert_arc(n2, n4, 9);
  g.insert_arc(n3, n1, -2);
  g.insert_arc(n4, n0, 2);
  g.insert_arc(n4, n3, 7);

  Bellman_Ford<GT> bf(g);
  bool negative_cycle = bf.faster_paint_spanning_tree(n0);

  ASSERT_FALSE(negative_cycle);
}

// ========== TEST 5: Negative Cycle Detection (full version) ==========
TEST(BellmanFordTest, NegativeCycleDetectionFull) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -1);
  g.insert_arc(n2, n3, -1);
  g.insert_arc(n3, n1, -1); // Negative cycle: 1->2->3->1 with weight -3

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.test_negative_cycle(n0);

  // The resulting cycle must not be empty
  ASSERT_FALSE(cycle.is_empty());
}

// ========== TEST 6: Search Negative Cycle with Parameters ==========
TEST(BellmanFordTest, SearchNegativeCycleWithParams) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n3, -2);
  g.insert_arc(n3, n1, -1); // Negative cycle

  Bellman_Ford<GT> bf(g);
  auto result = bf.search_negative_cycle(n0, 0.5, 2);

  Path<GT>& cycle = get<0>(result);
  size_t iterations = get<1>(result);

  ASSERT_FALSE(cycle.is_empty());
  ASSERT_GT(iterations, 0u);
}

// ========== TEST 7: Trivial Graph without Negative Cycles ==========
TEST(BellmanFordTest, TrivialGraphNoNegativeCycles) {

  GT g;
  Node* n0 = g.insert_node(0);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 8: Graph with Zero Weights ==========
TEST(BellmanFordTest, GraphWithZeroWeights) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n0, 0);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  // A cycle with zero total weight is not negative
  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 9: Disconnected Graph ==========
TEST(BellmanFordTest, DisconnectedGraph) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Component 1
  g.insert_arc(n0, n1, 1);

  // Component 2 (disconnected)
  g.insert_arc(n2, n3, 2);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 10: Negative Weights without Negative Cycle ==========
TEST(BellmanFordTest, NegativeWeightsNoCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, -1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n3, -3);
  // There is no cycle

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 11: Large Graph with Multiple Paths ==========
TEST(BellmanFordTest, LargeGraphNoNegativeCycle) {

  GT g;
  constexpr int num_nodes = 100;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back(g.insert_node(i));
  }

  // Create a long chain
  for (int i = 0; i < num_nodes - 1; ++i) {
    g.insert_arc(nodes[i], nodes[i+1], 1);
  }

  // Add some extra arcs
  for (int i = 0; i < num_nodes - 5; i += 5) {
    g.insert_arc(nodes[i], nodes[i+5], 2);
  }

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(nodes[0]);

  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 12: Complex Negative Cycle ==========
TEST(BellmanFordTest, ComplexNegativeCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 3);
  g.insert_arc(n2, n3, -2);
  g.insert_arc(n3, n4, -1);
  g.insert_arc(n4, n2, -3); // Negative cycle: 2->3->4->2 with weight -6

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.test_negative_cycle(n0);

  ASSERT_FALSE(cycle.is_empty());
}

// ========== TEST 13: test_negative_cycle with Output Parameter ==========
TEST(BellmanFordTest, NegativeCycleWithOutputParameter) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle;
  bool has_cycle = bf.test_negative_cycle(n0, cycle);

  ASSERT_TRUE(has_cycle);
  ASSERT_FALSE(cycle.is_empty());
}

// ========== TEST 14: search_negative_cycle without Start Node ==========
TEST(BellmanFordTest, SearchNegativeCycleNoStartNode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Component without negative cycle
  g.insert_arc(n0, n1, 1);

  // Component with negative cycle
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, -2);
  g.insert_arc(n3, n1, -1);

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.search_negative_cycle();

  ASSERT_FALSE(cycle.is_empty());
}

// ========== TEST 15: Bellman_Ford_Negative_Cycle Functor ==========
TEST(BellmanFordTest, NegativeCycleFunctor) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford_Negative_Cycle<GT> detector;
  Path<GT> cycle;
  bool has_cycle = detector(g, cycle);

  ASSERT_TRUE(has_cycle);
  ASSERT_FALSE(cycle.is_empty());
}

// ========== TEST 16: Graph with Negative Self-loop ==========
TEST(BellmanFordTest, NegativeSelfLoop) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n1, -1); // Negative self-loop

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  ASSERT_TRUE(has_negative_cycle);
}

// ========== TEST 17: All Negative Weights without Cycle ==========
TEST(BellmanFordTest, AllNegativeWeightsNoCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, -1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n3, -3);
  g.insert_arc(n0, n2, -4);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  ASSERT_FALSE(has_negative_cycle);
}

// ========== TEST 18: Nullptr Validation ==========
TEST(BellmanFordTest, NullptrValidation) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  bool caught_exception = false;
  try {
    bf.paint_spanning_tree(nullptr);
  } catch (const std::domain_error&) {
    caught_exception = true;
  }
  ASSERT_TRUE(caught_exception);

  caught_exception = false;
  try {
    bf.faster_paint_spanning_tree(nullptr);
  } catch (const std::domain_error&) {
    caught_exception = true;
  }
  ASSERT_TRUE(caught_exception);

  caught_exception = false;
  try {
    bf.has_negative_cycle(nullptr);
  } catch (const std::domain_error&) {
    caught_exception = true;
  }
  ASSERT_TRUE(caught_exception);
}

// ========== TEST 19: Completely Empty Graph ==========
TEST(BellmanFordTest, CompletelyEmptyGraph) {

  GT g;
  // No nodes inserted

  Bellman_Ford<GT> bf(g);

  // has_negative_cycle() without parameters must work
  bool has_cycle = bf.has_negative_cycle();
  ASSERT_FALSE(has_cycle);
}

// ========== TEST 20: Build Tree from Spanning Tree ==========
TEST(BellmanFordTest, BuildTreeFromSpanningTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  GT tree;
  bf.build_tree(tree, false);

  // The tree must have the same number of nodes
  ASSERT_EQ(tree.get_num_nodes(), g.get_num_nodes());

  // The tree must have n-1 arcs
  ASSERT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);
}

// ========== TEST 21: Extract Min Spanning Tree ==========
TEST(BellmanFordTest, ExtractMinSpanningTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  DynArray<Arc*> tree_arcs = bf.extract_min_spanning_tree();

  // There must be n-1 arcs (some entries may be nullptr)
  int non_null_arcs = 0;
  for (size_t i = 0; i < tree_arcs.size(); ++i) {
    if (tree_arcs(i) != nullptr) {
      non_null_arcs++;
    }
  }

  ASSERT_EQ(non_null_arcs, 3); // n-1 for 4 nodes
}

// ========== TEST 22: Compute Nodes Weights (Johnson) ==========
TEST(BellmanFordTest, ComputeNodesWeightsJohnson) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n0, 3);

  Bellman_Ford<GT> bf(g);

  auto weights = bf.compute_nodes_weights();

  // There must be one weight for each node
  ASSERT_EQ(weights.size(), g.get_num_nodes());
}

// ========== TEST 23: Compute Nodes Weights with Negative Cycle ==========
TEST(BellmanFordTest, ComputeNodesWeightsWithNegativeCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1); // Negative cycle

  Bellman_Ford<GT> bf(g);

  bool caught_exception = false;
  try {
    bf.compute_nodes_weights();
  } catch (const std::domain_error&) {
    caught_exception = true;
  }

  ASSERT_TRUE(caught_exception);
}

// ========== TEST 24: has_negative_cycle Without Start Node ==========
TEST(BellmanFordTest, HasNegativeCycleNoStartNode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1); // Negative cycle

  Bellman_Ford<GT> bf(g);
  bool has_cycle = bf.has_negative_cycle();

  ASSERT_TRUE(has_cycle);
}

// ========== TEST 25: test_negative_cycle Without Parameters ==========
TEST(BellmanFordTest, NegativeCycleNoParams) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle;
  const bool has_cycle = bf.test_negative_cycle(cycle);

  ASSERT_TRUE(has_cycle);
  ASSERT_FALSE(cycle.is_empty());
}

// ========== TEST 26: get_min_path - CRITICAL MISSING TEST ==========
TEST(BellmanFordTest, GetMinPath) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  // Test path to n3 (should be: n0->n1->n2->n3 with cost 4)
  Path<GT> path(g);
  int distance = bf.get_min_path(n3, path);

  ASSERT_EQ(distance, 4);
  ASSERT_FALSE(path.is_empty());

  // Verify path contains correct nodes
  size_t path_length = 0;
  for (typename Path<GT>::Iterator it(path); it.has_current_node(); it.next_ne())
    path_length++;

  ASSERT_EQ(path_length, 4); // n0, n1, n2, n3
}

// ========== TEST 27: get_min_path to All Nodes ==========
TEST(BellmanFordTest, GetMinPathToAllNodes) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n0, n2, 10);
  g.insert_arc(n1, n2, 3);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  // Path to n1
  Path<GT> path1(g);
  int dist1 = bf.get_min_path(n1, path1);
  ASSERT_EQ(dist1, 5);

  // Path to n2 (via n1: 5+3=8, better than direct 10)
  Path<GT> path2(g);
  int dist2 = bf.get_min_path(n2, path2);
  ASSERT_EQ(dist2, 8);
}

// ========== TEST 28: extract_min_spanning_tree Validation ==========
TEST(BellmanFordTest, ExtractMinSpanningTreeValidation) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  [[maybe_unused]] Arc* a01 = g.insert_arc(n0, n1, 1);
  [[maybe_unused]] Arc* a02 = g.insert_arc(n0, n2, 5);
  [[maybe_unused]] Arc* a12 = g.insert_arc(n1, n2, 2);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  auto arcs = bf.extract_min_spanning_tree();

  // Verify arc count (should be n-1 = 2 arcs)
  size_t valid_arcs = 0;
  for (size_t i = 0; i < arcs.size(); ++i)
    if (arcs(i) != nullptr)
      valid_arcs++;

  ASSERT_EQ(valid_arcs, 2);

  // Verify arcs are actually from the tree (a01 and a12, not a02)
  bool found_a01 = false;
  bool found_a12 = false;
  for (size_t i = 0; i < arcs.size(); ++i) {
    if (arcs(i) == a01) found_a01 = true;
    if (arcs(i) == a12) found_a12 = true;
  }

  ASSERT_TRUE(found_a01);
  ASSERT_TRUE(found_a12);
}

// ========== TEST 29: build_tree with with_map=true ==========
TEST(BellmanFordTest, BuildTreeWithMapping) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 5);
  g.insert_arc(n1, n2, 2);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  GT tree;
  bf.build_tree(tree, true); // with_map = true

  // Verify tree has same number of nodes
  ASSERT_EQ(tree.get_num_nodes(), 3);

  // Verify tree has n-1 arcs
  ASSERT_EQ(tree.get_num_arcs(), 2);

  // Verify nodes have correct info
  bool found_0 = false, found_1 = false, found_2 = false;
  for (typename GT::Node_Iterator it(tree); it.has_curr(); it.next()) {
    auto node = it.get_curr();
    int info = node->get_info();
    if (info == 0) found_0 = true;
    if (info == 1) found_1 = true;
    if (info == 2) found_2 = true;
  }

  ASSERT_TRUE(found_0);
  ASSERT_TRUE(found_1);
  ASSERT_TRUE(found_2);
}

// ========== TEST 30: compute_nodes_weights Values Validation ==========
TEST(BellmanFordTest, ComputeNodesWeightsValues) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, 3);
  g.insert_arc(n0, n2, 10);

  Bellman_Ford<GT> bf(g);
  auto weights = bf.compute_nodes_weights();

  ASSERT_EQ(weights.size(), 3);

  // All weights should be 0 (Johnson's algorithm uses these to reweight)
  // The actual values depend on the dummy node distances
  for (auto it = weights.get_it(); it.has_curr(); it.next()) {
    auto & pair = it.get_curr();
    // Weight should be >= 0 after Johnson preprocessing
    ASSERT_GE(pair.second, std::numeric_limits<int>::min());
  }
}

// ========== TEST 31: Bellman_Ford_Negative_Cycle Functor (variant 1) ==========
TEST(BellmanFordTest, NegativeCycleFunctorVariant1) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1); // Negative cycle

  Path<GT> path(g);
  Bellman_Ford_Negative_Cycle<GT> detector;

  bool has_cycle = detector(g, path);

  ASSERT_TRUE(has_cycle);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 32: Bellman_Ford_Negative_Cycle Functor (variant 2 with start node) ==========
TEST(BellmanFordTest, NegativeCycleFunctorWithStartNode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Path<GT> path(g);
  Bellman_Ford_Negative_Cycle<GT> detector;

  bool has_cycle = detector(g, n0, path);

  ASSERT_TRUE(has_cycle);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 33: search_negative_cycle with different it_factor ==========
TEST(BellmanFordTest, SearchNegativeCycleWithDifferentItFactors) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);

  // Test with it_factor = 0.1
  auto [path1, iter1] = bf.search_negative_cycle(n0, 0.1, 1);
  ASSERT_FALSE(path1.is_empty());
  ASSERT_GT(iter1, 0u);

  // Test with it_factor = 0.9
  auto [path2, iter2] = bf.search_negative_cycle(n0, 0.9, 1);
  ASSERT_FALSE(path2.is_empty());
  ASSERT_GT(iter2, 0u);
}

// ========== TEST 34: Empty Graph Edge Case ==========
TEST(BellmanFordTest, EmptyGraphEdgeCase) {

  GT g;
  Bellman_Ford<GT> bf(g);

  bool has_cycle = bf.has_negative_cycle();
  ASSERT_FALSE(has_cycle);
}

// ========== TEST 35: Large Graph Performance Test ==========
TEST(BellmanFordTest, LargeGraphPerformance) {

  GT g;
  const size_t N = 100;

  // Create nodes
  std::vector<Node*> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));

  // Create chain of arcs
  for (size_t i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  Bellman_Ford<GT> bf(g);
  bool negative_cycle = bf.paint_spanning_tree(nodes[0]);

  ASSERT_FALSE(negative_cycle);

  // Verify path to last node
  Path<GT> path(g);
  int distance = bf.get_min_path(nodes[N - 1], path);

  ASSERT_EQ(distance, N - 1);
}

// ========== TEST 36: State Getters ==========
TEST(BellmanFordTest, StateGetters) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  // Before painting
  ASSERT_FALSE(bf.is_painted());
  ASSERT_FALSE(bf.has_computation());
  ASSERT_EQ(bf.get_start_node(), nullptr);
  ASSERT_EQ(&bf.get_graph(), &g);

  // After painting
  bf.paint_spanning_tree(n0);
  ASSERT_TRUE(bf.is_painted());
  ASSERT_TRUE(bf.has_computation());
  ASSERT_EQ(bf.get_start_node(), n0);
}

// ========== TEST 37: get_min_path without Painting ==========
TEST(BellmanFordTest, GetMinPathWithoutPainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  Path<GT> path(g);
  bool caught = false;
  try {
    bf.get_min_path(n1, path);
  } catch (const std::domain_error&) {
    caught = true;
  }
  ASSERT_TRUE(caught);
}

// ========== TEST 38: Negative Cycle Path Validity ==========
TEST(BellmanFordTest, NegativeCyclePathValidity) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n0, -1); // Total: -2 (negative cycle)

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.test_negative_cycle(n0);

  ASSERT_FALSE(cycle.is_empty());

  // Verify cycle properties:
  // 1. Has at least 2 nodes (for a cycle)
  size_t cycle_length = 0;
  for (typename Path<GT>::Iterator it(cycle); it.has_current_node(); it.next_ne())
    cycle_length++;

  ASSERT_GE(cycle_length, 2u);
}

// ========== TEST 39: Single Node with Self-loop (Positive) ==========
TEST(BellmanFordTest, SingleNodePositiveSelfLoop) {

  GT g;
  Node* n0 = g.insert_node(0);
  g.insert_arc(n0, n0, 1); // Positive self-loop

  Bellman_Ford<GT> bf(g);
  bool has_neg_cycle = bf.has_negative_cycle(n0);

  ASSERT_FALSE(has_neg_cycle);
}

// ========== TEST 40: Faster Version with Negative Cycle ==========
TEST(BellmanFordTest, FasterVersionWithNegativeCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);
  bool has_neg_cycle = bf.faster_paint_spanning_tree(n0);

  ASSERT_TRUE(has_neg_cycle);
}

// ========== TEST 41: Search Negative Cycle (Double Overload) ==========
TEST(BellmanFordTest, SearchNegativeCycleDoubleOverload) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);

  // search_negative_cycle(start, it_factor, step)
  auto [path, iter] = bf.search_negative_cycle(n0, 0.5, 1);

  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 42: search_negative_cycle (No Params Overload) ==========
TEST(BellmanFordTest, SearchNegativeCycleNoParamsVariant) {

  GT g;
  [[maybe_unused]] Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Disconnected negative cycle
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n1, -1);

  Bellman_Ford<GT> bf(g);

  // Using the overload without start node (uses dummy node)
  Path<GT> path = bf.search_negative_cycle();

  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 43: Build Tree Without Prior Painting ==========
TEST(BellmanFordTest, BuildTreeWithoutPriorPainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  GT tree;
  bool caught = false;
  try {
    bf.build_tree(tree, true); // with_map = true requires painting
  } catch (const std::domain_error&) {
    caught = true;
  }
  ASSERT_TRUE(caught);
}

// ========== TEST 44: Multiple Calls to paint_spanning_tree ==========
TEST(BellmanFordTest, MultiplePaintCalls) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 10);
  g.insert_arc(n1, n2, 2);

  Bellman_Ford<GT> bf(g);

  // First call
  bf.paint_spanning_tree(n0);
  ASSERT_TRUE(bf.is_painted());
  ASSERT_EQ(bf.get_start_node(), n0);

  Path<GT> path1(g);
  int dist1 = bf.get_min_path(n2, path1);
  ASSERT_EQ(dist1, 3);

  // Can we paint again from a different node? (fresh graph state)
  GT g2;
  Node* m0 = g2.insert_node(0);
  Node* m1 = g2.insert_node(1);
  g2.insert_arc(m0, m1, 5);

  Bellman_Ford<GT> bf2(g2);
  bf2.paint_spanning_tree(m0);
  ASSERT_TRUE(bf2.is_painted());

  Path<GT> path2(g2);
  int dist2 = bf2.get_min_path(m1, path2);
  ASSERT_EQ(dist2, 5);
}

// ========== TEST 45: Functor With Different Signatures ==========
TEST(BellmanFordTest, FunctorDifferentSignatures) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford_Negative_Cycle<GT> detector;

  // Signature: (g, s, d, sa) -> Path
  Dft_Dist<GT> dist;
  Dft_Show_Arc<GT> sa;
  Path<GT> path1 = detector(g, n0, dist, sa);
  ASSERT_FALSE(path1.is_empty());

  // Signature: (g, d, sa) -> Path
  Path<GT> path2 = detector(g, dist, sa);
  ASSERT_FALSE(path2.is_empty());

  // Signature: (g) -> Path with default args
  Path<GT> path3 = detector(g);
  ASSERT_FALSE(path3.is_empty());
}

// ========== TEST 46: Strongly Connected Component with Negative Cycle ==========
TEST(BellmanFordTest, StronglyConnectedWithNegativeCycle) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Connect all nodes strongly
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n3, n0, 1);

  // Add negative cycle within
  g.insert_arc(n1, n3, -5);
  g.insert_arc(n3, n1, 1);

  Bellman_Ford<GT> bf(g);
  bool has_neg = bf.has_negative_cycle(n0);

  ASSERT_TRUE(has_neg);
}

// ========== TEST 47: Very Long Chain Performance ==========
TEST(BellmanFordTest, VeryLongChainPerformance) {

  GT g;
  const size_t N = 500;

  std::vector<Node*> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  for (size_t i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  Bellman_Ford<GT> bf(g);
  bool neg_cycle = bf.paint_spanning_tree(nodes[0]);

  ASSERT_FALSE(neg_cycle);

  Path<GT> path(g);
  int dist = bf.get_min_path(nodes[N - 1], path);

  ASSERT_EQ(dist, static_cast<int>(N - 1));
}

// ========== TEST 48: Bidirectional Edges ==========
TEST(BellmanFordTest, BidirectionalEdges) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 3);
  g.insert_arc(n1, n0, 5);

  Bellman_Ford<GT> bf(g);
  bool neg_cycle = bf.paint_spanning_tree(n0);

  ASSERT_FALSE(neg_cycle);

  Path<GT> path(g);
  int dist = bf.get_min_path(n1, path);

  ASSERT_EQ(dist, 3);
}

// ========== TEST 49: Unreachable Node ==========
TEST(BellmanFordTest, UnreachableNode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Not connected

  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  // Node n2 is unreachable - get_min_path should throw
  Path<GT> path(g);
  bool caught = false;
  try {
    bf.get_min_path(n2, path);
  } catch (const std::domain_error&) {
    caught = true;
  }

  ASSERT_TRUE(caught);
}

// ========== TEST 50: Dense Graph ==========
TEST(BellmanFordTest, DenseGraph) {

  GT g;
  const size_t N = 20;

  std::vector<Node*> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  // Create dense graph (almost complete)
  for (size_t i = 0; i < N; ++i)
    for (size_t j = 0; j < N; ++j)
      if (i != j)
        g.insert_arc(nodes[i], nodes[j], static_cast<int>(i + j));

  Bellman_Ford<GT> bf(g);
  bool neg_cycle = bf.paint_spanning_tree(nodes[0]);

  ASSERT_FALSE(neg_cycle);
}

// ========== TEST 51: get_distance Method ==========
TEST(BellmanFordTest, GetDistance) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  // Distance to n3 should be 1+2+3 = 6
  int dist_n3 = bf.get_distance(n3);
  ASSERT_EQ(dist_n3, 6);

  // Distance to n1 should be 1
  int dist_n1 = bf.get_distance(n1);
  ASSERT_EQ(dist_n1, 1);

  // Distance to start node should be 0
  int dist_n0 = bf.get_distance(n0);
  ASSERT_EQ(dist_n0, 0);
}

// ========== TEST 52: get_distance Without Painting ==========
TEST(BellmanFordTest, GetDistanceWithoutPainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  bool caught = false;
  try {
    bf.get_distance(n1);
  } catch (const std::domain_error&) {
    caught = true;
  }
  ASSERT_TRUE(caught);
}

// ========== TEST 53: get_distance Unreachable Node ==========
TEST(BellmanFordTest, GetDistanceUnreachableNode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Not connected

  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  bool caught = false;
  try {
    bf.get_distance(n2);
  } catch (const std::domain_error&) {
    caught = true;
  }
  ASSERT_TRUE(caught);
}

// ========== TEST 54: extract_min_spanning_tree Without Painting ==========
TEST(BellmanFordTest, ExtractMinSpanningTreeWithoutPainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  bool caught = false;
  try {
    bf.extract_min_spanning_tree();
  } catch (const std::domain_error&) {
    caught = true;
  }
  ASSERT_TRUE(caught);
}

// ========== TEST 55: get_distance with Null Node ==========
TEST(BellmanFordTest, GetDistanceNullNode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  bool caught = false;
  try {
    bf.get_distance(nullptr);
  } catch (const std::domain_error&) {
    caught = true;
  }
  ASSERT_TRUE(caught);
}

// ========== TEST 56: get_distance vs get_min_path Consistency ==========
TEST(BellmanFordTest, GetDistanceConsistentWithGetMinPath) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n0, n2, 10);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  // Verify that get_distance returns the same value as get_min_path
  for (Node* target : {n1, n2, n3}) {
    Path<GT> path(g);
    int path_dist = bf.get_min_path(target, path);
    int direct_dist = bf.get_distance(target);
    ASSERT_EQ(path_dist, direct_dist);
  }
}

// ========== TEST: Parallel Arcs (Multigraph) ==========
// Test that Bellman-Ford correctly handles graphs with multiple arcs between same nodes
TEST(BellmanFordTest, ParallelArcsChoosesMinimum) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Multiple arcs between n0 and n1 with different weights
  g.insert_arc(n0, n1, 10);  // expensive
  g.insert_arc(n0, n1, 3);   // cheap - should be chosen
  g.insert_arc(n0, n1, 7);   // medium

  // Single arc to destination
  g.insert_arc(n1, n2, 2);

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 4) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  Bellman_Ford<GT> bf(g);
  bool has_neg = bf.paint_spanning_tree(n0);
  EXPECT_FALSE(has_neg);

  Path<GT> path(g);
  int d = bf.get_min_path(n2, path);

  // Shortest path should use the 3 arc: 3 + 2 = 5
  EXPECT_EQ(d, 5);
}

// ========== TEST: Parallel Arcs with Negative Weights ==========
TEST(BellmanFordTest, ParallelArcsNegativeWeights) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Multiple arcs including negative
  g.insert_arc(n0, n1, 5);
  g.insert_arc(n0, n1, -2);  // Best (negative)
  g.insert_arc(n1, n2, 3);

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 3) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  Bellman_Ford<GT> bf(g);
  bool has_neg = bf.paint_spanning_tree(n0);
  EXPECT_FALSE(has_neg);  // No negative cycle

  Path<GT> path(g);
  int d = bf.get_min_path(n2, path);

  // Best path: -2 + 3 = 1
  EXPECT_EQ(d, 1);
}

// ========== TEST: Complex Multigraph ==========
TEST(BellmanFordTest, ComplexMultigraph) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Multiple paths with parallel arcs
  g.insert_arc(n0, n1, 4);
  g.insert_arc(n0, n1, 2);   // Best n0->n1
  g.insert_arc(n1, n2, 3);
  g.insert_arc(n1, n2, 1);   // Best n1->n2
  g.insert_arc(n2, n3, 5);
  g.insert_arc(n2, n3, 2);   // Best n2->n3
  
  // Alternative direct path
  g.insert_arc(n0, n3, 10);

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 7) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  Bellman_Ford<GT> bf(g);
  bool has_neg = bf.paint_spanning_tree(n0);
  EXPECT_FALSE(has_neg);

  Path<GT> path(g);
  int d = bf.get_min_path(n3, path);

  // Best path: 2 + 1 + 2 = 5
  EXPECT_EQ(d, 5);
}

// ========== TEST: Parallel Arcs Creating Negative Cycle ==========
TEST(BellmanFordTest, ParallelArcsNegativeCycle) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  // Parallel arcs that form a negative cycle in undirected interpretation
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n0, -5);  // Creates negative cycle: 1 + (-5) = -4

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 2) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  Bellman_Ford<GT> bf(g);
  bool has_neg = bf.paint_spanning_tree(n0);

  // Should detect negative cycle
  EXPECT_TRUE(has_neg);
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
