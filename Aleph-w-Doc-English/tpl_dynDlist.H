
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
  @file tpl_dynDlist.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNDLIST_H
# define TPL_DYNDLIST_H

# include <ahFunctional.H>
# include <ahDry.H>
# include <tpl_dnode.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Listbrief dynamic element type T.

  @details DynDlist <T> defines a dynamic list is a sequence of elements
  some kind of T.

  This type can be used as a stack or queue.

  @param T Type of items of the list.

  @see DynDlist::Iterator

  @ingroup Secuencias
*/
    template <typename T = int> 
class DynDlist : public Dnode<T>
{
  size_t num_elem;

public:

/** 
  @typedef The set type on which iterates. 
*/
  typedef DynDlist Set_Type;

/** 
  @Typedef The type of element that houses the set. 
*/
  typedef T Item_Type;

  typedef T Key_Type;

/** 
  @Typedef Returns the number of item that has the list. 
*/
  const size_t & size() const { return num_elem; }

  DynDlist() : num_elem(0) { /* Empty */ }

/** 
  @brief Empty all items from the list.
*/
  void empty() 

  {
    while (not this->is_empty())
      delete this->remove_next();

    num_elem = 0;
  }

/** 
  @brief Destructor 
*/
  ~DynDlist() 
  { 
    empty();
  }

private:
  
  T & __insert(Dnode<T> * p)
  {
    Dnode<T>::insert(p);
    ++num_elem;
    return p->get_data();
  }

  T & __append(Dnode<T> * p)
  {
    Dnode<T>::append(p);
    ++num_elem;
    return p->get_data();
  }

public:

/** 
  @brief Inserts an item to the top of the list.

  @details Inserted in the "this" list as the first element a _data copy.
  After the operation the first item in the list is _data.

  @param[in] item Data to be inserted.

  @return A reference to the newly inserted data.

  @throw bad_alloc If no memory for the new element.
*/
  T & insert(const T & item) throw(std::exception, std::bad_alloc)
  {
    return __insert(new Dnode<T> (item));
  }

  T & insert(T && item) 
  {
    return __insert(new Dnode<T> (std::move(item)));
  }
/** 
  @brief Inserts an element at the end of the list.

  @details Insert in the "this" list as the last item back _data.
  After of the operation the last item in the list is _data.

  @param[in] _data Data to be inserted.
  
  @return A reference to the newly inserted data.
  
  @throw bad_alloc If no memory for the new element.
*/
  T & append(const T & item) throw(std::exception, std::bad_alloc)
  {
    return __append(new Dnode<T> (item));
  }

  T & append(T && item) throw(std::exception, std::bad_alloc)
  {
    return __append(new Dnode<T> (std::move(item)));
  }

/** 
  @brief Inserts "list" before "this"; the result is "list-this".

  @details The method takes all the elements of the "list" list and
  sequentially inserted before "this" list. The "list" list
  It becomes empty.

  The method takes constant time.

  If required to "list" is not emptied, then the process must
  be copied "list" and then perform this operation on the copy.

  @param[in,out] list The list to be copied to the principle of "this". This
  parameter becomes empty after insertion.
  
  @return The new amount of elements of "this"
*/
  size_t insert_list(const DynDlist & list)
  {
    Dlink::insert_list(&list);
    num_elem += list.num_elem;
    list.num_elem = 0;

    assert(list.is_empty());

    return num_elem;
  }

  size_t insert_list(DynDlist && list)
  {
    Dlink::insert_list(&list);
    num_elem += list.num_elem;
    list.num_elem = 0;

    assert(list.is_empty());

    return num_elem;
  }

/** 
  @brief Inserts "list" after "this"; the result is "list-this".

  @details The method takes all the elements of the "list" list and
  sequentially inserted after "this" list. The "list" list
  It becomes empty.

  The method takes constant time.

  If required to "list" is not emptied, then the process must
  be copied "list" and then perform this operation on the copy.

  @param[in,out] list The list to be copied at the end of "this". This
  parameter becomes empty after insertion.
  
  @return The new amount of elements of "this"
*/
  size_t append_list(DynDlist & list)
  {
    Dlink::append_list(&list);
    num_elem += list.num_elem;
    list.num_elem = 0;

    assert(list.is_empty());

    return num_elem;
  }

  size_t append_list(DynDlist && list)
  {
    Dlink::append_list(&list);
    num_elem += list.num_elem;
    list.num_elem = 0;

    assert(list.is_empty());

    return num_elem;
  }

/** 
  @brief Returns a reference to the first item in the list. 
*/
  T & get_first() throw(std::exception, std::underflow_error)
  {
    return this->get_next()->get_data();
  }

