
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
  @file tpl_dynArray.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNARRAY_H
# define TPL_DYNARRAY_H

# include <math.h>

# include <string>
# include <aleph.H>
# include <ahDry.H>
# include <htlist.H>

using namespace std;

using namespace Aleph;

namespace Aleph {
 
/** 
  @brief dynamic array lazy.

  @details This classdetails dynamic lazy Implement Agreement. By dynamic is
  Which means the memory is automatically Secluded in function
  dimension. For lazy which it means the memory Puede
  In time writing away.

  A dynamic array manages three very important values
  determine its dynamism. These values ​​Called son:
  - block_size: Ideally, dynamic UN As verse Flower Arrangement A
    block_size block group size. Initially, the
    No dynamic array block has Apartadó. Of When
    First Time to block an entry is written b [i], it departs from the UN
    will house b [i] and surrounding entries.
    As the mayor sea block_size value will be required
    Less appealing to the memory manager and will take advantage Mas La
    Locality of Reference given by the hardware cache, but since
    Block is continuing the probability of fault memory
    Increases. A small value makes it more likely Reservations memory away
  - Seg_size: The blocks are indexed array called Through UN
    segment.
  - Dir_size: Once thereof, the segments are indexed in other arrangement
    Called Directory.

  To achieve v A constant access access as efficient as possible, these
  Exact son sizes powers of two.

  A dynamic array only occupies newly initialized UN Settlement
  Reported continuous Directory.

  Maximum size of dynamic array esta for limited
  seg_size dir_size x x block_size elements.

  The dynamic array handles a real dimension, which is itself
  SPECIFIC build time and spreads
  automatically if entries are written above the dimension
  real.

  Elements can be accessed in different ways. It is the
  By the incumbent Director [], which is Performs
  All verifications of range, automatically turns away
  Flowers memory and Expand the dimension.

  Since the memory section unit block is not there
  Leida guarantee that a corresponding entry with an already
  Written.

  @param T Data type of the array.

  @ingroup Secuencias
*/
    template <typename T> 
class DynArray
{
  /* Look at the end of this file for seeing the values */
public:

  typedef T Item_Type;
  typedef T Key_Type;

  static const size_t Default_Pow_Dir;  
  static const size_t Default_Pow_Seg;  
  static const size_t Default_Pow_Block;

  static const size_t Max_Bits_Allowed; 

  static const unsigned long long Max_Dim_Allowed;

private:

  static const size_t Max_Pow_Block;

  mutable size_t pow_dir              = Default_Pow_Dir;
  mutable size_t pow_seg              = Default_Pow_Seg;
  mutable size_t pow_block            = 
    std::min(Default_Pow_Block, Max_Pow_Block);
  mutable size_t seg_plus_block_pow   = pow_seg + pow_block;
  mutable size_t mask_seg_plus_block = two_raised(seg_plus_block_pow) - 1;
  mutable size_t dir_size            = two_raised(pow_dir); /* = 2^pow_dir */
  mutable size_t seg_size            = two_raised(pow_seg); /* = 2^pow_seg */
  mutable size_t block_size          = two_raised(pow_block); /* = 2^pow_block */

    /* 2^(pow_dir + pow_seg + pow_block) - 1 */
  unsigned long long max_dim         = two_raised(seg_plus_block_pow + pow_dir);

  static size_t two_raised(const size_t n)
  {
    if (n >= Max_Bits_Allowed)
      throw std::overflow_error ("number of bits exceeds maximum allowed");

    return 1 << n;
  }

  static size_t compute_dim(size_t d, size_t s, size_t b)
  {
    return two_raised(d)*two_raised(s)*two_raised(b);
  }

public:

/** 
  @brief Proposed powers of two for directory and block segment
  are sufficient to accommodate an array of dimension n.
*/
  static void compute_sizes(const size_t n, size_t & d, size_t & s, size_t & b)
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

private:

  size_t mask_seg   = seg_size - 1;
  size_t mask_block = block_size - 1;

  size_t index_in_dir(const size_t i) const
  {
    assert( (pow_block + pow_seg) == seg_plus_block_pow );
    assert( seg_size * block_size == two_raised(seg_plus_block_pow) );
    assert( i >> seg_plus_block_pow == i/(seg_size*block_size) );

    return i >> seg_plus_block_pow;
  }

