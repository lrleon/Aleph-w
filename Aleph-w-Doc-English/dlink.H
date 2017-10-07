
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
  @file dlink.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef DLINK_H
# define DLINK_H

# include <aleph.H>

using namespace Aleph;

namespace Aleph {

/**  
 @brief Link node doubly linked circular list with
  header node.
         
  @details The Dlink class implements all links handling it
  possible to manage nodes belonging to lists doubly
  linked , circular , with head node.

  Close attention should be paid to the fact that no defined Dlink
  a node, but the double bond.
         
  @ingroup Sequences 
  
  @see Macros_Dlink Dlink :: iterator Dnode DLIST DynDlist
*/
class Dlink 
{
protected: 

  mutable Dlink * prev; 
  mutable Dlink * next;

public:

  Dlink & swap(Dlink & l)
  {
    std::swap(prev, l.prev);
    std::swap(next, l.next);

    return *this;
  }
   
  Dlink() : prev(this), next(this) { /* Empty */ }

/** 
  @brief Reset copy constructor ( not copy) 
*/
  Dlink(const Dlink &) { reset(); } 

  Dlink(Dlink && l) : prev(this), next(this) 
  {
    swap(l);
  }

/** 
  @brief Assignment restarts ( not copy).

  @details assignment operator compatibility purposes only. in
  Actually no copy occurs. Only \c esta restarts for
  point to itself.

  @param [in] l List Heade
*/
  Dlink & operator = (const Dlink & l) 
  {
    if (this == &l) 
      return *this;

    if (not is_empty())
      throw std::invalid_argument ("left list must be empty");

    reset();

    return *this;
  }   

  Dlink & operator = (Dlink && l) 
  {
    return swap(l);
  }

/** 
  @brief Restart dlink
  (equivalent to the list is empty )
*/
  void reset() 
  {
    assert(this != NULL);
    next = prev = this; 
  }

/**
  @brief Initializes dlink, whens are used malloc use 
*/
  void init() 
  {
    reset();
  }

/** 
  @brief Exchange lists headed with \c esta y \c link in constant time.

  @param [in] Header link Link to other exchange list
*/
  void swap(Dlink * link) 
  {
    if (is_empty() and link->is_empty()) 
      return;

    if (is_empty())
      {
        link->next->prev = this;
        link->prev->next = this;
        next = link->next;
        prev = link->prev;
        link->reset();

        return;
      }

    if (link->is_empty())
      {
        next->prev = link;
        prev->next = link;
        link->next = next;
        link->prev = prev;
        reset();

        return;
      }

    std::swap(prev->next, link->prev->next);
    std::swap(next->prev, link->next->prev);
    std::swap(prev, link->prev);
    std::swap(next, link->next);
  }

/** 
  @brief Returns true if \c this is empty. 
*/
  bool is_empty() const { return this == next and this == prev; }

/** 
  @brief Returns true if \c this have exactly one element. 
*/
  bool is_unitarian() const { return this != next and next == prev; }

/** 
  @brief Returns true if \c this have one or no element. 
*/
  bool is_unitarian_or_empty() const { return next == prev; }

/** 
  @brief Insert node after the this.

  @param[in] isolated node node should point to itself without being
  other elements enlado. 
*/
  void insert(Dlink * node) 
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));
    assert(node != NULL); 
    assert(node->is_empty());

    node->prev = this;
    node->next = next;
    next->prev = node;
    next       = node;
  }

/** 
  @brief Insert node as stack 

  @param[in] isolated node node should point to itself without being
  other elements enlado
*/
  void push(Dlink * node) 
  {
    insert(node);
  }

/** 
  @brief Insert node before \c this

  @param[in] Pointer node to node; the node must not be a list
*/
  void append(Dlink * node) 
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));
    assert(node != NULL);
    assert(node->is_empty());

    node->next = this;
    node->prev = prev;
    prev->next = node;
    prev       = node;
  }

/** 
  @brief Returns link after \c this 
*/
  Dlink *& get_next()
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));
    return next; 
  }

