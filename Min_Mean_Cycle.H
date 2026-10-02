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
 *  ## Arithmetic
 *
 *  The table adds up to @f$V@f$ weights per entry. Integer costs of at most
 *  64 bits never overflow it: the sums are `long long` when
 *  @f$V \cdot \max|w|@f$ stays below `LLONG_MAX`, and 128-bit integers
 *  otherwise (GCC and Clang, `ALEPH_KARP_INT128`); only the cost of the
 *  reported witness has to fit the cost type. Without 128-bit integers
 *  (MSVC), and for `unsigned long long` or 128-bit costs, the sums are
 *  checked and may overflow. Floating-point sums must stay finite. The
 *  means are compared in `long double`.
 *
 *  ## Complexity
 *
 *  - Time:@f$O(VE)@f$
 *  - Space:@f$O(V^2)@f$ (twice as many bytes for the table when its sums
 *    need 128 bits)
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef MIN_MEAN_CYCLE_H
# define MIN_MEAN_CYCLE_H

# include <ah-graph-concepts.H>

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

/** Whether Karp's table can use 128-bit integers when the sums of a
    `long long` table could overflow: GCC and Clang (`__SIZEOF_INT128__`),
    except clang-cl, as `ALEPH_HOWARD_INT128` in `Howard_Min_Mean_Cycle.H`. */
# if defined(__SIZEOF_INT128__) and not defined(_MSC_VER)
#   define ALEPH_KARP_INT128 1
# else
#   define ALEPH_KARP_INT128 0
# endif

namespace Aleph
{
  /** @brief Result of minimum mean cycle computation.
   *
   *  @tparam GT Graph type.
   *  @tparam Cost_Type Arc weight type returned by the distance accessor.
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename Cost_Type>
  struct Min_Mean_Cycle_Result
  {
    bool has_cycle = false; ///< True if at least one directed cycle exists.
    long double minimum_mean = std::numeric_limits<long double>::infinity();
    typename GT::Node * witness_node = nullptr; ///< Vertex Karp's walk ends at (not necessarily on the cycle).

    Cost_Type cycle_total_cost = Cost_Type{0}; ///< Weight sum of witness cycle.
    size_t cycle_length = 0; ///< Number of arcs in witness cycle.

    DynList<typename GT::Node *> cycle_nodes; ///< Witness simple cycle (first node repeated at end).
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
# if ALEPH_KARP_INT128
    /// 128-bit integers for the sums that could overflow a `long long`.
    __extension__ typedef __int128 karp_int128_t;
    /// Unsigned counterpart of `karp_int128_t`.
    __extension__ typedef unsigned __int128 karp_uint128_t;
# endif

    /** @brief Arc entering a node, as Karp's table reads it.
     *
     *  @tparam GT Graph type.
     *  @tparam Cost_Type Arc weight type.
     */
    template <AlephGraph GT, typename Cost_Type>
    struct Incoming_Arc
    {
      size_t src_idx = 0;                 ///< Position of the source node.
      typename GT::Arc * arc = nullptr;   ///< The arc of the graph.
      Cost_Type weight = Cost_Type{0};    ///< Its weight, read once.
    };


    /** @brief Throw `std::domain_error` with `context` if a floating-point
     *         `value` is not finite.
     *  @param[in] value Value to check.
     *  @param[in] context Message of the exception.
     *  @throws std::domain_error If `value` is a NaN or an infinity.
     */
    template <typename Cost_Type>
    void validate_finite_accumulator(const Cost_Type & value, const char * context)
    {
      if constexpr (std::is_floating_point_v<Cost_Type>)
        ah_domain_error_if(not std::isfinite(value))
          << context;
    }


    /** @brief Build the incoming lists of the arcs of `g` accepted by `sa`,
     *         in arc-iteration order, reading each weight once.
     *
     *  @param[in] g Directed graph.
     *  @param[in] distance Arc cost accessor.
     *  @param[in] sa Arc filter.
     *  @param[out] nodes Receives the nodes of `g` by position.
     *  @return One list per node position: the arcs entering it.
     *  @throws std::domain_error If a floating-point weight is not finite.
     */
    template <AlephGraph GT, ArcDistance<GT> Distance, ArcFilter<GT> SA>
    Array<Array<Incoming_Arc<GT, typename Distance::Distance_Type>>>
    incoming_lists(const GT & g, Distance & distance, SA & sa,
                   Array<typename GT::Node *> & nodes)
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;
      using Cost_Type = typename Distance::Distance_Type;
      using Incoming = Incoming_Arc<GT, Cost_Type>;

