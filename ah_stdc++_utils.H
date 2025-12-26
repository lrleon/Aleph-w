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
# ifndef __AH_STDC_UTILS__
# define __AH_STDC_UTILS__

# include <exception>
# include <stdexcept>
 # include <ah-errors.H>

namespace Aleph
{

    template <typename Container, typename Iterator> inline
void verify_container_and_iterator(const Container & container, 
				   const Iterator & itor)
{
  ah_domain_error_if(not itor.verify(container))
    << "Iterator is not set to same container";
}


    template <typename Iterator> inline
void verify_iterators(const Iterator & itor1, const Iterator & itor2)
{
  ah_domain_error_if(not itor1.verify(itor2))
    << "Iterator is not set to same container";
}

    template <typename Container, typename Iterator> inline void 
verify_container_and_iterators
 (const Container & container, const Iterator & itor_container, 
  const Iterator & itor1, const Iterator & itor2)
{
  verify_container_and_iterator(container, itor_container);
  verify_container_and_iterator(itor1, itor2);

  ah_domain_error_if(itor_container.verify(itor1))
    << "All iterators reference the same object";
}

    template <typename Container, typename Iterator> inline void 
verify_container_and_iterators
 (const Container & container, const Iterator & itor1, const Iterator & itor2)
{
  verify_container_and_iterator(container, itor1);
  verify_container_and_iterator(container, itor2);
}


}

# endif // __AH_STDC_UTILS__
