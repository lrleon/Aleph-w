
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
# ifndef AH_SEARCH_H
# define AH_SEARCH_H

# include <ahFunctional.H>
# include <tpl_sort_utils.H>

namespace Aleph
{

  template <typename T, 
	    template <typename> class Container,
	    class Cmp = Aleph::less<T>>
bool contains(const Container<T> & c, const T & item)
{
  return exists(c, /* Lambda */ [&item] (const T & i)
		{
		  return are_equals<T, Cmp>(item, i);
		});
}

}

# endif // AH_SEARCH_H
