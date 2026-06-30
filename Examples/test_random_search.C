
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
# include <tpl_sort_utils.H>

# include <ctime>
# include <cstdlib>
# include <cassert>
using namespace std;

struct Cmp
{
  bool operator () (Dlink * p, Dlink * q) const
  {
    return (static_cast<Dnode<int>*>(p)->get_data() < 
	    static_cast<Dnode<int>*>(q)->get_data());
  }
};


int main(int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); }
      catch (...) { n = 1000; }
    }

  // Validate input to prevent invalid operations
  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  unsigned int t = std::time(0);

  if (argc > 2)
    {
      try { t = static_cast<unsigned int>(stoul(argv[2])); }
      catch (...) { t = std::time(0); }
    }

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  Dnode<int> list;

  for (int i = 0; i < n; ++i)
    {
      int value = 1 + (int) (1.0 * n * (rand() / (RAND_MAX + 1.0)));
      
      list.append(new Dnode<int>(value));
    }

  for (Dnode<int>::Iterator it(list); it.has_curr(); it.next())
    cout << it.get_curr()->get_data() << " ";
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

  for (Dnode<int>::Iterator it(list); it.has_curr(); it.next())
    cout << it.get_curr()->get_data() << " ";
  cout << endl;

  assert(list.check());

  Dnode<int> * ptr = Aleph::random_select <int> (list, n/2);

  cout << "El elemento " << n/2 << " es: " << ptr->get_data() << endl;

  list.remove_all_and_delete();

}
