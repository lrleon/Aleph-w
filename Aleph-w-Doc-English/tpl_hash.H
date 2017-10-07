
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
  @file tpl_hash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_HASH_H
# define TPL_HASH_H

# include <initializer_list>
# include <tpl_dynSetHash.H>
# include <tpl_dynSetOhash.H>

namespace Aleph 
{

    /* TODO: Documentar para que se usa esta clase. */
template <typename Key,
	  template <typename, class> class HashSetTable = ODhashTable,
	  class Cmp                                     = Aleph::equal_to<Key>>
struct HashSet : public HashSetTable<Key, Cmp>
{
  typedef HashSetTable<Key, Cmp> Base;
  using Base::Base;

  void add (std::initializer_list<Key> l)
  {
    for (auto it = l.begin(); it != l.end(); it++)
      this->insert(*it);
  }

  HashSet(std::initializer_list<Key> l) : Base() { add(l); }

  HashSet() : Base() {}
};

    /* TODO: Documentar para que se usa esta clase. */
template 
<typename Key, typename Data,
 template <typename, typename, class> class HashMapTable = ODhashTable,
 class Cmp = Aleph::Dft_Pair_Cmp<Key, Data>>
struct HashMap : public HashMapTable<Key, Data, Cmp>
{
  typedef HashMapTable<Key, Data, Cmp> Base;
  using Base::Base;

  void add (std::initializer_list<Key> lk,
	    std::initializer_list<Data> ld)
  {
    if (lk.size() != ld.size())
      throw std::range_error("size mismatch between domain and range");

    auto itk = lk.begin();
    auto itd = ld.begin();
    for (; itk != lk.end(); itk++, itd++)
      this->insert(*itk, *itd);
  }

  HashMap() : Base() {}

  HashMap(std::initializer_list<Key> lk,
	  std::initializer_list<Data> ld) 
    : Base()
  {
    add(lk, ld);
  }

  template <template <typename T> class Container = DynList>
  HashMap(const Container<Key> & c, std::initializer_list<Data> ld) : Base()
  {
    if (c.size() != ld.size())
      throw std::range_error("size mismatch between domain and range");

    auto itd = ld.begin();
    c.for_each(/* Lambda */ [this, &itd] (const Key & key)
	       {
		 set_entry(key, *itd++); 
	       });
  }

  Generate_Proxy_Operator(HashMap);
};




} /* End namespace Aleph */


# endif //  TPL_HASH_H
