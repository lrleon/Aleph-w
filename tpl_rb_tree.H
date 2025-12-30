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

# ifndef TPL_RB_TREE_H
# define TPL_RB_TREE_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeUtils.H>
# include <rbNode.H>

using namespace Aleph;

namespace Aleph
{
  /** @brief Red-black binary search tree implementation.

      A red-black tree is a self-balancing binary search tree where each node
      has a color (red or black). The tree maintains balance through five
      invariants that guarantee the height is at most 2*log₂(n+1), ensuring
      O(log n) worst-case time for all operations.

      @par Red-Black Tree Properties:
      1. Every node is either red or black
      2. The root is black
      3. All leaves (null nodes) are black
      4. Red nodes have only black children
      5. Every path from root to leaf has the same number of black nodes

      These properties ensure the tree remains approximately balanced, with
      the longest path at most twice the length of the shortest path.

      This implementation is generic over the node type (with or without
      virtual destructor). For typical usage, prefer the Rb_Tree typedef.

      @tparam NodeType Node template (typically RbNode or RbNodeVtl).
      @tparam Key The type of keys stored in the tree.
      @tparam Compare Comparison functor for ordering keys.

      @par Complexity:
      - Search: O(log n) worst case
      - Insert: O(log n) worst case
      - Delete: O(log n) worst case
      - Space: O(n) + 1 bit per node for color

      @par Example:
      @code
      Rb_Tree<int> tree;
      tree.insert(new Rb_Tree<int>::Node(42));
      tree.insert(new Rb_Tree<int>::Node(17));
      tree.insert(new Rb_Tree<int>::Node(99));

      auto node = tree.search(42);
      if (node != nullptr)
        std::cout << "Found: " << node->get_key() << std::endl;

      tree.remove(42);
      @endcode

      @note This is a low-level implementation managing raw nodes.
            For automatic memory management, use DynSetRbTree.

      @see Rb_Tree Convenient typedef with default node type.
      @see DynSetRbTree High-level wrapper with automatic memory management.
      @see Avl_Tree Alternative balanced tree (stricter balance, more rotations).
      @ingroup Arboles
   */
  template <template <typename> class NodeType, typename Key, class Compare>
  class Gen_Rb_Tree
  {
  public:
    typedef NodeType<Key> Node;

  private:
    Node head_node; // sentinel header
    Node *head; // pointer to sentinel
    Node *& root;
    FixedStack<Node *> rb_stack;
    Compare cmp;

    Node * search_and_stack_rb(const Key & key)
    {
      Node *p = root;
      rb_stack.push(head);
      do
        {
          rb_stack.push(p);
          if (cmp(key, KEY(p)))
            p = LLINK(p);
          else if (cmp(KEY(p), key))
            p = RLINK(p);
          else
            return p;
        }
      while (p != Node::NullPtr);

      return rb_stack.top();
    }

    Node * search_dup_and_stack_rb(const Key & key)
    {
      Node *p = root;
      rb_stack.push(head);
      do
        {
          rb_stack.push(p);
          if (cmp(key, KEY(p)))
            p = LLINK(p);
          else
            p = RLINK(p);
        }
      while (p != Node::NullPtr);

      return rb_stack.top();
    }

    void fix_red_condition(Node *p)
    {
      assert(COLOR(p) == RED);

      while (p != root)
        {
          Node *pp = rb_stack.pop(); // parent of p
          if (COLOR(pp) == BLACK) // is p's parent black?
            break; // yes ==> no consecutive reds ==> done

          if (root == pp) // is p a direct child of the root?
            { // yes ==> color root black and finish
              COLOR(root) = BLACK;
              break;
            }

          Node *ppp = rb_stack.pop(); // grandparent of p
          if (Node *spp = LLINK(ppp) == pp ? RLINK(ppp) : LLINK(ppp); COLOR(spp) == RED) // is p's uncle red?
            { // swap colors between levels
              COLOR(ppp) = RED;
              COLOR(pp) = BLACK;
              COLOR(spp) = BLACK;
              p = ppp;
              continue; // go to next ancestor, check violations
            }

          Node *pppp = rb_stack.pop(); // great-grandparent of p
          if (LLINK(pp) == p and LLINK(ppp) == pp)
            {
              rotate_to_right(ppp, pppp);
              COLOR(pp) = BLACK;
            }
          else if (RLINK(pp) == p and RLINK(ppp) == pp)
            {
              rotate_to_left(ppp, pppp);
              COLOR(pp) = BLACK;
            }
          else
            {
              if (RLINK(pp) == p)
                {
                  rotate_to_left(pp, ppp);
                  rotate_to_right(ppp, pppp);
                }
              else
                {
                  rotate_to_right(pp, ppp);
                  rotate_to_left(ppp, pppp);
                }
              COLOR(p) = BLACK;
            }

          COLOR(ppp) = RED;
          break; // tree is red-black ==> done
        }

      rb_stack.empty();
    }

