
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

// <algorithm> -*- C++ -*-

// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/**
  @file ahAlgo.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


    //    /**********************************************
	  //    Aleph implementation of C++ standard algorithms
    //    **********************************************/


# ifndef AHALGO_H
# define AHALGO_H

# include <ahAssert.H>
# include <ahUtils.H>

typedef size_t size_type;


    /* Non Modifying Algorithms */

namespace Aleph
{

/**
  @brief Execute trades <code>op</code> on each element of
  where the targeted container iterators <code>beg</code> y
  <code>end</code>. 

  @param[in] beg Iterator to the first element primer of the container from
  where to start the search. 
  @param[in] end Iterator the last element of the container.
  @param[in] op Operation to be perfomed on each element.
      
  @return A copy of the element op

  @ingroup Somenthings 
*/
    template <class Itor, class Operation> inline
Operation for_each (Itor beg, const Itor & end, Operation op)
{
  while (beg != end)
    op (*beg++);
	
  return op;
}

/**
  @brief Count the occurrences if the elements who operation 
  <code>op</code> certain. 

  @param[in] beg Iterator the first element of the container.
  @param[in] end Iterator the last element of the container.
  @param[in] op Operation to be perfomed on each element.
      
  @return The quantity of the elements for which some return
  operation

  @ingroup Algos
*/
    template<class Itor, class Operation> inline
typename Itor::difference_type count_if(Itor beg, const Itor & end, 
					Operation op)
{
  typename Itor::difference_type n = 0;

  while (beg != end)
    if (op (*beg++))
      ++n;
	
  return n;
}

/**  
  @brief Count the occurences of the elements which the value it is the equal to
  <code>value</code>. 

  @param[in] beg Iterator the first element of the container from
  where you want to start the search. 
  @param[in] end Iterator the last element of the container
  @param[in] value Value to compare
   
  @return The quantity of the elements it is the equal to <code>value</code>

  @ingroup Algos
*/
    template <class Itor, class T> inline
typename Itor::difference_type count(const Itor & beg, const Itor & end, 
				     const T & value)
{
  return Aleph::count_if (beg, end, Aleph::bind2nd(equal_to<T>(), value));
}

/** 
  @brief Returns the smallest element that involves container iterators. 

  @param[in] beg Iterator pointing to the first element from which
  you want to start the search. 
  @param[in] end Iterator the last element of the container.
  @param[in] op Types comparision
     
  @return An iterator positioned on the lower element.

  @ingroup Algos
*/
    template<class Itor, 
	     class CompFunc = Aleph::less<typename Itor::value_type> > inline
Itor min_element(Itor beg, const Itor & end, 
		 CompFunc op = CompFunc())
{
  Itor min = beg;
  beg++;

  while (beg != end)
    {
      if (op (*beg, *min))
	min = beg;

      ++beg;
    }

  return min;
}

/** 
  @brief Returns the largest container element that involves
  iterators. 

  @param[in] beg Iterator pointing to the first element where
  you want to start the search. 
  @param[in] end Iterator to the last element of the container..
  @param[in] op Types comparision
     
  @return An iterator positioned on the largest element.

  @ingroup Algos
*/
    template<class Itor, 
	     class CompFunc =  Aleph::greater<typename Itor::value_type> > 
    inline
Itor max_element(const Itor& beg, const Itor& end, CompFunc op = CompFunc())
{
  return Aleph::min_element (beg, end, op);
}


/** 
  @brief Find the first element of a container that fits in
  preached < code> op < / code>.

  @param [in] beg Iterator pointing to the first element from which
  to start the search.
  @param [in] end Iterator to the last element of the container.
  @param [in] op Class that determines whether or not an item satisfies the
  preached.
    
  @return Iterator pointing to the member found (if it was the case )
    
  @ingroup Algos
*/
    template<class Itor, class UnaryPredicate> inline
Itor find_if(Itor beg, const Itor & end, UnaryPredicate op)
{
  while (beg != end and not op(*beg))
    ++beg;

  assert(beg == end or op(*beg));

  return beg;
}	

/** 
  @brief Find the first element of a container whose value is equal
  <code > value < / code>.

  @param [in] beg Iterator pointing to the first element from which
  to start the search.
  @param [in] end Iterator to the last element of the container.
  @param [in] value Value to find.
 
  @return Iterator pointing to the member found (if it was the case )
    
  @ingroup Algos
*/	
    template<class Itor, class T> inline
Itor find(const Itor & beg, const Itor & end, const T & value)
{
  return 
    Aleph::find_if (beg, end, Aleph::bind2nd(Aleph::equal_to<T>(), value));
}

/** 
  @brief Returns the position of the first of the <code > count < / code> elements
  Consecutive which is in the range <code > beg < / code> and
  < code> end < / code> for which a predicate is true.

  details The predicate is invoked as:
  @code
  op () (item , value)
  @endcode

  @param [in] beg Iterator pointing to the first element
  container from where to start the search.
  @param [in] end Iterator to the last element of the container.
  @param [in] count Number of consecutive occurrences to be searched.
  @param [in] value Specific value of the item to be searched.
  @param [in] op Compared Criteria
      
  @return An iterator positioned at the first item found or
  end if there is no <code > count < / code> occurrences.
      
  @ingroup Algos
*/
    template<class Itor, class Size, class T, 
	     class BinaryPredicate = Aleph::equal_to<T> > inline
Itor search_n (Itor            beg, 
	       const Itor &    end, 
	       Size            count, 
	       const T &       value, 
	       BinaryPredicate op = BinaryPredicate() )
{
  if (count <= 0 or beg == end)
    return end;

  Size i = 0;
  Itor first;

  while (beg != end and i < count)
    {
      if (op(*beg, value))
	{
	  if (i == 0)
	    first = beg;
	  ++i;
	}
      else
	i = 0;
		
      ++beg;
    }

  if (i == count)
    return first;

  return end;
}

 
/** 
  @brief Search subranges.

  @details This function searches within the range given <code> beg </ code> and
  <Code> end </ code> the first accurate occurrence of subrange
  <Code> searchBeg </ code> and <code> searchEnd </ code>.

  To meet an occurrence of subrange, the routine returns a
  iterator positioned in the first element of the first
  subrange occurrence. Otherwise, return the iterator
  is <code> end </ code>.

  To compare items between containers, the routine invokes
  preached class <code> op </ code> as follows
   
  @code
  op (* beg * pivot)
  @endcode
   
  Where <code> beg </ code> is the current position of the range
  Search and <code> pivot </ code> is the current position
  I subrange.

  @note Note that the range in which search and find no subrange
  They can refer to different containers only, but also of
  types. 

  @example: For example, <code> [beg, end) </ code> could in September to
  a container of type <code> set </ code>, while
  <Code> [searchBeg, searchEnd) </ code> to a container type
  <Code> vector </ code>.

  @param [in] beg Iterator pointing to the first element
  container from where to start the search.
  @param [in] end Iterator to the last element of the container.
  @param [in] searchBeg Iterator pointing to the first element
  subrange for.
  @param [in] searchEnd Iterator to the last element of a subrange
  search.
  @param [in] op Operation comparison between elements of a
  range.
      
  @return If the subrange is found, iterator positioned on
  the first element of the first occurrence of the subrange. Than
  Otherwise <code> end </ code>.

  @note Very important to note that you can through op
  implement different containers, different class and
  elements of different types.

  @ingroup Algos
*/
  template<class Itor1, class Itor2, 
	   class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type> 
	   > inline
Itor1 search (Itor1 beg, const Itor1 & end, 
	      Itor2 searchBeg, const Itor2 & searchEnd, 
	      BinaryPredicate op = BinaryPredicate() )
{
  if (beg == end or searchBeg == searchEnd)
    return end;

  Itor1 first;
  Itor2 pivot = searchBeg;
  int count = 0;

  while (beg != end and pivot != searchEnd)
    {	
      if (op(*beg, *pivot))
	{
	  if (*beg == *searchBeg and count == 0)
	    first = beg;
		
	  ++pivot;
	  ++count;
	}
      else
	{
	  pivot = searchBeg;
	  count = 0;
	}

      ++beg;
    }
	
  if (pivot == searchEnd)
    return first;

  return end;
}


