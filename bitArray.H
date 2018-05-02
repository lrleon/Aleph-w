
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

# ifndef BITARRAY_H
# define BITARRAY_H

# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_dynArray.H>

namespace Aleph {

class Byte
{
  unsigned int b0 : 1;
  unsigned int b1 : 1;
  unsigned int b2 : 1;
  unsigned int b3 : 1;
  unsigned int b4 : 1;
  unsigned int b5 : 1;
  unsigned int b6 : 1;
  unsigned int b7 : 1;

public:

  unsigned int read_bit(const unsigned int i) const noexcept
  {
    assert(i < 8);

    switch (i)
      {
      case 0 : return b0;
      case 1 : return b1;
      case 2 : return b2;
      case 3 : return b3;
      case 4 : return b4;
      case 5 : return b5;
      case 6 : return b6;
      case 7 : return b7;
      default: assert(false);
      }
    return 0; // never reached
  } 

  void write_bit(const unsigned int i, const unsigned int value) noexcept
  {
    assert(i < 8);
    assert(value <= 1);

    switch (i)
      {
      case 0 : b0 = value; break;
      case 1 : b1 = value; break;
      case 2 : b2 = value; break;
      case 3 : b3 = value; break;
      case 4 : b4 = value; break;
      case 5 : b5 = value; break;
      case 6 : b6 = value; break;
      case 7 : b7 = value; break;
      default: assert(false);
      }
  } 

  Byte() noexcept : b0(0), b1(0), b2(0), b3(0), b4(0), b5(0), b6(0), b7(0) {}

  int get_int() const noexcept
  {
    const unsigned char * const ptr = (unsigned char*) this;
    return *ptr;
  }

  void set_int(int i) noexcept
  {
    unsigned char * ptr = (unsigned char*) this;
    *ptr = i;
  }

  Byte & operator |= (const Byte & rhs) 
  {
    (unsigned char&) *this |= (unsigned char&) rhs;
    return *this;
  }

  Byte & operator &= (const Byte & rhs) 
  {
    (unsigned char&) *this &= (unsigned char&) rhs;
    return *this;
  }
};

    /** Arreglo contiguo de bits.

        BitArray define una arreglo contiguo de bits. El consumo de
        memoria es proporcional a la n/8; donde n es la cantidad de bits
        que se desean manejar.

        El arreglo es dinámico, lo que implica que éste puede expandirse
        dinámicamente según la necesidad.

        El acceso al i-ésimo bit puede realizarse mediante el operador
        []. Sin embargo, puesto que esta clase de acceso está
        implementada mediante una clase proxy, el acceso puede fallar en
        algunas expresiones donde el compilador no logre efectuar las
        transformaciones de tipos adecuadas.

        @ingroup Secuencias
	@author Alejandro Mujica
	@author Leandro Rabindranath Leon
     */
class BitArray
{
  size_t         current_size;
  DynArray<Byte> array_of_bytes; 

  size_t get_num_bytes() const noexcept
  {
    size_t div = current_size/8;

    return (current_size % 8) != 0 ? div + 1 : div;
  }

  class BitProxy
  {
    const size_t index;
    const size_t bit_index;
    const size_t byte_index;
    BitArray *   array;
    Byte *       byte_ptr;

  public:

    BitProxy(BitArray & a, const size_t i) noexcept
      : index(i), bit_index(i % 8), byte_index(i/8), array(&a)
    {
      if (array->array_of_bytes.exist(byte_index))
        byte_ptr = &array->array_of_bytes.access(byte_index);
      else
        byte_ptr = nullptr;
    }
    
    operator int () const 
    {
      if (index >= array->current_size)
        throw std::out_of_range("Index out of range");

      return byte_ptr != nullptr ? byte_ptr->read_bit(bit_index) : 0; 
    }

