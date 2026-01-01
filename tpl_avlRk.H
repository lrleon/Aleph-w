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

# ifndef TPL_AVLRK_H
# define TPL_AVLRK_H

# include <algorithm>
# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTreeOps.H>
# include <avlNodeRk.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

/** Extended AVL balanced binary search tree with select and position.

    An AVL tree is a binary search tree deterministically balanced.
    The maximum height of an AVL tree of n nodes is
    1.4404 * lg(n + 2) - 0.3277.

    This extended version maintains a count in each node representing
    the number of nodes in its subtree, enabling O(log n) operations
    for selecting by position and finding the position of a key.

    @see Avl_Tree_Rk Avl_Tree_Rk_Vtl
    @ingroup Arboles
*/
template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Avl_Tree_Rk
{
public:

  using Node = NodeType<Key>; /// The type of node

private:

  FixedStack<Node *> avl_stack;
  Node               head_node;
  Node *             head_ptr;
  Node *&            root;
  Compare            cmp;

  bool avl_stack_empty() noexcept { return avl_stack.top() == head_ptr; }

  void clean_avl_stack() noexcept { avl_stack.popn(avl_stack.size() - 1); }

  Node * search_and_stack_avl(const Key & key) noexcept
  {
    assert(avl_stack_empty());

    Node * p = root;
    do
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else if (cmp(KEY(p), key))
          p = RLINK(p);
        else
          return p; // duplicated key
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  Node * search_dup_and_stack_avl(const Key & key) noexcept
  {
    assert(avl_stack_empty());

    Node * p = root;
    do
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else
          p = RLINK(p);
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  // Rotate to left with counter update
  static Node * rotateLeft(Node * p) noexcept
  {
    assert(DIFF(p) == 2);
    assert(RLINK(p) != Node::NullPtr);

    Node * q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;

    // Update counters
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;

    if (DIFF(q) == 0)
      {
        DIFF(q) = -1;
        DIFF(p) = 1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  // Rotate to right with counter update
  static Node * rotateRight(Node * p) noexcept
  {
    assert(DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr);

    Node * q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;

    // Update counters
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;

    if (DIFF(q) == 0)
      {
        DIFF(q) = 1;
        DIFF(p) = -1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  // Double rotate left with counter update
  static Node * doubleRotateLeft(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(RLINK(p) != Node::NullPtr and LLINK(RLINK(p)) != Node::NullPtr);

    Node * q = RLINK(p);
    Node * r = LLINK(q);
    RLINK(p) = LLINK(r);
    LLINK(q) = RLINK(r);
    LLINK(r) = p;
    RLINK(r) = q;

    // Update counters
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;
    COUNT(r) = COUNT(LLINK(r)) + COUNT(RLINK(r)) + 1;

    unsigned char b, c;
    if (DIFF(r) == 1)
      { c = 1; b = 0; }
    else if (DIFF(r) == -1)
      { c = 0; b = 1; }
    else
      c = b = 1;

    DIFF(r) = 0;
    DIFF(p) = b - 1;
    DIFF(q) = 1 - c;

    return r;
  }

  // Double rotate right with counter update
  static Node * doubleRotateRight(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr and RLINK(LLINK(p)) != Node::NullPtr);

    Node * q = LLINK(p);
    Node * r = RLINK(q);
    LLINK(p) = RLINK(r);
    RLINK(q) = LLINK(r);
    RLINK(r) = p;
    LLINK(r) = q;

    // Update counters
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;
    COUNT(r) = COUNT(LLINK(r)) + COUNT(RLINK(r)) + 1;

    unsigned char b, c;
    if (DIFF(r) == 1)
      { c = 1; b = 0; }
    else if (DIFF(r) == -1)
      { c = 0; b = 1; }
    else
      c = b = 1;

    DIFF(r) = 0;
    DIFF(p) = 1 - c;
    DIFF(q) = b - 1;

    return r;
  }

  enum Rotation_Type
    { ROTATE_LEFT, ROTATE_RIGHT, DOUBLE_ROTATE_LEFT, DOUBLE_ROTATE_RIGHT };

  static Rotation_Type rotation_type(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);

    Node * pc;
    if (DIFF(p) == 2)
      {
        pc = RLINK(p);
        if (DIFF(pc) == 1 or DIFF(pc) == 0)
          return ROTATE_LEFT;
        return DOUBLE_ROTATE_LEFT;
      }

    pc = LLINK(p);
    if (DIFF(pc) == -1 or DIFF(pc) == 0)
      return ROTATE_RIGHT;

    return DOUBLE_ROTATE_RIGHT;
  }

  static Node * restore_avl(Node * p, Node * pp) noexcept
  {
    assert(LLINK(pp) == p or RLINK(pp) == p);
    assert(DIFF(p) == -2 or DIFF(p) == 2);

    Node ** link = LLINK(pp) == p ? &LLINK(pp) : &RLINK(pp);
    switch (rotation_type(p))
      {
      case ROTATE_LEFT:         return *link = rotateLeft(p);
      case ROTATE_RIGHT:        return *link = rotateRight(p);
      case DOUBLE_ROTATE_LEFT:  return *link = doubleRotateLeft(p);
      case DOUBLE_ROTATE_RIGHT: return *link = doubleRotateRight(p);
      default:
        AH_ERROR("Invalid rotation type");
        break;
      }

    return nullptr;
  }

  void restore_avl_after_insertion(Node * p) noexcept
  {
    Node * pp = avl_stack.pop();
    if (LLINK(pp) == p)
      --DIFF(pp);
    else
      ++DIFF(pp);

    if (DIFF(pp) == 0)
      {
        clean_avl_stack();
        return;
      }

    if (avl_stack_empty())
      return;

    do
      {
        Node * gpp = avl_stack.pop();
        if (LLINK(gpp) == pp)
          --DIFF(gpp);
        else
          ++DIFF(gpp);

        if (DIFF(gpp) == 0)
          break;
        if (DIFF(gpp) == -2 or DIFF(gpp) == 2)
          {
            Node * ggpp = avl_stack.pop();
            restore_avl(gpp, ggpp);
            break;
          }

        pp = gpp;
      }
    while (not avl_stack_empty());

    clean_avl_stack();
  }

  Node * swapWithSuccessor(Node * p, Node *& pp) noexcept
  {
    Node *& ref_to_stack_top = avl_stack.top();

    Node * fSucc = p;
    Node * succ = RLINK(p);

    avl_stack.push(succ);

    while (LLINK(succ) != Node::NullPtr)
      {
        fSucc = succ;
        succ = LLINK(succ);
        avl_stack.push(succ);
      }

    ref_to_stack_top = succ;
    avl_stack.top() = p;

    if (LLINK(pp) == p)
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p);
    LLINK(p) = Node::NullPtr;

    if (RLINK(p) == succ)
      {
        RLINK(p) = RLINK(succ);
        RLINK(succ) = p;
        pp = succ;
      }
    else
      {
        Node * succr = RLINK(succ);
        RLINK(succ) = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p) = succr;
        pp = fSucc;
      }

    // Swap balance factors
    DIFF(succ) = DIFF(p);

    // Swap counters
    COUNT(succ) = COUNT(p);

    return succ;
  }

  void restore_avl_after_deletion(bool left_deficit) noexcept
  {
    Node * pp = avl_stack.top(1);
    Node * ppp = avl_stack.popn(3);

    while (true)
      {
        if (left_deficit)
          ++DIFF(pp);
        else
          --DIFF(pp);

        if (DIFF(pp) == -2 or DIFF(pp) == 2)
          pp = restore_avl(pp, ppp);

        if (DIFF(pp) != 0 or pp == root)
          break;

        left_deficit = LLINK(ppp) == pp;
        pp = ppp;
        ppp = avl_stack.pop();
      }

    clean_avl_stack();
  }

  // Update counters along the stack after insertion
  void update_counters_after_insertion() noexcept
  {
    // Stack contains: [head_ptr, ..., nodes in path, ..., closest to insertion]
    // top(0) = closest node, top(size-1) = head_ptr (don't update)
    const size_t sz = avl_stack.size();
    for (size_t i = 0; i < sz - 1; ++i)
      ++COUNT(avl_stack.top(i));
  }

  // Update counters along the stack after deletion
  void update_counters_after_deletion() noexcept
  {
    const size_t sz = avl_stack.size();
    for (size_t i = 0; i < sz - 1; ++i)
      --COUNT(avl_stack.top(i));
  }

public:

  using key_type = Key; /// The key type

  /// Return a reference to the comparison criteria
  Compare & key_comp() noexcept { return cmp; }

  /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  Gen_Avl_Tree_Rk(Compare __cmp = Compare()) noexcept
    : avl_stack(Node::MaxHeight), head_ptr(&head_node),
      root(RLINK(head_ptr)), cmp(__cmp)
  {
    avl_stack.push(head_ptr);
  }

  void swap(Gen_Avl_Tree_Rk & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  virtual ~Gen_Avl_Tree_Rk() noexcept { assert(avl_stack_empty()); }

  /// Return a modifiable reference to tree's root
  Node *& getRoot() noexcept { return root; }

  Node * getRoot() const noexcept { return root; }

  /// Return the number of nodes in the tree
  [[nodiscard]] size_t size() const noexcept { return COUNT(root); }

  /// Return true if tree is empty
  [[nodiscard]] constexpr bool is_empty() const noexcept { return root == Node::NullPtr; }

  /// Search a node containing key
  Node * search(const Key & key) const noexcept
  {
    Node * result = searchInBinTree<Node, Compare>(root, key, cmp);
    return result == Node::NullPtr ? nullptr : result;
  }

  /** Insert the node pointed by p in the tree.

      @param[in] p the node to be inserted
      @return p if KEY(p) is not found in the tree, nullptr otherwise.
  */
  Node * insert(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (Node * pp = search_and_stack_avl(KEY(p)); cmp(KEY(p), KEY(pp)))
      LLINK(pp) = p;
    else if (cmp(KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      {
        clean_avl_stack();
        return nullptr;
      }

    update_counters_after_insertion();
    restore_avl_after_insertion(p);

    return p;
  }

  /** Search or insert a key.

      @param[in] p the node whose key must be searched and
      eventually inserted
      @return if p is inserted, this value is returned;
      otherwise, a pointer to the node containing KEY(p) is returned.
  */
  Node * search_or_insert(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (Node * pp = search_and_stack_avl(KEY(p)); cmp(KEY(p), KEY(pp)))
      LLINK(pp) = p;
    else if (cmp(KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      {
        clean_avl_stack();
        return pp;
      }

    update_counters_after_insertion();
    restore_avl_after_insertion(p);

    return p;
  }

  /// Insert the node p without testing for key duplicity
  Node * insert_dup(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (Node * pp = search_dup_and_stack_avl(KEY(p)); cmp(KEY(p), KEY(pp)))
      LLINK(pp) = p;
    else
      RLINK(pp) = p;

    update_counters_after_insertion();
    restore_avl_after_insertion(p);

    return p;
  }

  /// Remove from tree the node containing key.
  Node * remove(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    Node * p = search_and_stack_avl(key);
    if (no_equals<Key, Compare>(KEY(p), key, cmp))
      {
        clean_avl_stack();
        return nullptr;
      }

    Node * pp = avl_stack.top(1);
    bool left_deficit;

    while (true)
      {
        left_deficit = LLINK(pp) == p;
        if (LLINK(p) == Node::NullPtr)
          {
            if (LLINK(pp) == p)
              LLINK(pp) = RLINK(p);
            else
              RLINK(pp) = RLINK(p);
            break;
          }

        if (RLINK(p) == Node::NullPtr)
          {
            if (LLINK(pp) == p)
              LLINK(pp) = LLINK(p);
            else
              RLINK(pp) = LLINK(p);
            break;
          }

        swapWithSuccessor(p, pp);
      }

    p->reset();

    if (pp == head_ptr)
      {
        clean_avl_stack();
        return p;
      }

    update_counters_after_deletion();
    restore_avl_after_deletion(left_deficit);

    return p;
  }

  /** Return the i-th node in order sense.

      @param[in] i inorder position of node to be selected
      @return a pointer to the i-th node inorder sense
      @throw out_of_range if i is greater or equal than the number
      of nodes of tree
  */
  Node * select(const size_t i) const
  {
    return Aleph::select(root, i);
  }

  /** Compute the inorder position of a key.

      @param[in] key to be searched
      @return a pair with the inorder position and the node pointer.
      If key is not found, position is -1.
  */
  std::pair<long, Node*> position(const Key & key) const noexcept
  {
    std::pair<long, Node*> ret_val;

    ret_val.first = BinTreeXt_Operation<Node, Compare>(cmp).
      inorder_position(root, key, ret_val.second);

    return ret_val;
  }

  /** Find the inorder position of a key in the tree.

      @param[in] key to be searched
      @return a pair with the position and the node pointer
  */
  std::pair<long, Node*> find_position(const Key & key) const noexcept
  {
    std::pair<long, Node*> r(-2, nullptr);

    r.first = BinTreeXt_Operation<Node, Compare>(cmp).
      find_position(root, key, r.second);

    return r;
  }

  /// Return true if the tree is a valid AVL tree with correct counters
  [[nodiscard]] bool verify() const
  {
    return is_avl_rk(root) and check_rank_tree(root);
  }

private:

  // Compute height of AVL tree in O(log n) using balance factors
  static size_t avl_height(Node * p) noexcept
  {
    size_t h = 0;
    while (p != Node::NullPtr)
      {
        ++h;
        // Follow the taller subtree
        if (DIFF(p) >= 0)
          p = RLINK(p);  // right is taller or equal
        else
          p = LLINK(p);  // left is taller
      }
    return h;
  }

  // Extract and remove the minimum node from tree rooted at p
  // Returns the extracted node and updates p to the new root
  static Node * extract_min(Node *& p) noexcept
  {
    if (p == Node::NullPtr)
      return Node::NullPtr;

    if (LLINK(p) == Node::NullPtr)
      {
        Node * ret = p;
        p = RLINK(p);
        LLINK(ret) = RLINK(ret) = Node::NullPtr;
        COUNT(ret) = 1;
        DIFF(ret) = 0;
        return ret;
      }

    // Stack for rebalancing
    FixedStack<Node**> stack(Node::MaxHeight);
    Node ** pp = &p;

    while (LLINK(*pp) != Node::NullPtr)
      {
        stack.push(pp);
        pp = &LLINK(*pp);
      }

    Node * ret = *pp;
    *pp = RLINK(ret);

    // Update counts and rebalance going up
    while (not stack.is_empty())
      {
        Node ** parent_ptr = stack.pop();
        Node * parent = *parent_ptr;
        --COUNT(parent);
        ++DIFF(parent);  // left subtree got shorter

        if (DIFF(parent) == 2)
          {
            // Need to rebalance
            Node * q = RLINK(parent);
            if (DIFF(q) >= 0)
              *parent_ptr = rotateLeft(parent);
            else
              *parent_ptr = doubleRotateLeft(parent);
          }
        else if (DIFF(parent) == 1)
          break;  // height didn't change
      }

    LLINK(ret) = RLINK(ret) = Node::NullPtr;
    COUNT(ret) = 1;
    DIFF(ret) = 0;
    return ret;
  }

  // Extract and remove the maximum node from tree rooted at p
  static Node * extract_max(Node *& p) noexcept
  {
    if (p == Node::NullPtr)
      return Node::NullPtr;

    if (RLINK(p) == Node::NullPtr)
      {
        Node * ret = p;
        p = LLINK(p);
        LLINK(ret) = RLINK(ret) = Node::NullPtr;
        COUNT(ret) = 1;
        DIFF(ret) = 0;
        return ret;
      }

    FixedStack<Node**> stack(Node::MaxHeight);
    Node ** pp = &p;

    while (RLINK(*pp) != Node::NullPtr)
      {
        stack.push(pp);
        pp = &RLINK(*pp);
      }

    Node * ret = *pp;
    *pp = LLINK(ret);

    while (not stack.is_empty())
      {
        Node ** parent_ptr = stack.pop();
        Node * parent = *parent_ptr;
        --COUNT(parent);
        --DIFF(parent);  // right subtree got shorter

        if (DIFF(parent) == -2)
          {
            Node * q = LLINK(parent);
            if (DIFF(q) <= 0)
              *parent_ptr = rotateRight(parent);
            else
              *parent_ptr = doubleRotateRight(parent);
          }
        else if (DIFF(parent) == -1)
          break;
      }

    LLINK(ret) = RLINK(ret) = Node::NullPtr;
    COUNT(ret) = 1;
    DIFF(ret) = 0;
    return ret;
  }

  // Recursive join of two AVL trees where all keys in t1 < all keys in t2
  // h1 and h2 are the heights of t1 and t2 respectively
  static Node * join_exclusive_rec(Node * t1, size_t h1,
                                   Node * t2, size_t h2) noexcept
  {
    if (t1 == Node::NullPtr)
      return t2;
    if (t2 == Node::NullPtr)
      return t1;

    if (h1 >= h2)
      {
        // Extract max from t1 to use as pivot
        Node * pivot = extract_max(t1);
        if (t1 != Node::NullPtr)
          h1 = avl_height(t1);
        else
          h1 = 0;

        return join_with_pivot(t1, h1, pivot, t2, h2);
      }
    else
      {
        // Extract min from t2 to use as pivot
        Node * pivot = extract_min(t2);
        if (t2 != Node::NullPtr)
          h2 = avl_height(t2);
        else
          h2 = 0;

        return join_with_pivot(t1, h1, pivot, t2, h2);
      }
  }

  // Join t1 and t2 using pivot as the connecting node
  // All keys in t1 < pivot < all keys in t2
  static Node * join_with_pivot(Node * t1, const size_t h1, Node * pivot,
                                Node * t2, const size_t h2) noexcept
  {
    if (h1 <= h2 + 1 and h2 <= h1 + 1)
      {
        // Heights are close enough, pivot becomes root
        LLINK(pivot) = t1;
        RLINK(pivot) = t2;
        DIFF(pivot) = static_cast<signed char>(h2) - static_cast<signed char>(h1);
        COUNT(pivot) = COUNT(t1) + COUNT(t2) + 1;
        return pivot;
      }

    if (h1 > h2)
      {
        // t1 is taller, descend into right spine of t1
        Node * result = join_right(t1, h1, pivot, t2, h2);
        return result;
      }
    // t2 is taller, descend into left spine of t2
    Node * result = join_left(t1, h1, pivot, t2, h2);
    return result;
  }

  // Join when h1 > h2 + 1: descend right spine of t1
  static Node * join_right(Node * t1, size_t h1, Node * pivot,
                           Node * t2, size_t h2) noexcept
  {
    FixedStack<Node**> stack(Node::MaxHeight);
    Node ** pp = &t1;
    size_t curr_h = h1;

    // Descend right spine until we find a subtree of appropriate height
    while (curr_h > h2 + 1 and *pp != Node::NullPtr)
      {
        stack.push(pp);
        if (DIFF(*pp) >= 0)
          curr_h--;  // right child has height curr_h - 1
        else
          curr_h -= 2;  // right child has height curr_h - 2 (since left is curr_h - 1)

        pp = &RLINK(*pp);
      }

    // Now *pp points to a subtree of height <= h2 + 1
    // Join pivot with *pp and t2
    LLINK(pivot) = *pp;
    RLINK(pivot) = t2;
    const size_t left_h = (*pp != Node::NullPtr) ? avl_height(*pp) : 0;
    DIFF(pivot) = static_cast<signed char>(h2) - static_cast<signed char>(left_h);
    COUNT(pivot) = COUNT(*pp) + COUNT(t2) + 1;
    *pp = pivot;

    // Rebalance going up
    while (not stack.is_empty())
      {
        Node ** parent_ptr = stack.pop();
        Node * parent = *parent_ptr;
        COUNT(parent) = COUNT(LLINK(parent)) + COUNT(RLINK(parent)) + 1;

        const size_t new_right_h = avl_height(RLINK(parent));
        const size_t new_left_h = avl_height(LLINK(parent));
        DIFF(parent) = static_cast<signed char>(new_right_h) -
                       static_cast<signed char>(new_left_h);

        if (DIFF(parent) == 2)
            if (Node * q = RLINK(parent); DIFF(q) >= 0)
              *parent_ptr = rotateLeft(parent);
            else
              *parent_ptr = doubleRotateLeft(parent);
      }

    return t1;
  }

  // Join when h2 > h1 + 1: descend left spine of t2
  static Node * join_left(Node * t1, const size_t h1, Node * pivot,
                          Node * t2, const size_t h2) noexcept
  {
    FixedStack<Node**> stack(Node::MaxHeight);
    Node ** pp = &t2;
    size_t curr_h = h2;

    while (curr_h > h1 + 1 and *pp != Node::NullPtr)
      {
        stack.push(pp);
        if (DIFF(*pp) <= 0)
          curr_h--;
        else
          curr_h -= 2;

        pp = &LLINK(*pp);
      }

    RLINK(pivot) = *pp;
    LLINK(pivot) = t1;
    const size_t right_h = (*pp != Node::NullPtr) ? avl_height(*pp) : 0;
    DIFF(pivot) = static_cast<signed char>(right_h) - static_cast<signed char>(h1);
    COUNT(pivot) = COUNT(t1) + COUNT(*pp) + 1;
    *pp = pivot;

    while (not stack.is_empty())
      {
        Node ** parent_ptr = stack.pop();
        Node * parent = *parent_ptr;
        COUNT(parent) = COUNT(LLINK(parent)) + COUNT(RLINK(parent)) + 1;

        const size_t new_right_h = avl_height(RLINK(parent));
        const size_t new_left_h = avl_height(LLINK(parent));
        DIFF(parent) = static_cast<signed char>(new_right_h) -
                       static_cast<signed char>(new_left_h);

        if (DIFF(parent) == -2)
          {
            if (Node * q = LLINK(parent); DIFF(q) <= 0)
              *parent_ptr = rotateRight(parent);
            else
              *parent_ptr = doubleRotateRight(parent);
          }
      }

    return t2;
  }

  // Split by key recursively
  // Returns the node containing key (or NullPtr if not found)
  // t1 gets all keys < key, t2 gets all keys > key
  Node * split_key_rec(Node * p, const Key & key,
                       Node *& t1, Node *& t2) noexcept
  {
    if (p == Node::NullPtr)
      {
        t1 = t2 = Node::NullPtr;
        return Node::NullPtr;
      }

    Node * found;
    if (cmp(key, KEY(p)))
      {
        // key < p->key, go left
        Node * left_t2;
        found = split_key_rec(LLINK(p), key, t1, left_t2);

        // p and its right subtree go to t2
        LLINK(p) = Node::NullPtr;
        const size_t left_t2_h = (left_t2 != Node::NullPtr) ? avl_height(left_t2) : 0;
        const size_t right_h = (RLINK(p) != Node::NullPtr) ? avl_height(RLINK(p)) : 0;

        Node * right_tree = RLINK(p);
        RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        t2 = join_with_pivot(left_t2, left_t2_h, p, right_tree, right_h);
      }
    else if (cmp(KEY(p), key))
      {
        // key > p->key, go right
        Node * right_t1;
        found = split_key_rec(RLINK(p), key, right_t1, t2);

        // p and its left subtree go to t1
        RLINK(p) = Node::NullPtr;
        const size_t right_t1_h = (right_t1 != Node::NullPtr) ? avl_height(right_t1) : 0;
        const size_t left_h = (LLINK(p) != Node::NullPtr) ? avl_height(LLINK(p)) : 0;

        Node * left_tree = LLINK(p);
        LLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        t1 = join_with_pivot(left_tree, left_h, p, right_t1, right_t1_h);
      }
    else
      {
        // Found the key
        t1 = LLINK(p);
        t2 = RLINK(p);
        LLINK(p) = RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;
        found = p;
      }

    return found;
  }

  // Split by key for duplicates: keys <= key go to t1, keys > key go to t2
  void split_key_dup_rec(Node * p, const Key & key,
                         Node *& t1, Node *& t2) noexcept
  {
    if (p == Node::NullPtr)
      {
        t1 = t2 = Node::NullPtr;
        return;
      }

    if (cmp(key, KEY(p)))
      {
        // key < p->key, p goes to t2
        Node * left_t2;
        split_key_dup_rec(LLINK(p), key, t1, left_t2);

        LLINK(p) = Node::NullPtr;
        const size_t left_t2_h = (left_t2 != Node::NullPtr) ? avl_height(left_t2) : 0;
        const size_t right_h = (RLINK(p) != Node::NullPtr) ? avl_height(RLINK(p)) : 0;

        Node * right_tree = RLINK(p);
        RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        t2 = join_with_pivot(left_t2, left_t2_h, p, right_tree, right_h);
      }
    else
      {
        // key >= p->key, p goes to t1
        Node * right_t1;
        split_key_dup_rec(RLINK(p), key, right_t1, t2);

        RLINK(p) = Node::NullPtr;
        const size_t right_t1_h = (right_t1 != Node::NullPtr) ? avl_height(right_t1) : 0;
        const size_t left_h = (LLINK(p) != Node::NullPtr) ? avl_height(LLINK(p)) : 0;

        Node * left_tree = LLINK(p);
        LLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        t1 = join_with_pivot(left_tree, left_h, p, right_t1, right_t1_h);
      }
  }

  // Split by position recursively
  // Nodes with position < pos go to t1, nodes with position >= pos go to t2
  void split_pos_rec(Node * p, size_t pos, Node *& t1, Node *& t2) noexcept
  {
    if (p == Node::NullPtr)
      {
        t1 = t2 = Node::NullPtr;
        return;
      }

    if (size_t left_count = COUNT(LLINK(p)); pos <= left_count)
      {
        // Split point is in left subtree or at p
        Node * left_t2;
        split_pos_rec(LLINK(p), pos, t1, left_t2);

        // p goes to t2
        LLINK(p) = Node::NullPtr;
        const size_t left_t2_h = (left_t2 != Node::NullPtr) ? avl_height(left_t2) : 0;
        const size_t right_h = (RLINK(p) != Node::NullPtr) ? avl_height(RLINK(p)) : 0;

        Node * right_tree = RLINK(p);
        RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        t2 = join_with_pivot(left_t2, left_t2_h, p, right_tree, right_h);
      }
    else
      {
        // Split point is in right subtree
        Node * right_t1;
        split_pos_rec(RLINK(p), pos - left_count - 1, right_t1, t2);

        // p goes to t1
        RLINK(p) = Node::NullPtr;
        const size_t right_t1_h = (right_t1 != Node::NullPtr) ? avl_height(right_t1) : 0;
        const size_t left_h = (LLINK(p) != Node::NullPtr) ? avl_height(LLINK(p)) : 0;

        Node * left_tree = LLINK(p);
        LLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        t1 = join_with_pivot(left_tree, left_h, p, right_t1, right_t1_h);
      }
  }

public:

  /** Join this tree exclusively with another tree.

      All keys in this tree must be less than all keys in t.
      After the operation, t becomes empty and this tree contains all nodes.

      @param[in,out] t tree to join with (will be empty after)
      @note O(m log(n+m)) time complexity where m is size of t
  */
  void join_exclusive(Gen_Avl_Tree_Rk & t) noexcept
  {
    if (t.root == Node::NullPtr)
      return;

    if (root == Node::NullPtr)
      {
        root = t.root;
        t.root = Node::NullPtr;
        return;
      }

    // Collect nodes from t in preorder and insert them one by one
    // This is O(m log(n+m)) but guarantees correctness
    FixedStack<Node*> stack(Node::MaxHeight);
    stack.push(t.root);
    t.root = Node::NullPtr;

    while (not stack.is_empty())
      {
        Node * p = stack.pop();
        Node * left = LLINK(p);
        Node * right = RLINK(p);

        // Reset node and insert
        LLINK(p) = RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;
        insert(p);

        if (right != Node::NullPtr)
          stack.push(right);
        if (left != Node::NullPtr)
          stack.push(left);
      }
  }

  /** Split tree by key.

      After the operation:
      - t1 contains all keys < key
      - t2 contains all keys > key
      - This tree becomes empty

      @param[in] key the splitting key
      @param[out] t1 tree to receive keys < key
      @param[out] t2 tree to receive keys > key
      @return pointer to node containing key, or nullptr if not found
      @note O(n) time complexity (simple iterative implementation)
  */
  Node * split_key(const Key & key, Gen_Avl_Tree_Rk & t1,
                   Gen_Avl_Tree_Rk & t2) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    // Simple O(n) implementation: iterate through tree and partition
    Node * found = nullptr;
    FixedStack<Node*> stack(Node::MaxHeight);
    stack.push(root);
    root = Node::NullPtr;

    while (not stack.is_empty())
      {
        Node * p = stack.pop();
        Node * left = LLINK(p);
        Node * right = RLINK(p);

        if (right != Node::NullPtr)
          stack.push(right);
        if (left != Node::NullPtr)
          stack.push(left);

        LLINK(p) = RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        if (cmp(KEY(p), key))
          t1.insert(p);
        else if (cmp(key, KEY(p)))
          t2.insert(p);
        else
          found = p;  // Found the key
      }

    return found;
  }

  /** Split tree by key including duplicates.

      After the operation:
      - t1 contains all keys <= key
      - t2 contains all keys > key
      - This tree becomes empty

      @param[in] key the splitting key
      @param[out] t1 tree to receive keys <= key
      @param[out] t2 tree to receive keys > key
      @note O(n) time complexity (simple iterative implementation)
  */
  void split_key_dup(const Key & key, Gen_Avl_Tree_Rk & t1,
                     Gen_Avl_Tree_Rk & t2) noexcept
  {
    if (root == Node::NullPtr)
      return;

    FixedStack<Node*> stack(Node::MaxHeight);
    stack.push(root);
    root = Node::NullPtr;

    while (not stack.is_empty())
      {
        Node * p = stack.pop();
        Node * left = LLINK(p);
        Node * right = RLINK(p);

        if (right != Node::NullPtr)
          stack.push(right);
        if (left != Node::NullPtr)
          stack.push(left);

        LLINK(p) = RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        DIFF(p) = 0;

        if (cmp(key, KEY(p)))
          t2.insert(p);
        else
          t1.insert_dup(p);
      }
  }

  /** Split tree by inorder position.

      After the operation:
      - t1 contains nodes at positions [0, pos)
      - t2 contains nodes at positions [pos, size)
      - This tree becomes empty

      @param[in] pos the splitting position (0-based)
      @param[out] t1 tree to receive positions [0, pos)
      @param[out] t2 tree to receive positions [pos, size)
      @note O(n) time complexity (simple iterative implementation)
  */
  void split_pos(const size_t pos, Gen_Avl_Tree_Rk & t1,
                 Gen_Avl_Tree_Rk & t2) noexcept
  {
    if (root == Node::NullPtr)
      return;

    if (pos == 0)
      {
        t2.root = root;
        root = Node::NullPtr;
        return;
      }

    if (pos >= size())
      {
        t1.root = root;
        root = Node::NullPtr;
        return;
      }

    // Simple O(n) implementation using inorder traversal
    size_t count = 0;
    FixedStack<Node*> stack(Node::MaxHeight);
    Node * curr = root;
    root = Node::NullPtr;

    // Inorder traversal
    while (curr != Node::NullPtr or not stack.is_empty())
      {
        while (curr != Node::NullPtr)
          {
            stack.push(curr);
            Node * left = LLINK(curr);
            LLINK(curr) = Node::NullPtr;  // Disconnect
            curr = left;
          }

        curr = stack.pop();
        Node * right = RLINK(curr);
        RLINK(curr) = Node::NullPtr;
        COUNT(curr) = 1;
        DIFF(curr) = 0;

        if (count < pos)
          t1.insert(curr);
        else
          t2.insert(curr);

        ++count;
        curr = right;
      }
  }

  /** Iterator on nodes of the tree.

      The visit order is inorder.

      @ingroup Arboles
  */
  class Iterator : public BinTreeXt_Iterator<Gen_Avl_Tree_Rk, Node, Key, Compare>
  {
    using Base = BinTreeXt_Iterator<Gen_Avl_Tree_Rk, Node, Key, Compare>;

  public:

    using Base::Base;
    using Base::operator=;
  }; // end class Iterator
};


/** AVL binary search tree with nodes without virtual destructor and
    with subtree counters for select/position operations.

    @param Key the key type stored in the tree nodes.
    @param Compare key comparison class; by default, this is the
    less-than relational operator for type Key.
    @see Avl_Tree_Rk_Vtl
    @ingroup Arboles
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Avl_Tree_Rk : public Gen_Avl_Tree_Rk<AvlNodeRk, Key, Compare>
{
  using Base = Gen_Avl_Tree_Rk<AvlNodeRk, Key, Compare>;
  using Base::Base;
};


/** AVL binary search tree with virtual destructor in its nodes and
    with subtree counters for select/position operations.

    @param Key the key type stored in the tree nodes.
    @param Compare key comparison class; by default, this is the
    less-than relational operator for type Key.
    @see Avl_Tree_Rk
    @ingroup Arboles
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Avl_Tree_Rk_Vtl : public Gen_Avl_Tree_Rk<AvlNodeRkVtl, Key, Compare>
{
  using Base = Gen_Avl_Tree_Rk<AvlNodeRkVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph

# endif // TPL_AVLRK_H