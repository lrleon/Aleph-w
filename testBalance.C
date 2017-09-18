
# include <iostream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_balanceXt.H>


using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;

void print_node(Node * p, int, int)
{
  cout << "(" << p->get_key() << "," << p->getCount() << ")" ;
}


void print_key(Node * p, int, int)
{
  cout << p->get_key() << " ";
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
  Node * root = Node::NullPtr;

  for (i = 0; i < n - 1; i++)
    {
      while (1)
	{
	  value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root, value) not_eq Node::NullPtr)
	    continue;

	  Node * p = new Node (value);
	  insert_by_key_xt(root, p);
	  break;
	}
    }

  assert(check_rank_tree(root));

  cout << endl << endl;

  preOrderRec(root, &print_key); cout << endl;

  root = balance_tree(root);

  assert(check_rank_tree(root));

  cout << endl << endl;

  preOrderRec(root, &print_key); cout << endl;

  destroyRec(root);
}
