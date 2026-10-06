
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
 * @file memarray.cc
 * @brief Tests for Memarray
 */
# include <gtest/gtest.h>

# include <tpl_memArray.H>
# include <htlist.H>
# include <limits>
# include <memory>

using namespace std;
using namespace testing;
using namespace Aleph;

bool is_power_of_two(size_t x)
{
  return ((x != 0) && !(x & (x - 1))) != 0;
}

struct Default_MemArray : public Test
{
  const size_t n = 64;
  MemArray<int> m = {n};
};

struct MemArray_with_30_items : public Test
{
  MemArray<int> m;
  MemArray_with_30_items()
  {
    for (size_t i = 0; i < 30; ++i)
      m.append(i);
  }
};

TEST(MemArray, Basic_initialization)
{
  {
    MemArray<int> m1;
    EXPECT_TRUE(is_power_of_two(m1.capacity()));
    EXPECT_EQ(m1.size(), 0);
    EXPECT_TRUE(m1.is_empty());
    EXPECT_THROW(m1.get(), underflow_error);
  }

  {
    MemArray<int> m1(32);
    EXPECT_TRUE(is_power_of_two(m1.capacity()));
    EXPECT_EQ(m1.size(), 0);
    EXPECT_TRUE(m1.is_empty());
    EXPECT_THROW(m1.get(), underflow_error);

    MemArray<int> m2(31);
    MemArray<int> m3(17);
    EXPECT_TRUE(is_power_of_two(m2.capacity()));
    EXPECT_TRUE(m2.is_empty());
    EXPECT_TRUE(m3.is_empty());
    EXPECT_EQ(m2.size(), 0);
    EXPECT_EQ(m3.size(), 0);
    EXPECT_EQ(m1.capacity(), m2.capacity());
    EXPECT_EQ(m1.capacity(), m3.capacity());
  }

  {
    MemArray<int> m1(512);
    EXPECT_TRUE(is_power_of_two(m1.capacity()));
    EXPECT_EQ(m1.size(), 0);
    EXPECT_TRUE(m1.is_empty());
    EXPECT_THROW(m1.get(), underflow_error);

    MemArray<int> m2(257);
    MemArray<int> m3(316);
    EXPECT_TRUE(is_power_of_two(m2.capacity()));
    EXPECT_TRUE(m2.is_empty());
    EXPECT_TRUE(m3.is_empty());
    EXPECT_EQ(m2.size(), 0);
    EXPECT_EQ(m3.size(), 0);
    EXPECT_EQ(m1.capacity(), m2.capacity());
    EXPECT_EQ(m1.capacity(), m3.capacity());
  }

  {
    MemArray<int> m1(4096);
    EXPECT_TRUE(is_power_of_two(m1.capacity()));
    EXPECT_EQ(m1.size(), 0);
    EXPECT_TRUE(m1.is_empty());
    EXPECT_THROW(m1.get(), underflow_error);

    MemArray<int> m2(2049);
    MemArray<int> m3(3000);
    EXPECT_TRUE(is_power_of_two(m2.capacity()));
    EXPECT_TRUE(m2.is_empty());
    EXPECT_TRUE(m3.is_empty());
    EXPECT_EQ(m2.size(), 0);
    EXPECT_EQ(m3.size(), 0);
    EXPECT_EQ(m1.capacity(), m2.capacity());
    EXPECT_EQ(m1.capacity(), m3.capacity());
  }
}

