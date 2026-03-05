/**
 * @file test_zero_one_bfs.cc
 * @brief Comprehensive tests for Zero_One_BFS.H
 *
 * Tests cover correctness, edge cases, validation, and cross-checks
 * against Dijkstra's algorithm for 0-1 weighted graphs.
 */

# include <gtest/gtest.h>
# include <Zero_One_BFS.H>
# include <Dijkstra.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

using namespace Aleph;

// ============================================================================
// Type aliases
// ============================================================================

// Undirected graph with integer weights
using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// Directed graph with integer weights
using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using DNode = DGT::Node;
using DArc = DGT::Arc;

// ============================================================================
// TEST 1: Basic shortest path with mixed 0/1 weights
// ============================================================================
TEST(ZeroOneBFS, BasicShortestPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n0, n3, 1);  // direct but same cost

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  // Direct path n0->n3 costs 1, path through n1,n2 costs 1+0+1=2
  EXPECT_EQ(d, 1);
  EXPECT_FALSE(path.is_empty());
}

// ============================================================================
// TEST 2: All zero weights
// ============================================================================
TEST(ZeroOneBFS, AllZeroWeights)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 3: All one weights
// ============================================================================
TEST(ZeroOneBFS, AllOneWeights)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  EXPECT_EQ(d, 3);
}

// ============================================================================
// TEST 4: Path to self
// ============================================================================
TEST(ZeroOneBFS, PathToSelf)
{
  GT g;
  auto n0 = g.insert_node(0);
  g.insert_arc(n0, n0, 1);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n0, path);

  // Start == end: trivially reachable, distance 0
  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 5: No path exists (disconnected)
// ============================================================================
TEST(ZeroOneBFS, NoPathExists)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  // n2 is disconnected

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n2, path);

  EXPECT_EQ(d, std::numeric_limits<int>::max());
  EXPECT_TRUE(path.is_empty());
}

// ============================================================================
// TEST 6: Single node graph
// ============================================================================
TEST(ZeroOneBFS, SingleNode)
{
  GT g;
  auto n0 = g.insert_node(0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n0, path);

  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 7: Linear chain
// ============================================================================
TEST(ZeroOneBFS, LinearChain)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n3, n4, 0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n4, path);

  EXPECT_EQ(d, 2);  // 1 + 0 + 1 + 0
}

// ============================================================================
// TEST 8: 0-weight shortcut
// ============================================================================
TEST(ZeroOneBFS, ZeroWeightShortcut)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  // Long path: n0->n1->n2->n3 costs 1+1+1=3
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);

  // Shortcut: n0->n3 via zero-cost edges
  auto n4 = g.insert_node(4);
  g.insert_arc(n0, n4, 0);
  g.insert_arc(n4, n3, 0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  EXPECT_EQ(d, 0);  // Free path via n4
}

// ============================================================================
// TEST 9: Null node validation
// ============================================================================
TEST(ZeroOneBFS, NullNodeValidation)
{
  GT g;
  auto n0 = g.insert_node(0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);

  EXPECT_THROW(bfs(g, nullptr, n0, path), std::domain_error);
  EXPECT_THROW(bfs(g, n0, nullptr, path), std::domain_error);
}

// ============================================================================
// TEST 10: Empty graph validation
// ============================================================================
TEST(ZeroOneBFS, EmptyGraphValidation)
{
  GT g;

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);

  EXPECT_EQ(g.get_num_nodes(), 0u);

  // According to API, search nodes must be non-null and belong to g.
  // For empty graph, any node is invalid.
  Node * n = nullptr;
  EXPECT_THROW(bfs(g, n, n, path), std::domain_error);
  EXPECT_TRUE(path.is_empty());
}

// ============================================================================
// TEST 11: Invalid weight validation
// ============================================================================
TEST(ZeroOneBFS, InvalidWeightValidation)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  auto a01 = g.insert_arc(n0, n1, 2);  // Invalid weight!

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);

  EXPECT_THROW(bfs(g, n0, n1, path), std::domain_error);
  EXPECT_EQ(NODE_COOKIE(n0), nullptr);
  EXPECT_EQ(NODE_COOKIE(n1), nullptr);
  EXPECT_FALSE(IS_NODE_VISITED(n0, Aleph::Spanning_Tree));
  EXPECT_FALSE(IS_NODE_VISITED(n1, Aleph::Spanning_Tree));
  EXPECT_FALSE(IS_ARC_VISITED(a01, Aleph::Spanning_Tree));
}

