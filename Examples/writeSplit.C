
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


ofstream output("split-before-aux.Tree", ios::out),
         output_1("split-1-aux.Tree", ios::out),
         output_2("split-2-aux.Tree", ios::out);


void print_key(Node * p, int, int)
{
  output << p->get_key() << " ";
}


void print_key1(Node * p, int, int)
{
  output_1 << p->get_key() << " ";
}


void print_key2(Node * p, int, int)
{
  output_2 << p->get_key() << " ";
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
	  value = (int) (1.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root, value) == Node::NullPtr)
	    break;
	}

      Node * p = new Node (value);
      insert_by_key_xt(root, p);
    }

  cout << endl << endl;

  preOrderRec(root, &print_key); cout << endl;

  assert(check_rank_tree(root));
  assert(check_bst(root));

  cout << endl << endl << "Particionando recursivamente ... " << endl << endl;

  Node * l, * r;

  int num_nodes = root->getCount();

  /* particion recursivamente */
  split_pos_rec(root, num_nodes/2, l, r);
  cout << " ...  listo" << endl;

  output << "SPLIT " << num_nodes/2 << " \"\"" << " \"\""  << endl;

  assert(check_rank_tree(l));
  assert(check_bst(l));

  assert(check_rank_tree(r));
  assert(check_bst(r));

  preOrderRec(l, print_key1); 

  preOrderRec(r, print_key2); 
  cout << endl << endl;

  destroyRec(l);
  destroyRec(r);
}
