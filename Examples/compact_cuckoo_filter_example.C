
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/** @file compact_cuckoo_filter_example.C
 *  @brief Comprehensive usage examples for Compact_Cuckoo_Filter
 */

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>

#include <compact-cuckoo-filter.H>
#include <cuckoo-filter.H>

using namespace Aleph;
using namespace std;

template <size_t FingerprintBits, size_t EntriesPerBucket> double cuckoo_theoretical_fp_rate()
{
  return (2.0 * EntriesPerBucket) / std::ldexp(1.0, FingerprintBits);
}

template <size_t EntriesPerBucket> size_t standard_cuckoo_memory_usage(size_t capacity)
{
  static_cast<void>(EntriesPerBucket);
  return capacity * sizeof(uint32_t);
}

// =====================================================================
// Example 1: Basic usage
// =====================================================================

void example_basic_usage()
{
  cout << "\n=== Example 1: Basic Usage ===\n";

  // Create a compact cuckoo filter for ~1000 elements
  Compact_Cuckoo_Filter<int> cf(1000);

  cout << "Created filter with:\n";
  cout << "  - Capacity: " << cf.capacity() << " entries\n";
  cout << "  - Fingerprint bits: " << 8 << "\n";
  cout << "  - Entries per bucket: " << 4 << "\n";
  cout << "  - Theoretical false positive rate: " << (cuckoo_theoretical_fp_rate<8, 4>() * 100) << "%\n";
  cout << "  - Memory usage: " << cf.memory_usage() << " bytes\n";

  // Insert elements
  vector<int> items = {42, 100, 256, 512, 1024, 2048};
  for (int item : items)
    {
      if (cf.insert(item))
        cout << "  Inserted: " << item << "\n";
      else
        cout << "  Failed to insert: " << item << " (filter full)\n";
    }

  cout << "\nCurrent size: " << cf.size() << "\n";
  cout << "Load factor: " << (cf.load_factor() * 100) << "%\n";

  // Test membership
  cout << "\nMembership tests:\n";
  for (int item : items)
    cout << "  contains(" << item << "): " << (cf.contains(item) ? "YES" : "NO") << "\n";

  cout << "  contains(999): " << (cf.contains(999) ? "YES (false positive)" : "NO") << "\n";

  // Remove an element
  if (cf.remove(256))
    cout << "\nSuccessfully removed 256\n";

  cout << "  contains(256): " << (cf.contains(256) ? "YES" : "NO") << "\n";
  cout << "  Size: " << cf.size() << "\n";
}

// =====================================================================
// Example 2: Memory-constrained scenario
// =====================================================================

void example_memory_optimization()
{
  cout << "\n=== Example 2: Memory Optimization ===\n";

  const size_t num_elements = 1000000;  // 1M elements

  // Standard cuckoo filter
  Cuckoo_Filter<int> standard(num_elements);

  // Compact cuckoo filter with 8-bit fingerprints
  Compact_Cuckoo_Filter<int, 8, 4> compact(num_elements);

  cout << "For " << num_elements << " expected elements:\n\n";

  cout << "Standard Cuckoo_Filter:\n";
  const size_t standard_memory = standard_cuckoo_memory_usage<4>(standard.capacity());
  cout << "  - Memory: " << standard_memory << " bytes (" << (standard_memory / 1024.0 / 1024.0) << " MB)\n";
  cout << "  - Fingerprint size: 8 bits stored in 32-bit slots\n";

  cout << "\nCompact_Cuckoo_Filter (8-bit FP):\n";
  cout << "  - Memory: " << compact.memory_usage() << " bytes (" << (compact.memory_usage() / 1024.0 / 1024.0)
       << " MB)\n";
  cout << "  - Fingerprint size: 8 bits\n";

  double savings = 100.0 * (1.0 - static_cast<double>(compact.memory_usage()) / standard_memory);
  cout << "\n*** Memory savings: " << fixed << setprecision(1) << savings << "% ***\n";
}

// =====================================================================
// Example 3: Custom fingerprint sizes
// =====================================================================

