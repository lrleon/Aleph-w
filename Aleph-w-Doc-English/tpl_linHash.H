
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
  @file tpl_linHash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/
  

# ifndef TPL_LINHASH_H
# define TPL_LINHASH_H

# include <iostream>
# include <primes.H>
# include <tpl_dynArray.H>
# include <tpl_dnode.H>
# include <hash-fct.H>
# include <hash-dry.H>


# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif


using namespace Aleph;

namespace Aleph {

# define LINBUCKET_BODY(BUCKETNAME)				\
								\
  Dlink link;							\
								\
public:								\
								\
 BUCKETNAME(const BUCKETNAME & bucket) : Dnode<Key>(bucket) {}	\
								\
 BUCKETNAME() {}						\
								\
 BUCKETNAME(const Key & key) : Dnode<Key>(key) {}		\
								\
 Key & get_key() { return this->get_data(); }			\
								\
 Dlink * get_link() { return &link; }				\
								\
 DLINK_TO_BASE(BUCKETNAME, link);

 
/** 
  @brief Bucket without virtual destructor for a resolution hash table
  separate chain collision.

  @param Key hash key search
  
  @see LhashTable
  
  @ingroup Hash
*/
    template <typename Key>
class LinHashBucket : public Dnode<Key>
{
  LINBUCKET_BODY(LinHashBucket);
};

/** 
  @brief Bucket without virtual destructor for a resolution hash table
  separate chain collision.

  @param Key hash key search
  
  @see LhashTable
  
  @ingroup Hash
*/
    template <typename Key>
class LinHashBucketVtl : public Dnode<Key>
{
  LINBUCKET_BODY(LinHashBucketVtl);

/** @brief Virtual destructor. */
  virtual ~LinHashBucketVtl() {}
};

 

/**  
  @brief Generic linear hashtable.

  @details Basically, a linear hash table is a hash table
  Collision resolution by chaining separate but with the
  Unlike the table size increases dynamically
  to ensure that the load factor, typically called
  \ f $ \ alpha \ f $, is bounded between two lower values
  higher, respectively.

  Internally, the table uses a dynamic array type
  dynArray This implies savings of memory entries
  table that have not been written.

  This type is generic and is not intended to be used. Instead,
  Use LinearHashTable or LinearHashTableVtl as desired
  use buckets with or without virtual destructor.

  @param Key Key type for which the table is indexed.
  @param BucketType Tipo de cubeta entre LinHashBucket o
  LinHashBucketVtl. 
  @param Cmp class of comparison between the keys.

  @see DynArray LinearHashTable LinearHashTableVtl

  @ingroup Hash
*/
    template <typename Key, template <class> class BucketType, 
              class Cmp = Aleph::equal_to<Key>>
class GenLinearHashTable 
{
public:

/** 
  @brief The type of hash function. 

  @return Positive values should return pseudo
  random, between zero and the largest number possible. */
  typedef size_t (*Hash_Fct)(const Key &);

/** @brief Bucket type. */
  typedef BucketType<Key> Bucket;

  typedef Key Key_Type;

  typedef Key Item_Type;

private:

  typedef Dnode<Key> BucketList;

  typedef typename Dnode<Key>::Iterator BucketItor;

  static size_t multiply_by_two(const size_t & n) { return n << 1; }

  static size_t divide_by_two(const size_t & n) { return n >> 1; }

  DynArray<BucketList> table;
  Dlink                entries_list;

protected:

  Hash_Fct             hash_fct; /* Hash function pointer*/

private:

  Cmp                  cmp = Cmp();

  size_t               M; /* table size */
  size_t               N; /* Number of elements on table */
  size_t               busy_slots_counter; /* Number of occupied posts */
  bool                 remove_all_buckets; /* Indicates whether destructor should
                                           release cuvettes*/

protected:

  float upper_alpha; /* Top/Uper Load Factor */
  float lower_alpha; /* Lower load factor */

private:

  size_t p; /* List index that is partitioned (or increases) */
  size_t l; /* Number of times the table has doubled */
  size_t MP; /* Saves the value  p + M */
  size_t MM; /* Result of 2*M */

protected:

  mutable size_t len;

private:

  size_t call_hash_fct(const Key & key) const
  {
    const size_t hash = (*hash_fct)(key);
    const size_t i = hash % M;
    return i < p ? hash % MM : i;
  }

