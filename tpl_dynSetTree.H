
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file tpl_dynSetTree.H
 *  @brief Dynamic set implementations based on balanced binary search trees.
 *
 *  This file provides `DynSetTree`, a generic dynamic set backed by various
 *  balanced BST implementations (AVL, Treap, Red-Black, Splay, etc.). It offers
 *  logarithmic-time insertion, deletion, and search operations with support
 *  for ranked operations and functional programming patterns.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 *  @ingroup Sets
 */

# ifndef TPL_DYNSETTREE_H
# define TPL_DYNSETTREE_H

# include <typeinfo>
# include <type_traits>
# include <utility>
# include <stdexcept>
# include <ah-errors.H>
# include <ah-args-ctor.H>
# include <ahIterator.H>
# include <ah-zip.H>
# include <ah-arena.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTree.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_avl.H>
# include <tpl_avlRk.H>
# include <tpl_rand_tree.H>
# include <tpl_rb_tree.H>
# include <tpl_rbRk.H>
# include <tpl_tdRbTree.H>
# include <tpl_tdRbTreeRk.H>
# include <tpl_hRbTree.H>
# include <tpl_hRbTreeRk.H>
# include <tpl_splay_tree.H>
# include <tpl_splay_treeRk.H>

using namespace Aleph;

namespace Aleph
{
  template <typename Node>
  class AbstractTreeNodeAllocator
  {
  public:
    AbstractTreeNodeAllocator() = default;

    virtual Node * alloc_lval(const typename Node::key_type & key) = 0;

    virtual Node * alloc_rval(typename Node::key_type && key) = 0;

    virtual void unalloc(Node *) = 0;

    virtual ~AbstractTreeNodeAllocator() = default;
  };

  template <typename Node>
  class DftTreeNodeAllocator : public AbstractTreeNodeAllocator<Node>
  {
  public:
    Node * alloc_lval(const typename Node::key_type & key) override
    {
      return new Node(key);
    }

    Node * alloc_rval(typename Node::key_type && key) override
    {
      return new Node(std::forward<typename Node::key_type>(key));
    }

    void unalloc(Node *p) override
    {
      delete p;
    }

    ~DftTreeNodeAllocator() override = default;
  };

  template <typename Node>
  struct ArenaTreeAllocator : public AbstractTreeNodeAllocator<Node>
  {
    AhArenaAllocator arena;

    ArenaTreeAllocator(const size_t & sz = 1024 * 1024)
      : arena(sz)
    {
      // empty
    }

    ArenaTreeAllocator(const char *base_addr, const size_t & sz)
      : arena(base_addr, sz)
    {
      // empty
    }

    Node * alloc_lval(const typename Node::key_type & key) override
    {
      Node *ptr = allocate<Node>(arena, key);
      ah_bad_alloc_if(ptr == nullptr);
      return ptr;
    }

    Node * alloc_rval(typename Node::key_type && key) override
    {
      Node *ptr =
          allocate<Node>(arena, std::forward<typename Node::key_type>(key));
      ah_bad_alloc_if(ptr == nullptr);
      return ptr;
    }

    void unalloc(Node *p) override
    {
      dealloc<Node>(arena, p);
    }

    ~ArenaTreeAllocator() = default;
  };

  template <typename Container, typename T>
  struct CmpContainer
  {
    bool operator ()(const Container & c1, const Container & c2) const
    {
      return std::lexicographical_compare(c1.begin(), c1.end(), c2.begin(), c2.end(),
                                     [](const T & item1, const T & item2)
                                       {
                                         return item1 < item2;
                                       });
    }
  };

