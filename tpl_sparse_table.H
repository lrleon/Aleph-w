
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


/** @file tpl_sparse_table.H
 *  @brief Sparse Table for static range queries in O(1).
 *
 *  A Sparse Table preprocesses a fixed array of `n` elements over an
 *  **idempotent** binary operation (one satisfying `Op(a, a) == a`,
 *  such as min, max, gcd, bitwise AND/OR) and answers arbitrary range
 *  queries in O(1) time after an O(n log n) build phase.
 *
 *  Three class templates are provided:
 *
 *  - **Gen_Sparse_Table\<T, Op\>** — fully generic over any idempotent
 *    binary functor `Op`.
 *  - **Sparse_Table\<T\>** — convenient specialisation for range
 *    minimum queries using the `Min_Op` functor.
 *  - **Max_Sparse_Table\<T\>** — convenient specialisation for range
 *    maximum queries using the `Max_Op` functor.
 *
 *  ## Complexity
 *  | Operation     | Time       |
 *  |--------------|------------|
 *  | Construction  | O(n log n) |
 *  | query         | O(1)       |
 *  | get           | O(1)       |
 *  | Space         | O(n log n) |
 *
 *  @note The underlying array is **immutable** after construction.
 *        If point updates are needed, use a Segment Tree or Fenwick
 *        Tree instead.
 *
 *  @see tpl_fenwick_tree.H  Fenwick tree (dynamic, prefix queries).
 *  @see https://en.wikipedia.org/wiki/Sparse_table
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SPARSE_TABLE_H
# define TPL_SPARSE_TABLE_H

# include <bit>
# include <cassert>
# include <concepts>
# include <initializer_list>
# include <type_traits>
# include <vector>
# include <utility>
# include <algorithm>
# include <tpl_array.H>
# include <tpl_dynList.H>
# include <ahFunction.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Binary operation compatible with Sparse Table queries.
   *
   *  Requires the functor to be callable with two immutable values
   *  of type @p T and return a value convertible to @p T.  The functor
   *  must be **idempotent**: `Op(a, a) == a` for all `a`.
   */
  template <typename F, typename T>
  concept SparseTableOp =
      requires(const F& f, const T& a, const T& b)
        {
          { f(a, b) } -> std::convertible_to<T>;
        };

  /** @brief Functor returning the minimum of two values.
   *
   *  Idempotent binary operation for range-minimum queries.
   *
   *  @tparam T a totally ordered type.
   *  @ingroup Functors
   */
  template <std::totally_ordered T>
  struct Min_Op
  {
    constexpr T operator()(const T & a, const T & b) const noexcept
    {
      return a <= b ? a : b;
    }
  };

  /** @brief Functor returning the maximum of two values.
   *
   *  Idempotent binary operation for range-maximum queries.
   *
   *  @tparam T a totally ordered type.
   *  @ingroup Functors
   */
  template <std::totally_ordered T>
  struct Max_Op
  {
    constexpr T operator()(const T & a, const T & b) const noexcept
    {
      return a >= b ? a : b;
    }
  };

  /** @brief Sparse Table over an arbitrary idempotent binary operation.
   *
   *  Maintains a precomputed table `table[k][i]` storing the result of
   *  applying `Op` over the sub-array `a[i .. i + 2^k - 1]`.  Since the
   *  operation is idempotent, overlapping sub-ranges can be combined in
   *  O(1):
   *  \f[
   *    \text{query}(l, r) = \text{Op}\bigl(
   *      \text{table}[k][l],\;
   *      \text{table}[k][r - 2^k + 1]
   *    \bigr),
   *    \quad k = \lfloor \log_2(r - l + 1) \rfloor
   *  \f]
   *
   *  @tparam T  element type.
   *  @tparam Op idempotent binary functor.
   *
   *  @par Example
   *  @code
   *  // Range-GCD sparse table
   *  struct Gcd {
   *    int operator()(int a, int b) const { return std::gcd(a, b); }
   *  };
   *  Gen_Sparse_Table<int, Gcd> st = {12, 6, 9, 3, 15};
   *  int g = st.query(1, 3);  // gcd(6, 9, 3) = 3
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Op>
    requires SparseTableOp<Op, T>
  class Gen_Sparse_Table
  {
    Array<T> table;        // flattened 2D: table[k * n + i]
    Array<size_t> log_tbl; // log_tbl[len] = floor(log2(len)) for len in [1..n]
    size_t n = 0;          // number of logical elements
    size_t levels = 0;     // number of levels = floor(log2(n)) + 1

    Op op;

    /// Access table[k][i] (0-based row k, 0-based column i).
    T & at(size_t k, size_t i) { return table(k * n + i); }
    const T & at(size_t k, size_t i) const { return table(k * n + i); }

    /// Compute levels = floor(log2(n)) + 1; 0 if n == 0.
    static constexpr size_t compute_levels(size_t nn) noexcept
    {
      return nn == 0 ? 0 : static_cast<size_t>(std::bit_width(nn));
    }

    /// Precompute the log lookup table for lengths [0..n].
    void build_log_table()
    {
      if (n == 0)
        return;

      log_tbl = Array<size_t>::create(n + 1);
      log_tbl(0) = 0;
      log_tbl(1) = 0;
      for (size_t i = 2; i <= n; ++i)
        log_tbl(i) = log_tbl(i / 2) + 1;
    }

    /// Fill level 0 from a 0-based indexed getter and build higher levels.
    template <class Getter>
    void fill_and_build(Getter getter)
    {
      // Level 0: intervals of length 1
      for (size_t i = 0; i < n; ++i)
        at(0, i) = getter(i);

      // Higher levels: table[k][i] = Op(table[k-1][i], table[k-1][i + 2^(k-1)])
      for (size_t k = 1; k < levels; ++k)
        {
          const size_t half = size_t{1} << (k - 1);
          const size_t limit = n - (size_t{1} << k) + 1;
          for (size_t i = 0; i < limit; ++i)
            at(k, i) = op(at(k - 1, i), at(k - 1, i + half));
        }
    }

    /// Fill level 0 from an Aleph-style iterator and build higher levels.
    template <class AlephIt>
    void fill_from_aleph_it(AlephIt it)
    {
      size_t i = 0;
      for (; it.has_curr(); it.next_ne())
        at(0, i++) = it.get_curr();

      for (size_t k = 1; k < levels; ++k)
        {
          const size_t half = size_t{1} << (k - 1);
          const size_t limit = n - (size_t{1} << k) + 1;
          for (size_t i = 0; i < limit; ++i)
            at(k, i) = op(at(k - 1, i), at(k - 1, i + half));
        }
    }

  public:
    /// The type of the element stored in the table.
    using Item_Type = T;

    /** @brief Construct a sparse table with @p num elements, all equal
     *  to @p init_val.
     *
     *  @param num      number of elements.
     *  @param init_val value for every position.
     *  @param oper     idempotent binary functor.
     */
    Gen_Sparse_Table(const size_t num, const T & init_val,
                     Op oper = Op())
      : table(num == 0 ? 1 : compute_levels(num) * num, T()),
        n(num), levels(compute_levels(num)), op(oper)
    {
      build_log_table();
      if (n > 0)
        fill_and_build([&init_val](size_t) { return init_val; });
    }

    /** @brief Construct from an initializer list in O(n log n) time.
     *
     *  @par Example
     *  @code
     *  Gen_Sparse_Table<int, Min_Op<int>> st = {3, 1, 4, 1, 5, 9};
     *  assert(st.query(0, 3) == 1);  // min(3, 1, 4, 1)
     *  @endcode
     */
    Gen_Sparse_Table(std::initializer_list<T> il, Op oper = Op())
      : table(il.size() == 0 ? 1 : compute_levels(il.size()) * il.size(), T()),
        n(il.size()), levels(compute_levels(il.size())), op(oper)
    {
      build_log_table();
      if (n > 0)
        {
          auto it = il.begin();
          fill_and_build([&it](size_t) { return *it++; });
        }
    }

    /** @brief Construct from an Array\<T\> in O(n log n) time.
     *
     *  @param values source array with the initial element values.
     *  @param oper   idempotent binary functor.
     */
    Gen_Sparse_Table(const Array<T> & values, Op oper = Op())
      : table(values.size() == 0 ? 1 :
              compute_levels(values.size()) * values.size(), T()),
        n(values.size()), levels(compute_levels(values.size())), op(oper)
    {
      build_log_table();
      if (n > 0)
        fill_and_build([&values](size_t i) { return values(i); });
    }

    /** @brief Construct from a std::vector\<T\> in O(n log n) time.
     *
     *  @param values source vector with the initial element values.
     *  @param oper   idempotent binary functor.
     */
    Gen_Sparse_Table(const std::vector<T> & values, Op oper = Op())
      : table(values.size() == 0 ? 1 :
              compute_levels(values.size()) * values.size(), T()),
        n(values.size()), levels(compute_levels(values.size())), op(oper)
    {
      build_log_table();
      if (n > 0)
        fill_and_build([&values](size_t i) { return values[i]; });
    }

    /** @brief Construct from a DynList\<T\> in O(n log n) time.
     *
     *  @param values source list with the initial element values.
     *  @param oper   idempotent binary functor.
     */
    Gen_Sparse_Table(const DynList<T> & values, Op oper = Op())
      : table(values.size() == 0 ? 1 :
              compute_levels(values.size()) * values.size(), T()),
        n(values.size()), levels(compute_levels(values.size())), op(oper)
    {
      build_log_table();
      if (n > 0)
        fill_from_aleph_it(values.get_it());
    }

    Gen_Sparse_Table(const Gen_Sparse_Table &) = default;

    Gen_Sparse_Table(Gen_Sparse_Table &&) noexcept = default;

    Gen_Sparse_Table & operator=(const Gen_Sparse_Table &) = default;

    Gen_Sparse_Table & operator=(Gen_Sparse_Table &&) noexcept = default;

    /** @brief Range query over `[l, r]` in O(1).
     *
     *  Returns `Op(a[l], a[l+1], ..., a[r])`.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @return  result of applying `Op` over the range.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Gen_Sparse_Table::query: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Gen_Sparse_Table::query: l=" << l << " > r=" << r;

      const size_t k = log_tbl(r - l + 1);
      return op(at(k, l), at(k, r - (size_t{1} << k) + 1));
    }

    /** @brief Retrieve the value `a[i]` in O(1).
     *
     *  Equivalent to `query(i, i)`.
     *
     *  @param i 0-based index.
     *  @throw std::out_of_range if `i >= size()`.
     */
    T get(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Sparse_Table::get: index " << i << " >= size " << n;

      return at(0, i);
    }

    /** @brief Number of logical elements. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n; }

    /** @brief True if the table contains no elements. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0; }

    /** @brief Number of precomputed levels (floor(log2(n)) + 1). */
    [[nodiscard]] constexpr size_t num_levels() const noexcept
    {
      return levels;
    }

    /** @brief Reconstruct all original values into an Array.
     *
     *  O(n).
     */
    Array<T> values() const
    {
      auto ret = Array<T>::create(n);
      for (size_t i = 0; i < n; ++i)
        ret(i) = at(0, i);
      return ret;
    }

    /** @brief Swap this table with @p other in O(1). */
    void swap(Gen_Sparse_Table & other) noexcept
    {
      table.swap(other.table);
      log_tbl.swap(other.log_tbl);
      std::swap(n, other.n);
      std::swap(levels, other.levels);
      std::swap(op, other.op);
    }
  };


  /** @brief Sparse Table for range minimum queries.
   *
   *  Convenient specialisation of Gen_Sparse_Table using `Min_Op<T>`
   *  for O(1) range minimum queries on a static array.
   *
   *  @tparam T a totally ordered type.
   *
   *  @par Example
   *  @code
   *  Sparse_Table<int> st = {5, 2, 4, 7, 1, 3, 6};
   *  assert(st.query(0, 3) == 2);  // min(5, 2, 4, 7)
   *  assert(st.query(2, 6) == 1);  // min(4, 7, 1, 3, 6)
   *  assert(st.query(4, 4) == 1);  // min(1)
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Sparse_Table
      : public Gen_Sparse_Table<T, Min_Op<T>>
  {
    using Base = Gen_Sparse_Table<T, Min_Op<T>>;
    using Base::Base; // inherit all constructors
  };

  /** @brief Sparse Table for range maximum queries.
   *
   *  Convenient specialisation of Gen_Sparse_Table using `Max_Op<T>`
   *  for O(1) range maximum queries on a static array.
   *
   *  @tparam T a totally ordered type.
   *
   *  @par Example
   *  @code
   *  Max_Sparse_Table<int> st = {5, 2, 4, 7, 1, 3, 6};
   *  assert(st.query(0, 3) == 7);  // max(5, 2, 4, 7)
   *  assert(st.query(2, 6) == 7);  // max(4, 7, 1, 3, 6)
   *  assert(st.query(5, 6) == 6);  // max(3, 6)
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Max_Sparse_Table
      : public Gen_Sparse_Table<T, Max_Op<T>>
  {
    using Base = Gen_Sparse_Table<T, Max_Op<T>>;
    using Base::Base; // inherit all constructors
  };

} // end namespace Aleph

# endif /* TPL_SPARSE_TABLE_H */
