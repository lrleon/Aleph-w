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

/** @file Vector.H
 *  @brief STL-compatible vector container (legacy).
 *
 *  @deprecated This is a legacy Aleph implementation of std::vector.
 *  It will be deprecated in future versions. For new code, prefer using
 *  `std::vector` directly or `Aleph::DynArray` for Aleph-native features.
 *
 *  Aleph implementation of std::vector with dynamic array storage.
 *
 *  @see tpl_dynArray.H Recommended alternative dynamic array
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

/*
	Aleph implementation of vector C++ standard container
*/

# ifndef ALEPH_VECTOR_H
# define ALEPH_VECTOR_H

# include <tpl_dynArray.H>
# include <ahIterator.H>
# include <ah_stdc++_utils.H>

namespace Aleph
{

template <class T> class vector;

    template <typename T> 
typename iterator_traits<typename vector<T>::iterator>::difference_type 
distance(typename vector<T>::iterator, typename vector<T>::iterator);

  /** Implantación Aleph del contenedor estándar C++ vector<T>.

      Esta implantación, basada en el tipo DynArray, tiene como ventaja
      sobre otras implantaciones de vector<T> su alta capacidad de
      relocalización, la cual se comprende bajo dos bondades al momento
      de que se requiera mayor capacidad:
      -# Ninguna de las entradas por debajo de la capacidad actual
      es relocalizada. Esto redunda en la posibilidad de mantener con
      seguridad punteros a entradas dentro del vector.
      -# La cantidad de memoria que se aparta cuando el vector crece es
      proporcional a la diferencia entre las nueva y antigua
      dimensiones. 
      .

      Las operaciones sobre un vector no efectúan verificación de
      rango. 

      Siendo derivado de DynArray, esta implantación es más lenta en
      acceso que otra implantación basada estrictamente en vectores
      contiguos de memoria.

      @ingroup stl
   */
    template <typename T>
class vector
{
public:

      /// El tipo de dato que almacena el vector.
  typedef T value_type;

      /// Tipo referencia a una entrada del vector.
  typedef typename vector::value_type & reference;

      /// Tipo referencia constante a una entrada del vector.
  typedef const typename vector::value_type & const_reference;

      /// Tipo numérico utilizados para tamaños e índices del vector.
  typedef size_t size_type;

private:

  mutable DynArray<T> array;

  mutable size_type num_elem;

public:

  /** Iterador sobre un vector<T>.

      @ingroup stl
   */
  class iterator 
  { 
    friend class vector<T>;
    
    static const int Invalid_Position = -1;

    mutable DynArray<T> * dyn_array_ptr;

    mutable size_type current_position;

    T cookie_data;

    iterator(const vector<T> & vec, const size_t & pos = 0)
      :  dyn_array_ptr(&vec.array), current_position(pos) 
    { 
      /* empty */ 
    }

    void set_pos(const size_t &)
    {
      current_position = this->num_elem;
    }

    int get_position() const { return current_position; }

    DynArray<T> * get_dyn_array_ptr() { return dyn_array_ptr; }

    T & access(const size_type & i)
    {
      if (i >= dyn_array_ptr->size())
	return cookie_data;

      return dyn_array_ptr->access(i);
    }

  public:

        /// El tipo de dato que almacena el vector.
    typedef typename vector<T>::value_type   value_type;

        /// Tipo numérico que representa diferencia entre posiciones de
        /// iteradores. 
    typedef int                              difference_type;

        /// Tipo puntero a entrada dentro del vector.
    typedef typename vector<T>::value_type * pointer;

        /// Tipo referencia a un elemento dentro del vector.
    typedef typename vector<T>::reference    reference;

        /// Instancia un iterador sin contenedor asociado.
    iterator() : dyn_array_ptr(nullptr), current_position(Invalid_Position) 
    { 
      /* empty */ 
    }

        /// Instancia un iterador copia de otro iterador
    iterator(const iterator & itor)
      : dyn_array_ptr(itor.dyn_array_ptr),
	current_position(itor.current_position) 
    { 
      /* empty */ 
    }
    
        /// Asigna un iterador.
    iterator & operator = (const iterator & itor)
    {
      if (&itor == this)
	return *this;

      dyn_array_ptr = itor.dyn_array_ptr;
      current_position  = itor.current_position;

      return *this;
    }

