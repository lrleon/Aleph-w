
# include <aleph.H>
# include <tpl_rb_tree.H>
# include <stdlib.h>
# include <iostream>
# include <time.h>


using namespace Aleph;

static void printNode(Rb_Tree<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

static void print_color(Rb_Tree<int>::Node* node, int, int pos)
{
  if (COLOR(node) == RED)
    cout << "shadow " << pos << endl;
}

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 2;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  Rb_Tree<int>  tree;
  Rb_Tree<int>::Node *node;
  int value;

  for (i = 0; i < n; i++)
    {
      value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == NULL)
	{
	  cout << value << " ";
	  node = new Rb_Tree<int>::Node (value);
	  tree.insert(node);
	}
    }

  assert(is_red_black_tree(tree.getRoot()));

  cout << endl 
       << "start-prefix ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;
  inOrderRec(tree.getRoot(), print_color);
  cout << endl << endl;

  for (i = 0; i < n/2; i++)
    {
      do
	{
	  value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	}
      while (node == NULL);
      cout << value << " ";
      delete node;
    }

  cout << endl 
       << "start-prefix ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;
  inOrderRec(tree.getRoot(), print_color);

  assert(is_red_black_tree(tree.getRoot()));

  cout << "destroying tree" << endl;

  destroyRec(tree.getRoot());

  cout << "testRb_Tree " << n << " " << t << endl;
}