  /** @brief Dynamic set backed by balanced binary search trees with automatic memory management.

      DynSetTree<Key, Tree, Compare> is a high-level dynamic set container that uses
      any of several balanced BST implementations (AVL, Red-Black, Treap, Splay, etc.)
      to maintain a sorted collection of unique keys.

      Unlike the low-level tree classes (e.g., Avl_Tree, Rb_Tree), DynSetTree
      automatically manages node memory, provides rich functional operations
      (map, filter, fold), and offers an STL-compatible interface.

      @tparam Key The type of keys stored in the set.
      @tparam Tree The underlying tree implementation template (default: Avl_Tree).
                   Can be Avl_Tree, Rb_Tree, Treap, TreapRk, Splay_Tree, Rand_Tree.
      @tparam Compare Comparison functor for ordering keys (default: Aleph::less<Key>).

      @par Complexity (depends on Tree type):
      - Search: O(log n) average/worst (AVL, RB); O(log n) amortized (Splay); O(log n) expected (Treap, Rand)
      - Insert: O(log n) with same characteristics as search
      - Delete: O(log n) with same characteristics as search
      - size(): O(1) - maintained by counter
      - Ranked operations (if Tree supports them, e.g., TreapRk, Avl_Tree_Rk): O(log n)

      @par Supported Tree Types:
      - Avl_Tree: Strictest balance, O(log n) worst-case, more rotations
      - Rb_Tree: Relaxed balance, O(log n) worst-case, fewer rotations
      - Treap: Randomized, O(log n) expected, simple implementation
      - TreapRk: Treap with rank (indexed access), O(log n) expected
      - Splay_Tree: Self-adjusting, O(log n) amortized, cache-friendly
      - Rand_Tree: Randomized tree, O(log n) expected
      - BinTree: Unbalanced (for testing/comparison, not recommended)

      @par Key Features:
      - Automatic node memory management (RAII)
      - STL-compatible iterators and interface
      - Rich functional operations (map, filter, fold, zip, etc.)
      - Support for ranked operations (if underlying tree supports them)
      - Duplicate handling variants (insert vs insert_dup)
      - Split/join operations
      - Set operations (union, intersection, difference)

      @par Example:
      @code
      // AVL-backed set (default)
      DynSetTree<int> avl_set = {5, 2, 8, 1, 9};

      // Red-Black backed set
      DynSetTree<int, Rb_Tree> rb_set;
      rb_set.insert(42);
      rb_set.insert(17);

      // Treap with rank (supports indexed access)
      DynSetTree<std::string, TreapRk> ranked_set;
      ranked_set.insert("apple");
      ranked_set.insert("banana");
      ranked_set.insert("cherry");
      auto second = ranked_set.select(1); // Get element at index 1

      // Functional operations
      auto doubled = avl_set.maps<int>([](int x) { return x * 2; });
      int sum = avl_set.foldl(0, [](int acc, int x) { return acc + x; });

      // STL compatibility
      for (const auto& key : avl_set)
        std::cout << key << " ";
      @endcode

      @par Ranked Operations:
      If the underlying tree supports rank (TreapRk, Avl_Tree_Rk, etc.),
      additional operations become available:
      - select(i): Get the i-th smallest element, O(log n)
      - position(key): Get the rank/position of a key, O(log n)
      - remove_pos(i): Remove element at position i, O(log n)

      @note Keys must be unique. For duplicate support, use insert_dup().
      @note The tree is automatically balanced according to the Tree policy.
      @note Memory is automatically managed - nodes are freed on destruction.

      @see Avl_Tree Low-level AVL tree (manual memory management).
      @see Rb_Tree Low-level Red-Black tree (manual memory management).
      @see TreapRk Low-level Treap with rank (manual memory management).
      @see FunctionalMethods Inherited functional operations.
      @ingroup Trees
  */
  template <typename Key,
            template <typename, class> class Tree = Avl_Tree,
            class Compare = Aleph::less<Key>>
  class DynSetTree
      : public LocateFunctions<DynSetTree<Key, Tree, Compare>, Key>,
        public FunctionalMethods<DynSetTree<Key, Tree, Compare>, Key>,
        public GenericKeys<DynSetTree<Key, Tree, Compare>, Key>,
        public EqualToMethod<DynSetTree<Key, Tree, Compare>>,
        public StlAlephIterator<DynSetTree<Key, Tree, Compare>>
  {
  public:
    /// Type of binary node used by the binary search tree
    /// internal.
    using Node = typename Tree<Key, Compare>::Node;

    using Tree_Type = Tree<Key, Compare>;

  private:
    template <typename T>
    struct Has_Range_Methods
    {
      // Test for const select (RB, AVL, etc.)
      template <typename U>
      static auto test_select_const(int)
        -> decltype(std::declval<const U &>().select(std::declval<size_t>()),
                    std::true_type());

      template <typename>
      static std::false_type test_select_const(...);

      // Test for non-const select (Splay tree - mutates on access)
      template <typename U>
      static auto test_select_nonconst(int)
        -> decltype(std::declval<U &>().select(std::declval<size_t>()),
                    std::true_type());

      template <typename>
      static std::false_type test_select_nonconst(...);

      template <typename U>
      static auto test_remove_pos(int)
        -> decltype(std::declval<U &>().remove_pos(std::declval<size_t>()),
                    std::true_type());

      template <typename>
      static std::false_type test_remove_pos(...);

      template <typename U>
      static auto test_split_pos(int)
        -> decltype(std::declval<U &>().split_pos(std::declval<size_t>(),
                                                  std::declval<U &>(),
                                                  std::declval<U &>()),
                    std::true_type());

      template <typename>
      static std::false_type test_split_pos(...);

      // Test for const position
      template <typename U>
      static auto test_position_const(int)
        -> decltype(std::declval<const U &>().position(std::declval<const Key &>()),
                    std::true_type());

      template <typename>
      static std::false_type test_position_const(...);

      // Test for non-const position (Splay tree)
      template <typename U>
      static auto test_position_nonconst(int)
        -> decltype(std::declval<U &>().position(std::declval<const Key &>()),
                    std::true_type());

      template <typename>
      static std::false_type test_position_nonconst(...);

      // Test for const find_position
      template <typename U>
      static auto test_find_position_const(int)
        -> decltype(std::declval<const U &>().find_position(std::declval<const Key &>()),
                    std::true_type());

      template <typename>
      static std::false_type test_find_position_const(...);

      // Test for non-const find_position (Splay tree)
      template <typename U>
      static auto test_find_position_nonconst(int)
        -> decltype(std::declval<U &>().find_position(std::declval<const Key &>()),
                    std::true_type());

      template <typename>
      static std::false_type test_find_position_nonconst(...);

      static constexpr bool has_select =
          std::is_same<decltype(test_select_const<T>(0)), std::true_type>::value ||
          std::is_same<decltype(test_select_nonconst<T>(0)), std::true_type>::value;
      static constexpr bool has_remove_pos =
          std::is_same<decltype(test_remove_pos<T>(0)), std::true_type>::value;
      static constexpr bool has_split_pos =
          std::is_same<decltype(test_split_pos<T>(0)), std::true_type>::value;
      static constexpr bool has_position =
          std::is_same<decltype(test_position_const<T>(0)), std::true_type>::value ||
          std::is_same<decltype(test_position_nonconst<T>(0)), std::true_type>::value;
      static constexpr bool has_find_position =
          std::is_same<decltype(test_find_position_const<T>(0)), std::true_type>::value ||
          std::is_same<decltype(test_find_position_nonconst<T>(0)), std::true_type>::value;
    };

    // call_select for const trees (RB, AVL, etc.)
    template <typename T>
    static auto call_select(const T & t, const size_t i, int)
      -> decltype(t.select(i))
    {
      return t.select(i);
    }

    // call_select for non-const trees (Splay - mutates on access)
    template <typename T>
    static auto call_select_nc(T & t, const size_t i, int)
      -> decltype(t.select(i))
    {
      return t.select(i);
    }

    template <typename T>
    static Node * call_select(const T &, const size_t, ...)
    {
      ah_domain_error() << "select is not supported by underlying tree";
      return nullptr;
    }

    template <typename T>
    static Node * call_select_nc(T &, const size_t, ...)
    {
      ah_domain_error() << "select is not supported by underlying tree";
      return nullptr;
    }

    template <typename T>
    static auto call_remove_pos(T & t, const size_t i, int)
      -> decltype(t.remove_pos(i))
    {
      return t.remove_pos(i);
    }

    template <typename T>
    static Node * call_remove_pos(T &, const size_t, ...)
    {
      ah_domain_error() << "remove_pos is not supported by underlying tree";
      return nullptr;
    }

    template <typename T>
    static auto call_split_pos(T & t, const size_t pos, T & l, T & r, int)
      -> decltype(t.split_pos(pos, l, r), void())
    {
      t.split_pos(pos, l, r);
    }

    template <typename T>
    static void call_split_pos(T &, const size_t, T &, T &, ...)
    {
      ah_domain_error() << "split_pos is not supported by underlying tree";
    }

    template <typename T>
    static auto call_position(const T & t, const Key & key, int)
      -> decltype(t.position(key))
    {
      return t.position(key);
    }

    template <typename T>
    static std::pair<long, Node *> call_position(const T &, const Key &, ...)
    {
      ah_domain_error() << "position is not supported by underlying tree";
      return std::pair<long, Node *>(0, nullptr);
    }

    template <typename T>
    static auto call_find_position(const T & t, const Key & key, int)
      -> decltype(t.find_position(key))
    {
      return t.find_position(key);
    }

    template <typename T>
    static std::pair<long, Node *> call_find_position(const T &, const Key &, ...)
    {
      ah_domain_error() << "find_position is not supported by underlying tree";
      return std::pair<long, Node *>(0, nullptr);
    }

    static constexpr size_t dim = 13;

    mutable Tree<Key, Compare> tree;
    size_t num_nodes;

  public:
    typedef DynSetTree Set_Type;

    typedef Key Item_Type;

    typedef Key Key_Type;

    /** Exchange all elements of this set with dset
        in constant time (and extremely fast).

        @param[in] dset the set to exchange with this
    */
    void swap(DynSetTree & dset) noexcept(noexcept(tree.swap(dset.tree)) &&
                                          noexcept(std::swap(num_nodes, dset.num_nodes)))
    {
      tree.swap(dset.tree);
      std::swap(num_nodes, dset.num_nodes);
    }

    /// Instantiate a dynamic set
    DynSetTree(const Compare & cmp = Compare())
      : tree(cmp), num_nodes(0)
    {
      // empty
    }

    DynSetTree(const char *base_addr, const size_t & sz,
               const Compare & cmp = Compare())
      : tree(cmp), num_nodes(0)
    {
      // empty
    }

    /// instantiates a dynamic copy of srcTree
    DynSetTree(const DynSetTree & srcTree)
      : tree(srcTree.tree.get_compare()), num_nodes(srcTree.num_nodes)
    {
      Node *srcRoot = srcTree.tree.getRoot();
      try
        {
          tree.getRoot() = copyRec(srcRoot);
        }
      catch (...)
        {
          num_nodes = 0;
          throw;
        }
    }

    Special_Ctors(DynSetTree, Key);

    DynSetTree(DynSetTree && srcTree)
      noexcept : num_nodes(0)
    {
      swap(srcTree);
    }

    /// remove all elements from the set
    void empty()
    {
      destroyRec(tree.getRoot());
      num_nodes = 0;
    }

    DynSetTree &operator =(const DynList<Key> & list)
    {
      return *this = DynSetTree(list);
    }

    /// assigns this to the dynamic array srctree
    DynSetTree &operator =(const DynSetTree & srcTree)
    {
      if (this == &srcTree)
        return *this;

      Node *src_root = srcTree.tree.getRoot();

      empty();

      tree.getRoot() = copyRec(src_root);
      num_nodes = srcTree.num_nodes;

      return *this;
    }

    /// Assigns the dynamic set srcTree to this.
    DynSetTree &operator =(DynSetTree && srcTree)
    {
      if (this == &srcTree)
        return *this;

      empty();
      swap(srcTree);
      return *this;
    }

    /// Destroyer; all elements are released.
    virtual ~DynSetTree()
    {
      empty();
    }

  private:
    Key * __insert(Node *p)
    {
      Node *q = nullptr;
      try
        {
          q = tree.search_or_insert(p);
        }
      catch (...)
        {
          delete p;
          throw;
        }

      if (q != p)
        return nullptr;

      ++num_nodes;

      return &p->get_key();
    }

  public:
    /** Inserts a key into the dynamic set.

        Inserts the key key into the dynamic set.

        @param[in] key key to insert.
        @return pointer to the inserted key if it is not inside
        of the tree; nullptr otherwise.
    */
    Key * insert(const Key & key)
    {
      Node *p = new Node(key);
      Key *key_p = __insert(p);
      if (key_p == nullptr) // was there insertion?
        { // No (KEY(p) is already in the tree) ==> free p and return nullptr
          delete p;
          return nullptr;
        }

      return key_p;
    }

    Key * insert(Key && key)
    {
      Node *p = new Node(std::forward<Key>(key));
      Key *key_p = __insert(p);
      if (key_p == nullptr) // was there insertion?
        {
          delete p;
          return nullptr;
        }

      return key_p;
    }

    Key * append(const Key & key)
    {
      return insert(key);
    }

    Key * append(Key && key)
    {
      return insert(std::forward<Key>(key));
    }

  private:
    Key * __search_or_insert(Node *p)
    {
      Node *q = nullptr;
      try
        {
          q = tree.search_or_insert(p);
        }
      catch (...)
        {
          delete p;
          throw;
        }
      if (q != p) // was there an insertion?
        delete p; // No (KEY(p) is already in the tree) ==> free
        // allocated node
      else
        ++num_nodes;

      return &q->get_key();
    }

    std::pair<Node *, bool> __contains_or_insert(Node *p)
    {
      Node *q = nullptr;
      try
        {
          q = tree.search_or_insert(p);
        }
      catch (...)
        {
          delete p;
          throw;
        }
      if (q != p)
        { // KEY(p) is already inside the tree
          delete p;
          return std::pair<Node *, bool>(q, true);
        }
      ++num_nodes;
      return std::pair<Node *, bool>(p, false);
    }

  public:
    /** Look for the key <code>key</code> in the binary search tree or
        inserts it if it is not found.

        <code>search_or_insert(key)</code> searches the binary tree for a
        node whose key is <code>KEY(p)</code>. If the key is
        finds, then a pointer to it is returned; of what
        Otherwise <code>key</code> is inserted into the binary tree of
        search this.

        @param[in] key the key to search or insert.
        @return pointer to the key inside the tree
    */
    Key * search_or_insert(const Key & key)
    {
      return __search_or_insert(new Node(key));
    }

    Key * search_or_insert(Key && key)
    {
      return
          __search_or_insert(new Node(std::forward<Key>(key)));
    }

    /* Look for the key <code>key</code> in the binary search tree and
       eventually inserts it if it is not found.

       <code>contains_or_insert(key)</code> searches the tree for the key
       <code>key</code>. If the key is already found, then it is returned
       true. Otherwise, the key is inserted and false is returned.

       @param[in] key key to find or insert
       @return a std::pair whose first field is a pointer to the found or
       inserted key, and the second field is a boolean whose value is
       `false` if the key was inserted; `true` otherwise, that is if the
       key is already present in the tree.
    */
    std::pair<Key *, bool> contains_or_insert(const Key & key)
    {
      auto p = __contains_or_insert(new Node(key));
      return std::pair<Key *, bool>(&p.first->get_key(), p.second);
    }

    std::pair<Key *, bool> contains_or_insert(Key && key)
    {
      auto p = __contains_or_insert(new Node(std::forward<Key>(key)));
      return std::pair<Key *, bool>(&p.first->get_key(), p.second);
    }

  private:
    Key * __insert_dup(Node *q)
    {
      try
        {
          Node *p = tree.insert_dup(q);
          ++num_nodes;
          return &p->get_key();
        }
      catch (...)
        {
          delete q;
          throw;
        }
    }

  public:
    Key * insert_dup(const Key & key)
    {
      return __insert_dup(new Node(key));
    }

    Key * insert_dup(Key && key)
    {
      return __insert_dup(new Node(std::forward<Key>(key)));
    }

    Key * put(const Key & key)
    {
      return insert(key);
    }

    Key * put(Key && key)
    {
      return insert(std::forward<Key>(key));
    }

    /** Removes a key from the dynamic set.

        remove(key) finds the key key in the set and removes it.

        @param[in] key key to delete
        @return number of elements that the set has.
    */
    size_t remove(const Key & key)
    {
      Node *p = static_cast<Node *>(tree.remove(key));

      if (p == nullptr)
        return num_nodes;

      delete p;

      return --num_nodes;
    }

    /** Deletes `key` and returns a full copy of stored key

        This method is intended to be used with compound keys, by example
        pairs, whose searching is done by a particular member of compound
        data.

        @param[in] key key to be removed
        @return a full copy of stored key
        @throw domain_error if the `key` is not found in the set
    */
    Key del(const Key & key)
    {
      Node *p = static_cast<Node *>(tree.remove(key));

      ah_domain_error_if(p == nullptr)
        << "DynSetTree::del key is not found in the tree";

      auto ret_val = p->get_key();

      delete p;

      --num_nodes;

      return ret_val;
    }

    /** Removes a key from the dynamic set.

        @return number of elements that the set has.
    */
    Key remove_pos(const size_t i)
    {
      ah_domain_error_if_constexpr(not Has_Range_Methods<Tree_Type>::has_remove_pos)
        << "remove_pos is not supported by underlying tree";

      ah_out_of_range_error_if(i >= num_nodes)
        << "remove_pos index out of range";

      Node *p = static_cast<Node *>(call_remove_pos(tree, i, 0));
      ah_logic_error_if(p == nullptr)
        << "remove_pos returned nullptr";
      const Key ret_val = KEY(p);

      delete p;

      --num_nodes;

      return ret_val;
    }

    /// Returns true if key belongs to the dynamic set.
    bool exist(const Key & key) const
    {
      return search(key) != nullptr;
    }

    bool has(const Key & key) const
    {
      return exist(key);
    }

    bool contains(const Key & key) const
    {
      return exist(key);
    }

    /** Returns a modifiable reference to an element within the
        set.

        find(key) looks for the key key in the set and returns a
        modifiable reference to the value contained in the set.

        @param[in] key key to search for.
        @return modifiable reference to the key contained within
        of the set.
        @throw domain_error if key is not within the set.

        @note Be very careful with altering the search order,
        since the reference is modifiable.
    */
    Key &find(const Key & key) const
    {
      Node *node = static_cast<Node *>(tree.search(key));

      ah_domain_error_if(node == nullptr)
        << "key not found";

      return node->get_key();
    }

    /** Returns the infix (ordinate) position of the key key or whatever
        It would be your position of belonging to the tree.

        find_position(key) searches the extended tree for the key key
        (which takes time \f$O(\lg n)\f$) and returns the position
        infix of the node containing the key.

        @param[in] key key to search for and determine infix position.
        @return infix position of the key within the set
        ordered if it is found or, otherwise, the position
        where it would be if it belonged to the tree.

        @warning It only works if the tree handles ranges.
    */
    std::pair<long, Key *> find_position(const Key & key) const
    {
      ah_domain_error_if_constexpr(not Has_Range_Methods<Tree_Type>::has_find_position)
        << "find_position is not supported by underlying tree";

      if (num_nodes == 0)
        return std::pair<long, Key *>(0, nullptr);

      auto p = call_find_position(tree, key, 0);
      if (p.second == nullptr)
        return std::pair<long, Key *>(static_cast<long>(p.first), nullptr);

      return std::pair<long, Key *>(static_cast<long>(p.first),
                                    &p.second->get_key());
    }

    /** Find an element in the set.

        search(key) searches for the key key in the set. If he
        element is found in the set, then a
        pointer to the value contained in the set; otherwise
        nullptr is returned.

        @param[in] key key to search for.
        @return pointer to the element in the set if it is
        found in it; nullptr of the opposite.

        @note Be very careful with altering the search order,
        because through the pointer the key is modifiable.
    */
    Key * search(const Key & key) const
    {
      Node *node = static_cast<Node *>(tree.search(key));

      if (node == nullptr)
        return nullptr;

      return &(node->get_key());
    }

    /// Returns the smallest key contained in the set according to the criterion
    /// comparison given.
    const Key &min() const
    {
      ah_domain_error_if(num_nodes == 0)
        << "set is empty";

      return find_min(tree.getRoot())->get_key();
    }

    /// \overload min
    const Key &get_first() const { return min(); }

    /// Returns the largest key contained in the set according to the criteria
    /// comparison given.
    const Key &max() const
    {
      ah_domain_error_if(num_nodes == 0)
        << "set is empty";

      return find_max(tree.getRoot())->get_key();
    }

    /// \overload max
    const Key &get_last() const { return max(); }

    /// Synonym of max
    const Key &get() const { return max(); }

    /// Returns the cardinality of the set.
    const size_t &size() const { return num_nodes; }

    /// returns true if the set is empty
    bool is_empty() const { return num_nodes == 0; }

    /// Calculates and returns the length of the internal path of the tree
    /// search binary.
    size_t internal_path_length() const
    {
      return Aleph::internal_path_length(tree.getRoot());
    }

    Node * get_root_node() const { return tree.getRoot(); }

    const Key &get_root() const
    {
      ah_domain_error_if(num_nodes == 0)
        << "Tree is empty";
      return KEY(tree.getRoot());
    }

    /// Returns any element of the set
    const Key &get_item() const { return get_root(); }

    /// Calculates and returns the height of the binary search tree.
    size_t height() const { return computeHeightRec(tree.getRoot()); }

    /// Performs a prefix traversal over all nodes in the tree and
    /// invokes the <code>visitFct</code> operation on each visit
    template <class Op>
    void for_each_in_preorder(void (*visitFct)(Node *, int, int))
    {
      Node *root = static_cast<Node *>(tree.getRoot());
      preOrderRec(root, visitFct);
    }

    /** Returns the infix (ordered) position of the key.

        position(key) searches the tree for the key (which
        takes time \f$O(\lg n)\f$) and returns the infix position
        of the node containing the key.

        @param[in] key key to search for and determine infix position.
        @return infix position of the key within the set
        sorted if key found; -1 otherwise

        @warning It only works if the tree handles ranges.
    */
    long position(const Key & key) const
    {
      ah_domain_error_if_constexpr(not Has_Range_Methods<Tree_Type>::has_position)
        << "position is not supported by underlying tree";

      auto p = call_position(tree, key, 0);
      return static_cast<long>(p.first);
    }

    /** Returns the ith node in infix position

        @param[in] i desired position
        @return reference to the ith key inserted into the set
    */
    Key &select(size_t i)
    {
      ah_domain_error_if_constexpr(not Has_Range_Methods<Tree_Type>::has_select)
        << "select is not supported by underlying tree";

      ah_out_of_range_error_if(i >= num_nodes)
        << "select index out of range";

      // Try non-const first (for Splay trees), then const
      Node *p = call_select_nc(tree, i, 0);
      ah_logic_error_if(p == nullptr)
        << "select returned nullptr";
      return p->get_key();
    }

    const Key &select(size_t i) const
    {
      ah_domain_error_if_constexpr(not Has_Range_Methods<Tree_Type>::has_select)
        << "select is not supported by underlying tree";

      ah_out_of_range_error_if(i >= num_nodes)
        << "select index out of range";

      // For const version, only const select works (not for Splay trees)
      Node *p = call_select(tree, i, 0);
      ah_logic_error_if(p == nullptr)
        << "select returned nullptr";
      return p->get_key();
    }

    Key &operator ()(size_t i)
    {
      return select(i);
    }

    const Key &operator [](const Key & key) const
    {
      return find(key);
    }

    const Key &operator [](const Key & key)
    {
      return *search_or_insert(key);
    }

    Key &access(size_t i)
    {
      return select(i);
    }

    bool verify() const
    {
      return tree.verify() and check_bst(tree.getRoot());
    }

  private:
    template <class Key_Op>
    struct Node_Op
    {
      Key_Op & key_op;

      Node_Op(Key_Op & __key_op) : key_op(__key_op)
      { /* empty */
      }

      Node_Op(Key_Op && __key_op) : key_op(__key_op)
      {
        /* empty */
      }

      void operator ()(Node *root)
      {
        assert(root != nullptr);
        key_op(KEY(root));
      }
    };

  public:
    /** Performs a prefix traversal over all keys in the set and
        invokes operation Op.

        <code>Op(p)</code> has the following structure:
        \code
        struct Key_Op
        {
        // state attributes to maintain

        Key_Op(...) // optional constructor if initialization is needed
        {
        // initialization
        }

        void operator () (const Key & key)
        {
        // operation on key
        }
        };
        \endcode

        @param[in] key_op operation to execute on each key
        \see For_Each_Preorder
    */
    template <class Key_Op>
    void for_each_preorder(Key_Op & key_op) const
    {
      Node *root = static_cast<Node *>(tree.getRoot());

      Node_Op<Key_Op> node_op(const_cast<Key_Op &>(key_op));

      For_Each_Preorder<Node>()(root, node_op);
    }

    /** \overload
     *
     *  Performs a preorder traversal over all keys using a temporary operation object.
     *
     *  @tparam Key_Op Operation type (default-constructible).
     *  @param[in] key_op Operation to execute on each key (default-constructed if not provided).
     */
    template <class Key_Op>
    void for_each_preorder(Key_Op && key_op = Key_Op()) const
    {
      for_each_preorder<Key_Op>(key_op);
    }

    /** Performs an infix traversal over all keys in the set and
        invokes operation Op.

        <code>Op(p)</code> has the following structure:
        \code
        struct Key_Op
        {
        // state attributes to maintain

        Key_Op(...) // optional constructor if initialization is needed
        {
        // initialization
        }

        void operator () (const Key & key)
        {
        // operation on key
        }
        };
        \endcode

        @param[in] key_op operation to execute on each key
        \see For_Each_In_Order
    */
    template <class Key_Op>
    void for_each_inorder(Key_Op & key_op) const
    {
      Node *root = static_cast<Node *>(tree.getRoot());

      Node_Op<Key_Op> node_op(const_cast<Key_Op &>(key_op));

      For_Each_In_Order<Node>()(root, node_op);
    }

    /** \overload
     *
     *  Performs an infix traversal over all keys using a temporary operation object.
     *
     *  @tparam Key_Op Operation type (default-constructible).
     *  @param[in] key_op Operation to execute on each key (default-constructed if not provided).
     */
    template <class Key_Op>
    void for_each_inorder(Key_Op && key_op = Key_Op()) const
    {
      for_each_inorder<Key_Op>(key_op);
    }

    /** Performs a postfix traversal over all keys in the set and
        invokes operation Op.

        <code>Op(p)</code> has the following structure:
        \code
        struct Key_Op
        {
        // state attributes to maintain

        Key_Op(...) // optional constructor if initialization is needed
        {
        // initialization
        }

        void operator () (const Key & key)
        {
        // operation on key
        }
        };
        \endcode

        @param[in] key_op operation to execute on each key
        \see For_Each_Postorder
    */
    template <class Key_Op>
    void for_each_postorder(Key_Op & key_op) const
    {
      Node *root = static_cast<Node *>(tree.getRoot());

      Node_Op<Key_Op> node_op(const_cast<Key_Op &>(key_op));

      For_Each_Postorder<Node>()(root, node_op);
    }

    /** \overload
     *
     *  Performs a postorder traversal over all keys using a temporary operation object.
     *
     *  @tparam Key_Op Operation type (default-constructible).
     *  @param[in] key_op Operation to execute on each key (default-constructed if not provided).
     */
    template <class Key_Op>
    void for_each_postorder(Key_Op && key_op = Key_Op()) const
    {
      for_each_postorder<Key_Op>(key_op);
    }

    /** Union of two binary search trees.

        join(t,dup) builds a binary search tree corresponding to the
        union of this with t. Duplicate keys are inserted in dup.

        @param[in] t binary search tree to join with this.
        @param[out] dup binary search tree with duplicate keys from t.
        @note After the calls, tree t becomes empty and this becomes
        the union of both trees.
        @return this
        @ingroup Trees
    */
    DynSetTree &join(DynSetTree & t, DynSetTree & dup)
    {
      tree.join(t.tree, dup.tree);
      t.tree.getRoot() = Node::NullPtr;
      t.num_nodes = 0;
      num_nodes = compute_cardinality_rec(tree.getRoot());
      dup.num_nodes = compute_cardinality_rec(dup.tree.getRoot());
      return *this;
    }

    /** \overload
     *
     *  Union of two binary search trees using a move-constructed duplicate tree.
     *
     *  @param[in] t Binary search tree to join with this.
     *  @param[in] dup Binary search tree with duplicate keys (move-constructed, default empty).
     *  @return Reference to this tree.
     *  @note After the call, tree t becomes empty and this becomes the union of both trees.
     */
    DynSetTree &join(DynSetTree & t, DynSetTree && dup = DynSetTree())
    {
      return join(t, dup);
    }

    /** Union of two binary search trees.

        join_dup(t) builds a binary search tree
        corresponding to the union of this with t in which there may be
        duplicate keys.

        @param[in] t binary search tree that you want to join to this.
        @note After the calls the t-tree becomes empty and this becomes
        the union of both trees;
        @return this
        @ingroup Trees
    */
    DynSetTree &join_dup(DynSetTree & t)
    {
      tree.join_dup(t.tree);
      t.num_nodes = 0;
      t.tree.getRoot() = Node::NullPtr;
      num_nodes = compute_cardinality_rec(tree.getRoot());
      return *this;
    }

    /** Partitions the binary search tree based on a key.

        split_key(key,l,r) partitions, according to the key key, the
        binary search tree this in two trees l and r. After the
        operation the tree this becomes empty, l contains
        all keys less than key and r the major ones.

        @param[in] key partition key.
        @param[out] l tree with keys less than key.
        @param[out] r tree with keys greater than key.
        @return true if key is not inside the binary tree; in which
        case the partition was done successfully. Otherwise, yes
        key is located inside the tree, the tree is not partitioned and the
        result is false.
    */
    bool split_key(const Key & key, DynSetTree & l, DynSetTree & r)
    {
      if (not tree.split_key(key, l.tree, r.tree))
        return false;

      tree.getRoot() = Node::NullPtr;
      num_nodes = 0;
      l.num_nodes = compute_cardinality_rec(l.tree.getRoot());
      r.num_nodes = compute_cardinality_rec(r.tree.getRoot());

      return true;
    }

    /** Partitions the binary search tree based on an infix position.

        split_pos(pos,l,r) partitions the binary search tree this
        into two trees l and r. After the operation, tree this
        becomes empty, l contains the first pos+1 keys and r the
        remaining ones.

        @param[in] pos partition position
        @param[out] l tree with keys in interval [0,pos]
        @param[out] r tree with keys in interval (pos,N),
        where N is the number of keys
    */
    void split_pos(const size_t pos, DynSetTree & l, DynSetTree & r)
    {
      ah_domain_error_if_constexpr(not Has_Range_Methods<Tree_Type>::has_split_pos)
        << "split_pos is not supported by underlying tree";

      ah_out_of_range_error_if(pos >= num_nodes)
        << "split_pos position out of range";

      // Underlying split_pos splits at [0, pos) and [pos, N)
      // But we want [0, pos] and (pos, N), so we split at pos+1
      call_split_pos(tree, pos + 1, l.tree, r.tree, 0);
      tree.getRoot() = Node::NullPtr;
      num_nodes = 0;
      l.num_nodes = compute_cardinality_rec(l.tree.getRoot());
      r.num_nodes = compute_cardinality_rec(r.tree.getRoot());
    }

    /** Partitions the binary search tree based on a key that
        may be present in the tree.

        split_key_dup(key,l,r) partitions, according to key, the
        binary search tree this into two trees l and r. After the
        operation, tree this becomes empty, l contains all keys
        less than key and r contains keys greater than or equal to key.

        @param[in] key partition key.
        @param[out] l tree with keys less than key.
        @param[out] r tree with keys greater than or equal to key.
    */
    void split_key_dup(const Key & key, DynSetTree & l, DynSetTree & r)
    {
      tree.split_key_dup(key, l.tree, r.tree);
      tree.getRoot() = Node::NullPtr;
      num_nodes = 0;
      l.num_nodes = compute_cardinality_rec(l.tree.getRoot());
      r.num_nodes = compute_cardinality_rec(r.tree.getRoot());
    }

    struct Iterator : public Tree_Type::Iterator
    {
      using Base = typename Tree_Type::Iterator;

      using Set_Type = DynSetTree;

      /// Default constructor creates an "end" iterator
      Iterator() noexcept = default;

      Iterator(const DynSetTree & tree) : Base(tree.tree)
      { /* empty */
      }

      const Key &get_curr_ne() const noexcept
      {
        return Base::get_curr_ne()->get_key();
      }

      Key &get_curr_ne() noexcept { return Base::get_curr_ne()->get_key(); }

      const Key &get_curr() const { return Base::get_curr()->get_key(); }

      Key &get_curr() { return Base::get_curr()->get_key(); }
    };

    /** Traverse all the set of pairs and for each key executes the operation op.

        Operation must have the signature

        \code
        bool operation(T & item)
        \endcode

        If \code operation() \endcode returns false then the traversal is
        aborted; otherwise the routine advance and so on

        @param[in] op operation to execute for each key
        @return true if all items are traversed; false otherwise
    */
    template <class Operation>
    bool traverse(Operation & op)
    {
      return Aleph::traverse(tree.getRoot(), [&op](Node *p)
                               {
                                 return op(p->get_key());
                               });
    }

    template <class Operation>
    bool traverse(Operation && op = Operation())
    {
      return traverse<Operation>(op);
    }

    template <class Operation>
    bool traverse(Operation & op) const
    {
      return Aleph::traverse(tree.getRoot(), [&op](Node *p)
                               {
                                 return op(p->get_key());
                               });
    }

    template <class Operation>
    bool traverse(Operation && op = Operation()) const
    {
      return traverse<Operation>(op);
    }
  };


# define SETTREE_ITOR(Name, Key, Cmp)                           \
  class Iterator : public DynSetTree<Key, Name, Cmp>::Iterator  \
  {                                                             \
  public:                                                       \
    Iterator() : DynSetTree<Key, Name, Cmp>::Iterator()         \
      { /* empty */ }                                           \
                                                                \
    Iterator(DynSetTree<Key, Name, Cmp> & tree)                 \
      : DynSetTree<Key, Name, Cmp>::Iterator(tree)              \
      { /* empty */ }                                           \
  };


