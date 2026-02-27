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

# ifndef COMB_H
# define COMB_H

/** @file ah-comb.H
    @brief Combinatorics utilities: permutations, combinations and matrix transposition.

    This header provides functions to:
    - Transpose matrices represented as nested containers (`transpose`, `in_place_transpose`).
    - Enumerate permutations (cartesian products) from a list of lists (`traverse_perm`,
      `for_each_perm`, `build_perms`).
    - Build unique sorted combinations (`build_combs`).
    - Fold over permutations (`fold_perm`).
    - Count permutations (`perm_count`).
    - Generate next lexicographic permutations on arrays (`next_permutation`).
    - Generate k-combinations by index progression (`next_combination_indices`).
    - Generate fixed-popcount bitmask combinations (`next_combination_mask`).
    - Enumerate materialized k-combinations (`for_each_combination`,
      `build_combinations`).
    - Count combinations with overflow checks (`combination_count`).
    - Check for existence or universality (`exists_perm`, `all_perm`).

    @note `traverse_perm`/`for_each_perm`/`build_perms` use "permutation" in the
          cartesian-product sense. `next_permutation` is the classical
          lexicographic permutation of a single sequence.

    @ingroup Algorithms
 *  @author Leandro Rabindranath León
*/

# include <algorithm>
# include <bit>
# include <cstdint>
# include <limits>
# include <numeric>

# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynArray.H>
# include <tpl_array.H>
# include <tpl_dynSetTree.H>
# include <ahFunction.H>
# include <ahSort.H>

namespace Aleph
{
  /// @cond INTERNAL
  namespace comb_detail
  {
    /** Transpose a matrix represented as a list of lists (internal helper).

        This is an internal helper optimized for Aleph's list internals.
        The input is expected to be rectangular (all rows with the same length).

        @tparam T Element type.
        @param[in] l Matrix as `DynList<DynList<T>>`.
        @return The transposed matrix.

        @note If `l` is empty, an empty matrix is returned.
        @note In debug builds, non-rectangular inputs trigger an assertion.
        @internal
    */
    template <typename T>
    [[nodiscard]] inline
    DynList<DynList<T>> transpose_impl(DynList<DynList<T>> & l)
    {
      if (l.is_empty())
        return {};

      Array<Array<Snodenc<T> *>> mat;

      size_t ncol = 0; {
        const HTList & lrow = l.get_first();
        Array<Snodenc<T> *> row;
        for (HTList::Iterator it(lrow); it.has_curr(); it.next_ne(), ++ncol)
          row.append(static_cast<Snodenc<T> *>(it.get_curr()));
        mat.append(std::move(row));
      }

      size_t nrow = 1;
      for (auto row_it = l.get_it(1); row_it.has_curr(); row_it.next_ne(), ++nrow)
        {
          const HTList & lrow = row_it.get_curr();
          Array<Snodenc<T> *> row;
          row.reserve(ncol);
          size_t col = 0;
          for (HTList::Iterator it(lrow); it.has_curr(); it.next_ne(), ++col)
            row.append(static_cast<Snodenc<T> *>(it.get_curr()));

          assert(col == ncol);

          mat.append(std::move(row));
        }

      DynList<DynList<T>> ret;
      for (size_t j = 0; j < ncol; ++j)
        {
          DynList<T> row;
          for (size_t i = 0; i < nrow; ++i)
            row.append(mat(i)(j)->get_data());

          ret.append(std::move(row));
        }

      return ret;
    }

    template <class ArrayLike>
    inline void reverse_range(ArrayLike & a,
                              size_t left, size_t right) noexcept
    {
      while (left < right)
        {
          std::swap(a(left), a(right));
          ++left;
          --right;
        }
    }

    template <class IndexArray>
    inline void validate_combination_indices(const IndexArray & idx,
                                             const size_t n)
    {
      const size_t k = idx.size();
      ah_domain_error_if(k > n)
        << "next_combination_indices: k=" << k << " cannot exceed n=" << n;

      for (size_t i = 0; i < k; ++i)
        {
          ah_out_of_range_error_if(idx(i) >= n)
            << "next_combination_indices: index " << idx(i)
            << " at position " << i << " is outside [0, " << n << ")";

          if (i > 0)
            ah_domain_error_if(idx(i - 1) >= idx(i))
              << "next_combination_indices: indices must be strictly increasing";
        }
    }

