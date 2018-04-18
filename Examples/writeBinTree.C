/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


ofstream output("bin-tree-aux.Tree", ios::out); 
ofstream example("example-34-ar-aux.Tree", ios::out); 
ofstream tex("example-34-aux.tex", ios::out);

static void print_key(BinTree<int>::Node *p, int, int) 
{
  output << p->get_key() << " ";
}


static void print_ex(BinTree<int>::Node *p, int, int) 
{
  example << p->get_key() << " ";
}

static void print_tex(BinTree<int>::Node *p, int, int) 
{
  tex << " $" << p->get_key() << "\\ $";
}


int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "writeBinTree " << n << " " << t << endl;

  BinTree<int> tree;
  BinTree<int>::Node *node;
  int i;

  for (i = 0; i < 30; i++)
    {
      do
	{

	  value = (int) (500.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	} while (node not_eq NULL);
      node = new BinTree<int>::Node (value);
      tree.insert(node);
    }

  preOrderRec(tree.getRoot(), print_ex);
  inOrderRec(tree.getRoot(), print_tex);
  destroyRec(tree.getRoot());

  for (i = 0; i < n; i++)
    {
      do
	{

	  value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	} while (node not_eq NULL);
      node = new BinTree<int>::Node (value);
      tree.insert(node);
    }

  preOrderRec(tree.getRoot(), print_key);

  destroyRec(tree.getRoot());
}

