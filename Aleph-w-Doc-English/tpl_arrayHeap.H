 
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
  @file tpl_arrayHeap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_ARRAYHEAP_H
# define TPL_ARRAYHEAP_H

# include <ahFunction.H>
# include <ahUtils.H>
# include <ahDefs.H>
# include <ahAssert.H>
# include <ahDry.H>

namespace Aleph {


    template <typename T, class Compare> inline
T & sift_up(T * ptr, const size_t l, const size_t r)
{
  size_t i = r;
  for (size_t p; i > l; i = p) 
    {
      p = u_index(i); /* Parent Index   (c = i/2) */
    
    /** Does this comply with the order property?
    If the answer is YES, then the whole array is a heap */
      if (Compare () (ptr[p], ptr[i])) 
        return ptr[i]; 

      std::swap(ptr[p], ptr[i]); /* Exchange and restore level p */
    }

  return ptr[i];
}

    template <typename T, class Compare> inline
void sift_down(T * ptr, const size_t l, const size_t r)
{
  size_t i = l, c;
  while (true)
    {
      c = l_index(i); /* Left Child Index (c = i/2) */
    
    /** Is there a left child?
    If the answer is NO, then finish */
      if (c > r) 
        return;
    
    /** Are there right child?
    If the answer is YES, then choose the minor */
      if (c + 1 <= r) 
        if (Compare () (ptr[c + 1], ptr[c])) 
          c++;
    
    /** Does this comply with the order property?
    If the answer is YES, then finish */
      if (Compare () (ptr[i], ptr[c]))
        return;

      std::swap(ptr[c], ptr[i]); 
      i = c;
    }
}

    template <typename T, class Compare> inline
void sift_down_up(T * ptr, const size_t l, const size_t i, const size_t r)
{
  sift_down <T, Compare> (ptr, i, r);
  sift_up <T, Compare> (ptr, l, i);
}

/** 
  @brief Sorts an array by heapsort method.

  @details heapsort(array,n) employs the method of heapsort to sort 
  the array of n elements.

  Heapsort has a guaranteed performance \f$O(n \; \lg n)\f$
  and it is stable.
  
  The method uses two type parameters:
  -# T: Elements Type having the array.
  -# Compare: Comparison class.

  An overloaded heapsort() version specialize the 
  comparison to operator "less than" the type T.

  @param[in,out] array Array to order.
  @param[in] n Dimension of the array.
  
  @see insertion_sort() quicksort_rec() mergesort() 
  @see selection_sort() faster_heapsort()
  
  @ingroup Algos
*/
    template <typename T, class Compare = Aleph::less<T> > 
void heapsort(T * array, const size_t n)
{
  --array; /* Move backward position ==> array[1] == first */
  for (int i = 2; i <= n; ++i)
    sift_up <T, Aleph::Inversed_Compare<T, Compare> > (array, 1, i);  
  for (int i = n; i > 1; --i)
    {
      std::swap(array[1], array[i]); /* Put in root i-th item */
      sift_down<T,Aleph::Inversed_Compare<T,Compare> >(array, 1, i - 1);
    }
}

/** 
  @brief Sorts an array by the improved heapsort method.

  @details faster_heapsort(array,n) employs a version substantially
  improved of heapsort to order the array of n elements.

  Heapsort has a guaranteed performance \f$O(n \; \lg n)\f$
  and it is stable.
  
  The method uses two type parameters:
  -# T: Elements Type having the array.
  -# Compare: Comparison class.

  An overloaded faster_heapsort() version specialize the 
  comparison to operator "less than" the type T.

  @param[in,out] array Array to order.
  @param[in] n Dimension of the array.
  
  @see insertion_sort() quicksort_rec() mergesort() heapsort() 
  @see selection_sort() heapsort()
  
  @ingroup Algos
*/
    template <typename T, class Compare = Aleph::less<T> >
void faster_heapsort(T * array, const size_t n)
{
  --array; /* Move backward position ==> array[1] == first */
  for (int i = n/2; i >= 1; --i)
    sift_down <T, Aleph::Inversed_Compare<T, Compare> > (array, i, n);
  for (int i = n; i > 1; --i)
    {
      std::swap(array[1], array[i]); /* Put in root i-th item */
      sift_down<T,Aleph::Inversed_Compare<T,Compare> >(array, 1, i - 1);
    }
}

/** @brief Returns true if array between l and r is a heap with
comparison criterion Compare()() */
      template <typename T, class Compare>
bool valid_heap(T * array, const size_t l, const size_t r) 
{
  size_t i;
  for (i = l_index(l) /* i = 2*l */; i <= r; i++)
    if (Compare () (array[i], array[u_index (i)]))
      break;
  return i > r;
}

/** 
  @brief Heap or priority queue implemented with arrays.

  @details ArrayHeap defines a heap implemented by an array
  static whose size is specified in time
  construction.

  @param T Elements Type having the heap.
  @param Compare Comparison criteria between
  elements which determines the type of order and priority.

  @see BinHeap DynBinHeap

  @ingroup Arboles
*/
    template <typename T, class Compare = Aleph::less<T>> 
class ArrayHeap 
{
  T * array;
  const size_t dim;
  size_t       num_items;

