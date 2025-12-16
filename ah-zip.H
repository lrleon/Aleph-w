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

  /** Construct a zip iterator over multiple Aleph containers.

      Iteration stops when any of the underlying iterators reaches its end.

      @param[in] head First container.
      @param[in] tail Remaining containers.
  */
  ZipIterator(const HeadC &head, const TailC &...tail)
    : ZipIterator<TailC...>(tail...), it(head) {}

  /** Return `true` if all underlying iterators are positioned on a valid item. */
  [[nodiscard]] bool has_curr() const noexcept
  {
    return it.has_curr() and ZipIterator<TailC...>::has_curr();
  }

  /** Return `true` if all underlying iterators are finished.

      This can be used to detect length mismatches after traversing with
      `has_curr()`.
  */
  [[nodiscard]] bool completed() const noexcept
  {
    return not it.has_curr() and ZipIterator<TailC...>::completed();
  }

  /** Return the current tuple (bounds-checked).

      @throw std::overflow_error if any underlying iterator is past the end.
  */
  auto get_curr() const
  {
    tuple<T> curr(it.get_curr());
    return tuple_cat(curr, ZipIterator<TailC...>::get_curr());
  }

  /** Return the current tuple (no-throw variant).

      Requires `has_curr()`.
  */
  auto get_curr_ne() const noexcept
  {
    tuple<T> curr(it.get_curr_ne());
    return tuple_cat(curr, ZipIterator<TailC...>::get_curr_ne());
  }

  /// Returns current elements as a DynList (requires homogeneous Item_Type)
  DynList<T> get_curr_list() const
  {
    DynList<T> l;
    l.append(it.get_curr());
    l.append(ZipIterator<TailC...>::get_curr_list());
    return l;
  }

  /** Advance all underlying iterators (bounds-checked). */
  void next()
  {
    it.next();
    ZipIterator<TailC...>::next();
  }

  /** Advance all underlying iterators (no-throw variant).

      Requires `has_curr()`.
  */
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

  /** Construct a zip iterator over a single container.

      This is mainly used as the base case for the recursive ZipIterator.

      @param[in] c Container to iterate.
  */
  ZipIterator(const C &c) : C::Iterator(c) {}

  /** Return the current tuple (bounds-checked).

      @throw std::overflow_error if the iterator is past the end.
  */
  tuple<T> get_curr() const
  {
    return tuple<T>(C::Iterator::get_curr());
  }

  /** Return the current tuple (no-throw variant).

      Requires `has_curr()`.
  */
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

  /** Return `true` if the underlying iterator has reached the end. */
  [[nodiscard]] bool completed() const noexcept { return not this->has_curr(); }
};

/** Create a ZipIterator over the given containers.

    @param cs Containers to zip.
    @return ZipIterator positioned at the first element.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
ZipIterator<Cs...> get_zip_it(const Cs &... cs)
{
  return ZipIterator<Cs...>(cs...);
}

/** Create a ZipIterator positioned at index `pos`.

    @param pos Starting position.
    @param cs Containers to zip.
    @return ZipIterator advanced `pos` positions.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
ZipIterator<Cs...> get_zip_it_pos(size_t pos, const Cs &... cs)
{
  ZipIterator<Cs...> ret(cs...);
  for (size_t i = 0; i < pos; ++i)
    ret.next();
  return ret;
}

/** Alias for `get_zip_it`. */
template <class ... Cs>
[[nodiscard]] inline
ZipIterator<Cs...> zip_it(const Cs &... cs)
{
  return get_zip_it(cs...);
}

/** Alias for `get_zip_it_pos`. */
template <class ... Cs>
[[nodiscard]] inline
ZipIterator<Cs...> zip_it_pos(size_t pos, const Cs &... cs)
{
  return get_zip_it_pos(pos, cs...);
}

/** Return `true` if all containers have the same length.

    @param cs Containers to compare.
    @return `true` iff all containers have equal size.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline constexpr
bool equal_length(const Cs &... cs)
{
  auto it = get_zip_it(cs...);
  for (/* already initialized */; it.has_curr(); it.next_ne())
    /* empty */;

  return it.completed();
}

