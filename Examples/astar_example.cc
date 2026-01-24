
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file astar_example.cc
 * @brief A* shortest path in Aleph-w (grid graph, heuristics, and comparison with Dijkstra).
 *
 * ## Overview
 *
 * This example demonstrates Aleph-w's A* implementation (`AStar.H`) on a 2D grid.
 * A* is a best-first shortest-path algorithm that uses a heuristic `h(n)` to guide
 * the search, typically expanding far fewer nodes than Dijkstra when the heuristic
 * is informative.
 *
 * It also compares A* with Dijkstra on the same grid setup.
 *
 * ## Data model used by this example
 *
 * - **Graph type**: `GridGraph = List_Graph<Graph_Node<GridCell>, Graph_Arc<double>>`
 * - **Node info**: `GridCell { x, y, blocked }`
 * - **Arc info**: edge cost (`double`)
 *
 * The helper `create_grid_graph(width, height, diagonal, nodes)` builds a regular
 * grid and optionally adds diagonal connections (8-connected vs 4-connected).
 *
 * ## Usage
 *
 * ```bash
 * ./astar_example
 * ```
 *
 * This example has no command-line options; the demo scenarios are hard-coded.
 *
 * ## Algorithms
 *
 * A* uses:
 *
 * - `g(n)`: cost from start to `n`
 * - `h(n)`: heuristic estimate from `n` to goal
 * - `f(n) = g(n) + h(n)` to prioritize expansions
 *
 * Two heuristics are shown:
 *
 * - **Euclidean** (`sqrt(dx^2 + dy^2)`): admissible when diagonal moves are allowed.
 * - **Manhattan** (`|dx| + |dy|`): admissible for 4-connected grids.
 *
 * A* is guaranteed to find an optimal path when `h` is **admissible** (never
 * overestimates) and typically behaves best when it is also **consistent**.
 *
 * ## Complexity
 *
 * Let **V** be the number of nodes and **E** the number of edges.
 *
 * - Worst-case time is similar to Dijkstra: `O((V + E) log V)` (priority queue).
 * - Auxiliary space is `O(V)`.
 *
 * In practice, a good heuristic can reduce the number of expanded nodes.
 *
 * ## Pitfalls and edge cases
 *
 * - **Heuristic quality**: a weak heuristic makes A* behave like Dijkstra.
 * - **Heuristic admissibility**: if `h` overestimates, A* may return suboptimal paths.
 * - **Movement model mismatch**: use Manhattan for 4-neighbor movement and Euclidean
 *   (or octile) when diagonals are allowed.
 *
 * ## References / see also
 *
 * - `AStar.H` (implementation)
 * - `Dijkstra.H` / `dijkstra_example.cc` (uninformed shortest paths)
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <limits>

#include <tpl_graph.H>
#include <AStar.H>
#include <Dijkstra.H>

using namespace Aleph;
using namespace std;

// =============================================================================
// Graph Node with 2D Coordinates
// =============================================================================

/**
 * @brief Node info containing 2D coordinates.
 *
 * Each node in our grid graph stores its (x, y) position.
 */
struct GridCell
{
  int x = 0;
  int y = 0;
  bool blocked = false;  // Can be used to create obstacles

  GridCell() = default;
  GridCell(int _x, int _y, bool _blocked = false)
    : x(_x), y(_y), blocked(_blocked) {}
};

// Graph types
using GridNode = Graph_Node<GridCell>;
using GridArc = Graph_Arc<double>;
using GridGraph = List_Graph<GridNode, GridArc>;

// =============================================================================
// Distance Accessor
// =============================================================================

/**
 * @brief Distance functor that reads arc weights.
 */
struct GridDistance
{
  using Distance_Type = double;

  Distance_Type operator()(GridGraph::Arc* arc) const
  {
    return arc->get_info();
  }
};

// =============================================================================
// Heuristic Functions
// =============================================================================

/**
 * @brief Euclidean distance heuristic.
 *
 * Computes straight-line distance between two nodes.
 * Admissible for graphs allowing diagonal movement.
 * h(n) = sqrt((x2-x1)² + (y2-y1)²)
 */
