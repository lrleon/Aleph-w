
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
  @file Multimap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


    //    /******************************************************
    //    Aleph implementation of multimap C++ standard container
    //    ******************************************************/

# ifndef ALEPH_MULTIMAP_H
# define ALEPH_MULTIMAP_H

# include <ah_stdc++_utils.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>

namespace Aleph
{

/** 
  @brief Aleph implementation of the standard type multimap <Key, T> 
  based on binary search trees ranges.

  @details This is a partial implementation, although quite advanced,
  best performance possible, standard C ++ class multimap <Key, T>.

  The implementation is based on binary search trees with
  range. By default, treaps used with tango, but could
  use other types of binary search trees provided
  they respect the interface.

  One difference with this implementation is that the standard
  type of tree is a parameter. In binary trees Aleph
  Search rank (BinNodeXt) treaps (Treap_Rk) and trees
  randomized (Rand_tree) can be used to implement this
  class.

  Another difference with the standard, perhaps most importantly,
  Couple mapping to repeat the primary key are ordered
  each. This requires the presence of the operator <for the type
  T. Similarly, violates standard rules that
  Repeated primary key pairs are sorted in order of
  insertion.

  The implementation uses a binary search tree with ranges
  to index the primary key Key type. Each node of this
  tree contains other binary search tree with ranges
  It is containing the associated keys. Each node, for both types of
  trees, runs a repeat counter. For example,
  for the pairs (1,0), (1,0), (1,0), (1,0), (1,5), (1,5), (1,5) is used one
  Ktree single node with key 1 and two nodes Ttree with keys 0 and 1
  respectively. This style of implementation saves a lot of space
  if indeed, it would make sense to wait, the multimapeo contains
  many duplications.

  This style of implementation tends to be faster than other
  traditional schemes. Especially, the progress of
  iterators.

  Note also that in the case of not containing many duplications
  This implementation uses almost the same space as a
  more trees based on linked lists of repetitions (the
  used by most of the standard instrumentation).

  @param Key Type of primary key
  @param T Data type associated to a primary key.
  @param Compare Comparison criterion elements T. 
  By default Key type :: operator <(const T &) is implanted.
  @param KTree Class of binary search tree ranges Yea, 
  the primary key Key type are indexed.
  @param TTree Class of binary search tree with ranges 
  that are indexed keys associated.

  @ingroup stl

  @see BinNodeXt Treap_Rk Rand_Tree set map multiset 
*/
template <typename Key, 
	  typename T, 
	  class Compare = Aleph::less<Key>,
	  template <class, class> class KTree = Treap_Rk,
	  template <class, class> class TTree = Treap_Rk
    >
class multimap
{
/** 
  @struct Defining record containing elem type T
*/
  struct Tdata
  {
    /* Mapped to a key element of type key number of occurrences of
     elem related to a key value key */ 
    T elem;          
    size_t num_reps;

    Tdata() : num_reps(0) { /* Empty */ }

    Tdata(const T & e) : elem(e), num_reps(0) { /* Empty */ }

    Tdata(const Tdata & item) : elem(item.elem), num_reps(item.num_reps) 
    {
      /* Empty */
    }
  };

  typedef Aleph::less<T> Tless;

/** 
  @struct Class compared to key associated. 
*/
  struct Cmpt 
  {
    bool operator () (const Tdata & op1, const Tdata & op2) const
    {
      return Tless () (op1.elem, op2.elem);
    }
  };

/** 
  @typedef Definition of key tree mapped elements. 
*/
  typedef TTree<Tdata, Cmpt> T_Tree;

  typedef typename T_Tree::Node Tnode;

  typedef Node_Pool<Tnode> Tpool;

/** 
  @struct Defining record containing elements multimap. 
*/
  struct Kdata
  {
/** 
  @brief Number of repetitions primary key value key
  Tree T-type elements associated with key. 
*/
    Key    key;      
    size_t num_reps;
    T_Tree t_tree;   
    
    Kdata() : num_reps(0) { /* Empty */ }

    Kdata(const Kdata & item) : key(item.key), num_reps(0)
    {
      t_tree.getRoot() = copyRec(const_cast<Kdata&>(item).t_tree.getRoot());
      num_reps = item.num_reps;
    }