  const T & get_first() const throw(std::exception, std::underflow_error)
  {
    return const_cast<DynDlist&>(*this).get_next()->get_data();
  }

/** 
  @brief Returns a reference to the last item in the list. 
*/
  T & get_last() throw(std::exception, std::underflow_error)
  {
    return this->get_prev()->get_data();
  }

  const T & get_last() const throw(std::exception, std::underflow_error)
  {
    return const_cast<DynDlist&>(*this).get_prev()->get_data();
  }

/** 
  @brief Removes the first element of the list: returns a copy of the
  deleted item.

  @return A copy of the deleted item.

  @throw underflow_error If the list is empty.
*/
  T remove_first() throw(std::exception, std::underflow_error)
  {
    Dnode<T> * ptr = this->remove_next();
    T retVal = ptr->get_data();
    delete ptr;
    --num_elem;

    return retVal;
  }

/** 
  @brief Removes the last element of the list: returns a copy of the
  deleted item.

  @return A copy of the deleted item.
      
  @throw underflow_error If the list is empty.
*/
  T remove_last() throw(std::exception, std::underflow_error)
  {
    Dnode<T> * ptr = this->remove_prev();
    T retVal = ptr->get_data();
    delete ptr;
    --num_elem;

    return retVal;
  }

/** 
  @brief If this is a queue, then puts the
item element. 
*/
  T & put(const T & item) { return append(item); }

  T & put(T && item) { return append(std::move(item)); }

/** 
  @brief If this is a queue, then extracts
oldest item. 
*/
  T get() { return remove_first(); }

/** 
  @brief If this e short tail, then returns the
younger element. 
*/
  T & rear() { return get_last(); }

/** 
  @brief If this e short tail, and then returns the oldest item. 
*/
  T & front() { return get_first(); }

/** 
  @brief If this is a stack, then insert item. 
*/
  T & push(const T & item) { return insert(item); }

/** 
  @brief If this It is a stack, then inserted item. 
*/
  T & push(T && item) { return insert(std::move(item)); }

/** 
  @brief If this is a stack, then removed the top. 
*/
  T pop() { return remove_first(); } 

/** 
  @brief If this is a stack, then returns the top. 
*/
  T & top() const { return get_first(); }
  
/** 
  @brief Eliminates data element. 

  @note Attention: data must be a reference to
  one element in the sequence. 
*/
  void remove(T & data)
  {
    Dnode<T> * p = Dnode<T>::data_to_node(data);
    p->del();
    delete p;
    --num_elem;
  }

/** 
  @brief Eliminates data element. 

  @note Attention: data must be a reference to
  one element in the sequence. 
*/
  void erase(T & data)
  {
    remove(data);
  }

/** 
  @brief Interactbrief constant time all the elements of "this" with
  the list l.

  @details The operation takes constant time (very fast) regardless
  the number of items that hold back the two lists.

  @param l List exchanged with "this"
*/
  void swap(DynDlist & l)
  {
    std::swap(num_elem, l.num_elem);
    this->Dlink::swap(&l);
  }

/** 
  @brief Partitions estabrief the middle esta dynamic list.

  @details The method partitions the this list by the time center
  linear. The first n / 2 elements are copied to the following
  n / 2 r.

  If n is odd, then l will contain n / 2 + 1 items.

  @param [out] l First partition.
  @param [out] r Second partition.
  
  @throw domain_error If any of the lor lists are not empty.
*/
  void split_list(DynDlist & l, DynDlist & r) 
    throw(std::exception, std::domain_error)
  {
    if ((not l.is_empty()) or (not r.is_empty()))
      throw std::domain_error("lists are not empty");

    Dlink::split_list(l, r);
    l.num_elem = r.num_elem = num_elem/2;

    if (num_elem % 2 == 1) /* ¿Es num_elem impar? */
      l.num_elem++;

    num_elem = 0;
  }

/** 
  @brief Dynamic list iterator over.

  @ingroup Secuencias
*/
  class Iterator : public Dnode<T>::Iterator
  {
    DynDlist * list_ptr; /* A pointer to the list */
    int        pos;      /* Position of the current element in the sequence */

