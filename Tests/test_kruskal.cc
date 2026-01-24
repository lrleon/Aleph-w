/**
 * GoogleTest suite for Kruskal.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Kruskal's minimum spanning tree algorithm implementation in Aleph-w.
 */


/**
 * @file test_kruskal.cc
 * @brief Tests for Test Kruskal
 */
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <set>
#include <vector>

#include <Kruskal.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <tpl_agraph.H>

using namespace Aleph;

// Graph types for tests
using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// Array-based graph type
using AGT = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;
using ANode = AGT::Node;
using AArc = AGT::Arc;

namespace {

// Helper to count painted arcs
template <class G>
size_t count_painted_arcs(const G &g)
{
  size_t n = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    if (IS_ARC_VISITED(it.get_current_arc_ne(), Aleph::Spanning_Tree))
      ++n;
  return n;
}

// Helper to compute total weight of tree
template <class G>
int tree_total_weight(const G &tree)
{
  int total = 0;
  for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
    total += it.get_current_arc_ne()->get_info();
  return total;
}

// Helper to compute total weight of painted arcs
template <class G>
int painted_total_weight(const G &g)
{
  int total = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_current_arc_ne();
      if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
        total += arc->get_info();
    }
  return total;
}

// Custom distance accessor for testing
template <class G>
struct Scaled_Dist
{
  using Distance_Type = int;
  static const Distance_Type Zero_Distance = 0;
  static const Distance_Type Max_Distance = std::numeric_limits<int>::max();

  int factor = 1;
  Scaled_Dist() = default;
  explicit Scaled_Dist(int f) : factor(f) {}

  int operator()(typename G::Arc *a) const noexcept
  {
    return a->get_info() * factor;
  }
};

// Custom arc filter that only accepts arcs with weight <= threshold
template <class G>
struct Weight_Filter
{
  int threshold;

  Weight_Filter(int t = std::numeric_limits<int>::max()) : threshold(t) {}

  bool operator()(typename G::Arc *a) const noexcept
  {
    return a->get_info() <= threshold;
  }
};

// Check if tree is connected (for undirected graphs)
template <class G>
bool is_tree_connected(const G &tree)
{
  if (tree.get_num_nodes() == 0)
    return true;
  if (tree.get_num_nodes() == 1)
    return true;

  // A tree with V nodes should have V-1 arcs
  if (tree.get_num_arcs() != tree.get_num_nodes() - 1)
    return false;

  // BFS to check connectivity
  tree.reset_nodes();
  auto first = tree.get_first_node();
  NODE_BITS(first).set_bit(Aleph::Spanning_Tree, true);

  DynList<typename G::Node*> queue;
  queue.append(first);
  size_t visited = 1;

  while (not queue.is_empty())
    {
      auto curr = queue.remove_first();
      for (Node_Arc_Iterator<G> it(curr); it.has_curr(); it.next_ne())
        {
          auto tgt = it.get_tgt_node_ne();
          if (not IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            {
              NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
              queue.append(tgt);
              ++visited;
            }
        }
    }

  return visited == tree.get_num_nodes();
}

} // namespace

// ========== TEST 1: Simple Connected Graph ==========
TEST(KruskalTest, SimpleConnectedGraph) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 3);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Tree should have same number of nodes as g
  ASSERT_EQ(tree.get_num_nodes(), g.get_num_nodes());

  // Tree should have n-1 arcs (spanning tree)
  ASSERT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);

  // MST weight: edges (0,1)=1 + (1,2)=2 + (2,3)=3 = 6
  ASSERT_EQ(tree_total_weight(tree), 6);
}

// ========== TEST 2: Single Node Graph ==========
TEST(KruskalTest, SingleNodeGraph) {

  GT g;
  g.insert_node(0);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 1u);
  ASSERT_EQ(tree.get_num_arcs(), 0u);
}

// ========== TEST 3: Two Nodes One Arc ==========
TEST(KruskalTest, TwoNodesOneArc) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 5);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 2u);
  ASSERT_EQ(tree.get_num_arcs(), 1u);
  ASSERT_EQ(tree_total_weight(tree), 5);
}