    void find_succ_and_swap(Node *p, Node *& pp)
    {
      Node *& ref_rb_stack = rb_stack.top();

      /* Find successor while updating rb_stack */
      Node *fSucc = p; // successor's parent
      Node *succ = RLINK(p); // Searching starts from p's right child
      rb_stack.push(succ);

      while (LLINK(succ) != Node::NullPtr) // go down to leftmost
        {
          fSucc = succ;
          succ = LLINK(succ);
          rb_stack.push(succ);
        }

      ref_rb_stack = succ; /* swap old top with current top */
      rb_stack.top() = p;

      if (LLINK(pp) == p) /* Setting of parent of p to new child(succ) */
        LLINK(pp) = succ;
      else
        RLINK(pp) = succ;

      LLINK(succ) = LLINK(p); /* Swaps left branches */
      LLINK(p) = Node::NullPtr;

      if (RLINK(p) == succ) /* For right branches there are two cases */
        { /* successor is just right child of p */
          RLINK(p) = RLINK(succ);
          RLINK(succ) = p;
          pp = succ;
        }
      else
        { /* Successor is leftmost node descending from right child of p */
          Node *succr = RLINK(succ);
          RLINK(succ) = RLINK(p);
          LLINK(fSucc) = p;
          RLINK(p) = succr;
          pp = fSucc;
        }

      std::swap(COLOR(succ), COLOR(p));
    }

    void fix_black_condition(Node *p)
    {
      if (COLOR(p) == RED) // is p red?
        { // yes ==> paint it black and finish
          COLOR(p) = BLACK; // this compensates the deficit
          return;
        }

      Node *pp = rb_stack.popn(2); // parent of p
      while (p != root)
        {
          assert(LLINK(pp) == p or RLINK(pp) == p);
          assert(LLINK(rb_stack.top()) == pp or RLINK(rb_stack.top()) == pp);

          Node *sp = LLINK(pp) == p ? RLINK(pp) : LLINK(pp); // sibling of p
          if (COLOR(sp) == RED) // is p's sibling red?
            {
              Node *& ppp = rb_stack.top(); // grandparent of p

              if (LLINK(pp) == p)
                {
                  sp = LLINK(sp);
                  ppp = rotate_to_left(pp, ppp);
                }
              else
                {
                  sp = RLINK(sp);
                  ppp = rotate_to_right(pp, ppp);
                }

              COLOR(ppp) = BLACK;
              COLOR(pp) = RED;
            }

          Node *np, *snp; // nephews of node p
          if (LLINK(pp) == p) // is p a left child?
            { // yes ==> sp is the right child
              np = RLINK(sp);
              snp = LLINK(sp);
            }
          else
            {
              np = LLINK(sp);
              snp = RLINK(sp);
            }

          if (COLOR(np) == RED) // is np red?
            {
              Node *ppp = rb_stack.top();

              if (RLINK(sp) == np)
                rotate_to_left(pp, ppp);
              else
                rotate_to_right(pp, ppp);

              COLOR(sp) = COLOR(pp);
              COLOR(pp) = BLACK;
              COLOR(np) = BLACK;

              return;
            }

          if (COLOR(snp) == RED) // is snp red?
            {
              Node *ppp = rb_stack.top();

              if (LLINK(sp) == snp)
                {
                  rotate_to_right(sp, pp);
                  rotate_to_left(pp, ppp);
                }
              else
                {
                  rotate_to_left(sp, pp);
                  rotate_to_right(pp, ppp);
                }

              COLOR(snp) = COLOR(pp);
              COLOR(pp) = BLACK;

              return;
            }

          if (COLOR(pp) == RED) // is pp red?
            {
              COLOR(pp) = BLACK;
              COLOR(sp) = RED;
              return;
            }

          // no red node in the adjacency of p ==> shift the deficit
          // towards pp and repeat the iteration
          COLOR(sp) = RED;
          p = pp;
          pp = rb_stack.pop();
        }
    }

