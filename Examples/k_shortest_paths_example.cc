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

# include <iostream>
# include <string>
# include <tuple>
# include <vector>

# include <K_Shortest_Paths.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;
  using Result_Item = K_Shortest_Path_Item<Graph, long long>;

  struct Scenario
  {
    size_t num_nodes = 0;
    vector<tuple<size_t, size_t, long long>> edges;
    size_t source = 0;
    size_t target = 0;
    size_t k = 5;
  };


  Graph build_graph(const Scenario & s, vector<Graph::Node *> & nodes)
  {
    Graph g;
    nodes.reserve(s.num_nodes);
    for (size_t i = 0; i < s.num_nodes; ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : s.edges)
      g.insert_arc(nodes[u], nodes[v], w);

    return g;
  }


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


  template <class Result_List>
  void print_results(const string & title, const Result_List & results)
  {
    cout << title << '\n';
    size_t rank = 1;
    for (typename Result_List::Iterator it(results); it.has_curr(); it.next_ne(), ++rank)
      {
        const Result_Item & item = it.get_curr();
        cout << "  #" << rank
             << "  cost=" << item.total_cost
             << "  path=" << path_to_string(item.path)
             << '\n';
      }
    if (results.is_empty())
      cout << "  (no path found)\n";
    cout << '\n';
  }
} // namespace


int main()
{
  const Scenario scenario{
      .num_nodes = 8,
      .edges = {
          {0, 1, 1}, {1, 2, 1}, {2, 7, 1},
          {0, 3, 2}, {3, 4, 2}, {4, 7, 2},
          {2, 5, 1}, {5, 2, 1}
      },
      .source = 0,
      .target = 7,
      .k = 6
  };

  vector<Graph::Node *> nodes;
  Graph g = build_graph(scenario, nodes);

  auto * source = nodes[scenario.source];
  auto * target = nodes[scenario.target];

  cout << "K shortest paths in Aleph graphs\n";
  cout << "source=" << scenario.source
       << ", target=" << scenario.target
       << ", k=" << scenario.k << "\n\n";

  const auto yen = yen_k_shortest_paths<Graph>(g, source, target, scenario.k);
  const auto epp = eppstein_k_shortest_paths<Graph>(g, source, target, scenario.k);

  print_results("Yen (loopless/simple paths)", yen);
  print_results("Eppstein-style API (general, may include cycles)", epp);

  return 0;
}
