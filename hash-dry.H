
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


/** @file hash-dry.H
 *  @brief Common hash table utilities and base classes.
 *
 *  Provides default hash functions, equal comparators, and common
 *  utilities used by all hash table implementations.
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef HASH_DRY_H
# define HASH_DRY_H

# include <ahFunction.H>

# define OHASH_COMMON(class_name)                                       \
  public:                                                               \
                                                                        \
  Hash_Fct get_hash_fct() const noexcept { return hash_fct; }           \
                                                                        \
private:                                                                \
                                                                        \
 Key * test_resize(Bucket * curr_bucket, const Key & key)               \
 {                                                                      \
   if (not ((with_resize and this->current_alpha() >= upper_alpha)))	\
     return &curr_bucket->key;                                          \
                                                                        \
   Key key_copy = key;                                                  \
   this->resize(Primes::next_prime(2*len));                             \
                                                                        \
   Key * key_ptr = search(key_copy);                                    \
   assert(key_ptr != nullptr);                                          \
   assert(cmp(*key_ptr, key_copy));                                     \
                                                                        \
   return key_ptr;                                                      \
 }                                                                      \
                                                                        \
 static void update_stat_len(DynArray<size_t> & lens, size_t i)         \
 {                                                                      \
   if (lens.exist(i))                                                   \
     lens(i) += 1;                                                      \
   else                                                                 \
     lens.touch(i) = 1;                                                 \
 }                                                                      \
                                                                        \
public:                                                                 \
                                                                        \
 void set_hash_fct(Hash_Fct fct) noexcept                               \
 {                                                                      \
   hash_fct = fct;                                                      \
 }                                                                      \
                                                                        \
 void set_hash_fct(Hash_Fct_Ptr fct) noexcept                           \
 {                                                                      \
   hash_fct = Hash_Fct(fct);                                            \
 }

namespace Aleph
{

  extern const float hash_default_lower_alpha;

  extern const float hash_default_upper_alpha;

}


# endif // HASH_DRY_H
