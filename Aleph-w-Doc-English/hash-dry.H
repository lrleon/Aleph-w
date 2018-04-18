
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
  @file hash-dry.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef HASH_DRY_H
# define HASH_DRY_H

# include <ahFunction.H>

# define HASH_STATS()							\
  public:								\
  struct Stats								\
  {									\
    float avg;								\
    float var;								\
    DynArray<size_t> lens;						\
  };									\
									\
private:								\
									\
 static void update_stat_len(DynArray<size_t> & lens, size_t i)		\
 {									\
   if (lens.exist(i))							\
     lens(i) += 1;							\
   else									\
     lens.touch(i) = 1;							\
 }									\
									\
public:									\
									\
 Stats stats() const							\
 {									\
   DynArray<size_t> lens;						\
									\
   for (int i = 0; i < capacity(); ++i)					\
     {									\
       BucketList & list = table[i];					\
       									\
       size_t count = 0;						\
       for (BucketItor it(list); it.has_curr(); it.next(), ++count)	\
	 ;								\
									\
       update_stat_len(lens, count);					\
     }									\
									\
   float avg = 0, sum = 0;						\
   for (int i = 0; i < lens.size(); ++i)				\
     {									\
       avg += lens(i)*i;						\
       sum += lens(i);							\
     }									\
   avg /= sum;								\
									\
   float var = 0;							\
   for (int i = 0; i < lens.size(); ++i)				\
     {									\
       float s = i - avg;						\
       var += lens(i)*s*s;						\
     }									\
   var /= sum;								\
									\
   Stats stats;								\
   stats.avg = avg;							\
   stats.var = var;							\
   std::swap(lens, stats.lens);						\
   									\
   return stats;							\
 }									\
									\
 void print_stats(const Stats & stats) const				\
 {									\
   cout << "M          = " << capacity() << endl			\
	<< "N          = " << size() << endl				\
	<< "busy slots = " << busy_slots_counter << endl		\
	<< "Average    = " << stats.avg << endl				\
	<< "Desv       = " << sqrt(stats.var) << endl			\
	<< "alpha      = " << (1.0*size())/capacity() << endl;		\
   for (int i = 0; i < stats.lens.size(); ++i)				\
     cout << "    " << i << " = " << stats.lens(i) << endl;		\
 }									\
									\
 void set_upper_alpha(const float &  __upper_alpha)			\
 {									\
   if (__upper_alpha <= lower_alpha)					\
     throw std::domain_error("upper_alpha lower than lower_alpha");	\
									\
   upper_alpha = __upper_alpha;						\
 }									\
									\
 void set_lower_alpha(const float &  __lower_alpha)			\
 {									\
   if (__lower_alpha >= upper_alpha)					\
     throw std::domain_error("lower_alpha greater than upper_alpha");	\
									\
   lower_alpha = __lower_alpha;						\
 }									\
									\
 float get_lower_alpha() const { return lower_alpha; }			\
									\
 float get_upper_alpha() const { return upper_alpha; }			\
									\
 Key & append(const Key & key)						\
 {									\
   Key * ret_val = insert(key);						\
   if (ret_val == NULL)							\
     throw std::domain_error("Duplicated key");				\
   return &ret_val;							\
 }									\
									\
 Key & append(Key && key)						\
 {									\
   Key * ret_val = insert((std::move(key)));				\
   if (ret_val == NULL)							\
     throw std::domain_error("Duplicated key");				\
   return &ret_val;							\
 }

# define OHASH_COMMON(class_name)					\
  private:								\
									\
  void copy_from_table(const class_name & other)			\
  {									\
    assert(N == 0 and len >= other.N);					\
    for (long i = 0, c = 0; c < other.N; ++i)				\
      {									\
	Bucket & bucket = other.table[i];				\
	if (bucket.status != BUSY)					\
	  continue;							\
									\
	insert(bucket.key);						\
	++c;								\
      }									\
    assert(N == other.N);						\
  }									\
									\
  void clean_table()							\
  {									\
    for (long i = 0; i < len; ++i)					\
      table[i].reset();							\
    N = 0;								\
  }									\
									\
