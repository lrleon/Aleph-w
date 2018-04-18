
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
  @file tpl_lhash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_LHASH_H
# define TPL_LHASH_H

# include <iostream>
# include <primes.H>
# include <hash-fct.H>
# include <tpl_dlist.H>
# include <tpl_dynArray.H>
# include <hash-dry.H>

# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif

using namespace Primes;

using namespace Aleph;

namespace Aleph {

/** 
  @brief Generic hash table with collision resolution by
  separate chaining.

  @details The GenLhashTable type implements a generic hash table with
  collision resolution by chaining separate n which the chain
  cell type is a type parameter.

  Typically, this is intended as a back-end rates
  LhashTable and LhashTableVtl, which are fundamentally the
  same, except that they are defined and their classes
  buckets with or without virtual destructor.

  @param Key Key of indexing of the table.
  @param BucketType Bucket type.
  @param Cmp Class comparison between the keys.

  @see LhashTable LhashTableVtl 

  @ingroup Hash
*/
    template <typename Key, class BucketType, class Cmp> 
class GenLhashTable
{
public:

  typedef BucketType Bucket;

  typedef size_t (*Hash_Fct)(const Key &);

  typedef Key Key_Type;

  typedef Key Item_Type;

protected:

  Hash_Fct hash_fct;

private:

  typedef Dnode<Key>                    BucketList;/* Type list buckets */
  typedef typename Dnode<Key>::Iterator BucketItor;/* Iterator buckets */
  typedef Dnode<Key>                    Node;      /* Synonymous node */

  BucketList *                          table;

protected:

  size_t len;                  /* Table Size */
  float  lower_alpha;
  float  upper_alpha;

private:

  size_t N;                  /* Number of items in the table */
  size_t busy_slots_counter; /* Number of occupied posts */
  bool remove_all_buckets;   /* Buckets release destroyer */
  bool with_resize;

public:

/** 
  @brief Instance a generic hash table.

  @param [in] __hash_fct Hash function.
  @param [in] table_size Table size.
  @param [in] __remove_all_buckets If true, then
  release all buckets when it invokes the destructor.
  
  @throw bad_alloc If no memory to divide the table.
*/
  GenLhashTable(Hash_Fct       __hash_fct, 
                size_t         table_size    = Primes::DefaultPrime,
		const float &  __lower_alpha = hash_default_lower_alpha,
		const float &  __upper_alpha = hash_default_upper_alpha,
                const bool     __remove_all_buckets = true,
		const bool     __with_resize        = true) 
    throw(std::exception, std::bad_alloc) 
    : hash_fct(__hash_fct), table(NULL),
      len(Primes::next_prime(table_size)), 
      lower_alpha(__lower_alpha), upper_alpha(__upper_alpha),
      N(0), busy_slots_counter(0), remove_all_buckets(__remove_all_buckets),
      with_resize(__with_resize)
  { 
    table = new BucketList [len];
  }

  void swap(GenLhashTable & other)
  {
    std::swap(hash_fct, other.hash_fct);
    std::swap(table, other.table);
    std::swap(len, other.len);
    std::swap(N, other.N);
    std::swap(busy_slots_counter, other.busy_slots_counter);
    std::swap(remove_all_buckets, other.remove_all_buckets);
    std::swap(with_resize, other.with_resize);
  }

/** 
  @brief Empty the hash table; frees memory for all buckets. 
*/
  void empty()
  {
    for (int i = 0; i < len; ++i)
      for (BucketItor itor(table[i]); itor.has_curr(); /* Nothing */)
        delete (Bucket*) itor.del();
    busy_slots_counter = N = 0;
  }

private:

  Bucket * search_in_bucket_list(BucketList & list, const Key & key) const
  {
    for (BucketItor it(list); it.has_curr(); it.next())
      {
        Bucket * bucket = static_cast<Bucket*>(it.get_current());
        if (Cmp () (key, bucket->get_key())) 
          return bucket; 
      }

    return NULL;
  }

public:

  Hash_Fct get_hash_fct() const { return hash_fct; }

/** 
  @brief Set the internal hash function. 
*/
  void set_hash_fct(Hash_Fct fct)
  {
    hash_fct = fct;
  }

/** 
  @brief Return the current table load. 
*/
  float current_alpha() const { return 1.0*N/len; }

/** 
  @brief inserted into the table and returns the bucket bucket address if
  the key is not in the table; otherwise NULL is returned 
*/
  Bucket * insert(Bucket * bucket) 
  {
    const size_t i = (*hash_fct)(bucket->get_key()) % len;
    BucketList & list = table[i];

    /** Is the table empty list [i]?
    If the answer is YES, then update counter occupation */
    if (list.is_empty()) 
      ++busy_slots_counter;  

    if (search_in_bucket_list(list, bucket->get_key()) != NULL)
      return NULL;

    list.append(bucket); /* Insert cuvette end */
    ++N;

    if (with_resize and current_alpha() >= upper_alpha)
      resize(next_prime(2*len));

    return bucket;
  }

/** 
  @brief Search the table a bucket with key key.

  @return Returns a pointer to the pan if it is;
  NULL otherwise.
*/
  Bucket * search(const Key & key) const
  {
    const size_t i = (*hash_fct)(key) % len;
    return search_in_bucket_list(table[i], key);
  }

private:

    /* Remove without perform resizing. 
    This is strictly required insidecthe del() method of Iterator. 
    In addtion dries the code */
  Bucket * remove_bucket(Bucket * bucket) 
  {     /* Guarda próxima colisión */
    Bucket * next = static_cast<Bucket*>(bucket->get_next());
    bucket->del(); /* Remove collisions wishlist */
    if (next->is_empty()) /* Is the list became empty? */
      --busy_slots_counter;/* Yes ==> update counter busy schedules */

    --N;

    return bucket;
  }

public:


/**
  @brief Removed from the table the bucket bucket.

  @return Returns the address of the cuvette.
  
  @note Not verified that the trough belonging to the table.
*/
  Bucket * remove(Bucket * bucket) 
  {     /* Save next collision */
    remove_bucket(bucket);

    if (with_resize and current_alpha() < lower_alpha)
      resize(next_prime(len/2));

    return bucket;
  }

/** 
  @brief Resets the size of the hash table relocates to new_size value and keys.

  @throw bad_alloc if there is not enough memory to relocate the settlement.
*/
  size_t resize(size_t new_size) throw(std::exception, std::bad_alloc)
  {
    assert(len > 0);

    if (new_size == len or new_size == 0)
      return len; 

    BucketList * new_table = new BucketList [new_size];
    BucketList * old_table = table; /* Save current table status */
    const size_t old_size  = len;
    table                  = new_table; /* Empty table with new arrangement */
    len                    = new_size;
    busy_slots_counter     = N = 0;

    /** Reinsert buckets */
    for (int i = 0; i < old_size; ++i) 
          
    /** Browse collision list i old_table[i]*/
      for (BucketItor it(old_table[i]); it.has_curr(); /* Nothing */)
        insert((Bucket*) it.del()); /* Remove and insertar in table[] */

    delete [] old_table; /* Releasing old memory table */

    return len;
  }

/** 
  @brief Frees memory the table and if it is true remove_all_buckets
  (you specified in the constructor), then also released
  the memory of all buckets. 
*/
  virtual ~GenLhashTable()
  {
    if (remove_all_buckets)
      empty();

    delete [] table; 
  }

/** 
  @brief Returns the next trough collides with bucket;
  If not, then NULL is returned. 
*/
  Bucket * search_next(Bucket * bucket) const 
  {
    assert(bucket != NULL);

    const size_t i = (*hash_fct)(bucket->get_key()) % len;

    BucketItor itor(table[i]); 
    itor.set(bucket);

    while (true)
      {
        itor.next();

        if (not itor.has_curr()) 
          return NULL;

        Bucket * node = static_cast<Bucket*>(itor.get_curr());

        if (Cmp () (bucket->get_key(), node->get_key())) 
          return node;
      }
  }

/** 
  @brief Returns the size of the table. 
*/
  const size_t & capacity() const { return len; }

/** 
  @brief Returns the number of elements contained in the table. 
*/
  const size_t & size() const { return N; }

/** 
  @brief Returns the number of entries of the array that are occupied.
*/
  const size_t & get_num_busy_slots() const { return busy_slots_counter; }

