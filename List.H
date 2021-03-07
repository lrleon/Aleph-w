
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
	Aleph implementation of list C++ standard container
*/

# ifndef ALEPH_LIST_H
# define ALEPH_LIST_H

# include <ah_stdc++_utils.H>
# include <tpl_sort_utils.H>

namespace Aleph
{

  /** Implementación Aleph del contenedor estándar C++ List<T>.

      La implantación está fundamentada sobre la clase Aleph Dlist<T>.

      @see Dlist<T>
      @ingroup stl
   */
    template <class T>
class list
{
  mutable Dnode<T> dlist;

  typedef Dnode<T> Node;

public:

      /// El tipo de dato que maneja la lista.
  typedef T value_type;

      /// Tipo referencia a elemento dentro de la lista.
  typedef typename list::value_type & reference;

      /// Tipo referencia constante a elemento dentro de la lista.
  typedef const typename list::value_type & const_reference;

      /// Tipo numérico para el tamaño de la lista.
  typedef size_t size_type;

private:

  mutable size_type num_elem;

  mutable bool num_elem_is_updated;

  void reset_num_elem(const size_type & num = 0)
  {
    num_elem = num;
    num_elem_is_updated = true;
  }

  void update_num_elem()
  {
    assert(not num_elem_is_updated);

    size_type counter = 0;

    for (typename Dnode<T>::Iterator it(dlist); it.has_curr(); it.next_ne())
      ++counter;

    num_elem = counter;
    num_elem_is_updated = true;
  }

public:

  /** Iterador sobre List<T>

      
      @ingroup stl
   */
  class iterator
  {
    friend class list<T>;

    typedef typename Dnode<T>::Iterator Iterator;
    
    Iterator itor;

    bool underflow;
    bool overflow;

    void init_flags()
    {
      if (itor.has_curr())
	underflow = overflow = false;
      else
	underflow = overflow = true;
    }

    iterator(Dnode<T> & __list) : itor(__list)
    {
      init_flags();
    }

    void goto_begin()
    {
      itor.reset_first();
      init_flags();
    }

    void goto_last()
    {
      itor.reset_last();
      init_flags();
    }

    void goto_end()
    {
      itor.reset_last();
      init_flags();
      if (not overflow)
	itor.next();
      overflow = true;
    }

    void forward()
    {
      if (underflow)
	{
	  goto_begin();
	  return;
	}
      
      itor.next();

      if (not itor.has_curr())
	overflow = true;
    }

    void backward()
    {
      if (overflow)
	{
	  goto_last();
	  return;
	}
      
      itor.prev();

      if (not itor.has_curr())
	underflow = true;
    }

  public:

        /// Tipo de dato que almacena lista
    typedef typename list::value_type   value_type;

        /// Tipo numérico que representa diferencia entre posiciones de
        /// iteradores.
    typedef int                         difference_type;

        /// Tipo puntero a elemento dentro de la lista.
    typedef typename list::value_type * pointer;

        /// Tipo referencia a elemento dentro de la lista.
    typedef typename list::reference    reference;

        /// Instancia un iterador vacío (no asociado a una lista).
    iterator() : underflow(false), overflow(false) { /* empty */ }

        /// Proporciona una referencia al elemento actual de iterador.
    T & operator *()
    {
      return itor.get_curr()->get_data();
    }

        /// Dereferencia el elemento actual del iterador.
    T * operator ->()
    {
      return & itor.get_curr()->get_data();
    }

        /// Avanza el iterador hacia delante y luego carga el elemento
        /// actual. 
    iterator operator ++()
    {
      forward();
      return *this;
    }

        /// Carga el elemento actual y luego avanza el iterador hacia
        /// delante. 
    iterator operator ++ (int)
    {
      iterator return_value = *this;
      forward();
      return return_value;
    }

        /// Retrocede el iterador hacia atrás y luego carga el elemento
        /// actual. 
    iterator operator --()
    {
      backward();
      return *this;
    }

        /// Carga el elemento actual y luego retrocede el iterador hacia
        /// atrás. 
    iterator operator -- (int)
    {
      iterator return_value = *this;
      backward();
      return return_value;
    }
    
        /// Avanza el iterador n posiciones.
    iterator operator += (const size_type & n)
    {
      for (int i = 0; i < n; ++i)
	forward();

      return *this;
    } 

