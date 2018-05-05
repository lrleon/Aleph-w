
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

# define LINBUCKET_BODY(BUCKETNAME)					\
									\
  Dlink link;								\
									\
public:									\
									\
 BUCKETNAME(const BUCKETNAME & bucket)					\
   noexcept(std::is_nothrow_constructible<Dnode<Key>>::value)		\
   : Dnode<Key>(bucket) {}						\
									\
 BUCKETNAME() noexcept(std::is_nothrow_constructible<Key>::value) {}	\
									\
 BUCKETNAME(const Key & key) noexcept(noexcept(Dnode<Key>(key)))	\
   : Dnode<Key>(key) {}							\
									\
 Key & get_key() noexcept { return this->get_data(); }			\
									\
 Dlink * get_link() noexcept { return &link; }				\
									\
 DLINK_TO_BASE(BUCKETNAME, link);

 
    /** Cubeta sin destructor virtual para una tabla hash con resolución
        de colisiones por encadenamiento separado. 

        @param Key clave de búsqueda hash
        @see LhashTable
        @ingroup Hash
     */
    template <typename Key>
class LinHashBucket : public Dnode<Key>
{
  LINBUCKET_BODY(LinHashBucket);
};

    /** Cubeta sin destructor virtual para una tabla hash con resolución
        de colisiones por encadenamiento separado. 

        @param Key clave de búsqueda hash
        @see LhashTable
        @ingroup Hash
     */
    template <typename Key>
class LinHashBucketVtl : public Dnode<Key>
{
  LINBUCKET_BODY(LinHashBucketVtl);

