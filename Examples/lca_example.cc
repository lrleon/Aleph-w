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
 * @file lca_example.cc
 * @brief Human-friendly demo of LCA on Aleph tree graphs.
 *
 * Demonstrates:
 * - Binary lifting LCA (`Binary_Lifting_LCA`)
 * - Euler tour + RMQ LCA (`Euler_RMQ_LCA`)
 * - Distance queries and cross-validation between engines
 * - Cross-backend parity (`List_Graph`, `List_SGraph`, `Array_Graph`)
 */

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>

#include <LCA.H>
#include <tpl_array.H>
#include <tpl_agraph.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
  enum Node_Id : size_t
  {
    CEO = 0,
    CTO = 1,
    CFO = 2,
    PLATFORM = 3,
    DATA = 4,
    ACCOUNTING = 5,
    LEGAL = 6,
    SRE = 7,
    ML = 8,
    NODES = 9
  };

  static const std::array<const char *, NODES> NODE_LABELS = {
      "CEO", "CTO", "CFO", "Platform", "Data",
      "Accounting", "Legal", "SRE", "ML"
  };

  struct Query
  {
    size_t u;
    size_t v;
    size_t expected_lca;
    size_t expected_distance;
  };

  static const std::array<Query, 6> QUERIES = {{
      {SRE, ML, CTO, 4},
      {SRE, DATA, CTO, 3},
      {ACCOUNTING, LEGAL, CFO, 2},
      {ACCOUNTING, ML, CEO, 5},
      {PLATFORM, CTO, CTO, 1},
      {CTO, CFO, CEO, 2}
  }};

  struct Backend_Result
  {
    const char * backend_name = "";
    bool deterministic_ok = true;
    bool engines_agree = true;
    bool checksum_equal = true;
    std::uint64_t binary_ms = 0;
    std::uint64_t euler_ms = 0;
    std::uint64_t checksum_bin = 0;
    std::uint64_t checksum_eul = 0;
  };

  /**
   * @brief Builds a small rooted tree on the specified graph backend, runs two LCA engines,
   * validates their results against a deterministic query set, and benchmarks them with many random queries.
   *
   * Constructs a fixed "company hierarchy" tree in the provided graph type, instantiates
   * Binary_Lifting_LCA and Euler_RMQ_LCA with the root at CEO, compares engine outputs for a
   * predefined set of queries (checking LCA and distance), and measures runtime and checksums
   * for 200,000 deterministic random queries to compare performance and result parity.
   *
   * @param backend_name Human-readable name of the graph backend (used in console output and result reporting).
   * @return Backend_Result Aggregated results for the backend including deterministic consistency flags,
   *         per-engine benchmark times (milliseconds), per-engine checksums, and a checksum equality flag.
   */
  template <class GT>
  Backend_Result run_backend(const char * backend_name)
  {
    using Graph = GT;
    using Node = typename Graph::Node;

    Graph g;
    auto nodes = Array<Node *>::create(NODES);
    for (size_t i = 0; i < NODES; ++i)
      nodes(i) = g.insert_node(static_cast<int>(i));

    // "Company hierarchy" as a rooted tree.
    g.insert_arc(nodes(CEO), nodes(CTO), 1);
    g.insert_arc(nodes(CEO), nodes(CFO), 1);
    g.insert_arc(nodes(CTO), nodes(PLATFORM), 1);
    g.insert_arc(nodes(CTO), nodes(DATA), 1);
    g.insert_arc(nodes(CFO), nodes(ACCOUNTING), 1);
    g.insert_arc(nodes(CFO), nodes(LEGAL), 1);
    g.insert_arc(nodes(PLATFORM), nodes(SRE), 1);
    g.insert_arc(nodes(DATA), nodes(ML), 1);

    Binary_Lifting_LCA<Graph> binary_lca(g, nodes(CEO));
    Euler_RMQ_LCA<Graph> euler_lca(g, nodes(CEO));

    Backend_Result result;
    result.backend_name = backend_name;

    std::cout << "Backend: " << backend_name << "\n";
    std::cout << std::left
              << std::setw(18) << "Query(u,v)"
              << std::setw(14) << "Binary LCA"
              << std::setw(14) << "Euler LCA"
              << std::setw(10) << "Distance"
              << "Status\n";
    std::cout << std::string(66, '-') << "\n";

    for (const auto & q : QUERIES)
      {
        Node * lca_bin = binary_lca.lca(nodes(q.u), nodes(q.v));
        Node * lca_eul = euler_lca.lca(nodes(q.u), nodes(q.v));

        const auto bl_idx = static_cast<size_t>(lca_bin->get_info());
        const auto er_idx = static_cast<size_t>(lca_eul->get_info());
        const size_t dist = binary_lca.distance(nodes(q.u), nodes(q.v));

        const bool agree = (lca_bin == lca_eul);
        const bool expected = (bl_idx == q.expected_lca) and
                              (er_idx == q.expected_lca) and
                              (dist == q.expected_distance);

        result.engines_agree = result.engines_agree and agree;
        result.deterministic_ok = result.deterministic_ok and expected;

        std::cout << std::left
                  << std::setw(18)
                  << (std::string(NODE_LABELS[q.u]) + "," + NODE_LABELS[q.v])
                  << std::setw(14) << NODE_LABELS[bl_idx]
                  << std::setw(14) << NODE_LABELS[er_idx]
                  << std::setw(10) << dist
                  << ((agree and expected) ? "OK" : "MISMATCH")
                  << "\n";
      }

    constexpr size_t qcount = 200000;
    auto random_queries = Array<std::pair<size_t, size_t>>::create(qcount);
    std::mt19937 rng(0x1ca2026u);
    std::uniform_int_distribution<size_t> pick(0, NODES - 1);
    for (size_t i = 0; i < qcount; ++i)
      random_queries(i) = std::make_pair(pick(rng), pick(rng));

    auto run_binary = [&]() -> std::pair<std::uint64_t, std::uint64_t>
    {
      const auto t0 = std::chrono::high_resolution_clock::now();
      std::uint64_t checksum = 0;
      for (size_t i = 0; i < qcount; ++i)
        {
          const auto [u, v] = random_queries(i);
          checksum += static_cast<std::uint64_t>(binary_lca.lca(nodes(u), nodes(v))->get_info() + 1);
        }
      const auto t1 = std::chrono::high_resolution_clock::now();
      const auto ms = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
      return std::make_pair(ms, checksum);
    };

    auto run_euler = [&]() -> std::pair<std::uint64_t, std::uint64_t>
    {
      const auto t0 = std::chrono::high_resolution_clock::now();
      std::uint64_t checksum = 0;
      for (size_t i = 0; i < qcount; ++i)
        {
          const auto [u, v] = random_queries(i);
          checksum += static_cast<std::uint64_t>(euler_lca.lca(nodes(u), nodes(v))->get_info() + 1);
        }
      const auto t1 = std::chrono::high_resolution_clock::now();
      const auto ms = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
      return std::make_pair(ms, checksum);
    };

    const auto [binary_ms, checksum_bin] = run_binary();
    const auto [euler_ms, checksum_eul] = run_euler();

    result.binary_ms = binary_ms;
    result.euler_ms = euler_ms;
    result.checksum_bin = checksum_bin;
    result.checksum_eul = checksum_eul;
    result.checksum_equal = (checksum_bin == checksum_eul);

    std::cout << "Benchmark (" << qcount << " random queries):\n";
    std::cout << "  Binary lifting: " << binary_ms << " ms\n";
    std::cout << "  Euler + RMQ:    " << euler_ms << " ms\n";
    std::cout << "  Checksums equal: " << (result.checksum_equal ? "yes" : "no") << "\n\n";

    return result;
  }
}