    BitProxy & operator = (const size_t value)
    {
      assert(value <= 1);

      if (byte_ptr == nullptr)
        byte_ptr = &array->array_of_bytes.touch(byte_index);

      if (index >= array->current_size)
        array->current_size = index;

      byte_ptr->write_bit(bit_index, value);

      return *this;
    }

    BitProxy & operator = (const BitProxy & proxy)
    {
      if (byte_ptr == nullptr)
        byte_ptr = &array->array_of_bytes.touch(byte_index);

      if (index >= array->current_size)
        array->current_size = index;

      byte_ptr->write_bit(bit_index, proxy.byte_ptr->read_bit(proxy.bit_index));

      return *this;
    }
  };

public:

  using Item_Type = unsigned char;

      /** Constructor de arreglo de dim bits.

          @param[in] dim cantidad de bits. Por omisión es 256.
          @throw bad_alloc si no hay suficiente memoria para un bloque
          continuo de dim/8 bytes       
       */
  BitArray(const size_t dim = 0) throw(std::exception, std::bad_alloc) 
    : current_size(dim), array_of_bytes(get_num_bytes())
  {
    array_of_bytes.set_default_initial_value(Byte());
  }
  
  /** Reserva por adelantado memoria para el arreglo de bits a la
      dimensión dim.

      reserve() aparta por adelantado la memoria suficiente para guardar
      dim bits. De este modo, los accesos de escritura o lectura pueden
      hacerse directa y más rápidamente mediante los métodos read() y
      write().

      @throw bad_alloc si no hay suficiente memoria
   */
  void reserve(size_t dim)
  {
    current_size = dim;

    int num_bytes = dim/8;
    if (dim % 8)
      ++num_bytes;
    
    array_of_bytes.reserve(num_bytes);
  }

      /// Retorna la dimensión del arreglo de bits 
  size_t size() const noexcept { return current_size; }

      /// Reajusta la dimensión del arreglo
  void set_size(const size_t sz) 
  {
    size_t array_size = sz / 8;
    if (array_size % 8)
      ++array_size;
    
    array_of_bytes.adjust(array_size);
    current_size = sz;
  }

  BitProxy operator [] (const size_t i) const noexcept
  { 
    return BitProxy(const_cast<BitArray&>(*this), i);
  }

  BitProxy operator [] (const size_t i) noexcept { return BitProxy(*this, i); }

       /** Lee el bit i.

           @param[in] i índice del bit a ser leído.
           @return el valor del bit i
           @note No se verifica fuera de rango.
        */
  int read_bit(const size_t i) const
  {
    if (i >= current_size)
      throw std::out_of_range("index out of range");

    const int bit_index = i % 8;
    const Byte byte = array_of_bytes[i/8];
    return byte.read_bit(bit_index);
  }

  int operator () (const size_t i) const { return read_bit(i); }

       /** Escribe el bit i con el valor value

           @param[in] i índice del bit a ser escrito.
           @param[in] value valor a ser escrito en la posición i.
           @note No se verifica fuera de rango y no verifica que value
           tenga un valor binario. 
        */
  void write_bit(const size_t i, const unsigned int value) 
  {
    array_of_bytes.touch(i/8).write_bit(i%8, value);
    if (i >= current_size)
      current_size = i + 1;
  }

  /** Lectura rápida del bit i.

      Lee el bit i sin verificar memoria

      @param[in] i índice del bit a ser leído.
      @return el valor del bit i
      @note No se verifica fuera de rango.
  */
  int read(const size_t i) const
  {
    if (i >= current_size)
      throw std::out_of_range("index out of range");

    const int bit_index = i % 8;
    return array_of_bytes.access(i/8).read_bit(bit_index);
  }

  /** Escribe el bit i con el valor value sin verificación de memoria.

      @param[in] i índice del bit a ser escrito.
      @param[in] value valor a ser escrito en la posición i.
      @note No se verifica fuera de rango y no verifica que value
      tenga un valor binario. 
  */
  void write(const size_t i, const unsigned int value) 
  {
    array_of_bytes.access(i/8).write_bit(i%8, value);
    if (i >= current_size)
      current_size = i + 1;
  }

