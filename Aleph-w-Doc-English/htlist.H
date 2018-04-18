
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
  @file htlist.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef HTLIST_H
# define HTLIST_H

# include <cassert>
# include <stdexcept>
# include <utility>
# include <initializer_list>
# include <ahFunction.H>
# include <ahFunctional.H>
# include <ahDry.H>

# define NEXT(p) (p->get_next())

namespace Aleph {

/** 
  @brief Link belonging to a linked list simplemenyte

  @ingroup sequences
*/
class Slinknc
{
  Slinknc * next;

public:

/** 
  @brief empty constructor. 
*/
  Slinknc() : next(NULL) { /* Empty */ }

/** 
  @brief Copy constructor; placed link to point to NULL. 
*/
  Slinknc(const Slinknc &) : next(NULL) { /* Empty */ }

/** 
  @brief Restart link to point to NULL 
*/
  void reset()
  {
    assert(this not_eq NULL);
    next = NULL;
  }
  
/** 
  @return Returns true if this rate is empty (points to NULL) 
*/
  bool is_empty() const
  {
    assert(this != NULL);
    return next == NULL;
  }

/** 
  @brief assignment; placed link to point to NULL. 
*/
  Slinknc & operator = (const Slinknc & link)
  {
    if (&link == this)
      return *this;
    
    if (not is_empty())
      throw std::invalid_argument("link is not empty");

    next = NULL; 

    return *this;
  }

/** 
  @return Returns the following link. 
*/
  Slinknc *& get_next()
  {
    assert(this not_eq NULL);
    return next;
  }

/** 
  @brief Enter the link after esta p.

  @details insert (p) Inserts a single node whose link is p after © s
  the node whose link is esta.

  @param [in] p simple Link to insert.
*/
  void insert(Slinknc * p)
  {
    assert(this not_eq NULL);
    assert(p not_eq NULL);
    assert(p->is_empty());
    p->next = next;
    next = p;
  }

/** 
  @brief Eliminates esta following link.

  @details remove_next () Removes the single node located after © s
  the node whose link is esta and returns the address of the link
  removed.

  @return The link removed.
*/
  Slinknc * remove_next()
  {
    assert(this not_eq NULL);
    Slinknc * ret_val = next;
    next = ret_val->next;
    ret_val->reset();
    return ret_val;
  }

  class Iterator
  {
    Slinknc * head;
    Slinknc * curr;

  public:

    Iterator()
      : head(NULL), curr(NULL)
    {
      /* Empty */
    }

    Iterator(Slinknc & list)
      : head(&list), curr(head)
    {
      /* Empty */
    }

    Iterator(Slinknc * _head, Slinknc * _curr)
      : head(_head), curr(_curr)
    {
      /* Empty */
    }

    bool has_current() const
    {
      return curr != NULL;
    }

    bool has_curr() const
    {
      return has_current();
    }

    Slinknc * get_current() const
    {
      if (not has_current())
        throw std::overflow_error("Iterator is at the end of the list");
      return curr;
    }

    Slinknc * get_curr() const
    {
      return get_current();
    }

    void next()
    {
      if (not has_current())
        throw std::overflow_error("Iterator is at the end of the list");
      curr = curr->next;
    }

    void reset_first()
    {
      curr = head;
    }
  };	
};


    template <typename T>
class Snodenc : public Slinknc
{
  T data;

public:

/** 
  @return Returns a reference to the data contained in the node. 
*/
  T & get_data() { return data; }

/** 
  @brief empty constructor. 
*/
  Snodenc() { /* empty*/ }

/** 
  @brief Copy constructor data. 
*/
  Snodenc(const T & item) : data(item) 
  {
    /* Empty */
  }

