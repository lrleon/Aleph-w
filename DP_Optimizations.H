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


/** @file DP_Optimizations.H
 *  @brief Classical DP optimization toolkit: D&C DP, Knuth, CHT, Li Chao,
 *         and monotone-queue transitions.
 *
 *  This header collects optimization patterns that are frequently used to cut
 *  asymptotic complexity in dynamic programming recurrences:
 *
 *  - Divide & Conquer optimization for partition-like DP layers.
 *  - Knuth optimization for interval DP under quadrangle-inequality assumptions.
 *  - Convex Hull Trick (monotone slopes, min-query lower envelope).
 *  - Li Chao Tree for arbitrary line insertion / min query on integral x-domain.
 *  - Monotone-queue optimization for windowed transitions.
 *
 *  Additionally, it includes a geometric utility that applies Li Chao to the
 *  lower envelope of parabolas (weighted squared distances on a line).
 *
 *  @example dp_optimizations_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef DP_OPTIMIZATIONS_H
# define DP_OPTIMIZATIONS_H

# include <algorithm>
# include <cstddef>
# include <limits>
# include <numeric>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  namespace dp_optimization_detail
  {
    template <typename T>
    [[nodiscard]] constexpr T default_inf() noexcept
    {
      if constexpr (std::numeric_limits<T>::has_infinity)
        return std::numeric_limits<T>::infinity();
      else
        return std::numeric_limits<T>::max() / 4;
    }
  } // namespace dp_optimization_detail


  /** @brief Result of divide-and-conquer partition DP optimization. */
  template <typename Cost>
  struct Divide_Conquer_DP_Result
  {
    Cost optimal_cost = Cost{}; /**< Final optimum at state (groups, n). */
    Array<Cost> last_row; /**< Last DP layer (size n+1). */
    Array<Array<size_t>> split; /**< Best split index per layer/state. */
  };


  /** @brief Optimize partition DP using divide-and-conquer optimization.
   *
   *  Computes DP of the form:
   *  @code
   *  dp[g][i] = min_{k < i} (dp[g-1][k] + transition_cost(k, i))
   *  @endcode
   *  for `g = 1..groups`, `i = 0..n`, where each partition is non-empty.
   *
   *  Correctness and complexity rely on the monotonicity of argmins:
   *  `opt[g][i] <= opt[g][i+1]`.
   *
   *  @tparam Cost Numeric DP cost type.
   *  @tparam Transition_Cost_Fn Callable returning `Cost` given `(k, i)`.
   *  @param[in] groups Number of groups/segments.
   *  @param[in] n Number of elements/states in prefix space.
   *  @param[in] transition_cost Cost for segment `[k, i)`.
   *  @param[in] inf Sentinel for unreachable states.
   *
   *  @return Divide_Conquer_DP_Result<Cost> with final optimum, last row,
   *          and split table.
   *
   *  @note Base state is `dp[0][0] = 0`, `dp[0][i>0] = inf`.
   *  @note Complexity: typically `O(groups * n * log n)` with O(1) transition.
   */
  template <typename Cost, class Transition_Cost_Fn>
  [[nodiscard]] inline Divide_Conquer_DP_Result<Cost>
  divide_and_conquer_partition_dp(const size_t groups,
                                  const size_t n,
                                  Transition_Cost_Fn transition_cost,
                                  const Cost inf =
                                    dp_optimization_detail::default_inf<Cost>())
  {
    Array<Cost> prev = Array<Cost>::create(n + 1);
    Array<Cost> curr = Array<Cost>::create(n + 1);

    for (size_t i = 0; i <= n; ++i)
      {
        prev(i) = inf;
        curr(i) = inf;
      }
    prev(0) = Cost{};

    Array<Array<size_t>> split;
    split.reserve(groups + 1);
    for (size_t g = 0; g <= groups; ++g)
      {
        Array<size_t> row = Array<size_t>::create(n + 1);
        for (size_t i = 0; i <= n; ++i)
          row(i) = 0;
        split.append(std::move(row));
      }

    for (size_t g = 1; g <= groups; ++g)
      {
        for (size_t i = 0; i <= n; ++i)
          curr(i) = inf;

        if (g <= n)
          {
            auto solve = [&] (auto && self,
                              const size_t left, const size_t right,
                              const size_t opt_left, const size_t opt_right) -> void
            {
              if (left > right)
                return;

              const size_t mid = std::midpoint(left, right);
              const size_t k_end = std::min(mid - 1, opt_right);

              Cost best = inf;
              size_t best_k = opt_left;

              for (size_t k = opt_left; k <= k_end; ++k)
                {
                  if (prev[k] == inf)
                    continue;

                  const Cost cand = prev[k] + transition_cost(k, mid);
                  if (cand < best)
                    {
                      best = cand;
                      best_k = k;
                    }
                }

              curr(mid) = best;
              split[g](mid) = best_k;

              if (left < mid)
                self(self, left, mid - 1, opt_left, best_k);
              if (mid < right)
                self(self, mid + 1, right, best_k, opt_right);
            };

            solve(solve, g, n, g - 1, n - 1);
          }

        prev.swap(curr);
      }

    return Divide_Conquer_DP_Result<Cost>{
      prev[n],
      std::move(prev),
      std::move(split)
    };
  }


  /** @brief Result of Knuth interval-DP optimization. */
  template <typename Cost>
  struct Knuth_Optimization_Result
  {
    Cost optimal_cost = Cost{}; /**< Final optimum at interval [0, n). */
    Array<Array<Cost>> dp; /**< Interval DP table. */
    Array<Array<size_t>> opt; /**< Argmin table used by Knuth bounds. */
  };


  /** @brief Optimize interval DP with Knuth optimization.
   *
   *  Solves recurrences of the form:
   *  @code
   *  dp[i][j] = min_{i < k < j} (dp[i][k] + dp[k][j] + interval_cost(i, j))
   *  @endcode
   *  for intervals `[i, j)` over `n` atomic elements.
   *
   *  Requires the standard Knuth assumptions to hold for `interval_cost`:
   *  quadrangle inequality and monotone optima.
   *
   *  @tparam Cost Numeric DP cost type.
   *  @tparam Interval_Cost_Fn Callable returning `Cost` given `(i, j)`.
   *  @param[in] n Number of atomic elements.
   *  @param[in] interval_cost Cost term depending only on interval `[i, j)`.
   *  @param[in] inf Sentinel for unreachable states.
   *
   *  @return Knuth_Optimization_Result<Cost> with optimum, DP table and opt table.
   *
   *  @note Base case: `dp[i][i] = 0`, `dp[i][i+1] = 0`.
   *  @note Complexity: O(n^2).
   */
  template <typename Cost, class Interval_Cost_Fn>
  [[nodiscard]] inline Knuth_Optimization_Result<Cost>
  knuth_optimize_interval(const size_t n,
                          Interval_Cost_Fn interval_cost,
                          const Cost inf =
                            dp_optimization_detail::default_inf<Cost>())
  {
    Array<Array<Cost>> dp;
    dp.reserve(n + 1);
    for (size_t i = 0; i <= n; ++i)
      {
        Array<Cost> row = Array<Cost>::create(n + 1);
        for (size_t j = 0; j <= n; ++j)
          row(j) = inf;
        dp.append(std::move(row));
      }

    Array<Array<size_t>> opt;
    opt.reserve(n + 1);
    for (size_t i = 0; i <= n; ++i)
      {
        Array<size_t> row = Array<size_t>::create(n + 1);
        for (size_t j = 0; j <= n; ++j)
          row(j) = 0;
        opt.append(std::move(row));
      }

    for (size_t i = 0; i <= n; ++i)
      {
        dp(i)(i) = Cost{};
        opt(i)(i) = i;
        if (i + 1 <= n)
          {
            dp(i)(i + 1) = Cost{};
            opt(i)(i + 1) = i + 1;
          }
      }

    for (size_t len = 2; len <= n; ++len)
      for (size_t i = 0; i + len <= n; ++i)
        {
          const size_t j = i + len;

          size_t left = opt[i][j - 1];
          size_t right = opt[i + 1][j];
          if (left < i + 1)
            left = i + 1;
          if (right > j - 1)
            right = j - 1;

          ah_runtime_error_if(left > right)
            << "knuth_optimize_interval: invalid opt bounds at ["
            << i << ", " << j << ")";

          const Cost w = interval_cost(i, j);

          Cost best = inf;
          size_t best_k = left;

          for (size_t k = left; k <= right; ++k)
            {
              const Cost cand = dp[i][k] + dp[k][j] + w;
              if (cand < best)
                {
                  best = cand;
                  best_k = k;
                }
            }

          dp(i)(j) = best;
          opt(i)(j) = best_k;
        }

    return Knuth_Optimization_Result<Cost>{
      n == 0 ? Cost{} : dp[0][n],
      std::move(dp),
      std::move(opt)
    };
  }


  /** @brief Optimal adjacent merge cost via Knuth optimization.
   *
   *  Given item weights, computes minimum total merge cost when only adjacent
   *  groups can be merged (optimal alphabetic merge / interval merge DP).
   *
   *  @param[in] weights Atomic interval weights.
   *  @return Knuth_Optimization_Result<size_t> for the full interval [0, n).
   *
   *  @throws ah_runtime_error on prefix-sum overflow.
   */
  [[nodiscard]] inline Knuth_Optimization_Result<size_t>
  optimal_merge_knuth(const Array<size_t> & weights)
  {
    const size_t n = weights.size();
    Array<size_t> prefix = Array<size_t>::create(n + 1);
    prefix(0) = 0;

    for (size_t i = 0; i < n; ++i)
      {
        ah_runtime_error_if(prefix[i] >
                            std::numeric_limits<size_t>::max() - weights[i])
          << "optimal_merge_knuth: prefix sum overflow";
        prefix(i + 1) = prefix[i] + weights[i];
      }

    return knuth_optimize_interval<size_t>(
      n,
      [&] (const size_t i, const size_t j) -> size_t
      {
        return prefix[j] - prefix[i];
      },
      std::numeric_limits<size_t>::max() / 4
    );
  }


  /** @brief Convex Hull Trick for minimum queries.
   *
   *  Supports insertion of lines with non-increasing slope and minimum queries
   *  over x. Query can be done in O(log n) for arbitrary x or amortized O(1)
   *  for non-decreasing x via `query_monotone()`.
   */
  template <typename T>
  class Convex_Hull_Trick
  {
  public:
    /** @brief Affine line y = m*x + b. */
    struct Line
    {
      T slope = T{};
      T intercept = T{};

      [[nodiscard]] T value_at(const T x) const noexcept
      {
        return slope * x + intercept;
      }
    };

  private:
    Array<Line> lines_;
    Array<long double> starts_; // x where this line becomes optimal
    size_t cursor_ = 0; // for monotone queries

    [[nodiscard]] static long double intersection_x(const Line & a,
                                                    const Line & b) noexcept
    {
      return static_cast<long double>(b.intercept - a.intercept)
           / static_cast<long double>(a.slope - b.slope);
    }

  public:
    [[nodiscard]] size_t size() const noexcept { return lines_.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return lines_.size() == 0; }

    void clear()
    {
      Array<Line> tmp_lines;
      lines_.swap(tmp_lines);

      Array<long double> tmp_starts;
      starts_.swap(tmp_starts);

      cursor_ = 0;
    }

    void reset_query_cursor() noexcept
    {
      cursor_ = 0;
    }

    /** @brief Insert a new line; slopes must be non-increasing. */
    void add_line(const T slope, const T intercept)
    {
      Line ln{slope, intercept};

      if (lines_.size() > 0)
        {
          const Line & last = lines_[lines_.size() - 1];
          ah_domain_error_if(ln.slope > last.slope)
            << "Convex_Hull_Trick::add_line: slopes must be non-increasing";

          // Keep only best intercept for equal slope.
          if (ln.slope == last.slope)
            {
              if (ln.intercept >= last.intercept)
                return;

              (void) lines_.remove_last();
              (void) starts_.remove_last();
              if (cursor_ > lines_.size())
                cursor_ = lines_.size();
            }
        }

      while (lines_.size() > 0)
        {
          const long double x = intersection_x(lines_[lines_.size() - 1], ln);

          if (lines_.size() == 1 or x > starts_[starts_.size() - 1])
            {
              starts_.append(x);
              lines_.append(ln);
              if (cursor_ >= lines_.size())
                cursor_ = lines_.size() - 1;
              return;
            }

          (void) lines_.remove_last();
          (void) starts_.remove_last();
          if (cursor_ > lines_.size())
            cursor_ = lines_.size();
        }

      starts_.append(-std::numeric_limits<long double>::infinity());
      lines_.append(ln);
      if (cursor_ >= lines_.size())
        cursor_ = lines_.size() - 1;
    }

    /** @brief Query minimum value at arbitrary x (O(log n)). */
    [[nodiscard]] T query(const T x) const
    {
      ah_runtime_error_if(lines_.size() == 0)
        << "Convex_Hull_Trick::query: no lines available";

      size_t lo = 0;
      size_t hi = lines_.size() - 1;
      const auto xd = static_cast<long double>(x);

      while (lo < hi)
        {
          const size_t mid = std::midpoint(lo, hi + 1);
          if (starts_[mid] <= xd)
            lo = mid;
          else
            hi = mid - 1;
        }

      return lines_[lo].value_at(x);
    }

    /** @brief Query minimum with non-decreasing x (amortized O(1)). */
    [[nodiscard]] T query_monotone(const T x)
    {
      ah_runtime_error_if(lines_.size() == 0)
        << "Convex_Hull_Trick::query_monotone: no lines available";

      while (cursor_ + 1 < lines_.size()
             and starts_[cursor_ + 1] <= static_cast<long double>(x))
        ++cursor_;

      return lines_[cursor_].value_at(x);
    }
  };


  /** @brief Li Chao tree for min line queries on an integral x-domain.
   *
   *  Supports arbitrary line insertion order and O(log X) min queries where X
   *  is the domain length.
   */
  template <typename T>
  class Li_Chao_Tree
  {
    static_assert(std::is_integral_v<T>,
                  "Li_Chao_Tree requires an integral coordinate/value type");

  public:
    /** @brief Affine line y = m*x + b. */
    struct Line
    {
      T slope = T{};
      T intercept = T{};

      [[nodiscard]] T value_at(const T x) const noexcept
      {
        return slope * x + intercept;
      }
    };

  private:
    struct Node
    {
      Line line;
      size_t left = std::numeric_limits<size_t>::max();
      size_t right = std::numeric_limits<size_t>::max();
    };

    static constexpr size_t NIL = std::numeric_limits<size_t>::max();

    T x_left_;
    T x_right_;
    Array<Node> nodes_;
    size_t root_ = NIL;

    [[nodiscard]] size_t new_node(const Line & line)
    {
      nodes_.append(Node{line, NIL, NIL});
      return nodes_.size() - 1;
    }

    void add_line_impl(const size_t idx, const T l, const T r, Line line)
    {
      const T mid = std::midpoint(l, r);

      if (line.value_at(mid) < nodes_[idx].line.value_at(mid))
        std::swap(line, nodes_[idx].line);

      if (l == r)
        return;

      if (line.value_at(l) < nodes_[idx].line.value_at(l))
        {
          if (nodes_[idx].left == NIL)
            {
              nodes_[idx].left = new_node(line);
              return;
            }
          add_line_impl(nodes_[idx].left, l, mid, line);
          return;
        }

      if (line.value_at(r) < nodes_[idx].line.value_at(r))
        {
          if (nodes_[idx].right == NIL)
            {
              nodes_[idx].right = new_node(line);
              return;
            }
          add_line_impl(nodes_[idx].right, mid + 1, r, line);
        }
    }

    [[nodiscard]] T query_impl(const size_t idx,
                               const T l, const T r,
                               const T x) const
    {
      T ret = nodes_[idx].line.value_at(x);
      if (l == r)
        return ret;

      const T mid = std::midpoint(l, r);
      if (x <= mid and nodes_[idx].left != NIL)
        ret = std::min(ret, query_impl(nodes_[idx].left, l, mid, x));
      else if (x > mid and nodes_[idx].right != NIL)
        ret = std::min(ret, query_impl(nodes_[idx].right, mid + 1, r, x));

      return ret;
    }

  public:
    Li_Chao_Tree(const T x_left, const T x_right)
      : x_left_(x_left), x_right_(x_right)
    {
      ah_domain_error_if(x_left_ > x_right_)
        << "Li_Chao_Tree: invalid domain [" << x_left_ << ", " << x_right_ << "]";
    }

    [[nodiscard]] bool is_empty() const noexcept { return root_ == NIL; }
    [[nodiscard]] size_t node_count() const noexcept { return nodes_.size(); }

    void clear()
    {
      Array<Node> tmp;
      nodes_.swap(tmp);
      root_ = NIL;
    }

    void add_line(const T slope, const T intercept)
    {
      const Line line{slope, intercept};
      if (root_ == NIL)
        {
          root_ = new_node(line);
          return;
        }
      add_line_impl(root_, x_left_, x_right_, line);
    }

    [[nodiscard]] T query(const T x) const
    {
      ah_runtime_error_if(root_ == NIL)
        << "Li_Chao_Tree::query: no lines available";
      ah_out_of_range_error_if(x < x_left_ or x > x_right_)
        << "Li_Chao_Tree::query: x=" << x
        << " outside domain [" << x_left_ << ", " << x_right_ << "]";

      return query_impl(root_, x_left_, x_right_, x);
    }
  };


  /** @brief Result of monotone-queue windowed DP transition. */
  template <typename Cost>
  struct Monotone_Queue_DP_Result
  {
    Array<Cost> dp; /**< DP values. */
    Array<size_t> parent; /**< Chosen predecessor index for each i. */
  };


  /** @brief Optimize windowed min-transition DP with a monotone queue.
   *
   *  Solves:
   *  @code
   *  dp[0] = base_cost[0]
   *  dp[i] = base_cost[i] + min(dp[j]) for j in [max(0, i-window), i-1]
   *  @endcode
   *
   *  @param[in] base_cost Additive base cost per state.
   *  @param[in] window Maximum predecessor distance.
   *
   *  @return Monotone_Queue_DP_Result with dp and parent pointers.
   *
   *  @throws ah_domain_error if `window == 0` and size > 1.
   *
   *  @note Complexity: O(n).
   */
  template <typename Cost>
  [[nodiscard]] inline Monotone_Queue_DP_Result<Cost>
  monotone_queue_min_dp(const Array<Cost> & base_cost,
                        const size_t window)
  {
    const size_t n = base_cost.size();
    if (n == 0)
      return Monotone_Queue_DP_Result<Cost>{Array<Cost>(), Array<size_t>()};

    ah_domain_error_if(window == 0 and n > 1)
      << "monotone_queue_min_dp: window must be > 0 when n > 1";

    Array<Cost> dp = Array<Cost>::create(n);
    Array<size_t> parent = Array<size_t>::create(n);

    Array<size_t> deque_idx;
    size_t head = 0;

    dp(0) = base_cost[0];
    parent(0) = 0;
    deque_idx.append(0);

    for (size_t i = 1; i < n; ++i)
      {
        const size_t min_valid = (i > window) ? (i - window) : 0;

        while (head < deque_idx.size() and deque_idx[head] < min_valid)
          ++head;

        ah_runtime_error_if(head == deque_idx.size())
          << "monotone_queue_min_dp: no valid predecessor for i=" << i;

        const size_t best = deque_idx[head];
        dp(i) = dp[best] + base_cost[i];
        parent(i) = best;

        while (deque_idx.size() > head
               and dp[deque_idx[deque_idx.size() - 1]] >= dp[i])
          (void) deque_idx.remove_last();

        deque_idx.append(i);
      }

    return Monotone_Queue_DP_Result<Cost>{std::move(dp), std::move(parent)};
  }


  /** @brief Weighted squared-distance lower envelope on a line.
   *
   *  For each query coordinate `x_i`, computes:
   *  @code
   *  min_j ( (x_i - x_j)^2 + weight_j )
   *  @endcode
   *  by transforming each site `(x_j, weight_j)` into a line:
   *  `m = -2*x_j`, `b = x_j^2 + weight_j`, then adding back `x_i^2`.
   *
   *  This is a geometric application of CHT/Li Chao (lower envelope of
   *  parabolas) and also a common DP optimization pattern.
   *
   *  @param[in] xs Signed integral coordinates.
   *  @param[in] weights Additive weights per coordinate.
   *
   *  @return Array<T> with one minimum value per query coordinate.
   *
   *  @throws ah_domain_error if array sizes differ.
   */
  template <typename T>
  [[nodiscard]] inline Array<T>
  min_weighted_squared_distance_1d(const Array<T> & xs,
                                   const Array<T> & weights)
  {
    static_assert(std::is_integral_v<T> and std::is_signed_v<T>,
                  "min_weighted_squared_distance_1d requires signed integral T");

    ah_domain_error_if(xs.size() != weights.size())
      << "min_weighted_squared_distance_1d: xs and weights size mismatch";

    const size_t n = xs.size();
    if (n == 0)
      return Array<T>();

    T min_x = xs[0];
    T max_x = xs[0];
    for (size_t i = 1; i < n; ++i)
      {
        if (xs[i] < min_x)
          min_x = xs[i];
        if (xs[i] > max_x)
          max_x = xs[i];
      }

    Li_Chao_Tree<T> lc(min_x, max_x);
    for (size_t j = 0; j < n; ++j)
      {
        const T m = -2 * xs[j];
        const T b = xs[j] * xs[j] + weights[j];
        lc.add_line(m, b);
      }

    Array<T> ret = Array<T>::create(n);
    for (size_t i = 0; i < n; ++i)
      ret(i) = xs[i] * xs[i] + lc.query(xs[i]);

    return ret;
  }
} // namespace Aleph

# endif // DP_OPTIMIZATIONS_H
