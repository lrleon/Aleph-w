
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

/** @file compact-quotient-filter.cc
 *  @brief Comprehensive tests for Compact_Quotient_Filter
 */

#include <gtest/gtest.h>

#include <quotient-filter.H>
#include <compact-quotient-filter.H>

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

TEST(CompactQuotientFilterCtor, valid_parameters)
{
  Compact_Quotient_Filter<int> qf(4, 8);
  EXPECT_EQ(qf.q(), 4);
  EXPECT_EQ(qf.r(), 8);
  EXPECT_EQ(qf.capacity(), 16u);
  EXPECT_EQ(qf.size(), 0u);
  EXPECT_TRUE(qf.is_empty());
}

TEST(CompactQuotientFilterCtor, rejects_invalid_q)
{
  EXPECT_THROW((Compact_Quotient_Filter<int>(0, 8)), std::domain_error);
  EXPECT_THROW((Compact_Quotient_Filter<int>(33, 8)), std::domain_error);
}

TEST(CompactQuotientFilterCtor, rejects_invalid_r)
{
  EXPECT_THROW((Compact_Quotient_Filter<int>(4, 0)), std::domain_error);
  EXPECT_THROW((Compact_Quotient_Filter<int>(4, 61)), std::domain_error);
}

TEST(CompactQuotientFilterCtor, rejects_q_plus_r_exceeds_64)
{
  EXPECT_THROW((Compact_Quotient_Filter<int>(32, 33)), std::domain_error);
}

TEST(CompactQuotientFilterCtor, from_capacity_creates_valid_filter)
{
  auto qf = Compact_Quotient_Filter<int>::from_capacity(1000, 0.01);
  EXPECT_GE(qf.capacity(), 1000u);
  EXPECT_LE(qf.false_positive_rate(), 0.02);
}

TEST(CompactQuotientFilterCtor, from_capacity_rejects_bad_args)
{
  EXPECT_THROW(Compact_Quotient_Filter<int>::from_capacity(0, 0.01), std::domain_error);
  EXPECT_THROW(Compact_Quotient_Filter<int>::from_capacity(100, 0.0), std::domain_error);
  EXPECT_THROW(Compact_Quotient_Filter<int>::from_capacity(100, 1.0), std::domain_error);
}

TEST(CompactQuotientFilterCtor, memory_usage_is_compact)
{
  Compact_Quotient_Filter<int> qf(10, 8);       // 1024 slots, 11 bits/slot
  size_t expected_bytes = (1024 * 11 + 7) / 8;  // 1408 bytes
  EXPECT_EQ(qf.memory_usage(), expected_bytes);

  // Compare to standard: would be 1024 * 8 = 8192 bytes
  // Savings: ~82%
  EXPECT_LT(qf.memory_usage(), 1024u * 2);
}

// =====================================================================
// Basic insert / contains
// =====================================================================

TEST(CompactQuotientFilterBasic, insert_and_contains_no_false_negatives)
{
  Compact_Quotient_Filter<int> qf(10, 8);

  for (int i = 0; i < 100; ++i)
    qf.insert(i);

  EXPECT_EQ(qf.size(), 100u);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(qf.contains(i)) << "False negative for i=" << i;
}

TEST(CompactQuotientFilterBasic, absent_elements_usually_not_found)
{
  Compact_Quotient_Filter<int> qf(12, 10);

  for (int i = 0; i < 100; ++i)
    qf.insert(i);

  size_t false_positives = 0;
  for (int i = 1000; i < 2000; ++i)
    if (qf.contains(i))
      ++false_positives;

  // With r=10, FP rate ≈ 1/1024 ≈ 0.1%.
  // Testing 1000 elements: expect < 10 false positives.
  EXPECT_LT(false_positives, 20u) << "Too many false positives: " << false_positives;
}

TEST(CompactQuotientFilterBasic, duplicate_insert_is_idempotent)
{
  Compact_Quotient_Filter<int> qf(8, 8);

  qf.insert(42);
  EXPECT_EQ(qf.size(), 1u);

  qf.insert(42);
  EXPECT_EQ(qf.size(), 1u);

  EXPECT_TRUE(qf.contains(42));
}

