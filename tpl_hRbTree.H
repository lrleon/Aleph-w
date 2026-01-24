
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


/** @file tpl_hRbTree.H
    @brief Hybrid top-down/bottom-up red-black tree implementation.
    
    This file implements a red-black tree that uses:
    - Top-down insertion with proactive color flipping (like Sedgewick's approach)
    - Bottom-up deletion with stack-based ancestor tracking
    
    This hybrid approach combines the benefits of both strategies:
    - Single-pass insertion (no backtracking)
    - Efficient deletion with stack for ancestor access
*/

# ifndef TPL_HTDRBTREE_H
# define TPL_HTDRBTREE_H

# ifdef DEBUG
# include <cmath>
# endif
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <tpl_arrayStack.H>
# include <rbNode.H>

namespace Aleph
{

/** @brief Hybrid top-down/bottom-up red-black tree.
    
    This class implements a self-balancing binary search tree using
    the red-black tree algorithm with a hybrid approach:
    
    - **Insertion**: Top-down with proactive color flipping. As we descend
      to find the insertion point, we preemptively flip colors of nodes
      with two red children. This ensures the tree remains balanced with
      at most one rotation after insertion.
    
    - **Deletion**: Bottom-up using a stack to track ancestors. After
      removing a node, we fix any violations by walking back up the path.
    
    @par Complexity:
    - All operations (insert, remove, search): O(log n)
    - Space: O(n) + O(log n) stack for deletion
    
    @tparam Key The type of keys stored in the tree
    @tparam Compare Comparison functor (default: Aleph::less<Key>)
    
    @ingroup Trees
    @author Leandro Rabindranath León
*/
  template <class Key, class Compare = Aleph::less<Key>>
  class HtdRbTree
  {
  public:
    /// Color type for red-black nodes
    using Color = unsigned char;
    
    /// The node type used by this tree
    using Node = RbNode<Key>;

  private:

  private:
    Node headNode;        ///< Auxiliary node, parent of root
    Node headParent;      ///< Auxiliary node, grandparent of root
    Node headGrandParent; ///< Auxiliary node, great-grandparent of root

    Node *head;           ///< Pointer to head node
    Node *fHead;          ///< Pointer to head's parent
    Node *ffHead;         ///< Pointer to head's grandparent
    Node *& root;         ///< Reference to root (right child of head)

    FixedStack<Node *> path{Node::MaxHeight}; ///< Stack for deletion path

    size_t node_count = 0; ///< Number of nodes in tree

    Compare cmp;          ///< Comparison functor

    /// Returns sibling of p given its parent fp.
    static Node * getSibling(Node *p, Node *fp) noexcept
    {
      assert(LLINK(fp) == p || RLINK(fp) == p);
      return LLINK(fp) == p ? RLINK(fp) : LLINK(fp);
    }

    // ===================== INSERTION ROUTINES =====================

    /** @brief Restore red-black condition after insertion.
        
        Called when we have two consecutive red nodes (p and fp are both red).
        Performs rotations to fix the violation.
        
        @param p Violating node (red)
        @param fp Parent of p (red) - may be updated
        @param ffp Grandparent of p (black)
        @param fffp Great-grandparent of p
    */
    void restoreRedCondition(Node *p,
                             Node *& fp,
                             Node *ffp,
                             Node *fffp)
    {
      /* Sanity checks */
      assert(LLINK(fp) == p || RLINK(fp) == p);
      assert(COLOR(fp) == RED);
      assert(COLOR(p) == RED);

      if (fp == root)
        {
          COLOR(fp) = BLACK;
          return;
        }

      assert(LLINK(ffp) == fp || RLINK(ffp) == fp);
      assert(COLOR(ffp) == BLACK);
      assert(LLINK(fffp) == ffp || RLINK(fffp) == ffp);

      COLOR(ffp) = RED;

      if (LLINK(fp) == p && LLINK(ffp) == fp)
        {
          COLOR(fp) = BLACK;
          rotate_to_right(ffp, fffp);
        }
      else if (RLINK(fp) == p && RLINK(ffp) == fp)
        {
          COLOR(fp) = BLACK;
          rotate_to_left(ffp, fffp);
        }
      else
        {
          COLOR(p) = BLACK;
          if (RLINK(fp) == p)
            {
              rotate_to_left(fp, ffp);
              rotate_to_right(ffp, fffp);
            }
          else
            {
              rotate_to_right(fp, ffp);
              rotate_to_left(ffp, fffp);
            }
          fp = fffp;
        }
    }

