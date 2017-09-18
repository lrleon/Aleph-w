# ifndef TPL_DYNSETHASH_H
# define TPL_DYNSETHASH_H

# include <algorithm>
# include <typeinfo>
# include <ahDry.H>
# include <ahIterator.H>
# include <primes.H>
# include <htlist.H>
# include <tpl_dynArray.H>
# include <tpl_dynSetOhash.H>
# include <tpl_dynLhash.H>
# include <tpl_linHash.H>


namespace Aleph  
{

    /** Tabla hash dinámica autoajustante

	DynHashTable<Key> instrumenta una tabla hash lineal y
	dinámica en la cual no es necesario preocuparse por el tamaño;
	éste es automáticamente ajustado según unos límites al factor de
	carga definidos en tiempo de construcción. 

	@param Key el tipo de dato del dominio según el cual se indiza
	el rango.

	@see LinearHashTable
	@ingroup Hash
     */
  template <typename                          Key, 
	    class                             Cmp       = Aleph::equal_to<Key>,
	    template <typename,  class> class HashTable = LhashTable>
class DynHashTable 
    : public HashTable<Key, Cmp>,
      public GenericTraverse<DynHashTable<Key, Cmp, HashTable>>,
      public LocateFunctions<DynHashTable<Key, Cmp, HashTable>, Key>,
      public FunctionalMethods<DynHashTable<Key, Cmp, HashTable>, Key>,
      public GenericKeys<DynHashTable<Key, Cmp, HashTable>, Key>,
      public EqualToMethod<DynHashTable<Key, Cmp, HashTable>>,
      public StlAlephIterator<DynHashTable<Key, Cmp, HashTable>>
{
protected:

  using Base = HashTable<Key, Cmp>;

  using Bucket = typename HashTable<Key, Cmp>::Bucket;

public:

      /// El tipo de función hash.
  using Hash_Fct = typename Base::Hash_Fct;

  using Key_Type = Key;

  using Item_Type = Key;

      /** Instancia una tabla hash lineal dinámica.

	  @param[in] hash_fct la función hash.
	  @param[in] len tamaño inicial y mínimo de la tabla hash. El
	  tamaño nunca será más bajo que el este.
	  @param[in] lower_alpha umbral inferior de carga; tamaño de la
	  tabla se reducirá si la carga es inferior a este factor.
	  @param[in] upper_alpha umbral superior de carga;  tamaño de la
	  tabla se expandirá si la carga es superior a este factor.
       */
  DynHashTable(const size_t len, Hash_Fct hash_fct,
	       Cmp & cmp, const float lower_alpha, const float upper_alpha) 
    : Base(len, hash_fct, cmp, lower_alpha, upper_alpha, true, true)
  {
    // empty
  }

  DynHashTable(size_t len        = Primes::DefaultPrime,
	       Hash_Fct hash_fct = Aleph::dft_hash_fct<Key>,
	       Cmp && cmp        = Cmp(),
	       float lower_alpha = hash_default_lower_alpha,
	       float upper_alpha = hash_default_upper_alpha) 
    : DynHashTable(len, hash_fct, cmp, lower_alpha, upper_alpha)
    {
      // empty
    }

private:

  void copy(const DynHashTable & other)
  {
    for (typename Base::Iterator it(other); it.has_curr(); it.next())
      {
	Bucket * bucket = (Bucket*) it.get_curr();
	insert(bucket->get_key());
	assert(Base::cmp(*search(bucket->get_key()), bucket->get_key()));
      }
  }

public:

  DynHashTable(const DynHashTable & other)
    : Base(other.len, other.hash_fct, 
	   const_cast<DynHashTable&>(other).get_compare(),
	   other.lower_alpha, other.upper_alpha, true, true)
  {
    copy(other);
  }
  
  DynHashTable(DynHashTable && other) noexcept
    : Base(other.len, other.hash_fct, other.cmp,
	   other.lower_alpha, other.upper_alpha, true, true)
  {
    this->swap(other);
  }

  Special_Ctors(DynHashTable, Key);

  ~DynHashTable()
  {
    this->empty();
  }

  DynHashTable & operator = (const DynHashTable & other)
  {
    if (this == &other)
      return *this;

    this->empty();
    copy(other);

    return *this;
  }

  DynHashTable & operator = (DynHashTable && other) noexcept
  {
    this->swap(other);
    return *this;
  }

protected:

  Key * insert_bucket(Bucket * bucket) 
  {
    Bucket * ret_val = (Bucket*) this->Base::insert(bucket);
    if (ret_val == nullptr) // is the key in the table?
      {    // yes! ==> free bucket
	delete bucket;
	return nullptr;
      }

    return &ret_val->get_key();
  }

  tuple<Key*, bool> search_or_insert_bucket(Bucket * bucket)
  {
    Bucket * ret_val = (Bucket*) this->Base::search_or_insert(bucket);
    if (ret_val != bucket) // is the key in the table?
      {    // yes! ==> free bucket
	delete bucket;
	return make_tuple(&ret_val->get_key(), true);
      }

    return make_tuple(&ret_val->get_key(), false);
  }

public:

      /// Inserta en el conjunto hash la clave key. Retorna un puntero a
      /// la clave si fue insertada o nullptr si la clave no fue insertada
  Key * insert(const Key & key)
  {
    return insert_bucket(new Bucket(key));
  }

  Key * insert(Key && key)
  {
    return insert_bucket(new Bucket(std::forward<Key>(key)));
  }

  Key * search_or_insert(const Key & key)
  {
    return get<0>(search_or_insert_bucket(new Bucket(key)));
  }

  Key * search_or_insert(Key && key)
  {
    return get<0>(search_or_insert_bucket(new Bucket(std::forward<Key>(key))));
  }

  // Retorna true si key ya se encuentra en la tabla. De lo contrario,
  // se inserta key y se retorna false
  pair<Key*, bool> contains_or_insert(const Key & key)
  {
    return search_or_insert_bucket(new Bucket(key));
  }

  pair<Key*, bool> contains_or_insert(Key && key)
  {
    return search_or_insert_bucket(new Bucket(std::forward<Key>(key)));
  }

  Key * add(const Key & key)
  {
    return insert_bucket(new Bucket(key));
  }
 
  Key * add(Key && key)
  {
    return insert_bucket(new Bucket(std::forward<Key>(key)));
  }

  Key * append(const Key & key)
  {
    return insert_bucket(new Bucket(key));
  }
 
  Key * append(Key && key)
  {
    return insert_bucket(new Bucket(std::forward<Key>(key)));
  }

      /// Busca la clave key, si se encuentra, entonces retorna un puntero
      /// dentro de la tabla a la clave. De lo contrario -no se
      /// encuentra la clave-, se retorna nullptr:
  Key * search(const Key & key) const
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);
    return bucket != nullptr ? &bucket->get_key() : nullptr;
  }

  bool has(const Key & key) const 
  {
    return this->Base::search(key) != nullptr;
  }

  bool contains(const Key & key) const { return has(key); }

  const Key & find(const Key & key) const 
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);
    if (bucket == nullptr)
      throw std::domain_error("Key not found in hash");

    return bucket->get_key();
  }

  Key & find(const Key & key) 
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);

    if (bucket == nullptr)
      throw std::domain_error("Key not found in hash");

    return bucket->get_key();
  }