  Snodenc(T && item)
  {
    std::swap(data, item);
  }

/** 
  @brief eliminates following node this.

  @details remove_next () Removes the bound after esta node and returns
  your memory.
          
  @return node removed.
*/
  Snodenc * remove_next() { return (Snodenc*) Slinknc::remove_next(); }

/** 
  @return Returns the next node this.
*/
  Snodenc *& get_next() { return (Snodenc*&) Slinknc::get_next(); }

/** 
  @brief Removes the following node this.

  @details remove_next () removes the bound after esta node and returns
  your memory.
          
  @return Node removed.
*/
  Snodenc * remove_first() { return Snodenc::remove_next(); }

/** 
  @return Returns the next node to this 
*/
  Snodenc *& get_first() const { return Snodenc::get_next(); }
Returns the next node

/** 
  @brief singly-linked list.

  @details HTList Slinknc Renders a list of objects. Is a Slinknc
  single bond that may be contained within any
  structure.

  A HTlist maintains two references. A so-called "head" (head)
  Slinknc corresponding to the first sequence; and another call
  "tail" or tail, corresponding to the last element
  sequence.

  Remember that this class handles pointers to Slinknc, not data
  type.

  @See Dynlist for any sequence types
  instrumented with HTLink.

  For further explanation see documentation
  Slinknc and Snodenc.

  @see Slinknc Snodenc DynList

  @ingroup Sequences
*/
class HTList
{
  Slinknc * head;
  Slinknc * tail;

public:

/** 
  @brief Initializes an empty list. 
*/
  HTList() : head(NULL), tail(NULL) { /* Empty */ }

/** 
  @brief Initializes a list of one Slinknc 
*/
  HTList(Slinknc * l) : head(l), tail(l) { /* Empty */ }

/** 
  @return Returns true if the list is empty. 
*/ 
  bool is_empty() const { return head == NULL; }

/** 
  @return Returns true if the list contains exactly one element. 
*/
  bool is_unitarian() const { return head != NULL and head == tail; }

/** 
  @return Returns true if the list is an item or empty. 
*/
  bool is_unitarian_or_empty() const { return head == tail; }

/**
  @return Returns the top of the list
  (the first element) 
*/
  Slinknc * get_head() const { return head; }

/** 
  @return Returns the bottom of the list
  (the last item) 
*/
  Slinknc * get_tail() const { return tail; }

/** 
  @return Returns the first item in the list. 
*/
  Slinknc * get_first() const { return get_head(); }

/** 
  @return Returns the last item in the list. 
*/
  Slinknc * get_last() const { return get_tail(); }

/** 
  @brief Interact constant time (very fast)
  esta elements with l 
*/
  HTList & swap(HTList & l)
  {
    std::swap(head, l.head);
    std::swap(tail, l.tail);
    return *this;
  }

/** 
  @brief Insert link as the first element. 
*/
  void insert(Slinknc * link)
  {
    assert(NEXT(link) == NULL);

    if (head == NULL)
      {
	assert(tail == NULL);
	head = tail = link;
	return;
      }
    
    NEXT(link) = head;
    head = link;
  }

/** 
  @brief Insert link as the first element. 
*/
  void push(Slinknc * link) { insert(link); }

/** 
  @brief Insert link as the last element. 
*/
  void append(Slinknc * link)
  {
    assert(link != NULL and NEXT(link) == NULL);

    if (head == NULL)
      {
	assert(tail == NULL);
	head = tail = link;
	return;
      }

    NEXT(tail) = link;
    tail = link;
  }

/** 
  @brief Inserted at the end the whole list this l; l becomes empty. 
*/
  void append(HTList & l)
  {
    if (l.is_empty())
      return;

    if (this->is_empty())
      {
	this->swap(l);
	return;
      }
    
    NEXT(tail) = l.head;
    tail = l.tail;
    l.head = l.tail = NULL;
  }

/** 
  @brief Insert link at the bottom of the list. 
*/
  void put(Slinknc * link) { append(link); }

/** 
  @brief Concatenated to whole this list l; l becomes empty. 
*/
  void concat(HTList & l) { append(l); }

/** 
  @overload concat 
*/
  void concat_list(HTList & l) { append(l); }

/** 
  @brief Insert whole this list before  l; l becomes empty. 
*/
  void insert(HTList & l)
  {
    l.append(*this);
    this->swap(l);
  }

/** 
  @brief Inserted from link (contained in the this list)
  list list. list becomes empty. 
*/
  void insert(Slinknc * link, HTList & list)
  {
    NEXT(link) = list.head;
    tail = list.tail;
    list.head = list.tail = NULL;
  }

/** 
  @brief Removes the element at the head (the first).

  @return Returns the deleted item. 
*/
  Slinknc * remove_head()
  {
    if (is_empty())
      throw std::underflow_error("HTList is empty");

    Slinknc * ret_val = head;
    if (head == tail)
      head = tail = NULL;
    else
      {
	head = NEXT(head);
	if (head == NULL)
	  tail = NULL;
      }

    ret_val->reset();

    return ret_val;
  }
  
/** 
  @brief Removes the first element from this 
*/
  Slinknc * remove_first() { return remove_head(); }

/** 
  @brief Removes the link to link direction.
  
  @return Returns true if in effect was eliminated. 
*/
  bool remove(Slinknc * link)
  {
    if (is_empty())
      throw std::domain_error("Removing from a empty list");

    if (link == head)
      {
	head = NEXT(head);
	link->reset();
	return true;
      }

    for (Slinknc * prev = head, * p = NEXT(head); p != NULL; 
	 prev = p, p = NEXT(p))
      if (p == link)
	{
	  NEXT(prev) = NEXT(p);
	  if (link == tail)
	    tail = prev;
	  link->reset();
	  return true;	
	}

    return false;
  }

/** 
  @brief Eliminates the first element of this 
*/
  Slinknc * pop() { return remove_head(); }

/** 
  @brief divided this into two equal lists without changing the order of items.

  @details l contains the n / first 2 elements r n / 2 below.

  @return Returns the total number of elements on this. this becomes empty
*/
  size_t split_list(HTList & l, HTList & r)
  {
    assert(l.is_empty() and r.is_empty()); /* l y r should be empty */

    if (is_empty())
      return 0;

    if (is_unitarian())
      {
	swap(l);
	return 1;
      }

    size_t count = 0;
    Slinknc * p = head;
    Slinknc * q = head;
    while (q != tail and q != NULL)
      {
	q = NEXT(q); ++count;
	if (q == tail or q == NULL)
	  break;

	p = NEXT(p); 
	q = NEXT(q); ++count;
      }

    l.head = head;
    l.tail = p;

    r.head = NEXT(p);
    r.tail = tail;

    head = tail = NULL;

    return count;
  }

/** 
  @overload split_list 
*/
  size_t split(HTList & l, HTList & r)
  {
    return split_list(l, r);
  }

/** 
  @brief Reverses the items in the list.

  @return Returns the size of the list. 
*/
  size_t reverse()
  {
    HTList tmp;

    size_t count = 0;
    while (not is_empty())
      {
	tmp.insert(remove_first());
	++count;
      }

    swap(tmp);

    return count;
  }

/** 
  @overload reverse 
*/
  size_t reverse_list()
  {
    return reverse();
  }
  
/** 
  @brief Cut this for the link element and place the remaining elements
  (those following link) in list 
*/
  void cut(Slinknc * link, HTList & list)
  {
    assert(list.is_empty());

    list.head = NEXT(link);
    list.tail = tail;

    tail = link;
    NEXT(link) = NULL;
  }

/** 
  @overload cut 
*/
  void cut_list(Slinknc * link, HTList & list) { cut(link, list); }

/** 
  @brief Assuming that the elements were removed with new, deleted
  all elements of the list and frees the memory. 
*/
  void remove_all_and_delete() 
  {
    while (not is_empty())
      delete remove_head();
  }

/** 
  @brief Iterador over HTList 
*/
  class Iterator
  {
    HTList * lptr;
    Slinknc * curr;
    Slinknc * prev;
 
