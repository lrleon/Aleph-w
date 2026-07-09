
/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file radix_tree_example.cc
 * @brief Word lookup, prefix autocomplete, and longest-prefix match with
 *        `Aleph::RadixTree`.
 *
 * Usage:
 *   ./radix_tree_example
 */

#include <print_rule.H>
#include <tpl_radix_tree.H>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace Aleph;

namespace
{
void demo_basic_usage()
{
  std::cout << "[1] Insert/find/erase walkthrough\n";
  print_rule();

  RadixTree<int> word_count;
  word_count.insert("apple", 3);
  word_count.insert("app", 1);
  word_count.insert("application", 7);

  std::cout << "apple: " << *word_count.find("apple") << " (expect 3)\n";
  std::cout << "contains 'appl': " << std::boolalpha
            << word_count.contains("appl") << " (expect false)\n";

  word_count.insert_or_assign("app", 2);
  std::cout << "app after insert_or_assign: " << *word_count.find("app")
            << " (expect 2)\n";

  word_count.erase("application");
  std::cout << "contains 'application' after erase: "
            << word_count.contains("application") << " (expect false)\n";
  std::cout << "still contains 'app': " << word_count.contains("app")
            << " (expect true)\n\n";
}

void demo_autocomplete()
{
  std::cout << "[2] Prefix autocomplete (keys_with_prefix)\n";
  print_rule();

  RadixTree<int> dictionary;
  for (const auto & word :
       {"cat", "car", "cart", "carbon", "care", "dog", "door"})
    dictionary.insert(word, 0);

  auto suggestions = dictionary.keys_with_prefix("car");
  std::vector<std::string> sorted_suggestions;
  for (const auto & w : suggestions)
    sorted_suggestions.push_back(w);
  std::sort(sorted_suggestions.begin(), sorted_suggestions.end());

  std::cout << "Words starting with \"car\": ";
  for (const auto & w : sorted_suggestions)
    std::cout << w << " ";
  std::cout << "\n\n";
}

void demo_longest_prefix_match()
{
  std::cout << "[3] Longest-prefix match (e.g. hierarchical route lookup)\n";
  print_rule();

  // A toy routing-like table: more specific routes shadow general ones.
  RadixTree<std::string> routes;
  routes.insert("/api", "generic-api-handler");
  routes.insert("/api/users", "users-handler");
  routes.insert("/api/users/admin", "admin-handler");

  for (const auto & path :
       {"/api/users/admin/settings", "/api/users/42", "/api/health"})
    {
      auto matched = routes.longest_prefix(path);
      if (matched)
        std::cout << path << " -> " << *routes.find(*matched) << " (matched \""
                  << *matched << "\")\n";
      else
        std::cout << path << " -> no route matched\n";
    }
  std::cout << "\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Aleph::RadixTree: compressed prefix tree ===\n\n";

  demo_basic_usage();
  demo_autocomplete();
  demo_longest_prefix_match();

  std::cout << "Done.\n";
  return 0;
}