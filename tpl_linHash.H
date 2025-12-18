# ifndef TPL_LINHASH_H
# define TPL_LINHASH_H

# include <iostream>
# include <primes.H>
# include <dlink.H>
# include <tpl_dynArray.H>
# include <tpl_dnode.H>
# include <htlist.H>
# include <hashDry.H>
# include <hash-fct.H>
# include <hash-dry.H>
# include <ah-errors.H>

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

# define LINBUCKET_BODY(BUCKETNAME)                     \
                                                        \
  Dlink link;                                           \
                                                        \
public:                                                 \
                                                        \
 BUCKETNAME(const BUCKETNAME & bucket)                  \
   : Dnode<Key>(bucket) {}                              \
                                                        \
 BUCKETNAME() {}                                        \
                                                        \
 BUCKETNAME(const Key & key)                            \
   : Dnode<Key>(key) {}                                 \
                                                        \
 Key & get_key() noexcept { return this->get_data(); }  \
                                                        \
 Dlink * get_link() noexcept { return &link; }          \
                                                        \
 DLINK_TO_BASE(BUCKETNAME, link);


    /** Bucket without virtual destructor for a hash table with collision
        resolution by separate chaining.

        @param Key hash search key
        @see LhashTable
        @ingroup Hash
     */
    template <typename Key>
class LinHashBucket : public Dnode<Key>
{
  LINBUCKET_BODY(LinHashBucket);
};

    /** Bucket with virtual destructor for a hash table with collision
        resolution by separate chaining.

        @param Key hash search key
        @see LhashTable
        @ingroup Hash
     */
    template <typename Key>
class LinHashBucketVtl : public Dnode<Key>
{
  LINBUCKET_BODY(LinHashBucketVtl);

      /// virtual destructor.
  virtual ~LinHashBucketVtl() {}
};


   /** Generic linear hash table.

        Basically, a linear hash table is a hash table with collision
        resolution by separate chaining but with the difference that
        the table size increases dynamically to ensure that the load
        factor, typically called \f$\alpha\f$, is bounded between two
        lower and upper values, respectively.

        Internally, the table uses a dynamic array of type DynArray.
        This leads to memory savings for table entries that have not
        been written.

        This type is generic and is not intended to be used directly.
        Instead, use LinearHashTable or LinearHashTableVtl depending on
        whether you want buckets without or with a virtual destructor.

        @param Key the key type by which the table is indexed.
        @param BucketType the bucket type between LinHashBucket or
        LinHashBucketVtl.
        @param Cmp comparison class between keys.

        @see DynArray LinearHashTable LinearHashTableVtl
        @ingroup Hash
    */
  template <typename Key, template <class> class BucketType,
            class Cmp = Aleph::equal_to<Key>>