  size_t modulus_from_index_in_dir(const size_t i) const
  {
    assert( mask_seg_plus_block == seg_size*block_size - 1 );
    assert( (i & mask_seg_plus_block) == i%(seg_size*block_size) );

    return (i & mask_seg_plus_block);
  }

  size_t index_in_seg(const size_t & i) const
  {
    assert( two_raised(pow_block) == block_size );
    assert( (modulus_from_index_in_dir(i) >> pow_block) == 
            (i%(seg_size*block_size))/block_size );

    return modulus_from_index_in_dir(i) >> pow_block;
  }

  size_t index_in_block(const size_t i) const
  {
    assert( mask_block == block_size - 1 );
    assert( (modulus_from_index_in_dir(i) & mask_block) == 
            ((i%(seg_size*block_size))%block_size) );

    return modulus_from_index_in_dir(i) & mask_block;
  }

  size_t current_dim;  
  size_t num_segs   = 0;   
  size_t num_blocks = 0;
  T *** dir         = NULL;

  void fill_dir_to_null()
  {
    assert(dir != NULL);

    for (size_t i = 0; i < dir_size; ++i)
      dir[i] = NULL;
  }

  void fill_seg_to_null(T ** seg)
  {
    assert(seg != NULL);

    for (size_t i = 0; i < seg_size; ++i)
      seg[i] = NULL;
  }

  void allocate_dir()
  {
    dir = (T***) malloc(dir_size*sizeof(T**)); 
    if (dir == NULL)
      throw std::bad_alloc();
    
    fill_dir_to_null();
  }

  void resize_dir(const size_t i) /* Resize dir to fit index i */
  {
    assert(i >= max_dim);

    size_t new_pow_dir = pow_dir + 1;
    while (compute_dim(new_pow_dir, pow_seg, pow_block) <= i)
      ++new_pow_dir;
    
    const size_t new_dir_sz = two_raised(new_pow_dir);
    T *** new_dir = (T***) realloc(dir, new_dir_sz*sizeof(T**));
    if (new_dir == NULL)
      throw std::bad_alloc();

    dir = new_dir;
    for (size_t k = dir_size; k < new_dir_sz; ++k)
      dir[k] = NULL;

    pow_dir = new_pow_dir;
    dir_size = new_dir_sz;

    max_dim = compute_dim(pow_dir, pow_seg, pow_block);
  }

  void allocate_segment(T **& seg)
  {
    assert(seg == NULL);

    seg = new T* [seg_size];
    fill_seg_to_null(seg);
    ++num_segs;
  }

  T   default_initial_value = T();
  T * default_initial_value_ptr = &default_initial_value;

  void allocate_block(T *& block)
  {
    assert(block == NULL);

    block = new T [block_size];
    ++num_blocks;

    if (default_initial_value_ptr == NULL) 
      return;

    for (size_t i = 0; i < block_size; ++i)
      block[i] = *default_initial_value_ptr;
  }

  void release_segment(T **& seg)
  {
    assert(seg != NULL);

    delete [] seg;
    seg = NULL;
    --num_segs;
  }

  void release_block(T *& block)
  {
    assert(block != NULL);

    delete [] block;
    block = NULL;
    --num_blocks;
  }

  void release_blocks_and_segment(T ** & seg)
  {
    assert(seg != NULL);

    for(size_t i = 0; i < seg_size ; ++i)
      if (seg[i] != NULL) 
        release_block(seg[i]);

    release_segment(seg);
  }

  void release_all_segments_and_blocks()
  {
    assert(dir != NULL);

    for(size_t i = 0; i < dir_size ; ++i)
      if (dir[i] != NULL)
        release_blocks_and_segment(dir[i]); 

    current_dim = 0;
  }

  void release_dir()
  {
    if (dir == NULL)
      return;

    release_all_segments_and_blocks();
    if (dir != NULL)
      free(dir);

    dir = NULL;
    current_dim = 0;
  }

  static size_t next2Pow(const size_t number) 
  {
    return (size_t) ceil( log((float) number)/ log(2.0) );
  }

  size_t divide_by_block_size(const size_t number) const
  {
    assert(number/block_size == number >> pow_block);

    return number >> pow_block;
  }

