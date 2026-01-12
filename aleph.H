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

/** @file aleph.H
 *  @brief Core header for the Aleph-w library.
 *
 *  This is the main include file for the Aleph-w library. It provides
 *  fundamental includes for definitions, utilities, and assertions that
 *  are used throughout the library.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef ALEPH_H
# define ALEPH_H

# include <stdexcept>
# include <new>
# include <ahDefs.H>
# include <ahUtils.H>
# include <ahAssert.H>

template <class Itor> inline
void put_itor_at_the_end(Itor & it) noexcept
{
  it.reset_last();
  if (it.has_curr()) // Test for non empty container
    it.next_ne();
}

# endif /* ALEPH_H */