  public:

/** 
  @brief Constructor copy of interador

  @param [in] it Iterator to be copied
*/
    Iterator(const Iterator & it) 
      : lptr(it.lptr), curr(it.curr), prev(it.prev) { /* Empty */ }
    
    Iterator() : lptr(NULL), curr(NULL), prev(NULL) { /* Empty */ }

/** 
  @brief Initializes the iterator to the first element of the list list 
*/
    Iterator(const HTList & list) 
      : lptr(& (HTList&) list), curr(lptr->head), prev(curr)
    { /* Empty */ }

    void reset() 
    {
      prev = curr = lptr->head;
    }

    void reset_first() { reset(); }

    Iterator & operator = (const Iterator & it)
    {
      lptr = it.lptr;
      curr = it.curr;
      prev = it.prev;
      return *this;
    }

/** 
  @return Returns true if the iterator has current item 
*/
    bool has_curr() const { return curr != NULL; }

/** 
  @overload has_curr() 
*/
    bool has_current() const { return has_curr(); }
  
/** 
  @briefReturns the current node iterator

	@throw overflow_error If the iterator is out of range
*/
    Slinknc * get_curr() const
    {
      if (curr == NULL)
	throw std::overflow_error("Iterator overflow");

      return curr;
    }

    Slinknc * get_current() const { return get_curr(); }

