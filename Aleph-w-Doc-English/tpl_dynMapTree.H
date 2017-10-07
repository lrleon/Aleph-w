
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
  @file tpl_dynMapTree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNMAPTREE_H
# define TPL_DYNMAPTREE_H

# include <tpl_dynSetTree.H>
 
using namespace Aleph;

namespace Aleph {

/** 
  @brief Mapping generic instrumented binary search trees.

  @details DynMapTree defines a mapping between a set of keys, which
  They will not be repeated, and set range. The mapping is implanted
  by some kind of binary search tree defined as type parameters.

  The type of this class parameters are defined as follows:
  - # Tree: type binary search tree that is desired
  implement the mapping. Possible types Bintree Avl_Tree
  Splay_Tree Rb_Tree Treap and Rand_Tree.
  - # Key: Key Type Indexing. He is also saying "domain"
  mapping.
  - # Data: Type range.
  - # Compare: Criteria comparison between the keys.

  Mapping elements can be accessed via the operator []
  placing as the key parameter indexing.

  @see BinTree Avl_Tree Splay_Tree Rb_Tree Treap Rand_Tree

  @ingroup Arboles
*/
template <
  typename Key, typename Data,
  template <typename, class> class Tree = Avl_Tree,
  class Compare                         = Aleph::less<Key>>
class DynMapTree : 
public DynSetTree<std::pair<Key, Data>, Tree, Dft_Pair_Cmp<Key, Data, Compare>>
{
  typedef std::pair<Key, Data> Pair;

  typedef DynSetTree<Pair, Tree, Dft_Pair_Cmp<Key, Data, Compare>> Base;

public:

  typedef Key Key_Type;

  typedef Key Item_Type;

  typedef Data Value_Type;

  using Base::Base;  

    Data & get_data(const Key & key)
    {
      return key_to_pair<Key, Data>(&const_cast<Key&>(key))->second;    
    }

    const Key & get_key(Data * data_ptr)
    {
      return data_to_pair<Key, Data>(data_ptr)->first;
    }

/** 
  @brief Inserts a pair in the mapping.

  @details insert(key,data) inserted in the mapping a new pair (key,data)
  indexed by the key value. The insertion operation is not performed if
  the key value is already present in the mapping.

  @param[in] key The key to insert.
  @param[in] data Value to index by key "key".

  @return The number of elements that contains the mapping.

  @throw bad_alloc If there is insufficient memory.
*/
  Key * insert(const Key & key, const Data & data)
  {
    Pair * p = this->Base::insert(Pair(key, data));
    return p != NULL ? &p->first : NULL;
  }

  Key * insert(const Key & key, Data && data = Data())
  {
    Pair * p = this->Base::insert(Pair(key, std::move(data)));
    return p != NULL ? &p->first : NULL;
  }
  
  Key * insert(Key && key, const Data & data)
  {
    Pair * p = this->Base::insert(Pair(std::move(key), data));
    return p != NULL ? &p->first : NULL;
  }

  Key * insert(Key && key, Data && data = Data())
  {
    Pair * p = this->Base::insert(Pair(std::move(key), std::move(data)));
    return p != NULL ? &p->first : NULL;
  }

/** 
  @brief Look for the <code>"key"</code> key in the binary tree search or
  insert if not found.

  @details <code>search_or_insert(key)</code> search the binary tree a
  node whose key is <code>KEY(p)</code>.
  If the key is found, then it returns a pointer to it; otherwise
  inserting <code>key</code> in the binary search tree this.

  @param[in] key The key to search or insert.

  @return A pointer to the key in the tree
*/
  Key * search_or_insert(const Key & key, const Data & data)
  {
    return &this->Base::search_or_insert(Pair(key, data))->first;
  }

  Data * search_or_insert(const Key & key, Data && data)
  {
    return &this->Base::search_or_insert(Pair(key, std::move(data)))->first;
  }

  Data * search_or_insert(Key && key, const Data & data)
  {
    return &this->Base::search_or_insert(Pair(std::move(key), data))->first;
  }

  Data * search_or_insert(Key && key, Data && data)
  {
    return &search_or_insert(Pair(std::move(key), std::move(data)))->first;
  }

/** 
  @brief Inserts a pair in the mapping.

  @details put(key,data) inserted in the mapping a new pair (key,data)
  indexed by the key value. The insertion operation is not performed if
  the key value is already present in the mapping.

  @param[in] key The key to insert.
  @param[in] data Value to index by key "key".

  @return The number of elements that contains the mapping.

  @throw bad_alloc If there is insufficient memory.
*/
  Key * put(const Key & key, const Data & data)
  {
    return insert(key, data);
  }

  Key * put(const Key & key, Data && data)
  {
    return insert(key, std::move(data));
  }

  Key * put(Key && key, const Data & data)
  {
    return insert(std::move(key), data);
  }

  Key * put(Key && key, Data && data)
  {
    return insert(std::move(key), std::move(data));
  }

/** 
  @brief Eliminates data associated with the key "key".

  @details remove(key) eliminates of mapping the data associated with 
  the "key" key.

  @param[in] key The key to delete.

  @return The number of elements that contains the mapping.
*/
  size_t remove(const Key & key)
  {
    return this->Base::remove(Pair(key, Data()));
  }

/** 
  @brief Returns true if key is present in the mapping. 
*/
  bool test_key(const Key & key) const
  {
    return this->Base::exist(Pair(key, Data()));
  }

  bool has(const Key & key) const { return test_key(key); }

/** 
  @brief Returns data associated with a key.

  @details test(key) returns a pointer to the data value associated with the
  key "key".

  @param[in] key The key to look for.

  @return A pointer to the data associated with a key if the key exists
  in mapping; NULL otherwise.
*/
  Data * test(const Key & key)
  {
    Pair * p = this->Base::search(Pair(key, Data()));
    return p != NULL ? &p->second : NULL;
  }

/** 
  @brief Returns data associated with a key.

  @details search(key) returns a pointer to the data value associated with
  the key "key".

  @param[in] key The key to look for.

  @return A pointer to the data associated with a key if the key exists
  in mapping; NULL otherwise.

  @see test
*/
  Data * search(const Key & key) const
  {
    Pair * p = this->Base::search(Pair(key, Data()));
    return p != NULL ? &p->second : NULL;
  }

/** 
  @brief Returns data associated with a key.

  @details search(key) returns a pointer to the data value associated with
  the key "key".

  @param[in] key The key to look for.

  @return A pointer to the data associated with a key if the key exists
  in mapping; NULL otherwise.
  
  @throw domain_error If the key is not in the mapping
*/
  Data & find(const Key & key) 
  { 
    return this->Base::find(Pair(key, Data())).second;
  }

  Map_Sequences_Methods();

  Generate_Proxy_Operator(DynMapTree);
};
} /* End namespace Aleph */