    /** @brief Flip colors of a black node with two red children.
        
        The black node becomes red and its children become black.
        This is the key operation for top-down insertion.
        
        @param p Node to flip (must be black with two red children)
    */
    static void flipColors(Node *p) noexcept
    {
      assert(p != Node::NullPtr);
      assert(COLOR(p) == BLACK);
      assert(COLOR(LLINK(p)) == RED && COLOR(RLINK(p)) == RED);

      COLOR(p) = RED;
      COLOR(LLINK(p)) = COLOR(RLINK(p)) = BLACK;
    }

    /** @brief Search for insertion point with proactive color flipping.
        
        Descends the tree looking for where to insert q. During descent,
        any node with two red children has its colors flipped preemptively.
        
        @param q Node to insert
        @return Pointer to inserted node, or nullptr if duplicate
    */
    Node * searchFlipColorsAndInsert(Node *q) noexcept
    {
      assert(q != Node::NullPtr);
      assert(root != Node::NullPtr);
      assert(COLOR(q) == RED);
      assert(LLINK(q) == Node::NullPtr && RLINK(q) == Node::NullPtr);

      const Key & qk = KEY(q);

      Node *p = root; // Current node
      Node *fp = head; // p's parent
      Node *ffp = fHead; // p's grand parent
      Node *fffp = ffHead; // p's great grand parent
      Node *nextNode;

      while (true)
        {
          const Key & pk = KEY(p);

          if (are_equals<Key, Compare>(qk, pk, cmp)) [[unlikely]]
            return nullptr; /* Duplicated key, insertion is not possible */

          if (COLOR(p) == BLACK && COLOR(LLINK(p)) == RED
              && COLOR(RLINK(p)) == RED)
            {
              flipColors(p); // Rends p red
              if (COLOR(fp) == RED) // violation of red condition
                {
                  assert(fffp != Node::NullPtr);
                  restoreRedCondition(p, fp, ffp, fffp);
                }
            }

          if (cmp(qk, pk))
            {
              if (LLINK(p) == Node::NullPtr)
                break;
              nextNode = LLINK(p);
            }
          else
            {
              if (RLINK(p) == Node::NullPtr)
                break;
              nextNode = RLINK(p);
            }

          fffp = ffp;
          ffp = fp;
          fp = p;
          p = nextNode;
        }

      ++node_count;

      /* Insertion act */
      const Key & pk = KEY(p);
      if (cmp(qk, pk))
        LLINK(p) = q;

      else
        RLINK(p) = q;

      if (COLOR(p) == RED) // Violation of red condition
        restoreRedCondition(q, p, fp, ffp);

      return q;
    }

    /// Like searchFlipColorsAndInsert but allows duplicate keys.
    Node * searchFlipColorsAndInsertDup(Node *q) noexcept
    {
      assert(q != Node::NullPtr);
      assert(root != Node::NullPtr);
      assert(COLOR(q) == RED);
      assert(LLINK(q) == Node::NullPtr && RLINK(q) == Node::NullPtr);

      const Key & qk = KEY(q);

      Node *p = root; // Current node
      Node *fp = head; // p's parent
      Node *ffp = fHead; // p's grand parent
      Node *fffp = ffHead; // p's great grand parent
      Node *nextNode;

      while (true)
        {
          const Key & pk = KEY(p);

          if (COLOR(p) == BLACK && COLOR(LLINK(p)) == RED
              && COLOR(RLINK(p)) == RED)
            {
              flipColors(p); // Rends p red
              if (COLOR(fp) == RED) // violation of red condition
                {
                  assert(fffp != Node::NullPtr);
                  restoreRedCondition(p, fp, ffp, fffp);
                }
            }

          if (cmp(qk, pk))
            {
              if (LLINK(p) == Node::NullPtr)
                break;
              nextNode = LLINK(p);
            }
          else
            {
              if (RLINK(p) == Node::NullPtr)
                break;
              nextNode = RLINK(p);
            }

          fffp = ffp;
          ffp = fp;
          fp = p;
          p = nextNode;
        }

      ++node_count;

      const Key & pk = KEY(p);
      if (cmp(qk, pk))
        LLINK(p) = q;
      else
        RLINK(p) = q;

      if (COLOR(p) == RED) // Violation of red condition
        restoreRedCondition(q, p, fp, ffp);

      return q;
    }

