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
 * @file dynmap_example.C
 * @brief DynMapTree: Key-Value Mappings with Various Tree Backends
 * 
 * This example demonstrates the DynMapTree class, which provides
 * associative key-value storage using different balanced BST implementations.
 * 
 * ## Available Tree Backends
 * 
 * - **Avl_Tree**: Strictly balanced, deterministic O(log n)
 * - **Rb_Tree**: Red-Black tree, efficient for frequent updates
 * - **Splay_Tree**: Self-adjusting, good for access patterns with locality
 * - **Treap**: Randomized, probabilistic O(log n)
 * - **Rand_Tree**: Another randomized tree variant
 * 
 * ## When to Use DynMapTree vs Hash Maps
 * 
 * ### Use DynMapTree when:
 * - Keys need to be ordered
 * - You need range queries (min, max, in-range iteration)
 * - Predictable worst-case performance matters
 * - Keys have good comparison operators but poor hash functions
 * 
 * ### Use Hash Maps when:
 * - Only point queries are needed
 * - Average O(1) access is acceptable
 * - Order doesn't matter
 * 
 * @see tpl_dynMapTree.H
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <tpl_dynMapTree.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

/**
 * @brief Demonstrate basic map operations
 */
void demo_basic_operations()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "DynMapTree: Basic Operations" << endl;
  cout << string(60, '=') << endl;
  
  // Default uses AVL tree
  DynMapTree<string, int> ages;
  
  cout << "\n--- Insertion ---" << endl;
  
  // Different ways to insert
  ages.insert("Alice", 30);
  ages.insert("Bob", 25);
  ages["Charlie"] = 35;  // Creates if not exists
  ages["Diana"] = 28;
  ages.insert(make_pair("Eve", 42));
  
  cout << "Inserted 5 entries" << endl;
  cout << "Size: " << ages.size() << endl;
  
  cout << "\n--- Access ---" << endl;
  
  // Direct access (throws if not found)
  cout << "Alice's age: " << ages["Alice"] << endl;
  cout << "Bob's age: " << ages.find("Bob") << endl;
  
  // Safe access with search
  auto* result = ages.search("Charlie");
  if (result != nullptr)
    cout << "Charlie found: " << result->second << endl;
  
  result = ages.search("Unknown");
  cout << "Unknown found: " << (result ? "yes" : "no") << endl;
  
  cout << "\n--- Modification ---" << endl;
  
  ages["Alice"] = 31;  // Update existing
  cout << "Updated Alice's age to " << ages["Alice"] << endl;
  
  cout << "\n--- Iteration (sorted by key) ---" << endl;
  
  for (auto it = ages.get_it(); it.has_curr(); it.next())
  {
    auto& pair = it.get_curr();
    cout << "  " << pair.first << " -> " << pair.second << endl;
  }
  
  cout << "\n--- Removal ---" << endl;
  
  ages.remove("Bob");
  cout << "Removed Bob, new size: " << ages.size() << endl;
  
  cout << "\n--- Contains check ---" << endl;
  cout << "Has Alice: " << (ages.has("Alice") ? "yes" : "no") << endl;
  cout << "Has Bob: " << (ages.has("Bob") ? "yes" : "no") << endl;
}

/**
 * @brief Demonstrate different tree backends
 */
void demo_tree_backends()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Different Tree Backend Implementations" << endl;
  cout << string(60, '=') << endl;
  
  // AVL Tree (default)
  DynMapTree<int, string, Avl_Tree> avl_map;
  avl_map[1] = "one";
  avl_map[2] = "two";
  avl_map[3] = "three";
  cout << "\n1. Avl_Tree (strictly balanced):" << endl;
  cout << "   Height guarantee: <= 1.44 * log2(n)" << endl;
  cout << "   Size: " << avl_map.size() << endl;
  
  // Red-Black Tree
  DynMapTree<int, string, Rb_Tree> rb_map;
  rb_map[1] = "one";
  rb_map[2] = "two";
  rb_map[3] = "three";
  cout << "\n2. Rb_Tree (red-black):" << endl;
  cout << "   Height guarantee: <= 2 * log2(n)" << endl;
  cout << "   Size: " << rb_map.size() << endl;
  
  // Splay Tree
  DynMapTree<int, string, Splay_Tree> splay_map;
  splay_map[1] = "one";
  splay_map[2] = "two";
  splay_map[3] = "three";
  cout << "\n3. Splay_Tree (self-adjusting):" << endl;
  cout << "   Amortized O(log n), good for locality" << endl;
  cout << "   Size: " << splay_map.size() << endl;
  
  // Treap
  DynMapTree<int, string, Treap> treap_map;
  treap_map[1] = "one";
  treap_map[2] = "two";
  treap_map[3] = "three";
  cout << "\n4. Treap (tree + heap):" << endl;
  cout << "   Expected O(log n), randomized" << endl;
  cout << "   Size: " << treap_map.size() << endl;
  
  // Rand Tree
  DynMapTree<int, string, Rand_Tree> rand_map;
  rand_map[1] = "one";
  rand_map[2] = "two";
  rand_map[3] = "three";
  cout << "\n5. Rand_Tree (randomized):" << endl;
  cout << "   Expected O(log n), randomized" << endl;
  cout << "   Size: " << rand_map.size() << endl;
  
  cout << "\nAll backends provide the same interface!" << endl;
}