protected:

  static Bucket * key_to_bucket(Key * key)
  {
    Bucket * ret_val = 0;
    size_t offset = reinterpret_cast<size_t>(&ret_val->get_key());
    
    return reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(key) - offset);
  }

public:

      /// Elimina de la tabla la clave key cuyo puntero debe ser
      /// resultante de una inserción o búsqueda.
  void remove(Key * key)
  {
    Bucket * bucket = key_to_bucket(key);
    this->Base::remove(bucket);
    delete bucket;
  }

  Key remove(const Key & key)
  {
    Bucket * bucket = (Bucket*) this->Base::search(key);
    if (bucket == nullptr)
      throw std::domain_error("Key not found in hash table");

    this->Base::remove(bucket);
    auto ret_val = bucket->get_key();
    delete bucket;
    return ret_val;
  }

  class Iterator : public Base::Iterator
  { 
  public: 

    using Item_Type = Key;

    using Set_Type = DynHashTable;

    Iterator(const DynHashTable & table) : Base::Iterator(table) {} 

    const Key & get_curr() const
    { 
      return const_cast<Iterator*>(this)->get_curr();
    } 
    
    Key & get_curr() 
    { 
      return this->Base::Iterator::get_curr()->get_key(); 
    } 
    
    void del() { delete this->Base::Iterator::del(); } 
  }; 
};

