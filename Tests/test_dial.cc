/**
 * @file test_dial.cc
 * @brief Comprehensive tests for Dial.H
 *
 * Tests cover correctness, edge cases, validation, and cross-checks
 * against Dijkstra's algorithm for integer-weighted graphs.
 */

# include <gtest/gtest.h>
# include <Dial.H>
# include <Dijkstra.H>
# include <tpl_graph.H>

using namespace Aleph;

// ============================================================================
// Type aliases
// ============================================================================

using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using DNode = DGT::Node;
using DArc = DGT::Arc;

// ============================================================================
// TEST 1: Basic shortest path
// ============================================================================
TEST(Dial, BasicShortestPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 3);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 2);
  g.insert_arc(n2, n3, 5);
  g.insert_arc(n2, n1, 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n3, path);

  // Best: n0->n2(1)->n1(1)->n3(2) = 4
  EXPECT_EQ(d, 4);
}

// ============================================================================
// TEST 2: Path to self
// ============================================================================
TEST(Dial, PathToSelf)
{
  GT g;
  auto n0 = g.insert_node(0);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n0, path);

  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 3: No path exists
// ============================================================================
TEST(Dial, NoPathExists)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n1, path);

  EXPECT_EQ(d, std::numeric_limits<int>::max());
  EXPECT_TRUE(path.is_empty());
}

// ============================================================================
// TEST 4: Single node graph
// ============================================================================
TEST(Dial, SingleNode)
{
  GT g;
  auto n0 = g.insert_node(0);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n0, path);

  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 5: Linear chain
// ============================================================================
TEST(Dial, LinearChain)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n2, 3);
  g.insert_arc(n2, n3, 4);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n3, path);

  EXPECT_EQ(d, 9);
}

// ============================================================================
// TEST 6: All zero weights
// ============================================================================
TEST(Dial, AllZeroWeights)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n2, path);

  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 7: Weight 1 edges (like BFS)
// ============================================================================
TEST(Dial, WeightOneEdges)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n2, n3, 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n3, path);

  EXPECT_EQ(d, 2);
}

// ============================================================================
// TEST 8: Null node validation
// ============================================================================
TEST(Dial, NullNodeValidation)
{
  GT g;
  auto n0 = g.insert_node(0);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);

  EXPECT_THROW(dial(g, nullptr, n0, path), std::domain_error);
  EXPECT_THROW(dial(g, n0, nullptr, path), std::domain_error);
}

// ============================================================================
// TEST 9: Paint and get_min_path
// ============================================================================
TEST(Dial, PaintAndGetMinPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Dial_Min_Paths<GT> dial;
  dial.paint_min_paths_tree(g, n0);

  EXPECT_TRUE(dial.is_painted());

  Path<GT> path(g);
  int d = dial.get_min_path(n3, path);
  EXPECT_EQ(d, 6);
}

// ============================================================================
// TEST 10: Get distance after painting
// ============================================================================
TEST(Dial, GetDistanceAfterPainting)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Dial_Min_Paths<GT> dial;
  dial.paint_min_paths_tree(g, n0);

  EXPECT_EQ(dial.get_distance(n0), 0);
  EXPECT_EQ(dial.get_distance(n1), 1);
  EXPECT_EQ(dial.get_distance(n2), 3);
  EXPECT_EQ(dial.get_distance(n3), 6);
}

// ============================================================================
// TEST 11: Get distance before painting
// ============================================================================
TEST(Dial, GetDistanceBeforePainting)
{
  GT g;
  auto n0 = g.insert_node(0);

  Dial_Min_Paths<GT> dial;
  EXPECT_THROW(dial.get_distance(n0), std::domain_error);
}