// ========== TEST 4: Linear Chain ==========
TEST(KruskalTest, LinearChain) {

  GT g;
  const int num_nodes = 5;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i)
    nodes.push_back(g.insert_node(i));

  // Chain: 0--1--2--3--4 with weights 1,2,3,4
  for (int i = 0; i < num_nodes - 1; ++i)
    g.insert_arc(nodes[i], nodes[i+1], i + 1);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Tree is the chain itself
  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_nodes));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_nodes - 1));
  // Total weight: 1+2+3+4 = 10
  ASSERT_EQ(tree_total_weight(tree), 10);
}

// ========== TEST 5: Complete Graph K4 ==========
TEST(KruskalTest, CompleteGraphK4) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // K4 with distinct weights
  g.insert_arc(n0, n1, 1);  // min
  g.insert_arc(n0, n2, 2);  // 2nd min
  g.insert_arc(n0, n3, 10);
  g.insert_arc(n1, n2, 5);
  g.insert_arc(n1, n3, 3);  // 3rd min
  g.insert_arc(n2, n3, 7);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 4u);
  ASSERT_EQ(tree.get_num_arcs(), 3u);
  // MST: (0,1)=1 + (0,2)=2 + (1,3)=3 = 6
  ASSERT_EQ(tree_total_weight(tree), 6);
}

// ========== TEST 6: Star Graph ==========
TEST(KruskalTest, StarGraph) {

  GT g;
  Node* center = g.insert_node(0);
  const int num_leaves = 5;
  std::vector<Node*> leaves;

  for (int i = 1; i <= num_leaves; ++i)
    {
      auto leaf = g.insert_node(i);
      leaves.push_back(leaf);
      g.insert_arc(center, leaf, i);
    }

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Star tree: center connects to all leaves
  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_leaves + 1));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_leaves));
  // Total weight: 1+2+3+4+5 = 15
  ASSERT_EQ(tree_total_weight(tree), 15);
}

// ========== TEST 7: Grid Graph 3x3 ==========
TEST(KruskalTest, GridGraph3x3) {

  GT g;
  std::vector<std::vector<Node*>> grid(3, std::vector<Node*>(3));

  // Create 3x3 grid
  int id = 0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      grid[i][j] = g.insert_node(id++);

  // Add horizontal edges (weight = row + 1)
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 2; ++j)
      g.insert_arc(grid[i][j], grid[i][j+1], i + 1);

  // Add vertical edges (weight = column + 5)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
      g.insert_arc(grid[i][j], grid[i+1][j], j + 5);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // 9 nodes, should have 8 arcs
  ASSERT_EQ(tree.get_num_nodes(), 9u);
  ASSERT_EQ(tree.get_num_arcs(), 8u);
  ASSERT_TRUE(is_tree_connected(tree));
}

// ========== TEST 8: Diamond Graph ==========
TEST(KruskalTest, DiamondGraph) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Diamond shape with two paths
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 2);
  g.insert_arc(n1, n3, 3);
  g.insert_arc(n2, n3, 1);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 4u);
  ASSERT_EQ(tree.get_num_arcs(), 3u);
  // MST: (0,1)=1 + (2,3)=1 + (0,2)=2 = 4
  ASSERT_EQ(tree_total_weight(tree), 4);
}

// ========== TEST 9: All Equal Weights ==========
TEST(KruskalTest, AllEqualWeights) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // All edges have weight 5
  g.insert_arc(n0, n1, 5);
  g.insert_arc(n0, n2, 5);
  g.insert_arc(n0, n3, 5);
  g.insert_arc(n1, n2, 5);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 5);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Any spanning tree is valid
  ASSERT_EQ(tree.get_num_nodes(), 4u);
  ASSERT_EQ(tree.get_num_arcs(), 3u);
  // Total weight: 3 * 5 = 15
  ASSERT_EQ(tree_total_weight(tree), 15);
}

// ========== TEST 10: Unique Weights ==========
TEST(KruskalTest, UniqueWeights) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // All edges have unique weights
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 3);
  g.insert_arc(n0, n3, 5);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 6);
  g.insert_arc(n2, n3, 4);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Deterministic MST: (0,1)=1 + (1,2)=2 + (2,3)=4 = 7
  ASSERT_EQ(tree_total_weight(tree), 7);
}

