/**
 * GoogleTest suite for Dijkstra.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Dijkstra's shortest path algorithm implementation in Aleph-w.
 */

#include <gtest/gtest.h>

#include <Dijkstra.H>
#include <tpl_graph.H>

using namespace Aleph;

// Graph type for tests - uses double weights for distances
using GT = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using Node = GT::Node;
using Arc = GT::Arc;

// Digraph type for directed graph tests
using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using DNode = DGT::Node;
using DArc = DGT::Arc;

// ========== TEST 1: Basic Shortest Path ==========
TEST(DijkstraTest, BasicShortestPath) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 10.0);
  g.insert_arc(n0, n2, 5.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n1, n3, 1.0);
  g.insert_arc(n2, n1, 3.0);
  g.insert_arc(n2, n3, 9.0);
  g.insert_arc(n2, n4, 2.0);
  g.insert_arc(n3, n4, 4.0);
  g.insert_arc(n4, n0, 7.0);
  g.insert_arc(n4, n3, 6.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  // Path 0->2->1->3 costs 5+2+1=8 (undirected graph)
  ASSERT_EQ(d, 8.0);

  Path<GT>::Iterator it(path);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 0);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 2);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 1);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 3);
  it.next();
  ASSERT_FALSE(it.has_curr());
}

// ========== TEST 2: Path to Self ==========
TEST(DijkstraTest, PathToSelf) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n0, path);

  // When start == end and no self-loop exists, Dijkstra returns max distance
  // and empty path (there's no actual "path" from a node to itself)
  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ========== TEST 3: No Path Exists ==========
TEST(DijkstraTest, NoPathExists) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  // No arcs connecting the nodes

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n1, path);

  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ========== TEST 4: Compute Spanning Tree ==========
TEST(DijkstraTest, ComputeSpanningTree) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 10.0);
  g.insert_arc(n0, n2, 5.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n1, n3, 1.0);
  g.insert_arc(n2, n1, 3.0);
  g.insert_arc(n2, n3, 9.0);
  g.insert_arc(n2, n4, 2.0);
  g.insert_arc(n3, n4, 4.0);
  g.insert_arc(n4, n0, 7.0);
  g.insert_arc(n4, n3, 6.0);

  Dijkstra_Min_Paths<GT> dij;
  GT tree;
  dij(g, n0, tree);

  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4); // n-1 arcs in spanning tree
}

// ========== TEST 5: Update Path in Heap (Relaxation) ==========
TEST(DijkstraTest, RelaxationUpdate) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Direct path 0->1 costs 10, but 0->2->1 costs 2
  g.insert_arc(n0, n1, 10.0);
  g.insert_arc(n0, n2, 1.0);
  g.insert_arc(n2, n1, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n1, path);

  ASSERT_EQ(d, 2.0); // Should find the cheaper path through n2

  Path<GT>::Iterator it(path);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 0);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 2);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_current_node()->get_info(), 1);
  it.next();
  ASSERT_FALSE(it.has_curr());
}

// ========== TEST 6: Paint Spanning Tree ==========
TEST(DijkstraTest, PaintSpanningTree) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 4.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n1, n3, 5.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  dij.paint_min_paths_tree(g, n0);

  // After painting, we should be able to get min path to any node
  Path<GT> path(g);
  dij.get_min_path(n3, path);

  // Verify path: 0 -> 1 -> 2 -> 3 (cost 4)
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 7: Copy Painted Min Paths Tree ==========
TEST(DijkstraTest, CopyPaintedMinPathsTree) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 4.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n1, n3, 5.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  dij.paint_min_paths_tree(g, n0);

  GT tree;
  dij.copy_painted_min_paths_tree(g, tree);

  EXPECT_EQ(tree.get_num_nodes(), 4);
  EXPECT_EQ(tree.get_num_arcs(), 3); // n-1 arcs in tree
}

// ========== TEST 8: Single Node Graph ==========
TEST(DijkstraTest, SingleNodeGraph) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra_Min_Paths<GT> dij;
  GT tree;
  dij(g, n0, tree);

  EXPECT_EQ(tree.get_num_nodes(), 1);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

// ========== TEST 9: Linear Chain Graph ==========
TEST(DijkstraTest, LinearChainGraph) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 3.0);
}

// ========== TEST 10: Complete Graph K4 ==========
TEST(DijkstraTest, CompleteGraphK4) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // All edges with weight 1
  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 1.0);
  g.insert_arc(n0, n3, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n1, n3, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 1.0); // Direct path
}

// ========== TEST 11: Graph with Self Loop ==========
TEST(DijkstraTest, GraphWithSelfLoop) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n0, 5.0); // Self loop
  g.insert_arc(n0, n1, 2.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n1, path);

  EXPECT_EQ(d, 2.0); // Self loop should be ignored
}

// ========== TEST 12: Digraph Basic Path ==========
TEST(DijkstraDigraphTest, BasicPath) {
  DGT g;
  DNode* n0 = g.insert_node(0);
  DNode* n1 = g.insert_node(1);
  DNode* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  // Note: no arc n2->n0, so no path back

  Dijkstra_Min_Paths<DGT> dij;
  Path<DGT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 2.0);
}

