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
# include <tpl_dynSkipList.H>

/**
 * @file ah-zip.H
 * @brief Zip iterators and functional operations for multiple containers.
 *
 * This header provides:
 * - **ZipIterator**: Iterate over multiple Aleph containers in lockstep
 * - **EnumZipIterator**: Like ZipIterator but includes enumeration index
 * - **Functional operations**: zip_traverse, zip_for_each, zip_map, zip_filter,
 *   zip_fold, zip_partition, and more
 * - **STL compatibility**: std_zip, tzip_std for std::vector and other STL containers
 * - **Tuple utilities**: for_each_in_tuple, transform_tuple, all_of_tuple, any_of_tuple
 *
 * All zip operations stop at the shortest container. Variants ending in `_eq`
 * throw `std::length_error` if container lengths differ.
 *
 * @note This header is NOT thread-safe. Concurrent access to iterators
 *       or containers from multiple threads leads to undefined behavior.
 *
 * @ingroup Algorithms
 * @author Leandro Rabindranath Leon
 */

namespace Aleph
{

/**
 * @class ZipIterator
 * @brief Iterator that traverses multiple Aleph containers in lockstep.
 *
 * ZipIterator allows synchronized iteration over two or more containers,
 * producing tuples of corresponding elements at each position.
 *
 * Iteration stops when ANY underlying iterator reaches its end.
 *
 * @tparam HeadC Type of the first container.
 * @tparam TailC Types of remaining containers.
 *
 * Usage example:
 * @code
 * DynList<int> ints = {1, 2, 3};
 * DynList<string> strs = {"a", "b", "c"};
 *
 * for (auto it = zip_it(ints, strs); it.has_curr(); it.next_ne())
 *   {
 *     auto [i, s] = it.get_curr_ne();
 *     std::cout << i << ": " << s << '\n';
 *   }
 * @endcode
 *
 * @see get_zip_it(), zip_it(), EnumZipIterator
 * @note This class uses recursive template inheritance (CRTP pattern).
 *       The base class is a different specialization: ZipIterator<TailC...>
 * @ingroup Algorithms
 * @author Leandro Rabindranath Leon
 * @cond DOXYGEN_SHOULD_SKIP_THIS
 */
template <class HeadC, class ... TailC>
class ZipIterator : public ZipIterator<TailC...>
/// @endcond
{
  typename HeadC::Iterator it;

  using T = std::decay_t<typename HeadC::Item_Type>;

 public:

  /// The tuple type returned by get_curr() and get_curr_ne()
  using Tuple_Type = decltype(std::tuple_cat(
    std::declval<std::tuple<T>>(),
    std::declval<typename ZipIterator<TailC...>::Tuple_Type>()));

  /// Type of elements from the first container
  using Item_Type = T;

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
    std::tuple<T> curr(it.get_curr());
    return std::tuple_cat(curr, ZipIterator<TailC...>::get_curr());
  }