/**
 * @brief Practical example: Word frequency counter
 */
void demo_word_frequency()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Word Frequency Counter" << endl;
  cout << string(60, '=') << endl;
  
  DynMapTree<string, int> freq;
  
  // Sample text
  vector<string> words = {
    "the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog",
    "the", "fox", "is", "quick", "and", "the", "dog", "is", "lazy",
    "quick", "foxes", "are", "brown", "lazy", "dogs", "are", "cute"
  };
  
  cout << "\nProcessing " << words.size() << " words..." << endl;
  
  for (const auto& word : words)
  {
    if (freq.has(word))
      freq[word]++;
    else
      freq[word] = 1;
  }
  
  cout << "\nWord frequencies (alphabetically sorted):" << endl;
  for (auto it = freq.get_it(); it.has_curr(); it.next())
  {
    auto& p = it.get_curr();
    cout << "  " << setw(8) << p.first << ": " << p.second << endl;
  }
  
  cout << "\nUnique words: " << freq.size() << endl;
}

/**
 * @brief Practical example: Configuration store
 */
void demo_config_store()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Configuration Store" << endl;
  cout << string(60, '=') << endl;
  
  // Using string keys for configuration
  DynMapTree<string, string> config;
  
  // Load configuration
  config["app.name"] = "MyApplication";
  config["app.version"] = "1.0.0";
  config["database.host"] = "localhost";
  config["database.port"] = "5432";
  config["database.name"] = "mydb";
  config["logging.level"] = "INFO";
  config["logging.file"] = "/var/log/app.log";
  config["cache.enabled"] = "true";
  config["cache.ttl"] = "3600";
  
  cout << "\nAll configuration (sorted by key):" << endl;
  for (auto it = config.get_it(); it.has_curr(); it.next())
  {
    auto& p = it.get_curr();
    cout << "  " << p.first << " = " << p.second << endl;
  }
  
  // Access specific sections
  cout << "\n--- Accessing Specific Values ---" << endl;
  cout << "App name: " << config["app.name"] << endl;
  cout << "DB host: " << config["database.host"] << endl;
  
  // Check for optional config
  auto* log_file = config.search("logging.file");
  if (log_file)
    cout << "Log file: " << log_file->second << endl;
  
  // Default value pattern
  auto* optional = config.search("optional.feature");
  string value = optional ? optional->second : "default_value";
  cout << "Optional feature: " << value << endl;
}

/**
 * @brief Functional programming with maps
 */
void demo_functional()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Functional Programming Features" << endl;
  cout << string(60, '=') << endl;
  
  DynMapTree<string, int> scores;
  scores["Alice"] = 85;
  scores["Bob"] = 92;
  scores["Charlie"] = 78;
  scores["Diana"] = 95;
  scores["Eve"] = 88;
  
  cout << "\nOriginal scores:" << endl;
  scores.for_each([](auto& p) {
    cout << "  " << p.first << ": " << p.second << endl;
  });
  
  // Count elements satisfying condition
  size_t high_scorers = 0;
  scores.for_each([&high_scorers](const auto& p) { 
    if (p.second >= 90) ++high_scorers;
  });
  cout << "\nStudents with score >= 90: " << high_scorers << endl;
  
  // Check if all satisfy condition
  bool all_passed = scores.all([](const auto& p) { 
    return p.second >= 60; 
  });
  cout << "All passed (>= 60): " << (all_passed ? "yes" : "no") << endl;
  
  // Check if any satisfies condition
  bool any_perfect = scores.exists([](const auto& p) { 
    return p.second == 100; 
  });
  cout << "Any perfect score: " << (any_perfect ? "yes" : "no") << endl;
  
  // Filter (create new map with qualifying entries)
  cout << "\nHigh scorers (>= 90):" << endl;
  auto high = scores.filter([](const auto& p) { return p.second >= 90; });
  high.for_each([](auto& p) {
    cout << "  " << p.first << ": " << p.second << endl;
  });
  
  // Fold (reduce to single value)
  int total = scores.foldl<int>(0, [](int acc, const auto& p) {
    return acc + p.second;
  });
  cout << "\nTotal score: " << total << endl;
  cout << "Average: " << (total / (double)scores.size()) << endl;
}

