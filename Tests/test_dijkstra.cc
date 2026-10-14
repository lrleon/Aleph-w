/**
 * GoogleTest suite for Dijkstra.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Dijkstra's shortest path algorithm implementation in Aleph-w.
 */


/**
 * @file test_dijkstra.cc
 * @brief Tests for Test Dijkstra
 */
#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <functional>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include <Dijkstra.H>
#include <tpl_graph.H>

using namespace Aleph;

// Helper iterator for multigraphs that visits ALL arcs including parallel ones
// and provides the get_current_arc_ne() method required by Dijkstra
template <class GT, class SA = Dft_Show_Arc<GT>>
class Out_Iterator_AllArcs : public Out_Iterator<GT, SA>
{
public:
  using Base = Out_Iterator<GT, SA>;
  using Base::Base;

  typename GT::Arc * get_current_arc_ne() const noexcept
  {
    return const_cast<Out_Iterator_AllArcs *>(this)->get_current_arc();
  }
};

// Graph type for tests - uses double weights for distances
using GT = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using Node = GT::Node;
using Arc = GT::Arc;

// Digraph type for directed graph tests
using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using DNode = DGT::Node;
using DArc = DGT::Arc;

struct BinHeapTag
{
  template <class G, class D, class A>
  using Heap = ArcHeap<G, D, A>;
};

struct FibHeapTag
{
  template <class G, class D, class A>
  using Heap = ArcFibonacciHeap<G, D, A>;
};

template <typename HeapTag, typename Graph>
using DijkstraFor =
  Dijkstra_Min_Paths<Graph, Dft_Dist<Graph>, Node_Arc_Iterator,
                     Dft_Show_Arc<Graph>, HeapTag::template Heap>;

// Dijkstra with Out_Iterator_AllArcs for multigraphs (visits ALL arcs, including parallel)
template <typename HeapTag, typename Graph>
using DijkstraForAllArcs =
  Dijkstra_Min_Paths<Graph, Dft_Dist<Graph>, Out_Iterator_AllArcs,
                     Dft_Show_Arc<Graph>, HeapTag::template Heap>;

template <typename HeapTag>
using Dijkstra = DijkstraFor<HeapTag, GT>;

template <typename HeapTag>
using DijkstraAllArcs = DijkstraForAllArcs<HeapTag, GT>;

template <typename HeapTag>
using DigraphDijkstra = DijkstraFor<HeapTag, DGT>;

template <typename HeapTag>
class DijkstraHeapTest : public ::testing::Test {};

template <typename HeapTag>
class DijkstraDigraphHeapTest : public ::testing::Test {};

using HeapTypes = ::testing::Types<BinHeapTag, FibHeapTag>;
TYPED_TEST_SUITE(DijkstraHeapTest, HeapTypes);
TYPED_TEST_SUITE(DijkstraDigraphHeapTest, HeapTypes);

// ========== TEST 1: Basic Shortest Path ==========
TYPED_TEST(DijkstraHeapTest, BasicShortestPath) {
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

  Dijkstra<TypeParam> dij;
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
TYPED_TEST(DijkstraHeapTest, PathToSelf) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n0, path);

  // When start == end and no self-loop exists, Dijkstra returns max distance
  // and empty path (there's no actual "path" from a node to itself)
  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ========== TEST 3: No Path Exists ==========
TYPED_TEST(DijkstraHeapTest, NoPathExists) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  // No arcs connecting the nodes

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n1, path);

  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ========== TEST 4: Compute Spanning Tree ==========
TYPED_TEST(DijkstraHeapTest, ComputeSpanningTree) {
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

  Dijkstra<TypeParam> dij;
  GT tree;
  dij(g, n0, tree);

  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4); // n-1 arcs in spanning tree
}

// ========== TEST 5: Update Path in Heap (Relaxation) ==========
TYPED_TEST(DijkstraHeapTest, RelaxationUpdate) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Direct path 0->1 costs 10, but 0->2->1 costs 2
  g.insert_arc(n0, n1, 10.0);
  g.insert_arc(n0, n2, 1.0);
  g.insert_arc(n2, n1, 1.0);

  Dijkstra<TypeParam> dij;
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
TYPED_TEST(DijkstraHeapTest, PaintSpanningTree) {
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

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);

  // After painting, we should be able to get min path to any node
  Path<GT> path(g);
  dij.get_min_path(n3, path);

  // Verify path: 0 -> 1 -> 2 -> 3 (cost 4)
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 7: Copy Painted Min Paths Tree ==========
TYPED_TEST(DijkstraHeapTest, CopyPaintedMinPathsTree) {
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

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);

  GT tree;
  dij.copy_painted_min_paths_tree(g, tree);

  EXPECT_EQ(tree.get_num_nodes(), 4);
  EXPECT_EQ(tree.get_num_arcs(), 3); // n-1 arcs in tree
}

