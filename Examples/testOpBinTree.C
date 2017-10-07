
# include <iostream>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>
# include <opBinTree.H>

using namespace std;
using namespace Aleph;


void print(BinNode<int>* node, int, int)
{
  cout << node->get_key() << " ";
}


int * k;
int counter = 0;

void assign(BinNode<int>* node, int, int)
{
  cout << node->get_key() << " ";
  k[counter++] = node->get_key();
}


double bin_coef(int n, int m)
{
  double result = 1;

  if (m == 0 or n == m)
    return result;

  int nm = n - m;

  int max, min;
  if (nm > m)
    {
      max = nm; min = m;
    }
  else
    {
      max = m; min = nm;
    }

  while (n > max and min > 1)
    {
      result *= (1.0*n) / (1.0*min);
      n--; min--;
    }

  while (n > max)
    {
      result *= n;
      n--;
    }

  while (min > 1)
    {
      result /= min;
      min--;
    }

  return result;
}


int main(int argn, char * argc[])
{
  unsigned int t;
  int n = 10;
  if (argn > 1)
    n = atoi(argc[1]);

  double prob = 0.5;
  if (argn > 2)
    prob = atof(argc[2]);

  t = time(NULL);
  if (argn > 3)
    t = atoi(argc[3]);

  srand(t);

  cout << argc[0] << " " << n << " " << prob << " " << t << " " << endl;

  int keys[n];

  BinTree<int> tree;
  BinTree<int>::Node * node;
  int value;

  for (int i = 0; i < n; i++)
    {
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	}
      while (tree.search(value) not_eq NULL);

      node = new BinTree<int>::Node (value);
      tree.insert(node);
    }

  /* Coloca las claves del arbol en el arreglo keys */
  k = &keys[0];
  inOrderRec(tree.getRoot(), assign);
  cout << endl;

  /* construye arreglo de probabilidades segun dist binomial */
  double p[n];
  for (int i = 0; i < n; i++)
    {
      p[i] = bin_coef(n, i) * pow(prob, i) * pow((1 - prob), n - i);
      printf("%.5f ", p[i]);
    }

  BinNode<int> * optimal_tree = 
    build_optimal_tree<BinNode<int>, int>(keys, p, n);

  cout << endl;
  preOrderRec(optimal_tree, print); cout << endl;

  destroyRec(tree.getRoot());
  destroyRec(optimal_tree);
}
