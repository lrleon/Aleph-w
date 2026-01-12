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

# ifndef AH_STL_FUNCTIONAL_H
# define AH_STL_FUNCTIONAL_H

/**
 * @file ah-stl-functional.H
 * @brief Functional programming utilities for C++ Standard Library containers.
 *
 * This header provides functional programming operations (map, filter, fold,
 * etc.) that work with any STL-compatible container (std::vector, std::list,
 * std::deque, std::set, etc.).
 *
 * ## Key Features:
 * - Works with any container that has `begin()`/`end()` iterators
 * - Includes `foldr` (right fold) using reverse iterators
 * - ML-style operations: `mapi`, `filteri`, `scan_left`, `find_mapi`
 * - Zero dependencies on Aleph containers
 *
 * ## Usage Example:
 * @code
 * #include <ah-stl-functional.H>
 * #include <vector>
 *
 * std::vector<int> nums = {1, 2, 3, 4, 5};
 *
 * // Map: square each element
 * auto squares = stl_map([](int x) { return x * x; }, nums);
 *
 * // Filter: keep only even numbers
 * auto evens = stl_filter([](int x) { return x % 2 == 0; }, nums);
 *
 * // Fold left: sum
 * int sum = stl_foldl(0, [](int acc, int x) { return acc + x; }, nums);
 *
 * // Fold right (possible with STL!)
 * int result = stl_foldr(0, [](int x, int acc) { return x - acc; }, nums);
 * @endcode
 *
 * @ingroup Algorithms
 * @author Leandro Rabindranath Leon
 */

# include <type_traits>
# include <vector>
# include <optional>
# include <functional>
# include <algorithm>
# include <numeric>
# include <utility>
# include <tuple>
# include <iterator>
# include <stdexcept>
# include <ah-ranges.H>