  static size_t r_index(const size_t & i)
  {
    return (i << 1) + 1; /* Multiply i by 2 and add 1 */
  }

  mutable bool array_allocated;

public:

/** @brief Default constructor with dimension. */
  ArrayHeap(const size_t & d = 1024) 
    : array(NULL), dim(d), num_items(0), array_allocated(false) 
  { 
    array = new T [d + 1];
    array_allocated = true;
  }

/** @brief Constructor with array and dimension. */
  ArrayHeap(T * ptr, const size_t & d) 
    : array(ptr), dim(d), num_items(0), array_allocated(false) 
  { 
    /* Empty */
  }

/** @brief Destroyer. */
  virtual ~ArrayHeap() 
  {
    if (array_allocated and array != NULL)
      delete [] array;
  }

/** @brief Returns the smallest element of the heap. */
  T & top() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    return array[1];
  }

/** 
  @brief Inserts an element into the heap.

  @details insert(key) inserted into the heap the key a copy of the
  keyword key.

  @param[in] key Key to insert.
  
  @return A modifiable reference to the element inserted.
  
  @throw overflow_error If the internal array is full.
*/
  T & insert(const T & key) throw(std::exception, std::overflow_error)
  {
    if (num_items >= dim)
      throw std::overflow_error("Heap out of capacity"); 

    array[++num_items] = key;  /* Place new element */
    return sift_up<T,Compare>(array, 1, num_items); 
  }

/** 
  @brief Removes the smallest element of the heap and returns a copy of the deleted value.

  @details getMin() extract from heap the element with lower value as
  comparison criteria specified.

  @throw underflow_error If the heap is empty.
  
  @return A copy of the value removed.
*/
  T getMin() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    T ret_val = array[1];
    array[1] = array[num_items--]; 
    sift_down<T, Compare>(array, 1, num_items); 
    return ret_val;
  }

/** @overload getMin() */
  T get() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** @overload getMin() to use if the priority order is reversed. */
  T getMax() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

/** @brief Returns the number of elements. */
  const size_t & size() const { return num_items; }
  
/** @brief Returns true if the heap is empty. */
  bool is_empty() const { return num_items == 0; }

/** 
  @brief Upgrade priority of a reference contained in the heap.

  @details update(data) takes a reference to an element in the heap,
  presumably modified and updated its priority within the
  heap. The idea is that if for some reason a priority should be
  modified, then this can be updated.

  The reference should have been obtained by a previous call
  to insert().

  @param[in] data Reference to the entry within the heap that
  to be updated
  
  @see insert()
  
  @note It is essential that data is a valid reference to the heap.
  Results are unpredictable (and probably fatal) if this
  it is not the case.
*/
  void update(T & data) 
  {
    assert(&data >= &array[0] and &data <= &array[dim]);

    const size_t i = &data - array;
    sift_down_up <T, Compare> (array, 1, i, num_items);
    sift_up <T, Compare> (array, i, num_items);
  }

  void remove(T & item)
  {
    item = array[num_items--];
    update(item);
  }

/** @brief Returns the i-th entry of heap. */
  T & operator [] (const size_t i)
  {
    return array[i];
  }

private:

    /* super fast array traversal */
  template <class Operation>
  bool __traverse(Operation & operation)
  {
    for (size_t i = 1; i <= num_items; ++i)
      if (not operation(array[i]))
	return false;

    return true;
  }

public:

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return const_cast<ArrayHeap&>(*this).__traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation & operation) 
  {
    return __traverse(operation); 
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return const_cast<ArrayHeap&>(*this).__traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
  {
    return traverse(operation);
  }

  Functional_Methods(T);
};

} /* End namespace Aleph */

# endif /* TPL_ARRAYHEAP_H */
