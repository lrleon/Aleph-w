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
 * @file bidirectional_bfs_example.cc
 * @brief Illustrative example for Bidirectional Breadth-First Search (BFS).
 */

# include <iostream>
# include <iomanip>
# include <string>
# include <utility>
# include <aleph.H>
# include <tpl_graph.H>
# include <Bidirectional_BFS.H>

using namespace Aleph;
using namespace std;

using Node_Info = string;
using Arc_Info = int;
using GT = List_Graph<Graph_Node<Node_Info>, Graph_Arc<Arc_Info>>;

int main() {
  cout << "=== Bidirectional BFS Example ===" << endl << endl;

  GT g;

  auto alice = g.insert_node("Alice");
  auto bob = g.insert_node("Bob");
  auto charlie = g.insert_node("Charlie");
  auto david = g.insert_node("David");
  auto eve = g.insert_node("Eve");
  auto frank = g.insert_node("Frank");
  auto grace = g.insert_node("Grace");

  // Undirected: insert both directions
  g.insert_arc(alice, bob, Arc_Info(0));
  g.insert_arc(bob, alice, Arc_Info(0));
  g.insert_arc(alice, eve, Arc_Info(0));
  g.insert_arc(eve, alice, Arc_Info(0));
  g.insert_arc(bob, charlie, Arc_Info(0));
  g.insert_arc(charlie, bob, Arc_Info(0));
  g.insert_arc(charlie, david, Arc_Info(0));
  g.insert_arc(david, charlie, Arc_Info(0));
  g.insert_arc(charlie, grace, Arc_Info(0));
  g.insert_arc(grace, charlie, Arc_Info(0));
  g.insert_arc(eve, frank, Arc_Info(0));
  g.insert_arc(frank, eve, Arc_Info(0));
  g.insert_arc(frank, grace, Arc_Info(0));
  g.insert_arc(grace, frank, Arc_Info(0));

  cout << "Social network (degrees of separation):" << endl
       << endl
       << "    Alice --- Bob --- Charlie --- David" << endl
       << "      |               |" << endl
       << "     Eve --- Frank - Grace" << endl << endl;

  Bidirectional_BFS<GT> bibfs;

  Aleph::Array<std::pair<GT::Node *, GT::Node *>> queries = {
      {alice, david}, {alice, charlie}, {alice, grace}, {bob, eve}};

  cout << "Finding shortest paths:" << endl;

  for (auto [src, tgt] : queries) {
    Path<GT> path(g);
    bool found = bibfs(g, src, tgt, path);

    cout << "  " << left << setw(12) << src->get_info()
         << " to " << left << setw(12) << tgt->get_info()
         << ": ";

    if (found) {
      path.for_each_node([](auto * n) { cout << n->get_info() << " "; });
    } else {
      cout << "No path";
    }

    cout << endl;
  }

  cout << endl << "=== Performance ===" << endl
       << "Time:  O(b^(d/2))" << endl
       << "Space: O(b^(d/2))" << endl
       << "vs Standard BFS: O(b^d) both time and space" << endl;

  return 0;
}