  int fast_read(const size_t i) const noexcept
  {
    return array_of_bytes.access(i/8).read_bit(i%8);
  }

  void fast_write(const size_t i, const unsigned int value) 
  {
    array_of_bytes.access(i/8).write_bit(i%8, value);
  }

    /// Inserta al final del arreglo el valor value.
  void push(const unsigned int value)
  {
    write_bit(current_size, value);
  }

    /// Elimina el último bit del arreglo.
  void pop()
  {
    current_size--;
    array_of_bytes.cut(get_num_bytes());
  }

      /// Elimina todos los bits insertados
  void empty() noexcept
  {
    current_size = 0;
    array_of_bytes.cut();
  }

      /** Constructor copia.

          Instancia un arreglo de bits copia exacta de array.

          @param[in] array el arreglo de bits a ser copiado.
          @throw bad_alloc si no hay suficiente memoria para un bloque
          continuo de dim/8 bytes       
      */
  BitArray(const BitArray & array) throw(std::exception, std::bad_alloc)
    : current_size(array.current_size), array_of_bytes (array.array_of_bytes)
  { 
    // empty
  }

  void swap(BitArray & array) noexcept
  {
    std::swap(current_size, array.current_size);
    array_of_bytes.swap(array.array_of_bytes);
  }

  BitArray(BitArray && array) noexcept 
    : BitArray()
  {
    swap(array);
  }
		
  BitArray & operator = (BitArray && array) noexcept
  {
    current_size = 0;
    array_of_bytes.cut();
    swap(array);
    return *this;
  }

  /// Lo convierte a una lista
  DynList<char> bits_list() 
  {
    DynList<char> ret_val;
    for (size_t i = 0; i < current_size; ++i)
      ret_val.append((char) read_bit(i));
    return ret_val;
  }

      /** Asignación de arreglo de bits.

          Libera toda la memoria de this, aparta nueva memoria para
          albergar array y copia exactamente los bits de array a this.

          @param[in] array el arreglo de bits a ser copiado.
          @throw bad_alloc si no hay suficiente memoria para un bloque
          continuo de dim/8 bytes. 
          @note Si ocurre esta excepción, el estado de this permanece el
          mismo anterior a la asignación.
       */
  BitArray & operator = (const BitArray & array) 
    throw(std::exception, std::bad_alloc)
  {
    if (this == &array) 
      return *this;

    current_size = array.current_size;
    array_of_bytes = array.array_of_bytes;

    return *this;
  }

      /** Guarda en un archivo texto la secuencia de bits.

          save() almacena en el archivo texto output, ya abierto, la secuencia
          de bits en formato decimal.
     
          La primera línea siempre guarda el tamaño en bytes y en
          bits. Seguidamente se guardan en valor decimal los bytes secuenciales
          correspondientes al arreglo.

          @param[in] output stream de salida abierto donde se guardaría el
          arreglo. 
      */
  void save(std::ostream & output)
  {
    const size_t & num_bytes = array_of_bytes.size();

    output << num_bytes << " " << current_size << endl;

    for (size_t i = 0; i < num_bytes; i++)
      if (array_of_bytes.exist(i))
        {
          int byte = array_of_bytes.access(i).get_int();
          output << byte << " ";
        }
      else
        output << "0 ";

    output << endl;
  }

      /** Carga desde un archivo un arreglo de bits.

          load(input) toma el archivo abierto en input, previamente
          guardado con el método load(), y restaura el arreglo al valro
          guardado. 

          @param[in] input stream de entrada donde se encuentra fguarado
          el arreglo y que debe haber sido previamente guardado con
          save(). 
       */
  void load(std::istream & input)
  {
    array_of_bytes.cut();

    // TODO: bound num_bytes to a maximum according to available ram
    size_t num_bytes = 0;
    input >> num_bytes >> current_size;
    for (size_t i = 0; i < num_bytes; i++)
      {
        int c;
        input >> c;
        array_of_bytes.touch(i).set_int(c);;
      }
  }