    // ===================== DELETION ROUTINES =====================

    /** @brief Search for key while building path stack.
        
        Used for deletion. The path stack is needed to walk back up
        the tree when fixing violations.
        
        @param key Key to search for
        @return Node with key, or leaf where key would be
    */
    Node * searchAndBuildPath(const Key & key) noexcept
    {
      assert(root != Node::NullPtr);

      Node *p = root;
      path.push(head);

      while (true)
        {
          path.push(p);

          const Key & pk = KEY(p);

          if (are_equals<Key, Compare>(key, pk, cmp))
            {
#           ifdef DEBUG
              assert(path.size() - 1.0 <= 2*log(node_count + 1)/log(2));
#           endif
              return p;
            }

          if (cmp(key, pk))
            {
              if (LLINK(p) == Node::NullPtr)
                {
#               ifdef DEBUG
                  assert(path.size() - 1.0 <= 2*log(node_count + 1)/log(2));
#               endif
                  return p;
                }

              p = LLINK(p);
              continue;
            }

          if (RLINK(p) == Node::NullPtr)
            {
#           ifdef DEBUG
              assert(path.size() - 1.0 <= 2*log(node_count + 1)/log(2));
#           endif
              return p;
            }

          p = RLINK(p);
        }
    }

    /** @brief Find successor and swap with node to delete.
        
        Used during deletion when the node has two children.
        Finds the in-order successor and swaps pointers (not contents).
        
        @param p Node to delete
        @param fp Parent of p (updated to point to new position)
    */
    void findSuccAndSwap(Node *p, Node *& fp) noexcept
    {
      assert(p != Node::NullPtr);
      assert(RLINK(p) != Node::NullPtr);
      assert(fp != Node::NullPtr);
      assert(LLINK(fp) == p || RLINK(fp) == p);

      /*
        We save a reference to current stack content because p will be
        swapped and p's position in stack should be occupied by
        successor of p
      */
      Node *& refToSearchPath = path.top();

      assert(refToSearchPath == p);

      /* Find succesor while updating searchPath */
      Node *fSucc = p; // succesor's parent
      Node *succ = RLINK(p); // Searching starts from p's right child

      path.push(succ);
      while (LLINK(succ) != Node::NullPtr) // go down to leftmost
        {
          fSucc = succ;
          succ = LLINK(succ);
          path.push(succ);
        }
#   ifdef DEBUG
      assert(path.size() - 1.0 <= 2*log(node_count + 1)/log(2));
#   endif

      /*
         update old stack entry occupied by p These operations are
         equivalents to swap old top with current top
      */
      refToSearchPath = succ;
      path.top() = p;

      /* Setting of parent of p to its new child (succ) */
      if (LLINK(fp) == p)
        LLINK(fp) = succ;
      else
        RLINK(fp) = succ;

      /* Swaps left branches */
      LLINK(succ) = LLINK(p);
      LLINK(p) = Node::NullPtr;

      /* For right branches there are two cases */
      if (RLINK(p) == succ)
        { /* successor is just right's child of p */
          RLINK(p) = RLINK(succ);
          RLINK(succ) = p;
          fp = succ;
        }
      else
        { /*
             successor is the leftmost nodo descending from right child of p
          */
          Node *succr = RLINK(succ);
          RLINK(succ) = RLINK(p);
          LLINK(fSucc) = p;
          RLINK(p) = succr;
          fp = fSucc;
        }

      // Swap of colors
      Color tmp = COLOR(succ);
      COLOR(succ) = COLOR(p);
      COLOR(p) = tmp;
    }