        /// Retrocede el iterador n posiciones.
    iterator operator -= (const size_type & n)
    {
      for (int i = 0; i < n; ++i)
	backward();

      return *this;
    } 

        /// Retorna true si los iteradores son iguales.
    bool operator == (const iterator & __itor)
    {
      return itor == __itor.itor;
    }

        /// Retorna true si los iteradores son distintos.
    bool operator != (const iterator& __itor)
    {
      return itor != __itor.itor;
    }

    bool verify (const list<T> & list) const
    {
      return itor.verify((Dlink*)(&list.dlist));
    }

    bool verify(const iterator & it) const
    {
      return itor.verify(it.itor);
    }
  }; // end class iterator

private:

  void copy(const list& _list)
  {
    assert(dlist.is_empty());
    assert(num_elem == 0);

    for (typename Dnode<T>::Iterator it(_list.dlist); 
	 it.has_curr(); it.next_ne())
      {
	Node * node = new Node(it.get_curr_ne()->get_data());

	dlist.append(node);
	++num_elem;
      }
  }

public:

      /// Instancia una lista vacía.
  list() : num_elem(0), num_elem_is_updated(true) { /* empty */ }

      /// Instancia una lista copia de c.
  list(const list & c) : num_elem(0), num_elem_is_updated(true)
  {
    copy(c);
  }

      /// Instancia una nueva lista de num elementos cuyo valor está
      /// determinado por lo que el constructor T::T().
  list(const size_type & num) : num_elem(0), num_elem_is_updated(true)
  {
    for (/* nothing */; num_elem < num; ++num_elem)
      dlist.append(new Node);
   }

      /** Instancia una nueva lista de num elementos con valor inicial
	  value. 

	  @param[in] num la cantidad inicial de elementos.
	  @param[in] value el valor que tendrán los elementos de la
	  lista.
	  @note Según el compilador y el contexto, el compilador puede
	  confundirse este constructor con el constructor
	  list(beg,end). Use conversión explícita de tipo si se le
	  presenta esta situación. 
      */
  list(const size_type & num, const T & value) 
    : num_elem(0), num_elem_is_updated(true)
  {
    for (/* nothing */; num_elem < num; ++num_elem)
      dlist.append(new Node(value));
  }

      /** Instancia una nueva lista con los elementos de un contenedor
	  comprendidos en el rango [beg..end).

	  Este constructor instancia una nueva lista con elementos
	  copiados desde un contenedor mediante el rango especificado
	  por [ben..end).

	  @param[in] beg inicio del rango en el contenedor de copia.
	  @param[in] end término del rango en el contenedor de copia.
	  @note Según el compilador y el contexto, cuando se usa el
	  constructor list(num, int), el compilador puede confundirse e
	  interpretar este constructor. Use conversión explícita de tipo
	  si se le presenta esta situación.
      */
      template <class Itor>
  list(Itor beg, const Itor & end) 
    : num_elem(0), num_elem_is_updated(true)
  {
    Aleph::verify_iterators(beg, end);

    while (beg != end)
      {
	dlist.append(new Node(*beg++));
	++num_elem;
      }
  }

  ~list() 
  {
    clear();
  }

      /// Retorna la cantidad de elementos que tiene la lista.
  size_type size() 
  {
    if (not num_elem_is_updated)
      update_num_elem();

    return num_elem;
  }

      /// Retorna true si la lista está vacía
  bool empty() const
  {
    return dlist.is_empty();
  }

      /// Retorna true si el contenido de this es exactamente el mismo
      /// que el de la lista c.
  bool operator == (const list & c) const
  {
    if (this == &c)
      return true;

    if (num_elem_is_updated and c.num_elem_is_updated)
      if (num_elem != c.num_elem)
	return false;

    typename Dnode<T>::Iterator it_l(*this), it_r(c.dlist);

    while (it_l.has_curr() and it_r.has_curr())
      {
	if (it_l.get_curr_ne()->get_data() != it_r.get_curr_ne()->get_data())
	  return false;

	it_l.next_ne();
	it_r.next_ne();
      }

    if (it_l.is_empty() and it_r.is_empty())
      return true;
    
    return false;
  }