/** 
  @brief Returns the firts node 
  (like stack) 
*/
  Dlink * top() { return get_next(); }

/** 
  @brief Returns link before \c this 
*/
  Dlink *& get_prev()
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));
    return prev; 
  }

/** 
  @brief Returns the first element of \c this 
*/
  Dlink *& get_first() { return next; }

/** 
  @brief Returns the last element of \c this \c this 
*/
  Dlink *& get_last() { return prev; } 

/** 
  @brief Enter led by head after node \c list this.

  @details The function assumes that this rate is a node that forms a list and
  from it entirely want to insert the list whose head node is \c head

  @param [in] head head of the list to be inserted in the
  node \c esta
*/
  void insert_list(Dlink * head)
  {
    if (head->is_empty()) 
      return;

    head->prev->next = next;
    head->next->prev = this;
    next->prev       = head->prev;
    next             = head->next;
    head->reset();
  }

/** 
  @brief Enter led by head before node \c list this.

  @details The function assumes that this rate is a node that forms a list and
  before him you want to insert entirely the list whose head node is \c head

  @param [in] Header head of the list to be inserted before
  node \c esta
*/
  void append_list(Dlink * head)
  {
    if (head->is_empty()) 
      return;

    head->next->prev = prev;
    head->prev->next = this;
    prev->next       = head->next;
    prev             = head->prev;
    head->reset();
  }

/** 
  @brief Concatenates a list headed by head headed by list this.

  @details The method assumes that this rate is a header of a list and
  entirely concatenated list whose head is head.

  After the operation becomes empty head.

  @param head header list you want to concatenate
*/
  void concat_list(Dlink * head) 
  { 
    assert(head != NULL);

    if (head->is_empty()) 
      return;

    if (this->is_empty())
      {
        swap(head);

        return;
      }

    prev->next       = head->next;
    head->next->prev = prev;
    prev             = head->prev;
    head->prev->next = this;
    head->reset();
  }

  void concat_list(Dlink & head) 
  { 
    concat_list(&head);
  }

/** 
  @brief Eliminates \c esta of context in a list 
*/
  void del() 
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));

    prev->next = next;
    next->prev = prev;
    reset();
  }

/** 
  @brief Eliminates \c this of context in a list 
*/
  void erase() { del(); }

/** 
  @brief Eliminates before \c this.

  @details Deletes the previous node \c this.

  @return newly deleted node address
*/
  Dlink * remove_prev() 
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));
    assert(prev != this); 
    assert(next != this); 

    Dlink* retValue = prev;
    retValue->del();

    return retValue;
  }

/** 
  @brief Eliminates successor \c this 
*/
  Dlink * remove_next() 
  {
    assert((this != NULL) and (next != NULL) and (prev != NULL));
    assert(prev != this); 
    assert(next != this); 

    Dlink* retValue = next;
    retValue->del();

    return retValue;
  }

/** 
  @brief Eliminates the last element this 
*/
  Dlink * remove_last() 
  {
    return remove_prev();
  }

/** 
  @brief Eliminates the firts element this 
*/
  Dlink * remove_first() 
  {
    return remove_next();
  }

/** 
  @brief Removes the first element this
  (as if battery) 
*/
  Dlink * pop() 
  {
    return remove_next();
  }

/**  
  @brief Invest led by \c esta list and returns the amount
  of list elements.
           
  @return Number of items you have listed
*/
  size_t reverse_list() 
  {
    if (is_empty()) 
      return 0;

/** 
  @brief Temporary head where inverted list is saved 
*/
    Dlink tmp; 

/** 
  @brief Browse esta list , first delete and insert tmp 
*/  
    size_t counter = 0; 
    for (/* Nothing */; not is_empty(); counter++)
      tmp.insert(remove_next()); /* Eliminates and insert in tmp */

    swap(&tmp); /* tmp == inverted list; this empty ==> swap */

    return counter;
  }