// ============================================================================
// TEST 12: Cross-validation with Dijkstra
// ============================================================================
TEST(Dial, CrossValidationWithDijkstra)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);

  g.insert_arc(n0, n1, 4);
  g.insert_arc(n0, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n1, 1);
  g.insert_arc(n2, n3, 8);
  g.insert_arc(n3, n4, 2);
  g.insert_arc(n4, n5, 1);
  g.insert_arc(n2, n5, 10);

  // Dial
  Dial_Min_Paths<GT> dial;
  Path<GT> dial_path(g);
  int dial_d = dial(g, n0, n5, dial_path);

  // Dijkstra
  Dijkstra_Min_Paths<GT> dij;
  Path<GT> dij_path(g);
  int dij_d = dij(g, n0, n5, dij_path);

  EXPECT_EQ(dial_d, dij_d);
}

// ============================================================================
// TEST 13: Directed graph
// ============================================================================
TEST(Dial, DigraphBasic)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 3);
  g.insert_arc(n1, n2, 2);

  Dial_Min_Paths<DGT> dial;
  Path<DGT> path(g);
  int d = dial(g, n0, n2, path);

  EXPECT_EQ(d, 5);
}

// ============================================================================
// TEST 14: Directed graph no return
// ============================================================================
TEST(Dial, DigraphNoReturn)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 3);

  Dial_Min_Paths<DGT> dial;

  Path<DGT> p1(g);
  EXPECT_EQ(dial(g, n0, n1, p1), 3);

  Path<DGT> p2(g);
  EXPECT_EQ(dial(g, n1, n0, p2), std::numeric_limits<int>::max());
}

// ============================================================================
// TEST 15: Multiple computations
// ============================================================================
TEST(Dial, MultipleComputations)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n2, 3);

  Dial_Min_Paths<GT> dial;

  Path<GT> p1(g);
  EXPECT_EQ(dial(g, n0, n2, p1), 5);

  Path<GT> p2(g);
  EXPECT_EQ(dial(g, n2, n0, p2), 5);  // undirected
}

// ============================================================================
// TEST 16: Large linear graph
// ============================================================================
TEST(Dial, LargeLinearGraph)
{
  GT g;
  const int N = 100;

  std::vector<Node *> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, nodes[0], nodes[N - 1], path);

  EXPECT_EQ(d, N - 1);
}

// ============================================================================
// TEST 17: Large weights
// ============================================================================
TEST(Dial, LargeWeights)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 100);
  g.insert_arc(n1, n2, 200);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n2, path);

  EXPECT_EQ(d, 300);
}

// ============================================================================
// TEST 18: Path node-by-node verification
// ============================================================================
TEST(Dial, PathNodeVerification)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);
  g.insert_arc(n0, n3, 100);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n3, path);

  EXPECT_EQ(d, 6);

  DynList<int> nodes;
  for (Path<GT>::Iterator it(path); it.has_curr(); it.next())
    nodes.append(it.get_current_node()->get_info());

  ASSERT_EQ(nodes.size(), 4u);
  auto pit = nodes.get_it();
  EXPECT_EQ(pit.get_curr(), 0); pit.next();
  EXPECT_EQ(pit.get_curr(), 1); pit.next();
  EXPECT_EQ(pit.get_curr(), 2); pit.next();
  EXPECT_EQ(pit.get_curr(), 3);
}

// ============================================================================
// TEST 19: Complete graph K4
// ============================================================================
TEST(Dial, CompleteGraphK4)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n0, n3, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n2, n3, 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n3, path);

  EXPECT_EQ(d, 1);
}

// ============================================================================
// TEST 20: Graph with cycle
// ============================================================================
TEST(Dial, GraphWithCycle)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n0, 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n2, path);

  // Undirected: n0->n2 via edge n2-n0 costs 1
  EXPECT_EQ(d, 1);
}