      /// Retorna false si la lista this contiene al menos un elemento
      /// secuencialmente distinto al de la lista c.
  bool operator != (const list & c) const
  {
    return not (*this == c);
  }

      /// Retorna true si la lista this es lexicográficamente menor que
      /// la lista c.
  bool operator < (const list & c) const
  {
    if (this == &c)
      return false;

    typename Dnode<T>::Iterator it_l(*this);
    typename Dnode<T>::Iterator it_r(c);

    while (it_l.has_curr() and it_r.has_curr())
      {
	if (it_l.get_curr_ne()->get_data() < it_r.get_curr_ne()->get_data())
	  return true;
	else if (it_r.get_curr()->get_data() < 
		 it_l.get_curr()->get_data())
	  return false; // no son iguales

	it_l.next_ne();
	it_r.next_ne();
      }

    return it_l.is_empty();
  }

      /// Retorna true si la lista this es lexicográficamente mayor que
      /// la lista c. 
  bool operator > (const list & c) const
  {
    return c < *this;
  }

      /// Retorna true si la lista this es lexicográficamente menor o
      /// igual que la lista c. 
  bool operator <= (const list & c) const
  {
    return not (c > *this );
  }

      /// Retorna true si la lista this es lexicográficamente mayor o
      /// igual que la lista c. 
  bool operator >= (const list & c) const
  {
    return not (*this < c);
  }

      /// Elimina todos los elementos de this y le asigna todos los
      /// elementos de c.
  list & operator = (const list & c)
  {
    if (this == &c)
      return *this;

    clear();
    copy(c);
  }

      /** Elimina todos los elementos de this e inserta num elementos
	  con valor value.

	  @param[in] num cantidad de elementos a insertar.
	  @param[in] value valor a copiar.
	  @note Según el compilador y el contexto, el compilador puede
	  confundir esta operación con assign(beg,end). Use conversión
	  explícita de tipo si se le presenta esta situación.
      */
  void assign(const size_type & num, const T & value)
  {
    clear();
    
    for (int n = 0; n < num; ++n)
      push_back(value);
  }

      /** Elimina todos los elementos de this e inserta en el vector los
	  elementos de un contenedor en el rango comprendido entre
	  [beg..end). 

	  @param[in] beg comienzo del rango.
	  @param[in] end término del rango.
	  @note Según el compilador y el contexto, el compilador puede
	  confundir esta operación con assign(num,value). Use conversión
	  explícita de tipo si se le presenta esta situación.
      */
      template <typename Itor>
  void assign (Itor beg, const Itor & end)
  {
    Aleph::verify_iterators(beg, end);

    clear();
    
    while (beg != end)
      push_back(*beg++);
  }
    
      /// Intercambian en tiempo constante (muy rápido) todos los
      /// elementos del vector this con los del vector c.
  void swap(list & c)
  {
    dlist.swap(c.dlist);
    std::swap(num_elem, c.num_elem);
    std::swap(num_elem_is_updated, c.num_elem_is_updated);
  }

      /// Retorna una referencia al primer elemento de la lista.
  reference front() const
  {
    return dlist.get_next()->get_data();
  }

      /// Retorna una referencia al último elemento de la lista.
  reference back() const
  {
    return dlist.get_prev()->get_data();
  }

      /// Retorna un iterador posicionado en el primer elemento de la
      /// lista. 
  iterator begin() const
  {
    return iterator(dlist);
  }
  
      /// Retorna un iterador posicionado en el último elemento de la
      /// lista. 
  iterator end() const
  {
    iterator _itor(dlist);
    _itor.goto_end();

    return _itor;
  }
 
      /// Inserta el valor de value a partir del elemento actual del
      /// iterador pos. Retorna un iterador posicionado sobre el
      /// elemento recién insertado.
  iterator insert(const iterator & pos, const T & value)
  {
    Aleph::verify_container_and_iterator(*this, pos);

    Node * new_node = new Node(value);

    Node * current_node = pos.itor.get_curr();

    current_node->append(new_node);

    pos.itor.set(new_node);

    ++num_elem;

    return pos;
  }