/** Traverse zipped containers while `op` returns `true`.

    @param op Callable returning `bool`; traversal stops on `false`.
    @param cs Containers to zip.
    @return `true` if `op` returned `true` for every tuple.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_traverse(Op &&op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (not std::forward<Op>(op)(it.get_curr()))
      return false;
  return true;
}

/** Traverse zipped containers while `op` returns `true`; verify equal lengths.

    @param op Callable returning `bool`; traversal stops on `false`.
    @param cs Containers to zip.
    @return `true` if all containers have equal length and `op` returned `true`
            for every tuple.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_traverse_eq(Op &&op, const Cs &... cs)
{
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    if (not std::forward<Op>(op)(it.get_curr()))
      return false;
  return it.completed();
}

/** Apply `op` to every zipped tuple.

    @param op Callable to invoke on each tuple.
    @param cs Containers to zip.
    @ingroup Algos
*/
template <class Op, class ... Cs>
inline
void zip_for_each(Op &&op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    std::forward<Op>(op)(it.get_curr());
}

/** Apply `op` to every zipped tuple; throw if containers differ in length.

    @param op Callable to invoke on each tuple.
    @param cs Containers to zip.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algos
*/
template <class Op, class ... Cs>
inline
void zip_for_each_eq(Op &&op, const Cs &... cs)
{
  auto it = get_zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    std::forward<Op>(op)(it.get_curr());

  if (not it.completed())
    throw length_error("zip_for_each_eq() containers sizes mismatch");
}

/** Return `true` if `op` returns `true` for all tuples and containers have equal length.

    @param op Predicate.
    @param cs Containers to zip.
    @return `true` iff `op` is `true` for all tuples and lengths match.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_all(Op &&op, const Cs &... cs)
{
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    if (not std::forward<Op>(op)(it.get_curr()))
      return false;

  return it.completed();
}

/** Return `true` if `op` returns `true` for at least one tuple.

    @param op Predicate.
    @param cs Containers to zip.
    @return `true` if any tuple satisfies `op`.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_exists(Op &&op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (std::forward<Op>(op)(it.get_curr()))
      return true;

  return false;
}

/** Map `op` over zipped tuples, returning a list of results.

    @tparam T Result type.
    @param op Transformation callable.
    @param cs Containers to zip.
    @return DynList of transformed values.
    @ingroup Algos
*/
template <typename T, class Op, class ... Cs>
[[nodiscard]]
DynList<T> zip_maps(Op op, const Cs &... cs)
{
  DynList<T> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    ret.append(op(it.get_curr()));
  return ret;
}

/** Map `op` over zipped tuples that satisfy `prop`.

    @tparam T Result type.
    @param prop Predicate selecting which tuples to transform.
    @param op Transformation callable.
    @param cs Containers to zip.
    @return DynList of transformed values for tuples where `prop` is `true`.
    @ingroup Algos
*/
template <typename T, class Prop, class Op, class ... Cs>
[[nodiscard]]
DynList<T> zip_maps_if(Prop prop, Op op, const Cs &... cs)
{
  DynList<T> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
      if (auto t = it.get_curr(); prop(t))
        ret.append(op(t));

  return ret;
}

/** Left fold over zipped tuples.

    @tparam T Accumulator type.
    @param init Initial accumulator value.
    @param op Binary operation `(T, tuple) -> T`.
    @param cs Containers to zip.
    @return Final accumulated value.
    @ingroup Algos
*/
template <typename T, class Op, class ... Cs>
[[nodiscard]]
T zip_foldl(const T &init, Op op, const Cs &... cs)
{
  T acu = init;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    acu = op(acu, it.get_curr());
  return acu;
}

/** Filter zipped tuples by predicate `op`.

    @param op Predicate.
    @param cs Containers to zip.
    @return DynList of tuples for which `op` returned `true`.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_filter(Op op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
      if (auto t = it.get_curr(); op(t))
        ret.append(t);

  return ret;
}

namespace zip_detail
{
  // Helper to compare all adjacent pairs in a tuple
  template <class Cmp, class Tuple, size_t... Is>
  bool compare_adjacent(Cmp &cmp, const Tuple &t, std::index_sequence<Is...>)
  {
    // Compare t[0] with t[1], t[1] with t[2], etc.
    return (... and cmp(std::get<Is>(t), std::get<Is + 1>(t)));
  }
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

    @note All containers must have the same Item_Type for comparison to work.

    @author Leandro Rabindranath Leon
*/
template <class Cmp, class ... Cs>
bool zip_cmp(Cmp cmp, const Cs &... cs)
{
  constexpr size_t N = sizeof...(Cs);
  static_assert(N >= 2, "zip_cmp requires at least 2 containers");

  for (auto it = get_zip_it(cs...); it.has_curr(); it.next_ne())
      if (auto t = it.get_curr(); not zip_detail::compare_adjacent(cmp, t, std::make_index_sequence<N - 1>{}))
        return false;

  return true;
}

