
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
  @file tpl_binTree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BINTREE_H
# define TPL_BINTREE_H

# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Binary tree search.

  @details The GenBinTree class implements a binary tree search classic.

  This class is not intended for public use. Its purpose is to provide
  basic functionality to BinHeap, BinHeapVtl and DynBinHeap
  classes.

  @param NodeType Node type that uses the heap; it will be with 
  or without virtual destructor.
  @param Key The key that saves each node.
  @param Compare Standard of comparison between the nodes keys.

  @see BinTree BinTreeVtl DynBinTree

  @ingroup Arboles
*/
  template <template <typename> class NodeType, typename Key, class Compare>
class GenBinTree 
{
public:

  typedef NodeType<Key> Node;

private:

  Node      headNode;
  Node  *   head;
  Node *&   root;
  Compare & cmp;

  GenBinTree(const GenBinTree & ) = delete;

  GenBinTree & operator = (const GenBinTree &) = delete;

public:

  void swap(GenBinTree & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

/** @brief Returns a reference to the criterion of comparison. */
  Compare & key_comp() { return cmp; }

/** @overload key_comp() */
  Compare & get_compare() { return key_comp(); }

  GenBinTree(Compare & __cmp) 
    : head(&headNode), root(headNode.getR()), cmp(__cmp)
  {
    /* Empty */
  }

  GenBinTree(Compare && __cmp) 
    : head(&headNode), root(headNode.getR()), cmp(__cmp)
  {
    /* Empty */
  }

/** 
  @brief Search a key in a binary tree search.

  @details search(key) search in the binary tree search this por una
  ocurrencia  de la clave key.

  @param[in] key The key to look for.
  
  @return A pointer to the containing node the specified key if it is
  within the tree; NULL otherwise.
*/
  Node * search(const Key & key)
  {
    return BinTree_Operation<Node, Compare>(cmp).search(root, key);
  }

  virtual ~GenBinTree() { /* Empty */ }

  bool verify() { return check_binary_search_tree<Node, Compare>(root); }

/** @brief Return the root of binary tree search */
  Node*& getRoot() { return root; }

  bool verifyBin() 
  { 
    return check_binary_search_tree<Node, Compare>(root); 
  } 

/** 
  @brief Insert p in the binary tree search.

  @details Insert p in the binary tree search this.

  @param[in] p Node to insert.
  
  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, NULL.
*/
  Node * insert(Node *p)
  {
    return BinTree_Operation<Node, Compare>(cmp).insert(root, p);
  }

/** 
  @brief Insert "p" so duplicate the binary search tree.

  @details Insert p in the binary tree search this.

  @param[in] p Node to insert.
  
  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, NULL.  
*/
  Node * insert_dup(Node *p)
  {
    return BinTree_Operation<Node, Compare>(cmp).insert_dup(root, p);
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
  Node * search_or_insert(Node *p)
  {
    return BinTree_Operation<Node, Compare>(cmp).search_or_insert(root, p);
  }

/** 
  @brief Partition the binary tree search as a key

  @details split(key,l,r) partitions, according to the "key" key, the
  binary tree search "this" in two trees "l" and "r". After of the 
  operation tree, the "root" tree becomes empty, "l" contains
  all keys less than "key" and "r" greater or equal.

  @param[in] key Partition key.
  @param[out] l Tree with smaller keys that key.
  @param[out] r Tree with the biggest keys that key.

  @return true if "key" is not within the binary tree; in which
  case if the partition was performed successfully. Otherwise, if
  "key" is inside the tree, the tree is not partitioned and the
  result is false.
*/
  bool split(const Key & key, GenBinTree & l, GenBinTree & r)
  {
    return BinTree_Operation<Node, Compare>(cmp).
      split_key_rec(root, key, l.root, r.root);
  }

/** 
  @brief Partition the binary tree search as a key
  may be present on the tree.

  @details split_dup(key,l,r) partitions, according to the "key" key, the
  binary tree search "this" in two trees "l" and "r". After of the 
  operation tree, the "root" tree becomes empty, "l" contains
  all keys less than "key" and "r" greater or equal.

  @param[in] key Partition key.
  @param[out] l Tree with smaller keys that key.
  @param[out] r Tree with the biggest keys that key.
*/
  bool split_dup(const Key & key, GenBinTree & l, GenBinTree & r)
  {
    return BinTree_Operation<Node, Compare>(cmp).
      split_key_dup_rec(root, key, l.root, r.root);
  }

/** 
  @brief Deletes a key from a binary tree search.

  @details Search in the binary tree search "this", "root" a node
  containing the "key" key. If the node is found, then this
  it is substituted in the general binary tree by the result of the
  exclusive union of hives removed node.

  @param[in] key Key of node to delete.

  @return A pointer to the node removed if the key is in the
  tree; NULL otherwise.
*/
  Node * remove(const Key & key)
  {
    Node * ret = BinTree_Operation<Node, Compare>(cmp).remove(root, key);

    return ret != Node::NullPtr ? ret : NULL;
  }

/** 
  @brief Union of "this" with "tree".

  @details join(tree,dup) binding performs binary trees
  search "this" and "tree". Duplicate "tree" keys are
  placed in "dup".

  @param[in,out] tree Joining tree Treewith "this". Becomes empty after the call.
  @param[out] dup Binary tree search where to place repeated keys.
*/
  void join(GenBinTree & tree, GenBinTree & dup)
  {
    root = BinTree_Operation<Node, Compare>(cmp).join(root, tree.root, dup);
    tree.root = Node::NullPtr;
  }
};

/** 
  @brief Binary tree search without virtual destructors in the nodes.

  @details The BinTree class implements a binary tree of search classic
  in which nodes have no virtual destructor. Consequently,
  if you have a class derived from a node belonging to this
  binary tree search, then the destruction of a node
  not derived invokes the destructor of the derived class.

  @param Key The key that saves each node.
  @param Compare Standard of comparison between the nodes keys.

  @see GenBinTree BinTreeVtl DynBinTree

  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key> >
class BinTree : public GenBinTree<BinNode, Key, Compare> 
{
public:

  BinTree(Compare && cmp = Compare()) 
    : GenBinTree<BinNode, Key, Compare>(std::move(cmp))
  {
    /* Empty */
  }

  BinTree(Compare & cmp) 
    : GenBinTree<BinNode, Key, Compare>(cmp)
  {
    /* Empty */
  }
};

/** 
  @brief Binary tree search with destroyers in the virtual nodes.

  @details The BinTree class implements a binary tree of search classic
  in which the nodes are virtual destructor. Consequently,
  if you have a class derived from a node belonging to this
  binary tree search, then the destruction of a node
  derived it invoke the derived class destructor.

  @param Key The key that saves each node.
  @param Compare Standard of comparison between the nodes keys.

  @see GenBinTree BinTree DynBinTree

  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key> >
class BinTreeVtl : public GenBinTree<BinNodeVtl, Key, Compare> 
{
public:

  BinTreeVtl(Compare && cmp = Compare()) 
    : GenBinTree<BinNodeVtl, Key, Compare>(std::move(cmp))
  {
    /* Empty */
  }

  BinTreeVtl(Compare & cmp) 
    : GenBinTree<BinNodeVtl, Key, Compare>(cmp)
  {
    /* Empty */
  }
};

} /* End namespace Aleph */

# endif /* TPL_BINTREE_H */
