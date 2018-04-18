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
# include <aleph.H>
# include <tpl_splay_tree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


static void print(Splay_Tree<int>::Node *p, int, int) 
{
  cout << p->get_key() << " ";
}

static int r[] = { 776, 846, 736, 515, 528, 677, 404, 629, 879,  762 };

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

  cout << "testSplayTree " << n << " " << t << endl;

  Splay_Tree<int> tree;
  Splay_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  unsigned int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*10.0*rand()/(RAND_MAX+1.0));
      //      value = r[i];
      node = tree.search(value);
      if (node == NULL)
	{
	  insCount++;
	  node = new Splay_Tree<int>::Node (value);
	  tree.insert(node);
	  cout << value << " ";
	}
    }
  cout << endl;

  cout << endl << "Preorder " << endl << endl;
  preOrderRec(tree.getRoot(), print);

  cout << endl << insCount << " Items inserted" << endl;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.remove(value);
      if (node != NULL)
	{
	  delCount++;
	  delete node;
	}
    }
    
  cout << delCount << " Items removed" << endl;

  destroyRec(tree.getRoot());
  cout << "testSplayTree " << n << " " << t << endl;
}
