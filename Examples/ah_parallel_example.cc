/**
 * @file ah_parallel_example.cc
 * @brief Parallel functional programming utilities (ah-parallel.H): map/filter/fold/predicates/zip/sort.
 *
 * ## Overview
 *
 * This example demonstrates Aleph-w's **parallel functional programming** helpers
 * from `ah-parallel.H`. The API provides ML-style operations (map, filter, fold,
 * predicates, etc.) that execute in parallel using an `Aleph::ThreadPool`.
 *
 * The file is structured as a series of demos covering:
 *
 * - parallel map/filter/fold
 * - parallel predicates and find
 * - aggregations (sum/product/min/max)
 * - parallel sort
 * - parallel zip / enumerate
 * - a simple parallel vs sequential performance comparison
 *
 * ## Data model used by this example
 *
 * - Primary containers: `std::vector<int>`, `std::vector<double>`, `std::vector<std::string>`
 * - Execution engine: `ThreadPool pool(std::thread::hardware_concurrency())`
 *
 * ## Usage
 *
 * ```bash
 * ./ah_parallel_example
 * ```
 *
 * This example has no command-line options; it runs all demos.
 *
 * ## Algorithms and Aleph-w API
 *
 * Common signature pattern:
 *
 * - the first parameter is always a `ThreadPool&`
 * - the remaining parameters are similar to the sequential counterparts
 *
 * Core operations demonstrated:
 *
 * - `pmaps(pool, container, f)`
 * - `pfilter(pool, container, pred)`
 * - `pfoldl(pool, container, init, op)`
 * - `pall` / `pexists` / `pnone` / `pcount_if`
 * - `pfind` / `pfind_value`
 * - `psum` / `pproduct` / `pmin` / `pmax`
 * - `psort`
 * - `pzip_*`, `penumerate_*`
 *
 * ## Complexity
 *
 * Asymptotically, most operations have the same work complexity as the sequential
 * version (e.g. `O(n)` for map/filter), but with wall-clock time reduced by
 * parallelization.
 *
 * Actual speedups depend on:
 *
 * - task granularity and chunking
 * - CPU core count
 * - memory bandwidth
 * - overhead of scheduling
 *
 * ## Pitfalls and edge cases
 *
 * - For `pfoldl`, the binary operator should be **associative** to ensure that
 *   parallel reduction is well-defined.
 * - Short-circuit operations (`pall`, `pexists`, `pfind`) may stop early.
 * - Parallel overhead can dominate for small inputs; measure before assuming gains.
 *
 * ## References / see also
 *
 * - `ah-parallel.H`
 * - `thread_pool_example.cc` (ThreadPool usage)
 *
 * @ingroup Examples
 */

#include <ah-parallel.H>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <numeric>
#include <random>
#include <string>

using namespace Aleph;
using namespace std::chrono_literals;

// Helper to print section headers
void print_header(const std::string& title)
{
  std::cout << "\n";
  std::cout << "+" << std::string(65, '-') << "+\n";
  std::cout << "| " << std::left << std::setw(63) << title << " |\n";
  std::cout << "+" << std::string(65, '-') << "+\n\n";
}

// =============================================================================
// EXAMPLE 1: Parallel Map (pmaps)
// =============================================================================
//
// pmaps applies a function to each element in parallel, returning a new vector.
// This is the parallel equivalent of std::transform or Haskell's map.
//
// SIGNATURE:
//   pmaps<ResultT>(pool, container, func) → std::vector<ResultT>
//   pmaps(pool, container, func)          → std::vector<auto>  (type deduced)
//

