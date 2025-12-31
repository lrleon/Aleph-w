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

/** @file tpl_dynMapTree.H
 *  @brief Dynamic key-value map based on balanced binary search trees.
 *
 *  This file provides `DynMapTree`, a generic associative container mapping
 *  keys to values. It supports logarithmic-time operations and can use
 *  various balanced BST implementations (AVL, Treap, Red-Black, etc.).
 *
 *  @ingroup Trees
 *  @ingroup Maps
 */

# ifndef TPL_DYNMAPTREE_H
# define TPL_DYNMAPTREE_H

# include <type_traits>
# include <utility>

# include <tpl_dynSetTree.H>

using namespace Aleph;

namespace Aleph {

  /** @brief Generic key-value map implemented on top of a binary search tree.
   *
   * DynMapTree maps unique keys (`Key`) to values (`Data`). Internally it is a
   * `DynSetTree<std::pair<Key, Data>, ...>` whose ordering compares pairs by
   * their `first` component.
   *
   * Template parameters:
   * - `Key`: key type (map domain).
   * - `Data`: mapped type (map range).
   * - `Tree`: binary search tree type used for instrumentation.
   * - `Compare`: ordering for keys.
   *
   * Element access:
   * - `operator[]` inserts a default-constructed `Data` when the key is
   *   missing.
   * - `find()` throws `std::domain_error` when the key is missing.
   *
   * @note Several operations internally create a temporary `Data()` when
   * building a search key (`std::pair<Key, Data>`). Therefore, those methods
   * require `Data` to be default-constructible.
   *
   * @see BinTree Avl_Tree Splay_Tree Rb_Tree Treap Rand_Tree
   * @ingroup Arboles
   */
  template <
    typename Key, typename Data,
    template <typename, class> class Tree = Avl_Tree,
    class Compare                         = Aleph::less<Key>>
  class DynMapTree :
    public DynSetTree<std::pair<Key, Data>, Tree,
                      Dft_Pair_Cmp<Key, Data, Compare>>
  {
    using Pair = std::pair<Key, Data>;

    using Base =
      DynSetTree<std::pair<Key, Data>, Tree, Dft_Pair_Cmp<Key, Data, Compare>>;

  public:

    using Base::Base;

    DynMapTree(const DynList<Key> & keys)
    {
      keys.for_each([this] (auto & k) { this->insert(std::make_pair(k, Data())); });
    }

    DynMapTree() {}

    using Key_Type = Key;

    using Item_Type = Pair;

    using Value_Type = Data ;

    // using Base::Base;  // no more need. But I don't remember why I put it
    using Base::insert;

    static Data & get_data(Key & key) noexcept
    {
      return key_to_pair<Key, Data>(&key)->second;
    }

    static const Data & get_data(const Key & key) noexcept
    {
      return key_to_pair<Key, Data>(&key)->second;
    }

    static const Key & get_key(Data * data_ptr) noexcept
    {
      return data_to_pair<Key, Data>(data_ptr)->first;
    }

    static const Key & get_key(const Data * data_ptr) noexcept
    {
      return data_to_pair<Key, Data>(data_ptr)->first;
    }

    /** @brief Insert a key-value pair.
     *
     * If `key` is already present, no insertion is performed and `nullptr`
     * is returned.
     *
     * @param[in] key Key to insert.
     * @param[in] data Value to associate with `key`.
     * @return Pointer to the inserted pair, or nullptr if the key exists.
     * @throw std::bad_alloc If there is not enough memory.
     */
    Pair * insert(const Key & key, const Data & data)
    {
      return this->Base::insert(Pair(key, data));
    }

    Pair * insert(const Key & key, Data && data = Data())
    {
      return this->Base::insert(Pair(key, std::forward<Data>(data)));
    }

    Pair * insert(Key && key, const Data & data)
    {
      return this->Base::insert(Pair(std::forward<Key>(key), data));
    }

    Pair * insert(Key && key, Data && data = Data())
    {
      return this->Base::insert(Pair(std::forward<Key>(key),
                                     std::forward<Data>(data)));
    }

    Pair * append(const Key & key, const Data & data)
    {
      return this->Base::insert(Pair(key, data));
    }

    Pair * append(const Key & key, Data && data = Data())
    {
      return this->Base::insert(Pair(key, std::forward<Data>(data)));
    }

    Pair * append(Key && key, const Data & data)
    {
      return this->Base::insert(Pair(std::forward<Key>(key), data));
    }

    Pair * append(Key && key, Data && data = Data())
    {
      return this->Base::insert(Pair(std::forward<Key>(key),
                                     std::forward<Data>(data)));
    }

    /** @brief Alias for insert().
     *
     * @return Pointer to the inserted pair, or nullptr if the key exists.
     */
    Pair * put(const Key & key, const Data & data)
    {
      return insert(key, data);
    }

    Pair * put(const Key & key, Data && data)
    {
      return insert(key, std::forward<Data>(data));
    }

    Pair * put(Key && key, const Data & data)
    {
      return insert(std::forward<Key>(key), data);
    }

    Pair * put(Key && key, Data && data)
    {
      return insert(std::forward<Key>(key), std::forward<Data>(data));
    }

    /** Deletes the pair `key,data`

        `remove(key)` deletes from the mapping the pair associated to `key`

        @param[in] key to delete
        @return the data associated to the removed key.
        @throw domain_error if key is not in the mapping
    */
    Data remove(const Key & key)
    {
      static_assert(std::is_default_constructible<Data>::value,
                    "DynMapTree::remove() requires Data to be default-constructible");
      Pair p(key, Data());
      return this->del(p).second;
    }

    Data remove(Key && key)
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::remove() requires Data to be default-constructible");
      Pair p(std::forward<Key>(key), Data());
      return this->del(p).second;
    }

