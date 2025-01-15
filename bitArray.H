
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

# ifndef BITARRAY_H
# define BITARRAY_H

# include <bit>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_dynArray.H>

using namespace std;

namespace Aleph
{

  class Byte
  {
    unsigned int b0: 1;
    unsigned int b1: 1;
    unsigned int b2: 1;
    unsigned int b3: 1;
    unsigned int b4: 1;
    unsigned int b5: 1;
    unsigned int b6: 1;
    unsigned int b7: 1;

   public:

    unsigned int read_bit(const unsigned int i) const noexcept
    {
      assert(i < 8);

      switch (i)
        {
          case 0 :
            return b0;
          case 1 :
            return b1;
          case 2 :
            return b2;
          case 3 :
            return b3;
          case 4 :
            return b4;
          case 5 :
            return b5;
          case 6 :
            return b6;
          case 7 :
            return b7;
          default:
            assert(false);
        }
      return 0; // never reached
    }

    void write_bit(const unsigned int i, const unsigned int value) noexcept
    {
      assert(i < 8);
      assert(value <= 1);

      switch (i)
        {
          case 0 :
            b0 = value;
          break;
          case 1 :
            b1 = value;
          break;
          case 2 :
            b2 = value;
          break;
          case 3 :
            b3 = value;
          break;
          case 4 :
            b4 = value;
          break;
          case 5 :
            b5 = value;
          break;
          case 6 :
            b6 = value;
          break;
          case 7 :
            b7 = value;
          break;
          default:
            assert(false);
        }
    }

    Byte() noexcept: b0(0), b1(0), b2(0), b3(0), b4(0), b5(0), b6(0), b7(0) {}

    int get_int() const noexcept
    {
      const unsigned char *const ptr = (unsigned char *) this;
      return *ptr;
    }

    void set_int(int i) noexcept
    {
      unsigned char *ptr = (unsigned char *) this;
      *ptr = i;
    }

    Byte &operator|=(const Byte &rhs)
    {
      (unsigned char &) *this |= (unsigned char &) rhs;
      return *this;
    }

    Byte &operator&=(const Byte &rhs)
    {
      (unsigned char &) *this &= (unsigned char &) rhs;
      return *this;
    }

    // int count_ones() const noexcept
    // {
    //   return popcount(*((unsigned char *) this));
    // }

    int count_ones() const noexcept
    {
      int count = 0;
      unsigned char byte = *((unsigned char *) this);
      while (byte)
        {
          count += byte & 1;
          byte >>= 1;
        }
      return count;
    }

    int count_zeros() const noexcept
    {
      return 8 - count_ones();
    }
  };

  /**Contiguous array of bits.

      BitArray defines a contiguous array of bits. The consumption of
      memory is proportional to n/8; where n is the number of bits
      that you want to manage.

      The array is dynamic, which means it can expand
      dynamically according to need.

      Access to the ith bit can be done using the operator
      []. However, since this kind of access is
      implemented using a proxy class, access may fail in
      some expressions where the compiler cannot carry out the
      appropriate type transformations.

      @ingroup Sequences
      @author Alejandro Mujica
      @author Leandro Rabindranath Leon
   */
  class BitArray
  {
    size_t current_size;
    DynArray<Byte> array_of_bytes;

    size_t get_num_bytes() const noexcept
    {
      size_t div = current_size / 8;

      return (current_size % 8) != 0 ? div + 1 : div;
    }

    class BitProxy
    {
      const size_t index;
      const size_t bit_index;
      const size_t byte_index;
      BitArray *array;
      Byte *byte_ptr;

     public:

      BitProxy(BitArray &a, const size_t i) noexcept
        : index(i), bit_index(i % 8), byte_index(i / 8), array(&a)
      {
        if (array->array_of_bytes.exist(byte_index))
          byte_ptr = &array->array_of_bytes.access(byte_index);
        else
          byte_ptr = nullptr;
      }

      operator int() const
      {
        if (index >= array->current_size)
          throw std::out_of_range("Index out of range");

        return byte_ptr != nullptr ? byte_ptr->read_bit(bit_index) : 0;
      }

      BitProxy &operator=(const size_t value)
      {
        assert(value <= 1);

        if (byte_ptr == nullptr)
          byte_ptr = &array->array_of_bytes.touch(byte_index);

        if (index >= array->current_size)
          array->current_size = index;

        byte_ptr->write_bit(bit_index, value);

        return *this;
      }

      BitProxy &operator=(const BitProxy &proxy)
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

