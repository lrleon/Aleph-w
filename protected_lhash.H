/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# ifndef PROTECTED_LHASH_H
# define PROTECTED_LHASH_H


# include <ahExceptions.H>
# include <tpl_lhash.H>
# include <useMutex.H>


/* ProtectedLhashTable add a protection to the concurrent use of any
   instance of LhashTable. In order to achive this, it use a mutex in
   all operations related with the table */
    template <class Key>
class ProtectedLhashTable : private LhashTableVtl<Key>
{
private:
      /// Mutex used in all operations.
  pthread_mutex_t mutex;

public:

  /* This bucket is an specialization of the LhashTable<key>::Bucket. This
     class allows to know if a bucket is busy when the table is used
     concurrently  */
  class Bucket : public LhashTableVtl<Key>::Bucket
  {
    // Says is a bucket is busy.
    bool is_busy;

  public:

    Bucket() : is_busy(false) { /* Empty */ }

    Bucket(const Key& key) 
      : LhashTableVtl<Key>::Bucket(key), is_busy(false) 
    { 
      // EMPTY 
    }

    // Lock the bucket. 
    void lock() { is_busy = true; }

    // Unlock the bucket. 
    void unlock() { is_busy = false; }

    // Says if the bucket is busy.
    const bool & is_locked() const { return is_busy; }
  };

  ProtectedLhashTable(typename LhashTableVtl<Key>::HashFctType _hashFct,
		      const unsigned long &                    _len) 
    : LhashTableVtl<Key>::LhashTableVtl(_hashFct, _len)
  {
    init_mutex(mutex);
  }

  void remove_all_and_delete()
  {
    CRITICAL_SECTION(&mutex);

    LhashTableVtl<Key>::remove_all_and_delete(); 
  }

  ~ProtectedLhashTable() 
  {
    destroy_mutex(mutex);
  }

  Bucket* insert(Bucket * bucket)
  {
    CRITICAL_SECTION(mutex);

    return static_cast<Bucket*>(LhashTableVtl<Key>::insert(bucket));
  }

  Bucket * search(const Key & key) throw (Lock)
  {
    CRITICAL_SECTION(mutex);

    Bucket *retVal = static_cast<Bucket*>(LhashTableVtl<Key>::search(key));

    if (retVal != nullptr)
      {
	if (retVal->is_locked())
	  Throw (Lock) ();
      
	retVal->lock();
      }

    return retVal;
  }

  Bucket* remove(Bucket * bucket)
  {
    CRITICAL_SECTION(mutex);

    if (bucket->is_locked())
      Throw (Lock) ();

    return static_cast<Bucket*>(LhashTableVtl<Key>::remove(bucket));
  }

  void reset()
  {
    CRITICAL_SECTION(mutex);
    remove_all_and_delete();
  }

  const int resize(const unsigned long & _len)
  {
    CRITICAL_SECTION(mutex);

    return LhashTableVtl<Key>::resize(_len);
  }
  
  const int get_table_size() const 
  {
    CRITICAL_SECTION(mutex);

    return LhashTableVtl<Key>::get_table_size();
  }

  const int get_num_items() const
  {
    CRITICAL_SECTION(mutex);

    return LhashTableVtl<Key>::get_num_items();
  }

  const int get_num_busy_slots() const
  {
    CRITICAL_SECTION(mutex);

    return get_num_busy_slots();
  }

};

# endif // ifdef PROTECTED_LHASH_H

