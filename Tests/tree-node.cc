
# include <gmock/gmock.h>

# include <tpl_tree_node.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(Tree_Node, on_isolated_node)
{
  Tree_Node<int> p;

  ASSERT_TRUE(p.is_root());
  ASSERT_TRUE(p.is_leaf());
  ASSERT_TRUE(p.is_leftmost());
  ASSERT_TRUE(p.is_rightmost());
  ASSERT_TRUE(p.get_child_list()->is_empty());
  ASSERT_TRUE(p.get_sibling_list()->is_empty());

  ASSERT_EQ(p.get_right_child(), nullptr);
  ASSERT_EQ(p.get_left_child(), nullptr);

  ASSERT_EQ(p.get_child(0), nullptr);
  ASSERT_EQ(p.get_parent(), nullptr);

  ASSERT_EQ(p.get_left_tree(), nullptr);
  ASSERT_EQ(p.get_right_tree(), nullptr);

  ASSERT_TRUE(p.children().is_empty());

  ASSERT_TRUE(p.traverse([] (auto) { return true; }));
  ASSERT_FALSE(p.traverse([] (auto) { return false; }));

  Tree_Node<int>::Iterator it = p.get_it();
  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &p);
  ASSERT_NO_THROW(it.next());
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), overflow_error);
  ASSERT_THROW(it.next(), overflow_error);
  it.reset_first();
  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &p);
  ASSERT_NO_THROW(it.next());
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), overflow_error);
  ASSERT_THROW(it.next(), overflow_error);

  Tree_Node<int>::Children_Iterator cit(p);
  ASSERT_FALSE(cit.has_curr());
  ASSERT_THROW(cit.get_curr(), overflow_error);
  ASSERT_THROW(cit.next(), overflow_error);
}

