 
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
  @file tpl_dynSetHash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNSETHASH_H
# define TPL_DYNSETHASH_H

# include <algorithm>
# include <typeinfo>
# include <ahDry.H>
# include <primes.H>
# include <htlist.H>
# include <tpl_dynArray.H>
# include <tpl_dynSetOhash.H>
# include <tpl_lhash.H>
# include <tpl_linHash.H>


namespace Aleph  
{

/** 
  @brief dynamic self-adjusting hash table

  @details DynHashTable <Key> implements a linear hash table and
  dynamic in which you need not worry about the size;
  it is automatically adjusted limits factor
  load defined in construction time.

  @param Key Data type of the domain under which is indexed
  the range.

  @see LinearHashTable

  @ingroup Hash
*/
  template <typename                          Key, 
	    class                             Cmp       = Aleph::equal_to<Key>,
	    template <typename,  class> class HashTable = LhashTable>
class DynHashTable : public HashTable<Key, Cmp>
{
protected:

  typedef HashTable<Key, Cmp> Base;

  typedef typename HashTable<Key, Cmp>::Bucket Bucket;

public:

/** 
  @brief The type of hash function. 
*/
  typedef typename Base::Hash_Fct Hash_Fct;

  typedef Key Key_Type;

  typedef Key Item_Type;

/** 
  @brief Instance linear dynamic hash table.

  @param [in] hash_fct Hash function.
  @param [in] len initial and minimum size of the hash table. the
  size never be lower than this.
  @param [in] lower_alpha lower load threshold; size
  table is reduced if the load is less than this factor.
  @param [in] upper_alpha upper load threshold; size
  It expands table if the load is higher than this factor.
*/
  DynHashTable(Hash_Fct hash_fct        = Aleph::dft_hash_fct<Key>,
	       const size_t len         = Primes::DefaultPrime,
	       const float  lower_alpha = hash_default_lower_alpha,
	       const float  upper_alpha = hash_default_upper_alpha) 
    : Base(hash_fct, len, lower_alpha, upper_alpha, true, true)
  {
    /* Empty */
  }

private:

  void copy(const DynHashTable & other)
  {
    for (typename Base::Iterator it(other); it.has_curr(); it.next())
      {
	Bucket * bucket = (Bucket*) it.get_curr();
	insert(bucket->get_key());
	assert(*search(bucket->get_key()) == bucket->get_key());
      }
  }

public:

  DynHashTable(const DynHashTable & other)
    : Base(other.hash_fct, other.len, 
	   other.lower_alpha, other.upper_alpha, true, true)
  {
    copy(other);
  }
  
  DynHashTable(DynHashTable && other)
    : Base(other.hash_fct, other.len, 
	   other.lower_alpha, other.upper_alpha, true, true)
  {
    this->swap(other);
  }

  ~DynHashTable()
  {
    this->empty();
  }

  DynHashTable & operator = (const DynHashTable & other)
  {
    if (this == &other)
      return *this;

    this->empty();
    copy(other);

    return *this;
  }

  DynHashTable & operator = (DynHashTable && other)
  {
    this->swap(other);
    return *this;
  }

protected:

  Key * insert_bucket(Bucket * bucket)
  {
    Bucket * ret_val = (Bucket*) this->Base::insert(bucket);
    /* Is the key in the table?
    yes! ==> free bucket */
    if (ret_val == NULL) 
      {    
	delete bucket;
	return NULL;
      }

    return &ret_val->get_key();
  }

public:

/** 
  @brief Inserted in the set the key hash key.
  
  @return Returns a pointer to the key if it was inserted or
  NULL if the key was not inserted
*/
  Key * insert(const Key & key)
  {
    return insert_bucket(new Bucket (key));
  }

  Key * insert(Key && key)
  {
    return insert_bucket(new Bucket (std::move(key)));
  }

  Key * add(const Key & key)
  {
    return insert_bucket(new Bucket (key));
  }
 
