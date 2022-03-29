

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


# include <htlist.H>
# include <ahFunctional.H>

using namespace std;

template <typename T,
	  template <typename> class Container1,
	  template <typename> class Container2>
DynList<T> flaten(const Container1<Container2<T>> & c)
{
  DynList<T> ret;
  for (auto & l : c)
    for (auto & item : l)
      ret.append(item);

  return ret;
}

template <typename T,
	  template <typename> class Container1,
	  template <typename> class Container2,
	  template <typename> class Container3>
DynList<T>
flaten(const Container1<Container2<Container3<T>>> & c)
{
  DynList<T> ret;
  for (auto & l : c)
    ret.append(flaten(l));

  return ret;
}
	  

int main()
{
  DynList<DynList<DynList<DynList<int>>>> l =
    { { {{1, 2, 3}, {4, 5, 6}}, {{7, 8, 9}, {10, 11, 12}} },
      { {{13, 14, 15}, {16, 17, 18}}, {{19, 20, 21}, {22, 23, 24}} }};

  flatten(l);
  
}

