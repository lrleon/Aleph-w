
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


/** @file ah-convert.H
    @brief Conversion utilities between Aleph-w and STL container types.

    Provides template functions for converting between different
    container types including Aleph-w containers (Array, DynList, DynArray,
    DynDlist) and STL containers (std::vector, std::deque, std::set, std::map).

    ## Key Features

    - **Copy conversions**: All functions create copies of elements
    - **Move conversions**: `vector_to_*` overloads accept rvalue references
    - **Set/Map support**: Convert to/from std::set and std::map
    - **initializer_list**: Create containers from brace-init lists

    ## Available Conversions

    ### Generic Conversions (from any Aleph container)
    | Function | To |
    |----------|-----|
    | `to_Array` | Array |
    | `to_DynArray` | DynArray |
    | `to_vector` | std::vector |
    | `to_deque` | std::deque |
    | `to_set` | std::set |

    ### From std::vector (copy and move)
    | Function | To |
    |----------|-----|
    | `vector_to_DynList` | DynList |
    | `vector_to_Array` | Array |
    | `vector_to_DynArray` | DynArray |
    | `vector_to_DynDlist` | DynDlist |

    ### From std::set
    | Function | To |
    |----------|-----|
    | `set_to_DynList` | DynList |
    | `set_to_Array` | Array |
    | `set_to_DynArray` | DynArray |

    ### From std::map
    | Function | To |
    |----------|-----|
    | `map_to_DynList` | DynList<pair> |
    | `map_to_Array` | Array<pair> |
    | `map_keys_to_DynList` | DynList (keys only) |
    | `map_values_to_DynList` | DynList (values only) |

    ### From initializer_list
    | Function | To |
    |----------|-----|
    | `init_to_DynList` | DynList |
    | `init_to_Array` | Array |
    | `init_to_DynArray` | DynArray |
    | `init_to_DynDlist` | DynDlist |

    ## Usage Examples

    ```cpp
    // From Aleph to STL
    DynList<int> list = {1, 2, 3};
    auto vec = to_vector(list);
    auto s = to_set(list);

    // From STL to Aleph (copy)
    std::vector<int> v = {4, 5, 6};
    auto arr = vector_to_Array(v);

    // From STL to Aleph (move - efficient for large data)
    auto dynList = vector_to_DynList(std::move(v));

    // From initializer_list
    auto arr2 = init_to_Array({1, 2, 3, 4, 5});

    // From std::map
    std::map<std::string, int> m = {{"a", 1}, {"b", 2}};
    auto keys = map_keys_to_DynList(m);
    ```

    @ingroup Sequences
    @author Leandro Rabindranath León
*/

#ifndef AH_CONVERT_H
#define AH_CONVERT_H

#include <deque>
#include <initializer_list>
#include <set>
#include <map>
#include <utility>
#include <tpl_array.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynSetHash.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynMapOhash.H>