TEST(Tree_Node, simple_tree_construction_and_destruction)
{
  Tree_Node<int> p1 = 1;
  Tree_Node<int> p2 = 2;
  Tree_Node<int> p3 = 3;
  Tree_Node<int> p4 = 4;
  Tree_Node<int> p5 = 5;

  /* 1  insert_leftmost_child() test
     |
     2
  */
  p1.insert_leftmost_child(&p2); 
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_TRUE(p2.is_rightmost());

  /*     1   insert_rightmost_child() test
	/\
       2 3
   */
  p1.insert_rightmost_child(&p3);
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_TRUE(p2.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_FALSE(p2.is_rightmost());
  ASSERT_TRUE(p2.is_leaf());
  ASSERT_FALSE(p3.is_root());
  ASSERT_FALSE(p3.is_leftmost());
  ASSERT_TRUE(p3.is_rightmost());

  /*        0
          / | \
         2  3 5
   */
  p3.insert_right_sibling(&p5);
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_FALSE(p2.is_rightmost());
  ASSERT_TRUE(p2.is_leaf());
  ASSERT_FALSE(p3.is_root());
  ASSERT_FALSE(p3.is_leftmost());
  ASSERT_FALSE(p3.is_rightmost());
  ASSERT_TRUE(p3.is_leaf());
  ASSERT_FALSE(p5.is_leftmost());
  ASSERT_TRUE(p5.is_rightmost());
  ASSERT_FALSE(p5.is_root());
  ASSERT_TRUE(p5.is_leaf());

  /*            1
           /  /  |  | 
	   2 3   4  5
   */
  p5.insert_left_sibling(&p4);
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_FALSE(p2.is_rightmost());
  ASSERT_TRUE(p2.is_leaf());

  ASSERT_FALSE(p3.is_root());
  ASSERT_FALSE(p3.is_leftmost());
  ASSERT_FALSE(p3.is_rightmost());
  ASSERT_TRUE(p3.is_leaf());

  ASSERT_FALSE(p4.is_root());
  ASSERT_FALSE(p4.is_leftmost());
  ASSERT_FALSE(p4.is_rightmost());
  ASSERT_TRUE(p4.is_leaf());

  ASSERT_FALSE(p5.is_leftmost());
  ASSERT_TRUE(p5.is_rightmost());
  ASSERT_FALSE(p5.is_root());
  ASSERT_TRUE(p5.is_leaf());

  ASSERT_EQ(p1.get_left_child(), &p2);
  ASSERT_EQ(p1.get_right_child(), &p5);

  ASSERT_EQ(p2.get_left_sibling(), nullptr);
  ASSERT_EQ(p2.get_right_sibling(), &p3);

  ASSERT_EQ(p3.get_left_sibling(), &p2);
  ASSERT_EQ(p3.get_right_sibling(), &p4);

  ASSERT_EQ(p4.get_left_sibling(), &p3);
  ASSERT_EQ(p4.get_right_sibling(), &p5);

  ASSERT_EQ(p5.get_left_sibling(), &p4);
  ASSERT_EQ(p5.get_right_sibling(), nullptr);

  ASSERT_EQ(p1.get_child(0), &p2);
  ASSERT_EQ(p1.get_child(1), &p3);
  ASSERT_EQ(p1.get_child(2), &p4);
  ASSERT_EQ(p1.get_child(3), &p5);

  int k = 0;
  ASSERT_TRUE(p1.traverse([&k] (auto p) { return p->get_key() == ++k; }));
  ASSERT_EQ(k, 5);
  k = 1;
  ASSERT_TRUE(p1.children_nodes().traverse([&k] (auto p)
					   { return p->get_key() == ++k; }));
  ASSERT_EQ(k, 5);
  k = 1;
  ASSERT_TRUE(p1.children().traverse([&k] (auto i) { return i == ++k; }));
  ASSERT_EQ(k, 5);
}

struct Simple_Tree : public Test
{
  const size_t num_nodes_by_subtree = 5;
  static int key;
  Tree_Node<int> * root = nullptr;
  Simple_Tree()
  {
    root = new Tree_Node<int>(key++);
    for (size_t i = 0; i < num_nodes_by_subtree; ++i, ++key)
      {
	root->insert_rightmost_child(new Tree_Node<int>(key));
      }
    for (Tree_Node<int>::Children_Iterator it(root); it.has_curr(); it.next())
      {
    	auto ptr = it.get_curr();
    	for (size_t i = 0; i < num_nodes_by_subtree; ++i, ++key)
    	  ptr->insert_rightmost_child(new Tree_Node<int>(key));
      }
  }
  ~Simple_Tree()
  {
    destroy_tree(root);
  }
};

int Simple_Tree::key = 0;

TEST(Tree_Node, Iterator_on_extreme_cases)
{
  {
    Tree_Node<int>::Iterator it = nullptr;
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);
  }

  {
    Tree_Node<int> p(0);
    auto it = p.get_it();
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_pos(), 0);
    ASSERT_NO_THROW(it.next());
    ASSERT_FALSE(it.has_curr());
    ASSERT_EQ(it.get_pos(), 1);
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);
  }

  {
    Tree_Node<int> p0(0);
    Tree_Node<int> p1(1);
    p0.insert_leftmost_child(&p1);
    auto it = p0.get_it();
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p0);
    ASSERT_NO_THROW(it.next());
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p1);
    ASSERT_NO_THROW(it.next());
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);

    ASSERT_NO_THROW(it.reset_first());
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p0);
    ASSERT_NO_THROW(it.next());
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p1);
    ASSERT_NO_THROW(it.next());
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);
  }
}

TEST_F(Simple_Tree, Iterators)
{
  DynList<int> l = { 0, 1, 6, 7, 8, 9, 10, 2, 11, 12, 13, 14, 15, 3, 16, 17,
		     18, 19, 20, 4, 21, 22, 23, 24, 25, 5, 26, 27, 28, 29, 30 };
  auto itl = l.get_it();

  size_t k = 0;
  for (auto it = root->get_it(); it.has_curr(); it.next(), itl.next(), ++k)
    ASSERT_EQ(it.get_curr()->get_key(), itl.get_curr());
  ASSERT_GT(k, 0);
}


TEST(Tree_Node, Forrest)
{

}
