/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \|  __/ |_) | | | |_____\ V  V /  version 1.9c
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

#ifndef AH_STL_UTILS_H
#define AH_STL_UTILS_H

/** @file ah-stl-utils.H
    @brief Conversion utilities between Aleph-w containers and STL containers.

    This file provides functions for converting between Aleph-w containers
    (DynList, Array, DynArray) and STL containers (std::vector, std::list),
    as well as utilities for working with tuples.

    @ingroup Algos
 *  @author Leandro Rabindranath León
*/

#include <list>
#include <vector>
#include <tuple>
#include <utility>

#include <htlist.H>
#include <tpl_dynDlist.H>
#include <tpl_array.H>
#include <tpl_dynArray.H>
#include <tpl_dynSetTree.H>

namespace Aleph
{

/** @brief Apply a function to each element in a tuple (tuple first).

    Iterates over all elements in a tuple and applies the given function
    to each one.

    @note This function takes the tuple as the first argument, unlike
          Aleph::for_each_in_tuple in ah-zip.H which takes the operation first.

    @tparam F Callable type.
    @tparam Ts Types in the tuple.
    @tparam Is Index sequence.
    @param tuple The tuple to iterate over.
    @param func Function to apply to each element.
    @param index_sequence Index sequence for unpacking.

    @ingroup Algos
*/
template<class F, class...Ts, std::size_t...Is>
inline void tuple_for_each(const std::tuple<Ts...> & tuple,
                           F func, std::index_sequence<Is...>)
{
  using expander = int[];
  (void)expander { 0, ((void)func(std::get<Is>(tuple)), 0)... };
}

/** @overload tuple_for_each

    Convenience overload that automatically generates the index sequence.

    @tparam F Callable type.
    @tparam Ts Types in the tuple.
    @param tuple The tuple to iterate over.
    @param func Function to apply to each element.

    @ingroup Algos
*/
template<class F, class...Ts>
inline void tuple_for_each(const std::tuple<Ts...> & tuple, F func)
{
  tuple_for_each(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}

/** @brief Convert a std::vector to a DynList.

    @tparam T Element type.
    @param v Source vector.
    @return DynList containing copies of all elements from the vector.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline DynList<T> vector_to_DynList(const std::vector<T> & v)
{
  DynList<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::vector to a DynList (alias).

    @tparam T Element type.
    @param v Source vector.
    @return DynList containing copies of all elements from the vector.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline DynList<T> to_DynList(const std::vector<T> & v)
{
  return vector_to_DynList(v);
}

/** @brief Convert an iterator range to a DynList.

    @tparam Iterator Iterator type (must be dereferenceable).
    @param begin Start iterator.
    @param end End iterator.
    @return DynList containing copies of all elements in the range.

    @ingroup Algos
*/
template <typename Iterator>
[[nodiscard]] inline auto range_to_DynList(Iterator begin, Iterator end)
  -> DynList<std::decay_t<decltype(*begin)>>
{
  DynList<std::decay_t<decltype(*begin)>> ret;
  for (auto it = begin; it != end; ++it)
    ret.append(*it);
  return ret;
}

/** @brief Convert a std::vector to an Array.

    @tparam T Element type.
    @param v Source vector.
    @return Array containing copies of all elements from the vector.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline Array<T> to_Array(const std::vector<T> & v)
{
  Array<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

/** @brief Convert an Aleph-w container to a std::vector.

    Works with any Aleph-w container that exposes Item_Type and supports
    range-based for loops.

    @tparam C Aleph-w container type (must have Item_Type typedef).
    @param container Source container.
    @return std::vector containing copies of all elements.

    @ingroup Algos
*/
template <typename C>
[[nodiscard]] inline std::vector<typename C::Item_Type>
to_vector(const C & container)
{
  std::vector<typename C::Item_Type> ret;
  for (const auto & item : container)
    ret.push_back(item);
  return ret;
}

/** @brief Map a function over a std::vector.

    @tparam T Element type of input vector.
    @tparam Op Callable type.
    @param v Source vector.
    @param op Function to apply to each element.
    @return std::vector with transformed elements.

    @ingroup Algos
*/
template <typename T, class Op>
[[nodiscard]] inline auto map_vector(const std::vector<T> & v, Op op)
  -> std::vector<std::decay_t<decltype(op(v[0]))>>
{
  using ResultType = std::decay_t<decltype(op(v[0]))>;
  std::vector<ResultType> ret;
  ret.reserve(v.size());
  for (const auto & item : v)
    ret.push_back(op(item));
  return ret;
}

/** @brief Convert a variadic parameter pack to a std::vector.

    @tparam T Target element type.
    @tparam Args Argument types (must be convertible to T).
    @param args Arguments to pack into a vector.
    @return std::vector containing all arguments.

    @ingroup Algos
*/
template <typename T, typename... Args>
[[nodiscard]] inline std::vector<T> variadic_to_vector(Args&&... args)
{
  return { static_cast<T>(std::forward<Args>(args))... };
}

/** @brief Convert a variadic parameter pack to a DynList.

    @tparam T Target element type.
    @tparam Args Argument types (must be convertible to T).
    @param args Arguments to pack into a DynList.
    @return DynList containing all arguments.

    @ingroup Algos
*/
template <typename T, typename... Args>
[[nodiscard]] inline DynList<T> variadic_to_DynList(Args&&... args)
{
  return to_DynList<T>(variadic_to_vector<T>(std::forward<Args>(args)...));
}


/* Boost Software License - Version 1.0 - August 17th, 2003 Permission
   is hereby granted, free of charge, to any person or organization
   obtaining a copy of the software and accompanying documentation
   covered by this license (the "Software") to use, reproduce,
   display, distribute, execute, and transmit the Software, and to
   prepare derivative works of the Software, and to permit
   third-parties to whom the Software is furnished to do so, all
   subject to the following:

   The copyright notices in the Software and this entire statement,
   including the above license grant, this restriction and the
   following disclaimer, must be included in all copies of the
   Software, in whole or in part, and all derivative works of the
   Software, unless such copies or derivative works are solely in the
   form of machine-executable object code generated by a source
   language processor.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
   NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE
   DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER
   LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/** @brief A type that represents a parameter pack of zero or more integers.

    Used for compile-time index generation when working with tuples.

    @tparam Indexes The integer indices stored in this type.

    @ingroup Algos
*/
template<unsigned... Indexes>
struct index_tuple
{
  /** @brief Generate an index_tuple with an additional element.

      @tparam N The new index to append.
  */
  template<unsigned N>
  using append = index_tuple<Indexes..., N>;
};

/** @brief Unary metafunction that generates an index_tuple containing [0, Size).

    Recursively builds an index sequence from 0 to Size-1.

    @tparam Size The number of indices to generate.

    @ingroup Algos
*/
template<unsigned Size>
struct make_index_tuple
{
  using type = typename make_index_tuple<Size-1>::type::template append<Size-1>;
};

/// @cond INTERNAL
/// Terminal case of the recursive metafunction.
template<>
struct make_index_tuple<0u>
{
  using type = index_tuple<>;
};
/// @endcond

/** @brief Convenience alias to generate an index tuple from a parameter pack.

    @tparam Types The types whose count determines the index sequence length.

    @ingroup Algos
*/
template<typename... Types>
using to_index_tuple = typename make_index_tuple<sizeof...(Types)>::type;

/// @cond INTERNAL
// Based on:
//   http://stackoverflow.com/questions/10604794/convert-stdtuple-to-stdarray-c11

template<class Container, typename T, typename... U, unsigned... I>
[[nodiscard]] inline static Container
tuple_to_container_impl(const std::tuple<T, U...> & t, index_tuple<I...>)
{
  return build_container<Container>(std::get<I>(t)...);
}
/// @endcond

/** @brief Convert a tuple to an Aleph-w container.

    Converts all elements of a homogeneous tuple to the specified container type.

    @tparam Container Target container type (must support build_container).
    @tparam T First element type (determines container element type).
    @tparam U Remaining element types (must all be T).
    @param t Source tuple.
    @return Container with all tuple elements.

    @ingroup Algos
*/
template<class Container, typename T, typename... U>
[[nodiscard]] inline Container tuple_to_container(const std::tuple<T, U...> & t)
{
  using IndexTuple = typename make_index_tuple<1 + sizeof...(U)>::type;
  return tuple_to_container_impl<Container>(t, IndexTuple());
}

/** @brief Convert a tuple to a DynList.

    @tparam T First element type (determines list element type).
    @tparam U Remaining element types (must all be T).
    @param t Source tuple.
    @return DynList containing all tuple elements.

    @ingroup Algos
*/
template<typename T, typename... U>
[[nodiscard]] inline DynList<T> tuple_to_dynlist(const std::tuple<T, U...> & t)
{
  return tuple_to_container<DynList<T>>(t);
}

/** @brief Convert a tuple to an Array.

    @tparam T First element type (determines array element type).
    @tparam U Remaining element types (must all be T).
    @param t Source tuple.
    @return Array containing all tuple elements.

    @ingroup Algos
*/
template<typename T, typename... U>
[[nodiscard]] inline Array<T> tuple_to_array(const std::tuple<T, U...> & t)
{
  return tuple_to_container<Array<T>>(t);
}

/** @brief Convert any STL container to a DynList.

    Works with any container that exposes value_type and supports range-based
    for loops (std::vector, std::list, std::set, etc.).

    @tparam StlContainer STL container type (must have value_type typedef).
    @param container Source container.
    @return DynList containing copies of all elements.

    @ingroup Algos
*/
template <typename StlContainer>
[[nodiscard]] inline auto stl_container_to_dynList(const StlContainer & container)
  -> DynList<typename StlContainer::value_type>
{
  DynList<typename StlContainer::value_type> ret;
  for (const auto & item : container)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::list to a DynList.

    @tparam T Element type.
    @param l Source std::list.
    @return DynList containing copies of all elements.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline DynList<T> list_to_DynList(const std::list<T> & l)
{
  DynList<T> ret;
  for (const auto & item : l)
    ret.append(item);
  return ret;
}

/** @brief Convert a DynList to a std::list.

    @tparam T Element type.
    @param l Source DynList.
    @return std::list containing copies of all elements.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline std::list<T> DynList_to_list(const DynList<T> & l)
{
  std::list<T> ret;
  for (const auto & item : l)
    ret.push_back(item);
  return ret;
}

/** @brief Convert a DynArray to a std::vector.

    Uses the operator[] which safely returns the default value for
    unallocated entries.

    @tparam T Element type.
    @param arr Source DynArray.
    @return std::vector containing copies of all elements.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline std::vector<T> DynArray_to_vector(const DynArray<T> & arr)
{
  std::vector<T> ret;
  ret.reserve(arr.size());
  for (size_t i = 0; i < arr.size(); ++i)
    ret.push_back(arr[i]);
  return ret;
}

/** @brief Convert a std::vector to a DynArray.

    @tparam T Element type.
    @param v Source vector.
    @return DynArray containing copies of all elements.

    @ingroup Algos
*/
template <typename T>
[[nodiscard]] inline DynArray<T> vector_to_DynArray(const std::vector<T> & v)
{
  DynArray<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

} // end namespace Aleph

// Keep backward compatibility: expose commonly used functions at global scope
using Aleph::tuple_for_each;
using Aleph::vector_to_DynList;
using Aleph::to_DynList;
using Aleph::to_Array;
using Aleph::to_vector;
using Aleph::map_vector;
using Aleph::variadic_to_vector;
using Aleph::variadic_to_DynList;
using Aleph::tuple_to_container;
using Aleph::tuple_to_dynlist;
using Aleph::tuple_to_array;
using Aleph::stl_container_to_dynList;
using Aleph::range_to_DynList;
using Aleph::list_to_DynList;
using Aleph::DynList_to_list;
using Aleph::DynArray_to_vector;
using Aleph::vector_to_DynArray;

#endif // AH_STL_UTILS_H