    template <class ArrayLike, class Compare>
    [[nodiscard]] inline bool next_permutation_impl(ArrayLike & a,
                                                    Compare cmp,
                                                    const bool reset_on_last)
    {
      const size_t n = a.size();
      if (n < 2)
        return false;

      size_t pivot = n;
      for (size_t i = n - 1; i > 0; --i)
        if (cmp(a(i - 1), a(i)))
          {
            pivot = i - 1;
            break;
          }

      if (pivot == n)
        {
          if (reset_on_last)
            reverse_range(a, 0, n - 1);
          return false;
        }

      size_t succ = n - 1;
      while (not cmp(a(pivot), a(succ)))
        --succ;

      std::swap(a(pivot), a(succ));
      reverse_range(a, pivot + 1, n - 1);
      return true;
    }

    template <class IndexArray>
    [[nodiscard]] inline bool next_combination_indices_impl(IndexArray & idx,
                                                            const size_t n,
                                                            const bool reset_on_last)
    {
      validate_combination_indices(idx, n);

      const size_t k = idx.size();
      if (k == 0)
        return false;

      for (size_t pos = k; pos > 0; --pos)
        {
          const size_t i = pos - 1;
          const size_t max_here = n - (k - i);
          if (idx(i) < max_here)
            {
              ++idx(i);
              for (size_t j = i + 1; j < k; ++j)
                idx(j) = idx(j - 1) + 1;
              return true;
            }
        }

      if (reset_on_last)
        for (size_t i = 0; i < k; ++i)
          idx(i) = i;

      return false;
    }
  } // namespace comb_detail
  /// @endcond

  /** Transpose a matrix represented as a list of lists.

      The input is expected to be rectangular (all rows with the same length).

      @tparam T Element type.
      @param[in] l Matrix as `DynList<DynList<T>>`.
      @return The transposed matrix.

      @note If `l` is empty, an empty matrix is returned.
      @note In debug builds, non-rectangular inputs trigger an assertion.

      @ingroup Algorithms
  */
  template <typename T>
  [[nodiscard]] inline
  DynList<DynList<T>> transpose(const DynList<DynList<T>> & l)
  {
    if (l.is_empty())
      return {};

    Array<Array<T>> mat;

    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      mat.append(it.get_curr());

    const size_t nrow = mat.size();
    const size_t ncol = mat[0].size();

    for (size_t i = 1; i < nrow; ++i)
      assert(mat[i].size() == ncol);

    DynList<DynList<T>> ret;

    for (size_t j = 0; j < ncol; ++j)
      {
        DynList<T> row;
        for (size_t i = 0; i < nrow; ++i)
          row.append(mat(i)(j));

        ret.append(std::move(row));
      }

    return ret;
  }


  /** In-place transpose of a rectangular matrix stored as a nested container.

      The matrix is represented as a container of rows (`C<C<T>>`). This
      implementation moves elements into a temporary matrix and then swaps.

      @tparam C Container template with:
        - `size()`, `reserve(size_t)`, `append(...)`, `swap(...)`,
        - random-access via `operator()(size_t)` returning a row, and
        - the row must support `size()`, `reserve(size_t)`, `append(...)`, and
          random-access via `operator()(size_t)`.
      @tparam T Element type.
      @param[in,out] l Matrix to transpose.

      @note If `l` is empty, the function is a no-op.
      @note In debug builds, non-rectangular inputs trigger an assertion.

      @ingroup Algorithms
  */
  template <template <typename> class C, typename T>
  inline
  void in_place_transpose(C<C<T>> & l)
  {
    C<C<T>> mat;

    const size_t nrow = l.size();
    if (nrow == 0)
      return;

    const size_t ncol = l.get_first().size();

    for (size_t i = 0; i < nrow; ++i)
      assert(l(i).size() == ncol);

    for (size_t j = 0; j < ncol; ++j)
      {
        C<T> row;
        for (size_t i = 0; i < nrow; ++i)
          row.append(std::move(l(i)(j)));
        mat.append(std::move(row));
      }
    l.swap(mat);
  }