    /** @brief Balance down a violating black node.
        
        When sibling is red, rotate to make it black.
        This ensures the sibling-black cases can be applied.
        
        @pre p is black, fp is black, sp is red
        @param p Violating node (black, missing a black node)
        @param fp Parent of p (updated)
        @param sp Sibling of p (updated to new sibling, which will be black)
    */
    void balanceDownAndColor(Node *p, Node *& fp, Node *& sp) noexcept
    {
      assert(LLINK(fp) == p || RLINK(fp) == p);
      assert(LLINK(fp) == sp || RLINK(fp) == sp);
      assert(COLOR(fp) == BLACK);
      assert(COLOR(sp) == RED);
      assert(COLOR(p) == BLACK);
      assert(!path.is_empty());

      /* needed by rotation for links' update. We save a reference to search
       stack because stack's head will cahnge after rotation */
      Node *& ffp = path.top();

      assert(LLINK(ffp) == fp || RLINK(ffp) == fp);

      /* balancing down of p, update of out parameters and update of stack
         entry corresponding to fp */
      if (LLINK(fp) == p)
        {
          sp = LLINK(sp);
          ffp = rotate_to_left(fp, ffp);
        }
      else
        {
          sp = RLINK(sp);
          ffp = rotate_to_right(fp, ffp);
        }

      assert(LLINK(fp) == sp || RLINK(fp) == sp);
      assert(COLOR(ffp) == RED);

      /* coloring  for ensuring to apply sibling black cases */
      COLOR(ffp) = BLACK;
      COLOR(fp) = RED;
    }

    /** @brief Rotate nephew up and recolor to fix violation.
        
        Case: nephew np is red and on the "outside" (same side as sibling).
        Single rotation fixes the tree.
        
        @pre sp is black, np is red
        @param fp Parent of violating node
        @param sp Sibling (black)
        @param np Nephew (red, same side as sp)
    */
    void rotateNephewAndColor(Node *fp, Node *sp, Node *np) noexcept
    {
      assert(LLINK(fp) == sp || RLINK(fp) == sp);
      assert(LLINK(sp) == np || RLINK(sp) == np);
      assert(COLOR(sp) == BLACK);
      assert(COLOR(np) == RED);
      assert(!path.is_empty());

      Node *ffp = path.top();

      assert(LLINK(ffp) == fp || RLINK(ffp) == fp);

      /* rotation for downing low fp */
      if (RLINK(sp) == np)
        rotate_to_left(fp, ffp);
      else
        rotate_to_right(fp, ffp);

      /* coloring for fixing up red black conditions */
      COLOR(sp) = COLOR(fp);
      COLOR(fp) = BLACK;
      COLOR(np) = BLACK;
    }

    /** @brief Double rotation and recolor to fix violation.
        
        Case: nephew snp is red and on the "inside" (opposite side from sibling).
        Double rotation (rotate sibling, then parent) fixes the tree.
        
        @pre sp is black, snp is red
        @param fp Parent of violating node
        @param sp Sibling (black)
        @param snp Sibling's nephew (red, opposite side from sp)
    */
    void doubleRotateNephewAndColor(Node *fp, Node *sp, Node *snp) noexcept
    {
      assert(LLINK(fp) == sp || RLINK(fp) == sp);
      assert(LLINK(sp) == snp || RLINK(sp) == snp);
      assert(COLOR(sp) == BLACK);
      assert(COLOR(snp) == RED);
      assert(!path.is_empty());

      Node *ffp = path.top();

      assert(LLINK(ffp) == fp || RLINK(ffp) == fp);

      /* double rotation for raising up of snp. snp becomes the new root of
         sub tree fp  */
      if (LLINK(sp) == snp)
        {
          rotate_to_right(sp, fp);
          rotate_to_left(fp, ffp);
        }
      else
        {
          rotate_to_left(sp, fp);
          rotate_to_right(fp, ffp);
        }

      /* coloring for restoring all red black tree conditions */
      COLOR(snp) = COLOR(fp);
      COLOR(fp) = BLACK;
    }

