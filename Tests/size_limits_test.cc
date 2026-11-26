
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
 * @file size_limits_test.cc
 * @brief Every container that takes a size refuses one that it cannot hold
 */

// BitArray::set_size() computed its bytes as (size + 7) / 8, which wraps for
// the last seven sizes of size_t, and ArrayHeap allocated an array of
// capacity + 1 entries, which wraps for the last one: each took a size that
// it could not hold, with less memory than it said. DynArray, whose
// constructor refuses a dimension past Max_Dim_Allowed, took it in reserve(),
// touch(), operator[] and adjust(). MemArray, ArrayHeap, FixedStack and
// FixedQueue left to new[] a count whose bytes do not fit in a size_t, for
// which Clang, and GCC with items that have a destructor, ask the allocator
// for SIZE_MAX bytes; and FixedQueue shifted 1 by 64 bits or more past 2^63.
// These tests give each container that takes a size the sizes that it cannot
// hold, those whose bytes do not fit in a size_t and, for the arrays of
// DynArray, those past its limit. Static_Event_Table, Simplex and the hash
// tables left such a count to new[] too, the tables after a search for the
// next prime, which past the table of primes is by trial division. Each one must refuse them before it
// allocates: with an exception that is not a failed allocation, since a
// request of that size never gets memory, and under ASan the attempt itself
// ends the program. And the container must be as it was.

#include <gtest/gtest.h>

#include <Simplex.H>
#include <bitArray.H>
#include <bloom-filter.H>
#include <compact-cuckoo-filter.H>
#include <cuckoo-filter.H>
#include <driven_table.H>
#include <tpl_array.H>
#include <tpl_arrayHeap.H>
#include <tpl_arrayQueue.H>
#include <tpl_arrayStack.H>
#include <tpl_dynArray.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynLhash.H>
#include <tpl_dynMapOhash.H>
#include <tpl_dynSetHash.H>
#include <tpl_flat_map.H>
#include <tpl_flat_set.H>
#include <tpl_lhash.H>
#include <tpl_memArray.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>
#include <tpl_small_vector.H>

#include <cstddef>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <typeinfo>

using namespace Aleph;

namespace
{
  constexpr size_t max_size = std::numeric_limits<size_t>::max();

  // The counts of items of `item_size` bytes, of two or more, that no
  // container can hold: their bytes do not fit in a size_t, near the
  // maximum and from the least count that wraps
  Array<size_t> sizes_beyond_bytes(const size_t item_size)
  {
    return {max_size, max_size - 1, max_size - 6, max_size - 7, max_size - 8,
            max_size / item_size + 1, max_size / 2 + 1};
  }

  // And those that a DynArray of items of `item_size` bytes cannot hold
  // either: past its limit, Max_Dim_Allowed entries
  Array<size_t> sizes_beyond_dynarray(const size_t item_size)
  {
    Array<size_t> sizes = sizes_beyond_bytes(item_size);
    const size_t limit = DynArray<int>::Max_Dim_Allowed;
    for (const size_t s : {limit + 1, limit * 2, size_t(1) << 61, size_t(1) << 62})
      sizes.append(s);
    return sizes;
  }

  // The numbers of bits that a BitArray cannot hold: those whose bytes are
  // more than a DynArray holds. It is not a test of 2^38 to 2^41 bits,
  // which a BitArray holds with up to 256 GiB
  Array<size_t> bits_beyond_bitarray()
  {
    const size_t limit = DynArray<int>::Max_Dim_Allowed;
    return {max_size, max_size - 1, max_size - 6, max_size - 7, max_size - 8, max_size / 2 + 1,
            limit * 8 + 1, limit * 8 + 8, limit * 16, size_t(1) << 61, size_t(1) << 62};
  }

  // What `request` did with a size that it cannot hold: "" if it threw an
  // exception that is not a failed allocation; otherwise what happened
  template <class Request>
  std::string refusal_of(Request request)
  {
    try
      {
        request();
      }
    catch (const std::bad_array_new_length &)
      {
        return ""; // new[] refuses the count before it allocates
      }
    catch (const std::bad_alloc &)
      {
        return "it tried to allocate";
      }
    catch (const std::exception &)
      {
        return "";
      }
    return "it took the size";
  }

