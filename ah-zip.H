

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

template <class C, typename ... Lists>
auto zip_lists(const C & c, const Lists & ... lists)
{
  using T = typename C::Item_Type;
  DynList<DynList<T>> ret;
  for (auto it = get_zip_it(c, lists...); it.has_curr(); it.next_ne())
    ret.append(it.get_curr_list());
  return ret;
}

template <class C, typename ... Lists>
auto zip_lists_eq(const C & c, const Lists & ... lists)
{
  using T = typename C::Item_Type;
  DynList<DynList<T>> ret;
  auto it = get_zip_it(c, lists...);
  for (; it.has_curr(); it.next_ne())
    ret. append(it.get_curr_list());

  if (not it.completed())
    throw std::length_error("zip_list_eq: container sizes mismatch");

  return ret;
}



# endif // AH_ZIP_H

