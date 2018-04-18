
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
  @file tpl_odhash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_ODHASH_H
# define TPL_ODHASH_H

# include <iostream>
# include <limits>
# include <primes.H>
# include <htlist.H>
# include <ahDry.H>
# include <hash-dry.H>
# include <hash-fct.H>
# include <tpl_dynArray.H>

using namespace Aleph;

namespace Aleph {

# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif


/** 
  @brief Closed hash table with collision resolution by double hash function.

  @details This type implements a closed hash table (the array is
  contiguous memory), which stores collisions within the same
  table. When a collision occurs, a second function is invoked
  to probe a hash bucket available. If you are,
  then the key and the record is placed in the bucket
  available; otherwise, it polls linearly from
  given index for the second hash function.

  The table uses a method that eliminates without relocation and
  constant time buckets are deleted in the middle of
  a search string.

  ODhashTable manages two type parameters:
  -# Key: Datatype that holds the table.
  -# Record: Data type to map by the key.
  -# Cmp: Class of comparison between the keys; must return
  true if the keys are equal.

  @ingroup Hash
*/
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
class ODhashTable
{
  public:

  typedef Key Key_Type;

  typedef Key Item_Type;

/** @brief The type of hash function. */
  typedef size_t (*Hash_Fct)(const Key &);

  enum Status { EMPTY, BUSY, DELETED };

  enum Probe { NO_PROBED, FIRST_PROBE, SECOND_PROBE, LINEAR_PROBE };

  struct Bucket
  {
    Key      key;               /* Clave */
    unsigned status     : 4;    /* Status EMPTY, DELETED o BUSY */
    unsigned probe_type : 4;    /* FIRST_PROBE SECOND_PROBE LINEAR_PROBE */
    unsigned int probe_counter; /* Contador de sondeos */

    Bucket() : status(EMPTY), probe_type(NO_PROBED), probe_counter(0)
    { /* Empty */ }

    void reset() /* Put all as when constructed */
    {
      status = EMPTY;
      probe_type = NO_PROBED;
      probe_counter = 0;
    }
  };

  Bucket * table           = NULL; /* Arreglo de cubetas */
  Hash_Fct hash_fct        = NULL; /* Primera función hash */
  Hash_Fct second_hash_fct = NULL; /* Segunda función hash */

protected:

  size_t len;                       /* Tamaño de la tabla */
  float  lower_alpha;
  float  upper_alpha;

private:

  size_t   N;                       /* Número de cubetas ocupadas */
  bool     with_resize;

  Bucket * allocate_bucket(Bucket & bucket, unsigned char probe_type)
  {
    assert(bucket.status != BUSY);

    ++N;
    bucket.status     = BUSY;
    bucket.probe_type = probe_type;
    bucket.probe_counter++;

    return &bucket;
  }

  void decrease_probe_counter(Bucket * bucket)
  {
    assert(bucket->status == BUSY or bucket->status == DELETED);

    bucket->probe_counter--;
    if (bucket->probe_counter == 0) /* marcar EMPTY sobre la cubeta? */
	bucket->status = EMPTY;
  }

  void deallocate_bucket(Bucket * bucket)
  {
    assert(bucket->status == BUSY);

    bucket->status = DELETED;
    const Key & key = bucket->key;

    const size_t i_fst = (*hash_fct)(key) % len;
    if (&table[i_fst] == bucket)
      {
	assert(Cmp () (table[i_fst].key, key)); 
	assert(table[i_fst].probe_type == FIRST_PROBE);
	;
      }
    else
      {
	const size_t i_snd = (*second_hash_fct)(key) % len;
	if (&table[i_snd] == bucket)
	  {
	    assert(Cmp () (table[i_snd].key, key));
	    assert(table[i_snd].probe_type == SECOND_PROBE);
	    decrease_probe_counter(&table[i_fst]);
	    ;
	  }
	else
	  {
	    decrease_probe_counter(&table[i_fst]);
	    decrease_probe_counter(&table[i_snd]);
	    size_t i = i_snd;
	    for (index_forward(i); &table[i] != bucket; index_forward(i))
	      {
		assert(table[i].status != EMPTY);
		decrease_probe_counter(&table[i]);
	      }
	    assert(Cmp () (table[i].key, key));
	    assert(table[i].probe_type == LINEAR_PROBE);
	  }
      }
    
    decrease_probe_counter(bucket);
    --N;
  }

  size_t index_forward(size_t & i) const
  {
    assert(i < len);
    if (++i == len)
      i = 0;
    return i;
  }

  size_t index_backward(size_t & i) const
  {
    assert(i < len);
    if (i-- == 0)
      i = len - 1;
    return i;
  }

  static Bucket * key_to_bucket(Key * rec)
  {
    Bucket * ret_val = 0;
    const long offset = (long) &ret_val->key;
    return (Bucket*) ((long) rec - offset); 
  }

