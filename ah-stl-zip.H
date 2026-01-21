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


# ifndef AH_STL_ZIP_H
# define AH_STL_ZIP_H

# include <type_traits>
# include <tuple>
# include <iterator>
# include <utility>
# include <optional>
# include <functional>


/**
 * @file ah-stl-zip.H
 * @brief Lazy zip iterators and functional operations for STL containers.
 *
 * This header provides lazy (non-materializing) zip operations for C++ STL
 * containers such as std::vector, std::list, std::set, std::deque, etc.
 *
 * **Key Features:**
 * - **Zero additional memory allocation**: All operations iterate inline
 * - **Works with any STL-compatible container**: vector, list, set, map, deque, array, etc.
 * - **Arbitrary number of containers**: Zip 2, 3, or more containers of different types
 * - **Functional operations**: all, exists, map, foldl, for_each in Aleph style
 * - **Range-based for loop support**: Use with modern C++ for(:) syntax
 *
 * ## Usage Examples:
 *
 * @code
 * #include <vector>
 * #include <list>
 * #include <ah-stl-zip.H>
 *
 * std::vector<int> nums = {1, 2, 3, 4};
 * std::list<std::string> names = {"a", "b", "c", "d"};
 *
 * // Range-based for loop (lazy, no allocation)
 * for (auto [n, s] : stl_zip(nums, names))
 *   std::cout << n << ": " << s << "\n";
 *
 * // Functional operations
 * bool all_positive = stl_zip_all([](auto t) {
 *   return std::get<0>(t) > 0;
 * }, nums, names);
 *
 * int sum = stl_zip_foldl(0, [](int acc, auto t) {
 *   return acc + std::get<0>(t);
 * }, nums, names);
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
  // StlZipIterator - Core iterator for zipping STL containers
  // ============================================================================

  namespace stl_zip_detail
  {
    // Check if all iterators are valid (not at end)
    template <typename... IterPairs>
    constexpr bool all_valid(const IterPairs &... its) noexcept
    {
      return (... and (std::get<0>(its) != std::get<1>(its)));
    }

    // Advance all iterators
    template <typename... IterPairs>
    constexpr void advance_all(IterPairs &... its) noexcept
    {
      (++std::get<0>(its), ...);
    }

    // Dereference all iterators and make a tuple
    template <typename... IterPairs>
    constexpr auto deref_all(const IterPairs &... its)
    {
      return std::tuple<decltype(*std::get<0>(its))...>(*std::get<0>(its)...);
    }

    // Get value types from iterator pairs
    template <typename IterPair>
    using iter_value_t = std::decay_t<decltype(*std::get<0>(std::declval<IterPair>()))>;
  } // namespace stl_zip_detail

  /**
   * @class StlZipIterator
   * @brief Lazy iterator that traverses multiple STL containers in lockstep.
   *
   * This iterator allows synchronized iteration over multiple STL containers
   * without allocating any additional memory. It produces tuples of references
   * to elements at each position.
   *
   * The iterator satisfies the InputIterator concept and can be used with
   * range-based for loops and STL algorithms.
   *
   * @tparam Containers Parameter pack of container types.
   *
   * @note Iteration stops when ANY underlying iterator reaches its end.
   *
   * @see stl_zip() Factory function to create zip views
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  class StlZipIterator
  {
  public:
    // Iterator type aliases for STL compatibility
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    // Each element is a pair of (current_iterator, end_iterator)
    using IteratorTuple = std::tuple<
      std::pair<typename std::decay_t<Containers>::const_iterator,
                typename std::decay_t<Containers>::const_iterator>...>;

    // Value type is a tuple of references to elements
    using value_type = std::tuple<
      typename std::decay_t<Containers>::value_type...>;

    using reference = std::tuple<
      const typename std::decay_t<Containers>::value_type &...>;

    using pointer = void;

  private:
    IteratorTuple iters_;

    // Helper to check if all iterators are valid
    template <size_t... Is>
    [[nodiscard]] constexpr bool has_curr_impl(std::index_sequence<Is...>) const noexcept
    {
      return stl_zip_detail::all_valid(std::get<Is>(iters_)...);
    }

    // Helper to dereference all iterators
    template <size_t... Is>
    [[nodiscard]] constexpr auto deref_impl(std::index_sequence<Is...>) const
    {
      return stl_zip_detail::deref_all(std::get<Is>(iters_)...);
    }

    // Helper to advance all iterators
    template <size_t... Is>
    constexpr void advance_impl(std::index_sequence<Is...>) noexcept
    {
      stl_zip_detail::advance_all(std::get<Is>(iters_)...);
    }

    // Helper to check if all iterators completed
    template <size_t... Is>
    [[nodiscard]] constexpr bool completed_impl(std::index_sequence<Is...>) const noexcept
    {
      return (... and (std::get<0>(std::get<Is>(iters_)) ==
                       std::get<1>(std::get<Is>(iters_))));
    }

  public:
    /// Number of containers being zipped
    static constexpr size_t num_containers = sizeof...(Containers);

    /** @brief Construct iterator from containers.
     *
     * @param cs References to containers to zip.
     */
    explicit constexpr StlZipIterator(const Containers &... cs)
      : iters_(std::make_pair(cs.begin(), cs.end())...) {}

    /** @brief Construct end iterator (all at end).
     *
     * @param cs References to containers.
     * @param tag Disambiguation tag for end iterator.
     */
    struct end_tag
    {};

    constexpr StlZipIterator(const Containers &... cs, end_tag)
      : iters_(std::make_pair(cs.end(), cs.end())...) {}

    /** @brief Check if iterator has current element.
     *
     * @return `true` if all underlying iterators are valid.
     */
    [[nodiscard]] constexpr bool has_curr() const noexcept
    {
      return has_curr_impl(std::make_index_sequence<num_containers>{});
    }

    /** @brief Check if all iterators have completed.
     *
     * Useful to verify equal-length containers after iteration.
     *
     * @return `true` if all underlying iterators are at their end.
     */
    [[nodiscard]] constexpr bool completed() const noexcept
    {
      return completed_impl(std::make_index_sequence<num_containers>{});
    }

    /** @brief Get current tuple of elements.
     *
     * @return Tuple of references to current elements.
     * @pre has_curr() must be true.
     */
    [[nodiscard]] constexpr auto get_curr() const
    {
      return deref_impl(std::make_index_sequence<num_containers>{});
    }

    /** @brief Advance all iterators.
     *
     * @pre has_curr() must be true.
     */
    constexpr void next() noexcept
    {
      advance_impl(std::make_index_sequence<num_containers>{});
    }

    // STL iterator interface

    /** @brief Dereference operator for range-based for.
     *
     * @return Tuple of current elements.
     */
    [[nodiscard]] constexpr auto operator*() const
    {
      return get_curr();
    }

    /** @brief Pre-increment operator.
     *
     * @return Reference to this iterator after advancing.
     */
    constexpr StlZipIterator &operator++() noexcept
    {
      next();
      return *this;
    }

    /** @brief Post-increment operator.
     *
     * @return Copy of iterator before advancing.
     */
    constexpr StlZipIterator operator++(int) noexcept
    {
      auto tmp = *this;
      next();
      return tmp;
    }

    /** @brief Equality comparison.
     *
     * Two iterators are equal if both have no current element.
     */
    [[nodiscard]] constexpr bool operator==(const StlZipIterator & other) const noexcept
    {
      return not has_curr() and not other.has_curr();
    }

    /** @brief Inequality comparison. */
    [[nodiscard]] constexpr bool operator!=(const StlZipIterator & other) const noexcept
    {
      return not (*this == other);
    }
  };

  /**
   * @class StlZipView
   * @brief Lazy view over multiple zipped STL containers.
   *
   * This class provides a range interface for zipping containers without
   * materializing the result. It can be used with range-based for loops
   * and integrates with STL algorithms.
   *
   * ## Memory Efficiency:
   *
   * Unlike `std::vector` of tuples approaches, this view allocates **zero**
   * additional memory beyond storing references to the original containers.
   *
   * @tparam Containers Types of containers being zipped.
   *
   * @see stl_zip() Factory function
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  class StlZipView
  {
    std::tuple<const std::decay_t<Containers> &...> containers_;

  public:
    using iterator = StlZipIterator<std::decay_t<Containers>...>;
    using const_iterator = iterator;
    using value_type = typename iterator::value_type;

    /** @brief Construct view from containers.
     *
     * @param cs References to containers to zip.
     */
    explicit constexpr StlZipView(const Containers &... cs)
      : containers_(cs...) {}

    /** @brief Get iterator to beginning.
     *
     * @return Iterator positioned at first tuple.
     */
    [[nodiscard]] constexpr iterator begin() const
    {
      return std::apply([](const auto &... cs)
                          {
                            return iterator(cs...);
                          }, containers_);
    }

    /** @brief Get iterator to end.
     *
     * @return Past-the-end iterator.
     */
    [[nodiscard]] constexpr iterator end() const
    {
      return std::apply([](const auto &... cs)
                          {
                            return iterator(cs..., typename iterator::end_tag{});
                          }, containers_);
    }

    /** @brief Check if view is empty.
     *
     * @return `true` if any container is empty.
     */
    [[nodiscard]] constexpr bool empty() const
    {
      return not begin().has_curr();
    }

    /** @brief Get number of tuples (minimum size of all containers).
     *
     * @return Number of tuples that would be produced.
     * @note This iterates through all elements - O(n) complexity.
     */
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
   * @brief Create a lazy zip view over STL containers.
   *
   * Returns a view that can be iterated with range-based for loops or
   * used with functional operations. No memory is allocated for the
   * zipped result - iteration happens inline.
   *
   * @tparam Containers Types of containers to zip.
   * @param cs Containers to zip together.
   * @return StlZipView for lazy iteration.
   *
   * @par Example:
   * @code
   * std::vector<int> v = {1, 2, 3};
   * std::list<std::string> l = {"a", "b", "c"};
   *
   * for (auto [num, str] : stl_zip(v, l))
   *   std::cout << num << ": " << str << "\n";
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] constexpr auto stl_zip(const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2, "stl_zip requires at least 2 containers");
    return StlZipView<Containers...>(cs...);
  }

  /**
   * @brief Get a zip iterator over STL containers.
   *
   * Similar to `stl_zip()` but returns an iterator directly instead of a view.
   * Useful for algorithms that need explicit iterator control.
   *
   * @param cs Containers to zip.
   * @return StlZipIterator positioned at first element.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] constexpr auto stl_zip_it(const Containers &... cs)
  {
    return StlZipIterator<std::decay_t<Containers>...>(cs...);
  }

  // ============================================================================
  // Functional Operations - Aleph Style
  // ============================================================================

  /**
   * @brief Check if predicate holds for all zipped tuples.
   *
   * Iterates over zipped containers and tests each tuple with the predicate.
   * Returns `true` only if the predicate returns `true` for every tuple.
   * Short-circuits on first `false`.
   *
   * This is similar to ML's `forall` or Haskell's `all`.
   *
   * @tparam Pred Predicate type `(tuple) -> bool`.
   * @tparam Containers Container types.
   * @param pred Predicate to test each tuple.
   * @param cs Containers to zip.
   * @return `true` if pred is true for all tuples; `false` otherwise.
   *
   * @par Example:
   * @code
   * std::vector<int> xs = {1, 2, 3};
   * std::vector<int> ys = {10, 20, 30};
   *
   * // Check if all xs are less than ys
   * bool result = stl_zip_all([](auto t) {
   *   return std::get<0>(t) < std::get<1>(t);
   * }, xs, ys);  // true
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_all(Pred && pred, const Containers &... cs)
  {
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (not std::forward<Pred>(pred)(it.get_curr()))
        return false;
    return true;
  }

  /**
   * @brief Check if predicate holds for all tuples AND containers have equal length.
   *
   * Like `stl_zip_all` but also verifies that all containers have the same size.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return `true` if pred is true for all tuples and lengths match.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_all_eq(Pred && pred, const Containers &... cs)
  {
    auto it = stl_zip_it(cs...);
    for (; it.has_curr(); it.next())
      if (not std::forward<Pred>(pred)(it.get_curr()))
        return false;
    return it.completed();
  }

  /**
   * @brief Check if predicate holds for any zipped tuple.
   *
   * Returns `true` if at least one tuple satisfies the predicate.
   * Short-circuits on first `true`.
   *
   * This is similar to ML's `exists` or Haskell's `any`.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return `true` if any tuple satisfies pred.
   *
   * @par Example:
   * @code
   * std::vector<int> xs = {1, -2, 3};
   * std::vector<int> ys = {10, 20, 30};
   *
   * // Check if any x is negative
   * bool result = stl_zip_exists([](auto t) {
   *   return std::get<0>(t) < 0;
   * }, xs, ys);  // true
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_exists(Pred && pred, const Containers &... cs)
  {
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (std::forward<Pred>(pred)(it.get_curr()))
        return true;
    return false;
  }

  /// Alias for stl_zip_exists (Python/JS style name)
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_any(Pred && pred, const Containers &... cs)
  {
    return stl_zip_exists(std::forward<Pred>(pred), cs...);
  }

  /**
   * @brief Check if no tuple satisfies the predicate.
   *
   * Returns `true` if the predicate returns `false` for all tuples.
   * This is the complement of `stl_zip_exists`.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return `true` if no tuple satisfies pred.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_none(Pred && pred, const Containers &... cs)
  {
    return not stl_zip_exists(std::forward<Pred>(pred), cs...);
  }

  /**
   * @brief Apply operation to each zipped tuple (for side effects).
   *
   * Iterates over all zipped tuples, applying the callable to each.
   * This is a side effect operation that returns void.
   *
   * @param op Callable to apply to each tuple.
   * @param cs Containers to zip.
   *
   * @par Example:
   * @code
   * std::vector<std::string> names = {"Alice", "Bob"};
   * std::vector<int> ages = {30, 25};
   *
   * stl_zip_for_each([](auto t) {
   *   std::cout << std::get<0>(t) << " is "
   *             << std::get<1>(t) << " years old\n";
   * }, names, ages);
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Op, typename... Containers>
  void stl_zip_for_each(Op && op, const Containers &... cs)
  {
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      std::forward<Op>(op)(it.get_curr());
  }

  /**
   * @brief Apply operation to each tuple with its index.
   *
   * @param op Callable `(size_t index, tuple) -> void`.
   * @param cs Containers to zip.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Op, typename... Containers>
  void stl_zip_for_each_indexed(Op && op, const Containers &... cs)
  {
    size_t idx = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next(), ++idx)
      std::forward<Op>(op)(idx, it.get_curr());
  }

  /**
   * @brief Left fold over zipped tuples.
   *
   * Accumulates a value by applying a binary operation to the accumulator
   * and each tuple, from left to right.
   *
   * This is equivalent to ML's `foldl` or JavaScript's `reduce`.
   *
   * @tparam T Accumulator type.
   * @tparam Op Binary operation `(T, tuple) -> T`.
   * @param init Initial accumulator value.
   * @param op Binary operation.
   * @param cs Containers to zip.
   * @return Final accumulated value.
   *
   * @par Example:
   * @code
   * std::vector<int> prices = {10, 20, 30};
   * std::vector<int> quantities = {2, 3, 1};
   *
   * // Total cost = sum of (price * quantity)
   * int total = stl_zip_foldl(0, [](int acc, auto t) {
   *   return acc + std::get<0>(t) * std::get<1>(t);
   * }, prices, quantities);
   * // total = 10*2 + 20*3 + 30*1 = 110
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename T, typename Op, typename... Containers>
  [[nodiscard]] T stl_zip_foldl(T init, Op && op, const Containers &... cs)
  {
    T acc = std::move(init);
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      acc = std::forward<Op>(op)(std::move(acc), it.get_curr());
    return acc;
  }

  /// Alias for stl_zip_foldl (alternative name)
  template <typename T, typename Op, typename... Containers>
  [[nodiscard]] T stl_zip_reduce(T init, Op && op, const Containers &... cs)
  {
    return stl_zip_foldl(std::move(init), std::forward<Op>(op), cs...);
  }

  /**
   * @brief Map operation over zipped tuples, returning a vector.
   *
   * Applies a transformation to each tuple and collects results in a vector.
   * The result type is automatically deduced from the transformation.
   *
   * @param op Transformation `(tuple) -> R`.
   * @param cs Containers to zip.
   * @return std::vector of transformed values.
   *
   * @par Example:
   * @code
   * std::vector<int> xs = {1, 2, 3};
   * std::vector<int> ys = {10, 20, 30};
   *
   * // Create sums
   * auto sums = stl_zip_map([](auto t) {
   *   return std::get<0>(t) + std::get<1>(t);
   * }, xs, ys);
   * // sums = {11, 22, 33}
   * @endcode
   *
   * @note This DOES allocate memory for the result vector.
   *       For lazy transformation, use stl_zip() with a transforming view.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Op, typename... Containers>
  [[nodiscard]] auto stl_zip_map(Op && op, const Containers &... cs)
  {
    using TupleType = decltype(stl_zip_it(cs...).get_curr());
    using ResultType = std::decay_t<decltype(std::forward<Op>(op)(std::declval<TupleType>()))>;

    std::vector<ResultType> result;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      result.push_back(std::forward<Op>(op)(it.get_curr()));
    return result;
  }

  /**
   * @brief Filter zipped tuples by predicate.
   *
   * Selects tuples for which the predicate returns true.
   *
   * @param pred Predicate `(tuple) -> bool`.
   * @param cs Containers to zip.
   * @return std::vector of tuples satisfying predicate.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_filter(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (auto t = it.get_curr(); std::forward<Pred>(pred)(t))
        result.push_back(t);
    return result;
  }

  /**
   * @brief Find first tuple satisfying predicate.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return std::optional containing tuple if found, empty otherwise.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_find_first(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (auto t = it.get_curr(); std::forward<Pred>(pred)(t))
        return std::optional<TupleType>(t);
    return std::optional<TupleType>{};
  }

  /**
   * @brief Count tuples satisfying predicate.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return Number of tuples satisfying predicate.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] size_t stl_zip_count(Pred && pred, const Containers &... cs)
  {
    size_t count = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (std::forward<Pred>(pred)(it.get_curr()))
        ++count;
    return count;
  }

  /**
   * @brief Count total tuples (minimum length of containers).
   *
   * @param cs Containers to zip.
   * @return Number of tuples that would be produced.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] size_t stl_zip_length(const Containers &... cs)
  {
    size_t count = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      ++count;
    return count;
  }

  /**
   * @brief Check if all containers have equal length.
   *
   * @param cs Containers to compare.
   * @return `true` if all containers have the same size.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] bool stl_zip_equal_length(const Containers &... cs)
  {
    auto it = stl_zip_it(cs...);
    while (it.has_curr())
      it.next();
    return it.completed();
  }

  /**
   * @brief Get n-th tuple from zipped containers.
   *
   * @param n Index (0-based).
   * @param cs Containers to zip.
   * @return std::optional containing tuple if index is valid, empty otherwise.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_nth(const size_t n, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    size_t i = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next(), ++i)
      if (i == n)
        return std::optional<TupleType>(it.get_curr());
    return std::optional<TupleType>{};
  }

  /**
   * @brief Take first n tuples from zipped containers.
   *
   * @param n Maximum number of tuples to take.
   * @param cs Containers to zip.
   * @return std::vector of up to n tuples.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_take(const size_t n, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    result.reserve(n);
    size_t count = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr() and count < n; it.next(), ++count)
      result.push_back(it.get_curr());
    return result;
  }

  /**
   * @brief Skip first n tuples, return the rest.
   *
   * @param n Number of tuples to skip.
   * @param cs Containers to zip.
   * @return std::vector of remaining tuples.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_drop(const size_t n, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    auto it = stl_zip_it(cs...);

    // Skip first n
    for (size_t i = 0; i < n and it.has_curr(); ++i)
      it.next();

    // Collect rest
    for (; it.has_curr(); it.next())
      result.push_back(it.get_curr());
    return result;
  }

  /**
   * @brief Partition tuples by predicate.
   *
   * Separates tuples into two groups: those satisfying the predicate
   * and those that don't.
   *
   * @param pred Predicate to partition by.
   * @param cs Containers to zip.
   * @return Pair of vectors: (matching, non-matching).
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_partition(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> matching, non_matching;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (auto t = it.get_curr(); std::forward<Pred>(pred)(t))
        matching.push_back(t);
      else
        non_matching.push_back(t);
    return std::make_pair(std::move(matching), std::move(non_matching));
  }

  /**
   * @brief Materialize zipped tuples into a vector.
   *
   * Collects all zipped tuples into a std::vector. Use this when you need
   * to iterate multiple times or pass the result to other functions.
   *
   * @param cs Containers to zip.
   * @return std::vector of tuples.
   *
   * @note This allocates memory. For single-pass iteration, use stl_zip().
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_to_vector(const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      result.push_back(it.get_curr());
    return result;
  }

  // ============================================================================
  // Traverse operations (like zip_traverse in ah-zip.H)
  // ============================================================================

  /**
   * @brief Traverse while predicate returns true.
   *
   * Iterates over zipped tuples, calling the predicate for each.
   * Stops early if predicate returns false.
   *
   * @param pred Predicate `(tuple) -> bool`.
   * @param cs Containers to zip.
   * @return `true` if pred was true for all tuples, `false` if stopped early.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_traverse(Pred && pred, const Containers &... cs)
  {
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (not std::forward<Pred>(pred)(it.get_curr()))
        return false;
    return true;
  }

  /**
   * @brief Traverse while predicate returns true, verify equal lengths.
   *
   * @param pred Predicate `(tuple) -> bool`.
   * @param cs Containers to zip.
   * @return `true` if pred was true for all AND containers have equal length.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool stl_zip_traverse_eq(Pred && pred, const Containers &... cs)
  {
    auto it = stl_zip_it(cs...);
    for (; it.has_curr(); it.next())
      if (not std::forward<Pred>(pred)(it.get_curr()))
        return false;
    return it.completed();
  }

  // ============================================================================
  // Comparison helpers
  // ============================================================================

  namespace stl_zip_detail
  {
    // Compare adjacent elements in a tuple
    template <typename Cmp, typename Tuple, size_t... Is>
    bool compare_adjacent_impl(Cmp & cmp, const Tuple & t, std::index_sequence<Is...>)
    {
      return (... and cmp(std::get<Is>(t), std::get<Is + 1>(t)));
    }
  }

  /**
   * @brief Compare elements across containers using a comparator.
   *
   * For each position, checks if all elements from different containers
   * satisfy the comparator pairwise. Useful for checking if multiple
   * containers have equal elements at each position.
   *
   * @param cmp Binary comparator `(a, b) -> bool`.
   * @param cs Containers to compare (must have same element type).
   * @return `true` if comparator holds for all adjacent pairs at all positions.
   *
   * @par Example:
   * @code
   * std::vector<int> a = {1, 2, 3};
   * std::vector<int> b = {1, 2, 3};
   * std::vector<int> c = {1, 2, 3};
   *
   * // Check if all three are element-wise equal
   * bool equal = stl_zip_cmp([](auto x, auto y) { return x == y; }, a, b, c);
   * // true
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Cmp, typename... Containers>
  [[nodiscard]] bool stl_zip_cmp(Cmp && cmp, const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2, "stl_zip_cmp requires at least 2 containers");

    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      {
        constexpr size_t N = sizeof...(Containers);
        if (auto t = it.get_curr(); not
          stl_zip_detail::compare_adjacent_impl(cmp, t, std::make_index_sequence<N - 1>{}))
          return false;
      }
    return true;
  }

  // ============================================================================
  // Enumerate - like Python's enumerate()
  // ============================================================================

  /**
   * @class StlEnumerateIterator
   * @brief Iterator that pairs each element with its index.
   *
   * Similar to Python's enumerate(). Produces tuples of (index, element).
   *
   * @tparam Container Container type.
   *
   * @see stl_enumerate() Factory function
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Container>
  class StlEnumerateIterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<size_t, typename std::decay_t<Container>::value_type>;

  private:
    typename std::decay_t<Container>::const_iterator curr_;
    typename std::decay_t<Container>::const_iterator end_;
    size_t index_ = 0;

  public:
    explicit constexpr StlEnumerateIterator(const Container & c)
      : curr_(c.begin()), end_(c.end()), index_(0) {}

    struct end_tag
    {};

    constexpr StlEnumerateIterator(const Container & c, end_tag)
      : curr_(c.end()), end_(c.end()), index_(0) {}

    [[nodiscard]] constexpr bool has_curr() const noexcept { return curr_ != end_; }

    [[nodiscard]] constexpr auto get_curr() const { return std::make_tuple(index_, *curr_); }

    constexpr void next() noexcept
    {
      ++curr_;
      ++index_;
    }

    [[nodiscard]] constexpr auto operator*() const { return get_curr(); }

    constexpr StlEnumerateIterator &operator++() noexcept
    {
      next();
      return *this;
    }

    [[nodiscard]] constexpr bool operator==(const StlEnumerateIterator & other) const noexcept
    {
      return not has_curr() and not other.has_curr();
    }

    [[nodiscard]] constexpr bool operator!=(const StlEnumerateIterator & other) const noexcept
    {
      return not (*this == other);
    }
  };

  /**
   * @class StlEnumerateView
   * @brief Lazy view that pairs each element with its index.
   *
   * @tparam Container Container type.
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Container>
  class StlEnumerateView
  {
    const std::decay_t<Container> & container_;

  public:
    using iterator = StlEnumerateIterator<std::decay_t<Container>>;

    explicit constexpr StlEnumerateView(const Container & c) : container_(c) {}

    [[nodiscard]] constexpr iterator begin() const { return iterator(container_); }

    [[nodiscard]] constexpr iterator end() const
    {
      return iterator(container_, typename iterator::end_tag{});
    }
  };

  /**
   * @brief Create an enumerate view over a container.
   *
   * Like Python's enumerate(). Produces tuples of (index, element).
   *
   * @param c Container to enumerate.
   * @return StlEnumerateView for lazy iteration.
   *
   * @par Example:
   * @code
   * std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
   *
   * for (auto [idx, name] : stl_enumerate(names))
   *   std::cout << idx << ": " << name << "\n";
   * // Output:
   * // 0: Alice
   * // 1: Bob
   * // 2: Charlie
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Container>
  [[nodiscard]] constexpr auto stl_enumerate(const Container & c)
  {
    return StlEnumerateView<Container>(c);
  }

  // ============================================================================
  // Take while / Drop while
  // ============================================================================

  /**
   * @brief Take tuples while predicate returns true.
   *
   * Collects tuples from the beginning while the predicate is satisfied.
   * Stops at the first tuple where predicate returns false.
   *
   * @param pred Predicate `(tuple) -> bool`.
   * @param cs Containers to zip.
   * @return std::vector of tuples while pred is true.
   *
   * @par Example:
   * @code
   * std::vector<int> xs = {1, 2, 3, 10, 4, 5};
   * std::vector<int> ys = {10, 20, 30, 100, 40, 50};
   *
   * // Take while x < 10
   * auto result = stl_zip_take_while([](auto t) {
   *   return std::get<0>(t) < 10;
   * }, xs, ys);
   * // result contains: {(1,10), (2,20), (3,30)}
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_take_while(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      {
        auto t = it.get_curr();
        if (not std::forward<Pred>(pred)(t))
          break;
        result.push_back(t);
      }
    return result;
  }

  /**
   * @brief Skip tuples while predicate returns true, then return the rest.
   *
   * Skips tuples from the beginning while the predicate is satisfied,
   * then collects all remaining tuples.
   *
   * @param pred Predicate `(tuple) -> bool`.
   * @param cs Containers to zip.
   * @return std::vector of tuples after pred becomes false.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_drop_while(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    auto it = stl_zip_it(cs...);

    // Skip while pred is true
    for (; it.has_curr() and std::forward<Pred>(pred)(it.get_curr()); it.next())
      /* skip */;

    // Collect the rest
    for (; it.has_curr(); it.next())
      result.push_back(it.get_curr());
    return result;
  }

  // ============================================================================
  // First / Last convenience functions
  // ============================================================================

  /**
   * @brief Get first tuple from zipped containers.
   *
   * @param cs Containers to zip.
   * @return std::optional containing first tuple, or empty if containers are empty.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_first(const Containers &... cs)
  {
    return stl_zip_nth(0, cs...);
  }

  /**
   * @brief Get last tuple from zipped containers.
   *
   * @param cs Containers to zip.
   * @return std::optional containing last tuple, or empty if containers are empty.
   *
   * @note This iterates through all elements - O(n) complexity.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_last(const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::optional<TupleType> result;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      result = it.get_curr();
    return result;
  }

  /**
   * @brief Find last tuple satisfying predicate.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return std::optional containing last matching tuple, or empty if none found.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_find_last(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::optional<TupleType> result;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      {
        auto t = it.get_curr();
        if (std::forward<Pred>(pred)(t))
          result = t;
      }
    return result;
  }

  /**
   * @brief Find index of first tuple satisfying predicate.
   *
   * @param pred Predicate to test.
   * @param cs Containers to zip.
   * @return Index of first match, or total count if not found.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] size_t stl_zip_find_index(Pred && pred, const Containers &... cs)
  {
    size_t idx = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next(), ++idx)
      if (std::forward<Pred>(pred)(it.get_curr()))
        return idx;
    return idx;
  }

  // ============================================================================
  // Unzip - inverse of zip
  // ============================================================================

  /**
   * @brief Unzip a vector of pairs into two vectors.
   *
   * Inverse operation of zip for pairs.
   *
   * @param pairs Vector of pairs to unzip.
   * @return Pair of vectors containing separated elements.
   *
   * @par Example:
   * @code
   * std::vector<std::pair<int, std::string>> pairs = {{1, "a"}, {2, "b"}};
   * auto [nums, strs] = stl_unzip(pairs);
   * // nums = {1, 2}, strs = {"a", "b"}
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename T, typename U>
  [[nodiscard]] auto stl_unzip(const std::vector<std::pair<T, U>> & pairs)
  {
    std::vector<T> firsts;
    std::vector<U> seconds;
    firsts.reserve(pairs.size());
    seconds.reserve(pairs.size());

    for (const auto & [f, s]: pairs)
      {
        firsts.push_back(f);
        seconds.push_back(s);
      }
    return std::make_pair(std::move(firsts), std::move(seconds));
  }

  /**
   * @brief Unzip a vector of tuples into a tuple of vectors.
   *
   * Inverse operation of zip for tuples.
   *
   * @param tuples Vector of tuples to unzip.
   * @return Tuple of vectors, one for each tuple element.
   *
   * @par Example:
   * @code
   * std::vector<std::tuple<int, double, char>> tuples = {
   *   {1, 1.1, 'a'}, {2, 2.2, 'b'}
   * };
   * auto [ints, doubles, chars] = stl_unzip_tuple(tuples);
   * // ints = {1, 2}, doubles = {1.1, 2.2}, chars = {'a', 'b'}
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Ts>
  [[nodiscard]] auto stl_unzip_tuple(const std::vector<std::tuple<Ts...>> & tuples)
  {
    std::tuple<std::vector<Ts>...> result;

    // Reserve space in each vector
    std::apply([&tuples](auto &... vecs)
                 {
                   (vecs.reserve(tuples.size()), ...);
                 }, result);

    // Populate vectors
    for (const auto & t: tuples)
      {
        std::apply([&t](auto &... vecs)
                     {
                       std::apply([&vecs...](const auto &... elems)
                                    {
                                      (vecs.push_back(elems), ...);
                                    }, t);
                     }, result);
      }

    return result;
  }

  // ============================================================================
  // Adjacent - zip consecutive elements of same container
  // ============================================================================

  /**
   * @brief Zip adjacent (consecutive) elements of a container.
   *
   * Creates pairs of consecutive elements: (e[0], e[1]), (e[1], e[2]), etc.
   * Useful for computing differences, detecting changes, etc.
   *
   * @param c Container with elements.
   * @return std::vector of pairs of adjacent elements.
   *
   * @par Example:
   * @code
   * std::vector<int> v = {1, 2, 3, 4, 5};
   * auto pairs = stl_adjacent(v);
   * // pairs = {(1,2), (2,3), (3,4), (4,5)}
   *
   * // Compute differences
   * for (auto [prev, curr] : stl_adjacent(v))
   *   std::cout << curr - prev << " ";  // 1 1 1 1
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Container>
  [[nodiscard]] auto stl_adjacent(const Container & c)
  {
    using T = typename std::decay_t<Container>::value_type;
    std::vector<std::pair<T, T>> result;

    auto it = c.begin();
    if (it == c.end())
      return result;

    auto prev = *it;
    ++it;

    for (; it != c.end(); ++it)
      {
        result.emplace_back(prev, *it);
        prev = *it;
      }
    return result;
  }

  /**
   * @brief Apply function to adjacent pairs of elements.
   *
   * @param op Function `(prev, curr) -> R`.
   * @param c Container.
   * @return std::vector of results.
   *
   * @par Example:
   * @code
   * std::vector<int> v = {1, 3, 6, 10};
   * auto diffs = stl_adjacent_map([](int a, int b) { return b - a; }, v);
   * // diffs = {2, 3, 4}
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Op, typename Container>
  [[nodiscard]] auto stl_adjacent_map(Op && op, const Container & c)
  {
    using T = typename std::decay_t<Container>::value_type;
    using R = std::decay_t<decltype(std::forward<Op>(op)(std::declval<T>(), std::declval<T>()))>;

    std::vector<R> result;

    auto it = c.begin();
    if (it == c.end())
      return result;

    auto prev = *it;
    ++it;

    for (; it != c.end(); ++it)
      {
        result.push_back(std::forward<Op>(op)(prev, *it));
        prev = *it;
      }
    return result;
  }

  // ============================================================================
  // Pairwise operations on single container
  // ============================================================================

  /**
   * @brief Check if predicate holds for all adjacent pairs.
   *
   * @param pred Predicate `(prev, curr) -> bool`.
   * @param c Container.
   * @return `true` if predicate holds for all consecutive pairs.
   *
   * @par Example:
   * @code
   * std::vector<int> v = {1, 2, 3, 4, 5};
   * bool sorted = stl_adjacent_all([](int a, int b) { return a < b; }, v);
   * // sorted = true (ascending order)
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename Container>
  [[nodiscard]] bool stl_adjacent_all(Pred && pred, const Container & c)
  {
    auto it = c.begin();
    if (it == c.end())
      return true;

    auto prev = *it;
    ++it;

    for (; it != c.end(); ++it)
      {
        if (not std::forward<Pred>(pred)(prev, *it))
          return false;
        prev = *it;
      }
    return true;
  }

  /**
   * @brief Check if predicate holds for any adjacent pair.
   *
   * @param pred Predicate `(prev, curr) -> bool`.
   * @param c Container.
   * @return `true` if any consecutive pair satisfies predicate.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename Container>
  [[nodiscard]] bool stl_adjacent_exists(Pred && pred, const Container & c)
  {
    auto it = c.begin();
    if (it == c.end())
      return false;

    auto prev = *it;
    ++it;

    for (; it != c.end(); ++it)
      {
        if (std::forward<Pred>(pred)(prev, *it))
          return true;
        prev = *it;
      }
    return false;
  }

  // ============================================================================
  // ML-style Additional Operations
  // ============================================================================

  /**
   * @brief Map with index (mapi in ML).
   *
   * Applies a function that receives both the index and the tuple.
   *
   * @param op Callable `(size_t idx, tuple) -> R`.
   * @param cs Containers to zip.
   * @return std::vector of transformed values.
   *
   * @par Example:
   * @code
   * auto indexed = stl_zip_mapi([](size_t i, auto t) {
   *   return std::to_string(i) + ":" + std::to_string(std::get<0>(t));
   * }, xs, ys);
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Op, typename... Containers>
  [[nodiscard]] auto stl_zip_mapi(Op && op, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;
    using ResultType = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<TupleType>()))>;

    std::vector<ResultType> result;
    size_t idx = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next(), ++idx)
      result.push_back(std::forward<Op>(op)(idx, it.get_curr()));
    return result;
  }

  /**
   * @brief Filter with index (filteri in ML).
   *
   * Filters tuples using a predicate that receives both index and tuple.
   *
   * @param pred Predicate `(size_t idx, tuple) -> bool`.
   * @param cs Containers to zip.
   * @return std::vector of tuples satisfying predicate.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Pred, typename... Containers>
  [[nodiscard]] auto stl_zip_filteri(Pred && pred, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    std::vector<TupleType> result;
    size_t idx = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next(), ++idx)
      if (auto t = it.get_curr(); std::forward<Pred>(pred)(idx, t))
        result.push_back(t);
    return result;
  }

  /**
   * @brief Scan left (scan_left in ML) - fold with intermediate results.
   *
   * Like foldl but returns all intermediate accumulator values.
   *
   * @param init Initial value.
   * @param op Binary operation `(acc, tuple) -> acc`.
   * @param cs Containers to zip.
   * @return std::vector containing init and all intermediate results.
   *
   * @par Example:
   * @code
   * // Running sum: [0, 1, 3, 6, 10]
   * auto sums = stl_zip_scan_left(0, [](int acc, auto t) {
   *   return acc + std::get<0>(t);
   * }, xs, ys);
   * @endcode
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename T, typename Op, typename... Containers>
  [[nodiscard]] auto stl_zip_scan_left(T init, Op && op, const Containers &... cs)
  {
    std::vector<T> result;
    result.push_back(init);

    T acc = std::move(init);
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      {
        acc = std::forward<Op>(op)(std::move(acc), it.get_curr());
        result.push_back(acc);
      }
    return result;
  }

  /**
   * @brief Find and map with index (find_mapi in ML).
   *
   * Finds the first element where the function returns a non-empty optional.
   *
   * @param op Callable `(size_t idx, tuple) -> std::optional<R>`.
   * @param cs Containers to zip.
   * @return First non-empty result from op, or std::nullopt.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Op, typename... Containers>
  [[nodiscard]] auto stl_zip_find_mapi(Op && op, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;
    using OptType = std::decay_t<decltype(std::forward<Op>(op)(size_t{}, std::declval<TupleType>()))>;

    size_t idx = 0;
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next(), ++idx)
      if (auto result = std::forward<Op>(op)(idx, it.get_curr()))
        return result;
    return OptType{};
  }

  /**
   * @brief Check equality of zipped sequences.
   *
   * Two zipped sequences are equal if they have the same length and
   * all corresponding tuples are equal (or satisfy the given predicate).
   *
   * @param cs Containers to zip.
   * @return `true` if sequences are equal.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] bool stl_zip_equal(const Containers &... cs)
  {
    // For a single zip, check if all containers have equal length
    auto it = stl_zip_it(cs...);
    while (it.has_curr())
      it.next();
    return it.completed();
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
  [[nodiscard]] bool stl_zip_equal_by(Eq && eq, const Containers &... cs)
  {
    auto it = stl_zip_it(cs...);
    for (; it.has_curr(); it.next())
      if (not std::forward<Eq>(eq)(it.get_curr()))
        return false;
    return it.completed();
  }

  /**
   * @brief Compare two zipped sequences lexicographically.
   *
   * Uses default comparison (operator<) for elements at corresponding positions.
   *
   * @tparam Containers1 Types of first set of containers.
   * @tparam Containers2 Types of second set of containers.
   * @param cs1 First set of containers (as tuple).
   * @param cs2 Second set of containers (as tuple).
   * @return -1 if first < second, 0 if equal, 1 if first > second.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers1, typename... Containers2>
  [[nodiscard]] int stl_zip_compare(
      const std::tuple<const Containers1 &...> & cs1,
      const std::tuple<const Containers2 &...> & cs2)
  {
    auto it1 = std::apply([](const auto &... cs) { return stl_zip_it(cs...); }, cs1);
    auto it2 = std::apply([](const auto &... cs) { return stl_zip_it(cs...); }, cs2);

    while (it1.has_curr() and it2.has_curr())
      {
        auto t1 = it1.get_curr();
        auto t2 = it2.get_curr();
        if (t1 < t2) return -1;
        if (t2 < t1) return 1;
        it1.next();
        it2.next();
      }

    if (it1.has_curr()) return 1; // first is longer
    if (it2.has_curr()) return -1; // second is longer
    return 0; // equal
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
  [[nodiscard]] bool stl_zip_mem(const Tuple & target, const Containers &... cs)
  {
    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (it.get_curr() == target)
        return true;
    return false;
  }

  /**
   * @brief Find value associated with key in zipped pairs (assoc in ML).
   *
   * Searches for a tuple where the first element equals the key.
   *
   * @param key The key to search for.
   * @param cs Containers to zip (first container contains keys).
   * @return Optional containing the full tuple if found.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename Key, typename... Containers>
  [[nodiscard]] auto stl_zip_assoc(const Key & key, const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    for (auto it = stl_zip_it(cs...); it.has_curr(); it.next())
      if (auto t = it.get_curr(); std::get<0>(t) == key)
        return std::optional<TupleType>(t);
    return std::optional<TupleType>{};
  }

  /**
   * @brief Get minimum tuple according to comparator.
   *
   * @param cs Containers to zip.
   * @return Optional containing the minimum tuple.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_min(const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    auto it = stl_zip_it(cs...);
    if (not it.has_curr())
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
   * @brief Get maximum tuple according to comparator.
   *
   * @param cs Containers to zip.
   * @return Optional containing the maximum tuple.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_max(const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    auto it = stl_zip_it(cs...);
    if (not it.has_curr())
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
  [[nodiscard]] auto stl_zip_min_max(const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;
    using ResultType = std::optional<std::pair<TupleType, TupleType>>;

    auto it = stl_zip_it(cs...);
    if (not it.has_curr())
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

  /**
   * @brief Sum all tuples element-wise (requires + operator on tuple elements).
   *
   * For numeric containers, computes the sum of each position.
   *
   * @param cs Containers to zip.
   * @return Optional tuple with sums.
   *
   * @ingroup Algorithms
   *  @author Leandro Rabindranath León
   */
  template <typename... Containers>
  [[nodiscard]] auto stl_zip_sum(const Containers &... cs)
  {
    using TupleType = typename StlZipIterator<std::decay_t<Containers>...>::value_type;

    auto it = stl_zip_it(cs...);
    if (not it.has_curr())
      return std::optional<TupleType>{};

    TupleType sum = it.get_curr();
    it.next();

    for (; it.has_curr(); it.next())
      {
        auto t = it.get_curr();
        std::apply([&t](auto &... sum_elems)
                     {
                       size_t i = 0;
                       ((sum_elems = sum_elems + std::get<decltype(i){}>(t), ++i), ...);
                     }, sum);
      }
    return std::optional<TupleType>(sum);
  }
} // end namespace Aleph

# endif // AH_STL_ZIP_H
