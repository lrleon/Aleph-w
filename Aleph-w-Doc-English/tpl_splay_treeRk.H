
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
  @file tpl_splay_treeRk.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


    //    /*******************
    //    Top Dowm Splay Trees
    //    *******************/

/** 
  @note This code is a c++ adaptation of Danny Sleator code. 
  It can be loaded from:
  http://www.link.cs.cmu.edu/link/ftp-site/splaying/SplayTree.java
*/

# ifndef TPL_SPLAY_TREE_H
# define TPL_SPLAY_TREE_H

# include <tpl_binNodeXt.H>


using namespace Aleph;


    template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTreeRk
{
public:

  typedef NodeType<Key> Node;

private:

  Node * root;
  Compare & cmp;

public:
  
/** @brief The type of key that contains the node. */
  typedef Key key_type;

/** @brief Returns a reference to the criterion of comparison. */
  Compare & key_comp() { return cmp; }

/** @overload key_comp() */
  Compare & get_compare() { return key_comp(); }

/** @brief search key within tree and splay that node, 
if not found it return Node::NullPtr. */ 
  void splay(const Key & key)
  {
    Node header(sentinelCtor);
    Node * head_ptr = &header;
    Node * l = head_ptr;
    Node * r = head_ptr;
    Node * t = root;

    while (true)
      if (cmp(key, KEY(t)))
	{     
	  if (LLINK(t) == Node::NullPtr)
	    break;

	  if (cmp(key, KEY(LLINK(t))))
	    {
	      t = rotate_to_right_xt(t);
	      if (LLINK(t) == Node::NullPtr)
		break;
	    }

	  LLINK(r) = t;
	  r        = t;
	  t        = LLINK(t);
	}
      else if (cmp(KEY(t), key))
	{   
	  if (RLINK(t) == Node::NullPtr)
	    break;

	  if (cmp(KEY(RLINK(t)), key))
	    {
	      t = rotate_to_left_xt(t);
	      if (RLINK(t) == Node::NullPtr)
		break;
	    }

	  RLINK(l) = t;
	  l        = t;
	  t        = RLINK(t);
	}
      else
	break;

    /**
      @if 

      @param t points to the splayed node
      @param l points to the predecessor of splayed node
      @param r points to the predecessor of splayed node
    */

    if (l != head_ptr)
      COUNT(l) += -COUNT(RLINK(l)) + COUNT(LLINK(t));
    if (r != head_ptr)
      COUNT(r) += -COUNT(LLINK(r)) + COUNT(RLINK(t));

    /* Reassembling of sub trees in one with t as root */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);

    LLINK(t) = RLINK(head_ptr);
    RLINK(t) = LLINK(head_ptr);

    COUNT(t) = COUNT(LLINK(t)) + 1 + COUNT(RLINK(t));

    root = t;
  }

/** @brief Constructor. */
  GenTdSplayTreeRk(Compare & __cmp) 
      : root(Node::NullPtr), cmp(__cmp)
  {
    /* Empty */
  }

  GenTdSplayTreeRk(Compare && __cmp) 
    : root(Node::NullPtr), cmp(__cmp)
  {
    /* Empty */
  }

  void swap(GenTdSplayTreeRk & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

/** @brief Destroyer. */
  virtual ~GenTdSplayTreeRk() { /* Empty */ }

private:

  Node * __insert(Node * p)
  {
    COUNT(p) = COUNT(root) + 1;
    if (cmp(KEY(p), KEY(root)))
      { 
	COUNT(root) -= COUNT(LLINK(root));
	LLINK(p)     = LLINK(root);
	RLINK(p)     = root;
	LLINK(root)  = Node::NullPtr;
      }
    else 
      { 
	COUNT(root) -= COUNT(RLINK(root));
	RLINK(p)    = RLINK(root);
	LLINK(p)    = root;
	RLINK(root) = Node::NullPtr;
      }

    return root = p; /* Inserted node become root */
  }

public:

/** 
  @brief Inserts a node in a top down splay tree.

  @return A pointer to the inserted node if node is not found in tree;
  NULL otherwise.

  @param p A pointer to the node to be inserted
*/
  Node * insert(Node * p)
  {
    assert(p != Node::NullPtr);
    assert(COUNT(p) == 1);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);  

    /* Test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);

    splay(key);

    if (are_equals<Key, Compare>(KEY(root), key, cmp))
      return NULL; /* Item is already in tree */

    return __insert(p);
  }

  Node * insert_dup(Node * p)
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);  

    /* Test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    splay(KEY(p));

    return __insert(p);
  }

/**
  @brief Searches a key in a top down splay tree.

  @return A pointer to the node containing the key if the key is found;
  NULL otherwise.

  @param key key to search
*/
  Node * search(const Key & key)
  {
    if (root == Node::NullPtr)
      return NULL;
      
    splay(key);

    return are_equals<Key, Compare>(KEY(root), key) ? root : NULL;
  }

  Node * search_or_insert(Node * p)
  {
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);
    splay(key);
    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      return root;

    return __insert(p);
  }