TEST_F(Default_MemArray, growing_in_2_powers)
{
  const size_t n = m.capacity();
  for (size_t i = 0; i < n; ++i)
    m.append(i);

  EXPECT_EQ(m.size(), n);
  EXPECT_EQ(m.capacity(), n);

  m.append(n); // this append would cause expansion
  EXPECT_EQ(m.capacity(), 2 * n);
  EXPECT_EQ(m.size(), n + 1);
  EXPECT_EQ(m.get_first(), 0);
  EXPECT_EQ(m.get_last(), n);

  // testing gap opening
  m.insert(-1);
  EXPECT_EQ(m.get_first(), -1);
  EXPECT_EQ(m.get_last(), n);

  EXPECT_THROW(m[m.size()], out_of_range);
  EXPECT_THROW(m[m.capacity()], out_of_range);

  { // Testing operator [] in read mode
    int k = -1;
    for (size_t i = 0; i < m.size(); ++i, ++k)
      EXPECT_EQ(m[i], k);
    EXPECT_EQ(k, n + 1);
  }

  { // Testing operator [] in write mode
    for (size_t i = 0; i < m.size(); ++i)
      m[i]++;

    int k = 0;
    for (size_t i = 0; i < m.size(); ++i, ++k)
      EXPECT_EQ(m[i], k);
    EXPECT_EQ(k, n + 2);
  }
}

TEST_F(Default_MemArray, putn)
{
  const size_t dim = m.capacity();

  m.putn(dim + 1); // This will cause expansion

  EXPECT_EQ(m.capacity(), 2 * dim); // Verify expansion
  EXPECT_FALSE(m.is_empty());
  EXPECT_EQ(m.size(), dim + 1);

  for (size_t i = 0; i < m.size(); ++i)
    {
      EXPECT_NO_THROW(m[i]);
      m[i] = i;
    }

  EXPECT_THROW(m[m.size()], out_of_range);
  EXPECT_THROW(m.get(m.size() + 1), underflow_error);

  size_t k = 0;
  EXPECT_NE(m.size(), k);
  for (size_t i = 0; i < m.size(); ++i, ++k)
    EXPECT_EQ(m[i], i);
  EXPECT_EQ(k, m.size()); // TEST that loop has been executed

  const size_t curr_cap = m.capacity();
  const size_t avail = m.capacity() - m.size();
  m.putn(avail); // this shouldn't cause expansion

  EXPECT_EQ(m.capacity(), curr_cap);
  EXPECT_EQ(m.size(), m.capacity());

  int item;
  EXPECT_NO_THROW(item = m.get(m.size())); // it must take out all items

  EXPECT_EQ(item, 0);
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0);
}

TEST(MemArrayMoveOnly, AppendAndRemoveUniquePtr)
{
  MemArray<std::unique_ptr<int>> m;

  auto p1 = std::make_unique<int>(5);
  auto p2 = std::make_unique<int>(7);

  m.append(std::move(p1));
  m.append(std::move(p2));

  ASSERT_EQ(m.size(), 2u);
  EXPECT_EQ(*m[0], 5);
  EXPECT_EQ(*m[1], 7);

  auto last = m.remove_last();
  EXPECT_EQ(*last, 7);
  EXPECT_EQ(m.size(), 1u);

  auto first = m.remove_first();
  EXPECT_EQ(*first, 5);
  EXPECT_TRUE(m.is_empty());
}

TEST_F(Default_MemArray, access_operator)
{
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0);
  EXPECT_NE(m.capacity(), 0);

  const size_t cap1 = m.capacity();

  // Test invalid accesses without insertion neither expansion 
  size_t k = 0;
  for (size_t i = 0; i < m.capacity(); ++i, ++k)
    EXPECT_THROW(m[i], out_of_range);
  EXPECT_EQ(k, m.capacity());
  EXPECT_EQ(m.capacity(), cap1); // capacity has not changed
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0);

  // Insert until capacity (no expansion)
  k = 0;
  for (size_t i = 0; i < m.capacity(); ++i, ++k)
    m.append(i);
  EXPECT_EQ(k, m.capacity());
  EXPECT_EQ(m.size(), m.capacity());

  // Test that item were inserted
  k = 0;
  for (size_t i = 0; i < m.capacity(); ++i, ++k)
    {
      EXPECT_NO_THROW(m[i]);
      EXPECT_EQ(m[i], i);
    }
  EXPECT_EQ(k, m.capacity());

  // Now we cause an expansion
  k = 0;
  for (size_t i = m.size(); i < 2 * cap1; ++i, ++k)
    m.append(i);

  EXPECT_EQ(k, cap1);
  EXPECT_EQ(m.capacity(), 2 * cap1);
  EXPECT_EQ(m.size(), 2 * cap1);

  k = 0;
  for (size_t i = 0; i < m.size(); ++i, ++k)
    {
      EXPECT_NO_THROW(m[i]);
      EXPECT_EQ(m[i], i);
    }
  EXPECT_EQ(k, m.capacity());
}