namespace Aleph
{

/** @brief Convert an Array to a DynList.

    Creates a new DynList containing copies of all elements from the
    given Array.

    @tparam T The element type.
    @param a The source Array.
    @return A DynList containing copies of all elements from a.

    @ingroup Sequences
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

    @ingroup Sequences
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

    @ingroup Sequences
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

    @ingroup Sequences
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

    @ingroup Sequences
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

/** @brief Convert a std::vector to an Array.

    Creates a new Array containing copies of all elements from the
    given std::vector.

    @tparam T The element type.
    @param v The source std::vector.
    @return An Array containing copies of all elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] Array<T> vector_to_Array(const std::vector<T> & v)
{
  Array<T> ret;
  ret.reserve(v.size());
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

/** @brief Convert a container to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given container. The container must provide a get_it() method
    that returns an iterator with has_curr(), next_ne(), and get_curr().

    @tparam C The container type. Must expose Item_Type and provide
              a compatible iterator interface.
    @param c The source container.
    @return A DynArray containing copies of all elements from c.

    @ingroup Sequences
*/
template <class C>
[[nodiscard]] DynArray<typename C::Item_Type> to_DynArray(const C & c)
{
  DynArray<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    ret.append(it.get_curr());
  return ret;
}

/** @brief Convert an Array to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given Array.

    @tparam T The element type.
    @param a The source Array.
    @return A DynArray containing copies of all elements from a.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynArray<T> array_to_DynArray(const Array<T> & a)
{
  DynArray<T> ret;
  for (size_t i = 0; i < a.size(); ++i)
    ret.append(a(i));
  return ret;
}

/** @brief Convert a DynArray to an Array.

    Creates a new Array containing copies of all elements from the
    given DynArray.

    @tparam T The element type.
    @param a The source DynArray.
    @return An Array containing copies of all elements from a.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] Array<T> dynarray_to_Array(const DynArray<T> & a)
{
  Array<T> ret;
  for (size_t i = 0; i < a.size(); ++i)
    ret.append(a(i));
  return ret;
}

/** @brief Convert a container to a std::deque.

    Creates a new std::deque containing copies of all elements from the
    given Aleph-w container.

    @tparam C The container type. Must expose Item_Type and provide
              a compatible iterator interface.
    @param c The source container.
    @return A std::deque containing copies of all elements from c.

    @ingroup Sequences
*/
template <class C>
[[nodiscard]] std::deque<typename C::Item_Type> to_deque(const C & c)
{
  std::deque<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    ret.push_back(it.get_curr());
  return ret;
}

/** @brief Convert a std::deque to a DynList.

    Creates a new DynList containing copies of all elements from the
    given std::deque.

    @tparam T The element type.
    @param d The source std::deque.
    @return A DynList containing copies of all elements from d.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynList<T> deque_to_DynList(const std::deque<T> & d)
{
  DynList<T> ret;
  for (const auto & item : d)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::deque to an Array.

    Creates a new Array containing copies of all elements from the
    given std::deque.

    @tparam T The element type.
    @param d The source std::deque.
    @return An Array containing copies of all elements from d.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] Array<T> deque_to_Array(const std::deque<T> & d)
{
  Array<T> ret;
  ret.reserve(d.size());
  for (const auto & item : d)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::deque to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given std::deque.

    @tparam T The element type.
    @param d The source std::deque.
    @return A DynArray containing copies of all elements from d.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynArray<T> deque_to_DynArray(const std::deque<T> & d)
{
  DynArray<T> ret;
  for (const auto & item : d)
    ret.append(item);
  return ret;
}

// ==================== DynDlist Conversions ====================

/** @brief Convert a DynDlist to a DynList.

    Creates a new DynList containing copies of all elements from the
    given DynDlist.

    @tparam T The element type.
    @param l The source DynDlist.
    @return A DynList containing copies of all elements from l.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynList<T> dyndlist_to_DynList(const DynDlist<T> & l)
{
  DynList<T> ret;
  l.for_each([&ret](const T & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynList to a DynDlist.

    Creates a new DynDlist containing copies of all elements from the
    given DynList.

    @tparam T The element type.
    @param l The source DynList.
    @return A DynDlist containing copies of all elements from l.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynDlist<T> dynlist_to_DynDlist(const DynList<T> & l)
{
  DynDlist<T> ret;
  l.for_each([&ret](const T & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynDlist to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given DynDlist.

    @tparam T The element type.
    @param l The source DynDlist.
    @return A DynArray containing copies of all elements from l.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynArray<T> dyndlist_to_DynArray(const DynDlist<T> & l)
{
  DynArray<T> ret;
  l.for_each([&ret](const T & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynArray to a DynDlist.

    Creates a new DynDlist containing copies of all elements from the
    given DynArray.

    @tparam T The element type.
    @param a The source DynArray.
    @return A DynDlist containing copies of all elements from a.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynDlist<T> dynarray_to_DynDlist(const DynArray<T> & a)
{
  DynDlist<T> ret;
  for (size_t i = 0; i < a.size(); ++i)
    ret.append(a(i));
  return ret;
}

/** @brief Convert a std::vector to a DynDlist.

    Creates a new DynDlist containing copies of all elements from the
    given std::vector.

    @tparam T The element type.
    @param v The source std::vector.
    @return A DynDlist containing copies of all elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynDlist<T> vector_to_DynDlist(const std::vector<T> & v)
{
  DynDlist<T> ret;
  for (const auto & item : v)
    ret.append(item);
  return ret;
}

// ==================== Move Semantics Conversions ====================

/** @brief Convert a std::vector to a DynList using move semantics.

    Creates a new DynList by moving all elements from the given std::vector.
    After conversion, the source vector's elements are in a valid but
    unspecified state.

    @tparam T The element type.
    @param v The source std::vector (rvalue reference).
    @return A DynList containing moved elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynList<T> vector_to_DynList(std::vector<T> && v)
{
  DynList<T> ret;
  for (auto & item : v)
    ret.append(std::move(item));
  return ret;
}

/** @brief Convert a std::vector to an Array using move semantics.

    Creates a new Array by moving all elements from the given std::vector.
    After conversion, the source vector's elements are in a valid but
    unspecified state.

    @tparam T The element type.
    @param v The source std::vector (rvalue reference).
    @return An Array containing moved elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] Array<T> vector_to_Array(std::vector<T> && v)
{
  Array<T> ret;
  for (auto & item : v)
    ret.append(std::move(item));
  return ret;
}

/** @brief Convert a std::vector to a DynArray using move semantics.

    Creates a new DynArray by moving all elements from the given std::vector.
    After conversion, the source vector's elements are in a valid but
    unspecified state.

    @tparam T The element type.
    @param v The source std::vector (rvalue reference).
    @return A DynArray containing moved elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynArray<T> vector_to_DynArray(std::vector<T> && v)
{
  DynArray<T> ret;
  for (auto & item : v)
    ret.append(std::move(item));
  return ret;
}

/** @brief Convert a std::vector to a DynDlist using move semantics.

    Creates a new DynDlist by moving all elements from the given std::vector.
    After conversion, the source vector's elements are in a valid but
    unspecified state.

    @tparam T The element type.
    @param v The source std::vector (rvalue reference).
    @return A DynDlist containing moved elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynDlist<T> vector_to_DynDlist(std::vector<T> && v)
{
  DynDlist<T> ret;
  for (auto & item : v)
    ret.append(std::move(item));
  return ret;
}

// ==================== std::set Conversions ====================

/** @brief Convert a std::set to a DynList.

    Creates a new DynList containing copies of all elements from the
    given std::set. Elements are inserted in sorted order.

    @tparam T The element type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param s The source std::set.
    @return A DynList containing copies of all elements from s.

    @ingroup Sequences
*/
template <class T, class Compare, class Alloc>
[[nodiscard]] DynList<T> set_to_DynList(const std::set<T, Compare, Alloc> & s)
{
  DynList<T> ret;
  for (const auto & item : s)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::set to an Array.

    Creates a new Array containing copies of all elements from the
    given std::set. Elements are inserted in sorted order.

    @tparam T The element type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param s The source std::set.
    @return An Array containing copies of all elements from s.

    @ingroup Sequences
*/
template <class T, class Compare, class Alloc>
[[nodiscard]] Array<T> set_to_Array(const std::set<T, Compare, Alloc> & s)
{
  Array<T> ret;
  ret.reserve(s.size());
  for (const auto & item : s)
    ret.append(item);
  return ret;
}

/** @brief Convert a std::set to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given std::set. Elements are inserted in sorted order.

    @tparam T The element type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param s The source std::set.
    @return A DynArray containing copies of all elements from s.

    @ingroup Sequences
*/
template <class T, class Compare, class Alloc>
[[nodiscard]] DynArray<T> set_to_DynArray(const std::set<T, Compare, Alloc> & s)
{
  DynArray<T> ret;
  for (const auto & item : s)
    ret.append(item);
  return ret;
}

/** @brief Convert a container to a std::set.

    Creates a new std::set containing copies of all unique elements from
    the given Aleph-w container. Duplicates are automatically removed.

    @tparam C The container type. Must expose Item_Type.
    @param c The source container.
    @return A std::set containing unique elements from c.

    @ingroup Sequences
*/
template <class C>
[[nodiscard]] std::set<typename C::Item_Type> to_set(const C & c)
{
  std::set<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    ret.insert(it.get_curr());
  return ret;
}

// ==================== std::map Conversions ====================

/** @brief Convert a std::map to a DynList of pairs.

    Creates a new DynList containing copies of all key-value pairs from
    the given std::map. Elements are inserted in key-sorted order.

    @tparam K The key type.
    @tparam V The value type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param m The source std::map.
    @return A DynList of pairs containing all key-value pairs from m.

    @ingroup Sequences
*/
template <class K, class V, class Compare, class Alloc>
[[nodiscard]] DynList<std::pair<K, V>>
map_to_DynList(const std::map<K, V, Compare, Alloc> & m)
{
  DynList<std::pair<K, V>> ret;
  for (const auto & [key, value] : m)
    ret.append(std::make_pair(key, value));
  return ret;
}

/** @brief Convert a std::map to an Array of pairs.

    Creates a new Array containing copies of all key-value pairs from
    the given std::map. Elements are inserted in key-sorted order.

    @tparam K The key type.
    @tparam V The value type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param m The source std::map.
    @return An Array of pairs containing all key-value pairs from m.

    @ingroup Sequences
*/
template <class K, class V, class Compare, class Alloc>
[[nodiscard]] Array<std::pair<K, V>>
map_to_Array(const std::map<K, V, Compare, Alloc> & m)
{
  Array<std::pair<K, V>> ret;
  ret.reserve(m.size());
  for (const auto & [key, value] : m)
    ret.append(std::make_pair(key, value));
  return ret;
}

/** @brief Extract keys from a std::map to a DynList.

    Creates a new DynList containing copies of all keys from the
    given std::map. Keys are inserted in sorted order.

    @tparam K The key type.
    @tparam V The value type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param m The source std::map.
    @return A DynList containing all keys from m.

    @ingroup Sequences
*/
template <class K, class V, class Compare, class Alloc>
[[nodiscard]] DynList<K> map_keys_to_DynList(const std::map<K, V, Compare, Alloc> & m)
{
  DynList<K> ret;
  for (const auto & [key, value] : m)
    ret.append(key);
  return ret;
}

/** @brief Extract values from a std::map to a DynList.

    Creates a new DynList containing copies of all values from the
    given std::map. Values are inserted in key-sorted order.

    @tparam K The key type.
    @tparam V The value type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param m The source std::map.
    @return A DynList containing all values from m.

    @ingroup Sequences
*/
template <class K, class V, class Compare, class Alloc>
[[nodiscard]] DynList<V> map_values_to_DynList(const std::map<K, V, Compare, Alloc> & m)
{
  DynList<V> ret;
  for (const auto & [key, value] : m)
    ret.append(value);
  return ret;
}

// ==================== initializer_list Conversions ====================

/** @brief Convert an initializer_list to a DynList.

    Creates a new DynList containing copies of all elements from the
    given initializer_list.

    @tparam T The element type.
    @param il The source initializer_list.
    @return A DynList containing copies of all elements from il.

    @par Example
    @code
    auto list = init_to_DynList({1, 2, 3, 4, 5});
    @endcode

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynList<T> init_to_DynList(std::initializer_list<T> il)
{
  DynList<T> ret;
  for (const auto & item : il)
    ret.append(item);
  return ret;
}

/** @brief Convert an initializer_list to an Array.

    Creates a new Array containing copies of all elements from the
    given initializer_list.

    @tparam T The element type.
    @param il The source initializer_list.
    @return An Array containing copies of all elements from il.

    @par Example
    @code
    auto arr = init_to_Array({1, 2, 3, 4, 5});
    @endcode

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] Array<T> init_to_Array(std::initializer_list<T> il)
{
  Array<T> ret;
  for (const auto & item : il)
    ret.append(item);
  return ret;
}

/** @brief Convert an initializer_list to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given initializer_list.

    @tparam T The element type.
    @param il The source initializer_list.
    @return A DynArray containing copies of all elements from il.

    @par Example
    @code
    auto arr = init_to_DynArray({1, 2, 3, 4, 5});
    @endcode

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynArray<T> init_to_DynArray(std::initializer_list<T> il)
{
  DynArray<T> ret;
  for (const auto & item : il)
    ret.append(item);
  return ret;
}

/** @brief Convert an initializer_list to a DynDlist.

    Creates a new DynDlist containing copies of all elements from the
    given initializer_list.

    @tparam T The element type.
    @param il The source initializer_list.
    @return A DynDlist containing copies of all elements from il.

    @par Example
    @code
    auto list = init_to_DynDlist({1, 2, 3, 4, 5});
    @endcode

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynDlist<T> init_to_DynDlist(std::initializer_list<T> il)
{
  DynDlist<T> ret;
  for (const auto & item : il)
    ret.append(item);
  return ret;
}

// ==================== DynSetTree Conversions ====================

/** @brief Convert a DynSetTree to a DynList.

    Creates a new DynList containing copies of all elements from the
    given DynSetTree. Elements are in tree traversal order (in-order).

    @tparam Key The key type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param s The source DynSetTree.
    @return A DynList containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class Tree, class Compare>
[[nodiscard]] DynList<Key> settree_to_DynList(const DynSetTree<Key, Tree, Compare> & s)
{
  DynList<Key> ret;
  s.for_each([&ret](const Key & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynSetTree to an Array.

    Creates a new Array containing copies of all elements from the
    given DynSetTree. Elements are in tree traversal order (in-order).

    @tparam Key The key type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param s The source DynSetTree.
    @return An Array containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class Tree, class Compare>
[[nodiscard]] Array<Key> settree_to_Array(const DynSetTree<Key, Tree, Compare> & s)
{
  Array<Key> ret;
  ret.reserve(s.size());
  s.for_each([&ret](const Key & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynSetTree to a DynArray.

    Creates a new DynArray containing copies of all elements from the
    given DynSetTree. Elements are in tree traversal order (in-order).

    @tparam Key The key type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param s The source DynSetTree.
    @return A DynArray containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class Tree, class Compare>
[[nodiscard]] DynArray<Key> settree_to_DynArray(const DynSetTree<Key, Tree, Compare> & s)
{
  DynArray<Key> ret;
  s.for_each([&ret](const Key & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynSetTree to a std::vector.

    @tparam Key The key type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param s The source DynSetTree.
    @return A std::vector containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class Tree, class Compare>
[[nodiscard]] std::vector<Key> settree_to_vector(const DynSetTree<Key, Tree, Compare> & s)
{
  std::vector<Key> ret;
  ret.reserve(s.size());
  s.for_each([&ret](const Key & item) { ret.push_back(item); });
  return ret;
}

/** @brief Convert a DynSetTree to a std::set.

    @tparam Key The key type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param s The source DynSetTree.
    @return A std::set containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class Tree, class Compare>
[[nodiscard]] std::set<Key> settree_to_stdset(const DynSetTree<Key, Tree, Compare> & s)
{
  std::set<Key> ret;
  s.for_each([&ret](const Key & item) { ret.insert(item); });
  return ret;
}

/** @brief Convert a container to a DynSetTree (AVL by default).

    Creates a new DynSetTree containing unique elements from the
    given container.

    @tparam C The container type. Must expose Item_Type.
    @param c The source container.
    @return A DynSetTree containing unique elements from c.

    @ingroup Sequences
*/
template <class C>
[[nodiscard]] DynSetTree<typename C::Item_Type> to_DynSetTree(const C & c)
{
  DynSetTree<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    ret.insert(it.get_curr());
  return ret;
}

/** @brief Convert a std::vector to a DynSetTree.

    @tparam T The element type.
    @param v The source std::vector.
    @return A DynSetTree containing unique elements from v.

    @ingroup Sequences
*/
template <class T>
[[nodiscard]] DynSetTree<T> vector_to_DynSetTree(const std::vector<T> & v)
{
  DynSetTree<T> ret;
  for (const auto & item : v)
    ret.insert(item);
  return ret;
}

// ==================== DynSetHash Conversions ====================

/** @brief Convert a DynHashTable (hash set) to a DynList.

    @tparam Key The key type.
    @tparam HashTable The hash table template.
    @tparam Cmp The equality comparator.
    @param s The source DynHashTable.
    @return A DynList containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class HashTable, class Cmp>
[[nodiscard]] DynList<Key> sethash_to_DynList(const DynHashTable<Key, HashTable, Cmp> & s)
{
  DynList<Key> ret;
  s.for_each([&ret](const Key & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynHashTable (hash set) to an Array.

    @tparam Key The key type.
    @tparam HashTable The hash table template.
    @tparam Cmp The equality comparator.
    @param s The source DynHashTable.
    @return An Array containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class HashTable, class Cmp>
[[nodiscard]] Array<Key> sethash_to_Array(const DynHashTable<Key, HashTable, Cmp> & s)
{
  Array<Key> ret;
  ret.reserve(s.size());
  s.for_each([&ret](const Key & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynHashTable (hash set) to a DynArray.

    @tparam Key The key type.
    @tparam HashTable The hash table template.
    @tparam Cmp The equality comparator.
    @param s The source DynHashTable.
    @return A DynArray containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class HashTable, class Cmp>
[[nodiscard]] DynArray<Key> sethash_to_DynArray(const DynHashTable<Key, HashTable, Cmp> & s)
{
  DynArray<Key> ret;
  s.for_each([&ret](const Key & item) { ret.append(item); });
  return ret;
}

/** @brief Convert a DynHashTable (hash set) to a std::vector.

    @tparam Key The key type.
    @tparam HashTable The hash table template.
    @tparam Cmp The equality comparator.
    @param s The source DynHashTable.
    @return A std::vector containing copies of all elements from s.

    @ingroup Sequences
*/
template <typename Key, template <typename, class> class HashTable, class Cmp>
[[nodiscard]] std::vector<Key> sethash_to_vector(const DynHashTable<Key, HashTable, Cmp> & s)
{
  std::vector<Key> ret;
  ret.reserve(s.size());
  s.for_each([&ret](const Key & item) { ret.push_back(item); });
  return ret;
}

// ==================== DynMapTree Conversions ====================

/** @brief Convert a DynMapTree to a DynList of pairs.

    Creates a new DynList containing copies of all key-value pairs from
    the given DynMapTree. Elements are in tree traversal order (in-order).

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param m The source DynMapTree.
    @return A DynList of pairs containing all key-value pairs from m.

    @ingroup Sequences
*/
template <typename Key, typename Data,
          template <typename, class> class Tree, class Compare>
[[nodiscard]] DynList<std::pair<Key, Data>>
maptree_to_DynList(const DynMapTree<Key, Data, Tree, Compare> & m)
{
  DynList<std::pair<Key, Data>> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p); });
  return ret;
}

