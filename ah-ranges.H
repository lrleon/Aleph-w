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

/**
 * @file ah-ranges.H
 * @brief C++20 Ranges support and adaptors for Aleph-w containers.
 *
 * This header provides:
 * - **Range adaptors** for Aleph containers (DynList, DynArray, etc.)
 * - **Internal utilities** using std::ranges for improved performance
 * - **Lazy range views** compatible with Aleph containers
 *
 * ## Requirements
 * - C++20 or later with ranges support
 * - GCC 10+, Clang 13+, or MSVC 19.29+
 *
 * ## Usage Example
 * @code
 * #include <ah-ranges.H>
 * #include <htlist.H>
 *
 * DynList<int> list = {1, 2, 3, 4, 5};
 *
 * // Use with std::ranges algorithms
 * bool has_even = std::ranges::any_of(list, [](int x) { return x % 2 == 0; });
 *
 * // Use with views
 * for (auto x : list | std::views::filter([](int x) { return x % 2 == 0; }))
 *   std::cout << x << " ";  // prints: 2 4
 *
 * // Use internal lazy range generation
 * for (auto x : Aleph::lazy_range(1, 10))
 *   std::cout << x << " ";  // prints: 1 2 3 4 5 6 7 8 9
 * @endcode
 *
 * @note The public API of ahFunctional.H is preserved. This header provides
 *       internal optimizations and additional range-based utilities.
 *
 * @ingroup Algos
 * @author Leandro Rabindranath Leon
 */

#ifndef AH_RANGES_H
#define AH_RANGES_H

// Check for C++20 ranges support
#if __cplusplus >= 202002L && __has_include(<ranges>)
#  define ALEPH_HAS_RANGES 1
#  include <ranges>
#  include <algorithm>
#  include <iterator>
#  include <concepts>
#else
#  define ALEPH_HAS_RANGES 0
#endif

// Feature detection for C++23 views
#ifdef __cpp_lib_ranges_stride
#  define ALEPH_HAS_STRIDE 1
#else
#  define ALEPH_HAS_STRIDE 0
#endif

#ifdef __cpp_lib_ranges_repeat
#  define ALEPH_HAS_REPEAT 1
#else
#  define ALEPH_HAS_REPEAT 0
#endif

#ifdef __cpp_lib_ranges_zip
#  define ALEPH_HAS_ZIP 1
#else
#  define ALEPH_HAS_ZIP 0
#endif

#ifdef __cpp_lib_ranges_enumerate
#  define ALEPH_HAS_ENUMERATE 1
#else
#  define ALEPH_HAS_ENUMERATE 0
#endif

#include <type_traits>
#include <utility>
#include <tuple>
#include <functional>

