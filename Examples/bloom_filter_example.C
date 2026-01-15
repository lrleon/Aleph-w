/**
 * @file bloom_filter_example.C
 * @brief Example demonstrating Bloom filters in Aleph-w
 *
 * This example demonstrates Bloom filters, one of the most elegant and
 * practical probabilistic data structures. Invented by Burton Howard Bloom
 * in 1970, Bloom filters provide space-efficient set membership testing
 * with a trade-off: they may have false positives but never false negatives.
 *
 * ## What is a Bloom Filter?
 *
 * A Bloom filter is a space-efficient probabilistic data structure that
 * answers "Is element x in the set?" with:
 * - **Definitely NO**: If answer is "no", element is definitely not in set
 * - **Probably YES**: If answer is "yes", element is probably in set (may be false positive)
 *
 * **Key insight**: Use multiple hash functions to set bits in a bit array.
 * To check membership, verify all corresponding bits are set.
 *
 * ## How It Works
 *
### Insertion
 * ```
 * For element x:
 *   1. Compute k hash functions: h₁(x), h₂(x), ..., hₖ(x)
 *   2. Set bits at positions h₁(x), h₂(x), ..., hₖ(x) to 1
 * ```
 *
### Query
 * ```
 * For element x:
 *   1. Compute k hash functions: h₁(x), h₂(x), ..., hₖ(x)
 *   2. Check if ALL bits at h₁(x), h₂(x), ..., hₖ(x) are 1
 *   3. If all 1: Probably in set (may be false positive)
 *      If any 0: Definitely not in set
 * ```
 *
 * ## Key Properties
 *
### No False Negatives
 * - If element was inserted, all its bits are set
 * - Query will always return "found"
 * - **Guarantee**: 100% accurate for "not found" answers
 *
### Possible False Positives
 * - Bits may be set by other elements (collisions)
 * - Query may return "found" even if element wasn't inserted
 * - **Probability**: Can be controlled by parameters
 *
### No Deletion
 * - Standard Bloom filters don't support removal
 * - Clearing bits might affect other elements
 * - **Solution**: Use Counting Bloom Filter variant
 *
### Space Efficient
 * - Stores only bits, not actual elements
 * - Much smaller than storing full set
 * - **Trade-off**: Space vs false positive rate
 *
 * ## Parameters and Tuning
 *
### Parameters
 * - **m**: Size of bit array (larger = lower false positive rate)
 * - **k**: Number of hash functions (optimal ≈ (m/n) × ln(2))
 * - **n**: Expected number of elements to insert
 *
### False Positive Rate
 *
 * Formula: `P(false positive) ≈ (1 - e^(-kn/m))^k`
 *
 * **Optimal k**: `k = (m/n) × ln(2)` ≈ `0.693 × (m/n)`
 *
 * **Optimal false positive rate**: `(1/2)^k` when k is optimal
 *
### Example Calculations
 *
 * For n = 1,000,000 elements, m = 10,000,000 bits (1.25 MB):
 * - Optimal k ≈ 7 hash functions
 * - False positive rate ≈ 0.8% (less than 1%)
 *
 * Compare to storing 1M strings: ~100 MB+ vs 1.25 MB!
 *
 * ## Applications
 *
### Cache Filtering
 * - **Problem**: Check if data exists in slow storage (disk, network)
 * - **Solution**: Use Bloom filter to avoid expensive lookups
 * - **Benefit**: Skip 99%+ of unnecessary disk/network accesses
 *
### Spell Checkers
 * - **Problem**: Check if word is in dictionary
 * - **Solution**: Bloom filter for common words
 * - **Benefit**: Fast rejection of misspellings
 *
### Network Packet Filtering
 * - **Problem**: Filter packets by source/destination
 * - **Solution**: Bloom filter for allowed/blocked addresses
 * - **Benefit**: Fast packet classification
 *
### Database Query Optimization
 * - **Problem**: Avoid expensive joins for non-existent keys
 * - **Solution**: Bloom filter on join keys
 * - **Benefit**: Skip unnecessary join operations
 *
### Distributed Systems
 * - **Cassandra**: Uses Bloom filters to avoid disk reads
 * - **Chrome**: Uses Bloom filters for malicious URL checking
 * - **Bitcoin**: Uses Bloom filters for wallet synchronization
 *
## When to Use Bloom Filters
 *
 * ✅ **Good for**:
 * - Large datasets where space matters
 * - Fast rejection of non-members
 * - False positives acceptable
 * - One-way operations (insert-only)
 *
 * ❌ **Not good for**:
 * - When false positives are unacceptable
 * - When deletion is needed (use Counting Bloom Filter)
 * - When you need to retrieve the actual elements
 * - Small datasets (overhead not worth it)
 *
 * ## Complexity
 *
 * | Operation | Complexity | Notes |
 * |-----------|-----------|-------|
 * | Insert | O(k) | k hash computations |
 * | Query | O(k) | k hash computations + bit checks |
 * | Space | O(m) | m bits |
 *
 * Where k is number of hash functions (typically 3-10).
 *
 * ## Usage Examples
 *
 * ```bash
 * # Run all demonstrations
 * ./bloom_filter_example
 *
 * # Run specific demo
 * ./bloom_filter_example -s basic     # Basic operations
 * ./bloom_filter_example -s tuning    # Parameter tuning
 * ```
 *
 * @see bloom-filter.H Bloom filter implementation
 * @see bitArray.H Underlying BitArray storage
 * @see bitarray_example.C BitArray operations
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

#include <tclap/CmdLine.h>

#include <bloom-filter.H>

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
// 1. Basic Usage
// =============================================================================

void demo_basic()
{
  print_section("BASIC BLOOM FILTER USAGE");
  
  cout << "A Bloom filter tests set membership probabilistically.\n";
  cout << "May have false positives, but NEVER false negatives.\n\n";
  
  // Create a Bloom filter for strings
  // Parameters: bit array size (m), number of hash functions (k)
  size_t m = 1000;  // 1000 bits
  size_t k = 7;     // 7 hash functions
  
  Bloom_Filter<string> filter(m, k);
  
  cout << "Created Bloom filter:" << endl;
  cout << "  Bit array size (m): " << m << endl;
  cout << "  Hash functions (k): " << k << endl;
  
  // Insert some elements
  print_subsection("Inserting elements");
  vector<string> words = {"cafe", "arepa", "empanada", "bandeja", "sancocho"};
  
  for (const auto& w : words)
  {
    filter.insert(w);
    cout << "  Inserted: \"" << w << "\"" << endl;
  }
  
  // Test membership
  print_subsection("Testing membership");
  
  vector<string> test_words = {"cafe", "pizza", "arepa", "hamburguesa", "sancocho"};
  
  for (const auto& w : test_words)
  {
    bool found = filter.contains(w);
    cout << "  \"" << w << "\": " 
         << (found ? "POSSIBLY present" : "DEFINITELY absent") << endl;
  }
  
  cout << "\nNote: \"POSSIBLY present\" may be a false positive." << endl;
  cout << "      \"DEFINITELY absent\" is always correct!" << endl;
}

// =============================================================================
// 2. False Positive Rate
// =============================================================================

void demo_false_positives()
{
  print_section("FALSE POSITIVE RATE ANALYSIS");
  
  cout << "The false positive rate depends on m (bits), k (hashes), n (elements).\n";
  cout << "Formula: P(FP) ≈ (1 - e^(-kn/m))^k\n\n";
  
  // Insert known elements
  size_t n = 100;  // Number of elements to insert
  size_t m = 1000; // Bit array size
  size_t k = 7;    // Number of hash functions
  
  Bloom_Filter<int> filter(m, k);
  
  // Insert elements 0 to n-1
  for (size_t i = 0; i < n; i++)
    filter.insert(static_cast<int>(i));
  
  cout << "Configuration:" << endl;
  cout << "  Elements inserted (n): " << n << endl;
  cout << "  Bit array size (m):    " << m << endl;
  cout << "  Hash functions (k):    " << k << endl;
  
  // Theoretical false positive rate
  double theoretical_fp = pow(1.0 - exp(-static_cast<double>(k * n) / m), k);
  cout << "\nTheoretical FP rate: " << fixed << setprecision(4) 
       << (theoretical_fp * 100) << "%" << endl;
  
  // Empirical test: check elements NOT in the filter
  print_subsection("Empirical test");
  
  size_t test_count = 10000;
  size_t false_positives = 0;
  
  for (size_t i = n; i < n + test_count; i++)
  {
    if (filter.contains(static_cast<int>(i)))
      false_positives++;
  }
  
  double empirical_fp = static_cast<double>(false_positives) / test_count;
  
  cout << "Tested " << test_count << " elements NOT in the filter:" << endl;
  cout << "  False positives: " << false_positives << endl;
  cout << "  Empirical FP rate: " << fixed << setprecision(4) 
       << (empirical_fp * 100) << "%" << endl;
}

// =============================================================================
// 3. Optimal Parameters
// =============================================================================

void demo_optimal_params()
{
  print_section("OPTIMAL PARAMETERS");
  
  cout << "For a target false positive rate p and n elements:\n";
  cout << "  Optimal m = -n * ln(p) / (ln(2))²\n";
  cout << "  Optimal k = (m/n) * ln(2) ≈ 0.693 * m/n\n\n";
  
  // Example: design for 1% FP rate with 1000 elements
  size_t n = 1000;
  double target_fp = 0.01;  // 1% false positive rate
  
  // Calculate optimal parameters
  double ln2 = log(2);
  double m_optimal = -static_cast<double>(n) * log(target_fp) / (ln2 * ln2);
  double k_optimal = (m_optimal / n) * ln2;
  
  cout << "Target: " << (target_fp * 100) << "% FP rate for " << n << " elements" << endl;
  cout << "  Optimal m: " << static_cast<size_t>(ceil(m_optimal)) << " bits" << endl;
  cout << "  Optimal k: " << static_cast<size_t>(round(k_optimal)) << " hash functions" << endl;
  cout << "  Bits per element: " << fixed << setprecision(2) << (m_optimal / n) << endl;
  
  // Compare different configurations
  print_subsection("Comparison of configurations");
  
  cout << setw(10) << "m" << setw(10) << "k" 
       << setw(15) << "FP Rate (%)" << setw(15) << "Bits/elem" << endl;
  cout << string(50, '-') << endl;
  
  vector<pair<size_t, size_t>> configs = {
    {5000, 3}, {5000, 7}, {10000, 7}, {10000, 10}, {15000, 10}
  };
  
  for (auto [m, k] : configs)
  {
    double fp = pow(1.0 - exp(-static_cast<double>(k * n) / m), k);
    cout << setw(10) << m << setw(10) << k 
         << setw(15) << fixed << setprecision(4) << (fp * 100)
         << setw(15) << setprecision(2) << (static_cast<double>(m) / n) << endl;
  }
}

// =============================================================================
// 4. Practical Application: Spell Checker
// =============================================================================

void demo_spell_checker()
{
  print_section("PRACTICAL: Simple Spell Checker");
  
  cout << "Use a Bloom filter as a fast first-pass spell checker.\n";
  cout << "If word is 'definitely absent', it's misspelled.\n\n";
  
  // Create dictionary filter
  size_t dict_size = 50;  // Expected dictionary size
  size_t m = dict_size * 10;  // ~1% FP rate
  size_t k = 7;
  
  Bloom_Filter<string> dictionary(m, k);
  
  // Add Spanish words to dictionary
  vector<string> spanish_words = {
    "hola", "mundo", "casa", "perro", "gato", "agua", "fuego", "tierra",
    "aire", "sol", "luna", "estrella", "mar", "cielo", "arbol", "flor",
    "libro", "mesa", "silla", "puerta", "ventana", "calle", "ciudad",
    "pueblo", "pais", "rio", "montana", "valle", "bosque", "campo",
    "tiempo", "dia", "noche", "semana", "mes", "ano", "hora", "minuto",
    "segundo", "mano", "pie", "cabeza", "ojo", "nariz", "boca", "oreja",
    "corazon", "alma", "vida", "muerte"
  };
  
  cout << "Loading dictionary (" << spanish_words.size() << " words)..." << endl;
  for (const auto& w : spanish_words)
    dictionary.insert(w);
  
  // Check some text
  print_subsection("Spell checking");
  
  vector<string> text = {
    "hola", "munod", "casa", "perro", "gatoh", "agua", "xyz", "libro"
  };
  
  cout << "Checking words:\n";
  for (const auto& w : text)
  {
    bool found = dictionary.contains(w);
    cout << "  \"" << w << "\": ";
    if (found)
      cout << "OK (in dictionary)" << endl;
    else
      cout << "MISSPELLED (not in dictionary)" << endl;
  }
  
  cout << "\nNote: 'OK' might be a false positive for unknown words." << endl;
  cout << "      'MISSPELLED' is always correct!" << endl;
}

// =============================================================================
// 5. Practical Application: Cache Filter
// =============================================================================

void demo_cache_filter()
{
  print_section("PRACTICAL: Database Cache Filter");
  
  cout << "Use Bloom filter to avoid expensive database lookups.\n";
  cout << "If key is 'definitely absent', skip the database query.\n\n";
  
  // Simulate a cache with some IDs
  size_t cache_size = 1000;
  size_t m = cache_size * 10;
  size_t k = 7;
  
  Bloom_Filter<int> cache_filter(m, k);
  
  // Populate cache with even IDs
  cout << "Cache contains IDs: 0, 2, 4, 6, ..., 1998 (even numbers)" << endl;
  for (int i = 0; i < 2000; i += 2)
    cache_filter.insert(i);
  
  // Simulate queries
  print_subsection("Query simulation");
  
  vector<int> queries = {100, 101, 500, 999, 1000, 1001, 1500, 9999};
  
  int cache_hits = 0;
  int db_lookups_saved = 0;
  
  cout << setw(10) << "Query ID" << setw(20) << "Bloom Result" 
       << setw(20) << "Action" << endl;
  cout << string(50, '-') << endl;
  
  for (int id : queries)
  {
    bool maybe_cached = cache_filter.contains(id);
    bool actually_cached = (id < 2000) and (id % 2 == 0);  // Ground truth
    
    cout << setw(10) << id;
    
    if (maybe_cached)
    {
      cout << setw(20) << "Maybe present";
      if (actually_cached)
      {
        cout << setw(20) << "Cache HIT" << endl;
        cache_hits++;
      }
      else
        cout << setw(20) << "False positive, DB lookup" << endl;
    }
    else
    {
      cout << setw(20) << "Definitely absent" << setw(20) << "Skip DB (saved!)" << endl;
      db_lookups_saved++;
    }
  }
  
  cout << "\nResults:" << endl;
  cout << "  Cache hits: " << cache_hits << endl;
  cout << "  DB lookups saved: " << db_lookups_saved << endl;
}

// =============================================================================
// 6. Space Comparison
// =============================================================================

void demo_space_comparison()
{
  print_section("SPACE EFFICIENCY");
  
  cout << "Bloom filters trade accuracy for space efficiency.\n\n";
  
  size_t n = 10000;  // Number of elements
  
  cout << "Storing " << n << " 64-bit integers:\n\n";
  
  // Hash set: stores actual elements
  size_t hashset_bytes = n * (sizeof(int64_t) + sizeof(void*) * 2);  // Approx
  
  // Bloom filter for various FP rates
  cout << setw(15) << "FP Rate" << setw(15) << "Bits/elem" 
       << setw(15) << "Total KB" << setw(15) << "Savings" << endl;
  cout << string(60, '-') << endl;
  
  vector<double> fp_rates = {0.10, 0.05, 0.01, 0.001, 0.0001};
  
  for (double fp : fp_rates)
  {
    double ln2 = log(2);
    double m = -static_cast<double>(n) * log(fp) / (ln2 * ln2);
    double bits_per_elem = m / n;
    double bloom_kb = m / 8.0 / 1024.0;
    double savings = 1.0 - (bloom_kb * 1024.0 / hashset_bytes);
    
    cout << setw(14) << fixed << setprecision(4) << (fp * 100) << "%"
         << setw(15) << setprecision(2) << bits_per_elem
         << setw(15) << setprecision(2) << bloom_kb
         << setw(14) << setprecision(1) << (savings * 100) << "%" << endl;
  }
  
  cout << "\nHash set (approximate): " 
       << fixed << setprecision(2) << (hashset_bytes / 1024.0) << " KB" << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Bloom filter example for Aleph-w.\n"
      "Demonstrates probabilistic set membership testing.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: basic, fp, params, spell, cache, space, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "          ALEPH-W BLOOM FILTER EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "basic")
      demo_basic();
    
    if (section == "all" or section == "fp")
      demo_false_positives();
    
    if (section == "all" or section == "params")
      demo_optimal_params();
    
    if (section == "all" or section == "spell")
      demo_spell_checker();
    
    if (section == "all" or section == "cache")
      demo_cache_filter();
    
    if (section == "all" or section == "space")
      demo_space_comparison();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Bloom filter demo completed!\n";
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

