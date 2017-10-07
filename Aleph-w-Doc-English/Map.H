
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
  @file Map.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef ALEPH_MAP_H
# define ALEPH_MAP_H

    //    /*************************************************
	  //    Aleph implementation of map C++ standard container
    //    *************************************************/

# include <stdlib.h>
# include <ahFunction.H>
# include <ah_stdc++_utils.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>


namespace Aleph
{


/** 
  @briefImplementation Aleph standard type map <T> based on trees
  binary search ranges.

  @details This is a partial implementation of the standard C ++ class map <Key, Elem>,
  which it is based on binary search trees ranges. By
  Treap_Rk default the class is used, with which in measures
  performance has proven to be faster than their counterparts and gnu
  Boost.

  map <Key, Elem> implements a mapping from keys of type Key, not
  can be repeated, a mapping type Elem elements, the
  which can eventually be repeated if associated with key
  different.

  Mapping elements are ordered pairs. A mapping is synonymous
  Function from Key -> Elem. This version uses the type mapping
  Pair of Aleph, whose implementation is based on GNU.

  A mapping m is Reachable by operator m [key] where key is
  the key indexing. The result of m [key] is the element
  key associated with the key.

  @param Key Data type mapping store.
  @param Elem Type element indexed by a key.
  @param compare comparison between keys
  Key type. By default Key :: operator <(const Key &) is implanted.
  @param Class Tree binary search tree with ranges
  It will implement the mapping. By default if you use Treap_Rk

  @see Treap_Rk September multimap multiset
  
  @ingroup stl
*/
    template <class Key, 
	      class Elem, 
	      class Compare = Aleph::less<Key>,
	      template <class, class> class Tree = Treap_Rk
    >
class map
{

  struct Cmp
  {
    bool
    operator () (const std::pair<Key, Elem> & __x, 
		 const std::pair<Key, Elem> & __y) const
    { 
      return Compare () (__x.first, __y.first); 
    }
  };

public:

/** 
  @typedef Type pair stored in mapping. 
*/
  typedef std::pair<Key, Elem> Pair;

/** 
  @typedef Type exporting container type. 
*/
  typedef Pair value_type;

/** 
  @typedef Type generic reference to the type of item that stores the mapping. 
*/
  typedef typename map::value_type & reference;

/** 
  @typedef Generic type constant reference. 
*/
  typedef const typename map::value_type & const_reference;

  typedef size_t size_type;

/**  
  @typedef The key type indexing. 
*/
  typedef Key key_type;

/** 
  @typedef The type of mapping range. 
*/
  typedef Elem mapped_type;

private:

  typedef Tree<Pair, Cmp> Tree_Type;

  typedef typename Tree_Type::Node Node;

  Node_Pool<Node> node_pool;

  mutable Tree_Type tree;

  Node * search_in_tree (const Key & k)
  {
    return tree.search (Pair(k, Elem()));
  }

public:

/** 
  @brief Iterator for mapping.

	@ingroup stl
*/
  class iterator
  {
  private:

    friend class map<Key, Elem>;
    
    typedef typename Tree_Type::Iterator Iterator;
    
    Iterator itor;
    bool underflow;
    bool overflow;

    void init_flags ()
    {
      if (itor.has_current () )
	underflow = overflow = false;
      else
	underflow = overflow = true;
    }

    void goto_begin ()
    {
      itor.reset_first ();
      init_flags ();
    }

    void goto_last ()
    {
      itor.reset_last ();
      init_flags ();
    }

    void goto_end ()
    {
      itor.reset_last ();
      init_flags ();
      if (not overflow) 
	itor.next ();
      overflow = true;
    }

    void forward ()
    {
      if (underflow)
	{
	  goto_begin ();
	  return;
	}
      
      itor.next ();

      if (not itor.has_current () )
	overflow = true;
    }

    void backward ()
    {
      if (overflow)
	{
	  goto_last ();
	  return;
	}
      
      itor.prev ();

      if (not itor.has_current () )
	underflow = true;
    }

    iterator (Tree_Type & _tree, Node * node) 
      : itor (_tree, node), underflow (false), overflow (false)
    {
      /* Empty */
    }

  public:

/** 
  @typedef The type of item that handles the iterator. 
*/
    typedef typename map::value_type   value_type;

/** 
  @typedef The numeric arithmetic to handle. 
*/
    typedef typename map::size_type    difference_type;

/** 
  @typedef Data rate pointer to current element. 
*/
    typedef typename map::value_type * pointer;

/** 
  @typedef Data type refers to current element. 
*/
    typedef typename map::reference    reference;

/** 
  @brief Empty constructor;
  It does not apply if a mapping is not associated. 
*/
    iterator () { /* Empty */ }

