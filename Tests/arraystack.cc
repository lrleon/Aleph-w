
# include <gmock/gmock.h>

# include <tpl_arrayStack.H>
# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

constexpr size_t N = 17;

struct SimpleStack : public Test
{
  size_t n = 0;
  ArrayStack<int> s;
  SimpleStack()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.push(i);
  }
};

struct ComplexStack : public Test
{
  size_t n = 0;
  ArrayStack<DynList<int>> s;
  ComplexStack()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.push({ int(i), 0, 1, 2, int(i) });
  }
};

TEST(ArrayStack, empty_stack)
{
  ArrayStack<int> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
  EXPECT_THROW(s.top(), underflow_error);
  EXPECT_THROW(s.top(10), out_of_range);
}

TEST_F(SimpleStack, push_pop)
{
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.top(), n - 1);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(s.top(i), n - i - 1);

  const size_t m = 100;
  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.push(i), i);
  EXPECT_EQ(s.size(), n + m);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.pop(), m - i - 1);

  EXPECT_EQ(s.size(), n);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.push(i), i);
  EXPECT_EQ(s.size(), n + m);
  EXPECT_EQ(s.popn(m), 0);
  EXPECT_EQ(s.size(), n);

  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(s.pop(), n - i - 1);
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  s.pushn(n);
  EXPECT_EQ(s.size(), n);
  for (size_t i = 0; i < n; ++i)
    s.top(i) = i;

  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(s.top(i), i);

  EXPECT_EQ(s.popn(n), n - 1);
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(s.push(i), i);
  EXPECT_EQ(s.size(), m);

  s.empty();
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST_F(ComplexStack, push_pop)
{
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.top().get_first(), n - 1);
  for (size_t i = 0; i < n; ++i)
    {
      EXPECT_EQ(s.top(i).get_first(), n - i - 1);
      EXPECT_EQ(s.top(i).get_last(), n - i - 1);
      EXPECT_EQ(s.top(i).nth(1), 0);
    }

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
  EXPECT_EQ(s.popn(m).get_first(), 0);
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

  {
    auto & ll = s.pushn(n);
    EXPECT_TRUE(ll.is_empty());
  }
  EXPECT_EQ(s.size(), n);
  for (size_t i = 0; i < n; ++i)
    s.top(i) = { int(i), 0, 1, int(i) };

  for (size_t i = 0; i < n; ++i)
    {
      auto & l = s.top(i);
      EXPECT_EQ(l.get_first(), i);
      EXPECT_EQ(l.get_last(), i);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }

  {
    auto ll = s.popn(n);
    EXPECT_EQ(ll.get_first(), n - 1);
    EXPECT_EQ(ll.get_last(), n - 1);
    EXPECT_EQ(ll.nth(1), 0);
    EXPECT_EQ(ll.nth(2), 1);
  }
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

TEST(ArrayStack, Iterator_on_empty_stack)
{
  ArrayStack<int> s;
  auto it = s.get_it();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_THROW(it.next(), overflow_error);
}

TEST_F(SimpleStack, Iterator)
{
  auto it = s.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);

  it.reset_last();
  for (size_t i = 0; it.has_curr(); it.prev(), ++i)
    EXPECT_EQ(it.get_curr(), n - i -1);
}

TEST_F(ComplexStack, Iterator)
{
  auto it = s.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    {
      EXPECT_EQ(it.get_curr().get_first(), i);
      EXPECT_EQ(it.get_curr().get_last(), i);
      EXPECT_EQ(it.get_curr().nth(1), 0);
      EXPECT_EQ(it.get_curr().nth(2), 1);
    }

  it.reset_last();
  for (size_t i = 0; it.has_curr(); it.prev(), ++i)
    {
      EXPECT_EQ(it.get_curr().get_first(), n - i -1);
      EXPECT_EQ(it.get_curr().get_last(), n - i -1);
      EXPECT_EQ(it.get_curr().nth(1), 0);
      EXPECT_EQ(it.get_curr().nth(2), 1);
    }
}

TEST_F(SimpleStack, copy_operations)
{
  { // test copy ctor
    ArrayStack<int> sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      EXPECT_EQ(s.top(i), sc.top(i));
    EXPECT_EQ(i, s.size());
  }
  
  { // test copy assignment 
    ArrayStack<int> sc;
    sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      EXPECT_EQ(s.top(i), sc.top(i));
    EXPECT_EQ(i, s.size());
  }

  // test move ctor
  ArrayStack<int> sc(move(s));
  EXPECT_FALSE(sc.is_empty());
  EXPECT_EQ(sc.size(), n);
  EXPECT_EQ(s.size(), 0);
  EXPECT_TRUE(s.is_empty());
  int i = 0;
  for (; i < sc.size(); ++i)
    EXPECT_EQ(sc.top(i), n - i - 1);
  EXPECT_EQ(i, sc.size());

  s = move(sc);
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_TRUE(sc.is_empty());
  i = 0;
  for (; i < s.size(); ++i)
    EXPECT_EQ(s.top(i), n - i - 1);
  EXPECT_EQ(i, s.size());
}

TEST_F(ComplexStack, copy_operations)
{
  { // test copy ctor
    ArrayStack<DynList<int>> sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      EXPECT_TRUE(eq(s.top(i), sc.top(i)));
    EXPECT_EQ(i, s.size());
  }
  
  { // test copy assignment 
    ArrayStack<DynList<int>> sc;
    sc = s;
    EXPECT_FALSE(sc.is_empty());
    EXPECT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      EXPECT_TRUE(eq(s.top(i), sc.top(i)));
    EXPECT_EQ(i, s.size());
  }

  // test move ctor
  ArrayStack<DynList<int>> sc(move(s));
  EXPECT_FALSE(sc.is_empty());
  EXPECT_EQ(sc.size(), n);
  EXPECT_EQ(s.size(), 0);
  EXPECT_TRUE(s.is_empty());
  int i = 0;
  for (; i < sc.size(); ++i)
    {
      EXPECT_EQ(sc.top(i).get_first(), n - i - 1);
      EXPECT_EQ(sc.top(i).get_last(), n - i - 1);
      EXPECT_EQ(sc.top(i).nth(1), 0);
      EXPECT_EQ(sc.top(i).nth(2), 1);
    }
  EXPECT_EQ(i, sc.size());

  s = move(sc);
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_TRUE(sc.is_empty());
  i = 0;
  for (; i < s.size(); ++i)
    {
      EXPECT_EQ(s.top(i).get_first(), n - i - 1);
      EXPECT_EQ(s.top(i).get_last(), n - i - 1);
      EXPECT_EQ(s.top(i).nth(1), 0);
      EXPECT_EQ(s.top(i).nth(2), 1);
    }
  EXPECT_EQ(i, s.size());
}

TEST_F(SimpleStack, traverse)
{
  size_t i = 0;
  auto ret = s.traverse([&i] (auto k) { EXPECT_EQ(k, i); return k == i++; });
  EXPECT_TRUE(ret);
  EXPECT_EQ(i, n);
}

TEST_F(ComplexStack, traverse)
{
  int i = 0;
  auto ret = s.traverse([&i] (const DynList<int> & l)
	     {
	       EXPECT_EQ(l.get_first(), i);
	       EXPECT_EQ(l.get_last(), i);
	       EXPECT_EQ(l.nth(1), 0);
	       EXPECT_EQ(l.nth(2), 1);
	       return l.get_first()  == i++;
	     });
  EXPECT_TRUE(ret);
  EXPECT_EQ(i, n);
}