// ============================================================================
// TEST 12: Paint and then get_min_path
// ============================================================================
TEST(ZeroOneBFS, PaintAndGetMinPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 1);

  Zero_One_BFS<GT> bfs;
  bfs.paint_min_paths_tree(g, n0);

  EXPECT_TRUE(bfs.is_painted());
  EXPECT_EQ(bfs.get_start_node(), n0);

  Path<GT> path(g);
  int d = bfs.get_min_path(n3, path);
  EXPECT_EQ(d, 2);
  EXPECT_FALSE(path.is_empty());
}

// ============================================================================
// TEST 13: Get distance after painting
// ============================================================================
TEST(ZeroOneBFS, GetDistanceAfterPainting)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 1);

  Zero_One_BFS<GT> bfs;
  bfs.paint_min_paths_tree(g, n0);

  EXPECT_EQ(bfs.get_distance(n0), 0);
  EXPECT_EQ(bfs.get_distance(n1), 1);
  EXPECT_EQ(bfs.get_distance(n2), 1);
  EXPECT_EQ(bfs.get_distance(n3), 2);
}

// ============================================================================
// TEST 14: Get distance before painting
// ============================================================================
TEST(ZeroOneBFS, GetDistanceBeforePainting)
{
  GT g;
  auto n0 = g.insert_node(0);

  Zero_One_BFS<GT> bfs;
  EXPECT_THROW(bfs.get_distance(n0), std::domain_error);
}

// ============================================================================
// TEST 15: Path node-by-node verification
// ============================================================================
TEST(ZeroOneBFS, PathNodeVerification)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n0, n3, 1);  // direct path, same cost as via n1,n2

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  EXPECT_EQ(d, 1);

  // Collect path nodes
  DynList<int> nodes;
  for (Path<GT>::Iterator it(path); it.has_curr(); it.next())
    nodes.append(it.get_current_node()->get_info());

  // Path should be n0 -> n3 (direct)
  ASSERT_EQ(nodes.size(), 2u);
  auto pit = nodes.get_it();
  EXPECT_EQ(pit.get_curr(), 0); pit.next();
  EXPECT_EQ(pit.get_curr(), 3);
}

// ============================================================================
// TEST 16: Complete graph K4 with 0/1 weights
// ============================================================================
TEST(ZeroOneBFS, CompleteGraphK4)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 0);
  g.insert_arc(n0, n3, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n1, n3, 0);
  g.insert_arc(n2, n3, 1);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  // Best: n0->n2(0)->n3(1)? No, n0->n1(1)->n3(0)=1, n0->n2(0)+n2->n3(1)=1
  // or n0->n3(1)=1. All paths cost 1.
  EXPECT_EQ(d, 1);
}

// ============================================================================
// TEST 17: Graph with cycle
// ============================================================================
TEST(ZeroOneBFS, GraphWithCycle)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n0, 1);  // cycle back
  g.insert_arc(n2, n3, 0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n3, path);

  // Undirected: n0->n2(via arc n2-n0, weight 1)->n3(weight 0) = 1
  EXPECT_EQ(d, 1);
}

// ============================================================================
// TEST 18: Directed graph basic
// ============================================================================
TEST(ZeroOneBFS, DigraphBasic)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);

  Zero_One_BFS<DGT> bfs;
  Path<DGT> path(g);
  int d = bfs(g, n0, n2, path);

  EXPECT_EQ(d, 1);
}

// ============================================================================
// TEST 19: Directed graph no reverse path
// ============================================================================
TEST(ZeroOneBFS, DigraphNoReversePath)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1);  // only forward

  Zero_One_BFS<DGT> bfs;

  Path<DGT> path_fwd(g);
  int d1 = bfs(g, n0, n1, path_fwd);
  EXPECT_EQ(d1, 1);

  Path<DGT> path_bck(g);
  int d2 = bfs(g, n1, n0, path_bck);
  EXPECT_EQ(d2, std::numeric_limits<int>::max());
}

// ============================================================================
// TEST 20: Multiple successive computations
// ============================================================================
TEST(ZeroOneBFS, MultipleComputations)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 0);

  Zero_One_BFS<GT> bfs;

  Path<GT> p1(g);
  int d1 = bfs(g, n0, n2, p1);
  EXPECT_EQ(d1, 1);

  Path<GT> p2(g);
  int d2 = bfs(g, n2, n0, p2);
  EXPECT_EQ(d2, 1);  // Same in undirected
}

