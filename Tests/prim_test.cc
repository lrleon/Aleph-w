/**
 * GoogleTest suite for Prim.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Prim's minimum spanning tree algorithm implementation in Aleph-w.
 */

#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <set>
#include <vector>

#include <Prim.H>
#include <Kruskal.H>  // For comparison tests
#include <tpl_graph.H>
#include <tpl_graph_utils.H>

using namespace Aleph;

// Graph types for tests
using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

namespace {

// Helper to compute total weight of tree
template <class G>
int tree_total_weight(const G &tree)
{
  int total = 0;
  for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
    total += it.get_current_arc_ne()->get_info();
  return total;
}

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

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST(PrimBasic, EmptyGraph)
{
  GT g;
  GT tree;
  
  Prim_Min_Spanning_Tree<GT> prim;
  
  // Prim throws exception when graph has no nodes
  EXPECT_THROW(prim(g, tree), std::exception);
}

TEST(PrimBasic, SingleNode)
{
  GT g;
  g.insert_node(1);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 1u);
  EXPECT_EQ(tree.get_num_arcs(), 0u);
}

TEST(PrimBasic, TwoNodesOneArc)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 10);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 2u);
  EXPECT_EQ(tree.get_num_arcs(), 1u);
  EXPECT_EQ(tree_total_weight(tree), 10);
}

TEST(PrimBasic, TriangleGraph)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  // Triangle with weights 1, 2, 3
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n1, n3, 3);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 3u);
  EXPECT_EQ(tree.get_num_arcs(), 2u);
  EXPECT_EQ(tree_total_weight(tree), 3);  // MST uses edges 1 and 2
  EXPECT_TRUE(is_tree_connected(tree));
}

TEST(PrimBasic, SquareGraph)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Square: 1-2-3-4-1 with diagonal 1-3
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n3, n4, 3);
  g.insert_arc(n4, n1, 4);
  g.insert_arc(n1, n3, 5);  // diagonal
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 4u);
  EXPECT_EQ(tree.get_num_arcs(), 3u);
  EXPECT_EQ(tree_total_weight(tree), 6);  // MST uses edges 1, 2, 3
  EXPECT_TRUE(is_tree_connected(tree));
}

// =============================================================================
// Comparison with Kruskal Tests
// =============================================================================

TEST(PrimVsKruskal, SameWeightOnSimpleGraph)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n1, n3, 3);
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n2, n4, 4);
  g.insert_arc(n3, n4, 5);
  g.insert_arc(n3, n5, 6);
  g.insert_arc(n4, n5, 7);
  
  GT prim_tree, kruskal_tree;
  
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, prim_tree);
  
  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g, kruskal_tree);
  
  EXPECT_EQ(tree_total_weight(prim_tree), tree_total_weight(kruskal_tree));
  EXPECT_TRUE(is_tree_connected(prim_tree));
  EXPECT_TRUE(is_tree_connected(kruskal_tree));
}

TEST(PrimVsKruskal, RandomGraph)
{
  std::mt19937 rng(42);  // Fixed seed for reproducibility
  std::uniform_int_distribution<int> weight_dist(1, 100);
  
  const int N = 20;
  GT g;
  std::vector<Node *> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create a connected graph
  for (int i = 1; i < N; ++i)
    g.insert_arc(nodes[i-1], nodes[i], weight_dist(rng));
  
  // Add some random edges
  for (int i = 0; i < N * 2; ++i)
    {
      int a = rng() % N;
      int b = rng() % N;
      if (a != b)
        g.insert_arc(nodes[a], nodes[b], weight_dist(rng));
    }
  
  GT prim_tree, kruskal_tree;
  
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, prim_tree);
  
  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g, kruskal_tree);
  
  EXPECT_EQ(tree_total_weight(prim_tree), tree_total_weight(kruskal_tree));
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST(PrimEdgeCases, AllEqualWeights)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  
  // Complete graph with all equal weights
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n1, n4, 1);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n2, n4, 1);
  g.insert_arc(n3, n4, 1);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 4u);
  EXPECT_EQ(tree.get_num_arcs(), 3u);
  EXPECT_EQ(tree_total_weight(tree), 3);
  EXPECT_TRUE(is_tree_connected(tree));
}

TEST(PrimEdgeCases, LinearGraph)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  
  // Linear chain: 1-2-3-4-5
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 2);
  g.insert_arc(n3, n4, 3);
  g.insert_arc(n4, n5, 4);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 5u);
  EXPECT_EQ(tree.get_num_arcs(), 4u);
  EXPECT_EQ(tree_total_weight(tree), 10);  // 1+2+3+4
  EXPECT_TRUE(is_tree_connected(tree));
}