  /** In-place transpose of a matrix stored as `DynList<DynList<T>>`.

      This overload preserves list nodes by physically moving the internal
      `Slinknc` nodes instead of copying values.

      @tparam T Element type.
      @param[in,out] l Matrix to transpose.

      @note If `l` is empty, the function is a no-op.
      @note In debug builds, non-rectangular inputs trigger an assertion.

      @ingroup Algorithms
  */
  template <typename T>
  inline
  void in_place_transpose(DynList<DynList<T>> & l)
  {
    if (l.is_empty())
      return;

    Array<Array<Slinknc *>> mat;

    size_t ncol = 0; {
      DynList<T> lrow = l.remove_first();
      Array<Slinknc *> row;
      for (; not lrow.is_empty(); ++ncol)
        row.append(lrow.remove_head());
      mat.append(std::move(row));
    }

    size_t nrow = 1;
    for (; not l.is_empty(); ++nrow)
      {
        DynList<T> lrow = l.remove_first();
        Array<Slinknc *> row;
        row.reserve(ncol);
        size_t col = 0;
        while (not lrow.is_empty())
          {
            row.append(lrow.remove_head());
            ++col;
          }

        assert(col == ncol);

        mat.append(std::move(row));
      }

    assert(l.is_empty());

    for (size_t j = 0; j < ncol; ++j)
      {
        DynList<T> row;
        for (size_t i = 0; i < nrow; ++i)
          {
            Slinknc *node_ptr = mat(i)(j);
            row.HTList::append(static_cast<Snodenc<T> *>(node_ptr));
          }
        l.append(std::move(row));
      }
  }

  /// @cond INTERNAL
  /** Internal recursive engine for `traverse_perm`.

      This routine performs a depth-first traversal over the cartesian product
      of the input lists. It is not part of the public API.

      @tparam T Element type.
      @tparam Op Callable used by `traverse_perm`.
      @param[in,out] sample Partial permutation being built.
      @param[in,out] its    Iterators (one per input list) remaining to expand.
      @param[in,out] op     Operation applied to each final permutation.
      @return `false` if `op` requested early termination; `true` otherwise.
      @internal
  */
  template <typename T, class Op>
  static inline
  bool traverse_perm_impl(DynList<T> & sample,
                          DynList<typename DynList<T>::Iterator> & its, Op & op)
  {
    if (its.is_empty())
      return op(sample.template maps<T>([](const T & i) { return i; }));

    auto itor = its.remove_first();
    for (auto it = itor; it.has_curr(); it.next_ne())
      {
        auto item = it.get_curr();
        sample.insert(item);
        if (not traverse_perm_impl(sample, its, op))
          {
            sample.remove_first();
            its.insert(itor);
            return false;
          }
        sample.remove_first();
      }
    its.insert(itor);

    return true;
  }

  /// @endcond

  /** Traverse all the possible permutations that can be done of a list
      of lists and on each permutation performs an operation.

      `traverse_perm(l, op)` builds on line, one to one, each possible
      permutation between the lists stored at the list of lists `l`
      which form is `{l1, l2, ..., ln}`, where each item `li` is a list
      of arbitrary size.

      On each permutation seen, a list `{a1, a2, ..., an}` is built
      where `a1` belongs to `l1, `a2` to `l2` and so on. Afterward the
      operation `op({a1, a2, ..., an})` is performed. If `op` returns
      `true`, then `traverse_perm()` advances forward to the next
      permutation. Otherwise (`op()` returns `false`) the entire process
      is stopped with return value `false`.

      `op()` must be a function, functor o lambda with the following
      signature:

      bool op(const DynList<T> & perm)

      where `perm` would be a permutation. If `op()` returns `true`, then
      the process continues forward the next permutation. Otherwise, the
      process is stopped.

      The algorithm is very conservative in memory. The size of `l` is
      the maximum recursion depth and also the heap memory consumption.

      @param[in] l a list of lists of items of generic type `T`
      @param[in] op an operation to be performed on each permutation.
      @return `true` if all permutation were traversed,; `false`
      otherwise. Note that `true` is returned only if `op()` always
      returned `true`.

      @ingroup Algorithms
  */
  template <typename T, class Op>
  inline
  bool traverse_perm(const DynList<DynList<T>> & l, Op & op)
  {
    using IT = typename DynList<T>::Iterator;
    DynList<IT> its;

    { // This block allows getting a constant copy of l and then reverse
      // it. At the end of block lcpy memory is freed
      const DynList<IT> lcpy =
          l.template maps<IT>([](const auto & l) { return l.get_it(); });
      its = lcpy.rev();
    }

    DynList<T> ll;
    return traverse_perm_impl(ll, its, op);
  }

