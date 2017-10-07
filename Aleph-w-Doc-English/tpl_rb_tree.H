
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
  @file tpl_rb_tree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_RB_TREE_H
# define TPL_RB_TREE_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeUtils.H>
# include <rbNode.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Red-black search binary tree.

  @details A Red-black search binary tree is a search binary tree
  whose height is bounded to \f$O(\lg n)\f$ and its operations
  modification time bounded by \f$O(\lg n)\f$ nodes
  inspected.

  This class is generic in the sense that manages nodes with or
  no virtual destructor. It is not intended to be used
  normally. Instead, use the derived classes or Rb_Tree
  Rb_Tree_Vtl.

  @param NodeType Node type between RbNode and RbNodeVtl
  (they are not documented).
  @param Key Type of key that house the tree nodes.
  @param Compare Comparison class between the keys.

  @see Rb_Tree Rb_Tree_Vtl

  @ingroup Arboles
*/
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Rb_Tree 
{
public:

  typedef NodeType<Key> Node;

private:

  Node              head_node; /* Cabecera centinela */
  Node *            head;      /* Puntero a centinela */
  Node *&           root; 
  FixedStack<Node*> rb_stack; 
  Compare &         cmp;

  Node * search_and_stack_rb(const Key & key)             
  {
    Node * p = root; 
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
    Node * p = root; 
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

  void fix_red_condition(Node * p)
  {
    assert(COLOR(p) == RED);

    while (p != root) 
      {
        Node * pp = rb_stack.pop(); /* Father of p */

    /** Father of "p" is black?
    If the answer is YES, then no consecutive red and end */
        if (COLOR(pp) == BLACK) 
          break;

    /** "p" is a direct child of the root?
    If the answer is YES, then color black root and end */
        if (root == pp) 
          {
            COLOR(root) = BLACK; 
            break; 
          }

        Node * ppp = rb_stack.pop(); /* Abuelo de p */
        Node * spp = LLINK(ppp) == pp ? RLINK(ppp) : LLINK(ppp); /* Tío */

    /**¿Uncle of "p" is red?
    If the answer is YES, then swap colors between levels */
        if (COLOR(spp) == RED) 
          {
            COLOR(ppp) = RED;
            COLOR(pp)  = BLACK;
            COLOR(spp) = BLACK;
            p = ppp;
            continue; /* Ir próximo ancestro, verificar violaciones */
          }

        Node * pppp = rb_stack.pop(); /* Bisabuelo de p */
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
        break; /* Arbol es rojo-negro ==> terminar */
      }

     rb_stack.empty();
  }

  void find_succ_and_swap(Node * p, Node *& pp)
  {
    Node *& ref_rb_stack = rb_stack.top(); 

    /* Find successor while updating rb_stack */
    Node * fSucc = p;        /* Successor's parent */
    Node * succ  = RLINK(p); /* Searching starts from p's right child */
    rb_stack.push(succ);

    while (LLINK(succ) != Node::NullPtr)  /* Go down to leftmost */
      {
        fSucc = succ;
        succ  = LLINK(succ);
        rb_stack.push(succ);
      }

    ref_rb_stack   = succ; /* Swap old top with current top */
    rb_stack.top() = p; 

    if (LLINK(pp) == p) /* Setting of parent of p to new child(succ) */ 
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p); /* Swaps left branches */
    LLINK(p)    = Node::NullPtr; 

    if (RLINK(p) == succ) /* For right branches there are two cases */
      { /* Successor is just right child of p */   
	RLINK(p)    = RLINK(succ); 
	RLINK(succ) = p;
	pp          = succ;
      }
    else
      { /* Successor is leftmost node descending from right child of p */ 
	Node *succr  = RLINK(succ); 
	RLINK(succ)  = RLINK(p);
	LLINK(fSucc) = p;
	RLINK(p)     = succr;
	pp           = fSucc;
      }

    std::swap(COLOR(succ), COLOR(p));
  }

  void fix_black_condition(Node * p)
  {

    /** "p" is red?
    If the answer is YES, then we painted it black and ended */
    if (COLOR(p) == RED)
      {
        COLOR(p) = BLACK; /* Esto compensa el déficit  */
        return;
      }

    Node * pp = rb_stack.popn(2); /* Father of p */
    while (p != root)
      {
        assert(LLINK(pp) == p or RLINK(pp) == p);
        assert(LLINK(rb_stack.top()) == pp or RLINK(rb_stack.top()) == pp);

        Node * sp = LLINK(pp) == p ? RLINK(pp) : LLINK(pp); /* Hermano p */
        if (COLOR(sp) == RED) /* ¿Hermano de p es rojo? */
          {
            Node *& ppp = rb_stack.top(); /* abuelo de p */

            if (LLINK(pp) == p)
              {
                sp  = LLINK(sp);
                ppp = rotate_to_left(pp, ppp);
              }
            else
              {
                sp  = RLINK(sp);
                ppp = rotate_to_right(pp, ppp);
              }

            COLOR(ppp) = BLACK;
            COLOR(pp)  = RED;
          }

        Node * np, * snp; /* Sobrinos de nodo p */

    /** "p" is left child?
    If the answer is YES, it means that "sp" is right child */
        if (LLINK(pp) == p) 
          {
            np  = RLINK(sp);
            snp = LLINK(sp);
          }
        else
          {
            np  = LLINK(sp);
            snp = RLINK(sp);
          }

        if (COLOR(np) == RED) /* ¿np es rojo? */
          {
            Node * ppp = rb_stack.top();

            if (RLINK(sp) == np)
              rotate_to_left(pp, ppp);
             else
              rotate_to_right(pp, ppp);

            COLOR(sp) = COLOR(pp);
            COLOR(pp) = BLACK;
            COLOR(np) = BLACK;

            return;
          }

        if (COLOR(snp) == RED) /* ¿snp es rojo? */
          {
            Node * ppp = rb_stack.top();

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
            COLOR(pp)  = BLACK;;

            return;
          }

        if (COLOR(pp) == RED) /* ¿pp es rojo? */
          {
            COLOR(pp) = BLACK;
            COLOR(sp) = RED;;
            return;
          }

    /* No hay nodo rojo en las adyacencias de p, entonces desplazar el
    déficit hacia pp y repetir la iteración */
        COLOR(sp) = RED;
        p         = pp;
        pp        = rb_stack.pop(); 
      }
  }


public:

/** @brief The type of key that contains the node */
  typedef Key key_type;

/** @brief Returns a reference to the comparison criterion */
  Compare & key_comp() { return cmp; }

/** @overload key_comp() */
  Compare & get_compare() { return key_comp(); }

/** @brief Instancia un árbol rojo-negro. */
  Gen_Rb_Tree(Compare && __cmp) 
    : rb_stack(Node::MaxHeight), head(&head_node), 
      root(head_node.getR()), cmp(__cmp)
  {
    /* Empty */ 
  }

  Gen_Rb_Tree(Compare & __cmp) 
    : rb_stack(Node::MaxHeight), head(&head_node), 
      root(head_node.getR()), cmp(__cmp)
  {
    /* Empty */ 
  }

/** 
  @brief Exchanges all elements of treap this with tree treap
  in constant time (and very fast).

  @param[in] tree Treap to exchange with this
*/
  void swap (Gen_Rb_Tree & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

/** @brief Destroy a red-black tree. */
  virtual ~Gen_Rb_Tree() { /* Empty */ }

/** @brief Looking for a node with "key" key.
Returns a pointer to the node contentivo of "key" if it is in the tree;
NULL otherwise. */
  Node * search(const Key & key)
  {
    Node * retVal = Aleph::searchInBinTree <Node, Compare> (root, key, cmp);
    return retVal == Node::NullPtr ? NULL : retVal;
  }

/** @brief Gets a pointer to the root of the tree. */
  Node *& getRoot() { return root; }
      
/** 
  @brief Insert the "p" node in the red-black tree. 

  @details If the "p" key is not contained in the tree,
  then the node is inserted and the cursor returns to "p".
  Otherwise no insertion occurs and NULL is returned.
*/
  Node * insert(Node * p)
  {
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr) 
      return root = p; /* Inserción en árbol vacío */

    Node * q = search_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q))) 
      LLINK(q) = p;
    else if (cmp(KEY(q), KEY(p))) 
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return NULL;  /* Duplicate key */
      }
    fix_red_condition(p);

    return p;
  }

