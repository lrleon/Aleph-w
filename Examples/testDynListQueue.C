

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
# include <aleph.H>
# include <tpl_dynListQueue.H>

using namespace std;

using namespace Aleph;

# define NumItems 1000

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : NumItems;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  DynListQueue<int> q;
  
  for (i = 0; i < n; i++)
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