        /// Acceso por posición; es equivalente al mismo acceso en el vector.
    T & operator [] (const size_type & index)
    {
      current_position = index;

      return access(index);
    }

        /// Asigna al elemento actual del iterador el valor
        /// key. El iterador no cambia de posición.
    iterator & operator = (const T & key)
    {
      access(current_position) = key;

      return *this;
    }

        /// Retorna una referencia al elemento actual del iterador.
    T & operator * ()
    {
      return dyn_array_ptr->access(current_position);
    }
    
        /// Dereferencia el elemento actual del iterador.
    T * operator -> ()
    {
      return & dyn_array_ptr->access(current_position);
    }
    
        /// Avanza el iterador una posición adelante, luego carga el
        /// elemento actual.
    iterator operator ++ ()
    {
      current_position++;
      return *this;
    }

        /// Carga el elemento actual y luego avanza el iterador hacia
        /// delante. 
    iterator operator ++ (int)
    {
      iterator return_value = *this;
      current_position++;
      return return_value;
    }

        /// Retrocede el iterador una posición atrá, luego carga el
        /// elemento actual.
    iterator operator -- ()
    {
      current_position--;
      return *this;
    }

        /// Carga el elemento actual y luego retrocede  el iterador hacia
        /// atrás. 
    iterator operator -- (int)
    {
      iterator return_value = *this;
      current_position--;
      return return_value;
    }

        /// Avanza el iterador n posiciones hacia delante.
    iterator operator += (const size_type & n)
    {
      current_position += n;
      return *this;
    }

        /// Retrocede el iterador n posiciones hacia atrás.
    iterator operator -= (const size_type & n)
    {
      current_position -= n;
      return *this;
    }

        /// Retorna true si elemento actual de this es anterior al elemento
        /// actual de itor.
    bool operator < (const iterator & itor) const
    {
      return current_position < itor.current_position;
    }
    
        /// Retorna true si elemento actual de this es anterior o igual
        /// al elemento actual de itor.
    bool operator <= (const iterator & itor) const
    {
      return current_position <= itor.current_position;
    }

        /// Retorna true si elemento actual de this es posterior al elemento
        /// actual de itor.
    bool operator > (const iterator & itor) const
    {
      return current_position > itor.current_position;
    }
    
        /// Retorna true si elemento actual de this es posterior o igual
        /// al elemento actual de itor.
    bool operator >= (const iterator& itor) const
    {
      return current_position >= itor.current_position;
    }
    
        /// Retorna true si la posición actual de this es igual a la de
        /// itor. 
    bool operator == (const iterator & itor) const
    {
      return current_position == itor.current_position;
    }
    
        /// Retorna true si la posición actual de this es igual a la de
        /// itor. 
    bool operator != (const iterator & itor) const
    {
      return current_position != itor.current_position;
    }    
    
        /// Retorna la diferencia de posiciones entre el iterador this e
        /// itor. 
    int operator - (const iterator & itor) const
    {
      return current_position - itor.current_position;
    }

        /// Retorna un iterador n posiciones adelante del elemento
        /// actual.
    iterator operator + (const int & n) const
    {
      iterator ret_val(*this);

      ret_val.current_position += n;

      return ret_val;
    }

    bool verify(const DynArray<T> & array) const
    {
      return &array == dyn_array_ptr;
    }

    bool verify(const iterator & it) const
    {
      return dyn_array_ptr == it.dyn_array_ptr;
    }
  };

      /// Instancia un vector vacío.
  vector() : array(0), num_elem(0) { /* empty */ }

      /// Instancia un vector copia de v.
  vector (const vector & c) : array(c.array), num_elem(c.num_elem)
  {
    // empty
  }

      /// Instancia un vector con capacidad inicial de num.
  vector (const size_type & num) : array(num), num_elem(num)
  {
    array.reserve(0, num_elem - 1);
  }

      /// Instancia un nuevo vector con copia de los elementos comprendidos
      /// en el rango [beg..end) de un contenedor. 
      template <typename Itor> explicit 
  vector (Itor beg, const Itor & end) : array(0), num_elem(0)
  {
    Aleph::verify_iterators(beg, end);

    while (beg != end)
      array[num_elem++] = *beg++;

    assert(num_elem == array.size());
  }

      /// Instancia un nuevo vector de num elementos de valor value.
  vector (const size_type & num, const T & value) : array(num), num_elem(num)
  {
    array.reserve(0, num_elem - 1);
        
    for(size_type i = 0; i < num; i++)
      array.access(i) = value;
  }

