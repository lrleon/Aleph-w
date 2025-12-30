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

# ifndef TPL_AVL_H
# define TPL_AVL_H

# include <algorithm>
# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <avlNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace Aleph {

   /** AVL balanced binary search tree.

       An AVL tree is a binary search tree deterministically
       balanced. The maximum height of AVL tree of \f$n\f$ nodes is
       \f$ 1.4404 \lg{(n + 2)} \ - \ 0.3277\f$. So this number bounds
       the maximum number of visited nodes in a unsuccessful search,
       what bounds the traditional operations (insertion, search and
       removal) to \f$O(\lg n)\$f complexity.

       @see Avl_Tree Avl_Tree_Vtl
       @ingroup Arboles
    */
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Avl_Tree
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

  void clean_avl_stack() noexcept { avl_stack.popn (avl_stack.size() - 1); }

  Node * search_and_stack_avl(const Key &  key) noexcept
  {
    assert(avl_stack_empty());

    Node * p = root;
    do // descend searching for key and push the search path
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p))) // key < KEY(p)?
          p = LLINK(p);
        else if (cmp(KEY(p), key)) // key > KEY(p)?
          p = RLINK(p);
        else
          return p; // duplicated key
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  Node * search_dup_and_stack_avl(const Key &  key) noexcept
  {
    assert(avl_stack_empty());

    Node * p = root;
    do // descend searching for key and push the search path
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p))) // key < KEY(p)?
          p = LLINK(p);
        else // key >= KEY(p)
          p = RLINK(p);
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  static Node * rotateLeft(Node * p) noexcept
  {
    assert(DIFF(p) == 2);
    assert(RLINK(p) != Node::NullPtr);

    Node * q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;

    if (DIFF(q) == 0)      // balance factors adjustment
      { // this case happens during deletion
        DIFF(q) = -1;
        DIFF(p) = 1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  static Node * rotateRight(Node * p) noexcept
  {
    assert(DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr);

    Node * q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;

    if (DIFF(q) == 0)       // balance factors adjustment
      { // this case happens during deletion
        DIFF(q) = 1;
        DIFF(p) = -1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

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

    unsigned char b; // logical height of r's left child
    unsigned char c; // logical height of r's right child

        // Determine logical heights of p, q and r
    if (DIFF(r) == 1) // ==> c > b ==> c-b == 1
      {
        c = 1;
        b = 0;
      }
    else if (DIFF(r) == -1) // ==> c < b ==> c-b = -1
      {
        c = 0;
        b = 1;
      }
    else
      c = b = 1;

        // balance factors adjustment
    DIFF(r) = 0;
    DIFF(p) = b - 1; // logical height of p's left child is 1
    DIFF(q) = 1 - c; // logical height of q's right child is 1

    return r;
  }

  static Node * doubleRotateRight(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr and RLINK(LLINK(p)) != Node::NullPtr);

    Node * q  = LLINK(p);
    Node * r  = RLINK(q);
    LLINK(p) = RLINK(r);
    RLINK(q) = LLINK(r);
    RLINK(r) = p;
    LLINK(r) = q;

    unsigned char b; // logical height of r's left child
    unsigned char c; // logical height of r's right child

        // determine logical heights of children of p, q and r
    if (DIFF(r) == 1) // ==> c > b ==> c-b == 1
      {
        c = 1;
        b = 0;
      }
    else if (DIFF(r) == -1) // ==> c < b ==> c-b == -1
      {
        c = 0;
        b = 1;
      }
    else
      c = b = 1;

        // balance factors adjustment
    DIFF(r) = 0;
    DIFF(p) = 1 - c; // logical height of p's right child is 1
    DIFF(q) = b - 1; // logical height of p's left child is 1

    return r;
  }

  enum Rotation_Type
    { ROTATE_LEFT, ROTATE_RIGHT, DOUBLE_ROTATE_LEFT, DOUBLE_ROTATE_RIGHT };

  static Rotation_Type rotation_type(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);

    Node * pc; // saves p's child
    if (DIFF(p) == 2) // to the left
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
    Node * pp  = avl_stack.pop(); // parent of the inserted node
    if (LLINK(pp) == p) // adjust parent's balance factor
      --DIFF(pp);
    else
      ++DIFF(pp);

    if (DIFF(pp) == 0)
      {    // in this case, the height of pp's ancestor does not increase
        clean_avl_stack();
        return;
      }

    if (avl_stack_empty())
      return; // pp is the root
    do     // search a node whose balance factor becomes 0
      {
        Node *gpp = avl_stack.pop(); // parent of pp
            // update balance factors
        if (LLINK(gpp) == pp) // AH_ERROR if (Compare () (key, KEY(gpp)))
          --DIFF(gpp);
        else
          ++DIFF(gpp);

        if (DIFF(gpp) == 0)
          break; // no rebalancing is needed
        if (DIFF(gpp) == -2 or DIFF(gpp) == 2)// AVL violation?
          {      // yes ==> rebalancing is required
            Node *ggpp = avl_stack.pop();
            restore_avl(gpp, ggpp);
            break;
          }

        pp = gpp; // AH_ERROR; add
      }
    while (not avl_stack_empty());

    clean_avl_stack();
  }

  Node * swapWithSuccessor(Node * p, Node *& pp) noexcept
  {   // Reference to the stack top, since p will be swapped with its
      // successor and the successor will take p's position in the stack
    Node *& ref_to_stack_top = avl_stack.top();

    Node *fSucc = p;        // successor's parent
    Node *succ  = RLINK(p); // search starts from RLINK(p)

    avl_stack.push(succ);

        // find the successor while updating the stack
    while (LLINK(succ) != Node::NullPtr) // descend as far left as possible
      {
        fSucc = succ;
        succ  = LLINK(succ);
        avl_stack.push(succ);
      }

      // update old stack entry occupied by p: it is equivalent to swapping
      // the old top (before searching succ) with the current one
    ref_to_stack_top = succ;
    avl_stack.top()  = p;
    if (LLINK(pp) == p) // update pp's new child (successor)
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p); // swap left subtrees
    LLINK(p)    = Node::NullPtr;
    if (RLINK(p) == succ) // update right subtrees
      { // successor is exactly p's right child
        RLINK(p)    = RLINK(succ);
        RLINK(succ) = p;
        pp          = succ;
      }
    else
      { // successor is the leftmost descendant of RLINK(p)
        Node *succr  = RLINK(succ);
        RLINK(succ)  = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p)     = succr;
        pp           = fSucc;
      }

    DIFF(succ) = DIFF(p); // swap balance factors

    return succ;
  }

  void restore_avl_after_deletion(bool left_deficit) noexcept
  {
    Node * pp  = avl_stack.top(1);  // parent of p
    Node * ppp = avl_stack.popn(3); // remove from stack p, parent and grandparent
    while (true)
      {    // update balance factors
        if (left_deficit) // AH_ERROR Compare () (key, KEY(pp)))
          ++DIFF(pp);
        else
          --DIFF(pp);

        if (DIFF(pp) == -2 or DIFF(pp) == 2) // still valid?
          pp = restore_avl(pp, ppp); // no

        if (DIFF(pp) != 0 or pp == root)
          break; // global tree height has not changed ==> stop

        left_deficit = LLINK(ppp) == pp;
        pp  = ppp; // advance to next ancestor
        ppp = avl_stack.pop();
      }

      clean_avl_stack();
    }