  /** Dynamic set implemented using binary search trees
      of type BinTree<Key>.

      @see DynSetTree DynMapTree BinTree<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetBinTree : public DynSetTree<Key, BinTree, Compare>
  {
  public:
    using Base = DynSetTree<Key, BinTree, Compare>;
    using Base::Base;
  };


  /** Dynamic set implemented using AVL binary search trees
      of type Avl_Tree<Key>.

      @see DynSetTree DynMapTree Avl_Tree<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetAvlTree : public DynSetTree<Key, Avl_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Avl_Tree, Compare>;
    using Base::Base;
  };


  /** Dynamic set implemented using splay binary search trees
      of type Splay_Tree<Key>.

      @see DynSetTree DynMapTree Splay_Tree<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetSplayTree : public DynSetTree<Key, Splay_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Splay_Tree, Compare>;
    using Base::Base;
  };


  /** Dynamic set implemented using splay trees with rank support
      of type Splay_Tree_Rk<Key>.

      Splay trees are self-adjusting BSTs that move recently accessed
      elements to the root, providing amortized O(log n) for all operations.
      Frequently accessed elements have faster access times.

      This extended version maintains subtree counts for O(log n) rank
      operations (select, position).

      @see DynSetTree DynMapTree Splay_Tree_Rk<Key> DynSetSplayTree
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetSplayRkTree : public DynSetTree<Key, Splay_Tree_Rk, Compare>
  {
  public:
    using Base = DynSetTree<Key, Splay_Tree_Rk, Compare>;
    using Base::Base;
    SETTREE_ITOR(Splay_Tree_Rk, Key, Compare);
  };


  /** Dynamic set implemented using randomized binary search trees
      of type Rand_Tree<Key>.

      @see DynSetTree DynMapTree Rand_Tree<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetRandTree : public DynSetTree<Key, Rand_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Rand_Tree, Compare>;
    using Base::Base;

    class Iterator : public DynSetTree<Key, Rand_Tree, Compare>::Iterator
    {
    public:
      Iterator() : DynSetTree<Key, Rand_Tree, Compare>::Iterator()
      { /* empty */
      }

      Iterator(DynSetTree<Key, Rand_Tree, Compare> & tree)
        : DynSetTree<Key, Rand_Tree, Compare>::Iterator(tree)
      { /* empty */
      }
    };
  };


  /** Dynamic set implemented using randomized treap binary search trees
      of type Treap<Key>.

      @see DynSetTree DynMapTree Treap<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetTreap : public DynSetTree<Key, Treap, Compare>
  {
  public:
    using Base = DynSetTree<Key, Treap, Compare>;
    using Base::Base;
  };

  /** Dynamic set implemented using extended treap binary search trees
      with rank support of type Treap_Rk<Key>.

      @see DynSetTree DynMapTree Treap_Rk<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetTreapRk : public DynSetTree<Key, Treap_Rk, Compare>
  {
  public:
    using Base = DynSetTree<Key, Treap_Rk, Compare>;
    using Base::Base;
    SETTREE_ITOR(Treap_Rk, Key, Compare);
  };


  /** Dynamic set implemented using extended AVL binary search trees
      with rank support of type Avl_Tree_Rk<Key>.

      Supports O(log n) select and position operations.

      @see DynSetTree DynMapTree Avl_Tree_Rk<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetAvlRkTree : public DynSetTree<Key, Avl_Tree_Rk, Compare>
  {
  public:
    using Base = DynSetTree<Key, Avl_Tree_Rk, Compare>;
    using Base::Base;
    SETTREE_ITOR(Avl_Tree_Rk, Key, Compare);
  };


  /** Dynamic set implemented using Red-Black binary search trees
      of type Rb_Tree<Key> (bottom-up implementation).

      @see DynSetTree DynMapTree Rb_Tree<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetRbTree : public DynSetTree<Key, Rb_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Rb_Tree, Compare>;
    using Base::Base;
  };


  /** Dynamic set implemented using Top-Down Red-Black binary search trees
      of type TdRbTree<Key>.

      The top-down implementation performs all balancing operations during
      a single pass down the tree, which can be more cache-friendly and
      avoids the need for a stack to track the ancestor path.

      @see DynSetTree DynMapTree TdRbTree<Key> DynSetRbTree
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetTdRbTree : public DynSetTree<Key, TdRbTree, Compare>
  {
  public:
    using Base = DynSetTree<Key, TdRbTree, Compare>;
    using Base::Base;
  };


  /** Dynamic set implemented using extended Red-Black binary search trees
      with rank support of type Rb_Tree_Rk<Key> (bottom-up implementation).

      Supports O(log n) select and position operations.

      @see DynSetTree DynMapTree Rb_Tree_Rk<Key>
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetRbRkTree : public DynSetTree<Key, Rb_Tree_Rk, Compare>
  {
  public:
    using Base = DynSetTree<Key, Rb_Tree_Rk, Compare>;
    using Base::Base;
    SETTREE_ITOR(Rb_Tree_Rk, Key, Compare);
  };


  /** Dynamic set implemented using Top-Down Red-Black binary search trees
      with rank support of type TdRbTreeRk<Key>.

      The top-down implementation performs all balancing operations during
      a single pass down the tree. Supports O(log n) select and position.

      @see DynSetTree DynMapTree TdRbTreeRk<Key> DynSetRbRkTree
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetTdRbRkTree : public DynSetTree<Key, TdRbTreeRk, Compare>
  {
  public:
    using Base = DynSetTree<Key, TdRbTreeRk, Compare>;
    using Base::Base;
    SETTREE_ITOR(TdRbTreeRk, Key, Compare);
  };


  /** Dynamic set implemented using Hybrid Top-Down/Bottom-Up Red-Black trees
      of type HtdRbTree<Key>.

      This hybrid approach combines:
      - Top-down insertion with proactive color flipping
      - Bottom-up deletion with stack-based ancestor tracking

      This can be more efficient than pure top-down for workloads with
      many deletions.

      @see DynSetTree DynMapTree HtdRbTree<Key> DynSetRbTree DynSetTdRbTree
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetHtdRbTree : public DynSetTree<Key, HtdRbTree, Compare>
  {
  public:
    using Base = DynSetTree<Key, HtdRbTree, Compare>;
    using Base::Base;
    SETTREE_ITOR(HtdRbTree, Key, Compare);
  };


  /** Dynamic set implemented using Hybrid Red-Black trees with rank support
      of type HtdRbTreeRk<Key>.

      This is the only red-black implementation with rank support that achieves
      O(log n) for ALL operations including removal. Combines:
      - Top-down insertion with rank updates
      - Bottom-up deletion with rank updates during stack unwind

      Supports O(log n) select and position operations.

      @see DynSetTree DynMapTree HtdRbTreeRk<Key> DynSetRbRkTree DynSetTdRbRkTree
      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  class DynSetHtdRbRkTree : public DynSetTree<Key, HtdRbTreeRk, Compare>
  {
  public:
    using Base = DynSetTree<Key, HtdRbTreeRk, Compare>;
    using Base::Base;
    SETTREE_ITOR(HtdRbTreeRk, Key, Compare);
  };


  template <typename T, class Op, class C>
  DynSetTree<T> set_unify(const C & c, Op op)
  {
    DynSetTree<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.insert(op(it.get_curr()));
    return ret;
  }
} // end namespace Aleph

# endif /* TPL_DYNSETTREE_H */
