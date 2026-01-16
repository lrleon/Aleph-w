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
*/

/**
 * @file set_structures_benchmark.cc
 * @brief Comprehensive benchmark comparing all set data structures in Aleph-w
 *
 * This benchmark compares performance of different set implementations
 * available in Aleph-w. Understanding the performance characteristics
 * of each structure helps choose the right one for your use case.
 *
 * ## Why Benchmark?
 *
 * Different set structures have different performance characteristics:
 * - **Time complexity**: Varies by operation and structure
 * - **Space complexity**: Memory usage differs
 * - **Cache performance**: Some structures are more cache-friendly
 * - **Real-world performance**: Theoretical complexity doesn't tell the whole story
 *
 * ## Tree-Based Sets (O(log n) operations)
 *
 * ### AVL Tree
 * - **Balance**: Strictly balanced (height difference ≤ 1)
 * - **Operations**: O(log n) worst case
 * - **Best for**: Read-heavy workloads, predictable performance
 * - **Trade-off**: More rotations than Red-Black
 *
 * ### Red-Black Tree
 * - **Balance**: Relaxed balance (height ≤ 2 log(n+1))
 * - **Operations**: O(log n) worst case
 * - **Best for**: General-purpose, good all-around choice
 * - **Trade-off**: Fewer rotations than AVL
 *
 * ### Splay Tree
 * - **Balance**: Self-adjusting (no explicit balance)
 * - **Operations**: O(log n) amortized
 * - **Best for**: Temporal locality, caching patterns
 * - **Trade-off**: Worst case O(n), but adapts to access patterns
 *
 * ### Treap
 * - **Balance**: Randomized BST using heap priorities
 * - **Operations**: O(log n) expected
 * - **Best for**: Simple implementation, good average performance
 * - **Trade-off**: Randomized, less predictable
 *
 * ### Rand Tree
 * - **Balance**: Another randomized approach
 * - **Operations**: O(log n) expected
 * - **Best for**: Alternative randomized structure
 *
 * ## Skip Lists (Expected O(log n) operations)
 *
 * ### DynSkipList
 * - **Structure**: Probabilistic linked structure with multiple levels
 * - **Operations**: O(log n) expected
 * - **Best for**: Simplicity, concurrent extensions possible
 * - **Trade-off**: Probabilistic, not deterministic
 *
 * ## Hash Tables (Expected O(1) operations)
 *
 * ### DynSetLhash (Separate Chaining)
 * - **Collision resolution**: Linked lists per bucket
 * - **Operations**: O(1) average, O(n) worst case
 * - **Best for**: High load factors, many insertions/deletions
 * - **Trade-off**: Pointer overhead, cache misses
 *
 * ### DynSetLinHash (Linear Hashing)
 * - **Collision resolution**: Linear probing with incremental growth
 * - **Operations**: O(1) average
 * - **Best for**: Dynamic workloads with varying sizes
 * - **Trade-off**: More complex, predictable performance
 *
 * ### SetODhash (Open Addressing - Double Hashing)
 * - **Collision resolution**: Double hashing
 * - **Operations**: O(1) average
 * - **Best for**: Memory efficiency, cache-friendly access
 * - **Trade-off**: Fixed size or expensive resizing
 *
 * ### SetOLhash (Open Addressing - Linear Probing)
 * - **Collision resolution**: Linear probing
 * - **Operations**: O(1) average
 * - **Best for**: Simple, cache-friendly
 * - **Trade-off**: Clustering can degrade performance
 *
 * ## Performance Comparison
 *
 * | Structure | Insert | Search | Delete | Ordered? | Best For |
 * |-----------|--------|--------|--------|----------|----------|
 * | AVL Tree | O(log n) | O(log n) | O(log n) | Yes | Read-heavy |
 * | Red-Black | O(log n) | O(log n) | O(log n) | Yes | General |
 * | Splay | O(log n)* | O(log n)* | O(log n)* | Yes | Temporal locality |
 * | Treap | O(log n)** | O(log n)** | O(log n)** | Yes | Simple |
 * | Skip List | O(log n)** | O(log n)** | O(log n)** | Yes | Simplicity |
 * | Hash Tables | O(1)** | O(1)** | O(1)** | No | Maximum speed |
 *
 * ## When to Use What?
 *
 * | Structure | Best For | Avoid When |
 * |-----------|----------|------------|
 * | AVL/RB Tree | Ordered traversal, range queries, predictable O(log n) | Order not needed |
 * | Splay Tree | Temporal locality, caching patterns | Worst-case matters |
 * | Skip List | Simplicity, concurrent extensions | Deterministic needed |
 * | Hash Tables | Maximum speed, order doesn't matter | Ordered iteration needed |
 * | DynSetLhash | High load factors, many insertions/deletions | Memory critical |
 * | ODhash/OLhash | Memory efficiency, cache-friendly | Dynamic resizing needed |
 * | DynSetLinHash | Dynamic workloads, varying sizes | Fixed size preferred |
 *
 * ## Benchmark Metrics
 *
 * This benchmark measures:
 * - **Insertion time**: Time to insert elements
 * - **Search time**: Time to search for elements
 * - **Deletion time**: Time to delete elements
 * - **Memory usage**: Space overhead
 * - **Cache performance**: Cache misses/hits
 *
 * ## Usage
 *
 * ```bash
 * # Run benchmark
 * ./set_structures_benchmark
 *
 * # Configure benchmark size and seed
 * ./set_structures_benchmark --count 200000 --seed 123
 *
 * # Include ranked tree variants
 * ./set_structures_benchmark --ranked
 *
 * # Also run sequential insertion test (limited internally)
 * ./set_structures_benchmark --sequential
 *
 * # Show help
 * ./set_structures_benchmark --help
 * ```
 *
 * @see hash_tables_example.C Hash table implementations
 * @see dynmap_example.C Tree-based maps (similar to sets)
 * @see skiplist_example.C Skip list details
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <functional>
#include <cmath>
#include <set>
#include <unordered_set>

// Tree-based sets
#include <tpl_dynSetTree.H>

// Skip list
#include <tpl_dynSkipList.H>

// Hash-based sets
#include <tpl_dynSetHash.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Type Aliases for All Set Structures
// =============================================================================

// Tree-based sets (O(log n) worst case)
using AvlSet     = DynSetTree<int, Avl_Tree>;
using RbSet      = DynSetTree<int, Rb_Tree>;
using SplaySet   = DynSetTree<int, Splay_Tree>;
using TreapSet   = DynSetTree<int, Treap>;
using RandSet    = DynSetTree<int, Rand_Tree>;

// Ranked tree variants (with select/position operations)
using AvlRkSet   = DynSetTree<int, Avl_Tree_Rk>;
using TreapRkSet = DynSetTree<int, Treap_Rk>;

// Skip list (expected O(log n))
using SkipSet    = DynSkipList<int>;

// Hash-based sets (expected O(1))
using LhashSet   = DynSetLhash<int>;       // Separate chaining
using LinHashSet = DynSetLinHash<int>;     // Linear probing (dynamic)
using ODHashSet  = SetODhash<int>;         // Double hashing (open addr)
using OLHashSet  = SetOLhash<int>;         // Linear probing (open addr)

// =============================================================================
// Benchmark Infrastructure
// =============================================================================

struct BenchmarkResult
{
  string name;
  string category;
  double insert_ms;
  double search_ms;
  double remove_ms;
  double total_ms;
  size_t memory_hint;  // Rough estimate
};

// Timing utility
template <typename Func>
double measure_ms(Func&& f)
{
  auto start = chrono::high_resolution_clock::now();
  f();
  auto end = chrono::high_resolution_clock::now();
  return chrono::duration<double, milli>(end - start).count();
}

// Generic benchmark for tree-based sets
template <typename Set>
BenchmarkResult benchmark_tree_set(const string& name, const string& category,
                                   const vector<int>& data)
{
  BenchmarkResult result;
  result.name = name;
  result.category = category;

  Set set;

  // Insert
  result.insert_ms = measure_ms([&]() {
    for (int x : data)
      (void)set.insert(x);
  });

  // Search all
  result.search_ms = measure_ms([&]() {
    for (int x : data)
      {
        auto* p = set.search(x);
        if (p == nullptr)
          cerr << "ERROR: " << x << " not found in " << name << endl;
      }
  });

  // Remove all
  result.remove_ms = measure_ms([&]() {
    for (int x : data)
      set.remove(x);
  });

  result.total_ms = result.insert_ms + result.search_ms + result.remove_ms;
  result.memory_hint = data.size() * (sizeof(int) + 3 * sizeof(void*));  // Estimate

  return result;
}

// Generic benchmark for hash-based sets with proper interface
template <typename Set>
BenchmarkResult benchmark_hash_set(const string& name, const string& category,
                                   const vector<int>& data)
{
  BenchmarkResult result;
  result.name = name;
  result.category = category;

  Set set;

  // Insert
  result.insert_ms = measure_ms([&]() {
    for (int x : data)
      (void)set.insert(x);
  });

  // Search all using search() which returns pointer
  result.search_ms = measure_ms([&]() {
    for (int x : data)
      {
        auto* p = set.search(x);
        if (p == nullptr)
          cerr << "ERROR: " << x << " not found in " << name << endl;
      }
  });

  // Remove all
  result.remove_ms = measure_ms([&]() {
    for (int x : data)
      set.remove(x);
  });

  result.total_ms = result.insert_ms + result.search_ms + result.remove_ms;
  result.memory_hint = data.size() * sizeof(int) * 2;  // Rough estimate

  return result;
}

// Benchmark for skip list
BenchmarkResult benchmark_skip_list(const string& name, const string& category,
                                    const vector<int>& data)
{
  BenchmarkResult result;
  result.name = name;
  result.category = category;

  SkipSet set;

  // Insert
  result.insert_ms = measure_ms([&]() {
    for (int x : data)
      (void)set.insert(x);
  });

  // Search all
  result.search_ms = measure_ms([&]() {
    for (int x : data)
      {
        auto* p = set.search(x);
        if (p == nullptr)
          cerr << "ERROR: " << x << " not found in " << name << endl;
      }
  });

  // Remove all
  result.remove_ms = measure_ms([&]() {
    for (int x : data)
      set.remove(x);
  });

  result.total_ms = result.insert_ms + result.search_ms + result.remove_ms;
  result.memory_hint = data.size() * sizeof(int) * 4;  // Skip list has ~2 pointers per level avg

  return result;
}

// Benchmark for std::set (STL ordered set - Red-Black tree based)
BenchmarkResult benchmark_std_set(const string& name, const string& category,
                                  const vector<int>& data)
{
  BenchmarkResult result;
  result.name = name;
  result.category = category;

  std::set<int> s;

  // Insert
  result.insert_ms = measure_ms([&]() {
    for (int x : data)
      s.insert(x);
  });

  // Search all using find()
  result.search_ms = measure_ms([&]() {
    for (int x : data)
      {
        auto it = s.find(x);
        if (it == s.end())
          cerr << "ERROR: " << x << " not found in " << name << endl;
      }
  });

  // Remove all using erase()
  result.remove_ms = measure_ms([&]() {
    for (int x : data)
      s.erase(x);
  });

  result.total_ms = result.insert_ms + result.search_ms + result.remove_ms;
  result.memory_hint = data.size() * (sizeof(int) + 3 * sizeof(void*));

  return result;
}

// Benchmark for std::unordered_set (STL hash set)
BenchmarkResult benchmark_std_unordered_set(const string& name, const string& category,
                                            const vector<int>& data)
{
  BenchmarkResult result;
  result.name = name;
  result.category = category;

  std::unordered_set<int> s;

  // Insert
  result.insert_ms = measure_ms([&]() {
    for (int x : data)
      s.insert(x);
  });

  // Search all using find()
  result.search_ms = measure_ms([&]() {
    for (int x : data)
      {
        auto it = s.find(x);
        if (it == s.end())
          cerr << "ERROR: " << x << " not found in " << name << endl;
      }
  });

  // Remove all using erase()
  result.remove_ms = measure_ms([&]() {
    for (int x : data)
      s.erase(x);
  });

  result.total_ms = result.insert_ms + result.search_ms + result.remove_ms;
  result.memory_hint = data.size() * sizeof(int) * 2;

  return result;
}

// =============================================================================
// Data Generation
// =============================================================================

vector<int> generate_random_data(size_t n, unsigned seed)
{
  mt19937 rng(seed);
  uniform_int_distribution<int> dist(0, numeric_limits<int>::max());

  vector<int> data;
  data.reserve(n);
  for (size_t i = 0; i < n; ++i)
    data.push_back(dist(rng));

  // Remove duplicates (sets don't allow them)
  sort(data.begin(), data.end());
  data.erase(unique(data.begin(), data.end()), data.end());

  // Shuffle for random insertion order
  shuffle(data.begin(), data.end(), rng);

  return data;
}

vector<int> generate_sequential_data(size_t n)
{
  vector<int> data;
  data.reserve(n);
  for (size_t i = 0; i < n; ++i)
    data.push_back(static_cast<int>(i));
  return data;
}

// =============================================================================
// Results Display
// =============================================================================

void print_results_table(const vector<BenchmarkResult>& results, size_t n)
{
  cout << "\n";
  cout << "┌────────────────────┬─────────────┬────────────┬────────────┬────────────┬────────────┐\n";
  cout << "│ Structure          │ Category    │ Insert(ms) │ Search(ms) │ Remove(ms) │  Total(ms) │\n";
  cout << "├────────────────────┼─────────────┼────────────┼────────────┼────────────┼────────────┤\n";

  string last_category;
  for (const auto& r : results)
    {
      if (!last_category.empty() && r.category != last_category)
        cout << "├────────────────────┼─────────────┼────────────┼────────────┼────────────┼────────────┤\n";
      last_category = r.category;

      cout << "│ " << setw(18) << left << r.name
           << " │ " << setw(11) << r.category
           << " │ " << setw(10) << right << fixed << setprecision(2) << r.insert_ms
           << " │ " << setw(10) << r.search_ms
           << " │ " << setw(10) << r.remove_ms
           << " │ " << setw(10) << r.total_ms << " │\n";
    }

  cout << "└────────────────────┴─────────────┴────────────┴────────────┴────────────┴────────────┘\n";

  // Find best in each category
  cout << "\n▶ Best by Operation:\n";

  auto find_best = [&](auto getter, const string& op_name) {
    const BenchmarkResult* best = nullptr;
    for (const auto& r : results)
      if (!best || getter(r) < getter(*best))
        best = &r;
    if (best)
      cout << "  " << setw(10) << left << op_name << ": "
           << best->name << " (" << fixed << setprecision(2) << getter(*best) << " ms)\n";
  };

  find_best([](const auto& r) { return r.insert_ms; }, "Insert");
  find_best([](const auto& r) { return r.search_ms; }, "Search");
  find_best([](const auto& r) { return r.remove_ms; }, "Remove");
  find_best([](const auto& r) { return r.total_ms; }, "Overall");
}

void print_operations_per_second(const vector<BenchmarkResult>& results, size_t n)
{
  cout << "\n▶ Operations per Second (thousands):\n\n";
  cout << "  Structure          │  Insert K/s │  Search K/s │  Remove K/s\n";
  cout << "  ───────────────────┼─────────────┼─────────────┼────────────\n";

  for (const auto& r : results)
    {
      double insert_kops = (n / r.insert_ms);
      double search_kops = (n / r.search_ms);
      double remove_kops = (n / r.remove_ms);

      cout << "  " << setw(18) << left << r.name << " │ "
           << setw(11) << right << fixed << setprecision(0) << insert_kops << " │ "
           << setw(11) << search_kops << " │ "
           << setw(10) << remove_kops << "\n";
    }
}

// =============================================================================
// Main Benchmark
// =============================================================================

void run_full_benchmark(size_t n, unsigned seed, bool include_ranked)
{
  cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
  cout << "║              Set Data Structures Benchmark - Aleph-w                         ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Configuration:\n";
  cout << "  Elements: " << n << "\n";
  cout << "  Seed: " << seed << "\n";
  cout << "  Theoretical log₂(n) = " << fixed << setprecision(1) << log2(n) << "\n\n";

  cout << "Generating random data... " << flush;
  auto data = generate_random_data(n, seed);
  cout << "done (" << data.size() << " unique elements)\n\n";

  vector<BenchmarkResult> results;

  // ==========================================================================
  // Tree-based sets
  // ==========================================================================
  cout << "Running Tree benchmarks...\n";

  results.push_back(benchmark_tree_set<AvlSet>("AVL Tree", "Tree", data));
  cout << "  ✓ AVL Tree\n";

  results.push_back(benchmark_tree_set<RbSet>("Red-Black Tree", "Tree", data));
  cout << "  ✓ Red-Black Tree\n";

  results.push_back(benchmark_tree_set<SplaySet>("Splay Tree", "Tree", data));
  cout << "  ✓ Splay Tree\n";

  results.push_back(benchmark_tree_set<TreapSet>("Treap", "Tree", data));
  cout << "  ✓ Treap\n";

  results.push_back(benchmark_tree_set<RandSet>("Rand Tree", "Tree", data));
  cout << "  ✓ Rand Tree\n";

  if (include_ranked)
    {
      results.push_back(benchmark_tree_set<AvlRkSet>("AVL Tree Rk", "Tree+Rank", data));
      cout << "  ✓ AVL Tree Rk\n";

      results.push_back(benchmark_tree_set<TreapRkSet>("Treap Rk", "Tree+Rank", data));
      cout << "  ✓ Treap Rk\n";
    }

  // ==========================================================================
  // Skip List
  // ==========================================================================
  cout << "Running Skip List benchmark...\n";

  results.push_back(benchmark_skip_list("Skip List", "Skip List", data));
  cout << "  ✓ Skip List\n";

  // ==========================================================================
  // Hash-based sets
  // ==========================================================================
  cout << "Running Hash benchmarks...\n";

  results.push_back(benchmark_hash_set<LhashSet>("DynSetLhash", "Hash Chain", data));
  cout << "  ✓ DynSetLhash (chaining)\n";

  results.push_back(benchmark_hash_set<LinHashSet>("DynSetLinHash", "Hash Dyn", data));
  cout << "  ✓ DynSetLinHash (dynamic)\n";

  results.push_back(benchmark_hash_set<ODHashSet>("SetODhash", "Hash Open", data));
  cout << "  ✓ SetODhash (double hash)\n";

  results.push_back(benchmark_hash_set<OLHashSet>("SetOLhash", "Hash Open", data));
  cout << "  ✓ SetOLhash (linear probe)\n";

  // ==========================================================================
  // STL containers for comparison
  // ==========================================================================
  cout << "Running STL benchmarks...\n";

  results.push_back(benchmark_std_set("std::set", "STL Tree", data));
  cout << "  ✓ std::set (RB tree)\n";

  results.push_back(benchmark_std_unordered_set("std::unordered_set", "STL Hash", data));
  cout << "  ✓ std::unordered_set\n";

  // ==========================================================================
  // Display Results
  // ==========================================================================
  print_results_table(results, data.size());
  print_operations_per_second(results, data.size());

  // ==========================================================================
  // Summary
  // ==========================================================================
  cout << R"(
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Summary & Recommendations
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

┌───────────────────────────────────────────────────────────────────────────────┐
│ TREE-BASED (O(log n))                                                         │
│ ═══════════════════                                                           │
│ • AVL Tree:      Strictest balance, best for read-heavy, deterministic        │
│ • Red-Black:     Good all-around, used in std::set/map                        │
│ • Splay Tree:    Self-adjusting, great if same elements accessed often        │
│ • Treap/Rand:    Randomized, simpler code, good average case                  │
│ • Use when:      Need ordered traversal, range queries, or worst-case O(log n)│
├───────────────────────────────────────────────────────────────────────────────┤
│ SKIP LIST (Expected O(log n))                                                 │
│ ═════════════════════════════                                                 │
│ • Simple probabilistic structure, no rotations needed                         │
│ • Easy to make concurrent (though not implemented here)                       │
│ • Use when:      Want simplicity, or planning concurrent extension            │
├───────────────────────────────────────────────────────────────────────────────┤
│ HASH TABLES (Expected O(1))                                                   │
│ ═══════════════════════════                                                   │
│ • DynSetLhash:   Separate chaining - handles high load gracefully             │
│ • DynSetLinHash: Linear probing with expansion - good for varying sizes       │
│ • SetODhash:     Double hashing - minimal clustering, cache-friendly          │
│ • SetOLhash:     Linear probing - best cache locality, simple                 │
│ • Use when:      Speed is critical and order doesn't matter                   │
│                                                                               │
│ Hash Table Selection Guide:                                                   │
│ • High insert/delete rate → DynSetLhash (chaining handles it well)            │
│ • Memory efficiency → SetODhash/SetOLhash (no pointers per element)           │
│ • Unknown final size → DynSetLinHash (expands automatically)                  │
│ • Fixed size known → SetODhash (set size at construction)                     │
└───────────────────────────────────────────────────────────────────────────────┘
)";
}

// =============================================================================
// Sequential Access Pattern Test
// =============================================================================

void run_sequential_test(size_t n)
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Sequential Access Pattern (tests worst-case for some structures)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "Inserting " << n << " elements in sorted order...\n";
  auto data = generate_sequential_data(n);

  vector<BenchmarkResult> results;

  // Trees should handle this well (self-balancing)
  results.push_back(benchmark_tree_set<AvlSet>("AVL Tree", "Tree", data));
  results.push_back(benchmark_tree_set<RbSet>("Red-Black", "Tree", data));

  // Skip list - probabilistic, should be OK
  results.push_back(benchmark_skip_list("Skip List", "Skip", data));

  // Hash tables - should be fine
  results.push_back(benchmark_hash_set<LhashSet>("DynSetLhash", "Hash", data));
  results.push_back(benchmark_hash_set<ODHashSet>("SetODhash", "Hash", data));

  // STL containers
  results.push_back(benchmark_std_set("std::set", "STL Tree", data));
  results.push_back(benchmark_std_unordered_set("std::unordered_set", "STL Hash", data));

  cout << "\nSequential insertion results:\n";
  print_results_table(results, data.size());

  cout << "\nNote: Splay tree would be O(n²) for sequential insertion without\n";
  cout << "subsequent accesses, as it doesn't rebalance until accessed.\n";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  // Default parameters
  size_t n = 1000000; // 1,000,000 keys by default
  unsigned seed = 42;
  bool include_ranked = false;
  bool run_sequential = false;

  // Simple argument parsing (no TCLAP dependency)
  for (int i = 1; i < argc; ++i)
    {
      string arg = argv[i];
      if ((arg == "-n" || arg == "--count") && i + 1 < argc)
        n = stoull(argv[++i]);
      else if ((arg == "-s" || arg == "--seed") && i + 1 < argc)
        seed = stoul(argv[++i]);
      else if (arg == "-r" || arg == "--ranked")
        include_ranked = true;
      else if (arg == "-q" || arg == "--sequential")
        run_sequential = true;
      else if (arg == "-h" || arg == "--help")
        {
          cout << "Set Data Structures Benchmark\n\n"
               << "Usage: " << argv[0] << " [options]\n\n"
               << "Options:\n"
               << "  -n, --count N      Number of elements (default: 1000000)\n"
               << "  -s, --seed S       Random seed (default: 42)\n"
               << "  -r, --ranked       Include ranked tree variants\n"
               << "  -q, --sequential   Also run sequential insertion test\n"
               << "  -h, --help         Show this help\n";
          return 0;
        }
    }

  // Run main benchmark
  run_full_benchmark(n, seed, include_ranked);

  // Optional sequential test
  if (run_sequential)
    run_sequential_test(min(n, size_t(50000)));  // Limit sequential test size

  cout << "\nDone.\n";
  return 0;
}
