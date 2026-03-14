
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/** @file compact_quotient_filter_example.C
 *  @brief Comprehensive usage examples for Compact_Quotient_Filter
 */

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

#include <compact-quotient-filter.H>
#include <quotient-filter.H>

using namespace Aleph;
using namespace std;

// =====================================================================
// Example 1: Basic usage
// =====================================================================

void example_basic_usage()
{
  cout << "\n=== Example 1: Basic Usage ===\n";

  // Create a compact quotient filter with 2^10 slots (1024) and 8-bit
  // remainders
  Compact_Quotient_Filter<int> qf(10, 8);

  cout << "Created filter with:\n";
  cout << "  - Capacity: " << qf.capacity() << " slots\n";
  cout << "  - Remainder bits: " << static_cast<int>(qf.r()) << "\n";
  cout << "  - False positive rate: " << (qf.false_positive_rate() * 100) << "%\n";
  cout << "  - Memory usage: " << qf.memory_usage() << " bytes\n";

  // Insert elements
  vector<int> items = {42, 100, 256, 512, 1024, 2048};
  for (int item : items)
    qf.insert(item);

  cout << "\nInserted " << items.size() << " elements\n";
  cout << "Current size: " << qf.size() << "\n";
  cout << "Load factor: " << (qf.load_factor() * 100) << "%\n";

  // Test membership
  cout << "\nMembership tests:\n";
  for (int item : items)
    cout << "  contains(" << item << "): " << (qf.contains(item) ? "YES" : "NO") << "\n";

  cout << "  contains(999): " << (qf.contains(999) ? "YES (false positive)" : "NO") << "\n";

  // Remove an element
  qf.remove(256);
  cout << "\nAfter removing 256:\n";
  cout << "  contains(256): " << (qf.contains(256) ? "YES" : "NO") << "\n";
  cout << "  Size: " << qf.size() << "\n";
}

// =====================================================================
// Example 2: Memory-constrained scenario
// =====================================================================

void example_memory_optimization()
{
  cout << "\n=== Example 2: Memory Optimization ===\n";

  const size_t num_elements = 1000000;  // 1M elements

  // Standard quotient filter
  Quotient_Filter<int> standard(20, 8);  // 2^20 slots

  // Compact quotient filter
  Compact_Quotient_Filter<int> compact(20, 8);

  cout << "For " << num_elements << " expected elements:\n\n";

  cout << "Standard Quotient_Filter:\n";
  cout << "  - Memory: " << standard.memory_usage() << " bytes (" << (standard.memory_usage() / 1024.0 / 1024.0)
       << " MB)\n";

  cout << "\nCompact_Quotient_Filter:\n";
  cout << "  - Memory: " << compact.memory_usage() << " bytes (" << (compact.memory_usage() / 1024.0 / 1024.0)
       << " MB)\n";

  double savings = 100.0 * (1.0 - static_cast<double>(compact.memory_usage()) / standard.memory_usage());
  cout << "\n*** Memory savings: " << fixed << setprecision(1) << savings << "% ***\n";
}

// =====================================================================
// Example 3: Using from_capacity factory
// =====================================================================

void example_from_capacity()
{
  cout << "\n=== Example 3: Auto-sizing with from_capacity ===\n";

  // Create filter for 10000 elements with 0.1% false positive rate
  auto qf = Compact_Quotient_Filter<string>::from_capacity(10000, 0.001);

  cout << "Auto-sized filter for 10000 elements, 0.1% FP rate:\n";
  cout << "  - q (quotient bits): " << static_cast<int>(qf.q()) << "\n";
  cout << "  - r (remainder bits): " << static_cast<int>(qf.r()) << "\n";
  cout << "  - Capacity: " << qf.capacity() << " slots\n";
  cout << "  - Actual FP rate: " << (qf.false_positive_rate() * 100) << "%\n";
  cout << "  - Memory: " << qf.memory_usage() << " bytes\n";

  // Insert some strings
  vector<string> users = {"alice@example.com", "bob@example.com", "charlie@example.com", "diana@example.com"};

  for (const auto &user : users)
    qf.insert(user);

  cout << "\nInserted " << users.size() << " user emails\n";

  // Test lookups
  cout << "\nLookup tests:\n";
  cout << "  alice@example.com: " << (qf.contains("alice@example.com") ? "FOUND" : "NOT FOUND") << "\n";
  cout << "  eve@example.com: " << (qf.contains("eve@example.com") ? "FOUND" : "NOT FOUND") << "\n";
}

