 
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
  @file tpl_binNodeXt.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BINNODEXT_H
# define TPL_BINNODEXT_H

# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace Aleph {

class BinNodeXt_Data
{
  size_t count; /* Tree cardinality */

public:

  BinNodeXt_Data() : count(1) {}
  BinNodeXt_Data(SentinelCtor) : count(0) {}

  size_t & getCount() { return count; }
  const size_t & size() const { return count; }

  void reset() { count = 1; }
};

DECLARE_BINNODE_SENTINEL(BinNodeXt, 255, BinNodeXt_Data);

/** 
  @brief Returns the cardinality of the tree with root p in a tree with ranges.

  @param p A pointer to a node with ranges.
  
  @ingroup Arboles
*/
# define COUNT(p) ((p)->getCount())

/** 
  @brief Select a node of a binary tree by infix position.

  @details select_rec(r,i) returns the node whose infix position i in
  binary tree whose root is r ranges.

  This selection algorithm is recursive.

  @param[in] r The binary tree root with ranges.
  @param[in] i Infix position you want to access.
  
  @return A pointer to the node in the infix position i.
  
  @throw out_of_range If i is greater than or equal to the total amount
  of binary tree nodes.

  @see select()

  @ingroup Arboles
*/
    template <class Node> inline 
Node * select_rec(Node * r, const size_t i) 
  throw(std::exception, std::out_of_range) 
{ 
  assert(r != Node::NullPtr);
  assert(COUNT(Node::NullPtr) == 0);

  if (i >= COUNT(r))
    throw std::out_of_range("infix position out of range");

  if (i == COUNT(LLINK(r))) 
    return r;

  if (i < COUNT(LLINK(r))) 
    return select_rec((Node*) LLINK(r), i);

  return select_rec((Node*) RLINK(r), i - COUNT(LLINK(r)) - 1);
}

/** 
  @brief Select a node of a binary tree by infix position.

  @details select(r,i) returns the node whose infix position is in the
  binary tree whose root is r ranges.

  This selection algorithm is iterative. It is more efficient and
  sure than select_rec(). 

  @param[in] r The binary tree root with ranges.
  @param[in] pos Infix position you want to access.
  
  @return A pointer to the node in the infix position i.
  
  @throw out_of_range If i is greater than or equal to the total amount
  of binary tree nodes.

  @see select_rec()
  
  @ingroup Arboles
*/
    template <class Node> inline 
Node * select(Node * r, const size_t pos) 
  throw(std::exception, std::out_of_range)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (pos >= COUNT(r))
    throw std::out_of_range("infix position out of range");

  for (size_t i = pos; i != COUNT(LLINK(r)); /* Nada */)
    {
      assert(i < COUNT(r) and COUNT(LLINK(r)) + COUNT(RLINK(r)) + 1 == COUNT(r));

      if (i < COUNT(LLINK(r)))
        r = static_cast<Node*>(LLINK(r)); 
      else
        {
          i -= COUNT(LLINK(r)) + 1;
          r = static_cast<Node*>(RLINK(r));
        }
    }

  return r;
}

/** 
  @brief Select a node of a binary tree by infix position.

  @details select(r,i) returns the node whose infix position is in the
  binary tree whose root is r ranges.

  This selection algorithm is iterative. It is more efficient and
  sure than select_rec(). 

  @param[in] root The binary tree root with ranges.
  @param[in] pos Infix position you want to access.
  @parent[out] parent Nodo padre del encontrado

  @return A pointer to the node in the infix position i.
  
  @throw out_of_range If i is greater than or equal to the total amount
  of binary tree nodes.

  @see select_rec()

  @ingroup Arboles
*/
    template <class Node> inline 
