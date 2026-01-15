/**
 * @file functional_example.C
 * @brief Comprehensive example of functional programming in Aleph-w
 *
 * This program demonstrates all major functional programming features available
 * in Aleph-w through `ahFunctional.H`. Functional programming provides a
 * declarative, composable approach to data processing that is often more
 * readable and less error-prone than imperative loops.
 *
 * ## What is Functional Programming?
 *
 * Functional programming emphasizes:
 * - **Immutability**: Operations don't modify original data
 * - **Composability**: Small functions combine into larger operations
 * - **Declarative**: Describe *what* you want, not *how* to do it
 * - **Higher-order functions**: Functions that take/return functions
 *
 * **Benefits**:
 * - More readable code
 * - Easier to reason about
 * - Better for parallelization
 * - Less error-prone (no mutation bugs)
 *
## Features Demonstrated
 *
### Range Generation
 *
 * Create sequences of values:
 * - **`range(start, end, step)`**: Generate numeric ranges (like Python's range)
 * - **`nrange(start, end, n)`**: Generate n evenly spaced values
 * - **`contiguous_range(start, n)`**: Generate n consecutive values
 * - **`rep(n, value)`**: Repeat a value n times
 *
 * **Example**: `range(1, 10, 2)` → [1, 3, 5, 7, 9]
 *
### Iteration
 *
 * Apply operations to containers:
 * - **`for_each(container, op)`**: Apply function to each element
 * - **`enum_for_each(container, op)`**: Apply with index (i, element)
 * - **`traverse(container, op)`**: Conditional traversal (can stop early)
 *
### Predicates
 *
 * Test conditions on containers:
 * - **`all(container, pred)`**: All elements satisfy predicate?
 * - **`exists(container, pred)`**: At least one satisfies?
 * - **`none(container, pred)`**: No element satisfies?
 * - **`contains(container, value)`**: Value exists in container?
 *
### Transformation
 *
 * Transform containers into new containers:
 * - **`maps<T>(container, op)`**: Transform each element (like std::transform)
 * - **`filter(container, pred)`**: Keep elements satisfying predicate
 * - **`flat_map(container, op)`**: Map and flatten nested results
 * - **`reverse(container)`**: Reverse order of elements
 * - **`flatten(container)`**: Flatten nested containers
 *
### Folding/Reduction
 *
 * Combine elements into a single value:
 * - **`foldl(container, init, op)`**: Left fold (reduce from left)
 * - **`foldr(container, init, op)`**: Right fold (reduce from right)
 * - **`sum(container)`**: Sum all numeric elements
 * - **`product(container)`**: Multiply all numeric elements
 *
 * **Example**: `foldl([1,2,3], 0, +)` → 6 (sum)
 *
### Zipping
 *
 * Combine multiple containers element-wise:
 * - **`zip(c1, c2)`**: Create pairs from two containers
 * - **`zipEq(c1, c2)`**: Zip with length equality check
 * - **`unzip(container)`**: Split pairs back into two containers
 * - **`zip_longest(c1, c2, d1, d2)`**: Zip with defaults for shorter container
 *
 * **Example**: `zip([1,2,3], ['a','b','c'])` → [(1,'a'), (2,'b'), (3,'c')]
 *
### Grouping
 *
 * Organize elements by criteria:
 * - **`group_by(container, key_func)`**: Group elements by key function
 * - **`partition(container, pred)`**: Split into two groups (true/false)
 * - **`take_while(container, pred)`**: Take prefix satisfying predicate
 * - **`drop_while(container, pred)`**: Drop prefix satisfying predicate
 *
## Functional Style Example
 *
 * **Imperative style** (traditional):
 * ```cpp
 * vector<int> result;
 * for (int x : data) {
 *   if (x > 0) {
 *     result.push_back(x * 2);
 *   }
 * }
 * ```
 *
 * **Functional style** (Aleph-w):
 * ```cpp
 * auto result = filter(data, [](int x) { return x > 0; })
 *               .maps<int>([](int x) { return x * 2; });
 * ```
 *
 * More concise, readable, and composable!
 *
## Composition and Pipelining
 *
 * Functional operations compose naturally:
 * ```cpp
 * // Process data through pipeline
 * auto result = data
 *   | filter(is_positive)      // Keep positive numbers
 *   | maps(square)             // Square them
 *   | filter(is_even)          // Keep even results
 *   | sum();                   // Sum everything
 * ```
 *
## Comparison with STL
 *
 * | Feature | STL | Aleph-w Functional |
 * |---------|-----|-------------------|
 * | Transform | std::transform | maps() |
 * | Filter | Manual loop | filter() |
 * | Reduce | std::accumulate | foldl() |
 * | Composition | Manual chaining | Natural composition |
 * | Immutability | Modifies input | Returns new container |
 *
## Performance Considerations
 *
 * - **Immutability**: Creates new containers (memory overhead)
 * - **Composition**: Can be optimized by compiler
 * - **Lazy evaluation**: Some operations can be deferred (see ranges_example.C)
 * - **Parallelization**: Functional code easier to parallelize
 *
## Usage Examples
 *
 * ```bash
 * # Run all demonstrations
 * ./functional_example
 *
 * # Run specific section
 * ./functional_example -s ranges    # Range generation
 * ./functional_example -s fold      # Folding operations
 * ./functional_example -s zip       # Zipping operations
 * ```
 *
 * @see ahFunctional.H Main functional programming header
 * @see ranges_example.C C++20 Ranges (lazy evaluation)
 * @see uni_functional_example.C Unified functional (works with STL too)
 * @see ah-dry.H Container mixins for functional methods
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>

#include <tclap/CmdLine.h>

#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <htlist.H>
#include <ahFunctional.H>
#include <ahSort.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Helper functions for printing
// =============================================================================

template <typename Container>
void print_container(const string& label, const Container& c)
{
  cout << label << ": [";
  bool first = true;
  c.for_each([&first](const auto& x) {
    if (not first) cout << ", ";
    cout << x;
    first = false;
  });
  cout << "]" << endl;
}

template <typename T1, typename T2>
void print_pairs(const string& label, const DynList<pair<T1, T2>>& c)
{
  cout << label << ": [";
  bool first = true;
  c.for_each([&first](const auto& p) {
    if (not first) cout << ", ";
    cout << "(" << p.first << ", " << p.second << ")";
    first = false;
  });
  cout << "]" << endl;
}

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
// 1. Range Generation
// =============================================================================

void demo_ranges()
{
  print_section("RANGE GENERATION");
  
  // Basic range: range(start, end, step)
  print_subsection("range(start, end, step)");
  auto r1 = range(1, 10);           // 1 to 10, step 1
  auto r2 = range(0, 20, 5);        // 0 to 20, step 5
  auto r3 = range(10, 1, -2);       // 10 to 1, step -2 (empty, doesn't work backwards)
  
  print_container("range(1, 10)", r1);
  print_container("range(0, 20, 5)", r2);
  print_container("range(10, 1, -2)", r3);
  cout << "  Note: range() only works with positive steps\n";
  
  // Single argument range: range(n) = 0, 1, ..., n-1
  print_subsection("range(n) - generates 0 to n-1");
  auto r4 = range(5);
  print_container("range(5)", r4);
  
  // nrange: exactly n values evenly spaced
  print_subsection("nrange(start, end, n) - n evenly spaced values");
  auto nr1 = nrange(0.0, 1.0, 5);   // 5 values from 0 to 1
  auto nr2 = nrange(0.0, 10.0, 11); // 11 values from 0 to 10
  
  cout << "nrange(0.0, 1.0, 5): [";
  bool first = true;
  nr1.for_each([&first](double x) {
    if (not first) cout << ", ";
    cout << fixed << setprecision(2) << x;
    first = false;
  });
  cout << "]" << endl;
  
  cout << "nrange(0.0, 10.0, 11): [";
  first = true;
  nr2.for_each([&first](double x) {
    if (not first) cout << ", ";
    cout << fixed << setprecision(1) << x;
    first = false;
  });
  cout << "]" << endl;
  
  // contiguous_range: n consecutive values
  print_subsection("contiguous_range(start, n) - n consecutive values");
  auto cr1 = contiguous_range(100, 5);
  auto cr2 = contiguous_range(-3, 7);
  print_container("contiguous_range(100, 5)", cr1);
  print_container("contiguous_range(-3, 7)", cr2);
  
  // rep: repeat a value
  print_subsection("rep(n, value) - repeat value n times");
  auto rep1 = rep(5, 42);
  auto rep2 = rep<string>(3, "hello");
  print_container("rep(5, 42)", rep1);
  print_container("rep(3, \"hello\")", rep2);
}

// =============================================================================
// 2. Iteration
// =============================================================================

void demo_iteration()
{
  print_section("ITERATION");
  
  DynList<int> nums = {1, 2, 3, 4, 5};
  
  // for_each: apply operation to each element
  print_subsection("for_each(container, op)");
  cout << "Elements: ";
  nums.for_each([](int x) { cout << x << " "; });
  cout << endl;
  
  // Method chaining
  cout << "Squared:  ";
  nums.for_each([](int x) { cout << (x * x) << " "; });
  cout << endl;
  
  // enum_for_each: with index
  print_subsection("enum_for_each(container, op) - with index");
  DynList<string> names = {"Alice", "Bob", "Carol", "Dave"};
  cout << "Indexed list:\n";
  enum_for_each(names, [](const string& name, size_t i) {
    cout << "  [" << i << "] " << name << endl;
  });
  
  // traverse: conditional traversal (stops on false)
  print_subsection("traverse(container, op) - stops on false");
  cout << "Print until finding 3: ";
  nums.traverse([](int x) {
    cout << x << " ";
    return x != 3;  // stop when x == 3
  });
  cout << "(stopped)" << endl;
}

// =============================================================================
// 3. Predicates
// =============================================================================

void demo_predicates()
{
  print_section("PREDICATES");
  
  DynList<int> nums = {2, 4, 6, 8, 10};
  DynList<int> mixed = {1, 2, 3, 4, 5};
  DynList<int> empty_list;
  
  print_container("nums", nums);
  print_container("mixed", mixed);
  
  // all: check if all elements satisfy predicate
  print_subsection("all(container, pred)");
  auto is_even = [](int x) { return x % 2 == 0; };
  auto is_positive = [](int x) { return x > 0; };
  auto is_less_than_20 = [](int x) { return x < 20; };
  
  cout << "All even in nums?  " << (nums.all(is_even) ? "yes" : "no") << endl;
  cout << "All even in mixed? " << (mixed.all(is_even) ? "yes" : "no") << endl;
  cout << "All positive in nums? " << (nums.all(is_positive) ? "yes" : "no") << endl;
  cout << "All < 20 in nums? " << (nums.all(is_less_than_20) ? "yes" : "no") << endl;
  cout << "All in empty list? " << (empty_list.all(is_even) ? "yes (vacuous truth)" : "no") << endl;
  
  // exists: check if at least one satisfies
  print_subsection("exists(container, pred)");
  auto is_five = [](int x) { return x == 5; };
  auto is_greater_than_7 = [](int x) { return x > 7; };
  
  cout << "Exists 5 in nums?  " << (nums.exists(is_five) ? "yes" : "no") << endl;
  cout << "Exists 5 in mixed? " << (mixed.exists(is_five) ? "yes" : "no") << endl;
  cout << "Exists > 7 in nums? " << (nums.exists(is_greater_than_7) ? "yes" : "no") << endl;
  
  // none: check if no element satisfies
  print_subsection("none(container, pred)");
  auto is_negative = [](int x) { return x < 0; };
  auto is_odd = [](int x) { return x % 2 != 0; };
  
  cout << "None negative in nums? " << (none(nums, is_negative) ? "yes" : "no") << endl;
  cout << "None odd in nums? " << (none(nums, is_odd) ? "yes" : "no") << endl;
  cout << "None odd in mixed? " << (none(mixed, is_odd) ? "yes" : "no") << endl;
  
  // contains: check if value exists
  print_subsection("contains(container, value)");
  cout << "nums contains 6? " << (contains(nums, 6) ? "yes" : "no") << endl;
  cout << "nums contains 7? " << (contains(nums, 7) ? "yes" : "no") << endl;
}

// =============================================================================
// 4. Transformation (map, filter)
// =============================================================================

void demo_transformation()
{
  print_section("TRANSFORMATION");
  
  DynList<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  print_container("Original", nums);
  
  // filter: keep elements satisfying predicate
  print_subsection("filter(container, pred)");
  auto evens = nums.filter([](int x) { return x % 2 == 0; });
  auto greater_than_5 = nums.filter([](int x) { return x > 5; });
  auto primes = nums.filter([](int x) {
    if (x < 2) return false;
    for (int i = 2; i * i <= x; ++i)
      if (x % i == 0) return false;
    return true;
  });
  
  print_container("Even numbers", evens);
  print_container("Greater than 5", greater_than_5);
  print_container("Primes", primes);
  
  // maps: transform elements
  print_subsection("maps<T>(container, op)");
  auto squares = nums.template maps<int>([](int x) { return x * x; });
  auto doubled = nums.template maps<int>([](int x) { return x * 2; });
  auto as_strings = nums.template maps<string>([](int x) { 
    return "n" + to_string(x); 
  });
  
  print_container("Squares", squares);
  print_container("Doubled", doubled);
  print_container("As strings", as_strings);
  
  // Chaining filter and map
  print_subsection("Chaining: filter then map");
  auto even_squares = nums
    .filter([](int x) { return x % 2 == 0; })
    .template maps<int>([](int x) { return x * x; });
  print_container("Even numbers squared", even_squares);
  
  // reverse
  print_subsection("reverse(container)");
  auto reversed = reverse(nums);
  print_container("Reversed", reversed);
  
  // flat_map: map and flatten
  print_subsection("flat_map(container, op) - map then flatten");
  DynList<int> small = {1, 2, 3};
  auto expanded = flat_map(small, [](int x) {
    return DynList<int>({x, x * 10, x * 100});
  });
  print_container("flat_map({1,2,3}, x -> {x, x*10, x*100})", expanded);
  
  // flatten: flatten nested containers
  print_subsection("flatten(container) - flatten nested lists");
  DynList<DynList<int>> nested;
  nested.append(DynList<int>({1, 2}));
  nested.append(DynList<int>({3, 4, 5}));
  nested.append(DynList<int>({6}));
  
  cout << "Nested: [[1,2], [3,4,5], [6]]\n";
  auto flattened = flatten(nested);
  print_container("Flattened", flattened);
}

// =============================================================================
// 5. Folding/Reduction
// =============================================================================

void demo_folding()
{
  print_section("FOLDING / REDUCTION");
  
  DynList<int> nums = {1, 2, 3, 4, 5};
  print_container("nums", nums);
  
  // foldl: left fold (reduce from left)
  print_subsection("foldl(container, init, op) - left fold");
  
  // Sum using foldl
  int sum_result = nums.template foldl<int>(0, [](int acc, int x) { 
    return acc + x; 
  });
  cout << "Sum (foldl): " << sum_result << endl;
  
  // Product using foldl
  int prod_result = nums.template foldl<int>(1, [](int acc, int x) { 
    return acc * x; 
  });
  cout << "Product (foldl): " << prod_result << endl;
  
  // Max using foldl
  int max_result = nums.template foldl<int>(nums.get_first(), [](int acc, int x) { 
    return x > acc ? x : acc; 
  });
  cout << "Max (foldl): " << max_result << endl;
  
  // String concatenation
  DynList<string> words = {"Hello", " ", "World", "!"};
  string concat = words.template foldl<string>("", [](const string& acc, const string& s) {
    return acc + s;
  });
  cout << "Concatenation: \"" << concat << "\"" << endl;
  
  // Demonstrate fold order
  print_subsection("Fold direction matters!");
  DynList<int> seq = {1, 2, 3};
  
  // foldl: ((init op 1) op 2) op 3
  string left = seq.template foldl<string>("", [](const string& acc, int x) {
    return "(" + acc + "+" + to_string(x) + ")";
  });
  cout << "foldl with +: " << left << endl;
  cout << "  Evaluation: ((\"\" + 1) + 2) + 3" << endl;
  
  // For foldr, show the concept with a reversed list
  auto rev_seq = reverse(seq);
  string right = rev_seq.template foldl<string>("", [](const string& acc, int x) {
    return "(" + to_string(x) + "+" + acc + ")";
  });
  cout << "Right-to-left fold: " << right << endl;
  cout << "  Evaluation: 1 + (2 + (3 + \"\"))" << endl;
  
  // Convenience functions: sum and product
  print_subsection("sum(container) and product(container)");
  cout << "sum({1,2,3,4,5}) = " << sum(nums) << endl;
  cout << "product({1,2,3,4,5}) = " << product(nums) << endl;
  
  DynList<double> doubles = {1.5, 2.0, 3.5};
  cout << "sum({1.5, 2.0, 3.5}) = " << sum(doubles) << endl;
}

// =============================================================================
// 6. Zipping
// =============================================================================

void demo_zipping()
{
  print_section("ZIPPING");
  
  DynList<int> nums = {1, 2, 3, 4};
  DynList<string> letters = {"a", "b", "c", "d"};
  DynList<double> values = {1.1, 2.2, 3.3};
  
  print_container("nums", nums);
  print_container("letters", letters);
  print_container("values (shorter)", values);
  
  // zip: combine two containers
  print_subsection("zip(c1, c2) - stops at shorter");
  auto zipped = zip(nums, letters);
  print_pairs("zip(nums, letters)", zipped);
  
  auto zipped_short = zip(nums, values);
  cout << "zip(nums, values): [";
  bool first = true;
  zipped_short.for_each([&first](const auto& p) {
    if (not first) cout << ", ";
    cout << "(" << p.first << ", " << p.second << ")";
    first = false;
  });
  cout << "] (stops at shorter)\n";
  
  // Using ZipIterator for lazy evaluation
  print_subsection("ZipIterator - lazy zipping");
  cout << "Iterating with ZipIterator:\n";
  for (ZipIterator it(nums, letters); it.has_curr(); it.next())
  {
    auto [n, l] = it.get_curr();
    cout << "  " << n << " -> " << l << endl;
  }
  
  // unzip: separate pairs
  print_subsection("unzip(pairs) - separate into two lists");
  DynList<pair<int, string>> pairs;
  pairs.append({1, "one"});
  pairs.append({2, "two"});
  pairs.append({3, "three"});
  
  auto [first_list, second_list] = unzip(pairs);
  print_container("First elements", first_list);
  print_container("Second elements", second_list);
  
  // zip with operation
  print_subsection("Combining zipped elements");
  DynList<int> a = {1, 2, 3};
  DynList<int> b = {10, 20, 30};
  
  auto sums = zip(a, b).template maps<int>([](const auto& p) {
    return p.first + p.second;
  });
  auto products = zip(a, b).template maps<int>([](const auto& p) {
    return p.first * p.second;
  });
  
  print_container("Pairwise sums", sums);
  print_container("Pairwise products", products);
}

// =============================================================================
// 7. Grouping and Partitioning
// =============================================================================

void demo_grouping()
{
  print_section("GROUPING AND PARTITIONING");
  
  DynList<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  print_container("nums", nums);
  
  // partition: split by predicate
  print_subsection("partition(container, pred) - split by predicate");
  auto [evens, odds] = partition(nums, [](int x) { return x % 2 == 0; });
  print_container("Even (satisfies)", evens);
  print_container("Odd (doesn't satisfy)", odds);
  
  // take_while: take prefix while predicate holds
  print_subsection("take_while(container, pred) - take prefix");
  auto prefix = take_while(nums, [](int x) { return x < 5; });
  print_container("take_while(< 5)", prefix);
  
  // drop_while: drop prefix while predicate holds
  print_subsection("drop_while(container, pred) - drop prefix");
  auto suffix = drop_while(nums, [](int x) { return x < 5; });
  print_container("drop_while(< 5)", suffix);
  
  // group_by: group elements by key
  print_subsection("group_by(container, key_func)");
  
  // Group by remainder mod 3
  auto by_mod3 = group_by(nums, [](int x) { return x % 3; });
  cout << "Grouped by x % 3:\n";
  by_mod3.for_each([](const auto& group) {
    cout << "  Key " << group.first << ": [";
    bool first = true;
    group.second.for_each([&first](int x) {
      if (not first) cout << ", ";
      cout << x;
      first = false;
    });
    cout << "]\n";
  });
  
  // Group strings by length
  DynList<string> words = {"hi", "hello", "bye", "ok", "world", "no", "yes"};
  print_container("words", words);
  
  auto by_length = group_by(words, [](const string& s) { return s.length(); });
  cout << "Grouped by length:\n";
  by_length.for_each([](const auto& group) {
    cout << "  Length " << group.first << ": [";
    bool first = true;
    group.second.for_each([&first](const string& s) {
      if (not first) cout << ", ";
      cout << "\"" << s << "\"";
      first = false;
    });
    cout << "]\n";
  });
}

// =============================================================================
// 8. Practical Examples
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL EXAMPLES");
  
  // Example 1: Statistics
  print_subsection("Example 1: Computing statistics");
  DynList<double> data = {23.5, 45.2, 12.8, 67.3, 34.1, 89.0, 56.4};
  print_container("Data", data);
  
  double total = sum(data);
  size_t count = data.size();
  double mean = total / count;
  
  // Variance using map and fold
  double variance = data
    .template maps<double>([mean](double x) { return (x - mean) * (x - mean); })
    .template foldl<double>(0.0, [](double acc, double x) { return acc + x; }) / count;
  
  double stddev = sqrt(variance);
  
  cout << "Count: " << count << endl;
  cout << "Sum: " << fixed << setprecision(2) << total << endl;
  cout << "Mean: " << mean << endl;
  cout << "Variance: " << variance << endl;
  cout << "Std Dev: " << stddev << endl;
  
  // Example 2: Word processing
  print_subsection("Example 2: Word processing pipeline");
  DynList<string> text = {"Hello", "WORLD", "this", "IS", "a", "TEST"};
  print_container("Original text", text);
  
  // Convert to lowercase and filter long words
  auto processed = text
    .template maps<string>([](const string& s) {
      string lower;
      for (char c : s) lower += tolower(c);
      return lower;
    })
    .filter([](const string& s) { return s.length() > 2; });
  
  print_container("Lowercase, length > 2", processed);
  
  // Example 3: Matrix operations with nested lists
  print_subsection("Example 3: Matrix transpose using zip");
  DynList<DynList<int>> matrix;
  matrix.append(DynList<int>({1, 2, 3}));
  matrix.append(DynList<int>({4, 5, 6}));
  
  cout << "Matrix:\n";
  matrix.for_each([](const DynList<int>& row) {
    cout << "  ";
    row.for_each([](int x) { cout << x << " "; });
    cout << endl;
  });
  
  // Example 4: Fibonacci using unfold pattern
  print_subsection("Example 4: Generate sequence with fold");
  auto fibonacci = range(10).template foldl<DynList<int>>(
    DynList<int>({0, 1}),
    [](DynList<int> acc, int) {
      size_t n = acc.size();
      int next = 0;
      size_t idx = 0;
      acc.for_each([&](int x) {
        if (idx == n - 2 or idx == n - 1)
          next += x;
        idx++;
      });
      acc.append(next);
      return acc;
    }
  );
  print_container("First 12 Fibonacci numbers", fibonacci);
}

// =============================================================================
// 9. Comparison and Equality
// =============================================================================

void demo_comparison()
{
  print_section("COMPARISON AND EQUALITY");
  
  DynList<int> a = {1, 2, 3, 4, 5};
  DynList<int> b = {1, 2, 3, 4, 5};
  DynList<int> c = {1, 2, 3};
  DynList<int> d = {1, 2, 3, 4, 6};
  
  print_container("a", a);
  print_container("b", b);
  print_container("c", c);
  print_container("d", d);
  
  // eq: check equality
  print_subsection("eq(c1, c2) - element-wise equality");
  cout << "eq(a, b)? " << (eq(a, b) ? "yes" : "no") << endl;
  cout << "eq(a, c)? " << (eq(a, c) ? "yes" : "no") << " (different lengths)" << endl;
  cout << "eq(a, d)? " << (eq(a, d) ? "yes" : "no") << " (different element)" << endl;
  
  // diff: find first difference
  print_subsection("diff(c1, c2) - check if different");
  cout << "diff(a, b)? " << (diff(a, b) ? "yes" : "no") << endl;
  cout << "diff(a, d)? " << (diff(a, d) ? "yes" : "no") << endl;
  
  // lesser: lexicographic comparison
  print_subsection("lesser(c1, c2) - lexicographic less-than");
  DynList<int> x = {1, 2, 3};
  DynList<int> y = {1, 2, 4};
  DynList<int> z = {1, 2};
  
  print_container("x", x);
  print_container("y", y);
  print_container("z", z);
  
  cout << "lesser(x, y)? " << (lesser(x, y) ? "yes" : "no") << " (3 < 4)" << endl;
  cout << "lesser(y, x)? " << (lesser(y, x) ? "yes" : "no") << endl;
  cout << "lesser(z, x)? " << (lesser(z, x) ? "yes" : "no") << " (prefix)" << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Comprehensive functional programming example for Aleph-w.\n"
      "Demonstrates range generation, iteration, predicates, transformation,\n"
      "folding, zipping, grouping, and more.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: ranges, iteration, predicates, transform, "
      "fold, zip, group, practical, compare, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "     ALEPH-W FUNCTIONAL PROGRAMMING EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "ranges")
      demo_ranges();
    
    if (section == "all" or section == "iteration")
      demo_iteration();
    
    if (section == "all" or section == "predicates")
      demo_predicates();
    
    if (section == "all" or section == "transform")
      demo_transformation();
    
    if (section == "all" or section == "fold")
      demo_folding();
    
    if (section == "all" or section == "zip")
      demo_zipping();
    
    if (section == "all" or section == "group")
      demo_grouping();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    if (section == "all" or section == "compare")
      demo_comparison();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Functional programming demo completed!\n";
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

