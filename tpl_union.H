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
# ifndef TPL_UNION_H
# define TPL_UNION_H

# include <tpl_dynArray.H>
# include <tpl_dynSetTree.H>

# include <limits>
# include <stdexcept>

# include <ah-errors.H>


/** Binary relation between a set of integers.

    This class implements a static relation of equivalence between a set
    of n integers from 0 until n - 1.

    By static is understood that the number of items cannot change.

    The used algorithm is the quick weighted fast union which is very
    fast, bounded by \f$O(\lg n)\f$ but in the practice trends to be
    \f$O(1)\f$.

    This implementation is strongly based on java version of Sedgewick
    and Wayne

    @see Relation Relation_T
    @author Leandro Rabindranath Leon 
    @ingroup Grafos
 */
class Fixed_Relation
{
protected:
  DynArray<size_t> id;
  DynArray<size_t> sz;
  size_t num_blocks;

  void verify_index(const size_t i) const
  {
    ah_out_of_range_error_if(i >= id.size()) << "index out of range";
  }

  virtual size_t root(size_t i)
  {
    verify_index(i);
    while (i != id(i))
      {
        id(i) = id(id(i));
        i = id(i);
      }

    return i;
  }

  size_t depth(size_t i) const
  {
    verify_index(i);
    size_t d = 0;
    while (true)
      {
        ++d;
        const size_t parent = id(i);
        verify_index(parent);
        if (parent == i)
          return d;
        i = parent;
        if (d > id.size())
          ah_logic_error_if(d > id.size())
              << "cycle detected in union-find structure";
      }
  }

public:
  virtual ~Fixed_Relation() = default;

  /// Initialize an empty binary relation of integers between 0 and n - 1 
  Fixed_Relation(const size_t n = 0) : num_blocks(n)
  {
    if (num_blocks == 0)
      return;

    id.reserve(num_blocks);
    sz.reserve(num_blocks);
    for (size_t i = 0; i < num_blocks; ++i)
      {
        id(i) = i;
        sz(i) = 1;
      }
  }

  /** Set the number of items of the relation.

      The advantage of this method is that it allows constructing a
      Fixed_Relation without needing to know the number of
      items. Afterward, when these are known, this number could be set
      with this method.

      @param[in] n the number of items.
      @throw bad_alloc if there is not enough memory
   */
  void set_n(size_t n)
  {
    id.empty();
    sz.empty();
    num_blocks = n;
    if (num_blocks == 0)
      return;

    id.reserve(num_blocks);
    sz.reserve(num_blocks);
    for (size_t i = 0; i < num_blocks; ++i)
      {
        id(i) = i;
        sz(i) = 1;
      }
  }

  /// Return the number of items of the set (not of relation)
  size_t size() const noexcept { return id.size(); }

  /// Return the number of connected blocks, which is the number of
  /// equivalence classes
  size_t get_num_blocks() const noexcept { return num_blocks; }

  /** Return `true` if item `i` is related to item `j`.

      Note since the relation is symmetric saying that `i` is related to
      `j` is the same thing than saying that `j` is related to `i`.

      Basically `i` is related to `j` if it is possible to reach 'j`
      from 'i'.

      @param[in] i an item index
      @param[in] j an item index
      @return true if `i` is related to `j`; `false` otherwise
   */
  bool are_connected(size_t i, size_t j)
  {
    return root(i) == root(j);
  }

  /** Insert the pair '(i,j)' in the relation. All the reachability
      state is updated.

      @param[in] i an item index
      @param[in] j an item index
   */
  void join(size_t i, size_t j)
  {
    i = root(i);
    j = root(j);
    if (i == j)
      return;

    if (sz(i) < sz(j))
      {
        id(i) = j;
        sz(j) += sz(i);
      }
    else
      {
        id(j) = i;
        sz(i) += sz(j);
      }
    --num_blocks;
  }
};


/** Binary relation between a set of integers.

    This class implements a dynamic relation of equivalence between a set
    of n integers from 0 until n - 1.

    By dynamic is understood that the number of items can grow.

    The used algorithm is the quick weighted fast union which is very
    fast, bounded by \f$O(\lg n)\f$ but in the practice trends to be
    \f$O(1)\f$.

    This implementation is strongly based on java version of Sedgewick
    and Wayne

    @see Relation Relation_T
    @author Leandro Rabindranath Leon 
    @ingroup Grafos
 */
class Relation : public Fixed_Relation
{
  void verify_if_add_new_points(const size_t n)
  {
    const size_t l = size();
    if (n < l)
      return;

    id.reserve(l, n);
    sz.reserve(l, n);
    for (size_t i = l; i <= n; ++i)
      {
        id(i) = i;
        sz(i) = 1;
      }
    num_blocks += n - l + 1;
  }

  size_t root(size_t i) override
  {
    verify_if_add_new_points(i);
    return Fixed_Relation::root(i);
  }

public:
  /// Initialize an empty relation of n elements between [0..n)
  Relation(const size_t n = 0) : Fixed_Relation(n) {}
};


/** Binary relation between a set of items.

    This class implements a dynamic relation of equivalence between a set
    of n different items of type T

    The used algorithm is the quick weighted fast union which is very
    fast, bounded by \f$O(\lg n)\f$ but in the practice trends to be
    \f$O(1)\f$.

    This implementation is strongly based on java version of Sedgewick
    and Wayne

    @see Relation Relation_T
    @author Leandro Rabindranath Leon 
    @ingroup Grafos
 */
template <typename T, class Compare = Aleph::less<T>>
class Relation_T : public Relation
{
  struct Pair
  {
    T item;
    size_t i;

    Pair() noexcept {}

    Pair(const T & __item, size_t __i)
      : item(__item), i(__i)
    { /* empty */
    }
  };

  struct Cmp
  {
    bool operator ()(const Pair & p1, const Pair & p2) const noexcept
    {
      return Compare()(p1.item, p2.item);
    }
  };

  DynSetAvlTree<Pair, Cmp> items_tree;

  // returns the item ID; either because it is found or because it is inserted
  size_t test_and_insert_new_item(const T & item)
  {
    Pair p(item, std::numeric_limits<size_t>::max());
    Pair *ptr = items_tree.search_or_insert(p);
    if (ptr->i == std::numeric_limits<size_t>::max())
      {
        const size_t new_id = size();
        ptr->i = new_id;
        Relation::are_connected(new_id, new_id);
      }
    return ptr->i;
  }

public:
  /// Return true if p and q are connected; inserts missing items into the relation
  bool are_connected(const T & p, const T & q)
  {
    const size_t i = test_and_insert_new_item(p);
    const size_t j = test_and_insert_new_item(q);

    return Relation::are_connected(i, j);
  }

  /// Join p with q; inserts missing items into the relation
  void join(const T & p, const T & q)
  {
    const size_t i = test_and_insert_new_item(p);
    const size_t j = test_and_insert_new_item(q);
    Relation::join(i, j);
  }
};

# endif // TPL_UNION_H
