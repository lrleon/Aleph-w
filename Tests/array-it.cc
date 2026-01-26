
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
 * @file array-it.cc
 * @brief Tests for Array It
 */
# include <gtest/gtest.h>

# include <array_it.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(Array_Container, empty_array)
{
  Array_Container<int> a(nullptr, 0);
  EXPECT_TRUE(a.is_empty());
  EXPECT_EQ(a.size(), 0);
  EXPECT_THROW(a.get_first(), std::underflow_error);
  EXPECT_THROW(a.get_last(), std::underflow_error);
}

TEST(Array_Container, container_operations)
{
  int ptr[20];
  Array_Container<int> a(ptr, 20);
  EXPECT_FALSE(a.is_empty());
  EXPECT_EQ(a.size(), 20);
  EXPECT_NO_THROW(a.get_first());
  EXPECT_NO_THROW(a.get_last());
}

TEST(Array_Container, helper_make_array_container)
{
  int ptr[4] = {0, 1, 2, 3};
  auto c = make_array_container(ptr, 4);

  EXPECT_FALSE(c.is_empty());
  EXPECT_EQ(c.size(), 4);
  EXPECT_EQ(c.get_first(), 0);
  EXPECT_EQ(c.get_last(), 3);

  auto it = c.get_it();
  for (int i = 0; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST(Array_Iterator, iterator_on_empty_array)
{
  int ptr[20];
  Array_Iterator<int> it(ptr, 10, 0);
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);

  it.reset();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);

  it.reset_last();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::underflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);
}

TEST(Array_Iterator, invalid_parameters)
{
  int ptr[10];

  EXPECT_THROW(Array_Iterator<int>(nullptr, 5, 1), std::invalid_argument);
  EXPECT_THROW(Array_Iterator<int>(ptr, 5, 6), std::domain_error);
  EXPECT_THROW(Array_Iterator<int>(ptr, 0, 1), std::domain_error);
  EXPECT_THROW(Array_Iterator<int>(ptr, 5, 3, 4, 5), std::domain_error);

  EXPECT_NO_THROW(Array_Iterator<int>(ptr, 5, 3, 1, 2));
}

constexpr size_t N = 29;

struct Array_of_n_items : public testing::Test
{
  size_t n = 0;
  int * a = nullptr;
  Array_of_n_items() : a(new int [N])
  {
    for (size_t i = 0; i < N; ++i, ++n)
      a[i] = i;
  }
  ~Array_of_n_items() { delete [] a; }
};

TEST_F(Array_of_n_items, Iterator_with_simple_bounds)
{
  Array_Iterator<int> it = { a, n, n };

  EXPECT_TRUE(it.has_curr());
  EXPECT_NO_THROW(it.get_curr());
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);

  it.reset();
  EXPECT_TRUE(it.has_curr());
  EXPECT_NO_THROW(it.get_curr());
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_NO_THROW(it.prev());
  EXPECT_EQ(it.get_curr(), n - 1);

  it.reset_last();
  EXPECT_TRUE(it.has_curr());
  EXPECT_NO_THROW(it.get_curr());
  for (size_t i = n - 1; it.has_curr(); it.prev(), --i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_THROW(it.get_curr(), std::underflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);
  EXPECT_NO_THROW(it.next());
  EXPECT_EQ(it.get_curr(), 0);
}

TEST_F(Array_of_n_items, Iterator_on_Array_Container)
{
  Array_Container<int> c(a, n);
  auto it = c.get_it();

  EXPECT_TRUE(it.has_curr());
  EXPECT_NO_THROW(it.get_curr());
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);

  it.reset();
  EXPECT_TRUE(it.has_curr());
  EXPECT_NO_THROW(it.get_curr());
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_NO_THROW(it.prev());
  EXPECT_EQ(it.get_curr(), n - 1);

  it.reset_last();
  EXPECT_TRUE(it.has_curr());
  EXPECT_NO_THROW(it.get_curr());
  for (size_t i = n - 1; it.has_curr(); it.prev(), --i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_THROW(it.get_curr(), std::underflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);
  EXPECT_NO_THROW(it.next());
  EXPECT_EQ(it.get_curr(), 0);

  it.end();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_NO_THROW(it.prev());
  EXPECT_NO_THROW(it.get_curr());
  EXPECT_EQ(it.get_curr(), n - 1);
}

struct Array100 : public testing::Test
{
  const size_t dim = 100;
  int * ptr = new int [dim];
  Array100()
  {
    for (size_t i = 0; i < dim; ++i)
      ptr[i] = i;
  }
  ~Array100() { delete [] ptr; }    
};

TEST_F(Array100, Iterator_on_empty_array_region)
{
  Array_Iterator<int> it(ptr + 23, 0, 0);
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), std::overflow_error);
  ASSERT_THROW(it.next(), std::overflow_error);
  ASSERT_THROW(it.prev(), std::underflow_error);
}

TEST_F(Array100, Iterator_on_array_region)
{
  // Iterate on [23, 47]
  Array_Iterator<int> it(ptr + 23, dim - 23 + 1, 47 - 23 + 1);

  ASSERT_TRUE(it.has_curr());

  int i = 23;
  for (; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  ASSERT_EQ(i, 48);

  it.reset_first();
  i = 23;
  for (; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  ASSERT_EQ(i, 48);

  it.reset_last();
  ASSERT_TRUE(it.has_curr());
  for (i = 47; it.has_curr(); --i, it.prev())
    ASSERT_EQ(it.get_curr(), i);
  ASSERT_EQ(i, 22);
}

TEST_F(Array100, Iterator_on_circular_array_region)
{
  // iterate on [47, 7]
  Array_Iterator<int> it(ptr, dim, dim - 47 + 1 + 7, 47, 7);

  ASSERT_TRUE(it.has_curr());
  int i = 47;
  for (; it.has_curr(); it.next(), i = (i + 1) % dim)
    ASSERT_EQ(it.get_curr(), i);
  ASSERT_EQ(i, 8);

  it.reset_last();
  i = 7;
  for (; it.has_curr(); it.prev(), --i)
    {
      ASSERT_EQ(it.get_curr(), i);
      if (i == 0)
	i = dim;
    }
}

TEST_F(Array100, Iterator_on_full_circular_array_region)
{
  Array_Iterator<int> it(ptr, dim, dim, 47, 47);
  ASSERT_TRUE(it.has_curr());

  int i = 47, k = 0;
  for (; it.has_curr(); it.next(), i = (i + 1) % dim, ++k)
    ASSERT_EQ(it.get_curr(), i);
  ASSERT_EQ(k, dim);

  it.reset_last();
  i = 47, k = 0;
  for (; it.has_curr(); it.prev(), --i, ++k)
    {
      ASSERT_EQ(it.get_curr(), i);
      if (i == 0)
	i = dim;
    }
  ASSERT_EQ(k, dim);
}
