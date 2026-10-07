/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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

#ifndef TPL_DYNARRAY_H
#define TPL_DYNARRAY_H

#include <cmath>

#include <string>
#include <concepts>
#include <type_traits>
#include <aleph.H>
#include <ahIterator.H>
#include <array_utils.H>
#include <ahDry.H>
#include <tpl_dynDlist.H>
#include <ah-args-ctor.H>
#include <htlist.H>
#include <ah-dry.H>
#include <ah-errors.H>
#include <tpl_array.H>

namespace Aleph {
class BitArray;  // forward needed

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
   certain that this will be used, the operator `[]` is used. A read of
   an entry whose block was never allocated, through `read(i)` or `[]` on
   a constant array, some such as:

       std::cout << a.read(i) << endl;

   returns the default initialization value without allocating anything.
   This value is the result of the default constructor call, unless
   set_default_initial_value() changed it. On a modifiable array, `a[i]`
   returns a proxy that cannot tell a read from a write, so using it as a
   value allocates the block (it then holds the default value). Traversals
   and iterators never allocate.

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
   between \f$(0, 100000000)\f$ with read(), on a constant array or by
   traversing it, always it will be returned the default value and
   neither a segment nor a block will be allocated, since they have not
   been written.

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

   @note Due to the directory, segment, and block structuring, not all
   entries in the array are contiguous in memory. Only the entries
   within the same block are contiguous.