void example_parallel_map()
{
  print_header("Example 1: Parallel Map (pmaps)");
  
  std::cout << "GOAL: Transform a large dataset in parallel.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  std::cout << "Using ThreadPool with " << pool.num_threads() << " workers\n\n";
  
  // Create input data: 1 million integers
  std::vector<int> numbers(1000000);
  std::iota(numbers.begin(), numbers.end(), 1);
  
  std::cout << "Input: " << numbers.size() << " integers\n";
  
  // PARALLEL MAP: Square each number (with type conversion)
  auto start = std::chrono::high_resolution_clock::now();
  
  // Type deduction: returns vector<long long> because lambda returns long long
  auto squares = pmaps(pool, numbers, [](int x) {
    return static_cast<long long>(x) * x;
  });
  
  auto end = std::chrono::high_resolution_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  
  std::cout << "Output: " << squares.size() << " squared values\n\n";
  
  // Show sample results
  std::cout << "First 5 results: ";
  for (size_t i = 0; i < 5; ++i)
    std::cout << squares[i] << " ";
  std::cout << "\n";
  
  std::cout << "Last 5 results: ";
  for (size_t i = squares.size() - 5; i < squares.size(); ++i)
    std::cout << squares[i] << " ";
  std::cout << "\n\n";
  
  std::cout << "✓ Completed in " << ms << " ms\n";
}

// =============================================================================
// EXAMPLE 2: Parallel Filter (pfilter)
// =============================================================================
//
// pfilter selects elements satisfying a predicate, preserving order.
// Elements are tested in parallel chunks, then merged sequentially.
//

void example_parallel_filter()
{
  print_header("Example 2: Parallel Filter (pfilter)");
  
  std::cout << "GOAL: Find all prime numbers in a range using parallel filtering.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Generate numbers 2 to 100000
  std::vector<int> candidates(99999);
  std::iota(candidates.begin(), candidates.end(), 2);
  
  std::cout << "Checking " << candidates.size() << " candidates for primality...\n";
  
  // Primality test (intentionally slow for demo)
  auto is_prime = [](int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2)
      if (n % i == 0) return false;
    return true;
  };
  
  auto start = std::chrono::high_resolution_clock::now();
  
  auto primes = pfilter(pool, candidates, is_prime);
  
  auto end = std::chrono::high_resolution_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  
  std::cout << "Found " << primes.size() << " primes\n\n";
  
  // Show some primes
  std::cout << "First 10: ";
  for (size_t i = 0; i < 10 && i < primes.size(); ++i)
    std::cout << primes[i] << " ";
  std::cout << "\n";
  
  std::cout << "Last 10:  ";
  for (size_t i = primes.size() > 10 ? primes.size() - 10 : 0; i < primes.size(); ++i)
    std::cout << primes[i] << " ";
  std::cout << "\n\n";
  
  std::cout << "✓ Completed in " << ms << " ms\n";
}

// =============================================================================
// EXAMPLE 3: Parallel Fold (pfoldl)
// =============================================================================
//
// pfoldl reduces a container using a binary operation.
// IMPORTANT: The operation must be ASSOCIATIVE for correct parallel results.
//
// Works by:
// 1. Each chunk computes a partial result
// 2. Partial results are combined
//

void example_parallel_fold()
{
  print_header("Example 3: Parallel Fold (pfoldl)");
  
  std::cout << "GOAL: Compute sum and product of a large dataset in parallel.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Create data
  std::vector<double> data(100000);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = 1.0 + 1.0 / (i + 1);  // 2, 1.5, 1.333..., etc.
  
  std::cout << "Data size: " << data.size() << " elements\n\n";
  
  // SUM with pfoldl
  auto sum = pfoldl(pool, data, 0.0, std::plus<double>());
  std::cout << "Sum: " << std::fixed << std::setprecision(2) << sum << "\n";
  
  // Using psum (convenience function)
  auto sum2 = psum(pool, data);
  std::cout << "Sum (psum): " << sum2 << "\n\n";
  
  // CONCATENATION of strings (associative)
  std::vector<std::string> words = {"Parallel", " ", "functional", " ", 
                                     "programming", " ", "is", " ", "powerful!"};
  
  auto sentence = pfoldl(pool, words, std::string{}, std::plus<std::string>());
  std::cout << "Concatenated: \"" << sentence << "\"\n\n";
  
  std::cout << "✓ Fold operations completed\n";
}

// =============================================================================
// EXAMPLE 4: Parallel Predicates (pall, pexists, pnone, pcount_if)
// =============================================================================
//
// These functions test conditions on all elements:
// - pall:   true if ALL elements satisfy the predicate
// - pexists: true if ANY element satisfies the predicate
// - pnone:  true if NO element satisfies the predicate
// - pcount_if: counts elements satisfying the predicate
//
// pall and pexists use SHORT-CIRCUIT evaluation - they stop early when possible.
//

