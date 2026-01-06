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
 * @file skiplist_example.C
 * @brief Skip List: A probabilistic alternative to balanced trees
 * 
 * This example demonstrates Skip Lists, a randomized data structure
 * invented by William Pugh (1990) that provides expected O(log n)
 * operations without the complexity of balanced tree rotations.
 * 
 * ## How Skip Lists Work
 * 
 * A Skip List is a layered linked list where:
 * - Level 0 contains all elements in sorted order
 * - Each higher level contains a random subset (~50%) of the level below
 * - "Express lanes" at higher levels allow fast traversal
 * 
 * Example structure (4 elements: 3, 6, 7, 9):
 * 
 *     Level 2:  HEAD -----------------> 6 -----------------> NIL
 *     Level 1:  HEAD --------> 3 -----> 6 --------> 9 -----> NIL
 *     Level 0:  HEAD -> 3 -> 6 -> 7 -> 9 -> NIL
 * 
 * ## Advantages Over Balanced Trees
 * 
 * - Simpler implementation (no rotations)
 * - Lock-free concurrent versions are easier to implement
 * - Good cache locality for sequential access
 * - Same expected O(log n) performance
 * 
 * ## Applications
 * 
 * - Redis sorted sets (ZSET)
 * - LevelDB/RocksDB memtables
 * - Concurrent data structures
 * - Range queries
 * 
 * @see tpl_dynSkipList.H for the implementation
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 * @see Pugh, W. "Skip Lists: A Probabilistic Alternative to Balanced Trees" (1990)
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <random>
#include <vector>
#include <set>
#include <tpl_dynSkipList.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

/**
 * @brief Demonstrate basic Skip List operations
 * 
 * DynSkipList is an ordered set (not a map). Each element is stored
 * and accessed by its own value.
 */
