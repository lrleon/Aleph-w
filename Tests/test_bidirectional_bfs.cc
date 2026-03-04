/**
 * @file test_bidirectional_bfs.cc
 * @brief Comprehensive tests for Bidirectional_BFS.H
 *
 * Tests cover correctness, edge cases, validation, and cross-checks
 * against standard BFS for unweighted shortest paths.
 */

# include <gtest/gtest.h>
# include <Bidirectional_BFS.H>
# include <tpl_find_path.H>
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

// Helper: count path length (number of edges)
template <class G>
size_t path_edge_count(Path<G> & p)
{
  if (p.is_empty())
    return 0;
  size_t count = 0;
  for (typename Path<G>::Iterator it(p); it.has_curr(); it.next())
    ++count;
  return count > 0 ? count - 1 : 0;  // nodes - 1 = edges
}

// ============================================================================
// TEST 1: Basic shortest path
// ============================================================================
TEST(BidirectionalBFS, BasicShortestPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);
  g.insert_arc(n0, n3, 0);  // direct edge

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n3, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 1u);  // direct edge n0->n3
}

// ============================================================================
// TEST 2: Path to self
// ============================================================================
TEST(BidirectionalBFS, PathToSelf)
{
  GT g;
  auto n0 = g.insert_node(0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n0, path);

  EXPECT_TRUE(found);
}

// ============================================================================
// TEST 3: No path exists
// ============================================================================
TEST(BidirectionalBFS, NoPathExists)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n1, path);

  EXPECT_FALSE(found);
}

// ============================================================================
// TEST 4: Single node
// ============================================================================
TEST(BidirectionalBFS, SingleNode)
{
  GT g;
  auto n0 = g.insert_node(0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n0, path);

  EXPECT_TRUE(found);
}

// ============================================================================
// TEST 5: Linear chain
// ============================================================================
TEST(BidirectionalBFS, LinearChain)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n3, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 3u);
}

// ============================================================================
// TEST 6: Complete graph K4
// ============================================================================
TEST(BidirectionalBFS, CompleteGraphK4)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n2, 0);
  g.insert_arc(n0, n3, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n1, n3, 0);
  g.insert_arc(n2, n3, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n3, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 1u);  // direct edge
}

// ============================================================================
// TEST 7: Null node validation
// ============================================================================
TEST(BidirectionalBFS, NullNodeValidation)
{
  GT g;
  auto n0 = g.insert_node(0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);

  EXPECT_THROW(bibfs(g, nullptr, n0, path), std::domain_error);
  EXPECT_THROW(bibfs(g, n0, nullptr, path), std::domain_error);
}

// ============================================================================
// TEST 8: Disconnected components
// ============================================================================
TEST(BidirectionalBFS, DisconnectedComponents)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n2, n3, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n3, path);

  EXPECT_FALSE(found);
}

// ============================================================================
// TEST 9: Two-node graph
// ============================================================================
TEST(BidirectionalBFS, TwoNodeGraph)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n1, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 1u);
}

// ============================================================================
// TEST 10: Graph with cycle
// ============================================================================
TEST(BidirectionalBFS, GraphWithCycle)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  // Cycle: n0-n1-n2-n3-n0
  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);
  g.insert_arc(n3, n0, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n2, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 2u);  // n0->n1->n2 or n0->n3->n2
}

// ============================================================================
// TEST 11: Odd-length path
// ============================================================================
TEST(BidirectionalBFS, OddLengthPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n2, path);

  ASSERT_TRUE(found);

  // Verify exact path: n0 -> n1 -> n2
  DynList<int> nodes;
  for (Path<GT>::Iterator it(path); it.has_curr(); it.next())
    nodes.append(it.get_current_node()->get_info());

  ASSERT_EQ(nodes.size(), 3u);
  auto pit = nodes.get_it();
  EXPECT_EQ(pit.get_curr(), 0); pit.next();
  EXPECT_EQ(pit.get_curr(), 1); pit.next();
  EXPECT_EQ(pit.get_curr(), 2);
}

// ============================================================================
// TEST 12: Even-length path
// ============================================================================
TEST(BidirectionalBFS, EvenLengthPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n3, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 3u);
}

