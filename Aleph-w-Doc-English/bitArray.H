
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file bitArray.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
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

  unsigned int read_bit(const unsigned int i) const
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
      default: throw std::out_of_range ("bit index greater than 7"); 
      }
  } 

  void write_bit(const unsigned int i, const unsigned int value)
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
      default: throw std::out_of_range ("bit index greater than 7"); 
      }
  } 

  Byte() : b0(0), b1(0), b2(0), b3(0), b4(0), b5(0), b6(0), b7(0) {}

  int get_int() const
  {
    const unsigned char * const ptr = (unsigned char*) this;
    return *ptr;
  }

  void set_int(int i)
  {
    unsigned char * ptr = (unsigned char*) this;
    *ptr = i;
  }
};

/** 
  @brief Arrangement adjacent bits.

  @details BitArray defines a contiguous array of bits. Consumption
  memory is proportional to the n / 8; where n is the number of bits
  that you want to manage.

  The array is dynamic , meaning that it can expand
  dynamically as needed.

  Access to the i- th bit can be performed by the operator
  [ ]. However , since this kind of access is
  implemented by a proxy class , access may fail
  Some expressions where the compiler fails to make the
  Suitable types of transformations.

  @ingroup Sequences
*/
class BitArray
{
  size_t         current_size;
  DynArray<Byte> array_of_bytes; 

  size_t get_num_bytes() const
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

    BitProxy(BitArray & a, const size_t i) :
      index(i), bit_index(i % 8), byte_index(i/8), array(&a)
    {
      if (array->array_of_bytes.exist(byte_index))
        byte_ptr = &array->array_of_bytes.access(byte_index);
      else
        byte_ptr = NULL;
    }
    
    operator int () const 
    {
      if (index >= array->current_size)
        throw std::out_of_range("Index out of range");

      return byte_ptr != NULL ? byte_ptr->read_bit(bit_index) : 0; 
    }

    BitProxy & operator = (const size_t value)
    {
      assert(value <= 1);

      if (byte_ptr == NULL)
        byte_ptr = &array->array_of_bytes.touch(byte_index);

      if (index >= array->current_size)
        array->current_size = index;

      byte_ptr->write_bit(bit_index, value);

      return *this;
    }

    BitProxy & operator = (const BitProxy & proxy)
    {
      if (byte_ptr == NULL)
        byte_ptr = &array->array_of_bytes.touch(byte_index);

      if (index >= array->current_size)
        array->current_size = index;

      byte_ptr->write_bit(bit_index, proxy.byte_ptr->read_bit(proxy.bit_index));

      return *this;
    }
  };

public:

/** 
  @brief Builder under dim bits.

  @param [in] Number of bit dim. The default is 256.

  @throw bad_alloc If there is not enough memory for a block
  continuous dim / 8 bytes      
*/
  BitArray(const size_t dim = 0) throw(std::exception, std::bad_alloc) 
    : current_size(dim), array_of_bytes(get_num_bytes())
  {
    array_of_bytes.set_default_initial_value(Byte());
  }
  
/** 
  @brief Memory Book in advance for the settlement of bits to the dimension dim.

  @details reserve ( ) Set aside in advance enough memory to store
  dim bits. Thus , access can write and read
  be directly and quickly using the methods read () and
  write ().

  @throw bad_alloc If there is insufficient memory
*/
  void reserve(size_t dim)
  {
    current_size = dim;

    int num_bytes = dim/8;
    if (dim % 8)
      ++num_bytes;
    
    array_of_bytes.reserve(num_bytes);
  }

/** 
  @brief Returns the dimension the arrengenment of the bits  
*/
  size_t size() const { return current_size; }

/** 
  @brief Reajust the dimension of the arrengenment  
*/
  void set_size(const size_t sz)
  {
    size_t array_size = sz / 8;
    if (array_size % 8)
      ++array_size;
    array_of_bytes.cut(array_size);
    current_size = sz;
  }

  BitProxy operator [] (const size_t i) const
  { 
    return BitProxy(const_cast<BitArray&>(*this), i);
  }

  BitProxy operator [] (const size_t i) { return BitProxy(*this, i); }

