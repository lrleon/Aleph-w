
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>
# include <stdlib.h>
# include <iostream>
# include <time.h>


using namespace Aleph;

static void printNode(BinTreeVtl<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinTreeVtl<int>  tree;
  BinTreeVtl<int>::Node *node;
  int value;

  cout << "Inserting " << n << " random values in treee ...\n";

  int ins_count = 0;

  for (i = 0; i < n; i++)
    {
      do 
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	  }
      while (node not_eq NULL);

      node = new BinTreeVtl<int>::Node (value);
      tree.insert(node);
      ins_count++;
    }

  assert(tree.verifyBin());
  cout << endl << ins_count << " insertions" << endl
       << "prefijo: " << endl;
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  cout << "sufijo: " << endl;
  postOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  int ipl = compute_path_length(tree.getRoot());
  
  cout << "IPL = " << ipl << endl
       << "EPL = " << ipl + 2*n << endl;

  int del_count = 0;

  for (i = 0; i < n/2; i++)
    {
      while (1)
	{
	  value = 1+(int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	  if (node != NULL)
	    {
	      del_count++;
	      cout << node->get_key() << " ";
	      delete node;
	      break;
	    }
	}
    }

  cout << endl << del_count << " deletions" << endl
       << "prefijo: ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;
  cout << endl;

  assert(tree.verifyBin());

  destroyRec(tree.getRoot());

  cout << "testBinTreeVtl " << n << " " << t << endl;
}

// 1018058241