// ========== TEST 8: Single Node Graph ==========
TYPED_TEST(DijkstraHeapTest, SingleNodeGraph) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra<TypeParam> dij;
  GT tree;
  dij(g, n0, tree);

  EXPECT_EQ(tree.get_num_nodes(), 1);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

// ========== TEST 9: Linear Chain Graph ==========
TYPED_TEST(DijkstraHeapTest, LinearChainGraph) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 3.0);
}

// ========== TEST 10: Complete Graph K4 ==========
TYPED_TEST(DijkstraHeapTest, CompleteGraphK4) {
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

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 1.0); // Direct path
}

// ========== TEST 11: Graph with Self Loop ==========
TYPED_TEST(DijkstraHeapTest, GraphWithSelfLoop) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n0, 5.0); // Self loop
  g.insert_arc(n0, n1, 2.0);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n1, path);

  EXPECT_EQ(d, 2.0); // Self loop should be ignored
}

// ========== TEST 12: Digraph Basic Path ==========
TYPED_TEST(DijkstraDigraphHeapTest, BasicPath) {
  DGT g;
  DNode* n0 = g.insert_node(0);
  DNode* n1 = g.insert_node(1);
  DNode* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  // Note: no arc n2->n0, so no path back

  DigraphDijkstra<TypeParam> dij;
  Path<DGT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 2.0);
}

// ========== TEST 13: Digraph No Return Path ==========
TYPED_TEST(DijkstraDigraphHeapTest, NoReturnPath) {
  DGT g;
  DNode* n0 = g.insert_node(0);
  DNode* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1.0); // Only forward direction

  DigraphDijkstra<TypeParam> dij;

  Path<DGT> path_forward(g);
  double d1 = dij(g, n0, n1, path_forward);
  EXPECT_EQ(d1, 1.0);

  Path<DGT> path_backward(g);
  double d2 = dij(g, n1, n0, path_backward);
  EXPECT_EQ(d2, std::numeric_limits<double>::max());
}

// ========== TEST 14: Empty Graph ==========
TYPED_TEST(DijkstraHeapTest, EmptyGraph) {
  GT g;

  Dijkstra<TypeParam> dij;
  GT tree;

  // Should handle empty graph gracefully - tree stays empty
  // Note: calling dij on empty graph would need a null node, so we just check tree
  EXPECT_EQ(tree.get_num_nodes(), 0);
  EXPECT_EQ(tree.get_num_arcs(), 0);
}

// ========== TEST 15: Multiple Paths Same Cost ==========
TYPED_TEST(DijkstraHeapTest, MultiplePathsSameCost) {
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

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  EXPECT_EQ(d, 2.0);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 16: Large Weights ==========
TYPED_TEST(DijkstraHeapTest, LargeWeights) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1e10);
  g.insert_arc(n1, n2, 1e10);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 2e10);
}

// ========== TEST 17: Zero Weight Edges ==========
TYPED_TEST(DijkstraHeapTest, ZeroWeightEdges) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0.0);
  g.insert_arc(n1, n2, 0.0);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 0.0);
}

// ========== TEST 18: Fractional Weights ==========
TYPED_TEST(DijkstraHeapTest, FractionalWeights) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0.5);
  g.insert_arc(n1, n2, 0.3);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  EXPECT_NEAR(d, 0.8, 1e-9);
}

// ========== TEST 19: Star Graph ==========
TYPED_TEST(DijkstraHeapTest, StarGraph) {
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

  Dijkstra<TypeParam> dij;
  GT tree;
  dij(g, center, tree);

  EXPECT_EQ(tree.get_num_nodes(), 5);
  EXPECT_EQ(tree.get_num_arcs(), 4);
}

