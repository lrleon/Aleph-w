
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


/** @file tpl_disjoint_sparse_table.H
 *  @brief Disjoint Sparse Table for static range queries in O(1).
 *
 *  A Disjoint Sparse Table preprocesses a fixed array of `n` elements
 *  over an **associative** binary operation (not necessarily idempotent)
 *  and answers arbitrary range queries in O(1) time after an O(n log n)
 *  build phase.
 *
 *  Unlike the classical Sparse Table (which requires idempotency), the
 *  disjoint variant works for **any associative** operation: addition,
 *  multiplication, XOR, matrix multiplication, string concatenation,
 *  modular arithmetic, etc.
 *
 *  The technique decomposes the array into disjoint blocks at each
 *  level and stores prefix/suffix aggregates from each block's midpoint.
 *  Because the sub-ranges never overlap, no idempotency is needed.
 *
 *  Three class templates are provided:
 *
 *  - **Gen_Disjoint_Sparse_Table\<T, Op\>** — fully generic over any
 *    associative binary functor `Op`.
 *  - **Sum_Disjoint_Sparse_Table\<T\>** — convenient specialisation
 *    for range sum queries using `Aleph::plus<T>`.
 *  - **Product_Disjoint_Sparse_Table\<T\>** — convenient specialisation
 *    for range product queries using `Aleph::multiplies<T>`.
 *
 *  ## Complexity
 *  | Operation     | Time       |
 *  |--------------|------------|
 *  | Construction  | O(n log n) |
 *  | query         | O(1)       |
 *  | get           | O(1)       |
 *  | Space         | O(n log n) |
 *
 *  ## Comparison with classical Sparse Table
 *  | Feature                | Sparse Table        | Disjoint Sparse Table |
 *  |------------------------|---------------------|-----------------------|
 *  | Operation requirement  | Assoc. + idempotent | Associative only      |
 *  | Works for sum/product  | No                  | **Yes**               |
 *  | Works for min/max      | Yes                 | Yes                   |
 *  | Query time             | O(1)                | O(1)                  |
 *  | Build time             | O(n log n)          | O(n log n)            |
 *
 *  @note The underlying array is **immutable** after construction.
 *        If point updates are needed, use a Segment Tree or Fenwick
 *        Tree instead.
 *
 *  @see tpl_sparse_table.H  Classical sparse table (idempotent ops).
 *  @see tpl_fenwick_tree.H  Fenwick tree (dynamic, prefix queries).
 *  @see https://discuss.codechef.com/t/tutorial-disjoint-sparse-table/17404
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DISJOINT_SPARSE_TABLE_H
# define TPL_DISJOINT_SPARSE_TABLE_H

# include <bit>
# include <cassert>
# include <cstdlib>
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
  /** @brief Binary operation compatible with Disjoint Sparse Table queries.
   *
   *  Requires the functor to be callable with two immutable values
   *  of type @p T and return a value convertible to @p T.
   *
   *  @note This concept checks only the call signature.  For Disjoint
   *  Sparse Table correctness, @p F must satisfy **associativity**:
   *  `f(f(a, b), c) == f(a, f(b, c))`.  Idempotency is **not** required.
   */
  template <typename F, typename T>
  concept DisjointSparseTableOp =
      requires(const F& f, const T& a, const T& b)
        {
          { f(a, b) } -> std::convertible_to<T>;
        };

  /** @brief Disjoint Sparse Table over an arbitrary associative binary
   *         operation.
   *
   *  The structure decomposes the array into disjoint blocks at each
   *  level and stores prefix/suffix aggregates from each block's
   *  midpoint.  For a query `[l, r]` with `l != r`, the answer is
   *  obtained by combining the suffix aggregate ending at `l` with the
   *  prefix aggregate starting at `r`, both relative to the block
   *  midpoint at the appropriate level:
   *  \f[
   *    \text{query}(l, r) = \text{Op}\bigl(
   *      \text{suffix}[k][l],\;
   *      \text{prefix}[k][r]
   *    \bigr),
   *    \quad k = \lfloor \log_2(l \oplus r) \rfloor
   *  \f]
   *
   *  Because the sub-ranges are **disjoint** (non-overlapping), the
   *  operation need not be idempotent — only associative.
   *
   *  @tparam T  element type.
   *  @tparam Op associative binary functor.
   *
   *  @par Example
   *  @code
   *  // Range-sum disjoint sparse table
   *  Gen_Disjoint_Sparse_Table<int, Aleph::plus<int>> st = {3, 1, 4, 1, 5};
   *  int s = st.query(1, 3);  // 1 + 4 + 1 = 6
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Op>
    requires DisjointSparseTableOp<Op, T>
  class Gen_Disjoint_Sparse_Table
  {
    Array<T> data;         // original values: data[i] = a[i]
    Array<T> table;        // flattened 2D: table[k * n + i]
    size_t n = 0;          // number of logical elements
    size_t levels = 0;     // number of levels

    Op op;

    /// Access table[k][i] (0-based row k, 0-based column i).
    T & at(const size_t k, const size_t i) { return table(k * n + i); }
    const T & at(const size_t k, const size_t i) const
    {
      return table(k * n + i);
    }

    /** @brief Compute the number of levels for n elements.
     *
     *  For n <= 1 no levels are needed (single-element queries use data
     *  directly).  For n >= 2, levels = bit_width(n - 1).
     */
    static constexpr size_t compute_levels(const size_t nn) noexcept
    {
      if (nn <= 1) return 0;
      return static_cast<size_t>(std::bit_width(nn - 1));
    }

    /** @brief Build the disjoint sparse table from the data array.
     *
     *  For each level k (block size 2^(k+1), half = 2^k):
     *    - Compute suffix aggregates from midpoint-1 going left.
     *    - Compute prefix aggregates from midpoint going right.
     */
    void build()
    {
      if (n <= 1)
        return;

      for (size_t k = 0; k < levels; ++k)
        {
          const size_t half = size_t{1} << k;
          const size_t block_sz = half << 1;

          for (size_t b = 0; b < n; b += block_sz)
            {
              const size_t mid = b + half;
              if (mid > n) break;

              // Suffix aggregates: from mid-1 going left to b
              at(k, mid - 1) = data(mid - 1);
              for (size_t i = mid - 1; i > b; --i)
                at(k, i - 1) = op(data(i - 1), at(k, i));

              // Prefix aggregates: from mid-going right
              if (mid < n)
                {
                  at(k, mid) = data(mid);
                  const size_t right_end = std::min(b + block_sz, n);
                  for (size_t i = mid + 1; i < right_end; ++i)
                    at(k, i) = op(at(k, i - 1), data(i));
                }
            }
        }
    }

    /// Fill data array from a 0-based indexed getter.
    template <class Getter>
    void fill_data(Getter getter)
    {
      for (size_t i = 0; i < n; ++i)
        data(i) = getter(i);
    }

    /// Fill data array from an Aleph-style iterator.
    template <class AlephIt>
    void fill_data_from_aleph_it(AlephIt it)
    {
      size_t i = 0;
      for (; it.has_curr(); it.next_ne())
        data(i++) = it.get_curr();
    }

  public:
    /// The type of the element stored in the table.
    using Item_Type = T;

    /** @brief Construct a disjoint sparse table with @p num elements,
     *  all equal to @p init_val.
     *
     *  @param num      number of elements.
     *  @param init_val value for every position.
     *  @param oper     associative binary functor.
     */
    Gen_Disjoint_Sparse_Table(const size_t num, const T & init_val,
                              Op oper = Op())
      : data(Array<T>::create(std::max(num, size_t{1}))),
        table(Array<T>::create(
          std::max(compute_levels(num) * num, size_t{1}))),
        n(num), levels(compute_levels(num)), op(oper)
    {
      fill_data([&init_val](size_t) { return init_val; });
      build();
    }

    /** @brief Construct from an initializer list in O(n log n) time.
     *
     *  @par Example
     *  @code
     *  Gen_Disjoint_Sparse_Table<int, Aleph::plus<int>> st = {3, 1, 4, 1, 5};
     *  assert(st.query(0, 4) == 14);  // 3 + 1 + 4 + 1 + 5
     *  @endcode
     */
    Gen_Disjoint_Sparse_Table(std::initializer_list<T> il, Op oper = Op())
      : data(Array<T>::create(std::max(il.size(), size_t{1}))),
        table(Array<T>::create(
          std::max(compute_levels(il.size()) * il.size(), size_t{1}))),
        n(il.size()), levels(compute_levels(il.size())), op(oper)
    {
      auto it = il.begin();
      fill_data([&it](size_t) { return *it++; });
      build();
    }

    /** @brief Construct from an Array\<T\> in O(n log n) time.
     *
     *  @param values source array with the initial element values.
     *  @param oper   associative binary functor.
     */
    Gen_Disjoint_Sparse_Table(const Array<T> & values, Op oper = Op())
      : data(Array<T>::create(std::max(values.size(), size_t{1}))),
        table(Array<T>::create(
          std::max(compute_levels(values.size()) * values.size(),
                   size_t{1}))),
        n(values.size()), levels(compute_levels(values.size())), op(oper)
    {
      fill_data([&values](size_t i) { return values(i); });
      build();
    }

    /** @brief Construct from a std::vector\<T\> in O(n log n) time.
     *
     *  @param values source vector with the initial element values.
     *  @param oper   associative binary functor.
     */
    Gen_Disjoint_Sparse_Table(const std::vector<T> & values,
                              Op oper = Op())
      : data(Array<T>::create(std::max(values.size(), size_t{1}))),
        table(Array<T>::create(
          std::max(compute_levels(values.size()) * values.size(),
                   size_t{1}))),
        n(values.size()), levels(compute_levels(values.size())), op(oper)
    {
      fill_data([&values](size_t i) { return values[i]; });
      build();
    }

    /** @brief Construct from a DynList\<T\> in O(n log n) time.
     *
     *  @param values source list with the initial element values.
     *  @param oper   associative binary functor.
     */
    Gen_Disjoint_Sparse_Table(const DynList<T> & values, Op oper = Op())
      : data(Array<T>::create(std::max(values.size(), size_t{1}))),
        table(Array<T>::create(
          std::max(compute_levels(values.size()) * values.size(),
                   size_t{1}))),
        n(values.size()), levels(compute_levels(values.size())), op(oper)
    {
      fill_data_from_aleph_it(values.get_it());
      build();
    }

    Gen_Disjoint_Sparse_Table(const Gen_Disjoint_Sparse_Table &) = default;

    Gen_Disjoint_Sparse_Table(Gen_Disjoint_Sparse_Table &&) noexcept
      = default;

    Gen_Disjoint_Sparse_Table & operator=(const Gen_Disjoint_Sparse_Table &)
      = default;

    Gen_Disjoint_Sparse_Table & operator=(Gen_Disjoint_Sparse_Table &&)
      noexcept = default;

    /** @brief Range query over `[l, r]` in O(1).
     *
     *  Returns `Op(a[l], a[l+1], ..., a[r])`.
     *
     *  @note Correctness requires @p Op to be associative.
     *        Idempotency is **not** required.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @return  result of applying `Op` over the range.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Gen_Disjoint_Sparse_Table::query: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Gen_Disjoint_Sparse_Table::query: l=" << l << " > r=" << r;

      if (l == r)
        return data(l);

      const size_t k = static_cast<size_t>(std::bit_width(l ^ r)) - 1;
      return op(at(k, l), at(k, r));
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
        << "Gen_Disjoint_Sparse_Table::get: index " << i
        << " >= size " << n;

      return data(i);
    }

    /** @brief Number of logical elements. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n; }

    /** @brief True if the table contains no elements. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0; }

    /** @brief Number of precomputed levels. */
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
        ret(i) = data(i);
      return ret;
    }

    /** @brief Swap this table with @p other in O(1). */
    void swap(Gen_Disjoint_Sparse_Table & other) noexcept
    {
      data.swap(other.data);
      table.swap(other.table);
      std::swap(n, other.n);
      std::swap(levels, other.levels);
      std::swap(op, other.op);
    }
  };


  /** @brief Disjoint Sparse Table for range sum queries.
   *
   *  Convenient specialisation of Gen_Disjoint_Sparse_Table using
   *  `Aleph::plus<T>` for O(1) range sum queries on a static array.
   *
   *  @tparam T an arithmetic type supporting `operator+`.
   *
   *  @par Example
   *  @code
   *  Sum_Disjoint_Sparse_Table<int> st = {3, 1, 4, 1, 5, 9};
   *  assert(st.query(0, 5) == 23);   // 3+1+4+1+5+9
   *  assert(st.query(2, 4) == 10);   // 4+1+5
   *  assert(st.query(3, 3) == 1);    // single element
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T>
  struct Sum_Disjoint_Sparse_Table
      : public Gen_Disjoint_Sparse_Table<T, Aleph::plus<T>>
  {
    using Base = Gen_Disjoint_Sparse_Table<T, Aleph::plus<T>>;
    using Base::Base; // inherit all constructors
  };

  /** @brief Disjoint Sparse Table for range product queries.
   *
   *  Convenient specialisation of Gen_Disjoint_Sparse_Table using
   *  `Aleph::multiplies<T>` for O(1) range product queries on a
   *  static array.
   *
   *  @tparam T an arithmetic type supporting `operator*`.
   *
   *  @par Example
   *  @code
   *  Product_Disjoint_Sparse_Table<long long> st = {2, 3, 5, 7, 11};
   *  assert(st.query(0, 4) == 2310);  // 2*3*5*7*11
   *  assert(st.query(1, 3) == 105);   // 3*5*7
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T>
  struct Product_Disjoint_Sparse_Table
      : public Gen_Disjoint_Sparse_Table<T, Aleph::multiplies<T>>
  {
    using Base = Gen_Disjoint_Sparse_Table<T, Aleph::multiplies<T>>;
    using Base::Base; // inherit all constructors
  };

} // end namespace Aleph

# endif /* TPL_DISJOINT_SPARSE_TABLE_H */
