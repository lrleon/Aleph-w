
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

# include <iostream>

# include <ah-comb.H>

using namespace std;
using namespace Aleph;


void test()
{
  DynList<char> l1 = { 'a', 'b', 'c' };
  DynList<char> l2 = { 'A', 'B', 'C' };
  DynList<char> l3 = { '1', '2', '3' };
  DynList<char> l4 = { '5', '6', '7'};

  DynList<DynList<char>> l = { l1, l2, l3, l4 };

  cout << "Permutations:" << endl;
  traverse_perm(l, [] (auto s)
	  {
	    cout << "s = "; s.for_each([] (auto c) { cout << c << " "; });
	    cout << endl;
	    return true;
	  });

  cout << endl
       << "Transpose (out-of-place):" << endl;

  transpose(l).for_each([] (const auto & row)
			{
			  row.for_each([] (auto i) { cout << i << " "; });
			  cout << endl;
			});

  cout << endl
       << "Transpose (in-place):" << endl;
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
