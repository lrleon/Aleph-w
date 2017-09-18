
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_balanceXt.H>


using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;


ofstream outputa("balance-after.Tree", ios::out); 

void print_keya(Node * p, int, int)
{
  outputa << p->get_key() << " ";
}

void print_key(Node * p, int level, int pos)
{
  outputa << p->get_key() << " ";
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 15;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
  Node * root = Node::NullPtr;

  for (i = 0; i < n - 1; i++)
    {
      while (true)
	{
	  value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root, value) == Node::NullPtr)
	    break;
	}
      Node * p = new Node (value); 
      insert_by_key<Node, int>(root, p);
    }


  assert(check_rank_tree(root));

  root = balance_tree(root);

  assert(check_rank_tree(root));

  outputa << "start-prefix " ;
  preOrderRec(root, &print_keya);

  outputa << "start-infix " ;
  inOrderRec(root, &print_key);

  destroyRec(root);
}
