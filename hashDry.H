
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

# ifndef HASHDRY_H
# define HASHDRY_H

# include <aleph.H>
# include <tpl_dynArray.H>

template <class HashTbl, typename Key>
class OhashCommon
{
  HashTbl * me() { return static_cast<HashTbl*>(this); }

  const HashTbl * const_me() const { return static_cast<const HashTbl*>(this); }

public:

  void copy_from_table(const HashTbl & other)
  {
    assert(me()->N == 0 and me()->len >= other.N);
    for (long i = 0, c = 0; c < other.N; ++i)
      {
	auto & bucket = other.table[i];
	if (bucket.status != HashTbl::BUSY)
	  continue;

	me()->insert(bucket.key);
	++c;
      }
    assert(me()->N == other.N);
  }

  void clean_table()
  {
    for (long i = 0; i < me()->len; ++i)
      me()->table[i].reset();
    me()->N = 0;
  }

  Key * insert(const Key & key)
  {
    auto bucket = me()->allocate_bucket(key);
    if (bucket == nullptr)
     return nullptr;

   bucket->key = key;

   return me()->test_resize(bucket, key);
 }

 Key * insert(Key && key)
 {
   auto bucket = me()->allocate_bucket(key);
   if (bucket == nullptr)
     return nullptr;

   std::swap(bucket->key, key);

   return me()->test_resize(bucket, bucket->key);
 }

  Key * search_or_insert(const Key & key)
  {
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (get<1>(r))
      return &bucket->key;

    bucket->key = key;
    
    return me()->test_resize(bucket, bucket->key);
  }

  Key * search_or_insert(Key && key)
  {
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (get<1>(r))
      return &bucket->key;

    std::swap(bucket->key, key);

    return me()->test_resize(bucket, bucket->key);
  }

  bool contains_or_insert(const Key & key)
  {
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (get<1>(r))
      return true;

    bucket->key = key;
    me()->test_resize(bucket, bucket->key);

    return false;
  }

 bool contains_or_insert(Key && key)
 {
   auto r = me()->hard_allocate_bucket(key);
   auto bucket = get<0>(r);
   if (get<1>(r))
     return true;

   std::swap(bucket->key, key);
   me()->test_resize(bucket, bucket->key);

   return false;
 }

  Key * append(const Key & key)
  {
    return this->insert(key);
  }

  Key * append(Key && key)
  {
    return this->insert(std::forward<Key>(key));
  }

  bool has(const Key & key) const noexcept
  {
    return const_me()->search(key) != nullptr;
  }

  bool contains(const Key & key) const noexcept { return has(key); }

  Key & find(const Key & key)
  {
    auto key_ptr = me()->search(key);
    if (key_ptr == nullptr)
      throw std::domain_error("Key not found in hash");

    return *key_ptr;
  }

  const Key & find(const Key & key) const
  {
    return const_cast<OhashCommon*>(this)->find(key);
  }

  const Key & operator [] (const Key & key) const
  {
    return find(key);
  }

  const Key & operator [] (const Key & key)
  {
    return *search_or_insert(key);
  }

  void remove_ptr(Key * key)
  {
    auto bucket = HashTbl::key_to_bucket(key);
    me()->deallocate_bucket(bucket);

    if (me()->with_resize and me()->current_alpha() < me()->lower_alpha)
      resize(Primes::next_prime(me()->len/2 + 1));
  }

  size_t resize(const size_t & new_size)
  {
    assert(me()->len > 0);

    if (new_size == 0 or new_size == me()->len)
      return me()->len;

    if (me()->N > new_size)
      throw std::overflow_error ("New size is not enough for current" 
				 " number of entries"); 

    typename HashTbl::Bucket * new_table = new typename HashTbl::Bucket[new_size];
    typename HashTbl::Bucket * old_table = me()->table;
    const size_t old_len = me()->len;

    int old_N = me()->N;

    me()->table = new_table;
    me()->len   = new_size;
    me()->N     = 0;

    for (int i = 0, c = 0; i < old_len and c < old_N; ++i)
      if (old_table[i].status == HashTbl::BUSY)
	{
	  Key & key = old_table[i].key;
	  auto bucket = me()->allocate_bucket(key);
	  std::swap(bucket->key, key);
	  ++c;
	}

    assert(old_N == me()->N);

    delete [] old_table;

    return me()->len;
  }

