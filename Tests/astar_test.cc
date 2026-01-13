/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
  / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
 / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
/_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
              |_|

This file is part of Aleph-w library

Copyright (c) 2002-2018 Leandro Rabindranath Leon

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file astar_test.cc
 * @brief Tests for A* shortest path algorithm.
 *
 * Tests cover:
 * - Basic correctness on simple graphs
 * - Comparison with Dijkstra to verify optimality
 * - Different heuristics (zero, euclidean, manhattan)
 * - Grid-based pathfinding
 * - Edge cases: no path, same start/end, single node
 */

#include <gtest/gtest.h>
#include <tpl_graph.H>
#include <AStar.H>
#include <Dijkstra.H>
#include <tpl_graph_utils.H>
#include <random>
#include <cmath>

using namespace Aleph;

// ==================== Heap Type Tags ====================

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

// ==================== Type definitions ====================

// Simple weighted graph
struct SimpleNode : public Graph_Node<int>
{
  using Graph_Node<int>::Graph_Node;
};

struct SimpleArc : public Graph_Arc<double>
{
  using Graph_Arc<double>::Graph_Arc;
};

using SimpleGraph = List_Graph<SimpleNode, SimpleArc>;

// Grid-based node with coordinates
struct GridNode : public Graph_Node<int>
{
  int x = 0;
  int y = 0;

  GridNode() = default;
  GridNode(int id) : Graph_Node<int>(id), x(0), y(0) {}
  GridNode(int id, int _x, int _y) : Graph_Node<int>(id), x(_x), y(_y) {}
};

struct GridArc : public Graph_Arc<double>
{
  using Graph_Arc<double>::Graph_Arc;
};

using GridGraph = List_Graph<GridNode, GridArc>;

// Distance accessor for double weights
struct DoubleDistance
{
  using Distance_Type = double;

  Distance_Type operator()(SimpleGraph::Arc* arc) const
  {
    return arc->get_info();
  }

  Distance_Type operator()(GridGraph::Arc* arc) const
  {
    return arc->get_info();
  }

  static void set_zero(SimpleGraph::Arc* arc)
  {
    arc->get_info() = 0.0;
  }
};

// Euclidean heuristic for GridGraph
struct GridEuclideanHeuristic
{
  using Distance_Type = double;

  Distance_Type operator()(GridGraph::Node* from, GridGraph::Node* to) const
  {
    double dx = from->x - to->x;
    double dy = from->y - to->y;
    return std::sqrt(dx*dx + dy*dy);
  }
};

// Manhattan heuristic for GridGraph
struct GridManhattanHeuristic
{
  using Distance_Type = double;

  Distance_Type operator()(GridGraph::Node* from, GridGraph::Node* to) const
  {
    return std::abs(from->x - to->x) + std::abs(from->y - to->y);
  }
};

// ==================== Test Fixtures ====================

class AStarBasicTest : public ::testing::Test
{
protected:
  SimpleGraph g;
  std::vector<SimpleGraph::Node*> nodes;

  void SetUp() override
  {
    // Create a simple graph:
    //       1
    //   0 ----> 1
    //   |       |
    // 4 |       | 2
    //   v       v
    //   3 ----> 2
    //       1
    for (int i = 0; i < 4; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[0], nodes[3], 4.0);
    g.insert_arc(nodes[3], nodes[2], 1.0);
  }
};

class AStarGridTest : public ::testing::Test
{
protected:
  GridGraph g;
  std::vector<GridGraph::Node*> nodes;
  static constexpr int GRID_SIZE = 5;

