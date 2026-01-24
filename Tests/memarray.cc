
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/



/**
 * @file memarray.cc
 * @brief Tests for Memarray
 */
# include <gtest/gtest.h>

# include <tpl_memArray.H>
# include <htlist.H>

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
      EXPECT_TRUE(m(num_items - i - 1).is_empty()); // verify moving
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
  EXPECT_THROW(m.get(), underflow_error);
  EXPECT_THROW(m.get(2), underflow_error);
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
