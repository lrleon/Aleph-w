
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNodeXt.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binTree.H>

using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;


ofstream output("rank-tree-aux.Tree", ios::out); 


void print_key(Node * p, int, int)
{
  output << p->get_key() << " ";
}


void print_count(Node * p, int, int)
{
  output << p->getCount() << " ";
}

void print_tag(Node *, int, int pos)
{
  output << "tag " << pos << " " << pos << " N -15 35" << endl;
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Node * root = Node::NullPtr;

  int value; 

  for (i = 0; i < n; i++)
    {
      while (true)
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root, value) == Node::NullPtr)
	    break;
	}

      Node * p = new Node (value);
      insert_by_key_xt(root, p);
    }

  cout << endl << endl;

  preOrderRec(root, &print_key); cout << endl;

  output << endl 
	 << endl
	 << "START-AUX ";
  inOrderRec(root, &print_count); cout << endl;

  output << endl 
	 << endl
	 << "% Etiquetas de posicion infija" << endl
	 << endl;

  inOrderRec(root, &print_tag); 

  output << endl;

  assert(check_rank_tree(root));
  assert(check_bst(root));

  cout << endl;

  destroyRec(root);
}