    /** Bit array constructor.

        @param[in] dim number of bits. By default it is 256.
        @throw bad_alloc if there is not enough memory for a block
        continuous dim/8 bytes.
     */
    BitArray(const size_t dim = 0)
      : current_size(dim), array_of_bytes(get_num_bytes())
    {
      array_of_bytes.set_default_initial_value(Byte());
    }

    /// Build a BitArray of size dim with all bits set to value
    BitArray(const size_t dim, const unsigned int value)
      : BitArray(dim)
    {
      assert(value <= 1);
      for (size_t i = 0; i < dim; ++i)
        write_bit(i, value);
    }

    /** Reserve memory in advance for the bit array
        dim dimension

        reserve() sets aside enough memory in advance to save
        dim bits. In this way, write or read accesses can
        be done directly and more quickly using the read() and
        write().

        @throw bad_alloc if not enough memory
     */
    void reserve(size_t dim)
    {
      current_size = dim;

      int num_bytes = dim / 8;
      if (dim % 8)
        ++num_bytes;

      array_of_bytes.reserve(num_bytes);
    }

    /// Returns the dimension of the bit array
    size_t size() const noexcept { return current_size; }

    /// Resets the dimension of the array
    void set_size(const size_t sz)
    {
      size_t array_size = sz / 8;
      if (array_size % 8)
        ++array_size;

      array_of_bytes.adjust(array_size);
      current_size = sz;
    }

    BitProxy operator[](const size_t i) const noexcept
    {
      return BitProxy(const_cast<BitArray &>(*this), i);
    }

    BitProxy operator[](const size_t i) noexcept { return BitProxy(*this, i); }

    int read_bit_ne(const size_t i) const noexcept
    {
      const int bit_index = i % 8;
      auto ptr = array_of_bytes.test(i / 8);
      if (ptr)
        {
          const Byte &byte = *ptr;
          return byte.read_bit(bit_index);
        }
      else
        return 0;
    }

    /** Read bit i.

         @param[in] i index of the bit to be read.
         @return the value of bit i
         @note No checking out of range.
      */
    int read_bit(const size_t i) const
    {
      if (i >= current_size)
        throw std::out_of_range("index out of range");
      const int bit_index = i % 8;
      const Byte byte = array_of_bytes[i / 8];
      return byte.read_bit(bit_index);
    }

    int operator()(const size_t i) const { return read_bit(i); }

    /** Write bit i with the value value

        @param[in] i index of the bit to be written.
        @param[in] value value to be written at position i.
        @note It doesn't check out of range and it doesn't check that value
        have a binary value.
     */
    void write_bit(const size_t i, const unsigned int value)
    {
      array_of_bytes.touch(i / 8).write_bit(i % 8, value);
      if (i >= current_size)
        current_size = i + 1;
    }

    /** Quick read of bit i.

        Read bit i without checking memory

        @param[in] i index of the bit to be read.
        @return the value of bit i
        @note No checking out of range.
    */
    int read(const size_t i) const
    {
      if (i >= current_size)
        throw std::out_of_range("index out of range");

      const int bit_index = i % 8;
      return array_of_bytes.access(i / 8).read_bit(bit_index);
    }

    /** Writes bit i with value value without memory check.

        @param[in] i index of the bit to be written.
        @param[in] value value to be written at position i.
        @note It doesn't check out of range and it doesn't check that value
        have a binary value.
    */
    void write(const size_t i, const unsigned int value)
    {
      array_of_bytes.access(i / 8).write_bit(i % 8, value);
      if (i >= current_size)
        current_size = i + 1;
    }

    int fast_read(const size_t i) const noexcept
    {
      return array_of_bytes.access(i / 8).read_bit(i % 8);
    }

    void fast_write(const size_t i, const unsigned int value)
    {
      array_of_bytes.access(i / 8).write_bit(i % 8, value);
    }

    /// Inserts the value value at the end of the array.
    void push(const unsigned int value)
    {
      write_bit(current_size, value);
    }

    /// Removes the last bit of the array.
    void pop()
    {
      current_size--;
      array_of_bytes.cut(get_num_bytes());
    }

    /// Delete all inserted bits
    void empty() noexcept
    {
      current_size = 0;
      array_of_bytes.cut();
    }

    /** Copy constructor.

        Instantiates an array of bits exact copy of array.

        @param[in] array the array of bits to be copied.
        @throw bad_alloc if there is not enough memory for a block
        continuous dim/8 bytes.
    */
    BitArray(const BitArray &array)
      : current_size(array.current_size), array_of_bytes(array.array_of_bytes)
    {
      // empty
    }