    void next() 
    {
      if (curr == lptr->head)
	{
	  assert(prev == lptr->head);
	  curr = NEXT(curr);
	}
      else if (curr == lptr->tail)
	{
	  assert(NEXT(prev) == curr);
	  curr = NULL;
	}
      else
	{
	  assert(NEXT(prev) == curr);
	  prev = curr;
	  curr = NEXT(curr);
	  assert(NEXT(prev) == curr);
	}
    }

    Slinknc * del()
    {
      if (not has_curr())
	throw std::overflow_error("Iterator overflow");

      Slinknc * ret_val = NULL;
      if (curr == lptr->head) /* First item removal */
	{
	  ret_val = lptr->remove_first();
	  prev = curr = lptr->head;
	}
      else if (curr == lptr->tail) /* Last item removaln */
	{
	  assert(NEXT(prev) == curr);
	  ret_val = curr;
	  NEXT(prev) = NEXT(curr);
	  lptr->tail = prev;
	  curr = NULL; /* Put in overflow */
	}
      else
	{
	  assert(NEXT(prev) == curr);
	  ret_val = curr;
	  NEXT(prev) = NEXT(curr);
	  curr = NEXT(curr);
	}
      
      ret_val->reset();
      return ret_val;
    }
  };

  void insert(const Iterator & it, HTList & list) 
  { 
    insert(it.get_curr(), list);
  }

/** 
  @brief Cut this within the current position of the iterator.
  
  @details The remaining elements are put in list 
*/
  void cut_list(const Iterator & it, HTList & list) 
  {
    cut(it.get_curr(), list); 
  }

/** 
  @return Returns the size of the list.

  @warning This method has the elements of the list
*/
  size_t size() const
  {
    size_t count = 0;

    for (Iterator it(*this); it.has_curr(); it.next())
      ++count;

    return count;
  }
};


/** 
  @brief DynList modeled a list of elements of type T.

  @details sequence of elements of type T instrumented
  by singly-linked list.

  @see Slinknc Snodenc DynDList

  @ingroup Sequences
*/
    template <typename T = int>
class DynList : public HTList
{
public:

  DynList & swap(DynList & l) { return (DynList&) HTList::swap(l); }

  DynList() { /* Empty */ }

  typedef T Item_Type;

  typedef T Key_Type;

private:

  T & __insert(Snodenc<T> * p)
  {
    HTList::insert(p);
    return p->get_data();
  }

  T & __append(Snodenc<T> * p)
  {
    HTList::append(p);
    return p->get_data();
  }

public:

/** 
  @brief Inserts an item as the first item 
*/
  T & insert(const T & item)
  {
    return __insert(new Snodenc<T> (item));
  }

  T & insert(T && item)
  {
    return __insert(new Snodenc<T> (std::move(item)));
  }

/** 
  @brief Inserts an item as last element
*/
  T & append(const T & item)
  {
    return __append(new Snodenc<T> (item));
  }

  T & append(T && item)
  {
    return __append(new Snodenc<T> (std::move(item)));
  } 

/** 
  @brief Create a list item element 
*/
  DynList(const T & item)
  {
    insert(item);
  }

/** 
  @brief Create a list item element 
*/
  DynList(T && item)
  {
    insert(std::move(item));
  }

/** 
  @brief Removes the first element from the list 
*/
  T remove()
  {
    Slinknc * l = this->HTList::remove_head();
    Snodenc<T> * p = static_cast<Snodenc<T>*> (l);
    T ret_val = std::move(p->get_data());
    delete p;

    return ret_val;
  }
  
/** 
  @overload remove 
*/
  T remove_first() { return remove(); }

