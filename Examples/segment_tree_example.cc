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
 * @file segment_tree_example.cc
 * @brief Four creative scenarios demonstrating Segment Tree variants
 *
 * SCENARIO 1 — "El Arbol del Tiempo" (Min_Segment_Tree)
 * ======================================================
 * A historian maintains a timeline of the oldest known artifact per
 * century (indices 0..9 represent centuries 1-10).  She queries
 * "what is the oldest artifact across centuries 3-7?" and updates
 * individual centuries when new archaeological discoveries push dates
 * further back.
 *
 * SCENARIO 2 — "Ajustes salariales" (Lazy_Sum_Segment_Tree)
 * ===========================================================
 * An HR system tracks salaries by department index.  Corporate
 * announces range raises ("add $500 to departments 3-7") and the
 * CFO queries "total payroll for departments 2-5".  This is the
 * canonical range-update + range-query pattern.
 *
 * SCENARIO 3 — "Balanceo de servidores" (Segment_Tree_Beats)
 * ============================================================
 * A data center has server racks with varying load.  Operations
 * applies chmin (cap load) and chmax (set minimum floor) across
 * rack ranges, then queries total load per section.
 *
 * SCENARIO 4 — "RMQ dinámico vs estático"
 * =========================================
 * Side-by-side comparison showing when to use Sparse Table (static,
 * O(1) query) vs Segment Tree (dynamic, O(log n) query).
 */

# include <tpl_segment_tree.H>
# include <tpl_sparse_table.H>
# include <iostream>
# include <iomanip>

using namespace Aleph;

static void scenario_1_el_arbol_del_tiempo()
{
  std::cout << "=== SCENARIO 1: El Arbol del Tiempo ===\n\n";
  std::cout << "A historian tracks the oldest artifact per century.\n";
  std::cout << "Index i represents century (i+1).\n\n";

  // Oldest artifact year per century (lower = older)
  // Centuries 1-10
  Min_Segment_Tree<int> timeline = {
    -3000,  // century 1: oldest artifact from 3000 BC
    -2500,  // century 2
    -1800,  // century 3
    -1200,  // century 4
    -800,   // century 5
    -500,   // century 6
    -200,   // century 7
    100,    // century 8
    400,    // century 9
    700     // century 10
  };

  std::cout << "Initial oldest dates per century (negative = BC):\n";
  for (size_t i = 0; i < timeline.size(); ++i)
    std::cout << "  Century " << (i + 1) << ": " << timeline.get(i) << "\n";

  std::cout << "\nOldest artifact in centuries 3-7: "
            << timeline.query(2, 6) << "\n";
  std::cout << "Oldest artifact in centuries 1-5: "
            << timeline.query(0, 4) << "\n";

  // New archaeological discovery!
  std::cout << "\nNew discovery: artifact from -2000 found in century 5!\n";
  timeline.set(4, -2000);

  std::cout << "Oldest artifact in centuries 3-7 (updated): "
            << timeline.query(2, 6) << "\n";

  std::cout << "\n";
}

static void scenario_2_ajustes_salariales()
{
  std::cout << "=== SCENARIO 2: Ajustes Salariales ===\n\n";
  std::cout << "HR system: 8 departments, salary adjustments and payroll queries.\n\n";

  // Initial salaries per department (in thousands)
  Lazy_Sum_Segment_Tree<int> payroll = {50, 45, 60, 55, 70, 48, 52, 65};

  std::cout << "Initial payroll per department (in $K):\n";
  for (size_t i = 0; i < payroll.size(); ++i)
    std::cout << "  Dept " << i << ": $" << payroll.get(i) << "K\n";

  std::cout << "\nTotal payroll (all depts): $"
            << payroll.query(0, 7) << "K\n";

  // Range raise: departments 2-5 get +$10K
  std::cout << "\nCorporate raises: departments 2-5 get +$10K\n";
  payroll.update(2, 5, 10);

  std::cout << "Total payroll (all depts): $"
            << payroll.query(0, 7) << "K\n";
  std::cout << "Payroll for depts 2-5: $"
            << payroll.query(2, 5) << "K\n";

  // Another raise: departments 0-3 get +$5K
  std::cout << "\nAnother raise: departments 0-3 get +$5K\n";
  payroll.update(0, 3, 5);

  std::cout << "Updated payroll per department:\n";
  for (size_t i = 0; i < payroll.size(); ++i)
    std::cout << "  Dept " << i << ": $" << payroll.get(i) << "K\n";

  std::cout << "Total payroll: $" << payroll.query(0, 7) << "K\n";

  std::cout << "\n";
}