  // MemArray and Array of `item`, refusing the counts of items that do not
  // fit: those of a class with a destructor too, whose new[] keeps the count
  // before the items
  template <class T>
  void expect_memarray_and_array_refuse(const T & item)
  {
    for (const size_t n : sizes_beyond_bytes(sizeof(T)))
      {
        SCOPED_TRACE(n);
        MemArray<T> m;
        m.append(item);
        const size_t m_cap = m.capacity();
        EXPECT_EQ(refusal_of([&] { m.reserve(n); }), "");
        ASSERT_EQ(m.size(), 1u);
        EXPECT_EQ(m.first(), item);
        EXPECT_EQ(m.capacity(), m_cap);
        EXPECT_EQ(refusal_of([&] { MemArray<T> b(n); }), "");

        Array<T> a;
        a.append(item);
        const size_t a_cap = a.capacity();
        EXPECT_EQ(refusal_of([&] { a.reserve(n); }), "");
        ASSERT_EQ(a.size(), 1u);
        EXPECT_EQ(a[0], item);
        EXPECT_EQ(a.capacity(), a_cap);
        EXPECT_EQ(refusal_of([&] { Array<T> b(n, item); }), "");
      }
  }

  // The queues and the stacks of arrays of `item`, refusing the counts of
  // items that do not fit
  template <class T>
  void expect_queues_and_stacks_refuse(const T & item)
  {
    for (const size_t n : sizes_beyond_bytes(sizeof(T)))
      {
        SCOPED_TRACE(n);
        ArrayQueue<T> q;
        q.put(item);
        EXPECT_EQ(refusal_of([&] { q.reserve(n); }), "");
        ASSERT_EQ(q.size(), 1u);
        EXPECT_EQ(q.front(), item);
        EXPECT_EQ(refusal_of([&] { ArrayQueue<T> b(n); }), "");
        EXPECT_EQ(refusal_of([&] { ArrayStack<T> s(n); }), "");
        EXPECT_EQ(refusal_of([&] { FixedQueue<T> f(n); }), "");
        EXPECT_EQ(refusal_of([&] { FixedStack<T> s(n); }), "");
      }
  }
}

TEST(SizeLimits, BitArray)
{
  for (const size_t bits : bits_beyond_bitarray())
    {
      SCOPED_TRACE(bits);
      BitArray a;
      a.set_bit_str("101");
      EXPECT_EQ(refusal_of([&] { a.set_size(bits); }), "");
      EXPECT_EQ(refusal_of([&] { a.reserve(bits); }), "");
      ASSERT_EQ(a.size(), 3u); // before the bits of an array that took the size
      EXPECT_EQ(a.get_bit_str(), "101");
      EXPECT_EQ(refusal_of([&] { BitArray b(bits); }), "");
    }
}

TEST(SizeLimits, DynArray)
{
  for (const size_t n : sizes_beyond_dynarray(sizeof(int)))
    {
      SCOPED_TRACE(n);
      DynArray<int> a;
      a.append(5);
      EXPECT_EQ(refusal_of([&] { a.reserve(n); }), "");
      EXPECT_EQ(refusal_of([&] { a.adjust(n); }), "");
      EXPECT_EQ(refusal_of([&] { (void) a.touch(n - 1); }), "");
      EXPECT_EQ(refusal_of([&] { a[n - 1] = 6; }), "");
      ASSERT_EQ(a.size(), 1u);
      EXPECT_EQ(a(0), 5);
      EXPECT_EQ(refusal_of([&] { DynArray<int> b(n); }), "");
    }
}

TEST(SizeLimits, DynArrayHeap)
{
  for (const size_t n : sizes_beyond_dynarray(sizeof(int)))
    {
      SCOPED_TRACE(n);
      DynArrayHeap<int> h;
      h.insert(5);
      EXPECT_EQ(refusal_of([&] { h.reserve(n); }), "");
      ASSERT_EQ(h.size(), 1u);
      EXPECT_EQ(h.top(), 5);
    }
}

