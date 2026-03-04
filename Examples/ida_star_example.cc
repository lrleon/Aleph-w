/*
                         Aleph_w
 Data structures & Algorithms
 version 2.0.0b
 https://github.com/lrleon/Aleph-w
*/

/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file ida_star_example.cc
 * @brief Illustrative example for Iterative Deepening A* (IDA*) algorithm.
 */

# include <iostream>
# include <iomanip>
# include <algorithm>
# include <limits>
# include <aleph.H>
# include <tpl_graph.H>
# include <tpl_dynMapTree.H>
# include <IDA_Star.H>

using namespace Aleph;
using namespace std;

struct Coord {
  int x, y;
  bool operator==(const Coord & other) const {
    return x == other.x and y == other.y;
  }
};

using Node_Info = Coord;
using Arc_Info = int;
using GT = List_Graph<Graph_Node<Node_Info>, Graph_Arc<Arc_Info>>;

template <class GT>
struct ManhattanH {
  using Distance_Type = int;
  int operator()(typename GT::Node * from, typename GT::Node * to) const {
    auto & f = from->get_info();
    auto & t = to->get_info();
    return abs(f.x - t.x) + abs(f.y - t.y);
  }
};

int main() {
  cout << "=== IDA* (Iterative Deepening A*) Example ===" << endl << endl;

  int grid_size = 5;
  
  // Use DynList<DynList<bool>> for obstacles as requested
  DynList<DynList<bool>> obstacles;
  for (int i = 0; i < grid_size; ++i)
    {
      DynList<bool> row;
      for (int j = 0; j < grid_size; ++j)
        row.append(false);
      obstacles.append(row);
    }

  auto set_obstacle = [&](int y, int x, bool val) {
    auto & row = obstacles.nth(y);
    row.nth(x) = val;
  };

  auto get_obstacle = [&](int y, int x) -> bool {
    return obstacles.nth(y).nth(x);
  };

  for (int i = 1; i <= 3; ++i) {
    set_obstacle(1, i, true);
    set_obstacle(3, i, true);
  }
  set_obstacle(2, 1, true);
  set_obstacle(2, 3, true);

  cout << "Grid-based pathfinding (5x5):" << endl;
  cout << "S = Start, G = Goal, # = Obstacle, . = Free" << endl << endl;

  for (int y = 0; y < grid_size; ++y) {
    for (int x = 0; x < grid_size; ++x) {
      if (x == 0 and y == 0)
        cout << "S ";
      else if (x == grid_size - 1 and y == grid_size - 1)
        cout << "G ";
      else if (get_obstacle(y, x))
        cout << "# ";
      else
        cout << ". ";
    }
    cout << endl;
  }

  cout << endl;

  GT g;
  DynMapTree<pair<int, int>, GT::Node *> nodes;

  for (int y = 0; y < grid_size; ++y) {
    for (int x = 0; x < grid_size; ++x) {
      if (not get_obstacle(y, x)) {
        auto node = g.insert_node(Coord{x, y});
        nodes.insert(make_pair(x, y), node);
      }
    }
  }

  for (int y = 0; y < grid_size; ++y) {
    for (int x = 0; x < grid_size; ++x) {
      if (get_obstacle(y, x))
        continue;

      int dx[] = {0, 0, 1, -1};
      int dy[] = {1, -1, 0, 0};

      for (int d = 0; d < 4; ++d) {
        int nx = x + dx[d];
        int ny = y + dy[d];

        if (nx >= 0 and nx < grid_size and ny >= 0 and ny < grid_size
            and not get_obstacle(ny, nx)) {
          auto src = nodes.find(make_pair(x, y));
          auto tgt = nodes.find(make_pair(nx, ny));
          g.insert_arc(src, tgt, 1);
        }
      }
    }
  }

  auto start = nodes.find(make_pair(0, 0));
  auto goal = nodes.find(make_pair(grid_size - 1, grid_size - 1));

  cout << "Running IDA* with Manhattan distance heuristic:" << endl;

  IDA_Star<GT, Dft_Dist<GT>, ManhattanH<GT>> ida;
  Path<GT> path(g);

  auto cost = ida(g, start, goal, path);

  if (cost == numeric_limits<int>::max()) {
    cout << "No path found!" << endl;
  } else {
    cout << "Path found! Cost: " << cost << endl << endl;

    cout << "Path coordinates: ";
    int count = 0;
    path.for_each_node([&count](auto * n) {
      auto & c = n->get_info();
      cout << "(" << c.x << "," << c.y << ") ";
      count++;
    });
    cout << endl << "Path length: " << (count - 1) << " moves" << endl;
  }

  cout << endl << "=== Performance ===" << endl
       << "Time:  O(b^d)" << endl
       << "Space: O(d) [linear in depth]" << endl;

  return 0;
}
