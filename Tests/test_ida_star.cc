/**
 * @file test_ida_star.cc
 * @brief Comprehensive tests for IDA_Star.H
 *
 * Tests cover correctness, edge cases, heuristic behavior, and
 * cross-checks against A* and Dijkstra for weighted shortest paths.
 */

# include <gtest/gtest.h>
# include <IDA_Star.H>
# include <AStar.H>
# include <Dijkstra.H>
# include <tpl_graph.H>

using namespace Aleph;

// ============================================================================
// Type aliases
// ============================================================================

using GT = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using Node = GT::Node;
using Arc = GT::Arc;

using IGT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using INode = IGT::Node;
using IArc = IGT::Arc;

using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using DNode = DGT::Node;
using DArc = DGT::Arc;

// Zero heuristic (degrades to iterative deepening DFS with cost bound)
template <class G>
using IDA_Zero = IDA_Star<G, Dft_Dist<G>, Zero_Heuristic<G, Dft_Dist<G>>>;

// ============================================================================
// TEST 1: Basic shortest path (zero heuristic)
// ============================================================================
TEST(IDAStar, BasicShortestPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);
  g.insert_arc(n0, n3, 100.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n3, path);

  EXPECT_EQ(d, 6.0);
  EXPECT_FALSE(path.is_empty());
}

// ============================================================================
// TEST 2: Path to self
// ============================================================================
TEST(IDAStar, PathToSelf)
{
  GT g;
  auto n0 = g.insert_node(0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n0, path);

  EXPECT_EQ(d, 0.0);
}

// ============================================================================
// TEST 3: No path exists
// ============================================================================
TEST(IDAStar, NoPathExists)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n1, path);

  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ============================================================================
// TEST 4: Single node
// ============================================================================
TEST(IDAStar, SingleNode)
{
  GT g;
  auto n0 = g.insert_node(0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n0, path);

  EXPECT_EQ(d, 0.0);
}

// ============================================================================
// TEST 5: Linear chain
// ============================================================================
TEST(IDAStar, LinearChain)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n2, n3, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n3, path);

  EXPECT_EQ(d, 3.0);
}

// ============================================================================
// TEST 6: Two-node graph
// ============================================================================
TEST(IDAStar, TwoNodeGraph)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 5.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n1, path);

  EXPECT_EQ(d, 5.0);
}

// ============================================================================
// TEST 7: Null node validation
// ============================================================================
TEST(IDAStar, NullNodeValidation)
{
  GT g;
  auto n0 = g.insert_node(0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);

  EXPECT_THROW(ida(g, nullptr, n0, path), std::domain_error);
  EXPECT_THROW(ida(g, n0, nullptr, path), std::domain_error);
}

// ============================================================================
// TEST 8: Relaxation (finds cheaper path)
// ============================================================================
TEST(IDAStar, RelaxationFindsCheeperPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 10.0);
  g.insert_arc(n0, n2, 1.0);
  g.insert_arc(n2, n1, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n1, path);

  EXPECT_EQ(d, 2.0);
}

// ============================================================================
// TEST 9: Cross-validation with Dijkstra
// ============================================================================
TEST(IDAStar, CrossValidationWithDijkstra)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 4.0);
  g.insert_arc(n0, n2, 2.0);
  g.insert_arc(n1, n3, 5.0);
  g.insert_arc(n2, n1, 1.0);
  g.insert_arc(n2, n3, 8.0);
  g.insert_arc(n3, n4, 2.0);
  g.insert_arc(n0, n4, 10.0);

  IDA_Zero<GT> ida;
  Path<GT> ida_path(g);
  double ida_d = ida(g, n0, n4, ida_path);

  Dijkstra_Min_Paths<GT> dij;
  Path<GT> dij_path(g);
  double dij_d = dij(g, n0, n4, dij_path);

  EXPECT_NEAR(ida_d, dij_d, 1e-9);
}

// ============================================================================
// TEST 10: Cross-validation with A*
// ============================================================================
TEST(IDAStar, CrossValidationWithAStar)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 4.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n1, n3, 5.0);
  g.insert_arc(n2, n3, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> ida_path(g);
  double ida_d = ida(g, n0, n3, ida_path);

  AStar_Min_Path<GT> astar;
  Path<GT> astar_path(g);
  double astar_d = astar(g, n0, n3, astar_path);

  EXPECT_NEAR(ida_d, astar_d, 1e-9);
}

// ============================================================================
// TEST 11: Complete graph K4
// ============================================================================
TEST(IDAStar, CompleteGraphK4)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n0, n2, 1.0);
  g.insert_arc(n0, n3, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n1, n3, 1.0);
  g.insert_arc(n2, n3, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n3, path);

  EXPECT_EQ(d, 1.0);
}

// ============================================================================
// TEST 12: Graph with cycle
// ============================================================================
TEST(IDAStar, GraphWithCycle)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n2, n0, 1.0);  // cycle
  g.insert_arc(n2, n3, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n3, path);

  // Undirected: n0-n2(via arc n2-n0, weight 1)->n3(1) = 2
  // Or n0->n1->n2->n3 = 3. Best is 2.
  EXPECT_EQ(d, 2.0);
}

// ============================================================================
// TEST 13: Integer weights
// ============================================================================
TEST(IDAStar, IntegerWeights)
{
  IGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 5);
  g.insert_arc(n1, n2, 3);

  IDA_Zero<IGT> ida;
  Path<IGT> path(g);
  int d = ida(g, n0, n2, path);

  EXPECT_EQ(d, 8);
}

