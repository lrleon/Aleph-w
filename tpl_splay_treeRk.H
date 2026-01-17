
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

/** @file tpl_splay_treeRk.H
    @brief Top-down splay tree with rank support.

    This file implements a splay tree with node counts (rank support),
    enabling O(log n) access to the k-th element. The implementation is
    based on Danny Sleator's original splay tree code, adapted to C++
    with extended functionality.

    Original source: http://www.link.cs.cmu.edu/link/ftp-site/splaying/SplayTree.java

    @see Splay_Tree_Rk Convenient typedef.
    @see DynSetSplayTree High-level wrapper with automatic memory management.
    @ingroup Trees
 *  @author Leandro Rabindranath León
*/

# ifndef TPL_SPLAY_TREE_RK_H
# define TPL_SPLAY_TREE_RK_H

# include <tpl_binNodeXt.H>
# include <ah-errors.H>

using namespace Aleph;


/** @brief Top-down splay tree with rank support.

    A splay tree is a self-adjusting binary search tree that moves
    recently accessed elements closer to the root. This provides
    amortized O(log n) time complexity for all operations, with
    frequently accessed elements having faster access times.

    This implementation extends the basic splay tree with COUNT fields
    in each node, enabling:
    - O(log n) selection of the k-th smallest element
    - O(log n) rank queries (position of an element)
    - All standard BST operations with amortized O(log n) complexity

    The splay operation brings the accessed node to the root using
    a sequence of zig, zig-zig, and zig-zag rotations. This top-down
    variant performs splaying during the descent, avoiding a second pass.

    @tparam NodeType Template for node type (typically BinNodeXt).
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor for ordering keys.

    @par Complexity (amortized):
    - Search: O(log n)
    - Insert: O(log n)
    - Delete: O(log n)
    - Select k-th: O(log n)
    - Rank query: O(log n)

    @par Example:
    @code
    Splay_Tree_Rk<int> tree;
    tree.insert(new Splay_Tree_Rk<int>::Node(42));
    tree.insert(new Splay_Tree_Rk<int>::Node(17));
    tree.insert(new Splay_Tree_Rk<int>::Node(99));

    // Find 2nd smallest element
    auto node = tree.select(1);  // 0-indexed

    // Get rank of element
    size_t pos = tree.position(KEY(node));
    @endcode

    @note This is a low-level implementation managing raw nodes.
          For automatic memory management, use DynSetSplayTree.

    @see Splay_Tree Basic splay tree without rank support.
    @see DynSetSplayTree High-level wrapper.
    @ingroup Trees
*/
template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTreeRk
{
public:
  typedef NodeType<Key> Node;

private:
  Node *root;
  Compare cmp;
  static constexpr signed char CmpLess = -1;
  static constexpr signed char CmpEqual = 0;
  static constexpr signed char CmpGreater = 1;

  signed char splay_impl(const Key & key) noexcept
  {
    assert(root != Node::NullPtr); // splay requires non-empty tree
    Node header(sentinelCtor);
    Node * head_ptr = &header;
    Node * l = head_ptr;
    Node * r = head_ptr;
    Node * t = root;

    size_t l_size = 0;
    size_t r_size = 0;
    signed char cmp_result = CmpEqual;

    while (true)
      {
        // Prefetch both children to reduce memory latency
        __builtin_prefetch(LLINK(t));
        __builtin_prefetch(RLINK(t));

        if (const Key & tk = KEY(t); cmp(key, tk))  // key < tk: go left
          {
            cmp_result = CmpLess;
            if (LLINK(t) == Node::NullPtr) [[unlikely]]
              break;

            if (cmp(key, KEY(LLINK(t))))  // zig-zig case
              {
                t = rotate_to_right_xt(t);
                if (LLINK(t) == Node::NullPtr) [[unlikely]]
                  break;
              }

            LLINK(r) = t;
            r = t;
            t = LLINK(t);
            r_size += 1 + COUNT(RLINK(r));
          }
        else if (cmp(tk, key))  // tk < key: go right
          {
            cmp_result = CmpGreater;
            if (RLINK(t) == Node::NullPtr) [[unlikely]]
              break;

            if (cmp(KEY(RLINK(t)), key))  // zig-zig case
              {
                t = rotate_to_left_xt(t);
                if (RLINK(t) == Node::NullPtr) [[unlikely]]
                  break;
              }

            RLINK(l) = t;
            l = t;
            t = RLINK(t);
            l_size += 1 + COUNT(LLINK(l));
          }
        else [[unlikely]]  // key == tk: found
          {
            cmp_result = CmpEqual;
            break;
          }
      }

    l_size += COUNT(LLINK(t));
    r_size += COUNT(RLINK(t));
    COUNT(t) = l_size + r_size + 1;

    /* reassembling of subtrees in one with t as root */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);

    LLINK(t) = RLINK(head_ptr);
    RLINK(t) = LLINK(head_ptr);

    for (Node * y = LLINK(t); y != Node::NullPtr; y = RLINK(y))
      {
        COUNT(y) = l_size;
        l_size -= 1 + COUNT(LLINK(y));
      }

    for (Node * y = RLINK(t); y != Node::NullPtr; y = LLINK(y))
      {
        COUNT(y) = r_size;
        r_size -= 1 + COUNT(RLINK(y));
      }

    root = t;

    return cmp_result;
  }

