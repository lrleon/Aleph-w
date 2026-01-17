
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


# ifndef AH_ZIP_UTILS_H
# define AH_ZIP_UTILS_H

# include <type_traits>
# include <tuple>
# include <iterator>
# include <utility>
# include <optional>
# include <functional>

/**
 * @file ah-zip-utils.H
 * @brief Unified zip operations for both STL and Aleph containers.
 *
 * This header provides zip operations that work seamlessly with:
 * - **STL containers**: std::vector, std::list, std::set, etc.
 * - **Aleph containers**: DynList, DynArray, DynSetTree, etc.
 * - **Mixed combinations**: Zip a std::vector with a DynList
 *
 * The implementation uses type traits to detect the container type and
 * automatically adapts to the appropriate iterator interface.
 *
 * ## Usage Examples:
 *
 * @code
 * #include <vector>
 * #include <ah-zip-utils.H>
 * #include <htlist.H>
 *
 * std::vector<int> stl_vec = {1, 2, 3};
 * DynList<std::string> aleph_list = {"a", "b", "c"};
 *
 * // Mix STL and Aleph containers!
 * for (auto [num, str] : uni_zip(stl_vec, aleph_list))
 *   std::cout << num << ": " << str << "\n";
 *
 * // All functional operations work
 * bool all_ok = uni_zip_all([](auto t) { 
 *   return std::get<0>(t) > 0; 
 * }, stl_vec, aleph_list);
 * @endcode
 *
 * @note All zip operations stop at the shortest container.
 *
 * @ingroup Algorithms
 * @author Leandro Rabindranath Leon
 */

namespace Aleph
{

// ============================================================================
// Type Traits for Container Detection
// ============================================================================

namespace uni_zip_detail
{

// Detect if type has STL-style begin()/end()
template <typename T, typename = void>
struct has_stl_iterator : std::false_type {};

template <typename T>
struct has_stl_iterator<T, std::void_t<
  decltype(std::declval<const T&>().begin()),
  decltype(std::declval<const T&>().end()),
  typename T::value_type
>> : std::true_type {};

// Detect if type has Aleph-style Iterator with has_curr()/get_curr()/next()
template <typename T, typename = void>
struct has_aleph_iterator : std::false_type {};

template <typename T>
struct has_aleph_iterator<T, std::void_t<
  typename T::Iterator,
  typename T::Item_Type,
  decltype(std::declval<typename T::Iterator>().has_curr()),
  decltype(std::declval<typename T::Iterator>().get_curr()),
  decltype(std::declval<typename T::Iterator>().next())
>> : std::true_type {};

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
  has_aleph_iterator<T>::value && !has_stl_iterator<T>::value>>
{
  using type = std::decay_t<typename T::Item_Type>;
};

template <typename T>
using container_value_t = typename container_value_type<std::decay_t<T>>::type;

// ============================================================================
// Unified Iterator Wrapper
// ============================================================================

/**
 * @brief Wrapper that provides uniform interface for STL iterators.
 */
template <typename Container>
class StlIteratorWrapper
{
  typename std::decay_t<Container>::const_iterator curr_;
  typename std::decay_t<Container>::const_iterator end_;

public:
  using value_type = typename std::decay_t<Container>::value_type;

  explicit StlIteratorWrapper(const Container& c)
    : curr_(c.begin()), end_(c.end()) {}

  [[nodiscard]] bool has_curr() const noexcept { return curr_ != end_; }
  [[nodiscard]] bool completed() const noexcept { return curr_ == end_; }
  [[nodiscard]] auto get_curr() const { return *curr_; }
  void next() noexcept { ++curr_; }
};

/**
 * @brief Wrapper that provides uniform interface for Aleph iterators.
 */
template <typename Container>
class AlephIteratorWrapper
{
  typename std::decay_t<Container>::Iterator it_;

public:
  using value_type = std::decay_t<typename std::decay_t<Container>::Item_Type>;

  explicit AlephIteratorWrapper(const Container& c)
    : it_(c) {}

