
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

/** @file quotient-filter.cc
 *  @brief Tests for Quotient_Filter
 */

#include <gtest/gtest.h>

#include <quotient-filter.H>

#include <cstddef>
#include <string>
#include <set>
#include <random>

using namespace Aleph;
using namespace std;

// =====================================================================
// Construction
// =====================================================================

TEST(QuotientFilterCtor, valid_parameters)
{
  Quotient_Filter<int> qf(4, 8);
  EXPECT_EQ(qf.q(), 4);
  EXPECT_EQ(qf.r(), 8);
  EXPECT_EQ(qf.capacity(), 16u);
  EXPECT_EQ(qf.size(), 0u);
  EXPECT_TRUE(qf.is_empty());
}

TEST(QuotientFilterCtor, rejects_invalid_q)
{
  EXPECT_THROW((Quotient_Filter<int>(0, 8)), std::domain_error);
  EXPECT_THROW((Quotient_Filter<int>(33, 8)), std::domain_error);
}

TEST(QuotientFilterCtor, rejects_invalid_r)
{
  EXPECT_THROW((Quotient_Filter<int>(4, 0)), std::domain_error);
  EXPECT_THROW((Quotient_Filter<int>(4, 61)), std::domain_error);
}

TEST(QuotientFilterCtor, rejects_q_plus_r_exceeds_64)
{
  EXPECT_THROW((Quotient_Filter<int>(32, 33)), std::domain_error);
}

TEST(QuotientFilterCtor, from_capacity_creates_valid_filter)
{
  auto qf = Quotient_Filter<int>::from_capacity(1000, 0.01);
  EXPECT_GE(qf.capacity(), 1000u);
  EXPECT_LE(qf.false_positive_rate(), 0.02);
}

TEST(QuotientFilterCtor, from_capacity_rejects_bad_args)
{
  EXPECT_THROW(Quotient_Filter<int>::from_capacity(0, 0.01),
               std::domain_error);
  EXPECT_THROW(Quotient_Filter<int>::from_capacity(100, 0.0),
               std::domain_error);
  EXPECT_THROW(Quotient_Filter<int>::from_capacity(100, 1.0),
               std::domain_error);
}

// =====================================================================
// Basic insert / contains
// =====================================================================

TEST(QuotientFilterBasic, insert_and_contains_no_false_negatives)
{
  Quotient_Filter<int> qf(10, 8);

  for (int i = 0; i < 100; ++i)
    qf.insert(i);

  EXPECT_EQ(qf.size(), 100u);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(qf.contains(i)) << "False negative for i=" << i;
}

TEST(QuotientFilterBasic, absent_elements_usually_not_found)
{
  Quotient_Filter<int> qf(12, 10);

  for (int i = 0; i < 100; ++i)
    qf.insert(i);

  size_t false_positives = 0;
  for (int i = 1000; i < 2000; ++i)
    if (qf.contains(i))
      ++false_positives;

  // With r=10, FP rate ≈ 1/1024 ≈ 0.1%.
  // Testing 1000 elements: expect < 10 false positives.
  EXPECT_LT(false_positives, 20u)
    << "Too many false positives: " << false_positives;
}

TEST(QuotientFilterBasic, duplicate_insert_is_idempotent)
{
  Quotient_Filter<int> qf(8, 8);

  qf.insert(42);
  EXPECT_EQ(qf.size(), 1u);

  qf.insert(42);
  EXPECT_EQ(qf.size(), 1u);

  EXPECT_TRUE(qf.contains(42));
}

TEST(QuotientFilterBasic, string_keys)
{
  Quotient_Filter<string> qf(10, 8);

  qf.insert("hello");
  qf.insert("world");
  qf.insert("aleph");

  EXPECT_TRUE(qf.contains("hello"));
  EXPECT_TRUE(qf.contains("world"));
  EXPECT_TRUE(qf.contains("aleph"));

  EXPECT_EQ(qf.size(), 3u);
}

// =====================================================================
// Deletion
// =====================================================================