// ============================================================================
// TEST 21: Large linear graph
// ============================================================================
TEST(ZeroOneBFS, LargeLinearGraph)
{
  GT g;
  const int N = 200;

  auto nodes = Aleph::Array<Node *>::create(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Alternating 0 and 1 weights
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], i % 2);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, nodes[0], nodes[N - 1], path);

  // Count of 1-weight edges: indices 1, 3, 5, ... (odd indices)
  int expected = (N - 1) / 2;
  EXPECT_EQ(d, expected);
}

// ============================================================================
// TEST 22: Cross-validation with Dijkstra
// ============================================================================
TEST(ZeroOneBFS, CrossValidationWithDijkstra)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n2, n3, 0);
  g.insert_arc(n3, n4, 1);
  g.insert_arc(n1, n5, 0);
  g.insert_arc(n5, n4, 0);

  // 0-1 BFS
  Zero_One_BFS<GT> bfs;
  Path<GT> bfs_path(g);
  int bfs_d = bfs(g, n0, n4, bfs_path);

  // Dijkstra (should give same result)
  using IGT = GT;  // same type
  Dijkstra_Min_Paths<IGT> dij;
  Path<IGT> dij_path(g);
  int dij_d = dij(g, n0, n4, dij_path);

  EXPECT_EQ(bfs_d, dij_d);
}

// ============================================================================
// TEST 23: Star graph with mixed weights
// ============================================================================
TEST(ZeroOneBFS, StarGraph)
{
  GT g;
  auto center = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(center, n1, 0);
  g.insert_arc(center, n2, 1);
  g.insert_arc(center, n3, 0);
  g.insert_arc(center, n4, 1);

  Zero_One_BFS<GT> bfs;
  bfs.paint_min_paths_tree(g, center);

  EXPECT_EQ(bfs.get_distance(n1), 0);
  EXPECT_EQ(bfs.get_distance(n2), 1);
  EXPECT_EQ(bfs.get_distance(n3), 0);
  EXPECT_EQ(bfs.get_distance(n4), 1);
}

// ============================================================================
// TEST 24: Grid-like graph (illustrative example)
// ============================================================================
// === EXAMPLE: Grid navigation with free/paid passages ===
// Imagine a 3x3 grid where some passages are free (weight 0)
// and others cost 1 unit. Find shortest path from top-left to
// bottom-right.
//
//   (0,0) --0-- (0,1) --1-- (0,2)
//     |           |           |
//     1           0           1
//     |           |           |
//   (1,0) --1-- (1,1) --0-- (1,2)
//     |           |           |
//     0           1           0
//     |           |           |
//   (2,0) --0-- (2,1) --0-- (2,2)
//
TEST(ZeroOneBFS, GridExample)
{
  GT g;

  // Create 3x3 grid nodes (row * 3 + col as node info)
  Node * grid[3][3];
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      grid[r][c] = g.insert_node(r * 3 + c);

  // Horizontal edges
  g.insert_arc(grid[0][0], grid[0][1], 0);
  g.insert_arc(grid[0][1], grid[0][2], 1);
  g.insert_arc(grid[1][0], grid[1][1], 1);
  g.insert_arc(grid[1][1], grid[1][2], 0);
  g.insert_arc(grid[2][0], grid[2][1], 0);
  g.insert_arc(grid[2][1], grid[2][2], 0);

  // Vertical edges
  g.insert_arc(grid[0][0], grid[1][0], 1);
  g.insert_arc(grid[0][1], grid[1][1], 0);
  g.insert_arc(grid[0][2], grid[1][2], 1);
  g.insert_arc(grid[1][0], grid[2][0], 0);
  g.insert_arc(grid[1][1], grid[2][1], 1);
  g.insert_arc(grid[1][2], grid[2][2], 0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, grid[0][0], grid[2][2], path);

  // Best path: (0,0)->0->(0,1)->0->(1,1)->0->(1,2)->0->(2,2) = 0
  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 25: Two-node graph weight 0
// ============================================================================
TEST(ZeroOneBFS, TwoNodeZeroWeight)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 0);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n1, path);

  EXPECT_EQ(d, 0);
}