  /** Return the current tuple (no-throw variant).

      Requires `has_curr()`.
  */
  auto get_curr_ne() const noexcept
  {
    std::tuple<T> curr(it.get_curr_ne());
    return std::tuple_cat(curr, ZipIterator<TailC...>::get_curr_ne());
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

  /// The tuple type returned by get_curr() and get_curr_ne()
  using Tuple_Type = std::tuple<T>;

  /// Type of elements from the container
  using Item_Type = T;

  /** Construct a zip iterator over a single container.

      This is mainly used as the base case for the recursive ZipIterator.

      @param[in] c Container to iterate.
  */
  ZipIterator(const C &c) : C::Iterator(c) {}

  /** Return the current tuple (bounds-checked).

      @throw std::overflow_error if the iterator is past the end.
  */
  std::tuple<T> get_curr() const
  {
    return std::tuple<T>(C::Iterator::get_curr());
  }

  /** Return the current tuple (no-throw variant).

      Requires `has_curr()`.
  */
  std::tuple<T> get_curr_ne() const noexcept
  {
    return std::tuple<T>(C::Iterator::get_curr_ne());
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
    @ingroup Algorithms
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
    @ingroup Algorithms
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
    @ingroup Algorithms
*/
template <class ... Cs>
[[nodiscard]] inline
bool equal_length(const Cs &... cs)
{
  auto it = get_zip_it(cs...);
  for (/* already initialized */; it.has_curr(); it.next_ne())
    /* empty */;

  return it.completed();
}

/** Traverse zipped containers while `op` returns `true`.

    Iterates over zipped tuples, calling the predicate for each.
    Stops early if the predicate returns false. Iterates over the
    minimum length of all containers.
    
    @tparam Op Predicate type `(tuple) -> bool`.
    @tparam Cs Container types.
    @param op Callable returning `bool`; traversal stops on `false`.
    @param cs Containers to zip together.
    @return `true` if `op` returned `true` for every tuple, `false` otherwise.
    
    @par Example:
    @code
    DynList<int> xs = {1, 2, 3, 4};
    DynList<int> ys = {10, 20, 30, 40};
    
    // Check if all pairs have x < y
    bool all_less = zip_traverse([](auto t) {
      return std::get<0>(t) < std::get<1>(t);
    }, xs, ys);
    // all_less is true
    @endcode
    
    @see zip_traverse_eq for version that verifies equal lengths
    @ingroup Algorithms
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
    @ingroup Algorithms
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

    Iterates over all zipped tuples, applying the callable to each.
    This is a side-effect operation (returns void). Iterates over
    the minimum length of all containers.
    
    @tparam Op Callable type `(tuple) -> void`.
    @tparam Cs Container types.
    @param op Callable to invoke on each tuple (for side effects).
    @param cs Containers to zip together.
    
    @par Example:
    @code
    DynList<std::string> names = {"Alice", "Bob"};
    DynList<int> ages = {30, 25};
    
    zip_for_each([](auto t) {
      std::cout << std::get<0>(t) << " is " 
                << std::get<1>(t) << " years old\n";
    }, names, ages);
    // Output:
    // Alice is 30 years old
    // Bob is 25 years old
    @endcode
    
    @see zip_for_each_eq for version that verifies equal lengths
    @see zip_for_each_indexed for version with index
    @ingroup Algorithms
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
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
inline
void zip_for_each_eq(Op &&op, const Cs &... cs)
{
  auto it = get_zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    std::forward<Op>(op)(it.get_curr());

  ah_length_error_if(not it.completed())
    << "zip_for_each_eq() containers sizes mismatch";
}

/** Return `true` if `op` returns `true` for all tuples and containers have equal length.

    This is like ML's `forall` but also verifies that all containers have
    the same length. Use `zip_all_short` if you don't need length verification.

    @param op Predicate.
    @param cs Containers to zip.
    @return `true` iff `op` is `true` for all tuples and lengths match.
    @ingroup Algorithms
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

/** Return `true` if `op` returns `true` for all tuples (without length check).

    Like ML's `forall`. Iterates over the minimum length of all containers.
    Does NOT verify that containers have equal length.

    @param op Predicate.
    @param cs Containers to zip.
    @return `true` iff `op` is `true` for all tuples.
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_all_short(Op &&op, const Cs &... cs)
{
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (not std::forward<Op>(op)(it.get_curr()))
      return false;

  return true;
}

/** Alias for `zip_all`. Like ML's `forall` with length verification.
    @see zip_all
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_forall(Op &&op, const Cs &... cs)
{
  return zip_all(std::forward<Op>(op), cs...);
}

/** Alias for `zip_all_short`. Like ML's `forall` without length verification.
    @see zip_all_short
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_forall_short(Op &&op, const Cs &... cs)
{
  return zip_all_short(std::forward<Op>(op), cs...);
}

/** Return `true` if `op` returns `true` for at least one tuple.

    Like ML's `exists`. Iterates over the minimum length of all containers.

    @param op Predicate.
    @param cs Containers to zip.
    @return `true` if any tuple satisfies `op`.
    @ingroup Algorithms
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

/** Alias for `zip_exists`. Like Python/JS `any()`.
    @see zip_exists
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_any(Op &&op, const Cs &... cs)
{
  return zip_exists(std::forward<Op>(op), cs...);
}

/** Return `true` if `op` returns `false` for all tuples.

    This is the complement of `zip_exists()`.

    @param op Predicate.
    @param cs Containers to zip.
    @return `true` if no tuple satisfies `op`.
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
bool zip_none(Op &&op, const Cs &... cs)
{
  return not zip_exists(std::forward<Op>(op), cs...);
}

/** Count tuples that satisfy predicate `op`.

    @param op Predicate.
    @param cs Containers to zip.
    @return Number of tuples for which `op` returns `true`.
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]] inline
size_t zip_count(Op &&op, const Cs &... cs)
{
  size_t count = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (std::forward<Op>(op)(it.get_curr()))
      ++count;
  return count;
}

/** Count all tuples (minimum length of containers).

    @param cs Containers to zip.
    @return Number of tuples that would be produced.
    @ingroup Algorithms
*/
template <class ... Cs>
[[nodiscard]] inline
size_t zip_length(const Cs &... cs)
{
  size_t count = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    ++count;
  return count;
}

/** Return the first tuple that satisfies predicate `op`.

    @param op Predicate.
    @param cs Containers to zip.
    @return A pair (tuple, found). If found is true, tuple contains the match.
            If found is false, tuple is default-constructed.
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_find_first(Op &&op, const Cs &... cs)
{
  using ZipType = typename ZipIterator<Cs...>::Tuple_Type;
  
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (auto t = it.get_curr(); std::forward<Op>(op)(t))
      return std::make_pair(t, true);
  
  return std::make_pair(ZipType{}, false);
}

/** Return the last tuple that satisfies predicate `op`.

    @param op Predicate.
    @param cs Containers to zip.
    @return A pair (tuple, found). If found is true, tuple contains the match.
            If found is false, tuple is default-constructed.
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_find_last(Op &&op, const Cs &... cs)
{
  using ZipType = typename ZipIterator<Cs...>::Tuple_Type;
  
  ZipType result{};
  bool found = false;
  
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    if (auto t = it.get_curr(); std::forward<Op>(op)(t))
      {
        result = t;
        found = true;
      }
  
  return std::make_pair(result, found);
}

/** Return the n-th tuple from zipped containers.

    @param n Index (0-based).
    @param cs Containers to zip.
    @return A pair (tuple, found). If found is true, tuple contains the element.
            If n >= zip_length, found is false and tuple is default-constructed.
    @ingroup Algorithms
*/
template <class ... Cs>
[[nodiscard]]
auto zip_nth(size_t n, const Cs &... cs)
{
  using ZipType = typename ZipIterator<Cs...>::Tuple_Type;
  
  size_t i = 0;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne(), ++i)
    if (i == n)
      return std::make_pair(it.get_curr(), true);
  
  return std::make_pair(ZipType{}, false);
}

/** Return the first tuple from zipped containers.

    @param cs Containers to zip.
    @return A pair (tuple, found). If found is true, tuple contains the element.
            If containers are empty, found is false.
    @ingroup Algorithms
*/
template <class ... Cs>
[[nodiscard]]
auto zip_first(const Cs &... cs)
{
  return zip_nth(0, cs...);
}

/** Map `op` over zipped tuples, returning a list of results.

    Applies a transformation function to each tuple from the zipped containers
    and collects the results in a DynList. The result type must be explicitly
    specified. For auto-deduced return type, use `zip_map` instead.
    
    @tparam T Result type (must be explicitly specified).
    @tparam Op Transformation callable type.
    @tparam Cs Container types.
    @param op Transformation callable that takes a tuple and returns T.
    @param cs Containers to zip together.
    @return DynList<T> of transformed values.
    
    @par Example:
    @code
    DynList<int> xs = {1, 2, 3};
    DynList<int> ys = {10, 20, 30};
    
    // Create strings from each pair
    auto result = zip_maps<std::string>([](auto t) {
      return std::to_string(std::get<0>(t)) + "+" + 
             std::to_string(std::get<1>(t));
    }, xs, ys);
    // result contains: {"1+10", "2+20", "3+30"}
    @endcode
    
    @see zip_map for auto-deduced return type version
    @see zip_maps_eq for version that verifies equal lengths
    @ingroup Algorithms
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
    @ingroup Algorithms
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

/** Left fold (reduce) over zipped tuples.

    Accumulates a value by applying a binary operation to the accumulator
    and each tuple from the zipped containers, from left to right.
    This is equivalent to ML's `foldl` or JavaScript's `reduce`.
    
    @tparam T Accumulator type.
    @tparam Op Binary operation type `(T, tuple) -> T`.
    @tparam Cs Container types.
    @param init Initial accumulator value.
    @param op Binary operation that combines accumulator with each tuple.
    @param cs Containers to zip together.
    @return Final accumulated value after processing all tuples.
    
    @par Example:
    @code
    DynList<int> prices = {10, 20, 30};
    DynList<int> quantities = {2, 3, 1};
    
    // Calculate total cost: sum of (price * quantity)
    int total = zip_foldl(0, [](int acc, auto t) {
      return acc + std::get<0>(t) * std::get<1>(t);
    }, prices, quantities);
    // total = 0 + 10*2 + 20*3 + 30*1 = 110
    @endcode
    
    @ingroup Algorithms
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

/** Left fold over zipped tuples; throw if containers differ in length.

    @tparam T Accumulator type.
    @param init Initial accumulator value.
    @param op Binary operation `(T, tuple) -> T`.
    @param cs Containers to zip.
    @return Final accumulated value.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algorithms
*/
template <typename T, class Op, class ... Cs>
[[nodiscard]]
T zip_foldl_eq(const T &init, Op op, const Cs &... cs)
{
  T acu = init;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    acu = op(acu, it.get_curr());

  ah_length_error_if(not it.completed())
    << "zip_foldl_eq() containers sizes mismatch";

  return acu;
}

/** Map `op` over zipped tuples; throw if containers differ in length.

    @tparam T Result type.
    @param op Transformation callable.
    @param cs Containers to zip.
    @return DynList of transformed values.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algorithms
*/
template <typename T, class Op, class ... Cs>
[[nodiscard]]
DynList<T> zip_maps_eq(Op op, const Cs &... cs)
{
  DynList<T> ret;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    ret.append(op(it.get_curr()));

  ah_length_error_if(not it.completed())
    << "zip_maps_eq() containers sizes mismatch";

  return ret;
}

/** Map `op` over zipped tuples that satisfy `prop`; throw if lengths differ.

    @tparam T Result type.
    @param prop Predicate selecting which tuples to transform.
    @param op Transformation callable.
    @param cs Containers to zip.
    @return DynList of transformed values for tuples where `prop` is `true`.
    @throw std::length_error if container sizes mismatch.
    @ingroup Algorithms
*/
template <typename T, class Prop, class Op, class ... Cs>
[[nodiscard]]
DynList<T> zip_maps_if_eq(Prop prop, Op op, const Cs &... cs)
{
  DynList<T> ret;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    if (auto t = it.get_curr(); prop(t))
      ret.append(op(t));

  ah_length_error_if(not it.completed())
    << "zip_maps_if_eq() containers sizes mismatch";

  return ret;
}

/** Map `op` over zipped tuples with auto-deduced return type.

    Unlike `zip_maps<T>`, this version automatically deduces the result type
    from the return type of `op`.

    @param op Transformation callable.
    @param cs Containers to zip.
    @return DynList of transformed values (type auto-deduced).
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_map(Op &&op, const Cs &... cs)
{
  using ZipType = typename ZipIterator<Cs...>::Tuple_Type;
  using ResultType = std::decay_t<decltype(std::forward<Op>(op)(std::declval<ZipType>()))>;

  DynList<ResultType> ret;
  for (auto it = zip_it(cs...); it.has_curr(); it.next_ne())
    ret.append(std::forward<Op>(op)(it.get_curr()));
  return ret;
}

/** Map `op` over zipped tuples with auto-deduced return type; throw if lengths differ.

    @param op Transformation callable.
    @param cs Containers to zip.
    @return DynList of transformed values (type auto-deduced).
    @throw std::length_error if container sizes mismatch.
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_map_eq(Op &&op, const Cs &... cs)
{
  using ZipType = typename ZipIterator<Cs...>::Tuple_Type;
  using ResultType = std::decay_t<decltype(std::forward<Op>(op)(std::declval<ZipType>()))>;

  DynList<ResultType> ret;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    ret.append(std::forward<Op>(op)(it.get_curr()));

  ah_length_error_if(not it.completed())
    << "zip_map_eq() containers sizes mismatch";

  return ret;
}

/** Filter zipped tuples by predicate `op`.

    Selects tuples from the zipped containers where the predicate returns true.
    Iterates over the minimum length of all containers.

    @param op Predicate function that takes a tuple and returns bool.
    @param cs Containers to zip together.
    @return DynList of tuples for which `op` returned `true`.
    
    @par Example:
    @code
    DynList<int> nums = {1, 2, 3, 4, 5};
    DynList<int> vals = {10, 20, 30, 40, 50};
    
    // Filter tuples where first element is even
    auto result = zip_filter([](auto t) { 
      return std::get<0>(t) % 2 == 0; 
    }, nums, vals);
    // result contains: {(2, 20), (4, 40)}
    @endcode
    
    @ingroup Algorithms
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

/** Filter zipped tuples by predicate `op`; throw if containers differ in length.

    Like `zip_filter`, but additionally verifies that all containers have
    the same length after iteration.

    @param op Predicate function that takes a tuple and returns bool.
    @param cs Containers to zip together.
    @return DynList of tuples for which `op` returned `true`.
    @throw std::length_error if container sizes mismatch.
    
    @par Example:
    @code
    DynList<int> nums = {1, 2, 3};
    DynList<int> vals = {10, 20, 30};
    
    // This will succeed (equal lengths)
    auto result = zip_filter_eq([](auto t) { 
      return std::get<0>(t) > 1; 
    }, nums, vals);
    
    DynList<int> short_list = {1, 2};
    // This will throw std::length_error
    auto bad = zip_filter_eq([](auto) { return true; }, nums, short_list);
    @endcode
    
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
[[nodiscard]]
auto zip_filter_eq(Op op, const Cs &... cs)
{
  using ZipType = decltype(zip_it(cs...).get_curr());

  DynList<ZipType> ret;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne())
    if (auto t = it.get_curr(); op(t))
      ret.append(t);

  ah_length_error_if(not it.completed())
    << "zip_filter_eq() containers sizes mismatch";

  return ret;
}

/** @internal
 *  @brief Internal helpers for zip_cmp comparison across adjacent tuple elements.
 */
namespace zip_detail
{
  /** Compare all adjacent pairs in a tuple using the given comparator.
   *  @param cmp Binary comparator.
   *  @param t Tuple to compare.
   *  @return true if cmp(t[i], t[i+1]) is true for all adjacent pairs.
   */
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
[[nodiscard]]
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
    @ingroup Algorithms
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

    Separates zipped tuples into two groups based on a predicate:
    those for which the predicate returns true, and those for which
    it returns false. Also returns the count of each group.
    
    @tparam Op Predicate type `(tuple) -> bool`.
    @tparam Cs Container types.
    @param op Predicate to partition by.
    @param cs Containers to zip together.
    @return A tuple of four elements:
            - `std::get<0>`: DynList of tuples where `op` returned true
            - `std::get<1>`: Count of matching tuples (size_t)
            - `std::get<2>`: DynList of tuples where `op` returned false
            - `std::get<3>`: Count of non-matching tuples (size_t)
    
    @par Example:
    @code
    DynList<int> nums = {1, 2, 3, 4, 5};
    DynList<std::string> labels = {"a", "b", "c", "d", "e"};
    
    // Partition by even numbers
    auto [evens, even_count, odds, odd_count] = 
      zip_partition([](auto t) { 
        return std::get<0>(t) % 2 == 0; 
      }, nums, labels);
    
    // evens contains: {(2, "b"), (4, "d")}
    // even_count is 2
    // odds contains: {(1, "a"), (3, "c"), (5, "e")}
    // odd_count is 3
    @endcode
    
    @ingroup Algorithms
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

  return std::make_tuple(ret1, n1, ret2, n2);
}

/** Materialize zipped tuples into a DynList.

    Collects all zipped tuples into a DynList. This is useful when you
    need to store the zipped result for later use or multiple iterations.
    Iterates over the minimum length of all containers.
    
    @tparam Cs Container types.
    @param cs Containers to zip together.
    @return DynList of tuples containing all zipped elements.
    
    @par Example:
    @code
    DynList<int> xs = {1, 2, 3};
    DynList<std::string> ys = {"a", "b", "c"};
    
    auto zipped = t_zip(xs, ys);
    // zipped is DynList<std::tuple<int, string>> containing:
    // {(1, "a"), (2, "b"), (3, "c")}
    
    // Can iterate multiple times
    for (auto& t : zipped) { ... }
    for (auto& t : zipped) { ... }
    @endcode
    
    @see t_zip_eq for version that verifies equal lengths
    @see t_unzip for the inverse operation
    @ingroup Algorithms
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
    @ingroup Algorithms
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

  ah_length_error_if(not it.completed())
    << "Container sizes mismatch";

  return ret;
}

/**
 * @class EnumZipIterator
 * @brief Iterator that traverses multiple containers with enumeration index.
 *
 * Like ZipIterator, but appends the current index as the last element
 * in each tuple. Useful when you need both the position and values.
 *
 * @tparam HeadC Type of the first container.
 * @tparam TailC Types of remaining containers.
 *
 * Usage example:
 * @code
 * DynList<int> ints = {10, 20, 30};
 * DynList<std::string> strs = {"a", "b", "c"};
 *
 * for (auto it = enum_zip_it(ints, strs); it.has_curr(); it.next_ne())
 *   {
 *     auto t = it.get_curr_ne();
 *     // t is std::tuple<int, string, size_t> where last element is the index
 *     std::cout << std::get<2>(t) << ": " << std::get<0>(t) << '\n';
 *   }
 * @endcode
 *
 * @see get_enum_zip_it(), enum_zip_it(), ZipIterator
 * @ingroup Algorithms
 * @cond DOXYGEN_SHOULD_SKIP_THIS
 */
template <class HeadC, class ... TailC>
class EnumZipIterator : public EnumZipIterator<TailC...>
/// @endcond
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
    std::tuple<T> curr(it.get_curr());
    return std::tuple_cat(curr, EnumZipIterator<TailC...>::get_curr());
  }

