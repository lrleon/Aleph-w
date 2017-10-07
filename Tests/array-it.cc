
# include <gmock/gmock.h>

# include <array_it.H>

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