    iterator (Tree_Type & tree) : itor (tree)
    {
      init_flags ();
    }

/** 
  @brief It provides a reference to the current item. 
*/
    const Pair & operator * ()
    {
      return KEY(itor.get_current());
    }

/** 
  @brief "Dereference" a pointer to the current item. 
*/
    const Pair * operator -> ()
    {
      return &KEY(itor.get_current());
    }

/** 
  @brief Advances the iterator a forward position,
  then loads the current item. 
*/
    iterator operator ++ ()
    {
      forward ();
      return *this;
    }

/** 
  @brief Current load element then moves the iterator forward. 
*/
    iterator operator ++ (int)
    {
      iterator ret_val = *this;
      forward ();
      return ret_val;
    }

/** 
  @brief Advances the iterator a rearward position,
  then load current item. 
*/
    iterator operator -- ()
    {
      backward ();
      return *this;
    }

/** 
  @brief Current load element then moves the iterator back. 
*/
    iterator operator -- (int)
    {
      iterator ret_val = *this;
      backward ();
      return ret_val;
    }
    
/** 
  @brief Advances the iterator n elements forward and load the new current element. 
*/
    iterator operator += (const size_type & n)
    {
      itor.reset_to_pos (itor.get_current_position () + n);
      return *this;
    } 

/** 
  @brief Advances the iterator n elements back and load the new current element. 
*/
    iterator operator -= (const size_type & n)
    {
      itor.reset_to_pos (itor.get_current_position () - n);
      return *this;
    } 

/** 
  @brief Returns true if the iterator have the same status. 
*/
    bool operator == (const iterator & _itor) const
    {
      return itor == _itor.itor;
    }

/** 
  @brief Returns true if the iterator have different states. 
*/
    bool operator != (const iterator & _itor) const
    {
      return not (itor == _itor.itor);
    }

    bool verify (const map & _map) const
    {
      return itor.verify ( (Tree_Type*) &_map.tree); 
    }