TEST(SizeLimits, ArrayHeap)
{
  // Its array has an entry more, the entry 0, so the least capacity that it
  // cannot hold is one less than for the others
  Array<size_t> sizes = sizes_beyond_bytes(sizeof(int));
  sizes.append(max_size / sizeof(int));
  for (const size_t n : sizes)
    {
      SCOPED_TRACE(n);
      EXPECT_EQ(refusal_of([&] { ArrayHeap<int> h(n); }), "");
      ArrayHeap<int> h(4);
      h.insert(5);
      EXPECT_EQ(refusal_of([&] { h.allocate_storage(n); }), "");
      ASSERT_EQ(h.size(), 1u);
      EXPECT_EQ(h.top(), 5);
    }
}

TEST(SizeLimits, MemArrayAndArray)
{
  expect_memarray_and_array_refuse(5);
  expect_memarray_and_array_refuse(std::string("five"));
}

TEST(SizeLimits, QueuesAndStacks)
{
  expect_queues_and_stacks_refuse(5);
  expect_queues_and_stacks_refuse(std::string("five"));
}

TEST(SizeLimits, FlatSetAndFlatMap)
{
  for (const size_t n : sizes_beyond_bytes(sizeof(std::string)))
    {
      SCOPED_TRACE(n);
      FlatSet<std::string> s;
      s.insert("five");
      EXPECT_EQ(refusal_of([&] { s.reserve(n); }), "");
      EXPECT_EQ(s.size(), 1u);
      EXPECT_TRUE(s.contains("five"));
      EXPECT_EQ(refusal_of([&] { FlatSet<std::string> b(n); }), "");

      FlatMap<std::string, int> m;
      m.insert("five", 5);
      EXPECT_EQ(refusal_of([&] { m.reserve(n); }), "");
      EXPECT_EQ(m.size(), 1u);
      EXPECT_TRUE(m.contains("five"));
      EXPECT_EQ(refusal_of([&] { FlatMap<std::string, int> b(n); }), "");
    }
}

TEST(SizeLimits, SmallVector)
{
  for (const size_t n : sizes_beyond_bytes(sizeof(std::string)))
    {
      SCOPED_TRACE(n);
      SmallVector<std::string, 4> v;
      v.push_back("five");
      EXPECT_EQ(refusal_of([&] { v.reserve(n); }), "");
      ASSERT_EQ(v.size(), 1u);
      EXPECT_EQ(v[0], "five");
      EXPECT_EQ(refusal_of([&] { SmallVector<std::string, 4> b(n, "x"); }), "");
    }
}

TEST(SizeLimits, StaticEventTable)
{
  using Table = Static_Event_Table<int(int)>;
  for (const size_t n : sizes_beyond_bytes(sizeof(Table::Event_Handler)))
    {
      SCOPED_TRACE(n);
      EXPECT_EQ(refusal_of([&] { Table t(n); }), "");
    }
  EXPECT_EQ(Table(3).size(), 3u);
  EXPECT_EQ(Table().size(), 0u);
}

TEST(SizeLimits, Simplex)
{
  for (const size_t n : sizes_beyond_bytes(sizeof(double)))
    {
      SCOPED_TRACE(n);
      EXPECT_EQ(refusal_of([&] { Simplex<double> s(n); }), "");
    }
  // a restriction has one coefficient more than the variables: the largest
  // count of variables whose bytes fit leaves no room for it
  EXPECT_EQ(refusal_of([&] { Simplex<double> s(max_size / sizeof(double)); }), "");

  Simplex<double> s(2);
  const double row[] = {1.0, 1.0, 4.0};
  EXPECT_NE(s.put_restriction(row), nullptr);
}