      /// Construye un nuevo arreglo de bits a partir de un archivo
      /// construido mediante el método save().
  BitArray(std::ifstream & input)
  {
    load(input);
  }

      /** Guarda en un archivo texto una declaración estática de cadena de
          carateres que representa el arreglo.

          save_in_array_of_chars(name, output) genera una declaración del
          tipo:

          char name [ size ] = { valores del arreglo separados con comas };

          La declaración es guardada en el stream de salida output y el
          nombre del arreglo es name.

          @param[in] name nombre de la variable arreglo.
          @param[in] output stream de salida donde se desea guardar la
          declaración. 
       */
  void save_in_array_of_chars(const string & name, std::ostream & output)
  {
    const size_t & num_bytes = array_of_bytes.size();

    output << "// " << current_size << " bits declaration" << endl
           << "const unsigned char " << name << " [" << num_bytes << "] = {" 
           << endl << "  ";
    for (size_t i = 0; i < num_bytes; i++)
      {
        if (array_of_bytes.exist(i))
          {
            int byte = array_of_bytes.access(i).get_int();
            output << byte;
          }
        else
          output << "0";

        if (i != num_bytes - 1)
          output << ", ";

        if ((i+1) % 15 == 0)
          output << endl << "  ";
      }
    output << endl << "};" << endl << endl;
  }

      /** Lee un arreglo de bits guardado en un arreglo de caracteres.

          load_from_array_of_chars(str, num_bits) toma un arreglo str,
          previamente generado con save_in_array_of_chars, de num_bits de
          largo, y lo carga.

          @param[in] str el arreglo de caracteres donde se encuentra el de
          bits. 
          @param[in] num_bits la cantidad de bits del arreglo. Note que no
          es la misma que la cantidad de bytes.
       */
  void load_from_array_of_chars(const unsigned char str[], 
				const size_t        num_bits)
  {
    array_of_bytes.cut();

    size_t num_bytes = num_bits/8;

    if (num_bits % 8 != 0)
      num_bytes++;
    
    for (size_t i = 0; i < num_bytes; i++)
      array_of_bytes.touch(i).set_int(str[i]);

    current_size = num_bits;
  }

  /** Desplaza los bits n posiciones a la izquierda.

      Los bits son desplazados y se considera al arreglo de tamaño fijo,
      es decir, los n primeros bits se pierden.

      @param[in] n la cantidad de posiciones que se van a desplazar los bits.
  */
  void left_shift(const size_t n = 1) noexcept
  {
    const size_t real_n = std::min <size_t>(n, current_size);

    for (size_t i = 0; i < current_size - real_n; ++i)
      write_bit(i, read_bit(i + real_n));

    for (size_t i = current_size - real_n; i < current_size; ++i)
      write_bit(i, 0);
  }

  /** Desplaza los bits n posiciones a la derecha.

      Los bits son desplazados y se considera al arreglo de tamaño fijo,
      es decir, los últimos n bits se pierden.

      @param[in] n la cantidad de posiciones que se van a desplazar los bits.
  */
  void right_shift(const size_t n = 1) noexcept
  {
    const size_t real_n = std::min <size_t>(n, current_size);

    for (size_t i = current_size; i > real_n; --i)
      write_bit(i - 1, read_bit(i - real_n - 1));

    for (size_t i = real_n; i > 0; --i)
      write_bit(i - 1, 0);
  }

  /** Desplaza los bits n posiciones a la izquierda dinámicamente.

      Los bits son desplazados sin perderse ninguno y el arreglo crece
      agregando n ceros a la derecha.

      @param[in] n la cantidad de posiciones que se van a desplazar los bits.
  */
  void dyn_left_shift(const size_t n = 1)
  {
    for (size_t i = 0; i < n; ++i)
      push(0);
  }

