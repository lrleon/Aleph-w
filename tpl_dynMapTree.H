
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


/** @file tpl_dynMapTree.H
 *  @brief Dynamic key-value map based on balanced binary search trees.
 *
 *  This file provides `DynMapTree`, a generic associative container mapping
 *  keys to values. It supports logarithmic-time operations and can use
 *  various balanced BST implementations (AVL, Treap, Red-Black, etc.).
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 *  @ingroup Maps
 */

# ifndef TPL_DYNMAPTREE_H
# define TPL_DYNMAPTREE_H

# include <type_traits>
# include <utility>

# include <tpl_dynSetTree.H>
# include <ah-concepts.H>

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
   * @note search(), has(), contains() and find() search by the key alone,
   * without building a pair: they neither copy the key nor construct a
   * `Data`. The splay trees are the exception: only their own search,
   * which takes a pair, moves what it finds to the root. operator[],
   * remove() and remove_key() build a `std::pair<Key, Data>` with a
   * `Data()`, as the trees insert and remove whole pairs, so they require
   * `Data` to be default-constructible.
   *
   * @see BinTree Avl_Tree Splay_Tree Rb_Tree Treap Rand_Tree
   * @ingroup Trees
   */
  template <
    typename Key, typename Data,
    template <typename, class> class Tree = Avl_Tree,
    class Compare                         = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
  class DynMapTree :
    public DynSetTree<std::pair<Key, Data>, Tree,
                      Dft_Pair_Cmp<Key, Data, Compare>>
  {
    using Pair = std::pair<Key, Data>;

    using Base =
      DynSetTree<std::pair<Key, Data>, Tree, Dft_Pair_Cmp<Key, Data, Compare>>;

    using Node = typename Base::Node;

    // The splay trees move what a search finds to the root, which only
    // their own search does: a search by the key alone would skip it
    static constexpr bool splays =
      requires (typename Base::Tree_Type & t, const Pair & p) { t.splay(p); };

    // Whether a search by the key cannot throw: it takes the comparison of
    // the tree, which a tree given by the user may copy, compares constant
    // keys and takes the result as a condition and, in a splay tree, builds
    // the pair that the tree searches for, a copy of the key and a Data
    // built by default and moved
    static constexpr bool nothrow_search =
      noexcept(std::declval<typename Base::Tree_Type &>().get_compare()) and
      nothrow_condition_v<const Compare &, const Key &, const Key &> and
      (not splays or (std::is_nothrow_copy_constructible_v<Key> and
                      std::is_nothrow_default_constructible_v<Data> and
                      std::is_nothrow_move_constructible_v<Data>));

    // The pair whose key is equivalent to key, or nullptr. It descends
    // comparing key with the first member of each pair, so it builds no
    // pair: no copy of the key and no Data, which may allocate or throw.
    // Two comparisons per level stop at the pair found, as the search of
    // the AVL tree does, whatever the key. One comparison per level, down
    // to a leaf, as searchInBinTree() does, makes fewer, but Clang takes
    // the child at an address computed from the comparison, without a
    // branch: each level then waits for the comparison of the one above,
    // and a long std::string, whose characters are out of the node, for
    // their memory. The second comparison keeps the branch, which lets the
    // processor go down while it compares. It brings the children of each
    // node to the cache meanwhile. A splay tree searches for a pair built
    // from key, so that it moves what it finds to the root.
    Pair * search_pair(const Key & key) const noexcept(nothrow_search)
    {
      if constexpr (splays)
        {
          static_assert(std::is_default_constructible_v<Data>,
                        "DynMapTree::search() on a splay tree requires Data to be default-constructible");
          return const_cast<Pair *>(Base::search(Pair(key, Data())));
        }
      else
        {
          const Compare & cmp = this->get_compare().cmp;
          for (Node * p = this->get_root_node(); p != Node::NullPtr; )
            {
              __builtin_prefetch(LLINK(p));
              __builtin_prefetch(RLINK(p));
              const Key & pk = KEY(p).first;  // constant, as in the pairs
              if (cmp(key, pk))
                p = LLINK(p);
              else if (cmp(pk, key))
                p = RLINK(p);
              else
                return &KEY(p);
            }
          return nullptr;
        }
    }

  public:

    using Base::Base;

    DynMapTree(const DynList<Key> & keys)
    {
      keys.for_each([this] (auto & k) { this->insert(std::make_pair(k, Data())); });
    }

    DynMapTree() = default;

    Derived_Build(DynMapTree);

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
        @return the data associated with the removed key.
        @throw domain_error if the key is not in the mapping
    */
    Data remove(const Key & key)
    {
      static_assert(std::is_default_constructible_v<Data>,
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

    /** @brief Deletes the pair whose key is `key`.

        @param[in] key the key of the pair to delete.
        @throw std::domain_error if `key` is not in the map.
        @throw what copying `key` or building a `Data` throws.
        @note The trees remove by a whole pair, so it builds one from `key`
        and a `Data()`: removing by the pair stored in the node, found by
        the key alone, would take a second descent of the tree, which
        costs more than that pair. It does not copy the pair that it
        removes.
    */
    void remove_key(const Key & key)
    {
      static_assert(std::is_default_constructible_v<Data>,
                    "DynMapTree::remove_key() requires Data to be default-constructible");
      const size_t n = this->size();
      ah_domain_error_if(Base::remove(Pair(key, Data())) == n)
        << "DynMapTree::remove_key: key is not in the map";
    }

    /** @brief Collect all keys.
     *
     * @return A `DynList` of keys, in the tree traversal order.
     */

    /** @brief Search for a key.
     *
     * @param[in] key Key to search.
     * @return Pointer to the stored pair if found; nullptr otherwise.
     * @throw what comparing two keys throws; nothing when that comparison
     * is `noexcept`, as `Aleph::less` of the arithmetic types. In a splay
     * tree, what copying `key` or building the `Data` of the pair to search
     * for throws instead. The splay that follows is `noexcept`: a
     * comparison that throws in it ends the program, and so does a `Data()`
     * that throws in the header of the splay, a whole node, which it builds
     * by default. So, in a splay tree, neither the comparison nor the
     * default construction of `Data` may throw.
     * @note It compares `key` with the key of each pair, without building a
     * pair to search for: a copy of a long `std::string` key or a `Data()`
     * that is an `Array` would allocate in every search, and what such a
     * copy or construction throws would end the program. The splay trees
     * are the exception: only their own search, which takes a pair, moves
     * the pair found to the root, as they need.
     */
    Pair * search(const Key & key) noexcept(nothrow_search)
    {
      return search_pair(key);
    }

    /// \overload
    /// A pointer to const.
    const Pair * search(const Key & key) const noexcept(nothrow_search)
    {
      return search_pair(key);
    }

    /// \overload
    Pair * search(Key && key) noexcept(nothrow_search) { return search_pair(key); }

    /// \overload
    /// A pointer to const.
    const Pair * search(Key && key) const noexcept(nothrow_search) { return search_pair(key); }

    /** @brief Return whether `key` is in the map.
     *
     * @param[in] key Key to search.
     * @return `true` if a pair has a key equivalent to `key`.
     * @throw what search() throws.
     */
    bool has(const Key & key) const noexcept(nothrow_search) { return search_pair(key) != nullptr; }

    /// \overload
    bool has(Key && key) const noexcept(nothrow_search) { return search_pair(key) != nullptr; }

    /// Same as has().
    bool contains(const Key & key) const noexcept(nothrow_search) { return has(key); }

    /// \overload
    bool contains(Key && key) const noexcept(nothrow_search) { return has(key); }

    /** @brief Find the value associated with `key`.
     *
     * @param[in] key Key to find.
     * @return Reference to the associated value.
     * @throw std::domain_error If `key` is not present.
     * @throw what search() throws.
     * @note It searches by the key, as search(), without building a pair
     * but in a splay tree, where it builds one from `key` and a `Data()`.
     */
    Data & find(const Key & key)
    {
      Pair * p = search_pair(key);
      ah_domain_error_if(p == nullptr) << "key not found";
      return p->second;
    }

    /// \overload
    /// A reference to const.
    const Data & find(const Key & key) const
    {
      const Pair * p = search_pair(key);
      ah_domain_error_if(p == nullptr) << "key not found";
      return p->second;
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

    /// Read-only iterator: the iterator of a constant map.
    using Const_Iterator = typename Base::Const_Iterator;

    DynList<Key> keys() const
    {
      return this->template maps<Key>([] (const auto & p) { return p.first; });
    }

    /** @brief Collect all values.
     *
     * @return A `DynList` of values, in the tree traversal order.
     */
    DynList<Data> values() const
    {
      return this->template maps<Data>([] (const auto & p) { return p.second; });
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
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapBinTree : public DynMapTree<Key, Type, BinTree, Compare>
  {
  public:
    Derived_Build(DynMapBinTree);
  };

  /** @brief Dynamic map implemented with an AVL tree.
   * @see Avl_Tree DynMapTree
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapAvlTree : public DynMapTree<Key, Type, Avl_Tree, Compare>
  {
  public:
    Derived_Build(DynMapAvlTree);
  };
  /** @brief Dynamic map implemented with a red-black tree.
   * @see Rb_Tree DynMapTree
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapRbTree : public DynMapTree<Key, Type, Rb_Tree, Compare>
  {
  public:
    Derived_Build(DynMapRbTree);
  };

  /** @brief Dynamic map implemented with a randomized BST.
   * @see Rand_Tree DynMapTree
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapRandTree : public DynMapTree<Key, Type, Rand_Tree, Compare>
  {
  public:
    Derived_Build(DynMapRandTree);
  };

  /** @brief Dynamic map implemented with a treap.
   * @see Treap DynMapTree
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapTreap : public DynMapTree<Key, Type, Treap, Compare>
  {
  public:
    Derived_Build(DynMapTreap);
  };

  /** @brief Dynamic map implemented with a ranked treap.
   *
   * This specialization provides rank-related operations (e.g. position)
   * from the underlying tree type.
   *
   * @see Treap_Rk DynMapTree
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapTreapRk : public DynMapTree<Key, Type, Treap_Rk, Compare>
  {
  public:
    Derived_Build(DynMapTreapRk);
  };

  /** @brief Dynamic map implemented with a splay tree.
   * @see Splay_Tree DynMapTree
   * @ingroup Trees
   */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapSplayTree : public DynMapTree<Key, Type, Splay_Tree, Compare>
  {
  public:
    Derived_Build(DynMapSplayTree);
  };

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

