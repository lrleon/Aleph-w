
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef ARRAY_UTILS_H
# define ARRAY_UTILS_H

# include <cstdlib>
# include <cassert>
# include <stdexcept>
# include <iostream>
using namespace std;

namespace Aleph
{

template <class Tarray> inline
void open_gap(Tarray & ptr, size_t n, size_t pos = 0, size_t num_entries = 1)
{
  if (pos >= n)
    throw std::out_of_range("pos is greater than n");

  const long navail = long(n) - pos;
  if (long(num_entries) > navail)
    throw std::out_of_range("num_entries is greater than number"
			    " of available entries");

  for (long k = n - num_entries - 1, i = n - 1; k >= long(pos); --i, --k)
    ptr[i] = move(ptr[k]);
}

template <typename T> inline
void close_gap(T * ptr, size_t n, size_t pos, size_t num_entries = 1)
{
  assert(ptr);

  if (pos >= n)
    throw std::out_of_range("pos is greater than n");

  const long navail = n - pos;
  if (long(num_entries) > navail)
    throw std::out_of_range("num_entries is greater than pos");

  for (size_t i = pos; i < n - num_entries; ++i)
    ptr[i] = move(ptr[i + num_entries]);
}

  template <typename T> inline
void reverse(T * ptr, const size_t n)
{
  for (size_t i = 0; i < n/2; ++i)
    swap(ptr[i], ptr[n - i - 1]);
}

  template <typename T> inline
void rotate_left(T * ptr, const size_t n, size_t m)
{
  if (m > n)
    m = m % n;

  reverse(ptr, m);
  reverse(ptr + m, n - m);
  reverse(ptr, n);
}

    template <typename T> inline
void rotate_right(T * ptr, const size_t n, size_t m)
{
  if (m > n)
    m = m % n;

  reverse(ptr, n);
  reverse(ptr, m);
  reverse(ptr + m, n - m);
}
  
} // namespace Aleph

# endif
