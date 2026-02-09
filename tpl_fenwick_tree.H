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


/** @file tpl_fenwick_tree.H
 *  @brief Fenwick tree (Binary Indexed Tree) for prefix queries.
 *
 *  A Fenwick tree maintains an implicit array of `n` elements over an
 *  abelian group (an associative, commutative, invertible binary
 *  operation with identity) and supports prefix and range queries and
 *  point updates in O(log n) time.
 *
 *  Three class templates are provided:
 *
 *  - **Gen_Fenwick_Tree\<T, Plus, Minus\>** — fully generic over any
 *    abelian group given by the binary functors `Plus` and `Minus`.
 *  - **Fenwick_Tree\<T\>** — convenient specialisation for signed arithmetic
 *    types (uses `operator+` / `operator-`).  Adds the `find_kth`
 *    order-statistic query that exploits the internal bit layout in
 *    O(log n).
 *  - **Range_Fenwick_Tree\<T\>** — signed arithmetic variant that
 *    supports range updates and range queries in O(log n).
 *
 *  ## Complexity
 *  | Operation    | Time      |
 *  |-------------|-----------|
 *  | Construction | O(n)      |
 *  | update       | O(log n)  |
 *  | prefix       | O(log n)  |
 *  | query        | O(log n)  |
 *  | find_kth     | O(log n)  |
 *  | get / set    | O(log n)  |
 *  | Space        | O(n)      |
 *
 *  @see https://en.wikipedia.org/wiki/Fenwick_tree
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_FENWICK_TREE_H
# define TPL_FENWICK_TREE_H

# include <bit>
# include <cassert>
# include <concepts>
# include <initializer_list>
# include <type_traits>
# include <vector>
# include <utility>
# include <tpl_array.H>
# include <tpl_dynList.H>
# include <ahFunction.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Arithmetic domain accepted by Fenwick specializations.
   *
   *  Signed, totally ordered arithmetic types excluding `bool`.
   */
  template <typename T>
  concept FenwickArithmetic =
      std::is_arithmetic_v<T> &&
      std::totally_ordered<T> &&
      std::is_signed_v<T> &&
      !std::same_as<T, bool>;

  /** @brief Binary operation compatible with Fenwick tree group functors.
   *
   *  Requires the functor to be callable with two (logically) immutable values
   *  of type @p T and return a value convertible to @p T.  The functor itself
   *  is also required to be callable through a const reference, matching usage
   *  from const member functions in Gen_Fenwick_Tree.
   */
  template <typename F, typename T>
  concept FenwickBinaryOp =
      requires(const F& f, const T& a, const T& b)
        {
          { f(a, b) } -> std::convertible_to<T>;
        };

  /** @brief Fenwick tree over an arbitrary abelian group.
   *
   *  Maintains an implicit array `a[0..n-1]` and a companion tree
   *  array so that prefix queries `a[0] + a[1] + ... + a[i]` and
   *  point updates `a[i] += delta` are answered in O(log n).
   *
   *  The group is specified through two binary functors:
   *  - `Plus(a, b)`  — the group operation (must be associative and
   *    commutative).
   *  - `Minus(a, b)` — the group subtraction, i.e.
   *    `Plus(Minus(a, b), b) == a`.
   *
   *  The identity element is assumed to be `T()` (the value produced
   *  by the default constructor).
   *
   *  @tparam T     element type.
   *  @tparam Plus  binary functor for the group operation.
   *  @tparam Minus binary functor for the inverse operation.
   *
   *  @par Example
   *  @code
   *  // XOR Fenwick tree
   *  struct Xor { int operator()(int a, int b) const { return a ^ b; } };
   *  Gen_Fenwick_Tree<int, Xor, Xor> ft(10); // XOR is its own inverse
   *  ft.update(3, 7);
   *  int pfx = ft.prefix(5);
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T,
            class Plus = Aleph::plus<T>,
            class Minus = Aleph::minus<T>>
    requires (FenwickBinaryOp<Plus, T> && FenwickBinaryOp<Minus, T>)
  class Gen_Fenwick_Tree
  {
  protected:
    Array<T> tree; // 1-indexed internal storage (position 0 unused)
    size_t n = 0; // number of logical elements (0-indexed: 0..n-1)

    Plus plus_op;
    Minus minus_op;

    /// O(n) bottom-up construction of the tree array from raw values
    /// already placed in tree[1..n].
    void build()
    {
      for (size_t i = 1; i <= n; ++i)
        if (const size_t parent = i + (i & (-i)); parent <= n)
          tree(parent) = plus_op(tree(parent), tree(i));
    }

    /// Fill the internal storage from a 0-based indexed getter and build.
    template <class F>
    void fill_from_indexed(F getter)
    {
      for (size_t i = 0; i < n; ++i)
        tree(i + 1) = getter(i);
      build();
    }

    /// Fill the internal storage from a standard iterator range and build.
    template <class StdIt>
    void fill_from_std_iter(StdIt first, StdIt last)
    {
      size_t i = 1;
      for (; first != last; ++first)
        tree(i++) = *first;
      build();
    }

    /// Fill the internal storage from an Aleph-style iterator (get_it()) and build.
    template <class AlephIt>
    void fill_from_aleph_it(AlephIt it)
    {
      size_t i = 1;
      for (; it.has_curr(); it.next_ne())
        tree(i++) = it.get_curr();
      build();
    }

  public:
    /// The type of element stored in the tree.
    using Item_Type = T;

    /** @brief Construct a tree with @p num elements, all equal to
     *  the identity `T()`.
     *
     *  @param num     number of elements.
     *  @param pop     group operation functor.
     *  @param mop     group subtraction functor.
     */
    Gen_Fenwick_Tree(const size_t num,
                     Plus pop = Plus(), Minus mop = Minus())
      : tree(num + 1, T()), n(num),
        plus_op(pop), minus_op(mop)
    {
      // empty — all elements are identity
    }

    /** @brief Construct from an initializer list in O(n) time.
     *
     *  @par Example
     *  @code
     *  Gen_Fenwick_Tree<int> ft = {3, 1, 4, 1, 5};
     *  assert(ft.prefix(2) == 8); // 3 + 1 + 4
     *  @endcode
     */
    Gen_Fenwick_Tree(std::initializer_list<T> il,
                     Plus pop = Plus(), Minus mop = Minus())
      : tree(il.size() + 1, T()), n(il.size()),
        plus_op(pop), minus_op(mop)
    {
      fill_from_std_iter(il.begin(), il.end());
    }

    /** @brief Construct from an Array\<T\> in O(n) time.
     *
     *  @param values  source array with the initial element values.
     *  @param pop     group operation functor.
     *  @param mop     group subtraction functor.
     */
    Gen_Fenwick_Tree(const Array<T> & values,
                     Plus pop = Plus(), Minus mop = Minus())
      : tree(values.size() + 1, T()), n(values.size()),
        plus_op(pop), minus_op(mop)
    {
      fill_from_indexed([&values](size_t i) { return values(i); });
    }

    /** @brief Construct from a std::vector<T> in O(n) time.
     *
     *  This overload is intentionally provided as STL-interoperability
     *  convenience. Aleph containers remain the primary API surface.
     *
     *  @param values  source vector with the initial element values.
     *  @param pop     group operation functor.
     *  @param mop     group subtraction functor.
     */
    Gen_Fenwick_Tree(const std::vector<T> & values,
                     Plus pop = Plus(), Minus mop = Minus())
      : tree(values.size() + 1, T()), n(values.size()),
        plus_op(pop), minus_op(mop)
    {
      fill_from_indexed([&values](size_t i) { return values[i]; });
    }

    /** @brief Construct from a DynList<T> in O(n) time.
     *
     *  @param values  source list with the initial element values.
     *  @param pop     group operation functor.
     *  @param mop     group subtraction functor.
     */
    Gen_Fenwick_Tree(const DynList<T> & values,
                     Plus pop = Plus(), Minus mop = Minus())
      : tree(values.size() + 1, T()), n(values.size()),
        plus_op(pop), minus_op(mop)
    {
      fill_from_aleph_it(values.get_it());
    }

    Gen_Fenwick_Tree(const Gen_Fenwick_Tree &) = default;

    Gen_Fenwick_Tree(Gen_Fenwick_Tree &&) noexcept = default;

    Gen_Fenwick_Tree &operator=(const Gen_Fenwick_Tree &) = default;

    Gen_Fenwick_Tree &operator=(Gen_Fenwick_Tree &&) noexcept = default;

    /** @brief Point update: `a[i] = Plus(a[i], delta)`.
     *
     *  @param i     0-based index.
     *  @param delta value to add.
     *  @throw std::out_of_range if `i >= size()`.
     */
    void update(size_t i, const T & delta)
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Fenwick_Tree::update: index " << i << " >= size " << n;

      for (++i; i <= n; i += i & (-i))
        tree(i) = plus_op(tree(i), delta);
    }

    /** @brief Prefix query: `Plus(a[0], a[1], ..., a[i])`.
     *
     *  @param i 0-based index.
     *  @return  accumulated value from index 0 through i (inclusive).
     *  @throw std::out_of_range if `i >= size()`.
     */
    T prefix(size_t i) const
    {
      ah_out_of_range_error_if(i >= n)
        << "Gen_Fenwick_Tree::prefix: index " << i << " >= size " << n;

      T s = T();
      for (++i; i > 0; i -= i & (-i))
        s = plus_op(s, tree(i));
      return s;
    }

    /** @brief Range query: `Plus(a[l], a[l+1], ..., a[r])`.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @return  accumulated value over `[l, r]`.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Gen_Fenwick_Tree::query: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Gen_Fenwick_Tree::query: l=" << l << " > r=" << r;

      return l > 0 ? minus_op(prefix(r), prefix(l - 1)) : prefix(r);
    }

    /** @brief Retrieve the logical value `a[i]`.
     *
     *  Equivalent to `query(i, i)`.  O(log n).
     *
     *  @param i 0-based index.
     *  @throw std::out_of_range if `i >= size()`.
     */
    T get(const size_t i) const
    {
      return query(i, i);
    }

    /** @brief Set `a[i] = value`.
     *
     *  Computes the required delta and calls update().  O(log n).
     *
     *  @param i     0-based index.
     *  @param value new value for position i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    void set(const size_t i, const T & value)
    {
      update(i, minus_op(value, get(i)));
    }

    /** @brief Number of logical elements. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n; }

    /** @brief True if the tree contains no elements. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0; }

    /** @brief Reconstruct all original values into an Array.
     *
     *  O(n log n) in the general case.
     */
    Array<T> values() const
    {
      auto ret = Array<T>::create(n);
      for (size_t i = 0; i < n; ++i)
        ret(i) = get(i);
      return ret;
    }

    /** @brief Swap this tree with @p other in O(1). */
    void swap(Gen_Fenwick_Tree & other) noexcept
    {
      tree.swap(other.tree);
      std::swap(n, other.n);
      std::swap(plus_op, other.plus_op);
      std::swap(minus_op, other.minus_op);
    }
  };


  /** @brief Fenwick tree for arithmetic types with find_kth support.
   *
   *  Inherits every operation from Gen_Fenwick_Tree using the standard
   *  `operator+` / `operator-` and adds `find_kth`, which finds the
   *  smallest 0-based index whose prefix sum reaches a given target.
   *
   *  @tparam T arithmetic element type.
   *
   *  @par Example
   *  @code
   *  Fenwick_Tree<int> ft(10);
   *  ft.update(2, 3);
   *  ft.update(5, 7);
   *  ft.update(8, 1);
   *  size_t idx = ft.find_kth(4);  // smallest i with prefix(i) >= 4
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <FenwickArithmetic T>
  struct Fenwick_Tree
      : public Gen_Fenwick_Tree<T, Aleph::plus<T>, Aleph::minus<T>>
  {
    using Base = Gen_Fenwick_Tree<T, Aleph::plus<T>, Aleph::minus<T>>;
    using Base::Base; // inherit all constructors

    /** @brief Find the smallest 0-based index `i` such that
     *  `prefix(i) >= k`.
     *
     *  **Precondition**: all stored values must be non-negative so
     *  that prefix sums are monotonically non-decreasing.
     *
     *  Performs a single O(log n) descent through the tree structure
     *  (no binary search).
     *
     *  @param k target cumulative value.
     *  @return  0-based index, or `size()` if the total sum is less
     *           than @p k.
     */
    size_t find_kth(T k) const
    {
      const size_t nn = this->n;
      if (nn == 0)
        return 0;

      size_t pos = 0;
      // highest power of 2 that is <= nn
      size_t bit_mask = std::bit_floor(nn);

      while (bit_mask > 0)
        {
          if (const size_t next = pos + bit_mask; next <= nn && this->tree(next) < k)
            {
              k -= this->tree(next);
              pos = next;
            }
          bit_mask >>= 1;
        }

      return pos < nn ? pos : nn;
    }
  };

  /** @brief Fenwick tree supporting range updates and range queries.
   *
   *  Maintains an implicit array `a[0..n-1]` and supports adding a
   *  constant to every element in a contiguous range, as well as
   *  querying the sum over any contiguous range, both in O(log n).
   *
   *  Internally, two classic Fenwick trees (`B1`, `B2`) are kept so
   *  that the prefix sum can be recovered as
   *  \f[
   *    P(i) = (i+1)\,\text{B1.prefix}(i) - \text{B2.prefix}(i)
   *  \f]
   *
   *  This class is restricted to signed arithmetic types (it uses
   *  multiplication, subtraction, and negative deltas directly).
   *
   *  @tparam T signed arithmetic element type.
   *
   *  @par Complexity
   *  | Operation       | Time     |
   *  |----------------|----------|
   *  | Construction    | O(n)     |
   *  | update (range)  | O(log n) |
   *  | prefix          | O(log n) |
   *  | query           | O(log n) |
   *  | get / set       | O(log n) |
   *  | Space           | O(n)     |
   *
   *  @par Example
   *  @code
   *  Range_Fenwick_Tree<int> ft(8);
   *  ft.update(1, 4, 3);         // a[1..4] += 3
   *  ft.update(2, 6, 5);         // a[2..6] += 5
   *  int s = ft.query(0, 7);     // sum of entire array
   *  int v = ft.get(3);          // value at index 3 = 3 + 5 = 8
   *  @endcode
   *
   *  @see Gen_Fenwick_Tree  Point-update / range-query variant.
   *  @see Fenwick_Tree      Arithmetic specialisation with find_kth.
   *  @ingroup Sequences
   */
  template <FenwickArithmetic T>
  class Range_Fenwick_Tree
  {
   Gen_Fenwick_Tree<T> b1; // stores differences d[i]
   Gen_Fenwick_Tree<T> b2; // stores d[i] * i
   size_t n = 0;

    /// Internal: prefix sum P(i) = (i+1)*B1.prefix(i) - B2.prefix(i)
    T prefix_sum(size_t i) const
    {
      return static_cast<T>(i + 1) * b1.prefix(i) - b2.prefix(i);
    }

    /// Rebuild b1 and b2 from a difference array stored in `d`.
    void build_from_diffs(const Array<T> & d)
    {
      auto d2 = Array<T>::create(n);
      d2(0) = T();
      for (size_t i = 1; i < n; ++i)
        d2(i) = d(i) * static_cast<T>(i);

      Gen_Fenwick_Tree<T> new_b1(d);
      Gen_Fenwick_Tree<T> new_b2(d2);
      b1.swap(new_b1);
      b2.swap(new_b2);
    }

  public:
    using Item_Type = T;

    /** @brief Construct a tree with @p num elements, all zero. */
    Range_Fenwick_Tree(const size_t num)
      : b1(num), b2(num), n(num)
    {
      // empty — all elements are identity
    }

    /** @brief Construct from an initializer list in O(n) time.
     *
     *  @par Example
     *  @code
     *  Range_Fenwick_Tree<int> ft = {1, 2, 3, 4, 5};
     *  ft.update(0, 4, 10);  // add 10 to all elements
     *  @endcode
     */
    Range_Fenwick_Tree(std::initializer_list<T> il)
      : b1(il.size()), b2(il.size()), n(il.size())
    {
      if (n == 0)
        return;

      auto d = Array<T>::create(n);
      auto it = il.begin();
      d(0) = *it;
      T prev = *it++;
      for (size_t i = 1; i < n; ++i, ++it)
        {
          d(i) = *it - prev;
          prev = *it;
        }
      build_from_diffs(d);
    }

    /** @brief Construct from an Array\<T\> in O(n) time.
     *
     *  @param values source array with the initial element values.
     */
    Range_Fenwick_Tree(const Array<T> & values)
      : b1(values.size()), b2(values.size()), n(values.size())
    {
      if (n == 0)
        return;

      auto d = Array<T>::create(n);
      d(0) = values(0);
      for (size_t i = 1; i < n; ++i)
        d(i) = values(i) - values(i - 1);
      build_from_diffs(d);
    }

    Range_Fenwick_Tree(const Range_Fenwick_Tree &) = default;

    Range_Fenwick_Tree(Range_Fenwick_Tree &&) noexcept = default;

    Range_Fenwick_Tree &operator=(const Range_Fenwick_Tree &) = default;

    Range_Fenwick_Tree &operator=(Range_Fenwick_Tree &&) noexcept = default;

    /** @brief Range update: add @p delta to every `a[i]` with
     *  `l <= i <= r`.
     *
     *  @param l     0-based left index (inclusive).
     *  @param r     0-based right index (inclusive).
     *  @param delta value to add to each element in `[l, r]`.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    void update(size_t l, const size_t r, const T & delta)
    {
      ah_out_of_range_error_if(l > r)
        << "Range_Fenwick_Tree::update: l=" << l << " > r=" << r;
      ah_out_of_range_error_if(r >= n)
        << "Range_Fenwick_Tree::update: r=" << r << " >= size " << n;

      b1.update(l, delta);
      b2.update(l, delta * static_cast<T>(l));

      if (r + 1 < n)
        {
          T neg = -delta;
          b1.update(r + 1, neg);
          b2.update(r + 1, neg * static_cast<T>(r + 1));
        }
    }

    /** @brief Point update: add @p delta to `a[i]`.
     *
     *  Convenience shorthand for `update(i, i, delta)`.
     */
    void point_update(const size_t i, const T & delta)
    {
      update(i, i, delta);
    }

    /** @brief Prefix query: sum of `a[0..i]`.
     *
     *  @param i 0-based index.
     *  @return  accumulated sum from index 0 through i (inclusive).
     *  @throw std::out_of_range if `i >= size()`.
     */
    T prefix(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n)
        << "Range_Fenwick_Tree::prefix: index " << i << " >= size " << n;

      return prefix_sum(i);
    }

    /** @brief Range query: sum of `a[l..r]`.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @return  accumulated sum over `[l, r]`.
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= n)
        << "Range_Fenwick_Tree::query: r=" << r << " >= n=" << n;
      ah_out_of_range_error_if(l > r)
        << "Range_Fenwick_Tree::query: l=" << l << " > r=" << r;

      return l > 0 ? prefix_sum(r) - prefix_sum(l - 1) : prefix_sum(r);
    }

    /** @brief Retrieve the logical value `a[i]`.
     *
     *  Equivalent to `query(i, i)`.  O(log n).
     */
    T get(const size_t i) const
    {
      return query(i, i);
    }

    /** @brief Set `a[i] = value`.
     *
     *  Computes the required delta and calls point_update().  O(log n).
     */
    void set(const size_t i, const T & value)
    {
      point_update(i, value - get(i));
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
    void swap(Range_Fenwick_Tree & other) noexcept
    {
      b1.swap(other.b1);
      b2.swap(other.b2);
      std::swap(n, other.n);
    }
  };
} // end namespace Aleph

# endif /* TPL_FENWICK_TREE_H */