void example_parallel_predicates()
{
  print_header("Example 4: Parallel Predicates");
  
  std::cout << "GOAL: Test conditions on large datasets efficiently.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Create test data
  std::vector<int> data(1000000);
  std::iota(data.begin(), data.end(), 1);  // 1, 2, 3, ..., 1000000
  
  std::cout << "Dataset: integers 1 to " << data.size() << "\n\n";
  
  // pall - All positive?
  bool all_positive = pall(pool, data, [](int x) { return x > 0; });
  std::cout << "All positive? " << (all_positive ? "YES" : "NO") << "\n";
  
  // pall - All even? (will short-circuit on first odd)
  bool all_even = pall(pool, data, [](int x) { return x % 2 == 0; });
  std::cout << "All even? " << (all_even ? "YES" : "NO") << " (short-circuits early!)\n";
  
  // pexists - Any divisible by 12345?
  bool has_special = pexists(pool, data, [](int x) { return x % 12345 == 0; });
  std::cout << "Has number divisible by 12345? " << (has_special ? "YES" : "NO") << "\n";
  
  // pnone - No negatives?
  bool no_negatives = pnone(pool, data, [](int x) { return x < 0; });
  std::cout << "No negatives? " << (no_negatives ? "YES" : "NO") << "\n";
  
  // pcount_if - Count multiples of 7
  size_t sevens = pcount_if(pool, data, [](int x) { return x % 7 == 0; });
  std::cout << "Multiples of 7: " << sevens << "\n\n";
  
  std::cout << "✓ Predicate tests completed\n";
}

// =============================================================================
// EXAMPLE 5: Parallel Find (pfind, pfind_value)
// =============================================================================
//
// Parallel search with short-circuit optimization.
// - pfind: returns std::optional<size_t> (index of first match)
// - pfind_value: returns std::optional<T> (the matched element)
//

void example_parallel_find()
{
  print_header("Example 5: Parallel Find (pfind, pfind_value)");
  
  std::cout << "GOAL: Search for elements in parallel with early termination.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Create shuffled data
  std::vector<int> data(1000000);
  std::iota(data.begin(), data.end(), 0);
  std::mt19937 rng(42);
  std::shuffle(data.begin(), data.end(), rng);
  
  std::cout << "Shuffled dataset of " << data.size() << " elements\n\n";
  
  // Find index of value 500000
  auto idx = pfind(pool, data, [](int x) { return x == 500000; });
  if (idx)
    std::cout << "Value 500000 found at index " << *idx << "\n";
  else
    std::cout << "Value 500000 not found\n";
  
  // Find first value > 999990
  auto val = pfind_value(pool, data, [](int x) { return x > 999990; });
  if (val)
    std::cout << "First value > 999990: " << *val << "\n";
  else
    std::cout << "No value > 999990\n";
  
  // Search for non-existent value
  auto missing = pfind(pool, data, [](int x) { return x == -1; });
  std::cout << "Value -1: " << (missing ? "found" : "not found") << "\n\n";
  
  std::cout << "✓ Search operations completed\n";
}

// =============================================================================
// EXAMPLE 6: Parallel Aggregations (psum, pproduct, pmin, pmax, pminmax)
// =============================================================================
//
// Convenience functions for common reductions.
//

void example_parallel_aggregations()
{
  print_header("Example 6: Parallel Aggregations");
  
  std::cout << "GOAL: Compute statistics on large datasets in parallel.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Create random data
  std::vector<double> data(500000);
  std::mt19937 rng(123);
  std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
  for (auto& x : data)
    x = dist(rng);
  
  std::cout << "Dataset: " << data.size() << " random doubles in [-1000, 1000]\n\n";
  
  // Aggregations
  auto sum = psum(pool, data);
  auto min_opt = pmin(pool, data);
  auto max_opt = pmax(pool, data);
  auto minmax_opt = pminmax(pool, data);
  
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "Sum:     " << sum << "\n";
  
  if (min_opt)
    std::cout << "Min:     " << *min_opt << "\n";
  if (max_opt)
    std::cout << "Max:     " << *max_opt << "\n";
  if (minmax_opt)
    std::cout << "MinMax:  (" << minmax_opt->first << ", " << minmax_opt->second << ")\n";
  
  double mean = sum / data.size();
  std::cout << "Mean:    " << mean << "\n\n";
  
  std::cout << "✓ Aggregation operations completed\n";
}