    void swap(BitArray &array) noexcept
    {
      std::swap(current_size, array.current_size);
      array_of_bytes.swap(array.array_of_bytes);
    }

    BitArray(BitArray &&array) noexcept
      : BitArray()
    {
      swap(array);
    }

    BitArray &operator=(BitArray &&array) noexcept
    {
      current_size = 0;
      array_of_bytes.cut();
      swap(array);
      return *this;
    }

    /// Converts it to a list
    DynList<char> bits_list()
    {
      DynList<char> ret_val;
      for (size_t i = 0; i < current_size; ++i)
        ret_val.append((char) read_bit(i));
      return ret_val;
    }

    /** Bit array allocation.

         Release all memory of this, set aside new memory for
         host array and exactly copies the bits of array to this.

         @param[in] array the array of bits to be copied.
         @throw bad_alloc if there is not enough memory for a block
         continuous dim/8 bytes.
         @note If this exception occurs, the state of this remains the
         same prior to assignment.
      */
    BitArray &operator=(const BitArray &array)
    {
      if (this == &array)
        return *this;

      current_size = array.current_size;
      array_of_bytes = array.array_of_bytes;

      return *this;
    }

    /** Saves the bit sequence in a text file.

        save() stores the sequence in the output text file, already opened,
        of bits in decimal format.

        The first line always saves the size in bytes and in
        bits. The sequential bytes are then saved as a decimal value.
        corresponding to the arrangement.

        @param[in] output open output stream where the
        arrangement.
    */
    void save(std::ostream &output)
    {
      const size_t &num_bytes = array_of_bytes.size();

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

    /** Loads an array of bits from a file.

        load(input) takes the file opened in input, previously
        saved with the load() method, and restores the array to the value
        saved.

        @param[in] input stream where the
        arrangement and that must have been previously saved with
        save().
     */
    void load(std::istream &input)
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

    /// Build a new array of bits from a file
    /// constructed using the save() method.
    BitArray(std::ifstream &input)
    {
      load(input);
    }

    /**Saves a static string declaration to a text file.
        characters representing the arrangement.

        save_in_array_of_chars(name, output) generates a declaration of the
        guy:

        char name [ size ] = { comma-separated array values };

        The declaration is saved in the output stream and the
        array name is name.

        @param[in] name name of the array variable.
        @param[in] output output stream where you want to save the
        statement.
     */
    void save_in_array_of_chars(const string &name, std::ostream &output)
    {
      const size_t &num_bytes = array_of_bytes.size();

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

          if ((i + 1) % 15 == 0)
            output << endl << "  ";
        }
      output << endl << "};" << endl << endl;
    }

    /** Reads an array of bits saved in a character array.

         load_from_array_of_chars(str, num_bits) takes an array str,
         previously generated with save_in_array_of_chars, of num_bits
         long, and loads it.

         @param[in] str the character array where the of is located
         bits.
         @param[in] num_bits the number of bits in the array. Note that not
         is the same as the number of bytes.
      */
    void load_from_array_of_chars(const unsigned char str[],
                                  const size_t num_bits)
    {
      array_of_bytes.cut();

      size_t num_bytes = num_bits / 8;

      if (num_bits % 8 != 0)
        num_bytes++;

      for (size_t i = 0; i < num_bytes; i++)
        array_of_bytes.touch(i).set_int(str[i]);

      current_size = num_bits;
    }

    /** Shifts the bits n positions to the left.

        The bits are shifted and the array is considered a fixed size,
        that is, the first n bits are lost.

        @param[in] n the number of positions to shift the bits.
    */
    void left_shift(const size_t n = 1) noexcept
    {
      const size_t real_n = std::min<size_t>(n, current_size);

      for (size_t i = 0; i < current_size - real_n; ++i)
        write_bit(i, read_bit(i + real_n));

      for (size_t i = current_size - real_n; i < current_size; ++i)
        write_bit(i, 0);
    }

    /** Shifts the bits n positions to the right.

        The bits are shifted and the array is considered a fixed size,
        that is, the last n bits are lost.

        @param[in] n the number of positions to shift the bits.
    */
    void right_shift(const size_t n = 1) noexcept
    {
      const size_t real_n = std::min<size_t>(n, current_size);

      for (size_t i = current_size; i > real_n; --i)
        write_bit(i - 1, read_bit(i - real_n - 1));

      for (size_t i = real_n; i > 0; --i)
        write_bit(i - 1, 0);
    }

    /** Shifts bits n positions to the left dynamically.

         The bits are shifted without losing any and the array grows
         adding n zeros to the right.

         @param[in] n the number of positions to shift the bits.
     */
    void dyn_left_shift(const size_t n = 1)
    {
      for (size_t i = 0; i < n; ++i)
        push(0);
    }

