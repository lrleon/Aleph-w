
# include <gmock/gmock.h>

# include <htlist.H>

using namespace testing;

struct List_of_5_nodes : public Test
{
  Snodenc<int> list;
  List_of_5_nodes()
  {
    list.insert(new Snodenc<int>(5));
    list.insert(new Snodenc<int>(4));
    list.insert(new Snodenc<int>(3));
    list.insert(new Snodenc<int>(2));
    list.insert(new Snodenc<int>(1));
  }
  ~List_of_5_nodes()
  {
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
  }
};

TEST(Snodenc, conversion_from_slinknc)
{
  {
    Snodenc<int> node = 10;
    Slinknc * ptr = &node;
    EXPECT_THAT(ptr->to_snodenc<int>(), &node);
    EXPECT_THAT(ptr->to_data<int>(), 10);
  }

  {
    const Snodenc<int> node = 10;
    const Slinknc * ptr = &node;
    EXPECT_THAT(ptr->to_snodenc<int>(), &node);
    EXPECT_THAT(ptr->to_data<int>(), 10);
  }
}

TEST_F(List_of_5_nodes, Basic_operations)
{
  EXPECT_THAT(list.get_next()->get_data(), 1);

  int i = 1;
  for (Snodenc<int>::Iterator it = list; it.has_curr(); it.next())
    {
      auto ptr = static_cast<Snodenc<int>*>(it.get_curr());
      EXPECT_EQ(ptr->get_data(), i++);
    }

  auto ptr = list.remove_first();
  EXPECT_EQ(ptr->get_data(), 1);
  EXPECT_TRUE(ptr->is_empty());
  list.insert(ptr);
  EXPECT_EQ(list.get_next()->get_data(), 1);
}
