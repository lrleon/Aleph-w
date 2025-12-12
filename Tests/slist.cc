#include <gtest/gtest.h>

#include <tpl_slist.H>

using namespace Aleph;
using namespace testing;

TEST(Slist, InsertAndRemoveFirst)
{
  Slist<int> list;
  auto * n1 = new Snode<int>(1);
  auto * n2 = new Snode<int>(2);

  list.insert_first(n1);
  EXPECT_EQ(list.get_first(), n1);

  list.insert_first(n2);
  EXPECT_EQ(list.get_first(), n2);

  auto removed = list.remove_first();
  EXPECT_EQ(removed, n2);
  EXPECT_TRUE(removed->is_empty());
  delete removed;

  removed = list.remove_first();
  EXPECT_EQ(removed, n1);
  delete removed;

  EXPECT_TRUE(list.is_empty());
  EXPECT_THROW(list.remove_first(), std::underflow_error);
}

TEST(Slist, IteratorTraversesAllNodes)
{
  Slist<int> list;
  for (int i = 0; i < 5; ++i)
    list.insert_first(new Snode<int>(i));

  int count = 0;
  for (Slist<int>::Iterator it(list); it.has_curr(); it.next())
    {
      ASSERT_NE(it.get_curr(), nullptr);
      ++count;
    }
  EXPECT_EQ(count, 5);

  while (not list.is_empty())
    delete list.remove_first();
}