void demonstrate_basic_operations()
{
  cout << "\n=== Basic Skip List Operations ===" << endl;
  
  // Create a skip list set of integers
  DynSkipList<int> skiplist;
  
  // Insert elements
  cout << "\nInserting elements..." << endl;
  vector<int> data = {42, 17, 99, 23, 8, 64, 31, 55, 100, 5};
  
  for (int val : data)
  {
    skiplist.insert(val);
    cout << "  Inserted: " << val << endl;
  }
  
  cout << "\nSkip list size: " << skiplist.size() << endl;
  
  // Search operations
  cout << "\n--- Search Operations ---" << endl;
  
  vector<int> search_keys = {23, 100, 42, 0, 50};
  for (int key : search_keys)
  {
    auto* found = skiplist.search(key);
    cout << "  search(" << key << "): ";
    if (found)
      cout << "Found!" << endl;
    else
      cout << "Not found" << endl;
  }
  
  // has() convenience method
  cout << "\nUsing has() method:" << endl;
  cout << "  has(42): " << (skiplist.has(42) ? "yes" : "no") << endl;
  cout << "  has(50): " << (skiplist.has(50) ? "yes" : "no") << endl;
  
  // Traversal (sorted order)
  cout << "\n--- Sorted Traversal ---" << endl;
  cout << "Elements in ascending order:" << endl;
  cout << "  ";
  
  for (auto it = skiplist.get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;
  
  // Removal
  cout << "\n--- Removal Operations ---" << endl;
  
  cout << "Removing 23..." << endl;
  size_t removed_count = skiplist.remove(23);
  cout << "  Removed " << removed_count << " element(s)" << endl;
  
  cout << "Trying to remove non-existent 1000..." << endl;
  removed_count = skiplist.remove(1000);
  cout << "  Removed " << removed_count << " element(s)" << endl;
  
  cout << "\nFinal size: " << skiplist.size() << endl;
  cout << "Final elements: ";
  for (auto it = skiplist.get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;
}

/**
 * @brief Demonstrate skip list with strings
 */
void demonstrate_string_set()
{
  cout << "\n=== Skip List with Strings ===" << endl;
  
  DynSkipList<string> words;
  
  // Add words
  vector<string> vocabulary = {
    "algorithm", "binary", "complexity", "data", "efficient",
    "function", "graph", "hash", "index", "join"
  };
  
  cout << "Building vocabulary..." << endl;
  for (const auto& word : vocabulary)
    words.insert(word);
  
  // Display in sorted (alphabetical) order
  cout << "\nWords in alphabetical order:" << endl;
  for (auto it = words.get_it(); it.has_curr(); it.next())
    cout << "  " << it.get_curr() << endl;
  
  // Search for words
  cout << "\nSearching:" << endl;
  cout << "  'complexity' exists: " << (words.has("complexity") ? "yes" : "no") << endl;
  cout << "  'hello' exists:      " << (words.has("hello") ? "yes" : "no") << endl;
}

/**
 * @brief Demonstrate functional programming features
 */
void demonstrate_functional()
{
  cout << "\n=== Functional Programming with Skip Lists ===" << endl;
  
  DynSkipList<int> numbers;
  for (int i = 1; i <= 20; i++)
    numbers.insert(i * 3);  // Multiples of 3: 3, 6, 9, ..., 60
  
  cout << "Numbers (multiples of 3 up to 60):" << endl;
  cout << "  ";
  numbers.for_each([](int n) { cout << n << " "; });
  cout << endl;
  
  // All / Exists predicates
  cout << "\nPredicates:" << endl;
  cout << "  all > 0:   " << (numbers.all([](int n) { return n > 0; }) ? "true" : "false") << endl;
  cout << "  exists 30: " << (numbers.exists([](int n) { return n == 30; }) ? "true" : "false") << endl;
  cout << "  exists 31: " << (numbers.exists([](int n) { return n == 31; }) ? "true" : "false") << endl;
  
  // Fold
  int sum = numbers.template foldl<int>(0, [](int acc, int n) { return acc + n; });
  cout << "\nSum of all elements: " << sum << endl;
  cout << "Expected sum (3+6+...+60): " << (3 * 20 * 21 / 2) << endl;
}

/**
 * @brief Benchmark skip list vs std::set
 */
void benchmark_comparison(size_t n, unsigned seed, bool verbose)
{
  cout << "\n=== Performance Benchmark ===" << endl;
  cout << "Comparing DynSkipList vs std::set with " << n << " elements" << endl;
  
  // Generate random keys
  mt19937 rng(seed);
  uniform_int_distribution<int> dist(0, static_cast<int>(n) * 10);
  
  vector<int> keys(n);
  for (size_t i = 0; i < n; i++)
    keys[i] = dist(rng);
  
  // Search keys (mix of existing and non-existing)
  vector<int> search_keys(n / 10);
  for (size_t i = 0; i < search_keys.size(); i++)
    search_keys[i] = (i % 2 == 0) ? keys[i * 10 % n] : dist(rng);
  
  // Benchmark DynSkipList
  DynSkipList<int> skiplist;
  
  auto start = chrono::high_resolution_clock::now();
  for (int key : keys)
    skiplist.insert(key);
  auto end = chrono::high_resolution_clock::now();
  double skiplist_insert = chrono::duration<double, milli>(end - start).count();
  
  start = chrono::high_resolution_clock::now();
  int skiplist_found = 0;
  for (int key : search_keys)
    if (skiplist.has(key)) skiplist_found++;
  end = chrono::high_resolution_clock::now();
  double skiplist_search = chrono::duration<double, milli>(end - start).count();
  
  // Benchmark std::set
  set<int> stdset;
  
  start = chrono::high_resolution_clock::now();
  for (int key : keys)
    stdset.insert(key);
  end = chrono::high_resolution_clock::now();
  double stdset_insert = chrono::duration<double, milli>(end - start).count();
  
  start = chrono::high_resolution_clock::now();
  int stdset_found = 0;
  for (int key : search_keys)
    if (stdset.find(key) != stdset.end()) stdset_found++;
  end = chrono::high_resolution_clock::now();
  double stdset_search = chrono::duration<double, milli>(end - start).count();
  
  // Results
  cout << "\n" << setw(15) << "Operation" 
       << setw(18) << "DynSkipList (ms)"
       << setw(18) << "std::set (ms)"
       << setw(12) << "Ratio" << endl;
  cout << string(63, '-') << endl;
  
  cout << setw(15) << "Insert " << n
       << setw(18) << fixed << setprecision(3) << skiplist_insert
       << setw(18) << stdset_insert
       << setw(12) << setprecision(2) << skiplist_insert / stdset_insert << "x" << endl;
  
  cout << setw(15) << "Search " << search_keys.size()
       << setw(18) << fixed << setprecision(3) << skiplist_search
       << setw(18) << stdset_search
       << setw(12) << setprecision(2) << skiplist_search / stdset_search << "x" << endl;
  
  if (verbose)
  {
    cout << "\nDynSkipList found: " << skiplist_found << "/" << search_keys.size() << endl;
    cout << "std::set found:    " << stdset_found << "/" << search_keys.size() << endl;
    cout << "DynSkipList size:  " << skiplist.size() << " (unique elements)" << endl;
    cout << "std::set size:     " << stdset.size() << " (unique elements)" << endl;
  }
  
  cout << "\nNote: Skip Lists trade some raw performance for:" << endl;
  cout << "  - Simpler implementation (no rotations)" << endl;
  cout << "  - Easier concurrent access" << endl;
  cout << "  - Good cache locality for range queries" << endl;
}

/**
 * @brief Visualize skip list structure (small example)
 */
void visualize_structure()
{
  cout << "\n=== Skip List Structure Visualization ===" << endl;
  cout << "\nConceptual view of a skip list with keys 3, 6, 7, 9, 12, 17, 19, 21:" << endl;
  cout << endl;
  cout << "  Level 3:  HEAD ---------------------------------> 12 ---------------------------------> NIL" << endl;
  cout << "  Level 2:  HEAD ----------------> 6 ------------> 12 ----------------> 19 ------------> NIL" << endl;
  cout << "  Level 1:  HEAD --------> 3 ----> 6 ----> 9 ----> 12 ----> 17 -------> 19 ----> 21 ----> NIL" << endl;
  cout << "  Level 0:  HEAD -> 3 -> 6 -> 7 -> 9 -> 12 -> 17 -> 19 -> 21 -> NIL" << endl;
  cout << endl;
  
  cout << "Search for 17:" << endl;
  cout << "  1. Start at HEAD, Level 3" << endl;
  cout << "  2. 12 < 17, move right to 12" << endl;
  cout << "  3. 12 -> NIL, drop to Level 2" << endl;
  cout << "  4. 19 > 17, drop to Level 1" << endl;
  cout << "  5. 17 = 17, FOUND!" << endl;
  cout << "\nSteps: ~4 (vs 6 for linear search)" << endl;
  
  cout << "\nKey insight: Higher levels act as 'express lanes'" << endl;
  cout << "Expected levels per node: log(n) with p=0.5" << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Skip List Example", ' ', "1.0");
    
    TCLAP::SwitchArg basicArg("b", "basic",
      "Show basic operations demo", false);
    TCLAP::SwitchArg stringArg("s", "string",
      "Show string set demo", false);
    TCLAP::SwitchArg funcArg("f", "functional",
      "Show functional programming demo", false);
    TCLAP::SwitchArg benchArg("p", "benchmark",
      "Run performance benchmark", false);
    TCLAP::SwitchArg visArg("i", "visualize",
      "Visualize skip list structure", false);
    TCLAP::ValueArg<size_t> sizeArg("n", "size",
      "Number of elements for benchmark", false, 100000, "size_t");
    TCLAP::ValueArg<unsigned> seedArg("r", "seed",
      "Random seed", false, 42, "unsigned");
    TCLAP::SwitchArg verboseArg("v", "verbose",
      "Show detailed output", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(basicArg);
    cmd.add(stringArg);
    cmd.add(funcArg);
    cmd.add(benchArg);
    cmd.add(visArg);
    cmd.add(sizeArg);
    cmd.add(seedArg);
    cmd.add(verboseArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    bool runBasic = basicArg.getValue();
    bool runString = stringArg.getValue();
    bool runFunc = funcArg.getValue();
    bool runBench = benchArg.getValue();
    bool runVis = visArg.getValue();
    bool runAll = allArg.getValue();
    size_t size = sizeArg.getValue();
    unsigned seed = seedArg.getValue();
    bool verbose = verboseArg.getValue();
    
    // Default: run all if nothing specified
    if (!runBasic && !runString && !runFunc && !runBench && !runVis)
      runAll = true;
    
    cout << "=== Skip List: Probabilistic Data Structure ===" << endl;
    cout << "Invented by William Pugh (1990)" << endl;
    
    if (runAll || runVis)
      visualize_structure();
    
    if (runAll || runBasic)
      demonstrate_basic_operations();
    
    if (runAll || runString)
      demonstrate_string_set();
    
    if (runAll || runFunc)
      demonstrate_functional();
    
    if (runAll || runBench)
      benchmark_comparison(size, seed, verbose);
    
    cout << "\n=== Complexity Summary ===" << endl;
    cout << "Search:  O(log n) expected" << endl;
    cout << "Insert:  O(log n) expected" << endl;
    cout << "Delete:  O(log n) expected" << endl;
    cout << "Space:   O(n) expected" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}
