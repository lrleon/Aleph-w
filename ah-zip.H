/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# ifndef AH_ZIP_H
# define AH_ZIP_H 1

# include <type_traits>
# include <tuple>

# include <htlist.H>
# include <ahFunctional.H>

/** 

    @author Leandro Rabindranath Leon
 */
template <class HeadC, class ... TailC>
class ZipIterator : public ZipIterator<TailC...>
{
  typename HeadC::Iterator it;

  using T = typename HeadC::Item_Type;

public:

  ZipIterator(const HeadC & head, const TailC & ...tail)
    : ZipIterator<TailC...>(tail...), it(head) {}

  bool has_curr() const 
  {
    return it.has_curr() and ZipIterator<TailC...>::has_curr();
  }

  bool completed() const
  {
    return not it.has_curr() and ZipIterator<TailC...>::completed();
  }

  auto get_curr() const
  {
    std::tuple<T> curr(it.get_curr());
    return std::tuple_cat(curr, ZipIterator<TailC...>::get_curr());
  }

  using ZipType = std::tuple<T, TailC...>;

  DynList<T> get_curr_list() const
  {
    DynList<T> l = ZipIterator<TailC...>::get_curr_list();
    l.insert(it.get_curr());
    return l;
  }

  void next()
  {
    it.next();
    ZipIterator<TailC...>::next();
  }
};

template <class C>
class ZipIterator<C> : public C::Iterator
{
public:

  using T = typename C::Item_Type;

  ZipIterator(const C & c) : C::Iterator(c) {}

  std::tuple<T> get_curr() const
  {
    return std::tuple<T>(C::Iterator::get_curr());
  }

  DynList<T> get_curr_list() const
  {
    DynList<T> l;
    l.append(C::Iterator::get_curr());
    return l;
  }

  bool completed() const { return not this->has_curr(); }
};

template <class ... Cs> inline
ZipIterator<Cs...> get_zip_it(const Cs & ... cs)
{
  return ZipIterator<Cs...>(cs...);
}

template <class ... Cs> inline
ZipIterator<Cs...> get_zip_it_pos(size_t pos, const Cs & ... cs)
{
  ZipIterator<Cs...> ret(cs...);
  for (size_t i = 0; i < pos; ++i)
    ret.next();
  return ret;
}

template <class ... Cs> inline
ZipIterator<Cs...> zip_it(const Cs & ... cs)
{
  return get_zip_it(cs...);
}

template <class ... Cs> inline
ZipIterator<Cs...> zip_it_pos(size_t pos, const Cs & ... cs)
{
  return get_zip_it_pos(pos, cs...);
}

template <class ... Cs> inline
bool equal_length(const Cs & ... cs)
{
  auto it = get_zip_it(cs...); 
  for (/* already initalized */; it.has_curr(); it.next())
    /* empty */;

  return it.completed();
}

template <class Op, class ... Cs> inline
bool zip_traverse(Op & op, const Cs & ... cs)
{
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    if (not op(it.get_curr()))
      return false;
  return true; 
}

template <class Op, class ... Cs> inline
bool zip_traverse(Op && op, const Cs & ... cs)
{
  return zip_traverse<Op, Cs ...>(cs ..., op);
}

template <class Op, class ... Cs> inline
void zip_for_each(Op & op, const Cs & ... cs)
{
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    op(it.get_curr());
}

template <class Op, class ... Cs> inline
void zip_for_each(Op && op, const Cs & ... cs)
{
  zip_for_each<Op, Cs...>(op, cs ...);
}

template <class Op, class ... Cs> inline
void zip_for_each_eq(Op & op, const Cs & ... cs)
{
  auto it = get_zip_it(cs...);
  for (; it.has_curr(); it.next())
    op(it.get_curr());

  if (not it.completed())
    throw std::domain_error("zip_for_each_eq() containers sizes mismatch");
}