/**
 * @brief Performance comparison of backends
 */
void demo_performance(int n)
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Performance Comparison (n = " << n << ")" << endl;
  cout << string(60, '=') << endl;
  
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dis(1, n * 10);
  
  vector<int> keys(n);
  for (int i = 0; i < n; ++i)
    keys[i] = dis(gen);
  
  auto benchmark = [&](auto& map, const string& name) {
    auto start = chrono::high_resolution_clock::now();
    
    // Insert
    for (int k : keys)
      map[k] = k * 2;
    
    auto mid = chrono::high_resolution_clock::now();
    
    // Lookup
    volatile int dummy = 0;
    for (int k : keys)
    {
      auto* p = map.search(k);
      if (p) dummy = p->second;
    }
    
    auto end = chrono::high_resolution_clock::now();
    
    auto insert_us = chrono::duration_cast<chrono::microseconds>(mid - start).count();
    auto lookup_us = chrono::duration_cast<chrono::microseconds>(end - mid).count();
    
    cout << setw(12) << name << ": "
         << "Insert " << setw(6) << insert_us << " us, "
         << "Lookup " << setw(6) << lookup_us << " us" << endl;
    
    (void)dummy;  // Suppress warning
  };
  
  {
    DynMapTree<int, int, Avl_Tree> map;
    benchmark(map, "Avl_Tree");
  }
  
  {
    DynMapTree<int, int, Rb_Tree> map;
    benchmark(map, "Rb_Tree");
  }
  
  {
    DynMapTree<int, int, Splay_Tree> map;
    benchmark(map, "Splay_Tree");
  }
  
  {
    DynMapTree<int, int, Treap> map;
    benchmark(map, "Treap");
  }
  
  {
    DynMapTree<int, int, Rand_Tree> map;
    benchmark(map, "Rand_Tree");
  }
  
  cout << "\n--- Analysis ---" << endl;
  cout << "All backends are O(log n) average case" << endl;
  cout << "AVL: Most balanced, slightly slower updates" << endl;
  cout << "Red-Black: Good balance, faster updates" << endl;
  cout << "Splay: Best for repeated access patterns" << endl;
  cout << "Treap/Rand: Good average, simple implementation" << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("DynMapTree Example", ' ', "1.0");
    
    TCLAP::ValueArg<int> nArg("n", "count",
      "Number of elements for performance test", false, 10000, "int");
    TCLAP::SwitchArg basicArg("b", "basic",
      "Show basic operations", false);
    TCLAP::SwitchArg backendsArg("t", "trees",
      "Show different tree backends", false);
    TCLAP::SwitchArg freqArg("w", "words",
      "Show word frequency example", false);
    TCLAP::SwitchArg configArg("c", "config",
      "Show configuration store example", false);
    TCLAP::SwitchArg funcArg("f", "functional",
      "Show functional programming features", false);
    TCLAP::SwitchArg perfArg("p", "performance",
      "Run performance comparison", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(nArg);
    cmd.add(basicArg);
    cmd.add(backendsArg);
    cmd.add(freqArg);
    cmd.add(configArg);
    cmd.add(funcArg);
    cmd.add(perfArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    int n = nArg.getValue();
    bool runBasic = basicArg.getValue();
    bool runBackends = backendsArg.getValue();
    bool runFreq = freqArg.getValue();
    bool runConfig = configArg.getValue();
    bool runFunc = funcArg.getValue();
    bool runPerf = perfArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runBasic and not runBackends and not runFreq and 
        not runConfig and not runFunc and not runPerf)
      runAll = true;
    
    cout << "=== DynMapTree: Key-Value Mappings ===" << endl;
    
    if (runAll or runBasic)
      demo_basic_operations();
    
    if (runAll or runBackends)
      demo_tree_backends();
    
    if (runAll or runFreq)
      demo_word_frequency();
    
    if (runAll or runConfig)
      demo_config_store();
    
    if (runAll or runFunc)
      demo_functional();
    
    if (runAll or runPerf)
      demo_performance(n);
    
    cout << "\n=== Summary ===" << endl;
    cout << "DynMapTree provides ordered key-value storage" << endl;
    cout << "Choose backend based on access patterns:" << endl;
    cout << "  - AVL for predictable performance" << endl;
    cout << "  - Splay for locality-heavy workloads" << endl;
    cout << "  - Treap for simplicity with good average case" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

