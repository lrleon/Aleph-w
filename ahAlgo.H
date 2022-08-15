
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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

/*
	   Aleph implementation of C++ standard algorithms
*/

# ifndef AHALGO_H
# define AHALGO_H

# include <ahAssert.H>
# include <ahUtils.H>

typedef size_t size_type;

// Non Modifying Algorithms


namespace Aleph
{

  /** Ejecuta la operación <code>op</code> sobre cada elemento del
      contenedor donde esté apuntando los iterador <code>beg</code> y
      <code>end</code>. 

      @param[in] beg iterador al primer elemento del contenedor desde
      donde se desea iniciar la búsqueda. 
      @param[in] end iterador al último elemento del contenedor.
      @param[in] op operación a efectuar sobre cada elemento.
      @return una copia de la operación op

     @ingroup Algos
   */
    template <class Itor, class Operation> inline
Operation for_each (Itor beg, const Itor & end, Operation op)
{
  while (beg != end)
    op (*beg++);
	
  return op;
}

  /** Cuenta las ocurrencias de los elementos cuya operación
      <code>op</code> dé cierto.

      @param[in] beg iterador al primer elemento del contenedor.
      @param[in] end iterador al último elemento del contenedor.
      @param[in] op operación a efectuar sobre cada elemento.
      @return la cantidad de elementos para los cuales la operación
      retornó cierto.

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

 /** Cuenta las ocurrencias de los elementos con valor igual a
    <code>value</code>. 

    @param[in] beg iterador al primer elemento del contenedor desde
    donde se desea iniciar la búsqueda. 
    @param[in] end iterador al último elemento del contenedor.
    @param[in] value valor a comparar
    @return la cantidad de elementos iguales a <code>value</code>

    @ingroup Algos
 */
    template <class Itor, class T> inline
typename Itor::difference_type count(const Itor & beg, const Itor & end, 
				     const T & value)
{
  return Aleph::count_if (beg, end, Aleph::bind2nd(equal_to<T>(), value));
}

  /** Retorna el menor elemento del contenedor que involucra a los
      iteradores. 

     @param[in] beg iterador apuntando al primer elemento desde donde se
     desea iniciar la búsqueda. 
     @param[in] end iterador al último elemento del contenedor.
     @param[in] op clase de comparación
     @return un iterador posicionado sobre el menor elemento.

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

/** Retorna el mayor elemento del contenedor que involucra a los
      iteradores. 

     @param[in] beg iterador apuntando al primer elemento  desde donde
     se desea iniciar la búsqueda. 
     @param[in] end iterador al último elemento del contenedor.
     @param[in] op clase de comparación
     @return un iterador posicionado sobre el mayor elemento.

     @ingroup Algos
   */
    template<class Itor, 
	     class CompFunc =  Aleph::greater<typename Itor::value_type> > 
    inline
Itor max_element(const Itor& beg, const Itor& end, CompFunc op = CompFunc())
{
  return Aleph::min_element (beg, end, op);
}


  /** Encuentra el primer elemento de un contenedor que satisfaga en
      predicado <code>op</code>.

    @param[in] beg iterador apuntando al primer elemento  desde donde se
    desea iniciar la búsqueda. 
    @param[in] end iterador al último elemento del contenedor.
    @param[in] op clase que determina si un elemento satisface o no el
    predicado. 
    @return iterador apuntando al miembro encontrado (si fue el caso)
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

 /** Encuentra el primer elemento de un contenedor cuyo valor sea igual
      a <code>value</code>.

    @param[in] beg iterador apuntando al primer elemento desde donde se
    desea iniciar la búsqueda. 
    @param[in] end iterador al último elemento del contenedor.
    @param[in] value valor a encontrar.
    predicado. 
    @return iterador apuntando al miembro encontrado (si fue el caso)
    @ingroup Algos
  */	
    template<class Itor, class T> inline
Itor find(const Itor & beg, const Itor & end, const T & value)
{
  return 
    Aleph::find_if (beg, end, Aleph::bind2nd(Aleph::equal_to<T>(), value));
}

