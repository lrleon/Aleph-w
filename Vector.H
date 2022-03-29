

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
; num_elem < num; num_elem++)
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