// The hash tables take the count of buckets asked for to the next prime:
// they refuse before that search a count whose bytes do not fit, in their
// constructors and in resize(), which leaves the table as it was. Each
// table is given the counts past its own buckets, which have from 8 to 48
// bytes, and must say overflow_error: a count that passed the check would
// reach new[] after the search, which says another thing
TEST(SizeLimits, HashTables)
{
  const auto refuses = [] (const char * name, const size_t bucket_size, const auto & build)
    {
      for (const size_t n : sizes_beyond_bytes(bucket_size))
        {
          SCOPED_TRACE(testing::Message() << name << " of " << n << " buckets of " << bucket_size << " bytes");
          EXPECT_THROW(build(n), std::overflow_error);
        }
    };
  refuses("ODhashTable", sizeof(ODhashTable<int>::Bucket), [] (const size_t n) { ODhashTable<int> t(n); });
  refuses("OLhashTable", sizeof(OLhashTable<int>::Bucket), [] (const size_t n) { OLhashTable<int> t(n); });
  refuses("LhashTable", sizeof(Dlink), [] (const size_t n) { LhashTable<int> t(n); });
  refuses("DynSetLhash", sizeof(Dlink), [] (const size_t n) { DynSetLhash<int> t(n); });
  refuses("DynMapHash", sizeof(Dlink), [] (const size_t n) { DynMapHash<int, int> t(n); });
  refuses("DynLhashTable", sizeof(Dlink), [] (const size_t n) { DynLhashTable<int, int> t(n); });
  refuses("MapODhash", sizeof(MapODhash<int, std::string>::Bucket),
          [] (const size_t n) { MapODhash<int, std::string> t(n); });
  refuses("MapOLhash", sizeof(MapOLhash<int, std::string>::Bucket),
          [] (const size_t n) { MapOLhash<int, std::string> t(n); });
  // the sizes of the buckets are not all the same: the counts differ
  static_assert(sizeof(ODhashTable<int>::Bucket) > sizeof(OLhashTable<int>::Bucket));
  static_assert(sizeof(MapODhash<int, std::string>::Bucket) > sizeof(Dlink));

  ODhashTable<int> d;
  ASSERT_NE(d.insert(5), nullptr);
  const size_t d_cap = d.capacity();
  refuses("ODhashTable::resize()", sizeof(ODhashTable<int>::Bucket), [&d] (const size_t n) { d.resize(n); });
  EXPECT_EQ(d.size(), 1u);
  EXPECT_EQ(d.capacity(), d_cap);
  EXPECT_TRUE(d.has(5));

  OLhashTable<int> l;
  ASSERT_NE(l.insert(5), nullptr);
  const size_t l_cap = l.capacity();
  refuses("OLhashTable::resize()", sizeof(OLhashTable<int>::Bucket), [&l] (const size_t n) { l.resize(n); });
  EXPECT_EQ(l.size(), 1u);
  EXPECT_EQ(l.capacity(), l_cap);
  EXPECT_TRUE(l.has(5));

  DynSetLhash<int> lists;
  lists.insert(5);
  const size_t lists_cap = lists.capacity();
  refuses("DynSetLhash::resize()", sizeof(Dlink), [&lists] (const size_t n) { lists.resize(n); });
  EXPECT_EQ(lists.size(), 1u);
  EXPECT_EQ(lists.capacity(), lists_cap);
  EXPECT_TRUE(lists.has(5));
}

// The cuckoo filters compute their buckets, and the estimate of a Bloom
// filter its bits, as a double: for a count next to the largest size_t
// they are more than a size_t counts, and the conversion of such a double
// is undefined. A cuckoo filter of one entry per bucket was built there
// with one bucket
TEST(SizeLimits, CuckooFiltersAndTheEstimateOfABloomFilter)
{
  for (const size_t n : {max_size, max_size - 1, max_size / 2 + 1, max_size / 2, max_size / 4})
    {
      SCOPED_TRACE(n);
      EXPECT_THROW((Cuckoo_Filter<int, 8, 1>(n, 42)), std::overflow_error);
      EXPECT_THROW((Cuckoo_Filter<int, 8, 4>(n, 42)), std::overflow_error);
      EXPECT_THROW((Compact_Cuckoo_Filter<int, 8, 1>(n, 42)), std::overflow_error);
      EXPECT_THROW((Compact_Cuckoo_Filter<int, 8, 4>(n, 42)), std::overflow_error);
      EXPECT_THROW((void) Bloom_Filter<int>::estimate(n, 0.01), std::overflow_error);
    }

  // those that fit are built as before
  const Cuckoo_Filter<int, 8, 1> one(1000, 42);
  EXPECT_GE(one.capacity(), 1000u);
  const Compact_Cuckoo_Filter<int, 8, 4> four(1000, 42);
  EXPECT_GE(four.capacity(), 1000u);
  const auto [m, k] = Bloom_Filter<int>::estimate(1000, 0.01);
  EXPECT_EQ(m, 9586u);
  EXPECT_EQ(k, 7u);
  // and the most bits that a size_t counts, at a probability of 1/2
  EXPECT_NO_THROW((void) Bloom_Filter<int>::estimate(max_size / 4, 0.5));
}
