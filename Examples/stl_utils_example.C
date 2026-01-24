
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
 * @file stl_utils_example.C
 * @brief Example demonstrating STL <-> Aleph-w container conversions
 *
 * This example demonstrates seamless interoperability between Standard Template
 * Library (STL) containers and Aleph-w containers using `ah-stl-utils.H`. This
 * is essential for integrating Aleph-w with existing STL-based codebases and
 * third-party libraries.
 *
 * ## Why Container Interoperability?
 *
 * Many C++ projects use STL containers (`std::vector`, `std::list`, etc.), while
 * Aleph-w provides its own container types (`DynList`, `DynArray`, etc.). This
 * utility bridge allows you to:
 *
 * - **Use both**: Leverage strengths of both container families
 * - **Integrate easily**: Convert between formats as needed
 * - **Migrate gradually**: Move from STL to Aleph-w incrementally
 * - **Interoperate**: Work with libraries expecting STL containers
 *
 * ## Features Demonstrated
 *
 * ### Container Conversions
 *
 * #### STL → Aleph-w
 * - `std::vector<T>` → `DynList<T>` / `DynArray<T>`
 * - `std::list<T>` → `DynList<T>`
 * - `std::set<T>` → `DynSetTree<T>`
 * - Iterator ranges → Aleph containers
 *
 * #### Aleph-w → STL
 * - `DynList<T>` → `std::vector<T>` / `std::list<T>`
 * - `DynArray<T>` → `std::vector<T>`
 * - Any Aleph container → STL container
 *
 * ### Advanced Conversions
 *
 * #### Tuple Conversions
 * - Convert tuples to containers
 * - Useful for function return values
 * - Unpack structured data
 *
 * #### Iterator Ranges
 * - Convert STL iterator ranges to Aleph containers
 * - Convert Aleph iterator ranges to STL containers
 * - Work with partial ranges
 *
 * #### Variadic Arguments
 * - Pack variadic arguments into containers
 * - Useful for function templates
 * - Flexible argument handling
 *
 * #### Map Transformations
 * - Convert between `std::map` and Aleph map types
 * - Transform key-value structures
 * - Preserve or transform data during conversion
 *
 * ## Use Cases
 *
 * ### Library Integration
 * - **Third-party APIs**: Many libraries expect STL containers
 * - **Legacy code**: Existing codebases using STL
 * - **Standard compliance**: Some interfaces require STL types
 *
 * ### Data Processing Pipelines
 * - **Input**: Read data into STL containers
 * - **Process**: Use Aleph-w algorithms (may be faster/better)
 * - **Output**: Convert back to STL for compatibility
 *
 * ### Migration Strategy
 * - **Phase 1**: Use STL containers
 * - **Phase 2**: Convert to Aleph-w where beneficial
 * - **Phase 3**: Use conversion utilities for compatibility
 *
 * ### Performance Optimization
 * - Use Aleph-w containers for performance-critical sections
 * - Convert to STL only when necessary for compatibility
 * - Minimize conversion overhead
 *
 * ## Conversion Overhead
 *
 * Most conversions are O(n) where n is container size:
 * - **Copy conversion**: Creates new container (memory overhead)
 * - **Move conversion**: More efficient (if supported)
 * - **View conversion**: Zero-copy (if available)
 *
 * **Best practice**: Minimize conversions, convert once at boundaries.
 *
 * ## Example Workflow
 *
 * ```cpp
 * // Read data into STL vector (from external API)
 * std::vector<int> stl_data = read_from_api();
 *
 * // Convert to Aleph-w for processing
 * DynList<int> aleph_data = vector_to_DynList(stl_data);
 *
 * // Use Aleph-w algorithms (may be faster)
 * auto result = aleph_data.maps([](int x) { return x * 2; });
 *
 * // Convert back to STL for output
 * std::vector<int> output = DynList_to_vector(result);
 * ```
 *
 * ## Usage
 *
 * ```bash
 * # Run all conversion examples
 * ./stl_utils_example
 * ```
 *
 * This example has no command-line options; it runs all demos.
 *
 * @see ah-stl-utils.H STL/Aleph-w conversion utilities
 * @see uni_functional_example.C Unified functional operations (works with both)
 * @see zip_utils_example.C Unified zip operations (works with both)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <numeric>

#include <ah-stl-utils.H>
#include <htlist.H>
#include <tpl_dynArray.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Helper functions
// ============================================================================

