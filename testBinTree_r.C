
# include <stdlib.h>
# include <iostream>
# include <time.h>
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>


using namespace Aleph;

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinTree<int>  tree;
  BinTree<int>::Node *node;
  int value;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      value = (int) (1.0*n*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == NULL)
	{
	  node = new BinTree<int>::Node (value);
	  tree.insert(node);
	}
    }


  assert( check_bst(tree.getRoot()) );

  cout << "Deletion test ...";

  for (i = 0; i < n; i++)
    {
      value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
      node = tree.remove(value);
      if (node)
	delete node;
    }

  assert( check_bst(tree.getRoot()) );

  cout << " passed"<< endl;

  destroyRec(tree.getRoot());
}
