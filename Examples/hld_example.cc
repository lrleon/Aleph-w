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
 * @file hld_example.cc
 * @brief Human-friendly demo of Heavy-Light Decomposition on Aleph tree graphs.
 *
 * Demonstrates three real-world scenarios:
 * 1. Corporate Hierarchy — Path Max (Security Clearance)
 * 2. Network Infrastructure — Path Min (Bandwidth Bottleneck)
 * 3. Tax Routes — Path Sum + Dynamic Updates
 */

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

#include <HLD.H>
#include <tpl_array.H>
#include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = G::Node;

  // ================================================================
  // Scenario 1: Corporate Hierarchy — Security Clearance (Path Max)
  // ================================================================
  void scenario_security_clearance()
  {
    std::cout << "============================================================\n";
    std::cout << " Scenario 1: Corporate Hierarchy — Security Clearance\n";
    std::cout << "============================================================\n\n";
    std::cout << " Find the highest security clearance on the path between\n";
    std::cout << " two employees in the org chart.\n\n";

    // Org chart:
    //          CEO(0)  clearance=5
    //         /      |
    //      CTO(1)   CFO(2)   clearance=4, 3
    //      / |       / |
    //   Eng(3) Data(4) Acct(5) Legal(6)  clearance=2,3,1,2
    //          |
    //        ML(7)  clearance=3

    G g;
    auto n = Array<Node *>::create(8);
    for (size_t i = 0; i < 8; ++i)
      n(i) = g.insert_node(static_cast<int>(i));

    g.insert_arc(n(0), n(1), 1);
    g.insert_arc(n(0), n(2), 1);
    g.insert_arc(n(1), n(3), 1);
    g.insert_arc(n(1), n(4), 1);
    g.insert_arc(n(2), n(5), 1);
    g.insert_arc(n(2), n(6), 1);
    g.insert_arc(n(4), n(7), 1);

    const char * names[] = {"CEO", "CTO", "CFO", "Eng", "Data",
                            "Acct", "Legal", "ML"};
    const int clearance[] = {5, 4, 3, 2, 3, 1, 2, 3};

    auto nv = [&clearance](Node * p) { return clearance[p->get_info()]; };
    HLD_Max<G, int> hld(g, n(0), nv);

    struct Query { size_t u, v; };
    constexpr Query queries[] = {{3, 7}, {5, 7}, {3, 6}, {7, 7}};

    std::cout << std::left
              << std::setw(20) << "Query(u,v)"
              << std::setw(12) << "Max Clear."
              << std::setw(12) << "Distance"
              << "LCA\n";
    std::cout << std::string(54, '-') << "\n";

    for (const auto & q : queries)
      {
        const int mx = hld.path_query(n(q.u), n(q.v));
        const size_t dist = hld.distance(n(q.u), n(q.v));
        auto * l = hld.lca(n(q.u), n(q.v));
        const auto lid = static_cast<size_t>(l->get_info());

        std::cout << std::left
                  << std::setw(20)
                  << (std::string(names[q.u]) + "," + names[q.v])
                  << std::setw(12) << mx
                  << std::setw(12) << dist
                  << names[lid] << "\n";
      }

    std::cout << "\nHLD chains: " << hld.num_chains() << "\n\n";
  }

  // ================================================================
  // Scenario 2: Network — Bandwidth Bottleneck (Path Min)
  // ================================================================
  void scenario_bandwidth_bottleneck()
  {
    std::cout << "============================================================\n";
    std::cout << " Scenario 2: Network — Bandwidth Bottleneck\n";
    std::cout << "============================================================\n\n";
    std::cout << " Find the minimum bandwidth (bottleneck) on the path\n";
    std::cout << " between two servers in a tree network.\n";
    std::cout << " Edge weights stored on child nodes.\n\n";

    // Network tree:
    //        Router(0)  bw=0 (root, unused)
    //       /    |    |
    //    S1(1) S2(2) S3(3)  bw=100, 50, 80 (link to router)
    //    /       |
    //  S4(4)    S5(5)       bw=30, 60 (link to parent)
    //    |
    //  S6(6)                bw=90

    G g;
    auto n = Array<Node *>::create(7);
    for (size_t i = 0; i < 7; ++i)
      n(i) = g.insert_node(static_cast<int>(i));

    g.insert_arc(n(0), n(1), 1);
    g.insert_arc(n(0), n(2), 1);
    g.insert_arc(n(0), n(3), 1);
    g.insert_arc(n(1), n(4), 1);
    g.insert_arc(n(2), n(5), 1);
    g.insert_arc(n(4), n(6), 1);

    const char * names[] = {"Router", "S1", "S2", "S3", "S4", "S5", "S6"};
    const int bandwidth[] = {1000, 100, 50, 80, 30, 60, 90};

    auto nv = [&bandwidth](Node * p) { return bandwidth[p->get_info()]; };
    HLD_Min<G, int> hld(g, n(0), nv);

    struct Query { size_t u, v; };
    const Query queries[] = {{6, 5}, {4, 3}, {6, 3}, {1, 2}};

    std::cout << std::left
              << std::setw(16) << "Path"
              << std::setw(18) << "Bottleneck(edge)"
              << "Distance\n";
    std::cout << std::string(44, '-') << "\n";

    for (const auto & q : queries)
      {
        // Edge-weighted: skip LCA node
        const int bw = hld.path_query_edges(n(q.u), n(q.v));
        const size_t dist = hld.distance(n(q.u), n(q.v));

        std::cout << std::left
                  << std::setw(16)
                  << (std::string(names[q.u]) + " -> " + names[q.v])
                  << std::setw(18) << bw
                  << dist << " hops\n";
      }

    std::cout << "\n";
  }

  // ================================================================
  // Scenario 3: Tax Routes — Path Sum + Dynamic Updates
  // ================================================================
  void scenario_tax_routes()
  {
    std::cout << "============================================================\n";
    std::cout << " Scenario 3: Tax Routes — Sum + Dynamic Updates\n";
    std::cout << "============================================================\n\n";
    std::cout << " Sum taxes on a trade route (path between cities).\n";
    std::cout << " Update taxes after policy change.\n\n";

    //         Capital(0)  tax=10
    //        /          |
    //    Port(1)      Market(2)  tax=5, 8
    //    / |            |
    // Farm(3) Mine(4) Workshop(5)  tax=3, 7, 6

    G g;
    auto n = Array<Node *>::create(6);
    for (size_t i = 0; i < 6; ++i)
      n(i) = g.insert_node(static_cast<int>(i));

    g.insert_arc(n(0), n(1), 1);
    g.insert_arc(n(0), n(2), 1);
    g.insert_arc(n(1), n(3), 1);
    g.insert_arc(n(1), n(4), 1);
    g.insert_arc(n(2), n(5), 1);

    const char * names[] = {"Capital", "Port", "Market",
                            "Farm", "Mine", "Workshop"};
    const int taxes[] = {10, 5, 8, 3, 7, 6};

    auto nv = [&taxes](Node * p) { return taxes[p->get_info()]; };
    HLD_Sum<G, int> hld(g, n(0), nv);

    auto print_query = [&](const char * label, size_t u, size_t v)
    {
      std::cout << "  " << std::left << std::setw(25) << label
                << "path_sum(" << names[u] << ", " << names[v] << ") = "
                << hld.path_query(n(u), n(v)) << "\n";
    };

    std::cout << "Before policy change:\n";
    print_query("Farm->Workshop", 3, 5);
    print_query("Mine->Workshop", 4, 5);
    print_query("Farm->Mine", 3, 4);

    std::cout << "\n  subtree_sum(Port) = "
              << hld.subtree_query(n(1)) << "\n";
    std::cout << "  subtree_sum(Capital) = "
              << hld.subtree_query(n(0)) << "\n";

    // Policy change: Capital tax doubles
    std::cout << "\nPolicy change: Capital tax 10 -> 20\n";
    hld.point_update(n(0), 20);

    std::cout << "\nAfter policy change:\n";
    print_query("Farm->Workshop", 3, 5);
    print_query("Mine->Workshop", 4, 5);

    std::cout << "\n  subtree_sum(Capital) = "
              << hld.subtree_query(n(0)) << "\n";

    // Another update
    std::cout << "\nNew tax-free zone at Port (5 -> 0):\n";
    hld.point_update(n(1), 0);

    print_query("Farm->Workshop", 3, 5);
    std::cout << "  subtree_sum(Port) = "
              << hld.subtree_query(n(1)) << "\n";

    std::cout << "\n";
  }
}


int main()
{
  std::cout << "==============================================================\n";
  std::cout << " Heavy-Light Decomposition on Aleph Tree Graphs\n";
  std::cout << "==============================================================\n\n";

  scenario_security_clearance();
  scenario_bandwidth_bottleneck();
  scenario_tax_routes();

  std::cout << "All scenarios completed successfully.\n";

  return 0;
}
