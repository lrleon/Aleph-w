
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

# include <iostream>
# include <string>
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


int main(int argc, char * argv[])
{
  int n = 10;
  if (argc > 1)
    {
      try
        {
          n = stoi(argv[1]);
        }
      catch (...)
        {
          n = 10;
        }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

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
