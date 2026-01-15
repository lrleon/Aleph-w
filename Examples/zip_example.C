/**
 * @file zip_example.C
 * @brief Comprehensive example of zip operations in Aleph-w
 *
 * This program demonstrates zip operations from `ah-zip.H`, which provide
 * powerful tools for working with multiple containers simultaneously. Zip
 * operations are inspired by Python's `zip()` function and are essential
 * for parallel processing of related data.
 *
 * ## What is Zipping?
 *
 * Zipping combines multiple containers element-wise, creating tuples:
 * ```
 * Container 1: [a, b, c]
 * Container 2: [1, 2, 3]
 * Zipped:      [(a,1), (b,2), (c,3)]
 * ```
 *
 * **Key insight**: Process related data from multiple sources together,
 * maintaining correspondence between elements.
 *
## Features Demonstrated
 *
### Basic Zip Iteration
 *
 * Iterate multiple containers simultaneously:
 * - **`ZipIterator`**: Iterate 2+ containers in lockstep
 * - **`EnumZipIterator`**: Zip with enumeration index (i, elem₁, elem₂, ...)
 *
 * **Use case**: Process related data from multiple sources together
 *
### Tuple Creation
 *
 * Create and manipulate tuples:
 * - **`t_zip()`**: Create list of tuples from containers
 * - **`t_unzip()`**: Split tuple list back into separate containers
 * - **`t_enum_zip()`**: Create tuples with index: (i, elem₁, elem₂)
 *
### Zip Functional Operations
 *
 * Apply functional operations to zipped data:
 * - **`zip_map()`**: Transform tuples (apply function to each tuple)
 * - **`zip_filter()`**: Keep tuples satisfying predicate
 * - **`zip_take(n)`**: Take first n tuples
 * - **`zip_drop(n)`**: Skip first n tuples
 * - **`zip_partition(pred)`**: Split into two groups by predicate
 *
### STL Compatibility
 *
 * - **`std_zip()`**: Zip STL containers (std::vector, std::list, etc.)
 * - Works seamlessly with standard library containers
 *
## Common Use Cases
 *
### Parallel Processing
 * ```cpp
 * // Process names and ages together
 * auto zipped = zip(names, ages);
 * zip_for_each(zipped, [](auto name, auto age) {
 *   cout << name << " is " << age << " years old\n";
 * });
 * ```
 *
### Data Transformation
 * ```cpp
 * // Combine and transform data
 * auto result = zip_map(prices, quantities, 
 *   [](auto price, auto qty) { return price * qty; });
 * ```
 *
### Filtering Related Data
 * ```cpp
 * // Keep only valid pairs
 * auto valid = zip_filter(names, scores,
 *   [](auto name, auto score) { return score >= 60; });
 * ```
 *
### Indexed Operations
 * ```cpp
 * // Process with index
 * enum_zip_for_each(data, [](size_t i, auto elem) {
 *   cout << "Element " << i << ": " << elem << "\n";
 * });
 * ```
 *
## Advantages
 *
 * ✅ **Type safety**: Compile-time checking of container compatibility
 * ✅ **Efficiency**: Single pass through containers
 * ✅ **Readability**: Clear intent (process related data together)
 * ✅ **Composability**: Works with other functional operations
 *
## Comparison with Manual Loops
 *
 * **Manual approach**:
 * ```cpp
 * for (size_t i = 0; i < min(names.size(), ages.size()); i++) {
 *   process(names[i], ages[i]);
 * }
 * ```
 *
 * **Zip approach**:
 * ```cpp
 * zip_for_each(names, ages, process);
 * ```
 *
 * More concise, safer (no index errors), clearer intent!
 *
## Usage Examples
 *
 * ```bash
 * # Run all zip demonstrations
 * ./zip_example
 *
 * # Run specific section
 * ./zip_example -s basic      # Basic zip operations
 * ./zip_example -s functional # Functional zip operations
 * ```
 *
 * @see ah-zip.H Main zip operations header (Aleph containers only)
 * @see zip_utils_example.C Unified zip (works with STL and Aleph)
 * @see functional_example.C General functional programming
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <tclap/CmdLine.h>

#include <htlist.H>
#include <tpl_dynDlist.H>
#include <ah-zip.H>
#include <ahFunctional.H>

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
void print_list(const string& label, const Container& c)
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
// 1. Basic Zip Iteration
// =============================================================================

void demo_basic_zip()
{
  print_section("BASIC ZIP ITERATION");
  
  DynList<int> numbers = {1, 2, 3, 4, 5};
  DynList<string> names = {"uno", "dos", "tres", "cuatro", "cinco"};
  DynList<char> letters = {'a', 'b', 'c', 'd', 'e'};
  
  print_list("numbers", numbers);
  print_list("names", names);
  print_list("letters", letters);
  
  // ZipIterator with two containers
  print_subsection("ZipIterator (2 containers)");
  cout << "Iterating numbers and names together:\n";
  for (ZipIterator it(numbers, names); it.has_curr(); it.next())
  {
    auto [num, name] = it.get_curr();
    cout << "  " << num << " -> " << name << endl;
  }
  
  // ZipIterator with three containers
  print_subsection("ZipIterator (3 containers)");
  cout << "Iterating numbers, names, and letters:\n";
  for (ZipIterator it(numbers, names, letters); it.has_curr(); it.next())
  {
    auto [num, name, letter] = it.get_curr();
    cout << "  " << num << " (" << name << ") [" << letter << "]" << endl;
  }
  
  // Different length containers
  print_subsection("Handling Different Lengths");
  DynList<int> short_list = {10, 20};
  print_list("short_list", short_list);
  
  cout << "Zipping numbers with short_list (stops at shorter):\n";
  for (ZipIterator it(numbers, short_list); it.has_curr(); it.next())
  {
    auto [a, b] = it.get_curr();
    cout << "  " << a << ", " << b << endl;
  }
}

// =============================================================================
// 2. Enumerated Zip
// =============================================================================

void demo_enum_zip()
{
  print_section("ENUMERATED ZIP");
  
  DynList<string> fruits = {"manzana", "pera", "uva", "mango"};
  DynList<double> prices = {1.50, 0.80, 2.30, 1.20};
  
  print_list("fruits", fruits);
  cout << "prices: [1.50, 0.80, 2.30, 1.20]" << endl;
  
  // EnumZipIterator
  print_subsection("EnumZipIterator");
  cout << "Iterating with index:\n";
  for (EnumZipIterator it(fruits, prices); it.has_curr(); it.next())
  {
    auto [idx, fruit, price] = it.get_curr();
    cout << "  [" << idx << "] " << fruit << ": $" << fixed << setprecision(2) << price << endl;
  }
  
  // t_enum_zip - creates list of tuples with index
  print_subsection("t_enum_zip()");
  auto enumerated = t_enum_zip(fruits, prices);
  cout << "Created list of (index, fruit, price) tuples:\n";
  for (auto it = enumerated.get_it(); it.has_curr(); it.next())
  {
    auto [idx, fruit, price] = it.get_curr();
    cout << "  (" << idx << ", " << fruit << ", $" << price << ")" << endl;
  }
}

// =============================================================================
// 3. Tuple List Operations
// =============================================================================

void demo_tuple_operations()
{
  print_section("TUPLE LIST OPERATIONS");
  
  DynList<string> products = {"cafe", "panela", "arroz"};
  DynList<double> prices = {15.0, 5.0, 8.0};
  DynList<int> stock = {100, 250, 180};
  
  print_list("products", products);
  cout << "prices: [15.0, 5.0, 8.0]" << endl;
  print_list("stock", stock);
  
  // t_zip - create list of tuples
  print_subsection("t_zip()");
  auto inventory = t_zip(products, prices, stock);
  cout << "Inventory as list of tuples:\n";
  for (auto it = inventory.get_it(); it.has_curr(); it.next())
  {
    auto [prod, price, qty] = it.get_curr();
    cout << "  (" << prod << ", $" << price << ", " << qty << " units)" << endl;
  }
  
  // t_unzip - split tuple list back
  print_subsection("t_unzip()");
  auto [prods_back, prices_back, stock_back] = t_unzip(inventory);
  cout << "Unzipped back to separate lists:\n";
  print_list("  products", prods_back);
  cout << "  prices: [";
  bool first = true;
  for (auto it = prices_back.get_it(); it.has_curr(); it.next())
  {
    if (not first) cout << ", ";
    cout << it.get_curr();
    first = false;
  }
  cout << "]" << endl;
  print_list("  stock", stock_back);
}

// =============================================================================
// 4. Zip Transformation
// =============================================================================

void demo_zip_transformation()
{
  print_section("ZIP TRANSFORMATION");
  
  DynList<int> quantities = {5, 3, 8, 2};
  DynList<double> unit_prices = {10.0, 25.0, 5.0, 100.0};
  
  print_list("quantities", quantities);
  cout << "unit_prices: [10.0, 25.0, 5.0, 100.0]" << endl;
  
  // zip_map - transform pairs (lambda receives tuple)
  print_subsection("zip_map()");
  auto totals = zip_map([](const auto& t) {
    return get<0>(t) * get<1>(t);
  }, quantities, unit_prices);
  
  cout << "Totals (qty * price):\n";
  for (auto it = totals.get_it(); it.has_curr(); it.next())
    cout << "  $" << fixed << setprecision(2) << it.get_curr() << endl;
  
  // zip_filter - keep pairs satisfying predicate (lambda receives tuple)
  print_subsection("zip_filter()");
  auto expensive = zip_filter([](const auto& t) {
    return get<0>(t) * get<1>(t) > 30;
  }, quantities, unit_prices);
  
  cout << "Orders > $30:\n";
  for (auto it = expensive.get_it(); it.has_curr(); it.next())
  {
    auto [qty, price] = it.get_curr();
    cout << "  " << qty << " x $" << price << " = $" << (qty * price) << endl;
  }
  
  // Manual map with ZipIterator
  print_subsection("Manual transformation with ZipIterator");
  DynList<string> names = {"laptop", "mouse", "cable", "monitor"};
  cout << "Product descriptions:\n";
  for (ZipIterator it(names, quantities, unit_prices); it.has_curr(); it.next())
  {
    auto [name, qty, price] = it.get_curr();
    cout << "  " << name << " (" << qty << " @ $" << price << ")" << endl;
  }
}

// =============================================================================
// 5. Zip Utilities
// =============================================================================

void demo_zip_utilities()
{
  print_section("ZIP UTILITIES");
  
  DynList<string> cities = {"Bogota", "Medellin", "Cali", "Barranquilla", "Cartagena"};
  DynList<int> populations = {7400, 2500, 2200, 1200, 1000};  // in thousands
  
  print_list("cities", cities);
  print_list("populations (thousands)", populations);
  
  // zip_take - take first n
  print_subsection("zip_take()");
  auto top3 = zip_take(3, cities, populations);
  cout << "Top 3 cities:\n";
  for (auto it = top3.get_it(); it.has_curr(); it.next())
  {
    auto [city, pop] = it.get_curr();
    cout << "  " << city << ": " << pop << "k" << endl;
  }
  
  // zip_drop - drop first n
  print_subsection("zip_drop()");
  auto rest = zip_drop(3, cities, populations);
  cout << "Remaining cities (after dropping 3):\n";
  for (auto it = rest.get_it(); it.has_curr(); it.next())
  {
    auto [city, pop] = it.get_curr();
    cout << "  " << city << ": " << pop << "k" << endl;
  }
  
  // zip_take_while (lambda receives tuple)
  print_subsection("zip_take_while()");
  auto big_cities = zip_take_while([](const auto& t) {
    return get<1>(t) > 1500;
  }, cities, populations);
  
  cout << "Cities with population > 1500k (prefix):\n";
  for (auto it = big_cities.get_it(); it.has_curr(); it.next())
  {
    auto [city, pop] = it.get_curr();
    cout << "  " << city << ": " << pop << "k" << endl;
  }
  
  // Manual partition using filter
  print_subsection("Manual partition using filter");
  auto all_tuples = t_zip(cities, populations);
  
  auto large = all_tuples.filter([](const auto& t) {
    return get<1>(t) >= 2000;
  });
  auto small_cities = all_tuples.filter([](const auto& t) {
    return get<1>(t) < 2000;
  });
  
  cout << "Large cities (>= 2M):\n";
  for (auto it = large.get_it(); it.has_curr(); it.next())
  {
    auto [city, pop] = it.get_curr();
    cout << "  " << city << ": " << pop << "k" << endl;
  }
  cout << "Smaller cities (< 2M):\n";
  for (auto it = small_cities.get_it(); it.has_curr(); it.next())
  {
    auto [city, pop] = it.get_curr();
    cout << "  " << city << ": " << pop << "k" << endl;
  }
}

// =============================================================================
// 6. STL Compatibility
// =============================================================================

void demo_stl_compatibility()
{
  print_section("STL COMPATIBILITY");
  
  vector<int> vec1 = {1, 2, 3, 4};
  vector<string> vec2 = {"a", "b", "c", "d"};
  
  cout << "vec1 (std::vector): [1, 2, 3, 4]" << endl;
  cout << "vec2 (std::vector): [a, b, c, d]" << endl;
  
  // std_zip - zip two STL containers (returns std::vector<pair>)
  print_subsection("std_zip()");
  auto zipped = std_zip(vec1, vec2);
  cout << "Zipped STL vectors (returns std::vector):\n";
  for (const auto& [num, letter] : zipped)
  {
    cout << "  (" << num << ", " << letter << ")" << endl;
  }
  
  // tzip_std - zip multiple STL containers (returns std::vector<tuple>)
  print_subsection("tzip_std()");
  vector<double> vec3 = {1.1, 2.2, 3.3, 4.4};
  cout << "vec3 (std::vector): [1.1, 2.2, 3.3, 4.4]" << endl;
  
  auto triple_zip = tzip_std(vec1, vec2, vec3);
  cout << "Triple-zipped (returns std::vector):\n";
  for (const auto& t : triple_zip)
  {
    auto [num, letter, dbl] = t;
    cout << "  (" << num << ", " << letter << ", " << dbl << ")" << endl;
  }
}

// =============================================================================
// 7. Length Checking
// =============================================================================

void demo_length_checking()
{
  print_section("LENGTH CHECKING");
  
  DynList<int> list1 = {1, 2, 3};
  DynList<int> list2 = {10, 20, 30};
  DynList<int> list3 = {100, 200};  // Different length!
  
  print_list("list1", list1);
  print_list("list2", list2);
  print_list("list3 (shorter!)", list3);
  
  // equal_length check
  print_subsection("equal_length()");
  cout << "list1 and list2 equal length? " 
       << (equal_length(list1, list2) ? "yes" : "no") << endl;
  cout << "list1 and list3 equal length? " 
       << (equal_length(list1, list3) ? "yes" : "no") << endl;
  
  // Regular zip stops at shorter
  print_subsection("Regular zip (stops at shorter)");
  cout << "Zipping list1 and list3:\n";
  for (ZipIterator it(list1, list3); it.has_curr(); it.next())
  {
    auto [a, b] = it.get_curr();
    cout << "  " << a << " + " << b << " = " << (a + b) << endl;
  }
  cout << "  (Only 2 pairs produced)\n";
  
  // Using t_zip_eq throws on mismatch
  print_subsection("t_zip_eq() - throws on mismatch");
  cout << "t_zip_eq on equal lists:\n";
  try
  {
    auto result = t_zip_eq(list1, list2);
    cout << "  Created " << result.size() << " tuples successfully\n";
  }
  catch (const length_error& e)
  {
    cout << "  Error: " << e.what() << endl;
  }
  
  cout << "\nt_zip_eq on unequal lists:\n";
  try
  {
    auto result = t_zip_eq(list1, list3);
    cout << "  Created " << result.size() << " tuples\n";
  }
  catch (const length_error& e)
  {
    cout << "  Caught exception: " << e.what() << endl;
  }
}

// =============================================================================
// 8. Practical Example: Sales Analysis
// =============================================================================

void demo_practical_example()
{
  print_section("PRACTICAL EXAMPLE: Coffee Sales Analysis");
  
  // Colombian coffee regions sales data
  DynList<string> regions = {"Huila", "Nariño", "Cauca", "Tolima", "Antioquia"};
  DynList<int> bags_2023 = {1200, 850, 720, 650, 580};
  DynList<int> bags_2024 = {1350, 920, 800, 680, 620};
  
  print_list("regions", regions);
  print_list("bags_2023", bags_2023);
  print_list("bags_2024", bags_2024);
  
  // Calculate growth using ZipIterator
  print_subsection("Year-over-Year Growth");
  cout << "Growth by region:\n";
  for (ZipIterator it(regions, bags_2023, bags_2024); it.has_curr(); it.next())
  {
    auto [region, y23, y24] = it.get_curr();
    double pct = 100.0 * (y24 - y23) / y23;
    cout << "  " << setw(12) << left << region 
         << ": " << y23 << " -> " << y24 
         << " (" << showpos << fixed << setprecision(1) << pct << "%)" 
         << noshowpos << endl;
  }
  
  // Find best performers using zip_filter
  print_subsection("Regions with > 10% growth");
  auto tuples = t_zip(regions, bags_2023, bags_2024);
  auto good_growth = tuples.filter([](const auto& t) {
    int y23 = get<1>(t);
    int y24 = get<2>(t);
    return (y24 - y23) * 100.0 / y23 > 10.0;
  });
  
  cout << "High growth regions:\n";
  for (auto it = good_growth.get_it(); it.has_curr(); it.next())
  {
    auto [region, y23, y24] = it.get_curr();
    double pct = 100.0 * (y24 - y23) / y23;
    cout << "  " << region << ": " << showpos << fixed << setprecision(1) 
         << pct << "%" << noshowpos << endl;
  }
  
  // Total production using ZipIterator
  print_subsection("Total Production");
  int total_2023 = 0, total_2024 = 0;
  for (ZipIterator it(bags_2023, bags_2024); it.has_curr(); it.next())
  {
    auto [y23, y24] = it.get_curr();
    total_2023 += y23;
    total_2024 += y24;
  }
  
  cout << "2023 total: " << total_2023 << " bags" << endl;
  cout << "2024 total: " << total_2024 << " bags" << endl;
  cout << "Overall growth: " << showpos << fixed << setprecision(1)
       << 100.0 * (total_2024 - total_2023) / total_2023 << "%" << noshowpos << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Comprehensive zip operations example for Aleph-w.\n"
      "Demonstrates ZipIterator, t_zip, zip_map, and more.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: basic, enum, tuples, transform, "
      "utilities, stl, length, practical, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "        ALEPH-W ZIP OPERATIONS EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "basic")
      demo_basic_zip();
    
    if (section == "all" or section == "enum")
      demo_enum_zip();
    
    if (section == "all" or section == "tuples")
      demo_tuple_operations();
    
    if (section == "all" or section == "transform")
      demo_zip_transformation();
    
    if (section == "all" or section == "utilities")
      demo_zip_utilities();
    
    if (section == "all" or section == "stl")
      demo_stl_compatibility();
    
    if (section == "all" or section == "length")
      demo_length_checking();
    
    if (section == "all" or section == "practical")
      demo_practical_example();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Zip operations demo completed!\n";
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