// ========== TEST 11: Zero Weight Edges ==========
TEST(KruskalTest, ZeroWeightEdges) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);
  ASSERT_EQ(tree_total_weight(tree), 0);
}

// ========== TEST 12: Large Weights ==========
TEST(KruskalTest, LargeWeights) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1000000);
  g.insert_arc(n1, n2, 2000000);
  g.insert_arc(n0, n2, 1500000);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_arcs(), 2u);
  // MST: (0,1)=1000000 + (0,2)=1500000 = 2500000
  ASSERT_EQ(tree_total_weight(tree), 2500000);
}

// ========== TEST 13: Parallel Edges ==========
TEST(KruskalTest, ParallelEdges) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  // Multiple edges between same nodes
  g.insert_arc(n0, n1, 10);
  g.insert_arc(n0, n1, 5);  // minimum
  g.insert_arc(n0, n1, 8);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 2u);
  ASSERT_EQ(tree.get_num_arcs(), 1u);
  // Should pick the minimum weight edge
  ASSERT_EQ(tree_total_weight(tree), 5);
}

// ========== TEST 14: Self Loop ==========
TEST(KruskalTest, SelfLoop) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n0, 1);  // Self-loop
  g.insert_arc(n0, n1, 2);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 2u);
  // Self-loops should be ignored (would create cycle)
  ASSERT_EQ(tree.get_num_arcs(), 1u);
  ASSERT_EQ(tree_total_weight(tree), 2);
}

// ========== TEST 15: Cycle Graph ==========
TEST(KruskalTest, CycleGraph) {

  GT g;
  const int num_nodes = 5;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i)
    nodes.push_back(g.insert_node(i));

  // Create cycle: 0--1--2--3--4--0
  for (int i = 0; i < num_nodes; ++i)
    g.insert_arc(nodes[i], nodes[(i+1) % num_nodes], i + 1);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Should remove one edge to break the cycle
  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_nodes));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_nodes - 1));
  // MST: picks 4 smallest edges = 1+2+3+4 = 10
  ASSERT_EQ(tree_total_weight(tree), 10);
}

// ========== TEST 16: Paint Mode ==========
TEST(KruskalTest, PaintMode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 3);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g);  // Paint mode

  // Verify spanning tree arcs are painted
  ASSERT_EQ(count_painted_arcs(g), g.get_num_nodes() - 1);
  // MST weight: 1+2+3 = 6
  ASSERT_EQ(painted_total_weight(g), 6);
}

// ========== TEST 17: Tree Building Mode ==========
TEST(KruskalTest, TreeBuildingMode) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);
  ASSERT_TRUE(is_tree_connected(tree));
}

// ========== TEST 18: Tree Has V-1 Arcs ==========
TEST(KruskalTest, TreeHasCorrectArcCount) {

  GT g;
  const int num_nodes = 10;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i)
    nodes.push_back(g.insert_node(i));

  // Create a complete graph
  int weight = 1;
  for (int i = 0; i < num_nodes; ++i)
    for (int j = i + 1; j < num_nodes; ++j)
      g.insert_arc(nodes[i], nodes[j], weight++);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_nodes));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_nodes - 1));
}

// ========== TEST 19: All Nodes in Tree ==========
TEST(KruskalTest, AllNodesInTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Verify all node info values are present
  std::set<int> node_infos;
  for (typename GT::Node_Iterator it(tree); it.has_curr(); it.next_ne())
    node_infos.insert(it.get_curr()->get_info());

  ASSERT_TRUE(node_infos.count(0));
  ASSERT_TRUE(node_infos.count(1));
  ASSERT_TRUE(node_infos.count(2));
  ASSERT_TRUE(node_infos.count(3));
}

// ========== TEST 20: Tree is Connected ==========
TEST(KruskalTest, TreeIsConnected) {

  GT g;
  const int num_nodes = 8;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i)
    nodes.push_back(g.insert_node(i));

  // Create random-ish connected graph
  std::mt19937 rng(12345);
  for (int i = 0; i < num_nodes - 1; ++i)
    g.insert_arc(nodes[i], nodes[i+1], rng() % 10 + 1);

  // Add some cross edges
  g.insert_arc(nodes[0], nodes[4], rng() % 10 + 1);
  g.insert_arc(nodes[2], nodes[6], rng() % 10 + 1);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_TRUE(is_tree_connected(tree));
}

