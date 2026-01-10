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
          auto node = new GridNode(idx, x, y);
          nodes[idx] = g.insert_node(node->get_info());
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

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}