// ============================================================================
// TEST 13: Cross-validation with standard BFS
// ============================================================================
TEST(BidirectionalBFS, CrossValidationWithBFS)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n2, 0);
  g.insert_arc(n1, n3, 0);
  g.insert_arc(n2, n4, 0);
  g.insert_arc(n3, n4, 0);

  // Bidirectional BFS
  Bidirectional_BFS<GT> bibfs;
  Path<GT> bi_path(g);
  bool bi_found = bibfs(g, n0, n4, bi_path);

  // Standard BFS
  Find_Path_Breadth_First<GT> std_bfs;
  Path<GT> std_path(g);
  bool std_found = std_bfs(g, n0, n4, std_path);

  EXPECT_EQ(bi_found, std_found);
  if (bi_found and std_found)
    EXPECT_EQ(path_edge_count(bi_path), path_edge_count(std_path));
}

// ============================================================================
// TEST 14: Multiple successive computations
// ============================================================================
TEST(BidirectionalBFS, MultipleComputations)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  Bidirectional_BFS<GT> bibfs;

  Path<GT> p1(g);
  EXPECT_TRUE(bibfs(g, n0, n2, p1));

  Path<GT> p2(g);
  EXPECT_TRUE(bibfs(g, n2, n0, p2));

  EXPECT_EQ(path_edge_count(p1), path_edge_count(p2));
}

// ============================================================================
// TEST 15: Large graph
// ============================================================================
TEST(BidirectionalBFS, LargeGraph)
{
  GT g;
  const int N = 200;

  std::vector<Node *> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, nodes[0], nodes[N - 1], path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), static_cast<size_t>(N - 1));
}

// ============================================================================
// TEST 16: Operator returning Path
// ============================================================================
TEST(BidirectionalBFS, OperatorReturnsPath)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  Bidirectional_BFS<GT> bibfs;
  auto path = bibfs(g, n0, n2);

  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(path_edge_count(path), 2u);
}

// ============================================================================
// TEST 17: Star graph
// ============================================================================
TEST(BidirectionalBFS, StarGraph)
{
  GT g;
  auto center = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(center, n1, 0);
  g.insert_arc(center, n2, 0);
  g.insert_arc(center, n3, 0);
  g.insert_arc(center, n4, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n1, n4, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 2u);  // n1->center->n4
}

// ============================================================================
// TEST 18: Binary tree-like graph
// ============================================================================
TEST(BidirectionalBFS, BinaryTreeGraph)
{
  GT g;
  //         0
  //        / \          (backslash)
  //       1   2
  //      /|   |\        (backslash)
  //     3  4 5  6
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  auto n6 = g.insert_node(6);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n0, n2, 0);
  g.insert_arc(n1, n3, 0);
  g.insert_arc(n1, n4, 0);
  g.insert_arc(n2, n5, 0);
  g.insert_arc(n2, n6, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n3, n6, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 4u);  // n3->n1->n0->n2->n6
}

// ============================================================================
// TEST 19: Digraph forward only
// ============================================================================
TEST(BidirectionalBFS, DigraphForward)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  // For digraphs, we need Out_Iterator for forward and In_Iterator for backward
  Bidirectional_BFS<DGT, Out_Iterator, In_Iterator> bibfs;
  Path<DGT> path(g);
  bool found = bibfs(g, n0, n2, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 2u);
}

// ============================================================================
// TEST 20: Digraph no backward path
// ============================================================================
TEST(BidirectionalBFS, DigraphNoBackward)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 0);

  Bidirectional_BFS<DGT, Out_Iterator, In_Iterator> bibfs;
  Path<DGT> p2(g);
  bool found = bibfs(g, n1, n0, p2);

  EXPECT_FALSE(found);
}

// ============================================================================
// TEST 21: Digraph with In_Iterator backward search
// ============================================================================
TEST(BidirectionalBFS, DigraphInIteratorBackward)
{
  DGT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);

  Bidirectional_BFS<DGT, Out_Iterator, In_Iterator> bibfs;
  Path<DGT> path(g);
  bool found = bibfs(g, n0, n3, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 3u);

  // Verify path
  DynList<int> nodes;
  for (Path<DGT>::Iterator it(path); it.has_curr(); it.next())
    nodes.append(it.get_current_node()->get_info());

  ASSERT_EQ(nodes.size(), 4u);
  auto pit = nodes.get_it();
  EXPECT_EQ(pit.get_curr(), 0); pit.next();
  EXPECT_EQ(pit.get_curr(), 1); pit.next();
  EXPECT_EQ(pit.get_curr(), 2); pit.next();
  EXPECT_EQ(pit.get_curr(), 3);
}