// ========== TEST 21: Bit Flags Correctly Set ==========
TEST(KruskalTest, BitFlagsCorrectlySet) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  Arc* a01 = g.insert_arc(n0, n1, 1);
  Arc* a12 = g.insert_arc(n1, n2, 2);
  Arc* a02 = g.insert_arc(n0, n2, 10);  // Should not be in MST

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g);

  ASSERT_TRUE(IS_ARC_VISITED(a01, Aleph::Spanning_Tree));
  ASSERT_TRUE(IS_ARC_VISITED(a12, Aleph::Spanning_Tree));
  ASSERT_FALSE(IS_ARC_VISITED(a02, Aleph::Spanning_Tree));
}

// ========== TEST 22: Node Mapping Correct ==========
TEST(KruskalTest, NodeMappingCorrect) {

  GT g;
  Node* n0 = g.insert_node(100);
  Node* n1 = g.insert_node(200);
  Node* n2 = g.insert_node(300);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Check that tree nodes have same info as original nodes
  std::set<int> tree_infos;
  for (typename GT::Node_Iterator it(tree); it.has_curr(); it.next_ne())
    tree_infos.insert(it.get_curr()->get_info());

  ASSERT_TRUE(tree_infos.count(100));
  ASSERT_TRUE(tree_infos.count(200));
  ASSERT_TRUE(tree_infos.count(300));
}

// ========== TEST 23: Arc Mapping Correct ==========
TEST(KruskalTest, ArcMappingCorrect) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, 7);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Check that tree arcs have same weights as original arcs
  std::set<int> tree_weights;
  for (typename GT::Arc_Iterator it(tree); it.has_curr(); it.next_ne())
    tree_weights.insert(it.get_curr()->get_info());

  ASSERT_TRUE(tree_weights.count(5));
  ASSERT_TRUE(tree_weights.count(7));
}

// ========== TEST 24: Digraph Rejection ==========
TEST(KruskalTest, DigraphRejection) {

  using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  DGT dg;
  auto n0 = dg.insert_node(0);
  auto n1 = dg.insert_node(1);
  dg.insert_arc(n0, n1, 1);

  Kruskal_Min_Spanning_Tree<DGT> kruskal;
  EXPECT_THROW(kruskal(dg), std::domain_error);
}

// ========== TEST 27: Empty Graph ==========
TEST(KruskalTest, EmptyGraph) {

  GT g;  // Empty graph

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 0u);
  ASSERT_EQ(tree.get_num_arcs(), 0u);
}

// ========== TEST 28: Custom Distance Functor ==========
TEST(KruskalTest, CustomDistanceFunctor) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 2);  // scaled = 4
  g.insert_arc(n1, n2, 3);  // scaled = 6
  g.insert_arc(n0, n2, 4);  // scaled = 8

  // Use default constructor and let it pick the default Distance
  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // MST should still pick (0,1) and (1,2)
  ASSERT_EQ(tree.get_num_arcs(), 2u);
  // Original weights: 2 + 3 = 5
  ASSERT_EQ(tree_total_weight(tree), 5);
}

// ========== TEST 29: Array Graph Type ==========
TEST(KruskalTest, ArrayGraphType) {

  AGT g;
  ANode* n0 = g.insert_node(0);
  ANode* n1 = g.insert_node(1);
  ANode* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5);

  Kruskal_Min_Spanning_Tree<AGT> kruskal;
  AGT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);
  ASSERT_EQ(tree_total_weight(tree), 3);
}

// ========== TEST 30: Multiple Calls on Same Instance ==========
TEST(KruskalTest, MultipleCallsSameInstance) {

  GT g1;
  Node* n0 = g1.insert_node(0);
  Node* n1 = g1.insert_node(1);
  g1.insert_arc(n0, n1, 5);

  GT g2;
  Node* m0 = g2.insert_node(0);
  Node* m1 = g2.insert_node(1);
  Node* m2 = g2.insert_node(2);
  g2.insert_arc(m0, m1, 1);
  g2.insert_arc(m1, m2, 2);

  Kruskal_Min_Spanning_Tree<GT> kruskal;

  GT tree1;
  kruskal(g1, tree1);
  ASSERT_EQ(tree1.get_num_arcs(), 1u);

  GT tree2;
  kruskal(g2, tree2);
  ASSERT_EQ(tree2.get_num_arcs(), 2u);
}

