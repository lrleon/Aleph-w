
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

# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynArray.H>
# include <tpl_array.H>
# include <tpl_dynSetTree.H>
# include <ahSort.H>

namespace Aleph
{

  template <typename T> inline
  DynList<DynList<T>> __transpose(const DynList<DynList<T>> & l)
  {
    Array<Array<Snodenc<T>*>> mat;

    size_t ncol = 0;
    {
      const HTList & lrow = l.get_first();
      Array<Snodenc<T>*> row;
      for (HTList::Iterator it(lrow); it.has_curr(); it.next_ne(), ++ncol)
        row.append(static_cast<Snodenc<T>*>(it.get_curr()));
      mat.append(std::move(row));
    }

    size_t nrow = 1;
    for (auto row_it = l.get_it(1); row_it.has_curr(); row_it.next_ne(), ++nrow)
      {
        const HTList & lrow = row_it.get_curr();
        Array<Snodenc<T>*> row; row.reserve(ncol);
        for (HTList::Iterator it(lrow); it.has_curr(); it.next_ne())
          row.append(static_cast<Snodenc<T>*>(it.get_curr()));

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

  template <typename T> inline
  DynList<DynList<T>> transpose(const DynList<DynList<T>> & l)
  {
    Array<Array<T>> mat;

    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      mat.append(it.get_curr());

    const size_t nrow = mat.size();
    const size_t ncol = mat[0].size();

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


  template <template <typename> class C, typename T> inline
  void in_place_transpose(C<C<T>> & l)
  {
    C<C<T>> mat;

    const size_t nrow = l.size();
    const size_t ncol = l.get_first().size();

    mat.reserve(ncol);

    for (size_t j = 0; j < ncol; ++j)
      {
        C<T> row;
        row.reserve(nrow);
        for (size_t i = 0; i < nrow; ++i)
          row.append(std::move(l(i)(j)));
        mat.append(std::move(row));
      }
    l.swap(mat);
  }

  template <typename T> inline
  void in_place_transpose(DynList<DynList<T>> & l)
  {
    Array<Array<Slinknc*>> mat;

    size_t ncol = 0;
    {
      DynList<T> lrow = l.remove_first();
      Array<Slinknc*> row;
      for (; not lrow.is_empty(); ++ncol)
        row.append(lrow.remove_head());
      mat.append(std::move(row));
    }

    size_t nrow = 1;
    for (; not l.is_empty(); ++nrow)
      {
        DynList<T> lrow = l.remove_first();
        Array<Slinknc*> row; row.reserve(ncol);
        while (not lrow.is_empty())
          row.append(lrow.remove_head());

        mat.append(std::move(row));
      }

    assert(l.is_empty());

    for (size_t j = 0; j < ncol; ++j)
      {
        DynList<T> row;
        for (size_t i = 0; i < nrow; ++i)
          {
            Slinknc * node_ptr = mat(i)(j);
            row.HTList::append(static_cast<Snodenc<T>*>(node_ptr));
          }
        l.append(std::move(row));
      }
  }

  template <typename T, class Op>
  static inline
  bool traverse_perm(DynList<T> & sample,
                     DynList<typename DynList<T>::Iterator> & its, Op & op)
  {
    if (its.is_empty())
      return op(sample.template maps<T>([] (const T & i) { return i; }));

    auto itor = its.remove_first();
    for (auto it = itor; it.has_curr(); it.next_ne())
      {
        auto item = it.get_curr();
        sample.insert(item);
        if (not traverse_perm(sample, its, op))
          return false;
        sample.remove_first();
      }
    its.insert(itor);

    return true;
  }

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

      bool op(cons DynList<T> & perm)

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

      @ingroup Algos
  */
  template <typename T, class Op> inline
  bool traverse_perm(const DynList<DynList<T>> & l, Op & op)
  {
    using IT = typename DynList<T>::Iterator;
    DynList<IT> its;

    { // This block allows to get a constant copy of l and then reverse
      // it. At the end of block lcpy memory is freed
      const DynList<IT> lcpy =
        l.template maps<IT>([] (const auto & l) { return l.get_it(); });
      its = lcpy.rev();
    }

    DynList<T> ll;
    return traverse_perm(ll, its, op);
  }

  /// \overload traverse_perm
  template <typename T, class Op> inline
  bool traverse_perm(const DynList<DynList<T>> & l, Op && op)
  {
    return traverse_perm(l, op);
  }

  template <typename T, class Op> inline
  void for_each_perm(const DynList<DynList<T>> & l, Op & op)
  {
    traverse_perm(l, [&op] (const auto & row)
    {
      op(row);
      return true;
    });
  }

  /// \overloadf traverse_perm
  template <typename T, class Op> inline
  void for_each_perm(const DynList<DynList<T>> & l, Op && op)
  {
    return for_each_perm(l, op);
  }

  template <typename T>
  DynList<DynList<T>> build_perms(const DynList<DynList<T>> & l)
  {
    DynList<DynList<T>> ret;
    for_each_perm(l, [&ret] (const DynList<T> & perm) { ret.append(perm); });
    return ret;
  }

  template <typename T>
  DynList<DynList<T>> build_combs(const DynList<DynList<T>> & l)
  {
    DynList<DynList<T>> perms;
    for_each_perm(l, [&perms] (const DynList<T> & perm) { perms.append(perm); });

    DynSetTree<DynList<T>, Avl_Tree, CmpContainer<DynList<T>, T>> combs;
    for (auto it = perms.get_it(); it.has_curr(); it.next())
      {
        DynList<T> perm = sort(it.get_curr());
        combs.insert(perm);
      }

    return combs.
      template maps<DynList<T>>([] (const DynList<T> & comb) { return comb; });
  }

  template <typename T, typename Tc, class Op = Dft_Fold_Op<Tc, T>>
  T fold_perm(const T & init, const DynList<DynList<Tc>> & l, Op & op)
  {
    T acu = init;
    traverse_perm(l, [&op, &acu] (const auto & l)
    {
      acu = op(acu, l);
      return true;
    });
    return acu;
  }

  template <typename T, typename Tc, class Op = Dft_Fold_Op<Tc, T>>
  T fold_perm(const T & init, const DynList<DynList<Tc>> & l, Op && op)
  {
    return fold_perm(init, l, op);
  }


}

# endif // COMB_H