public:									\
									\
 Key * insert(const Key & key)						\
 {									\
   Bucket * bucket = allocate_bucket(key);				\
									\
   if (bucket == NULL)							\
     return NULL;							\
									\
   bucket->key = key;							\
									\
   return test_resize(bucket, key);					\
 }									\
									\
 Key * insert(Key && key)						\
 {									\
   Bucket * bucket = allocate_bucket(key);				\
									\
   if (bucket == NULL)							\
     return NULL;							\
									\
   std::swap(bucket->key, key);						\
									\
   return test_resize(bucket, bucket->key);				\
 }									\
									\
 bool has(const Key & key) const { return search(key) != NULL; }	\
									\
 bool contains(const Key & key) const { return has(key); }		\
									\
 Key & find(const Key & key)						\
 {									\
   Key * key_ptr = search(key);						\
 									\
   if (key_ptr == NULL)							\
     throw std::domain_error("Key not found in hash");			\
 									\
   return *key_ptr;							\
 }									\
									\
 void remove(Key * key)							\
 {									\
   Bucket * bucket = key_to_bucket(key);				\
   deallocate_bucket(bucket);						\
									\
   if (with_resize and current_alpha() < lower_alpha)			\
     resize(Primes::next_prime(len/2 + 1));				\
 }									\
									\
private:								\
									\
  Key * test_resize(Bucket * curr_bucket, const Key & key)		\
  {									\
    if (not ((with_resize and current_alpha() >= upper_alpha)))		\
      return &curr_bucket->key;						\
									\
    Key key_copy = key;							\
    resize(Primes::next_prime(2*len));					\
									\
    Key * key_ptr = search(key_copy);					\
    assert(key_ptr != NULL);							\
    assert(*key_ptr == key_copy);						\
									\
    return key_ptr;							\
  }									\
									\
public:									\
									\
 size_t resize(const size_t & new_size)					\
 {									\
   assert(len > 0);								\
									\
   if (new_size == 0 or new_size == len)				\
     return len;							\
									\
   if (N > new_size)							\
     throw std::overflow_error ("New size is not enough for current"	\
				" number of entries");			\
									\
   Bucket * new_table   = new Bucket [new_size];			\
   Bucket * old_table   = table;					\
   const size_t old_len = len;						\
									\
   int old_N = N;							\
									\
   table = new_table;							\
   len   = new_size;							\
   N     = 0;								\
									\
   for (int i = 0, c = 0; i < old_len and c < old_N; ++i)		\
     if (old_table[i].status == BUSY)					\
       {								\
	 Key & key = old_table[i].key;					\
	 Bucket * bucket = allocate_bucket(key);			\
	 std::swap(bucket->key, key);					\
	 ++c;								\
       }								\
									\
   assert(old_N == N);							\
									\
   delete [] old_table;							\
									\
   return len;								\
 }									\
									\
 void rehash()								\
 {									\
   Bucket * new_table = new Bucket [len];				\
   Bucket * old_table = table;						\
									\
   int old_N = N;							\
									\
   table = new_table;							\
   N     = 0;								\
									\
   for (int i = 0, c = 0; i < len and c < old_N; ++i)			\
     if (old_table[i].status == BUSY)					\
       {								\
	 Key & key = old_table[i].key;					\
	 Bucket * bucket = allocate_bucket(key);			\
	 std::swap(bucket->key, key);					\
	 ++c;								\
       }								\
									\
   assert(old_N == N);							\
									\
   delete [] old_table;							\
 }									\
 									\
 void empty()								\
 {									\
   delete [] table;							\
   N = 0;								\
   len = Primes::DefaultPrime;						\
   table = new Bucket [len];						\
 }									\
 									\
