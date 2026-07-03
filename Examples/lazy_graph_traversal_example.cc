
/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file lazy_graph_traversal_example.cc
 * @brief Lazy graph BFS/DFS with coroutines (graph-traverse-generators.H).
 *
 * ## Overview
 *
 * `Graph_Traverse` (`graph-traverse.H`) already supports early termination:
 * its visitor callback returns `bool`, and `false` stops the walk. What
 * `Graph_Traverse_BFS_Generator`/`Graph_Traverse_DFS_Generator`
 * (`graph-traverse-generators.H`) add is **ergonomics and composability**:
 * the exact same algorithm, driven by a plain range-`for` with `break`
 * instead of a callback that has to remember to `return false`, and
 * naturally chainable with other lazy code.
 *
 * This example runs the same "find a node, stop as soon as you do" search
 * both ways on a small social-network graph, to make the code-shape
 * difference concrete.
 *
 * @see bfs_dfs_example.C The eager callback-based version of this walk.
 * @author Leandro Rabindranath Leon
 * @ingroup Examples
 */

#include <iostream>
#include <string>

#include <graph-traverse-generators.H>
#include <graph-traverse.H>
#include <tpl_graph.H>

using namespace Aleph;
using namespace std;

namespace
{
using Node = Graph_Node<string>;
using Arc = Graph_Arc<int>;
using Graph = List_Graph<Node, Arc>;
using Itor = Node_Arc_Iterator<Graph>;

void rule()
{
  cout << "------------------------------------------------------------\n";
}

// Small social network: Alice is friends with Bob and Charlie; Bob and
// Charlie know each other and are both friends with the Diana/Eve pair;
// Diana is also friends with Frank, who is friends with Grace.
Graph build_social_network(Graph::Node *&alice, Graph::Node *&eve)
{
  Graph g;

  alice = g.insert_node("Alice");
  auto *bob = g.insert_node("Bob");
  auto *charlie = g.insert_node("Charlie");
  auto *diana = g.insert_node("Diana");
  eve = g.insert_node("Eve");
  auto *frank = g.insert_node("Frank");
  auto *grace = g.insert_node("Grace");

  g.insert_arc(alice, bob);
  g.insert_arc(alice, charlie);
  g.insert_arc(bob, charlie);
  g.insert_arc(bob, diana);
  g.insert_arc(charlie, eve);
  g.insert_arc(diana, eve);
  g.insert_arc(diana, frank);
  g.insert_arc(frank, grace);

  return g;
}

void demo_bfs_dfs_order(Graph &g, Graph::Node *alice)
{
  cout << "[1] BFS / DFS, driven lazily from Alice\n";
  rule();

  cout << "BFS order: ";
  Graph_Traverse_BFS_Generator<Graph, Itor> bfs(g);
  for (Graph::Node *n : bfs.traverse(alice))
    cout << n->get_info() << " ";
  cout << "\n";

  cout << "DFS order: ";
  Graph_Traverse_DFS_Generator<Graph, Itor> dfs(g);
  for (Graph::Node *n : dfs.traverse(alice))
    cout << n->get_info() << " ";
  cout << "\n\n";
}

void demo_early_termination_ergonomics(Graph &g, Graph::Node *alice)
{
  cout << "[2] Early termination: callback-with-bool vs plain break\n";
  rule();

  const string target = "Eve";

  // --- Eager: the visitor must remember to `return false` to stop, and
  // the traversal machinery has to be threaded through as a callback.
  int eager_visits = 0;
  Graph::Node *eager_found = nullptr;
  Graph_Traverse<Graph, Itor, DynListQueue> eager_bfs(g);
  eager_bfs(alice, [&](Graph::Node *n)
  {
    ++eager_visits;
    if (n->get_info() == target)
      {
        eager_found = n;
        return false;   // easy to forget this line and scan the whole graph
      }
    return true;
  });
  cout << "Eager Graph_Traverse (callback) searching for \"" << target << "\":\n";
  cout << "  found=" << (eager_found != nullptr)
       << ", nodes visited=" << eager_visits << "\n";

  // --- Lazy: the search reads as an ordinary loop; `break` is all it takes.
  int lazy_visits = 0;
  Graph::Node *lazy_found = nullptr;
  Graph_Traverse_BFS_Generator<Graph, Itor> lazy_bfs(g);
  for (Graph::Node *n : lazy_bfs.traverse(alice))
    {
      ++lazy_visits;
      if (n->get_info() == target)
        {
          lazy_found = n;
          break;
        }
    }
  cout << "Lazy Graph_Traverse_BFS_Generator (range-for) searching for \""
       << target << "\":\n";
  cout << "  found=" << (lazy_found != nullptr)
       << ", nodes visited=" << lazy_visits << "\n\n";

  cout << "Same node count either way — both approaches support early\n"
          "termination. The lazy version reads as plain iteration-with-break\n"
          "instead of a callback contract you have to get right, and it\n"
          "composes with other range-for/`Generator` code for free.\n\n";
}
} // namespace

int main()
{
  cout << boolalpha;
  cout << "\n=== Lazy graph BFS/DFS ===\n\n";

  Graph::Node *alice = nullptr;
  Graph::Node *eve = nullptr;
  Graph g = build_social_network(alice, eve);

  demo_bfs_dfs_order(g, alice);
  demo_early_termination_ergonomics(g, alice);

  cout << "Done.\n";
  return 0;
}