namespace Aleph
{

// Forward declarations for major Aleph containers with simple template signatures.
// Containers with complex template signatures (like DynSetTree, DynBinHeap) 
// should use the generic to<Container>() adaptor instead.
template <typename T> class DynList;
template <typename T> class DynArray;
template <typename T> class DynDlist;
template <typename T> class DynListStack;
template <typename T> class DynListQueue;
template <typename T> class ArrayStack;
template <typename T> class ArrayQueue;
template <typename T> class Random_Set;

#if ALEPH_HAS_RANGES

// ============================================================================
// Concepts for Aleph containers
// ============================================================================

/**
 * @brief Concept for Aleph containers that support iteration.
 *
 * A type satisfies AlephContainer if it has:
 * - A nested `Iterator` type
 * - A nested `Item_Type` type
 * - `begin()` and `end()` methods returning iterators
 */
template <typename C>
concept AlephContainer = requires(C c) {
  typename C::Iterator;
  typename C::Item_Type;
  { c.begin() } -> std::input_or_output_iterator;
  { c.end() } -> std::sentinel_for<decltype(c.begin())>;
};

/**
 * @brief Concept for Aleph containers that support append.
 */
template <typename C>
concept AlephAppendable = AlephContainer<C> && requires(C c, typename C::Item_Type v) {
  c.append(v);
};

/**
 * @brief Concept for types that can be used with std::ranges.
 */
template <typename R>
concept RangeLike = std::ranges::range<R>;

// ============================================================================
// Range Adaptors - Convert ranges to Aleph containers
// ============================================================================

/**
 * @brief Materialize a range into an Aleph container.
 *
 * Converts any range (including lazy views) into an Aleph container
 * by appending each element.
 *
 * @tparam Container Target Aleph container type (must be fully specified).
 * @tparam R Source range type.
 * @param r Source range.
 * @return Container with all elements from the range.
 *
 * @par Example:
 * @code
 * auto view = std::views::iota(1, 10) | std::views::filter([](int x) { return x % 2 == 0; });
 * DynList<int> list = to_aleph<DynList<int>>(view);  // {2, 4, 6, 8}
 * @endcode
 */
template <AlephAppendable Container, RangeLike R>
[[nodiscard]] Container to_aleph(R&& r)
{
  Container result;
  for (auto&& elem : r)
    result.append(std::forward<decltype(elem)>(elem));
  return result;
}

// ============================================================================
// Lazy Range Generation (zero-allocation until materialized)
// ============================================================================

/**
 * @brief Generate a lazy range [start, end).
 *
 * Unlike `range()` in ahFunctional.H which allocates immediately,
 * this returns a lazy view that generates values on demand.
 *
 * @tparam T Numeric type.
 * @param start Starting value (inclusive).
 * @param end Ending value (exclusive).
 * @return A lazy range view.
 *
 * @par Example:
 * @code
 * // No allocation - lazy evaluation
 * for (auto x : lazy_range(0, 1000000))
 *   if (x > 10) break;  // Only generates 11 values
 *
 * // Materialize when needed
 * auto list = to_aleph<DynList<int>>(lazy_range(1, 5));
 * @endcode
 *
 * @note This is for internal use. The public API `range()` in ahFunctional.H
 *       is preserved for backward compatibility.
 */
template <typename T = int>
[[nodiscard]] constexpr auto lazy_range(T start, T end)
{
  return std::views::iota(start, end);
}

/**
 * @brief Generate a lazy range [0, n).
 *
 * @param n Number of elements.
 * @return A lazy range view from 0 to n-1.
 */
template <typename T = int>
[[nodiscard]] constexpr auto lazy_range(T n)
{
  return std::views::iota(T{0}, n);
}

/**
 * @brief Generate a lazy infinite range starting from a value.
 *
 * @param start Starting value.
 * @return An infinite lazy range view.
 *
 * @warning Must be used with take() or similar to avoid infinite loop.
 *
 * @par Example:
 * @code
 * auto first_10_evens = lazy_iota(0) 
 *                     | std::views::filter([](int x) { return x % 2 == 0; })
 *                     | std::views::take(10);
 * @endcode
 */
template <typename T = int>
[[nodiscard]] constexpr auto lazy_iota(T start = T{0})
{
  return std::views::iota(start);
}

#if ALEPH_HAS_REPEAT
/**
 * @brief Repeat a value n times (lazy).
 *
 * @param value The value to repeat.
 * @param n Number of repetitions.
 * @return A lazy range view.
 */
template <typename T>
[[nodiscard]] constexpr auto lazy_repeat(const T& value, size_t n)
{
  return std::views::repeat(value) | std::views::take(n);
}
#endif

// ============================================================================
// Internal Utilities using Ranges (for performance optimization)
// ============================================================================

namespace detail
{

/**
 * @brief Check if all elements satisfy a predicate using ranges.
 *
 * Used internally to optimize `all()` when ranges are available.
 */
template <RangeLike R, typename Pred>
[[nodiscard]] constexpr bool ranges_all_of(R&& r, Pred&& pred)
{
  return std::ranges::all_of(std::forward<R>(r), std::forward<Pred>(pred));
}

/**
 * @brief Check if any element satisfies a predicate using ranges.
 *
 * Used internally to optimize `exists()` when ranges are available.
 */
template <RangeLike R, typename Pred>
[[nodiscard]] constexpr bool ranges_any_of(R&& r, Pred&& pred)
{
  return std::ranges::any_of(std::forward<R>(r), std::forward<Pred>(pred));
}

/**
 * @brief Check if no element satisfies a predicate using ranges.
 *
 * Used internally to optimize `none()` when ranges are available.
 */
template <RangeLike R, typename Pred>
[[nodiscard]] constexpr bool ranges_none_of(R&& r, Pred&& pred)
{
  return std::ranges::none_of(std::forward<R>(r), std::forward<Pred>(pred));
}

/**
 * @brief Find first element satisfying predicate using ranges.
 *
 * Used internally to optimize `find_if()`.
 */
template <RangeLike R, typename Pred>
[[nodiscard]] constexpr auto ranges_find_if(R&& r, Pred&& pred)
{
  return std::ranges::find_if(std::forward<R>(r), std::forward<Pred>(pred));
}

/**
 * @brief Count elements satisfying predicate using ranges.
 */
template <RangeLike R, typename Pred>
[[nodiscard]] constexpr auto ranges_count_if(R&& r, Pred&& pred)
{
  return std::ranges::count_if(std::forward<R>(r), std::forward<Pred>(pred));
}

/**
 * @brief Transform range elements using ranges.
 *
 * Returns a lazy view - must be materialized if container output needed.
 */
template <RangeLike R, typename Func>
[[nodiscard]] constexpr auto ranges_transform(R&& r, Func&& func)
{
  return std::forward<R>(r) | std::views::transform(std::forward<Func>(func));
}

/**
 * @brief Filter range elements using ranges.
 *
 * Returns a lazy view - must be materialized if container output needed.
 */
template <RangeLike R, typename Pred>
[[nodiscard]] constexpr auto ranges_filter(R&& r, Pred&& pred)
{
  return std::forward<R>(r) | std::views::filter(std::forward<Pred>(pred));
}

/**
 * @brief Take first n elements using ranges.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_take(R&& r, size_t n)
{
  return std::forward<R>(r) | std::views::take(n);
}

/**
 * @brief Drop first n elements using ranges.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_drop(R&& r, size_t n)
{
  return std::forward<R>(r) | std::views::drop(n);
}

/**
 * @brief Reverse range using ranges.
 *
 * @note Requires bidirectional range.
 */
template <RangeLike R>
  requires std::ranges::bidirectional_range<R>
[[nodiscard]] constexpr auto ranges_reverse(R&& r)
{
  return std::forward<R>(r) | std::views::reverse;
}

/**
 * @brief Flatten nested ranges using join.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_flatten(R&& r)
{
  return std::forward<R>(r) | std::views::join;
}

#if ALEPH_HAS_ZIP
/**
 * @brief Zip multiple ranges together.
 */
template <RangeLike... Rs>
[[nodiscard]] constexpr auto ranges_zip(Rs&&... rs)
{
  return std::views::zip(std::forward<Rs>(rs)...);
}
#endif

#if ALEPH_HAS_ENUMERATE
/**
 * @brief Enumerate range elements (returns pairs of index, value).
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_enumerate(R&& r)
{
  return std::forward<R>(r) | std::views::enumerate;
}
#endif

/**
 * @brief Fold left (reduce) operation using ranges (C++23) or fallback.
 *
 * @note C++20 doesn't have fold_left, so we provide a fallback.
 */
template <RangeLike R, typename T, typename BinaryOp>
[[nodiscard]] constexpr T ranges_fold_left(R&& r, T init, BinaryOp&& op)
{
#ifdef __cpp_lib_ranges_fold
  return std::ranges::fold_left(std::forward<R>(r), init, std::forward<BinaryOp>(op));
#else
  // Fallback for C++20 - use explicit variable to avoid rvalue issues
  for (auto&& elem : r)
    init = op(init, elem);
  return init;
#endif
}

/**
 * @brief Sum elements in a range.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_sum(R&& r)
{
  using T = std::ranges::range_value_t<R>;
  return ranges_fold_left(std::forward<R>(r), T{0}, std::plus<>{});
}

/**
 * @brief Product of elements in a range.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_product(R&& r)
{
  using T = std::ranges::range_value_t<R>;
  return ranges_fold_left(std::forward<R>(r), T{1}, std::multiplies<>{});
}

/**
 * @brief Get minimum element in a range.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_min(R&& r)
{
  return std::ranges::min_element(std::forward<R>(r));
}

/**
 * @brief Get maximum element in a range.
 */
template <RangeLike R>
[[nodiscard]] constexpr auto ranges_max(R&& r)
{
  return std::ranges::max_element(std::forward<R>(r));
}

/**
 * @brief Sort a range in place.
 */
template <RangeLike R, typename Comp = std::less<>>
constexpr void ranges_sort(R&& r, Comp&& comp = Comp{})
{
  std::ranges::sort(std::forward<R>(r), std::forward<Comp>(comp));
}

} // namespace detail

// ============================================================================
// Pipe Operator Support for Aleph Containers
// ============================================================================

/**
 * @brief Tag type for to_aleph pipe adaptor.
 */
template <template <typename> class Container>
struct to_aleph_adaptor
{
  template <RangeLike R>
  [[nodiscard]] friend auto operator|(R&& r, to_aleph_adaptor)
  {
    using T = std::ranges::range_value_t<R>;
    Container<T> result;
    for (auto&& elem : r)
      result.append(std::forward<decltype(elem)>(elem));
    return result;
  }
};

/**
 * @brief Pipe adaptor to convert range to DynList.
 *
 * @par Example:
 * @code
 * auto list = std::views::iota(1, 10) 
 *           | std::views::filter([](int x) { return x % 2 == 0; })
 *           | to_dynlist_v;
 * @endcode
 */
inline constexpr to_aleph_adaptor<DynList> to_dynlist_v{};

/**
 * @brief Pipe adaptor to convert range to DynArray.
 */
inline constexpr to_aleph_adaptor<DynArray> to_dynarray_v{};

/**
 * @brief Pipe adaptor to convert range to DynDlist (double-linked list).
 */
inline constexpr to_aleph_adaptor<DynDlist> to_dyndlist_v{};

/**
 * @brief Tag type for stack containers (uses push instead of append).
 */
template <template <typename> class Container>
struct to_aleph_push_adaptor
{
  template <RangeLike R>
  [[nodiscard]] friend auto operator|(R&& r, to_aleph_push_adaptor)
  {
    using T = std::ranges::range_value_t<R>;
    Container<T> result;
    for (auto&& elem : r)
      result.push(std::forward<decltype(elem)>(elem));
    return result;
  }
};

/**
 * @brief Pipe adaptor to convert range to DynListStack.
 * @note Elements are pushed in order, so first element of range will be at bottom.
 */
inline constexpr to_aleph_push_adaptor<DynListStack> to_dynliststack_v{};

/**
 * @brief Pipe adaptor to convert range to ArrayStack.
 */
inline constexpr to_aleph_push_adaptor<ArrayStack> to_arraystack_v{};

/**
 * @brief Tag type for queue containers (uses put instead of append).
 */
template <template <typename> class Container>
struct to_aleph_put_adaptor
{
  template <RangeLike R>
  [[nodiscard]] friend auto operator|(R&& r, to_aleph_put_adaptor)
  {
    using T = std::ranges::range_value_t<R>;
    Container<T> result;
    for (auto&& elem : r)
      result.put(std::forward<decltype(elem)>(elem));
    return result;
  }
};

/**
 * @brief Pipe adaptor to convert range to DynListQueue.
 */
inline constexpr to_aleph_put_adaptor<DynListQueue> to_dynlistqueue_v{};

/**
 * @brief Pipe adaptor to convert range to ArrayQueue.
 */
inline constexpr to_aleph_put_adaptor<ArrayQueue> to_arrayqueue_v{};

/**
 * @brief Pipe adaptor to convert range to Random_Set.
 */
inline constexpr to_aleph_adaptor<Random_Set> to_randomset_v{};

/**
 * @brief Tag type for to_aleph pipe adaptor using insert (for sets/trees).
 */
template <template <typename, typename...> class Container, typename... ExtraArgs>
struct to_aleph_insert_adaptor
{
  template <RangeLike R>
  [[nodiscard]] friend auto operator|(R&& r, to_aleph_insert_adaptor)
  {
    using T = std::ranges::range_value_t<R>;
    Container<T, ExtraArgs...> result;
    for (auto&& elem : r)
      result.insert(std::forward<decltype(elem)>(elem));
    return result;
  }
};

/**
 * @brief Generic function to collect a range into any Aleph container.
 *
 * This is the most flexible way to materialize a range into an Aleph container.
 * Works with any container that has either `append()` or `insert()`.
 *
 * @tparam Container The target container type (fully specified).
 * @param r The source range.
 * @return Container filled with elements from the range.
 *
 * @par Example:
 * @code
 * // For containers with append()
 * auto list = collect<DynList<int>>(std::views::iota(1, 10));
 *
 * // For containers with insert()
 * auto set = collect<DynSetRbTree<int>>(std::views::iota(1, 10));
 * @endcode
 */
template <typename Container, RangeLike R>
[[nodiscard]] Container collect(R&& r)
{
  Container result;
  for (auto&& elem : r)
    {
      if constexpr (requires { result.append(elem); })
        result.append(std::forward<decltype(elem)>(elem));
      else if constexpr (requires { result.insert(elem); })
        result.insert(std::forward<decltype(elem)>(elem));
      else if constexpr (requires { result.push(elem); })
        result.push(std::forward<decltype(elem)>(elem));
      else
        static_assert(sizeof(Container) == 0, 
          "Container must have append(), insert(), or push() method");
    }
  return result;
}

/**
 * @brief Pipe adaptor factory for any Aleph container.
 *
 * @tparam Container The target container type.
 *
 * @par Example:
 * @code
 * auto list = std::views::iota(1, 10) | to<DynList<int>>();
 * auto set = std::views::iota(1, 10) | to<DynSetRbTree<int>>();
 * @endcode
 */
template <typename Container>
struct to_adaptor
{
  template <RangeLike R>
  [[nodiscard]] friend Container operator|(R&& r, to_adaptor)
  {
    return collect<Container>(std::forward<R>(r));
  }
};

/**
 * @brief Create a pipe adaptor for any Aleph container.
 *
 * @tparam Container The target container type.
 * @return A pipe adaptor that converts ranges to the specified container.
 *
 * @par Example:
 * @code
 * auto evens = std::views::iota(1, 100)
 *            | std::views::filter([](int x) { return x % 2 == 0; })
 *            | to<DynList<int>>();
 * @endcode
 */
template <typename Container>
[[nodiscard]] constexpr to_adaptor<Container> to() { return {}; }

#else // !ALEPH_HAS_RANGES

// ============================================================================
// Fallback for C++17 and earlier
// ============================================================================

// Define ALEPH_HAS_RANGES as 0 for feature detection
// The public API in ahFunctional.H will be used instead

namespace detail
{

// Fallback implementations that work without ranges
template <typename Container, typename Pred>
[[nodiscard]] bool fallback_all_of(const Container& c, Pred&& pred)
{
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    if (!pred(it.get_curr()))
      return false;
  return true;
}

template <typename Container, typename Pred>
[[nodiscard]] bool fallback_any_of(const Container& c, Pred&& pred)
{
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    if (pred(it.get_curr()))
      return true;
  return false;
}

template <typename Container, typename Pred>
[[nodiscard]] bool fallback_none_of(const Container& c, Pred&& pred)
{
  return !fallback_any_of(c, std::forward<Pred>(pred));
}

} // namespace detail

#endif // ALEPH_HAS_RANGES

} // namespace Aleph

#endif // AH_RANGES_H
