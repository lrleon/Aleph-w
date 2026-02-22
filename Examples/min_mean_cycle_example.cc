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
 * @file min_mean_cycle_example.cc
 * @brief Example: Karp minimum mean cycle on Aleph digraphs.
 *
 * Build and run:
 *
 *   cmake -S . -B build
 *   cmake --build build --target min_mean_cycle_example
 *   ./build/Examples/min_mean_cycle_example
 */

# include <iomanip>
# include <iostream>
# include <string>

# include <Min_Mean_Cycle.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace
{
  using Graph = List_Digraph<Graph_Node<string>, Graph_Arc<double>>;
  using Arc = Graph::Arc;

  struct Hide_Arc
  {
    Arc * blocked = nullptr;

    bool operator()(Arc * arc) const noexcept
    {
      return arc != blocked;
    }
  };


  void print_result(const Graph & g,
                    const string & title,
                    const Min_Mean_Cycle_Result<Graph, double> & r)
  {
    cout << title << '\n';

    if (not r.has_cycle)
      {
        cout << "  no directed cycle\n\n";
        return;
      }

    cout << fixed << setprecision(6);
    cout << "  minimum mean = " << r.minimum_mean << '\n';

    if (r.cycle_length == 0)
      {
        cout << "  witness cycle unavailable\n\n";
        return;
      }

    // Recomputed from witness cycle cost/length as a sanity check against
    // r.minimum_mean (they should agree up to numerical precision).
    const long double witness_mean = static_cast<long double>(r.cycle_total_cost)
                                     / static_cast<long double>(r.cycle_length);

    cout << "  witness cycle: cost=" << r.cycle_total_cost
         << ", length=" << r.cycle_length
         << ", mean=" << witness_mean << '\n';

    cout << "  nodes: ";
    bool first = true;
    for (auto it = r.cycle_nodes.get_it(); it.has_curr(); it.next_ne())
      {
        if (not first)
          cout << " -> ";
        first = false;
        cout << it.get_curr()->get_info();
      }
    cout << '\n';

    cout << "  arcs: ";
    first = true;
    for (auto it = r.cycle_arcs.get_it(); it.has_curr(); it.next_ne())
      {
        Arc * arc = it.get_curr();
        if (not first)
          cout << ", ";
        first = false;

        cout << g.get_src_node(arc)->get_info()
             << "->"
             << g.get_tgt_node(arc)->get_info()
             << "(" << arc->get_info() << ")";
      }
    cout << "\n\n";
  }
} // namespace


int main()
{
  Graph g;

  auto * A = g.insert_node("A");
  auto * B = g.insert_node("B");
  auto * C = g.insert_node("C");
  auto * D = g.insert_node("D");
  auto * E = g.insert_node("E");
  auto * F = g.insert_node("F");

  // Cycle A->B->C->A has mean 2.0
  g.insert_arc(A, B, 4.0);
  g.insert_arc(B, C, 1.0);
  g.insert_arc(C, A, 1.0);

  // Cycle B->D->E->B has mean 1.0 (optimal in full graph)
  g.insert_arc(B, D, 1.0);
  Arc * blocked = g.insert_arc(D, E, 1.0);
  g.insert_arc(E, B, 1.0);

  // Extra expensive cycle
  g.insert_arc(C, F, 6.0);
  g.insert_arc(F, C, 6.0);

  cout << "Minimum mean cycle with Karp (Aleph graphs)\n";
  cout << "---------------------------------------------\n\n";

  const auto full = karp_minimum_mean_cycle(g);
  print_result(g, "Full graph", full);

  const auto value_only = karp_minimum_mean_cycle_value(g);
  cout << "Value-only API (full graph): ";
  if (value_only.has_cycle)
    cout << "minimum mean = " << value_only.minimum_mean << "\n\n";
  else
    cout << "no directed cycle\n\n";

  const auto filtered =
      karp_minimum_mean_cycle<Graph, Dft_Dist<Graph>, Hide_Arc>(
          g, Dft_Dist<Graph>(), Hide_Arc{blocked});
  print_result(g, "Filtered graph (blocking D->E)", filtered);

  return 0;
}
