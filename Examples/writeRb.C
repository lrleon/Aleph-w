
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
# include <string>
# include <fstream>
# include <aleph.H>
# include <tpl_rb_tree.H>
# include <tpl_binNodeUtils.H>

# include <cassert>
using namespace std;
# include <cassert>
using namespace Aleph;


ofstream output;


ofstream file;

static void print_key(const Rb_Tree<int>::Node *p, int, int) 
{
  file << p->get_key() << " ";
}


static void print_color(const Rb_Tree<int>::Node *p, int, int pos) 
{
  if (COLOR(p) == RED)
    file << pos << " ";
}


static void print_key_ex(const Rb_Tree<int>::Node *p, int, int) 
{
  output << p->get_key() << " ";
}


static void print_color_ex(const Rb_Tree<int>::Node *p, int, int pos) 
{
  if (COLOR(p) == RED)
    output << pos << " ";
}


int main(int argc, char *argv[])
{
  int n = 1000;
  unsigned int t = std::time(0);
  int value;

  if (argc > 1)
    {
      try { n = std::stoi(argv[1]); }
      catch (const std::exception & e)
	{
	  cerr << "Warning: could not parse n from '" << argv[1] 
	       << "': " << e.what() << ". Using default n=1000" << endl;
	}
    }

  if (argc > 2)
    {
      try { t = std::stoi(argv[2]); }
      catch (const std::exception & e)
	{
	  cerr << "Warning: could not parse t from '" << argv[2] 
	       << "': " << e.what() << ". Using default t=" << t << endl;
	}
    }

  if (n <= 0)
    {
      cout << "n must be positive" << endl;
      return 1;
    }

  std::srand(t);

  output.open("rb-example-aux.Tree", ios::out);
  if (not output.is_open())
    {
      cerr << "Error: could not open rb-example-aux.Tree for writing" << endl;
      return 1;
    }

  file.open("rb-tree-aux.Tree", ios::out);
  if (not file.is_open())
    {
      cerr << "Error: could not open rb-tree-aux.Tree for writing" << endl;
      return 1;
    }

  cout << "writeRb " << n << " " << t << endl;

  {
    Rb_Tree<int> tree;
    Rb_Tree<int>::Node *node;
    int i;

    for (i = 0; i < 30; i++)
      {
	do
	  {
	    value = (int) (100.0*std::rand()/(RAND_MAX+1.0));
	    node = tree.search(value);
	  } while (node not_eq NULL);
	node = new Rb_Tree<int>::Node (value);
	tree.insert(node);
      }

    assert(is_red_black(tree.getRoot())); 

    preOrderRec<const Rb_Tree<int>::Node>(tree.getRoot(), print_key_ex);
    output << endl << "START-SHADOW "; 
    inOrderRec<const Rb_Tree<int>::Node>(tree.getRoot(), print_color_ex);
    output << endl;
    
    destroyRec(tree.getRoot());
  }

  {
     Rb_Tree<int> tree;
    Rb_Tree<int>::Node *node;
    int i;
    for (i = 0; i < n; i++)
      {
	do
	  {
	    value = (int) (n*10.0*std::rand()/(RAND_MAX+1.0));
	    node = tree.search(value);
	  } while (node not_eq NULL);
	node = new Rb_Tree<int>::Node (value);
	tree.insert(node);
      }

    assert(is_red_black(tree.getRoot())); 

    preOrderRec<const Rb_Tree<int>::Node>(tree.getRoot(), print_key);
    file << "START-SHADOW ";
    inOrderRec<const Rb_Tree<int>::Node>(tree.getRoot(), print_color);
    file << endl;

    destroyRec(tree.getRoot());
  }
}