TEST_F(Default_MemArray, reserve)
{
  const size_t cap = m.capacity();
  EXPECT_TRUE(m.is_empty());
  EXPECT_NE(m.capacity(), 0);
  EXPECT_EQ(m.size(), 0);

  m.reserve(2 * cap + 1); // this should expand to 4*cap
  EXPECT_EQ(m.capacity(), 4 * cap);
}

TEST_F(MemArray_with_30_items, copy_and_assigment)
{
  EXPECT_FALSE(m.is_empty());
  EXPECT_EQ(m.size(), 30);
  EXPECT_EQ(m.capacity(), 32);
  size_t k = 0;
  for (size_t i = 0; i < m.size(); ++i, ++k)
    EXPECT_EQ(m[i], i);
  EXPECT_EQ(k, m.size());

  { // Copy constructor
    MemArray<int> aux = m;
    EXPECT_FALSE(aux.is_empty());
    EXPECT_EQ(aux.size(), 30);
    EXPECT_EQ(aux.capacity(), 32);
    size_t k = 0;
    for (size_t i = 0; i < m.size(); ++i, ++k)
      EXPECT_EQ(aux[i], i);
    EXPECT_EQ(k, m.size());
    EXPECT_NE(m.get_ptr(), aux.get_ptr());
  }

  { // move constructor
    auto ptr = m.get_ptr();
    MemArray<int> aux = move(m);
    EXPECT_EQ(aux.get_ptr(), ptr);
    EXPECT_FALSE(aux.is_empty());
    EXPECT_EQ(aux.size(), 30);
    EXPECT_EQ(aux.capacity(), 32);
    EXPECT_TRUE(m.is_empty());
    EXPECT_EQ(m.size(), 0);
    EXPECT_EQ(m.capacity(), 0);
    EXPECT_EQ(m.get_ptr(), nullptr); // array of zero must be nullptr
    size_t k = 0;
    for (size_t i = 0; i < m.size(); ++i, ++k)
      EXPECT_EQ(aux[i], i);
    EXPECT_EQ(k, m.size());
    EXPECT_NE(m.get_ptr(), aux.get_ptr());

    m.swap(aux); // restore m to previous initialized state
    EXPECT_EQ(m.get_ptr(), ptr);
    EXPECT_TRUE(aux.is_empty());
    EXPECT_EQ(aux.size(), 0);
    EXPECT_EQ(aux.capacity(), 0);
    EXPECT_FALSE(m.is_empty());
    EXPECT_EQ(m.size(), 30);
    EXPECT_EQ(m.capacity(), 32);
    k = 0;
    for (size_t i = 0; i < m.size(); ++i, ++k)
      EXPECT_EQ(m[i], i);
    EXPECT_EQ(k, m.size());
  }

  // copy assigment
  MemArray<int> aux;
  EXPECT_TRUE(aux.is_empty());
  EXPECT_EQ(aux.size(), 0);
  EXPECT_NE(aux.capacity(), 0);
  EXPECT_NE(aux.get_ptr(), nullptr);

  aux = m;
  EXPECT_FALSE(aux.is_empty());
  EXPECT_NE(m.size(), 0);
  EXPECT_EQ(aux.size(), m.size());
  EXPECT_EQ(aux.capacity(), m.capacity());
  EXPECT_FALSE(m.is_empty());
  EXPECT_NE(m.size(), 0);
  EXPECT_NE(m.capacity(), 0);
  EXPECT_NE(m.get_ptr(), aux.get_ptr()); // array of zero must be nullptr
  k = 0;
  for (size_t i = 0; i < m.size(); ++i, ++k)
    EXPECT_EQ(aux[i], m[i]);
  EXPECT_EQ(k, m.size());

  // TODO move assigment
}