namespace Aleph
{

// ============================================================================
// Range Generation
// ============================================================================

/**
 * @brief Generate a range of values [start, end] with given step.
 *
 * @tparam T Numeric type.
 * @param start Start value (inclusive).
 * @param end End value (inclusive).
 * @param step Increment between values (default 1).
 * @return std::vector<T> with the range.
 *
 * @par Example:
 * @code
 * auto r = stl_range(1, 5);      // {1, 2, 3, 4, 5}
 * auto r2 = stl_range(0, 10, 2); // {0, 2, 4, 6, 8, 10}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T = int>
[[nodiscard]] std::vector<T> stl_range(T start, T end, T step = 1)
{
  std::vector<T> result;
  if (step > 0 && start <= end)
    {
      result.reserve(static_cast<size_t>((end - start) / step + 1));
      for (T i = start; i <= end; i += step)
        result.push_back(i);
    }
  else if (step < 0 && start >= end)
    {
      result.reserve(static_cast<size_t>((start - end) / (-step) + 1));
      for (T i = start; i >= end; i += step)
        result.push_back(i);
    }
  return result;
}

/**
 * @brief Generate a range [0, n-1].
 *
 * @param n Number of elements.
 * @return std::vector<T> with values 0 to n-1.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T = int>
[[nodiscard]] std::vector<T> stl_range(T n)
{
  std::vector<T> result;
  result.reserve(static_cast<size_t>(n));
  for (T i = 0; i < n; ++i)
    result.push_back(i);
  return result;
}

/**
 * @brief Generate n evenly spaced values between start and end.
 *
 * @param start Start value.
 * @param end End value.
 * @param n Number of values to generate.
 * @return std::vector with n values.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T = double>
[[nodiscard]] std::vector<T> stl_linspace(T start, T end, size_t n)
{
  std::vector<T> result;
  if (n == 0) return result;
  result.reserve(n);
  if (n == 1)
    {
      result.push_back(start);
      return result;
    }

  result.reserve(n);
  const auto step = (end - start) / static_cast<T>(n - 1);
  for (size_t i = 0; i < n; ++i)
    result.push_back(start + static_cast<T>(i) * step);
  return result;
}

/**
 * @brief Generate a vector of n repeated values.
 *
 * @param n Number of repetitions.
 * @param value Value to repeat.
 * @return std::vector with n copies of value.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T>
[[nodiscard]] std::vector<T> stl_rep(size_t n, const T& value)
{
  return std::vector<T>(n, value);
}

/**
 * @brief Generate a vector using a generator function.
 *
 * @param n Number of elements.
 * @param gen Generator function `(size_t index) -> T`.
 * @return std::vector with generated values.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Gen>
[[nodiscard]] auto stl_generate(size_t n, Gen&& gen)
{
  using T = std::decay_t<decltype(gen(size_t{}))>;
  std::vector<T> result;
  result.reserve(n);
  for (size_t i = 0; i < n; ++i)
    result.push_back(std::forward<Gen>(gen)(i));
  return result;
}

// ============================================================================
// Core Functional Operations
// ============================================================================

/**
 * @brief Apply operation to each element (for_each).
 *
 * @param op Callable `(element) -> void`.
 * @param c Container.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
void stl_for_each(Op&& op, const Container& c)
{
  for (const auto& item : c)
    std::forward<Op>(op)(item);
}

/**
 * @brief Apply operation to each element with index.
 *
 * @param op Callable `(size_t idx, element) -> void`.
 * @param c Container.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
void stl_for_each_indexed(Op&& op, const Container& c)
{
  size_t i = 0;
  for (const auto& item : c)
    std::forward<Op>(op)(i++, item);
}

/**
 * @brief Map operation - transform each element.
 *
 * @param op Callable `(element) -> R`.
 * @param c Container.
 * @return std::vector<R> with transformed values.
 *
 * @par Example:
 * @code
 * auto squares = stl_map([](int x) { return x * x; }, nums);
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
[[nodiscard]] auto stl_map(Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  using R = std::decay_t<decltype(std::forward<Op>(op)(std::declval<T>()))>;

  std::vector<R> result;
  result.reserve(c.size());
  for (const auto& item : c)
    result.push_back(std::forward<Op>(op)(item));
  return result;
}

/**
 * @brief Map with index (mapi in ML).
 *
 * @param op Callable `(size_t idx, element) -> R`.
 * @param c Container.
 * @return std::vector<R> with transformed values.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
[[nodiscard]] auto stl_mapi(Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  using R = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<T>()))>;

  std::vector<R> result;
  result.reserve(c.size());
  size_t i = 0;
  for (const auto& item : c)
    result.push_back(std::forward<Op>(op)(i++, item));
  return result;
}

/**
 * @brief Filter elements satisfying predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return std::vector with filtered elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_filter(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  for (const auto& item : c)
    if (std::forward<Pred>(pred)(item))
      result.push_back(item);
  return result;
}

/**
 * @brief Filter with index (filteri in ML).
 *
 * @param pred Predicate `(size_t idx, element) -> bool`.
 * @param c Container.
 * @return std::vector with filtered elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_filteri(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  size_t i = 0;
  for (const auto& item : c)
    {
      if (std::forward<Pred>(pred)(i, item))
        result.push_back(item);
      ++i;
    }
  return result;
}

/**
 * @brief Left fold (foldl) - reduce from left to right.
 *
 * Computes: op(...op(op(init, x1), x2)..., xn)
 *
 * @param init Initial accumulator value.
 * @param op Binary operation `(acc, element) -> acc`.
 * @param c Container.
 * @return Final accumulated value.
 *
 * @par Example:
 * @code
 * int sum = stl_foldl(0, [](int acc, int x) { return acc + x; }, nums);
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Op, typename Container>
[[nodiscard]] T stl_foldl(T init, Op&& op, const Container& c)
{
#if ALEPH_HAS_RANGES
  if constexpr (std::ranges::range<Container>)
    return detail::ranges_fold_left(c, std::move(init), std::forward<Op>(op));
  else
#endif
  {
    T acc = std::move(init);
    for (const auto& item : c)
      acc = std::forward<Op>(op)(std::move(acc), item);
    return acc;
  }
}

/**
 * @brief Right fold (foldr) - reduce from right to left.
 *
 * Computes: op(x1, op(x2, ...op(xn, init)...))
 *
 * @note This is possible with STL containers that have reverse iterators!
 *
 * @param init Initial accumulator value.
 * @param op Binary operation `(element, acc) -> acc`.
 * @param c Container (must support rbegin()/rend()).
 * @return Final accumulated value.
 *
 * @par Example:
 * @code
 * // Right fold with subtraction: 1 - (2 - (3 - 0)) = 1 - (2 - 3) = 1 - (-1) = 2
 * int result = stl_foldr(0, [](int x, int acc) { return x - acc; }, {1, 2, 3});
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Op, typename Container>
[[nodiscard]] T stl_foldr(T init, Op&& op, const Container& c)
{
  T acc = std::move(init);
  for (auto it = c.rbegin(); it != c.rend(); ++it)
    acc = std::forward<Op>(op)(*it, std::move(acc));
  return acc;
}

/**
 * @brief Scan left - fold with all intermediate results.
 *
 * @param init Initial value.
 * @param op Binary operation `(acc, element) -> acc`.
 * @param c Container.
 * @return std::vector containing init and all intermediate results.
 *
 * @par Example:
 * @code
 * // Running sum: [0, 1, 3, 6, 10]
 * auto sums = stl_scan_left(0, [](int acc, int x) { return acc + x; }, {1,2,3,4});
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Op, typename Container>
[[nodiscard]] std::vector<T> stl_scan_left(T init, Op&& op, const Container& c)
{
  std::vector<T> result;
  result.reserve(c.size() + 1);
  result.push_back(init);

  T acc = std::move(init);
  for (const auto& item : c)
    {
      acc = std::forward<Op>(op)(std::move(acc), item);
      result.push_back(acc);
    }
  return result;
}

/**
 * @brief Scan right - right fold with all intermediate results.
 *
 * @param init Initial value.
 * @param op Binary operation `(element, acc) -> acc`.
 * @param c Container.
 * @return std::vector containing all intermediate results (in reverse order) and init.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Op, typename Container>
[[nodiscard]] std::vector<T> stl_scan_right(T init, Op&& op, const Container& c)
{
  std::vector<T> result;
  result.reserve(c.size() + 1);

  T acc = std::move(init);
  result.push_back(acc);

  for (auto it = c.rbegin(); it != c.rend(); ++it)
    {
      acc = std::forward<Op>(op)(*it, std::move(acc));
      result.push_back(acc);
    }

  std::reverse(result.begin(), result.end());
  return result;
}

/// Alias for stl_foldl
template <typename T, typename Op, typename Container>
[[nodiscard]] T stl_reduce(T init, Op&& op, const Container& c)
{
  return stl_foldl(std::move(init), std::forward<Op>(op), c);
}

// ============================================================================
// Predicates
// ============================================================================

/**
 * @brief Check if all elements satisfy predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return `true` if all elements satisfy predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] bool stl_all(Pred&& pred, const Container& c)
{
#if ALEPH_HAS_RANGES
  if constexpr (std::ranges::range<Container>)
    return detail::ranges_all_of(c, std::forward<Pred>(pred));
  else
#endif
  {
    for (const auto& item : c)
      if (!std::forward<Pred>(pred)(item))
        return false;
    return true;
  }
}

/**
 * @brief Check if any element satisfies predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return `true` if any element satisfies predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] bool stl_exists(Pred&& pred, const Container& c)
{
#if ALEPH_HAS_RANGES
  if constexpr (std::ranges::range<Container>)
    return detail::ranges_any_of(c, std::forward<Pred>(pred));
  else
#endif
  {
    for (const auto& item : c)
      if (std::forward<Pred>(pred)(item))
        return true;
    return false;
  }
}

/// Alias for stl_exists
template <typename Pred, typename Container>
[[nodiscard]] bool stl_any(Pred&& pred, const Container& c)
{
  return stl_exists(std::forward<Pred>(pred), c);
}

/**
 * @brief Check if no element satisfies predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] bool stl_none(Pred&& pred, const Container& c)
{
  return !stl_exists(std::forward<Pred>(pred), c);
}

// ============================================================================
// Finding Elements
// ============================================================================

/**
 * @brief Find first element satisfying predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return std::optional with element if found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_find(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  for (const auto& item : c)
    if (std::forward<Pred>(pred)(item))
      return std::optional<T>(item);
  return std::optional<T>{};
}

/**
 * @brief Find last element satisfying predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return std::optional with element if found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_find_last(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::optional<T> result;
  for (const auto& item : c)
    if (std::forward<Pred>(pred)(item))
      result = item;
  return result;
}

/**
 * @brief Find index of first element satisfying predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return std::optional<size_t> with index if found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] std::optional<size_t> stl_find_index(Pred&& pred, const Container& c)
{
  size_t i = 0;
  for (const auto& item : c)
    {
      if (std::forward<Pred>(pred)(item))
        return i;
      ++i;
    }
  return std::nullopt;
}

/**
 * @brief Find and map with index (find_mapi in ML).
 *
 * @param op Callable `(size_t idx, element) -> std::optional<R>`.
 * @param c Container.
 * @return First non-empty result from op.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
[[nodiscard]] auto stl_find_mapi(Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  using OptType = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<T>()))>;

  size_t i = 0;
  for (const auto& item : c)
    {
      auto result = std::forward<Op>(op)(i++, item);
      if (result)
        return result;
    }
  return OptType{};
}

/**
 * @brief Check if element exists in container (mem in ML).
 *
 * @param target Element to search for.
 * @param c Container.
 * @return `true` if target is found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Container>
[[nodiscard]] bool stl_mem(const T& target, const Container& c)
{
  for (const auto& item : c)
    if (item == target)
      return true;
  return false;
}

// ============================================================================
// Counting
// ============================================================================

/**
 * @brief Count elements satisfying predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return Number of elements satisfying predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] size_t stl_count(Pred&& pred, const Container& c)
{
  size_t count = 0;
  for (const auto& item : c)
    if (std::forward<Pred>(pred)(item))
      ++count;
  return count;
}

/**
 * @brief Count occurrences of a value.
 *
 * @param target Value to count.
 * @param c Container.
 * @return Number of occurrences.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Container>
[[nodiscard]] size_t stl_count_value(const T& target, const Container& c)
{
  size_t count = 0;
  for (const auto& item : c)
    if (item == target)
      ++count;
  return count;
}

// ============================================================================
// Taking and Dropping
// ============================================================================

/**
 * @brief Take first n elements.
 *
 * @param n Number of elements to take.
 * @param c Container.
 * @return std::vector with first n elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_take(size_t n, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  result.reserve(n);
  size_t count = 0;
  for (const auto& item : c)
    {
      if (count >= n) break;
      result.push_back(item);
      ++count;
    }
  return result;
}

/**
 * @brief Drop first n elements, return the rest.
 *
 * @param n Number of elements to skip.
 * @param c Container.
 * @return std::vector with remaining elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_drop(size_t n, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  size_t size = c.size();
  if (size > n)
    result.reserve(size - n);
  size_t count = 0;
  for (const auto& item : c)
    {
      if (count >= n)
        result.push_back(item);
      ++count;
    }
  return result;
}

/**
 * @brief Take last n elements.
 *
 * @param n Number of elements to take.
 * @param c Container.
 * @return std::vector with last n elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_take_last(size_t n, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  size_t size = c.size();
  size_t actual_n = (size > n) ? n : size;
  result.reserve(actual_n);
  size_t skip = (size > n) ? (size - n) : 0;

  size_t count = 0;
  for (const auto& item : c)
    {
      if (count >= skip)
        result.push_back(item);
      ++count;
    }
  return result;
}

/**
 * @brief Take elements while predicate is true.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return std::vector with elements taken while predicate is true.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_take_while(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  for (const auto& item : c)
    {
      if (!std::forward<Pred>(pred)(item))
        break;
      result.push_back(item);
    }
  return result;
}

/**
 * @brief Drop elements while predicate is true, return the rest.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return std::vector with remaining elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_drop_while(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  bool dropping = true;
  for (const auto& item : c)
    {
      if (dropping && std::forward<Pred>(pred)(item))
        continue;
      dropping = false;
      result.push_back(item);
    }
  return result;
}

// ============================================================================
// Accessing Elements
// ============================================================================

/**
 * @brief Get first element.
 *
 * @param c Container.
 * @return std::optional with first element if container is not empty.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_first(const Container& c)
{
  using T = typename Container::value_type;

  auto it = c.begin();
  if (it == c.end())
    return std::optional<T>{};
  return std::optional<T>(*it);
}

/**
 * @brief Get last element.
 *
 * @param c Container.
 * @return std::optional with last element if container is not empty.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_last(const Container& c)
{
  using T = typename Container::value_type;

  if (c.empty())
    return std::optional<T>{};

  auto it = c.end();
  --it;
  return std::optional<T>(*it);
}

/**
 * @brief Get n-th element.
 *
 * @param n Index (0-based).
 * @param c Container.
 * @return std::optional with n-th element if exists.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_nth(size_t n, const Container& c)
{
  using T = typename Container::value_type;

  size_t i = 0;
  for (const auto& item : c)
    {
      if (i == n)
        return std::optional<T>(item);
      ++i;
    }
  return std::optional<T>{};
}

// ============================================================================
// Min/Max
// ============================================================================

/**
 * @brief Get minimum element.
 *
 * @param c Container.
 * @return std::optional with minimum element.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_min(const Container& c)
{
  using T = typename Container::value_type;

  auto it = c.begin();
  if (it == c.end())
    return std::optional<T>{};

  T min_val = *it;
  ++it;
  for (; it != c.end(); ++it)
    if (*it < min_val)
      min_val = *it;
  return std::optional<T>(min_val);
}

/**
 * @brief Get maximum element.
 *
 * @param c Container.
 * @return std::optional with maximum element.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_max(const Container& c)
{
  using T = typename Container::value_type;

  auto it = c.begin();
  if (it == c.end())
    return std::optional<T>{};

  T max_val = *it;
  ++it;
  for (; it != c.end(); ++it)
    if (*it > max_val)
      max_val = *it;
  return std::optional<T>(max_val);
}

/**
 * @brief Get both min and max in a single pass.
 *
 * @param c Container.
 * @return std::optional<std::pair<T, T>> with (min, max).
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_min_max(const Container& c)
{
  using T = typename Container::value_type;
  using ResultType = std::optional<std::pair<T, T>>;

  auto it = c.begin();
  if (it == c.end())
    return ResultType{};

  T min_val = *it;
  T max_val = *it;
  ++it;

  for (; it != c.end(); ++it)
    {
      if (*it < min_val) min_val = *it;
      if (*it > max_val) max_val = *it;
    }
  return ResultType(std::make_pair(min_val, max_val));
}

/**
 * @brief Get minimum element by key function.
 *
 * @param key Key extractor `(element) -> comparable`.
 * @param c Container.
 * @return std::optional with element having minimum key.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Key, typename Container>
[[nodiscard]] auto stl_min_by(Key&& key, const Container& c)
{
  using T = typename Container::value_type;

  auto it = c.begin();
  if (it == c.end())
    return std::optional<T>{};

  T min_elem = *it;
  auto min_key = std::forward<Key>(key)(min_elem);
  ++it;

  for (; it != c.end(); ++it)
    {
      auto k = std::forward<Key>(key)(*it);
      if (k < min_key)
        {
          min_key = k;
          min_elem = *it;
        }
    }
  return std::optional<T>(min_elem);
}

/**
 * @brief Get maximum element by key function.
 *
 * @param key Key extractor `(element) -> comparable`.
 * @param c Container.
 * @return std::optional with element having maximum key.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Key, typename Container>
[[nodiscard]] auto stl_max_by(Key&& key, const Container& c)
{
  using T = typename Container::value_type;

  auto it = c.begin();
  if (it == c.end())
    return std::optional<T>{};

  T max_elem = *it;
  auto max_key = std::forward<Key>(key)(max_elem);
  ++it;

  for (; it != c.end(); ++it)
    {
      auto k = std::forward<Key>(key)(*it);
      if (k > max_key)
        {
          max_key = k;
          max_elem = *it;
        }
    }
  return std::optional<T>(max_elem);
}

// ============================================================================
// Sum and Product
// ============================================================================

/**
 * @brief Sum all elements.
 *
 * @param c Container of numeric values.
 * @return Sum of all elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_sum(const Container& c)
{
  using T = typename Container::value_type;
  T sum{};
  for (const auto& item : c)
    sum = sum + item;
  return sum;
}

/**
 * @brief Product of all elements.
 *
 * @param c Container of numeric values.
 * @return Product of all elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_product(const Container& c)
{
  using T = typename Container::value_type;
  if (c.empty()) return T{};

  auto it = c.begin();
  T prod = *it;
  ++it;
  for (; it != c.end(); ++it)
    prod = prod * (*it);
  return prod;
}

// ============================================================================
// Partitioning
// ============================================================================

/**
 * @brief Partition elements by predicate.
 *
 * @param pred Predicate `(element) -> bool`.
 * @param c Container.
 * @return Pair of vectors: (elements satisfying pred, elements not satisfying pred).
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_partition(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> matching, non_matching;
  for (const auto& item : c)
    {
      if (std::forward<Pred>(pred)(item))
        matching.push_back(item);
      else
        non_matching.push_back(item);
    }
  return std::make_pair(std::move(matching), std::move(non_matching));
}

// ============================================================================
// Zipping and Pairing
// ============================================================================

/**
 * @brief Zip two containers into pairs.
 *
 * @param c1 First container.
 * @param c2 Second container.
 * @return std::vector of pairs.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container1, typename Container2>
[[nodiscard]] auto stl_zip_to_pairs(const Container1& c1, const Container2& c2)
{
  using T1 = typename Container1::value_type;
  using T2 = typename Container2::value_type;

  std::vector<std::pair<T1, T2>> result;
  result.reserve(std::min(c1.size(), c2.size()));
  auto it1 = c1.begin();
  auto it2 = c2.begin();
  for (; it1 != c1.end() && it2 != c2.end(); ++it1, ++it2)
    result.emplace_back(*it1, *it2);
  return result;
}

/**
 * @brief Unzip pairs into two vectors.
 *
 * @param c Container of pairs.
 * @return Pair of vectors.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_unzip_pairs(const Container& c)
{
  using PairType = typename Container::value_type;
  using T1 = std::decay_t<decltype(std::declval<PairType>().first)>;
  using T2 = std::decay_t<decltype(std::declval<PairType>().second)>;

  std::vector<T1> v1;
  std::vector<T2> v2;
  v1.reserve(c.size());
  v2.reserve(c.size());

  for (const auto& p : c)
    {
      v1.push_back(p.first);
      v2.push_back(p.second);
    }
  return std::make_pair(std::move(v1), std::move(v2));
}

/**
 * @brief Enumerate container (return pairs of index and element).
 *
 * @param c Container.
 * @return std::vector of pairs (index, element).
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_enumerate_to_pairs(const Container& c)
{
  using T = typename Container::value_type;

  std::vector<std::pair<size_t, T>> result;
  result.reserve(c.size());
  size_t i = 0;
  for (const auto& item : c)
    result.emplace_back(i++, item);
  return result;
}

// ============================================================================
// Comparison
// ============================================================================

/**
 * @brief Check equality of two containers.
 *
 * @param c1 First container.
 * @param c2 Second container.
 * @return `true` if containers have same elements in same order.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container1, typename Container2>
[[nodiscard]] bool stl_equal(const Container1& c1, const Container2& c2)
{
  auto it1 = c1.begin();
  auto it2 = c2.begin();
  for (; it1 != c1.end() && it2 != c2.end(); ++it1, ++it2)
    if (!(*it1 == *it2))
      return false;
  return it1 == c1.end() && it2 == c2.end();
}

/**
 * @brief Compare two containers lexicographically.
 *
 * @param c1 First container.
 * @param c2 Second container.
 * @return -1 if c1 < c2, 0 if equal, 1 if c1 > c2.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container1, typename Container2>
[[nodiscard]] int stl_compare(const Container1& c1, const Container2& c2)
{
  auto it1 = c1.begin();
  auto it2 = c2.begin();

  for (; it1 != c1.end() && it2 != c2.end(); ++it1, ++it2)
    {
      if (*it1 < *it2) return -1;
      if (*it2 < *it1) return 1;
    }

  if (it1 == c1.end() && it2 == c2.end()) return 0;
  return (it1 == c1.end()) ? -1 : 1;
}

// ============================================================================
// Reversing and Sorting
// ============================================================================

/**
 * @brief Return reversed copy of container.
 *
 * @param c Container.
 * @return std::vector with elements in reverse order.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_reverse(const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result(c.begin(), c.end());
  std::reverse(result.begin(), result.end());
  return result;
}

/**
 * @brief Return sorted copy of container.
 *
 * @param c Container.
 * @return std::vector with sorted elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_sort(const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result(c.begin(), c.end());
  std::sort(result.begin(), result.end());
  return result;
}

/**
 * @brief Return sorted copy using custom comparator.
 *
 * @param cmp Comparator `(a, b) -> bool` (true if a < b).
 * @param c Container.
 * @return std::vector with sorted elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Cmp, typename Container>
[[nodiscard]] auto stl_sort_by(Cmp&& cmp, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result(c.begin(), c.end());
  std::sort(result.begin(), result.end(), std::forward<Cmp>(cmp));
  return result;
}

// ============================================================================
// Uniqueness
// ============================================================================

/**
 * @brief Remove consecutive duplicates.
 *
 * @param c Container.
 * @return std::vector with consecutive duplicates removed.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_unique(const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  for (const auto& item : c)
    {
      if (result.empty() || !(result.back() == item))
        result.push_back(item);
    }
  return result;
}

/**
 * @brief Remove all duplicates (keeps first occurrence).
 *
 * @param c Container.
 * @return std::vector with all duplicates removed.
 *
 * @note Uses O(n²) algorithm. For better performance with large containers,
 *       use a set-based approach.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_distinct(const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  for (const auto& item : c)
    {
      bool found = false;
      for (const auto& r : result)
        if (r == item)
          {
            found = true;
            break;
          }
      if (!found)
        result.push_back(item);
    }
  return result;
}

// ============================================================================
// Concatenation and Flattening
// ============================================================================

/**
 * @brief Concatenate two containers.
 *
 * @param c1 First container.
 * @param c2 Second container.
 * @return std::vector with all elements from c1 followed by c2.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container1, typename Container2>
[[nodiscard]] auto stl_concat(const Container1& c1, const Container2& c2)
{
  using T = typename Container1::value_type;

  std::vector<T> result;
  result.reserve(c1.size() + c2.size());
  for (const auto& item : c1)
    result.push_back(item);
  for (const auto& item : c2)
    result.push_back(item);
  return result;
}

/**
 * @brief Flatten a container of containers.
 *
 * @param c Container of containers.
 * @return std::vector with all nested elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_flatten(const Container& c)
{
  using InnerContainer = typename Container::value_type;
  using T = typename InnerContainer::value_type;

  std::vector<T> result;
  for (const auto& inner : c)
    for (const auto& item : inner)
      result.push_back(item);
  return result;
}

/**
 * @brief Flat map - map then flatten.
 *
 * @param op Operation `(element) -> Container<R>`.
 * @param c Container.
 * @return std::vector with flattened mapped results.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
[[nodiscard]] auto stl_flat_map(Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  using InnerContainer = std::decay_t<decltype(std::forward<Op>(op)(std::declval<T>()))>;
  using R = typename InnerContainer::value_type;

  std::vector<R> result;
  for (const auto& item : c)
    for (const auto& inner_item : std::forward<Op>(op)(item))
      result.push_back(inner_item);
  return result;
}

// ============================================================================
// Grouping
// ============================================================================

/**
 * @brief Group consecutive equal elements.
 *
 * @param c Container.
 * @return std::vector of vectors, each containing consecutive equal elements.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_group(const Container& c)
{
  using T = typename Container::value_type;

  std::vector<std::vector<T>> result;
  if (c.empty()) return result;

  auto it = c.begin();
  result.emplace_back();
  result.back().push_back(*it);
  T current = *it;
  ++it;

  for (; it != c.end(); ++it)
    {
      if (!(*it == current))
        {
          result.emplace_back();
          current = *it;
        }
      result.back().push_back(*it);
    }
  return result;
}

/**
 * @brief Group elements by key function.
 *
 * @param key Key extractor `(element) -> K`.
 * @param c Container.
 * @return std::vector of pairs (key, vector of elements with that key).
 *
 * @note This is an O(n²) implementation. For better performance, use a map.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Key, typename Container>
[[nodiscard]] auto stl_group_by(Key&& key, const Container& c)
{
  using T = typename Container::value_type;
  using K = std::decay_t<decltype(std::forward<Key>(key)(std::declval<T>()))>;

  std::vector<std::pair<K, std::vector<T>>> result;

  for (const auto& item : c)
    {
      auto k = std::forward<Key>(key)(item);
      bool found = false;
      for (auto& group : result)
        {
          if (group.first == k)
            {
              group.second.push_back(item);
              found = true;
              break;
            }
        }
      if (!found)
        result.emplace_back(k, std::vector<T>{item});
    }
  return result;
}

// ============================================================================
// Combinatorics: Permutations, Combinations, Arrangements
// ============================================================================

namespace stl_comb_detail
{

template <typename T, typename Op>
bool permutations_impl(std::vector<T>& arr, size_t start, Op&& op)
{
  if (start >= arr.size())
    return std::forward<Op>(op)(arr);

  for (size_t i = start; i < arr.size(); ++i)
    {
      std::swap(arr[start], arr[i]);
      if (!permutations_impl(arr, start + 1, std::forward<Op>(op)))
        {
          std::swap(arr[start], arr[i]);
          return false;
        }
      std::swap(arr[start], arr[i]);
    }
  return true;
}

template <typename T, typename Op>
bool combinations_impl(const std::vector<T>& arr, size_t k, size_t start,
                       std::vector<T>& current, Op&& op)
{
  if (current.size() == k)
    return std::forward<Op>(op)(current);

  for (size_t i = start; i <= arr.size() - (k - current.size()); ++i)
    {
      current.push_back(arr[i]);
      if (!combinations_impl(arr, k, i + 1, current, std::forward<Op>(op)))
        {
          current.pop_back();
          return false;
        }
      current.pop_back();
    }
  return true;
}

template <typename T, typename Op>
bool arrangements_impl(const std::vector<T>& arr, size_t k,
                       std::vector<T>& current, std::vector<bool>& used, Op&& op)
{
  if (current.size() == k)
    return std::forward<Op>(op)(current);

  for (size_t i = 0; i < arr.size(); ++i)
    {
      if (used[i]) continue;
      used[i] = true;
      current.push_back(arr[i]);
      if (!arrangements_impl(arr, k, current, used, std::forward<Op>(op)))
        {
          current.pop_back();
          used[i] = false;
          return false;
        }
      current.pop_back();
      used[i] = false;
    }
  return true;
}

} // namespace stl_comb_detail

/**
 * @brief Traverse all permutations of a container.
 *
 * Generates all n! permutations (orderings) of the elements.
 * The operation can return false to stop early.
 *
 * @param op Callable `(const std::vector<T>&) -> bool`. Return false to stop.
 * @param c Container.
 * @return `true` if all permutations were visited, `false` if stopped early.
 *
 * @par Example:
 * @code
 * stl_traverse_permutations([](const auto& p) {
 *   for (int x : p) std::cout << x << " ";
 *   std::cout << "\n";
 *   return true;
 * }, std::vector<int>{1, 2, 3});
 * // Output: all 6 permutations
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
bool stl_traverse_permutations(Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<T> arr(c.begin(), c.end());
  return stl_comb_detail::permutations_impl(arr, 0, std::forward<Op>(op));
}

/**
 * @brief Generate all permutations of a container.
 *
 * @param c Container.
 * @return Vector of vectors, each containing one permutation.
 *
 * @note For n elements, generates n! permutations. Use with care for large n.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_permutations(const Container& c)
{
  using T = typename Container::value_type;
  std::vector<std::vector<T>> result;
  
  stl_traverse_permutations([&result](const std::vector<T>& p) {
    result.push_back(p);
    return true;
  }, c);
  
  return result;
}

/**
 * @brief Traverse all k-combinations of a container.
 *
 * Generates all C(n,k) ways to choose k elements from n, without regard to order.
 * The operation can return false to stop early.
 *
 * @param k Number of elements to choose.
 * @param op Callable `(const std::vector<T>&) -> bool`. Return false to stop.
 * @param c Container.
 * @return `true` if all combinations were visited, `false` if stopped early.
 *
 * @par Example:
 * @code
 * stl_traverse_combinations(2, [](const auto& combo) {
 *   for (int x : combo) std::cout << x << " ";
 *   std::cout << "\n";
 *   return true;
 * }, std::vector<int>{1, 2, 3, 4});
 * // Output: {1,2}, {1,3}, {1,4}, {2,3}, {2,4}, {3,4}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
bool stl_traverse_combinations(size_t k, Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<T> arr(c.begin(), c.end());
  
  if (k > arr.size())
    return true;
  
  std::vector<T> current;
  current.reserve(k);
  return stl_comb_detail::combinations_impl(arr, k, 0, current, std::forward<Op>(op));
}

/**
 * @brief Generate all k-combinations of a container.
 *
 * @param k Number of elements to choose.
 * @param c Container.
 * @return Vector of vectors, each containing one combination.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_combinations(size_t k, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<std::vector<T>> result;
  
  stl_traverse_combinations(k, [&result](const std::vector<T>& combo) {
    result.push_back(combo);
    return true;
  }, c);
  
  return result;
}

/**
 * @brief Traverse all k-arrangements (k-permutations) of a container.
 *
 * Generates all P(n,k) = n!/(n-k)! ways to choose k elements from n WITH order.
 * The operation can return false to stop early.
 *
 * @param k Number of elements to choose.
 * @param op Callable `(const std::vector<T>&) -> bool`. Return false to stop.
 * @param c Container.
 * @return `true` if all arrangements were visited, `false` if stopped early.
 *
 * @par Example:
 * @code
 * stl_traverse_arrangements(2, [](const auto& arr) {
 *   for (int x : arr) std::cout << x << " ";
 *   std::cout << "\n";
 *   return true;
 * }, std::vector<int>{1, 2, 3});
 * // Output: {1,2}, {1,3}, {2,1}, {2,3}, {3,1}, {3,2}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename Container>
bool stl_traverse_arrangements(size_t k, Op&& op, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<T> arr(c.begin(), c.end());
  
  if (k > arr.size())
    return true;
  
  std::vector<T> current;
  current.reserve(k);
  std::vector<bool> used(arr.size(), false);
  return stl_comb_detail::arrangements_impl(arr, k, current, used, std::forward<Op>(op));
}

/**
 * @brief Generate all k-arrangements (k-permutations) of a container.
 *
 * @param k Number of elements to choose.
 * @param c Container.
 * @return Vector of vectors, each containing one arrangement.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_arrangements(size_t k, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<std::vector<T>> result;
  
  stl_traverse_arrangements(k, [&result](const std::vector<T>& arr) {
    result.push_back(arr);
    return true;
  }, c);
  
  return result;
}

/**
 * @brief Generate cartesian product of multiple containers.
 *
 * @param containers Vector of containers.
 * @return Vector of vectors, each containing one tuple from the product.
 *
 * @par Example:
 * @code
 * std::vector<std::vector<int>> sets = {{1, 2}, {3, 4}};
 * auto product = stl_cartesian_product(sets);
 * // Result: {{1,3}, {1,4}, {2,3}, {2,4}}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T>
[[nodiscard]] auto stl_cartesian_product(const std::vector<std::vector<T>>& containers)
{
  std::vector<std::vector<T>> result;
  
  if (containers.empty())
    return result;
  
  // Start with first container
  for (const auto& elem : containers[0])
    result.push_back({elem});
  
  // Extend with each subsequent container
  for (size_t i = 1; i < containers.size(); ++i)
    {
      std::vector<std::vector<T>> new_result;
      for (const auto& partial : result)
        for (const auto& elem : containers[i])
          {
            auto extended = partial;
            extended.push_back(elem);
            new_result.push_back(std::move(extended));
          }
      result = std::move(new_result);
    }
  
  return result;
}

/**
 * @brief Generate power set (all subsets) of a container.
 *
 * @param c Container.
 * @return Vector of vectors containing all 2^n subsets.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_power_set(const Container& c)
{
  using T = typename Container::value_type;
  std::vector<T> arr(c.begin(), c.end());
  std::vector<std::vector<T>> result;
  
  size_t n = arr.size();
  size_t total = 1ULL << n;  // 2^n
  
  for (size_t mask = 0; mask < total; ++mask)
    {
      std::vector<T> subset;
      for (size_t i = 0; i < n; ++i)
        if (mask & (1ULL << i))
          subset.push_back(arr[i]);
      result.push_back(std::move(subset));
    }
  
  return result;
}

// ============================================================================
// Additional Ruby/ML-style Operations
// ============================================================================

/**
 * @brief Sliding window of size n over container (each_cons in Ruby).
 *
 * @param n Window size.
 * @param c Container.
 * @return Vector of vectors, each containing n consecutive elements.
 *
 * @par Example:
 * @code
 * auto windows = stl_sliding_window(3, {1, 2, 3, 4, 5});
 * // Result: {{1,2,3}, {2,3,4}, {3,4,5}}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_sliding_window(size_t n, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<std::vector<T>> result;
  
  if (n == 0) return result;
  
  std::vector<T> arr(c.begin(), c.end());
  if (arr.size() < n) return result;
  
  for (size_t i = 0; i <= arr.size() - n; ++i)
    result.emplace_back(arr.begin() + i, arr.begin() + i + n);
  
  return result;
}

/**
 * @brief Split container into chunks of size n (each_slice in Ruby).
 *
 * @param n Chunk size.
 * @param c Container.
 * @return Vector of vectors, each containing up to n elements.
 *
 * @par Example:
 * @code
 * auto chunks = stl_chunks(2, {1, 2, 3, 4, 5});
 * // Result: {{1,2}, {3,4}, {5}}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_chunks(size_t n, const Container& c)
{
  using T = typename Container::value_type;
  std::vector<std::vector<T>> result;
  
  if (n == 0) return result;
  
  std::vector<T> arr(c.begin(), c.end());
  
  for (size_t i = 0; i < arr.size(); i += n)
    {
      size_t end = std::min(i + n, arr.size());
      result.emplace_back(arr.begin() + i, arr.begin() + end);
    }
  
  return result;
}

/**
 * @brief Insert element between each pair (intersperse in Haskell).
 *
 * @param sep Separator element.
 * @param c Container.
 * @return Vector with separator inserted between elements.
 *
 * @par Example:
 * @code
 * auto result = stl_intersperse(0, {1, 2, 3});
 * // Result: {1, 0, 2, 0, 3}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Container>
[[nodiscard]] auto stl_intersperse(const T& sep, const Container& c)
{
  std::vector<T> result;
  
  bool first = true;
  for (const auto& item : c)
    {
      if (!first)
        result.push_back(sep);
      result.push_back(item);
      first = false;
    }
  
  return result;
}

/**
 * @brief Split at position n, returning (take n, drop n) in one pass.
 *
 * @param n Position to split at.
 * @param c Container.
 * @return Pair of vectors.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_split_at(size_t n, const Container& c)
{
  using T = typename Container::value_type;
  
  std::vector<T> first, second;
  size_t i = 0;
  for (const auto& item : c)
    {
      if (i < n)
        first.push_back(item);
      else
        second.push_back(item);
      ++i;
    }
  
  return std::make_pair(std::move(first), std::move(second));
}

/**
 * @brief Split at predicate boundary (span in Haskell).
 *
 * Returns (takeWhile pred, dropWhile pred) in one pass.
 *
 * @param pred Predicate.
 * @param c Container.
 * @return Pair of vectors.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_span(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;
  
  std::vector<T> first, second;
  bool taking = true;
  
  for (const auto& item : c)
    {
      if (taking && std::forward<Pred>(pred)(item))
        first.push_back(item);
      else
        {
          taking = false;
          second.push_back(item);
        }
    }
  
  return std::make_pair(std::move(first), std::move(second));
}

/**
 * @brief Get all elements except the last (init in Haskell).
 *
 * @param c Container.
 * @return Vector without last element.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_init(const Container& c)
{
  using T = typename Container::value_type;
  std::vector<T> arr(c.begin(), c.end());
  
  if (!arr.empty())
    arr.pop_back();
  
  return arr;
}

/**
 * @brief Get all elements except the first (tail in Haskell).
 *
 * @param c Container.
 * @return Vector without first element.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_tail(const Container& c)
{
  using T = typename Container::value_type;
  std::vector<T> result;
  
  bool first = true;
  for (const auto& item : c)
    {
      if (first)
        first = false;
      else
        result.push_back(item);
    }
  
  return result;
}

/**
 * @brief Count occurrences of each element (tally in Ruby).
 *
 * @param c Container.
 * @return Vector of pairs (element, count).
 *
 * @note Uses O(n²) algorithm. For large containers, use a map.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto stl_tally(const Container& c)
{
  using T = typename Container::value_type;
  std::vector<std::pair<T, size_t>> result;
  
  for (const auto& item : c)
    {
      bool found = false;
      for (auto& p : result)
        if (p.first == item)
          {
            ++p.second;
            found = true;
            break;
          }
      if (!found)
        result.emplace_back(item, 1);
    }
  
  return result;
}

/**
 * @brief Filter out elements (reject in Ruby, opposite of filter).
 *
 * @param pred Predicate - elements where pred is TRUE are removed.
 * @param c Container.
 * @return Vector with elements not satisfying predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename Container>
[[nodiscard]] auto stl_reject(Pred&& pred, const Container& c)
{
  using T = typename Container::value_type;

  std::vector<T> result;
  for (const auto& item : c)
    if (!std::forward<Pred>(pred)(item))
      result.push_back(item);
  return result;
}

} // end namespace Aleph

# endif // AH_STL_FUNCTIONAL_H
