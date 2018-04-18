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
# include <tpl_sort_utils.H>

using namespace std;

struct Cmp
{
  bool operator () (Dlink * p, Dlink * q) const
  {
    return (static_cast<Dnode<int>*>(p)->get_data() < 
	    static_cast<Dnode<int>*>(q)->get_data());
  }
};


int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Dnode<int> list;

  for (int i = 0; i < n; ++i)
    {
      int value = 1 + (int) (1.0 * n * (rand() / (RAND_MAX + 1.0)));
      
      list.append(new Dnode<int>(value));
    }

  for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";
  cout << endl;

# ifdef nada
  if (Aleph::random_search<int, Cmp>(list, n/2) != NULL)
    cout << n/2 << " se encuentra en la lista" << endl;
  else
    cout << n/2 << " no se encuentra en la lista" << endl;
# endif

  assert(list.check());

  if (Aleph::random_search(list, n/2) != NULL)
    cout << n/2 << " se encuentra en la lista" << endl;
  else
    cout << n/2 << " no se encuentra en la lista" << endl;

  assert(list.check());

  for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";
  cout << endl;

  assert(list.check());

  Dnode<int> * ptr = Aleph::random_select <int> (list, n/2);

  cout << "El elemento " << n/2 << " es: " << ptr->get_data() << endl;

  list.remove_all_and_delete();

}
