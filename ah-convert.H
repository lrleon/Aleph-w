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

/** @file ah-convert.H
    @brief Conversion utilities between Aleph-w container types.

    Provides template functions for converting between different
    Aleph-w container types such as Array, DynList, and DynArray.

    @ingroup Secuencias
 *  @author Leandro Rabindranath León
*/

#ifndef AH_CONVERT_H
#define AH_CONVERT_H

#include <tpl_agraph.H>
#include <tpl_dynArray.H>

namespace Aleph
{

/** @brief Convert an Array to a DynList.

    Creates a new DynList containing copies of all elements from the
    given Array.

    @tparam T The element type.
    @param a The source Array.
    @return A DynList containing copies of all elements from a.

    @ingroup Secuencias
*/
template <class T>
[[nodiscard]] DynList<T> to_DynList(const Array<T> & a)
{
  return a.template maps<T>([] (const auto & item) { return item; });
}

/** @brief Convert a container to an Array.

    Creates a new Array containing copies of all elements from the
    given container. The container must provide a get_it() method
    that returns an iterator with has_curr(), next_ne(), and get_curr()
    methods.

    @tparam C The container type. Must expose Item_Type and provide
              a compatible iterator interface.
    @param c The source container.
    @return An Array containing copies of all elements from c.

    @ingroup Secuencias
*/
template <class C>
[[nodiscard]] Array<typename C::Item_Type> to_Array(const C & c)
{
  Array<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    ret.append(it.get_curr());
  return ret;
}

/** @brief Convert a DynArray to a DynList.

    Creates a new DynList containing copies of all elements from the
    given DynArray.

    @tparam T The element type.
    @param a The source DynArray.
    @return A DynList containing copies of all elements from a.

    @ingroup Secuencias
*/
template <class T>
[[nodiscard]] DynList<T> dynarray_to_DynList(const DynArray<T> & a)
{
  DynList<T> ret;
  for (size_t i = 0; i < a.size(); ++i)
    ret.append(a(i));
  return ret;
}

/** @brief Convert a DynList to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given DynList.

    @tparam T The element type.
    @param l The source DynList.
    @return A DynArray containing copies of all elements from l.

    @ingroup Secuencias
*/
template <class T>
[[nodiscard]] DynArray<T> dynlist_to_DynArray(const DynList<T> & l)
{
  DynArray<T> ret;
  for (auto it = l.get_it(); it.has_curr(); it.next_ne())
    ret.append(it.get_curr());
  return ret;
}

/** @brief Convert a container to a std::vector.

    Creates a new std::vector containing copies of all elements from the
    given Aleph-w container.

    @tparam C The container type. Must expose Item_Type and provide
              a compatible iterator interface.
    @param c The source container.
    @return A std::vector containing copies of all elements from c.

    @ingroup Secuencias
*/
template <class C>
[[nodiscard]] std::vector<typename C::Item_Type> to_vector(const C & c)
{
  std::vector<typename C::Item_Type> ret;
  if constexpr (requires { c.size(); })
    ret.reserve(c.size());
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    ret.push_back(it.get_curr());
  return ret;
}

/** @brief Convert a std::vector to a DynList.

    Creates a new DynList containing copies of all elements from the
    given std::vector.

    @tparam T The element type.
    @param v The source std::vector.
    @return A DynList containing copies of all elements from v.

    @ingroup Secuencias
*/
template <class T>
[[nodiscard]] DynList<T> vector_to_DynList(const std::vector<T> & v)
{
  DynList<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::vector to an Array.

    Creates a new Array containing copies of all elements from the
    given std::vector.

    @tparam T The element type.
    @param v The source std::vector.
    @return An Array containing copies of all elements from v.

    @ingroup Secuencias
*/
template <class T>
[[nodiscard]] Array<T> vector_to_Array(const std::vector<T> & v)
{
  Array<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

} // end namespace Aleph

#endif // AH_CONVERT_H