TEST(CompactQuotientFilterBasic, string_keys)
{
  Compact_Quotient_Filter<string> qf(10, 8);

  qf.insert("hello");
  qf.insert("world");
  qf.insert("aleph");

  EXPECT_TRUE(qf.contains("hello"));
  EXPECT_TRUE(qf.contains("world"));
  EXPECT_TRUE(qf.contains("aleph"));

  EXPECT_EQ(qf.size(), 3u);
}

TEST(CompactQuotientFilterBasic, small_remainder_bits)
{
  // Test with very small r (high FP rate but minimal memory)
  Compact_Quotient_Filter<int> qf(8, 4);  // 256 slots, 7 bits/slot

  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(qf.contains(i));

  // Memory should be tiny: 256 * 7 / 8 = 224 bytes
  EXPECT_EQ(qf.memory_usage(), 224u);
}

TEST(CompactQuotientFilterBasic, large_remainder_bits)
{
  // Test with large r (low FP rate)
  Compact_Quotient_Filter<int> qf(8, 20);  // 256 slots, 23 bits/slot

  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(qf.contains(i));

  // FP rate should be ~2^-20 ≈ 0.0001%
  EXPECT_LT(qf.false_positive_rate(), 0.000002);
}

// =====================================================================
// Deletion
// =====================================================================

TEST(CompactQuotientFilterDelete, remove_existing_element)
{
  Compact_Quotient_Filter<int> qf(8, 8);

  qf.insert(10);
  qf.insert(20);
  qf.insert(30);
  EXPECT_EQ(qf.size(), 3u);

  EXPECT_TRUE(qf.remove(20));
  EXPECT_EQ(qf.size(), 2u);
  EXPECT_FALSE(qf.contains(20));
  EXPECT_TRUE(qf.contains(10));
  EXPECT_TRUE(qf.contains(30));
}

TEST(CompactQuotientFilterDelete, remove_nonexistent_is_noop)
{
  Compact_Quotient_Filter<int> qf(8, 8);

  qf.insert(1);
  EXPECT_FALSE(qf.remove(999));
  EXPECT_EQ(qf.size(), 1u);
  EXPECT_TRUE(qf.contains(1));
}

TEST(CompactQuotientFilterDelete, remove_all_makes_empty)
{
  Compact_Quotient_Filter<int> qf(8, 8);

  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(qf.remove(i)) << "Failed to remove i=" << i;

  EXPECT_EQ(qf.size(), 0u);
  EXPECT_TRUE(qf.is_empty());
}

TEST(CompactQuotientFilterDelete, insert_after_remove)
{
  Compact_Quotient_Filter<int> qf(8, 8);

  qf.insert(5);
  qf.remove(5);
  EXPECT_FALSE(qf.contains(5));

  qf.insert(5);
  EXPECT_TRUE(qf.contains(5));
  EXPECT_EQ(qf.size(), 1u);
}

TEST(CompactQuotientFilterDelete, interleaved_insert_remove)
{
  Compact_Quotient_Filter<int> qf(10, 8);

  for (int i = 0; i < 100; ++i)
    qf.insert(i);

  // Remove every other element
  for (int i = 0; i < 100; i += 2)
    EXPECT_TRUE(qf.remove(i));

  EXPECT_EQ(qf.size(), 50u);

  // Verify odd elements remain
  for (int i = 1; i < 100; i += 2)
    EXPECT_TRUE(qf.contains(i)) << "Lost i=" << i;

  // Verify even elements removed
  for (int i = 0; i < 100; i += 2)
    EXPECT_FALSE(qf.contains(i)) << "Still has i=" << i;
}

// =====================================================================
// Capacity and overflow
// =====================================================================

