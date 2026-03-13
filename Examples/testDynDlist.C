
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

int main(int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try
        {
          n = stoi(argv[1]);
        }
      catch (...)
        {
          n = 1000;
        }
    }

  int m = 1000;
  if (argc > 2)
    {
      try
        {
          m = stoi(argv[2]);
        }
      catch (...)
        {
          m = 1000;
        }
    }

  // Validate inputs
  if (n <= 0 or m <= 0)
    {
      cerr << "Error: n and m must be positive integers." << endl;
      return 1;
    }

  unsigned int t = std::time(0);

  if (argc > 3)
    {
      try
        {
          t = stoi(argv[3]);
        }
      catch (...)
        {
          t = std::time(0);
        }
    }

  srand(t);

  cout << argv[0] << " " << n << " " << m << " " << t << endl;

  DynDlist< DynDlist<int>> list;
  int i;

  for (i = 0; i < n; i++)
    {
      DynDlist<int> & l = list.insert(DynDlist<int>());

      for (int k = 0; k < m; ++k)
	l.append(k);
    }

  imprime(list);

  // Guard against empty list before calling get_first()
  if (list.is_empty())
    {
      cerr << "Error: List is empty, cannot get first element." << endl;
      return 1;
    }

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
    // expected output: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 
    l1.for_each([] (auto i) { cout << i << " "; }); cout << endl;
    // assert(l2.is_empty() and l2.size() == 0);
    int i = 0;
    assert(l1.all([&i] (auto k) { return k == i++; }));
  }

}

