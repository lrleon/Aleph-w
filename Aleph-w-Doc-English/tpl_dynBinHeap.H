
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
  @file tpl_dynbinHeap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNBINHEAP_H
# define TPL_DYNBINHEAP_H

# include <ahDry.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binHeap.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Heap dynamic elements generic type T with criteria
  Compare comparison.

  @details DynBinHeap implements a dynamic heap of elements of type T
  sorted comparison criterion Compare () ().

  By dynamic it means that the heap can grow or decrease in
  depending on resources. In other words, is not subject
  by use of an internal arrangement

	@ingroup Arboles
*/
    template <class T, class Compare = Aleph::less<T>>
class DynBinHeap : public BinHeap<T, Compare>
{
public:

/** 
  @typedef The set type on which iterates. 
*/
  typedef DynBinHeap Set_Type;

/** 
  @typedef The type of item returning get_current(). 
*/
  typedef T Item_Type;

  typedef T Key_Type;

private:

  typedef typename BinHeap<T, Compare>::Node Node;

  T & __insert(Node * p)
  {
    return BinHeap<T, Compare>::insert(p)->get_key();
  }

  void copy(DynBinHeap & src)
  {
    for_each_in_preorder(src.top(), 
			 [this] (Node * p)
			 {
			   this->insert(new Node (p->get_key()));
			 });
  }

public:

  DynBinHeap(Compare & cmp) 
    : BinHeap<T, Compare>(cmp) 
  { /* Empty */ }

  DynBinHeap(Compare && cmp = Compare()) 
    : BinHeap<T, Compare>(cmp) 
  { /* Empty */ }

  DynBinHeap(const DynBinHeap & h)
  {
    copy(const_cast<DynBinHeap&>(h));
  }

  DynBinHeap(DynBinHeap && h)
  {
    this->swap(h);
  }

  DynBinHeap & operator = (const DynBinHeap & h)
  {
    if (this == &h)
      return *this;

    empty();

    copy(h);

    return *this;
  }

  DynBinHeap & operator = (DynBinHeap && h)
  {
    this->swap(h);
    return *this;
  }

/** 
  @brief Inserted into the heap a copy item.

  @details Insert a copy of items in the dynamic heap.

  @param[in] item Element inserted into the heap.

  @return A modified reference to the copied item and
  inserted into the heap.

  @throw bad_alloc If there is insufficient memory.
*/
  T & insert(const T & item) throw(std::exception, std::bad_alloc)
  {
    return __insert(new Node (item));
  }

  T & insert(T && item) throw(std::exception, std::bad_alloc)
  {
    return __insert(new Node (std::move(item)));
  }
  
/** 
  @brief Inserted into the heap a copy item.

  @details Synonymous with insert();

  @param[in] item Element inserted into the heap.

  @return A modified reference to the copied item and
  inserted into the heap.

  @throw bad_alloc If there is insufficient memory.
*/
  T & put(const T & item) throw(std::exception, std::bad_alloc)
  {
    return insert(item);
  }

  T & put(T && item) throw(std::exception, std::bad_alloc)
  {
    return insert(std::move(item));
  }

/** 
  @brief Removes the smallest element of a dynamic heap and returns a copy.

  @return A copy of the deleted item
  
  @throw underflow_error If the heap is empty.
*/
  T getMin() throw(std::exception, std::underflow_error)
  {
    Node * node = BinHeap<T, Compare>::getMin();

    T return_value = std::move(node->get_key());

    delete node;

    return return_value;
  }

/** 
  @overload getMin() to use if it reverses the order of priority 
*/
  T getMax() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** 
  @brief Removes the smallest element of a dynamic heap and returns a copy.

  @return A copy of the deleted item
  
  @throw underflow_error If the heap is empty.
*/
  T get() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** 
  @brief Updates a priority heap.

  @details update(data) making a reference to an element of the heap, whose
  presumably priority has been modified and updated its
  position in the heap.

  @param[in] data Modifiable reference to an element within the
  heap.

  @note The reference must be valid in the sense that it must
  be a reference returned by insert(). They are not made
  checks thereon.
*/
  void update(T & data)
  {
    Node * node = Node::key_to_node(data);
    BinHeap<T, Compare>::update(node);
  }

/** 
  @brief Removes an arbitrary element of the heap.

  @details remove(data) eliminates any element that is within
  the heap.

  data must belong to the heap.

  This operation is useful when some way, according to
  application, data element loses force in the
  heap. For example, the cancellation of an event.

  @param data Element to be removed.

  @note Unexpected results if data does not belong to the heap.
*/
  void remove(T & data)
  {
    Node * node = BinHeap<T, Compare>::Node::key_to_node(data);

    BinHeap<T, Compare>::remove(node);
  }
  
/** 
  @brief Synonymous remove. 
*/
  void erase(T & data)
  {
    remove(data);
  }
/** 
  @brief Returns a modifiable reference to the smallest element within
  the dynamic heap in the order determined by the criterion
  comparison given in the statement. 
*/
  T & top() const throw(std::exception, std::underflow_error)
  {
    return BinHeap<T, Compare>::top()->get_key();
  }

/** 
  @brief Empty all elements of the dynamic heap. 
*/
  void empty()
  {
    this->remove_all_and_delete();
  }

/** 
  @brief Destroyer. 
*/
  ~DynBinHeap()
  {
    empty();
  }

  template <class Operation>
  bool traverse(Operation & op) 
  {
    return this->level_traverse(this->getRoot(), [&op] (Node * p)
				{
				  return op(p->get_key());
				});
  }

  template <class Operation>
  bool traverse(Operation && op = Operation())
  {
    return traverse(op);
  }

  template <class Operation>
  bool traverse(Operation & op) const
  {
    return this->level_traverse(this->getRoot(), [&op] (Node * p)
				{
				  return op(p->get_key());
				});
  }

  template <class Operation>
  bool traverse(Operation && op = Operation()) const
  {
    return traverse<Operation>(op);
  }

  Functional_Methods(T);
};

} /* End namespace Aleph */

# endif // TPL_DYNBINHEAP_H
