
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_binHeap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BINHEAP_H
# define TPL_BINHEAP_H

# include <ahDefs.H>
# include <ahUtils.H>
# include <ahFunction.H>
# include <tpl_binNode.H>
# include <tpl_dynListQueue.H>

namespace Aleph {

class BinHeapNode_Data
{
/** @brief Definition of control flags */
  struct Control_Fields 
  {
    int is_leaf : 4; /* True if the node is a sheet */
    int is_left : 4; /* True if the node is left child */
  };
  

  BinHeapNode_Data * pLink; /* Pointer to the parent */
  Control_Fields control_fields;

public:

  BinHeapNode_Data() : pLink(NULL)
  { 
    control_fields.is_leaf = true;
    control_fields.is_left = true;
  }

  BinHeapNode_Data *& getU() { return pLink; }

  Control_Fields & get_control_fields() { return control_fields; }

  void reset()
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

/** 
  @brief Heap generic of node.

  @details The GenBinHeap class implements one heap of nodes. This heap no
  is implemented using an array, but a binary tree.
  This provides the advantage of being highly dynamic.
  The memory in use is therefore proportional to the amount of
  heap nodes.

  This class is not intended for public use. Its purpose is to provide
  basic functionality to BinHeap, BinHeapVtl and DynBinHeap
  classes.

  @param NodeType Node type that uses the heap; it will be with 
  or without virtual destructor.
  @param Key The key that saves each node.
  @param Compare Standard of comparison between the nodes keys.

  @see BinHeap BinHeapVtl DynBinHeap

  @ingroup Arboles
*/
  template <template <class> class NodeType, typename Key, 
            class Compare = Aleph::less<Key>>
class GenBinHeap
{
  Compare & cmp;

public:

  typedef NodeType<Key> Node;

  Compare & key_comp() { return cmp; }

  Compare & get_compare() { return cmp; }

private:

    Node     head_node;
    Node *   head;
    Node *&  root;
    Node *   last;
    size_t   num_nodes;

public:

  void swap(GenBinHeap & h)
  {
    std::swap(root, h.root);
    std::swap(last, h.last);
    std::swap(num_nodes, h.num_nodes);
    std::swap(cmp, h.cmp);
  }

private:

  static bool is_in_list(Node * p)
  {
    if (IS_LEAF(p)) 
      return true;

    return ULINK(LLINK(p)) == RLINK(LLINK(p));
  }

  static bool has_sibling(Node * p)
  {
    return ULINK(p) != RLINK(p);
  }

  void swap_with_parent(Node * p)
  {
    assert(num_nodes >= 2);
    assert(p != root);

    Node *pp = ULINK(p); /* Parent of p */

    const bool p_has_sibling = has_sibling(p); 
    const bool p_is_in_list  = is_in_list(p);  /* p is in last level? */
    const bool pp_is_in_list = is_in_list(pp); /* p == last & LLINK(pp) == last? */
    const bool p_has_child   = not IS_LEAF(p); /* p have children? */

    std::swap(CTRL_BITS(pp), CTRL_BITS(p)); /* Swap flags */

    if (pp == root) 
      root = p;

    Node *ppp = ULINK(pp); /* Grandfather of p; parent of pp */

    ULINK(pp) = p;  /* Update ULINK */
    ULINK(p) = ppp;

    if (LLINK(ppp) == pp)
      LLINK(ppp) = p;
    else
      RLINK(ppp) = p;

    Node *sp = NULL;  /* Guarda hermano de p */
    if (p_has_sibling) 
      {
        sp = p == LLINK(pp) ? RLINK(pp) : LLINK(pp); /* Hermano de p */
        assert(ULINK(sp) == pp);
        ULINK(sp) = p;
      }

    if (p == last) /* Update last? */
      last = pp;
    
    if (num_nodes == 2) 
      return;

    Node *lcp = LLINK(p); /* Support of children of p */
    Node *rcp = RLINK(p);
   
    if (num_nodes == 3)
      {
        if (RLINK(pp) == p)
          {
            LLINK(lcp) = RLINK(lcp) = pp;
            RLINK(pp)  = lcp;
            RLINK(p)   = pp;
          }
        else
          {
            LLINK(rcp) = RLINK(rcp) = pp;
            LLINK(pp)  = rcp;
            LLINK(p)   = pp;
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
            RLINK(p)  = pp;
            LLINK(p)  = LLINK(pp);
          }

        LLINK(pp) = lcp;
        RLINK(pp) = rcp;

        return;
      }
      
    RLINK(lcp)       = pp;
    LLINK(RLINK(pp)) = p;
    LLINK(pp)        = lcp;
    RLINK(p)         = RLINK(pp);
    RLINK(pp)        = p;
    LLINK(p)         = pp;
  }

