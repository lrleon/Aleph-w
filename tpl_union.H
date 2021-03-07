
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
		

/** Binary relation between a set of integers.

    This class implements a static relation of equivalence between a set
    of n integer from 0 until n - 1.

    By static is understood that the number of items canno0t change.

    The used algorithm is the quick weighted fast union which is very
    fast, bounded by \f$O(\lg n)\f$ but in the practice trends to be
    \f$O(1)\f$.

    This implementation is strongly based on java version of Sedgewick
    and Wayne

    @see Relation Relation_T
    @author Leandro Rabindranath Leon 
    @ingroups Grafos
 */
class Fixed_Relation 
{
protected:

  DynArray<size_t> id;
  DynArray<size_t> sz;
  size_t           num_blocks;

  virtual size_t root(size_t i)
  {
    while (i != id(i))
      {
	id(i) = id(id(i));
	i = id(i);
      }
    
    return i;			
  }

  size_t depth(size_t i) noexcept
  {
    const int & parent = id(i);
    if (parent == i)
      return 1;

    return 1 + depth(parent);
  }

public:

  /// Initialize an empty binary relation of integers between 0 and n - 1 
  Fixed_Relation(size_t n = 0) : num_blocks(n)
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

      The advantage of this method is that it allows to construc A
      Fixed_relation without need forknowing the number of
      items. Afterward, when threse are known this number could be set
      with this method.

      @param[in] n the number of items.
      @throw bad_alloc if there is no enough memory
   */
  void set_n(size_t n) { new (this) Fixed_Relation(n); }

  /// Return the number of items of set (not of relation)
  size_t size() const noexcept { return id.size(); }

  /// Return the number ob connected blocks, which is the number of
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
  bool are_connected(size_t i, size_t j) noexcept
  {
    return root(i) == root(j);
  }

  /** Insert the pair '(i,j)' in the relation. All the reachability
      state is updated.

      @param[in] i an item index
      @param[in] j an item index
   */
  void join(size_t i, size_t j)	noexcept
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
    of n integer from 0 until n - 1.

    By static is understood that the number of items can grow.

    The used algorithm is the quick weighted fast union which is very
    fast, bounded by \f$O(\lg n)\f$ but in the practice trends to be
    \f$O(1)\f$.

    This implementation is strongly based on java version of Sedgewick
    and Wayne

    @see Relation Relation_T
    @author Leandro Rabindranath Leon 
    @ingroups Grafos
 */
class Relation : public Fixed_Relation
{
  void verify_if_add_new_points(size_t n)
  {
    size_t l = size();
    if (n < l)
      return;
    
    id.reserve(l, n);
    sz.reserve(l, n);
    for (int i = l; i <= n; ++i)
      {
	id(i) = i;
	sz(i) = 1;
      }
    num_blocks += n - l + 1;
  }

  virtual size_t root(size_t i)
  {
    verify_if_add_new_points(i);
    return Fixed_Relation::root(i);
  }

public:

      /// Intialize an empty relation of n elementos between [0..n)
  Relation(size_t n = 0) : Fixed_Relation(n) {}
};


/** Binary relation between a set of integers.

    This class implements a dynamic relation of equivalence between a set
    of n different items of type T

    The used algorithm is the quick weighted fast union which is very
    fast, bounded by \f$O(\lg n)\f$ but in the practice trends to be
    \f$O(1)\f$.

    This implementation is strongly based on java version of Sedgewick
    and Wayne

    @see Relation Relation_T
    @author Leandro Rabindranath Leon 
    @ingroups Grafos
 */
    template <typename T, class Compare = Aleph::less<T>>
class Relation_T : public Relation
{
  struct Pair
  {
    T      item;
    size_t i;

    Pair() noexcept {}

    Pair(const T & __item, size_t __i) 
    noexcept(std::is_nothrow_copy_assignable<T>::value)
      : item(__item), i(__i) { /* empty */ }
  };

  struct Cmp
  {
    bool operator () (const Pair & p1, const Pair & p2) const noexcept
    {
      return Compare() (p1.item, p2.item);
    }
  };

  DynSetAvlTree<Pair, Cmp> items_tree;

  // retorna el id del item; sea porque lo encuentra o porque lo inserta
  size_t test_and_insert_new_item(const T & item) 
  {
    size_t N = size();
    Pair p(item, N);
    Pair * ptr = items_tree.search_or_insert(p);
    return ptr->i;
  }

public:

      /// Retorna true is i y j están conectados
  bool are_connected(const T & p, const T & q)
  {
    size_t i = test_and_insert_new_item(p);
    size_t j = test_and_insert_new_item(q);

    return Relation::are_connected(i, j);
  }

      /// Une p con q
  void join(const T & p, const T & q)
  {
    size_t i = test_and_insert_new_item(p);
    size_t j = test_and_insert_new_item(q);
    Relation::join(i, j);
  }
};

# endif // TPL_UNION_H
