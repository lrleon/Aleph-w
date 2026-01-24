
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
 * @file trie_example.C
 * @brief Prefix Tree (Trie): Efficient String Storage and Search
 * 
 * This example demonstrates the Trie (also called prefix tree or digital tree),
 * a tree-like data structure optimized for string storage and retrieval. Tries
 * excel at prefix-based operations and are fundamental to many text processing
 * applications.
 *
 * ## What is a Trie?
 *
 * A trie is a tree data structure where:
 * - **Each node**: Represents a character (or part of a key)
 * - **Edges**: Represent character transitions
 * - **Paths**: From root to marked nodes represent complete words/keys
 * - **Prefix sharing**: Common prefixes share the same path (memory efficient)
 *
 * ### Structure Example
 *
 * Trie storing words: "cat", "car", "card", "dog"
 * ```
 *         Root
 *        /    \
 *       c      d
 *      /        \
 *     a          o
 *    / \          \
 *   t   r          g
 *       |
 *       d
 * ```
 *
 * **Key insight**: Words sharing prefixes share nodes, making tries
 * space-efficient for datasets with many common prefixes.
 *
 * ## Time Complexity
 *
 * For a word/key of length k:
 *
 * | Operation | Complexity | Notes |
 * |-----------|-----------|-------|
 * | Insert | O(k) | One node per character |
 * | Search (exact) | O(k) | Traverse path |
 * | Prefix search | O(k) | Find prefix node |
 * | Delete | O(k) | Remove nodes if unused |
 * | Longest prefix match | O(k) | Find longest matching prefix |
 *
 * **Note**: Complexity is O(k) where k is key length, NOT O(log n)!
 * This makes tries especially efficient for short keys.
 *
 * ## Space Complexity
 *
 * - **Worst case**: O(ALPHABET_SIZE × N × k) where N = number of keys
 * - **Best case**: O(N × k) when many prefixes shared
 * - **Practical**: Usually much better than worst case due to prefix sharing
 *
 * ## Key Operations
 *
 * ### Insertion
 * 1. Start at root
 * 2. For each character in word:
 *    - If child node exists, traverse to it
 *    - If not, create new node
 * 3. Mark final node as "end of word"
 *
 * ### Search
 * 1. Start at root
 * 2. For each character, traverse to child
 * 3. If path exists and final node is marked, word found
 *
 * ### Prefix Search
 * 1. Traverse to prefix node (same as search)
 * 2. Collect all words in subtree
 * 3. Return all words with given prefix
 *
 * ## Real-World Applications
 *
 * ### Autocomplete
 * - **IDEs**: Code completion (IntelliSense, etc.)
 * - **Search engines**: Query suggestions
 * - **Mobile keyboards**: Word prediction
 * - **Command-line**: Tab completion
 *
 * ### Spell Checkers
 * - **Word lookup**: Fast dictionary lookup
 * - **Suggestions**: Find similar words (edit distance)
 * - **Correction**: Suggest corrections for typos
 *
 * ### Network Routing
 * - **IP routing**: Longest prefix matching
 * - **Packet forwarding**: Find best matching route
 * - **CIDR**: Classless Inter-Domain Routing
 *
 * ### Text Processing
 * - **Dictionaries**: Fast word lookup
 * - **Lexical analysis**: Token recognition
 * - **Pattern matching**: Find all occurrences of pattern
 *
 * ### User Interfaces
 * - **T9 predictive text**: Old phone keyboards
 * - **Search bars**: Incremental search
 * - **Command completion**: Shell autocomplete
 *
 * ## Advantages
 *
 * ✅ **Fast lookups**: O(k) where k is key length (not dependent on dataset size!)
 * ✅ **Prefix operations**: Excellent for prefix matching
 * ✅ **Memory efficient**: Shared prefixes save space
 * ✅ **Ordered**: Keys are naturally sorted (inorder traversal)
 *
 * ## Disadvantages
 *
 * ❌ **Memory overhead**: Each node stores pointers (can be large)
 * ❌ **Cache performance**: Pointer chasing (not cache-friendly)
 * ❌ **Sparse**: Many nodes may be empty (wasted space)
 *
 * ## Optimizations
 *
 * ### Compressed Trie (Patricia Trie)
 * - Compress single-child paths
 * - Reduces memory usage
 * - Maintains O(k) operations
 *
 * ### Ternary Search Tree
 * - Hybrid of trie and BST
 * - Better memory usage
 * - Still O(k) operations
 *
 * ## Comparison with Other Structures
 *
 * | Structure | Lookup | Prefix Search | Memory | Best For |
 * |-----------|--------|---------------|--------|----------|
 * | Trie | O(k) | O(k) | High | Prefix operations |
 * | Hash Table | O(1) | No | Medium | Exact lookup |
 * | BST | O(log n) | O(n) | Low | General purpose |
 *
 * **Trie wins when**: Prefix operations are common, keys are short
 *
 * ## Usage Example
 *
 * ```cpp
 * Trie<string> dictionary;
 *
 * dictionary.insert("hello");
 * dictionary.insert("world");
 * dictionary.insert("help");
 *
 * // Search
 * if (dictionary.search("hello"))
 *   cout << "Found!\n";
 *
 * // Prefix search
 * auto words = dictionary.prefix_search("hel"); // Returns ["hello", "help"]
 * ```
 *
 * ## Command-line Usage
 *
 * ```bash
 * # Run all demos (default)
 * ./trie_example
 *
 * # Run specific demos
 * ./trie_example --basic
 * ./trie_example --prefix
 * ./trie_example --spell
 * ./trie_example --commands
 * ./trie_example --structure
 *
 * # Performance demo (use --count to control dataset size)
 * ./trie_example --performance --count 5000
 *
 * # Show help
 * ./trie_example --help
 * ```
 *
 * @see prefix-tree.H Trie implementation
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <prefix-tree.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