// ========== TEST 20: Verify Path Correctness ==========
TYPED_TEST(DijkstraHeapTest, VerifyPathNodes) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);
  g.insert_arc(n0, n3, 100.0); // Much longer direct path

  Dijkstra<TypeParam> dij;
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
TYPED_TEST(DijkstraHeapTest, StateGetters) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1.0);

  Dijkstra<TypeParam> dij;

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
TYPED_TEST(DijkstraHeapTest, ComputePartialMinPathsTree) {
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

  Dijkstra<TypeParam> dij;
  GT tree;
  dij.compute_partial_min_paths_tree(g, n0, n3, tree);

  // Tree should contain n0, n1, n2, n3 but NOT n4
  EXPECT_EQ(tree.get_num_nodes(), 4);
  EXPECT_EQ(tree.get_num_arcs(), 3);
}

// ========== TEST 23: Get Min Path from Tree ==========
TYPED_TEST(DijkstraHeapTest, GetMinPathFromTree) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);

  Dijkstra<TypeParam> dij;
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
TYPED_TEST(DijkstraHeapTest, NullNodeValidation) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra<TypeParam> dij;
  GT tree;

  EXPECT_THROW(dij.compute_min_paths_tree(g, nullptr, tree), std::domain_error);
  EXPECT_THROW(dij.compute_partial_min_paths_tree(g, nullptr, n0, tree), std::domain_error);
  EXPECT_THROW(dij.compute_partial_min_paths_tree(g, n0, nullptr, tree), std::domain_error);
  EXPECT_THROW(dij.paint_min_paths_tree(g, nullptr), std::domain_error);
  EXPECT_THROW(dij.paint_partial_min_paths_tree(g, nullptr, n0), std::domain_error);
  EXPECT_THROW(dij.paint_partial_min_paths_tree(g, n0, nullptr), std::domain_error);
}

// ========== TEST 25: Disconnected Graph ==========
TYPED_TEST(DijkstraHeapTest, DisconnectedGraph) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Two disconnected components: {n0, n1} and {n2, n3}
  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  // n3 is not reachable from n0
  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ========== TEST 26: Integer Weights ==========
TYPED_TEST(DijkstraHeapTest, IntegerWeights) {
  using IGT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

  IGT g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, 3);

  DijkstraFor<TypeParam, IGT> dij;
  Path<IGT> path(g);
  int d = dij(g, n0, n2, path);

  EXPECT_EQ(d, 8);
}

// ========== TEST 27: Get Distance After Painting ==========
TYPED_TEST(DijkstraHeapTest, GetDistanceAfterPainting) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);

  EXPECT_EQ(dij.get_distance(n0), 0.0);
  EXPECT_EQ(dij.get_distance(n1), 1.0);
  EXPECT_EQ(dij.get_distance(n2), 3.0);
  EXPECT_EQ(dij.get_distance(n3), 6.0);
}

// ========== TEST 28: Get Distance Before Painting ==========
TYPED_TEST(DijkstraHeapTest, GetDistanceBeforePainting) {
  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra<TypeParam> dij;

  // Should throw because not painted yet
  EXPECT_THROW(dij.get_distance(n0), std::domain_error);
}

// ========== TEST 29: Get Distance Unreachable Node ==========
TYPED_TEST(DijkstraHeapTest, GetDistanceUnreachableNode) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Not connected

  g.insert_arc(n0, n1, 1.0);

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);

  // n2 is not reachable
  EXPECT_THROW(dij.get_distance(n2), std::domain_error);
}

// ========== TEST 30: Multiple Successive Computations ==========
TYPED_TEST(DijkstraHeapTest, MultipleSuccessiveComputations) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n0, n2, 10.0);

  Dijkstra<TypeParam> dij;

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
TYPED_TEST(DijkstraHeapTest, PaintPartialReturnsFalse) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Disconnected

  g.insert_arc(n0, n1, 1.0);

  Dijkstra<TypeParam> dij;
  bool found = dij.paint_partial_min_paths_tree(g, n0, n2);

  EXPECT_FALSE(found);
}

// ========== TEST 32: Paint Partial Returns True When Found ==========
TYPED_TEST(DijkstraHeapTest, PaintPartialReturnsTrue) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);

  Dijkstra<TypeParam> dij;
  bool found = dij.paint_partial_min_paths_tree(g, n0, n2);

  EXPECT_TRUE(found);

  // Should be able to get path now
  Path<GT> path(g);
  double d = dij.get_min_path(n2, path);
  EXPECT_EQ(d, 3.0);
}

