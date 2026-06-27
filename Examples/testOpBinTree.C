
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

# include <iostream>
# include <vector>
# include <string>
# include <cmath>
# include <ctime>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>
# include <opBinTree.H>

# include <cstdlib>
using namespace std;
# include <cstdlib>
using namespace Aleph;


void print(BinNode<int>* node, int, int)
{
  cout << node->get_key() << " ";
}


static int * k;

void assign(BinTree<int>::Node* node, int, int)
{
  *k++ = node->get_key();
}


double bin_coef(int n, int k)
{
  if (k < 0 or k > n)
    return 0;

  if (k == 0 or k == n)
    return 1;

  if (k > n / 2)
    k = n - k;

  double result = 1;
  int min = k;
  while (k > 0)
    {
      result *= n;
      n--;
      k--;
    }

  while (min > 1)
    {
      result /= min;
      min--;
    }

  return result;
}


int main(int argc, char * argv[])
{
  int n = 10;
  if (argc > 1)
    {
      try { n = std::stoi(argv[1]); }
      catch (...) { n = 10; }
    }

  if (n <= 0)
    {
      cerr << "n must be greater than 0" << endl;
      return 1;
    }

  double prob = 0.5;
  if (argc > 2)
    {
      try { prob = std::stod(argv[2]); }
      catch (...) { prob = 0.5; }
    }

  unsigned int t = static_cast<unsigned int>(time(nullptr));
  if (argc > 3)
    {
      try { t = static_cast<unsigned int>(std::stoul(argv[3])); }
      catch (...) { t = static_cast<unsigned int>(time(nullptr)); }
    }

  srand(t);

  cout << argv[0] << " " << n << " " << prob << " " << t << " " << endl;

  std::vector<int> keys(static_cast<size_t>(n));

  BinTree<int> tree;
  BinTree<int>::Node * node;
  int value;

  for (int i = 0; i < n; i++)
    {
      do
	{
	  value = static_cast<int>(10.0 * n * rand() / (RAND_MAX + 1.0));
	}
      while (tree.search(value) not_eq nullptr);

      node = new BinTree<int>::Node (value);
      tree.insert(node);
    }

  /* Coloca las claves del arbol en el arreglo keys */
  k = keys.data();
  inOrderRec(tree.getRoot(), assign);
  cout << endl;

  /* construye arreglo de probabilidades segun dist binomial */
  std::vector<double> p(static_cast<size_t>(n));
  for (int i = 0; i < n; i++)
    {
      p[static_cast<size_t>(i)] = bin_coef(n, i) * pow(prob, i) * pow((1 - prob), n - i);
      printf("%.5f ", p[static_cast<size_t>(i)]);
    }

  BinNode<int> * optimal_tree = 
    build_optimal_tree<BinNode<int>, int>(keys.data(), p.data(), static_cast<size_t>(n));

  cout << endl;
  preOrderRec(optimal_tree, print); cout << endl;

  destroyRec(tree.getRoot());
  destroyRec(optimal_tree);

  return 0;
}
