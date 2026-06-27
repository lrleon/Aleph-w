
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
# include <cstdlib>
# include <ctime>
# include <iostream>
# include <random>
# include <memory>
# include <tpl_binNodeUtils.H>
# include <tpl_tree_node.H>


# include <cassert>
using namespace std;
using namespace Aleph;

static mt19937 engine;

int random(int l, int r)
{
  assert(l <= r);
  uniform_int_distribution<int> dist(l, r);
  return dist(engine);
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


    template <class Node>
static void printNode(Node * node, int, int)
{ 
  cout << " " << node->get_key();
}


int main(int argc, char *argv[])
{
  int n = 2;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); }
      catch (...) { n = 2; }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  unsigned int t = std::time(0);

  if (argc > 2)
    {
      try { t = static_cast<unsigned int>(stoul(argv[2])); }
      catch (...) { t = std::time(0); }
    }

  engine.seed(t);

  cout << argv[0] << " " << n << " " << t << endl;

  auto bp_deleter = [](BinNode<int> * p) { destroyRec(p); };
  unique_ptr<BinNode<int>, decltype(bp_deleter)> bp_ptr(random_tree(1, n), bp_deleter);
  BinNode<int> * bp = bp_ptr.get();

  cout << "Prefijo:";
  preOrderRec(bp,  printNode);
  cout << endl << endl;
  
  cout << "Infijo:";
  inOrderRec(bp,  printNode);
  cout << endl << endl;
  
  auto forest_deleter = [](Tree_Node<int> * p) { destroy_forest(p); };
  unique_ptr<Tree_Node<int>, decltype(forest_deleter)> 
    tree_ptr(bin_to_forest< Tree_Node<int>, BinNode<int> > (bp), forest_deleter);
  Tree_Node<int> * tree = tree_ptr.get();

  forest_preorder_traversal(tree, printNode);

  unique_ptr<BinNode<int>, decltype(bp_deleter)> 
    prb_ptr(forest_to_bin< Tree_Node<int>, BinNode<int> > (tree), bp_deleter);
  BinNode<int> * prb = prb_ptr.get();

  assert(areEquivalents(prb, bp));
}
