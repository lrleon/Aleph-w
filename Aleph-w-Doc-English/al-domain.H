
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
  @file al-domain.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AL_DOMAIN_H
# define AL_DOMAIN_H

# include <tpl_hash.H>

namespace Aleph
{

  template <typename T = int>
struct AlDomain : public Aleph::HashSet<T, DynSetODhash>
{
  typedef Aleph::HashSet<T, DynSetODhash> Base;

  using Base::Base;

  template <template <typename> class Container = DynList>
  Container<T> to_list() const
  {
    Container<T> ret_val;
    this->for_each(/* Lambda */ [&ret_val] (const T & item)
	     {
	       ret_val.append(item);
	     });
    return ret_val;    
  }

  std::string to_str() const
  {
    return sort(to_list()).template foldl<std::string> 
      ("", /* Lambda */ [] (const std::string & s, const T & item)
       {
	 return s + " " + Aleph::to_str(item);
       });
  }
};

template <typename T> inline
ostream & operator << (ostream & s, const AlDomain<T> & dom)
{
  return s << dom.to_str();
}

struct IntRange : public AlDomain<int>
{
  IntRange() = delete; 

  IntRange(int start, int end,  int step = 1)
  {
    if (step < 0)
      throw std::domain_error("negative step");

    cout << "step = " << step << endl;
    for (long i = start; i <= end; i += step)
      insert(i);
  }

  IntRange(size_t n)
  {
    for (int i = 0; i < n; ++i)
      insert(i);
  }

};

} /* End namespace Aleph */

# endif // AL_DOMAIN_H