TEST(MemArray, zero_capacity)
{
  MemArray<int> m(0);
  EXPECT_NE(m.capacity(), 0);
  EXPECT_EQ(m.size(), 0);
  EXPECT_NE(m.get_ptr(), nullptr);
  EXPECT_TRUE(m.is_empty());
}

TEST(MemArray, insertion_with_rvalues)
{
  MemArray<DynList<int>> m;
  EXPECT_EQ(m.size(), 0);
  EXPECT_TRUE(m.is_empty());

  size_t N = 0;
  for (size_t i = 0; i < 10; ++i)
    {
      DynList<int> l;
      EXPECT_TRUE(l.is_empty());
      for (size_t k = 0; k < 10; ++k, ++N)
        l.append(N);
      EXPECT_FALSE(l.is_empty());
      m.insert(move(l));
      EXPECT_TRUE(l.is_empty());
    }

  size_t n = 0;
  for (long i = 9; i >= 0; --i) // descending for matching values of N
    {
      const DynList<int> &l = m[i];
      EXPECT_FALSE(l.is_empty());
      for (auto it = l.get_it(); it.has_curr(); it.next(), ++n)
        EXPECT_EQ(it.get_curr(), n);
    }
}

TEST(MemArray, remove_with_rvalues)
{
  constexpr size_t num_items = 10;
  MemArray<DynList<int>> m;
  size_t N = 0;
  for (size_t i = 0; i < num_items; ++i)
    {
      DynList<int> l;
      EXPECT_TRUE(l.is_empty());
      for (size_t k = 0; k < num_items; ++k, ++N)
        l.insert(N);
      EXPECT_FALSE(l.is_empty());
      m.insert(move(l));
      EXPECT_TRUE(l.is_empty());
    }

  size_t n = N - 1;
  for (size_t i = 0; i < num_items; ++i)
    {
      DynList<int> l = m.remove_first();
      auto it = l.get_it();
      for (size_t k = 0; k < 10; ++k, it.next(), --n)
        EXPECT_EQ(it.get_curr(), n);
      assert(num_items - i < m.capacity()); // hard assert. Better leave it!
      // The removed slot lies beyond the logical size: inspect it with
      // access(), since operator() only reaches live items. Container
      // annotations report any read of that slot, so skip it then.
#ifndef ALEPH_MEMARRAY_ANNOTATIONS
      EXPECT_TRUE(m.access(num_items - i - 1).is_empty()); // verify moving
#endif
    }
}

TEST_F(Default_MemArray, contraction)
{
  for (size_t i = 0; i < n; ++i)
    m.append(i);

  EXPECT_EQ(m.capacity(), n);
  EXPECT_EQ(m.capacity(), m.size());

  size_t N = m.capacity();
  for (size_t i = 0; i < n; ++i)
    {
      EXPECT_EQ(m.remove_last(), n - i - 1);
      if (m.size() == N / 4 - 1 and m.size() > m.contract_threshold)
        {
          N /= 2;
          EXPECT_EQ(m.capacity(), N); // valid if contraction was done!
        }
    }
}

TEST_F(Default_MemArray, remove_on_empty)
{
  EXPECT_THROW(m.remove_last(), underflow_error);
  EXPECT_THROW(m.remove_first(), underflow_error);
  EXPECT_THROW(m.get(0), underflow_error);
  EXPECT_THROW(m.get(), underflow_error);
  EXPECT_THROW(m.get(2), underflow_error);
}

TEST(MemArray, reverse_empty_and_single_item)
{
  MemArray<int> m;
  EXPECT_NO_THROW(m.reverse());
  EXPECT_TRUE(m.is_empty());

  m.append(7);
  EXPECT_NO_THROW(m.reverse());
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m[0], 7);
}

TEST(MemArray, putn_rejects_capacity_overflow)
{
  MemArray<int> m;
  EXPECT_THROW(m.putn(std::numeric_limits<size_t>::max()), overflow_error);
}

