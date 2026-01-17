
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


# ifndef AH_UNI_FUNCTIONAL_H
# define AH_UNI_FUNCTIONAL_H

/**
 * @file ah-uni-functional.H
 * @brief Unified functional programming utilities for both STL and Aleph containers.
 *
 * This header provides functional programming operations that work seamlessly
 * with any container type:
 * - **STL containers**: std::vector, std::list, std::deque, std::set, etc.
 * - **Aleph containers**: DynList, DynArray, DynDlist, DynSetTree, etc.
 *
 * The implementation uses type traits to automatically detect the container type
 * and apply the appropriate iteration method.
 *
 * ## Usage Example:
 * @code
 * #include <ah-uni-functional.H>
 * #include <htlist.H>
 * #include <vector>
 *
 * std::vector<int> stl_vec = {1, 2, 3, 4, 5};
 * DynList<int> aleph_list = {10, 20, 30, 40, 50};
 *
 * // Same function works with both!
 * auto stl_squares = uni_map([](int x) { return x * x; }, stl_vec);
 * auto aleph_squares = uni_map([](int x) { return x * x; }, aleph_list);
 *
 * int stl_sum = uni_foldl(0, std::plus<int>{}, stl_vec);
 * int aleph_sum = uni_foldl(0, std::plus<int>{}, aleph_list);
 * @endcode
 *
 * @note `foldr` is only available for STL containers with reverse iterators.
 *
 * @ingroup Algorithms
 * @author Leandro Rabindranath Leon
 */

# include <type_traits>
# include <vector>
# include <optional>
# include <functional>
# include <algorithm>
# include <utility>
# include <tuple>
# include <iterator>

namespace Aleph
{
  // ============================================================================
  // Type Traits for Container Detection
  // ============================================================================

  namespace uni_functional_detail
  {
    // Detect if type has STL-style begin()/end()
    template <typename T, typename = void>
    struct has_stl_iterator : std::false_type
    {};

    template <typename T>
    struct has_stl_iterator<T, std::void_t<
                              decltype(std::declval<const T &>().begin()),
                              decltype(std::declval<const T &>().end()),
                              typename T::value_type
                            >> : std::true_type
    {};

    // Detect if type has Aleph-style Iterator with has_curr()/get_curr()/next()
    template <typename T, typename = void>
    struct has_aleph_iterator : std::false_type
    {};

    template <typename T>
    struct has_aleph_iterator<T, std::void_t<
                                typename T::Iterator,
                                typename T::Item_Type,
                                decltype(std::declval<typename T::Iterator>().has_curr()),
                                decltype(std::declval<typename T::Iterator>().get_curr()),
                                decltype(std::declval<typename T::Iterator>().next())
                              >> : std::true_type
    {};

    // Detect if type has reverse iterators (rbegin/rend)
    template <typename T, typename = void>
    struct has_reverse_iterator : std::false_type
    {};

    template <typename T>
    struct has_reverse_iterator<T, std::void_t<
                                  decltype(std::declval<const T &>().rbegin()),
                                  decltype(std::declval<const T &>().rend())
                                >> : std::true_type
    {};

    // Get value type from container
    template <typename T, typename = void>
    struct container_value_type;

    template <typename T>
    struct container_value_type<T, std::enable_if_t<has_stl_iterator<T>::value>>
    {
      using type = typename T::value_type;
    };

    template <typename T>
    struct container_value_type<T, std::enable_if_t<
                                  has_aleph_iterator<T>::value && ! has_stl_iterator<T>::value>>
    {
      using type = std::decay_t<typename T::Item_Type>;
    };

    template <typename T>
    using value_t = typename container_value_type<std::decay_t<T>>::type;

    // Check if container is STL-style
    template <typename T>
    constexpr bool is_stl_container_v = has_stl_iterator<std::decay_t<T>>::value;

    // Check if container is Aleph-only
    template <typename T>
    constexpr bool is_aleph_only_v =
        has_aleph_iterator<std::decay_t<T>>::value &&
        ! has_stl_iterator<std::decay_t<T>>::value;
  } // namespace uni_functional_detail

  // ============================================================================
  // Core Functional Operations
  // ============================================================================

  /**
   * @brief Apply operation to each element (for_each).
   *
   * Works with both STL and Aleph containers.
   *
   * @param op Callable `(element) -> void`.
   * @param c Container (STL or Aleph).
   *
   * @ingroup Algorithms
   */
  template <typename Op, typename Container>
  void uni_for_each(Op && op, const Container & c)
  {
    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      for (const auto & item: c)
        std::forward<Op>(op)(item);
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        std::forward<Op>(op)(it.get_curr());
  }