void print_header(const string& title)
{
  cout << "\n";
  cout << "+" << string(70, '-') << "+" << endl;
  cout << "| " << left << setw(68) << title << " |" << endl;
  cout << "+" << string(70, '-') << "+" << endl;
}

void print_subheader(const string& subtitle)
{
  cout << "\n  " << subtitle << endl;
  cout << "  " << string(subtitle.length(), '-') << endl;
}

template <typename Container>
void print_stl_container(const string& name, const Container& c)
{
  cout << "  " << name << " (" << c.size() << " elements): ";
  for (const auto& item : c)
    cout << item << " ";
  cout << endl;
}

template <typename T>
void print_dynlist(const string& name, const DynList<T>& l)
{
  cout << "  " << name << " (" << l.size() << " elements): ";
  for (auto it = l.get_it(); it.has_curr(); it.next_ne())
    cout << it.get_curr() << " ";
  cout << endl;
}

template <typename T>
void print_dynarray(const string& name, const DynArray<T>& arr)
{
  cout << "  " << name << " (" << arr.size() << " elements): ";
  for (size_t i = 0; i < arr.size(); ++i)
    cout << arr(i) << " ";
  cout << endl;
}

// ============================================================================
// Example 1: Vector <-> DynList conversions
// ============================================================================

void demo_vector_dynlist()
{
  print_header("Example 1: std::vector <-> DynList Conversions");
  
  // Colombian cities
  vector<string> cities_vec = {
    "Bogota", "Medellin", "Cali", "Barranquilla", "Cartagena"
  };
  
  print_subheader("STL vector to Aleph-w DynList");
  print_stl_container("Original vector", cities_vec);
  
  // Convert to DynList
  DynList<string> cities_list = to_DynList(cities_vec);
  print_dynlist("Converted DynList", cities_list);
  
  // Also works with vector_to_DynList
  DynList<string> cities_list2 = vector_to_DynList(cities_vec);
  cout << "  (vector_to_DynList also works)" << endl;
  
  print_subheader("Aleph-w DynList to STL vector");
  
  // Add more cities to DynList
  cities_list.append("Santa Marta");
  cities_list.append("Bucaramanga");
  
  // Convert back to vector
  vector<string> cities_vec2 = to_vector(cities_list);
  print_stl_container("Converted vector", cities_vec2);
  
  print_subheader("Numeric example");
  
  // Population data (thousands)
  vector<int> population = {8281, 2569, 2228, 1274, 1047};
  print_stl_container("Population vector", population);
  
  DynList<int> pop_list = to_DynList(population);
  print_dynlist("Population DynList", pop_list);
  
  // Sum using DynList's fold
  int total = pop_list.foldl(0, [](int acc, int p) { return acc + p; });
  cout << "  Total population: " << total << " thousand" << endl;
}

// ============================================================================
// Example 2: Vector <-> DynArray conversions
// ============================================================================

void demo_vector_dynarray()
{
  print_header("Example 2: std::vector <-> DynArray Conversions");
  
  // Colombian department areas (km²)
  vector<double> areas = {63612, 23188, 22140, 44640, 25020, 24885};
  
  print_subheader("STL vector to Aleph-w DynArray");
  print_stl_container("Areas vector", areas);
  
  DynArray<double> areas_arr = vector_to_DynArray(areas);
  print_dynarray("Areas DynArray", areas_arr);
  
  print_subheader("Aleph-w DynArray to STL vector");
  
  // Modify DynArray
  areas_arr.append(30000.5);
  areas_arr.append(55000.0);
  
  vector<double> areas_vec2 = DynArray_to_vector(areas_arr);
  print_stl_container("Modified areas vector", areas_vec2);
  
  // Calculate statistics using STL algorithms
  double total = accumulate(areas_vec2.begin(), areas_vec2.end(), 0.0);
  double avg = total / areas_vec2.size();
  
  cout << "\n  Statistics (using STL algorithms):" << endl;
  cout << "    Total area: " << fixed << setprecision(1) << total << " km2" << endl;
  cout << "    Average: " << avg << " km2" << endl;
}

// ============================================================================
// Example 3: std::list <-> DynList conversions
// ============================================================================

