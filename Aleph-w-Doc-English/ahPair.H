
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
  @file ahPair.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AHPAIR_H
# define AHPAIR_H 1

namespace Aleph
{
      template<class _T1, class _T2>
  struct pair
  {
    typedef _T1 first_type;    
    typedef _T2 second_type;   

    _T1 first;                 
    _T2 second;                

    pair() : first(), second() { }

    pair(const _T1 & __a, const _T2 & __b) : first(__a), second(__b) { }

    pair(const _T1& __a) : first(__a) { }

        template<class _U1, class _U2>
    pair(const pair<_U1, _U2> & __p)
      : first(__p.first), second(__p.second) { }

    pair & operator = (const pair & p)
    {
      if (this == &p)
	return *this;

      first  = p.first;
      second = p.second;

      return *this;
    }
  };


      template<class _T1, class _T2> inline 
  bool operator == (const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
  { 
    return __x.first == __y.first and __x.second == __y.second; 
  }


      template<class _T1, class _T2> inline 
  bool operator < (const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
  { 
    return __x.first < __y.first
      or (not (__y.first < __x.first) and __x.second < __y.second); 
  }


      template<class _T1, class _T2> inline 
  bool operator != (const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
  { 
    return not (__x == __y); 
  }


      template<class _T1, class _T2> inline 
  bool operator > (const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
  {
    return __y < __x; 
  }


      template<class _T1, class _T2> inline
  bool operator <= (const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
  { 
    return not (__y < __x); 
  }


      template<class _T1, class _T2> inline
  bool operator >= (const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
  {
    return not (__x < __y); 
  }

  
      template<class _T1, class _T2> inline
  pair<_T1, _T2> make_pair(_T1 __x, _T2 __y) 
  {
    return pair<_T1, _T2>(__x, __y); 
  }


} /* End namespace Aleph */

# endif /* AHPAIR_H */
  