  void expand()
  { 
    /** Expand the table until the load is below upper_alpha */
    for (float alpha = 1.0*N/MP; alpha >= upper_alpha; alpha = 1.0*N/MP)
      {
        BucketList * src_list_ptr = table.test(p);
        if (src_list_ptr != NULL) /* Is Table[p] wrote? */
          if (not src_list_ptr->is_empty()) /* is not empty Table[p]? */
            {
              BucketList * tgt_list_ptr = NULL; 

    /** Browse list and moving buckets collisions de table[p+M] */
              for (BucketItor it(*src_list_ptr); it.has_current(); /* Nothing */)
                {
                  Bucket * bucket = static_cast<Bucket*>(it.get_current());

                  it.next(); /* Advance to the next item in the list */

                  const Key & key = bucket->get_key();
                  const int i = (*hash_fct)(key) % MM;
                  if (i == p) /* This key belong to table[p]?
                              Yes? ==> key keeps in table[p] ==> next */
                  continue;

                  if (tgt_list_ptr == NULL)
                    tgt_list_ptr = &table.touch(MP);

    /* Bucket don't belong table[p], instead belong to table[p+m] ==>
    Remove bucket from table[i] and insert in table[p+m] */
                  bucket->del();
                  tgt_list_ptr->append(bucket);
                }

              if (src_list_ptr->is_empty()) /* Is empty Table[p]?
                                            Yes? ==> a empty slot*/
                --busy_slots_counter;

              ++busy_slots_counter; /* A new for table[p+M] */
            } 
        ++p;
        ++MP;

    /** (p == 2*M) Should doubled the size of the table?
    If the answer is YES, then resize the table to 2*M */
        if (p == M) 
                    
          {
            ++l;   /* Number of times the table has doubled */
            p = 0;
            MP = M = MM; /* Asign 2*M */
            MM = multiply_by_two(MM);
          }
      } 
  }

  void contract()
  { 
    /** Collapse the table until load is below lower_alpha */
    for (float alpha = (1.0*N)/MP; alpha <= lower_alpha and MP > len; 
         alpha = (1.0*N)/MP)
      {
    /** Must be divided into two size table?
    If the answer is YES, then update the table size M/2 */
        if (p == 0) 
          {   
            --l; /* Number of times the table has doubled decreases */
            MM = M; /* Divide between two */
            M = divide_by_two(M);
            p = M - 1;
          }
    /** If the answer is NO, then only reduce index p */
        else
          --p;

        --MP;
        if (MP < table.size()) /* Exist table[MP]]? */
          {
            BucketList * src_list_ptr = table.test(MP);
            if (src_list_ptr != NULL) /* ¿Existe entrada para table[p+M]? */
              {
    /** ¿Table[p+M] it's empty?
    If the answer is NO, then merge the lists */
                if (not src_list_ptr->is_empty()) 
                  {     
                    BucketList & tgt_list = table.touch(p);/* Apart table[p] */
                    tgt_list.concat_list(src_list_ptr);
                    --busy_slots_counter; /* Table[p+M]  cames empty */
                  }
                table.cut(MP); /* Eventually free memory table[p+M] */
              }
          }
      }
  }

public:

/** @brief Set the internal hash function */
  void set_hash_fct(Hash_Fct fct)
  {
    hash_fct = fct;
  }

  Hash_Fct get_hash_fct() const { return hash_fct; }

  const Cmp & get_compare() const { return cmp; }

/** 
  @brief Return the current table load 
*/
  float current_alpha() const { return 1.0*N/MP; }

/** 
  @brief Instance a generic linear hash table.

  Instancedetails a generic linear __len hash table size.

  @param [in] __hash_fct Hash function.
  @param [in] __len initial and minimum size of the table.
  @param [in] __upper_alpha Upper threshold in the table should
  expand.
  @param [in] __lower_alpha lower threshold in the table should contract.
  @param [in] __remove_all_buckets If true, then released
  the cuvettes of the table. Otherwise, they remain
  intact. The default value is true.

  @throw length_error If __ is higher than the maximum
  allowable dimension for a dynamic array.
  @throw domain_error If __upper_alpha is less than or equal to __lower_alpha.
  @throw bad_alloc If there is Insufficient Memory
  @throw overflow_error If an overflow occurs from DYNARRAY
  (caused by internal calculations).
  */
  GenLinearHashTable(Hash_Fct __hash_fct          = Aleph::dft_hash_fct<Key>,
		     const size_t & __len         = Primes::DefaultPrime, 
                     const float &  __lower_alpha = hash_default_lower_alpha,
                     const float &  __upper_alpha = hash_default_upper_alpha,
		     const bool     __remove_all_buckets = true,
		     const bool     __with_resize = true/* Fictitious */)
    throw(std::exception, std::length_error, std::domain_error,
          std::bad_alloc, std::overflow_error) 
    : table(__len), hash_fct(__hash_fct), M(__len), N(0), 
      busy_slots_counter(0), remove_all_buckets(__remove_all_buckets), 
      upper_alpha(__upper_alpha), lower_alpha(__lower_alpha),
      p(0), l(0), MP(M), MM(multiply_by_two(M)), len(__len)
  {
    if (M == 0)
      std::length_error("table's length is zero");

    if (MM > table.max_size())
      throw std::length_error("table's length too big");
      
    if (upper_alpha <= lower_alpha)
      throw std::domain_error("lower alpha is greater than lower alpha");
  }

