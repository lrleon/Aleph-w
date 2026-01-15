/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
 * @author Leandro Rabindranath León
 * @ingroup Examples
*/

/**
 * @file dynset_trees.C
 * @brief Demonstration of DynSetTree with different BST implementations
 * 
 * This example showcases one of Aleph-w's most powerful features: the ability
 * to swap underlying data structure implementations through template parameters
 * without changing your code. This demonstrates the power of generic programming
 * and the Strategy pattern at compile time.
 *
 * ## The DynSetTree Abstraction
 *
 * `DynSetTree<Key, Tree, Compare>` is a generic dynamic set wrapper that provides
 * a uniform interface regardless of the underlying tree implementation. You can
 * switch between different BST implementations by simply changing a template
 * parameter, allowing you to:
 *
 * - **Experiment**: Try different trees to find the best fit
 * - **Optimize**: Choose the tree that matches your access patterns
 * - **Learn**: Compare implementations side-by-side
 * - **Maintain**: Change implementation without rewriting code
 *
 * ## Available Tree Implementations
 *
 * ### Balanced Trees (Guaranteed O(log n))
 *
 * #### Avl_Tree / Avl_Tree_Rk
 * - **Balance**: Strict height balance (heights differ by ≤ 1)
 * - **Operations**: O(log n) guaranteed
 * - **Best for**: Read-heavy workloads, predictable performance
 * - **Trade-off**: More rotations than Red-Black (slightly slower inserts)
 *
 * #### Rb_Tree / Rb_Tree_Rk
 * - **Balance**: Relaxed (no path > 2× shortest path)
 * - **Operations**: O(log n) guaranteed
 * - **Best for**: General-purpose, balanced read/write
 * - **Trade-off**: Less strict balance than AVL (faster inserts)
 *
 * ### Self-Adjusting Trees (Amortized O(log n))
 *
 * #### Splay_Tree / Splay_Tree_Rk
 * - **Strategy**: Moves accessed elements to root
 * - **Operations**: O(log n) amortized
 * - **Best for**: Temporal locality, caching patterns
 * - **Trade-off**: No worst-case guarantee, but excellent for hot data
 *
 * ### Randomized Trees (Expected O(log n))
 *
 * #### Treap / Treap_Rk
 * - **Strategy**: Randomized BST with heap priorities
 * - **Operations**: O(log n) expected
 * - **Best for**: Simple implementation, good average case
 * - **Trade-off**: Probabilistic, no worst-case guarantee
 *
 * #### Rand_Tree
 * - **Strategy**: Different randomization approach
 * - **Operations**: O(log n) expected
 * - **Best for**: Alternative randomized approach
 *
 * ## Rank Support
 *
 * Trees with `_Rk` suffix support order statistics:
 * - **select(k)**: Find k-th smallest element in O(log n)
 * - **rank(x)**: Find position of element x in O(log n)
 * - **Trade-off**: Slightly slower operations, more memory
 *
 * ## What This Example Demonstrates
 *
 * 1. **Declaration**: How to create DynSetTree with different tree types
 * 2. **Operations**: Insert, search, remove, iteration (same API for all)
 * 3. **Performance**: Timing comparison across implementations
 * 4. **Functional**: Using map, filter, fold operations
 *
 * ## Performance Comparison
 *
 * | Tree Type | Insert | Search | Delete | Best Use Case |
 * |-----------|--------|--------|--------|---------------|
 * | AVL | O(log n) | O(log n) | O(log n) | Read-heavy |
 * | Red-Black | O(log n) | O(log n) | O(log n) | General purpose |
 * | Splay | O(log n) am. | O(log n) am. | O(log n) am. | Temporal locality |
 * | Treap | O(log n) exp. | O(log n) exp. | O(log n) exp. | Simple, average case |
 *
 * ## Usage Examples
 *
 * ```bash
 * # Compare all tree types with 10000 elements
 * dynset_trees -n 10000 -a
 *
 * # Compare AVL vs Red-Black with verbose output
 * dynset_trees -n 50000 -s 42 -v
 *
 * # Quick test with 1000 elements
 * dynset_trees -n 1000
 * ```
 *
 * ## Code Example
 *
 * ```cpp
 * // Same interface, different implementations!
 * DynSetTree<int, Avl_Tree> avl_set;
 * DynSetTree<int, Rb_Tree> rb_set;
 * DynSetTree<int, Splay_Tree> splay_set;
 *
 * // All support the same operations
 * avl_set.insert(42);
 * rb_set.insert(42);
 * splay_set.insert(42);
 * ```
 *
 * @see timeAllTree.C Detailed performance benchmark
 * @see tpl_dynSetTree.H DynSetTree template class
 * @see tpl_avl.H AVL tree implementation
 * @see tpl_rb_tree.H Red-Black tree implementation
 * @see tpl_splay_tree.H Splay tree implementation
 * @see tpl_treap.H Treap implementation
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <iostream>
# include <iomanip>
# include <chrono>
# include <vector>
# include <algorithm>
# include <tclap/CmdLine.h>
# include <tpl_dynSetTree.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Type aliases for different tree implementations
// =============================================================================

// Standard trees (no rank support)
using AvlSet     = DynSetTree<int, Avl_Tree>;
using RbSet      = DynSetTree<int, Rb_Tree>;
using SplaySet   = DynSetTree<int, Splay_Tree>;
using TreapSet   = DynSetTree<int, Treap>;
using RandSet    = DynSetTree<int, Rand_Tree>;

// Ranked versions (support select(i) and position(x) operations)
// These trees maintain subtree sizes for O(log n) positional access
using AvlRkSet     = DynSetTree<int, Avl_Tree_Rk>;
using TreapRkSet   = DynSetTree<int, Treap_Rk>;

// Note: Other ranked trees exist (Rb_Tree_Rk, Splay_Tree_Rk) but have
// interface differences that may cause issues with DynSetTree wrapper

// =============================================================================
// Timing utilities
// =============================================================================

struct TimingResult
{
  string name;
  double insert_ms;
  double search_ms;
  double remove_ms;
  size_t height;
  size_t internal_path_length;
};

template <typename Set>
TimingResult benchmark_set(const string& name, const vector<int>& data, 
                           bool verbose)
{
  TimingResult result;
  result.name = name;
  
  Set set;
  
  // Benchmark insertions
  auto start = chrono::high_resolution_clock::now();
  for (int x : data)
    set.insert(x);
  auto end = chrono::high_resolution_clock::now();
  result.insert_ms = chrono::duration<double, milli>(end - start).count();
  
  // Get tree statistics
  result.height = computeHeightRec(set.get_root_node());
  result.internal_path_length = internal_path_length(set.get_root_node());
  
  // Benchmark searches (search all elements)
  start = chrono::high_resolution_clock::now();
  for (int x : data)
    {
      auto* p = set.search(x);
      if (p == nullptr)
        cerr << "ERROR: element " << x << " not found!" << endl;
    }
  end = chrono::high_resolution_clock::now();
  result.search_ms = chrono::duration<double, milli>(end - start).count();
  
  // Benchmark removals
  start = chrono::high_resolution_clock::now();
  for (int x : data)
    set.remove(x);
  end = chrono::high_resolution_clock::now();
  result.remove_ms = chrono::duration<double, milli>(end - start).count();
  
  if (set.size() != 0)
    cerr << "ERROR: set not empty after removals!" << endl;
  
  if (verbose)
    {
      cout << "  " << name << ":" << endl;
      cout << "    Height: " << result.height << endl;
      cout << "    Internal path length: " << result.internal_path_length << endl;
      cout << "    Avg path length: " << fixed << setprecision(2)
           << (double)result.internal_path_length / data.size() << endl;
    }
  
  return result;
}

// =============================================================================
// Demonstration of basic operations
// =============================================================================

void demonstrate_basic_operations()
{
  cout << "=== Basic Operations Demo ===" << endl << endl;
  
  // Using AVL tree as backend (the default)
  DynSetTree<int> avl_set;
  
  // Insert elements
  cout << "Inserting elements: 5, 3, 7, 1, 4, 6, 9" << endl;
  for (int x : {5, 3, 7, 1, 4, 6, 9})
    avl_set.insert(x);
  
  cout << "Set size: " << avl_set.size() << endl;
  cout << "Elements (in order): ";
  avl_set.for_each([](int x) { cout << x << " "; });
  cout << endl;
  
  // Search
  cout << endl << "Searching for 4: " 
       << (avl_set.search(4) ? "found" : "not found") << endl;
  cout << "Searching for 8: " 
       << (avl_set.search(8) ? "found" : "not found") << endl;
  
  // Contains
  cout << endl << "Contains 7: " << (avl_set.contains(7) ? "yes" : "no") << endl;
  cout << "Contains 10: " << (avl_set.contains(10) ? "yes" : "no") << endl;
  
  // Min/Max
  cout << endl << "Minimum: " << avl_set.min() << endl;
  cout << "Maximum: " << avl_set.max() << endl;
  
  // Remove
  cout << endl << "Removing 3..." << endl;
  avl_set.remove(3);
  cout << "Elements after removal: ";
  avl_set.for_each([](int x) { cout << x << " "; });
  cout << endl;
  
  cout << endl;
}

// =============================================================================
// Demonstration of different tree types
// =============================================================================

void demonstrate_tree_types()
{
  cout << "=== Different Tree Types Demo ===" << endl << endl;
  
  // Create sets with different backends
  AvlSet   avl;
  RbSet    rb;
  SplaySet splay;
  TreapSet treap;
  
  vector<int> data = {50, 25, 75, 10, 30, 60, 90, 5, 15, 27, 35};
  
  cout << "Inserting same data into different tree types:" << endl;
  cout << "Data: ";
  for (int x : data) cout << x << " ";
  cout << endl << endl;
  
  for (int x : data)
    {
      avl.insert(x);
      rb.insert(x);
      splay.insert(x);
      treap.insert(x);
    }
  
  cout << "Tree heights (lower is better balanced):" << endl;
  cout << "  AVL:    " << computeHeightRec(avl.get_root_node()) << endl;
  cout << "  RB:     " << computeHeightRec(rb.get_root_node()) << endl;
  cout << "  Splay:  " << computeHeightRec(splay.get_root_node()) << endl;
  cout << "  Treap:  " << computeHeightRec(treap.get_root_node()) << endl;
  
  // Note: Splay tree changes structure on every access
  cout << endl << "After searching for 5, 10, 15 (watch Splay change):" << endl;
  splay.search(5);
  splay.search(10);
  splay.search(15);
  cout << "  Splay height after searches: " 
       << computeHeightRec(splay.get_root_node()) << endl;
  cout << "  (Splay moves accessed elements toward root)" << endl;
  
  cout << endl;
}

// =============================================================================
// Demonstration of ranked operations
// =============================================================================

void demonstrate_ranked_operations()
{
  cout << "=== Ranked Operations Demo ===" << endl << endl;
  
  cout << "Ranked trees maintain subtree sizes, enabling O(log n) operations:" << endl;
  cout << "  select(i)   - get i-th smallest element (0-indexed)" << endl;
  cout << "  position(x) - get rank/position of element x" << endl << endl;
  
  cout << "Available ranked tree types:" << endl;
  cout << "  - Avl_Tree_Rk : AVL with rank (strictly balanced, deterministic)" << endl;
  cout << "  - Treap_Rk    : Treap with rank (randomized balance)" << endl << endl;
  
  // Demonstrate with ranked tree types
  AvlRkSet   avl_rk;
  TreapRkSet treap_rk;
  
  vector<int> data = {100, 50, 150, 25, 75, 125, 175, 10, 200};
  
  for (int x : data)
    {
      avl_rk.insert(x);
      treap_rk.insert(x);
    }
  
  cout << "Set contents (sorted): ";
  avl_rk.for_each([](int x) { cout << x << " "; });
  cout << endl << endl;
  
  // Show select operation
  cout << "Positional access - select(i) returns i-th element:" << endl;
  cout << "  Index  AVL_Rk  Treap_Rk" << endl;
  cout << "  -----  ------  --------" << endl;
  for (size_t i = 0; i < avl_rk.size(); ++i)
    {
      cout << "    " << i << "     "
           << setw(4) << avl_rk.select(i) << "      "
           << setw(4) << treap_rk.select(i) << endl;
    }
  
  // Show position operation
  cout << endl << "Element ranks - position(x) returns index of x:" << endl;
  cout << "  Value  AVL_Rk  Treap_Rk" << endl;
  cout << "  -----  ------  --------" << endl;
  for (int x : {10, 50, 100, 150, 200})
    {
      cout << "   " << setw(3) << x << "      "
           << avl_rk.position(x) << "         "
           << treap_rk.position(x) << endl;
    }
  
  // Demonstrate practical use: find median
  cout << endl << "Practical use - Finding median in O(log n):" << endl;
  size_t mid = avl_rk.size() / 2;
  cout << "  Median (middle element): " << avl_rk.select(mid) << endl;
  
  // Find k-th percentile
  size_t p25 = avl_rk.size() / 4;
  size_t p75 = 3 * avl_rk.size() / 4;
  cout << "  25th percentile: " << avl_rk.select(p25) << endl;
  cout << "  75th percentile: " << avl_rk.select(p75) << endl;
  
  // Count elements less than a value
  cout << endl << "Count elements < 100: " << avl_rk.position(100) << endl;
  
  cout << endl;
}

// =============================================================================
// Demonstration of functional programming features
// =============================================================================

void demonstrate_functional_features()
{
  cout << "=== Functional Programming Features ===" << endl << endl;
  
  DynSetTree<int> set;
  for (int i = 1; i <= 10; ++i)
    set.insert(i);
  
  cout << "Original set: ";
  set.for_each([](int x) { cout << x << " "; });
  cout << endl << endl;
  
  // Filter: keep only even numbers
  auto evens = set.filter([](int x) { return x % 2 == 0; });
  cout << "Filter (even): ";
  evens.for_each([](int x) { cout << x << " "; });
  cout << endl;
  
  // Map: square each element (returns DynList)
  auto squares = set.template maps<int>([](int x) { return x * x; });
  cout << "Map (square): ";
  squares.for_each([](int x) { cout << x << " "; });
  cout << endl;
  
  // Fold: sum all elements
  int sum = set.template foldl<int>(0, [](int acc, int x) { return acc + x; });
  cout << "Fold (sum): " << sum << endl;
  
  // All/Exists predicates
  cout << endl << "Predicates:" << endl;
  cout << "  All positive? " << (set.all([](int x) { return x > 0; }) ? "yes" : "no") << endl;
  cout << "  Exists > 5? " << (set.exists([](int x) { return x > 5; }) ? "yes" : "no") << endl;
  cout << "  All <= 10? " << (set.all([](int x) { return x <= 10; }) ? "yes" : "no") << endl;
  
  cout << endl;
}

// =============================================================================
// Performance comparison
// =============================================================================

void run_performance_comparison(size_t n, unsigned int seed, bool verbose)
{
  cout << "=== Performance Comparison ===" << endl;
  cout << "Testing with " << n << " elements (seed: " << seed << ")" << endl << endl;
  
  // Generate random data
  srand(seed);
  vector<int> data;
  data.reserve(n);
  for (size_t i = 0; i < n; ++i)
    data.push_back(rand());
  
  // Remove duplicates (sets don't allow them)
  sort(data.begin(), data.end());
  data.erase(unique(data.begin(), data.end()), data.end());
  
  // Shuffle for random insertion order
  for (size_t i = data.size() - 1; i > 0; --i)
    swap(data[i], data[rand() % (i + 1)]);
  
  cout << "Actual unique elements: " << data.size() << endl << endl;
  
  // Run benchmarks - Standard trees
  vector<TimingResult> results;
  
  cout << "Standard trees (no rank support):" << endl;
  results.push_back(benchmark_set<AvlSet>("AVL Tree", data, verbose));
  results.push_back(benchmark_set<RbSet>("Red-Black Tree", data, verbose));
  results.push_back(benchmark_set<SplaySet>("Splay Tree", data, verbose));
  results.push_back(benchmark_set<TreapSet>("Treap", data, verbose));
  results.push_back(benchmark_set<RandSet>("Rand Tree", data, verbose));
  
  cout << endl << "Ranked trees (with select/position support):" << endl;
  results.push_back(benchmark_set<AvlRkSet>("AVL_Rk", data, verbose));
  results.push_back(benchmark_set<TreapRkSet>("Treap_Rk", data, verbose));
  
  // Print results table
  cout << endl;
  cout << left << setw(18) << "Tree Type"
       << right << setw(12) << "Insert(ms)"
       << setw(12) << "Search(ms)"
       << setw(12) << "Remove(ms)"
       << setw(10) << "Height"
       << setw(15) << "Avg Path" << endl;
  cout << string(79, '-') << endl;
  
  for (const auto& r : results)
    {
      cout << left << setw(18) << r.name
           << right << fixed << setprecision(2)
           << setw(12) << r.insert_ms
           << setw(12) << r.search_ms
           << setw(12) << r.remove_ms
           << setw(10) << r.height
           << setw(15) << setprecision(2) 
           << (double)r.internal_path_length / data.size() << endl;
    }
  
  cout << endl;
  cout << "Notes:" << endl;
  cout << "  - Height: tree height (log2(" << data.size() << ") ~= " 
       << fixed << setprecision(1) << log2(data.size()) << ")" << endl;
  cout << "  - Avg Path: average path length from root (ideal ~= log2(n))" << endl;
  cout << "  - Splay tree optimizes for access patterns, not balance" << endl;
  cout << "  - _Rk variants have slight overhead for maintaining subtree sizes" << endl;
  cout << "  - Use _Rk trees when you need select(i) or position(x) operations" << endl;
  cout << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstration of DynSetTree with different BST implementations.\n"
        "Shows how Aleph-w allows swapping data structure implementations\n"
        "through template parameters.",
        ' ', "1.0");

      TCLAP::ValueArg<size_t> nArg("n", "count",
                                    "Number of elements for performance test",
                                    false, 100000, "size_t");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      TCLAP::SwitchArg allArg("a", "all",
                               "Run all demonstrations (not just performance)",
                               false);
      cmd.add(allArg);

      TCLAP::SwitchArg verboseArg("v", "verbose",
                                   "Show detailed tree statistics",
                                   false);
      cmd.add(verboseArg);

      cmd.parse(argc, argv);

      size_t n = nArg.getValue();
      unsigned int seed = seedArg.getValue();
      bool runAll = allArg.getValue();
      bool verbose = verboseArg.getValue();

      if (seed == 0)
        seed = time(nullptr);

      cout << "DynSetTree - Multiple BST Implementations Demo" << endl;
      cout << "==============================================" << endl << endl;

      if (runAll)
        {
          demonstrate_basic_operations();
          demonstrate_tree_types();
          demonstrate_ranked_operations();
          demonstrate_functional_features();
        }

      run_performance_comparison(n, seed, verbose);

      cout << "Done." << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}