    /** @brief Color sibling red to propagate violation upward.
        
        After this, fp becomes the new violating node.
        
        @param sp Sibling (black, both children black)
    */
    static void colorSiblingAsRed(Node *sp) noexcept
    {
      assert(COLOR(sp) == BLACK);
      COLOR(sp) = RED;
    }

    /** @brief Recolor parent and sibling to fix violation.
        
        Case: parent is red. Swapping colors fixes the tree.
        
        @param fp Parent (red)
        @param sp Sibling (black)
    */
    static void colorParentAndSibling(Node *fp, Node *sp) noexcept
    {
      assert(LLINK(fp) == sp || RLINK(fp) == sp);
      assert(COLOR(fp) == RED);
      assert(COLOR(sp) == BLACK);

      COLOR(fp) = BLACK;
      COLOR(sp) = RED;
    }

    /** @brief Remove node and fix any black-height violations.
        
        Performs the actual BST deletion (bypass or swap with successor),
        then walks up the path fixing violations.
        
        @param q Node to remove (must be on top of path stack)
    */
    void removeAndFixBlackCondition(Node *q) noexcept
    {
      assert(path.top() == q);

      /* look at second item after stack's head. We cannot pop it because
         successor finding would require stack pushing */
      Node *fq = path.top(1);
      Node *p; /* Saves the new child of fp after p has been deleted, this
                  node can be a violating black condition black node */

      assert(fq != Node::NullPtr);
      assert(LLINK(fq) == q || RLINK(fq) == q);

      /* Deletion step: by pass if there is a Node::NullPtr link or swap
         with successor */
      while (1)
        {
          if (LLINK(q) == Node::NullPtr) // by pass to the left side
            {
              if (LLINK(fq) == q)
                p = LLINK(fq) = RLINK(q);
              else
                p = RLINK(fq) = RLINK(q);
              break;
            }

          if (RLINK(q) == Node::NullPtr) // by pass to the right side
            {
              if (LLINK(fq) == q)
                p = LLINK(fq) = LLINK(q);
              else
                p = RLINK(fq) = LLINK(q);
              break;
            }

          findSuccAndSwap(q, fq);
        }

      /* if color of deleted node is red, then all red black conditions are
         met and adjust is not necessary */
      if (COLOR(q) == RED)
        {
          assert(COLOR(p) == BLACK);
          path.empty();
          return;
        }

      /* if color of p is black, then it misses a black node and four condition
         is violated. However, if p's child is red we can recoloring it black
         for restoring the missing black node */
      if (COLOR(p) == RED)
        {
          COLOR(p) = BLACK;
          path.empty();
          return;
        }

      /* Bad luck we must do recoloring and/or balancing for restoring the
         four condition */
      // p's sibling
      Node *np, *snp; // p's nephew and nephewg's sibling
      Node *fp = fq; // we process in function of p

      path.popn(2); // pops deleted node and fp

      /* we examine p and we restore red black properties */
      while (true)
        {
          if (p == root)
            break;

          Node *sp = getSibling(p, fp);

          /* if sibling is red, we rotate down for assuring that p' sibling
             to be black */
          if (COLOR(sp) == RED)
            balanceDownAndColor(p, fp, sp);

          assert(COLOR(sp) == BLACK);

          /* Compute nephews */
          if (LLINK(fp) == p)
            {
              np = RLINK(sp);
              snp = LLINK(sp);
            }
          else
            {
              np = LLINK(sp);
              snp = RLINK(sp);
            }

          if (COLOR(np) == RED)
            {
              rotateNephewAndColor(fp, sp, np);
              break;
            }

          if (COLOR(snp) == RED)
            {
              doubleRotateNephewAndColor(fp, sp, snp);
              break;
            }

          if (COLOR(fp) == RED)
            {
              colorParentAndSibling(fp, sp);
              break;
            }

          /* color and restart process with fp */
          colorSiblingAsRed(sp);
          p = fp;
          fp = path.pop(); // colorSiblingAsRed(sp) does not pop it
        }
      path.empty();
    }

