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
 * @file k_shortest_paths_example.cc
 * @brief K shortest paths: loopless Yen vs general Eppstein-style API.
 *
 * Build and run:
 *
 *   cmake -S . -B build
 *   cmake --build build --target k_shortest_paths_example
 *   ./build/Examples/k_shortest_paths_example
 */

# include <format>
# include <iostream>
# include <string>

# include <K_Shortest_Paths.H>
# include <tpl_array.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;
  using Result_Item = K_Shortest_Path_Item<Graph, long long>;

  struct Edge
  {
    size_t u = 0;
    size_t v = 0;
    long long w = 0;
  };

  struct Scenario
  {
    size_t num_nodes = 0;
    Array<Edge> edges;
    size_t source = 0;
    size_t target = 0;
    size_t k = 5;
  };


  /**
   * @brief Construct a directed graph from a Scenario and populate an external node pointer array.
   *
   * Builds a Graph with s.num_nodes nodes and inserts directed arcs for every Edge in s.edges.
   *
   * @param s Scenario describing number of nodes, edge list, source/target, and k.
   * @param nodes Receives pointers to the newly inserted Graph nodes in ascending index order (size reserved to s.num_nodes).
   * @return Graph The constructed directed graph containing the nodes and arcs specified by the scenario.
   */
  Graph build_graph(const Scenario & s, Array<Graph::Node *> & nodes)
  {
    Graph g;
    nodes.reserve(s.num_nodes);
    for (size_t i = 0; i < s.num_nodes; ++i)
      nodes.append(g.insert_node(static_cast<int>(i)));

    for (typename Array<Edge>::Iterator it(s.edges); it.has_curr(); it.next_ne())
      {
        const Edge & e = it.get_curr();
        g.insert_arc(nodes[e.u], nodes[e.v], e.w);
      }

    return g;
  }


  /**
   * @brief Convert a graph path into a human-readable node sequence.
   *
   * Produces a string listing node identifiers in path order separated by " -> ".
   *
   * @param path Path to convert.
   * @return std::string The formatted node sequence, e.g. "0 -> 1 -> 3".
   */
  string path_to_string(const Path<Graph> & path)
  {
    string out;
    bool first = true;
    for (Path<Graph>::Iterator it(path); it.has_current_node(); it.next_ne())
      {
        if (not first)
          out += " -> ";
        first = false;
        out += to_string(it.get_current_node_ne()->get_info());
      }
    return out;
  }


  /** @brief Helper to print k-shortest path results. */
  /**
   * @brief Print a titled list of k-shortest-path results to standard output.
   *
   * Prints each result on its own line with a 1-based rank, total cost, and
   * path (node sequence). If the results list is empty, prints "  (no path found)".
   *
   * @param title Header text printed before the results.
   * @param results List of k-shortest-path result items to display; each item's
   *                `total_cost` and `path` are shown.
   */
  void print_results(const string & title, const DynList<Result_Item> & results)
  {
    cout << title << '\n';
    size_t rank = 1;
    for (DynList<Result_Item>::Iterator it(results); it.has_curr(); it.next_ne(), ++rank)
      {
        const auto & item = it.get_curr();
        cout << std::format("  #{}  cost={}  path={}\n",
                            rank, item.total_cost, path_to_string(item.path));
      }
    if (results.is_empty())
      cout << "  (no path found)\n";
    cout << '\n';
  }
} /**
 * @brief Runs a sample demonstration computing K shortest paths using two APIs and prints their results.
 *
 * Constructs a sample scenario, builds a directed graph, computes up to K shortest paths using Yen's
 * loopless algorithm and an Eppstein-style API, and prints each algorithm's results to standard output.
 *
 * @return int Exit code (0 on success).
 */


int main()
{
  Scenario scenario;
  scenario.num_nodes = 8;
  scenario.source = 0;
  scenario.target = 7;
  scenario.k = 6;
  scenario.edges.reserve(8);
  scenario.edges.append(Edge{0, 1, 1});
  scenario.edges.append(Edge{1, 2, 1});
  scenario.edges.append(Edge{2, 7, 1});
  scenario.edges.append(Edge{0, 3, 2});
  scenario.edges.append(Edge{3, 4, 2});
  scenario.edges.append(Edge{4, 7, 2});
  scenario.edges.append(Edge{2, 5, 1});
  scenario.edges.append(Edge{5, 2, 1});

  Array<Graph::Node *> nodes;
  Graph g = build_graph(scenario, nodes);

  auto * source = nodes[scenario.source];
  auto * target = nodes[scenario.target];

  cout << std::format("K shortest paths in Aleph graphs\n");
  cout << std::format("source={}, target={}, k={}\n\n",
                      scenario.source, scenario.target, scenario.k);

  const auto yen = yen_k_shortest_paths<Graph>(g, source, target, scenario.k);
  const auto epp = eppstein_k_shortest_paths<Graph>(g, source, target, scenario.k);

  print_results("Yen (loopless/simple paths)", yen);
  print_results("Eppstein-style API (general, may include cycles)", epp);

  return 0;
}