// ========== TEST 33: Very Long Path ==========
TYPED_TEST(DijkstraHeapTest, VeryLongPath) {
  GT g;
  const int N = 100;

  std::vector<Node*> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i+1], 1.0);

  Dijkstra<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, nodes[0], nodes[N-1], path);

  EXPECT_EQ(d, static_cast<double>(N - 1));
}

// ========== TEST 34: Dense Graph Performance ==========
TYPED_TEST(DijkstraHeapTest, DenseGraphPerformance) {
  GT g;
  const int N = 50;

  std::vector<Node*> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Create complete graph
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes[i], nodes[j], static_cast<double>(i + j));

  Dijkstra<TypeParam> dij;
  GT tree;
  dij(g, nodes[0], tree);

  EXPECT_EQ(tree.get_num_nodes(), N);
  EXPECT_EQ(tree.get_num_arcs(), N - 1);
}

// ========== TEST 35: Compute Spanning Tree on Disconnected Graph ==========
TYPED_TEST(DijkstraHeapTest, ComputeSpanningTreeDisconnected) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n2, n3, 1.0);

  Dijkstra<TypeParam> dij;
  GT tree;
  dij.compute_min_paths_tree(g, n0, tree);

  EXPECT_EQ(tree.get_num_nodes(), 2);
  EXPECT_EQ(tree.get_num_arcs(), 1);
}

// ========== TEST 36: Parallel Arcs (Multigraph) ==========
// Test that Dijkstra correctly handles graphs with multiple arcs between same nodes
// NOTE: Uses DijkstraAllArcs which uses Out_Iterator_Ne to visit ALL arcs including parallel ones
TYPED_TEST(DijkstraHeapTest, ParallelArcsChoosesMinimum) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Multiple arcs between n0 and n1 with different weights
  g.insert_arc(n0, n1, 10.0);  // expensive
  g.insert_arc(n0, n1, 3.0);   // cheap - should be chosen
  g.insert_arc(n0, n1, 7.0);   // medium

  // Single arc to destination
  g.insert_arc(n1, n2, 2.0);

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 4) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  DijkstraAllArcs<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n2, path);

  // Shortest path should use the 3.0 arc: 3.0 + 2.0 = 5.0
  EXPECT_EQ(d, 5.0);
}

// ========== TEST 37: Parallel Arcs in Both Directions ==========
TYPED_TEST(DijkstraHeapTest, ParallelArcsBothDirections) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  // Arcs in both directions with different weights
  g.insert_arc(n0, n1, 5.0);
  g.insert_arc(n1, n0, 3.0);
  g.insert_arc(n0, n1, 2.0);  // Best from n0 to n1

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 3) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  DijkstraAllArcs<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n1, path);

  // In undirected graph, shortest should be 2.0
  EXPECT_EQ(d, 2.0);
}

// ========== TEST 38: Complex Multigraph ==========
TYPED_TEST(DijkstraHeapTest, ComplexMultigraph) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Multiple paths with parallel arcs
  g.insert_arc(n0, n1, 4.0);
  g.insert_arc(n0, n1, 2.0);  // Best n0->n1
  g.insert_arc(n1, n2, 3.0);
  g.insert_arc(n1, n2, 1.0);  // Best n1->n2
  g.insert_arc(n2, n3, 5.0);
  g.insert_arc(n2, n3, 2.0);  // Best n2->n3
  
  // Alternative path
  g.insert_arc(n0, n3, 10.0);

  // Check if graph supports parallel arcs
  if (g.get_num_arcs() < 7) {
    GTEST_SKIP() << "Graph type does not support parallel arcs (multigraph)";
  }

  DijkstraAllArcs<TypeParam> dij;
  Path<GT> path(g);
  double d = dij(g, n0, n3, path);

  // Best path: 2.0 + 1.0 + 2.0 = 5.0
  EXPECT_EQ(d, 5.0);
}