/**
 * @brief Runs the LCA cross-backend parity demo and prints results.
 *
 * Builds and executes the LCA example for three graph backends, collects per-backend
 * timing and checksum metrics, prints a parity summary table, and reports whether
 * all backends produced identical checksums.
 *
  * @return int 0 on success, non-zero on failure.
  */
 int main()
 {
   using LG = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
   using SG = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;
   using AG = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;
 
   std::cout << "==============================================================\n";
   std::cout << " LCA on Aleph Tree Graphs (Cross-Backend Parity Demo)\n";
   std::cout << "==============================================================\n\n";
 
   const auto list_result = run_backend<LG>("List_Graph");
   const auto sgraph_result = run_backend<SG>("List_SGraph");
   const auto agraph_result = run_backend<AG>("Array_Graph");
 
   std::cout << "Parity summary:\n";
   std::cout << std::left
             << std::setw(13) << "Backend"
             << std::setw(14) << "Deterministic"
             << std::setw(14) << "EnginesAgree"
             << std::setw(14) << "ChecksumEq"
             << std::setw(11) << "Binary(ms)"
             << "Euler(ms)\n";
   std::cout << std::string(72, '-') << "\n";
 
   bool any_failure = false;
   const std::array<Backend_Result, 3> results = {list_result, sgraph_result, agraph_result};
   for (const auto & r : results)
     {
       std::cout << std::left
                 << std::setw(13) << r.backend_name
                 << std::setw(14) << (r.deterministic_ok ? "yes" : "no")
                 << std::setw(14) << (r.engines_agree ? "yes" : "no")
                 << std::setw(14) << (r.checksum_equal ? "yes" : "no")
                 << std::setw(11) << r.binary_ms
                 << r.euler_ms << "\n";
 
       if (not r.deterministic_ok or not r.engines_agree or not r.checksum_equal)
         any_failure = true;
     }
 
   const bool cross_backend_checksum_ok =
       list_result.checksum_bin == sgraph_result.checksum_bin and
       list_result.checksum_bin == agraph_result.checksum_bin;
 
   std::cout << "\nCross-backend checksum parity: "
             << (cross_backend_checksum_ok ? "yes" : "no") << "\n";
 
   if (not cross_backend_checksum_ok)
     any_failure = true;
 
   return any_failure ? 1 : 0;
 }
 