  size_t modulus_by_block_size(const size_t number) const
  {
    assert((number%block_size) == (number & mask_block));

    return number & mask_block;
  }

  void advance_block_index(size_t block_index, size_t seg_index, 
                           const size_t len) const
  {
    if (block_index + len < block_size)
      {
        block_index += len;
        return;
      }

    seg_index += divide_by_block_size(len);
    block_index = modulus_by_block_size(len); 
  }

  void allocate_block(T *& block, T * src_block)
  {
    allocate_block(block);
    for (int i = 0; i < block_size; i++)
      block[i] = src_block[i];
  }

  void allocate_segment(T **& seg, T ** src_seg)
  {
    allocate_segment(seg);
    for (int i = 0; i < seg_size; i++)
      if (src_seg[i] != NULL)
        allocate_block(seg[i], src_seg[i]);
  }

  void allocate_dir(T *** src_dir)
  {
    allocate_dir();
    for (int i = 0; i < dir_size; i++)
      if (src_dir[i] != NULL)
        allocate_segment(dir[i], src_dir[i]);
  }

  class Proxy
  {
    size_t     index;
    size_t     pos_in_dir;
    size_t     pos_in_seg;
    size_t     pos_in_block;
    T **&      ref_seg;
    T *        block;
    DynArray & array;

  public:

    Proxy(DynArray<T> & _array, const size_t i) : 
      index(i), pos_in_dir(_array.index_in_dir(index)),
      pos_in_seg(_array.index_in_seg(index)), 
      pos_in_block(_array.index_in_block(index)),
      ref_seg(_array.dir[pos_in_dir]), block (NULL), array(_array)
    {
      if (ref_seg != NULL)
        block = ref_seg[pos_in_seg]; /* Block already exists for i entry */
    }

    operator T & () 
    {
      if (block == NULL)
        throw std::invalid_argument("accessed entry not been still written");
      return block[pos_in_block];
    }