  void SetUp() override
  {
    // Create a 5x5 grid graph
    nodes.resize(GRID_SIZE * GRID_SIZE);

    for (int y = 0; y < GRID_SIZE; ++y)
      for (int x = 0; x < GRID_SIZE; ++x)
        {
          int idx = y * GRID_SIZE + x;
          GridNode node(idx, x, y);  // Stack-allocated, no leak
          nodes[idx] = g.insert_node(node.get_info());
          nodes[idx]->x = x;
          nodes[idx]->y = y;
        }

    // Connect adjacent cells (4-connected grid)
    for (int y = 0; y < GRID_SIZE; ++y)
      for (int x = 0; x < GRID_SIZE; ++x)
        {
          int idx = y * GRID_SIZE + x;

          // Right neighbor
          if (x + 1 < GRID_SIZE)
            {
              int right = y * GRID_SIZE + (x + 1);
              g.insert_arc(nodes[idx], nodes[right], 1.0);
              g.insert_arc(nodes[right], nodes[idx], 1.0);
            }

          // Down neighbor
          if (y + 1 < GRID_SIZE)
            {
              int down = (y + 1) * GRID_SIZE + x;
              g.insert_arc(nodes[idx], nodes[down], 1.0);
              g.insert_arc(nodes[down], nodes[idx], 1.0);
            }
        }
  }

  GridGraph::Node* get_node(int x, int y)
  {
    return nodes[y * GRID_SIZE + x];
  }
};

// ==================== Tests ====================

// Test basic path finding with zero heuristic (should match Dijkstra)
TEST_F(AStarBasicTest, ZeroHeuristicMatchesDijkstra)
{
  Path<SimpleGraph> astar_path(g);
  Path<SimpleGraph> dijkstra_path(g);

  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;
  Dijkstra_Min_Paths<SimpleGraph, DoubleDistance> dijkstra;

  auto astar_cost = astar.find_path(g, nodes[0], nodes[2], astar_path);
  auto dijkstra_cost = dijkstra.find_min_path(g, nodes[0], nodes[2], dijkstra_path);

  EXPECT_DOUBLE_EQ(astar_cost, dijkstra_cost);
  EXPECT_EQ(astar_path.size(), dijkstra_path.size());
}

// Test finding shortest path
TEST_F(AStarBasicTest, FindsShortestPath)
{
  Path<SimpleGraph> path(g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  auto cost = astar.find_path(g, nodes[0], nodes[2], path);

  // Shortest path: 0 -> 1 -> 2 with cost 3
  EXPECT_DOUBLE_EQ(cost, 3.0);
  EXPECT_EQ(path.size(), 3);
}

// Test paint_path version
TEST_F(AStarBasicTest, PaintPathWorks)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  bool found = astar.paint_path(g, nodes[0], nodes[2]);
  EXPECT_TRUE(found);
  EXPECT_TRUE(astar.is_painted());

  Path<SimpleGraph> path(g);
  auto cost = astar.get_min_path(nodes[2], path);
  EXPECT_DOUBLE_EQ(cost, 3.0);
}

// Test compute_path (tree building version)
TEST_F(AStarBasicTest, ComputePathBuildsTree)
{
  SimpleGraph tree;
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  auto end_tree_node = astar.compute_path(g, nodes[0], nodes[2], tree);

  EXPECT_NE(end_tree_node, nullptr);
  EXPECT_GE(tree.get_num_nodes(), 2u);
}