void demo_list_dynlist()
{
  print_header("Example 3: std::list <-> DynList Conversions");
  
  // Colombian rivers
  list<string> rivers_stl = {
    "Magdalena", "Cauca", "Atrato", "Meta", "Guaviare", "Caqueta"
  };
  
  print_subheader("std::list to DynList");
  print_stl_container("Rivers std::list", rivers_stl);
  
  DynList<string> rivers_aleph = list_to_DynList(rivers_stl);
  print_dynlist("Rivers DynList", rivers_aleph);
  
  print_subheader("DynList to std::list");
  
  // Add more rivers
  rivers_aleph.append("Putumayo");
  rivers_aleph.append("Orinoco");
  
  list<string> rivers_stl2 = DynList_to_list(rivers_aleph);
  print_stl_container("Extended rivers std::list", rivers_stl2);
  
  print_subheader("Using stl_container_to_dynList (generic)");
  
  // Works with any STL container
  set<int> altitude_set = {2640, 1538, 995, 18, 5, 213, 965};
  DynList<int> altitude_list = stl_container_to_dynList(altitude_set);
  print_dynlist("Altitudes from set", altitude_list);
  cout << "  (Note: set maintains sorted order)" << endl;
}

// ============================================================================
// Example 4: Iterator range conversions
// ============================================================================

void demo_range_conversions()
{
  print_header("Example 4: Iterator Range Conversions");
  
  // Temperature readings
  vector<double> temps = {23.5, 25.1, 24.8, 26.2, 22.9, 27.0, 25.5, 24.0};
  
  print_subheader("Full range to DynList");
  print_stl_container("Temperatures", temps);
  
  DynList<double> all_temps = range_to_DynList(temps.begin(), temps.end());
  print_dynlist("All temps DynList", all_temps);
  
  print_subheader("Partial range to DynList");
  
  // First 4 readings
  DynList<double> first_temps = range_to_DynList(temps.begin(), temps.begin() + 4);
  print_dynlist("First 4 temps", first_temps);
  
  // Last 3 readings
  DynList<double> last_temps = range_to_DynList(temps.end() - 3, temps.end());
  print_dynlist("Last 3 temps", last_temps);
  
  print_subheader("From raw array");
  
  int days[] = {1, 2, 3, 4, 5, 6, 7};
  DynList<int> days_list = range_to_DynList(begin(days), end(days));
  print_dynlist("Days from array", days_list);
}

// ============================================================================
// Example 5: Tuple conversions
// ============================================================================

void demo_tuple_conversions()
{
  print_header("Example 5: Tuple Conversions");
  
  print_subheader("Homogeneous tuple to DynList");
  
  // Coffee production by region (tons)
  auto production = make_tuple(125000, 98000, 85000, 72000, 65000);
  
  cout << "  Tuple elements: ";
  tuple_for_each(production, [](int val) {
    cout << val << " ";
  });
  cout << endl;
  
  DynList<int> prod_list = tuple_to_dynlist(production);
  print_dynlist("Production DynList", prod_list);
  
  print_subheader("Tuple to Array");
  
  auto regions = make_tuple(string("Huila"), string("Narino"), 
                            string("Cauca"), string("Tolima"));
  
  cout << "  Tuple elements: ";
  tuple_for_each(regions, [](const string& s) {
    cout << s << " ";
  });
  cout << endl;
  
  Array<string> regions_arr = tuple_to_array(regions);
  cout << "  Regions Array (" << regions_arr.size() << " elements): ";
  for (auto it = regions_arr.get_it(); it.has_curr(); it.next_ne())
    cout << it.get_curr() << " ";
  cout << endl;
  
  print_subheader("tuple_for_each for heterogeneous tuples");
  
  auto mixed = make_tuple(42, 3.14159, string("Colombia"), 'C');
  
  cout << "  Processing heterogeneous tuple:" << endl;
  tuple_for_each(mixed, [](const auto& val) {
    cout << "    -> " << val << endl;
  });
}

// ============================================================================
// Example 6: Variadic argument packing
// ============================================================================

void demo_variadic_packing()
{
  print_header("Example 6: Variadic Argument Packing");
  
  print_subheader("variadic_to_vector");
  
  // Create vector from arguments
  vector<int> scores = variadic_to_vector<int>(95, 87, 92, 78, 88, 91);
  print_stl_container("Scores vector", scores);
  
  // With doubles
  vector<double> rates = variadic_to_vector<double>(4.5, 3.8, 4.2, 4.0);
  print_stl_container("Rates vector", rates);
  
  print_subheader("variadic_to_DynList");
  
  DynList<string> products = variadic_to_DynList<string>(
    "Cafe", "Flores", "Banano", "Carbon", "Petroleo"
  );
  print_dynlist("Products DynList", products);
  
  print_subheader("Practical use case");
  
  // Build a quick list for processing
  auto cities = variadic_to_DynList<string>(
    "Leticia", "Mitú", "Puerto Inírida", "San José del Guaviare"
  );
  
  cout << "  Amazon region cities:" << endl;
  for (auto it = cities.get_it(); it.has_curr(); it.next_ne())
    cout << "    - " << it.get_curr() << endl;
}

