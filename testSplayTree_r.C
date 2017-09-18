
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_splayTree.H>

using namespace std;

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

  cout << "testSplayTree " << n << " " << t << endl;

  SplayTree<int> tree;
  SplayTree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  unsigned int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == NULL)
	{
	  insCount++;
	  node = new SplayTree<int>::Node (value);
	  tree.insert(node);
	}
    }

  cout << insCount << " Items inserted" << endl;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.remove(value);
      if (node != NULL)
	{
	  delCount++;
	  delete node;
	}
    }
    
  cout << delCount << " Items removed" << endl;

  destroyRec(tree.getRoot());
  cout << "testSplayTree " << n << " " << t << endl;
}
