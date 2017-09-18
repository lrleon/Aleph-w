
# ifndef PROTECTED_ODHASH_H
# define PROTECTED_ODHASH_H

# include <ahExceptions.H>
# include <tpl_odhash.H>
# include <useMutex.H>

    template <class Key, class Record>
class Protected_OdHashTable
{
  struct Bucket
  {
    Record record;
    bool   locked;

    Bucket() : locked(false) { /* empty */ }

    Bucket(const Record& _record) : 
      record(_record),
      locked(false) 
    { /* empty */ }
  };

  static Bucket * record_to_bucket(Record * record)
  {
    Bucket * ret_val = 0;
    size_t offset = reinterpret_cast<size_t>(&ret_val->record);

    return 
      reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(record) - offset);
    }

  ODhashTable<Key, Bucket> hash_table;

  pthread_mutex_t mutex;

public:

  typedef const unsigned long (*HashFctType)(const Key&);

  const bool is_locked(Record * record)
  {
    Bucket * bucket = record_to_bucket(record);

    return bucket->locked;
  }

  void lock(Record * record)
  {
    Bucket * bucket = record_to_bucket(record);
    bucket->locked = true;
  }

  void unlock(Record * record)
  {
    Bucket * bucket = record_to_bucket(record);
    bucket->locked = false;
  }

  const size_t get_table_size() 
  {
    CRITICAL_SECTION(mutex);

    return hash_table.get_table_size();
  }

  const size_t get_num_items() 
  {
    CRITICAL_SECTION(&mutex);

    return hash_table.get_num_items();
  }

  Protected_OdHashTable (HashFctType    _first_hash_fct, 
			 HashFctType    _second_hash_fct, 
			 const size_t & len) :
    hash_table (_first_hash_fct, _second_hash_fct, len)
  {
    init_mutex(mutex);
  }

  ~Protected_OdHashTable()
  {
    destroy_mutex(mutex);
  }
   
  Record* insert(const Key & key, const Record & record)
  {
    Bucket bucket(record);

    CRITICAL_SECTION(mutex);

    Bucket * ret_val = hash_table.insert(key, bucket);

    return &ret_val->record;
  }

  Record* search(const Key & key)
  {
    CRITICAL_SECTION(mutex);

    Bucket * bucket = hash_table.search(key);

    if (bucket == nullptr)
      return nullptr;

    if (bucket->locked)
      Throw (Lock) ();

    return &bucket->record;
  }

  void remove(Record * record) 
  {
    Bucket * bucket = record_to_bucket(record);

    if (bucket->locked)
      Throw (Lock) ();

    CRITICAL_SECTION(mutex);

    hash_table.remove(bucket);
  }

  void remove_all_and_delete()
  {
    CRITICAL_SECTION(mutex);

    hash_table.remove_all_and_delete();
  }

  const int get_table_size() const 
  { 
    CRITICAL_SECTION(mutex);
    return hash_table.get_table_size(); 
  }

  class Iterator
  {
  private:

    typename ODhashTable<Key, Bucket>::Iterator itor;
    UseMutex                                    critical_section;
    
  public:

    Iterator(Protected_OdHashTable & table) :
      itor(table.hash_table), CTOR_USE_MUTEX(critical_section, table.mutex)
    {
      // mutex will remain locked until destructor be performed
    }

    const bool has_current() const { return itor.has_current(); }

    Record * get_current() { return &(itor.get_current()->record); }

    void next() { itor.next(); }

    void prev() { itor.prev(); }
  };


};


# endif //  PROTECTED_ODHASH_H

