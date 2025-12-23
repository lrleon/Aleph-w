
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
# include <tpl_binNodeUtils.H>
# include <tpl_tree_node.H>
# include <generate_tree.H>
 # include <ah-errors.H>

using namespace std;
using namespace Aleph;

void deway(Tree_Node<int> * p, int prefix[], const int & len, 
	   const size_t & dim)
{
  int i = 1;

  if (len == 0)
    cout << "Root ";
  else
    cout << "Node " << prefix[i++];
  
  while (i <= len)
    cout << "." << prefix[i++];

  cout << " \"" << p->get_key() << "\"" << endl;
  
  if (len >= dim)
    ah_overflow_error_if(true) << "Array dimension es smaller than Deway chain";
  
  Tree_Node<int> * child = p->get_left_child(); 
  
  for (int i = 0; child != NULL; ++i, child = child->get_right_sibling())
    {
      prefix[len + 1] = i;
      deway(child, prefix, len + 1, dim);
    }
}

struct Convert
{
  const string operator () (Tree_Node<int> * p)
  {
    char buf[512];
    snprintf(buf, 512, "%d", p->get_key());
    
    return string(buf);
  }
};


void deway(Tree_Node<int> * p, const int & h)
{
  const size_t dim = 10*h;

  int * prefix = new int [dim];

  for (int i = 0; p != NULL; ++i, p = p->get_right_sibling())
    {
      prefix[0] = i;
      deway(p, prefix, 0, dim);
    }

  delete [] prefix;
}


    template <class Node>
static void printNode(Node * node, int, int)
{ 
  cout << " " << node->get_key();
}


int random(int l, int r)
{
  assert(l <= r);

  const int n = r - l;

  const int rd = 1 + static_cast<int>(1.0 * n * rand() / (RAND_MAX + 1.0));

  return l + rd - 1;
}


BinNode<int> * random_tree(int l, int r)
{
  if (l > r)
    return nullptr;

  auto * root = new BinNode<int> (random(l, r));

  LLINK(root) = random_tree(l, KEY(root) - 1);
  RLINK(root) = random_tree(KEY(root) + 1, r);

  return root;
}


int main(int argn, char * argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 2;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinNode<int> * bp = random_tree(1, n);

  cout << "Prefijo:";
  preOrderRec(bp,  printNode);
  cout << endl << endl;
  
  cout << "Infijo:";
  inOrderRec(bp,  printNode);
  cout << endl << endl;
  
  Tree_Node<int> * tree = bin_to_forest< Tree_Node<int>, BinNode<int> > (bp);

  cout << "Preorder:" << endl;
  forest_preorder_traversal(tree, printNode);
  cout << endl << endl;

  cout << "Postorder:" << endl;
  forest_postorder_traversal(tree, printNode);
  cout << endl << endl;

  BinNode<int> * prb = forest_to_bin< Tree_Node<int>, BinNode<int> > (tree);

  assert(areEquivalents(prb, bp));

  deway(tree, computeHeightRec(bp));

  destroyRec(bp);
  destroyRec(prb);
  destroy_forest(tree);
}