/** @brief Convert a DynMapTree to an Array of pairs.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param m The source DynMapTree.
    @return An Array of pairs containing all key-value pairs from m.

    @ingroup Sequences
*/
template <typename Key, typename Data,
          template <typename, class> class Tree, class Compare>
[[nodiscard]] Array<std::pair<Key, Data>>
maptree_to_Array(const DynMapTree<Key, Data, Tree, Compare> & m)
{
  Array<std::pair<Key, Data>> ret;
  ret.reserve(m.size());
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p); });
  return ret;
}

/** @brief Convert a DynMapTree to a std::map.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param m The source DynMapTree.
    @return A std::map containing all key-value pairs from m.

    @ingroup Sequences
*/
template <typename Key, typename Data,
          template <typename, class> class Tree, class Compare>
[[nodiscard]] std::map<Key, Data>
maptree_to_stdmap(const DynMapTree<Key, Data, Tree, Compare> & m)
{
  std::map<Key, Data> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) {
    ret.emplace(p.first, p.second);
  });
  return ret;
}

/** @brief Extract keys from a DynMapTree to a DynList.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param m The source DynMapTree.
    @return A DynList containing all keys from m.

    @ingroup Sequences
*/
template <typename Key, typename Data,
          template <typename, class> class Tree, class Compare>
