
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

# include <iostream>
# include <vector>
# include <string>
# include <cmath>
# include <ctime>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>
# include <opBinTree.H>

using namespace std;
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
