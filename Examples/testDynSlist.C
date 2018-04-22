
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# include <tpl_dynSlist.H>

# define Num_Items 10

using namespace std;

int main()
{
  DynSlist<int> list;
  DynSlist<int*> list_ptr;

  for (int i = 0; i < Num_Items; i++)
    {
      list.insert(i, i);
      list_ptr.insert(i, &i);
    }

  DynSlist<int>::Iterator itor(list);

  DynSlist<int>::Iterator itor_aux(list);

  itor_aux = itor;

  for (/* nothing */; itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

  for (int i = Num_Items; i < 2*Num_Items; i++)
    list.insert(i - Num_Items, i);

  for (itor.reset_first(); itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

  for (int i = 2*Num_Items; i < 3*Num_Items; i++)
    list.insert(0, i);

  for (itor.reset_first(); itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

  for (int i = 3*Num_Items; i < 4*Num_Items; i++)
    list.insert(list.size(), i);

  for (int i = 0; i < list.size(); i++)
    cout << list[i] << " ";

  cout << endl;

  for (int i = 0; i < list.size(); i++)
    cout << list[list.size() - i - 1] << " ";

  cout << endl;

  list.remove(list.size() - 1);

  list.remove(0);

  for (itor.reset_first(); itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

}
