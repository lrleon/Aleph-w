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
 * This example demonstrates Skip Lists, an elegant randomized data structure
 * invented by William Pugh in 1990. Skip Lists provide expected O(log n)
 * performance for search, insert, and delete operations without the complexity
 * of balanced tree rotations, making them easier to implement and understand.
 *
 * ## What is a Skip List?
 *
 * A Skip List is a probabilistic data structure that uses multiple sorted
 * linked lists at different "levels" to enable fast search. Think of it as
 * a subway system with express trains (higher levels) and local trains
 * (lower levels).
 *
 * ## How Skip Lists Work
 *
### Structure
 *
 * A Skip List consists of multiple linked lists (levels):
 * - **Level 0**: Contains ALL elements in sorted order (base level)
 * - **Level 1**: Contains ~50% of elements (express lane)
 * - **Level 2**: Contains ~25% of elements (super express)
 * - **Level i**: Contains ~(1/2)^i of elements
 *
### Example Structure
 *
 * For elements {3, 6, 7, 9}:
 * ```
 * Level 2:  HEAD -------------------------> 6 -------------------------> NIL
 * Level 1:  HEAD ---------> 3 ---------> 6 ---------> 9 ---------> NIL
 * Level 0:  HEAD -> 3 -> 6 -> 7 -> 9 -> NIL
 * ```
 *
### Search Algorithm
 *
 * 1. Start at highest level (HEAD)
 * 2. Move right while next element < target
 * 3. If can't go right, go down one level
 * 4. Repeat until found or reach level 0
 *
 * **Key insight**: Higher levels act as "express lanes" allowing you to
 * skip over many elements quickly.
 *
### Insertion Algorithm
 *
 * 1. Search for insertion point (as in search)
 * 2. Insert at level 0
 * 3. "Flip a coin" (random): With 50% probability, promote to level 1
 * 4. If promoted, flip again for level 2, etc.
 * 5. Stop when coin flip fails
 *
 * **Randomization**: Each element has 50% chance of being at level i+1
 * if it's at level i. This creates the probabilistic structure.
 *
## Advantages Over Balanced Trees
 *
### Simplicity
 * - **No rotations**: Unlike AVL or Red-Black trees
 * - **No rebalancing**: Structure maintained probabilistically
 * - **Easier to implement**: Simpler code, fewer edge cases
 * - **Easier to debug**: Linear structure easier to visualize
 *
### Concurrency
 * - **Lock-free versions**: Easier to make thread-safe
 * - **Less contention**: Fewer shared data structures
 * - **Better scalability**: Used in high-performance systems
 *
### Performance
 * - **Cache-friendly**: Sequential memory access (linked lists)
 * - **Expected O(log n)**: Same as balanced trees
 * - **Good constants**: Often faster in practice
 *
### Flexibility
 * - **Range queries**: Efficient iteration over ranges
 * - **Bidirectional**: Can traverse forward and backward
 * - **Dynamic**: Easy to add/remove levels
 *
## Disadvantages
 *
 * ❌ **Probabilistic**: No worst-case guarantee (though extremely unlikely)
 * ❌ **Memory overhead**: Multiple pointers per element
 * ❌ **Randomization**: Requires good random number generator
 *
## Complexity
 *
 * | Operation | Expected | Worst Case | Notes |
 * |-----------|----------|------------|-------|
 * | Search | O(log n) | O(n) | Extremely rare worst case |
 * | Insert | O(log n) | O(n) | Includes search + insertion |
 * | Delete | O(log n) | O(n) | Includes search + deletion |
 * | Range query | O(log n + k) | O(n) | k = elements in range |
 *
 * **Expected height**: O(log n) with high probability
 * **Worst case height**: O(n) but probability is negligible
 *
## Applications
 *
### Database Systems
 * - **Redis**: Sorted sets (ZSET) use skip lists
 * - **LevelDB/RocksDB**: Memtable implementation
 * - **Apache Cassandra**: Index structures
 *
### Concurrent Data Structures
 * - **Lock-free skip lists**: High-performance concurrent maps
 * - **Multi-threaded applications**: Thread-safe containers
 *
### Range Queries
 * - **Time-series data**: Efficient range scans
 * - **Spatial data**: Range queries on sorted data
 *
### Educational
 * - **Teaching data structures**: Simpler than balanced trees
 * - **Algorithm courses**: Demonstrates randomization
 *
## Comparison with Balanced Trees
 *
 * | Feature | Skip List | Balanced Tree (AVL/RB) |
 * |---------|-----------|------------------------|
 * | Implementation | Simpler | More complex |
 * | Rotations | None | Required |
 * | Worst-case guarantee | Probabilistic | Guaranteed |
 * | Concurrent version | Easier | Harder |
 * | Cache performance | Good | Variable |
 * | Memory overhead | Higher | Lower |
 *
## When to Use Skip Lists
 *
 * ✅ **Good for**:
 * - When simplicity matters
 * - Concurrent applications
 * - Range queries needed
 * - Educational purposes
 *
 * ❌ **Not good for**:
 * - When worst-case guarantees critical
 * - Memory-constrained environments
 * - Deterministic performance required
 *
## Usage Example
 *
 * ```cpp
 * DynSkipList<int> skip_list;
 *
 * skip_list.insert(10);
 * skip_list.insert(5);
 * skip_list.insert(15);
 *
 * if (skip_list.search(10))
 *   cout << "Found 10!\n";
 *
 * skip_list.remove(5);
 * ```
 *
 * @see tpl_dynSkipList.H Skip List implementation
 * @see dynset_trees.C Balanced tree alternatives
 * @see Pugh, W. "Skip Lists: A Probabilistic Alternative to Balanced Trees" (1990)
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
