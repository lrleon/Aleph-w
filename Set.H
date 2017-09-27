/*
	Aleph implementation of set C++ standard container
*/

# ifndef AH_SET_H
# define AH_SET_H

# include <ah_stdc++_utils.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>

namespace Aleph
{

  /** Implementación Aleph del tipo estándar set<T> basada en árboles
      binarios de búsqueda con rangos.

      Esta es una implantación parcial de la clase estándar C++ set<T>
      basada en árboles binarios de búsqueda con rangos. Por
      omisión se emplea la clase Treap_Rk, con la cual en medidas de
      rendimiento ha probado ser más rápida que sus contrapartes gnu y
      Boost. 

      set<T> implanta un conjunto de claves de tipo T en el cual no se
      permiten repitencias.

      @param T el tipo de dato que almacenará el conjunto.
      @param Compare el criterio de comparación entre los elementos de
      tipo T. Por omisión se implanta T::operator<(const T&).
      @param Tree la clase de árbol binario de búsqueda con rangos que
      instrumentará el conjunto. Por omisión si usa Treap_Rk

      @see Treap_Rk map multiset multimap
      @ingroup Contenedores
   */
    template <typename T, 
	      class Compare = Aleph::less<T>,
	      template <class, class> class Tree = Treap_Rk>
class set
{
private:

  Tree<T, Compare> tree;

  typedef typename Tree<T, Compare>::Node Node;

  Node_Pool<Node> node_pool;

public:

      /// El tipo dato de los elementos del conjunto.
  typedef T value_type;

      /// Tipo genérico de puntero a elemento del conjunto.
  typedef typename set::value_type * pointer;

      /// Tipo genérico de referencia a elemento del conjunto.
  typedef typename set::value_type & reference;

      /// Tipo genérico de referencia constante a elemento del conjunto.
  typedef const typename set::value_type & const_reference;

      /// Tipo numérico para los tamaños (natural).
  typedef size_t size_type;

      /// El tipo dato de los elementos del conjunto.
  typedef T key_type;

  class iterator
  {
  private:

    friend class set<T, Compare, Tree>;
    
    typedef typename Tree<T, Compare>::Iterator Iterator;
    
    const Tree<T, Compare> * tree;
    Iterator  itor;
    bool underflow;
    bool overflow;

    iterator(const Tree<T, Compare> & _tree, Node * node) 
      : tree (&_tree), itor (_tree, node), underflow (false), overflow (false)
    {
      /* empty */
    }

    void init_flags ()
    {
      if (tree->size () == 0)
	underflow = overflow = true;
      else
	underflow = overflow = false;
    }

    iterator(const Tree<T, Compare> & _tree) : tree(&_tree), itor(_tree)
    {
      init_flags ();
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

      if (not itor.has_current() )
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

      if (not itor.has_current() )
	underflow = true;
    }

  public:

        /// El tipo de dato que contiene el conjunto.
    typedef typename set<T>::value_type   value_type;

        /// Tipo numérico usado para diferencias entre posiciones de
        /// iteradores. 
    typedef typename set<T>::size_type       difference_type;

        /// Tipo apuntador a elemento dentro del conjunto.
    typedef typename set<T>::value_type *    pointer;

        /// Tipo referencia a elemento dentro del conjunto.
    typedef typename set<T>::reference       reference;

        /// Tipo referencia a elemento dentro del conjunto.
    typedef const typename set<T>::reference const_reference;

        /// Constructor vacío; no tiene validez si no se asocia un conjunto.
    iterator () : tree (nullptr), underflow (true), overflow (true) 
    {
      /* empty */ 
    }
    
        /// Proporciona una referencia al elemento actual.
    const T & operator * ()
    {
      return KEY (itor.get_current () );
    }

        /// "Dereferencia" un puntero al elemento actual.
    const T * operator -> ()
    {
      return & KEY (itor.get_current () );
    }

        /// Avanza el iterador una posición hacia adelante, luego carga
        /// elemento actual.
    iterator operator ++ ()
    {
      forward ();
      return *this;
    }