  /** \overload traverse_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename T, class Op>
  inline
  bool traverse_perm(const DynList<DynList<T>> & l, Op && op)
  {
    return traverse_perm(l, op);
  }

  /** Apply a procedure to every permutation produced by `traverse_perm`.

      Unlike `traverse_perm`, the operation cannot stop the traversal early.

      @tparam T Element type.
      @tparam Op Callable with signature `void(const DynList<T>&)`.
      @param[in] l List of lists.
      @param[in] op Operation to run on each permutation.

      @ingroup Algorithms
  */
  template <typename T, class Op>
  inline
  void for_each_perm(const DynList<DynList<T>> & l, Op & op)
  {
    traverse_perm(l, [&op](const auto & row)
                    {
                      op(row);
                      return true;
                    });
  }

  /** \overload for_each_perm

      Convenience overload accepting a rvalue callable.
  */
  template <typename T, class Op>
  inline
  void for_each_perm(const DynList<DynList<T>> & l, Op && op)
  {
    return for_each_perm(l, op);
  }

  /** Materialize all permutations from a list of lists.

      @tparam T Element type.
      @param[in] l List of lists.
      @return A list containing all permutations.

      @note The output size is the product of input list sizes.

      @ingroup Algorithms
  */
  template <typename T>
  [[nodiscard]]
  DynList<DynList<T>> build_perms(const DynList<DynList<T>> & l)
  {
    DynList<DynList<T>> ret;
    for_each_perm(l, [&ret](const DynList<T> & perm) { ret.append(perm); });
    return ret;
  }

  /** Build the set of unique combinations from a list of lists.

      Each permutation is sorted (to remove ordering) and inserted into a set,
      so duplicates are eliminated.

      @tparam T Element type. Must be sortable by `sort()` and comparable by
        `CmpContainer`.
      @param[in] l List of lists.
      @return Unique sorted combinations.

      @ingroup Algorithms
  */
  template <typename T>
  [[nodiscard]]
  DynList<DynList<T>> build_combs(const DynList<DynList<T>> & l)
  {
    DynSetTree<DynList<T>, Avl_Tree, CmpContainer<DynList<T>, T>> combs;

    for_each_perm(l, [&combs](const DynList<T> & perm)
                    {
                      DynList<T> comb = sort(perm);
                      combs.insert(std::move(comb));
                    });

    return combs.
        template maps<DynList<T>>([](const DynList<T> & comb) { return comb; });
  }

  /** Left-fold over all permutations.

      For each permutation `p`, the accumulator is updated as:
      `acu = op(acu, p)`.

      @tparam T Accumulator type.
      @tparam Tc Element type stored in the input lists.
      @tparam Op Callable with signature `T op(T acu, const DynList<Tc>& perm)`.
      @param[in] init Initial accumulator value.
      @param[in] l List of lists.
      @param[in] op Folding operation.
      @return Final accumulator.

      @ingroup Algorithms
  */
  template <typename T, typename Tc, class Op = Dft_Fold_Op<Tc, T>>
  [[nodiscard]]
  T fold_perm(const T & init, const DynList<DynList<Tc>> & l, Op & op)
  {
    T acu = init;
    traverse_perm(l, [&op, &acu](const auto & l)
                    {
                      acu = op(acu, l);
                      return true;
                    });
    return acu;
  }