    bool verify (const iterator & it) const
    {
      return itor.verify (it.itor);
    }
  };

/** 
  @brief empty constructor. 
*/
  map () : node_pool(100) { /* Empty */ }
 
/** 
  @brief Request a copy of the mapping c. 
*/
  map (const map & c) : node_pool(100)
  {
    tree.getRoot () = copyRec (c.tree.getRoot () );
  }


/** 
  @brief destroyer;
  All elements are eliminated and the memory is released. 
*/
  ~map () 
  { 
    destroyRec (tree.getRoot () );
  }  

/** 
  @brief Assigns the entire contents of c to This. 
*/
  map & operator = (const map& c)
  {
    if (this == &c)
      return *this;

    destroyRec (tree.getRoot ());

    tree.getRoot () = copyRec (c.tree.getRoot ()); 

    return *this;
  }

/** 
  @brief Returns the number of elements that contains the mapping. 
*/
  size_t size () const { return COUNT (tree.getRoot () ); }

/** 
  @brief Returns true if the container is empty. 
*/
  bool empty () const 
  {
    return size() == 0;
  }

/** 
  @brief Returns true if the mapping this has exactly the
  mapping same elements c. 
*/
  bool operator == (const map & c) const
  {
    if (size () != c.size () )
      return false;

    typename Tree_Type::Iterator itor1 (tree), itor2 (c.tree);
    
    while (itor1.has_current () and itor2.has_current () )
    {
      if (KEY (itor1.get_current () ) != KEY (itor2.get_current () ) )
	return false;

      itor1.next ();
      itor2.next ();
    }
    
    assert(not itor1.has_current () and not itor2.has_current () );

    return true;
  }

/** 
  @brief Returns true if the mapping contains exactly this
  mapping same elements c. 
*/
  bool operator != (const map & c) const
  {
    return not *this == c;
  }

/** 
  @brief Returns true if the ordered items
  this mapping are lower than those of the mapping c. 
*/
  bool operator < (const map & c) const
  {
    typename Tree_Type::Iterator itor1 (tree), itor2 (c.tree);

    while (itor1.has_current () and itor2.has_current () )
    {
      if (KEY (itor1.get_current () ) < KEY (itor2.get_current () ))
	return true;
      else if (KEY (itor2.get_current () ) < KEY (itor1.get_current () ))
	return false;
      
    /* In this case the keys equal sons */
      itor1.next ();
      itor2.next ();
    }
    
    if (not itor1.has_current ())
      return true;

    return false;
  }

/** 
  @brief Returns true if the ordered items
  mapping this outweigh the mapping c
*/
  bool operator > (const map & c)
  {
    return not (*this == c or *this < c);
  }

/** 
  @brief Returns true if the ordered items
  this mapping is less than or equal to the mapping c 
*/
  bool operator <= (const map & c)
  {
    return not (*this > c );
  }

/** 
  @brief Returns true if the ordered items
  this mapping are greater or equal to the mapping c 
*/
  bool operator >= (const map & c)
  {
    return not (*this < c);
  }

/** 
  @brief This function is itself an absolute search whose result is 0 or 1.
  
  @details Returns the number of instances of the key is inside
  mapping; that is, by definition set <T>, 1 if the element
  It is within the mapping; 0 otherwise.

  @param [in] key search key value.

  @return 1 if the element is in the mapping; 0 otherwise.
*/
  size_type count (const Key & key)
  {
    if (search_in_tree (key) != NULL)
      return 1;

    return 0;
  } 

/** 
  @brief Returns an iterator positioned on the key element mapping.

  @details find (key) seeks the key element in the mapping. Yes this is
  It found, then a positioned iterator returns on
  mapping element whose value is key; otherwise,
  an overflowing iterator is returned.

  @param [in] key Value to search within key mapping.

  @return The iterator positioned on the item with key value
  iterator overwhelmed or if key is not within the
  mapping.
*/
  iterator find (const Key & key) 
  { 
    Node * p = search_in_tree (key);

    if (p == NULL)
      return end ();

    return iterator (tree, p);
  }
/** 
  @brief Returns an iterator positioned on the predecessor key in the mapping;
  key is whether or not in the mapping. 
*/
  iterator lower_bound (const Key & key)
  {
    Node * p = 
      search_rank_parent <Node, Cmp> (tree.getRoot(), Pair(key, Elem()));

    return iterator(tree, p);
  }

/** 
  @brief Returns an iterator positioned on the successor key in the mapping;
  key is whether or not in the mapping. 
*/
  iterator upper_bound (const Key & key)
  {
    Node * p = 
      search_rank_parent <Node, Cmp> (tree.getRoot(), Pair(key, Elem()));

    iterator upper(tree, p);
    
    if (KEY (p).first == key)
      upper.itor.next();

    return upper;
  }

/** 
  @brief Interact constant time all the elements of c this. 
*/
  void swap (map & c)
  {
    tree.swap (c.tree);
  }

/** 
  @brief Returns a first element positioned at mapping iterator. 
*/
  iterator begin ()
  {
    return iterator (tree);
  }

/** 
  @brief Returns an iterator positioned at the last item mapping. 
*/
  iterator end ()
  {
    iterator last (tree);
    last.goto_end ();

    return last;
  }

/** 
  @brief Enter key in the mapping.

  @details Insert (key) mapping esta inserted in a copy of the
  key element and returns a pair with a positioned iterator
  by insertion and a logical value that indicates whether the
  insertion was successful or not.

  @param [in] key value inserted into the mapping.
  
  @return A couple with an iterator and a logical value. If no key
  It is within the mapping, then the iterator is
  positioned on the newly inserted element and the value
  logic is true; otherwise, the iterator is overwhelmed and
  the logical value is false.
  
  @throw bad_alloc If there is insufficient memory.
*/
  std::pair<iterator, bool> insert (const Pair & key)
  {
    Node * p = node_pool.allocate(key);

    if (tree.search_or_insert(p) != p)
    {
      node_pool.deallocate(p);
      return std::pair<iterator, bool> (iterator (tree), false);
    }

    return std::pair<iterator, bool> (iterator(tree, p), true);
  }

/** 
  @brief Instance mapping the items within the range [beg..end).

  @details This constructor instantiates a mapping with initial elements
  copied from the associated container beg and end iterators.

  @param [in] beg Iterator positioned at the first item
  container copied.
  @param [in] end Iterator positioned at the last plus
  copying the container element.
 
  @throw domain_error If beg and end iterators are not
  associated with the same container.
  @throw bad_alloc if there is insufficient memory.

  @note The iterator positioned by the end element is not included.
*/
      template <typename Itor> 
  map (Itor beg, const Itor & end) : map()
  {
    while (beg != end)
      insert(*beg++);
  }
    
/** 
  @brief Enter key in the mapping.

  @details insert (pos, key) inserted into the back of esta mapping
  key element and returns a pair with a positioned iterator
  by insertion and a logical value that indicates whether the
  insertion was successful or not.

  Actually, the iterator is irrelevant in the case of set <T>,
  because no duplication allowed. This scheme insertion
  It makes sense in multiset and multimap. It is provided for reasons of
  code compatibility and economy.

  @param [in] key value inserted into the mapping.

  An iteratorreturn the element positioned just
  key inserted if not within the mapping; than
  Otherwise, the iterator is delivered overflowed.

  @throw bad_alloc If there is insufficient memory.
*/  
  iterator insert (iterator /* Pos */, const Pair& key)
  {
    return insert(key);
  }

/** 
  @brief inserted in mapping elements of a container
  within the range [beg..end).

  @details insert (beg, end) takes a range of items in a container
  (set, map, multiset, multimap, list or table) associated with
  iterators beg and end, and insert all the elements of the range
  in mapping esta.

  @param [in] beg Iterator positioned on the start element
  range.
  @param [in] end Iterator positioned on the last plus
  final element of the range.

  @throw domain_error If beg and end iterators are not
  associated with the same container.
  @throw bad_alloc If there is insufficient memory.

  @note The iterator positioned by the end element is not included
*/
      template <typename Itor>
  void insert (Itor beg, const Itor & end)
  {
    while (beg != end)
      insert (*beg++);
  }

/** 
  @brief Delete key mapping.

  @details erase (key) deletes the key element mapping.

  @param [in] key Value of the item to delete.
  
  @return 1 if key was inside the mapping; or else
  otherwise.

  @note Actually the return value is meaningless for
  multiset and multimap in which the return value is the
  amount of the element instances that were deleted.
*/
  size_type erase (const Key & key)
  {
    Node * p = tree.remove (Pair(key, Elem()));

    if (p == NULL)
      return 0;
	
    node_pool.deallocate(p);

    return 1;
  }

/** 
  @brief mapping Clears the current value of the iterator pos.

  @details erase (pos) deletes the corresponding element mapping
  current value of the iterator pos.

  @param [in] pos Iterator whose current value is deleted.

  @throw domain_error If the iterator is not associated with the mapping esta.
  @throw underflow_error If the iterator is overwhelmed by the left.
  @throw overflow_error If the iterator is overwhelmed by the right.
*/
  void erase (iterator pos)
  {
    erase (KEY (pos.itor.get_current ()).first);
  }

/** 
  @brief Deletes mapping the range [beg..end).

  @details erase (beg, end) efficiently removes all elements
  delimited mapping the beg and end iterators.

  @param [in] beg Iterator element positioned on the start of the range.
  @param [in] end iterator positioned on the last one more element of the range.
    
  @throw domain_error If beg and end iterators are not associated with esta.
    
  @note The iterator positioned by the end element is not included.
*/
  iterator erase (const iterator & beg, const iterator & end)
  {
    Aleph::verify_iterators (beg, end);
    Aleph::verify_container_and_iterator (*this, beg);

    iterator ret_val = end;

    const size_t pos_beg = beg.itor.get_current_position ();
    const size_t pos_end = end.itor.get_current_position ();

    Node * removed_tree = tree.remove (pos_beg, pos_end - 1);

    destroyRec (removed_tree);

    return ret_val;
  }

/** 
  @brief Deletes all elements of mapping. 
*/   
  void clear ()
  {
    destroyRec (tree.getRoot());
  }
  
private:
  