  /** Advance all underlying iterators (bounds-checked). */
  void next()
  {
    it.next();
    EnumZipIterator<TailC...>::next();
  }

  auto get_curr_ne() const noexcept
  {
    std::tuple<T> curr(it.get_curr_ne());
    return std::tuple_cat(curr, EnumZipIterator<TailC...>::get_curr_ne());
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
  std::tuple<T, size_t> get_curr() const
  {
    return std::tuple<T, size_t>(C::Iterator::get_curr(), i);
  }

  /** Return the current tuple `(value, index)` (no-throw variant).

      Requires `has_curr()`.
  */
  std::tuple<T, size_t> get_curr_ne() const noexcept
  {
    return std::tuple<T, size_t>(C::Iterator::get_curr_ne(), i);
  }

  /** Return `true` if the underlying iterator has reached the end. */
  [[nodiscard]] bool completed() const noexcept { return not this->has_curr(); }
};

/** Create an EnumZipIterator over the given containers.

    @param cs Containers to zip.
    @return EnumZipIterator positioned at the first element (index 0).
    @ingroup Algorithms
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
    @ingroup Algorithms
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
    @ingroup Algorithms
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
    @ingroup Algorithms
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

  ah_length_error_if(not it.completed())
    << "Container sizes mismatch";

  return ret;
}

template <typename... Ts, size_t... is>
static inline
auto t_unzip_impl(const DynList<std::tuple<Ts...>> &l,
                  std::index_sequence<is...>)
{
  std::tuple<DynList<Ts>...> ret;
  for (auto it = l.get_it(); it.has_curr(); it.next_ne())
    std::initializer_list<int>{(std::get<is>(ret).append(std::get<is>(it.get_curr())), 0)...};

  return ret;
}

/** Unzip a list of tuples into a tuple of lists.

    @param tuples Input list of tuples.
    @return Tuple of DynLists, one per tuple element.
    @ingroup Algorithms
*/
template <class... Ts>
[[nodiscard]] inline
auto t_unzip(const DynList<std::tuple<Ts...>> &tuples)
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

