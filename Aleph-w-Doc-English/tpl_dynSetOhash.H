 
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
  @file tpl_dynSetOhash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNSETOHASH_H
# define TPL_DYNSETOHASH_H

# include <tpl_olhash.H>
# include <tpl_odhash.H>

namespace Aleph
{

template <typename Key,
	  class Cmp = Aleph::equal_to<Key>,
	  template <typename, class> class HashTable = ODhashTable>
struct DynSetOpenHash : public HashTable<Key, Cmp>
{
  typedef HashTable<Key, Cmp> Base;
  using Base::Base;
};


  template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct DynSetOLhash : public DynSetOpenHash<Key, Cmp, OLhashTable>
{
  typedef DynSetOpenHash<Key, Cmp, OLhashTable> Base;
  using Base::Base;
};


template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct DynSetODhash : public DynSetOpenHash<Key, Cmp, ODhashTable>
{
  typedef DynSetOpenHash<Key, Cmp, ODhashTable> Base;
  using Base::Base;
};


template <typename Key, typename Data, 
	  class Cmp = Aleph::Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>,
	  template <typename, class> class HashTable = ODhashTable>
struct DynMapOpenHash : public HashTable<std::pair<Key, Data>, Cmp>
{
  typedef std::pair<Key, Data> Pair;
  typedef HashTable<Pair, Cmp> Base;
  using Base::Base;

  static Pair * key_to_pair(Key * ptr) { return (Pair*) ptr; }

  static Pair * data_to_pair(Data * ptr)
  {
    Pair * zero = 0;
    return (Pair*) ((long) ptr - (long) &zero->second); 
  }

  Data & get_data(const Key & key)
  {
    return key_to_pair(&const_cast<Key&>(key))->second;    
  }

  const Key & get_key(Data * data_ptr)
  {
    return data_to_pair(data_ptr)->first;
  }  

  typedef Data Value_Type;

  typedef size_t (*Hash_Fct)(const Key&);

  template <size_t (*fct)(const Key & k)> static
  size_t wrapper(const std::pair<Key, Data> & p)
  {
    return (*fct)(p.first);
  }

    /* It is overloaded because the wrapper to has function */
  DynMapOpenHash
  (typename Base::Hash_Fct hash_fct     = wrapper<Aleph::dft_hash_fct<Key>>,
   typename Base::Hash_Fct snd_hash_fct = wrapper<Aleph::snd_hash_fct<Key>>,
   const size_t len                     = Primes::DefaultPrime,
   const float  lower_alpha             = hash_default_lower_alpha,
   const float  upper_alpha             = hash_default_upper_alpha) 
    : Base(hash_fct, snd_hash_fct, len, lower_alpha, upper_alpha)
  {
    /* Empty */
  }

  Key * insert(const Key & key, const Data & data)
  {
    Pair * p = this->Base::insert(Pair(key, data));
    return p != NULL ? &p->first : NULL;
  }

  Key * insert(const Key & key, Data && data)
  {
    Pair * p = this->Base::insert(Pair(key, std::move(data)));
    return p != NULL ? &p->first : NULL;
  }

  Key * insert(Key && key, Data && data)
  {
    Pair * p = this->Base::insert(Pair(std::move(key), std::move(data)));
    return p != NULL ? &p->first : NULL;
  }

  Key * insert(Key && key, const Data & data)
  {
    Pair * p = this->Base::insert(Pair(std::move(key), data));
    return p != NULL ? &p->first : NULL;
  }

  Data * search(const Key & key) const
  {
    Pair * p = this->Base::search(Pair(key, Data()));
    return p != NULL ? &p->second : NULL;
  }

  bool has(const Key & key) const
  {
    return this->Base::search(Pair(key, Data())) != NULL;
  }

  Data & find(const Key & key)
  {
    return Base::find(Pair(key, Data())).second;
  }

  void remove_by_data(Data & data)
  {
    Base::remove(data_to_pair(&data));
  }

  void remove(const Key & key)
  {
    Base::remove(Pair(key, Data()));
  }

  Map_Sequences_Methods();
};



template <typename Key, typename Data, 
	  class Cmp = Aleph::Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
struct DynMapOLHash 
  : public DynMapOpenHash<Key, Data, Cmp, OLhashTable>
{
  typedef DynMapOpenHash<Key, Data, Cmp, OLhashTable> Base;  
  /*  Using Base::Base; */ 
};


template <typename Key, typename Data, 
	  class Cmp = Aleph::Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
struct DynMapODHash 
  : public DynMapOpenHash<Key, Data, Cmp, ODhashTable>
{
  typedef DynMapOpenHash<Key, Data, Cmp, ODhashTable> Base;  
  /*  Using Base::Base;  */
};




} /* End namespace Aleph */

# endif // TPL_DYNSETOHASH_H