  ~vector() { /* empty */ }

      /// Retorna el número de elementos que tiene el vector.
  const size_type & size() const { return num_elem; }

      /// Retorna true si el vector está vacío.
  bool empty() const { return num_elem == 0; }

      /// Retorna la máxima dimensión que puede tener el vector.
  size_type max_size() const { return array.max_size(); }

      /// Retorna la capacidad (dimension actual) del vector.
  size_type capacity() const { return array.size(); }  

      /// Aumenta la capacidad del vector en num entradas.
  void reserve (const size_type & num)
  {
    if (num < array.size())
      return;

    array.reserve(array.size(), array.size() + num);  
  }

      /// Reajusta la capacidad del vector a num entradas.
  void resize (const size_type & num)
  {
    if (num < array.size())
      {
	num_elem = num;
	return;
      }

    reserve(num - array.size());
  }
    
      /// Reajusta la capacidad a num entradas con valor value (en caso
      /// de que num sea mayor que la capacidad actual.
  void resize (const size_type & num, const T & value)
  {
    if (num < array.size())
      {
	num_elem = num;
	return;
      }

    reserve(num - array.size());

    for (/* nothing */; num_elem < num; num_elem++)
      array.access(num_elem) = value;
  }

      /// Asigna un vector.
  vector & operator = (const vector & c)
  {
    if (this == &c)
      return *this;

    array = c.array;
    num_elem = c.num_elem;

    return *this;
  }

      /// Asigna al vector num entradas con valor value.
  void assign (const size_type & num, const T & value)
  {
    if (num > array.size())
      array.reserve(0, num - 1);

    num_elem = num;

    for(size_type i = 0; i < num; i++)
      array.access(i) = value;
  }

      /// Asigna al vector los elementos de un contenedor comprendido en
      /// el rango [beg..end).
      template <typename Itor> 
  void assign (Itor beg, const Itor & end)
  {
    Aleph::verify_iterators(beg, end);

    num_elem = 0;
    while (beg < end)
      array[num_elem++] = *beg++;
  }

      /// Intercambia todos los elementos de this con los de c en tiempo
      /// contante (muy rápido).
  void swap(vector & c)
  {
    std::swap(num_elem, c.num_elem);
    array.swap(c.array);
  }
    
      /// Retorna una referencia al elemento idx del vector.
  reference at(const size_type & idx) 
  { 
    return array[idx]; 
  }

        /// Retorna una referencia constante al elemento idx de un
        /// vector constante.
  const_reference at(const size_type & idx) const 
  { 
    return array[idx]; 
  }

      /// Accede a la entrada idx.
  reference operator [] (const size_type & idx) 
  { 
    return array.access(idx); 
  }

      /// Accede a la entrada idx de un vector constante
  const_reference operator [] (const size_type & idx) const 
  {
    return array.access(idx); 
  }

      /// Retorna una referencia al elemento del frente (el primero
      /// copiado y el menos reciente si se inserta secuencialmente).
  reference front() const
  { 
    return array.access(0); 
  }

      /// Retorna una referencia al elemento del trasero (el último
      /// copiado y el más reciente si se inserta secuencialmente).
  reference back() const
  { 
    return array.access(num_elem - 1); 
  }

      /// Retorna un iterador al frente del vector; el primero si fue
      /// insertado secuencialmente (índice 0).
  iterator begin() const
  {
    return iterator (*this); 
  }
  
      /// Retorna un iterador al frente del vector; el último si fue
      /// insertado secuencialmente.
  iterator end() const
  { 
    return iterator (*this, num_elem); 
  }

private:

  void open_gap(const size_t & position, const size_type & gap_len = 1)
  {
    const size_type old_size = array.size();
    array.reserve(old_size, old_size + gap_len - 1);

    if (position >= old_size)
      return;

    const size_t final_pos = position + gap_len;
    for (int i = array.size() - 1; i > final_pos; i--)
      array.access(i) = array.access(i - gap_len);
  }

public:

      /// Inserta value en la posición dada por el iterador pos.
  iterator insert(const iterator & pos, const T & value)
  {
    Aleph::verify_container_and_iterator(array, pos);

    open_gap(pos.get_position());

    array.access(pos.get_position()) = value;
  
    num_elem++;

    return pos;    
  }