  template<class Itor1, class Itor2, 
	   class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type> >
  inline
Itor1 find_end (Itor1 beg, Itor1 end, 
		Itor2 searchBeg, Itor2 searchEnd, 
		BinaryPredicate op = BinaryPredicate())
{
  if (beg == end or searchBeg == searchEnd)
    return end;

  Itor1 ret_val = end;
  while (true)
    {
      Itor1 new_ret = search(beg, end, searchBeg, searchEnd, op);
      if (new_ret == end)
	return ret_val;
      else
	{
	  ret_val = new_ret;
	  beg = new_ret;
	  ++beg;
	}
    }
}


  template<class Itor1, class Itor2, 
	   class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>
	   > inline
Itor1 find_first_of (const Itor1& beg, const Itor1& end, 
		     Itor2 searchBeg, const Itor2& searchEnd, 
		     BinaryPredicate op = BinaryPredicate())
{
  while (searchBeg != searchEnd)
    {
      Itor1 current = beg;

      while (current != end)
	{
	  if (op (*current, *searchBeg))
	    return current;

	  ++current;
	}

      ++searchBeg;
    }
  
  return end;
}


  template<class Itor, 
	   class BinaryPredicate = Aleph::equal_to<typename Itor::value_type>
	   > 
  inline
Itor adjacent_find (Itor beg, const Itor & end, 
		    BinaryPredicate op = BinaryPredicate() )
{
  Itor next (beg);
  ++next;

  while (next != end)
    {
      if (op (*beg, *next))
	return beg;

      ++beg;
      ++next;
    }

  return end;
}