TEST(QuotientFilterDelete, remove_existing_element)
{
  Quotient_Filter<int> qf(8, 8);

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

TEST(QuotientFilterDelete, remove_nonexistent_is_noop)
{
  Quotient_Filter<int> qf(8, 8);

  qf.insert(1);
  EXPECT_FALSE(qf.remove(999));
  EXPECT_EQ(qf.size(), 1u);
  EXPECT_TRUE(qf.contains(1));
}

TEST(QuotientFilterDelete, remove_all_makes_empty)
{
  Quotient_Filter<int> qf(8, 8);

  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  for (int i = 0; i < 50; ++i)
    EXPECT_TRUE(qf.remove(i)) << "Failed to remove i=" << i;

  EXPECT_EQ(qf.size(), 0u);
  EXPECT_TRUE(qf.is_empty());
}

TEST(QuotientFilterDelete, insert_after_remove)
{
  Quotient_Filter<int> qf(8, 8);

  qf.insert(5);
  qf.remove(5);
  EXPECT_FALSE(qf.contains(5));

  qf.insert(5);
  EXPECT_TRUE(qf.contains(5));
  EXPECT_EQ(qf.size(), 1u);
}

// =====================================================================
// Capacity and overflow
// =====================================================================

TEST(QuotientFilterCapacity, load_factor_computed_correctly)
{
  Quotient_Filter<int> qf(4, 8); // 16 slots
  EXPECT_DOUBLE_EQ(qf.load_factor(), 0.0);

  for (int i = 0; i < 8; ++i)
    qf.insert(i);

  EXPECT_DOUBLE_EQ(qf.load_factor(), 0.5);
}

TEST(QuotientFilterCapacity, overflow_throws)
{
  Quotient_Filter<int> qf(2, 8); // 4 slots

  for (int i = 0; i < 4; ++i)
    qf.insert(i * 100); // large spread to avoid collisions

  // Filter is full (or nearly). Inserting more should throw.
  // Some inserts might succeed due to fingerprint collisions (duplicates),
  // so we try enough to guarantee overflow.
  bool threw = false;
  for (int i = 100; i < 200; ++i)
    {
      try { qf.insert(i); }
      catch (const std::overflow_error &) { threw = true; break; }
    }
  EXPECT_TRUE(threw);
}

// =====================================================================
// Introspection
// =====================================================================

TEST(QuotientFilterIntrospection, memory_usage)
{
  Quotient_Filter<int> qf(10, 8);
  EXPECT_EQ(qf.memory_usage(), 1024u * sizeof(uint64_t));
}

TEST(QuotientFilterIntrospection, false_positive_rate)
{
  Quotient_Filter<int> qf(10, 8);
  double expected = 1.0 / 256.0; // 2^{-8}
  EXPECT_DOUBLE_EQ(qf.false_positive_rate(), expected);
}

TEST(QuotientFilterIntrospection, estimate_returns_valid_params)
{
  auto [q_val, r_val] = Quotient_Filter<int>::estimate(10000, 0.001);
  EXPECT_GE(q_val, 1);
  EXPECT_GE(r_val, 10);  // -log2(0.001) ≈ 10
  EXPECT_LE(q_val + r_val, 64);
}

// =====================================================================
// Copy / Move semantics
// =====================================================================

TEST(QuotientFilterSemantics, copy_preserves_state)
{
  Quotient_Filter<int> qf(8, 8);
  for (int i = 0; i < 30; ++i)
    qf.insert(i);

  Quotient_Filter<int> copy(qf);
  EXPECT_EQ(copy.size(), qf.size());
  EXPECT_EQ(copy.q(), qf.q());
  EXPECT_EQ(copy.r(), qf.r());

  for (int i = 0; i < 30; ++i)
    EXPECT_TRUE(copy.contains(i));
}

TEST(QuotientFilterSemantics, move_transfers_state)
{
  Quotient_Filter<int> qf(8, 8);
  for (int i = 0; i < 30; ++i)
    qf.insert(i);

  size_t orig_size = qf.size();
  Quotient_Filter<int> moved(std::move(qf));

  EXPECT_EQ(moved.size(), orig_size);
  for (int i = 0; i < 30; ++i)
    EXPECT_TRUE(moved.contains(i));
}

TEST(QuotientFilterSemantics, swap_works)
{
  Quotient_Filter<int> a(8, 8);
  Quotient_Filter<int> b(8, 8);

  a.insert(1);
  b.insert(2);

  a.swap(b);
  EXPECT_TRUE(a.contains(2));
  EXPECT_TRUE(b.contains(1));
}

// =====================================================================
// Clear
// =====================================================================

TEST(QuotientFilterClear, clears_all_state)
{
  Quotient_Filter<int> qf(8, 8);
  for (int i = 0; i < 50; ++i)
    qf.insert(i);

  qf.clear();
  EXPECT_EQ(qf.size(), 0u);
  EXPECT_TRUE(qf.is_empty());

  for (int i = 0; i < 50; ++i)
    EXPECT_FALSE(qf.contains(i));
}

// =====================================================================
// Merge
// =====================================================================

TEST(QuotientFilterMerge, merge_combines_elements)
{
  Quotient_Filter<int> a(10, 8, 42);
  Quotient_Filter<int> b(10, 8, 42);

  for (int i = 0; i < 50; ++i)
    a.insert(i);
  for (int i = 50; i < 100; ++i)
    b.insert(i);

  a.merge(b);

  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(a.contains(i)) << "Missing i=" << i;
}

TEST(QuotientFilterMerge, merge_rejects_mismatched_params)
{
  Quotient_Filter<int> a(10, 8, 42);
  Quotient_Filter<int> b(10, 9, 42); // different r

  EXPECT_THROW(a.merge(b), std::domain_error);
}

TEST(QuotientFilterMerge, merge_rejects_mismatched_seed)
{
  Quotient_Filter<int> a(10, 8, 42);
  Quotient_Filter<int> b(10, 8, 99); // different seed

  EXPECT_THROW(a.merge(b), std::domain_error);
}

// =====================================================================
// False positive rate (statistical)
// =====================================================================

TEST(QuotientFilterFPRate, empirical_fp_rate_within_bounds)
{
  const size_t n = 500;
  Quotient_Filter<int> qf(12, 8); // 4096 slots, FP ≈ 1/256

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
    << "Empirical FP rate " << empirical
    << " exceeds 5x theoretical " << theoretical;
}