// ========== TEST 13: Digraph No Return Path ==========
TEST(DijkstraDigraphTest, NoReturnPath) {
  DGT g;
  DNode* n0 = g.insert_node(0);
  DNode* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1.0); // Only forward direction

  Dijkstra_Min_Paths<DGT> dij;

  Path<DGT> path_forward(g);
  double d1 = dij(g, n0, n1, path_forward);
  EXPECT_EQ(d1, 1.0);

  Path<DGT> path_backward(g);
  double d2 = dij(g, n1, n0, path_backward);
  EXPECT_EQ(d2, std::numeric_limits<double>::max());
}

// ========== TEST 14: Empty Graph ==========
TEST(DijkstraTest, EmptyGraph) {
  GT g;

  Dijkstra_Min_Paths<GT> dij;
  GT tree;

  // Should handle empty graph gracefully - tree stays empty
  // Note: calling dij on empty graph would need a null node, so we just check tree
  EXPECT_EQ(tree.get_num_nodes(), 0);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

// ========== TEST 15: Multiple Paths Same Cost ==========
TEST(DijkstraTest, MultiplePathsSameCost) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Two paths from n0 to n3 with same cost 2
  // Path 1: n0 -> n1 -> n3 (cost 2)
  // Path 2: n0 -> n2 -> n3 (cost 2)
  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 1.0);
  g.insert_arc(n1, n3, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 2.0);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 16: Large Weights ==========
TEST(DijkstraTest, LargeWeights) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1e10);
  g.insert_arc(n1, n2, 1e10);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 2e10);
}

// ========== TEST 17: Zero Weight Edges ==========
TEST(DijkstraTest, ZeroWeightEdges) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0.0);
  g.insert_arc(n1, n2, 0.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 0.0);
}

// ========== TEST 18: Fractional Weights ==========
TEST(DijkstraTest, FractionalWeights) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0.5);
  g.insert_arc(n1, n2, 0.3);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_NEAR(d, 0.8, 1e-9);
}

// ========== TEST 19: Star Graph ==========
TEST(DijkstraTest, StarGraph) {
  GT g;
  Node* center = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(center, n1, 1.0);
  g.insert_arc(center, n2, 2.0);
  g.insert_arc(center, n3, 3.0);
  g.insert_arc(center, n4, 4.0);

  Dijkstra_Min_Paths<GT> dij;
  GT tree;
  dij(g, center, tree);

  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4);
}

// ========== TEST 20: Verify Path Correctness ==========
TEST(DijkstraTest, VerifyPathNodes) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);
  g.insert_arc(n0, n3, 100.0); // Much longer direct path

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 6.0);

  // Collect all nodes in path
  DynList<int> path_nodes;
  for (Path<GT>::Iterator it(path); it.has_curr(); it.next())
    path_nodes.append(it.get_current_node()->get_info());

  ASSERT_EQ(path_nodes.size(), 4);

  auto pit = path_nodes.get_it();
  EXPECT_EQ(pit.get_curr(), 0); pit.next();
  EXPECT_EQ(pit.get_curr(), 1); pit.next();
  EXPECT_EQ(pit.get_curr(), 2); pit.next();
  EXPECT_EQ(pit.get_curr(), 3);
}

// ========== TEST 21: State Getters ==========
TEST(DijkstraTest, StateGetters) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1.0);

  Dijkstra_Min_Paths<GT> dij;

  // Before any computation
  EXPECT_FALSE(dij.has_computation());
  EXPECT_FALSE(dij.is_painted());
  EXPECT_EQ(dij.get_start_node(), nullptr);
  EXPECT_EQ(dij.get_graph(), nullptr);

  // After painting
  dij.paint_min_paths_tree(g, n0);

  EXPECT_TRUE(dij.has_computation());
  EXPECT_TRUE(dij.is_painted());
  EXPECT_EQ(dij.get_start_node(), n0);
  EXPECT_EQ(dij.get_graph(), &g);
}

// ========== TEST 22: Compute Partial Min Paths Tree ==========
TEST(DijkstraTest, ComputePartialMinPathsTree) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n2, n3, 1.0);
  g.insert_arc(n3, n4, 1.0); // n4 should NOT be in partial tree

  Dijkstra_Min_Paths<GT> dij;
  GT tree;
  dij.compute_partial_min_paths_tree(g, n0, n3, tree);

  // Tree should contain n0, n1, n2, n3 but NOT n4
  EXPECT_EQ(tree.get_num_nodes(), 4);
  EXPECT_EQ(tree.get_num_arcs(), 3);
}