/**
 * @brief Demonstrate basic trie operations
 */
void demo_basic_operations()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Trie: Basic Operations" << endl;
  cout << string(60, '=') << endl;
  
  Cnode root('\0');  // Root with sentinel character
  
  cout << "\n--- Insertion ---" << endl;
  
  vector<string> words = {"cat", "car", "card", "care", "careful", "cart"};
  
  cout << "Inserting words with common prefix 'ca':" << endl;
  for (const auto& word : words)
  {
    bool inserted = root.insert_word(word);
    cout << "  " << word << " -> " << (inserted ? "inserted" : "exists") << endl;
  }
  
  // Try inserting duplicates
  cout << "\nTrying to insert duplicate:" << endl;
  cout << "  cat -> " << (root.insert_word("cat") ? "inserted" : "already exists") << endl;
  
  cout << "\n--- Search ---" << endl;
  
  vector<string> to_find = {"cat", "car", "care", "cap", "dog"};
  cout << "Searching for words:" << endl;
  for (const auto& word : to_find)
  {
    bool found = root.contains(word);
    cout << "  " << word << " -> " << (found ? "FOUND" : "not found") << endl;
  }
  
  cout << "\n--- Statistics ---" << endl;
  cout << "Total words stored: " << root.count() << endl;
  
  cout << "\n--- All Words ---" << endl;
  cout << "Words in lexicographic order:" << endl;
  auto all_words = root.words();
  for (size_t i = 0; i < all_words.size(); ++i)
  {
    string word = all_words[i];
    cout << "  " << (i + 1) << ". " << word << endl;
  }
  
  root.destroy();
}

/**
 * @brief Demonstrate prefix search - the trie's killer feature
 */
