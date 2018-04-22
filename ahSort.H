
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
# ifndef AHSORT_H
# define AHSORT_H

# include <ahFunctional.H>
# include <tpl_sort_utils.H>
# include <tpl_dynDlist.H>
# include <htlist.H>

# define List_Sort(List)						\
  template <typename T, class Cmp = Aleph::less<T>> inline		\
    List<T> sort(const List<T> & c, Cmp & cmp)				\
  {									\
    List<T> ret_val = c;						\
    mergesort<List, T, Cmp>(ret_val, cmp);				\
    return ret_val;							\
  }									\
									\
  template <typename T, class Cmp = Aleph::less<T>> inline		\
    List<T> sort(const List<T> & c, Cmp && cmp = Cmp())			\
  {									\
    return sort<T, Cmp>(c, cmp);					\
  }									\
									\
  template <typename T, class Cmp = Aleph::less<T>> inline		\
    List<T> sort(List<T> && c, Cmp & cmp)				\
  {									\
    mergesort<List, T, Cmp>(c, cmp);					\
    return move(c);							\
  }									\
									\
  template <typename T, class Cmp = Aleph::less<T>> inline		\
    List<T> sort(List<T> && c, Cmp && cmp = Cmp())			\
  {									\
    return sort<T, Cmp>(move(c), cmp);					\
  }									\
									\
									\
  template <typename T, class Cmp = Aleph::less<T>> inline		\
    List<T> & in_place_sort(List<T> & c, Cmp & cmp) noexcept		\
  {									\
    mergeinsertsort(c, cmp);						\
    return c;								\
  }									\
									\
  template <typename T, class Cmp = Aleph::less<T>> inline		\
    List<T> & in_place_sort(List<T> & c, Cmp && cmp = Cmp()) noexcept	\
  {									\
    return in_place_sort<T, Cmp>(c, cmp);				\
  }

namespace Aleph
{
  List_Sort(DynList);

  List_Sort(DynDlist);

  template <typename T, class Cmp = Aleph::less<T>> inline
    DynArray<T> sort(const DynArray<T> & a, Cmp && cmp = Cmp()) 
  {
    DynArray<T> ret_val = a;
    quicksort_op(ret_val, cmp);
    return ret_val;
  }

  template <typename T, class Cmp = Aleph::less<T>> inline
    DynArray<T> sort(DynArray<T> && a, Cmp && cmp = Cmp())
  {
    quicksort_op(a, cmp);
    return move(a);
  }

  template <typename T, class Cmp = Aleph::less<T>> inline
    Array<T> sort(const Array<T> & a, Cmp && cmp = Cmp()) noexcept
  {
    Array<T> ret_val = a;
    quicksort_op(ret_val, cmp);
    return ret_val;
  }

  template <typename T, class Cmp = Aleph::less<T>> inline
    Array<T> sort(Array<T> && a, Cmp cmp = Cmp()) 
  {
    quicksort_op(a, cmp);
    return move(a);
  }

  template <typename Container,
	    class Cmp = std::less<typename Container::value_type>> inline
    Container stdsort(const Container & c, Cmp cmp = Cmp())
  {
    Container ret = c;
    sort(ret.begin(), ret.end(), cmp);
    return ret;
  }

  template <typename T, class Cmp = Aleph::less<T>> inline
    DynArray<T> & in_place_sort(DynArray<T> & c, Cmp cmp = Cmp()) noexcept
  {
    quicksort_op(c, cmp);
    return c;
  }

  template <typename T, class Cmp = Aleph::less<T>> inline
    Array<T> & in_place_sort(Array<T> & c, Cmp cmp = Cmp()) noexcept
  {
    quicksort_op(c, cmp);
    return c;
  }


  template <typename T, template <typename> class C>
class Compute_Ranks
{
  using P = std::pair<T, size_t>;

  C<size_t> indexes;

public:

  C<size_t> compute_ranks(const C<T> & c)
  {
    const size_t n = c.size();
    indexes.reserve(n);
    for (size_t i = 0; i < n; ++i)
      indexes(i) = i;
    quicksort_op(indexes, [&c] (auto i1, auto i2) { return c(i1) < c(i2); });
    C<size_t> ret; ret.reserve(n);
    for (size_t i = 0; i < n; ++i)
      ret(indexes(i)) = i;
    return ret;
  }

  template <template <typename Type> class List>
  C<size_t> list_compute_ranks(const List<T> & c)
  {
    C<T> items; 
    size_t n = 0;
    c.for_each([this, &items, &n] (auto k)
	       {
		 items.append(k);
		 indexes.append(n++);
	       });
    quicksort_op(indexes, [&items] (auto i1, auto i2)
		 {
		   return items(i1) < items(i2);
		 });
    C<size_t> ret; ret.reserve(n);
    for (size_t i = 0; i < n; ++i)
      ret(indexes(i)) = i;
    return ret;
  }

  C<P> compute_pair_ranks(const C<T> & c)
  {
    const size_t n = c.size();
    indexes.reserve(n);
    for (size_t i = 0; i < n; ++i)
      indexes(i) = i;
    quicksort_op(indexes, [&c] (auto i1, auto i2) { return c(i1) < c(i2); });
    C<P> ret; ret.reserve(n);
    for (size_t i = 0; i < n; ++i)
      {
	auto idx = indexes(i);
	ret(idx) = P(c(idx), i);
      }
    return ret;
  }

  template <template <typename Type> class List>
  C<P> list_pair_ranks(const List<T> & c)
  {
    C<T> items; // array copy of c for direct access
    size_t n = 0;
    c.for_each([this, &items, &n] (auto k) // copy elements of c to item
	       {
		 items.append(k);
		 indexes.append(n++);
	       });
    quicksort_op(indexes, [&items] (auto i1, auto i2) 
		 {
		   return items(i1) < items(i2);
		 });
    C<P> ret; ret.reserve(n);
    for (size_t i = 0; i < n; ++i)
      {
	auto idx = indexes(i);
	ret(idx) = P(items(idx), i);
      }
    return ret;
  }
};

template <typename T> Array<size_t> ranks(const Array<T> & array)
{
  return Compute_Ranks<T, Array>().compute_ranks(array);
}

template <typename T> DynArray<size_t> ranks(const DynArray<T> & array)
{
  return Compute_Ranks<T, DynArray>().compute_ranks(array);
}

template <typename T>
DynArray<size_t> ranks(const DynList<T> & l)
{
  return Compute_Ranks<T, DynArray>().list_compute_ranks(l);
}

template <typename T>
DynArray<size_t> ranks(const DynDlist<T> & l)
{
  return Compute_Ranks<T, DynArray>().list_compute_ranks(l);
}

template <typename T> auto pair_ranks(const Array<T> & c)
{
  return Compute_Ranks<T, Array>().compute_pair_ranks(c);
}

template <typename T> auto pair_ranks(const DynArray<T> & c)
{
  return Compute_Ranks<T, DynArray>().compute_pair_ranks(c);
}

template <typename T> auto pair_ranks(const DynList<T> & l)
{
  Compute_Ranks<T, DynArray> func;
  return func.list_pair_ranks(l);
}

template <typename T> auto pair_ranks(const DynDlist<T> & l)
{
  Compute_Ranks<T, DynArray> func;
  return func.list_pair_ranks(l);
}

} // end namespace Aleph

# endif // AHSORT_H
