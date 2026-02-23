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


/** @file Min_Cost_Matching.H
 *  @brief Minimum-cost matching in general undirected graphs.
 *
 *  This header exposes a dedicated API for minimum-cost matching on
 *  non-bipartite graphs over Aleph graph backends.
 *
 *  ## Problem solved
 *
 *  Given an undirected graph @f$G=(V,E)@f$ where each edge @f$e \in E@f$
 *  has a cost @f$c(e)@f$, find a matching @f$M@f$ that minimizes
 *  @f$\sum_{e \in M} c(e)@f$.
 *
 *  Optional mode `max_cardinality = true` changes the objective to:
 *  1) maximize matching cardinality, and
 *  2) among those, minimize total cost.
 *
 *  It also provides a dedicated perfect-matching variant that reports
 *  feasibility and, when feasible, the minimum perfect-matching cost.
 *
 *  ## Highlights
 *
 *  - Works on `List_Graph`, `List_SGraph`, and `Array_Graph`
 *  - Supports Aleph arc filters
 *  - Uses integral costs (converted to `long long` with checks)
 *  - Dedicated API names for minimum-cost intent
 *
 *  ## Complexity
 *
 *  - Time: @f$O(V^3)@f$
 *  - Space: @f$O(V + E)@f$
 *
 *  @see Blossom.H For maximum-cardinality matching in general graphs
 *  @see Blossom_Weighted.H For maximum-weight matching
 *  @ingroup Graphs
 */

# ifndef MIN_COST_MATCHING_H
# define MIN_COST_MATCHING_H

# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <Blossom_Weighted.H>

namespace Aleph
{
  /** @brief Result of minimum-cost matching.
   *
   *  Stores the total cost and cardinality of the computed matching.
   *
   *  @tparam Cost_Type Numeric type for total cost (default `long long`).
   *  @ingroup Graphs
   */
  template <typename Cost_Type = long long>
  struct Min_Cost_Matching_Result
  {
    Cost_Type total_cost = Cost_Type{0}; ///< Sum of matched arc costs.
    size_t cardinality = 0; ///< Number of arcs in the matching.
  };


  /** @brief Result of minimum-cost perfect matching.
   *
   *  If `feasible` is true, `matching` contains a perfect matching and
   *  `cardinality == |V|/2` (for even |V|). If false, no perfect matching
   *  exists and the output matching is empty.
   *
   *  @tparam Cost_Type Numeric type for total cost (default `long long`).
   *  @ingroup Graphs
   */
  template <typename Cost_Type = long long>
  struct Min_Cost_Perfect_Matching_Result
  {
    bool feasible = false; ///< Whether a perfect matching exists.
    Cost_Type total_cost = Cost_Type{0}; ///< Total cost if feasible.
    size_t cardinality = 0; ///< Cardinality (`|V|/2` if feasible).
  };


  namespace min_cost_matching_detail
  {
    /** @internal
     *  @brief Convert integral values to `long long` with overflow checks.
     *
     *  Checks are skipped with `constexpr` logic when T's range is known at
     *  compile time to fit within `long long` (e.g. `int`, `unsigned int`).
     *  When a check is needed we compare via `__int128` to avoid the
     *  narrowing/UB that arises from casting LLONG_MAX/MIN into a narrower T.
     */
    template <typename T>
    long long to_ll_checked(T value)
    {
      static_assert(std::is_integral_v<T>,
                    "Min_Cost_Matching requires integral arc costs");

      // Number of value bits in T and in long long (excludes the sign bit).
      constexpr int T_digits  = std::numeric_limits<T>::digits;
      constexpr int LL_digits = std::numeric_limits<long long>::digits; // 63

      // Upper check: T can hold values > LLONG_MAX when
      //   unsigned T: T_digits >= LL_digits  (e.g. unsigned long long: 64 >= 63)
      //   signed   T: T_digits >  LL_digits  (e.g. __int128: 127 > 63)
      constexpr bool need_upper =
          std::is_unsigned_v<T> ? (T_digits >= LL_digits)
                                : (T_digits >  LL_digits);

      // Lower check: only signed T wider than long long can go below LLONG_MIN.
      constexpr bool need_lower =
          std::is_signed_v<T> and (T_digits > LL_digits);

      if constexpr (need_upper)
        {
          ah_overflow_error_if(static_cast<__int128>(value)
                               > static_cast<__int128>(std::numeric_limits<long long>::max()))
            << "Cost cannot be represented as long long";
        }

      if constexpr (need_lower)
        {
          ah_overflow_error_if(static_cast<__int128>(value)
                               < static_cast<__int128>(std::numeric_limits<long long>::min()))
            << "Cost cannot be represented as long long";
        }

      return static_cast<long long>(value);
    }