// ============================================================================
// Example 7: Map transformations
// ============================================================================

void demo_map_transformations()
{
  print_header("Example 7: Map Transformations");
  
  print_subheader("map_vector: Transform elements");
  
  // GDP per capita in USD
  vector<double> gdp = {6500, 8200, 7800, 5900, 12000};
  print_stl_container("GDP per capita (USD)", gdp);
  
  // Convert to COP (1 USD = 4150 COP)
  auto gdp_cop = map_vector(gdp, [](double usd) { return usd * 4150.0; });
  
  cout << "  GDP per capita (COP): ";
  for (const auto& v : gdp_cop)
    cout << fixed << setprecision(0) << v << " ";
  cout << endl;
  
  print_subheader("map_vector: Type transformation");
  
  // Convert to formatted strings
  auto gdp_strings = map_vector(gdp, [](double val) {
    return "$" + to_string(static_cast<int>(val)) + " USD";
  });
  print_stl_container("GDP strings", gdp_strings);
  
  print_subheader("Chained transformations");
  
  vector<int> quantities = {10, 25, 15, 30, 20};
  print_stl_container("Quantities", quantities);
  
  // Apply discount and convert to DynList
  auto with_discount = map_vector(quantities, [](int q) { 
    return q * 0.9; 
  });
  
  DynList<double> final_list = to_DynList(with_discount);
  print_dynlist("After 10% discount", final_list);
}

// ============================================================================
// Example 8: Integration example
// ============================================================================

void demo_integration()
{
  print_header("Example 8: Integration - Processing Pipeline");
  
  cout << "\n  Scenario: Process sales data from STL to Aleph-w and back\n" << endl;
  
  // Step 1: Start with STL data
  vector<pair<string, double>> sales = {
    {"Bogota", 1250000},
    {"Medellin", 890000},
    {"Cali", 720000},
    {"Barranquilla", 450000},
    {"Cartagena", 380000}
  };
  
  cout << "  Step 1: Original STL data (city, sales)" << endl;
  for (const auto& [city, amount] : sales)
    cout << "    " << left << setw(15) << city << fixed << setprecision(0) 
         << amount << " COP" << endl;
  
  // Step 2: Extract cities and amounts separately
  vector<string> cities;
  vector<double> amounts;
  for (const auto& [city, amount] : sales) {
    cities.push_back(city);
    amounts.push_back(amount);
  }
  
  // Step 3: Convert to Aleph-w for processing
  DynList<string> cities_list = to_DynList(cities);
  DynList<double> amounts_list = to_DynList(amounts);
  
  cout << "\n  Step 2: Converted to Aleph-w DynList" << endl;
  cout << "    Cities: " << cities_list.size() << " items" << endl;
  cout << "    Amounts: " << amounts_list.size() << " items" << endl;
  
  // Step 4: Process with Aleph-w functional operations
  double total = amounts_list.foldl(0.0, [](double acc, double v) { 
    return acc + v; 
  });
  
  double avg = total / amounts_list.size();
  
  auto above_avg = amounts_list.filter([avg](double v) { 
    return v > avg; 
  });
  
  cout << "\n  Step 3: Aleph-w processing" << endl;
  cout << "    Total sales: " << fixed << setprecision(0) << total << " COP" << endl;
  cout << "    Average: " << avg << " COP" << endl;
  cout << "    Cities above average: " << above_avg.size() << endl;
  
  // Step 5: Convert back to STL for output
  vector<double> above_vec = to_vector(above_avg);
  
  cout << "\n  Step 4: Back to STL for output" << endl;
  cout << "    Sales above average: ";
  for (double v : above_vec)
    cout << v << " ";
  cout << endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "        ALEPH-W STL UTILS EXAMPLE" << endl;
  cout << "        STL <-> Aleph-w Container Conversions" << endl;
  cout << "========================================================================" << endl;
  
  demo_vector_dynlist();
  demo_vector_dynarray();
  demo_list_dynlist();
  demo_range_conversions();
  demo_tuple_conversions();
  demo_variadic_packing();
  demo_map_transformations();
  demo_integration();
  
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "                    Example completed successfully!" << endl;
  cout << "========================================================================" << endl;
  cout << endl;
  
  return 0;
}

