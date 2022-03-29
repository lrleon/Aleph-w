

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

  DynList & insert(const DynList & list) noexcept(noexcept(DynList(list)))
  {
    DynList tmp = list;
    HTList::insert(tmp);
    return *this;
  }

  /// Obtains a modifiable reference to the i-th item of `this`. Throws
  /// `overflow_error` if `i` is greater or equal to number of elements
  T & get(const size_t & i)
  {
    Iterator it(*this);

    for (size_t __i = 0 ; it.has_curr() and __i < i; it.next_ne(), ++__i);

    return it.get_curr_ne();
  }

  /// \overload get(const size_t & i)
  T & operator [] (const size_t & i)
  {
    return get(i);
  }
};

template <class Container>
inline DynList<typename Container::Item_Type> to_dynlist(const Container & c)
{
  return c.template maps<typename Container::Item_Type>([] (const auto & d)
							{
							  return d;
							});
}

  template <typename T> inline
DynList<T> get_unitarian_DynList(const T & item)
{
  DynList<T> ret;
  ret.append(item);
  return ret;
}

    template <typename T> inline
DynList<T> unitarian_DynList(const T & item)
{
  return DynList<T>({item});
}

# undef NEXT

} // end namespace Aleph


# endif // HTLIST_H