/** 
  @brief Search the "p" key in the red black tree or inserts
  if not found.

  @details search_or_insert(p) searches red-black tree a node whose key
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
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr) 
      return root = p; /* Inserción en árbol vacío */

    Node * q = search_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q))) 
      LLINK(q) = p;
    else if (cmp(KEY(q), KEY(p))) 
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return q;  /* Duplicate key */
      }
    fix_red_condition(p);

    return p;
  }

/** 
  @brief Insert the "p" node in the red-black tree.

  @details If the "p" key is not contained in the tree,
  then the node is inserted and the cursor returns to "p".
  Otherwise no insertion occurs and NULL is returned.
*/
  Node * insert_dup(Node * p)
  {
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr) 
      return root = p; /* Inserción en árbol vacío */

    Node * q = search_dup_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q))) 
      LLINK(q) = p;
    else 
      RLINK(q) = p;

    fix_red_condition(p);

    return p;
  }

  bool verify() { return is_red_black_tree(root) ; }

  /** @brief Removes the contentivo node the "key" key.
  
  @details If the key is in the tree,
  then the node that contains the address is returned and removed.
  Otherwise NULL is returned.
*/
  Node* remove(const Key & key)
  {
    if (root == Node::NullPtr) 
      return NULL;

    Node * q = search_and_stack_rb(key);
    if (no_equals<Key, Compare> (KEY(q), key, cmp)) /* ¿Clave no encontrada? */
      {    
        rb_stack.empty();
        return NULL;
      }

    Node * pq = rb_stack.top(1); /* Padre de 1 */
    Node * p; /* Hijo de q luego de que éste ha sido eliminado */

    /* Eliminación clásica árbol binario de búsqueda */
    while (true) 
      {
        if (LLINK(q) == Node::NullPtr) 
          {
            if (LLINK(pq) == q)
              p = LLINK(pq) = RLINK(q);
            else
              p = RLINK(pq) = RLINK(q);

            break; /* goto end; */
          }

        if (RLINK(q) == Node::NullPtr) 
          {
            if (LLINK(pq) == q)
              p = LLINK(pq) = LLINK(q);
            else
              p = RLINK(pq) = LLINK(q);

            break; /* goto end; */
          }

        find_succ_and_swap(q, pq);
      }

    if (COLOR(q) == BLACK) /* ¿Se eliminó un nodo negro? */
      fix_black_condition(p);

    q->reset();
    rb_stack.empty();

    return q;
  }
};
    
