
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

#include <iostream>
#include <random>
#include <tpl_splay_treeRk.H>
#include <tpl_binNodeXt.H>

using namespace std;

int
main (int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      char *endptr = nullptr;
      n = (int) strtol (argv[1], &endptr, 10);
      if (*endptr != '\0' or n < 0)
	{
	  cerr << "Invalid value for n: " << argv[1] << endl;
	  exit (1);
	}
    }

  auto t = static_cast<unsigned int> (time (nullptr));
  if (argc > 2)
    {
      char *endptr = nullptr;
      t = static_cast<unsigned int> (strtoul (argv[2], &endptr, 10));
      if (*endptr != '\0')
	{
	  cerr << "Invalid value for t: " << argv[2] << endl;
	  exit (1);
	}
    }

  std::mt19937 rng(t);
  std::uniform_int_distribution<int> dist(1, n * 100);

  cout << "testSplayTree " << n << " " << t << endl;

  Splay_Tree_Rk<int> tree;

  cout << "Inserting " << n << " random values in tree ...\n";

  unsigned int insCount = 0;

  for (int i = 0; i < n; i++)
    {
      const int value = dist(rng);
      if (Splay_Tree_Rk<int>::Node *node = tree.search (value); node == nullptr)
	{
	  insCount++;
	  node = new Splay_Tree_Rk<int>::Node (value);
	  tree.insert (node);
	}
    }

  cout << insCount << " Items inserted" << endl;

  if (tree.verify ())
    cout << "arbol equilibrado" << endl;
  else
    {
      cout << "error de equilibrio en el arbol" << endl;
      return EXIT_FAILURE;
    }

  destroyRec (tree.getRoot ());
  cout << "testSplayTree " << n << " " << t << endl;
}