    ~Kdata()
    {
      destroyRec(t_tree.getRoot());
    }

    Kdata & operator = (const Kdata & item) 
    {
      if (this == &item)
      	return *this;

      destroyRec(t_tree.getRoot());
      num_reps = 0;
      t_tree.getRoot() = copyRec(const_cast<Kdata&>(item).t_tree.getRoot());

      key      = item.key;
      num_reps = item.num_reps;

      return *this;
    }
  };
/** 
  @struct Class compared to key. 
*/
  struct Cmpk
  {
    bool operator () (const Kdata & op1, const Kdata & op2) const
    {
      return Compare () (op1.key, op2.key);
    }
  };

  typedef KTree<Kdata, Cmpk> K_Tree;

  typedef typename K_Tree::Node Knode;

    /* members Data */
  Node_Pool<Knode> kpool;
  Tpool            tpool;

  K_Tree           k_tree;

  size_t           num_elem;

/**
  @note To save memory, the tree Ktree records indexed by type
  KDATA and following comparison function handles only
  compare the key. Now it is very important not to have a
  penalty in performance, no time is wasted in
  KDATA builder whenever required to perform a
  Search. So the next field is created once and used
  for searches.
*/ 
  Kdata                 searching_kdata; 

/** 
  @brief This function is used when you want to search key and
  obtain required KDATA 
*/
  Kdata & key_kdata(const Key & key) 
  {
    searching_kdata.key = key;
    return searching_kdata;
  }
  
public:

  multimap() : kpool(100), tpool(100), num_elem(0) { /* Empty */ }

  ~multimap()
  {
    destroyRec(k_tree.getRoot());
  }

/** 
  @brief Empty the multimap, all items are deleted. 
*/
  void clear()
  {
    destroyRec(k_tree.getRoot());
    num_elem = 0;
  }

  typedef std::pair<Key, T> Pair;

/** 
  @typedef Type key value managed
  (which is a couple) 
*/
  typedef Pair value_type;

/** 
  @typedef Reference to pair 
*/
  typedef typename multimap::value_type & reference;

  typedef const typename multimap::value_type & const_reference;

  typedef size_t size_type;

  typedef Key key_type;

  typedef T mapped_type;

/** 
  @brief Returns the number of elements contained in multimap
  (total number of pairs) 
*/
  const size_t & size () const { return num_elem; }

/** 
  @brief Returns an estimate of the maximum amount of elements
  You can hold the multimap. 
*/
  size_type max_size() const 
  { 
    int sizek = sizeof(Knode);
    int sizet = sizeof(Tnode);

    double total_mem = pow(2, __WORDSIZE);

    size_type ret_val = floor(total_mem / (sizek + sizet));

    return ret_val;
  }

/** 
  @brief Returns true if the multimap is empty
  (contains no elements) 
*/
  bool empty () const {return k_tree.is_empty(); }

private:

  typedef typename K_Tree::Iterator K_Itor;
  typedef typename T_Tree::Iterator T_Itor;
public:

/** 
  @brief Iterator on a multimap

  @ingroup stl
*/
  class iterator
  {
    friend class multimap<Key, T, Compare, KTree, TTree>;

    mutable multimap * multimap_ptr;
    mutable K_Tree *   k_tree_ptr;
    mutable K_Itor     k_it;
    mutable T_Tree *   t_tree_ptr;
    mutable T_Itor     t_it;
    mutable int        pos_in_k;
    mutable int        pos_in_t;
    bool               underflow;
    bool               overflow;
    Pair               ret_pair; /* Used by the operator -> */

    void default_init()
    {
      assert(k_tree_ptr != NULL);

      if (k_it.has_curr())
	{
	  assert(KEY(k_it.get_curr()).t_tree.size() > 0);
	  underflow = overflow = false;
	  pos_in_k = pos_in_t = 0;
	  t_tree_ptr = &KEY(k_it.get_curr()).t_tree;
	  t_it = T_Itor(*t_tree_ptr);
	}
      else
	put_in_overflow();
    }

