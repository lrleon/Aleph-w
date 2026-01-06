/**
 * @file ranges_example.C
 * @brief Example demonstrating C++20 Ranges support in Aleph-w.
 *
 * This program demonstrates `ah-ranges.H` which provides:
 * - Lazy range generation (no allocation until materialized)
 * - Range adaptors for Aleph containers
 * - Pipe operator (|) support
 * - Integration with std::views
 *
 * ## Key Concepts
 *
 * - **Lazy evaluation**: Values generated on demand, not upfront
 * - **Range adaptors**: Convert ranges to Aleph containers
 * - **Views**: Non-owning, composable range transformations
 * - **Pipe syntax**: `range | filter | transform | collect`
 *
 * ## Requirements
 *
 * - C++20 (-std=c++20)
 * - GCC 12+ or Clang 16+ with libc++
 *
 * ## Usage
 *
 * ```bash
 * ./ranges_example           # Run all demos
 * ./ranges_example -s lazy   # Only lazy ranges demo
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see ah-ranges.H C++20 Ranges support
 * @see ahFunctional.H Traditional functional utilities
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

#include <tclap/CmdLine.h>

#include <htlist.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <ah-ranges.H>

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
void print_container(const string& label, const Container& c)
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
// 1. Lazy Range Generation
// =============================================================================

#if ALEPH_HAS_RANGES

void demo_lazy_ranges()
{
  print_section("LAZY RANGE GENERATION");
  
  cout << "Unlike traditional range() which allocates immediately,\n";
  cout << "lazy_range() generates values on demand.\n\n";
  
  // Basic lazy range
  print_subsection("lazy_range(start, end)");
  
  cout << "lazy_range(1, 10): ";
  for (auto x : lazy_range(1, 10))
    cout << x << " ";
  cout << endl;
  
  cout << "lazy_range(5): ";  // 0 to 4
  for (auto x : lazy_range(5))
    cout << x << " ";
  cout << endl;
  
  // Demonstrate laziness
  print_subsection("Laziness demonstration");
  
  cout << "Processing lazy_range(1, 1000000) but stopping at 5:\n";
  int count = 0;
  for (auto x : lazy_range(1, 1000000))
  {
    cout << x << " ";
    if (++count >= 5) break;
  }
  cout << "\n(Only 5 values were generated, not 1 million!)\n";
  
  // Infinite range with lazy_iota
  print_subsection("lazy_iota() - infinite range");
  
  cout << "First 10 values from lazy_iota(100): ";
  count = 0;
  for (auto x : lazy_iota(100))
  {
    cout << x << " ";
    if (++count >= 10) break;
  }
  cout << endl;
}

// =============================================================================
// 2. Range Views
// =============================================================================

void demo_views()
{
  print_section("RANGE VIEWS (std::views)");
  
  cout << "Views are lazy, composable transformations.\n";
  cout << "No allocation until you iterate or materialize.\n\n";
  
  // Filter view
  print_subsection("filter - keep elements matching predicate");
  
  cout << "Even numbers in [1, 20]: ";
  for (auto x : lazy_range(1, 21) | std::views::filter([](int x) { return x % 2 == 0; }))
    cout << x << " ";
  cout << endl;
  
  // Transform view
  print_subsection("transform - apply function to each element");
  
  cout << "Squares of [1, 10]: ";
  for (auto x : lazy_range(1, 11) | std::views::transform([](int x) { return x * x; }))
    cout << x << " ";
  cout << endl;
  
  // Take and drop
  print_subsection("take and drop");
  
  cout << "First 5 of [1, 100]: ";
  for (auto x : lazy_range(1, 101) | std::views::take(5))
    cout << x << " ";
  cout << endl;
  
  cout << "Skip first 95 of [1, 100]: ";
  for (auto x : lazy_range(1, 101) | std::views::drop(95))
    cout << x << " ";
  cout << endl;
  
  // Composition
  print_subsection("Composing views");
  
  cout << "First 5 primes (brute force): ";
  auto is_prime = [](int n) {
    if (n < 2) return false;
    for (int i = 2; i <= sqrt(n); i++)
      if (n % i == 0) return false;
    return true;
  };
  
  for (auto x : lazy_iota(2) | std::views::filter(is_prime) | std::views::take(5))
    cout << x << " ";
  cout << endl;
}

// =============================================================================
// 3. Materializing to Aleph Containers
// =============================================================================

void demo_materialize()
{
  print_section("MATERIALIZING TO ALEPH CONTAINERS");
  
  cout << "Convert lazy ranges to Aleph containers using pipe adaptors.\n\n";
  
  // to_dynlist_v
  print_subsection("to_dynlist_v");
  
  auto list = lazy_range(1, 6) 
            | std::views::transform([](int x) { return x * 10; })
            | to_dynlist_v;
  
  print_container("DynList from [10, 20, 30, 40, 50]", list);
  
  // to_dynarray_v
  print_subsection("to_dynarray_v");
  
  auto arr = lazy_range(1, 6) 
           | std::views::filter([](int x) { return x % 2 == 1; })
           | to_dynarray_v;
  
  print_container("DynArray of odd numbers [1, 3, 5]", arr);
  
  // to_dyndlist_v
  print_subsection("to_dyndlist_v");
  
  auto dlist = lazy_range(1, 4) | to_dyndlist_v;
  print_container("DynDlist [1, 2, 3]", dlist);
  
  // Generic to<>()
  print_subsection("Generic to<Container>()");
  
  auto result = lazy_range(1, 5) | to<DynList<int>>();
  print_container("Using to<DynList<int>>()", result);
  
  // collect function
  print_subsection("collect<Container>() function");
  
  auto collected = collect<DynArray<double>>(
    lazy_range(1, 5) | std::views::transform([](int x) { return x * 0.5; })
  );
  print_container("Collected DynArray<double>", collected);
}

// =============================================================================
// 4. Working with Aleph Containers
// =============================================================================

void demo_aleph_containers()
{
  print_section("RANGES WITH ALEPH CONTAINERS");
  
  cout << "Aleph containers work with std::ranges algorithms.\n\n";
  
  // Create a DynList
  DynList<int> numbers;
  for (int i = 1; i <= 10; i++)
    numbers.append(i);
  
  print_container("Original DynList", numbers);
  
  // std::ranges algorithms
  print_subsection("std::ranges algorithms");
  
  bool has_even = std::ranges::any_of(numbers, [](int x) { return x % 2 == 0; });
  cout << "Has even number? " << (has_even ? "yes" : "no") << endl;
  
  bool all_positive = std::ranges::all_of(numbers, [](int x) { return x > 0; });
  cout << "All positive? " << (all_positive ? "yes" : "no") << endl;
  
  auto count_gt5 = std::ranges::count_if(numbers, [](int x) { return x > 5; });
  cout << "Count > 5: " << count_gt5 << endl;
  
  // Using views on DynList
  print_subsection("Views on DynList");
  
  cout << "Filtered (even): ";
  for (auto x : numbers | std::views::filter([](int x) { return x % 2 == 0; }))
    cout << x << " ";
  cout << endl;
  
  cout << "Transformed (squared): ";
  for (auto x : numbers | std::views::transform([](int x) { return x * x; }))
    cout << x << " ";
  cout << endl;
  
  cout << "First 3: ";
  for (auto x : numbers | std::views::take(3))
    cout << x << " ";
  cout << endl;
}

// =============================================================================
// 5. Practical Examples
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL EXAMPLES");
  
  // Fibonacci sequence
  print_subsection("Fibonacci (first 15 numbers)");
  
  // Using transform with state (via mutable lambda)
  int a = 0, b = 1;
  auto fib = lazy_range(15) | std::views::transform([&a, &b](int) mutable {
    int result = a;
    int next = a + b;
    a = b;
    b = next;
    return result;
  });
  
  cout << "Fibonacci: ";
  for (auto x : fib)
    cout << x << " ";
  cout << endl;
  
  // FizzBuzz
  print_subsection("FizzBuzz (1-20)");
  
  for (auto n : lazy_range(1, 21))
  {
    if (n % 15 == 0) cout << "FizzBuzz ";
    else if (n % 3 == 0) cout << "Fizz ";
    else if (n % 5 == 0) cout << "Buzz ";
    else cout << n << " ";
  }
  cout << endl;
  
  // Data pipeline
  print_subsection("Data pipeline: Process sensor readings");
  
  // Simulated sensor readings (some invalid, marked as -1)
  DynList<double> readings;
  readings.append(23.5); readings.append(-1); readings.append(24.1);
  readings.append(25.0); readings.append(-1); readings.append(23.8);
  readings.append(24.5); readings.append(-1); readings.append(24.0);
  
  cout << "Raw readings: ";
  for (auto it = readings.get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;
  
  // Pipeline: filter valid -> convert to Fahrenheit -> collect
  auto fahrenheit = readings 
                  | std::views::filter([](double x) { return x >= 0; })
                  | std::views::transform([](double c) { return c * 9/5 + 32; })
                  | to_dynlist_v;
  
  cout << "Valid readings in Fahrenheit: ";
  for (auto it = fahrenheit.get_it(); it.has_curr(); it.next())
    cout << fixed << setprecision(1) << it.get_curr() << " ";
  cout << endl;
  
  // Statistics using ranges
  double sum = 0;
  size_t count = 0;
  for (auto x : readings | std::views::filter([](double x) { return x >= 0; }))
  {
    sum += x;
    count++;
  }
  cout << "Average (Celsius): " << fixed << setprecision(2) << (sum / count) << endl;
}

// =============================================================================
// 6. Performance Comparison
// =============================================================================

void demo_performance()
{
  print_section("LAZY vs EAGER EVALUATION");
  
  cout << "Lazy evaluation can be more efficient when:\n";
  cout << "- You don't need all elements\n";
  cout << "- You're chaining multiple operations\n";
  cout << "- Working with large or infinite sequences\n\n";
  
  print_subsection("Example: Find first square > 1000");
  
  // Lazy approach - stops when found
  cout << "Lazy: ";
  for (auto x : lazy_iota(1) 
              | std::views::transform([](int x) { return x * x; })
              | std::views::filter([](int x) { return x > 1000; })
              | std::views::take(1))
  {
    cout << x << " (computed ~32 squares)" << endl;
  }
  
  // Show the work
  cout << "\nWith lazy evaluation, we only compute what's needed.\n";
  cout << "No intermediate containers are created.\n";
}

#else // !ALEPH_HAS_RANGES

void demo_no_ranges()
{
  print_section("C++20 RANGES NOT AVAILABLE");
  
  cout << "Your compiler does not support C++20 ranges.\n\n";
  cout << "Requirements:\n";
  cout << "  - C++20 or later (-std=c++20)\n";
  cout << "  - GCC 12+ with libstdc++, OR\n";
  cout << "  - Clang 14+ with libc++ (-stdlib=libc++)\n\n";
  
  cout << "Alternative: Use ahFunctional.H for functional operations:\n";
  cout << "  - range(start, end) - eager range generation\n";
  cout << "  - filter(container, pred)\n";
  cout << "  - maps(container, func)\n";
  cout << "  - foldl(container, init, op)\n";
}

#endif // ALEPH_HAS_RANGES

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "C++20 Ranges example for Aleph-w.\n"
      "Demonstrates lazy evaluation and range adaptors.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: lazy, views, materialize, aleph, "
      "practical, perf, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "          ALEPH-W C++20 RANGES EXAMPLE\n";
    cout << "============================================================\n";
    
#if ALEPH_HAS_RANGES
    cout << "\nALEPH_HAS_RANGES = 1 (C++20 ranges supported)\n";
    
    if (section == "all" or section == "lazy")
      demo_lazy_ranges();
    
    if (section == "all" or section == "views")
      demo_views();
    
    if (section == "all" or section == "materialize")
      demo_materialize();
    
    if (section == "all" or section == "aleph")
      demo_aleph_containers();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    if (section == "all" or section == "perf")
      demo_performance();
#else
    cout << "\nALEPH_HAS_RANGES = 0 (C++20 ranges not available)\n";
    demo_no_ranges();
#endif
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Ranges demo completed!\n";
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