[[nodiscard]] DynList<Key>
maptree_keys_to_DynList(const DynMapTree<Key, Data, Tree, Compare> & m)
{
  DynList<Key> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p.first); });
  return ret;
}

/** @brief Extract values from a DynMapTree to a DynList.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Tree The tree implementation template.
    @tparam Compare The comparison functor.
    @param m The source DynMapTree.
    @return A DynList containing all values from m.

    @ingroup Sequences
*/
template <typename Key, typename Data,
          template <typename, class> class Tree, class Compare>
[[nodiscard]] DynList<Data>
maptree_values_to_DynList(const DynMapTree<Key, Data, Tree, Compare> & m)
{
  DynList<Data> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p.second); });
  return ret;
}

/** @brief Convert a std::map to a DynMapTree.

    @tparam K The key type.
    @tparam V The value type.
    @tparam Compare The comparison function type.
    @tparam Alloc The allocator type.
    @param m The source std::map.
    @return A DynMapTree containing all key-value pairs from m.

    @ingroup Sequences
*/
template <class K, class V, class Compare, class Alloc>
[[nodiscard]] DynMapTree<K, V>
stdmap_to_DynMapTree(const std::map<K, V, Compare, Alloc> & m)
{
  DynMapTree<K, V> ret;
  for (const auto & [key, value] : m)
    ret.insert(key, value);
  return ret;
}

