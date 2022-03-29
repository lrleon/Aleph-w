

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

  /// \overload traverse_perm
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


  // template <typename T> inline
  // DynList<DynList<T>> comb(const DynList<T> & items, const size_t n)
  // {
  
  // }

}

# endif // COMB_H