TEST(CompactQuotientFilterCapacity, load_factor_computed_correctly)
{
  Compact_Quotient_Filter<int> qf(4, 8);  // 16 slots
  EXPECT_DOUBLE_EQ(qf.load_factor(), 0.0);

  for (int i = 0; i < 8; ++i)
    qf.insert(i);

  EXPECT_DOUBLE_EQ(qf.load_factor(), 0.5);
}

TEST(CompactQuotientFilterCapacity, overflow_throws)
{
  Compact_Quotient_Filter<int> qf(2, 8);  // 4 slots

  for (int i = 0; i < 4; ++i)
    qf.insert(i * 100);

  bool threw = false;
  for (int i = 100; i < 200; ++i)
    {
      try
        {
          qf.insert(i);
        }
      catch (const std::overflow_error &)
        {
          threw = true;
          break;
        }
    }
  EXPECT_TRUE(threw);
}

TEST(CompactQuotientFilterCapacity, high_load_factor_still_works)
{
  Compact_Quotient_Filter<int> qf(8, 8);  // 256 slots

  // Fill to 90% capacity
  for (int i = 0; i < 230; ++i)
    qf.insert(i);

  EXPECT_GT(qf.load_factor(), 0.85);

  // All elements should still be found
  for (int i = 0; i < 230; ++i)
    EXPECT_TRUE(qf.contains(i)) << "Lost i=" << i;
}

// =====================================================================
// Introspection
// =====================================================================

TEST(CompactQuotientFilterIntrospection, memory_usage)
{
  Compact_Quotient_Filter<int> qf(10, 8);
  size_t expected = (1024 * 11 + 7) / 8;  // 1024 slots * 11 bits/slot
  EXPECT_EQ(qf.memory_usage(), expected);
}

TEST(CompactQuotientFilterIntrospection, false_positive_rate)
{
  Compact_Quotient_Filter<int> qf(10, 8);
  double expected = 1.0 / 256.0;  // 2^{-8}
  EXPECT_DOUBLE_EQ(qf.false_positive_rate(), expected);
}

TEST(CompactQuotientFilterIntrospection, estimate_returns_valid_params)
{
  auto [q_val, r_val] = Compact_Quotient_Filter<int>::estimate(10000, 0.001);
  EXPECT_GE(q_val, 1);
  EXPECT_GE(r_val, 10);  // -log2(0.001) ≈ 10
  EXPECT_LE(q_val + r_val, 64);
}

// =====================================================================
// Copy / Move semantics
// =====================================================================

TEST(CompactQuotientFilterSemantics, copy_preserves_state)
{
  Compact_Quotient_Filter<int> qf(8, 8);
  for (int i = 0; i < 30; ++i)
    qf.insert(i);

  Compact_Quotient_Filter<int> copy(qf);
  EXPECT_EQ(copy.size(), qf.size());
  EXPECT_EQ(copy.q(), qf.q());
  EXPECT_EQ(copy.r(), qf.r());

  for (int i = 0; i < 30; ++i)
    EXPECT_TRUE(copy.contains(i));
}

TEST(CompactQuotientFilterSemantics, move_transfers_state)
{
  Compact_Quotient_Filter<int> qf(8, 8);
  for (int i = 0; i < 30; ++i)
    qf.insert(i);

  size_t orig_size = qf.size();
  Compact_Quotient_Filter<int> moved(std::move(qf));

  EXPECT_EQ(moved.size(), orig_size);
  for (int i = 0; i < 30; ++i)
    EXPECT_TRUE(moved.contains(i));
}

TEST(CompactQuotientFilterSemantics, swap_works)
{
  Compact_Quotient_Filter<int> a(8, 8);
  Compact_Quotient_Filter<int> b(8, 8);

  a.insert(1);
  b.insert(2);

  a.swap(b);
  EXPECT_TRUE(a.contains(2));
  EXPECT_TRUE(b.contains(1));
}

// =====================================================================
// Clear
// =====================================================================

TEST(CompactQuotientFilterClear, clears_all_state)
{
  Compact_Quotient_Filter<int> qf(8, 8);
  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  qf.clear();
  EXPECT_EQ(qf.size(), 0u);
  EXPECT_TRUE(qf.is_empty());

  for (int i = 0; i < 50; ++i)
    EXPECT_FALSE(qf.contains(i));
}