  public:
    /// The key type stored in nodes
    using key_type = Key;

    /// Returns a reference to the comparison functor
    Compare & key_comp() noexcept { return cmp; }
    
    /// \overload key_comp() const
    const Compare & key_comp() const noexcept { return cmp; }

    /// Alias for key_comp()
    Compare & get_compare() noexcept { return cmp; }
    
    /// \overload get_compare() const
    [[nodiscard]] constexpr const Compare & get_compare() const noexcept { return cmp; }

    /// Default constructor
    HtdRbTree(Compare __cmp = Compare()) noexcept
      : head(&headNode),
        fHead(&headParent),
        ffHead(&headGrandParent),
        root(headNode.getR()),
        cmp(__cmp)
    {
      RLINK(fHead) = head;
      RLINK(ffHead) = fHead;
      COLOR(Node::NullPtr) = BLACK;
      COLOR(head) = BLACK;
      COLOR(fHead) = BLACK;
      COLOR(ffHead) = BLACK;
    }

    /// Swap contents with another tree
    void swap(HtdRbTree & tree) noexcept
    {
      std::swap(root, tree.root);
      std::swap(node_count, tree.node_count);
      std::swap(cmp, tree.cmp);
    }

    /// Move constructor
    HtdRbTree(HtdRbTree && tree) noexcept
      : head(&headNode),
        fHead(&headParent),
        ffHead(&headGrandParent),
        root(headNode.getR()),
        path(Node::MaxHeight),
        node_count(0),
        cmp()
    {
      RLINK(fHead) = head;
      RLINK(ffHead) = fHead;
      COLOR(Node::NullPtr) = BLACK;
      COLOR(head) = BLACK;
      COLOR(fHead) = BLACK;
      COLOR(ffHead) = BLACK;
      swap(tree);
    }

    /// Move assignment
    HtdRbTree & operator=(HtdRbTree && tree) noexcept
    {
      swap(tree);
      return *this;
    }
    
    /// Copy constructor deleted
    HtdRbTree(const HtdRbTree &) = delete;
    
    /// Copy assignment deleted
    HtdRbTree & operator=(const HtdRbTree &) = delete;

    /// Virtual destructor
    virtual ~HtdRbTree() = default;

    /// Check if tree is empty
    [[nodiscard]] constexpr bool is_empty() const noexcept { return root == Node::NullPtr; }

    /// Get number of nodes in tree
    [[nodiscard]] constexpr size_t size() const noexcept { return node_count; }
    
    /// Reset tree (does not free nodes)
    void reset() noexcept
    {
      root = Node::NullPtr;
      node_count = 0;
    }

    /** @brief Insert a node into the tree.
        
        @param p Node to insert (must have COLOR=RED and null children)
        @return Pointer to inserted node, or nullptr if duplicate key
    */
    Node * insert(Node *p) noexcept
    {
      assert(p != Node::NullPtr);
      assert(COLOR(p) == RED);
      assert(LLINK(p) == Node::NullPtr && RLINK(p) == Node::NullPtr);

      if (root == Node::NullPtr) [[unlikely]]
        {
          root = p;
          ++node_count;
          return p;
        }

      return searchFlipColorsAndInsert(p);
    }

    /** @brief Search for key or insert if not found.
        
        @param p Node to insert if key not found
        @return Existing node with key, or p if inserted
    */
    Node * search_or_insert(Node *p) noexcept
    {
      assert(p != Node::NullPtr);
      assert(COLOR(p) == RED);
      assert(LLINK(p) == Node::NullPtr && RLINK(p) == Node::NullPtr);

      if (root == Node::NullPtr) [[unlikely]]
        {
          root = p;
          ++node_count;
          return p;
        }

      const Key & pk = KEY(p);
      Node *found = search(pk);
      if (found != nullptr) [[unlikely]]
        return found;

      return insert(p);
    }

    /** @brief Insert allowing duplicate keys.
        
        @param p Node to insert
        @return Pointer to inserted node
    */
    Node * insert_dup(Node *p) noexcept
    {
      assert(p != Node::NullPtr);
      assert(COLOR(p) == RED);
      assert(LLINK(p) == Node::NullPtr && RLINK(p) == Node::NullPtr);

      if (root == Node::NullPtr) [[unlikely]]
        {
          root = p;
          ++node_count;
          return p;
        }

      return searchFlipColorsAndInsertDup(p);
    }