  public:
/** 
  @Typedef The set type on which iterates. 
*/
    typedef DynDlist Set_Type;

/** 
  @typedef The type of item returning get_current(). 
*/
    typedef T Item_Type;

/** 
  @brief Returns the ordinal position of the current element iterator. 
*/
    const int & get_pos() const { return pos; }

/** 
  @brief Avanza forward an iterator over a dynamic list.

  @return The current position of the iterator.

  @throw overflow_error If the iterator has no current element.
*/
    const int & next()
    {
      Dnode<T>::Iterator::next();
      pos++;
      return pos;
    }

/** 
  @brief Avanza back to an iterator over a dynamic list.

  @return The current position of the iterator.

  @throw underflow_error If the iterator has no current element.
*/
    const int & prev()
    {
      Dnode<T>::Iterator::prev();
      pos--;
      return pos;
    }

/** 
  @brief Restart dynamic iterator on the first element of the list list.

  @return 0 (which is the ordinal position of the first element).
*/
    const int & reset_first()
    {
      Dnode<T>::Iterator::reset_first();
      pos = 0;
      return pos;
    }

/** 
  @Restart brief dynamic iterator over the last list item list.

  @return tThe number of elements minus one
  (which is the ordinal position of the last element).
*/
    const int & reset_last()
    {
      Dnode<T>::Iterator::reset_last();
      pos = list_ptr->num_elem - 1;
      return pos;
    }

/** 
  @brief Builder list iterator over _list
*/
    Iterator(DynDlist<T> & _list) 
      : Dnode<T>::Iterator(_list), list_ptr(&_list), pos(0) 
    {
      /* Empty */
    }

/** 
  @brief Builder constant list iterator over _list
*/
    Iterator(const DynDlist<T> & _list) 
      : Dnode<T>::Iterator(const_cast<DynDlist<T>&>(_list)), 
        list_ptr(&const_cast<DynDlist<T>&>(_list)), pos(0) 
    {
      /* Empty */
    }

    Iterator() : list_ptr(NULL) 
    {
      /* Empty */
    }

/** 
  @brief Mapping and iterator. 
*/
    Iterator & operator = (const Iterator & it) 
    { 
      Dnode<T>::Iterator::operator = (it);
      list_ptr = it.list_ptr;
      pos      = it.pos;

      return *this;
    }

/** 
  @brief Returns the current element of the iterator. 
*/
    T & get_current() const
    {
      return Dnode<T>::Iterator::get_current()->get_data(); 
    }

/** 
  @brief Returns the current element of the iterator. 
*/
    T & get_curr() const
    {
      return get_current(); 
    }

/** 
  @brief Inserts an element after the current element of the iterator.

  @details This method inserts a copy of _data after the current element
  on the iterator is positioned. The current element
  iterator remains unchanged.

  @param[in] _data Element value to insert

  @throw overflow_error If the iterator has no current element
  @throw bad_alloc If there is insufficient memory.
*/
    void insert(const T & item) 
      throw(std::exception, std::overflow_error, std::bad_alloc)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T>::Iterator::get_current()->insert(new Dnode<T>(item));
      ++list_ptr->num_elem;
    }

    void insert(T && item) 
      throw(std::exception, std::overflow_error, std::bad_alloc)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T>::Iterator::get_current()->
	insert(new Dnode<T>(std::move(item)));
      ++list_ptr->num_elem;
    }

/** 
  @brief Inserts an element before the current element of the iterator.

  @details This method inserts a copy of _data before the current item
  on the iterator is positioned. The current element
  iterator remains unchanged.

  @param[in] _data Element value to insert

  @throw overflow_error If the iterator has no current element
  @throw bad_alloc If there is insufficient memory.
*/
    void append(const T & item) 
      throw(std::exception, std::overflow_error, std::bad_alloc)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T>::Iterator::get_current()->append(new Dnode<T>(item));
      ++list_ptr->num_elem;
    }

    void append(T && item) 
      throw(std::exception, std::overflow_error, std::bad_alloc)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T>::Iterator::get_current()->
	append(new Dnode<T>(std::move(item)));
      ++list_ptr->num_elem;
    }

/** 
  @brief Inserts a dynamic list after the current element.

  @details The method inserts all elements of list from
  current element of the iterator. The list becomes empty after list
  operation.

  The operation Takes time constant.

  If what is required is to insert a copy, then it must
  perform the operation on a copy of list.

  @param [in, out] list List inserting after the item
  current. It becomes empty after the operation.

  @throw overflow_error If the iterator is not positioned on a
  current item.
*/
    void insert_list(const DynDlist & list) 
      throw(std::exception, std::overflow_error)
    {

      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T>::Iterator::get_current()->insert_list(&list);
      list_ptr->num_elem += list.num_elem;
      list.num_elem = 0;

      assert(list.is_empty());
    }

/** 
  @brief Inserts a dynamic list before the current element.

  @details The method inserts all elements of list before
  current element of the iterator. The list becomes empty after list
  operation.

  The operation Takes time constant.

  If what is required is to insert a copy, then it must
  perform the operation on a copy of list.

  @param [in, out] list List item to insert before
  current. It becomes empty after the operation.
  
  @throw overflow_error If the iterator is not positioned on a
  current item.
*/
    void append_list(const DynDlist & list) 
      throw(std::exception, std::overflow_error)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T>::Iterator::get_current()->append_list(&list);
      list_ptr->num_elem += list.num_elem;
      list.num_elem = 0;

      assert(list.is_empty());
    }

