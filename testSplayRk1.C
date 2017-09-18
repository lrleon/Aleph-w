
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <tpl_splay_treeRk.H>
# include <tpl_binNodeXt.H>


using namespace std;

int main(int argn, char *argc[]) {
  int n = 1000;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testSplayTree " << n << " " << t << endl;

  Splay_Tree_Rk<int> tree;
  Splay_Tree_Rk<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  unsigned int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++) {
    value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
    node = tree.search(value);
    if (node == NULL) {
      insCount++;
      node = new Splay_Tree_Rk<int>::Node (value);
      tree.insert(node);
      }
    }
  
  cout << insCount << " Items inserted" << endl;

  if (Aleph::check_rank_tree(tree.getRoot()))
		cout << "arbol equilibrado" << endl;
	else
		cout << "error de equilibrio en el arbol" << endl;

/*
  for (i = 0; i < n; i++) {
    value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
    node = tree.remove(value);
    if (node != NULL) {
      delCount++;
      delete node;
      }
    }

  cout << delCount << " Items removed" << endl;
*/
  destroyRec(tree.getRoot());
  cout << "testSplayTree " << n << " " << t << endl;
  }
