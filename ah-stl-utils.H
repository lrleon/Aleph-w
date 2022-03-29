

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


/// A type that represents a parameter pack of zero or more integers.
template<unsigned... Indexes>
struct index_tuple
{
  /// Generate an index_tuple with an additional element.
  template<unsigned N> using append = index_tuple<Indexes..., N>;
};

/// Unary metafunction that generates an index_tuple containing [0, Size)
template<unsigned Size>
struct make_index_tuple
{
  typedef typename make_index_tuple<Size-1>::type::template append<Size-1>
  type;
};

// Terminal case of the recursive metafunction.
template<> struct make_index_tuple<0u>
{
  typedef index_tuple<> type;
};

template<typename... Types>
using to_index_tuple = typename make_index_tuple<sizeof...(Types)>::type;

// Based on:
//   http://stackoverflow.com/questions/10604794/convert-stdtuple-to-stdarray-c11

template<class Container, typename T, typename... U, unsigned... I>
inline static Container
tuple_to_container(const std::tuple<T, U...> & t, index_tuple<I...>)
{
  return build_container<Container>(std::get<I>(t)...);
}

template<class Container, typename T, typename... U, unsigned... I>
inline Container tuple_to_container(const std::tuple<T, U...> & t)
{
  using IndexTuple = typename make_index_tuple<1 + sizeof...(U)>::type;
  return tuple_to_container<Container>(t, IndexTuple());
}

template<typename T, typename... U>
inline DynList<T> tuple_to_dynlist(const std::tuple<T, U...> & t)
{
  return tuple_to_container<DynList<T>>(t);
}

template<typename T, typename... U>
inline Array<T> tuple_to_array(const std::tuple<T, U...> & t)
{
  return tuple_to_container<Array<T>>(t);
}

template <typename StlContainer>
auto stl_container_to_dynList(const StlContainer & container)
{
  DynList<typename StlContainer::value_type> ret;
  for (const auto & item : container)
    ret.append(item);
  return ret;
}


# endif // AH_STL_UTILS_H
