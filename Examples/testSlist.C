
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
# include <tpl_slist.H>

using namespace std;

int main()
{
  Slist<int> list;
  Slist<int>::Node *p;

  int i;

  for (i = 0; i < 20; i++)
    {
      p = new Slist<int>::Node (i);
      list.insert_first(p);
    }

  p = list.get_first(); 
  do
    {
      cout << p->get_data() << " ";
      p = p->get_next();
    } while (p not_eq &list);

  cout << endl;

  for (Slist<int>::Iterator itor(list); itor.has_curr(); itor.next())
    {
      p = itor.get_curr();
      cout << p->get_data() << " ";
    }

  try
    {
      while (1)
	{
	  p = list.remove_first();
	  delete p;
	}
    }
  catch (std::underflow_error)
    {
      cout << "List has been emptied"<< endl;
    }
}

  
    
