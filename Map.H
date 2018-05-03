
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

# ifndef ALEPH_MAP_H
# define ALEPH_MAP_H
/*
	   Aleph implementation of map C++ standard container
*/

# include <cstdlib>
# include <ahFunction.H>
# include <ah_stdc++_utils.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>


namespace Aleph
{


  /** Implementación Aleph del tipo estándar map<T> basada en árboles
      binarios de búsqueda con rangos.

      Esta es una implantación parcial de la clase estándar C++ map<Key,Elem>,
      la cual está basada en árboles binarios de búsqueda con rangos. Por 
      omisión se emplea la clase Treap_Rk, con la cual en medidas de
      rendimiento ha probado ser más rápida que sus contrapartes gnu y
      Boost. 

      map<Key,Elem> implanta un mapeo desde claves de tipo Key, que no
      se pueden repetir, a un mapeo de elementos de tipo Elem, los
      cuales eventualmente pueden repetirse si se asocian a claves
      distintas. 

      Los elementos del mapeo son pares ordenados. Un mapeo es sinónimo
      de función desde Key-->Elem. Esta versión el mapeo usa el tipo
      Pair de Aleph, cuya implantación está basada en la de gnu.

      Un mapeo m es acesible mediante el operador m[key], donde key es
      la clave de indización. El resultado de m[key] es el elemento
      asociado a la clave key.

      @param Key el tipo de dato que almacenará el mapeo. 
      @param Elem el tipo de elemento a indizar según una clave.
      @param Compare el criterio de comparación entre las claves de
      tipo Key. Por omisión se implanta Key::operator<(const Key &).
      @param Tree la clase de árbol binario de búsqueda con rangos que
      instrumentará el mapeo. Por omisión si usa Treap_Rk

      @see Treap_Rk set multiset multimap
      @ingroup stl
   */
    template <class Key, 
	      class Elem, 
	      class Compare = Aleph::less<Key>,
	      template <class, class> class Tree = Treap_Rk
    >
class map
{
  struct Cmp
  {
    bool
    operator () (const std::pair<Key, Elem> & __x, 
		 const std::pair<Key, Elem> & __y) const noexcept
    { 
      return Compare () (__x.first, __y.first); 
    }
  };

public:

      /// Tipo par almacenado en mapeo.
  typedef std::pair<Key, Elem> Pair;

      /// Tipo a exportar como tipo del contenedor
  typedef Pair value_type;

      /// Tipo generico referencia al tipo de elemento que almacena el
      /// mapeo. 
  typedef typename map::value_type & reference;

      /// Tipo generico referencia constante
  typedef const typename map::value_type & const_reference;

  typedef size_t size_type;

      /// El tipo de clave de indización.
  typedef Key key_type;

      /// El tipo de rango del mapeo.
  typedef Elem mapped_type;

private:

  typedef Tree<Pair, Cmp> Tree_Type;

  typedef typename Tree_Type::Node Node;

  Node_Pool<Node> node_pool;

  mutable Tree_Type tree;

  Node * search_in_tree (const Key & k)
  {
    return tree.search (Pair(k, Elem()));
  }

public:

    /** Iterador sobre un mapeo.

	@ingroup stl
    */
  class iterator
  {
  private:

    friend class map<Key, Elem>;
    
    typedef typename Tree_Type::Iterator Iterator;
    
    Iterator itor;
    bool underflow;
    bool overflow;

    void init_flags ()
    {
      if (itor.has_current () )
	underflow = overflow = false;
      else
	underflow = overflow = true;
    }

    void goto_begin ()
    {
      itor.reset_first ();
      init_flags ();
    }

    void goto_last ()
    {
      itor.reset_last ();
      init_flags ();
    }

    void goto_end ()
    {
      itor.reset_last ();
      init_flags ();
      if (not overflow) 
	itor.next ();
      overflow = true;
    }

    void forward ()
    {
      if (underflow)
	{
	  goto_begin ();
	  return;
	}
      
      itor.next ();

      if (not itor.has_current () )
	overflow = true;
    }