 template <class Itor1, class Itor2, 
	   class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type> >
  inline
bool equal(Itor1 beg, const Itor1 & end,
	   Itor2 cmpBeg, BinaryPredicate op = BinaryPredicate())
{
  while (beg != end)
    {
      if (not op (*beg, *cmpBeg))
	return false;

      ++beg;
      ++cmpBeg;
    }

  return true;
}


  template <class Itor1, class Itor2, 
	    class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>
	    > inline
std::pair<Itor1, Itor2> mismatch (Itor1 beg, const Itor1& end, 
				  Itor2 cmpBeg, 
				  BinaryPredicate op = BinaryPredicate())
{
  while (beg != end and op (*beg, *cmpBeg))
    {
      ++beg;
      ++cmpBeg;
    }
  
  return std::make_pair (beg, cmpBeg);
}

		
/** 
  @brief Returns true if the range [beg1, end1) is less lexicographically
  the range [beg2, End2) as the standard of comparison op.

	@ingroup Algos
*/
  template <class Itor1, class Itor2, 
	    class Comp = Aleph::equal_to<typename Itor1::value_type>
	    > inline
bool lexicographical_compare (Itor1 beg1, const Itor1 & end1, 
			      Itor2 beg2, const Itor2 & end2, 
			      Comp op = Comp())
{
  while (beg1 != end1 and beg2 != end2) 
    {
      if (op (*beg1, *beg2))
	return true;
      else if (op (*beg2, *beg1))
	return false;
	
      ++beg1;
      ++beg2;
    }

  return beg1 == end1 and beg2 != end2;
}


    /* Modifying Algorithms */

    template <class Itor1, class Itor2> inline
Itor2 copy (Itor1 sourceBeg, const Itor1& sourceEnd, Itor2 destBeg)
{
  while (sourceBeg != sourceEnd)
    *destBeg++ = *sourceBeg++;

  return destBeg;
}


    template <class Itor1, class Itor2> inline
Itor2 copy_backward (const Itor1 & sourceBeg, Itor1 sourceEnd, Itor2 destEnd)
{
  
  while (sourceBeg != sourceEnd)
    *--destEnd = *--sourceEnd;

  return destEnd;
}


    template <class Itor1, class Itor2, class UnaryFunc> inline
Itor2 transform (Itor1 sourceBeg, Itor1 sourceEnd, 
		 Itor2 destBeg, UnaryFunc op)
{
  while (sourceBeg != sourceEnd)
    *destBeg++ = op (*sourceBeg++);
  
  return destBeg;
}


    template <class Itor1, class Itor2, class Itor3, class UnaryFunc> inline
Itor3 transform (Itor1 source1Beg, Itor1 source1End, 
		 Itor2 source2Beg, 
		 Itor3 destBeg, 
		 UnaryFunc op)
{
  while (source1Beg != source1End)
    *destBeg++ = op (*source1Beg++, *source2Beg++);

  return destBeg;
}

    template <class Itor1, class Itor2> inline
Itor2 swap_ranges (Itor1 beg1, const Itor1& end1, Itor2 beg2)
{
  while (beg1 != end1)
    std::swap (*beg1++, *beg2++);

  return beg2;
}


    template <class Itor, class T> inline
void fill (Itor beg, const Itor& end, const T & value)
{
  while (beg != end)
    *beg++ = value;
}


