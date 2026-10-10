
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
 *  ## Heap Types
 *
 *  - **GenBinHeap**: Generic base heap with customizable comparison
 *  - **BinHeap**: Min-heap (smallest element at root)
 *  - **BinHeapMax**: Max-heap (largest element at root)
 *  - **BinHeapVtl**: Min-heap with virtual destructor for polymorphism
 *
 *  ## Key Features
 *
 *  - Tree-based structure (not array-based)
 *  - Parent pointers for efficient operations
 *  - Supports iteration via Iterator class
 *  - Move semantics for efficient transfers
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | insert | O(log n) |
 *  | getMin/getMax | O(1) |
 *  | remove | O(log n) |
 *  | update | O(log n) |
 *  | build from n elements | O(n) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  BinHeap<int> heap;
 *  
 *  heap.insert(5);
 *  heap.insert(3);
 *  heap.insert(7);
 *  
 *  while (!heap.is_empty()) {
 *    int min = heap.getMin();  // Get minimum
 *    heap.getMin();            // Remove minimum
 *  }
 *  ```
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

# define PREV(p)      (p->getL())
# define NEXT(p)      (p->getR())
# define ULINK(p)     reinterpret_cast<Node*&>((p)->getU())
# define IS_LEAF(p)   ((p)->get_control_fields().is_leaf)
# define IS_LEFT(p)   ((p)->get_control_fields().is_left)
# define CTRL_BITS(p) ((p)->get_control_fields())

  /** Generic heap of nodes.

      The GenBinHeap class instruments a node heap. This team doesn't
      is implemented by array, but with a binary tree.
      This provides the great advantage of being highly dynamic.
      The memory used is therefore proportional to the amount of
      nodes of the HEAP.

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

  public:
    using Node = NodeType<Key>;

    Compare &key_comp() noexcept { return cmp; }

    Compare &get_compare() noexcept { return cmp; }

  private:
    Node head_node;
    Node *head;
    Node *& root;
    Node *last;
    size_t num_nodes;

  public:
    void swap(GenBinHeap & h) noexcept
    {
      std::swap(root, h.root);
      std::swap(last, h.last);
      std::swap(num_nodes, h.num_nodes);
      std::swap(cmp, h.cmp);
    }

  private:
    static bool is_in_list(Node *p) noexcept
    {
      if (IS_LEAF(p))
        return true;

      return ULINK(LLINK(p)) == RLINK(LLINK(p));
    }

    static bool has_sibling(Node *p) noexcept
    {
      return ULINK(p) != RLINK(p);
    }

    void swap_with_parent(Node *p) noexcept
    {
      assert(num_nodes >= 2);
      assert(p != root);

      Node *pp = ULINK(p); // padre de p

      const bool p_has_sibling = has_sibling(p);
      const bool p_is_in_list = is_in_list(p); // p is it in the last level?
      const bool pp_is_in_list = is_in_list(pp); // p == last & LLINK(pp) == last?
      const bool p_has_child = not IS_LEAF(p); // Does p have children?

      std::swap(CTRL_BITS(pp), CTRL_BITS(p));

      if (pp == root)
        root = p;

      Node *ppp = ULINK(pp); // abuelo de p; padre de pp

      ULINK(pp) = p; // Actualizar ULINK
      ULINK(p) = ppp;

      if (LLINK(ppp) == pp)
        LLINK(ppp) = p;
      else
        RLINK(ppp) = p;

      Node *sp = nullptr; // guarda hermano de p
      if (p_has_sibling)
        {
          sp = p == LLINK(pp) ? RLINK(pp) : LLINK(pp); // hermano de p
          assert(ULINK(sp) == pp);
          ULINK(sp) = p;
        }

      if (p == last) // ¿actualizar last?
        last = pp;

      if (num_nodes == 2)
        return;

      Node *lcp = LLINK(p); // respaldo de hijos de p
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

      if (not p_is_in_list)
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

      if (not pp_is_in_list)
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

      RLINK(lcp) = pp;
      LLINK(RLINK(pp)) = p;
      LLINK(pp) = lcp;
      RLINK(p) = RLINK(pp);
      RLINK(pp) = p;
      LLINK(p) = pp;
    }

    virtual void sift_up(Node *p) noexcept
    {
      while (p != root and cmp(KEY(p), KEY(ULINK(p))))
        swap_with_parent(p);
    }

    virtual void sift_down(Node *p) noexcept
    {
      while (not IS_LEAF(p))
        {
          Node *cp = LLINK(p); // guarda el menor hijo de p
          if (has_sibling(cp))
            if (cmp(KEY(RLINK(p)), KEY(LLINK(p))))
              cp = RLINK(p);

          if (cmp(KEY(p), KEY(cp)))
            return;

          swap_with_parent(cp);
        }
    }

    void swap_root_with_last() noexcept
    {
      assert(num_nodes > 1);
      assert(ULINK(root) == head);
      assert(not IS_LEAF(root));
      assert(IS_LEAF(last));

      if (num_nodes > 3) // caso general
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
      else // casos particulares con num_nodes < 3
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

    Node * remove_last() noexcept
    {
      assert(last != root and num_nodes > 0);
      assert(IS_LEAF(last));

      Node *ret_val = last;
      Node *pp = ULINK(last);
      Node *new_last = LLINK(last);

      if (IS_LEFT(last))
        {
          IS_LEAF(pp) = true;
          LLINK(pp) = new_last;
        }
      else
        {
          RLINK(pp) = RLINK(last);
          LLINK(RLINK(last)) = pp;
        }

      RLINK(LLINK(last)) = pp;
      last = new_last;
      num_nodes--;
      ret_val->reset();

      return ret_val;
    }

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
        {
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

    GenBinHeap(Compare __cmp = Compare()) noexcept
      : cmp(__cmp), head(&head_node), root(RLINK(head)), last(&head_node),
        num_nodes(0)
    {
      // empty
    }

    virtual ~GenBinHeap() noexcept
    { /* empty */
    }

    /** Inserts a node into a heap.

        insert(p) inserts node p into this heap.

        @param[in] p the node to insert.
        @return pointer to the inserted node.
    */
    Node * insert(Node *p) noexcept
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
      // general insertion
      Node *pp = RLINK(last); // parent of the current last
      LLINK(p) = last;
      ULINK(p) = pp;

      if (IS_LEFT(last))
        { // p will be a right child
          IS_LEFT(p) = false;
          RLINK(p) = RLINK(pp);
          LLINK(RLINK(pp)) = p;
          RLINK(pp) = p;
        }
      else
        { // p will be a left child
          IS_LEFT(p) = true;
          RLINK(p) = pp;
          IS_LEAF(pp) = false; // if p is a left child ==> pp was a leaf
          LLINK(pp) = p;
        }

      assert(not IS_LEAF(pp));

      RLINK(last) = p;
      last = p;
      num_nodes++;
      sift_up(last);
      return p;
    }

    Node * getMin_ne() noexcept
    {
      Node *ret_val = root;
      if (num_nodes == 1)
        {
          root = nullptr;
          ret_val->reset();
          num_nodes = 0;

          return ret_val;
        }

      swap_root_with_last();
      remove_last();
      sift_down(root);
      ret_val->reset();

      return ret_val;
    }

    /** Removes the node with the lowest priority from the heap.

        getMIn() extracts from this heap the node that holds the
        lowest priority value according to the comparison criterion
        defined in the declaration.

        @return pointer to the removed node.
        @throw underflow_error if the heap is empty.
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
        @see insert()
    */
    void update(Node *p) noexcept
    {
      sift_down(p);
      sift_up(p);
    }

    /** Removes node from the heap.

        remove(node) removes node from this heap.

        @param[in] node pointer to the node to remove.
        @throw underflow_error if the heap is empty
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
          insert(p);

          return node;
        }
      replace_node(node, p);
      update(p);
      node->reset();

      return node;
    }

    /// Deletes all the nodes of the heap, invokes the destructors of
    /// the removed nodes, and frees all the memory
    void remove_all_and_delete() noexcept
    {
      if (root == nullptr)
        return;

      if (num_nodes <= 3)
        {
          while (not this->is_empty())
            delete getMin_ne();
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
    static Node * advance_left(Node *p) noexcept
    {
      if (IS_LEAF(p))
        return nullptr;

      return LLINK(p);
    }

    static Node * advance_right(Node *p) noexcept
    {
      if (IS_LEAF(p))
        return nullptr;

      if (not has_sibling(LLINK(p)))
        return nullptr;

      return RLINK(p);
    }

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

      return verify_heap(right_link);
    }

  public:
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

      The BinHeap class instruments a node heap. This team doesn't
      is implemented by array, but with a binary tree.
      This provides the great advantage of being highly dynamic.
      The memory used is therefore proportional to the amount of
      nodes of the HEAP.

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

  /** Heap of nodes with virtual destroyer.

      The BinHeapVtl class instruments a node heap with a destroyer
      virtual. This team doesn't
      is implemented by array, but with a binary tree.
      This provides the great advantage of being highly dynamic.
      The memory used is therefore proportional to the amount of
      nodes of the HEAP.

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