TEST(PrimEdgeCases, StarGraph)
{
  GT g;
  auto center = g.insert_node(0);
  
  // Star graph with center connected to all others
  for (int i = 1; i <= 5; ++i)
    {
      auto leaf = g.insert_node(i);
      g.insert_arc(center, leaf, i);
    }
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 6u);
  EXPECT_EQ(tree.get_num_arcs(), 5u);
  EXPECT_EQ(tree_total_weight(tree), 15);  // 1+2+3+4+5
  EXPECT_TRUE(is_tree_connected(tree));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(PrimStress, LargeGraph)
{
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> weight_dist(1, 1000);
  
  const int N = 100;
  GT g;
  std::vector<Node *> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create a connected graph (chain)
  for (int i = 1; i < N; ++i)
    g.insert_arc(nodes[i-1], nodes[i], weight_dist(rng));
  
  // Add random edges to make it denser
  for (int i = 0; i < N * 3; ++i)
    {
      int a = rng() % N;
      int b = rng() % N;
      if (a != b)
        g.insert_arc(nodes[a], nodes[b], weight_dist(rng));
    }
  
  GT prim_tree, kruskal_tree;
  
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, prim_tree);
  
  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g, kruskal_tree);
  
  EXPECT_EQ(prim_tree.get_num_nodes(), static_cast<size_t>(N));
  EXPECT_EQ(prim_tree.get_num_arcs(), static_cast<size_t>(N - 1));
  EXPECT_TRUE(is_tree_connected(prim_tree));
  
  // Both algorithms should produce trees with the same total weight
  EXPECT_EQ(tree_total_weight(prim_tree), tree_total_weight(kruskal_tree));
}

TEST(PrimStress, DenseGraph)
{
  std::mt19937 rng(54321);
  std::uniform_int_distribution<int> weight_dist(1, 100);
  
  const int N = 30;
  GT g;
  std::vector<Node *> nodes;
  
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  
  // Create almost complete graph
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.insert_arc(nodes[i], nodes[j], weight_dist(rng));
  
  GT prim_tree, kruskal_tree;
  
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, prim_tree);
  
  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g, kruskal_tree);
  
  EXPECT_EQ(prim_tree.get_num_nodes(), static_cast<size_t>(N));
  EXPECT_EQ(prim_tree.get_num_arcs(), static_cast<size_t>(N - 1));
  EXPECT_TRUE(is_tree_connected(prim_tree));
  
  // Both algorithms should produce trees with the same total weight
  EXPECT_EQ(tree_total_weight(prim_tree), tree_total_weight(kruskal_tree));
}

// =============================================================================
// Node Mapping Tests
// =============================================================================

TEST(PrimMapping, NodesAreMapped)
{
  GT g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 2);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  // Verify that each node in g maps to a node in tree
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto gnode = it.get_curr();
      auto tnode = mapped_node<GT>(gnode);
      EXPECT_NE(tnode, nullptr);
      EXPECT_EQ(gnode->get_info(), tnode->get_info());
    }
}

// =============================================================================
// Textbook Example
// =============================================================================

TEST(PrimTextbook, ClassicExample)
{
  // Classic MST example from textbooks
  // Graph with nodes A(0), B(1), C(2), D(3), E(4), F(5)
  GT g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);
  auto d = g.insert_node(3);
  auto e = g.insert_node(4);
  auto f = g.insert_node(5);
  
  g.insert_arc(a, b, 6);
  g.insert_arc(a, c, 1);
  g.insert_arc(a, d, 5);
  g.insert_arc(b, c, 2);
  g.insert_arc(b, e, 5);
  g.insert_arc(c, d, 2);
  g.insert_arc(c, e, 6);
  g.insert_arc(c, f, 4);
  g.insert_arc(d, f, 4);
  g.insert_arc(e, f, 3);
  
  GT tree;
  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, tree);
  
  EXPECT_EQ(tree.get_num_nodes(), 6u);
  EXPECT_EQ(tree.get_num_arcs(), 5u);
  // MST weight: 1 + 2 + 2 + 3 + 4 = 12 (or could be 1+2+2+4+3=12)
  EXPECT_EQ(tree_total_weight(tree), 12);
  EXPECT_TRUE(is_tree_connected(tree));
}
