
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
  @file tpl_olhash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_OLHASH_H
# define TPL_OLHASH_H

# include <iostream>
# include <primes.H>
# include <ahDry.H>
# include <hash-dry.H>
# include <hash-fct.H>

using namespace Primes;

using namespace Aleph;

# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif

namespace Aleph {

/** 
  @brief Hash table with collision resolution by addressing
  open and linear probing available cuvette.

  @details This type implements a closed "hash" table (the array is
  contiguous in memory), which stores collisions within the same
  table. When a collision is polled first linearly
  bucket from the index given by the "hash" function.

  OLhashTable handles three parameters type:
  -# Key: Datatype that holds the table.
  -# Record: Data type to map by the key. 
  -# Cmp: Class of comparison between the keys; return true when
  the keys are equal.

  @ingroup Hash
*/
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
class OLhashTable
{
public:

  typedef Key Key_Type;

  typedef Key Item_Type;

/** @brief The type of hash function */
  typedef size_t (*Hash_Fct)(const Key &);

  enum Status { EMPTY, BUSY, DELETED };

  struct Bucket
  {
    Key key;
    char status;

    Bucket() : status(EMPTY) {}
    void reset() { status = EMPTY; }
  };

  static Bucket * key_to_bucket(Key * rec)
  {
    Bucket * ret_val = 0;
    long offset = (long) &ret_val->key;
    return (Bucket*) ((long) rec - offset); 
  }
  
  Bucket *    table;
  size_t      N;

protected:

  size_t      len;
  float       lower_alpha;
  float       upper_alpha;

private:

  Hash_Fct hash_fct;
  bool     with_resize;

  bool is_valid_bucket(Bucket * bucket) const
  {
    if (bucket < &table[0] or bucket >= &table[len]) 
      return false;

    int offset_with_base = (char*) bucket - (char*) &table[0];

    return offset_with_base % sizeof(*bucket) == 0;
  }

public:

/** @brief Instantiates a "hash" table with "hash" function __hash_fct and dimension "len". */
  OLhashTable(Hash_Fct      __hash_fct     = Aleph::dft_hash_fct<Key>, 
	      Hash_Fct      null_hash_fct  = NULL, /* Ficticius compatibily */
	      size_t         __len         = Primes::DefaultPrime,
 	      const float &  __lower_alpha = hash_default_lower_alpha,
	      const float &  __upper_alpha = hash_default_upper_alpha,
	      const bool     __with_resize = true)
    : table(NULL), N(0), len(Primes::next_prime(__len)), 
      lower_alpha(__lower_alpha), upper_alpha(__upper_alpha),
      hash_fct(__hash_fct), with_resize(__with_resize)
  {
    table = new Bucket [len];
  }

/** @brief Set free all occupied memory. */
  ~OLhashTable()
  {
    if (table != NULL)
      delete [] table;
  }

  void swap(OLhashTable & other)
  {
    std::swap(table, other.table);
    std::swap(N, other.N);
    std::swap(len, other.len);
    std::swap(hash_fct, other.hash_fct);
    std::swap(lower_alpha, other.lower_alpha);
    std::swap(upper_alpha, other.upper_alpha);
    std::swap(with_resize, other.with_resize);
  }

  OLhashTable(const OLhashTable & other)
    : OLhashTable(other.hash_fct, NULL, other.len,
		  other.lower_alpha, other.upper_alpha, other.with_resize)
  {
    copy_from_table(other);
  }

  OLhashTable(OLhashTable && other)
    : OLhashTable(other.hash_fct, NULL, other.len,
		  other.lower_alpha, other.upper_alpha, other.with_resize)
  {
    swap(other);
  }

  OLhashTable & operator = (const OLhashTable & other)
  {
    if (this == &other)
      return *this;

    if (len > other.N)
      clean_table();
    else
      {
	Bucket * new_table = new Bucket [other.len];
	delete [] table;
	table = new_table;
	N = 0;
	len = other.len;
	hash_fct = other.hash_fct;
	lower_alpha = other.lower_alpha;
	upper_alpha = other.upper_alpha;
      }

    copy_from_table(other);

    return *this;
  }

