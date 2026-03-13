
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