Node * select(Node * root, const size_t pos, Node *& parent) 
  throw(std::exception, std::out_of_range)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (pos >= COUNT(root))
    throw std::out_of_range("infix position out of range");

  parent = Node::NullPtr;
  for (size_t i = pos; i != COUNT(LLINK(root)); /* nada */)
    {
      assert(i < COUNT(root) and 
	     COUNT(LLINK(root)) + COUNT(RLINK(root)) + 1 == COUNT(root));

      parent = root;
      if (i < COUNT(LLINK(root)))
	root = static_cast<Node*>(LLINK(root)); 
      else
        {
          i -= COUNT(LLINK(root)) + 1;
          root = static_cast<Node*>(RLINK(root));
        }
    }

  return root;
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

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type>> inline
long inorder_position(Node * r, const typename Node::key_type & key, Node *& p)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr)
    return -1;

  if (Compare () (key, KEY(r))) 
    return inorder_position<Node,Compare>((Node*) LLINK(r), key, p);
  else if (Compare () (KEY(r), key)) 
    {
      int ret = inorder_position<Node,Compare>((Node*) RLINK(r), key, p);
      if (ret != -1)
	return ret + COUNT(LLINK(r)) + 1;
      return ret;
    }
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

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
long find_position(Node * r, const typename Node::key_type & key, Node *& p)
{
  assert(COUNT(Node::NullPtr) == 0);

  Node * parent = NULL;
  long pos = COUNT(LLINK(r));

  while (r != Node::NullPtr)
    if (Compare () (key, KEY(r)))  
      {
        parent = r;
        r = (Node*) LLINK(r);
        pos -= (COUNT(RLINK(r)) + 1);
      }
    else if (Compare () (KEY(r), key)) 
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
  @brief Inserts a node in a binary tree search with ranges by
  substitution of an external node.

  @details insert_by_key_xt(root, p) p inserts the p node in the
  binary tree search with ranges whose root is "root" under
  Compare()() comparison criterion.

  @param[in,out] r The root of binary tree search.
  @param[in] p Node to insert.
  
  @return The direction of the inserted node (p) if the key
  of p is not contained within the tree; Node::NullPtr than
  otherwise.

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_by_key_xt(Node *& r, Node * p)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr) 
    return r = p;

  Node * q;
  if (Compare () (KEY(p), KEY(r)))
    {
      q = insert_by_key_xt<Node, Compare>((Node*&) LLINK(r), p);
      if (q != Node::NullPtr)
        ++COUNT(r); 
    }
  else if (Compare ()(KEY(r), KEY(p)))
    {
      q = insert_by_key_xt<Node, Compare>((Node*&) RLINK(r), p);
      if (q != Node::NullPtr)
        ++COUNT(r); 
    }
  else
    return (Node*) Node::NullPtr; /* Duplicate key */

  return q;
}

