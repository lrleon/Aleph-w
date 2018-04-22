
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

/*
       Aleph implementation of multimap C++ standard container
*/

# ifndef ALEPH_MULTIMAP_H
# define ALEPH_MULTIMAP_H

# include <ah_stdc++_utils.H>
# include <tpl_treapRk.H>

namespace Aleph
{

  /** Implementación Aleph del tipo estándar multimap<Key,T> basada en
      árboles binarios de búsqueda con rangos.

      Esta es una implantación parcial, aunque bastante avanzada y de
      posible mejor rendimiento, de la clase estándar C++
      multimap<Key,T>. 

      La implementación está basada en árboles binarios de búsqueda con
      rango. Por omisión, se emplean treaps con tango, pero podrían
      emplearse otros tipos de árboles binarios de búsqueda siempre y
      cuando respeten la interfaz. 

      Una diferencia de esta implementación con el estándar es que el
      tipo de árbol es un parámetro. En Aleph los árboles binarios de
      búsqueda con rango (BinNodeXt), treaps (Treap_Rk) y los árboles
      aleatorizados (Rand_tree) pueden emplearse para implementar esta
      clase. 

      Otra diferencia con el estándar, quizá la más importante, es que
      los pares del mapeo que repitan la clave primaria están ordenados
      entre sí. Esto exige la presencia del operador < para el tipo
      T. Del mismo modo, viola la normativa del estándar de que los
      pares de clave primaria repetida estén ordenados según su orden de
      inserción.

      La implementación emplea un árbol binario de búsqueda con rangos
      para indizar las claves primarias de tipo Key. Cada nodo de este
      árbol contiene otro árbol binario de búsqueda con rangos que
      contiene las claves asociadas. Cada nodo, en ambos tipos de
      árboles, maneja un contador de repeticiones. Así, por ejemplo,
      para los pares (1,0),(1,0),(1,0),(1,0),(1,5),(1,5),(1,5) se usa un
      solo nodo KTree con clave 1 y dos nodos TTree con claves 0 y 1
      respectivamente. Este estilo de implantación ahorra mucho espacio
      si en efecto, tendría sentido esperar, el multimapeo contiene
      muchas duplicidades. 

      Este estilo de implantación tiende a ser más veloz que otros
      esquemas tradicionales. Especialmente, el avance de los
      iteradores. 

      Nótese también que en el caso de no contener muchas duplicidades
      esta implementación consume prácticamente el mismo espacio que una
      basada en árboles más listas enlazadas de repitencias (que es el
      usado por la mayoría de las instrumentaciones del estándar).

      @param Key el tipo de clave primaria
      @param T el tipo de dato que asociará a una clave primaria.
      @param Compare el criterio de comparación entre los elementos de
      tipo T. Por omisión se implanta Key::operator<(const T&).
      @param KTree la clase de árbol binario de búsqueda con rangos
      conque se indizan las claves primarias de tipo Key.
      @param TTree la clase de árbol binario de búsqueda con rangos
      conque se indizan las claves asociadas.

      @ingroup stl
      @see BinNodeXt Treap_Rk Rand_Tree set map multiset 
      @author Leandro Rabindranath León
   */
template <typename Key, 
	  typename T, 
	  class Compare = Aleph::less<Key>,
	  template <class, class> class KTree = Treap_Rk,
	  template <class, class> class TTree = Treap_Rk>
class multimap
{
      // Definición de registro conteniendo elem de tipo T
  struct Tdata
  {
    T elem;          // elemento mapeado a una clave de tipo key
    size_t num_reps = 0; // número de ocurrencias de elem relacionado
			 // a un valor de clave key

    Tdata() : elem(), num_reps(0) { /* empty */ }

    Tdata(const T & e) : elem(e), num_reps(0) { /* empty */ }

    Tdata(const Tdata & item) : elem(item.elem), num_reps(item.num_reps) 
    {
      // empty
    }
  };

  typedef Aleph::less<T> Tless;

  struct Cmpt // Clase de comparación para clave asociada
  {
    bool operator () (const Tdata & op1, const Tdata & op2) const
    {
      return Tless () (op1.elem, op2.elem);
    }
  };