  [[nodiscard]] bool has_curr() const noexcept { return it_.has_curr(); }
  [[nodiscard]] bool completed() const noexcept { return !it_.has_curr(); }
  [[nodiscard]] auto get_curr() const { return it_.get_curr(); }
  void next() { it_.next_ne(); }
};

/**
 * @brief Select appropriate wrapper based on container type.
 */
template <typename Container, typename = void>
struct IteratorWrapperSelector;

// STL containers (prefer STL if both interfaces exist)
template <typename Container>
struct IteratorWrapperSelector<Container, 
  std::enable_if_t<has_stl_iterator<std::decay_t<Container>>::value>>
{
  using type = StlIteratorWrapper<std::decay_t<Container>>;
};

// Aleph-only containers (no STL interface)
template <typename Container>
struct IteratorWrapperSelector<Container,
  std::enable_if_t<
    has_aleph_iterator<std::decay_t<Container>>::value &&
    !has_stl_iterator<std::decay_t<Container>>::value>>
{
  using type = AlephIteratorWrapper<std::decay_t<Container>>;
};

template <typename Container>
using iterator_wrapper_t = typename IteratorWrapperSelector<Container>::type;

} // namespace uni_zip_detail

// ============================================================================
// Unified Zip Iterator
// ============================================================================

/**
 * @class UniZipIterator
 * @brief Iterator that traverses multiple containers (STL or Aleph) in lockstep.
 *
 * This iterator automatically adapts to the container type, allowing you to
 * zip together any combination of STL and Aleph containers.
 *
 * @tparam Containers Parameter pack of container types.
 *
 * @see uni_zip() Factory function
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
class UniZipIterator
{
public:
  using value_type = std::tuple<uni_zip_detail::container_value_t<Containers>...>;

private:
  std::tuple<uni_zip_detail::iterator_wrapper_t<Containers>...> iters_;

  template <size_t... Is>
  [[nodiscard]] bool has_curr_impl(std::index_sequence<Is...>) const noexcept
  {
    return (... && std::get<Is>(iters_).has_curr());
  }

  template <size_t... Is>
  [[nodiscard]] bool completed_impl(std::index_sequence<Is...>) const noexcept
  {
    return (... && std::get<Is>(iters_).completed());
  }

  template <size_t... Is>
  [[nodiscard]] auto get_curr_impl(std::index_sequence<Is...>) const
  {
    return std::make_tuple(std::get<Is>(iters_).get_curr()...);
  }

  template <size_t... Is>
  void next_impl(std::index_sequence<Is...>)
  {
    (std::get<Is>(iters_).next(), ...);
  }

public:
  static constexpr size_t num_containers = sizeof...(Containers);

  explicit UniZipIterator(const Containers&... cs)
    : iters_(uni_zip_detail::iterator_wrapper_t<Containers>(cs)...) {}

  [[nodiscard]] bool has_curr() const noexcept
  {
    return has_curr_impl(std::make_index_sequence<num_containers>{});
  }

  [[nodiscard]] bool completed() const noexcept
  {
    return completed_impl(std::make_index_sequence<num_containers>{});
  }

  [[nodiscard]] auto get_curr() const
  {
    return get_curr_impl(std::make_index_sequence<num_containers>{});
  }

  void next()
  {
    next_impl(std::make_index_sequence<num_containers>{});
  }

  // STL iterator interface for range-based for
  [[nodiscard]] auto operator*() const { return get_curr(); }

  UniZipIterator& operator++()
  {
    next();
    return *this;
  }

  [[nodiscard]] bool operator==(const UniZipIterator& other) const noexcept
  {
    return !has_curr() && !other.has_curr();
  }

  [[nodiscard]] bool operator!=(const UniZipIterator& other) const noexcept
  {
    return !(*this == other);
  }
};

/**
 * @class UniZipView
 * @brief Lazy view over multiple zipped containers (STL or Aleph).
 *
 * @tparam Containers Types of containers being zipped.
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
class UniZipView
{
  std::tuple<const std::decay_t<Containers>&...> containers_;

public:
  using iterator = UniZipIterator<std::decay_t<Containers>...>;
  using value_type = typename iterator::value_type;

  explicit UniZipView(const Containers&... cs)
    : containers_(cs...) {}

  [[nodiscard]] iterator begin() const
  {
    return std::apply([](const auto&... cs) {
      return iterator(cs...);
    }, containers_);
  }

  [[nodiscard]] iterator end() const
  {
    // Create an "exhausted" iterator for comparison
    return std::apply([](const auto&... cs) {
      auto it = iterator(cs...);
      while (it.has_curr())
        it.next();
      return it;
    }, containers_);
  }

  [[nodiscard]] bool empty() const
  {
    return !begin().has_curr();
  }

  [[nodiscard]] size_t size() const
  {
    size_t count = 0;
    for (auto it = begin(); it.has_curr(); it.next())
      ++count;
    return count;
  }
};

// ============================================================================
// Factory Functions
// ============================================================================

/**
 * @brief Create a lazy zip view over any combination of STL/Aleph containers.
 *
 * @param cs Containers to zip (any mix of STL and Aleph).
 * @return UniZipView for lazy iteration.
 *
 * @par Example:
 * @code
 * std::vector<int> v = {1, 2, 3};
 * DynList<std::string> l = {"a", "b", "c"};
 *
 * // Mix STL and Aleph!
 * for (auto [num, str] : uni_zip(v, l))
 *   std::cout << num << ": " << str << "\n";
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip(const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2,
                "uni_zip requires at least 2 containers");
  return UniZipView<Containers...>(cs...);
}

/**
 * @brief Get a unified zip iterator.
 *
 * @param cs Containers to zip.
 * @return UniZipIterator positioned at first element.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_it(const Containers&... cs)
{
  return UniZipIterator<std::decay_t<Containers>...>(cs...);
}

// ============================================================================
// Functional Operations
// ============================================================================

/**
 * @brief Check if predicate holds for all zipped tuples.
 *
 * @param pred Predicate `(tuple) -> bool`.
 * @param cs Containers to zip (STL or Aleph).
 * @return `true` if pred is true for all tuples.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] bool uni_zip_all(Pred&& pred, const Containers&... cs)
{
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    if (!std::forward<Pred>(pred)(it.get_curr()))
      return false;
  return true;
}

/**
 * @brief Check if all tuples satisfy predicate AND containers have equal length.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] bool uni_zip_all_eq(Pred&& pred, const Containers&... cs)
{
  auto it = uni_zip_it(cs...);
  for (; it.has_curr(); it.next())
    if (!std::forward<Pred>(pred)(it.get_curr()))
      return false;
  return it.completed();
}

/**
 * @brief Check if predicate holds for any zipped tuple.
 *
 * @param pred Predicate `(tuple) -> bool`.
 * @param cs Containers to zip.
 * @return `true` if any tuple satisfies pred.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] bool uni_zip_exists(Pred&& pred, const Containers&... cs)
{
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    if (std::forward<Pred>(pred)(it.get_curr()))
      return true;
  return false;
}

/// Alias for uni_zip_exists
template <typename Pred, typename... Containers>
[[nodiscard]] bool uni_zip_any(Pred&& pred, const Containers&... cs)
{
  return uni_zip_exists(std::forward<Pred>(pred), cs...);
}

/**
 * @brief Check if no tuple satisfies the predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] bool uni_zip_none(Pred&& pred, const Containers&... cs)
{
  return !uni_zip_exists(std::forward<Pred>(pred), cs...);
}

/**
 * @brief Apply operation to each zipped tuple.
 *
 * @param op Callable `(tuple) -> void`.
 * @param cs Containers to zip.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename... Containers>
void uni_zip_for_each(Op&& op, const Containers&... cs)
{
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    std::forward<Op>(op)(it.get_curr());
}

/**
 * @brief Apply operation to each tuple with index.
 *
 * @param op Callable `(size_t idx, tuple) -> void`.
 * @param cs Containers to zip.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename... Containers>
void uni_zip_for_each_indexed(Op&& op, const Containers&... cs)
{
  size_t idx = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next(), ++idx)
    std::forward<Op>(op)(idx, it.get_curr());
}

/**
 * @brief Left fold over zipped tuples.
 *
 * @param init Initial value.
 * @param op Binary operation `(acc, tuple) -> acc`.
 * @param cs Containers to zip.
 * @return Final accumulated value.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Op, typename... Containers>
[[nodiscard]] T uni_zip_foldl(T init, Op&& op, const Containers&... cs)
{
  T acc = std::move(init);
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    acc = std::forward<Op>(op)(std::move(acc), it.get_curr());
  return acc;
}

/// Alias for uni_zip_foldl
template <typename T, typename Op, typename... Containers>
[[nodiscard]] T uni_zip_reduce(T init, Op&& op, const Containers&... cs)
{
  return uni_zip_foldl(std::move(init), std::forward<Op>(op), cs...);
}

/**
 * @brief Map operation over zipped tuples.
 *
 * @param op Transformation `(tuple) -> R`.
 * @param cs Containers to zip.
 * @return std::vector of transformed values.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename... Containers>
[[nodiscard]] auto uni_zip_map(Op&& op, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;
  using ResultType = std::decay_t<decltype(std::forward<Op>(op)(std::declval<TupleType>()))>;

  std::vector<ResultType> result;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    result.push_back(std::forward<Op>(op)(it.get_curr()));
  return result;
}

/**
 * @brief Filter zipped tuples by predicate.
 *
 * @param pred Predicate `(tuple) -> bool`.
 * @param cs Containers to zip.
 * @return std::vector of tuples satisfying predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] auto uni_zip_filter(Pred&& pred, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (std::forward<Pred>(pred)(t))
        result.push_back(t);
    }
  return result;
}

/**
 * @brief Find first tuple satisfying predicate.
 *
 * @param pred Predicate to test.
 * @param cs Containers to zip.
 * @return std::optional containing tuple if found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] auto uni_zip_find_first(Pred&& pred, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (std::forward<Pred>(pred)(t))
        return std::optional<TupleType>(t);
    }
  return std::optional<TupleType>{};
}

/**
 * @brief Count tuples satisfying predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] size_t uni_zip_count(Pred&& pred, const Containers&... cs)
{
  size_t count = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    if (std::forward<Pred>(pred)(it.get_curr()))
      ++count;
  return count;
}

/**
 * @brief Get zip length (minimum of all container sizes).
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] size_t uni_zip_length(const Containers&... cs)
{
  size_t count = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    ++count;
  return count;
}

/**
 * @brief Check if all containers have equal length.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] bool uni_zip_equal_length(const Containers&... cs)
{
  auto it = uni_zip_it(cs...);
  while (it.has_curr())
    it.next();
  return it.completed();
}

/**
 * @brief Get n-th tuple from zipped containers.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_nth(size_t n, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  size_t i = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next(), ++i)
    if (i == n)
      return std::optional<TupleType>(it.get_curr());
  return std::optional<TupleType>{};
}

/**
 * @brief Take first n tuples.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_take(size_t n, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  result.reserve(n);
  size_t count = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr() && count < n; it.next(), ++count)
    result.push_back(it.get_curr());
  return result;
}

/**
 * @brief Skip first n tuples, return the rest.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_drop(size_t n, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  auto it = uni_zip_it(cs...);

  for (size_t i = 0; i < n && it.has_curr(); ++i)
    it.next();

  for (; it.has_curr(); it.next())
    result.push_back(it.get_curr());
  return result;
}

/**
 * @brief Take tuples while predicate is true.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] auto uni_zip_take_while(Pred&& pred, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (!std::forward<Pred>(pred)(t))
        break;
      result.push_back(t);
    }
  return result;
}

/**
 * @brief Skip tuples while predicate is true, return the rest.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] auto uni_zip_drop_while(Pred&& pred, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  auto it = uni_zip_it(cs...);

  for (; it.has_curr() && std::forward<Pred>(pred)(it.get_curr()); it.next())
    /* skip */;