    void backward ()
    {
      if (overflow)
	{
	  goto_last ();
	  return;
	}
      
      itor.prev ();

      if (not itor.has_current () )
	underflow = true;
    }

    iterator (Tree_Type & _tree, Node * node) 
      : itor (_tree, node), underflow (false), overflow (false)
    {
      /* empty */
    }

  public:

        /// El tipo de elemento que maneja el iterador
    typedef typename map::value_type   value_type;

        /// El tipo numérico para manejar aritmética.
    typedef typename map::size_type    difference_type;

        /// Tipo de dato puntero a elemento actual.
    typedef typename map::value_type * pointer;

        /// Tipo de dato referencia a elemento actual.
    typedef typename map::reference    reference;

        /// Constructor vacío; no tiene validez si no se asocia un mapeo
    iterator () { /* empty */ }

    iterator (Tree_Type & tree) : itor (tree)
    {
      init_flags ();
    }

        /// Proporciona una referencia al elemento actual.
    const Pair & operator * ()
    {
      return KEY(itor.get_curr());
    }

        /// "Dereferencia" un puntero al elemento actual.
    const Pair * operator -> ()
    {
      return &KEY(itor.get_curr());
    }

        /// Avanza el iterador una posición hacia adelante, luego carga
        /// el elemento actual.
    iterator operator ++ ()
    {
      forward ();
      return *this;
    }

        /// Carga elemento actual luego avanza el iterador hacia delante.    
    iterator operator ++ (int)
    {
      iterator ret_val = *this;
      forward ();
      return ret_val;
    }

        /// Avanza el iterador una posición hacia atrás, luego carga 
        /// elemento actual.
    iterator operator -- ()
    {
      backward ();
      return *this;
    }

        /// Carga elemento actual luego avanza el iterador hacia atrás.
    iterator operator -- (int)
    {
      iterator ret_val = *this;
      backward ();
      return ret_val;
    }
    
        /// Avanza el iterador n elementos hacia delante y carga el nuevo
        /// elemento actual.
    iterator operator += (const size_type & n)
    {
      itor.reset_to_pos (itor.get_current_position () + n);
      return *this;
    } 

        /// Avanza el iterador n elementos hacia atrás y carga el nuevo
        /// elemento actual.
    iterator operator -= (const size_type & n)
    {
      itor.reset_to_pos (itor.get_current_position () - n);
      return *this;
    } 

        /// Retorna true si los iterador tienen el mismo estado.
    bool operator == (const iterator & _itor) const
    {
      return itor == _itor.itor;
    }

        /// Retorna true si los iterador tienen estados distintos.
    bool operator != (const iterator & _itor) const
    {
      return not (itor == _itor.itor);
    }

    bool verify (const map & _map) const
    {
      return itor.verify ( (Tree_Type*) &_map.tree); 
    }

    bool verify (const iterator & it) const
    {
      return itor.verify (it.itor);
    }
  };

      /// Constructor vacío.
  map () : node_pool(100) { /* empty */ }
 
      /// Instancia una copia del mapeo c.
  map (const map & c) : node_pool(100)
  {
    tree.getRoot () = copyRec (c.tree.getRoot () );
  }


      /// Destructor; todos los elementos son eliminados y la memoria es
      /// liberada. 
  ~map () 
  { 
    destroyRec (tree.getRoot () );
  }  

      /// Asigna todo el contenido de c a this.
  map & operator = (const map& c)
  {
    if (this == &c)
      return *this;

    destroyRec (tree.getRoot ());

    tree.getRoot () = copyRec (c.tree.getRoot ()); 

    return *this;
  }

      /// Retorna la cantidad de elementos que contiene el mapeo.
  size_t size () const { return COUNT (tree.getRoot () ); }

      /// Retorna true si el contenedor esta vacío.
  bool empty () const 
  {
    return size() == 0;
  }

