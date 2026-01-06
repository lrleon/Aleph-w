/**
 * @file bloom_filter_example.C
 * @brief Example demonstrating Bloom filters in Aleph-w.
 *
 * A Bloom filter is a space-efficient probabilistic data structure for
 * testing set membership. It may return false positives but NEVER false
 * negatives.
 *
 * ## Key Properties
 *
 * - **No false negatives**: If "not found", the element is definitely absent
 * - **Possible false positives**: "Found" may be wrong (with low probability)
 * - **No deletion**: Standard Bloom filters don't support removal
 * - **Space efficient**: Uses bits instead of storing actual elements
 *
 * ## Parameters
 *
 * - **m**: Size of bit array
 * - **k**: Number of hash functions
 * - **n**: Expected number of elements
 *
 * False positive rate ≈ (1 - e^(-kn/m))^k
 *
 * ## Applications
 *
 * - Cache filtering (avoid disk lookups for non-existent keys)
 * - Spell checkers
 * - Network packet filtering
 * - Database query optimization (skip unnecessary joins)
 *
 * ## Usage
 *
 * ```bash
 * ./bloom_filter_example           # Run all demos
 * ./bloom_filter_example -s basic  # Only basic demo
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see bloom-filter.H Bloom filter implementation
 * @see bitArray.H Underlying bit storage
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