  Key * add(Key && key)
  {
    return insert_bucket(new Bucket (std::move(key)));
  }

/** 
  @brief Search the specified key, if it is, then returns a pointer
  within the table to the key, otherwise-not is the key-,
  NULL is returned:
*/
  Key * search(const Key & key) const
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);

    return bucket != NULL ? &bucket->get_key() : NULL;
  }

  bool has(const Key & key) const 
  {
    return this->Base::search(key) != NULL;
  }

  bool contains(const Key & key) const { return has(key); }

  Key & find(const Key & key)
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);

    if (bucket == NULL)
      throw std::domain_error("Key not found in hash");

    return bucket->get_key();
  }

/** 
  @brief Returns a container with all the keys of the table 
*/
  Generic_Keys(Key);

protected:

  static Bucket * key_to_bucket(Key * key)
  {
    Bucket * ret_val = 0;
    size_t offset = reinterpret_cast<size_t>(&ret_val->get_key());
    
    return reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(key) - offset);
  }

public:

/** 
  @brief Removes the specified "key" key whose pointer must be
  resulting from an insert or search. 
*/
  void remove(Key * key)
  {
    Bucket * bucket = key_to_bucket(key);
    this->Base::remove(bucket);
    delete bucket;
  }

  void remove(const Key & key)
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);
    if (bucket == NULL)
      throw std::domain_error("Key not in hash table");

    this->Base::remove(bucket);
    delete bucket;
  }


/** 
  @fn for_each() Traverse all keys and for each one	executes operation. 

  @brief Operation must have the signature

  @code
  bool operation(const Key & key)
  @endcode

  If @code operation() @endcode returns false then the traversal is
  aborted; otherwise the the routine advance and so on

  @param[in] operation
  
  @return true if all items are traversed; false otherwise
*/

/** 
  @fn map() Traverse all keys and produce a list resulting of
  calling operation() on each key

  @brief Operation must have the signature

  @code
  ResultType operation(const Key & key)
  @endcode

  @param[in] operation
*/

  Generic_Traverse(Key);
  Functional_Methods(Key);
  Equal_To_Method(DynHashTable);

  class Iterator : public Base::Iterator
  { 
  public: 

    Iterator(const DynHashTable & table) : Base::Iterator(table) {} 

    const Key & get_curr() 
    { 
      return this->Base::Iterator::get_curr()->get_key(); 
    } 
    
    void del() { delete this->Base::Iterator::del(); } 
  }; 
};

/** 
  @ingroup Hash 
*/
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct DynSetLhash : public DynHashTable<Key, Cmp, LhashTable>
{
  typedef DynHashTable<Key, Cmp, LhashTable> Base;

  using Base::Base;
}; 

/** 
  @ingroup Hash 
*/
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct DynSetLinHash : public DynHashTable<Key, Cmp, LinearHashTable>
{
  typedef DynHashTable<Key, Cmp, LinearHashTable> Base;

  using Base::Base;
}; 

/** 
  @ingroup Hash 
*/
template <typename Key, class Cmp = Aleph::equal_to<Key>>
  using DynSetHash = DynHashTable<Key, Cmp, LhashTable>;

/** 
  @ingroup Hash 
*/
template <typename Key, typename Data, 
	  class Cmp = Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>,
	  template <class, class> class HashTable = LinearHashTable>