  OLhashTable & operator = (OLhashTable && other)
  {
    swap(other);
    return *this;
  }

/** @brief Search the "key" key and returns the associated record if "key"
located within the table; otherwise, it returns NULL. */
  Key * search(const Key & key) const
  {   
    long i = (*hash_fct)(key) % len, c = 0;

    while (c < len and table[i].status != EMPTY)
      {
	if (table[i].status == BUSY and Cmp () (table[i].key, key))
	  return &table[i].key;

	++c;
	if (++i == len)
	  i = 0;
      }

    return NULL; /* No se encuentra la clave */
  }

private:

  Bucket * allocate_bucket(const Key & key)
  {
    if (N >= len)
      throw std::overflow_error("Hash table is full");

    size_t i = (*hash_fct)(key) % len; 

    while (table[i].status == BUSY) /* Find a slot that is not busy */
      {
	if (Cmp () (key, table[i].key))
	  return NULL;

	if (++i == len)
	  i = 0;
      }

    Bucket * bucket = &table[i];

    /* i contiene celda con DELETED o EMPTY ==> ocuparla */
    bucket->status = BUSY;
    N++;

    return bucket;
  }

/**
  @brief Removed from the table the record pointed to by "record".

  @details Obviously, "record" should point to an entry thrown by
  insert() or search().
  Shoot invalid_argument exceptions if "record" is not a
  address within the range of the table or domain_error if the bucket
  registration is not occupied.

*/
  void deallocate_bucket(Bucket * bucket)
  {
    if (not is_valid_bucket(bucket))
      throw std::invalid_argument("record address is not inside table's range");

    if (bucket->status != BUSY)
      throw std::domain_error("Bucket containing record is not busy");

    --N;
    int i = bucket - &table[0]; /* índice de brecha */
    table[i].status = DELETED;
  }

  public:

/** 
  @fn insert()

  @brief Insert the "key" key.

  @return Returns a pointer to the key in the table.  
*/
  
/** 
  @fn remove()

  @brief Removes the key targeted by "key"
*/

  void remove(const Key & key)
  {
    Key * key_ptr = search(key);
    if (key_ptr == NULL)
      throw std::domain_error("Key not in hash table");

    remove(key_ptr);
  }

  OHASH_COMMON(OLhashTable);

  Generic_Traverse(Key);
  Functional_Methods(Key)
  Equal_To_Method(OLhashTable);

  Stats stats() const
  {									
    DynArray<size_t> lens;						
    size_t num_busy = 0, num_deleted = 0, num_empty = 0;			
    size_t max_len = std::numeric_limits<size_t>::min();			
    for (int i = 0; i < len; ++i)					
      switch (table[i].status)						
	{								
	case BUSY:							
	  {								
	    ++num_busy;							
	    const Key & key = table[i].key;				
	    long i = (*hash_fct)(key) % len;				
	    size_t count = 1;						
									
	    while (true)							
	      {								
		if (table[i].status == BUSY and Cmp () (table[i].key, key)) 
		  break;							
		++count;							
		if (++i == len)						
		  i = 0;							
	      }								
									
	    max_len = std::max(max_len, count);				
	    update_stat_len(lens, count);				
	    break;							
	  }								
	case EMPTY:							
	  ++num_empty;							
	  update_stat_len(lens, 0);					
	  break;								
	case DELETED:							
	  ++num_deleted;							
	  break;								
	}								
 									
    float avg = 0, sum = 0;						
    for (int i = 0; i < lens.size(); ++i)				
      {									
	avg += lens(i)*i;						
	sum += lens(i);							
      }									
									
    avg /= sum;								
    float var = 0;							
    for (int i = 0; i < lens.size(); ++i)				
      {									
	float s = i - avg;						
	var += lens(i)*s*s;						
      }									
    var /= sum;								
   									
    Stats stats;								
    stats.num_busy = num_busy;						
    stats.num_deleted = num_deleted;					
    stats.num_empty = num_empty;						
    std::swap(lens, stats.lens);						
    stats.avg = avg;							
    stats.var = var;							
    stats.max_len = max_len;						
									
    return stats;						
  }
};
									
}
# endif // TPL_OLHASH_H
