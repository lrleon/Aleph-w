
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
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <random>
#include <cerrno>
#include <climits>
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
      errno = 0;
      const long parsed_n = strtol (argv[1], &endptr, 10);
      if (errno != 0 or endptr == argv[1] or *endptr != '\0' or 
          parsed_n <= 0 or parsed_n > 1000000)
	{
	  cerr << "Invalid value for n: " << argv[1] << " (must be between 1 and 1,000,000)" << endl;
	  exit (1);
	}
      n = static_cast<int>(parsed_n);
    }

  auto t = static_cast<unsigned int> (time (nullptr));
  if (argc > 2)
    {
      if (argv[2][0] == '-')
        {
          cerr << "Invalid value for t (must be non-negative): " << argv[2] << endl;
          exit(1);
        }
      char *endptr = nullptr;
      errno = 0;
      const unsigned long parsed_t = strtoul (argv[2], &endptr, 10);
      if (errno != 0 or endptr == argv[2] or *endptr != '\0' or parsed_t > UINT_MAX)
	{
	  cerr << "Invalid value for t: " << argv[2] << endl;
	  exit (1);
	}
      t = static_cast<unsigned int>(parsed_t);
    }

  std::mt19937 rng(t);
  
  // Safely compute upper bound for random distribution
  const uint64_t upper_64 = static_cast<uint64_t>(n) * 100;
  const int upper = (upper_64 > static_cast<uint64_t>(INT_MAX)) ? 
                     INT_MAX : static_cast<int>(upper_64);
  std::uniform_int_distribution<int> dist(1, upper);

  cout << "testSplayTree " << n << " " << t << endl;

  Splay_Tree_Rk<int> tree;

  cout << "Inserting " << n << " random values in tree ...\n";

  unsigned int insCount = 0;

  while (insCount < static_cast<unsigned int>(n))
    {
      const int value = dist(rng);
      if (Splay_Tree_Rk<int>::Node *node = tree.search (value); node == nullptr)
	{
	  node = new Splay_Tree_Rk<int>::Node (value);
	  tree.insert (node);
	  insCount++;
	}
    }

  cout << insCount << " Items inserted" << endl;

  if (tree.verify ())
    cout << "arbol equilibrado" << endl;
  else
    {
      cout << "error de equilibrio en el arbol" << endl;
      destroyRec (tree.getRoot ());
      return EXIT_FAILURE;
    }

  destroyRec (tree.getRoot ());
  cout << "testSplayTree " << n << " " << t << endl;
}
