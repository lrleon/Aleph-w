/**
 * @file bitarray_example.C
 * @brief Example demonstrating BitArray operations in Aleph-w.
 *
 * This program demonstrates the BitArray class which provides
 * compact storage and efficient operations for bit sets.
 *
 * ## Features Demonstrated
 *
 * ### Basic Operations
 * - Setting, clearing, and reading individual bits
 * - Flip (toggle) operations
 * - Fill operations
 *
 * ### Bulk Operations
 * - Bitwise AND, OR, XOR, NOT
 * - Shift operations
 * - Population count (popcount)
 *
 * ### Set Operations
 * - Union (OR)
 * - Intersection (AND)
 * - Difference (AND NOT)
 * - Symmetric difference (XOR)
 *
 * ### Practical Applications
 * - Sieve of Eratosthenes for prime numbers
 * - Bloom filter approximation
 * - Subset representation
 *
 * ## Usage
 *
 * ```bash
 * ./bitarray_example          # Run all demos
 * ./bitarray_example -n 100   # Use 100 bits for demos
 * ```
 *
 * @author Leandro Rabindranath León
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see BitArray Main bit array class
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>

#include <tclap/CmdLine.h>

#include <bitArray.H>
#include <htlist.H>
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

void print_bits(const string& label, const BitArray& ba, size_t max_show = 64)
{
  cout << label << " (size=" << ba.size() << "): ";
  size_t to_show = min(ba.size(), max_show);
  for (size_t i = 0; i < to_show; ++i)
    cout << (ba.read_bit(i) ? '1' : '0');
  if (to_show < ba.size())
    cout << "... (truncated)";
  cout << endl;
}

// =============================================================================
// 1. Basic Operations
// =============================================================================

void demo_basic_operations()
{
  print_section("BASIC OPERATIONS");
  
  // Creating BitArrays
  print_subsection("Creating BitArrays");
  
  BitArray ba1(16);  // 16 bits, all zeros
  cout << "Created BitArray with 16 bits (all zeros)" << endl;
  print_bits("ba1", ba1);
  
  // Setting bits
  print_subsection("Setting individual bits");
  ba1.write_bit(0, 1);
  ba1.write_bit(3, 1);
  ba1.write_bit(7, 1);
  ba1.write_bit(15, 1);
  
  cout << "After setting bits 0, 3, 7, 15:" << endl;
  print_bits("ba1", ba1);
  
  // Reading bits
  print_subsection("Reading bits");
  cout << "bit[0] = " << ba1.read_bit(0) << endl;
  cout << "bit[1] = " << ba1.read_bit(1) << endl;
  cout << "bit[3] = " << ba1.read_bit(3) << endl;
  cout << "bit[7] = " << ba1.read_bit(7) << endl;
  
  // Alternative access with [] operator
  print_subsection("Using [] operator");
  cout << "ba1[0] = " << (int)ba1[0] << endl;
  cout << "ba1[3] = " << (int)ba1[3] << endl;
  
  // Toggle (flip) bits manually
  print_subsection("Toggling bits");
  cout << "Before toggle: bit[3] = " << ba1.read_bit(3) << endl;
  ba1.write_bit(3, ba1.read_bit(3) ? 0 : 1);  // Manual toggle
  cout << "After toggle:  bit[3] = " << ba1.read_bit(3) << endl;
  ba1.write_bit(3, ba1.read_bit(3) ? 0 : 1);  // Toggle back
  
  // Fill with ones and zeros
  print_subsection("Fill operations");
  BitArray ba2(8);
  // Set all bits to 1
  for (size_t i = 0; i < ba2.size(); ++i)
    ba2.write_bit(i, 1);
  print_bits("All ones", ba2);
  // Clear all bits to 0
  for (size_t i = 0; i < ba2.size(); ++i)
    ba2.write_bit(i, 0);
  print_bits("All zeros", ba2);
}

// =============================================================================
// 2. Bitwise Operations
// =============================================================================

void demo_bitwise_operations()
{
  print_section("BITWISE OPERATIONS");
  
  BitArray a(8);
  BitArray b(8);
  
  // Setup: a = 11110000, b = 11001100
  a.write_bit(0, 1); a.write_bit(1, 1); a.write_bit(2, 1); a.write_bit(3, 1);
  b.write_bit(0, 1); b.write_bit(1, 1); b.write_bit(4, 1); b.write_bit(5, 1);
  
  print_bits("a", a);
  print_bits("b", b);
  
  // AND
  print_subsection("AND operation");
  BitArray and_result = a;
  for (size_t i = 0; i < 8; ++i)
    if (not b.read_bit(i))
      and_result.write_bit(i, 0);
  print_bits("a AND b", and_result);
  
  // OR
  print_subsection("OR operation");
  BitArray or_result = a;
  for (size_t i = 0; i < 8; ++i)
    if (b.read_bit(i))
      or_result.write_bit(i, 1);
  print_bits("a OR b", or_result);
  
  // XOR
  print_subsection("XOR operation");
  BitArray xor_result(8);
  for (size_t i = 0; i < 8; ++i)
    xor_result.write_bit(i, a.read_bit(i) != b.read_bit(i) ? 1 : 0);
  print_bits("a XOR b", xor_result);
  
  // NOT
  print_subsection("NOT operation");
  BitArray not_a(8);
  for (size_t i = 0; i < 8; ++i)
    not_a.write_bit(i, a.read_bit(i) ? 0 : 1);
  print_bits("NOT a", not_a);
  
  // Population count
  print_subsection("Population count (number of 1s)");
  size_t count_a = 0;
  for (size_t i = 0; i < a.size(); ++i)
    if (a.read_bit(i)) count_a++;
  cout << "popcount(a) = " << count_a << endl;
  
  size_t count_b = 0;
  for (size_t i = 0; i < b.size(); ++i)
    if (b.read_bit(i)) count_b++;
  cout << "popcount(b) = " << count_b << endl;
}

// =============================================================================
// 3. Set Operations
// =============================================================================

void demo_set_operations()
{
  print_section("SET OPERATIONS (using bits as sets)");
  
  cout << "Universal set U = {0, 1, 2, 3, 4, 5, 6, 7}" << endl;
  
  // Set A = {0, 1, 2, 3}
  BitArray setA(8);
  setA.write_bit(0, 1); setA.write_bit(1, 1);
  setA.write_bit(2, 1); setA.write_bit(3, 1);
  
  // Set B = {2, 3, 4, 5}
  BitArray setB(8);
  setB.write_bit(2, 1); setB.write_bit(3, 1);
  setB.write_bit(4, 1); setB.write_bit(5, 1);
  
  auto bits_to_set = [](const BitArray& ba) {
    cout << "{";
    bool first = true;
    for (size_t i = 0; i < ba.size(); ++i)
    {
      if (ba.read_bit(i))
      {
        if (not first) cout << ", ";
        cout << i;
        first = false;
      }
    }
    cout << "}";
  };
  
  cout << "A = "; bits_to_set(setA); cout << endl;
  cout << "B = "; bits_to_set(setB); cout << endl;
  
  // Union (A OR B)
  print_subsection("Union A ∪ B");
  BitArray unionAB(8);
  for (size_t i = 0; i < 8; ++i)
    unionAB.write_bit(i, setA.read_bit(i) or setB.read_bit(i) ? 1 : 0);
  cout << "A ∪ B = "; bits_to_set(unionAB); cout << endl;
  
  // Intersection (A AND B)
  print_subsection("Intersection A ∩ B");
  BitArray intersectAB(8);
  for (size_t i = 0; i < 8; ++i)
    intersectAB.write_bit(i, setA.read_bit(i) and setB.read_bit(i) ? 1 : 0);
  cout << "A ∩ B = "; bits_to_set(intersectAB); cout << endl;
  
  // Difference (A - B = A AND NOT B)
  print_subsection("Difference A - B");
  BitArray diffAB(8);
  for (size_t i = 0; i < 8; ++i)
    diffAB.write_bit(i, setA.read_bit(i) and not setB.read_bit(i) ? 1 : 0);
  cout << "A - B = "; bits_to_set(diffAB); cout << endl;
  
  // Symmetric difference (A XOR B)
  print_subsection("Symmetric Difference A △ B");
  BitArray symDiffAB(8);
  for (size_t i = 0; i < 8; ++i)
    symDiffAB.write_bit(i, (setA.read_bit(i) != setB.read_bit(i)) ? 1 : 0);
  cout << "A △ B = "; bits_to_set(symDiffAB); cout << endl;
  
  // Complement
  print_subsection("Complement A'");
  BitArray compA(8);
  for (size_t i = 0; i < 8; ++i)
    compA.write_bit(i, setA.read_bit(i) ? 0 : 1);
  cout << "A' = "; bits_to_set(compA); cout << endl;
}

// =============================================================================
// 4. Practical Applications
// =============================================================================

void demo_sieve_of_eratosthenes(size_t n)
{
  print_section("SIEVE OF ERATOSTHENES");
  
  cout << "Finding all primes up to " << n << endl << endl;
  
  // true = composite, false = prime (initially all false)
  BitArray is_composite(n + 1);
  
  // 0 and 1 are not prime
  is_composite.write_bit(0, 1);
  is_composite.write_bit(1, 1);
  
  // Sieve
  size_t sqrt_n = static_cast<size_t>(sqrt(static_cast<double>(n)));
  for (size_t i = 2; i <= sqrt_n; ++i)
  {
    if (not is_composite.read_bit(i))
    {
      // Mark all multiples of i as composite
      for (size_t j = i * i; j <= n; j += i)
        is_composite.write_bit(j, 1);
    }
  }
  
  // Collect and display primes
  DynList<size_t> primes;
  for (size_t i = 2; i <= n; ++i)
    if (not is_composite.read_bit(i))
      primes.append(i);
  
  cout << "Found " << primes.size() << " primes:\n";
  cout << "[";
  bool first = true;
  size_t shown = 0;
  primes.for_each([&first, &shown](size_t p) {
    if (shown < 50)
    {
      if (not first) cout << ", ";
      cout << p;
      first = false;
      shown++;
    }
  });
  if (primes.size() > 50)
    cout << ", ... (showing first 50)";
  cout << "]\n";
  
  // Memory efficiency
  cout << "\nMemory used: " << (n + 1 + 7) / 8 << " bytes" << endl;
  cout << "vs. bool array: " << (n + 1) << " bytes" << endl;
  cout << "Space savings: " << 100 * (1 - 1.0/8) << "%" << endl;
}

void demo_subset_enumeration()
{
  print_section("SUBSET ENUMERATION");
  
  DynList<string> universe;
  universe.append("apple");
  universe.append("banana");
  universe.append("cherry");
  
  cout << "Universe: {apple, banana, cherry}" << endl;
  cout << "\nAll possible subsets (2^3 = 8):\n" << endl;
  
  size_t n = 3;
  size_t total_subsets = 1 << n;  // 2^n
  
  for (size_t mask = 0; mask < total_subsets; ++mask)
  {
    BitArray subset(n);
    // Convert mask to BitArray
    for (size_t i = 0; i < n; ++i)
      if (mask & (1 << i))
        subset.write_bit(i, 1);
    
    cout << "  {";
    bool first = true;
    size_t idx = 0;
    universe.for_each([&](const string& item) {
      if (subset.read_bit(idx))
      {
        if (not first) cout << ", ";
        cout << item;
        first = false;
      }
      idx++;
    });
    cout << "}" << endl;
  }
}

void demo_simple_bloom_filter()
{
  print_section("SIMPLE BLOOM FILTER CONCEPT");
  
  size_t filter_size = 32;
  BitArray bloom(filter_size);
  
  cout << "Bloom filter size: " << filter_size << " bits\n" << endl;
  
  // Simple hash functions
  auto hash1 = [filter_size](const string& s) {
    size_t h = 0;
    for (char c : s) h = h * 31 + c;
    return h % filter_size;
  };
  
  auto hash2 = [filter_size](const string& s) {
    size_t h = 0;
    for (char c : s) h = h * 37 + c;
    return h % filter_size;
  };
  
  // Add items to filter
  auto add = [&](const string& item) {
    bloom.write_bit(hash1(item), 1);
    bloom.write_bit(hash2(item), 1);
    cout << "Added \"" << item << "\" -> bits " << hash1(item) 
         << ", " << hash2(item) << endl;
  };
  
  // Check if item might be in filter
  auto might_contain = [&](const string& item) {
    return bloom.read_bit(hash1(item)) and bloom.read_bit(hash2(item));
  };
  
  // Add some items
  cout << "Adding items:\n";
  add("hello");
  add("world");
  add("aleph");
  
  print_bits("\nBloom filter", bloom);
  
  // Test membership
  cout << "\nMembership tests:" << endl;
  DynList<string> tests;
  tests.append("hello");
  tests.append("world");
  tests.append("aleph");
  tests.append("test");
  tests.append("foo");
  tests.append("bar");
  
  tests.for_each([&](const string& item) {
    bool result = might_contain(item);
    cout << "  \"" << item << "\": " 
         << (result ? "probably in set" : "definitely NOT in set") << endl;
  });
  
  cout << "\nNote: Bloom filters may have false positives, but never false negatives." << endl;
}

// =============================================================================
// 5. Performance Comparison
// =============================================================================

void demo_performance()
{
  print_section("MEMORY EFFICIENCY");
  
  cout << "Comparison of memory usage:\n\n";
  
  cout << setw(20) << "Size" 
       << setw(20) << "BitArray (bytes)"
       << setw(20) << "bool[] (bytes)"
       << setw(15) << "Savings" << endl;
  cout << string(75, '-') << endl;
  
  DynList<size_t> sizes;
  sizes.append(100);
  sizes.append(1000);
  sizes.append(10000);
  sizes.append(100000);
  sizes.append(1000000);
  
  sizes.for_each([](size_t n) {
    size_t bitarray_bytes = (n + 7) / 8;
    size_t bool_bytes = n;
    double savings = 100.0 * (1.0 - static_cast<double>(bitarray_bytes) / bool_bytes);
    
    cout << setw(20) << n
         << setw(20) << bitarray_bytes
         << setw(20) << bool_bytes
         << setw(14) << fixed << setprecision(1) << savings << "%" << endl;
  });
  
  cout << "\nBitArray uses 8x less memory than bool arrays!" << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "BitArray example for Aleph-w.\n"
      "Demonstrates bit manipulation, set operations, and practical applications.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<size_t> sieveArg(
      "n", "sieve-size",
      "Size for Sieve of Eratosthenes demo (default: 100)",
      false, 100, "size", cmd
    );
    
    cmd.parse(argc, argv);
    
    size_t sieve_size = sieveArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "          ALEPH-W BITARRAY EXAMPLE\n";
    cout << "============================================================\n";
    
    demo_basic_operations();
    demo_bitwise_operations();
    demo_set_operations();
    demo_sieve_of_eratosthenes(sieve_size);
    demo_subset_enumeration();
    demo_simple_bloom_filter();
    demo_performance();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "BitArray demo completed!\n";
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

