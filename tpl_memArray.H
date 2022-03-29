

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

      struct Iterator : public Array_Iterator<T>
      {
        using Base = Array_Iterator<T>;
        using Base::Base;

        /// Construct an iterator on array `a`
        Iterator(const MemArray<T> & a) noexcept
          : Array_Iterator<T>(no_exception_ctor, a.ptr, a.dim, a.n)
        {
          assert(a.ptr != nullptr);
        }
      };
    };


} // end namespace Aleph

# endif // TPL_MEMARRAY_H