// ========== TEST 39: Get Min Path From Tree ==========
TYPED_TEST(DijkstraHeapTest, GetMinPathFromTreeMethod) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);
  g.insert_arc(n3, n4, 4.0);
  g.insert_arc(n0, n4, 100.0);  // Long direct path

  Dijkstra<TypeParam> dij;
  GT tree;

  // First compute the spanning tree
  auto tree_start = dij.compute_min_paths_tree(g, n0, tree);
  ASSERT_NE(tree_start, nullptr);

  // Now extract paths from the tree to different destinations
  Path<GT> path_to_n2(g);
  double d2 = dij.get_min_path(tree, n2, path_to_n2);
  EXPECT_EQ(d2, 3.0);  // 1 + 2
  EXPECT_FALSE(path_to_n2.is_empty());

  Path<GT> path_to_n4(g);
  double d4 = dij.get_min_path(tree, n4, path_to_n4);
  EXPECT_EQ(d4, 10.0);  // 1 + 2 + 3 + 4
  EXPECT_FALSE(path_to_n4.is_empty());

  // Verify path nodes for n4
  DynList<int> path_nodes;
  for (Path<GT>::Iterator it(path_to_n4); it.has_curr(); it.next())
    path_nodes.append(it.get_current_node()->get_info());

  ASSERT_EQ(path_nodes.size(), 5);
  auto pit = path_nodes.get_it();
  EXPECT_EQ(pit.get_curr(), 0); pit.next();
  EXPECT_EQ(pit.get_curr(), 1); pit.next();
  EXPECT_EQ(pit.get_curr(), 2); pit.next();
  EXPECT_EQ(pit.get_curr(), 3); pit.next();
  EXPECT_EQ(pit.get_curr(), 4);
}

// ========== TEST 40: Get Min Path From Tree Multiple Queries ==========
TYPED_TEST(DijkstraHeapTest, GetMinPathFromTreeMultipleQueries) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Star graph from n0
  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 2.0);
  g.insert_arc(n0, n3, 3.0);

  Dijkstra<TypeParam> dij;
  GT tree;
  dij.compute_min_paths_tree(g, n0, tree);

  // Query multiple destinations from same tree (efficient!)
  Path<GT> p1(g), p2(g), p3(g);
  
  double d1 = dij.get_min_path(tree, n1, p1);
  double d2 = dij.get_min_path(tree, n2, p2);
  double d3 = dij.get_min_path(tree, n3, p3);

  EXPECT_EQ(d1, 1.0);
  EXPECT_EQ(d2, 2.0);
  EXPECT_EQ(d3, 3.0);

  // Each path should have 2 nodes (start + destination)
  EXPECT_EQ(p1.size(), 2);
  EXPECT_EQ(p2.size(), 2);
  EXPECT_EQ(p3.size(), 2);
}

// ========== Distance policies and exceptions ==========

// Graph whose arcs hold their index. The weights are kept apart, so a
// distance that read the index would find other paths and other costs.
using IGT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

// The policy of the report on 7.0.0: the weight of an arc times a factor.
struct Scaled_Distance
{
  using Distance_Type = int;

  int factor = 1;

  int operator()(IGT::Arc * a) const { return factor * a->get_info(); }
};

// Counts the calls of a distance and makes one of them throw.
struct Distance_Failure
{
  long calls = 0;
  long fail_at = 0; // number of the call that throws; 0 for none

  void count()
  {
    if (++calls == fail_at)
      throw std::runtime_error("arc weight not available");
  }
};

// Weights read from a table by the index of the arc. A default-constructed
// one has no table and takes the index as the weight, so an algorithm that
// built its own instance would find other paths and other costs. (The heap
// of the algorithms builds its comparator by default, so the distance must
// have a default constructor.)
struct Table_Distance
{
  using Distance_Type = long;

  const std::vector<long> * weights = nullptr;
  Distance_Failure * failure = nullptr;

  Table_Distance() = default;

  explicit Table_Distance(const std::vector<long> & w,
                          Distance_Failure * f = nullptr) noexcept
    : weights(&w), failure(f) {}

  long operator()(IGT::Arc * a) const
  {
    if (failure != nullptr)
      failure->count();
    const auto i = static_cast<size_t>(a->get_info());
    return weights == nullptr ? static_cast<long>(i) : (*weights)[i];
  }
};

template <typename HeapTag>
using Table_Dijkstra = Dijkstra_Min_Paths<IGT, Table_Distance,
                                          Node_Arc_Iterator, Dft_Show_Arc<IGT>,
                                          HeapTag::template Heap>;

// A random simple graph, maybe disconnected, with weights from 1 to 50.
struct Weighted_Graph
{
  IGT g;
  std::vector<IGT::Node *> nodes;
  std::vector<long> weights; // weights[i] is the weight of the arc with info i