/**  
  @brief Partitions -led "this" in two lists list

  @details Partitions headed by esta list into two lists
  equitable that are placed in l and r respectively.
  Returns the number of items that had esta before partition.

  The method takes the list whose head node and is esta
  partitioned equally by the center.

  After the operation the this becomes empty.

  @param [out] l List the first n / 2 elements
  @param [out] r List the last n / 2 elements
  
  @return Total number of items that had the this list before the operation.
*/
  size_t split_list(Dlink & l, Dlink & r) 
  {
    assert(l.is_empty() and r.is_empty()); /* l y r deben estar vacías */

    size_t count = 0;
    while (not is_empty())
      {
        l.append(remove_next()); ++count;

        if (is_empty())
          break;

        r.insert(remove_prev()); ++count;
      }

    return count;
  }

/**  
  @brief list \c esta for the link \c link and spends all
  items to the empty list \c list.

  @details Partitions the list whose head node is \c esta , from
  one of its targeted by \c link nodes and places all
  elements from \c link to the bottom of the list to the empty list \c list.

  @param [in] Pointer to the link list item \c esta from
  which you want to partition.
  
  @return list header empty list. After
  operation , this list contains the items that were in
  \c esta from \c link
*/ 
  Dlink cut_list(Dlink * link) 
  {
    assert(not is_empty() and not link->is_empty() and link != this);

    Dlink list;
    if (link == this->prev) /* Is link the last of the list? */
      {
	link->del();
	list.append(link);
	return list;
      }

    if (link == this->next) /* Is link the first of the list */
      {
	list.swap(this);
	assert(this->is_empty());
	return list;
      }

    list.prev = this->prev;       /** Link list to link ( cutoff ) */
    list.next = link; 
    this->prev = link->prev;      /** Remove everything from esta link */
    link->prev->next = this;
    link->prev = &list;           /** Place the cut list*/
    list.prev->next = &list; 
    
    return list;
  }