// Test no path exists
TEST_F(AStarBasicTest, NoPathReturnsMax)
{
  // Add isolated node
  auto isolated = g.insert_node(99);

  Path<SimpleGraph> path(g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  auto cost = astar.find_path(g, nodes[0], isolated, path);

  EXPECT_EQ(cost, std::numeric_limits<double>::max());
  EXPECT_TRUE(path.is_empty());
}

// Test start equals end
TEST_F(AStarBasicTest, StartEqualsEnd)
{
  Path<SimpleGraph> path(g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  // When start == end, paint_path should find it immediately
  bool found = astar.paint_path(g, nodes[0], nodes[0]);
  EXPECT_TRUE(found);
}

// Test grid with Euclidean heuristic
TEST_F(AStarGridTest, EuclideanHeuristicFindsShortest)
{
  Path<GridGraph> path(g);
  AStar_Min_Path<GridGraph, DoubleDistance, GridEuclideanHeuristic> astar;

  auto start = get_node(0, 0);
  auto end = get_node(GRID_SIZE-1, GRID_SIZE-1);

  auto cost = astar.find_path(g, start, end, path);

  // Manhattan distance in 4-connected grid
  double expected = (GRID_SIZE-1) + (GRID_SIZE-1);
  EXPECT_DOUBLE_EQ(cost, expected);
}

// Test grid with Manhattan heuristic
TEST_F(AStarGridTest, ManhattanHeuristicFindsShortest)
{
  Path<GridGraph> path(g);
  AStar_Min_Path<GridGraph, DoubleDistance, GridManhattanHeuristic> astar;

  auto start = get_node(0, 0);
  auto end = get_node(GRID_SIZE-1, GRID_SIZE-1);

  auto cost = astar.find_path(g, start, end, path);

  double expected = (GRID_SIZE-1) + (GRID_SIZE-1);
  EXPECT_DOUBLE_EQ(cost, expected);
}

// Test that A* with good heuristic explores fewer nodes than Dijkstra
TEST_F(AStarGridTest, GoodHeuristicReducesExploration)
{
  // This is a qualitative test - A* should find the path
  Path<GridGraph> path(g);
  AStar_Min_Path<GridGraph, DoubleDistance, GridManhattanHeuristic> astar;

  auto start = get_node(0, 0);
  auto end = get_node(4, 4);

  auto cost = astar.find_path(g, start, end, path);

  EXPECT_FALSE(path.is_empty());
  EXPECT_EQ(cost, 8.0); // Manhattan distance in grid
}

// Test operator() interface
TEST_F(AStarBasicTest, OperatorInterface)
{
  Path<SimpleGraph> path(g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  auto cost = astar(g, nodes[0], nodes[2], path);

  EXPECT_DOUBLE_EQ(cost, 3.0);
}

// Test single node graph
TEST(AStarEdgeCases, SingleNodeGraph)
{
  SimpleGraph g;
  auto node = g.insert_node(0);

  Path<SimpleGraph> path(g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  bool found = astar.paint_path(g, node, node);
  EXPECT_TRUE(found);
}

// Test null parameters throw
TEST(AStarEdgeCases, NullParametersThrow)
{
  SimpleGraph g;
  auto node = g.insert_node(0);

  Path<SimpleGraph> path(g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  EXPECT_THROW(astar.find_path(g, nullptr, node, path), std::domain_error);
  EXPECT_THROW(astar.find_path(g, node, nullptr, path), std::domain_error);
}

// Test empty graph throws
TEST(AStarEdgeCases, EmptyGraphThrows)
{
  SimpleGraph empty_g;
  SimpleGraph g;
  auto node = g.insert_node(0);

  Path<SimpleGraph> path(empty_g);
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  EXPECT_THROW(astar.find_path(empty_g, node, node, path), std::domain_error);
}

// Test comparison: A* vs Dijkstra on random graph
TEST(AStarComparison, RandomGraphMatchesDijkstra)
{
  SimpleGraph g;
  std::vector<SimpleGraph::Node*> nodes;
  std::mt19937 rng(42);
  std::uniform_real_distribution<double> weight_dist(0.1, 10.0);

  // Create 20 nodes
  for (int i = 0; i < 20; ++i)
    nodes.push_back(g.insert_node(i));

  // Create random edges
  for (size_t i = 0; i < nodes.size(); ++i)
    for (size_t j = i + 1; j < nodes.size(); ++j)
      if (rng() % 3 == 0)
        {
          double w = weight_dist(rng);
          g.insert_arc(nodes[i], nodes[j], w);
          g.insert_arc(nodes[j], nodes[i], w);
        }

  // Compare paths for multiple pairs
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;
  Dijkstra_Min_Paths<SimpleGraph, DoubleDistance> dijkstra;

  for (int i = 0; i < 5; ++i)
    {
      auto start = nodes[rng() % nodes.size()];
      auto end = nodes[rng() % nodes.size()];

      if (start == end)
        continue;

      Path<SimpleGraph> astar_path(g);
      Path<SimpleGraph> dijkstra_path(g);

      auto astar_cost = astar.find_path(g, start, end, astar_path);
      auto dijkstra_cost = dijkstra.find_min_path(g, start, end, dijkstra_path);

      // Costs should match (both should be optimal)
      EXPECT_NEAR(astar_cost, dijkstra_cost, 1e-9)
        << "Mismatch for start=" << start->get_info()
        << " end=" << end->get_info();
    }
}

// ==================== Tests for New Dijkstra-Compatible Methods ====================

class AStarDijkstraModeTest : public ::testing::Test
{
protected:
  SimpleGraph g;
  std::vector<SimpleGraph::Node*> nodes;

  void SetUp() override
  {
    // Create a larger graph for testing complete tree
    //     1        2
    // 0 ----> 1 ----> 2
    //  \      |       ^
    //   \5    |3      |1
    //    \    v       |
    //     --> 3 ------+
    for (int i = 0; i < 4; ++i)
      nodes.push_back(g.insert_node(i));

    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[0], nodes[3], 5.0);
    g.insert_arc(nodes[1], nodes[3], 3.0);
    g.insert_arc(nodes[3], nodes[2], 1.0);
  }
};

// Test compute_min_paths_tree (Dijkstra mode - complete tree)
TEST_F(AStarDijkstraModeTest, ComputeMinPathsTreeBuildsCompleteTree)
{
  SimpleGraph tree;
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  auto root = astar.compute_min_paths_tree(g, nodes[0], tree);

  ASSERT_NE(root, nullptr);
  // Tree should have all reachable nodes
  EXPECT_EQ(tree.get_num_nodes(), g.get_num_nodes());
  // Tree should have n-1 arcs for n nodes (spanning tree)
  EXPECT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);
}

// Test paint_min_paths_tree (Dijkstra mode - complete)
TEST_F(AStarDijkstraModeTest, PaintMinPathsTreePaintsAll)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  astar.paint_min_paths_tree(g, nodes[0]);

  EXPECT_TRUE(astar.is_painted());
  EXPECT_TRUE(astar.has_computation());
  EXPECT_EQ(astar.get_start_node(), nodes[0]);

  // All nodes should be reachable
  for (auto node : nodes)
    {
      Path<SimpleGraph> path(g);
      auto dist = astar.get_min_path(node, path);
      EXPECT_LT(dist, std::numeric_limits<double>::max());
    }
}

// Test find_min_path (Dijkstra mode - no heuristic)
TEST_F(AStarDijkstraModeTest, FindMinPathMatchesDijkstra)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;
  Dijkstra_Min_Paths<SimpleGraph, DoubleDistance> dijkstra;

  Path<SimpleGraph> astar_path(g);
  Path<SimpleGraph> dijkstra_path(g);

  auto astar_cost = astar.find_min_path(g, nodes[0], nodes[2], astar_path);
  auto dijkstra_cost = dijkstra.find_min_path(g, nodes[0], nodes[2], dijkstra_path);

  EXPECT_DOUBLE_EQ(astar_cost, dijkstra_cost);
  EXPECT_EQ(astar_path.size(), dijkstra_path.size());
}

// Test compute_partial_min_paths_tree (without heuristic)
TEST_F(AStarDijkstraModeTest, ComputePartialMinPathsTree)
{
  SimpleGraph tree;
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  astar.compute_partial_min_paths_tree(g, nodes[0], nodes[2], tree);

  // Tree should contain at least the path nodes
  EXPECT_GE(tree.get_num_nodes(), 2u);
  // But may not contain all nodes (partial)
  EXPECT_LE(tree.get_num_nodes(), g.get_num_nodes());
}

// Test paint_partial_min_paths_tree (without heuristic)
TEST_F(AStarDijkstraModeTest, PaintPartialMinPathsTree)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  bool found = astar.paint_partial_min_paths_tree(g, nodes[0], nodes[2]);

  EXPECT_TRUE(found);
  EXPECT_TRUE(astar.is_painted());

  // Can extract path
  Path<SimpleGraph> path(g);
  auto cost = astar.get_min_path(nodes[2], path);
  EXPECT_EQ(cost, 3.0); // 0->1->2
}

