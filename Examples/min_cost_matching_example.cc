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
  /**
   * @brief Constructs a graph of type `GT` representing the given scenario.
   *
   * Creates `s.num_nodes` nodes whose stored node info is the node index (0..s.num_nodes-1)
   * and inserts an arc for each entry in `s.edges` connecting the corresponding node indices
   * with the provided cost.
   *
   * @tparam GT Graph type to construct.
   * @param s Scenario describing node count and edges (tuples of u, v, cost).
   * @return GT A graph instance populated with the scenario's nodes and arcs.
   */
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
  /**
   * @brief Convert a matching (list of arc pointers) into a sorted list of node-index pairs.
   *
   * For each arc in |matching| obtains the source and target node info from |g|,
   * normalizes each pair so the smaller index comes first, and returns all pairs
   * sorted in ascending lexicographic order.
   *
   * @param g Graph instance whose node info fields contain the node indices.
   * @param matching Dynamic list of arc pointers representing the matching.
   * @return std::vector<std::pair<size_t, size_t>> Sorted list of matched node pairs with each pair formatted as (u, v) where u <= v.
   */
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


  /**
   * @brief Formats a list of node-pair tuples into a human-readable string.
   *
   * Produces "(empty)" when the input list contains no pairs. Otherwise returns
   * the pairs as a space-separated sequence like "(u,v) (x,y) ..." where each
   * pair preserves the order of elements provided.
   *
   * @param pairs Sorted vector of node-index pairs to format.
   * @return std::string "(empty)" for an empty list, or a space-separated list of pairs formatted as "(u,v)".
   */
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
  /**
   * @brief Run the minimum-cost matching demo for a specific graph backend and print results.
   *
   * Builds a graph from the given scenario, computes a minimum-cost matching using the
   * specified objective mode, prints the backend's matching cardinality, total cost,
   * and matched pairs to stdout, and checks consistency with a canonical result.
   *
   * @param backend_name Human-readable name for the graph backend (printed with results).
   * @param s Scenario describing nodes and weighted edges used to build the graph.
   * @param max_cardinality If true, prefer maximum-cardinality matchings before minimizing cost;
   *        if false, minimize cost without prioritizing cardinality.
   * @param canonical_cost On first backend call (when `first_backend` is true), set to this
   *        backend's total cost; on subsequent calls, used as the canonical cost to compare against.
   * @param canonical_cardinality On first backend call (when `first_backend` is true), set to this
   *        backend's cardinality; on subsequent calls, used as the canonical cardinality to compare against.
   * @param first_backend Input/output flag indicating whether this is the first backend being processed.
   *        If true, the function updates `canonical_cost` and `canonical_cardinality` and sets this flag to false.
   *
   * Side effects:
   * - Writes a result line to stdout with cardinality, cost, and matched pairs.
   * - Writes a warning to stderr if this backend's cost or cardinality differs from the canonical values.
   */
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


  /**
   * @brief Execute a min-cost matching demonstration for a scenario across backends.
   *
   * Runs the scenario `s` for both objective modes (pure minimum-cost and
   * maximum-cardinality-then-minimum-cost), invokes the matching demonstration
   * for each graph backend, and prints per-backend results and consistency
   * warnings to standard output.
   *
   * @param s Scenario describing title, node count, and edge list used to build graphs.
   */
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
  /**
   * @brief Build the scenario graph for a backend, check for a feasible perfect matching, and print the result.
   *
   * Builds a graph from the provided Scenario, determines whether a perfect matching exists,
   * and writes a one-line summary to standard output. If a feasible perfect matching is found,
   * prints the matching cardinality, total cost, and the list of matched node pairs.
   *
   * @tparam GT Graph backend type used to construct and analyze the graph.
   * @param backend_name Human-readable name of the graph backend (printed as part of the output).
   * @param s Scenario describing node count and weighted edges to populate the graph.
   */
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


  /**
   * @brief Runs the perfect-matching demonstration for all supported graph backends.
   *
   * Builds graphs from the provided scenario, invokes the perfect matching routine for
   * each backend, and prints the formatted results (feasibility, cardinality, cost, and pairs).
   *
   * @param s Scenario containing the demo title, node count, and edge list.
   */
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
} /**
 * @brief Runs example demonstrations for minimum-cost matching and perfect matching.
 *
 * Builds and executes three example scenarios that demonstrate the dedicated
 * Blossom APIs (minimum-cost matching and minimum-cost perfect matching)
 * across the provided backends, printing results and consistency checks to
 * standard output.
 *
 * @return int 0 on success.
 */


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