// =============================================================================
// EXAMPLE 7: Parallel Sort (psort)
// =============================================================================
//
// Parallel merge sort: chunks are sorted in parallel, then merged.
//

void example_parallel_sort()
{
  print_header("Example 7: Parallel Sort (psort)");
  
  std::cout << "GOAL: Sort large datasets using parallel merge sort.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Create random data
  std::vector<int> data(500000);
  std::mt19937 rng(456);
  for (auto& x : data)
    x = rng() % 1000000;
  
  std::cout << "Dataset: " << data.size() << " random integers\n";
  std::cout << "First 10 (unsorted): ";
  for (size_t i = 0; i < 10; ++i)
    std::cout << data[i] << " ";
  std::cout << "\n\n";
  
  auto start = std::chrono::high_resolution_clock::now();
  
  psort(pool, data);  // In-place parallel sort
  
  auto end = std::chrono::high_resolution_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  
  std::cout << "First 10 (sorted): ";
  for (size_t i = 0; i < 10; ++i)
    std::cout << data[i] << " ";
  std::cout << "\n";
  
  std::cout << "Last 10 (sorted):  ";
  for (size_t i = data.size() - 10; i < data.size(); ++i)
    std::cout << data[i] << " ";
  std::cout << "\n\n";
  
  // Verify sorted
  bool is_sorted = std::is_sorted(data.begin(), data.end());
  std::cout << "Correctly sorted? " << (is_sorted ? "YES" : "NO") << "\n";
  std::cout << "Time: " << ms << " ms\n\n";
  
  std::cout << "✓ Parallel sort completed\n";
}

// =============================================================================
// EXAMPLE 8: Parallel Zip Operations (2 containers)
// =============================================================================
//
// pzip_for_each: Apply function to pairs of elements
// pzip_maps: Map pairs to a new container
// pzip_foldl: Reduce pairs (e.g., dot product)
//

void example_parallel_zip()
{
  print_header("Example 8: Parallel Zip Operations (2 containers)");
  
  std::cout << "GOAL: Process corresponding elements from two containers.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Create two vectors
  std::vector<double> a(100000);
  std::vector<double> b(100000);
  for (size_t i = 0; i < a.size(); ++i)
    {
      a[i] = static_cast<double>(i);
      b[i] = static_cast<double>(i) * 2;
    }
  
  std::cout << "Vectors a and b, each with " << a.size() << " elements\n\n";
  
  // pzip_maps: Element-wise product
  auto products = pzip_maps(pool, a, b, [](double x, double y) { 
    return x * y; 
  });
  
  std::cout << "Element-wise products (first 5): ";
  for (size_t i = 0; i < 5; ++i)
    std::cout << products[i] << " ";
  std::cout << "\n\n";
  
  // pzip_foldl: Dot product
  double dot_product = pzip_foldl(pool, a, b, 0.0,
    [](double acc, double x, double y) { return acc + x * y; });
  
  std::cout << "Dot product: " << std::fixed << std::setprecision(0) 
            << dot_product << "\n\n";
  
  // pzip_for_each with side effects
  std::atomic<double> sum{0};
  pzip_for_each(pool, a, b, [&sum](double x, double y) {
    sum += x + y;
  });
  
  std::cout << "Sum of all pairs: " << sum.load() << "\n\n";
  
  std::cout << "✓ Zip operations completed\n";
}

// =============================================================================
// EXAMPLE 9: Variadic Zip (N containers) - pzip_*_n functions
// =============================================================================
//
// For 3+ containers, use the _n suffix functions:
// - pzip_for_each_n
// - pzip_maps_n
// - pzip_foldl_n (requires a combiner function)
// - pzip_all_n, pzip_exists_n, pzip_count_if_n
//

