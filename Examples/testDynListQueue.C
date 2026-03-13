
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
# include <ctime>
# include <cstdlib>
# include <cassert>

# include <iostream>
# include <string>
# include <aleph.H>
# include <tpl_dynListQueue.H>

using namespace std;

using namespace Aleph;

# define NumItems 1000

int main(int argc, char *argv[])
{
  int n = NumItems;
  if (argc > 1)
    {
      try
        {
          n = stoi(argv[1]);
        }
      catch (...)
        {
          n = NumItems;
        }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  unsigned int t = std::time(0);

  if (argc > 2)
    {
      try
        {
          t = stoi(argv[2]);
        }
      catch (...)
        {
          t = std::time(0);
        }
    }

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  DynListQueue<int> q;
  
  for (int i = 0; i < n; i++)
    {
      q.put(i);
      cout << q.rear() << " ";
    }

  cout << endl;

  while (not q.is_empty())
    {
      cout << q.front() << " ";
      q.get();
    }

  cout << endl;
}