/** 
  @brief Binary tree of search red-black with nodes without virtual destructor.

  @details A binary tree search is a red black binary tree search
  whose height is bounded to \f$O(\lg n)\f$ and its operations
  modification time bounded by \f$O(\lg n)\f$ nodes
  inspected.

  This class handles nodes without virtual destructor.

  @param Key Type of key that house the tree nodes.
  @param Compare Comparison class between the keys.

  @see Rb_Tree_Vtl

  @ingroup Arboles
*/
    template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree : public Gen_Rb_Tree<RbNode, Key, Compare> 
{
public:

  Rb_Tree(Compare && cmp = Compare()) 
    : Gen_Rb_Tree<RbNode, Key, Compare> (std::move(cmp))
  {
    /* Empty */
  }

  Rb_Tree(Compare & cmp) 
    : Gen_Rb_Tree<RbNode, Key, Compare> (cmp)
  {
    /* Empty */
  }
};

/** 
  @brief Binary tree of search red-black with virtual destructor in their nodes.

  @details A binary tree search is a red black binary tree search
  whose height is bounded to \f$O(\lg n)\f$ and its operations
  modification time bounded by \f$O(\lg n)\f$ nodes
  inspected. 

  This class handles nodes with virtual destructor.

  @param Key Type of key that house the tree nodes.
  @param Compare Comparison class between the keys.

  @see Rb_Tree_Vtl

  @ingroup Arboles
*/
    template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree_Vtl : public Gen_Rb_Tree<RbNodeVtl, Key, Compare> 
{
public:

  Rb_Tree_Vtl(Compare && cmp = Compare())
    :  Gen_Rb_Tree<RbNodeVtl, Key, Compare> (std::move(cmp))
  {
    /* Empty */
  }

  Rb_Tree_Vtl(Compare & cmp)
    :  Gen_Rb_Tree<RbNodeVtl, Key, Compare> (cmp)
  {
    /* Empty */
  }
};

} /* End namespace Aleph */

# endif // TPL_RB_TREE_H
