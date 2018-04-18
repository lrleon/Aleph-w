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

# include <aleph.H>
# include <tpl_snode.H>


# define NumItems 10


# include <iostream>

using namespace std;

int main()
{
  Snode<int> head;
  unsigned i;
  Snode<int> *node; 
  Snode<int> *aux;

  for (i = 0; i < NumItems; i++)
    {
      node = new Snode<int> (i);
      head.insert_next(node);
    }

  for (node = head.get_next(); node != &head; node = node->get_next())
    {
      cout << node->get_data() << " ";
    }

  while (not head.is_empty())
    {
      aux = head.remove_next();
      delete aux;
    }

  printf("Ended\n");
}
