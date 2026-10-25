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
 * @file hash_allocation_failure_test.cc
 * @brief Removals from hash tables when memory runs out.
 *
 * This program replaces the global operator new, so that a test can make
 * every allocation fail for a while. A removal that shrinks a table which
 * resizes itself allocates the smaller table; without memory, the removal
 * must stand and the table must keep its capacity, without throwing.
 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <new>
#include <vector>

#include <tpl_lhash.H>
#include <tpl_linHash.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>

using namespace Aleph;

// ThreadSanitizer has its own global operator new and delete, which collide
// with the ones below at link time; these tests are skipped under it.
#if defined(__SANITIZE_THREAD__)
#  define ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN 1
#  endif
#endif
#ifndef ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN
#  define ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN 0
#endif

namespace
{
  bool fail_allocations = false; // while true, operator new throws
  size_t refused_allocations = 0;
  size_t allocations = 0;

  // While it lives, every allocation through operator new fails
  struct No_Memory
  {
    No_Memory() noexcept
    {
      refused_allocations = 0;
      fail_allocations = true;
    }

    ~No_Memory() { fail_allocations = false; }
  };

#if !ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN
  void * allocate(const std::size_t size)
  {
    ++allocations;
    if (fail_allocations)
      {
        ++refused_allocations;
        throw std::bad_alloc();
      }
    if (void * p = std::malloc(size == 0 ? 1 : size))
      return p;
    throw std::bad_alloc();
  }
#endif
}

#if !ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN

void * operator new(const std::size_t size)
{
  return allocate(size);
}

void * operator new[](const std::size_t size)
{
  return allocate(size);
}