void example_variadic_zip()
{
  print_header("Example 9: Variadic Zip (N containers)");
  
  std::cout << "GOAL: Process corresponding elements from 3+ containers.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Three vectors
  std::vector<int> x = {1, 2, 3, 4, 5};
  std::vector<int> y = {10, 20, 30, 40, 50};
  std::vector<int> z = {100, 200, 300, 400, 500};
  
  std::cout << "x = {1, 2, 3, 4, 5}\n";
  std::cout << "y = {10, 20, 30, 40, 50}\n";
  std::cout << "z = {100, 200, 300, 400, 500}\n\n";
  
  // pzip_maps_n: Sum triplets
  auto sums = pzip_maps_n(pool, [](int a, int b, int c) {
    return a + b + c;
  }, x, y, z);
  
  std::cout << "x + y + z = ";
  for (auto v : sums)
    std::cout << v << " ";
  std::cout << "\n\n";
  
  // pzip_all_n: Check if all triplets satisfy condition
  bool all_ordered = pzip_all_n(pool, [](int a, int b, int c) {
    return a < b && b < c;
  }, x, y, z);
  
  std::cout << "All x[i] < y[i] < z[i]? " << (all_ordered ? "YES" : "NO") << "\n";
  
  // pzip_count_if_n: Count triplets with sum > 100
  size_t count = pzip_count_if_n(pool, [](int a, int b, int c) {
    return a + b + c > 100;
  }, x, y, z);
  
  std::cout << "Triplets with sum > 100: " << count << "\n\n";
  
  // Four vectors example
  std::vector<double> v1 = {1.0, 2.0, 3.0};
  std::vector<double> v2 = {1.0, 2.0, 3.0};
  std::vector<double> v3 = {1.0, 2.0, 3.0};
  std::vector<double> v4 = {1.0, 2.0, 3.0};
  
  auto products = pzip_maps_n(pool, [](double a, double b, double c, double d) {
    return a * b * c * d;
  }, v1, v2, v3, v4);
  
  std::cout << "v1 * v2 * v3 * v4 = ";
  for (auto v : products)
    std::cout << v << " ";
  std::cout << "\n\n";
  
  std::cout << "✓ Variadic zip operations completed\n";
}

// =============================================================================
// EXAMPLE 10: Parallel Enumerate
// =============================================================================
//
// Like Python's enumerate(), but parallel:
// - penumerate_for_each: Apply function to (index, element) pairs
// - penumerate_maps: Map (index, element) pairs to results
//

void example_parallel_enumerate()
{
  print_header("Example 10: Parallel Enumerate");
  
  std::cout << "GOAL: Process elements along with their indices in parallel.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Initialize vector with indices
  std::vector<int> data(10, 0);
  
  // penumerate_for_each: Set each element to its index * 10
  penumerate_for_each(pool, data, [](size_t i, int& x) {
    x = static_cast<int>(i * 10);
  });
  
  std::cout << "After penumerate_for_each (x = i * 10): ";
  for (auto x : data)
    std::cout << x << " ";
  std::cout << "\n\n";
  
  // penumerate_maps: Create indexed strings
  std::vector<std::string> words = {"apple", "banana", "cherry", "date", "elderberry"};
  
  auto indexed = penumerate_maps(pool, words, 
    [](size_t i, const std::string& s) {
      return "[" + std::to_string(i) + "] " + s;
    });
  
  std::cout << "Indexed strings:\n";
  for (const auto& s : indexed)
    std::cout << "  " << s << "\n";
  std::cout << "\n";
  
  std::cout << "✓ Enumerate operations completed\n";
}

// =============================================================================
// EXAMPLE 11: Performance Comparison
// =============================================================================

