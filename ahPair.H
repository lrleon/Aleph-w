
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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


} // namespace Aleph

# endif /* AHPAIR_H */


