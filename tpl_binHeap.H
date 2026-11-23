
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


/** @file tpl_binHeap.H
 *  @brief Binary heap implementation using tree structure.
 *
 *  This file provides a binary heap implementation that uses a binary tree
 *  structure instead of an array. This allows for dynamic sizing and
 *  efficient node-based operations.
 *
 *  A heap on an array finds the parent and the children of an entry with
 *  arithmetic on its index, and its last entry at the end of the array. The
 *  heap of this file has no array and no indexes: it gets the same from the
 *  links of its nodes, in O(1), because it keeps the nodes of the bottom of
 *  the tree in a list. The section "Representation" describes how; whoever
 *  changes this file must read it first.
 *
 *  ## Heap Types
 *
 *  - **GenBinHeap**: Generic base heap with customizable comparison
 *  - **BinHeap**: Min-heap (smallest element at root)
 *  - **BinHeapVtl**: Min-heap with virtual destructor for polymorphism
 *
 *  With the reverse comparison, any of them is a max-heap.
 *
 *  ## Key Features
 *
 *  - Tree-based structure (not array-based)
 *  - Parent pointers for efficient operations
 *  - The place of an insertion and the last node, each in O(1), from a list
 *    of the nodes that lack a child
 *  - Supports iteration via Iterator class
 *  - The nodes belong to the caller, which allocates and frees them
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | insert | O(log n) |
 *  | top | O(1) |
 *  | getMin/getMax | O(log n) |
 *  | remove | O(log n) |
 *  | update | O(log n) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  BinHeap<int> heap;
 *  BinHeap<int>::Node a(5), b(3), c(7); // the nodes belong to the caller
 *
 *  heap.insert(&a);
 *  heap.insert(&b);
 *  heap.insert(&c);
 *
 *  while (not heap.is_empty())
 *    {
 *      BinHeap<int>::Node *min = heap.getMin(); // removes the minimum
 *      std::cout << min->get_key() << '\n';     // 3, then 5, then 7
 *    }
 *  ```
 *
 *  ## Representation
 *
 *  ### The tree
 *
 *  The tree is complete: all its levels are full except, at most, the last
 *  one, whose nodes are at its left. Number the places of the nodes by
 *  levels and from left to right, with the root at 1: the place i has its
 *  parent at i / 2 and its children at 2 i and 2 i + 1, and a heap of n
 *  nodes fills the places 1 to n. These are the indexes of a heap on an
 *  array. Here no node knows its own, but the links follow them. The shape
 *  of the tree depends only on n: the operations change which node is at
 *  each place.
 *
 *  Each node has three links, LLINK, RLINK and ULINK, and two flags: IS_LEAF,
 *  it has no children, and IS_LEFT, it is the left child of its parent.
 *  ULINK is its parent; that of the root is the header node of the heap,
 *  whose right child is the root. The heap keeps the root, the node at n,
 *  which is called `last`, and n.
 *
 *  ### The list of the nodes that lack a child
 *
 *  A node with two children has them in LLINK and RLINK, as in any binary
 *  tree. A node that lacks a child has links to spare, and the heap uses
 *  those links to keep all such nodes in a circular doubly linked list, in
 *  the order of their places: RLINK is the next node of the list and LLINK
 *  the previous one.
 *
 *  Those nodes are the ones at the places f to n, where f = (n + 1) / 2,
 *  that is, n / 2 rounded up: all the leaves and, if n is even, the parent
 *  of the last node, which has a left child only. The list starts at f and
 *  ends at `last`, which is followed by the node at f again.
 *
 *  If n is even, the first node of the list is the parent of the last one.
 *  Its LLINK is then its left child and its previous node of the list at
 *  once, with no conflict, since both are `last`. Its RLINK is its next
 *  node of the list, not a child.
 *
 *  ```text
 *                         1                      n = 10, f = 5
 *                   /           \
 *                 2               3              1 to 4: two children
 *               /   \           /   \
 *              4     5         6     7           5: a left child only
 *             / \   /
 *            8   9 10                            6 to 10: leaves
 *
 *     the list:   5 <-> 6 <-> 7 <-> 8 <-> 9 <-> 10 = last
 *                 ^                              ^
 *                 +------------------------------+
 *
 *     LLINK(5) = 10: its left child and its previous node of the list
 *     RLINK(5) = 6:  its next node of the list
 *  ```
 *
 *  A heap of one node has it as its own next and previous node. In an empty
 *  heap the root is null and `last` is the header node.
 *
 *  ### What the list is for
 *
 *  The two nodes around `last` in the list are the ones that an insertion
 *  and a removal need:
 *
 *  - RLINK(last), the node at f, is the parent of the place n + 1, because
 *    (n + 1) / 2 = f. An insertion makes the new node its child, the left
 *    one if it had none and the right one otherwise, and puts it in the list
 *    after `last`, as the new last node. A parent that gets its second child
 *    leaves the list.
 *  - LLINK(last), the node at n - 1, is the last node once `last` is out. A
 *    removal takes `last` out of the list, and a parent that loses its
 *    second child enters the list as its first node.
 *
 *  So both find their place in O(1): with no array, no search and no path
 *  computed from the bits of n. Only the sifts, which exchange a node with
 *  its parent level by level, take O(log n).
 *
 *  ### Reading the links
 *
 *  Whether a link is a child or a neighbour of the list comes from the
 *  flags and from the list itself:
 *
 *  - If IS_LEAF(p), both links of p are of the list.
 *  - Otherwise LLINK(p) is the left child of p, and RLINK(p) is its right
 *    child unless the left one is the only child. A node is an only child
 *    when its next node of the list is its own parent, which only happens
 *    to `last`, when n is even: this is has_sibling().
 *
 *  advance_left() and advance_right() give the children of a node by those
 *  rules, and null where there is none. Whatever walks the tree must use
 *  them: followed blindly, the links never reach a null pointer, they go
 *  round the list.
 *
 *  ### Exchanges
 *
 *  A sift exchanges a node with its parent, swap_with_parent(), and an
 *  extraction exchanges the root with the last node, swap_root_with_last().
 *  An exchange gives each of the two nodes the place of the other, in the
 *  tree and in the list, flags included, so that the shape and the list stay
 *  those of n. That is why swap_with_parent() tells apart whether the nodes
 *  are in the list: neither, the child only, or both, which is the case of
 *  `last` under a parent that has no other child.
 *
 *  Since the links depend only on which node is at each place, an exchange
 *  is undone by exchanging the same two nodes again, and the linking of a
 *  last node by its removal. That is how an operation leaves the heap as it
 *  was when the comparison throws: it takes each node back to its place,
 *  with exchanges, which compare nothing and do not throw.
 *
 *  ## When to Use
 *
 *  Use tree-based heap when:
 *  - Need dynamic resizing without reallocation
 *  - Working with node pointers directly
 *  - Need to update priorities of existing nodes
 *
 *  For array-based heaps (more cache-friendly), see:
 *  - tpl_arrayHeap.H
 *  - tpl_dynArrayHeap.H
 *
 *  @see tpl_arrayHeap.H Array-based heap implementation
 *  @see tpl_dynBinHeap.H Dynamic heap with key updates
 *  @see tpl_fibonacci_heap.H Fibonacci heap for advanced priority queues
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BINHEAP_H
# define TPL_BINHEAP_H

# include <type_traits>
# include <utility>

# include <ah-concepts.H>
# include <htlist.H>
# include <tpl_arrayStack.H>
# include <tpl_binNode.H>
# include <tpl_dynListQueue.H>

using namespace Aleph;

namespace Aleph
{
  class BinHeapNode_Data
  {
    struct Control_Fields //Defining Control Flags
    {
      int is_leaf: 4; // true If the node is leaf
      int is_left: 4; // true if the node is a left child
    };

    BinHeapNode_Data *pLink; // Pointer to the parent
    Control_Fields control_fields;

  public:
    BinHeapNode_Data() noexcept : pLink(nullptr)
    {
      control_fields.is_leaf = true;
      control_fields.is_left = true;
    }

    BinHeapNode_Data *&getU() noexcept { return pLink; }

    Control_Fields &get_control_fields() noexcept { return control_fields; }

    void reset() noexcept
    {
      control_fields.is_leaf = true;
      control_fields.is_left = true;
    }
  };

  DECLARE_BINNODE(BinHeapNode, 64, BinHeapNode_Data);

  // The previous and the next node of a node of the list of the nodes that
  // lack a child: the links LLINK and RLINK of the node, under the names of
  // their other use. See "Representation" in the description of this file
# define PREV(p)      (p->getL())
# define NEXT(p)      (p->getR())
# define ULINK(p)     reinterpret_cast<Node*&>((p)->getU())
# define IS_LEAF(p)   ((p)->get_control_fields().is_leaf)
# define IS_LEFT(p)   ((p)->get_control_fields().is_left)
# define CTRL_BITS(p) ((p)->get_control_fields())

  /** Generic heap of nodes.

      The GenBinHeap class implements a heap of nodes. It is not on an
      array, but on a binary tree of linked nodes, which makes it highly
      dynamic: the memory used is proportional to the number of nodes of
      the heap.

      The tree is complete, and the nodes that lack a child, which are those
      of its bottom, are also in a circular doubly linked list made with the
      links that they do not use for children. The node after the last one
      in that list is the parent of the next node to insert, and the node
      before it is the last one after a removal, so the heap finds both in
      O(1). The description of the file tpl_binHeap.H, in its section
      "Representation", has the whole of it.

      This class is not intended for public use. Its purpose is to provide
      basic functionality to the BinHeap, BinHeapVtl, and
      DynBinHeap.

      @param NodeType the type of node the HEAP uses; this will be with
      or without a virtual destroyer.
      @param Key the key that each node keeps.
      @param Compare the criterion of comparison between the keys of the
      Nodes.

      @see BinHeap BinHeapVtl DynBinHeap

      @ingroup Trees
  */
  template <template <class> class NodeType, typename Key,
            class Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
  class GenBinHeap
  {
  protected:
    Compare cmp;

    // The operations that compare keys throw only if the comparison does,
    // or the conversion of its result to bool: the one that a condition
    // makes, which for a result of a class type may be another function
    // than that of an implicit conversion
    static constexpr bool nothrow_cmp =
      noexcept(static_cast<bool>(std::declval<Compare &>()(std::declval<Key &>(),
                                                           std::declval<Key &>())));

  public:
    using Node = NodeType<Key>;

    Compare &key_comp() noexcept { return cmp; }

    Compare &get_compare() noexcept { return cmp; }

  private:
    /// The header node: the root is its right child
    Node head_node;
    /// The address of the header node
    Node *head;
    /// The node at the place 1; null in an empty heap
    Node *& root;
    /// The node at the place n, where the list of the nodes that lack a
    /// child ends; the header node in an empty heap
    Node *last;
    /// The number n of nodes
    size_t num_nodes;

  public:
    void swap(GenBinHeap & h) noexcept
    {
      std::swap(root, h.root);
      std::swap(last, h.last);
      std::swap(num_nodes, h.num_nodes);
      std::swap(cmp, h.cmp);
      adopt_nodes();
      h.adopt_nodes();
    }

  private:
    // After a swap, the root must point up to the header of its new heap,
    // and an empty heap has its own header as its last node, as when it was
    // built
    void adopt_nodes() noexcept
    {
      if (root != nullptr)
        ULINK(root) = head;
      else
        last = &head_node;
    }

    /// Whether p is in the list of the nodes that lack a child: it is a
    /// leaf, or its left child is its only one
    static bool is_in_list(Node *p) noexcept
    {
      if (IS_LEAF(p))
        return true;

      return ULINK(LLINK(p)) == RLINK(LLINK(p));
    }

    /// Whether the parent of p, which is not the root, has another child.
    /// It does not when the next node of p in the list is that parent,
    /// which only happens to the last node, if it is a left child
    static bool has_sibling(Node *p) noexcept
    {
      return ULINK(p) != RLINK(p);
    }

    /// Exchanges the node p, which is not the root, with its parent: each
    /// one takes the place of the other in the tree and in the list of the
    /// nodes that lack a child, and its flags. The heaps of 2 and of 3
    /// nodes apart, the cases are three: neither node is in the list, only
    /// p is, or both are, which is when p is the last node and the only
    /// child of its parent
    void swap_with_parent(Node *p) noexcept
    {
      assert(num_nodes >= 2);
      assert(p != root);

      Node *pp = ULINK(p); // the parent of p

      const bool p_has_sibling = has_sibling(p);
      const bool p_is_in_list = is_in_list(p); // does p lack a child?
      const bool pp_is_in_list = is_in_list(pp); // is p the only child of pp? then p == last
      const bool p_has_child = not IS_LEAF(p); // if p is in the list, a left child only

      std::swap(CTRL_BITS(pp), CTRL_BITS(p));

      if (pp == root)
        root = p;

      Node *ppp = ULINK(pp); // the parent of pp

      ULINK(pp) = p;
      ULINK(p) = ppp;

      if (LLINK(ppp) == pp)
        LLINK(ppp) = p;
      else
        RLINK(ppp) = p;

      Node *sp = nullptr; // the sibling of p
      if (p_has_sibling)
        {
          sp = p == LLINK(pp) ? RLINK(pp) : LLINK(pp);
          assert(ULINK(sp) == pp);
          ULINK(sp) = p;
        }

      if (p == last)
        last = pp;

      if (num_nodes == 2)
        return;

      // The children of p or, where p lacks one, its neighbours of the list
      Node *lcp = LLINK(p);
      Node *rcp = RLINK(p);

      if (num_nodes == 3)
        {
          if (RLINK(pp) == p)
            {
              LLINK(lcp) = RLINK(lcp) = pp;
              RLINK(pp) = lcp;
              RLINK(p) = pp;
            }
          else
            {
              LLINK(rcp) = RLINK(rcp) = pp;
              LLINK(pp) = rcp;
              LLINK(p) = pp;
            }

          return;
        }

      if (not p_is_in_list) // lcp and rcp are the children of p
        {
          ULINK(lcp) = ULINK(rcp) = pp;

          if (LLINK(pp) == p)
            {
              assert(RLINK(pp) == sp);
              LLINK(p) = pp;
              RLINK(p) = RLINK(pp);
            }
          else
            {
              assert(LLINK(pp) == sp);
              RLINK(p) = pp;
              LLINK(p) = LLINK(pp);
            }

          LLINK(pp) = lcp;
          RLINK(pp) = rcp;

          return;
        }

      if (not pp_is_in_list) // pp takes the place of p in the list
        {
          if (p_has_child)
            ULINK(LLINK(p)) = pp;

          RLINK(lcp) = LLINK(rcp) = pp;

          if (LLINK(pp) == p)
            {
              assert(RLINK(pp) == sp);
              LLINK(p) = pp;
              RLINK(p) = RLINK(pp);
            }
          else
            {
              assert(LLINK(pp) == sp);
              RLINK(p) = pp;
              LLINK(p) = LLINK(pp);
            }

          LLINK(pp) = lcp;
          RLINK(pp) = rcp;

          return;
        }

      // p is the last node and pp the first one of the list, which follows
      // p. They exchange those two places: pp ends as the only child of p,
      // and so as its previous node too
      RLINK(lcp) = pp;
      LLINK(RLINK(pp)) = p;
      LLINK(pp) = lcp;
      RLINK(p) = RLINK(pp);
      RLINK(pp) = p;
      LLINK(p) = pp;
    }

    // The sifts exchange the node with its parent, or with a child, level by
    // level, as they compare. If the comparison throws they leave the node
    // where it was by then, and the operation that called them takes it
    // back to its place: an exchange is undone by another one, which
    // compares nothing and does not throw, so the heap is as it was before
    // the exception goes on. That undoing is in functions of its own, cold
    // and out of line, to keep it out of the way of the code that runs when
    // nothing throws. A comparison that cannot throw leaves nothing to
    // undo: the operations are compiled then without it (nothrow_cmp).

    /// Takes p up while its key is less than that of its parent
    virtual void sift_up(Node *p) noexcept(nothrow_cmp)
    {
      while (p != root and cmp(KEY(p), KEY(ULINK(p))))
        swap_with_parent(p);
    }

    /// Takes p down while its key is not less than that of its child with
    /// the least key
    virtual void sift_down(Node *p) noexcept(nothrow_cmp)
    {
      while (not IS_LEAF(p))
        {
          Node *cp = LLINK(p); // the child of p with the least key
          if (has_sibling(cp))
            if (cmp(KEY(RLINK(p)), KEY(LLINK(p))))
              cp = RLINK(p);

          if (cmp(KEY(p), KEY(cp)))
            return;

          swap_with_parent(cp);
        }
    }

    /// Takes back up the node p, which went down from under the node pp,
    /// until it is under it again
    void back_up(Node *p, Node *pp) noexcept
    {
      while (ULINK(p) != pp)
        swap_with_parent(p);
    }

    /// Takes back down the node p, which went up, to the place of the node
    /// c, which is below it. The nodes that p passed are those of the way
    /// from c up to p: p exchanges with each one of them, from the top, so
    /// it goes over the way once
    void back_down(Node *p, Node *c) noexcept
    {
      if (Node *over = ULINK(c); over != p)
        back_down(p, over); // first to the place over that of c
      swap_with_parent(c);
    }

    /// Undoes what sift_down(p) and sift_up(p) did before the comparison
    /// threw: takes p, which is below or above its place, back under the
    /// node pp, which was over it
    [[gnu::cold, gnu::noinline]] void back_under(Node *p, Node *pp) noexcept
    {
      Node *q = ULINK(p); // pp is over p if p is below its place, or at it
      while (q != pp and q != head)
        q = ULINK(q);
      if (q == pp)
        back_up(p, pp);
      else
        back_down(p, pp); // pp is at the place of p, which went up
    }

    /// Undoes an insertion whose sift_up(p) threw: takes p back down to the
    /// last place, where it was linked, and unlinks it
    [[gnu::cold, gnu::noinline]] void undo_insert(Node *p) noexcept
    {
      if (p != last)
        back_down(p, last);
      remove_last();
    }

    /// Undoes an extraction whose sift_down() threw: the node that took
    /// the place of the root goes back up to it, the old root is linked
    /// again as the last node, and the two exchange their places
    [[gnu::cold, gnu::noinline]] void undo_extraction(Node *moved, Node *old_root) noexcept
    {
      back_up(moved, head);
      link_last(old_root);
      swap_root_with_last();
    }

    /// Undoes a removal whose update(p) threw, which left p at the place of
    /// node: node goes back to it, and p to the end
    [[gnu::cold, gnu::noinline]] void undo_removal(Node *node, Node *p) noexcept
    {
      replace_node(p, node);
      p->reset();
      link_last(p);
    }

    /// Links again as the last nodes two nodes that were taken out: first,
    /// and then second
    [[gnu::cold, gnu::noinline]] void link_last_two(Node *first, Node *second) noexcept
    {
      link_last(first);
      link_last(second);
    }

    /// Exchanges the root with the last node: each one takes the place of
    /// the other in the tree, and the root that of the last node in the
    /// list of the nodes that lack a child
    void swap_root_with_last() noexcept
    {
      assert(num_nodes > 1);
      assert(ULINK(root) == head);
      assert(not IS_LEAF(root));
      assert(IS_LEAF(last));

      if (num_nodes > 3) // general case
        {
          Node *lRoot = LLINK(root);
          Node *rRoot = RLINK(root);
          Node *f_last = ULINK(last);
          Node *prev_last = LLINK(last);
          Node *next_last = RLINK(last);

          if (LLINK(f_last) == last)
            LLINK(f_last) = root;
          else
            RLINK(f_last) = root;

          if (RLINK(root) != last)
            std::swap(ULINK(root), ULINK(last));
          else
            {
              ULINK(root) = last;
              ULINK(root) = head;
            }

          std::swap(LLINK(root), LLINK(last));
          std::swap(RLINK(root), RLINK(last));

          ULINK(lRoot) = ULINK(rRoot) = last;

          LLINK(last) = lRoot;
          RLINK(last) = rRoot;

          PREV(root) = prev_last;
          NEXT(root) = next_last;

          NEXT(prev_last) = PREV(next_last) = root;
        }
      else if (num_nodes == 3) // special case with 3 nodes
        {
          assert(RLINK(root) == last);
          assert(LLINK(last) == LLINK(root) and RLINK(last) == LLINK(root));

          ULINK(last) = ULINK(root);
          ULINK(root) = last;

          Node *s_last = LLINK(last);
          ULINK(s_last) = last;

          LLINK(last) = s_last;
          RLINK(last) = root;

          LLINK(root) = RLINK(root) = s_last;
          RLINK(s_last) = LLINK(s_last) = root;
        }
      else // special case with 2 nodes
        {
          assert(LLINK(root) == last);

          ULINK(last) = ULINK(root);
          ULINK(root) = last;
          RLINK(last) = LLINK(last) = root;
          RLINK(root) = LLINK(root) = last;
        }

      std::swap(CTRL_BITS(root), CTRL_BITS(last));
      std::swap(root, last);
    }

    /// Takes the last node, which is not the root, out of the tree and of
    /// the list of the nodes that lack a child, and returns it. Its
    /// previous node in the list is the new last node, and its parent is
    /// the first node of the list from then on: it stays so if the last
    /// node was its only child, and it enters the list if it had two
    Node * remove_last() noexcept
    {
      assert(last != root and num_nodes > 0);
      assert(IS_LEAF(last));

      Node *ret_val = last;
      Node *pp = ULINK(last);
      Node *new_last = LLINK(last);

      if (IS_LEFT(last))
        { // pp is left without children: its links are both of the list
          IS_LEAF(pp) = true;
          LLINK(pp) = new_last;
        }
      else
        { // pp is left with its left child: its right link is of the list
          RLINK(pp) = RLINK(last);
          LLINK(RLINK(last)) = pp;
        }

      RLINK(LLINK(last)) = pp; // the list is circular
      last = new_last;
      num_nodes--;
      ret_val->reset();

      return ret_val;
    }

    /// Puts new_node, which is out of the heap, at the place of node, which
    /// is not the last one: in the tree and, if node lacks a child, in the
    /// list of the nodes that lack one. The links of node are left as they
    /// were
    void replace_node(Node *node, Node *new_node) noexcept
    {
      assert(node != new_node);
      assert(node != last);

      // save node's immediate relatives
      Node *parent = ULINK(node);
      Node *left_child = LLINK(node);
      Node *right_child = RLINK(node);

      // update new_node's own pointers
      ULINK(new_node) = parent;
      LLINK(new_node) = left_child;
      RLINK(new_node) = right_child;

      // update parent
      if (IS_LEFT(node))
        {
          assert(LLINK(parent) == node);
          LLINK(parent) = new_node;
        }
      else
        {
          assert(RLINK(parent) == node);
          RLINK(parent) = new_node;
        }

      // Update children
      if (IS_LEAF(node))
        { // they are not children, but the neighbours of node in the list
          RLINK(left_child) = new_node;
          LLINK(right_child) = new_node;
        }
      else
        {
          ULINK(left_child) = new_node;

          if (ULINK(right_child) == node) // Node could have only one child
            ULINK(right_child) = new_node;
          else
            {
              assert(left_child == last);
              RLINK(left_child) = new_node;
              LLINK(right_child) = new_node;
            }
        }

      CTRL_BITS(new_node) = CTRL_BITS(node);
    }

    /// Deletes the nodes of the tree of root p. incomplete_node is the node
    /// with a left child only, whose right link is of the list of the nodes
    /// that lack a child, or null if there is none
    static void __postorder_delete(Node *p, Node *incomplete_node) noexcept
    {
      if (IS_LEAF(p))
        {
          delete p;
          return;
        }

      __postorder_delete(LLINK(p), incomplete_node);

      if (p != incomplete_node)
        __postorder_delete(RLINK(p), incomplete_node);

      delete p;
    }

  public:
    Node * getRoot() noexcept { return root; }

    Node * getRoot() const noexcept { return const_cast<Node *>(root); }

  private:
    template <class Operation>
    static
    void __for_each_in_preorder(Node *p, Operation & operation)
    {
      if (p == nullptr)
        return;

      operation(p);
      __for_each_in_preorder(advance_left(p), operation);
      __for_each_in_preorder(advance_right(p), operation);
    }

    template <class Operation>
    static
    void __for_each_in_inorder(Node *p, Operation & operation)
    {
      if (p == nullptr)
        return;

      __for_each_in_inorder(advance_left(p), operation);
      operation(p);
      __for_each_in_inorder(advance_right(p), operation);
    }

    template <class Operation>
    bool preorder_traverse(Node *p, Operation op) const
    {
      if (p == nullptr)
        return true;
      if (not op(p))
        return false;
      if (not preorder_traverse(advance_left(p), op))
        return false;
      return preorder_traverse(advance_right(p), op);
    }

  public:
    template <class Operation>
    bool preorder_traverse(Operation op) const
    {
      return preorder_traverse(getRoot(), op);
    }

    template <class Operation>
    void for_each_in_preorder(Operation & operation) const
    {
      return __for_each_in_preorder<Operation>(getRoot(), operation);
    }

    template <class Operation>
    void for_each_in_preorder(Operation && operation = Operation()) const
    {
      return for_each_in_preorder(operation);
    }

    template <class Operation>
    void for_each_in_inorder(Operation & operation) const
    {
      return __for_each_in_inorder<Operation>(getRoot(), operation);
    }

    template <class Operation>
    void for_each_in_inorder(Operation && operation = Operation()) const
    {
      return for_each_in_inorder(operation);
    }

  private:
    template <class Op>
    bool __level_traverse(Node *root, Op & operation) const
    {
      if (root == nullptr)
        return true;

      DynListQueue<Node *> queue;
      queue.put(root);

      while (not queue.is_empty())
        {
          Node *p = queue.get();

          if (not operation(p))
            return false;

          Node *c = advance_left(p);
          if (c == nullptr)
            continue;

          queue.put(c);

          c = advance_right(p);
          if (c != nullptr)
            queue.put(c);
        }

      return true;
    }

  public:
    template <class Op>
    bool level_traverse(Op operation = Op()) const
    {
      return __level_traverse(getRoot(), operation);
    }

    /** @brief Builds an empty heap with the comparison `__cmp`.

        @param[in] __cmp the criterion of comparison, which the heap copies.
        @throws what the default construction of the key of the header node
        or the copy of `__cmp` throws; nothing when both are `noexcept`.
    */
    GenBinHeap(Compare __cmp = Compare())
      noexcept(std::is_nothrow_default_constructible_v<Node> and
               std::is_nothrow_copy_constructible_v<Compare>)
      : cmp(__cmp), head(&head_node), root(RLINK(head)), last(&head_node),
        num_nodes(0)
    {
      // empty
    }

    virtual ~GenBinHeap() noexcept
    { /* empty */
    }

  private:
    /// Links the node p as the last one of a heap that is not empty: after
    /// the last node in the list of the nodes that lack a child, and under
    /// the node that follows the last one in that list, which is the first
    /// one of the list. remove_last() undoes it
    void link_last(Node *p) noexcept
    {
      Node *pp = RLINK(last); // the parent of p
      LLINK(p) = last;
      ULINK(p) = pp;

      if (IS_LEFT(last))
        { // p will be a right child: pp is the parent of the last node too,
          // and with its second child it leaves the list
          IS_LEFT(p) = false;
          RLINK(p) = RLINK(pp);
          LLINK(RLINK(pp)) = p;
          RLINK(pp) = p;
        }
      else
        { // p will be a left child: pp stays as the first node of the list,
          // after p, which is its previous node as well as its child
          IS_LEFT(p) = true;
          RLINK(p) = pp;
          IS_LEAF(pp) = false; // if p is a left child ==> pp was a leaf
          LLINK(pp) = p;
        }

      assert(not IS_LEAF(pp));

      RLINK(last) = p;
      last = p;
      num_nodes++;
    }

  public:
    /** Inserts a node into a heap.

        insert(p) inserts node p into this heap.

        @param[in] p the node to insert.
        @return pointer to the inserted node.
        @throw anything that the comparison throws. Then p is out of the
        heap, which is as it was, and can be inserted again.
    */
    Node * insert(Node *p) noexcept(nothrow_cmp)
    {
      assert(IS_LEAF(p));

      if (root == nullptr) // Is HEAP empty?
        { // Yes, initialize

          assert(num_nodes == 0);

          root = p;
          LLINK(p) = RLINK(p) = p;
          ULINK(p) = head;
          IS_LEAF(p) = true;
          IS_LEFT(p) = false; /* root is right child of header node */
          last = root;
          num_nodes = 1;
          return p;
        }

      link_last(p);
      if constexpr (nothrow_cmp)
        sift_up(p);
      else
        try
          {
            sift_up(p);
          }
        catch (...)
          {
            undo_insert(p);
            throw;
          }
      return p;
    }

    /** Removes the node with the lowest priority from a heap that is not
        empty.

        @return pointer to the removed node.
        @throw anything that the comparison throws. Then the node is in
        the heap, which is as it was.
    */
    Node * getMin_ne() noexcept(nothrow_cmp)
    {
      Node *ret_val = root;
      if (num_nodes == 1)
        {
          root = nullptr;
          last = &head_node; // as in a heap just built
          ret_val->reset();
          num_nodes = 0;

          return ret_val;
        }

      swap_root_with_last();
      remove_last();
      if constexpr (nothrow_cmp)
        sift_down(root);
      else
        {
          Node *moved = root; // the node that was the last one
          try
            {
              sift_down(moved);
            }
          catch (...)
            {
              undo_extraction(moved, ret_val);
              throw;
            }
        }
      ret_val->reset();

      return ret_val;
    }

    /** Removes the node with the lowest priority from the heap.

        getMIn() extracts from this heap the node that holds the
        lowest priority value according to the comparison criterion
        defined in the declaration.

        @return pointer to the removed node.
        @throw underflow_error if the heap is empty.
        @throw anything that the comparison throws, as getMin_ne() does.
    */
    Node * getMin()
    {
      ah_underflow_error_if(root == nullptr) << "Heap is empty";
      return getMin_ne();
    }

    /// \overload getMin() to use when the priority order is reversed
    Node * getMax()
    {
      return getMin();
    }

    /** Updates the priority of a node contained in the heap.

        update(p) takes a node of the heap whose priority has been
        modified and updates its priority within the
        heap. The idea is that if for some reason a priority must be
        modified, then the extraction order can be updated.

        @param[in] p pointer to the node to update
        @throw anything that the comparison throws. Then no node has moved:
        the heap is as it was before the call, with p at its place, where
        its new priority may be out of order.
        @see insert()
    */
    void update(Node *p) noexcept(nothrow_cmp)
    {
      if constexpr (nothrow_cmp)
        {
          sift_down(p);
          sift_up(p);
        }
      else
        {
          Node *pp = ULINK(p); // the node over the place of p
          try
            {
              sift_down(p);
              sift_up(p);
            }
          catch (...)
            {
              back_under(p, pp);
              throw;
            }
        }
    }

    /** Removes node from the heap.

        remove(node) removes node from this heap.

        @param[in] node pointer to the node to remove.
        @throw underflow_error if the heap is empty
        @throw anything that the comparison throws. Then node is in the
        heap, which is as it was.
        @return pointer to the removed node
        @note It is not verified whether node belongs to the heap.
    */
    Node * remove(Node *node)
    {
      ah_underflow_error_if(root == nullptr) << "Heap is empty";

      if (node == root)
        return getMin_ne();

      if (node == last)
        return remove_last();

      Node *p = remove_last();

      if (node == last)
        {
          remove_last();
          try
            {
              insert(p);
            }
          catch (...)
            { // insert() left p out: the two go back to the end
              link_last_two(node, p);
              throw;
            }

          return node;
        }
      replace_node(node, p);
      node->reset(); // undo_removal() gives it the flags of its place again
      try
        {
          update(p);
        }
      catch (...)
        { // update() left p at the place of node
          undo_removal(node, p);
          throw;
        }

      return node;
    }

    /// Deletes all the nodes of the heap, invokes the destructors of
    /// the removed nodes, and frees all the memory
    void remove_all_and_delete() noexcept
    {
      if (root == nullptr)
        return;

      if (num_nodes <= 3)
        { // without comparing the keys, since the comparison may throw
          while (num_nodes > 1)
            delete remove_last();
          delete root;
          root = nullptr;
          last = &head_node;
          num_nodes = 0;
          return;
        }

      if (IS_LEFT(last))
        __postorder_delete(root, ULINK(last));
      else
        __postorder_delete(root, nullptr);

      root = nullptr; // reset as if it were the constructor
      last = &head_node;
      num_nodes = 0;
    }

    /// Returns the node with the lowest priority according to the
    /// comparison criterion specified in the declaration.
    Node * top()
    {
      ah_underflow_error_if(root == nullptr) << "Heap is empty";

      return root;
    }

    Node * top() const
    {
      ah_underflow_error_if(root == nullptr) << "Heap is empty";

      return const_cast<Node *>(root);
    }

    const size_t &size() const noexcept { return num_nodes; }

    bool is_empty() const noexcept { return size() == 0; }

  protected:
    /// The left child of p, or null if it has none. Whatever walks the tree
    /// must take the children from here and from advance_right(): in the
    /// nodes that lack a child, the links LLINK and RLINK are those of
    /// their list, and never null
    static Node * advance_left(Node *p) noexcept
    {
      if (IS_LEAF(p))
        return nullptr;

      return LLINK(p);
    }

    /// The right child of p, or null if it has none. A node with a left
    /// child only has in RLINK its next node of the list of the nodes that
    /// lack a child
    static Node * advance_right(Node *p) noexcept
    {
      if (IS_LEAF(p))
        return nullptr;

      if (not has_sibling(LLINK(p)))
        return nullptr;

      return RLINK(p);
    }

    /// Whether no node of the tree of root p has a key less than that of
    /// its parent
    virtual bool verify_heap(Node *p) const
    {
      Node *left_link = advance_left(p);
      if (left_link == nullptr)
        {
          assert(IS_LEAF(p));
          return true;
        }

      if (cmp(KEY(left_link), KEY(p)))
        return false;

      Node *right_link = advance_right(p);
      if (right_link == nullptr)
        return verify_heap(left_link);

      if (cmp(KEY(right_link), KEY(p)))
        return false;

      return verify_heap(left_link) and verify_heap(right_link);
    }

  public:
    /** Checks the order of the heap.

        @return true if no node has a key less than that of its parent.
        @throw anything that the comparison throws.
        @note It visits all the nodes: O(n).
    */
    bool verify_heap() const
    {
      if (root == nullptr)
        return true;

      return verify_heap(root);
    }

    class Iterator
    {
      static const size_t Stack_Size = 64;

      GenBinHeap *heap_ptr = nullptr;
      InlineStack<Node *, Stack_Size> s;
      Node *curr = nullptr;
      size_t pos = 0;

    public:
      /// Default constructor creates an "end" iterator
      Iterator() noexcept {}

      Iterator(const GenBinHeap & h)
        : heap_ptr(&const_cast<GenBinHeap &>(h))
      {
        if (h.is_empty())
          return;
        curr = h.root;
      }

      void reset_first() noexcept
      {
        s.empty();
        if (heap_ptr->is_empty())
          curr = nullptr;
        else
          curr = heap_ptr->root;
        pos = 0;
      }

      void reset_last() noexcept
      {
        s.empty();
        if (heap_ptr->is_empty())
          curr = nullptr;
        else
          {
            auto ptr = heap_ptr->root;
            curr = ptr;
            while (true)
              {
                ptr = advance_right(ptr);
                if (ptr == nullptr)
                  break;
                curr = ptr;
              }
          }
        pos = heap_ptr->num_nodes - 1;
      }

      bool has_curr() const noexcept { return curr != nullptr; }

      Node * get_curr_ne() const noexcept { return curr; }

      Node * get_curr() const
      {
        ah_overflow_error_if(not has_curr()) << "Iterator overflow";
        return get_curr_ne();
      }

      void next_ne() noexcept
      {
        ++pos;
        auto l = advance_left(curr), r = advance_right(curr);
        if (l != nullptr)
          {
            curr = l;
            if (r != nullptr)
              s.push(r);
            return;
          }

        if (r != nullptr)
          {
            curr = r;
            return;
          }

        if (s.is_empty())
          curr = nullptr;
        else
          curr = s.pop();
      }

      void next()
      {
        ah_overflow_error_if(not has_curr()) << "Iterator overflow";
        next_ne();
      }

      size_t get_pos() const noexcept { return pos; }

      void end() noexcept
      {
        s.empty();
        curr = nullptr;
        pos = heap_ptr->num_nodes;
      }
    };
  };

  /** Node heap without virtual destructor.

      The BinHeap class implements a heap of nodes. It is not on an array,
      but on a binary tree of linked nodes, which makes it highly dynamic:
      the memory used is proportional to the number of nodes of the heap.
      The description of the file tpl_binHeap.H, in its section
      "Representation", tells how the tree is kept.

      @param Key the key that each node keeps.
      @param Compare the criterion of comparison between the keys of the
      Nodes; by default is the relationship "less than".

      @see BinHeapVtl DynBinHeap

      @ingroup Trees
  */
  template <class Key, typename Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
  struct BinHeap : public GenBinHeap<BinHeapNode, Key, Compare>
  {
    /// The heap's node type.
    using Node = BinHeapNode<Key>;
    using GenBinHeap<BinHeapNode, Key, Compare>::GenBinHeap;
  };

  /** Heap of nodes with virtual destructor.

      The BinHeapVtl class implements a heap of nodes with a virtual
      destructor. It is not on an array, but on a binary tree of linked
      nodes, which makes it highly dynamic: the memory used is proportional
      to the number of nodes of the heap. The description of the file
      tpl_binHeap.H, in its section "Representation", tells how the tree is
      kept.

      @param Key the key that each node keeps.
      @param Compare the criterion of comparison between the keys of the
      Nodes; by default is the relationship "less than".

      @see BinHeap DynBinHeap

      @ingroup Trees
  */
  template <class Key, typename Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
  struct BinHeapVtl : public GenBinHeap<BinHeapNodeVtl, Key, Compare>
  {
    /// The heap's node type.
    using Node = BinHeapNodeVtl<Key>;
    using GenBinHeap<BinHeapNodeVtl, Key, Compare>::GenBinHeap;
  };

# undef PREV
# undef NEXT
# undef ULINK
# undef IS_LEAF
# undef IS_LEFT
# undef CTRL_BITS
} // end namespace Aleph
# endif //  TPL_BINHEAP_H