        /// Carga elemento actual luego avanza el iterador hacia delante.
    iterator operator ++ (int)
    {
      iterator return_value = *this;
      forward ();
      return return_value;
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
      iterator return_value = *this;
      backward ();
      return return_value;
    }
    
        /// Avanza el iterador n elementos hacia delante y carga el nuevo
        /// elemento actual.
    iterator operator += (const size_type & n)
    {
      itor.reset_to_pos(itor.get_current_position () + n);
      return *this;
    } 

        /// Avanza el iterador n elementos hacia atrás y carga el nuevo
        /// elemento actual.
    iterator operator -= (const size_type & n)
    {
      itor.reset_to_pos(itor.get_current_position () - n);
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

    bool verify (const set & _set) const
    {
      return itor.verify ( (Tree<T, Compare>*) &_set.tree);
    }

    bool verify (const iterator & it) const
    {
      return itor.verify (it.itor);
    }
  };

      /// Constructor vacío.
  set () : node_pool(100) { /* empty */  }

      /// Instancia una copia del conjunto c.
  set (const set & c) : set()
  {
    void * ptr = c.tree.getRoot();
    tree.getRoot () = copyRec ((Node*) ptr);
  }

      /// Destructor; todos los elementos son eliminados y la memoria es
      /// liberada. 
  ~set () 
  { 
    destroyRec(tree.getRoot () );
  }  

      /// Retorna la cantidad de elementos que contiene el conjunto.
  size_type size () const { return COUNT(tree.getRoot()); }

      /// Retorna true si el contenedor esta vacío.
  bool empty () const { return size() == 0; }

      /// retorna true si el conjunto this contiene exactamente los mismos
      /// elementos del conjunto c.
  bool operator == (const set & c) const
  {
    if (this == &c)
      return true;

    if (size () != c.size () )
      return false;

    typename Tree<T, Compare>::Iterator it1 (tree), it2 (c.tree);

    while (it1.has_current () and it2.has_current () )
      {
	if (Aleph::no_equals<T, Compare> (KEY(it1.get_curr()), 
					  KEY(it2.get_curr())))
	  return false;

	it1.next ();
	it2.next ();
      }
    
    return true;
  }

      /// Retorna true si el conjunto this no contiene exactamente los mismos
      /// elementos del conjunto c.
  bool operator != (const set & c) const
  {
    return not (*this == c);
  }

      /// Retorna true si los elementos ordenados del conjunto this son
      /// menores que los del conjunto c.
  bool operator < (const set & c) const
  {
    if (this == &c)
      return false;

    typename Tree<T, Compare>::Iterator itor1 (tree), itor2 (c.tree);

    while (itor1.has_current () and itor2.has_current () )
      {
	if (Compare () (KEY(itor1.get_curr()), KEY(itor2.get_curr())))
	  return true;
	else if (Compare () (KEY(itor2.get_curr()), KEY(itor1.get_curr())))
	  return false; 

	itor1.next ();
	itor2.next ();
      }
    
    if (itor1.has_current () ) /* |*this| >= |c| */
      return false;

    return itor2.has_current ();
  }

      /// Retorna true si los elementos ordenados del conjunto this son
      /// mayores que los del conjunto c.
  bool operator > (const set & c) const
  {
    return not (*this == c or *this < c);
  }

      /// Retorna true si los elementos ordenados del conjunto this son
      /// menores o iguales que los del conjunto c.
  bool operator <= (const set & c) const
  {
    return not (*this > c );
  }

      /// Retorna true si los elementos ordenados del conjunto this son
      /// mayores o iguales que los del conjunto c.
  bool operator >= (const set & c) const
  {
    return not (*this < c);
  }
  
      /** Retorna la cantidad de instancias de value que hay dentro del
	  conjunto; o sea, por definición de set<T>, 1 si el elemento
	  está dentro del conjunto; 0 de lo contrario.

	  Esta función es en sí una búsqueda absoluta cuyo resultado es
	  0 o 1.
	  
	  @param[in] value valor a buscar.
	  @return 1 si el elemento se encuentra en el conjunto; 0 de lo
	  contrario. 
      */
  size_type count (const T & value) const
  {
    if (tree.search (value) == nullptr)
      return 0;

    return 1;
  } 

