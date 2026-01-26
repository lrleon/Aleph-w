
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


/**
 * @file bloom-filter.cc
 * @brief Tests for Bloom Filter
 */

#include <gtest/gtest.h>

#include <bloom-filter.H>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

using namespace Aleph;
using namespace std;

namespace {

size_t identity_hash(const int &item, unsigned long) noexcept
{
  return static_cast<size_t>(item);
}

size_t salted_identity_hash(const int &item, unsigned long seed) noexcept
{
  return static_cast<size_t>(item) + static_cast<size_t>(seed);
}

template <typename T>
vector<T> to_vector(const DynList<T> &list)
{
  vector<T> out;
  DynList<T> tmp = list;
  tmp.for_each([&out](auto &value) { out.push_back(value); });
  return out;
}

} // namespace

TEST(BloomFilterCtor, rejects_invalid_arguments)
{
  EXPECT_THROW((Bloom_Filter<int>(0, 1, identity_hash, 0)), std::invalid_argument);
  EXPECT_THROW((Bloom_Filter<int>(8, 0, identity_hash, 0)), std::invalid_argument);
  EXPECT_THROW((Bloom_Filter<int>(8, 1, nullptr, 0)), std::invalid_argument);
}

TEST(BloomFilterBasics, insert_contains_no_false_negatives)
{
  Bloom_Filter<int> f(128, 3, salted_identity_hash, 0);

  for (int i = 0; i < 50; ++i)
    {
      f.insert(i);
      EXPECT_TRUE(f.contains(i));
    }
}

TEST(BloomFilterBasics, deterministic_no_false_positives_when_k1_identity_and_sparse)
{
  // Deterministic mapping: with k=1 and identity_hash ignoring seed, the bit
  // index is item % m.
  Bloom_Filter<int> f(128, 1, identity_hash, 123);

  f.insert(1);
  f.insert(2);
  f.insert(3);

  EXPECT_TRUE(f.contains(1));
  EXPECT_TRUE(f.contains(2));
  EXPECT_TRUE(f.contains(3));

  // These map to distinct bits not set above.
  EXPECT_FALSE(f.contains(10));
  EXPECT_FALSE(f.contains(64));
  EXPECT_FALSE(f.contains(127));
}

TEST(BloomFilterIntrospection, hash_seeds_and_hashes_sizes)
{
  Bloom_Filter<int> f(256, 5, salted_identity_hash, 42);

  const auto seeds = to_vector(f.hash_seeds());
  EXPECT_EQ(seeds.size(), 5u);

  const auto hashes = to_vector(f.hashes(123));
  EXPECT_EQ(hashes.size(), 5u);
  for (const auto h : hashes)
    EXPECT_LT(h, f.get_m());
}

TEST(BloomFilterIntrospection, set_bits_matches_inserted_for_k1_identity)
{
  Bloom_Filter<int> f(64, 1, identity_hash, 999);

  f.insert(0);
  f.insert(3);
  f.insert(63);

  const auto bits = to_vector(f.set_bits());
  EXPECT_EQ(bits.size(), 3u);

  EXPECT_NE(find(bits.begin(), bits.end(), 0u), bits.end());
  EXPECT_NE(find(bits.begin(), bits.end(), 3u), bits.end());
  EXPECT_NE(find(bits.begin(), bits.end(), 63u), bits.end());
}

TEST(BloomFilterSemantics, copy_and_move_preserve_behavior)
{
  Bloom_Filter<int> f(128, 2, salted_identity_hash, 7);
  for (int i = 0; i < 20; ++i)
    f.insert(i);

  Bloom_Filter<int> copy(f);
  EXPECT_EQ(copy.get_m(), f.get_m());
  EXPECT_EQ(copy.get_k(), f.get_k());
  EXPECT_EQ(copy.get_n(), f.get_n());
  for (int i = 0; i < 20; ++i)
    EXPECT_TRUE(copy.contains(i));

  Bloom_Filter<int> moved(std::move(copy));
  for (int i = 0; i < 20; ++i)
    EXPECT_TRUE(moved.contains(i));
}

TEST(BloomFilterSetOps, union_and_intersection_require_compatible_seeds)
{
  // Create a base filter and copy it to guarantee compatibility (same seeds).
  Bloom_Filter<int> base(128, 2, salted_identity_hash, 101);
  Bloom_Filter<int> a(base);
  Bloom_Filter<int> b(base);

  a.insert(1);
  a.insert(2);
  b.insert(2);
  b.insert(3);

  Bloom_Filter<int> u(a);
  u |= b;
  EXPECT_TRUE(u.contains(1));
  EXPECT_TRUE(u.contains(2));
  EXPECT_TRUE(u.contains(3));

  Bloom_Filter<int> inter(a);
  inter &= b;
  EXPECT_TRUE(inter.contains(2));

  // With a Bloom filter, negative assertions are probabilistic; we only
  // validate that the operation succeeds and keeps known positives.
}

TEST(BloomFilterSetOps, mismatch_throws_domain_error_and_keeps_message)
{
  Bloom_Filter<int> a(128, 2, salted_identity_hash, 1);
  Bloom_Filter<int> b(128, 2, salted_identity_hash, 2); // different internal seeds

  try
    {
      a |= b;
      FAIL() << "expected std::domain_error";
    }
  catch (const std::domain_error &e)
    {
      const string msg = e.what();
      EXPECT_NE(msg.find("Bloom filter have different hashes"), string::npos);
    }
}

TEST(BloomFilterMath, expected_size_is_reasonable_for_sparse_k1_identity)
{
  Bloom_Filter<int> f(1024, 1, identity_hash, 0);
  for (int i = 0; i < 50; ++i)
    f.insert(i);

  const auto x = f.get_x();
  EXPECT_GE(x, 1u);
  const auto est = f.expected_size(x);

  // For sparse occupancy with k=1, this estimate should be close to x.
  EXPECT_GE(est, 45u);
  EXPECT_LE(est, 60u);
}
