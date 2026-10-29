/**
 * GoogleTest suite for Prim.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Prim's minimum spanning tree algorithm implementation in Aleph-w.
 */


/**
 * @file prim_test.cc
 * @brief Tests for Prim
 */
#include <gtest/gtest.h>

#include <functional>
#include <limits>
#include <new>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

#include <Prim.H>
#include <Kruskal.H>  // For comparison tests
#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <tpl_sgraph.H>

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

// =============================================================================
// Distance Tests
// =============================================================================

namespace {

// The weight of an arc, or 100 minus it when `reversed` is set: a
// default-constructed instance orders the arcs the other way.
struct Reversible_Distance
{
  using Distance_Type = int;

  bool reversed = false;

  int operator()(Arc *a) const
  {
    return reversed ? 100 - a->get_info() : a->get_info();
  }
};

// The weight of an arc times a factor, with no default constructor.
struct Factor_Only_Distance
{
  using Distance_Type = int;

  int factor;

  explicit Factor_Only_Distance(int f) noexcept : factor(f) {}

  int operator()(Arc *a) const { return factor * a->get_info(); }
};

// Total of the arcs of tree, measured with d
template <class Distance>
int tree_distance(const GT &tree, const Distance &d)
{
  int total = 0;
  for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
    total += d(it.get_current_arc_ne());
  return total;
}

// Total of the arcs of g that Prim painted, measured with d
template <class Distance>
int painted_distance(const GT &g, const Distance &d)
{
  int total = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    if (IS_ARC_VISITED(it.get_current_arc_ne(), Aleph::Spanning_Tree))
      total += d(it.get_current_arc_ne());
  return total;
}

} // namespace

// The heap ordered the arcs with a default-constructed distance; only the
// choice between two arcs that reach the same node used the given one. With
// the reversed weights (99, 98 and 95), the tree took 99 and 95.
TEST(PrimDistance, UsesTheDistanceItWasGiven)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5);

  const Reversible_Distance reversed{true};
  Prim_Min_Spanning_Tree<GT, Reversible_Distance> prim(reversed);

  GT tree;
  prim(g, n0, tree);
  EXPECT_EQ(tree_distance(tree, reversed), 98 + 95);

  prim(g, n0);
  EXPECT_EQ(painted_distance(g, reversed), 98 + 95);
}

// The same on random graphs, against Kruskal, which uses the given distance
TEST(PrimDistance, MatchesKruskalWithTheGivenDistance)
{
  std::mt19937 rng(7);
  std::uniform_int_distribution<int> weight_dist(1, 99);
  const Reversible_Distance reversed{true};

  for (int iter = 0; iter < 20; ++iter)
    {
      const int N = 15;
      GT g;
      std::vector<Node *> nodes;
      for (int i = 0; i < N; ++i)
        nodes.push_back(g.insert_node(i));
      for (int i = 1; i < N; ++i)
        g.insert_arc(nodes[i - 1], nodes[i], weight_dist(rng));
      for (int i = 0; i < 2 * N; ++i)
        {
          const int a = rng() % N;
          const int b = rng() % N;
          if (a != b)
            g.insert_arc(nodes[a], nodes[b], weight_dist(rng));
        }

      GT prim_tree, kruskal_tree;
      Prim_Min_Spanning_Tree<GT, Reversible_Distance> prim(reversed);
      prim(g, prim_tree);
      Kruskal_Min_Spanning_Tree<GT, Reversible_Distance>
        kruskal(Reversible_Distance{true});
      kruskal(g, kruskal_tree);

      EXPECT_EQ(tree_distance(prim_tree, reversed),
                tree_distance(kruskal_tree, reversed)) << "graph " << iter;

      prim(g);
      EXPECT_EQ(painted_distance(g, reversed),
                tree_distance(kruskal_tree, reversed)) << "graph " << iter;
    }
}

// Building that default comparator required a default constructor.
TEST(PrimDistance, TakesADistanceWithoutADefaultConstructor)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5);

  const Factor_Only_Distance twice(2);
  Prim_Min_Spanning_Tree<GT, Factor_Only_Distance> prim(twice);

  GT tree;
  prim(g, tree);
  EXPECT_EQ(tree_distance(tree, twice), 2 * (1 + 2));

  prim(g);
  EXPECT_EQ(painted_distance(g, twice), 2 * (1 + 2));
}

// =============================================================================
// Painting Tests
// =============================================================================

namespace {

size_t painted_arcs(const GT &g)
{
  size_t count = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    if (IS_ARC_VISITED(it.get_current_arc_ne(), Aleph::Spanning_Tree))
      ++count;
  return count;
}

} // namespace