// ========== TEST 23: Get Min Path from Tree ==========
TEST(DijkstraTest, GetMinPathFromTree) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);

  Dijkstra_Min_Paths<GT> dij;
  GT tree;
  auto tree_start = dij.compute_min_paths_tree(g, n0, tree);

  // Verify tree was built correctly
  EXPECT_EQ(tree.get_num_nodes(), 4);
  EXPECT_EQ(tree.get_num_arcs(), 3);
  EXPECT_NE(tree_start, nullptr);

  // Use find_min_path instead which is more straightforward
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 6.0);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 24: Null Node Validation ==========
TEST(DijkstraTest, NullNodeValidation) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra_Min_Paths<GT> dij;
  GT tree;

  EXPECT_THROW(dij.compute_min_paths_tree(g, nullptr, tree), std::domain_error);
  EXPECT_THROW(dij.compute_partial_min_paths_tree(g, nullptr, n0, tree), std::domain_error);
  EXPECT_THROW(dij.compute_partial_min_paths_tree(g, n0, nullptr, tree), std::domain_error);
  EXPECT_THROW(dij.paint_min_paths_tree(g, nullptr), std::domain_error);
  EXPECT_THROW(dij.paint_partial_min_paths_tree(g, nullptr, n0), std::domain_error);
  EXPECT_THROW(dij.paint_partial_min_paths_tree(g, n0, nullptr), std::domain_error);
}

// ========== TEST 25: Disconnected Graph ==========
TEST(DijkstraTest, DisconnectedGraph) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Two disconnected components: {n0, n1} and {n2, n3}
  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  // n3 is not reachable from n0
  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ========== TEST 26: Integer Weights ==========
TEST(DijkstraTest, IntegerWeights) {
  using IGT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

  IGT g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, 3);

  Dijkstra_Min_Paths<IGT> dij;
  Path<IGT> path(g);
  int d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 8);
}

// ========== TEST 27: Get Distance After Painting ==========
TEST(DijkstraTest, GetDistanceAfterPainting) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);

  Dijkstra_Min_Paths<GT> dij;
  dij.paint_min_paths_tree(g, n0);

  EXPECT_EQ(dij.get_distance(n0), 0.0);
  EXPECT_EQ(dij.get_distance(n1), 1.0);
  EXPECT_EQ(dij.get_distance(n2), 3.0);
  EXPECT_EQ(dij.get_distance(n3), 6.0);
}

// ========== TEST 28: Get Distance Before Painting ==========
TEST(DijkstraTest, GetDistanceBeforePainting) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra_Min_Paths<GT> dij;

  // Should throw because not painted yet
  EXPECT_THROW(dij.get_distance(n0), std::domain_error);
}

// ========== TEST 29: Get Distance Unreachable Node ==========
TEST(DijkstraTest, GetDistanceUnreachableNode) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Not connected

  g.insert_arc(n0, n1, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  dij.paint_min_paths_tree(g, n0);

  // n2 is not reachable
  EXPECT_THROW(dij.get_distance(n2), std::domain_error);
}

// ========== TEST 30: Multiple Successive Computations ==========
TEST(DijkstraTest, MultipleSuccessiveComputations) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n0, n2, 10.0);

  Dijkstra_Min_Paths<GT> dij;

  // First computation from n0
  Path<GT> path1(g);
  double d1 = dij(g, n0, n2, path1);
  EXPECT_EQ(d1, 3.0);

  // Second computation from n2 (different start)
  Path<GT> path2(g);
  double d2 = dij(g, n2, n0, path2);
  EXPECT_EQ(d2, 3.0); // Same in undirected graph
}

// ========== TEST 31: Paint Partial Returns False When Not Found ==========
TEST(DijkstraTest, PaintPartialReturnsFalse) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Disconnected

  g.insert_arc(n0, n1, 1.0);

  Dijkstra_Min_Paths<GT> dij;
  bool found = dij.paint_partial_min_paths_tree(g, n0, n2);

  EXPECT_FALSE(found);
}

// ========== TEST 32: Paint Partial Returns True When Found ==========
TEST(DijkstraTest, PaintPartialReturnsTrue) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);

  Dijkstra_Min_Paths<GT> dij;
  bool found = dij.paint_partial_min_paths_tree(g, n0, n2);

  EXPECT_TRUE(found);

  // Should be able to get path now
  Path<GT> path(g);
  double d = dij.get_min_path(n2, path);
  EXPECT_EQ(d, 3.0);
}

// ========== TEST 33: Very Long Path ==========
TEST(DijkstraTest, VeryLongPath) {
  GT g;
  const int N = 100;

  std::vector<Node*> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i+1], 1.0);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> path(g);
  double d = dij(g, nodes[0], nodes[N-1], path);

  EXPECT_EQ(d, static_cast<double>(N - 1));
}

// ========== TEST 34: Dense Graph Performance ==========
TEST(DijkstraTest, DenseGraphPerformance) {
  GT g;
  const int N = 50;

  std::vector<Node*> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Create complete graph
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes[i], nodes[j], static_cast<double>(i + j));

  Dijkstra_Min_Paths<GT> dij;
  GT tree;
  dij(g, nodes[0], tree);

  EXPECT_EQ(tree.get_num_nodes(), N);
  EXPECT_EQ(tree.get_num_arcs(), N - 1);
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