  /** \overload fold_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename T, typename Tc, class Op = Dft_Fold_Op<Tc, T>>
  [[nodiscard]]
  T fold_perm(const T & init, const DynList<DynList<Tc>> & l, Op && op)
  {
    return fold_perm(init, l, op);
  }

  /** Count the total number of permutations from a list of lists.

      The result is the product of all list sizes. If the input is empty,
      returns 1 (there is exactly one empty permutation).

      @tparam T Element type.
      @param[in] l List of lists.
      @return Total permutation count.

      @ingroup Algorithms
  */
  template <typename T>
  [[nodiscard]]
  size_t perm_count(const DynList<DynList<T>> & l)
  {
    size_t count = 1;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      {
        const size_t sz = it.get_curr().size();
        if (sz == 0)
          return 0; // Any empty list results in no permutations
        count *= sz;
      }
    return count;
  }

  /** Check if any permutation satisfies a predicate.

      Stops as soon as a permutation makes `pred` return `true`.

      @tparam T Element type.
      @tparam Pred Callable with signature `bool(const DynList<T>&)`.
      @param[in] l List of lists.
      @param[in] pred Predicate to test.
      @return `true` if at least one permutation satisfies `pred`.

      @ingroup Algorithms
  */
  template <typename T, class Pred>
  [[nodiscard]]
  bool exists_perm(const DynList<DynList<T>> & l, Pred & pred)
  {
    bool found = false;
    traverse_perm(l, [&pred, &found](const DynList<T> & perm)
                    {
                      if (pred(perm))
                        {
                          found = true;
                          return false; // stop
                        }
                      return true;
                    });
    return found;
  }

  /** \overload exists_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename T, class Pred>
  [[nodiscard]]
  bool exists_perm(const DynList<DynList<T>> & l, Pred && pred)
  {
    return exists_perm(l, pred);
  }

  /** Check if all permutations satisfy a predicate.

      Stops as soon as a permutation makes `pred` return `false`.

      @tparam T Element type.
      @tparam Pred Callable with signature `bool(const DynList<T>&)`.
      @param[in] l List of lists.
      @param[in] pred Predicate to test.
      @return `true` if all permutations satisfy `pred`.

      @ingroup Algorithms
  */
  template <typename T, class Pred>
  [[nodiscard]]
  bool all_perm(const DynList<DynList<T>> & l, Pred & pred)
  {
    return traverse_perm(l, pred);
  }

  /** \overload all_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename T, class Pred>
  [[nodiscard]]
  bool all_perm(const DynList<DynList<T>> & l, Pred && pred)
  {
    return all_perm(l, pred);
  }

  /** Check if no permutation satisfies a predicate.

      Equivalent to `!exists_perm(l, pred)`.

      @tparam T Element type.
      @tparam Pred Callable with signature `bool(const DynList<T>&)`.
      @param[in] l List of lists.
      @param[in] pred Predicate to test.
      @return `true` if no permutation satisfies `pred`.

      @ingroup Algorithms
  */
  template <typename T, class Pred>
  [[nodiscard]]
  bool none_perm(const DynList<DynList<T>> & l, Pred & pred)
  {
    return not exists_perm(l, pred);
  }

  /** \overload none_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename T, class Pred>
  [[nodiscard]]
  bool none_perm(const DynList<DynList<T>> & l, Pred && pred)
  {
    return none_perm(l, pred);
  }

  /** Filter permutations that satisfy a predicate.

      @tparam T Element type.
      @tparam Pred Callable with signature `bool(const DynList<T>&)`.
      @param[in] l List of lists.
      @param[in] pred Predicate to select permutations.
      @return List of permutations for which `pred` returned `true`.

      @ingroup Algorithms
  */
  template <typename T, class Pred>
  [[nodiscard]]
  DynList<DynList<T>> filter_perm(const DynList<DynList<T>> & l, Pred & pred)
  {
    DynList<DynList<T>> ret;
    for_each_perm(l, [&ret, &pred](const DynList<T> & perm)
                    {
                      if (pred(perm))
                        ret.append(perm);
                    });
    return ret;
  }

