
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file compact-cuckoo-filter.cc
 *  @brief Comprehensive tests for Compact_Cuckoo_Filter
 */

#include <gtest/gtest.h>

#include <cuckoo-filter.H>
#include <compact-cuckoo-filter.H>

#include <cstddef>
#include <string>
#include <set>
#include <random>
#include <vector>
#include <algorithm>

using namespace Aleph;
using namespace std;

// =====================================================================
// Construction
// =====================================================================

TEST(CompactCuckooFilterCtor, default_construction)
{
  Compact_Cuckoo_Filter<int> cf(1000);
  EXPECT_GE(cf.capacity(), 1000u);
  EXPECT_EQ(cf.size(), 0u);
  EXPECT_TRUE(cf.is_empty());
}

TEST(CompactCuckooFilterCtor, custom_fingerprint_bits)
{
  Compact_Cuckoo_Filter<int, 12> cf(1000);  // 12-bit fingerprints
  EXPECT_GE(cf.capacity(), 1000u);
}

TEST(CompactCuckooFilterCtor, custom_entries_per_bucket)
{
  Compact_Cuckoo_Filter<int, 8, 8> cf(1000);  // 8 entries per bucket
  EXPECT_GE(cf.capacity(), 1000u);
}

TEST(CompactCuckooFilterCtor, memory_usage_is_compact)
{
  // Standard: 4 entries × 32 bits = 16 bytes/bucket
  // Compact with 8-bit FP: 4 entries × 8 bits = 4 bytes/bucket
  Compact_Cuckoo_Filter<int, 8, 4> cf(1000);

  size_t num_buckets = cf.capacity() / 4;
  size_t expected_bytes = (num_buckets * 4 * 8 + 7) / 8;  // 4 entries × 8 bits

  EXPECT_LE(cf.memory_usage(), expected_bytes + 100);  // Allow small overhead

  // Should be much less than standard (16 bytes/bucket)
  EXPECT_LT(cf.memory_usage(), num_buckets * 10);
}

// =====================================================================
// Basic insert / contains
// =====================================================================

TEST(CompactCuckooFilterBasic, insert_and_contains_no_false_negatives)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(cf.insert(i));

  EXPECT_EQ(cf.size(), 100u);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(cf.contains(i)) << "False negative for i=" << i;
}

TEST(CompactCuckooFilterBasic, absent_elements_usually_not_found)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  for (int i = 0; i < 100; ++i)
    cf.insert(i);

  size_t false_positives = 0;
  for (int i = 1000; i < 2000; ++i)
    if (cf.contains(i))
      ++false_positives;

  // With 8-bit FP and 4 entries/bucket, FP rate ≈ 3.1%
  // Testing 1000 elements: expect < 50 false positives
  EXPECT_LT(false_positives, 60u) << "Too many false positives: " << false_positives;
}

TEST(CompactCuckooFilterBasic, duplicate_insert_stores_twice)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  EXPECT_TRUE(cf.insert(42));
  EXPECT_EQ(cf.size(), 1u);

  // Cuckoo filter allows duplicates
  EXPECT_TRUE(cf.insert(42));
  EXPECT_EQ(cf.size(), 2u);

  EXPECT_TRUE(cf.contains(42));
}

TEST(CompactCuckooFilterBasic, string_keys)
{
  Compact_Cuckoo_Filter<string> cf(1000);

  cf.insert("hello");
  cf.insert("world");
  cf.insert("aleph");

  EXPECT_TRUE(cf.contains("hello"));
  EXPECT_TRUE(cf.contains("world"));
  EXPECT_TRUE(cf.contains("aleph"));

  EXPECT_EQ(cf.size(), 3u);
}

TEST(CompactCuckooFilterBasic, small_fingerprint_bits)
{
  // 4-bit fingerprints → high FP rate but minimal memory
  Compact_Cuckoo_Filter<int, 4> cf(1000);

  for (int i = 0; i < 50; ++i)
    cf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(cf.contains(i));
}

TEST(CompactCuckooFilterBasic, large_fingerprint_bits)
{
  // 16-bit fingerprints → very low FP rate
  Compact_Cuckoo_Filter<int, 16> cf(1000);

  for (int i = 0; i < 50; ++i)
    cf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(cf.contains(i));
}

// =====================================================================
// Deletion
// =====================================================================

