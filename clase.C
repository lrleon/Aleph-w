# include <tpl_avl.H>
# include <tpl_rb_tree.H>

template <class Key, template <class> class Tree = Avl_Tree >
class Set
{
  Tree<Key> tree;
};

int main()
{
  Set<int> set1;
  Set<int, Rb_Tree> set2;

}