/** 
  @brief Inserts a node in a binary tree search with ranges by
  substitution of an external node and the possibility of duplication.

  @details insert_by_key_xt(root, p) inserts the p node in the
  binary tree search with ranges whose root is "root" under
  Compare()() comparison criterion.

  @param[in,out] r The root of binary tree search.
  @param[in] p Node to insert.
  
  @return The node address inserted p

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_dup_by_key_xt(Node *& r, Node * p)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr) 
    return r = p;

  Node * q;
  if (Compare () (KEY(p), KEY(r)))
    q = insert_dup_by_key_xt<Node, Compare>((Node*&) LLINK(r), p);
  else 
    q = insert_dup_by_key_xt<Node, Compare>((Node*&) RLINK(r), p);

  ++COUNT(r); 

  return q;
}

/** 
  @brief Search the p key in the binary search tree extended
  with root or inserts should not be.

  @details search_or_insert_by_key_xt(p) busca en el árbol binario
  extendido un nodo cuya clave 
  sea <code>KEY(p)</code>. Si la clave se encuentra, entonces se
  retorna un puntero al nodo que la alberga. De lo contrario se
  inserta p en el árbol binario de búsqueda this. 
  
  @param[in] r Raíz del árbol binario de búsqueda extendido.
  @param[in] p Node to find or insert.
  
  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, it returns a pointer 
  to the node in the tree containing <code>KEY(p)</code>.

*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * search_or_insert_by_key_xt(Node *& r, Node * p)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr) 
    return r = p;

  Node * q;
  if (Compare () (KEY(p), KEY(r)))
    {
      q = insert_by_key_xt<Node, Compare>((Node*&) LLINK(r), p);
      if (q != Node::NullPtr)
        ++COUNT(r); 
    }
  else if (Compare () (KEY(r), KEY(p)))
    {
      q = insert_by_key_xt<Node, Compare>((Node*&) RLINK(r), p);
      if (q != Node::NullPtr)
        ++COUNT(r); 
    }
  else
    return r;

  return q;
}

# define SPLIT split_key_rec_xt<Node, Compare>

/** 
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  @brief Partitions a binary tree search with ranges as a key.

  @details split_key_rec_xt(root,key,l,r) particiona, según la clave key, un
  árbol binario de búsqueda con rangos en dos árboles ts y
  tg. Luego de la operación el árbol, el árbol root deviene vacío,
  ts contiene todas las claves menores que key y tg las mayores.

  @param[in,out] root A pointer to the root of the binary tree
  with ranges that you want to partition.
  @param[in] key Partition key.
  @param[out] l Binary tree search with ranges with the
  key less than key.
  @param[out] r Binary tree search with ranges with the 
  biggest keys that key.

  @return true if "key" is not within the binary tree; in which
  case if the partition was performed successfully. Otherwise, if
  "key" is inside the tree, the tree is not partitioned and the
  result is false.

  @return true if "key" is not within the binary tree; in which
  case if the partition was performed successfully. Otherwise, if
  "key" is inside the tree, the tree is not partitioned and the
  result is false.

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
bool split_key_rec_xt(Node * root, const typename Node::key_type & key, 
                      Node *& l, Node *& r)
{
  if (root == Node::NullPtr)
    {
      l = r = Node::NullPtr;
      return true;
    }

  if (Compare() (key, KEY(root)))
    {
      if (not SPLIT((Node*) LLINK(root), key, l, (Node*&) LLINK(root))) 
        return false;

      r = root;
      COUNT(r) -= COUNT(l);
    }
  else if (Compare() (KEY(root), key))
    {
      if (not SPLIT((Node*) RLINK(root), key, (Node*&) RLINK(root), r)) 
        return false;

      l = root;
      COUNT(l) -= COUNT(r);
    }
  else
    return false; /* Duplicate key */

  return true;
}

# undef SPLIT
# define SPLIT split_key_dup_rec_xt<Node, Compare>