  /** Desplaza los bits n posiciones a la derecha dinámicamente.

      Los bits son desplazados sin perderse ninguno y el arreglo decrece
      eliminando los n ceros que quedarían a la izquierda

      @param[in] n la cantidad de posiciones que se van a desplazar los bits.
  */
  void dyn_right_shift(const size_t n = 1)
  {
    if (n >= current_size)
      {
        set_size(1);
        array_of_bytes.set_default_initial_value(Byte());
        return;
      }

    BitArray array(current_size - n);

    for (size_t i = 0; i < current_size - n; ++i)
      array.write_bit(i, read_bit(i));

    *this = array;
  }

  /** Desplaza los bits n posiciones a la izquierda circularmente.

      Los bits son desplazados sin perderse ninguno, los n primeros bits
      pasan a ocupar las últimas n posiciones del arreglo.

      @param[in] n la cantidad de posiciones que se van a desplazar los bits.
  */
  void circular_left_shift(const size_t n = 1) noexcept
  {
    const size_t real_n = n % current_size;

    if (real_n == 0)
      return;

    BitArray array(real_n);

    for (size_t i = 0; i < real_n; ++i)
      array.write_bit(i, read_bit(i));

    for (size_t i = 0; i < current_size - real_n; ++i)
      write_bit(i, read_bit(i + real_n));

    for (size_t i = 0; i < real_n; ++i)
      write_bit(current_size - real_n + i, array.read_bit(i));
  }

  /** Desplaza los bits n posiciones a la derecha circularmente.

      Los bits son desplazados sin perderse ninguno, los últimos n bits
      pasan a ocupar las primeras n posiciones del arreglo.

      @param[in] n la cantidad de posiciones que se van a desplazar los bits.
  */
  void circular_right_shift(const size_t n = 1) noexcept
  {
    const size_t real_n = n % current_size;

    if (real_n == 0)
      return;

    BitArray array(real_n);

    for (size_t i = current_size - real_n; i < current_size; ++i)
      array.write_bit(i - (current_size - real_n), read_bit(i));

    for (size_t i = current_size - 1; i >= real_n; --i)
      write_bit(i, read_bit(i - real_n));

    for (size_t i = 0; i < real_n; ++i)
      write_bit(i, array.read_bit(i));
  }

  template <typename T>
  void set_num(T n) // Paso copia porque es modificado adentro
  {                                                    
    empty();
    const size_t num_bits = sizeof(T) * 8;
    reserve(num_bits);
    for (int i = 0; i < num_bits; ++i)
      {
        write_bit(current_size - i - 1, n & 1);
        n >>= 1;
      }
  }

  void set_num(const char & c)
  {
    set_num<char>(c);
  }

  void set_num(const short & c)
  {
    set_num<short>(c);
  }

  void set_num(const int & c)
  {
    set_num<int>(c);
  }

  void set_num(const long & c)
  {
    set_num<long>(c);
  }

  long get_num() noexcept
  {
    long ret_val = 0;
    for (size_t i = 0; i < current_size; ++i)
      ret_val += read_bit(current_size - i - 1) << i;
    return ret_val;                                   
  }

  void set_bit_str(const std::string & str)
  {
    empty();
    const size_t & str_size = str.size();
    
    reserve(str_size);

    for (size_t i = 0; i < str_size; ++i)
      {
        char c = str[i];
        assert(c == '1' or c == '0');
        write_bit(i, (c == '0' ? 0 : 1));
      }
  }

  std::string get_bit_str() const
  {
    std::string ret_val;
    for (size_t i = 0; i < current_size; ++i)
      ret_val.append(read_bit(i) == 0 ? "0" : "1");

    return ret_val;
  }

  friend ostream & operator << (ostream & out, const BitArray & array)
  {
    for (size_t i = 0; i < array.current_size; ++i)
      out << array.read_bit(i);
    return out;
  }