TEST(MemArray, as_stack)
{
  MemArray<int> m;

  EXPECT_THROW(m.top(), underflow_error);

  for (size_t i = 0; i < 100; ++i)
    EXPECT_EQ(m.push(i), i);

  for (size_t i = 100; i > 0; --i)
    EXPECT_EQ(m.pop(), i - 1);

  EXPECT_TRUE(m.is_empty());
  ASSERT_EQ(m.size(), 0);
  EXPECT_THROW(m.top(), underflow_error);
  EXPECT_THROW(m.pop(), underflow_error);
}

TEST(MemArray, Iterator_on_empty_container)
{
  MemArray<int> empty_m;
  MemArray<int>::Iterator it = empty_m;
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_THROW(it.next(), overflow_error);
  EXPECT_THROW(it.prev(), underflow_error);
  it.reset();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_THROW(it.next(), overflow_error);
  EXPECT_THROW(it.prev(), underflow_error);
  it.reset_last();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), underflow_error);
  EXPECT_THROW(it.next(), overflow_error);
  EXPECT_THROW(it.prev(), underflow_error);
}

TEST_F(Default_MemArray, Iterator)
{
  int i = 0;
  for (MemArray<int>::Iterator it = m; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);

  MemArray<int>::Iterator it = m;
  it.reset_last();
  i = n - 1;
  for (MemArray<int>::Iterator it = m; it.has_curr(); it.prev(), --i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST(MemArray, traverse_on_empty_container)
{
  MemArray<int> m;
  size_t n = 0;
  auto ret = m.traverse([&n](int)
                        {
                          ++n;
                          return true;
                        });
  EXPECT_TRUE(ret);
  EXPECT_EQ(n, 0);
}

TEST_F(Default_MemArray, clear)
{
  const size_t cap = m.capacity();
  
  // Test 1: clear on empty
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0);
  
  static_assert(noexcept(m.clear()), "clear() must be noexcept");
  m.clear(); 
  
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0);
  EXPECT_EQ(m.capacity(), cap);
  EXPECT_NE(m.get_ptr(), nullptr);

  // Test 2: clear on populated
  for (size_t i = 0; i < 10; ++i)
    m.append(i);
    
  EXPECT_FALSE(m.is_empty());
  EXPECT_EQ(m.size(), 10);
  const size_t cap_before = m.capacity();
  const int* ptr_before = m.get_ptr();

  m.clear();

  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0);
  EXPECT_EQ(m.capacity(), cap_before);
  EXPECT_EQ(m.get_ptr(), ptr_before);
  
  // Verify it can be reused
  m.append(100);
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m[0], 100);
}

TEST_F(Default_MemArray, traverse)
{
  int N = 0;
  auto ret = m.traverse([&N, this](int i)
                        {
                          ++N;
                          return i == n / 2;
                        }); // m is empty
  EXPECT_TRUE(ret);
  EXPECT_EQ(N, 0);

  for (size_t i = 0; i < n; ++i)
    m.append(i);

  EXPECT_EQ(N, 0);
  EXPECT_TRUE(m.size() > 0);
  EXPECT_EQ(m.size(), n);
  ret = m.traverse([&N, this](int i)
                   {
                     ++N;
                     return i < n / 2;
                   });
  EXPECT_FALSE(ret);
  EXPECT_EQ(N, n / 2 + 1);
}

// operator() reaches live items only: debug builds assert i < n. access()
// still reaches every reserved slot (ArrayQueue relies on it).
#ifndef NDEBUG
TEST(MemArray, call_operator_asserts_a_live_index_in_debug_builds)
{
  MemArray<int> a(16);
  a.put(1);
  a.put(2);
  EXPECT_EQ(a(1), 2);
  EXPECT_EQ(a.capacity(), 16U);
  EXPECT_DEATH((void) a(2), "i < n");
  EXPECT_NO_FATAL_FAILURE((void) a.access(2));
}
#endif