/**  
  @brief iterator over links

  @ingroup Sequences 
*/
  class Iterator 
  {
  private:

    mutable Dlink * head;
    mutable Dlink * curr;

  public:

/** 
  @typedef Joint type which is iterated. 
*/
    typedef Dlink Set_Type;

/** 
  @typedef Type element containing the set. 
*/
    typedef Dlink * Item_Type;

/** 
  @brief Instance an iterator over list.

  @details Instance an iterator over list. \c head_ptr is the pointer to
  header node.

  The iterator is initiated on the first element of the list.
            
  @param [in] head_ptr pointer to the top of the list on which you want to iterate.
*/  
    Iterator(Dlink * head_ptr) : head(head_ptr), curr(head->get_next()) {}

/** 
  @brief Instance node list iterator over head

  @details Instance an iterator over list header node \c head.
  The iterator is initiated on the first element of the list.

  @param [in] _head reference to the head of the list on the
  which you want to iterate.
*/  
    Iterator(const Dlink & _head) 
      : head(&const_cast<Dlink&>(_head)), curr(head->get_next())
    {
      /* Empty */
    }

/** 
  @brief Instance node list iterator over head pointed

  @details Instance node list iterator over head pointed
  by \c head_ptr from pointed to by \c curr_ptr node.

  The iterator is initiated on targeted by \c curr_ptr element.

  @param [in] head_ptr pointer to the top of the list on the
  which you want to iterate.
  @param [in] curr_ptr pointer to the current item on which
  want the iterator starts.
  This node must belong to the list \c head_ptr
*/
    Iterator(Dlink * head_ptr, Dlink * curr_ptr) 
      : head(head_ptr), curr(curr_ptr)
    {
      /* Empty */
    }

/** 
  @brief default constructor 
*/
    Iterator() : head(NULL), curr(NULL) { /* Empty */ }

/** 
  @brief Iterator restarts first node in the list 
*/
    void reset_first() 
    {
      assert(curr != NULL and head != NULL);

      curr = head->get_next();
    }

/** 
  @brief Iterator restarts at the last node in the list 
*/
    void reset_last() 
    {
      assert(curr != NULL and head != NULL);
      curr = head->get_prev();
    }

/** 
  @brief Changes to current targeted by \c new_curr node.

  @details Changes the current element of the iterator \c esta the targeted element
  \c new_curr.
          
  @param [in] new_curr pointer to the new element.

  @note \c new_curr must imperatively point to an element of the
  same list iteration.
*/
    void set(Dlink * new_curr) 
    {
      assert(curr != NULL and head != NULL);
      curr = new_curr;
    }

/** 
  @brief Change list iterator header node \c new_head and
  puts the current node pointed to by \c new_curr.

  @details Change the iterator to iterate over the list \c new_head from
  the current item \c new_curr.

  @param [in] new_head pointer to the new list of iteration
  @param [in] New Item new_curr acts.
 
  @note new_curr must belong to the list \c new_head.
*/
    void reset(Dlink * new_head, Dlink * new_curr)
    {
      head = new_head;
      curr = new_curr;
    }

/** 
  @brief Change iteration list pointed to by \c new_head.

  @details This method deletes the state of iteration and restarts to the list
  \c new_head.

  @param [in] new_head New list iteration.
*/
    void reset(Dlink * new_head)
    {
      head = new_head;
      curr = head->get_next();;
    }

/** 
  @brief Returns true if the current element has yet iterator 
*/
    bool has_current() const 
    {
      assert(curr != NULL and head != NULL);
      return curr != head;
    }

/** 
  @brief Synonymous of has_current() 
*/
    bool has_curr() const { return has_current(); }

/** 
  @brief Returns direction of actual node 
*/
    Dlink * get_current() const
    {
      assert(curr != NULL and head != NULL);

      if (not has_current())
        throw std::overflow_error("Not element in list");

      return curr;
    }

/** 
  @brief Synonymous of get_current() 
*/    

    Dlink * get_curr() const { return get_current(); }

/** 
  @brief Returns true if the iterator is on first element 
*/
    bool is_in_first() const { return curr == head->next; }

/** 
  @brief Returns true if the iterator is on the last item
*/
    bool is_in_last() const { return curr == head->prev; }

/** 
  @brief Iterator back in a position
*/
    void prev() throw(std::exception, std::underflow_error)
    {
      if (not has_current())
        throw std::underflow_error("Not previous element in list");

      curr = curr->get_prev();
    }

/** 
  @brief Iterator advances in a position 
*/
    void next() throw(std::exception, std::overflow_error)
    {
      if (not has_current())
        throw std::overflow_error("Not next element in list");

      curr = curr->get_next();
    }

/** 
  @brief Returns true if iterators are on the same element. 
*/
    bool operator == (const Iterator & it) const { return curr == it.curr; }

/** 
  @brief Returns true if iterator have different states. 
*/
    bool operator != (const Iterator & it) const { return curr != it.curr; }
    
/** 
  @brief Eliminates current element of the iterator and leave it in the following
  element newly removed.

  @return pointer to newly deleted
*/
    Dlink * del() 
    {
      assert(curr != NULL and head != NULL);
      assert(has_current());

      Dlink * current = get_current(); /** Get current node */
      next(); /* Advance to the next node */
      current->del(); /* Remove from old current node list */
      return current;
    } 

    bool verify(Dlink * l) const { return head == l; }

    bool verify(const Iterator & it) const { return head == it.head; }
  };

/** 
  @brief Removes and frees all memory nodes this.

  @note elements were eliminated from with \c new
*/
  void remove_all_and_delete() 
  {
    for (Iterator itor(this); itor.has_current(); delete itor.del()) ;
  }

  bool check()
  {
    Iterator itor(this);
    Dlink* node;

    for (/* Nothing */; itor.has_current(); itor.next())
      {
        node = itor.get_current();

        if (not (node->get_next()->get_prev() == node))
          return false;

        if (not (node->get_prev()->get_next() == node))
          return false;
      }

    for (itor.reset_last(); itor.has_current(); itor.prev())
      {
        node = itor.get_current();

        if (not (node->get_next()->get_prev() == node))
          return false;

        if (not (node->get_prev()->get_next() == node))
          return false;
      }

    return true;
  }
};

