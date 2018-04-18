
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
  @file Set.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


    //    /**************************************************
	  //    Aleph implementation of C ++ standard container set 
    //    **************************************************/

# ifndef AH_SET_H
# define AH_SET_H

# include <ah_stdc++_utils.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>

namespace Aleph
{

/** 
  @brief Aleph implementation of the standard type set <T> based on trees
  binary search ranges.

  @details This is a partial implementation of the standard C ++ class set <T>
  based on binary search trees ranges. 
  by Treap_Rk default the class is used, with which in measures
  performance has proven to be faster than their counterparts and gnu
  Boost. 

  set<T> implements a set of keys of type T in which there is
  allow repetitions.

  @param T Data type that will store the set.
  @param Compare Comparison criterion elements T. 
  The default type is implanted T::operator<(const T&).
  @param Tree The class of binary search tree with ranges
  It will implement the set. By default if you use Treap_Rk

  @see Treap_Rk map multiset multimap
 
  @ingroup Contenedores
*/
    template <typename T, 
	      class Compare = Aleph::less<T>,
	      template <class, class> class Tree = Treap_Rk>
class set
{
private:

  Tree<T, Compare> tree;

  typedef typename Tree<T, Compare>::Node Node;

  Node_Pool<Node> node_pool;

public:

/** 
  @typedef Data type array elements.
*/
  typedef T value_type;

/** 
  @typedef Generic pointer to array element. 
*/
  typedef typename set::value_type * pointer;

/** 
  @typedef Generic reference element of the set. 
*/
  typedef typename set::value_type & reference; 

/** 
  @typedef Generic reference to constant element of the set. 
*/
  typedef const typename set::value_type & const_reference;

/** 
  @typedef Numeric type for sizes (natural). 
*/
  typedef size_t size_type;

/** 
  @typedef Data type array elements.
*/
  typedef T key_type;

  class iterator
  {
  private:

    friend class set<T, Compare, Tree>;
    
    typedef typename Tree<T, Compare>::Iterator Iterator;
    
    Tree<T, Compare> * tree;
    Iterator  itor;
    bool underflow;
    bool overflow;

    iterator (Tree<T, Compare> & _tree, Node * node) 
      : tree (&_tree), itor (_tree, node), underflow (false), overflow (false)
    {
      /* Empty */
    }

    void init_flags ()
    {
      if (tree->size () == 0)
	underflow = overflow = true;
      else
	underflow = overflow = false;
    }

    iterator (Tree<T, Compare> & _tree) : tree (&_tree), itor (_tree)
    {
      init_flags ();
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

      if (not itor.has_current() )
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

      if (not itor.has_current() )
	underflow = true;
    }