    template <class Itor, class T, class Size> inline
void fill_n (Itor beg, Size num, const T & value)
{
  while (num-- > 0)
    *beg++ = value;
}	


    template <class Itor, class Func> inline
void generate (Itor beg, const Itor& end, Func op)
{
  while (beg != end)
    *beg++ = op ();
}


    template <class Itor, class Size, class Func> inline
void generate_n (Itor beg, Size num, Func op)
{
  while (num-- > 0)
    beg++ = op ();
}


    template <class Itor, class UnaryPredicate, class T> inline
void replace_if (Itor beg, const Itor& end, 
		 UnaryPredicate op, 
		 const T & value)
{
  while (beg != end)
    {
      if (op (*beg))
	*beg = value;

      ++beg;
    }
}


    template <class Itor, class T> inline
void replace (Itor beg, const Itor& end, 
	      const T & old_value, 
	      const T & new_value)
{
  Aleph::replace_if (beg, end, 
		     Aleph::bind2nd(equal_to<T>(), old_value), new_value);
}


    template <class Itor1, class Itor2, class UnaryPredicate, class T> inline
Itor2 replace_copy_if (Itor1 sourceBeg, const Itor1& sourceEnd, 
		       Itor2 destBeg, 
		       UnaryPredicate op, 
		       const T & value)
{
  while (sourceBeg != sourceEnd)
    {
      if (op (*sourceBeg))
	*sourceBeg = value;

      *destBeg++ = *sourceBeg++;
    }

  return destBeg;
}


    template <class Itor1, class Itor2, class T> inline
Itor2 replace_copy (const Itor1& sourceBeg, const Itor1& sourceEnd, 
		    const Itor2& destBeg, 
		    const T & old_value, 
		    const T & new_value)
{
  return Aleph::replace_copy_if 
    (sourceBeg, sourceEnd, destBeg, 
     Aleph::bind2nd(equal_to<T>(), old_value), new_value);
}


// Copyright (C) 2001, 2002, 2003, 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */


  template<class In_Itor, class Out_Itor, 
	   class Predicate = Aleph::equal_to<typename Out_Itor::value_type>
	   > inline
Out_Itor remove_copy_if(In_Itor __first, const In_Itor & __last,
			Out_Itor __result, 
			Predicate __pred)
{
  for ( ; __first != __last; ++__first)
    if (not __pred(*__first))
      {
	*__result = *__first;
	++__result;
      }

  return __result;
}


    template<class Fw_Itor, class Predicate> inline
Fw_Itor remove_if(Fw_Itor __first, const Fw_Itor & __last,
		  Predicate __pred)
{
  __first = Aleph::find_if(__first, __last, __pred);

  Fw_Itor __i = __first;

  if (__first == __last) 
    return __first;
  else
    return Aleph::remove_copy_if(++__i, __last, __first, __pred);
}


    template<class Fw_Itor, class T> inline
Fw_Itor remove(Fw_Itor __first, const Fw_Itor & __last, 
	       const T & __value)
{
  __first = Aleph::find(__first, __last, __value);

  Fw_Itor __i = __first;

  if (__first == __last) 
    return __first;
  else
    return Aleph::remove_copy_if(++__i, __last, __first, __value);
}


 template<class In_Itor, class Out_Itor, 
	  class BinaryPredicate = Aleph::equal_to<typename In_Itor::value_type>
	  > static inline
Out_Itor __unique_copy(In_Itor __first, In_Itor __last,
		       Out_Itor __result,
		       BinaryPredicate __binary_pred)
{
  typename In_Itor::value_type __value = *__first;

  *__result = __value;
  
  while (++__first != __last)
    if (not __binary_pred(__value, *__first))
      {
	__value = *__first;
	*++__result = __value;
      }

  
  return ++__result;
}


    template<class In_Itor, class Out_Itor> inline
Out_Itor unique_copy(In_Itor __first, In_Itor __last,
		     Out_Itor __result)
{
  if (__first == __last) 
    return __result;
   
  return Aleph::__unique_copy(__first, __last, __result);
}


  template <class In_Itor, class Out_Itor, class BinaryPredicate> inline
Out_Itor unique_copy(In_Itor __first, In_Itor __last,
		     Out_Itor __result,
		     BinaryPredicate __binary_pred)
{
  return Aleph::__unique_copy(__first, __last, __result, __binary_pred);
}