void demo_prefix_search()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Prefix Search: Autocomplete Feature" << endl;
  cout << string(60, '=') << endl;
  
  Cnode root('\0');
  
  // Build a dictionary
  vector<string> dictionary = {
    "apple", "application", "apply", "approach",
    "apt", "aptitude",
    "banana", "band", "bandana", "bank", "banner",
    "car", "card", "care", "careful", "careless", "career",
    "cart", "cartoon", "carton"
  };
  
  cout << "\nBuilding dictionary with " << dictionary.size() << " words..." << endl;
  for (const auto& word : dictionary)
    root.insert_word(word);
  
  cout << "\n--- Prefix Search Demo ---" << endl;
  
  vector<string> prefixes = {"app", "ban", "car", "cart", "xyz"};
  
  for (const auto& prefix : prefixes)
  {
    cout << "\nPrefix '" << prefix << "' matches:" << endl;
    
    auto matches = root.words_with_prefix(prefix);
    
    if (matches.size() == 0)
      cout << "  (no matches)" << endl;
    else
    {
      for (size_t i = 0; i < matches.size(); ++i)
      {
        string match = matches[i];
        cout << "  - " << match << endl;
      }
    }
  }
  
  cout << "\n--- Simulating Autocomplete ---" << endl;
  
  string input = "c";
  cout << "\nTyping simulation (showing suggestions):" << endl;
  
  while (input.length() <= 4)
  {
    auto suggestions = root.words_with_prefix(input);
    cout << "  User types: '" << input << "'" << endl;
    cout << "    Suggestions (" << suggestions.size() << " matches): ";
    
    size_t shown = 0;
    for (size_t i = 0; i < suggestions.size() and shown < 5; ++i, ++shown)
    {
      if (i > 0) cout << ", ";
      string s = suggestions[i];
      cout << s;
    }
    if (suggestions.size() > 5)
      cout << " ...(" << (suggestions.size() - 5) << " more)";
    cout << endl;
    
    // Simulate user typing more
    if (input == "c") input = "ca";
    else if (input == "ca") input = "car";
    else if (input == "car") input = "care";
    else break;
  }
  
  root.destroy();
}

/**
 * @brief Practical example: Spell checker suggestions
 */
void demo_spell_checker()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Simple Spell Checker" << endl;
  cout << string(60, '=') << endl;
  
  Cnode root('\0');
  
  // Build dictionary
  vector<string> dictionary = {
    "program", "programming", "programmer", "progress", "project",
    "computer", "compute", "computing", "computation",
    "algorithm", "algorithms", "algorithmic",
    "data", "database", "datum",
    "structure", "structures", "structural",
    "the", "they", "them", "there", "their", "these",
    "hello", "help", "helper", "helpful"
  };
  
  cout << "Loading dictionary with " << dictionary.size() << " words..." << endl;
  for (const auto& word : dictionary)
    root.insert_word(word);
  
  cout << "\n--- Spell Check Demo ---" << endl;
  
  vector<string> to_check = {"program", "progam", "algoritm", "helllo", "data", "computer"};
  
  for (const auto& word : to_check)
  {
    cout << "\nChecking: '" << word << "'" << endl;
    
    if (root.contains(word))
    {
      cout << "  Status: Correct!" << endl;
    }
    else
    {
      cout << "  Status: Not found - might be misspelled" << endl;
      
      // Simple suggestion: words with same prefix
      for (size_t prefixLen = word.length() - 1; prefixLen >= 2; --prefixLen)
      {
        string prefix = word.substr(0, prefixLen);
        auto suggestions = root.words_with_prefix(prefix);
        
        if (suggestions.size() > 0)
        {
          cout << "  Did you mean: ";
          size_t shown = 0;
          for (size_t i = 0; i < suggestions.size() and shown < 3; ++i, ++shown)
          {
            if (i > 0) cout << ", ";
            string s = suggestions[i];
            cout << s;
          }
          cout << "?" << endl;
          break;
        }
      }
    }
  }
  
  root.destroy();
}

/**
 * @brief Practical example: Command-line autocomplete
 */
