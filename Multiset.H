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

/** @file Multiset.H
 *  @brief STL-compatible multiset container (legacy).
 *
 *  @deprecated This is a legacy Aleph implementation of std::multiset.
 *  It will be deprecated in future versions. For new code, prefer using
 *  `std::multiset` directly or `Aleph::DynSetTree` with duplicate support.
 *
 *  Aleph implementation of std::multiset allowing duplicate elements
 *  with O(log n) operations using balanced BST.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef AH_MULTISET_H
# define AH_MULTISET_H

# include <memory>
# include <ah_stdc++_utils.H>
# include <tpl_dnode.H>
# include <tpl_treapRk.H>
# include <tpl_nodePool.H>

namespace Aleph
{

  /** Implementación Aleph del tipo estándar multiset<T> basada en árboles
      binarios de búsqueda con rangos.

      Esta es una implantación parcial de la clase estándar C++ multiset<T>
      basada en árboles binarios de búsqueda con rangos. Por
      omisión se emplea la clase Treap_Rk, con la cual, en medidas de
      rendimiento, ha probado ser más rápida que sus contrapartes gnu y
      Boost. 

      multiset<T> implanta un conjunto de claves de tipo T en el cual se
      permiten repitencias.

      @param T el tipo de dato que almacenará el conjunto.
      @param Compare el criterio de comparación entre los elementos de
      tipo T. Por omisión se implanta T::operator<(const T&).
      @param Tree la clase de árbol binario de búsqueda con rangos que
      instrumentará el conjunto. Por omisión si usa Treap_Rk

      @see Treap_Rk Rand_Tree set map multimap 
      @ingroup STL
   */
  template <class T, 
            class Compare = Aleph::less<T>,
            template <class, class> class Tree = Treap_Rk
            >
  class multiset
  {
    struct Node_Data
    {
      mutable T key;
      mutable size_t num_reps;

      Node_Data() : num_reps(0) { /* empty */ }

      Node_Data(const T & k) : key(k), num_reps(1)  { /* empty */ }
    };

    struct Cmp_Data
    {
      bool operator () (const Node_Data & op1, const Node_Data & op2) const
      {
        return Compare () (op1.key, op2.key);
      }
    };

  public:

    /// El tipo dato de los elementos del conjunto.  
    typedef T value_type;

    /// Tipo genérico de referencia a elemento del conjunto.
    typedef typename multiset::value_type & reference;

    /// Tipo genérico de referencia constante a elemento del conjunto.
    typedef const typename multiset::value_type & const_reference;

    /// Tipo numérico para los tamaños (natural).
    typedef size_t size_type;

    /// El tipo dato de los elementos del conjunto.
    typedef T key_type;

  private:

    typedef Tree<Node_Data, Cmp_Data> Tree_Type;

    typedef typename Tree_Type::Node Node;

    typedef typename Tree_Type::Iterator Tree_Iterator;

    mutable Tree_Type tree;

    mutable size_t num_elem;

    Node_Pool<Node> pool;

    static T & get_key(Node * p) { return KEY(p).key; }

    static size_t & get_num_reps(Node * p) { return KEY(p).num_reps; }

  public:  

    /** Iterador sobre un multi-conjunto (multiset).

        @ingroup STL
    */
    class iterator
    {
    private:

      friend class multiset<T, Compare, Tree>;
    
      typedef typename Tree_Type::Iterator Iterator;
    
      mutable multiset * multiset_ptr = nullptr;

      mutable Iterator tree_it;

      mutable int pos_in_node;

      bool overflow;
      bool underflow;

      iterator (multiset * mset_ptr, Node * curr_tree_node, int pos = 0)
        : multiset_ptr(mset_ptr), tree_it(mset_ptr->tree, curr_tree_node), 
          pos_in_node(pos), overflow (false), underflow (false)
      {
        // empty
      }

      void default_init ()
      {
        if (tree_it.has_curr ())
          {
            underflow = overflow = false;
            pos_in_node = 0;
          }
        else
          underflow = overflow = true;
      }

      iterator(const multiset & ms)
        : multiset_ptr(const_cast<multiset*>(&ms)), tree_it(ms.tree)
      {
        default_init();
      }

      Node * get_curr_node() { return tree_it.get_curr(); }

      bool has_curr() const 
      {
        return tree_it.has_curr();
      }

      Node_Data & get_data() { return KEY(get_curr_node()); }

      T & get_key() { return multiset::get_key(get_curr_node()); }

      size_t & get_num_reps() { return multiset::get_num_reps(get_curr_node()); }

    public:

      /// El tipo de dato que contiene el multi-conjunto.
      typedef typename multiset<T>::value_type      value_type;

      /// Tipo numérico usado para diferencias entre posiciones de
      /// iteradores.
      typedef typename multiset<T>::size_type       difference_type;

      /// Tipo apuntador a elemento dentro del multi-conjunto.
      typedef typename multiset<T>::value_type *    pointer;

      /// Tipo referencia a elemento dentro del multi-conjunto.
      typedef typename multiset<T>::reference       reference;

      /// Tipo referencia a elemento dentro del multi-conjunto.
      typedef const typename multiset<T>::reference const_reference;

      iterator(multiset * mset_ptr)
        : multiset_ptr(mset_ptr), tree_it(multiset_ptr->tree)
      {
        default_init();
      }

      iterator(const iterator & it)
        : multiset_ptr(it.multiset_ptr), tree_it(it.tree_it),
          pos_in_node(it.pos_in_node),
          overflow(it.overflow), underflow(it.underflow)
      {
        // empty
      }

      /// Constructor vacío; no tiene validez si no se asocia un conjunto.
      iterator ()
      {
        underflow = overflow = true;
        pos_in_node = -1;
      }

      /// Proporciona una referencia al elemento actual.
      const T & operator * ()
      {
        return get_key();
      }

      /// "Dereferencia" un puntero al elemento actual.
      const T * operator -> ()
      {
        return & get_key();
      }

    private:

      void goto_begin ()
      {
        tree_it.reset_first ();
        if (tree_it.has_curr ())
          {
            underflow = false;
            pos_in_node = 0;
          }
        else
          {
            underflow = true;
            pos_in_node = -1;
          }
      }

      void goto_last ()
      {
        tree_it.reset_last ();
        if (tree_it.has_curr ())
          {
            overflow = false;
            pos_in_node = get_num_reps() - 1;
          }
        else
          {
            overflow = true;
            pos_in_node = -1;
          }
      }

      void goto_end ()
      {
        tree_it.reset_last();
        if (tree_it.has_curr()) 
          {     // el árbol no está vacío 
            tree_it.next (); // coloca tree_it fuera de rango
            underflow = false;
          }
        else
          underflow = true;

        pos_in_node = -1;
        overflow = true;
      }

      iterator compute_end() const
      {
        iterator it(*this);
        it.goto_end();
        return it;
      }

      bool is_at_end() const
      {
        return not tree_it.has_curr();
      }

      iterator compute_begin() const
      {
        iterator it(*this);
        return it;
      }

      void forward ()
      {
        if (underflow)
          {
            goto_begin ();
            return;
          }

        if (++pos_in_node == get_num_reps())
          { 
            tree_it.next (); 

            if (tree_it.has_curr ())
              pos_in_node = 0;
            else
              {
                overflow = true;
                pos_in_node = -1;
                assert(*this == compute_end());
              }
          }
      }

      void backward ()
      {
        if (overflow)
          {
            goto_last ();
            return;
          }

        if (pos_in_node-- == 0)
          {
            tree_it.prev ();

            if (tree_it.has_curr ())
              pos_in_node = get_num_reps();
            else
              underflow = true;
          }
      }

      void del()
      {
        Node * tree_node = get_curr_node();
        size_t & num_reps = multiset::get_num_reps(tree_node);

        --num_reps;

        --multiset_ptr->num_elem;

        if (num_reps == 0)
          {
            tree_it.del();
            multiset_ptr->pool.deallocate(tree_node);

            if (tree_it.has_curr ())
              pos_in_node = 0;
            else
              {
                overflow = true;
                pos_in_node = -1;
              }
          }
      }

    public:

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
      T & operator -- (int)
      {
        iterator ret_val = *this;
        backward ();
        return ret_val;
      }
    
      /// Avanza el iterador n elementos hacia delante y carga el nuevo
      /// elemento actual.
      iterator operator += (size_type n)
      {
        for (int i = 0; i < n; ++i)
          forward ();

        return *this;
      } 

      /// Avanza el iterador n elementos hacia atrás y carga el nuevo
      /// elemento actual.
      iterator operator -= (size_type n)
      {
        for (int i = 0; i < n; ++i)
          backward ();
      
        return *this;
      } 

      /// Retorna true si los iteradores tienen el mismo estado.
      bool operator == (const iterator & it) const
      {
        if (this->has_curr() and it.has_curr())
          return pos_in_node == it.pos_in_node;

        if (this->is_at_end() and it.is_at_end())
          {
            assert(this->overflow and it.overflow);
            return true;
          }
      
        return false;
      }

      /// Retorna true si los iteradores tienen estados distintos.
      bool operator != (const iterator & itor) const
      {
        return not (*this == itor);
      }

      bool verify (const multiset & _multiset) const
      {
        return tree_it.verify ( (Tree_Type*)& (_multiset.tree));
      }

      bool verify (const iterator & it) const
      {
        return tree_it.verify (it.tree_it);
      }
    }; // class iterator;

    /// Tipo iterador constante.
    typedef typename multiset::iterator const_iterator;

    /// Tipo iterador invertido.
    typedef typename multiset::iterator reverse_iterator;
  
    /// Tipo iterador invertido constante.
    typedef typename multiset::iterator const_reverse_iterator;

    /// Constructor vacío de un multi-conjunto.
    multiset () : num_elem (0), pool(100) { /* empty */  }

  private:

    void copy (const multiset & c)
    {
      tree.getRoot () = copyRec(c.tree.getRoot());
    }

  public:
 
    /// Instancia una copia del multi-conjunto c.
    multiset (const multiset & c) : num_elem (c.num_elem), pool(100)
    {
      copy (c);
    }

    /** Instancia un multi-conjunto con los elementos comprendidos en el
        rango [beg..end).

        Este constructor instancia un multi-conjunto con elementos iniciales
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
    multiset (Itor beg, const Itor & end) : multiset()
    {
      while (beg != end)
        insert (*beg++);
    }

    /// Destructor; todos los elementos son eliminados y la memoria es
    /// liberada. 
    ~multiset () 
    { 
      destroyRec(tree.getRoot());
    }  

    /// Asigna todo el contenido de c a this.
    multiset & operator = (const multiset & c)
    {
      if (this == &c)
        return *this;

      destroyRec(tree.getRoot());

      copy(c);

      num_elem = c.num_elem;

      return *this;
    }

    /// Retorna la cantidad de elementos que contiene el multi-conjunto.
    size_type size () const { return num_elem; }

    /// Retorna true si el contenedor esta vacío.
    bool empty () const
    {
      return COUNT(tree.getRoot ()) == 0;
    }

    /// Retorna true si el multi-conjunto this tiene exactamente los mismos
    /// elementos del multi-conjunto c.
    bool operator == (const multiset & c) const
    {
      if (this == &c)
        return true;

      if (size () != c.size ())
        return false;

      Tree_Iterator itor1 (tree), itor2 (c.tree);

      while (itor1.has_curr() and itor2.has_curr())
        {
          Node * p1 = itor1.get_curr();
          Node * p2 = itor2.get_curr();

          if (no_equals <Node_Data, Cmp_Data> (KEY(p1), KEY(p2)))
            return false;
          else if (get_num_reps(p1) != get_num_reps(p2))
            return false;

          itor1.next ();
          itor2.next ();
        }
    
      return not itor1.has_curr() and not itor2.has_curr();
    }

    /// Retorna true si el multi-conjunto this no contiene exactamente
    /// los mismos elementos del multi-conjunto c.
    bool operator != (const multiset & c) const
    {
      return not (*this == c);
    }

    /// Retorna true si los elementos ordenados del multi-conjunto this son
    /// menores que los del multi-conjunto c.
    bool operator < (const multiset & c) const
    {
      if (this == &c)
        return false;

      iterator itor1 (*this), itor2 (c);

      while (itor1.has_curr() and itor2.has_curr())
        {
          if (Cmp_Data () (itor1.get_data(), itor2.get_data()))
            return true;
          else if (Cmp_Data () (itor2.get_data(), itor1.get_data()))
            return false; // no son iguales
	  
          itor1.forward();
          itor2.forward();
        }
    
      if (itor1.has_curr()) /* |*this| >= |c| */
        return false;

      return itor2.has_curr();
    }

    /// Retorna true si los elementos ordenados del multi-conjunto this son
    /// mayores que los del multi-conjunto c.
    bool operator > (const multiset & c) const
    {
      return not (*this == c or *this < c);
    }

    /// Retorna true si los elementos ordenados del multi-conjunto this son
    /// menores o iguales que los del multi-conjunto c.
    bool operator <= (const multiset & c) const
    {
      return not (*this > c );
    }

    /// Retorna true si los elementos ordenados del multi-conjunto this son
    /// mayores o iguales que los del multi-conjunto c.
    bool operator >= (const multiset & c) const
    {
      return not (*this < c);
    }

    /** Retorna la cantidad de instancias de value que hay dentro del
        multi-conjunto.

        @param[in] value valor de clave a buscar.
        @return cantidad de instancias del valor de clave value que
        contiene el multi-conjunto.
    */  
    size_type count (const T & value) const
    {
      Node * p = tree.search (Node_Data(value));

      if (p == nullptr)
        return 0;

      return get_num_reps(p);
    } 

    /** Retorna un iterador posicionado sobre la primera instancia del
        elemento value en el multi-conjunto.

        find(value) busca el elemento value en el multi-conjunto. Si
        éste es encontrado, entonces se retorna un iterador
        posicionado sobre el primer elemento del multi-conjunto con el
        valor en cuestión; de lo contrario, se retorna un iterador al
        elemento que antecedería a value si éste fuese
        insertado. Nótese que en este caso el elemento antecesor es el
        último entre los repetidos. 

        @param[in] value valor a buscar dentro del multi-conjunto.
        @return iterador posicionado sobre el primer elemento con
        valor value si éste se encuentra en el multi-conjunto 
        o un iterador al último elemento que antecedería a value si
        éste se insertase en el multi-conjunto. 
    */
    iterator find (const T & value) const
    { 
      Node * node = tree.search(Node_Data(value));

      if (node == nullptr)
        return end ();

      return iterator(const_cast<multiset*>(this), node);
    }

    /** Retorna un iterador posicionado sobre la primera instancia de
        value dentro de multi-conjunto.

        lower_bound(value) busca el elemento value en el multi-conjunto. Si
        éste es encontrado, entonces se retorna un iterador
        posicionado sobre el primer elemento del multi-conjunto con el
        valor en cuestión; de lo contrario, se retorna un iterador
        desbordado al final del conjunto.

        @param[in] value valor a buscar dentro del multi-conjunto.
        @return iterador posicionado sobre el primer elemento con
        valor value si éste se encuentra en el multi-conjunto 
        o un iterador desbordado si value no se encuentra
        en el multi-conjunto. 
    */
    iterator lower_bound (const T & value) const
    {
      if (size () == 0)
        ah_underflow_error_if(size() == 0) << "multiset is empty";

      Node * tree_node = tree.search(Node_Data(value));

      if (tree_node == nullptr)
        return end ();

      return iterator (this, tree_node);
    }

    /** Retorna un iterador posicionado sobre la última instancia de
        value dentro de multi-conjunto.

        upper_bound(value) busca el elemento value en el multi-conjunto. Si
        éste es encontrado, entonces se retorna un iterador
        posicionado sobre el último elemento del multi-conjunto con el
        valor en cuestión; de lo contrario, se retorna un iterador
        desbordado al final del conjunto.

        @param[in] value valor a buscar dentro del multi-conjunto.
        @return iterador posicionado sobre el último elemento con
        valor value si éste se encuentra en el multi-conjunto 
        o un iterador desbordado si value no se encuentra
        en el multi-conjunto. 
    */
    iterator upper_bound (const T & value) const
    {
      if (size () == 0)
        ah_underflow_error_if(size() == 0) << "multiset is empty";

      Node * tree_node = tree.search(Node_Data(value));

      if (tree_node == nullptr)
        return end ();

      iterator it (this, tree_node);
      it.list_it.reset_last ();

      return it;
    }

    /// Intercambia en tiempo constante todos los elementos de this
    /// con los de c.  
    void swap (multiset & c)
    {
      std::swap (tree.getRoot (), c.tree.getRoot ());
      std::swap (num_elem, c.num_elem);
    }

    /// Retorna un iterador posicionado al primer elemento del
    /// multi-conjunto. 
    iterator begin () const
    {
      return iterator (*this);
    }

    /// Retorna un iterador posicionado al último elemento del
    /// multi-conjunto. 
    iterator end () const
    {
      return iterator(*this).compute_end();
    }

    /** Inserta value en el multi-conjunto.

        insert(value) inserta en el multi-conjunto this una copia del
        elemento value y iterador posicionado en el elemento recién insertado.

        @param[in] value valor a insertar en el multi-conjunto.
        @return un iterador posicionado sobre el elemento recién insertado 
        @throw bad_alloc si no hay suficiente memoria.
    */
    iterator insert (const T & value) 
    {
      Node * p = pool.allocate(Node_Data(value));
      Node * ptr = tree.search_or_insert(p);

      if (ptr != p) // ya está value en el árbol
        pool.deallocate(p);

      ++num_elem;

      return iterator(this, ptr, get_num_reps(ptr)++);
    }
    
    /** Inserta value en el multi-conjunto a partir del iterador pos.

        insert(pos, value) inserta en el multi-conjunto this, a partir
        del elemento posicionado por el iterador pos.

        Según el estándar, pos debe ser considerado una ayuda a la
        inserción que ahorre eventuales búsquedas. Esto es correcto en
        el caso de Aleph si el valor del elemento actual de pos es
        igual a value; en este caso, la inserción toma \f$O(1)\f$ y
        value queda exactamente después del elemento actual de pos . 

        Si el valor del elemento actual de pos no es igual a value,
        entonces es inevitable una búsqueda -que toma \f$O(\lg n)\f$-
        de value en el multi-conjunto. Si value ya se encuentra,
        entonces value se inserta al final de los elementos repetidos.
        mismo que value, entonces la inserción se realiza en 

        @param[in] pos iterador a partir del cual se desea realizar la
        inserción.
        @param[in] value valor a insertar en el multi-conjunto.
        @return un iterador posicionado sobre el elemento recién
        insertado.
        @throw bad_alloc si no hay suficiente memoria.
        @throw overflow_error si el iter
    */  
    iterator insert (iterator pos, const T & value)
    {
      Aleph::verify_container_and_iterator (this, pos);

      if (pos != pos.compute_end())
        {
          Node * p = pos.get_curr_node();

          if (are_equals <Node_Data, Cmp_Data> (KEY(p), Node_Data(value)))
            {
              get_num_reps(p)++;
              pos.pos_in_node++;
	    
              return pos;
            }
        }

      return insert(value);
    }

    /** Inserta en el multi-conjunto los elementos de un contenedor
        comprendidos en el rango [beg..end).

        insert(beg,end) toma un rango de elementos de un contenedor
        (set, map, multiset, multimap, list o vector) asociados a los
        iteradores beg y end e inserta todos los elementos del rango
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

    /** Borra del multi-conjunto todas las instancias de value.

        erase(value) borra del multi-conjunto todas las instancias el
        elemento value. 

        @param[in] value valor del elemento a borrar.
        @return cantidad de instancias de value que fueron borradas.
    */
    size_type erase (const T & value)
    {
      Node * tree_node = tree.remove(Node_Data(value));

      if (tree_node == nullptr)
        return 0;

      const size_t ret_val = get_num_reps(tree_node);
	
      pool.deallocate(tree_node);

      num_elem -= ret_val;

      return ret_val;
    }

    /** Borra del multi-conjunto el valor actual del iterador pos.

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

      Node * tree_node = pos.get_curr_node();

      size_t & num_reps = get_num_reps(tree_node);

      --num_reps;
      --num_elem;

      if (num_reps == 0)
        {
          tree.remove(Node_Data(KEY(tree_node)));
          pool.deallocate(tree_node);
        }
    }
  
  private:

    /* Borra el rango secuencialmente */
    iterator delete_range (iterator beg, const iterator & end)
    {
      while (beg != end)
        beg.del();

      return beg;
    }

  public:

    /** Borra del multi-conjunto el rango comprendido por [beg..end).

        erase(beg,end) borra eficientemente todos los elementos del
        multi-conjunto delimitados por los iteradores beg y end.

        @param[in] beg iterador posicionado sobre el elemento inicio del
        rango.
        @param[in] end iterador posicionado sobre el último más un
        elemento del rango.
        @throw domain_error si los iteradores beg y end no están
        asociados a this.
        @note El elemento posicionado por el iterador end no se incluye.
    */
    iterator erase (iterator beg, const iterator & end)
    {
      Aleph::verify_container_and_iterator (*this, beg);
      Aleph::verify_iterators (beg, end);

      return delete_range(beg, end);
    }

    /// Borra todos los elementos del multi-conjunto.
    void clear ()
    {
      destroyRec (static_cast<Node *> (tree.getRoot ()));
      num_elem = 0;
    }
  };


  template <typename T>
  typename iterator_traits<typename multiset<T>::iterator>::difference_type 
  distance (typename multiset<T>::iterator it1, 
            typename multiset<T>::iterator it2)
  {
    typename iterator_traits<typename multiset<T>::iterator>::difference_type
      counter = 0;

  
    while (it1 != it2)
      {
        ++counter;
        ++it1;
      }

    return counter;
  }

} // end namespace Aleph

# endif // AH_MULTISET_H