/**

   @ingroup Hash
 */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct DynSetLhash : public DynHashTable<Key, Cmp, LhashTable>
{
  typedef DynHashTable<Key, Cmp, LhashTable> Base;
  using Base::Base;
}; 

/**

   @ingroup Hash
 */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
struct DynSetLinHash : public DynHashTable<Key, Cmp, LinearHashTable>
{
  typedef DynHashTable<Key, Cmp, LinearHashTable> Base;
  using Base::Base;
}; 

/**

   @ingroup Hash
 */
template <typename Key, class Cmp = Aleph::equal_to<Key>>
  using DynSetHash = DynHashTable<Key, Cmp, LhashTable>;

/**

   @ingroup Hash
 */
template <typename Key, typename Data, 
	  class Cmp = Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>,
	  template <class, class> class HashTable = LhashTable>
class DynMapHashTable 
  : public DynHashTable<std::pair<Key, Data>, Cmp, HashTable>
{
  using Pair = std::pair<Key, Data>;

  using Base = DynHashTable<std::pair<Key, Data>, Cmp, HashTable>;

  using Bucket = typename Base::Bucket;

public:

  static Data & get_data(const Key & key)
  {
    return key_to_pair<Key, Data>(&const_cast<Key&>(key))->second;    
  }

  static const Key & get_key(Data * data_ptr)
  {
    return data_to_pair<Key, Data>(data_ptr)->first;
  }

  using Value_Type = Data;

  // using Base::Base;  // no more need. But I don't remember why I put it
  using Base::insert;
  using Iterator = typename Base::Iterator;

  typedef size_t (*Hash_Fct)(const Pair&);

  DynMapHashTable(size_t len, Hash_Fct hash_fct, Cmp & cmp, 
		  float lower_alpha, float upper_alpha)
    : Base(len, hash_fct, cmp, lower_alpha, upper_alpha) {}

  DynMapHashTable(size_t len        = Primes::DefaultPrime, 
		  Hash_Fct hash_fct = map_dft_hash_fct, 
		  Cmp && cmp        = Cmp(), 
		  float lower_alpha = hash_default_lower_alpha, 
		  float upper_alpha = hash_default_upper_alpha)
    : DynMapHashTable(len, hash_fct, cmp, lower_alpha, upper_alpha) {}

  /// Inserta en el mapeo hash el par (key,record) indizado por la clave
  /// key. Retorna un puntero al registro dentro de la tabla. Dispara la
  /// excepción bad_alloc si no hay suficiente memoria.
  Pair * insert(const Key & key, const Data & data)
  {
    return this->insert_bucket(new typename Base::Bucket (Pair(key, data)));
  }

  Pair * insert(const Key & key, Data && data)
  {
    return this->insert_bucket(new typename Base::Bucket(Pair(key, 
							      move(data))));
  }

  Pair * insert(Key && key, Data && data)
  {
    return this->insert_bucket(new typename Base::Bucket(Pair(move(key), 
							      move(data))));
  }

  Pair * insert(Key && key, const Data & data)
  {
    return this->insert_bucket(new typename Base::Bucket(Pair(move(key), 
							      data)));
  }

  /// Busca la clave key y, si se encuentra, entonces retorna un puntero
  /// dentro de la tabla a par asociado. De lo contrario -no se
  /// encuentra la clave-, se retorna nullptr:
  Pair * search(const Key & key) const
  {
    return this->Base::search(Pair(key, Data()));
  }

  Pair * search(Key && key) const
  {
    return this->Base::search(Pair(move(key), Data()));
  }

  bool has(const Key & key) const { return search(key) != nullptr; }

  bool has(Key && key) const { return search(move(key)) != nullptr; }

  bool contains(const Key & key) const { return has(key); }

  bool contains(Key && key) const { return has(move(key)); }

  Data & find(const Key & key) 
  {
    return Base::find(Pair(key, Data())).second;
  }

  const Data & find(const Key & key) const 
  {
    return Base::find(Pair(key, Data())).second;
  }

  Data & operator [] (const Key & key)
  { 
    return this->search_or_insert(Pair(key, Data()))->second;
  }

  const Data & operator [] (const Key & key) const
  {
    return this->find(key);
  }

  Data & operator [] (Key && key)
  { 
    return this->search_or_insert(Pair(move(key), Data()))->second;
  }

  const Data & operator [] (Key && key) const
  {
    return this->find(move(key));
  }

  /// Elimina de la tabla la clave key cuyo puntero debe ser
  /// resultante de una inserción o búsqueda.
  void remove_by_data(Data & data)
  {
    Base::remove(data_to_pair<Key, Data>(&data));
  }

  Data remove(const Key & key)
  {
    auto p = Base::remove(Pair(key, Data()));
    return p.second;
  }

  Data remove(Key && key)
  {
    auto p = Base::remove(Pair(move(key), Data()));
    return p.second;
  }

  DynList<Key> keys() const
  {
    return this->template maps<Key>([] (auto p) { return p.first; });
  }

  DynList<Key> values() const
  {
    return this->template maps<Data>([] (auto p) { return p.second; });
  }

  DynList<Data*> values_ptr()
  {
    DynList<Data*> ret;
    for (Iterator it(*this); it.has_curr(); it.next())
      ret.append(&it.get_curr().second);
    return ret;
  }

  DynList<Pair*> items_ptr()
  {
    DynList<Pair*> ret;
    for (Iterator it(*this); it.has_curr(); it.next())
      ret.append(&it.get_curr());
    return ret;
  }
};

