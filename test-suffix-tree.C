
# include <iostream>
# include <tpl_tree_node.H>

using namespace std;


using Node = Tree_Node<long>;

static long count = 0;

ostream & operator << (ostream & out, Node * p)
{
  p->traverse([&out] (auto p) 
	      {
		out << p->get_key() << " ";
		return true;
	      });
  // for (auto it = p->get_it(); it.has_curr(); it.next())
  //   out << it.get_curr()->get_key() << " ";
  return out;
}


Node * test()
{
  Node * leaf1 = new Node(::count++);
  Node * leaf2 = new Node(::count++);
  Node * leaf3 = new Node(::count++);
  Node * leaf4 = new Node(::count++);

  Node * root = new Node(::count++);

  return root->join(leaf1)->join(leaf2)->join(leaf3)->join(leaf4);

  // root->join(leaf1)->join(leaf2);

  // cout << "Root = " << root << endl
  //      << endl;

  // root->join(leaf3);

  // cout << "Root = " << root << endl
  //      << endl;

  // cout << "****";
  // root->mutable_for_each([] (auto p) { cout << " " << p->get_key(); });
  // cout << endl;

  // return root;
}

int main()
{
  auto tree1 = test();
  
  auto tree2 = test();

  cout << "tree1: " << tree1 << endl
       << endl
       << "tree2: " << tree2 << endl
       << endl;

  Node * root = new Node(::count++); 
  root->join(tree1)->join(tree2);

  cout << "root =" << root << endl
       << endl;

  destroy_tree(root);
}