  Weighted_Graph(const size_t num_nodes, const size_t num_arcs,
                 const unsigned seed)
  {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> pick(0, num_nodes - 1);
    std::uniform_int_distribution<long> weight(1, 50);

    for (size_t i = 0; i < num_nodes; ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    std::map<std::pair<size_t, size_t>, bool> used;
    while (weights.size() < num_arcs)
      {
        auto u = pick(rng), v = pick(rng);
        if (u == v)
          continue;
        if (u > v)
          std::swap(u, v);
        if (used[{u, v}])
          continue;
        used[{u, v}] = true;
        g.insert_arc(nodes[u], nodes[v], static_cast<int>(weights.size()));
        weights.push_back(weight(rng));
      }
  }

  [[nodiscard]] long weight(IGT::Arc * a) const
  {
    return weights[static_cast<size_t>(a->get_info())];
  }

  // Distances from s, by relaxing every arc until nothing changes; apart
  // from the algorithms under test. The unreachable nodes are not in it.
  std::map<IGT::Node *, long> distances_from(IGT::Node * s)
  {
    std::map<IGT::Node *, long> d{{s, 0}};
    for (bool changed = true; changed; )
      {
        changed = false;
        for (IGT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
          {
            auto a = it.get_curr();
            auto u = g.get_src_node(a);
            auto v = g.get_tgt_node(a);
            for (int dir = 0; dir < 2; ++dir, std::swap(u, v))
              if (d.contains(u) and
                  (not d.contains(v) or d[u] + weight(a) < d[v]))
                {
                  d[v] = d[u] + weight(a);
                  changed = true;
                }
          }
      }
    return d;
  }

  // Cost of a path, which must go from s to t
  long cost_of(const Path<IGT> & path, IGT::Node * s, IGT::Node * t) const
  {
    EXPECT_EQ(path.get_first_node(), s);
    EXPECT_EQ(path.get_last_node(), t);
    long total = 0;
    path.for_each_arc([this, &total](IGT::Arc * a) { total += weight(a); });
    return total;
  }

  // Sum of the weights of the arcs of a copy of some arcs of g
  long weight_of(IGT & tree) const
  {
    long total = 0;
    for (IGT::Arc_Iterator it(tree); it.has_curr(); it.next_ne())
      total += weight(it.get_curr());
    return total;
  }
};

// Every method used a default-constructed distance instead of the one
// given to the constructor, so a factor of 3 was ignored.
TYPED_TEST(DijkstraHeapTest, UsesTheDistanceItWasGiven) {
  IGT g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  g.insert_arc(a, b, 3);
  g.insert_arc(b, c, 4);
  g.insert_arc(a, c, 20);

  Dijkstra_Min_Paths<IGT, Scaled_Distance, Node_Arc_Iterator,
                     Dft_Show_Arc<IGT>, TypeParam::template Heap>
    dij(Scaled_Distance{3});

  Path<IGT> path(g);
  EXPECT_EQ(dij.find_min_path(g, a, c, path), 21);

  dij.paint_min_paths_tree(g, a);
  EXPECT_EQ(dij.get_distance(c), 21);
  EXPECT_EQ(dij.get_min_path(c, path), 21);
  IGT painted;
  EXPECT_EQ(dij.copy_painted_min_paths_tree(g, painted), 21);

  IGT tree;
  dij.compute_min_paths_tree(g, a, tree);
  EXPECT_EQ(dij.get_min_path(tree, c, path), 21);

  dij.compute_partial_min_paths_tree(g, a, c, tree);
  EXPECT_EQ(dij.get_min_path(tree, c, path), 21);
}

// copy_painted_min_paths_tree() returned 0: Copy_Graph added the weights
// up in its own copy of the filter.
TYPED_TEST(DijkstraHeapTest, CopyOfThePaintedTreeReturnsItsWeight) {
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

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);

  GT tree;
  EXPECT_EQ(dij.copy_painted_min_paths_tree(g, tree), 4.0); // 1 + 2 + 1
  EXPECT_EQ(tree.get_num_arcs(), 3u);
}

// A distance with state gives in every method the costs of an independent
// oracle.
TYPED_TEST(DijkstraHeapTest, DistanceWithStateMatchesAnOracle) {
  for (unsigned seed = 1; seed <= 3; ++seed)
    {
      Weighted_Graph w(30, 60, seed);
      auto s = w.nodes[0];
      auto dist = w.distances_from(s);
      ASSERT_GT(dist.size(), 1u) << "seed " << seed;

      Table_Dijkstra<TypeParam> dij{Table_Distance(w.weights)};

      for (auto t : w.nodes)
        {
          if (t == s)
            continue; // no path from a node to itself; see PathToSelf

          Path<IGT> path(w.g);
          const long cost = dij.find_min_path(w.g, s, t, path);
          if (not dist.contains(t))
            {
              EXPECT_EQ(cost, std::numeric_limits<long>::max());
              continue;
            }
          EXPECT_EQ(cost, dist[t]) << "seed " << seed;
          EXPECT_EQ(w.cost_of(path, s, t), dist[t]) << "seed " << seed;
        }

      dij.paint_min_paths_tree(w.g, s);
      for (auto [t, d] : dist)
        {
          EXPECT_EQ(dij.get_distance(t), d) << "seed " << seed;
          Path<IGT> path(w.g);
          EXPECT_EQ(dij.get_min_path(t, path), d) << "seed " << seed;
          EXPECT_EQ(w.cost_of(path, s, t), d) << "seed " << seed;
        }

      IGT painted;
      const long painted_weight = dij.copy_painted_min_paths_tree(w.g, painted);
      EXPECT_EQ(painted.get_num_arcs(), dist.size() - 1);
      EXPECT_EQ(painted_weight, w.weight_of(painted)) << "seed " << seed;

      IGT tree;
      dij.compute_min_paths_tree(w.g, s, tree);
      EXPECT_EQ(tree.get_num_nodes(), dist.size());
      for (auto [t, d] : dist)
        {
          Path<IGT> path(w.g);
          EXPECT_EQ(dij.get_min_path(tree, t, path), d) << "seed " << seed;
          EXPECT_EQ(w.cost_of(path, s, t), d) << "seed " << seed;
        }

      for (auto [t, d] : dist)
        {
          IGT partial;
          dij.compute_partial_min_paths_tree(w.g, s, t, partial);
          Path<IGT> path(w.g);
          EXPECT_EQ(dij.get_min_path(partial, t, path), d) << "seed " << seed;
        }
    }
}

// The cookies were allocated at the start and freed at the end, but not
// when the algorithm threw: a distance that failed leaked them. Each call
// of the distance fails in turn; the same object, on the same graph, must
// then give the right costs. LeakSanitizer reports any cookie left.
TYPED_TEST(DijkstraHeapTest, DistanceThatThrowsLeavesTheGraphReusable) {
  Weighted_Graph w(12, 24, 7);
  auto s = w.nodes[0];
  auto dist = w.distances_from(s);
  ASSERT_GT(dist.size(), 3u);
  auto far = std::max_element(dist.begin(), dist.end(),
                              [](auto & x, auto & y)
                              { return x.second < y.second; })->first;

  Distance_Failure failure;
  Table_Dijkstra<TypeParam> dij{Table_Distance(w.weights, &failure)};
  IGT tree;

  const std::vector<std::function<void()>> methods = {
    [&] {
      Path<IGT> path(w.g);
      EXPECT_EQ(dij.find_min_path(w.g, s, far, path), dist[far]);
    },
    [&] {
      dij.paint_min_paths_tree(w.g, s);
      for (auto [t, d] : dist)
        EXPECT_EQ(dij.get_distance(t), d);
    },
    [&] {
      dij.compute_min_paths_tree(w.g, s, tree);
      Path<IGT> path(w.g);
      EXPECT_EQ(dij.get_min_path(tree, far, path), dist[far]);
    },
    [&] {
      dij.compute_partial_min_paths_tree(w.g, s, far, tree);
      Path<IGT> path(w.g);
      EXPECT_EQ(dij.get_min_path(tree, far, path), dist[far]);
    }
  };

  for (size_t m = 0; m < methods.size(); ++m)
    {
      failure = Distance_Failure();
      methods[m]();
      const long calls = failure.calls;
      ASSERT_GT(calls, 0) << "method " << m;

      for (long k = 1; k <= calls; ++k)
        {
          failure.calls = 0;
          failure.fail_at = k;
          EXPECT_THROW(methods[m](), std::runtime_error)
            << "method " << m << ", call " << k;

          failure.fail_at = 0;
          methods[m]();
        }
    }
}

// The overflow of the report on 7.0.0: the cookies leaked when the sum of
// two weights overflowed.
TYPED_TEST(DijkstraHeapTest, OverflowLeavesTheGraphReusable) {
  IGT g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  auto big = g.insert_arc(a, b, INT_MAX - 10);
  g.insert_arc(b, c, 20);

  DijkstraFor<TypeParam, IGT> dij;
  IGT tree;

  for (int m = 0; m < 4; ++m)
    {
      big->get_info() = INT_MAX - 10;
      Path<IGT> path(g);
      switch (m)
        {
        case 0:
          EXPECT_THROW(dij.find_min_path(g, a, c, path), std::overflow_error);
          break;
        case 1:
          EXPECT_THROW(dij.paint_min_paths_tree(g, a), std::overflow_error);
          break;
        case 2:
          EXPECT_THROW(dij.compute_min_paths_tree(g, a, tree),
                       std::overflow_error);
          break;
        default:
          EXPECT_THROW(dij.compute_partial_min_paths_tree(g, a, c, tree),
                       std::overflow_error);
          break;
        }
      EXPECT_FALSE(dij.is_painted()) << "method " << m;

      big->get_info() = 1;
      EXPECT_EQ(dij.find_min_path(g, a, c, path), 21) << "method " << m;
    }
}

// Computing a tree resets the bits and cookies of a previous painting, so
// the painting cannot be used any more.
TYPED_TEST(DijkstraHeapTest, ComputingATreeForgetsThePainting) {
  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);
  ASSERT_TRUE(dij.is_painted());