/** 
  @brief read the bit i.

  @param[in] i Index of bit to be read.
  
  @return the value of the bit i
  
  @note is not verified out of the rank.
*/
  int read_bit(const size_t i) const
  {
    if (i >= current_size)
      throw std::out_of_range("index out of range");

    const int bit_index = i % 8;
    const Byte byte = array_of_bytes[i/8];
    return byte.read_bit(bit_index);
  }

/** 
  @brief Write bit i with the value value

  @param [in] i Index bit to be written.
  @param [in] value Value to be written at position i.
   
  @note No checks out of range and verify that value
  have a binary value.
*/
  void write_bit(const size_t i, const unsigned int value) 
  {
    array_of_bytes.touch(i/8).write_bit(i%8, value);
    if (i >= current_size)
      current_size = i + 1;
  }

/** 
  @brief Fast reading bit i.

  @details Read unverified i bit memory

  @param [in] i Index bit to be read.

  @return The value of bit i

  @note No checks out of range.
*/
  int read(const size_t i) const
  {
    if (i >= current_size)
      throw std::out_of_range("index out of range");

    const int bit_index = i % 8;
    return array_of_bytes.access(i/8).read_bit(bit_index);
  }

/** 
  @brief Write bit i to the value memory value without verification.

  @param [in] i Index bit to be written.
  @param [in] value Value to be written at position i.

  @note No checks out of range and verify that value
  have a binary value.
* /
  void write(const size_t i, const unsigned int value) 
  {
    array_of_bytes.access(i/8).write_bit(i%8, value);
    if (i >= current_size)
      current_size = i + 1;
  }

/** 
  @brief Insert the end of the array the value. 
*/
  void push(const unsigned int value)
  {
    write_bit(current_size, value);
  }

/** 
  @brief Removes the last bit of the array. 
*/
  void pop()
  {
    current_size--;
    array_of_bytes.cut(get_num_bytes());
  }

/** 
  @brief Removes all bits inserted. 
*/
  void empty()
  {
    current_size = 0;
    array_of_bytes.cut();
  }

/** 
  @brief copy constructor.

  @details Instance an array of bit exact copy of array.

  @param [in] array arrangement of bits to be copied.

  @throw bad_alloc If there is not enough memory for a block
  continuous dim / 8 bytes      
*/
  BitArray(const BitArray & array) throw(std::exception, std::bad_alloc)
    : current_size(array.current_size), array_of_bytes (array.array_of_bytes)
  { 
    /* Empty */
  }

  void swap(BitArray & array)
  {
    std::swap(current_size, array.current_size);
    array_of_bytes.swap(array.array_of_bytes);
  }

  BitArray(BitArray && array) 
    : BitArray()
  {
    swap(array);
  }
		
  BitArray & operator = (BitArray && array)
  {
    current_size = 0;
    array_of_bytes.cut();
    swap(array);
    return *this;
  }

/**  
  @brief Converts it to a list 
*/
  DynList<char> bits_list() 
  {
    DynList<char> ret_val;
    for (size_t i = 0; i < current_size; ++i)
      ret_val.append((char) read_bit(i));
    return ret_val;
  }

/** 
  @brief Assigning bit arrangement.

  @details Unleash this memory , new memory away
  host array and copy exactly the bit array to esta.

  @param [in] array Arrangement of bits to be copied.
  
  @throw bad_alloc If there is not enough memory for a block
  continuous dim / 8 bytes.
  
  @note If this exception occurs, the state remains the esta
  same before the assignment.
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

/** 
  @brief Save a text file bit sequence.

  @details save () stores the output text file , and open the sequence
  bit in decimal format.
     
  The first line always keeps the size in bytes and bits.
  Then decimal value stored in sequential bytes
  for the arrangement.

  @param [in] Stream output open end where the settlement would be saved.
*/
  void save(std::ofstream & output)
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