// The heap of the painting kept its nodes in a member that the nodes do not
// have, so painting did not compile.
TEST(PrimPainting, PaintsTheArcsOfTheTree)
{
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

  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, a);
  EXPECT_EQ(painted_arcs(g), 5u);
  EXPECT_EQ(painted_distance(g, Dft_Dist<GT>()), 12);

  prim(g);
  EXPECT_EQ(painted_arcs(g), 5u);
  EXPECT_EQ(painted_distance(g, Dft_Dist<GT>()), 12);
}

// The painting put the loops of the first node in the heap. A light one
// came out first and was painted, since the painting marks each arc it
// takes from the heap before seeing whether it closes a cycle.
TEST(PrimPainting, LeavesOutALoopOnTheFirstNode)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto loop = g.insert_arc(n0, n0, 1);
  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n2, 3);

  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, n0);
  EXPECT_FALSE(IS_ARC_VISITED(loop, Aleph::Spanning_Tree));
  EXPECT_EQ(painted_arcs(g), 2u);
  EXPECT_EQ(painted_distance(g, Dft_Dist<GT>()), 2 + 3);

  GT tree;
  prim(g, n0, tree);
  EXPECT_EQ(tree_total_weight(tree), 2 + 3);
}

// A heavy one never came out, and the cookie of the first node kept its heap
// node after the heap freed it.
TEST(PrimPainting, LeavesTheCookiesNullWithALoopOnTheFirstNode)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  g.insert_arc(n0, n0, 100);
  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n2, 3);

  Prim_Min_Spanning_Tree<GT> prim;
  prim(g, n0);
  EXPECT_EQ(painted_arcs(g), 2u);
  EXPECT_EQ(NODE_COOKIE(n0), nullptr);
  EXPECT_EQ(NODE_COOKIE(n1), nullptr);
  EXPECT_EQ(NODE_COOKIE(n2), nullptr);
}

namespace {

// The weight of an arc; tells in *read whether it read that of watched
struct Watching_Distance
{
  using Distance_Type = int;

  Arc *watched = nullptr;
  bool *read = nullptr;

  int operator()(Arc *a) const
  {
    if (a == watched)
      *read = true;
    return a->get_info();
  }
};

} // namespace

// The version that builds the tree put the loops of the first node in the
// heap, and it marked in g the one that came out, as if it were in the tree
TEST(PrimMapping, LeavesOutALoopOnTheFirstNode)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto loop = g.insert_arc(n0, n0, 0);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 3);

  bool read = false;
  Prim_Min_Spanning_Tree<GT, Watching_Distance>
    prim(Watching_Distance{loop, &read});
  GT tree;
  prim(g, n0, tree);
  EXPECT_EQ(tree.get_num_arcs(), 2u);
  EXPECT_EQ(tree_total_weight(tree), 1 + 2);
  EXPECT_FALSE(IS_ARC_VISITED(loop, Aleph::Spanning_Tree));
  EXPECT_EQ(painted_arcs(g), 2u); // the arcs of the tree, as when painting
  EXPECT_FALSE(read);             // the loop did not enter the heap

  prim(g, n0);
  EXPECT_FALSE(IS_ARC_VISITED(loop, Aleph::Spanning_Tree));
  EXPECT_EQ(painted_arcs(g), 2u);
  EXPECT_FALSE(read);
}

// =============================================================================
// Exception Tests
// =============================================================================

namespace {

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

// The weight of an arc, counting the calls in failure
struct Failing_Distance
{
  using Distance_Type = int;

  Distance_Failure *failure = nullptr;

  int operator()(Arc *a) const
  {
    if (failure != nullptr)
      failure->count();
    return a->get_info();
  }
};

} // namespace

// The heap called the distance in functions that were noexcept, so a
// distance that threw there ended the program. When painting, the cookies
// of the arcs left in the heap pointed to the heap nodes freed with it.
TEST(PrimExceptions, DistanceThatThrowsLeavesTheGraphReusable)
{
  std::mt19937 rng(11);
  std::uniform_int_distribution<int> weight_dist(1, 50);
  const int N = 12;
  GT g;
  std::vector<Node *> nodes;
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));
  for (int i = 1; i < N; ++i)
    g.insert_arc(nodes[i - 1], nodes[i], weight_dist(rng));
  for (int i = 0; i < 2 * N; ++i)
    {
      const int a = rng() % N;
      const int b = rng() % N;
      if (a != b)
        g.insert_arc(nodes[a], nodes[b], weight_dist(rng));
    }

  GT expected;
  Kruskal_Min_Spanning_Tree<GT> kruskal;
  kruskal(g, expected);
  const int weight = tree_total_weight(expected);

  Distance_Failure failure;
  Prim_Min_Spanning_Tree<GT, Failing_Distance> prim(Failing_Distance{&failure});

  const std::vector<std::function<void()>> methods = {
    [&] {
      GT tree;
      prim(g, tree);
      EXPECT_EQ(tree_total_weight(tree), weight);
    },
    [&] {
      prim(g);
      EXPECT_EQ(painted_distance(g, Dft_Dist<GT>()), weight);
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

          if (m == 1)
            {
              for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
                {
                  EXPECT_EQ(NODE_COOKIE(it.get_curr()), nullptr)
                    << "call " << k;
                }
            }

          failure.fail_at = 0;
          methods[m]();
        }
    }
}

