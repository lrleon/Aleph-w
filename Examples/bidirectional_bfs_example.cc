/*
                         Aleph_w
 Data structures & Algorithms
 version 2.0.0b
 https://github.com/lrleon/Aleph-w
*/

# include <iostream>
# include <iomanip>
# include <vector>
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
  g.insert_arc(alice, bob);
  g.insert_arc(bob, alice);
  g.insert_arc(alice, eve);
  g.insert_arc(eve, alice);
  g.insert_arc(bob, charlie);
  g.insert_arc(charlie, bob);
  g.insert_arc(charlie, david);
  g.insert_arc(david, charlie);
  g.insert_arc(charlie, grace);
  g.insert_arc(grace, charlie);
  g.insert_arc(eve, frank);
  g.insert_arc(frank, eve);
  g.insert_arc(frank, grace);
  g.insert_arc(grace, frank);

  cout << "Social network (degrees of separation):" << endl
       << endl
       << "    Alice --- Bob --- Charlie --- David" << endl
       << "      |               |" << endl
       << "     Eve --- Frank - Grace" << endl << endl;

  Bidirectional_BFS<GT> bibfs;

  vector<pair<GT::Node *, GT::Node *>> queries = {
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