      // definición de árbol de claves elementos mapeados
  typedef TTree<Tdata, Cmpt> T_Tree;

  typedef typename T_Tree::Node Tnode;

      // Definición de registro conteniendo elementos del multimapeo
  struct Kdata
  {
    Key    key;      // clave primaria
    size_t num_reps = 0; // Número de repeticiones del valor de key
    T_Tree t_tree;   // árbol de elementos de tipo T asociados a key
    
    Kdata() : num_reps(0) { /* empty */ }

    Kdata(const Kdata & item) : key(item.key), num_reps(0)
    {
      t_tree.getRoot() = copyRec(const_cast<Kdata&>(item).t_tree.getRoot());
      num_reps = item.num_reps;
    }

    ~Kdata()
    {
      destroyRec(t_tree.getRoot());
    }

    Kdata & operator = (const Kdata & item) 
    {
      if (this == &item)
      	return *this;

      destroyRec(t_tree.getRoot());
      num_reps = 0;
      t_tree.getRoot() = copyRec(const_cast<Kdata&>(item).t_tree.getRoot());

      key      = item.key;
      num_reps = item.num_reps;

      return *this;
    }
  };

  struct Cmpk // clase de comparación para clave
  {
    bool operator () (const Kdata & op1, const Kdata & op2) const
    {
      return Compare () (op1.key, op2.key);
    }
  };

  typedef KTree<Kdata, Cmpk> K_Tree;

  typedef typename K_Tree::Node Knode;

  K_Tree           k_tree;

  size_t           num_elem = 0;

  // Para ahorrar memoria, el árbol ktree indiza por registros de tipo
  // Kdata y la función de comparación siguiente se encarga de sólo
  // comparar la clave. Ahora bien, es muy importante, para no tener una
  // penalidad en desempeño, que no se desperdicie tiempo en el
  // constructor de Kdata cada vez que se requiera realizar una
  // búsqueda. Así el siguiente campo se crea una sola vez y se emplea
  // para las búsquedas. 
  mutable Kdata searching_kdata; 

      // esta función se emplea cuando se desee buscar key y se requiera
      // obtener un Kdata
  Kdata & key_kdata(const Key & key) const
  {
    searching_kdata.key = key;
    return searching_kdata;
  }
  
public:

  multimap() { /* empty */ }

  ~multimap()
  {
    destroyRec(k_tree.getRoot());
  }

      /// Vacía el multimapeo. Todos los elementos son borrados.
  void clear()
  {
    destroyRec(k_tree.getRoot());
    num_elem = 0;
  }

  typedef std::pair<Key, T> Pair;

      /// Tipo de valor de clave manejado (que es un par)
  typedef Pair value_type;

      /// Referencia al par
  typedef typename multimap::value_type & reference;

  typedef const typename multimap::value_type & const_reference;

  typedef size_t size_type;

  typedef Key key_type;

  typedef T mapped_type;

      /// Retorna la cantidad de elementos que contiene el multimapeo
      /// (cantidad total de pares).
  const size_t & size () const { return num_elem; }

      /// Retorna un estimado sobre la máxima cantidad de elementos que
      /// puede albergar el multimapeo
  size_type max_size() const 
  { 
    int sizek = sizeof(Knode);
    int sizet = sizeof(Tnode);

    double total_mem = pow(2, __WORDSIZE);

    size_type ret_val = floor(total_mem / (sizek + sizet));

    return ret_val;
  }

      /// Retorna true si el multimapeo está vacío (no contiene ningún
      /// elemento).
  bool empty () const {return k_tree.is_empty(); }

private:

  typedef typename K_Tree::Iterator K_Itor;
  typedef typename T_Tree::Iterator T_Itor;

public:

  /** Iterador sobre un multimapeo

      @ingroup stl
   */
  class iterator
  {
    friend class multimap<Key, T, Compare, KTree, TTree>;

    multimap *       multimap_ptr = nullptr;
    const K_Tree *   k_tree_ptr = nullptr;
    mutable K_Itor   k_it;
    const T_Tree *   t_tree_ptr = nullptr;
    mutable T_Itor   t_it;
    mutable int      pos_in_k = 0;
    mutable int      pos_in_t = 0;
    bool             underflow;
    bool             overflow;
    Pair             ret_pair; // Usado por el operador ->