  public:
    /// The key type stored in the node
    typedef Key key_type;

    /// Returns a reference to the comparison criteria
    Compare &key_comp() noexcept { return cmp; }

    /// \overload key_comp()
    Compare &get_compare() noexcept { return key_comp(); }

    /// Instantiates a red-black tree.
    Gen_Rb_Tree(Compare __cmp = Compare())
      : head(&head_node), root(head_node.getR()),
        rb_stack(Node::MaxHeight), cmp(__cmp)
    {
      /* empty */
    }

    /** Swaps all elements of this Rb_Tree with tree in constant time
        (and extremely fast).

        @param[in] tree the red-black tree to swap with this
    */
    void swap(Gen_Rb_Tree & tree) noexcept
    {
      std::swap(root, tree.root);
      std::swap(cmp, tree.cmp);
    }

    /// Move constructor.
    Gen_Rb_Tree(Gen_Rb_Tree && tree) noexcept
      : head(&head_node), root(head_node.getR()),
        rb_stack(Node::MaxHeight), cmp(std::move(tree.cmp))
    {
      root = tree.root;
      tree.root = Node::NullPtr;
    }

    /// Move assignment operator.
    Gen_Rb_Tree &operator=(Gen_Rb_Tree && tree) noexcept
    {
      if (this != &tree)
        {
          root = tree.root;
          tree.root = Node::NullPtr;
          cmp = std::move(tree.cmp);
        }
      return *this;
    }

    /// Destroys a red-black tree.
    virtual ~Gen_Rb_Tree() = default;

    /// Searches for a node with key. Returns a pointer to the node
    /// containing key if it is found in the tree; nullptr otherwise.
    Node * search(const Key & key)
    {
      Node *retVal = Aleph::searchInBinTree<Node, Compare>(root, key, cmp);
      return retVal == Node::NullPtr ? nullptr : retVal;
    }

    /// Obtains a pointer to the root of the tree.
    Node *&getRoot() noexcept { return root; }

    /// Returns true if the tree is empty.
    bool is_empty() const noexcept { return root == Node::NullPtr; }

    /// Returns the number of nodes in the tree.
    size_t size() const noexcept { return Aleph::size(root); }

    /// Inserts node p into the red-black tree. If p's key is not contained
    /// in the tree, then the node is inserted and a pointer to p is
    /// returned. Otherwise insertion does not occur and nullptr is returned.
    Node * insert(Node *p)
    {
      assert(p != nullptr and p != Node::NullPtr);
      assert(COLOR(p) == RED);

      if (root == Node::NullPtr)
        return root = p; // insertion into empty tree

      Node *q = search_and_stack_rb(KEY(p));
      if (cmp(KEY(p), KEY(q)))
        LLINK(q) = p;
      else if (cmp(KEY(q), KEY(p)))
        RLINK(q) = p;
      else
        {
          rb_stack.empty();
          return nullptr; // duplicate key
        }
      fix_red_condition(p);

      return p;
    }

    /** Searches for p's key in the red-black tree or inserts it
        if it is not found.

        search_or_insert(p) searches in the red-black tree for a node whose
        key is <code>KEY(p)</code>. If the key is found, then it returns a
        pointer to the node that stores it. Otherwise p is inserted into
        this binary search tree.

        @param[in] p node to search or insert.
        @return pointer to the inserted node if p's key is not contained
        within the tree; otherwise, a pointer to the node in the tree that
        contains <code>KEY(p)</code>.
    */
    Node * search_or_insert(Node *p)
    {
      assert(p != nullptr and p != Node::NullPtr);
      assert(COLOR(p) == RED);

      if (root == Node::NullPtr)
        return root = p; // insertion into empty tree

      Node *q = search_and_stack_rb(KEY(p));
      if (cmp(KEY(p), KEY(q)))
        LLINK(q) = p;
      else if (cmp(KEY(q), KEY(p)))
        RLINK(q) = p;
      else
        {
          rb_stack.empty();
          return q; // duplicate key
        }
      fix_red_condition(p);

      return p;
    }

