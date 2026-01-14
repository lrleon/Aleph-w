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
 * @file hash_tables_example.C
 * @brief Hash table implementations in Aleph-w
 *
 * This example demonstrates different hash table implementations in Aleph-w
 * and their characteristics.
 *
 * ## Hash Set Types (for simple keys)
 *
 * ### DynSetLhash
 * - Separate chaining with linked lists
 * - **Pros**: Graceful degradation, simple deletion
 * - **Best for**: General purpose, frequent insertions/deletions
 *
 * ### DynSetLinHash  
 * - Linear hashing with incremental growth
 * - **Pros**: Predictable performance, no sudden slowdowns
 * - **Best for**: Real-time systems, growing datasets
 *
 * ### ODhashTable
 * - Open addressing with double hashing
 * - **Pros**: Cache-friendly, no pointer overhead
 * - **Best for**: Fixed size, high-performance requirements
 *
 * ## Complexity (Average Case)
 *
 * | Operation | Average | Worst Case |
 * |-----------|---------|------------|
 * | Insert    | O(1)    | O(n)       |
 * | Search    | O(1)    | O(n)       |
 * | Delete    | O(1)    | O(n)       |
 *
 * @see tpl_dynSetHash.H tpl_odhash.H
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>

#include <tpl_dynSetHash.H>
#include <tpl_odhash.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Example 1: DynSetLhash - Separate Chaining
// =============================================================================

void demo_dynset_lhash()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 1: DynSetLhash (Separate Chaining)                  ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "DynSetLhash uses linked lists to handle collisions.\n";
  cout << "Each bucket is a list of entries with the same hash.\n\n";

  DynSetLhash<int> set;

  // Insert values
  vector<int> values = {10, 20, 30, 40, 50, 60, 70, 80};

  cout << "Inserting values:\n";
  for (int v : values) {
    set.insert(v);
    cout << "  ✓ " << v << "\n";
  }

  cout << "\nSet size: " << set.size() << "\n";

  cout << "\n--- Membership testing ---\n\n";

  auto test_contains = [&](int key) {
    bool found = set.contains(key);
    cout << "  contains(" << key << "): " << (found ? "YES" : "NO") << "\n";
  };

  test_contains(30);
  test_contains(50);
  test_contains(100);
  test_contains(70);

  cout << "\n--- Removal ---\n\n";

  cout << "Removing 40...\n";
  set.remove(40);

  cout << "Removing 70...\n";
  set.remove(70);

  test_contains(40);
  test_contains(70);

  cout << "\nFinal size: " << set.size() << "\n";

  cout << "\n--- Iteration ---\n\n";
  cout << "All elements: ";
  set.for_each([](const int& v) { cout << v << " "; });
  cout << "\n";
}

// =============================================================================
// Example 2: DynSetLinHash - Linear Hashing
// =============================================================================

void demo_dynset_linhash()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 2: DynSetLinHash (Linear Hashing)                   ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Linear hashing grows the table incrementally, one bucket at a time.\n";
  cout << "This avoids expensive full-table rehashing operations.\n\n";

  DynSetLinHash<int> set;

  cout << "Inserting 100 random integers...\n\n";

  mt19937 rng(42);
  uniform_int_distribution<int> dist(1, 10000);

  for (int i = 0; i < 100; ++i)
    set.insert(dist(rng));

  cout << "Set size: " << set.size() << "\n";

  cout << "\n--- Membership testing ---\n\n";

  // Reset RNG and check first few values
  rng.seed(42);
  for (int i = 0; i < 5; ++i) {
    int v = dist(rng);
    cout << "  contains(" << v << "): " 
         << (set.contains(v) ? "YES" : "NO") << "\n";
  }

  cout << "  contains(99999): " 
       << (set.contains(99999) ? "YES" : "NO") << "\n";
}

// =============================================================================
// Example 3: ODhashTable - Open Addressing  
// =============================================================================

void demo_odhash()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 3: ODhashTable (Open Addressing)                    ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "ODhashTable stores all entries in a contiguous array.\n";
  cout << "Collisions are resolved by double hashing + linear probing.\n\n";

  // Create hash table with capacity 17 (prime for better distribution)
  ODhashTable<int> table(17);

  cout << "Table capacity: 17 (prime number recommended)\n\n";

  // Insert values
  vector<int> values = {10, 20, 30, 40, 50, 60, 70, 80};

  cout << "Inserting values:\n";
  for (int v : values) {
    int* result = table.insert(v);
    if (result)
      cout << "  ✓ Inserted: " << v << "\n";
    else
      cout << "  ✗ Failed (duplicate?): " << v << "\n";
  }

  cout << "\nTable statistics:\n";
  cout << "  Size: " << table.size() << "\n";
  cout << "  Capacity: " << table.capacity() << "\n";
  cout << "  Load factor: " << fixed << setprecision(2)
       << (double)table.size() / table.capacity() << "\n";

  cout << "\n--- Search operations ---\n\n";

  auto search_test = [&](int key) {
    int* found = table.search(key);
    cout << "  search(" << key << "): "
         << (found ? "FOUND" : "NOT FOUND") << "\n";
  };

  search_test(30);
  search_test(50);
  search_test(100);
  search_test(70);

  cout << "\n--- Remove operations ---\n\n";

  cout << "Removing 40...\n";
  table.remove(40);

  search_test(40);

  cout << "\nFinal size: " << table.size() << "\n";
}