    @ingroup Algorithms
*/
template <class C, typename ... Lists>
[[nodiscard]]
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
    @ingroup Algorithms
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

  ah_length_error_if(not it.completed())
    << "zip_list_eq: container sizes mismatch";

  return ret;
}

/** @internal
 *  @brief Internal helpers for STL container zipping (tzip_std, std_zip_n).
 */
namespace std_zip_detail
{
  /** Check if all iterators are valid (not at end).
   *  @param its Iterator pairs (begin, end).
   *  @return true if all first iterators != second (end).
   */
  template <typename... Its>
  bool all_valid(const Its&... its)
  {
    return (... and (its.first != its.second));
  }

  /** Advance all iterators.
   *  @param its Iterator pairs (begin, end).
   */
  template <typename... Its>
  void advance_all(Its&... its)
  {
    (++its.first, ...);
  }

  /** Dereference all iterators and make a tuple.
   *  @param its Iterator pairs (begin, end).
   *  @return Tuple of dereferenced values.
   */
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
  std::vector<std::pair<T, U>> result;
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
    std::vector<int> v1 = {1, 2, 3};
    std::vector<double> v2 = {1.1, 2.2, 3.3};
    std::vector<std::string> v3 = {"a", "b", "c"};
    auto result = tzip_std(v1, v2, v3);
    // result is std::vector<std::tuple<int, double, std::string>>
    @endcode
*/
template <typename... Cs>
[[nodiscard]]
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
    @ingroup Algorithms
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
    @ingroup Algorithms
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
  
