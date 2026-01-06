/**
 * @file sort_benchmark.C
 * @brief Comprehensive benchmark for Aleph-w sorting algorithms.
 *
 * This program benchmarks **only Aleph-w sorting algorithms** across
 * different data distributions and Aleph-w container types.
 * No std::sort or STL containers are used.
 *
 * ## Sorting Algorithms Tested (all from Aleph-w)
 *
 * ### O(n²) Algorithms (optional, for small inputs)
 * - **selection_sort()**: Simple, minimal swaps, always O(n²)
 * - **insertion_sort()**: Adaptive, excellent for nearly sorted data
 * - **bubble_sort()**: Educational, rarely used in practice
 *
 * ### Sub-quadratic Algorithms
 * - **shellsort()**: O(n^1.3) empirical, good for medium arrays
 *
 * ### O(n log n) Algorithms
 * - **mergesort()**: Stable, guaranteed O(n log n), great for lists
 * - **quicksort_op()**: Fastest average case, iterative (no stack overflow)
 * - **heapsort()**: Guaranteed O(n log n), in-place, not stable
 *
 * ## Data Distributions Tested
 *
 * | Distribution | Description | Best Algorithm |
 * |--------------|-------------|----------------|
 * | Random | Uniformly distributed | quicksort_op() |
 * | Sorted | Already ascending | insertion_sort() |
 * | Reverse | Descending order | mergesort()/heapsort() |
 * | Nearly Sorted | 5% elements swapped | insertion_sort() |
 * | Few Unique | Only 10 distinct values | quicksort_op() |
 * | Sawtooth | Alternating ascending runs | mergesort() |
 *
 * ## Aleph-w Container Types
 *
 * - **DynArray**: Segmented blocks, O(1) access via operator()
 * - **DynList**: Singly linked, O(1) insert/delete, O(n) access
 * - **DynDlist**: Doubly linked, bidirectional iteration
 *
 * ## Usage
 *
 * ```bash
 * ./sort_benchmark                    # Default: 10000 elements
 * ./sort_benchmark -n 50000           # 50000 elements
 * ./sort_benchmark -n 1000 -a         # 1000 elements, all algorithms
 * ./sort_benchmark --list-only        # Only test linked lists
 * ./sort_benchmark --array-only       # Only test arrays
 * ./sort_benchmark -c                 # Complexity demonstration
 * ./sort_benchmark -g                 # Algorithm selection guide
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see tpl_sort_utils.H For sorting algorithm implementations
 * @see ahSort.H For high-level sorting functions
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <functional>
#include <random>

#include <tclap/CmdLine.h>

#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <htlist.H>
#include <tpl_sort_utils.H>
#include <ahSort.H>

using namespace std;
using namespace Aleph;
using namespace std::chrono;

// =============================================================================
// Configuration
// =============================================================================

struct BenchmarkConfig
{
  size_t num_elements = 10000;
  bool run_slow_algorithms = false;  // O(n²) algorithms
  bool test_arrays = true;
  bool test_lists = true;
  bool verbose = false;
  unsigned seed = 42;
};

// =============================================================================
// Timer utility
// =============================================================================

class Timer
{
  high_resolution_clock::time_point start_time;
  
public:
  void start() { start_time = high_resolution_clock::now(); }
  
  double elapsed_ms() const
  {
    auto end_time = high_resolution_clock::now();
    return duration<double, milli>(end_time - start_time).count();
  }
};

// =============================================================================
// Data generators
// =============================================================================

class DataGenerator
{
  mt19937 rng;
  
public:
  explicit DataGenerator(unsigned seed) : rng(seed) {}
  
  /// Uniformly distributed random integers
  void random(DynArray<int>& arr, size_t n)
  {
    arr.empty();
    uniform_int_distribution<int> dist(1, static_cast<int>(n * 10));
    for (size_t i = 0; i < n; ++i)
      arr.append(dist(rng));
  }
  
  /// Already sorted in ascending order
  void sorted_asc(DynArray<int>& arr, size_t n)
  {
    arr.empty();
    for (size_t i = 0; i < n; ++i)
      arr.append(static_cast<int>(i));
  }
  
  /// Sorted in descending order
  void sorted_desc(DynArray<int>& arr, size_t n)
  {
    arr.empty();
    for (size_t i = n; i > 0; --i)
      arr.append(static_cast<int>(i));
  }
  
  /// Nearly sorted: 5% of elements are swapped
  void nearly_sorted(DynArray<int>& arr, size_t n)
  {
    sorted_asc(arr, n);
    size_t swaps = n / 20;  // 5%
    uniform_int_distribution<size_t> dist(0, n - 1);
    for (size_t i = 0; i < swaps; ++i)
    {
      size_t a = dist(rng);
      size_t b = dist(rng);
      swap(arr(a), arr(b));
    }
  }
  
  /// Few unique values (high repetition)
  void few_unique(DynArray<int>& arr, size_t n)
  {
    arr.empty();
    uniform_int_distribution<int> dist(1, 10);
    for (size_t i = 0; i < n; ++i)
      arr.append(dist(rng));
  }
  
  /// Sawtooth pattern: multiple ascending runs
  void sawtooth(DynArray<int>& arr, size_t n)
  {
    arr.empty();
    size_t run_size = max(n / 10, size_t(1));
    for (size_t i = 0; i < n; ++i)
      arr.append(static_cast<int>(i % run_size));
  }
};

// =============================================================================
// Container converters
// =============================================================================

template <typename T>
void to_dynlist(const DynArray<T>& arr, DynList<T>& list)
{
  list.empty();
  for (size_t i = 0; i < arr.size(); ++i)
    list.append(arr(i));
}

template <typename T>
void to_dyndlist(const DynArray<T>& arr, DynDlist<T>& list)
{
  list.empty();
  for (size_t i = 0; i < arr.size(); ++i)
    list.append(arr(i));
}

// =============================================================================
// Output formatting
// =============================================================================

void print_header()
{
  cout << "\n" << string(79, '=') << "\n";
  cout << setw(18) << left << "Algorithm"
       << setw(15) << left << "Distribution"
       << setw(12) << left << "Container"
       << setw(14) << right << "Time (ms)"
       << setw(10) << "Status"
       << endl;
  cout << string(79, '-') << endl;
}

void print_result(const string& algo, const string& dist, 
                  const string& container, double time_ms, bool ok)
{
  cout << setw(18) << left << algo
       << setw(15) << left << dist
       << setw(12) << left << container
       << setw(14) << right << fixed << setprecision(2) << time_ms
       << setw(10) << (ok ? "OK" : "FAIL")
       << endl;
}

void print_separator()
{
  cout << string(79, '-') << endl;
}

// =============================================================================
// Array sorting benchmarks
// =============================================================================

void benchmark_array_algorithms(const BenchmarkConfig& config,
                                 const string& dist_name,
                                 const DynArray<int>& source_data)
{
  Timer timer;
  DynArray<int> arr;
  
  // Note: DynArray uses segmented blocks (not contiguous memory)
  // So we must use Aleph's sorting functions, not std::sort
  
  // Quicksort (optimized iterative version) - uses DynArray::operator()
  arr = source_data;
  timer.start();
  quicksort_op(arr);
  print_result("Quicksort", dist_name, "DynArray", timer.elapsed_ms(), is_sorted(arr));
  
  // Heapsort - uses DynArray::operator()
  arr = source_data;
  timer.start();
  heapsort(arr);
  print_result("Heapsort", dist_name, "DynArray", timer.elapsed_ms(), is_sorted(arr));
  
  // Shell Sort - uses DynArray::operator()
  arr = source_data;
  timer.start();
  shellsort(arr);
  print_result("Shell Sort", dist_name, "DynArray", timer.elapsed_ms(), is_sorted(arr));
  
  // O(n²) algorithms - only for small inputs
  if (config.run_slow_algorithms and config.num_elements <= 30000)
  {
    arr = source_data;
    timer.start();
    insertion_sort(arr);
    print_result("Insertion Sort", dist_name, "DynArray", timer.elapsed_ms(), is_sorted(arr));
    
    arr = source_data;
    timer.start();
    selection_sort(arr);
    print_result("Selection Sort", dist_name, "DynArray", timer.elapsed_ms(), is_sorted(arr));
    
    if (config.num_elements <= 10000)
    {
      arr = source_data;
      timer.start();
      bubble_sort(arr);
      print_result("Bubble Sort", dist_name, "DynArray", timer.elapsed_ms(), is_sorted(arr));
    }
  }
}

// =============================================================================
// List sorting benchmarks
// =============================================================================

void benchmark_list_algorithms(const BenchmarkConfig& config,
                                const string& dist_name,
                                const DynArray<int>& source_data)
{
  Timer timer;
  DynList<int> slist;
  DynDlist<int> dlist;
  
  // DynList - Merge Sort
  to_dynlist(source_data, slist);
  timer.start();
  mergesort(slist);
  print_result("Merge Sort", dist_name, "DynList", timer.elapsed_ms(), is_sorted(slist));
  
  // DynList - Quicksort
  to_dynlist(source_data, slist);
  timer.start();
  quicksort(slist);
  print_result("Quicksort", dist_name, "DynList", timer.elapsed_ms(), is_sorted(slist));
  
  // DynDlist - Merge Sort
  to_dyndlist(source_data, dlist);
  timer.start();
  mergesort(dlist);
  print_result("Merge Sort", dist_name, "DynDlist", timer.elapsed_ms(), is_sorted(dlist));
  
  // DynDlist - Quicksort
  to_dyndlist(source_data, dlist);
  timer.start();
  quicksort(dlist);
  print_result("Quicksort", dist_name, "DynDlist", timer.elapsed_ms(), is_sorted(dlist));
  
  // O(n²) - only for small inputs
  if (config.run_slow_algorithms and config.num_elements <= 10000)
  {
    to_dynlist(source_data, slist);
    timer.start();
    insertion_sort(slist);
    print_result("Insertion Sort", dist_name, "DynList", timer.elapsed_ms(), is_sorted(slist));
    
    to_dyndlist(source_data, dlist);
    timer.start();
    in_place_sort(dlist);
    print_result("Insertion Sort", dist_name, "DynDlist", timer.elapsed_ms(), is_sorted(dlist));
  }
}

// =============================================================================
// Main benchmark runner
// =============================================================================

void run_benchmarks(const BenchmarkConfig& config)
{
  cout << "\n";
  cout << "============================================================\n";
  cout << "       ALEPH-W SORTING ALGORITHMS BENCHMARK\n";
  cout << "============================================================\n";
  cout << "\nConfiguration:\n";
  cout << "  Elements:        " << config.num_elements << "\n";
  cout << "  O(n^2) algos:    " << (config.run_slow_algorithms ? "Yes" : "No") << "\n";
  cout << "  Test arrays:     " << (config.test_arrays ? "Yes" : "No") << "\n";
  cout << "  Test lists:      " << (config.test_lists ? "Yes" : "No") << "\n";
  cout << "  Random seed:     " << config.seed << "\n";
  
  DataGenerator gen(config.seed);
  DynArray<int> data;
  
  // Distribution names and generators
  struct DistInfo
  {
    const char* name;
    void (DataGenerator::*generator)(DynArray<int>&, size_t);
  };
  
  DistInfo distributions[] = {
    {"Random",        &DataGenerator::random},
    {"Sorted",        &DataGenerator::sorted_asc},
    {"Reverse",       &DataGenerator::sorted_desc},
    {"Nearly Sorted", &DataGenerator::nearly_sorted},
    {"Few Unique",    &DataGenerator::few_unique},
    {"Sawtooth",      &DataGenerator::sawtooth}
  };
  
  print_header();
  
  for (const auto& dist : distributions)
  {
    // Generate data for this distribution
    (gen.*(dist.generator))(data, config.num_elements);
    
    if (config.test_arrays)
      benchmark_array_algorithms(config, dist.name, data);
    
    if (config.test_lists)
      benchmark_list_algorithms(config, dist.name, data);
    
    print_separator();
  }
  
  cout << "\nBenchmark completed successfully!\n\n";
}

// =============================================================================
// Complexity demonstration
// =============================================================================

void demonstrate_complexity(const BenchmarkConfig& base_config)
{
  cout << "\n";
  cout << "============================================================\n";
  cout << "       COMPLEXITY DEMONSTRATION\n";
  cout << "============================================================\n";
  cout << "\nShows how sorting time scales with input size.\n";
  cout << "For O(n log n): doubling n roughly doubles time.\n";
  cout << "For O(n^2): doubling n roughly quadruples time.\n\n";
  
  DataGenerator gen(base_config.seed);
  DynArray<int> data;
  DynArray<int> arr;
  DynList<int> list;
  Timer timer;
  
  size_t sizes[] = {1000, 2000, 4000, 8000, 16000, 32000};
  
  cout << setw(10) << "Size"
       << setw(15) << "Quicksort"
       << setw(15) << "Heapsort"
       << setw(15) << "Shell Sort"
       << setw(15) << "Merge (list)"
       << endl;
  cout << string(70, '-') << endl;
  
  for (size_t n : sizes)
  {
    gen.random(data, n);
    cout << setw(10) << n;
    
    // Quicksort (DynArray)
    arr = data;
    timer.start();
    quicksort_op(arr);
    cout << setw(15) << fixed << setprecision(2) << timer.elapsed_ms();
    
    // Heapsort (DynArray)
    arr = data;
    timer.start();
    heapsort(arr);
    cout << setw(15) << timer.elapsed_ms();
    
    // Shell Sort (DynArray)
    arr = data;
    timer.start();
    shellsort(arr);
    cout << setw(15) << timer.elapsed_ms();
    
    // Merge Sort (DynList)
    to_dynlist(data, list);
    timer.start();
    mergesort(list);
    cout << setw(15) << timer.elapsed_ms();
    
    cout << endl;
  }
  
  cout << "\nAll algorithms use Aleph-w containers and sorting functions.\n";
}

// =============================================================================
// Algorithm selection guide
// =============================================================================

void print_recommendations()
{
  cout << "\n";
  cout << "============================================================\n";
  cout << "       ALGORITHM SELECTION GUIDE\n";
  cout << "============================================================\n";
  cout << R"(
+-------------------+------------------------+-------------------------+
| Scenario          | Best Choice            | Why                     |
+-------------------+------------------------+-------------------------+
| General purpose   | quicksort_op()         | Fastest average case    |
| Nearly sorted     | insertion_sort()       | O(n) for sorted data    |
| Guaranteed O(nlogn)| heapsort()            | No worst case O(n^2)    |
| Linked lists      | mergesort()            | O(1) extra space        |
| Stability needed  | mergesort()            | Preserves equal order   |
| Limited memory    | heapsort()             | O(1) extra space        |
| Small arrays (<50)| insertion_sort()       | Low overhead            |
| External sorting  | mergesort()            | Sequential access       |
| Medium arrays     | shellsort()            | Good balance            |
+-------------------+------------------------+-------------------------+

For Aleph-w containers:
  - DynArray: Use quicksort_op(), heapsort(), or shellsort()
  - DynList:  Use mergesort() (O(1) extra space for lists!)
  - DynDlist: Use mergesort() or quicksort()
)";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Comprehensive sorting algorithm benchmark for Aleph-w.\n"
      "Tests multiple algorithms across different data distributions "
      "and container types.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<size_t> nArg(
      "n", "count",
      "Number of elements to sort",
      false, 10000, "count", cmd
    );
    
    TCLAP::SwitchArg allArg(
      "a", "all",
      "Include O(n^2) algorithms (selection, insertion, bubble)",
      cmd, false
    );
    
    TCLAP::SwitchArg arrayOnlyArg(
      "", "array-only",
      "Only test array-based containers",
      cmd, false
    );
    
    TCLAP::SwitchArg listOnlyArg(
      "", "list-only",
      "Only test linked list containers",
      cmd, false
    );
    
    TCLAP::SwitchArg complexityArg(
      "c", "complexity",
      "Run complexity demonstration (time vs size)",
      cmd, false
    );
    
    TCLAP::SwitchArg guideArg(
      "g", "guide",
      "Print algorithm selection guide",
      cmd, false
    );
    
    TCLAP::ValueArg<unsigned> seedArg(
      "s", "seed",
      "Random seed for reproducible results",
      false, 42, "seed", cmd
    );
    
    TCLAP::SwitchArg verboseArg(
      "v", "verbose",
      "Verbose output",
      cmd, false
    );
    
    cmd.parse(argc, argv);
    
    BenchmarkConfig config;
    config.num_elements = nArg.getValue();
    config.run_slow_algorithms = allArg.getValue();
    config.test_arrays = not listOnlyArg.getValue();
    config.test_lists = not arrayOnlyArg.getValue();
    config.seed = seedArg.getValue();
    config.verbose = verboseArg.getValue();
    
    if (listOnlyArg.getValue() and arrayOnlyArg.getValue())
    {
      cerr << "Error: Cannot specify both --array-only and --list-only\n";
      return 1;
    }
    
    if (guideArg.getValue())
    {
      print_recommendations();
      return 0;
    }
    
    if (complexityArg.getValue())
    {
      demonstrate_complexity(config);
      return 0;
    }
    
    run_benchmarks(config);
    
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