    /** Shifts bits n positions to the right dynamically.

        The bits are shifted without losing any and the array is decremented.
        eliminating the n zeros that would be on the left

        @param[in] n the number of positions to shift the bits.
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

    /** Shifts the bits n positions to the left circularly.

        The bits are shifted without losing any, the first n bits
        They begin to occupy the last n positions in the arrangement.

        @param[in] n the number of positions to shift the bits.
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

    /** Shifts the bits n positions to the right circularly.

        The bits are shifted without losing any, the last n bits
        They come to occupy the first n positions in the arrangement.

        @param[in] n the number of positions to shift the bits.
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
    void set_num(T n) //Copy step because it is modified inside
    {
      empty();
      const size_t num_bits = sizeof(T) * 8;
      reserve(num_bits);
      for (size_t i = 0; i < num_bits; ++i)
        {
          write_bit(current_size - i - 1, n & 1);
          n >>= 1;
        }
    }

    void set_num(const char &c)
    {
      set_num<char>(c);
    }

    void set_num(const short &c)
    {
      set_num<short>(c);
    }

    void set_num(const int &c)
    {
      set_num<int>(c);
    }

    void set_num(const long &c)
    {
      set_num<long>(c);
    }

    long get_num() const noexcept
    {
      long ret_val = 0;
      for (size_t i = 0; i < current_size; ++i)
        ret_val += read_bit(current_size - i - 1) << i;
      return ret_val;
    }

    void set_bit_str(const std::string &str)
    {
      empty();
      const size_t &str_size = str.size();

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

    string to_string() const
    {
      return get_bit_str();
    }

    friend ostream &operator<<(ostream &out, const BitArray &array)
    {
      for (size_t i = 0; i < array.current_size; ++i)
        out << array.read_bit(i);
      return out;
    }

    /// Constructs a new array of bits from an array of
    /// characters previously generated with load_from_array_of_chars
    BitArray(const unsigned char str[], const size_t num_bits)
    {
      load_from_array_of_chars(str, num_bits);
    }

    BitArray &operator|=(const BitArray &rhs)
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

    BitArray &operator&=(const BitArray &rhs)
    {
      set_size(std::min(size(), rhs.size()));
      for (size_t i = 0; i < size(); ++i)
        array_of_bytes(i) &= rhs.array_of_bytes(i);

      return *this;
    }

    friend BitArray operator|(const BitArray &op1, const BitArray &op2)
    {
      BitArray ret = op1;
      ret |= op2;
      return ret;
    }

    friend BitArray operator&(const BitArray &op1, const BitArray &op2)
    {
      BitArray ret = op1;
      ret &= op2;
      return ret;
    }

    bool operator==(const BitArray &rhs) const
    {
      if (size() != rhs.size())
        return false;

      for (size_t i = 0; i < size(); ++i)
        if (read_bit(i) != rhs.read_bit(i))
          return false;

      return true;
    }

   private:

    template <class Operation>
    bool __traverse(Operation &operation)
    {
      for (size_t i = 0; i < current_size; ++i)
        if (not operation(read_bit(i)))
          return false;
      return true;
    }

   public:

    class Iterator
    {
      BitArray *array_ptr = nullptr;
      long curr_idx = 0;

     public:

      Iterator() noexcept { /* empty */ }

      Iterator(const BitArray &array) noexcept
        : array_ptr(const_cast<BitArray *>(&array)), curr_idx(0)
      {
        // empty
      }

      bool has_curr() const noexcept
      {
        return curr_idx >= 0 and curr_idx < array_ptr->size();
      }

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

    auto get_it() const noexcept { return Iterator(*this); }

    template <class Operation>
    bool traverse(Operation &operation) const
    {
      return const_cast<BitArray &>(*this).__traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation &operation)
    {
      return __traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation &&operation = Operation()) const
    {
      return traverse<Operation>(operation);
    }

    template <class Operation>
    bool traverse(Operation &&operation = Operation())
    {
      return traverse<Operation>(operation);
    }

    Functional_Methods(unsigned short);

    Generic_Items(unsigned char);

    STL_ALEPH_ITERATOR(BitArray);

    int count_ones() const noexcept
    {
      return foldl<int>(0, [](int acc, int x) { return acc + x; });
    }

    int count_zeros() const noexcept
    {
      return foldl<int>(0, [](int acc, int x) { return acc + (x == 0); });
    }
  };

} // end namespace Aleph
# endif /* BITARRAY_H */