  template<class Itor, 
	   class BinaryPredicate = Aleph::equal_to<typename Itor::value_type>
	   > inline
Itor unique(Itor __first, Itor __last, BinaryPredicate __binary_pred)
{
    /* Skip the beginning, if already unique. */
  __first = Aleph::adjacent_find(__first, __last, __binary_pred);

  if (__first == __last)
    return __last;

  /* Do the real copy work. */
  Itor __dest = __first;
  ++__first;

  while (++__first != __last)
    if (not __binary_pred(*__dest, *__first))
      *++__dest = *__first;
  
  return ++__dest;
}


    /* MUTATING ALGORITHMS */


    template <class Itor> inline
void reverse(Itor beg, Itor end)
{
  while (beg != end)
    std::swap(*beg++, *--end);
}


    template <class Itor1, class Itor2> inline
Itor2 reverse_copy(Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  while (sourceBeg != sourceEnd)
    {
      --sourceEnd;
      *destBeg = *sourceEnd;
      ++destBeg;
    }
	
  return destBeg;
}


    template <class Itor>
void rotate(Itor beg, Itor pos, Itor end)
{
  Aleph::reverse(beg, pos);
  Aleph::reverse(pos, end);
  Aleph::reverse(beg, end);
}


    template <class Itor1, class Itor2> inline
Itor2 rotate_copy (const Itor1& beg, const Itor1& pos, const Itor1& end, 
		   const Itor2& tgt_beg)
{
  return Aleph::copy(beg, pos, Aleph::copy(pos, end, tgt_beg));
}

# ifdef nada

    template<typename Itor> inline
bool next_permutation(Itor first, Itor last)
{
  if (first == last)
    return false;

  Itor __i = __first;
  ++__i;
   
  if (__i == __last)
    return false;
    
  __i = __last;
  --__i;

  while (true)
    {
      Itor __ii = __i;
      --__i;
	
      if (*__i < *__ii)
	{
	  Itor __j = __last;
	  
	  while (not (*__i < *--__j)) 
	    {}
	      
	  std::iter_swap(__i, __j);
	  std::reverse(__ii, __last);
	  
	  return true;
	}
	
      if (__i == __first)
	{
	  std::reverse(__first, __last);
	  return false;
	}
    }
}


    template<class Itor, class Compare> inline
bool next_permutation(Itor first, Itor last, Compare comp)
{
  if (first == last)
    return false;

  Itor i = first;
  ++i;

  if (i == last)
    return false;

  i = last;
  --i;

  while (true)
    {
      Itor ii = i;
      --i;

      if (comp(*i, *ii))
	{
	  Itor j = last;

	  while (not comp(*i, *--j)) { /* Empty */ }
	      
	  std::iter_swap(i, j);
	  Aleph::reverse(ii, last);

	  return true;
	}
      
      if (i == first)
	{
	  Aleph::reverse(first, last);

	  return false;
	}
    }
}

/**
  @brief Permute range into the previous "dictionary" ordering.

  @details Treats all permutations of the range as a set of 
  "dictionary" sorted sequences.
  Permutes the current sequence into the previous one of this set.
  Returns true if there are more sequences to generate.  
  If the sequence is the smallest of the set, 
  the largest is generated and false returned.
  
  @param first Start of range.
  @param last End of range.
  
  @return false if wrapped to last permutation, true otherwise.
*/
  template<typename Itor>
    bool
    prev_permutation(Itor first,
		     Itor last)
    {
    /* Concept requirements */
      glibcxx_function_requires(ItorConcept<
				  Itor>)
      glibcxx_function_requires(_LessThanComparableConcept<
	    typename iterator_traits<Itor>::value_type>)
      glibcxx_requires_valid_range(first, last);

      if (first == last)
	return false;
      Itor i = first;
      ++i;
      if (i == last)
	return false;
      i = last;
      --i;

      for(;;)
	{
	  Itor ii = i;
	  --i;
	  if (*ii < *i)
	    {
	      Itor j = last;
	      while (!(*--j < *i))
		{}
	      std::iter_swap(i, j);
	      std::reverse(ii, last);
	      return true;
	    }
	  if (i == first)
	    {
	      std::reverse(first, last);
	      return false;
	    }
	}
    }

/**
  @brief Permute range into the previous "dictionary" ordering using
  comparison functor.

  @details Treats all permutations of the range [first,last) as a set of
  "dictionary" sorted sequences ordered by @a comp.  
  Permutes the current sequence into the previous one of this set.  
  Returns true if there are more sequences to generate.  
  If the sequence is the smallest of the set, 
  the largest is generated and false returned.
  
  @param first Start of range.
  @param last End of range.
  @param comp
  
  @return false if wrapped to last permutation, true otherwise.
*/
  template<typename Itor, typename _Compare>
    bool
    prev_permutation(Itor first,
		     Itor last, _Compare comp)
    {
    /* Concept requirements */
      glibcxx_function_requires(ItorConcept<Itor>)
      glibcxx_function_requires(_BinaryPredicateConcept<_Compare,
	    typename iterator_traits<Itor>::value_type,
	    typename iterator_traits<Itor>::value_type>)
      glibcxx_requires_valid_range(first, last);

      if (first == last)
	return false;
      Itor i = first;
      ++i;
      if (i == last)
	return false;
      i = last;
      --i;

      for(;;)
	{
	  Itor ii = i;
	  --i;
	  if (comp(*ii, *i))
	    {
	      Itor j = last;
	      while (!comp(*--j, *i))
		{}
	      std::iter_swap(i, j);
	      std::reverse(ii, last);
	      return true;
	    }
	  if (i == first)
	    {
	      std::reverse(first, last);
	      return false;
	    }
	}
    }

 


template <class Itor>
bool next_permutation (Itor beg, Itor end)
{
  ERROR();
  return true;
}



template <class Itor>
bool prev_permutation (Itor beg, Itor end)
{

}


template<class Itor>
void random_shuffle (Itor beg, Itor end)
{

}


template<class Itor, class RandomFunc>
void random_shuffle (Itor beg, Itor end, RandomFunc& op)
{
  if (beg == end) 
    return;
  		
  Itor iter(beg);
	
  while(iter != end)
    {   
      iter_swap(iter, beg + op((i - beg) + 1));
      ++iter;
    }

}


template<class Itor, class UnaryPredicate>
Itor partition (Itor beg, Itor end, UnaryPredicate op)
{

}


template<class Itor, class UnaryPredicate>
Itor stable_partition (Itor beg, Itor end, UnaryPredicate op)
{

}



