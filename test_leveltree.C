# include <string.h>
# include <tpl_binNodeUtils.H>
# include <generate_tree.H>

using namespace Aleph;

BinNode<char*> * build_tree(char * a[], int & i, int n)
{
  if (i >= n)
    return NULL;

  BinNode<char*> * p = new BinNode <char*> (a[i++]);

  if (i >= n) 
    return p;

  //  if (a[i] < a[i - 1]) // próximo hijo derecho?
  if (strcmp(a[i], a[i - 1]) < 0) // próximo hijo derecho?
    RLINK(p) = build_tree(a, i, n);
  else
    { 
      LLINK(p) = build_tree(a, i, n);
      RLINK(p) = build_tree(a, i, n);
    }

  return p;
}

struct W
{
  std::string operator() (BinNode<char*> * p)
  {
    return string(p->get_key());
  }
};


int main(int argn, char * argc[])
{
  if (argn < 2)
    {
      cout << "usage: " << argc[0] << " -level-order-list-" << endl;
      exit(0);
    }

  int i = 1;
  
  BinNode<char*> * r = build_tree(argc, i, argn);

  generate_btree<BinNode<char*>, W>(r, cout);
}