  ah_length_error_if(not it.completed())
    << "zip_transform_eq() containers sizes mismatch";
  return ret;
}

/** Apply `op` to each zipped tuple along with its index.
    
    More direct than using `enum_zip_it` manually.
    
    @param op Callable `(size_t index, tuple) -> void`.
    @param cs Containers to zip.
    @ingroup Algorithms
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
    @ingroup Algorithms
*/
template <class Op, class ... Cs>
inline
void zip_for_each_indexed_eq(Op &&op, const Cs &... cs)
{
  size_t idx = 0;
  auto it = zip_it(cs...);
  for (; it.has_curr(); it.next_ne(), ++idx)
    std::forward<Op>(op)(idx, it.get_curr());
  
  ah_length_error_if(not it.completed())
    << "zip_for_each_indexed_eq() containers sizes mismatch";
}

/** Take at most `n` tuples from the zipped containers.
    
    @param n Maximum number of tuples to take.
    @param cs Containers to zip.
    @return DynList of up to `n` tuples.
    @ingroup Algorithms
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
    @ingroup Algorithms
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
    @ingroup Algorithms
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
    @ingroup Algorithms
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

/** @internal
 *  @brief Internal helpers for tuple operations using std::index_sequence.
 */
namespace zip_index_detail
{
  /** Apply a function to each element in a tuple.
   *  @param op Callable to apply.
   *  @param t Tuple to iterate.
   */
  template <class Op, class Tuple, size_t... Is>
  void for_each_in_tuple_impl(Op &&op, Tuple &&t, std::index_sequence<Is...>)
  {
    (std::forward<Op>(op)(std::get<Is>(std::forward<Tuple>(t))), ...);
  }
  