/** Find the first index where `op` returns `true`.

    @param op Predicate.
    @param cs Containers to zip.
    @return Index of the first matching tuple, or the total number of tuples
            if none match.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]]
size_t zip_find_index(Op op, const Cs &... cs)
{
  size_t i = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne(), ++i)
      if (auto t = it.get_curr(); op(t))
        break;

  return i;
}

/** Partition zipped tuples by predicate `op`.

    @param op Predicate.
    @param cs Containers to zip.
    @return Tuple `(matching_list, matching_count, non_matching_list, non_matching_count)`.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_partition(Op op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret1, ret2;
  size_t n1 = 0, n2 = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
      if (auto t = it.get_curr(); op(t))
        {
          ret1.append(t);
          ++n1;
        }
      else
        {
          ret2.append(t);
          ++n2;
        }

  return make_tuple(ret1, n1, ret2, n2);
}

/** Materialize zipped tuples into a DynList.

    @param cs Containers to zip.
    @return DynList of tuples.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
auto t_zip(const Cs &... cs)
{
  auto it = zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());

  return ret;
}

/** Materialize zipped tuples; throw if containers differ in length.

    @param cs Containers to zip.
    @return DynList of tuples.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
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

  using T = std::decay_t<typename HeadC::Item_Type>;

 public:

  /** Construct an enumerating zip iterator over multiple containers.

      The index is appended as the last element in the resulting tuple.
  */
  EnumZipIterator(const HeadC &head, const TailC &...tail)
    : EnumZipIterator<TailC...>(tail...), it(head) {}

  /** Return `true` if all underlying iterators have a current element. */
  [[nodiscard]] bool has_curr() const noexcept
  {
    return it.has_curr() and EnumZipIterator<TailC...>::has_curr();
  }

  /** Return `true` if all underlying iterators are finished. */
  [[nodiscard]] bool completed() const noexcept
  {
    return not it.has_curr() and EnumZipIterator<TailC...>::completed();
  }

  /** Return the current tuple (bounds-checked).

      @throw std::overflow_error if any underlying iterator is past the end.
  */
  auto get_curr() const
  {
    tuple<T> curr(it.get_curr());
    return tuple_cat(curr, EnumZipIterator<TailC...>::get_curr());
  }

  /** Advance all underlying iterators (bounds-checked). */
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

  /** Advance all underlying iterators (no-throw variant).

      Requires `has_curr()`.
  */
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

  /** Advance the iterator and increment the associated index (bounds-checked). */
  void next()
  {
    C::Iterator::next();
    ++i;
  }

  /** Advance the iterator and increment the associated index (no-throw variant).

      Requires `has_curr()`.
  */
  void next_ne() noexcept
  {
    C::Iterator::next_ne();
    ++i;
  }

  using T = std::decay_t<typename C::Item_Type>;

  /** Construct an enumerating iterator over a single container.

      @param[in] c Container to iterate.
  */
  EnumZipIterator(const C &c) : C::Iterator(c) {}

  /** Return the current tuple `(value, index)` (bounds-checked). */
  tuple<T, size_t> get_curr() const
  {
    return tuple<T, size_t>(C::Iterator::get_curr(), i);
  }

  /** Return the current tuple `(value, index)` (no-throw variant).

      Requires `has_curr()`.
  */
  tuple<T, size_t> get_curr_ne() const noexcept
  {
    return tuple<T, size_t>(C::Iterator::get_curr_ne(), i);
  }

  /** Return `true` if the underlying iterator has reached the end. */
  [[nodiscard]] bool completed() const noexcept { return not this->has_curr(); }
};