  void rehash()
  {
    auto new_table = new typename HashTbl::Bucket [me()->len];
    auto old_table = me()->table;

    int old_N = me()->N;

    me()->table = new_table;
    me()->N     = 0;

    for (int i = 0, c = 0; i < me()->len and c < old_N; ++i)
      if (old_table[i].status == HashTbl::BUSY)
	{
	  Key & key = old_table[i].key;
	  auto bucket = me()->allocate_bucket(key);
	  std::swap(bucket->key, key);
	  ++c;
	}

    assert(old_N == me()->N);

    delete [] old_table;
  }

  void empty()
  {
    delete [] me()->table;
    me()->N = 0;
    me()->len = Primes::DefaultPrime;
    me()->table = new typename HashTbl::Bucket [me()->len];
  }

  size_t size() const noexcept { return const_me()->N; }

  bool is_empty() const noexcept { return const_me()->N == 0; }

  size_t capacity() const noexcept { return const_me()->len; }

  class Iterator
  {
    HashTbl * table_ptr = nullptr;
    long curr_idx          = 0;
    long ordinal           = 0;

    bool check() const noexcept
    {
      assert(table_ptr != nullptr);
      return (curr_idx >= 0 and curr_idx < (long) table_ptr->len) and
	(ordinal >= -1 and ordinal <= (long) table_ptr->size());
    }

    void locate_next_available_entry_ne() noexcept
    {
      assert(check());
      if (++ordinal == table_ptr->size())
	return;

      while (table_ptr->table[++curr_idx].status != HashTbl::BUSY);
    }

    void locate_prev_available_entry_ne()
    {
      assert(check());
      if (--ordinal == -1)
	return;

      while (table_ptr->table[--curr_idx].status != HashTbl::BUSY);
    }

    void locate_next_available_entry()
    {
      if (ordinal >= table_ptr->size())
	throw std::overflow_error("Iterator next() overflow"); 
      locate_next_available_entry_ne();
    }

    void locate_prev_available_entry()
    {
      if (ordinal <= -1)
	throw std::underflow_error("Iterator next() underflow");
      locate_prev_available_entry_ne();
    }

  public:

    void reset_first() noexcept
    {
      assert(table_ptr != nullptr);
      curr_idx = 0;
      ordinal = -1;
      if (table_ptr->is_empty())
	{
	  curr_idx = table_ptr->len;
	  ordinal = 0;
	  return;
	}

      while (table_ptr->table[curr_idx].status != HashTbl::BUSY)
	++curr_idx;

      ordinal = 0;
      assert(check());
    }

    void reset_last() noexcept
    {
      assert(table_ptr != nullptr);
      if (table_ptr->is_empty())
	{
	  curr_idx = -1;
	  ordinal = -1;
	  return;
	}

      curr_idx = table_ptr->len - 1;
      while (table_ptr->table[curr_idx].status != HashTbl::BUSY)
	--curr_idx;

      ordinal = table_ptr->size() - 1;
      assert(check());
    }

    void end() noexcept
    {
      put_itor_at_the_end(*this);
    }

    long get_pos() const noexcept { return ordinal; }

    Key & get_curr_ne() noexcept
    {
      assert(table_ptr->table[curr_idx].status == HashTbl::BUSY);
      return table_ptr->table[curr_idx].key;
    }

    const Key & get_curr_ne() const noexcept
    {
      return const_cast<Iterator*>(this)->get_curr_ne();
    }

    Key & get_curr()
    {
      if (ordinal == table_ptr->size())
	throw std::overflow_error("O hash::Iterator next() overflow");

      if (ordinal == -1)
	throw std::underflow_error("O hash::Iterator next() underflow");

      assert(check());

      return get_curr_ne();
    }

    const Key & get_curr() const
    {
      return const_cast<Iterator*>(this)->get_curr();
    }

    bool has_curr() const noexcept
    {
      assert(table_ptr != nullptr);
      if (table_ptr->is_empty())
	return false;
      return ordinal >= 0 and ordinal < table_ptr->size();
    }

    bool is_last() const noexcept { return ordinal == table_ptr->size() - 1; }

    void next() { locate_next_available_entry(); }

    void next_ne() noexcept { locate_next_available_entry_ne(); }

    void prev_ne() { locate_prev_available_entry_ne(); }

    void prev() { locate_prev_available_entry(); }

    Iterator(const HashTbl & table) noexcept
      : table_ptr(&const_cast<HashTbl&>(table)), curr_idx(0), ordinal(-1)
    {
      reset_first();
    }