      /// Retorna true si el mapeo this tiene exactamente los mismos
      /// elementos del mapeo c.
  bool operator == (const map & c) const
  {
    if (size () != c.size () )
      return false;

    typename Tree_Type::Iterator itor1 (tree), itor2 (c.tree);
    
    while (itor1.has_current () and itor2.has_current () )
    {
      if (KEY (itor1.get_current () ) != KEY (itor2.get_current () ) )
	return false;

      itor1.next ();
      itor2.next ();
    }
    
    assert(not itor1.has_current () and not itor2.has_current () );

    return true;
  }

      /// Retorna true si el mapeo this no contiene exactamente los mismos
      /// elementos del mapeo c.
  bool operator != (const map & c) const
  {
    return not (*this == c);
  }

      /// Retorna true si los elementos ordenados del mapeo this son
      /// menores que los del mapeo c.
  bool operator < (const map & c) const
  {
    typename Tree_Type::Iterator itor1 (tree), itor2 (c.tree);

    while (itor1.has_current () and itor2.has_current () )
    {
      if (KEY (itor1.get_current () ) < KEY (itor2.get_current () ))
	return true;
      else if (KEY (itor2.get_current () ) < KEY (itor1.get_current () ))
	return false;
      
      // En este caso las claves sons iguales
      itor1.next ();
      itor2.next ();
    }
    
    if (not itor1.has_current ())
      return true;

    return false;
  }

      /// Retorna true si los elementos ordenados del mapeo this son
      /// mayores que los del mapeo c.
  bool operator > (const map & c)
  {
    return not (*this == c or *this < c);
  }

      /// Retorna true si los elementos ordenados del mapeo this son
      /// menores o iguales que los del mapeo c.
  bool operator <= (const map & c)
  {
    return not (*this > c );
  }

      /// Retorna true si los elementos ordenados del mapeo this son
      /// mayores o iguales que los del mapeo c.
  bool operator >= (const map & c)
  {
    return not (*this < c);
  }

      /** Retorna la cantidad de instancias de key que hay dentro del
	  mapeo; o sea, por definición de set<T>, 1 si el elemento
	  está dentro del mapeo; 0 de lo contrario.

	  Esta función es en sí una búsqueda absoluta cuyo resultado es
	  0 o 1.
	  
	  @param[in] key valor de clave a buscar.
	  @return 1 si el elemento se encuentra en el mapeo; 0 de lo
	  contrario. 
      */
  size_type count (const Key & key)
  {
    if (search_in_tree (key) != nullptr)
      return 1;

    return 0;
  } 

      /** Retorna un iterador posicionado sobre el elemento del mapeo key.

	  find(key) busca el elemento key en el mapeo. Si éste es
	  encontrado, entonces se retorna un iterador posicionado sobre
	  el elemento del mapeo cuyo valor es key; de lo contrario,
	  se retorna un iterador desbordado.

	  @param[in] key valor a buscar dentro del mapeo.
	  @return iterador posicionado sobre el elemento con valor key
	  o iterador desbordado si key no se encuentra dentro del
	  mapeo. 
       */
  iterator find (const Key & key) 
  { 
    Node * p = search_in_tree (key);

    if (p == nullptr)
      return end ();

    return iterator (tree, p);
  }
      /// Retorna un iterador posicionado sobre el predecesor de key en el
      /// mapeo; independientemente de que key se encuentre o no
      /// en el mapeo.
  iterator lower_bound (const Key & key)
  {
    Node * p = 
      search_rank_parent <Node, Cmp> (tree.getRoot(), Pair(key, Elem()));

    return iterator(tree, p);
  }

      /// Retorna un iterador posicionado sobre el sucesor de key en el
      /// mapeo; independientemente de que key se encuentre o no
      /// en el mapeo.
  iterator upper_bound (const Key & key)
  {
    Node * p = 
      search_rank_parent <Node, Cmp> (tree.getRoot(), Pair(key, Elem()));

    iterator upper(tree, p);
    
    if (KEY (p).first == key)
      upper.itor.next();

    return upper;
  }

