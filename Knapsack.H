/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file Knapsack.H
 *  @brief Classical knapsack problem variants (0/1, unbounded, bounded).
 *
 *  Provides dynamic programming solutions for:
 *  - 0/1 Knapsack: each item used at most once
 *  - Unbounded Knapsack: unlimited copies of each item
 *  - Bounded Knapsack: limited copies via binary decomposition
 *
 *  All functions support reconstruction of selected items.
 *
 *  @example knapsack_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef KNAPSACK_H
# define KNAPSACK_H

# include <algorithm>
# include <concepts>
# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief An item for knapsack problems. */
  template <typename W, typename V>
  struct Knapsack_Item
  {
    W weight; /**< Weight (or cost) of the item */
    V value; /**< Value (or profit) of the item */
  };


  /** @brief Result of a knapsack computation. */
  template <typename V>
  struct Knapsack_Result
  {
    V optimal_value = V{}; /**< Optimal objective value */
    Array<size_t> selected_items; /**< Indices (0-based) of selected items */
  };


  namespace knapsack_detail
  {
    template <std::integral W>
    [[nodiscard]] inline size_t
    to_size_checked(const W value, const char *fn_name,
                    const char *field_name)
    {
      if constexpr (std::is_signed_v<W>)
        ah_domain_error_if(value < W{})
          << fn_name << ": " << field_name << " must be non-negative";

      using UW = std::make_unsigned_t<W>;
      const UW uvalue = static_cast<UW>(value);
      if constexpr (sizeof(UW) > sizeof(size_t))
        ah_out_of_range_error_if(uvalue >
                               static_cast<UW>(
                                 std::numeric_limits<size_t>::max()))
          << fn_name << ": " << field_name << " is too large for size_t";

      return static_cast<size_t>(uvalue);
    }

    template <std::integral W, typename V>
    [[nodiscard]] inline Array<size_t>
    extract_weights_checked(const Array<Knapsack_Item<W, V>> & items,
                            const char *fn_name)
    {
      Array<size_t> weights = Array<size_t>::create(items.size());
      for (size_t i = 0; i < items.size(); ++i)
        weights(i) = to_size_checked(items[i].weight, fn_name, "item weight");
      return weights;
    }
  } // namespace knapsack_detail


  /** @brief 0/1 Knapsack with item reconstruction.
   *
   *  Each item can be taken at most once.
   *
   *  @tparam V  Value type (must support +, comparison).
   *  @tparam W  Weight type (integral, must support <=).
   *
   *  @param[in] items     Array of items.
   *  @param[in] capacity  Maximum weight capacity.
   *  @return Knapsack_Result with optimal value and selected item indices.
   *
   *  @throws ah_domain_error if capacity or any item weight is negative.
   *
   *  @note Complexity: O(n * C) time, O(n * C) space.
   */
  template <typename V, std::integral W>
  [[nodiscard]] Knapsack_Result<V>
  knapsack_01(const Array<Knapsack_Item<W, V>> & items, W capacity)
  {
    const size_t n = items.size();
    const size_t C
        = knapsack_detail::to_size_checked(capacity, "knapsack_01", "capacity");
    const Array<size_t> weights
        = knapsack_detail::extract_weights_checked(items, "knapsack_01");

    if (n == 0)
      return Knapsack_Result<V>{V{}, Array<size_t>()};

    // dp[i][w] = best value using items 0..i-1 with capacity w
    Array<Array<V>> dp;
    dp.reserve(n + 1);
    for (size_t i = 0; i <= n; ++i)
      {
        Array<V> row = Array<V>::create(C + 1);
        for (size_t w = 0; w <= C; ++w)
          row(w) = V{};
        dp.append(std::move(row));
      }

    for (size_t i = 1; i <= n; ++i)
      {
        const size_t wi = weights[i - 1];
        const V vi = items[i - 1].value;
        for (size_t w = 0; w <= C; ++w)
          if (wi <= w and dp[i - 1][w - wi] + vi > dp[i - 1][w])
            dp[i][w] = dp[i - 1][w - wi] + vi;
          else
            dp[i][w] = dp[i - 1][w];
      }

    // reconstruct
    Array<size_t> sel;
    size_t w = C;
    for (size_t i = n; i > 0; --i)
      if (dp[i][w] != dp[i - 1][w])
        {
          sel.append(i - 1);
          w -= weights[i - 1];
        }

    // reverse to ascending order
    Array<size_t> selected;
    selected.reserve(sel.size());
    for (size_t k = sel.size(); k > 0; --k)
      selected.append(sel[k - 1]);

    return Knapsack_Result<V>{dp[n][C], std::move(selected)};
  }


  /** @brief 0/1 Knapsack — value only (space-optimized).
   *
   *  @tparam V  Value type.
   *  @tparam W  Weight type.
   *
   *  @param[in] items     Array of items.
   *  @param[in] capacity  Maximum weight capacity.
   *  @return Optimal value.
   *
   *  @throws ah_domain_error if capacity or any item weight is negative.
   *
   *  @note Complexity: O(n * C) time, O(C) space.
   */
  template <typename V, std::integral W>
  [[nodiscard]] V
  knapsack_01_value(const Array<Knapsack_Item<W, V>> & items, W capacity)
  {
    const size_t n = items.size();
    const size_t C = knapsack_detail::to_size_checked(
                                                      capacity, "knapsack_01_value", "capacity");
    const Array<size_t> weights = knapsack_detail::extract_weights_checked(
                                                                           items, "knapsack_01_value");

    if (n == 0)
      return V{};

    Array<V> dp = Array<V>::create(C + 1);
    for (size_t w = 0; w <= C; ++w)
      dp(w) = V{};

    for (size_t i = 0; i < n; ++i)
      {
        const size_t wi = weights[i];
        const V vi = items[i].value;
        for (size_t w = C; w >= wi and w != static_cast<size_t>(-1); --w)
          if (dp[w - wi] + vi > dp[w])
            dp(w) = dp[w - wi] + vi;
      }

    return dp[C];
  }


  /** @brief Unbounded Knapsack with reconstruction.
   *
   *  Each item can be taken unlimited times.
   *
   *  @tparam V  Value type.
   *  @tparam W  Weight type.
   *
   *  @param[in] items     Array of items.
   *  @param[in] capacity  Maximum weight capacity.
   *  @return Knapsack_Result with optimal value and selected item indices
   *          (may contain repeats).
   *
   *  @throws ah_domain_error if capacity or any item weight is negative.
   *  @throws ah_domain_error if a zero-weight item has positive value
   *          (objective becomes unbounded).
   *
   *  @note Complexity: O(n * C) time, O(C) space.
   */
  template <typename V, std::integral W>
  [[nodiscard]] Knapsack_Result<V>
  knapsack_unbounded(const Array<Knapsack_Item<W, V>> & items, W capacity)
  {
    const size_t n = items.size();
    const size_t C = knapsack_detail::to_size_checked(
                                                      capacity, "knapsack_unbounded", "capacity");
    const Array<size_t> weights = knapsack_detail::extract_weights_checked(
                                                                           items, "knapsack_unbounded");

    if (n == 0)
      return Knapsack_Result<V>{V{}, Array<size_t>()};

    for (size_t i = 0; i < n; ++i)
      if (weights[i] == 0 and items[i].value > V{})
        ah_domain_error_if(true)
          << "knapsack_unbounded: zero-weight item with positive value "
          << "leads to unbounded optimum";

    Array<V> dp = Array<V>::create(C + 1);
    // choice[w] = which item was last added at capacity w
    Array<size_t> choice = Array<size_t>::create(C + 1);
    const size_t NONE = std::numeric_limits<size_t>::max();
    for (size_t w = 0; w <= C; ++w)
      {
        dp(w) = V{};
        choice(w) = NONE;
      }

    for (size_t w = 1; w <= C; ++w)
      for (size_t i = 0; i < n; ++i)
        {
          const size_t wi = weights[i];
          if (wi <= w and dp[w - wi] + items[i].value > dp[w])
            {
              dp(w) = dp[w - wi] + items[i].value;
              choice(w) = i;
            }
        }

    // reconstruct
    Array<size_t> sel;
    size_t w = C;
    while (w > 0 and choice[w] != NONE)
      {
        const size_t idx = choice[w];
        sel.append(idx);
        w -= weights[idx];
      }

    return Knapsack_Result<V>{dp[C], std::move(sel)};
  }


  /** @brief Bounded Knapsack with reconstruction.
   *
   *  Each item i can be taken at most counts[i] times.
   *  Internally uses binary decomposition to reduce to 0/1 knapsack.
   *
   *  @tparam V  Value type.
   *  @tparam W  Weight type.
   *
   *  @param[in] items     Array of items.
   *  @param[in] counts    Maximum copies for each item.
   *  @param[in] capacity  Maximum weight capacity.
   *  @return Knapsack_Result with optimal value and selected item indices
   *          (original indices, may contain repeats).
   *
   *  @throws ah_domain_error if capacity or any item weight is negative.
   *  @throws ah_invalid_argument_error if items.size() != counts.size().
   *
   *  @note Complexity: O(C * sum(log(counts[i]))) time.
   */
  template <typename V, std::integral W>
  [[nodiscard]] Knapsack_Result<V>
  knapsack_bounded(const Array<Knapsack_Item<W, V>> & items,
                   const Array<size_t> & counts, W capacity)
  {
    ah_invalid_argument_if(items.size() != counts.size())
      << "knapsack_bounded: items and counts must have same size";

    const size_t n = items.size();
    const size_t C = knapsack_detail::to_size_checked(
                                                      capacity, "knapsack_bounded", "capacity");
    const Array<size_t> weights = knapsack_detail::extract_weights_checked(
                                                                           items, "knapsack_bounded");

    if (n == 0)
      return Knapsack_Result<V>{V{}, Array<size_t>()};

    // Binary decomposition: for item i with count c, create groups
    // of 1, 2, 4, ..., 2^k, remainder
    Array<Knapsack_Item<W, V>> expanded;
    Array<size_t> origin; // maps expanded index -> original index
    Array<size_t> multiplier; // how many copies this group represents

    for (size_t i = 0; i < n; ++i)
      {
        const size_t wi = weights[i];
        size_t rem = counts[i];
        size_t k = 1;
        while (rem > 0)
          {
            const size_t take = std::min(k, rem);

            if (wi == 0 or take <= C / wi)
              {
                expanded.append(Knapsack_Item<W, V>{
                                  static_cast<W>(wi * take),
                                  static_cast<V>(take) * items[i].value
                                });
                origin.append(i);
                multiplier.append(take);
              }

            rem -= take;
            if (rem == 0)
              break;

            if (k > std::numeric_limits<size_t>::max() / 2)
              k = rem;
            else
              k *= 2;
          }
      }

    // Solve as 0/1
    auto result = knapsack_01(expanded, capacity);

    // Map back to original indices
    Array<size_t> sel;
    for (size_t k = 0; k < result.selected_items.size(); ++k)
      {
        const size_t ei = result.selected_items[k];
        const size_t orig = origin[ei];
        const size_t mult = multiplier[ei];
        for (size_t j = 0; j < mult; ++j)
          sel.append(orig);
      }

    return Knapsack_Result<V>{result.optimal_value, std::move(sel)};
  }
} // namespace Aleph

# endif // KNAPSACK_H