public:

  using key_type = Key; /// The key type

      /// Return a reference to the comparison criteria
  [[nodiscard]] constexpr Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  [[nodiscard]] constexpr Compare & get_compare() noexcept { return key_comp(); }

  Gen_Avl_Tree(Compare __cmp = Compare()) noexcept
    : avl_stack(Node::MaxHeight), head_ptr(&head_node),
      root(RLINK (head_ptr)), cmp(__cmp)
  {
    avl_stack.push(head_ptr);
  }

  /** Swap in constant time all the items of `this` with the items of
      `tree`.

      @param[in] tree the tree to swap with this
  */
  void swap(Gen_Avl_Tree & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  virtual ~Gen_Avl_Tree() noexcept { assert(avl_stack_empty()); }

      /// Return a modifiable reference to tree's root
  [[nodiscard]] constexpr Node *& getRoot() noexcept { return root; }

      /// Return a modifiable reference to tree's root
  [[nodiscard]] constexpr Node * getRoot() const noexcept { return root; }

     /// Search a node containing `key`; if found, then a pointer to the
     /// node containing it is returned; otherwise `nullptr` is returned
  [[nodiscard]] Node * search(const Key & key) const noexcept
  {
    return searchInBinTree <Node, Compare> (root, key, cmp);
  }

  /** Insert the node pointed by `p` in the tree.

      `insert(p)' inserts the node `p` inte AVL tree. If `KEY(p)` is
      found in the tree, then no insertion is done and `nullptr` is
      returned. Otherwise, `p` is inserted and this same value is
      returned.

      @param[in] p the node to be inserted
      @return a the pointer `p` if `KEY(p)` is not found in the tree,
      `nullptr` otherwise.
  */
  [[nodiscard]] Node * insert(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (Node *pp = search_and_stack_avl(KEY(p)); cmp (KEY(p), KEY(pp)))
      LLINK (pp) = p;
    else if (cmp (KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      { // duplicated key
        clean_avl_stack();
        return nullptr;
      }

    restore_avl_after_insertion(p);

    return p;
  }

      /** Search or insert a key.

          `search_or_insert(p)` searches in the tree a node containing
          `KEY(p)`. If this is found, then a pointer to the node in the
          tree is returned. Otherwise, `p` is inserted and its value
          returned,

          This method is very useful when it is required to search and
          eventually to insert,

          @param[in] p the node whose key must be searched and
          eventually inserted,
          @return if `p` is inserted, the this value is returned;
          otherwise, a pointer to the node containing `KEY(p)` is returned.
       */
  [[nodiscard]] Node * search_or_insert(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (Node * pp = search_and_stack_avl(KEY(p)); cmp(KEY(p), KEY(pp)))
      LLINK(pp) = p;
    else if (cmp(KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      { // duplicated key
        clean_avl_stack();
        return pp;
      }

    restore_avl_after_insertion(p);

    return p;
  }

      /// Insert the node `p` without testing for key duplicity
  [[nodiscard]] Node * insert_dup(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (Node *pp = search_dup_and_stack_avl(KEY(p)); cmp (KEY(p), KEY(pp)))
      LLINK (pp) = p;
    else
      RLINK(pp) = p;

    restore_avl_after_insertion(p);

    return p;
  }

      /// Remove from an AVL tree the node containing key `key`.
      /// Returns the removed node if the key is found; `nullptr` otherwise.
  [[nodiscard]] Node * remove(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    Node * p = search_and_stack_avl(key);
    if (no_equals<Key, Compare> (KEY(p), key, cmp))
      {     // key was not found
        clean_avl_stack();
        return nullptr;
      }

    Node * pp = avl_stack.top(1); // get parent of p
    bool left_deficit; // AH_ERROR Key removed_key = KEY(p);
    while (true)
      {
        left_deficit = LLINK(pp) == p;
        if (LLINK(p) == Node::NullPtr) // missing left child?
          {      // yes: link pp to p's child
            if (LLINK(pp) == p)
              LLINK(pp) = RLINK(p);
            else
              RLINK(pp) = RLINK(p);
            break;
          }

        if (RLINK(p) == Node::NullPtr) // missing right child?
          {     // yes: link pp to p's child
            if (LLINK(pp) == p)
              LLINK(pp) = LLINK(p);
            else
              RLINK(pp) = LLINK(p);
            break;
          }

            // here p is a full node ==> swap with successor
        swapWithSuccessor(p, pp);
        //    removed_key = KEY(succ); // AH_ERROR remove
      }

     p->reset();

    if (pp == head_ptr) // check if the root was removed
      { // balance factors unchanged ==> AVL condition is not violated
        clean_avl_stack();
        return p;
      }

    restore_avl_after_deletion(left_deficit);

    return p;
  }

  [[nodiscard]] bool verify() const noexcept
  {
    return is_avl(root);
  }

  /** Iterator over the nodes.

      This class allows traversing the tree nodes in sorted order
      according to the comparison criterion specified at tree
      instantiation.

     @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(Gen_Avl_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
    Iterator(const Gen_Avl_Tree & tree)
      : BinNodeInfixIterator<Node>(tree.getRoot()) {}
  };
};


   /** AVL binary search tree with nodes without virtual destructor.

       An AVL binary search tree is a binary search tree whose height is
       bounded by \f$O(\lg n)\f$ and whose update operations are bounded
       in time by inspecting \f$O(\lg n)\f$ nodes.

       This class uses nodes without a virtual destructor.

       @param Key the key type stored in the tree nodes.
       @param Compare key comparison class; by default, this is the
       less-than relational operator for type Key.
       @see Avl_Tree_Vtl
       @ingroup Arboles
    */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Avl_Tree : public Gen_Avl_Tree<AvlNode, Key, Compare>
{
  using Base = Gen_Avl_Tree<AvlNode, Key, Compare>;
  using Base::Base;
};


   /** AVL binary search tree with virtual destructor in its nodes.

       An AVL binary search tree is a binary search tree whose height is
       bounded by \f$O(\lg n)\f$ and whose update operations are bounded
       in time by inspecting \f$O(\lg n)\f$ nodes.

       This class uses nodes with a virtual destructor.

       @param Key the key type stored in the tree nodes.
       @param Compare key comparison class; by default, this is the
       less-than relational operator for type Key.
       @see Avl_Tree
       @ingroup Arboles
    */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Avl_Tree_Vtl : public Gen_Avl_Tree<AvlNodeVtl, Key, Compare>
{
  using Base = Gen_Avl_Tree<AvlNodeVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph
# endif // TPL_AVL_H