TEST(CompactCuckooFilterDelete, remove_existing_element)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  cf.insert(10);
  cf.insert(20);
  cf.insert(30);
  EXPECT_EQ(cf.size(), 3u);

  EXPECT_TRUE(cf.remove(20));
  EXPECT_EQ(cf.size(), 2u);
  EXPECT_FALSE(cf.contains(20));
  EXPECT_TRUE(cf.contains(10));
  EXPECT_TRUE(cf.contains(30));
}

TEST(CompactCuckooFilterDelete, remove_nonexistent_is_noop)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  cf.insert(1);
  EXPECT_FALSE(cf.remove(999));
  EXPECT_EQ(cf.size(), 1u);
  EXPECT_TRUE(cf.contains(1));
}

TEST(CompactCuckooFilterDelete, remove_duplicate_removes_one_copy)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  cf.insert(5);
  cf.insert(5);  // duplicate
  EXPECT_EQ(cf.size(), 2u);

  EXPECT_TRUE(cf.remove(5));
  EXPECT_EQ(cf.size(), 1u);
  EXPECT_TRUE(cf.contains(5));  // Still has one copy

  EXPECT_TRUE(cf.remove(5));
  EXPECT_EQ(cf.size(), 0u);
  EXPECT_FALSE(cf.contains(5));
}

TEST(CompactCuckooFilterDelete, insert_after_remove)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  cf.insert(5);
  cf.remove(5);
  EXPECT_FALSE(cf.contains(5));

  cf.insert(5);
  EXPECT_TRUE(cf.contains(5));
  EXPECT_EQ(cf.size(), 1u);
}

TEST(CompactCuckooFilterDelete, interleaved_insert_remove)
{
  Compact_Cuckoo_Filter<int> cf(2000);

  for (int i = 0; i < 100; ++i)
    cf.insert(i);

  // Remove every other element
  for (int i = 0; i < 100; i += 2)
    EXPECT_TRUE(cf.remove(i));

  EXPECT_EQ(cf.size(), 50u);

  // Verify odd elements remain
  for (int i = 1; i < 100; i += 2)
    EXPECT_TRUE(cf.contains(i)) << "Lost i=" << i;

  // Verify even elements removed
  for (int i = 0; i < 100; i += 2)
    EXPECT_FALSE(cf.contains(i)) << "Still has i=" << i;
}

// =====================================================================
// Capacity and load factor
// =====================================================================

TEST(CompactCuckooFilterCapacity, load_factor_computed_correctly)
{
  Compact_Cuckoo_Filter<int> cf(100);
  EXPECT_DOUBLE_EQ(cf.load_factor(), 0.0);

  int inserted = 0;
  for (int i = 0; i < 50; ++i)
    if (cf.insert(i))
      ++inserted;

  double lf = cf.load_factor();
  EXPECT_GT(lf, 0.3);  // At least 30% of capacity
  EXPECT_LT(lf, 0.6);
}

TEST(CompactCuckooFilterCapacity, high_load_factor_still_works)
{
  Compact_Cuckoo_Filter<int> cf(1000, 42);

  std::set<int> inserted_set;
  // Fill to ~90% capacity
  for (int i = 0; i < 1000; ++i)
    if (cf.insert(i))
      inserted_set.insert(i);

  EXPECT_GT(inserted_set.size(), 800u);

  // All inserted elements should still be found
  for (int val : inserted_set)
    EXPECT_TRUE(cf.contains(val)) << "Lost val=" << val;
}

TEST(CompactCuckooFilterCapacity, insert_fails_when_full)
{
  Compact_Cuckoo_Filter<int> cf(100);

  int successful = 0;
  for (int i = 0; i < 200; ++i)
    if (cf.insert(i))
      ++successful;

  // Should have inserted most but not all
  EXPECT_GT(successful, 80);
  EXPECT_LT(successful, 200);
}

// =====================================================================
// Introspection
// =====================================================================

TEST(CompactCuckooFilterIntrospection, memory_usage)
{
  Compact_Cuckoo_Filter<int, 8, 4> cf(1000);

  // Should use ~4 bytes/bucket (4 × 8 bits)
  size_t num_buckets = cf.capacity() / 4;
  size_t expected = (num_buckets * 4 * 8 + 7) / 8;

  EXPECT_LE(cf.memory_usage(), expected + 100);  // Allow small overhead
}

TEST(CompactCuckooFilterIntrospection, load_factor)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  for (int i = 0; i < 100; ++i)
    cf.insert(i);

  double lf = cf.load_factor();
  EXPECT_GT(lf, 0.04);
  EXPECT_LT(lf, 0.15);
}

// =====================================================================
// Copy / Move semantics
// =====================================================================