void demo_command_autocomplete()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Shell Command Autocomplete" << endl;
  cout << string(60, '=') << endl;
  
  Cnode root('\0');
  
  // Common shell commands
  vector<string> commands = {
    "cd", "ls", "pwd", "mkdir", "rmdir", "rm", "cp", "mv",
    "cat", "less", "more", "head", "tail",
    "grep", "find", "locate", "which", "whereis",
    "chmod", "chown", "chgrp",
    "ps", "top", "htop", "kill", "killall",
    "ssh", "scp", "sftp",
    "git", "gitk", "github",
    "make", "cmake", "gcc", "g++", "gdb",
    "python", "python3", "pip", "pip3",
    "apt", "apt-get", "apt-cache"
  };
  
  cout << "Loading " << commands.size() << " shell commands..." << endl;
  for (const auto& cmd : commands)
    root.insert_word(cmd);
  
  cout << "\n--- Tab Completion Simulation ---" << endl;
  
  vector<string> partial_inputs = {"g", "gi", "apt", "ch", "py"};
  
  for (const auto& input : partial_inputs)
  {
    cout << "\n$ " << input << "<TAB>" << endl;
    
    auto completions = root.words_with_prefix(input);
    
    if (completions.size() == 0)
      cout << "  (no completions)" << endl;
    else if (completions.size() == 1)
    {
      string c = completions[0];
      cout << "  -> " << c << " (unique match)" << endl;
    }
    else
    {
      cout << "  Possible completions: ";
      for (size_t i = 0; i < completions.size(); ++i)
      {
        if (i > 0) cout << "  ";
        string c = completions[i];
        cout << c;
      }
      cout << endl;
    }
  }
  
  root.destroy();
}

/**
 * @brief Show trie structure visualization
 */
void demo_trie_structure()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Trie Structure Visualization" << endl;
  cout << string(60, '=') << endl;
  
  Cnode root('\0');
  
  vector<string> words = {"cat", "car", "card"};
  
  cout << "\nInserting: ";
  for (size_t i = 0; i < words.size(); ++i)
  {
    if (i > 0) cout << ", ";
    cout << words[i];
    root.insert_word(words[i]);
  }
  cout << endl;
  
  cout << "\nTrie structure:" << endl;
  cout << "             root" << endl;
  cout << "               |" << endl;
  cout << "               c" << endl;
  cout << "               |" << endl;
  cout << "               a" << endl;
  cout << "              /|" << endl;
  cout << "             t r ($=end)" << endl;
  cout << "             |  |" << endl;
  cout << "             $ ($) d" << endl;
  cout << "                  |" << endl;
  cout << "                  $" << endl;
  cout << endl;
  cout << "Words: cat, car, card" << endl;
  cout << "Notice how 'c', 'a' are shared!" << endl;
  
  cout << "\nTree string representation: " << root.to_str() << endl;
  
  root.destroy();
}

/**
 * @brief Performance comparison vs hash set
 */