// =============================================================================
// Example 4: Performance Comparison
// =============================================================================

void demo_performance()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 4: Performance Comparison                           ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  const int N = 50000;
  cout << "Benchmark: " << N << " insertions + " << N << " lookups\n\n";

  mt19937 rng(12345);
  vector<int> values;
  for (int i = 0; i < N; ++i)
    values.push_back(rng());

  // DynSetLhash benchmark
  {
    DynSetLhash<int> set;
    auto start = chrono::high_resolution_clock::now();
    
    for (int v : values)
      set.insert(v);
    
    auto mid = chrono::high_resolution_clock::now();
    
    size_t found = 0;
    for (int v : values)
      if (set.contains(v))
        ++found;
    
    auto end = chrono::high_resolution_clock::now();
    
    auto insert_ms = chrono::duration_cast<chrono::milliseconds>(mid - start).count();
    auto search_ms = chrono::duration_cast<chrono::milliseconds>(end - mid).count();
    
    cout << "DynSetLhash (chaining):\n";
    cout << "  Insert: " << insert_ms << " ms\n";
    cout << "  Search: " << search_ms << " ms (found " << found << ")\n";
    cout << "  Size:   " << set.size() << "\n\n";
  }

  // DynSetLinHash benchmark
  {
    DynSetLinHash<int> set;
    auto start = chrono::high_resolution_clock::now();
    
    for (int v : values)
      set.insert(v);
    
    auto mid = chrono::high_resolution_clock::now();
    
    size_t found = 0;
    for (int v : values)
      if (set.contains(v))
        ++found;
    
    auto end = chrono::high_resolution_clock::now();
    
    auto insert_ms = chrono::duration_cast<chrono::milliseconds>(mid - start).count();
    auto search_ms = chrono::duration_cast<chrono::milliseconds>(end - mid).count();
    
    cout << "DynSetLinHash (linear hashing):\n";
    cout << "  Insert: " << insert_ms << " ms\n";
    cout << "  Search: " << search_ms << " ms (found " << found << ")\n";
    cout << "  Size:   " << set.size() << "\n\n";
  }

  // ODhashTable benchmark
  {
    ODhashTable<int> table(N * 2);
    auto start = chrono::high_resolution_clock::now();
    
    for (int v : values)
      table.insert(v);
    
    auto mid = chrono::high_resolution_clock::now();
    
    size_t found = 0;
    for (int v : values)
      if (table.search(v))
        ++found;
    
    auto end = chrono::high_resolution_clock::now();
    
    auto insert_ms = chrono::duration_cast<chrono::milliseconds>(mid - start).count();
    auto search_ms = chrono::duration_cast<chrono::milliseconds>(end - mid).count();
    
    cout << "ODhashTable (open addressing):\n";
    cout << "  Insert: " << insert_ms << " ms\n";
    cout << "  Search: " << search_ms << " ms (found " << found << ")\n";
    cout << "  Size:   " << table.size() << "\n\n";
  }
}

// =============================================================================
// Main
// =============================================================================

int main()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║              Hash Tables in Aleph-w Library                      ║\n";
  cout << "║                                                                  ║\n";
  cout << "║     Aleph-w Library - https://github.com/lrleon/Aleph-w          ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n";

  demo_dynset_lhash();
  demo_dynset_linhash();
  demo_odhash();
  demo_performance();

  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║                     Hash Table Summary                           ║\n";
  cout << "╠══════════════════════════════════════════════════════════════════╣\n";
  cout << "║  DynSetLhash:   Separate chaining, flexible, easy deletion      ║\n";
  cout << "║  DynSetLinHash: Linear hashing, smooth growth, no spikes        ║\n";
  cout << "║  ODhashTable:   Open addressing, cache-friendly, fast           ║\n";
  cout << "║                                                                  ║\n";
  cout << "║  Choose based on your use case:                                  ║\n";
  cout << "║  • DynSetLhash: General purpose, many deletions                 ║\n";
  cout << "║  • DynSetLinHash: Realtime, predictable performance             ║\n";
  cout << "║  • ODhashTable: Known size, maximum speed                       ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  return 0;
}