// ============================================================================
// TEST 21: State getters
// ============================================================================
TEST(Dial, StateGetters)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Dial_Min_Paths<GT> dial;

  EXPECT_FALSE(dial.is_painted());
  EXPECT_EQ(dial.get_start_node(), nullptr);
  EXPECT_EQ(dial.get_graph(), nullptr);

  dial.paint_min_paths_tree(g, n0);

  EXPECT_TRUE(dial.is_painted());
  EXPECT_EQ(dial.get_start_node(), n0);
  EXPECT_EQ(dial.get_graph(), &g);
}

// ============================================================================
// TEST 22: Disconnected graph painting
// ============================================================================
TEST(Dial, DisconnectedPainting)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n2, n3, 2);

  Dial_Min_Paths<GT> dial;
  dial.paint_min_paths_tree(g, n0);

  EXPECT_EQ(dial.get_distance(n1), 1);
  EXPECT_THROW(dial.get_distance(n2), std::domain_error);
}

// ============================================================================
// TEST 23: Relaxation correctness (cheaper path found later)
// ============================================================================
TEST(Dial, RelaxationCorrectness)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 10);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n2, n1, 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int d = dial(g, n0, n1, path);

  EXPECT_EQ(d, 2);  // n0->n2(1)->n1(1) = 2, not direct n0->n1(10)
}

// ============================================================================
// TEST 24: Star graph
// ============================================================================
TEST(Dial, StarGraph)
{
  GT g;
  auto center = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(center, n1, 1);
  g.insert_arc(center, n2, 2);
  g.insert_arc(center, n3, 3);
  g.insert_arc(center, n4, 4);

  Dial_Min_Paths<GT> dial;
  dial.paint_min_paths_tree(g, center);

  EXPECT_EQ(dial.get_distance(n1), 1);
  EXPECT_EQ(dial.get_distance(n2), 2);
  EXPECT_EQ(dial.get_distance(n3), 3);
  EXPECT_EQ(dial.get_distance(n4), 4);
}

// ============================================================================
// === EXAMPLE: Road network with integer kilometer distances ===
// A small network of cities connected by roads with integer distances.
// Find the shortest route from city A to city E.
//
//     A ---3--- B ---2--- E
//     |         |
//     1         4
//     |         |
//     C ---1--- D
//
// Shortest A->E: A->C(1)->D(1)->B(4)->E(2) = 8? No, A->B(3)->E(2)=5
// ============================================================================
TEST(Dial, RoadNetworkExample)
{
  GT g;
  auto a = g.insert_node(0);  // A
  auto b = g.insert_node(1);  // B
  auto c = g.insert_node(2);  // C
  auto d = g.insert_node(3);  // D
  auto e = g.insert_node(4);  // E

  g.insert_arc(a, b, 3);
  g.insert_arc(a, c, 1);
  g.insert_arc(b, d, 4);
  g.insert_arc(b, e, 2);
  g.insert_arc(c, d, 1);

  Dial_Min_Paths<GT> dial;
  Path<GT> path(g);
  int dist = dial(g, a, e, path);

  EXPECT_EQ(dist, 5);  // A->B(3)->E(2)
  EXPECT_FALSE(path.is_empty());
}

// ============================================================================
// TEST 26: Mixed weights cross-validation with Dijkstra on larger graph
// ============================================================================
TEST(Dial, LargerCrossValidation)
{
  GT g;
  const int N = 20;

  std::vector<Node *> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Create edges with various weights
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], (i % 5) + 1);

  // Some cross edges
  for (int i = 0; i < N - 3; i += 3)
    g.insert_arc(nodes[i], nodes[i + 3], 10);

  Dial_Min_Paths<GT> dial;
  Dijkstra_Min_Paths<GT> dij;

  for (int end = 1; end < N; end += 3)
    {
      Path<GT> dp(g), jp(g);
      int dd = dial(g, nodes[0], nodes[end], dp);
      int jd = dij(g, nodes[0], nodes[end], jp);
      EXPECT_EQ(dd, jd) << "Mismatch for end=" << end;
    }
}

// ============================================================================
// GoogleTest main
// ============================================================================
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