  virtual void sift_up(Node * p)
  {
    while (p != root and cmp (KEY(p), KEY(ULINK(p))))
      swap_with_parent(p);
  }

  virtual void sift_down(Node * p)
  {
    while (not IS_LEAF(p))
      {
        Node * cp = LLINK(p);  /* Save the youngest son of p */
        if (has_sibling(cp))
          if (cmp (KEY(RLINK(p)), KEY(LLINK(p))))
            cp = RLINK(p);

        if (cmp (KEY(p), KEY(cp))) 
          return;

        swap_with_parent(cp);
      }
  }

  void swap_root_with_last()
  {
    assert(num_nodes > 1);
    assert(ULINK(root) == head);
    assert(not IS_LEAF(root));
    assert(IS_LEAF(last));

    if (num_nodes > 3) /* General case */
      {
        Node * lRoot     = LLINK(root);
        Node * rRoot     = RLINK(root);
        Node * f_last    = ULINK(last);
        Node * prev_last = LLINK(last);
        Node * next_last = RLINK(last);

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
    else if (num_nodes == 3) /* Special case with 3 nodes */
      {
        assert(RLINK(root) == last);
        assert(LLINK(last) == LLINK(root) and RLINK(last) == LLINK(root));

        ULINK(last) = ULINK(root);
        ULINK(root) = last;

        Node *s_last  = LLINK(last);
        ULINK(s_last) = last;

        LLINK(last) = s_last;
        RLINK(last) = root;

        LLINK(root) = RLINK(root) = s_last;
        RLINK(s_last) = LLINK(s_last) = root;
      }
    else /* Special cases with num_nodes < 3 */
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

  Node * remove_last()
  {
    assert(last != root and num_nodes > 0);
    assert(IS_LEAF(last));

    Node * ret_val  = last;
    Node * pp       = ULINK(last);
    Node * new_last = LLINK(last);

    if (IS_LEFT(last))
      {
        IS_LEAF(pp) = true;
        LLINK(pp)   = new_last;
      }
    else
      {
        RLINK(pp)          = RLINK(last);
        LLINK(RLINK(last)) = pp;
      }

    RLINK(LLINK(last)) = pp;
    last = new_last;
    num_nodes--;
    ret_val->reset();

    return ret_val;
  }

  void replace_node(Node * node, Node * new_node)
  {
    assert(node != new_node);
    assert(node != last);

    /* Save the immediate relatives of node */
    Node * parent = ULINK(node);
    Node * left_child  = LLINK(node);
    Node * right_child = RLINK(node);
      
    /* Update pointers belonging to new_node */
    ULINK(new_node) = parent;
    LLINK(new_node) = left_child;
    RLINK(new_node) = right_child;

    /* Update parent */
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

    /* Update children */
    if (IS_LEAF(node))
      {
        RLINK(left_child)  = new_node;
        LLINK(right_child) = new_node;
      }
    else 
      {
        ULINK(left_child) = new_node;

    /* Node may have only one child */
        if (ULINK(right_child) == node) 
          ULINK(right_child) = new_node;
        else
          {
            assert(left_child == last);
            RLINK(left_child)  = new_node;
            LLINK(right_child) = new_node;
          }
      }
      
    CTRL_BITS(new_node) = CTRL_BITS(node);
  }

  static void __postorder_delete(Node * p, Node * incomplete_node)
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

  Node * getRoot() { return root; }

  Node * getRoot() const { return const_cast<Node*>(root); }

  template <class Op>
  void for_each_in_preorder(Node * p, Op & op)
  {
    if (p == NULL)
      return;

    op(p);

    for_each_in_preorder(advance_left(p), op);
    for_each_in_preorder(advance_right(p), op);    
  }

  template <class Op>
  void for_each_in_preorder(Node * p, Op && op())
  {
    return for_each_in_preorder<Op>(p, op);
  }

private:

  template <class Op>
  bool __level_traverse(Node * root, Op & operation)
  {
    if (root == NULL)
      return true;

    DynListQueue<Node*> queue; 
    queue.put(root);

    while (not queue.is_empty())
      {
	Node * p = queue.get();

	if (not operation(p))
	  return false;

	Node * c = advance_left(p);
	if (c == NULL)
	  continue;

	queue.put(c);

	c = advance_right(p);
	if (c != NULL)
	  queue.put(c);
      }

    return true;
  }

public:

  template <class Op>
  bool level_traverse(Node * root, Op & operation)
  {
    return __level_traverse(root, operation);
  }

  template <class Op>
  bool level_traverse(Node * root, Op && operation = Op()) 
  {
    return __level_traverse(root, operation);
  }

  template <class Op>
  bool level_traverse(Node * root, Op & operation) const
  {
    return const_cast<GenBinHeap&>(*this).__level_traverse(root, operation);
  }

  template <class Op>
  bool level_traverse(Node * root, Op && operation = Op()) const
  {
    return const_cast<GenBinHeap&>(*this).__level_traverse(root, operation);
  }

  GenBinHeap(Compare & __cmp) 
    : cmp(__cmp), head(&head_node), root(RLINK(head)), last(&head_node), 
      num_nodes(0) 
  {
    /* Empty */
  }

  GenBinHeap(Compare && __cmp = Compare()) 
    : GenBinHeap(__cmp)
  {
    /* Empty */
  }

  virtual ~GenBinHeap() { /* Empty */ }

/** 
  @brief Inserts a node in a heap.

  @details insert(p) Insert into the heap this p node.

  @param[in] p Node to insert.

  @return A pointer the node to inserted.
*/
  Node * insert(Node * p)
  {
    assert(IS_LEAF(p));

    /** Heap empty?
    If the answer is YES, then initialize */
    if (root == NULL) 
      { 

        assert(num_nodes == 0);

        root       = p;
        LLINK(p)   = RLINK(p) = p;
        ULINK(p)   = head;
        IS_LEAF(p) = true;
        IS_LEFT(p) = false; /* Root is right child of header node */
        last      = root;
        num_nodes = 1;
        return p;
      }
    /* General insertion */
    Node * pp = RLINK(last); /* Parent current last */
    LLINK(p) = last;
    ULINK(p) = pp;

    if (IS_LEFT(last)) 
      { /* p will the right child */
        IS_LEFT(p)       = false;
        RLINK(p)         = RLINK(pp);
        LLINK(RLINK(pp)) = p;
        RLINK(pp)        = p;
      }
    else
      { /* p will left child */
        IS_LEFT(p) = true;
        RLINK(p)   = pp;
        IS_LEAF(pp) = false; /* If p is left ==> pp was sheet */
        LLINK(pp)   = p;
      }

    assert(not IS_LEAF(pp));

    RLINK(last) = p;
    last        = p;
    num_nodes++;
    sift_up(last);
    return p;
  }

/** 
  @brief Eliminates of heap node lower priority.

  @details getMIn() extract from the heap this the node containing the smallest
  priority value according to the comparison criteria defined
  in the statement.

  @return A pointer to the node removed.
  
  @throw underflow_error If the heap is empty.
*/
  Node * getMin() throw(std::exception, std::underflow_error)
  {
    if (root == NULL)
      throw std::underflow_error ("Heap is empty");

    Node *ret_val = root;

    if (num_nodes == 1)
      {
        root = NULL;
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

/** @overload getMin() to use if the priority order is reversed. */
  Node * getMax() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** 
  @brief Upgrade priority of a node contained in the heap.

  @details update(p) take a heap node whose priority has been modified
  and updates its priority within the heap.
  The idea is that if for some reason a priority should be
  modified, then the order of extraction can be updated.

  @param[in] p Pointer to the node you want to upgrade

  @see insert()
*/
  void update(Node * p)
  {
    sift_down(p);
    sift_up(p);
  }

/** 
  @brief Removes of heap the "node" node.

  @details remove(node) remove from heap node the "node" node.

  @param[in] node Pointer to the node to remove.

  @return A pointer to the node removed.
  
  @throw underflow_error If the heap is empty.
  
  @note It does not check whether node belongs to the heap.
*/
  Node * remove(Node * node) throw(std::exception, std::underflow_error)
  {
    if (root == NULL)
      throw std::underflow_error ("Heap is empty");

    if (node == root) 
      return getMin();

    if (node == last) 
      return remove_last();

    Node * p = remove_last();

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

/** @brief Deletes all nodes in the heap, invokes destroyers
nodes removed and frees all memory. */
  void remove_all_and_delete()
  {
    if (root == NULL) 
      return;

    if (num_nodes <= 3)
      {
        while (not this->is_empty()) 
          delete getMin(); 
        return;
      }

    if (IS_LEFT(last))
      __postorder_delete(root, ULINK(last));
    else
      __postorder_delete(root, NULL);

    root = NULL; /* Restart as if builder */
    last = &head_node;
    num_nodes = 0;
  }

/** @brief Returns the node with the lowest priority at the discretion of
comparation specified in the declaration. */
  Node * top() const throw(std::exception, std::underflow_error)
  {
    if (root == NULL)
      throw std::underflow_error ("Heap is empty");

    return root;
  }

/** @brief Returns the cardinality of the heap. */
  const size_t & size() const { return num_nodes; }

/** @brief Returns true if the heap is empty. */
  bool is_empty() const { return size() == 0; }

  protected:

  Node * advance_left(Node * p)
  {
    if (IS_LEAF(p)) 
      return NULL;

    return LLINK(p);
  }

  Node * advance_right(Node * p)
  {
    assert(not IS_LEAF(p));

    if (not has_sibling(LLINK(p))) 
      return NULL;

    return RLINK(p);
  }

  virtual bool verify_heap(Node * p)
  {
    Node * left_link = advance_left(p);

    if (left_link == NULL)
      {
        assert(IS_LEAF(p));

        return true;
      }

    if (cmp (KEY(left_link), KEY(p))) 
      return false;

    Node * right_link = advance_right(p);

    if (right_link == NULL) 
      return verify_heap(left_link);

    if (cmp (KEY(right_link), KEY(p))) 
      return false;

    return verify_heap(right_link);
  }

  public:

  bool verify_heap()
  {
    if (root == NULL) 
      return true;

    return verify_heap(root);
  }
};

/** 
  @brief Heap of nodes without virtual destructor.

  @details The BinHeap class implements one heap of nodes. This heap no
  is implemented using an array, but a binary tree.
  This provides the advantage of being highly dynamic.
  The memory in use is therefore proportional to the amount of heap nodes.

  @param Key The key that saves each node.
  @param Compare Standard of comparison between the nodes keys; 
  default is the "less than" relationship.

  @see BinHeapVtl DynBinHeap

  @ingroup Arboles
*/
    template <class Key, typename Compare = Aleph::less<Key> >
struct BinHeap : public GenBinHeap<BinHeapNode, Key, Compare>
{
/** @brief The type of node heap. */
  typedef BinHeapNode<Key> Node;

  using GenBinHeap<BinHeapNode, Key, Compare>::GenBinHeap;
};

/** 
  @brief Heap of nodes with virtual destructor.

  @details The BinHeapVtl class implements one heap of nodes with destructor
  virtual. This heap is implemented using an array, but with a binary tree.
  This provides the advantage of being highly dynamic.
  The memory in use is therefore proportional to the amount of heap nodes.

  @param Key The key that saves each node.
  @param Compare Standard of comparison between the nodes keys; 
  default is the "less than" relationship.

  @see BinHeap DynBinHeap

  @ingroup Arboles
*/
    template <class Key, typename Compare = Aleph::less<Key> >
struct BinHeapVtl : public GenBinHeap<BinHeapNodeVtl, Key, Compare>
{
/** @brief The type of heap node. */
  typedef BinHeapNodeVtl<Key> Node;

  using GenBinHeap<BinHeapNodeVtl, Key, Compare>::GenBinHeap;
};

# undef PREV
# undef NEXT
# undef ULINK
# undef IS_LEAF
# undef IS_LEFT
# undef CTRL_BITS

} /* End namespace Aleph */

# endif //  TPL_BINHEAP_H