struct EuclideanHeuristic
{
  using Distance_Type = double;

  Distance_Type operator()(GridGraph::Node* from, GridGraph::Node* to) const
  {
    const auto& f = from->get_info();
    const auto& t = to->get_info();
    double dx = f.x - t.x;
    double dy = f.y - t.y;
    return sqrt(dx * dx + dy * dy);
  }
};

/**
 * @brief Manhattan distance heuristic.
 *
 * Computes taxi-cab distance (only horizontal/vertical moves).
 * Optimal for 4-connected grids where diagonal movement is not allowed.
 * h(n) = |x2-x1| + |y2-y1|
 */
struct ManhattanHeuristic
{
  using Distance_Type = double;

  Distance_Type operator()(GridGraph::Node* from, GridGraph::Node* to) const
  {
    const auto& f = from->get_info();
    const auto& t = to->get_info();
    return abs(f.x - t.x) + abs(f.y - t.y);
  }
};

// =============================================================================
// Grid Graph Builder
// =============================================================================

/**
 * @brief Creates a 2D grid graph.
 *
 * @param width Grid width (number of columns).
 * @param height Grid height (number of rows).
 * @param diagonal If true, add diagonal connections (8-connected).
 * @param nodes Output vector of node pointers indexed by y*width+x.
 * @return The constructed grid graph.
 */
GridGraph create_grid_graph(int width, int height, bool diagonal,
                            vector<GridGraph::Node*>& nodes)
{
  GridGraph g;
  nodes.resize(width * height);

  // Create nodes
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      {
        int idx = y * width + x;
        nodes[idx] = g.insert_node(GridCell(x, y));
      }

  // Create edges
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      {
        int idx = y * width + x;

        // Right neighbor (weight 1.0)
        if (x + 1 < width)
          {
            int right = y * width + (x + 1);
            g.insert_arc(nodes[idx], nodes[right], 1.0);
            g.insert_arc(nodes[right], nodes[idx], 1.0);
          }

        // Down neighbor (weight 1.0)
        if (y + 1 < height)
          {
            int down = (y + 1) * width + x;
            g.insert_arc(nodes[idx], nodes[down], 1.0);
            g.insert_arc(nodes[down], nodes[idx], 1.0);
          }

        // Diagonal neighbors (weight sqrt(2) ≈ 1.414)
        if (diagonal)
          {
            double diag_weight = sqrt(2.0);

            // Down-right
            if (x + 1 < width && y + 1 < height)
              {
                int dr = (y + 1) * width + (x + 1);
                g.insert_arc(nodes[idx], nodes[dr], diag_weight);
                g.insert_arc(nodes[dr], nodes[idx], diag_weight);
              }

            // Down-left
            if (x - 1 >= 0 && y + 1 < height)
              {
                int dl = (y + 1) * width + (x - 1);
                g.insert_arc(nodes[idx], nodes[dl], diag_weight);
                g.insert_arc(nodes[dl], nodes[idx], diag_weight);
              }
          }
      }

  return g;
}

// =============================================================================
// Path Visualization
// =============================================================================

/**
 * @brief Prints the grid with the path highlighted.
 *
 * Legend:
 *   S = Start
 *   E = End
 *   * = Path
 *   . = Empty cell
 */
void print_grid_with_path(int width, int height,
                          const vector<GridGraph::Node*>& nodes,
                          GridGraph::Node* start,
                          GridGraph::Node* end,
                          const Path<GridGraph>& path)
{
  (void)nodes;

  // Create grid representation
  vector<vector<char>> grid(height, vector<char>(width, '.'));

  // Mark path
  for (Path<GridGraph>::Iterator it(path); it.has_curr(); it.next())
    {
      auto node = it.get_current_node();
      const auto& info = node->get_info();
      grid[info.y][info.x] = '*';
    }

  // Mark start and end
  grid[start->get_info().y][start->get_info().x] = 'S';
  grid[end->get_info().y][end->get_info().x] = 'E';

  // Print grid
  cout << "  ";
  for (int x = 0; x < width; ++x)
    cout << (x % 10);
  cout << "\n";

  for (int y = 0; y < height; ++y)
    {
      cout << setw(2) << y;
      for (int x = 0; x < width; ++x)
        cout << grid[y][x];
      cout << "\n";
    }
}