    void remove_key(const Key & key)
    {
      static_assert(std::is_default_constructible<Data>::value,
                    "DynMapTree::remove_key() requires Data to be default-constructible");
      (void) this->del(Pair(key, Data()));
    }

    /** @brief Collect all keys.
     *
     * @return A `DynList` of keys, in the tree traversal order.
     */

    /** @brief Search for a key.
     *
     * @param[in] key Key to search.
     * @return Pointer to the stored pair if found; nullptr otherwise.
     */
    Pair * search(const Key & key) const noexcept
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::search() requires Data to be default-constructible");
      return this->Base::search(Pair(key, Data()));
    }

    Pair * search(Key && key) const noexcept
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::search() requires Data to be default-constructible");
      return this->Base::search(Pair(std::move(key), Data()));
    }

    bool has(const Key & key) const noexcept { return search(key) != nullptr; }

    bool has(Key && key) const noexcept { return search(std::move(key)) != nullptr; }

    bool contains(const Key & key) const noexcept { return has(key); }

    bool contains(Key && key) const noexcept { return has(std::move(key)); }

    /** @brief Find the value associated with `key`.
     *
     * @param[in] key Key to find.
     * @return Reference to the associated value.
     * @throw std::domain_error If `key` is not present.
     */
    Data & find(const Key & key)
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::find() requires Data to be default-constructible");
      return this->Base::find(Pair(key, Data())).second;
    }

    const Data & find(const Key & key) const
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::find() requires Data to be default-constructible");
      return this->Base::find(Pair(key, Data())).second;
    }

    Data & operator [] (const Key & key)
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::operator[] requires Data to be default-constructible");
      return this->search_or_insert(Pair(key, Data()))->second;
    }

    const Data & operator [] (const Key & key) const
    {
      return this->find(key);
    }

    Data & operator [] (Key && key)
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::operator[] requires Data to be default-constructible");
      return this->search_or_insert(Pair(std::move(key), Data()))->second;
    }

    const Data & operator [] (Key && key) const
    {
      return this->find(std::move(key));
    }

    using Iterator = typename Base::Iterator;

    DynList<Key> keys() const
    {
      return this->template maps<Key>([] (auto p) { return p.first; });
    }

    /** @brief Collect all values.
     *
     * @return A `DynList` of values, in the tree traversal order.
     */
    DynList<Data> values() const
    {
      return this->template maps<Data>([] (auto p) { return p.second; });
    }

    /** @brief Collect pointers to all values.
     *
     * @note Pointers are valid until the tree is structurally modified.
     */
    DynList<Data*> values_ptr()
    {
      DynList<Data*> ret;
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(&it.get_curr().second);
      return ret;
    }

    /** @brief Collect pointers to all stored pairs.
     *
     * @note Pointers are valid until the tree is structurally modified.
     */
    DynList<Pair*> items_ptr()
    {
      DynList<Pair*> ret;
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(&it.get_curr());
      return ret;
    }
  };
} // end namespace Aleph


# include <tpl_binTree.H>
# include <tpl_avl.H>
# include <tpl_rb_tree.H>
# include <tpl_rand_tree.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_splay_tree.H>

namespace Aleph {
  /** @brief Dynamic map implemented with a classic binary search tree.
   * @see BinTree DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapBinTree : public DynMapTree<Key, Type, BinTree, Compare> {};

  /** @brief Dynamic map implemented with an AVL tree.
   * @see Avl_Tree DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapAvlTree : public DynMapTree<Key, Type, Avl_Tree, Compare> {};
  /** @brief Dynamic map implemented with a red-black tree.
   * @see Rb_Tree DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapRbTree : public DynMapTree<Key, Type, Rb_Tree, Compare>
  { /* empty */ };

  /** @brief Dynamic map implemented with a randomized BST.
   * @see Rand_Tree DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapRandTree : public DynMapTree<Key, Type, Rand_Tree, Compare>
  { /* empty */ };

  /** @brief Dynamic map implemented with a treap.
   * @see Treap DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapTreap : public DynMapTree<Key, Type, Treap, Compare>
  { /* empty */ };

  /** @brief Dynamic map implemented with a ranked treap.
   *
   * This specialization provides rank-related operations (e.g. position)
   * from the underlying tree type.
   *
   * @see Treap_Rk DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapTreapRk : public DynMapTree<Key, Type, Treap_Rk, Compare>
  { /* empty */ };

  /** @brief Dynamic map implemented with a splay tree.
   * @see Splay_Tree DynMapTree
   * @ingroup Arboles
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapSplayTree : public DynMapTree<Key, Type, Splay_Tree, Compare>
  { /* empty */ };

  template <typename T, class Op, class C>
  DynMapTree<typename C::Item_Type, T> map_unify(const C & c, Op op)
  {
    DynMapTree<typename C::Item_Type, T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & curr = it.get_curr();
        ret.insert(curr, op(curr));
      }
    return ret;
  }

} // end namespace Aleph

# endif /* TPL_DYNMAPTREE_H */

