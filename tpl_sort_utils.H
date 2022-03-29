

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

  template <typename T, class Compare = Aleph::less<T>> inline 
  long binary_search(T * a, const T & x, long l, long r,
                     Compare cmp = Compare()) noexcept
  {
    long m;
    while (l <= r)
      {
        m = (l + r) / 2;
        if (cmp(x, a[m]))
          r = m - 1;
        else if (cmp(a[m], x))
          l = m + 1;
        else
          break;
      }
    return m;
  }

}

# endif // TPL_SORT_UTILS_H
