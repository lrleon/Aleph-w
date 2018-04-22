
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
# include <htlist.H>
# include <filter_iterator.H>

using namespace std;

using namespace Aleph;

void print(HTList & l)
{
  cout << "l: ";
  for (HTList::Iterator it(l); it.has_curr(); it.next())
    {
      Snodenc<int> * p = (Snodenc<int>*) it.get_curr();
      cout << p->get_data() << " ";
    }
  cout << endl;
}

struct Par
{
  bool operator () (int i) const
  {
    return i % 2 == 0;
  }
};

typedef Filter_Iterator<DynList<int>, DynList<int>::Iterator, Par> It;


int main(int argn, char * argc[])
{
  size_t n = 10;
  if (argn > 1)
    n = atoi(argc[1]);

  HTList list;

  for (int i = 0; i < n; ++i)
    list.append(new Snodenc<int>(i));

  while (not list.is_empty())
    delete list.remove_first();


  for (int i = 0; i < n; ++i)
    list.append(new Snodenc<int>(i));

  HTList l1, l2;

  size_t sz = list.split_list(l1, l2);

  cout << "Lista de " << sz << " elementos partida en dos" << endl;
  print(l1);
  cout << endl;

  print(l2);
  cout << endl;

  list.append(l2);
  print(list);
  list.insert(l1);

  print(list);

  list.reverse();
  print(list);

  HTList::Iterator it(list);
  for (int i = 0; it.has_curr() and i < n/10; it.next(), ++i)
    ;

  list.cut(it.get_curr(), l1);

  print(list);
  cout << endl;

  print(l1);
  cout << endl;

  list.remove_all_and_delete();
  l1.remove_all_and_delete();

  cout << "****************" << endl;

  DynList<int> l = {7};

  for (int i = 0; i < n; ++i)
    l.append(i);

  DynList<int> ll = {2};

  ll = l ;

  cout << "Mostrando todos los elementos .. " << endl;
  for (DynList<int>::Iterator it(ll); it.has_curr(); it.next())
    cout << it.get_curr() << " " ;
  cout << endl;

  for (It it(ll); it.has_curr(); it.next())
     cout << it.get_curr() << " " ;
  cout << endl;

  DynList<int> lll = l;

  lll.reverse();

  l.append(l);

  l.append(ll);

  l.append(DynList<int>(l));

  l.insert(DynList<int>(lll));

  print(l);
}