    void del()
    {
      if (not has_curr())
	throw std::overflow_error("Overflow in del() of iterator");

      table_ptr->deallocate_bucket(&table_ptr->table[curr_idx]);
      if (table_ptr->size() == 0)
	return;

      while (table_ptr->table[++curr_idx].status != HashTbl::BUSY);
    }
  };

  DynList<Key> keys() const
  {
    return const_me()->template maps<Key> ([] (const Key & key) { return key; });
  }

  auto items() const { return keys(); }

  struct Stats
  {
    size_t num_busy = 0;
    size_t num_deleted = 0;
    size_t num_empty = 0;
    DynArray<size_t> lens;
    float avg = 0, var = 0;
    size_t max_len = 0;

    Stats() : max_len(std::numeric_limits<size_t>::min())
      {
	assert(lens.size() == 0);
      }
  };

  void print_stats(const Stats & stats) const
  {
    std::cout << "M             = " << this->capacity() << std::endl
	      << "N             = " << this->size() << std::endl
	      << "busy slots    = " << stats.num_busy << std::endl
	      << "deleted slots = " << stats.num_deleted << std::endl
	      << "empty slots   = " << stats.num_empty << std::endl
	      << "alpha         = " << const_me()->current_alpha() << std::endl
	      << "max lenght    = " << stats.max_len << std::endl;
    for (int i = 0; i < stats.lens.size(); ++i)
      cout << "    " << i << " = " << stats.lens(i) << endl;
  }

  float current_alpha() const noexcept 
  {
    return 1.0*const_me()->N/const_me()->len; 
  }
};


template <class HashTbl>
class HashStats
{
  HashTbl * me() { return static_cast<HashTbl*>(this); }

  const HashTbl * const_me() const { return static_cast<const HashTbl*>(this); }

  float  lower_alpha;
  float  upper_alpha;
  size_t busy_slots_counter = 0;
  size_t N = 0;

public:

  struct Stats
  {
    float avg;
    float var;
    DynArray<size_t> lens;
  };

private:

  static void update_stat_len(DynArray<size_t> & lens, size_t i)
  {
    if (lens.exist(i))
      lens(i) += 1;
    else
      lens.touch(i) = 1;
  }
  
public:

  Stats stats() const
  { 
    DynArray<size_t> lens;
    for (int i = 0; i < const_me()->capacity(); ++i)
      {
	Dlink & list = const_me()->table[i];
	size_t count = 0;
	for (typename HashTbl::BucketItor it(list); it.has_curr(); 
	     it.next(), ++count)
	  ;

	update_stat_len(lens, count);
      }

    float avg = 0, sum = 0;
    for (size_t i = 0; i < lens.size(); ++i)
      {
	avg += lens(i)*i;
	sum += lens(i);
      }
    avg /= sum;

    float var = 0;
    for (size_t i = 0; i < lens.size(); ++i)
      {
	float s = i - avg;
	var += lens(i)*s*s;
      }
    var /= sum;

    Stats stats;
    stats.avg = avg;
    stats.var = var;
    std::swap(lens, stats.lens);

    return stats;
  }

  void print_stats(const Stats & stats) const
  {
    cout << "M          = " << const_me()->capacity() << endl
	 << "N          = " << const_me()->size() << endl
	 << "busy slots = " << busy_slots_counter << endl
	 << "Average    = " << stats.avg << endl
	 << "Desv       = " << sqrt(stats.var) << endl
	 << "alpha      = " << (1.0*const_me()->size())/const_me()->capacity() 
	 << endl;
    for (int i = 0; i < stats.lens.size(); ++i)
      cout << "    " << i << " = " << stats.lens(i) << endl;
  }

  void set_upper_alpha(const float &  __upper_alpha)
  {
    if (__upper_alpha <= lower_alpha)
      throw std::domain_error("upper_alpha lower than lower_alpha");
    
    upper_alpha = __upper_alpha;
  }

  void set_lower_alpha(const float &  __lower_alpha)
  {
    if (__lower_alpha >= upper_alpha)
      throw std::domain_error("lower_alpha greater than upper_alpha");

    lower_alpha = __lower_alpha;
  }

  float get_lower_alpha() const noexcept { return lower_alpha; }

  float get_upper_alpha() const noexcept { return upper_alpha; }
};


# endif // HASHDRY_H