  for (; it.has_curr(); it.next())
    result.push_back(it.get_curr());
  return result;
}

/**
 * @brief Get first tuple.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_first(const Containers&... cs)
{
  return uni_zip_nth(0, cs...);
}

/**
 * @brief Get last tuple.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_last(const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::optional<TupleType> result;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    result = it.get_curr();
  return result;
}

/**
 * @brief Partition tuples by predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] auto uni_zip_partition(Pred&& pred, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> matching, non_matching;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (std::forward<Pred>(pred)(t))
        matching.push_back(t);
      else
        non_matching.push_back(t);
    }
  return std::make_pair(std::move(matching), std::move(non_matching));
}

/**
 * @brief Materialize zipped tuples into a vector.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_to_vector(const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    result.push_back(it.get_curr());
  return result;
}

/**
 * @brief Traverse while predicate returns true.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] bool uni_zip_traverse(Pred&& pred, const Containers&... cs)
{
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    if (!std::forward<Pred>(pred)(it.get_curr()))
      return false;
  return true;
}

// ============================================================================
// ML-style Additional Operations
// ============================================================================

/**
 * @brief Map with index (mapi in ML).
 *
 * @param op Callable `(size_t idx, tuple) -> R`.
 * @param cs Containers to zip (STL or Aleph).
 * @return std::vector of transformed values.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename... Containers>
[[nodiscard]] auto uni_zip_mapi(Op&& op, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;
  using ResultType = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<TupleType>()))>;

  std::vector<ResultType> result;
  size_t idx = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next(), ++idx)
    result.push_back(std::forward<Op>(op)(idx, it.get_curr()));
  return result;
}

/**
 * @brief Filter with index (filteri in ML).
 *
 * @param pred Predicate `(size_t idx, tuple) -> bool`.
 * @param cs Containers to zip.
 * @return std::vector of tuples satisfying predicate.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Pred, typename... Containers>
[[nodiscard]] auto uni_zip_filteri(Pred&& pred, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  std::vector<TupleType> result;
  size_t idx = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next(), ++idx)
    {
      auto t = it.get_curr();
      if (std::forward<Pred>(pred)(idx, t))
        result.push_back(t);
    }
  return result;
}

/**
 * @brief Scan left - fold with intermediate results.
 *
 * @param init Initial value.
 * @param op Binary operation `(acc, tuple) -> acc`.
 * @param cs Containers to zip.
 * @return std::vector containing init and all intermediate results.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename T, typename Op, typename... Containers>
[[nodiscard]] auto uni_zip_scan_left(T init, Op&& op, const Containers&... cs)
{
  std::vector<T> result;
  result.push_back(init);

  T acc = std::move(init);
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    {
      acc = std::forward<Op>(op)(std::move(acc), it.get_curr());
      result.push_back(acc);
    }
  return result;
}

/**
 * @brief Find and map with index (find_mapi in ML).
 *
 * @param op Callable `(size_t idx, tuple) -> std::optional<R>`.
 * @param cs Containers to zip.
 * @return First non-empty result from op, or std::nullopt.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Op, typename... Containers>
[[nodiscard]] auto uni_zip_find_mapi(Op&& op, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;
  using OptType = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<TupleType>()))>;

  size_t idx = 0;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next(), ++idx)
    {
      auto result = std::forward<Op>(op)(idx, it.get_curr());
      if (result)
        return result;
    }
  return OptType{};
}

/**
 * @brief Check equality with custom comparator.
 *
 * @param eq Equality predicate `(tuple) -> bool`.
 * @param cs Containers to zip.
 * @return `true` if all tuples satisfy eq AND containers have equal length.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Eq, typename... Containers>
[[nodiscard]] bool uni_zip_equal_by(Eq&& eq, const Containers&... cs)
{
  auto it = uni_zip_it(cs...);
  for (; it.has_curr(); it.next())
    if (!std::forward<Eq>(eq)(it.get_curr()))
      return false;
  return it.completed();
}

/**
 * @brief Check if a tuple exists in the zipped sequence (mem in ML).
 *
 * @param target The tuple to search for.
 * @param cs Containers to zip.
 * @return `true` if target is found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Tuple, typename... Containers>
[[nodiscard]] bool uni_zip_mem(const Tuple& target, const Containers&... cs)
{
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    if (it.get_curr() == target)
      return true;
  return false;
}

/**
 * @brief Find value associated with key in zipped pairs (assoc in ML).
 *
 * @param key The key to search for.
 * @param cs Containers to zip (first container contains keys).
 * @return Optional containing the full tuple if found.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Key, typename... Containers>
[[nodiscard]] auto uni_zip_assoc(const Key& key, const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (std::get<0>(t) == key)
        return std::optional<TupleType>(t);
    }
  return std::optional<TupleType>{};
}

/**
 * @brief Get minimum tuple.
 *
 * @param cs Containers to zip.
 * @return Optional containing the minimum tuple.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_min(const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  auto it = uni_zip_it(cs...);
  if (!it.has_curr())
    return std::optional<TupleType>{};

  TupleType min_val = it.get_curr();
  it.next();

  for (; it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (t < min_val)
        min_val = t;
    }
  return std::optional<TupleType>(min_val);
}

/**
 * @brief Get maximum tuple.
 *
 * @param cs Containers to zip.
 * @return Optional containing the maximum tuple.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_max(const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  auto it = uni_zip_it(cs...);
  if (!it.has_curr())
    return std::optional<TupleType>{};

  TupleType max_val = it.get_curr();
  it.next();

  for (; it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (t > max_val)
        max_val = t;
    }
  return std::optional<TupleType>(max_val);
}

/**
 * @brief Get both min and max in a single pass.
 *
 * @param cs Containers to zip.
 * @return Optional pair of (min, max) tuples.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_min_max(const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;
  using ResultType = std::optional<std::pair<TupleType, TupleType>>;

  auto it = uni_zip_it(cs...);
  if (!it.has_curr())
    return ResultType{};

  TupleType min_val = it.get_curr();
  TupleType max_val = min_val;
  it.next();

  for (; it.has_curr(); it.next())
    {
      auto t = it.get_curr();
      if (t < min_val) min_val = t;
      if (t > max_val) max_val = t;
    }
  return ResultType(std::make_pair(min_val, max_val));
}

// ============================================================================
// Unzip Operations (returns DynList for mixed containers)
// ============================================================================

} // end namespace Aleph

// Need DynList for unzip operations
# include <htlist.H>

namespace Aleph
{

/**
 * @brief Unzip a container of pairs into two DynLists.
 *
 * Works with any container (STL or Aleph) containing pairs.
 * Returns DynList since this is for mixed container scenarios.
 *
 * @param c Container of pairs to unzip.
 * @return Pair of DynLists containing separated elements.
 *
 * @par Example:
 * @code
 * std::vector<std::pair<int, std::string>> pairs = {{1, "a"}, {2, "b"}};
 * auto [nums, strs] = uni_unzip(pairs);
 * // nums is DynList<int>{1, 2}, strs is DynList<std::string>{"a", "b"}
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto uni_unzip(const Container& c)
{
  using PairType = uni_zip_detail::container_value_t<Container>;
  using T1 = std::decay_t<decltype(std::declval<PairType>().first)>;
  using T2 = std::decay_t<decltype(std::declval<PairType>().second)>;

  DynList<T1> firsts;
  DynList<T2> seconds;

  if constexpr (uni_zip_detail::has_stl_iterator<std::decay_t<Container>>::value)
    {
      for (const auto& p : c)
        {
          firsts.append(p.first);
          seconds.append(p.second);
        }
    }
  else
    {
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        {
          const auto& p = it.get_curr();
          firsts.append(p.first);
          seconds.append(p.second);
        }
    }

  return std::make_pair(std::move(firsts), std::move(seconds));
}

namespace uni_unzip_detail
{

template <typename Tuple, size_t... Is>
auto make_dynlist_tuple(std::index_sequence<Is...>)
{
  return std::make_tuple(DynList<std::tuple_element_t<Is, Tuple>>()...);
}

template <typename Tuple, typename ResultTuple, size_t... Is>
void append_tuple_elements(const Tuple& t, ResultTuple& result, std::index_sequence<Is...>)
{
  ((std::get<Is>(result).append(std::get<Is>(t))), ...);
}

} // namespace uni_unzip_detail

/**
 * @brief Unzip a container of tuples into a tuple of DynLists.
 *
 * Works with any container (STL or Aleph) containing tuples.
 * Returns tuple of DynLists since this is for mixed container scenarios.
 *
 * @param c Container of tuples to unzip.
 * @return Tuple of DynLists, one for each tuple element.
 *
 * @par Example:
 * @code
 * std::vector<std::tuple<int, double, char>> tuples = {
 *   {1, 1.1, 'a'}, {2, 2.2, 'b'}
 * };
 * auto [ints, doubles, chars] = uni_unzip_tuple(tuples);
 * // Each is a DynList
 * @endcode
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename Container>
[[nodiscard]] auto uni_unzip_tuple(const Container& c)
{
  using TupleType = uni_zip_detail::container_value_t<Container>;
  constexpr size_t N = std::tuple_size_v<TupleType>;

  auto result = uni_unzip_detail::make_dynlist_tuple<TupleType>(
    std::make_index_sequence<N>{});

  if constexpr (uni_zip_detail::has_stl_iterator<std::decay_t<Container>>::value)
    {
      for (const auto& t : c)
        uni_unzip_detail::append_tuple_elements(t, result,
          std::make_index_sequence<N>{});
    }
  else
    {
      for (auto it = c.get_it(); it.has_curr(); it.next_ne())
        uni_unzip_detail::append_tuple_elements(it.get_curr(), result,
          std::make_index_sequence<N>{});
    }

  return result;
}

/**
 * @brief Materialize a zipped view into a DynList of tuples.
 *
 * Converts a lazy zip view into a concrete DynList.
 *
 * @param cs Containers to zip (STL or Aleph).
 * @return DynList of tuples.
 *
 * @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */
template <typename... Containers>
[[nodiscard]] auto uni_zip_to_dynlist(const Containers&... cs)
{
  using TupleType = typename UniZipIterator<std::decay_t<Containers>...>::value_type;

  DynList<TupleType> result;
  for (auto it = uni_zip_it(cs...); it.has_curr(); it.next())
    result.append(it.get_curr());
  return result;
}

} // end namespace Aleph

# endif // AH_ZIP_UTILS_H
