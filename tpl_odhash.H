
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

# ifndef TPL_ODHASH_H
# define TPL_ODHASH_H

# include <iostream>

# include <primes.H>
# include <dlink.H>
# include <tpl_dynArray.H>
# include <array_it.H>
# include <ahDry.H>
# include <hash-dry.H>
# include <hashDry.H>
# include <hash-fct.H>

using namespace Aleph;

namespace Aleph {

# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif


   /** Tabla hash cerrada con resolución de colisiones por doble función
       hash.  

       Este tipo instrumenta una tabla hash cerrada (el arreglo es
       contiguo en memoria), que guarda las colisiones dentro de la misma
       tabla. Cuando ocurre una colisión, se invoca una segunda función
       hash que para sondear una cubeta disponible. Si se encuentra,
       entonces la clave y el registro se colocan en la cubeta
       disponible; de lo  contrario, se sondea linealmente  a partir del
       índice dado por la segunda función hash. 

       La tabla emplea un método que elimina sin relocalización y en
       tiempo constante las cubetas eliminadas que están en el medio de
       una cadena de búsqueda.

       ODhashTable maneja dos parámetros tipo:
       -# Key: el tipo de dato que guarda la tabla.
       -# Record: el tipo de dato a mapear por la clave.
       -# Cmp clase de comparación entre las claves; debe retornar
       true si las claves son iguales.
 
       @ingroup Hash
   */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
class ODhashTable
    : public OhashCommon<ODhashTable<Key, Cmp>, Key>,
      public GenericTraverse<ODhashTable<Key, Cmp>>,
      public LocateFunctions<ODhashTable<Key, Cmp>, Key>,
      public FunctionalMethods<ODhashTable<Key, Cmp>, Key>,
      public EqualToMethod<ODhashTable<Key, Cmp>>,
      public StlAlephIterator<ODhashTable<Key, Cmp>>
{
  friend class OhashCommon<ODhashTable<Key, Cmp>, Key>;

  public:

  using Key_Type = Key;

  using Item_Type = Key;

  using Hash_Fct = std::function<size_t(const Key &)>;

  using Hash_Fct_Ptr = size_t (*) (const Key&);

  enum Status { EMPTY, BUSY, DELETED };

  enum Probe { NO_PROBED, FIRST_PROBE, SECOND_PROBE, LINEAR_PROBE };

  struct Bucket
  {
    Key           key;                // clave
    unsigned char status     = EMPTY; // status EMPTY, DELETED o BUSY
    unsigned char probe_type = NO_PROBED; // FIRST_PROBE SECOND_PROBE LINEAR_PROBE
    unsigned int  probe_counter = 0; // contador de sondeos

    Bucket() noexcept
      : key(), status(EMPTY), probe_type(NO_PROBED), probe_counter(0)
    { /* empty */ }

    void reset() noexcept // put all as when constructed
    {
      status = EMPTY;
      probe_type = NO_PROBED;
      probe_counter = 0;
    }

    bool valid() const
    {
      return (status == EMPTY or status == DELETED or status == BUSY) and
	(probe_type == NO_PROBED or probe_type == FIRST_PROBE or
	 probe_type == SECOND_PROBE or probe_type == LINEAR_PROBE);
    }

    friend ostream & operator << (ostream & out, const Bucket & bucket)
    {
      string status;
      switch (bucket.status)
	{
	case EMPTY: status = "EMPTY"; break;
	case BUSY: status = "BUSY"; break;
	case DELETED: status = "DELETED"; break;
	}
      string probe_type;
      switch (bucket.probe_type)
	{
	case NO_PROBED: probe_type = "NO_PROBED"; break;
	case FIRST_PROBE: probe_type = "FIRST_PROBE"; break;
	case SECOND_PROBE: probe_type = "SECOND_PROBE"; break;
	case LINEAR_PROBE: probe_type = "LINEAR_PROBE"; break;
	}
      return out << "Bucket at " << &bucket << endl
		 << "status = " << status << endl
		 << "probe_type = " << probe_type << endl
		 << "probe_counter = " << bucket.probe_counter;
    }
  };

  Bucket * table           = nullptr; // arreglo de cubetas
  Hash_Fct hash_fct        = nullptr; // primera función hash
  Hash_Fct second_hash_fct = nullptr; // segunda función hash

  Cmp cmp;

protected:

  size_t len;                       // tamaño de la tabla
  float  lower_alpha;
  float  upper_alpha;

private:

  size_t   N;                       // número de cubetas ocupadas
  bool     with_resize;

  Bucket * allocate_bucket(Bucket & bucket, unsigned char probe_type) noexcept
  {
    assert(bucket.status != BUSY);

    ++N;
    bucket.status     = BUSY;
    bucket.probe_type = probe_type;
    bucket.probe_counter++;

    return &bucket;
  }

  void decrease_probe_counter(Bucket * bucket) noexcept
  {
    assert(bucket->status == BUSY or bucket->status == DELETED);

    bucket->probe_counter--;
    if (bucket->probe_counter == 0) // <marcar EMPTY sobre la cubeta?
	bucket->status = EMPTY;
  }

  void deallocate_bucket(Bucket * bucket) noexcept
  {
    assert(bucket->status == BUSY);

    bucket->status = DELETED;
    const Key & key = bucket->key;

    const size_t i_fst = hash_fct(key) % len;
    if (&table[i_fst] == bucket)
      {
	assert(Cmp () (table[i_fst].key, key)); 
	assert(table[i_fst].probe_type == FIRST_PROBE);
      }
    else
      {
	const size_t i_snd = second_hash_fct(key) % len;
	if (&table[i_snd] == bucket)
	  {
	    assert(Cmp () (table[i_snd].key, key));
	    assert(table[i_snd].probe_type == SECOND_PROBE);
	    decrease_probe_counter(&table[i_fst]);
	  }
	else
	  {
	    decrease_probe_counter(&table[i_fst]);
	    decrease_probe_counter(&table[i_snd]);
	    size_t i = i_snd;
	    for (index_forward(i); &table[i] != bucket; index_forward(i))
	      {
		assert(table[i].status != EMPTY);
		decrease_probe_counter(&table[i]);
	      }
	    assert(Cmp () (table[i].key, key));
	    assert(table[i].probe_type == LINEAR_PROBE);
	  }
      }
    
    decrease_probe_counter(bucket);
    --N;
  }

  size_t index_forward(size_t & i) const noexcept
  {
    assert(i < len);
    if (++i == len)
      i = 0;
    return i;
  }

  size_t index_backward(size_t & i) const noexcept
  {
    assert(i < len);
    if (i-- == 0)
      i = len - 1;
    return i;
  }

  static Bucket * key_to_bucket(Key * rec) noexcept
  {
    Bucket * ret_val = 0;
    const long offset = (long) &ret_val->key;
    return (Bucket*) ((long) rec - offset); 
  }

  bool is_valid_bucket(Bucket * bucket) noexcept
  {
    if (bucket < &table[0] or bucket >= &table[len])
      return false;

    const int offset_with_base = (char*) bucket - (char*) &table[0];
    return offset_with_base % sizeof(*bucket) == 0;
  }

  size_t bucket_to_index(Bucket * bucket) noexcept
  {
    assert(is_valid_bucket(bucket));
    return bucket - &table[0];
  }

public:

  const Cmp & get_compare() const { return cmp; }

  Cmp & get_compare() { return cmp; }

  void swap(ODhashTable & other) noexcept
  {
    std::swap(table, other.table);
    std::swap(hash_fct, other.hash_fct);
    std::swap(second_hash_fct, other.second_hash_fct);
    std::swap(cmp, other.cmp);
    std::swap(N, other.N);
    std::swap(len, other.len);
  }

 protected:

  ODhashTable(size_t __len, Hash_Fct __first_hash_fct,
	      Hash_Fct __second_hash_fct, Cmp __cmp,
 	      float __lower_alpha, float __upper_alpha, bool __with_resize)
    : table(nullptr), hash_fct(__first_hash_fct), 
      second_hash_fct(__second_hash_fct), cmp(__cmp),
      len(Primes::next_prime(__len)),  
      lower_alpha(__lower_alpha), upper_alpha(__upper_alpha),
      N(0), with_resize(__with_resize)
  {
    table = new Bucket[len];
  }

 public:

    /** Instancia una tabla hash cerrada con resolución de colisiones por
        doble hash.  

        @param[in] __first_hash_fct función hash para el primer sondeo.
        @param[in] __second_hash_fct función hash para el segundo sondeo.
        @param[in] len tamaño de la tabla.

        @throw bad_alloc si no hay memoria para la tabla de cubetas.        
    */
  ODhashTable(size_t   len                 = Primes::DefaultPrime,
	      Hash_Fct_Ptr first_hash_fct  = Aleph::dft_hash_fct<Key>, 
	      Hash_Fct_Ptr second_hash_fct = Aleph::snd_hash_fct<Key>, 
	      Cmp      cmp                 = Cmp(),
 	      float    lower_alpha         = hash_default_lower_alpha,
	      float    upper_alpha         = hash_default_upper_alpha,
	      bool     with_resize         = true)
    : ODhashTable(len, Hash_Fct(first_hash_fct), Hash_Fct(second_hash_fct),
		  cmp, lower_alpha, upper_alpha, with_resize) {}

    /// Libera toda la tabla hash.
  ~ODhashTable()
  {
    if (table != nullptr)
      delete [] table;
  }

  ODhashTable(const ODhashTable & other)
    : ODhashTable(other.len, other.hash_fct, other.second_hash_fct, other.cmp,
		  other.lower_alpha, other.upper_alpha, other.with_resize)
  {
    assert(table != nullptr);
    this->copy_from_table(other);
  }

  ODhashTable(ODhashTable && other) : ODhashTable(other)
  {
    assert(table != nullptr);
    swap(other);
  }

  Special_Ctors(ODhashTable, Key);

  ODhashTable & operator = (const ODhashTable & other)
  {
    if (this == &other)
      return *this;

    if (len > other.N)
      this->clean_table();
    else
      {
	Bucket * new_table = new Bucket [other.len];
	delete [] table;
	table = new_table;
	N = 0;
	len = other.len;
	hash_fct        = other.hash_fct;
	second_hash_fct = other.second_hash_fct;
	cmp             = other.cmp;
	lower_alpha     = other.lower_alpha;
	upper_alpha     = other.upper_alpha;
	with_resize     = other.with_resize;
      }

    this->copy_from_table(other);

    return *this;
  }

  ODhashTable & operator = (ODhashTable && other) noexcept
  {
    swap(other);
    return *this;
  }

  OHASH_COMMON(ODhashTable);


    /// busca en la tabla la clave key. Retorna un puntero al registro
    /// asociado a key dentro de la tabla; nullptr de lo contrario.
  Key * search(const Key & key) const noexcept
  {
    const size_t i_fst = hash_fct(key) % len; // 1er sondeo (1ra fun hash)
    if (table[i_fst].status == EMPTY) 
      return nullptr;

    if (table[i_fst].status == BUSY and cmp(table[i_fst].key, key))
      {
	assert(table[i_fst].probe_type == FIRST_PROBE);
	assert(table[i_fst].probe_counter > 0);
	return &table[i_fst].key;
      }

    const size_t i_snd = second_hash_fct(key) % len; // 2do sondeo

    if (table[i_snd].status == EMPTY)
      return nullptr;

    if (table[i_snd].status == BUSY and cmp(table[i_snd].key, key)) 
      {
	assert(table[i_snd].probe_type == SECOND_PROBE);
	assert(table[i_snd].probe_counter > 0);
	return &table[i_snd].key;
      }

    size_t i = i_snd; 
        // sondeo lineal a partir de índice de 2da función hash
    for (size_t count = 0; count < len; ++count)
      {
	index_forward(i);
	switch (table[i].status)
	  {
	  case EMPTY:
	    assert(table[i].probe_counter == 0);
	    return nullptr;
	  case BUSY:
	    assert(table[i].probe_counter > 0);
	    if (cmp(table[i].key, key))
	      {
		assert(table[i].probe_type == LINEAR_PROBE);
		return &table[i].key;
	      }
	    break;
	  case DELETED:
	    assert(table[i].probe_counter > 0);
	    break;
	  default: AH_ERROR("ODhashTable search: inconsistent bucket status");
	  }
      }
   
    return nullptr; 
  }

  Hash_Fct get_second_hash_fct() const noexcept { return second_hash_fct; }

  void set_second_hash_fct(Hash_Fct fct) noexcept
  {
    second_hash_fct = fct;
  }

  void set_second_hash_fct(Hash_Fct_Ptr fct) noexcept
  {
    second_hash_fct = Hash_Fct(fct);
  }

private:

  Bucket * allocate_bucket(const Key & key) noexcept
  {
    assert(N < len);

    const size_t i_fst = hash_fct(key) % len; // sondeo con 1ra función hash
    if (table[i_fst].status != BUSY) // ¿cubeta disponible?
      return allocate_bucket(table[i_fst], FIRST_PROBE);

    if (cmp(table[i_fst].key, key)) // test if key is alredy present
      return nullptr;

    const size_t i_snd = second_hash_fct(key) % len; // 2do hash
    if (table[i_snd].status != BUSY) // ¿cubeta disponible?
      {
	table[i_fst].probe_counter++;
	return allocate_bucket(table[i_snd], SECOND_PROBE);
      }

    if (cmp(table[i_snd].key, key)) // test if key is alredy present
      return nullptr;

    size_t i = i_snd;
    for (size_t c = 0; c < len; ++c)
      {	 
	index_forward(i);
	switch (table[i].status)
	{
	case BUSY: 
	  if (cmp(table[i].key, key))
	    { // duplicated key ==> rollback previous probe_counter increases
	      for (size_t k = 0; k < c; ++k)
		{
		  index_backward(i);
		  table[i].probe_counter--;
		}
	      return nullptr;
	    }
	  break;
	case DELETED: 
	case EMPTY:
	  table[i_fst].probe_counter++;
	  table[i_snd].probe_counter++;
	  return allocate_bucket(table[i], LINEAR_PROBE); 
	default: AH_ERROR("ODhashTable: Invalid bucket status"); break;
	}
	table[i].probe_counter++;
      }

    return nullptr;
  }

  // search key. If found return true, otherwise false and allocates the bucket
  tuple<Bucket*, bool> hard_allocate_bucket(const Key & key) noexcept
  {
    assert(N < len);

    const size_t i_fst = hash_fct(key) % len; // sondeo con 1ra función hash
    if (table[i_fst].status != BUSY) // ¿cubeta disponible?
      return tuple<Bucket*, bool>(allocate_bucket(table[i_fst], FIRST_PROBE),
				  false);

    if (cmp(table[i_fst].key, key)) // test if key is alredy present
      return tuple<Bucket*, bool>(&table[i_fst], true);

    const size_t i_snd = second_hash_fct(key) % len; // 2do hash
    if (table[i_snd].status != BUSY) // ¿cubeta disponible?
      {
	table[i_fst].probe_counter++;
	return tuple<Bucket*, bool>(allocate_bucket(table[i_snd], SECOND_PROBE),
				    false);
      }

    if (cmp(table[i_snd].key, key)) // test if key is alredy present
      return tuple<Bucket*, bool>(&table[i_snd], true);

    size_t i = i_snd;
    for (size_t c = 0; c < len; ++c)
      {	 
	index_forward(i);
	switch (table[i].status)
	  {
	  case BUSY: 
	    if (cmp(table[i].key, key))
	      { // duplicated key ==> rollback previous probe_counter increases
		const size_t idx = i;
		for (size_t k = 0; k < c; ++k)
		  {
		    index_backward(i);
		    table[i].probe_counter--;
		  }
		return tuple<Bucket*, bool>(&table[idx], true);
	      }
	    break;
	  case DELETED: 
	  case EMPTY:
	    table[i_fst].probe_counter++;
	    table[i_snd].probe_counter++;
	    return make_tuple(allocate_bucket(table[i], LINEAR_PROBE), false);
	  default: AH_ERROR("ODhashTable: Invalid bucket status"); break;
	  }
	table[i].probe_counter++;
      }
    assert(false); // never must reach this point
    return tuple<Bucket*, bool>((Bucket*)nullptr, false);
  }

    /// Elimina de la tabla el registro record. record debe pertenecer a
    /// la tabla y debe haberse determinado previamente mediante una
    /// inserción o búsqueda. Se dispara la excepción invalid_argument si
    /// el registro no corresponde a una cubeta de la tabla. Se dispara
    /// domain_error si el registro en la tabla no contiene un elemento.
  void remove_bucket(Bucket * bucket) 
  {
    if (not is_valid_bucket(bucket))
      throw std::invalid_argument("key pty does not belong to hash table");

    if (bucket->status != BUSY)
      throw std::domain_error("Bucket containing key is not BUSY");

    deallocate_bucket(bucket);
  }

  // returns true if the bucket contains the key
  bool process_entry_for_remove(Bucket * bucket, const Key & key) 
  {
    switch (bucket->status)
      {
      case EMPTY:
	throw std::domain_error("Key not in hash table");
      case BUSY:
	if (cmp(bucket->key, key))
	  {
	    bucket->status = DELETED;
	    decrease_probe_counter(bucket);
	    return true;
	  }
	break;
      case DELETED:
	decrease_probe_counter(bucket);
	break;
      default: AH_ERROR("Inconsistent bucket status");
      }
    return false;
  }

public:

  void remove(const Key & key)
  {
    --N;
    try
      {
	const size_t i_fst = hash_fct(key) % len;
	if (process_entry_for_remove(&table[i_fst], key))
	  return;
    
	const size_t i_snd = second_hash_fct(key) % len;
	if (process_entry_for_remove(&table[i_snd], key))
	  return;

	size_t i = i_snd;
	for (size_t c = 0; i < len; ++c)
	  {
	    index_forward(i);
	    if (process_entry_for_remove(&table[i], key))
	      return;
	  }
      }
    catch (...)
      {
	++N;
	this->rehash(); // in order to leave the table in a consistent state
	throw;
      }
  }
  
  using Stats = typename OhashCommon<ODhashTable<Key, Cmp>, Key>::Stats;

  Stats stats()	const							
  {									
    DynArray<size_t> lens;						
    size_t num_busy = 0, num_deleted = 0, num_empty = 0;			
    size_t max_len = std::numeric_limits<size_t>::min();			
    for (size_t i = 0; i < len; ++i)					
      switch (table[i].status)						
	{								
	case BUSY:							
	  {								
	    ++num_busy;							
	    const Key & key = table[i].key;				
	    size_t count = 1;
	    const size_t i_fst = hash_fct(key) % len;
	    if (cmp(table[i_fst].key, key))
		{
		  assert(table[i_fst].probe_type == FIRST_PROBE);
		  assert(table[i_fst].probe_counter > 0);
		  ;
		}
	    else 
	      {
		++count;
		size_t i_snd = second_hash_fct(key) % len;
		if (cmp(table[i_snd].key, key))
		{
		  assert(table[i_snd].probe_type == SECOND_PROBE);
		  assert(table[i_snd].probe_counter > 0);
		  ;
		}
		else
		  {
		    for (size_t i = index_forward(i_snd); true; 
			 index_forward(i))
		      {
			if (table[i].status == BUSY and cmp(table[i].key, key))
			  break;
			++count;
		      }
		  }
	      }
	    max_len = std::max(max_len, count);				
	    update_stat_len(lens, count);				
	    break;							
	  }								
	case EMPTY:							
	  ++num_empty;							
	  update_stat_len(lens, 0);					
	  break;								
	case DELETED:							
	  ++num_deleted;							
	  break;								
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
    stats.num_busy = num_busy;						
    stats.num_deleted = num_deleted;					
    stats.num_empty = num_empty;						
    std::swap(lens, stats.lens);						
    stats.avg = avg;							
    stats.var = var;							
    stats.max_len = max_len;						
									
    return stats;						
  }
};


template <typename Key, class Cmp = Aleph::equal_to<Key>>
using SetODhash = ODhashTable<Key, Cmp>;


# ifdef NBACKUP
# define N NBACKUP
# undef NBACKUP
# endif

# ifdef MBACKUP
# define M MBACKUP
# undef MBACKUP
# endif

# undef EMPTY
# undef BUSY
# undef DELETED
# undef NO_PROBED    
# undef FIRST_PROBE  
# undef SECOND_PROBE 
# undef LINEAR_PROBE 

} // end namespace Aleph

# endif // TPL_ODHASH_H