      /// Intercambia en tiempo constante todos los elementos de this
      /// con los de c.  
  void swap (map & c)
  {
    tree.swap (c.tree);
  }

      /// Retorna un iterador posicionado al primer elemento del mapeo.
  iterator begin ()
  {
    return iterator (tree);
  }

      /// Retorna un iterador posicionado al último elemento del mapeo.
  iterator end ()
  {
    iterator last (tree);
    last.goto_end ();

    return last;
  }

      /** Inserta key en el mapeo.

	  insert(key) inserta en el mapeo this una copia del
	  elemento key y retorna un par con un iterador posicionado
	  según la inserción y un valor lógico que indica si la
	  inserción fue o no exitosa.

	  @param[in] key valor a insertar en el mapeo.
	  @return un par con un iterador y un valor lógico. Si key no
	  se encuentra dentro del mapeo, entonces el iterador está
	  posicionado sobre el elemento recién insertado y el valor
	  lógico es true; de lo contrario, el iterador está desbordado y
	  el valor lógico es false.
	  @throw bad_alloc si no hay suficiente memoria.
       */
  std::pair<iterator, bool> insert (const Pair & key)
  {
    Node * p = node_pool.allocate(key);

    if (tree.search_or_insert(p) != p)
    {
      node_pool.deallocate(p);
      return std::pair<iterator, bool> (iterator (tree), false);
    }

    return std::pair<iterator, bool> (iterator(tree, p), true);
  }

      /** Instancia un mapeo con los elementos comprendidos en el
	  rango [beg..end).

	  Este constructor instancia un mapeo con elementos iniciales
	  copiados del contenedor asociado los iteradores beg y end.

	  @param[in] beg iterador posicionado en el primer elemento del
	  contenedor a copiar.
	  @param[in] end iterador posicionado en el último más un
	  elemento del contenedor a copiar.
	  @throw domain_error si los iteradores beg y end no están
	  asociados al mismo contenedor.
	  @throw bad_alloc si no hay suficiente memoria.
	  @note El elemento posicionado por el iterador end no se incluye.
       */
      template <typename Itor> 
  map (Itor beg, const Itor & end) : map()
  {
    while (beg != end)
      insert(*beg++);
  }
    
      /** Inserta key en el mapeo.

	  insert(pos, key) inserta en el mapeo this una copia del
	  elemento key y retorna un par con un iterador posicionado
	  según la inserción y un valor lógico que indica si la
	  inserción fue o no exitosa. 

	  En realidad, el iterador es irrelevante en el caso de set<T>,
	  pues no se admiten duplicidades. Este esquema de inserción
	  tiene sentido en multiset y multimap. Se provee por razones de
	  compatibilidad y economía de código.

	  @param[in] key valor a insertar en el mapeo.
	  @return un iterador posicionado sobre el elemento recién
	  insertado si key no se encontraba dentro del mapeo; de lo
	  contrario, el iterador se entrega desbordado.
	  @throw bad_alloc si no hay suficiente memoria.
       */  
  iterator insert (iterator /* pos */, const Pair& key)
  {
    return insert(key);
  }

      /** Inserta en el mapeo los elementos de un contenedor
	  comprendidos en el rango [beg..end).

	  insert(beg,end) toma un rango de  elementos de un contenedor
	  (set, map, multiset, multimap, list o vector) asociados a los
	  iteradores beg y end, e inserta todos los elementos del rango
	  en el mapeo this. 

	  @param[in] beg iterador posicionado sobre el elemento inicio
	  del rango.
	  @param[in] end iterador posicionado sobre el último más un
	  elemento final del rango.
	  @throw domain_error si los iteradores beg y end no están
	  asociados al mismo contenedor.
	  @throw bad_alloc si no hay suficiente memoria.
	  @note El elemento posicionado por el iterador end no se incluye.
       */
      template <typename Itor>
  void insert (Itor beg, const Itor & end)
  {
    while (beg != end)
      insert (*beg++);
  }