      static_assert(std::is_arithmetic_v<Cost_Type>,
                    "Karp minimum mean cycle requires arithmetic arc costs");

      const size_t n = g.get_num_nodes();
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
          const Cost_Type w = distance(arc);
          if constexpr (std::is_floating_point_v<Cost_Type>)
            ah_domain_error_if(not std::isfinite(w))
              << "Karp minimum mean cycle requires finite arc weights";

          const size_t src_idx = node_to_idx.find(g.get_src_node(arc));
          const size_t tgt_idx = node_to_idx.find(g.get_tgt_node(arc));
          incoming[tgt_idx].append(Incoming{src_idx, arc, w});
        }

      return incoming;
    }


    /** @brief Answer of `karp_solve()`, in the node positions of its input.
     *
     *  @tparam GT Graph type.
     *  @tparam Cost_Type Arc weight type.
     */
    template <AlephGraph GT, typename Cost_Type>
    struct Karp_Answer
    {
      bool has_cycle = false;   ///< True if at least one directed cycle exists.
      long double minimum_mean = std::numeric_limits<long double>::infinity();
      size_t witness = 0;       ///< Position of the vertex the walk ends at (if `has_cycle`).
      Cost_Type cycle_total_cost = Cost_Type{0};   ///< Weight sum of the witness.
      Array<size_t> cycle_nodes;   ///< A simple cycle, by positions, the first one repeated at the end; may be empty.
      Array<typename GT::Arc *> cycle_arcs;   ///< Witness arcs, aligned with `cycle_nodes`.
      Array<Cost_Type> cycle_weights;         ///< Their weights, as read.
    };


    /** @brief Whether the mean `c1 / l1` is smaller than `c2 / l2`.
     *
     *  Integral sums are compared exactly, cross-multiplied in 128 bits
     *  (`ALEPH_KARP_INT128`): they are below `n * 2^63` and the lengths at
     *  most `n < 2^32`, so the products fit. Otherwise the two quotients are
     *  compared in `long double`.
     *
     *  @param[in] c1 Total cost of the first cycle.
     *  @param[in] l1 Its length, positive.
     *  @param[in] c2 Total cost of the second cycle.
     *  @param[in] l2 Its length, positive.
     *  @return True if the first mean is smaller.
     */
    template <typename Acc>
    [[nodiscard]] bool mean_less(const Acc c1, const size_t l1, const Acc c2, const size_t l2)
    {
# if ALEPH_KARP_INT128
      if constexpr (not std::is_floating_point_v<Acc>)
        return static_cast<karp_int128_t>(c1) * static_cast<karp_int128_t>(l2)
               < static_cast<karp_int128_t>(c2) * static_cast<karp_int128_t>(l1);
# endif
      return static_cast<long double>(c1) / static_cast<long double>(l1)
             < static_cast<long double>(c2) / static_cast<long double>(l2);
    }


    /** @brief Karp's table, with sums kept in `Acc`; see `karp_solve()`.
     *
     *  @tparam With_Witness Whether to keep predecessors and extract a witness.
     *  @tparam Acc Type of the sums of the table and of the witness.
     *  @tparam Checked Whether an addition can overflow `Acc` and has to be
     *          checked (it cannot when `karp_solve()` has bounded the sums).
     *  @tparam Exact_Difference Whether `Acc` holds the difference of two
     *          sums exactly, so that it is rounded once, not each sum.
     *  @param[in] incoming Incoming arcs of every node.
     *  @param[in] inf Value of `Acc` that marks an unreachable state; no sum
     *             can take it.
     *  @param[in] fn Name of the public function, for error messages.
     *  @return The minimum mean and, if `With_Witness`, the witness.
     *  @throws std::overflow_error If a checked addition overflows, or the
     *          witness cost does not fit `Cost_Type`.
     *  @throws std::domain_error If a floating-point accumulation is not finite.
     *  @note `karp_solve()` has checked that the tables fit a `size_t` of bytes.
     */
    template <bool With_Witness, typename Acc, bool Checked, bool Exact_Difference,
              AlephGraph GT, typename Cost_Type>
    Karp_Answer<GT, Cost_Type>
    karp_table(const Array<Array<Incoming_Arc<GT, Cost_Type>>> & incoming, const Acc inf,
               const char * fn)
    {
      using Incoming = Incoming_Arc<GT, Cost_Type>;

      const auto add = [](const Acc a, const Cost_Type w) -> Acc
      {
        if constexpr (Checked)
          return shortest_path_detail::checked_add(a, static_cast<Acc>(w));
        else
          return a + static_cast<Acc>(w);
      };

      Karp_Answer<GT, Cost_Type> result;
      const size_t n = incoming.size();
      if (n == 0)
        return result;

      const size_t total_states = (n + 1) * n;
      constexpr size_t none = std::numeric_limits<size_t>::max();

      Array<Acc> dp(total_states, inf);
      // Position, in `incoming[v]`, of the arc that reaches state (k, v).
      Array<size_t> pred;
      if constexpr (With_Witness)
        pred = Array<size_t>(total_states, none);

      const auto state_of = [n](const size_t k, const size_t v)
      {
        return k * n + v;
      };

      for (size_t v = 0; v < n; ++v)
        dp[state_of(0, v)] = Acc{0};

      for (size_t k = 1; k <= n; ++k)
        for (size_t v = 0; v < n; ++v)
          {
            Acc best = inf;
            size_t best_pos = none;
            const Array<Incoming> & in_v = incoming[v];

            for (size_t pos = 0; pos < in_v.size(); ++pos)
              {
                const Incoming & in = in_v[pos];
                const Acc prev = dp[state_of(k - 1, in.src_idx)];
                if (prev == inf)
                  continue;

                const Acc cand = add(prev, in.weight);
                validate_finite_accumulator(
                    cand,
                    "Karp minimum mean cycle accumulation became non-finite");

                if (best_pos == none
                    or cand < best
                    or (cand == best and in.src_idx < in_v[best_pos].src_idx))
                  {
                    best = cand;
                    best_pos = pos;
                  }
              }

            dp[state_of(k, v)] = best;
            if constexpr (With_Witness)
              pred[state_of(k, v)] = best_pos;
          }

      const long double ld_inf = std::numeric_limits<long double>::infinity();

      long double best_mean = ld_inf;
      size_t best_vertex = n;

      for (size_t v = 0; v < n; ++v)
        {
          const Acc dnv = dp[state_of(n, v)];
          if (dnv == inf)
            continue;

          long double local_max = -ld_inf;
          bool has_ratio = false;

          for (size_t k = 0; k < n; ++k)
            {
              const Acc dkv = dp[state_of(k, v)];
              if (dkv == inf)
                continue;

              long double num;
              if constexpr (Exact_Difference)
                num = static_cast<long double>(dnv - dkv);
              else
                num = static_cast<long double>(dnv) - static_cast<long double>(dkv);
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
      result.witness = best_vertex;

      if constexpr (With_Witness)
        {
          // Extract an n-step walk ending at best_vertex from predecessor table.
          Array<size_t> reverse_walk_nodes;
          reverse_walk_nodes.reserve(n + 1);
          Array<const Incoming *> reverse_walk_arcs;
          reverse_walk_arcs.reserve(n);

          size_t curr = best_vertex;
          reverse_walk_nodes.append(curr);

          for (size_t k = n; k > 0; --k)
            {
              const size_t pos = pred[state_of(k, curr)];
              if (pos == none)
                break;

              const Incoming & in = incoming[curr][pos];
              reverse_walk_arcs.append(&in);
              curr = in.src_idx;
              reverse_walk_nodes.append(curr);
            }

          if (reverse_walk_nodes.size() < 2)
            return result;

          Array<size_t> walk_nodes;
          walk_nodes.reserve(reverse_walk_nodes.size());
          for (size_t i = reverse_walk_nodes.size(); i > 0; --i)
            walk_nodes.append(reverse_walk_nodes[i - 1]);

          Array<const Incoming *> walk_arcs;
          walk_arcs.reserve(reverse_walk_arcs.size());
          for (size_t i = reverse_walk_arcs.size(); i > 0; --i)
            walk_arcs.append(reverse_walk_arcs[i - 1]);

          // Decompose the walk into simple cycles with a stack: when the walk
          // reaches a node already on the stack, the nodes above it close a
          // simple cycle, which is popped. The walk is then a simple path to
          // the chosen vertex plus those cycles, and the argument of Karp's
          // theorem shows that in exact arithmetic every one of them has the
          // minimum mean, so the first one would do. But the vertex was chosen
          // by rounded comparisons, and floating-point tables are rounded, so
          // the best of them is kept: compared exactly for integral costs
          // (cross-multiplied means), in long double otherwise.
          Array<size_t> stack_nodes;
          stack_nodes.reserve(n + 1);
          Array<const Incoming *> stack_arcs;   // stack_arcs[t] leaves stack_nodes[t]
          stack_arcs.reserve(n);
          Array<size_t> depth(n, none);          // position of a node on the stack

          bool found_cycle = false;
          Acc best_cycle_cost = Acc{0};
          size_t best_length = 0;
          Array<size_t> best_nodes;
          Array<const Incoming *> best_arcs;

          stack_nodes.append(walk_nodes[0]);
          depth[walk_nodes[0]] = 0;
          for (size_t i = 0; i < walk_arcs.size(); ++i)
            {
              const size_t x = walk_nodes[i + 1];
              ah_runtime_error_if(x >= n) << fn << "(): invalid witness node index";
              stack_arcs.append(walk_arcs[i]);
              if (depth[x] == none)
                {
                  depth[x] = stack_nodes.size();
                  stack_nodes.append(x);
                  continue;
                }

              const size_t j = depth[x];   // the cycle is stack_nodes[j..] back to x
              const size_t len = stack_arcs.size() - j;
              Acc cycle_cost = Acc{0};
              for (size_t t = j; t < stack_arcs.size(); ++t)
                {
                  cycle_cost = add(cycle_cost, stack_arcs[t]->weight);
                  validate_finite_accumulator(
                      cycle_cost,
                      "Karp minimum mean cycle witness accumulation became non-finite");
                }

              if (not found_cycle or mean_less(cycle_cost, len, best_cycle_cost, best_length))
                {
                  found_cycle = true;
                  best_cycle_cost = cycle_cost;
                  best_length = len;
                  best_nodes = Array<size_t>();
                  best_arcs = Array<const Incoming *>();
                  best_nodes.reserve(len + 1);
                  best_arcs.reserve(len);
                  for (size_t t = j; t < stack_arcs.size(); ++t)
                    {
                      best_nodes.append(stack_nodes[t]);
                      best_arcs.append(stack_arcs[t]);
                    }
                  best_nodes.append(x);
                }

              while (stack_nodes.size() > j + 1)
                {
                  depth[stack_nodes.get_last()] = none;
                  (void) stack_nodes.remove_last();
                }
              while (stack_arcs.size() > j)
                (void) stack_arcs.remove_last();
            }

          if (not found_cycle)
            return result;

          if constexpr (not std::is_same_v<Acc, Cost_Type>)
            ah_overflow_error_if(
                best_cycle_cost > static_cast<Acc>(std::numeric_limits<Cost_Type>::max())
                or best_cycle_cost < static_cast<Acc>(std::numeric_limits<Cost_Type>::lowest()))
              << fn << "(): the cost of the witness cycle does not fit the cost type";
          result.cycle_total_cost = static_cast<Cost_Type>(best_cycle_cost);
          result.cycle_nodes = std::move(best_nodes);
          result.cycle_arcs.reserve(best_arcs.size());
          result.cycle_weights.reserve(best_arcs.size());
          for (size_t t = 0; t < best_arcs.size(); ++t)
            {
              result.cycle_arcs.append(best_arcs[t]->arc);
              result.cycle_weights.append(best_arcs[t]->weight);
            }
        }

      return result;
    }


    /// How `karp_solve()` keeps the sums of a table (see there).
    enum class Karp_Sums
    {
      Cost_Type_Checked,   ///< The cost type itself, every addition checked.
      Long_Long,           ///< `long long`, which the bound keeps the sums inside.
      Long_Long_Checked,   ///< `long long`, every addition checked (no 128-bit integers).
      Int128               ///< 128-bit integers, which the bound keeps the sums inside.
    };


    /** @brief Choose how the sums of Karp's table over `incoming` are kept.
     *
     *  For integral costs that fit a `long long`, every sum adds at most `n`
     *  weights, so `n * max|w|` bounds it: `long long` when the bound is below
     *  `LLONG_MAX`, 128-bit integers otherwise, or checked `long long` without
     *  them. Any other cost type keeps its own sums, checked.
     *
     *  @param[in] incoming Incoming arcs of every node.
     *  @return The choice. O(E).
     */
    template <AlephGraph GT, typename Cost_Type>
    Karp_Sums karp_sums(const Array<Array<Incoming_Arc<GT, Cost_Type>>> & incoming)
    {
      if constexpr (std::is_integral_v<Cost_Type>
                    and std::numeric_limits<Cost_Type>::max()
                          <= std::numeric_limits<long long>::max())
        {
          constexpr long long ll_max = std::numeric_limits<long long>::max();
          unsigned long long max_abs = 0;   // max |w|, LLONG_MIN included
          for (size_t v = 0; v < incoming.size(); ++v)
            for (size_t pos = 0; pos < incoming[v].size(); ++pos)
              {
                const long long w = static_cast<long long>(incoming[v][pos].weight);
                const unsigned long long a = w < 0 ? 0ULL - static_cast<unsigned long long>(w)
                                                   : static_cast<unsigned long long>(w);
                if (a > max_abs)
                  max_abs = a;
              }

          const unsigned long long n = incoming.size();
          if (max_abs == 0 or n <= (static_cast<unsigned long long>(ll_max) - 1) / max_abs)
            return Karp_Sums::Long_Long;
# if ALEPH_KARP_INT128
          return Karp_Sums::Int128;
# else
          return Karp_Sums::Long_Long_Checked;
# endif
        }
      else
        return Karp_Sums::Cost_Type_Checked;
    }


    /** @brief Bytes that `karp_solve()` reserves for the tables of a graph of
     *         `n` nodes whose sums are kept as `sums`.
     *
     *  The table of sums has `(n + 1) * n` entries, and so has the table of
     *  predecessor positions (`size_t`), kept only with the witness. `Array`
     *  reserves a power of two of elements, the smallest not below the count,
     *  so that is what is counted: up to twice the entries. The arrays of
     *  `O(n)` elements that the witness extraction uses are not counted.
     *
     *  @tparam With_Witness Whether the table of predecessors is kept.
     *  @tparam Cost_Type Arc weight type.
     *  @param[in] n Number of nodes.
     *  @param[in] sums How the sums are kept (`karp_sums()`).
     *  @param[out] bytes The bytes, when they fit a `size_t`.
     *  @return False if they do not fit a `size_t`.
     */
    template <bool With_Witness, typename Cost_Type>
    [[nodiscard]] bool karp_table_bytes(const size_t n, const Karp_Sums sums, size_t & bytes) noexcept
    {
      constexpr size_t max = std::numeric_limits<size_t>::max();
      if (n == 0)
        {
          bytes = 0;
          return true;
        }
      if (n > max - 1 or n + 1 > max / n)
        return false;

      const size_t entries = (n + 1) * n;
      size_t reserved = 1;
      while (reserved < entries)
        {
          if (reserved > max / 2)
            return false;
          reserved <<= 1;
        }

      size_t sum_bytes = sizeof(Cost_Type);
      if (sums == Karp_Sums::Long_Long or sums == Karp_Sums::Long_Long_Checked)
        sum_bytes = sizeof(long long);
# if ALEPH_KARP_INT128
      else if (sums == Karp_Sums::Int128)
        sum_bytes = sizeof(karp_int128_t);
# endif
      const size_t per_entry = sum_bytes + (With_Witness ? sizeof(size_t) : 0);
      if (reserved > max / per_entry)
        return false;
      bytes = reserved * per_entry;
      return true;
    }


    /** @brief Karp's algorithm on a graph given by its incoming lists.
     *
     *  The node positions are `0 .. incoming.size() - 1`, and `incoming[v]`
     *  holds the arcs entering `v`. Nothing else is read: the graph, its
     *  accessor and its filter are not touched, so the public functions and
     *  the fallback of `howard_minimum_mean_cycle()` (which runs this on each
     *  strongly connected component of its own snapshot) share the same
     *  arithmetic and the same tie-breaking.
     *
     *  The table holds `(n + 1) * n` sums, plus as many predecessor positions
     *  when `With_Witness` is true. Its arithmetic depends on `Cost_Type`:
     *
     *  - **Integral types that fit a `long long`.** Every sum of the table and
     *    of the witness adds at most `n` weights, so it is bounded by
     *    `n * max|w|`. When that bound is below `LLONG_MAX` the sums are
     *    `long long` and cannot overflow. Otherwise they are 128-bit integers
     *    (`ALEPH_KARP_INT128`), which cannot overflow either; without them
     *    (MSVC) they stay `long long` and every addition is checked. Only
     *    the cost of the reported witness has to fit `Cost_Type`.
     *  - **Floating-point types.** The sums are `Cost_Type`; one that is not
     *    finite throws.
     *  - **Other types** (`unsigned long long`, 128-bit costs) keep their own
     *    type with checked additions.
     *
     *  An unreachable state is marked with a value no sum can take: the
     *  maximum of a type the bound keeps the sums below, or an infinity.
     *  Only in the last case of each list, with checked `long long` or the
     *  cost type's own sums, is it their maximum, which a sum could equal.
     *
     *  The choice is `karp_sums()`, and the bytes of the tables are
     *  `karp_table_bytes()`; both are separate so that a caller can know the
     *  bytes before running the table.
     *
     *  @tparam With_Witness Whether to keep predecessors and extract a witness.
     *  @param[in] incoming Incoming arcs of every node.
     *  @param[in] fn Name of the public function, for error messages.
     *  @return The minimum mean and, if `With_Witness`, the witness.
     *  @throws std::overflow_error If the bytes of the tables do not fit a
     *          `size_t`, a checked addition overflows, or the witness cost
     *          does not fit `Cost_Type`.
     *  @throws std::domain_error If a floating-point accumulation is not finite.
     */
    template <bool With_Witness, AlephGraph GT, typename Cost_Type>
    Karp_Answer<GT, Cost_Type>
    karp_solve(const Array<Array<Incoming_Arc<GT, Cost_Type>>> & incoming, const char * fn)
    {
      const Karp_Sums sums = karp_sums(incoming);
      size_t bytes = 0;
      const bool fits = karp_table_bytes<With_Witness, Cost_Type>(incoming.size(), sums, bytes);
      ah_overflow_error_if(not fits) << fn << "(): DP table size overflow";

      if constexpr (std::is_floating_point_v<Cost_Type>)
        return karp_table<With_Witness, Cost_Type, true, false>(
            incoming, std::numeric_limits<Cost_Type>::infinity(), fn);
      else if constexpr (std::is_integral_v<Cost_Type>
                         and std::numeric_limits<Cost_Type>::max()
                               <= std::numeric_limits<long long>::max())
        {
          constexpr long long ll_max = std::numeric_limits<long long>::max();
          if (sums == Karp_Sums::Long_Long)
            return karp_table<With_Witness, long long, false, false>(incoming, ll_max, fn);
# if ALEPH_KARP_INT128
          // n * max|w| < 2^64 * 2^63: no sum, nor the difference of two, overflows.
          constexpr karp_int128_t int128_max =
            static_cast<karp_int128_t>(~static_cast<karp_uint128_t>(0) >> 1);
          return karp_table<With_Witness, karp_int128_t, false, true>(incoming, int128_max, fn);
# else
          return karp_table<With_Witness, long long, true, false>(incoming, ll_max, fn);
# endif
        }
      else
        return karp_table<With_Witness, Cost_Type, true, false>(
            incoming, std::numeric_limits<Cost_Type>::max(), fn);
    }
  } // namespace min_mean_cycle_detail


  /** @brief Compute minimum mean cycle by Karp's algorithm.
   *
   *  Behavior:
   *  - If the digraph has no directed cycle, `has_cycle = false`.
   *  - Otherwise returns `minimum_mean` and a witness: a *simple* cycle. The
   *    predecessors of the table give a walk of `V` arcs to the vertex
   *    `witness_node`; it is decomposed into simple cycles and the one of
   *    smallest mean is reported (means compared exactly for integral costs
   *    where 128-bit integers exist, in `long double` otherwise). In exact
   *    arithmetic every one of them has the minimum mean; with floating-point
   *    costs the reported one attains `minimum_mean` up to rounding, and its
   *    `cycle_total_cost` is the sum of its weights in arc order.
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
   *  @exception overflow_error If the cost of the witness does not fit the
   *             cost type, or a checked integer addition overflows (see
   *             the file documentation).
   *
   *  @note The distance accessor is called once per arc accepted by `sa`;
   *        the witness cost is summed from those values.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  Min_Mean_Cycle_Result<GT, typename Distance::Distance_Type>
  karp_minimum_mean_cycle(const GT & g,
                          Distance distance = Distance(),
                          SA sa = SA())
  {
    using Node = typename GT::Node;
    using Cost_Type = typename Distance::Distance_Type;

    ah_domain_error_if(not g.is_digraph())
      << "karp_minimum_mean_cycle(): graph must be directed";

    Array<Node *> nodes;
    const auto incoming = min_mean_cycle_detail::incoming_lists(g, distance, sa, nodes);
    const auto answer = min_mean_cycle_detail::karp_solve<true, GT, Cost_Type>(
        incoming, "karp_minimum_mean_cycle");

    Min_Mean_Cycle_Result<GT, Cost_Type> result;
    if (not answer.has_cycle)
      return result;

    result.has_cycle = true;
    result.minimum_mean = answer.minimum_mean;
    result.witness_node = nodes[answer.witness];
    result.cycle_total_cost = answer.cycle_total_cost;
    result.cycle_length = answer.cycle_arcs.size();
    for (size_t i = 0; i < answer.cycle_nodes.size(); ++i)
      result.cycle_nodes.append(nodes[answer.cycle_nodes[i]]);
    for (size_t i = 0; i < answer.cycle_arcs.size(); ++i)
      result.cycle_arcs.append(answer.cycle_arcs[i]);

    return result;
  }

  /** @brief Compute only the minimum mean value (without witness walk).
   *
   *  This variant avoids predecessor storage and witness reconstruction.
   *  It is useful when only the optimal mean value is required. Its table
   *  and arithmetic are those of `karp_minimum_mean_cycle()`, so both return
   *  the same `minimum_mean`.
   *
   *  @tparam GT Directed Aleph graph type.
   *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
   *  @tparam SA Arc filter.
   *
   *  @param[in] g Graph (must be directed).
   *  @param[in] distance Arc cost accessor, called once per accepted arc.
   *  @param[in] sa Arc filter.
   *
   *  @return Whether a cycle exists and the minimum mean.
   *
   *  @exception domain_error If `g` is undirected, or floating weights (or
   *             their sums) are not finite.
   *  @exception overflow_error If a checked integer addition overflows:
   *             never for integer costs of at most 64 bits where 128-bit
   *             integers exist (see the file documentation).
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  Min_Mean_Cycle_Value_Result
  karp_minimum_mean_cycle_value(const GT & g,
                                Distance distance = Distance(),
                                SA sa = SA())
  {
    using Node = typename GT::Node;
    using Cost_Type = typename Distance::Distance_Type;

    ah_domain_error_if(not g.is_digraph())
      << "karp_minimum_mean_cycle_value(): graph must be directed";

    Array<Node *> nodes;
    const auto incoming = min_mean_cycle_detail::incoming_lists(g, distance, sa, nodes);
    const auto answer = min_mean_cycle_detail::karp_solve<false, GT, Cost_Type>(
        incoming, "karp_minimum_mean_cycle_value");

    Min_Mean_Cycle_Value_Result result;
    result.has_cycle = answer.has_cycle;
    result.minimum_mean = answer.minimum_mean;
    return result;
  }

  /** @brief Alias for `karp_minimum_mean_cycle_value()`.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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