  /** Transform each element in a tuple.
   *  @param op Transformation to apply.
   *  @param t Input tuple.
   *  @return Tuple of transformed values.
   */
  template <class Op, class Tuple, size_t... Is>
  auto transform_tuple_impl(Op &&op, Tuple &&t, std::index_sequence<Is...>)
  {
    return std::make_tuple(std::forward<Op>(op)(std::get<Is>(std::forward<Tuple>(t)))...);
  }
  
  /** Check if all elements satisfy a predicate.
   *  @param pred Predicate to test.
   *  @param t Tuple to check.
   *  @return true if all elements satisfy pred.
   */
  template <class Pred, class Tuple, size_t... Is>
  bool all_of_tuple_impl(Pred &&pred, const Tuple &t, std::index_sequence<Is...>)
  {
    return (... and std::forward<Pred>(pred)(std::get<Is>(t)));
  }
  
  /** Check if any element satisfies a predicate.
   *  @param pred Predicate to test.
   *  @param t Tuple to check.
   *  @return true if any element satisfies pred.
   */
  template <class Pred, class Tuple, size_t... Is>
  bool any_of_tuple_impl(Pred &&pred, const Tuple &t, std::index_sequence<Is...>)
  {
    return (... or std::forward<Pred>(pred)(std::get<Is>(t)));
  }
}

