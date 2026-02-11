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
 * @file cartesian_tree_example.cc
 * @brief Four creative scenarios demonstrating Cartesian Tree, LCA, and RMQ.
 *
 * SCENARIO 1 — "El Arbol Genealogico de los Datos"
 * =================================================
 * Visualizes the Cartesian Tree as a hierarchy: given an array of ages,
 * build the tree and show parent/children.  Demonstrates heap property
 * + inorder = original array.
 *
 * SCENARIO 2 — "Ancestros Comunes en el Linaje"
 * ===============================================
 * Uses Euler_Tour_LCA to find common ancestors.  Visualizes the Euler
 * Tour and depths.
 *
 * SCENARIO 3 — "RMQ sin Trucos: de Arbol a Respuestas O(1)"
 * ===========================================================
 * Builds Cartesian_Tree_RMQ and compares results with Sparse_Table on
 * the same data.  Shows the chain CT -> Euler Tour -> Sparse Table -> O(1).
 *
 * SCENARIO 4 — "El Circulo Completo: RMQ <-> LCA"
 * ================================================
 * Demonstrates the equivalence: RMQ(l,r) = value at LCA(l,r).
 * Comparative timing of build/query vs Sparse Table.
 */

# include <tpl_cartesian_tree.H>
# include <tpl_sparse_table.H>

# include <iostream>
# include <iomanip>
# include <chrono>
# include <random>
# include <vector>

using namespace Aleph;

// ================================================================
//  SCENARIO 1 — El Arbol Genealogico de los Datos
/**
 * @brief Demonstrates building and inspecting a min-heap Cartesian Tree from a small array.
 *
 * Prints the input array, the tree root and its value, a table of each node's index,
 * value, parent, left/right child, and whether it is a leaf, the tree height, the
 * inorder traversal of node indices, and a verification that the inorder sequence
 * equals {0, 1, ..., n-1}.
 */

static void scenario_1()
{
  std::cout << "\n"
    "========================================================\n"
    " SCENARIO 1: El Arbol Genealogico de los Datos\n"
    "========================================================\n\n";

  // Ages of people in a family line
  Cartesian_Tree<int> ct = {45, 23, 67, 12, 56, 34, 78};

  std::cout << "Array of ages: {45, 23, 67, 12, 56, 34, 78}\n\n";

  std::cout << "Cartesian Tree structure (min-heap):\n";
  std::cout << "  Root: index " << ct.root()
            << " (age " << ct.data_at(ct.root()) << ")\n\n";

  std::cout << std::setw(8) << "Index" << std::setw(8) << "Age"
            << std::setw(10) << "Parent" << std::setw(10) << "Left"
            << std::setw(10) << "Right" << std::setw(8) << "Leaf?"
            << "\n";
  std::cout << std::string(54, '-') << "\n";

  for (size_t i = 0; i < ct.size(); ++i)
    {
      auto fmt = [](size_t v) -> std::string {
        return v == Cartesian_Tree<int>::NONE ? "-" : std::to_string(v);
      };
      std::cout << std::setw(8) << i
                << std::setw(8) << ct.data_at(i)
                << std::setw(10) << fmt(ct.parent_of(i))
                << std::setw(10) << fmt(ct.left_child(i))
                << std::setw(10) << fmt(ct.right_child(i))
                << std::setw(8) << (ct.is_leaf(i) ? "yes" : "no")
                << "\n";
    }

  std::cout << "\nTree height: " << ct.height() << "\n";

  // Verify inorder = original order
  auto io = ct.inorder();
  std::cout << "\nInorder traversal: {";
  for (size_t i = 0; i < io.size(); ++i)
    std::cout << (i ? ", " : "") << io(i);
  std::cout << "}\n";

  bool inorder_ok = true;
  for (size_t i = 0; i < io.size(); ++i)
    if (io(i) != i) { inorder_ok = false; break; }
  std::cout << "Inorder = {0, 1, ..., n-1}? "
            << (inorder_ok ? "YES" : "NO") << "\n";
}


