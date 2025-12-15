#include <gtest/gtest.h>

#include <slink.H>

using namespace Aleph;

namespace {

struct TestNode
{
  int value;
  Slink link;
  SLINK_TO_TYPE(TestNode, link);
};

void push_front(Slink & head, TestNode & node)
{
  node.link.reset();
  head.insert_next(&node.link);
}

} // namespace

TEST(Slink, BasicInsertRemove)
{
  Slink head;
  TestNode n1{1, {}};
  TestNode n2{2, {}};

  ASSERT_TRUE(head.is_empty());
  ASSERT_TRUE(n1.link.is_empty());

  push_front(head, n1);
  EXPECT_FALSE(head.is_empty());
  EXPECT_EQ(head.get_next(), &n1.link);

  push_front(head, n2);
  EXPECT_EQ(head.get_next(), &n2.link);
  EXPECT_EQ(n2.link.get_next(), &n1.link);

  auto removed = head.remove_next();
  EXPECT_EQ(removed, &n2.link);
  EXPECT_TRUE(n2.link.is_empty());
  EXPECT_EQ(head.get_next(), &n1.link);

  removed = head.remove_next();
  EXPECT_EQ(removed, &n1.link);
  EXPECT_TRUE(head.is_empty());
}

TEST(Slink, MacroConversionRetrievesOwningNode)
{
  Slink head;
  TestNode nodes[] = {{10, {}}, {20, {}}, {30, {}}};

  for (auto & node : nodes)
    push_front(head, node);

  int expected[] = {30, 20, 10};
  size_t idx = 0;
  for (Slink * it = head.get_next(); it != &head; it = it->get_next())
    {
      auto * owner = TestNode::slink_to_type(it);
      ASSERT_NE(owner, nullptr);
      EXPECT_EQ(owner->value, expected[idx++]);
    }
  EXPECT_EQ(idx, std::size(expected));

  while (not head.is_empty())
    {
      auto * removed = head.remove_next();
      auto * owner = TestNode::slink_to_type(removed);
      owner->link.reset();
      EXPECT_TRUE(owner->link.is_empty());
    }
  EXPECT_TRUE(head.is_empty());
}

TEST(Slink, ConstGetNextMatchesMutable)
{
  Slink head;
  TestNode node{42, {}};
  push_front(head, node);

  const Slink & cref = head;
  EXPECT_EQ(cref.get_next(), head.get_next());
}