    T * operator -> ()
    {
      bool seg_was_allocated_in_current_call = false;

    /** Is segment?
    No? ==> Get away! */
      if (ref_seg == NULL) 
	{     
	  array.allocate_segment(ref_seg);
	  seg_was_allocated_in_current_call = true; 
	}

      if (block == NULL) /* Test if block is allocated */
        {
	  try /* Treat block away */
	    {
	      array.allocate_block(block);
	      ref_seg[pos_in_seg] = block;

	      assert(array.dir[pos_in_dir] == ref_seg); 
	    }
	  catch (...) /* A failure occurs in the section of the block */
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

    Proxy & operator = (const T & data)
    {
      bool seg_was_allocated_in_current_call = false;

    /** Is segment?
    No? ==> Get away! */
      if (ref_seg == NULL) 
	{ 
	  array.allocate_segment(ref_seg);
	  seg_was_allocated_in_current_call = true; 
	}

      if (block == NULL) /* Test if block is allocated */
        {
	  try /* Treat block away */
	    {
	      array.allocate_block(block);
	      ref_seg[pos_in_seg] = block;

	      assert(array.dir[pos_in_dir] == ref_seg); 
	    }
	  catch (...) /* A failure occurs in the section of the block */
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

    Proxy & operator = (const Proxy & proxy)
    {
      if (proxy.block == NULL) /* Right operand can read? */
        throw std::domain_error("right entry has not been still written");

      if (&proxy == this) 
	return *this;

      bool seg_was_allocated_in_current_call = false;
    
    /** Is segment?
    No? ==> Get away! */
      if (ref_seg == NULL)
	{
	  array.allocate_segment(ref_seg);
	  seg_was_allocated_in_current_call = true; 
	}

      if (block == NULL) /* Test if block is allocated */
        {
	  try /* Treat block away*/
	    {
	      array.allocate_block(block);
	      ref_seg[pos_in_seg] = block;

	      assert(array.dir[pos_in_dir] == ref_seg); 
	    }
	  catch (...) /* A failure occurs in the section of the block */
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

/** 
  @brief Returns directory size.
*/
  size_t get_dir_size() const { return dir_size; }

/** 
  @brief Returns segment size.
*/
  size_t get_seg_size() const { return seg_size; }

/** 
  @brief Returns size block.
*/
  size_t get_block_size() const { return block_size; } 

/** 
  @brief Returns current size (farthest written index).
*/
  size_t size() const { return current_dim; }

/** 
  @brief Returns maximum allowable dimension.
*/
  size_t max_size() const { return max_dim; }

/** 
  @brief Returns the number of blocks currently paragraphs.
*/
  size_t get_num_blocks() const { return num_blocks; }

/** 
  @brief Set the default you should have a new entry.

  @details Normally, when a new block is withdrawn, the initial value
  of its elements it is given by the value assigned by the
  default constructor T :: T (). This primitive sets the value to
  placed on each element of a new block to the value value.

  @param[in] value Default entries that must take block.
*/
  void set_default_initial_value(const T & value)
  {
    default_initial_value     = value;
    default_initial_value_ptr = &default_initial_value;
  }

  void set_default_initial_value(T && value = T())
  {
    std::swap(default_initial_value, value);
    default_initial_value_ptr = &default_initial_value;
  }

/** 
  @brief Specialized builder.

  @details Defines a dynamic array sizes directory segment and block.

  @param[in] _pow_dir Power of two directory size.
  @param[in] _pow_seg Power of two size segment.
  @param[in] _pow_block Power of two block size.

  @throw bad_alloc if there is no memory to divide the
  directory. This may be because it is too large or
  because there is little available memory.
  @throw length_error Si los tamaños seleccionados exceden la
  máxima dimensión posible
  @throw overflow_error Si ocurre overflow de con las operaciones
  de bits. 
*/
  DynArray(size_t _pow_dir, size_t _pow_seg, size_t _pow_block) 
    throw (std::exception, std::bad_alloc, 
           std::length_error, std::overflow_error) 
    : pow_dir             ( _pow_dir                                 ),
      pow_seg             ( _pow_seg                                 ),
      pow_block           ( _pow_block                               ),
      seg_plus_block_pow  ( _pow_seg + _pow_block                    ),
      mask_seg_plus_block ( two_raised(seg_plus_block_pow) - 1       ),
      dir_size            ( two_raised(pow_dir)                      ), 
      seg_size            ( two_raised(pow_seg)                      ), 
      block_size          ( two_raised(pow_block)                    ), 
      max_dim             ( two_raised(seg_plus_block_pow + pow_dir) ), 
      mask_seg            ( seg_size - 1                             ),
      mask_block          ( block_size - 1                           ),
      current_dim         ( 0                                        ),
      num_segs            ( 0                                        ),
      num_blocks          ( 0                                        )
  {
    assert(Max_Dim_Allowed > 0);

    if (max_dim > Max_Dim_Allowed)
      throw std::length_error ("Dimension too large"); 

    allocate_dir();
  }

/** 
  @brief Default builder.

  @details This is the default constructor where you specify a
  dimension. Directory sizes, and block segment are
  automatically selected.
  
  @param[in] dim Initial size of the array (may expand
  automatically if you write beyond it)

  @throw bad_alloc if there is no memory to divide the
  directory. This may be because it is too large or
  because there is little available memory.
  @throw length_error If the selected sizes exceed the
  maximum dimension
  @throw overflow_error If overflow occurs with operations
  bit.
*/
  DynArray(const size_t dim = 0) 
    throw (std::exception, std::bad_alloc, 
           std::length_error, std::overflow_error) 
    : current_dim(dim)
  {
    assert(Max_Dim_Allowed > 0);

    if (max_dim > Max_Dim_Allowed)
      throw std::length_error ("Dimension too large"); 

    allocate_dir();
  }

  DynArray(const DynList<T> & list) : DynArray()
  {
    int i = 0;
    list.for_each([&i, this] (const T & item) { touch(i++) = item; });
  }

/** 
  @brief Destructor frees all memory occupied
*/
  ~DynArray() 
  { 
    release_dir();
  }

/** 
  @brief Copy from element to element.

  @details Copy all items to esta src_array. The copy is
  performed element by element from the writings
  src_array.
  
  @param [in] src_array Copy source arrangement.

  @note directory sizes, and block segment are
  independent.
*/

  void copy_array(const DynArray<T> & src_array)
  {
    for (int i = 0; i < src_array.current_dim; ++i)
      if (src_array.exist(i))
        (*this)[i] = src_array.access(i);
  }

/** 
  @brief Builder copy.

  @details Copy the array element by element array to esta.
  Directory sizes, esta block segment will be the same as the array.
  
  @param[in] array Settlement copy source.
  
  @throw bad_alloc If there is insufficient memory.
*/
  DynArray(const DynArray<T> & array) 
      throw (std::exception, std::bad_alloc)
    : pow_dir                   (array.pow_dir),
      pow_seg                   (array.pow_seg),
      pow_block                 (array.pow_block),
      seg_plus_block_pow        (array.seg_plus_block_pow),
      mask_seg_plus_block       (array.mask_seg_plus_block),
      dir_size                  (array.dir_size),
      seg_size                  (array.seg_size),
      block_size                (array.block_size),
      max_dim                   (array.max_dim),
      mask_seg                  (array.mask_seg),
      mask_block                (array.mask_block),
      current_dim               (0),
      num_segs                  (0),
      num_blocks                (0),
      dir                       (NULL),
      default_initial_value_ptr (array.default_initial_value_ptr)
  {
    allocate_dir(array.dir);
    copy_array(array);
  }

/** 
  @brief Asignation.

  @details This assigned to all existing array elements

  @param[in] array Under the copy source.

  @throw bad_alloc If there is insufficient memory.
*/
  DynArray<T> & operator = (const DynArray<T> & array) 
    throw (std::exception, std::bad_alloc)
  {
    if (this == &array) 
      return *this;

    copy_array(array);
    
    if (array.current_dim < current_dim)
      cut(array.current_dim);

    current_dim = array.current_dim;

    return *this;
  }

/** 
  @brief very fast exchange between arrangements.

  @details exchange swap all elements of the array in accordance with the
  Fix this within a very fast time constant.

  @param [in] array arrangement to exchange with esta

  @note directory sizes, and block segment are also
  exchanged.
*/
  void swap(DynArray<T> & array)
  {
    std::swap(dir, array.dir);
    std::swap(pow_dir,  array.pow_dir);
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
  }

  DynArray(DynArray && other)
    : pow_dir             ( Default_Pow_Dir                          ),
      pow_seg             ( Default_Pow_Seg                          ),
      pow_block           (std::min(Default_Pow_Block, Max_Pow_Block)), 
      seg_plus_block_pow  ( pow_seg + pow_block                      ),
      mask_seg_plus_block ( two_raised(seg_plus_block_pow) - 1       ),
      dir_size            ( two_raised(pow_dir)                      ), 
      seg_size            ( two_raised(pow_seg)                      ), 
      block_size          ( two_raised(pow_block)                    ), 
      max_dim             ( two_raised(seg_plus_block_pow + pow_dir) ),
      mask_seg            ( seg_size - 1                             ),
      mask_block          ( block_size - 1                           ),
      current_dim         ( 0                                        ),
      num_segs            ( 0                                        ),
      num_blocks          ( 0                                        ),
      dir                 (NULL                                      )
  {
    swap(other);
  }

  DynArray & operator = (DynArray && other)
  {
    swap(other);
    return *this;
  }

/** 
  @brief quick access without validation.

  @details This routine accesses the i-th element of fashion fix
  fastest possible detriment to check if the input
  It exists, either because it has not been written or because it is out of
  range.

  The purpose of this function is to realize a high speed access
  under the certainty of use.

  Regardless of whether or not the access is valid, this fuunción
  updates the current size of the array if index
  i is greater.

  @param [in] i index entry access.

  @note Use with extreme care. If the post has a block
  paragraph, then, at best, be a failure
  exception memory corresponding operating system. from
  Otherwise, then access will be made but it does not
  It implies that it is correct.

  @see exist
*/
  T & access(const size_t i) 
  {
    assert(dir[index_in_dir(i)] != NULL);
    assert(dir[index_in_dir(i)][index_in_seg(i)] != NULL);

    if (i >= current_dim)
      current_dim = i;

    return dir[index_in_dir(i)][index_in_seg(i)][index_in_block(i)];
  }

  T & operator () (const size_t i) 
  {
    return access(i);
  }

/** 
  @brief Quick access without validationbrief constant basis.

  @details This routine accesses the i-th element of fashion fix
  fastest possible detriment to check if the input
  It exists, either because it has not been written or because it is out of
  range.

  The purpose of this function is to realize a high speed access
  under the certainty of use.

  @param [in] i index entry access.

  @note Use with extreme care. If the post has a block
  paragraph, then, at best, be a failure
  exception memory corresponding operating system. from
  Otherwise, then access will be made but it does not
  It implies that it is correct.

  @see exist
*/
  const T & access(const size_t i) const
  {
    assert(dir[index_in_dir(i)] != NULL);
    assert(dir[index_in_dir(i)][index_in_seg(i)] != NULL);

    return dir[index_in_dir(i)][index_in_seg(i)][index_in_block(i)];
  }

  const T & operator () (const size_t i) const
  {
    return access(i);
  }
/** 
  @brief Returns true if the entry exists i.

  @details The method checks if the input i is present in memory; It is
  say, if you can read with certainty that there is a block
  section for entry.

  The method returns true does not necessarily mean that the
  entry was previously written (it could be an entry
  unwritten but adjacent to a written sharing the same
  block.

  @param [in] i index entry to be verified.

  @throw out_of_range If i exceeds the maximum dimension.
*/
  bool exist(const size_t i) const throw (std::exception, std::out_of_range)
  {
    if (i >= max_dim)
      throw std::out_of_range ("index out of maximun range");

    const size_t pos_in_dir = index_in_dir(i);

    assert(pos_in_dir < dir_size);

    if (dir[pos_in_dir] == NULL) 
      return false;

    const size_t pos_in_seg = index_in_seg(i);

    assert(pos_in_seg < seg_size);

    if (dir[pos_in_dir][pos_in_seg] == NULL) 
      return false;

    return true;
  }  

/** 
  @brief Check if there is an entry in a dynamic aareglo.

  @details test (i) examines the dynamic esta arrangement and, if the
  cell exists, ie, that his memory has been set aside,
  returns a pointer to the corresponding element to position
  i. Otherwise, the routine returns a null pointer.

  The main use of this routine is to verify the existence
  the entry and simultaneously get your address incase the
  it exists.

  @param [in] i index entry to try.

  @return A pointer to the input i of the array or NULL if
  they do not exist.
*/
  T * test(const size_t i) const
  {
    const size_t pos_in_dir = index_in_dir(i);
    if (dir[pos_in_dir] == NULL) 
      return NULL;

    const size_t pos_in_seg = index_in_seg(i);
    if (dir[pos_in_dir][pos_in_seg] == NULL) 
      return NULL;

    return &dir[index_in_dir(i)][index_in_seg(i)][index_in_block(i)];
  }

/** 
  @brief Touch the i-th entry in order to ensure that there is memory.

  @details touch (i) checks if the host input block i has
  set apart and in the absence of block, remove the
  corresponding memory.

  @param[in] i Index of the entry to play.

  @throw bad_alloc If no memory to divide the block
  @throw out_of_range If "i" is greater than the maximum dimension

  @see cut
*/
  T & touch(const size_t i) 
    throw (std::exception, std::bad_alloc, std::out_of_range)
  {
    if (i >= max_dim)
      resize_dir(i);

    const size_t pos_in_dir = index_in_dir(i);
    if (dir[pos_in_dir] == NULL)
      allocate_segment(dir[pos_in_dir]);

    const size_t pos_in_seg = index_in_seg(i);
    if (dir[pos_in_dir][pos_in_seg] == NULL)
      {
        try
          {
	    allocate_block(dir[pos_in_dir][pos_in_seg]);
          }
        catch (...)
          {
            release_segment(dir[pos_in_dir]);
            throw;
          }
      }

    if (i >= current_dim)
      current_dim = i + 1;

    return dir[pos_in_dir][pos_in_seg][index_in_block(i)];
  }

/** 
  @brief Apart memory range.

  @details reserve (l, r) satisfies the input range of the array
  lyry between memory ensures there.

  @param[in] l Lower index
  @param[in] r Uper index

  @throw std::bad_alloc If there is insufficient memory. Then,
  no accurate way to know how far away it was the memory. 
  If l is larger than the former size, then the current dimension 
  value will be changed to the last block section.
  @throw domain_error Si l es mayor que r
  @throw length_error Si r es mayor que la máxima dimensión.
*/
  void reserve(const size_t l, const size_t r) 
    throw (std::exception, std::bad_alloc, std::domain_error, std::length_error)
  {
    if (l > r)
      throw std::domain_error("invalid range");

    if (r >= max_dim)
      resize_dir(r);

    const size_t first_seg   = index_in_dir(l);
    const size_t last_seg    = index_in_dir(r);
    const size_t first_block = index_in_seg(l);
    const size_t last_block  = index_in_seg(r);

    try
      {
	for (size_t seg_idx = first_seg; seg_idx <= last_seg; ++seg_idx)
	  {
	    if (dir[seg_idx] == NULL)
	      allocate_segment(dir[seg_idx]);
	    
	    size_t block_idx = (seg_idx == first_seg) ? first_block : 0;
	    const size_t final_block = 
	      (seg_idx == last_seg) ? last_block : seg_size - 1;

	    while (block_idx <= final_block)
	      {
		if (dir[seg_idx][block_idx] == NULL)
		  allocate_block(dir[seg_idx][block_idx]);

		++block_idx; 
	      }
	  } /* End for (...) */

	if (r + 1 > current_dim)
	  current_dim = r + 1;
      }
    catch (...)
      {
        if (r + 1 > current_dim)
          current_dim = r + 1;

        throw;
      }
  }

/** 
  @brief Book memory range.

  @details reerve (dim) verifies all tickets Fix
  between 0 and dim - 1 and ensures that there is memory.

  It is a method overload reserve (l, r).

  @param[in] dim Uper index

  @throw std::bad_alloc If there is insufficient memory. Then,
  no accurate way to know how far away it was the memory. 
  If l is larger than the former size, then the current dimension 
  value will be changed to the last block section.
  @throw length_error If dim is greater than the maximum dimension.
*/
  void reserve(const size_t dim) 
  {
    if (dim > 0)
      reserve(0, dim - 1);
  }

/** 
  @brief Cut a settlement; reduces its size.

  @details cut (dim) reduces the size of the under dim value. to
  the method makes sense, dim must be less than the dimension
  current. After the operation, the memory occupied by the
  above the new dimension blocks they have been released.

  In general, cut () is used to free memory.

  @param new_dim value of the new dimension. By default, this
  value is zero, in which case all blocks are released.

  @throw domain_error if new_dim is greater than the current size.
*/
  void cut(const size_t new_dim = 0) 
    throw (std::exception, std::domain_error) 
  {
    if (new_dim > current_dim)
      throw std::domain_error("new dimension greater that current dimension");

    if (new_dim == 0) 
      {
        release_all_segments_and_blocks();
        current_dim = 0;

        return;
      }

    const size_t old_dim = current_dim; /* Old dimention */

    /* Block indexes heights */
    const int idx_first_seg   = index_in_dir(old_dim - 1);
    const int idx_first_block = index_in_seg(old_dim - 1);

    /* Levels indices segments */
    const int idx_last_seg    = index_in_dir(new_dim - 1); 
    const int idx_last_block  = index_in_seg(new_dim - 1);    
    for (int idx_seg = index_in_dir(old_dim - 1); idx_seg >= idx_last_seg; 
         --idx_seg) /* Runs downwardly segments */
      {

    /* Exist a segment?
    No? ==> Go to the next */
        if (dir[idx_seg] == NULL) 
          continue; 

        int idx_block =  /* First block to release */
          idx_seg == idx_first_seg ? idx_first_block : seg_size - 1;

    /* Downward releases the reserved block of segment */
        while ( (idx_seg > idx_last_seg and idx_block >= 0) or
                (idx_seg == idx_last_seg and idx_block > idx_last_block) )
        {
          if (dir[idx_seg][idx_block] != NULL) /* is here a block? */
            release_block(dir[idx_seg][idx_block]);

          --idx_block; 
        }

        if (idx_block < 0)
          release_segment(dir[idx_seg]);
      }

    current_dim = new_dim; /* Upgrade new dimension */
  }

/** 
  @brief Unleash the memory occupied by the array,
  The dimension is zero. 
*/
  void empty() { cut(0); }

  Proxy operator [] (const size_t i) const
    throw (std::exception, std::bad_alloc, std::length_error, 
	   std::invalid_argument) 
  {
    if (i >= max_dim)
      throw std::out_of_range ("index out of maximun range");

    return Proxy (const_cast<DynArray<T>&>(*this), i);
  }

  Proxy operator [] (const size_t i) 
    throw (std::exception, std::length_error, 
           std::invalid_argument, std::bad_alloc)
  {
    if (i >= max_dim)
      resize_dir(i);

    return Proxy (const_cast<DynArray<T>&>(*this), i);
  }

/** 
  @brief Set aside at the end of arrreglo element.
  
  @return Returns reference to the secluded entrance.
*/
  T & append() 
  {
    return touch(this->size()); 
  }
  
/** 
  @brief Write an item to the end of the array. 

  @return Returns reference to the place where the element of the array was written.
*/
  T & append(const T & data) 
  { 
    T & ref = this->append();
    ref = data;
    return ref;
  }

  T & append(T && data) 
  { 
    T & ref = this->append();
    std::swap(ref, data);
    return ref;
  }
  
/** 
  @brief Inserted as an element in a pile at the end of the array.
*/
  void push(const T & data) { this->append(data); }

/** 
  @brief Away an element at the end of array.
*/
  T & push() { return this->append(); }

  bool is_empty() { return this->size() == 0; }

/** 
  @brief Elimina el último elemento del arreglo como si fuese una pila.
*/
  T pop() 
  {
    T ret_val = this->access(this->size() - 1);
    cut(size() - 1); 

    return ret_val;
  }

/** 
  @brief Returns the last element of the array as if it were a stack.
*/
  T & top() { return (*this)[size() - 1]; }

/** 
  @brief Returns the last element of the array as if it were a tail.
*/
  T & get_first() { return top(); } 

/** 
  @brief Returns the first element of the array as if it were a tail. 
*/
  T & get_last() { return (*this)[0]; }

  class Iterator
  {
    DynArray * array_ptr;
    long curr_idx;

  public:

    Iterator() : array_ptr(NULL), curr_idx(0) { /* Empty */ }

    Iterator(const DynArray & array) 
      : array_ptr(const_cast<DynArray*>(&array)), curr_idx(0)
    {
      /* Empty */
    }

    bool has_curr() const 
    {
      return curr_idx >= 0 and curr_idx < array_ptr->size(); 
    }

    bool has_current() const { return has_curr(); }

    T & get_curr() { return array_ptr->access(curr_idx); }

    void next() 
    {
      if (curr_idx == array_ptr->size())
	throw std::overflow_error("not current item in iterator");
	  
      ++curr_idx;
    }

    void prev()
    {
      if (curr_idx == -1)
	throw std::underflow_error("not current item in iterator");

      --curr_idx;
    }

    void reset_last() { curr_idx = array_ptr->size() - 1; }

    void reset_first() { curr_idx = 0; }

    void reset() { reset_first(); }
  };

private:

    /* Super fast array traversal */
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

/** 
  @brief Traverse all the set of pairs and for eah pair executes operation.

  @details Operation must have the signature

  @code
  bool operation(T & item)
  @endcode

  If @code operation() @endcode returns false then the traversal is
  aborted; otherwise the the routine advance and so on

  @param[in] operation

  @return true if all items are traversed; false otherwise
*/
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return const_cast<DynArray&>(*this).__traverse(operation);
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

  Functional_Methods(T);
};

    template <typename T> 
const size_t DynArray<T>::Default_Pow_Dir   = 6;  /* 64   */

    template <typename T> 
const size_t DynArray<T>::Default_Pow_Seg   = 8;  /* 256   */

    template <typename T> 
const size_t DynArray<T>::Default_Pow_Block = 12; /* 4096 */

    template <typename T> 
const size_t DynArray<T>::Max_Bits_Allowed  = 8 * sizeof(size_t); 

    template <typename T> 
const unsigned long long  DynArray<T>::Max_Dim_Allowed = 
      256*1024*1024*1024ull; /* 256 Gb */

    template <typename T> 
const size_t DynArray<T>::Max_Pow_Block =
  (Max_Bits_Allowed - Default_Pow_Dir  - Default_Pow_Seg  - 1);

} /* End namespace Aleph */

# endif /* TPL_DYNARRAY_H */
  