// =============================================================================
// Benchmark Helper
// =============================================================================

template <typename Func>
double measure_time_ms(Func&& f)
{
  auto start = chrono::high_resolution_clock::now();
  f();
  auto end = chrono::high_resolution_clock::now();
  return chrono::duration<double, milli>(end - start).count();
}

// =============================================================================
// Main Program
// =============================================================================

int main()
{
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║           A* Shortest Path Algorithm - Example                   ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  // =========================================================================
  // Part 1: 4-Connected Grid (Manhattan heuristic optimal)
  // =========================================================================

  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 1: 4-Connected Grid (no diagonal movement)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  const int WIDTH = 15;
  const int HEIGHT = 10;

  vector<GridGraph::Node*> nodes4;
  GridGraph g4 = create_grid_graph(WIDTH, HEIGHT, false, nodes4);

  cout << "Grid size: " << WIDTH << " x " << HEIGHT << "\n";
  cout << "Nodes: " << g4.get_num_nodes() << "\n";
  cout << "Edges: " << g4.get_num_arcs() << "\n\n";

  // Define start and end
  auto start4 = nodes4[0];                              // Top-left (0,0)
  auto end4 = nodes4[(HEIGHT-1) * WIDTH + (WIDTH-1)];   // Bottom-right

  cout << "Start: (" << start4->get_info().x << ", " << start4->get_info().y << ")\n";
  cout << "End:   (" << end4->get_info().x << ", " << end4->get_info().y << ")\n\n";

  // A* with Manhattan heuristic
  cout << "▶ A* with Manhattan heuristic:\n";
  {
    AStar_Min_Path<GridGraph, GridDistance, ManhattanHeuristic> astar;
    Path<GridGraph> path(g4);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_path(g4, start4, end4, path);
    });

    if (cost == std::numeric_limits<double>::max())
      {
        cout << "  No path found.\n";
      }
    else
      {
        cout << "  Path cost: " << cost << "\n";
        cout << "  Path length: " << path.size() << " nodes\n";
      }
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n\n";

    if (cost != std::numeric_limits<double>::max())
      {
        print_grid_with_path(WIDTH, HEIGHT, nodes4, start4, end4, path);
        cout << "\n";
      }
  }

  // A* with Euclidean heuristic
  cout << "▶ A* with Euclidean heuristic:\n";
  {
    AStar_Min_Path<GridGraph, GridDistance, EuclideanHeuristic> astar;
    Path<GridGraph> path(g4);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_path(g4, start4, end4, path);
    });

    if (cost == std::numeric_limits<double>::max())
      {
        cout << "  No path found.\n";
      }
    else
      {
        cout << "  Path cost: " << cost << "\n";
        cout << "  Path length: " << path.size() << " nodes\n";
      }
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n";
    cout << "  Note: Euclidean underestimates in 4-connected grid,\n";
    cout << "        but still finds optimal path (admissible).\n\n";
  }

  // Dijkstra (zero heuristic)
  cout << "▶ Dijkstra (A* with zero heuristic):\n";
  {
    AStar_Min_Path<GridGraph, GridDistance> astar;  // Default: Zero_Heuristic
    Path<GridGraph> path(g4);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_min_path(g4, start4, end4, path);
    });

    if (cost == std::numeric_limits<double>::max())
      {
        cout << "  No path found.\n";
      }
    else
      {
        cout << "  Path cost: " << cost << "\n";
        cout << "  Path length: " << path.size() << " nodes\n";
      }
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n";
    cout << "  Note: Explores more nodes than A* with good heuristic.\n\n";
  }

  // =========================================================================
  // Part 2: 8-Connected Grid (Euclidean heuristic optimal)
  // =========================================================================

  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 2: 8-Connected Grid (with diagonal movement)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  vector<GridGraph::Node*> nodes8;
  GridGraph g8 = create_grid_graph(WIDTH, HEIGHT, true, nodes8);

  cout << "Grid size: " << WIDTH << " x " << HEIGHT << "\n";
  cout << "Nodes: " << g8.get_num_nodes() << "\n";
  cout << "Edges: " << g8.get_num_arcs() << " (includes diagonals)\n\n";

  auto start8 = nodes8[0];
  auto end8 = nodes8[(HEIGHT-1) * WIDTH + (WIDTH-1)];

  // A* with Euclidean heuristic
  cout << "▶ A* with Euclidean heuristic (optimal for 8-connected):\n";
  {
    AStar_Min_Path<GridGraph, GridDistance, EuclideanHeuristic> astar;
    Path<GridGraph> path(g8);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_path(g8, start8, end8, path);
    });

    if (cost == std::numeric_limits<double>::max())
      {
        cout << "  No path found.\n";
      }
    else
      {
        cout << "  Path cost: " << fixed << setprecision(3) << cost << "\n";
        cout << "  Path length: " << path.size() << " nodes\n";
      }
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n\n";

    if (cost != std::numeric_limits<double>::max())
      {
        print_grid_with_path(WIDTH, HEIGHT, nodes8, start8, end8, path);
        cout << "\n";
      }
  }

  // A* with Manhattan heuristic
  cout << "▶ A* with Manhattan heuristic:\n";
  {
    AStar_Min_Path<GridGraph, GridDistance, ManhattanHeuristic> astar;
    Path<GridGraph> path(g8);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_path(g8, start8, end8, path);
    });

    if (cost == std::numeric_limits<double>::max())
      {
        cout << "  No path found.\n";
      }
    else
      {
        cout << "  Path cost: " << fixed << setprecision(3) << cost << "\n";
        cout << "  Path length: " << path.size() << " nodes\n";
      }
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n";
    cout << "  Note: Manhattan overestimates for 8-connected (not admissible),\n";
    cout << "        may not find optimal path!\n\n";
  }

  // =========================================================================
  // Part 3: Computing Full Shortest Paths Tree
  // =========================================================================

  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 3: Computing Full Shortest Paths Tree\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "When you need shortest paths from one source to ALL destinations,\n";
  cout << "use compute_min_paths_tree() or paint_min_paths_tree().\n\n";

  {
    AStar_Min_Path<GridGraph, GridDistance> astar;
    GridGraph tree;

    double time = measure_time_ms([&]() {
      astar.compute_min_paths_tree(g4, start4, tree);
    });

    cout << "▶ compute_min_paths_tree() from (0,0):\n";
    cout << "  Tree nodes: " << tree.get_num_nodes() << "\n";
    cout << "  Tree edges: " << tree.get_num_arcs() << "\n";
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n\n";

    // Query multiple destinations efficiently
    cout << "  Distances from (0,0):\n";

    vector<pair<int,int>> targets = {{5, 5}, {10, 5}, {14, 9}};
    for (const auto& [tx, ty] : targets)
      {
        auto target = nodes4[ty * WIDTH + tx];
        Path<GridGraph> path(g4);
        double dist = astar.get_min_path(tree, target, path);
        cout << "    to (" << tx << ", " << ty << "): " << dist << "\n";
      }
  }

  // =========================================================================
  // Part 4: Using Fibonacci Heap for Large Graphs
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 4: Using Fibonacci Heap (for dense/large graphs)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "For very large or dense graphs, Fibonacci heap can be faster:\n\n";

  // Create larger grid for comparison
  const int BIG_WIDTH = 50;
  const int BIG_HEIGHT = 50;

  vector<GridGraph::Node*> big_nodes;
  GridGraph big_g = create_grid_graph(BIG_WIDTH, BIG_HEIGHT, false, big_nodes);

  auto big_start = big_nodes[0];
  auto big_end = big_nodes[(BIG_HEIGHT-1) * BIG_WIDTH + (BIG_WIDTH-1)];

  cout << "Grid: " << BIG_WIDTH << " x " << BIG_HEIGHT;
  cout << " (" << big_g.get_num_nodes() << " nodes)\n\n";

  // Binary Heap (default)
  {
    using AStar_BinHeap = AStar_Min_Path<GridGraph, GridDistance, ManhattanHeuristic,
                                         Node_Arc_Iterator, Dft_Show_Arc<GridGraph>,
                                         ArcHeap>;
    AStar_BinHeap astar;
    Path<GridGraph> path(big_g);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_path(big_g, big_start, big_end, path);
    });

    cout << "▶ A* with Binary Heap:\n";
    cout << "  Path cost: " << cost << "\n";
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n\n";
  }

  // Fibonacci Heap
  {
    using AStar_FibHeap = AStar_Min_Path<GridGraph, GridDistance, ManhattanHeuristic,
                                         Node_Arc_Iterator, Dft_Show_Arc<GridGraph>,
                                         ArcFibonacciHeap>;
    AStar_FibHeap astar;
    Path<GridGraph> path(big_g);

    double cost = std::numeric_limits<double>::max();
    double time = measure_time_ms([&]() {
      cost = astar.find_path(big_g, big_start, big_end, path);
    });

    cout << "▶ A* with Fibonacci Heap:\n";
    cout << "  Path cost: " << cost << "\n";
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n\n";
  }

  // =========================================================================
  // Part 5: Demonstrating Inadmissible Heuristic (Educational)
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 5: Inadmissible Heuristic Demonstration (Educational)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "WARNING: This example demonstrates what happens when you use an\n";
  cout << "inadmissible heuristic (one that overestimates). This is for\n";
  cout << "educational purposes only. In production, always use admissible\n";
  cout << "heuristics to guarantee optimal paths!\n\n";

  // Create a simple 3-node graph with two paths
  vector<GridGraph::Node*> demo_nodes;
  GridGraph demo_g;

  demo_nodes.push_back(demo_g.insert_node(GridCell(0, 0)));  // Start
  demo_nodes.push_back(demo_g.insert_node(GridCell(5, 0)));  // Middle
  demo_nodes.push_back(demo_g.insert_node(GridCell(10, 0))); // End

  // Two paths:
  // 1. Direct: 0 -> 2 (cost 15.0) - suboptimal
  // 2. Via middle: 0 -> 1 -> 2 (cost 8.0) - optimal
  demo_g.insert_arc(demo_nodes[0], demo_nodes[2], 15.0);  // Direct
  demo_g.insert_arc(demo_nodes[0], demo_nodes[1], 3.0);   // Via middle (better)
  demo_g.insert_arc(demo_nodes[1], demo_nodes[2], 5.0);

  cout << "Graph structure:\n";
  cout << "  Node 0 (0,0) -> Node 2 (10,0): cost 15.0\n";
  cout << "  Node 0 (0,0) -> Node 1 (5,0):  cost  3.0\n";
  cout << "  Node 1 (5,0) -> Node 2 (10,0): cost  5.0\n";
  cout << "  Optimal path: 0 -> 1 -> 2 (total: 8.0)\n\n";

  // Inadmissible heuristic that massively overestimates
  struct BadHeuristic
  {
    using Distance_Type = double;
    Distance_Type operator()(GridGraph::Node* from, GridGraph::Node* to) const
    {
      const auto& f = from->get_info();
      const auto& t = to->get_info();
      double dx = f.x - t.x;
      double dy = f.y - t.y;
      // Overestimate by 10x
      return 10.0 * sqrt(dx * dx + dy * dy);
    }
  };

  // Test with admissible heuristic first (Euclidean)
  cout << "▶ With Euclidean heuristic (admissible):\n";
  {
    AStar_Min_Path<GridGraph, GridDistance, EuclideanHeuristic> astar;
    Path<GridGraph> path(demo_g);

    double cost = astar.find_path(demo_g, demo_nodes[0], demo_nodes[2], path);

    cout << "  Path cost: " << cost << "\n";
    cout << "  Path length: " << path.size() << " nodes\n";
    cout << "  Result: ";
    if (cost == 8.0)
      cout << "✓ Found optimal path (0 -> 1 -> 2)\n\n";
    else
      cout << "✗ Found suboptimal path\n\n";
  }

  // Test with inadmissible heuristic
  cout << "▶ With inadmissible heuristic (10x overestimate):\n";
  {
    AStar_Min_Path<GridGraph, GridDistance, BadHeuristic> astar;
    Path<GridGraph> path(demo_g);

    double cost = astar.find_path(demo_g, demo_nodes[0], demo_nodes[2], path);

    cout << "  Path cost: " << cost << "\n";
    cout << "  Path length: " << path.size() << " nodes\n";
    cout << "  Result: ";
    if (cost == 8.0)
      cout << "Found optimal path (by chance)\n";
    else if (cost == 15.0)
      cout << "✗ Found suboptimal direct path (0 -> 2)\n";
    else
      cout << "Found path with cost " << cost << "\n";

    cout << "\n  Explanation: The inadmissible heuristic made node 1 look\n";
    cout << "  too expensive (h(1) >> actual cost), so A* chose the direct\n";
    cout << "  path instead of exploring through node 1.\n\n";
  }

  // Compare with Dijkstra (always optimal)
  cout << "▶ With Dijkstra (zero heuristic, always optimal):\n";
  {
    AStar_Min_Path<GridGraph, GridDistance> astar;
    Path<GridGraph> path(demo_g);

    double cost = astar.find_min_path(demo_g, demo_nodes[0], demo_nodes[2], path);

    cout << "  Path cost: " << cost << "\n";
    cout << "  Path length: " << path.size() << " nodes\n";
    cout << "  Result: ✓ Always finds optimal path\n\n";
  }

  cout << "┌─────────────────────────────────────────────────────────────────┐\n";
  cout << "│ Key Takeaway:                                                   │\n";
  cout << "│                                                                 │\n";
  cout << "│ An inadmissible heuristic CAN make A* return suboptimal paths! │\n";
  cout << "│                                                                 │\n";
  cout << "│ Always verify your heuristic never overestimates:              │\n";
  cout << "│   h(n) ≤ actual_cost(n, goal)  for all nodes n                 │\n";
  cout << "│                                                                 │\n";
  cout << "│ When in doubt, use zero heuristic (Dijkstra) for correctness.  │\n";
  cout << "└─────────────────────────────────────────────────────────────────┘\n\n";

  // =========================================================================
  // Summary
  // =========================================================================

  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Summary\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "┌─────────────────────────────────────────────────────────────────┐\n";
  cout << "│ Heuristic Choice:                                               │\n";
  cout << "│   • 4-connected grid → Manhattan heuristic (optimal)           │\n";
  cout << "│   • 8-connected grid → Euclidean heuristic (optimal)           │\n";
  cout << "│   • Unknown graph    → Zero heuristic (Dijkstra, always works) │\n";
  cout << "├─────────────────────────────────────────────────────────────────┤\n";
  cout << "│ Key Methods:                                                    │\n";
  cout << "│   • find_path()      - Find single shortest path (recommended) │\n";
  cout << "│   • find_min_path()  - Same but without heuristic (Dijkstra)   │\n";
  cout << "│   • paint_min_paths_tree() - All paths from source (paint)     │\n";
  cout << "│   • compute_min_paths_tree() - All paths (build tree)          │\n";
  cout << "├─────────────────────────────────────────────────────────────────┤\n";
  cout << "│ Heap Selection:                                                 │\n";
  cout << "│   • ArcHeap (Binary) - Good for most cases                     │\n";
  cout << "│   • ArcFibonacciHeap - Better for very large/dense graphs      │\n";
  cout << "└─────────────────────────────────────────────────────────────────┘\n\n";

  return 0;
}