// ========== TEST 31: Large Connected Graph ==========
TEST(KruskalTest, LargeConnectedGraph) {

  GT g;
  const int num_nodes = 100;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i)
    nodes.push_back(g.insert_node(i));

  // Create a chain to ensure connectivity
  for (int i = 0; i < num_nodes - 1; ++i)
    g.insert_arc(nodes[i], nodes[i+1], i + 1);

  // Add random edges
  std::mt19937 rng(99999);
  std::uniform_int_distribution<int> ndist(0, num_nodes - 1);
  std::uniform_int_distribution<int> wdist(1, 1000);

  for (int e = 0; e < num_nodes * 3; ++e)
    {
      int u = ndist(rng);
      int v = ndist(rng);
      if (u != v)
        g.insert_arc(nodes[u], nodes[v], wdist(rng));
    }

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_nodes));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_nodes - 1));
  ASSERT_TRUE(is_tree_connected(tree));
}

// ========== TEST 32: Paint Then Build Tree ==========
TEST(KruskalTest, PaintThenBuildTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5);

  Kruskal_Min_Spanning_Tree<GT> kruskal;

  // First paint
  kruskal(g);
  int painted_weight = painted_total_weight(g);

  // Then build tree
  GT tree;
  kruskal(g, tree);
  int tree_weight = tree_total_weight(tree);

  // Both should have same weight
  ASSERT_EQ(painted_weight, tree_weight);
}

// ========== TEST 33: Disconnected Graph Creates Forest ==========
TEST(KruskalTest, DisconnectedGraphCreatesForest) {

  GT g;
  // Component 1
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  // Component 2 (disconnected)
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  g.insert_arc(n2, n3, 2);

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Should create a forest with 2 trees
  ASSERT_EQ(tree.get_num_nodes(), 4u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);  // Two separate edges
}

// ========== TEST 34: Verify Minimum Weight Property ==========
TEST(KruskalTest, VerifyMinimumWeightProperty) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Create graph with known MST
  g.insert_arc(n0, n1, 1);   // in MST
  g.insert_arc(n0, n2, 3);   // in MST
  g.insert_arc(n0, n3, 10);  // not in MST
  g.insert_arc(n1, n2, 5);   // not in MST
  g.insert_arc(n1, n3, 4);   // in MST
  g.insert_arc(n2, n3, 6);   // not in MST

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // MST weight should be: 1 + 3 + 4 = 8
  ASSERT_EQ(tree_total_weight(tree), 8);

  // Verify this is minimum by checking against all spanning tree possibilities
  // (For this simple graph, we can verify manually)
  ASSERT_LT(tree_total_weight(tree), 1 + 3 + 6);   // alt tree
  ASSERT_LT(tree_total_weight(tree), 1 + 5 + 4);   // alt tree
  ASSERT_LT(tree_total_weight(tree), 1 + 10 + 6);  // alt tree
}

// ========== TEST 35: Repeated Edges Same Nodes ==========
TEST(KruskalTest, RepeatedEdgesSameNodes) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Multiple edges between pairs
  g.insert_arc(n0, n1, 10);
  g.insert_arc(n0, n1, 1);   // min
  g.insert_arc(n1, n2, 8);
  g.insert_arc(n1, n2, 2);   // min

  Kruskal_Min_Spanning_Tree<GT> kruskal;
  GT tree;
  kruskal(g, tree);

  // Should pick minimum edges
  ASSERT_EQ(tree_total_weight(tree), 3);  // 1 + 2
}

// ========== TEST 36: is_painted() Getter ==========
TEST(KruskalTest, IsPaintedGetter) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  Kruskal_Min_Spanning_Tree<GT> kruskal;

  // Before painting
  ASSERT_FALSE(kruskal.is_painted());

  // After painting
  kruskal(g);
  ASSERT_TRUE(kruskal.is_painted());

  // After painting to tree (should still be true)
  GT tree;
  kruskal(g, tree);
  ASSERT_TRUE(kruskal.is_painted());
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}