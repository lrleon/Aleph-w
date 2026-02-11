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


/** @file tpl_mo_algorithm.H
 *  @brief Mo's algorithm for offline range queries.
 *
 *  Mo's algorithm answers Q offline range queries on a static array of
 *  N elements in O((N+Q)sqrt(N)) time.  It is useful when the query
 *  function is "decomposable" (maintainable via add/remove of single
 *  elements) but has no algebraic structure (not a monoid, not
 *  invertible, not idempotent).
 *
 *  Classic applications: distinct element count, mode, powerful array.
 *
 *  The implementation uses the snake-optimized sweep (alternating
 *  direction of the right pointer in even/odd blocks) for better
 *  cache behaviour.
 *
 *  ## Complexity
 *  | Phase       | Time               | Space  |
 *  |-------------|--------------------|--------|
 *  | Sort queries| O(Q lg Q)          | O(Q)   |
 *  | Sweep       | O((N+Q) sqrt(N))   | O(N+Q) |
 *  | Total       | O((N+Q) sqrt(N))   | O(N+Q) |
 *
 *  @note Mo's algorithm on trees is not covered here (it depends on
 *  the graph subsystem and will be a separate header).
 *
 *  @see tpl_segment_tree.H     Online range queries (monoid).
 *  @see tpl_sparse_table.H     Static O(1) range queries (idempotent).
 *  @see tpl_fenwick_tree.H     Prefix-based range queries (invertible).
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_MO_ALGORITHM_H
# define TPL_MO_ALGORITHM_H

# include <algorithm>
# include <cmath>
# include <concepts>
# include <type_traits>
# include <unordered_map>
# include <utility>
# include <vector>
# include <tpl_array.H>
# include <tpl_dynList.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief A query for Mo's algorithm: half-open range [l, r] with id.
   *  @ingroup Sequences
   */
  struct Mo_Query
  {
    size_t l;   ///< Left endpoint (inclusive, 0-based).
    size_t r;   ///< Right endpoint (inclusive, 0-based).
    size_t id;  ///< Original query index (for reordering answers).
  };

  /** @brief Concept constraining a policy for Mo's algorithm.
   *
   *  A valid `MoPolicy` must provide:
   *  - `answer_type` — the result type for a single query.
   *  - `init(data, n)` — reset internal state before the sweep.
   *  - `add(data, idx)` — expand the current window to include `data(idx)`.
   *  - `remove(data, idx)` — shrink the current window by removing `data(idx)`.
   *  - `answer()` — return the answer for the current window.
   *
   *  @tparam P  Policy type.
   *  @tparam T  Element type of the data array.
   *  @ingroup Sequences
   */
  template <typename P, typename T>
  concept MoPolicy = requires(P & p, const Array<T> & data,
                              size_t idx, size_t n)
    {
      typename P::answer_type;
      { p.init(data, n) };
      { p.add(data, idx) };
      { p.remove(data, idx) };
      { p.answer() } -> std::convertible_to<typename P::answer_type>;
    };

  // ================================================================
  // Gen_Mo_Algorithm — Offline range queries via Mo's algorithm
  // ================================================================

  /** @brief Offline range query engine using Mo's algorithm.
   *
   *  Given a static array of N elements and Q range queries [l, r],
   *  `Gen_Mo_Algorithm` answers all queries in O((N+Q) sqrt(N)) time
   *  by sorting queries into blocks and sweeping a sliding window.
   *
   *  The user supplies a `Policy` object that maintains per-window
   *  state (add/remove elements, compute the current answer).
   *
   *  @tparam T      Element type stored in the array.
   *  @tparam Policy A type satisfying `MoPolicy<Policy, T>`.
   *
   *  @par Example
   *  @code
   *  // Count distinct elements in ranges
   *  Distinct_Count_Mo<int> mo = {1, 2, 1, 3, 2, 1};
   *  auto ans = mo.solve({{0, 2}, {1, 4}, {0, 5}});
   *  // ans(0) == 2, ans(1) == 3, ans(2) == 3
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Policy>
    requires MoPolicy<Policy, T>
  class Gen_Mo_Algorithm
  {
    Array<T> data_;
    mutable Policy pol_;

  public:
    using Item_Type = T;
    using answer_type = typename Policy::answer_type;

    /** @brief Construct from an initializer list.
     *  @param il   initializer list of elements.
     *  @param p    policy object (default-constructed if omitted).
     */
    Gen_Mo_Algorithm(std::initializer_list<T> il, Policy p = Policy())
      : data_(il), pol_(std::move(p))
    {}

    /** @brief Construct from an Array\<T\>.
     *  @param arr  source array.
     *  @param p    policy object.
     */
    Gen_Mo_Algorithm(const Array<T> & arr, Policy p = Policy())
      : data_(arr), pol_(std::move(p))
    {}

    /** @brief Construct from an Array\<T\> (move).
     *  @param arr  source array (moved from).
     *  @param p    policy object.
     */
    Gen_Mo_Algorithm(Array<T> && arr, Policy p = Policy())
      : data_(std::move(arr)), pol_(std::move(p))
    {}

    /** @brief Construct from a std::vector\<T\>.
     *  @param vec  source vector.
     *  @param p    policy object.
     */
    Gen_Mo_Algorithm(const std::vector<T> & vec, Policy p = Policy())
      : data_(Array<T>::create(vec.size())), pol_(std::move(p))
    {
      for (size_t i = 0; i < vec.size(); ++i)
        data_(i) = vec[i];
    }

    /** @brief Construct from a DynList\<T\>.
     *  @param lst  source list.
     *  @param p    policy object.
     */
    Gen_Mo_Algorithm(const DynList<T> & lst, Policy p = Policy())
      : data_(Array<T>::create(lst.size())), pol_(std::move(p))
    {
      size_t i = 0;
      for (auto it = lst.get_it(); it.has_curr(); it.next_ne())
        data_(i++) = it.get_curr();
    }

    Gen_Mo_Algorithm(const Gen_Mo_Algorithm &) = default;

    Gen_Mo_Algorithm(Gen_Mo_Algorithm &&) noexcept(
      std::is_nothrow_move_constructible_v<Array<T>> and
      std::is_nothrow_move_constructible_v<Policy>) = default;

    Gen_Mo_Algorithm & operator=(const Gen_Mo_Algorithm &) = default;

    Gen_Mo_Algorithm & operator=(Gen_Mo_Algorithm &&) noexcept(
      std::is_nothrow_move_assignable_v<Array<T>> and
      std::is_nothrow_move_assignable_v<Policy>) = default;

    /** @brief Number of elements in the data array. */
    [[nodiscard]] constexpr size_t size() const noexcept
    {
      return data_.size();
    }

    /** @brief True if the data array is empty. */
    [[nodiscard]] constexpr bool is_empty() const noexcept
    {
      return data_.size() == 0;
    }

    /** @brief Swap with @p other in O(1). */
    void swap(Gen_Mo_Algorithm & other) noexcept(
      noexcept(std::swap(std::declval<Policy&>(), std::declval<Policy&>())))
    {
      data_.swap(other.data_);
      std::swap(pol_, other.pol_);
    }

    /** @brief Solve queries given as (l, r) pairs.
     *
     *  Convenience overload: assigns sequential ids 0, 1, 2, ...
     *
     *  @param ranges  Array of (l, r) pairs (inclusive, 0-based).
     *  @return Array of answers indexed by original query order.
     *  @throw std::out_of_range if any query is out of bounds.
     */
    Array<answer_type> solve(
        const Array<std::pair<size_t, size_t>> & ranges) const
    {
      auto queries = Array<Mo_Query>::create(ranges.size());
      for (size_t i = 0; i < ranges.size(); ++i)
        queries(i) = {ranges(i).first, ranges(i).second, i};
      return solve(queries);
    }

    /** @brief Solve queries given as (l, r) pairs (initializer list).
     *
     *  @param il  initializer list of (l, r) pairs.
     *  @return Array of answers indexed by original query order.
     */
    Array<answer_type> solve(
        std::initializer_list<std::pair<size_t, size_t>> il) const
    {
      Array<std::pair<size_t, size_t>> ranges(il);
      return solve(ranges);
    }

    /** @brief Core solver: answer all Mo_Query objects.
     *
     *  The queries are sorted by block (snake-optimized), swept with
     *  a sliding window, and the answers are returned in original order.
     *
     *  @param queries  Array of Mo_Query (l, r, id).
     *  @return Array of answers indexed by original query id.
     *  @throw std::out_of_range if any query is out of bounds.
     */
    Array<answer_type> solve(Array<Mo_Query> queries) const
    {
      const size_t n = data_.size();
      const size_t q = queries.size();

      if (q == 0)
        return Array<answer_type>();

      // Validate all queries
      for (size_t i = 0; i < q; ++i)
        {
          ah_out_of_range_error_if(queries(i).r >= n)
            << "Gen_Mo_Algorithm::solve: query " << i
            << " r=" << queries(i).r << " >= n=" << n;
          ah_out_of_range_error_if(queries(i).l > queries(i).r)
            << "Gen_Mo_Algorithm::solve: query " << i
            << " l=" << queries(i).l << " > r=" << queries(i).r;
        }

      const size_t block = std::max<size_t>(1, (size_t)std::sqrt((double)n));

      // Snake sort: primary by l/block, secondary alternating by r
      std::sort(&queries(0), &queries(0) + q,
                [block](const Mo_Query & a, const Mo_Query & b)
                {
                  const size_t ba = a.l / block;
                  const size_t bb = b.l / block;
                  if (ba != bb)
                    return ba < bb;
                  // Even blocks: r ascending; odd blocks: r descending
                  return (ba & 1) ? (a.r > b.r) : (a.r < b.r);
                });

      pol_.init(data_, n);

      auto answers = Array<answer_type>::create(q);

      // Initialise window to first query
      size_t cur_l = queries(0).l;
      size_t cur_r = queries(0).l;
      pol_.add(data_, cur_l);

      // Expand to first query's right endpoint
      while (cur_r < queries(0).r)
        pol_.add(data_, ++cur_r);

      answers(queries(0).id) = pol_.answer();

      // Sweep remaining queries
      for (size_t i = 1; i < q; ++i)
        {
          const size_t ql = queries(i).l;
          const size_t qr = queries(i).r;

          // Expand right
          while (cur_r < qr)
            pol_.add(data_, ++cur_r);

          // Expand left
          while (cur_l > ql)
            pol_.add(data_, --cur_l);

          // Shrink right
          while (cur_r > qr)
            pol_.remove(data_, cur_r--);

          // Shrink left
          while (cur_l < ql)
            pol_.remove(data_, cur_l++);

          answers(queries(i).id) = pol_.answer();
        }

      return answers;
    }
  };

  // ================================================================
  // Built-in policies
  // ================================================================

  /** @brief Policy: count distinct elements in a range.
   *
   *  Maintains a frequency map and a running distinct count.
   *  add: if freq goes from 0 to 1, increment count.
   *  remove: if freq goes from 1 to 0, decrement count.
   *
   *  @tparam T  Element type (must be hashable).
   *  @ingroup Sequences
   */
  template <typename T>
  struct Distinct_Count_Policy
  {
    using answer_type = size_t;

    std::unordered_map<T, size_t> freq;
    size_t distinct = 0;

    void init(const Array<T> &, size_t)
    {
      freq.clear();
      distinct = 0;
    }

    void add(const Array<T> & data, size_t idx)
    {
      if (++freq[data(idx)] == 1)
        ++distinct;
    }

    void remove(const Array<T> & data, size_t idx)
    {
      if (--freq[data(idx)] == 0)
        --distinct;
    }

    answer_type answer() const { return distinct; }
  };

  /** @brief Policy: "powerful array" sum = sum(cnt[x]^2 * x).
   *
   *  For each element x in the current window with count c,
   *  the contribution is c^2 * x.  We maintain the sum
   *  incrementally:
   *    add x:    sum += (2*cnt+1)*x, then cnt++
   *    remove x: cnt--, then sum -= (2*cnt+1)*x
   *
   *  @tparam T  Element type (must be hashable and arithmetic-like).
   *  @ingroup Sequences
   */
  template <typename T>
  struct Powerful_Array_Policy
  {
    using answer_type = long long;

    std::unordered_map<T, long long> cnt;
    long long sum = 0;

    void init(const Array<T> &, size_t)
    {
      cnt.clear();
      sum = 0;
    }

    void add(const Array<T> & data, size_t idx)
    {
      const auto x = static_cast<long long>(data(idx));
      sum += (2 * cnt[data(idx)] + 1) * x;
      ++cnt[data(idx)];
    }

    void remove(const Array<T> & data, size_t idx)
    {
      const auto x = static_cast<long long>(data(idx));
      --cnt[data(idx)];
      sum -= (2 * cnt[data(idx)] + 1) * x;
    }

    answer_type answer() const { return sum; }
  };

  /** @brief Policy: range mode (most frequent element).
   *
   *  Tracks frequencies and uses a "count of counts" trick for O(1)
   *  mode maintenance.  Returns {frequency, value} of the mode.
   *  Ties are broken arbitrarily.
   *
   *  @tparam T  Element type (must be hashable).
   *  @ingroup Sequences
   */
  template <typename T>
  struct Range_Mode_Policy
  {
    using answer_type = std::pair<size_t, T>;

    std::unordered_map<T, size_t> freq;
    std::unordered_map<size_t, size_t> cnt_of_cnt;
    size_t max_freq = 0;
    T mode_val = T();

    void init(const Array<T> &, size_t)
    {
      freq.clear();
      cnt_of_cnt.clear();
      max_freq = 0;
      mode_val = T();
    }

    void add(const Array<T> & data, size_t idx)
    {
      const T & x = data(idx);
      size_t old_f = freq[x];
      if (old_f > 0)
        --cnt_of_cnt[old_f];
      ++freq[x];
      ++cnt_of_cnt[old_f + 1];
      if (old_f + 1 > max_freq)
        {
          max_freq = old_f + 1;
          mode_val = x;
        }
    }

    void remove(const Array<T> & data, size_t idx)
    {
      const T & x = data(idx);
      size_t old_f = freq[x];
      --cnt_of_cnt[old_f];
      if (old_f > 1)
        ++cnt_of_cnt[old_f - 1];
      --freq[x];
      if (old_f == max_freq and cnt_of_cnt[old_f] == 0)
        {
          --max_freq;
          // Find a value with the new max frequency; if max_freq is 0,
          // the window is empty.
          if (max_freq > 0)
            {
              for (auto & [val, f] : freq)
                if (f == max_freq)
                  {
                    mode_val = val;
                    break;
                  }
            }
        }
    }

    answer_type answer() const { return {max_freq, mode_val}; }
  };

  // ================================================================
  // Convenient typedefs
  // ================================================================

  /** @brief Mo's algorithm specialised for counting distinct elements.
   *  @tparam T  Element type (must be hashable).
   *  @ingroup Sequences
   */
  template <typename T>
  using Distinct_Count_Mo = Gen_Mo_Algorithm<T, Distinct_Count_Policy<T>>;

  /** @brief Mo's algorithm specialised for the "powerful array" query.
   *  @tparam T  Arithmetic element type.
   *  @ingroup Sequences
   */
  template <typename T>
  using Powerful_Array_Mo = Gen_Mo_Algorithm<T, Powerful_Array_Policy<T>>;

  /** @brief Mo's algorithm specialised for range mode queries.
   *  @tparam T  Element type (must be hashable).
   *  @ingroup Sequences
   */
  template <typename T>
  using Range_Mode_Mo = Gen_Mo_Algorithm<T, Range_Mode_Policy<T>>;

} // namespace Aleph

# endif // TPL_MO_ALGORITHM_H