/** 
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  @brief Partitions a binary tree search with ranges as a key.

  @details split_dup_key_rec_xt(root,key,l,r) particiona, según la clave key, un
  árbol binario de búsqueda con rangos en dos árboles ts y
  tg. Luego de la operación el árbol, el árbol root deviene vacío,
  ts contiene todas las claves menores o iguales que key y tg las
  mayores. 

  @param[in,out] root A pointer to the root of the binary tree
  with ranges that you want to partition.
  @param[in] key Partition key.
  @param[out] l Binary tree search with ranges with the
  key less than key.
  @param[out] r Binary tree search with ranges with the 
  keys greater than or equal to key.

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
void split_key_dup_rec_xt(Node * root, const typename Node::key_type & key, 
                          Node *& l, Node *& r)
{
  if (root == Node::NullPtr)
    {
      l = r = Node::NullPtr;
      return;
    }

  if (Compare() (key, KEY(root)))
    {
      SPLIT((Node*) LLINK(root), key, l, (Node*&) LLINK(root));
      r = root;
      COUNT(r) -= COUNT(l);
    }
  else if (Compare() (KEY(root), key))
    {
      SPLIT((Node*) RLINK(root), key, (Node*&) RLINK(root), r);
      l = root;
      COUNT(l) -= COUNT(r);
    }
  else
    {
      SPLIT((Node*) LLINK(root), key, l, (Node*&) LLINK(root));
      r = root;
      COUNT(r) -= COUNT(l);
    }
}

# undef SPLIT

/** 
  @brief Insert a node as root a binary tree search with ranges. 

  @details insert_root_xt(root,p) inserta el árbol binario de búsqueda con
  rangos de raíz root el nodo p. Luego de la operación, p deviene
  raíz del árbol binario de búsqueda con rangos.

  @param[in,out] root Root of tree binary of ranges search. 
  After the operation root acquires the value of the parameter p.
  @param[in] p Node to insert as a root.
  
  @return Pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
  
  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_root_xt(Node *& root, Node * p)
{
  if (root == Node::NullPtr) 
    return p;

  if (not split_key_rec_xt<Node, Compare>(root, KEY(p), LLINK(p), RLINK(p)))
    return Node::NullPtr;

  COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
  root = p;

  return p; 
}

/** 
  @brief Insert a node as root a binary tree search with ranges
  and with the possibility of duplication.

  @details insert_dup_root_xt(root,p) inserta el árbol binario de búsqueda con
  rangos de raíz root el nodo p. Luego de la operación, p deviene
  raíz del árbol binario de búsqueda con rangos.

  @param[in,out] root Root of tree binary of ranges search. 
  After the operation root acquires the value of the parameter p.
  @param[in] p Node to insert as a root.

  @return A pointer to the inserted node (the new root)

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_dup_root_xt(Node *& root, Node * p)
{
  if (root == Node::NullPtr) 
    return p;

  split_key_dup_rec_xt<Node, Compare>(root, KEY(p), LLINK(p), RLINK(p));

  COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;

  return root = p;
}

/** 
  @brief Partitions a binary tree ranges in i position.

  @details split_pos_rec(r,i,ts,tg) partitions a
  binary search tree with ranges, according to infix position
  or, two tg ts and trees. After the operation tree,
  tree root becomes empty, ts has all the keys between
  [0..i] and tg between (i..n) where n is the number of nodes
  binary tree.

  @param[in,out] r A pointer to the root of the binary tree
   ranges that you want to partition. 
  @param[in] i Posición infija de partición.
  @param[out] ts Binary tree search with ranges with the keys [0..i).
  @param[out] tg Binary tree search with ranges with the keys [i..n).

  @ingroup Arboles
*/
    template <class Node> inline
void split_pos_rec(Node * r, const size_t i, Node *& ts, Node *& tg)
{
  if (i > COUNT(r))
    throw std::out_of_range("infix position out of range");

  if (i == COUNT(r)) /* Is the last position? */
    {
      ts = r;
      tg = Node::NullPtr;
      return;
    }

  if (i == COUNT(LLINK(r))) /* Partition position is reached? */
    {
      ts = LLINK(r);
      tg = r;
      LLINK(tg) = Node::NullPtr;
      COUNT(tg) -= COUNT(ts);
      return;
    }

  if (i < COUNT(LLINK(r)))
    {
      split_pos_rec((Node*) LLINK(r), i, ts, (Node*&) LLINK(r));
      tg = r;
      COUNT(r) -= COUNT(ts);
    }
  else
    {
      split_pos_rec((Node*) RLINK(r), i - (COUNT(LLINK(r)) + 1), 
                    (Node*&) RLINK(r), tg);
      ts = r;
      COUNT(r) -= COUNT(tg);
    }
}

/** 
  @brief Insert a node in binary tree with ranges.

  @details insert_by_pos_xt(r,p,pos) inserta en el árbol binario con
  rangos, en la posición infija pos, el nodo p.

  @param[in,out] r The root of binary tree with ranges.
  @param[in] p Node to insert.
  @param[in] pos Posición infija de inserción.
  
  @note Note that insertion occurs regardless of
  key value that contains p. Wear a risk if it is a
  binary search tree.

  @ingroup Arboles
*/
    template <class Node>  inline
void insert_by_pos_xt(Node *& r, Node * p, const size_t & pos)
{
  assert(COUNT(Node::NullPtr) == 0);

  split_pos_rec(r, pos, (Node*&) LLINK(p), (Node*&) RLINK(p));
  COUNT(p) = COUNT(LLINK(p)) + 1 + COUNT(RLINK(p));
  r = p;
}