/** 
  @brief Loading from a file an array of bits.

  @details load ( input) Take the file open in input previously
  saved with the load () method and restores under VALRO
  I saved.

  @param [in] input stream where input is fguarado
  arrangement and it must have been previously saved
  save (). 
*/
  void load(std::ifstream & input)
  {
    array_of_bytes.cut();

    size_t num_bytes = 0;
    input >> num_bytes >> current_size;
    for (size_t i = 0; i < num_bytes; i++)
      {
        int c;
        input >> c;
        array_of_bytes.touch(i).set_int(c);;
      }
  }

/** 
  @brief Constructs a new array of bits from a file
  built by the save () method.
*/
  BitArray(std::ifstream & input)
  {
    load(input);
  }

/** 
  @brief Save a text file a static statement string
  carateres representing the arrangement.

  @details save_in_array_of_chars (name , output)
  Generates a statement such as

  char name [size ] = { values ​​of the separate arrangement with commas } ;

  The statement is stored in the output stream output and
  array name is name.

  @param [in] name Name of the arrangement variable.
  @param [in] output Output Stream where you want to save the
  statement 
*/
  void save_in_array_of_chars(const string & name, std::ofstream & output)
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

/** 
  @brief Read an array of bits stored in an array of characters.

  @details load_from_array_of_chars (str, num_bits ) Take a str arrangement,
  previously generated with save_in_array_of_chars of num_bits of
  long, and loads.

  @param [in] str Settlement of characters where the bit.
  @param [in] num_bits Number of bits in the array. Note that no
  is the same as the number of bytes.
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

/** 
  @brief Shift the bits n positions to the left.

  @details bits are shifted and is considered the settlement of fixed size,
  that is, the first n bits are lost.

  @param [in] n Number of positions to be moved bits.
*/
  void left_shift(const size_t n = 1)
  {
    const size_t real_n = std::min <size_t>(n, current_size);

    for (size_t i = 0; i < current_size - real_n; ++i)
      write_bit(i, read_bit(i + real_n));

    for (size_t i = current_size - real_n; i < current_size; ++i)
      write_bit(i, 0);
  }

/** 
  @brief Shift the bits n positions to the right.

  @details bits are shifted and is considered the settlement of fixed size,
  that is, the last n bits are lost.

  @param [in] n Number of positions to be moved bits.
*/
  void right_shift(const size_t n = 1)
  {
    const size_t real_n = std::min <size_t>(n, current_size);

    for (size_t i = current_size; i > real_n; --i)
      write_bit(i - 1, read_bit(i - real_n - 1));

    for (size_t i = real_n; i > 0; --i)
      write_bit(i - 1, 0);
  }

/** 
  @brief Shift the bits n positions to the left dynamically.

  @details bits are shifted without missing any and arrangement grows
  n adding zeros to the right.

  @param [in] n Number of positions to be moved bits
*/
  void dyn_left_shift(const size_t n = 1)
  {
    for (size_t i = 0; i < n; ++i)
      push(0);
  }

/** 
  @brief Shift the bits n right positions dynamically.

  @details bits are shifted without missing any and arrangement decreases
  n eliminating zeros would be left

  @param [in] n Number of positions to be moved bits.
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

/** 
  @brief Shift the bits n positions to the left circularly.

  @details bits are shifted without missing any , the first n bits
  They come to occupy the last n positions of the array.

  @param [in] n Number of positions to be moved bits
*/
  void circular_left_shift(const size_t n = 1)
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

  /** 
    @brief Shift the bits n positions to the right circularly.

    @details bits are shifted without missing any , the last n bits
    They come to occupy the first n positions of the array.

    @param [in] n Number of positions to be moved bits.
  */
  void circular_right_shift(const size_t n = 1)
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
  void set_num(T n) /* A copy is passed because it is modified in */
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

  long get_num()
  {
    long ret_val = 0;
    for (int i = 0; i < current_size; ++i)
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

/** 
  @brief Constructs a new array of bits from an array of
  characters previously generated with load_from_array_of_chars 
*/
  BitArray(const unsigned char str[], const size_t num_bits)
  {
    load_from_array_of_chars(str, num_bits);
  }
};

} /* End namespace Aleph */

# endif /* BITARRAY_H */