/**
  @brief Remove a key from a top down splay tree.

  @details Searches a key in a top down splay tree and remove the containing
  the key if this is found. 

  @return A pointer to node containing the removed key.

  @param key to search
*/
  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return NULL;

    splay(key);
      
    if (no_equals<Key, Compare>(KEY(root), key, cmp))
      return NULL; /* Key not found */

    Node * ret_val = root; /* Store node to delete */

    if (LLINK(root) == Node::NullPtr)
      root = RLINK(root);
    else
      {
	Node * p = RLINK(root);
	root = LLINK(root);
	splay(key);
	RLINK(root) = p;
      }

    ret_val->reset();

    return ret_val;
  }

/** @brief Get the top down splay tree's root. */
  Node *& getRoot()
  {
    return root;
  } 

  bool verify() const { return check_rank_tree(root); }

/** @brief Returns the amount of nodes containing the "treap". */
  size_t size() const 
  {
    return COUNT(root);
  }

/** @brief Returns true if the "treap" is empty. */
  bool is_empty() const 
  {
    return root == Node::NullPtr; 
  }

/** 
  @brief Returns the (ordered) infix position of the "key" key.

  @details position(key) search in the extended "splay" tree the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key.

  @param[in] key The key to seek and to determine infix position.
  
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  otherwise, it returns -1 and node is undetermined.
*/
  std::pair<int, Node*> position (const Key & key)
  {
    std::pair<int, Node*> ret_val;

    splay(key);

    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      {
	ret_val.first = COUNT(LLINK(root));
	ret_val.second = root;
      }
    else
      ret_val.first = -1;

    return ret_val;
  }

/** 
  @brief Returns the (ordered) infix position of the "key" key.

  @details find_position(key) searches in the tree "splay" extended the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key with the node. If the key
  not in the tree, position(key) returns the position where
  it should be if "key" belonged to the tree.

  The return value is a pair<int,Node*>. The "first" field is the
  infix position and the "second" the node.

  If key is not in the tree, then it can be
  distinguish three cases:
	-# If "key" is less than the minimum key tree, then "first"
   -1 and "second" is the node containing the minimum key.
	-# If the "key" is greater than the maximum tree key, then "first"
  is COUNT(r) (number of nodes in the tree) and "second" is
  the node containing the maximum key node.
	-# In any case, "first" is the position
  that would have the "key" key to tree and "second" node "p" is a key
  immediately adjacent to "key". Note that "second" can have a
  key minor or major, but is guaranteed to be immediately
  continuing to "key".
  .
  @param[in] key The key to seek and to determine infix position.
  
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  otherwise, it returns -1 and node is undetermined.
*/
  std::pair<int, Node*> find_position (const Key & key)
  {
    std::pair<int, Node*> r(-2, NULL);

    r.first = COUNT(Splay(key));
    r.second = root;

    return r;
  }

/** 
  @brief Returns the node whose infix position in the "treap" extended is "i".

  @details select(i) returns the node of "splay" tree extended whose
  infix position is "i".

  @param[in] i Infix position to select.

  @return A pointer to node corresponding to the infix position "i".

  @throw out_of_range If "i" is greater or equal to the number of nodes
  containing the tree.
*/
  Node * select (const size_t & i)
  {
    return Aleph::select(root, i); 
  }
};


    template <typename Key, class Compare = Aleph::less<Key>>
class Splay_Tree_Rk : public GenTdSplayTreeRk<BinNodeXt, Key, Compare>
{
public:

  Splay_Tree_Rk(Compare && cmp = Compare()) 
    : GenTdSplayTreeRk<BinNodeXt, Key, Compare> (std::move(cmp))
  {
    /* Empty */ 
  }

 Splay_Tree_Rk(Compare & cmp) 
    : GenTdSplayTreeRk<BinNodeXt, Key, Compare> (cmp)
  {
    /* Empty */ 
  }
};


    template <typename Key, class Compare = Aleph::less<Key>>
class Splay_Tree_Rk_Vtl : public GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>
{
public:

  Splay_Tree_Rk_Vtl(Compare && cmp = Compare()) 
    : GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare> (std::move(cmp))
  {
    /* Empty */ 
  }

 Splay_Tree_Rk_Vtl(Compare & cmp) 
    : GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare> (cmp)
  {
    /* Empty */ 
  }
};


#endif /* TPL_SPLAY_TREE_H */