/** 
  @brief Exclusive union of two binary search trees ranges.

  @details join_exclusive_xt(ts,tg) joins two binary search trees
  ranges into one. By unique is meant that all
  ts keys are smaller than all keys tg.

  @param[in] ts Binary tree search of minor keys.
  @param[in] tg Binary tree search for higher keys.

  @return The resulting tree of the exclusive union.

  @note The routine does not validate that tg and ts are binary trees
  search nor that their ranges are exclusive.

  @ingroup Árboles

  @see remove_by_key_xt()
*/
    template <class Node> inline 
Node * join_exclusive_xt(Node *& ts, Node *& tg)
{
  if (ts == Node::NullPtr) 
    return tg;

  if (tg == Node::NullPtr) 
    return ts;

  LLINK(tg) = join_exclusive_xt(RLINK(ts), LLINK(tg));
  RLINK(ts) = tg;

    /* Update counters */
  COUNT(tg) = COUNT(LLINK(tg)) + 1 + COUNT(RLINK(tg)); 
  COUNT(ts) = COUNT(LLINK(ts)) + 1 + COUNT(RLINK(ts));

  Node * ret_val = ts;
  ts = tg = Node::NullPtr; /* They may stay empty after join */

  return ret_val;
}  

# define REMOVE remove_by_key_xt<Node, Compare>

/** 
  @brief Deletes a key from a binary tree search with ranges
  by the exclusive union.

  @details remove_by_key_xt(root,key) tree searches
  binary search ranges root root node containing
  the key key. If the node is found, then this is
  substituted into the general binary tree by the result of the
  exclusive union of hives removed node.

  @param[in,out] root The root the binary tree search.
  @param[in] key Key of node to delete.
  
  @return A pointer to the node removed if the key is in the
  tree; Node::NullPtr otherwise.

  @see join_exclusive_xt()
  
  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline 
Node * remove_by_key_xt(Node *& root, const typename Node::key_type & key)
{
  assert(root != Node::NullPtr);

  if (root == Node::NullPtr) 
    return (Node*) Node::NullPtr; /* Key Not Found */

  Node * ret_val = Node::NullPtr; 
  if (Compare () (key, KEY(root)))
    {
      ret_val = REMOVE((Node*&) LLINK(root), key);

    /** There was deletion?
    Yes? ==>, Update counter */
      if (ret_val != Node::NullPtr) 
        --COUNT(root); 

      return ret_val;
    }
  else if (Compare () (KEY(root), key))
    {
      ret_val = REMOVE((Node*&) RLINK(root), key);

    /** There was deletion?
    Yes? ==> Update counter */
      if (ret_val != Node::NullPtr) 
        --COUNT(root);

      return ret_val;
    }

  ret_val = root; /* Key found ==> Delete */
  root = join_exclusive_xt((Node*&)LLINK(root), (Node*&)RLINK(root));

  ret_val->reset();

  return ret_val;
}

# undef REMOVE

/** 
  @brief Removes a node in a binary search tree ranges in the pos.

  @details remove_by_pos_xt(root,pos) removed from the binary tree ranges
  des root root node at position pos. If the node is
  found, then it is replaced in the binary tree
  generally the result of the exclusive union of subtrees
  removed node.  

  @param[in,out] root The root the binary tree search.
  @param[in] pos Infix position of the node to remove.
  
  @return A pointer to the node removed.

  @see join_exclusive()
  
  @ingroup Arboles
*/
    template <class Node> inline
Node * remove_by_pos_xt(Node *& root, const size_t & pos)
{
  if (pos >= COUNT(root))
    throw std::out_of_range("infix position out of range");

    /** Position found?
    Yes? ==> save node and perform "join" exclusive */
  if (COUNT(LLINK(root)) == pos) 
    {     
      Node * ret_val = root;
      root = join_exclusive_xt((Node*&) LLINK(root), 
                               (Node*&) RLINK(root));

      ret_val->reset();

      return ret_val;
    }

  Node * ret_val; /* Save return value of recursive call */
  if (pos < COUNT(LLINK(root)))
    ret_val = remove_by_pos_xt((Node*&) LLINK(root), pos);
  else
    ret_val = remove_by_pos_xt((Node*&) RLINK(root),
                               pos - (COUNT(LLINK(root)) + 1));

    /** There was deletion?
    Yes? ==> the tree with "root" root node lost */
  if (ret_val != Node::NullPtr) 
    --COUNT(root); 

  return ret_val;
}  

    template <class Node> inline