// ============================================================================
// === EXAMPLE: Social network ===
// Find the shortest connection between two people in a social network.
//
//   Alice -- Bob -- Charlie
//     |              |
//   Diana -- Eve -- Frank
//
// Shortest Alice->Frank: Alice->Diana->Eve->Frank (3 hops)
// or Alice->Bob->Charlie->Frank (3 hops)
// ============================================================================
TEST(BidirectionalBFS, SocialNetworkExample)
{
  GT g;
  auto alice   = g.insert_node(0);  // Alice
  auto bob     = g.insert_node(1);  // Bob
  auto charlie = g.insert_node(2);  // Charlie
  auto diana   = g.insert_node(3);  // Diana
  auto eve     = g.insert_node(4);  // Eve
  auto frank   = g.insert_node(5);  // Frank

  g.insert_arc(alice, bob, 0);
  g.insert_arc(bob, charlie, 0);
  g.insert_arc(charlie, frank, 0);
  g.insert_arc(alice, diana, 0);
  g.insert_arc(diana, eve, 0);
  g.insert_arc(eve, frank, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, alice, frank, path);

  ASSERT_TRUE(found);
  EXPECT_EQ(path_edge_count(path), 3u);
}

// ============================================================================
// TEST 23: Path correctness (first and last node)
// ============================================================================
TEST(BidirectionalBFS, PathEndsCorrectness)
{
  GT g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);
  g.insert_arc(n3, n4, 0);

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, n0, n4, path);

  ASSERT_TRUE(found);

  // Verify first node is n0 and last is n4
  DynList<int> nodes;
  for (Path<GT>::Iterator it(path); it.has_curr(); it.next())
    nodes.append(it.get_current_node()->get_info());

  ASSERT_GE(nodes.size(), 2u);
  EXPECT_EQ(nodes.get_first(), 0);
  EXPECT_EQ(nodes.get_last(), 4);
}

// ============================================================================
// TEST 24: Cross-validation with BFS on various endpoints
// ============================================================================
TEST(BidirectionalBFS, CrossValidationMultipleEndpoints)
{
  GT g;
  const int N = 10;

  std::vector<Node *> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Create a connected graph
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 0);

  // Add some shortcuts
  g.insert_arc(nodes[0], nodes[5], 0);
  g.insert_arc(nodes[3], nodes[8], 0);

  for (int start = 0; start < N; start += 3)
    for (int end = start + 1; end < N; end += 2)
      {
        Bidirectional_BFS<GT> bibfs;
        Find_Path_Breadth_First<GT> std_bfs;

        Path<GT> bi_path(g), std_path(g);
        bool bi_found = bibfs(g, nodes[start], nodes[end], bi_path);
        bool std_found = std_bfs(g, nodes[start], nodes[end], std_path);

        EXPECT_EQ(bi_found, std_found)
          << "Mismatch for start=" << start << " end=" << end;
        if (bi_found and std_found)
          EXPECT_EQ(path_edge_count(bi_path), path_edge_count(std_path))
            << "Length mismatch for start=" << start << " end=" << end;
      }
}

// ============================================================================
// TEST 25: Grid graph
// ============================================================================
TEST(BidirectionalBFS, GridGraph)
{
  GT g;
  const int ROWS = 4, COLS = 4;
  Node * grid[ROWS][COLS];

  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c)
      grid[r][c] = g.insert_node(r * COLS + c);

  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c)
      {
        if (c + 1 < COLS)
          g.insert_arc(grid[r][c], grid[r][c + 1], 0);
        if (r + 1 < ROWS)
          g.insert_arc(grid[r][c], grid[r + 1][c], 0);
      }

  Bidirectional_BFS<GT> bibfs;
  Path<GT> path(g);
  bool found = bibfs(g, grid[0][0], grid[ROWS - 1][COLS - 1], path);

  ASSERT_TRUE(found);
  // Manhattan distance = (ROWS-1) + (COLS-1)
  EXPECT_EQ(path_edge_count(path), static_cast<size_t>(ROWS + COLS - 2));
}

// ============================================================================
// GoogleTest main
// ============================================================================
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
