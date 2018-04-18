
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
  @file tpl_.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/

    //    /*******************
    //    Top Dowm Splay Trees
    //    *******************/

/** 
  @note This code is a c++ adaptation of Danny Sleator code. It can be loaded from 
  http://www.link.cs.cmu.edu/link/ftp-site/splaying/SplayTree.java
*/

# ifndef TPL_SPLAY_TREE_H
# define TPL_SPLAY_TREE_H

# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;


    template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTree 
{
public:

  typedef NodeType<Key> Node;

private:

  Node headnode; 
  Node * head, *& root;
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
    Node headNode;
    Node * l = &headNode;
    Node * r = &headNode;
    Node * t = root;

    while (true)
      if (cmp(key, KEY(t)))
	{     
	  if (LLINK(t) == Node::NullPtr)
	    break;

	  if (cmp(key, KEY(LLINK(t))))
	    {
	      t = rotate_to_right(t);
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
	      t = rotate_to_left(t);
	      if (RLINK(t) == Node::NullPtr)
		break;
	    }

	  RLINK(l) = t;
	  l        = t;
	  t        = RLINK(t);
	}
      else
	break;

/** @brief reassembling of sub trees in one with current as root. */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);
    LLINK(t) = headNode.getR();
    RLINK(t) = headNode.getL();
		 
    root = t;
  }

/** @brief Constructor */
  GenTdSplayTree(Compare & __cmp) 
    : head(&headnode), root(headnode.getR()), cmp(__cmp)
  {
    /* Empty */
  }

  GenTdSplayTree(Compare && __cmp) 
    : head(&headnode), root(headnode.getR()), cmp(__cmp)
  {
    /* Empty */
  }

  void swap(GenTdSplayTree & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

/** @brief Destructor */
  virtual ~GenTdSplayTree() { /* Empty */ }

private:

  Node * __insert(Node * p)
  {
    /** root is predecessor of p? */
    if (cmp(KEY(p), KEY(root)))
      {
       
	LLINK(p)    = LLINK(root);
	RLINK(p)    = root;
	LLINK(root) = Node::NullPtr;
      }
    /** root is successor of p. */
    else
      {
       
	RLINK(p)    = RLINK(root);
	LLINK(p)    = root;
	RLINK(root) = Node::NullPtr;
      }
    return root = p; /* Inserted node become root */
  }

public:

/** 
  @brief Inserts A node in a top down splay tree.

  @return A pointer to the inserted node if node is not found in tree;
  NULL otherwise.

  @param p A pointer to the node to be inserted
*/
  Node * insert(Node * p)
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);  

    /** Test insertion in empty tree */
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

    /** Test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    splay(KEY(p));

    return __insert(p);
  }

/**
  @brief Searches a key in a top down splay tree.

  @return A pointer to the node containing the key if the key is found;
  NULL otherwise.

  @param key Key to search
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

  @param key Key to search
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

/** @brief Get the top down splay tree's root */
  Node *& getRoot()
  {
    return root;
  } 

  bool verify() const { return true; }
};


    template <typename Key, class Compare = Aleph::less<Key>>
class Splay_Tree : public GenTdSplayTree<BinNode, Key, Compare>
{
public:

  Splay_Tree(Compare && cmp = Compare()) 
    : GenTdSplayTree<BinNode, Key, Compare> (std::move(cmp))
  {
    /* Empty */ 
  }

 Splay_Tree(Compare & cmp) 
    : GenTdSplayTree<BinNode, Key, Compare> (cmp)
  {
    /* Empty */ 
  }
};


    template <typename Key, class Compare = Aleph::less<Key>>
class Splay_Tree_Vtl : public GenTdSplayTree<BinNodeVtl, Key, Compare>
{
public:

  Splay_Tree_Vtl(Compare && cmp = Compare()) 
    : GenTdSplayTree<BinNodeVtl, Key, Compare> (std::move(cmp))
  {
    /* Empty */ 
  }

 Splay_Tree_Vtl(Compare & cmp) 
    : GenTdSplayTree<BinNodeVtl, Key, Compare> (cmp)
  {
    /* Empty */ 
  }
};


#endif /* TPL_SPLAY_TREE_H */