#ifdef ALEPH_MEMARRAY_ANNOTATIONS
// With ALEPH_ANNOTATE_CONTAINERS and AddressSanitizer, the reserved slots
// beyond the logical size are poisoned: a raw read of slot size() is reported
// as container-overflow after every operation that changes the size or the
// buffer, while the legitimate accesses of those operations are not.

/// @brief Read the first byte of slot i through a volatile access.
static void read_slot(const string *raw, size_t i)
{
  const volatile char *byte = reinterpret_cast<const volatile char *>(raw + i);
  volatile char sink = *byte;
  (void) sink;
}

/// @brief Verify that ASan rejects the first non-logical element.
static void expect_overflow_at_size(const MemArray<string> &a)
{
  ASSERT_LT(a.size(), a.capacity());
  if (a.size() > 0)
    read_slot(a.get_ptr(), a.size() - 1);  // live: no report
  EXPECT_DEATH(read_slot(a.get_ptr(), a.size()), "container-overflow");
}

TEST(MemArray, asan_annotations_follow_size_and_buffer_changes)
{
  MemArray<string> a(16);
  expect_overflow_at_size(a);                     // empty after construction
  for (int i = 0; i < 5; ++i)
    a.put(to_string(i));                          // put
  EXPECT_EQ(a(4), "4");
  expect_overflow_at_size(a);

  for (int i = 5; i < 40; ++i)
    a.append(to_string(i));                       // growth by expand()
  expect_overflow_at_size(a);

  for (int i = 0; i < 30; ++i)
    (void) a.get();                               // shrink by contract()
  EXPECT_EQ(a.size(), 10U);
  expect_overflow_at_size(a);

  a.putn(3);                                      // logical growth in place
  a(12) = "putn";
  expect_overflow_at_size(a);

  a.push("front");                                // open_gap()
  EXPECT_EQ(a(0), "front");
  EXPECT_EQ(a.remove_first(), "front");           // close_gap()
  expect_overflow_at_size(a);

  a.reserve(256);                                 // reallocation
  EXPECT_EQ(a(12), "putn");
  expect_overflow_at_size(a);

  MemArray<string> copy(a);                       // copy construction
  expect_overflow_at_size(copy);
  MemArray<string> assigned(4);
  assigned = a;                                   // copy assignment
  expect_overflow_at_size(assigned);
  MemArray<string> moved(std::move(copy));         // move construction
  expect_overflow_at_size(moved);
  assigned.swap(moved);
  expect_overflow_at_size(assigned);
  expect_overflow_at_size(moved);

  a.empty();                                      // logical emptying
  expect_overflow_at_size(a);
  a.empty_and_release();                          // release and reallocation
  expect_overflow_at_size(a);
  a.put("again");
  EXPECT_EQ(a(0), "again");
}
#endif

static_assert(std::is_nothrow_move_constructible_v<MemArray<int>>);

TEST(MemArray, moved_from_storage_recovers_on_every_insertion_path)
{
  for (int operation = 0; operation < 10; ++operation)
    {
      SCOPED_TRACE(operation);
      MemArray<int> source(16);
      source.append(17);
      MemArray<int> moved(std::move(source));
      EXPECT_EQ(source.size(), 0u);
      EXPECT_EQ(source.capacity(), 0u);
      EXPECT_FALSE(MemArray<int>::Iterator(source).has_curr());
      EXPECT_TRUE(source.traverse([](int) { return false; }));
      const int value = 9;
      switch (operation)
        {
        case 0: source.put(value); break;
        case 1: source.put(9); break;
        case 2: source.append(value); break;
        case 3: source.append(9); break;
        case 4: source.insert(value); break;
        case 5: source.insert(9); break;
        case 6: source.push(value); break;
        case 7: source.putn(1); source(0) = 9; break;
        case 8: source.reserve(16); source.put(9); break;
        case 9: source.empty_and_release(); source.put(9); break;
        }
      ASSERT_EQ(source.size(), 1u);
      EXPECT_GE(source.capacity(), MemArray<int>::Min_Dim);
      EXPECT_EQ(source(0), 9);
      EXPECT_EQ(moved(0), 17);
    }
}