/** Create an EnumZipIterator over the given containers.

    @param cs Containers to zip.
    @return EnumZipIterator positioned at the first element (index 0).
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
EnumZipIterator<Cs...> get_enum_zip_it(const Cs &... cs)
{
  return EnumZipIterator<Cs...>(cs...);
}

/** Create an EnumZipIterator positioned at index `pos`.

    @param pos Starting position.
    @param c First container.
    @param cs Additional containers.
    @return EnumZipIterator advanced `pos` positions.
    @ingroup Algos
*/
template <class C, class ... Cs>
[[nodiscard]] inline
EnumZipIterator<C, Cs...>
get_enum_zip_it_pos(size_t pos, const C &c, const Cs &... cs)
{
  EnumZipIterator<C, Cs...> ret(c, cs...);
  for (size_t i = 0; i < pos; ++i)
    ret.next();

  return ret;
}

/** Alias for `get_enum_zip_it`. */
template <class ... Cs>
[[nodiscard]] inline
EnumZipIterator<Cs...> enum_zip_it(const Cs &... cs)
{
  return EnumZipIterator<Cs...>(cs...);
}

/** Alias for `get_enum_zip_it_pos`. */
template <class ... Cs>
[[nodiscard]] inline
EnumZipIterator<Cs...> enum_zip_it_pos(size_t pos, const Cs &... cs)
{
  return get_enum_zip_it_pos(pos, cs...);
}

/** Materialize enumerated zipped tuples into a DynList.

    Each tuple includes the index as its last element.

    @param cs Containers to zip.
    @return DynList of tuples with index.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
auto t_enum_zip(const Cs &... cs)
{
  auto it = get_enum_zip_it(cs...);
  using T = decltype(it.get_curr());
  DynList<T> ret;
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());

  return ret;
}

/** Materialize enumerated zipped tuples; throw if containers differ in length.

    @param cs Containers to zip.
    @return DynList of tuples with index.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]] inline
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

/** Unzip a list of tuples into a tuple of lists.

    @param tuples Input list of tuples.
    @return Tuple of DynLists, one per tuple element.
    @ingroup Algos
*/
template <class... Ts>
[[nodiscard]] inline
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

/** Like `zip_lists` but throws if containers differ in length.

    @param c First container.
    @param lists Additional containers.
    @return DynList of DynLists.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algos
*/
template <class C, typename ... Lists>
[[nodiscard]]
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

namespace std_zip_detail
{
  // Helper to check if all iterators are valid (not at end)
  template <typename... Its>
  bool all_valid(const Its&... its)
  {
    return (... and (its.first != its.second));
  }

  // Helper to advance all iterators
  template <typename... Its>
  void advance_all(Its&... its)
  {
    (++its.first, ...);
  }

  // Helper to dereference all iterators and make a tuple
  template <typename... Its>
  auto deref_all(const Its&... its)
  {
    return std::make_tuple(*its.first...);
  }
}

/** Build a vector of pairs from two STL containers.

    @note For backwards compatibility. For more than 2 containers, use tzip_std.
*/
template <typename C1, typename C2>
[[nodiscard]]
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

/** Build a vector of tuples from 2 or more STL containers (variadic).

    Works with any STL-compatible containers that have begin(), end(),
    and value_type. Stops at the shortest container.

    Example:
    @code
    vector<int> v1 = {1, 2, 3};
    vector<double> v2 = {1.1, 2.2, 3.3};
    vector<string> v3 = {"a", "b", "c"};
    auto result = tzip_std(v1, v2, v3);
    // result is vector<tuple<int, double, string>>
    @endcode
*/
template <typename... Cs>
auto tzip_std(const Cs&... cs)
{
  static_assert(sizeof...(Cs) >= 2, "tzip_std requires at least 2 containers");

  using TupleType = std::tuple<typename Cs::value_type...>;
  std::vector<TupleType> ret;

  // Create iterator pairs (begin, end) for each container
  auto iters = std::make_tuple(std::make_pair(cs.begin(), cs.end())...);

  // Helper to check all iterators and build tuples
  auto check_and_push = [&ret](auto&... its) {
    while (std_zip_detail::all_valid(its...))
      {
        ret.push_back(std_zip_detail::deref_all(its...));
        std_zip_detail::advance_all(its...);
      }
  };

  std::apply(check_and_push, iters);
  return ret;
}

