#include <gtest/gtest.h>

#include <tpl_snode.H>

using namespace Aleph;
using namespace testing;

TEST(Snode, BasicInsertAndRemove)
{
  Snode<int> head; // sentinel
  head.reset();

  Snode<int> n1(1);
  Snode<int> n2(2);

  head.insert_next(&n1);
  EXPECT_EQ(head.get_next(), &n1);
  EXPECT_EQ(head.get_next()->get_data(), 1);

  head.insert_next(&n2);
  EXPECT_EQ(head.get_next(), &n2);
  EXPECT_EQ(head.get_next()->get_next(), &n1);

  auto removed = head.remove_next();
  ASSERT_EQ(removed, &n2);
  EXPECT_TRUE(removed->is_empty());

  removed = head.remove_first();
  EXPECT_EQ(removed, &n1);
  EXPECT_TRUE(head.is_empty());
}

TEST(Snode, ConstAccessors)
{
  const Snode<int> node(42);
  EXPECT_EQ(node.get_data(), 42);

  Snode<int> head;
  head.reset();
  head.insert_next(const_cast<Snode<int>*>(&node));
  const Snode<int> & cref = head;
  EXPECT_EQ(cref.get_next(), head.get_next());
}
