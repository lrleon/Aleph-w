
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


/** @file tpl_segment_tree.H
 *  @brief Segment trees for dynamic range queries with lazy propagation.
 *
 *  This file provides three segment tree variants:
 *
 *  - **Gen_Segment_Tree\<T, Op\>** — Point update + range query over
 *    an arbitrary associative binary operation (monoid).  Does not
 *    require idempotency or invertibility.
 *  - **Gen_Lazy_Segment_Tree\<Policy\>** — Range update + range query
 *    with lazy propagation, parameterised by a policy that bundles
 *    value type, lazy type, combine, apply, and compose operations.
 *  - **Segment_Tree_Beats\<T\>** — Ji Driver's Segment Tree supporting
 *    range chmin/chmax with sum/min/max queries.
 *
 *  ## Complexity
 *  | Operation     | Gen_Segment_Tree | Gen_Lazy_Segment_Tree | Segment_Tree_Beats |
 *  |--------------|------------------|----------------------|-------------------|
 *  | Build         | O(n)             | O(n)                  | O(n)               |
 *  | Point update  | O(log n)         | O(log n)              | O(log n)           |
 *  | Range update  | —                | O(log n)              | O(log n) amort.    |
 *  | Query         | O(log n)         | O(log n)              | O(log n)           |
 *  | Space         | O(n)             | O(n)                  | O(n)               |
 *
 *  @see tpl_sparse_table.H    Static O(1) range queries (idempotent ops).
 *  @see tpl_fenwick_tree.H    Prefix-based range queries (invertible ops).
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SEGMENT_TREE_H
# define TPL_SEGMENT_TREE_H

# include <cassert>
# include <concepts>
# include <algorithm>
# include <initializer_list>
# include <limits>
# include <type_traits>
# include <utility>
# include <vector>
# include <tpl_array.H>
# include <tpl_dynList.H>
# include <ahFunction.H>
# include <ah-errors.H>
# include <tpl_sparse_table.H>

namespace Aleph
{
  /** @brief Binary operation compatible with Segment Tree queries.
   *
   *  This concept is used by `Gen_Segment_Tree` to constrain the binary
   *  operation passed as template parameter.
   *
   *  @tparam F Binary functor type.
   *  @tparam T Value type.
   *
   *  @par Requirements
   *  - `f(a, b)` must be well-formed for `const T& a, const T& b`.
   *  - The result must be convertible to `T`.
   *
   *  @note This concept only checks the call signature. For correctness
   *  of range queries, `F` must define an **associative** operation and
   *  the caller must provide a correct identity element where required.
   *
   *  @ingroup Sequences
   */
  template <typename F, typename T>
  concept SegmentTreeOp =
      requires(const F& f, const T& a, const T& b)
        {
          { f(a, b) } -> std::convertible_to<T>;
        };

  // ================================================================
  // Gen_Segment_Tree — Point update + Range query
  // ================================================================

  /** @brief Segment tree over an arbitrary associative binary operation.
   *
   *  This is the classical segment tree supporting:
   *
   *  - Point updates in O(log n)
   *  - Range queries in O(log n)
   *
   *  It maintains an implicit binary tree backed by `Array<T>`,
   *  using 1-based node indexing and allocating roughly `4*n` nodes.
   *
   *  Unlike Sparse Table, the operation need not be idempotent.
   *  Unlike Fenwick Tree, the operation need not be invertible.
   *  Only associativity is required.
   *
   *  @par Identity element
   *  Queries rely on an identity element `id` such that
   *  `op(id, x) == x` and `op(x, id) == x`.
   *
   *  @par Update semantics
   *  `update(i, delta)` applies `a[i] = op(a[i], delta)`.
   *  Use `set(i, val)` if you need assignment semantics.
   *
   *  @tparam T  element type.
   *  @tparam Op associative binary functor.
   *
   *  @par Example
   *  @code
   *  // Range-sum segment tree
   *  Gen_Segment_Tree<int, Aleph::plus<int>> st = {3, 1, 4, 1, 5};
   *  st.query(1, 3);    // 1 + 4 + 1 = 6
   *  st.update(2, 10);  // a[2] += 10 => {3, 1, 14, 1, 5}
   *  st.query(1, 3);    // 1 + 14 + 1 = 16
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Op>
    requires SegmentTreeOp<Op, T>
  class Gen_Segment_Tree
  {
    Array<T> tree;       // 1-indexed implicit binary tree
    size_t n = 0;        // number of logical elements
    T identity;          // identity element of the monoid
    Op op;

    void build(size_t node, size_t start, size_t end)
    {
      if (start == end)
        return;

      const size_t mid = start + (end - start) / 2;
      build(2 * node, start, mid);
      build(2 * node + 1, mid + 1, end);
      tree(node) = op(tree(2 * node), tree(2 * node + 1));
    }

    void update_impl(size_t node, size_t start, size_t end,
                     size_t idx, const T & delta)
    {
      if (start == end)
        {
          tree(node) = op(tree(node), delta);
          return;
        }

      const size_t mid = start + (end - start) / 2;
      if (idx <= mid)
        update_impl(2 * node, start, mid, idx, delta);
      else
        update_impl(2 * node + 1, mid + 1, end, idx, delta);

      tree(node) = op(tree(2 * node), tree(2 * node + 1));
    }

    void set_impl(size_t node, size_t start, size_t end,
                  size_t idx, const T & val)
    {
      if (start == end)
        {
          tree(node) = val;
          return;
        }

      const size_t mid = start + (end - start) / 2;
      if (idx <= mid)
        set_impl(2 * node, start, mid, idx, val);
      else
        set_impl(2 * node + 1, mid + 1, end, idx, val);

      tree(node) = op(tree(2 * node), tree(2 * node + 1));
    }

    T query_impl(size_t node, size_t start, size_t end,
                 size_t l, size_t r) const
    {
      if (r < start || end < l)
        return identity;

      if (l <= start && end <= r)
        return tree(node);

      const size_t mid = start + (end - start) / 2;
      return op(query_impl(2 * node, start, mid, l, r),
                query_impl(2 * node + 1, mid + 1, end, l, r));
    }

    template <class Getter>
    void fill_and_build(Getter getter)
    {
      // Initialize entire tree with identity
      for (size_t i = 0; i < tree.size(); ++i)
        tree(i) = identity;

      // Place values in leaves via recursive descent
      fill_leaves_recursive(1, 0, n - 1, getter);

      // Build internal nodes bottom-up
      build(1, 0, n - 1);
    }

    template <class Getter>
    void fill_leaves_recursive(size_t node, size_t start, size_t end,
                               Getter & getter)
    {
      if (start == end)
        {
          tree(node) = getter(start);
          return;
        }

      const size_t mid = start + (end - start) / 2;
      fill_leaves_recursive(2 * node, start, mid, getter);
      fill_leaves_recursive(2 * node + 1, mid + 1, end, getter);
    }

    template <class AlephIt>
    void fill_from_aleph_it(AlephIt it)
    {
      auto tmp = Array<T>::create(n);
      size_t i = 0;
      for (; it.has_curr(); it.next_ne())
        tmp(i++) = it.get_curr();

      auto getter = [&tmp](size_t j) { return tmp(j); };
      fill_and_build(getter);
    }

  public:
    using Item_Type = T;

    /** @brief Construct a segment tree with @p num elements, all equal
     *  to @p init_val.
     *
     *  @param num      number of elements.
     *  @param init_val value for every position.
     *  @param id       identity element of the monoid.
     *  @param oper     associative binary functor.
     */
    Gen_Segment_Tree(const size_t num, const T & init_val,
                     const T & id, Op oper = Op())
      : tree(num == 0 ? 1 : 4 * num, id),
        n(num), identity(id), op(oper)
    {
      if (n > 0)
        fill_and_build([&init_val](size_t) { return init_val; });
    }

    /** @brief Construct from an initializer list.
     *
     *  @param il       initializer list with the element values.
     *  @param id       identity element of the monoid.
     *  @param oper     associative binary functor.
     *
     *  @par Example
     *  @code
     *  Gen_Segment_Tree<int, Aleph::plus<int>> st({3, 1, 4}, 0);
     *  assert(st.query(0, 2) == 8);  // 3 + 1 + 4
     *  @endcode
     */
    Gen_Segment_Tree(std::initializer_list<T> il,
                     const T & id, Op oper = Op())
      : tree(il.size() == 0 ? 1 : 4 * il.size(), id),
        n(il.size()), identity(id), op(oper)
    {
      if (n > 0)
        {
          auto it = il.begin();
          fill_and_build([&it](size_t) { return *it++; });
        }
    }

    /** @brief Construct from an Array\<T\>.
     *
     *  @param values source array with the initial element values.
     *  @param id     identity element of the monoid.
     *  @param oper   associative binary functor.
     */
    Gen_Segment_Tree(const Array<T> & values,
                     const T & id, Op oper = Op())
      : tree(values.size() == 0 ? 1 : 4 * values.size(), id),
        n(values.size()), identity(id), op(oper)
    {
      if (n > 0)
        fill_and_build([&values](size_t i) { return values(i); });
    }

    /** @brief Construct from a std::vector\<T\>.
     *
     *  @param values source vector with the initial element values.
     *  @param id     identity element of the monoid.
     *  @param oper   associative binary functor.
     */
    Gen_Segment_Tree(const std::vector<T> & values,
                     const T & id, Op oper = Op())
      : tree(values.size() == 0 ? 1 : 4 * values.size(), id),
        n(values.size()), identity(id), op(oper)
    {
      if (n > 0)
        fill_and_build([&values](size_t i) { return values[i]; });
    }

    /** @brief Construct from a DynList\<T\>.
     *
     *  @param values source list with the initial element values.
     *  @param id     identity element of the monoid.
     *  @param oper   associative binary functor.
     */
    Gen_Segment_Tree(const DynList<T> & values,
                     const T & id, Op oper = Op())
      : tree(values.size() == 0 ? 1 : 4 * values.size(), id),
        n(values.size()), identity(id), op(oper)
    {
      if (n > 0)
        fill_from_aleph_it(values.get_it());
    }

    Gen_Segment_Tree(const Gen_Segment_Tree &) = default;

    Gen_Segment_Tree(Gen_Segment_Tree &&) noexcept(
      std::is_nothrow_move_constructible_v<Array<T>> &&
      std::is_nothrow_move_constructible_v<Op>) = default;

    Gen_Segment_Tree & operator=(const Gen_Segment_Tree &) = default;

    Gen_Segment_Tree & operator=(Gen_Segment_Tree &&) noexcept(
      std::is_nothrow_move_assignable_v<Array<T>> &&
      std::is_nothrow_move_assignable_v<Op>) = default;

    /** @brief Point update: `a[i] = Op(a[i], delta)`.
     *
     *  @param i     0-based index.
     *  @param delta value to combine with a[i].
     *  @throw std::out_of_range if `i >= size()`.
     */
    void update(const size_t i, const T & delta)
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Segment_Tree::update: index " << i << " >= size " << n;

      update_impl(1, 0, n - 1, i, delta);
    }

    /** @brief Set `a[i] = val`.
     *
     *  @param i   0-based index.
     *  @param val new value for position i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    void set(const size_t i, const T & val)
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Segment_Tree::set: index " << i << " >= size " << n;

      set_impl(1, 0, n - 1, i, val);
    }

    /** @brief Range query over `[l, r]`.
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
        << "Gen_Segment_Tree::query: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Gen_Segment_Tree::query: l=" << l << " > r=" << r;

      return query_impl(1, 0, n - 1, l, r);
    }

    /** @brief Retrieve the value `a[i]`.
     *
     *  Equivalent to `query(i, i)`.  O(log n).
     *
     *  @param i 0-based index.
     *  @throw std::out_of_range if `i >= size()`.
     */
    T get(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Segment_Tree::get: index " << i << " >= size " << n;

      return query_impl(1, 0, n - 1, i, i);
    }

    /** @brief Number of logical elements. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n; }

    /** @brief True if the tree contains no elements. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0; }

    /** @brief Reconstruct all original values into an Array.
     *
     *  O(n log n).
     */
    Array<T> values() const
    {
      auto ret = Array<T>::create(n);
      for (size_t i = 0; i < n; ++i)
        ret(i) = get(i);
      return ret;
    }

    /** @brief Swap this tree with @p other in O(1). */
    void swap(Gen_Segment_Tree & other) noexcept
    {
      tree.swap(other.tree);
      std::swap(n, other.n);
      std::swap(identity, other.identity);
      std::swap(op, other.op);
    }
  };


  /** @brief Sum Segment Tree for range sum queries with point updates.
   *
   *  Convenient specialisation of Gen_Segment_Tree using `plus<T>`
   *  with identity `T()` (zero for arithmetic types).
   *
   *  @tparam T an arithmetic type.
   *
   *  @par Example
   *  @code
   *  Sum_Segment_Tree<int> st = {3, 1, 4, 1, 5};
   *  assert(st.query(0, 4) == 14);
   *  st.update(2, 10);  // a[2] += 10
   *  assert(st.query(0, 4) == 24);
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T>
  struct Sum_Segment_Tree
      : public Gen_Segment_Tree<T, Aleph::plus<T>>
  {
    using Base = Gen_Segment_Tree<T, Aleph::plus<T>>;

    Sum_Segment_Tree(const size_t num, const T & init_val = T())
      : Base(num, init_val, T())
    {}

    Sum_Segment_Tree(std::initializer_list<T> il)
      : Base(il, T())
    {}

    Sum_Segment_Tree(const Array<T> & values)
      : Base(values, T())
    {}

    Sum_Segment_Tree(const std::vector<T> & values)
      : Base(values, T())
    {}

    Sum_Segment_Tree(const DynList<T> & values)
      : Base(values, T())
    {}
  };

  /** @brief Min Segment Tree for range minimum queries with point updates.
   *
   *  Convenient specialisation of Gen_Segment_Tree using `Min_Op<T>`
   *  with identity `numeric_limits<T>::max()`.
   *
   *  @tparam T a totally ordered type with numeric_limits support.
   *
   *  @note This structure supports point updates and range minimum
   *  queries. For static RMQ with O(1) queries, prefer `Sparse_Table`.
   *
   *  @par Example
   *  @code
   *  Min_Segment_Tree<int> st = {5, 2, 4, 7, 1};
   *  assert(st.query(0, 3) == 2);  // min(5, 2, 4, 7)
   *  st.set(1, 10);
   *  assert(st.query(0, 3) == 4);  // min(5, 10, 4, 7)
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Min_Segment_Tree
      : public Gen_Segment_Tree<T, Min_Op<T>>
  {
    using Base = Gen_Segment_Tree<T, Min_Op<T>>;

    Min_Segment_Tree(const size_t num,
                     const T & init_val = std::numeric_limits<T>::max())
      : Base(num, init_val, std::numeric_limits<T>::max())
    {}

    Min_Segment_Tree(std::initializer_list<T> il)
      : Base(il, std::numeric_limits<T>::max())
    {}

    Min_Segment_Tree(const Array<T> & values)
      : Base(values, std::numeric_limits<T>::max())
    {}

    Min_Segment_Tree(const std::vector<T> & values)
      : Base(values, std::numeric_limits<T>::max())
    {}

    Min_Segment_Tree(const DynList<T> & values)
      : Base(values, std::numeric_limits<T>::max())
    {}
  };

  /** @brief Max Segment Tree for range maximum queries with point updates.
   *
   *  Convenient specialisation of Gen_Segment_Tree using `Max_Op<T>`
   *  with identity `numeric_limits<T>::lowest()`.
   *
   *  @tparam T a totally ordered type with numeric_limits support.
   *
   *  @note This structure supports point updates and range maximum
   *  queries. For static range max queries with O(1) queries, prefer
   *  `Max_Sparse_Table`.
   *
   *  @par Example
   *  @code
   *  Max_Segment_Tree<int> st = {5, 2, 4, 7, 1};
   *  assert(st.query(0, 3) == 7);  // max(5, 2, 4, 7)
   *  st.set(3, 0);
   *  assert(st.query(0, 3) == 5);  // max(5, 2, 4, 0)
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Max_Segment_Tree
      : public Gen_Segment_Tree<T, Max_Op<T>>
  {
    using Base = Gen_Segment_Tree<T, Max_Op<T>>;

    Max_Segment_Tree(const size_t num,
                     const T & init_val = std::numeric_limits<T>::lowest())
      : Base(num, init_val, std::numeric_limits<T>::lowest())
    {}

    Max_Segment_Tree(std::initializer_list<T> il)
      : Base(il, std::numeric_limits<T>::lowest())
    {}

    Max_Segment_Tree(const Array<T> & values)
      : Base(values, std::numeric_limits<T>::lowest())
    {}

    Max_Segment_Tree(const std::vector<T> & values)
      : Base(values, std::numeric_limits<T>::lowest())
    {}

    Max_Segment_Tree(const DynList<T> & values)
      : Base(values, std::numeric_limits<T>::lowest())
    {}
  };


  // ================================================================
  // Gen_Lazy_Segment_Tree — Range update + Range query
  // ================================================================

  /** @brief Policy concept for lazy segment tree.
   *
   *  This concept describes the interface required by
   *  `Gen_Lazy_Segment_Tree<Policy>`.
   *
   *  A policy must provide:
   *  - `value_type` — the type stored in tree nodes.
   *  - `lazy_type` — the type of pending lazy tags.
   *  - `combine(a, b) -> value_type` — merge two node values.
   *  - `apply(val, lazy, count) -> value_type` — apply a lazy tag to a
   *    node value covering `count` leaves.
   *  - `compose(old_lazy, new_lazy) -> lazy_type` — compose two lazy
   *    tags (new on top of old).
   *  - `value_identity() -> value_type` — identity for combine.
   *  - `lazy_identity() -> lazy_type` — identity for compose (no-op tag).
   *
   *  Additionally, `lazy_type` must be equality-comparable so that
   *  `push_down` can test `lazy(node) == pol.lazy_identity()` to
   *  skip propagation when no tag is pending.
   *
   *  @note Correctness requires `combine` to be associative and
   *  consistent with `apply` and `compose`.
   *
   *  @ingroup Sequences
   */
  template <typename P>
  concept SegmentTreePolicy =
      std::equality_comparable<typename P::lazy_type> &&
      requires(const P& p,
               const typename P::value_type& v1,
               const typename P::value_type& v2,
               const typename P::lazy_type& lz1,
               const typename P::lazy_type& lz2,
               size_t count)
        {
          typename P::value_type;
          typename P::lazy_type;
          { p.combine(v1, v2) } -> std::convertible_to<typename P::value_type>;
          { p.apply(v1, lz1, count) } -> std::convertible_to<typename P::value_type>;
          { p.compose(lz1, lz2) } -> std::convertible_to<typename P::lazy_type>;
          { p.value_identity() } -> std::convertible_to<typename P::value_type>;
          { p.lazy_identity() } -> std::convertible_to<typename P::lazy_type>;
        };

  /** @brief Range add + range sum policy.
   *
   *  Policy for `Gen_Lazy_Segment_Tree` implementing the classic
   *  "range add" + "range sum" segment tree.
   *
   *  - `combine(a, b) = a + b`
   *  - `apply(val, delta, count) = val + delta * count`
   *  - `compose(old, new) = old + new`
   *
   *  @tparam T arithmetic type.
   *
   *  @ingroup Sequences
   */
  template <typename T>
  struct Add_Sum_Policy
  {
    using value_type = T;
    using lazy_type = T;

    constexpr T combine(const T & a, const T & b) const noexcept
    {
      return a + b;
    }

    constexpr T apply(const T & val, const T & delta,
                      size_t count) const noexcept
    {
      return val + delta * static_cast<T>(count);
    }

    constexpr T compose(const T & old_lazy,
                        const T & new_lazy) const noexcept
    {
      return old_lazy + new_lazy;
    }

    constexpr T value_identity() const noexcept { return T(); }
    constexpr T lazy_identity() const noexcept { return T(); }
  };

  /** @brief Range add + range min policy.
   *
   *  Policy for `Gen_Lazy_Segment_Tree` implementing a segment tree
   *  supporting range additions and range minimum queries.
   *
   *  - `combine(a, b) = min(a, b)`
   *  - `apply(val, delta, count) = val + delta`
   *  - `compose(old, new) = old + new`
   *
   *  @tparam T a totally ordered arithmetic type.
   *
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Add_Min_Policy
  {
    using value_type = T;
    using lazy_type = T;

    constexpr T combine(const T & a, const T & b) const noexcept
    {
      return a <= b ? a : b;
    }

    constexpr T apply(const T & val, const T & delta,
                      size_t) const noexcept
    {
      return val + delta;
    }

    constexpr T compose(const T & old_lazy,
                        const T & new_lazy) const noexcept
    {
      return old_lazy + new_lazy;
    }

    constexpr T value_identity() const noexcept
    {
      return std::numeric_limits<T>::max();
    }

    constexpr T lazy_identity() const noexcept { return T(); }
  };

  /** @brief Range add + range max policy.
   *
   *  Policy for `Gen_Lazy_Segment_Tree` implementing a segment tree
   *  supporting range additions and range maximum queries.
   *
   *  - `combine(a, b) = max(a, b)`
   *  - `apply(val, delta, count) = val + delta`
   *  - `compose(old, new) = old + new`
   *
   *  @tparam T a totally ordered arithmetic type.
   *
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Add_Max_Policy
  {
    using value_type = T;
    using lazy_type = T;

    constexpr T combine(const T & a, const T & b) const noexcept
    {
      return a >= b ? a : b;
    }

    constexpr T apply(const T & val, const T & delta,
                      size_t) const noexcept
    {
      return val + delta;
    }

    constexpr T compose(const T & old_lazy,
                        const T & new_lazy) const noexcept
    {
      return old_lazy + new_lazy;
    }

    constexpr T value_identity() const noexcept
    {
      return std::numeric_limits<T>::lowest();
    }

    constexpr T lazy_identity() const noexcept { return T(); }
  };

  /** @brief Range assign + range sum policy.
   *
   *  Policy for `Gen_Lazy_Segment_Tree` implementing "range assign" +
   *  "range sum" updates/queries.
   *
   *  - `combine(a, b) = a + b`
   *  - `apply(val, (true, x), count) = x * count`
   *  - `compose(old, new)`: new overrides old if new.first is true.
   *
   *  The lazy type is `pair<bool, T>` where `first` indicates whether
   *  an assignment is pending.
   *
   *  @tparam T arithmetic type.
   *
   *  @note This policy is useful for classic tasks like "set a[l..r] = x"
   *  while querying range sums.
   *
   *  @ingroup Sequences
   */
  template <typename T>
  struct Assign_Sum_Policy
  {
    using value_type = T;
    using lazy_type = std::pair<bool, T>;

    constexpr T combine(const T & a, const T & b) const noexcept
    {
      return a + b;
    }

    constexpr T apply(const T & val, const lazy_type & lz,
                      size_t count) const noexcept
    {
      return lz.first ? lz.second * static_cast<T>(count) : val;
    }

    constexpr lazy_type compose(const lazy_type & old_lz,
                                const lazy_type & new_lz) const noexcept
    {
      return new_lz.first ? new_lz : old_lz;
    }

    constexpr T value_identity() const noexcept { return T(); }

    constexpr lazy_type lazy_identity() const noexcept
    {
      return {false, T()};
    }
  };


  /** @brief Lazy segment tree with range update and range query.
   *
   *  Parameterised by a `Policy` that bundles value type, lazy type,
   *  and the combine/apply/compose functions.  Supports range updates
   *  and range queries in O(log n) with lazy propagation.
   *
   *  @par Lazy propagation
   *  Updates store a pending tag at internal nodes and push it down to
   *  children only when required (during updates/queries), keeping the
   *  asymptotic cost to O(log n).
   *
   *  @par Constness
   *  `query()` is intentionally non-const because it may trigger
   *  propagation (`push_down`) and thus mutate internal lazy state.
   *
   *  @tparam Policy must satisfy `SegmentTreePolicy`.
   *
   *  @par Example
   *  @code
   *  Lazy_Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};
   *  st.update(1, 3, 10);   // a[1..3] += 10
   *  st.query(0, 4);        // 1 + 12 + 13 + 14 + 5 = 45
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename Policy>
    requires SegmentTreePolicy<Policy>
  class Gen_Lazy_Segment_Tree
  {
  public:
    using value_type = typename Policy::value_type;
    using lazy_type = typename Policy::lazy_type;
    using Item_Type = value_type;

  private:
    mutable Array<value_type> tree;
    mutable Array<lazy_type> lazy;
    size_t n = 0;
    Policy pol;

    void push_down(size_t node, size_t start, size_t end) const
    {
      if (lazy(node) == pol.lazy_identity())
        return;

      const size_t mid = start + (end - start) / 2;
      const size_t left = 2 * node;
      const size_t right = 2 * node + 1;

      tree(left) = pol.apply(tree(left), lazy(node), mid - start + 1);
      lazy(left) = pol.compose(lazy(left), lazy(node));

      tree(right) = pol.apply(tree(right), lazy(node), end - mid);
      lazy(right) = pol.compose(lazy(right), lazy(node));

      lazy(node) = pol.lazy_identity();
    }

    void build(size_t node, size_t start, size_t end)
    {
      if (start == end)
        return;

      const size_t mid = start + (end - start) / 2;
      build(2 * node, start, mid);
      build(2 * node + 1, mid + 1, end);
      tree(node) = pol.combine(tree(2 * node), tree(2 * node + 1));
    }

    void update_impl(size_t node, size_t start, size_t end,
                     size_t l, size_t r, const lazy_type & val)
    {
      if (r < start || end < l)
        return;

      if (l <= start && end <= r)
        {
          tree(node) = pol.apply(tree(node), val, end - start + 1);
          lazy(node) = pol.compose(lazy(node), val);
          return;
        }

      push_down(node, start, end);

      const size_t mid = start + (end - start) / 2;
      update_impl(2 * node, start, mid, l, r, val);
      update_impl(2 * node + 1, mid + 1, end, l, r, val);
      tree(node) = pol.combine(tree(2 * node), tree(2 * node + 1));
    }

    value_type query_impl(size_t node, size_t start, size_t end,
                          size_t l, size_t r) const
    {
      if (r < start || end < l)
        return pol.value_identity();

      if (l <= start && end <= r)
        return tree(node);

      push_down(node, start, end);

      const size_t mid = start + (end - start) / 2;
      return pol.combine(
        query_impl(2 * node, start, mid, l, r),
        query_impl(2 * node + 1, mid + 1, end, l, r));
    }

    void set_impl(size_t node, size_t start, size_t end,
                  size_t idx, const value_type & val)
    {
      if (start == end)
        {
          tree(node) = val;
          lazy(node) = pol.lazy_identity();
          return;
        }

      push_down(node, start, end);

      const size_t mid = start + (end - start) / 2;
      if (idx <= mid)
        set_impl(2 * node, start, mid, idx, val);
      else
        set_impl(2 * node + 1, mid + 1, end, idx, val);

      tree(node) = pol.combine(tree(2 * node), tree(2 * node + 1));
    }

    template <class Getter>
    void fill_and_build(Getter getter)
    {
      fill_leaves_recursive(1, 0, n - 1, getter);
      build(1, 0, n - 1);
    }

    template <class Getter>
    void fill_leaves_recursive(size_t node, size_t start, size_t end,
                               Getter & getter)
    {
      if (start == end)
        {
          tree(node) = getter(start);
          return;
        }

      const size_t mid = start + (end - start) / 2;
      fill_leaves_recursive(2 * node, start, mid, getter);
      fill_leaves_recursive(2 * node + 1, mid + 1, end, getter);
    }

    template <class AlephIt>
    void fill_from_aleph_it(AlephIt it)
    {
      auto tmp = Array<value_type>::create(n);
      size_t i = 0;
      for (; it.has_curr(); it.next_ne())
        tmp(i++) = it.get_curr();

      auto getter = [&tmp](size_t j) { return tmp(j); };
      fill_and_build(getter);
    }

    size_t alloc_size() const
    {
      return n == 0 ? 1 : 4 * n;
    }

  public:
    /** @brief Construct with @p num elements, all equal to @p init_val. */
    Gen_Lazy_Segment_Tree(const size_t num,
                          const value_type & init_val = value_type(),
                          Policy p = Policy())
      : tree(num == 0 ? 1 : 4 * num, p.value_identity()),
        lazy(num == 0 ? 1 : 4 * num, p.lazy_identity()),
        n(num), pol(p)
    {
      if (n > 0)
        fill_and_build([&init_val](size_t) { return init_val; });
    }

    /** @brief Construct from an initializer list. */
    Gen_Lazy_Segment_Tree(std::initializer_list<value_type> il,
                          Policy p = Policy())
      : tree(il.size() == 0 ? 1 : 4 * il.size(), p.value_identity()),
        lazy(il.size() == 0 ? 1 : 4 * il.size(), p.lazy_identity()),
        n(il.size()), pol(p)
    {
      if (n > 0)
        {
          auto it = il.begin();
          fill_and_build([&it](size_t) { return *it++; });
        }
    }

    /** @brief Construct from an Array\<value_type\>. */
    Gen_Lazy_Segment_Tree(const Array<value_type> & values,
                          Policy p = Policy())
      : tree(values.size() == 0 ? 1 : 4 * values.size(), p.value_identity()),
        lazy(values.size() == 0 ? 1 : 4 * values.size(), p.lazy_identity()),
        n(values.size()), pol(p)
    {
      if (n > 0)
        fill_and_build([&values](size_t i) { return values(i); });
    }

    /** @brief Construct from a std::vector. */
    Gen_Lazy_Segment_Tree(const std::vector<value_type> & values,
                          Policy p = Policy())
      : tree(values.size() == 0 ? 1 : 4 * values.size(), p.value_identity()),
        lazy(values.size() == 0 ? 1 : 4 * values.size(), p.lazy_identity()),
        n(values.size()), pol(p)
    {
      if (n > 0)
        fill_and_build([&values](size_t i) { return values[i]; });
    }

    /** @brief Construct from a DynList. */
    Gen_Lazy_Segment_Tree(const DynList<value_type> & values,
                          Policy p = Policy())
      : tree(values.size() == 0 ? 1 : 4 * values.size(), p.value_identity()),
        lazy(values.size() == 0 ? 1 : 4 * values.size(), p.lazy_identity()),
        n(values.size()), pol(p)
    {
      if (n > 0)
        fill_from_aleph_it(values.get_it());
    }

    Gen_Lazy_Segment_Tree(const Gen_Lazy_Segment_Tree &) = default;

    Gen_Lazy_Segment_Tree(Gen_Lazy_Segment_Tree &&) noexcept(
      std::is_nothrow_move_constructible_v<Array<value_type>> &&
      std::is_nothrow_move_constructible_v<Array<lazy_type>> &&
      std::is_nothrow_move_constructible_v<Policy>) = default;

    Gen_Lazy_Segment_Tree & operator=(const Gen_Lazy_Segment_Tree &) = default;

    Gen_Lazy_Segment_Tree & operator=(Gen_Lazy_Segment_Tree &&) noexcept(
      std::is_nothrow_move_assignable_v<Array<value_type>> &&
      std::is_nothrow_move_assignable_v<Array<lazy_type>> &&
      std::is_nothrow_move_assignable_v<Policy>) = default;

    /** @brief Range update: apply @p val to every `a[i]` with
     *  `l <= i <= r`.
     *
     *  @param l   0-based left index (inclusive).
     *  @param r   0-based right index (inclusive).
     *  @param val lazy tag to apply.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    void update(const size_t l, const size_t r, const lazy_type & val)
    {
      ah_out_of_range_error_if(r >= n)
        << "Gen_Lazy_Segment_Tree::update: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Gen_Lazy_Segment_Tree::update: l=" << l << " > r=" << r;

      update_impl(1, 0, n - 1, l, r, val);
    }

    /** @brief Point update: apply @p delta to `a[i]`.
     *
     *  Convenience shorthand for `update(i, i, delta)`.
     */
    void point_update(const size_t i, const lazy_type & delta)
    {
      update(i, i, delta);
    }

    /** @brief Set `a[i] = val`.
     *
     *  O(log n).
     *
     *  @param i   0-based index.
     *  @param val new value for position i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    void set(const size_t i, const value_type & val)
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Lazy_Segment_Tree::set: index " << i << " >= size " << n;

      set_impl(1, 0, n - 1, i, val);
    }

    /** @brief Range query over `[l, r]`.
     *
     *  Lazy tags are propagated internally via `mutable` storage,
     *  so this method is `const` and safe to call through
     *  `const` references.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @return  combined value over the range.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    value_type query(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Gen_Lazy_Segment_Tree::query: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Gen_Lazy_Segment_Tree::query: l=" << l << " > r=" << r;

      return query_impl(1, 0, n - 1, l, r);
    }

    /** @brief Retrieve the value `a[i]`.
     *
     *  Equivalent to `query(i, i)`.  O(log n).
     */
    value_type get(const size_t i) const
    {
      return query(i, i);
    }

    /** @brief Number of logical elements. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n; }

    /** @brief True if the tree contains no elements. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0; }

    /** @brief Reconstruct all original values into an Array.
     *
     *  O(n log n).
     */
    Array<value_type> values() const
    {
      auto ret = Array<value_type>::create(n);
      for (size_t i = 0; i < n; ++i)
        ret(i) = get(i);
      return ret;
    }

    /** @brief Swap this tree with @p other in O(1). */
    void swap(Gen_Lazy_Segment_Tree & other) noexcept
    {
      tree.swap(other.tree);
      lazy.swap(other.lazy);
      std::swap(n, other.n);
      std::swap(pol, other.pol);
    }
  };


  /** @brief Lazy segment tree for range add + range sum.
   *  @tparam T arithmetic type.
   *  @ingroup Sequences
   */
  template <typename T>
  struct Lazy_Sum_Segment_Tree
      : public Gen_Lazy_Segment_Tree<Add_Sum_Policy<T>>
  {
    using Base = Gen_Lazy_Segment_Tree<Add_Sum_Policy<T>>;
    using Base::Base;
  };

  /** @brief Lazy segment tree for range add + range min.
   *
   *  Convenience wrapper around:
   *  `Gen_Lazy_Segment_Tree<Add_Min_Policy<T>>`.
   *
   *  @tparam T a totally ordered arithmetic type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Lazy_Min_Segment_Tree
      : public Gen_Lazy_Segment_Tree<Add_Min_Policy<T>>
  {
    using Base = Gen_Lazy_Segment_Tree<Add_Min_Policy<T>>;
    using Base::Base;
  };

  /** @brief Lazy segment tree for range add + range max.
   *
   *  Convenience wrapper around:
   *  `Gen_Lazy_Segment_Tree<Add_Max_Policy<T>>`.
   *
   *  @tparam T a totally ordered arithmetic type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Lazy_Max_Segment_Tree
      : public Gen_Lazy_Segment_Tree<Add_Max_Policy<T>>
  {
    using Base = Gen_Lazy_Segment_Tree<Add_Max_Policy<T>>;
    using Base::Base;
  };


  // ================================================================
  // Segment_Tree_Beats — Ji Driver's Segment Tree
  // ================================================================

  /** @brief Ji Driver's Segment Tree (Segment Tree Beats).
   *
   *  Supports range `chmin(l, r, v)` and `chmax(l, r, v)` operations
   *  together with range sum, min, and max queries.
   *
   *  Each node maintains: max, second_max, count_max, min, second_min,
   *  count_min, sum.  The push-down uses break/tag conditions for
   *  amortised O(n log^2 n) total complexity.
   *
   *  @par When to use
   *  Use this structure when you need range clamping operations
   *  (`chmin`/`chmax`) in addition to sum/min/max queries.
   *  For simpler update patterns, prefer `Gen_Lazy_Segment_Tree`.
   *
   *  @tparam T a signed arithmetic type.
   *
   *  @par Example
   *  @code
   *  Segment_Tree_Beats<int> st = {5, 2, 4, 7, 1};
   *  st.chmin(0, 4, 4);        // clamp all to <= 4
   *  // array becomes {4, 2, 4, 4, 1}
   *  st.query_sum(0, 4);       // 15
   *  st.query_max(0, 4);       // 4
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T>
    requires std::is_arithmetic_v<T> && std::is_signed_v<T>
  class Segment_Tree_Beats
  {
    struct Node
    {
      T max_val, second_max, min_val, second_min, sum;
      size_t count_max, count_min;
      T lazy_chmin, lazy_chmax;
    };

    mutable Array<Node> tree;
    size_t n = 0;

    static constexpr T INF = std::numeric_limits<T>::max();
    static constexpr T NEG_INF = std::numeric_limits<T>::lowest();

    void init_leaf(size_t node, const T & val)
    {
      tree(node) = {val, NEG_INF, val, INF, val,
                    1, 1, INF, NEG_INF};
    }

    void pull_up(size_t node) const
    {
      const auto & l = tree(2 * node);
      const auto & r = tree(2 * node + 1);

      tree(node).sum = l.sum + r.sum;

      // max
      if (l.max_val > r.max_val)
        {
          tree(node).max_val = l.max_val;
          tree(node).count_max = l.count_max;
          tree(node).second_max = std::max(l.second_max, r.max_val);
        }
      else if (l.max_val < r.max_val)
        {
          tree(node).max_val = r.max_val;
          tree(node).count_max = r.count_max;
          tree(node).second_max = std::max(l.max_val, r.second_max);
        }
      else
        {
          tree(node).max_val = l.max_val;
          tree(node).count_max = l.count_max + r.count_max;
          tree(node).second_max = std::max(l.second_max, r.second_max);
        }

      // min
      if (l.min_val < r.min_val)
        {
          tree(node).min_val = l.min_val;
          tree(node).count_min = l.count_min;
          tree(node).second_min = std::min(l.second_min, r.min_val);
        }
      else if (l.min_val > r.min_val)
        {
          tree(node).min_val = r.min_val;
          tree(node).count_min = r.count_min;
          tree(node).second_min = std::min(l.min_val, r.second_min);
        }
      else
        {
          tree(node).min_val = l.min_val;
          tree(node).count_min = l.count_min + r.count_min;
          tree(node).second_min = std::min(l.second_min, r.second_min);
        }

      tree(node).lazy_chmin = INF;
      tree(node).lazy_chmax = NEG_INF;
    }

    void push_chmin_to_child(size_t child, T v) const
    {
      auto & nd = tree(child);
      if (v >= nd.max_val)
        return;

      nd.sum -= static_cast<T>(nd.count_max) * (nd.max_val - v);

      // If all elements in this node are the same value (max == min),
      // then reducing max also reduces min
      if (nd.min_val == nd.max_val)
        nd.min_val = v;
      if (nd.second_min == nd.max_val)
        nd.second_min = v;

      nd.max_val = v;

      // Compose with existing lazy tags
      nd.lazy_chmin = std::min(nd.lazy_chmin, v);
      nd.lazy_chmax = std::min(nd.lazy_chmax, v);
    }

    void push_chmax_to_child(size_t child, T v) const
    {
      auto & nd = tree(child);
      if (v <= nd.min_val)
        return;

      nd.sum += static_cast<T>(nd.count_min) * (v - nd.min_val);

      // If all elements are the same value, raising min also raises max
      if (nd.max_val == nd.min_val)
        nd.max_val = v;
      if (nd.second_max == nd.min_val)
        nd.second_max = v;

      nd.min_val = v;

      // Compose with existing lazy tags
      nd.lazy_chmax = std::max(nd.lazy_chmax, v);
      nd.lazy_chmin = std::max(nd.lazy_chmin, v);
    }

    void push_down(size_t node) const
    {
      auto & nd = tree(node);

      // Push chmax first (raise minimum), then chmin (lower maximum)
      // This order is critical for correctness
      if (nd.lazy_chmax != NEG_INF)
        {
          push_chmax_to_child(2 * node, nd.lazy_chmax);
          push_chmax_to_child(2 * node + 1, nd.lazy_chmax);
          nd.lazy_chmax = NEG_INF;
        }

      if (nd.lazy_chmin != INF)
        {
          push_chmin_to_child(2 * node, nd.lazy_chmin);
          push_chmin_to_child(2 * node + 1, nd.lazy_chmin);
          nd.lazy_chmin = INF;
        }
    }

    void build(size_t node, size_t start, size_t end)
    {
      if (start == end)
        return;

      const size_t mid = start + (end - start) / 2;
      build(2 * node, start, mid);
      build(2 * node + 1, mid + 1, end);
      pull_up(node);
    }

    void chmin_impl(size_t node, size_t start, size_t end,
                    size_t l, size_t r, T v)
    {
      if (r < start || end < l || tree(node).max_val <= v)
        return;

      if (l <= start && end <= r && tree(node).second_max < v)
        {
          push_chmin_to_child(node, v);
          return;
        }

      push_down(node);
      const size_t mid = start + (end - start) / 2;
      chmin_impl(2 * node, start, mid, l, r, v);
      chmin_impl(2 * node + 1, mid + 1, end, l, r, v);
      pull_up(node);
    }

    void chmax_impl(size_t node, size_t start, size_t end,
                    size_t l, size_t r, T v)
    {
      if (r < start || end < l || tree(node).min_val >= v)
        return;

      if (l <= start && end <= r && tree(node).second_min > v)
        {
          push_chmax_to_child(node, v);
          return;
        }

      push_down(node);
      const size_t mid = start + (end - start) / 2;
      chmax_impl(2 * node, start, mid, l, r, v);
      chmax_impl(2 * node + 1, mid + 1, end, l, r, v);
      pull_up(node);
    }

    T query_sum_impl(size_t node, size_t start, size_t end,
                     size_t l, size_t r) const
    {
      if (r < start || end < l)
        return T();

      if (l <= start && end <= r)
        return tree(node).sum;

      push_down(node);
      const size_t mid = start + (end - start) / 2;
      return query_sum_impl(2 * node, start, mid, l, r) +
             query_sum_impl(2 * node + 1, mid + 1, end, l, r);
    }

    T query_max_impl(size_t node, size_t start, size_t end,
                     size_t l, size_t r) const
    {
      if (r < start || end < l)
        return NEG_INF;

      if (l <= start && end <= r)
        return tree(node).max_val;

      push_down(node);
      const size_t mid = start + (end - start) / 2;
      return std::max(query_max_impl(2 * node, start, mid, l, r),
                      query_max_impl(2 * node + 1, mid + 1, end, l, r));
    }

    T query_min_impl(size_t node, size_t start, size_t end,
                     size_t l, size_t r) const
    {
      if (r < start || end < l)
        return INF;

      if (l <= start && end <= r)
        return tree(node).min_val;

      push_down(node);
      const size_t mid = start + (end - start) / 2;
      return std::min(query_min_impl(2 * node, start, mid, l, r),
                      query_min_impl(2 * node + 1, mid + 1, end, l, r));
    }

    template <class Getter>
    void fill_and_build(Getter getter)
    {
      fill_leaves_recursive(1, 0, n - 1, getter);
      build(1, 0, n - 1);
    }

    template <class Getter>
    void fill_leaves_recursive(size_t node, size_t start, size_t end,
                               Getter & getter)
    {
      if (start == end)
        {
          init_leaf(node, getter(start));
          return;
        }

      const size_t mid = start + (end - start) / 2;
      fill_leaves_recursive(2 * node, start, mid, getter);
      fill_leaves_recursive(2 * node + 1, mid + 1, end, getter);
    }

    void init_all_nodes()
    {
      Node def = {NEG_INF, NEG_INF, INF, INF, T(),
                  0, 0, INF, NEG_INF};
      for (size_t i = 0; i < tree.size(); ++i)
        tree(i) = def;
    }

  public:
    using Item_Type = T;

    /** @brief Construct with @p num elements, all equal to @p init_val. */
    Segment_Tree_Beats(const size_t num, const T & init_val = T())
      : tree(num == 0 ? 1 : 4 * num), n(num)
    {
      init_all_nodes();
      if (n > 0)
        fill_and_build([&init_val](size_t) { return init_val; });
    }

    /** @brief Construct from an initializer list. */
    Segment_Tree_Beats(std::initializer_list<T> il)
      : tree(il.size() == 0 ? 1 : 4 * il.size()), n(il.size())
    {
      init_all_nodes();
      if (n > 0)
        {
          auto it = il.begin();
          fill_and_build([&it](size_t) { return *it++; });
        }
    }

    /** @brief Construct from an Array\<T\>. */
    Segment_Tree_Beats(const Array<T> & values)
      : tree(values.size() == 0 ? 1 : 4 * values.size()),
        n(values.size())
    {
      init_all_nodes();
      if (n > 0)
        fill_and_build([&values](size_t i) { return values(i); });
    }

    /** @brief Construct from a std::vector\<T\>. */
    Segment_Tree_Beats(const std::vector<T> & values)
      : tree(values.size() == 0 ? 1 : 4 * values.size()),
        n(values.size())
    {
      init_all_nodes();
      if (n > 0)
        fill_and_build([&values](size_t i) { return values[i]; });
    }

    Segment_Tree_Beats(const Segment_Tree_Beats &) = default;
    Segment_Tree_Beats(Segment_Tree_Beats &&) noexcept = default;
    Segment_Tree_Beats & operator=(const Segment_Tree_Beats &) = default;
    Segment_Tree_Beats & operator=(Segment_Tree_Beats &&) noexcept = default;

    /** @brief Range chmin: for each `i` in `[l, r]`, set `a[i] = min(a[i], v)`.
     *
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    void chmin(const size_t l, const size_t r, const T & v)
    {
      ah_out_of_range_error_if(r >= n)
        << "Segment_Tree_Beats::chmin: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Segment_Tree_Beats::chmin: l=" << l << " > r=" << r;

      chmin_impl(1, 0, n - 1, l, r, v);
    }

    /** @brief Range chmax: for each `i` in `[l, r]`, set `a[i] = max(a[i], v)`.
     *
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    void chmax(const size_t l, const size_t r, const T & v)
    {
      ah_out_of_range_error_if(r >= n)
        << "Segment_Tree_Beats::chmax: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Segment_Tree_Beats::chmax: l=" << l << " > r=" << r;

      chmax_impl(1, 0, n - 1, l, r, v);
    }

    /** @brief Range sum query over `[l, r]`.
     *
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query_sum(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Segment_Tree_Beats::query_sum: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Segment_Tree_Beats::query_sum: l=" << l << " > r=" << r;

      return query_sum_impl(1, 0, n - 1, l, r);
    }

    /** @brief Range max query over `[l, r]`.
     *
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query_max(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Segment_Tree_Beats::query_max: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Segment_Tree_Beats::query_max: l=" << l << " > r=" << r;

      return query_max_impl(1, 0, n - 1, l, r);
    }

    /** @brief Range min query over `[l, r]`.
     *
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query_min(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Segment_Tree_Beats::query_min: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Segment_Tree_Beats::query_min: l=" << l << " > r=" << r;

      return query_min_impl(1, 0, n - 1, l, r);
    }

    /** @brief Retrieve the value at position @p i.  O(log n). */
    T get(const size_t i) const
    {
      return query_sum(i, i);
    }

    /** @brief Number of logical elements. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n; }

    /** @brief True if the tree contains no elements. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0; }

    /** @brief Reconstruct all values into an Array.  O(n log n). */
    Array<T> values() const
    {
      auto ret = Array<T>::create(n);
      for (size_t i = 0; i < n; ++i)
        ret(i) = get(i);
      return ret;
    }

    /** @brief Swap this tree with @p other in O(1). */
    void swap(Segment_Tree_Beats & other) noexcept
    {
      tree.swap(other.tree);
      std::swap(n, other.n);
    }
  };

} // end namespace Aleph

# endif /* TPL_SEGMENT_TREE_H */