// GoogleTest allocates with these too; under AddressSanitizer, its own
// versions would give the operator delete below memory it did not take
void * operator new(const std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void * operator new[](const std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

// They free what operator new took from malloc; GCC does not see that
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

void operator delete(void * p) noexcept
{
  std::free(p);
}

void operator delete[](void * p) noexcept
{
  std::free(p);
}

void operator delete(void * p, std::size_t) noexcept
{
  std::free(p);
}

void operator delete[](void * p, std::size_t) noexcept
{
  std::free(p);
}

void operator delete(void * p, const std::nothrow_t &) noexcept
{
  std::free(p);
}

void operator delete[](void * p, const std::nothrow_t &) noexcept
{
  std::free(p);
}

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#endif // !ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN

// The tests need the operator new above
struct HashTableWithoutMemory : public ::testing::Test
{
  void SetUp() override
  {
    if (ALEPH_HASH_ALLOCATION_TEST_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

namespace
{
  // Number of non-empty lists of a chained or linear table, counted
  template <class Table> size_t busy_lists(const Table & table)
  {
    const auto stats = table.stats();
    return table.capacity() - (stats.lens.size() > 0 ? stats.lens(0) : 0);
  }
}

// remove() of a chained table is noexcept; when it shrank the table and
// there was no memory for the smaller one, it ended the program.
TEST_F(HashTableWithoutMemory, ChainedRemovalKeepsTheCapacity)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> table(101, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                        hash_default_lower_alpha, hash_default_upper_alpha,
                        true, true);
  std::vector<Bucket *> buckets;
  for (int k = 0; k < 1000; ++k)
    {
      buckets.push_back(new Bucket(k));
      ASSERT_NE(table.insert(buckets.back()), nullptr) << "key " << k;
    }
  const size_t grown = table.capacity();

  size_t refused = 0;
  {
    No_Memory no_memory;
    for (int k = 0; k < 990; ++k)
      delete table.remove(buckets[k]);
    refused = refused_allocations;
  }

  EXPECT_GT(refused, 0u); // it tried to shrink
  EXPECT_EQ(table.capacity(), grown);
  EXPECT_EQ(table.size(), 10u);
  EXPECT_EQ(table.get_num_busy_slots(), busy_lists(table));
  for (int k = 990; k < 1000; ++k)
    EXPECT_EQ(table.search(k), buckets[k]) << "key " << k;

  // with memory again, the next removal shrinks it
  delete table.remove(buckets[990]);
  EXPECT_LT(table.capacity(), grown);
  for (int k = 991; k < 1000; ++k)
    {
      EXPECT_EQ(table.search(k), buckets[k]) << "key " << k;
      delete table.remove(buckets[k]);
    }
  EXPECT_TRUE(table.is_empty());
}

// next_prime() gives a prime of its table, and the one after 101 is 211:
// half of 211 is 211 again, so every removal from a table of 211 lists
// under the lower threshold rebuilt it as it was.
TEST_F(HashTableWithoutMemory, ChainedRemovalDoesNotRebuildToTheSameCapacity)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> table(211, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                        hash_default_lower_alpha, hash_default_upper_alpha,
                        true, true);
  ASSERT_EQ(table.capacity(), 211u);
  std::vector<Bucket *> buckets;
  for (int k = 0; k < 20; ++k) // under 0.25 of 211
    {
      buckets.push_back(new Bucket(k));
      ASSERT_NE(table.insert(buckets.back()), nullptr) << "key " << k;
    }

  const size_t before = allocations;
  for (int k = 0; k < 10; ++k)
    delete table.remove(buckets[k]);
  EXPECT_EQ(allocations, before);
  EXPECT_EQ(table.capacity(), 211u);

  for (int k = 10; k < 20; ++k)
    delete table.remove(buckets[k]);
}

// With a lower threshold over half the upper one, half the capacity may
// leave the keys over the upper threshold. Such a removal shrank the
// table, and resize(), which places the keys again with insert(), grew it
// back at once: two rebuilds for nothing.
TEST_F(HashTableWithoutMemory, ChainedRemovalDoesNotShrinkWhereTheKeysWouldNotFit)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> table(47, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                        0.85f, 0.9f, true, true);
  ASSERT_EQ(table.capacity(), 47u);
  std::vector<Bucket *> buckets;
  for (int k = 0; k < 22; ++k)
    {
      buckets.push_back(new Bucket(k));
      ASSERT_NE(table.insert(buckets.back()), nullptr) << "key " << k;
    }

  const size_t before = allocations;
  delete table.remove(buckets[0]); // 21 keys in 23 lists: 0.91
  EXPECT_EQ(allocations, before);
  EXPECT_EQ(table.capacity(), 47u);

  delete table.remove(buckets[1]); // 20 keys in 23 lists: 0.87
  EXPECT_EQ(table.capacity(), 23u);

  for (int k = 2; k < 22; ++k)
    delete table.remove(buckets[k]);
}

template <class Table> struct OpenTableWithoutMemory : public HashTableWithoutMemory
{
  static Table make()
  {
    if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
      return Table(11, Aleph::dft_hash_ptr_fct<long>,
                   Aleph::snd_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                   hash_default_lower_alpha, hash_default_upper_alpha, true);
    else
      return Table(11, Aleph::dft_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                   hash_default_lower_alpha, hash_default_upper_alpha, true);
  }
};

using Open_Tables = ::testing::Types<OLhashTable<long>, ODhashTable<long>>;
TYPED_TEST_SUITE(OpenTableWithoutMemory, Open_Tables);

// remove_ptr() threw std::bad_alloc after it had removed the key.
TYPED_TEST(OpenTableWithoutMemory, RemovalsKeepTheCapacity)
{
  TypeParam table = this->make();
  for (long k = 0; k < 1000; ++k)
    ASSERT_NE(table.insert(k), nullptr) << "key " << k;
  const size_t grown = table.capacity();

  size_t refused = 0, threw = 0;
  {
    No_Memory no_memory;
    for (long k = 0; k < 990; ++k)
      try
        {
          if (k % 2 == 0)
            table.remove(k);
          else
            table.remove_ptr(table.search(k));
        }
      catch (...)
        {
          ++threw;
        }
    refused = refused_allocations;
  }

  EXPECT_EQ(threw, 0u);
  EXPECT_GT(refused, 0u);
  EXPECT_EQ(table.capacity(), grown);
  EXPECT_EQ(table.size(), 10u);
  for (long k = 0; k < 1000; ++k)
    EXPECT_EQ(table.contains(k), k >= 990) << "key " << k;

  table.remove(990L);
  EXPECT_LT(table.capacity(), grown);
  for (long k = 991; k < 1000; ++k)
    EXPECT_TRUE(table.contains(k)) << "key " << k;
}

namespace
{
  size_t identity_hash(const size_t & k) noexcept
  {
    return k;
  }
}

// When linear hashing contracts, it merges its last list into the list
// that it was split from. In a lazily allocated table that list may never
// have been written, and contract() allocates it. contract() was noexcept
// and changed the table before allocating.
//
// The table starts with 8193 lists. Its keys never touch the block of
// lists [4096, 8192) of its array (blocks have 4096 lists): their values
// modulo 8193 are under 4096. Once list 4096 has been split, key 12289
// goes straight to list 12289 = 4096 + 8193, so contracting into list
// 4096 has to allocate that block.
TEST_F(HashTableWithoutMemory, LinearContractionKeepsTheTable)
{
  using Table = LinearHashTable<size_t>;
  using Bucket = Table::Bucket;
  constexpr size_t len = 8193;
  Table table(len, identity_hash, Aleph::equal_to<size_t>(),
              hash_default_lower_alpha, hash_default_upper_alpha, true, true);

  std::vector<Bucket *> fillers;
  for (size_t t = 0; table.capacity() <= len + 4097; ++t)
    for (size_t r = 0; r < 4096 and table.capacity() <= len + 4097; ++r)
      {
        fillers.push_back(new Bucket(r + len * t));
        ASSERT_NE(table.insert(fillers.back()), nullptr);
      }
  auto * special = new Bucket(4096 + len);
  ASSERT_NE(table.insert(special), nullptr);
  const size_t expanded = table.capacity();
  ASSERT_GT(expanded, len + 4097);

  size_t refused = 0, removed = 0;
  {
    No_Memory no_memory;
    for (; removed < fillers.size() - 100; ++removed)
      delete table.remove(fillers[removed]);
    refused = refused_allocations;
  }

  EXPECT_GT(refused, 0u); // it had to allocate the block of list 4096
  EXPECT_GE(table.capacity(), len + 4097); // it stopped before that list
  EXPECT_EQ(table.size(), 101u);
  EXPECT_EQ(table.busy_slots(), busy_lists(table));
  EXPECT_EQ(table.search(4096 + len), special);
  for (size_t i = removed; i < fillers.size(); ++i)
    EXPECT_EQ(table.search(fillers[i]->get_key()), fillers[i]) << i;

  // with memory again, the next removal goes on contracting
  delete table.remove(fillers[removed++]);
  EXPECT_LT(table.capacity(), len + 4097);
  EXPECT_EQ(table.busy_slots(), busy_lists(table));
  EXPECT_EQ(table.search(4096 + len), special);
  for (size_t i = removed; i < fillers.size(); ++i)
    EXPECT_EQ(table.search(fillers[i]->get_key()), fillers[i]) << i;
}