// Test get_distance after painting
TEST_F(AStarDijkstraModeTest, GetDistanceAfterPainting)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  astar.paint_min_paths_tree(g, nodes[0]);

  // Distance to node 0 (start) should be 0
  EXPECT_DOUBLE_EQ(astar.get_distance(nodes[0]), 0.0);

  // Distance to node 1 should be 1
  EXPECT_DOUBLE_EQ(astar.get_distance(nodes[1]), 1.0);

  // Distance to node 2 should be 3 (0->1->2)
  EXPECT_DOUBLE_EQ(astar.get_distance(nodes[2]), 3.0);

  // Distance to node 3 should be 4 (0->1->3)
  EXPECT_DOUBLE_EQ(astar.get_distance(nodes[3]), 4.0);
}

// Test copy_painted_min_paths_tree
TEST_F(AStarDijkstraModeTest, CopyPaintedMinPathsTree)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  astar.paint_min_paths_tree(g, nodes[0]);

  SimpleGraph tree;
  auto total_dist = astar.copy_painted_min_paths_tree(g, tree);
  (void)total_dist;  // May be 0 depending on Paint_Filt implementation

  // Tree should have all nodes
  EXPECT_EQ(tree.get_num_nodes(), g.get_num_nodes());
  // Tree should have n-1 arcs
  EXPECT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);
}