  bool is_empty() const { return N == 0; }

/** 
  @brief Iterator over a hash table GenLhasTable.

  @note The observation sequence not thrown by the iterator is ordered.

  @ingroup Hash
*/
  class Iterator
  {
  private:

    int             curr_index; /* Index in table */
    BucketItor      curr_itor;  /* Iterator over table[curr_index] */
    GenLhashTable * hash_table;

/** 
  @brief Advances to next entry table
*/
    void locate_next_available_entry() 
    {
      while (true)
        {
          if (curr_index == hash_table->len)
            throw std::overflow_error ("hash table iterator overflow");

          if (curr_index == hash_table->len - 1)
            {  /* Be in overflow state */
              curr_index = hash_table->len;
              return;
            }

          ++curr_index;

          if (not hash_table->table[curr_index].is_empty())
            {
              BucketItor itor(hash_table->table[curr_index]);
              curr_itor = itor;
              return;
            }
        }
    }

    void locate_next_available_bucket() 
    {
      curr_itor.next();

      if (not curr_itor.has_curr())
        locate_next_available_entry();
    }

    void locate_prev_available_entry() 
    {
      while (true)
        {
          if (curr_index == -1)
            throw std::underflow_error ("hash table iterator underflow");

          if (curr_index == 0)
            { /* Be in overflow state */
              curr_index = -1;
              return;
            }

          --curr_index;

          if (not hash_table->table[curr_index].is_empty())
            {
              BucketItor itor(hash_table->table[curr_index]);
              curr_itor = itor;
              curr_itor.reset_last();
              return;
            }
        }
    }
    
    void locate_prev_available_bucket() 
    {
      curr_itor.prev();

      if (not curr_itor.has_curr())
        locate_prev_available_entry();
    }

public:

/** 
  @brief The set type on which iterates. 
*/
    typedef GenLhashTable Set_Type;

/** 
  @brief The type of element returns get_curr() 
*/
    typedef Bucket * Item_Type;

/** 
  @brief An iterator instance hash table on the table.
*/
    Iterator(const GenLhashTable & table) 
      : curr_index(-1), hash_table(&const_cast<GenLhashTable&>(table))
    {
      try
        {
          locate_next_available_entry();
        }
      catch (std::overflow_error)
        { /* Nothing to do */ }
    }
    
/** 
  @brief An empty iterator instance.
*/
    Iterator() : curr_index(-1), hash_table(NULL)
    {
      /* Empty */  
    }

/** 
  @brief Resets the iterator on the first bucket.
*/
    void reset_first()
    {
      curr_index = -1;
      
      locate_next_available_entry();
    }

/** 
  @brief Resets the iterator over the last bucket.
*/
    void reset_last()
    { 
      curr_index = hash_table->len;

      locate_prev_available_entry();
    }

/** 
  @brief Returns true if the iterator has current bucket. 
*/
    bool has_curr() const 
    {
      return curr_index >= 0 and curr_index < hash_table->len;
    }
    
/** 
  @brief Returns the current bucket.
*/
    Bucket * get_curr() 
      throw(std::exception, std::overflow_error, std::underflow_error)
    {
      if (curr_index == -1)
        throw std::underflow_error ("hash table iterator underflow");

      if (curr_index == hash_table->len) 
        throw std::overflow_error ("hash table iterator overflow");

      return static_cast<Bucket*>(curr_itor.get_curr());
    }

/** 
  @brief Advances the iterator a bucket. 
*/
    void next() 
    {
      if (curr_index == hash_table->len) 
        throw std::overflow_error ("hash table iterator overflow");

      locate_next_available_bucket();
    }    

/** 
  @brief Backs the iterator a bucket.. 
*/
    void prev() 
    {
      if (curr_index == -1) 
        throw std::underflow_error ("hash table iterator underflow");

      locate_prev_available_bucket();
    }    

