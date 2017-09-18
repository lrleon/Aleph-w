
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

