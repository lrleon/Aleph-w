
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

# include <tpl_binNode.H>
# include <tpl_avl.H>
# include <tpl_binNodeUtils.H>
# include <tpl_arrayQueue.H>
# include <iostream>

using namespace std;
using namespace Aleph;

unsigned int fib(unsigned int n)
{
  if (n == 1 || n == 0)
    return n;

  unsigned int fi_1 = 1, fi_2 = 0, fi = 0, i;

  for (i = 2; i <= n; i++, fi_2 = fi_1, fi_1 = fi)
    fi = fi_1 + fi_2;

  return fi;
}

static int sum = 0;

static void sum_node(BinNode<int> *node, int, int)
{
  node->get_key() = sum++;
}

void build_avl(Avl_Tree<int> &tree, BinNode<int> * p)
{
  ArrayQueue<BinNode<int> *> queue;

  queue.put(p);

  while (not queue.is_empty())
    {
      p = queue.get();

      Avl_Tree<int>::Node * node = new Avl_Tree<int>::Node(KEY(p));

      tree.insert(node);

      if (RLINK(p) not_eq NULL)
	queue.put(RLINK(p));

      if (LLINK(p) not_eq NULL)
	queue.put(LLINK(p));
    }
}


static BinNode<int> * __fibonacci(int k)
{             
  if (k <= 0)
    return NULL;

  if (k == 1)
    return new BinNode<int> (0);

  int label = fib(k + 1);

  BinNode<int> * p = new BinNode<int> (label - 1);

  p->getL() = __fibonacci(k - 2);
  p->getR() = __fibonacci(k - 1);

  return p;
}

static BinNode<int> * fibonacci(int k)
{             
  BinNode<int> * p = __fibonacci(k);

  sum = 0;

  if (k > 2)
    inOrderRec(p, sum_node);

  return p;
}


void print(Avl_Tree<int>::Node *p, int, int)
{
  cout << p->get_key() << " ";
}

int main(int, char *argc[])
{
  const int n = atoi(argc[1]);

  BinNode<int> * p = fibonacci(n);

  Avl_Tree<int> tree;

  build_avl(tree, p);

  assert(is_avl(tree.getRoot()));

  cout << "**** " << fib(n) << endl;
  preOrderRec(tree.getRoot(), &print);
  cout << endl << endl;

  int k = 0;
  //  int k = fib(n + 2) - 2;

  cout << "Borrando " << k;

  Avl_Tree<int>::Node * q = tree.remove(k);

  if (q == NULL)
    cout << " ... no encontrado" << endl;
  else
    {
      cout << " ... borrado" << endl;
      delete q;
    }

  cout << "**** " << fib(n) << endl;
  preOrderRec(tree.getRoot(), &print);
  cout << endl << endl;

  destroyRec(p);
  destroyRec(tree.getRoot());
}