// ================================================================
//  SCENARIO 2 — Ancestros Comunes en el Linaje
/**
 * @brief Demonstrates Euler-tour based LCA operations on a sample array.
 *
 * Prints the input array, the Euler tour and its size, each node's depth with its value,
 * and several example LCA queries. For each query it prints the LCA index, the value
 * at that index, and the distance between the queried nodes.
 */

static void scenario_2()
{
  std::cout << "\n"
    "========================================================\n"
    " SCENARIO 2: Ancestros Comunes en el Linaje\n"
    "========================================================\n\n";

  Euler_Tour_LCA<int> lca = {45, 23, 67, 12, 56, 34, 78};

  std::cout << "Array: {45, 23, 67, 12, 56, 34, 78}\n\n";

  // Show the Euler Tour
  auto euler = lca.euler_tour();
  std::cout << "Euler Tour (" << lca.euler_tour_size() << " entries):\n  ";
  for (size_t i = 0; i < euler.size(); ++i)
    std::cout << (i ? ", " : "") << euler(i);
  std::cout << "\n\n";

  // Show depths
  std::cout << "Node depths:\n";
  for (size_t i = 0; i < lca.size(); ++i)
    std::cout << "  Node " << i << " (value "
              << lca.tree().data_at(i) << "): depth "
              << lca.depth_of(i) << "\n";

  // Query some LCAs
  std::cout << "\nLCA queries:\n";
  auto show_lca = [&](size_t u, size_t v) {
    size_t a = lca.lca(u, v);
    std::cout << "  LCA(" << u << ", " << v << ") = " << a
              << " (value " << lca.tree().data_at(a) << ")"
              << "  distance = " << lca.distance(u, v) << "\n";
  };

  show_lca(0, 2);  // siblings under node 1
  show_lca(0, 4);  // across root
  show_lca(4, 6);  // right subtree
  show_lca(1, 5);  // different subtrees
  show_lca(3, 3);  // self
}


// ================================================================
//  SCENARIO 3 — RMQ sin Trucos: de Arbol a Respuestas O(1)
/**
 * @brief Compares RMQ results from a Cartesian Tree-based RMQ and a Sparse Table on a fixed example.
 *
 * Builds a Cartesian_Tree_RMQ and a Sparse_Table over a small sample array, runs several range-min queries,
 * prints a formatted comparison table showing whether both structures agree for each range, and demonstrates
 * index-based RMQ via `query_idx`.
 */

static void scenario_3()
{
  std::cout << "\n"
    "========================================================\n"
    " SCENARIO 3: RMQ sin Trucos: de Arbol a Respuestas O(1)\n"
    "========================================================\n\n";

  std::vector<int> data = {5, 2, 4, 7, 1, 3, 6, 8, 0, 9};
  std::cout << "Array: {5, 2, 4, 7, 1, 3, 6, 8, 0, 9}\n\n";

  Cartesian_Tree_RMQ<int> ct_rmq(data);
  Sparse_Table<int> sparse(data);

  // Compare queries
  std::cout << "Comparing Cartesian Tree RMQ vs Sparse Table:\n\n";
  std::cout << std::setw(12) << "Range"
            << std::setw(12) << "CT-RMQ"
            << std::setw(12) << "Sparse"
            << std::setw(10) << "Match?"
            << "\n";
  std::cout << std::string(46, '-') << "\n";

  auto test_range = [&](size_t l, size_t r) {
    int ct_val = ct_rmq.query(l, r);
    int sp_val = sparse.query(l, r);
    std::string range = "[" + std::to_string(l) + "," + std::to_string(r) + "]";
    std::cout << std::setw(12) << range
              << std::setw(12) << ct_val
              << std::setw(12) << sp_val
              << std::setw(10) << (ct_val == sp_val ? "OK" : "FAIL")
              << "\n";
  };

  test_range(0, 3);
  test_range(2, 6);
  test_range(0, 9);
  test_range(4, 4);
  test_range(7, 9);
  test_range(1, 8);

  std::cout << "\nquery_idx(2, 6) = " << ct_rmq.query_idx(2, 6)
            << " (value " << ct_rmq.get(ct_rmq.query_idx(2, 6)) << ")\n";
}


