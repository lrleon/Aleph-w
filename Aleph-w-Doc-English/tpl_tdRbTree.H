
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
  @file tpl_tdRbTree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_TDRBTREE_H
# define TPL_TDRBTREE_H

# ifdef DEBUG
# include <math.h>
# endif
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <tpl_arrayStack.H>
# include <rbNode.H>


using namespace BinNode_Utils;


    template <template <class> class NodeType, class Key>
class GenTdRbTree 
{
public:

  typedef NodeType<Key> Node;

private:

    /* Head's content.
    This is an auxiliar node which is parent of root */
  Node                               headNode; 

    /* Another header node which is grand parent of root */
  Node                               headParent;

    /* Head pointer */
  Node                              *head;

    /* Head parent */
  Node                              *fHead;

    /* Root pointer. 
    This is a reference to right child of head */
  Node                             *&root;

# ifdef DEBUG
  unsigned int n;
# endif /* DEBUG */

  Node* getSibling(Node *p, Node *fp)
  {
    assert(LLINK(fp) == p or RLINK(fp) == p);
    return LLINK(fp) == p ? RLINK(fp) : LLINK(fp);
  }

  void restoreRedCondition(Node *p, 
			   Node *&fp, 
			   Node *ffp,
			   Node *fffp)
  {
    /* Sanity checks */
    assert(LLINK(fp) == p or RLINK(fp) == p);
    assert(COLOR(fp) == RED);
    assert(COLOR(p) == RED);

    if (fp == root)
      {
	COLOR(fp) = BLACK;
	return;
      }

    assert(LLINK(ffp) == fp or RLINK(ffp) == fp);
    assert(COLOR(ffp) == BLACK);
    assert(LLINK(fffp) == ffp or RLINK(fffp) == ffp);

    COLOR(ffp) = RED;

    if (LLINK(fp) == p and LLINK(ffp) == fp)
      {
	COLOR(fp) = BLACK;
	rotate_to_right(ffp, fffp);
      }
    else if (RLINK(fp) == p and RLINK(ffp) == fp)
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
  

  static void flipColors(Node* p)
  {
    assert(p not_eq Node::NullPtr);
    assert(COLOR(p) == BLACK);
    assert(COLOR(LLINK(p)) == RED and COLOR(RLINK(p)) == RED);

    COLOR(p)                          = RED;
    COLOR(LLINK(p)) = COLOR(RLINK(p)) = BLACK;
  }


  Node* searchFlipColorsAndInsert(Node *q)
  {
    assert(q not_eq Node::NullPtr);
    assert(root not_eq Node::NullPtr);
    assert(COLOR(q) == RED);
    assert(LLINK(q) == Node::NullPtr and RLINK(q) == Node::NullPtr);

    Node *p    = root;     /* Current node */
    Node *fp   = head;     /* p's parent */
    Node *ffp  = fHead;    /* p's grand parent */
    Node *fffp = Node::NullPtr;  /* p's great grand parent */
    Node *nextNode;

    while (1)
      {
	if (KEY(q) == KEY(p))
	  return NULL; /* Duplicated key, insertion is not possible */

	if (COLOR(p) == BLACK and COLOR(LLINK(p)) == RED 
	    and COLOR(RLINK(p)) == RED)
	  {
	    flipColors(p); /* Rends p red */
	    if (COLOR(fp) == RED) /* Violation of red condition */
	      {
		assert(fffp not_eq Node::NullPtr);
		restoreRedCondition(p, fp, ffp, fffp);
	      }
	  }

	if (KEY(q) < KEY(p))
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

	/* Parents' update */
	fffp = ffp;
	ffp  = fp;
	fp   = p;
	p    = nextNode;
      }

#   ifdef DEBUG
    n++;
#   endif

    /* Insertion act */
    if (KEY(q) < KEY(p))
      LLINK(p) = q;
    else 
      RLINK(p) = q;

    if (COLOR(p) == RED) /* Violation of red condition */
      restoreRedCondition(q, p, fp, ffp);

    return q;
  }


  static Node* gotoLeftAndColorRed(Node *fp, Node *&ffp)
  {
    assert(fp not_eq Node::NullPtr);
    assert(ffp not_eq Node::NullPtr);
    assert(LLINK(ffp) == fp or RLINK(ffp) == fp);
    assert(LLINK(fp) not_eq Node::NullPtr);

    Node *p = LLINK(fp); /* Advance to left link */

    if (COLOR(p) == RED)  
      return p; 

    Node *sp = RLINK(fp);

    /* fp black means that we come from another black node */ 
    if (COLOR(fp) == BLACK)
      { /* Guarantee that a leaf will be red */
	assert(COLOR(sp) == RED);
	rotate_to_left(fp, ffp);
	COLOR(fp) = RED;
	COLOR(sp) = BLACK;
	ffp       = sp;
	sp        = RLINK(fp);
      }

    if (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK)
      { /* Only valid if fp is RED */
	assert(COLOR(LLINK(fp)) == BLACK);
	assert(COLOR(RLINK(fp)) == BLACK);
	assert(COLOR(fp) == RED);

	COLOR(p)  = RED;
	COLOR(fp) = BLACK;

	Node *np  = RLINK(sp); /* Nephew of p (child of sp) */
	Node *snp = LLINK(sp); /* Another nephew of p (sibling of np) */

	if (COLOR(snp) == BLACK and COLOR(np) == BLACK) 
	  { /* Flips colors of fp with their children */
	    COLOR(sp) = RED;
	    return p;
	  }

	if (COLOR(np) == RED)
	  {
	    ffp = rotate_to_left(fp, ffp);
	    COLOR(sp) = RED;
	    COLOR(np) = BLACK;
	    assert(ffp == sp);
	    return p;
	  }
	  
	assert(COLOR(snp) == RED);

	rotate_to_right(sp, fp);
	ffp = rotate_to_left(fp, ffp);
	assert(ffp == snp);	   
      }
    return p;
  }

  static  Node* gotoRightAndColorRed(Node *fp, Node *&ffp)
  {
    assert(fp not_eq Node::NullPtr);
    assert(ffp not_eq Node::NullPtr);
    assert(LLINK(ffp) == fp or RLINK(ffp) == fp);
    assert(RLINK(fp) not_eq Node::NullPtr);

    Node *p = RLINK(fp); /* Advance to right link */

    if (COLOR(p) == RED)  
      return p; 

    Node *sp = LLINK(fp);

    /* fp black means that we come from another black node. */
    if (COLOR(fp) == BLACK)
      { /* Guarantee that a leaf will be red */
	assert(COLOR(sp) == RED);
	rotate_to_right(fp, ffp);
	COLOR(fp) = RED;
	COLOR(sp) = BLACK;
	ffp       = sp;
	sp        = LLINK(fp);
      }

    if (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK)
      { /* Only valid if fp is RED */
	assert(COLOR(RLINK(fp)) == BLACK);
	assert(COLOR(LLINK(fp)) == BLACK);
	assert(COLOR(fp) == RED);

	COLOR(p)  = RED;
	COLOR(fp) = BLACK;

	Node *np  = LLINK(sp); /* Nephew of p (child of sp) */
	Node *snp = RLINK(sp); /* Another nephew of p (sibling of np) */

	if (COLOR(snp) == BLACK and COLOR(np) == BLACK)
	  { /* Flips colors of fp with their children */
	    COLOR(sp) = RED;
	    return p;
	  }

	if (COLOR(np) == RED)
	  {
	    ffp       = rotate_to_right(fp, ffp);
	    COLOR(sp) = RED;
	    COLOR(np) = BLACK;
	    assert(ffp == sp);
	    return p;
	  }
	  
	assert(COLOR(snp) == RED);

	rotate_to_left(sp, fp);
	ffp = rotate_to_right(fp, ffp);
	assert(ffp == snp);
      }
    return p;
  }

  static void findSuccAndSwap(Node *p, Node *&fp)
  {
    assert(p not_eq Node::NullPtr);
    assert(RLINK(p) not_eq Node::NullPtr);
    assert(fp not_eq Node::NullPtr);
    assert(LLINK(fp) == p or RLINK(fp) == p);


    Node *fSucc  = p;  /* Succesor's parent */

/** @brief Searching starts from p's right child. */
    Node *succ   = gotoRightAndColorRed(p, fp); 
    Node *ffSucc = fp; /* Successor grand parent */
    while (LLINK(succ) not_eq Node::NullPtr)  /* Go down to leftmost */
      {
	ffSucc = fSucc;
	fSucc  = succ;
	succ   = gotoLeftAndColorRed(fSucc, ffSucc);
      }

    /** Setting of parent of p to its new child (succ) */
    if (LLINK(fp) == p)
      LLINK(fp) = succ;
    else
      RLINK(fp) = succ;
    
/** @brief Swaps left branches. */
    LLINK(succ) = LLINK(p);
    LLINK(p)    = Node::NullPtr; 

    /** For right branches there are two cases */
    if (RLINK(p) == succ) 
      {
/** @brief successor is just right's child of p */
	RLINK(p)    = RLINK(succ); 
	RLINK(succ) = p;
	fSucc       = fp;
	fp          = succ;
      }
    else
      {
/** @brief successor is the leftmost nodo descending from right child of p */
	Node *tmp    = fp; /* Backup for swapping fp and fsucc */
	Node *succr  = RLINK(succ); 
          
	RLINK(succ)  = RLINK(p);
	LLINK(fSucc) = p;
	RLINK(p)     = succr;
	fp           = fSucc;
	fSucc        = tmp; 
      }

/** @brief Swaps of colors */
    Color tmp   = COLOR(succ);
    COLOR(succ) = COLOR(p); 
    COLOR(p)    = tmp;
  }

  static void findPredAndSwap(Node *p, Node *&fp)
  {
    assert(p not_eq Node::NullPtr);
    assert(LLINK(p) not_eq Node::NullPtr);
    assert(fp not_eq Node::NullPtr);
    assert(LLINK(fp) == p or RLINK(fp) == p);

    Node *fPred  = p;  /* Predesor's parent */
/** @brief Searching starts from p's right child */ 
    Node *pred   = gotoLeftAndColorRed(p, fp);
    Node *ffPred = fp; /* Predesor's grand parent */
    while (RLINK(pred) not_eq Node::NullPtr)  /* Go down to rightmost */
      {
	ffPred = fPred;
	fPred  = pred;
	pred   = gotoRightAndColorRed(fPred, ffPred);
      }

    /** Setting of parent of p to its new child (pred) */
    if (LLINK(fp) == p)
      LLINK(fp) = pred;
    else
      RLINK(fp) = pred;
    

/** @brief Swaps right branches */
    RLINK(pred) = RLINK(p);
    RLINK(p)    = Node::NullPtr; 

    /** For right branches there are two cases */
    if (LLINK(p) == pred) 
      {
/** @brief Predecessor is just right's child of p */
	LLINK(p)    = LLINK(pred); 
	LLINK(pred) = p;
	fPred       = fp;
	fp          = pred;
      }
    else
      {
/** @brief Predecessor is the rightmost nodo descending from right child of p */
	Node *tmp    = fp; /* Backup for swapping fp and fpred */
	Node *predl  = LLINK(pred); 
          
	LLINK(pred)  = LLINK(p);
	RLINK(fPred) = p;
	LLINK(p)     = predl;
	fp           = fPred;
	fPred        = tmp; 
      }

    /* Swap of colors */
    Color tmp   = COLOR(pred);
    COLOR(pred) = COLOR(p); 
    COLOR(p)    = tmp;
  }


  void colorRootAsRed()
  {
    if (COLOR(root) == RED)
      return;

    if (COLOR(LLINK(root)) == BLACK and COLOR(RLINK(root)) == BLACK)
      COLOR(root) = RED;      
  }


  Node *searchAndColorRed(const Key& key, Node *&fp)
  {
    Node *p   = root;
    fp        = head;
    Node *ffp = fHead;

    colorRootAsRed();

    while (1)
      {
	if (key == KEY(p))
	  return p;

	ffp = fp;
	fp  = p;

	if (key < KEY(p))
	  {
	    if (LLINK(p) == Node::NullPtr)
	      return p;
	    p = gotoLeftAndColorRed(fp, ffp);
	  }
	else
	  {
	    if (RLINK(p) == Node::NullPtr)
	      return p;
	    p = gotoRightAndColorRed(fp, ffp);
	  }
      }
  }


  static void removeAndRendLeafRed(Node *p, Node *fp)
  {
    assert(p not_eq Node::NullPtr);
    assert(fp not_eq Node::NullPtr);
    assert(LLINK(fp) == p or RLINK(fp) == p);

    while (LLINK(p) not_eq Node::NullPtr or RLINK(p) not_eq Node::NullPtr)
      {
	if (RLINK(p) not_eq Node::NullPtr)
	  findSuccAndSwap(p, fp);
	else if (LLINK(p) not_eq Node::NullPtr)
	  findPredAndSwap(p, fp);
      }

    if (LLINK(fp) == p)
      LLINK(fp) = Node::NullPtr;
    else
      RLINK(fp) = Node::NullPtr;
  }

public:

  GenTdRbTree() : head(&headNode), fHead(&headParent), root(headNode.getR())
  {
    RLINK(fHead)   = head;
    COLOR(Node::NullPtr) = BLACK;
    COLOR(head)    = BLACK;

#   ifdef DEBUG
    n      = 0;
#   endif /* DEBUG */
  }

  void reset()
  {
    new (this) GenTdRbTree;
  }

  virtual ~GenTdRbTree()
  {
    /* Empty */
  }

  Node* insert(Node *p)
  {
    assert(p not_eq Node::NullPtr);
    assert(COLOR(p) == RED);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    if (root == Node::NullPtr)
      {
	root = p;
#       ifdef DEBUG
	n++;
#       endif
	return p;
      }

    return searchFlipColorsAndInsert(p);
  }

  Node* search(const Key& key)
  {
    Node* retVal = searchInBinTree(root, key);

    return retVal == Node::NullPtr ? NULL : retVal;
  }

  Node* remove(const Key& key)
  {
    if (root == Node::NullPtr)
      return NULL;
      
    Node *fp;
    Node *p = searchAndColorRed(key, fp);

    if (KEY(p) not_eq key)
      return NULL;

    removeAndRendLeafRed(p, fp);

#   ifdef DEBUG
    n++;
#   endif

    return p;
  }

  Node*& getRoot() { return root; }

private:

  static bool testBlackHeight(Node *p, int &max, int bh = 0)
  {
    if (p == Node::NullPtr)
      return true;

    if (COLOR(p) == BLACK)
      bh++; /* Another seen black node */

    /** if a leaf is reached, we must verify max with current bh 
    (number of visited black nodes) */ 
    if (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr)
      {
	if (max == -1) /* This is only for the first reached leaf */
	  max = bh;
	return bh == max;
      }

    /** @brief continue counting in the left and right branches respectively */
    return (testBlackHeight(LLINK(p), max, bh)
	    and testBlackHeight(RLINK(p), max, bh));
  }


  static bool testNode(Node* p)
  {
    /** verify red condition. */
    if (COLOR(p) == RED and
	not (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK))
      return false;

    int max = -1;
    int bh  = 0;

    return testBlackHeight(p, max, bh);
  }


  static bool verify(Node* p)
  {
    if (testNode(p))
      return testNode(LLINK(p)) and testNode(RLINK(p));
    return false;
  }

public:

/** @brief Traverse all tree and verify black height for each visited node */
  bool verifyRedBlack()
  {
    return verify(root);
  }
};


    template <class Key>
class TdRbTree : public GenTdRbTree<RbNode, Key>
{
public:

  TdRbTree() { /* Empty */ }
};


    template <class Key>
class TdRbTreeVtl : public GenTdRbTree<RbNodeVtl, Key>
{
public:

  TdRbTreeVtl() { /* Empty */ }
};


# endif /* TPL_TDRBTREE_H */