TEST(CompactQuotientFilterClear, can_reuse_after_clear)
{
  Compact_Quotient_Filter<int> qf(8, 8);

  for (int i = 0; i < 20; ++i)
    qf.insert(i);

  qf.clear();

  for (int i = 100; i < 120; ++i)
    qf.insert(i);

  EXPECT_EQ(qf.size(), 20u);
  for (int i = 100; i < 120; ++i)
    EXPECT_TRUE(qf.contains(i));
}

// =====================================================================
// Merge
// =====================================================================

TEST(CompactQuotientFilterMerge, merge_combines_elements)
{
  Compact_Quotient_Filter<int> a(10, 8, 42);
  Compact_Quotient_Filter<int> b(10, 8, 42);

  for (int i = 0; i < 50; ++i)
    a.insert(i);
  for (int i = 50; i < 100; ++i)
    b.insert(i);

  a.merge(b);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(a.contains(i)) << "Missing i=" << i;
}

TEST(CompactQuotientFilterMerge, merge_rejects_mismatched_params)
{
  Compact_Quotient_Filter<int> a(10, 8, 42);
  Compact_Quotient_Filter<int> b(10, 9, 42);  // different r

  EXPECT_THROW(a.merge(b), std::domain_error);
}

TEST(CompactQuotientFilterMerge, merge_rejects_mismatched_seed)
{
  Compact_Quotient_Filter<int> a(10, 8, 42);
  Compact_Quotient_Filter<int> b(10, 8, 99);  // different seed

  EXPECT_THROW(a.merge(b), std::domain_error);
}

TEST(CompactQuotientFilterMerge, merge_with_overlapping_elements)
{
  Compact_Quotient_Filter<int> a(10, 8, 42);
  Compact_Quotient_Filter<int> b(10, 8, 42);

  for (int i = 0; i < 60; ++i)
    a.insert(i);
  for (int i = 40; i < 100; ++i)
    b.insert(i);

  a.merge(b);

  // Should have union: [0, 100)
  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(a.contains(i)) << "Missing i=" << i;
}

// =====================================================================
// False positive rate (statistical)
// =====================================================================

TEST(CompactQuotientFilterFPRate, empirical_fp_rate_within_bounds)
{
  const size_t n = 500;
  Compact_Quotient_Filter<int> qf(12, 8);  // 4096 slots, FP ≈ 1/256

  for (size_t i = 0; i < n; ++i)
    qf.insert(static_cast<int>(i));

  size_t test_count = 10000;
  size_t fps = 0;
  for (size_t i = n + 10000; i < n + 10000 + test_count; ++i)
    if (qf.contains(static_cast<int>(i)))
      ++fps;

  double empirical = static_cast<double>(fps) / test_count;
  double theoretical = qf.false_positive_rate();

  // Allow 5x margin over theoretical for statistical variance.
  EXPECT_LT(empirical, theoretical * 5.0 + 0.01)
    << "Empirical FP rate " << empirical << " exceeds 5x theoretical " << theoretical;
}

TEST(CompactQuotientFilterFPRate, low_fp_rate_with_large_r)
{
  Compact_Quotient_Filter<int> qf(10, 16);  // r=16 → FP ≈ 1/65536

  for (int i = 0; i < 200; ++i)
    qf.insert(i);

  size_t fps = 0;
  for (int i = 10000; i < 20000; ++i)
    if (qf.contains(i))
      ++fps;

  // With 10000 tests and FP rate ~0.0015%, expect < 5 false positives
  EXPECT_LT(fps, 10u);
}

// =====================================================================
// Stress tests
// =====================================================================

TEST(CompactQuotientFilterStress, random_insertions_and_lookups)
{
  Compact_Quotient_Filter<int> qf(14, 10);  // 16K slots
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 1000000);

  std::set<int> inserted;

  // Insert 5000 random elements
  for (int i = 0; i < 5000; ++i)
    {
      int val = dist(rng);
      qf.insert(val);
      inserted.insert(val);
    }

  // All inserted elements must be found
  for (int val : inserted)
    EXPECT_TRUE(qf.contains(val)) << "Lost value " << val;
}