// ================================================================
//  SCENARIO 4 — El Circulo Completo: RMQ <-> LCA
/**
 * @brief Demonstrates and benchmarks the equivalence between RMQ and LCA on large random data.
 *
 * Builds a Cartesian Tree RMQ and a Sparse Table over a large random array, measures and prints
 * their build and query times, and verifies for a subset of queries that RMQ(l,r) equals the
 * tree value at LCA(l,r). Prints formatted timing and verification results to stdout.
 */

static void scenario_4()
{
  std::cout << "\n"
    "========================================================\n"
    " SCENARIO 4: El Circulo Completo: RMQ <-> LCA\n"
    "========================================================\n\n";

  constexpr size_t N = 100000;
  constexpr size_t Q = 500000;

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-100000, 100000);

  std::vector<int> data(N);
  for (auto & x : data)
    x = dist(rng);

  std::cout << "N = " << N << " elements, Q = " << Q << " queries\n\n";

  // Build Cartesian Tree RMQ
  auto t0 = std::chrono::high_resolution_clock::now();
  Cartesian_Tree_RMQ<int> ct_rmq(data);
  auto t1 = std::chrono::high_resolution_clock::now();
  double ct_build_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  // Build Sparse Table
  t0 = std::chrono::high_resolution_clock::now();
  Sparse_Table<int> sparse(data);
  t1 = std::chrono::high_resolution_clock::now();
  double sp_build_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  std::cout << "Build times:\n";
  std::cout << "  Cartesian Tree RMQ: " << std::fixed << std::setprecision(2)
            << ct_build_ms << " ms\n";
  std::cout << "  Sparse Table:       " << sp_build_ms << " ms\n\n";

  // Generate random queries
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);
  std::vector<std::pair<size_t, size_t>> queries(Q);
  for (auto & [l, r] : queries)
    {
      l = idx_dist(rng);
      r = idx_dist(rng);
      if (l > r)
        std::swap(l, r);
    }

  // Time Cartesian Tree RMQ queries
  t0 = std::chrono::high_resolution_clock::now();
  volatile int sink = 0;
  for (auto & [l, r] : queries)
    sink = ct_rmq.query(l, r);
  t1 = std::chrono::high_resolution_clock::now();
  double ct_query_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  // Time Sparse Table queries
  t0 = std::chrono::high_resolution_clock::now();
  for (auto & [l, r] : queries)
    sink = sparse.query(l, r);
  t1 = std::chrono::high_resolution_clock::now();
  double sp_query_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  std::cout << "Query times (" << Q << " queries):\n";
  std::cout << "  Cartesian Tree RMQ: " << ct_query_ms << " ms\n";
  std::cout << "  Sparse Table:       " << sp_query_ms << " ms\n\n";

  // Verify equivalence: RMQ(l,r) = data_at(LCA(l,r))
  std::cout << "Verifying RMQ(l,r) = data[LCA(l,r)] for first 10000 queries... ";
  bool all_ok = true;
  const auto & lca_engine = ct_rmq.lca_engine();
  for (size_t i = 0; i < std::min<size_t>(10000, Q); ++i)
    {
      auto [l, r] = queries[i];
      size_t ancestor = lca_engine.lca(l, r);
      int via_lca = lca_engine.tree().data_at(ancestor);
      int via_rmq = ct_rmq.query(l, r);
      int via_sparse = sparse.query(l, r);
      if (via_lca != via_rmq || via_rmq != via_sparse)
        {
          all_ok = false;
          break;
        }
    }
  std::cout << (all_ok ? "ALL MATCH" : "MISMATCH FOUND") << "\n\n";

  std::cout << "The circle is complete:\n"
            << "  Array -> Cartesian Tree -> Euler Tour -> "
               "Sparse Table -> O(1) LCA -> O(1) RMQ\n"
            << "  Confirming: RMQ and LCA are equivalent problems.\n";

  (void)sink;
}


/**
 * @brief Executes the four example scenarios demonstrating Cartesian Tree, LCA, and RMQ.
 *
 * Runs scenario_1 through scenario_4 in sequence, prints a completion message, and exits.
 *
 * @return int `0` on successful completion.
 */
int main()
{
  scenario_1();
  scenario_2();
  scenario_3();
  scenario_4();

  std::cout << "\nAll scenarios completed successfully.\n";
  return 0;
}