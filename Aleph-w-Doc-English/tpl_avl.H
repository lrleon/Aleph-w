
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
  @file tpl_avl.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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

/** 
  @brief AVL search binary tree.

  @details A AVL search binary tree is a search binary tree
  whose height is bounded to \f$O(\lg n)\f$ and its operations
  modification time bounded by \f$O(\lg n)\f$ nodes
  inspected. 

  This class is generic in the sense that manages nodes with or
  no virtual destructor. It is not intended to be used
  normally. Instead, use the derived classes or Avl_Tree
  Avl_Tree_Vtl.

  @param NodeType Node type between AvlNode and AvlNodeVtl
  (they are not documented).
  @param Key Type of key that house the tree nodes.
  @param Compare Comparison class between the keys.

  @see Avl_Tree Avl_Tree_Vtl

  @ingroup Arboles
*/
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Avl_Tree
{
public:

  typedef NodeType<Key> Node;

private:

  FixedStack<Node *> avl_stack;
  Node               head_node; 
  Node *             head_ptr;  
  Node *&            root;      
  Compare &          cmp;       

  bool avl_stack_empty() { return avl_stack.top() == head_ptr; }

  void clean_avl_stack() { avl_stack.popn (avl_stack.size() - 1); }

  Node * search_and_stack_avl(const Key &  key)
  {
    assert(avl_stack_empty());

    Node * p = root;
    do /* Descends into key search and path stacks search. */
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p))) /* Key < KEY(p)? */
          p = LLINK(p);
        else if (cmp(KEY(p), key)) /* Key > KEY(p)? */
          p = RLINK(p);
        else
          return p; /* Duplicate key */
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  Node * search_dup_and_stack_avl(const Key &  key)
  {
    assert(avl_stack_empty());

    Node * p = root;
    do /* Descends into key search and path stacks search. */
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p))) /* Key < KEY(p)? */
          p = LLINK(p);
        else /* Key >= KEY(p) */
          p = RLINK(p);
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  static Node * rotateLeft(Node * p)
  {
    assert(DIFF(p) == 2);
    assert(RLINK(p) != Node::NullPtr);

    Node * q = RLINK(p); 
    RLINK(p) = LLINK(q);
    LLINK(q) = p;

      
      
    if (DIFF(q) == 0) /* Adjustment of the factors of balance */
      { 
    /* This case occurs during the removal */                  
        DIFF(q) = -1;
        DIFF(p) = 1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  static Node * rotateRight(Node * p)
  {
    assert(DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr);

    Node * q = LLINK(p); 
    LLINK(p) = RLINK(q);
    RLINK(q) = p;

    if (DIFF(q) == 0)   /* Adjustment of the factors of balance */       
      { 
        /* This case occurs during the removal */
        DIFF(q) = 1;
        DIFF(p) = -1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  static Node * doubleRotateLeft(Node * p)
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(RLINK(p) != Node::NullPtr and LLINK(RLINK(p)) != Node::NullPtr);

    Node * q = RLINK(p);
    Node * r = LLINK(q);
    RLINK(p) = LLINK(r);
    LLINK(q) = RLINK(r);
    LLINK(r) = p;
    RLINK(r) = q;

    unsigned char b;    /* Logical height of son izq of r */
    unsigned char c;    /* Logical height of son der of r */


    /* Determining logical heights p, q and r */
    if (DIFF(r) == 1) /*  ==> c > b ==> c-b == 1 */
      {
        c = 1;
        b = 0;
      }
    else if (DIFF(r) == -1) /* ==> c < b ==> c-b = -1 */
      {
        c = 0;
        b = 1;
      }
    else
      c = b = 1;
            
    /* Adjustment of the factors of balance */
    DIFF(r) = 0; 
    DIFF(p) = b - 1; /* Logical height of son izq of p es 1 */
    DIFF(q) = 1 - c; /* Logical height of son der of q is 1 */

    return r;
  }

  static Node * doubleRotateRight(Node * p)
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr and RLINK(LLINK(p)) != Node::NullPtr);

    Node * q  = LLINK(p);
    Node * r  = RLINK(q);
    LLINK(p) = RLINK(r);
    RLINK(q) = LLINK(r);
    RLINK(r) = p;
    LLINK(r) = q;

    unsigned char b; /* Logical height of son izq of r */
    unsigned char c; /* Logical height of son der of r */

    /* Determination of children logical heights of p, q and r */
    if (DIFF(r) == 1) /* ==> c > b ==> c-b == 1 */
      {
        c = 1;
        b = 0;
      }
    else if (DIFF(r) == -1) /* ==> c < b ==> c-b == -1 */
      {
        c = 0;
        b = 1;
      }
    else
      c = b = 1;

    /* Adjustment of the factors of balance */
    DIFF(r) = 0;
    DIFF(p) = 1 - c; /* Logical height of son der of p is 1 */
    DIFF(q) = b - 1; /* Logical height of son izq of p es 1 */

    return r;
  }

/** @brief Enumeration for the rotations */
  enum Rotation_Type 
    { ROTATE_LEFT, ROTATE_RIGHT, DOUBLE_ROTATE_LEFT, DOUBLE_ROTATE_RIGHT };

  static Rotation_Type rotation_type(Node * p) 
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);

    Node * pc; /* Save the child of p */
    if (DIFF(p) == 2) /* Leftwards */
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

  static Node * restore_avl(Node * p, Node * pp)
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
        ERROR("Invalid rotation type");
        break;
      }

    return NULL;
  }

  void restore_avl_after_insertion(Node * p) /* ERROR const Key & key) */
  {
    Node * pp  = avl_stack.pop(); /* Father of the inserted node */
    if (LLINK(pp) == p) /* Adjustment factor parent node of the inserted */
      DIFF(pp)--;
    else
      DIFF(pp)++;

    if (DIFF(pp) == 0) /* In this case, height of ascendancy of pp does not increase */
      {    
        clean_avl_stack();                      
        return;
      }

    if (avl_stack_empty()) 
      return; /* pp is root */

    Node *gpp; /* Father of pp */
    do     /* Search node with factor equal to 0 */
      {
        gpp = avl_stack.pop(); 
            /* Update the factors of balance */
        if (LLINK(gpp) == pp) /* ERROR if (Compare () (key, KEY(gpp))) */
          DIFF(gpp)--;
        else
          DIFF(gpp)++;

        if (DIFF(gpp) == 0)
          break; /* No adjustment is needed */
        else if (DIFF(gpp) == -2 or DIFF(gpp) == 2)   /* Is AVL? */
          {      /* Yes ==> reset is required */
            Node *ggpp = avl_stack.pop();
            restore_avl(gpp, ggpp);
            break;
          }

        pp = gpp; /* ERROR; add */
      }
    while (not avl_stack_empty());

    clean_avl_stack();
  }

  Node * swapWithSuccessor(Node * p, Node *& pp)
  {   
    /* Reference to the top of the stack, then p will be exchanged
    successor and the position in the stack the deal's successor of p */
    Node *& ref_to_stack_top = avl_stack.top();
        
    Node *fSucc = p;        /* Father's successor */
    Node *succ  = RLINK(p); /* Search begins from RLINK(p) */

    avl_stack.push(succ);

    /* Find a successor at a time while updates the stack */
    while (LLINK(succ) != Node::NullPtr) /* Down as far to the left */
      {
        fSucc = succ;
        succ  = LLINK(succ);
        avl_stack.push(succ);
      }

    /* Upgrade old post of stack occupied by p. Equivalent
    to exchange old top (before seeking suc) with current */
    ref_to_stack_top = succ; 
    avl_stack.top()  = p;   
    if (LLINK(pp) == p) /* Upgrade the new son of pp (successor) */
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p); /* Swap the left branches */
    LLINK(p)    = Node::NullPtr; 
    if (RLINK(p) == succ) /* Update right branches */
      { /* Successor is exactly the right child of p */
        RLINK(p)    = RLINK(succ); 
        RLINK(succ) = p;
        pp          = succ;
      }
    else
      { /* Successor is the descendant to the left of RLINK(p) */
        Node *succr  = RLINK(succ); 
        RLINK(succ)  = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p)     = succr;
        pp           = fSucc;
      }

    DIFF(succ) = DIFF(p); /* Exchange the factors of balance */

    return succ;
  }

  void restore_avl_after_deletion(bool left_deficit) /* ERROR const Key & key) */
  {
    Node * pp  = avl_stack.top(1);  /* Parent of p */
    Node * ppp = avl_stack.popn(3); /* Eliminates of stack p, parent and grandfather */
    while (true)
      {    /* Update the factors of balance */
        if (left_deficit) /* ERROR Compare () (key, KEY(pp))) */
          DIFF(pp)++;
        else
          DIFF(pp)--;

        if (DIFF(pp) == -2 or DIFF(pp) == 2) /* It is valid? */
          pp = restore_avl(pp, ppp); /* No! */

        if (DIFF(pp) != 0 or pp == root)
          break; /* Overall height of the tree has not changed ==> finish */

        left_deficit = LLINK(ppp) == pp;
        pp  = ppp; /* Advance to the next Ascending */
        ppp = avl_stack.pop();
      }

      clean_avl_stack();
    }