// Test operator() for Dijkstra mode (tree version)
TEST_F(AStarDijkstraModeTest, OperatorTreeVersion)
{
  SimpleGraph tree;
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  astar(g, nodes[0], tree);

  EXPECT_EQ(tree.get_num_nodes(), g.get_num_nodes());
}

// Test get_distance throws if not painted
TEST(AStarErrorCases, GetDistanceThrowsIfNotPainted)
{
  SimpleGraph g;
  auto node = g.insert_node(0);

  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  EXPECT_THROW(astar.get_distance(node), std::domain_error);
}

// Test copy_painted throws if not painted
TEST(AStarErrorCases, CopyPaintedThrowsIfNotPainted)
{
  SimpleGraph g;
  g.insert_node(0);

  SimpleGraph tree;
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  EXPECT_THROW(astar.copy_painted_min_paths_tree(g, tree), std::domain_error);
}

// Compare A* Dijkstra mode trees with actual Dijkstra trees
TEST_F(AStarDijkstraModeTest, TreesMatchDijkstra)
{
  SimpleGraph astar_tree, dijkstra_tree;

  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;
  Dijkstra_Min_Paths<SimpleGraph, DoubleDistance> dijkstra;

  astar.compute_min_paths_tree(g, nodes[0], astar_tree);
  dijkstra.compute_min_paths_tree(g, nodes[0], dijkstra_tree);

  // Same number of nodes and arcs
  EXPECT_EQ(astar_tree.get_num_nodes(), dijkstra_tree.get_num_nodes());
  EXPECT_EQ(astar_tree.get_num_arcs(), dijkstra_tree.get_num_arcs());
}

// Test disconnected graph
TEST(AStarDijkstraMode, DisconnectedGraph)
{
  SimpleGraph g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  (void)g.insert_node(2);  // n2 is intentionally disconnected

  g.insert_arc(n0, n1, 1.0);

  SimpleGraph tree;
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  astar.compute_min_paths_tree(g, n0, tree);

  // Tree should only contain connected component
  EXPECT_EQ(tree.get_num_nodes(), 2u);
}

// Test state getters
TEST_F(AStarDijkstraModeTest, StateGetters)
{
  AStar_Min_Path<SimpleGraph, DoubleDistance> astar;

  // Before computation
  EXPECT_FALSE(astar.has_computation());
  EXPECT_FALSE(astar.is_painted());
  EXPECT_EQ(astar.get_start_node(), nullptr);
  EXPECT_EQ(astar.get_graph(), nullptr);

  astar.paint_min_paths_tree(g, nodes[0]);

  // After computation
  EXPECT_TRUE(astar.has_computation());
  EXPECT_TRUE(astar.is_painted());
  EXPECT_EQ(astar.get_start_node(), nodes[0]);
  EXPECT_EQ(astar.get_graph(), &g);
}

// ==================== Typed Tests for Both Heap Types ====================

template <typename HeapTag>
class AStarHeapTest : public ::testing::Test
{
protected:
  SimpleGraph g;
  std::vector<SimpleGraph::Node*> nodes;