  /** \overload filter_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename T, class Pred>
  [[nodiscard]]
  DynList<DynList<T>> filter_perm(const DynList<DynList<T>> & l, Pred && pred)
  {
    return filter_perm(l, pred);
  }

  /** Transform each permutation via a mapping operation.

      @tparam R Result element type.
      @tparam T Element type in input lists.
      @tparam Op Callable with signature `R(const DynList<T>&)`.
      @param[in] l List of lists.
      @param[in] op Mapping operation.
      @return List of results from applying `op` to each permutation.

      @ingroup Algorithms
  */
  template <typename R, typename T, class Op>
  [[nodiscard]]
  DynList<R> map_perm(const DynList<DynList<T>> & l, Op & op)
  {
    DynList<R> ret;
    for_each_perm(l, [&ret, &op](const DynList<T> & perm)
                    {
                      ret.append(op(perm));
                    });
    return ret;
  }

  /** \overload map_perm

      Convenience overload accepting an rvalue callable.
  */
  template <typename R, typename T, class Op>
  [[nodiscard]]
  DynList<R> map_perm(const DynList<DynList<T>> & l, Op && op)
  {
    return map_perm<R>(l, op);
  }

  /** Compute the next lexicographic permutation of an `Array`.

      This function follows the classical linear-time pivot/successor/suffix
      algorithm.

      - If a next permutation exists, it is written in-place and `true` is
        returned.
      - If the input is already the last permutation:
        - when `reset_on_last == true`, the array is reset to the first
          permutation and `false` is returned;
        - when `reset_on_last == false`, the array is left unchanged and
          `false` is returned.

      Duplicate values are fully supported. Starting from a sorted array yields
      each distinct permutation exactly once.

      @tparam T Element type.
      @tparam Compare Strict weak ordering comparator.
      @param[in,out] a Sequence to transform.
      @param[in] cmp Ordering relation.
      @param[in] reset_on_last Whether to reset to the first permutation.
      @return `true` if advanced to a new permutation; `false` otherwise.

      @note Complexity: O(n) per step.

      @ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  [[nodiscard]] inline bool
  next_permutation(Array<T> & a,
                   Compare cmp = Compare(),
                   const bool reset_on_last = true)
  {
    return comb_detail::next_permutation_impl(a, cmp, reset_on_last);
  }

  /** \overload next_permutation for `DynArray`. */
  template <typename T, class Compare = Aleph::less<T>>
  [[nodiscard]] inline bool
  next_permutation(DynArray<T> & a,
                   Compare cmp = Compare(),
                   const bool reset_on_last = true)
  {
    return comb_detail::next_permutation_impl(a, cmp, reset_on_last);
  }

  /** Compute `n choose k` with overflow checks.

      @param[in] n Universe size.
      @param[in] k Selection size.
      @return The exact binomial coefficient C(n, k), or 0 when `k > n`.

      @throws ah_runtime_error if the exact result does not fit in `size_t`.

      @ingroup Algorithms
  */
  [[nodiscard]] inline size_t combination_count(size_t n, size_t k)
  {
    if (k > n)
      return 0;

    k = std::min(k, n - k);
    if (k == 0)
      return 1;

    size_t result = 1;
    for (size_t i = 1; i <= k; ++i)
      {
        size_t num = n - k + i;
        size_t den = i;

        size_t g = std::gcd(num, den);
        num /= g;
        den /= g;

        g = std::gcd(result, den);
        result /= g;
        den /= g;

        g = std::gcd(num, den);
        num /= g;
        den /= g;

        ah_runtime_error_if(den != 1)
          << "combination_count: internal reduction failure for C("
          << n << ", " << k << ")";

        ah_runtime_error_if(result > std::numeric_limits<size_t>::max() / num)
          << "combination_count: overflow for C(" << n << ", " << k << ")";
        result *= num;
      }

    return result;
  }

  /** Advance an index-combination `[i0 < i1 < ... < i(k-1)]` to the next one.

      `idx` must contain valid strictly increasing indices in `[0, n)`.

      @param[in,out] idx Current combination indices.
      @param[in] n Universe size.
      @param[in] reset_on_last Whether to reset to `{0,1,...,k-1}` at the end.
      @return `true` if advanced; `false` if `idx` was already the last one.

      @throws ah_domain_error if `idx` is not strictly increasing.
      @throws ah_out_of_range_error if an index is outside `[0, n)`.

      @note Complexity: O(k) per step.

      @ingroup Algorithms
  */
  [[nodiscard]] inline bool
  next_combination_indices(Array<size_t> & idx,
                           const size_t n,
                           const bool reset_on_last = true)
  {
    return comb_detail::next_combination_indices_impl(idx, n, reset_on_last);
  }

