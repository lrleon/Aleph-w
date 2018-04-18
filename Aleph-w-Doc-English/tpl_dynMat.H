
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
  @file tpl_dynMat.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


#ifndef TPL_DYNMAT_H
#define TPL_DYNMAT_H

# include <ahDry.H>
# include <tpl_dynArray.H>

namespace Aleph {

/**
  @Matrix dynamics.

  @details This class implements a rectangular matrix of n rows by m
  columns, based on dynarray type, containing elements
  generic type T. Since dynarray is based on many
  entries that have never been written up space
  from memory.

  By default, the entries in the matrix house value
  DynMatrix <T> :: Zero, which is the value that the
  default constructor of the generic type T :: T (), which requires
  obligation on the existence of this constructor.

  Access to an entry in a matrix mat is made by
  operator mat (i, j). This distinguishes readings operator
  scriptures, but it is a bit slower than access through
  the primitive read () or write (). Moreover, there
  situations where it can not be used by the operator access
  (I, j).

  Each entry i, j contains the Zero written independently
  that memory works or not.

  When an entry is first, it is checked whether
  memory exists apart to accommodate the entrance. If such is the
  case simplememte is written; otherwise, through
  dynarray, departs memory and is written with the value
  given. dynarray away memory may fail, in which case
  std :: exception triggers bad_alloc.

	@see DynArray

  @ingroup Secuencias
*/
    template <typename T> 
class DynMatrix 
{
  size_t n = 0;
  size_t m = 0;

  DynArray<T> * array_ptr = NULL;

  mutable T Zero = T();

  const T & read_array_entry(const size_t i) const
  {
    const T * entry_ptr = array_ptr->test(i);

    return entry_ptr == NULL ? Zero : *entry_ptr;
  }

  T & write_array_entry(const size_t i, const T & data)
  {
    T & ref = array_ptr->touch(i) = data;

    return ref;
  }

  size_t get_index(const size_t i, const size_t j) const
  {
    return i*m + j;
  }

public:

  void set_default_initial_value(const T & value)
  {
    array_ptr->set_default_initial_value(value);
  }

  void swap(DynMatrix & mat)
  {
    std::swap(n, mat.n);
    std::swap(m, mat.m);
    std::swap(Zero, mat.Zero);
    std::swap(array_ptr, mat.array_ptr);
  }

/** 
  @brief Readjusts the dimension of the array 
*/
  void set_dimension(const size_t __n, const size_t __m)
  {
    if (array_ptr != NULL) /* It is verified that it can be called from the ctor */
      delete array_ptr;

    n = __n;
    m = __m;
    size_t d, s, b;
    const size_t N = n*m;
    DynArray<T>::compute_sizes(N, d, s, b);

    array_ptr = new DynArray<T> (d, s, b);
    array_ptr->set_default_initial_value(Zero);
  }
/
/** 
  @brief It ensures that all the memory array is away
*/
  void allocate()
  {
    array_ptr->reserve(n*m);
  }

/** 
  @brief main dynamic matrix builder.

  @param [in] __n Number of rows
  @param [in] __M Number of columns
  @param [in] zero zero value should be a null entry or
  unwritten matrix. By default, the value is taken
  T :: T ().
  
  @throw bad_alloc If there is insufficient memory
*/
  DynMatrix(const size_t __n, const size_t __m, const T & zero = T())
    : n(__n), m(__m), array_ptr(NULL), Zero(zero)
  {
    set_dimension(n, m);
  }

  ~DynMatrix()
  {
    if (array_ptr)
      delete array_ptr;
  }

/** 
  @brief Builder copy.

  @details Creates a new dynamic array of efficient copy
  mat elements.

  @param[in] mat Matrizx to copy.

  @throw bad_alloc If there is insufficient memory.
*/
  DynMatrix(const DynMatrix<T> & mat) 
    : DynMatrix(mat.n, mat.m, mat.Zero)
  {
    *array_ptr = *mat.array_ptr;
  }

  DynMatrix(DynMatrix<T> && mat)
  {
    swap(mat);
  }

/** 
  @brief matrix Asignation.

  @details Asigna a this los elementos de mat.

  @param[in] mat Matrx to copy.

  @throw bad_alloc If there is insufficient memory.
*/
  DynMatrix<T> & operator = (const DynMatrix<T> & mat) 
  {
    if (this == &mat)
      return *this;

    n = mat.n;
    m = mat.m;
    array_ptr->copy_array_exactly(*this, mat);

    return *this;
  }

  DynMatrix<T> operator = (DynMatrix<T> && mat) 
  {
    swap(mat);
    return *this;
  }

  bool operator == (const DynMatrix<T> & mat) const
  {
    const size_t N = n*m;
    for (int i = 0; i < N; ++i)
      if (read_array_entry(i) != mat.read_array_entry(i))
	return false;

    return true;
  }

/** 
  @brief Returns the number of rows of the matrix
*/
  const size_t & rows() const { return n; }

/** 
  @brief Returns the number of columns of the matrix
*/
  const size_t & cols() const { return m; }

/** 
  @brief Read the value of row i and column j 
*/
  const T & read(const size_t i, const size_t j) const
  {
    assert(i < n and j < m);

    return read_array_entry(get_index(i, j));
  }

/** 
  @brief Write data in row i and column j

  @param [in] i index row
  @param [in] j column index
  @param [in] data Data to name the input i, j

  @throw bad_alloc If there is insufficient memory.
*/
  T & write(const size_t i, const size_t j, const T & data)
  {
    assert(i < n and j < m);

    return write_array_entry(get_index(i, j), data);
  }

/** 
  @brief Check input access spread
*/
  T & access(const size_t i, const size_t j)
  {
    return array_ptr->access(get_index(i, j));
  }

  T & operator () (const size_t i, const size_t j)
  {
    return access(i, j);
  }

  T & access(const size_t i, const size_t j) const
  {
    return array_ptr->access(get_index(i, j));
  }

  T & operator () (const size_t i, const size_t j) const
  {
    return access(i, j);
  }

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return array_ptr->traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation & operation) 
  {
    return array_ptr->traverse(operation); 
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

#endif // TPL_DYNMAT_H