  T & get() 
  {
    if (is_empty())
      throw std::underflow_error("List is empty");

    Snodenc<T> * p = static_cast<Snodenc<T>*> (this->HTList::get_first());
    return p->get_data();
  }

  T & get_last() 
  {
    if (is_empty())
      throw std::underflow_error("List is empty");

    Snodenc<T> * p = static_cast<Snodenc<T>*> (this->HTList::get_last());
    return p->get_data();
  }

  const T & get_last() const 
  {
    return const_cast<DynList*>(this)->get_last();
  }

  T & get_first() { return get(); }

  const T & get_first() const { return get(); }

  T & get() const 
  { 
    DynList * ptr = const_cast<DynList*>(this);
    return ptr->get(); 
  }

/** 
  @brief Removes all items from the list 
*/
  void empty()
  {
    while (not is_empty())
      remove();

    assert(this->is_empty());
  }

  ~DynList() { empty(); }
  
    /* Iterador */
  class Iterator : public HTList::Iterator
  {
  public:

    typedef T Item_Type;

    typedef Iterator Iterator_Type;

    Iterator() { /* Empty */ }

    Iterator(const DynList & list) : HTList::Iterator(list) { /* Empty */ }

    T & get_curr() const
    {
      return ((Snodenc<T>*) (HTList::Iterator::get_curr()))->get_data();
    }

    T & get_current() const
    {
      return get_curr();
    }

    T del() 
    {
      Snodenc<T> * p = (Snodenc<T> *) this->HTList::Iterator::del();
      T ret_val = std::move(p->get_data());
      delete p;
      return ret_val;
    }
  };

/** 
  @brief Removes the first element found that meets the search criteria.

  @return Returns true if the item was found and removed 
*/
      template <class Equal = Aleph::equal_to<T>>
  bool remove(const T & item)
  {
    for (Iterator it(*this); it.has_curr(); it.next())
      if (Equal () (it.get_curr(), item))
	{
	  it.del();
	  return true;
	}

    return false;
  }

/** 
  @brief Create a copy list l 
*/
  DynList(const DynList & l)
  {
    for (typename DynList::Iterator it(l); it.has_curr(); it.next())
      append(it.get_curr());
  }

/** 
  @brief Create a copy list l 
*/
  DynList(DynList && l)
  {
    swap(l);
  }

  DynList(std::initializer_list<T> l)
  {
    std::for_each(l.begin(), l.end(), /* Lambda */ [this] (const T & item)
		  {
		    append(item); 
		  });
  }
  
/** 
  @brief Copy to this elements l
*/
  DynList & operator = (const DynList & l)
  {
    if (&l == this)
      return *this;

    empty();

    for (typename DynList::Iterator it(l); it.has_curr(); it.next())
      append(it.get_curr());
    
    return *this;
  }

  DynList & operator = (DynList && l)
  {
    return (DynList&) this->swap(l);
  }

/** 
  @brief Copy constant time items list at the end of this 
*/
  void append(DynList && list)
  {
    HTList::append(list);
  }

/** 
  @brief Copy constant time items list at the beginning of this 
*/
  void insert(DynList && list)
  {
    HTList::insert(list);
  }

/** 
  @brief Copy in linear time (O (| list |) list elements at the end ofthis 
*/
  void append(const DynList & list)
  {
    if (&list == this)
      return;

    DynList copy = list;
    HTList::append(copy);
  }

/** 
  @brief Copy in linear time (O (| list |) list the elements of the beginning of this 
*/
  void insert(const DynList & list)
  {
    if (&list == this)
      return;

    DynList tmp = list;
    HTList::insert(tmp);
  }

  T & get(const size_t & i)
  {
    Iterator it(*this);

    for (size_t __i = 0 ; it.has_current() and __i < i; it.next(), ++__i);

    return it.get_current();
  }

  T & operator [] (const size_t & i)
  {
    return get(i);
  }

  Generic_Traverse(T);

  Functional_Methods(T);
};


# undef NEXT

} /* End namespace Aleph */


# endif // HTLIST_H