  /** \overload next_combination_indices for `DynArray<size_t>`. */
  [[nodiscard]] inline bool
  next_combination_indices(DynArray<size_t> & idx,
                           const size_t n,
                           const bool reset_on_last = true)
  {
    return comb_detail::next_combination_indices_impl(idx, n, reset_on_last);
  }

  /** Build the first k-of-64 combination mask (`k` low bits set). */
  [[nodiscard]] inline uint64_t first_combination_mask(const size_t k)
  {
    ah_out_of_range_error_if(k > 64)
      << "first_combination_mask: k=" << k << " cannot exceed 64";

    if (k == 0)
      return 0;
    if (k == 64)
      return std::numeric_limits<uint64_t>::max();
    return (uint64_t(1) << k) - 1;
  }

  /** Advance a fixed-popcount bitmask to the next combination (Gosper hack).

      The mask is interpreted over the low `n` bits.

      @param[in,out] mask Current mask.
      @param[in] n Number of active low bits (domain size).
      @param[in] reset_on_last Whether to reset to the first mask when done.
      @return `true` if advanced; `false` if already last within `n` bits.

      @throws ah_out_of_range_error if `n > 64`.
      @throws ah_domain_error if `mask` has bits outside the low `n` bits.

      @note Complexity: O(1) arithmetic operations.

      @ingroup Algorithms
  */
  [[nodiscard]] inline bool
  next_combination_mask(uint64_t & mask,
                        const size_t n,
                        const bool reset_on_last = true)
  {
    ah_out_of_range_error_if(n > 64)
      << "next_combination_mask: n=" << n << " cannot exceed 64";

    if (n == 0)
      {
        ah_domain_error_if(mask != 0)
          << "next_combination_mask: for n=0, mask must be 0";
        return false;
      }

    const uint64_t domain_mask =
      n == 64 ? std::numeric_limits<uint64_t>::max() : ((uint64_t(1) << n) - 1);

    ah_domain_error_if(mask & ~domain_mask)
      << "next_combination_mask: mask has bits outside the low n bits";

    const size_t k = std::popcount(mask);
    if (k == 0)
      {
        if (reset_on_last)
          mask = 0;
        return false;
      }

    const uint64_t c = mask & (~mask + 1); // isolate least significant set bit
    const uint64_t r = mask + c;

    if (r == 0)
      {
        if (reset_on_last)
          mask = first_combination_mask(k);
        return false;
      }

    const uint64_t next = (((r ^ mask) >> 2) / c) | r;
    if (next & ~domain_mask)
      {
        if (reset_on_last)
          mask = first_combination_mask(k);
        return false;
      }

    mask = next;
    return true;
  }

  /// @cond INTERNAL
  template <class ValuesArray, class Op>
  [[nodiscard]] static inline bool
  for_each_combination_impl(const ValuesArray & values,
                            const size_t k,
                            Op & op)
  {
    const size_t n = values.size();
    ah_domain_error_if(k > n)
      << "for_each_combination: k=" << k << " cannot exceed n=" << n;

    if (k == 0)
      return op(Array<typename ValuesArray::Item_Type>());

    Array<size_t> idx = Array<size_t>::create(k);
    for (size_t i = 0; i < k; ++i)
      idx(i) = i;

    while (true)
      {
        Array<typename ValuesArray::Item_Type> comb = Array<typename ValuesArray::Item_Type>::create(k);
        for (size_t i = 0; i < k; ++i)
          comb(i) = values(idx(i));

        if (not op(comb))
          return false;

        if (not next_combination_indices(idx, n, false))
          break;
      }

    return true;
  }
  /// @endcond

  /** Enumerate all k-combinations of an `Array<T>` in lexicographic index order.

      The callback receives each materialized combination as `Array<T>` and can
      stop early by returning `false`.

      @tparam T Element type.
      @tparam Op Callable with the signature `bool(const Array<T>&)`.
      @param[in] values Source values.
      @param[in] k Combination size.
      @param[in] op Callback invoked for each combination.
      @return `true` if fully traversed; `false` if the callback stopped early.

      @ingroup Algorithms
  */
  template <typename T, class Op>
  [[nodiscard]] inline bool
  for_each_combination(const Array<T> & values, const size_t k, Op & op)
  {
    return for_each_combination_impl(values, k, op);
  }