class GenLinearHashTable
    : public HashStats<GenLinearHashTable<Key, BucketType, Cmp>>
{
  friend class HashStats<GenLinearHashTable<Key, BucketType, Cmp>>;

public:

      /// The hash function type. Must return pseudo-random positive values,
      /// between zero and the largest possible number.
  using Hash_Fct     = std::function<size_t(const Key &)>;
  using Hash_Fct_Ptr = size_t (*) (const Key &);

  /// The bucket type.
  using Bucket = BucketType<Key>;

  using Key_Type = Key;

  using Item_Type = Key;

private:

  using BucketList = Dnode<Key>;

  using BucketItor = typename Dnode<Key>::Iterator;

  static size_t multiply_by_two(const size_t & n) noexcept { return n << 1; }

  static size_t divide_by_two(const size_t & n) noexcept { return n >> 1; }

  DynArray<BucketList> table;
  Dlink                entries_list;

protected:

  Hash_Fct             hash_fct;
  Cmp                  cmp;

private:

  size_t               M; // Table size
  size_t               N; // Number of elements in the table
  size_t               busy_slots_counter; // Number of occupied entries
  bool                 remove_all_buckets; // Indicates if destructor should free
                                           // the buckets

protected:

  float upper_alpha; // upper load factor
  float lower_alpha; // lower load factor

private:

  size_t p; // index of the list being partitioned (or expanded)
  size_t l; // number of times the table has been doubled
  size_t MP; // stores the value p + M
  size_t MM; // product 2*M

protected:

  mutable size_t len;

private:

  size_t call_hash_fct(const Key & key) const
  {
    const auto hash = hash_fct(key);
    const auto i = hash % M;
    return i < p ? hash % MM : i;
  }

  void expand()
  { // expand the table until load is below upper_alpha
    for (float alpha = 1.0*N/MP; alpha >= upper_alpha; alpha = 1.0*N/MP)
      {
        BucketList * src_list_ptr = table.test(p);
        if (src_list_ptr != nullptr) // is table[p] written?
          if (not src_list_ptr->is_empty()) // is table[p] not empty?
            {
              BucketList * tgt_list_ptr = nullptr;

                  // traverse collision list and move buckets to table[p+M]
              for (BucketItor it(*src_list_ptr); it.has_curr(); /* nothing */)
                {
                  Bucket * bucket = static_cast<Bucket*>(it.get_curr());

                  it.next_ne(); // advance to the next element in the list

                  const Key & key = bucket->get_key();
                  const size_t i = hash_fct(key) % MM;
                  if (i == p) // does this key belong to table[p]?
                    continue; // yes ==> key stays in table[p] ==> next

                  if (tgt_list_ptr == nullptr)
                    tgt_list_ptr = &table.touch(MP);

                    // bucket does not belong to table[p] but to table[p+m] ==>
                    // remove bucket from table[i] and insert it in table[p+m]
                  bucket->del();
                  tgt_list_ptr->append(bucket);
                }

              if (src_list_ptr->is_empty()) // did table[p] become empty?
                --busy_slots_counter; // yes ==> one empty slot

              ++busy_slots_counter; // one new for table[p+M]
            }
        ++p;
        ++MP;
        if (p == M) // (p == 2*M) should the table size be doubled?
          {     // yes ==> change table size to 2*M
            ++l;   // Number of times the table has been doubled
            p = 0;
            MP = M = MM; // assign 2*M to them
            MM = multiply_by_two(MM);
          }
      }
  }

  void contract() noexcept
  { // contract the table until load is below lower_alpha
    for (float alpha = (1.0*N)/MP; alpha <= lower_alpha and MP > len;
         alpha = (1.0*N)/MP)
      {
        if (p == 0) // should the table size be halved?
          {     // yes ==> update table size to M/2
            --l; // Number of times the table has been doubled decreases
            MM = M; // divide by two
            M = divide_by_two(M);
            p = M - 1;
          }
        else
          --p;   // no ==> just reduce index p

        --MP;
        if (MP < table.size()) // Does table[MP] exist?
          {
            BucketList * src_list_ptr = table.test(MP);
            if (src_list_ptr != nullptr) // does entry for table[p+M] exist?
              {
                if (not src_list_ptr->is_empty()) // is table[p+M] empty?
                  {     // no ==> merge the lists
                    BucketList & tgt_list = table.touch(p);// allocate table[p]
                    tgt_list.concat_list(src_list_ptr);
                    --busy_slots_counter; // table[p+M] became empty
                  }
                table.cut_ne(MP); // eventually free memory of table[p+M]
              }
          }
      }
  }

public:

  /// Set the internal hash function
  void set_hash_fct(Hash_Fct fct) noexcept
  {
    hash_fct = fct;
  }

  void set_hash_fct(Hash_Fct_Ptr fct) noexcept
  {
    hash_fct = Hash_Fct(fct);
  }

  Hash_Fct get_hash_fct() const noexcept { return hash_fct; }

  Cmp & get_compare() noexcept { return cmp; }

  const Cmp & get_compare() const noexcept { return cmp; }

  /// return the current table load
  float current_alpha() const noexcept { return 1.0*N/MP; }

 protected:

  GenLinearHashTable(size_t __len, Hash_Fct __hash_fct, Cmp __cmp,
                     float __lower_alpha, float __upper_alpha,
                     bool __remove_all_buckets, bool /* fake */)
  : table(__len), hash_fct(__hash_fct), cmp(__cmp), M(__len), N(0),
      busy_slots_counter(0), remove_all_buckets(__remove_all_buckets),
      upper_alpha(__upper_alpha), lower_alpha(__lower_alpha),
      p(0), l(0), MP(M), MM(multiply_by_two(M)), len(__len)
  {
    ah_length_error_if(M == 0) << "table's length is zero";

    ah_length_error_if(MM > table.max_size()) << "table's length too big";

    ah_domain_error_if(upper_alpha <= lower_alpha)
      << "upper alpha must be greater than lower alpha";
  }

 public:

  /** Instantiate a generic linear hash table.

      Instantiate a generic linear hash table of size __len.

      @param[in] __hash_fct hash function.
      @param[in] __len initial and minimum table size.
      @param[in] __upper_alpha upper threshold at which the table should
      expand.
      @param[in] __lower_alpha lower threshold at which the table should
      contract.
      @param[in] __remove_all_buckets if true, then the table's buckets
      are freed. Otherwise, they remain intact. By default the value is true.

      @throw length_error if __len is equal or greater than the maximum
      dimension allowed for a dynamic array.
      @throw domain_error if __upper_alpha is less than or equal to
      __lower_alpha.
      @throw bad_alloc if there is not enough memory
      @throw overflow_error if an overflow occurs from DynArray
      (caused by its internal calculations).
  */
  GenLinearHashTable(size_t       len                = Primes::DefaultPrime,
                     Hash_Fct_Ptr hash_fct           = Aleph::dft_hash_fct<Key>,
                     Cmp          cmp                = Cmp(),
                     float        lower_alpha        = hash_default_lower_alpha,
                     float        upper_alpha        = hash_default_upper_alpha,
                     bool         remove_all_buckets = true,
                     bool         with_resize        = true)
  : GenLinearHashTable(len, Hash_Fct(hash_fct), cmp,
                       lower_alpha, upper_alpha,
                       remove_all_buckets, with_resize) {}

  void swap(GenLinearHashTable & other) noexcept
  {
    std::swap(table, other.table);
    entries_list.swap(other.entries_list);
    std::swap(hash_fct, other.hash_fct);
    std::swap(cmp, other.cmp);
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

  /** \fn set_upper_alpha

      Change the upper load threshold at which the table begins to
      expand. Throws domain_error if the new threshold is less than
      the lower threshold (lower_alpha)
  */

  /** \fn set_lower_alpha

      Change the lower load threshold at which the table begins to
      contract. Throws domain_error if the new threshold is greater than
      the upper threshold (upper_alpha)
  */

      /// Empty the entire table. All buckets are freed and the size
      /// is adjusted to the initial one.
  void empty() noexcept
  {
    while (not entries_list.is_empty())
      {
        Bucket * bucket = Bucket::dlink_to_base(entries_list.remove_first_ne());
        bucket->del();             // remove from the list in the array
        bucket->get_link()->del(); // remove from entries list
        delete bucket;
      }

    M = MP = len;
    MM = multiply_by_two(M);
    N = p = l = 0;
    table.cut_ne(len);
  }

  ~GenLinearHashTable()
  {
    if (remove_all_buckets)
      empty();
  }

private:

  Bucket *
  search_in_bucket_list(BucketList * list, const Key & key) const
  {
    for (BucketItor it(*list); it.has_curr(); it.next_ne())
      {
        Bucket * bucket = static_cast<Bucket*>(it.get_curr());
        if (cmp (key, bucket->get_key()))
          return bucket;
      }

    return nullptr;
  }

public:

      /// Search for key in the linear hash table. Returns a pointer to
      /// the bucket containing key if it is found in the table;
      /// nullptr otherwise.
  Bucket * search(const Key & key) const noexcept
  {
    auto i = call_hash_fct(key);
    BucketList * list = table.test(i);
    if (list == nullptr) // Has table[i] ever been written?
      return nullptr; // No ==> element is not in the table

    if (list->is_empty())
      return nullptr;

    return search_in_bucket_list(list, key);
  }

     /// Returns the number of elements in the table.
  const size_t & size() const noexcept { return N; }

      /// return true is table is empty
  bool is_empty() const noexcept { return N == 0; }

      /// Returns the table capacity.
  const size_t & capacity() const noexcept { return MP; }

      /// Returns the number of busy slots in the table.
  const size_t & busy_slots() const noexcept { return busy_slots_counter; }

      /// Returns the expansion level performed on the table.
  const size_t & expansions() const noexcept { return l; }

      /// Insert bucket in the table. Return bucket if bucket->get_key()
      /// is not in the table, nullptr otherwise
  Bucket * insert(Bucket * bucket)
  {
    const size_t i = call_hash_fct(bucket->get_key());
    BucketList & list = table.touch(i); // allocates memory for table[i]
    if (list.is_empty())
      ++busy_slots_counter;

    if (search_in_bucket_list(&list, bucket->get_key()) != nullptr)
      return nullptr; // duplicated key

    list.append(bucket);
    entries_list.append(bucket->get_link());
    ++N;
    expand();

    return bucket;
  }

  Bucket * search_or_insert(Bucket * bucket)
  {
    const size_t i = call_hash_fct(bucket->get_key());
    BucketList & list = table.touch(i); // allocates memory for table[i]
    if (list.is_empty())
      ++busy_slots_counter;

    Bucket * p = search_in_bucket_list(&list, bucket->get_key());
    if (p != nullptr)
      return p; // duplicated key

    list.append(bucket);
    entries_list.append(bucket->get_link());
    ++N;
    expand();

    return bucket;
  }

  /// Provided for generic programming compatibility
  size_t resize(size_t) noexcept { return MP; }

private:

  // This routine deletes bucket from hash table EXCEPT from
  // entries_list. The end of this routine is to dry the deletion and to
  // allow remove from other places; ofor instance, from the del()
  // method of Iterator class
  Bucket * remove_bucket(Bucket * bucket) noexcept
  {
    assert(bucket != nullptr);
    assert(search(bucket->get_key()) == bucket);

    Bucket * next = static_cast<Bucket*>(bucket->get_next());

    bucket->del();             // remove from collision list

    if (next->is_empty()) // collision list empty?
      --busy_slots_counter; // yes ==> one empty slot

    --N;
    contract();

    return bucket;
  }

public:

      /// Remove bucket from table. Warning: it is not verified whether
      /// the bucket belongs to the table.
  Bucket * remove(Bucket * bucket) noexcept
  {
    bucket->get_link()->del(); // remove from entries_list
    return remove_bucket(bucket);
  }

  void print()
  {
    for (size_t i = 0; i < MP; ++i)
      {
        cout << "table[" << i << "] = [ ";

        if (table.exist(i))
          {
            BucketList & list = table.access(i);

            if (not list.is_empty())
              for (BucketItor it(list); it.has_curr(); it.next_ne())
                {
                  Bucket * bucket = static_cast<Bucket*>(it.get_curr());
                  const Key & key = bucket->get_key();
                  cout << key << ",";
                }
          }
        cout << "]" << endl;
      }
  }

  class Iterator : public Dlink::Iterator
  {
  private:

    GenLinearHashTable * hash_table = nullptr;
    long pos = 0;

  public:

       /// The set type being iterated over.
    using Set_Type = GenLinearHashTable;

      /// The element type returned by get_curr().
    using Item_Type = Bucket *;

        /// Instantiate an iterator over the hash table.
    Iterator(const GenLinearHashTable & table) noexcept
      : Dlink::Iterator(const_cast<Dlink &>(table.entries_list)),
        hash_table(& const_cast<GenLinearHashTable &>(table))
    {
      // Empty
    }

        /// Instantiate an empty iterator.
    Iterator() noexcept { /* Empty */ }

    Bucket * get_curr_ne() noexcept
    {
      return Bucket::dlink_to_base(this->Dlink::Iterator::get_curr_ne());
    }

        /// Return the current bucket.
    Bucket * get_curr()
    {
      return Bucket::dlink_to_base(this->Dlink::Iterator::get_curr());
    }

    Bucket * del()
    {
      Bucket * bucket = Bucket::dlink_to_base(this->Dlink::Iterator::del());
      return (Bucket *) hash_table->remove_bucket(bucket);
    }

    void next_ne() noexcept
    {
      this->Dlink::Iterator::next_ne();
      pos++;
    }

    void next()
    {
      this->Dlink::Iterator::next();
      pos++;
    }

    void prev()
    {
      this->Dlink::Iterator::prev();
      pos--;
    }

    long get_pos() const noexcept { return pos; }
  };
};


    /** Linear hash table with buckets without virtual destructor.

        Basically, a linear hash table is a hash table with collision
        resolution by separate chaining but with the difference that
        the table size increases dynamically to ensure that the load
        factor, typically called \f$\alpha\f$, is bounded between two
        lower and upper values, respectively.

        Internally, the table uses a dynamic array of type DynArray.
        This leads to memory savings for table entries that have not
        been written.

        @param Key the key type by which the table is indexed.
        @param Cmp comparison class between keys.

        @see DynArray LinearHashTableVtl
        @ingroup Hash
    */
template <typename Key, class Cmp = Aleph::equal_to<Key>>
using LinearHashTable = GenLinearHashTable<Key, LinHashBucket, Cmp>;


    /** Linear hash table with virtual destructor in its buckets.

        Basically, a linear hash table is a hash table with collision
        resolution by separate chaining but with the difference that
        the table size increases dynamically to ensure that the load
        factor, typically called \f$\alpha\f$, is bounded between two
        lower and upper values, respectively.

        Internally, the table uses a dynamic array of type DynArray.
        This leads to memory savings for table entries that have not
        been written.

        @param Key the key type by which the table is indexed.
        @param Cmp comparison class between keys.

        @see DynArray LinearHashTable
        @ingroup Hash
    */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
using LinearHashTableVtl = GenLinearHashTable<Key, LinHashBucketVtl, Cmp>;


} // end namespace Aleph

# ifdef NBACKUP
# define N NBACKUP
# undef NBACKUP
# endif

# ifdef MBACKUP
# define M MBACKUP
# undef MBACKUP
# endif

# endif // TPL_LINHASH_H