void example_custom_fingerprints()
{
  cout << "\n=== Example 3: Custom Fingerprint Sizes ===\n";

  // 4-bit fingerprints: minimal memory, high FP rate
  Compact_Cuckoo_Filter<int, 4> tiny(1000);

  // 12-bit fingerprints: balanced
  Compact_Cuckoo_Filter<int, 12> balanced(1000);

  // 16-bit fingerprints: low FP rate
  Compact_Cuckoo_Filter<int, 16> precise(1000);

  cout << "Fingerprint size comparison:\n\n";

  cout << "4-bit fingerprints:\n";
  cout << "  - Theoretical FP rate: " << (cuckoo_theoretical_fp_rate<4, 4>() * 100) << "%\n";
  cout << "  - Memory: " << tiny.memory_usage() << " bytes\n";

  cout << "\n12-bit fingerprints:\n";
  cout << "  - Theoretical FP rate: " << (cuckoo_theoretical_fp_rate<12, 4>() * 100) << "%\n";
  cout << "  - Memory: " << balanced.memory_usage() << " bytes\n";

  cout << "\n16-bit fingerprints:\n";
  cout << "  - Theoretical FP rate: " << (cuckoo_theoretical_fp_rate<16, 4>() * 100) << "%\n";
  cout << "  - Memory: " << precise.memory_usage() << " bytes\n";
}

// =====================================================================
// Example 4: Deletion support
// =====================================================================

void example_deletion()
{
  cout << "\n=== Example 4: Deletion Support ===\n";

  Compact_Cuckoo_Filter<string> active_users(5000);

  cout << "User session tracking with deletion:\n\n";

  // Add users
  vector<string> users = {"alice@example.com", "bob@example.com", "charlie@example.com", "diana@example.com",
                          "eve@example.com"};

  for (const auto &user : users)
    {
      active_users.insert(user);
      cout << "  + User logged in: " << user << "\n";
    }

  cout << "\nActive users: " << active_users.size() << "\n";

  // Users log out
  cout << "\nUsers logging out:\n";
  active_users.remove("bob@example.com");
  active_users.remove("diana@example.com");
  cout << "  - bob@example.com\n";
  cout << "  - diana@example.com\n";

  cout << "\nActive users: " << active_users.size() << "\n";

  // Check who's still active
  cout << "\nUser status:\n";
  for (const auto &user : users)
    cout << "  " << user << ": " << (active_users.contains(user) ? "ACTIVE" : "LOGGED OUT") << "\n";
}

// =====================================================================
// Example 5: Handling duplicates
// =====================================================================

void example_duplicates()
{
  cout << "\n=== Example 5: Duplicate Handling ===\n";

  Compact_Cuckoo_Filter<int> cf(1000);

  cout << "Cuckoo filters allow duplicate insertions:\n\n";

  // Insert same element multiple times
  cout << "Inserting 42 three times:\n";
  for (int i = 0; i < 3; ++i)
    {
      cf.insert(42);
      cout << "  Insert #" << (i + 1) << " - Size: " << cf.size() << "\n";
    }

  cout << "\nRemoving 42 twice:\n";
  for (int i = 0; i < 2; ++i)
    {
      cf.remove(42);
      cout << "  Remove #" << (i + 1) << " - Size: " << cf.size() << "\n";
    }

  cout << "\ncontains(42): " << (cf.contains(42) ? "YES (1 copy remains)" : "NO") << "\n";

  cout << "\n⚠️  To prevent duplicates, check before inserting:\n";
  cout << "    if (!cf.contains(item)) cf.insert(item);\n";
}

// =====================================================================
// Example 6: Performance comparison
// =====================================================================

void example_performance()
{
  cout << "\n=== Example 6: Performance Comparison ===\n";

  const size_t n = 50000;
  size_t hits = 0;

  Cuckoo_Filter<int> standard(100000);
  Compact_Cuckoo_Filter<int> compact(100000);

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
    hits += standard.contains(static_cast<int>(i)) ? 1u : 0u;
  end = chrono::high_resolution_clock::now();
  auto standard_lookup_us = chrono::duration_cast<chrono::microseconds>(end - start).count();

  start = chrono::high_resolution_clock::now();
  for (size_t i = 0; i < n; ++i)
    hits += compact.contains(static_cast<int>(i)) ? 1u : 0u;
  end = chrono::high_resolution_clock::now();
  auto compact_lookup_us = chrono::duration_cast<chrono::microseconds>(end - start).count();

  cout << "Benchmarking " << n << " operations:\n\n";

  cout << "Standard Cuckoo_Filter:\n";
  cout << "  - Insert time: " << standard_insert_us << " μs\n";
  cout << "  - Lookup time: " << standard_lookup_us << " μs\n";
  const size_t standard_memory = standard_cuckoo_memory_usage<4>(standard.capacity());
  cout << "  - Memory: " << standard_memory << " bytes\n";

  cout << "\nCompact_Cuckoo_Filter:\n";
  cout << "  - Insert time: " << compact_insert_us << " μs\n";
  cout << "  - Lookup time: " << compact_lookup_us << " μs\n";
  cout << "  - Memory: " << compact.memory_usage() << " bytes\n";

  double insert_overhead = 100.0 * (static_cast<double>(compact_insert_us) / standard_insert_us - 1.0);
  double lookup_overhead = 100.0 * (static_cast<double>(compact_lookup_us) / standard_lookup_us - 1.0);
  double memory_savings = 100.0 * (1.0 - static_cast<double>(compact.memory_usage()) / standard_memory);

  cout << "\nTrade-offs:\n";
  cout << "  - Insert overhead: " << fixed << setprecision(1) << insert_overhead << "%\n";
  cout << "  - Lookup overhead: " << lookup_overhead << "%\n";
  cout << "  - Memory savings: " << memory_savings << "%\n";
  cout << "  - Successful lookups observed: " << hits << "\n";
}