// ============================================================================
// TEST 26: Two-node graph weight 1
// ============================================================================
TEST(ZeroOneBFS, TwoNodeOneWeight)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, n0, n1, path);

  EXPECT_EQ(d, 1);
}

// ============================================================================
// TEST 27: Negative weight validation
// ============================================================================
TEST(ZeroOneBFS, NegativeWeightValidation)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  auto a01 = g.insert_arc(n0, n1, -1);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);

  EXPECT_THROW(bfs(g, n0, n1, path), std::domain_error);
  EXPECT_EQ(NODE_COOKIE(n0), nullptr);
  EXPECT_EQ(NODE_COOKIE(n1), nullptr);
  EXPECT_FALSE(IS_NODE_VISITED(n0, Aleph::Spanning_Tree));
  EXPECT_FALSE(IS_NODE_VISITED(n1, Aleph::Spanning_Tree));
  EXPECT_FALSE(IS_ARC_VISITED(a01, Aleph::Spanning_Tree));
}

// ============================================================================
// TEST 28: State getters
// ============================================================================
TEST(ZeroOneBFS, StateGetters)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Zero_One_BFS<GT> bfs;

  EXPECT_FALSE(bfs.is_painted());
  EXPECT_EQ(bfs.get_start_node(), nullptr);
  EXPECT_EQ(bfs.get_graph(), nullptr);

  bfs.paint_min_paths_tree(g, n0);

  EXPECT_TRUE(bfs.is_painted());
  EXPECT_EQ(bfs.get_start_node(), n0);
  EXPECT_EQ(bfs.get_graph(), &g);
}

// ============================================================================
// TEST 29: Disconnected graph painting
// ============================================================================
TEST(ZeroOneBFS, DisconnectedPainting)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n2, n3, 0);

  Zero_One_BFS<GT> bfs;
  bfs.paint_min_paths_tree(g, n0);

  EXPECT_EQ(bfs.get_distance(n1), 1);
  EXPECT_THROW(bfs.get_distance(n2), std::domain_error);
}

// ============================================================================
// TEST 30: Large graph with many zero edges
// ============================================================================
TEST(ZeroOneBFS, LargeGraphManyZeros)
{
  GT g;
  const int N = 100;

  auto nodes = Aleph::Array<Node *>::create(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Chain with all zero weights
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 0);

  // Add a single 1-weight shortcut
  g.insert_arc(nodes[0], nodes[N - 1], 1);

  Zero_One_BFS<GT> bfs;
  Path<GT> path(g);
  int d = bfs(g, nodes[0], nodes[N - 1], path);

  EXPECT_EQ(d, 0);  // Zero-cost chain is better than weight-1 shortcut
}

// ============================================================================
// TEST 31: Parallel edges must not leave stale spanning-tree arc marks
// ============================================================================
TEST(ZeroOneBFS, ParallelEdgesKeepSingleParentArcMarked)
{
  GT g;
  auto start = g.insert_node(0);
  auto mid = g.insert_node(1);
  auto end = g.insert_node(2);

  auto direct_one = g.insert_arc(start, end, 1);
  auto start_mid_zero = g.insert_arc(start, mid, 0);
  auto mid_end_one = g.insert_arc(mid, end, 1);
  auto mid_end_zero = g.insert_arc(mid, end, 0); // Parallel arc, better cost

  Zero_One_BFS<GT> bfs;
  bfs.paint_min_paths_tree(g, start);

  EXPECT_EQ(bfs.get_distance(end), 0);
  EXPECT_TRUE(IS_NODE_VISITED(mid, Aleph::Spanning_Tree));
  EXPECT_TRUE(IS_NODE_VISITED(end, Aleph::Spanning_Tree));

  EXPECT_TRUE(IS_ARC_VISITED(start_mid_zero, Aleph::Spanning_Tree));
  EXPECT_TRUE(IS_ARC_VISITED(mid_end_zero, Aleph::Spanning_Tree));
  EXPECT_FALSE(IS_ARC_VISITED(direct_one, Aleph::Spanning_Tree));
  EXPECT_FALSE(IS_ARC_VISITED(mid_end_one, Aleph::Spanning_Tree));

  size_t painted_arcs = 0;
  for (GT::Arc_Iterator it(g); it.has_curr(); it.next())
    if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree))
      ++painted_arcs;

  EXPECT_EQ(painted_arcs, 2u);
}