    iterator(multimap * m, Knode * kp, Tnode * tp, int kpos = 0, int tpos = 0)
      : multimap_ptr(m), k_tree_ptr(&m->k_tree), k_it(*k_tree_ptr, kp),
	t_tree_ptr(&KEY(kp).t_tree), t_it(*t_tree_ptr, tp), 
	pos_in_k(kpos), pos_in_t(tpos), underflow(false), overflow(false)
    {
      assert(not KEY(kp).t_tree.is_empty());
      assert(KEY(tp).num_reps > 0 and tpos < KEY(tp).num_reps);
    }

    iterator(multimap * m, Knode * p)
      : multimap_ptr(m), k_tree_ptr(&m->k_tree), k_it(*k_tree_ptr, p),
	t_tree_ptr(&KEY(p).t_tree), t_it(*t_tree_ptr),
	pos_in_t(0), underflow(false), overflow(false)
    {
      assert(KEY(p).num_reps > 0);
      assert(not KEY(p).t_tree.is_empty());
    }

  public:

/** 
  @typedef The data type that contains the multi-mapping. 
*/
    typedef typename multimap::value_type   value_type;

/** 
  @typedef Numeric type used for differences between positions iterators. 
*/
    typedef typename multimap::size_type    difference_type;

/** 
  @typedef Type pointer to element within the multi-mapping. 
*/
    typedef typename multimap::value_type * pointer;

/** 
  @typedef Type reference element in the multi-mapping. 
*/
    typedef typename multimap::reference    reference;

/** 
  @brief Builder from a multimap
  (not delestándar part, but it's very useful). 
*/
    iterator(const multimap & mm) 
      : multimap_ptr(const_cast<multimap*>(&mm)), 
	k_tree_ptr(&multimap_ptr->k_tree), k_it(*k_tree_ptr)
    {
      default_init();
    }

/** 
  @brief copy constructor
  (massively used by the standard) 
*/
    iterator(const iterator & it) 
      : multimap_ptr(it.multimap_ptr), k_tree_ptr(it.k_tree_ptr), k_it(it.k_it),
	t_tree_ptr(it.t_tree_ptr), t_it(it.t_it), 
	pos_in_k(it.pos_in_k), pos_in_t(it.pos_in_t), 
	underflow(it.underflow), overflow(it.overflow)
    {
      /* Empty */
    }

/** 
  @brief Empty constructor; Unlimited use. 
*/
    iterator()
      : multimap_ptr(NULL), k_tree_ptr(NULL), t_tree_ptr(NULL),
	pos_in_k(-1), pos_in_t(-1), underflow(true), overflow(true)
    {
      /* Empty */
    }

/** 
  @brief allocation
*/
    iterator & operator = (const iterator & it) 
    {
      if (this == &it)
	return *this;

      multimap_ptr = it.multimap_ptr;
      k_tree_ptr   = it.k_tree_ptr;
      k_it         = it.k_it;
      t_tree_ptr   = it.t_tree_ptr;
      t_it         = it.t_it; 
      pos_in_k     = it.pos_in_k;
      pos_in_t     = it.pos_in_t;
      underflow    = it.underflow;
      overflow     = it.overflow;

      return *this;
    }

  private:

    bool has_curr() const { return k_it.has_curr(); }

    Knode * get_curr() { return k_it.get_curr(); }

    Kdata & get_curr_kdata() { return KEY(get_curr()); }

  public:

/** 
  @brief Returns a copy of the current iterator pair. 
*/
    value_type  operator * () 
    {
      return value_type(get_curr_kdata().key, KEY(t_it.get_curr()).elem);
    }

/** 
  @brief Returns a pointer to dereference back
  (access to one of the fields of the pair -first or second)

  @note The return address is that of an inner pair whose
  existence is contingent on the iterator. <b> Do not use for
  get pointer to pair </ b>, then, apart from that it is not
  purpose of this operator, the address is invalid once
  destroys the iterator. <b> Use strictly for access
  torque fields (first or second) of the current element which is
  positioned iterator </ b>.
*/
    const value_type * operator -> () 
    {
      ret_pair.first  = get_curr_kdata().key;
      ret_pair.second = KEY(t_it.get_curr()).elem;

      return &ret_pair;
    }