namespace {

// Counts the nodes created with an info; the creation number fail_at throws
int fragile_creations = 0;
int fragile_fail_at = 0; // 0 for none

struct Fragile_Node : public Graph_Node<int>
{
  using Graph_Node<int>::Graph_Node;

  Fragile_Node(const int & info) : Graph_Node<int>(info)
  {
    if (fragile_fail_at != 0 and ++fragile_creations == fragile_fail_at)
      throw std::bad_alloc();
  }
};

using Fragile_Graph = List_Graph<Fragile_Node, Graph_Arc<int>>;

} // namespace

// The Prim_Info of the nodes were created before the guard that frees them,
// so a node of the tree that could not be created leaked those before it.
TEST(PrimExceptions, ANodeOfTheTreeThatCannotBeCreatedLeaksNothing)
{
  Fragile_Graph g;
  std::vector<Fragile_Node *> nodes;
  for (int i = 0; i < 5; ++i)
    nodes.push_back(g.insert_node(i));
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[i - 1], nodes[i], i);
  g.insert_arc(nodes[0], nodes[4], 10);

  Prim_Min_Spanning_Tree<Fragile_Graph> prim;
  for (int k = 1; k <= 5; ++k) // the first, the ones between and the last
    {
      // the cookies hold the mapping of the previous run, and Prim leaves
      // those of the nodes it does not reach as they are
      for (auto p : nodes)
        NODE_COOKIE(p) = nullptr;

      fragile_creations = 0;
      fragile_fail_at = k;
      Fragile_Graph tree;
      EXPECT_THROW(prim(g, nodes[0], tree), std::bad_alloc) << "node " << k;
      fragile_fail_at = 0;

      for (auto p : nodes)
        {
          EXPECT_EQ(NODE_COOKIE(p), nullptr) << "node " << k;
        }

      Fragile_Graph again; // the graph can be used again
      prim(g, nodes[0], again);
      EXPECT_EQ(tree_total_weight(again), 1 + 2 + 3 + 4) << "node " << k;
    }
}

// =============================================================================
// Mapping Tests
// =============================================================================

namespace {

template <class G>
struct PrimMappingTest : public ::testing::Test {};

using Representations =
  ::testing::Types<List_Graph<Graph_Node<int>, Graph_Arc<int>>,
                   List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
                   Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

} // namespace

TYPED_TEST_SUITE(PrimMappingTest, Representations);

// Unmapping a node gave its node in the tree the cookie that the node still
// had, its Prim_Info, freed right after: the tree mapped back to freed
// memory. The data of the nodes repeat, so that only identity counts.
TYPED_TEST(PrimMappingTest, NodesAndArcsMapBothWays)
{
  using G = TypeParam;
  std::mt19937 rng(5);
  std::uniform_int_distribution<int> weight_dist(1, 30);
  const int N = 12;
  G g;
  std::vector<typename G::Node *> nodes;
  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i % 3));
  for (int i = 1; i < N; ++i)
    g.insert_arc(nodes[i - 1], nodes[i], weight_dist(rng));
  for (int i = 0; i < 2 * N; ++i)
    {
      const int a = rng() % N;
      const int b = rng() % N;
      if (a != b)
        g.insert_arc(nodes[a], nodes[b], weight_dist(rng));
    }

  G tree;
  Prim_Min_Spanning_Tree<G> prim;
  prim(g, nodes[0], tree);
  ASSERT_EQ(tree.get_num_nodes(), g.get_num_nodes());
  ASSERT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);

  std::set<typename G::Node *> tree_nodes;
  for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
    tree_nodes.insert(it.get_curr());
  std::set<typename G::Arc *> graph_arcs;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    graph_arcs.insert(it.get_curr());

  for (auto p : nodes)
    {
      auto t = mapped_node<G>(p);
      ASSERT_TRUE(tree_nodes.contains(t));
      EXPECT_EQ(mapped_node<G>(t), p);
      EXPECT_EQ(t->get_info(), p->get_info());
    }

  for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto ta = it.get_curr();
      auto a = mapped_arc<G>(ta);
      ASSERT_TRUE(graph_arcs.contains(a));
      EXPECT_EQ(mapped_arc<G>(a), ta);
      auto s = mapped_node<G>(tree.get_src_node(ta));
      auto t = mapped_node<G>(tree.get_tgt_node(ta));
      EXPECT_TRUE((g.get_src_node(a) == s and g.get_tgt_node(a) == t) or
                  (g.get_src_node(a) == t and g.get_tgt_node(a) == s));
    }
}