static void scenario_3_balanceo_servidores()
{
  std::cout << "=== SCENARIO 3: Balanceo de Servidores (Beats) ===\n\n";
  std::cout << "Data center: 8 racks with varying CPU load (%).\n\n";

  Segment_Tree_Beats<int> loads = {75, 90, 45, 60, 85, 30, 95, 50};

  std::cout << "Initial loads per rack:\n";
  for (size_t i = 0; i < loads.size(); ++i)
    std::cout << "  Rack " << i << ": " << loads.get(i) << "%\n";

  std::cout << "\nTotal load: " << loads.query_sum(0, 7) << "%\n";
  std::cout << "Max load: " << loads.query_max(0, 7) << "%\n";
  std::cout << "Min load: " << loads.query_min(0, 7) << "%\n";

  // Cap all loads at 80% (throttle overloaded servers)
  std::cout << "\nOperations: cap all loads at 80% (chmin)\n";
  loads.chmin(0, 7, 80);

  std::cout << "After capping:\n";
  for (size_t i = 0; i < loads.size(); ++i)
    std::cout << "  Rack " << i << ": " << loads.get(i) << "%\n";
  std::cout << "Total load: " << loads.query_sum(0, 7) << "%\n";
  std::cout << "Max load: " << loads.query_max(0, 7) << "%\n";

  // Set minimum floor at 50% (ensure baseline utilisation)
  std::cout << "\nOperations: set minimum floor at 50% for racks 0-3 (chmax)\n";
  loads.chmax(0, 3, 50);

  std::cout << "After floor:\n";
  for (size_t i = 0; i < loads.size(); ++i)
    std::cout << "  Rack " << i << ": " << loads.get(i) << "%\n";
  std::cout << "Total load: " << loads.query_sum(0, 7) << "%\n";

  std::cout << "\n";
}

static void scenario_4_rmq_comparison()
{
  std::cout << "=== SCENARIO 4: RMQ dinamico vs estatico ===\n\n";

  const std::vector<int> data = {9, 4, 7, 1, 8, 2, 6, 3, 5, 0};

  // Static: Sparse Table — O(1) query, no updates
  Sparse_Table<int> sparse(data);

  // Dynamic: Segment Tree — O(log n) query, O(log n) updates
  Min_Segment_Tree<int> segtree(data);

  std::cout << "Data: {9, 4, 7, 1, 8, 2, 6, 3, 5, 0}\n\n";

  std::cout << "Query     | Sparse Table (O(1)) | Segment Tree (O(lg n))\n";
  std::cout << "----------|--------------------|-----------------------\n";

  auto show_query = [&](size_t l, size_t r) {
    std::cout << "[" << l << ", " << r << "]     | "
              << std::setw(19) << sparse.query(l, r) << " | "
              << std::setw(21) << segtree.query(l, r) << "\n";
  };

  show_query(0, 9);
  show_query(2, 6);
  show_query(5, 8);

  std::cout << "\nNow update a[3] = 100 (only Segment Tree supports this):\n";
  segtree.set(3, 100);

  std::cout << "Segment Tree min[0, 9] after update: "
            << segtree.query(0, 9) << "\n";
  std::cout << "Sparse Table min[0, 9] (unchanged):  "
            << sparse.query(0, 9) << "\n";

  std::cout << "\nWhen to use each:\n";
  std::cout << "  Sparse Table: static data, many queries, O(1) per query\n";
  std::cout << "  Segment Tree: dynamic data with updates, O(lg n) per query\n";
  std::cout << "  Fenwick Tree:  dynamic data, invertible operation (e.g. sum)\n";
}

int main()
{
  scenario_1_el_arbol_del_tiempo();
  scenario_2_ajustes_salariales();
  scenario_3_balanceo_servidores();
  scenario_4_rmq_comparison();

  return 0;
}