   @ingroup Sequences
 */
template <typename T>
class DynArray : public LocateFunctions<DynArray<T>, T>,
                 public FunctionalMethods<DynArray<T>, T>,
                 public GenericKeys<DynArray<T>, T>,
                 public EqualSequenceMethod<DynArray<T>>,
                 public StlAlephIterator<DynArray<T>>
{
  friend class BitArray;  // for access to __traversal() method

  // look at the end of this file for seeing the values
public:
  using Item_Type = T;  /// The type of element stored in the array
  using Key_Type = T;   /// The type of element stored in the array

  static const size_t Default_Pow_Dir;    /// Default two power for directory size
  static const size_t Default_Pow_Seg;    /// Default two power for segment size
  static const size_t Default_Pow_Block;  /// Default two power for block size

private:
  static const size_t Max_Bits_Allowed;

public:
  /// Maximum dimension allowed
  static const unsigned long long Max_Dim_Allowed;

private:
  static const size_t Max_Pow_Block;

  mutable size_t pow_dir = Default_Pow_Dir;
  mutable size_t pow_seg = Default_Pow_Seg;
  mutable size_t pow_block = std::min(Default_Pow_Block, Max_Pow_Block);
  mutable size_t seg_plus_block_pow = pow_seg + pow_block;
  mutable size_t mask_seg_plus_block = two_raised(seg_plus_block_pow) - 1;
  mutable size_t dir_size = two_raised(pow_dir);      // = 2^pow_dir
  mutable size_t seg_size = two_raised(pow_seg);      // = 2^pow_seg
  mutable size_t block_size = two_raised(pow_block);  // = 2^pow_block

  // 2^(pow_dir + pow_seg + pow_block) - 1
  unsigned long long max_dim = two_raised(seg_plus_block_pow + pow_dir);

  static size_t two_raised(const size_t n) noexcept
  {
    ah_overflow_error_if(n >= Max_Bits_Allowed) << "two_raised: exponent " << n << " is too large";
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
  static void compute_sizes(const size_t n, size_t &d, size_t &s, size_t &b) noexcept
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
  static std::tuple<size_t, size_t, size_t> compute_sizes(const size_t n) noexcept
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
    assert(i >> seg_plus_block_pow == i / (seg_size * block_size));

    return i >> seg_plus_block_pow;
  }

  size_t modulus_from_index_in_dir(const size_t i) const noexcept
  {
    assert(mask_seg_plus_block == seg_size * block_size - 1);
    assert((i & mask_seg_plus_block) == i % (seg_size * block_size));

    return (i & mask_seg_plus_block);
  }

  size_t index_in_seg(const size_t &i) const noexcept
  {
    assert(two_raised(pow_block) == block_size);
    assert((modulus_from_index_in_dir(i) >> pow_block) == (i % (seg_size * block_size)) / block_size);

    return modulus_from_index_in_dir(i) >> pow_block;
  }

  size_t index_in_block(const size_t i) const noexcept
  {
    assert(mask_block == block_size - 1);
    assert((modulus_from_index_in_dir(i) & mask_block) ==
           ((i % (seg_size * block_size)) % block_size));

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

  /** @brief Allocate the directory if the array has none.

      An array that was moved from keeps no directory, so that moving needs
      no allocation and stays noexcept. Every writer calls this before
      indexing the directory; readers treat a missing directory as an array
      without segments.

      @throw bad_alloc if the directory cannot be allocated.
  */
  void ensure_dir()
  {
    if (dir == nullptr)
      allocate_dir();
  }

  void resize_dir(const size_t i)  // resize dir to fit index i
  {
    assert(i >= max_dim);
    ensure_dir();  // realloc() must extend a fully initialized directory

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

  void allocate_segment(T **&seg)
  {
    assert(seg == nullptr);

    seg = new T *[seg_size];
    fill_seg_to_null(seg);
    ++num_segs;
  }

  T default_initial_value = T();
  T *default_initial_value_ptr = &default_initial_value;

  void allocate_block(T *&block)
  {
    assert(block == nullptr);

    block = new T[block_size];
    ++num_blocks;

    if (default_initial_value_ptr == nullptr)
      return;

    for (size_t i = 0; i < block_size; ++i)
      block[i] = *default_initial_value_ptr;
  }

  void release_segment(T **&seg) noexcept
  {
    assert(seg != nullptr);

    delete[] seg;
    seg = nullptr;
    --num_segs;
  }

  void release_block(T *&block) noexcept
  {
    assert(block != nullptr);

    delete[] block;
    block = nullptr;
    --num_blocks;
  }

  void release_blocks_and_segment(T **&seg) noexcept
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
    if (dir != nullptr)  // a moved-from array has no directory
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
    assert(number / block_size == number >> pow_block);

    return number >> pow_block;
  }

  size_t modulus_by_block_size(const size_t number) const noexcept
  {
    assert((number % block_size) == (number & mask_block));

    return number & mask_block;
  }

  void allocate_block(T *&block, T *src_block)
  {
    allocate_block(block);
    for (size_t i = 0; i < block_size; i++)
      block[i] = src_block[i];
  }

  void allocate_segment(T **&seg, T **src_seg)
  {
    allocate_segment(seg);
    for (size_t i = 0; i < seg_size; i++)
      if (src_seg[i] != nullptr)
        allocate_block(seg[i], src_seg[i]);
  }

  class Proxy
  {
    size_t index;
    size_t pos_in_dir;
    size_t pos_in_seg;
    size_t pos_in_block;
    T **&ref_seg;
    T *block;
    DynArray &array;

  public:
    Proxy(DynArray<T> &_array, const size_t i) noexcept
      : index(i), pos_in_dir(_array.index_in_dir(index)), pos_in_seg(_array.index_in_seg(index)),
        pos_in_block(_array.index_in_block(index)), ref_seg(_array.dir[pos_in_dir]), block(nullptr),
        array(_array)
    {
      if (ref_seg != nullptr)
        block = ref_seg[pos_in_seg];  // Entry block already exists
    }

    /** @brief Return a modifiable reference to the entry.

        The proxy cannot tell a read (`int x = a[i]`) from a modification
        (`a[i] += 1`), so it allocates the entry's block, exactly as
        operator->() does: an entry that was never written then holds the
        default value and writes through the reference are kept. To read
        without allocating, use read() or a const array.

        @return Reference to the entry.
        @throw bad_alloc if the segment or block cannot be allocated.
    */
    operator T &()
    {
      return *operator->();
    }

    T *operator ->()
    {
      bool seg_was_allocated_in_current_call = false;

      if (ref_seg == nullptr)  // Is there a segment?
        {                      // No ==> allocate it!
          array.allocate_segment(ref_seg);
          seg_was_allocated_in_current_call = true;
        }

      if (block == nullptr)  // test if block is allocated
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

    Proxy &operator = (const T &data)
    {
      bool seg_was_allocated_in_current_call = false;
      if (ref_seg == nullptr)  // Is there a segment?
        {                      // No ==> allocate ii!
          array.allocate_segment(ref_seg);
          seg_was_allocated_in_current_call = true;
        }

      if (block == nullptr)  // test if block is allocated
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

    Proxy &operator = (const Proxy &proxy)
    {
      if (&proxy == this)
        return *this;

      // An entry that was never written reads as its array's default value.
      // Allocating blocks never moves existing ones, so this reference stays
      // valid while the left entry is allocated below.
      const T &value = proxy.block != nullptr ? proxy.block[proxy.pos_in_block]
                                              : proxy.array.default_initial_value;

      bool seg_was_allocated_in_current_call = false;
      if (ref_seg == nullptr)  // Is there a segment?
        {                      // No ==> allocate it!
          array.allocate_segment(ref_seg);
          seg_was_allocated_in_current_call = true;
        }

      if (block == nullptr)  // test if block is allocated
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

      block[pos_in_block] = value;

      return *this;
    }
  };

public:
  /// Return the directory size
  size_t get_dir_size() const noexcept
  {
    return dir_size;
  }

  /// Return the segment size
  size_t get_seg_size() const noexcept
  {
    return seg_size;
  }

  /// Return the block size
  size_t get_block_size() const noexcept
  {
    return block_size;
  }

  /// Return the current dimension of array. According to usage style,
  /// this could represent the number of items stored in the array seen
  /// as a container
  size_t size() const noexcept
  {
    return current_dim;
  }

  /// Return the maximum allowed dimension (or the maximum number of
  /// elements that could have the array treated as a container). Be
  /// careful with the fact that **this bound_min_clock is not related to
  /// available memory**
  size_t max_size() const noexcept
  {
    return max_dim;
  }

  /// Return the number of blocks consumed by the array
  size_t get_num_blocks() const noexcept
  {
    return num_blocks;
  }

  /** Set the default value.

      The default value of a dynamic array is the value to be
      returned when entries not still written are accessed.

      @param[in] value default value
      @note Set it before writing. A block that is already allocated keeps
            the default value it was filled with; only blocks allocated
            afterwards, and reads of entries whose block does not exist,
            use the new value.
   */
  void set_default_initial_value(const T &value) noexcept
  {
    default_initial_value = value;
    default_initial_value_ptr = &default_initial_value;
  }

  /// \overload set_default_initial_value(const T & value)
  void set_default_initial_value(T &&value = T())
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
    : pow_dir(_pow_dir), pow_seg(_pow_seg), pow_block(_pow_block),
      seg_plus_block_pow(_pow_seg + _pow_block),
      mask_seg_plus_block(two_raised(seg_plus_block_pow) - 1), dir_size(two_raised(pow_dir)),
      seg_size(two_raised(pow_seg)), block_size(two_raised(pow_block)),
      max_dim(two_raised(seg_plus_block_pow + pow_dir)), mask_seg(seg_size - 1),
      mask_block(block_size - 1), current_dim(0), num_segs(0), num_blocks(0)
  {
    static_assert(std::is_copy_constructible_v<T>, "No copy constructor for T");
    static_assert(std::is_move_constructible_v<T>, "No move constructor for T");
    static_assert(std::is_copy_assignable_v<T>, "No copy assign for T");
    static_assert(std::is_move_assignable_v<T>, "No move assign for T");
    assert(Max_Dim_Allowed > 0);

    ah_length_error_if(max_dim > Max_Dim_Allowed) << "Dimension too large";

    allocate_dir();
  }

  /** @brief Construct an empty array with the default geometry.
      @throw bad_alloc if the directory cannot be allocated.
      @note Not explicit, so `DynArray<T> a = {};` and omitted aggregate
            members still work.
  */
  DynArray() : DynArray(size_t{0})
  { /* empty */
  }

  /** @brief Construct an array of logical size dim without allocating entries.

      Segments and blocks are allocated lazily, on the first write that falls
      in them. The directory covers the whole logical size from the start,
      so size() <= max_size(). Like `std::vector<T>(dim)`, append() then
      writes at index dim. Entries that were never written have no storage;
      check them with exist() or test() before reading them with access().

      The constructor is explicit: an integer never becomes a DynArray
      implicitly, so `DynArray<T> a = n;` and passing an integer where a
      DynArray is expected do not compile.

      @param[in] dim initial logical size; zero builds an empty array.
      @throw bad_alloc if the directory cannot be allocated.
      @throw length_error if dim is greater than Max_Dim_Allowed.
      @throw overflow_error if it happens an arithmetic overflow with
      the bits operations
  */
  explicit DynArray(const size_t dim) : current_dim(dim)
  {
    static_assert(std::is_default_constructible_v<T>, "No default constructor for T");
    static_assert(std::is_copy_constructible_v<T>, "No copy constructor for T");
    static_assert(std::is_move_constructible_v<T>, "No move constructor for T");
    static_assert(std::is_copy_assignable_v<T>, "No copy assign for T");
    static_assert(std::is_move_assignable_v<T>, "No move assign for T");
    assert(Max_Dim_Allowed > 0);

    ah_length_error_if(dim > Max_Dim_Allowed)
      << "DynArray: dimension " << dim << " is greater than the maximum allowed "
      << Max_Dim_Allowed;

    // Grow the directory geometry before allocating it, so that it covers
    // [0, dim) and nothing is allocated yet if a check fails.
    while (max_dim < dim)
      {
        ++pow_dir;
        dir_size = two_raised(pow_dir);
        max_dim = compute_dim(pow_dir, pow_seg, pow_block);
      }

    ah_length_error_if(max_dim > Max_Dim_Allowed) << "Dimension too large";

    allocate_dir();
  }

  /** @brief Construct an array of n copies of value; every entry is allocated.
      @param[in] n number of entries; size() is n.
      @param[in] value value copied into each entry.
      @throw bad_alloc if there is no enough memory.
      @throw length_error if n is greater than Max_Dim_Allowed.
      @throws Any exception from copy-assigning T; nothing is leaked.
      @note O(n), like `std::vector<T>(n, value)`.
  */
  DynArray(const size_t n, const T & value) : DynArray(n)
  {
    if (n == 0)
      return;

    reserve(0, n - 1);
    for (size_t i = 0; i < n; ++i)
      access(i) = value;
  }

  Special_Ctors(DynArray, T);

  ~DynArray()
  {
    release_dir();
  }

  /** Copy the items of `src_array` to `this`

      @param[in] src_array source array
  */
  void copy_array(const DynArray<T> &src_array)
  {
    for (size_t i = 0; i < src_array.current_dim; ++i)
      if (src_array.exist(i))
        (*this)[i] = src_array.access(i);
  }

  /** @brief Copy constructor: build an exact copy of array.

      The copy has the geometry, logical size and default value of `array`
      and allocates exactly the blocks that `array` has allocated, so an
      entry that `array` never wrote is not written in the copy either.

      @param[in] array source of copy
      @throw bad_alloc if there is no enough memory
      @throws Any exception from copy-assigning T; nothing is leaked, since
              the delegated constructor has already completed.
  */
  DynArray(const DynArray<T> &array)
    : DynArray(array.pow_dir, array.pow_seg, array.pow_block)
  {
    default_initial_value = array.default_initial_value;
    if (array.dir != nullptr)  // an array that was moved from has no directory
      for (size_t i = 0; i < dir_size; ++i)
        if (array.dir[i] != nullptr)
          allocate_segment(dir[i], array.dir[i]);
    current_dim = array.current_dim;
  }

  /** @brief Copy assignment: `*this` becomes an exact copy of array.

      @param[in] array source of copy
      @return `*this`
      @throw bad_alloc if there is no enough memory
      @note Strong guarantee: the copy is built first and then swapped in,
            so `*this` is unchanged if copying throws. The default value is
            copied, and entries that `array` never wrote are not left with
            the previous contents of `*this`.
   */
  DynArray<T> &operator = (const DynArray<T> &array)
  {
    if (this != &array)
      {
        DynArray<T> copy(array);
        swap(copy);
      }

    return *this;
  }

  /** Swap in constant time `array` with `this`

      @param[in] array to swap
      @note The sizes of directory, segment and block are also swapped
   */
  void swap(DynArray<T> &array) noexcept
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

  /** @brief Move constructor; allocates nothing.
      @param[in,out] other source; it is left empty and reusable, without a
      directory, which its first write allocates again.
  */
  DynArray(DynArray &&other) noexcept
    : pow_dir(Default_Pow_Dir), pow_seg(Default_Pow_Seg),
      pow_block(std::min(Default_Pow_Block, Max_Pow_Block)), seg_plus_block_pow(pow_seg + pow_block),
      mask_seg_plus_block(two_raised(seg_plus_block_pow) - 1), dir_size(two_raised(pow_dir)),
      seg_size(two_raised(pow_seg)), block_size(two_raised(pow_block)),
      max_dim(two_raised(seg_plus_block_pow + pow_dir)), mask_seg(seg_size - 1),
      mask_block(block_size - 1), current_dim(0), num_segs(0), num_blocks(0), dir(nullptr)
  {
    default_initial_value_ptr = &default_initial_value;
    swap(other);
  }

  /// Move assignment
  DynArray &operator = (DynArray &&other) noexcept
  {
    swap(other);
    return *this;
  }

  /** Fast access without checking allocation and bound_min_clock checking

      The purpose of this method is to access the i-th entry in the
      fastest possible way. For that, no checks are done.

      @param[in] i index of entry to be accessed
      @pre The block of entry i exists (exist(i) is true), for instance
           because the entry was written or reserve() covered it. Use read()
           to read an entry that may never have been written.
      @see exist
  */
  T &access(const size_t i) const noexcept
  {
    assert(dir[index_in_dir(i)] != nullptr);
    assert(dir[index_in_dir(i)][index_in_seg(i)] != nullptr);

    return dir[index_in_dir(i)][index_in_seg(i)][index_in_block(i)];
  }

  /// \overload access(const size_t i)
  T &operator () (const size_t i) const noexcept
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
    if (i >= max_dim or dir == nullptr)
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
  T *test(const size_t i) const noexcept
  {
    if (i >= max_dim or dir == nullptr)
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
    ensure_dir();
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
      between l and r can be surely accessed with `access()`, and
      size() is at least r + 1 (see the warning of reserve(size_t)).

      @param[in] l lower index
      @param[in] r upper index
      @throw bad_alloc if there is no enough memory. If allocating a
      segment or a block fails, the segments and blocks previously
      allocated by this call are released before the exception
      propagates.
      @throw domain_error if l is greater than r
      @note If every entry of the range is already allocated, the call
      performs no heap allocation.
   */
  void reserve(const size_t l, const size_t r)
  {
    ah_domain_error_if(l > r) << "invalid range";

    ensure_dir();
    if (r >= max_dim)
      resize_dir(r);

    const size_t first_seg = index_in_dir(l);
    const size_t last_seg = index_in_dir(r);
    const size_t first_block = index_in_seg(l);
    const size_t last_block = index_in_seg(r);

    // First pass: count the segments and blocks this call must allocate.
    // It keeps the common case, where the whole range already exists, free
    // of heap allocations, and it lets the rollback bookkeeping be sized
    // before anything is allocated. Recording an allocation then never
    // allocates, so it cannot fail and leave an allocation out of the
    // rollback.
    size_t missing_segs = 0;
    size_t missing_blocks = 0;
    for (size_t seg_idx = first_seg; seg_idx <= last_seg; ++seg_idx)
      {
        const size_t lo = (seg_idx == first_seg) ? first_block : 0;
        const size_t hi = (seg_idx == last_seg) ? last_block : seg_size - 1;
        if (dir[seg_idx] == nullptr)
          {
            ++missing_segs;
            missing_blocks += hi - lo + 1;
          }
        else
          for (size_t block_idx = lo; block_idx <= hi; ++block_idx)
            if (dir[seg_idx][block_idx] == nullptr)
              ++missing_blocks;
      }

    if (missing_blocks == 0)  // a missing segment always implies missing blocks
      {
        if (r + 1 > current_dim)
          current_dim = r + 1;
        return;
      }

    Array<size_t> new_segments;
    new_segments.reserve(missing_segs);
    Array<std::pair<size_t, size_t>> new_blocks;
    new_blocks.reserve(missing_blocks);

    try
      {
        for (size_t seg_idx = first_seg; seg_idx <= last_seg; ++seg_idx)
          {
            if (dir[seg_idx] == nullptr)
              {
                allocate_segment(dir[seg_idx]);
                new_segments.append(seg_idx);
              }

            size_t block_idx = (seg_idx == first_seg) ? first_block : 0;
            const size_t final_block = (seg_idx == last_seg) ? last_block : seg_size - 1;

            while (block_idx <= final_block)
              {
                if (dir[seg_idx][block_idx] == nullptr)
                  {
                    allocate_block(dir[seg_idx][block_idx]);
                    new_blocks.append(std::make_pair(seg_idx, block_idx));
                  }

                ++block_idx;
              }
          }  // end for (...)
      }
    catch (...)
      {
        for (size_t k = new_blocks.size(); k > 0; --k)
          {
            const auto [seg_idx, block_idx] = new_blocks(k - 1);
            if (dir[seg_idx] != nullptr and dir[seg_idx][block_idx] != nullptr)
              release_block(dir[seg_idx][block_idx]);
          }

        for (size_t k = new_segments.size(); k > 0; --k)
          if (const size_t seg_idx = new_segments(k - 1); dir[seg_idx] != nullptr)
            release_segment(dir[seg_idx]);

        throw;
      }

    if (r + 1 > current_dim)
      current_dim = r + 1;
  }

  /** @brief Allocate the first dim entries and make size() at least dim.

      After the call, every entry in [0, dim) can be accessed with access()
      or operator()(), which do not check allocation. Entries that were
      never written hold the default value.

      @param[in] dim number of entries to allocate.
      @throw bad_alloc if there is no enough memory.
      @warning Unlike `std::vector::reserve()` and `Array::reserve()`, this
               does not just reserve capacity: size() becomes dim if it was
               smaller. Do not follow it with append(), which writes after
               those dim entries; write them with `a(i) = ...` or
               `a[i] = ...`. To fill a DynArray with append() there is
               nothing to reserve: each block is allocated when needed.
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

    if (dir == nullptr)  // moved from: there is nothing to release
      {
        current_dim = std::min(current_dim, new_dim);
        return;
      }

    const size_t old_dim = current_dim;  // old dimension

    // segment and block first indexes
    const long idx_first_seg = index_in_dir(old_dim - 1);
    const long idx_first_block = index_in_seg(old_dim - 1);

    // segment and block last indexes
    const long idx_last_seg = index_in_dir(new_dim - 1);
    const long idx_last_block = index_in_seg(new_dim - 1);
    for (long idx_seg = index_in_dir(old_dim - 1); idx_seg >= idx_last_seg;
         --idx_seg)  // recorre descendentemente los segmentos
      {
        if (dir[idx_seg] == nullptr)  // ¿hay un segmento?
          continue;                   // no ==> Advance to the next

        long idx_block =  // First block to be released
          idx_seg == idx_first_seg ? idx_first_block : seg_size - 1;

        // Libera descendentemente los bloques reservados del segmento
        while ((idx_seg > idx_last_seg and idx_block >= 0) or
               (idx_seg == idx_last_seg and idx_block > idx_last_block))
          {
            if (dir[idx_seg][idx_block] != nullptr)  // ¿Hay un bloque aquí?
              release_block(dir[idx_seg][idx_block]);

            --idx_block;
          }

        if (idx_block < 0)
          release_segment(dir[idx_seg]);
      }

    // The entries of the last kept block in [new_dim, old_dim) leave the
    // array: reset them to the default value, so that growing it again shows
    // unwritten entries instead of stale ones. Entries past old_dim already
    // hold the default value, so a pop() costs O(1), not O(block_size).
    if (new_dim < old_dim)
      if (T * last = test(new_dim - 1); last != nullptr)
        {
          const size_t pos = index_in_block(new_dim - 1);
          const bool same_block = (new_dim - 1) / block_size == (old_dim - 1) / block_size;
          const size_t end = same_block ? index_in_block(old_dim - 1) + 1 : block_size;
          T * block = last - pos;
          for (size_t k = pos + 1; k < end; ++k)
            block[k] = default_initial_value;
        }

    current_dim = new_dim;  // Updates New Dimension
  }

  /** Cut the array to a new dimension; that is, it reduces the
    dimension of array and frees the remaining memory.

    @param[in] new_dim new dimension value
    @throw domain_error if `new_dim` is greater than current dim
 */
  void cut(const size_t new_dim = 0)
  {
    // Only shrinking is allowed; growing must be done via adjust().
    // If new_dim equals current_dim, this is a no-op and we return early.
    ah_length_error_if(new_dim > current_dim) << "new dimension greater than current dimension";

    if (new_dim == current_dim)
      return;

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
  void empty() noexcept
  {
    cut(0);
  }

  /** @brief Empties the container.

      @throws none
      @see empty()
  */
  void clear() noexcept
  {
    empty();
  }

  /** @brief Read entry i without allocating memory.

      Neither the directory, nor a segment, nor a block is allocated: an
      entry whose block was never allocated reads as the default value (see
      set_default_initial_value()). Every other entry that was never written
      holds the default value too, since new blocks are filled with it.

      @param[in] i index of the entry, smaller than size().
      @return Constant reference to the entry or to the default value.
      @throw out_of_range if i >= size().
      @note O(1).
  */
  [[nodiscard]] const T &read(const size_t i) const
  {
    ah_out_of_range_error_if(i >= current_dim)
      << "DynArray::read(" << i << "): index out of range; size() is " << current_dim;

    if (const T *entry = test(i); entry != nullptr)
      return *entry;

    return default_initial_value;
  }

  /** @brief Read entry i of a constant array; equivalent to read(i).
      @param[in] i index of the entry, smaller than size().
      @return Constant reference to the entry or to the default value.
      @throw out_of_range if i >= size().
      @note Never allocates memory. It returns `const T &`, not a proxy, so
            writing through a constant array does not compile.
  */
  [[nodiscard]] const T &operator [] (const size_t i) const
  {
    return read(i);
  }

  /** @brief Access entry i of a modifiable array through a proxy.

      Assigning to the proxy writes the entry and extends size() to i + 1
      if needed. Converting it to `T &`, as in `a[i] += 1` or
      `int x = a[i]`, allocates the entry's block, since a read cannot be
      told apart from a write; read() reads without allocating.

      @param[in] i index of the entry; it may be greater than size().
      @return Proxy to entry i.
      @throw bad_alloc if the directory cannot be grown.
  */
  Proxy operator [] (const size_t i)
  {
    ensure_dir();
    if (i >= max_dim)
      resize_dir(i);

    return Proxy(*this, i);
  }

  /// Allocate a new entry to the end of array. Increase the dimension
  /// and return a modifiable reference to the last entry
  T &append()
  {
    return touch(this->size());
  }

  /// Copy `data` to the end of array, increase the dimension and return
  /// a modifiable reference to the copied data
  T &append(const T &data)
  {
    T &ref = this->append();
    ref = data;
    return ref;
  }

  /// Move `data` to the end of array, increase the dimension and return
  /// a modifiable reference to the copied data
  T &append(T &&data)
  {
    T &ref = this->append();
    ref = std::move(data);
    return ref;
  }

  /** @brief Insert a copy of `item` at the beginning of the array.
      @param[in] item Value to insert; it may be an entry of this array,
                 as in `a.insert(a.access(1))`.
      @return Reference to the inserted copy, now entry 0.
      @throws std::bad_alloc or an exception from T.
      @note O(n): every entry shifts one position. The array is unchanged
            if the copy or the allocation throws.
  */
  T &insert(const T &item)
  {
    T tmp = item;  // open_gap() shifts the entry that item may be
    this->append();
    open_gap(*this, current_dim);
    T &ret = touch(0);  // the block of entry 0 may not exist in a lazy array
    ret = std::move(tmp);
    return ret;
  }

  /** @brief Move `item` to the beginning of the array.
      @param[in] item Value to insert; it may be an entry of this array,
                 which is then left moved from.
      @return Reference to the inserted item, now entry 0.
      @throws std::bad_alloc or an exception from T.
      @note O(n): every entry shifts one position. `item` is untouched if
            the allocation throws.
  */
  T &insert(T &&item)
  {
    this->append();          // allocate before item is moved from
    T tmp = std::move(item); // open_gap() shifts the entry that item may be
    open_gap(*this, current_dim);
    T &ret = touch(0);  // the block of entry 0 may not exist in a lazy array
    ret = std::move(tmp);
    return ret;
  }

  /// \overload append(const T & data)
  void push(const T &data)
  {
    this->append(data);
  }

  /// \overload append(T && data)
  T &push(T &&data)
  {
    return this->append(std::forward<T>(data));
  }

  /// \overload append(const T & data)
  void put(const T &data)
  {
    this->append(data);
  }

  /// \overload append(T && data)
  T &put(T &&data)
  {
    return this->append(std::forward<T>(data));
  }

  /** Given a valid reference to an item in the array, it removes it and
      decrease the dimension.

      @param[in] item valid reference to the item to remove
   */
  void remove(T &item)
  {
    ensure_not_empty("DynArray::remove(): empty array");
    std::swap(item, this->touch(this->size() - 1));  // the last entry may be unwritten
    this->cut_ne(this->size() - 1);
  }

  /// \overload remove(T & item)
  void erase(T &item)
  {
    remove(item);
  }

  /// Return `true` if the array is empty
  bool is_empty() const noexcept
  {
    return this->size() == 0;
  }

  /** Copy contents into Aleph::Array (requires copyable elements). */
  Array<T> to_array() const
    requires std::copy_constructible<T>
  {
    Array<T> ret;
    ret.reserve(this->size());
    for (size_t i = 0; i < this->size(); ++i)
      ret.append(this->read(i));  // unwritten entries read as the default value
    return ret;
  }

  /// Reverse the order of items in an array
  DynArray &reverse()
  {
    if (current_dim < 2)
      return *this;

    for (size_t i = 0, j = current_dim - 1; i < j; ++i, --j)
      std::swap(touch(i), touch(j));

    return *this;
  }

  /// Remove the last item of array (as if this was a stack)
  T pop()
  {
    ensure_not_empty("DynArray::pop(): empty array");
    const size_t last = this->size() - 1;
    T *entry = test(last);  // an unwritten last entry pops the default value
    T ret_val = entry != nullptr ? std::move(*entry) : default_initial_value;
    cut(last);

    return ret_val;
  }

  /// Return a modifiable reference to the last item of stack
  T &top() const
  {
    ensure_not_empty("DynArray::top(): empty array");
    // A modifiable reference: allocate the entry's block, as a[i] does.
    return const_cast<DynArray &>(*this).touch(size() - 1);
  }

  /// Return a modifiable reference to the first item of array (as if
  /// this was a queue)
  T &get_first() const
  {
    ensure_not_empty("DynArray::get_first(): empty array");
    return const_cast<DynArray &>(*this).touch(0);  // allocates, as a[i] does
  }

  /// Return a modifiable reference to the last item of array (as if
  /// this was a queue)
  T &get_last() const
  {
    ensure_not_empty("DynArray::get_last(): empty array");
    return const_cast<DynArray &>(*this).touch(size() - 1);  // allocates, as a[i] does
  }

  /** Iterator on the items of array.

      The visit order is from the lower index 0 to current dimension
      minus one.

      Iterating never allocates memory. An entry that was never written is
      seen as the default value: get_curr() then returns a reference to a
      copy held by the iterator, so writing through it has no effect on the
      array. Write unwritten entries through operator[], touch() or append().

      @ingroup Sequences
  */
  class Iterator
  {
  protected:
    DynArray *array_ptr = nullptr;
    long curr_idx = 0;
    mutable T unwritten_entry{};  ///< copy handed out for an entry never written

  public:
    using Set_Type = DynArray;

    /// Opt-in marker for StlIterator random-access promotion. It is the
    /// iterator's own type on purpose: derived iterators that reuse this
    /// class as a base (e.g. DynArrayHeap::Iterator, which redefines
    /// get_pos() with an offset) do NOT inherit the promotion, because their
    /// own type differs from this marker. This keeps the promotion fail-safe.
    using aleph_random_access_iterator = Iterator;

    /// Default constructor creates an "end" iterator
    Iterator() noexcept = default;

    /// Initializes an iterator on `array`
    Iterator(const DynArray &array) noexcept : array_ptr(const_cast<DynArray *>(&array))
    {
      // empty
    }

    /** @brief Check whether the iterator refers to an item.

        A default-constructed (singular) iterator behaves as an empty sequence.
        @return `true` if there is a current item; `false` for singular, empty,
                or out-of-range iterators.
    */
    [[nodiscard]] bool has_curr() const noexcept
    {
      return array_ptr != nullptr and curr_idx >= 0 and
             static_cast<size_t>(curr_idx) < array_ptr->size();
    }

    /** @brief Check whether the current item is the last item.

        @return `true` only when a current item exists and it is the last;
                `false` for singular or empty iterators.
    */
    [[nodiscard]] bool is_last() const noexcept
    {
      const long n = array_ptr == nullptr ? 0 : static_cast<long>(array_ptr->size());
      return has_curr() and curr_idx == n - 1;
    }

    /** @brief Return the current item without checking the position.
        @return Reference to the current entry or, if its block was never
                allocated, to a copy of the default value held by the
                iterator.
        @note Be careful: the position is not checked.
    */
    T &get_curr_ne() const noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
      if (T *entry = array_ptr->test(static_cast<size_t>(curr_idx)); entry != nullptr)
        return *entry;

      unwritten_entry = array_ptr->default_initial_value;
      return unwritten_entry;
    }

    /** @brief Return the current item.

        @return Reference to the current item.
        @throws std::underflow_error if positioned before the first item.
        @throws std::overflow_error if singular or at or past the end.
    */
    T &get_curr() const
    {
      ah_underflow_error_if(curr_idx < 0) << "not current item in iterator";
      ah_overflow_error_if(array_ptr == nullptr or
                           static_cast<size_t>(curr_idx) >= array_ptr->size())
        << "not current item in iterator";
      return get_curr_ne();
    }

    /// Return the ordinal position of current item
    [[nodiscard]] long get_pos() const noexcept
    {
      return curr_idx;
    }

    /// Move the iterator one position forward guaranteeing no
    /// exception. Be careful.
    void next_ne() noexcept
    {
      ++curr_idx;
    }

    /** @brief Advance one position, from the last item to the end if needed.

        @throws std::overflow_error if the iterator is singular or already
                at or past the end.
    */
    void next()
    {
      ah_overflow_error_if(array_ptr == nullptr or
                           (curr_idx >= 0 and
                            static_cast<size_t>(curr_idx) >= array_ptr->size()))
        << "not current item in iterator";
      next_ne();
    }

    // Move the iterator one position backward guaranteeing no
    /// exception. Be careful.
    void prev_ne() noexcept
    {
      --curr_idx;
    }

    /// Move the current a position backward. Throw `underflow_error` if
    /// current is the first item
    void prev()
    {
      ah_underflow_error_if(curr_idx == -1) << "not current item in iterator";
      prev_ne();
    }

    /// Reset the iterator to the last item
    void reset_last() noexcept
    {
      curr_idx = array_ptr == nullptr ? -1 : static_cast<long>(array_ptr->size()) - 1;
    }

    /// Put the iterator in the end state
    void end() noexcept
    {
      curr_idx = array_ptr == nullptr ? 0 : static_cast<long>(array_ptr->size());
    }

    /// Reset the iterator to the first item
    void reset_first() noexcept
    {
      curr_idx = 0;
    }

    void set_pos(const long pos) noexcept
    {
      curr_idx = pos;
    }
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
  // Fast traversal, block by block. It never allocates memory: the entries
  // of a block that does not exist are handed to `operation` as copies of
  // the default value, so modifying them has no effect on the array.
  template <class Operation>
  bool __traverse(Operation &operation)
  {
    size_t i = 0;
    while (i < current_dim)
      {
        const size_t count = std::min(block_size - index_in_block(i), current_dim - i);
        if (T *entry = test(i); entry != nullptr)
          {
            for (size_t k = 0; k < count; ++k)
              if (not operation(entry[k]))
                return false;
          }
        else
          for (size_t k = 0; k < count; ++k)
            {
              T unwritten = default_initial_value;
              if (not operation(unwritten))
                return false;
            }
        i += count;
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
      @note It never allocates memory. Entries that were never written are
            passed as copies of the default value; modifying them has no
            effect on the array.
  */
  template <class Operation>
  bool traverse(Operation &operation) const
  {
    return const_cast<DynArray &>(*this).__traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation &operation)
  {
    return __traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation &&operation) const
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation &&operation)
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
const unsigned long long DynArray<T>::Max_Dim_Allowed = 256 * 1024 * 1024 * 1024ull;  // 256 Gb

template <typename T>
const size_t DynArray<T>::Max_Pow_Block = (Max_Bits_Allowed - Default_Pow_Dir - Default_Pow_Seg - 1);
}  // end namespace Aleph

#endif /* TPL_DYNARRAY_H */
