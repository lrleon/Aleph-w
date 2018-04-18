
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
  @file tpl_test_treap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_TREAP_H
# define TPL_TREAP_H

# include <gsl/gsl_rng.h> 
# include <ahFunction.H>
# include <treapNode.H>
# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph
{

/**
  @brief Binary tree of generic randomized search of Treap.

  @details A treap is a special class of binary tree search
  modification operations which are randomized. Consequently, 
  all operations on this binary tree class are \f$O(\lg n)\f$, 
  regardless of any bias that exists about the order of insertion or
  removal of keys.

  The treap is probably the class of binary search tree than 
  preserves a better balance probabilistic performance. Empirical 
  studies suggest that this is the type of binary tree search faster.

  @param NodeType Type of node that will manage the tree. It may be
  TreapNode or TreapNodeVtl type as desired or may not have
  virtual destructors. 
  @param Key Type of key to save the nodes.
  @param Compare the comparison criterion between the keys of the nodes.

  @ingroup Arboles

  @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl
*/
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Treap
{
public:

/** @brief Node Type of "treap" */
  typedef NodeType<Key> Node;
  
private:

  Node      head;
  Node *    head_ptr;
  Node *&   tree_root;
  gsl_rng * r;
  Compare & cmp;

  void init(unsigned int seed)
  {
    PRIO(head_ptr) = Min_Priority;
    r = gsl_rng_alloc (gsl_rng_mt19937);

    if (r == NULL)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  public:

/** 
  @brief Exchanges all elements of treap this with tree treap
  in constant time (and very fast).

  @param[in] tree Treap to exchange with this
*/
  void swap (Gen_Treap & tree)
  {
    std::swap(tree_root, tree.tree_root);
    std::swap(cmp, tree.cmp);
    std::swap(r, tree.r);
  }

/** @brief Returns a reference to the criterion of comparison. */
  Compare & key_comp() { return cmp; }

/** @overload key_comp() */
  Compare & get_compare() { return key_comp(); }

/** @brief Constructor; initialize random number generator. */
  Gen_Treap(unsigned int seed, Compare & __cmp) 
    : head_ptr(&head), tree_root(RLINK(head_ptr)), r(NULL), cmp(__cmp)
  {
    init(seed);
  }

  Gen_Treap(unsigned int seed, Compare && __cmp) 
    : head_ptr(&head), tree_root(RLINK(head_ptr)), r(NULL), cmp(__cmp)
  {
    init(seed);
  }

/** @brief Destroyer; frees memory for random number generator. */
  ~Gen_Treap() 
  {
    if (r != NULL)
      gsl_rng_free(r);
  }

/** @brief You get a pointer to the object gsl_rng random number generator. */
  gsl_rng * gsl_rng_object() { return r;}

/** @brief Returns the root of the "treap". */
  Node *& getRoot() { return tree_root; }

/** @brief Search the "key" key in the "treap". */
  Node * search(const Key & key)
  {
    Node* ret_val = searchInBinTree <Node, Compare> (tree_root, key, cmp);

    return ret_val == Node::NullPtr ? NULL : ret_val;
  }

  private:

  Node * insert(Node * root, Node * p)
  {
    if (root == Node::NullPtr) 
      return p;

    Node * insertion_result = NULL;
    if (cmp(KEY(p), KEY(root))) 
      {
        insertion_result = insert(LLINK(root), p);
        if (insertion_result == Node::NullPtr) 
          return Node::NullPtr;

        LLINK(root) = insertion_result;
        if (PRIO(insertion_result) < PRIO(root))
          return rotate_to_right(root);
        else
          return root;
      }
    else if (cmp(KEY(root), KEY(p)))
      {
        insertion_result = insert(RLINK(root), p);
        if (insertion_result == Node::NullPtr) 
          return Node::NullPtr;

        RLINK(root) = insertion_result;
        if (PRIO(insertion_result) < PRIO(root))
          return rotate_to_left(root);
        else
          return root;
      }

    return Node::NullPtr;
  }

/** @brief Search or insert "p". Returns "p" if KEY(p) is not within the tree;
otherwise, the address of the node contentivo KEY(p) is returned */
  Node * search_or_insert (Node *& root, Node * p) 
  {
    if (root == Node::NullPtr)
      return root = p;

    if (cmp(KEY(p), KEY(root)))
      {
        Node * ret = search_or_insert(LLINK(root), p);

    /** Did insertion?
    If the answer is YES, then eventually rebalance */
        if (ret == p) 
          if (PRIO(LLINK(root)) < PRIO(root))
            root = rotate_to_right (root);

        return ret;
      }
    else if (cmp(KEY(root), KEY(p)))
      {
        Node * ret = search_or_insert(RLINK(root), p);

    /** Did insertion?
    If the answer is YES, then eventually rebalance */
        if (ret == p) 
          if (PRIO(RLINK(root)) < PRIO(root))
            root = rotate_to_left (root);
            
        return ret;
      }

    return root; /* root contiene KEY(p) */
  }

  Node * insert_dup(Node * root, Node * p)
  {
    if (root == Node::NullPtr) 
      return p;

    if (cmp(KEY(p), KEY(root))) 
      {
        Node * result = LLINK(root) = insert_dup(LLINK(root), p);
        if (PRIO(result) < PRIO(root))
          return rotate_to_right(root);
        else
          return root;
      }

    Node * result = RLINK(root) = insert_dup(RLINK(root), p);

    if (PRIO(result) < PRIO(root))
      return rotate_to_left(root);
    else
      return root;
  }

  public:

/**
  @brief Insert the p node in a treap.

  @details insert(p) inserts the p node in the treap "this".

  @param[in] p Node to insert.

  @return A pointer to the newly inserted node if your password was not
  contained in treap; NULL otherwise.
*/
  Node * insert(Node * p)
  {
    assert(p != Node::NullPtr);

    PRIO(p)       = gsl_rng_get(r); /* Random selection of priority */
    Node * result = insert(tree_root, p);
    if (result == Node::NullPtr) 
      return NULL;

    tree_root = result;

    return p;  
  }

/** 
  @brief Search the p key in the binary tree search or inserts 
  in case if not found.

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
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r); /* Random selection of priority */
    
    return search_or_insert(tree_root, p);  
  }

/**
  @brief Insert the p node in a treap.

  @details insert(p) inserts the p node in the treap "this".

  @param[in] p Node to insert.

  @return A pointer to the newly inserted node if your password was not
  contained in treap; NULL otherwise.
*/
  Node * insert_dup(Node * p)
  {
    assert(p != Node::NullPtr);

    PRIO(p)   = gsl_rng_get(r); /* Random selection of priority */
    tree_root = insert_dup(tree_root, p);

    return p;  
  }

  bool verify() { return is_treap(tree_root); }

/**
  @brief Removes the key "key" of treap.

  @details remove(key) searches for the "key" key in the "treap" "this" and,
  found the key, removes node "treap".

  @param[in] key The key to delete.

  @return A pointer to the newly deleted node if the key is
  in the treap; NULL otherwise.
*/
  Node * remove(const Key & key)
  {
    Node ** pp = &RLINK(head_ptr);
    Node * p   = tree_root;

    /** Looking down the key */
    while (p != Node::NullPtr)
      if (cmp(key, KEY(p)))
        {
          pp = &LLINK(p);
          p  = LLINK(p);
        }
      else if (cmp(KEY(p), key))
        {
          pp = &RLINK(p);
          p  = RLINK(p);
        }
      else
        break; /* Encontrada! */

    if (p == Node::NullPtr) 
      return NULL; /* Key was not found */

    /* Rotar p hasta que devenga hoja */
    while (not (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr))
      if (PRIO(LLINK(p)) < PRIO(RLINK(p)))
        {
          *pp = rotate_to_right(p);
          pp  = &RLINK(*pp);
        }
      else
        {
          *pp = rotate_to_left(p);
          pp  = &LLINK(*pp);
        }

    *pp = Node::NullPtr;

    p->reset();

    return p;
  } 
};

/**
  @brief Binary tree of generic randomized search of Treap type with nodes 
  without virtual destructor.

  @details A treap is a special class of binary tree search
  modification operations which are randomized. Consequently, 
  all operations on this binary tree class are \f$O(\lg n)\f$, 
  regardless of any bias that exists about the order of insertion or
  removal of keys.

  The treap is probably the class of binary search tree than 
  preserves a better balance probabilistic performance. Empirical 
  studies suggest that this is the type of binary tree search faster.

  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys. 

  @ingroup Arboles

  @see Treap_Vtl Treap_Rk Treap_Rk_Vtl
*/
    template <typename Key, class Compare = Aleph::less<Key>>
class Treap : public Gen_Treap<TreapNode, Key, Compare> 
{
public:

  Treap(unsigned int seed, Compare && cmp = Compare()) 
    : Gen_Treap<TreapNode, Key, Compare>(seed, std::move(cmp))
  {
    /* Empty */
  }

  Treap(Compare && cmp = Compare()) 
    : Gen_Treap<TreapNode, Key, Compare>(time(NULL), std::move(cmp))
  {
    /* Empty */
  }

  Treap(unsigned int seed, Compare & cmp) 
    : Gen_Treap<TreapNode, Key, Compare>(seed, cmp)
  {
    /* Empty */
  }

  Treap(Compare & cmp) 
    : Gen_Treap<TreapNode, Key, Compare>(time(NULL), cmp)
  {
    /* Empty */
  }
};

/**
  @brief Binary tree of generic randomized search of Treap type with 
  virtual destructor in their nodes.

  @details A treap is a special class of binary tree search
  modification operations which are randomized. Consequently, 
  all operations on this binary tree class are \f$O(\lg n)\f$, 
  regardless of any bias that exists about the order of insertion or
  removal of keys.

  The treap is probably the class of binary search tree than 
  preserves a better balance probabilistic performance. Empirical 
  studies suggest that this is the type of binary tree search faster.

  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys. 

  @ingroup Arboles

  @see Treap Treap_Rk Treap_Rk_Vtl
*/
    template <typename Key, class Compare = Aleph::less<Key>>
class Treap_Vtl : public Gen_Treap<TreapNodeVtl, Key, Compare>
{
public:

  Treap_Vtl(unsigned int seed, Compare && cmp = Compare()) 
    : Gen_Treap<TreapNodeVtl, Key, Compare>(seed, std::move(cmp))
  {
    /* Empty */
  }

  Treap_Vtl(Compare && cmp = Compare()) 
    : Gen_Treap<TreapNodeVtl, Key, Compare>(time(NULL), std::move(cmp))
  {
    /* Empty */
  }

  Treap_Vtl(unsigned int seed, Compare & cmp) 
    : Gen_Treap<TreapNodeVtl, Key, Compare>(seed, cmp)
  {
    /* Empty */
  }

  Treap_Vtl(Compare & cmp) 
    : Gen_Treap<TreapNodeVtl, Key, Compare>(time(NULL), cmp)
  {
    /* Empty */
  }
};


} /* End namespace Aleph */

# endif // TPL_TREAP_H
