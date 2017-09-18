
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

using namespace Aleph;
using namespace std;

typedef BinNode<int> Node;

ofstream output("join-aux.Tree", ios::out);
ofstream output1("join-1-aux.Tree", ios::out);
ofstream output2("join-2-aux.Tree", ios::out);

void print_key(Node * p, int, int)
{
  output << p->get_key() << " " ;
}


void print_key1(Node * p, int, int)
{
  output1 << p->get_key() << " " ;
}


void print_key2(Node * p, int, int)
{
  output2 << p->get_key() << " " ;
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int value;
  Node * root1 = Node::NullPtr;

  for (i = 0; i < n/2; i++)
    {
      while (true)
	{
	  value = rand();
	  if (searchInBinTree(root1, value) == NULL)
	    break;
	}
      Node * p = new Node (value);
      insert_root(root1, p);
    }

  assert(check_bst(root1));

  preOrderRec(root1, &print_key1); 

  Node * root2 = Node::NullPtr;
  for (i = 0; i < n/2; i++)
    {
      while (true)
	{
	  value = rand();
	  if (searchInBinTree(root1, value) == NULL and
	      searchInBinTree(root2, value) == NULL)
	    break;
	}
      Node * p = new Node (value);
      insert_in_bst(root2, p);
    }

  assert(check_bst(root2));

  preOrderRec(root2, &print_key2);

  Node * dup = NULL;

  Node * root = join(root1, root2, dup);

  assert(dup ==NULL); // no deben haber repetidos

  assert(check_bst(root));

  preOrderRec(root, &print_key);

  destroyRec(root);
}