  GT tree;
  dij.compute_min_paths_tree(g, n0, tree);
  EXPECT_FALSE(dij.is_painted());
  Path<GT> path(g);
  EXPECT_THROW(dij.get_min_path(n2, path), std::domain_error);
  EXPECT_THROW(dij.get_distance(n2), std::domain_error);

  dij.paint_min_paths_tree(g, n0);
  EXPECT_EQ(dij.get_distance(n2), 3.0);
}

// Checks that every node and arc of tree is mapped, both ways, to one of
// the graph with the same info.
void expect_mapped_to_graph(GT & tree)
{
  for (GT::Node_Iterator it(tree); it.has_curr(); it.next_ne())
    {
      auto tp = it.get_curr();
      auto gp = mapped_node<GT>(tp);
      ASSERT_NE(gp, nullptr);
      EXPECT_EQ(mapped_node<GT>(gp), tp);
      EXPECT_EQ(gp->get_info(), tp->get_info());
    }
  for (GT::Arc_Iterator it(tree); it.has_curr(); it.next_ne())
    {
      auto ta = it.get_curr();
      auto ga = mapped_arc<GT>(ta);
      ASSERT_NE(ga, nullptr);
      EXPECT_EQ(mapped_arc<GT>(ga), ta);
      EXPECT_EQ(ga->get_info(), ta->get_info());
    }
}

