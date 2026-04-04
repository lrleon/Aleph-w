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


/** @file Min_Mean_Cycle.H
 *  @brief Minimum mean cycle in directed graphs (Karp algorithm).
 *
 *  This header provides a dedicated API to compute the minimum cycle mean
 *  in a directed weighted graph using Karp's dynamic programming algorithm.
 *
 *  ## Problem solved
 *
 *  Given a directed graph@f$G=(V,E)@f$with arc costs@f$w(e)@f$, find
 *  the cycle@f$C@f$minimizing the average weight:
 *
 *  @f[
 *    \mu^* = \min_C \frac{w(C)}{|C|}
 *  @f]
 *
 *  ## Highlights
 *
 *  - Works with Aleph directed graph backends (`List_Digraph`, `Array_Graph`, ...)
 *  - Supports arc filters (`SA`) and custom distance accessors
 *  - Accepts negative and positive weights (only finite arithmetic costs)
 *  - Returns minimum mean and a cycle witness when available
 *  - Includes value-only API when only the optimal mean is required
 *
 *  ## Complexity
 *
 *  - Time:@f$O(VE)@f$
 *  - Space:@f$O(V^2)@f$
 *
 *  @ingroup Graphs
 */

# ifndef MIN_MEAN_CYCLE_H
# define MIN_MEAN_CYCLE_H

# include <cmath>
# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <shortest_path_common.H>
# include <htlist.H>
# include <tpl_array.H>
# include <tpl_dynMapTree.H>

namespace Aleph
{
  /** @brief Result of minimum mean cycle computation.
   *
   *  @tparam GT Graph type.
   *  @tparam Cost_Type Arc weight type returned by the distance accessor.
   *  @ingroup Graphs
   */
  template <class GT, typename Cost_Type>
  struct Min_Mean_Cycle_Result
  {
    bool has_cycle = false; ///< True if at least one directed cycle exists.
    long double minimum_mean = std::numeric_limits<long double>::infinity();
    typename GT::Node * witness_node = nullptr; ///< Vertex used in Karp witness extraction.

    Cost_Type cycle_total_cost = Cost_Type{0}; ///< Weight sum of witness cycle.
    size_t cycle_length = 0; ///< Number of arcs in witness cycle.

    DynList<typename GT::Node *> cycle_nodes; ///< Closed witness walk (first node repeated at end).
    DynList<typename GT::Arc *> cycle_arcs; ///< Witness arcs aligned with `cycle_nodes`.
  };

  /** @brief Lightweight minimum-mean-cycle value result.
   *
   *  This result variant only returns whether a cycle exists and its
   *  minimum mean value.
   *
   *  @ingroup Graphs
   */
  struct Min_Mean_Cycle_Value_Result
  {
    bool has_cycle = false; ///< True if at least one directed cycle exists.
    long double minimum_mean = std::numeric_limits<long double>::infinity();
  };


  namespace min_mean_cycle_detail
  {
    template <class GT, typename Cost_Type>
    struct Incoming_Arc
    {
      size_t src_idx = 0;
      typename GT::Arc * arc = nullptr;
      Cost_Type weight = Cost_Type{0};
    };


    template <class GT, class Distance, class SA>
    void validate_weights(const GT & g, Distance distance, SA sa)
    {
      using Arc = typename GT::Arc;
      using Cost_Type = typename Distance::Distance_Type;

      static_assert(std::is_arithmetic_v<Cost_Type>,
                    "Karp minimum mean cycle requires arithmetic arc costs");

      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_curr_ne();
          const Cost_Type w = distance(arc);

          if constexpr (std::is_floating_point_v<Cost_Type>)
            ah_domain_error_if(not std::isfinite(w))
              << "Karp minimum mean cycle requires finite arc weights";
        }
    }


