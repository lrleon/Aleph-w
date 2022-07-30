
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# include <tpl_arrayQueue.H>

# define NumItems 10

using namespace std;

int main()
{
  FixedQueue<unsigned> q;
  unsigned i;

  for (i = 0; i < NumItems; i++)
    {
      q.putn(1);
      q.rear() = i;
      printf("%u ", i);
    }

  cout << endl
       << "queue = ";
  q.for_each([] (unsigned i) { cout << " " << i; });
  cout << endl;

  cout << "rear de 0: " << q.rear() << endl
       << "rear de 1: " << q.rear(1) << endl
       << "rear de 2: " << q.rear(2)<< endl
       << "rear de 3: " << q.rear(3)<< endl
       << "la cola deberia ser: " << endl;
  while (not q.is_empty())
    cout << q.get() << " ";

  cout << endl;
}
