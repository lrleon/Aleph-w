
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

# include <ah-comb.H>

using namespace std;


void test()
{
  DynList<char> l1 = { 'a', 'b', 'c' };
  DynList<char> l2 = { 'A', 'B', 'C' };
  DynList<char> l3 = { '1', '2', '3' };
  DynList<char> l4 = { '5', '6', '7'};

  DynList<DynList<char>> l = { l1, l2, l3, l4 };

  traverse_perm(l, [] (auto s)
	  {
	    cout << "s = "; s.for_each([] (auto c) { cout << c << " "; });
	    cout << endl;
	    return true;
	  });

  cout << endl
       << endl;

  transpose(l).for_each([] (const auto & row)
			{
			  row.for_each([] (auto i) { cout << i << " "; });
			  cout << endl;
			});

  auto aux = l;
  in_place_transpose(aux);
  aux.for_each([] (const auto & row)
	       {
		 row.for_each([] (auto i) { cout << i << " "; });
		 cout << endl;
	       });
}


int main()
{
  test();
  return 0;
}