  /** Retorna la posición del primero de los <code>count</code> elementos
      consecutivos que se encuentre en el rango <code>beg</code> y
      <code>end</code> para el cual un predicado es cierto.

      El predicado es invocado como:
      \code
      op () (item, value)
      \endcode

      @param[in] beg iterador apuntando al primer elemento del
      contenedor desde donde se desea iniciar la búsqueda.
      @param[in] end iterador al último elemento del contenedor.
      @param[in] count cantidad de ocurrencias consecutivas que se desea
      buscar. 
      @param[in] value valor específico del elemento a buscar.
      @param[in] op criterio de comparación
      @return un iterador posicionado en el primer elemento encontrado o
      end si no existe <code>count</code> ocurrencias.
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

 
  /** Búsqueda de subrangos.

      Esta función busca entre el rango dado <code>beg</code> y
      <code>end</code> la primera ocurrencia exacta del subrango
      <code>searchBeg</code> y <code>searchEnd</code>.

      De encontrarse una ocurrencia del subrango, la rutina retorna un
      iterador posicionado en el primer elemento de la primera
      ocurrencia del subrango. De lo contrario, el iterador de retorno
      es <code>end</code>.

      Para comparar los elementos entre contenedores, la rutina invoca a
      la clase predicado <code>op</code> de la siguiente manera
      \code
      op(*beg, *pivot)
      \endcode
      Donde <code>beg </code> es la posición actual del rango de
      búsqueda y <code>pivot</code> es la posición actual en el
      subrango.  

      Nótese que el rango en el cual se busca y el subrango a buscar no
      sólo pueden referir a contenedores diferentes, sino, también de
      tipos distintos. Por ejemplo, <code>[beg, end)</code> podría set a
      un contenedor de tipo <code>set</code>, mientras que 
      <code>[searchBeg, searchEnd)</code> a un contenedor de tipo
      <code>vector</code>.

      @param[in] beg iterador apuntando al primer elemento del
      contenedor desde donde se desea iniciar la búsqueda.
      @param[in] end iterador al último elemento del contenedor.
      @param[in] searchBeg iterador apuntando al primer elemento del 
      subrango a buscar.
      @param[in] searchEnd iterador al último elemento del subrango a
      buscar. 
      @param[in] op operación de comparación entre elementos de un
      rango.
      @return si el subrango es encontrado, iterador posicionado sobre
      el primer elemento de la primera ocurrencia del subrango. De lo
      contrario <code>end</code>.

      @note Muy importante a notar que a través de op se puede
      instrumentar contenedores distintos, de clase distinta y con
      elementos de diferentes tipos.

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

		
    /** Retorna true si el rango [beg1,end1) es menor lexicográficamente
	que el rango [beg2,end2) según el criterio de comparación op.

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


// Modifying Algorithms

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
  // Skip the beginning, if already unique.
  __first = Aleph::adjacent_find(__first, __last, __binary_pred);

  if (__first == __last)
    return __last;

  // Do the real copy work.
  Itor __dest = __first;
  ++__first;

  while (++__first != __last)
    if (not __binary_pred(*__dest, *__first))
      *++__dest = *__first;

  return ++__dest;
}


// MUTATING ALGORITHMS


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

	  while (not comp(*i, *--j)) { /* empty */ }
	      
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
   *  @brief  Permute range into the previous "dictionary" ordering.
   *  @param  first  Start of range.
   *  @param  last   End of range.
   *  @return  False if wrapped to last permutation, true otherwise.
   *
   *  Treats all permutations of the range as a set of "dictionary" sorted
   *  sequences.  Permutes the current sequence into the previous one of this
   *  set.  Returns true if there are more sequences to generate.  If the
   *  sequence is the smallest of the set, the largest is generated and false
   *  returned.
  */
  template<typename Itor>
    bool
    prev_permutation(Itor first,
		     Itor last)
    {
      // concept requirements
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
   *  @brief  Permute range into the previous "dictionary" ordering using
   *  comparison functor.
   *  @param  first  Start of range.
   *  @param  last   End of range.
   *  @param  comp
   *  @return  False if wrapped to last permutation, true otherwise.
   *
   *  Treats all permutations of the range [first,last) as a set of
   *  "dictionary" sorted sequences ordered by @a comp.  Permutes the current
   *  sequence into the previous one of this set.  Returns true if there are
   *  more sequences to generate.  If the sequence is the smallest of the set,
   *  the largest is generated and false returned.
  */
  template<typename Itor, typename _Compare>
    bool
    prev_permutation(Itor first,
		     Itor last, _Compare comp)
    {
      // concept requirements
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


// SORTING ALGORITHMS

// OJO

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


// SORTED RANGE ALGORITHMS


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


// ALGORITMOS NUMERICOS


template <class Itor, class T>
T accumulate (Itor beg, Itor end, T initValue)
{
  for( /* nothing */ ; beg != end; ++beg)
    initValue = initValue + *beg;

  return initValue;
}


template <class Itor, class T, class BinaryFunc>
T accumulate (Itor beg, Itor end, T initValue, BinaryFunc op)
{
  for( /* nothing */ ; beg != end; ++beg)
    initValue = op(initValue, *beg);

  return initValue;
}


template <class Itor1, class Itor2, class T>
T inner_product (Itor1 beg1, Itor1 end1, Itor2 beg2, T initValue)
{
  for (/*nothing*/; beg1 != end1; ++beg1, ++beg2)
    initValue = initValue + (*beg1 * *beg2);

  return initValue;
}


template <class Itor1, class Itor2, class T, class BinaryFunc>
T inner_product (Itor1 beg1, Itor1 end1, Itor2 beg2, T initValue, BinaryFunc op1, BinaryFunc op2)
{
  for (/*nothing*/; beg1 != end1; ++beg1, ++beg2)
    initValue = op1(initValue, op2(*beg1, *beg2));

  return initValue;
}


template <class Itor1, class Itor2, class T>
Itor2 partial_sum (Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  T sum = *sourceBeg;
  sum = 0;

  for (/*nothing*/; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
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

  for (/*nothing*/; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
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

} // end namespace Aleph

# endif // AHALGO_H