    /// Inserts node p into the red-black tree. If p's key is not contained
    /// in the tree, then the node is inserted and a pointer to p is
    /// returned. Otherwise insertion does not occur and nullptr is returned.
    Node * insert_dup(Node *p)
    {
      assert(p != nullptr and p != Node::NullPtr);
      assert(COLOR(p) == RED);

      if (root == Node::NullPtr)
        return root = p; // insertion into empty tree

      Node *q = search_dup_and_stack_rb(KEY(p));
      if (cmp(KEY(p), KEY(q)))
        LLINK(q) = p;
      else
        RLINK(q) = p;

      fix_red_condition(p);

      return p;
    }

    bool verify() const { return is_red_black_bst(root, cmp); }

    /// Removes the node containing key. If the key is found in the tree,
    /// then the node that contains it is removed and its address is
    /// returned. Otherwise nullptr is returned.
    Node * remove(const Key & key)
    {
      if (root == Node::NullPtr)
        return nullptr;

      Node *q = search_and_stack_rb(key);
      if (no_equals<Key, Compare>(KEY(q), key, cmp)) // key not found?
        {
          rb_stack.empty();
          return nullptr;
        }

      Node *pq = rb_stack.top(1); // parent of q
      Node *p; // child of q after q has been removed
      while (true) // classic binary search tree removal
        {
          if (LLINK(q) == Node::NullPtr)
            {
              if (LLINK(pq) == q)
                p = LLINK(pq) = RLINK(q);
              else
                p = RLINK(pq) = RLINK(q);

              break; // goto end;
            }

          if (RLINK(q) == Node::NullPtr)
            {
              if (LLINK(pq) == q)
                p = LLINK(pq) = LLINK(q);
              else
                p = RLINK(pq) = LLINK(q);

              break; // goto end;
            }

          find_succ_and_swap(q, pq);
        }

      if (COLOR(q) == BLACK) // was a black node removed?
        fix_black_condition(p);

      q->reset();
      rb_stack.empty();

      return q;
    }

    /** Iterator over the nodes.

        This class allows traversing the tree nodes in sorted order
        according to the comparison criteria specified when the tree
        is instantiated.

       @ingroup Arboles
    */
    struct Iterator : public BinNodeInfixIterator<Node>
    {
      Iterator(Gen_Rb_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
    };
  };

  /** Red-black binary search tree with nodes without virtual destructor.

     A red-black binary search tree is a binary search tree whose height is
     bounded by \f$O(\lg n)\f$ and whose update operations are bounded in time
     by inspecting \f$O(\lg n)\f$ nodes.

     This class handles nodes without virtual destructor.

     @param Key key type stored in the tree nodes.
     @param Compare comparison functor between keys.
     @see Rb_Tree_Vtl
     @ingroup Trees
     @ingroup Arboles
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  struct Rb_Tree : public Gen_Rb_Tree<RbNode, Key, Compare>
  {
    using Base = Gen_Rb_Tree<RbNode, Key, Compare>;
    using Base::Base;
  };

  /** Red-black binary search tree with virtual destructor in its nodes.

     A red-black binary search tree is a binary search tree whose height is
     bounded by \f$O(\lg n)\f$ and whose update operations are bounded in time
     by inspecting \f$O(\lg n)\f$ nodes.

     This class handles nodes with virtual destructor.

     @param Key key type stored in the tree nodes.
     @param Compare comparison functor between keys.
     @see Rb_Tree_Vtl
     @ingroup Arboles
  */
  template <typename Key, class Compare = Aleph::less<Key>>
  struct Rb_Tree_Vtl : public Gen_Rb_Tree<RbNodeVtl, Key, Compare>
  {
    using Base = Gen_Rb_Tree<RbNodeVtl, Key, Compare>;
    using Base::Base;
  };
} // end namespace Aleph

# endif // TPL_RB_TREE_H