// =====================================================================
// Example 4: Deletion support
// =====================================================================

void example_deletion()
{
  cout << "\n=== Example 4: Deletion Support ===\n";

  Compact_Quotient_Filter<string> active_sessions(12, 10);

  cout << "Session tracking with deletion:\n\n";

  // Add sessions
  vector<string> sessions = {"session_001", "session_002", "session_003", "session_004", "session_005"};

  for (const auto &sid : sessions)
    {
      active_sessions.insert(sid);
      cout << "  + Added " << sid << "\n";
    }

  cout << "\nActive sessions: " << active_sessions.size() << "\n";

  // Expire some sessions
  cout << "\nExpiring sessions 002 and 004:\n";
  active_sessions.remove("session_002");
  active_sessions.remove("session_004");

  cout << "Active sessions: " << active_sessions.size() << "\n";

  // Check which are still active
  cout << "\nSession status:\n";
  for (const auto &sid : sessions)
    cout << "  " << sid << ": " << (active_sessions.contains(sid) ? "ACTIVE" : "EXPIRED") << "\n";
}

// =====================================================================
// Example 5: Merging filters
// =====================================================================

void example_merge()
{
  cout << "\n=== Example 5: Merging Filters ===\n";

  const uint32_t seed = 42;

  Compact_Quotient_Filter<int> filter_a(10, 8, seed);
  Compact_Quotient_Filter<int> filter_b(10, 8, seed);

  // Populate filter A with even numbers
  cout << "Filter A: inserting even numbers [0, 20)\n";
  for (int i = 0; i < 20; i += 2)
    filter_a.insert(i);

  // Populate filter B with odd numbers
  cout << "Filter B: inserting odd numbers [1, 21)\n";
  for (int i = 1; i < 20; i += 2)
    filter_b.insert(i);

  cout << "\nBefore merge:\n";
  cout << "  Filter A size: " << filter_a.size() << "\n";
  cout << "  Filter B size: " << filter_b.size() << "\n";

  // Merge B into A
  filter_a.merge(filter_b);

  cout << "\nAfter merging B into A:\n";
  cout << "  Filter A size: " << filter_a.size() << "\n";

  // Verify all numbers are present
  cout << "\nVerifying merged filter contains [0, 20):\n";
  bool all_found = true;
  for (int i = 0; i < 20; ++i)
    if (!filter_a.contains(i))
      {
        cout << "  Missing: " << i << "\n";
        all_found = false;
      }

  if (all_found)
    cout << "  ✓ All elements found!\n";
}

// =====================================================================
// Example 6: Performance comparison
// =====================================================================