// ==================== MapOpenHash Conversions ====================

/** @brief Convert a MapOpenHash to a DynList of pairs.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Cmp The equality comparator.
    @tparam HashTable The hash table template.
    @param m The source MapOpenHash.
    @return A DynList of pairs containing all key-value pairs from m.

    @ingroup Sequences
*/
template <typename Key, typename Data, class Cmp,
          template <typename, class> class HashTable>
[[nodiscard]] DynList<std::pair<Key, Data>>
maphash_to_DynList(const MapOpenHash<Key, Data, Cmp, HashTable> & m)
{
  DynList<std::pair<Key, Data>> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p); });
  return ret;
}

/** @brief Convert a MapOpenHash to an Array of pairs.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Cmp The equality comparator.
    @tparam HashTable The hash table template.
    @param m The source MapOpenHash.
    @return An Array of pairs containing all key-value pairs from m.

    @ingroup Sequences
*/
template <typename Key, typename Data, class Cmp,
          template <typename, class> class HashTable>
[[nodiscard]] Array<std::pair<Key, Data>>
maphash_to_Array(const MapOpenHash<Key, Data, Cmp, HashTable> & m)
{
  Array<std::pair<Key, Data>> ret;
  ret.reserve(m.size());
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p); });
  return ret;
}

/** @brief Convert a MapOpenHash to a std::map.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Cmp The equality comparator.
    @tparam HashTable The hash table template.
    @param m The source MapOpenHash.
    @return A std::map containing all key-value pairs from m.

    @ingroup Sequences
*/
template <typename Key, typename Data, class Cmp,
          template <typename, class> class HashTable>
