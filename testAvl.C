
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_avl.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

static void print_pair(Avl_Tree<int>::Node *p, int, int) 
{
  cout << "(" << p->get_key() << "," << (int) (p->getDiff()) << ") ";
}

static void print(Avl_Tree<int>::Node *p, int, int) 
{
  cout << p->get_key() << " ";
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

  cout << "testAvl " << n << " " << t << endl;

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
	}
      while (node not_eq NULL);

      cout << value << " ";
      node = new Avl_Tree<int>::Node (value);
      tree.insert(node);
    }
  cout << endl << "verifying avl tree after insertions ... " 
       << endl;
  assert(is_avl(tree.getRoot()));
  cout << " done" << endl;

  cout << "start-prefix ";
  preOrderRec(tree.getRoot(), print);
  cout << endl << endl;

  cout << endl
       << "start-infix ";
  inOrderRec(tree.getRoot(), print_pair);
  cout << endl << endl;
    
  
  for (i = 0; i < n/2; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	} 
      while (node == NULL);

      cout << value << " ";
      delete node;
    }

  cout << endl
       << "start-prefix ";
  preOrderRec(tree.getRoot(), print);
  cout << endl << endl;
    
  cout << endl << "verifying avl tree after deletions ... " ;
  assert(is_avl(tree.getRoot()));
  cout << " done" << endl;

  destroyRec(tree.getRoot());
}