    /* SORTING ALGORITHMS */


template<class Itor>
void sort (Itor beg, Itor end)
{
 
}


template<class Itor, class BinaryPredicate>
void sort (Itor beg, Itor end, BinaryPredicate op)
{

}


template<class Itor>
void stable_sort (Itor beg, Itor end)
{

}


template<class Itor, class BinaryPredicate>
void stable_sort (Itor beg, Itor end, BinaryPredicate op)
{

}


template <class Itor>
void partial_sort (Itor beg, Itor sortEnd, Itor end)
{

}


template <class Itor, class BinaryPredicate>
void partial_sort (Itor beg, Itor sortEnd, Itor end, BinaryPredicate op)
{

}


template <class Itor1, class Itor2>
Itor2 partial_sort_copy (Itor1 sourceBeg, Itor2 sourceEnd, Itor2 destBeg, Itor2 destEnd)
{

}


template <class Itor1, class Itor2, class BinaryPredicate>
Itor2 partial_sort_copy (Itor1 sourceBeg, Itor2 sourceEnd, Itor2 destBeg, Itor2 destEnd, BinaryPredicate op)
{

}


template <class Itor>
void nth_element (Itor beg, Itor nth, Itor end)
{

}


template <class Itor, class BinaryPredicate>
void nth_element (Itor beg, Itor nth, Itor end, BinaryPredicate op)
{

}


template  <class Itor>
void make_heap (Itor beg, Itor end)
{

}


template  <class Itor, class BinaryPredicate>
void make_heap (Itor beg, Itor end, BinaryPredicate op)
{

}


template <class Itor>
void push_heap (Itor beg, Itor end)
{

}


template <class Itor, class BinaryPredicate>
void push_heap (Itor beg, Itor end, BinaryPredicate op)
{

}


template <class Itor>
void pop_heap (Itor beg, Itor end)
{

}


template <class Itor, class BinaryPredicate>
void pop_heap (Itor beg, Itor end, BinaryPredicate op)
{

}


template <class Itor>
void sort_heap (Itor beg, Itor end)
{

}


template <class Itor, class BinaryPredicate>
void sort_heap (Itor beg, Itor end, BinaryPredicate op)
{

}