/** 
  @brief generates a conversion function name double bond
  structure that contains it.

  @details This macro is used when you have a dlink which is part of a
  structure is desired and a pointer to the structure from
  the link.

  If we, for example:
  struct Registroew
    {
      ...
      Dlink l;
      ...
    };

  Then DLINK_TO_TYPE (Registration, l) generated function:

  Registration * dlink_to_Registro (Dlink * link), which receives
  Dlink pointer to a record field "Register" and returns
  pointer to register.

  @param type_nameparam type structure (struct or class) that
  It contains at dlink.
  @param link_nameparam name field of the double bond within
  the structure.
 
  @ingroup Sequences
*/  
# define DLINK_TO_TYPE(type_name, link_name) \
inline static type_name * dlink_to_##type_name(Dlink * link) \
{ \
  type_name * ptr_zero = 0; \
  size_t offset_link = reinterpret_cast<size_t>(&(ptr_zero->link_name));\
  char * address_type = reinterpret_cast<char*>(link) - offset_link; \
  return reinterpret_cast<type_name *>(address_type); \
} 

/** 
  @brief It generates conversion function name double bond
  structure that contains it.

  @details The function name is literally
  the parameter to be instantiated as link_name

  This macro is used when you have two or more Dlink are
  part of a structure and want to get a pointer to the
  structure since some of the links.

  If we, for example:
  struct Registro
  {
  ...
  Dlink l1;
  Dlink l2;
  ...
  };

  Then LINKNAME_TO_TYPE (Registration, l1) and
  LINKNAME_TO_TYPE (Registration, l2) generate functions:

   - # Registration * l1_to_type (Dlink * link), which receives
   l1 pointer to field registration and returns
   pointer to register.
      
   - # Registration * l2_to_type (Dlink * link), which receives
   l2 pointer to the field of registration and returns
   pointer to register.
  .

  The idea is to have naming schemes that allow
  distición make between fields.

  @param type_name Type structure (struct or class) that
  It contains at Dlink.      
  @param link_name Name field of the double bond within the structure.
  
  @ingroup Sequences
*/  
# define LINKNAME_TO_TYPE(type_name, link_name) \
inline static type_name * link_name##_to_##type_name(Dlink * link) \
{ \
  type_name * ptr_zero = 0; \
  size_t offset_link = reinterpret_cast<size_t>(&(ptr_zero->link_name));\
  char * address_type = reinterpret_cast<char*>(link) - offset_link; \
  return reinterpret_cast<type_name *>(address_type); \
} 


/** 
  @brief link_to_base function generates a double bond
  structure that contains it.

  @details This macro is used when you have a dlink which is part of a
  structure, it is desired to obtain a pointer to the structure from
  link and a fixed name is required. Other macros generate
  names containing the name of the class.

  If we, for example:
  struct Record
  {
   ...
   Dlink l;
   ...
  };

  Then DLINK_TO_BASE (Registration, l) generated function:

  Registration * dlink_to_base (Dlink * link), which receives
  Dlink pointer to a record field "Register" and returns
  pointer to register.

  @param type_name Type structure (struct or class) that
  It contains at dlink.
  @param link_name Name field of the double bond within
  the structure.

  @ingroup Sequences
*/  
# define DLINK_TO_BASE(type_name, link_name)				\
  inline static type_name * dlink_to_base(Dlink * link)			\
  {									\
    type_name * ptr_zero = 0;						\
    size_t offset_link = reinterpret_cast<size_t>(&(ptr_zero->link_name)); \
    char * address_type = reinterpret_cast<char*>(link) - offset_link;	\
    return reinterpret_cast<type_name *>(address_type);			\
  } 

}

# endif /* DLINK_H */ 