[[nodiscard]] std::map<Key, Data>
maphash_to_stdmap(const MapOpenHash<Key, Data, Cmp, HashTable> & m)
{
  std::map<Key, Data> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) {
    ret.emplace(p.first, p.second);
  });
  return ret;
}

/** @brief Extract keys from a MapOpenHash to a DynList.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Cmp The equality comparator.
    @tparam HashTable The hash table template.
    @param m The source MapOpenHash.
    @return A DynList containing all keys from m.

    @ingroup Sequences
*/
template <typename Key, typename Data, class Cmp,
          template <typename, class> class HashTable>
[[nodiscard]] DynList<Key>
maphash_keys_to_DynList(const MapOpenHash<Key, Data, Cmp, HashTable> & m)
{
  DynList<Key> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p.first); });
  return ret;
}

/** @brief Extract values from a MapOpenHash to a DynList.

    @tparam Key The key type.
    @tparam Data The data type.
    @tparam Cmp The equality comparator.
    @tparam HashTable The hash table template.
    @param m The source MapOpenHash.
    @return A DynList containing all values from m.

    @ingroup Sequences
*/
template <typename Key, typename Data, class Cmp,
          template <typename, class> class HashTable>
[[nodiscard]] DynList<Data>
maphash_values_to_DynList(const MapOpenHash<Key, Data, Cmp, HashTable> & m)
{
  DynList<Data> ret;
  m.for_each([&ret](const std::pair<Key, Data> & p) { ret.append(p.second); });
  return ret;
}

} // end namespace Aleph

#endif // AH_CONVERT_H