    /** @brief Search for a key.
        
        @param key Key to search for
        @return Pointer to node with key, or nullptr if not found
    */
    Node * search(const Key & key) const noexcept
    {
      Node *retVal = searchInBinTree(root, key, cmp);
      return retVal == Node::NullPtr ? nullptr : retVal;
    }

    /** @brief Remove node with given key.
        
        @param key Key to remove
        @return Pointer to removed node, or nullptr if not found
    */
    Node * remove(const Key & key) noexcept
    {
      if (root == Node::NullPtr) [[unlikely]]
        return nullptr;

      Node *p = searchAndBuildPath(key);

      if (no_equals<Key, Compare>(KEY(p), key, cmp))
        {
          path.empty();
          return nullptr;
        }

      removeAndFixBlackCondition(p);

      assert(node_count > 0);
      --node_count;

      p->reset();

      return p;
    }


    /// Get reference to root pointer
    Node *& getRoot() noexcept { return root; }
    
    /// Get root pointer (const)
    Node * getRoot() const noexcept { return root; }

  private:
    /** @brief Verify black height consistency.
        
        Recursively checks that all paths to leaves have the same
        number of black nodes.
        
        @param p Node to check from
        @param max Current maximum black height (initialize to -1)
        @param bh Current black height count
    */
    static void blackHeight(Node *p, int & max, int bh = 0) noexcept
    {
      if (COLOR(p) == BLACK)
        bh++; // Another seen black node

      /* if a leaf is reached, we must verify max with current bh (number of
       visited black nodes */
      if (LLINK(p) == Node::NullPtr && RLINK(p) == Node::NullPtr)
        {
          if (max == -1) // This is onbly for the first leaf reached
            max = bh;
          assert(bh == max);
          return;
        }

      if (LLINK(p) != Node::NullPtr) /* continue counting in the left side */
        blackHeight(LLINK(p), max, bh);

      if (RLINK(p) != Node::NullPtr) /* continue counting in the right side */
        blackHeight(RLINK(p), max, bh);
    }

    /// Verify red condition and black height for a node
    static void testNode(Node *p) noexcept
    {
      /* verify red condition */
      COND_assert(COLOR(p) == RED,
                  COLOR(LLINK(p)) == BLACK && COLOR(RLINK(p)) == BLACK);

      int max = -1;
      int bh = 0;

      blackHeight(p, max, bh);
    }

  public:
    /** @brief Verify red-black tree invariants (DEBUG mode).
        
        Checks that each node satisfies red-black conditions.
        Aborts on violation.
    */
    void verifyRedBlack() const
    {
      preOrderRec(root, testNode);
    }

    /** @brief Verify tree is a valid red-black BST.
        
        @return true if all red-black properties hold
    */
    bool verify() const noexcept
    {
      return is_red_black_bst(const_cast<Node *>(root),
                              const_cast<HtdRbTree *>(this)->cmp);
    }

    /// In-order iterator
    struct Iterator : public BinNodeInfixIterator<Node>
    {
      Iterator(HtdRbTree & t) noexcept
        : BinNodeInfixIterator<Node>(t.getRoot()) {}

      Iterator(const HtdRbTree & t) noexcept
        : BinNodeInfixIterator<Node>(const_cast<HtdRbTree &>(t).getRoot()) {}
    };
  };

/** @brief Hybrid red-black tree with virtual node destructor.
    
    Same as HtdRbTree but uses RbNodeVtl which has virtual destructor.
*/
  template <class Key, class Compare = Aleph::less<Key>>
  class HtdRbTreeVtl : public HtdRbTree<Key, Compare>
  {
  public:
    using Base = HtdRbTree<Key, Compare>;
    using Node = typename Base::Node;
    using Base::Base;  // Inherit constructors
  };

} // namespace Aleph

#endif /* TPL_HTDRBTREE_H */
