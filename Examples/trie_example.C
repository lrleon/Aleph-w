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
 * @file trie_example.C
 * @brief Prefix Tree (Trie): Efficient String Storage and Search
 * 
 * This example demonstrates the Trie (prefix tree) data structure,
 * which is highly efficient for string-related operations.
 * 
 * ## What is a Trie?
 * 
 * A trie is a tree-like data structure where:
 * - Each node represents a character
 * - Paths from root to special markers represent complete words
 * - Common prefixes share nodes (memory efficient)
 * 
 * ## Time Complexity
 * 
 * For a word of length k:
 * - Insert: O(k)
 * - Search: O(k)
 * - Prefix search: O(k)
 * - Delete: O(k)
 * 
 * ## Real-World Applications
 * 
 * - **Autocomplete**: IDE suggestions, search engines
 * - **Spell checkers**: Finding similar words
 * - **IP routing**: Longest prefix matching
 * - **Dictionaries**: Fast word lookup
 * - **T9 predictive text**: Phone keyboards
 * 
 * @see prefix-tree.H
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