  private:

    void goto_begin()
    {
      k_it.reset_first();
      if (not has_curr())
	{
	  put_in_underflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      pos_in_k = pos_in_t = 0;
    }

    void goto_last()
    {
      k_it.reset_last();
      if (not has_curr())
	{
	  put_in_overflow();
	  return;
	}

      overflow = false;
      Kdata & kdata = KEY(get_curr());
      t_tree_ptr = &kdata.t_tree;
      t_it = T_Itor(*t_tree_ptr);
      t_it.reset_last();
      pos_in_k = kdata.num_reps - 1;
      pos_in_t = KEY(t_it.get_curr()).num_reps - 1;
    }

    void goto_end()
    {
      k_it.reset_last();
      if (has_curr())
	{
	  k_it.next(); /* Puts it out of range */
	  underflow = false;
	}
      else
	put_in_underflow();

      put_in_overflow();
    }

    iterator compute_end() const
    {
      iterator it(*this);
      it.goto_end();
      assert(it.overflow);
      return it;
    }

    bool is_at_end() const { return not has_curr(); }

    iterator compute_begin() const
    {
      iterator it(*this);
      return it;
    }

    void put_in_overflow()
    {
      t_tree_ptr = NULL;

      if (k_tree_ptr->is_empty())
	put_in_underflow();

      overflow = true;
    }
    
    void put_in_underflow()
    {
      t_tree_ptr = NULL;
      
      pos_in_t = -1;
      underflow = true;
    }

/** 
  @brief All key advances the current position. 
*/
    void forward_k_it()
    {
      k_it.next();
      if (not has_curr())
	{
	  put_in_overflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      pos_in_t = 0;
    }

    void forward_tree_iterators()
    {
      t_it.next();
      if (t_it.has_curr())
	{
	  pos_in_t = 0;
	  return;
	}
	  
      forward_k_it();
    }

    void forward()
    {
      if (underflow)
	{
	  goto_begin();
	  return;
	}

      if (overflow)
	{
	  assert(t_tree_ptr == NULL);
	  throw std::overflow_error("Multimap::iterator is already "
				    "in overflow");
	}

      assert(t_it.has_curr() and t_tree_ptr != NULL);

      Tdata & tdata = KEY(t_it.get_curr());
      if (++pos_in_t < tdata.num_reps) /* Reaches the last item */
	return; /* Not yet! */

      forward_tree_iterators();
    }

    void backward_k_it()
    {
      k_it.prev();
      if (not has_curr())
	{
	  put_in_underflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      t_it.reset_last();
      pos_in_t = KEY(t_it.get_curr()).num_reps - 1;
    }

    void backward_tree_iterators()
    {
      t_it.prev();
      if (t_it.has_curr())
	{
	  pos_in_t = KEY(t_it.get_curr()).num_reps - 1;
	  return;
	}
	  
      backward_k_it();
    }

    void backward()
    {
      if (overflow)
	{
	  goto_last();
	  return;
	}

      if (underflow)
	{
	  assert(t_tree_ptr == NULL);
	  throw std::underflow_error("Multimap::iterator is "
				     "already in underflow");
	}

      assert(t_it.has_curr() and t_tree_ptr != NULL);

      if (--pos_in_t >= 0) /* Reaches the first element */
	return;

      backward_tree_iterators();
    }

    void del()
    {
      Kdata & kdata = get_curr_kdata();
      Tnode * tp = t_it.get_curr();
      Tdata & tdata = KEY(tp);

      --multimap_ptr->num_elem;
      --kdata.num_reps;
      if (--tdata.num_reps == 0)
	{
	  multimap_ptr->tpool.deallocate(t_it.del()); /* Remove and release tp */
	  pos_in_t = 0;
	}
      else if (pos_in_t == tdata.num_reps)
	{
	  t_it.next();
	  pos_in_t = 0;
	}

      if (t_it.has_curr())
	{
	  assert(kdata.num_reps > 0);
	  return;
	}

      if (kdata.num_reps == 0)
	{
	  Knode * kp = k_it.del();
	  assert(KEY(kp).t_tree.is_empty());
	  multimap_ptr->kpool.deallocate(kp);
	}
      else
	k_it.next();

      if (not k_it.has_curr())
	{
	  put_in_overflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      pos_in_k = pos_in_t = 0;
    }

  public:

/** 
  @brief Returns true if the iterators have the same status. 
*/
    bool operator == (const iterator & it) const
    {
      if (this->has_curr() and it.has_curr())
	return (t_it.get_curr() == it.t_it.get_curr() and 
		pos_in_t == it.pos_in_t);

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
 
/** 
  @brief Advances the iterator a forward position,
  then load current item. 
*/   
    iterator operator ++ ()
    {
      forward();
      return *this;
    }

/** 
  @brief Advances the iterator a forward position,
  then load current item. 
*/  
    iterator operator ++ (int)
    {
      iterator ret_val = *this;
      forward();
      return ret_val;
    }

/** 
  @brief Advances the iterator a rearward position,
  then load current item. 
*/
    iterator operator -- ()
    {
      backward();
      return *this;
    }

/** 
  @brief Avanza el iterador una posición hacia atrás, 
  luego carga elemento actual. 
*/
    iterator operator -- (int)
    {
      iterator ret_val = *this;
      backward();
      return ret_val;
    }

/** 
  @brief Advances the iterator n elements forward and
  load current new element. < 
*/
    iterator operator += (size_type n)
    {
      if (n == 0)
	return *this;

/** 
  @brief Forward by nodes t_it 
*/
      while (true) 
	{
	  assert(KEY(t_it.get_curr()).num_reps > 0);

	  const size_t treps = KEY(t_it.get_curr()).num_reps;
	  const int remain_in_t_node = treps - pos_in_t;
	  if (n < remain_in_t_node)
	    {
	      pos_in_k += n;
	      pos_in_t += n;
	      assert(pos_in_t < KEY(t_it.get_curr()).num_reps);

	      return *this;
	    }

	  n -= remain_in_t_node;
	  pos_in_k += treps;
	  t_it.next();
	  pos_in_t = 0;
	  if (t_it.has_curr())
	    continue;

	  assert(pos_in_k == KEY(k_it.get_curr()).num_reps);

/** 
  @brief Forward by nodes k_it 
*/
	  while (true) 
	    {
	      k_it.next();
	      if (not has_curr())
		{
		  put_in_overflow();
		  throw std::overflow_error("Multimap::iterator is "
					    "already in overflow");
		}
	  	  
	      assert(KEY(get_curr()).num_reps > 0);

	      const int remain_in_k_node = KEY(get_curr()).num_reps;
	      if (n < remain_in_k_node)
		{
		  t_tree_ptr = &KEY(get_curr()).t_tree;
		  t_it = T_Itor(*t_tree_ptr);
		  pos_in_k = pos_in_t = 0;
		  break;
		}

	      n -= remain_in_k_node;
	    }
	}
    } 
  };

/** 
  @typedef constant iterator. 
*/
  typedef const iterator const_iterator;

/** 
  @brief Multimap inserted into the elements in the range
  [first, last).

  @details This insertion takes \f $ \lg n \f $; where n is the number of keys
  Key different type containing the multimap.
*/
  iterator insert(const value_type & value)
  {
    Knode * kp = kpool.allocate(key_kdata(value.first)); /* New node */
    Knode * kq = k_tree.search_or_insert(kp); 

    /** It is now the primary key in the multiset?
    YES? ==> keep the node for future use */
    if (kp != kq)
      kpool.deallocate(kp);

    assert(KEY(kq).key == value.first); /* TMP */

    T_Tree & t_tree = KEY(kq).t_tree;
    const Tdata tdata(value.second);
    
    try
      {
	Tnode * tp = tpool.allocate(tdata);
	Tnode * tq = t_tree.search_or_insert(tp);

    /* Is the item already associated with the key? */
	if (tp != tq)  
	  tpool.deallocate(tp);

	++num_elem;
	++KEY(kq).num_reps;

	return iterator(this, kq, tq, KEY(tq).num_reps++);
      }
    catch (...)
      {
	if (kp == kq) 
	  kpool.deallocate(kp);
	
	throw;
      }
  }

/** 
  @brief Multimap inserted into the elements in the range [first, last). 
*/
      template <class InputIterator>
  void insert(InputIterator first, const InputIterator & last)
  {
    while (first != last)
      insert(*first++);
  }

/** 
  @brief Build a couple multimap with range [first, last). 
*/
       template <class InputIterator>
 multimap(InputIterator first, const InputIterator & last) 
   : multimap()
  {
    this->insert(first, last);
  }

/** 
  @brief copy constructor. 
*/
  multimap(const multimap & m) 
    : multimap::multimap()
  {
    k_tree.getRoot() = copyRec(const_cast<multimap&>(m).k_tree.getRoot());
    num_elem = m.num_elem;
  }

/** 
  @brief Allocation. 

  @details All elements of the left side are released. 
*/
  multimap & operator = (const multimap & m)
  {
    if (this == &m)
      return *this;

    destroyRec(k_tree.getRoot());
    num_elem = 0;

    k_tree.getRoot() = copyRec(const_cast<multimap&>(m).k_tree.getRoot());
    num_elem = m.num_elem;

    return *this;
  }

/** 
  @brief Inserting "helped" position.

  @details This version of insert () takes the hint iterator position
  to effect more rapid than traditional insertion.

  If hint-> first == value.first the discretion of comparison,
  then this insertion consume \f $ \lg m \f $; where m is the
  number of repetitions different secondary key associated
  to hint-> first or value.first. Otherwise it performs
  Traditional integration.

  @param [in] hint iterator "help"
  @param [in] Par value inserted

  @return An iterator pointing to a copy of the newly couple
  inserted.

  @throw bad_alloc If there is insufficient memory.
*/
  iterator insert(const_iterator & hint, const value_type & value)
  {
    if (hint.has_curr()) /* look what is in the iterator */
      {
	Knode * kp = const_cast<iterator&>(hint).get_curr();
	Kdata & kdata = KEY(kp);

    /** Primary key hint value.first match?
    YES? ==> no need to search tree k_tree */
	if (Aleph::are_equals <Key, Compare> (kdata.key, value.first))
	  {

	    Tnode * tq = hint.t_it.get_curr(); /* Look key associated */
	    const Tdata & tdata = KEY(tq);

    /** The key partner is different?
    YES? ==> look or insert into hive key associated*/
	    if (Aleph::no_equals <T, Tless> (tdata.elem, value.second)) 
	      {
		Tnode * tp = tpool.allocate(tdata);
		tq = kdata.t_tree.search_or_insert(tp);

    /** Is the item already associated with the key?
    YES? ==> keep it for future use */
		if (tp != tq)  
		  tpool.deallocate(tp); 
	      }
	    
	    ++num_elem;
	    ++kdata.num_reps;

	    return iterator(this, kp, tq, KEY(tq).num_reps++);
	  }
      }
	
    /* Hint It has nothing to do with value ==> Normal insertion */
    return insert(value);
  }

/** 
  @brief Multimap Removes the element at position. 
*/
  iterator erase(const_iterator & position)
  {
    iterator ret_val = position;
    
    ret_val.del();

    return ret_val;
  }

/** 
  @brief Removes all key pairs with key and returns the number of deleted pairs. 
*/
  size_type erase(const key_type & key)
  {
    Knode * p = k_tree.remove(key_kdata(key));
    if (p == NULL)
      return 0;

    size_type ret_val = KEY(p).num_reps;

    num_elem -= ret_val;

    delete p; /* We do kpool.deallocate (p) because we want to liberate
              tree entirely associated keys KEY (p).t_tree */

    return ret_val;
  }

/** 
  @brief Eliminate all pairs within the range [first, last).

  @param [in] first Position the first element to be removed.
  @param [in] last Position of the last element successor
  eliminating element.

  @return An iterator to the last element removed successor
  (must match last).

  @All Jump key leverage given by inter counters
  repetitions.
*/
  iterator erase(iterator first, const_iterator & last)
  {
    iterator it = first; 
    while (it != last)
      it = erase(it);

    return it;
  }

/** 
  @brief Returns an iterator to the first element of multimap 
*/
  iterator begin() const { return iterator(*this); }
  
/** 
  @brief Returns an iterator to the last element of multimap. 
*/
  iterator end () const { return iterator(*this).compute_end(); }

/** 
  @brief Returns the number pairs with key equal to key 
*/
  size_type count(const Key & key) const
  {
    Kdata & kdata = const_cast<multimap*>(this)->key_kdata(key);
    Knode * p = const_cast<multimap*>(this)->k_tree.search(kdata);
    if (p == NULL)
      return 0;

    return KEY(p).num_reps;
  }

/** 
  @brief Returns an iterator positioned on the first pair with
  key key; end () otherwise. 
*/
  iterator find(const Key & key)
  {
    Knode * p = k_tree.search(key_kdata(key));
    if (p == NULL)
      return end();

    return iterator(this, p);
  }

/** 
  @overload find() 
*/
  const_iterator find(const Key & key) const
  {
    return const_cast<multimap*>(this)->find(key);
  }

/** 
  @brief Returns an iterator positioned at the location where key would be inserted. 
*/
  iterator lower_bound(const Key & key)
  {
    if (k_tree.is_empty())
      return end();

    std::pair<int, Knode*> p = k_tree.find_position(key_kdata(key));
    Knode * kp = p.second;

    iterator ret = iterator(this, kp);
    
    if (Aleph::are_equals <Key, Compare> (KEY(kp).key, key))
      return ret;

    if (p.first == k_tree.size()) /* Does greater key that contained all? */
      return end();

    if (p.first == -1)
      return begin();

    if (Compare () (ret->first, key))
      ret.forward_k_it();

    return ret;
  }

/** 
  @overload lower_bound() 
*/
  const_iterator lower_bound(const Key & key) const
  {
    return const_cast<multimap*>(this)->lower_bound(key);
  }

/** 
  @brief Returns an iterator positioned at the location successor
  key if key was inserted. 
*/
  iterator upper_bound(const Key & key)
  {
    if (k_tree.is_empty())
      return end();

    std::pair<int, Knode*> p = k_tree.find_position(key_kdata(key));
    Knode * kp = p.second;

    iterator ret = iterator(this, kp);
    
    if (Aleph::are_equals <Key, Compare> (KEY(kp).key, key))
	ret.forward_k_it();

    /* ¿Key contained greater than all? */
    if (p.first == k_tree.size()) 
      return end();

    if (p.first == -1)
      return begin();

    if (Compare () (ret->first, key)) // key > *ret
      ret.forward_k_it();

    return ret;
  }

/** 
  @overload upper_bound() 
*/
  const_iterator upper_bound(const Key & key) const
  {
    return const_cast<multimap*>(this)->upper_bound(key);
  }

/** 
  @brief Returns a pair of iterators that define the full range of
  keys with key value that are contained in the multimap. 
*/
  std::pair<iterator,iterator> equal_range(const Key & key)
  {
    Knode * p = k_tree.search(key_kdata(key));
    if (p == NULL)
      {
	iterator e = end();
	return std::pair<iterator,iterator>(e, e);
      }

    iterator first(this, p);
    iterator last(this, p);
    last += KEY(p).num_reps;
    
    return std::pair<iterator,iterator>(first, last);
  }

/** 
  @overload equal_range() 
*/
  std::pair<const iterator,const iterator> equal_range(const Key & key) const 
  {
    return const_cast<multimap*>(this)->equal_range(key);
  }

/** 
  @brief This exchanges values with those of other. 
*/
  void swap(multimap & other)
  {
    k_tree.swap(other.k_tree);
    std::swap(num_elem, other.num_elem);
  }  

/** 
  @brief Returns true if this rate is less than rhs according to the 
  lexicographical comparison.

  @see lexicographical_compare
*/
  bool operator < (const multimap & rhs) const
  {
    K_Itor kit1(const_cast<multimap*>(this)->k_tree);
    K_Itor kit2(const_cast<multimap&>(rhs).k_tree);
    for (; kit1.has_curr() and kit2.has_curr(); kit1.next(), kit2.next())
      {
	Kdata & kdata1 = KEY(kit1.get_curr());
	Kdata & kdata2 = KEY(kit2.get_curr());

	const size_t & n1 = kdata1.num_reps;
	const size_t & n2 = kdata2.num_reps;

	if (n1 != n2)
	  return n1 > n2;

	const Key & key1 = kdata1.key;
	const Key & key2 = kdata2.key;
	if (Compare () (key1, key2))
	  return true;
	else if (Compare () (key2, key1))
	  return false;
      }
    
    if (kit1.has_curr() or kit2.has_curr())
      return kit2.has_curr();
    
    return false;
  }

/** 
  @brief Returns true if this rate is lower than or equal to rhs according to the lexicographical comparison.
  @see lexicographical_compare
*/
  bool operator <= (const multimap & rhs) const
  {
    K_Itor kit1(const_cast<multimap*>(this)->k_tree);
    K_Itor kit2(const_cast<multimap&>(rhs).k_tree);
    for (; kit1.has_curr() and kit2.has_curr(); kit1.next(), kit2.next())
      {
	Kdata & kdata1 = KEY(kit1.get_curr());
	Kdata & kdata2 = KEY(kit2.get_curr());

	const size_t & n1 = kdata1.num_reps;
	const size_t & n2 = kdata2.num_reps;

	if (n1 != n2)
	  return n1 > n2;

	const Key & key1 = kdata1.key;
	const Key & key2 = kdata2.key;
	if (Compare () (key1, key2))
	  return true;
	else if (Compare () (key2, key1))
	  return false;
      }
    
    if (kit1.has_curr() or kit2.has_curr())
      return kit2.has_curr(); 
    
    return true;
  }
  
/** 
  @brief Returns true if this rate is exactly rhs. 
*/
  bool operator == (const multimap & rhs) const
  {
    if (size() != rhs.size())
      return false;

    K_Itor kit1(const_cast<multimap*>(this)->k_tree);
    K_Itor kit2(const_cast<multimap&>(rhs).k_tree);
    for (; kit1.has_curr() and kit2.has_curr(); kit1.next(), kit2.next())
      {
	Kdata & kdata1 = KEY(kit1.get_curr());
	Kdata & kdata2 = KEY(kit2.get_curr());

	if (kdata1.num_reps != kdata2.num_reps)
	  return false;

	const Key & key1     = kdata1.key;
	const Key & key2     = kdata2.key;
	if (Compare () (key1, key2))
	  return false;
	else if (Compare () (key2, key1))
	  return false;
      }
    
    if (kit1.has_curr() or kit2.has_curr())
      return false;

    return true;
  }

/** 
  @brief Returns true if this rate is different from rhs. 
*/
  bool operator != (const multimap & rhs) const
  {
    return not (*this == rhs);
  }

/** 
  @brief Returns true if this rate is less than rhs according to the lexicographical comparison.

  @see lexicographical_compare
*/
  bool operator > (const multimap & rhs) const
  {
    return rhs < *this;
  }

/** 
  @brief Returns true if this rate is greater than or equal to rhs according to the lexicographical comparison.

  @see lexicographical_compare
*/
  bool operator >=  (const multimap & rhs) const
  {
    return rhs <= *this;
  }

/** 
  @brief Access and settlement.

  @details The operator [] can be used to write and read data to the
  multimapeo.

  If m is a mapping, then m [key] = item; He inserted the pair
  (key, item).

  The expression cout << m [key] "reads" the item associated with the key
  key. If the key already exists, then the first item is returned
  associated. Otherwise, a new pair is inserted whose value
  item is initialized by the constructor T :: T ().
*/
  const T & operator [] (const Key & key) 
  {
    iterator ret = insert(make_pair(key, T()));

    return ret->second;
  }

/** 
  @brief Access as a constant basis,
  if the key is not inserted, then the exception is triggered. 
*/
  const T & operator [] (const Key & key) const 
    Exception_Prototypes (std::domain_error)
  {
    iterator ret = find(key);
    if (ret == end())
      throw std::domain_error("key not found on constant multimap");

    return ret->second;
  }
};





} /* End namespace Aleph */


# endif // ALEPH_MULTIMAP_H
