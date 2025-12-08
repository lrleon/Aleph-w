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

# ifndef AH_ZIP_H
# define AH_ZIP_H 1

# include <type_traits>
# include <tuple>
# include <vector>

# include <htlist.H>
# include <ahFunctional.H>

using namespace std;
using namespace Aleph;

/**

    @author Leandro Rabindranath Leon
 */
template <class HeadC, class ... TailC>
class ZipIterator : public ZipIterator<TailC...>
{
  typename HeadC::Iterator it;

  using T = std::decay_t<typename HeadC::Item_Type>;

 public:

  ZipIterator(const HeadC &head, const TailC &...tail)
    : ZipIterator<TailC...>(tail...), it(head) {}

  [[nodiscard]] bool has_curr() const
  {
    return it.has_curr() and ZipIterator<TailC...>::has_curr();
  }

  [[nodiscard]] bool completed() const
  {
    return not it.has_curr() and ZipIterator<TailC...>::completed();
  }

  auto get_curr() const
  {
    tuple<T> curr(it.get_curr());
    return tuple_cat(curr, ZipIterator<TailC...>::get_curr());
  }

  auto get_curr_ne() const noexcept
  {
    tuple<T> curr(it.get_curr_ne());
    return tuple_cat(curr, ZipIterator<TailC...>::get_curr_ne());
  }

  using ZipType = tuple<T, TailC...>;

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

  void next_ne() noexcept
  {
    it.next_ne();
    ZipIterator<TailC...>::next_ne();
  }
};

template <class C>
class ZipIterator<C> : public C::Iterator
{
 public:

  using T = std::decay_t<typename C::Item_Type>;

  ZipIterator(const C &c) : C::Iterator(c) {}

  tuple<T> get_curr() const
  {
    return tuple<T>(C::Iterator::get_curr());
  }

  tuple<T> get_curr_ne() const noexcept
  {
    return tuple<T>(C::Iterator::get_curr_ne());
  }

  DynList<T> get_curr_list() const
  {
    DynList<T> l;
    l.append(C::Iterator::get_curr());
    return l;
  }

  [[nodiscard]] virtual bool completed() const { return not this->has_curr(); }
};

template <class ... Cs>
inline
ZipIterator<Cs...> get_zip_it(const Cs &... cs)
{
  return ZipIterator<Cs...>(cs...);
}

template <class ... Cs>
inline
ZipIterator<Cs...> get_zip_it_pos(size_t pos, const Cs &... cs)
{
  ZipIterator<Cs...> ret(cs...);
  for (size_t i = 0; i < pos; ++i)
    ret.next();
  return ret;
}

// TODO: why it does not work with rvalues?
template <class ... Cs>
inline
ZipIterator<Cs...> zip_it(const Cs &... cs)
{
  return get_zip_it(cs...);
}

template <class ... Cs>
inline
ZipIterator<Cs...> zip_it_pos(size_t pos, const Cs &... cs)
{
  return get_zip_it_pos(pos, cs...);
}

template <class ... Cs>
inline
bool equal_length(const Cs &... cs)
{
  auto it = get_zip_it(cs...);
  for (/* already initialized */; it.has_curr(); it.next_ne())
    /* empty */;

  return it.completed();
}

template <class Op, class ... Cs>
inline
bool zip_traverse(Op &op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (not op(it.get_curr()))
      return false;
  return true;
}

template <class Op, class ... Cs>
inline
bool zip_traverse(Op &&op, const Cs &... cs)
{
  return zip_traverse(op, cs ...);
}

template <class Op, class ... Cs>
inline
bool zip_traverse_eq(Op &op, const Cs &... cs)
{
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    if (not op(it.get_curr()))
      return false;
  return it.completed();
}

template <class Op, class ... Cs>
inline
bool zip_traverse_eq(Op &&op, const Cs &... cs)
{
  return zip_traverse_eq(op, cs ...);
}

template <class Op, class ... Cs>
inline
void zip_for_each(Op &op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    op(it.get_curr());
}

template <class Op, class ... Cs>
inline
void zip_for_each(Op &&op, const Cs &... cs)
{
  zip_for_each<Op, Cs...>(op, cs ...);
}