  bool is_valid_bucket(Bucket * bucket)
  {
    if (bucket < &table[0] or bucket >= &table[len])
      return false;

    const int offset_with_base = (char*) bucket - (char*) &table[0];
    return offset_with_base % sizeof(*bucket) == 0;
  }

  size_t bucket_to_index(Bucket * bucket) 
  {
    assert(is_valid_bucket(bucket));
    return bucket - &table[0];
  }

public:

  void swap(ODhashTable & other)
  {
    std::swap(table, other.table);
    std::swap(hash_fct, other.hash_fct);
    std::swap(second_hash_fct, other.second_hash_fct);
    std::swap(N, other.N);
    std::swap(len, other.len);
  }

/** 
  @brief Instance a closed hash table with collision resolution by
  double hash.

  @param[in] __first_hash_fct Hash function to the first survey.
  @param[in] __second_hash_fct Hash function for the second survey.
  @param[in] len Table size.

  @throw bad_alloc If no memory for the table of bucket.
  
*/
  ODhashTable(Hash_Fct __first_hash_fct    = Aleph::dft_hash_fct<Key>, 
	      Hash_Fct __second_hash_fct   = Aleph::snd_hash_fct<Key>, 
              const size_t  __len          = Primes::DefaultPrime,
 	      const float &  __lower_alpha = hash_default_lower_alpha,
	      const float &  __upper_alpha = hash_default_upper_alpha,
	      const bool     __with_resize = true)
    : table(NULL), hash_fct(__first_hash_fct), 
      second_hash_fct(__second_hash_fct), len(Primes::next_prime(__len)), 
      lower_alpha(__lower_alpha), upper_alpha(__upper_alpha),
      N(0), with_resize(__with_resize)
  {
    table = new Bucket[len];
  }

/** @brief Ser free all the hash table. */
  ~ODhashTable()
  {
    if (table != NULL)
      delete [] table;
  }

  ODhashTable(const ODhashTable & other)
    : ODhashTable(other.hash_fct, other.second_hash_fct, other.len, 
		  other.lower_alpha, other.upper_alpha, other.with_resize)
  {
    assert(table != NULL);
    copy_from_table(other);
  }

  ODhashTable(ODhashTable && other)
    : ODhashTable(other.hash_fct, other.second_hash_fct, other.len, 
		  other.lower_alpha, other.upper_alpha, other.with_resize)
  {
    assert(table != NULL);
    swap(other);
  }

  ODhashTable & operator = (const ODhashTable & other)
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
	hash_fct        = other.hash_fct;
	second_hash_fct = other.second_hash_fct;
	lower_alpha     = other.lower_alpha;
	upper_alpha     = other.upper_alpha;
	with_resize     = other.with_resize;
      }

    copy_from_table(other);

    return *this;
  }

  ODhashTable & operator = (ODhashTable && other)
  {
    swap(other);
    return *this;
  }

  OHASH_COMMON(ODhashTable);

  Generic_Traverse(Key);
  Functional_Methods(Key);
  Equal_To_Method(ODhashTable);

/** @brief Search the table the "key" key.
Returns a pointer to the record associated with "key" in the table;
NULL otherwise. */
  Key * search(const Key & key) const
  {
    const size_t i_fst = (*hash_fct)(key) % len; /* 1er sondeo (1ra fun hash) */
    if (table[i_fst].status == EMPTY) 
      return NULL;

    if (table[i_fst].status == BUSY and Cmp() (table[i_fst].key, key))
      {
	assert(table[i_fst].probe_type == FIRST_PROBE);
	assert(table[i_fst].probe_counter > 0);
	return &table[i_fst].key;
      }

    const size_t i_snd = (*second_hash_fct)(key) % len; /* 2do sondeo */

    if (table[i_snd].status == EMPTY)
      return NULL;

    if (table[i_snd].status == BUSY and Cmp() (table[i_snd].key, key)) 
      {
	assert(table[i_snd].probe_type == SECOND_PROBE);
	assert(table[i_snd].probe_counter > 0);
	return &table[i_snd].key;
      }

    size_t i = i_snd; 
    
    /* Sondeo lineal a partir de índice de 2da función hash */
    for (int count = 0; count < len; ++count)
      {
	index_forward(i);
	switch (table[i].status)
	  {
	  case EMPTY:
	    assert(table[i].probe_counter == 0);
	    return NULL;
	  case BUSY:
	    assert(table[i].probe_counter > 0);
	    if (Cmp() (table[i].key, key))
	      {
		assert(table[i].probe_type == LINEAR_PROBE);
		return &table[i].key;
	      }
	    break;
	  case DELETED:
	    assert(table[i].probe_counter > 0);
	    break;
	  default: ERROR("ODhashTable search: inconsistent bucket status");
	  }
      }
   
    return NULL; 
  }

  Hash_Fct get_second_hash_fct() const { return second_hash_fct; }

  void set_second_hash_fct(Hash_Fct fct)
  {
    second_hash_fct = fct;
  }