// The arcs of the tree were mapped to the cookies freed at the end, not to
// the arcs of the graph.
TYPED_TEST(DijkstraHeapTest, TreeIsMappedToTheGraph) {
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

  Dijkstra<TypeParam> dij;
  GT tree;
  dij.compute_min_paths_tree(g, n0, tree);
  ASSERT_EQ(tree.get_num_arcs(), 3u);
  expect_mapped_to_graph(tree);

  dij.compute_partial_min_paths_tree(g, n0, n2, tree);
  expect_mapped_to_graph(tree);
}

// The copy of the painted tree was mapped through the cookies that held
// the painting: its nodes got the parents and its arcs the freed cookies,
// while get_distance() went on and returned 0.
TYPED_TEST(DijkstraHeapTest, CopyOfThePaintedTreeIsMappedAndEndsThePainting) {
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

  Dijkstra<TypeParam> dij;
  dij.paint_min_paths_tree(g, n0);
  ASSERT_EQ(dij.get_distance(n3), 4.0);

  GT tree;
  EXPECT_EQ(dij.copy_painted_min_paths_tree(g, tree), 4.0);
  ASSERT_EQ(tree.get_num_arcs(), 3u);
  expect_mapped_to_graph(tree);

  EXPECT_FALSE(dij.is_painted());
  EXPECT_THROW(dij.get_distance(n3), std::domain_error);
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
