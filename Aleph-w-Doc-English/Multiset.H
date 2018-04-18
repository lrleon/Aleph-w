
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
  @file Multiset.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


    //    /******************************************************
    //    Aleph implementation of multiset C++ standard container
    //    ******************************************************/

# ifndef AH_MULTISET_H
# define AH_MULTISET_H

# include <memory>
# include <ah_stdc++_utils.H>
# include <tpl_dnode.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>

namespace Aleph
{

/** 
  @brief Implementation Aleph standard type multiset <T> based on trees
  binary search ranges.

  @details This is a partial implementation of the standard C ++ class multiset <T>
  based on binary search trees ranges. by
  Treap_Rk default the class is used, with which, in measures
  performance has proven to be faster than their counterparts and GNU
  Boost.

  multiset <T> implements a set of keys of type T in which
  allow repetitions.

  @param T Data type that will store the set.
  @param Compare criteria comparison between the elements
  T. The default type is implanted T :: operator <(const T &).
  @param Class Tree binary search tree with ranges
  It will implement the set. By default if you use Treap_Rk

  @see Treap_Rk Rand_Tree set map multimap 

  @ingroup stl
*/
    template <class T, 
	      class Compare = Aleph::less<T>,
	      template <class, class> class Tree = Treap_Rk
	      >
class multiset
{
  struct Node_Data
  {
    mutable T key;
    mutable size_t num_reps;

    Node_Data() : num_reps(0) { /* Empty */ }

    Node_Data(const T & k) : key(k), num_reps(1)  { /* Empty */ }
  };

  struct Cmp_Data
  {
    bool operator () (const Node_Data & op1, const Node_Data & op2) const
    {
      return Compare () (op1.key, op2.key);
    }
  };

public:

/** 
  @brief The data type of the array elements. 
*/
  typedef T value_type;

/** 
  @brief Generic reference to array element. 
*/
  typedef typename multiset::value_type & reference;

/** 
  @brief Generic reference to constant element of the set. 
*/
  typedef const typename multiset::value_type & const_reference;

/** 
  @brief Numeric type for sizes (natural). 
*/
  typedef size_t size_type;

/** 
  @brief The data type of the array elements. 
*/
  typedef T key_type;

private:

  typedef Tree<Node_Data, Cmp_Data> Tree_Type;

  typedef typename Tree_Type::Node Node;

  typedef typename Tree_Type::Iterator Tree_Iterator;

  mutable Tree_Type tree;

  mutable size_t num_elem;

  Node_Pool<Node> pool;

  static T & get_key(Node * p) { return KEY(p).key; }

  static size_t & get_num_reps(Node * p) { return KEY(p).num_reps; }

public:  

/** 
  @brief Iterator over a multi-set (multiset).

	@ingroup stl
*/
  class iterator
  {
  private:

    friend class multiset<T, Compare, Tree>;
    
    typedef typename Tree_Type::Iterator Iterator;
    
    mutable multiset * multiset_ptr;

    mutable Iterator tree_it;

    mutable int pos_in_node;

    bool overflow;
    bool underflow;

    iterator (multiset * mset_ptr, Node * curr_tree_node, int pos = 0)
      : multiset_ptr(mset_ptr), tree_it(mset_ptr->tree, curr_tree_node), 
	pos_in_node(pos), overflow (false), underflow (false)
    {
      /* Empty */
    }

    void default_init ()
    {
      if (tree_it.has_curr ())
	{
	  underflow = overflow = false;
	  pos_in_node = 0;
	}
      else
	underflow = overflow = true;
    }

    iterator(const multiset & ms)
      : multiset_ptr(const_cast<multiset*>(&ms)), tree_it(ms.tree)
    {
      default_init();
    }

    Node * get_curr_node() { return tree_it.get_curr(); }

    bool has_curr() const 
    {
      return tree_it.has_curr();
    }

    Node_Data & get_data() { return KEY(get_curr_node()); }

    T & get_key() { return multiset::get_key(get_curr_node()); }

    size_t & get_num_reps() { return multiset::get_num_reps(get_curr_node()); }

  public:

/** 
  @brief The data type that contains the multi-set. 
*/
    typedef typename multiset<T>::value_type      value_type;

/** 
  @brief Numeric type used for differences between positions iterators. 
*/
    typedef typename multiset<T>::size_type       difference_type;

/** 
  @brief Type pointer to element within the multi-set. 
*/
    typedef typename multiset<T>::value_type *    pointer;

/** 
  @brief Type reference element in the multi-set. 
*/
    typedef typename multiset<T>::reference       reference;

/** 
  @brief Type reference element in the multi-set. 
*/
    typedef const typename multiset<T>::reference const_reference;