template <class Op, class ... Cs> inline
void zip_for_each_eq(Op && op, const Cs & ... cs)
{
  zip_for_each_eq<Op, Cs...>(op, cs ...);
}

template <class Op, class ... Cs> inline
bool zip_all(Op & op, const Cs & ... cs)
{
  auto it = get_zip_it(cs...);
  for (; it.has_curr(); it.next())
    if (not op(it.get_curr()))
      return false;

  return it.completed();
}

template <class Op, class ... Cs> inline
bool zip_all(Op && op, const Cs & ... cs)
{
  return zip_all(op, cs...);
}
    
template <class Op, class ... Cs> inline
bool zip_exists(Op & op, const Cs & ... cs)
{
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    if (op(it.get_curr()))
      return true;

  return false;
}

template <class Op, class ... Cs> inline
bool zip_exists(Op && op, const Cs & ... cs)
{
  return zip_exists(op, cs...);
}

template <typename T, class Op, class ... Cs>
DynList<T> zip_maps(Op op, const Cs & ... cs)
{
  DynList<T> ret;
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    ret.append(op(it.get_curr()));
  return ret;
}

template <typename T, class Prop, class Op, class ... Cs>
DynList<T> zip_maps_if(Prop prop, Op op, const Cs & ... cs)
{
  DynList<T> ret;
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (prop(t))
	ret.append(op(it.get_curr()));
    }
  return ret;
}

template <typename T, class Op, class ... Cs>
T zip_foldl(const T & init, Op op, const Cs & ... cs)
{
  T acu = init;
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    acu = op(acu, it.get_curr());
  return acu;
}

template <class Op, class ... Cs>
auto zip_filter(Op op, const Cs & ... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret;
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (op(t))
	ret.append(t);
    }
  return ret;
}

template <class Cmp, class ... Cs>
bool zip_cmp(Cmp cmp, const Cs & ... cs)
{
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    {
      auto l = it.get_curr_list();
      auto curr = l.remove_first();
      while (not l.is_empty())
	{
	  if (not cmp(curr, l.get_first()))
	    return false;
	  curr = l.remove_first();
	}
    }
  return true;
}

template <class Op, class ... Cs>
size_t zip_find_index(Op op, const Cs & ... cs)
{
  size_t i = 0;
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next(), ++i)
    {
      auto t = it.get_curr();
      if (op(t))
	break;
    }
  return i;
}

template <class Op, class ... Cs>
auto zip_partition(Op op, const Cs & ... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret1, ret2;
  size_t n1 = 0, n2 = 0;
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (op(t))
	{
	  ret1.append(t);
	  ++n1;
	}
      else
	{
	  ret2.append(t);
	  ++n2;
	}
    }
  return make_tuple(ret1, n1, ret2, n2);
}

template <class ... Cs> inline
auto t_zip(const Cs & ... cs)
{
  auto it = get_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next())
    ret.append(it.get_curr());

  return ret;
}

template <class ... Cs> inline
auto t_zip_eq(const Cs & ... cs)
{
  auto it = get_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next())
    ret.append(it.get_curr());

  if (not it.completed())
    throw std::length_error("Container sizes mismatch");

  return ret;
}

template <class HeadC, class ... TailC>
class EnumZipIterator : public EnumZipIterator<TailC...>
{
  typename HeadC::Iterator it;

  using T = typename HeadC::Item_Type;

public:

  EnumZipIterator(const HeadC & head, const TailC & ...tail)
    : EnumZipIterator<TailC...>(tail...), it(head) {}

  bool has_curr() const
  {
    return it.has_curr() and EnumZipIterator<TailC...>::has_curr();
  }

  bool completed() const
  {
    return not it.has_curr() and EnumZipIterator<TailC...>::completed();
  }

  auto get_curr() const
  {
    std::tuple<T> curr(it.get_curr());
    return std::tuple_cat(curr, EnumZipIterator<TailC...>::get_curr());
  }