/**

   @ingroup Hash
 */
template <typename Key, typename Data,
 	  class    Cmp = Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
using DynMapLinHash = DynMapHashTable<Key, Data, Cmp, LinearHashTable>;

/**

   @ingroup Hash
 */
template <typename Key, typename Data,
 	  class    Cmp = Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
using DynMapHash = DynMapHashTable<Key, Data, Cmp, LhashTable>;


// Implementation de coming from ahFunctional.H

template <typename T, template <typename> class Container> inline
DynList<T> join(const Container<T> & c1, const Container<T> & c2)
{
  DynSetLhash<T> table(c1);
  c2.for_each([&table] (const T & item)
	      {
		table.insert(item);
	      });
  return table.keys();
}

template <typename T, template <typename> class Container = DynList> inline
DynList<T> intercept(const Container<T> & c1, const Container<T> & c2)
{
  DynSetLhash<T> set1 = c1;
  DynSetLhash<T> set2 = c2;
  return set1.filter([&set2] (const T & i) { return set2.contains(i); });
}

template <typename T, template <typename> class Container> inline
DynList<T> unique(const Container<T> & c)
{
  return DynSetLhash<T>(c).keys();
}

template <typename T, template <typename> class Container> inline
DynList<T> repeated(const Container<T> & c)
{
  DynList<T> ret;
  DynSetLhash<T> table;
  
  c.for_each([&table, &ret] (const T & i)
    {
      auto * ptr = table.insert(i);
      if (ptr == nullptr)
	ret.append(i);
    });

  return ret;
}

template <typename T, template <typename> class Container> inline
DynList<std::pair<T, size_t>> repeated_with_index(const Container<T> & c)
{
  DynList<std::pair<T, size_t>> ret;
  DynSetLhash<T> table;
  
  size_t i = 0;
  c.for_each([&table, &ret, &i] (const T & item)
    {
      auto * ptr = table.insert(item);
      if (ptr == nullptr)
	ret.append(std::pair<T, size_t>(item, i));
      ++i;
    });

  return ret;
}

} // end namespace Aleph

# endif // TPL_DYNSETHASH_H




