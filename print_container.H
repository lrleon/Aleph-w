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

/** @file print_container.H
 *  @brief Container printing utilities.
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

namespace Aleph
{

    template <class Container>
void print_container(Container & c)
{
  if (c.size() == 0)
    {
      std::cout << "Container is empty" << '\n';
      return;
    }

  std::cout << "(" <<  c.size() << ") " ;

  typename Container::iterator beg = c.begin();
  const typename Container::iterator end = c.end();

  for (typename Container::iterator it(beg); it != end; /* empty */)
    std::cout << *it++ << " ";

  std::cout << '\n' 
	    << '\n'; 
}


template <class Itor>
void print_range(Itor beg, const Itor & end)
{
  int n = 0;
  while (beg != end)
    {
      std::cout << *beg++ << " ";
      ++n;
    }

  std::cout << '\n' 
	    << "len = " << n << '\n'
	    << '\n'; 
}


}
