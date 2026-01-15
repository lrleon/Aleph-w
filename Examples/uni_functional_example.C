/**
 * @file uni_functional_example.C
 * @brief Unified functional programming for mixed STL/Aleph containers
 *
 * This program demonstrates `ah-uni-functional.H` which provides functional
 * programming operations that work with **any** container type - STL or Aleph.
 * This unified interface eliminates the need to remember different APIs for
 * different container types.
 *
 * ## Key Feature: Universal Container Support
 *
 * ### The Problem
 *
 * Different container libraries have different APIs:
 * - **STL**: `std::transform`, `std::find_if`, etc.
 * - **Aleph-w**: `maps()`, `filter()`, etc.
 * - **Different syntax**: Hard to remember multiple APIs
 *
 * ### The Solution
 *
 * `ah-uni-functional.H` provides **unified functions** that:
 * - Work with **any** container type (STL or Aleph)
 * - **Automatically detect** container type
 * - Use **same API** regardless of container
 * - **Mix containers** in same operation
 *
 * ### Example
 *
 * ```cpp
 * // Works with STL containers
 * std::vector<int> vec = {1, 2, 3};
 * auto squared = uni_map(vec, [](int x) { return x * x; });
 *
 * // Works with Aleph containers
 * DynList<int> list;
 * list.append(1); list.append(2); list.append(3);
 * auto squared2 = uni_map(list, [](int x) { return x * x; });
 *
 * // Same function, different containers!
 * ```
 *
 * ## Functions Demonstrated
 *
 * ### Transformation
 * - **`uni_map()`**: Transform each element
 * - **`uni_mapi()`**: Transform with index
 * - **`uni_filter()`**: Keep elements satisfying predicate
 * - **`uni_filteri()`**: Filter with index
 *
 * ### Reduction
 * - **`uni_foldl()`**: Left fold (reduce from left)
 * - **`uni_reduce()`**: Alias for foldl
 * - **`uni_sum()`**: Sum all numeric elements
 * - **`uni_product()`**: Multiply all numeric elements
 *
 * ### Predicates
 * - **`uni_all()`**: All elements satisfy predicate?
 * - **`uni_exists()`**: At least one satisfies?
 * - **`uni_none()`**: None satisfy?
 * - **`uni_mem()`**: Membership test (contains?)
 *
 * ### Access
 * - **`uni_first()`**: Get first element
 * - **`uni_last()`**: Get last element
 * - **`uni_nth()`**: Get n-th element
 * - **`uni_find()`**: Find first satisfying predicate
 * - **`uni_find_mapi()`**: Find with index
 *
 * ### Slicing
 * - **`uni_take(n)`**: Take first n elements
 * - **`uni_drop(n)`**: Skip first n elements
 * - **`uni_take_while()`**: Take while predicate true
 * - **`uni_drop_while()`**: Drop while predicate true
 *
 * ### Statistics
 * - **`uni_min()`**: Find minimum element
 * - **`uni_max()`**: Find maximum element
 * - **`uni_min_max()`**: Find both min and max
 * - **`uni_count()`**: Count elements
 * - **`uni_length()`**: Get container size
 *
 * ## Comparison with Alternatives
 *
 * | Feature | STL | Aleph-w | Unified |
 * |---------|-----|---------|---------|
 * | Container support | STL only | Aleph only | **Both** |
 * | API consistency | Different | Different | **Same** |
 * | Mix containers | No | No | **Yes** |
 * | Type detection | Manual | Manual | **Automatic** |
 *
 * ## Use Cases
 *
 * ### Mixed Codebases
 * - **Legacy code**: Existing STL containers
 * - **New code**: Using Aleph-w containers
 * - **Unified API**: Same functions for both
 *
 * ### Library Integration
 * - **Third-party APIs**: May return STL containers
 * - **Your code**: Uses Aleph-w containers
 * - **Seamless**: Convert and process easily
 *
 * ### Migration
 * - **Gradual migration**: Move from STL to Aleph-w
 * - **No API changes**: Same functions work with both
 * - **Low risk**: Can mix containers during migration
 *
 * ## Performance Considerations
 *
 * - **Type detection**: Minimal overhead (compile-time)
 * - **Container operations**: Same as underlying container
 * - **No extra copies**: Operations are efficient
 *
 * ## Usage
 *
 * ```bash
 * # Run all demonstrations
 * ./uni_functional_example
 *
 * # Run specific section
 * ./uni_functional_example -s unified     # Unified API overview
 * ./uni_functional_example -s map         # Map/filter demo
 * ./uni_functional_example -s fold        # Fold operations
 * ./uni_functional_example -s predicates  # all/exists/none/mem, etc.
 * ./uni_functional_example -s access      # first/last/nth/find
 * ./uni_functional_example -s slicing     # take/drop/take_while/drop_while
 * ./uni_functional_example -s statistics  # min/max/min_max
 * ./uni_functional_example -s conversion  # STL <-> Aleph conversions
 * ./uni_functional_example -s practical   # Practical examples
 * ```
 *
 * @see ah-uni-functional.H Unified functional utilities
 * @see functional_example.C Aleph-w only functional (faster for Aleph containers)
 * @see stl_utils_example.C Container conversions (related)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>
#include <deque>

#include <tclap/CmdLine.h>

#include <htlist.H>
#include <tpl_dynArray.H>
#include <ah-uni-functional.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Helper functions
// =============================================================================

void print_section(const string& title)
{
  cout << "\n" << string(60, '=') << "\n";
  cout << "  " << title << "\n";
  cout << string(60, '=') << "\n\n";
}

void print_subsection(const string& title)
{
  cout << "\n--- " << title << " ---\n";
}

template <typename Container>
void print_stl(const string& label, const Container& c)
{
  cout << label << ": [";
  bool first = true;
  for (const auto& x : c)
  {
    if (not first) cout << ", ";
    cout << x;
    first = false;
  }
  cout << "]" << endl;
}

template <typename Container>
void print_aleph(const string& label, const Container& c)
{
  cout << label << ": [";
  bool first = true;
  for (auto it = c.get_it(); it.has_curr(); it.next())
  {
    if (not first) cout << ", ";
    cout << it.get_curr();
    first = false;
  }
  cout << "]" << endl;
}

// =============================================================================
// 1. Same API for Different Containers
// =============================================================================

void demo_unified_api()
{
  print_section("UNIFIED API - Same Function, Any Container");
  
  cout << "The key insight: ONE function works with ALL container types!\n\n";
  
  // Different container types
  vector<int> stl_vec = {1, 2, 3, 4, 5};
  list<int> stl_list = {1, 2, 3, 4, 5};
  DynList<int> aleph_list = {1, 2, 3, 4, 5};
  
  print_stl("std::vector", stl_vec);
  print_stl("std::list", stl_list);
  print_aleph("DynList", aleph_list);
  
  // Same uni_map works on all!
  print_subsection("uni_map() on all containers");
  
  auto vec_squares = uni_map([](int x) { return x * x; }, stl_vec);
  auto list_squares = uni_map([](int x) { return x * x; }, stl_list);
  auto aleph_squares = uni_map([](int x) { return x * x; }, aleph_list);
  
  print_stl("  vector squared", vec_squares);
  print_stl("  list squared", list_squares);
  print_stl("  DynList squared", aleph_squares);
  
  // Same uni_foldl works on all!
  print_subsection("uni_foldl() on all containers");
  
  int vec_sum = uni_foldl(0, [](int a, int b) { return a + b; }, stl_vec);
  int list_sum = uni_foldl(0, [](int a, int b) { return a + b; }, stl_list);
  int aleph_sum = uni_foldl(0, [](int a, int b) { return a + b; }, aleph_list);
  
  cout << "  vector sum:  " << vec_sum << endl;
  cout << "  list sum:    " << list_sum << endl;
  cout << "  DynList sum: " << aleph_sum << endl;
}

// =============================================================================
// 2. Map and Filter
// =============================================================================

void demo_map_filter()
{
  print_section("MAP AND FILTER");
  
  vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  print_stl("numbers", numbers);
  
  // uni_map
  print_subsection("uni_map()");
  auto doubled = uni_map([](int x) { return x * 2; }, numbers);
  print_stl("Doubled", doubled);
  
  auto as_strings = uni_map([](int x) { return "n" + to_string(x); }, numbers);
  print_stl("As strings", as_strings);
  
  // uni_mapi (with index)
  print_subsection("uni_mapi() - with index");
  auto indexed = uni_mapi([](size_t i, int x) {
    return "[" + to_string(i) + "]=" + to_string(x);
  }, numbers);
  print_stl("Indexed", indexed);
  
  // uni_filter
  print_subsection("uni_filter()");
  auto evens = uni_filter([](int x) { return x % 2 == 0; }, numbers);
  print_stl("Evens", evens);
  
  auto greater_5 = uni_filter([](int x) { return x > 5; }, numbers);
  print_stl("> 5", greater_5);
  
  // uni_filteri (with index)
  print_subsection("uni_filteri() - with index");
  auto even_positions = uni_filteri([](size_t i, int) { return i % 2 == 0; }, numbers);
  print_stl("Even positions", even_positions);
}

// =============================================================================
// 3. Fold and Reduce
// =============================================================================

void demo_fold_reduce()
{
  print_section("FOLD AND REDUCE");
  
  DynList<int> nums = {1, 2, 3, 4, 5};
  print_aleph("nums", nums);
  
  // uni_foldl
  print_subsection("uni_foldl()");
  int sum = uni_foldl(0, [](int a, int b) { return a + b; }, nums);
  cout << "Sum: " << sum << endl;
  
  int product = uni_foldl(1, [](int a, int b) { return a * b; }, nums);
  cout << "Product: " << product << endl;
  
  int max_val = uni_foldl(nums.get_first(), [](int a, int b) { 
    return a > b ? a : b; 
  }, nums);
  cout << "Max: " << max_val << endl;
  
  // String concatenation
  DynList<string> words = {"Hola", "desde", "Colombia"};
  print_aleph("words", words);
  
  string sentence = uni_foldl(string(""), [](const string& a, const string& b) {
    return a.empty() ? b : a + " " + b;
  }, words);
  cout << "Sentence: \"" << sentence << "\"" << endl;
  
  // uni_sum and uni_product
  print_subsection("uni_sum() / uni_product()");
  cout << "uni_sum: " << uni_sum(nums) << endl;
  cout << "uni_product: " << uni_product(nums) << endl;
}

// =============================================================================
// 4. Predicates
// =============================================================================

void demo_predicates()
{
  print_section("PREDICATES");
  
  vector<int> all_positive = {1, 2, 3, 4, 5};
  vector<int> mixed = {-1, 2, -3, 4, 5};
  vector<int> all_negative = {-1, -2, -3};
  
  print_stl("all_positive", all_positive);
  print_stl("mixed", mixed);
  print_stl("all_negative", all_negative);
  
  auto is_positive = [](int x) { return x > 0; };
  
  // uni_all
  print_subsection("uni_all()");
  cout << "All positive in all_positive? " 
       << (uni_all(is_positive, all_positive) ? "yes" : "no") << endl;
  cout << "All positive in mixed? " 
       << (uni_all(is_positive, mixed) ? "yes" : "no") << endl;
  
  // uni_exists / uni_any
  print_subsection("uni_exists() / uni_any()");
  cout << "Exists positive in mixed? " 
       << (uni_exists(is_positive, mixed) ? "yes" : "no") << endl;
  cout << "Exists positive in all_negative? " 
       << (uni_any(is_positive, all_negative) ? "yes" : "no") << endl;
  
  // uni_none
  print_subsection("uni_none()");
  cout << "None positive in all_negative? " 
       << (uni_none(is_positive, all_negative) ? "yes" : "no") << endl;
  
  // uni_mem
  print_subsection("uni_mem() - membership");
  cout << "3 in all_positive? " << (uni_mem(3, all_positive) ? "yes" : "no") << endl;
  cout << "10 in all_positive? " << (uni_mem(10, all_positive) ? "yes" : "no") << endl;
  
  // uni_count
  print_subsection("uni_count()");
  cout << "Count positive in mixed: " << uni_count(is_positive, mixed) << endl;
}

// =============================================================================
// 5. Access Functions
// =============================================================================

void demo_access()
{
  print_section("ACCESS FUNCTIONS");
  
  DynList<string> cities = {"Bogota", "Medellin", "Cali", "Barranquilla", "Cartagena"};
  print_aleph("cities", cities);
  
  // uni_first, uni_last
  print_subsection("uni_first() / uni_last()");
  auto first = uni_first(cities);
  auto last = uni_last(cities);
  
  if (first) cout << "First: " << *first << endl;
  if (last) cout << "Last: " << *last << endl;
  
  // uni_nth
  print_subsection("uni_nth()");
  auto third = uni_nth(2, cities);
  if (third) cout << "Third (index 2): " << *third << endl;
  
  auto tenth = uni_nth(10, cities);
  cout << "Tenth exists? " << (tenth ? "yes" : "no") << endl;
  
  // uni_find
  print_subsection("uni_find()");
  auto found = uni_find([](const string& s) { 
    return s.length() > 8; 
  }, cities);
  
  if (found) cout << "First with length > 8: " << *found << endl;
  
  // uni_length
  print_subsection("uni_length()");
  cout << "Length: " << uni_length(cities) << endl;
}

// =============================================================================
// 6. Slicing
// =============================================================================

void demo_slicing()
{
  print_section("SLICING");
  
  vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  print_stl("nums", nums);
  
  // uni_take
  print_subsection("uni_take()");
  auto first_3 = uni_take(3, nums);
  print_stl("First 3", first_3);
  
  // uni_drop
  print_subsection("uni_drop()");
  auto after_3 = uni_drop(3, nums);
  print_stl("After dropping 3", after_3);
  
  // uni_take_while
  print_subsection("uni_take_while()");
  auto less_than_5 = uni_take_while([](int x) { return x < 5; }, nums);
  print_stl("Take while < 5", less_than_5);
  
  // uni_drop_while
  print_subsection("uni_drop_while()");
  auto from_5 = uni_drop_while([](int x) { return x < 5; }, nums);
  print_stl("Drop while < 5", from_5);
  
  // uni_partition
  print_subsection("uni_partition()");
  auto [evens, odds] = uni_partition([](int x) { return x % 2 == 0; }, nums);
  print_stl("Evens", evens);
  print_stl("Odds", odds);
}

// =============================================================================
// 7. Statistics
// =============================================================================

void demo_statistics()
{
  print_section("STATISTICS");
  
  DynList<double> temps = {14.2, 24.5, 25.1, 28.3, 29.0, 18.5, 22.7};
  print_aleph("temperatures", temps);
  
  // uni_min, uni_max
  print_subsection("uni_min() / uni_max()");
  auto min_temp = uni_min(temps);
  auto max_temp = uni_max(temps);
  
  if (min_temp) cout << "Min: " << *min_temp << "°C" << endl;
  if (max_temp) cout << "Max: " << *max_temp << "°C" << endl;
  
  // uni_min_max (returns optional<pair>)
  print_subsection("uni_min_max()");
  auto min_max_opt = uni_min_max(temps);
  if (min_max_opt)
  {
    auto [min_v, max_v] = *min_max_opt;
    cout << "Range: " << min_v << "°C to " << max_v << "°C" << endl;
    cout << "Spread: " << (max_v - min_v) << "°C" << endl;
  }
  
  // Calculate average using fold
  print_subsection("Average (using uni_foldl)");
  double total = uni_foldl(0.0, [](double a, double b) { return a + b; }, temps);
  size_t count = uni_length(temps);
  cout << "Average: " << fixed << setprecision(2) << (total / count) << "°C" << endl;
}

// =============================================================================
// 8. Conversion
// =============================================================================

void demo_conversion()
{
  print_section("CONVERSION");
  
  DynList<int> aleph_list = {1, 2, 3, 4, 5};
  print_aleph("Aleph DynList", aleph_list);
  
  // uni_to_vector
  print_subsection("uni_to_vector()");
  vector<int> stl_vec = uni_to_vector(aleph_list);
  print_stl("Converted to std::vector", stl_vec);
  
  // Chain operations
  print_subsection("Chaining operations");
  auto result = uni_to_vector(
    uni_filter([](int x) { return x % 2 == 0; },
      uni_map([](int x) { return x * 10; }, aleph_list))
  );
  print_stl("map(*10) -> filter(even) -> vector", result);
}

// =============================================================================
// 9. Practical Example
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL: Sales Data Processing");
  
  // Sales from different systems (STL from database, Aleph from processing)
  vector<string> products = {"Cafe", "Panela", "Arroz", "Frijol", "Azucar"};
  DynList<double> prices = {25.0, 8.0, 12.0, 15.0, 10.0};
  vector<int> quantities = {100, 250, 180, 120, 200};
  
  print_stl("products (vector)", products);
  cout << "prices (DynList): [25.0, 8.0, 12.0, 15.0, 10.0]" << endl;
  print_stl("quantities (vector)", quantities);
  
  // Calculate totals
  print_subsection("Calculate total revenue");
  
  // First, get prices as vector
  auto prices_vec = uni_to_vector(prices);
  
  // Calculate revenue for each product
  vector<double> revenues;
  for (size_t i = 0; i < products.size(); i++)
    revenues.push_back(prices_vec[i] * quantities[i]);
  
  cout << "Revenue by product:\n";
  for (size_t i = 0; i < products.size(); i++)
    cout << "  " << setw(8) << left << products[i] 
         << ": $" << fixed << setprecision(2) << revenues[i] << endl;
  
  double total = uni_sum(revenues);
  cout << "\nTotal revenue: $" << total << endl;
  
  // Find most valuable product
  print_subsection("Find best seller");
  auto max_rev = uni_max(revenues);
  if (max_rev)
  {
    size_t idx = 0;
    for (size_t i = 0; i < revenues.size(); i++)
      if (revenues[i] == *max_rev) idx = i;
    cout << "Best seller: " << products[idx] << " ($" << *max_rev << ")" << endl;
  }
  
  // Filter high-value sales
  print_subsection("High-value products (> $1500)");
  for (size_t i = 0; i < products.size(); i++)
  {
    if (revenues[i] > 1500)
      cout << "  " << products[i] << ": $" << revenues[i] << endl;
  }
  
  // Statistics
  print_subsection("Statistics");
  cout << "Products: " << uni_length(products) << endl;
  cout << "Avg price: $" << (uni_sum(prices) / uni_length(prices)) << endl;
  cout << "Total units: " << uni_sum(quantities) << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Unified functional programming example for Aleph-w.\n"
      "Same functions work with STL and Aleph containers.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: unified, map, fold, predicates, "
      "access, slicing, statistics, conversion, practical, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "    ALEPH-W UNIFIED FUNCTIONAL PROGRAMMING EXAMPLE\n";
    cout << "    (Same API for STL and Aleph containers!)\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "unified")
      demo_unified_api();
    
    if (section == "all" or section == "map")
      demo_map_filter();
    
    if (section == "all" or section == "fold")
      demo_fold_reduce();
    
    if (section == "all" or section == "predicates")
      demo_predicates();
    
    if (section == "all" or section == "access")
      demo_access();
    
    if (section == "all" or section == "slicing")
      demo_slicing();
    
    if (section == "all" or section == "statistics")
      demo_statistics();
    
    if (section == "all" or section == "conversion")
      demo_conversion();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Unified functional programming demo completed!\n";
    cout << string(60, '=') << "\n\n";
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
    return 1;
  }
  catch (exception& e)
  {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
}

