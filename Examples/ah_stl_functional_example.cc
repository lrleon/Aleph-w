/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file ah-stl-functional-example.cc
 * @brief Comprehensive examples of ah-stl-functional.H usage
 *
 * This file demonstrates the functional programming utilities available
 * in ah-stl-functional.H for working with STL containers.
 *
 * Compile with:
 *   g++ -std=c++20 -I.. -o ah-stl-functional-example ah-stl-functional-example.cc
 *
 * Or using CMake from the build directory:
 *   make ah-stl-functional-example
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <set>
#include <string>
#include <ah-stl-functional.H>
#include <ah-uni-functional.H>
#include <htlist.H>
#include <tpl_dynSetTree.H>

using namespace std;
using namespace Aleph;

// Helper to print vectors
template <typename T>
void print_vec(const string & label, const vector<T> & v)
{
  cout << label << ": {";
  for (size_t i = 0; i < v.size(); ++i)
    {
      cout << v[i];
      if (i < v.size() - 1) cout << ", ";
    }
  cout << "}" << endl;
}

// Helper to print pairs
template <typename T1, typename T2>
void print_pairs(const string & label, const vector<pair<T1, T2>> & v)
{
  cout << label << ": {";
  for (size_t i = 0; i < v.size(); ++i)
    {
      cout << "(" << v[i].first << ", " << v[i].second << ")";
      if (i < v.size() - 1) cout << ", ";
    }
  cout << "}" << endl;
}

// Helper for nested vectors
template <typename T>
void print_nested(const string & label, const vector<vector<T>> & v)
{
  cout << label << ":" << endl;
  for (const auto & inner : v)
    {
      cout << "  {";
      for (size_t i = 0; i < inner.size(); ++i)
        {
          cout << inner[i];
          if (i < inner.size() - 1) cout << ", ";
        }
      cout << "}" << endl;
    }
}

