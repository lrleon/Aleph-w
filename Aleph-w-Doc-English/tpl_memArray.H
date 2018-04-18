 
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
  @file tpl_memArray.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_MEMARRAY_H
# define TPL_MEMARRAY_H

# include <utility>
# include <stdlib.h>
# include <math.h>
# include <stdexcept>

# include <ahDry.H>

namespace Aleph
{

    /* TODO Documentar para que se usa esta clase. */
    template <typename T>
class MemArray
{
protected:

  T *    ptr;
  size_t dim;
  size_t n;
  size_t contract_threshold;

  void allocate()
  {
    ptr = new T [dim];
    contract_threshold = dim / 4; 
  }

/** @brief Return true if expansion is done */
  bool expand(const size_t first = 0)
  {
    if (n < dim)
      return false;

    const size_t newsz = dim << 1; /* 2*dim */
    T * new_ptr = new T [newsz];
    for (int i = 0; i < dim; ++i)
      std::swap(ptr[(i + first) % dim ], new_ptr[i]);
    delete [] ptr;
    ptr = new_ptr;
    dim = newsz;
    contract_threshold = dim/4;

    return true;
  }

/** @brief Return true if contraction is done */
  bool contract(const size_t first = 0)
  {
    if (n > contract_threshold)
      return false;

    const size_t newsz = dim >> 1; /* dim/2 */

    T * new_ptr = new T [newsz];

    for (int i = 0; i < newsz; ++i)
      std::swap(ptr[(first + i) % dim], new_ptr[i]);

    for (int i = newsz; i < dim; ++i)
      ptr[i].~T();

    delete [] ptr;
    ptr = new_ptr;
    dim = newsz;
    contract_threshold = dim/4;

    return true;
  }

  /* if dim == 2^k returns dim; else next two power 2^k > dim  */
  static size_t init_dim(const size_t dim)
  {
    if (dim == 0 or ((dim > 0) and (dim & (dim - 1)) == 0))
      return dim;

    return 1 << (int) (log2(dim) + 1);
  }

public:

/** @brief Returns the capacity of the array */
  const size_t & capacity() const { return dim; }

  const size_t & size() const { return n; }

  bool is_empty() const { return n == 0; }

/** 
  @brief Builder that specifies maximum capacity.

  @details Build a dynamic array with a capacity of \f$ 2^{tp}\f$ elements.

  @param[in] __dim Dimension array proposal. If __dim is
  exactly a power of two, then this will be the ability
  of array. Otherwise, the ability to adjust to the
  nearest power of two that is sufficient to accommodate the
  dimension. 

  @throw bad_alloc If there is insufficient memory.
*/
  MemArray(size_t __dim = 8)
    : ptr(NULL), dim(init_dim(__dim)), n(0)
  {
    allocate();
  }

  ~MemArray()
  {
    if (ptr != NULL)
      {
	delete [] ptr;
	ptr = NULL;
      }
  }

  void swap(MemArray & a)
  {
    std::swap(ptr, a.ptr);
    std::swap(dim, a.dim);
    std::swap(n, a.n);
    std::swap(contract_threshold, a.contract_threshold);
  }

  MemArray(const MemArray & a) 
    : ptr(NULL), dim(a.dim), n(a.n)
  {
    allocate();
    for (int i = 0; i < dim; ++i)
      ptr[i] = a.ptr[i];
  }

  MemArray(MemArray && a) 
    : ptr(NULL), dim(0), n(0), contract_threshold(0)
  {
    swap(a);
  }

  MemArray & operator = (const MemArray & a)
  {
    if (this == &a)
      return *this;

    T * newptr = new T [a.n];      /* Allocate new array */
    for (int i = 0; i < a.n; ++i)  /* Copy items to new array */
      newptr[i] = a.ptr[i];

    delete [] ptr;
    ptr = newptr;
    dim = a.dim;
    n = a.n;

    return *this;
  }

  MemArray & operator = (MemArray && a)
  {
    swap(a);
    return *this;
  }

  void empty()
  {
    n = 0;
    if (dim == 1)
      return;

    contract();
  }

/** @brief Place item at the end of array,
It expands when the end is reached */
  T & put(const T & item)
  {
    expand();

    T & ret = ptr[n++] = item;
    return ret;
  }

  T & put(T && item)
  {
    expand();
    
    T & ret = ptr[n++] = std::move(item);
    return ret;
  }

  T & put(const size_t i, const T & item)
  {
    if (i >= dim)
      throw std::range_error("index out of range");

    return ptr[i] = item;
  }

  T & put(const size_t i, T && item)
  {
    if (i >= dim)
      throw std::range_error("index out of range");

    return ptr[i] = std::move(item);
  }

/** @brief Insert entries for the array. */
  void putn(const size_t more)
  {
    n += more;
    expand();
  }
  
/** @brief Remove i entries,
Returns the last deleted entry */
  T get(int i = 1) 
  {
    int idx = n - i;
    if (idx < 0)
      throw std::underflow_error("Deleted more entries than capacity");

    n = idx;
    T ret = std::move(ptr[n]);

    contract();

    return ret;
  }

  T & last() { return ptr[n - 1]; }

  T & first() { return ptr[0]; }

  T & access(const size_t i)
  {
    return ptr[i];
  }

  const T & last() const { return ptr[n - 1]; }

  const T & first() const { return ptr[0]; }

  const T & access(const size_t i) const
  {
    return ptr[i];
  }

/** @brief Access to the i-th valid entry */
  T & operator [] (const size_t i) 
  {
    return ptr[i];
  }

  const T & operator [] (const size_t i) const
  {
    if (i >= n)
      throw std::range_error("access out of range");

    return ptr[i];
  }

  T & operator () (const size_t i) 
  {
    assert(i < dim);
    return ptr[i];
  }

  const T & operator () (const size_t i) const
  {
    assert(i < dim);
    return ptr[i];
  }

  template <class Operation>
  bool traverse(Operation & operation)
  {
    for (int i = 0; i < n; i++)
      if (not operation(ptr[i]))
	return false;

    return true;
  }

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return const_cast<MemArray*>(this)->traverse(operation);
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

# endif // TPL_MEMARRAY_H