/** Build a vector of tuples from 2 or more STL containers (variadic).

    This is a convenience alias for `tzip_std()` with a more explicit name
    (and to avoid ambiguity with the legacy 2-container `std_zip()` that
    returns `std::vector<std::pair<...>>`).

    @param cs STL-compatible containers.
    @return `std::vector<std::tuple<...>>` stopping at the shortest container.
*/
template <typename... Cs>
[[nodiscard]]
auto std_zip_n(const Cs &... cs)
{
  return tzip_std(cs...);
}


/** Transform zipped tuples using `op` and return results in a vector.
    
    Similar to `std::ranges::transform` but operates on zipped containers.
    
    @tparam Op Transformation callable `(tuple) -> R`.
    @param op Transformation to apply.
    @param cs Containers to zip.
    @return `std::vector` of transformed values.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_transform(Op &&op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());
  using ResultType = std::decay_t<decltype(std::forward<Op>(op)(std::declval<ZipType>()))>;
  
  std::vector<ResultType> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    ret.push_back(std::forward<Op>(op)(it.get_curr()));
  return ret;
}

/** Transform zipped tuples; throw if containers differ in length.
    
    @tparam Op Transformation callable.
    @param op Transformation to apply.
    @param cs Containers to zip.
    @return `std::vector` of transformed values.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algos
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_transform_eq(Op &&op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());
  using ResultType = std::decay_t<decltype(std::forward<Op>(op)(std::declval<ZipType>()))>;
  
  std::vector<ResultType> ret;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    ret.push_back(std::forward<Op>(op)(it.get_curr()));
  
  if (not it.completed())
    throw length_error("zip_transform_eq() containers sizes mismatch");
  return ret;
}

/** Apply `op` to each zipped tuple along with its index.
    
    More direct than using `enum_zip_it` manually.
    
    @param op Callable `(size_t index, tuple) -> void`.
    @param cs Containers to zip.
    @ingroup Algos
*/
template <class Op, class ... Cs>
inline
void zip_for_each_indexed(Op &&op, const Cs &... cs)
{
  size_t idx = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne(), ++idx)
    std::forward<Op>(op)(idx, it.get_curr());
}

/** Apply `op` to each zipped tuple with index; throw if lengths differ.
    
    @param op Callable `(size_t index, tuple) -> void`.
    @param cs Containers to zip.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algos
*/
template <class Op, class ... Cs>
inline
void zip_for_each_indexed_eq(Op &&op, const Cs &... cs)
{
  size_t idx = 0;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne(), ++idx)
    std::forward<Op>(op)(idx, it.get_curr());
  
  if (not it.completed())
    throw length_error("zip_for_each_indexed_eq() containers sizes mismatch");
}

/** Take at most `n` tuples from the zipped containers.
    
    @param n Maximum number of tuples to take.
    @param cs Containers to zip.
    @return DynList of up to `n` tuples.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]]
auto zip_take(size_t n, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());
  DynList<ZipType> ret;
  size_t count = 0;
  for (auto it = zip_it(cs...); it.has_curr() and count < n; it.next_ne(), ++count)
    ret.append(it.get_curr());
  return ret;
}

/** Skip `n` tuples and return the rest.
    
    @param n Number of tuples to skip.
    @param cs Containers to zip.
    @return DynList of remaining tuples after skipping `n`.
    @ingroup Algos
*/
template <class ... Cs>
[[nodiscard]]
auto zip_drop(size_t n, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());
  DynList<ZipType> ret;
  auto it = zip_it(cs...);
  
  // Skip first n elements
  for (size_t i = 0; i < n and it.has_curr(); ++i)
    it.next_ne();
  
  // Collect the rest
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());
  return ret;
}

/** Take tuples while predicate `pred` returns true.
    
    @param pred Predicate `(tuple) -> bool`.
    @param cs Containers to zip.
    @return DynList of tuples while `pred` is satisfied.
    @ingroup Algos
*/
template <class Pred, class ... Cs>
[[nodiscard]]
auto zip_take_while(Pred &&pred, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());
  DynList<ZipType> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    {
      auto t = it.get_curr();
      if (not std::forward<Pred>(pred)(t))
        break;
      ret.append(t);
    }
  return ret;
}