    /** @internal
     *  @brief Adapter that negates costs to reuse maximum-weight blossom.
     */
    template <class Cost_Accessor>
    class Negated_Cost_Accessor
    {
      Cost_Accessor cost_;

    public:
      explicit Negated_Cost_Accessor(Cost_Accessor cost = Cost_Accessor())
        : cost_(std::move(cost))
      {
        // empty
      }

      template <class Arc>
      long long operator()(Arc *arc) const
      {
        const long long c = to_ll_checked(cost_(arc));
        ah_overflow_error_if(c == std::numeric_limits<long long>::min())
          << "Minimum-cost matching cannot negate LLONG_MIN cost";
        return -c;
      }
    };
  } // namespace min_cost_matching_detail


  /** @brief Compute minimum-cost matching in a general undirected graph.
   *
   *  Computes a matching that minimizes total cost. If `max_cardinality` is
   *  true, it first maximizes cardinality and then minimizes cost.
   *
   *  @tparam GT Aleph undirected graph type.
   *  @tparam Cost Cost accessor functor (must return integral type).
   *  @tparam SA Arc filter.
   *
   *  @param[in] g Graph (must be undirected).
   *  @param[out] matching Output list with matching arcs.
   *  @param[in] cost Cost accessor (defaults to `Dft_Dist<GT>`).
   *  @param[in] sa Arc filter.
   *  @param[in] max_cardinality If true, optimize lexicographically:
   *             maximum cardinality first, then minimum cost.
   *
   *  @return A `Min_Cost_Matching_Result<long long>` with total cost and
   *          cardinality.
   *
   *  @exception domain_error If `g` is a digraph.
   *  @exception overflow_error If costs cannot be represented or summed in
   *                            `long long`.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Cost = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Cost_Matching_Result<long long>
  compute_minimum_cost_general_matching(const GT & g,
                                        DynDlist<typename GT::Arc *> & matching,
                                        Cost cost = Cost(),
                                        SA sa = SA(),
                                        const bool max_cardinality = false)
  {
    ah_domain_error_if(g.is_digraph())
      << "compute_minimum_cost_general_matching(): g is a digraph";

    using Arc = typename GT::Arc;
    using Raw_Cost = std::decay_t<decltype(cost(static_cast<Arc *>(nullptr)))>;

    static_assert(std::is_integral_v<Raw_Cost>,
                  "Min_Cost_Matching requires integral arc costs");

    Cost cost_accessor = std::move(cost);

    const auto weighted_result =
        compute_maximum_weight_general_matching<
          GT,
          min_cost_matching_detail::Negated_Cost_Accessor<Cost>,
          SA>(g,
              matching,
              min_cost_matching_detail::Negated_Cost_Accessor<Cost>(cost_accessor),
              std::move(sa),
              max_cardinality);

    long long total_cost = 0;
    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        Arc *arc = it.get_curr();
        const long long c = min_cost_matching_detail::to_ll_checked(cost_accessor(arc));
        const __int128 sum = static_cast<__int128>(total_cost)
                             + static_cast<__int128>(c);
        ah_overflow_error_if(sum > static_cast<__int128>(std::numeric_limits<long long>::max())
                             or sum < static_cast<__int128>(std::numeric_limits<long long>::min()))
          << "Minimum-cost matching total cost overflows long long";
        total_cost = static_cast<long long>(sum);
      }

    ah_runtime_error_unless(weighted_result.cardinality == matching.size())
      << "Minimum-cost matching internal cardinality mismatch";

    return Min_Cost_Matching_Result<long long>{total_cost, weighted_result.cardinality};
  }


  /** @brief Alias for compute_minimum_cost_general_matching().
   *
   *  Keeps the explicit blossom naming at call sites.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Cost = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Cost_Matching_Result<long long>
  blossom_minimum_cost_matching(const GT & g,
                                DynDlist<typename GT::Arc *> & matching,
                                Cost cost = Cost(),
                                SA sa = SA(),
                                const bool max_cardinality = false)
  {
    return compute_minimum_cost_general_matching<GT, Cost, SA>(
                                                               g, matching, std::move(cost), std::move(sa),
                                                               max_cardinality);
  }


  /** @brief Compute minimum-cost perfect matching in a general undirected graph.
   *
   *  This function searches for a perfect matching (all vertices matched)
   *  and minimizes total cost among perfect matchings.
   *
   *  Behavior:
   *  - If a perfect matching exists, returns `feasible = true` and fills
   *    `matching` with a minimum-cost perfect matching.
   *  - If it does not exist, returns `feasible = false` and leaves
   *    `matching` empty.
   *
   *  @tparam GT Aleph undirected graph type.
   *  @tparam Cost Cost accessor functor (must return integral type).
   *  @tparam SA Arc filter.
   *
   *  @param[in] g Graph (must be undirected).
   *  @param[out] matching Output list with matching arcs if feasible.
   *  @param[in] cost Cost accessor (defaults to `Dft_Dist<GT>`).
   *  @param[in] sa Arc filter.
   *
   *  @return A `Min_Cost_Perfect_Matching_Result<long long>`.
   *
   *  @exception domain_error If `g` is a digraph.
   *  @exception overflow_error If costs cannot be represented or summed in
   *                            `long long`.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Cost = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Cost_Perfect_Matching_Result<long long>
  compute_minimum_cost_perfect_general_matching(const GT & g,
                                                DynDlist<typename GT::Arc *> & matching,
                                                Cost cost = Cost(),
                                                SA sa = SA())
  {
    ah_domain_error_if(g.is_digraph())
      << "compute_minimum_cost_perfect_general_matching(): g is a digraph";

    matching.empty();

    const size_t num_nodes = g.get_num_nodes();
    if (num_nodes == 0)
      return Min_Cost_Perfect_Matching_Result<long long>{true, 0, 0};

    if (num_nodes % 2 == 1)
      return Min_Cost_Perfect_Matching_Result<long long>{false, 0, 0};

    const auto best = compute_minimum_cost_general_matching<GT, Cost, SA>(
                                                                          g, matching, std::move(cost), std::move(sa),
                                                                          true);

    const size_t expected = num_nodes / 2;
    if (best.cardinality != expected)
      {
        matching.empty();
        return Min_Cost_Perfect_Matching_Result<long long>{false, 0, 0};
      }

    return Min_Cost_Perfect_Matching_Result<long long>{
          true, best.total_cost, best.cardinality
        };
  }


  /** @brief Alias for compute_minimum_cost_perfect_general_matching().
   *
   *  Keeps the explicit blossom naming at call sites.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Cost = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Cost_Perfect_Matching_Result<long long>
  blossom_minimum_cost_perfect_matching(const GT & g,
                                        DynDlist<typename GT::Arc *> & matching,
                                        Cost cost = Cost(),
                                        SA sa = SA())
  {
    return compute_minimum_cost_perfect_general_matching<GT, Cost, SA>(
                                                                       g, matching, std::move(cost), std::move(sa));
  }


  /** @brief Functor wrapper for minimum-cost general matching.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Cost = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Compute_Minimum_Cost_General_Matching
  {
    Cost cost_;
    SA sa_;
    bool max_cardinality_ = false;

  public:
    Compute_Minimum_Cost_General_Matching(Cost cost = Cost(),
                                          SA sa = SA(),
                                          const bool max_cardinality = false)
      : cost_(std::move(cost)),
        sa_(std::move(sa)),
        max_cardinality_(max_cardinality)
    {
      // empty
    }

    Min_Cost_Matching_Result<long long>
    operator()(const GT & g, DynDlist<typename GT::Arc *> & matching)
    {
      return compute_minimum_cost_general_matching<GT, Cost, SA>(
                                                                 g, matching, cost_, sa_, max_cardinality_);
    }
  };


  /** @brief Functor wrapper for minimum-cost perfect general matching.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Cost = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Compute_Minimum_Cost_Perfect_General_Matching
  {
    Cost cost_;
    SA sa_;

  public:
    Compute_Minimum_Cost_Perfect_General_Matching(Cost cost = Cost(), SA sa = SA())
      : cost_(std::move(cost)),
        sa_(std::move(sa))
    {
      // empty
    }

    Min_Cost_Perfect_Matching_Result<long long>
    operator()(const GT & g, DynDlist<typename GT::Arc *> & matching)
    {
      return compute_minimum_cost_perfect_general_matching<GT, Cost, SA>(g, matching, cost_, sa_);
    }
  };
} // namespace Aleph

# endif // MIN_COST_MATCHING_H
