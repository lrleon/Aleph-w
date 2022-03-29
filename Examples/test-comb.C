

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
# include <htlist.H>
# include <tpl_dynDlist.H>

using namespace std;


void test()
{
  DynList<char> l1 = { 'a', 'b', 'c' };
  DynList<char> l2 = { 'A', 'B', 'C' };
  DynList<char> l3 = { '1', '2', '3', '4' };
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
