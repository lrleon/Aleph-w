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

  for (Slist<int>::Iterator itor(list); itor.has_current(); itor.next())
    {
      p = itor.get_current();
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

  
    
