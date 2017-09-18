
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_splay_tree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


static void print(Splay_Tree<int>::Node *p, int, int) 
{
  cout << p->get_key() << " ";
}

static int r[] = { 776, 846, 736, 515, 528, 677, 404, 629, 879,  762 };

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

  Splay_Tree<int> tree;
  Splay_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  unsigned int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*10.0*rand()/(RAND_MAX+1.0));
      //      value = r[i];
      node = tree.search(value);
      if (node == NULL)
	{
	  insCount++;
	  node = new Splay_Tree<int>::Node (value);
	  tree.insert(node);
	  cout << value << " ";
	}
    }
  cout << endl;

  cout << endl << "Preorder " << endl << endl;
  preOrderRec(tree.getRoot(), print);

  cout << endl << insCount << " Items inserted" << endl;

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