  public:

/** 
  @typedef Data type that contains the set.
*/
    typedef typename set<T>::value_type   value_type;

/** 
  @typedef Numeric type used for differences between positions iterators. 
*/
    typedef typename set<T>::size_type       difference_type;

/** 
  @typedef Type pointer to element within the set.
*/
    typedef typename set<T>::value_type *    pointer;

/** 
  @typedef Reference type element in the set.
*/
    typedef typename set<T>::reference       reference;

/** 
  @typedef Reference type element in the set.
*/
    typedef const typename set<T>::reference const_reference;

/** 
  @brief Empty constructor; It does not apply if a group is not associated.
*/
    iterator () : tree (NULL), underflow (true), overflow (true) 
    {
      /* Empty */ 
    }
    
/** 
  @brief It provides a reference to the current item.
*/
    const T & operator * ()
    {
      return KEY (itor.get_current () );
    }

/** 
  @brief "Dereferencing" a pointer to the current item.
*/
    const T * operator -> ()
    {
      return & KEY (itor.get_current () );
    }

/** 
  @brief Advances the iterator a forward position,
  then load current item.
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
      iterator return_value = *this;
      forward ();
      return return_value;
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
      iterator return_value = *this;
      backward ();
      return return_value;
    }
    
/** 
  @brief Advances the iterator n elements forward and
  load current new element.
*/
    iterator operator += (const size_type & n)
    {
      itor.reset_to_pos(itor.get_current_position () + n);
      return *this;
    } 

/** 
  @brief Advances the iterator n elements back and
  load current new element. 
*/
    iterator operator -= (const size_type & n)
    {
      itor.reset_to_pos(itor.get_current_position () - n);
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

    bool verify (const set & _set) const
    {
      return itor.verify ( (Tree<T, Compare>*) &_set.tree);
    }

    bool verify (const iterator & it) const
    {
      return itor.verify (it.itor);
    }
  };

/** 
  @brief Empty constructor. 
*/
  set () : node_pool(100) { /* Empty */  }

/** 
  @brief Request a copy of the set c. 
*/
  set (const set & c) : set()
  {
    void * ptr = c.tree.getRoot();
    tree.getRoot () = copyRec ((Node*) ptr);
  }

/** 
  @brief Destructor
  
  @details All elements are eliminated and the memory is released.
*/
  ~set () 
  { 
    destroyRec(tree.getRoot () );
  }  

/** 
  @brief Returns the number of items contained in the set. 
*/
  size_type size () { return COUNT (tree.getRoot () ); }

/** 
  @brief Returns true if the container is empty. 
*/
  bool empty () 
  {
    return COUNT (tree.getRoot () ) == 0;
  }

/** 
  @brief Returns true if the set this contains exactly
  the same elements of the set c.
*/
  bool operator == (const set & c) const
  {
    if (this == &c)
      return true;

    if (size () != c.size () )
      return false;

    typename Tree<T, Compare>::Iterator it1 (tree), it2 (c.tree);

    while (it1.has_current () and it2.has_current () )
      {
	if (Aleph::no_equals<T, Compare> (KEY(it1.get_curr()), 
					  KEY(it2.get_curr())))
	  return false;

	it1.next ();
	it2.next ();
      }
    
    return true;
  }

/** 
  @brief Returns true if the this set contains exactly 
  the same elements of the set c. 
*/
  bool operator != (const set & c) const
  {
    return not (*this == c);
  }

/** 
  @brief Returns true if the ordered items of this set are
  lower than the set c. 
*/
  bool operator < (const set & c) const
  {
    if (this == &c)
      return false;

    typename Tree<T, Compare>::Iterator itor1 (tree), itor2 (c.tree);

    while (itor1.has_current () and itor2.has_current () )
      {
	if (Compare () (KEY(itor1.get_curr()), KEY(itor2.get_curr())))
	  return true;
	else if (Compare () (KEY(itor2.get_curr()), KEY(itor1.get_curr())))
	  return false; 

	itor1.next ();
	itor2.next ();
      }
    
    if (itor1.has_current () ) /* |*this| >= |c| */
      return false;

    return itor2.has_current ();
  }

/** 
  @brief Returns true if the ordered items of the this set are
  larger than c.
*/
  bool operator > (const set & c) const
  {
    return not (*this == c or *this < c);
  }

/** 
  @brief Returns true if the ordered items of the this set are 
  less than or equal to the set c.
*/
  bool operator <= (const set & c) const
  {
    return not (*this > c );
  }

/** 
  @brief Returns true if the ordered items of the this set are
  greater or equal to the set c.
*/
  bool operator >= (const set & c) const
  {
    return not (*this < c);
  }
  
/** 
  @brief Returns the number of instances of value is inside the
  set; that is, by definition set <T>, 1 if the element
  it is within the set; 0 otherwise.

  @details This function is itself an absolute search whose result is 0 or 1.
  
  @param[in] value Valor a buscar.
  
  @return 1 si el elemento se encuentra en el conjunto; 0 de lo
  contrario. 
*/
  size_type count (const T & value) 
  {
    if (tree.search (value) == NULL)
      return 0;

    return 1;
  } 

/** 
  @brief Returns an iterator positioned on the element of the set
  value.

  find (value) looks for the value element in the set. Yes this is
  It found, then a positioned iterator returns on
  element whose value is set value; otherwise,
  an overflowing iterator is returned.

  @param[in] value Value to search within the set
  
  @return An iterator positioned on the element with value value
  or iterator overwhelmed if value is not within the set.
*/
  iterator find(const T & value) 
  { 
    Node * node = tree.search (value);

    if (node == NULL)
      return end();

    iterator itor (tree);
    itor.itor.reset_to_node (node);

    return itor;
  }

/** 
  @brief Returns an iterator positioned on the predecessor of value in the set;
  value is whether or not in the set. 
*/
  iterator lower_bound(const T & value)
  {
    if (size() == 0)
      return end ();

    Node * p = search_rank_parent(tree.getRoot(), value);

    return iterator (tree, p);
  }

/** 
  @brief Returns an iterator positioned on the successor of value in the set;
  value is whether or not in the set. 
*/
  iterator upper_bound(const T & value)
  {
    if (size () == 0)
      return end ();

    Node * p = search_rank_parent(tree.getRoot(), value);

    iterator upper(tree, p);
    
    if (KEY(p) == value)
      upper.itor.next();

    return upper;
  }

/** 
  @brief Assigns the entire contents of c to This.
*/
  set & operator = (set & c)
  {
    if (this == &c)
      return *this;

    destroyRec (tree.getRoot () );

    tree.getRoot () = copyRec (c.tree.getRoot () ); 

    return *this;
  }

/** 
  @brief Interact constant time all esta elements with those of c.
*/
  void swap (set & c)
  {
    std::swap (tree.getRoot (), c.tree.getRoot () );
  }

/** 
  @brief Returns an iterator positioned at the first element of the set.
*/
  iterator begin () 
  {
    return iterator (tree);
  }

/** 
  @brief Returns an iterator positioned at the last item in the set.
*/
  iterator end ()
  {
    iterator last (tree);
    last.goto_end ();

    return last;
  }

/** 
  @brief Enter value in the set.

  @details insert(value) inserted in a copy of the this set
  item value and returns a pair with a positioned iterator
  by insertion and a logical value that indicates whether the
  insertion was successful or not.

  @param[in] value Value inserted in the set.

  @return A couple with an iterator and a logical value. If no value
  is within the set, then the iterator is
  positioned on the newly inserted element and the value
  logic is true; otherwise, the iterator is overwhelmed and
  the logical value is false.

  @throw bad_alloc If there is insufficient memory.
*/
  std::pair<iterator, bool> insert (const T & value)
  {
    Node * p = node_pool.allocate(value);

    iterator itor (tree);

    if (tree.search_or_insert(p) != p) /* It is inserted or is already in the set? */
      {  /* Value is already in the set */
	node_pool.deallocate(p);
	itor.itor.reset_to_key (value);
	
	return std::pair<iterator, bool> (itor, false);
      }

    itor.itor.reset_to_node(p);

    return std::pair<iterator, bool> (itor, true);
  }


/** 
  @brief Instance Together with the elements included in the
  range [beg..end).

  @details This constructor instantiates A with initial set elements
  copied from The Associated container beg and end iterators.

  @param[in] beg Iterator positioned at the first item
  container copied.
  @param[in] end Iterator positioned at the last plus
  copying the container element.

  @throw domain_error If beg and end iterators are not
  associated with the same container.
  @throw bad_alloc If there is insufficient memory.

  @note The positioning by the end iterator item is not included.
*/
      template <typename Itor> 
  set (Itor beg, const Itor & end) : set()
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      insert(*beg++) ;
  }
    
/** 
  @brief Enter value in the set.

  @details insert(pos, value) inserted in a copy of the set esta
  item value and returns a pair with a positioned iterator
  by insertion and a logical value that indicates whether the
  insertion was successful or not.

  Actually, the iterator is irrelevant in the case of set <T>,
  because no duplication allowed. This scheme insertion
  It makes sense in multiset and multimap. It is provided for reasons of
  code compatibility and economy.

  @param[in] value Value inserted in the set.

  @return An iterator positioned on the element recently
  value inserted if not within the set; than
  Otherwise, the iterator is delivered overflowed.

  @throw bad_alloc If there is insufficient memory.
*/  
  iterator insert (const iterator & /* pos */, const T & value)
  {
    Node * p = node_pool.allocate(value);

    iterator _itor(tree);

    if (tree.search_or_insert(p) != p)
      { /* Duplicated key */
	node_pool.deallocate(p);
	_itor.itor.reset_to_key(value);
      }
    else
      _itor.itor.reset_to_node(p);   
  
    return _itor;    
  }

/** 
  @brief Inserted into the array elements of a container
  within the range [beg..end).

  @details insert(beg,end) making a range of items in a container
  (set, map, multiset, multimap, list or table) associated with
  iterators beg and end, and insert all the elements of the range
  in the set esta.

  @param[in] beg Iterator positioned on the start element
  range.
  @param[in] end Iterator positioned on the last plus
  final element of the range.

  @throw domain_error If beg and end iterators are not
  associated with the same container.
  @throw bad_alloc If there is insufficient memory.

  @note The positioning by the end iterator item is not included.
*/
      template <typename Itor>
  void insert (Itor beg, const Itor & end)
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      insert(*beg++);
  }

/** 
  @brief Clears the set value.

  @details erase(value) deletes the element of the set value.

  @param[in] value Value of the item to delete.

  @return 1 if value was within the set; or else
  otherwise.

  @note Actually the return value is meaningless for
  multiset and multimap in which the return value is the
  amount of the element instances that were deleted.
*/
  size_type erase (const T & value)
  {
    Node * p = tree.remove (value);

    if (p == NULL)
      return 0;
	
    node_pool.deallocate(p);

    return 1;
  }

/** 
  @brief Deletes the current value set iterator pos.

  @details erase(pos) deletes the corresponding item set to
  current value of the iterator pos.

  @param[in] pos Iterator whose current value is deleted.

  @throw domain_error If the iterator is not associated with
  the this set.
  @throw underflow_error If the iterator is overwhelmed by the
  left.
  @throw overflow_error If the iterator is overwhelmed by the
  right.
*/
  void erase (iterator pos)
  {
    Aleph::verify_container_and_iterator (*this, pos);

    node_pool.deallocate(pos.itor.del());
  }
  
/** 
  @brief Deletes assembly comprised range [beg..end).

  @details erase(beg,end) 
  effectively erases all elements
  delimited set the beg and end iterators.

  @param[in] beg 
  Iterator element positioned on the start of
  range.
  @param[in] end Iterator positioned on the last plus
  element of the range.

  @throw domain_error If beg and end iterators are not
  this associated.

  @note The positioning by the end iterator item is not included.
*/
  iterator erase (const iterator & beg, const iterator & end)
  {
    Aleph::verify_container_and_iterator (*this, beg);
    Aleph::verify_iterators (beg, end);

    iterator ret_val = end;

    const size_t pos_beg = beg.itor.get_current_position ();
    const size_t pos_end = end.itor.get_current_position ();

    Node * removed_tree = tree.remove(pos_beg, pos_end - 1);

    destroyRec(removed_tree);

    return ret_val;
  }

/** 
  @brief Deletes all elements of the set.
*/ 
  void clear ()
  {
    destroyRec(tree.getRoot());
  }
};


/** 
  @brief Distance between two iterators.

  @details distance(it1,it2) calculates the number of elements between the
  Current positions or elements of iterators it1 and it2 ..

  @param[in] it1 First iterator.
  @param[in] it2 Second iterator.

  @return The number of elements that separate the two iterators.

  @throw domain_error If iterators are not associated with it
  container. 

  @ingroup Contenedores
*/
    template <typename T>
typename iterator_traits<typename set<T>::iterator>::difference_type
distance(typename set<T>::iterator it1, typename set<T>::iterator it2)
{
  Aleph::verify_iterators(it1, it2);

  return it2.itor.get_current_position() - it1.itor.get_current_position();
}

}

# endif // AH_SET_H