bool check_rank_tree(Node * root)
{
  if (root == Node::NullPtr) 
    return true;

  if (COUNT(LLINK(root)) + COUNT(RLINK(root)) + 1 != COUNT(root)) 
    return false;

  return check_rank_tree(LLINK(root)) and check_rank_tree(RLINK(root));
}

/**  
  @brief Rotates clockwise binary tree with ranges of "p" root.

  @ingroup Arboles
*/
    template <class Node> inline 
Node * rotate_to_right_xt(Node* p)
{
  assert(p != Node::NullPtr);
  assert(COUNT(LLINK(p)) + 1 + COUNT(RLINK(p)) == COUNT(p));

  Node * q  = static_cast<Node*>(LLINK(p));
  LLINK(p) = RLINK(q);
  RLINK(q) = p;
  COUNT(p) -= 1 + COUNT(LLINK(q));
  COUNT(q) += 1 + COUNT(RLINK(p));
  assert(COUNT(LLINK(q)) + 1 + COUNT(RLINK(q)) == COUNT(q));
  return q;           
} 

/**  
  @brief Rotate left the binary tree with ranges "p" root.

  @ingroup Arboles
*/
    template <class Node> inline
Node * rotate_to_left_xt(Node* p)
{
  assert(p != Node::NullPtr);
  assert(COUNT(LLINK(p)) + 1 + COUNT(RLINK(p)) == COUNT(p));

  Node * q  = static_cast<Node*>(RLINK(p));
  RLINK(p) = LLINK(q);
  LLINK(q) = p;
  COUNT(p) -= 1 + COUNT(RLINK(q));
  COUNT(q) += 1 + COUNT(LLINK(p));
  assert(COUNT(LLINK(q)) + 1 + COUNT(RLINK(q)) == COUNT(q));
  return q;                  
} 

# define SEARCH_OR_INSERT search_or_insert_root_rec_xt<Node, Key, Compare>

/** 
  @brief Search the p key in the binary tree search or inserts 
  in case if not found.

  @details search_or_insert_root_rec(p) search the binary tree node
  whose key is <code>KEY(p)</code>.
  If the key is found, then it returns a pointer to the node that hosts it.
  Otherwise p is inserted into the binary search tree <code>this</code>.

  @param[in] r Root of tree.
  @param[in] p Node to find or insert.
  
  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, it returns a pointer 
  to the node in the tree containing <code>KEY(p)</code>.
*/
    template <class Node, class Key, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * search_or_insert_root_rec_xt(Node * root, Node * p)
{
  if (root == Node::NullPtr)
    return p; /* Insertion in empty tree */

  if (Compare () (KEY(p), KEY(root)))
    { /* Insert in left subtree */
      Node * left_branch = SEARCH_OR_INSERT ((Node*) LLINK(root), p);
      if (left_branch == p) /* There was insertion? */
        {
          ++COUNT(root);
          LLINK(root) = left_branch;
          root        = rotate_to_right_xt(root);
          return p;
        }

        return left_branch; /* No la hubo */
    }
  else if (Compare () (KEY(root), KEY(p) ))
    { /* Insert in right subtree */
      Node * right_branch = SEARCH_OR_INSERT ((Node*) RLINK(root), p);
      if (right_branch == p) /* There was insertion? */
        {
          ++COUNT(root);
          RLINK(root) = right_branch;
          root        = rotate_to_left_xt(root);
          return p;
        }

      return right_branch; /* It was not */
    }

  return root;
}

# undef SEARCH_OR_INSERT

} /* End namespace Aleph */

# endif // TPL_BINNODEXT_H
