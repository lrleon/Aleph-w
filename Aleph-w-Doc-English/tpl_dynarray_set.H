
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
  @file tpl_dynArray_set.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNARRAY_SET_H
# define TPL_DYNARRAY_SET_H

# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>

/** 
  @brief Set of elements implemented by a dynamic array.
    
  @details DynArray_Set implements a set of elements of type "T".

  The set is represented internally by dynamic array
  DynArray type. Consequently, the consumption
  memory is proportional to the number of elements.

  The insertion is very fast, the search is linear and the
  deleting an item already found is also very fast.

  The class takes two type parameters: the type of element
  assembly and a comparison class whose only role is to determine
  if an element is not equal to another.

  For reasons of speed, it allows duplicate elements.
  
  @see DynArray
  
  @ingroup Secuencias
*/
   template <typename T, class Equal = Aleph::equal_to<T>>
class DynArray_Set : public DynArray<T>
{
public:

/** @brief Default constructor */
  DynArray_Set(const size_t & dim = 0) 
    : DynArray<T>(dim)
  {
    /* Empty */
  }

/** 
  @brief Specialized builder.

  @details Defines a dynamic array with sizes directory, segment
  and block.

  @param[in] pow_dir Power of two directory size.
  @param[in] pow_seg Power of two size segment.
  @param[in] pow_block Power of two block size.

  @throw bad_alloc If there is no memory to divide the
  directory. This may be because it is too large or
  because there is little available memory.
  @throw length_error If the selected sizes exceed the 
  maximum dimension possible
  @throw overflow_error If "overflow" occurs with the operations
  of bits.
*/
  DynArray_Set(size_t pow_dir, size_t pow_seg, size_t pow_block)
    :  DynArray<T>(pow_dir, pow_seg, pow_block)
  {
    /* Empty */
  }

/** @brief Insert of "item".
  T & put(const T & item) { return this->append(item); }

/** @brief Search "item" in the set.
Returns pointer if found; NULL otherwise. */
  T * search(const T & item) 
  {
    int i = 
      Aleph::sequential_search<T, Equal>(*this, item, 0, this->size() - 1);
    //Aleph::random_search<T, Equal>(*this, item, 0, this->size() - 1); */
    return (i >= 0 and i < this->size()) ? &this->access(i) : NULL;
  }

/** @brief Eliminates the element of the "item" set, which must have
previously found using "search" or be the result of "insert". */
  void remove(T & item)
  {
    std::swap(item, this->access(this->size() - 1));
    this->cut(this->size() - 1);
  }

/** @brief @overload remove */
  void erase(T & item) { remove(item); }

  class Iterator
  {
    DynArray_Set & array;
    size_t curr;

  public:
    Iterator(DynArray_Set & _array)
      : array(_array), curr(0)
    {
      /* Empty */
    }

    bool has_current() const
    {
      return curr < array.size();
    }  

    bool has_curr() const
    {
      return has_current();
    }

    T & get_current()
    {
      if (not has_current())
        throw std::overflow_error("Iterator is at the end of array");

      return array(curr);
    }

    T & get_curr()
    {
      return get_current();
    }

    void next()
    {
      if (not has_current())
        throw std::overflow_error("Iterator is at the end of array");
      ++curr;
    }

    void prev()
    {
      if (curr == 0)
        throw std::underflow_error("Iterator is at the begin of array");
      --curr;
    }

    void reset_first()
    {
      curr = 0;
    }

    void reset_last()
    {
      curr = array.size() - 1;
    }
  };
};

# endif // TPL_DYNARRAY_SET_H