      /** Retorna un iterador posicionado sobre el elemento del conjunto
	  value.

	  find(value) busca el elemento value en el conjunto. Si éste es
	  encontrado, entonces se retorna un iterador posicionado sobre
	  el elemento del conjunto cuyo valor es value; de lo contrario,
	  se retorna un iterador desbordado.

	  @param[in] value valor a buscar dentro del conjunto
	  @return iterador posicionado sobre el elemento con valor value
	  o iterador desbordado si value no se encuentra dentro del
	  conjunto. 
       */
  iterator find(const T & value) const
  { 
    Node * node = tree.search (value);

    if (node == nullptr)
      return end();

    iterator itor (tree);
    itor.itor.reset_to_node (node);

    return itor;
  }

      /// Retorna un iterador posicionado sobre el predecesor de value en el
      /// conjunto; independientemente de que value se encuentre o no
      /// en el conjunto.
  iterator lower_bound(const T & value) const
  {
    if (size() == 0)
      return end ();

    Node * p = search_rank_parent(tree.getRoot(), value);

    return iterator (tree, p);
  }

      /// Retorna un iterador posicionado sobre el sucesor de value en el
      /// conjunto; independientemente de que value se encuentre o no
      /// en el conjunto.
  iterator upper_bound(const T & value) const
  {
    if (size () == 0)
      return end ();

    Node * p = search_rank_parent(tree.getRoot(), value);

    iterator upper(tree, p);
    
    if (KEY(p) == value)
      upper.itor.next();

    return upper;
  }

      /// Asigna todo el contenido de c a this.
  set & operator = (set & c)
  {
    if (this == &c)
      return *this;

    destroyRec (tree.getRoot () );

    tree.getRoot () = copyRec (c.tree.getRoot () ); 

    return *this;
  }

      /// Intercambia en tiempo constante todos los elementos de this
      /// con los de c.  
  void swap (set & c)
  {
    std::swap (tree.getRoot (), c.tree.getRoot () );
  }

      /// Retorna un iterador posicionado al primer elemento del conjunto.
  iterator begin () const
  {
    return iterator (tree);
  }

      /// Retorna un iterador posicionado al último elemento del conjunto.
  iterator end () const
  {
    iterator last (tree);
    last.goto_end ();

    return last;
  }

      /** Inserta value en el conjunto.

	  insert(value) inserta en el conjunto this una copia del
	  elemento value y retorna un par con un iterador posicionado
	  según la inserción y un valor lógico que indica si la
	  inserción fue o no exitosa.

	  @param[in] value valor a insertar en el conjunto.
	  @return un par con un iterador y un valor lógico. Si value no
	  se encuentra dentro del conjunto, entonces el iterador está
	  posicionado sobre el elemento recién insertado y el valor
	  lógico es true; de lo contrario, el iterador está desbordado y
	  el valor lógico es false.
	  @throw bad_alloc si no hay suficiente memoria.
       */
  std::pair<iterator, bool> insert (const T & value)
  {
    Node * p = node_pool.allocate(value);

    iterator itor (tree);

    if (tree.search_or_insert(p) != p) // se inserta o ya está en el conjunto?
      {    // value ya está dentro del conjunto
	node_pool.deallocate(p);
	itor.itor.reset_to_key (value);
	
	return std::pair<iterator, bool> (itor, false);
      }

    itor.itor.reset_to_node(p);

    return std::pair<iterator, bool> (itor, true);
  }


