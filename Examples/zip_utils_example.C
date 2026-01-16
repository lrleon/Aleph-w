/**
 * @file zip_utils_example.C
 * @brief Unified zip operations for mixed STL/Aleph containers
 *
 * This program demonstrates `ah-zip-utils.H` which provides zip operations
 * that work seamlessly with **both** STL and Aleph containers, and even
 * allows **mixing** them in the same operation. This unified approach
 * eliminates container-specific APIs.
 *
 * ## What is Zipping?
 *
 * Zipping combines multiple containers element-wise:
 * ```
 * Container 1: [a, b, c]
 * Container 2: [1, 2, 3]
 * Zipped:      [(a,1), (b,2), (c,3)]
 * ```
 *
 * **Key insight**: Process related data from multiple sources together.
 *
 * ## Key Feature: Container Interoperability
 *
 * ### The Problem
 *
 * Different zip libraries support different containers:
 * - **`ah-zip.H`**: Aleph containers only
 * - **STL**: No built-in zip (C++20 ranges add it)
 * - **Different APIs**: Hard to mix containers
 *
 * ### The Solution
 *
 * `ah-zip-utils.H` provides **unified zip** that:
 * - Works with **any** container type (STL or Aleph)
 * - **Automatically detects** container type
 * - **Mixes containers** in same operation
 * - Uses **same API** for all
 *
 * ### Example: Mixing Containers
 *
 * ```cpp
 * std::vector<string> names = {"Alice", "Bob", "Charlie"};
 * DynList<int> ages;
 * ages.append(25); ages.append(30); ages.append(35);
 *
 * // Mix STL and Aleph containers!
 * auto zipped = uni_zip(names, ages);
 * // Result: [("Alice", 25), ("Bob", 30), ("Charlie", 35)]
 * ```
 *
 * ## Functions Demonstrated
 *
 * ### Basic Operations
 * - **`uni_zip()`**: Create list of tuples from any containers
 * - **`uni_zip_it()`**: Get unified iterator for zipped containers
 *
 * ### Predicates
 * - **`uni_zip_all()`**: All tuples satisfy predicate?
 * - **`uni_zip_exists()`**: At least one tuple satisfies?
 * - **`uni_zip_none()`**: No tuple satisfies?
 *
 * ### Transformations
 * - **`uni_zip_map()`**: Transform tuples (apply function to each tuple)
 * - **`uni_zip_filter()`**: Keep tuples satisfying predicate
 * - **`uni_zip_mapi()`**: Transform with index
 * - **`uni_zip_filteri()`**: Filter with index
 *
 * ### Utilities
 * - **`uni_zip_take(n)`**: Take first n tuples
 * - **`uni_zip_drop(n)`**: Skip first n tuples
 * - **`uni_zip_min()`**: Find minimum tuple (by first element)
 * - **`uni_zip_max()`**: Find maximum tuple
 * - **`uni_unzip()`**: Split tuples back into separate containers
 *
 * ## Use Cases
 *
 * ### Data Processing
 * ```cpp
 * // Process names and scores together
 * auto results = uni_zip_map(names, scores,
 *   [](auto name, auto score) {
 *     return name + ": " + std::to_string(score);
 *   });
 * ```
 *
 * ### Parallel Processing
 * ```cpp
 * // Process multiple related datasets
 * uni_zip_for_each(prices, quantities, costs,
 *   [](auto price, auto qty, auto cost) {
 *     process_order(price, qty, cost);
 *   });
 * ```
 *
 * ### Data Validation
 * ```cpp
 * // Check if all pairs are valid
 * if (uni_zip_all(names, emails, is_valid_pair)) {
 *   process_data();
 * }
 * ```
 *
 * ## Comparison with Alternatives
 *
 * | Feature | ah-zip.H | STL (C++20) | ah-zip-utils.H |
 * |---------|----------|-------------|----------------|
 * | STL support | ❌ No | ✅ Yes | ✅ Yes |
 * | Aleph support | ✅ Yes | ❌ No | ✅ Yes |
 * | Mix containers | ❌ No | ❌ No | ✅ Yes |
 * | Unified API | ❌ No | ❌ No | ✅ Yes |
 *
 * ## Performance Considerations
 *
 * - **Type detection**: Minimal overhead (compile-time)
 * - **Iterator abstraction**: Small overhead for unified interface
 * - **Efficiency**: Operations are as efficient as underlying containers
 *
 * ## Usage
 *
 * ```bash
 * # Run all demonstrations
 * ./zip_utils_example
 *
 * # Run specific demo
 * ./zip_utils_example -s mixed       # Mixed container demo
 * ./zip_utils_example -s predicates  # Predicates on zipped data
 * ./zip_utils_example -s transform   # Transformation demo
 * ./zip_utils_example -s utilities   # Utility helpers
 * ./zip_utils_example -s advanced    # Advanced usage
 * ./zip_utils_example -s conversion  # Conversions and interop
 * ./zip_utils_example -s practical   # Practical examples
 *
 * # Show help
 * ./zip_utils_example --help
 * ```
 *
 * @see ah-zip-utils.H Unified zip utilities
 * @see zip_example.C Aleph-only zip operations (faster for Aleph containers)
 * @see uni_functional_example.C Unified functional (related)
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
#include <set>

#include <tclap/CmdLine.h>

#include <htlist.H>
#include <tpl_dynArray.H>
#include <ah-zip-utils.H>

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

// =============================================================================
// 1. Mixed Container Demo (The Key Feature!)
// =============================================================================

void demo_mixed_containers()
{
  print_section("MIXED CONTAINER OPERATIONS");
  
  cout << "The key feature of ah-zip-utils.H is mixing STL and Aleph containers!\n\n";
  
  // STL containers
  vector<int> stl_ids = {101, 102, 103, 104};
  list<string> stl_names = {"Juan", "Maria", "Carlos", "Ana"};
  
  // Aleph container
  DynList<double> aleph_scores = {85.5, 92.3, 78.0, 95.8};
  
  cout << "STL vector<int> ids:    [101, 102, 103, 104]" << endl;
  cout << "STL list<string> names: [Juan, Maria, Carlos, Ana]" << endl;
  cout << "Aleph DynList<double>:  [85.5, 92.3, 78.0, 95.8]" << endl;
  
  // Mix all three!
  print_subsection("uni_zip() - Mix STL and Aleph");
  cout << "Zipping vector + list + DynList together:\n";
  for (auto it = uni_zip_it(stl_ids, stl_names, aleph_scores); it.has_curr(); it.next())
  {
    auto [id, name, score] = it.get_curr();
    cout << "  ID " << id << ": " << setw(8) << left << name 
         << " Score: " << fixed << setprecision(1) << score << endl;
  }
  
  // Using uni_zip_it for lazy iteration
  print_subsection("uni_zip_it() - Lazy iteration");
  cout << "Using unified iterator:\n";
  for (auto it = uni_zip_it(stl_ids, stl_names); it.has_curr(); it.next())
  {
    auto [id, name] = it.get_curr();
    cout << "  " << id << " -> " << name << endl;
  }
}

// =============================================================================
// 2. Predicates with Mixed Containers
// =============================================================================

void demo_predicates()
{
  print_section("PREDICATES WITH MIXED CONTAINERS");
  
  vector<int> stl_quantities = {10, 25, 5, 30};
  DynList<double> aleph_prices = {100.0, 50.0, 200.0, 25.0};
  
  cout << "STL vector quantities: [10, 25, 5, 30]" << endl;
  cout << "Aleph DynList prices:  [100, 50, 200, 25]" << endl;
  
  // uni_zip_all - all tuples satisfy predicate
  print_subsection("uni_zip_all()");
  bool all_valuable = uni_zip_all([](const auto& t) {
    return get<0>(t) * get<1>(t) > 100;  // qty * price > 100
  }, stl_quantities, aleph_prices);
  cout << "All orders > $100? " << (all_valuable ? "yes" : "no") << endl;
  
  // uni_zip_exists
  print_subsection("uni_zip_exists()");
  bool has_expensive = uni_zip_exists([](const auto& t) {
    return get<0>(t) * get<1>(t) > 500;
  }, stl_quantities, aleph_prices);
  cout << "Exists order > $500? " << (has_expensive ? "yes" : "no") << endl;
  
  // uni_zip_none
  print_subsection("uni_zip_none()");
  bool none_cheap = uni_zip_none([](const auto& t) {
    return get<0>(t) * get<1>(t) < 50;
  }, stl_quantities, aleph_prices);
  cout << "No orders < $50? " << (none_cheap ? "yes" : "no") << endl;
  
  // Show all orders
  cout << "\nAll orders:\n";
  uni_zip_for_each([](const auto& t) {
    int qty = get<0>(t);
    double price = get<1>(t);
    cout << "  " << qty << " x $" << price << " = $" << (qty * price) << endl;
  }, stl_quantities, aleph_prices);
}

// =============================================================================
// 3. Transformations
// =============================================================================

void demo_transformations()
{
  print_section("TRANSFORMATIONS");
  
  // Colombian departments and their coffee production
  vector<string> stl_depts = {"Huila", "Nariño", "Cauca", "Tolima"};
  DynList<int> aleph_production = {150, 85, 72, 65};  // thousands of bags
  
  cout << "STL vector depts:       [Huila, Nariño, Cauca, Tolima]" << endl;
  cout << "Aleph DynList bags(k):  [150, 85, 72, 65]" << endl;
  
  // uni_zip_map (returns std::vector)
  print_subsection("uni_zip_map()");
  auto reports = uni_zip_map([](const auto& t) {
    return get<0>(t) + ": " + to_string(get<1>(t)) + "k bags";
  }, stl_depts, aleph_production);
  
  cout << "Production reports:\n";
  for (const auto& r : reports)
    cout << "  " << r << endl;
  
  // uni_zip_filter (returns std::vector)
  print_subsection("uni_zip_filter()");
  auto major_producers = uni_zip_filter([](const auto& t) {
    return get<1>(t) >= 80;  // >= 80k bags
  }, stl_depts, aleph_production);
  
  cout << "Major producers (>= 80k bags):\n";
  for (const auto& [dept, bags] : major_producers)
    cout << "  " << dept << ": " << bags << "k bags" << endl;
  
  // uni_zip_mapi (with index, returns std::vector)
  print_subsection("uni_zip_mapi() - with index");
  auto ranked = uni_zip_mapi([](size_t idx, const auto& t) {
    return "#" + to_string(idx + 1) + " " + get<0>(t);
  }, stl_depts, aleph_production);
  
  cout << "Ranked by position:\n";
  for (const auto& r : ranked)
    cout << "  " << r << endl;
}

// =============================================================================
// 4. Utilities
// =============================================================================

void demo_utilities()
{
  print_section("UTILITIES");
  
  list<string> stl_cities = {"Bogota", "Medellin", "Cali", "Barranquilla", "Cartagena"};
  DynList<int> aleph_temps = {14, 24, 25, 28, 29};  // avg temp in Celsius
  
  cout << "STL list cities:        [Bogota, Medellin, Cali, Barranquilla, Cartagena]" << endl;
  cout << "Aleph DynList temps(C): [14, 24, 25, 28, 29]" << endl;
  
  // uni_zip_take (returns std::vector)
  print_subsection("uni_zip_take()");
  auto top3 = uni_zip_take(3, stl_cities, aleph_temps);
  cout << "First 3 cities:\n";
  for (const auto& [city, temp] : top3)
    cout << "  " << city << ": " << temp << "°C" << endl;
  
  // uni_zip_drop (returns std::vector)
  print_subsection("uni_zip_drop()");
  auto rest = uni_zip_drop(3, stl_cities, aleph_temps);
  cout << "Remaining cities:\n";
  for (const auto& [city, temp] : rest)
    cout << "  " << city << ": " << temp << "°C" << endl;
  
  // uni_zip_min and uni_zip_max (return std::optional)
  print_subsection("uni_zip_min() / uni_zip_max()");
  
  // Find city with min temperature
  auto min_opt = uni_zip_min(stl_cities, aleph_temps);
  if (min_opt)
  {
    auto [city, temp] = *min_opt;
    cout << "Coldest: " << city << " at " << temp << "°C" << endl;
  }
  
  auto max_opt = uni_zip_max(stl_cities, aleph_temps);
  if (max_opt)
  {
    auto [city, temp] = *max_opt;
    cout << "Hottest: " << city << " at " << temp << "°C" << endl;
  }
}

// =============================================================================
// 5. Advanced Operations
// =============================================================================

void demo_advanced()
{
  print_section("ADVANCED OPERATIONS");
  
  vector<int> vec_a = {1, 2, 3, 4, 5};
  DynList<int> list_b = {10, 20, 30, 40, 50};
  
  cout << "STL vector a:   [1, 2, 3, 4, 5]" << endl;
  cout << "Aleph DynList b: [10, 20, 30, 40, 50]" << endl;
  
  // uni_zip_scan_left - running computation (returns std::vector)
  print_subsection("uni_zip_scan_left()");
  auto running_sums = uni_zip_scan_left(0, [](int acc, const auto& t) {
    return acc + get<0>(t) + get<1>(t);
  }, vec_a, list_b);
  
  cout << "Running sum of (a[i] + b[i]):\n  ";
  for (int val : running_sums)
    cout << val << " ";
  cout << endl;
  
  // uni_zip_equal_length
  print_subsection("uni_zip_equal_length()");
  vector<int> short_vec = {1, 2};
  cout << "vec_a and list_b same length? " 
       << (uni_zip_equal_length(vec_a, list_b) ? "yes" : "no") << endl;
  cout << "vec_a and short_vec same length? " 
       << (uni_zip_equal_length(vec_a, short_vec) ? "yes" : "no") << endl;
  
  // uni_zip returns a view that can be iterated
  print_subsection("uni_zip() - iterate view");
  cout << "Zipped pairs: ";
  for (auto it = uni_zip_it(vec_a, list_b); it.has_curr(); it.next())
  {
    auto [x, y] = it.get_curr();
    cout << "(" << x << "," << y << ") ";
  }
  cout << endl;
}

// =============================================================================
// 6. STL to Aleph Conversion
// =============================================================================

void demo_conversion()
{
  print_section("STL TO ALEPH CONVERSION");
  
  vector<string> stl_products = {"Cafe", "Panela", "Arroz", "Frijol"};
  vector<double> stl_prices = {25.0, 8.0, 12.0, 15.0};
  
  cout << "STL vectors:\n";
  cout << "  products: [Cafe, Panela, Arroz, Frijol]" << endl;
  cout << "  prices:   [25.0, 8.0, 12.0, 15.0]" << endl;
  
  // Convert to DynList of tuples
  print_subsection("uni_zip_to_dynlist()");
  DynList<tuple<string, double>> aleph_result = uni_zip_to_dynlist(stl_products, stl_prices);
  
  cout << "Converted to DynList<tuple>:\n";
  for (auto it = aleph_result.get_it(); it.has_curr(); it.next())
  {
    auto [product, price] = it.get_curr();
    cout << "  " << product << ": $" << price << endl;
  }
  
  // Convert to std::vector
  print_subsection("uni_zip_to_vector()");
  DynList<int> aleph_ids = {1, 2, 3, 4};
  auto stl_result = uni_zip_to_vector(aleph_ids, stl_prices);
  
  cout << "Converted to std::vector<tuple>:\n";
  for (const auto& [id, price] : stl_result)
    cout << "  ID " << id << ": $" << price << endl;
}

// =============================================================================
// 7. Practical Example
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL: Student Grade Analysis");
  
  // Data from different sources (simulating real-world scenario)
  // STL container from database
  vector<string> db_students = {"Sofia", "Andres", "Valentina", "Santiago", "Isabella"};
  // Aleph container from processing
  DynList<double> processed_math = {4.2, 3.8, 4.5, 3.5, 4.8};
  // Another STL container
  list<double> spanish_grades = {4.0, 4.2, 4.3, 3.9, 4.6};
  
  cout << "Data from mixed sources:\n";
  cout << "  DB (vector):      [Sofia, Andres, Valentina, Santiago, Isabella]" << endl;
  cout << "  Processed (DynList): [4.2, 3.8, 4.5, 3.5, 4.8] (Math)" << endl;
  cout << "  Input (list):     [4.0, 4.2, 4.3, 3.9, 4.6] (Spanish)" << endl;
  
  // Calculate averages (uni_zip_map returns std::vector)
  print_subsection("Calculate averages");
  auto averages = uni_zip_map([](const auto& t) {
    string name = get<0>(t);
    double math = get<1>(t);
    double spanish = get<2>(t);
    double avg = (math + spanish) / 2.0;
    return make_tuple(name, avg);
  }, db_students, processed_math, spanish_grades);
  
  cout << "Student averages:\n";
  for (const auto& [name, avg] : averages)  // std::vector iteration
  {
    cout << "  " << setw(10) << left << name << ": " 
         << fixed << setprecision(2) << avg << endl;
  }
  
  // Find honors students (avg >= 4.3)
  print_subsection("Honors students (avg >= 4.3)");
  auto honors = uni_zip_filter([](const auto& t) {
    double math = get<1>(t);
    double spanish = get<2>(t);
    return (math + spanish) / 2.0 >= 4.3;
  }, db_students, processed_math, spanish_grades);
  
  cout << "Honors list:\n";
  for (const auto& t : honors)  // std::vector iteration
  {
    auto [name, math, spanish] = t;
    double avg = (math + spanish) / 2.0;
    cout << "  " << name << " (avg: " << fixed << setprecision(2) << avg << ")" << endl;
  }
  
  // Count passing students
  print_subsection("Statistics");
  size_t total = 0;
  size_t passing = 0;
  uni_zip_for_each([&](const auto& t) {
    total++;
    if ((get<1>(t) + get<2>(t)) / 2.0 >= 3.0)
      passing++;
  }, db_students, processed_math, spanish_grades);
  
  cout << "Total students: " << total << endl;
  cout << "Passing (avg >= 3.0): " << passing << endl;
  cout << "Pass rate: " << (100.0 * passing / total) << "%" << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Unified zip utilities example for Aleph-w.\n"
      "Demonstrates mixing STL and Aleph containers in zip operations.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: mixed, predicates, transform, "
      "utilities, advanced, conversion, practical, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "    ALEPH-W UNIFIED ZIP UTILITIES EXAMPLE\n";
    cout << "    (Mix STL and Aleph containers!)\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "mixed")
      demo_mixed_containers();
    
    if (section == "all" or section == "predicates")
      demo_predicates();
    
    if (section == "all" or section == "transform")
      demo_transformations();
    
    if (section == "all" or section == "utilities")
      demo_utilities();
    
    if (section == "all" or section == "advanced")
      demo_advanced();
    
    if (section == "all" or section == "conversion")
      demo_conversion();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Unified zip utilities demo completed!\n";
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

