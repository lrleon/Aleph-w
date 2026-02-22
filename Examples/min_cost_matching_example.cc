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
 * @file min_cost_matching_example.cc
 * @brief Dedicated minimum-cost matching in general graphs.
 *
 * Demonstrates:
 * - Dedicated API: blossom_minimum_cost_matching()
 * - Dedicated API: blossom_minimum_cost_perfect_matching()
 * - Objective modes:
 *   - pure minimum-cost
 *   - maximum-cardinality then minimum-cost
 * - Perfect matching feasibility reporting
 * - Same result across List/SGraph/Array Aleph graph backends
 *
 * Build and run:
 *
 *   cmake -S . -B build
 *   cmake --build build --target min_cost_matching_example
 *   ./build/Examples/min_cost_matching_example
 */

# include <algorithm>
# include <iostream>
# include <string>
# include <tuple>
# include <utility>
# include <vector>

# include <Min_Cost_Matching.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>

using namespace std;
using namespace Aleph;

namespace
{
  using Cost_Edge = tuple<size_t, size_t, long long>;

  struct Scenario
  {
    string title;
    size_t num_nodes = 0;
    vector<Cost_Edge> edges;
  };


  template <class GT>
  GT build_graph(const Scenario & s)
  {
    GT g;
    vector<typename GT::Node *> nodes;
    nodes.reserve(s.num_nodes);

    for (size_t i = 0; i < s.num_nodes; ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, cost] : s.edges)
      g.insert_arc(nodes[u], nodes[v], cost);

    return g;
  }


  template <class GT>
  vector<pair<size_t, size_t>>
  extract_pairs(const GT & g, const DynDlist<typename GT::Arc *> & matching)
  {
    vector<pair<size_t, size_t>> pairs;
    pairs.reserve(matching.size());

    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        size_t u = static_cast<size_t>(g.get_src_node(arc)->get_info());
        size_t v = static_cast<size_t>(g.get_tgt_node(arc)->get_info());
        if (u > v)
          swap(u, v);
        pairs.emplace_back(u, v);
      }

    sort(pairs.begin(), pairs.end());
    return pairs;
  }


  string format_pairs(const vector<pair<size_t, size_t>> & pairs)
  {
    if (pairs.empty())
      return "(empty)";

    string out;
    for (size_t i = 0; i < pairs.size(); ++i)
      {
        out += "(" + to_string(pairs[i].first) + "," + to_string(pairs[i].second) + ")";
        if (i + 1 < pairs.size())
          out += " ";
      }

    return out;
  }


  template <class GT>
  void print_backend(const string & backend_name,
                     const Scenario & s,
                     bool max_cardinality,
                     long long & canonical_cost,
                     size_t & canonical_cardinality,
                     bool & first_backend)
  {
    GT g = build_graph<GT>(s);
    DynDlist<typename GT::Arc *> matching;

    const auto result = blossom_minimum_cost_matching<GT>(
        g,
        matching,
        Dft_Dist<GT>(),
        Dft_Show_Arc<GT>(),
        max_cardinality);

    const auto pairs = extract_pairs(g, matching);

    if (first_backend)
      {
        canonical_cost = result.total_cost;
        canonical_cardinality = result.cardinality;
        first_backend = false;
      }

    cout << "  " << backend_name
         << " -> card " << result.cardinality
         << ", cost " << result.total_cost
         << " | " << format_pairs(pairs) << '\n';

    if (result.total_cost != canonical_cost
        or result.cardinality != canonical_cardinality)
      cerr << "  Warning: objective mismatch across backends\n";
  }


  void run_scenario(const Scenario & s)
  {
    cout << '\n' << s.title << '\n';
    cout << string(s.title.size(), '-') << '\n';

    for (bool max_cardinality : {false, true})
      {
        cout << (max_cardinality
                 ? "\nMode: maximum-cardinality then minimum-cost\n"
                 : "\nMode: pure minimum-cost\n");

        long long canonical_cost = 0;
        size_t canonical_cardinality = 0;
        bool first_backend = true;

        print_backend<List_Graph<Graph_Node<int>, Graph_Arc<long long>>>(
            "List_Graph", s, max_cardinality, canonical_cost, canonical_cardinality, first_backend);
        print_backend<List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>>(
            "List_SGraph", s, max_cardinality, canonical_cost, canonical_cardinality, first_backend);
        print_backend<Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>>(
            "Array_Graph", s, max_cardinality, canonical_cost, canonical_cardinality, first_backend);
      }
  }


  template <class GT>
  void print_backend_perfect(const string & backend_name,
                             const Scenario & s)
  {
    GT g = build_graph<GT>(s);
    DynDlist<typename GT::Arc *> matching;
    const auto result = blossom_minimum_cost_perfect_matching<GT>(g, matching);

    cout << "  " << backend_name << " -> feasible "
         << (result.feasible ? "yes" : "no");
    if (result.feasible)
      {
        const auto pairs = extract_pairs(g, matching);
        cout << ", card " << result.cardinality
             << ", cost " << result.total_cost
             << " | " << format_pairs(pairs);
      }
    cout << '\n';
  }


  void run_perfect_demo(const Scenario & s)
  {
    cout << '\n' << s.title << " (perfect matching)\n";
    cout << string(s.title.size() + 19, '-') << '\n';

    print_backend_perfect<List_Graph<Graph_Node<int>, Graph_Arc<long long>>>(
        "List_Graph", s);
    print_backend_perfect<List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>>(
        "List_SGraph", s);
    print_backend_perfect<Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>>(
        "Array_Graph", s);
  }
} // namespace


int main()
{
  const Scenario scenario{
      .title = "General min-cost matching (objective-mode demo)",
      .num_nodes = 6,
      .edges = {
          {0, 1, 8},
          {0, 2, -5},
          {1, 2, 4},
          {1, 3, 6},
          {2, 4, 3},
          {3, 4, 2},
          {3, 5, 7},
          {4, 5, 9},
          {0, 5, 5},
          {2, 5, 1}
      }
  };

  const Scenario perfect_feasible{
      .title = "Perfect min-cost matching demo (feasible)",
      .num_nodes = 4,
      .edges = {
          {0, 1, 5},
          {0, 2, 2},
          {1, 3, 1},
          {2, 3, 7}
      }
  };

  const Scenario perfect_infeasible{
      .title = "Perfect min-cost matching demo (infeasible)",
      .num_nodes = 5,
      .edges = {
          {0, 1, 4},
          {1, 2, 3},
          {2, 3, 2}
      }
  };

  cout << "Dedicated API: blossom_minimum_cost_matching()\n";
  cout << "Dedicated API: blossom_minimum_cost_perfect_matching()\n";
  cout << "All costs are edge costs in a non-bipartite graph.\n";
  run_scenario(scenario);
  run_perfect_demo(perfect_feasible);
  run_perfect_demo(perfect_infeasible);

  return 0;
}