    void default_init()
    {
      assert(k_tree_ptr != nullptr);

      if (k_it.has_curr())
	{
	  assert(KEY(k_it.get_curr()).t_tree.size() > 0);
	  underflow = overflow = false;
	  pos_in_k = pos_in_t = 0;
	  t_tree_ptr = &KEY(k_it.get_curr()).t_tree;
	  t_it = T_Itor(*t_tree_ptr);
	}
      else
	put_in_overflow();
    }

    iterator(const multimap * m, Knode * kp, Tnode * tp,
	     int kpos = 0, int tpos = 0)
      : multimap_ptr(const_cast<multimap*>(m)),
	k_tree_ptr(&m->k_tree),
	k_it(*k_tree_ptr, kp),
	t_tree_ptr(&KEY(kp).t_tree),
	t_it(*t_tree_ptr, tp), 
	pos_in_k(kpos),
	pos_in_t(tpos),
	underflow(false),
	overflow(false)
    {
      assert(not KEY(kp).t_tree.is_empty());
      assert(KEY(tp).num_reps > 0 and tpos < KEY(tp).num_reps);
    }

    iterator(const multimap * m, Knode * p)
      : multimap_ptr(const_cast<multimap*>(m)),
	k_tree_ptr(&m->k_tree), k_it(*k_tree_ptr, p),
	t_tree_ptr(&KEY(p).t_tree), t_it(*t_tree_ptr),
	pos_in_t(0), underflow(false), overflow(false)
    {
      assert(KEY(p).num_reps > 0);
      assert(not KEY(p).t_tree.is_empty());
    }

  public:

        /// El tipo de dato que contiene el multi-mapeo
    typedef typename multimap::value_type   value_type;

        /// Tipo numérico usado para diferencias entre posiciones de
        /// iteradores. 
    typedef typename multimap::size_type    difference_type;

        /// Tipo apuntador a elemento dentro del multi-mapeo
    typedef typename multimap::value_type * pointer;

        /// Tipo referencia a elemento dentro del multi-mapeo
    typedef typename multimap::reference    reference;

        /// Constructor a partir de un multimapeo (no es parte del
        /// estándar, pero es muy útil).
    iterator(const multimap & mm) 
      : multimap_ptr(const_cast<multimap*>(&mm)), 
	k_tree_ptr(&multimap_ptr->k_tree), k_it(*k_tree_ptr)
    {
      default_init();
    }

        /// Constructor copia (masivamente usado por el estándar)
    iterator(const iterator & it) 
      : multimap_ptr(it.multimap_ptr), k_tree_ptr(it.k_tree_ptr), k_it(it.k_it),
	t_tree_ptr(it.t_tree_ptr), t_it(it.t_it), 
	pos_in_k(it.pos_in_k), pos_in_t(it.pos_in_t), 
	underflow(it.underflow), overflow(it.overflow)
    {
      // empty
    }

        /// Constructor vacío; uso indefinido.
    iterator()
      : multimap_ptr(nullptr), k_tree_ptr(nullptr), t_tree_ptr(nullptr),
	pos_in_k(-1), pos_in_t(-1), underflow(true), overflow(true)
    {
      // empty
    }

        /// Asignación
    iterator & operator = (const iterator & it) 
    {
      if (this == &it)
	return *this;

      multimap_ptr = it.multimap_ptr;
      k_tree_ptr   = it.k_tree_ptr;
      k_it         = it.k_it;
      t_tree_ptr   = it.t_tree_ptr;
      t_it         = it.t_it; 
      pos_in_k     = it.pos_in_k;
      pos_in_t     = it.pos_in_t;
      underflow    = it.underflow;
      overflow     = it.overflow;

      return *this;
    }

  private:

    bool has_curr() const { return k_it.has_curr(); }

    Knode * get_curr() { return k_it.get_curr(); }

    Kdata & get_curr_kdata() { return KEY(get_curr()); }

  public:

        /// Retorna una copia del par actual del iterador.
    value_type  operator * () 
    {
      return value_type(get_curr_kdata().key, KEY(t_it.get_curr()).elem);
    }

