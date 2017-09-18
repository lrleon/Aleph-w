
# include <iostream>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

# define LLINK(p) ((p)->getL())
# define RLINK(p) ((p)->getR())

using namespace Aleph;

class Inorder
{
  int vector[100];
  int current;
  int n;

public:

  Inorder(int _n) : current(0), n(_n)
    {
      for (int i = 0; i < n; i++)
	vector[i] = i;
    }

  int get() 
    {
      assert(current < n);
      return vector[current++]; 
    }

};


BinNode<int>* make(int level, int max_level, Inorder& order)
{
  if (level == max_level)
    {
      
      return new BinNode<int> (order.get());

  BinNode<int> *llink  = make(level + 1, max_level, order);
  BinNode<int> *result = new BinNode<int> (order.get());

  LLINK(result) = llink;
  RLINK(result) = make(level + 1, max_level, order);

  return result;
}

static void printNode(BinNode<int>* node)
{
  cout << node->get_key() << " ";
}

int main()
{
  Inorder vector(12);

  BinNode<int>* root = make(0, 3, vector);

  inOrderRec(root, printNode); 

  cout << endl;

  preOrderRec(root, printNode);

  cout << endl;

  destroyRec(root);
}
