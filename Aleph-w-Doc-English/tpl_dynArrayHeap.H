
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
  @file tpl_dynArrayHeap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNARRAYHEAP_H
# define TPL_DYNARRAYHEAP_H

# include <tpl_dynArray.H>

namespace Aleph {


    template <typename T, class Compare> inline
void sift_up(DynArray<T> & a, size_t l, size_t r)
{
  for (size_t p, i = r; i > l; i = p) 
    {
      p = u_index(i);
      T & ap = a.access(p);
      T & ai = a.access(i);
    /** Does property order?
    If the answer is YES, then the whole array is a heap */
      if (Compare () (ap, ai)) 
        return;

      std::swap(ap, ai); /* Intercambie y restaure nivel p */
    }
}

    template <typename T, class Compare> inline
void sift_down(DynArray<T> & a, size_t l, size_t r)
{
  size_t i = l, c;
  while (true)
    {
      c = l_index(i); /* Índice del hijo izquierdo (c = i/2) */

      /** Is there a left child?
      If the answer is NO, then finish */
      if (c > r)
        return;

      T * ac = & a.access(c);
      /** Are there right child?
      If answer is YES, then choose the minor */
      if (c + 1 <= r)
	{
	  T * ac1 =  & a.access(c + 1);
	  if (Compare () (*ac1, *ac))
	    {
	      c++;
	      ac = ac1;
	    }
	}

      T & ai = a.access(i);
    /** Does property order?
    If the answer is YES, then finish */
      if (Compare () (ai, *ac)) 
        return;

      std::swap(*ac, ai); 
      i = c;
    }
}

/** 
  @brief Heap or priority queue implemented with arrays.

  @details ArrayHeap defines a heap implemented by array 
  static whose size is specified in time construction.

  @param T Type elements having the heap.
  @param Compare The comparison criterion between the 
  elements which determines the type of order and priority.

  @see BinHeap DynBinHeap

  @ingroup Arboles
*/
    template <typename T, class Compare = Aleph::less<T>> 
class DynArrayHeap 
{
  DynArray<T> array;
  size_t      num_items;

  static size_t r_index(const size_t & i)
  {
    return (i << 1) + 1; /* multiplica i por 2 y suma 1 */
  }

public:

/** @brief Constructor with dimension by default. */
  DynArrayHeap(Compare &) : num_items(0)
  { 
    /* Empty */
  }

  DynArrayHeap(Compare && cmp = Compare()) : num_items(0)
  { 
    /* Empty */
  }

/** @brief Returns the smallest element of the "heap". */
  T & top() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    return array.access(1);
  }

/** 
  @brief Inserts an element into the "heap".

  @details insert(key) inserted into the "heap" the key a copy of the
  "key" key.

  @param[in] key The key to insert.

  @return A reference to the modifiable element inserted.

  @throw overflow_error If the internal array is full.
*/
  T & insert(const T & key) throw(std::exception, std::overflow_error)
  {
    array.touch(++num_items) = key;  /* Colocar nuevo elemento */
    sift_up<T,Compare>(array, 1, num_items); /* Restaurar propiedad orden */
    return array.access(num_items);
  }

  T & insert(T && key) throw(std::exception, std::overflow_error)
  {
    array.touch(++num_items) = move(key);  /* Colocar nuevo elemento */
    sift_up<T,Compare>(array, 1, num_items); /* Restaurar propiedad orden */
    return array.access(num_items);
  }

  T & put(const T & key) { return insert(key); }

  T & put(T && key) { return insert(move(key)); }

/** 
  @brief Removes the smallest element of the "heap" and returns a copy of the value
  removed.

  @details getMin() extracted from the "heap" with lower value element as
  comparison criteria specified.

  @throw underflow_error If the "heap" is empty.

  @return A copy of the value removed.
*/
  T getMin() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    T & a1 = array.access(1);
    T ret_val = a1;
    a1 = array[num_items--]; 
    sift_down<T,Compare>(array, 1, num_items); /* Propiedad orden */

    array.cut(num_items + 1);

    return ret_val;
  }

/** @overload getMin() */
  T get() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** @overload getMin() to use if it reverses the order of priority */
  T getMax() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** @brief Returns the number of elements */
  const size_t & size() const { return num_items; }
  
/** @brief Returns true if the "heap" is empty */
  bool is_empty() const { return num_items == 0; }

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return array.traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation & operation) 
  {
    return array.traverse(operation); 
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return traverse<Operation>(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
  {
    return traverse<Operation>(operation);
  }

  Functional_Methods(T);
};



} /* End namespace Aleph */

# endif /* TPL_DYNARRAYHEAP_H
