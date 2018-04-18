
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
  @file protected_lhash.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef PROTECTED_LHASH_H
# define PROTECTED_LHASH_H


# include <ahExceptions.H>
# include <tpl_lhash.H>
# include <useMutex.H>


/** 
  @brief ProtectedLhashTable adds protection for the simultaneous 
  use of any instance LhashTable. 

  @details To accomplish this, 
  use a mutex in all operations related to the table
*/
    template <class Key>
class ProtectedLhashTable : private LhashTableVtl<Key>
{
private:
/* Mutex usado en todas las operaciones. */
  pthread_mutex_t mutex;

public:

/** 
  @brief This bucket is a specialization of LhashTable<key>::Bucket. 
 
  @details This class lets you know if a bucket is occupied when the table
  it used simultaneously
*/
  class Bucket : public LhashTableVtl<Key>::Bucket
  {
    
/** 
  @brief Says if a bucket is full
*/
    bool is_busy;

  public:

    Bucket() : is_busy(false) { /* Empty */ }

    Bucket(const Key& key) 
      : LhashTableVtl<Key>::Bucket(key), is_busy(false) 
    { 
      /* Empty */ 
    }

/** 
  @brief Blocks the bucket. 
*/
    void lock() { is_busy = true; }

/** 
  @brief Unblocks the bucket. 
*/
    void unlock() { is_busy = false; }

/** 
  @brief Says if a bucket is full.
*/
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

    if (retVal != NULL)
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