# include <tpl_binTree.H>
# include <tpl_avl.H>
# include <tpl_rb_tree.H>
# include <tpl_rand_tree.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_splay_tree.H>

namespace Aleph {
/**
  @brief Dynamic mapping implemented with classic binary trees search.

  @see BinTree DynMapTree
   
  @ingroup Arboles
*/
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapBinTree : public DynMapTree<Key, Type, BinTree, Compare> {};

/**
  @brief Dynamic mapping implemented with binary trees of AVL search.

  @see Avl_Tree DynMapTree

  @ingroup Arboles
*/
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapAvlTree : public DynMapTree<Key, Type, Avl_Tree, Compare> {};
/**
  @brief Dynamic mapping implemented with binary trees of red-black search.

  @see Rb_Tree DynMapTree

  @ingroup Arboles
*/
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
    class DynMapRbTree : public DynMapTree<Key, Type, Rb_Tree, Compare>
    { /* Empty */ };

/**
  @brief Dynamic mapping implemented with binary trees of randomized search.

  @see Rand_Tree DynMapTree

  @ingroup Arboles
*/
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
    class DynMapRandTree : public DynMapTree<Key, Type, Rand_Tree, Compare>
    { /* Empty */ };

/**
  @brief Dynamic mapping implemented with binary trees of treaps search.

  @see Treap DynMapTree

  @ingroup Arboles
*/
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
    class DynMapTreap : public DynMapTree<Key, Type, Treap, Compare>
    { /* Empty */ };

/**
  @brief Dynamic mapping implemented with binary trees of treaps and 
  with range search. It offers the operation position.

  @see Treap_Rk DynMapTree

  @ingroup Arboles
*/
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
    class DynMapTreapRk : public DynMapTree<Key, Type, Treap_Rk, Compare>
    { /* Empty */ };

/**
  @brief Dynamic mapping implemented with binary trees of splay search.

  @see Splay_Tree DynMapTree

  @ingroup Arboles
*/
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
    class DynMapSplayTree : public DynMapTree<Key, Type, Splay_Tree, Compare>
    { /* Empty */ };

} /* End namespace Aleph */

# endif /* TPL_DYNMAPTREE_H */
