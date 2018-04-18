
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
  @file tpl_TreeOps.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BINTREEOPS_H
# define TPL_BINTREEOPS_H

# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

namespace Aleph {

/** 
  @brief Basic and general operations on binary trees search.

  @details The idea of this class in efficiently encompass the various
  operations that can be performed on a binary tree search.

  The class manages two type parameters:
  -# Node: Binary node type. It must be based on BinNode.
  -# Cmp: Comparison criterion between the keys.

  @see BinNode
  
  @ingroup Arboles
*/
  template <class Node,
	    class Cmp = Aleph::less<typename Node::key_type>
	    >
class BinTree_Operation
{
protected:

  Cmp & cmp;

public:

/** @brief Returns a reference to the criterion of comparison. */
  Cmp & key_comp() { return cmp; }

/** @overload key_comp() */
  Cmp & get_compare() { return cmp; }

/** @brief The type of search key. */
  typedef typename Node::key_type Key;

/** @brief The type of search key. */
  typedef typename Node::key_type key_type;

  BinTree_Operation(Cmp & __cmp) : cmp(__cmp) { /* Empty */ }

  BinTree_Operation(Cmp && __cmp = Cmp()) : cmp(__cmp) { /* Empty */ }

/** @brief Search for the "key" key in the tree whose root is "root".
Returns pointer to the node if found; Node::NullPtr otherwise */
  Node * search(Node * root, const Key & key)
  {
    return searchInBinTree <Node, Cmp> (root, key, cmp);
  }

/** 
  @brief Keyword Search binary tree search with determination 
  of the parent.

  @details search_parent(root,key,parent) searches for the key in the 
  key binary tree search whose root is "root" and determines,
  contentivo addition the key node, the parent node.

  @param[in] root The root the binary tree search where search.
  @param[in] key The search key.
  @param[out] parent Pointer to the parent node containing the
  key (if it was found).

  @return A pointer to the key document containing key node if it
  is in the binary tree search; Node::NullPtr than otherwise.
*/
  Node * search_parent(Node * root, const Key & key, Node *& parent)
  {
    return Aleph::search_parent<Node, Cmp>(root, key, parent, cmp);
  }

/** 
  @brief Search failed node and determination of the parent.

  @details search_rank_parent(root,key) search the binary tree search
  its root is the key "key". If the key is in the 
  binary tree, then returns the document containing key node;
  otherwise, the node returns to the key would parent
  if it it were inserted into the binary tree search.

  @param[in] root The root the binary tree search.
  @param[in] key The key to look for.

  @return A pointer to contentivo node of the key if it is
  in the binary tree; otherwise, it returns a pointer
  the node would parent of key if this key was inserted in
  binary tree search.

  @note Do not check if the tree is empty.
*/
  Node * search_rank_parent(Node * root, const Key & key)
  {
    return Aleph::search_rank_parent<Node, Cmp>(root, key, cmp);
  }

/** 
  @brief Inserts a node in a binary tree search by substitution
  an external node.

  @details insert(root, p) inserts the p node in the
  binary tree search with ranges whose root is "root" under
  Compare()() comparison criterion.

  @param[in,out] root The root the binary tree search.
  @param[in] p Node to insert.

  @return The direction of the inserted node (p) if the key
  of p is not contained within the tree; Node::NullPtr than
  otherwise.
*/
  Node * insert(Node *& root, Node * p)
  {
    if (root == Node::NullPtr) 
      return root = p;

    if (cmp(KEY(p), KEY(root)))        /* p < root? */
      return insert((Node*&) LLINK(root), p);
    else if (cmp(KEY(root), KEY(p)))   /* ¿p > root? */
      return insert((Node*&) RLINK(root), p);

    return Node::NullPtr; /* Clave repetida ==> no hay inserción */
  }

/** 
  @brief Inserts a node in a binary tree search by substitution
  an external node.

  @details insert_dup(root, p) insert the "p" node in the
  binary tree search whose root is "root" at the discretion of
  comparison Compare()().

  Repeated keys are allowed.

  @param[in,out] root The root the binary tree search.
  @param[in] p Node to insert.

  @return The direction of the inserted node (p) if the key
  of p is not contained within the tree; Node::NullPtr than
  otherwise.
*/
  Node * insert_dup(Node *& root, Node * p)
  {
    if (root == Node::NullPtr) 
      return root = p;

    if (cmp(KEY(p), KEY(root)))  /* p < root? */
      return insert_dup((Node*&) LLINK(root), p);
  
    return insert_dup((Node*&) RLINK(root), p);
  }

/** 
  @brief Find a key in a binary tree search and inserted into
  if you do not find it.

  @details search_or_insert(root, p) searches for the key
  <code>KEY(p)</code> binary tree search whose root is "root"
  according to the comparison criteria <code>Compare()()</code>. If the
  key is found, then it returns a pointer to the node
  the houses; otherwise, the <code>p</code> node is inserted
  in the tree and it returns <code>p</code>.

  @param[in,out] r The root of binary tree search.
  @param[in] p Node to find or insert.
  
  @return The address of the node that contains a <code>KEY(p)</code>
  if your key is already in the tree or <code>p</code> if the key does not
  it is, in which case inserts.
*/
  Node * search_or_insert(Node *& r, Node * p)
  {
    if (r == Node::NullPtr) 
      return r = p;

    if (cmp(KEY(p), KEY(r)))        /* p < root? */
      return search_or_insert((Node*&) LLINK(r), p);
    else if (cmp (KEY(r), KEY(p)))   /* ¿p > root? */
      return search_or_insert((Node*&) RLINK(r), p);

    return r; 
  }

/** 
  @brief Recursively partitioned a binary tree search as a key.

  @details split_key_rec(root,key,ts,tg) partitions, according to the key 
  "key" a binary tree search in two trees tg and ts. After the
  operation tree, tree root becomes empty, ts contains all lower keys than 
  key and tg the majors.

  @param[in,out] root A pointer to the root of the binary tree to partition. 
  @param[in] key Partition key.
  @param[out] ts Tree with smaller keys that key.
  @param[out] tg Tree with the biggest keys that key.
  
  @return true if "key" is not within the binary tree; in which
  case if the partition was performed successfully. Otherwise, if
  "key" is inside the tree, the tree is not partitioned and the
  result is false.
*/
  bool split_key_rec(Node * root, const Key & key, Node *& ts, Node *& tg)
  {
    if (root == Node::NullPtr)
      {    
    /* key is not in tree ==> split will succeed */
	ts = tg = Node::NullPtr;

	return true;
      }

    if (cmp(key, KEY(root)) ) /* key < KEY(root)? */
      if (split_key_rec((Node*&) LLINK(root), key, ts, (Node*&) LLINK(root)))
	{
	  tg = root;

	  return true;
	}
      else
	return false;

    if (cmp(KEY(root), key) ) /* key < KEY(root)? */
      if (split_key_rec((Node*&) RLINK(root), key, (Node*&) RLINK(root), tg))
	{
	  ts = root;

	  return true;
	}
      else
	return false;

    return false; /* Key exists in tree ==> is left intact */
  }

/** 
  @brief Recursively partitioned a binary tree search as a key.

  @details split_key_dup_rec(root,key,ts,tg) partitions, according to the key 
  "key" a binary tree search in two trees tg and ts. After the
  operation tree, tree root becomes empty, ts contains all lower keys than 
  key and tg the majors.

  With this approach, the infix traversal of tree will be ordered
  by keys and the duplication by insertion order.

  @param[in,out] root A pointer to the root of the binary tree to partition. 
  @param[in] key Partition key.
  @param[out] ts Tree with smaller keys that key.
  @param[out] tg Tree with the biggest keys that key.
*/
  void split_key_dup_rec(Node * root, const typename Node::key_type & key, 
			 Node *& ts, Node *& tg)
  {
    if (root == Node::NullPtr)
      {    
    /* key is not in tree ==> split will succeed */
	ts = tg = Node::NullPtr;

	return;
      }

    if (cmp(key, KEY(root))) /* key < KEY(root)? */
      split_key_dup_rec((Node*&) LLINK(root), key, ts, (Node*&) LLINK(root));
    else if (cmp(KEY(root), key)) /* key < KEY(root)? */
      split_key_dup_rec((Node*&) RLINK(root), key, (Node*&) RLINK(root), tg);
    else /* key == KEY(root) */
      split_key_dup_rec((Node*&) LLINK(root), key, ts, (Node*&) LLINK(root));
  }

/** 
  @brief Deletes a key from a binary tree search by the exclusive union.

  @details remove_from_search_binary_tree(root,key) searches into the 
  binary tree search with "root" root a node containing the "key"
  key. If the node is found, then it is replaced in the
  general binary tree by the result of the exclusive union of
  hives removed node.

  @param[in,out] root The root the binary tree search.
  @param[in] key Key of node to delete.

  @return A pointer to the node removed if the key is in the
  tree; Node::NullPtr otherwise.

  @see join_exclusive()
*/
  Node * remove(Node *& root, const Key & key)
  {
    if (root == Node::NullPtr) 
      return Node::NullPtr;

    if (cmp (key, KEY(root)))       /* key < KEY(root)? */
      return remove((Node*&) LLINK(root), key);
    else if (cmp(KEY(root), key))  /* key < KEY(root)? */
      return remove((Node*&) RLINK(root), key);

    Node * ret_val = root; /* Support root to be deleted */
    root = join_exclusive((Node*&) LLINK(root), (Node*&) RLINK(root));

    ret_val->reset();

    return ret_val;
  }

/** 
  @brief Insert a node as root in a binary tree search by the 
  partitioning algorithm.

  @details insert_root(root,p) inserts the binary tree search with
  root "root" the node p. After the operation, p becomes the root from 
  binary tree search.

  The routine partitione root into two trees as the key
  contained in p and then attached to the partitions to 
  the branch left and right of p.

  @param[in,out] root The root the binary tree search. After of
  the root operation acquires the value of the parameter p.
  @param[in] p Node to insert as a root.
  
  @return A pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
*/
  Node * insert_root(Node *& root, Node * p)
  {
    Node * l = Node::NullPtr, * r = Node::NullPtr;

    if (not split_key_rec(root, KEY(p), l, r)) 
      return Node::NullPtr; 

    LLINK(p) = l;
    RLINK(p) = r;
    root = p;

    return root; 
  }

/** 
  @brief Inserts a node with the possibility of doubling as a root in a
  binary tree search by partitioning algorithm.

  @details insert_dup_root(root,p) inserts the binary tree search with
  root "root" the p node. After the operation, p becomes the root of 
  binary tree search.

  The keys equal to KEY(p) are on the left.

  The routine partitione root into two trees as the key
  contained in p and then attached to the partitions to 
  the branch left and right of p.

  @param[in,out] root The root the binary tree search. After of
  the root operation acquires the value of the parameter p.
  @param[in] p Node to insert as a root.

  @return Pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
*/
  Node * insert_dup_root(Node *& root, Node * p)
  {
    split_key_dup_rec(root, KEY(p), LLINK(p), RLINK(p));

    return root = p;
  }

/** 
  @brief Union of two binary trees search as prefix algorithm.

  @details join_preorder(t1,t2,dup) walk the prefix tree "t2". Every
  key "t2" is inserted in "t1". If the key is duplicated,
  then inserted into "dup".

  @param[in,out] root A pointer to the root of the binary tree
  @param[in] t1 Binary tree search.
  @param[in] t2 Binary tree search.
  @param[out] dup Binary tree search with the key duplicate of t2.

  @return A pointer to the root of the binary search tree
  resulting from the union of t1 and t2.

  @see join()

  @note After the trees calls t1 and t2 become empty;
  however the values of t1 and t2 are unchanged.
*/
  Node * join_preorder(Node * t1, Node * t2, Node *& dup)
  {
    if (t2 == Node::NullPtr) 
      return t1;

    Node * l = (Node*) LLINK(t2); /* t2 branches backups */
    Node * r = (Node*) RLINK(t2);

    if (insert(t1, t2) == Node::NullPtr)
    /* Inserción fallida ==> Elemento duplicado ==> Insertar en dup */
      insert(dup, t2);

    join_preorder(t1, l, dup);
    join_preorder(t1, r, dup);

    return t1;
  }

/** 
  @brief Union of two binary trees search.

  @details join(t1,t2,dup) build a binary tree search corresponding 
  to the union of t1 to t2. Duplicate keys is inserted in dup.

  @param[in,out] root A pointer to the root of the binary tree
  @param[in] t1 Binary tree search.
  @param[in] t2 Binary tree search.
  @param[out] dup Binary tree search with the key duplicate of t2.
  
  @return A pointer to the root of the binary search tree
  resulting from the union of t1 and t2.

  @see join_preorder()
  
  @note After the trees calls t1 and t2 become empty;
  however the values of t1 and t2 are unchanged.
*/
  Node * join(Node * t1, Node * t2, Node *& dup)
  {
    if (t1 == Node::NullPtr) 
      return t2;

    if (t2 == Node::NullPtr) 
      return t1;

    Node * l = (Node*) LLINK(t1); /* t1 branches backups */
    Node * r = (Node*) RLINK(t1);

    t1->reset();

    /* Mientras la clave raíz de t1 esté contenida en t2 */
    while (insert_root(t2, t1) == Node::NullPtr) 
      { /* Sí ==> sáquelo de t1 e insértelo en dup */
	Node * p = remove(t1, KEY(t1));

	assert(p != Node::NullPtr);

	insert(dup, t1);
      }

    LLINK(t2) = join(l, (Node*) LLINK(t2), dup);
    RLINK(t2) = join(r, (Node*) RLINK(t2), dup);

    return t2;
  }

/** 
  @brief Iteratively partitions a binary tree search according to key.

  @details split_key(root,key,l,r) as the "key" key,
  a binary tree search into two trees "l" and "r". Then
  operation tree, the tree "root" becomes empty, "l" contains
  all keys less than "key" and "r" greater or equal.

  The iterative partition is faster than the recursive.

  @param[in,out] root A pointer to the root of the binary tree to partition. 
  @param[in] key Partition key.
  @param[out] l Tree with smaller keys that key.
  @param[out] r Tree with the biggest keys that key.

  @return true if "key" is not within the binary tree; in which
  case if the partition was performed successfully. Otherwise, if
  "key" is inside the tree, the tree is not partitioned and the
  result is false.

  @note Unlike split_key_rec() this routine includes the 
  partition key if it is already within the
  binary tree.
*/
  void split_key(Node * root, const Key & key, Node *& l, Node *& r)
  {
    if (root == Node::NullPtr) 
      {
	l = r = (Node*) Node::NullPtr;

	return;
      }

    Node * current;
    Node ** current_parent = NULL;
    Node ** pending_child  = NULL;
    char current_is_right;

    if (cmp (key, KEY(root)))
      {
	r = root;
	pending_child    = &l;
	current_is_right = true;
      }
    else
      {
	l = root;
	pending_child    = &r;
	current_is_right = false;
      }

    current = root;

    while (current != Node::NullPtr) 
      {
	if (cmp (key, KEY(current)))
	  { /* Current must be in right side */
	    if (not current_is_right)
	      { 
		current_is_right = not current_is_right;
		*pending_child   = *current_parent; /* change of side */
		pending_child    = current_parent;
	      }

	    current_parent = static_cast<Node**>(&LLINK(current));
	  }
	else
	  { /* Current must be in left side */
	    if (current_is_right)
	      { 
		current_is_right = not current_is_right;
		*pending_child   = *current_parent; /* Change of side */
		pending_child    = current_parent;
	      }
	    current_parent = static_cast<Node**>(&RLINK(current));
	  }

	current = *current_parent;
      }

    *pending_child = static_cast<Node*>(Node::NullPtr);
  }

/** 
  @brief Insert a node as root in a binary tree search by the 
  recursive reverse and rotations to the root.

  @details insert_root_rec(root,p) inserts the binary tree search with
  root "root" the p node. After the operation, p becomes the root from 
  binary tree search.

  The routine first inserted p in the classic insertion, then
  p rotates until it earns the root.

  @param[in] root The root the binary tree search.
  @param[in] p Node to insert as a root.

  @return A pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
*/
  Node * insert_root_rec(Node * root, Node * p)
  {
    if (root == Node::NullPtr)
      return p; /* Insertion in empty tree */

    if (cmp(KEY(p), KEY(root)))
      { /* Insert in left subtree */
	Node *left_branch = insert_root_rec((Node*) LLINK(root), p);
	if (left_branch == Node::NullPtr)
	  return (Node*) Node::NullPtr;

	LLINK(root) = left_branch;
	root        = rotate_to_right(root);
      }
    else if (cmp(KEY(root), KEY(p)))
      { /* Insert in right subtree */
	Node *right_branch = insert_root_rec((Node*) RLINK(root), p);
	if (right_branch == Node::NullPtr)
	  return (Node*) Node::NullPtr;

	RLINK(root) = right_branch;
	root        = rotate_to_left(root);
      }
    else
      return (Node*) Node::NullPtr; /* Duplicated key */

    return root;
  }

/** 
  @brief Search the p key in the binary tree search or inserts 
  in case if not found.

  @details search_or_insert_root_rec(p) search the binary tree a node
  whose key is <code>KEY(p)</code>.
  If the key is found, then it returns a pointer to the node that hosts it.
  Otherwise p is inserted into the binary tree search <code>this</code>.

  @param[in] r Root of tree.
  @param[in] p Node to find or insert.

  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, it returns a pointer 
  to the node in the tree containing <code>KEY(p)</code>.
*/
  Node * search_or_insert_root_rec(Node * root, Node * p)
  {
    if (root == Node::NullPtr)
      return p; /* Insertion in empty tree */

    if (cmp(KEY(p), KEY(root)))
      {     /* Insert in left subtree */
	Node * left_branch = search_or_insert_root_rec((Node*) LLINK(root), p);
	if (left_branch == p) /* ¿Hubo inserción? */
	  {
	    LLINK(root) = left_branch;
	    root        = rotate_to_right(root);

	    return p;
	  }

        return left_branch; /* No la hubo */
      }
    else if (cmp(KEY(root), KEY(p)))
      {     /* Insert in right subtree */
	Node * right_branch = search_or_insert_root_rec((Node*) RLINK(root), p);
	if (right_branch == p) /* ¿Hubo inserción?*/
	  {
	    RLINK(root) = right_branch;
	    root        = rotate_to_left(root);

	    return p;
	  }

	return right_branch; /* No la hubo */
      }

    return root;
  }
};


/** 
  @brief Basic and general operations on binary trees search with ranges.

  @details The idea of this class in efficiently encompass the various
  operations that can be performed on a binary tree
  containing search ranges.

  The class manages two type parameters:
  -# Node: Binary node type. It must be based on BinNode.
  -# Cmp: Comparison criterion between the keys.

  @see BinNodeXt

  @ingroup Arboles
*/
    template <class Node,
	      class Cmp = Aleph::less<typename Node::key_type>
	      >
class BinTreeXt_Operation : public BinTree_Operation<Node, Cmp>
{
public:

