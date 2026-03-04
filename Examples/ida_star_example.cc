/*
                         Aleph_w
 Data structures & Algorithms
 version 2.0.0b
 https://github.com/lrleon/Aleph-w
*/

# include <iostream>
# include <iomanip>
# include <vector>
# include <map>
# include <algorithm>
# include <limits>
# include <aleph.H>
# include <tpl_graph.H>
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
  vector<vector<bool>> obstacles(grid_size, vector<bool>(grid_size, false));

  for (int i = 1; i <= 3; ++i) {
    obstacles[1][i] = true;
    obstacles[3][i] = true;
  }
  obstacles[2][1] = true;
  obstacles[2][3] = true;

  cout << "Grid-based pathfinding (5x5):" << endl;
  cout << "S = Start, G = Goal, # = Obstacle, . = Free" << endl << endl;

  for (int y = 0; y < grid_size; ++y) {
    for (int x = 0; x < grid_size; ++x) {
      if (x == 0 and y == 0)
        cout << "S ";
      else if (x == grid_size - 1 and y == grid_size - 1)
        cout << "G ";
      else if (obstacles[y][x])
        cout << "# ";
      else
        cout << ". ";
    }
    cout << endl;
  }

  cout << endl;

  GT g;
  map<pair<int, int>, GT::Node *> nodes;

  for (int y = 0; y < grid_size; ++y) {
    for (int x = 0; x < grid_size; ++x) {
      if (not obstacles[y][x]) {
        auto node = g.insert_node(Coord{x, y});
        nodes[{x, y}] = node;
      }
    }
  }

  for (int y = 0; y < grid_size; ++y) {
    for (int x = 0; x < grid_size; ++x) {
      if (obstacles[y][x])
        continue;

      int dx[] = {0, 0, 1, -1};
      int dy[] = {1, -1, 0, 0};

      for (int d = 0; d < 4; ++d) {
        int nx = x + dx[d];
        int ny = y + dy[d];

        if (nx >= 0 and nx < grid_size and ny >= 0 and ny < grid_size
            and not obstacles[ny][nx]) {
          auto src = nodes[{x, y}];
          auto tgt = nodes[{nx, ny}];
          g.insert_arc(src, tgt, 1);
        }
      }
    }
  }

  auto start = nodes[{0, 0}];
  auto goal = nodes[{grid_size - 1, grid_size - 1}];

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