public:
  /// The key type stored in the node
  typedef Key key_type;

  /// Returns a reference to the comparison functor
  Compare &key_comp() { return cmp; }

  /// \overload key_comp()
  Compare &get_compare() { return key_comp(); }

  /** search key within tree and splay that node.

      @warning The tree must not be empty. All public methods check for
      empty tree before calling splay().
   */
  void splay(const Key & key) noexcept
  {
    (void) splay_impl(key);
  }

  /// Constructor
  GenTdSplayTreeRk(Compare __cmp = Compare())
    : root(Node::NullPtr), cmp(__cmp)
  {
    // Empty
  }

  void swap(GenTdSplayTreeRk & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  /// Destructor
  virtual ~GenTdSplayTreeRk()
  { /* empty */
  }

private:
  Node * __insert(Node *p)
  {
    COUNT(p) = COUNT(root) + 1;
    const Key & pk = KEY(p);
    const Key & rk = KEY(root);
    if (cmp(pk, rk))
      {
        COUNT(root) -= COUNT(LLINK(root));
        LLINK(p) = LLINK(root);
        RLINK(p) = root;
        LLINK(root) = Node::NullPtr;
      }
    else
      {
        COUNT(root) -= COUNT(RLINK(root));
        RLINK(p) = RLINK(root);
        LLINK(p) = root;
        RLINK(root) = Node::NullPtr;
      }

    return root = p; // inserted node become root
  }

public:
  /** Inserts a node in a top down splay tree.

      @return a pointer to the inserted node if node is not found in tree;
      nullptr otherwise.
      @param p a pointer to the node to be inserted
  */
  Node * insert(Node *p)
  {
    assert(p != Node::NullPtr);
    assert(COUNT(p) == 1);
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

  Node * insert_dup(Node *p)
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
     Searches a key in a top down splay tree.

     @return a pointer to the node containing the key if the key is found;
     nullptr otherwise.
     @param key key to search
  */
  Node * search(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    const signed char cmp_result = splay_impl(key);

    return cmp_result == CmpEqual ? root : nullptr;
  }

  Node * search_or_insert(Node *p)
  {
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);
    const signed char cmp_result = splay_impl(key);
    if (cmp_result == CmpEqual)
      return root;

    return __insert(p);
  }

private:
  /** Splay the maximum element to the root.

      Uses the same zig-zig and zig rotations as standard splay to maintain
      the amortized O(log n) guarantee. Unlike splay(key), this function
      always brings the actual maximum to the root, which is needed for
      correct behavior when there are duplicate keys.

      @param r root of the subtree
      @return the new root (the maximum element)
   */
  Node * splay_max(Node * r) noexcept
  {
    if (r == Node::NullPtr || RLINK(r) == Node::NullPtr)
      return r;

    Node header(sentinelCtor);
    Node * head_ptr = &header;
    Node * l = head_ptr;
    Node * t = r;

    size_t l_size = 0;

    // Always go right to find maximum
    while (RLINK(t) != Node::NullPtr)
      {
        // Zig-zig: if grandchild exists on the right, rotate twice
        if (RLINK(RLINK(t)) != Node::NullPtr)
          {
            t = rotate_to_left_xt(t);
            if (RLINK(t) == Node::NullPtr)
              break;
          }

        // Link left: attach t to l's right
        RLINK(l) = t;
        l = t;
        t = RLINK(t);
        l_size += 1 + COUNT(LLINK(l));
      }

    // Reassemble
    l_size += COUNT(LLINK(t));
    COUNT(t) = l_size + 1;

    RLINK(l) = LLINK(t);
    LLINK(t) = RLINK(head_ptr);

    // Recompute counts along the left spine
    for (Node * y = LLINK(t); y != Node::NullPtr; y = RLINK(y))
      {
        COUNT(y) = l_size;
        l_size -= 1 + COUNT(LLINK(y));
      }

    return t;
  }

public:
  /**
     Remove a key from a top-down splay tree.

     Searches a key in a top-down splay tree and remove the containing
     the key if this is found.

     @return a pointer to node containing the removed key.
     @param key to search
  */
  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    const signed char cmp_result = splay_impl(key);
    if (cmp_result != CmpEqual)
      return nullptr; /* key not found */

    Node *ret_val = root; /* store node to delete */

    if (LLINK(root) == Node::NullPtr)
      root = RLINK(root);
    else if (RLINK(root) == Node::NullPtr)
      root = LLINK(root);
    else
      {
        Node *right_subtree = RLINK(root);

        // Splay the maximum of left subtree to become root.
        // We use splay_max() instead of splay(key) because splay(key) can
        // bring a duplicate that is not the actual maximum, corrupting counts.
        // splay_max() uses proper zig-zig rotations to maintain O(log n) amortized.
        root = splay_max(LLINK(root));

        // The new root has no right child (it was the maximum)
        // Connect the right subtree
        RLINK(root) = right_subtree;
        COUNT(root) = COUNT(LLINK(root)) + 1 + COUNT(right_subtree);
      }

    ret_val->reset();

    return ret_val;
  }

  /// Get the top-down splay tree's root
  Node *&getRoot()
  {
    return root;
  }

  bool verify() const { return check_rank_tree(root) and check_bst(root, cmp); }

  /// Returns the number of nodes stored in the tree.
  size_t size() const
  {
    return COUNT(root);
  }

  /// Returns true if the tree is empty.
  bool is_empty() const
  {
    return root == Node::NullPtr;
  }

  /** Returns the inorder (sorted) position of key.

      `position(key)` searches in the extended splay tree for key (which
      takes \f$O(\lg n)\f$ time) and returns the inorder position of the
      node that contains the key.

      @param[in] key key to search and whose inorder position is requested.
      @return a pair containing the inorder position of key within the
      ordered set together with the node if the key is found in the tree;
      otherwise it returns -1 and the node is unspecified.
   */
  std::pair<long, Node *> position(const Key & key)
  {
    if (root == Node::NullPtr)
      return {-1, nullptr};

    const signed char cmp_result = splay_impl(key);
    if (cmp_result == CmpEqual)
      return {static_cast<long>(COUNT(LLINK(root))), root};

    return {-1, nullptr};
  }

  /** Returns the inorder (sorted) position of key.

      `find_position(key)` searches in the extended splay tree for key
      (which takes \f$O(\lg n)\f$ time) and returns the inorder position
      of the node that contains the key together with the node. If the
      key is not in the tree, `find_position(key)` returns the position
      where it would be if key belonged to the tree.

      The return value is a `pair<long, Node*>`. The first component is
      the inorder position and the second is the node.

      If key is not found in the tree, three cases can be distinguished:
      -# If key is smaller than the minimum key in the tree, then `first`
         is -1 and `second` is the node containing the minimum key.
      -# If key is greater than the maximum key in the tree, then `first`
         is COUNT(root) (number of nodes in the tree) and `second` is the
         node containing the maximum key.
      -# In any other case, `first` is the position that key would have in
         the tree and `second` (node `p`) is a key immediately adjacent to
         key. Note that `second` can hold a smaller or greater key, but it
         is guaranteed to be immediately adjacent to key.

      @param[in] key key to search and whose inorder position is requested.
      @return a pair containing the inorder position of key within the
      ordered set together with the node if the key is found in the tree;
      otherwise it returns -1 and the node is unspecified.
   */
  std::pair<long, Node *> find_position(const Key & key)
  {
    if (root == Node::NullPtr)
      return {-1, nullptr};

    const signed char cmp_result = splay_impl(key);
    if (cmp_result == CmpEqual)
      return {static_cast<long>(COUNT(LLINK(root))), root};

    // key not found - determine position it would have
    if (cmp_result == CmpLess)
      return {static_cast<long>(COUNT(LLINK(root))) - 1, root}; // less than root
    return {static_cast<long>(COUNT(LLINK(root))) + 1, root};
    // greater than root
  }

  /** Returns the node whose inorder position in the extended tree is i.

      `select(i)` returns the node in the extended splay tree whose
      inorder position is i.

      @param[in] i inorder position to select.
      @return pointer to the node corresponding to inorder position i.
      @throw out_of_range if i is greater than or equal to the number of
      nodes in the tree.
  */
  Node * select(const size_t & i)
  {
    return Aleph::select(root, i);
  }

  /** Inorder iterator over the extended splay tree.

      Allows traversing the tree elements in order.
   */
  class Iterator
  {
    GenTdSplayTreeRk *tree_ptr = nullptr;
    Node *curr = Node::NullPtr;
    size_t pos = 0;

  public:
    Iterator() = default;

    Iterator(GenTdSplayTreeRk & tree) noexcept
      : tree_ptr(&tree), curr(Node::NullPtr), pos(0)
    {
      if (tree_ptr->root != Node::NullPtr)
        curr = find_min(tree_ptr->root);
    }

    bool has_curr() const noexcept { return curr != Node::NullPtr; }

    Node * get_curr() const
    {
      ah_overflow_error_if(curr == Node::NullPtr)
          << "Iterator is at end";
      return curr;
    }

    Node * get_curr_ne() const noexcept { return curr; }

    void next()
    {
      ah_overflow_error_if(curr == Node::NullPtr)
          << "Iterator is at end";
      curr = inorder_successor(tree_ptr->root, KEY(curr));
      ++pos;
    }

    void next_ne() noexcept
    {
      curr = inorder_successor(tree_ptr->root, KEY(curr));
      ++pos;
    }

    size_t get_pos() const noexcept { return pos; }

    void reset_first() noexcept
    {
      pos = 0;
      if (tree_ptr != nullptr && tree_ptr->root != Node::NullPtr)
        curr = find_min(tree_ptr->root);
      else
        curr = Node::NullPtr;
    }

  private:
    static Node * find_min(Node *r) noexcept
    {
      while (LLINK(r) != Node::NullPtr)
        r = LLINK(r);
      return r;
    }

    static Node * inorder_successor(Node *root, const Key & key) noexcept
    {
      Node *successor = Node::NullPtr;
      Node *curr = root;

      while (curr != Node::NullPtr)
        {
          if (key < KEY(curr))
            {
              successor = curr;
              curr = LLINK(curr);
            }
          else
            curr = RLINK(curr);
        }

      return successor;
    }
  };
};


template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Rk : public GenTdSplayTreeRk<BinNodeXt, Key, Compare>
{
  using Base = GenTdSplayTreeRk<BinNodeXt, Key, Compare>;
  using Base::Base;
};


template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Rk_Vtl : public GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>
{
  using Base = GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>;
  using Base::Base;
};


#endif /* TPL_SPLAY_TREE_RK_H */
