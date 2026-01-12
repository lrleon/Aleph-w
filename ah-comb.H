/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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
    - Check for existence or universality (`exists_perm`, `all_perm`).

    @note "Permutation" in this context means the cartesian product of the input lists,
          not mathematical permutations of a set.

    @ingroup Algorithms
 *  @author Leandro Rabindranath León
*/

# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynArray.H>
# include <tpl_array.H>
# include <tpl_dynSetTree.H>
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
    DynList<IT> its; { // This block allows getting a constant copy of l and then reverse
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
}

# endif // COMB_H