int main()
{
  cout << "========================================" << endl;
  cout << "  ah-stl-functional.H Usage Examples" << endl;
  cout << "========================================" << endl << endl;

  // ============================================================================
  // 1. Range Generation
  // ============================================================================
  cout << "--- 1. Range Generation ---" << endl;

  auto r1 = stl_range(1, 5);
  print_vec("stl_range(1, 5)", r1);

  auto r2 = stl_range(0, 10, 2);
  print_vec("stl_range(0, 10, 2)", r2);

  auto r3 = stl_range(5);
  print_vec("stl_range(5)", r3);

  auto lin = stl_linspace(0.0, 1.0, 5);
  cout << "stl_linspace(0.0, 1.0, 5): {";
  for (size_t i = 0; i < lin.size(); ++i)
    {
      cout << fixed << setprecision(2) << lin[i];
      if (i < lin.size() - 1) cout << ", ";
    }
  cout << "}" << endl;

  auto rep = stl_rep(4, 42);
  print_vec("stl_rep(4, 42)", rep);

  auto gen = stl_generate(5, [](size_t i) { return i * i; });
  print_vec("stl_generate(5, i -> i²)", gen);

  cout << endl;

  // ============================================================================
  // 2. Map and Transform
  // ============================================================================
  cout << "--- 2. Map and Transform ---" << endl;

  vector<int> nums = {1, 2, 3, 4, 5};
  print_vec("Original", nums);

  auto squares = stl_map([](int x) { return x * x; }, nums);
  print_vec("stl_map(x -> x²)", squares);

  auto strings = stl_map([](int x) { return "num_" + to_string(x); }, nums);
  print_vec("stl_map(x -> \"num_\" + x)", strings);

  auto indexed = stl_mapi([](size_t i, int x) {
    return "[" + to_string(i) + "]=" + to_string(x);
  }, nums);
  print_vec("stl_mapi((i, x) -> \"[i]=x\")", indexed);

  cout << endl;

  // ============================================================================
  // 3. Filter and Reject
  // ============================================================================
  cout << "--- 3. Filter and Reject ---" << endl;

  vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  print_vec("Original", data);

  auto evens = stl_filter([](int x) { return x % 2 == 0; }, data);
  print_vec("stl_filter(x -> x % 2 == 0)", evens);

  auto odds = stl_reject([](int x) { return x % 2 == 0; }, data);
  print_vec("stl_reject(x -> x % 2 == 0)", odds);

  auto even_indices = stl_filteri([](size_t i, int) { return i % 2 == 0; }, data);
  print_vec("stl_filteri((i, x) -> i % 2 == 0)", even_indices);

  cout << endl;

  // ============================================================================
  // 4. Fold (Reduce) Operations
  // ============================================================================
  cout << "--- 4. Fold (Reduce) Operations ---" << endl;

  vector<int> v = {1, 2, 3, 4, 5};
  print_vec("Original", v);

  int sum = stl_foldl(0, [](int acc, int x) { return acc + x; }, v);
  cout << "stl_foldl(0, +): " << sum << endl;

  int product = stl_foldl(1, [](int acc, int x) { return acc * x; }, v);
  cout << "stl_foldl(1, *): " << product << endl;

  // Right fold: 1 - (2 - (3 - (4 - (5 - 0)))) = 3
  int foldr_result = stl_foldr(0, [](int x, int acc) { return x - acc; }, v);
  cout << "stl_foldr(0, -): " << foldr_result << " (1-(2-(3-(4-(5-0)))))" << endl;

  // Scan left: running sum
  auto scan = stl_scan_left(0, [](int acc, int x) { return acc + x; }, v);
  print_vec("stl_scan_left(0, +)", scan);

  cout << endl;

  // ============================================================================
  // 5. Predicates (all, exists, none)
  // ============================================================================
  cout << "--- 5. Predicates ---" << endl;

  vector<int> all_even = {2, 4, 6, 8};
  vector<int> some_even = {1, 2, 3, 4};
  vector<int> no_even = {1, 3, 5, 7};

  auto is_even = [](int x) { return x % 2 == 0; };

  cout << "all_even = {2, 4, 6, 8}" << endl;
  cout << "  stl_all(is_even): " << boolalpha << stl_all(is_even, all_even) << endl;
  cout << "  stl_exists(is_even): " << stl_exists(is_even, all_even) << endl;
  cout << "  stl_none(is_even): " << stl_none(is_even, all_even) << endl;

  cout << "no_even = {1, 3, 5, 7}" << endl;
  cout << "  stl_all(is_even): " << stl_all(is_even, no_even) << endl;
  cout << "  stl_exists(is_even): " << stl_exists(is_even, no_even) << endl;
  cout << "  stl_none(is_even): " << stl_none(is_even, no_even) << endl;

  cout << endl;

  // ============================================================================
  // 6. Finding Elements
  // ============================================================================
  cout << "--- 6. Finding Elements ---" << endl;

  vector<int> find_data = {10, 20, 30, 40, 50};
  print_vec("Original", find_data);

  auto found = stl_find([](int x) { return x > 25; }, find_data);
  cout << "stl_find(x > 25): " << (found ? to_string(*found) : "not found") << endl;

  auto last_found = stl_find_last([](int x) { return x < 45; }, find_data);
  cout << "stl_find_last(x < 45): " << (last_found ? to_string(*last_found) : "not found") << endl;

  auto idx = stl_find_index([](int x) { return x == 30; }, find_data);
  cout << "stl_find_index(x == 30): " << (idx ? to_string(*idx) : "not found") << endl;

  cout << "stl_mem(30, data): " << stl_mem(30, find_data) << endl;
  cout << "stl_mem(99, data): " << stl_mem(99, find_data) << endl;

  cout << endl;

  // ============================================================================
  // 7. Counting
  // ============================================================================
  cout << "--- 7. Counting ---" << endl;

  vector<int> count_data = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
  print_vec("Original", count_data);

  cout << "stl_count(x -> x % 2 == 0): " << stl_count(is_even, count_data) << endl;
  cout << "stl_count_value(3): " << stl_count_value(3, count_data) << endl;

  cout << endl;

  // ============================================================================
  // 8. Take and Drop
  // ============================================================================
  cout << "--- 8. Take and Drop ---" << endl;

  vector<int> td = {1, 2, 3, 4, 5, 6, 7, 8};
  print_vec("Original", td);

  print_vec("stl_take(3)", stl_take(3, td));
  print_vec("stl_drop(3)", stl_drop(3, td));
  print_vec("stl_take_last(3)", stl_take_last(3, td));
  print_vec("stl_take_while(x < 5)", stl_take_while([](int x) { return x < 5; }, td));
  print_vec("stl_drop_while(x < 5)", stl_drop_while([](int x) { return x < 5; }, td));

  cout << endl;

  // ============================================================================
  // 9. Accessing Elements
  // ============================================================================
  cout << "--- 9. Accessing Elements ---" << endl;

  vector<int> access = {10, 20, 30, 40, 50};
  print_vec("Original", access);

  auto first = stl_first(access);
  cout << "stl_first: " << (first ? to_string(*first) : "empty") << endl;

  auto last = stl_last(access);
  cout << "stl_last: " << (last ? to_string(*last) : "empty") << endl;

  auto nth = stl_nth(2, access);
  cout << "stl_nth(2): " << (nth ? to_string(*nth) : "out of bounds") << endl;

  cout << endl;

  // ============================================================================
  // 10. Min, Max, Sum, Product
  // ============================================================================
  cout << "--- 10. Min, Max, Sum, Product ---" << endl;

  vector<int> mm = {3, 1, 4, 1, 5, 9, 2, 6};
  print_vec("Original", mm);

  cout << "stl_min: " << *stl_min(mm) << endl;
  cout << "stl_max: " << *stl_max(mm) << endl;

  auto [min_val, max_val] = *stl_min_max(mm);
  cout << "stl_min_max: (" << min_val << ", " << max_val << ")" << endl;

  cout << "stl_sum: " << stl_sum(mm) << endl;
  cout << "stl_product: " << stl_product(mm) << endl;

  vector<string> words = {"hello", "a", "wonderful", "world"};
  auto shortest = stl_min_by([](const string & s) { return s.length(); }, words);
  auto longest = stl_max_by([](const string & s) { return s.length(); }, words);
  cout << "Shortest word: " << *shortest << endl;
  cout << "Longest word: " << *longest << endl;

  cout << endl;

  // ============================================================================
  // 11. Partition
  // ============================================================================
  cout << "--- 11. Partition ---" << endl;

  vector<int> part = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  print_vec("Original", part);

  auto [evens_part, odds_part] = stl_partition(is_even, part);
  print_vec("Evens (matching)", evens_part);
  print_vec("Odds (non-matching)", odds_part);

  cout << endl;

  // ============================================================================
  // 12. Zip and Enumerate
  // ============================================================================
  cout << "--- 12. Zip and Enumerate ---" << endl;

  vector<int> keys = {1, 2, 3};
  vector<string> values = {"one", "two", "three"};

  print_vec("Keys", keys);
  print_vec("Values", values);

  auto zipped = stl_zip_to_pairs(keys, values);
  print_pairs("stl_zip_to_pairs", zipped);

  auto [unzipped_keys, unzipped_values] = stl_unzip_pairs(zipped);
  print_vec("Unzipped keys", unzipped_keys);
  print_vec("Unzipped values", unzipped_values);

  auto enumerated = stl_enumerate_to_pairs(values);
  print_pairs("stl_enumerate_to_pairs", enumerated);

  cout << endl;

  // ============================================================================
  // 13. Reverse and Sort
  // ============================================================================
  cout << "--- 13. Reverse and Sort ---" << endl;

  vector<int> unsorted = {3, 1, 4, 1, 5, 9, 2, 6};
  print_vec("Original", unsorted);

  print_vec("stl_reverse", stl_reverse(unsorted));
  print_vec("stl_sort", stl_sort(unsorted));
  print_vec("stl_sort_by(descending)", stl_sort_by([](int a, int b) { return a > b; }, unsorted));

  cout << endl;

  // ============================================================================
  // 14. Unique and Distinct
  // ============================================================================
  cout << "--- 14. Unique and Distinct ---" << endl;

  vector<int> with_dups = {1, 1, 2, 2, 2, 3, 3, 1, 1};
  print_vec("Original", with_dups);

  print_vec("stl_unique (consecutive)", stl_unique(with_dups));
  print_vec("stl_distinct (all)", stl_distinct(with_dups));

  cout << endl;

  // ============================================================================
  // 15. Concat and Flatten
  // ============================================================================
  cout << "--- 15. Concat and Flatten ---" << endl;

  vector<int> a = {1, 2, 3};
  vector<int> b = {4, 5, 6};
  print_vec("a", a);
  print_vec("b", b);
  print_vec("stl_concat(a, b)", stl_concat(a, b));

  vector<vector<int>> nested = {{1, 2}, {3, 4}, {5}};
  print_nested("Nested", nested);
  print_vec("stl_flatten", stl_flatten(nested));

  auto flat_mapped = stl_flat_map([](int x) {
    return vector<int>{x, x * 10};
  }, a);
  print_vec("stl_flat_map(x -> {x, x*10})", flat_mapped);

  cout << endl;

  // ============================================================================
  // 16. Grouping
  // ============================================================================
  cout << "--- 16. Grouping ---" << endl;

  vector<int> to_group = {1, 1, 2, 2, 2, 3, 1};
  print_vec("Original", to_group);

  auto grouped = stl_group(to_group);
  cout << "stl_group (consecutive):" << endl;
  for (const auto & g : grouped)
    {
      cout << "  ";
      print_vec("", g);
    }

  vector<string> words_to_group = {"apple", "ant", "banana", "bear", "apricot"};
  print_vec("Words", words_to_group);

  auto by_first_char = stl_group_by([](const string & s) { return s[0]; }, words_to_group);
  cout << "stl_group_by(first char):" << endl;
  for (const auto & [key, vals] : by_first_char)
    {
      cout << "  '" << key << "': ";
      print_vec("", vals);
    }

  cout << endl;

  // ============================================================================
  // 17. Tally (Frequency Count)
  // ============================================================================
  cout << "--- 17. Tally (Frequency Count) ---" << endl;

  vector<string> fruits = {"apple", "banana", "apple", "cherry", "banana", "apple"};
  print_vec("Original", fruits);

  auto tally = stl_tally(fruits);
  cout << "stl_tally:" << endl;
  for (const auto & [item, count] : tally)
    cout << "  \"" << item << "\": " << count << endl;

  cout << endl;

  // ============================================================================
  // 18. Sliding Window and Chunks
  // ============================================================================
  cout << "--- 18. Sliding Window and Chunks ---" << endl;

  vector<int> seq = {1, 2, 3, 4, 5};
  print_vec("Original", seq);

  auto windows = stl_sliding_window(3, seq);
  print_nested("stl_sliding_window(3)", windows);

  auto chunks = stl_chunks(2, seq);
  print_nested("stl_chunks(2)", chunks);

  cout << endl;

  // ============================================================================
  // 19. Intersperse, Split, Span
  // ============================================================================
  cout << "--- 19. Intersperse, Split, Span ---" << endl;

  vector<int> to_inter = {1, 2, 3};
  print_vec("Original", to_inter);

  print_vec("stl_intersperse(0)", stl_intersperse(0, to_inter));

  vector<int> to_split = {1, 2, 3, 4, 5};
  auto [first_part, second_part] = stl_split_at(2, to_split);
  print_vec("stl_split_at(2) first", first_part);
  print_vec("stl_split_at(2) second", second_part);

  auto [span_match, span_rest] = stl_span([](int x) { return x < 4; }, to_split);
  print_vec("stl_span(x < 4) matching", span_match);
  print_vec("stl_span(x < 4) rest", span_rest);

  cout << endl;

  // ============================================================================
  // 20. Init and Tail
  // ============================================================================
  cout << "--- 20. Init and Tail ---" << endl;

  vector<int> it = {1, 2, 3, 4, 5};
  print_vec("Original", it);

  print_vec("stl_init (all except last)", stl_init(it));
  print_vec("stl_tail (all except first)", stl_tail(it));

  cout << endl;

  // ============================================================================
  // 21. Combinatorics
  // ============================================================================
  cout << "--- 21. Combinatorics ---" << endl;

  vector<int> comb_set = {1, 2, 3};
  print_vec("Original", comb_set);

  auto perms = stl_permutations(comb_set);
  cout << "stl_permutations (" << perms.size() << " total):" << endl;
  for (const auto & p : perms)
    {
      cout << "  ";
      print_vec("", p);
    }

  auto combos = stl_combinations(2, comb_set);
  cout << "stl_combinations(2) (" << combos.size() << " total):" << endl;
  for (const auto & c : combos)
    {
      cout << "  ";
      print_vec("", c);
    }

  auto arrs = stl_arrangements(2, comb_set);
  cout << "stl_arrangements(2) (" << arrs.size() << " total):" << endl;
  for (const auto & arr : arrs)
    {
      cout << "  ";
      print_vec("", arr);
    }

  cout << endl;

  // ============================================================================
  // 22. Cartesian Product and Power Set
  // ============================================================================
  cout << "--- 22. Cartesian Product and Power Set ---" << endl;

  vector<vector<int>> sets = {{1, 2}, {3, 4}};
  cout << "Sets: {{1, 2}, {3, 4}}" << endl;

  auto cart = stl_cartesian_product(sets);
  print_nested("stl_cartesian_product", cart);

  vector<int> ps_set = {1, 2, 3};
  print_vec("Original", ps_set);

  auto power = stl_power_set(ps_set);
  cout << "stl_power_set (" << power.size() << " subsets):" << endl;
  for (const auto & s : power)
    {
      cout << "  {";
      for (size_t i = 0; i < s.size(); ++i)
        {
          cout << s[i];
          if (i < s.size() - 1) cout << ", ";
        }
      cout << "}" << endl;
    }

  cout << endl;

  // ============================================================================
  // 23. Works with Different STL Container Types
  // ============================================================================
  cout << "--- 23. Works with Different STL Container Types ---" << endl;

  // std::list
  list<int> my_list = {1, 2, 3, 4, 5};
  cout << "std::list<int>: {1, 2, 3, 4, 5}" << endl;

  auto list_squares = stl_map([](int x) { return x * x; }, my_list);
  print_vec("stl_map(x -> x²) on list", list_squares);

  int list_sum = stl_foldl(0, std::plus<int>{}, my_list);
  cout << "stl_foldl(0, +) on list: " << list_sum << endl;

  cout << endl;

  // std::set (ordered, unique elements)
  set<int> my_set = {5, 2, 8, 1, 9, 3};
  cout << "std::set<int>: {5, 2, 8, 1, 9, 3} -> ordered: {";
  bool first_set = true;
  for (int x : my_set)
    {
      if (!first_set) cout << ", ";
      cout << x;
      first_set = false;
    }
  cout << "}" << endl;

  auto set_doubled = stl_map([](int x) { return x * 2; }, my_set);
  print_vec("stl_map(x -> x*2) on set", set_doubled);

  auto set_filtered_gt3 = stl_filter([](int x) { return x > 3; }, my_set);
  print_vec("stl_filter(x > 3) on set", set_filtered_gt3);

  cout << "stl_sum on set: " << stl_sum(my_set) << endl;

  cout << endl;

  // ============================================================================
  // 23b. Works with Aleph Containers (using uni_* functions)
  // ============================================================================
  cout << "--- 23b. Aleph Containers (DynList, DynSetTree) ---" << endl;

  // DynList (Aleph singly-linked list)
  DynList<int> dyn_list = {10, 20, 30, 40, 50};
  cout << "DynList<int>: {10, 20, 30, 40, 50}" << endl;

  // uni_map/uni_filter return std::vector, works with both STL and Aleph containers
  auto dyn_list_squares = uni_map([](int x) { return x * x; }, dyn_list);
  print_vec("uni_map(x -> x²) on DynList", dyn_list_squares);

  auto dyn_list_filtered = uni_filter([](int x) { return x >= 30; }, dyn_list);
  print_vec("uni_filter(x >= 30) on DynList", dyn_list_filtered);

  int dyn_list_sum = uni_foldl(0, [](int a, int b) { return a + b; }, dyn_list);
  cout << "uni_foldl(0, +) on DynList: " << dyn_list_sum << endl;

  cout << endl;

  // DynSetTree (Aleph balanced tree set)
  DynSetTree<int> dyn_tree;
  dyn_tree.insert(15);
  dyn_tree.insert(5);
  dyn_tree.insert(25);
  dyn_tree.insert(10);
  dyn_tree.insert(20);
  cout << "DynSetTree<int>: {15, 5, 25, 10, 20} -> in-order: {";
  bool first_tree = true;
  for (auto it = dyn_tree.get_it(); it.has_curr(); it.next_ne())
    {
      if (!first_tree) cout << ", ";
      cout << it.get_curr();
      first_tree = false;
    }
  cout << "}" << endl;

  auto tree_mapped = uni_map([](int x) { return x + 100; }, dyn_tree);
  print_vec("uni_map(x -> x+100) on DynSetTree", tree_mapped);

  auto dyn_tree_even_filtered =
    uni_filter([](int x) { return x % 2 == 0; }, dyn_tree);
  print_vec("uni_filter(even) on DynSetTree", dyn_tree_even_filtered);

  bool tree_all_positive = uni_all([](int x) { return x > 0; }, dyn_tree);
  cout << "uni_all(x > 0) on DynSetTree: " << boolalpha << tree_all_positive << endl;

  cout << endl;

  // ============================================================================
  // 23c. Mixing STL and Aleph Results
  // ============================================================================
  cout << "--- 23c. Mixing STL and Aleph Results ---" << endl;

  // Create containers of different types
  vector<int> vec_a = {1, 2, 3, 4, 5};
  set<int> set_b = {10, 20, 30, 40, 50};
  DynList<int> dlist_c = {100, 200, 300, 400, 500};

  cout << "vector: {1, 2, 3, 4, 5}" << endl;
  cout << "set: {10, 20, 30, 40, 50}" << endl;
  cout << "DynList: {100, 200, 300, 400, 500}" << endl;
  cout << endl;

  // Apply same squaring operation to all
  auto vec_sq = stl_map([](int x) { return x * x; }, vec_a);
  auto set_sq = stl_map([](int x) { return x * x; }, set_b);

  // For DynList, uni_map already returns a std::vector
  auto dlist_sq = uni_map([](int x) { return x * x; }, dlist_c);

  print_vec("squares from vector", vec_sq);
  print_vec("squares from set", set_sq);
  print_vec("squares from DynList", dlist_sq);

  // Combine results using STL concat
  auto combined = stl_concat(stl_concat(vec_sq, set_sq), dlist_sq);
  print_vec("All squares combined", combined);

  cout << "Sum of all squares: " << stl_sum(combined) << endl;

  cout << endl;

  // ============================================================================
  // 23d. Unified API Example: DynList, set, vector, DynSetTree
  // ============================================================================
  cout << "--- 23d. Unified API with uni_*: DynList, set, vector, DynSetTree ---" << endl;

  // Create four different container types with similar data
  vector<int> uni_vec = {10, 5, 8, 3, 15, 7, 12};
  set<int> uni_set = {10, 5, 8, 3, 15, 7, 12};
  DynList<int> uni_dlist;
  for (int x : {10, 5, 8, 3, 15, 7, 12})
    uni_dlist.append(x);
  
  DynSetTree<int> uni_tree;
  for (int x : {10, 5, 8, 3, 15, 7, 12})
    uni_tree.insert(x);

  cout << "All containers initialized with: {10, 5, 8, 3, 15, 7, 12}" << endl;
  cout << "  vector preserves insertion order" << endl;
  cout << "  set sorts and removes duplicates" << endl;
  cout << "  DynList preserves insertion order" << endl;
  cout << "  DynSetTree sorts and stores unique values" << endl;
  cout << endl;

  // Same operation applied to all containers using uni_* functions
  auto is_greater_than_7 = [](int x) { return x > 7; };
  auto square = [](int x) { return x * x; };

  // 1. Filter: Keep only elements > 7
  auto vec_filtered = uni_filter(is_greater_than_7, uni_vec);
  auto set_filtered = uni_filter(is_greater_than_7, uni_set);
  auto dlist_filtered = uni_filter(is_greater_than_7, uni_dlist);
  auto tree_filtered = uni_filter(is_greater_than_7, uni_tree);

  cout << "After uni_filter(x > 7):" << endl;
  print_vec("  vector", vec_filtered);
  print_vec("  set", set_filtered);
  print_vec("  DynList", dlist_filtered);
  print_vec("  DynSetTree", tree_filtered);
  cout << endl;

  // 2. Map: Square all remaining elements
  auto vec_squared = uni_map(square, vec_filtered);
  auto set_squared = uni_map(square, set_filtered);
  auto dlist_squared = uni_map(square, dlist_filtered);
  auto tree_squared = uni_map(square, tree_filtered);

  cout << "After uni_map(x -> x²) on filtered results:" << endl;
  print_vec("  vector", vec_squared);
  print_vec("  set", set_squared);
  print_vec("  DynList", dlist_squared);
  print_vec("  DynSetTree", tree_squared);
  cout << endl;

  // 3. Fold: Sum all squared values
  auto sum_op = [](int acc, int x) { return acc + x; };
  int vec_sum = uni_foldl(0, sum_op, vec_squared);
  int set_sum = uni_foldl(0, sum_op, set_squared);
  int dlist_sum = uni_foldl(0, sum_op, dlist_squared);
  int tree_sum = uni_foldl(0, sum_op, tree_squared);

  cout << "After uni_foldl(0, +) on squared results:" << endl;
  cout << "  vector sum: " << vec_sum << endl;
  cout << "  set sum: " << set_sum << endl;
  cout << "  DynList sum: " << dlist_sum << endl;
  cout << "  DynSetTree sum: " << tree_sum << endl;
  cout << endl;

  // 4. Predicates: Check properties
  auto all_positive = [](int x) { return x > 0; };
  auto has_large = [](int x) { return x > 100; };

  cout << "Predicate tests using uni_all and uni_exists:" << endl;
  cout << "  vector - uni_all(x > 0): " << boolalpha << uni_all(all_positive, uni_vec) << endl;
  cout << "  set - uni_all(x > 0): " << uni_all(all_positive, uni_set) << endl;
  cout << "  DynList - uni_all(x > 0): " << uni_all(all_positive, uni_dlist) << endl;
  cout << "  DynSetTree - uni_all(x > 0): " << uni_all(all_positive, uni_tree) << endl;
  cout << endl;
  cout << "  vector - uni_exists(x > 100): " << uni_exists(has_large, vec_squared) << endl;
  cout << "  set - uni_exists(x > 100): " << uni_exists(has_large, set_squared) << endl;
  cout << "  DynList - uni_exists(x > 100): " << uni_exists(has_large, dlist_squared) << endl;
  cout << "  DynSetTree - uni_exists(x > 100): " << uni_exists(has_large, tree_squared) << endl;
  cout << endl;

  // 5. Min/Max operations
  cout << "Min/Max operations:" << endl;
  auto vec_min = uni_min(uni_vec);
  auto set_min = uni_min(uni_set);
  auto dlist_min = uni_min(uni_dlist);
  auto tree_min = uni_min(uni_tree);

  cout << "  vector min: " << (vec_min ? to_string(*vec_min) : "empty") << endl;
  cout << "  set min: " << (set_min ? to_string(*set_min) : "empty") << endl;
  cout << "  DynList min: " << (dlist_min ? to_string(*dlist_min) : "empty") << endl;
  cout << "  DynSetTree min: " << (tree_min ? to_string(*tree_min) : "empty") << endl;
  cout << endl;

  // 6. Take/Drop operations
  cout << "Take/Drop operations (first 3 elements):" << endl;
  auto vec_take = uni_take(3, uni_vec);
  auto set_take = uni_take(3, uni_set);
  auto dlist_take = uni_take(3, uni_dlist);
  auto tree_take = uni_take(3, uni_tree);

  print_vec("  uni_take(3) from vector", vec_take);
  print_vec("  uni_take(3) from set", set_take);
  print_vec("  uni_take(3) from DynList", dlist_take);
  print_vec("  uni_take(3) from DynSetTree", tree_take);
  cout << endl;

  // 7. Comparison across container types
  cout << "Cross-container comparisons:" << endl;
  cout << "  vector == DynList: " << boolalpha << uni_equal(uni_vec, uni_dlist) << endl;
  cout << "  set == DynSetTree: " << uni_equal(uni_set, uni_tree) << endl;
  cout << "  vector == set: " << uni_equal(uni_vec, uni_set) << " (order differs)" << endl;
  cout << endl;

  // 8. Unified pipeline demonstration
  cout << "Unified pipeline: filter -> map -> take -> sum" << endl;
  cout << "  Applied identically to all four container types" << endl;
  
  auto pipeline = [](const auto & container) {
    auto step1 = uni_filter([](int x) { return x % 2 != 0; }, container);  // odd numbers
    auto step2 = uni_map([](int x) { return x * 3; }, step1);              // triple them
    auto step3 = uni_take(2, step2);                                        // first 2
    return uni_sum(step3);                                                  // sum
  };

  cout << "  vector result: " << pipeline(uni_vec) << endl;
  cout << "  set result: " << pipeline(uni_set) << endl;
  cout << "  DynList result: " << pipeline(uni_dlist) << endl;
  cout << "  DynSetTree result: " << pipeline(uni_tree) << endl;
  cout << endl;

  cout << "Key insight: uni_* functions provide a single API that works" << endl;
  cout << "seamlessly with both STL containers (vector, set) and Aleph" << endl;
  cout << "containers (DynList, DynSetTree) without any code changes!" << endl;

  cout << endl;

  // ============================================================================
  // 24. Function Composition Example
  // ============================================================================
  cout << "--- 24. Function Composition Example ---" << endl;

  vector<int> raw = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5};
  print_vec("Raw data", raw);

  // Pipeline: distinct -> filter evens -> square -> sum
  auto distinct_vals = stl_distinct(raw);
  auto even_vals = stl_filter(is_even, distinct_vals);
  auto squared_vals = stl_map([](int x) { return x * x; }, even_vals);
  int final_sum = stl_foldl(0, std::plus<int>{}, squared_vals);

  cout << "Pipeline: distinct -> filter(even) -> map(square) -> sum" << endl;
  print_vec("  After distinct", distinct_vals);
  print_vec("  After filter(even)", even_vals);
  print_vec("  After map(square)", squared_vals);
  cout << "  Final sum: " << final_sum << endl;

  cout << endl;
  cout << "========================================" << endl;
  cout << "  All examples completed successfully!" << endl;
  cout << "========================================" << endl;

  return 0;
}