/** Skip tuples while predicate `pred` returns true, then return the rest.
    
    @param pred Predicate `(tuple) -> bool`.
    @param cs Containers to zip.
    @return DynList of tuples after `pred` becomes false.
    @ingroup Algos
*/
template <class Pred, class ... Cs>
[[nodiscard]]
auto zip_drop_while(Pred &&pred, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());
  DynList<ZipType> ret;
  auto it = zip_it(cs...);
  
  // Skip while pred is true
  for (; it.has_curr() and std::forward<Pred>(pred)(it.get_curr()); it.next_ne())
    /* skip */;
  
  // Collect the rest
  for (; it.has_curr(); it.next_ne())
    ret.append(it.get_curr());
  return ret;
}

// ============================================================================
// Index sequence helpers
// ============================================================================

namespace zip_index_detail
{
  // Apply a function to each element in a tuple using index_sequence
  template <class Op, class Tuple, size_t... Is>
  void for_each_in_tuple_impl(Op &&op, Tuple &&t, std::index_sequence<Is...>)
  {
    (std::forward<Op>(op)(std::get<Is>(std::forward<Tuple>(t))), ...);
  }
  
  // Transform each element in a tuple using index_sequence
  template <class Op, class Tuple, size_t... Is>
  auto transform_tuple_impl(Op &&op, Tuple &&t, std::index_sequence<Is...>)
  {
    return std::make_tuple(std::forward<Op>(op)(std::get<Is>(std::forward<Tuple>(t)))...);
  }
  
  // Check if all elements satisfy a predicate using index_sequence
  template <class Pred, class Tuple, size_t... Is>
  bool all_of_tuple_impl(Pred &&pred, const Tuple &t, std::index_sequence<Is...>)
  {
    return (... and std::forward<Pred>(pred)(std::get<Is>(t)));
  }
  
  // Check if any element satisfies a predicate using index_sequence
  template <class Pred, class Tuple, size_t... Is>
  bool any_of_tuple_impl(Pred &&pred, const Tuple &t, std::index_sequence<Is...>)
  {
    return (... or std::forward<Pred>(pred)(std::get<Is>(t)));
  }
}

/** Apply `op` to each element of a tuple.
    
    Uses `std::index_sequence` for compile-time iteration.
    
    @param op Callable to apply to each element.
    @param t Tuple to iterate over.
    @ingroup Algos
*/
template <class Op, class Tuple>
void for_each_in_tuple(Op &&op, Tuple &&t)
{
  constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
  zip_index_detail::for_each_in_tuple_impl(
    std::forward<Op>(op), std::forward<Tuple>(t), std::make_index_sequence<N>{});
}

/** Transform each element of a tuple, returning a new tuple.
    
    Uses `std::index_sequence` for compile-time iteration.
    
    @param op Transformation to apply to each element.
    @param t Input tuple.
    @return Tuple of transformed values.
    @ingroup Algos
*/
template <class Op, class Tuple>
[[nodiscard]]
auto transform_tuple(Op &&op, Tuple &&t)
{
  constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
  return zip_index_detail::transform_tuple_impl(
    std::forward<Op>(op), std::forward<Tuple>(t), std::make_index_sequence<N>{});
}

/** Return `true` if all tuple elements satisfy `pred`.
    
    @param pred Predicate to test.
    @param t Tuple to check.
    @return `true` if `pred` is `true` for all elements.
    @ingroup Algos
*/
template <class Pred, class Tuple>
[[nodiscard]]
bool all_of_tuple(Pred &&pred, const Tuple &t)
{
  constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
  return zip_index_detail::all_of_tuple_impl(
    std::forward<Pred>(pred), t, std::make_index_sequence<N>{});
}

/** Return `true` if any tuple element satisfies `pred`.
    
    @param pred Predicate to test.
    @param t Tuple to check.
    @return `true` if `pred` is `true` for at least one element.
    @ingroup Algos
*/
template <class Pred, class Tuple>
[[nodiscard]]
bool any_of_tuple(Pred &&pred, const Tuple &t)
{
  constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
  return zip_index_detail::any_of_tuple_impl(
    std::forward<Pred>(pred), t, std::make_index_sequence<N>{});
}

# endif // AH_ZIP_H