    /** Retorna un puntero para posterior dereferencia (acceso a uno
	de los campos del par -first o second). 

	@note La dirección retornada es la de un par interno cuya
	existencia está supeditada a la del iterador. <b>No lo use para
	obtener puntero a pares</b>, pues, aparte de que no es la
	finalidad de este operador, su dirección es inválida una vez se
	destruye el iterador. <b>Úselo estrictamente para acceder a los
	campos del par (first o second) del elemento actual donde está
	posicionado el iterador</b>.
    */
    const value_type * operator -> () 
    {
      ret_pair.first  = get_curr_kdata().key;
      ret_pair.second = KEY(t_it.get_curr()).elem;

      return &ret_pair;
    }

  private:

    void goto_begin()
    {
      k_it.reset_first();
      if (not has_curr())
	{
	  put_in_underflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      pos_in_k = pos_in_t = 0;
    }

    void goto_last()
    {
      k_it.reset_last();
      if (not has_curr())
	{
	  put_in_overflow();
	  return;
	}

      overflow = false;
      Kdata & kdata = KEY(get_curr());
      t_tree_ptr = &kdata.t_tree;
      t_it = T_Itor(*t_tree_ptr);
      t_it.reset_last();
      pos_in_k = kdata.num_reps - 1;
      pos_in_t = KEY(t_it.get_curr()).num_reps - 1;
    }

    void goto_end()
    {
      k_it.reset_last();
      if (has_curr())
	{
	  k_it.next(); // lo coloca fuera de rango
	  underflow = false;
	}
      else
	put_in_underflow();

      put_in_overflow();
    }

    iterator compute_end() const
    {
      iterator it(*this);
      it.goto_end();
      assert(it.overflow);
      return it;
    }

    bool is_at_end() const { return not has_curr(); }

    iterator compute_begin() const
    {
      iterator it(*this);
      return it;
    }

    void put_in_overflow()
    {
      t_tree_ptr = nullptr;

      if (k_tree_ptr->is_empty())
	put_in_underflow();

      overflow = true;
    }
    
    void put_in_underflow()
    {
      t_tree_ptr = nullptr;
      
      pos_in_t = -1;
      underflow = true;
    }

         // avanza todas las claves de la posición actual
    void forward_k_it()
    {
      k_it.next();
      if (not has_curr())
	{
	  put_in_overflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      pos_in_t = 0;
    }

    void forward_tree_iterators()
    {
      t_it.next();
      if (t_it.has_curr())
	{
	  pos_in_t = 0;
	  return;
	}
	  
      forward_k_it();
    }

    void forward()
    {
      if (underflow)
	{
	  goto_begin();
	  return;
	}

      if (overflow)
	{
	  assert(t_tree_ptr == nullptr);
	  throw std::overflow_error("Multimap::iterator is already "
				    "in overflow");
	}

      assert(t_it.has_curr() and t_tree_ptr != nullptr);

      Tdata & tdata = KEY(t_it.get_curr());
      if (++pos_in_t < tdata.num_reps) // alcanza el último elemento 
	return; // aún no!

      forward_tree_iterators();
    }

    void backward_k_it()
    {
      k_it.prev();
      if (not has_curr())
	{
	  put_in_underflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      t_it.reset_last();
      pos_in_t = KEY(t_it.get_curr()).num_reps - 1;
    }

    void backward_tree_iterators()
    {
      t_it.prev();
      if (t_it.has_curr())
	{
	  pos_in_t = KEY(t_it.get_curr()).num_reps - 1;
	  return;
	}
	  
      backward_k_it();
    }

    void backward()
    {
      if (overflow)
	{
	  goto_last();
	  return;
	}

      if (underflow)
	{
	  assert(t_tree_ptr == nullptr);
	  throw std::underflow_error("Multimap::iterator is "
				     "already in underflow");
	}

      assert(t_it.has_curr() and t_tree_ptr != nullptr);

      if (--pos_in_t >= 0) // alcanza el primer elemento 
	return;

      backward_tree_iterators();
    }

    void del()
    {
      Kdata & kdata = get_curr_kdata();
      Tnode * tp = t_it.get_curr();
      Tdata & tdata = KEY(tp);

      --multimap_ptr->num_elem;
      --kdata.num_reps;
      if (--tdata.num_reps == 0)
	{
	  delete t_it.del();
	  pos_in_t = 0;
	}
      else if (pos_in_t == tdata.num_reps)
	{
	  t_it.next();
	  pos_in_t = 0;
	}

      if (t_it.has_curr())
	{
	  assert(kdata.num_reps > 0);
	  return;
	}

      if (kdata.num_reps == 0)
	{
	  Knode * kp = k_it.del();
	  assert(KEY(kp).t_tree.is_empty());
	  delete kp;
	}
      else
	k_it.next();

      if (not k_it.has_curr())
	{
	  put_in_overflow();
	  return;
	}

      t_tree_ptr = &KEY(get_curr()).t_tree;
      t_it = T_Itor(*t_tree_ptr);
      pos_in_k = pos_in_t = 0;
    }

  public:

        /// Retorna true si los iteradores tienen el mismo estado.
    bool operator == (const iterator & it) const
    {
      if (this->has_curr() and it.has_curr())
	return (t_it.get_curr() == it.t_it.get_curr() and 
		pos_in_t == it.pos_in_t);

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
 
        /// Avanza el iterador una posición hacia adelante, luego carga
        /// elemento actual.    
    iterator operator ++ ()
    {
      forward();
      return *this;
    }

        /// Avanza el iterador una posición hacia adelante, luego carga
        /// elemento actual.    
    iterator operator ++ (int)
    {
      iterator ret_val = *this;
      forward();
      return ret_val;
    }

        /// Avanza el iterador una posición hacia atrás, luego carga
        /// elemento actual.    
    iterator operator -- ()
    {
      backward();
      return *this;
    }

        /// Avanza el iterador una posición hacia atrás, luego carga
        /// elemento actual.    
    iterator operator -- (int)
    {
      iterator ret_val = *this;
      backward();
      return ret_val;
    }

        /// Avanza el iterador n elementos hacia delante y carga el nuevo
        /// elemento actual. <
    iterator operator += (size_type n)
    {
      if (n == 0)
	return *this;

      while (true) // avanzar por nodos en t_it
	{
	  assert(KEY(t_it.get_curr()).num_reps > 0);

	  const size_t treps = KEY(t_it.get_curr()).num_reps;
	  const int remain_in_t_node = treps - pos_in_t;
	  if (n < remain_in_t_node)
	    {
	      pos_in_k += n;
	      pos_in_t += n;
	      assert(pos_in_t < KEY(t_it.get_curr()).num_reps);

	      return *this;
	    }

	  n -= remain_in_t_node;
	  pos_in_k += treps;
	  t_it.next();
	  pos_in_t = 0;
	  if (t_it.has_curr())
	    continue;

	  assert(pos_in_k == KEY(k_it.get_curr()).num_reps);

	  while (true) // avanzar por nodos en k_it
	    {
	      k_it.next();
	      if (not has_curr())
		{
		  put_in_overflow();
		  throw std::overflow_error("Multimap::iterator is "
					    "already in overflow");
		}
	  	  
	      assert(KEY(get_curr()).num_reps > 0);

	      const int remain_in_k_node = KEY(get_curr()).num_reps;
	      if (n < remain_in_k_node)
		{
		  t_tree_ptr = &KEY(get_curr()).t_tree;
		  t_it = T_Itor(*t_tree_ptr);
		  pos_in_k = pos_in_t = 0;
		  break;
		}

	      n -= remain_in_k_node;
	    }
	}
    } 
  };

      /// iterador constante
  typedef const iterator const_iterator;

  /** Inserta en el multimapeo los elementos contenidos en el rango
      [first,last).  

      Esta inserción toma \f$\lg n\f$; donde n es la cantidad de claves
      distintas de tipo Key que contiene el multimapeo.
  */
  iterator insert(const value_type & value)
  {
    Knode * kp = new Knode(key_kdata(value.first)); // nuevo nodo
    Knode * kq = k_tree.search_or_insert(kp); 

    if (kp != kq) // ¿Está ya la clave primaria en el multiconjunto?
      delete kp; // sí está ==> mantenga el nodo para uso futuro pero borre kp

    assert(KEY(kq).key == value.first); // TMP

    T_Tree & t_tree = KEY(kq).t_tree;
    const Tdata tdata(value.second);
    
    try
      {
	Tnode * tp = new Tnode(tdata);
	Tnode * tq = t_tree.search_or_insert(tp);

	if (tp != tq)  // ¿Está el elemento ya asociado a la clave?
	  delete tp;

	++num_elem;
	++KEY(kq).num_reps;

	return iterator(this, kq, tq, KEY(tq).num_reps++);
      }
    catch (...)
      {
	if (kp == kq)
	  delete kp;
	
	throw;
      }
  }

      /// Inserta en el multimapeo los elementos contenidos en el rango
      /// [first,last).  
      template <class InputIterator>
  void insert(InputIterator first, const InputIterator & last)
  {
    while (first != last)
      insert(*first++);
  }

      /// Construye un multimapeo con los pares del rango [first,last). 
       template <class InputIterator>
 multimap(InputIterator first, const InputIterator & last) 
   : multimap()
  {
    this->insert(first, last);
  }

      /// Constructor copia.
  multimap(const multimap & m) 
    : multimap::multimap()
  {
    k_tree.getRoot() = copyRec(const_cast<multimap&>(m).k_tree.getRoot());
    num_elem = m.num_elem;
  }

      /// Asignación. Todos los elementos del lado izquierdo son
      /// liberados. 
  multimap & operator = (const multimap & m)
  {
    if (this == &m)
      return *this;

    destroyRec(k_tree.getRoot());
    num_elem = 0;

    k_tree.getRoot() = copyRec(const_cast<multimap&>(m).k_tree.getRoot());
    num_elem = m.num_elem;

    return *this;
  }

  /** Inserción "ayudada" de posición.

      Esta versión de insert() aprovecha la posición del iterador hint
      para efectuar una inserción más rápida que la tradicional.

      Si hint->first == value.first según el criterio de comparación,
      entonces esta inserción consume \f$\lg m\f$; donde m es la
      cantidad de repeticiones distintas de la clave secundaria asociada
      a hint->first o value.first. De lo contrario se efectúa la
      inserción tradicional.

      @param[in] hint iterador de "ayuda"
      @param[in] value par a insertar
      @return un iterador apuntando a una copia del par recién
      insertado. 
      @throw bad_alloc si no hay suficiente memoria.
   */
  iterator insert(const_iterator & hint, const value_type & value)
  {
    if (hint.has_curr()) // mirar lo que contiene el iterador
      {
	Knode * kp = const_cast<iterator&>(hint).get_curr();
	Kdata & kdata = KEY(kp);

	    // clave primaria de hint  coincide con value.first
	if (Aleph::are_equals <Key, Compare> (kdata.key, value.first))
	  {     // sí ==> no hay necesidad de buscar en árbol k_tree

	    Tnode * tq = hint.t_it.get_curr(); // mirar clave asociada
	    const Tdata & tdata = KEY(tq);
    	        // ¿clave asociada distinta?
	    if (Aleph::no_equals <T, Tless> (tdata.elem, value.second)) 
	      {     // sí == > buscar o insertar en subárbol de clave asociada
		Tnode * tp = new Tnode(tdata); 
		tq = kdata.t_tree.search_or_insert(tp);
		if (tp != tq)  // ¿Está el elemento ya asociado a la clave?
		  delete tp; // sí ==> manténgalo para uso futuro
	      }
	    
	    ++num_elem;
	    ++kdata.num_reps;

	    return iterator(this, kp, tq, KEY(tq).num_reps++);
	  }
      }
	
        // hint no tiene nada que ver con value ==> inserción normal
    return insert(value);
  }

      /// Elimina del multimapeo el elemento situado en position.
  iterator erase(const_iterator & position)
  {
    iterator ret_val = position;
    
    ret_val.del();

    return ret_val;
  }

      /// Elimina todos los pares con clave key. Retorna la cantidad de
      /// pares eliminados.
  size_type erase(const key_type & key)
  {
    Knode * p = k_tree.remove(key_kdata(key));
    if (p == nullptr)
      return 0;

    size_type ret_val = KEY(p).num_reps;

    num_elem -= ret_val;

    delete p; // no hacemos kpool.deallocate(p) porque queremos liberar
	      // enteramente el árbol de claves asociadas KEY(p).t_tree

    return ret_val;
  }

      /** Elimina todos los pares comprendidos en el rango [first,last).

	  @param[in] first position del primer elemento a eliminar.
	  @param[in] last posición al elemento sucesor del último
	  elemento a eliminar.
	  @return iterador al el elemento sucesor del último eliminado
	  (debe corresponderse con last).

	  @todo Aprovechar salto interclaves dados por los contadores de
	  repitencias. 
       */
  iterator erase(iterator first, const_iterator & last)
  {
    iterator it = first; 
    while (it != last)
      it = erase(it);

    return it;
  }

      /// retorna un iterador al primer elemento del multimapeo.
  iterator begin() const { return iterator(*this); }
  
      /// retorna un iterador al último elemento del multimapeo.
  iterator end () const { return iterator(*this).compute_end(); }

      /// retorna la cantidad pares con clave igual a key
  size_type count(const Key & key) const
  {
    Kdata & kdata = const_cast<multimap*>(this)->key_kdata(key);
    Knode * p = const_cast<multimap*>(this)->k_tree.search(kdata);
    if (p == nullptr)
      return 0;

    return KEY(p).num_reps;
  }

      /// Retorna un iterador posicionado sobre el primer par con clave
      /// key; end() de lo contrario.
  iterator find(const Key & key) const
  {
    Knode * p = k_tree.search(key_kdata(key));
    if (p == nullptr)
      return end();

    return iterator(this, p);
  }

  /// Retorna un iterador posicionado en el lugar donde sería insertado key.
  iterator lower_bound(const Key & key) const
  {
    if (k_tree.is_empty())
      return end();

    std::pair<int, Knode*> p = k_tree.find_position(key_kdata(key));
    Knode * kp = p.second;

    iterator ret = iterator(this, kp);
    
    if (Aleph::are_equals <Key, Compare> (KEY(kp).key, key))
      return ret;

    if (p.first == k_tree.size()) // ¿clave mayor que todas las contenidas?
      return end();

    if (p.first == -1)
      return begin();

    if (Compare () (ret->first, key)) // key > *ret
      ret.forward_k_it();

    return ret;
  }

  /// Retorna un iterador posicionado en el lugar sucesor de key si key
  /// fuese insertado.
  iterator upper_bound(const Key & key) const
  {
    if (k_tree.is_empty())
      return end();

    std::pair<int, Knode*> p = k_tree.find_position(key_kdata(key));
    Knode * kp = p.second;

    iterator ret = iterator(this, kp);
    
    if (Aleph::are_equals <Key, Compare> (KEY(kp).key, key))
	ret.forward_k_it();

    if (p.first == k_tree.size()) // ¿clave mayor que todas las contenidas?
      return end();

    if (p.first == -1)
      return begin();

    if (Compare () (ret->first, key)) // key > *ret
      ret.forward_k_it();

    return ret;
  }

  /// Retorna un par de iteradores que definen el rango completo de
  /// claves con valor key que están contenidas en el multimapeo.
  std::pair<iterator,iterator> equal_range(const Key & key) const
  {
    Knode * p = k_tree.search(key_kdata(key));
    if (p == nullptr)
      {
	iterator e = end();
	return std::pair<iterator,iterator>(e, e);
      }

    iterator first(this, p);
    iterator last(this, p);
    last += KEY(p).num_reps;
    
    return std::pair<iterator,iterator>(first, last);
  }

  /// Intercambia los valores de this con los de other.
  void swap(multimap & other)
  {
    k_tree.swap(other.k_tree);
    std::swap(num_elem, other.num_elem);
  }  

  /** Retorna true si this es menor que rhs según la comparación
      lexicográfica. 

      @see lexicographical_compare
  */
  bool operator < (const multimap & rhs) const
  {
    K_Itor kit1(const_cast<multimap*>(this)->k_tree);
    K_Itor kit2(const_cast<multimap&>(rhs).k_tree);
    for (; kit1.has_curr() and kit2.has_curr(); kit1.next(), kit2.next())
      {
	Kdata & kdata1 = KEY(kit1.get_curr());
	Kdata & kdata2 = KEY(kit2.get_curr());

	const size_t & n1 = kdata1.num_reps;
	const size_t & n2 = kdata2.num_reps;

	if (n1 != n2)
	  return n1 > n2;

	const Key & key1 = kdata1.key;
	const Key & key2 = kdata2.key;
	if (Compare () (key1, key2))
	  return true;
	else if (Compare () (key2, key1))
	  return false;
      }
    
    if (kit1.has_curr() or kit2.has_curr())
      return kit2.has_curr();
    
    return false;
  }

  /** Retorna true si this es menor o igual que rhs según la comparación
      lexicográfica. 

      @see lexicographical_compare
  */
  bool operator <= (const multimap & rhs) const
  {
    K_Itor kit1(const_cast<multimap*>(this)->k_tree);
    K_Itor kit2(const_cast<multimap&>(rhs).k_tree);
    for (; kit1.has_curr() and kit2.has_curr(); kit1.next(), kit2.next())
      {
	Kdata & kdata1 = KEY(kit1.get_curr());
	Kdata & kdata2 = KEY(kit2.get_curr());

	const size_t & n1 = kdata1.num_reps;
	const size_t & n2 = kdata2.num_reps;

	if (n1 != n2)
	  return n1 > n2;

	const Key & key1 = kdata1.key;
	const Key & key2 = kdata2.key;
	if (Compare () (key1, key2))
	  return true;
	else if (Compare () (key2, key1))
	  return false;
      }
    
    if (kit1.has_curr() or kit2.has_curr())
      return kit2.has_curr(); 
    
    return true;
  }
  
      /// Retorna true si this es exactamente igual a rhs.
  bool operator == (const multimap & rhs) const
  {
    if (size() != rhs.size())
      return false;

    K_Itor kit1(const_cast<multimap*>(this)->k_tree);
    K_Itor kit2(const_cast<multimap&>(rhs).k_tree);
    for (; kit1.has_curr() and kit2.has_curr(); kit1.next(), kit2.next())
      {
	Kdata & kdata1 = KEY(kit1.get_curr());
	Kdata & kdata2 = KEY(kit2.get_curr());

	if (kdata1.num_reps != kdata2.num_reps)
	  return false;

	const Key & key1     = kdata1.key;
	const Key & key2     = kdata2.key;
	if (Compare () (key1, key2))
	  return false;
	else if (Compare () (key2, key1))
	  return false;
      }
    
    if (kit1.has_curr() or kit2.has_curr())
      return false;

    return true;
  }

      /// Retorna true si this es distinto de rhs.
  bool operator != (const multimap & rhs) const
  {
    return not (*this == rhs);
  }

  /** Retorna true si this es menor que rhs según la comparación
      lexicográfica. 

      @see lexicographical_compare
  */
  bool operator > (const multimap & rhs) const
  {
    return rhs < *this;
  }

  /** Retorna true si this es mayor o igual que rhs según la comparación
      lexicográfica. 

      @see lexicographical_compare
  */
  bool operator >=  (const multimap & rhs) const
  {
    return rhs <= *this;
  }

      /** Acceso como arreglo. 

	  El operador [] puede emplearse para escribir y leer datos en el
	  multimapeo. 

	  Si m es un mapeo, entonces m[key] = item; insertará el par
	  (key,item). 

	  La expresión cout << m[key] "lee" el item asociado a la clave
	  key. Si la clave ya existe, entonces se retorna el primer item
	  asociado. De lo contrario, se inserta un nuevo par cuyo valor de
	  item está inicializado por el constructor T::T().
      */
  const T & operator [] (const Key & key) 
  {
    iterator ret = insert(make_pair(key, T()));

    return ret->second;
  }

      /// Acceso como arreglo constante. Si la clave no está insertada
      /// entonces se dispara excepción.
  const T & operator [] (const Key & key) const 
    Exception_Prototypes (std::domain_error)
  {
    iterator ret = find(key);
    if (ret == end())
      throw std::domain_error("key not found on constant multimap");

    return ret->second;
  }
};

} // end namespace Aleph


# endif // ALEPH_MULTIMAP_H