      /// Inserta en la lista, a partir de la posición actual de pos,
      /// num copias del valor value. 
  void insert(iterator pos, const size_type & num, const T & value)
  {
    Aleph::verify_container_and_iterator(*this, pos);

    Node new_list;

    try
      {
	for (int i = 0; i < num; ++i)
	  new_list.append (new Node(value));

	Node * current_node = pos.itor.get_curr();
	current_node->insert_list(&new_list);
	num_elem += num;
      }
    catch (...)
      {
	new_list.remove_all_and_delete();
	throw;
      }
  }

      /// Inserta en la lista, a partir de la posición pos, los
      /// elementos de un contenedor comprendidos en el rango [beg..end).
      template <class Itor>
  void insert(iterator pos, Itor beg, const Itor & end)
  {
    Aleph::verify_container_and_iterator(*this, pos);
    Aleph::verify_iterators(beg, end);

    Node new_list;
    try
      {
	while (beg != end)
	  {
	    new_list.append(new Node(*beg++));
	    ++num_elem;
	  }

	Node * current_node = pos.itor.get_curr();
	current_node->insert_list(&new_list);
      }
    catch (...)
      {
	new_list.remove_all_and_delete();
	throw;
      }
  }

      /// Inserta al principio de la lista el valor value.
  void push_front(const T & value)
  {
    dlist.insert(new Node(value));
    ++num_elem;
  }

  void push_back(const T & value)
  {
    dlist.append(new Node(value));
    ++num_elem;
  }

      /// Inserta al final de la lista el valor value.
  void remove(const T & value)
  {
    for (typename Dnode<T>::Iterator it(dlist); it.has_curr(); /* nothing */)
      if (it.get_curr_ne()->get_data() == value)
	{
	  delete it.del();
	  --num_elem;
	}
      else
	it.next_ne();
  }

      /// Elimina el elemento posicionado por el iterador pos.
  iterator erase(iterator pos)
  {
    Aleph::verify_container_and_iterator(*this, pos);

    delete pos.itor.del();
    --num_elem;

    return pos;
  }

      /// Elimina de la lista los elementos del rango
      /// [[beg..end). Retorna un iterador posicionado en el primer
      /// elemento sucesor a los eliminados.
  iterator erase(iterator beg, const iterator & end)
  {
    Aleph::verify_container_and_iterator(*this, beg);
    Aleph::verify_iterators(beg, end);

    while (beg != end)
      {
	delete  beg.itor.del();
	--num_elem;
      }

    return beg;
  }

      /// Elimina de la lista el primer elemento.
  void pop_front()
  {
    delete dlist.remove_next();
    num_elem--;
  }

      /// Elimina de la lista el último elemento.
  void pop_back()
  {
    delete dlist.remove_prev();
    --num_elem;
  }

      /// Elimina todos los elementos de la lista.
  void clear()
  {
    dlist.remove_all_and_delete();
    reset_num_elem();
  }

      /** Reajusta el tamaño de la lista a num elementos.

	  resize(num,t) puede tener dos comportamientos según el valor
	  de num:
	  -# Si num es menor que la cantidad de elementos de la lista,
	  entonces se eliminan los elementos finales de la lista para
	  que ésta sea de tamaño num.
	  -# De lo contrario, se insertan elementos al final de la lista
	  hasta que su tamaño devenga num. 
	  .

	  @param[in] num nuevo tamaño de la lista.
	  @param[in] t valor a insertar si num es mayor que el tamaño de
	  la lista. Por omisión, si no se especifica este parámetros,
	  entonces el valor a insertar será el arrojado por el
	  constructor T::T().
	  @throw bad_alloc si no hay suficiente memoria.
       */
  void resize(size_type num, const T & t = T())
  {
    if (num == size())
      return;
    
    if (num < num_elem)
      {
	while (num_elem > num)
	  {
	    delete dlist.remove_prev();
	    --num_elem;
	  }

	return;
      }

    iterator itor(end());
    --itor;
    insert(itor, num - num_elem, t);
  }

      /// Elimina de la lista todos los elementos duplicados
      /// consecutivos cuyo criterio op sea true.
      template <class Op>
  void unique(const Op & op) 
  {
    reset_num_elem(); // recordar que coloca num_elem = 0
    
    for (typename Dnode<T>::Iterator it1(dlist); it1.has_curr(); 
	 it1.next_ne(), ++num_elem)
      {
	typename Dnode<T>::Iterator it2(it1); it2.next_ne();

	while (it2.has_curr())
	  if (op(it1.get_curr_ne()->get_data(), it2.get_curr_ne()->get_data()))
	    delete it2.del();
	  else
	    break;
      }
  }