TEST(CompactCuckooFilterSemantics, copy_preserves_state)
{
  Compact_Cuckoo_Filter<int> cf(1000);
  for (int i = 0; i < 30; ++i)
    cf.insert(i);

  Compact_Cuckoo_Filter<int> copy(cf);
  EXPECT_EQ(copy.size(), cf.size());
  EXPECT_EQ(copy.capacity(), cf.capacity());

  for (int i = 0; i < 30; ++i)
    EXPECT_TRUE(copy.contains(i));
}

TEST(CompactCuckooFilterSemantics, move_transfers_state)
{
  Compact_Cuckoo_Filter<int> cf(1000);
  for (int i = 0; i < 30; ++i)
    cf.insert(i);

  size_t orig_size = cf.size();
  Compact_Cuckoo_Filter<int> moved(std::move(cf));

  EXPECT_EQ(moved.size(), orig_size);
  for (int i = 0; i < 30; ++i)
    EXPECT_TRUE(moved.contains(i));
}

// =====================================================================
// Clear
// =====================================================================

TEST(CompactCuckooFilterClear, clears_all_state)
{
  Compact_Cuckoo_Filter<int> cf(1000);
  for (int i = 0; i < 50; ++i)
    cf.insert(i);

  cf.clear();
  EXPECT_EQ(cf.size(), 0u);
  EXPECT_TRUE(cf.is_empty());

  for (int i = 0; i < 50; ++i)
    EXPECT_FALSE(cf.contains(i));
}

TEST(CompactCuckooFilterClear, can_reuse_after_clear)
{
  Compact_Cuckoo_Filter<int> cf(1000);

  for (int i = 0; i < 20; ++i)
    cf.insert(i);

  cf.clear();

  for (int i = 100; i < 120; ++i)
    cf.insert(i);

  EXPECT_EQ(cf.size(), 20u);
  for (int i = 100; i < 120; ++i)
    EXPECT_TRUE(cf.contains(i));
}

// =====================================================================
// False positive rate (statistical)
// =====================================================================

TEST(CompactCuckooFilterFPRate, empirical_fp_rate_within_bounds)
{
  const size_t n = 500;
  Compact_Cuckoo_Filter<int, 8, 4> cf(2000);

  for (size_t i = 0; i < n; ++i)
    cf.insert(static_cast<int>(i));

  size_t test_count = 10000;
  size_t fps = 0;
  for (size_t i = n + 10000; i < n + 10000 + test_count; ++i)
    if (cf.contains(static_cast<int>(i)))
      ++fps;

  double empirical = static_cast<double>(fps) / test_count;

  // With 8-bit FP and 4 entries/bucket, theoretical FP ≈ 3.1%
  // Empirical should be reasonably close
  EXPECT_LT(empirical, 0.10)  // < 10%
    << "Empirical FP rate " << empirical << " too high";
}

TEST(CompactCuckooFilterFPRate, low_fp_rate_with_large_fingerprint)
{
  Compact_Cuckoo_Filter<int, 16> cf(1000);

  for (int i = 0; i < 200; ++i)
    cf.insert(i);

  size_t fps = 0;
  for (int i = 10000; i < 20000; ++i)
    if (cf.contains(i))
      ++fps;

  // With 16-bit FP, expect very few false positives
  EXPECT_LT(fps, 20u);
}

// =====================================================================
// Stress tests
// =====================================================================

TEST(CompactCuckooFilterStress, random_insertions_and_lookups)
{
  Compact_Cuckoo_Filter<int> cf(5000, 42);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 1000000);

  std::set<int> inserted;

  // Insert 2000 random elements
  for (int i = 0; i < 2000; ++i)
    {
      int val = dist(rng);
      if (cf.insert(val))
        inserted.insert(val);
    }

  // All inserted elements must be found
  for (int val : inserted)
    EXPECT_TRUE(cf.contains(val)) << "Lost value " << val;
}

TEST(CompactCuckooFilterStress, sequential_insert_remove_cycles)
{
  Compact_Cuckoo_Filter<int> cf(2000, 42);

  for (int cycle = 0; cycle < 5; ++cycle)
    {
      cf.clear();

      // Insert 200 elements
      for (int i = 0; i < 200; ++i)
        cf.insert(cycle * 1000 + i);

      // Remove half
      for (int i = 0; i < 100; ++i)
        cf.remove(cycle * 1000 + i);

      // Verify remaining half
      for (int i = 100; i < 200; ++i)
        EXPECT_TRUE(cf.contains(cycle * 1000 + i)) << "Cycle " << cycle << ", lost i=" << i;
    }
}