  struct Proxy
  {
    map  * map_ptr;
    Node * node;
    Key    key;

    Proxy (map * m_ptr, const Key & _key) : map_ptr (m_ptr), key (_key)
    {
      node = map_ptr->search_in_tree(key);
    }

    Proxy & operator = (const Elem & elem)
    {
      if (node == NULL)
        {
	  node = map_ptr->node_pool.allocate(std::make_pair(key, elem));
	  map_ptr->tree.insert(node);
	}
      else
        KEY(node).second = elem;

      return *this;
    }

    Proxy & operator = (const Proxy & proxy)
    {
      if (this == &proxy)
	return *this;

      if (proxy.node == NULL)
        throw std::domain_error("key not found");;

      if (map_ptr == proxy.map_ptr and key == proxy.key)
        return *this; /* They are the same */

      if (node == NULL)
        {
	  node = map_ptr->node_pool.allocate(KEY(proxy.node));
	  map_ptr->tree.insert(node);
	} 
      else
        KEY(node).second = KEY(proxy.node).second;

      return *this;
    }

    operator Elem & () const
    {
      if (node == NULL)
        throw std::domain_error ("key not found");;

      return KEY (node).second;
    }    
  };

public:

  const Proxy operator [] (const Key & key) const 
    Exception_Prototypes (std::domain_error)
  {
    return Proxy (this, key);
  }

  Proxy operator [] (const Key & key) 
    Exception_Prototypes (std::domain_error)
  {
    return Proxy (this, key);
  }
};



} /* End namespace Aleph */


# endif // ALEPH_MAP_H
