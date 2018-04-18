
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
  @file ahUtils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AHUTILS_H
# define AHUTILS_H

# include <ahAssert.H>
# include <ahFunction.H>

namespace Aleph {


/** 
  @brief Returns a pointer to the median of three data.
  @details a pointer in order to save the copy is returned , and most
  especially for equality by using pointers.

	@ingroup fcts
*/
  template <class T, class Compare = Aleph::less<T>> inline
const T * median(const T & a, const T & b, const T & c, Compare & cmp)
{
  if (cmp(a, b))
    if (cmp(b, c))
      return &b;
    else if (cmp(a, c))
      return &c;
    else
      return &a;
  else if (cmp(a, c))
    return &a;
  else if (cmp(b, c))
    return &c;
  else
    return &b;
}

/** 
  @brief Returns <code>true</code> if n pair

  @ingroup fcts
*/
inline bool is_even(long n)
{
  return (n % 2) == 0;
}

/** 
  @brief Returns <code>true</code> if n odd

  @ingroup fcts
*/
inline bool is_odd(long n)
{
  return not is_even(n);
}

/** 
  @brief Converts a nibla stored a <code> int < / code > to < code < > char / code>.

  @ingroup fcts
*/
inline char nibble_to_char(const int & i)
{
  assert(i >= 0 and i <= 15);

  const char ret = i < 10 ? i + '0' : i - 10 + 'A';

  assert((ret >= '0' and ret <= '9') or (ret >= 'A' and ret <= 'F'));

  return ret;
}

/** 
  @brief Converts a nibla stored a <code<>char/code> to <code>int</code>.

  @ingroup fcts
*/
inline int char_to_nibble(const char & c)
{
  assert((c >= '0' and c <= '9') or (c >= 'A' and c <= 'F'));

  const int ret = c < 'A' ? c - '0' : c - 'A' + 10; 

  assert(ret >= 0 and ret <= 15);

  return ret;
} 

# define DERIVATE_ITERATOR(container_name,  base_it_name, it_name) \
  struct it_name : public base_it_name				   \
  {								   \
    it_name() { /* Empty */ }					   \
								   \
    it_name(container_name & c) : base_it_name(c)		   \
      {								   \
	     /* Empty */						   \
      }								   \
								   \
    it_name(const it_name & it) : base_it_name(it)		   \
      {								   \
	     /* Empty */						   \
      }								   \
								   \
    it_name & operator = (const it_name & it)			   \
      {								   \
	return base_it_name::operator = (it);			   \
      }								   \
  }; 


/** 
  @brief Resizes new_size battery system. Returns true if
  He succeeds ; false otherwise

  @ingroup fcts
*/
extern bool resize_process_stack(size_t new_size);

/** 
  @brief Divide i between 2 
*/
inline size_t u_index(const size_t & i)
{
  return i >> 1; 
}

/** 
  @brief Multiply i per 2 
*/
inline size_t l_index(const size_t & i)
{
  return i << 1; 
}


}


# endif // AHUTILS_H