    /* SORTED RANGE ALGORITHMS */


template<class Itor, class T>
bool binary_search(Itor beg, Itor end, const T& value)
{
  Itor i(lower_bound(beg, end, value));
  return i != last and not(value < *i);
}


template<class Itor, class T, class BinaryPredicate>
bool binary_search(Itor beg, Itor end, const T& value, BinaryPredicate op)
{
  Itor i = lower_bound(beg, end, value, op);
  return i != last and not(comp(value, *i));
}


template <class Itor1, class Itor2>
bool includes (Itor1 beg, Itor1 end, Itor2 searchBeg, Itor2 searchEnd)
{
  while (beg != end and searchBeg != searchEnd)
    {
      if (*searchBeg > *beg)
	++beg;
      
      else
	{		
	  if (*searchBeg == *beg)
	    {
	      ++searchBeg;
	      
	      if (*searchBeg > *beg)
		++beg;
	    }
	  
	  else
	    return false;
	}
    }
  
  return true;
}


template <class Itor1, class Itor2, class BinaryPredicate>
bool includes (Itor1 beg, Itor1 end, Itor2 searchBeg, Itor2 searchEnd, BinaryPredicate op)
{
	
}


template <class Itor, class T>
Itor lower_bound (Itor beg, Itor end, const T& value)
{
  while (beg != end and *beg < value)
    ++beg;

  return beg;
}


template <class Itor, class T, class BinaryPredicate>
Itor lower_bound (Itor beg, Itor end, const T& value, BinaryPredicate op)
{
  while (beg != end and op(*beg,value))
    ++beg;

  return beg;
}


template <class Itor, class T>
Itor upper_bound (Itor beg, Itor end, const T& value)
{
  while (beg != end and *beg <= value)
    ++beg;

  return beg;
}


template <class Itor, class T, class BinaryPredicate>
Itor upper_bound (Itor beg, Itor end, const T& value, BinaryPredicate op)
{
  while (beg != end and (op(*beg,value) or *beg == value))
    ++beg;

  return beg;
}


template <class Itor, class T>
pair<Itor, Itor> equal_range (Itor beg, Itor end, const T& value)
{
  pair<Itor,Itor> _pair;
  Itor _first = lower_bound(beg, end, value);
  Itor _second = upper_bound(beg, end, value);
  _pair.first = _first;
  _pair.second = _second;


  return _pair;
}


template <class Itor, class T, class BinaryPredicate>
pair<Itor, Itor> equal_range (Itor beg, Itor end, const T& value, BinaryPredicate op)
{
  return make_pair(lower_bound(beg, end, value, op), upper_bound(beg, end, value, op));
}


template <class Itor1, class Itor2, class Itor3>
Itor3 merge(Itor1 source1Beg, Itor1 source1End, Itor2 source2Beg, Itor2 source2End, Itor3 destBeg) 
{
  while (source1Beg != source1End and source2Beg != source2End) 
    {
      if (*source2Beg < *source1Beg) 
	{
	  *destBeg = *source2Beg;
	  ++source2Beg;
	}
      else 
	{
	  *destBeg = *source1Beg;
	  ++source1Beg;
	}
      ++destBeg;
    }
  return copy(source2Beg, source2End, copy(source1Beg, source1End, destBeg));
}


template <class Itor1, class Itor2, class Itor3, class BinaryPredicate>
Itor3 merge(Itor1 source1Beg, Itor1 source1End, Itor2 source2Beg, Itor2 source2End, Itor3 destBeg, BinaryPredicate op) 
{
  while (source1Beg != source1End && source2Beg != source2End) 
    {
      if (op(*source2Beg, *source1Beg)) 
	{
	  *destBeg = *source2Beg;
	  ++source2Beg;
	}
      else 
	{
	  *destBeg = *source1Beg;
	  ++source1Beg;
	}
      ++destBeg;
    }
  return copy(source2Beg, source2End, copy(source1Beg, source1End, destBeg));
}


template <class Itor1, class Itor2, class Itor3>
Itor3 set_union (Itor1 source1Beg, Itor1 source1End, Itor2 source2Beg, Itor2 source2End, Itor3 destBeg)
{

}


template <class Itor1, class Itor2, class Itor3, class BinaryPredicate>
Itor3 set_union (Itor1 source1Beg, Itor1 source1End, Itor2 source2Beg, Itor2 source2End, Itor3 destBeg,
		 BinaryPredicate op)
{

}


template <class Itor1, class Itor2, class Itor3>
Itor3 set_intersection (Itor1 source1Beg, Itor1 source1End, Itor2 source2Beg, Itor2 source2End, Itor3 destBeg)
{

}


template <class Itor1, class Itor2, class Itor3, class BinaryPredicate>
Itor3 set_intersection (Itor1 source1Beg, Itor1 source1End, 
			Itor2 source2Beg, Itor2 source2End, Itor3 destBeg,
			BinaryPredicate op)
{

}


template <class Itor1, class Itor2, class Itor3>
Itor3 set_difference (Itor1 source1Beg, Itor1 source1End,
		      Itor2 source2Beg, Itor2 source2End, Itor3 destBeg)
{

}


template <class Itor1, class Itor2, class Itor3, class BinaryPredicate>
Itor3 set_difference (Itor1 source1Beg, Itor1 source1End,
		      Itor2 source2Beg, Itor2 source2End, Itor3 destBeg,
		      BinaryPredicate op)
{

}


template <class Itor1, class Itor2, class Itor3>
Itor3 set_symmetric_difference (Itor1 source1Beg, Itor1 source1End,
				Itor2 source2Beg, Itor2 source2End, Itor3 destBeg)
{

}


template <class Itor1, class Itor2, class Itor3, class BinaryPredicate>
Itor3 set_symmetric_difference (Itor1 source1Beg, Itor1 source1End,
				Itor2 source2Beg, Itor2 source2End, Itor3 destBeg,
				BinaryPredicate op)
{

}


template <class Itor>
void inplace_merge (Itor beg1, Itor end1beg2, Itor end2)
{

}


template <class Itor, class BinaryPredicate>
void inplace_merge (Itor beg1, Itor end1beg2, Itor end2, BinaryPredicate op)
{

}