    Bucket * del()
    {
      Bucket * ret_val = get_curr();
      next();
      hash_table->remove_bucket(ret_val);
      return ret_val;
    }
  };

  HASH_STATS();
};

/** 
  @brief Bucket without virtual destructor for a resolution hash table
  separate chain collision.
  
  @param Key Key of search hash

  @see LhashTable

  @ingroup Hash
*/
    template <typename Key>
class LhashBucket : public Dnode<Key>
{
public:

/** 
  @brief Instance a bucket copy of another. 
*/
  LhashBucket(const LhashBucket & bucket) : Dnode<Key>(bucket) {}

/** 
  @brief Instance an empty bucket; undefined key value. 
*/
  LhashBucket() {}

/** 
  @brief Instance a bucket with "key" key. 
*/
  LhashBucket(const Key & key) : Dnode<Key>(key) {}

/** 
  @brief Returns the key indexing of the bucket. 
*/
  Key & get_key() { return this->get_data(); }
};

/** 
  @brief Bucket without virtual destructor for a resolution hash table
  separate chain collision.

  @param Key Key of search hash

  @see LhashTable

  @ingroup Hash
*/
    template <typename Key>
class LhashBucketVtl : public Dnode<Key>
{

public:
      
/** 
  @brief Instance a bucket copy of another. 
*/
  LhashBucketVtl(const LhashBucketVtl & bucket) : Dnode<Key>(bucket) {}

/** 
  @brief Instance an empty bucket; undefined key value. 
*/
  LhashBucketVtl() {}

/** 
  @brief Virtual destructor. 
*/
  virtual ~LhashBucketVtl() {}

/** 
  @brief Instance a bucket with "key" key. 
*/
  LhashBucketVtl(const Key & key) : Dnode<Key>(key) {}

/** 
  @brief Returns the key indexing of the bucket. 
*/
  Key & get_key() { return this->get_data(); }
};

/** 
  @brief Generic hash table with collision resolution by
  separate chaining and buckets without virtual destructor.

  @details The LhashTable type implements a generic hash table
  collision resolution by chaining separate in which
  buckets have no virtual destructor.

  @param Key Key of indexing of the table.
  @param Cmp Class of comparison between the keys; must return
  true if the keys are equal.

  @see GenLhashTable LhashTableVtl 

  @ingroup Hash
*/
    template <typename Key, class Cmp = Aleph::equal_to<Key> > 
class LhashTable : public GenLhashTable<Key, LhashBucket<Key>, Cmp>
{
public:

  typedef GenLhashTable<Key, LhashBucket<Key>, Cmp> Base;

  using Base::Base;
};

/** 
  @brief Generic hash table with collision resolution by
  separate chaining and buckets with virtual destructor.

  @details The LhashTableVtl type implements a generic hash table
  collision resolution by chaining separate in which
  buckets have no virtual destructor.

  @param Key Key of indexing of the table.
  @param Cmp Class of comparison between the keys; must return
  true if the keys are equal.

  @see GenLhashTable LhashTable 

  @ingroup Hash
*/
    template <typename Key, class Cmp = Aleph::equal_to<Key> > 
class LhashTableVtl : public GenLhashTable<Key, LhashBucketVtl<Key>, Cmp>
{
 public:

  typedef GenLhashTable<Key, LhashBucketVtl<Key>, Cmp> Base;

  using Base::Base;
};


} /* End namespace Aleph */

# ifdef NBACKUP
# define N NBACKUP
# undef NBACKUP
# endif

# ifdef MBACKUP
# define M MBACKUP
# undef MBACKUP
# endif

# endif/* TPL_LHASH_H */