    iterator(multiset * mset_ptr)
      : multiset_ptr(mset_ptr), tree_it(multiset_ptr->tree) 
    {
      default_init();
    }

    iterator(const iterator & it) 
      : multiset_ptr(it.multiset_ptr), tree_it(it.tree_it), 
	pos_in_node(it.pos_in_node), 
	overflow(it.overflow), underflow(it.underflow)
    {
      /* Empty */
    }

/** 
  @brief Empty constructor;It does not apply if a group is not associated. 
*/
    iterator () 
    {
      underflow = overflow = true;
      pos_in_node = -1;
    }

/** 
  @brief It provides a reference to the current item. 
*/
    const T & operator * ()
    {
      return get_key();
    }

/** 
  @brief "Dereference" a pointer to the current item. 
*/
    const T * operator -> ()
    {
      return & get_key();
    }

  private:

    void goto_begin ()
    {
      tree_it.reset_first ();
      if (tree_it.has_curr ())
	{
	  underflow = false;
	  pos_in_node = 0;
	}
      else
	{
	  underflow = true;
	  pos_in_node = -1;
	}
    }

    void goto_last ()
    {
      tree_it.reset_last ();
      if (tree_it.has_curr ())
	{
	  overflow = false;
	  pos_in_node = get_num_reps() - 1;
	}
      else
	{
	  overflow = true;
	  pos_in_node = -1;
	}
    }

    void goto_end ()
    {
      tree_it.reset_last();
      if (tree_it.has_curr()) 
	{ /* The tree is not empty */
	  tree_it.next (); /* Place tree_it out of range */
	  underflow = false;
	}
      else
	underflow = true;

      pos_in_node = -1;
      overflow = true;
    }

    iterator compute_end() const
    {
      iterator it(*this);
      it.goto_end();
      return it;
    }

    bool is_at_end() const
    {
      return not tree_it.has_curr();
    }

    iterator compute_begin() const
    {
      iterator it(*this);
      return it;
    }

    void forward ()
    {
      if (underflow)
	{
	  goto_begin ();
	  return;
	}

      if (++pos_in_node == get_num_reps())
	{ 
	  tree_it.next (); 

	  if (tree_it.has_curr ())
	    pos_in_node = 0;
	  else
	    {
	      overflow = true;
	      pos_in_node = -1;
	      assert(*this == compute_end());
	    }
	}
    }

    void backward ()
    {
      if (overflow)
	{
	  goto_last ();
	  return;
	}

      if (pos_in_node-- == 0)
	{
	  tree_it.prev ();

	  if (tree_it.has_curr ())
	    pos_in_node = get_num_reps();
	  else
	    underflow = true;
	}
    }

    void del()
    {
      Node * tree_node = get_curr_node();
      size_t & num_reps = multiset::get_num_reps(tree_node);

      --num_reps;

      --multiset_ptr->num_elem;

      if (num_reps == 0)
	{
	  tree_it.del();
	  multiset_ptr->pool.deallocate(tree_node);

	  if (tree_it.has_curr ())
	    pos_in_node = 0;
	  else
	    {
	      overflow = true;
	      pos_in_node = -1;
	    }
	}
    }

  public:

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
    T & operator -- (int)
    {
      iterator ret_val = *this;
      backward ();
      return ret_val;
    }
    
/** 
  @brief Advances the iterator n elements forward and load the new current item. 
*/
    iterator operator += (size_type n)
    {
      for (int i = 0; i < n; ++i)
        forward ();

      return *this;
    } 

/** 
  @brief Advances the iterator n elements back and load the new current element. 
*/
    iterator operator -= (size_type n)
    {
      for (int i = 0; i < n; ++i)
        backward ();
      
      return *this;
    } 

/** 
  @brief Returns true if the iterators have the same status. 
*/
    bool operator == (const iterator & it) const
    {
      if (this->has_curr() and it.has_curr())
	return pos_in_node == it.pos_in_node;

      if (this->is_at_end() and it.is_at_end())
	{
	  assert(this->overflow and it.overflow);
	  return true;
	}
      
      return false;
    }

/** 
  @brief Returns true if the iterator have different states. 
*/
    bool operator != (const iterator & itor) const
    {
      return not (*this == itor);
    }