    /* Numerical Algorithms*/


template <class Itor, class T>
T accumulate (Itor beg, Itor end, T initValue)
{
  for( /* Nothing */ ; beg != end; ++beg)
    initValue = initValue + *beg;

  return initValue;
}


template <class Itor, class T, class BinaryFunc>
T accumulate (Itor beg, Itor end, T initValue, BinaryFunc op)
{
  for( /* Nothing */ ; beg != end; ++beg)
    initValue = op(initValue, *beg);

  return initValue;
}


template <class Itor1, class Itor2, class T>
T inner_product (Itor1 beg1, Itor1 end1, Itor2 beg2, T initValue)
{
  for (/*Nothing*/; beg1 != end1; ++beg1, ++beg2)
    initValue = initValue + (*beg1 * *beg2);

  return initValue;
}


template <class Itor1, class Itor2, class T, class BinaryFunc>
T inner_product (Itor1 beg1, Itor1 end1, Itor2 beg2, T initValue, BinaryFunc op1, BinaryFunc op2)
{
  for (/*Nothing*/; beg1 != end1; ++beg1, ++beg2)
    initValue = op1(initValue, op2(*beg1, *beg2));

  return initValue;
}


template <class Itor1, class Itor2, class T>
Itor2 partial_sum (Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  T sum = *sourceBeg;
  sum = 0;

  for (/*Nothing*/; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      sum = sum + *sourceBeg;
      *destBeg = sum;
    }

  return destBeg;
}


template <class Itor1, class Itor2, class BinaryFunc, class T>
Itor2 partial_sum (Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg, BinaryFunc op)
{
  T sum = *sourceBeg;
  sum = 0;

  for (unsigned int i = 0; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg, i++)
    {
      if (i = 0)
	sum = *sourceBeg;
		
      else
	sum = op(sum,*sourceBeg);
		
      *destBeg = sum;
    }

  return destBeg;
}	


template <class Itor1, class Itor2, class T>
Itor2 adjacent_difference (Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  T diff = *sourceBeg;
  diff = 0;

  for (/*Nothing*/; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      *destBeg = *sourceBeg - diff;
      diff = *sourceBeg;
    }

  return destBeg;
}


template <class Itor1, class Itor2, class BinaryFunc, class T>
Itor2 adjacent_difference (Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg, BinaryFunc op)
{
  T diff = *sourceBeg;
  diff = 0;

  for (unsigned int i = 0; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg, i++)
    {
      if (i = 0)
	*destBeg = *sourceBeg;
		
      else
	*destBeg = op(*sourceBeg, diff);
		
      diff = *sourceBeg;
    }

  return destBeg;
}

# endif //nada

} /* End namespace Aleph */

# endif // AHALGO_H