void demo_performance(int n)
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Performance Analysis (n = " << n << " words)" << endl;
  cout << string(60, '=') << endl;
  
  Cnode root('\0');
  
  // Generate random-ish words
  vector<string> words;
  words.reserve(n);
  
  vector<string> prefixes = {"pre", "post", "un", "re", "in", "ex", "sub", "super", "anti", "auto"};
  vector<string> roots = {"act", "form", "port", "ject", "duct", "spect", "scrib", "struct", "mit", "vers"};
  vector<string> suffixes = {"ion", "ment", "ness", "able", "ible", "ful", "less", "ive", "ous", "al"};
  
  for (int i = 0; i < n; ++i)
  {
    string word = prefixes[i % prefixes.size()] + 
                  roots[(i / prefixes.size()) % roots.size()] +
                  suffixes[(i / (prefixes.size() * roots.size())) % suffixes.size()];
    // Add some variation
    if (i % 3 == 0) word += "ed";
    if (i % 5 == 0) word += "ly";
    if (i % 7 == 0) word += "ing";
    words.push_back(word);
  }
  
  cout << "\nGenerated " << words.size() << " words for testing" << endl;
  
  // Insertion benchmark
  auto start = chrono::high_resolution_clock::now();
  
  for (const auto& word : words)
    root.insert_word(word);
  
  auto mid = chrono::high_resolution_clock::now();
  
  // Search benchmark
  int found = 0;
  for (const auto& word : words)
    if (root.contains(word)) ++found;
  
  auto end = chrono::high_resolution_clock::now();
  
  auto insert_us = chrono::duration_cast<chrono::microseconds>(mid - start).count();
  auto search_us = chrono::duration_cast<chrono::microseconds>(end - mid).count();
  
  cout << "\nResults:" << endl;
  cout << "  Words in trie: " << root.count() << endl;
  cout << "  Insert time: " << insert_us << " us" << endl;
  cout << "  Search time: " << search_us << " us" << endl;
  cout << "  Found: " << found << "/" << words.size() << endl;
  
  // Prefix search benchmark
  start = chrono::high_resolution_clock::now();
  
  size_t total_matches = 0;
  for (const auto& prefix : prefixes)
  {
    auto matches = root.words_with_prefix(prefix);
    total_matches += matches.size();
  }
  
  end = chrono::high_resolution_clock::now();
  
  auto prefix_us = chrono::duration_cast<chrono::microseconds>(end - start).count();
  
  cout << "\nPrefix search (10 prefixes):" << endl;
  cout << "  Time: " << prefix_us << " us" << endl;
  cout << "  Total matches: " << total_matches << endl;
  
  root.destroy();
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Trie (Prefix Tree) Example", ' ', "1.0");
    
    TCLAP::ValueArg<int> nArg("n", "count",
      "Number of words for performance test", false, 1000, "int");
    TCLAP::SwitchArg basicArg("b", "basic",
      "Show basic operations", false);
    TCLAP::SwitchArg prefixArg("p", "prefix",
      "Show prefix search / autocomplete", false);
    TCLAP::SwitchArg spellArg("s", "spell",
      "Show spell checker example", false);
    TCLAP::SwitchArg cmdArg("c", "commands",
      "Show command autocomplete example", false);
    TCLAP::SwitchArg structArg("t", "structure",
      "Show trie structure visualization", false);
    TCLAP::SwitchArg perfArg("f", "performance",
      "Run performance analysis", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(nArg);
    cmd.add(basicArg);
    cmd.add(prefixArg);
    cmd.add(spellArg);
    cmd.add(cmdArg);
    cmd.add(structArg);
    cmd.add(perfArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    int n = nArg.getValue();
    bool runBasic = basicArg.getValue();
    bool runPrefix = prefixArg.getValue();
    bool runSpell = spellArg.getValue();
    bool runCmd = cmdArg.getValue();
    bool runStruct = structArg.getValue();
    bool runPerf = perfArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runBasic and not runPrefix and not runSpell and 
        not runCmd and not runStruct and not runPerf)
      runAll = true;
    
    cout << "=== Trie (Prefix Tree): Efficient String Storage ===" << endl;
    
    if (runAll or runBasic)
      demo_basic_operations();
    
    if (runAll or runStruct)
      demo_trie_structure();
    
    if (runAll or runPrefix)
      demo_prefix_search();
    
    if (runAll or runSpell)
      demo_spell_checker();
    
    if (runAll or runCmd)
      demo_command_autocomplete();
    
    if (runAll or runPerf)
      demo_performance(n);
    
    cout << "\n=== Summary ===" << endl;
    cout << "Tries excel at:" << endl;
    cout << "  - Fast prefix searches (autocomplete)" << endl;
    cout << "  - Memory-efficient storage of strings with shared prefixes" << endl;
    cout << "  - O(k) operations where k = word length" << endl;
    cout << "Use cases: autocomplete, spell checkers, IP routing, dictionaries" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

