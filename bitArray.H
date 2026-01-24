/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file bitArray.H
 *  @brief Space-efficient bit array implementation.
 *
 *  This file provides a compact bit array that stores boolean values using
 *  only 1 bit per element, achieving 8x memory savings compared to using
 *  `bool` arrays.
 *
 *  ## Classes Provided
 *
 *  - **Byte**: Low-level byte manipulation utilities
 *  - **BitArray**: Dynamic bit array with automatic resizing
 *  - **BitProxy**: Proxy class for bit-level assignment via operator[]
 *
 *  ## Key Features
 *
 *  - Memory-efficient: 1 bit per boolean value
 *  - Dynamic resizing with configurable block size
 *  - Bitwise operations (AND, OR, XOR, NOT)
 *  - File I/O support (save/load)
 *  - STL-compatible iteration
 *  - Functional methods via CRTP mixin
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | read_bit | O(1) | O(1) |
 *  | write_bit | O(1) | O(1) |
 *  | resize | O(n) | O(n/8) |
 *  | AND/OR/XOR | O(n) | O(1) |
 *  | popcount | O(n) | O(1) |
 *
 *  ## Memory Layout
 *
 *  Bits are packed into bytes, with bit 0 of element i stored in
 *  the lowest bit of byte i/8, position i%8.
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  BitArray bits(100);  // 100 bits, all initialized to 0
 *
 *  bits[0] = 1;         // Set bit 0
 *  bits[50] = true;     // Set bit 50
 *
 *  if (bits[0])         // Check bit 0
 *    std::cout << "Bit 0 is set\n";
 *
 *  // Bitwise operations
 *  BitArray other(100);
 *  BitArray result = bits & other;  // AND
 *  result = bits | other;           // OR
 *  result = ~bits;                  // NOT
 *
 *  // Count set bits
 *  size_t count = bits.popcount();
 *  ```
 *
 *  @see bloom-filter.H Uses BitArray for Bloom filter implementation
 *  @see tpl_dynArray.H Underlying storage mechanism
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef BITARRAY_H
# define BITARRAY_H

# include <bit>
# include <cstdint>
# include <iostream>
# include <fstream>
# include <algorithm>
# include <type_traits>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <ah-errors.H>
# include <ahDry.H>
# include <ah-dry-mixin.H>

namespace Aleph
{
  class Byte
  {
    std::uint8_t value = 0;

  public:
    [[nodiscard]] unsigned int read_bit(const unsigned int i) const noexcept
    {
      assert(i < 8);
      return (value >> i) & 0x1;
    }

    void write_bit(const unsigned int i, const unsigned int val) noexcept
    {
      assert(i < 8);
      assert(val <= 1);
      const std::uint8_t mask = static_cast<std::uint8_t>(1) << i;
      const auto bit = static_cast<std::uint8_t>(val & 0x1u);
      this->value = (this->value & ~mask) | (bit << i);
    }

    Byte() noexcept = default;

    [[nodiscard]] int get_int() const noexcept
    {
      return static_cast<int>(value);
    }

    void set_int(int i) noexcept
    {
      value = static_cast<std::uint8_t>(i);
    }

    Byte &operator|=(const Byte & rhs) noexcept
    {
      value |= rhs.value;
      return *this;
    }

    Byte &operator&=(const Byte & rhs) noexcept
    {
      value &= rhs.value;
      return *this;
    }

    [[nodiscard]] int count_ones() const noexcept
    {
      return std::popcount(value);
    }

    [[nodiscard]] int count_zeros() const noexcept
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
  class BitArray : public FunctionalMixin<BitArray, unsigned int>
  {
    size_t current_size;
    DynArray<Byte> array_of_bytes;

    void clear_unused_bits_in_last_byte() noexcept
    {
      if (current_size == 0)
        return;

      const size_t used_bits_in_last_byte = current_size % 8;
      if (used_bits_in_last_byte == 0)
        return;

      const size_t last_byte_index = current_size / 8;
      Byte *last_byte = array_of_bytes.test(last_byte_index);
      if (last_byte == nullptr)
        return;

      const int mask = (1u << used_bits_in_last_byte) - 1u;
      last_byte->set_int(last_byte->get_int() & mask);
    }

    void ensure_num_bytes(const size_t num_bytes)
    {
      if (num_bytes == 0)
        {
          array_of_bytes.cut();
          return;
        }

      if (array_of_bytes.size() < num_bytes)
        array_of_bytes.touch(num_bytes - 1);
    }

    size_t get_num_bytes() const noexcept
    {
      const size_t div = current_size / 8;

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
      BitProxy(BitArray & a, const size_t i) noexcept
        : index(i), bit_index(i % 8), byte_index(i / 8), array(&a)
      {
        if (array->array_of_bytes.exist(byte_index))
          byte_ptr = &array->array_of_bytes.access(byte_index);
        else
          byte_ptr = nullptr;
      }

      operator int() const
      {
        ah_out_of_range_error_if(index >= array->current_size) << "Index out of range";

        return byte_ptr != nullptr ? byte_ptr->read_bit(bit_index) : 0;
      }

      BitProxy &operator=(const size_t value)
      {
        assert(value <= 1);

        if (byte_ptr == nullptr)
          byte_ptr = &array->array_of_bytes.touch(byte_index);

        const bool grew = index >= array->current_size;
        if (grew)
          array->current_size = index + 1;

        byte_ptr->write_bit(bit_index, value);

        if (grew)
          array->clear_unused_bits_in_last_byte();

        return *this;
      }

      BitProxy &operator=(const BitProxy & proxy)
      {
        if (byte_ptr == nullptr)
          byte_ptr = &array->array_of_bytes.touch(byte_index);

        const bool grew = index >= array->current_size;
        if (grew)
          array->current_size = index + 1;

        const int rhs_value = proxy;
        byte_ptr->write_bit(bit_index, rhs_value);

        if (grew)
          array->clear_unused_bits_in_last_byte();

        return *this;
      }
    };

  public:
    /// Type returned by Iterator::get_curr() - individual bits as unsigned int
    using Item_Type = unsigned int;

    /** Bit array constructor.

        @param[in] dim number of bits. By default, it is 256.
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
    void reserve(const size_t dim)
    {
      set_size(dim);
    }

    /// Returns the dimension of the bit array
    [[nodiscard]] constexpr size_t size() const noexcept { return current_size; }

    /// Resets the dimension of the array
    void set_size(const size_t sz)
    {
      const size_t old_size = current_size;
      const size_t array_size = (sz + 7) / 8;

      array_of_bytes.adjust(array_size); // allocate for fast read()/write()
      current_size = sz;

      if (sz > old_size && old_size != 0)
        if (const size_t rem = old_size % 8; rem != 0)
          {
            const size_t clear_end = std::min(sz, old_size + (8 - rem));
            for (size_t i = old_size; i < clear_end; ++i)
              write_bit(i, 0);
          }

      clear_unused_bits_in_last_byte();
    }

    int operator[](const size_t i) const { return read_bit(i); }

    BitProxy operator[](const size_t i) noexcept { return BitProxy(*this, i); }

    int read_bit_ne(const size_t i) const noexcept
    {
      const unsigned int bit_index = static_cast<unsigned int>(i % 8);
      if (const auto ptr = array_of_bytes.test(i / 8))
        {
          const Byte & byte = *ptr;
          return byte.read_bit(bit_index);
        }
      return 0;
    }

    /** Read bit i.

         @param[in] i index of the bit to be read.
         @return the value of bit i
         @note No checking out of range.
      */
    int read_bit(const size_t i) const
    {
      ah_out_of_range_error_if(i >= current_size) << "index out of range";
      return read_bit_ne(i);
    }

    int operator()(const size_t i) const { return read_bit(i); }

    /** Write bit i with the value

        @param[in] i index of the bit to be written.
        @param[in] value value to be written at position i.
        @note It doesn't check out of range, and it doesn't check that value
        have a binary value.
     */
    void write_bit(const size_t i, const unsigned int value)
    {
      ah_out_of_range_error_if(value > 1) << "BitArray::write_bit: value must be 0 or 1";
      array_of_bytes.touch(i / 8).write_bit(i % 8, value);
      if (i >= current_size)
        {
          current_size = i + 1;
          clear_unused_bits_in_last_byte();
        }
    }

    /** Quick read of bit i.

        Read bit i without checking memory

        @param[in] i index of the bit to be read.
        @return the value of bit i
        @note No checking out of range.
    */
    int read(const size_t i) const
    {
      ah_out_of_range_error_if(i >= current_size) << "index out of range";

      const int bit_index = i % 8;
      return array_of_bytes.access(i / 8).read_bit(bit_index);
    }

    /** Writes bit i with value without memory check.

        @param[in] i index of the bit to be written.
        @param[in] value value to be written at position i.
        @note It doesn't check out of range, and it doesn't check that value
        have a binary value.
    */
    void write(const size_t i, const unsigned int value)
    {
      ah_out_of_range_error_if(value > 1) << "BitArray::write: value must be 0 or 1";

      array_of_bytes.access(i / 8).write_bit(i % 8, value);
      if (i >= current_size)
        {
          current_size = i + 1;
          clear_unused_bits_in_last_byte();
        }
    }

    int fast_read(const size_t i) const noexcept
    {
      return array_of_bytes.access(i / 8).read_bit(i % 8);
    }

    void fast_write(const size_t i, const unsigned int value)
    {
      ah_out_of_range_error_if(value > 1) << "BitArray::fast_write: value must be 0 or 1";
      array_of_bytes.access(i / 8).write_bit(i % 8, value);
    }

    /// Inserts the value at the end of the array.
    void push(const unsigned int value)
    {
      write_bit(current_size, value);
    }

    /// Removes the last bit of the array.
    void pop()
    {
      ah_underflow_error_if(current_size == 0) << "BitArray::pop on empty array";
      current_size--;
      array_of_bytes.cut(get_num_bytes());
      clear_unused_bits_in_last_byte();
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
    BitArray(const BitArray & array)
      : current_size(array.current_size), array_of_bytes(array.array_of_bytes)
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

    BitArray &operator=(BitArray && array) noexcept
    {
      current_size = 0;
      array_of_bytes.cut();
      swap(array);
      return *this;
    }

    /// Converts it to a list
    DynList<char> bits_list() const
    {
      DynList<char> ret_val;
      for (size_t i = 0; i < current_size; ++i)
        ret_val.append(static_cast<char>(read_bit(i)));
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
    BitArray &operator=(const BitArray & array)
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
    void save(std::ostream & output) const
    {
      const size_t num_bytes = get_num_bytes();

      // Header
      output << num_bytes << " " << current_size << '\n';
      ah_runtime_error_if(not output) << "BitArray::save: write failed (header)";

      // Payload
      for (size_t i = 0; i < num_bytes; ++i)
        {
          int byte = 0;
          if (const Byte *p = array_of_bytes.test(i); p != nullptr)
            byte = p->get_int();

          // defend export invariant last masked byte
          if (num_bytes != 0 && i + 1 == num_bytes)
            if (const size_t used = current_size % 8; used != 0)
              {
                const int mask = (1u << used) - 1u;
                byte &= mask;
              }

          output << byte << " ";
          ah_runtime_error_if(not output)
            << "BitArray::save: write payload failed (byte " << i << ")";
        }

      output << '\n';
      ah_runtime_error_if(not output) << "BitArray::save: write failed (newline)";
    }

    /** Loads an array of bits from a file.

        load(input) takes the file opened in input, previously
        saved with the load() method, and restores the array to the value
        saved.

        @param[in] input stream where the
        arrangement and that must have been previously saved with
        save().
     */
    void load(std::istream & input)
    {
      // Read header first (without touching the current state until validated)
      size_t num_bytes = 0;
      size_t num_bits = 0;

      ah_runtime_error_if(not (input >> num_bytes >> num_bits)) << "BitArray::load: read failed (header)";

      const size_t expected = (num_bits + 7) / 8;
      ah_runtime_error_if(num_bytes != expected)
        << "BitArray::load: inconsistent header (num_bytes vs num_bits)";

      // Heavy Load: build temporary and then swap
      BitArray tmp;
      tmp.current_size = num_bits;
      tmp.array_of_bytes.cut();
      tmp.ensure_num_bytes(num_bytes);

      for (size_t i = 0; i < num_bytes; ++i)
        {
          long v = 0;
          ah_runtime_error_if(not (input >> v)) << "BitArray::load: read failed (payload)";
          ah_runtime_error_if(v < 0 or v > 255) << "BitArray::load: byte value out of range";

          tmp.array_of_bytes.touch(i).set_int(static_cast<int>(v));
        }

      tmp.clear_unused_bits_in_last_byte();
      swap(tmp);
    }

    /// Build a new array of bits from a file
    /// constructed using the save() method.
    BitArray(std::ifstream & input)
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
    void save_in_array_of_chars(const std::string & name, std::ostream & output) const
    {
      const size_t num_bytes = get_num_bytes();

      output << "// " << current_size << " bits declaration" << '\n'
          << "const unsigned char " << name << " [" << num_bytes << "] = {"
          << '\n' << "  ";

      for (size_t i = 0; i < num_bytes; i++)
        {
          int byte = 0;
          if (const Byte *p = array_of_bytes.test(i); p != nullptr)
            byte = p->get_int();

          if (num_bytes != 0 && i + 1 == num_bytes)
            if (const size_t used = current_size % 8; used != 0)
              {
                const int mask = (1u << used) - 1u;
                byte &= mask;
              }

          output << byte;

          if (i != num_bytes - 1)
            output << ", ";

          if ((i + 1) % 15 == 0)
            output << '\n' << "  ";
        }

      output << '\n' << "};" << '\n' << '\n';
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
      clear_unused_bits_in_last_byte();
    }

    /** Shifts the bits n positions to the left.

        The bits are shifted and the array is considered a fixed size,
        that is, the first n bits are lost.

        @param[in] n the number of positions to shift the bits.
    */
    void left_shift(const size_t n = 1)
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
    void right_shift(const size_t n = 1)
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
      if (current_size == 0)
        return;

      if (n >= current_size)
        {
          set_size(1);
          write_bit(0, 0);
          clear_unused_bits_in_last_byte();
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
    void circular_left_shift(const size_t n = 1)
    {
      if (current_size < 2)
        return;

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
    void circular_right_shift(const size_t n = 1)
    {
      if (current_size < 2)
        return;

      const size_t real_n = n % current_size;

      if (real_n == 0)
        return;

      BitArray array(real_n);

      for (size_t i = current_size - real_n; i < current_size; ++i)
        array.write_bit(i - (current_size - real_n), read_bit(i));

      for (size_t i = current_size; i-- > real_n;)
        write_bit(i, read_bit(i - real_n));

      for (size_t i = 0; i < real_n; ++i)
        write_bit(i, array.read_bit(i));
    }

    template <typename T>
    void set_num(T n) // Copy step because it is modified inside
    {
      using U = std::make_unsigned_t<T>;
      U u = static_cast<U>(n);
      empty();
      const size_t num_bits = sizeof(T) * 8;
      reserve(num_bits);
      for (size_t i = 0; i < num_bits; ++i)
        {
          write_bit(current_size - i - 1, u & 1u);
          u >>= 1;
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

    unsigned long get_unum() const noexcept
    {
      using UL = unsigned long;
      constexpr size_t max_bits = sizeof(UL) * 8;
      const size_t n = std::min(current_size, max_bits);

      UL ret_val = 0;
      for (size_t i = 0; i < n; ++i)
        ret_val |= static_cast<UL>(read_bit_ne(current_size - i - 1)) << i;
      return ret_val;
    }

    long get_num() const noexcept
    {
      return static_cast<long>(get_unum());
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

    std::string to_string() const
    {
      return get_bit_str();
    }

    friend std::ostream &operator<<(std::ostream & out, const BitArray & array)
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

    BitArray &operator|=(const BitArray & rhs)
    {
      if (rhs.size() > current_size)
        {
          current_size = rhs.size();
          ensure_num_bytes(get_num_bytes());
        }

      const size_t rhs_num_bytes = (rhs.size() + 7) / 8;
      const size_t rhs_used_bits_in_last_byte = rhs.size() % 8;
      for (size_t i = 0; i < rhs_num_bytes; ++i)
        {
          const Byte *rhs_byte = rhs.array_of_bytes.test(i);
          if (rhs_byte == nullptr)
            continue;

          Byte masked = *rhs_byte;
          if (rhs_used_bits_in_last_byte != 0 && i + 1 == rhs_num_bytes)
            {
              const int mask = (1u << rhs_used_bits_in_last_byte) - 1u;
              masked.set_int(masked.get_int() & mask);
            }

          if (masked.get_int() == 0)
            continue;

          array_of_bytes.touch(i) |= masked;
        }

      clear_unused_bits_in_last_byte();
      return *this;
    }

    BitArray &operator&=(const BitArray & rhs)
    {
      if (const size_t new_size = std::min(size(), rhs.size()); new_size != current_size)
        {
          current_size = new_size;
          array_of_bytes.cut(get_num_bytes());
        }

      const size_t num_bytes = get_num_bytes();
      for (size_t i = 0; i < num_bytes; ++i)
        {
          Byte *lhs_byte = array_of_bytes.test(i);
          if (lhs_byte == nullptr)
            continue;

          if (const Byte *rhs_byte = rhs.array_of_bytes.test(i); rhs_byte == nullptr)
            lhs_byte->set_int(0);
          else
            *lhs_byte &= *rhs_byte;
        }

      clear_unused_bits_in_last_byte();
      return *this;
    }

    friend BitArray operator|(const BitArray & op1, const BitArray & op2)
    {
      BitArray ret = op1;
      ret |= op2;
      return ret;
    }

    friend BitArray operator&(const BitArray & op1, const BitArray & op2)
    {
      BitArray ret = op1;
      ret &= op2;
      return ret;
    }

    bool operator==(const BitArray & rhs) const
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
    bool __traverse(Operation & operation)
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
      Iterator() = default;

      Iterator(const BitArray & array) noexcept
        : array_ptr(const_cast<BitArray *>(&array)), curr_idx(0)
      {
        // empty
      }

      [[nodiscard]] bool has_curr() const noexcept
      {
        return array_ptr != nullptr and curr_idx >= 0 and
               static_cast<size_t>(curr_idx) < array_ptr->size();
      }

      [[nodiscard]] unsigned int get_curr_ne() const noexcept
      {
        return has_curr() ? array_ptr->read_bit_ne(static_cast<size_t>(curr_idx)) : 0;
      }

      [[nodiscard]] unsigned int get_curr() const
      {
        ah_overflow_error_if(not has_curr())
          << "Iterator is at the end of the list";
        return array_ptr->read_bit(static_cast<size_t>(curr_idx));
      }

      [[nodiscard]] long get_pos() const noexcept { return curr_idx; }

      void next_ne() noexcept { ++curr_idx; }

      void next()
      {
        ah_overflow_error_if(array_ptr == nullptr) << "Iterator is not bound to any BitArray";
        ah_overflow_error_if(curr_idx == static_cast<long>(array_ptr->size()))
          << "not current item in iterator";
        next_ne();
      }

      void prev_ne() noexcept { --curr_idx; }

      void prev()
      {
        ah_underflow_error_if(array_ptr == nullptr) << "Iterator is not bound to any BitArray";
        ah_underflow_error_if(curr_idx == -1) << "not current item in iterator";
        prev_ne();
      }

      void reset_last() noexcept
      {
        if (array_ptr == nullptr or array_ptr->size() == 0)
          curr_idx = -1;
        else
          curr_idx = static_cast<long>(array_ptr->size()) - 1;
      }

      void end() noexcept
      {
        if (array_ptr == nullptr)
          {
            curr_idx = 0;
            return;
          }
        curr_idx = static_cast<long>(array_ptr->size());
      }

      void reset_first() noexcept { curr_idx = 0; }

      void reset() noexcept { reset_first(); }
    };

    auto get_it() const noexcept { return Iterator(*this); }

    template <class Operation>
    bool traverse(Operation & operation) const
    {
      return const_cast<BitArray &>(*this).__traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation & operation)
    {
      return __traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation && operation = Operation()) const
    {
      return traverse<Operation>(operation);
    }

    template <class Operation>
    bool traverse(Operation && operation = Operation())
    {
      return traverse<Operation>(operation);
    }

    // Functional methods provided by FunctionalMixin<BitArray, unsigned int>

    Generic_Items(unsigned int);

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
