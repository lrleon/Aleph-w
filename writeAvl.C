
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_avl.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

ofstream file("avl-tree-aux.Tree", ios::out); 


static void print_key(Avl_Tree<int>::Node *p, int, int) 
{
  file << p->get_key() << " ";
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

  cout << "writeAvl " << n << " " << t << endl;

  Avl_Tree<int> tree;
  Avl_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      do
	{

	  value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	} while (node not_eq NULL);
      node = new Avl_Tree<int>::Node (value);
      tree.insert(node);
    }
  cout << endl << "verifying avl tree after insertions ... " 
       << endl;
  assert(is_avl(tree.getRoot()));
  cout << " done" << endl;

  preOrderRec(tree.getRoot(), print_key);

  destroyRec(tree.getRoot());
}