  void next()
  {
    it.next();
    EnumZipIterator<TailC...>::next();
  }
};

template <class C>
class EnumZipIterator<C> : public C::Iterator
{
  size_t i = 0;

public:

  void next()
  {
    C::Iterator::next();
    ++i;
  }

  using T = typename C::Item_Type;

  EnumZipIterator(const C & c) : C::Iterator(c) {}

  std::tuple<T, size_t> get_curr() const
  {
    return std::tuple<T, size_t>(C::Iterator::get_curr(), i);
  }

  bool completed() const { return not this->has_curr(); }
};

template <class ... Cs> inline
EnumZipIterator<Cs...> get_enum_zip_it(const Cs & ... cs)
{
  return EnumZipIterator<Cs...>(cs...);
}

template <class C, class ... Cs> inline
EnumZipIterator<C, Cs...> get_enum_zip_it(size_t pos,
					  const C & c, const Cs & ... cs)
{
  EnumZipIterator<C, Cs...> ret(c, cs...);
  for (size_t i = 0; i < pos; ++i)
    ret.next();

  return ret;
}

template <class ... Cs> inline
EnumZipIterator<Cs...> enum_zip_it(const Cs & ... cs)
{
  return EnumZipIterator<Cs...>(cs...);
}

template <class ... Cs> inline
EnumZipIterator<Cs...> enum_zip_it(size_t pos, const Cs & ... cs)
{
  return get_enum_zip_it(pos, cs...);
}

template <class ... Cs> inline
auto t_enum_zip(const Cs & ... cs)
{
  auto it = get_enum_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next())
    ret.append(it.get_curr());

  return ret;
}

template <class ... Cs> inline
auto t_enum_zip_eq(const Cs & ... cs)
{
  auto it = get_enum_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next())
    ret.append(it.get_curr());

  if (not it.completed())
    throw std::length_error("Container sizes mismatch");

  return ret;
}

template<typename... Ts, size_t... is> static inline
auto t_unzip_impl(const DynList<std::tuple<Ts...>> & l,
		std::index_sequence<is...>)
{
  std::tuple<DynList<Ts>...> ret;
  for (auto it = l.get_it(); it.has_curr(); it.next())
    std::initializer_list<int> {
      (std::get<is>(ret).append(std::get<is>(it.get_curr())), 0)... };

  return ret;
}

template <class... Ts> inline
auto t_unzip(const DynList<std::tuple<Ts...>> & l)
{
  return t_unzip_impl(l, std::make_index_sequence<sizeof...(Ts)>{});
}

/** Take several container of same type and some length and builds a
    list of lists

    Example: l1 = { 1, 2, 3, 4}
             l2 = { 5, 6, 7, 8}
	     l3 = { 9, 0, 10, 11}

    Where each li is a `DynList<int>` (but they could be
    heterogeneous; i.e. they could be DynList, DynSetTree, etc)

    Then `zip_list(l1, l2, l3)` produces a `DynList<DynList<int>>`
    with the following content:

    { {1, 5, 9}, {2, 6, 0}, {3, 7, 10}, {4, 8, 11} }

   @ingroup Algos
 */
template <class C, typename ... Lists>
auto zip_lists(const C & c, const Lists & ... lists)
{
  using T = typename C::Item_Type;
  DynList<DynList<T>> ret;
  for (auto it = get_zip_it(c, lists...); it.has_curr(); it.next())
    ret.append(it.get_curr_list());
  return ret;
}

template <class C, typename ... Lists>
auto zip_lists_eq(const C & c, const Lists & ... lists)
{
  using T = typename C::Item_Type;
  DynList<DynList<T>> ret;
  auto it = get_zip_it(c, lists...);
  for (; it.has_curr(); it.next())
    ret. append(it.get_curr_list());

  if (not it.completed())
    throw std::length_error("zip_list_eq: container sizes mismatch");

  return ret;
}



# endif // AH_ZIP_H