void example_performance()
{
  cout << "\n=== Example 6: Performance Comparison ===\n";

  const size_t n = 50000;
  const uint32_t seed = 12345;

  Quotient_Filter<int> standard(16, 8, seed);
  Compact_Quotient_Filter<int> compact(16, 8, seed);

  // Benchmark insertions
  auto start = chrono::high_resolution_clock::now();
  for (size_t i = 0; i < n; ++i)
    standard.insert(static_cast<int>(i));
  auto end = chrono::high_resolution_clock::now();
  auto standard_insert_us = chrono::duration_cast<chrono::microseconds>(end - start).count();

  start = chrono::high_resolution_clock::now();
  for (size_t i = 0; i < n; ++i)
    compact.insert(static_cast<int>(i));
  end = chrono::high_resolution_clock::now();
  auto compact_insert_us = chrono::duration_cast<chrono::microseconds>(end - start).count();

  // Benchmark lookups
  start = chrono::high_resolution_clock::now();
  for (size_t i = 0; i < n; ++i)
    standard.contains(static_cast<int>(i));
  end = chrono::high_resolution_clock::now();
  auto standard_lookup_us = chrono::duration_cast<chrono::microseconds>(end - start).count();

  start = chrono::high_resolution_clock::now();
  for (size_t i = 0; i < n; ++i)
    compact.contains(static_cast<int>(i));
  end = chrono::high_resolution_clock::now();
  auto compact_lookup_us = chrono::duration_cast<chrono::microseconds>(end - start).count();

  cout << "Benchmarking " << n << " operations:\n\n";

  cout << "Standard Quotient_Filter:\n";
  cout << "  - Insert time: " << standard_insert_us << " μs\n";
  cout << "  - Lookup time: " << standard_lookup_us << " μs\n";
  cout << "  - Memory: " << standard.memory_usage() << " bytes\n";

  cout << "\nCompact_Quotient_Filter:\n";
  cout << "  - Insert time: " << compact_insert_us << " μs\n";
  cout << "  - Lookup time: " << compact_lookup_us << " μs\n";
  cout << "  - Memory: " << compact.memory_usage() << " bytes\n";

  double insert_overhead = 100.0 * (static_cast<double>(compact_insert_us) / standard_insert_us - 1.0);
  double lookup_overhead = 100.0 * (static_cast<double>(compact_lookup_us) / standard_lookup_us - 1.0);
  double memory_savings = 100.0 * (1.0 - static_cast<double>(compact.memory_usage()) / standard.memory_usage());

  cout << "\nTrade-offs:\n";
  cout << "  - Insert overhead: " << fixed << setprecision(1) << insert_overhead << "%\n";
  cout << "  - Lookup overhead: " << lookup_overhead << "%\n";
  cout << "  - Memory savings: " << memory_savings << "%\n";
}

// =====================================================================
// Example 7: Real-world use case - URL deduplication
// =====================================================================

void example_url_deduplication()
{
  cout << "\n=== Example 7: URL Deduplication (Web Crawler) ===\n";

  // Compact filter for 10M URLs with 0.01% FP rate
  auto visited = Compact_Quotient_Filter<string>::from_capacity(10000000, 0.0001);

  cout << "Web crawler URL tracker:\n";
  cout << "  - Expected URLs: 10M\n";
  cout << "  - FP rate: " << (visited.false_positive_rate() * 100) << "%\n";
  cout << "  - Memory: " << (visited.memory_usage() / 1024.0 / 1024.0) << " MB\n";

  vector<string> urls = {
    "https://example.com/page1", "https://example.com/page2", "https://example.com/page3",
    "https://example.com/page1",  // duplicate
  };

  cout << "\nCrawling URLs:\n";
  for (const auto &url : urls)
    {
      if (visited.contains(url))
        cout << "  [SKIP] Already visited: " << url << "\n";
      else
        {
          cout << "  [CRAWL] New URL: " << url << "\n";
          visited.insert(url);
        }
    }

  cout << "\nTotal unique URLs visited: " << visited.size() << "\n";
}

// =====================================================================
// Example 8: Cache admission policy
// =====================================================================

void example_cache_admission()
{
  cout << "\n=== Example 8: Cache Admission Policy ===\n";

  // Track recently accessed items with compact filter
  Compact_Quotient_Filter<int> recent_access(14, 6);

  cout << "Cache admission filter (admit on second access):\n\n";

  vector<int> access_stream = {1, 2, 3, 1, 4, 2, 5, 1, 3};

  for (int item_id : access_stream)
    {
      if (recent_access.contains(item_id))
        {
          cout << "  Item " << item_id << ": Second access → ADMIT TO CACHE\n";
        }
      else
        {
          cout << "  Item " << item_id << ": First access → track only\n";
          recent_access.insert(item_id);
        }
    }
}

// =====================================================================
// Main
// =====================================================================

int main()
{
  cout << "╔════════════════════════════════════════════════════════════╗\n";
  cout << "║   Compact Quotient Filter - Comprehensive Examples        ║\n";
  cout << "╚════════════════════════════════════════════════════════════╝\n";

  example_basic_usage();
  example_memory_optimization();
  example_from_capacity();
  example_deletion();
  example_merge();
  example_performance();
  example_url_deduplication();
  example_cache_admission();

  cout << "\n╔════════════════════════════════════════════════════════════╗\n";
  cout << "║   All examples completed successfully!                     ║\n";
  cout << "╚════════════════════════════════════════════════════════════╝\n";

  return 0;
}