      /// destructor virtual.
  virtual ~LinHashBucketVtl() {}
};


   /** Tabla hash lineal genérica.

        Básicamente, una tabla hash lineal es una tabla hash con
        resolución de colisiones por encadenamiento separado pero con la
        diferencia de que el tamaño de la tabla aumenta dinámicamente
        para asegurar que el factor de carga, típicamente llamado
        \f$\alpha\f$, esté delimitado entre dos valores inferior y
        superior, respectivamente.

        Internamente, la tabla utiliza un arreglo dinámico del tipo
        DynArray. Esto conlleva ahorros de memoria por entradas de la
        tabla que no hayan sido escritas.

        Este tipo es genérico y no está destinado a usarse. En su lugar,
        utilícese LinearHashTable o LinearHashTableVtl según se desee
        utilizar cubetas sin o con destructor virtual.

        @param Key el tipo de clave por el cual se indiza la tabla.
        @param BucketType el tipo de cubeta entre LinHashBucket o
        LinHashBucketVtl. 
        @param Cmp clase de comparación entre las claves.

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

      /// El tipo de función hash. Debe retornar valores positivos seudo
      /// aleatorios, entre cero y el número más grande posible.
  using Hash_Fct     = std::function<size_t(const Key &)>;
  using Hash_Fct_Ptr = size_t (*) (const Key &);

      /// El tipo de cubeta.
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

  Hash_Fct             hash_fct; // puntero a función hash
  Cmp                  cmp;

private:

  size_t               M; // Tamaño de la tabla
  size_t               N; // Número de elementos que tiene la tabla
  size_t               busy_slots_counter; // Cantidad de entradas ocupadas 
  bool                 remove_all_buckets; // Indica si destructor debe liberar
                                           // las cubetas  

protected:

  float upper_alpha; // factor de carga superior
  float lower_alpha; // factor de carga inferior

private:

  size_t p; // índice de la lista que se particiona (o aumenta)
  size_t l; // cantidad de veces que se ha duplicado la tabla
  size_t MP; // guarda el valor p + M
  size_t MM; // producto 2*M

protected:

  mutable size_t len;

private:

  size_t call_hash_fct(const Key & key) const 
    noexcept(noexcept(hash_fct(key)))
  {
    const auto hash = hash_fct(key);
    const auto i = hash % M;
    return i < p ? hash % MM : i;
  }

  void expand()
  { // expandir la tabla hasta que la carga esté debajo de upper_alpha
    for (float alpha = 1.0*N/MP; alpha >= upper_alpha; alpha = 1.0*N/MP)
      {
        BucketList * src_list_ptr = table.test(p);
        if (src_list_ptr != nullptr) // ¿table[p] está escrita?
          if (not src_list_ptr->is_empty()) // ¿table[p] no está vacía'
            {
              BucketList * tgt_list_ptr = nullptr; 

                  // recorrer lista colisiones y mover cubetas de table[p+M] 
              for (BucketItor it(*src_list_ptr); it.has_curr(); /* nada */)
                {
                  Bucket * bucket = static_cast<Bucket*>(it.get_curr());

                  it.next(); // avance al siguiente elemento de la lista

                  const Key & key = bucket->get_key();
                  const int i = hash_fct(key) % MM;
                  if (i == p) // ¿pertenece esta clave a table[p]?
                    continue; // sí ==> clave sigue en table[p] ==> siguiente

                  if (tgt_list_ptr == nullptr)
                    tgt_list_ptr = &table.touch(MP);

                    // bucket no pertenece a table[p] sino a table[p+m] ==>
                    // eliminar bucket de table[i] e insertarlo en table[p+m]
                  bucket->del();
                  tgt_list_ptr->append(bucket);
                }

              if (src_list_ptr->is_empty()) // ¿table[p] quedó vacía?
                --busy_slots_counter; // sí ==> un slot vacío 

              ++busy_slots_counter; // uno nuevo por table[p+M]
            } 
        ++p;
        ++MP;
        if (p == M) // (p == 2*M) ¿debe duplicarse el tamaño de la tabla?
          {     // sí ==> modificar el tamaño de la tabla a 2*M
            ++l;   // Cantidad de veces que se ha duplicado la tabla
            p = 0;
            MP = M = MM; // se les asigna 2*M
            MM = multiply_by_two(MM);
          }
      } 
  }

  void contract()
  { // contraer la tabla hasta que carga esté debajo de lower_alpha
    for (float alpha = (1.0*N)/MP; alpha <= lower_alpha and MP > len; 
         alpha = (1.0*N)/MP)
      {
        if (p == 0) // ¿debe dividirse entre 2 el tamaño de la tabla?
          {     // sí ==> actualizar tamaño de la tabla a M/2
            --l; // Cantidad de veces que se ha duplicado la tabla disminuye
            MM = M; // se divide entre dos
            M = divide_by_two(M);
            p = M - 1;
          }
        else
          --p;   // no ==> sólo reducir índice p

        --MP;
        if (MP < table.size()) // ¿Existe table[MP]]? 
          {
            BucketList * src_list_ptr = table.test(MP);
            if (src_list_ptr != nullptr) // ¿existe entrada para table[p+M]?
              {
                if (not src_list_ptr->is_empty()) // ¿table[p+M] está vacía?
                  {     // no ==> fusionar las listas
                    BucketList & tgt_list = table.touch(p);// aparta table[p]
                    tgt_list.concat_list(src_list_ptr);
                    --busy_slots_counter; // table[p+M] devino vacía
                  }
                table.cut(MP); // eventualmente liberar memoria de table[p+M]
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
		     bool __remove_all_buckets, bool /* __with_resize */)
    throw(std::exception, std::length_error, std::domain_error,
          std::bad_alloc, std::overflow_error) 
  : table(__len), hash_fct(__hash_fct), cmp(__cmp), M(__len), N(0), 
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

 public:

    /** Instancia una tabla hash lineal genérica.

        Instancia una tabla hash lineal genérica de tamaño __len.

        @param[in] __hash_fct función hash.
        @param[in] __len tamaño inicial y mínimo de la tabla.
        @param[in] __upper_alpha umbral superior en que la tabla debe
        expandirse.  
        @param[in] __lower_alpha umbral inferior en que la tabla debe
        contraerse.  
        @param[in] __remove_all_buckets si es true, entonces se liberan
        las cuvetas de la tabla. De lo contrario, éstas permanecen
        intactas. Por omisión el valor es true.

        @throw length_error si __ es igual a superior que la máxima
        dimensión permitida para un arreglo dinámico.
        @throw domain_error si __upper_alpha es menor o igual a
        __lower_alpha.  
        @throw bad_alloc si no hay suficiente memoria
        @throw overflow_error si ocurre un desborde desde DynArray
        (causado por sus cálculos internos).
     */
  GenLinearHashTable(size_t       len                = Primes::DefaultPrime,
		     Hash_Fct_Ptr hash_fct           = Aleph::dft_hash_fct<Key>,
		     Cmp          cmp                = Cmp(),
                     float        lower_alpha        = hash_default_lower_alpha,
		     float        upper_alpha        = hash_default_upper_alpha,
		     bool         remove_all_buckets = true, 
		     bool         with_resize        = true)
    throw(std::exception, std::length_error, std::domain_error,
          std::bad_alloc, std::overflow_error) 
  : GenLinearHashTable(len, Hash_Fct(hash_fct), cmp, lower_alpha, upper_alpha,
		       remove_all_buckets, with_resize) {}

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

  /** \fn set_upper_alpha

      Cambia el umbral superior de carga en que la tabla comienza a
       expandirse. Dispara domain_error si el nuevo umbral es inferior
      que el umbral inferior (lower_alpha)
  */

  /** \fn set_lower_alpha

      Cambia el umbral inferior de carga en que la tabla comienza a
      contraerse. Dispara domain_error si el nuevo umbral es superior
      que el umbral superior (upper_alpha)
  */

      /// Vacía toda la tabla. Todas las cubetas son liberadas y el tamaño
      /// es ajustado al inicial.
  void empty() noexcept
  {
    while (not entries_list.is_empty())
      {    
	Bucket * bucket = Bucket::dlink_to_base(entries_list.remove_first_ne());
	bucket->del();             // borra de la lista en el arreglo
	bucket->get_link()->del(); // borra de entries list
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

  Bucket *
  search_in_bucket_list(BucketList * list, const Key & key) const noexcept
  {
    for (BucketItor it(*list); it.has_curr(); it.next_ne())
      {
        Bucket * bucket = static_cast<Bucket*>(it.get_curr_ne());
        if (cmp (key, bucket->get_key())) 
          return bucket; 
      }

    return nullptr;
  }

public:

      /// Busca la clave key en la tabla hash lineal. Retorna un puntero a
      /// la cubeta que contiene key si ésta se encuentra en la tabla;
      /// nullptr de lo contrario.
  Bucket * search(const Key & key) const noexcept
  {
    auto i = call_hash_fct(key);
    BucketList * list = table.test(i);
    if (list == nullptr) // ¿Ha sido escrita alguna vez table[i]?
      return nullptr; // No ==> el elemento no se encuentra en la tabla

    if (list->is_empty()) 
      return nullptr;

    return search_in_bucket_list(list, key);
  }

     /// Retorna la cantidad de elementos que tiene la tabla.
  const size_t & size() const noexcept { return N; }

      /// return true is table is empty
  bool is_empty() const noexcept { return N == 0; }

      /// Retorna el tamaño de la tabla.
  const size_t & capacity() const noexcept { return MP; }

      /// Retorna la cantidad de entradas vacía que tiene la tabla
  const size_t & busy_slots() const noexcept { return busy_slots_counter; }

      /// Retorna el nivel de expansiones que se han realizado sobre la
      /// tabla. 
  const size_t & expansions() const noexcept { return l; }

      /// Insert bucket in the table. Return bucket if bucket->get_key()
      /// is not in the table, nullptr otherwise
  Bucket * insert(Bucket * bucket) 
  {
    const int i       = call_hash_fct(bucket->get_key());
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
    const int i       = call_hash_fct(bucket->get_key());
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
  Bucket * remove_bucket(Bucket * bucket)
  {
    assert(bucket != nullptr);
    assert(search(bucket->get_key()) == bucket);

    Bucket * next = static_cast<Bucket*>(bucket->get_next());

    bucket->del();             // elimine de lista de colisiones

    if (next->is_empty()) // ¿lista de colisiones vacía?
      --busy_slots_counter; // sí ==> un slot vacío

    --N;
    contract();

    return bucket;
  }

public:

      /// Elimina la cubeta bucket. Atención: no se verifica si la cubeta
      /// pertenece a la tabla.
  Bucket * remove(Bucket * bucket)
  {
    bucket->get_link()->del(); // elimine de entries_list
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
              for (BucketItor it(list); it.has_curr(); it.next_ne())
                {
                  Bucket * bucket = static_cast<Bucket*>(it.get_curr_ne());
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

       /// El tipo de conjunto sobre el cual se itera.
    using Set_Type = GenLinearHashTable;

      /// El tipo de elemento que retorna get_curr().
    using Item_Type = Bucket *;

        /// Instancia un iterador sobre la tabla hash table.
    Iterator(const GenLinearHashTable & table) noexcept
      : Dlink::Iterator(const_cast<Dlink &>(table.entries_list)), 
	hash_table(& const_cast<GenLinearHashTable &>(table))
    {
      // Empty
    }
    
        /// Instancia un iterador vacío.
    Iterator() noexcept { /* Empty */ }

        /// Retorna la cubeta actual.
    Bucket * get_curr() const
    {
      return Bucket::dlink_to_base(this->Dlink::Iterator::get_curr());
    }

    /// Retorna la cubeta actual.
    Bucket * get_curr_ne() const noexcept
    {
      return Bucket::dlink_to_base(this->Dlink::Iterator::get_curr_ne());
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


    /** Tabla hash lineal con cubetas sin destructor virtual.

        Básicamente, una tabla hash lineal es una tabla hash con
        resolución de colisiones por encadenamiento separado pero con la
        diferencia de que el tamaño de la tabla aumenta dinámicamente
        para asegurar que el factor de carga, típicamente llamado
        \f$\alpha\f$, esté delimitado entre dos valores inferior y
        superior, respectivamente.

        Internamente, la tabla utiliza un arreglo dinámico del tipo
        DynArray. Esto conlleva ahorros de memoria por entradas de la
        tabla que no hayan sido escritas.

        @param Key el tipo de clave por el cual se indiza la tabla.
        @param Cmp clase de comparación entre las claves.

        @see DynArray LinearHashTableVtl
        @ingroup Hash
    */
template <typename Key, class Cmp = Aleph::equal_to<Key>>
using LinearHashTable = GenLinearHashTable<Key, LinHashBucket, Cmp>;


    /** Tabla hash lineal con destructor virtual en sus cubetas.

        Básicamente, una tabla hash lineal es una tabla hash con
        resolución de colisiones por encadenamiento separado pero con la
        diferencia de que el tamaño de la tabla aumenta dinámicamente
        para asegurar que el factor de carga, típicamente llamado
        \f$\alpha\f$, esté delimitado entre dos valores inferior y
        superior, respectivamente.

        Internamente, la tabla utiliza un arreglo dinámico del tipo
        DynArray. Esto conlleva ahorros de memoria por entradas de la
        tabla que no hayan sido escritas.

        @param Key el tipo de clave por el cual se indiza la tabla.
        @param Cmp clase de comparación entre las claves.

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

