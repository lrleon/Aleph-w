
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file ah-convert-common.H
    @brief Conversions between std::vector and Aleph-w containers shared by
           ah-convert.H and ah-stl-utils.H.

    Both headers offer to_vector(), vector_to_DynList() and
    vector_to_DynArray(). They are defined here once, so that the two
    headers can be included in the same translation unit, in either order.
    Include ah-convert.H or ah-stl-utils.H rather than this header.

    @ingroup Sequences
    @author Leandro Rabindranath León
*/

#ifndef AH_CONVERT_COMMON_H
#define AH_CONVERT_COMMON_H

#include <vector>
#include <htlist.H>
#include <tpl_dynArray.H>

namespace Aleph
{

/** @brief Convert a container to a std::vector.

    Creates a new std::vector containing copies of all elements from the
    given container, in the order of its iterator. The container is
    traversed with get_it() if it has it, as the Aleph-w containers do, and
    with a range-based for loop otherwise.

    @tparam C The container type. Must expose Item_Type and either get_it(),
              whose iterator has has_curr(), next_ne() and get_curr(), or
              begin() and end().
    @param c The source container.
    @return A std::vector containing copies of all elements from c.

    @ingroup Sequences
*/
template <class C>
[[nodiscard]] std::vector<typename C::Item_Type> to_vector(const C & c)
{
  std::vector<typename C::Item_Type> ret;
  if constexpr (requires { c.size(); })
    ret.reserve(c.size());
  if constexpr (requires { c.get_it(); })
    {
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        ret.push_back(it.get_curr());
    }
  else
    {
      for (const auto & item : c)
        ret.push_back(item);
    }
  return ret;
}

/** @brief Convert a std::vector to a DynList.

    Creates a new DynList containing copies of all elements from the
    given std::vector.

    @tparam T The element type.
    @param v The source std::vector.
    @return A DynList containing copies of all elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynList<T> vector_to_DynList(const std::vector<T> & v)
{
  DynList<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::vector to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given std::vector.

    @tparam T The element type.
    @param v The source std::vector.
    @return A DynArray containing copies of all elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynArray<T> vector_to_DynArray(const std::vector<T> & v)
{
  DynArray<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

} // end namespace Aleph

#endif // AH_CONVERT_COMMON_H