  /**
   * @brief Apply operation to each element with index.
   *
   * @param op Callable `(size_t idx, element) -> void`.
   * @param c Container.
   *
   * @ingroup Algorithms
   */
  template <typename Op, typename Container>
  void uni_for_each_indexed(Op && op, const Container & c)
  {
    size_t i = 0;
    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      for (const auto & item: c)
        std::forward<Op>(op)(i++, item);
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++i)
        std::forward<Op>(op)(i, it.get_curr());
  }

  /**
   * @brief Map operation - transform each element.
   *
   * @param op Callable `(element) -> R`.
   * @param c Container.
   * @return std::vector<R> with transformed values.
   *
   * @ingroup Algorithms
   */
  template <typename Op, typename Container>
  [[nodiscard]] auto uni_map(Op && op, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;
    using R = std::decay_t<decltype(std::forward<Op>(op)(std::declval<T>()))>;

    std::vector<R> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        result.reserve(c.size());
        for (const auto & item: c)
          result.push_back(std::forward<Op>(op)(item));
      }
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        result.push_back(std::forward<Op>(op)(it.get_curr()));

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
   */
  template <typename Op, typename Container>
  [[nodiscard]] auto uni_mapi(Op && op, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;
    using R = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<T>()))>;

    std::vector<R> result;
    size_t i = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        result.reserve(c.size());
        for (const auto & item: c)
          result.push_back(std::forward<Op>(op)(i++, item));
      }
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++i)
        result.push_back(std::forward<Op>(op)(i, it.get_curr()));

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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] auto uni_filter(Pred && pred, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (std::forward<Pred>(pred)(item))
            result.push_back(item);
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          if (const auto & item = it.get_curr(); std::forward<Pred>(pred)(item))
            result.push_back(item);
      }
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] auto uni_filteri(Pred && pred, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;
    size_t i = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      for (const auto & item: c)
        {
          if (std::forward<Pred>(pred)(i, item))
            result.push_back(item);
          ++i;
        }
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++i)
        if (const auto & item = it.get_curr(); std::forward<Pred>(pred)(i, item))
          result.push_back(item);

    return result;
  }

  /**
   * @brief Left fold (foldl) - reduce from left to right.
   *
   * @param init Initial accumulator value.
   * @param op Binary operation `(acc, element) -> acc`.
   * @param c Container.
   * @return Final accumulated value.
   *
   * @ingroup Algorithms
   */
  template <typename T, typename Op, typename Container>
  [[nodiscard]] T uni_foldl(T init, Op && op, const Container & c)
  {
    T acc = std::move(init);

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      for (const auto & item: c)
        acc = std::forward<Op>(op)(std::move(acc), item);
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        acc = std::forward<Op>(op)(std::move(acc), it.get_curr());
    return acc;
  }

  /// Alias for uni_foldl
  template <typename T, typename Op, typename Container>
  [[nodiscard]] T uni_reduce(T init, Op && op, const Container & c)
  {
    return uni_foldl(std::move(init), std::forward<Op>(op), c);
  }

  /**
   * @brief Scan left - fold with all intermediate results.
   *
   * @param init Initial value.
   * @param op Binary operation `(acc, element) -> acc`.
   * @param c Container.
   * @return std::vector containing init and all intermediate results.
   *
   * @ingroup Algorithms
   */
  template <typename T, typename Op, typename Container>
  [[nodiscard]] std::vector<T> uni_scan_left(T init, Op && op, const Container & c)
  {
    std::vector<T> result;
    result.push_back(init);

    T acc = std::move(init);

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      for (const auto & item: c)
        {
          acc = std::forward<Op>(op)(std::move(acc), item);
          result.push_back(acc);
        }
    else
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        {
          acc = std::forward<Op>(op)(std::move(acc), it.get_curr());
          result.push_back(acc);
        }
    return result;
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] bool uni_all(Pred && pred, const Container & c)
  {
    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (not std::forward<Pred>(pred)(item))
            return false;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          if (not std::forward<Pred>(pred)(it.get_curr()))
            return false;
      }
    return true;
  }

  /**
   * @brief Check if any element satisfies predicate.
   *
   * @param pred Predicate `(element) -> bool`.
   * @param c Container.
   * @return `true` if any element satisfies predicate.
   *
   * @ingroup Algorithms
   */
  template <typename Pred, typename Container>
  [[nodiscard]] bool uni_exists(Pred && pred, const Container & c)
  {
    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (std::forward<Pred>(pred)(item))
            return true;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          if (std::forward<Pred>(pred)(it.get_curr()))
            return true;
      }
    return false;
  }

  /// Alias for uni_exists
  template <typename Pred, typename Container>
  [[nodiscard]] bool uni_any(Pred && pred, const Container & c)
  {
    return uni_exists(std::forward<Pred>(pred), c);
  }

  /**
   * @brief Check if no element satisfies predicate.
   *
   * @ingroup Algorithms
   */
  template <typename Pred, typename Container>
  [[nodiscard]] bool uni_none(Pred && pred, const Container & c)
  {
    return ! uni_exists(std::forward<Pred>(pred), c);
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] auto uni_find(Pred && pred, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (std::forward<Pred>(pred)(item))
            return std::optional<T>(item);
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (std::forward<Pred>(pred)(item))
              return std::optional<T>(item);
          }
      }
    return std::optional<T>{};
  }

  /**
   * @brief Find index of first element satisfying predicate.
   *
   * @param pred Predicate `(element) -> bool`.
   * @param c Container.
   * @return std::optional<size_t> with index if found.
   *
   * @ingroup Algorithms
   */
  template <typename Pred, typename Container>
  [[nodiscard]] std::optional<size_t> uni_find_index(Pred && pred, const Container & c)
  {
    size_t i = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (std::forward<Pred>(pred)(item))
              return i;
            ++i;
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++i)
          if (std::forward<Pred>(pred)(it.get_curr()))
            return i;
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
   */
  template <typename Op, typename Container>
  [[nodiscard]] auto uni_find_mapi(Op && op, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;
    using OptType = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<T>()))>;

    size_t i = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (auto result = std::forward<Op>(op)(i++, item))
            return result;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++i)
          if (auto result = std::forward<Op>(op)(i, it.get_curr()))
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
   */
  template <typename T, typename Container>
  [[nodiscard]] bool uni_mem(const T & target, const Container & c)
  {
    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (item == target)
            return true;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          if (it.get_curr() == target)
            return true;
      }
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] size_t uni_count(Pred && pred, const Container & c)
  {
    size_t count = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          if (std::forward<Pred>(pred)(item))
            ++count;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          if (std::forward<Pred>(pred)(it.get_curr()))
            ++count;
      }
    return count;
  }

  /**
   * @brief Get container length.
   *
   * @param c Container.
   * @return Number of elements.
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] size_t uni_length(const Container & c)
  {
    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        return c.size();
      }
    else
      {
        size_t count = 0;
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          ++count;
        return count;
      }
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
   */
  template <typename Container>
  [[nodiscard]] auto uni_take(size_t n, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;
    result.reserve(n);
    size_t count = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (count >= n) break;
            result.push_back(item);
            ++count;
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr() && count < n; it.next_ne(), ++count)
          result.push_back(it.get_curr());
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
   */
  template <typename Container>
  [[nodiscard]] auto uni_drop(size_t n, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;
    size_t count = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (count >= n)
              result.push_back(item);
            ++count;
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++count)
          if (count >= n)
            result.push_back(it.get_curr());
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] auto uni_take_while(Pred && pred, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (! std::forward<Pred>(pred)(item))
              break;
            result.push_back(item);
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (! std::forward<Pred>(pred)(item))
              break;
            result.push_back(item);
          }
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] auto uni_drop_while(Pred && pred, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;
    bool dropping = true;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (dropping && std::forward<Pred>(pred)(item))
              continue;
            dropping = false;
            result.push_back(item);
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (dropping && std::forward<Pred>(pred)(item))
              continue;
            dropping = false;
            result.push_back(item);
          }
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
   */
  template <typename Container>
  [[nodiscard]] auto uni_first(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        auto it = c.begin();
        if (it == c.end())
          return std::optional<T>{};
        return std::optional<T>(*it);
      }
    else
      {
        auto it = c.get_it();
        if (! it.has_curr())
          return std::optional<T>{};
        return std::optional<T>(it.get_curr());
      }
  }

  /**
   * @brief Get last element.
   *
   * @param c Container.
   * @return std::optional with last element if container is not empty.
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] auto uni_last(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::optional<T> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          result = item;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          result = it.get_curr();
      }
    return result;
  }

  /**
   * @brief Get n-th element.
   *
   * @param n Index (0-based).
   * @param c Container.
   * @return std::optional with n-th element if exists.
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] auto uni_nth(size_t n, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    size_t i = 0;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (i == n)
              return std::optional<T>(item);
            ++i;
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne(), ++i)
          if (i == n)
            return std::optional<T>(it.get_curr());
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
   */
  template <typename Container>
  [[nodiscard]] auto uni_min(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::optional<T> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        auto it = c.begin();
        if (it == c.end())
          return result;

        T min_val = *it;
        ++it;
        for (; it != c.end(); ++it)
          if (*it < min_val)
            min_val = *it;
        return std::optional<T>(min_val);
      }
    else
      {
        auto it = c.get_it();
        if (! it.has_curr())
          return result;

        T min_val = it.get_curr();
        it.next_ne();
        for (; it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (item < min_val)
              min_val = item;
          }
        return std::optional<T>(min_val);
      }
  }

  /**
   * @brief Get maximum element.
   *
   * @param c Container.
   * @return std::optional with maximum element.
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] auto uni_max(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::optional<T> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        auto it = c.begin();
        if (it == c.end())
          return result;

        T max_val = *it;
        ++it;
        for (; it != c.end(); ++it)
          if (*it > max_val)
            max_val = *it;
        return std::optional<T>(max_val);
      }
    else
      {
        auto it = c.get_it();
        if (! it.has_curr())
          return result;

        T max_val = it.get_curr();
        it.next_ne();
        for (; it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (item > max_val)
              max_val = item;
          }
        return std::optional<T>(max_val);
      }
  }

  /**
   * @brief Get both min and max in a single pass.
   *
   * @param c Container.
   * @return std::optional<std::pair<T, T>> with (min, max).
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] auto uni_min_max(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;
    using ResultType = std::optional<std::pair<T, T>>;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
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
    else
      {
        auto it = c.get_it();
        if (! it.has_curr())
          return ResultType{};

        T min_val = it.get_curr();
        T max_val = min_val;
        it.next_ne();

        for (; it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (item < min_val) min_val = item;
            if (item > max_val) max_val = item;
          }
        return ResultType(std::make_pair(min_val, max_val));
      }
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
   */
  template <typename Container>
  [[nodiscard]] auto uni_sum(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;
    T sum{};

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          sum = sum + item;
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          sum = sum + it.get_curr();
      }
    return sum;
  }

  /**
   * @brief Product of all elements.
   *
   * @param c Container of numeric values.
   * @return Product of all elements.
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] auto uni_product(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        auto it = c.begin();
        if (it == c.end()) return T{};

        T prod = *it;
        ++it;
        for (; it != c.end(); ++it)
          prod = prod * (*it);
        return prod;
      }
    else
      {
        auto it = c.get_it();
        if (! it.has_curr()) return T{};

        T prod = it.get_curr();
        it.next_ne();
        for (; it.has_curr(); it.next_ne())
          prod = prod * it.get_curr();
        return prod;
      }
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
   */
  template <typename Pred, typename Container>
  [[nodiscard]] auto uni_partition(Pred && pred, const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> matching, non_matching;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        for (const auto & item: c)
          {
            if (std::forward<Pred>(pred)(item))
              matching.push_back(item);
            else
              non_matching.push_back(item);
          }
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            if (std::forward<Pred>(pred)(item))
              matching.push_back(item);
            else
              non_matching.push_back(item);
          }
      }
    return std::make_pair(std::move(matching), std::move(non_matching));
  }

  // ============================================================================
  // Conversion
  // ============================================================================

  /**
   * @brief Convert container to std::vector.
   *
   * @param c Container (STL or Aleph).
   * @return std::vector with all elements.
   *
   * @ingroup Algorithms
   */
  template <typename Container>
  [[nodiscard]] auto uni_to_vector(const Container & c)
  {
    using T = uni_functional_detail::value_t<Container>;

    std::vector<T> result;

    if constexpr (uni_functional_detail::is_stl_container_v<Container>)
      {
        result.reserve(c.size());
        for (const auto & item: c)
          result.push_back(item);
      }
    else
      {
        for (auto it = c.get_it(); it.has_curr(); it.next_ne())
          result.push_back(it.get_curr());
      }
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
   */
  template <typename Container1, typename Container2>
  [[nodiscard]] bool uni_equal(const Container1 & c1, const Container2 & c2)
  {
    auto v1 = uni_to_vector(c1);
    auto v2 = uni_to_vector(c2);

    if (v1.size() != v2.size())
      return false;

    for (size_t i = 0; i < v1.size(); ++i)
      if (! (v1[i] == v2[i]))
        return false;

    return true;
  }

  /**
   * @brief Compare two containers lexicographically.
   *
   * @param c1 First container.
   * @param c2 Second container.
   * @return -1 if c1 < c2, 0 if equal, 1 if c1 > c2.
   *
   * @ingroup Algorithms
   */
  template <typename Container1, typename Container2>
  [[nodiscard]] int uni_compare(const Container1 & c1, const Container2 & c2)
  {
    auto v1 = uni_to_vector(c1);
    auto v2 = uni_to_vector(c2);

    const size_t min_size = std::min(v1.size(), v2.size());

    for (size_t i = 0; i < min_size; ++i)
      {
        if (v1[i] < v2[i]) return -1;
        if (v2[i] < v1[i]) return 1;
      }

    if (v1.size() < v2.size()) return -1;
    if (v1.size() > v2.size()) return 1;
    return 0;
  }
} // end namespace Aleph

# endif // AH_UNI_FUNCTIONAL_H