void example_performance_comparison()
{
  print_header("Example 11: Performance Comparison");
  
  std::cout << "GOAL: Compare parallel vs sequential execution times.\n\n";
  
  ThreadPool pool(std::thread::hardware_concurrency());
  
  // Large dataset
  std::vector<double> data(2000000);
  std::iota(data.begin(), data.end(), 1.0);
  
  std::cout << "Dataset: " << data.size() << " elements\n";
  std::cout << "Threads: " << pool.num_threads() << "\n\n";
  
  // CPU-intensive operation
  auto expensive = [](double x) {
    double result = x;
    for (int i = 0; i < 50; ++i)
      result = std::sin(result) * std::cos(result) + std::sqrt(std::abs(result));
    return result;
  };
  
  // SEQUENTIAL
  std::cout << "Running sequential map...\n";
  auto seq_start = std::chrono::high_resolution_clock::now();
  
  std::vector<double> seq_result(data.size());
  std::transform(data.begin(), data.end(), seq_result.begin(), expensive);
  
  auto seq_end = std::chrono::high_resolution_clock::now();
  auto seq_ms = std::chrono::duration_cast<std::chrono::milliseconds>(seq_end - seq_start).count();
  
  // PARALLEL
  std::cout << "Running parallel map (pmaps)...\n\n";
  auto par_start = std::chrono::high_resolution_clock::now();
  
  auto par_result = pmaps(pool, data, expensive);
  
  auto par_end = std::chrono::high_resolution_clock::now();
  auto par_ms = std::chrono::duration_cast<std::chrono::milliseconds>(par_end - par_start).count();
  
  // Results
  double speedup = (par_ms > 0) ? static_cast<double>(seq_ms) / par_ms : 0;
  
  std::cout << "┌────────────────────────────────────────┐\n";
  std::cout << "│           PERFORMANCE RESULTS          │\n";
  std::cout << "├────────────────────────────────────────┤\n";
  std::cout << "│ Sequential:   " << std::setw(20) << seq_ms << " ms │\n";
  std::cout << "│ Parallel:     " << std::setw(20) << par_ms << " ms │\n";
  std::cout << "├────────────────────────────────────────┤\n";
  std::cout << "│ SPEEDUP:      " << std::setw(20) << std::fixed 
            << std::setprecision(2) << speedup << "x │\n";
  std::cout << "└────────────────────────────────────────┘\n";
  
  // Verify correctness
  bool match = std::equal(seq_result.begin(), seq_result.end(), par_result.begin(),
    [](double a, double b) { return std::abs(a - b) < 1e-10; });
  std::cout << "\n✓ Results match: " << (match ? "YES" : "NO") << "\n";
}

// =============================================================================
// MAIN
// =============================================================================

int main()
{
  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║                                                                    ║\n";
  std::cout << "║         ALEPH-W PARALLEL FUNCTIONAL PROGRAMMING EXAMPLES           ║\n";
  std::cout << "║                                                                    ║\n";
  std::cout << "║   ML-style operations (map, filter, fold, zip, etc.)               ║\n";
  std::cout << "║   accelerated with multi-threading via ThreadPool                  ║\n";
  std::cout << "║                                                                    ║\n";
  std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
  
  std::cout << "\nThis program demonstrates 11 parallel functional programming patterns.\n";
  std::cout << "Read the source code comments for detailed explanations.\n";
  
  example_parallel_map();
  example_parallel_filter();
  example_parallel_fold();
  example_parallel_predicates();
  example_parallel_find();
  example_parallel_aggregations();
  example_parallel_sort();
  example_parallel_zip();
  example_variadic_zip();
  example_parallel_enumerate();
  example_performance_comparison();
  
  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║  ✓ ALL EXAMPLES COMPLETED SUCCESSFULLY                             ║\n";
  std::cout << "║                                                                    ║\n";
  std::cout << "║  QUICK REFERENCE:                                                  ║\n";
  std::cout << "║    pmaps(pool, c, f)          → parallel map                       ║\n";
  std::cout << "║    pfilter(pool, c, pred)     → parallel filter                    ║\n";
  std::cout << "║    pfoldl(pool, c, init, op)  → parallel fold                      ║\n";
  std::cout << "║    pfor_each(pool, c, f)      → parallel for_each                  ║\n";
  std::cout << "║    pall/pexists/pnone         → parallel predicates                ║\n";
  std::cout << "║    pfind/pfind_value          → parallel search                    ║\n";
  std::cout << "║    psum/pproduct/pmin/pmax    → parallel aggregations              ║\n";
  std::cout << "║    psort                      → parallel merge sort                ║\n";
  std::cout << "║    pzip_*                     → parallel zip (2 containers)        ║\n";
  std::cout << "║    pzip_*_n                   → parallel zip (N containers)        ║\n";
  std::cout << "║    penumerate_*               → parallel enumerate                 ║\n";
  std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
  
  return 0;
}