      /** Instancia un conjunto con los elementos comprendidos en el
	  rango [beg..end).

	  Este constructor instancia un conjunto con elementos iniciales
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
  set (Itor beg, const Itor & end) : set()
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      insert(*beg++) ;
  }
    
      /** Inserta value en el conjunto.

	  insert(pos, value) inserta en el conjunto this una copia del
	  elemento value y retorna un par con un iterador posicionado
	  según la inserción y un valor lógico que indica si la
	  inserción fue o no exitosa. 

	  En realidad, el iterador es irrelevante en el caso de set<T>,
	  pues no se admiten duplicidades. Este esquema de inserción
	  tiene sentido en multiset y multimap. Se provee por razones de
	  compatibilidad y economía de código.

	  @param[in] value valor a insertar en el conjunto.
	  @return un iterador posicionado sobre el elemento recién
	  insertado si value no se encontraba dentro del conjunto; de lo
	  contrario, el iterador se entrega desbordado.
	  @throw bad_alloc si no hay suficiente memoria.
       */  
  iterator insert (const iterator & /* pos */, const T & value)
  {
    Node * p = node_pool.allocate(value);

    iterator _itor(tree);

    if (tree.search_or_insert(p) != p)
      { // clave duplicada
	node_pool.deallocate(p);
	_itor.itor.reset_to_key(value);
      }
    else
      _itor.itor.reset_to_node(p);   
  
    return _itor;    
  }

      /** Inserta en el conjunto los elementos de un contenedor
	  comprendidos en el rango [beg..end).

	  insert(beg,end) toma un rango de  elementos de un contenedor
	  (set, map, multiset, multimap, list o vector) asociados a los
	  iteradores beg y end, e inserta todos los elementos del rango
	  en el conjunto this. 

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
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      insert(*beg++);
  }

      /** Borra value del conjunto.

	  erase(value) borra el elemento value del conjunto.

	  @param[in] value valor del elemento a borrar.
	  @return 1 si value  estaba dentro del conjunto; o de lo
	  contrario.
	  @note En realidad el valor de retorno tiene sentido para
	  multiset y multimap en el cual el valor de retorno es la
	  cantidad instancias del elemento que fueron borradas.
      */
  size_type erase (const T & value)
  {
    Node * p = tree.remove (value);

    if (p == nullptr)
      return 0;
	
    node_pool.deallocate(p);

    return 1;
  }

      /** Borra del conjunto el valor actual del iterador pos.

	  erase(pos) borra del conjunto el elemento correspondiente al
	  valor actual del iterador pos.

	  @param[in] pos iterador cuyo valor actual será borrado.
	  @throw domain_error si el iterador no está asociado al
	  conjunto this.
	  @throw underflow_error si el iterador está desbordado por la
	  izquierda.
	  @throw overflow_error si el iterador está desbordado por la
	  derecha. 
      */
  void erase (iterator pos)
  {
    Aleph::verify_container_and_iterator (*this, pos);

    node_pool.deallocate(pos.itor.del());
  }
  
      /** Borra del conjunto el rango comprendido por [beg..end).

	  erase(beg,end) borra eficientemente todos los elementos del
	  conjunto delimitados por los iteradores beg y end.

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
    Aleph::verify_container_and_iterator (*this, beg);
    Aleph::verify_iterators (beg, end);

    iterator ret_val = end;

    const size_t pos_beg = beg.itor.get_current_position ();
    const size_t pos_end = end.itor.get_current_position ();

    Node * removed_tree = tree.remove(pos_beg, pos_end - 1);

    destroyRec(removed_tree);

    return ret_val;
  }

      /// Borra todos los elementos del conjunto.    
  void clear ()
  {
    destroyRec(tree.getRoot());
  }
};


  /** Distancia entre dos iteradores.

      distance(it1,it2) calcula el número de elementos que hay entre las
      posiciones o elementos actuales de los iteradores it1 y it2.

      @param[in] it1 primer iterador.
      @param[in] it2 segundo iterador.
      @return número de elementos que separan a ambos iteradores.
      @throw domain_error si los iteradores no están asociados al mismo
      contenedor. 
      @ingroup Contenedores
   */
    template <typename T>
typename iterator_traits<typename set<T>::iterator>::difference_type
distance(typename set<T>::iterator it1, typename set<T>::iterator it2)
{
  Aleph::verify_iterators(it1, it2);

  return it2.itor.get_current_position() - it1.itor.get_current_position();
}

}

# endif // AH_SET_H