TEST(CompactCuckooFilterStress, many_duplicates)
{
  Compact_Cuckoo_Filter<int> cf(1000, 42);

  // Insert same element multiple times (some may fail due to cuckoo hashing)
  int successful_inserts = 0;
  for (int i = 0; i < 10; ++i)
    if (cf.insert(42))
      ++successful_inserts;

  EXPECT_GE(successful_inserts, 5);  // At least half should succeed
  EXPECT_EQ(cf.size(), static_cast<size_t>(successful_inserts));
  EXPECT_TRUE(cf.contains(42));

  // Remove half
  int removals = successful_inserts / 2;
  for (int i = 0; i < removals; ++i)
    EXPECT_TRUE(cf.remove(42));

  EXPECT_EQ(cf.size(), static_cast<size_t>(successful_inserts - removals));
  if (successful_inserts > removals)
    EXPECT_TRUE(cf.contains(42));  // Still has copies remaining
}

// =====================================================================
// Edge cases
// =====================================================================

TEST(CompactCuckooFilterEdge, tiny_filter)
{
  Compact_Cuckoo_Filter<int> cf(10);

  cf.insert(1);
  EXPECT_TRUE(cf.contains(1));

  cf.insert(2);
  EXPECT_TRUE(cf.contains(1));
  EXPECT_TRUE(cf.contains(2));
}

TEST(CompactCuckooFilterEdge, minimal_fingerprint_bits)
{
  Compact_Cuckoo_Filter<int, 1> cf(1000);  // 1-bit FP

  for (int i = 0; i < 50; ++i)
    cf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(cf.contains(i));
}

TEST(CompactCuckooFilterEdge, maximal_fingerprint_bits)
{
  Compact_Cuckoo_Filter<int, 32> cf(1000);  // 32-bit FP

  cf.insert(42);
  EXPECT_TRUE(cf.contains(42));
  EXPECT_FALSE(cf.contains(43));
}

TEST(CompactCuckooFilterEdge, single_entry_per_bucket)
{
  Compact_Cuckoo_Filter<int, 8, 1> cf(1000);  // 1 entry/bucket

  for (int i = 0; i < 50; ++i)
    cf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(cf.contains(i));
}

// =====================================================================
// Comparison with standard Cuckoo_Filter
// =====================================================================

TEST(CompactCuckooFilterComparison, same_behavior_as_standard)
{
  Cuckoo_Filter<int> standard(1000);
  Compact_Cuckoo_Filter<int> compact(1000);

  std::vector<int> values = {1, 5, 10, 42, 99, 123, 456, 789};

  for (int val : values)
    {
      standard.insert(val);
      compact.insert(val);
    }

  // Both should find all inserted elements
  for (int val : values)
    {
      EXPECT_TRUE(standard.contains(val));
      EXPECT_TRUE(compact.contains(val));
    }

  // Both should have same size
  EXPECT_EQ(standard.size(), compact.size());
}

TEST(CompactCuckooFilterComparison, memory_savings_verified)
{
  Compact_Cuckoo_Filter<int, 8, 4> compact(1000);

  size_t compact_mem = compact.memory_usage();

  // Compact uses ~4 bytes/bucket (4 × 8 bits)
  // Standard would use ~16 bytes/bucket (4 × 32 bits)
  size_t num_buckets = compact.capacity() / 4;
  size_t expected_compact = (num_buckets * 4 * 8 + 7) / 8;  // 4 entries × 8 bits
  size_t expected_standard = num_buckets * 16;              // 4 entries × 32 bits

  // Verify compact is close to theoretical minimum
  EXPECT_LE(compact_mem, expected_compact + 100);  // Allow small overhead

  // Verify it's much less than standard would be
  EXPECT_LT(compact_mem, expected_standard / 3);  // < 33% of standard
}

TEST(CompactCuckooFilterComparison, performance_characteristics)
{
  // This test just verifies both work correctly under load
  Cuckoo_Filter<int> standard(5000);
  Compact_Cuckoo_Filter<int> compact(5000);

  for (int i = 0; i < 1000; ++i)
    {
      standard.insert(i);
      compact.insert(i);
    }

  // Both should have similar success rates
  EXPECT_EQ(standard.size(), 1000u);
  EXPECT_EQ(compact.size(), 1000u);

  // Both should find all elements
  for (int i = 0; i < 1000; ++i)
    {
      EXPECT_TRUE(standard.contains(i));
      EXPECT_TRUE(compact.contains(i));
    }
}