TEST(CompactQuotientFilterStress, sequential_insert_remove_cycles)
{
  Compact_Quotient_Filter<int> qf(10, 8);  // 1024 slots

  for (int cycle = 0; cycle < 5; ++cycle)
    {
      qf.clear();  // Start fresh each cycle

      // Insert 200 elements
      for (int i = 0; i < 200; ++i)
        qf.insert(cycle * 1000 + i);

      // Remove half
      for (int i = 0; i < 100; ++i)
        qf.remove(cycle * 1000 + i);

      // Verify remaining half
      for (int i = 100; i < 200; ++i)
        EXPECT_TRUE(qf.contains(cycle * 1000 + i)) << "Cycle " << cycle << ", lost i=" << i;
    }
}

TEST(CompactQuotientFilterStress, collision_heavy_workload)
{
  // Small q forces many collisions
  Compact_Quotient_Filter<int> qf(6, 12);  // 64 slots, 15 bits/slot

  for (int i = 0; i < 40; ++i)
    qf.insert(i);

  for (int i = 0; i < 40; ++i)
    EXPECT_TRUE(qf.contains(i)) << "Lost i=" << i;

  // Remove some
  for (int i = 0; i < 20; ++i)
    EXPECT_TRUE(qf.remove(i));

  // Verify remaining
  for (int i = 20; i < 40; ++i)
    EXPECT_TRUE(qf.contains(i));
}

// =====================================================================
// Edge cases
// =====================================================================

TEST(CompactQuotientFilterEdge, single_slot_filter)
{
  Compact_Quotient_Filter<int> qf(1, 8);  // 2 slots

  qf.insert(1);
  EXPECT_TRUE(qf.contains(1));

  qf.insert(2);
  EXPECT_TRUE(qf.contains(1));
  EXPECT_TRUE(qf.contains(2));
}

TEST(CompactQuotientFilterEdge, minimal_remainder_bits)
{
  Compact_Quotient_Filter<int> qf(8, 1);  // r=1 → 50% FP rate

  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(qf.contains(i));
}

TEST(CompactQuotientFilterEdge, maximal_remainder_bits)
{
  Compact_Quotient_Filter<int> qf(4, 60);  // r=60 → extremely low FP

  qf.insert(42);
  EXPECT_TRUE(qf.contains(42));
  EXPECT_FALSE(qf.contains(43));
}

TEST(CompactQuotientFilterEdge, wrap_around_behavior)
{
  Compact_Quotient_Filter<int> qf(4, 8);  // 16 slots

  // Force wrap-around by filling near the end
  for (int i = 0; i < 16; ++i)
    qf.insert(i);

  for (int i = 0; i < 16; ++i)
    EXPECT_TRUE(qf.contains(i)) << "Lost i=" << i;
}

// =====================================================================
// Comparison with standard Quotient_Filter
// =====================================================================

TEST(CompactQuotientFilterComparison, same_behavior_as_standard)
{
  const uint32_t seed = 12345;
  Quotient_Filter<int> standard(10, 8, seed);
  Compact_Quotient_Filter<int> compact(10, 8, seed);

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

TEST(CompactQuotientFilterComparison, memory_savings_verified)
{
  Quotient_Filter<int> standard(10, 8);
  Compact_Quotient_Filter<int> compact(10, 8);

  size_t standard_mem = standard.memory_usage();
  size_t compact_mem = compact.memory_usage();

  // Compact should use ~82% less memory
  EXPECT_LT(compact_mem, standard_mem / 5);  // < 20% of standard

  // Standard: 1024 slots * 8 bytes = 8192 bytes
  EXPECT_EQ(standard_mem, 8192u);

  // Compact: 1024 slots * 11 bits = 11264 bits = 1408 bytes
  EXPECT_EQ(compact_mem, 1408u);
}