template <class Op, class ... Cs>
inline
void zip_for_each_eq(Op &op, const Cs &... cs)
{
  auto it = get_zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    op(it.get_curr());

  if (not it.completed())
    throw length_error("zip_for_each_eq() containers sizes mismatch");
}

template <class Op, class ... Cs>
inline
void zip_for_each_eq(Op &&op, const Cs &... cs)
{
  zip_for_each_eq<Op, Cs...>(op, cs ...);
}

template <class Op, class ... Cs>
inline
bool zip_all(Op &op, const Cs &... cs)
{
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    if (not op(it.get_curr()))
      return false;

  return it.completed();
}

template <class Op, class ... Cs>
inline
bool zip_all(Op &&op, const Cs &... cs)
{
  return zip_all(op, cs...);
}

template <class Op, class ... Cs>
inline
bool zip_exists(Op &op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (op(it.get_curr()))
      return true;

  return false;
}

template <class Op, class ... Cs>
inline
bool zip_exists(Op &&op, const Cs &... cs)
{
  return zip_exists(op, cs...);
}

template <typename T, class Op, class ... Cs>
DynList<T> zip_maps(Op op, const Cs &... cs)
{
  DynList<T> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    ret.append(op(it.get_curr()));
  return ret;
}

template <typename T, class Prop, class Op, class ... Cs>
DynList<T> zip_maps_if(Prop prop, Op op, const Cs &... cs)
{
  DynList<T> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    {
      auto t = it.get_curr();
      if (prop(t))
        ret.append(op(it.get_curr()));
    }
  return ret;
}

template <typename T, class Op, class ... Cs>
T zip_foldl(const T &init, Op op, const Cs &... cs)
{
  T acu = init;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    acu = op(acu, it.get_curr());
  return acu;
}

template <class Op, class ... Cs>
auto zip_filter(Op op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    {
      auto t = it.get_curr();
      if (op(t))
        ret.append(t);
    }
  return ret;
}

/** Return true if `cmp(t)` is `true` for each built tuple `t`.

    Let us suppose 3 containers of the same internal type `T`. 

    l1 = { 1, 2, 3};
    l2 = { 1, 2, 3};
    l3 = { 1, 2, 3};

    This function, which receives a comparator `cmp(i1, i2)`, returns
    true if the elements of each tuple resulting of the zip of three
    containers are the same. So, in the previous case the result is
    `true`.

    If any item of any list variates, then the result should be
    `false`. For example with `l2 = { 1, 4, 3}`.

    @author Leandro Rabindranath Leon
*/
template <class Cmp, class ... Cs>
bool zip_cmp(Cmp cmp, const Cs &... cs)
{
  for (auto it = get_zip_it(cs...); it.has_curr(); it.next_ne())
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
size_t zip_find_index(Op op, const Cs &... cs)
{
  size_t i = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne(), ++i)
    {
      auto t = it.get_curr();
      if (op(t))
        break;
    }
  return i;
}

template <class Op, class ... Cs>
auto zip_partition(Op op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret1, ret2;
  size_t n1 = 0, n2 = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
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

template <class ... Cs>
inline
auto t_zip(const Cs &... cs)
{
  auto it = zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());

  return ret;
}

template <class ... Cs>
inline
auto t_zip_eq(const Cs &... cs)
{
  auto it = zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());

  if (not it.completed())
    throw length_error("Container sizes mismatch");

  return ret;
}

template <class HeadC, class ... TailC>
class EnumZipIterator : public EnumZipIterator<TailC...>
{
  typename HeadC::Iterator it;

  using T = typename HeadC::Item_Type;

 public:

  EnumZipIterator(const HeadC &head, const TailC &...tail)
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
    tuple<T> curr(it.get_curr());
    return tuple_cat(curr, EnumZipIterator<TailC...>::get_curr());
  }

  void next()
  {
    it.next();
    EnumZipIterator<TailC...>::next();
  }

  auto get_curr_ne() const noexcept
  {
    tuple<T> curr(it.get_curr_ne());
    return tuple_cat(curr, EnumZipIterator<TailC...>::get_curr_ne());
  }

  void next_ne() noexcept
  {
    it.next_ne();
    EnumZipIterator<TailC...>::next_ne();
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

  void next_ne() noexcept
  {
    C::Iterator::next_ne();
    ++i;
  }

  using T = typename C::Item_Type;

  EnumZipIterator(const C &c) : C::Iterator(c) {}

  tuple<T, size_t> get_curr() const
  {
    return tuple<T, size_t>(C::Iterator::get_curr(), i);
  }

  tuple<T, size_t> get_curr_ne() const noexcept
  {
    return tuple<T, size_t>(C::Iterator::get_curr_ne(), i);
  }

  bool completed() const { return not this->has_curr(); }
};