private:

  Bucket * allocate_bucket(const Key & key)
  {
    if (N >= len)
      throw std::overflow_error("Hash table is full");

    const size_t i_fst = (*hash_fct)(key) % len; /* Sondeo con 1ra función hash */
    if (table[i_fst].status != BUSY) /* ¿Cubeta disponible? */
      return allocate_bucket(table[i_fst], FIRST_PROBE);

    if (Cmp () (table[i_fst].key, key)) /* Test if key is alredy present */
      return NULL;

    const size_t i_snd = (*second_hash_fct)(key) % len; /* 2do hash */
    if (table[i_snd].status != BUSY) /* ¿Cubeta disponible? */
      {
	table[i_fst].probe_counter++;
	return allocate_bucket(table[i_snd], SECOND_PROBE);
      }

    if (Cmp () (table[i_snd].key, key)) /* Test if key is alredy present */
      return NULL;

    size_t i = i_snd;
    for (int c = 0; c < len; ++c)
      {	 
	index_forward(i);
	switch (table[i].status)
	{
	case BUSY: 
	  if (Cmp() (table[i].key, key))
	    { 
    /** Duplicated key ==> rollback previous probe_counter increases */
	      for (int k = 0; k < c; ++k)
		{
		  index_backward(i);
		  table[i].probe_counter--;
		}
	      return NULL;
	    }
	  break;
	case DELETED: 
	case EMPTY:
	  table[i_fst].probe_counter++;
	  table[i_snd].probe_counter++;
	  return allocate_bucket(table[i], LINEAR_PROBE); 
	default: ERROR("ODhashTable: Invalid bucket status"); break;
	}
	table[i].probe_counter++;
      }

    return NULL;
  }

/** 
  @brief Removed from the table log record.

  @details record must belong to the table and should have been determined
  previously by an insertion or search.
  The invalid_argument exception is triggered if 
  the record does not correspond to a bucket of the table.
  it fires domain_error if the record in the table does not contain an element.
*/
  void remove_bucket(Bucket * bucket)
  {
    if (not is_valid_bucket(bucket))
      throw std::invalid_argument("key pty does not belong to hash table");

    if (bucket->status != BUSY)
      throw std::domain_error("Bucket containing key is not BUSY");

    deallocate_bucket(bucket);
  }

/** @brief Returns true if the bucket contains the key */
  bool process_entry_for_remove(Bucket * bucket, const Key & key)
  {
    switch (bucket->status)
      {
      case EMPTY:
	throw std::domain_error("Key not in hash table");
      case BUSY:
	if (Cmp () (bucket->key, key))
	  {
	    bucket->status = DELETED;
	    decrease_probe_counter(bucket);
	    return true;
	  }
      case DELETED:
	decrease_probe_counter(bucket);
	break;
      default: ERROR("Inconsistent bucket status");
      }
    return false;
  }

public:

  void remove(const Key & key)
  {
    --N;
    try
      {
	const size_t i_fst = (*hash_fct)(key) % len;
	if (process_entry_for_remove(&table[i_fst], key))
	  return;
    
	const size_t i_snd = (*second_hash_fct)(key) % len;
	if (process_entry_for_remove(&table[i_snd], key))
	  return;

	size_t i = i_snd;
	for (int c = 0; i < len; ++c)
	  {
	    index_forward(i);
	    if (process_entry_for_remove(&table[i], key))
	      return;
	  }
      }
    catch (...)
      {
	++N;
	rehash(); /* In order to leave the table in a consistent state */
	throw;
      }
  }
  
  Stats stats()	const							
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
	    size_t count = 1;
	    const size_t i_fst = (*hash_fct)(key) % len;
	    if (Cmp () (table[i_fst].key, key))
		{
		  assert(table[i_fst].probe_type == FIRST_PROBE);
		  assert(table[i_fst].probe_counter > 0);
		  ;
		}
	    else 
	      {
		++count;
		size_t i_snd = (*second_hash_fct)(key) % len;
		if (Cmp () (table[i_snd].key, key))
		{
		  assert(table[i_snd].probe_type == SECOND_PROBE);
		  assert(table[i_snd].probe_counter > 0);
		  ;
		}
		else
		  {
		    for (size_t i = index_forward(i_snd); true; index_forward(i))
		      {
			if (table[i].status == BUSY and 
			    Cmp () (table[i].key, key))
			  break;
			++count;
		      }
		  }
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




# ifdef NBACKUP
# define N NBACKUP
# undef NBACKUP
# endif

# ifdef MBACKUP
# define M MBACKUP
# undef MBACKUP
# endif

# undef EMPTY
# undef BUSY
# undef DELETED
# undef NO_PROBED    
# undef FIRST_PROBE  
# undef SECOND_PROBE 
# undef LINEAR_PROBE 

} /* End namespace Aleph */

# endif /* TPL_ODHASH_H