TEST(MemArray, copying_and_moving_an_empty_moved_from_array)
{
  MemArray<int> source;
  source.put(7);
  MemArray<int> moved(std::move(source));
  MemArray<int> copy(source);
  MemArray<int> moved_again(std::move(source));
  MemArray<int> assigned;
  assigned = source;
  EXPECT_TRUE(copy.is_empty());
  EXPECT_TRUE(moved_again.is_empty());
  EXPECT_TRUE(assigned.is_empty());
  EXPECT_THROW(source[0], out_of_range);
  EXPECT_THROW(source.get(), underflow_error);
  moved_again.put(8);
  EXPECT_EQ(moved_again(0), 8);
  EXPECT_EQ(moved(0), 7);
}

TEST(MemArray, boolean_relocation_reads_only_logical_initialized_elements)
{
  MemArray<bool> a(32);
  a.put(true);
  a.reserve(64);
  EXPECT_TRUE(a(0));
  EXPECT_TRUE(a.get());  // contraction with no remaining logical elements
  a.put(true);
  a.putn(64);           // growth must not read fresh uninitialized slots
  for (size_t i = 1; i < a.size(); ++i)
    a(i) = false;
  EXPECT_TRUE(a(0));
  EXPECT_FALSE(a(64));
}

namespace
{
  /// @brief Element with observable lifetime and controllable assignment failures.
  struct ThrowingArrayItem
  {
    static inline int live = 0;
    static inline int assignments_left = -1;
    int value = 0;

    /// @brief Count default construction of a storage slot.
    ThrowingArrayItem() { ++live; }
    /// @brief Count copying an element.
    ThrowingArrayItem(const ThrowingArrayItem &other) : value(other.value) { ++live; }
    /// @brief Count a nonthrowing move construction.
    ThrowingArrayItem(ThrowingArrayItem &&other) noexcept : value(other.value) { ++live; }
    /// @brief Count destruction, including unused storage slots.
    ~ThrowingArrayItem() { --live; }

    /// @brief Fail at the configured assignment, otherwise store the value.
    ThrowingArrayItem &operator=(const ThrowingArrayItem &other)
    {
      if (assignments_left == 0)
        throw std::runtime_error("element assignment failed");
      if (assignments_left > 0)
        --assignments_left;
      value = other.value;
      return *this;
    }

    /// @brief Use the same controlled failure for move assignment.
    ThrowingArrayItem &operator=(ThrowingArrayItem &&other)
    {
      return *this = static_cast<const ThrowingArrayItem &>(other);
    }
  };
}

TEST(MemArray, failed_assignment_releases_temporary_storage_and_restores_annotations)
{
  ASSERT_EQ(ThrowingArrayItem::live, 0);
  {
    MemArray<ThrowingArrayItem> a(16);
    a.putn(3);
    a(0).value = 7;
    const int original_live = ThrowingArrayItem::live;

    ThrowingArrayItem::assignments_left = 1;
    EXPECT_THROW(a.reserve(128), std::runtime_error);
    EXPECT_EQ(ThrowingArrayItem::live, original_live);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.capacity(), 16u);

    ThrowingArrayItem::assignments_left = 1;
    EXPECT_THROW({ MemArray<ThrowingArrayItem> copy(a); }, std::runtime_error);
    EXPECT_EQ(ThrowingArrayItem::live, original_live);

    ThrowingArrayItem::assignments_left = 0;
    EXPECT_THROW(a.put(a(0)), std::runtime_error);
    EXPECT_EQ(a.size(), 3u);
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    EXPECT_EQ(__sanitizer_verify_contiguous_container(
                a.get_ptr(), a.get_ptr() + a.size(), a.get_ptr() + a.capacity()), 1);
#endif
    ThrowingArrayItem::assignments_left = -1;
    a.put(a(0));
    EXPECT_EQ(a(3).value, 7);
  }
  EXPECT_EQ(ThrowingArrayItem::live, 0);
}
