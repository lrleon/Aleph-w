
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_splay_tree.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;
using namespace std;


ofstream output("splay-tree-aux.Tree", ios::out);



void print_key(Splay_Tree<int>::Node * p, int, int)
{
  output << p->get_key() << " ";
}



int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "writeSplay " << n << " " << t << endl;

  Splay_Tree<int> tree;
  Splay_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      while (true)
	{
	  value = 1+(int) (n*1.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	  if (node == NULL)
	    break;
	}
      node = new Splay_Tree<int>::Node (value);
      tree.insert(node);
    }

  preOrderRec(tree.getRoot(), print_key);

  destroyRec(tree.getRoot());
}