/** Apply `op` to each element of a tuple.

    Iterates over all elements in the tuple at compile-time using
    `std::index_sequence`, applying the callable `op` to each element.
    The operation can modify elements if the tuple is passed by reference.
    
    @tparam Op Callable type (must be invocable with each tuple element type).
    @tparam Tuple Tuple type to iterate over.
    @param op Callable to apply to each element.
    @param t Tuple to iterate over (can be lvalue or rvalue reference).
    
    @par Example:
    @code
    auto t = std::make_tuple(1, 2.5, std::string("hello"));
    
    // Print each element
    for_each_in_tuple([](const auto& elem) { 
      std::cout << elem << " "; 
    }, t);
    // Output: 1 2.5 hello
    
    // Modify elements (requires non-const tuple)
    auto t2 = std::make_tuple(1, 2, 3);
    for_each_in_tuple([](auto& x) { x *= 2; }, t2);
    // t2 is now (2, 4, 6)
    @endcode
    
    @ingroup Algorithms
*/
template <class Op, class Tuple>
void for_each_in_tuple(Op &&op, Tuple &&t)
{
  constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
  zip_index_detail::for_each_in_tuple_impl(
    std::forward<Op>(op), std::forward<Tuple>(t), std::make_index_sequence<N>{});
}

/** Transform each element of a tuple, returning a new tuple.

    Creates a new tuple by applying a transformation function to each
    element of the input tuple. The result types are deduced from the
    return type of the transformation applied to each element.
    Uses `std::index_sequence` for compile-time iteration.
    
    @tparam Op Transformation callable type.
    @tparam Tuple Input tuple type.
    @param op Transformation to apply to each element.
    @param t Input tuple.
    @return New tuple containing transformed values.
    
    @par Example:
    @code
    auto t = std::make_tuple(1, 2, 3);
    
    // Double each element
    auto doubled = transform_tuple([](auto x) { return x * 2; }, t);
    // doubled is (2, 4, 6)
    
    // Convert to strings
    auto strings = transform_tuple([](auto x) { 
      return std::to_string(x); 
    }, t);
    // strings is ("1", "2", "3") with type std::tuple<string, string, string>
    @endcode
    
    @ingroup Algorithms
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

    Checks if every element in the tuple satisfies the given predicate.
    Uses fold expressions for compile-time iteration over tuple elements.
    For an empty tuple, returns true (vacuous truth).
    
    @tparam Pred Predicate type (must be callable with each tuple element type).
    @tparam Tuple Tuple type to check.
    @param pred Predicate to test on each element.
    @param t Tuple to check.
    @return `true` if `pred` returns `true` for all elements.
    
    @par Example:
    @code
    auto t = std::make_tuple(2, 4, 6);
    bool all_even = all_of_tuple([](auto x) { return x % 2 == 0; }, t);
    // all_even is true
    @endcode
    
    @ingroup Algorithms
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

    Checks if at least one element in the tuple satisfies the given predicate.
    Uses fold expressions for compile-time iteration over tuple elements.
    
    @tparam Pred Predicate type (must be callable with each tuple element type).
    @tparam Tuple Tuple type to check.
    @param pred Predicate to test on each element.
    @param t Tuple to check.
    @return `true` if `pred` returns `true` for at least one element.
    
    @par Example:
    @code
    auto t = std::make_tuple(1, -5, 3);
    bool has_negative = any_of_tuple([](auto x) { return x < 0; }, t);
    // has_negative is true (because -5 < 0)
    @endcode
    
    @ingroup Algorithms
*/
template <class Pred, class Tuple>
[[nodiscard]]
bool any_of_tuple(Pred &&pred, const Tuple &t)
{
  constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
  return zip_index_detail::any_of_tuple_impl(
    std::forward<Pred>(pred), t, std::make_index_sequence<N>{});
}

/** Return `true` if no tuple element satisfies `pred`.

    This is the complement of `any_of_tuple`. Returns true if the predicate
    returns false for all elements in the tuple.
    
    @tparam Pred Predicate type (must be callable with each tuple element type).
    @tparam Tuple Tuple type to check.
    @param pred Predicate to test on each element.
    @param t Tuple to check.
    @return `true` if `pred` returns `false` for all elements.
    
    @par Example:
    @code
    auto t = std::make_tuple(1, 2, 3);
    bool no_negatives = none_of_tuple([](auto x) { return x < 0; }, t);
    // no_negatives is true (no element is negative)
    @endcode
    
    @ingroup Algorithms
*/
template <class Pred, class Tuple>
[[nodiscard]]
bool none_of_tuple(Pred &&pred, const Tuple &t)
{
  return not any_of_tuple(std::forward<Pred>(pred), t);
}

} // end namespace Aleph

# endif // AH_ZIP_H