template <class ... Cs>
inline
EnumZipIterator<Cs...> get_enum_zip_it(const Cs &... cs)
{
  return EnumZipIterator<Cs...>(cs...);
}

template <class C, class ... Cs>
inline
EnumZipIterator<C, Cs...>
get_enum_zip_it_pos(size_t pos, const C &c, const Cs &... cs)
{
  EnumZipIterator<C, Cs...> ret(c, cs...);
  for (size_t i = 0; i < pos; ++i)
    ret.next();

  return ret;
}

template <class ... Cs>
inline
EnumZipIterator<Cs...> enum_zip_it(const Cs &... cs)
{
  return EnumZipIterator<Cs...>(cs...);
}

template <class ... Cs>
inline
EnumZipIterator<Cs...> enum_zip_it_pos(size_t pos, const Cs &... cs)
{
  return get_enum_zip_it_pos(pos, cs...);
}

template <class ... Cs>
inline
auto t_enum_zip(const Cs &... cs)
{
  auto it = get_enum_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());

  return ret;
}

template <class ... Cs>
inline
auto t_enum_zip_eq(const Cs &... cs)
{
  auto it = get_enum_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());

  if (not it.completed())
    throw length_error("Container sizes mismatch");

  return ret;
}

template <typename... Ts, size_t... is>
static inline
auto t_unzip_impl(const DynList<tuple<Ts...>> &l,
                  index_sequence<is...>)
{
  tuple<DynList<Ts>...> ret;
  for (auto it = l.get_it(); it.has_curr(); it.next_ne())
    initializer_list<int>{(get<is>(ret).append(get<is>(it.get_curr())), 0)...};

  return ret;
}

// Unzip a list of tuples. Return a tuple of lists
template <class... Ts>
inline
auto t_unzip(const DynList<tuple<Ts...>> &tuples)
{
  return t_unzip_impl(tuples, std::index_sequence_for<Ts...>{});
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
auto zip_lists(const C &c, const Lists &... lists)
{
  using T = typename C::Item_Type;
  DynList<DynList<T>> ret;
  for (auto it = get_zip_it(c, lists...); it.has_curr(); it.next_ne())
    ret.append(it.get_curr_list());
  return ret;
}

template <class C, typename ... Lists>
auto zip_lists_eq(const C &c, const Lists &... lists)
{
  using T = typename C::Item_Type;
  DynList<DynList<T>> ret;
  auto it = get_zip_it(c, lists...);
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr_list());

  if (not it.completed())
    throw length_error("zip_list_eq: container sizes mismatch");

  return ret;
}

// build a zi vector from two containers
template <typename C1, typename C2>
auto std_zip(const C1 &c1, const C2 &c2)
{
  using T = typename C1::value_type;
  using U = typename C2::value_type;
  vector<pair<T, U>> result;
  auto it1 = c1.begin();
  auto it2 = c2.begin();
  for (; it1 != c1.end() and it2 != c2.end(); ++it1, ++it2)
    result.emplace_back(*it1, *it2);
  return result;
}

// Receive a variadic list of std containers and build a list of tuples from them
template <typename C1, typename C2, typename C3, typename C4>
inline auto
tzip_std(const C1 &c1, const C2 &c2, const C3 &c3, const C4 &c4)
{
  using T1 = typename C1::value_type;
  using T2 = typename C2::value_type;
  using T3 = typename C3::value_type;
  using T4 = typename C4::value_type;
  vector<tuple<T1, T2, T3, T4>> ret;
  auto it1 = c1.begin();
  auto it2 = c2.begin();
  auto it3 = c3.begin();
  auto it4 = c4.begin();
  for (; it1 != c1.end() and it2 != c2.end() and it3 != c3.end() and it4 != c4.end();
         ++it1, ++it2, ++it3, ++it4)
    ret.push_back(make_tuple(*it1, *it2, *it3, *it4));
  return ret;
}

# endif // AH_ZIP_H

