/**
 * @file quotient_filter_example.C
 * @brief Quotient filter in Aleph-w: probabilistic set with deletion support.
 *
 * ## Overview
 *
 * Demonstrates `Quotient_Filter<T>`: a cache-friendly probabilistic set
 * that supports insertion, membership testing, **and deletion** — unlike
 * Bloom filters.
 *
 * Sections:
 * - basic:  insert / query / delete
 * - fp:     false positive rate analysis
 * - vs:     comparison with Bloom filter
 * - merge:  merging two filters
 * - cache:  practical cache-filter application
 *
 * ## Usage
 *
 * ```bash
 * ./quotient_filter_example                 # run all
 * ./quotient_filter_example -s basic        # run one section
 * ./quotient_filter_example --help
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

#include <tclap/CmdLine.h>

#include <quotient-filter.H>

using namespace std;
using namespace Aleph;

void print_section(const string & title)
{
  cout << "\n" << string(60, '=') << "\n";
  cout << "  " << title << "\n";
  cout << string(60, '=') << "\n\n";
}

void print_sub(const string & title)
{
  cout << "\n--- " << title << " ---\n";
}

// =====================================================================
// 1. Basic usage
// =====================================================================

void demo_basic()
{
  print_section("BASIC QUOTIENT FILTER USAGE");

  cout << "A quotient filter tests set membership probabilistically.\n";
  cout << "Like Bloom filters, but also supports DELETION.\n\n";

  // q=10 → 1024 slots, r=8 → FP ≈ 1/256 ≈ 0.4%
  Quotient_Filter<string> qf(10, 8);

  cout << "Created quotient filter:\n";
  cout << "  Slots (2^q):         " << qf.capacity() << "\n";
  cout << "  Remainder bits (r):  " << static_cast<int>(qf.r()) << "\n";
  cout << "  FP rate ≈ 2^{-r}:   " << fixed << setprecision(4)
       << (qf.false_positive_rate() * 100) << "%\n";
  cout << "  Memory:              " << qf.memory_usage() << " bytes\n\n";

  print_sub("Inserting elements");
  vector<string> words = {"cafe", "arepa", "empanada", "bandeja", "sancocho"};
  for (const auto & w : words)
    {
      qf.insert(w);
      cout << "  Inserted: \"" << w << "\"\n";
    }
  cout << "  Size: " << qf.size() << "\n";

  print_sub("Membership queries");
  vector<string> tests = {"cafe", "pizza", "arepa", "hamburguesa", "sancocho"};
  for (const auto & w : tests)
    {
      bool found = qf.contains(w);
      cout << "  \"" << w << "\": "
           << (found ? "PROBABLY present" : "DEFINITELY absent") << "\n";
    }

  print_sub("Deletion (Bloom filters can't do this!)");
  cout << "  Removing \"arepa\"...\n";
  qf.remove("arepa");
  cout << "  contains(\"arepa\"): "
       << (qf.contains("arepa") ? "true" : "false") << "\n";
  cout << "  Size after removal: " << qf.size() << "\n";

  cout << "\n  Removing \"sancocho\"...\n";
  qf.remove("sancocho");
  cout << "  contains(\"sancocho\"): "
       << (qf.contains("sancocho") ? "true" : "false") << "\n";

  cout << "\n  Remaining elements still present:\n";
  for (const auto & w : words)
    if (qf.contains(w))
      cout << "    \"" << w << "\"\n";
}

// =====================================================================
// 2. False positive rate
// =====================================================================

void demo_fp()
{
  print_section("FALSE POSITIVE RATE ANALYSIS");

  cout << "FP rate ≈ 2^{-r} where r is the remainder bits.\n\n";

  size_t n = 500;

  cout << setw(5) << "r" << setw(12) << "Theor FP%"
       << setw(15) << "Observed FP%" << setw(12) << "Tested\n";
  cout << string(44, '-') << "\n";

  for (int r_val : {4, 6, 8, 10, 12})
    {
      Quotient_Filter<int> qf(12, static_cast<uint8_t>(r_val));

      for (size_t i = 0; i < n; ++i)
        qf.insert(static_cast<int>(i));

      size_t test_count = 10000;
      size_t fps = 0;
      for (size_t i = n + 50000; i < n + 50000 + test_count; ++i)
        if (qf.contains(static_cast<int>(i)))
          ++fps;

      double theor = 100.0 * qf.false_positive_rate();
      double empir = 100.0 * static_cast<double>(fps) / test_count;

      cout << setw(5) << r_val
           << setw(12) << fixed << setprecision(4) << theor
           << setw(15) << empir
           << setw(12) << test_count << "\n";
    }
}

// =====================================================================
// 3. Comparison with Bloom filter
// =====================================================================

void demo_comparison()
{
  print_section("QUOTIENT FILTER vs BLOOM FILTER");

  cout << "Feature comparison:\n\n";

  cout << setw(25) << "Feature"
       << setw(20) << "Bloom Filter"
       << setw(20) << "Quotient Filter" << "\n";
  cout << string(65, '-') << "\n";

  auto row = [](const string & feat, const string & bloom, const string & qf)
    {
      cout << setw(25) << feat
           << setw(20) << bloom
           << setw(20) << qf << "\n";
    };

  row("Deletion",          "No",           "Yes");
  row("False negatives",   "Never",        "Never");
  row("False positives",   "Yes",          "Yes");
  row("Cache-friendly",    "Poor",         "Excellent");
  row("Mergeable",         "Yes (OR)",     "Yes (union)");
  row("Space per element", "~10 bits/1%",  "(r+3) bits");
  row("Resize",            "Rebuild",      "Rebuild");

  cout << "\nQuotient filter with r=8: FP ≈ 0.4%, 11 bits/element.\n";
  cout << "Bloom filter at 0.4%: ~11 bits/element, ~8 hash functions.\n";
  cout << "→ Similar space, but QF supports deletion and is cache-friendly.\n";
}

// =====================================================================
// 4. Merge
// =====================================================================

void demo_merge()
{
  print_section("MERGING QUOTIENT FILTERS");

  cout << "Two filters with the same (q, r, seed) can be merged.\n\n";

  uint32_t seed = 12345;
  Quotient_Filter<int> a(10, 8, seed);
  Quotient_Filter<int> b(10, 8, seed);

  for (int i = 0; i < 100; ++i)
    a.insert(i);
  for (int i = 100; i < 200; ++i)
    b.insert(i);

  cout << "Filter A: " << a.size() << " elements (0..99)\n";
  cout << "Filter B: " << b.size() << " elements (100..199)\n";

  a.merge(b);
  cout << "\nAfter merge: " << a.size() << " elements\n";

  size_t found = 0;
  for (int i = 0; i < 200; ++i)
    if (a.contains(i))
      ++found;

  cout << "Elements 0..199 found: " << found << "/200\n";
}

// =====================================================================
// 5. Practical: network dedup cache
// =====================================================================

void demo_cache()
{
  print_section("PRACTICAL: Network Packet Deduplication");

  cout << "Use a quotient filter as a dedup cache for packet IDs.\n";
  cout << "Old entries can be deleted (unlike Bloom filters).\n\n";

  auto qf = Quotient_Filter<int>::from_capacity(10000, 0.01);

  cout << "Filter for ~10000 packets at 1% FP:\n";
  cout << "  q=" << static_cast<int>(qf.q())
       << "  r=" << static_cast<int>(qf.r())
       << "  capacity=" << qf.capacity()
       << "  memory=" << qf.memory_usage() / 1024 << " KB\n\n";

  // Simulate a stream of packet IDs
  int window_start = 0;
  int window_size = 5000;

  // Insert first window
  for (int id = window_start; id < window_start + window_size; ++id)
    qf.insert(id);
  cout << "Inserted window [0, 5000): size=" << qf.size()
       << "  load=" << fixed << setprecision(2)
       << (qf.load_factor() * 100) << "%\n";

  // Slide window: remove old, add new
  int new_start = 2000;
  for (int id = window_start; id < new_start; ++id)
    qf.remove(id);
  for (int id = window_size; id < window_size + new_start; ++id)
    qf.insert(id);

  cout << "Slid window to [2000, 7000): size=" << qf.size()
       << "  load=" << fixed << setprecision(2)
       << (qf.load_factor() * 100) << "%\n";

  // Check dedup
  size_t dupes_caught = 0;
  size_t dupes_missed = 0;
  for (int id = new_start; id < new_start + 500; ++id)
    {
      if (qf.contains(id))
        ++dupes_caught;
      else
        ++dupes_missed;
    }
  cout << "\nDedup check for 500 IDs in window:\n";
  cout << "  Caught as duplicates: " << dupes_caught << "/500\n";
  cout << "  Missed (false neg):   " << dupes_missed << "/500\n";
}

// =====================================================================
// Main
// =====================================================================

int main(int argc, char * argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Quotient filter example for Aleph-w.\n"
        "Demonstrates probabilistic set with deletion support.",
        ' ', "1.0");

      TCLAP::ValueArg<string> sectionArg(
        "s", "section",
        "Run section: basic, fp, vs, merge, cache, or 'all'",
        false, "all", "section", cmd);

      cmd.parse(argc, argv);
      string section = sectionArg.getValue();

      cout << "\n";
      cout << "============================================================\n";
      cout << "        ALEPH-W QUOTIENT FILTER EXAMPLE\n";
      cout << "============================================================\n";

      if (section == "all" or section == "basic")  demo_basic();
      if (section == "all" or section == "fp")     demo_fp();
      if (section == "all" or section == "vs")     demo_comparison();
      if (section == "all" or section == "merge")  demo_merge();
      if (section == "all" or section == "cache")  demo_cache();

      cout << "\n" << string(60, '=') << "\n";
      cout << "Quotient filter demo completed!\n";
      cout << string(60, '=') << "\n\n";

      return 0;
    }
  catch (TCLAP::ArgException & e)
    {
      cerr << "Error: " << e.error() << " for " << e.argId() << endl;
      return 1;
    }
  catch (exception & e)
    {
      cerr << "Error: " << e.what() << endl;
      return 1;
    }
}