      /// Construye un nuevo arreglo de bits a partir de un arreglo de
      /// caracteres previamente generado con load_from_array_of_chars
  BitArray(const unsigned char str[], const size_t num_bits)
  {
    load_from_array_of_chars(str, num_bits);
  }

  BitArray & operator |= (const BitArray & rhs)
  {
    auto n = std::min(array_of_bytes.size(), rhs.array_of_bytes.size());
    for (size_t i = 0; i < n; ++i)
      array_of_bytes(i) |= rhs.array_of_bytes(i);

    if (size() < n)
      {
	auto i = array_of_bytes.size();
	set_size(rhs.size());
	for (; i < rhs.array_of_bytes.size(); ++i)
	  array_of_bytes(i) = rhs.array_of_bytes(i);
      }

    return *this;
  }

  BitArray & operator &= (const BitArray & rhs)
  {
    set_size(std::min(size(), rhs.size()));
    for (size_t i = 0; i < size(); ++i)
      array_of_bytes(i) &= rhs.array_of_bytes(i);

    return *this;
  }

  friend BitArray operator | (const BitArray & op1, const BitArray & op2)
  {
    BitArray ret = op1;
    ret |= op2;
    return ret;
  }

  friend BitArray operator & (const BitArray & op1, const BitArray & op2)
  {
    BitArray ret = op1;
    ret &= op2;
    return ret;
  }

private:

  template <class Operation>
  bool __traverse(Operation & operation) noexcept(noexcept(operation))
  {
    for (size_t i = 0; i < current_size; ++i)
      if (not operation(read_bit(i)))
	return false;
    return true;
  }

public:

  class Iterator
  {
    BitArray * array_ptr = nullptr;
    long curr_idx = 0;

  public:

    Iterator() noexcept { /* empty */ }

    Iterator(const BitArray & array) noexcept
    : array_ptr(const_cast<BitArray*>(&array)), curr_idx(0)
    {
      // empty
    }

    bool has_curr() const noexcept
    {
      return curr_idx >= 0 and curr_idx < array_ptr->size(); 
    }

    bool has_current() const noexcept { return has_curr(); }

    unsigned int get_curr_ne() const noexcept
    {
      return array_ptr->read_bit(curr_idx);
    }

    unsigned int get_curr() const
    {
      if (not has_curr())
	throw std::overflow_error("Iterator is at the end of the list");
      return array_ptr->read_bit(curr_idx); 
    }

    long get_pos() const noexcept { return curr_idx; }

    void next_ne() noexcept { ++curr_idx; }

    void next() 
    {
      if (curr_idx == array_ptr->size())
	throw std::overflow_error("not current item in iterator");
      next_ne();
    }

    void prev_ne() noexcept { --curr_idx; }
    void prev()
    {
      if (curr_idx == -1)
	throw std::underflow_error("not current item in iterator");
      prev_ne();
    }

    void reset_last() noexcept { curr_idx = array_ptr->size() - 1; }

    void end() noexcept { curr_idx = array_ptr->size(); }

    void reset_first() noexcept { curr_idx = 0; }

    void reset() noexcept { reset_first(); }
  };

  template <class Operation>
  bool traverse(Operation & operation) const noexcept(noexcept(operation))
  {
    return const_cast<BitArray&>(*this).__traverse(operation); 
  }

  template <class Operation>
  bool traverse(Operation & operation) noexcept(noexcept(operation))
  {
    return __traverse(operation); 
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
    noexcept(noexcept(operation))
  {
    return traverse<Operation>(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
    noexcept(noexcept(operation))
  {
    return traverse<Operation>(operation);
  }

  Functional_Methods(unsigned short);

  Generic_Items(unsigned char);

  STL_ALEPH_ITERATOR(BitArray);
};

} // end namespace Aleph
# endif /* BITARRAY_H */