class DynMapHashTable 
  : public DynHashTable<std::pair<Key, Data>, Cmp, HashTable>
{
  typedef std::pair<Key, Data> Pair;

  typedef DynHashTable<std::pair<Key, Data>, Cmp, HashTable> Base;

  typedef typename Base::Bucket Bucket;

  public:

    /* Given a valid reference to key, given from find or through iterator
    or other functional primitives, get_data() retrieves the data
    associated to the key inside the table. */
  Data & get_data(const Key & key)
  {
    return key_to_pair<Key, Data>(&const_cast<Key&>(key))->second;    
  }

  const Key & get_key(Data * data_ptr)
  {
    return data_to_pair<Key, Data>(data_ptr)->first;
  }

  typedef Data Value_Type;

  typedef size_t (*Hash_Fct)(const Key&);

  using Base::Base;

  template <size_t (*fct)(const Key & k)> static
  size_t wrapper(const std::pair<Key, Data> & p)
  {
    return (*fct)(p.first);
  }

/** 
  @brief Constructor.

  @details Please note that although the hash_fct parameter defines a hash
  function for std::pair<Key, Data> types, the hash function MUST
  only compute on the first field of pair. If you do not do so, then
  the mapping will fail, since it will be a set of pairs.

  @warning READ VERY CAREFULLY IF YOU PLAN PASS YOUR OWN HASH FUNCTION
*/
  DynMapHashTable
  (typename Base::Hash_Fct hash_fct = wrapper<Aleph::dft_hash_fct<Key>>,
   const size_t len                 = Primes::DefaultPrime,
   const float  lower_alpha         = hash_default_lower_alpha,
   const float  upper_alpha         = hash_default_upper_alpha) 
    : Base(hash_fct, len, lower_alpha, upper_alpha)
  {
    /* Empty */
  }

/** 
  @brief Inserted into the hash mapping the pair (key, record) indexed by key. 

  @return Returns a pointer to the row within the table. 
*/
Dispara la excepción bad_alloc si no hay suficiente memoria. 
  Key * insert(const Key & key, const Data & data)
  {
    Pair * p = this->insert_bucket(new typename Base::Bucket (Pair(key, data)));

    return p != NULL ? &p->first : NULL;
  }

  Key * insert(const Key & key, Data && data)
  {
    Pair * p = this->insert_bucket
      (new typename Base::Bucket (Pair(key, std::move(data))));

    return p != NULL ? &p->first : NULL;
  }

  Key * insert(Key && key, Data && data)
  {
    Pair * p = this->insert_bucket
      (new typename Base::Bucket (Pair(std::move(key), std::move(data))));

    return p != NULL ? &p->first : NULL;
  }

  Key * insert(Key && key, const Data & data)
  {
    Pair * p= this->insert_bucket
      (new typename Base::Bucket (Pair(std::move(key), data)));

    return p != NULL ? &p->first : NULL;
  }

/** 
  @brief Look for the key key and if found, then returns a pointer
  within the table to the associated registration,
  otherwise-not is the key-, NULL is returned:
*/
  Data * search(const Key & key) const
  {
    Pair * p = this->Base::search(Pair(key, Data()));
    return p != NULL ? &p->second : NULL;
  }

  bool has(const Key & key) const
  {
    return this->Base::search(Pair(key, Data())) != NULL;
  }

  bool contains(const Key & key) const { return has(key); }

  Data & find(const Key & key)
  {
    return Base::find(Pair(key, Data())).second;
  }

/** 
  @brief Removes the specified key table whose pointer must be
  resulting from an insert or search.
*/
  void remove_by_data(Data & data)
  {
    Base::remove(data_to_pair<Key, Data>(&data));
  }

  void remove(const Key & key)
  {
    Base::remove(Pair(key, Data()));
  }

  Map_Sequences_Methods();

  Generate_Proxy_Operator(DynMapHashTable);
};

/**
  @ingroup Hash 
*/
template <typename Key,
 	  typename Data,
 	  class    Cmp = Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
  using DynMapLinHash = DynMapHashTable<Key, Data, Cmp, LinearHashTable>;

/** 
  @ingroup Hash 
*/
template <typename Key,
 	  typename Data,
 	  class    Cmp = Dft_Pair_Cmp<Key,Data, Aleph::equal_to<Key>>>
  using DynMapHash = DynMapHashTable<Key, Data, Cmp, LhashTable>;


} /* End namespace Aleph */

# endif // TPL_DYNSETHASH_
  