
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
# include <ahFunctional.H>
# include <tpl_dynArray.H>

using namespace std;

int main()
{
  DynList<int> l = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  DynArray<int> a = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  using It1 = DynList<int>::Iterator;
  using It2 = DynArray<int>::Iterator;

  It2 it2(a);

  for (Pair_Iterator<It1, It2> it{It1(l), It2(a)}; it.has_curr(); it.next())
    ;

  // for (Pair_Iterator<It1, It2> it(It1(l), it2); it.has_curr(); it.next())
    // cout << it.get_curr().first << " " << it.get_curr().second << endl;
}