public:

/** @brief The type of key that contains the node. */
  typedef Key key_type;

/** @brief Returns a reference to the criterion of comparison. */
  Compare & key_comp() { return cmp; }

/** @brief @overload key_comp() */
  Compare & get_compare() { return key_comp(); }

/** @brief Instance an AVL tree generic. */
  Gen_Avl_Tree(Compare && __cmp) 
    : avl_stack(Node::MaxHeight), head_ptr(&head_node), 
      root(RLINK (head_ptr)), cmp(std::forward<Compare&>(__cmp))
  {
    avl_stack.push(head_ptr);
  }

  Gen_Avl_Tree(Compare & __cmp) 
    : avl_stack(Node::MaxHeight), head_ptr(&head_node), 
      root(RLINK (head_ptr)), cmp(__cmp)
  {
    avl_stack.push(head_ptr);
  }

/** 
  @brief Exchanges all elements of treap this with tree treap
  in constant time (and very fast).

  @param[in] tree Treap to exchange with this.
*/
  void swap(Gen_Avl_Tree & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

/** @brief Destroy a AVL tree generic. */
  virtual ~Gen_Avl_Tree() { assert(avl_stack_empty()); }

/** @brief Gets a pointer to the root of the tree. */
  Node *& getRoot() { return root; }

/** @brief Search the specified key in the AVL tree. Returns a pointer to the node
containing the searched key; NULL if the key is not found. */

  Node * search(const Key & key) const 
  { 
    return searchInBinTree <Node, Compare> (root, key, cmp); 
  }

/** @brief Inserts the node P in the search binary tree AVL. Returns p 
if tree does not contain another node with the same key p; NULL otherwise. */
  Node * insert(Node * p)
  {
    if (root == Node::NullPtr)
      return root = p;

    Node *pp = search_and_stack_avl(KEY(p));
    if (cmp (KEY(p), KEY(pp))) 
      LLINK (pp) = p;
    else if (cmp (KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      { /* Duplicate key */
        clean_avl_stack();
        return NULL;  
      }

    restore_avl_after_insertion(p);

    return p;
  }

/** 
  @brief Search the p key in the AVL tree or inserts
  if not found.

  @details search_or_insert(p) searches AVL tree a node whose key
  is <code>KEY(p)</code>. If the key is found, then returns a pointer 
  to the node that hosts it. Otherwise p inserted into the search 
  binary tree this. 

  @param[in] p Node to find or insert.
  
  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, it returns a pointer 
  to the node in the tree containing <code>KEY(p)</code>.
*/
  Node * search_or_insert(Node * p)
  {
    if (root == Node::NullPtr)
      return root = p;

    Node *pp = search_and_stack_avl(KEY(p));
    if (cmp (KEY(p), KEY(pp))) 
      LLINK (pp) = p;
    else if (cmp (KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      { /* Duplicate key */
        clean_avl_stack();
        return pp;  
      }

    restore_avl_after_insertion(p); 

    return p;
  }

/** @brief Inserts the p node in the binary tree AVL search with
replicability. */
  Node * insert_dup(Node * p)
  {
    if (root == Node::NullPtr)
      return root = p;

    Node *pp = search_dup_and_stack_avl(KEY(p));
    if (cmp (KEY(p), KEY(pp))) 
      LLINK (pp) = p;
    else
      RLINK(pp) = p;

    restore_avl_after_insertion(p); 

    return p;
  }

/** @brief Remove from a AVL tree the node that contains the key "key".
Returns the removed node if it is the key; NULL otherwise. */
  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return NULL;

    Node * p = search_and_stack_avl(key);
    if (no_equals<Key, Compare> (KEY(p), key, cmp))
      {     /* Key was not found */
        clean_avl_stack();
        return NULL;
      }

    Node * pp = avl_stack.top(1); /* get parent of p */
    bool left_deficit; /* ERROR Key removed_key = KEY(p); */
    while (true) 
      {
        left_deficit = LLINK(pp) == p; 

    /** Incomplete on the left?
    If the answer is YES, attach to pp the son of p */
        if (LLINK(p) == Node::NullPtr) 
          {      
            if (LLINK(pp) == p)
              LLINK(pp) = RLINK(p);
            else
              RLINK(pp) = RLINK(p); 
            break;
          }
    /** Incomplete on the left?
    If the answer is YES, attach to pp the son of p */
        if (RLINK(p) == Node::NullPtr) 
          {     
            if (LLINK(pp) == p)
              LLINK(pp) = LLINK(p);
            else
              RLINK(pp) = LLINK(p); 
            break;
          }

    /* Here p is a complete node ==> exchange by successor */
        swapWithSuccessor(p, pp);
    Removed_key = KEY(succ); /* ERROR remove */
      }

     p->reset();

    if (pp == head_ptr) /* Check the root was removed */
      { 

    /* Factors remain unchanged ==> no condition is violated AVL */
        clean_avl_stack();
        return p;
      }

    restore_avl_after_deletion(left_deficit); /* ERROR */

    return p;
  }

  bool verify()
  {
    return is_avl(root);
  }

};
   
/** 
  @brief Binary tree AVL search with nodes without virtual destructor.

  @details A binary tree of AVL search is a binary tree search
  whose height is bounded to \f$O(\lg n)\f$ and its operations of 
  modification time bounded by \f$O(\lg n)\f$ nodes inspected.

  This class handles nodes without virtual destructor.

  @param Key Type of key that house the tree nodes.
  @param Compare class of comparison between the keys; by
  default, this class is the least relational operator for
  Key type.

  @see Avl_Tree_Vtl

  @ingroup Arboles
*/
    template <typename Key, class Compare = Aleph::less<Key>>
class Avl_Tree : public Gen_Avl_Tree<AvlNode, Key, Compare> 
{
public:

  Avl_Tree(Compare && cmp = Compare()) 
    : Gen_Avl_Tree<AvlNode, Key, Compare> (std::move(cmp))
  {
    /* Empty */
  }

  Avl_Tree(Compare & cmp) 
    : Gen_Avl_Tree<AvlNode, Key, Compare>(cmp)
  {
    /* Empty */
  }
};

/** 
  @brief Binary tree AVL search with virtual destructor in their nodes.

  @details A binary tree of AVL search is a binary tree search
  whose height is bounded to \f$O(\lg n)\f$ and its operations of 
  modification time bounded by \f$O(\lg n)\f$ nodes inspected.

  This class handles nodes with virtual destructor.

  @param Key Type of key that house the tree nodes.
  @param Compare Class of comparison between the keys; by
  default, this class is the least relational operator for
  Key type.

  @see Avl_Tree

  @ingroup Arboles
*/
    template <typename Key, class Compare = Aleph::less<Key>>
class Avl_Tree_Vtl : public Gen_Avl_Tree<AvlNodeVtl, Key, Compare> 
{
public:

  Avl_Tree_Vtl(Compare && cmp = Compare()) 
    : Gen_Avl_Tree<AvlNodeVtl, Key, Compare> (std::move(cmp))
  {
    /* Empty */
  }

  Avl_Tree_Vtl(Compare & cmp) 
    : Gen_Avl_Tree<AvlNodeVtl, Key, Compare>(cmp)
  {
    /* Empty */
  }
};

} /* End namespace Aleph */

# endif /* TPL_AVL_H