/** 
  @brief Removes the current item and moves the iterator forward.

  @details This method eliminates the current item on which it is
  positioned and advances to the next item removed.

  @return A copy of the deleted item.

  @throw overflow_error If the iterator is not Positioned on a
  current item.
*/
    T del() throw(std::exception, std::overflow_error)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T> * ptr = Dnode<T>::Iterator::get_current(); 
      T ret_val      = ptr->get_data(); 
      Dnode<T>::Iterator::next();
      ptr->del();
      delete ptr;
      --list_ptr->num_elem;
      return ret_val;
    }

/** 
  @brief Removes the item predecessor of today.

  @details This method eliminates the predecessor element to the current element
  iterator. After the operation, the iterator is in the
  same position.

  @return A copy of the deleted item.

  @throw overflow_error If the iterator is not Positioned on a
  current item.
*/
    T remove_prev() throw(std::exception, std::overflow_error)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T> * curr_ptr = Dnode<T>::Iterator::get_current(); 
      Dnode<T> * ptr      = curr_ptr->remove_prev();
      T ret_val = ptr->get_data(); 
      delete ptr;
      --list_ptr->num_elem;

      return ret_val;
    }

/** 
  @brief eliminates the successor of the current element.

  @details This method eliminates the successor to the current item element
  iterator. After the operation, the iterator is in the
  same position.

  @return A copy of the deleted item.

  @throw overflow_error If the iterator is not Positioned on a
  current item.
*/
    T remove_next() throw(std::exception, std::overflow_error)
    {

      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      Dnode<T> * curr_ptr = Dnode<T>::Iterator::get_current(); 
      Dnode<T> * ptr      = curr_ptr->remove_next();
      T ret_val = ptr->get_data(); 
      delete ptr;
      --list_ptr->num_elem;

      return ret_val;
    }

/** 
  @brief Cut a dynamic list from the current element.

  @details The method eliminates all the elements of the dynamic list
  iterator from the current position. The elements
  deleted are copied to the "list" list.

  The "list" list should be empty.

  The operation takes constant time.

  After the operation the iterator is no current element.

  @param[out] list List where the items are placed
  eliminated.

  @return The amount of deleted items.

  @throw overflow_error If the iterator has no current element.
  @throw domain_error If "list" is not empty.

  @note @b Note: Please note that since after the operation the
  iterator is no current item, a call to next() will cause
  overflow_error.
*/
    size_t cut_list(DynDlist & list)
    {
      if (not this->has_current())
        throw std::overflow_error ("DynDlist Iterator has not current");

      list_ptr->Dnode<T>::cut_list(Dnode<T>::Iterator::get_current(), &list);
      list.num_elem = list_ptr->num_elem - pos; 
      list_ptr->num_elem -= pos;

      return list.num_elem;
    }
  };

/** 
  @brief Dynamic allocation list with lvalue semantics.

  @details The allocation removes all elements of "this" and then copy
  "this" the elements contained in the "list" list.

  @param[in] list The list to be assigned

  @throw bad_alloc If no memory.
*/
  DynDlist<T> & operator = (const DynDlist & list)
  {
    if (this == &list) 
      return *this;

    empty();

    assert(this->is_empty());

    for (typename DynDlist<T>::Iterator itor(const_cast<DynDlist&>(list)); 
	 itor.has_current();itor.next())
      this->append(itor.get_current()); 

    return *this;
  }

/** 
  @brief Copy constructor; all esta elements are copied. 
*/
  DynDlist(const DynDlist & list) : Dnode<T>(list)
  {
    this->reset();
    num_elem = 0;
    
    assert(this->is_empty());

    for (typename DynDlist<T>::Iterator itor(const_cast<DynDlist&>(list)); 
	 itor.has_current();itor.next())
      this->append(itor.get_current()); 
  }

/** 
  @brief Copy constructor rvalue semantics; all the
  this elements are copied.
*/
  DynDlist(DynDlist<T> && list) : Dnode<T>(std::move(list)), num_elem(0)
  {
    swap(list);
  }

  DynDlist(std::initializer_list<T> l)
  {
    std::for_each(l.begin(), l.end(), /* Lambda */ [this] (const T & item)
		  {
		    append(item);
		  }
		  );
  }

/** 
  @brief Dynamic allocation list with rvalue semantics.

  @details The allocation removes all elements of "this" and then copy
  "this" the elements contained in the "list" list.

  @param[in] list The list to be assigned

  @throw bad_alloc If no memory.
*/
  DynDlist<T> & operator = (DynDlist && list)
  {
    swap(list);
    return *this;
  }
 
  T & operator [] (const size_t & n)
  {
    typename DynDlist<T>::Iterator it(*this);

    for (int i = 0; i < n and it.has_current(); i++, it.next()) ;

    return it.get_current();
  }

  Generic_Traverse(T);

  Functional_Methods(T);
};

} /* End namespace Aleph */

# endif /* TPL_DYNDLIST_H */