const size_t & size() const { return N; }				\
									\
 bool is_empty() const { return N == 0; }				\
									\
 const size_t & capacity() const { return len; }			\
									\
 class Iterator								\
 {									\
   class_name * table_ptr = NULL;					\
   long curr_idx          = 0;						\
   long ordinal           = 0;						\
									\
   bool check() const							\
   {									\
     assert(table_ptr != NULL);						\
     return (curr_idx >= 0 and curr_idx < (long) table_ptr->len) and	\
       (ordinal >= -1 and ordinal <= (long) table_ptr->size());		\
   }									\
									\
   void locate_next_available_entry()					\
   {									\
     assert(check());							\
									\
     if (ordinal >= table_ptr->size())					\
       throw std::overflow_error("class_name::Iterator next() overflow"); \
									\
     if (++ordinal == table_ptr->size())				\
       return;								\
									\
     while (table_ptr->table[++curr_idx].status != BUSY) ;		\
   }									\
									\
   void locate_prev_available_entry()					\
   {									\
     assert(check());							\
									\
     if (ordinal <= -1)							\
       throw std::underflow_error("class_name::Iterator next() underflow"); \
    									\
     if (--ordinal == -1)						\
       return;								\
    									\
     while (table_ptr->table[--curr_idx].status != BUSY) ;		\
   }									\
									\
 public:								\
									\
   void reset_first()							\
   {									\
     assert(table_ptr != NULL);						\
     if (table_ptr->is_empty())						\
       {								\
	 curr_idx = table_ptr->len;					\
	 ordinal = 0;							\
	 return;							\
       }								\
									\
     while (table_ptr->table[curr_idx].status != BUSY)			\
       ++curr_idx;							\
									\
     ordinal = 0;							\
     assert(check());							\
   }									\
									\
   void reset_last()							\
   {									\
     assert(table_ptr != NULL);						\
     if (table_ptr->is_empty())						\
       {								\
	 curr_idx = -1;							\
	 ordinal = -1;							\
	 return;							\
       }								\
									\
     curr_idx = table_ptr->len - 1;					\
     while (table_ptr->table[curr_idx].status != BUSY)			\
       --curr_idx;							\
									\
     ordinal = table_ptr->size() - 1;					\
     assert(check());							\
   }									\
									\
   Key & get_curr() const						\
   {									\
     assert(check());							\
   									\
     if (ordinal == table_ptr->size())					\
       throw std::overflow_error("O hash::Iterator next() overflow");	\
									\
     if (ordinal == -1)							\
       throw std::underflow_error("O hash::Iterator next() underflow"); \
 									\
     assert(table_ptr->table[curr_idx].status == BUSY);			\
									\
     return table_ptr->table[curr_idx].key;				\
   }									\
									\
   const Key & get_current() const { return get_curr(); }		\
									\
   bool has_curr() const						\
   {									\
     assert(table_ptr != NULL);						\
     if (table_ptr->is_empty())						\
       return false;							\
     return ordinal >= 0 and ordinal < table_ptr->size();		\
   }									\
									\
   bool has_current() const { return has_curr(); }			\
									\
   void next() { locate_next_available_entry(); }			\
									\
   void prev() { locate_prev_available_entry(); }			\
									\
   Iterator(const class_name & table)					\
     : table_ptr(&const_cast<class_name&>(table)), curr_idx(0), ordinal(-1) \
     {									\
       reset_first();							\
     }									\
									\
   void del()								\
   {									\
     if (not has_curr())						\
       throw std::overflow_error("Overflow in del() of iterator");	\
									\
     table_ptr->deallocate_bucket(&table_ptr->table[curr_idx]);		\
     if (table_ptr->size() == 0)					\
       return;								\
									\
     while (table_ptr->table[++curr_idx].status != BUSY) ;		\
   }									\
 };									\
									\
 template <template <typename> class Container = DynList>		\
 Container<Key> keys() const						\
 {									\
   return this->template map<Key, Container>				\
		    ([] (const Key & key) { return key; });		\
 }									\
									\
 struct Stats								\
 {									\
   size_t num_busy;							\
   size_t num_deleted;							\
   size_t num_empty;							\
   DynArray<size_t> lens;						\
   float avg, var;							\
   size_t max_len;							\
									\
   Stats() : max_len(std::numeric_limits<size_t>::min())		\
     {									\
       assert(lens.size() == 0);						\
     }									\
 };									\
									\
private:								\
									\
 static void update_stat_len(DynArray<size_t> & lens, size_t i)		\
 {									\
   if (lens.exist(i))							\
     lens(i) += 1;							\
   else									\
     lens.touch(i) = 1;							\
 }									\
									\
public:									\
									\
 void print_stats(const Stats & stats) const				\
 {									\
   std::cout << "M             = " << capacity() << std::endl		\
	     << "N             = " << size() << std::endl		\
	     << "busy slots    = " << stats.num_busy << std::endl	\
	     << "deleted slots = " << stats.num_deleted << std::endl	\
	     << "empty slots   = " << stats.num_empty << std::endl	\
	     << "alpha         = " << current_alpha() << std::endl	\
	     << "max lenght    = " << stats.max_len << std::endl;	\
   for (int i = 0; i < stats.lens.size(); ++i)				\
     cout << "    " << i << " = " << stats.lens(i) << endl;		\
 }									\
									\
 Hash_Fct get_hash_fct() const { return hash_fct; }			\
									\
  void set_hash_fct(Hash_Fct fct)					\
  {									\
    hash_fct = fct;							\
  }									\
									\
  float current_alpha() const { return 1.0*N/len; }

namespace Aleph
{

  extern const float hash_default_lower_alpha;

  extern const float hash_default_upper_alpha;

}


# endif // HASH_DRY_H
