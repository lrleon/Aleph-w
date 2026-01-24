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


/** @file tpl_dynArray.H
 *  @brief Lazy and scalable dynamic array implementation.
 *
 *  This file provides `DynArray`, a versatile dynamic array with lazy
 *  allocation. Memory is allocated on-demand using a three-level indexing
 *  scheme (directory, segments, blocks) that provides O(1) access time
 *  while minimizing memory waste for sparse arrays.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNARRAY_H
# define TPL_DYNARRAY_H

# include <cmath>

# include <string>
# include <vector>
# include <aleph.H>
# include <ahIterator.H>
# include <array_utils.H>
# include <ahDry.H>
# include <tpl_dynDlist.H>
# include <ah-args-ctor.H>
# include <htlist.H>
# include <ah-dry.H>
# include <ah-errors.H>
# include <ah-errors.H>

namespace Aleph
{
  class BitArray; // forward needed

  /** Lazy and scalable dynamic array
      ===============================

      This class implements a very versatile dynamic array which would
      represent a good trade-off between fast and constant access time
      and memory consumption. The array is lazy in the sense that the
      dimension grows dynamically and the required memory for a cell
      could be allocated at the first writing.

      The data structure consists of three array types. A first array
      type is called block, and it is a contiguous chunk of `block_size`
      data entries. if an array occupies n entries then there are
      n/block_size + 1 blocks allocated. The blocks are indexed by
      segments of `seg_size` blocks which in turn are indexed by a
      directory of `dir_size` segment. For accessing to entry i the
      following calculations are done:

      1. The directory entry corresponds to i/(seg_size*block_size).
      2. The modulus of previous operation is divided between
         `block_size`. This gives the index in the segment.
      3. Finally, the modulus of previous operation gives the index of
        i-th in the block.

     So, there are always four operations for each access what gives a
     constant access time.

     In order to faster perform the calculations, the directory,
     segment and block size are adjusted to two powers. In this way,
     the division and modulus can be done with shifting and masking.

     Access through [] operator
     --------------------------

     For lazy writing, that is to allocate the block just when it is
     certain that this will be used, the operator `[]` is used. This
     operator is able to determine whether the i-th entry has been o
     not written. If for example a reading on a non allocated block is
     done, some such as:

         std::cout << a[i] << end;

     Then a default initialization value will be returned. This value
     corresponds to the resulting of default constructor call.

     At the contrary, if the first time that a writing is done, some such as:

         a[i] = value;

     Then the block, and eventually the segment will be allocated.

     Eventually the array could be fragmented and to consume memory some
     proportional to the writes done. Suppose for example that you only
     perform:

         a[0] = value;
         a[100000000] = value;

     In this case, the registered dimension of array will be 100000000,
     but only two blocks will be allocated. If you only performs reads
     between \f$(0, 100000000)\f$ always it will be returned the default
     value and the majority of times neither the segment nor the block
     will be read, since they have not been written.

     Access through [] operator perform bound_min_clock checks and it requires to
     test if the segment and/or block have been allocated and eventually
     to allocate them.

     Faster access though [] operator
     --------------------------------

     If you know (you are absolutely sure of) that the entry has already
     been written, and consequently it has already its block allocated,
     then you could use the operator () for direct access.

     Access with () operator does not perform any check (bounds and
     blocks and segment existence). This way is faster but unsure.

     () operator is an alias of `access(i)` method.

     In order to assure that a range of entries is already allocated,
     you could use `reserve(l, r)` method, which test and eventually
     allocates the needed memory for the entries comprised in the range
     \f$[l,r]\f$.

     Managing a dynamic array as a list
     ----------------------------------

     From a functional perspective, an dynamic array could be treated as
     a list.

     Initially, the array is empty.

     A new item can be inserted with `append(item`. This method copy (or
     moves) the item to the next available entry (at the end). The
     dimension is increased in one unit.

     You can also to treat the dynamic array as stack or queue.

     Setting the directory, segment and block sizes
     ----------------------------------------------

     The interface offer a default constructor which normally sets the
     sizes and serves for almost all situations.

     However, sometimes is desirable to set oneself these parameter. The
     principal justification is given for situations where the memory is
     very limited.

     Note that a memory manager could have enough memory dispersed
     through the total of available chunks, but it could not exist a
     contiguous block of a given size. The more large is the block size,
     the higher is the probability for failing to allocate. So, there
     are circumstances where you could be interested in setting a small
     block size, to facilitate the block's allocation.

     Of course, these settings restrict the maximum dimension, which
     eventually could be composed with a larger directory. As example,
     consider a directory of 1024 segments of 512 blocks of 4096
     entries. This gives a total of

     \f$1024 \times 512 \times 4096 = 2147483648\f$ entries.

     Now suppose that you think that 4096 is too much for a block, then
     you could set the block to 128 and displace the difference to the
     directory. This gives sizes of 32768, 512 and 128 respectively and
     thus to obtain the same maximum capacity.

     You could think that failure probability is displaced to the
     directory allocation, but the directory is allocated once at
     construction time.

     @ingroup Sequences
   */
  template <typename T>
  class DynArray : public LocateFunctions<DynArray<T>, T>,
                   public FunctionalMethods<DynArray<T>, T>,
                   public GenericKeys<DynArray<T>, T>,
                   public EqualToMethod<DynArray<T>>,
                   public StlAlephIterator<DynArray<T>>
  {
    friend class BitArray; // for access to __traversal() method

    // look at the end of this file for seeing the values
  public:
    using Item_Type = T; /// The type of element stored in the array
    using Key_Type = T; /// The type of element stored in the array

    static const size_t Default_Pow_Dir; /// Default two power for directory size
    static const size_t Default_Pow_Seg; /// Default two power for segment size
    static const size_t Default_Pow_Block; /// Default two power for block size

  private:
    static const size_t Max_Bits_Allowed;

  public:
    /// Maximum dimension allowed
    static const unsigned long long Max_Dim_Allowed;

  private:
    static const size_t Max_Pow_Block;

    mutable size_t pow_dir = Default_Pow_Dir;
    mutable size_t pow_seg = Default_Pow_Seg;
    mutable size_t pow_block =
        std::min(Default_Pow_Block, Max_Pow_Block);
    mutable size_t seg_plus_block_pow = pow_seg + pow_block;
    mutable size_t mask_seg_plus_block = two_raised(seg_plus_block_pow) - 1;
    mutable size_t dir_size = two_raised(pow_dir); // = 2^pow_dir
    mutable size_t seg_size = two_raised(pow_seg); // = 2^pow_seg
    mutable size_t block_size = two_raised(pow_block); // = 2^pow_block

    // 2^(pow_dir + pow_seg + pow_block) - 1
    unsigned long long max_dim = two_raised(seg_plus_block_pow + pow_dir);

    static size_t two_raised(const size_t n) noexcept
    {
      ah_overflow_error_if(n >= Max_Bits_Allowed)
        << "two_raised: exponent " << n << " is too large";
      return static_cast<size_t>(1) << n;
    }

    static size_t compute_dim(size_t d, size_t s, size_t b) noexcept
    {
      return two_raised(d) * two_raised(s) * two_raised(b);
    }

  public:
    /** Given a dimension `n`, it proposes values for the directory,
        segment and block sizes.

        @param[in] n proposed dimension
        @param[out] d directory size
        @param[out] s segment size
        @param[out] b block size
    */
    static void compute_sizes(const size_t n, size_t & d, size_t & s, size_t & b)
      noexcept
    {
      d = Default_Pow_Dir;
      s = Default_Pow_Seg;
      b = Default_Pow_Block;
      if (compute_dim(d, s, b) >= n)
        return;

      while (true)
        {
          if (compute_dim(++d, s, b) >= n)
            break;

          if (compute_dim(d, ++s, b) >= n)
            break;

          if (compute_dim(d, s, ++b) >= n)
            break;
        }
    }

    /** Given a dimension `n`, it proposes values for the directory,
        segment and block sizes.

        @param[in] n proposed dimension
        @return a 3-tuple with the directory, segment and block sizes
    */
    static std::tuple<size_t, size_t, size_t> compute_sizes(const size_t n)
      noexcept
    {
      size_t d, s, b;
      compute_sizes(n, d, s, b);
      return std::make_tuple(d, s, b);
    }

  private:
    size_t mask_seg = seg_size - 1;
    size_t mask_block = block_size - 1;

    size_t index_in_dir(const size_t i) const noexcept
    {
      assert((pow_block + pow_seg) == seg_plus_block_pow);
      assert(seg_size * block_size == two_raised(seg_plus_block_pow));
      assert(i >> seg_plus_block_pow == i/(seg_size*block_size));

      return i >> seg_plus_block_pow;
    }

    size_t modulus_from_index_in_dir(const size_t i) const noexcept
    {
      assert(mask_seg_plus_block == seg_size*block_size - 1);
      assert((i & mask_seg_plus_block) == i%(seg_size*block_size));

      return (i & mask_seg_plus_block);
    }

    size_t index_in_seg(const size_t & i) const noexcept
    {
      assert(two_raised(pow_block) == block_size);
      assert((modulus_from_index_in_dir(i) >> pow_block) ==
             (i%(seg_size*block_size))/block_size);

      return modulus_from_index_in_dir(i) >> pow_block;
    }

    size_t index_in_block(const size_t i) const noexcept
    {
      assert(mask_block == block_size - 1);
      assert((modulus_from_index_in_dir(i) & mask_block) ==
             ((i%(seg_size*block_size))%block_size));

      return modulus_from_index_in_dir(i) & mask_block;
    }

    size_t current_dim;
    size_t num_segs = 0;
    size_t num_blocks = 0;
    T ***dir = nullptr;

    void fill_dir_to_null() noexcept
    {
      assert(dir != nullptr);

      for (size_t i = 0; i < dir_size; ++i)
        dir[i] = nullptr;
    }

    void fill_seg_to_null(T **seg) noexcept
    {
      assert(seg != nullptr);

      for (size_t i = 0; i < seg_size; ++i)
        seg[i] = nullptr;
    }

    void allocate_dir()
    {
      dir = static_cast<T ***>(malloc(dir_size * sizeof(T **)));
      ah_bad_alloc_unless(dir != nullptr);

      fill_dir_to_null();
    }

    void resize_dir(const size_t i) // resize dir to fit index i
    {
      assert(i >= max_dim);

      size_t new_pow_dir = pow_dir + 1;
      while (compute_dim(new_pow_dir, pow_seg, pow_block) <= i)
        ++new_pow_dir;

      const size_t new_dir_sz = two_raised(new_pow_dir);
      T ***new_dir = static_cast<T ***>(realloc(dir, new_dir_sz * sizeof(T **)));
      ah_bad_alloc_unless(new_dir != nullptr);

      dir = new_dir;
      for (size_t k = dir_size; k < new_dir_sz; ++k)
        dir[k] = nullptr;

      pow_dir = new_pow_dir;
      dir_size = new_dir_sz;

      max_dim = compute_dim(pow_dir, pow_seg, pow_block);
    }

    void allocate_segment(T **& seg)
    {
      assert(seg == nullptr);

      seg = new T *[seg_size];
      fill_seg_to_null(seg);
      ++num_segs;
    }

    T default_initial_value = T();
    T *default_initial_value_ptr = &default_initial_value;

    void allocate_block(T *& block)
    {
      assert(block == nullptr);

      block = new T [block_size];
      ++num_blocks;

      if (default_initial_value_ptr == nullptr)
        return;

      for (size_t i = 0; i < block_size; ++i)
        block[i] = *default_initial_value_ptr;
    }

    void release_segment(T **& seg) noexcept
    {
      assert(seg != nullptr);

      delete [] seg;
      seg = nullptr;
      --num_segs;
    }

    void release_block(T *& block) noexcept
    {
      assert(block != nullptr);

      delete [] block;
      block = nullptr;
      --num_blocks;
    }

    void release_blocks_and_segment(T ** & seg) noexcept
    {
      assert(seg != nullptr);

      for (size_t i = 0; i < seg_size; ++i)
        if (seg[i] != nullptr)
          release_block(seg[i]);

      release_segment(seg);
    }

    void ensure_not_empty(const char *context) const
    {
      ah_underflow_error_if(is_empty()) << context;
    }

    void release_all_segments_and_blocks() noexcept
    {
      assert(dir != nullptr);

      for (size_t i = 0; i < dir_size; ++i)
        if (dir[i] != nullptr)
          release_blocks_and_segment(dir[i]);

      current_dim = 0;
    }

    void release_dir() noexcept
    {
      if (dir == nullptr)
        return;

      release_all_segments_and_blocks();
      if (dir != nullptr)
        free(dir);

      dir = nullptr;
      current_dim = 0;
    }

    static size_t next2Pow(const size_t number) noexcept
    {
      return static_cast<size_t>(ceil(log(static_cast<float>(number)) / log(2.0)));
    }

    size_t divide_by_block_size(const size_t number) const noexcept
    {
      assert(number/block_size == number >> pow_block);

      return number >> pow_block;
    }

    size_t modulus_by_block_size(const size_t number) const noexcept
    {
      assert((number%block_size) == (number & mask_block));

      return number & mask_block;
    }

    void advance_block_index(size_t block_index, size_t seg_index,
                             const size_t len) const noexcept
    {
      if (block_index + len < block_size)
        {
          block_index += len;
          return;
        }

      seg_index += divide_by_block_size(len);
      block_index = modulus_by_block_size(len);
    }

    void allocate_block(T *& block, T *src_block)
    {
      allocate_block(block);
      for (size_t i = 0; i < block_size; i++)
        block[i] = src_block[i];
    }

    void allocate_segment(T **& seg, T **src_seg)
    {
      allocate_segment(seg);
      for (size_t i = 0; i < seg_size; i++)
        if (src_seg[i] != nullptr)
          allocate_block(seg[i], src_seg[i]);
    }

    void allocate_dir(T ***src_dir)
    {
      allocate_dir();
      for (size_t i = 0; i < dir_size; i++)
        if (src_dir[i] != nullptr)
          allocate_segment(dir[i], src_dir[i]);
    }

    class Proxy
    {
      size_t index;
      size_t pos_in_dir;
      size_t pos_in_seg;
      size_t pos_in_block;
      T **& ref_seg;
      T *block;
      DynArray & array;

    public:
      Proxy(DynArray<T> & _array, const size_t i) noexcept
        : index(i), pos_in_dir(_array.index_in_dir(index)),
          pos_in_seg(_array.index_in_seg(index)),
          pos_in_block(_array.index_in_block(index)),
          ref_seg(_array.dir[pos_in_dir]), block(nullptr), array(_array)
      {
        if (ref_seg != nullptr)
          block = ref_seg[pos_in_seg]; // Entry block already exists
      }

      operator T &()
      {
        ah_invalid_argument_if(block == nullptr) << "accessed entry not been still written";
        return block[pos_in_block];
      }

      T * operator ->()
      {
        bool seg_was_allocated_in_current_call = false;

        if (ref_seg == nullptr) // Is there a segment?
          { // No ==> allocate it!
            array.allocate_segment(ref_seg);
            seg_was_allocated_in_current_call = true;
          }

        if (block == nullptr) // test if block is allocated
          {
            try
              {
                array.allocate_block(block);
                ref_seg[pos_in_seg] = block;

                assert(array.dir[pos_in_dir] == ref_seg);
              }
            catch (...)
              {
                if (seg_was_allocated_in_current_call)
                  array.release_segment(ref_seg);

                throw;
              }
          }

        if (index >= array.current_dim)
          array.current_dim = index + 1;

        return &block[pos_in_block];
      }

      Proxy &operator =(const T & data)
      {
        bool seg_was_allocated_in_current_call = false;
        if (ref_seg == nullptr) // Is there a segment?
          { // No ==> allocate ii!
            array.allocate_segment(ref_seg);
            seg_was_allocated_in_current_call = true;
          }

        if (block == nullptr) // test if block is allocated
          {
            try
              {
                array.allocate_block(block);
                ref_seg[pos_in_seg] = block;

                assert(array.dir[pos_in_dir] == ref_seg);
              }
            catch (...)
              {
                if (seg_was_allocated_in_current_call)
                  array.release_segment(ref_seg);

                throw;
              }
          }

        if (index >= array.current_dim)
          array.current_dim = index + 1;

        block[pos_in_block] = data;

        return *this;
      }

      Proxy &operator =(const Proxy & proxy)
      {
        ah_domain_error_if(proxy.block == nullptr) << "right entry has not been still written";

        if (&proxy == this)
          return *this;

        bool seg_was_allocated_in_current_call = false;
        if (ref_seg == nullptr) // Is there a segment?
          { // No ==> allocate it!
            array.allocate_segment(ref_seg);
            seg_was_allocated_in_current_call = true;
          }

        if (block == nullptr) // test if block is allocated
          {
            try
              {
                array.allocate_block(block);
                ref_seg[pos_in_seg] = block;

                assert(array.dir[pos_in_dir] == ref_seg);
              }
            catch (...)
              {
                if (seg_was_allocated_in_current_call)
                  array.release_segment(ref_seg);

                throw;
              }
          }

        if (index >= array.current_dim)
          array.current_dim = index + 1;

        block[pos_in_block] = proxy.block[proxy.pos_in_block];

        return *this;
      }
    };

  public:
    /// Return the directory size
    size_t get_dir_size() const noexcept { return dir_size; }

    /// Return the segment size
    size_t get_seg_size() const noexcept { return seg_size; }

    /// Return the block size
    size_t get_block_size() const noexcept { return block_size; }

    /// Return the current dimension of array. According to usage style,
    /// this could represent the number of items stored in the array seen
    /// as a container
    size_t size() const noexcept { return current_dim; }

    /// Return the maximum allowed dimension (or the maximum number of
    /// elements that could have the array treated as a container). Be
    /// careful with the fact that **this bound_min_clock is not related to
    /// available memory**
    size_t max_size() const noexcept { return max_dim; }

    /// Return the number of blocks consumed by the array
    size_t get_num_blocks() const noexcept { return num_blocks; }

    /** Set the default value.

        The default value of a dynamic array is the value to be
        returned when entries not still written are accessed.

        @param[in] value default value
     */
    void set_default_initial_value(const T & value) noexcept
    {
      default_initial_value = value;
      default_initial_value_ptr = &default_initial_value;
    }

    /// \overload set_default_initial_value(const T & value)
    void set_default_initial_value(T && value = T())
    {
      std::swap(default_initial_value, value);
      default_initial_value_ptr = &default_initial_value;
    }

    /** Construct a dynamic array given directory, segment and block
        sizes.

        @param[in] _pow_dir two power for directory size
        @param[in] _pow_seg two power for segment size
        @param[in] _pow_block two power for block size

        @throw bad_alloc if there is no enough memory

        @throw length_error if the given sizes exceed the maximum
        possible dimension

        @throw overflow_error if it happens an arithmetic overflow with
        the bits operations
    */
    DynArray(const size_t _pow_dir, const size_t _pow_seg, const size_t _pow_block)
      : pow_dir(_pow_dir),
        pow_seg(_pow_seg),
        pow_block(_pow_block),
        seg_plus_block_pow(_pow_seg + _pow_block),
        mask_seg_plus_block(two_raised(seg_plus_block_pow) - 1),
        dir_size(two_raised(pow_dir)),
        seg_size(two_raised(pow_seg)),
        block_size(two_raised(pow_block)),
        max_dim(two_raised(seg_plus_block_pow + pow_dir)),
        mask_seg(seg_size - 1),
        mask_block(block_size - 1),
        current_dim(0),
        num_segs(0),
        num_blocks(0)
    {
      static_assert(std::is_copy_constructible_v<T>, "No copy constructor for T");
      static_assert(std::is_move_constructible_v<T>, "No move constructor for T");
      static_assert(std::is_copy_assignable_v<T>, "No copy assign for T");
      static_assert(std::is_move_assignable_v<T>, "No move assign for T");
      assert(Max_Dim_Allowed > 0);

      ah_length_error_if(max_dim > Max_Dim_Allowed) << "Dimension too large";

      allocate_dir();
    }

    /** Default constructor

        @param[in] dim initial dimension of array
        @throw bad_alloc if there is no enough memory
        @throw length_error si dim is greater than maximum allowed
        @throw overflow_error if it happens an arithmetic overflow with
        the bits operations
    */
    DynArray(const size_t dim = 0)
      : current_dim(dim)
    {
      static_assert(std::is_default_constructible_v<T>, "No default constructor for T");
      static_assert(std::is_copy_constructible_v<T>, "No copy constructor for T");
      static_assert(std::is_move_constructible_v<T>, "No move constructor for T");
      static_assert(std::is_copy_assignable_v<T>, "No copy assign for T");
      static_assert(std::is_move_assignable_v<T>, "No move assign for T");
      assert(Max_Dim_Allowed > 0);

      ah_length_error_if(max_dim > Max_Dim_Allowed) << "Dimension too large";

      allocate_dir();
    }

    Special_Ctors(DynArray, T);

    ~DynArray()
    {
      release_dir();
    }

    /** Copy the items of `src_array` to `this`

        @param[in] src_array source array
    */
    void copy_array(const DynArray<T> & src_array)
    {
      for (size_t i = 0; i < src_array.current_dim; ++i)
        if (src_array.exist(i))
          (*this)[i] = src_array.access(i);
    }

    /** Copy constructor

        @param[in] array source of copy
        @throw bad_alloc if there is no enough memory
    */
    DynArray(const DynArray<T> & array)
      : pow_dir(array.pow_dir),
        pow_seg(array.pow_seg),
        pow_block(array.pow_block),
        seg_plus_block_pow(array.seg_plus_block_pow),
        mask_seg_plus_block(array.mask_seg_plus_block),
        dir_size(array.dir_size),
        seg_size(array.seg_size),
        block_size(array.block_size),
        max_dim(array.max_dim),
        mask_seg(array.mask_seg),
        mask_block(array.mask_block),
        current_dim(0),
        num_segs(0),
        num_blocks(0),
        dir(nullptr),
        default_initial_value(array.default_initial_value),
        default_initial_value_ptr(array.default_initial_value_ptr)
    {
      allocate_dir(array.dir);
      copy_array(array);
    }

    /** Copy assignment

        @param[in] array source of copy
        @throw bad_alloc if there is no enough memory
     */
    DynArray<T> &operator =(const DynArray<T> & array)
    {
      if (this == &array)
        return *this;

      copy_array(array);

      if (array.current_dim < current_dim)
        cut(array.current_dim);

      current_dim = array.current_dim;

      return *this;
    }

    /** Swap in constant time `array` with `this`

        @param[in] array to swap
        @note The sizes of directory, segment and block are also swapped
     */
    void swap(DynArray<T> & array) noexcept
    {
      std::swap(dir, array.dir);
      std::swap(pow_dir, array.pow_dir);
      std::swap(pow_seg, array.pow_seg);
      std::swap(pow_block, array.pow_block);
      std::swap(seg_plus_block_pow, array.seg_plus_block_pow);
      std::swap(mask_seg_plus_block, array.mask_seg_plus_block);
      std::swap(dir_size, array.dir_size);
      std::swap(seg_size, array.seg_size);
      std::swap(block_size, array.block_size);
      std::swap(mask_seg, array.mask_seg);
      std::swap(mask_block, array.mask_block);
      std::swap(max_dim, array.max_dim);
      std::swap(current_dim, array.current_dim);
      std::swap(num_segs, array.num_segs);
      std::swap(num_blocks, array.num_blocks);

      std::swap(default_initial_value, array.default_initial_value);

      default_initial_value_ptr = &default_initial_value;
      array.default_initial_value_ptr = &array.default_initial_value;
    }

    /// Move constructor
    DynArray(DynArray && other) noexcept
      : pow_dir(Default_Pow_Dir),
        pow_seg(Default_Pow_Seg),
        pow_block(std::min(Default_Pow_Block, Max_Pow_Block)),
        seg_plus_block_pow(pow_seg + pow_block),
        mask_seg_plus_block(two_raised(seg_plus_block_pow) - 1),
        dir_size(two_raised(pow_dir)),
        seg_size(two_raised(pow_seg)),
        block_size(two_raised(pow_block)),
        max_dim(two_raised(seg_plus_block_pow + pow_dir)),
        mask_seg(seg_size - 1),
        mask_block(block_size - 1),
        current_dim(0),
        num_segs(0),
        num_blocks(0),
        dir(nullptr)
    {
      default_initial_value_ptr = &default_initial_value;
      swap(other);
    }

    /// Move assignment
    DynArray &operator =(DynArray && other) noexcept
    {
      swap(other);
      return *this;
    }

    /** Fast access without checking allocation and bound_min_clock checking

	The purpose of this method is to access the i-th entry in the
	fastest possible way. For that, no checks are done.

        @param[in] i index of entry to be accessed
        @see exist
    */
    T &access(const size_t i) const noexcept
    {
      assert(dir[index_in_dir(i)] != nullptr);
      assert(dir[index_in_dir(i)][index_in_seg(i)] != nullptr);

      return dir[index_in_dir(i)][index_in_seg(i)][index_in_block(i)];
    }

    /// \overload access(const size_t i)
    T &operator ()(const size_t i) const noexcept
    {
      return access(i);
    }

    /** Return `true` if the i-th entry is accessible.

        By accessible is understood that either the entry has been
        previously written or the block than would contain it is
        already allocated,
        @param[in] i index to test
     */
    bool exist(const size_t i) const
    {
      if (i >= max_dim)
        return false;

      const size_t pos_in_dir = index_in_dir(i);

      assert(pos_in_dir < dir_size);

      if (dir[pos_in_dir] == nullptr)
        return false;

      const size_t pos_in_seg = index_in_seg(i);

      assert(pos_in_seg < seg_size);

      if (dir[pos_in_dir][pos_in_seg] == nullptr)
        return false;

      return true;
    }

    /** Test if the i-th entry es writable,

        `test(i)` inspects if the entry i is already allocated. If
        affirmative, then a pointer to the entry in the array is
        returned. Otherwise, `nullptr` is returned.

        @param[in] i index to test.
        @return `nullptr` if the entry is not allocated; a valid
        pointer inside the array otherwise
     */
    T * test(const size_t i) const noexcept
    {
      if (i >= max_dim)
        return nullptr;

      const size_t pos_in_dir = index_in_dir(i);
      if (dir[pos_in_dir] == nullptr)
        return nullptr;

      const size_t pos_in_seg = index_in_seg(i);
      if (dir[pos_in_dir][pos_in_seg] == nullptr)
        return nullptr;

      return &dir[index_in_dir(i)][index_in_seg(i)][index_in_block(i)];
    }

    /** Touch the entry i.

        `touch(i)` testes if the block that would contain to i is
        already allocated. If this is not the case, then the block,
        and eventually the segment, is allocated. If everything is ok,
        the method returns a valid pointer to the entry inside the
        array.

        `touch(i)` is a concise and effective way to test and
        eventually to allocate memory for a new entry.

        @param[in] i index to touch
        @throw bad_alloc if there is no enough memory
        @see cut
     */
    T &touch(const size_t i)
    {
      if (i >= max_dim)
        resize_dir(i);

      const size_t pos_in_dir = index_in_dir(i);
      bool new_segment = false;
      if (dir[pos_in_dir] == nullptr)
        {
          allocate_segment(dir[pos_in_dir]);
          new_segment = true;
        }

      const size_t pos_in_seg = index_in_seg(i);
      if (dir[pos_in_dir][pos_in_seg] == nullptr)
        {
          try
            {
              allocate_block(dir[pos_in_dir][pos_in_seg]);
            }
          catch (...)
            {
              if (new_segment && dir[pos_in_dir] != nullptr)
                release_segment(dir[pos_in_dir]);
              throw;
            }
        }

      if (i >= current_dim)
        current_dim = i + 1;

      return dir[pos_in_dir][pos_in_seg][index_in_block(i)];
    }

    /** Allocate a range of entries.

        `reserve(l, r)` assures that all the entries comprised between
        l and r are allocated. After a successful call, any entry
        between l and r can be surely accessed with `access()`.

        @param[in] l lower index
        @param[in] r upper index
        @throw bad_alloc if there is no enough memory
        @throw domain_error if l is greater than r
     */
    void reserve(const size_t l, const size_t r)
    {
      ah_domain_error_if(l > r) << "invalid range";

      if (r >= max_dim)
        resize_dir(r);

      const size_t first_seg = index_in_dir(l);
      const size_t last_seg = index_in_dir(r);
      const size_t first_block = index_in_seg(l);
      const size_t last_block = index_in_seg(r);

      std::vector<size_t> new_segments;
      std::vector<std::pair<size_t, size_t>> new_blocks;

      try
        {
          for (size_t seg_idx = first_seg; seg_idx <= last_seg; ++seg_idx)
            {
              if (dir[seg_idx] == nullptr)
                {
                  allocate_segment(dir[seg_idx]);
                  new_segments.push_back(seg_idx);
                }

              size_t block_idx = (seg_idx == first_seg) ? first_block : 0;
              const size_t final_block =
                  (seg_idx == last_seg) ? last_block : seg_size - 1;

              while (block_idx <= final_block)
                {
                  if (dir[seg_idx][block_idx] == nullptr)
                    {
                      allocate_block(dir[seg_idx][block_idx]);
                      new_blocks.emplace_back(seg_idx, block_idx);
                    }

                  ++block_idx;
                }
            } // end for (...)
        }
      catch (...)
        {
          for (auto it = new_blocks.rbegin(); it != new_blocks.rend(); ++it)
            {
              const auto [seg_idx, block_idx] = *it;
              if (dir[seg_idx] != nullptr and dir[seg_idx][block_idx] != nullptr)
                release_block(dir[seg_idx][block_idx]);
            }

          for (auto it = new_segments.rbegin(); it != new_segments.rend(); ++it)
            {
              const auto seg_idx = *it;
              if (dir[seg_idx] != nullptr)
                release_segment(dir[seg_idx]);
            }

          throw;
        }

      if (r + 1 > current_dim)
        current_dim = r + 1;
    }

    /** Assure that the range between 0 and dim is allocated

        @param[in] dim upper index
        @throw bad_alloc if there is no enough memory
     */
    void reserve(const size_t dim)
    {
      if (dim > 0)
        reserve(0, dim - 1);
    }

    void cut_ne(const size_t new_dim = 0)
    {
      if (new_dim == 0)
        {
          release_all_segments_and_blocks();
          current_dim = 0;
          return;
        }

      const size_t old_dim = current_dim; // old dimension

      // segment and block first indexes
      const long idx_first_seg = index_in_dir(old_dim - 1);
      const long idx_first_block = index_in_seg(old_dim - 1);

      // segment and block last indexes
      const long idx_last_seg = index_in_dir(new_dim - 1);
      const long idx_last_block = index_in_seg(new_dim - 1);
      for (long idx_seg = index_in_dir(old_dim - 1); idx_seg >= idx_last_seg;
           --idx_seg) // recorre descendentemente los segmentos
        {
          if (dir[idx_seg] == nullptr) // ¿hay un segmento?
            continue; // no ==> Advance to the next

          long idx_block = // First block to be released
              idx_seg == idx_first_seg ? idx_first_block : seg_size - 1;

          // Libera descendentemente los bloques reservados del segmento
          while ((idx_seg > idx_last_seg and idx_block >= 0) or
                 (idx_seg == idx_last_seg and idx_block > idx_last_block))
            {
              if (dir[idx_seg][idx_block] != nullptr) // ¿Hay un bloque aquí?
                release_block(dir[idx_seg][idx_block]);

              --idx_block;
            }

          if (idx_block < 0)
            release_segment(dir[idx_seg]);
        }

      current_dim = new_dim; // Updates New Dimension
    }

    /** Cut the array to a new dimension; that is, it reduces the
      dimension of array and frees the remaining memory.

      @param[in] new_dim new dimension value
      @throw domain_error if `new_dim` is greater than current dim
   */
    void cut(const size_t new_dim = 0)
    {
      ah_length_error_if(new_dim > current_dim)
      << "new dimension greater that current dimension";
      cut_ne(new_dim);
    }

    /** Set a new dimension.

        If dimension is greater than the current, then more memory is
        allocated; otherwise the remaining memory is freed. In both cases,
        the current dimension is adjusted.

        @param[in] dim new dimension value
     */
    void adjust(const size_t dim)
    {
      if (dim > current_dim)
        reserve(dim);
      else
        cut(dim);
    }

    /// Empty the array. All the occupied memory is freed and the dimension
    /// is to set to zero
    void empty() noexcept { cut(0); }

    Proxy operator [](const size_t i) const
    {
      ah_out_of_range_error_if(i >= max_dim) << "index out of maximum range";

      return Proxy(const_cast<DynArray<T> &>(*this), i);
    }

    Proxy operator [](const size_t i)
    {
      if (i >= max_dim)
        resize_dir(i);

      return Proxy(const_cast<DynArray<T> &>(*this), i);
    }

    /// Allocate a new entry to the end of array. Increase the dimension
    /// and return a modifiable reference to the last entry
    T &append()
    {
      return touch(this->size());
    }

    /// Copy `data` to the end of array, increase the dimension and return
    /// a modifiable reference to the copied data
    T &append(const T & data)
    {
      T & ref = this->append();
      ref = data;
      return ref;
    }

    /// Move `data` to the end of array, increase the dimension and return
    /// a modifiable reference to the copied data
    T &append(T && data)
    {
      T & ref = this->append();
      ref = std::move(data);
      return ref;
    }

    T &insert(const T & item)
    {
      this->append();
      open_gap(*this, current_dim);
      T & ret = access(0);
      ret = item;
      return ret;
    }

    T &insert(T && item)
    {
      this->append();
      open_gap(*this, current_dim);
      T & ret = access(0);
      ret = std::forward<T>(item);
      return ret;
    }

    /// \overload append(const T & data)
    void push(const T & data) { this->append(data); }

    /// \overload append(T && data)
    T &push(T && data) { return this->append(std::forward<T>(data)); }

    /// \overload append(const T & data)
    void put(const T & data) { this->append(data); }

    /// \overload append(T && data)
    T &put(T && data) { return this->append(std::forward<T>(data)); }

    /** Given a valid reference to an item in the array, it removes it and
        decrease the dimension.

        @param[in] item valid reference to the item to remove
     */
    void remove(T & item)
    {
      ensure_not_empty("DynArray::remove(): empty array");
      std::swap(item, this->access(this->size() - 1));
      this->cut_ne(this->size() - 1);
    }

    /// \overload remove(T & item)
    void erase(T & item) { remove(item); }

    /// Return `true` if the array is empty
    bool is_empty() const noexcept { return this->size() == 0; }

    /// Reverse the order of items in array
    DynArray &reverse()
    {
      for (size_t i = 0, j = current_dim - 1; i < j; ++i, --j)
        swap(touch(i), touch(j));

      return *this;
    }

    /// Remove the last item of array (as if this was a stack)
    T pop()
    {
      ensure_not_empty("DynArray::pop(): empty array");
      T ret_val = std::move(this->access(this->size() - 1));
      cut(size() - 1);

      return ret_val;
    }

    /// Return a modifiable reference to the last item of stack
    T &top() const
    {
      ensure_not_empty("DynArray::top(): empty array");
      return (*this)[size() - 1];
    }

    /// Return a modifiable reference to the first item of array (as if
    /// this was a queue)
    T &get_first() const
    {
      ensure_not_empty("DynArray::get_first(): empty array");
      return (*this)[0];
    }

    /// Return a modifiable reference to the last item of array (as if
    /// this was a queue)
    T &get_last() const
    {
      ensure_not_empty("DynArray::get_last(): empty array");
      return (*this)[size() - 1];
    }

    /** Iterator on the elements of array.

        The visit order is from the lower index 0 to current dimension
        minus one.

        It is assumed that all the entries between 0 and current dimension
        have been allocated.

        @ingroup Sequences
    */
    class Iterator
    {
    protected:
      DynArray *array_ptr = nullptr;
      long curr_idx = 0;

    public:
      using Set_Type = DynArray;

      /// Default constructor creates an "end" iterator
      Iterator() noexcept = default;

      /// Initializes an iterator on `array`
      Iterator(const DynArray & array) noexcept
        : array_ptr(const_cast<DynArray *>(&array))
      {
        // empty
      }

      /// Return `true` if there is current item
      [[nodiscard]] bool has_curr() const noexcept
      {
        return curr_idx >= 0 and curr_idx < array_ptr->size();
      }

      [[nodiscard]] bool is_last() const noexcept { return curr_idx == array_ptr->size() - 1; }

      /// Return the current link guaranteeing no exception. Be careful.
      T &get_curr_ne() const noexcept { return array_ptr->access(curr_idx); }

      /** Return the current item

          @throw overflow_error if there is no current
      */
      T &get_curr() const
      {
        ah_overflow_error_if(curr_idx == array_ptr->size()) << "not current item in iterator";
        return get_curr_ne();
      }

      /// Return the ordinal position of current item
      [[nodiscard]] long get_pos() const noexcept { return curr_idx; }

      /// Move the iterator one position forward guaranteeing no
      /// exception. Be careful.
      void next_ne() noexcept { ++curr_idx; }

      /// Move the current a position forward. Throw `overflow_error` if
      /// current is the last item
      void next()
      {
        ah_overflow_error_if(curr_idx == array_ptr->size()) << "not current item in iterator";
        next_ne();
      }

      // Move the iterator one position backward guaranteeing no
      /// exception. Be careful.
      void prev_ne() noexcept { --curr_idx; }

      /// Move the current a position backward. Throw `underflow_error` if
      /// current is the first item
      void prev()
      {
        ah_underflow_error_if(curr_idx == -1) << "not current item in iterator";
        prev_ne();
      }

      /// Reset the iterator to the last item
      void reset_last() noexcept { curr_idx = array_ptr->size() - 1; }

      /// Put the iterator in the end state
      void end() noexcept { curr_idx = array_ptr->size(); }

      /// Reset the iterator to the first item
      void reset_first() noexcept { curr_idx = 0; }

      void set_pos(const long pos) noexcept { curr_idx = pos; }
    };

    Iterator get_it()
    {
      return Iterator(*this);
    }

    Iterator get_it() const
    {
      return Iterator(*this);
    }

    Iterator get_it(const size_t pos)
    {
      ah_out_of_range_error_if(pos >= size()) << "DynArray::get_it(pos): pos >= size()";
      Iterator it(*this);
      it.set_pos(static_cast<long>(pos));
      return it;
    }

    Iterator get_it(const size_t pos) const
    {
      return const_cast<DynArray *>(this)->get_it(pos);
    }

  private:
    // superfast array traversal
    template <class Operation>
    bool __traverse(Operation & operation)
    {
      size_t dir_idx = 0, seg_idx = 0, block_idx = 0;
      for (size_t i = 0; i < current_dim; ++i)
        {
          if (not operation(dir[dir_idx][seg_idx][block_idx]))
            return false;

          if (++block_idx == block_size)
            {
              block_idx = 0;
              if (++seg_idx == seg_size)
                {
                  seg_idx = 0;
                  ++dir_idx;
                }
            }
        }
      return true;
    }

  public:
    /** Traverse all the array and execute a conditioned operation
         must have the signature:

        \code
        bool operation(const T & item)
        \endcode

        If \code operation() \endcode returns `false` then the traversal is
        stopped; otherwise the traverse move to the next item.

        @param[in] operation
        @return `true` if all items are traversed; `false` otherwise
    */
    template <class Operation>
    bool traverse(Operation & operation) const
    {
      return const_cast<DynArray &>(*this).__traverse(operation);
    }

    /// \overload traverse(Operation & operation)
    template <class Operation>
    bool traverse(Operation & operation)
    {
      return __traverse(operation);
    }

    /// \overload traverse(Operation & operation)
    template <class Operation>
    bool traverse(Operation && operation) const
    {
      return traverse<Operation>(operation);
    }

    /// \overload traverse(Operation & operation)
    template <class Operation>
    bool traverse(Operation && operation)
    {
      return traverse<Operation>(operation);
    }
  };

  template <typename T>
  const size_t DynArray<T>::Default_Pow_Dir = 6; /* 64   */

  template <typename T>
  const size_t DynArray<T>::Default_Pow_Seg = 8; /* 256   */

  template <typename T>
  const size_t DynArray<T>::Default_Pow_Block = 12; /* 4096 */

  template <typename T>
  const size_t DynArray<T>::Max_Bits_Allowed = 8 * sizeof(size_t);

  template <typename T>
  const unsigned long long DynArray<T>::Max_Dim_Allowed =
      256 * 1024 * 1024 * 1024ull; // 256 Gb

  template <typename T>
  const size_t DynArray<T>::Max_Pow_Block =
      (Max_Bits_Allowed - Default_Pow_Dir - Default_Pow_Seg - 1);
} // end namespace Aleph

# endif /* TPL_DYNARRAY_H */