// ============================================================================
// TEST 14: Multiple successive computations
// ============================================================================
TEST(IDAStar, MultipleComputations)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);

  IDA_Zero<GT> ida;

  Path<GT> p1(g);
  double d1 = ida(g, n0, n2, p1);
  EXPECT_EQ(d1, 3.0);

  Path<GT> p2(g);
  double d2 = ida(g, n2, n0, p2);
  EXPECT_EQ(d2, 3.0);  // undirected
}

// ============================================================================
// TEST 15: Path node-by-node verification
// ============================================================================
TEST(IDAStar, PathNodeVerification)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 2.0);
  g.insert_arc(n2, n3, 3.0);
  g.insert_arc(n0, n3, 100.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n3, path);

  EXPECT_EQ(d, 6.0);

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
// TEST 16: Zero weight edges
// ============================================================================
TEST(IDAStar, ZeroWeightEdges)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0.0);
  g.insert_arc(n1, n2, 0.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n2, path);

  EXPECT_EQ(d, 0.0);
}

// ============================================================================
// TEST 17: Fractional weights
// ============================================================================
TEST(IDAStar, FractionalWeights)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0.5);
  g.insert_arc(n1, n2, 0.3);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n2, path);

  EXPECT_NEAR(d, 0.8, 1e-9);
}

// ============================================================================
// TEST 18: Directed graph
// ============================================================================
TEST(IDAStar, DigraphBasic)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n1, n2, 1.0);

  IDA_Zero<DGT> ida;
  Path<DGT> path(g);
  double d = ida(g, n0, n2, path);

  EXPECT_EQ(d, 2.0);
}

// ============================================================================
// TEST 19: Directed graph no return
// ============================================================================
TEST(IDAStar, DigraphNoReturn)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1.0);

  IDA_Zero<DGT> ida;
  Path<DGT> p(g);
  double d = ida(g, n1, n0, p);

  EXPECT_EQ(d, std::numeric_limits<double>::max());
}

// ============================================================================
// TEST 20: Star graph
// ============================================================================
TEST(IDAStar, StarGraph)
{
  GT g;
  auto center = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(center, n1, 1.0);
  g.insert_arc(center, n2, 2.0);
  g.insert_arc(center, n3, 3.0);
  g.insert_arc(center, n4, 4.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n1, n4, path);

  // n1->center(1)->n4(4) = 5
  EXPECT_EQ(d, 5.0);
}

// ============================================================================
// TEST 21: Disconnected graph
// ============================================================================
TEST(IDAStar, DisconnectedGraph)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1.0);
  g.insert_arc(n2, n3, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n3, path);

  EXPECT_EQ(d, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// ============================================================================
// TEST 22: Larger graph cross-validation
// ============================================================================
TEST(IDAStar, LargerCrossValidation)
{
  GT g;
  const int N = 10;

  std::vector<Node *> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], static_cast<double>((i % 3) + 1));

  // Add shortcuts
  g.insert_arc(nodes[0], nodes[4], 20.0);
  g.insert_arc(nodes[3], nodes[7], 15.0);

  IDA_Zero<GT> ida;
  Dijkstra_Min_Paths<GT> dij;

  for (int end = 1; end < N; end += 2)
    {
      Path<GT> ip(g), dp(g);
      double id = ida(g, nodes[0], nodes[end], ip);
      double dd = dij(g, nodes[0], nodes[end], dp);
      EXPECT_NEAR(id, dd, 1e-9) << "Mismatch for end=" << end;
    }
}

// ============================================================================
// === EXAMPLE: Weighted grid pathfinding ===
// A small 3x2 grid with varying edge costs. Find the cheapest path
// from top-left to bottom-right.
//
//   (0) --2-- (1) --3-- (2)
//    |         |         |
//    1         5         1
//    |         |         |
//   (3) --1-- (4) --1-- (5)
//
// Best path: (0)-1-(3)-1-(4)-1-(5) = 3
// ============================================================================
TEST(IDAStar, WeightedGridExample)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);

  // Horizontal
  g.insert_arc(n0, n1, 2.0);
  g.insert_arc(n1, n2, 3.0);
  g.insert_arc(n3, n4, 1.0);
  g.insert_arc(n4, n5, 1.0);

  // Vertical
  g.insert_arc(n0, n3, 1.0);
  g.insert_arc(n1, n4, 5.0);
  g.insert_arc(n2, n5, 1.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n5, path);

  // Best: n0->n3(1)->n4(1)->n5(1) = 3
  EXPECT_EQ(d, 3.0);
}

// ============================================================================
// TEST 24: Self-loop
// ============================================================================
TEST(IDAStar, SelfLoop)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n0, 5.0);
  g.insert_arc(n0, n1, 2.0);

  IDA_Zero<GT> ida;
  Path<GT> path(g);
  double d = ida(g, n0, n1, path);

  EXPECT_EQ(d, 2.0);
}

// ============================================================================
// TEST 25: Chebyshev heuristic compilation check
// ============================================================================
// This just verifies the Chebyshev_Heuristic compiles correctly.
// A full test would need a grid with coordinate-based node info.
TEST(IDAStar, ChebyshevHeuristicExists)
{
  // Just verify the template instantiates
  struct Coord { int x; int y; };
  using CG = List_Graph<Graph_Node<Coord>, Graph_Arc<int>>;
  using CH = Chebyshev_Heuristic<CG>;

  CH ch;
  (void)ch;  // suppress unused warning
}

// ============================================================================
// GoogleTest main
// ============================================================================
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
