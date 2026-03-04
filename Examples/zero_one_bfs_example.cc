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

# include <iostream>
# include <iomanip>
# include <aleph.H>
# include <tpl_graph.H>
# include <Zero_One_BFS.H>

using namespace Aleph;
using namespace std;

using Node_Info = int;
using Arc_Info = int;
using GT = List_Graph<Graph_Node<Node_Info>, Graph_Arc<Arc_Info>>;

int main()
{
  cout << "=== Zero_One_BFS Algorithm Example ===" << endl << endl;

  GT g;

  auto a = g.insert_node('A');
  auto b = g.insert_node('B');
  auto c = g.insert_node('C');
  auto d = g.insert_node('D');
  auto e = g.insert_node('E');

  g.insert_arc(a, b, 0);  // A to B: free
  g.insert_arc(a, c, 0);  // A to C: free
  g.insert_arc(c, e, 0);  // C to E: free
  g.insert_arc(b, d, 1);  // B to D: costs 1
  g.insert_arc(b, e, 1);  // B to E: costs 1

  cout << "Graph with 0/1 weights:" << endl
       << "  A ---(0)---> B" << endl
       << "  |            |" << endl
       << " (0)          (1)" << endl
       << "  |            |" << endl
       << "  V            V" << endl
       << "  C ---(0)---> E" << endl << endl;

  Zero_One_BFS<GT> bfs01;

  bfs01.paint_min_paths_tree(g, a);

  cout << "Shortest distances from A:" << endl;
  cout << fixed << setw(8) << "Node" << setw(10) << "Distance" << endl;
  cout << setfill('-') << setw(18) << "-" << setfill(' ') << endl;

  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    {
      auto node = it.get_curr();
      auto dist = bfs01.get_distance(node);
      cout << setw(8) << (char)node->get_info()
           << setw(10) << dist << endl;
    }

  cout << endl << "=== Performance ===" << endl
       << "Time complexity:  O(V + E)" << endl
       << "Space complexity: O(V)" << endl;

  return 0;
}
