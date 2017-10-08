
# include <gmock/gmock.h>

# include <htlist.H>
# include <tpl_binNode.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(BinNode, single_node)
{
  BinNode<int> p(10);
  ASSERT_EQ(p.getL(), nullptr);
  ASSERT_EQ(p.getR(), nullptr);
  ASSERT_EQ(p.get_key(), 10);

  BinNode<int> q = p;                // test copy
  ASSERT_EQ(q.getL(), nullptr);
  ASSERT_EQ(q.getR(), nullptr);
  ASSERT_EQ(q.get_key(), 10);
}

TEST(BinNode, incomplete_node_left)
{
  BinNode<int> p(10);
  BinNode<int> q(5);

  p.getL() = &q;

  ASSERT_EQ(p.getL(), &q);
  ASSERT_EQ(p.getR(), nullptr);
  ASSERT_EQ(q.getL(), nullptr);
  ASSERT_EQ(q.getR(), nullptr);
  ASSERT_EQ(p.getL()->get_key(), q.get_key());
  ASSERT_EQ(p.get_key(), 10);
  ASSERT_EQ(q.get_key(), 5);
}

TEST(BinNode, incomplete_node_right)
{
  BinNode<int> p(10);
  BinNode<int> q(15);

  p.getR() = &q;

  ASSERT_EQ(p.getR(), &q);
  ASSERT_EQ(p.getL(), nullptr);
  ASSERT_EQ(q.getL(), nullptr);
  ASSERT_EQ(q.getR(), nullptr);
  ASSERT_EQ(p.getR()->get_key(), q.get_key());
  ASSERT_EQ(p.get_key(), 10);
  ASSERT_EQ(q.get_key(), 15);
}
