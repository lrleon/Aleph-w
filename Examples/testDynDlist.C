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

# include <ahFunctional.H>
# include <htlist.H>
# include <tpl_dynDlist.H>

using namespace std;


void imprime(DynDlist<int> & l)
{
  for (DynDlist<int>::Iterator it(l); it.has_curr(); it.next())
    cout << it.get_curr() << " ";

  cout << "size = " << l.size() << " **" << endl;
}


void imprime(DynDlist< DynDlist<int> > & l)
{
  int i = 0;
  for (DynDlist<DynDlist<int> >::Iterator it(l); it.has_curr(); it.next())
    {
      cout << i++ << " : ";
      imprime(it.get_curr());
      cout << endl;
    }
}

int main(int argn, char *argc[])
{
  int n = 1000;
  if (argn > 1)
    n = atoi(argc[1]);

  int m = 1000;
  if (argn > 2)
    m = atoi(argc[2]);

  unsigned int t = time(0);

  if (argn > 3)
    t = atoi(argc[3]);

  srand(t);

  cout << argc[0] << " " << n << " " << m << " " << t << endl;

  DynDlist< DynDlist<int>> list;
  int i;

  for (i = 0; i < n; i++)
    {
      DynDlist<int> & l = list.insert(DynDlist<int>());

      for (int k = 0; k < m; ++k)
	l.append(k);
    }

  imprime(list);

  //  DynDlist<int> te ( DynDlist<int>(list.get_first()) ); 
  DynDlist<int> te = DynDlist<int>(); 

  te = DynDlist<int>(list.get_first());	

  for (DynDlist<int>::Iterator it(te); it.has_curr(); it.next())
    cout << it.get_curr() << endl;

  //  imprime(list);

  {
    DynDlist<int> l1 = range(10);
    // DynDlist<int> l2 = range(10, 20);
    l1.append(range(10, 20));
    l1.for_each([] (auto i) { cout << i << " "; }); cout << endl;
    // assert(l2.is_empty() and l2.size() == 0);
    int i = 0;
    assert(l1.all([&i] (auto k) { return k == i++; }));
  }

}