      /// Inserta len instancias de value en la posición dada por el
      /// iterador pos. 
  void insert(iterator pos, const size_type & len, const T & value)
  {
    Aleph::verify_container_and_iterator(array, pos);

    open_gap(pos.get_position(), len);

    for (int i = 0; i < len; i++, pos++, num_elem++)
      array.access(i) = value;
  }

  /// Inserta a partir de la posición dada por el iterador pos, los
  /// elementos de un contenedor del rango [beg..end)
      template <class Itor>
  void insert(const iterator & pos, Itor beg, const Itor & end)
  {
    Aleph::verify_container_and_iterators(array, pos, beg, end);
    
    size_type gap_len = Aleph::distance<T>(beg, end);

    open_gap(pos.get_position(), gap_len);

    num_elem += gap_len;

    for (int i = pos.get_position(); gap_len > 0; i++, gap_len--)
      array.access(i) = *beg++;
  }

      /// Inserta value al final del vector.
  void push_back (const T & value)
  {
    array[num_elem] = value;
    num_elem++;
  }

private:

  void close_gap(const size_t & position, const size_type & len = 1)
  {
    for (int i = position; i < num_elem - len; i++)
      array.access(i) = array.access(i + len);

    num_elem -= len;
  }

public:

      /// Borra el elemento de la posición pos.
  iterator erase (const iterator & pos)
  {
    Aleph::verify_container_and_iterator(array, pos);

    close_gap(pos.get_position());

    return iterator(*this, pos.get_position());
  }

      /// Elimina los elementos comprendidos en el rango del vector
      /// [[beg..end]]. Retorna un iterador posicionado sobre el elemento
      /// perteneciente al vector que queda en la posición beg.
  iterator erase (const iterator & beg, const iterator & end)
  {
    Aleph::verify_container_and_iterators(array, beg, end);

    const size_t gap_last  = 
      end.get_position() <= num_elem ? end.get_position() : num_elem;
    const size_t gap_start = beg.get_position();
    const size_t gap_len   = gap_last - gap_start;

    if (gap_start > gap_last)
      return iterator(*this, num_elem);

    close_gap(gap_start, gap_len);

    return iterator(*this, gap_start);
  }

      /// Elimina el elemento situado al final del vector.
  void pop_back() { num_elem--; }

      /// Elimina todos los elementos del vector.
  void clear() { num_elem = 0; }

      /// Retorna true si todos los elementos del vector this son
      /// iguales a los elementos del vector r.
  bool operator == (const vector & r) const
  {
    if (this == &r)
      return true;

    if (num_elem != r.num_elem)
      return false;

    const size_t len = std::min( num_elem, r.num_elem);

    for (size_t i = 0; i < len; i++)
      if (array.access(i) != r.array.access(i))
	return false;

    return true;
  }

      /// Retorna true si this es distinto que el vector r.
  bool operator != (const vector & r) const
  {
    return not (*this == r);
  }
  
      /// Retorna true si los elementos del vector this son
      /// lexicográficamente menores que los elementos del vector r.
  bool operator < (const vector & r) const
  {
    if (this == &r)
      return false;

    const bool l_smaller = num_elem < r.num_elem;

    const size_t len = l_smaller ? num_elem : r.num_elem;

    for (size_t i = 0; i < len; i++)
      if (array.access(i) < r.array.access(i))
	return true;
      else if (r.array.access(i) < array.access(i))
	return false; // ==> no son iguales 

    return l_smaller;
  }

      /// Retorna true si los elementos del vector this son
      /// lexicográficamente menores que los elementos del vector r.
  bool operator > (const vector & r) const
  {
    return r < *this;
  }

      /// Retorna true si los elementos del vector this son
      /// lexicográficamente menores o iguales que los elementos del
      /// vector r. 
  bool operator <= (const vector & r) const
  {
    return not (r > *this);
  }

      /// Retorna true si los elementos del vector this son
      /// lexicográficamente mayores o iguales que los elementos del
      /// vector r. 
  bool operator >= (const vector & r) const
  {
    return not (*this < r);
  }
};



    template <typename T> 
typename iterator_traits<typename vector<T>::iterator>::difference_type 
distance(typename vector<T>::iterator it1, typename vector<T>::iterator it2)
{
  return it2 - it1;
}

} // end namespace Aleph

# endif // ALEPH_VECTOR_H
