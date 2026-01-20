
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

/** @file tpl_splay_tree.H
 *  @brief Top-down splay tree implementation (without rank support).
 *
 *  This file provides `Splay_Tree`, a self-adjusting binary search tree
 *  that moves accessed nodes to the root via rotations (splaying). This
 *  provides amortized O(log n) performance and excellent cache locality
 *  for frequently accessed items.
 *
 *  @note This code is adapted from Danny Sleator's implementation.
 *  @see http://www.link.cs.cmu.edu/link/ftp-site/splaying/
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SPLAY_TREE_H
# define TPL_SPLAY_TREE_H

# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;


/** @brief Top-down splay tree - Self-adjusting BST with amortized O(log n) operations.

    GenTdSplayTree implements a splay tree, a self-adjusting binary search tree
    where every access operation (search, insert, delete) moves the accessed node
    to the root through a sequence of tree rotations called "splaying".

    @par Key Characteristics:
    - **Self-adjusting**: Frequently accessed items migrate toward the root
    - **Amortized O(log n)**: While individual operations may be O(n), sequences
      of m operations take O(m log n) time total
    - **Cache-friendly**: Recent accesses stay near the root for fast re-access
    - **Simple implementation**: No balance information or color bits needed
    - **No worst-case guarantee**: Individual operations can be O(n)

    @par Splaying Rotations:
    - **Zig**: Single rotation when node is child of root
    - **Zig-zig**: Two rotations in same direction (child and parent on same side)
    - **Zig-zag**: Two rotations in opposite directions (child and parent on different sides)

    These rotations ensure amortized efficiency by reducing tree depth.

    @par When to Use Splay Trees:
    ✓ Access patterns have locality (same items accessed repeatedly)
    ✓ Amortized complexity is acceptable (no hard real-time constraints)
    ✓ Simplicity preferred over guaranteed worst-case performance
    ✓ Cache performance is critical

    @par When NOT to Use:
    ✗ Need guaranteed O(log n) worst-case (use AVL or Red-Black instead)
    ✗ Hard real-time requirements
    ✗ Uniform random access patterns (no locality benefit)

    @tparam NodeType Node template (BinNode or BinNodeVtl).
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor for ordering keys.

    @par Complexity:
    - Search: O(log n) amortized
    - Insert: O(log n) amortized
    - Delete: O(log n) amortized
    - Individual operation worst case: O(n)
    - Space: O(n) - no extra balance information needed

    @par Example:
    @code
    Splay_Tree<int> tree;

    // Insert nodes
    tree.insert(new Splay_Tree<int>::Node(42));
    tree.insert(new Splay_Tree<int>::Node(17));
    tree.insert(new Splay_Tree<int>::Node(99));

    // Search (splays node to root)
    auto node = tree.search(42);
    if (node != nullptr)
      std::cout << "Found and splayed to root: " << node->get_key() << '\n';

    // Subsequent access to 42 is now O(1)
    node = tree.search(42);  // Very fast - already at root

    // Remove (splays then removes root)
    auto removed = tree.remove(42);
    delete removed;
    @endcode

    @note This implementation does NOT support rank operations.
          For indexed access, use Splay_Tree_Rk instead.
    @note Based on Danny Sleator's top-down splay tree implementation.
    @note This is a low-level implementation managing raw nodes.
          For automatic memory management, use DynSetSplayTree.

    @see Splay_Tree Convenient typedef for Splay_Tree<Key, Compare>.
    @see Splay_Tree_Rk Splay tree variant with rank support.
    @see Avl_Tree Alternative with O(log n) worst-case guarantee.
    @see DynSetSplayTree High-level wrapper with automatic memory management.
    @ingroup Trees
 *  @author Leandro Rabindranath León
 */
template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTree
{
public:
  using Node = NodeType<Key>;

private:
  Node headnode;
  Node *head, *&root;
  Compare cmp;
  static constexpr signed char CmpLess = -1;
  static constexpr signed char CmpEqual = 0;
  static constexpr signed char CmpGreater = 1;

  signed char splay_impl(const Key & key) noexcept
  {
    Node headNode;
    Node *l = &headNode;
    Node *r = &headNode;
    Node *t = root;
    signed char cmp_result = CmpEqual;

    while (true)
      {
        // Prefetch both children to reduce memory latency
        if (LLINK(t) != Node::NullPtr)
          __builtin_prefetch(LLINK(t));
        if (RLINK(t) != Node::NullPtr)
          __builtin_prefetch(RLINK(t));

        const Key & tk = KEY(t);
        if (cmp(key, tk))  // key < tk: go left
          {
            cmp_result = CmpLess;
            if (LLINK(t) == Node::NullPtr) [[unlikely]]
              break;

            if (cmp(key, KEY(LLINK(t))))  // zig-zig case
              {
                t = rotate_to_right(t);
                if (LLINK(t) == Node::NullPtr) [[unlikely]]
                  break;
              }

            LLINK(r) = t;
            r = t;
            t = LLINK(t);
          }
        else if (cmp(tk, key))  // tk < key: go right
          {
            cmp_result = CmpGreater;
            if (RLINK(t) == Node::NullPtr) [[unlikely]]
              break;

            if (cmp(KEY(RLINK(t)), key))  // zig-zig case
              {
                t = rotate_to_left(t);
                if (RLINK(t) == Node::NullPtr) [[unlikely]]
                  break;
              }

            RLINK(l) = t;
            l = t;
            t = RLINK(t);
          }
        else [[unlikely]]  // key == tk: found
          {
            cmp_result = CmpEqual;
            break;
          }
      }

    /* reassembling of subtrees in one with current as root */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);
    LLINK(t) = headNode.getR();
    RLINK(t) = headNode.getL();

    root = t;

    return cmp_result;
  }