  void swap(GenLinearHashTable & other)
  {
    std::swap(table, other.table);
    std::swap(hash_fct, other.hash_fct);
    std::swap(M, other.M);
    std::swap(N, other.N);
    std::swap(busy_slots_counter, other.busy_slots_counter);
    std::swap(remove_all_buckets, other.remove_all_buckets);
    std::swap(upper_alpha, other.upper_alpha);
    std::swap(lower_alpha, other.lower_alpha);
    std::swap(p, other.p);
    std::swap(l, other.l);
    std::swap(MP, other.MP);
    std::swap(MM, other.MM);
    std::swap(len, other.len);
  }

/** 
  @fn set_upper_alpha

  @details  Change the upper thresholddetails load the table starts
  expand. Shoot domain_error if the new threshold is lower
  the lower threshold (lower_alpha)
*/

/** 
  @fn set_lower_alpha

  @details Changing the lower load threshold that starts the table
  contract. Shoot domain_error if the new threshold is higher
  the upper threshold (upper_alpha)
*/

/** 
  @brief Empty the entire table.

  @details All buckets are released and size
  It is set to initial.
*/
  void empty()
  {
    while (not entries_list.is_empty())
      {    
	Bucket * bucket = Bucket::dlink_to_base(entries_list.remove_first());
	bucket->del();             /* Deleted from the list in accordance */
	bucket->get_link()->del(); /* Entries deleted from list */
	delete bucket;
      }
    
    M = MP = len;
    MM = multiply_by_two(M);
    N = p = l = 0;
    table.cut(len); 
  }

  ~GenLinearHashTable()
  {
    if (remove_all_buckets)
      empty();
  }

private:

  Bucket * search_in_bucket_list(BucketList * list, const Key & key) const
  {
    for (BucketItor it(*list); it.has_curr(); it.next())
      {
        Bucket * bucket = static_cast<Bucket*>(it.get_current());
        if (cmp (key, bucket->get_key())) 
          return bucket; 
      }

    return NULL;
  }

public:


/**
  @brief Key Search key on the linear hash table. Returns a pointer to
  cuvette containing the key if it is on the table;
  NULL otherwise. 
*/
  Bucket * search(const Key & key) const
  {
    const int i = call_hash_fct(key);
    BucketList * list = table.test(i);

    /** Have you ever been written table [i]?
    If the answer is NO, then the element is not in the tabla */
    if (list == NULL) 
                      
      return NULL;

    if (list->is_empty()) 
      return NULL;

    return search_in_bucket_list(list, key);
  }

/** @brief Returns the number of elements contained in the table. */
  const size_t & size() const { return N; }

/** @brief Return true is table is empty */
  bool is_empty() const { return N == 0; }

/** @brief Returns the size of the table. */
  const size_t & capacity() const { return MP; }

/** @brief Returns the number of empty entrances to the table */
  const size_t & busy_slots() const { return busy_slots_counter; }

/** @brief Returns the level of expansions that have been done on the table. */
  const size_t & expansions() const { return l; }

/** 
  @brief Insert bucket in the table. 

  @return Return bucket if bucket->get_key()
  is not in the table, NULL otherwise 
*/
  Bucket * insert(Bucket * bucket)
  {
    const int i       = call_hash_fct(bucket->get_key());
    BucketList & list = table.touch(i); /* Allocates memory for table[i] */
    if (list.is_empty())
      ++busy_slots_counter;

    if (search_in_bucket_list(&list, bucket->get_key()) != NULL)
      return NULL; /* Duplicated key */

    list.append(bucket);
    entries_list.append(bucket->get_link());
    ++N;
    expand();

    return bucket;
  }

/** @brief Provided for generic programming compatibility */
  size_t resize(size_t) { return MP; }

private:

    /* This routine deletes bucket from hash table EXCEPT from
    entries_list. The end of this routine is to dry the deletion and to
    allow remove from other places; ofor instance, from the del()
    method of Iterator class */
  Bucket * remove_bucket(Bucket * bucket)
  {
    assert(bucket != NULL);
    assert(search(bucket->get_key()) == bucket);

    Bucket * next = static_cast<Bucket*>(bucket->get_next());

    bucket->del();             /* List eliminate collisions */

    /** Ready empty collisions?
    If the answer is YES, then it is an empty slot */
    if (next->is_empty())
      --busy_slots_counter; 

    --N;
    contract();

    return bucket;
  }

public:

/** 
  @brief Eliminates the bucket "bucket".

  @note Attention: there is verified if the bucket belongs to the table.
*/
  Bucket * remove(Bucket * bucket)
  {
    bucket->get_link()->del(); /* Elimine de entries_list */
    return remove_bucket(bucket);
  }

  void print()
  {
    for (int i = 0; i < MP; ++i)
      {
        cout << "table[" << i << "] = [ ";
        
        if (table.exist(i))
          {
            BucketList & list = table.access(i);

            if (not list.is_empty())
              for (BucketItor it(list); it.has_current(); it.next())
                {
                  Bucket * bucket = static_cast<Bucket*>(it.get_current());
                  const Key & key = bucket->get_key();
                  cout << key << ",";
                }
          }
        cout << "]" << endl;
      }
  }

  HASH_STATS();

  class Iterator : public Dlink::Iterator
  {
  private:

    GenLinearHashTable * hash_table;

  public:

/** @brief The set type on which iterates. */
    typedef GenLinearHashTable Set_Type;

/** @brief the type of element that returns get_curr() */
    typedef Bucket * Item_Type;

/** @brief An iterator instance hash table on the table. */
    Iterator(const GenLinearHashTable & table) 
      : Dlink::Iterator(const_cast<Dlink &>(table.entries_list)), 
	hash_table(& const_cast<GenLinearHashTable &>(table))
    {
      /* Empty */
    }
    
/** @brief An empty iterator instance. */
    Iterator() : hash_table(NULL)
    {
      /* Empty */  
    }

/** @brief Returns the current bucket. */
    Bucket * get_curr() 
    {
      return Bucket::dlink_to_base(this->Dlink::Iterator::get_curr());
    }

    Bucket * del()
    {
      Bucket * bucket = Bucket::dlink_to_base(this->Dlink::Iterator::del());
      return (Bucket *) hash_table->remove_bucket(bucket);
    }
  };
};


/** 
  @brief linear hash table with buckets without virtual destructor.

  @details Basically, a linear hash table is a hash table
  Collision resolution by chaining separate but with the
  Unlike the table size increases dynamically
  to ensure that the load factor, typically called
  \ f $ \ alpha \ f $, is bounded between two lower values
  higher, respectively.

  Internally, the table uses a dynamic array type
  dynArray. This implies savings of memory entries
  table that have not been written.

  @param Key Key type for which the table is indexed.
  @param Cmp Class of comparison between the keys.

  @see DynArray LinearHashTableVtl

  @ingroup Hash
*/
    template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct LinearHashTable : public GenLinearHashTable<Key, LinHashBucket, Cmp>
{
  typedef GenLinearHashTable<Key, LinHashBucket, Cmp> Base;

  using Base::Base;
};

/** 
  @brief Linear hash table with virtual destructor in their buckets.

  @details Basically, a linear hash table is a hash table
  Collision resolution by chaining separate but with the
  Unlike the table size increases dynamically
  to ensure that the load factor, typically called
  \ f $ \ alpha \ f $, is bounded between two lower values
  higher, respectively.

  Internally, the table uses a dynamic array type
  DYNARRAY. This implies savings of memory entries
  table that have not been written.

  @param Key Key type for which the table is indexed.
  @param Cmp Class of comparison between the keys.

  @see DynArray LinearHashTable 
  
  @ingroup Hash
*/
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct LinearHashTableVtl 
  : public GenLinearHashTable<Key, LinHashBucketVtl, Cmp>
{
  typedef GenLinearHashTable<Key, LinHashBucketVtl, Cmp> Base;

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

# endif // TPL_LINHASH_H
