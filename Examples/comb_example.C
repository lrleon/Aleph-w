/**
 * @file comb_example.C
 * @brief Example demonstrating combinatorics utilities in Aleph-w
 *
 * This program demonstrates combinatorial operations from `ah-comb.H`, providing
 * tools for generating and manipulating combinations, permutations (Cartesian
 * products), and matrix operations. These utilities are essential for solving
 * problems involving discrete mathematics, constraint satisfaction, and
 * exhaustive search.
 *
 * ## Key Concepts
 *
 * ### "Permutations" = Cartesian Product
 *
 * In this context, "permutations" refers to the **Cartesian product** of lists,
 * not traditional mathematical permutations. Given lists of choices, it generates
 * all possible combinations by selecting one element from each list.
 *
 * **Example**:
 * ```
 * Lists: [[a, b], [1, 2], [X, Y]]
 * Result: [a,1,X], [a,1,Y], [a,2,X], [a,2,Y],
 *         [b,1,X], [b,1,Y], [b,2,X], [b,2,Y]
 * ```
 *
 * **Total combinations**: 2 × 2 × 2 = 8
 *
 * ### Combinations
 *
 * Traditional **combinations** are unique sorted selections of k elements from n.
 * Unlike permutations, order doesn't matter in combinations.
 *
 * **Example**: Combinations of 3 from {a, b, c, d}:
 * ```
 * {a,b,c}, {a,b,d}, {a,c,d}, {b,c,d}
 * ```
 *
 * ## Features Demonstrated
 *
 * ### Matrix Operations
 * - **Transposition**: Swap rows and columns
 * - Useful for matrix manipulation and linear algebra
 *
 * ### Permutation Enumeration (Cartesian Products)
 * - Generate all combinations from multiple lists
 * - Lazy evaluation support (generate on demand)
 * - Useful for exhaustive search, constraint satisfaction
 *
 * ### Combination Building
 * - Generate k-combinations from a set
 * - Efficient enumeration algorithms
 * - Useful for subset selection problems
 *
 * ### Fold/Predicate Operations
 * - Apply functions over all permutations
 * - Filter permutations satisfying predicates
 * - Aggregate results (sum, product, etc.)
 *
 * ## Applications
 *
 * ### Constraint Satisfaction
 * - Generate all possible assignments
 * - Test combinations against constraints
 * - Find valid solutions
 *
 * ### Testing
 * - Generate test cases (all combinations of parameters)
 * - Exhaustive testing of configurations
 * - Parameter space exploration
 *
 * ### Game Theory
 * - Enumerate all possible moves
 * - Analyze game trees
 * - Strategy evaluation
 *
 * ### Optimization
 * - Brute-force search over parameter space
 * - Feature selection (all combinations of features)
 * - Configuration optimization
 *
 * ## Complexity Considerations
 *
 * | Operation | Complexity | Notes |
 * |-----------|-----------|-------|
 * | Cartesian Product | O(∏nᵢ) | Product of list sizes |
 * | Combinations | O(C(n,k)) | Binomial coefficient |
 * | Matrix Transpose | O(n×m) | n rows, m columns |
 *
 * **Warning**: Cartesian products grow exponentially! Use with caution for
 * large input lists.
 *
 * ## Usage Examples
 *
 * ```bash
 * # Run all demonstrations
 * ./comb_example
 *
 * # Run specific section
 * ./comb_example -s transpose    # Transpose demo
 * ./comb_example -s perm         # Permutations demo
 * ./comb_example -s predicates   # Predicate utilities on permutations
 * ./comb_example -s traverse     # Traversal utilities
 * ./comb_example -s fold         # Fold/reduce utilities
 * ./comb_example -s build        # Construction helpers
 * ./comb_example -s practical    # Practical applications
 * ```
 *
 * ## Example: Password Generation
 *
 * Generate all possible passwords from character sets:
 * ```
 * Letters: [a-z] (26 choices)
 * Numbers: [0-9] (10 choices)
 * Symbols: [!@#] (3 choices)
 * 
 * Total: 26 × 10 × 3 = 780 combinations
 * ```
 *
 * @see ah-comb.H Combinatorics utilities header
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>

#include <tclap/CmdLine.h>

#include <htlist.H>
#include <ah-comb.H>

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

template <typename T>
void print_list(const string& label, const DynList<T>& l)
{
  cout << label << ": [";
  bool first = true;
  for (auto it = l.get_it(); it.has_curr(); it.next())
  {
    if (not first) cout << ", ";
    cout << it.get_curr();
    first = false;
  }
  cout << "]" << endl;
}

template <typename T>
void print_matrix(const string& label, const DynList<DynList<T>>& mat)
{
  cout << label << ":" << endl;
  size_t row = 0;
  for (auto it = mat.get_it(); it.has_curr(); it.next(), row++)
  {
    cout << "  [" << row << "]: [";
    bool first = true;
    for (auto jt = it.get_curr().get_it(); jt.has_curr(); jt.next())
    {
      if (not first) cout << ", ";
      cout << jt.get_curr();
      first = false;
    }
    cout << "]" << endl;
  }
}

// =============================================================================
// 1. Matrix Transpose
// =============================================================================

void demo_transpose()
{
  print_section("MATRIX TRANSPOSE");
  
  // Create a 3x4 matrix
  DynList<DynList<int>> matrix;
  matrix.append(DynList<int>({1, 2, 3, 4}));
  matrix.append(DynList<int>({5, 6, 7, 8}));
  matrix.append(DynList<int>({9, 10, 11, 12}));
  
  print_matrix("Original matrix (3x4)", matrix);
  
  // Transpose
  print_subsection("transpose()");
  auto transposed = transpose(matrix);
  print_matrix("Transposed (4x3)", transposed);
  
  // In-place transpose
  print_subsection("in_place_transpose()");
  DynList<DynList<string>> names;
  names.append(DynList<string>({"Ana", "Juan"}));
  names.append(DynList<string>({"Maria", "Pedro"}));
  names.append(DynList<string>({"Luisa", "Carlos"}));
  
  print_matrix("Before", names);
  in_place_transpose(names);
  print_matrix("After in-place transpose", names);
}

// =============================================================================
// 2. Permutations (Cartesian Product)
// =============================================================================

void demo_permutations()
{
  print_section("PERMUTATIONS (Cartesian Product)");
  
  cout << "Given lists of choices, enumerate all combinations.\n";
  cout << "This is the CARTESIAN PRODUCT, not mathematical permutations.\n\n";
  
  // Simple example: colors and sizes
  DynList<DynList<string>> choices;
  choices.append(DynList<string>({"rojo", "azul", "verde"}));
  choices.append(DynList<string>({"S", "M", "L"}));
  
  cout << "Choices:" << endl;
  cout << "  Colors: [rojo, azul, verde]" << endl;
  cout << "  Sizes:  [S, M, L]" << endl;
  
  // for_each_perm
  print_subsection("for_each_perm()");
  cout << "All color-size combinations:\n";
  size_t count = 0;
  for_each_perm(choices, [&count](const DynList<string>& perm) {
    cout << "  " << ++count << ": ";
    bool first = true;
    for (auto it = perm.get_it(); it.has_curr(); it.next())
    {
      if (not first) cout << "-";
      cout << it.get_curr();
      first = false;
    }
    cout << endl;
  });
  
  // perm_count
  print_subsection("perm_count()");
  cout << "Total permutations: " << perm_count(choices) << endl;
  cout << "  (3 colors × 3 sizes = 9)" << endl;
  
  // Three-way product
  print_subsection("Three-way Cartesian product");
  DynList<DynList<int>> digits;
  digits.append(DynList<int>({0, 1}));
  digits.append(DynList<int>({0, 1}));
  digits.append(DynList<int>({0, 1}));
  
  cout << "Binary digits: [0,1] × [0,1] × [0,1]\n";
  cout << "All 3-bit binary numbers:\n";
  for_each_perm(digits, [](const DynList<int>& perm) {
    cout << "  ";
    for (auto it = perm.get_it(); it.has_curr(); it.next())
      cout << it.get_curr();
    cout << endl;
  });
  cout << "Total: " << perm_count(digits) << " (2³ = 8)" << endl;
}

// =============================================================================
// 3. Permutation Predicates
// =============================================================================

void demo_perm_predicates()
{
  print_section("PERMUTATION PREDICATES");
  
  // Dice combinations
  DynList<DynList<int>> dice;
  dice.append(DynList<int>({1, 2, 3, 4, 5, 6}));
  dice.append(DynList<int>({1, 2, 3, 4, 5, 6}));
  
  cout << "Two dice: [1-6] × [1-6] = 36 outcomes\n";
  
  // exists_perm - at least one satisfies
  print_subsection("exists_perm()");
  
  bool has_double_six = exists_perm(dice, [](const DynList<int>& roll) {
    int sum = 0;
    for (auto it = roll.get_it(); it.has_curr(); it.next())
      sum += it.get_curr();
    return sum == 12;  // Double six
  });
  cout << "Exists roll with sum = 12? " << (has_double_six ? "yes" : "no") << endl;
  
  bool has_sum_15 = exists_perm(dice, [](const DynList<int>& roll) {
    int sum = 0;
    for (auto it = roll.get_it(); it.has_curr(); it.next())
      sum += it.get_curr();
    return sum == 15;  // Impossible
  });
  cout << "Exists roll with sum = 15? " << (has_sum_15 ? "yes" : "no") << endl;
  
  // all_perm - all satisfy
  print_subsection("all_perm()");
  
  bool all_positive = all_perm(dice, [](const DynList<int>& roll) {
    for (auto it = roll.get_it(); it.has_curr(); it.next())
      if (it.get_curr() <= 0) return false;
    return true;
  });
  cout << "All rolls have positive values? " << (all_positive ? "yes" : "no") << endl;
  
  bool all_sum_gt_10 = all_perm(dice, [](const DynList<int>& roll) {
    int sum = 0;
    for (auto it = roll.get_it(); it.has_curr(); it.next())
      sum += it.get_curr();
    return sum > 10;
  });
  cout << "All rolls have sum > 10? " << (all_sum_gt_10 ? "yes" : "no") << endl;
  
  // none_perm
  print_subsection("none_perm()");
  
  bool none_zero = none_perm(dice, [](const DynList<int>& roll) {
    for (auto it = roll.get_it(); it.has_curr(); it.next())
      if (it.get_curr() == 0) return true;
    return false;
  });
  cout << "No roll contains a zero? " << (none_zero ? "yes" : "no") << endl;
}

// =============================================================================
// 4. Traverse with Early Exit
// =============================================================================

void demo_traverse()
{
  print_section("TRAVERSE WITH EARLY EXIT");
  
  DynList<DynList<int>> numbers;
  numbers.append(DynList<int>({1, 2, 3}));
  numbers.append(DynList<int>({10, 20, 30}));
  
  cout << "Lists: [1,2,3] × [10,20,30]\n\n";
  
  // traverse_perm stops on false
  print_subsection("traverse_perm() - stop when sum > 25");
  
  bool found = not traverse_perm(numbers, [](const DynList<int>& perm) {
    int sum = 0;
    for (auto it = perm.get_it(); it.has_curr(); it.next())
      sum += it.get_curr();
    
    cout << "  Checking: ";
    bool first = true;
    for (auto it = perm.get_it(); it.has_curr(); it.next())
    {
      if (not first) cout << "+";
      cout << it.get_curr();
      first = false;
    }
    cout << " = " << sum;
    
    if (sum > 25)
    {
      cout << " > 25, STOP!" << endl;
      return false;  // Stop traversal
    }
    cout << endl;
    return true;  // Continue
  });
  
  cout << "\nFound sum > 25? " << (found ? "yes" : "no") << endl;
}

// =============================================================================
// 5. Fold over Permutations
// =============================================================================

void demo_fold()
{
  print_section("FOLD OVER PERMUTATIONS");
  
  DynList<DynList<int>> values;
  values.append(DynList<int>({1, 2}));
  values.append(DynList<int>({3, 4}));
  
  cout << "Values: [1,2] × [3,4]\n";
  cout << "Permutations: (1,3), (1,4), (2,3), (2,4)\n\n";
  
  // fold_perm - accumulate over all permutations
  print_subsection("fold_perm() - sum of products");
  
  int total = fold_perm(0, values, [](int acc, const DynList<int>& perm) {
    int product = 1;
    for (auto it = perm.get_it(); it.has_curr(); it.next())
      product *= it.get_curr();
    cout << "  Product: " << product << ", Running total: " << (acc + product) << endl;
    return acc + product;
  });
  
  cout << "\nTotal (1×3 + 1×4 + 2×3 + 2×4) = " << total << endl;
  cout << "Expected: 3 + 4 + 6 + 8 = 21" << endl;
}

// =============================================================================
// 6. Build Permutations List
// =============================================================================

void demo_build()
{
  print_section("BUILD PERMUTATIONS LIST");
  
  DynList<DynList<string>> menu;
  menu.append(DynList<string>({"cafe", "te"}));
  menu.append(DynList<string>({"arepa", "empanada"}));
  menu.append(DynList<string>({"postre"}));
  
  cout << "Menu choices:" << endl;
  cout << "  Bebida: [cafe, te]" << endl;
  cout << "  Comida: [arepa, empanada]" << endl;
  cout << "  Extra:  [postre]" << endl;
  
  // build_perms
  print_subsection("build_perms()");
  auto all_combos = build_perms(menu);
  
  cout << "All possible orders (" << all_combos.size() << " total):\n";
  size_t n = 0;
  for (auto it = all_combos.get_it(); it.has_curr(); it.next())
  {
    cout << "  " << ++n << ": ";
    auto& combo = it.get_curr();
    bool first = true;
    for (auto jt = combo.get_it(); jt.has_curr(); jt.next())
    {
      if (not first) cout << " + ";
      cout << jt.get_curr();
      first = false;
    }
    cout << endl;
  }
}

// =============================================================================
// 7. Practical Example: Configuration Generator
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL: Configuration Generator");
  
  cout << "Generate all test configurations for a system.\n\n";
  
  DynList<DynList<string>> config_options;
  config_options.append(DynList<string>({"debug", "release"}));
  config_options.append(DynList<string>({"x86", "x64", "arm"}));
  config_options.append(DynList<string>({"linux", "windows"}));
  
  cout << "Options:" << endl;
  cout << "  Build:    [debug, release]" << endl;
  cout << "  Arch:     [x86, x64, arm]" << endl;
  cout << "  Platform: [linux, windows]" << endl;
  
  print_subsection("All configurations");
  cout << "Total: " << perm_count(config_options) << " configurations\n\n";
  
  size_t n = 0;
  for_each_perm(config_options, [&n](const DynList<string>& config) {
    cout << "  " << setw(2) << ++n << ". ";
    bool first = true;
    for (auto it = config.get_it(); it.has_curr(); it.next())
    {
      if (not first) cout << "-";
      cout << it.get_curr();
      first = false;
    }
    cout << endl;
  });
  
  // Count specific configurations
  print_subsection("Count Linux configurations");
  size_t linux_count = 0;
  for_each_perm(config_options, [&linux_count](const DynList<string>& config) {
    for (auto it = config.get_it(); it.has_curr(); it.next())
      if (it.get_curr() == "linux")
      {
        linux_count++;
        break;
      }
  });
  cout << "Linux configurations: " << linux_count << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Combinatorics example for Aleph-w.\n"
      "Demonstrates transpose, permutations, and combinations.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: transpose, perm, predicates, "
      "traverse, fold, build, practical, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "        ALEPH-W COMBINATORICS EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "transpose")
      demo_transpose();
    
    if (section == "all" or section == "perm")
      demo_permutations();
    
    if (section == "all" or section == "predicates")
      demo_perm_predicates();
    
    if (section == "all" or section == "traverse")
      demo_traverse();
    
    if (section == "all" or section == "fold")
      demo_fold();
    
    if (section == "all" or section == "build")
      demo_build();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Combinatorics demo completed!\n";
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