public:
  /// The key type stored in the node
  using key_type = Key;

  /// Returns a reference to the comparison criteria
  Compare &key_comp() noexcept { return cmp; }

  /// \overload key_comp()
  Compare &get_compare() noexcept { return key_comp(); }

  /** search key within tree and splay that node, if not found it return
      Node::NullPtr */
  void splay(const Key & key) noexcept
  {
    (void) splay_impl(key);
  }

  /// Constructor
  GenTdSplayTree(Compare __cmp = Compare()) noexcept
    : head(&headnode), root(headnode.getR()), cmp(__cmp)
  {
    // Empty
  }

  void swap(GenTdSplayTree & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  /// Destructor
  virtual ~GenTdSplayTree() = default;

private:
  Node * __insert(Node *p) noexcept
  {
    const Key & pk = KEY(p);
    const Key & rk = KEY(root);
    if (cmp(pk, rk))
      { /* root is predecessor of p */
        LLINK(p) = LLINK(root);
        RLINK(p) = root;
        LLINK(root) = Node::NullPtr;
      }
    else
      { /* root is successor of p */
        RLINK(p) = RLINK(root);
        LLINK(p) = root;
        RLINK(root) = Node::NullPtr;
      }
    return root = p; // inserted node become root
  }

public:
  /** Inserts a node in a top-down splay tree.

      @return a pointer to the inserted node if node is not found in tree;
      nullptr otherwise.
      @param p a pointer to the node to be inserted
  */
  Node * insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    /* test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);

    const signed char cmp_result = splay_impl(key);

    if (cmp_result == CmpEqual)
      return nullptr; // item is already in tree

    return __insert(p);
  }

  Node * insert_dup(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    /* test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    splay_impl(KEY(p));

    return __insert(p);
  }

  /**
     Searches a key in a top-down splay tree.

     @return a pointer to the node containing the key if the key is found;
     nullptr otherwise.
     @param key key to search
  */
  Node * search(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;
      
    const signed char cmp_result = splay_impl(key);

    return cmp_result == CmpEqual ? root : nullptr;
  }

  Node * search_or_insert(Node *p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);
    const signed char cmp_result = splay_impl(key);
    if (cmp_result == CmpEqual)
      return root;

    return __insert(p);
  }

  /**
     Remove a key from a top down splay tree.

     Searches a key in a top down splay tree and remove the containing
     the key if this is found.

     @return a pointer to node containing the removed key.
     @param key to search
  */
  Node * remove(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    const signed char cmp_result = splay_impl(key);
    if (cmp_result != CmpEqual)
      return nullptr; /* key not found */

    Node *ret_val = root; /* store node to delete */

    if (LLINK(root) == Node::NullPtr)
      root = RLINK(root);
    else
      {
        Node *p = RLINK(root);
        root = LLINK(root);
        splay_impl(key);
        RLINK(root) = p;
      }

    ret_val->reset();

    return ret_val;
  }

  /// Get the top-down splay tree's root
  Node *&getRoot() noexcept
  {
    return root;
  }

  [[nodiscard]] bool verify() const { return check_bst(root, cmp); }

  /** Iterator over the nodes.

    This class allows traversing the tree nodes in sorted order according
    to the comparison criteria specified when the tree is instantiated.

   @ingroup Trees
*/
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    /// Default constructor creates an "end" iterator
    Iterator() noexcept = default;
    Iterator(GenTdSplayTree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};


template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree : public GenTdSplayTree<BinNode, Key, Compare>
{
  using Base = GenTdSplayTree<BinNode, Key, Compare>;
  using Base::Base;
};


template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Vtl : public GenTdSplayTree<BinNodeVtl, Key, Compare>
{
  using Base = GenTdSplayTree<BinNodeVtl, Key, Compare>;
  using Base::Base;
};


#endif /* TPL_SPLAY_TREE_H */