    template <typename Cost_Type>
    void validate_finite_accumulator(const Cost_Type & value, const char * context)
    {
      if constexpr (std::is_floating_point_v<Cost_Type>)
        ah_domain_error_if(not std::isfinite(value))
          << context;
    }
  } // namespace min_mean_cycle_detail


  /** @brief Compute minimum mean cycle by Karp's algorithm.
   *
   *  Behavior:
   *  - If the digraph has no directed cycle, `has_cycle = false`.
   *  - Otherwise returns `minimum_mean` and a witness cycle extracted from
   *    predecessor chains.
   *
   *  @tparam GT Directed Aleph graph type.
   *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
   *  @tparam SA Arc filter.
   *
   *  @param[in] g Graph (must be directed).
   *  @param[in] distance Arc cost accessor.
   *  @param[in] sa Arc filter.
   *
   *  @return `Min_Mean_Cycle_Result<GT, Cost_Type>`.
   *
   *  @exception domain_error If `g` is undirected.
   *  @exception domain_error If floating weights are not finite.
   *  @exception overflow_error If integer additions overflow.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Mean_Cycle_Result<GT, typename Distance::Distance_Type>
  karp_minimum_mean_cycle(const GT & g,
                          Distance distance = Distance(),
                          SA sa = SA())
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Cost_Type = typename Distance::Distance_Type;
    using Result = Min_Mean_Cycle_Result<GT, Cost_Type>;
    using Incoming = min_mean_cycle_detail::Incoming_Arc<GT, Cost_Type>;

    ah_domain_error_if(not g.is_digraph())
      << "karp_minimum_mean_cycle(): graph must be directed";

    min_mean_cycle_detail::validate_weights(g, distance, sa);

    Result result;
    const size_t n = g.get_num_nodes();
    if (n == 0)
      return result;

    ah_overflow_error_if(
        n > std::numeric_limits<size_t>::max() - 1
        or (n + 1) > std::numeric_limits<size_t>::max() / n)
      << "karp_minimum_mean_cycle(): DP table size overflow";

    Array<Node *> nodes;
    nodes.reserve(n);

    DynMapTree<Node *, size_t> node_to_idx;

    size_t idx = 0;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne(), ++idx)
      {
        Node * node = it.get_curr_ne();
        nodes.append(node);
        node_to_idx.insert(node, idx);
      }

    Array<Array<Incoming>> incoming;
    incoming.reserve(n);
    for (size_t i = 0; i < n; ++i)
      incoming.append(Array<Incoming>());

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        Arc * arc = it.get_curr_ne();
        Node * src = g.get_src_node(arc);
        Node * tgt = g.get_tgt_node(arc);

        const size_t src_idx = node_to_idx.find(src);
        const size_t tgt_idx = node_to_idx.find(tgt);

        incoming[tgt_idx].append(Incoming{src_idx, arc, distance(arc)});
      }

    const Cost_Type inf = std::numeric_limits<Cost_Type>::max();
    const size_t total_states = (n + 1) * n;

    Array<Cost_Type> dp(total_states, inf);
    Array<long> pred_idx(total_states, -1);
    Array<Arc *> pred_arc(total_states, nullptr);

    const auto state_of = [n](const size_t k, const size_t v)
    {
      return k * n + v;
    };

    for (size_t v = 0; v < n; ++v)
      dp[state_of(0, v)] = Cost_Type{0};

    for (size_t k = 1; k <= n; ++k)
      for (size_t v = 0; v < n; ++v)
        {
          Cost_Type best = inf;
          long best_pred = -1;
          Arc * best_arc = nullptr;

          for (typename Array<Incoming>::Iterator it(incoming[v]);
               it.has_curr(); it.next_ne())
            {
              const Incoming & in = it.get_curr();
              const Cost_Type prev = dp[state_of(k - 1, in.src_idx)];
              if (prev == inf)
                continue;

              const Cost_Type cand = shortest_path_detail::checked_add(prev, in.weight);
              min_mean_cycle_detail::validate_finite_accumulator(
                  cand,
                  "Karp minimum mean cycle accumulation became non-finite");

              if (best_arc == nullptr
                  or cand < best
                  or (cand == best and in.src_idx < static_cast<size_t>(best_pred)))
                {
                  best = cand;
                  best_pred = static_cast<long>(in.src_idx);
                  best_arc = in.arc;
                }
            }

          dp[state_of(k, v)] = best;
          pred_idx[state_of(k, v)] = best_pred;
          pred_arc[state_of(k, v)] = best_arc;
        }

    const long double ld_inf = std::numeric_limits<long double>::infinity();

    long double best_mean = ld_inf;
    size_t best_vertex = n;

    for (size_t v = 0; v < n; ++v)
      {
        const Cost_Type dnv = dp[state_of(n, v)];
        if (dnv == inf)
          continue;

        long double local_max = -ld_inf;
        bool has_ratio = false;

        for (size_t k = 0; k < n; ++k)
          {
            const Cost_Type dkv = dp[state_of(k, v)];
            if (dkv == inf)
              continue;

            const long double num = static_cast<long double>(dnv)
                                    - static_cast<long double>(dkv);
            const long double den = static_cast<long double>(n - k);
            const long double ratio = num / den;

            if (not has_ratio or ratio > local_max)
              {
                local_max = ratio;
                has_ratio = true;
              }
          }

        if (not has_ratio)
          continue;

        result.has_cycle = true;

        if (local_max < best_mean
            or (local_max == best_mean and v < best_vertex))
          {
            best_mean = local_max;
            best_vertex = v;
          }
      }

    if (not result.has_cycle)
      return result;

    result.minimum_mean = best_mean;
    result.witness_node = nodes[best_vertex];

    // Extract an n-step walk ending at best_vertex from predecessor table.
    Array<size_t> reverse_walk_nodes;
    reverse_walk_nodes.reserve(n + 1);
    Array<Arc *> reverse_walk_arcs;
    reverse_walk_arcs.reserve(n);

    size_t curr = best_vertex;
    reverse_walk_nodes.append(curr);

    for (size_t k = n; k > 0; --k)
      {
        const size_t st = state_of(k, curr);
        const long pred = pred_idx[st];
        Arc * arc = pred_arc[st];

        if (pred < 0 or arc == nullptr)
          break;

        reverse_walk_arcs.append(arc);
        curr = static_cast<size_t>(pred);
        reverse_walk_nodes.append(curr);
      }

    if (reverse_walk_nodes.size() < 2)
      return result;

    Array<size_t> walk_nodes;
    walk_nodes.reserve(reverse_walk_nodes.size());
    for (size_t i = reverse_walk_nodes.size(); i > 0; --i)
      walk_nodes.append(reverse_walk_nodes[i - 1]);

    Array<Arc *> walk_arcs;
    walk_arcs.reserve(reverse_walk_arcs.size());
    for (size_t i = reverse_walk_arcs.size(); i > 0; --i)
      walk_arcs.append(reverse_walk_arcs[i - 1]);

    const size_t invalid_pos = std::numeric_limits<size_t>::max();
    Array<size_t> first_pos(n, invalid_pos);

    bool found_cycle = false;
    size_t best_start = 0;
    size_t best_end = 0;
    Cost_Type best_cycle_cost = Cost_Type{0};
    long double best_cycle_mean = ld_inf;

    for (size_t i = 0; i < walk_nodes.size(); ++i)
      {
        const size_t node_idx = walk_nodes[i];
        ah_runtime_error_if(node_idx >= n)
          << "karp_minimum_mean_cycle(): invalid witness node index";

        const size_t j = first_pos[node_idx];
        if (j == invalid_pos)
          {
            first_pos[node_idx] = i;
            continue;
          }

        const size_t len = i - j;
        if (len == 0)
          continue;

        Cost_Type cycle_cost = Cost_Type{0};
        for (size_t p = j; p < i; ++p)
          {
            cycle_cost = shortest_path_detail::checked_add(
                cycle_cost, distance(walk_arcs[p]));
            min_mean_cycle_detail::validate_finite_accumulator(
                cycle_cost,
                "Karp minimum mean cycle witness accumulation became non-finite");
          }

        const long double cycle_mean = static_cast<long double>(cycle_cost)
                                       / static_cast<long double>(len);

        if (not found_cycle or cycle_mean < best_cycle_mean)
          {
            found_cycle = true;
            best_cycle_mean = cycle_mean;
            best_cycle_cost = cycle_cost;
            best_start = j;
            best_end = i;
          }
      }

    if (not found_cycle)
      return result;

    result.cycle_total_cost = best_cycle_cost;
    result.cycle_length = best_end - best_start;

    for (size_t p = best_start; p <= best_end; ++p)
      result.cycle_nodes.append(nodes[walk_nodes[p]]);

    for (size_t p = best_start; p < best_end; ++p)
      result.cycle_arcs.append(walk_arcs[p]);

    return result;
  }

  /** @brief Compute only the minimum mean value (without witness walk).
   *
   *  This variant avoids predecessor storage and witness reconstruction.
   *  It is useful when only the optimal mean value is required.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Mean_Cycle_Value_Result
  karp_minimum_mean_cycle_value(const GT & g,
                                Distance distance = Distance(),
                                SA sa = SA())
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Cost_Type = typename Distance::Distance_Type;
    using Incoming = min_mean_cycle_detail::Incoming_Arc<GT, Cost_Type>;

    ah_domain_error_if(not g.is_digraph())
      << "karp_minimum_mean_cycle_value(): graph must be directed";

    min_mean_cycle_detail::validate_weights(g, distance, sa);

    Min_Mean_Cycle_Value_Result result;
    const size_t n = g.get_num_nodes();
    if (n == 0)
      return result;

    ah_overflow_error_if(
        n > std::numeric_limits<size_t>::max() - 1
        or (n + 1) > std::numeric_limits<size_t>::max() / n)
      << "karp_minimum_mean_cycle_value(): DP table size overflow";

    DynMapTree<Node *, size_t> node_to_idx;
    size_t idx = 0;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne(), ++idx)
      {
        node_to_idx.insert(it.get_curr_ne(), idx);
      }

    Array<Array<Incoming>> incoming;
    incoming.reserve(n);
    for (size_t i = 0; i < n; ++i)
      incoming.append(Array<Incoming>());

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        Arc * arc = it.get_curr_ne();
        Node * src = g.get_src_node(arc);
        Node * tgt = g.get_tgt_node(arc);
        const size_t src_idx = node_to_idx.find(src);
        const size_t tgt_idx = node_to_idx.find(tgt);
        incoming[tgt_idx].append(Incoming{src_idx, arc, distance(arc)});
      }

    const Cost_Type inf = std::numeric_limits<Cost_Type>::max();
    const size_t total_states = (n + 1) * n;
    Array<Cost_Type> dp(total_states, inf);

    const auto state_of = [n](const size_t k, const size_t v)
    {
      return k * n + v;
    };

    for (size_t v = 0; v < n; ++v)
      dp[state_of(0, v)] = Cost_Type{0};

    for (size_t k = 1; k <= n; ++k)
      for (size_t v = 0; v < n; ++v)
        {
          Cost_Type best = inf;
          bool has_best = false;

          for (typename Array<Incoming>::Iterator it(incoming[v]);
               it.has_curr(); it.next_ne())
            {
              const Incoming & in = it.get_curr();
              const Cost_Type prev = dp[state_of(k - 1, in.src_idx)];
              if (prev == inf)
                continue;

              const Cost_Type cand = shortest_path_detail::checked_add(prev, in.weight);
              min_mean_cycle_detail::validate_finite_accumulator(
                  cand,
                  "Karp minimum mean cycle accumulation became non-finite");
              if (not has_best or cand < best)
                {
                  has_best = true;
                  best = cand;
                }
            }

          dp[state_of(k, v)] = has_best ? best : inf;
        }

    const long double ld_inf = std::numeric_limits<long double>::infinity();
    long double best_mean = ld_inf;

    for (size_t v = 0; v < n; ++v)
      {
        const Cost_Type dnv = dp[state_of(n, v)];
        if (dnv == inf)
          continue;

        long double local_max = -ld_inf;
        bool has_ratio = false;
        for (size_t k = 0; k < n; ++k)
          {
            const Cost_Type dkv = dp[state_of(k, v)];
            if (dkv == inf)
              continue;

            const long double num = static_cast<long double>(dnv)
                                    - static_cast<long double>(dkv);
            const long double den = static_cast<long double>(n - k);
            const long double ratio = num / den;

            if (not has_ratio or ratio > local_max)
              {
                has_ratio = true;
                local_max = ratio;
              }
          }

        if (not has_ratio)
          continue;

        result.has_cycle = true;
        if (local_max < best_mean)
          best_mean = local_max;
      }

    if (result.has_cycle)
      result.minimum_mean = best_mean;

    return result;
  }

  /** @brief Alias for `karp_minimum_mean_cycle_value()`.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Mean_Cycle_Value_Result
  minimum_mean_cycle_value(const GT & g,
                           Distance distance = Distance(),
                           SA sa = SA())
  {
    return karp_minimum_mean_cycle_value<GT, Distance, SA>(
        g, std::move(distance), std::move(sa));
  }


  /** @brief Alias for `karp_minimum_mean_cycle()`.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Min_Mean_Cycle_Result<GT, typename Distance::Distance_Type>
  minimum_mean_cycle(const GT & g,
                     Distance distance = Distance(),
                     SA sa = SA())
  {
    return karp_minimum_mean_cycle<GT, Distance, SA>(
        g, std::move(distance), std::move(sa));
  }


  /** @brief Functor wrapper for Karp minimum mean cycle.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Karp_Minimum_Mean_Cycle
  {
    Distance distance_;
    SA sa_;

  public:
    Karp_Minimum_Mean_Cycle(Distance distance = Distance(),
                            SA sa = SA())
      : distance_(std::move(distance)),
        sa_(std::move(sa))
    {
      // empty
    }

    Min_Mean_Cycle_Result<GT, typename Distance::Distance_Type>
    operator()(const GT & g) const
    {
      return karp_minimum_mean_cycle<GT, Distance, SA>(g, distance_, sa_);
    }
  };

  /** @brief Functor wrapper for value-only minimum mean cycle.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Karp_Minimum_Mean_Cycle_Value
  {
    Distance distance_;
    SA sa_;

  public:
    Karp_Minimum_Mean_Cycle_Value(Distance distance = Distance(),
                                  SA sa = SA())
      : distance_(std::move(distance)),
        sa_(std::move(sa))
    {
      // empty
    }

    Min_Mean_Cycle_Value_Result
    operator()(const GT & g) const
    {
      return karp_minimum_mean_cycle_value<GT, Distance, SA>(g, distance_, sa_);
    }
  };
} // namespace Aleph

# endif // MIN_MEAN_CYCLE_H