      /** Borra key del mapeo.

	  erase(key) borra el elemento key del mapeo.

	  @param[in] key valor del elemento a borrar.
	  @return 1 si key  estaba dentro del mapeo; o de lo
	  contrario.
	  @note En realidad el valor de retorno tiene sentido para
	  multiset y multimap en el cual el valor de retorno es la
	  cantidad instancias del elemento que fueron borradas.
      */
  size_type erase (const Key & key)
  {
    Node * p = tree.remove (Pair(key, Elem()));

    if (p == nullptr)
      return 0;
	
    node_pool.deallocate(p);

    return 1;
  }

      /** Borra del mapeo el valor actual del iterador pos.

	  erase(pos) borra del mapeo el elemento correspondiente al
	  valor actual del iterador pos.

	  @param[in] pos iterador cuyo valor actual será borrado.
	  @throw domain_error si el iterador no está asociado al
	  mapeo this.
	  @throw underflow_error si el iterador está desbordado por la
	  izquierda.
	  @throw overflow_error si el iterador está desbordado por la
	  derecha. 
      */
  void erase (iterator pos)
  {
    erase (KEY (pos.itor.get_current ()).first);
  }

  /** Borra del mapeo el rango comprendido [beg..end).

      erase(beg,end) borra eficientemente todos los elementos del
      mapeo delimitados por los iteradores beg y end.

      @param[in] beg iterador posicionado sobre el elemento inicio del
      rango.
      @param[in] end iterador posicionado sobre el último más un
      elemento del rango.
      @throw domain_error si los iteradores beg y end no están
      asociados a this.
      @note El elemento posicionado por el iterador end no se incluye.
   */
  iterator erase (const iterator & beg, const iterator & end)
  {
    Aleph::verify_iterators (beg, end);
    Aleph::verify_container_and_iterator (*this, beg);

    iterator ret_val = end;

    const size_t pos_beg = beg.itor.get_current_position ();
    const size_t pos_end = end.itor.get_current_position ();

    Node * removed_tree = tree.remove (pos_beg, pos_end - 1);

    destroyRec (removed_tree);

    return ret_val;
  }

      /// Borra todos los elementos del mapeo.        
  void clear ()
  {
    destroyRec (tree.getRoot());
  }
  
private:
  
  struct Proxy
  {
    map  * map_ptr;
    Node * node;
    Key    key;

    Proxy (map * m_ptr, const Key & _key) : map_ptr (m_ptr), key (_key)
    {
      node = map_ptr->search_in_tree(key);
    }

    Proxy & operator = (const Elem & elem)
    {
      if (node == nullptr)
        {
	  node = map_ptr->node_pool.allocate(std::make_pair(key, elem));
	  map_ptr->tree.insert(node);
	}
      else
        KEY(node).second = elem;

      return *this;
    }

    Proxy & operator = (const Proxy & proxy)
    {
      if (this == &proxy)
	return *this;

      if (proxy.node == nullptr)
        throw std::domain_error("key not found");;

      if (map_ptr == proxy.map_ptr and key == proxy.key)
        return *this; // They are the same

      if (node == nullptr)
        {
	  node = map_ptr->node_pool.allocate(KEY(proxy.node));
	  map_ptr->tree.insert(node);
	} 
      else
        KEY(node).second = KEY(proxy.node).second;

      return *this;
    }

    operator Elem & () const
    {
      if (node == nullptr)
        throw std::domain_error ("key not found");;

      return KEY (node).second;
    }    
  };

public:

  const Proxy operator [] (const Key & key) const 
    Exception_Prototypes (std::domain_error)
  {
    return Proxy (this, key);
  }

  Proxy operator [] (const Key & key) 
    Exception_Prototypes (std::domain_error)
  {
    return Proxy (this, key);
  }
};



} // end namespace Aleph


# endif // ALEPH_MAP_H
