
# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


static void printNode(BinNode<int>* node)
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

  DynArray<int> array;

  BinNode<int> *root = new BinNode<int> ((int) (10.0*n*rand()/(RAND_MAX+1.0)));
  cout << root->get_key() << " ";

  array[0] = root->get_key();

  BinNode<int> *p;
  int value;
  int ins_count = 1;

  for (i = 1; i < n; i++)
    {
      do 
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  p = searchInBinTree(root, value);
	  }
      while (p not_eq NULL);
      p = new BinNode<int> (value);
      array[i] = value;
      cout << value << " ";
      insert_in_bst(root, p);
      ins_count++;
    }
  
  cout << endl << ins_count << " items inserted" << endl;
  assert(check_bst(root));

  cout << "prefix: ";
  preOrderThreaded(root, printNode);
  cout << endl;

  // construir el árbol con inserciones en la raíz y verificar si dan lo mismo

  BinNode<int> * root_aux = NULL;

  for (i = ins_count; i > 0; --i)
    root_aux = insert_root(root_aux, new BinNode<int>(array[i - 1]));

  cout << "prefix: ";
  preOrderThreaded(root_aux, printNode);
  cout << endl;

  assert(areEquivalents(root, root_aux));

  destroyRec(root_aux);
  destroyRec(root);
}