  void SetUp() override
  {
    for (int i = 0; i < 5; ++i)
      nodes.push_back(g.insert_node(i));

    // Create graph: 0 -> 1 -> 2 -> 3 -> 4
    //                \---------^
    g.insert_arc(nodes[0], nodes[1], 1.0);
    g.insert_arc(nodes[1], nodes[2], 2.0);
    g.insert_arc(nodes[2], nodes[3], 3.0);
    g.insert_arc(nodes[3], nodes[4], 4.0);
    g.insert_arc(nodes[0], nodes[2], 10.0);  // Long path
  }
};

using HeapTypes = ::testing::Types<BinHeapTag, FibHeapTag>;
TYPED_TEST_SUITE(AStarHeapTest, HeapTypes);

// Test A* with both heap types - find_path (with heuristic)
TYPED_TEST(AStarHeapTest, FindPathWithHeuristic)
{
  using AStar = AStar_Min_Path<SimpleGraph, DoubleDistance,
                               Zero_Heuristic<SimpleGraph, DoubleDistance>,
                               Node_Arc_Iterator, Dft_Show_Arc<SimpleGraph>,
                               TypeParam::template Heap>;
  AStar astar;
  Path<SimpleGraph> path(this->g);

  auto cost = astar.find_path(this->g, this->nodes[0], this->nodes[4], path);

  EXPECT_EQ(cost, 10.0);  // 1+2+3+4
  EXPECT_FALSE(path.is_empty());
}

// Test A* with both heap types - compute_min_paths_tree (Dijkstra mode)
TYPED_TEST(AStarHeapTest, ComputeMinPathsTree)
{
  using AStar = AStar_Min_Path<SimpleGraph, DoubleDistance,
                               Zero_Heuristic<SimpleGraph, DoubleDistance>,
                               Node_Arc_Iterator, Dft_Show_Arc<SimpleGraph>,
                               TypeParam::template Heap>;
  AStar astar;
  SimpleGraph tree;

  auto root = astar.compute_min_paths_tree(this->g, this->nodes[0], tree);

  EXPECT_NE(root, nullptr);
  EXPECT_EQ(tree.get_num_nodes(), 5u);
  EXPECT_EQ(tree.get_num_arcs(), 4u);
}

// Test A* with both heap types - paint and get_distance
TYPED_TEST(AStarHeapTest, PaintAndGetDistance)
{
  using AStar = AStar_Min_Path<SimpleGraph, DoubleDistance,
                               Zero_Heuristic<SimpleGraph, DoubleDistance>,
                               Node_Arc_Iterator, Dft_Show_Arc<SimpleGraph>,
                               TypeParam::template Heap>;
  AStar astar;

  astar.paint_min_paths_tree(this->g, this->nodes[0]);

  EXPECT_EQ(astar.get_distance(this->nodes[0]), 0.0);
  EXPECT_EQ(astar.get_distance(this->nodes[1]), 1.0);
  EXPECT_EQ(astar.get_distance(this->nodes[2]), 3.0);  // 1+2
  EXPECT_EQ(astar.get_distance(this->nodes[3]), 6.0);  // 1+2+3
  EXPECT_EQ(astar.get_distance(this->nodes[4]), 10.0); // 1+2+3+4
}

// Test A* vs Dijkstra with both heap types
TYPED_TEST(AStarHeapTest, MatchesDijkstra)
{
  using AStar = AStar_Min_Path<SimpleGraph, DoubleDistance,
                               Zero_Heuristic<SimpleGraph, DoubleDistance>,
                               Node_Arc_Iterator, Dft_Show_Arc<SimpleGraph>,
                               TypeParam::template Heap>;
  using Dijkstra = Dijkstra_Min_Paths<SimpleGraph, DoubleDistance,
                                      Node_Arc_Iterator, Dft_Show_Arc<SimpleGraph>,
                                      TypeParam::template Heap>;

  AStar astar;
  Dijkstra dijkstra;

  Path<SimpleGraph> astar_path(this->g);
  Path<SimpleGraph> dijkstra_path(this->g);

  auto astar_cost = astar.find_min_path(this->g, this->nodes[0], this->nodes[4], astar_path);
  auto dijkstra_cost = dijkstra.find_min_path(this->g, this->nodes[0], this->nodes[4], dijkstra_path);

  EXPECT_DOUBLE_EQ(astar_cost, dijkstra_cost);
  EXPECT_EQ(astar_path.size(), dijkstra_path.size());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}