// =====================================================================
// Example 7: Real-world use case - Network packet deduplication
// =====================================================================

void example_packet_deduplication()
{
  cout << "\n=== Example 7: Network Packet Deduplication ===\n";

  // Track seen packet IDs with compact filter
  Compact_Cuckoo_Filter<uint64_t, 12> seen_packets(1000000);

  cout << "Network packet deduplication filter:\n";
  cout << "  - Expected packets: 1M\n";
  cout << "  - Theoretical FP rate: " << (cuckoo_theoretical_fp_rate<12, 4>() * 100) << "%\n";
  cout << "  - Memory: " << (seen_packets.memory_usage() / 1024.0 / 1024.0) << " MB\n";

  vector<uint64_t> packet_stream = {
    0x1234567890ABCDEF, 0xFEDCBA0987654321, 0x1111222233334444,
    0x1234567890ABCDEF,  // duplicate
    0x5555666677778888,
  };

  cout << "\nProcessing packet stream:\n";
  for (uint64_t packet_id : packet_stream)
    {
      if (seen_packets.contains(packet_id))
        cout << "  [DROP] Duplicate packet: 0x" << hex << packet_id << dec << "\n";
      else
        {
          cout << "  [ACCEPT] New packet: 0x" << hex << packet_id << dec << "\n";
          seen_packets.insert(packet_id);
        }
    }

  cout << "\nTotal unique packets: " << seen_packets.size() << "\n";
}

// =====================================================================
// Example 8: Embedded systems - minimal memory footprint
// =====================================================================

void example_embedded_systems()
{
  cout << "\n=== Example 8: Embedded Systems (Minimal Memory) ===\n";

  // Ultra-compact filter with 6-bit fingerprints
  Compact_Cuckoo_Filter<int, 6, 4> tiny_filter(1000);

  cout << "Embedded device filter (6-bit fingerprints):\n";
  cout << "  - Capacity: " << tiny_filter.capacity() << " entries\n";
  cout << "  - Memory: " << tiny_filter.memory_usage() << " bytes\n";
  cout << "  - Theoretical FP rate: " << (cuckoo_theoretical_fp_rate<6, 4>() * 100) << "%\n";

  // Simulate sensor readings
  cout << "\nTracking sensor events:\n";
  for (int sensor_id = 1; sensor_id <= 10; ++sensor_id)
    {
      tiny_filter.insert(sensor_id);
      cout << "  Sensor " << sensor_id << " triggered\n";
    }

  cout << "\nMemory footprint: " << tiny_filter.memory_usage() << " bytes\n";
  cout << "(Compare to standard: would use ~" << (tiny_filter.capacity() / 4 * 16) << " bytes)\n";
}

// =====================================================================
// Main
// =====================================================================

int main()
{
  cout << "╔════════════════════════════════════════════════════════════╗\n";
  cout << "║   Compact Cuckoo Filter - Comprehensive Examples          ║\n";
  cout << "╚════════════════════════════════════════════════════════════╝\n";

  example_basic_usage();
  example_memory_optimization();
  example_custom_fingerprints();
  example_deletion();
  example_duplicates();
  example_performance();
  example_packet_deduplication();
  example_embedded_systems();

  cout << "\n╔════════════════════════════════════════════════════════════╗\n";
  cout << "║   All examples completed successfully!                     ║\n";
  cout << "╚════════════════════════════════════════════════════════════╝\n";

  return 0;
}