  typedef typename Node::key_type Key;

  BinTreeXt_Operation(Cmp && cmp = Cmp()) 
    : BinTree_Operation<Node, Cmp>(std::move(cmp))
  {
    /* Empty */
  }

  BinTreeXt_Operation(Cmp & cmp) 
    : BinTree_Operation<Node, Cmp>(cmp)
  {
    /* Empty */
  }

/** 
  @brief Determines the infix position of a key in a tree with ranges.

  @details inorder_position(r,key,node) search the key in the binary tree
  search with ranges whose root is r. If the key is found,
  then the routine returns the position of the node that hosts the
  key and stores a pointer to the node in the node parameter.

  The routine uses the comparison criterion expressed by the
  Compare class. 

  @param[in] r Root of binary tree search with ranges.
  @param[in] key The key to look and determine infix position
  @param[out] p Contentivo node pointer to the specified key (if
  was found).
  
  @return The node infix position contentivo of the "key" key if this
  it is in the tree; otherwise, it returns -1.
*/
  long inorder_position(Node * r, const Key & key, Node *& p)
  {
    assert(COUNT(Node::NullPtr) == 0);

    if (r == Node::NullPtr)
      return -1;

    if (this->cmp(key, KEY(r)))
      return inorder_position((Node*) LLINK(r), key, p);
    else if (this->cmp(KEY(r), key)) 
      return inorder_position((Node*) RLINK(r), key, p) + COUNT(LLINK(r)) + 1;
    else
      {
	p = r;
	return COUNT(LLINK(r));
      }
  }

/** 
  @brief Determines the infix position of a key in a tree with ranges.

  @details find_position(r,key,node) search the key in the binary tree
  search with ranges whose root is r. If the key is found,
  then the routine returns the position of the node that hosts the
  key and stores a pointer to the node in the node parameter. Otherwise, 
  the routine returns the position in which find the key if it was inserted 
  in the tree. In the latter case, the p parameter contains a continuous node
  to the key if key is not greater than the maximum key contained in the tree. 

  If key is not in the tree, then it can be
  distinguish three cases:
  -# If key is less than the minimum tree key, then return 
  the value is -1 and p is the node that containing the minimum 
  key. 
  -# If key is greater than the maximum key tree, then the
  return value is COUNT(r) (number of nodes in the tree) and p is
  the node containing the maximum key.
  -# In any other case, the return value is the position
  than would the key in the tree and the p node is a key
  adyancente immediately to key. Note that p may have a
  key minor or major, but is guaranteed to be immediately
  continuous to key.

  The routine uses the comparison criterion expressed by the
  Compare class. 

  @param[in] r Root of binary tree search with ranges.
  @param[in] key The key to look and determine infix position
  @param[out] p Contentivo node pointer to the specified key (if
  It was found) or key contained in the tree that is 
  greater than key.
  
  @return The node infix position contentivo of the "key" key if this
  it is in the tree; otherwise, it returns the position where would find 
  the key inserted into the tree.
*/
  int find_position(Node * r, const Key & key, Node *& p)
  {
    assert(COUNT(Node::NullPtr) == 0);

    Node * parent = NULL;
    int pos = COUNT(LLINK(r));

    while (r != Node::NullPtr)
      if (this->cmp(key, KEY(r)))  
	{
	  parent = r;
	  r = (Node*) LLINK(r);
	  pos -= (COUNT(RLINK(r)) + 1);
	}
      else if (this->cmp(KEY(r), key)) 
	{
	  parent = r;
	  r = (Node*) RLINK(r);
	  pos += COUNT(LLINK(r)) + 1;
	}
      else
	{
	  p = r;
	  return pos;
	}

    p = parent;

    return pos;
  }

/** 
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  @brief Partitions a binary tree search with ranges as a key.

  @details split_key_rec(root,key,l,r) partitions, according to the "key" key a
  binary tree search ranges in two trees "ts" and
  "tg". After the operation the tree, the "root" tree becomes empty,
  "ts" has all the keys under which "key" and "tg" the majors.

  @param[in,out] root A pointer to the root of the binary tree
  with ranges that you want to partition.
  @param[in] key Partition key.
  @param[out] l Binary tree search with ranges with the
  key less than key.
  @param[out] r Binary tree search with ranges with the
  key greater than key.
  
  @return true if "key" is not within the binary tree; in which
  case if the partition was performed successfully. Otherwise, if
  "key" is inside the tree, the tree is not partitioned and the
  result is false.
*/
  bool split_key_rec(Node * root, const Key & key, Node *& l, Node *& r)
  {
    if (root == Node::NullPtr)
      {
	l = r = Node::NullPtr;

	return true;
      }

    if (this->cmp(key, KEY(root)))
      {
	if (not split_key_rec(LLINK(root), key, l, LLINK(root))) 
	  return false;

	r = root;
	COUNT(r) -= COUNT(l);
      }
    else if (this->cmp(KEY(root), key))
      {
	if (not split_key_rec(RLINK(root), key, RLINK(root), r)) 
	  return false;

	l = root;
	COUNT(l) -= COUNT(r);
      }
    else
      return false; /* Duplicate key */

    return true;
  }

/** 
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  @brief Partitions a binary tree search with ranges as a key.

  @details split_dup_key_rec(root,key,l,r) partitions, according to the "key" key a
  binary tree search ranges in two trees "ts" and
  "tg". After the operation the tree, the "root" tree becomes empty,
  "ts" contains all keys less than or equal to "key" and "tg" the
  higher.

  @param[in,out] root A pointer to the root of the binary tree
  with ranges that you want to partition.
  @param[in] key Partition key.
  @param[out] l Binary tree search with ranges with the
  key less than key.
  @param[out] r Binary tree search with ranges with the
  key greater than key.
*/
  void split_key_dup_rec(Node * root, const Key & key, Node *& l, Node *& r)
  {
    if (root == Node::NullPtr)
      {
	l = r = Node::NullPtr;

	return;
      }

    if (this->cmp(key, KEY(root)))
      {
	split_key_dup_rec(LLINK(root), key, l, LLINK(root));
	r = root;
	COUNT(r) -= COUNT(l);
      }
    else if (this->cmp(KEY(root), key))
      {
	split_key_dup_rec(RLINK(root), key, RLINK(root), r);
	l = root;
	COUNT(l) -= COUNT(r);
      }
    else
      {
	split_key_dup_rec(LLINK(root), key, l, LLINK(root));
	r = root;
	COUNT(r) -= COUNT(l);
      }
  }

/** 
  @brief Insert a node as root a binary tree search with ranges. 

  @details insert_root(root,p) inserts the binary tree search with
  root "root" the node p. After the operation, p becomes the root from 
  binary tree search with ranges.

  @param[in,out] root Root of tree binary of ranges search. 
  After the operation root acquires the value of the parameter p.
  @param[in] p Node to insert as a root.
  
  @return A pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
*/
  Node * insert_root(Node *& root, Node * p)
  {
    if (root == Node::NullPtr) 
      return p;

    if (not split_key_rec(root, KEY(p), LLINK(p), RLINK(p)))
      return Node::NullPtr;

    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    root = p;

    return p; 
  }

/**
  @brief Insert a node as root a binary tree search with ranges
  and with the possibility of duplication.

  @details insert_dup_root(root,p) inserts the binary tree search with
  root "root" the p node. After the operation, p becomes the root of 
  binary tree search.

  @param[in,out] root Root of tree binary of ranges search. 
  After the operation root acquires the value of the parameter p.
  @param[in] p Node to insert as a root.
  
  @return A pointer to the inserted node (the new root)
*/
  Node * insert_dup_root(Node *& root, Node * p)
  {
    if (root == Node::NullPtr) 
      return p;

    split_key_dup_rec(root, KEY(p), LLINK(p), RLINK(p));

    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;

    return root = p;
  }
};

} /* End namespace Aleph */

# endif // TPL_BINTREEOPS_H