      /// Elimina de la lista todos los elementos duplicados consecutivos.
  void unique()  
  {
    unique(Aleph::equal_to<T>());
  }

      /** Particiona una lista a partir de la posición del iterador pos

	  splice(post,l) particiona la lista this a partir del iterador
	  pos y coloca los elementos a partir de pos en la lista l.

	  splice() particiona la lista en tiempo contante \f$O(1)\f$.

	  @param[in] pos iterador cuya posición indica punto de
	  partición de la lista.
	  @param[out] l lista donde se colocan los elementos de la lista
	  particionada. 
       */  
  void splice(iterator pos, list & l)
  {
    Aleph::verify_container_and_iterator(*this, pos);

    pos.itor.get_curr()->insert_list(&l.dlist);

    num_elem_is_updated = l.num_elem_is_updated;
    num_elem += l.num_elem;

    l.reset_num_elem();

    assert(l.dlist.is_empty());
  }

      /** Mueve el elemento en la posición src_pos de la lista src_list
	  al frente del elemento pos de la lista this.

	  splice(pos,src_list,src_pos) mueve el elemento actual de
	  src_pos, que itera sobre la lista src_list, al frente del
	  elemento pos de la lista this.

	  @param[in] pos posición dentro de this donde se insertará
	  elemento de la lista src_list.
	  @param[out] src_list lista origen de donde se moverá el
	  elemento.
	  @param[in] src_pos posición en src_list del elemento a mover.
       */
  void splice(iterator pos, list & src_list, iterator src_pos)
  {
    Aleph::verify_container_and_iterator(*this, pos);
    Aleph::verify_container_and_iterator(src_list, src_pos);

    pos.itor.get_curr()->insert(src_pos.itor.del());
    --src_list.num_elem;
    ++num_elem;
  }

      /// Mueve el rango de elementos [src_beg,src_end) de la lista src_list
      /// hacia la lista this en tiempo \f$O(1)\f$.
  void splice(iterator pos, list & src_list, 
	      iterator src_beg, const iterator & src_end)
  {
    Aleph::verify_container_and_iterator(*this, pos);
    Aleph::verify_container_and_iterator(src_list, src_beg);
    Aleph::verify_iterators(src_beg, src_end);    

    Dlink list_to_insert;
    src_list.dlist.cut_list(src_beg.itor.get_curr(), &list_to_insert);

    Dlink remaining_list;
    list_to_insert.cut_list(src_end.itor.get_curr(), &remaining_list);
    
    pos.itor.get_curr()->insert_list(&list_to_insert);
    num_elem_is_updated = false;

    src_list.dlist.concat_list(&remaining_list);
    src_list.num_elem_is_updated = false;
  }

      /// Ordena la lista según criterio de comparación Cmp.
      template <class Cmp>
  void sort(const Cmp &)
  {
    quicksort<T, Cmp>(dlist);	
  }

      /// Ordena la lista.
  void sort()
  {
    sort(Aleph::less<T>()); 
  }

      /// Mezcla dos listas ordenadas según criterio de comparación Cmp.
      template <class Cmp>
  void merge(list & l, const Cmp &)
  {
    Dnode<T> result;

    Aleph::merge_lists<T, Cmp>(dlist, l.dlist, result);

    dlist.swap(result);

    num_elem += l.num_elem;

    l.reset_num_elem();

    assert(l.dlist.is_empty());
  }

      /// Mezcla dos listas ordenadas.
  void merge(list & l)
  {
    merge(l, Aleph::less<T>());
  }

      /// Invierte la lista.
  void reverse()
  {
    reset_num_elem(dlist.reverse_list());
  }
};


    template <typename T>
typename list<T>::size_type distance(typename list<T>::iterator it1, 
				     typename list<T>::iterator it2)
{
  typename list<T>::size_type counter = 0;

  while (it1 != it2)
    {
      counter++;
      it1++;
    }

  return counter;
}

} // end namespace Aleph

# endif // ALEPH_LIST_H