    bool verify (const multiset & _multiset) const
    {
      return tree_it.verify ( (Tree_Type*)& (_multiset.tree));
    }

    bool verify (const iterator & it) const
    {
      return tree_it.verify (it.tree_it);
    }
  }; /* class iterator; */

/** 
  @brief Constant iterator type. 
*/
  typedef typename multiset::iterator const_iterator;

/** 
  @brief Iterator type inverted. 
*/
  typedef typename multiset::iterator reverse_iterator;
  
/** 
  @brief Iterator type constant invested. 
*/
  typedef typename multiset::iterator const_reverse_iterator;

/** 
  @brief Empty constructor of a multi-set. 
*/
  multiset () : num_elem (0), pool(100) { /* Empty */  }

private:

  void copy (const multiset & c)
  {
    tree.getRoot () = copyRec(c.tree.getRoot());
  }

public:
 
/** 
  @brief Request a copy of the multi-set c. 
*/
  multiset (const multiset & c) : num_elem (c.num_elem), pool(100)
  {
    copy (c);
  }

/** 
  @brief Instance multi-set with the items within the range [beg..end).

  @details This constructor instantiates a multi-set with initial elements
  copied from the associated container beg and end iterators.

  @param [in] beg Iterator positioned at the first item
  container copied.
  qparam [in] end Iterator positioned at the last plus
  copying the container element.

  @throw domain_error If beg and end iterators are not
  associated with the same container.

  @throw bad_alloc If there is insufficient memory.

  @note The iterator positioned by the end element is not included.
*/
      template <typename Itor> 
  multiset (Itor beg, const Itor & end) : multiset()
  {
    while (beg != end)
      insert (*beg++);
  }

/** 
  @brief Destructor; todos los elementos son eliminados y 
  la memoria es liberada. 
*/
  ~multiset () 
  { 
    destroyRec(tree.getRoot());
  }  

/** 
  @brief Assigns the entire contents of c to This. 
*/
  multiset & operator = (const multiset & c)
  {
    if (this == &c)
      return *this;

    destroyRec(tree.getRoot());

    copy(c);

    num_elem = c.num_elem;

    return *this;
  }

/** 
  @brief Returns the number of items contained in the multi-set. 
*/
  size_type size () const { return num_elem; }

/** 
  @brief Returns true if the container is empty. 
*/
  bool empty () const
  {
    return COUNT(tree.getRoot ()) == 0;
  }

/**
  @brief Returns true if the multi-set esta has exactly the same
  multi-group elements c. 
*/
  bool operator == (const multiset & c) const
  {
    if (this == &c)
      return true;

    if (size () != c.size ())
      return false;

    Tree_Iterator itor1 (tree), itor2 (c.tree);

    while (itor1.has_curr() and itor2.has_curr())
      {
	Node * p1 = itor1.get_curr();
	Node * p2 = itor2.get_curr();

	if (no_equals <Node_Data, Cmp_Data> (KEY(p1), KEY(p2)))
	  return false;
	else if (get_num_reps(p1) != get_num_reps(p2))
	  return false;

	itor1.next ();
	itor2.next ();
      }
    
    return not itor1.has_curr() and not itor2.has_curr();
  }

/** 
  @brief Returns true if the multi-set contains exactly esta
  the same elements of the multi-set c. 
*/
  bool operator != (const multiset & c) const
  {
    return not (*this == c);
  }

/** 
  @brief Returns true if the ordered items are multi-joint esta
  lower than in the multi-set c. 
*/
  bool operator < (const multiset & c) const
  {
    if (this == &c)
      return false;

    iterator itor1 (*this), itor2 (c);

    while (itor1.has_curr() and itor2.has_curr())
      {
	if (Cmp_Data () (itor1.get_data(), itor2.get_data()))
	  return true;
	else if (Cmp_Data () (itor2.get_data(), itor1.get_data()))
	  return false; /* They are not equal */
	  
	itor1.forward();
	itor2.forward();
      }
    
    if (itor1.has_curr()) /* |*this| >= |c| */
      return false;

    return itor2.has_curr();
  }

/** 
  @brief Returns true if the ordered items are multi-joint esta
  greater than the set multi-c. 
*/
  bool operator > (const multiset & c) const
  {
    return not (*this == c or *this < c);
  }

/** 
  @brief Returns true if the ordered items are multi-joint esta
  less than or equal to the multi-set c. 
*/
  bool operator <= (const multiset & c) const
  {
    return not (*this > c );
  }

/** 
  @brief Returns true if the ordered items are multi-joint esta
  greater or equal than the multi-set c. 
*/
  bool operator >= (const multiset & c) const
  {
    return not (*this < c);
  }

/** 
  @brief Returns the number of instances of value is inside the multi-set.

  @param [in] key Value key value to search for.

  @return number of instances of value that key value
  It contains the multi-set.
*/  
  size_type count (const T & value) const
  {
    Node * p = tree.search (Node_Data(value));

    if (p == NULL)
      return 0;

    return get_num_reps(p);
  } 

/** 
  @brief Returns an iterator positioned on the first instance of
  value element in the multi-set.

  @details find (value) seeks the value element in the multi-set. Yes
  it is found, then it returns an iterator
  positioned over the first item in conjunction with the multi-
  value in question; otherwise it returns an iterator to
  element that would precede a value if it were
  inserted. Note that in this case the parent element is the
  Finally among repeated.

  @param [in] value Value to search within the multi-set.

  @return An iterator positioned on the first element with
  value value if the multi-set is in the
  or an iterator to the last element that would precede a value if
  it was inserted into the multi-set. 
*/
  iterator find (const T & value) const
  { 
    Node * node = tree.search(Node_Data(value));

    if (node == NULL)
      return end ();

    return iterator(const_cast<multiset*>(this), node);
  }

/** 
  @breif Returns an iterator positioned on the first instance
  value in multi-set.

  @details lower_bound (value) seeks the value element in the multi-set.
  If it is found, then it returns an iterator
  positioned over the first item in conjunction with the multi-
  value in question; otherwise it returns an iterator
  overflowed at the end of the set.

  @param [in] value Value to search within the multi-set.

  @return An iterator positioned on the first element with
  value value if the multi-set is in the
  or an iterator overwhelmed if value is not found
  in the multi-set.
*/
  iterator lower_bound (const T & value) const
  {
    if (size () == 0)
      throw std::underflow_error ("multiset is empty");

    Node * tree_node = tree.search(Node_Data(value));

    if (tree_node == NULL)
      return end ();

    return iterator (this, tree_node);
  }

/** 
  @brief Returns an iterator positioned on the ultimate
  value in multi-set.

  @details upper_bound (value) seeks the value element in the multi-set. Yes
  it is found, then it returns an iterator
  positioned on the last element of the multi-set with
  value in question; otherwise it returns an iterator
  overflowed at the end of the set.

  @param [in] value Value to search within the multi-set.

  @return An iterator positioned on the last element
  value value if the multi-set is in the
  or an iterator overwhelmed if value is not found
  in the multi-set
*/
  iterator upper_bound (const T & value) const
  {
    if (size () == 0)
      throw std::underflow_error("multiset is empty");

    Node * tree_node = tree.search(Node_Data(value));

    if (tree_node == NULL)
      return end ();

    iterator it (this, tree_node);
    it.list_it.reset_last ();

    return it;
  }

/** 
  @brief Interact constant time all esta elements with those of c. 
*/
  void swap (multiset & c)
  {
    std::swap (tree.getRoot (), c.tree.getRoot ());
    std::swap (num_elem, c.num_elem);
  }

/** 
  @brief Returns an iterator positioned at the first element of multi-set. 
*/
  iterator begin () const
  {
    return iterator (*this);
  }

/** 
  @brief Returns an iterator positioned at the last element of multi-set. 
*/
  iterator end () const
  {
    return iterator(*this).compute_end();
  }

/** 
  @brief Enter value in the multi-set.

  @details insert (value) inserted into the multi-set esta a copy of
  element iterator value and positioned on the newly inserted element.

  @param [in] value Value inserted into the multi-set.

  @return An iterator positioned on the newly inserted element

  @throw bad_alloc if there is insufficient memory.
*/
  iterator insert (const T & value) 
  {
    Node * p = pool.allocate(Node_Data(value));
    Node * ptr = tree.search_or_insert(p);

    if (ptr != p) /* Value already in the tree */
      pool.deallocate(p);

    ++num_elem;

    return iterator(this, ptr, get_num_reps(ptr)++);
  }
    
/** 
  @brief Enter value in the multi-set from the iterator pos.

  @details insert (pos, value) inserted into the multi-set esta, from
  element positioned by the iterator pos.

  According to the standard, pos should be considered an aid to
  eventual insertion save searches. This is correct
  Aleph case if the value of the current element is pos
  equal to value; in this case, insertion takes \f O (1) \f $ and
  value is exactly after the current element pos.

  If the value of the current element of pos is not equal value,
  then it is inevitable a search which takes \f O (\lg n) \f $ -
  of value in the multi-set. If value already,
  then value is inserted at the end of the repeated elements.
  that same value, then the insertion is made in

  @param [in] pos Iterator from which you want to insert.
  @param [in] value Value inserted into the multi-set.
  
  @return An iterator positioned on the newly inserted element.
  
  @throw bad_alloc If there is insufficient memory.
  @throw throw overflow_error If ITER
*/  
  iterator insert (iterator pos, const T & value)
  {
    Aleph::verify_container_and_iterator (this, pos);

    if (pos != pos.compute_end())
      {
	Node * p = pos.get_curr_node();

	if (are_equals <Node_Data, Cmp_Data> (KEY(p), Node_Data(value)))
	  {
	    get_num_reps(p)++;
	    pos.pos_in_node++;
	    
	    return pos;
	  }
      }

    return insert(value);
  }

/** 
  @brief Inserted into the multi-joint elements of a container
  within the range [beg..end).

  @details insert (beg, end) takes a range of items in a container
  (set, map, multiset, multimap, list or table) associated with
  beg and end iterators and insert all elements of the range
  in the set esta.

  @param [in] beg Iterator element positioned on the start of the range.
  @param [in] end Iterator positioned on the last plus a final element of the range.
  
  @throw domain_error If beg and end iterators are not associated with the same container.
  @throw bad_alloc If there is insufficient memory.

  @note The iterator positioned by the end element is not included.
*/
      template <typename Itor>
  void insert (Itor beg, const Itor & end)
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      insert(*beg++);
  }

/** 
  @brief Clears the multi-set all instances of value.

  @details erase (value) clears the multi-set all instances the
  value element.

  @param [in] value Value of the item to delete.

  @return The number of instances of value that were deleted.
*/
  size_type erase (const T & value)
  {
    Node * tree_node = tree.remove(Node_Data(value));

    if (tree_node == NULL)
      return 0;

    const size_t ret_val = get_num_reps(tree_node);
	
    pool.deallocate(tree_node);

    num_elem -= ret_val;

    return ret_val;
  }

/** 
  @brief Clears the multi-set the current value of the iterator pos.

  @details erase (pos) deletes the corresponding item set to
  current value of the iterator pos.

  @param [in] pos Iterator whose current value is deleted.

  @throw domain_error If the iterator is not associated with
  this set.
  @throw underflow_error If the iterator is overwhelmed by the left.
  @throw overflow_error If the iterator is overwhelmed by the right.
*/
  void erase (iterator pos)
  {
    Aleph::verify_container_and_iterator (*this, pos);

    Node * tree_node = pos.get_curr_node();

    size_t & num_reps = get_num_reps(tree_node);

    --num_reps;
    --num_elem;

    if (num_reps == 0)
      {
	tree.remove(Node_Data(KEY(tree_node)));
	pool.deallocate(tree_node);
      }
  }
  
private:

    /* Deletes the range sequentially */
  iterator delete_range (iterator beg, const iterator & end)
  {
    while (beg != end)
      beg.del();

    return beg;
  }

public:

/** 
  @brief Clears the multi-set range comprised [beg..end).

  @details erase (beg, end) efficiently removes all elements
  multi-delimited set the beg and end iterators.

  @param [in] beg Iterator element positioned on the start of the range.
  @param [in] end Iterator positioned on the last one more element of the range.

  @throw domain_error If beg and end iterators are not
  this associated.

  @note The iterator positioned by the end element is not included.
*/
  iterator erase (iterator beg, const iterator & end)
  {
    Aleph::verify_container_and_iterator (*this, beg);
    Aleph::verify_iterators (beg, end);

    return delete_range(beg, end);
  }

/** 
  @brief Deletes all elements of the multi-set. 
*/
  void clear ()
  {
    destroyRec (static_cast<Node *> (tree.getRoot ()));
    num_elem = 0;
  }
};


    template <typename T>
typename iterator_traits<typename multiset<T>::iterator>::difference_type 
distance (typename multiset<T>::iterator it1, 
	  typename multiset<T>::iterator it2)
{
  typename iterator_traits<typename multiset<T>::iterator>::difference_type
    counter = 0;

  
  while (it1 != it2)
    {
      ++counter;
      ++it1;
    }

  return counter;
}

} /* End namespace Aleph */

# endif // AH_MULTISET_H