  /** \overload for_each_combination for rvalue callback. */
  template <typename T, class Op>
  [[nodiscard]] inline bool
  for_each_combination(const Array<T> & values, const size_t k, Op && op)
  {
    return for_each_combination(values, k, op);
  }

  /** \overload for_each_combination for `DynArray<T>`. */
  template <typename T, class Op>
  [[nodiscard]] inline bool
  for_each_combination(const DynArray<T> & values, const size_t k, Op & op)
  {
    return for_each_combination_impl(values, k, op);
  }

  /** \overload for_each_combination for `DynArray<T>` and rvalue callback. */
  template <typename T, class Op>
  [[nodiscard]] inline bool
  for_each_combination(const DynArray<T> & values, const size_t k, Op && op)
  {
    return for_each_combination(values, k, op);
  }

  /** Materialize all k-combinations of `Array<T>`.

      @tparam T Element type.
      @param[in] values Source values.
      @param[in] k Combination size.
      @return All combinations in lexicographic index order.

      @ingroup Algorithms
  */
  template <typename T>
  [[nodiscard]] inline Array<Array<T>>
  build_combinations(const Array<T> & values, const size_t k)
  {
    Array<Array<T>> ret;
    (void) for_each_combination(values, k, [&ret](const Array<T> & comb)
    {
      ret.append(comb);
      return true;
    });
    return ret;
  }

  /** \overload build_combinations for `DynArray<T>`. */
  template <typename T>
  [[nodiscard]] inline Array<Array<T>>
  build_combinations(const DynArray<T> & values, const size_t k)
  {
    Array<Array<T>> ret;
    (void) for_each_combination(values, k, [&ret](const Array<T> & comb)
    {
      ret.append(comb);
      return true;
    });
    return ret;
  }


  /** @brief Convert a binary number to its Gray code representation.
   *
   *  The i-th bit of the Gray code is 1 if the i-th and (i+1)-th bits of the
   *  binary number are different.
   *
   *  @tparam T Unsigned integral type.
   *  @param n Binary number.
   *  @return Gray code representation of n.
   *
   *  @ingroup Algorithms
   */
  template <std::unsigned_integral T>
  [[nodiscard]] constexpr T bin_to_gray(const T n) noexcept
  {
    return n ^ (n >> 1);
  }


  /** @brief Convert a Gray code number to its binary representation.
   *
   *  The conversion is performed by iteratively XORing the Gray code with
   *  right-shifted versions of itself.
   *
   *  @tparam T Unsigned integral type.
   *  @param g Gray code number.
   *  @return Binary representation of g.
   *
   *  @ingroup Algorithms
   */
  template <std::unsigned_integral T>
  [[nodiscard]] constexpr T gray_to_bin(T g) noexcept
  {
    for (size_t shift = 1; shift < sizeof(T) * 8; shift <<= 1)
      g ^= (g >> shift);
    return g;
  }


  /** @brief Generate the sequence of n-bit Gray codes.
   *
   *  Returns an array containing all 2^n Gray codes in sequence, where
   *  each successive value differs by exactly one bit.
   *
   *  @param n Number of bits (must be <= 31 to fit in Array).
   *  @return Array of 2^n Gray codes.
   *
   *  @throws ah_domain_error if n > 31 (exceeds Array size limits).
   *
   *  @ingroup Algorithms
   */
  [[nodiscard]] inline Array<uint32_t> build_gray_code(const size_t n)
  {
    ah_domain_error_if(n > 31)
      << "build_gray_code: n=" << n << " exceeds 31-bit limit for Array";

    const size_t count = size_t(1) << n;
    Array<uint32_t> ret;
    ret.reserve(count);
    for (size_t i = 0; i < count; ++i)
      ret.append(bin_to_gray(static_cast<uint32_t>(i)));

    return ret;
  }
}

# endif // COMB_H
