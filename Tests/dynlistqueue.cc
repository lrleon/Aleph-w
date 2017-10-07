
# include <gmock/gmock.h>

# include <tpl_dynListQueue.H>
# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

constexpr size_t N = 17;

struct SimpleQueue : public Test
{
  size_t n = 0;
  DynListQueue<int> s;
  SimpleQueue()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.put(i);
  }
};

struct ComplexQueue : public Test
{
  size_t n = 0;
  DynListQueue<DynList<int>> s;
  ComplexQueue()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.put({ int(i), 0, 1, 2, int(i) });
  }
};

TEST(DynListQueue, empty_queue)
{
  DynListQueue<int> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
  EXPECT_THROW(s.front(), underflow_error);
  EXPECT_THROW(s.rear(), underflow_error);
}

TEST_F(SimpleQueue, push_pop)
{
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.rear(), n - 1);
  EXPECT_EQ(s.front(), 0);

  const size_t m = 100;
  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.put(i), i);
  EXPECT_EQ(s.size(), n + m);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.get(), i);
  /*

  EXPECT_EQ(s.size(), n);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.push(i), i);
  EXPECT_EQ(s.size(), n + m);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.pop(), m - i - 1);
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);

  s.empty();
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
  */
}

/*

TEST_F(ComplexQueue, push_pop)
{
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.top().get_first(), n - 1);

  const size_t m = 100;
  for (size_t i = 0; i < m; ++i)
    {
      EXPECT_EQ(s.push({int(i), 0, 1, int(i)}).get_first(), i);
      EXPECT_EQ(s.top().get_first(), i);
      EXPECT_EQ(s.top().get_last(), i);
      EXPECT_EQ(s.top().nth(1), 0);
      EXPECT_EQ(s.top().nth(2), 1);
    }
  EXPECT_EQ(s.size(), n + m);

  for (size_t i = 0; i < m; ++i)
    {
      auto l = s.pop();
      EXPECT_EQ(l.get_first(), m - i - 1);      
      EXPECT_EQ(l.get_last(), m - i - 1);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }

  EXPECT_EQ(s.size(), n);

  for (size_t i = 0; i < m; ++i)
    {
      auto & l = s.push({ int(i), 0, 1, int(i) });
      EXPECT_EQ(l.get_first(), i);
      EXPECT_EQ(l.get_last(), i);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }
  EXPECT_EQ(s.size(), n + m);

  for (size_t i = 0; i < m; ++i)
    {
      auto l = s.pop();
      EXPECT_EQ(l.get_first(), m - i - 1);
      EXPECT_EQ(l.get_last(), m - i - 1);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }
  EXPECT_EQ(s.size(), n);
  
  for (size_t i = 0; i < n; ++i)
    {
      auto l = s.pop();
      EXPECT_EQ(l.get_first(), n - i - 1);
      EXPECT_EQ(l.get_last(), n - i - 1);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  for (size_t i = 0; i < m; ++i)
    {
      auto & l = s.push({ int(i), 0, 1, int(i) });
      EXPECT_EQ(l.get_first(), i);
      EXPECT_EQ(l.get_last(), i);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }
  EXPECT_EQ(s.size(), m);

  s.empty();
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST(DynListQueue, Iterator_on_empty_queue)
{
  DynListQueue<int> s;
  auto it = s.get_it();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_THROW(it.next(), overflow_error);
}

TEST_F(SimpleQueue, Iterator)
{
  auto it = s.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), n - i - 1);
}

TEST_F(ComplexQueue, Iterator)
{
  auto it = s.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    {
      EXPECT_EQ(it.get_curr().get_first(), n - i -1);
      EXPECT_EQ(it.get_curr().get_last(), n - i - 1);
      EXPECT_EQ(it.get_curr().nth(1), 0);
      EXPECT_EQ(it.get_curr().nth(2), 1);
    }
}

TEST_F(SimpleQueue, copy_operations)
{
  { // test copy ctor
    DynListQueue<int> sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(sc.size(), n);
    EXPECT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < n; ++i)
      EXPECT_EQ(sc.pop(), n - i - 1);
    EXPECT_EQ(i, n);
  }
  
  { // test copy assignment 
    DynListQueue<int> sc;
    sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < n; ++i)
      EXPECT_EQ(sc.pop(), n - i - 1);
    EXPECT_EQ(i, s.size());
  }

  // test move ctor
  DynListQueue<int> sc(move(s));
  EXPECT_FALSE(sc.is_empty());
  EXPECT_EQ(sc.size(), n);
  EXPECT_EQ(s.size(), 0);
  EXPECT_TRUE(s.is_empty());
  int i = 0;
  for (; i < n; ++i)
    {
      s.push(sc.pop());
      EXPECT_EQ(s.top(), n - i - 1);
    }
  EXPECT_EQ(i, n);
  EXPECT_EQ(s.size(), n);
  EXPECT_TRUE(sc.is_empty());

  sc = move(s);
  EXPECT_FALSE(sc.is_empty());
  EXPECT_EQ(sc.size(), n);
  EXPECT_TRUE(s.is_empty());
  i = 0;
  for (; i < n; ++i)
    {
      s.push(sc.pop());
      EXPECT_EQ(s.top(), i);
    }
  EXPECT_EQ(i, n);
  EXPECT_EQ(s.size(), n);
  EXPECT_TRUE(sc.is_empty());
}

TEST_F(ComplexQueue, copy_operations)
{
  { // test copy ctor
    DynListQueue<DynList<int>> sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    EXPECT_TRUE(eq(s.top(), sc.top()));
  }
  
  { // test copy assignment 
    DynListQueue<DynList<int>> sc;
    sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    EXPECT_TRUE(eq(s.top(), sc.top()));
  }

  // test move ctor
  DynListQueue<DynList<int>> sc(move(s));
  EXPECT_FALSE(sc.is_empty());
  EXPECT_EQ(sc.size(), n);
  EXPECT_EQ(s.size(), 0);
  EXPECT_TRUE(s.is_empty());

  EXPECT_EQ(sc.top().get_first(), n - 1);
  EXPECT_EQ(sc.top().get_last(), n - 1);
  EXPECT_EQ(sc.top().nth(1), 0);
  EXPECT_EQ(sc.top().nth(2), 1);

  s = move(sc);
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_TRUE(sc.is_empty());

  EXPECT_EQ(s.top().get_first(), n - 1);
  EXPECT_EQ(s.top().get_last(), n - 1);
  EXPECT_EQ(s.top().nth(1), 0);
  EXPECT_EQ(s.top().nth(2), 1);
}

TEST_F(SimpleQueue, traverse)
{
  size_t i = 0;
  bool ret = s.traverse([&i, N = n] (int k) { return (k == N - i++ - 1); });
  EXPECT_TRUE(ret);
  EXPECT_EQ(i, n);
}

TEST_F(ComplexQueue, traverse)
{
  int i = 0;
  auto ret = s.traverse([&i, N = n] (const DynList<int> & l)
			{
			  return l.get_first() == N - i - 1 and
			  l.get_last() == N - i++ - 1 and
			  l.nth(1) == 0 and l.nth(2) == 1;
			});
  EXPECT_TRUE(ret);
  EXPECT_EQ(i, n);
}

*/


